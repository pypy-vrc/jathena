#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "battle.h"

#include "timer.h"
#include "nullpo.h"
#include "malloc.h"

#include "map.h"
#include "pc.h"
#include "skill.h"
#include "mob.h"
#include "itemdb.h"
#include "clif.h"
#include "pet.h"
#include "guild.h"
#include "status.h"
#include "party.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

int attr_fix_table[4][10][10];

struct Battle_Config battle_config;

/*==========================================
 * ��_�Ԃ̋�����Ԃ�
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
static int distance(int x0,int y0,int x1,int y1)
{
	int dx,dy;

	dx=abs(x0-x1);
	dy=abs(y0-y1);
	return dx>dy ? dx : dy;
}

/*==========================================
 * ���������b�N���Ă���Ώۂ̐���Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int battle_counttargeted(struct block_list *bl,struct block_list *src,int target_lv)
{
	nullpo_retr(0, bl);
	if(bl->type == BL_PC)
		return pc_counttargeted((struct map_session_data *)bl,src,target_lv);
	else if(bl->type == BL_MOB)
		return mob_counttargeted((struct mob_data *)bl,src,target_lv);
	return 0;
}

//-------------------------------------------------------------------

// �_���[�W�̒x��
struct battle_delay_damage_ {
	struct block_list *src;
	int target;
	int damage;
	int flag;
};
int battle_delay_damage_sub(int tid,unsigned int tick,int id,int data)
{
	struct battle_delay_damage_ *dat=(struct battle_delay_damage_ *)data;
	struct block_list *target=map_id2bl(dat->target);
	if( dat && map_id2bl(id)==dat->src && target && target->prev!=NULL)
		battle_damage(dat->src,target,dat->damage,dat->flag);
	free(dat);
	return 0;
}
int battle_delay_damage(unsigned int tick,struct block_list *src,struct block_list *target,int damage,int flag)
{
	struct battle_delay_damage_ *dat = (struct battle_delay_damage_*)aCalloc(1,sizeof(struct battle_delay_damage_));

	nullpo_retr(0, src);
	nullpo_retr(0, target);

	dat->src=src;
	dat->target=target->id;
	dat->damage=damage;
	dat->flag=flag;
	add_timer(tick,battle_delay_damage_sub,src->id,(int)dat);
	return 0;
}

// ���ۂ�HP�𑀍�
int battle_damage(struct block_list *bl,struct block_list *target,int damage,int flag)
{
	struct map_session_data *sd=NULL;
	struct status_change *sc_data=status_get_sc_data(target);
	short *sc_count;
	int i;

	nullpo_retr(0, target); //bl��NULL�ŌĂ΂�邱�Ƃ�����̂ő��Ń`�F�b�N

	if(damage==0 || target->type == BL_PET)
		return 0;

	if(target->prev == NULL)
		return 0;

	if(bl) {
		if(bl->prev==NULL)
			return 0;

		if(bl->type==BL_PC)
			sd=(struct map_session_data *)bl;
	}
		
	if(damage<0)
		return battle_heal(bl,target,-damage,0,flag);

	if(!flag && (sc_count=status_get_sc_count(target))!=NULL && *sc_count>0){
		// �����A�Ή��A����������
		if(sc_data[SC_FREEZE].timer!=-1)
			status_change_end(target,SC_FREEZE,-1);
		if(sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2==0)
			status_change_end(target,SC_STONE,-1);
		if(sc_data[SC_SLEEP].timer!=-1)
			status_change_end(target,SC_SLEEP,-1);
	}

	if(target->type==BL_MOB){	// MOB
		struct mob_data *md=(struct mob_data *)target;
		if(md && md->skilltimer!=-1 && md->state.skillcastcancel)	// �r���W�Q
			skill_castcancel(target,0);
		return mob_damage(bl,md,damage,0);
	}
	else if(target->type==BL_PC){	// PC

		struct map_session_data *tsd=(struct map_session_data *)target;

		if(tsd && tsd->sc_data && tsd->sc_data[SC_DEVOTION].val1){	// �f�B�{�[�V�������������Ă���
			struct map_session_data *md = map_id2sd(tsd->sc_data[SC_DEVOTION].val1);
			if(md && skill_devotion3(&md->bl,target->id)){
				skill_devotion(md,target->id);
			}
			else if(md && bl)
				for(i=0;i<5;i++)
					if(md->dev.val1[i] == target->id){
						clif_damage(&md->bl,&md->bl, gettick(), 0, 0, 
							damage, 0 , 9, 0);
						pc_damage(&md->bl,md,damage);

						return 0;
					}
		}

		if(tsd && tsd->skilltimer!=-1){	// �r���W�Q
				// �t�F���J�[�h��W�Q����Ȃ��X�L�����̌���
			if( (!tsd->special_state.no_castcancel || map[bl->m].flag.gvg) && tsd->state.skillcastcancel &&
				!tsd->special_state.no_castcancel2)
				skill_castcancel(target,0);
		}

		return pc_damage(bl,tsd,damage);

	}
	else if(target->type==BL_SKILL)
		return skill_unit_ondamaged((struct skill_unit *)target,bl,damage,gettick());
	return 0;
}
int battle_heal(struct block_list *bl,struct block_list *target,int hp,int sp,int flag)
{
	nullpo_retr(0, target); //bl��NULL�ŌĂ΂�邱�Ƃ�����̂ő��Ń`�F�b�N

	if(target->type == BL_PET)
		return 0;
	if( target->type ==BL_PC && pc_isdead((struct map_session_data *)target) )
		return 0;
	if(hp==0 && sp==0)
		return 0;

	if(hp<0)
		return battle_damage(bl,target,-hp,flag);

	if(target->type==BL_MOB)
		return mob_heal((struct mob_data *)target,hp);
	else if(target->type==BL_PC)
		return pc_heal((struct map_session_data *)target,hp,sp);
	return 0;
}

// �U����~
int battle_stopattack(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB)
		return mob_stopattack((struct mob_data*)bl);
	else if(bl->type==BL_PC)
		return pc_stopattack((struct map_session_data*)bl);
	else if(bl->type==BL_PET)
		return pet_stopattack((struct pet_data*)bl);
	return 0;
}
// �ړ���~
int battle_stopwalking(struct block_list *bl,int type)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB)
		return mob_stop_walking((struct mob_data*)bl,type);
	else if(bl->type==BL_PC)
		return pc_stop_walking((struct map_session_data*)bl,type);
	else if(bl->type==BL_PET)
		return pet_stop_walking((struct pet_data*)bl,type);
	return 0;
}


/*==========================================
 * �_���[�W�̑����C��
 *------------------------------------------
 */
int battle_attr_fix(int damage,int atk_elem,int def_elem)
{
	int def_type= def_elem%10, def_lv=def_elem/10/2;

	if( atk_elem == 10 )	
		atk_elem = atn_rand()%9;	//���푮�������_���ŕt��

	if(	atk_elem<0 || atk_elem>9 || def_type<0 || def_type>9 ||
		def_lv<1 || def_lv>4){	// �� ���l�����������̂łƂ肠�������̂܂ܕԂ�
		if(battle_config.error_log)
			printf("battle_attr_fix: unknown attr type: atk=%d def_type=%d def_lv=%d\n",atk_elem,def_type,def_lv);
		return damage;
	}

	return damage*attr_fix_table[def_lv-1][atk_elem][def_type]/100;
}


/*==========================================
 * �_���[�W�ŏI�v�Z
 *------------------------------------------
 */
int battle_calc_damage(struct block_list *src,struct block_list *bl,int damage,int div_,int skill_num,int skill_lv,int flag)
{
	struct map_session_data *sd=NULL;
	struct mob_data *md=NULL;
	struct status_change *sc_data,*sc;
	short *sc_count;
	int class;

	nullpo_retr(0, bl);

	class = status_get_class(bl);
	if(bl->type==BL_MOB) md=(struct mob_data *)bl;
	else sd=(struct map_session_data *)bl;
	
	sc_data=status_get_sc_data(bl);
	sc_count=status_get_sc_count(bl);

	if(sc_count!=NULL && *sc_count>0){
		if (sc_data[SC_BASILICA].timer!=-1 && !(status_get_mode(src)&0x20))
			damage = 0;
		if (sc_data[SC_SAFETYWALL].timer!=-1 && damage>0 && flag&BF_WEAPON &&
					flag&BF_SHORT && skill_num != NPC_GUIDEDATTACK) {
			// �Z�[�t�e�B�E�H�[��
			struct skill_unit *unit;
			unit = (struct skill_unit *)sc_data[SC_SAFETYWALL].val2;
			if (unit) {
				if ((--unit->group->val2)<=0)
					skill_delunit(unit);
				damage=0;
			} else {
				status_change_end(bl,SC_SAFETYWALL,-1);
			}
		}
		
		// �j���[�}
		if(sc_data[SC_PNEUMA].timer!=-1 && damage>0 && flag&(BF_WEAPON|BF_MISC) && flag&BF_LONG && skill_num != NPC_GUIDEDATTACK){
			damage=0;
		}
		
		if(sc_data[SC_AETERNA].timer!=-1 && damage>0){	// ���b�N�X�G�[�e���i
			damage<<=1;
			status_change_end( bl,SC_AETERNA,-1 );
		}

		//������̃_���[�W����
		if(sc_data[SC_VOLCANO].timer!=-1){	// �{���P�[�m
			if(flag&BF_SKILL && skill_get_pl(skill_num)==3)
				damage += damage*sc_data[SC_VOLCANO].val4/100;
			else if(!flag&BF_SKILL && status_get_attack_element(bl)==3)
				damage += damage*sc_data[SC_VOLCANO].val4/100;
		}

		if(sc_data[SC_VIOLENTGALE].timer!=-1){	// �o�C�I�����g�Q�C��
			if(flag&BF_SKILL && skill_get_pl(skill_num)==4)
				damage += damage*sc_data[SC_VIOLENTGALE].val4/100;
			else if(!flag&BF_SKILL && status_get_attack_element(bl)==4)
				damage += damage*sc_data[SC_VIOLENTGALE].val4/100;
		}

		if(sc_data[SC_DELUGE].timer!=-1){	// �f�����[�W
			if(flag&BF_SKILL && skill_get_pl(skill_num)==1)
				damage += damage*sc_data[SC_DELUGE].val4/100;
			else if(!flag&BF_SKILL && status_get_attack_element(bl)==1)
				damage += damage*sc_data[SC_DELUGE].val4/100;
		}

		if(sc_data[SC_ENERGYCOAT].timer!=-1 && damage>0  && flag&BF_WEAPON){	// �G�i�W�[�R�[�g
			if(sd){
				if(sd->status.sp>0){
					int per = sd->status.sp * 5 / (sd->status.max_sp + 1);
					sd->status.sp -= sd->status.sp * (per * 5 + 10) / 1000;
					if( sd->status.sp < 0 ) sd->status.sp = 0;
					damage -= damage * ((per+1) * 6) / 100;
					clif_updatestatus(sd,SP_SP);
				}
				if(sd->status.sp<=0)
					status_change_end( bl,SC_ENERGYCOAT,-1 );
			}
			else
				damage -= damage * (sc_data[SC_ENERGYCOAT].val1 * 6) / 100;
		}

		if(sc_data[SC_KYRIE].timer!=-1 && damage > 0){	// �L���G�G���C�\��
			sc=&sc_data[SC_KYRIE];
			sc->val2-=damage;
			if(flag&BF_WEAPON){
				if(sc->val2>=0)	damage=0;
				else damage=-sc->val2;
			}
			if((--sc->val3)<=0 || (sc->val2<=0) || skill_num == AL_HOLYLIGHT)
				status_change_end(bl, SC_KYRIE, -1);
		}
		if(sc_data[SC_ENDURE].timer!=-1 && damage > 0 && flag&BF_WEAPON){	// �C���f���A
			if((--sc_data[SC_ENDURE].val2)<=0)
				status_change_end(bl, SC_ENDURE, -1);
		}
		/* �I�[�g�K�[�h */
		if(sc_data[SC_AUTOGUARD].timer != -1 && damage > 0 && flag&BF_WEAPON) {
			if(atn_rand()%100 < sc_data[SC_AUTOGUARD].val2) {
				int delay;
				damage = 0;
				clif_skill_nodamage(bl,bl,CR_AUTOGUARD,sc_data[SC_AUTOGUARD].val1,1);
				if (sc_data[SC_AUTOGUARD].val1 <= 5)
					delay = 300;
				else if (sc_data[SC_AUTOGUARD].val1 > 5 && sc_data[SC_AUTOGUARD].val1 <= 9)
					delay = 200;
				else
					delay = 100;
				if(sd)
					sd->canmove_tick = gettick() + delay;
				else if(md)
					md->canmove_tick = gettick() + delay;
			}
		}
		/* �p���C���O */
		if(sc_data[SC_PARRYING].timer != -1 && damage > 0 && flag&BF_WEAPON) {
			if(atn_rand()%100 < sc_data[SC_PARRYING].val2) {
				damage = 0;
				clif_skill_nodamage(bl,bl,LK_PARRYING,sc_data[SC_PARRYING].val1,1);
			}
		}
		// ���W�F�N�g�\�[�h
		if(sc_data[SC_REJECTSWORD].timer!=-1 && damage > 0 && flag&BF_WEAPON &&
		  ((src->type==BL_PC && ((struct map_session_data *)src)->status.weapon == (1 || 2 || 3)) || src->type==BL_MOB )){
			if(atn_rand()%100 < (15*sc_data[SC_REJECTSWORD].val1)){ //���ˊm����15*Lv
				damage = damage*50/100;
				battle_damage(bl,src,damage,0);
				//�_���[�W��^�����̂͗ǂ��񂾂��A��������ǂ����ĕ\������񂾂��킩��˂�
				//�G�t�F�N�g������ł����̂��킩��˂�
				clif_skill_nodamage(bl,bl,ST_REJECTSWORD,sc_data[SC_REJECTSWORD].val1,1);
				if((--sc_data[SC_REJECTSWORD].val2)<=0)
					status_change_end(bl, SC_REJECTSWORD, -1);
			}
		}
		if(sc_data[SC_SPIDERWEB].timer!=-1 && damage > 0)	// [Celest]
			if ((flag&BF_SKILL && skill_get_pl(skill_num)==3) ||
			  (!flag&BF_SKILL && status_get_attack_element(src)==3)) {
				damage<<=1;
				status_change_end(bl, SC_SPIDERWEB, -1);
			}
	}

	if(damage > 0) { //GvG
		struct guild_castle *gc=guild_mapname2gc(map[bl->m].name);
		struct guild *g;

		if(class == 1288) {
			if(flag&BF_SKILL)
				return 0;
			if(src->type == BL_PC) {
				g=guild_search(((struct map_session_data *)src)->status.guild_id);

				if(g == NULL)
					return 0;//�M���h�������Ȃ�_���[�W����
				else if((gc != NULL) && g->guild_id == gc->guild_id)
					return 0;//����̃M���h�̃G���y�Ȃ�_���[�W����
				else if(guild_checkskill(g,GD_APPROVAL) <= 0)
					return 0;//���K�M���h���F���Ȃ��ƃ_���[�W����
				else if (g && gc && guild_check_alliance(gc->guild_id, g->guild_id, 0) == 1)
					return 0;	// �����Ȃ�_���[�W����
			}
			else
				return 0;
		}
		if(map[bl->m].flag.gvg){
			if(gc && bl->type == BL_MOB){	//defense������΃_���[�W������炵���H
				damage -= damage*(gc->defense/100)*(battle_config.castle_defense_rate/100);
			}
			if(flag&BF_WEAPON) {
				if(flag&BF_SHORT)
					damage=damage*battle_config.gvg_short_damage_rate/100;
				if(flag&BF_LONG)
					damage=damage*battle_config.gvg_long_damage_rate/100;
			}
			if(flag&BF_MAGIC)
				damage = damage*battle_config.gvg_magic_damage_rate/100;
			if(flag&BF_MISC)
				damage=damage*battle_config.gvg_misc_damage_rate/100;
			if(damage < 1) damage  = 1;
		}
	}

	if(battle_config.skill_min_damage || flag&BF_MISC) {
		if(div_ < 255) {
			if(damage > 0 && damage < div_)
				damage = div_;
		}
		else if(damage > 0 && damage < 3)
			damage = 3;
	}

	if( md!=NULL && md->hp>0 && damage > 0 )	// �����Ȃǂ�MOB�X�L������
		mobskill_event(md,flag);
		

	//PC�̔���
	if(sd && sd->bl.type == BL_PC && src!=bl && sd->status.hp > 0 && damage > 0 )//&& flag&BF_WEAPON)
	{
		struct map_session_data *target=(struct map_session_data *)src;
		
		//�I�[�g�X�y��
		skill_bonus_autospell(&sd->bl,&target->bl,AS_REVENGE,0,0);
	}
	
	//PC�̔���
	if(sd && sd->bl.type == BL_PC && src!=bl &&
	 			sd->status.hp > 0 && damage > 0 && flag&BF_WEAPON)
	{
		struct map_session_data *target=(struct map_session_data *)src;
		//������Ԉُ�
		if(sd->addreveff_flag){
			int i;
			int rate;
			int luk;
			int sc_def_card=100;
			int sc_def_mdef,sc_def_vit,sc_def_int,sc_def_luk;
			//int race = status_get_race(target), ele = status_get_element(target);
			const int sc2[]={
				MG_STONECURSE,MG_FROSTDIVER,NPC_STUNATTACK,
				NPC_SLEEPATTACK,TF_POISON,NPC_CURSEATTACK,
				NPC_SILENCEATTACK,0,NPC_BLINDATTACK
			};
			//�Ώۂ̑ϐ�
			luk = status_get_luk(&target->bl);
			sc_def_mdef=100 - (3 + status_get_mdef(&target->bl) + luk/3);
			sc_def_vit=100 - (3 + status_get_vit(&target->bl) + luk/3);
			sc_def_int=100 - (3 + status_get_int(&target->bl) + luk/3);
			sc_def_luk=100 - (3 + luk);
			
			if(target->bl.type==BL_MOB){
				if(sc_def_mdef<50)
					sc_def_mdef=50;
				if(sc_def_vit<50)
					sc_def_vit=50;
				if(sc_def_int<50)
					sc_def_int=50;
				if(sc_def_luk<50)
					sc_def_luk=50;
			}
			
			if(sc_def_mdef<0)
				sc_def_mdef=0;
			if(sc_def_vit<0)
				sc_def_vit=0;
			if(sc_def_int<0)
				sc_def_int=0;

			for(i=SC_STONE;i<=SC_BLIND;i++){
				//�Ώۂɏ�Ԉُ�
				if(i==SC_STONE || i==SC_FREEZE)
					sc_def_card=sc_def_mdef;
				else if(i==SC_STAN || i==SC_POISON || i==SC_SILENCE)
					sc_def_card=sc_def_vit;
				else if(i==SC_SLEEP || i==SC_CONFUSION || i==SC_BLIND)
					sc_def_card=sc_def_int;
				else if(i==SC_CURSE)
					sc_def_card=sc_def_luk;

				if(battle_config.reveff_plus_addeff)
					rate = (sd->addreveff[i-SC_STONE] + sd->addeff[i-SC_STONE] + sd->arrow_addeff[i-SC_STONE])*sc_def_card/100;
				else 
					rate = (sd->addreveff[i-SC_STONE])*sc_def_card/100;
					
				if(target->bl.type == BL_PC)
				{
					if(atn_rand()%10000 < rate ){
						if(battle_config.battle_log)
							printf("PC %d skill_addreveff: card�ɂ��ُ픭�� %d %d\n",sd->bl.id,i,sd->addreveff[i-SC_STONE]);		
						status_change_start(&target->bl,i,7,0,0,0,(i==SC_CONFUSION)? 10000+7000:skill_get_time2(sc2[i-SC_STONE],7),0);
					}
				}else if(target->bl.type == BL_MOB)
				{
					if(atn_rand()%10000 < rate ){
						if(battle_config.battle_log)
							printf("PC %d skill_addreveff: card�ɂ��ُ픭�� %d %d\n",sd->bl.id,i,sd->addreveff[i-SC_STONE]);		
						status_change_start(&target->bl,i,7,0,0,0,(i==SC_CONFUSION)? 10000+7000:skill_get_time2(sc2[i-SC_STONE],7),0);
					}
				}
			}
		}
	}
	
	return damage;
}

/*==========================================
 * HP/SP�z���̌v�Z
 *------------------------------------------
 */
int battle_calc_drain(int damage, int rate, int per, int val)
{
	int diff = 0;

	if (damage <= 0 || rate <= 0)
		return 0;

	if (per && atn_rand()%100 < rate) {
		diff = (damage * per) / 100;
		if (diff == 0) {
			if (per > 0)
				diff = 1;
			else
				diff = -1;
		}
	}

	if (val && atn_rand()%100 < rate) {
		diff += val;
	}
	return diff;
}

/*==========================================
 * �C���_���[�W
 *------------------------------------------
 */
int battle_addmastery(struct map_session_data *sd,struct block_list *target,int dmg,int type)
{
	int damage,skill;
	int race=status_get_race(target);
	int weapon;
	damage = 0;

	nullpo_retr(0, sd);

	// �f�[�����x�C�� vs �s�� or ���� (���l�͊܂߂Ȃ��H)
	// DB�C���O: SkillLv * 3
	// DB�C����: floor( ( 3 + 0.05 * BaseLv ) * SkillLv )
	if((skill = pc_checkskill(sd,AL_DEMONBANE)) > 0 && (battle_check_undead(race,status_get_elem_type(target)) || race==6) ) {
		//damage += (skill * 3);
		damage += (int)(floor( ( 3 + 0.05 * sd->status.base_level ) * skill )); // sd�̓��e�͕ۏ؂���Ă���
	}

	// �r�[�X�g�x�C��(+4 �` +40) vs ���� or ����
	if((skill = pc_checkskill(sd,HT_BEASTBANE)) > 0 && (race==2 || race==4) )
	{
		damage += (skill * 4);
		
		if(sd->sc_data && sd->sc_data[SC_HUNTER].timer!=-1)
			damage += status_get_str(&sd->bl);
	}

	if(type == 0)
		weapon = sd->weapontype1;
	else
		weapon = sd->weapontype2;
	switch(weapon)
	{
		case 0x01:	// �Z�� (Updated By AppleGirl)
		case 0x02:	// 1HS
		{
			// ���C��(+4 �` +40) �Ў茕 �Z���܂�
			if((skill = pc_checkskill(sd,SM_SWORD)) > 0) {
				damage += (skill * 4);
			}
			break;
		}
		case 0x03:	// 2HS
		{
			// ���茕�C��(+4 �` +40) ���茕
			if((skill = pc_checkskill(sd,SM_TWOHAND)) > 0) {
				damage += (skill * 4);
			}
			break;
		}
		case 0x04:	// 1HL
		{
			// ���C��(+4 �` +40,+5 �` +50) ��
			if((skill = pc_checkskill(sd,KN_SPEARMASTERY)) > 0) {
				if(!pc_isriding(sd))
					damage += (skill * 4);	// �y�R�ɏ���ĂȂ�
				else
					damage += (skill * 5);	// �y�R�ɏ���Ă�
			}
			break;
		}
		case 0x05:	// 2HL
		{
			// ���C��(+4 �` +40,+5 �` +50) ��
			if((skill = pc_checkskill(sd,KN_SPEARMASTERY)) > 0) {
				if(!pc_isriding(sd))
					damage += (skill * 4);	// �y�R�ɏ���ĂȂ�
				else
					damage += (skill * 5);	// �y�R�ɏ���Ă�
			}
			break;
		}
		case 0x06:	// �Ў蕀
		{
			if((skill = pc_checkskill(sd,AM_AXEMASTERY)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x07: // Axe by Tato
		{
			if((skill = pc_checkskill(sd,AM_AXEMASTERY)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x08:	// ���C�X
		{
			// ���C�X�C��(+3 �` +30) ���C�X
			if((skill = pc_checkskill(sd,PR_MACEMASTERY)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x09:	// �Ȃ�?
			break;
		case 0x0a:	// ��
			break;
		case 0x0b:	// �|
			break;
		case 0x00:	// �f��
		case 0x0c:	// Knuckles
		{
			// �S��(+3 �` +30) �f��,�i�b�N��
			if((skill = pc_checkskill(sd,MO_IRONHAND)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x0d:	// Musical Instrument
		{
			// �y��̗��K(+3 �` +30) �y��
			if((skill = pc_checkskill(sd,BA_MUSICALLESSON)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x0e:	// Dance Mastery
		{
			// Dance Lesson Skill Effect(+3 damage for every lvl = +30) ��
			if((skill = pc_checkskill(sd,DC_DANCINGLESSON)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x0f:	// Book
		{
			// Advance Book Skill Effect(+3 damage for every lvl = +30) {
			if((skill = pc_checkskill(sd,SA_ADVANCEDBOOK)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x10:	// Katars
		{
			// �J�^�[���C��(+3 �` +30) �J�^�[��
			if((skill = pc_checkskill(sd,AS_KATAR)) > 0) {
				//�\�j�b�N�u���[���͕ʏ����i1���ɕt��1/8�K��)
				damage += (skill * 3);
			}
			// �A�h�o���X�h�J�^�[������
			if((skill = pc_checkskill(sd,ASC_KATAR)) > 0) {
				damage += dmg*(10+(skill * 2))/100;
			}
			
			break;
		}
	}
	damage = dmg + damage;
	return (damage);
}

static struct Damage battle_calc_pet_weapon_attack(
	struct block_list *src,struct block_list *target,int skill_num,int skill_lv,int wflag)
{
	struct pet_data *pd = (struct pet_data *)src;
	struct mob_data *tmd=NULL;
	int hitrate,flee,cri = 0,atkmin,atkmax;
	int luk,target_count = 1;
	int def1 = status_get_def(target);
	int def2 = status_get_def2(target);
	int t_vit = status_get_vit(target);
	int s_int=status_get_int(src);
	static struct Damage wd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	int damage,damage2=0,type,div_,blewcount=skill_get_blewcount(skill_num,skill_lv);
	int flag,dmg_lv=0;
	int t_mode=0,t_race=0,t_size=1,s_race=0,s_ele=0;
	struct status_change *t_sc_data;

	//return�O�̏���������̂ŏ��o�͕��̂ݕύX
	if( target == NULL || pd == NULL ){ //src�͓��e�ɒ��ڐG��Ă��Ȃ��̂ŃX���[���Ă݂�
		nullpo_info(NLP_MARK);
		memset(&wd,0,sizeof(wd));
		return wd;
	}
	
	puts("battle_calc_pet_weapon_attack");
	s_race=status_get_race(src);
	s_ele=status_get_attack_element(src);

	// �^�[�Q�b�g
	if(target->type == BL_MOB)
		tmd=(struct mob_data *)target;
	else {
		memset(&wd,0,sizeof(wd));
		return wd;
	}
	t_race=status_get_race( target );
	t_size=status_get_size( target );
	t_mode=status_get_mode( target );
	t_sc_data=status_get_sc_data( target );

	flag=BF_SHORT|BF_WEAPON|BF_NORMAL;	// �U���̎�ނ̐ݒ�
	
	// ��𗦌v�Z�A��𔻒�͌��
	flee = status_get_flee(target);
	if(battle_config.agi_penaly_type > 0 || battle_config.vit_penaly_type > 0)
		target_count += battle_counttargeted(target,src,battle_config.agi_penaly_count_lv);
	if(battle_config.agi_penaly_type > 0) {
		if(target_count >= battle_config.agi_penaly_count) {
			if(battle_config.agi_penaly_type == 1)
				flee = (flee * (100 - (target_count - (battle_config.agi_penaly_count - 1))*battle_config.agi_penaly_num))/100;
			else if(battle_config.agi_penaly_type == 2)
				flee -= (target_count - (battle_config.agi_penaly_count - 1))*battle_config.agi_penaly_num;
			if(flee < 1) flee = 1;
		}
	}
	//PvPMAP�Ō������ʂ�����邩�H
	if(((battle_config.gvg_flee_penaly ==1)||(battle_config.gvg_flee_penaly == 3))&&(map[target->m].flag.gvg))
		flee = flee*(200 - (battle_config.gvg_flee_rate))/100;//���ۂ�GvG��Flee������
	if(((battle_config.gvg_flee_penaly ==2)||(battle_config.gvg_flee_penaly == 3))&&(map[target->m].flag.pvp))
		flee = flee*(200 - (battle_config.gvg_flee_rate))/100;//���ۂ�PvP��Flee������
	hitrate=status_get_hit(src) - flee + 80;

	type=0;	// normal
	div_ = 1; // single attack

	luk=status_get_luk(src);

	if(battle_config.pet_str)
		damage = status_get_baseatk(src);
	else
		damage = 0;

	if(skill_num==HW_MAGICCRASHER){			/* �}�W�b�N�N���b�V���[��MATK�ŉ��� */
		atkmin = status_get_matk1(src);
		atkmax = status_get_matk2(src);
	}else{
		atkmin = status_get_atk(src);
		atkmax = status_get_atk2(src);
	}
	if(mob_db[pd->class].range>3 )
		flag=(flag&~BF_RANGEMASK)|BF_LONG;

	if(atkmin > atkmax) atkmin = atkmax;

	cri = status_get_critical(src);
	cri -= status_get_luk(target) * 3;
	if(battle_config.enemy_critical_rate != 100) {
		cri = cri*battle_config.enemy_critical_rate/100;
		if(cri < 1)
			cri = 1;
	}
	if(t_sc_data != NULL && t_sc_data[SC_SLEEP].timer!=-1 )
		cri <<=1;

	if(skill_num == 0 && skill_lv >= 0 && battle_config.enemy_critical && (atn_rand() % 1000) < cri)
	{
		damage += atkmax;
		type = 0x0a;
	}
	else {
		int vitbonusmax;
	
		if(atkmax > atkmin)
			damage += atkmin + atn_rand() % (atkmax-atkmin + 1);
		else
			damage += atkmin ;
		// �X�L���C���P�i�U���͔{���n�j
		// �I�[�o�[�g���X�g(+5% �` +25%),���U���n�X�L���̏ꍇ�����ŕ␳
		// �o�b�V��,�}�O�i���u���C�N,
		// �{�[�����O�o�b�V��,�X�s�A�u�[������,�u�����f�B�b�V���X�s�A,�X�s�A�X�^�b�u,
		// ���}�[�i�C�g,�J�[�g���{�����[�V����
		// �_�u���X�g���C�t�B���O,�A���[�V�����[,�`���[�W�A���[,
		// �\�j�b�N�u���[
		if(skill_num>0){
			int i;
			if( (i=skill_get_pl(skill_num))>0 )
				s_ele=i;

			flag=(flag&~BF_SKILLMASK)|BF_SKILL;
			switch( skill_num ){
			case SM_BASH:		// �o�b�V��
				damage = damage*(100+ 30*skill_lv)/100;
				hitrate = (hitrate*(100+5*skill_lv))/100;
				break;
			case SM_MAGNUM:		// �}�O�i���u���C�N
				damage = damage*(20*skill_lv + 100)/100;
				hitrate = hitrate*(10* skill_lv + 100)/100;
				break;
			case MC_MAMMONITE:	// ���}�[�i�C�g
				damage = damage*(100+ 50*skill_lv)/100;
				break;
			case AC_DOUBLE:	// �_�u���X�g���C�t�B���O
				damage = damage*(180+ 20*skill_lv)/100;
				div_=2;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case AC_SHOWER:	// �A���[�V�����[
				damage = damage*(75 + 5*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case AC_CHARGEARROW:	// �`���[�W�A���[
				damage = damage*150/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case KN_PIERCE:	// �s�A�[�X
				damage = damage*(100+ 10*skill_lv)/100;
				hitrate = hitrate*(100+5*skill_lv)/100;
				div_=t_size+1;
				damage*=div_;
				break;
			case KN_SPEARSTAB:	// �X�s�A�X�^�u
				damage = damage*(100+ 15*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case KN_SPEARBOOMERANG:	// �X�s�A�u�[������
				damage = damage*(100+ 50*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case KN_BRANDISHSPEAR: // �u�����f�B�b�V���X�s�A
				damage = damage*(100+ 20*skill_lv)/100;
				if(skill_lv>3 && wflag==1) damage2+=damage/2;
				if(skill_lv>6 && wflag==1) damage2+=damage/4;
				if(skill_lv>9 && wflag==1) damage2+=damage/8;
				if(skill_lv>6 && wflag==2) damage2+=damage/2;
				if(skill_lv>9 && wflag==2) damage2+=damage/4;
				if(skill_lv>9 && wflag==3) damage2+=damage/2;
				damage +=damage2;
				break;
			case KN_BOWLINGBASH:	// �{�E�����O�o�b�V��
				damage = damage*(100+ 50*skill_lv)/100;
				blewcount=0;
				break;
			case AS_SONICBLOW:	// �\�j�b�N�u���E
				damage = damage*(300+ 50*skill_lv)/100;
				div_=8;
				break;
			case AS_GRIMTOOTH:	// �O�����g�D�[�X
				damage = damage*(100+ 20*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case TF_SPRINKLESAND:	// ���܂�
				damage = damage*130/100;
				break;
			case MC_CARTREVOLUTION:	// �J�[�g���{�����[�V����
				damage = (damage*150)/100;
				break;
			// �ȉ�MOB
			case NPC_COMBOATTACK:	// ���i�U��
				div_=skill_get_num(skill_num,skill_lv);
				damage *= div_;
				break;
			case NPC_RANDOMATTACK:	// �����_��ATK�U��
				damage = damage*(50+atn_rand()%150)/100;
				break;
			// �����U���i�K���j
			case NPC_WATERATTACK:
			case NPC_GROUNDATTACK:
			case NPC_FIREATTACK:
			case NPC_WINDATTACK:
			case NPC_POISONATTACK:
			case NPC_HOLYATTACK:
			case NPC_DARKNESSATTACK:
			case NPC_TELEKINESISATTACK:
			case NPC_UNDEADATTACK:
				damage = damage*(100+25*(skill_lv-1))/100;
				break;
			case NPC_GUIDEDATTACK:
				hitrate = 1000000;
				break;
			case NPC_RANGEATTACK:
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case NPC_PIERCINGATT:
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
				break;
			case RG_BACKSTAP:	// �o�b�N�X�^�u
				damage = damage*(300+ 40*skill_lv)/100;
				hitrate = 1000000;
				break;
			case RG_RAID:	// �T�v���C�Y�A�^�b�N
				damage = damage*(100+ 40*skill_lv)/100;
				break;
			case RG_INTIMIDATE:	// �C���e�B�~�f�C�g
				damage = damage*(100+ 30*skill_lv)/100;
				break;
			case CR_SHIELDCHARGE:	// �V�[���h�`���[�W
				damage = damage*(100+ 20*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
				s_ele = 0;
				break;
			case CR_SHIELDBOOMERANG:	// �V�[���h�u�[������
				damage = damage*(100+ 30*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				s_ele = 0;
				break;
			case CR_HOLYCROSS:	// �z�[���[�N���X
				damage = damage*(100+ 35*skill_lv)/100;
				div_=2;
				break;
			case CR_GRANDCROSS:
			case NPC_DARKGRANDCROSS:
				hitrate= 1000000;
				break;
			case AM_DEMONSTRATION:	// �f�����X�g���[�V����
				hitrate= 1000000;
				damage = damage*(100+ 20*skill_lv)/100;
				damage2 = damage2*(100+ 20*skill_lv)/100;
				break;
			case AM_ACIDTERROR:	// �A�V�b�h�e���[
				hitrate= 1000000;
				damage = damage*(100+ 40*skill_lv)/100;
				damage2 = damage2*(100+ 40*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case MO_FINGEROFFENSIVE:	//�w�e
				damage = damage * (100 + 50 * skill_lv) / 100;
				div_ = 1;
				break;
			case MO_INVESTIGATE:	// �� ��
				if(def1 < 1000000)
					damage = damage*(100+ 75*skill_lv)/100 * (def1 + def2)/50;
				hitrate = 1000000;
				s_ele = 0;
				break;
			case MO_EXTREMITYFIST:	// ���C���e�P��
				damage = damage * 8 + 250 + (skill_lv * 150);
				hitrate = 1000000;
				s_ele = 0;
				break;
			case MO_CHAINCOMBO:	// �A�ŏ�
				damage = damage*(150+ 50*skill_lv)/100;
				div_=4;
				break;
			case MO_COMBOFINISH:	// �җ���
				damage = damage*(240+ 60*skill_lv)/100;
				break;
			case TK_STORMKICK://�����R��
				damage = damage*(160+ 20*skill_lv)/100;
				break;
			case TK_DOWNKICK://���i�R��
				damage = damage*(160+ 20*skill_lv)/100;
				break;
			case TK_TURNKICK://��]�R��
				damage = damage*(190+ 30*skill_lv)/100;
			case TK_COUNTER://�J�E���^�[�R��
				damage = damage*(190+ 30*skill_lv)/100;
				break;
			case DC_THROWARROW:	// ���
				damage = damage*(60+ 40 * skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case BA_MUSICALSTRIKE:	// �~���[�W�J���X�g���C�N
				damage = damage*(60+ 40 * skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case CH_TIGERFIST:	// ���Ռ�
				damage = damage*(40+ 100*skill_lv)/100;
				break;
			case CH_CHAINCRUSH:	// �A������
				damage = damage*(400+ 100*skill_lv)/100;
				div_=skill_get_num(skill_num,skill_lv);
				break;
			case CH_PALMSTRIKE:	// �ҌՍd�h�R
				damage = damage*(200+ 100*skill_lv)/100;
				break;
			case LK_SPIRALPIERCE:			/* �X�p�C�����s�A�[�X */
				damage = damage*(80+ 40*skill_lv)/100; //�����ʂ�������Ȃ��̂œK����
				div_=5;
				if(target->type == BL_PC)
					((struct map_session_data *)target)->canmove_tick = gettick() + 1000;
				else if(target->type == BL_MOB)
					((struct mob_data *)target)->canmove_tick = gettick() + 1000;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case LK_HEADCRUSH:				/* �w�b�h�N���b�V�� */
				damage = damage*(100+ 40*skill_lv)/100;
				break;
			case LK_JOINTBEAT:				/* �W���C���g�r�[�g */
				damage = damage*(50+ 10*skill_lv)/100;
				break;
			case ASC_METEORASSAULT:			/* ���e�I�A�T���g */
				damage = damage*(40+ 40*skill_lv)/100;
				break;
			case SN_SHARPSHOOTING:			/* �V���[�v�V���[�e�B���O */
				damage = damage*(200+50*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case CG_ARROWVULCAN:			/* �A���[�o���J�� */
				damage = damage*(200+100*skill_lv)/100;
				div_=9;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case AS_SPLASHER:		/* �x�i���X�v���b�V���[ */
				damage = damage*(500+50*skill_lv)/100;
				break;
			case PA_SHIELDCHAIN:	// �V�[���h�`�F�C��
				damage = damage*(100+ 10*skill_lv)/100;
				hitrate = hitrate*(100+(55-5*skill_lv))/100;
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
				div_=5;
				s_ele = 0;
				break;
			case WS_CARTTERMINATION:	/* �J�[�g�^�[�~�l�[�V���� */
				damage = damage*8000/(16-skill_lv)/100;
				break;
			case CG_TAROTCARD:
				break;
			case CR_ACIDDEMONSTRATION:	/* �A�V�b�h�f�����X�g���[�V���� */
				damage = damage*(s_int*skill_lv)/100;
				damage2 = damage2*(s_int*skill_lv)/100;
				if(t_vit>6){
					damage *= t_vit/6;
					damage2 *= t_vit/6;
				}
				if(target->type==BL_PC){
					damage /= 2;
					damage2 /= 2;
				}
				div_=skill_get_num( skill_num,skill_lv );
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case ITM_TOMAHAWK:		/* �g�}�z�[�N���� */
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			}
		}

		if( skill_num!=NPC_CRITICALSLASH ){
			// �Ώۂ̖h��͂ɂ��_���[�W�̌���
			// �f�B�o�C���v���e�N�V�����i�����ł����̂��ȁH�j
			if ( skill_num != MO_INVESTIGATE && skill_num != MO_EXTREMITYFIST && skill_num != KN_AUTOCOUNTER && skill_num != AM_ACIDTERROR && def1 < 1000000 ) {	//DEF, VIT����
				int t_def;
				target_count = 1 + battle_counttargeted(target,src,battle_config.vit_penaly_count_lv);
				if(battle_config.vit_penaly_type > 0) {
					if(target_count >= battle_config.vit_penaly_count) {
						if(battle_config.vit_penaly_type == 1) {
							def1 = (def1 * (100 - (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num))/100;
							def2 = (def2 * (100 - (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num))/100;
							t_vit = (t_vit * (100 - (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num))/100;
						}
						else if(battle_config.vit_penaly_type == 2) {
							def1 -= (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num;
							def2 -= (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num;
							t_vit -= (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num;
						}
						if(def1 < 0) def1 = 0;
						if(def2 < 1) def2 = 1;
						if(t_vit < 1) t_vit = 1;
					}
				}
				t_def = def2*8/10;
				vitbonusmax = (t_vit/20)*(t_vit/20)-1;
				if(battle_config.pet_defense_type) {
					damage = damage - (def1 * battle_config.pet_defense_type) - t_def - ((vitbonusmax < 1)?0: atn_rand()%(vitbonusmax+1) );
				}
				else{
					damage = damage * (100 - def1) /100 - t_def - ((vitbonusmax < 1)?0: atn_rand()%(vitbonusmax+1) );
				}
			}
		}
	}

	// 0�����������ꍇ1�ɕ␳
	if(damage<1) damage=1;

	// ����C��
	if(hitrate < 1000000)
		hitrate = ( (hitrate>95)?95: ((hitrate<5)?5:hitrate) );
	if(	hitrate < 1000000 &&			// �K���U��
		(t_sc_data != NULL && (t_sc_data[SC_SLEEP].timer!=-1 ||	// �����͕K��
		t_sc_data[SC_STAN].timer!=-1 ||		// �X�^���͕K��
		t_sc_data[SC_FREEZE].timer!=-1 || (t_sc_data[SC_STONE].timer!=-1 && t_sc_data[SC_STONE].val2==0) ) ) )	// �����͕K��
		hitrate = 1000000;
	if(type == 0 && atn_rand()%100 >= hitrate) {
		damage = damage2 = 0;
		dmg_lv = ATK_FLEE;
	}else{
		dmg_lv = ATK_DEF;
	}
	
	if(t_sc_data) {
		int cardfix=100;
		if(t_sc_data[SC_DEFENDER].timer != -1 && flag&BF_LONG)
			cardfix=cardfix*(100-t_sc_data[SC_DEFENDER].val2)/100;
		if(cardfix != 100)
			damage=damage*cardfix/100;
	}
	if(damage < 0) damage = 0;

	// �����̓K�p
	if(skill_num != 0 || s_ele != 0 || !battle_config.pet_attack_attr_none)
		damage=battle_attr_fix(damage, s_ele, status_get_element(target) );

	if(skill_num==PA_PRESSURE) /* �v���b�V���[ �K��? */
		damage = 500+300*skill_lv;

	// �C���x�i���C��
	if(skill_num==TF_POISON){
		damage = battle_attr_fix(damage + 15*skill_lv, s_ele, status_get_element(target) );
	}
	if(skill_num==MC_CARTREVOLUTION){
		damage = battle_attr_fix(damage, 0, status_get_element(target) );
	}

	// ���S����̔���
	if(battle_config.enemy_perfect_flee) {
		if(skill_num == 0 && skill_lv >= 0 && tmd!=NULL && atn_rand()%1000 < status_get_flee2(target) ){
			damage=0;
			type=0x0b;
			dmg_lv = ATK_LUCKY;
		}
	}

	if(t_mode&0x40 && damage > 0)
		damage = 1;

	if(skill_num != CR_GRANDCROSS||skill_num !=NPC_DARKGRANDCROSS)
		damage=battle_calc_damage(src,target,damage,div_,skill_num,skill_lv,flag);
		
	wd.damage=damage;
	wd.damage2=0;
	wd.type=type;
	wd.div_=div_;
	wd.amotion=status_get_amotion(src);
	if(skill_num == KN_AUTOCOUNTER)
		wd.amotion >>= 1;
	wd.dmotion=status_get_dmotion(target);
	wd.blewcount=blewcount;
	wd.flag=flag;
	wd.dmg_lv=dmg_lv;

	return wd;
}

static struct Damage battle_calc_mob_weapon_attack(
	struct block_list *src,struct block_list *target,int skill_num,int skill_lv,int wflag)
{
	struct map_session_data *tsd=NULL;
	struct mob_data* md=(struct mob_data *)src,*tmd=NULL;
	int hitrate,flee,cri = 0,atkmin,atkmax;
	int luk,target_count = 1;
	int def1 = status_get_def(target);
	int def2 = status_get_def2(target);
	int t_vit = status_get_vit(target);
	int s_int=status_get_int(src);
	static struct Damage wd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	int damage,damage2=0,damage_ot=0,type,div_,blewcount=skill_get_blewcount(skill_num,skill_lv);
	int flag,skill,ac_flag = 0,dmg_lv = 0;
	int t_mode=0,t_race=0,t_size=1,s_race=0,s_ele=0,s_size=1,s_group = 0;
	struct status_change *sc_data,*t_sc_data;
	short *sc_count;
	short *option, *opt1, *opt2;

	//return�O�̏���������̂ŏ��o�͕��̂ݕύX
	if( src == NULL || target == NULL || md == NULL ){
		nullpo_info(NLP_MARK);
		memset(&wd,0,sizeof(wd));
		return wd;
	}

	s_race=status_get_race(src);
	s_ele=status_get_attack_element(src);
	s_size=status_get_size( src ); //�Ώۂ̃T�C�Y
	s_group = status_get_group(src);
	sc_data=status_get_sc_data(src);
	sc_count=status_get_sc_count(src);
	option=status_get_option(src);
	opt1=status_get_opt1(src);
	opt2=status_get_opt2(src);
	
	// �^�[�Q�b�g
	if(target->type==BL_PC)
		tsd=(struct map_session_data *)target;
	else if(target->type==BL_MOB)
		tmd=(struct mob_data *)target;
	t_race=status_get_race( target );
	t_size=status_get_size( target );
	t_mode=status_get_mode( target );
	t_sc_data=status_get_sc_data( target );

	if((skill_num == 0 || (target->type == BL_PC && battle_config.pc_auto_counter_type&2) ||
		(target->type == BL_MOB && battle_config.monster_auto_counter_type&2)) && skill_lv >= 0) {
		if((skill_num != CR_GRANDCROSS||skill_num !=NPC_DARKGRANDCROSS)&& t_sc_data && t_sc_data[SC_AUTOCOUNTER].timer != -1) {
			int dir = map_calc_dir(src,target->x,target->y),t_dir = status_get_dir(target);
			int dist = distance(src->x,src->y,target->x,target->y);
			if(dist <= 0 || map_check_dir(dir,t_dir) ) {
				memset(&wd,0,sizeof(wd));
				t_sc_data[SC_AUTOCOUNTER].val3 = 0;
				t_sc_data[SC_AUTOCOUNTER].val4 = 1;
				if(sc_data && sc_data[SC_AUTOCOUNTER].timer == -1) {
					int range = status_get_range(target);
					if((target->type == BL_PC && ((struct map_session_data *)target)->status.weapon != 11 && dist <= range+1) ||
						(target->type == BL_MOB && range <= 3 && dist <= range+1) )
						t_sc_data[SC_AUTOCOUNTER].val3 = src->id;
				}
				return wd;
			}
			else ac_flag = 1;
		}
	}
	flag=BF_SHORT|BF_WEAPON|BF_NORMAL;	// �U���̎�ނ̐ݒ�

	// ��𗦌v�Z�A��𔻒�͌��
	flee = status_get_flee(target);
	if(battle_config.agi_penaly_type > 0 || battle_config.vit_penaly_type > 0)
		target_count += battle_counttargeted(target,src,battle_config.agi_penaly_count_lv);
	if(battle_config.agi_penaly_type > 0) {
		if(target_count >= battle_config.agi_penaly_count) {
			if(battle_config.agi_penaly_type == 1)
				flee = (flee * (100 - (target_count - (battle_config.agi_penaly_count - 1))*battle_config.agi_penaly_num))/100;
			else if(battle_config.agi_penaly_type == 2)
				flee -= (target_count - (battle_config.agi_penaly_count - 1))*battle_config.agi_penaly_num;
			if(flee < 1) flee = 1;
		}
	}
	//PvPMAP�Ō������ʂ�����邩�H
	if(((battle_config.gvg_flee_penaly ==1)||(battle_config.gvg_flee_penaly == 3))&&(map[target->m].flag.gvg))
		flee = flee*(200 - (battle_config.gvg_flee_rate))/100;//���ۂ�GvG��Flee������
	if(((battle_config.gvg_flee_penaly ==2)||(battle_config.gvg_flee_penaly == 3))&&(map[target->m].flag.pvp))
		flee = flee*(200 - (battle_config.gvg_flee_rate))/100;//���ۂ�PvP��Flee������
	hitrate=status_get_hit(src) - flee + 80;

	type=0;	// normal
	div_ = 1; // single attack

	luk=status_get_luk(src);

	if(battle_config.enemy_str)
		damage = status_get_baseatk(src);
	else
		damage = 0;
	if(skill_num==HW_MAGICCRASHER){			/* �}�W�b�N�N���b�V���[��MATK�ŉ��� */
		atkmin = status_get_matk1(src);
		atkmax = status_get_matk2(src);
	}else{
		atkmin = status_get_atk(src);
		atkmax = status_get_atk2(src);
	}
	if(mob_db[md->class].range>3 )
		flag=(flag&~BF_RANGEMASK)|BF_LONG;

	if(atkmin > atkmax) atkmin = atkmax;

	if(sc_data != NULL && sc_data[SC_MAXIMIZEPOWER].timer!=-1 ){	// �}�L�V�}�C�Y�p���[
		atkmin=atkmax;
	}

	cri = status_get_critical(src);
	cri -= status_get_luk(target) * 3;
	if(battle_config.enemy_critical_rate != 100) {
		cri = cri*battle_config.enemy_critical_rate/100;
		if(cri < 1)
			cri = 1;
	}
	if(t_sc_data != NULL && t_sc_data[SC_SLEEP].timer!=-1 )	// �������̓N���e�B�J�����{��
		cri <<=1;

	if(ac_flag) cri = 1000;

	if(skill_num == KN_AUTOCOUNTER) {
		if(!(battle_config.monster_auto_counter_type&1))
			cri = 1000;
		else
			cri <<= 1;
	}

	if(tsd && tsd->critical_def)
		cri = cri * (100 - tsd->critical_def) / 100;

	if((skill_num == 0 || skill_num == KN_AUTOCOUNTER) && skill_lv >= 0 && battle_config.enemy_critical && (atn_rand() % 1000) < cri) 	// ����i�X�L���̏ꍇ�͖����j
			// �G�̔���
	{
		damage += atkmax;
		type = 0x0a;
	}
	else {
		int vitbonusmax;
	
		if(atkmax > atkmin)
			damage += atkmin + atn_rand() % (atkmax-atkmin + 1);
		else
			damage += atkmin ;
		// �X�L���C���P�i�U���͔{���n�j
		// �I�[�o�[�g���X�g(+5% �` +25%),���U���n�X�L���̏ꍇ�����ŕ␳
		// �o�b�V��,�}�O�i���u���C�N,
		// �{�[�����O�o�b�V��,�X�s�A�u�[������,�u�����f�B�b�V���X�s�A,�X�s�A�X�^�b�u,
		// ���}�[�i�C�g,�J�[�g���{�����[�V����
		// �_�u���X�g���C�t�B���O,�A���[�V�����[,�`���[�W�A���[,
		// �\�j�b�N�u���[
		if(sc_data){ //��Ԉُ풆�̃_���[�W�ǉ�
			if(sc_data[SC_OVERTHRUST].timer!=-1)	// �I�[�o�[�g���X�g
				damage_ot += damage*(5*sc_data[SC_OVERTHRUST].val1)/100;
			if(sc_data[SC_OVERTHRUSTMAX].timer!=-1)	// �I�[�o�[�g���X�g�}�b�N�X
				damage_ot += damage*(100+20*sc_data[SC_OVERTHRUSTMAX].val1)/100;
			if(sc_data[SC_TRUESIGHT].timer!=-1)	// �g�D���[�T�C�g
				damage += damage*(2*sc_data[SC_TRUESIGHT].val1)/100;
			if(sc_data[SC_BERSERK].timer!=-1)	// �o�[�T�[�N
				damage += damage;
			if(sc_data && sc_data[SC_AURABLADE].timer!=-1)	//�I�[���u���[�h�����Ɉړ�
				damage += sc_data[SC_AURABLADE].val1 * 20;
		}

		if(skill_num>0){
			int i;
			if( (i=skill_get_pl(skill_num))>0 )
				s_ele=i;

			flag=(flag&~BF_SKILLMASK)|BF_SKILL;
			switch( skill_num ){
			case SM_BASH:		// �o�b�V��
				damage = damage*(100+ 30*skill_lv)/100;
				hitrate = (hitrate*(100+5*skill_lv))/100;
				break;
			case SM_MAGNUM:		// �}�O�i���u���C�N
				damage = damage*(20*skill_lv + 100)/100;
				hitrate = hitrate*(10* skill_lv + 100)/100;
				break;
			case MC_MAMMONITE:	// ���}�[�i�C�g
				damage = damage*(100+ 50*skill_lv)/100;
				break;
			case AC_DOUBLE:	// �_�u���X�g���C�t�B���O
				damage = damage*(180+ 20*skill_lv)/100;
				div_=2;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case AC_SHOWER:	// �A���[�V�����[
				damage = damage*(75 + 5*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case AC_CHARGEARROW:	// �`���[�W�A���[
				damage = damage*150/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case KN_PIERCE:	// �s�A�[�X
				damage = damage*(100+ 10*skill_lv)/100;
				hitrate=hitrate*(100+5*skill_lv)/100;
				div_=t_size+1;
				damage*=div_;
				break;
			case KN_SPEARSTAB:	// �X�s�A�X�^�u
				damage = damage*(100+ 15*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case KN_SPEARBOOMERANG:	// �X�s�A�u�[������
				damage = damage*(100+ 50*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case KN_BRANDISHSPEAR: // �u�����f�B�b�V���X�s�A
				damage = damage*(100+ 20*skill_lv)/100;
				if(skill_lv>3 && wflag==1) damage2+=damage/2;
				if(skill_lv>6 && wflag==1) damage2+=damage/4;
				if(skill_lv>9 && wflag==1) damage2+=damage/8;
				if(skill_lv>6 && wflag==2) damage2+=damage/2;
				if(skill_lv>9 && wflag==2) damage2+=damage/4;
				if(skill_lv>9 && wflag==3) damage2+=damage/2;
				damage +=damage2;
				break;
			case KN_BOWLINGBASH:	// �{�E�����O�o�b�V��
				damage = damage*(100+ 50*skill_lv)/100;
				blewcount=0;
				break;
			case KN_AUTOCOUNTER:
				if(battle_config.monster_auto_counter_type&1)
					hitrate += 20;
				else
					hitrate = 1000000;
				flag=(flag&~BF_SKILLMASK)|BF_NORMAL;
				break;
			case AS_SONICBLOW:	// �\�j�b�N�u���E
				damage = damage*(300+ 50*skill_lv)/100;
				div_=8;
				break;
			case AS_GRIMTOOTH:	// �O�����g�D�[�X
				damage = damage*(100+ 20*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case TF_SPRINKLESAND:	// ���܂�
				damage = damage*130/100;
				break;
			case MC_CARTREVOLUTION:	// �J�[�g���{�����[�V����
				damage = (damage*150)/100;
				break;
			// �ȉ�MOB
			case NPC_COMBOATTACK:	// ���i�U��
				div_=skill_get_num(skill_num,skill_lv);
				damage *= div_;
				break;
			case NPC_RANDOMATTACK:	// �����_��ATK�U��
				damage = damage*(50+atn_rand()%150)/100;
				break;
			// �����U���i�K���j
			case NPC_WATERATTACK:
			case NPC_GROUNDATTACK:
			case NPC_FIREATTACK:
			case NPC_WINDATTACK:
			case NPC_POISONATTACK:
			case NPC_HOLYATTACK:
			case NPC_DARKNESSATTACK:
			case NPC_TELEKINESISATTACK:
			case NPC_UNDEADATTACK:
				damage = damage*(100+25*(skill_lv-1))/100;
				break;
			case NPC_GUIDEDATTACK:
				hitrate = 1000000;
				break;
			case NPC_RANGEATTACK:
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case NPC_PIERCINGATT:
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
				break;
			case RG_BACKSTAP:	// �o�b�N�X�^�u
				damage = damage*(300+ 40*skill_lv)/100;
				hitrate = 1000000;
				break;
			case RG_RAID:	// �T�v���C�Y�A�^�b�N
				damage = damage*(100+ 40*skill_lv)/100;
				break;
			case RG_INTIMIDATE:	// �C���e�B�~�f�C�g
				damage = damage*(100+ 30*skill_lv)/100;
				break;
			case CR_SHIELDCHARGE:	// �V�[���h�`���[�W
				damage = damage*(100+ 20*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
				s_ele = 0;
				break;
			case CR_SHIELDBOOMERANG:	// �V�[���h�u�[������
				damage = damage*(100+ 30*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				s_ele = 0;
				break;
			case CR_HOLYCROSS:	// �z�[���[�N���X
				damage = damage*(100+ 35*skill_lv)/100;
				div_=2;
				break;
			case CR_GRANDCROSS:
			case NPC_DARKGRANDCROSS:
				hitrate= 1000000;
				break;
			case AM_DEMONSTRATION:	// �f�����X�g���[�V����
				hitrate= 1000000;
				damage = damage*(100+ 20*skill_lv)/100;
				damage2 = damage2*(100+ 20*skill_lv)/100;
				break;
			case AM_ACIDTERROR:	// �A�V�b�h�e���[
				hitrate= 1000000;
				damage = damage*(100+ 40*skill_lv)/100;
				damage2 = damage2*(100+ 40*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case MO_FINGEROFFENSIVE:	//�w�e
				damage = damage * (100 + 50 * skill_lv) / 100;
				div_ = 1;
				break;
			case MO_INVESTIGATE:	// �� ��
				if(def1 < 1000000)
					damage = damage*(100+ 75*skill_lv)/100 * (def1 + def2)/50;
				hitrate = 1000000;
				s_ele = 0;
				break;
			case MO_EXTREMITYFIST:	// ���C���e�P��
				damage = damage * 8 + 250 + (skill_lv * 150);
				hitrate = 1000000;
				s_ele = 0;
				break;
			case MO_CHAINCOMBO:	// �A�ŏ�
				damage = damage*(150+ 50*skill_lv)/100;
				div_=4;
				break;
			case BA_MUSICALSTRIKE:	// �~���[�W�J���X�g���C�N
				damage = damage*(60+ 40 * skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case DC_THROWARROW:	// ���
				damage = damage*(60+ 40 * skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case MO_COMBOFINISH:	// �җ���
				damage = damage*(240+ 60*skill_lv)/100;
				break;
			case CH_TIGERFIST:	// ���Ռ�
				damage = damage*(40+ 100*skill_lv)/100;
				break;
			case CH_CHAINCRUSH:	// �A������
				damage = damage*(400+ 100*skill_lv)/100;
				div_=skill_get_num(skill_num,skill_lv);
				break;
			case CH_PALMSTRIKE:	// �ҌՍd�h�R
				damage = damage*(200+ 100*skill_lv)/100;
				break;
			case TK_STORMKICK://�����R��
				damage = damage*(160+ 20*skill_lv)/100;
				break;
			case TK_DOWNKICK://���i�R��
				damage = damage*(160+ 20*skill_lv)/100;
				break;
			case TK_TURNKICK://��]�R��
				damage = damage*(190+ 30*skill_lv)/100;
			case TK_COUNTER://�J�E���^�[�R��
				damage = damage*(190+ 30*skill_lv)/100;
				break;
			case LK_SPIRALPIERCE:			/* �X�p�C�����s�A�[�X */
				damage = damage*(80+ 40*skill_lv)/100;
				div_=5;
				if(tsd)
					tsd->canmove_tick = gettick() + 1000;
				else if(tmd)
					tmd->canmove_tick = gettick() + 1000;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case LK_HEADCRUSH:				/* �w�b�h�N���b�V�� */
				damage = damage*(100+ 40*skill_lv)/100;
				break;
			case LK_JOINTBEAT:				/* �W���C���g�r�[�g */
				damage = damage*(50+ 10*skill_lv)/100;
				break;
			case ASC_METEORASSAULT:			/* ���e�I�A�T���g */
				damage = damage*(40+ 40*skill_lv)/100;
				break;
			case SN_SHARPSHOOTING:			/* �V���[�v�V���[�e�B���O */
				damage = damage*(200+50*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case CG_ARROWVULCAN:			/* �A���[�o���J�� */
				damage = damage*(200+100*skill_lv)/100;
				div_=9;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case AS_SPLASHER:		/* �x�i���X�v���b�V���[ */
				damage = damage*(500+50*skill_lv)/100;
				break;
			case PA_SHIELDCHAIN:	/* �V�[���h�`�F�C�� */
				damage = damage*(100+ 10*skill_lv)/100;
				hitrate = hitrate*(100+(55-5*skill_lv))/100;
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
				div_=5;
				s_ele = 0;
				break;
			case WS_CARTTERMINATION:	/* �J�[�g�^�[�~�l�[�V���� */
				damage = damage*8000/(16-skill_lv)/100;
				break;
			case CR_ACIDDEMONSTRATION:	/* �A�V�b�h�f�����X�g���[�V���� */
				damage = damage*(s_int*skill_lv)/100;
				damage2 = damage2*(s_int*skill_lv)/100;
				if(t_vit>6){
					damage *= t_vit/6;
					damage2 *= t_vit/6;
				}
				if(target->type==BL_PC){
					damage /= 2;
					damage2 /= 2;
				}
				div_=skill_get_num( skill_num,skill_lv );
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case ITM_TOMAHAWK:		/* �g�}�z�[�N���� */
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			}
		}

		damage += damage_ot;	//�I�[�o�[�g���X�g�A�I�[�o�[�g���X�g�}�b�N�X�C��

		if( skill_num!=NPC_CRITICALSLASH ){
			// �� �ۂ̖h��͂ɂ��_���[�W�̌���
			// �f�B�o�C���v���e�N�V�����i�����ł����̂��ȁH�j
			if ( skill_num != MO_INVESTIGATE && skill_num != MO_EXTREMITYFIST && skill_num != KN_AUTOCOUNTER && skill_num != AM_ACIDTERROR && def1 < 1000000) {	//DEF, VIT����
				int t_def;
				target_count = 1 + battle_counttargeted(target,src,battle_config.vit_penaly_count_lv);
				if(battle_config.vit_penaly_type > 0) {
					if(target_count >= battle_config.vit_penaly_count) {
						if(battle_config.vit_penaly_type == 1) {
							def1 = (def1 * (100 - (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num))/100;
							def2 = (def2 * (100 - (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num))/100;
							t_vit = (t_vit * (100 - (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num))/100;
						}
						else if(battle_config.vit_penaly_type == 2) {
							def1 -= (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num;
							def2 -= (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num;
							t_vit -= (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num;
						}
						if(def1 < 0) def1 = 0;
						if(def2 < 1) def2 = 1;
						if(t_vit < 1) t_vit = 1;
					}
				}
				t_def = def2*8/10;
				/* �f�B�o�C���v���e�N�V�����X�L���������Ă���̂�PC�����Ȃ̂ł��������ŗǂ��i�H
				   DP�X�L���C���O: SkillLv*3
				           �C����: round( ( 3 + 0.04 * BaseLv ) * SkillLv ) */
				if(battle_check_undead(s_race,status_get_elem_type(src)) || s_race==6)
					if(tsd && (skill=pc_checkskill(tsd,AL_DP)) > 0 ){
						//t_def += skill*3;
						// tsd�̓��e�͕ۏ؂���Ă���
						t_def += (int)(floor( ( 3 + 0.04 * tsd->status.base_level ) * skill + 0.5));
					}

				vitbonusmax = (t_vit/20)*(t_vit/20)-1;
				if(battle_config.monster_defense_type) {
					damage = damage - (def1 * battle_config.monster_defense_type) - t_def - ((vitbonusmax < 1)?0: atn_rand()%(vitbonusmax+1) );
				}
				else{
					damage = damage * (100 - def1) /100 - t_def - ((vitbonusmax < 1)?0: atn_rand()%(vitbonusmax+1) );
				}
			}
		}
	}

	// 0�����������ꍇ1�ɕ␳
	if(damage<1) damage=1;

	// ����C��
	if(hitrate < 1000000)
		hitrate = ( (hitrate>95)?95: ((hitrate<5)?5:hitrate) );
	if(	hitrate < 1000000 &&			// �K���U��
		(t_sc_data != NULL && (t_sc_data[SC_SLEEP].timer!=-1 ||	// �����͕K��
		t_sc_data[SC_STAN].timer!=-1 ||		// �X�^���͕K��
		t_sc_data[SC_FREEZE].timer!=-1 || (t_sc_data[SC_STONE].timer!=-1 && t_sc_data[SC_STONE].val2==0) ) ) )	// �����͕K��
		hitrate = 1000000;
	if(type == 0 && atn_rand()%100 >= hitrate) {
		damage = damage2 = 0;
		dmg_lv = ATK_FLEE;
	}else if(type == 0 && hitrate != 1000000 && t_sc_data &&
				t_sc_data[SC_KAUPE].timer !=-1 && atn_rand()%100 < t_sc_data[SC_KAUPE].val1*33)//�J�E�v
	{
		damage = damage2 = 0;
		dmg_lv = ATK_FLEE;
	}else if(tsd && t_sc_data && (flag&BF_LONG || t_sc_data[SC_SPURT].timer!=-1)&& t_sc_data[SC_DODGE].timer!=-1 && atn_rand()%100 < 20)//���@
	{
		int slv = pc_checkskill(tsd,TK_DODGE);
		damage = damage2 = 0;
		dmg_lv = ATK_FLEE;
		clif_skill_nodamage(&tsd->bl,&tsd->bl,TK_DODGE,slv,1);
		status_change_start(&tsd->bl,SC_DODGE_DELAY,slv,0,0,0,skill_get_time(TK_DODGE,slv),0);
	}else{
		dmg_lv = ATK_DEF;
	}
	//�J�E���^�[�L�b�N�̐�
	if(dmg_lv == ATK_DEF && tsd && t_sc_data && t_sc_data[SC_READYCOUNTER].timer!=-1)
	{
		int counter_rate = 20;
		if(t_sc_data && t_sc_data[SC_COUNTER_RATE_UP].timer!=-1)
		{
			int rate_up[] = {0,20,50,100};
			counter_rate += rate_up[pc_checkskill(tsd,SG_FRIEND)];
			status_change_end(&tsd->bl,SC_COUNTER_RATE_UP,-1);
		}
		if(atn_rand()%100 < counter_rate)
		{
			int delay = 0;
			int skillid = TK_COUNTER;
			int skilllv;
			if(damage+damage2 < status_get_hp(&tsd->bl)) {
				if((skilllv = pc_checkskill(tsd, skillid)) > 0) {
					delay = 1000 - 4 * status_get_agi(&tsd->bl) - 2 *  status_get_dex(&tsd->bl);
					delay += 300 * battle_config.combo_delay_rate /100; 
					//�R���{���͎��Ԃ̍Œ�ۏ�ǉ�
					if(delay < battle_config.combo_delay_lower_limits)
						delay = battle_config.combo_delay_lower_limits;
				}
				status_change_start(&tsd->bl,SC_TKCOMBO,skillid,skilllv,src->id,0,delay,0);
				tsd->attackabletime = tsd->canmove_tick = gettick() + delay;
				clif_combo_delay(&tsd->bl,delay);
				clif_skill_nodamage(&tsd->bl,&tsd->bl,skillid-1,pc_checkskill(tsd,skillid-1),1);
			}
		}
	}
	
	if(tsd){
		int cardfix=100,i;
		cardfix=cardfix*(100-tsd->subele[s_ele])/100;	// �� ���ɂ��_���[�W�ϐ�
		cardfix=cardfix*(100-tsd->subrace[s_race])/100;	// �푰�ɂ��_���[�W�ϐ�
		cardfix=cardfix*(100-tsd->subsize[s_size])/100;	// �T�C�Y�ɂ��_���[�W�ϐ�
		cardfix=cardfix*(100-tsd->subgroup[s_group])/100;	// �O���[�v�ɂ��_���[�W�ϐ�
		if(mob_db[md->class].mode & 0x20)
			cardfix=cardfix*(100-tsd->subrace[10])/100;
		else
			cardfix=cardfix*(100-tsd->subrace[11])/100;
		for(i=0;i<tsd->add_def_class_count;i++) {
			if(tsd->add_def_classid[i] == md->class) {
				cardfix=cardfix*(100-tsd->add_def_classrate[i])/100;
				break;
			}
		}
		if(flag&BF_LONG)
			cardfix=cardfix*(100-tsd->long_attack_def_rate)/100;
		if(flag&BF_SHORT)
			cardfix=cardfix*(100-tsd->near_attack_def_rate)/100;
		damage=damage*cardfix/100;
	}
	if(t_sc_data) {
		int cardfix=100;
		if(t_sc_data[SC_DEFENDER].timer != -1 && flag&BF_LONG)
			cardfix=cardfix*(100-t_sc_data[SC_DEFENDER].val2)/100;
		if(cardfix != 100)
			damage=damage*cardfix/100;
	}
	if(t_sc_data && t_sc_data[SC_ASSUMPTIO].timer != -1){ //�A�X���v�e�B�I
		if(map[target->m].flag.pvp || map[target->m].flag.gvg)
			damage=damage*2/3;
		else
			damage=damage/2;
	}

	if(damage < 0) damage = 0;

	// �� ���̓K�p
	if(skill_num != 0 || s_ele != 0 || !battle_config.mob_attack_attr_none)
		damage=battle_attr_fix(damage, s_ele, status_get_element(target) );

//	if(sc_data && sc_data[SC_AURABLADE].timer!=-1)	/* �I�[���u���[�h �K������Ȃ��Ȃ��� */
//		damage += sc_data[SC_AURABLADE].val1 * 20;
	if(skill_num==PA_PRESSURE) /* �v���b�V���[ �K��? */
		damage = 500+300*skill_lv;

	// �C���x�i���C��
	if(skill_num==TF_POISON){
		damage = battle_attr_fix(damage + 15*skill_lv, s_ele, status_get_element(target) );
	}
	if(skill_num==MC_CARTREVOLUTION){
		damage = battle_attr_fix(damage, 0, status_get_element(target) );
	}

	// ���S����̔���
	if(skill_num == 0 && skill_lv >= 0 && tsd!=NULL && atn_rand()%1000 < status_get_flee2(target) ){
		damage=0;
		type=0x0b;
		dmg_lv = ATK_LUCKY;
	}

	if(battle_config.enemy_perfect_flee) {
		if(skill_num == 0 && skill_lv >= 0 && tmd!=NULL && atn_rand()%1000 < status_get_flee2(target) ){
			damage=0;
			type=0x0b;
			dmg_lv = ATK_LUCKY;
		}
	}

	if(t_mode&0x40 && damage > 0)
		damage = 1;

	if( tsd && tsd->special_state.no_weapon_damage)
		damage = 0;

	if(skill_num != CR_GRANDCROSS||skill_num !=NPC_DARKGRANDCROSS)
		damage=battle_calc_damage(src,target,damage,div_,skill_num,skill_lv,flag);

	//�J�E�v�I������
	if(damage>0 && t_sc_data && t_sc_data[SC_KAUPE].timer!=-1)
		status_change_end(target,SC_KAUPE,-1);
		
	wd.damage=damage;
	wd.damage2=0;
	wd.type=type;
	wd.div_=div_;
	wd.amotion=status_get_amotion(src);
	if(skill_num == KN_AUTOCOUNTER)
		wd.amotion >>= 1;
	wd.dmotion=status_get_dmotion(target);
	wd.blewcount=blewcount;
	wd.flag=flag;
	wd.dmg_lv=dmg_lv;
	return wd;
}
/*
 * =========================================================================
 * PC�̕���ɂ��U��
 *-------------------------------------------------------------------------
 */
static struct Damage battle_calc_pc_weapon_attack(
	struct block_list *src,struct block_list *target,int skill_num,int skill_lv,int wflag)
{
	struct map_session_data *sd=(struct map_session_data *)src,*tsd=NULL;
	struct mob_data *tmd=NULL;
	int hitrate,flee,cri = 0,atkmin,atkmax;
	int str,dex,luk,target_count = 1;
	int no_cardfix = 0;
	int def1 = status_get_def(target);
	int def2 = status_get_def2(target);
	int t_vit = status_get_vit(target);
	int s_int=status_get_int(src);
	static struct Damage wd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	int damage,damage2,damage3=0,damage4=0,damage_ot=0,damage_ot2=0,type,div_,blewcount=skill_get_blewcount(skill_num,skill_lv);
	int flag,skill,dmg_lv = 0;
	int t_mode=0,t_race=0,t_size=1,t_group = 0,s_race=7,s_ele=0,s_size=1,s_group = 0;
	struct status_change *sc_data,*t_sc_data=0;
	short *sc_count;
	short *option, *opt1, *opt2;
	int atkmax_=0, atkmin_=0, s_ele_;	//�񓁗��p
	int watk,watk_,cardfix,t_ele;
	int da=0,i,t_class,ac_flag = 0;
	int idef_flag=0,idef_flag_=0;
	int tk_power_damage=0,tk_power_damage2=0;//TK_POWER�p

	//return�O�̏���������̂ŏ��o�͕��̂ݕύX
	if( src == NULL || target == NULL || sd == NULL ){
		nullpo_info(NLP_MARK);
		memset(&wd,0,sizeof(wd));
		return wd;
	}

	// �A�^�b�J�[
	s_race=status_get_race(src); //�푰
	s_ele=status_get_attack_element(src); //����
	s_ele_=status_get_attack_element2(src); //���葮��
	s_size=status_get_size( src ); //�Ώۂ̃T�C�Y
	s_group = status_get_group(src);
	sc_data=status_get_sc_data(src); //�X�e�[�^�X�ُ�
	sc_count=status_get_sc_count(src); //�X�e�[�^�X�ُ�̐�
	option=status_get_option(src); //��Ƃ��y�R�Ƃ��J�[�g�Ƃ�
	opt1=status_get_opt1(src); //�Ή��A�����A�X�^���A�����A�È�
	opt2=status_get_opt2(src); //�ŁA�􂢁A���فA�ÈŁH

	if(skill_num != CR_GRANDCROSS||skill_num !=NPC_DARKGRANDCROSS) //�O�����h�N���X�łȂ��Ȃ�
		sd->state.attack_type = BF_WEAPON; //�U���^�C�v�͕���U��

	// �^�[�Q�b�g
	if(target->type==BL_PC) //�Ώۂ�PC�Ȃ�
		tsd=(struct map_session_data *)target; //tsd�ɑ��(tmd��NULL)
	else if(target->type==BL_MOB) //�Ώۂ�Mob�Ȃ�
		tmd=(struct mob_data *)target; //tmd�ɑ��(tsd��NULL)
	t_race=status_get_race( target ); //�Ώۂ̎푰
	t_ele=status_get_elem_type(target); //�Ώۂ̑���
	t_size=status_get_size( target ); //�Ώۂ̃T�C�Y
	t_mode=status_get_mode( target ); //�Ώۂ�Mode
	t_sc_data=status_get_sc_data( target ); //�Ώۂ̃X�e�[�^�X�ُ�
	t_group = status_get_group( target );

//�I�[�g�J�E���^�[������������
	if((skill_num == 0 || (target->type == BL_PC && battle_config.pc_auto_counter_type&2) ||
		(target->type == BL_MOB && battle_config.monster_auto_counter_type&2)) && skill_lv >= 0) {
		if((skill_num != CR_GRANDCROSS||skill_num !=NPC_DARKGRANDCROSS) && t_sc_data && t_sc_data[SC_AUTOCOUNTER].timer != -1) { //�O�����h�N���X�łȂ��A�Ώۂ��I�[�g�J�E���^�[��Ԃ̏ꍇ
			int dir = map_calc_dir(src,target->x,target->y),t_dir = status_get_dir(target);
			int dist = distance(src->x,src->y,target->x,target->y);
			if(dist <= 0 || map_check_dir(dir,t_dir) ) { //�ΏۂƂ̋�����0�ȉ��A�܂��͑Ώۂ̐��ʁH
				memset(&wd,0,sizeof(wd));
				t_sc_data[SC_AUTOCOUNTER].val3 = 0;
				t_sc_data[SC_AUTOCOUNTER].val4 = 1;
				if(sc_data && sc_data[SC_AUTOCOUNTER].timer == -1) { //�������I�[�g�J�E���^�[���
					int range = status_get_range(target);
					if((target->type == BL_PC && ((struct map_session_data *)target)->status.weapon != 11 && dist <= range+1) || //�Ώۂ�PC�ŕ��킪�|��łȂ��˒���
						(target->type == BL_MOB && range <= 3 && dist <= range+1) ) //�܂��͑Ώۂ�Mob�Ŏ˒���3�ȉ��Ŏ˒���
						t_sc_data[SC_AUTOCOUNTER].val3 = src->id;
				}
				return wd; //�_���[�W�\���̂�Ԃ��ďI��
			}
			else ac_flag = 1;
		}
	}
//�I�[�g�J�E���^�[���������܂�

	flag=BF_SHORT|BF_WEAPON|BF_NORMAL;	// �U���̎�ނ̐ݒ�

	// ��𗦌v�Z�A��𔻒�͌��
	flee = status_get_flee(target);
	if(battle_config.agi_penaly_type > 0 || battle_config.vit_penaly_type > 0) //AGI�AVIT�y�i���e�B�ݒ肪�L��
		target_count += battle_counttargeted(target,src,battle_config.agi_penaly_count_lv);	//�Ώۂ̐����Z�o
	if(battle_config.agi_penaly_type > 0) {
		if(target_count >= battle_config.agi_penaly_count) { //�y�i���e�B�ݒ���Ώۂ�����
			if(battle_config.agi_penaly_type == 1) //��𗦂�agi_penaly_num%������
				flee = (flee * (100 - (target_count - (battle_config.agi_penaly_count - 1))*battle_config.agi_penaly_num))/100;
			else if(battle_config.agi_penaly_type == 2) //��𗦂�agi_penaly_num������
				flee -= (target_count - (battle_config.agi_penaly_count - 1))*battle_config.agi_penaly_num;
			if(flee < 1) flee = 1; //��𗦂͍Œ�ł�1
		}
	}
	//PvPMAP�Ō������ʂ�����邩�H
	if(((battle_config.gvg_flee_penaly ==1)||(battle_config.gvg_flee_penaly == 3))&&(map[target->m].flag.gvg))
		flee = flee*(200 - (battle_config.gvg_flee_rate))/100;//���ۂ�GvG��Flee������
	if(((battle_config.gvg_flee_penaly ==2)||(battle_config.gvg_flee_penaly == 3))&&(map[target->m].flag.pvp))
		flee = flee*(200 - (battle_config.gvg_flee_rate))/100;//���ۂ�PvP��Flee������
	hitrate=status_get_hit(src) - flee + 80; //�������v�Z

	type=0;	// normal
	div_ = 1; // single attack

	str=status_get_str(src); //STR
	dex=status_get_dex(src); //DEX
	luk=status_get_luk(src); //LUK
	watk = status_get_atk(src); //ATK
	watk_ = status_get_atk_(src); //ATK����

	if(skill_num==HW_MAGICCRASHER){			/* �}�W�b�N�N���b�V���[��MATK�ŉ��� */
		damage = damage2 = status_get_matk1(src); //damega,damega2���o��Abase_atk�̎擾
	}else{
		damage = damage2 = status_get_baseatk(&sd->bl); //damega,damega2���o��Abase_atk�̎擾
	}
	atkmin = atkmin_ = dex; //�Œ�ATK��DEX�ŏ������H
	sd->state.arrow_atk = 0; //arrow_atk������
	if(sd->equip_index[9] >= 0 && sd->inventory_data[sd->equip_index[9]])
		atkmin = atkmin*(80 + sd->inventory_data[sd->equip_index[9]]->wlv*20)/100;
	if(sd->equip_index[8] >= 0 && sd->inventory_data[sd->equip_index[8]])
		atkmin_ = atkmin_*(80 + sd->inventory_data[sd->equip_index[8]]->wlv*20)/100;
	if(sd->status.weapon == 11) { //���킪�|��̏ꍇ
		atkmin = watk * ((atkmin<watk)? atkmin:watk)/100; //�|�p�Œ�ATK�v�Z
		flag=(flag&~BF_RANGEMASK)|BF_LONG; //�������U���t���O��L��
		if(sd->arrow_ele > 0) //������Ȃ瑮�����̑����ɕύX
			s_ele = sd->arrow_ele;
		sd->state.arrow_atk = 1; //arrow_atk�L����
	}

		// �T�C�Y�C��
		// �y�R�R�悵�Ă��āA���ōU�������ꍇ�͒��^�̃T�C�Y�C����100�ɂ���
		// �E�F�|���p�[�t�F�N�V����,�h���C�NC
	if((pc_isriding(sd) && (sd->status.weapon==4 || sd->status.weapon==5) && t_size==1) || skill_num == MO_EXTREMITYFIST) {	//�y�R�R�悵�Ă��āA���Œ��^���U��
		atkmax = watk;
		atkmax_ = watk_;
	} else {
		atkmax = (watk * sd->atkmods[ t_size ]) / 100;
		atkmin = (atkmin * sd->atkmods[ t_size ]) / 100;
		atkmax_ = (watk_ * sd->atkmods_[ t_size ]) / 100;
		atkmin_ = (atkmin_ * sd->atkmods[ t_size ]) / 100;
	}
	if( (sc_data != NULL && sc_data[SC_WEAPONPERFECTION].timer!=-1) || (sd->special_state.no_sizefix)) {	// �E�F�|���p�[�t�F�N�V���� || �h���C�N�J�[�h
		atkmax = watk;
		atkmax_ = watk_;
	}

	if(atkmin > atkmax && !(sd->state.arrow_atk)) atkmin = atkmax;	//�|�͍ŒႪ����ꍇ����
	if(atkmin_ > atkmax_) atkmin_ = atkmax_;

	if(sc_data != NULL && sc_data[SC_MAXIMIZEPOWER].timer!=-1 ){	// �}�L�V�}�C�Y�p���[
		atkmin=atkmax;
		atkmin_=atkmax_;
	}

	//���z�ƌ��Ɛ��̓{��
	if(sd && (tsd!=NULL || tmd!=NULL))
	{
		int atk_rate = 0;
		int tclass = 0;
		if(tsd != NULL)//�Ώۂ��l
		{
			struct pc_base_job s_class;
			s_class = pc_calc_base_job(tsd->status.class);
			tclass = s_class.job;
		}else if(tmd != NULL)//�Ώۂ��G
			tclass = tmd->class;
		
		if(tclass == sd->hate_mob[0] && pc_checkskill(sd,SG_SUN_ANGER)>0)//���z�̓{��
			atk_rate = (sd->status.base_level + sd->status.dex + sd->status.luk)/(12-3*pc_checkskill(sd,SG_SUN_ANGER));
		else if(tclass == sd->hate_mob[1] && pc_checkskill(sd,SG_MOON_ANGER)>0)//���̓{��
			atk_rate = (sd->status.base_level + sd->status.dex + sd->status.luk)/(12-3*pc_checkskill(sd,SG_MOON_ANGER));
		else if(tclass == sd->hate_mob[2] && pc_checkskill(sd,SG_STAR_ANGER)>0)//���̓{��
			atk_rate = (sd->status.base_level + sd->status.dex + sd->status.luk + sd->status.str)/(12-3*pc_checkskill(sd,SG_STAR_ANGER));
		
		if(atk_rate > 0)
		{
			atkmin  += atkmin  * atk_rate / 100;
			atkmax  += atkmax  * atk_rate / 100;
			atkmin_ += atkmin_ * atk_rate / 100;
			atkmax_ += atkmax_ * atk_rate / 100;
		}
	}
	
	//�_�u���A�^�b�N����
	if(sd->weapontype1 == 0x01) {
		if(skill_num == 0 && skill_lv >= 0 && (skill = pc_checkskill(sd,TF_DOUBLE)) > 0)
		{
			if(sc_data !=NULL && sc_data[SC_ROGUE].timer!=-1)
				da = (atn_rand()%100 < (skill*10)) ? 1:0;
			else
				da = (atn_rand()%100 < (skill*5)) ? 1:0;
		}
	}
	
	//�O�i��
	if(skill_num == 0 && skill_lv >= 0 && (skill = pc_checkskill(sd,MO_TRIPLEATTACK)) > 0 && sd->status.weapon <= 16 && !sd->state.arrow_atk) {
		if(sd->sc_data[SC_TRIPLEATTACK_RATE_UP].timer!=-1)
		{
			int rate_up[3] = {20,50,100};
			da = (atn_rand()%100 < (30 - skill + rate_up[sd->sc_data[SC_TRIPLEATTACK_RATE_UP].val1 - 1])) ? 2:0;
			status_change_end(&sd->bl,SC_TRIPLEATTACK_RATE_UP,-1);
		}else
			da = (atn_rand()%100 < (30 - skill)) ? 2:0;
	}

	//����
	if(skill_num == 0 && skill_lv >= 0 && da == 0 && sc_data && sc_data[SC_READYSTORM].timer!=-1 && pc_checkskill(sd,TK_STORMKICK) > 0 && atn_rand()%100 < 15) {
			da = 3;
	}else if(skill_num == 0 && skill_lv >= 0 && da == 0 && sc_data && sc_data[SC_READYDOWN].timer!=-1 && pc_checkskill(sd,TK_DOWNKICK) > 0 && atn_rand()%100 < 15) {
			da = 4;
	}else if(skill_num == 0 && skill_lv >= 0 && da == 0 && sc_data && sc_data[SC_READYTURN].timer!=-1 &&  pc_checkskill(sd,TK_TURNKICK) > 0 && atn_rand()%100 < 15) {
			da = 5;
	}
	
	//�T�C�h���C���_�[��
	if(sd->double_rate > 0 && da == 0 && skill_num == 0 && skill_lv >= 0)
	{
		if((skill = pc_checkskill(sd,TF_DOUBLE)) > 0)
		{
			if( (skill*5) > sd->double_rate )
			{
				if(sc_data !=NULL && sc_data[SC_ROGUE].timer!=-1)
					da = (atn_rand()%100 < (skill*10)) ? 1:0;
				else
					da = (atn_rand()%100 < (skill*5)) ? 1:0;
			}else{
				if(sc_data !=NULL && sc_data[SC_ROGUE].timer!=-1)
					da = (atn_rand()%100 < sd->double_rate*2) ? 1:0;
				else
					da = (atn_rand()%100 < sd->double_rate) ? 1:0;
			}
		}else{
			
			if(sc_data !=NULL && sc_data[SC_ROGUE].timer!=-1)
				da = (atn_rand()%100 < sd->double_rate*2) ? 1:0;
			else
				da = (atn_rand()%100 < sd->double_rate) ? 1:0;
		}
	}

	
 	// �ߏ萸�B�{�[�i�X
	if(sd->overrefine>0 )
		damage+=(atn_rand()%sd->overrefine)+1;
	if(sd->overrefine_>0 )
		damage2+=(atn_rand()%sd->overrefine_)+1;
	
	if(da == 0){ //�_�u���A�^�b�N���������Ă��Ȃ�
		// �N���e�B�J���v�Z
		cri = status_get_critical(src) + sd->critical_race[t_race];

		if(sd->state.arrow_atk)
			cri += sd->arrow_cri;
		if(sd->status.weapon == 16)
				// �J�^�[���̏ꍇ�A�N���e�B�J����{��
			cri <<=1;
		cri -= status_get_luk(target) * 3;
		if(t_sc_data != NULL && t_sc_data[SC_SLEEP].timer!=-1 )	// �������̓N���e�B�J�����{��
			cri <<=1;
		if(ac_flag) cri = 1000;

		if(skill_num == KN_AUTOCOUNTER) {
			if(!(battle_config.pc_auto_counter_type&1))
				cri = 1000;
			else
				cri <<= 1;
		}
		
		if(skill_num==SN_SHARPSHOOTING)
			cri += 200;
	}

	if(tsd && tsd->critical_def)
		cri = cri * (100-tsd->critical_def) / 100;

	if(da == 0 && (skill_num==0 || skill_num == KN_AUTOCOUNTER || skill_num == SN_SHARPSHOOTING) && skill_lv >= 0 && //�_�u���A�^�b�N���������Ă��Ȃ�
		(atn_rand() % 1000) < cri) 	// ����i�X�L���̏ꍇ�͖����j
	{
		/* �N���e�B�J���U�� */
		damage += atkmax;
		damage2 += atkmax_;
		if(sd->atk_rate != 100 || sd->weapon_atk_rate != 0) {
			damage = (damage * (sd->atk_rate + sd->weapon_atk_rate[sd->status.weapon]))/100;
			damage2 = (damage2 * (sd->atk_rate + sd->weapon_atk_rate[sd->status.weapon]))/100;
			
			//�N���e�B�J�����_���[�W����
			damage  += damage *sd->critical_damage/100;
			damage2 += damage2*sd->critical_damage/100;
		}
		if(sd->state.arrow_atk)
			damage += sd->arrow_atk;
		type = 0x0a;

		//�t�@�C�e�B���O�̌v�Z�@���̈ʒu�H
		if(sd && pc_checkskill(sd,TK_POWER)>0 && sd->status.party_id >0)
		{
			int tk_power_lv = pc_checkskill(sd,TK_POWER);
			int member_num   = party_check_same_map_member_count(sd);
		
			if(member_num > 0)
			{
				tk_power_damage = damage*member_num*2*tk_power_lv/100;
				tk_power_damage2 = damage2*member_num*2*tk_power_lv/100;
			}
		}
		
/*		if(def1 < 1000000) {
			if(sd->def_ratio_atk_ele & (1<<t_ele) || sd->def_ratio_atk_race & (1<<t_race)) {
				damage = (damage * (def1 + def2))/100;
				idef_flag = 1;
			}
			if(sd->def_ratio_atk_ele_ & (1<<t_ele) || sd->def_ratio_atk_race_ & (1<<t_race)) {
				damage2 = (damage2 * (def1 + def2))/100;
				idef_flag_ = 1;
			}
			if(t_mode & 0x20) {
				if(!idef_flag && sd->def_ratio_atk_race & (1<<10)) {
					damage = (damage * (def1 + def2))/100;
					idef_flag = 1;
				}
				if(!idef_flag_ && sd->def_ratio_atk_race_ & (1<<10)) {
					damage2 = (damage2 * (def1 + def2))/100;
					idef_flag_ = 1;
				}
			}
			else {
				if(!idef_flag && sd->def_ratio_atk_race & (1<<11)) {
					damage = (damage * (def1 + def2))/100;
					idef_flag = 1;
				}
				if(!idef_flag_ && sd->def_ratio_atk_race_ & (1<<11)) {
					damage2 = (damage2 * (def1 + def2))/100;
					idef_flag_ = 1;
				}
			}
		}*/
	} else {
		/* �ʏ�U��/�X�L���U�� */
		int vitbonusmax;

		if(atkmax > atkmin)
			damage += atkmin + atn_rand() % (atkmax-atkmin + 1);
		else
			damage += atkmin ;
		if(atkmax_ > atkmin_)
			damage2 += atkmin_ + atn_rand() % (atkmax_-atkmin_ + 1);
		else
			damage2 += atkmin_ ;
		if(sd->atk_rate != 100 || sd->weapon_atk_rate != 0) {
			damage = (damage * (sd->atk_rate + sd->weapon_atk_rate[sd->status.weapon]))/100;
			damage2 = (damage2 * (sd->atk_rate + sd->weapon_atk_rate[sd->status.weapon]))/100;
		}
	
		//�t�@�C�e�B���O�̌v�Z�@���̈ʒu�H
		if(sd && pc_checkskill(sd,TK_POWER)>0 && sd->status.party_id >0)
		{
			int tk_power_lv = pc_checkskill(sd,TK_POWER);
			int member_num   = party_check_same_map_member_count(sd);
		
			if(member_num > 0)
			{
				tk_power_damage = damage*member_num*2*tk_power_lv/100;
				tk_power_damage2 = damage2*member_num*2*tk_power_lv/100;
			}
		}
		
		if(sd->state.arrow_atk) {
			if(sd->arrow_atk > 0)
				damage += atn_rand()%(sd->arrow_atk+1);
			hitrate += sd->arrow_hit;
		}

		if(skill_num != MO_INVESTIGATE && def1 < 1000000) {
			if(sd->def_ratio_atk_ele & (1<<t_ele) || sd->def_ratio_atk_race & (1<<t_race)) {
				damage = (damage * (def1 + def2))/100;
				idef_flag = 1;
			}
			if(sd->def_ratio_atk_ele_ & (1<<t_ele) || sd->def_ratio_atk_race_ & (1<<t_race)) {
				damage2 = (damage2 * (def1 + def2))/100;
				idef_flag_ = 1;
			}
			if(t_mode & 0x20) {
				if(!idef_flag && sd->def_ratio_atk_race & (1<<10)) {
					damage = (damage * (def1 + def2))/100;
					idef_flag = 1;
				}
				if(!idef_flag_ && sd->def_ratio_atk_race_ & (1<<10)) {
					damage2 = (damage2 * (def1 + def2))/100;
					idef_flag_ = 1;
				}
			}
			else {
				if(!idef_flag && sd->def_ratio_atk_race & (1<<11)) {
					damage = (damage * (def1 + def2))/100;
					idef_flag = 1;
				}
				if(!idef_flag_ && sd->def_ratio_atk_race_ & (1<<11)) {
					damage2 = (damage2 * (def1 + def2))/100;
					idef_flag_ = 1;
				}
			}
		}
		// ��Ԉُ풆�̃_���[�W�ǉ��ŃN���e�B�J���Ŗ����ȃX�L��
		if(sc_data){
			if(sc_data[SC_OVERTHRUST].timer!=-1){	// �I�[�o�[�g���X�g
				damage_ot += damage*(5*sc_data[SC_OVERTHRUST].val1)/100;
				damage_ot2 += damage2*(5*sc_data[SC_OVERTHRUST].val1)/100;
			}
			if(sc_data[SC_OVERTHRUSTMAX].timer!=-1){	// �I�[�o�[�g���X�g�}�b�N�X
				damage_ot += damage*(100+20*sc_data[SC_OVERTHRUSTMAX].val1)/100;
				damage_ot2 += damage2*(100+20*sc_data[SC_OVERTHRUSTMAX].val1)/100;
			}
			if(sc_data[SC_TRUESIGHT].timer!=-1){	// �g�D���[�T�C�g
				damage += damage*(2*sc_data[SC_TRUESIGHT].val1)/100;
				damage2 += damage2*(2*sc_data[SC_TRUESIGHT].val1)/100;
			}
			if(sc_data[SC_BERSERK].timer!=-1){	// �o�[�T�[�N
				damage += damage;
				damage2 += damage2;
			}
			if(sc_data && sc_data[SC_AURABLADE].timer!=-1) {	//�I�[���u���[�h������
				damage += sc_data[SC_AURABLADE].val1 * 20;
				damage2 += sc_data[SC_AURABLADE].val1 * 20;
			}
		}

		// �X�L���C���P�i�U���͔{���n�j
		// �o�b�V��,�}�O�i���u���C�N,
		// �{�[�����O�o�b�V��,�X�s�A�u�[������,�u�����f�B�b�V���X�s�A
		// �X�s�A�X�^�b�u,���}�[�i�C�g,�J�[�g���{�����[�V����
		// �_�u���X�g���C�t�B���O,�A���[�V�����[,�`���[�W�A���[,
		// �\�j�b�N�u���[
		if(skill_num>0){
			int i;
			if( (i=skill_get_pl(skill_num))>0 )
				s_ele=s_ele_=i;

			flag=(flag&~BF_SKILLMASK)|BF_SKILL;
			switch( skill_num ){
			case SM_BASH:		// �o�b�V��
				damage = damage*(100+ 30*skill_lv)/100;
				damage2 = damage2*(100+ 30*skill_lv)/100;
				hitrate = (hitrate*(100+5*skill_lv))/100;
				break;
			case SM_MAGNUM:		// �}�O�i���u���C�N
				damage = damage*(20*skill_lv + 100)/100;
				damage2 = damage2*(20*skill_lv + 100)/100;
				hitrate = hitrate*(10* skill_lv + 100)/100;
				break;
			case MC_MAMMONITE:	// ���}�[�i�C�g
				damage = damage*(100+ 50*skill_lv)/100;
				damage2 = damage2*(100+ 50*skill_lv)/100;
				break;
			case AC_DOUBLE:	// �_�u���X�g���C�t�B���O
				if(!sd->state.arrow_atk && sd->arrow_atk > 0) {
					int arr = atn_rand()%(sd->arrow_atk+1);
					damage += arr;
					damage2 += arr;
				}
				damage = damage*(180+ 20*skill_lv)/100;
				damage2 = damage2*(180+ 20*skill_lv)/100;
				div_=2;
				if(sd->arrow_ele > 0) {
					s_ele = sd->arrow_ele;
					s_ele_ = sd->arrow_ele;
				}
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				sd->state.arrow_atk = 1;
				break;
			case AC_SHOWER:	// �A���[�V�����[
				if(!sd->state.arrow_atk && sd->arrow_atk > 0) {
					int arr = atn_rand()%(sd->arrow_atk+1);
					damage += arr;
					damage2 += arr;
				}
				damage = damage*(75 + 5*skill_lv)/100;
				damage2 = damage2*(75 + 5*skill_lv)/100;
				if(sd->arrow_ele > 0) {
					s_ele = sd->arrow_ele;
					s_ele_ = sd->arrow_ele;
				}
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				sd->state.arrow_atk = 1;
				break;
			case AC_CHARGEARROW:	// �`���[�W�A���[
				if(!sd->state.arrow_atk && sd->arrow_atk > 0) {
					int arr = atn_rand()%(sd->arrow_atk+1);
					damage += arr;
					damage2 += arr;
				}
				damage = damage*150/100;
				damage2 = damage2*150/100;
				if(sd->arrow_ele > 0) {
					s_ele = sd->arrow_ele;
					s_ele_ = sd->arrow_ele;
				}
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				sd->state.arrow_atk = 1;
				break;
			case KN_PIERCE:	// �s�A�[�X
				damage = damage*(100+ 10*skill_lv)/100;
				damage2 = damage2*(100+ 10*skill_lv)/100;
				hitrate=hitrate*(100+5*skill_lv)/100;
				div_=t_size+1;
				damage*=div_;
				damage2*=div_;
				break;
			case KN_SPEARSTAB:	// �X�s�A�X�^�u
				damage = damage*(100+ 15*skill_lv)/100;
				damage2 = damage2*(100+ 15*skill_lv)/100;
				blewcount=0;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case KN_SPEARBOOMERANG:	// �X�s�A�u�[������
				damage = damage*(100+ 50*skill_lv)/100;
				damage2 = damage2*(100+ 50*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case KN_BRANDISHSPEAR: // �u�����f�B�b�V���X�s�A
				damage = damage*(100+ 20*skill_lv)/100;
				damage2 = damage2*(100+ 20*skill_lv)/100;
				if(skill_lv>3 && wflag==1) damage3+=damage/2;
				if(skill_lv>6 && wflag==1) damage3+=damage/4;
				if(skill_lv>9 && wflag==1) damage3+=damage/8;
				if(skill_lv>6 && wflag==2) damage3+=damage/2;
				if(skill_lv>9 && wflag==2) damage3+=damage/4;
				if(skill_lv>9 && wflag==3) damage3+=damage/2;
				damage +=damage3;
				if(skill_lv>3 && wflag==1) damage4+=damage2/2;
				if(skill_lv>6 && wflag==1) damage4+=damage2/4;
				if(skill_lv>9 && wflag==1) damage4+=damage2/8;
				if(skill_lv>6 && wflag==2) damage4+=damage2/2;
				if(skill_lv>9 && wflag==2) damage4+=damage2/4;
				if(skill_lv>9 && wflag==3) damage4+=damage2/2;
				damage2 +=damage4;
				break;
			case KN_BOWLINGBASH:	// �{�E�����O�o�b�V��
				damage = damage*(100+ 50*skill_lv)/100;
				blewcount=0;
				break;
			case KN_AUTOCOUNTER:
				if(battle_config.pc_auto_counter_type&1)
					hitrate += 20;
				else
					hitrate = 1000000;
				flag=(flag&~BF_SKILLMASK)|BF_NORMAL;
				break;
			case AS_SONICBLOW:	// �\�j�b�N�u���E
				if(sc_data!=NULL && sc_data[SC_ASSASIN].timer!=-1)
				{
					damage = damage*(400+ 50*skill_lv)/100;
					damage2 = damage2*(400+ 50*skill_lv)/100;
				}else{
					damage = damage*(300+ 50*skill_lv)/100;
					damage2 = damage2*(300+ 50*skill_lv)/100;
				}
				div_=8;
				break;
			case AS_GRIMTOOTH:	// �O�����g�D�[�X
				damage = damage*(100+ 20*skill_lv)/100;
				damage2 = damage2*(100+ 20*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case TF_SPRINKLESAND:	// ���܂�
				damage = damage*130/100;
				damage2 = damage2*130/100;
				break;
			case MC_CARTREVOLUTION:	// �J�[�g���{�����[�V����
				if(sd->cart_max_weight > 0 && sd->cart_weight > 0) {
					damage = (damage*(150 + pc_checkskill(sd,BS_WEAPONRESEARCH) + (sd->cart_weight*100/sd->cart_max_weight) ) )/100;
					damage2 = (damage2*(150 + pc_checkskill(sd,BS_WEAPONRESEARCH) + (sd->cart_weight*100/sd->cart_max_weight) ) )/100;
				}
				else {
					damage = (damage*150)/100;
					damage2 = (damage2*150)/100;
				}
				break;
			// �ȉ�MOB
			case NPC_COMBOATTACK:	// ���i�U��
				div_=skill_get_num(skill_num,skill_lv);
				damage *= div_;
				damage2 *= div_;
				break;
			case NPC_RANDOMATTACK:	// �����_��ATK�U��
				damage = damage*(50+atn_rand()%150)/100;
				damage2 = damage2*(50+atn_rand()%150)/100;
				break;
			// �����U���i�K���j
			case NPC_WATERATTACK:
			case NPC_GROUNDATTACK:
			case NPC_FIREATTACK:
			case NPC_WINDATTACK:
			case NPC_POISONATTACK:
			case NPC_HOLYATTACK:
			case NPC_DARKNESSATTACK:
			case NPC_TELEKINESISATTACK:
			case NPC_UNDEADATTACK:
				damage = damage*(100+25*skill_lv)/100;
				damage2 = damage2*(100+25*skill_lv)/100;
				break;
			case NPC_GUIDEDATTACK:
				hitrate = 1000000;
				break;
			case NPC_RANGEATTACK:
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case NPC_PIERCINGATT:
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
				break;
			case RG_BACKSTAP:	// �o�b�N�X�^�u
				damage = damage*(300+ 40*skill_lv)/100;
				damage2 = damage2*(300+ 40*skill_lv)/100;
				if(sd->status.weapon == 11) {	// �|�Ȃ甼��
					damage /= 2;
					damage2 /= 2;
				}
				hitrate = 1000000;
				break;
			case RG_RAID:	// �T�v���C�Y�A�^�b�N
				damage = damage*(100+ 40*skill_lv)/100;
				damage2 = damage2*(100+ 40*skill_lv)/100;
				break;
			case RG_INTIMIDATE:	// �C���e�B�~�f�C�g
				damage = damage*(100+ 30*skill_lv)/100;
				damage2 = damage2*(100+ 30*skill_lv)/100;
				break;
			case CR_SHIELDCHARGE:	// �V�[���h�`���[�W
				damage = damage*(100+ 20*skill_lv)/100;
				damage2 = damage2*(100+ 20*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
				s_ele = 0;
				break;
			case CR_SHIELDBOOMERANG:	// �V�[���h�u�[������
				if(sc_data && sc_data[SC_CRUSADER].timer!=-1)
				{	
					damage = damage*(200+ 30*skill_lv)/100;
					damage2 = damage2*(200+ 30*skill_lv)/100;
					hitrate= 1000000;
				}else{
					damage = damage*(100+ 30*skill_lv)/100;
					damage2 = damage2*(100+ 30*skill_lv)/100;
				}
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				s_ele = 0;
				break;
			case CR_HOLYCROSS:	// �z�[���[�N���X
				damage = damage*(100+ 35*skill_lv)/100;
				damage2 = damage2*(100+ 35*skill_lv)/100;
				div_=2;
				break;
			case CR_GRANDCROSS:
			case NPC_DARKGRANDCROSS:
				hitrate= 1000000;
				if (!battle_config.gx_cardfix)
					no_cardfix = 1;
				break;
			case AM_DEMONSTRATION:	// �f�����X�g���[�V����
				hitrate= 1000000;
				damage = damage*(100+ 20*skill_lv)/100;
				damage2 = damage2*(100+ 20*skill_lv)/100;
				no_cardfix = 1;
				break;
			case AM_ACIDTERROR:	// �A�V�b�h�e���[
				hitrate= 1000000;
				damage = damage*(100+ 40*skill_lv)/100;
				damage2 = damage2*(100+ 40*skill_lv)/100;
				s_ele = 0;
				s_ele_ = 0;
				no_cardfix = 1;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case MO_FINGEROFFENSIVE:	//�w�e
				if(battle_config.finger_offensive_type == 0) {
					damage = damage * (100 + 50 * skill_lv) / 100 * sd->spiritball_old;
					damage2 = damage2 * (100 + 50 * skill_lv) / 100 * sd->spiritball_old;
					div_ = sd->spiritball_old;
				}
				else {
					damage = damage * (100 + 50 * skill_lv) / 100;
					damage2 = damage2 * (100 + 50 * skill_lv) / 100;
					div_ = 1;
				}
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case MO_INVESTIGATE:	// �� ��
				if(def1 < 1000000) {
					damage = damage*(100+ 75*skill_lv)/100 * (def1 + def2)/50;
					damage2 = damage2*(100+ 75*skill_lv)/100 * (def1 + def2)/50;
				}
				hitrate = 1000000;
				s_ele = 0;
				s_ele_ = 0;
				break;
			case MO_EXTREMITYFIST:	// ���C���e�P��
				damage = damage * (8 + ((sd->status.sp)/10)) + 250 + (skill_lv * 150);
				damage2 = damage2 * (8 + ((sd->status.sp)/10)) + 250 + (skill_lv * 150);
				sd->status.sp = 0;
				clif_updatestatus(sd,SP_SP);
				hitrate = 1000000;
				s_ele = 0;
				s_ele_ = 0;
				break;
			case MO_CHAINCOMBO:	// �A�ŏ�
				damage = damage*(150+ 50*skill_lv)/100;
				damage2 = damage2*(150+ 50*skill_lv)/100;
				div_=4;
				break;
			case MO_COMBOFINISH:	// �җ���
				damage = damage*(240+ 60*skill_lv)/100;
				damage2 = damage2*(240+ 60*skill_lv)/100;
				//PT�ɂ͓����Ă���
				//�J�E���^�[�A�^�b�N�̊m���㏸
				if(sd && sd->status.party_id>0){
					struct party *pt = party_search(sd->status.party_id);
					if(pt!=NULL)
					{
						int i = 0;
						struct map_session_data* psd = NULL;
						
						for(i = 0;i<MAX_PARTY;i++)
						{
							psd = pt->member[i].sd;
							if(psd && sd->bl.m == psd->bl.m && pc_checkskill(psd,TK_COUNTER)>0)
							{
								status_change_start(&psd->bl,SC_COUNTER_RATE_UP,1,0,0,0,battle_config.tk_counter_rate_up_keeptime,0);
							}
						}
					}
				}
				break;
			case TK_STORMKICK://�����R��
				damage = damage*(160+ 20*skill_lv)/100;
				damage2 = damage2*(160+ 20*skill_lv)/100;
				break;
			case TK_DOWNKICK://���i�R��
				damage = damage*(160+ 20*skill_lv)/100;
				damage2 = damage2*(160+ 20*skill_lv)/100;
				break;
			case TK_TURNKICK://��]�R��
				damage = damage*(190+ 30*skill_lv)/100;
				damage2 = damage2*(190+ 30*skill_lv)/100;
			case TK_COUNTER://�J�E���^�[�R��
				damage = damage*(190+ 30*skill_lv)/100;
				damage2 = damage2*(190+ 30*skill_lv)/100;
				hitrate = 1000000;
				//PT�ɂ͓����Ă���
				//�O�i���̊m���㏸
				if(sd && sd->status.party_id>0){
					int tk_friend_lv = pc_checkskill(sd,SG_FRIEND);
					struct party *pt = party_search(sd->status.party_id);

					if(pt!=NULL && tk_friend_lv>0)
					{
						int i = 0;
						struct map_session_data* psd = NULL;
						
						for(i = 0;i<MAX_PARTY;i++)
						{
							psd = pt->member[i].sd;
							if(psd && sd->bl.m == psd->bl.m && pc_checkskill(psd,MO_TRIPLEATTACK)>0)
							{
								status_change_start(&psd->bl,SC_TRIPLEATTACK_RATE_UP,tk_friend_lv,0,0,0,battle_config.tripleattack_rate_up_keeptime,0);
							}
						}
					}
				}
				break;
			case BA_MUSICALSTRIKE:	// �~���[�W�J���X�g���C�N
				if(!sd->state.arrow_atk && sd->arrow_atk > 0) {
					int arr = atn_rand()%(sd->arrow_atk+1);
					damage += arr;
					damage2 += arr;
				}
				damage = damage*(60+ 40 * skill_lv)/100;
				damage2 = damage2*(60+ 40 * skill_lv)/100;
				if(sd->arrow_ele > 0) {
					s_ele = sd->arrow_ele;
					s_ele_ = sd->arrow_ele;
				}
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				sd->state.arrow_atk = 1;
				break;
			case DC_THROWARROW:	// ���
				damage = damage*(60+ 40 * skill_lv)/100;
				damage2 = damage2*(60+ 40 * skill_lv)/100;
				if(sd->arrow_ele > 0) {
					s_ele = sd->arrow_ele;
					s_ele_ = sd->arrow_ele;
				}
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case CH_TIGERFIST:	// ���Ռ�
				damage = damage*(40+ 100*skill_lv)/100;
				damage2 = damage2*(40+ 100*skill_lv)/100;
				break;
			case CH_CHAINCRUSH:	// �A������
				damage = damage*(400+ 100*skill_lv)/100;
				damage2 = damage2*(400+ 100*skill_lv)/100;
				div_=skill_get_num(skill_num,skill_lv);
				break;
			case CH_PALMSTRIKE:	// �ҌՍd�h�R
				damage = damage*(200+ 100*skill_lv)/100;
				damage2 = damage2*(200+ 100*skill_lv)/100;
				break;
			case LK_SPIRALPIERCE:			/* �X�p�C�����s�A�[�X */
				div_=5;
/*				if(tsd)
					tsd->canmove_tick = gettick() + 1000;
				else if(tmd)
					tmd->canmove_tick = gettick() + 1000;
					*/
				flag=(flag&~BF_RANGEMASK)|BF_LONG; 
				break;
			case LK_HEADCRUSH:				/* �w�b�h�N���b�V�� */
				damage = damage*(100+ 40*skill_lv)/100;
				damage2 = damage2*(100+ 40*skill_lv)/100;
				break;
			case LK_JOINTBEAT:				/* �W���C���g�r�[�g */
				damage = damage*(50+ 10*skill_lv)/100;
				damage2 = damage2*(50+ 10*skill_lv)/100;
				break;
			case ASC_METEORASSAULT:			/* ���e�I�A�T���g */
				damage = damage*(40+ 40*skill_lv)/100;
				damage2 = damage2*(40+ 40*skill_lv)/100;
				no_cardfix = 1;
				break;
			case ASC_BREAKER:				/* �\�E���u���C�J�[ */
				damage = damage * skill_lv;
				damage2 = damage2 * skill_lv;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				no_cardfix = 1;
				break;
			case SN_SHARPSHOOTING:			/* �V���[�v�V���[�e�B���O */
				damage = damage*(200+50*skill_lv)/100;
				damage2 = damage2*(200+50*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case CG_ARROWVULCAN:			/* �A���[�o���J�� */
				damage = damage*(200+100*skill_lv)/100;
				damage2 = damage2*(200+100*skill_lv)/100;
				div_=9;
				if(sd->arrow_ele > 0) {
					s_ele = sd->arrow_ele;
					s_ele_ = sd->arrow_ele;
				}
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case AS_SPLASHER:		/* �x�i���X�v���b�V���[ */
				damage = damage*(500+50*skill_lv+20*pc_checkskill(sd,AS_POISONREACT))/100;
				damage2 = damage2*(500+50*skill_lv+20*pc_checkskill(sd,AS_POISONREACT))/100;
				no_cardfix = 1;
				hitrate = 1000000;
				break;
			case TK_JUMPKICK: //��яR��
				if(sd && sd->sc_data && sd->sc_data[SC_DODGE_DELAY].timer!=-1)
				{
					int gain = sd->status.base_level/10;
					damage = damage*(40+10*(skill_lv+gain))/100;
					damage2 = damage2*(40+10*(skill_lv+gain))/100;
					if(sd->sc_data[SC_DODGE_DELAY].timer!=-1)
						status_change_end(&sd->bl,SC_DODGE_DELAY,-1);
				}else{
					damage = damage*(30+10*skill_lv)/100;
					damage2 = damage2*(30+10*skill_lv)/100;
				}
				break;
			case PA_SHIELDCHAIN:	/* �V�[���h�`�F�C�� */
				damage = damage*(100+ 10*skill_lv)/100;
				damage2 = damage2*(100+ 10*skill_lv)/100;
				hitrate = hitrate*(100+(55-5*skill_lv))/100;
				div_=5;
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
//				damage*=div_;//�Z�o���s���ɕt��
//				damage2*=div_;//���̂Q�s�͂Ƃ肠�����R�����g�A�E�g�B
				s_ele = 0;
				break;
			case WS_CARTTERMINATION:	/* �J�[�g�^�[�~�l�[�V���� */
				if(sd->cart_max_weight > 0 && sd->cart_weight > 0) {
					damage = damage*(sd->cart_weight/(16-skill_lv)/1000);
					damage2 = damage2*(sd->cart_weight/(16-skill_lv)/1000);
				}
				else {
					damage = (damage*100)/100;
					damage2 = (damage2*100)/100;
				}
				break;
			case CR_ACIDDEMONSTRATION:	/* �A�V�b�h�f�����X�g���[�V���� */
				hitrate= 1000000;
				damage = damage*(s_int*skill_lv)/100;
				damage2 = damage2*(s_int*skill_lv)/100;
				if(t_vit>6){
					damage *= t_vit/6;
					damage2 *= t_vit/6;
				}
				if(target->type==BL_PC){
					damage /= 2;
					damage2 /= 2;
				}
				div_=skill_get_num( skill_num,skill_lv );
				s_ele = 0;
				s_ele_ = 0;
				no_cardfix = 1;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case ITM_TOMAHAWK:		/* �g�}�z�[�N���� */
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			}
		}

		damage += damage_ot;	//�I�[�o�[�g���X�g�A�I�[�o�[�g���X�g�}�b�N�X�C��
		damage2 += damage_ot2;
		
		//�t�@�C�e�B���O�̒ǉ��_���[�W
		damage += tk_power_damage;
		damage2 += tk_power_damage2;
		
		if(da == 2) { //�O�i�����������Ă��邩
			type = 0x08;
			div_ = 255;	//�O�i���p�Ɂc
			damage = damage * (100 + 20 * pc_checkskill(sd, MO_TRIPLEATTACK)) / 100;
		}
		if(da == 3) { //�������������Ă��邩
			type = 0x08;
			div_ = 252;	//�����p�Ɂc
		}else if(da == 4) { //�������������Ă��邩
			type = 0x08;
			div_ = 253;	//�����p�Ɂc
		}else if(da == 5) { //�������������Ă��邩
			type = 0x08;
			div_ = 254;	//�����p�Ɂc
		}

		if( skill_num!=NPC_CRITICALSLASH ){
			// �� �ۂ̖h��͂ɂ��_���[�W�̌���
			// �f�B�o�C���v���e�N�V�����i�����ł����̂��ȁH�j
			if ( skill_num != MO_INVESTIGATE && skill_num != MO_EXTREMITYFIST && skill_num != KN_AUTOCOUNTER && skill_num != AM_ACIDTERROR && def1 < 1000000) {	//DEF, VIT����
				int t_def;
				target_count = 1 + battle_counttargeted(target,src,battle_config.vit_penaly_count_lv);
				if(battle_config.vit_penaly_type > 0) {
					if(target_count >= battle_config.vit_penaly_count) {
						if(battle_config.vit_penaly_type == 1) {
							def1 = (def1 * (100 - (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num))/100;
							def2 = (def2 * (100 - (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num))/100;
							t_vit = (t_vit * (100 - (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num))/100;
						}
						else if(battle_config.vit_penaly_type == 2) {
							def1 -= (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num;
							def2 -= (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num;
							t_vit -= (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num;
						}
						if(def1 < 0) def1 = 0;
						if(def2 < 1) def2 = 1;
						if(t_vit < 1) t_vit = 1;
					}
				}
				t_def = def2*8/10;
				vitbonusmax = (t_vit/20)*(t_vit/20)-1;
				if(sd->ignore_def_ele & (1<<t_ele) || sd->ignore_def_race & (1<<t_race))
					idef_flag = 1;
				if(sd->ignore_def_ele_ & (1<<t_ele) || sd->ignore_def_race_ & (1<<t_race))
					idef_flag_ = 1;
				if(t_mode & 0x20) {
					if(sd->ignore_def_race & (1<<10))
						idef_flag = 1;
					if(sd->ignore_def_race_ & (1<<10))
						idef_flag_ = 1;
				}
				else {
					if(sd->ignore_def_race & (1<<11))
						idef_flag = 1;
					if(sd->ignore_def_race_ & (1<<11))
						idef_flag_ = 1;
				}

				//���z�ƌ��Ɛ��̗Z�� DEF����
				if(sd && sd->sc_data[SC_FUSION].timer != -1)
					idef_flag = 1;
					
				if(!idef_flag){
					if(battle_config.player_defense_type) {
						damage = damage - (def1 * battle_config.player_defense_type) - t_def - ((vitbonusmax < 1)?0: atn_rand()%(vitbonusmax+1) );
						damage2 = damage2 - (def1 * battle_config.player_defense_type) - t_def - ((vitbonusmax < 1)?0: atn_rand()%(vitbonusmax+1) );
					}
					else{
						damage = damage * (100 - def1) /100 - t_def - ((vitbonusmax < 1)?0: atn_rand()%(vitbonusmax+1) );
						damage2 = damage2 * (100 - def1) /100 - t_def - ((vitbonusmax < 1)?0: atn_rand()%(vitbonusmax+1) );
					}
				}
			}
		}
	}
	
	// ��Ԉُ풆�̃_���[�W�ǉ��ŃN���e�B�J���ɂ��L���ȃX�L��
	if (sc_data) {
		// �G���`�����g�f�b�h���[�|�C�Y��
		if (!no_cardfix && sc_data[SC_EDP].timer != -1) {
			// �E��݂̂Ɍ��ʂ��̂�B�J�[�h���ʖ����̃X�L���ɂ͏��Ȃ�
			damage += damage * (150 + sc_data[SC_EDP].val1 * 50) / 100;
			no_cardfix = 1;
		}
		// �T�N���t�@�C�X
		if (!skill_num && sc_data[SC_SACRIFICE].timer != -1) {
			int mhp = status_get_max_hp(src);
			int dmg = mhp * 9 / 100;
			pc_heal(sd, -dmg, 0);
			damage = dmg * (90 + sc_data[SC_SACRIFICE].val1 * 10) / 100;
			damage2 = 0;
			hitrate = 1000000;
			s_ele = 0;
			s_ele_ = 0;
			sc_data[SC_SACRIFICE].val2 --;
			if (sc_data[SC_SACRIFICE].val2 == 0)
				status_change_end(src, SC_SACRIFICE,-1);
		}
	}

	// ���B�_���[�W�̒ǉ�
	if( skill_num != MO_INVESTIGATE && skill_num != MO_EXTREMITYFIST) {			//DEF, VIT����
		damage += status_get_atk2(src);
		damage2 += status_get_atk_2(src);
	}
	if(skill_num == CR_SHIELDBOOMERANG) {
		if(sd->equip_index[8] >= 0) {
			int index = sd->equip_index[8];
			if(sd->inventory_data[index] && sd->inventory_data[index]->type == 5) {
				damage += sd->inventory_data[index]->weight/10;
				damage += sd->status.inventory[index].refine * status_getrefinebonus(0,1);
			}
		}
	}
	if(skill_num == LK_SPIRALPIERCE) {			/* �X�p�C�����s�A�[�X */
		if(sd->equip_index[9] >= 0) {	//{((STR/10)^2 �{ ����d�ʁ~�X�L���{���~0.8)  �~  �T�C�Y�␳  �{  ���B}�~�J�[�h�{���~�����{���~5�̖͗l
			int index = sd->equip_index[9];
			if(sd->inventory_data[index] && sd->inventory_data[index]->type == 4) {
				damage = ( ( (str/10)*(str/10) + sd->inventory_data[index]->weight * (skill_lv * 4 + 8 ) / 100 )
							* (5 - t_size) / 4 + status_get_atk2(src) ) * 5;
//				damage = floor(str/10) * floor(str/10);
//				damage += (int)(double)(sd->inventory_data[index]->weight)/10*0.8*(skill_lv*0.5+1);
//				damage *= (1.25 - t_size*0.25);
//				damage += status_get_atk2(src);
//				damage *= 5;
			}
		}
	}
	if(skill_num == PA_SHIELDCHAIN) {			/* �V�[���h�`�F�C�����u�[���������l�� */
		if(sd->equip_index[8] >= 0) {
			int index = sd->equip_index[8];
			if(sd->inventory_data[index] && sd->inventory_data[index]->type == 5) {
				damage += sd->inventory_data[index]->weight/10;
				damage += sd->status.inventory[index].refine * status_getrefinebonus(0,1);
			}
		}
	}

	// 0�����������ꍇ1�ɕ␳
	if(damage<1) damage=1;
	if(damage2<1) damage2=1;

	// �X�L���C���Q�i�C���n�j
	// �C���_���[�W(�E��̂�) �\�j�b�N�u���[���͕ʏ����i1���ɕt��1/8�K��)
	if( skill_num != MO_INVESTIGATE && skill_num != MO_EXTREMITYFIST && (skill_num != CR_GRANDCROSS||skill_num !=NPC_DARKGRANDCROSS)
			&& skill_num != LK_SPIRALPIERCE) {			//�C���_���[�W����
		damage = battle_addmastery(sd,target,damage,0);
		damage2 = battle_addmastery(sd,target,damage2,1);
	}

	if(sd->perfect_hit > 0) {
		if(atn_rand()%100 < sd->perfect_hit)
			hitrate = 1000000;
	}

	// ����C��
	hitrate = (hitrate<5)?5:hitrate;
	if(	hitrate < 1000000 && // �K���U��
		(t_sc_data != NULL && (t_sc_data[SC_SLEEP].timer!=-1 ||	// �����͕K��
		t_sc_data[SC_STAN].timer!=-1 ||		// �X�^���͕K��
		t_sc_data[SC_FREEZE].timer!=-1 || (t_sc_data[SC_STONE].timer!=-1 && t_sc_data[SC_STONE].val2==0) ) ) )	// �����͕K��
		hitrate = 1000000;
	if(type == 0 && atn_rand()%100 >= hitrate) {
		damage = damage2 = 0;
		dmg_lv = ATK_FLEE;
	}else if(type == 0 && hitrate != 1000000 && t_sc_data && t_sc_data[SC_KAUPE].timer !=-1 && atn_rand()%100 < t_sc_data[SC_KAUPE].val1*33)//�J�E�v
	{
		damage = damage2 = 0;
		dmg_lv = ATK_FLEE;
	}else if(tsd && t_sc_data && (flag&BF_LONG || t_sc_data[SC_SPURT].timer!=-1)&& t_sc_data[SC_DODGE].timer!=-1 && atn_rand()%100 < 20)//���@
	{
		int slv = pc_checkskill(tsd,TK_DODGE);
		damage = damage2 = 0;
		dmg_lv = ATK_FLEE;
		clif_skill_nodamage(&tsd->bl,&tsd->bl,TK_DODGE,slv,1);
		status_change_start(&tsd->bl,SC_DODGE_DELAY,slv,0,0,0,skill_get_time(TK_DODGE,slv),0);
	}
	else{
		dmg_lv = ATK_DEF;
	}
	//�J�E���^�[�L�b�N�̐�
	if(dmg_lv == ATK_DEF && tsd && t_sc_data && t_sc_data[SC_READYCOUNTER].timer!=-1)
	{
		int counter_rate = 20;
		if(t_sc_data && t_sc_data[SC_COUNTER_RATE_UP].timer!=-1)
		{
			int rate_up[] = {0,20,50,100};
			counter_rate += rate_up[pc_checkskill(tsd,SG_FRIEND)];
			status_change_end(&tsd->bl,SC_COUNTER_RATE_UP,-1);
		}
		if(atn_rand()%100 < counter_rate)
		{
			int delay = 0;
			int skillid = TK_COUNTER;
			int skilllv;
			if(damage+damage2 < status_get_hp(&tsd->bl)) {
				if((skilllv = pc_checkskill(tsd, skillid)) > 0) {
					delay = 1000 - 4 * status_get_agi(&tsd->bl) - 2 *  status_get_dex(&tsd->bl);
					delay += 300 * battle_config.combo_delay_rate /100; 
					//�R���{���͎��Ԃ̍Œ�ۏ�ǉ�
					if(delay < battle_config.combo_delay_lower_limits)
						delay = battle_config.combo_delay_lower_limits;
				}
				status_change_start(&tsd->bl,SC_TKCOMBO,skillid,skilllv,src->id,0,delay,0);
				tsd->attackabletime = tsd->canmove_tick = gettick() + delay;
				clif_combo_delay(&tsd->bl,delay);
				clif_skill_nodamage(&tsd->bl,&tsd->bl,skillid-1,pc_checkskill(tsd,skillid-1),1);
			}
		}
	}
	
	// �X�L���C���R�i���팤���j
	if( (skill=pc_checkskill(sd,BS_WEAPONRESEARCH)) > 0) {
		damage+= skill*2;
		damage2+= skill*2;
	}
//�X�L���ɂ��_���[�W�␳�����܂�

//�J�[�h�ɂ��_���[�W�ǉ�������������
	cardfix=100;
	if(!sd->state.arrow_atk) { //�|��ȊO
		if(!battle_config.left_cardfix_to_right) { //����J�[�h�␳�ݒ薳��
			cardfix=cardfix*(100+sd->addrace[t_race])/100;	// �푰�ɂ��_���[�W�C��
			cardfix=cardfix*(100+sd->addele[t_ele])/100;	// �����ɂ��_���[�W�C��
			cardfix=cardfix*(100+sd->addsize[t_size])/100;	// �T�C�Y�ɂ��_���[�W�C��
			cardfix=cardfix*(100+sd->addgroup[t_group])/100;	// �O���[�v�ɂ��_���[�W�C��
		}
		else {
			cardfix=cardfix*(100+sd->addrace[t_race]+sd->addrace_[t_race])/100;	// �푰�ɂ��_���[�W�C��(����ɂ��ǉ�����)
			cardfix=cardfix*(100+sd->addele[t_ele]+sd->addele_[t_ele])/100;	// �����ɂ��_���[�W�C��(����ɂ��ǉ�����)
			cardfix=cardfix*(100+sd->addsize[t_size]+sd->addsize_[t_size])/100;	// �T�C�Y�ɂ��_���[�W�C��(����ɂ��ǉ�����)
			cardfix=cardfix*(100+sd->addgroup[t_group]+sd->addgroup_[t_group])/100;	// �O���[�v�ɂ��_���[�W�C��(����ɂ��ǉ�����)
		}
	}
	else { //�|��
		cardfix=cardfix*(100+sd->addrace[t_race]+sd->arrow_addrace[t_race])/100;	// �푰�ɂ��_���[�W�C��(�|��ɂ��ǉ�����)
		cardfix=cardfix*(100+sd->addele[t_ele]+sd->arrow_addele[t_ele])/100;	// �����ɂ��_���[�W�C��(�|��ɂ��ǉ�����)
		cardfix=cardfix*(100+sd->addsize[t_size]+sd->arrow_addsize[t_size])/100;	// �T�C�Y�ɂ��_���[�W�C��(�|��ɂ��ǉ�����)
		cardfix=cardfix*(100+sd->addgroup[t_size]+sd->arrow_addgroup[t_size])/100;	// �O���[�v�ɂ��_���[�W�C��(�|��ɂ��ǉ�����)
	}
	if(t_mode & 0x20) { //�{�X
		if(!sd->state.arrow_atk) { //�|��U���ȊO�Ȃ�
			if(!battle_config.left_cardfix_to_right) //����J�[�h�␳�ݒ薳��
				cardfix=cardfix*(100+sd->addrace[10])/100; //�{�X�����X�^�[�ɒǉ��_���[�W
			else //����J�[�h�␳�ݒ肠��
				cardfix=cardfix*(100+sd->addrace[10]+sd->addrace_[10])/100; //�{�X�����X�^�[�ɒǉ��_���[�W(����ɂ��ǉ�����)
		}
		else //�|��U��
			cardfix=cardfix*(100+sd->addrace[10]+sd->arrow_addrace[10])/100; //�{�X�����X�^�[�ɒǉ��_���[�W(�|��ɂ��ǉ�����)
	}
	else { //�{�X����Ȃ�
		if(!sd->state.arrow_atk) { //�|��U���ȊO
			if(!battle_config.left_cardfix_to_right) //����J�[�h�␳�ݒ薳��
				cardfix=cardfix*(100+sd->addrace[11])/100; //�{�X�ȊO�����X�^�[�ɒǉ��_���[�W
			else //����J�[�h�␳�ݒ肠��
				cardfix=cardfix*(100+sd->addrace[11]+sd->addrace_[11])/100; //�{�X�ȊO�����X�^�[�ɒǉ��_���[�W(����ɂ��ǉ�����)
	}
		else
			cardfix=cardfix*(100+sd->addrace[11]+sd->arrow_addrace[11])/100; //�{�X�ȊO�����X�^�[�ɒǉ��_���[�W(�|��ɂ��ǉ�����)
	}
	
	// �J�[�h���ʂɂ����背���W�U���̃_���[�W����
	if(damage > 0){
		if(flag&BF_SHORT){
			cardfix = cardfix * (100+sd->short_weapon_damege_rate) / 100;
		}
		if(flag&BF_LONG){
			cardfix = cardfix * (100+sd->long_weapon_damege_rate) / 100;
		}
	}
	// �J�[�h���ʂɂ�����X�L���̃_���[�W�����i����X�L���j
	if(sd->skill_dmgup.count > 0 && (skill_num > 0) && (damage > 0)){
		for( i=0 ; i<sd->skill_dmgup.count ; i++ ){
			if( skill_num == sd->skill_dmgup.id[i] ){
				cardfix = cardfix*(100+sd->skill_dmgup.rate[i])/100;
				break;
			}
		}
	}
	//����Class�p�␳����(�����̓��L���{���S���p�H)
	t_class = status_get_class(target);
	for(i=0;i<sd->add_damage_class_count;i++) {
		if(sd->add_damage_classid[i] == t_class) {
			cardfix=cardfix*(100+sd->add_damage_classrate[i])/100;
			break;
		}
	}
	if (!no_cardfix)
		damage=damage*cardfix/100; //�J�[�h�␳�ɂ��_���[�W����
//�J�[�h�ɂ��_���[�W�������������܂�

//�J�[�h�ɂ��_���[�W�ǉ�����(����)��������
	cardfix=100;
	if(!battle_config.left_cardfix_to_right) {  //����J�[�h�␳�ݒ薳��
		cardfix=cardfix*(100+sd->addrace_[t_race])/100;	// �푰�ɂ��_���[�W�C������
		cardfix=cardfix*(100+sd->addele_[t_ele])/100;	// �� ���ɂ��_���[�W�C������
		cardfix=cardfix*(100+sd->addsize_[t_size])/100;	// �T�C�Y�ɂ��_���[�W�C������
		cardfix=cardfix*(100+sd->addgroup_[t_size])/100;	// �O���[�v�ɂ��_���[�W�C������
		if(t_mode & 0x20) //�{�X
			cardfix=cardfix*(100+sd->addrace_[10])/100; //�{�X�����X�^�[�ɒǉ��_���[�W����
		else
			cardfix=cardfix*(100+sd->addrace_[11])/100; //�{�X�ȊO�����X�^�[�ɒǉ��_���[�W����
	}
	//����Class�p�␳��������(�����̓��L���{���S���p�H)
	for(i=0;i<sd->add_damage_class_count_;i++) {
		if(sd->add_damage_classid_[i] == t_class) {
			cardfix=cardfix*(100+sd->add_damage_classrate_[i])/100;
			break;
		}
	}
	if(!no_cardfix)
		damage2=damage2*cardfix/100; //�J�[�h�␳�ɂ�鍶��_���[�W����
//�J�[�h�ɂ��_���[�W��������(����)�����܂�

//�J�[�h�ɂ��_���[�W����������������
	if(tsd){ //�Ώۂ�PC�̏ꍇ
		cardfix=100;
		cardfix=cardfix*(100-tsd->subrace[s_race])/100;	// �푰�ɂ��_���[�W�ϐ�
		cardfix=cardfix*(100-tsd->subele[s_ele])/100;	// �����ɂ��_���[�W�ϐ�
		cardfix=cardfix*(100-tsd->subsize[s_size])/100;	// �T�C�Y�ɂ��_���[�W�ϐ�
		cardfix=cardfix*(100-tsd->subgroup[t_group])/100;	// �O���[�v�ɂ��_���[�W�ϐ�
		
		if(status_get_mode(src) & 0x20)
			cardfix=cardfix*(100-tsd->subrace[10])/100; //�{�X����̍U���̓_���[�W����
		else
			cardfix=cardfix*(100-tsd->subrace[11])/100; //�{�X�ȊO����̍U���̓_���[�W����
		
		//����Class�p�␳��������(�����̓��L���{���S���p�H)
		for(i=0;i<tsd->add_def_class_count;i++) {
			if(tsd->add_def_classid[i] == sd->status.class) {
				cardfix=cardfix*(100-tsd->add_def_classrate[i])/100;
				break;
			}
		}
		if(flag&BF_LONG)
			cardfix=cardfix*(100-tsd->long_attack_def_rate)/100; //�������U���̓_���[�W����(�z����C�Ƃ�)
		if(flag&BF_SHORT)
			cardfix=cardfix*(100-tsd->near_attack_def_rate)/100; //�ߋ����U���̓_���[�W����(�Y�������H)
		damage=damage*cardfix/100; //�J�[�h�␳�ɂ��_���[�W����
		damage2=damage2*cardfix/100; //�J�[�h�␳�ɂ�鍶��_���[�W����
	}
//�J�[�h�ɂ��_���[�W�������������܂�

	//�\�E���u���C�J�[
	if (skill_num==ASC_BREAKER) {
		// int�ɂ��ǉ��_���[�W
		damage += status_get_int(src) * skill_lv * 5;
		damage += 500 + (atn_rand() % 500);
	}
	
//�ΏۂɃX�e�[�^�X�ُ킪����ꍇ�̃_���[�W���Z������������
	if(t_sc_data) {
		cardfix=100;
		if(t_sc_data[SC_DEFENDER].timer != -1 && flag&BF_LONG) //�f�B�t�F���_�[��Ԃŉ������U��
			cardfix=cardfix*(100-t_sc_data[SC_DEFENDER].val2)/100; //�f�B�t�F���_�[�ɂ�錸��
		if(cardfix != 100) {
			damage=damage*cardfix/100; //�f�B�t�F���_�[�␳�ɂ��_���[�W����
			damage2=damage2*cardfix/100; //�f�B�t�F���_�[�␳�ɂ�鍶��_���[�W����
		}
		if(t_sc_data[SC_ASSUMPTIO].timer != -1){ //�A�X���v�e�B�I
			if(map[target->m].flag.pvp || map[target->m].flag.gvg)
				damage=damage*2/3;
			else
				damage=damage/2;
		}
	}
//�ΏۂɃX�e�[�^�X�ُ킪����ꍇ�̃_���[�W���Z���������܂�

	if(damage < 0) damage = 0;
	if(damage2 < 0) damage2 = 0;

	// �� ���̓K�p
	damage = battle_attr_fix(damage,s_ele, status_get_element(target));
	damage2 = battle_attr_fix(damage2,s_ele_, status_get_element(target));
	
	// ���̂�����A�C���̓K�p
	damage += sd->star;
	damage2 += sd->star_;
	damage += sd->spiritball*3;
	damage2 += sd->spiritball*3;
	damage += sd->bonus_damage;
	damage2 += sd->bonus_damage;
	
//	if(sc_data && sc_data[SC_AURABLADE].timer!=-1){	/* �I�[���u���[�h �K������Ȃ��Ȃ��� */
//		damage += sc_data[SC_AURABLADE].val1 * 20;
//		damage2 += sc_data[SC_AURABLADE].val1 * 20;
//	}

	if(skill_num==PA_PRESSURE){ /* �v���b�V���[ �K�� */
		damage = 500+300*skill_lv;
		damage2 = 500+300*skill_lv;
	}

	// >�񓁗��̍��E�_���[�W�v�Z�N������Ă��ꂥ�������������I
	// >map_session_data �ɍ���_���[�W(atk,atk2)�ǉ�����
	// >pc_calcstatus()�ł��ׂ����ȁH
	// map_session_data �ɍ��蕐��(atk,atk2,ele,star,atkmods)�ǉ�����
	// pc_calcstatus()�Ńf�[�^����͂��Ă��܂�

	//����̂ݕ��푕��
	if(sd->weapontype1 == 0 && sd->weapontype2 > 0) {
		damage = damage2;
		damage2 = 0;
	}

	// �E��A����C���̓K�p
	if(sd->status.weapon > 16) {// �񓁗���?
		int dmg = damage, dmg2 = damage2;
		// �E��C��(60% �` 100%) �E��S��
		skill = pc_checkskill(sd,AS_RIGHT);
		damage = damage * (50 + (skill * 10))/100;
		if(dmg > 0 && damage < 1) damage = 1;
		// ����C��(40% �` 80%) ����S��
		skill = pc_checkskill(sd,AS_LEFT);
		damage2 = damage2 * (30 + (skill * 10))/100;
		if(dmg2 > 0 && damage2 < 1) damage2 = 1;
	}
	else //�񓁗��łȂ���΍���_���[�W��0
		damage2 = 0;

		// �E��,�Z���̂�
	if(da == 1) { //�_�u���A�^�b�N���������Ă��邩
		div_ = 2;
		damage += damage;
		type = 0x08;
	}

	if(sd->status.weapon == 16) {
		// �J�^�[���ǌ��_���[�W
		skill = pc_checkskill(sd,TF_DOUBLE);
		damage2 = damage * (1 + (skill * 2))/100;
		if(damage > 0 && damage2 < 1) damage2 = 1;
	}

	// �C���x�i���C��
	if(skill_num==TF_POISON){
		damage = battle_attr_fix(damage + 15*skill_lv, s_ele, status_get_element(target) );
	}
	if(skill_num==MC_CARTREVOLUTION){
		damage = battle_attr_fix(damage, 0, status_get_element(target) );
	}

	// ���S����̔���
	if(skill_num == 0 && skill_lv >= 0 && tsd!=NULL && div_ < 255 && atn_rand()%1000 < status_get_flee2(target) ){
		damage=damage2=0;
		type=0x0b;
		dmg_lv = ATK_LUCKY;
	}
	
	// �Ώۂ����S���������ݒ肪ON�Ȃ�
	if(battle_config.enemy_perfect_flee) {
		if(skill_num == 0 && skill_lv >= 0 && tmd!=NULL && div_ < 255 && atn_rand()%1000 < status_get_flee2(target) ) {
			damage=damage2=0;
			type=0x0b;
			dmg_lv = ATK_LUCKY;
		}
	}

	//Mob��Mode�Ɋ拭�t���O�������Ă���Ƃ��̏���
	if(t_mode&0x40){
		if(damage > 0)
			damage = 1;
		if(damage2 > 0)
			damage2 = 1;
	}
	
	//bNoWeaponDamage(�ݒ�A�C�e�������H)�ŃO�����h�N���X����Ȃ��ꍇ�̓_���[�W��0
	if( tsd && tsd->special_state.no_weapon_damage &&(skill_num != CR_GRANDCROSS||skill_num !=NPC_DARKGRANDCROSS))
		damage = damage2 = 0;

	if((skill_num != CR_GRANDCROSS||skill_num !=NPC_DARKGRANDCROSS) && (damage > 0 || damage2 > 0) ) {
		if(damage2<1)		// �_���[�W�ŏI�C��
			damage=battle_calc_damage(src,target,damage,div_,skill_num,skill_lv,flag);
		else if(damage<1)	// �E�肪�~�X�H
			damage2=battle_calc_damage(src,target,damage2,div_,skill_num,skill_lv,flag);
		else {	// �� ��/�J�^�[���̏ꍇ�͂�����ƌv�Z��₱����
			int d1=damage+damage2,d2=damage2;
			damage=battle_calc_damage(src,target,damage+damage2,div_,skill_num,skill_lv,flag);
			damage2=(d2*100/d1)*damage/100;
			if(damage > 1 && damage2 < 1) damage2=1;
			damage-=damage2;
		}
	}
	
	//�����U���X�L���ɂ��I�[�g�X�y������(item_bonus)
	if(battle_config.weapon_attack_autospell && src && src->type == BL_PC && src != target 	&& (damage+damage2)> 0)
			skill_bonus_autospell(src,target,AS_ATTACK,0,0);
			
	//���z�ƌ��Ɛ��̗Z�� HP2%����
	if(sd && sd->sc_data && sd->sc_data[SC_FUSION].timer!=-1)
	{
		int hp = 0;
			
		if(target->type == BL_PC)
		{
			hp = sd->status.max_hp * 8 / 100;
			if((sd->status.hp/sd->status.max_hp) <= 20)
				hp = sd->status.hp;
		}else 
			hp = sd->status.max_hp * 2 / 100;
		pc_heal(sd,-hp,0);
	}
	
	//�J�E�v�I������
	if((damage>0 || damage2>0) && t_sc_data && t_sc_data[SC_KAUPE].timer!=-1)
		status_change_end(target,SC_KAUPE,-1);
	
	wd.damage=damage;
	wd.damage2=damage2;
	wd.type=type;
	wd.div_=div_;
	wd.amotion=status_get_amotion(src);
	if(skill_num == KN_AUTOCOUNTER)
		wd.amotion >>= 1;
	wd.dmotion=status_get_dmotion(target);
	wd.blewcount=blewcount;
	wd.flag=flag;
	wd.dmg_lv=dmg_lv;

	return wd;
}

/*==========================================
 * ����_���[�W�v�Z
 *------------------------------------------
 */
struct Damage battle_calc_weapon_attack(
	struct block_list *src,struct block_list *target,int skill_num,int skill_lv,int wflag)
{
	static struct Damage wd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	//return�O�̏���������̂ŏ��o�͕��̂ݕύX
	if( src == NULL || target == NULL ){
		nullpo_info(NLP_MARK);
		memset(&wd,0,sizeof(wd));
		return wd;
	}

	if(target->type == BL_PET)
		memset(&wd,0,sizeof(wd));
	else if(src->type == BL_PC)
		wd = battle_calc_pc_weapon_attack(src,target,skill_num,skill_lv,wflag);
	else if(src->type == BL_MOB)
		wd = battle_calc_mob_weapon_attack(src,target,skill_num,skill_lv,wflag);
	else if(src->type == BL_PET)
		wd = battle_calc_pet_weapon_attack(src,target,skill_num,skill_lv,wflag);
	else
		memset(&wd,0,sizeof(wd));

	return wd;
}

/*==========================================
 * ���@�_���[�W�v�Z
 *------------------------------------------
 */
struct Damage battle_calc_magic_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag)
{
	int mdef1=status_get_mdef(target);
	int mdef2=status_get_mdef2(target);
	int matk1,matk2,damage=0,div_=1,blewcount=skill_get_blewcount(skill_num,skill_lv);
	struct Damage md;
	int aflag;
	int normalmagic_flag=1;
	int ele=0,race=7,t_ele=0,t_race=7,t_mode = 0,cardfix,t_class,i;
	struct map_session_data *sd=NULL,*tsd=NULL;
	struct mob_data *tmd = NULL;
	struct status_change *sc_data;
	struct status_change *t_sc_data;

	//return�O�̏���������̂ŏ��o�͕��̂ݕύX
	if( bl == NULL || target == NULL ){
		nullpo_info(NLP_MARK);
		memset(&md,0,sizeof(md));
		return md;
	}

	if(target->type == BL_PET) {
		memset(&md,0,sizeof(md));
		return md;
	}

	matk1=status_get_matk1(bl);
	matk2=status_get_matk2(bl);
	ele = skill_get_pl(skill_num);
	race = status_get_race(bl);
	t_ele = status_get_elem_type(target);
	t_race = status_get_race(target);
	t_mode = status_get_mode(target);

#define MATK_FIX( a,b ) { matk1=matk1*(a)/(b); matk2=matk2*(a)/(b); }

	if( bl->type==BL_PC && (sd = (struct map_session_data *)bl))
	{
		sd->state.attack_type = BF_MAGIC;
		if(sd->matk_rate != 100)
			MATK_FIX(sd->matk_rate,100);
		sd->state.arrow_atk = 0;
	}
	if( target->type==BL_PC )
		tsd=(struct map_session_data *)target;
	else if( target->type==BL_MOB )
		tmd=(struct mob_data *)target;

	aflag=BF_MAGIC|BF_LONG|BF_SKILL;

	// ���@�͑����ɂ��MATK����
	sc_data = status_get_sc_data(bl);
	t_sc_data = status_get_sc_data(target);
	if (sc_data && sc_data[SC_MAGICPOWER].timer != -1) {
		matk1 += (matk1 * sc_data[SC_MAGICPOWER].val1 * 5)/100;
		matk2 += (matk2 * sc_data[SC_MAGICPOWER].val1 * 5)/100;
	}
	
	if(skill_num > 0){
		switch(skill_num){	// ��{�_���[�W�v�Z(�X�L�����Ƃɏ���)
					// �q�[��or����
		case AL_HEAL:
		case PR_BENEDICTIO:
			damage = skill_calc_heal(bl,skill_lv)/2;
			normalmagic_flag=0;
			break;
		case PR_ASPERSIO:		/* �A�X�y���V�I */
			damage = 40; //�Œ�_���[�W
			normalmagic_flag=0;
			break;
		case PR_SANCTUARY:	// �T���N�`���A��
			damage = (skill_lv>6)?388:skill_lv*50;
			normalmagic_flag=0;
			blewcount|=0x10000;
			break;
		case PA_GOSPEL:		// �S�X�y��(�����_���_���[�W����̏ꍇ)
			damage = 1000+atn_rand()%9000;
			normalmagic_flag=0;
			break;
		case ALL_RESURRECTION:
		case PR_TURNUNDEAD:	// �U�����U���N�V�����ƃ^�[���A���f�b�h
			if(target->type != BL_PC && battle_check_undead(t_race,t_ele)){
				int hp, mhp, thres;
				hp = status_get_hp(target);
				mhp = status_get_max_hp(target);
				thres = (skill_lv * 20) + status_get_luk(bl)+
						status_get_int(bl) + status_get_lv(bl)+
						((200 - hp * 200 / mhp));
				if(thres > 700) thres = 700;
//				if(battle_config.battle_log)
//					printf("�^�[���A���f�b�h�I �m��%d ��(�番��)\n", thres);
				if(atn_rand()%1000 < thres && !(t_mode&0x20))	// ����
					damage = hp;
				else					// ���s
					damage = status_get_lv(bl) + status_get_int(bl) + skill_lv * 10;
			}
			normalmagic_flag=0;
			break;

		case MG_NAPALMBEAT:	// �i�p�[���r�[�g�i���U�v�Z���݁j
			MATK_FIX(70+ skill_lv*10,100);
			if(flag>0){
				MATK_FIX(1,flag);
			}else {
				if(battle_config.error_log)
					printf("battle_calc_magic_attack(): napam enemy count=0 !\n");
			}
			break;
		case MG_SOULSTRIKE:			/* �\�E���X�g���C�N �i�΃A���f�b�h�_���[�W�␳�j*/
			if(target->type != BL_PC && battle_check_undead(t_race,t_ele)){
			matk1 = matk1*(20+skill_lv)/20;//MATK�ɕ␳����ʖڂł����ˁH
			matk2 = matk2*(20+skill_lv)/20;
			}
			break;
		case MG_FIREBALL:	// �t�@�C���[�{�[��
			{
				const int drate[]={100,90,70};
				if(flag>2)
					matk1=matk2=0;
				else
					MATK_FIX( (95+skill_lv*5)*drate[flag] ,10000 );
			}
			break;
		case MG_FIREWALL:	// �t�@�C���[�E�H�[��
/*
			if( (t_ele!=3 && !battle_check_undead(t_race,t_ele)) || target->type==BL_PC ) //PC�͉Α����ł���ԁH���������_���[�W�󂯂�H
				blewcount |= 0x10000;
			else
				blewcount = 0;
*/
			if((t_ele==3 || battle_check_undead(t_race,t_ele)) && target->type!=BL_PC)
				blewcount = 0;
			else
				blewcount |= 0x10000;
			MATK_FIX( 1,2 );
			break;
		case MG_THUNDERSTORM:	// �T���_�[�X�g�[��
			MATK_FIX( 80,100 );
			break;
		case MG_FROSTDIVER:	// �t���X�g�_�C�o
			MATK_FIX( 100+skill_lv*10, 100);
			break;
		case WZ_FROSTNOVA:	// �t���X�g�m���@
			MATK_FIX((100+skill_lv*10)*2/3, 100);
			break;
		case WZ_FIREPILLAR:	// �t�@�C���[�s���[
			if(mdef1 < 1000000)
				mdef1=mdef2=0;	// MDEF����
			MATK_FIX( 1,5 );
			matk1+=50;
			matk2+=50;
			break;
		case WZ_SIGHTRASHER:
			MATK_FIX( 100+skill_lv*20, 100);
			break;
		case WZ_METEOR:
		case WZ_JUPITEL:	// ���s�e���T���_�[
		case NPC_DARKJUPITEL:	//�Ń��s�e��
			break;
		case WZ_VERMILION:	// ���[�h�I�u�o�[�~���I��
			MATK_FIX( skill_lv*20+80, 100 );
			break;
		case WZ_WATERBALL:	// �E�H�[�^�[�{�[��
			MATK_FIX( 100+skill_lv*30, 100 );
			break;
		case WZ_STORMGUST:	// �X�g�[���K�X�g
			MATK_FIX( skill_lv*40+100 ,100 );
//			blewcount|=0x10000;
			break;
		case AL_HOLYLIGHT:	// �z�[���[���C�g
			MATK_FIX( 125,100 );
			if(sc_data && sc_data[SC_PRIEST].timer!=-1)
			{
				matk1 *= 5;
				matk2 *= 5;
			}
			break;
		case AL_RUWACH:
			MATK_FIX( 145,100 );
			break;
			
		case SL_STIN://�G�X�e�B��
			if(status_get_size(target) == 0)
			{
				MATK_FIX(skill_lv*10,100);
			}
			else
			{
				MATK_FIX(skill_lv*1,100);
			}
			//ele = status_get_attack_element(bl);
			if(skill_lv==7)
				status_change_start(bl,SC_SMA,skill_lv,0,0,0,3000,0);
			break;
		case SL_STUN://�G�X�^��
			MATK_FIX(skill_lv*5,100);
			ele = status_get_attack_element(bl);
			if(skill_lv==7)
				status_change_start(bl,SC_SMA,skill_lv,0,0,0,3000,0);
			break;
		case SL_SMA://�G�X�}
			if(sd)
			{
				MATK_FIX(40+sd->status.base_level,100);
			}
			ele = status_get_attack_element(bl);
			if(sc_data && sc_data[SC_SMA].timer!=-1)
				status_change_end(bl,SC_SMA,-1);
			break;
		}
	}

	if(normalmagic_flag){	// ��ʖ��@�_���[�W�v�Z
		int imdef_flag=0;
		if(matk1>matk2)
			damage= matk2+atn_rand()%(matk1-matk2+1);
		else
			damage= matk2;
		if(sd) {
			if(sd->ignore_mdef_ele & (1<<t_ele) || sd->ignore_mdef_race & (1<<t_race))
				imdef_flag = 1;
			if(t_mode & 0x20) {
				if(sd->ignore_mdef_race & (1<<10))
					imdef_flag = 1;
			}
			else {
				if(sd->ignore_mdef_race & (1<<11))
					imdef_flag = 1;
			}
		}
		if(!imdef_flag){
			if(battle_config.magic_defense_type) {
				damage = damage - (mdef1 * battle_config.magic_defense_type) - mdef2;
			}
			else{
				damage = (damage*(100-mdef1))/100 - mdef2;
			}
		}

		if(damage<1)
			damage=1;
	}

	if(sd) {
		cardfix=100;
		cardfix=cardfix*(100+sd->magic_addrace[t_race])/100;
		cardfix=cardfix*(100+sd->magic_addele[t_ele])/100;
		if(t_mode & 0x20)
			cardfix=cardfix*(100+sd->magic_addrace[10])/100;
		else
			cardfix=cardfix*(100+sd->magic_addrace[11])/100;
		t_class = status_get_class(target);
		for(i=0;i<sd->add_magic_damage_class_count;i++) {
			if(sd->add_magic_damage_classid[i] == t_class) {
				cardfix=cardfix*(100+sd->add_magic_damage_classrate[i])/100;
				break;
			}
		}
		// �J�[�h���ʂɂ�����X�L���̃_���[�W�����i���@�X�L���j
		if((bl->type == BL_PC) && (sd->skill_dmgup.count > 0) && (skill_num > 0) && (damage > 0)){
			for( i=0 ; i<sd->skill_dmgup.count ; i++ ){
				if( skill_num == sd->skill_dmgup.id[i] ){
					cardfix = cardfix*(100+sd->skill_dmgup.rate[i])/100;
					break;
				}
			}
		}
		damage=damage*cardfix/100;
	}

	if( tsd ){
		int s_class = status_get_class(bl);
		cardfix=100;
		cardfix=cardfix*(100-tsd->subele[ele])/100;	// �� ���ɂ��_���[�W�ϐ�
		cardfix=cardfix*(100-tsd->subrace[race])/100;	// �푰�ɂ��_���[�W�ϐ�
		cardfix=cardfix*(100-tsd->subsize[status_get_size( bl )])/100;	// �T�C�Y�ɂ��_���[�W�ϐ�
		cardfix=cardfix*(100-tsd->magic_subrace[race])/100;
		if(status_get_mode(bl) & 0x20)
			cardfix=cardfix*(100-tsd->magic_subrace[10])/100;
		else
			cardfix=cardfix*(100-tsd->magic_subrace[11])/100;
		for(i=0;i<tsd->add_mdef_class_count;i++) {
			if(tsd->add_mdef_classid[i] == s_class) {
				cardfix=cardfix*(100-tsd->add_mdef_classrate[i])/100;
				break;
			}
		}
		cardfix=cardfix*(100-tsd->magic_def_rate)/100;
		damage=damage*cardfix/100;
	}
	if(damage < 0) damage = 0;

	damage=battle_attr_fix(damage, ele, status_get_element(target) );		// �� ���C��

	if(skill_num == CR_GRANDCROSS||skill_num ==NPC_DARKGRANDCROSS) {	// �O�����h�N���X
		static struct Damage wd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		wd=battle_calc_weapon_attack(bl,target,skill_num,skill_lv,flag);
		damage = (damage + wd.damage) * (100 + 40*skill_lv)/100;
		if(battle_config.gx_dupele) damage=battle_attr_fix(damage, ele, status_get_element(target) );	//����2�񂩂���
		if(bl==target){
			if(bl->type == BL_MOB)
				damage = 0;		//MOB���g���ꍇ�͔�������
		else
			 damage=damage/2;	//�����͔���
		}
	}
	div_=skill_get_num( skill_num,skill_lv );

	if (skill_num==WZ_WATERBALL)
		div_ = 1;
	
	if(div_>1 && skill_num != WZ_VERMILION)
		damage*=div_;

	if(t_mode&0x40 && damage > 0)
		damage = 1;

	if( tsd && tsd->special_state.no_magic_damage )
		damage=0;	// �� ��峃J�[�h�i���@�_���[�W�O�j
		
	//�J�C�g
	/*
	if(damage > 0 && t_sc_data && t_sc_data[SC_KAITE].timer!=-1)
	{
		if(bl->type == BL_PC || status_get_lv(bl) < 80)
		{
			t_sc_data[SC_KAITE].val2--;
			if(t_sc_data[SC_KAITE].val2==0)
				status_change_end(target,SC_KAITE,-1);
		}
	}
	*/

	damage=battle_calc_damage(bl,target,damage,div_,skill_num,skill_lv,aflag);	// �ŏI�C��

	//���@�ł��I�[�g�X�y������(item_bonus)
	if(battle_config.magic_attack_autospell && bl && bl->type == BL_PC && bl != target 	&& damage > 0)
	{
		skill_bonus_autospell(bl,target,AS_ATTACK,0,0);
	}

	//���@�ł�HP/SP��(�������Ȃ�)
	if(battle_config.magic_attack_drain && sd)
		battle_attack_drain(bl,target,damage,0,battle_config.magic_attack_drain_per_enable);
	
	md.damage=damage;
	md.div_=div_;
	md.amotion=status_get_amotion(bl);
	md.dmotion=status_get_dmotion(target);
	md.damage2=0;
	md.type=0;
	md.blewcount=blewcount;
	md.flag=aflag;

	return md;
}

/*==========================================
 * ���̑��_���[�W�v�Z
 *------------------------------------------
 */
struct Damage  battle_calc_misc_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag)
{
	int int_=status_get_int(bl);
//	int luk=status_get_luk(bl);
	int dex=status_get_dex(bl);
	int skill,ele,race,cardfix;
	struct map_session_data *sd=NULL,*tsd=NULL;
	int damage=0,div_=1,blewcount=skill_get_blewcount(skill_num,skill_lv);
	struct Damage md;
	int damagefix=1;

	int aflag=BF_MISC|BF_SHORT|BF_SKILL;

	//return�O�̏���������̂ŏ��o�͕��̂ݕύX
	if( bl == NULL || target == NULL ){
		nullpo_info(NLP_MARK);
		memset(&md,0,sizeof(md));
		return md;
	}

	if(target->type == BL_PET) {
		memset(&md,0,sizeof(md));
		return md;
	}

	if( bl->type == BL_PC && (sd=(struct map_session_data *)bl)!=NULL ) {
		sd->state.attack_type = BF_MISC;
		sd->state.arrow_atk = 0;
	}

	if( target->type==BL_PC )
		tsd=(struct map_session_data *)target;

	ele = skill_get_pl(skill_num);
	race = status_get_race(bl);
	
	switch(skill_num){

	case HT_LANDMINE:	// �����h�}�C��
		damage=skill_lv*(dex+75)*(100+int_)/100;
		break;

	case HT_BLASTMINE:	// �u���X�g�}�C��
		damage=skill_lv*(dex/2+50)*(100+int_)/100;
		break;
	
	case HT_CLAYMORETRAP:	// �N���C���A�[�g���b�v
		damage=skill_lv*(dex/2+75)*(100+int_)/100;
		break;

	case HT_BLITZBEAT:	// �u���b�c�r�[�g
		if( sd==NULL || (skill = pc_checkskill(sd,HT_STEELCROW)) <= 0)
			skill=0;
		damage=(dex/10+int_/2+skill*3+40)*2;
		if(flag > 1)
			damage /= flag;
		aflag |= (flag&~BF_RANGEMASK)|BF_LONG;
		break;

	case TF_THROWSTONE:	// �Γ���
		damage=50;
		damagefix=0;
		aflag |= (flag&~BF_RANGEMASK)|BF_LONG;
		break;

	case BA_DISSONANCE:	// �s���a��
		damage=(skill_lv)*20+pc_checkskill(sd,BA_MUSICALLESSON)*3;
		break;

	case NPC_SELFDESTRUCTION:	// ����
	case NPC_SELFDESTRUCTION2:	// ����2
		damage=status_get_hp(bl)-(bl==target?1:0);
		damagefix=0;
		break;

	case NPC_SMOKING:	// �^�o�R���z��
		damage=3;
		damagefix=0;
		break;

	case NPC_DARKBREATH:
		{
			struct status_change *sc_data = status_get_sc_data(target);
			int hitrate=status_get_hit(bl) - status_get_flee(target) + 80;
			hitrate = ( (hitrate>95)?95: ((hitrate<5)?5:hitrate) );
			if(sc_data && (sc_data[SC_SLEEP].timer!=-1 || sc_data[SC_STAN].timer!=-1 ||
				sc_data[SC_FREEZE].timer!=-1 || (sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2==0) ) )
				hitrate = 1000000;
			if(atn_rand()%100 < hitrate) {
				damage = 500 + (skill_lv-1)*1000 + atn_rand()%1000;
				if(damage > 9999) damage = 9999;
			}
		}
		break;
	case SN_FALCONASSAULT:			/* �t�@���R���A�T���g */
		if( sd==NULL || (skill = pc_checkskill(sd,HT_STEELCROW)) <= 0)
			skill=0;
		{
			int skill_blitzbeat = pc_checkskill(sd,HT_BLITZBEAT);
			if(skill_blitzbeat < 1) skill_blitzbeat = 1;
			damage=(dex/10+int_/2+skill*3+40)*2*(150+skill_blitzbeat*70)/100*(150+skill_lv*70)/100;
			if(sd && battle_config.allow_falconassault_elemet) ele = sd->atk_ele;
		}
		aflag |= (flag&~BF_RANGEMASK)|BF_LONG;
		break;
	}

	if(damagefix){
		if(damage<1 && skill_num != NPC_DARKBREATH)
			damage=1;

		if( tsd ){
			cardfix=100;
			cardfix=cardfix*(100-tsd->subele[ele])/100;	// �����ɂ��_���[�W�ϐ�
			cardfix=cardfix*(100-tsd->subrace[race])/100;	// �푰�ɂ��_���[�W�ϐ�
			cardfix=cardfix*(100-tsd->subsize[status_get_size( bl )])/100;	// �T�C�Y�ɂ��_���[�W�ϐ�
			cardfix=cardfix*(100-tsd->misc_def_rate)/100;
			damage=damage*cardfix/100;
		}
		if(damage < 0) damage = 0;
		damage=battle_attr_fix(damage, ele, status_get_element(target) );		// �����C��
	}

	div_=skill_get_num( skill_num,skill_lv );
	if(div_>1)
		damage*=div_;

	if(damage > 0 && (damage < div_ || (status_get_def(target) >= 1000000 && status_get_mdef(target) >= 1000000) ) ) {
		damage = div_;
	}

	if(status_get_mode(target)&0x40 && damage>0)
		damage = 1;

	// �J�[�h���ʂɂ�����X�L���̃_���[�W�����i���̑��̃X�L���j
	if((bl->type == BL_PC) && (sd->skill_dmgup.count > 0) && (skill_num > 0) && (damage > 0)){
		int i;
		for( i=0 ; i<sd->skill_dmgup.count ; i++ ){
			if( skill_num == sd->skill_dmgup.id[i] ){
				damage += damage * sd->skill_dmgup.rate[i] / 100;
				break;
			}
		}
	}
	
	damage=battle_calc_damage(bl,target,damage,div_,skill_num,skill_lv,aflag);	// �ŏI�C��

	//misc�ł��I�[�g�X�y������(bonus)
	if(battle_config.misc_attack_autospell && bl && bl->type == BL_PC && bl != target 	&& damage > 0)
	{
		skill_bonus_autospell(bl,target,AS_ATTACK,0,0);
	}

	//misc�ł�HP/SP��(�������Ȃ�)
	if(battle_config.misc_attack_drain)
		battle_attack_drain(bl,target,damage,0,battle_config.misc_attack_drain_per_enable);
	
	md.damage=damage;
	md.div_=div_;
	md.amotion=status_get_amotion(bl);
	md.dmotion=status_get_dmotion(target);
	md.damage2=0;
	md.type=0;
	md.blewcount=blewcount;
	md.flag=aflag;
	
	return md;

}

/*


*/
int battle_attack_drain(struct block_list *bl,struct block_list *target,int damage,int damage2,int calc_per_drain_flag)
{
	int hp = 0,sp = 0;
	struct map_session_data* sd = NULL;
	
	nullpo_retr(0, bl);
	nullpo_retr(0, target);
	
	sd = (struct map_session_data *)bl;
	
	if(bl->type != BL_PC)
		return 0;
	
	if(bl == target)
		return 0;
		
	if(!(damage > 0 || damage2 >0))
		return 0;
		
	if(calc_per_drain_flag)//���z�����̂���
	{
		if (!battle_config.left_cardfix_to_right) { // �񓁗�����J�[�h�̋z���n���ʂ��E��ɒǉ����Ȃ��ꍇ
			hp += battle_calc_drain(damage, sd->hp_drain_rate, sd->hp_drain_per, sd->hp_drain_value);
			hp += battle_calc_drain(damage2, sd->hp_drain_rate_, sd->hp_drain_per_, sd->hp_drain_value_);
			sp += battle_calc_drain(damage, sd->sp_drain_rate, sd->sp_drain_per, sd->sp_drain_value);
			sp += battle_calc_drain(damage2, sd->sp_drain_rate_, sd->sp_drain_per_, sd->sp_drain_value_);
		} else { // �񓁗�����J�[�h�̋z���n���ʂ��E��ɒǉ�����ꍇ
			int hp_drain_rate = sd->hp_drain_rate + sd->hp_drain_rate_;
			int hp_drain_per = sd->hp_drain_per + sd->hp_drain_per_;
			int hp_drain_value = sd->hp_drain_value + sd->hp_drain_value_;
			int sp_drain_rate = sd->sp_drain_rate + sd->sp_drain_rate_;
			int sp_drain_per = sd->sp_drain_per + sd->sp_drain_per_;
			int sp_drain_value = sd->sp_drain_value + sd->sp_drain_value_;
			hp += battle_calc_drain(damage, hp_drain_rate, hp_drain_per, hp_drain_value);
			sp += battle_calc_drain(damage, sp_drain_rate, sp_drain_per, sp_drain_value);
		}
	}else{//���z���͏悹�Ȃ�
		if (!battle_config.left_cardfix_to_right) { // �񓁗�����J�[�h�̋z���n���ʂ��E��ɒǉ����Ȃ��ꍇ
			hp += battle_calc_drain(damage, sd->hp_drain_rate, 0, sd->hp_drain_value);
			hp += battle_calc_drain(damage2, sd->hp_drain_rate_, 0, sd->hp_drain_value_);
			sp += battle_calc_drain(damage, sd->sp_drain_rate, 0, sd->sp_drain_value);
			sp += battle_calc_drain(damage2, sd->sp_drain_rate_, 0, sd->sp_drain_value_);
		} else { // �񓁗�����J�[�h�̋z���n���ʂ��E��ɒǉ�����ꍇ
			int hp_drain_rate = sd->hp_drain_rate + sd->hp_drain_rate_;
			int hp_drain_value = sd->hp_drain_value + sd->hp_drain_value_;
			int sp_drain_rate = sd->sp_drain_rate + sd->sp_drain_rate_;
			int sp_drain_value = sd->sp_drain_value + sd->sp_drain_value_;
			hp += battle_calc_drain(damage,hp_drain_rate, 0, hp_drain_value);
			sp += battle_calc_drain(damage,sp_drain_rate, 0, sp_drain_value);
		}
	}
	if (hp || sp) pc_heal(sd, hp, sp);
	return 1;
}
/*==========================================
 * �_���[�W�v�Z�ꊇ�����p
 *------------------------------------------
 */
struct Damage battle_calc_attack(	int attack_type,
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag)
{
	static struct Damage wd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	switch(attack_type){
	case BF_WEAPON:
		return battle_calc_weapon_attack(bl,target,skill_num,skill_lv,flag);
	case BF_MAGIC:
		return battle_calc_magic_attack(bl,target,skill_num,skill_lv,flag);
	case BF_MISC:
		return battle_calc_misc_attack(bl,target,skill_num,skill_lv,flag);
	default:
		if(battle_config.error_log)
			printf("battle_calc_attack: unknwon attack type ! %d\n",attack_type);
		break;
	}
	return wd;
}
/*==========================================
 * �ʏ�U�������܂Ƃ�
 *------------------------------------------
 */
int battle_weapon_attack( struct block_list *src,struct block_list *target,
	 unsigned int tick,int flag)
{
	struct map_session_data *sd=NULL;
	struct status_change *sc_data = status_get_sc_data(src),*t_sc_data=status_get_sc_data(target);
	short *opt1;
	int race = 7, ele = 0;
	int damage,rdamage = 0;
	static struct Damage wd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	nullpo_retr(0, src);
	nullpo_retr(0, target);

	if(src->type == BL_PC)
		sd = (struct map_session_data *)src;

	if(src->prev == NULL || target->prev == NULL)
		return 0;
	if(src->type == BL_PC && pc_isdead(sd))
		return 0;
	if(target->type == BL_PC && pc_isdead((struct map_session_data *)target))
		return 0;

	opt1=status_get_opt1(src);
	if(opt1 && *opt1 > 0) {
		battle_stopattack(src);
		return 0;
	}
	if(sc_data && sc_data[SC_BLADESTOP].timer!=-1){
		battle_stopattack(src);
		return 0;
	}

	if(battle_check_target(src,target,BCT_ENEMY) <= 0 &&
				!battle_check_range(src,target,0))
		return 0;	// �U���ΏۊO

	//�^�[�Q�b�g��MOB GM�n�C�h���ŁA�R���t�B�O�Ńn�C�h���U���s�� GM���x�����w����傫���ꍇ
	if(target->type==BL_MOB && sd && sd->status.option&0x40 && battle_config.hide_attack == 0	&& pc_isGM(sd)<battle_config.gm_hide_attack_lv)
		return 0;	// �B��čU������Ȃ�Ĕڋ���GM�޽�

	race = status_get_race(target);
	ele = status_get_elem_type(target);
	if(sd && sd->status.weapon == 11) {
		if(sd->equip_index[10] >= 0) {
			if(battle_config.arrow_decrement)
				pc_delitem(sd,sd->equip_index[10],1,0);
		}
		else {
			clif_arrow_fail(sd,0);
			return 0;
		}
	}
	if(flag&0x8000) {
		if(sd && battle_config.pc_attack_direction_change)
			sd->dir = sd->head_dir = map_calc_dir(src, target->x,target->y );
		else if(src->type == BL_MOB && battle_config.monster_attack_direction_change)
			((struct mob_data *)src)->dir = map_calc_dir(src, target->x,target->y );
		wd=battle_calc_weapon_attack(src,target,KN_AUTOCOUNTER,flag&0xff,0);
	} else
		wd=battle_calc_weapon_attack(src,target,0,0,0);

	if((damage = wd.damage + wd.damage2) > 0 && src != target) {
		if(wd.flag&BF_SHORT) {
			if(target->type == BL_PC) {
				struct map_session_data *tsd = (struct map_session_data *)target;
				if(tsd && tsd->short_weapon_damage_return > 0) {
					rdamage += damage * tsd->short_weapon_damage_return / 100;
					if(rdamage < 1) rdamage = 1;
				}
			}
			if(t_sc_data && t_sc_data[SC_REFLECTSHIELD].timer != -1) {
				rdamage += damage * t_sc_data[SC_REFLECTSHIELD].val2 / 100;
				if(rdamage < 1) rdamage = 1;
			}
		} else if(wd.flag&BF_LONG) {
			if(target->type == BL_PC) {
				struct map_session_data *tsd = (struct map_session_data *)target;
				if(tsd && tsd->long_weapon_damage_return > 0) {
					rdamage += damage * tsd->long_weapon_damage_return / 100;
					if(rdamage < 1) rdamage = 1;
				}
			}
		}
		if(rdamage > 0)
			clif_damage(src,src,tick,wd.amotion,wd.dmotion,rdamage,1,4,0);
	}

	if (wd.div_ == 255 && sd)	{ //�O�i��
		int delay = 0;
		int skilllv;
		if(wd.damage+wd.damage2 < status_get_hp(target)) {
			if((skilllv = pc_checkskill(sd, MO_CHAINCOMBO)) > 0) {
				delay = 1000 - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
				delay += 300 * battle_config.combo_delay_rate /100; 
				//�R���{���͎��Ԃ̍Œ�ۏ�ǉ�
				if(delay < battle_config.combo_delay_lower_limits)
					delay = battle_config.combo_delay_lower_limits;
			}
			status_change_start(src,SC_COMBO,MO_TRIPLEATTACK,skilllv,0,0,delay,0);
		}
		sd->attackabletime = sd->canmove_tick = tick + delay;
		clif_combo_delay(src,delay);
		clif_skill_damage(src , target , tick , wd.amotion , wd.dmotion , 
			wd.damage , 3 , MO_TRIPLEATTACK, pc_checkskill(sd,MO_TRIPLEATTACK) , -1 );
	}else if (wd.div_ >= 252 && wd.div_<=254 && sd)	{ //����
		int delay = 0;
		int skillid = TK_STORMKICK + 2*(wd.div_-252);
		int skilllv;
		if(wd.damage+wd.damage2 < status_get_hp(target)) {
			if((skilllv = pc_checkskill(sd, skillid)) > 0) {
				delay = 1000;// - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
				delay += 300 * battle_config.combo_delay_rate /100; 
				//�R���{���͎��Ԃ̍Œ�ۏ�ǉ�
				if(delay < battle_config.combo_delay_lower_limits)
					delay = battle_config.combo_delay_lower_limits;
			}
			status_change_start(src,SC_TKCOMBO,skillid,skilllv,0,0,delay,0);
		}
		sd->attackabletime = sd->canmove_tick = tick + delay;
		clif_combo_delay(src,delay);
		//clif_skill_damage(src , target , tick , wd.amotion , wd.dmotion , 
		//	wd.damage , 1 , TK_READYSTORM, pc_checkskill(sd,TK_STORMKICK) , -1 );
		//clif_skill_nodamage(src , target , tick , wd.amotion , wd.dmotion , 
		//	wd.damage , 1 , skillid-1, pc_checkskill(sd,skillid-1) , -1 );
		clif_skill_nodamage(&sd->bl,&sd->bl,skillid-1,pc_checkskill(sd,skillid-1),1);
	}
	else {
		clif_damage(src,target,tick, wd.amotion, wd.dmotion, 
			wd.damage, wd.div_ , wd.type, wd.damage2);
	//�񓁗�����ƃJ�^�[���ǌ��̃~�X�\��(�������`)
		if(sd && sd->status.weapon >= 16 && wd.damage2 == 0)
			clif_damage(src,target,tick+10, wd.amotion, wd.dmotion,0, 1, 0, 0);
	}
	if(sd && sd->splash_range > 0 && (wd.damage > 0 || wd.damage2 > 0) )
		skill_castend_damage_id(src,target,0,-1,tick,0);
	map_freeblock_lock();
	battle_delay_damage(tick+wd.amotion,src,target,(wd.damage+wd.damage2),0);
	if(target->prev != NULL &&
		(target->type != BL_PC || (target->type == BL_PC && !pc_isdead((struct map_session_data *)target) ) ) ) {
		if(wd.damage > 0 || wd.damage2 > 0) {
			skill_additional_effect(src,target,0,0,BF_WEAPON,tick);
			if(sd) {
				if(sd->weapon_coma_ele[ele] > 0 && atn_rand()%10000 < sd->weapon_coma_ele[ele])
					battle_damage(src,target,status_get_max_hp(target),1);
				if(sd->weapon_coma_race[race] > 0 && atn_rand()%10000 < sd->weapon_coma_race[race])
					battle_damage(src,target,status_get_max_hp(target),1);
				if(status_get_mode(target) & 0x20) {
					if(sd->weapon_coma_race[10] > 0 && atn_rand()%10000 < sd->weapon_coma_race[10])
						battle_damage(src,target,status_get_max_hp(target),1);
				}
				else {
					if(sd->weapon_coma_race[11] > 0 && atn_rand()%10000 < sd->weapon_coma_race[11])
						battle_damage(src,target,status_get_max_hp(target),1);
				}

				if(sd->weapon_coma_ele2[ele] > 0 && atn_rand()%10000 < sd->weapon_coma_ele2[ele])
					battle_damage(src,target,status_get_max_hp(target)-1,1);
				if(sd->weapon_coma_race2[race] > 0 && atn_rand()%10000 < sd->weapon_coma_race2[race])
					battle_damage(src,target,status_get_max_hp(target)-1,1);
				if(status_get_mode(target) & 0x20) {
					if(sd->weapon_coma_race2[10] > 0 && atn_rand()%10000 < sd->weapon_coma_race2[10])
						battle_damage(src,target,status_get_max_hp(target)-1,1);
				}
				else {
					if(sd->weapon_coma_race2[11] > 0 && atn_rand()%10000 < sd->weapon_coma_race2[11])
						battle_damage(src,target,status_get_max_hp(target)-1,1);
				}
				
				if(sd->break_weapon_rate > 0 && atn_rand()%10000 < sd->break_weapon_rate
					&& target->type ==BL_PC)
						pc_break_equip((struct map_session_data *)target, EQP_WEAPON);
				if(sd->break_armor_rate > 0 && atn_rand()%10000 < sd->break_armor_rate
					&& target->type ==BL_PC)
						pc_break_equip((struct map_session_data *)target, EQP_ARMOR);

			}
		}
	}
	if(sc_data && sc_data[SC_AUTOSPELL].timer != -1 && atn_rand()%100 < sc_data[SC_AUTOSPELL].val4) {
		int skilllv=sc_data[SC_AUTOSPELL].val3,i,f=0;
		i = atn_rand()%100;
		if(i >= 50) skilllv -= 2;
		else if(i >= 15) skilllv--;
		if(skilllv < 1) skilllv = 1;
		//PC���Z�[�W�̍�
		if(sd && sd->sc_data[SC_SAGE].timer!=-1)
			skilllv = pc_checkskill(sd,sc_data[SC_AUTOSPELL].val2);

		if(sd) {
			int sp = skill_get_sp(sc_data[SC_AUTOSPELL].val2,skilllv)*2/3;
			if(sd->status.sp >= sp) {
				if((skill_get_inf(sd->autospell_id) == 2) || (skill_get_inf(sd->autospell_id) == 32))
					f = skill_castend_pos2(src,target->x,target->y,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
				else {
					switch( skill_get_nk(sc_data[SC_AUTOSPELL].val2) ) {
						case 0:	case 2: case 4:
							f = skill_castend_damage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
							break;
						case 1:/* �x���n */
							if((sc_data[SC_AUTOSPELL].val2==AL_HEAL || (sc_data[SC_AUTOSPELL].val2==ALL_RESURRECTION && target->type != BL_PC)) && battle_check_undead(race,ele))
								f = skill_castend_damage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
							else
								f = skill_castend_nodamage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
							break;
					}
				}
				if(!f) pc_heal(sd,0,-sp);
			}
		} else {
			if((skill_get_inf(sc_data[SC_AUTOSPELL].val2) == 2) || (skill_get_inf(sc_data[SC_AUTOSPELL].val2) == 32))
				skill_castend_pos2(src,target->x,target->y,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
			else {
				switch (skill_get_nk(sc_data[SC_AUTOSPELL].val2)) {
					case 0:
					case 2:
						skill_castend_damage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
						break;
					case 1:/* �x���n */
						if((sc_data[SC_AUTOSPELL].val2==AL_HEAL || (sc_data[SC_AUTOSPELL].val2==ALL_RESURRECTION && target->type != BL_PC)) && battle_check_undead(race,ele))
							skill_castend_damage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
						else
							skill_castend_nodamage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
						break;
				}
			}
		}
	}
	

	//�J�[�h�ɂ��I�[�g�X�y��
	if(sd && sd->bl.type == BL_PC && src != target 	&& (wd.damage > 0 || wd.damage2 > 0))
	{
		//�I�[�g�X�y��
		skill_bonus_autospell(src,target,AS_ATTACK,0,0);
	}

	if(sd && sd->bl.type == BL_PC && src != target 	&&
			 wd.flag&BF_WEAPON && (wd.damage > 0 || wd.damage2 > 0))
	{
		//SP����
		if(target && target->type == BL_PC && atn_rand()%100 < sd->sp_vanish_rate)
		{
			int sp = 0;
			sp = status_get_sp(target)* sd->sp_vanish_per/100;
			if (sp > 0)
				pc_heal((struct map_session_data*) target, 0, -sp);
		}
	}
		
	if(sd){	
		if (wd.flag&BF_WEAPON && src != target && (wd.damage > 0 || wd.damage2 > 0)) {
			int hp = 0,sp = 0;
			if (!battle_config.left_cardfix_to_right) { // �񓁗�����J�[�h�̋z���n���ʂ��E��ɒǉ����Ȃ��ꍇ
				hp += battle_calc_drain(wd.damage, sd->hp_drain_rate, sd->hp_drain_per, sd->hp_drain_value);
				hp += battle_calc_drain(wd.damage2, sd->hp_drain_rate_, sd->hp_drain_per_, sd->hp_drain_value_);
				sp += battle_calc_drain(wd.damage, sd->sp_drain_rate, sd->sp_drain_per, sd->sp_drain_value);
				sp += battle_calc_drain(wd.damage2, sd->sp_drain_rate_, sd->sp_drain_per_, sd->sp_drain_value_);
			} else { // �񓁗�����J�[�h�̋z���n���ʂ��E��ɒǉ�����ꍇ
				int hp_drain_rate = sd->hp_drain_rate + sd->hp_drain_rate_;
				int hp_drain_per = sd->hp_drain_per + sd->hp_drain_per_;
				int hp_drain_value = sd->hp_drain_value + sd->hp_drain_value_;
				int sp_drain_rate = sd->sp_drain_rate + sd->sp_drain_rate_;
				int sp_drain_per = sd->sp_drain_per + sd->sp_drain_per_;
				int sp_drain_value = sd->sp_drain_value + sd->sp_drain_value_;
				hp += battle_calc_drain(wd.damage, hp_drain_rate, hp_drain_per, hp_drain_value);
				sp += battle_calc_drain(wd.damage, sp_drain_rate, sp_drain_per, sp_drain_value);
			}

			if (hp || sp) pc_heal(sd, hp, sp);
		}
	}

	if(rdamage > 0)
	{
		battle_delay_damage(tick+wd.amotion,target,src,rdamage,0);
		
		//���˃_���[�W�̃I�[�g�X�y��
		if(battle_config.weapon_reflect_autospell && target->type == BL_PC)
		{
			skill_bonus_autospell(target,src,AS_ATTACK,0,0);
		}
		
		if(battle_config.weapon_reflect_drain)
			battle_attack_drain(target,src,rdamage,0,battle_config.weapon_reflect_drain_per_enable);
	}
	if(t_sc_data && t_sc_data[SC_AUTOCOUNTER].timer != -1 && t_sc_data[SC_AUTOCOUNTER].val4 > 0) {
		if(t_sc_data[SC_AUTOCOUNTER].val3 == src->id)
			battle_weapon_attack(target,src,tick,0x8000|t_sc_data[SC_AUTOCOUNTER].val1);
		status_change_end(target,SC_AUTOCOUNTER,-1);
	}
	if (t_sc_data && t_sc_data[SC_BLADESTOP_WAIT].timer != -1 &&
			!(status_get_mode(src)&0x20)) { // �{�X�ɂ͖���
		int lv = t_sc_data[SC_BLADESTOP_WAIT].val1;
		status_change_end(target,SC_BLADESTOP_WAIT,-1);
		status_change_start(src,SC_BLADESTOP,lv,1,(int)src,(int)target,skill_get_time2(MO_BLADESTOP,lv),0);
		status_change_start(target,SC_BLADESTOP,lv,2,(int)target,(int)src,skill_get_time2(MO_BLADESTOP,lv),0);
	}

	map_freeblock_unlock();
	return wd.dmg_lv;
}

int battle_check_undead(int race,int element)
{
	if(battle_config.undead_detect_type == 0) {
		if(element == 9)
			return 1;
	}
	else if(battle_config.undead_detect_type == 1) {
		if(race == 1)
			return 1;
	}
	else {
		if(element == 9 || race == 1)
			return 1;
	}
	return 0;
}

/*==========================================
 * �G��������(1=�m��,0=�ے�,-1=�G���[)
 * flag&0xf0000 = 0x00000:�G����Ȃ�������iret:1���G�ł͂Ȃ��j
 *				= 0x10000:�p�[�e�B�[����iret:1=�p�[�e�B�[�����o)
 *				= 0x20000:�S��(ret:1=�G��������)
 *				= 0x40000:�G������(ret:1=�G)
 *				= 0x50000:�p�[�e�B�[����Ȃ�������(ret:1=�p�[�e�B�łȂ�)
 *------------------------------------------
 */
int battle_check_target( struct block_list *src, struct block_list *target,int flag)
{
	int s_p,s_g,t_p,t_g;
	struct block_list *ss=src;

	nullpo_retr(0, src);
	nullpo_retr(0, target);

	if( flag&0x40000 ){	// ���]�t���O
		int ret=battle_check_target(src,target,flag&0x30000);
		if(ret!=-1)
			return !ret;
		return -1;
	}

	if( flag&0x20000 ){
		if( target->type==BL_MOB || target->type==BL_PC )
			return 1;
		else
			return -1;
	}
	
	if(src->type == BL_SKILL && target->type == BL_SKILL)	// �Ώۂ��X�L�����j�b�g�Ȃ疳�����m��
		return -1;

	if(target->type == BL_PC && ((struct map_session_data *)target)->invincible_timer != -1)
		return -1;

	if(target->type == BL_SKILL) {
		switch(((struct skill_unit *)target)->group->unit_id){
		case 0x8d:
		case 0x8f:
		case 0x98:
			return 0;
			break;
		}
	}

	if(target->type == BL_PET)
		return -1;

	// �X�L�����j�b�g�̏ꍇ�A�e�����߂�
	if( src->type==BL_SKILL) {
		int inf2 = skill_get_inf2(((struct skill_unit *)src)->group->skill_id);
		if( (ss=map_id2bl( ((struct skill_unit *)src)->group->src_id))==NULL )
			return -1;
		if(ss->prev == NULL)
			return -1;
		if(inf2&0x1000 && map[src->m].flag.gvg && !(target->type == BL_PC && pc_isinvisible((struct map_session_data *)target)))
			return 0;
		if(inf2&0x80	&& map[src->m].flag.pvp && !(target->type == BL_PC && pc_isinvisible((struct map_session_data *)target)))
			return 0;
		if(ss == target) {
			if(inf2&0x100)
				return 0;
			if(inf2&0x200)
				return -1;
		}
	}
	// Mob��master_id��������special_mob_ai�Ȃ�A����������߂�
	if( src->type==BL_MOB ){
		struct mob_data *md=(struct mob_data *)src;
		if(md && md->master_id>0){
			if(md->master_id==target->id)	// ��Ȃ�m��
				return 1;
			if(md->state.special_mob_ai){
				if(target->type==BL_MOB){	//special_mob_ai�őΏۂ�Mob
					struct mob_data *tmd=(struct mob_data *)target;
					if(tmd){
						if(tmd->master_id != md->master_id)	//�����傪�ꏏ�łȂ���Δے�
							return 0;
						else{	//�����傪�ꏏ�Ȃ̂ōm�肵�������ǎ����͔ے�
							if(md->state.special_mob_ai>2)
								return 0;
							else
								return 1;
						}
					}
				}
			}
			if((ss=map_id2bl(md->master_id))==NULL)
				return -1;
		}
	}

	if( src==target || ss==target )	// �����Ȃ�m��
		return 1;

	if(target->type == BL_PC && pc_isinvisible((struct map_session_data *)target))
		return -1;

	if( src->prev==NULL ||	// ����ł�Ȃ�G���[
		(src->type==BL_PC && pc_isdead((struct map_session_data *)src) ) )
		return -1;

	if( (ss->type == BL_PC && target->type==BL_MOB) ||
		(ss->type == BL_MOB && target->type==BL_PC) )
		return 0;	// PCvsMOB�Ȃ�ے�

	if(ss->type == BL_PET && target->type==BL_MOB)
		return 0;

	s_p=status_get_party_id(ss);
	s_g=status_get_guild_id(ss);

	t_p=status_get_party_id(target);
	t_g=status_get_guild_id(target);

	if(flag&0x10000) {
		if(s_p && t_p && s_p == t_p)	// �����p�[�e�B�Ȃ�m��i�����j
			return 1;
		else		// �p�[�e�B�����Ȃ瓯���p�[�e�B����Ȃ����_�Ŕے�
			return 0;
	}

	if(ss->type == BL_MOB && s_g > 0 && t_g > 0 && s_g == t_g )	// �����M���h/mob�N���X�Ȃ�m��i�����j
		return 1;

//printf("ss:%d src:%d target:%d flag:0x%x %d %d ",ss->id,src->id,target->id,flag,src->type,target->type);
//printf("p:%d %d g:%d %d\n",s_p,t_p,s_g,t_g);

	if( ss->type==BL_PC && target->type==BL_PC) { // ����PVP���[�h�Ȃ�ے�i�G�j
		struct skill_unit *su=NULL;
		if(src->type==BL_SKILL)
			su=(struct skill_unit *)src;
		if(map[ss->m].flag.pvp) {
			if(su && su->group->target_flag==BCT_NOENEMY)
				return 1;
			if(map[ss->m].flag.pvp_noparty && s_p > 0 && t_p > 0 && s_p == t_p)
				return 1;
			else if(map[ss->m].flag.pvp_noguild && s_g > 0 && t_g > 0 && s_g == t_g)
				return 1;
			return 0;
		}
		if(map[src->m].flag.gvg) {
			struct guild *g=NULL;
			if(su && su->group->target_flag==BCT_NOENEMY)
				return 1;
			if( s_g > 0 && s_g == t_g)
				return 1;
			if(map[src->m].flag.gvg_noparty && s_p > 0 && t_p > 0 && s_p == t_p)
				return 1;
			if((g = guild_search(s_g)) !=NULL) {
				int i;
				for(i=0;i<MAX_GUILDALLIANCE;i++){
					if(g->alliance[i].guild_id > 0 && g->alliance[i].guild_id == t_g) {
						if(g->alliance[i].opposition)
							return 0;//�G�΃M���h�Ȃ疳�����ɓG
						else
							return 1;//�����M���h�Ȃ疳�����ɖ���
					}
				}
			}
			return 0;
		}
	}

	return 1;	// �Y�����Ȃ��̂Ŗ��֌W�l���i�܂��G����Ȃ��̂Ŗ����j
}
/*==========================================
 * �˒�����
 *------------------------------------------
 */
int battle_check_range(struct block_list *src,struct block_list *bl,int range)
{

	int dx,dy;
	int arange;

	nullpo_retr(0, src);
	nullpo_retr(0, bl);
	
	dx=abs(bl->x-src->x);
	dy=abs(bl->y-src->y);
	arange=((dx>dy)?dx:dy);

	if(src->m != bl->m)	// �Ⴄ�}�b�v
		return 0;
	
	if( range>0 && range < arange )	// ��������
		return 0;

	if( arange<2 )	// �����}�X���א�
		return 1;

//	if(bl->type == BL_SKILL && ((struct skill_unit *)bl)->group->unit_id == 0x8d)
//		return 1;

	// ��Q������
	return path_search_long(NULL,src->m,src->x,src->y,bl->x,bl->y);
}

/*==========================================
 * �ݒ�t�@�C���ǂݍ��ݗp�i�t���O�j
 *------------------------------------------
 */
int battle_config_switch(const char *str)
{
	if(strcmpi(str,"on")==0 || strcmpi(str,"yes")==0)
		return 1;
	if(strcmpi(str,"off")==0 || strcmpi(str,"no")==0)
		return 0;
	return atoi(str);
}
/*==========================================
 * �ݒ�t�@�C����ǂݍ���
 *------------------------------------------
 */
int battle_config_read(const char *cfgName)
{
	int i;
	char line[1024],w1[1024],w2[1024];
	FILE *fp;
	static int count=0;
	
	if( (count++)==0 ){

		battle_config.warp_point_debug=0;
		battle_config.enemy_critical=0;
		battle_config.enemy_critical_rate=100;
		battle_config.enemy_str=1;
		battle_config.enemy_perfect_flee=0;
		battle_config.cast_rate=100;
		battle_config.delay_rate=100;
		battle_config.delay_dependon_dex=0;
		battle_config.sdelay_attack_enable=0;
		battle_config.left_cardfix_to_right=0;
		battle_config.pc_skill_add_range=0;
		battle_config.skill_out_range_consume=1;
		battle_config.mob_skill_add_range=0;
		battle_config.pc_damage_delay=1;
		battle_config.pc_damage_delay_rate=100;
		battle_config.defnotenemy=1;
		battle_config.random_monster_checklv=1;
		battle_config.attr_recover=1;
		battle_config.flooritem_lifetime=LIFETIME_FLOORITEM*1000;
		battle_config.item_auto_get=0;
		battle_config.item_first_get_time=3000;
		battle_config.item_second_get_time=1000;
		battle_config.item_third_get_time=1000;
		battle_config.mvp_item_first_get_time=10000;
		battle_config.mvp_item_second_get_time=10000;
		battle_config.mvp_item_third_get_time=2000;
		battle_config.item_rate=100;
		battle_config.drop_rate0item=0;
		battle_config.base_exp_rate=100;
		battle_config.job_exp_rate=100;
		battle_config.death_penalty_type=0;
		battle_config.death_penalty_base=0;
		battle_config.death_penalty_job=0;
		battle_config.zeny_penalty=0;
		battle_config.restart_hp_rate=0;
		battle_config.restart_sp_rate=0;
		battle_config.mvp_item_rate=100;
		battle_config.mvp_exp_rate=100;
		battle_config.mvp_hp_rate=100;
		battle_config.monster_hp_rate=100;
		battle_config.monster_max_aspd=199;
		battle_config.atc_gmonly=0;
		battle_config.gm_allskill=0;
		battle_config.gm_allskill_addabra=0;
		battle_config.gm_allequip=0;
		battle_config.gm_skilluncond=0;
		battle_config.skillfree = 0;
		battle_config.skillup_limit = 0;
		battle_config.wp_rate=100;
		battle_config.pp_rate=100;
		battle_config.cdp_rate=100;
		battle_config.monster_active_enable=1;
		battle_config.monster_damage_delay_rate=100;
		battle_config.monster_loot_type=0;
		battle_config.mob_skill_use=1;
		battle_config.mob_count_rate=100;
		battle_config.mob_delay_rate=100;
		battle_config.quest_skill_learn=0;
		battle_config.quest_skill_reset=1;
		battle_config.basic_skill_check=1;
		battle_config.guild_emperium_check=1;
		battle_config.guild_exp_limit=50;
		battle_config.pc_invincible_time = 5000;
		battle_config.pet_catch_rate=100;
		battle_config.pet_rename=0;
		battle_config.pet_friendly_rate=100;
		battle_config.pet_hungry_delay_rate=100;
		battle_config.pet_hungry_friendly_decrease=5;
		battle_config.pet_str=1;
		battle_config.pet_status_support=0;
		battle_config.pet_attack_support=0;
		battle_config.pet_damage_support=0;
		battle_config.pet_support_rate=100;
		battle_config.pet_attack_exp_to_master=0;
		battle_config.pet_attack_exp_rate=100;
		battle_config.skill_min_damage=0;
		battle_config.finger_offensive_type=0;
		battle_config.heal_exp=0;
		battle_config.resurrection_exp=0;
		battle_config.shop_exp=0;
		battle_config.combo_delay_rate=100;
		battle_config.item_check=1;
		battle_config.wedding_relog=1;
		battle_config.wedding_time=3600000;
		battle_config.wedding_modifydisplay=0;
		battle_config.natural_healhp_interval=6000;
		battle_config.natural_healsp_interval=8000;
		battle_config.natural_heal_skill_interval=10000;
		battle_config.natural_heal_weight_rate=50;
		battle_config.item_name_override_grffile=1;
		battle_config.arrow_decrement=1;
		battle_config.max_aspd = 199;
		battle_config.max_hp = 32500;
		battle_config.max_sp = 32500;
		battle_config.max_parameter = 99;
		battle_config.max_cart_weight = 8000;
		battle_config.pc_skill_log = 0;
		battle_config.mob_skill_log = 0;
		battle_config.battle_log = 0;
		battle_config.save_log = 0;
		battle_config.error_log = 1;
		battle_config.etc_log = 1;
		battle_config.save_clothcolor = 0;
		battle_config.undead_detect_type = 0;
		battle_config.pc_auto_counter_type = 1;
		battle_config.monster_auto_counter_type = 1;
		battle_config.agi_penaly_type = 0;
		battle_config.agi_penaly_count = 3;
		battle_config.agi_penaly_num = 0;
		battle_config.agi_penaly_count_lv = ATK_FLEE;
		battle_config.vit_penaly_type = 0;
		battle_config.vit_penaly_count = 3;
		battle_config.vit_penaly_num = 0;
		battle_config.vit_penaly_count_lv = ATK_DEF;
		battle_config.player_defense_type = 0;
		battle_config.monster_defense_type = 0;
		battle_config.pet_defense_type = 0;
		battle_config.magic_defense_type = 0;
		battle_config.pc_skill_reiteration = 0;
		battle_config.monster_skill_reiteration = 0;
		battle_config.pc_skill_nofootset = 0;
		battle_config.monster_skill_nofootset = 0;
		battle_config.pc_cloak_check_type = 0;
		battle_config.monster_cloak_check_type = 0;
		battle_config.gvg_short_damage_rate = 100;
		battle_config.gvg_long_damage_rate = 100;
		battle_config.gvg_magic_damage_rate = 100;
		battle_config.gvg_misc_damage_rate = 100;
		battle_config.gvg_eliminate_time = 7000;
		battle_config.mob_changetarget_byskill = 0;
		battle_config.pc_attack_direction_change = 1;
		battle_config.monster_attack_direction_change = 1;
		battle_config.pc_land_skill_limit = 1;
		battle_config.monster_land_skill_limit = 1;
		battle_config.party_skill_penaly = 1;
		battle_config.monster_class_change_full_recover = 0;
		battle_config.produce_item_name_input = 1;
		battle_config.produce_potion_name_input = 1;
		battle_config.making_arrow_name_input = 1;
		battle_config.holywater_name_input = 1;
		battle_config.display_delay_skill_fail = 1;
		battle_config.display_snatcher_skill_fail = 1;
		battle_config.chat_warpportal = 0;
		battle_config.mob_warpportal = 0;
		battle_config.dead_branch_active = 0;
		battle_config.vending_max_value = 10000000;
		battle_config.pet_lootitem = 0;
		battle_config.pet_weight = 1000;
		battle_config.show_steal_in_same_party = 0;
		battle_config.enable_upper_class = 0;
		battle_config.pet_attack_attr_none = 0;
		battle_config.pc_attack_attr_none = 0;
		battle_config.mob_attack_attr_none = 1;
		battle_config.gx_allhit = 0;
		battle_config.gx_cardfix = 0;
		battle_config.gx_dupele = 1;
		battle_config.gx_disptype = 1;
		battle_config.devotion_level_difference = 10;
		battle_config.player_skill_partner_check = 1;
		battle_config.hide_GM_session = 0;
		battle_config.unit_movement_type = 0;
		battle_config.invite_request_check = 1;
		battle_config.skill_removetrap_type = 0;
		battle_config.disp_experience = 0;
		battle_config.castle_defense_rate = 100;
		battle_config.riding_weight = 0;
		battle_config.hp_rate = 100;
		battle_config.sp_rate = 100;
		battle_config.gm_can_drop_lv = 0;
		battle_config.disp_hpmeter = 0;
		battle_config.bone_drop = 0;
		battle_config.bone_drop_itemid = 7005;
		battle_config.item_rate_details = 0;
		battle_config.item_rate_1 = 100;
		battle_config.item_rate_10 = 100;
		battle_config.item_rate_100 = 100;
		battle_config.item_rate_1000 = 100;
		battle_config.item_rate_1_min = 1;
		battle_config.item_rate_1_max = 9;
		battle_config.item_rate_10_min = 10;
		battle_config.item_rate_10_max = 99;
		battle_config.item_rate_100_min = 100;
		battle_config.item_rate_100_max = 999;
		battle_config.item_rate_1000_min = 1000;
		battle_config.item_rate_1000_max = 10000;
		battle_config.monster_damage_delay = 1;
		battle_config.card_drop_rate = 100;
		battle_config.equip_drop_rate = 100;
		battle_config.refine_drop_rate = 100;
		battle_config.Item_res = 1;
		battle_config.next_exp_limit = 150;
		battle_config.heal_counterstop = 11;
		battle_config.finding_ore_drop_rate = 100;
		battle_config.no_spel_dex1 = 0;
		battle_config.no_spel_dex2 = 0;
		battle_config.pt_bonus_b = 0;
		battle_config.pt_bonus_j = 0;
		battle_config.equip_autospell_nocost = 0;
		battle_config.limit_gemstone = 0;
		battle_config.mpv_announce = 0;
		battle_config.petowneditem = 0;
		battle_config.buyer_name = 0;
		battle_config.expand_autospell = 0;
		battle_config.once_autospell = 1;
		battle_config.allow_same_autospell = 0;
		battle_config.combo_delay_lower_limits = 0;
		battle_config.new_marrige_skill = 0;
		battle_config.reveff_plus_addeff = 0;
		battle_config.summonslave_no_drop = 0;
		battle_config.summonslave_no_exp = 0;
		battle_config.summonslave_no_mvp = 0;
		battle_config.cannibalize_no_drop = 0;
		battle_config.cannibalize_no_exp = 0;
		battle_config.cannibalize_no_mvp = 0;
		battle_config.branch_mob_no_drop = 0;
		battle_config.branch_mob_no_exp = 0;
		battle_config.branch_mob_no_mvp = 0;
		battle_config.branch_boss_no_drop = 0;
		battle_config.branch_boss_no_exp = 0;
		battle_config.branch_boss_no_mvp = 0;
		battle_config.pc_hit_stop_type = 3;
		battle_config.nomanner_mode = 0;
		battle_config.death_by_unrig_penalty = 0;
		battle_config.dance_and_play_duration = 20000;
		battle_config.soulcollect_max_fail = 0;
		battle_config.gvg_flee_rate	= 100;
		battle_config.gvg_flee_penaly	= 0;
		battle_config.equip_sex = 0;
		battle_config.noexp_hiding = 0;
		battle_config.noexp_trickdead = 0;
		battle_config.gm_hide_attack_lv = 1;
		battle_config.hide_attack = 0;
		battle_config.weapon_attack_autospell = 0;
		battle_config.magic_attack_autospell = 0;
		battle_config.misc_attack_autospell = 0;
		battle_config.magic_attack_drain = 0;
		battle_config.misc_attack_drain = 0;
		battle_config.magic_attack_drain_per_enable = 0;
		battle_config.misc_attack_drain_per_enable = 0;
		battle_config.hallucianation_off = 0;
		battle_config.weapon_reflect_autospell = 0;
		battle_config.magic_reflect_autospell = 0;
		battle_config.weapon_reflect_drain = 0;
		battle_config.weapon_reflect_drain_per_enable = 0;
		battle_config.magic_reflect_drain = 0;
		battle_config.magic_reflect_drain_per_enable = 0;
		battle_config.extended_cloneskill	= 0;
		battle_config.max_parameter_str	= 999;
		battle_config.max_parameter_agi	= 999;
		battle_config.max_parameter_vit	= 999;
		battle_config.max_parameter_int	= 999;
		battle_config.max_parameter_dex	= 999;
		battle_config.max_parameter_luk	= 999;
		battle_config.cannibalize_nocost	= 0;
		battle_config.spheremine_nocost	= 0;
		battle_config.demonstration_nocost	= 0;
		battle_config.acidterror_nocost	= 0;
		battle_config.aciddemonstration_nocost	= 0;
		battle_config.chemical_nocost	= 0;
		battle_config.slimpitcher_nocost	= 0;
		battle_config.mes_send_type = 0;
		battle_config.allow_assumptop_in_gvg = 1;
		battle_config.allow_falconassault_elemet = 0;
		battle_config.allow_guild_invite_in_gvg = 1;
		battle_config.allow_guild_leave_in_gvg  = 1;
		battle_config.guild_skill_available   = 1;
		battle_config.guild_hunting_skill_available = 1;
		battle_config.guild_skill_check_range = 0;
		battle_config.allow_guild_skill_in_gvg_only = 1;
		battle_config.allow_me_guild_skill = 0;
		battle_config.emergencycall_point_randam = 0;
		battle_config.emergencycall_call_limit = 0;
		battle_config.allow_guild_skill_in_gvgtime_only = 0;
		battle_config.guild_skill_in_pvp_limit = 1;
		battle_config.guild_exp_rate = 100;
		battle_config.guild_skill_effective_range = 2;
		battle_config.tarotcard_display_position = 2;
		battle_config.serverside_friendlist = 1;
		battle_config.pet0078_hair_id = 24;
		battle_config.job_soul_check = 1;
		battle_config.repeal_die_counter_rate = 100;
		battle_config.disp_job_soul_state_change = 1;
		battle_config.check_knowlege_map = 0;
		battle_config.tripleattack_rate_up_keeptime = 2000;
		battle_config.tk_counter_rate_up_keeptime = 2000;
		battle_config.allow_skill_without_day = 1;
		battle_config.debug_new_disp_status_icon_system = 0;
	}
	
	fp=fopen(cfgName,"r");
	if(fp==NULL){
		printf("file not found: %s\n",cfgName);
		return 1;
	}
	while(fgets(line,1020,fp)){
		const struct {
			char str[128];
			int *val;
		} data[] ={
			{ "warp_point_debug",					&battle_config.warp_point_debug						},
			{ "enemy_critical",						&battle_config.enemy_critical						},
			{ "enemy_critical_rate",				&battle_config.enemy_critical_rate					},
			{ "enemy_str",							&battle_config.enemy_str							},
			{ "enemy_perfect_flee",					&battle_config.enemy_perfect_flee					},
			{ "casting_rate",						&battle_config.cast_rate							},
			{ "delay_rate",							&battle_config.delay_rate							},
			{ "delay_dependon_dex",					&battle_config.delay_dependon_dex					},
			{ "skill_delay_attack_enable",			&battle_config.sdelay_attack_enable 				},
			{ "left_cardfix_to_right",				&battle_config.left_cardfix_to_right				},
			{ "player_skill_add_range",				&battle_config.pc_skill_add_range					},
			{ "skill_out_range_consume",			&battle_config.skill_out_range_consume				},
			{ "monster_skill_add_range",			&battle_config.mob_skill_add_range					},
			{ "player_damage_delay",				&battle_config.pc_damage_delay						},
			{ "player_damage_delay_rate",			&battle_config.pc_damage_delay_rate					},
			{ "defunit_not_enemy",					&battle_config.defnotenemy							},
			{ "random_monster_checklv",				&battle_config.random_monster_checklv				},
			{ "attribute_recover",					&battle_config.attr_recover							},
			{ "flooritem_lifetime",					&battle_config.flooritem_lifetime					},
			{ "item_auto_get",						&battle_config.item_auto_get						},
			{ "item_first_get_time",				&battle_config.item_first_get_time					},
			{ "item_second_get_time",				&battle_config.item_second_get_time					},
			{ "item_third_get_time",				&battle_config.item_third_get_time					},
			{ "mvp_item_first_get_time",			&battle_config.mvp_item_first_get_time				},
			{ "mvp_item_second_get_time",			&battle_config.mvp_item_second_get_time				},
			{ "mvp_item_third_get_time",			&battle_config.mvp_item_third_get_time				},
			{ "item_rate",							&battle_config.item_rate							},
			{ "drop_rate0item",						&battle_config.drop_rate0item						},
			{ "base_exp_rate",						&battle_config.base_exp_rate						},
			{ "job_exp_rate",						&battle_config.job_exp_rate							},
			{ "death_penalty_type",					&battle_config.death_penalty_type					},
			{ "death_penalty_base",					&battle_config.death_penalty_base					},
			{ "death_penalty_job",					&battle_config.death_penalty_job					},
			{ "zeny_penalty",						&battle_config.zeny_penalty							},
			{ "restart_hp_rate",					&battle_config.restart_hp_rate						},
			{ "restart_sp_rate",					&battle_config.restart_sp_rate						},
			{ "mvp_hp_rate",						&battle_config.mvp_hp_rate							},
			{ "mvp_item_rate",						&battle_config.mvp_item_rate						},
			{ "mvp_exp_rate",						&battle_config.mvp_exp_rate							},
			{ "monster_hp_rate",					&battle_config.monster_hp_rate						},
			{ "monster_max_aspd",					&battle_config.monster_max_aspd						},
			{ "atcommand_gm_only",					&battle_config.atc_gmonly							},
			{ "gm_all_skill",						&battle_config.gm_allskill							},
			{ "gm_all_skill_add_abra",				&battle_config.gm_allskill_addabra					},
			{ "gm_all_equipment",					&battle_config.gm_allequip							},
			{ "gm_skill_unconditional",				&battle_config.gm_skilluncond						},
			{ "player_skillfree",					&battle_config.skillfree							},
			{ "player_skillup_limit",				&battle_config.skillup_limit						},
			{ "weapon_produce_rate",				&battle_config.wp_rate								},
			{ "potion_produce_rate",				&battle_config.pp_rate								},
			{ "deadly_potion_produce_rate",			&battle_config.cdp_rate								},
			{ "monster_active_enable",				&battle_config.monster_active_enable				},
			{ "monster_damage_delay_rate",			&battle_config.monster_damage_delay_rate			},
			{ "monster_loot_type",					&battle_config.monster_loot_type					},
			{ "mob_skill_use",						&battle_config.mob_skill_use						},
			{ "mob_count_rate",						&battle_config.mob_count_rate						},
			{ "mob_delay_rate",						&battle_config.mob_delay_rate						},
			{ "quest_skill_learn",					&battle_config.quest_skill_learn					},
			{ "quest_skill_reset",					&battle_config.quest_skill_reset					},
			{ "basic_skill_check",					&battle_config.basic_skill_check					},
			{ "guild_emperium_check",				&battle_config.guild_emperium_check					},
			{ "guild_exp_limit",					&battle_config.guild_exp_limit						},
			{ "player_invincible_time" ,			&battle_config.pc_invincible_time					},
			{ "pet_catch_rate",						&battle_config.pet_catch_rate						},
			{ "pet_rename",							&battle_config.pet_rename							},
			{ "pet_friendly_rate",					&battle_config.pet_friendly_rate					},
			{ "pet_hungry_delay_rate",				&battle_config.pet_hungry_delay_rate				},
			{ "pet_hungry_friendly_decrease",		&battle_config.pet_hungry_friendly_decrease			},
			{ "pet_str",							&battle_config.pet_str								},
			{ "pet_status_support",					&battle_config.pet_status_support					},
			{ "pet_attack_support",					&battle_config.pet_attack_support					},
			{ "pet_damage_support",					&battle_config.pet_damage_support					},
			{ "pet_support_rate",					&battle_config.pet_support_rate						},
			{ "pet_attack_exp_to_master",			&battle_config.pet_attack_exp_to_master				},
			{ "pet_attack_exp_rate",				&battle_config.pet_attack_exp_rate					},
			{ "skill_min_damage",					&battle_config.skill_min_damage						},
			{ "finger_offensive_type",				&battle_config.finger_offensive_type				},
			{ "heal_exp",							&battle_config.heal_exp								},
			{ "resurrection_exp",					&battle_config.resurrection_exp						},
			{ "shop_exp",							&battle_config.shop_exp								},
			{ "combo_delay_rate",					&battle_config.combo_delay_rate						},
			{ "item_check",							&battle_config.item_check							},
			{ "wedding_relog",						&battle_config.wedding_relog						},
			{ "wedding_time",						&battle_config.wedding_time							},
			{ "wedding_modifydisplay",				&battle_config.wedding_modifydisplay				},
			{ "natural_healhp_interval",			&battle_config.natural_healhp_interval				},
			{ "natural_healsp_interval",			&battle_config.natural_healsp_interval				},
			{ "natural_heal_skill_interval",		&battle_config.natural_heal_skill_interval			},
			{ "natural_heal_weight_rate",			&battle_config.natural_heal_weight_rate				},
			{ "item_name_override_grffile",			&battle_config.item_name_override_grffile			},
			{ "arrow_decrement",					&battle_config.arrow_decrement						},
			{ "max_aspd",							&battle_config.max_aspd								},
			{ "max_hp",								&battle_config.max_hp								},
			{ "max_sp",								&battle_config.max_sp								},
			{ "max_parameter", 						&battle_config.max_parameter						},
			{ "max_cart_weight",					&battle_config.max_cart_weight						},
			{ "player_skill_log",					&battle_config.pc_skill_log							},
			{ "monster_skill_log",					&battle_config.mob_skill_log						},
			{ "battle_log",							&battle_config.battle_log							},
			{ "save_log",							&battle_config.save_log								},
			{ "error_log",							&battle_config.error_log							},
			{ "etc_log",							&battle_config.etc_log								},
			{ "save_clothcolor",					&battle_config.save_clothcolor						},
			{ "undead_detect_type",					&battle_config.undead_detect_type					},
			{ "player_auto_counter_type",			&battle_config.pc_auto_counter_type					},
			{ "monster_auto_counter_type",			&battle_config.monster_auto_counter_type			},
			{ "agi_penaly_type",					&battle_config.agi_penaly_type						},
			{ "agi_penaly_count",					&battle_config.agi_penaly_count						},
			{ "agi_penaly_num",						&battle_config.agi_penaly_num						},
			{ "agi_penaly_count_lv",				&battle_config.agi_penaly_count_lv					},
			{ "vit_penaly_type",					&battle_config.vit_penaly_type						},
			{ "vit_penaly_count",					&battle_config.vit_penaly_count						},
			{ "vit_penaly_num",						&battle_config.vit_penaly_num						},
			{ "vit_penaly_count_lv",				&battle_config.vit_penaly_count_lv					},
			{ "player_defense_type",				&battle_config.player_defense_type					},
			{ "monster_defense_type",				&battle_config.monster_defense_type					},
			{ "pet_defense_type",					&battle_config.pet_defense_type						},
			{ "magic_defense_type",					&battle_config.magic_defense_type					},
			{ "player_skill_reiteration",			&battle_config.pc_skill_reiteration					},
			{ "monster_skill_reiteration",			&battle_config.monster_skill_reiteration			},
			{ "player_skill_nofootset",				&battle_config.pc_skill_nofootset					},
			{ "monster_skill_nofootset",			&battle_config.monster_skill_nofootset				},
			{ "player_cloak_check_type",			&battle_config.pc_cloak_check_type					},
			{ "monster_cloak_check_type",			&battle_config.monster_cloak_check_type				},
			{ "gvg_short_attack_damage_rate",		&battle_config.gvg_short_damage_rate				},
			{ "gvg_long_attack_damage_rate",		&battle_config.gvg_long_damage_rate					},
			{ "gvg_magic_attack_damage_rate",		&battle_config.gvg_magic_damage_rate				},
			{ "gvg_misc_attack_damage_rate",		&battle_config.gvg_misc_damage_rate					},
			{ "gvg_eliminate_time",					&battle_config.gvg_eliminate_time					},
			{ "mob_changetarget_byskill",			&battle_config.mob_changetarget_byskill				},
			{ "player_attack_direction_change",		&battle_config.pc_attack_direction_change			},
			{ "monster_attack_direction_change",	&battle_config.monster_attack_direction_change		},
			{ "player_land_skill_limit",			&battle_config.pc_land_skill_limit					},
			{ "monster_land_skill_limit",			&battle_config.monster_land_skill_limit				},
			{ "party_skill_penaly",					&battle_config.party_skill_penaly					},
			{ "monster_class_change_full_recover",	&battle_config.monster_class_change_full_recover	},
			{ "produce_item_name_input",			&battle_config.produce_item_name_input				},
			{ "produce_potion_name_input",			&battle_config.produce_potion_name_input			},
			{ "making_arrow_name_input",			&battle_config.making_arrow_name_input				},
			{ "holywater_name_input",				&battle_config.holywater_name_input					},
			{ "display_delay_skill_fail",			&battle_config.display_delay_skill_fail				},
			{ "display_snatcher_skill_fail",		&battle_config.display_snatcher_skill_fail			},
			{ "chat_warpportal",					&battle_config.chat_warpportal						},
			{ "mob_warpportal",						&battle_config.mob_warpportal						},
			{ "dead_branch_active",					&battle_config.dead_branch_active					},
			{ "vending_max_value",					&battle_config.vending_max_value					},
			{ "pet_lootitem",						&battle_config.pet_lootitem							},
			{ "pet_weight",							&battle_config.pet_weight							},
			{ "show_steal_in_same_party",			&battle_config.show_steal_in_same_party				},
			{ "enable_upper_class", 				&battle_config.enable_upper_class					},
			{ "pet_attack_attr_none", 				&battle_config.pet_attack_attr_none					},
			{ "mob_attack_attr_none", 				&battle_config.mob_attack_attr_none					},
			{ "pc_attack_attr_none", 				&battle_config.pc_attack_attr_none					},
			{ "gx_allhit", 							&battle_config.gx_allhit							},
			{ "gx_cardfix",							&battle_config.gx_cardfix							},
			{ "gx_dupele", 							&battle_config.gx_dupele							},
			{ "gx_disptype", 						&battle_config.gx_disptype							},
			{ "devotion_level_difference",			&battle_config.devotion_level_difference			},
			{ "player_skill_partner_check",			&battle_config.player_skill_partner_check			},
			{ "hide_GM_session",					&battle_config.hide_GM_session						},
			{ "unit_movement_type",					&battle_config.unit_movement_type					},
			{ "invite_request_check",				&battle_config.invite_request_check					},
			{ "skill_removetrap_type",				&battle_config.skill_removetrap_type				},
			{ "disp_experience",					&battle_config.disp_experience						},
			{ "castle_defense_rate",				&battle_config.castle_defense_rate					},
			{ "riding_weight",						&battle_config.riding_weight						},
			{ "hp_rate",							&battle_config.hp_rate								},
			{ "sp_rate",							&battle_config.sp_rate								},
			{ "gm_can_drop_lv",						&battle_config.gm_can_drop_lv						},
			{ "disp_hpmeter",						&battle_config.disp_hpmeter							},
			{ "bone_drop",							&battle_config.bone_drop							},
			{ "bone_drop_itemid",					&battle_config.bone_drop_itemid						},
			{ "item_rate_details",					&battle_config.item_rate_details					},
			{ "item_rate_1",						&battle_config.item_rate_1							},
			{ "item_rate_10",						&battle_config.item_rate_10							},
			{ "item_rate_100",						&battle_config.item_rate_100						},
			{ "item_rate_1000",						&battle_config.item_rate_1000						},
			{ "item_rate_1_min",					&battle_config.item_rate_1_min						},
			{ "item_rate_1_max",					&battle_config.item_rate_1_max						},
			{ "item_rate_10_min",					&battle_config.item_rate_10_min						},
			{ "item_rate_10_max",					&battle_config.item_rate_10_max						},
			{ "item_rate_100_min",					&battle_config.item_rate_100_min					},
			{ "item_rate_100_max",					&battle_config.item_rate_100_max					},
			{ "item_rate_1000_min",					&battle_config.item_rate_1000_min					},
			{ "item_rate_1000_max",					&battle_config.item_rate_1000_max					},
			{ "monster_damage_delay",				&battle_config.monster_damage_delay					},
			{ "card_drop_rate",						&battle_config.card_drop_rate						},
			{ "equip_drop_rate",					&battle_config.equip_drop_rate						},
			{ "refine_drop_rate",					&battle_config.refine_drop_rate						},
			{ "Item_res",							&battle_config.Item_res								},
			{ "next_exp_limit",						&battle_config.next_exp_limit						},
			{ "heal_counterstop",					&battle_config.heal_counterstop						},
			{ "finding_ore_drop_rate",				&battle_config.finding_ore_drop_rate				},
			{ "no_spel_dex1",						&battle_config.no_spel_dex1							},
			{ "no_spel_dex2",						&battle_config.no_spel_dex2							},
			{ "pt_bonus_b",							&battle_config.pt_bonus_b							},
			{ "pt_bonus_j",							&battle_config.pt_bonus_j							},
			{ "equip_autospell_nocost",				&battle_config.equip_autospell_nocost				},
			{ "limit_gemstone",						&battle_config.limit_gemstone						},
			{ "mvp_announce",						&battle_config.mpv_announce							},
			{ "petowneditem",						&battle_config.petowneditem							},
			{ "buyer_name",							&battle_config.buyer_name							},
			{ "noportal_flag",						&battle_config.noportal_flag						},
			{ "expand_autospell",					&battle_config.expand_autospell						},
			{ "once_autospell",						&battle_config.once_autospell						},
			{ "allow_same_autospell",				&battle_config.allow_same_autospell					},
			{ "combo_delay_lower_limits",			&battle_config.combo_delay_lower_limits				},
			{ "new_marrige_skill",					&battle_config.new_marrige_skill					},
			{ "reveff_plus_addeff",					&battle_config.reveff_plus_addeff					},
			{ "summonslave_no_drop",				&battle_config.summonslave_no_drop					},
			{ "summonslave_no_exp",					&battle_config.summonslave_no_exp					},
			{ "summonslave_no_mvp",					&battle_config.summonslave_no_mvp					},
			{ "cannibalize_no_drop",				&battle_config.cannibalize_no_drop					},
			{ "cannibalize_no_exp",					&battle_config.cannibalize_no_exp					},
			{ "cannibalize_no_mvp",					&battle_config.cannibalize_no_mvp					},
			{ "branch_mob_no_drop",					&battle_config.branch_mob_no_drop					},
			{ "branch_mob_no_exp",					&battle_config.branch_mob_no_exp					},
			{ "branch_mob_no_mvp",					&battle_config.branch_mob_no_mvp					},
			{ "branch_boss_no_drop",				&battle_config.branch_boss_no_drop					},
			{ "branch_boss_no_exp",					&battle_config.branch_boss_no_exp					},
			{ "branch_boss_no_mvp",					&battle_config.branch_boss_no_mvp					},
			{ "pc_hit_stop_type",					&battle_config.pc_hit_stop_type						},
			{ "nomanner_mode",						&battle_config.nomanner_mode						},
			{ "death_by_unrig_penalty",				&battle_config.death_by_unrig_penalty				},
			{ "dance_and_play_duration",			&battle_config.dance_and_play_duration				},
			{ "soulcollect_max_fail",				&battle_config.soulcollect_max_fail					},
			{ "gvg_flee_rate",						&battle_config.gvg_flee_rate						},
			{ "gvg_flee_penaly",					&battle_config.gvg_flee_penaly						},
			{ "equip_sex",							&battle_config.equip_sex							},
			{ "noexp_hiding",						&battle_config.noexp_hiding							},
			{ "noexp_trickdead",					&battle_config.noexp_trickdead						},
			{ "hide_attack",						&battle_config.hide_attack							},
			{ "gm_hide_attack_lv",					&battle_config.gm_hide_attack_lv					},
			{ "weapon_attack_autospell",			&battle_config.weapon_attack_autospell				},
			{ "magic_attack_autospell",				&battle_config.magic_attack_autospell				},
			{ "misc_attack_autospell",				&battle_config.misc_attack_autospell				},
			{ "magic_attack_drain",					&battle_config.magic_attack_drain					},
			{ "misc_attack_drain",					&battle_config.misc_attack_drain					},
			{ "magic_attack_drain_per_enable",		&battle_config.magic_attack_drain_per_enable		},
			{ "misc_attack_drain_per_enable",		&battle_config.misc_attack_drain_per_enable			},
			{ "hallucianation_off",					&battle_config.hallucianation_off					},
			{ "weapon_reflect_autospell",			&battle_config.weapon_reflect_autospell				},
			{ "magic_reflect_autospell",			&battle_config.magic_reflect_autospell				},
			{ "weapon_reflect_drain",				&battle_config.weapon_reflect_drain					},
			{ "weapon_reflect_drain_per_enable",	&battle_config.weapon_reflect_drain_per_enable		},
			{ "magic_reflect_drain",				&battle_config.magic_reflect_drain					},
			{ "magic_reflect_drain_per_enable",		&battle_config.magic_reflect_drain_per_enable		},
			{ "extended_cloneskill",				&battle_config.extended_cloneskill					},
			{ "max_parameter_str",					&battle_config.max_parameter_str					},
			{ "max_parameter_agi",					&battle_config.max_parameter_agi					},
			{ "max_parameter_vit",					&battle_config.max_parameter_vit					},
			{ "max_parameter_int",					&battle_config.max_parameter_int					},
			{ "max_parameter_dex",					&battle_config.max_parameter_dex					},
			{ "max_parameter_luk",					&battle_config.max_parameter_luk					},
			{ "cannibalize_nocost",					&battle_config.cannibalize_nocost					},
			{ "spheremine_nocost",					&battle_config.spheremine_nocost					},
			{ "demonstration_nocost",				&battle_config.demonstration_nocost					},
			{ "acidterror_nocost",					&battle_config.acidterror_nocost					},
			{ "aciddemonstration_nocost",			&battle_config.aciddemonstration_nocost				},
			{ "chemical_nocost",					&battle_config.chemical_nocost						},
			{ "slimpitcher_nocost",					&battle_config.slimpitcher_nocost					},
			{ "mes_send_type",						&battle_config.mes_send_type						},
			{ "allow_assumptop_in_gvg",				&battle_config.allow_assumptop_in_gvg				},
			{ "allow_falconassault_elemet",			&battle_config.allow_falconassault_elemet			},
			{ "allow_guild_invite_in_gvg",			&battle_config.allow_guild_invite_in_gvg			},
			{ "allow_guild_leave_in_gvg",			&battle_config.allow_guild_leave_in_gvg				},
			{ "guild_skill_available",				&battle_config.guild_skill_available				},
			{ "guild_hunting_skill_available",		&battle_config.guild_hunting_skill_available		},
			{ "guild_skill_check_range",			&battle_config.guild_skill_check_range				},
			{ "allow_guild_skill_in_gvg_only",		&battle_config.allow_guild_skill_in_gvg_only		},
			{ "allow_me_guild_skill",				&battle_config.allow_me_guild_skill					},
			{ "emergencycall_point_randam",			&battle_config.emergencycall_point_randam			},
			{ "emergencycall_call_limit",			&battle_config.emergencycall_call_limit				},
			{ "allow_guild_skill_in_gvgtime_only",	&battle_config.allow_guild_skill_in_gvgtime_only	},
			{ "guild_skill_in_pvp_limit",			&battle_config.guild_skill_in_pvp_limit				},
			{ "guild_exp_rate",						&battle_config.guild_exp_rate						},
			{ "guild_skill_effective_range",		&battle_config.guild_skill_effective_range			},
			{ "tarotcard_display_position",			&battle_config.tarotcard_display_position			},
			{ "serverside_friendlist",				&battle_config.serverside_friendlist				},
			{ "pet0078_hair_id",					&battle_config.pet0078_hair_id						},
			{ "job_soul_check",						&battle_config.job_soul_check						},
			{ "repeal_die_counter_rate",			&battle_config.repeal_die_counter_rate				},
			{ "disp_job_soul_state_change",			&battle_config.disp_job_soul_state_change			},
			{ "check_knowlege_map",					&battle_config.check_knowlege_map					},
			{ "tripleattack_rate_up_keeptime",		&battle_config.tripleattack_rate_up_keeptime		},
			{ "tk_counter_rate_up_keeptime",		&battle_config.tk_counter_rate_up_keeptime			},
			{ "allow_skill_without_day",			&battle_config.allow_skill_without_day				},
			{ "debug_new_disp_status_icon_system",	&battle_config.debug_new_disp_status_icon_system	},
			
		};
		
		if(line[0] == '/' && line[1] == '/')
			continue;
		i=sscanf(line,"%[^:]:%s",w1,w2);
		if(i!=2)
			continue;
		for(i=0;i<sizeof(data)/(sizeof(data[0]));i++)
			if(strcmpi(w1,data[i].str)==0)
				*data[i].val=battle_config_switch(w2);

		if( strcmpi(w1,"import")==0 )
			battle_config_read(w2);
	}
	fclose(fp);
	
	{//�t���O����
	if(battle_config.max_parameter_str > battle_config.max_parameter)
		battle_config.max_parameter_str = battle_config.max_parameter;

	if(battle_config.max_parameter_agi > battle_config.max_parameter)
		battle_config.max_parameter_agi = battle_config.max_parameter;
		
	if(battle_config.max_parameter_vit > battle_config.max_parameter)
		battle_config.max_parameter_vit = battle_config.max_parameter;
		
	if(battle_config.max_parameter_int > battle_config.max_parameter)
		battle_config.max_parameter_int = battle_config.max_parameter;
		
	if(battle_config.max_parameter_dex > battle_config.max_parameter)
		battle_config.max_parameter_dex = battle_config.max_parameter;
		
	if(battle_config.max_parameter_luk > battle_config.max_parameter)
		battle_config.max_parameter_luk = battle_config.max_parameter;
		
	if(battle_config.allow_guild_skill_in_gvgtime_only)
		battle_config.guild_skill_available = 0;
	}

	if(--count==0){
		if(battle_config.flooritem_lifetime < 1000)
			battle_config.flooritem_lifetime = LIFETIME_FLOORITEM*1000;
		if(battle_config.restart_hp_rate < 0)
			battle_config.restart_hp_rate = 0;
		else if(battle_config.restart_hp_rate > 100)
			battle_config.restart_hp_rate = 100;
		if(battle_config.restart_sp_rate < 0)
			battle_config.restart_sp_rate = 0;
		else if(battle_config.restart_sp_rate > 100)
			battle_config.restart_sp_rate = 100;
		if(battle_config.natural_healhp_interval < NATURAL_HEAL_INTERVAL)
			battle_config.natural_healhp_interval=NATURAL_HEAL_INTERVAL;
		if(battle_config.natural_healsp_interval < NATURAL_HEAL_INTERVAL)
			battle_config.natural_healsp_interval=NATURAL_HEAL_INTERVAL;
		if(battle_config.natural_heal_skill_interval < NATURAL_HEAL_INTERVAL)
			battle_config.natural_heal_skill_interval=NATURAL_HEAL_INTERVAL;
		if(battle_config.natural_heal_weight_rate < 50)
			battle_config.natural_heal_weight_rate = 50;
		if(battle_config.natural_heal_weight_rate > 101)
			battle_config.natural_heal_weight_rate = 101;
		battle_config.monster_max_aspd = 2000 - battle_config.monster_max_aspd*10;
		if(battle_config.monster_max_aspd < 10)
			battle_config.monster_max_aspd = 10;
		if(battle_config.monster_max_aspd > 1000)
			battle_config.monster_max_aspd = 1000;
		battle_config.max_aspd = 2000 - battle_config.max_aspd*10;
		if(battle_config.max_aspd < 10)
			battle_config.max_aspd = 10;
		if(battle_config.max_aspd > 1000)
			battle_config.max_aspd = 1000;
		if(battle_config.hp_rate < 0)
			battle_config.hp_rate = 1;
		if(battle_config.sp_rate < 0)
			battle_config.sp_rate = 1;
		if(battle_config.max_hp > 1000000)
			battle_config.max_hp = 1000000;
		if(battle_config.max_hp < 100)
			battle_config.max_hp = 100;
		if(battle_config.max_sp > 1000000)
			battle_config.max_sp = 1000000;
		if(battle_config.max_sp < 100)
			battle_config.max_sp = 100;
		if(battle_config.max_parameter < 10)
			battle_config.max_parameter = 10;
		if(battle_config.max_parameter > 10000)
			battle_config.max_parameter = 10000;
		if(battle_config.max_cart_weight > 1000000)
			battle_config.max_cart_weight = 1000000;
		if(battle_config.max_cart_weight < 100)
			battle_config.max_cart_weight = 100;
		battle_config.max_cart_weight *= 10;

		if(battle_config.agi_penaly_count < 2)
			battle_config.agi_penaly_count = 2;
		if(battle_config.vit_penaly_count < 2)
			battle_config.vit_penaly_count = 2;

		if(battle_config.guild_exp_limit > 99)
			battle_config.guild_exp_limit = 99;
		if(battle_config.guild_exp_limit < 0)
			battle_config.guild_exp_limit = 0;
		if(battle_config.pet_weight < 0)
			battle_config.pet_weight = 0;

		if(battle_config.castle_defense_rate < 0)
			battle_config.castle_defense_rate = 0;
		if(battle_config.castle_defense_rate > 100)
			battle_config.castle_defense_rate = 100;

		if(battle_config.next_exp_limit < 0)
			battle_config.next_exp_limit = 150;
		if(battle_config.card_drop_rate < 0)
			battle_config.card_drop_rate = 0;
		if(battle_config.equip_drop_rate < 0)
			battle_config.equip_drop_rate = 0;
		if(battle_config.refine_drop_rate < 0)
			battle_config.refine_drop_rate = 0;
		if(battle_config.heal_counterstop < 0)
			battle_config.heal_counterstop = 0;
		if (battle_config.finding_ore_drop_rate < 0)
			battle_config.finding_ore_drop_rate = 0;
		else if (battle_config.finding_ore_drop_rate > 10000)
			battle_config.finding_ore_drop_rate = 10000;

		add_timer_func_list(battle_delay_damage_sub,"battle_delay_damage_sub");
	}
	
	return 0;
}
