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
#include "homun.h"
#include "itemdb.h"
#include "clif.h"
#include "pet.h"
#include "guild.h"
#include "status.h"
#include "party.h"
#include "unit.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

int attr_fix_table[4][10][10];

struct Battle_Config battle_config;

/*==========================================
 * 二点間の距離を返す
 * 戻りは整数で0以上
 *------------------------------------------
 */
static int distance(int x0,int y0,int x1,int y1)
{
	int dx,dy;

	dx=abs(x0-x1);
	dy=abs(y0-y1);
	return dx>dy ? dx : dy;
}

//-------------------------------------------------------------------

// ダメージの遅延
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

// 実際にHPを操作
int battle_damage(struct block_list *bl,struct block_list *target,int damage,int flag)
{
	struct map_session_data *sd=NULL;
	struct status_change *sc_data;
	short *sc_count;
	int race = 7, ele = 0;

	nullpo_retr(0, target); //blはNULLで呼ばれることがあるので他でチェック

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

	sc_data = status_get_sc_data(target);

	if(!flag && (sc_count=status_get_sc_count(target))!=NULL && *sc_count>0){
		// 凍結、石化、睡眠を消去
		if(sc_data[SC_FREEZE].timer!=-1)
			status_change_end(target,SC_FREEZE,-1);
		if(sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2==0)
			status_change_end(target,SC_STONE,-1);
		if(sc_data[SC_SLEEP].timer!=-1)
			status_change_end(target,SC_SLEEP,-1);
	}

	// 種族・属性取得
	race = status_get_race(target);
	ele  = status_get_elem_type(target);

	if(target->type==BL_MOB){	// MOB
		struct mob_data *md=(struct mob_data *)target;
		if(md && md->ud.skilltimer!=-1 && md->ud.state.skillcastcancel)	// 詠唱妨害
			unit_skillcastcancel(target,0);
		mob_damage(bl,md,damage,0);

		// カード効果のコーマ・即死
		if(sd && md && flag&(BF_WEAPON|BF_NORMAL) && status_get_class(target) != 1288){
			if(atn_rand()%10000 < sd->weapon_coma_ele2[ele] || atn_rand()%10000 < sd->weapon_coma_race2[race]) {
				mob_damage(bl,md,status_get_hp(target)-1,0);
			} else if(status_get_mode(target) & 0x20){
				if(atn_rand()%10000 < sd->weapon_coma_race2[10])
					mob_damage(bl,md,status_get_hp(target)-1,0);
			} else {
				if(atn_rand()%10000 < sd->weapon_coma_race2[11])
					mob_damage(bl,md,status_get_hp(target)-1,0);
			}

			if(atn_rand()%10000 < sd->weapon_coma_ele[ele] || atn_rand()%10000 < sd->weapon_coma_race[race]) {
				mob_damage(bl,md,status_get_hp(target),0);
			} else if(status_get_mode(target) & 0x20){
				if(atn_rand()%10000 < sd->weapon_coma_race[10])
					mob_damage(bl,md,status_get_hp(target),0);
			} else {
				if(atn_rand()%10000 < sd->weapon_coma_race[11])
					mob_damage(bl,md,status_get_hp(target),0);
			}
		}
		return 0;
	}
	else if(target->type==BL_PC){	// PC
		struct map_session_data *tsd=(struct map_session_data *)target;

		if(tsd && tsd->sc_data && tsd->sc_data[SC_DEVOTION].val1){	// ディボーションをかけられている
			struct map_session_data *md = map_id2sd(tsd->sc_data[SC_DEVOTION].val1);
			if(md && skill_devotion3(&md->bl,target->id)){
				skill_devotion(md,target->id);
			}
			else if(md && bl) {
				int i;
				for(i=0;i<5;i++) {
					if(md->dev.val1[i] == target->id){
						clif_damage(&md->bl,&md->bl, gettick(), 0, 0,
							damage, 0 , 9, 0);
						pc_damage(&md->bl,md,damage);

						// カード効果のコーマ・即死
						if(sd && tsd && md && flag&(BF_WEAPON|BF_NORMAL)){
							if(atn_rand()%10000 < sd->weapon_coma_ele2[ele] ||
								atn_rand()%10000 < sd->weapon_coma_race2[race] ||
								atn_rand()%10000 < sd->weapon_coma_race2[11])
								pc_damage(&md->bl,md,status_get_hp(target)-1);

							if(atn_rand()%10000 < sd->weapon_coma_ele[ele] ||
								atn_rand()%10000 < sd->weapon_coma_race[race] ||
								atn_rand()%10000 < sd->weapon_coma_race[11])
								pc_damage(&md->bl,md,status_get_hp(target));
						}
						return 0;
					}
				}
			}
		}

		if(tsd && tsd->ud.skilltimer!=-1){	// 詠唱妨害
				// フェンカードや妨害されないスキルかの検査
			if( (!tsd->special_state.no_castcancel || map[bl->m].flag.gvg) && tsd->ud.state.skillcastcancel &&
				!tsd->special_state.no_castcancel2)
			{
				unit_skillcastcancel(target,0);
			}
		}

		pc_damage(bl,tsd,damage);

		// カード効果のコーマ・即死
		if(sd && tsd && flag&(BF_WEAPON|BF_NORMAL)){
			if(atn_rand()%10000 < sd->weapon_coma_ele2[ele] ||
			   atn_rand()%10000 < sd->weapon_coma_race2[race] ||
			   atn_rand()%10000 < sd->weapon_coma_race2[11])
				pc_damage(bl,tsd,status_get_hp(target)-1);

			if(atn_rand()%10000 < sd->weapon_coma_ele[ele] ||
			   atn_rand()%10000 < sd->weapon_coma_race[race] ||
			   atn_rand()%10000 < sd->weapon_coma_race[11])
				pc_damage(bl,tsd,status_get_hp(target));
		}
		return 0;
	}
	else if(target->type==BL_HOM){	// HOM
		struct homun_data *hd=(struct homun_data *)target;
		if(hd && hd->ud.skilltimer!=-1 && hd->ud.state.skillcastcancel)	// 詠唱妨害
			unit_skillcastcancel(target,0);
		homun_damage(bl,hd,damage);

		// カード効果のコーマ・即死
		if(sd && hd && flag&(BF_WEAPON|BF_NORMAL) && status_get_class(target) != 1288){
			if(atn_rand()%10000 < sd->weapon_coma_ele2[ele] || atn_rand()%10000 < sd->weapon_coma_race2[race]) {
				homun_damage(bl,hd,status_get_hp(target)-1);
			} else if(status_get_mode(target) & 0x20){
				if(atn_rand()%10000 < sd->weapon_coma_race2[10])
					homun_damage(bl,hd,status_get_hp(target)-1);
			} else {
				if(atn_rand()%10000 < sd->weapon_coma_race2[11])
					homun_damage(bl,hd,status_get_hp(target)-1);
			}

			if(atn_rand()%10000 < sd->weapon_coma_ele[ele] || atn_rand()%10000 < sd->weapon_coma_race[race]) {
				homun_damage(bl,hd,status_get_hp(target));
			} else if(status_get_mode(target) & 0x20){
				if(atn_rand()%10000 < sd->weapon_coma_race[10])
					homun_damage(bl,hd,status_get_hp(target));
			} else {
				if(atn_rand()%10000 < sd->weapon_coma_race[11])
					homun_damage(bl,hd,status_get_hp(target));
			}
		}
		return 0;
	}
	else if(target->type==BL_SKILL)
		return skill_unit_ondamaged((struct skill_unit *)target,bl,damage,gettick());
	return 0;
}
int battle_heal(struct block_list *bl,struct block_list *target,int hp,int sp,int flag)
{
	nullpo_retr(0, target); //blはNULLで呼ばれることがあるので他でチェック

	if(target->type == BL_PET)
		return 0;
	if( unit_isdead(target) )
		return 0;
	if(hp==0 && sp==0)
		return 0;

	if(hp<0)
		return battle_damage(bl,target,-hp,flag);

	if(target->type==BL_MOB)
		return mob_heal((struct mob_data *)target,hp);
	else if(target->type==BL_PC)
		return pc_heal((struct map_session_data *)target,hp,sp);
	else if(target->type==BL_HOM)
		return homun_heal((struct homun_data *)target,hp,sp);
	return 0;
}

/*==========================================
 * ダメージの属性修正
 *------------------------------------------
 */
int battle_attr_fix(int damage,int atk_elem,int def_elem)
{
	int def_type= def_elem%10, def_lv=def_elem/10/2;

	if( atk_elem == 10 )
		atk_elem = atn_rand()%9;	//武器属性ランダムで付加

	// 属性無し(!=無属性)
	if (atk_elem == -1)
		return damage;

	if(	atk_elem<0 || atk_elem>9 || def_type<0 || def_type>9 ||
		def_lv<1 || def_lv>4){	// 属性値がおかしいのでとりあえずそのまま返す
		if(battle_config.error_log)
			printf("battle_attr_fix: unknown attr type: atk=%d def_type=%d def_lv=%d\n",atk_elem,def_type,def_lv);
		return damage;
	}

	return damage*attr_fix_table[def_lv-1][atk_elem][def_type]/100;
}

/*==========================================
 * ダメージ最終計算
 *------------------------------------------
 */
int battle_calc_damage(struct block_list *src,struct block_list *bl,int damage,int div_,int skill_num,int skill_lv,int flag)
{
	struct map_session_data *sd=NULL;
	struct mob_data *md=NULL;
	struct homun_data *hd=NULL;
	struct status_change *sc_data,*sc;
	short *sc_count;
	int class;

	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	BL_CAST( BL_PC,  bl, sd );
	BL_CAST( BL_MOB, bl, md );
	BL_CAST( BL_HOM, bl, hd );

	sc_data = status_get_sc_data(bl);
	sc_count= status_get_sc_count(bl);
	class = status_get_class(bl);
	
	//スキルダメージ補正
	if(skill_num>0){
		int damage_rate=100;
		if(map[bl->m].flag.normal)
			damage_rate = skill_get_damage_rate(skill_num,0);
		else if(map[bl->m].flag.gvg)
			damage_rate = skill_get_damage_rate(skill_num,2);
		else if(map[bl->m].flag.pvp)
			damage_rate = skill_get_damage_rate(skill_num,1);
		else if(map[bl->m].flag.pk)
			damage_rate = skill_get_damage_rate(skill_num,3);
		if(damage>0 && damage_rate!=100)
			damage = damage*damage_rate/100;
	}
	if(sc_count!=NULL && *sc_count>0){
		if(sc_data[SC_ASSUMPTIO].timer != -1 && damage > 0 && skill_num!=PA_PRESSURE && skill_num!=HW_GRAVITATION) { //アスムプティオ
			if(map[bl->m].flag.pvp || map[bl->m].flag.gvg)
				damage=damage*2/3;
			else
				damage=damage/2;
		}

		if (sc_data[SC_BASILICA].timer!=-1 && !(status_get_mode(src)&0x20) && damage > 0)
			damage = 0;

		if (sc_data[SC_FOGWALL].timer!=-1 && damage>0 && flag&BF_WEAPON && flag&BF_LONG)
		{
			if(skill_num == 0)//通常攻撃75%OFF
			{
				damage = damage*25/100;
			}else{	//スキル25%OFF
				damage = damage*75/100;
			}
		}

		if (sc_data[SC_SAFETYWALL].timer!=-1 && flag&BF_WEAPON &&
					flag&BF_SHORT && skill_num != NPC_GUIDEDATTACK) {
			// セーフティウォール
			struct skill_unit *unit;
			unit = (struct skill_unit *)sc_data[SC_SAFETYWALL].val2;
			if (unit && unit->group) {
				if ((--unit->group->val2)<=0)
					skill_delunit(unit);
				damage=0;
			} else {
				status_change_end(bl,SC_SAFETYWALL,-1);
			}
		}

		// ニューマ
		if((sc_data[SC_PNEUMA].timer!=-1 || sc_data[SC_TATAMIGAESHI].timer != -1) && damage>0 && flag&(BF_WEAPON|BF_MISC) && flag&BF_LONG && skill_num != NPC_GUIDEDATTACK){
			damage=0;
		}
		if(sc_data[SC_AETERNA].timer!=-1 && damage>0 && skill_num!=PA_PRESSURE && skill_num!=HW_GRAVITATION){	// レックスエーテルナ
			damage<<=1;
			status_change_end( bl,SC_AETERNA,-1 );
		}

		//属性場のダメージ増加
		if(sc_data[SC_VOLCANO].timer!=-1 && damage>0){	// ボルケーノ
			if(flag&BF_SKILL && skill_get_pl(skill_num)==3)
				damage += damage*sc_data[SC_VOLCANO].val4/100;
			else if(!flag&BF_SKILL && status_get_attack_element(bl)==3)
				damage += damage*sc_data[SC_VOLCANO].val4/100;
		}

		if(sc_data[SC_VIOLENTGALE].timer!=-1 && damage>0){	// バイオレントゲイル
			if(flag&BF_SKILL && skill_get_pl(skill_num)==4)
				damage += damage*sc_data[SC_VIOLENTGALE].val4/100;
			else if(!flag&BF_SKILL && status_get_attack_element(bl)==4)
				damage += damage*sc_data[SC_VIOLENTGALE].val4/100;
		}

		if(sc_data[SC_DELUGE].timer!=-1 && damage>0){	// デリュージ
			if(flag&BF_SKILL && skill_get_pl(skill_num)==1)
				damage += damage*sc_data[SC_DELUGE].val4/100;
			else if(!flag&BF_SKILL && status_get_attack_element(bl)==1)
				damage += damage*sc_data[SC_DELUGE].val4/100;
		}

		if(sc_data[SC_ENERGYCOAT].timer!=-1 && damage>0  && flag&BF_WEAPON && skill_num != PA_PRESSURE){	// エナジーコート プレッシャーは軽減しない
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

		if(sc_data[SC_KYRIE].timer!=-1 && damage > 0){	// キリエエレイソン
			sc=&sc_data[SC_KYRIE];
			sc->val2-=damage;
			if(flag&BF_WEAPON){
				if(sc->val2>=0)	damage=0;
				else damage=-sc->val2;
			}
			if((--sc->val3)<=0 || (sc->val2<=0) || skill_num == AL_HOLYLIGHT)
				status_change_end(bl, SC_KYRIE, -1);
		}
		/* インデュア */
		if(sc_data[SC_ENDURE].timer != -1 && damage > 0 && flag&BF_WEAPON && src->type != BL_PC){
			if((--sc_data[SC_ENDURE].val2)<=0)
				status_change_end(bl, SC_ENDURE, -1);
		}
		/* オートガード */
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
				if(sd){
					sd->ud.canmove_tick = gettick() + delay;
					if(sc_data[SC_SHRINK].timer != -1 && atn_rand()%100<5*sc_data[SC_AUTOGUARD].val1)
					{
						skill_blown(bl,src,2);
					}

				}else if(md)
					md->ud.canmove_tick = gettick() + delay;
			}
		}
		/* パリイング */
		if(sc_data[SC_PARRYING].timer != -1 && damage > 0 && flag&BF_WEAPON) {
			if(atn_rand()%100 < sc_data[SC_PARRYING].val2) {
				damage = 0;
				clif_skill_nodamage(bl,bl,LK_PARRYING,sc_data[SC_PARRYING].val1,1);
			}
		}
		// リジェクトソード
		if(sc_data[SC_REJECTSWORD].timer!=-1 && damage > 0 && flag&BF_WEAPON &&
		   ((src->type==BL_PC && ((struct map_session_data *)src)->status.weapon == (1 || 2 || 3))
		  || src->type==BL_MOB )){
			if(atn_rand()%100 < (15*sc_data[SC_REJECTSWORD].val1)){ //反射確率は15*Lv
				damage = damage*50/100;
				battle_damage(bl,src,damage,0);
				//ダメージを与えたのは良いんだが、ここからどうして表示するんだかわかんねぇ
				//エフェクトもこれでいいのかわかんねぇ
				clif_skill_nodamage(bl,bl,ST_REJECTSWORD,sc_data[SC_REJECTSWORD].val1,1);
				if((--sc_data[SC_REJECTSWORD].val2)<=0)
					status_change_end(bl, SC_REJECTSWORD, -1);
			}
		}
		if(sc_data[SC_SPIDERWEB].timer!=-1 && damage > 0)	// [Celest]
			if( (flag&BF_SKILL && skill_get_pl(skill_num)==3) ||
			  (!(flag&BF_SKILL) && status_get_attack_element(src)==3) ) {
				damage<<=1;
				status_change_end(bl, SC_SPIDERWEB, -1);
			}
	}

	if(damage > 0) { //GvG PK
		struct guild_castle *gc=guild_mapname2gc(map[bl->m].name);
		struct guild *g;

		if(class == 1288) {	// 1288:エンペリウム
			if(flag&BF_SKILL && skill_num!=PA_PRESSURE && skill_num!=HW_GRAVITATION)//プレッシャー
				return 0;
			if(src->type == BL_PC) {
				g=guild_search(((struct map_session_data *)src)->status.guild_id);

				if(g == NULL)
					return 0;//ギルド未加入ならダメージ無し
				else if((gc != NULL) && g->guild_id == gc->guild_id)
					return 0;//自占領ギルドのエンペならダメージ無し
				else if(guild_checkskill(g,GD_APPROVAL) <= 0)
					return 0;//正規ギルド承認がないとダメージ無し
				else if (g && gc && guild_check_alliance(gc->guild_id, g->guild_id, 0) == 1)
					return 0;	// 同盟ならダメージ無し
			}
			else
				return 0;
		}
		if(map[bl->m].flag.gvg && skill_num!=PA_PRESSURE && skill_num!=HW_GRAVITATION){
			if(gc && bl->type == BL_MOB){	//defenseがあればダメージが減るらしい？
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
			if(damage < 1) damage = (!battle_config.skill_min_damage && flag&BF_MAGIC && src->type==BL_PC)? 0: 1;
		}

		//PK
		if(map[bl->m].flag.pk && bl->type == BL_PC && skill_num!=PA_PRESSURE && skill_num!=HW_GRAVITATION){
			if(flag&BF_WEAPON) {
				if(flag&BF_SHORT)
					damage=damage*battle_config.pk_short_damage_rate/100;
				if(flag&BF_LONG)
					damage=damage*battle_config.pk_long_damage_rate/100;
			}
			if(flag&BF_MAGIC)
				damage = damage*battle_config.pk_magic_damage_rate/100;
			if(flag&BF_MISC)
				damage=damage*battle_config.pk_misc_damage_rate/100;
			if(damage < 1) damage = (!battle_config.skill_min_damage && flag&BF_MAGIC && src->type==BL_PC)? 0: 1;
		}
	}

	if((battle_config.skill_min_damage || flag&BF_MISC) && damage > 0) {
		if(div_==255) {
			if(damage<3)
				damage = 3;
		}
		else {
			if(damage<div_)
				damage = div_;
		}
	}

	if( md!=NULL && md->hp>0 && damage > 0 )	// 反撃などのMOBスキル判定
	{
		unsigned int mst = md->state.skillstate;
		int mtg = md->target_id;

		// 攻撃状態へ一時変更
		md->state.skillstate = MSS_ATTACK;
		if (battle_config.mob_changetarget_byskill != 0 || mtg == 0)
		{
			md->target_id = src->id;
		}

		mobskill_event(md,flag);

		// 状態を戻す
		md->state.skillstate = mst;
		md->target_id = mtg;
	}

	//PCの反撃オートスペル
	if(sd && sd->bl.type == BL_PC && src!=bl && sd->status.hp > 0 && damage > 0)
	{
		struct map_session_data *target=(struct map_session_data *)src;
		long asflag = EAS_REVENGE;

		nullpo_retr(damage, target);

		if(skill_num==AM_DEMONSTRATION)
			flag=(flag&~BF_WEAPONMASK)|BF_MISC;

		if(flag&BF_WEAPON) {
			if(flag&BF_SKILL){
				if(battle_config.weapon_attack_autospell)
					asflag += EAS_NORMAL;
				else
					asflag += EAS_SKILL;
			}else
				asflag += EAS_NORMAL;
			if(flag&BF_SHORT)
				asflag += EAS_SHORT;
			if(flag&BF_LONG)
				asflag += EAS_LONG;
		}
		if(flag&BF_MAGIC){
			if(battle_config.magic_attack_autospell)
				asflag += EAS_SHORT|EAS_LONG;
			else
				asflag += EAS_MAGIC;
		}
		if(flag&BF_MISC){
			if(battle_config.misc_attack_autospell)
				asflag += EAS_SHORT|EAS_LONG;
			else
				asflag += EAS_MISC;
		}

		skill_bonus_autospell(&sd->bl,&target->bl,asflag,gettick(),0);
	}

	//PCの反撃
	if(sd && sd->bl.type == BL_PC && src!=bl &&
	 			sd->status.hp > 0 && damage > 0 && flag&BF_WEAPON)
	{
		struct map_session_data *target=(struct map_session_data *)src;
		nullpo_retr(damage, target);
		//反撃状態異常
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
				NPC_SILENCEATTACK,0,NPC_BLINDATTACK,LK_HEADCRUSH
			};
			//対象の耐性
			luk = status_get_luk(&target->bl);
			sc_def_mdef=50 - (3 + status_get_mdef(&target->bl) + luk/3);
			sc_def_vit=50 - (3 + status_get_vit(&target->bl) + luk/3);
			sc_def_int=50 - (3 + status_get_int(&target->bl) + luk/3);
			sc_def_luk=50 - (3 + luk);

/*			if(target->bl.type==BL_MOB){
				if(sc_def_mdef<50)
					sc_def_mdef=50;
				if(sc_def_vit<50)
					sc_def_vit=50;
				if(sc_def_int<50)
					sc_def_int=50;
				if(sc_def_luk<50)
					sc_def_luk=50;
			}
*/
			if(sc_def_mdef<0)
				sc_def_mdef=0;
			if(sc_def_vit<0)
				sc_def_vit=0;
			if(sc_def_int<0)
				sc_def_int=0;

			for(i=SC_STONE;i<=SC_BLEED;i++){
				//対象に状態異常
				if(i==SC_STONE || i==SC_FREEZE)
					sc_def_card=sc_def_mdef;
				else if(i==SC_STAN || i==SC_POISON || i==SC_SILENCE || i==SC_BLEED)
					sc_def_card=sc_def_vit;
				else if(i==SC_SLEEP || i==SC_CONFUSION || i==SC_BLIND)
					sc_def_card=sc_def_int;
				else if(i==SC_CURSE)
					sc_def_card=sc_def_luk;

				if(battle_config.reveff_plus_addeff)
					rate = (sd->addreveff[i-SC_STONE] + sd->addeff[i-SC_STONE] + sd->arrow_addeff[i-SC_STONE])*sc_def_card/100;
				else
					rate = (sd->addreveff[i-SC_STONE])*sc_def_card/100;

				if(target->bl.type == BL_PC || target->bl.type == BL_MOB || target->bl.type == BL_HOM)
				{
					if(atn_rand()%10000 < rate ){
						if(battle_config.battle_log)
							printf("PC %d skill_addreveff: cardによる異常発動 %d %d\n",sd->bl.id,i,sd->addreveff[i-SC_STONE]);
						status_change_start(&target->bl,i,7,0,0,0,(i==SC_CONFUSION)? 10000+7000:skill_get_time2(sc2[i-SC_STONE],7),0);
					}
				}
			}
		}
	}

	return damage;
}

/*==========================================
 * HP/SP吸収の計算
 *------------------------------------------
 */
int battle_calc_drain(int damage, int rate, int per, int val)
{
	int diff = 0;

	if (damage <= 0 || rate <= 0)
		return 0;

	if (per && atn_rand()%100 < rate) {
		diff = (damage * per) / 100;
		if (diff == 0)
			diff = (per > 0)? 1: -1;
	}

	if (val && atn_rand()%100 < rate) {
		diff += val;
	}
	return diff;
}

/*==========================================
 * 修練ダメージ
 *------------------------------------------
 */
int battle_addmastery(struct map_session_data *sd,struct block_list *target,int dmg,int type)
{
	int damage = 0, race, skill, weapon;

	nullpo_retr(0, sd);
	nullpo_retr(0, target);

	race = status_get_race(target);

	// デーモンベイン vs 不死 or 悪魔 (死人は含めない？)
	// DB修正前: SkillLv * 3
	// DB修正後: floor( ( 3 + 0.05 * BaseLv ) * SkillLv )
	if((skill = pc_checkskill(sd,AL_DEMONBANE)) > 0 && (battle_check_undead(race,status_get_elem_type(target)) || race==6) ) {
		//damage += (skill * 3);
		damage += (int)(floor( ( 3 + 0.05 * sd->status.base_level ) * skill )); // sdの内容は保証されている
	}

	// ビーストベイン(+4 ～ +40) vs 動物 or 昆虫
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
		case 0x01:	// 短剣 (Updated By AppleGirl)
		case 0x02:	// 1HS
		{
			// 剣修練(+4 ～ +40) 片手剣 短剣含む
			if((skill = pc_checkskill(sd,SM_SWORD)) > 0) {
				damage += (skill * 4);
			}
			break;
		}
		case 0x03:	// 2HS
		{
			// 両手剣修練(+4 ～ +40) 両手剣
			if((skill = pc_checkskill(sd,SM_TWOHAND)) > 0) {
				damage += (skill * 4);
			}
			break;
		}
		case 0x04:	// 1HL
		{
			// 槍修練(+4 ～ +40,+5 ～ +50) 槍
			if((skill = pc_checkskill(sd,KN_SPEARMASTERY)) > 0) {
				if(!pc_isriding(sd))
					damage += (skill * 4);	// ペコに乗ってない
				else
					damage += (skill * 5);	// ペコに乗ってる
			}
			break;
		}
		case 0x05:	// 2HL
		{
			// 槍修練(+4 ～ +40,+5 ～ +50) 槍
			if((skill = pc_checkskill(sd,KN_SPEARMASTERY)) > 0) {
				if(!pc_isriding(sd))
					damage += (skill * 4);	// ペコに乗ってない
				else
					damage += (skill * 5);	// ペコに乗ってる
			}
			break;
		}
		case 0x06:	// 片手斧
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
		case 0x08:	// メイス
		{
			// メイス修練(+3 ～ +30) メイス
			if((skill = pc_checkskill(sd,PR_MACEMASTERY)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x09:	// なし?
			break;
		case 0x0a:	// 杖
			break;
		case 0x0b:	// 弓
			break;
		case 0x00:	// 素手
		case 0x0c:	// Knuckles
		{
			// 鉄拳(+3 ～ +30) 素手,ナックル
			if((skill = pc_checkskill(sd,MO_IRONHAND)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x0d:	// Musical Instrument
		{
			// 楽器の練習(+3 ～ +30) 楽器
			if((skill = pc_checkskill(sd,BA_MUSICALLESSON)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x0e:	// Dance Mastery
		{
			// Dance Lesson Skill Effect(+3 damage for every lvl = +30) 鞭
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
			// カタール修練(+3 ～ +30) カタール
			if((skill = pc_checkskill(sd,AS_KATAR)) > 0) {
				//ソニックブロー時は別処理（1撃に付き1/8適応)
				damage += (skill * 3);
			}
			break;
		}
		/*
		//銃に修練無し
		case 0x11:	//
		{
			break;
		}
		case 0x12:	//
		{
			break;
		}
		case 0x13:	//
		{
			break;
		}
		case 0x14:	//
		{
			break;
		}
		case 0x15:	//
		{
			break;
		}
		*/
		case 0x16:	//手裏剣
		{
			//飛刀修練
			if((skill = pc_checkskill(sd,NJ_TOBIDOUGU)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
	}
	return dmg+damage;
}


/*==========================================
 * 武器ダメージ計算
 *------------------------------------------
 */

struct Damage battle_calc_weapon_attack(
	struct block_list *src,struct block_list *target,int skill_num,int skill_lv,int wflag)
{
	struct map_session_data *src_sd = NULL, *target_sd = NULL;
	struct mob_data         *src_md = NULL, *target_md = NULL;
	struct pet_data         *src_pd = NULL;
	struct homun_data       *src_hd = NULL, *target_hd = NULL;
	int hitrate,t_flee,cri = 0,s_atkmin,s_atkmax;
	int s_str,s_dex,s_luk,target_count = 1;
	int no_cardfix = 0;
	int t_def1, t_def2, t_vit, s_int;
	int vitbonusmax = 0;
	struct Damage wd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	int damage=0,damage2=0,damage_ot=0,damage_ot2=0,type,div_;
	int blewcount=skill_get_blewcount(skill_num,skill_lv);
	int flag,skill,dmg_lv = 0;
	int t_mode=0,t_race=0,t_enemy=0,t_size=1,t_group = 0,s_ele=0;
	struct status_change *sc_data,*t_sc_data;
	int s_atkmax_=0, s_atkmin_=0, s_ele_;	//二刀流用
	int cardfix, t_ele;
	int da=0,i,t_class,ac_flag = 0;
	int idef_flag=0,idef_flag_=0;
	int tk_power_damage=0,tk_power_damage2=0;//TK_POWER用
	long asflag = EAS_ATTACK;

	memset(&wd,0,sizeof(wd));

	//return前の処理があるので情報出力部のみ変更
	if( src == NULL || target == NULL) {
		nullpo_info(NLP_MARK);
		return wd;
	}

	BL_CAST( BL_PC,  src   , src_sd );
	BL_CAST( BL_MOB, src   , src_md );
	BL_CAST( BL_PET, src   , src_pd );
	BL_CAST( BL_HOM, src   , src_hd );
	BL_CAST( BL_PC,  target, target_sd );
	BL_CAST( BL_MOB, target, target_md );
	BL_CAST( BL_HOM, target, target_hd );

	// アタッカー
	s_ele    = status_get_attack_element(src);	//属性
	s_ele_   = status_get_attack_element2(src);	//左手属性
	s_str    = status_get_str(src);				//STR
	s_dex    = status_get_dex(src);				//DEX
	s_luk    = status_get_luk(src);				//LUK
	s_int    = status_get_int(src);
	sc_data  = status_get_sc_data(src);			//ステータス異常

	// ターゲット
	t_vit     = status_get_vit(target);
	t_race    = status_get_race( target );		//対象の種族
	t_ele     = status_get_elem_type(target);	//対象の属性
	t_enemy   = status_get_enemy_type( target );	//対象の敵タイプ
	t_size    = status_get_size( target );		//対象のサイズ
	t_mode    = status_get_mode( target );		//対象のMode
	t_sc_data = status_get_sc_data( target );	//対象のステータス異常
	t_group   = status_get_group( target );
	t_flee    = status_get_flee( target );
	t_def1    = status_get_def(target);
	t_def2    = status_get_def2(target);
	hitrate   = status_get_hit(src) - t_flee + 80; //命中率計算

	// 属性無し(!=無属性)
	if ((src_sd && battle_config.pc_attack_attr_none) ||
		(src_md && battle_config.mob_attack_attr_none) ||
		(src_pd && battle_config.pet_attack_attr_none) ||
		 src_hd)
	{
		if (s_ele == 0) { s_ele = -1; }
		if (s_ele_ == 0) { s_ele_ = -1; }
	}

	//霧のHIT補正
	if(t_sc_data && t_sc_data[SC_FOGWALL].timer!=-1 && skill_num==0)
	{
		hitrate -= 50;
	}

	if(src_sd && (skill_num != CR_GRANDCROSS || skill_num !=NPC_DARKGRANDCROSS)) //グランドクロスでないなら
		src_sd->state.attack_type = BF_WEAPON; //攻撃タイプは武器攻撃

	//ジャンプ中もしくはウィンク中は駄目
	if(sc_data && (sc_data[SC_HIGHJUMP].timer!=-1 || sc_data[SC_WINKCHARM].timer!=-1)){
		unit_stopattack(src);
		return wd;
	}

	//相手がジャンプ中
	if(t_sc_data && t_sc_data[SC_HIGHJUMP].timer!=-1){
		unit_stopattack(src);
		return wd;
	}

	//オートカウンター処理ここから
	if(skill_lv >= 0 && (skill_num == 0 || (target_sd && battle_config.pc_auto_counter_type&2) ||
		(target_md && battle_config.monster_auto_counter_type&2))
	) {
		if(
			(skill_num != CR_GRANDCROSS || skill_num !=NPC_DARKGRANDCROSS) &&
			t_sc_data && t_sc_data[SC_AUTOCOUNTER].timer != -1
		) { //グランドクロスでなく、対象がオートカウンター状態の場合
			int dir   = map_calc_dir(src,target->x,target->y);
			int t_dir = status_get_dir(target);
			int dist  = distance(src->x,src->y,target->x,target->y);
			int range = status_get_range(target);
			if(dist <= 0 || map_check_dir(dir,t_dir) ) {
				//対象との距離が0以下、または対象の正面？
				t_sc_data[SC_AUTOCOUNTER].val3 = 0;
				t_sc_data[SC_AUTOCOUNTER].val4 = 1;
				if(sc_data && sc_data[SC_AUTOCOUNTER].timer == -1) {
					// 自分がオートカウンター状態
					if( target_sd &&
						(target_sd->status.weapon != 11 && !(target_sd->status.weapon > 16 && target_sd->status.weapon < 22))
						&& dist <= range+1)
						//対象がPCで武器が弓矢でなく射程内
						t_sc_data[SC_AUTOCOUNTER].val3 = src->id;
					if( target_md && range <= 3 && dist <= range+1)
						//または対象がMobで射程が3以下で射程内
						t_sc_data[SC_AUTOCOUNTER].val3 = src->id;
				}
				return wd; //ダメージ構造体を返して終了
			}
			else ac_flag = 1;
		}
	}
	//オートカウンター処理ここまで

	flag = BF_SHORT | BF_WEAPON | BF_NORMAL;	// 攻撃の種類の設定

	type=0;	// normal
	div_ = 1; // single attack

	if(src_md) {
		if(battle_config.enemy_str)
			damage = status_get_baseatk(src);
		else
			damage = 0;
		if(skill_num==HW_MAGICCRASHER){			/* マジッククラッシャーはMATKで殴る */
			s_atkmin = status_get_matk1(src);
			s_atkmax = status_get_matk2(src);
		}else{
			s_atkmin = status_get_atk(src);
			s_atkmax = status_get_atk2(src);
		}
		if(mob_db[src_md->class].range>3 )
			flag=(flag&~BF_RANGEMASK)|BF_LONG;
		if(s_atkmin > s_atkmax) s_atkmin = s_atkmax;
		s_atkmin_ = s_atkmax_ = damage2 = 0;
	} else if(src_pd) {
		if(battle_config.pet_str)
			damage = status_get_baseatk(src);
		else
			damage = 0;
		if(skill_num==HW_MAGICCRASHER){			/* マジッククラッシャーはMATKで殴る */
			s_atkmin = status_get_matk1(src);
			s_atkmax = status_get_matk2(src);
		}else{
			s_atkmin = status_get_atk(src);
			s_atkmax = status_get_atk2(src);
		}
		if(mob_db[src_pd->class].range>3 )
			flag=(flag&~BF_RANGEMASK)|BF_LONG;

		if(s_atkmin > s_atkmax) s_atkmin = s_atkmax;
		s_atkmin_ = s_atkmax_ = damage2 = 0;
	} else if(src_sd) {
		// player
		int s_watk  = status_get_atk(src);	//ATK
		int s_watk_ = status_get_atk_(src);	//ATK左手

		if(skill_num==HW_MAGICCRASHER){			/* マジッククラッシャーはMATKで殴る */
			damage = damage2 = status_get_matk1(src); //damega,damega2初登場、base_atkの取得
		}else{
			damage = damage2 = status_get_baseatk(src); //damega,damega2初登場、base_atkの取得
		}
		s_atkmin = s_atkmin_ = s_dex; //最低ATKはDEXで初期化？
		src_sd->state.arrow_atk = 0; //arrow_atk初期化
		if(src_sd->equip_index[9] >= 0 && src_sd->inventory_data[src_sd->equip_index[9]])
			s_atkmin  = s_atkmin *(80 + src_sd->inventory_data[src_sd->equip_index[9]]->wlv*20)/100;
		if(src_sd->equip_index[8] >= 0 && src_sd->inventory_data[src_sd->equip_index[8]])
			s_atkmin_ = s_atkmin_*(80 + src_sd->inventory_data[src_sd->equip_index[8]]->wlv*20)/100;
		if(src_sd->status.weapon == 11 || (src_sd->status.weapon>16 && src_sd->status.weapon<22)) { //武器が弓矢の場合
			s_atkmin = s_watk * ((s_atkmin<s_watk)? s_atkmin:s_watk)/100; //弓用最低ATK計算
			flag=(flag&~BF_RANGEMASK)|BF_LONG; //遠距離攻撃フラグを有効
			if(src_sd->arrow_ele > 0) //属性矢なら属性を矢の属性に変更
				s_ele = src_sd->arrow_ele;
			src_sd->state.arrow_atk = 1; //arrow_atk有効化
		}

		// サイズ修正
		// ペコ騎乗していて、槍で攻撃した場合は中型のサイズ修正を100にする
		// ウェポンパーフェクション,ドレイクC
		if(skill_num == MO_EXTREMITYFIST) {
			// 阿修羅
			s_atkmax  = s_watk;
			s_atkmax_ = s_watk_;
		} else if(pc_isriding(src_sd) && (src_sd->status.weapon==4 || src_sd->status.weapon==5) && t_size==1) {
			//ペコ騎乗していて、槍で中型を攻撃
			s_atkmax  = s_watk;
			s_atkmax_ = s_watk_;
		} else {
			s_atkmax  = (s_watk    * src_sd->atkmods [ t_size ]) / 100;
			s_atkmin  = (s_atkmin  * src_sd->atkmods [ t_size ]) / 100;
			s_atkmax_ = (s_watk_   * src_sd->atkmods_[ t_size ]) / 100;
			s_atkmin_ = (s_atkmin_ * src_sd->atkmods_[ t_size ]) / 100;
		}
		if( sc_data && sc_data[SC_WEAPONPERFECTION].timer!=-1) {
			// ウェポンパーフェクション || ドレイクカード
			s_atkmax = s_watk;
			s_atkmax_ = s_watk_;
		} else if(src_sd->special_state.no_sizefix) {
			s_atkmax = s_watk;
			s_atkmax_ = s_watk_;
		}
		if( !(src_sd->state.arrow_atk) && s_atkmin > s_atkmax)
			s_atkmin = s_atkmax;	//弓は最低が上回る場合あり
		if(s_atkmin_ > s_atkmax_)
			s_atkmin_ = s_atkmax_;
	} else if(src_hd) {
		if(battle_config.enemy_str)
			damage = status_get_baseatk(src);
		else
			damage = 0;
		if(skill_num==HW_MAGICCRASHER){			/* マジッククラッシャーはMATKで殴る */
			s_atkmin = status_get_matk1(src);
			s_atkmax = status_get_matk2(src);
		}else{
			s_atkmin = status_get_atk(src);
			s_atkmax = status_get_atk2(src);
		}
		if(s_atkmin > s_atkmax) s_atkmin = s_atkmax;
		s_atkmin_ = s_atkmax_ = damage2 = 0;
	} else {
		s_atkmin  = s_atkmax  = 0;
		s_atkmin_ = s_atkmax_ = 0;
		damage = damage2 = 0;
	}

	if(sc_data && sc_data[SC_MAXIMIZEPOWER].timer!=-1 ){
		// マキシマイズパワー
		s_atkmin  = s_atkmax;
		s_atkmin_ = s_atkmax_;
	}

	//太陽と月と星の怒り
	if( src_sd && (target_sd || target_md || target_hd) )
	{
		int atk_rate = 0;
		int tclass = 0;
		if(target_sd)//対象が人
		{
			struct pc_base_job s_class;
			s_class = pc_calc_base_job(target_sd->status.class);
			tclass = s_class.job;
		}else if(target_md)//対象が敵
			tclass = target_md->class;
		else if(target_hd)//対象がホム
			tclass = target_hd->status.class;

		if(sc_data && sc_data[SC_MIRACLE].timer!=-1)//太陽と月と星の奇跡
		{
			//全ての敵が月
			atk_rate = (src_sd->status.base_level + s_dex + s_luk + s_str)/(12-3*pc_checkskill(src_sd,SG_STAR_ANGER));
		}else{
			if(tclass == src_sd->hate_mob[0] && pc_checkskill(src_sd,SG_SUN_ANGER)>0)//太陽の怒り
				atk_rate = (src_sd->status.base_level + s_dex + s_luk)/(12-3*pc_checkskill(src_sd,SG_SUN_ANGER));
			else if(tclass == src_sd->hate_mob[1] && pc_checkskill(src_sd,SG_MOON_ANGER)>0)//月の怒り
				atk_rate = (src_sd->status.base_level + s_dex + s_luk)/(12-3*pc_checkskill(src_sd,SG_MOON_ANGER));
			else if(tclass == src_sd->hate_mob[2] && pc_checkskill(src_sd,SG_STAR_ANGER)>0)//星の怒り
				atk_rate = (src_sd->status.base_level + s_dex + s_luk + s_str)/(12-3*pc_checkskill(src_sd,SG_STAR_ANGER));
		}

		if(atk_rate > 0)
		{
			s_atkmin  += s_atkmin  * atk_rate / 100;
			s_atkmax  += s_atkmax  * atk_rate / 100;
			s_atkmin_ += s_atkmin_ * atk_rate / 100;
			s_atkmax_ += s_atkmax_ * atk_rate / 100;
		}
	}

	if(src_sd) {
		//ダブルアタック判定
		if(skill_num == 0 && skill_lv >= 0 && (skill = pc_checkskill(src_sd,TF_DOUBLE)) > 0 && src_sd->weapontype1 == 0x01 && atn_rand()%100 < (skill*5)) {
			da = 1;
			hitrate = hitrate*(100+skill)/100;
		}
		//チェインアクション
		if(skill_num == 0 && skill_lv >= 0 && da == 0 && (skill = pc_checkskill(src_sd,GS_CHAINACTION)) > 0 && src_sd->weapontype1 == 0x11)
			da = (atn_rand()%100 < (skill*5)) ? 1:0;
		//三段掌
		if( skill_num == 0 && skill_lv >= 0 && da == 0 && (skill = pc_checkskill(src_sd,MO_TRIPLEATTACK)) > 0 && src_sd->status.weapon <= 22 && !src_sd->state.arrow_atk)
		{
			if(sc_data && sc_data[SC_TRIPLEATTACK_RATE_UP].timer!=-1)
			{
				int rate_up[3] = {200,250,300};
				int triple_rate = (30 - skill)*rate_up[sc_data[SC_TRIPLEATTACK_RATE_UP].val1 - 1]/100;
				da = (atn_rand()%100 < triple_rate) ? 2:0;
				status_change_end(src,SC_TRIPLEATTACK_RATE_UP,-1);
			}else
				da = (atn_rand()%100 < (30 - skill)) ? 2:0;
		}

		if(skill_num == 0 && skill_lv >= 0 && da == 0 && sc_data && sc_data[SC_READYCOUNTER].timer!=-1 && pc_checkskill(src_sd,TK_COUNTER) > 0)
		{
			if(sc_data[SC_COUNTER_RATE_UP].timer!=-1 && (skill = pc_checkskill(src_sd,SG_FRIEND)) > 0)
			{
				int counter_rate[3] = {40,50,60};//{200,250,300};
				da = (atn_rand()%100 <  counter_rate[skill - 1]) ? 6:0;
				status_change_end(src,SC_COUNTER_RATE_UP,-1);
			}else
				da = (atn_rand()%100 < 20) ? 6:0;
		}
		//旋風
		if(skill_num == 0 && skill_lv >= 0 && da == 0 && sc_data && sc_data[SC_READYSTORM].timer!=-1 && pc_checkskill(src_sd,TK_STORMKICK) > 0 && atn_rand()%100 < 15) {
			da = 3;
		}else if(skill_num == 0 && skill_lv >= 0 && da == 0 && sc_data && sc_data[SC_READYDOWN].timer!=-1 && pc_checkskill(src_sd,TK_DOWNKICK) > 0 && atn_rand()%100 < 15) {
			da = 4;
		}else if(skill_num == 0 && skill_lv >= 0 && da == 0 && sc_data && sc_data[SC_READYTURN].timer!=-1 &&  pc_checkskill(src_sd,TK_TURNKICK) > 0 && atn_rand()%100 < 15) {
			da = 5;
		}

		//サイドワインダー等
		if(src_sd->double_rate > 0 && da == 0 && skill_num == 0 && skill_lv >= 0)
		{
			da = (atn_rand()%100 < src_sd->double_rate) ? 1:0;
		}

		// 過剰精錬ボーナス
		if(src_sd->overrefine>0 )
			damage  += (atn_rand() % src_sd->overrefine ) + 1;
		if(src_sd->overrefine_>0 )
			damage2 += (atn_rand() % src_sd->overrefine_) + 1;
	}

	if(da == 0){ //ダブルアタックが発動していない
		// クリティカル計算
		cri = status_get_critical(src);
		if(src_sd) cri += src_sd->critical_race[t_race];

		if(src_sd && src_sd->state.arrow_atk) cri += src_sd->arrow_cri;
		if(src_sd && src_sd->status.weapon == 16) cri <<=1; // カタールの場合、クリティカルを倍に
		cri -= status_get_luk(target) * 3;
		if(src_md && battle_config.enemy_critical_rate != 100) {
			cri = cri*battle_config.enemy_critical_rate/100;
			if(cri < 1) cri = 1;
		}
		if(t_sc_data != NULL && t_sc_data[SC_SLEEP].timer!=-1 ) cri <<=1; // 睡眠中はクリティカルが倍に
		if(ac_flag) cri = 1000;

		if(skill_num == KN_AUTOCOUNTER) {
			if(!(battle_config.pc_auto_counter_type&1))
				cri = 1000;
			else
				cri <<= 1;
		}

		if(skill_num==SN_SHARPSHOOTING)
			cri += 200;
		if(skill_num==NJ_KIRIKAGE)
			cri += (250+skill_lv*50);
	}

	if(target_sd && target_sd->critical_def)
		cri = cri * (100-target_sd->critical_def) / 100;

	if(da == 0 && (skill_num==0 || skill_num == KN_AUTOCOUNTER) &&
		(!src_md || battle_config.enemy_critical) &&
		skill_lv >= 0 && (atn_rand() % 1000) < cri
	) { // 判定（スキルの場合は無視）
		/* クリティカル攻撃 */
		damage += s_atkmax;
		damage2 += s_atkmax_;
		if(src_sd && (src_sd->atk_rate != 100 || src_sd->weapon_atk_rate != 0)) {
			damage = (damage * (src_sd->atk_rate + src_sd->weapon_atk_rate[src_sd->status.weapon]))/100;
			damage2 = (damage2 * (src_sd->atk_rate + src_sd->weapon_atk_rate[src_sd->status.weapon]))/100;

			//クリティカル時ダメージ増加
			damage  += damage *src_sd->critical_damage/100;
			damage2 += damage2*src_sd->critical_damage/100;
		}
		if(src_sd && src_sd->state.arrow_atk)
			damage += src_sd->arrow_atk;
		type = 0x0a;

		//ファイティングの計算　この位置？
		if(src_sd && pc_checkskill(src_sd,TK_POWER)>0 && src_sd->status.party_id >0)
		{
			int tk_power_lv = pc_checkskill(src_sd,TK_POWER);
			int member_num   = party_check_same_map_member_count(src_sd);

			if(member_num > 0)
			{
				tk_power_damage = damage*member_num*2*tk_power_lv/100;
				tk_power_damage2 = damage2*member_num*2*tk_power_lv/100;
			}
		}

		damage_ot += damage;	//オーバートラスト、オーバートラストマックスのスキル倍率計算前の攻撃力確保
		damage_ot2 += damage2;

	} else {
		/* 通常攻撃/スキル攻撃 */
		if(s_atkmax > s_atkmin)
			damage += s_atkmin + atn_rand() % (s_atkmax-s_atkmin + 1);
		else
			damage += s_atkmin ;
		if(s_atkmax_ > s_atkmin_)
			damage2 += s_atkmin_ + atn_rand() % (s_atkmax_-s_atkmin_ + 1);
		else
			damage2 += s_atkmin_ ;
		if(src_sd && (src_sd->atk_rate != 100 || src_sd->weapon_atk_rate != 0)) {
			damage = (damage * (src_sd->atk_rate + src_sd->weapon_atk_rate[src_sd->status.weapon]))/100;
			damage2 = (damage2 * (src_sd->atk_rate + src_sd->weapon_atk_rate[src_sd->status.weapon]))/100;
		}

		if(src_sd && src_sd->state.arrow_atk) {
			if(src_sd->arrow_atk > 0)
				damage += atn_rand()%(src_sd->arrow_atk+1);
			hitrate += src_sd->arrow_hit;
		}

		//ファイティングの計算　この位置？
		if(src_sd && pc_checkskill(src_sd,TK_POWER)>0 && src_sd->status.party_id >0)
		{
			int tk_power_lv = pc_checkskill(src_sd,TK_POWER);
			int member_num   = party_check_same_map_member_count(src_sd);

			if(member_num > 0)
			{
				tk_power_damage  = damage*member_num*tk_power_lv/50;
				tk_power_damage2 = damage2*member_num*tk_power_lv/50;
			}
		}

		damage_ot += damage;	//オーバートラスト、オーバートラストマックスのスキル倍率計算前の攻撃力確保
		damage_ot2 += damage2;

		// スキル修正１（攻撃力倍化系）
		// バッシュ,マグナムブレイク,
		// ボーリングバッシュ,スピアブーメラン,ブランディッシュスピア,
		// スピアスタッブ,メマーナイト,カートレボリューション,
		// ダブルストレイフィング,アローシャワー,チャージアロー,
		// ソニックブロー
		if(skill_num>0){
			int i;
			if( (i=skill_get_pl(skill_num))>0 && (!src_sd || !src_sd->arrow_ele ) )
				s_ele=s_ele_=i;

			flag=(flag&~BF_SKILLMASK)|BF_SKILL;
			switch( skill_num ){
			case SM_BASH:		// バッシュ
				damage  = damage *(100+ 30*skill_lv)/100;
				damage2 = damage2*(100+ 30*skill_lv)/100;
				hitrate = (hitrate*(100+5*skill_lv))/100;
				break;
			case SM_MAGNUM:		// マグナムブレイク
				damage  = damage *(20*skill_lv + 100)/100;
				damage2 = damage2*(20*skill_lv + 100)/100;
				hitrate = hitrate*(10*skill_lv + 100)/100;
				break;
			case HVAN_EXPLOSION:
				damage  = status_get_hp(src)*(50+50*skill_lv)/100;
				damage2 = status_get_hp(src)*(50+50*skill_lv)/100;
				hitrate = 1000000;
				break;
			case MC_MAMMONITE:	// メマーナイト
				damage  = damage *(100+ 50*skill_lv)/100;
				damage2 = damage2*(100+ 50*skill_lv)/100;
				break;
			case AC_DOUBLE:	// ダブルストレイフィング
				if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
					int arr = atn_rand()%(src_sd->arrow_atk+1);
					damage  += arr;
					damage2 += arr;
				}
				damage  = damage *(180+ 20*skill_lv)/100;
				damage2 = damage2*(180+ 20*skill_lv)/100;
				div_=2;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				if(src_sd)
					src_sd->state.arrow_atk = 1;
				break;
			case HT_POWER:	// ビーストストレイフィング
				if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
					int arr = atn_rand()%(src_sd->arrow_atk+1);
					damage  += arr;
					damage2 += arr;
				}
				damage  = damage *(180+ 20*skill_lv)/100;
				damage2 = damage2*(180+ 20*skill_lv)/100;
				div_=2;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				if(src_sd)
					src_sd->state.arrow_atk = 1;
				break;
			case AC_SHOWER:	// アローシャワー
				if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
					int arr = atn_rand()%(src_sd->arrow_atk+1);
					damage  += arr;
					damage2 += arr;
				}
				damage  = damage *(75 + 5*skill_lv)/100;
				damage2 = damage2*(75 + 5*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				if(src_sd)
					src_sd->state.arrow_atk = 1;
				blewcount=0;
				break;
			case AC_CHARGEARROW:	// チャージアロー
				if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
					int arr = atn_rand()%(src_sd->arrow_atk+1);
					damage  += arr;
					damage2 += arr;
				}
				damage  = damage*150/100;
				damage2 = damage2*150/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				if(src_sd)
					src_sd->state.arrow_atk = 1;
				break;
			case HT_PHANTASMIC:	// ファンタスミックアロー
				if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
					int arr = atn_rand()%(src_sd->arrow_atk+1);
					damage  += arr;
					damage2 += arr;
				}
				damage  = damage*150/100;
				damage2 = damage2*150/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				if(src_sd)
					src_sd->state.arrow_atk = 1;
				break;
			case KN_CHARGEATK:			//チャージアタック
				{
					//distance
					int dist  = distance(src->x,src->y,target->x,target->y)-1;
					if(dist>2){
						damage  = damage *(100+100*((int)dist/3))/100;
						damage2 = damage2*(100+100*((int)dist/3))/100;
					}else{
						damage  = damage;
						damage2 = damage2;
					}
				}
				break;
			case AS_VENOMKNIFE:			//ベナムナイフ
				if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
					int arr = atn_rand()%(src_sd->arrow_atk+1);
					damage  += arr;
					damage2 += arr;
					if(src_sd->arrow_ele > 0) //属性矢なら属性を矢の属性に変更
						s_ele = src_sd->arrow_ele;
				}
				damage  = damage;
				damage2 = damage2;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				no_cardfix = 1;
				if(src_sd)
					src_sd->state.arrow_atk = 1;
				break;
			case SG_SUN_WARM://太陽の温もり
			case SG_MOON_WARM://月の温もり
			case SG_STAR_WARM://星の温もり
				if(src_sd) {
					if(src_sd->status.sp < 2)
					{
						status_change_end(src,SkillStatusChangeTable[skill_num],-1);
						break;
					}
					if(target_sd)
					{
						target_sd->status.sp -= 5;
						if(target_sd->status.sp<0)
							target_sd->status.sp = 0;
						clif_updatestatus(target_sd,SP_SP);
					}
					//殴ったのでSP消費
					src_sd->status.sp -= 2;
					clif_updatestatus(src_sd,SP_SP);
				} else if(target_sd)
				{
					target_sd->status.sp -= 5;
					clif_updatestatus(target_sd,SP_SP);
				}
				break;
			case KN_PIERCE:	// ピアース
				damage  = damage*(100+ 10*skill_lv)/100;
				damage2 = damage2*(100+ 10*skill_lv)/100;
				hitrate = hitrate*(100+5*skill_lv)/100;
				div_=t_size+1;
				damage *=div_;
				damage2*=div_;
				break;
			case KN_SPEARSTAB:	// スピアスタブ
				damage  = damage *(100+ 15*skill_lv)/100;
				damage2 = damage2*(100+ 15*skill_lv)/100;
				blewcount=0;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case KN_SPEARBOOMERANG:	// スピアブーメラン
				damage  = damage *(100+ 50*skill_lv)/100;
				damage2 = damage2*(100+ 50*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case KN_BRANDISHSPEAR: // ブランディッシュスピア
				{
					int damage3 = 0, damage4 = 0;
					damage  = damage *(100+ 20*skill_lv)/100;
					damage2 = damage2*(100+ 20*skill_lv)/100;
					if(wflag==1){
						if(skill_lv>3){
							damage3+=damage /2;
							damage4+=damage2/2;
						}else if(skill_lv>6){
							damage3+=damage /4;
							damage4+=damage2/4;
						}else if(skill_lv>9){
							damage3+=damage /8;
							damage4+=damage2/8;
						}
					}else if(wflag==2){
						if(skill_lv>6){
							damage3+=damage /2;
							damage4+=damage2/2;
						}else if(skill_lv>9){
							damage3+=damage /4;
							damage4+=damage2/4;
						}
					}else if(wflag==3 && skill_lv>9){
							damage3+=damage /2;
							damage4+=damage2/2;
					}
					damage += damage3;
					damage2+= damage4;
				}
				break;
			case KN_BOWLINGBASH:	// ボウリングバッシュ
				damage = damage*(100+ 40*skill_lv)/100;
				blewcount=0;
				break;
			case KN_AUTOCOUNTER:
				if(battle_config.pc_auto_counter_type&1)
					hitrate += 20;
				else
					hitrate = 1000000;
				flag=(flag&~BF_SKILLMASK)|BF_NORMAL;
				break;
			case AS_SONICBLOW:	// ソニックブロウ
				damage = damage*(300+ 50*skill_lv)/100;
				damage2 = damage2*(300+ 50*skill_lv)/100;

				if(src_sd && pc_checkskill(src_sd,AS_SONICACCEL)>0)
				{
					damage = damage*110/100;
					damage2 = damage2*110/100;
					hitrate = hitrate*150/100;
				}
				if(sc_data && sc_data[SC_ASSASIN].timer!=-1)
				{
					if(map[src->m].flag.gvg){
						damage = damage*125/100;
						damage2 = damage2*125/100;
					}
					else{
						damage = damage*2;
						damage2 = damage2*2;
					}
				}
				div_=8;
				break;
			case AS_GRIMTOOTH:	// グリムトゥース
				damage  = damage *(100+ 20*skill_lv)/100;
				damage2 = damage2*(100+ 20*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case TF_SPRINKLESAND:	// 砂まき
				damage = damage*130/100;
				damage2 = damage2*130/100;
				break;
			case MC_CARTREVOLUTION:	// カートレボリューション
				if(src_sd && src_sd->cart_max_weight > 0 && src_sd->cart_weight > 0) {
					damage = (damage*(150 + pc_checkskill(src_sd,BS_WEAPONRESEARCH) + (src_sd->cart_weight*100/src_sd->cart_max_weight) ) )/100;
					damage2 = (damage2*(150 + pc_checkskill(src_sd,BS_WEAPONRESEARCH) + (src_sd->cart_weight*100/src_sd->cart_max_weight) ) )/100;
				}
				else {
					damage = (damage*150)/100;
					damage2 = (damage2*150)/100;
				}
				blewcount=0;
				break;
			// 以下MOB
			case NPC_COMBOATTACK:	// 多段攻撃
				damage = (damage*50)/100;
				damage2 = (damage2*50)/100;
				div_=skill_get_num(skill_num,skill_lv);
				damage *= div_;
				damage2 *= div_;
				s_ele = 0;
				s_ele_ = 0;
				break;
			case NPC_RANDOMATTACK:	// ランダムATK攻撃
				damage = damage*(50+atn_rand()%150)/100;
				damage2 = damage2*(50+atn_rand()%150)/100;
				s_ele = 0;
				s_ele_ = 0;
				break;
			// 属性攻撃
			case NPC_WATERATTACK:
			case NPC_GROUNDATTACK:
			case NPC_FIREATTACK:
			case NPC_WINDATTACK:
			case NPC_POISONATTACK:
			case NPC_HOLYATTACK:
			case NPC_DARKNESSATTACK:
			case NPC_TELEKINESISATTACK:
			case NPC_UNDEADATTACK:
				damage = damage*(25+75*skill_lv)/100;
				damage2 = damage2*(25+75*skill_lv)/100;
				break;
			case NPC_CRITICALSLASH:
			case NPC_GUIDEDATTACK:
				hitrate = 1000000;
				s_ele = 0;
				s_ele_ = 0;
				break;
			case NPC_RANGEATTACK:
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				s_ele = 0;
				s_ele_ = 0;
				break;
			case NPC_PIERCINGATT:
				damage = (damage*75)/100;
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
				s_ele = 0;
				s_ele_ = 0;
				break;
			case RG_BACKSTAP:	// バックスタブ
				damage = damage*(300+ 40*skill_lv)/100;
				damage2 = damage2*(300+ 40*skill_lv)/100;
				if(src_sd && src_sd->status.weapon == 11) {	// 弓なら半減
					damage /= 2;
					damage2 /= 2;
				}
				hitrate = 1000000;
				break;
			case RG_RAID:	// サプライズアタック
				damage = damage*(100+ 40*skill_lv)/100;
				damage2 = damage2*(100+ 40*skill_lv)/100;
				break;
			case RG_INTIMIDATE:	// インティミデイト
				damage = damage*(100+ 30*skill_lv)/100;
				damage2 = damage2*(100+ 30*skill_lv)/100;
				break;
			case CR_SHIELDCHARGE:	// シールドチャージ
				damage = damage*(100+ 20*skill_lv)/100;
				damage2 = damage2*(100+ 20*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
				s_ele = 0;
				break;
			case CR_SHIELDBOOMERANG:	// シールドブーメラン
				damage = damage*(100+ 30*skill_lv)/100;
				damage2 = damage2*(100+ 30*skill_lv)/100;
				if(sc_data && sc_data[SC_CRUSADER].timer!=-1)
				{
					damage = damage*2;
					damage2 = damage2*2;
					hitrate= 1000000;
				}
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				s_ele = 0;
				break;
			case CR_HOLYCROSS:	// ホーリークロス
			case NPC_DARKCROSS: // ダーククロス
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
			case AM_DEMONSTRATION:	// デモンストレーション
				hitrate= 1000000;
				damage = damage*(100+ 20*skill_lv)/100;
				damage2 = damage2*(100+ 20*skill_lv)/100;
				no_cardfix = 1;
				break;
			case AM_ACIDTERROR:	// アシッドテラー
				hitrate= 1000000;
				damage = damage*(100+ 40*skill_lv)/100;
				damage2 = damage2*(100+ 40*skill_lv)/100;
				s_ele = 0;
				s_ele_ = 0;
				no_cardfix = 1;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case MO_FINGEROFFENSIVE:	//指弾
				if(src_sd && battle_config.finger_offensive_type == 0) {
					damage = damage * (100 + 50 * skill_lv) / 100 * src_sd->spiritball_old;
					damage2 = damage2 * (100 + 50 * skill_lv) / 100 * src_sd->spiritball_old;
					div_ = src_sd->spiritball_old;
				}
				else {
					damage = damage * (100 + 50 * skill_lv) / 100;
					damage2 = damage2 * (100 + 50 * skill_lv) / 100;
					div_ = 1;
				}
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case MO_INVESTIGATE:	// 発勁
				if(t_def1 < 1000000) {
					damage = damage*(100+ 75*skill_lv)/100 * (t_def1 + t_def2)/50;
					damage2 = damage2*(100+ 75*skill_lv)/100 * (t_def1 + t_def2)/50;
				}
				hitrate = 1000000;
				s_ele = 0;
				s_ele_ = 0;
				break;
			case MO_BALKYOUNG:
				damage = damage*3;
				damage2 = damage2*3;
				break;
			case MO_EXTREMITYFIST:	// 阿修羅覇鳳拳
				if(src_sd) {
					damage = damage * (8 + ((src_sd->status.sp)/10)) + 250 + (skill_lv * 150);
					damage2 = damage2 * (8 + ((src_sd->status.sp)/10)) + 250 + (skill_lv * 150);
					src_sd->status.sp = 0;
					clif_updatestatus(src_sd,SP_SP);
				} else {
					damage = damage * 8 + 250 + (skill_lv * 150);
					damage2 = damage2 * 8 + 250 + (skill_lv * 150);
				}
				hitrate = 1000000;
				s_ele = 0;
				s_ele_ = 0;
				break;
			case MO_CHAINCOMBO:	// 連打掌
				damage = damage*(150+ 50*skill_lv)/100;
				damage2 = damage2*(150+ 50*skill_lv)/100;
				div_=4;
				break;
			case MO_COMBOFINISH:	// 猛龍拳
				damage = damage*(240+ 60*skill_lv)/100;
				damage2 = damage2*(240+ 60*skill_lv)/100;
				//PTには入っている
				//カウンターアタックの確率上昇
				if(src_sd && src_sd->status.party_id>0){
					struct party *pt = party_search(src_sd->status.party_id);
					if(pt!=NULL)
					{
						int i;
						struct map_session_data* psrc_sd = NULL;

						for(i=0;i<MAX_PARTY;i++)
						{
							psrc_sd = pt->member[i].sd;
							if(!psrc_sd || src_sd == psrc_sd)
								continue;
							if(src_sd->bl.m == psrc_sd->bl.m && pc_checkskill(psrc_sd,TK_COUNTER)>0)
							{
								status_change_start(&psrc_sd->bl,SC_COUNTER_RATE_UP,1,0,0,0,battle_config.tk_counter_rate_up_keeptime,0);
							}
						}
					}
				}
				break;
			case TK_STORMKICK:	//旋風蹴り
				damage = damage*(160+ 20*skill_lv)/100;
				damage2 = damage2*(160+ 20*skill_lv)/100;
				break;
			case TK_DOWNKICK:	//下段蹴り
				damage = damage*(160+ 20*skill_lv)/100;
				damage2 = damage2*(160+ 20*skill_lv)/100;
				break;
			case TK_TURNKICK:	//回転蹴り
				damage = damage*(190+ 30*skill_lv)/100;
				damage2 = damage2*(190+ 30*skill_lv)/100;
				break;
			case TK_COUNTER:	//カウンター蹴り
				damage = damage*(190+ 30*skill_lv)/100;
				damage2 = damage2*(190+ 30*skill_lv)/100;
				hitrate = 1000000;
				//PTには入っている
				//三段掌の確率上昇
				if(src_sd && src_sd->status.party_id>0){
					int tk_friend_lv = pc_checkskill(src_sd,SG_FRIEND);
					struct party *pt = party_search(src_sd->status.party_id);
					if(pt && tk_friend_lv>0)
					{
						int i;
						struct map_session_data* psrc_sd = NULL;

						for(i=0;i<MAX_PARTY;i++)
						{
							psrc_sd = pt->member[i].sd;
							if(!psrc_sd || src_sd==psrc_sd)
								continue;

							if(src_sd->bl.m == psrc_sd->bl.m && pc_checkskill(psrc_sd,MO_TRIPLEATTACK)>0)
							{
								status_change_start(&psrc_sd->bl,SC_TRIPLEATTACK_RATE_UP,tk_friend_lv,0,0,0,battle_config.tripleattack_rate_up_keeptime,0);
							}
						}
					}
				}
				break;

			case BA_MUSICALSTRIKE:	// ミュージカルストライク
			case DC_THROWARROW:	    // 矢撃ち
				damage = damage*(60+ 40 * skill_lv)/100;
				damage2 = damage2*(60+ 40 * skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				if (src_sd)
					s_ele = src_sd->arrow_ele;
				break;

			case CH_TIGERFIST:	// 伏虎拳
				damage = damage*(40+ 100*skill_lv)/100;
				damage2 = damage2*(40+ 100*skill_lv)/100;
				break;
			case CH_CHAINCRUSH:	// 連柱崩撃
				damage = damage*(400+ 100*skill_lv)/100;
				damage2 = damage2*(400+ 100*skill_lv)/100;
				div_=skill_get_num(skill_num,skill_lv);
				break;
			case CH_PALMSTRIKE:	// 猛虎硬派山
				damage = damage*(200+ 100*skill_lv)/100;
				damage2 = damage2*(200+ 100*skill_lv)/100;
				break;
			case LK_SPIRALPIERCE:			/* スパイラルピアース */
				damage = damage*(80+ 40*skill_lv)/100;
				damage2 = damage2*(80+ 40*skill_lv)/100;
				div_=5;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
			case LK_HEADCRUSH:				/* ヘッドクラッシュ */
				damage = damage*(100+ 40*skill_lv)/100;
				damage2 = damage2*(100+ 40*skill_lv)/100;
				break;
			case LK_JOINTBEAT:				/* ジョイントビート */
				damage = damage*(50+ 10*skill_lv)/100;
				damage2 = damage2*(50+ 10*skill_lv)/100;
				break;
			case HW_MAGICCRASHER:				/* マジッククラッシャー */
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case ASC_METEORASSAULT:			/* メテオアサルト */
				damage = damage*(40+ 40*skill_lv)/100;
				damage2 = damage2*(40+ 40*skill_lv)/100;
				no_cardfix = 1;
				break;
			case ASC_BREAKER:				/* ソウルブレイカー */
				damage = damage * skill_lv;
				damage2 = damage2 * skill_lv;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				no_cardfix = 1;
				break;
			case SN_SHARPSHOOTING:			/* シャープシューティング */
				damage = damage*(200+50*skill_lv)/100;
				damage2 = damage2*(200+50*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case CG_ARROWVULCAN:			/* アローバルカン */
				damage = damage*(200+100*skill_lv)/100;
				damage2 = damage2*(200+100*skill_lv)/100;
				div_=9;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				if (src_sd)
					 s_ele = src_sd->arrow_ele;
				break;
			case AS_SPLASHER:		/* ベナムスプラッシャー */
				if(src_sd) {
					damage = damage*(500+50*skill_lv+20*pc_checkskill(src_sd,AS_POISONREACT))/100;
					damage2 = damage2*(500+50*skill_lv+20*pc_checkskill(src_sd,AS_POISONREACT))/100;
				} else {
					damage = damage*(500+50*skill_lv)/100;
					damage2 = damage2*(500+50*skill_lv)/100;
				}
				no_cardfix = 1;
				hitrate = 1000000;
				break;
			case AS_POISONREACT:		/* ポイズンリアクト（攻撃で反撃） */
				damage = damage*(30*skill_lv+100)/100;
				//damage2 = damage2	//左手には乗らない
				break;
			case TK_JUMPKICK: //飛び蹴り
				if(src_sd && sc_data[SC_DODGE_DELAY].timer!=-1)
				{
					int gain = src_sd->status.base_level/10;
					damage = damage*(40+(10+gain)*skill_lv)/100;
					damage2 = damage2*(40+(10+gain)*skill_lv)/100;
					if(sc_data[SC_DODGE_DELAY].timer!=-1)
						status_change_end(src,SC_DODGE_DELAY,-1);
				}else{
					damage = damage*(40+10*skill_lv)/100;
					damage2 = damage2*(40+10*skill_lv)/100;
				}
				status_change_end_by_jumpkick(target);
				break;
			case PA_SHIELDCHAIN:	/* シールドチェイン */
				if(src_sd)
				{
					int idx = src_sd->equip_index[8];
					damage = s_str+(s_str/10)*(s_str/10)+(s_dex/5)+(s_luk/5);
					if(idx >= 0 && src_sd->inventory_data[idx] && src_sd->inventory_data[idx]->type == 5)
						damage += src_sd->status.inventory[idx].refine*4 + src_sd->inventory_data[idx]->weight/10;
				}else{
					damage = damage*(100+30*skill_lv)/100;
				}
				damage2 = damage;
				hitrate = (hitrate*(100+5*skill_lv))/100;
				div_=5;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				s_ele = 0;
				s_ele_ = 0;
				break;
			case WS_CARTTERMINATION:	/* カートターミネーション */
				if(src_sd && src_sd->cart_max_weight > 0 && src_sd->cart_weight > 0) {
					double weight=(8000.*src_sd->cart_weight)/src_sd->cart_max_weight;
					damage = (int)(damage*(weight/(16-skill_lv)/100));
					damage2 = (int)(damage2*(weight/(16-skill_lv)/100));
				}
				else {
					damage = (damage*100)/100;
					damage2 = (damage2*100)/100;
				}
				no_cardfix = 1;
				break;
			case CR_ACIDDEMONSTRATION:	/* アシッドデモンストレーション */
				hitrate = 1000000;
				{
					double val = s_int*s_int/10./(s_int+t_vit);
					val = val*t_vit*skill_lv*7.;
					if(target->type == BL_PC)
						val /= 2;
					damage  = (int)val;
					damage2 = (int)val;
				}
				div_=skill_get_num( skill_num,skill_lv );
				s_ele = 0;
				s_ele_ = 0;
				no_cardfix = 1;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case ITM_TOMAHAWK:		/* トマホーク投げ */
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case GS_FLING:			/* フライング */
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case GS_TRIPLEACTION:	/* トリプルアクション */
				damage *= 3;
				damage2 *= 3;
				div_=3;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case GS_BULLSEYE:		/* ブルズアイ */
				damage *= 5;
				damage2 *= 5;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case GS_MAGICALBULLET:	/* マジカルバレット */
				{
					int matk1=status_get_matk1(src),matk2=status_get_matk2(src);
					if(matk1>matk2)
						damage += matk2+atn_rand()%(matk1-matk2+1);
					else
						damage += matk2;
					flag=(flag&~BF_RANGEMASK)|BF_LONG;
				}
				break;
			case GS_TRACKING:		/* トラッキング */
				if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
					int arr = atn_rand()%(src_sd->arrow_atk+1);
					damage  += arr;
					damage2 += arr;
				}
				damage  = damage *(120*skill_lv)/100;
				damage2 = damage2*(120*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				if(src_sd)
					src_sd->state.arrow_atk = 1;
				break;
			case GS_DISARM:			/* ディザーム */
				if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
					int arr = atn_rand()%(src_sd->arrow_atk+1);
					damage  += arr;
					damage2 += arr;
				}
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				if(src_sd)
					src_sd->state.arrow_atk = 1;
				break;
			case GS_PIERCINGSHOT:	/* ピアシングショット */
				if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
					int arr = atn_rand()%(src_sd->arrow_atk+1);
					damage  += arr;
					damage2 += arr;
				}
				damage = damage*(100+skill_lv*20)/100;
				damage2 = damage2*(100+skill_lv*20)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				if(src_sd)
					src_sd->state.arrow_atk = 1;
				break;
			case GS_RAPIDSHOWER:	/* ラピッドシャワー */
				if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
					int arr = atn_rand()%(src_sd->arrow_atk+1);
					damage  += arr;
					damage2 += arr;
				}
				damage = damage*(500+skill_lv*50)/100;
				damage2 = damage2*(500+skill_lv*50)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				div_=5;
				if(src_sd)
					src_sd->state.arrow_atk = 1;
				break;
			case GS_DESPERADO:		/* デスペラード */
				if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
					int arr = atn_rand()%(src_sd->arrow_atk+1);
					damage  += arr;
					damage2 += arr;
				}
				damage = damage*(50+skill_lv*50)/100;
				damage2 = damage2*(50+skill_lv*50)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				div_=10;
				if(src_sd)
					src_sd->state.arrow_atk = 1;
				break;
			case GS_DUST:			/* ダスト */
				if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
					int arr = atn_rand()%(src_sd->arrow_atk+1);
					damage  += arr;
					damage2 += arr;
				}
				damage = damage*(100+skill_lv*50)/100;
				damage2 = damage2*(100+skill_lv*50)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				if(src_sd)
					src_sd->state.arrow_atk = 1;
				break;
			case GS_FULLBUSTER:		/* フルバスター */
				if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
					int arr = atn_rand()%(src_sd->arrow_atk+1);
					damage  += arr;
					damage2 += arr;
				}
				damage = damage*(300+skill_lv*100)/100;
				damage2 = damage2*(300+skill_lv*100)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				if(src_sd)
					src_sd->state.arrow_atk = 1;
				break;
			case GS_SPREADATTACK:	/* スプレッドアタック */
				if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
					int arr = atn_rand()%(src_sd->arrow_atk+1);
					damage  += arr;
					damage2 += arr;
				}
				damage = damage*(80+skill_lv*20)/100;
				damage2 = damage2*(80+skill_lv*20)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				if(src_sd)
					src_sd->state.arrow_atk = 1;
				break;
			case NJ_SYURIKEN:		/* 手裏剣投げ */
				if(src_sd){
					if(!src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
						int arr = atn_rand()%(src_sd->arrow_atk+1);
						damage  += arr;
						damage2 += arr;
					}
					if(src_sd->arrow_ele > 0) //属性矢なら属性を矢の属性に変更
						s_ele = src_sd->arrow_ele;
					src_sd->state.arrow_atk = 1;
				}
				damage += (skill_lv*4);
				damage2 += (skill_lv*4);
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case NJ_KUNAI:			/* クナイ投げ */
				if(src_sd){
					if(!src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
						int arr = atn_rand()%(src_sd->arrow_atk+1);
						damage  += arr;
						damage2 += arr;
					}
					if(src_sd->arrow_ele > 0) //属性矢なら属性を矢の属性に変更
						s_ele = src_sd->arrow_ele;
					src_sd->state.arrow_atk = 1;
				}
				damage *= 3;
				damage2 *= 3;
				div_=3;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case NJ_HUUMA:			/* 風魔手裏剣投げ */
				damage = damage*(150+skill_lv*150)/100;
				damage2 = damage2*(150+skill_lv*150)/100;
				div_=skill_get_num( skill_num,skill_lv );
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case NJ_ZENYNAGE:	/* 銭投げ */
				if(src_sd){
					damage = src_sd->zenynage_damage;
					damage2 = src_sd->zenynage_damage;
					src_sd->zenynage_damage = 0;//撃ったらリセット
				}else{
					damage = skill_get_zeny(NJ_ZENYNAGE,skill_lv)/2;
					damage += atn_rand()%damage;
					damage2 = damage;
				}
				if(target->type==BL_PC || t_mode & 0x20){
					damage /= 2;
					damage2 /= 2;
				}
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				s_ele = 0;
				s_ele_ = 0;
				no_cardfix = 1;
				break;
			case NJ_TATAMIGAESHI:	/* 畳替し */
				damage = damage*(100+skill_lv*10)/100;
				damage2 = damage2*(100+skill_lv*10)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case NJ_KASUMIKIRI:		/* 霞斬り */
				damage = damage*(100+skill_lv*10)/100;
				damage2 = damage2*(100+skill_lv*10)/100;
				break;
			case NJ_KIRIKAGE:		/* 斬影 */
				damage *= skill_lv;
				damage2 *= skill_lv;
				break;
			case NJ_ISSEN://一閃
				if(src_sd) {
					damage = damage* (src_sd->status.hp-1)/(250-10*skill_lv);
					damage2 = damage2* (src_sd->status.hp-1)/(250-10*skill_lv);
					src_sd->status.hp = 1;
					clif_updatestatus(src_sd,SP_HP);
				}
				s_ele = 0;
				s_ele_ = 0;
				if(sc_data && sc_data[SC_NEN].timer!=-1)
					status_change_end(src,SC_NEN,-1);
				break;
			case HFLI_MOON:
				damage = damage*(110+skill_lv*110)/100;
				damage2 = damage2*(110+skill_lv*110)/100;
				div_=skill_get_num(skill_num,skill_lv);
				break;
			case HFLI_SBR44:
				if(src_hd){
					damage = src_hd->intimate*skill_lv;
					damage2 = src_hd->intimate*skill_lv;
					src_hd->intimate = 200;
					if(battle_config.homun_skill_intimate_type)
						src_hd->status.intimate = 200;
					clif_send_homdata(src_hd->msd,0x100,src_hd->intimate/100);
				}
				break;
			}
		}

		//ファイティングの追加ダメージ
		damage += tk_power_damage;
		damage2 += tk_power_damage2;

		if(da == 2) { //三段掌が発動しているか
			if(src_sd)
				damage = damage * (100 + 20 * pc_checkskill(src_sd, MO_TRIPLEATTACK)) / 100;
		}

		// 防御無視判定および錐効果ダメージ計算
		switch (skill_num) {
		case KN_AUTOCOUNTER:
		case CR_GRANDCROSS:
		case MO_INVESTIGATE:
		case MO_EXTREMITYFIST:
		case AM_ACIDTERROR:
		case CR_ACIDDEMONSTRATION:
		case NJ_ZENYNAGE:
			break;
		case WS_CARTTERMINATION:
		case PA_SHIELDCHAIN:
			if( skill_num==WS_CARTTERMINATION && !battle_config.def_ratio_atk_to_carttermination )
				break;
			if( skill_num==PA_SHIELDCHAIN && !battle_config.def_ratio_atk_to_shieldchain )
				break;
		default:
			if(src_sd && t_def1 < 1000000)
			{
				int mask = (1<<t_race) | ( (t_mode&0x20)? (1<<10): (1<<11) );
				if( src_sd->ignore_def_ele & (1<<t_ele) || src_sd->ignore_def_race & mask || src_sd->ignore_def_enemy & (1<<t_enemy) )
					idef_flag = 1;
				if( src_sd->ignore_def_ele_ & (1<<t_ele) || src_sd->ignore_def_race_ & mask || src_sd->ignore_def_enemy_ & (1<<t_enemy) ) {
					idef_flag_ = 1;
					if(battle_config.left_cardfix_to_right)
						idef_flag = 1;
				}
				if( !idef_flag && (src_sd->def_ratio_atk_ele & (1<<t_ele) || src_sd->def_ratio_atk_race & mask || src_sd->def_ratio_atk_enemy & (1<<t_enemy)) ) {
					damage = (damage * (t_def1 + t_def2))/100;
					idef_flag = 1;
				}
				if( !idef_flag_ && (src_sd->def_ratio_atk_ele_ & (1<<t_ele) || src_sd->def_ratio_atk_race_ & mask || src_sd->def_ratio_atk_enemy_ & (1<<t_enemy)) ) {
					damage2 = (damage2 * (t_def1 + t_def2))/100;
					idef_flag_ = 1;
					if(!idef_flag && battle_config.left_cardfix_to_right){
						damage = (damage * (t_def1 + t_def2))/100;
						idef_flag = 1;
					}
				}
			}
			break;
		}

		// 対象の防御力によるダメージの減少
		switch (skill_num) {
		case KN_AUTOCOUNTER:
		case CR_GRANDCROSS:
		case MO_INVESTIGATE:
		case MO_EXTREMITYFIST:
		case CR_ACIDDEMONSTRATION:
		case NJ_ZENYNAGE:
		case NPC_CRITICALSLASH:
			break;
		default:
			if(t_def1 < 1000000) {	//DEF, VIT無視
				int t_def;
				if(target->type!=BL_HOM) {
					target_count = unit_counttargeted(target,battle_config.vit_penaly_count_lv);
				}
				if(battle_config.vit_penaly_type > 0 && (t_sc_data?(t_sc_data[SC_STEELBODY].timer==-1):1)) {
					if(target_count >= battle_config.vit_penaly_count) {
						if(battle_config.vit_penaly_type == 1) {
							t_def1 = (t_def1 * (100 - (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num))/100;
							t_def2 = (t_def2 * (100 - (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num))/100;
							t_vit = (t_vit * (100 - (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num))/100;
						}
						else if(battle_config.vit_penaly_type == 2) {
							t_def1 -= (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num;
							t_def2 -= (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num;
							t_vit -= (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num;
						}
						if(t_def1 < 0) t_def1 = 0;
						if(t_def2 < 1) t_def2 = 1;
						if(t_vit < 1) t_vit = 1;
					}
				}
				t_def = t_def2*8/10;
				vitbonusmax = (t_vit/20)*(t_vit/20)-1;

				// シャープシューティングはCRI+20(計算済み)でDEF無視
				// 位置ここでいいのか…？
				if ((skill_num == SN_SHARPSHOOTING || skill_num == NJ_KIRIKAGE) && (atn_rand() % 1000) < cri)
				{
					idef_flag = idef_flag_ = 1;
				}

				//太陽と月と星の融合 DEF無視
				if(sc_data && sc_data[SC_FUSION].timer != -1)
					idef_flag = 1;

				if(!idef_flag){
					if(battle_config.player_defense_type) {
						damage = damage - (t_def1 * battle_config.player_defense_type) - t_def - ((vitbonusmax < 1)?0: atn_rand()%(vitbonusmax+1) );
						damage2 = damage2 - (t_def1 * battle_config.player_defense_type) - t_def - ((vitbonusmax < 1)?0: atn_rand()%(vitbonusmax+1) );
						damage_ot = damage_ot - (t_def1 * battle_config.player_defense_type) - t_def - ((vitbonusmax < 1)?0: atn_rand()%(vitbonusmax+1) );
						damage_ot2 = damage_ot2 - (t_def1 * battle_config.player_defense_type) - t_def - ((vitbonusmax < 1)?0: atn_rand()%(vitbonusmax+1) );
					}
					else{
						damage = damage * (100 - t_def1) /100 - t_def - ((vitbonusmax < 1)?0: atn_rand()%(vitbonusmax+1) );
						damage2 = damage2 * (100 - t_def1) /100 - t_def - ((vitbonusmax < 1)?0: atn_rand()%(vitbonusmax+1) );
						damage_ot = damage_ot * (100 - t_def1) /100;
						damage_ot2 = damage_ot2 * (100 - t_def1) /100;
					}
				}
			}
			break;
		}
	}

	// 状態異常中のダメージ追加でクリティカルにも有効なスキル
	if (sc_data) {
		//オーバートラスト
		if(sc_data[SC_OVERTHRUST].timer!=-1){	// オーバートラスト
			damage += damage_ot*(5*sc_data[SC_OVERTHRUST].val1)/100;
			damage2 += damage_ot2*(5*sc_data[SC_OVERTHRUST].val1)/100;
		}
		//オーバートラストマックス
		if(sc_data[SC_OVERTHRUSTMAX].timer!=-1){	// オーバートラストマックス
			damage += damage_ot*(20*sc_data[SC_OVERTHRUSTMAX].val1)/100;
			damage2 += damage_ot2*(20*sc_data[SC_OVERTHRUSTMAX].val1)/100;
		}
		// トゥルーサイト
		if(sc_data[SC_TRUESIGHT].timer!=-1){
			damage += damage*(2*sc_data[SC_TRUESIGHT].val1)/100;
			damage2 += damage2*(2*sc_data[SC_TRUESIGHT].val1)/100;
		}
		// バーサーク
		if(sc_data[SC_BERSERK].timer!=-1){
			damage += damage;
			damage2 += damage2;
		}
		// エンチャントデッドリーポイズン
		if (!no_cardfix && sc_data[SC_EDP].timer != -1) {
			// 右手のみに効果がのる。カード効果無効のスキルには乗らない
			if(map[src->m].flag.pk && target->type==BL_PC){
				damage += damage * (150 + sc_data[SC_EDP].val1 * 50) * battle_config.pk_edp_down_rate / 10000;
			}else if(map[src->m].flag.gvg){
				damage += damage * (150 + sc_data[SC_EDP].val1 * 50) * battle_config.gvg_edp_down_rate / 10000;
			}else if(map[src->m].flag.pvp){
				damage += damage * (150 + sc_data[SC_EDP].val1 * 50) * battle_config.pvp_edp_down_rate / 10000;
			}else{
				damage += damage * (150 + sc_data[SC_EDP].val1 * 50) / 100;
			}
			// no_cardfix = 1;
		}
		// サクリファイス
		if (src_sd && !skill_num && sc_data[SC_SACRIFICE].timer != -1 && status_get_class(target)!=1288) {
			int mhp = status_get_max_hp(src);
			int dmg = mhp * 9 / 100;
			pc_heal(src_sd, -dmg, 0);
			damage = dmg * (90 + sc_data[SC_SACRIFICE].val1 * 10) / 100;
			damage2 = 0;
			hitrate = 1000000;
			s_ele = 0;
			s_ele_ = 0;
			clif_misceffect2(src,366);
			sc_data[SC_SACRIFICE].val2 --;
			if (sc_data[SC_SACRIFICE].val2 == 0)
				status_change_end(src, SC_SACRIFICE,-1);
		}
	}

	// 精錬ダメージの追加
	if( src_sd ) {
		if(skill_num != MO_INVESTIGATE && skill_num != MO_EXTREMITYFIST && skill_num != PA_SHIELDCHAIN && skill_num != NJ_ZENYNAGE) {
			damage  += status_get_atk2(src);
			damage2 += status_get_atk_2(src);
		}
		switch (skill_num) {
		case CR_SHIELDBOOMERANG:
			if(src_sd->equip_index[8] >= 0) {
				int idx = src_sd->equip_index[8];
				if(src_sd->inventory_data[idx] && src_sd->inventory_data[idx]->type == 5) {
					damage += src_sd->inventory_data[idx]->weight/10;
					damage += src_sd->status.inventory[idx].refine * status_getrefinebonus(0,1);
				}
			}
			break;
		case LK_SPIRALPIERCE:		/* スパイラルピアース */
			if(src_sd->equip_index[9] >= 0) {	//{((STR/10)^2 ＋ 武器重量×スキル倍率×0.8) × サイズ補正 ＋ 精錬}×カード倍率×属性倍率×5の模様
				int idx = src_sd->equip_index[9];
				if(src_sd->inventory_data[idx] && src_sd->inventory_data[idx]->type == 4) {
					damage = ( ( (s_str/10)*(s_str/10) + src_sd->inventory_data[idx]->weight * (skill_lv * 4 + 8 ) / 100 )
								* (5 - t_size) / 4 + status_get_atk2(src) ) * 5;
				}
			}
			break;
		case PA_SHIELDCHAIN:		/* シールドチェイン*/
			if(src_sd->equip_index[8] >= 0) {
				int idx = src_sd->equip_index[8];
				if(src_sd->inventory_data[idx] && src_sd->inventory_data[idx]->type == 5) {
					int refinedamage = 2*(src_sd->status.inventory[idx].refine-4) + src_sd->status.inventory[idx].refine * src_sd->status.inventory[idx].refine;
					damage *= (100+30*skill_lv)/100;
					if(refinedamage>0)
						damage += atn_rand() % refinedamage;
					damage = (damage+100) * 5;
				}
			}
			break;
		case NJ_SYURIKEN:		/* 手裏剣投げ */
		case NJ_KUNAI:			/* クナイ投げ */
			damage += pc_checkskill(src_sd,NJ_TOBIDOUGU) * 3;
			break;
		}
	}

	// 0未満だった場合1に補正
	if(damage<1) damage=1;
	if(damage2<1) damage2=1;

	// スキル修正２（修練系）
	// 修練ダメージ(右手のみ) ソニックブロー時は別処理（1撃に付き1/8適応)
	if( src_sd && skill_num != MO_INVESTIGATE && skill_num != MO_EXTREMITYFIST && (skill_num != CR_GRANDCROSS||skill_num !=NPC_DARKGRANDCROSS)
			&& skill_num != LK_SPIRALPIERCE && skill_num != NJ_ZENYNAGE) {			//修練ダメージ無視
		damage = battle_addmastery(src_sd,target,damage,0);
		damage2 = battle_addmastery(src_sd,target,damage2,1);
	}
	if(sc_data &&sc_data[SC_AURABLADE].timer!=-1) {	//オーラブレードここに
		damage += sc_data[SC_AURABLADE].val1 * 20;
		damage2 += sc_data[SC_AURABLADE].val1 * 20;
	}
	if(sc_data &&sc_data[SC_GATLINGFEVER].timer!=-1) {	//ガトリングフィーバー
		damage += (20+sc_data[SC_GATLINGFEVER].val1*10);
		damage2 += (20+sc_data[SC_GATLINGFEVER].val1*10);
	}
	if(src_sd && src_sd->perfect_hit > 0) {
		if(atn_rand()%100 < src_sd->perfect_hit)
			hitrate = 1000000;
	}

	// 回避修正
	hitrate = (hitrate<battle_config.min_hitrate)?battle_config.min_hitrate:hitrate;
	if(	hitrate < 1000000 && // 必中攻撃
		(t_sc_data != NULL && (t_sc_data[SC_SLEEP].timer!=-1 ||	// 睡眠は必中
		t_sc_data[SC_STAN].timer!=-1 ||		// スタンは必中
		t_sc_data[SC_FREEZE].timer!=-1 || (t_sc_data[SC_STONE].timer!=-1 && t_sc_data[SC_STONE].val2==0) ) ) )	// 凍結は必中
		hitrate = 1000000;

	if(type == 0 && atn_rand()%100 >= hitrate) {
		damage = damage2 = 0;
		dmg_lv = ATK_FLEE;
	}else if(type == 0 && t_sc_data && t_sc_data[SC_KAUPE].timer !=-1 && atn_rand()%100 < (t_sc_data[SC_KAUPE].val2))//カウプ
	{
		damage = damage2 = 0;
		dmg_lv = ATK_FLEE;
		//カウプ終了処理
		if(t_sc_data[SC_KAUPE].timer!=-1)
			status_change_end(target,SC_KAUPE,-1);
	}else if(type == 0 && t_sc_data && t_sc_data[SC_UTSUSEMI].timer !=-1)//空蝉
	{
		damage = damage2 = 0;
		dmg_lv = ATK_FLEE;
		if((--t_sc_data[SC_UTSUSEMI].val3)==0)
			status_change_end(target,SC_UTSUSEMI,-1);
		if(t_sc_data && t_sc_data[SC_ANKLE].timer==-1) {
			int dir = 0, head_dir = 0;
			if(target_sd) {
				dir = target_sd->dir;
				head_dir = target_sd->head_dir;
			}
			unit_stop_walking(target,1);
			skill_blown(src,target,7|SAB_REVERSEBLOW);
			if(target_sd) {
				target_sd->dir = dir;
				target_sd->head_dir = head_dir;
			}
			if(t_sc_data && t_sc_data[SC_CLOSECONFINE].timer != -1)
				status_change_end(target,SC_CLOSECONFINE,-1);
		}
	}else if(type == 0 && t_sc_data && t_sc_data[SC_BUNSINJYUTSU].timer !=-1)//分身
	{
		damage = damage2 = 0;
		dmg_lv = ATK_FLEE;
		if((--t_sc_data[SC_BUNSINJYUTSU].val3)==0)
			status_change_end(target,SC_BUNSINJYUTSU,-1);
	}else if(target_sd && t_sc_data && (flag&BF_LONG || t_sc_data[SC_SPURT].timer!=-1) && t_sc_data[SC_DODGE].timer!=-1 && atn_rand()%100 < 20)//落法
	{
		int slv = pc_checkskill(target_sd,TK_DODGE);
		damage = damage2 = 0;
		dmg_lv = ATK_FLEE;
		clif_skill_nodamage(&target_sd->bl,&target_sd->bl,TK_DODGE,slv,1);
		status_change_start(&target_sd->bl,SC_DODGE_DELAY,slv,0,0,0,skill_get_time(TK_DODGE,slv),0);
	}
	else{
		dmg_lv = ATK_DEF;
	}

	// スキル修正３（武器研究）
	if( src_sd && (skill=pc_checkskill(src_sd,BS_WEAPONRESEARCH)) > 0) {
		damage+= skill*2;
		damage2+= skill*2;
	}
//スキルによるダメージ補正ここまで

//カードによるダメージ追加処理ここから
	cardfix=100;
	if(src_sd) {
		if(!src_sd->state.arrow_atk) { //弓矢以外
			if(!battle_config.left_cardfix_to_right) { //左手カード補正設定無し
				cardfix=cardfix*(100+src_sd->addrace[t_race])/100;	// 種族によるダメージ修正
				cardfix=cardfix*(100+src_sd->addele[t_ele])/100;	// 属性によるダメージ修正
				cardfix=cardfix*(100+src_sd->addenemy[t_enemy])/100;	// 敵タイプによるダメージ修正
				cardfix=cardfix*(100+src_sd->addsize[t_size])/100;	// サイズによるダメージ修正
				cardfix=cardfix*(100+src_sd->addgroup[t_group])/100;	// グループによるダメージ修正
			}
			else {
				cardfix=cardfix*(100+src_sd->addrace[t_race]+src_sd->addrace_[t_race])/100;	// 種族によるダメージ修正(左手による追加あり)
				cardfix=cardfix*(100+src_sd->addele[t_ele]+src_sd->addele_[t_ele])/100;		// 属性によるダメージ修正(左手による追加あり)
				cardfix=cardfix*(100+src_sd->addenemy[t_enemy]+src_sd->addenemy_[t_enemy])/100;	// 敵タイプによるダメージ修正(左手による追加あり)
				cardfix=cardfix*(100+src_sd->addsize[t_size]+src_sd->addsize_[t_size])/100;	// サイズによるダメージ修正(左手による追加あり)
				cardfix=cardfix*(100+src_sd->addgroup[t_group]+src_sd->addgroup_[t_group])/100;	// グループによるダメージ修正(左手による追加あり)
			}
		}
		else { //弓矢
			cardfix=cardfix*(100+src_sd->addrace[t_race]+src_sd->arrow_addrace[t_race])/100;	// 種族によるダメージ修正(弓矢による追加あり)
			cardfix=cardfix*(100+src_sd->addele[t_ele]+src_sd->arrow_addele[t_ele])/100;		// 属性によるダメージ修正(弓矢による追加あり)
			cardfix=cardfix*(100+src_sd->addenemy[t_enemy]+src_sd->arrow_addenemy[t_enemy])/100;	// 敵タイプによるダメージ修正(弓矢による追加あり)
			cardfix=cardfix*(100+src_sd->addsize[t_size]+src_sd->arrow_addsize[t_size])/100;	// サイズによるダメージ修正(弓矢による追加あり)
			cardfix=cardfix*(100+src_sd->addgroup[t_group]+src_sd->arrow_addgroup[t_group])/100;	// グループによるダメージ修正(弓矢による追加あり)
		}
		if(t_mode & 0x20) { //ボス
			if(!src_sd->state.arrow_atk) { //弓矢攻撃以外なら
				if(!battle_config.left_cardfix_to_right) //左手カード補正設定無し
					cardfix=cardfix*(100+src_sd->addrace[10])/100; //ボスモンスターに追加ダメージ
				else //左手カード補正設定あり
					cardfix=cardfix*(100+src_sd->addrace[10]+src_sd->addrace_[10])/100; //ボスモンスターに追加ダメージ(左手による追加あり)
			}
			else //弓矢攻撃
				cardfix=cardfix*(100+src_sd->addrace[10]+src_sd->arrow_addrace[10])/100; //ボスモンスターに追加ダメージ(弓矢による追加あり)
		}
		else { //ボスじゃない
			if(!src_sd->state.arrow_atk) { //弓矢攻撃以外
				if(!battle_config.left_cardfix_to_right) //左手カード補正設定無し
					cardfix=cardfix*(100+src_sd->addrace[11])/100; //ボス以外モンスターに追加ダメージ
				else //左手カード補正設定あり
					cardfix=cardfix*(100+src_sd->addrace[11]+src_sd->addrace_[11])/100; //ボス以外モンスターに追加ダメージ(左手による追加あり)
		}
			else
				cardfix=cardfix*(100+src_sd->addrace[11]+src_sd->arrow_addrace[11])/100; //ボス以外モンスターに追加ダメージ(弓矢による追加あり)
		}
		// カード効果による特定レンジ攻撃のダメージ増幅
		if(damage > 0){
			if(flag&BF_SHORT){
				cardfix = cardfix * (100+src_sd->short_weapon_damege_rate) / 100;
			}
			if(flag&BF_LONG){
				cardfix = cardfix * (100+src_sd->long_weapon_damege_rate) / 100;
			}
		}
		// カード効果による特定スキルのダメージ増幅（武器スキル）
		if(src_sd->skill_dmgup.count > 0 && (skill_num > 0) && (damage > 0)){
			for( i=0 ; i<src_sd->skill_dmgup.count ; i++ ){
				if( skill_num == src_sd->skill_dmgup.id[i] ){
					cardfix = cardfix*(100+src_sd->skill_dmgup.rate[i])/100;
					break;
				}
			}
		}
		//特定Class用補正処理(少女の日記→ボンゴン用？)
		t_class = status_get_class(target);
		for(i=0;i<src_sd->add_damage_class_count;i++) {
			if(src_sd->add_damage_classid[i] == t_class) {
				cardfix=cardfix*(100+src_sd->add_damage_classrate[i])/100;
				break;
			}
		}
		if (!no_cardfix)
			damage=damage*cardfix/100; //カード補正によるダメージ増加
	}
//カードによるダメージ増加処理ここまで

//カードによるダメージ追加処理(左手)ここから
	cardfix=100;
	if( src_sd ) {
		if(!battle_config.left_cardfix_to_right) {  //左手カード補正設定無し
			cardfix=cardfix*(100+src_sd->addrace_[t_race])/100;	// 種族によるダメージ修正左手
			cardfix=cardfix*(100+src_sd->addele_[t_ele])/100;	// 属性によるダメージ修正左手
			cardfix=cardfix*(100+src_sd->addenemy_[t_enemy])/100;	// 敵タイプによるダメージ修正左手
			cardfix=cardfix*(100+src_sd->addsize_[t_size])/100;	// サイズによるダメージ修正左手
			cardfix=cardfix*(100+src_sd->addgroup_[t_group])/100;	// グループによるダメージ修正左手
			if(t_mode & 0x20) //ボス
				cardfix=cardfix*(100+src_sd->addrace_[10])/100; //ボスモンスターに追加ダメージ左手
			else
				cardfix=cardfix*(100+src_sd->addrace_[11])/100; //ボス以外モンスターに追加ダメージ左手
		}
		//特定Class用補正処理左手(少女の日記→ボンゴン用？)
		for(i=0;i<src_sd->add_damage_class_count_;i++) {
			if(src_sd->add_damage_classid_[i] == t_class) {
				cardfix=cardfix*(100+src_sd->add_damage_classrate_[i])/100;
				break;
			}
		}
		if(!no_cardfix)
			damage2=damage2*cardfix/100; //カード補正による左手ダメージ増加
	}
//カードによるダメージ増加処理(左手)ここまで

//カードによるダメージ減衰処理ここから
	if(target_sd){ //対象がPCの場合
		int s_race  = status_get_race(src);
		int s_enemy = status_get_enemy_type(src);
		int s_size  = status_get_size(src);
		int s_group = status_get_group(src);
		cardfix=100;
		cardfix=cardfix*(100-target_sd->subrace[s_race])/100;		// 種族によるダメージ耐性
		if (s_ele >= 0)
			cardfix=cardfix*(100-target_sd->subele[s_ele])/100;	// 属性によるダメージ耐性
		if (s_ele == -1)
			cardfix=cardfix*(100-target_sd->subele[0])/100;		// 属性無しの耐性は無属性
		cardfix=cardfix*(100-target_sd->subenemy[s_enemy])/100;		// 敵タイプによるダメージ耐性
		cardfix=cardfix*(100-target_sd->subsize[s_size])/100;		// サイズによるダメージ耐性
		cardfix=cardfix*(100-target_sd->subgroup[s_group])/100;	// グループによるダメージ耐性

		if(status_get_mode(src) & 0x20)
			cardfix=cardfix*(100-target_sd->subrace[10])/100; //ボスからの攻撃はダメージ減少
		else
			cardfix=cardfix*(100-target_sd->subrace[11])/100; //ボス以外からの攻撃はダメージ減少

		//特定Class用補正処理左手(少女の日記→ボンゴン用？)
		for(i=0;i<target_sd->add_def_class_count;i++) {
			if(target_sd->add_def_classid[i] == status_get_class(src)) {
				cardfix=cardfix*(100-target_sd->add_def_classrate[i])/100;
				break;
			}
		}
		if(flag&BF_LONG)
			cardfix=cardfix*(100-target_sd->long_attack_def_rate)/100; //遠距離攻撃はダメージ減少(ホルンCとか)
		if(flag&BF_SHORT)
			cardfix=cardfix*(100-target_sd->near_attack_def_rate)/100; //近距離攻撃はダメージ減少(該当無し？)
		damage=damage*cardfix/100; //カード補正によるダメージ減少
		damage2=damage2*cardfix/100; //カード補正による左手ダメージ減少
	}
//カードによるダメージ減衰処理ここまで

//アイテムボーナスのフラグ処理ここから
	// 状態異常のレンジフラグ
	//   addeff_range_flag  0:指定無し 1:近距離 2:遠距離 3,4:それぞれのレンジで状態異常を発動させない
	//   flagがあり、攻撃タイプとflagが一致しないときは、flag+2する
	if(src_sd && flag&BF_WEAPON){
		int i;
		for(i=SC_STONE;i<=SC_BLEED;i++){
			if( (src_sd->addeff_range_flag[i-SC_STONE]==1 && flag&BF_LONG ) ||
				(src_sd->addeff_range_flag[i-SC_STONE]==2 && flag&BF_SHORT) ){
				src_sd->addeff_range_flag[i-SC_STONE]+=2;
			}
		}
	}
//アイテムボーナスのフラグ処理ここまで

//対象にステータス異常がある場合のダメージ減算処理ここから
	if(t_sc_data) {
		cardfix=100;
		if(t_sc_data[SC_DEFENDER].timer != -1 && flag&BF_LONG) //ディフェンダー状態で遠距離攻撃
			cardfix=cardfix*(100-t_sc_data[SC_DEFENDER].val2)/100; //ディフェンダーによる減衰
		if(t_sc_data[SC_ADJUSTMENT].timer != -1 && flag&BF_LONG) //アジャストメント状態で遠距離攻撃
			cardfix-=20; //アジャストメント状態による減衰
		if(cardfix != 100) {
			damage=damage*cardfix/100; //ディフェンダー補正によるダメージ減少
			damage2=damage2*cardfix/100; //ディフェンダー補正による左手ダメージ減少
		}
	}
//対象にステータス異常がある場合のダメージ減算処理ここまで

	if(damage < 0) damage = 0;
	if(damage2 < 0) damage2 = 0;

	// 属性の適用
	damage = battle_attr_fix(damage,s_ele, status_get_element(target));
	damage2 = battle_attr_fix(damage2,s_ele_, status_get_element(target));

	//ソウルブレイカー
	if (skill_num==ASC_BREAKER) {
		// intによる追加ダメージ
		damage += status_get_int(src) * skill_lv * 5;
		if (target_sd) {
			if (s_ele >= 0)
				damage = damage * (100-target_sd->subele[s_ele])/100;
			if (s_ele == -1)
				damage = damage * (100-target_sd->subele[0])/100;
		}
		// ランダムダメージ
		damage += 500 + (atn_rand() % 500);
		damage -= (t_def1 + t_def2 + vitbonusmax + status_get_mdef(target) + status_get_mdef2(target))/2;
	}

	// 星のかけら、気球の適用
	if(src_sd) {
		damage += src_sd->star;
		damage2 += src_sd->star_;
		damage += src_sd->spiritball*3;
		damage2 += src_sd->spiritball*3;
		damage += src_sd->coin*3;
		damage2 += src_sd->coin*3;
		damage += src_sd->bonus_damage;
		damage2 += src_sd->bonus_damage;
		damage  += src_sd->ranker_weapon_bonus;
		damage2 += src_sd->ranker_weapon_bonus_;
	}
	// 固定ダメージ
	if(src_sd && src_sd->special_state.fix_damage){
		damage=src_sd->fix_damage;
		damage2=src_sd->fix_damage;
	}

	if(skill_num==PA_PRESSURE){ /* プレッシャー 必中 */
		damage = 500+300*skill_lv;
		damage2 = 500+300*skill_lv;
	}

	// PC 以外の左手ダメージ無し
	if( !src_sd ) {
		damage2 = 0;
	}

	// >二刀流の左右ダメージ計算誰かやってくれぇぇぇぇえええ！
	// >map_session_data に左手ダメージ(atk,atk2)追加して
	// >pc_calcstatus()でやるべきかな？
	// map_session_data に左手武器(atk,atk2,ele,star,atkmods)追加して
	// pc_calcstatus()でデータを入力しています

	//左手のみ武器装備
	if(src_sd && src_sd->weapontype1 == 0 && src_sd->weapontype2 > 0) {
		damage = damage2;
		damage2 = 0;
	}

	// 右手、左手修練の適用
	if(skill_num==0){	//スキルに適応しない
		if(src_sd && src_sd->status.weapon > 22) {// 二刀流か?
			int dmg = damage, dmg2 = damage2;
			// 右手修練(60% ～ 100%) 右手全般
			skill = pc_checkskill(src_sd,AS_RIGHT);
			damage = damage * (50 + (skill * 10))/100;
			if(dmg > 0 && damage < 1) damage = 1;
			// 左手修練(40% ～ 80%) 左手全般
			skill = pc_checkskill(src_sd,AS_LEFT);
			damage2 = damage2 * (30 + (skill * 10))/100;
			if(dmg2 > 0 && damage2 < 1) damage2 = 1;
		}
		else //二刀流でなければ左手ダメージは0
			damage2 = 0;
	}
		// 右手,短剣のみ
	if(da == 1) { //ダブルアタックが発動しているか
		div_ = 2;
		damage += damage;
		type = 0x08;
	}

	if(da == 2) { //三段掌が発動しているか
		type = 0x08;
		div_ = 255;	//三段掌用に…
		//ダメージ計算は上で行う
	}
	if(da>=3)
	{
		type = 0x08;
		div_ = 248+da;
	}

	if(src_sd && src_sd->status.weapon == 16) {
		// アドバンスドカタール研究
		if((skill = pc_checkskill(src_sd,ASC_KATAR)) > 0) {
			damage += damage*(10+(skill * 2))/100;
		}
		// カタール追撃ダメージ
		skill = pc_checkskill(src_sd,TF_DOUBLE);
		damage2 = damage * (1 + (skill * 2))/100;
		if(damage > 0 && damage2 < 1) damage2 = 1;
	}

	// インベナム修正
	if(skill_num==TF_POISON){
		damage = battle_attr_fix(damage + 15*skill_lv, s_ele, status_get_element(target) );
	}
	if(skill_num==MC_CARTREVOLUTION){
		damage = battle_attr_fix(damage, 0, status_get_element(target) );
	}

	// 完全回避の判定
	if(skill_num == 0 && skill_lv >= 0 && target_sd!=NULL && div_ < 255 && atn_rand()%1000 < status_get_flee2(target) ){
		damage=damage2=0;
		type=0x0b;
		dmg_lv = ATK_LUCKY;
	}

	// 対象が完全回避をする設定がONなら
	if(battle_config.enemy_perfect_flee) {
		if(skill_num == 0 && skill_lv >= 0 && target_md!=NULL && div_ < 255 && atn_rand()%1000 < status_get_flee2(target) ) {
			damage=damage2=0;
			type=0x0b;
			dmg_lv = ATK_LUCKY;
		}
	}

	//MobのModeに頑強フラグが立っているときの処理
	if(t_mode&0x40 && skill_num!=PA_PRESSURE){
		if(damage > 0)
			damage = (div_<255)? 1: 3; // 三段掌のみ3ダメージ
		if(damage2 > 0)
			damage2 = 1;
	}

	//bNoWeaponDamage(設定アイテム無し？)でグランドクロスじゃない場合はダメージが0
	if( target_sd && target_sd->special_state.no_weapon_damage &&(skill_num != CR_GRANDCROSS||skill_num !=NPC_DARKGRANDCROSS))
		damage = damage2 = 0;

	if(skill_num != CR_GRANDCROSS||skill_num !=NPC_DARKGRANDCROSS) {
		if(damage2<1)		// ダメージ最終修正
			damage=battle_calc_damage(src,target,damage,div_,skill_num,skill_lv,flag);
		else if(damage<1)	// 右手がミス？
			damage2=battle_calc_damage(src,target,damage2,div_,skill_num,skill_lv,flag);
		else {	// 両手/カタールの場合はちょっと計算ややこしい
			int d1=damage+damage2,d2=damage2;
			damage=battle_calc_damage(src,target,damage+damage2,div_,skill_num,skill_lv,flag);
			damage2=(d2*100/d1)*damage/100;
			if(damage > 1 && damage2 < 1) damage2=1;
			damage-=damage2;
		}
	}

	//物理攻撃スキルによるオートスペル発動(item_bonus)
	if(flag&BF_SKILL && src && src->type == BL_PC && src != target && (damage+damage2)> 0)
	{
		if(skill_num==AM_DEMONSTRATION)
			asflag += EAS_MISC;
		else{
			if(flag&BF_LONG)
				asflag += EAS_LONG;
			else
				asflag += EAS_SHORT;
		}
		if(battle_config.weapon_attack_autospell)
			asflag += EAS_NORMAL;
		else
			asflag += EAS_SKILL;

		skill_bonus_autospell(src,target,asflag,gettick(),0);
	}

	//太陽と月と星の融合 HP2%消費
	if(src_sd && sc_data && sc_data[SC_FUSION].timer!=-1)
	{
		int hp;

		if(target->type == BL_PC)
		{
			hp = src_sd->status.max_hp * 8 / 100;
			if( src_sd->status.hp < (src_sd->status.max_hp * 20 / 100))	//対象がプレイヤーでHPが20％未満である時、攻撃をすれば即死します。
				hp = src_sd->status.hp;
		}else
			hp = src_sd->status.max_hp * 2 / 100;
		pc_heal(src_sd,-hp,0);
	}

	//カアヒ
	if(skill_num==0 && (damage + damage2)>0 && flag&BF_WEAPON && t_sc_data && t_sc_data[SC_KAAHI].timer!=-1)
	{
		int kaahi_lv = t_sc_data[SC_KAAHI].val1;
		if(target_sd)
		{
			if(target_sd->status.sp >= 5*kaahi_lv)
			{
				int hp,sp;
				sp = 5*kaahi_lv;
				hp = 200*kaahi_lv;
				if(hp || sp) pc_heal(target_sd,hp,-sp);
			}
		}
		else if(target_md)
		{
			mob_heal(target_md,200*kaahi_lv);
		}
		else if(target_hd)
		{
			homun_heal(target_hd,200*kaahi_lv,-5*kaahi_lv);
		}
	}

	wd.damage=damage;
	wd.damage2 = (skill_num == 0) ? damage2 : 0;
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
 * 魔法ダメージ計算
 *------------------------------------------
 */
struct Damage battle_calc_magic_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag)
{
	int mdef1,mdef2;
	int matk1,matk2,damage=0,div_=1;
	int blewcount=skill_get_blewcount(skill_num,skill_lv);
	struct Damage md;
	int aflag;
	int normalmagic_flag=1;
	int ele=0,race=7,t_ele=0,t_race=7,t_enemy=0,t_mode = 0,cardfix,t_class,i;
	struct map_session_data *sd=NULL,*tsd=NULL;
	struct mob_data *tmd = NULL;
	struct homun_data *thd = NULL;
	struct status_change *sc_data;
	struct status_change *t_sc_data;
	long asflag = EAS_ATTACK;

	//return前の処理があるので情報出力部のみ変更
	if( bl == NULL || target == NULL ){
		nullpo_info(NLP_MARK);
		memset(&md,0,sizeof(md));
		return md;
	}

	if(target->type == BL_PET) {
		memset(&md,0,sizeof(md));
		return md;
	}

	matk1 = status_get_matk1(bl);
	matk2 = status_get_matk2(bl);
	ele = skill_get_pl(skill_num);
	race = status_get_race(bl);

	mdef1 = status_get_mdef(target);
	mdef2 = status_get_mdef2(target);
	t_ele = status_get_elem_type(target);
	t_race = status_get_race(target);
	t_enemy = status_get_enemy_type(target);
	t_mode = status_get_mode(target);

#define MATK_FIX( a,b ) { matk1=matk1*(a)/(b); matk2=matk2*(a)/(b); }

	BL_CAST( BL_PC,  bl,     sd  );
	BL_CAST( BL_PC,  target, tsd );
	BL_CAST( BL_MOB, target, tmd );
	BL_CAST( BL_HOM, target, thd );

	if(sd) {
		sd->state.attack_type = BF_MAGIC;
		if(sd->matk_rate != 100)
			MATK_FIX(sd->matk_rate,100);
		sd->state.arrow_atk = 0;
	}

	aflag=BF_MAGIC|BF_LONG|BF_SKILL;

	sc_data = status_get_sc_data(bl);
	t_sc_data = status_get_sc_data(target);

	// 魔法力増幅によるMATK増加
	if (sc_data && sc_data[SC_MAGICPOWER].timer != -1) {
		matk1 += (matk1 * sc_data[SC_MAGICPOWER].val1 * 5)/100;
		matk2 += (matk2 * sc_data[SC_MAGICPOWER].val1 * 5)/100;
	}

	// 基本ダメージ計算(スキルごとに処理)
	switch(skill_num)
	{
		case AL_HEAL:	// ヒールor聖体
		case PR_BENEDICTIO:
			damage = skill_calc_heal(bl,skill_lv)/2;
			if(sd)	//メディタティオを乗せる
				damage += damage * pc_checkskill(sd,HP_MEDITATIO)*2/100;
			normalmagic_flag=0;
			break;
		case PR_ASPERSIO:		/* アスペルシオ */
			damage = 40; //固定ダメージ
			normalmagic_flag=0;
			break;
		case PR_SANCTUARY:	// サンクチュアリ
			ele = 6;
			damage = (skill_lv>6)?388:skill_lv*50;
			normalmagic_flag=0;
			blewcount|=0x10000;
			break;
		case PA_GOSPEL:		// ゴスペル(ランダムダメージ判定の場合)
			damage = 1000+atn_rand()%9000;
			normalmagic_flag=0;
			break;
		case ALL_RESURRECTION:
		case PR_TURNUNDEAD:	// 攻撃リザレクションとターンアンデッド
			if(battle_check_undead(t_race,t_ele)){
				int hp, mhp, thres;
				hp = status_get_hp(target);
				mhp = status_get_max_hp(target);
				thres = (skill_lv * 20) + status_get_luk(bl)+
						status_get_int(bl) + status_get_lv(bl)+
						((200 - hp * 200 / mhp));
				if(thres > 700) thres = 700;
				if(atn_rand()%1000 < thres && !(t_mode&0x20))	// 成功
					damage = hp;
				else					// 失敗
					damage = status_get_lv(bl) + status_get_int(bl) + skill_lv * 10;
			}
			normalmagic_flag=0;
			break;

		case HW_NAPALMVULCAN:	// ナパームバルカン
		case MG_NAPALMBEAT:	// ナパームビート（分散計算込み）
			MATK_FIX(70+ skill_lv*10,100);
			if(flag>0){
				MATK_FIX(1,flag);
			}else {
				if(battle_config.error_log)
					printf("battle_calc_magic_attack(): napam enemy count=0 !\n");
			}
			break;
		case MG_SOULSTRIKE:			/* ソウルストライク （対アンデッドダメージ補正）*/
			if(battle_check_undead(t_race,t_ele))
				MATK_FIX( 20+skill_lv,20 );//MATKに補正じゃ駄目ですかね？
			break;
		case MG_FIREBALL:	// ファイヤーボール
			if(flag>2)
				matk1=matk2=0;
			else{
				MATK_FIX((70+skill_lv*10),100);
				if(flag==2)
					MATK_FIX(3,4);
			}
			break;
		case MG_FIREWALL:	// ファイヤーウォール
			if((t_ele==3 || battle_check_undead(t_race,t_ele)) && target->type!=BL_PC)
				blewcount = 0;
			else
				blewcount |= 0x10000;
			MATK_FIX( 1,2 );
			break;
		case MG_THUNDERSTORM:	// サンダーストーム
			MATK_FIX( 80,100 );
			break;
		case MG_FROSTDIVER:	// フロストダイバ
			MATK_FIX( 100+skill_lv*10, 100);
			break;
		case WZ_FROSTNOVA:	// フロストノヴァ
			MATK_FIX((100+skill_lv*10)*2/3, 100);
			break;
		case WZ_FIREPILLAR:	// ファイヤーピラー
			if(mdef1 < 1000000)
				mdef1=mdef2=0;	// MDEF無視
			if(bl->type!=BL_MOB)
				MATK_FIX( 1,5 );
			matk1+=50;
			matk2+=50;
			break;
		case WZ_SIGHTRASHER:
			MATK_FIX( 100+skill_lv*20, 100);
			break;
		case WZ_METEOR:
		case WZ_JUPITEL:	// ユピテルサンダー
		case NPC_DARKJUPITEL:	//闇ユピテル
			break;
		case WZ_VERMILION:	// ロードオブバーミリオン
			MATK_FIX( skill_lv*20+80, 100 );
			break;
		case WZ_WATERBALL:	// ウォーターボール
			MATK_FIX( 100+skill_lv*30, 100 );
			break;
		case WZ_STORMGUST:	// ストームガスト
			MATK_FIX( skill_lv*40+100 ,100 );
//			blewcount|=0x10000;
			break;
		case AL_HOLYLIGHT:	// ホーリーライト
			MATK_FIX( 125,100 );
			if(sc_data && sc_data[SC_PRIEST].timer!=-1)
			{
				MATK_FIX( 500,100 );
			//	matk1 *= 5;
			//	matk2 *= 5;
			}
			break;
		case AL_RUWACH:
			MATK_FIX( 145,100 );
			break;
		case WZ_SIGHTBLASTER:
			MATK_FIX( 145,100 );
			break;
		case SL_STIN://エスティン
			if(status_get_size(target) == 0)
			{
				MATK_FIX(skill_lv*10,100);
			}
			else
			{
				MATK_FIX(skill_lv*1,100);
			}
			//ele = status_get_attack_element(bl);
			if(skill_lv>=7)
				status_change_start(bl,SC_SMA,skill_lv,0,0,0,3000,0);
			break;
		case SL_STUN://エスタン
			MATK_FIX(skill_lv*5,100);
			ele = status_get_attack_element(bl);
			if(skill_lv>=7)
				status_change_start(bl,SC_SMA,skill_lv,0,0,0,3000,0);
			break;
		case SL_SMA://エスマ
			if(sd && skill_lv==10)
			{
				MATK_FIX(40+sd->status.base_level,100);
			}
			ele = status_get_attack_element(bl);
			if(sc_data && sc_data[SC_SMA].timer!=-1)
				status_change_end(bl,SC_SMA,-1);
			break;
		case NJ_KOUENKA:	// 紅炎華
			MATK_FIX( 90,100 );
			break;
		case NJ_KAENSIN:	// 火炎陣
		case NJ_HUUJIN:		// 風刃
			break;
		case NJ_HYOUSENSOU:	// 氷閃槍
			if(t_sc_data && t_sc_data[SC_SUITON].timer!=-1)
			{
				MATK_FIX(100+t_sc_data[SC_SUITON].val1*2,100 );
			}
			break;
		case NJ_BAKUENRYU:	// 爆炎龍
			MATK_FIX(250+ skill_lv*150,300);
			break;
		case NJ_HYOUSYOURAKU:	// 氷晶落
			MATK_FIX(200+ skill_lv*50,100);
			break;
		case NJ_RAIGEKISAI:		// 雷撃砕
			MATK_FIX(160+ skill_lv*40,100);
			break;
		case NJ_KAMAITACHI:		// カマイタチ
			MATK_FIX(300+ skill_lv*100,100);
			break;
	}

	if(normalmagic_flag){	// 一般魔法ダメージ計算
		int imdef_flag=0;
		if(matk1>matk2)
			damage= matk2+atn_rand()%(matk1-matk2+1);
		else
			damage= matk2;
		if(sd) {
			int mask = (1<<t_race) | ( (t_mode&0x20)? (1<<10): (1<<11) );
			if(sd->ignore_mdef_ele & (1<<t_ele) || sd->ignore_mdef_race & mask || sd->ignore_mdef_enemy & (1<<t_enemy))
				imdef_flag = 1;
		}
		if(!imdef_flag){
			if(battle_config.magic_defense_type) {
				damage = damage - (mdef1 * battle_config.magic_defense_type) - mdef2;
			}
			else{
				damage = (damage*(100-mdef1))/100 - mdef2;
			}
		}
		if(damage<1) // プレイヤーの魔法スキルは1ダメージ保証無し
			damage=(!battle_config.skill_min_damage && bl->type == BL_PC)?0:1;
	}

	if(sd) {
		cardfix=100;
		cardfix=cardfix*(100+sd->magic_addrace[t_race])/100;
		cardfix=cardfix*(100+sd->magic_addele[t_ele])/100;
		cardfix=cardfix*(100+sd->magic_addenemy[t_enemy])/100;
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
		// カード効果による特定スキルのダメージ増幅（魔法スキル）
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
		cardfix=cardfix*(100-tsd->subele[ele])/100;	// 属性によるダメージ耐性
		cardfix=cardfix*(100-tsd->subrace[race])/100;	// 種族によるダメージ耐性
		cardfix=cardfix*(100-tsd->subenemy[status_get_enemy_type(bl)])/100;	// 敵タイプによるダメージ耐性
		cardfix=cardfix*(100-tsd->subsize[status_get_size( bl )])/100;	// サイズによるダメージ耐性
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

	damage=battle_attr_fix(damage, ele, status_get_element(target) );		// 属性修正

	if(skill_num == CR_GRANDCROSS || skill_num ==NPC_DARKGRANDCROSS) {	// グランドクロス
		static struct Damage wd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		wd=battle_calc_weapon_attack(bl,target,skill_num,skill_lv,flag);
		damage = (damage + wd.damage) * (100 + 40*skill_lv)/100;
		if(battle_config.gx_dupele) damage=battle_attr_fix(damage, ele, status_get_element(target) );	//属性2回かかる
		if(bl==target){
			if(bl->type == BL_MOB || bl->type == BL_HOM)
				damage = 0;		//MOB,HOMが使う場合は反動無し
		else
			 damage=damage/2;	//反動は半分
		}
	}

	if (skill_num==WZ_WATERBALL)
		div_ = 1;
	else
		div_=skill_get_num( skill_num,skill_lv );

	if( tsd && tsd->special_state.no_magic_damage )
		damage=0;	// 黄金蟲カード（魔法ダメージ０)

	//ヘルモードなら魔法ダメージなし
	if(t_sc_data && t_sc_data[SC_HERMODE].timer!=-1 && t_sc_data[SC_HERMODE].val1 == 1)
		damage = 0;

	if(skill_num==HW_GRAVITATION)	// グラビテーションフィールド
		damage = 200+skill_lv*200;

	if(damage != 0) {
		if(t_mode&0x40) { // 草・きのこ等
			// ロードオブヴァーミリオンはノーダメージ。それ以外は連打数ダメージ
			if (!battle_config.skill_min_damage && skill_num == WZ_VERMILION)
				damage = 0;
			else
				damage = (div_==255)? 3: div_;
		}
		else if(div_>1 && skill_num != WZ_VERMILION)
			damage*=div_;
	}

	//カイト
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

	damage=battle_calc_damage(bl,target,damage,div_,skill_num,skill_lv,aflag);	// 最終修正

	//魔法でもオートスペル発動(item_bonus)
	if(bl && bl->type == BL_PC && bl != target && damage > 0)
	{
		if(battle_config.magic_attack_autospell)
			asflag += EAS_SHORT|EAS_LONG;
		else
			asflag += EAS_MAGIC;

		skill_bonus_autospell(bl,target,asflag,gettick(),0);
	}

	//魔法でもHP/SP回復(月光剣など)
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
 * その他ダメージ計算
 *------------------------------------------
 */
struct Damage  battle_calc_misc_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag)
{
	int int_, dex;
	int skill,ele,race,cardfix;
	struct map_session_data *sd=NULL,*tsd=NULL;
	int damage=0,div_=1;
	int blewcount=skill_get_blewcount(skill_num,skill_lv);
	struct Damage md;
	int damagefix=1;
	long asflag = EAS_ATTACK;

	int aflag=BF_MISC|BF_SHORT|BF_SKILL;

	//return前の処理があるので情報出力部のみ変更
	if( bl == NULL || target == NULL ){
		nullpo_info(NLP_MARK);
		memset(&md,0,sizeof(md));
		return md;
	}

	if(target->type == BL_PET) {
		memset(&md,0,sizeof(md));
		return md;
	}

	BL_CAST( BL_PC, bl,     sd );
	BL_CAST( BL_PC, target, tsd );

	if( sd ) {
		sd->state.attack_type = BF_MISC;
		sd->state.arrow_atk = 0;
	}

	int_ = status_get_int(bl);
	dex  = status_get_dex(bl);
	race = status_get_race(bl);
	ele  = skill_get_pl(skill_num);

	switch(skill_num){

	case HT_LANDMINE:	// ランドマイン
		damage=skill_lv*(dex+75)*(100+int_)/100;
		break;

	case HT_BLASTMINE:	// ブラストマイン
		damage=skill_lv*(dex/2+50)*(100+int_)/100;
		break;

	case HT_CLAYMORETRAP:	// クレイモアートラップ
		damage=skill_lv*(dex/2+75)*(100+int_)/100;
		break;

	case HT_BLITZBEAT:	// ブリッツビート
		if( sd==NULL || (skill = pc_checkskill(sd,HT_STEELCROW)) <= 0)
			skill=0;
		damage=((int)dex/10 + (int)int_/2 + skill*3 + 40)*2;
		if(flag > 1)
			damage /= flag;
		flag &= ~(BF_SKILLMASK|BF_RANGEMASK|BF_WEAPONMASK);
		aflag = flag|(aflag&~BF_RANGEMASK)|BF_LONG;
		break;

	case TF_THROWSTONE:	// 石投げ
		damage=50;
		damagefix=0;
		flag &= ~(BF_SKILLMASK|BF_RANGEMASK|BF_WEAPONMASK);
		aflag = flag|(aflag&~BF_RANGEMASK)|BF_LONG;
		break;

	case BA_DISSONANCE:	// 不協和音
		damage=(skill_lv)*20+pc_checkskill(sd,BA_MUSICALLESSON)*3;
		break;
	case NPC_SELFDESTRUCTION:	// 自爆
	case NPC_SELFDESTRUCTION2:	// 自爆2
		damage=status_get_hp(bl)-(bl==target?1:0);
		damagefix=0;
		break;

	case NPC_SMOKING:	// タバコを吸う
		damage=3;
		damagefix=0;
		break;

	case NPC_DARKBREATH:
		{
			struct status_change *sc_data = status_get_sc_data(target);
			int hitrate=status_get_hit(bl) - status_get_flee(target) + 80;
			int t_hp=status_get_hp(target);
			hitrate = ( (hitrate>95)?95: ((hitrate<5)?5:hitrate) );
			if(sc_data && (sc_data[SC_SLEEP].timer!=-1 || sc_data[SC_STAN].timer!=-1 ||
				sc_data[SC_FREEZE].timer!=-1 || (sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2==0) ) )
				hitrate = 1000000;
			if(atn_rand()%100 < hitrate)
				damage = t_hp*(skill_lv*6)/100;
		}
		break;
	case SN_FALCONASSAULT:			/* ファルコンアサルト */
		if( sd==NULL || (skill = pc_checkskill(sd,HT_STEELCROW)) <= 0)
			skill=0;
		damage=(((int)dex/10+(int)int_/2+skill*3+40)*2*(150+skill_lv*70)/100)*5;
		if(sd && battle_config.allow_falconassault_elemet)
			ele = sd->atk_ele;
		flag &= ~(BF_WEAPONMASK|BF_RANGEMASK|BF_WEAPONMASK);
		aflag = flag|(aflag&~BF_RANGEMASK)|BF_LONG;
		break;
	default:
		damage = status_get_baseatk(bl);
		break;
	}

	if(damagefix){
		if(damage<1 && skill_num != NPC_DARKBREATH)
			damage=1;

		if( tsd ){
			cardfix=100;
			cardfix=cardfix*(100-tsd->subele[ele])/100;	// 属性によるダメージ耐性
			cardfix=cardfix*(100-tsd->subrace[race])/100;	// 種族によるダメージ耐性
			cardfix=cardfix*(100-tsd->subenemy[status_get_enemy_type(bl)])/100;	// 敵タイプによるダメージ耐性
			cardfix=cardfix*(100-tsd->subsize[status_get_size( bl )])/100;	// サイズによるダメージ耐性
			cardfix=cardfix*(100-tsd->misc_def_rate)/100;
			damage=damage*cardfix/100;
		}
		if(damage < 0) damage = 0;
		damage=battle_attr_fix(damage, ele, status_get_element(target) );		// 属性修正
	}

	div_=skill_get_num( skill_num,skill_lv );
	if(div_>1)
		damage*=div_;

	if(damage > 0 && (damage < div_ || (status_get_def(target) >= 1000000 && status_get_mdef(target) >= 1000000) ) ) {
		damage = div_;
	}

	if(status_get_mode(target)&0x40 && damage>0) // 草・きのこ等
		damage = 1;

	// カード効果による特定スキルのダメージ増幅（その他のスキル）
	if(sd && sd->skill_dmgup.count > 0 && skill_num > 0 && damage > 0){
		int i;
		for( i=0 ; i<sd->skill_dmgup.count ; i++ ){
			if( skill_num == sd->skill_dmgup.id[i] ){
				damage += damage * sd->skill_dmgup.rate[i] / 100;
				break;
			}
		}
	}

	damage=battle_calc_damage(bl,target,damage,div_,skill_num,skill_lv,aflag);	// 最終修正

	//miscでもオートスペル発動(bonus)
	if(bl->type == BL_PC && bl != target && damage > 0)
	{
		if(battle_config.misc_attack_autospell)
			asflag += EAS_SHORT|EAS_LONG;
		else
			asflag += EAS_MISC;

		skill_bonus_autospell(bl,target,asflag,gettick(),0);
	}

	//miscでもHP/SP回復(月光剣など)
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
/*==========================================
 * 攻撃によるHP/SP吸収
 *------------------------------------------
 */
int battle_attack_drain(struct block_list *bl,struct block_list *target,int damage,int damage2,int calc_per_drain_flag)
{
	int hp = 0,sp = 0;
	struct map_session_data* sd = NULL;

	nullpo_retr(0, bl);
	nullpo_retr(0, target);

	if( bl->type != BL_PC || (sd=(struct map_session_data *)bl) == NULL )
		return 0;

	if(bl == target)
		return 0;

	if(!(damage > 0 || damage2 >0))
		return 0;

	if(calc_per_drain_flag)//％吸収ものせる
	{
		if (!battle_config.left_cardfix_to_right) { // 二刀流左手カードの吸収系効果を右手に追加しない場合
			hp += battle_calc_drain(damage, sd->hp_drain_rate, sd->hp_drain_per, sd->hp_drain_value);
			hp += battle_calc_drain(damage2, sd->hp_drain_rate_, sd->hp_drain_per_, sd->hp_drain_value_);
			sp += battle_calc_drain(damage, sd->sp_drain_rate, sd->sp_drain_per, sd->sp_drain_value);
			sp += battle_calc_drain(damage2, sd->sp_drain_rate_, sd->sp_drain_per_, sd->sp_drain_value_);
		} else { // 二刀流左手カードの吸収系効果を右手に追加する場合
			int hp_drain_rate = sd->hp_drain_rate + sd->hp_drain_rate_;
			int hp_drain_per = sd->hp_drain_per + sd->hp_drain_per_;
			int hp_drain_value = sd->hp_drain_value + sd->hp_drain_value_;
			int sp_drain_rate = sd->sp_drain_rate + sd->sp_drain_rate_;
			int sp_drain_per = sd->sp_drain_per + sd->sp_drain_per_;
			int sp_drain_value = sd->sp_drain_value + sd->sp_drain_value_;
			hp += battle_calc_drain(damage, hp_drain_rate, hp_drain_per, hp_drain_value);
			sp += battle_calc_drain(damage, sp_drain_rate, sp_drain_per, sp_drain_value);
		}
	}else{//％吸収は乗せない
		if (!battle_config.left_cardfix_to_right) { // 二刀流左手カードの吸収系効果を右手に追加しない場合
			hp += battle_calc_drain(damage, sd->hp_drain_rate, 0, sd->hp_drain_value);
			hp += battle_calc_drain(damage2, sd->hp_drain_rate_, 0, sd->hp_drain_value_);
			sp += battle_calc_drain(damage, sd->sp_drain_rate, 0, sd->sp_drain_value);
			sp += battle_calc_drain(damage2, sd->sp_drain_rate_, 0, sd->sp_drain_value_);
		} else { // 二刀流左手カードの吸収系効果を右手に追加する場合
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
 * ダメージ計算一括処理用
 *------------------------------------------
 */
struct Damage battle_calc_attack( int attack_type,
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
 * 通常攻撃処理まとめ
 *------------------------------------------
 */
int battle_weapon_attack( struct block_list *src,struct block_list *target,unsigned int tick,int flag)
{
	struct map_session_data *sd=NULL,*tsd=NULL;
	struct status_change *sc_data, *t_sc_data;
	short *opt1;
	int race = 7, ele = 0;
	int damage,rdamage = 0;
	static struct Damage wd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	long asflag = EAS_ATTACK;

	nullpo_retr(0, src);
	nullpo_retr(0, target);

	if(src->prev == NULL || target->prev == NULL)
		return 0;
	if( unit_isdead(src) || unit_isdead(target) )
		return 0;

	BL_CAST( BL_PC, src,    sd );
	BL_CAST( BL_PC, target, tsd );

	opt1=status_get_opt1(src);
	if(opt1 && *opt1 > 0) {
		unit_stopattack(src);
		return 0;
	}

	sc_data = status_get_sc_data(src);
	t_sc_data = status_get_sc_data(target);

	//白羽中もしくはジャンプ中はダメ
	if(sc_data && (sc_data[SC_BLADESTOP].timer!=-1 || sc_data[SC_HIGHJUMP].timer!=-1)) {
		unit_stopattack(src);
		return 0;
	}

	//相手がジャンプ中
	if(t_sc_data && t_sc_data[SC_HIGHJUMP].timer!=-1){
		unit_stopattack(src);
		return 0;
	}

	if(battle_check_target(src,target,BCT_ENEMY) <= 0 && !battle_check_range(src,target,0))
		return 0;	// 攻撃対象外

	//ターゲットがMOB GMハイド中で、コンフィグでハイド中攻撃不可 GMレベルが指定より大きい場合
	if(target->type==BL_MOB && sd && sd->status.option&0x40 && battle_config.hide_attack == 0 && pc_isGM(sd)<battle_config.gm_hide_attack_lv)
		return 0;	// 隠れて攻撃するなんて卑怯なGMﾃﾞｽﾈ

	race = status_get_race(target);
	ele = status_get_elem_type(target);

	if(sd){
		if(!battle_delarrow(sd,1))
			return 0;
	}

	if(flag&0x8000) {
		if(sd && battle_config.pc_attack_direction_change)
			sd->dir = sd->head_dir = map_calc_dir(src, target->x,target->y );
		else if(src->type == BL_MOB && battle_config.monster_attack_direction_change)
			((struct mob_data *)src)->dir = map_calc_dir(src, target->x,target->y );
		else if(src->type == BL_HOM && battle_config.monster_attack_direction_change)	// homun_attack_direction_change
			((struct homun_data *)src)->dir = map_calc_dir(src, target->x,target->y );
		wd=battle_calc_weapon_attack(src,target,KN_AUTOCOUNTER,flag&0xff,0);
	} else
		wd=battle_calc_weapon_attack(src,target,0,0,0);

	if((damage = wd.damage + wd.damage2) > 0 && src != target) {
		if(wd.flag&BF_SHORT) {
			if(tsd && tsd->short_weapon_damage_return > 0) {
				rdamage += damage * tsd->short_weapon_damage_return / 100;
				if(rdamage < 1) rdamage = 1;
			}
			if(t_sc_data && t_sc_data[SC_REFLECTSHIELD].timer != -1) {
				rdamage += damage * t_sc_data[SC_REFLECTSHIELD].val2 / 100;
				if(rdamage < 1) rdamage = 1;
			}
		} else if(wd.flag&BF_LONG) {
			if(tsd && tsd->long_weapon_damage_return > 0) {
				rdamage += damage * tsd->long_weapon_damage_return / 100;
				if(rdamage < 1) rdamage = 1;
			}
		}
		if(rdamage > 0)
			clif_damage(src,src,tick,wd.amotion,wd.dmotion,rdamage,1,4,0);
	}

	if (wd.div_ == 255 && sd) { //三段掌
		int delay = 0;
		int skilllv;
		if(wd.damage+wd.damage2 < status_get_hp(target)) {
			if((skilllv = pc_checkskill(sd, MO_CHAINCOMBO)) > 0) {
				delay = 1000 - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
				delay += 300 * battle_config.combo_delay_rate /100;
				//コンボ入力時間の最低保障追加
				if( delay < battle_config.combo_delay_lower_limits )
					delay = battle_config.combo_delay_lower_limits;
			}
			status_change_start(src,SC_COMBO,MO_TRIPLEATTACK,skilllv,0,0,delay,0);
		}
		sd->ud.attackabletime = sd->ud.canmove_tick = tick + delay;
		clif_combo_delay(src,delay);
		clif_skill_damage(src , target , tick , wd.amotion , wd.dmotion ,
			wd.damage , 3 , MO_TRIPLEATTACK, pc_checkskill(sd,MO_TRIPLEATTACK) , -1 );

		//クローンスキル
		if(wd.damage> 0 && tsd && pc_checkskill(tsd,RG_PLAGIARISM) && sc_data && sc_data[SC_PRESERVE].timer == -1){
			skill_clone(tsd,MO_TRIPLEATTACK,pc_checkskill(sd, MO_TRIPLEATTACK));
		}
	}else if (wd.div_ >= 251 && wd.div_<=254 && sd)	{ //旋風
		int delay = 0;
		int skillid = TK_STORMKICK + 2*(wd.div_-251);
		int skilllv;
		delay = status_get_adelay(src);
		if(wd.damage+wd.damage2 < status_get_hp(target)) {
			if((skilllv = pc_checkskill(sd, skillid)) > 0) {
				delay += 500 * battle_config.combo_delay_rate /100;
			}
			status_change_start(src,SC_TKCOMBO,skillid,skilllv,0,0,delay,0);
		}
		sd->ud.attackabletime = sd->ud.canmove_tick = tick + delay;
		clif_combo_delay(src,delay);
		clif_skill_nodamage(&sd->bl,&sd->bl,skillid-1,pc_checkskill(sd,skillid-1),1);
	}
	else {
		clif_damage(src,target,tick, wd.amotion, wd.dmotion,
			wd.damage, wd.div_ , wd.type, wd.damage2);
	//二刀流左手とカタール追撃のミス表示(無理やり～)
		if(sd && (sd->status.weapon > 22 || sd->status.weapon==16) && wd.damage2 == 0)
			clif_damage(src,target,tick+10, wd.amotion, wd.dmotion,0, 1, 0, 0);
	}
	if(sd && sd->splash_range > 0 && (wd.damage > 0 || wd.damage2 > 0))
		skill_castend_damage_id(src,target,0,-1,tick,0);

	map_freeblock_lock();
	battle_delay_damage(tick+wd.amotion,src,target,(wd.damage+wd.damage2),0);
	if(target->prev != NULL &&
		(target->type != BL_PC || (target->type == BL_PC && !unit_isdead(target)))) {
		if(wd.damage > 0 || wd.damage2 > 0){
			skill_additional_effect(src,target,0,0,BF_WEAPON,tick);
			if(sd) {
				if(sd->break_weapon_rate > 0 && atn_rand()%10000 < sd->break_weapon_rate && tsd)
					pc_break_equip(tsd, EQP_WEAPON);
				if(sd->break_armor_rate > 0 && atn_rand()%10000 < sd->break_armor_rate && tsd)
					pc_break_equip(tsd, EQP_ARMOR);
			}
		}
	}
	if(sc_data && sc_data[SC_AUTOSPELL].timer != -1 && atn_rand()%100 < sc_data[SC_AUTOSPELL].val4) {
		int skilllv = sc_data[SC_AUTOSPELL].val3;
		int i = atn_rand()%100, f = 0;

		if(i >= 50) skilllv -= 2;
		else if(i >= 15) skilllv--;
		if(skilllv < 1) skilllv = 1;
		//PCかつセージの魂
		if(sd && sd->sc_data[SC_SAGE].timer!=-1)
			skilllv = pc_checkskill(sd,sc_data[SC_AUTOSPELL].val2);

		if(sd) {
			int sp = skill_get_sp(sc_data[SC_AUTOSPELL].val2,skilllv)*2/3;
			if(sd->status.sp >= sp) {
				if((skill_get_inf(sd->autospell_id) == 2) || (skill_get_inf(sd->autospell_id) == 32))
					f = skill_castend_pos2(src,target->x,target->y,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
				else {
					switch( skill_get_nk(sc_data[SC_AUTOSPELL].val2) ) {
						case 0:
						case 2:
						case 4:
							f = skill_castend_damage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
							break;
						case 1:/* 支援系 */
							if((sc_data[SC_AUTOSPELL].val2==AL_HEAL || (sc_data[SC_AUTOSPELL].val2==ALL_RESURRECTION && target->type != BL_PC)) && battle_check_undead(race,ele))
								f = skill_castend_damage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
							else
								f = skill_castend_nodamage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
						default:
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
					case 1:/* 支援系 */
						if((sc_data[SC_AUTOSPELL].val2==AL_HEAL || (sc_data[SC_AUTOSPELL].val2==ALL_RESURRECTION && target->type != BL_PC)) && battle_check_undead(race,ele))
							skill_castend_damage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
						else
							skill_castend_nodamage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
					default:
						break;
				}
			}
		}
	}

	//カードによるオートスペル
	if(sd && sd->bl.type == BL_PC && src != target 	&& (wd.damage > 0 || wd.damage2 > 0))
	{
		asflag += EAS_NORMAL;
		if(wd.flag&BF_LONG)
			asflag += EAS_LONG;
		else
			asflag += EAS_SHORT;

		skill_bonus_autospell(src,target,asflag,gettick(),0);
	}

	if(sd && sd->bl.type == BL_PC && src != target && wd.flag&BF_WEAPON && (wd.damage > 0 || wd.damage2 > 0))
	{
		//SP消失
		if(tsd && atn_rand()%100 < sd->sp_vanish_rate)
		{
			int sp = 0;
			sp = status_get_sp(target)* sd->sp_vanish_per/100;
			if (sp > 0)
				pc_heal(tsd, 0, -sp);
		}
	}

	if(sd){
		if (wd.flag&BF_WEAPON && src != target && (wd.damage > 0 || wd.damage2 > 0)) {
			int hp = 0,sp = 0;
			if (!battle_config.left_cardfix_to_right) { // 二刀流左手カードの吸収系効果を右手に追加しない場合
				hp += battle_calc_drain(wd.damage, sd->hp_drain_rate, sd->hp_drain_per, sd->hp_drain_value);
				hp += battle_calc_drain(wd.damage2, sd->hp_drain_rate_, sd->hp_drain_per_, sd->hp_drain_value_);
				sp += battle_calc_drain(wd.damage, sd->sp_drain_rate, sd->sp_drain_per, sd->sp_drain_value);
				sp += battle_calc_drain(wd.damage2, sd->sp_drain_rate_, sd->sp_drain_per_, sd->sp_drain_value_);
			} else { // 二刀流左手カードの吸収系効果を右手に追加する場合
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

	if(rdamage > 0){
		battle_delay_damage(tick+wd.amotion,target,src,rdamage,0);

		//反射ダメージのオートスペル
		if(battle_config.weapon_reflect_autospell && target->type == BL_PC)
			skill_bonus_autospell(target,src,EAS_ATTACK,gettick(),0);

		if(battle_config.weapon_reflect_drain)
			battle_attack_drain(target,src,rdamage,0,battle_config.weapon_reflect_drain_per_enable);
	}
	if(t_sc_data && t_sc_data[SC_AUTOCOUNTER].timer != -1 && t_sc_data[SC_AUTOCOUNTER].val4 > 0) {
		if(t_sc_data[SC_AUTOCOUNTER].val3 == src->id)
			battle_weapon_attack(target,src,tick,0x8000|t_sc_data[SC_AUTOCOUNTER].val1);
		status_change_end(target,SC_AUTOCOUNTER,-1);
	}

	if (t_sc_data && t_sc_data[SC_BLADESTOP_WAIT].timer != -1 &&
			!(status_get_mode(src)&0x20)) { // ボスには無効
		int lv = t_sc_data[SC_BLADESTOP_WAIT].val1;
		status_change_end(target,SC_BLADESTOP_WAIT,-1);
		status_change_start(src,SC_BLADESTOP,lv,1,(int)src,(int)target,skill_get_time2(MO_BLADESTOP,lv),0);
		status_change_start(target,SC_BLADESTOP,lv,2,(int)target,(int)src,skill_get_time2(MO_BLADESTOP,lv),0);
	}

	if (t_sc_data && t_sc_data[SC_POISONREACT].timer != -1) {
		// 毒属性mobまたは毒属性による攻撃ならば反撃
		if( (src->type==BL_MOB && status_get_elem_type(src) == 5) || status_get_attack_element(src) == 5 ) {
			if( battle_check_range(target,src,status_get_range(target)+1) ) {
				t_sc_data[SC_POISONREACT].val2 = 0;
				battle_skill_attack(BF_WEAPON,target,target,src,AS_POISONREACT,t_sc_data[SC_POISONREACT].val1,tick,0);
			}
		}
		// それ以外の通常攻撃に対するインベ反撃（射線チェックなし）
		else {
			--t_sc_data[SC_POISONREACT].val2;
			if(atn_rand()&1) {
				if( tsd==NULL || pc_checkskill(tsd,TF_POISON)>=5 )
					battle_skill_attack(BF_WEAPON,target,target,src,TF_POISON,5,tick,flag);
			}
		}
		if (t_sc_data[SC_POISONREACT].val2 <= 0)
			status_change_end(target,SC_POISONREACT,-1);
	}
	map_freeblock_unlock();
	return wd.dmg_lv;
}

/*
 * =========================================================================
 * スキル攻撃効果処理まとめ
 * flagの説明。16進図
 * 	00XRTTff
 *  ff	= magicで計算に渡される
 *	TT	= パケットのtype部分(0でデフォルト)
 *  X   = パケットのスキルLv
 *  R	= 予約（skill_area_subで使用する)
 *-------------------------------------------------------------------------
 */
int battle_skill_attack(int attack_type,struct block_list* src,struct block_list *dsrc,
	 struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag)
{
	struct Damage dmg;
	struct status_change *sc_data;
	struct status_change *ssc_data;
	int type,lv,damage, rdamage = 0;

	nullpo_retr(0, src);
	nullpo_retr(0, dsrc);
	nullpo_retr(0, bl);

	sc_data = status_get_sc_data(bl);
	ssc_data = status_get_sc_data(src);

//何もしない判定ここから
	if(dsrc->m != bl->m) //対象が同じマップにいなければ何もしない
		return 0;
	if(src->prev == NULL || dsrc->prev == NULL || bl->prev == NULL) //prevよくわからない※
		return 0;
	if( unit_isdead(src)) //術者？がPCですでに死んでいたら何もしない
		return 0;
	if( unit_isdead(dsrc)) //術者？がPCですでに死んでいたら何もしない
		return 0;
	if( unit_isdead(bl)) //対象がPCですでに死んでいたら何もしない
		return 0;

	if(sc_data && sc_data[SC_HIDING].timer != -1) { //ハイディング状態で
		if(skill_get_pl(skillid) != 2) //スキルの属性が地属性でなければ何もしない
			return 0;
	}

	//矢の消費
	/*
	if(src->type == BL_PC && skill_get_arrow_cost(skillid,skilllv)>0 && flag == 0){
		struct map_session_data* sd = (struct map_session_data*)src;
		if(sd->equip_index[10] >= 0 &&
			sd->status.inventory[sd->equip_index[10]].amount >= skill_get_arrow_cost(skillid,skilllv) &&
			sd->inventory_data[sd->equip_index[10]]->arrow_type&skill_get_arrow_type(skillid))
		{
			if(battle_config.arrow_decrement)
				pc_delitem(sd,sd->equip_index[10],skill_get_arrow_cost(skillid,skilllv),0);
		}else {
			clif_arrow_fail(sd,0);
			return 0;
		}
	}
	*/
	if(src->type == BL_PC && skill_get_arrow_cost(skillid,skilllv)>0)
	{
		struct map_session_data* sd = (struct map_session_data*)src;
		switch(skillid)
		{
		case AC_DOUBLE:
		case AC_CHARGEARROW:
		case BA_MUSICALSTRIKE:
		case DC_THROWARROW:
		case CG_ARROWVULCAN:
		case AS_VENOMKNIFE:	//ベナムナイフ消費
		case NJ_SYURIKEN:	//手裏剣消費
		case NJ_KUNAI:	//クナイ消費
		case GS_TRACKING:	//銃弾消費
		case GS_DISARM:
		case GS_PIERCINGSHOT:
		case GS_DUST:
		case GS_DESPERADO:
		case GS_RAPIDSHOWER:	//複数消費
		case GS_FULLBUSTER:
		case GS_GROUNDDRIFT:	//グレネード弾消費
			if(sd->equip_index[10] >= 0 &&
				sd->status.inventory[sd->equip_index[10]].amount >= skill_get_arrow_cost(skillid,skilllv) &&
				sd->inventory_data[sd->equip_index[10]]->arrow_type&skill_get_arrow_type(skillid))
			{
				if(battle_config.arrow_decrement)
					pc_delitem(sd,sd->equip_index[10],skill_get_arrow_cost(skillid,skilllv),0);
			}else {
				clif_arrow_fail(sd,0);
				return 0;
			}
			break;
		case AC_SHOWER:
		case SN_SHARPSHOOTING:
		case GS_SPREADATTACK:
			if(flag == 0){
				if(sd->equip_index[10] >= 0 &&
					sd->status.inventory[sd->equip_index[10]].amount >= skill_get_arrow_cost(skillid,skilllv) &&
					sd->inventory_data[sd->equip_index[10]]->arrow_type&skill_get_arrow_type(skillid))
				{
					if(battle_config.arrow_decrement)
						pc_delitem(sd,sd->equip_index[10],skill_get_arrow_cost(skillid,skilllv),0);
				}else {
					clif_arrow_fail(sd,0);
					return 0;
				}
			}
			break;
		}
	}

	if(sc_data) {
		if(sc_data[SC_CHASEWALK].timer != -1 && skillid == AL_RUWACH)	//チェイスウォーク状態でルアフ無効
			return 0;
		if(sc_data[SC_TRICKDEAD].timer != -1) 				//死んだふり中は何もしない
			return 0;
		if(sc_data[SC_HIGHJUMP].timer != -1) 				//高跳び中は何もしない
			return 0;
		//凍結状態でストームガスト、フロストノヴァ、氷衝落は無効
		if(sc_data[SC_FREEZE].timer != -1 && (skillid == WZ_STORMGUST || skillid == WZ_FROSTNOVA || skillid == NJ_HYOUSYOURAKU))
			return 0;
	}
	if(skillid == WZ_FROSTNOVA && dsrc->x == bl->x && dsrc->y == bl->y) //使用スキルがフロストノヴァで、dsrcとblが同じ場所なら何もしない
		return 0;
	if(src->type == BL_PC && ((struct map_session_data *)src)->chatID) //術者がPCでチャット中なら何もしない
		return 0;
	if(dsrc->type == BL_PC && ((struct map_session_data *)dsrc)->chatID) //術者がPCでチャット中なら何もしない
		return 0;
	if(src->type == BL_PC && bl && mob_gvmobcheck(((struct map_session_data *)src),bl)==0)
		return 0;

//何もしない判定ここまで

	type=-1;
	lv=(flag>>20)&0xf;
	dmg=battle_calc_attack(attack_type,src,bl,skillid,skilllv,flag&0xff ); //ダメージ計算

//マジックロッド処理ここから
	if(attack_type&BF_MAGIC && sc_data && sc_data[SC_MAGICROD].timer != -1 && src == dsrc) { //魔法攻撃でマジックロッド状態でsrc=dsrcなら
		dmg.damage = dmg.damage2 = 0; //ダメージ0
		if(bl->type == BL_PC) { //対象がPCの場合
			int sp = skill_get_sp(skillid,skilllv); //使用されたスキルのSPを吸収
			sp = sp * sc_data[SC_MAGICROD].val2 / 100; //吸収率計算
			if(skillid == WZ_WATERBALL && skilllv > 1) //ウォーターボールLv1以上
				sp = sp/((skilllv|1)*(skilllv|1)); //さらに計算？
			if(sp > 0x7fff) sp = 0x7fff; //SP多すぎの場合は理論最大値
			else if(sp < 1) sp = 1; //1以下の場合は1
			if(((struct map_session_data *)bl)->status.sp + sp > ((struct map_session_data *)bl)->status.max_sp) { //回復SP+現在のSPがMSPより大きい場合
				sp = ((struct map_session_data *)bl)->status.max_sp - ((struct map_session_data *)bl)->status.sp; //SPをMSP-現在SPにする
				((struct map_session_data *)bl)->status.sp = ((struct map_session_data *)bl)->status.max_sp; //現在のSPにMSPを代入
			}
			else //回復SP+現在のSPがMSPより小さい場合は回復SPを加算
				((struct map_session_data *)bl)->status.sp += sp;
			clif_heal(((struct map_session_data *)bl)->fd,SP_SP,sp); //SP回復エフェクトの表示
			((struct map_session_data *)bl)->ud.canact_tick = tick + skill_delayfix(bl, skill_get_delay(SA_MAGICROD,sc_data[SC_MAGICROD].val1), skill_get_cast(SA_MAGICROD,sc_data[SC_MAGICROD].val1)); //
		}
		clif_skill_nodamage(bl,bl,SA_MAGICROD,sc_data[SC_MAGICROD].val1,1); //マジックロッドエフェクトを表示
	}
//マジックロッド処理ここまで

	damage = dmg.damage + dmg.damage2;

	if(lv==15)
		lv=-1;

	if( flag&0xff00 )
		type=(flag&0xff00)>>8;

	if(damage <= 0 || damage < dmg.div_) //吹き飛ばし判定？※
		dmg.blewcount = 0;

	//dmg.blewcount = 5;

	if(skillid == CR_GRANDCROSS || skillid == NPC_DARKGRANDCROSS) {//グランドクロス
		if(battle_config.gx_disptype) dsrc = src;	// 敵ダメージ白文字表示
		if( src == bl) type = 4;	// 反動はダメージモーションなし
	}

//使用者がPCの場合の処理ここから
	if(src->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)src;
		nullpo_retr(0, sd);
//連打掌(MO_CHAINCOMBO)ここから
		if(skillid == MO_CHAINCOMBO) {
			int delay = 1000 - 4 * status_get_agi(src) - 2 *  status_get_dex(src); //基本ディレイの計算
			if(damage < status_get_hp(bl)) { //ダメージが対象のHPより小さい場合
				if(pc_checkskill(sd, MO_COMBOFINISH) > 0 && sd->spiritball > 0){ //猛龍拳(MO_COMBOFINISH)取得＆気球保持時は+300ms
					delay += 300 * battle_config.combo_delay_rate /100; //追加ディレイをconfにより調整
					//コンボ入力時間の最低保障追加
					if(delay < battle_config.combo_delay_lower_limits)
						delay = battle_config.combo_delay_lower_limits;
				}
				status_change_start(src,SC_COMBO,MO_CHAINCOMBO,skilllv,0,0,delay,0); //コンボ状態に
			}
			sd->ud.attackabletime = sd->ud.canmove_tick = tick + delay;
			clif_combo_delay(src,delay); //コンボディレイパケットの送信
		}
//連打掌(MO_CHAINCOMBO)ここまで
//猛龍拳(MO_COMBOFINISH)ここから
		else if(skillid == MO_COMBOFINISH) {
			int delay = 700 - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
			if(damage < status_get_hp(bl)) {
				//阿修羅覇凰拳(MO_EXTREMITYFIST)取得＆気球4個保持＆爆裂波動(MO_EXPLOSIONSPIRITS)状態時は+300ms
				//伏虎拳(CH_TIGERFIST)取得時も+300ms
				if((pc_checkskill(sd, MO_EXTREMITYFIST) > 0 && sd->spiritball >= 4 && sd->sc_data[SC_EXPLOSIONSPIRITS].timer != -1) ||
				(pc_checkskill(sd, CH_TIGERFIST) > 0 && sd->spiritball > 0) ||
				(pc_checkskill(sd, CH_CHAINCRUSH) > 0 && sd->spiritball > 1))
				{
					delay += 300 * battle_config.combo_delay_rate /100; //追加ディレイをconfにより調整
					//コンボ入力時間最低保障追加
					if(delay < battle_config.combo_delay_lower_limits)
						delay = battle_config.combo_delay_lower_limits;
				}
				status_change_start(src,SC_COMBO,MO_COMBOFINISH,skilllv,0,0,delay,0); //コンボ状態に
			}
			sd->ud.attackabletime = sd->ud.canmove_tick = tick + delay;
			clif_combo_delay(src,delay); //コンボディレイパケットの送信
		}
//猛龍拳(MO_COMBOFINISH)ここまで
//伏虎拳(CH_TIGERFIST)ここから
		else if(skillid == CH_TIGERFIST) {
			int delay = 1000 - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
			if(damage < status_get_hp(bl)) {
				if(pc_checkskill(sd, CH_CHAINCRUSH) > 0){ //連柱崩撃(CH_CHAINCRUSH)取得時は+300ms
					delay += 300 * battle_config.combo_delay_rate /100; //追加ディレイをconfにより調整
					//コンボ入力時間最低保障追加
					if(delay < battle_config.combo_delay_lower_limits)
						delay = battle_config.combo_delay_lower_limits;
				}

				status_change_start(src,SC_COMBO,CH_TIGERFIST,skilllv,0,0,delay,0); //コンボ状態に
			}
			sd->ud.attackabletime = sd->ud.canmove_tick = tick + delay;
			clif_combo_delay(src,delay); //コンボディレイパケットの送信
		}
//伏虎拳(CH_TIGERFIST)ここまで
//連柱崩撃(CH_CHAINCRUSH)ここから
		else if(skillid == CH_CHAINCRUSH) {
			int delay = 1000 - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
			if(damage < status_get_hp(bl)) {
				//伏虎拳習得または阿修羅習得＆気球1個保持＆爆裂波動時ディレイ
				if(pc_checkskill(sd, CH_TIGERFIST) > 0 || (pc_checkskill(sd, MO_EXTREMITYFIST) > 0 && sd->spiritball >= 1 && sd->sc_data[SC_EXPLOSIONSPIRITS].timer != -1))
				{
					delay += (600+(skilllv/5)*200) * battle_config.combo_delay_rate /100; //追加ディレイをconfにより調整
					//コンボ入力時間最低保障追加
					if(delay < battle_config.combo_delay_lower_limits)
						delay = battle_config.combo_delay_lower_limits;
				}
				status_change_start(src,SC_COMBO,CH_CHAINCRUSH,skilllv,0,0,delay,0); //コンボ状態に
			}
			sd->ud.attackabletime = sd->ud.canmove_tick = tick + delay;
			clif_combo_delay(src,delay); //コンボディレイパケットの送信
		}
//連柱崩撃(CH_CHAINCRUSH)ここまで

		//TKコンボ
		{
			int tk_flag=0;
			if(skillid == TK_STORMKICK){
				tk_flag=1;
				if(ssc_data)
					ssc_data[SC_TKCOMBO].val4 |= 0x01;
			}else if(skillid == TK_DOWNKICK){
				tk_flag=1;
				if(ssc_data)
					ssc_data[SC_TKCOMBO].val4 |= 0x02;
			}else if(skillid == TK_TURNKICK){
				tk_flag=1;
				if(ssc_data)
					ssc_data[SC_TKCOMBO].val4 |= 0x04;
			}else if(skillid == TK_COUNTER){
				tk_flag=1;
				if(ssc_data)
					ssc_data[SC_TKCOMBO].val4 |= 0x08;
			}

			if(tk_flag && ssc_data)
			{
				if(sd->status.class != PC_CLASS_TK || ssc_data[SC_TKCOMBO].val4&~0x0F){
					//4つとも出したので終了
					status_change_end(src,SC_TKCOMBO,-1); //TKコンボ終了
				}else if(sd && ranking_get_pc_rank(sd,RK_TAEKWON)>0){
					int delay = status_get_adelay(src);
					if(damage < status_get_hp(bl)) {
						delay += 500 * battle_config.combo_delay_rate /100;
						status_change_start(src,SC_TKCOMBO,TK_MISSION,1,0,ssc_data[SC_TKCOMBO].val4,delay,0);
					}
					sd->ud.attackabletime = sd->ud.canmove_tick = tick + delay;
					clif_combo_delay(src,delay);
				}
			}
		}
	}
//使用者がPCの場合の処理ここまで
//武器スキル？ここから
	if(attack_type&BF_WEAPON && damage > 0 && src != bl && src == dsrc) { //武器スキル＆ダメージあり＆使用者と対象者が違う＆src=dsrc
		if(dmg.flag&BF_SHORT) { //近距離攻撃時？※
			if(bl->type == BL_PC) { //対象がPCの時
				struct map_session_data *tsd = (struct map_session_data *)bl;
				nullpo_retr(0, tsd);
				if(tsd->short_weapon_damage_return > 0) { //近距離攻撃跳ね返し？※
					rdamage += damage * tsd->short_weapon_damage_return / 100;
					if(rdamage < 1) rdamage = 1;
				}
			}
			if(sc_data && sc_data[SC_REFLECTSHIELD].timer != -1) { //リフレクトシールド時
				rdamage += damage * sc_data[SC_REFLECTSHIELD].val2 / 100; //跳ね返し計算
				if(rdamage < 1) rdamage = 1;
			}
		}
		else if(dmg.flag&BF_LONG) { //遠距離攻撃時？※
			if(bl->type == BL_PC) { //対象がPCの時
				struct map_session_data *tsd = (struct map_session_data *)bl;
				nullpo_retr(0, tsd);
				if(tsd->long_weapon_damage_return > 0) { //遠距離攻撃跳ね返し？※
					rdamage += damage * tsd->long_weapon_damage_return / 100;
					if(rdamage < 1) rdamage = 1;
				}
			}
		}
		if(rdamage > 0)
			clif_damage(src,src,tick, dmg.amotion,0,rdamage,1,4,0);
	}
	if(attack_type&BF_MAGIC && damage > 0 && src != bl && src == dsrc) { //魔法スキル＆ダメージあり＆使用者と対象者が違う
		if(bl->type == BL_PC) { //対象がPCの時
			struct map_session_data *tsd = (struct map_session_data *)bl;
			nullpo_retr(0, tsd);
			if(tsd->magic_damage_return > 0 && atn_rand()%100 < tsd->magic_damage_return) { //魔法攻撃跳ね返し？※
				rdamage = damage;
				damage  = 0;
			}
		}
		//カイト
		if(damage > 0 && sc_data && sc_data[SC_KAITE].timer!=-1)
		{
			if(src->type == BL_PC || status_get_lv(src) < 80)
			{
				sc_data[SC_KAITE].val2--;
				if(sc_data[SC_KAITE].val2==0)
					status_change_end(bl,SC_KAITE,-1);

				if(src->type==BL_PC && ssc_data && ssc_data[SC_WIZARD].timer!=-1)
				{
					struct map_session_data* ssd = (struct map_session_data* )src;
					int idx = pc_search_inventory(ssd,7321);
					if(idx!=-1 && ssd->status.inventory[idx].amount > 0)
					{
						pc_delitem(ssd,idx,1,0);
					}else{
						rdamage += damage;
					}
				}
				else{
					rdamage += damage;
				}
			}
		}
		if(rdamage > 0){
			clif_damage(src,src,tick, dmg.amotion,0,rdamage,1,4,0);
			memset(&dmg,0,sizeof(dmg));
		}
	}
//武器スキル？ここまで

	switch(skillid){
	case AS_SPLASHER:
		clif_skill_damage(dsrc,bl,tick,dmg.amotion,dmg.dmotion, damage, dmg.div_, skillid, -1, 5);
		break;
	case NPC_SELFDESTRUCTION:
	case NPC_SELFDESTRUCTION2:
		dmg.blewcount |= SAB_NODAMAGE;
		break;
	default:
		clif_skill_damage(dsrc,bl,tick,dmg.amotion,dmg.dmotion, damage, dmg.div_, skillid, (lv!=0)?lv:skilllv, (skillid==0)? 5:type );
	}
	/* 吹き飛ばし処理とそのパケット */
	if (dmg.blewcount>0 && bl->type!=BL_SKILL && !map[src->m].flag.gvg)
	{
		skill_blown(dsrc,bl,dmg.blewcount);
	}

	// 吹き飛ばし処理とそのパケット カード効果 ??
	if (dsrc->type == BL_PC && bl->type!=BL_SKILL && !map[src->m].flag.gvg)
	{
		skill_add_blown(dsrc,bl,skillid,SAB_REVERSEBLOW);
	}

	map_freeblock_lock();
	/* 実際にダメージ処理を行う */
	if (skillid || flag) {
		if (attack_type&BF_WEAPON)
		{
			battle_delay_damage(tick+dmg.amotion,src,bl,damage,0);
		}else{
			battle_damage(src,bl,damage,0);
		}
	}
	if(skillid == RG_INTIMIDATE && damage > 0 && !(status_get_mode(bl)&0x20) && !map[src->m].flag.gvg ) {
		int s_lv = status_get_lv(src),t_lv = status_get_lv(bl);
		int rate = 50 + skilllv * 5;
		rate = rate + (s_lv - t_lv);
		if(atn_rand()%100 < rate)
			skill_addtimerskill(src,tick + 800,bl->id,0,0,skillid,skilllv,0,flag);
	}

	//クローンスキル
	if(damage > 0 && dmg.flag&BF_SKILL && bl->type==BL_PC
		&& pc_checkskill((struct map_session_data *)bl,RG_PLAGIARISM) && sc_data && sc_data[SC_PRESERVE].timer == -1){
		skill_clone((struct map_session_data *)bl,skillid,skilllv);
	}

	/* ダメージがあるなら追加効果判定 */
	if(bl->prev != NULL){
		struct map_session_data *sd = (struct map_session_data *)bl;
		nullpo_retr(0, sd);
		if( bl->type != BL_PC || (sd && !unit_isdead(&sd->bl)) ) {
			if(damage > 0 || skillid==TF_POISON)
				skill_additional_effect(src,bl,skillid,skilllv,attack_type,tick);
			if(bl->type==BL_MOB && src!=bl)	/* スキル使用条件のMOBスキル */
			{
				struct mob_data *md=(struct mob_data *)bl;
				nullpo_retr(0, md);
				if(battle_config.mob_changetarget_byskill == 1)
				{
					int target;
					target=md->target_id;
					if(src->type == BL_PC)
						md->target_id=src->id;
					mobskill_use(md,tick,MSC_SKILLUSED|(skillid<<16));
					md->target_id=target;
				}
				else
					mobskill_use(md,tick,MSC_SKILLUSED|(skillid<<16));
			}
		}
	}

	if(src->type == BL_PC && dmg.flag&BF_WEAPON && src != bl && src == dsrc && damage > 0) {
		struct map_session_data *sd = (struct map_session_data *)src;
		int hp = 0,sp = 0;
		nullpo_retr(0, sd);
		if(sd->hp_drain_rate && sd->hp_drain_per > 0 && dmg.damage > 0 && atn_rand()%100 < sd->hp_drain_rate) {
			hp += (dmg.damage * sd->hp_drain_per)/100;
			if(sd->hp_drain_rate > 0 && hp < 1) hp = 1;
			else if(sd->hp_drain_rate < 0 && hp > -1) hp = -1;
		}
		if(sd->hp_drain_rate_ && sd->hp_drain_per_ > 0 && dmg.damage2 > 0 && atn_rand()%100 < sd->hp_drain_rate_) {
			hp += (dmg.damage2 * sd->hp_drain_per_)/100;
			if(sd->hp_drain_rate_ > 0 && hp < 1) hp = 1;
			else if(sd->hp_drain_rate_ < 0 && hp > -1) hp = -1;
		}
		if(sd->sp_drain_rate > 0 && sd->sp_drain_per > 0 && dmg.damage > 0 && atn_rand()%100 < sd->sp_drain_rate) {
			sp += (dmg.damage * sd->sp_drain_per)/100;
			if(sd->sp_drain_rate > 0 && sp < 1) sp = 1;
			else if(sd->sp_drain_rate < 0 && sp > -1) sp = -1;
		}
		if(sd->sp_drain_rate_ > 0 && sd->sp_drain_per_ > 0 && dmg.damage2 > 0 && atn_rand()%100 < sd->sp_drain_rate_) {
			sp += (dmg.damage2 * sd->sp_drain_per_)/100;
			if(sd->sp_drain_rate_ > 0 && sp < 1) sp = 1;
			else if(sd->sp_drain_rate_ < 0 && sp > -1) sp = -1;
		}
		if(hp || sp) pc_heal(sd,hp,sp);
	}

	if (src->type == BL_PC && (skillid || flag) && rdamage>0) {
		if (attack_type&BF_WEAPON)
		{
			battle_delay_damage(tick+dmg.amotion,bl,src,rdamage,0);
			//反射ダメージのオートスペル
			if(battle_config.weapon_reflect_autospell)
			{
				skill_bonus_autospell(bl,src,AS_ATTACK,gettick(),0);
			}

			if(battle_config.weapon_reflect_drain)
				battle_attack_drain(bl,src,rdamage,0,battle_config.weapon_reflect_drain_per_enable);
		}
		else
		{
			battle_damage(bl,src,rdamage,0);
			//反射ダメージのオートスペル
			if(battle_config.magic_reflect_autospell)
			{
				skill_bonus_autospell(bl,src,AS_ATTACK,gettick(),0);
			}
			if(battle_config.magic_reflect_drain)
				battle_attack_drain(bl,src,rdamage,0,battle_config.magic_reflect_drain_per_enable);
		}
	}

	if(attack_type&BF_WEAPON && sc_data && sc_data[SC_AUTOCOUNTER].timer != -1 && sc_data[SC_AUTOCOUNTER].val4 > 0) {
		if(sc_data[SC_AUTOCOUNTER].val3 == dsrc->id)
			battle_weapon_attack(bl,dsrc,tick,0x8000|sc_data[SC_AUTOCOUNTER].val1);
		status_change_end(bl,SC_AUTOCOUNTER,-1);
	}
	/* ダブルキャスティング */
	if ((skillid == MG_COLDBOLT || skillid == MG_FIREBOLT || skillid == MG_LIGHTNINGBOLT) &&
		(sc_data = status_get_sc_data(src)) &&
		sc_data[SC_DOUBLECASTING].timer != -1 &&
		atn_rand() % 100 < 30+10*sc_data[SC_DOUBLECASTING].val1) {
		if (!(flag & 1))
			//skill_castend_delay (src, bl, skillid, skilllv, tick + dmg.div_*dmg.amotion, flag|1);
			skill_castend_delay (src, bl, skillid, skilllv, tick + 100, flag|1);
	}

	if(src->type == BL_HOM && sc_data && sc_data[SC_BLOODLUST].timer!=-1 && dmg.flag&BF_WEAPON && src != bl && src == dsrc && damage > 0)
	{
		if(atn_rand()%100 < sc_data[SC_BLOODLUST].val1*9)
		{
			homun_heal((struct homun_data *)src,damage/5,0);
		}
	}
	
	map_freeblock_unlock();

	return (dmg.damage+dmg.damage2);	/* 与ダメを返す */
}

/*==========================================
 *
 *------------------------------------------
 */
int battle_skill_attack_area(struct block_list *bl,va_list ap)
{
	struct block_list *src,*dsrc;
	int atk_type,skillid,skilllv,flag,type;
	unsigned int tick;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	atk_type = va_arg(ap,int);
	if((src=va_arg(ap,struct block_list*)) == NULL)
		return 0;
	if((dsrc=va_arg(ap,struct block_list*)) == NULL)
		return 0;
	skillid=va_arg(ap,int);
	skilllv=va_arg(ap,int);
	tick=va_arg(ap,unsigned int);
	flag=va_arg(ap,int);
	type=va_arg(ap,int);

	if(battle_check_target(dsrc,bl,type) > 0)
		battle_skill_attack(atk_type,src,dsrc,bl,skillid,skilllv,tick,flag);

	return 0;
}

int battle_check_undead(int race,int element)
{
	// element に属性値＋lv(status_get_element の戻り値)が渡されるミスに
	// 対応する為、elementから属性タイプだけを抜き出す。
	element %= 10;

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
 * 敵味方判定(1=肯定,0=否定,-1=エラー)
 * flag&0xf0000 = 0x00000:敵じゃないか判定（ret:1＝敵ではない）
 *				= 0x10000:パーティー判定（ret:1=パーティーメンバ)
 *				= 0x20000:全て(ret:1=敵味方両方)
 *				= 0x40000:敵か判定(ret:1=敵)
 *				= 0x50000:パーティーじゃないか判定(ret:1=パーティでない)
 *------------------------------------------
 */
int battle_check_target( struct block_list *src, struct block_list *target,int flag)
{
	int s_p,s_g,t_p,t_g;
	struct block_list *ss=src;

	nullpo_retr(-1, src);
	nullpo_retr(-1, target);

	if( flag&0x40000 ){	// 反転フラグ
		int ret=battle_check_target(src,target,flag&0x30000);
		if(ret!=-1)
			return !ret;
		return -1;
	}

	if( flag&0x20000 ){
		if( target->type == BL_MOB || target->type == BL_PC )
			return 1;
		if( target->type == BL_HOM && src->type != BL_SKILL )	// ホムはスキルユニットの影響を受けない
			return 1;
		else
			return -1;
	}

	if(src->type == BL_SKILL && target->type == BL_SKILL)	// 対象がスキルユニットなら無条件肯定
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

	// スキルユニットの場合、親を求める
	if( src->type==BL_SKILL) {
		struct skill_unit_group *sg = ((struct skill_unit *)src)->group;
		int inf2;
		if( sg == NULL ) return -1;
		inf2 = skill_get_inf2(sg->skill_id);
		if( (ss=map_id2bl(sg->src_id))==NULL )
			return -1;
		if(ss->prev == NULL)
			return -1;
		if( target->type != BL_PC || !pc_isinvisible((struct map_session_data *)target) ) {
			if(inf2&0x2000 && map[src->m].flag.pk)
				return 0;
			if(inf2&0x1000 && map[src->m].flag.gvg)
				return 0;
			if(inf2&0x80   && map[src->m].flag.pvp)
				return 0;
		}
		if(ss == target) {
			if(inf2&0x100)
				return 0;
			if(inf2&0x200)
				return -1;
		}
	}

	// Mobでmaster_idがあってspecial_mob_aiなら、召喚主を求める
	if( src->type==BL_MOB ){
		struct mob_data *md=(struct mob_data *)src;
		if(md && md->master_id>0){
			if(md->master_id==target->id)	// 主なら肯定
				return 1;
			if(md->state.special_mob_ai && target->type==BL_MOB) {	//special_mob_aiで対象がMob
				struct mob_data *tmd=(struct mob_data *)target;
				if(tmd){
					if(tmd->master_id != md->master_id)	//召喚主が一緒でなければ否定
						return 0;
					else if(md->state.special_mob_ai>2)	//召喚主が一緒なので肯定したいけど自爆は否定
						return 0;
					else
						return 1;
				}
			}
			if((ss=map_id2bl(md->master_id))==NULL)
				return -1;
		}
	}

	if( src==target || ss==target )	// 同じなら肯定
		return 1;

	if(target->type == BL_PC && pc_isinvisible((struct map_session_data *)target))
		return -1;

	if( src->prev==NULL || unit_isdead(src) ) // 死んでるならエラー
		return -1;

	if( (ss->type == BL_PC && target->type==BL_MOB) ||
		(ss->type == BL_MOB && target->type==BL_PC) )
		return 0;	// PCvsMOBなら敵

	if(ss->type == BL_PET && target->type==BL_MOB) {
		struct pet_data *pd = (struct pet_data*)ss;
		struct mob_data *md = (struct mob_data*)target;
		int mode=mob_db[pd->class].mode;
		int race=mob_db[pd->class].race;
		if(mob_db[pd->class].mexp <= 0 && !(mode&0x20) && (md->option & 0x06 && race != 4 && race != 6) ) {
			return 1; // 失敗
		} else {
			return 0; // 成功
		}
	}

	s_p=status_get_party_id(ss);
	s_g=status_get_guild_id(ss);

	t_p=status_get_party_id(target);
	t_g=status_get_guild_id(target);

	if(flag&0x10000) {
		if(s_p && t_p && s_p == t_p)	// 同じパーティなら肯定（味方）
			return 1;
		else		// パーティ検索なら同じパーティじゃない時点で否定
			return 0;
	}

	if(ss->type == BL_MOB && s_g > 0 && t_g > 0 && s_g == t_g )	// 同じギルド/mobクラスなら肯定（味方）
		return 1;

	if( ss->type==BL_PC && target->type==BL_PC) { // 両方PVPモードなら否定（敵）
		struct skill_unit *su=NULL;
		if(src->type==BL_SKILL)
			su=(struct skill_unit *)src;
		//PK
		if(map[ss->m].flag.pk) {
			struct guild *g=NULL;
			struct map_session_data* ssd = (struct map_session_data*)ss;
			struct map_session_data* tsd = (struct map_session_data*)target;
			struct pc_base_job s_class,t_class;
			s_class = pc_calc_base_job(ssd->status.class);
			t_class = pc_calc_base_job(tsd->status.class);
			//battle_config.no_pk_level以下　1次は味方　転生は駄目
			if((ssd->sc_data && ssd->sc_data[SC_PK_PENALTY].timer!=-1) ||
				(ssd->status.base_level <= battle_config.no_pk_level && (s_class.job <=6 || s_class.job==24) && s_class.upper!=1))
				return 1;
			if(tsd->status.base_level <= battle_config.no_pk_level && (t_class.job <=6 || t_class.job==24) && t_class.upper!=1)
				return 1;
			if(su && su->group->target_flag==BCT_NOENEMY)
				return 1;
			if(s_p > 0 && t_p > 0 && s_p == t_p)
				return 1;
			else if(s_g > 0 && t_g > 0 && s_g == t_g)
				return 1;
			if((g = guild_search(s_g)) !=NULL) {
				int i;
				for(i=0;i<MAX_GUILDALLIANCE;i++){
					if(g->alliance[i].guild_id > 0 && g->alliance[i].guild_id == t_g) {
						if(g->alliance[i].opposition)
							return 0;//敵対ギルドなら無条件に敵
						else
							return 1;//同盟ギルドなら無条件に味方
					}
				}
			}
			return 0;
		}
		//PVP
		if(map[ss->m].flag.pvp) {//PVP
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
							return 0;//敵対ギルドなら無条件に敵
						else
							return 1;//同盟ギルドなら無条件に味方
					}
				}
			}
			return 0;
		}
	}

	if( (ss->type == BL_HOM && target->type==BL_MOB) ||
		(ss->type == BL_MOB && target->type==BL_HOM) )
		return 0;	// HOMvsMOBなら敵

	if(!(map[ss->m].flag.pvp || map[ss->m].flag.gvg) &&
		((ss->type == BL_PC && target->type==BL_HOM) ||
		( ss->type == BL_HOM && target->type==BL_PC)))
		return 1;	// PvでもGvでもないなら、PCvsHOMは味方

	// 同PTとか同盟Guildとかは後回し（＝＝
	if(ss->type == BL_HOM){
		struct homun_data *hd = (struct homun_data *)ss;
		if(map[ss->m].flag.pvp) {//PVP
			if(target->type==BL_HOM)
				return 0;
			if(target->type==BL_PC){
				struct map_session_data *tsd=(struct map_session_data*)target;
				if(tsd != hd->msd)
					return 0;
			}
		}
		if(map[ss->m].flag.gvg) {//GVG
			if(target->type==BL_HOM)
				return 0;
		}
	}
	if(ss->type == BL_PC && target->type == BL_HOM){
		struct homun_data *hd = (struct homun_data *)target;
		if(map[ss->m].flag.pvp) {//PVP
			if(ss != &hd->msd->bl){
				return 0;
			}
		}
		if(map[ss->m].flag.gvg) {//GVG
			return 0;
		}
	}
	return 1;	// 該当しないので無関係人物（まあ敵じゃないので味方）
}
/*==========================================
 * 射程判定
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

	if(src->m != bl->m)	// 違うマップ
		return 0;

	if( range>0 && range < arange )	// 遠すぎる
		return 0;

	if( arange<2 )	// 同じマスか隣接
		return 1;

//	if(bl->type == BL_SKILL && ((struct skill_unit *)bl)->group->unit_id == 0x8d)
//		return 1;

	// 障害物判定
	return path_search_long(NULL,src->m,src->x,src->y,bl->x,bl->y);
}

int battle_delarrow(struct map_session_data* sd,int num)
{
	nullpo_retr(0,sd);
	switch(sd->status.weapon){
	case 11:
		if(sd->equip_index[10] >= 0 &&
			sd->status.inventory[sd->equip_index[10]].amount>=num &&
			sd->inventory_data[sd->equip_index[10]]->arrow_type&1)
		{
			if(battle_config.arrow_decrement)
				pc_delitem(sd,sd->equip_index[10],num,0);
		} else {
			clif_arrow_fail(sd,0);
			return 0;
		}
		break;
	case 17:
		if(sd->equip_index[10] >= 0 &&
			sd->status.inventory[sd->equip_index[10]].amount>=num &&
			sd->inventory_data[sd->equip_index[10]]->arrow_type&4 )
		{
			if(battle_config.arrow_decrement)
				pc_delitem(sd,sd->equip_index[10],num,0);
		}else{
			clif_arrow_fail(sd,0);
			return 0;
		}
		break;
	case 18:
		if(sd->equip_index[10] >= 0 &&
			sd->status.inventory[sd->equip_index[10]].amount>=num &&
			sd->inventory_data[sd->equip_index[10]]->arrow_type&8 )
		{
			if(battle_config.arrow_decrement)
				pc_delitem(sd,sd->equip_index[10],num,0);
		}else{
			clif_arrow_fail(sd,0);
			return 0;
		}
		break;
	case 19:
		if(sd->equip_index[10] >= 0 &&
			sd->status.inventory[sd->equip_index[10]].amount>=num &&
			sd->inventory_data[sd->equip_index[10]]->arrow_type&16 )
		{
			if(battle_config.arrow_decrement)
				pc_delitem(sd,sd->equip_index[10],num,0);
		}else{
			clif_arrow_fail(sd,0);
			return 0;
		}
		break;
	case 20:
		if(sd->equip_index[10] >= 0 &&
			sd->status.inventory[sd->equip_index[10]].amount>=num &&
			sd->inventory_data[sd->equip_index[10]]->arrow_type&32 )
		{
			if(battle_config.arrow_decrement)
				pc_delitem(sd,sd->equip_index[10],num,0);
		}else{
			clif_arrow_fail(sd,0);
			return 0;
		}
		break;
	case 21:
		if(sd->equip_index[10] >= 0 &&
			sd->status.inventory[sd->equip_index[10]].amount>=num &&
			sd->inventory_data[sd->equip_index[10]]->arrow_type&64 )
		{
			if(battle_config.arrow_decrement)
				pc_delitem(sd,sd->equip_index[10],num,0);
		}else{
			clif_arrow_fail(sd,0);
			return 0;
		}
		break;
	}
	return 1;
}
/*==========================================
 * 設定ファイル読み込み用（フラグ）
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
 * 設定ファイルを読み込む
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
		battle_config.no_cast_dex=150;
		battle_config.delay_rate=100;
		battle_config.delay_dependon_dex=0;
		battle_config.no_delay_dex=150;
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
		battle_config.natural_heal_weight_rate_icon=0;
		battle_config.item_name_override_grffile=1;
		battle_config.arrow_decrement=1;
		battle_config.allow_any_weapon_autoblitz=0;
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
		battle_config.min_hitrate = 5;
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
		battle_config.gvg_edp_down_rate = 100;
		battle_config.pvp_edp_down_rate = 100;
		battle_config.pk_edp_down_rate = 100;
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
		battle_config.sole_concert_type = 3;
		battle_config.hide_GM_session = 0;
		battle_config.unit_movement_type = 0;
		battle_config.invite_request_check = 1;
		battle_config.gvg_trade_request_refused = 1;
		battle_config.pvp_trade_request_refused = 1;
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
		battle_config.consume_drop_rate = 100;
		battle_config.refine_drop_rate = 100;
		battle_config.etc_drop_rate = 100;

		battle_config.potion_drop_rate = 100;
		battle_config.arrow_drop_rate = 100;
		battle_config.petequip_drop_rate = 100;
		battle_config.weapon_drop_rate = 100;
		battle_config.other_drop_rate = 100;

		battle_config.Item_res = 1;
		battle_config.next_exp_limit = 150;
		battle_config.heal_counterstop = 11;
		battle_config.finding_ore_drop_rate = 100;
		battle_config.joint_struggle_exp_bonus = 25;
		battle_config.joint_struggle_limit = 600;
		battle_config.pt_bonus_b = 0;
		battle_config.pt_bonus_j = 0;
		battle_config.equip_autospell_nocost = 0;
		battle_config.limit_gemstone = 0;
		battle_config.mvp_announce = 0;
		battle_config.petowneditem = 0;
		battle_config.buyer_name = 0;
		battle_config.once_autospell = 1;
		battle_config.allow_same_autospell = 0;
		battle_config.combo_delay_lower_limits = 0;
		battle_config.new_marrige_skill = 0;
		battle_config.reveff_plus_addeff = 0;
		battle_config.summonslave_no_drop = 0;
		battle_config.summonslave_no_exp = 0;
		battle_config.summonslave_no_mvp = 0;
		battle_config.summonmonster_no_drop = 0;
		battle_config.summonmonster_no_exp = 0;
		battle_config.summonmonster_no_mvp = 0;
		battle_config.cannibalize_no_drop = 0;
		battle_config.cannibalize_no_exp = 0;
		battle_config.cannibalize_no_mvp = 0;
		battle_config.spheremine_no_drop = 0;
		battle_config.spheremine_no_exp = 0;
		battle_config.spheremine_no_mvp = 0;
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
		battle_config.save_hate_mob = 0;
		battle_config.twilight_party_check = 0;
		battle_config.alchemist_point_type = 1;
		battle_config.marionette_type	   = 0;
		battle_config.max_marionette_str	= 99;
		battle_config.max_marionette_agi	= 99;
		battle_config.max_marionette_vit	= 99;
		battle_config.max_marionette_int	= 99;
		battle_config.max_marionette_dex	= 99;
		battle_config.max_marionette_luk	= 99;
		battle_config.baby_status_max  = 80;
		battle_config.baby_hp_rate	   = 70;
		battle_config.baby_sp_rate	   = 70;
		battle_config.upper_hp_rate    =125;
		battle_config.upper_sp_rate    =125;
		battle_config.normal_hp_rate   =100;
		battle_config.normal_sp_rate   =100;
		battle_config.baby_weight_rate = 100;
		battle_config.no_emergency_call = 1;
		battle_config.save_am_pharmacy_success = 0;
		battle_config.save_all_ranking_point_when_logout = 0;
		battle_config.soul_linker_battle_mode 		= 0;
		battle_config.soul_linker_battle_mode_ka 	= 0;
		battle_config.skillup_type 					= 1;
		battle_config.allow_me_dance_effect			= 0;
		battle_config.allow_me_concert_effect		= 0;
		battle_config.allow_me_rokisweil			= 0;
		battle_config.pharmacy_get_point_type		= 0;
		battle_config.cheat_log = 1;
		battle_config.soulskill_can_be_used_for_myself = 1;
		battle_config.hermode_wp_check_range = 3;
		battle_config.hermode_wp_check = 1;
		battle_config.hermode_no_walking = 0;
		battle_config.hermode_gvg_only = 1;
		battle_config.atcommand_go_significant_values	= 21;
		battle_config.redemptio_penalty_type	=	1;
		battle_config.allow_weaponrearch_to_weaponrefine = 0;
		battle_config.boss_no_knockbacking = 0;
		battle_config.boss_no_element_change = 1;
		battle_config.scroll_produce_rate = 100;
		battle_config.scroll_item_name_input = 0;
		battle_config.pet_leave = 0;
		battle_config.pk_short_damage_rate = 100;
		battle_config.pk_long_damage_rate = 100;
		battle_config.pk_magic_damage_rate = 100;
		battle_config.pk_misc_damage_rate = 100;
		battle_config.cooking_rate = 100;
		battle_config.making_rate = 100;
		battle_config.extended_abracadabra = 0;
		battle_config.changeoption_packet_type = 0;
		battle_config.redemptio_user_noexp = 0;
		battle_config.no_pk_level		   = 60;
		battle_config.allow_cloneskill_at_autospell = 0;
		battle_config.pk_noshift = 0;
		battle_config.pk_penalty_time = 60000;
		battle_config.dropitem_itemrate_fix = 0;
		battle_config.gm_nomanner_lv = 50;
		battle_config.clif_fixpos_type = 1;
		battle_config.romail = 0;
		battle_config.pc_die_script = 0;
		battle_config.pc_kill_script = 0;
		battle_config.pc_movemap_script = 0;
		battle_config.pc_login_script = 0;
		battle_config.pc_logout_script = 0;
		battle_config.set_pckillerid = 0;
		battle_config.def_ratio_atk_to_shieldchain = 0;
		battle_config.def_ratio_atk_to_carttermination = 0;
		battle_config.player_gravitation_type = 0;
		battle_config.enemy_gravitation_type = 0;
		battle_config.mob_attack_fixwalkpos = 0;
		battle_config.mob_ai_limiter = 0;
		battle_config.mob_ai_cpu_usage = 80;
		battle_config.itemidentify = 0;
		battle_config.casting_penalty_type = 0;
		battle_config.casting_penalty_weapon = 0;
		battle_config.casting_penalty_shield = 0;
		battle_config.casting_penalty_armor = 0;
		battle_config.casting_penalty_helm = 0;
		battle_config.casting_penalty_robe = 0;
		battle_config.casting_penalty_shoes = 0;
		battle_config.casting_penalty_acce = 0;
		battle_config.casting_penalty_arrow = 0;
		battle_config.show_always_party_name = 0;
		battle_config.check_player_name_global_msg = 0;
		battle_config.check_player_name_party_msg = 0;
		battle_config.check_player_name_guild_msg = 0;
		battle_config.save_player_when_drop_item = 0;
		battle_config.check_sitting_player_using_skill = 0;
		battle_config.check_sitting_player_using_skill_p = 0;
		battle_config.allow_homun_status_change = 1;
		battle_config.save_homun_temporal_intimate = 1;
		battle_config.homun_intimate_rate = 100;
		battle_config.homun_temporal_intimate_resilience = 50;
		battle_config.hvan_explosion_intimate   = 45000;
		battle_config.homun_speed_is_same_as_pc = 1;
		battle_config.homun_skill_intimate_type = 0;
		battle_config.master_get_homun_base_exp = 0;
		battle_config.master_get_homun_job_exp = 0;
	}

	fp=fopen(cfgName,"r");
	if(fp==NULL){
		printf("file not found: %s\n",cfgName);
		return 1;
	}
	while(fgets(line,1020,fp)){
		static const struct {
			char *str;
			int  *val;
		} data[] ={
			{ "warp_point_debug",					&battle_config.warp_point_debug						},
			{ "enemy_critical",						&battle_config.enemy_critical						},
			{ "enemy_critical_rate",				&battle_config.enemy_critical_rate					},
			{ "enemy_str",							&battle_config.enemy_str							},
			{ "enemy_perfect_flee",					&battle_config.enemy_perfect_flee					},
			{ "casting_rate",						&battle_config.cast_rate							},
			{ "no_casting_dex",						&battle_config.no_cast_dex							},
			{ "delay_rate",							&battle_config.delay_rate							},
			{ "delay_dependon_dex",					&battle_config.delay_dependon_dex					},
			{ "no_delay_dex",						&battle_config.no_delay_dex							},
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
			{ "natural_heal_weight_rate_icon",		&battle_config.natural_heal_weight_rate_icon		},
			{ "item_name_override_grffile",			&battle_config.item_name_override_grffile			},
			{ "arrow_decrement",					&battle_config.arrow_decrement						},
			{ "allow_any_weapon_autoblitz",			&battle_config.allow_any_weapon_autoblitz			},
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
			{ "min_hitrate",						&battle_config.min_hitrate							},
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
			{ "gvg_edp_down_rate",					&battle_config.gvg_edp_down_rate					},
			{ "pvp_edp_down_rate",					&battle_config.pvp_edp_down_rate					},
			{ "pk_edp_down_rate",					&battle_config.pk_edp_down_rate						},
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
			{ "sole_concert_type",					&battle_config.sole_concert_type					},
			{ "hide_GM_session",					&battle_config.hide_GM_session						},
			{ "unit_movement_type",					&battle_config.unit_movement_type					},
			{ "invite_request_check",				&battle_config.invite_request_check					},
			{ "gvg_trade_request_refused",			&battle_config.gvg_trade_request_refused			},
			{ "pvp_trade_request_refused",			&battle_config.pvp_trade_request_refused			},
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
			{ "consume_drop_rate",					&battle_config.consume_drop_rate					},
			{ "refine_drop_rate",					&battle_config.refine_drop_rate						},
			{ "etc_drop_rate",						&battle_config.etc_drop_rate						},

			{ "potion_drop_rate",					&battle_config.potion_drop_rate						},
			{ "arrow_drop_rate",					&battle_config.arrow_drop_rate						},
			{ "petequip_drop_rate",					&battle_config.petequip_drop_rate						},
			{ "weapon_drop_rate",					&battle_config.weapon_drop_rate						},
			{ "other_drop_rate",					&battle_config.other_drop_rate						},

			{ "Item_res",							&battle_config.Item_res								},
			{ "next_exp_limit",						&battle_config.next_exp_limit						},
			{ "heal_counterstop",					&battle_config.heal_counterstop						},
			{ "finding_ore_drop_rate",				&battle_config.finding_ore_drop_rate				},
			{ "joint_struggle_exp_bonus",			&battle_config.joint_struggle_exp_bonus				},
			{ "joint_struggle_limit",				&battle_config.joint_struggle_limit					},
			{ "pt_bonus_b",							&battle_config.pt_bonus_b							},
			{ "pt_bonus_j",							&battle_config.pt_bonus_j							},
			{ "equip_autospell_nocost",				&battle_config.equip_autospell_nocost				},
			{ "limit_gemstone",						&battle_config.limit_gemstone						},
			{ "mvp_announce",						&battle_config.mvp_announce							},
			{ "petowneditem",						&battle_config.petowneditem							},
			{ "buyer_name",							&battle_config.buyer_name							},
			{ "noportal_flag",						&battle_config.noportal_flag						},
			{ "once_autospell",						&battle_config.once_autospell						},
			{ "allow_same_autospell",				&battle_config.allow_same_autospell					},
			{ "combo_delay_lower_limits",			&battle_config.combo_delay_lower_limits				},
			{ "new_marrige_skill",					&battle_config.new_marrige_skill					},
			{ "reveff_plus_addeff",					&battle_config.reveff_plus_addeff					},
			{ "summonslave_no_drop",				&battle_config.summonslave_no_drop					},
			{ "summonslave_no_exp",					&battle_config.summonslave_no_exp					},
			{ "summonslave_no_mvp",					&battle_config.summonslave_no_mvp					},
			{ "summonmonster_no_drop",				&battle_config.summonmonster_no_drop				},
			{ "summonmonster_no_exp",				&battle_config.summonmonster_no_exp					},
			{ "summonmonster_no_mvp",				&battle_config.summonmonster_no_mvp					},
			{ "cannibalize_no_drop",				&battle_config.cannibalize_no_drop					},
			{ "cannibalize_no_exp",					&battle_config.cannibalize_no_exp					},
			{ "cannibalize_no_mvp",					&battle_config.cannibalize_no_mvp					},
			{ "spheremine_no_drop",				&battle_config.spheremine_no_drop					},
			{ "spheremine_no_exp",					&battle_config.spheremine_no_exp					},
			{ "spheremine_no_mvp",					&battle_config.spheremine_no_mvp					},
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
			{ "save_hate_mob",						&battle_config.save_hate_mob						},
			{ "twilight_party_check",				&battle_config.twilight_party_check					},
			{ "alchemist_point_type",				&battle_config.alchemist_point_type					},
			{ "marionette_type",					&battle_config.marionette_type						},
			{ "max_marionette_str",					&battle_config.max_marionette_str					},
			{ "max_marionette_agi",					&battle_config.max_marionette_agi					},
			{ "max_marionette_vit",					&battle_config.max_marionette_vit					},
			{ "max_marionette_int",					&battle_config.max_marionette_int					},
			{ "max_marionette_dex",					&battle_config.max_marionette_dex					},
			{ "max_marionette_luk",					&battle_config.max_marionette_luk					},
			{ "baby_status_max",					&battle_config.baby_status_max						},
			{ "baby_hp_rate",						&battle_config.baby_hp_rate							},
			{ "baby_sp_rate",						&battle_config.baby_sp_rate							},
			{ "upper_hp_rate",						&battle_config.upper_hp_rate						},
			{ "upper_sp_rate",						&battle_config.upper_sp_rate						},
			{ "normal_hp_rate",						&battle_config.normal_hp_rate						},
			{ "normal_sp_rate",						&battle_config.normal_sp_rate						},
			{ "baby_weight_rate",					&battle_config.baby_weight_rate						},
			{ "no_emergency_call",					&battle_config.no_emergency_call					},
			{ "save_am_pharmacy_success",			&battle_config.save_am_pharmacy_success				},
			{ "save_all_ranking_point_when_logout",	&battle_config.save_all_ranking_point_when_logout	},
			{ "soul_linker_battle_mode",			&battle_config.soul_linker_battle_mode				},
			{ "soul_linker_battle_mode_ka",			&battle_config.soul_linker_battle_mode_ka			},
			{ "skillup_type",						&battle_config.skillup_type							},
			{ "allow_me_dance_effect",				&battle_config.allow_me_dance_effect				},
			{ "allow_me_concert_effect",			&battle_config.allow_me_concert_effect				},
			{ "allow_me_rokisweil",					&battle_config.allow_me_rokisweil					},
			{ "pharmacy_get_point_type",			&battle_config.pharmacy_get_point_type				},
			{ "cheat_log",							&battle_config.cheat_log							},
			{ "soulskill_can_be_used_for_myself",	&battle_config.soulskill_can_be_used_for_myself		},
			{ "hermode_wp_check_range",				&battle_config.hermode_wp_check_range				},
			{ "hermode_wp_check",					&battle_config.hermode_wp_check						},
			{ "hermode_no_walking",					&battle_config.hermode_no_walking					},
			{ "hermode_gvg_only",					&battle_config.hermode_gvg_only						},
			{ "atcommand_go_significant_values",	&battle_config.atcommand_go_significant_values		},
			{ "redemptio_penalty_type",				&battle_config.redemptio_penalty_type				},
			{ "allow_weaponrearch_to_weaponrefine",	&battle_config.allow_weaponrearch_to_weaponrefine	},
			{ "boss_no_knockbacking",				&battle_config.boss_no_knockbacking					},
			{ "boss_no_element_change",				&battle_config.boss_no_element_change				},
			{ "scroll_produce_rate",				&battle_config.scroll_produce_rate					},
			{ "scroll_item_name_input",				&battle_config.scroll_item_name_input				},
			{ "pet_leave",							&battle_config.pet_leave							},
			{ "pk_short_attack_damage_rate",		&battle_config.pk_short_damage_rate					},
			{ "pk_long_attack_damage_rate",			&battle_config.pk_long_damage_rate					},
			{ "pk_magic_attack_damage_rate",		&battle_config.pk_magic_damage_rate					},
			{ "pk_misc_attack_damage_rate",			&battle_config.pk_misc_damage_rate					},
			{ "cooking_rate",						&battle_config.cooking_rate							},
			{ "making_rate",						&battle_config.making_rate							},
			{ "extended_abracadabra",				&battle_config.extended_abracadabra					},
			{ "changeoption_packet_type",			&battle_config.changeoption_packet_type				},
			{ "redemptio_user_noexp",				&battle_config.redemptio_user_noexp					},
			{ "no_pk_level",						&battle_config.no_pk_level							},
			{ "allow_cloneskill_at_autospell",		&battle_config.allow_cloneskill_at_autospell		},
			{ "pk_noshift",							&battle_config.pk_noshift							},
			{ "pk_penalty_time",					&battle_config.pk_penalty_time						},
			{ "dropitem_itemrate_fix",				&battle_config.dropitem_itemrate_fix				},
			{ "gm_nomanner_lv",						&battle_config.gm_nomanner_lv						},
			{ "clif_fixpos_type",					&battle_config.clif_fixpos_type						},
			{ "romailuse",							&battle_config.romail								},
			{ "pc_die_script",						&battle_config.pc_die_script						},
			{ "pc_kill_script",						&battle_config.pc_kill_script						},
			{ "pc_movemap_script",					&battle_config.pc_movemap_script					},
			{ "pc_login_script",					&battle_config.pc_login_script						},
			{ "pc_logout_script",					&battle_config.pc_logout_script						},
			{ "set_pckillerid",						&battle_config.set_pckillerid						},
			{ "def_ratio_atk_to_shieldchain",		&battle_config.def_ratio_atk_to_shieldchain			},
			{ "def_ratio_atk_to_carttermination",	&battle_config.def_ratio_atk_to_carttermination		},
			{ "player_gravitation_type",			&battle_config.player_gravitation_type				},
			{ "enemy_gravitation_type",				&battle_config.enemy_gravitation_type				},
			{ "mob_attack_fixwalkpos",				&battle_config.mob_attack_fixwalkpos				},
			{ "mob_ai_limiter",						&battle_config.mob_ai_limiter						},
			{ "mob_ai_cpu_usage",					&battle_config.mob_ai_cpu_usage						},
			{ "itemidentify",						&battle_config.itemidentify							},
			{ "casting_penalty_type",				&battle_config.casting_penalty_type					},
			{ "casting_penalty_weapon",				&battle_config.casting_penalty_weapon				},
			{ "casting_penalty_shield",				&battle_config.casting_penalty_shield				},
			{ "casting_penalty_armor",				&battle_config.casting_penalty_armor				},
			{ "casting_penalty_helm",				&battle_config.casting_penalty_helm					},
			{ "casting_penalty_robe",				&battle_config.casting_penalty_robe					},
			{ "casting_penalty_shoes",				&battle_config.casting_penalty_shoes				},
			{ "casting_penalty_acce",				&battle_config.casting_penalty_acce					},
			{ "casting_penalty_arrow",				&battle_config.casting_penalty_arrow				},
			{ "show_always_party_name",				&battle_config.show_always_party_name				},
			{ "check_player_name_global_msg",		&battle_config.check_player_name_global_msg			},
			{ "check_player_name_party_msg",		&battle_config.check_player_name_party_msg			},
			{ "check_player_name_guild_msg",		&battle_config.check_player_name_guild_msg			},
			{ "save_player_when_drop_item",			&battle_config.save_player_when_drop_item			},
			{ "check_sitting_player_using_skill",	&battle_config.check_sitting_player_using_skill		},
			{ "check_sitting_player_using_skill_p",	&battle_config.check_sitting_player_using_skill_p	},
			{ "allow_homun_status_change",			&battle_config.allow_homun_status_change			},
			{ "save_homun_temporal_intimate",		&battle_config.save_homun_temporal_intimate			},
			{ "homun_intimate_rate",				&battle_config.homun_intimate_rate					},
			{ "homun_temporal_intimate_resilience",	&battle_config.homun_temporal_intimate_resilience	},
			{ "hvan_explosion_intimate",			&battle_config.hvan_explosion_intimate				},
			{ "homun_speed_is_same_as_pc",			&battle_config.homun_speed_is_same_as_pc			},
			{ "homun_skill_intimate_type",			&battle_config.homun_skill_intimate_type			},
			{ "master_get_homun_base_exp",			&battle_config.master_get_homun_base_exp			},
			{ "master_get_homun_job_exp",			&battle_config.master_get_homun_job_exp				},
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

	//フラグ調整
	if(battle_config.allow_guild_skill_in_gvgtime_only)
		battle_config.guild_skill_available = 0;

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
		else if(battle_config.natural_heal_weight_rate > 101)
			battle_config.natural_heal_weight_rate = 101;
		battle_config.monster_max_aspd = 2000 - battle_config.monster_max_aspd*10;
		if(battle_config.monster_max_aspd < 10)
			battle_config.monster_max_aspd = 10;
		else if(battle_config.monster_max_aspd > 1000)
			battle_config.monster_max_aspd = 1000;
		battle_config.max_aspd = 2000 - battle_config.max_aspd*10;
		if(battle_config.max_aspd < 10)
			battle_config.max_aspd = 10;
		else if(battle_config.max_aspd > 1000)
			battle_config.max_aspd = 1000;
		if(battle_config.hp_rate < 0)
			battle_config.hp_rate = 1;
		if(battle_config.sp_rate < 0)
			battle_config.sp_rate = 1;
		if(battle_config.max_hp > 1000000)
			battle_config.max_hp = 1000000;
		else if(battle_config.max_hp < 100)
			battle_config.max_hp = 100;
		if(battle_config.max_sp > 1000000)
			battle_config.max_sp = 1000000;
		else if(battle_config.max_sp < 100)
			battle_config.max_sp = 100;

		if(battle_config.max_parameter < 10)
			battle_config.max_parameter = 10;
		else if(battle_config.max_parameter > 10000)
			battle_config.max_parameter = 10000;
		if(battle_config.max_parameter_str < 1)
			battle_config.max_parameter_str = 1;
		else if(battle_config.max_parameter_str > battle_config.max_parameter)
			battle_config.max_parameter_str = battle_config.max_parameter;
		if(battle_config.max_parameter_agi < 1)
			battle_config.max_parameter_agi = 1;
		else if(battle_config.max_parameter_agi > battle_config.max_parameter)
			battle_config.max_parameter_agi = battle_config.max_parameter;
		if(battle_config.max_parameter_vit < 1)
			battle_config.max_parameter_vit = 1;
		else if(battle_config.max_parameter_vit > battle_config.max_parameter)
			battle_config.max_parameter_vit = battle_config.max_parameter;
		if(battle_config.max_parameter_int < 1)
			battle_config.max_parameter_int = 1;
		else if(battle_config.max_parameter_int > battle_config.max_parameter)
			battle_config.max_parameter_int = battle_config.max_parameter;
		if(battle_config.max_parameter_dex < 1)
			battle_config.max_parameter_dex = 1;
		else if(battle_config.max_parameter_dex > battle_config.max_parameter)
			battle_config.max_parameter_dex = battle_config.max_parameter;
		if(battle_config.max_parameter_luk < 1)
			battle_config.max_parameter_luk = 1;
		else if(battle_config.max_parameter_luk > battle_config.max_parameter)
			battle_config.max_parameter_luk = battle_config.max_parameter;

		if(battle_config.max_cart_weight > 1000000)
			battle_config.max_cart_weight = 1000000;
		else if(battle_config.max_cart_weight < 100)
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
		if(battle_config.consume_drop_rate < 0)
			battle_config.consume_drop_rate = 0;
		if(battle_config.refine_drop_rate < 0)
			battle_config.refine_drop_rate = 0;
		if(battle_config.etc_drop_rate < 0)
			battle_config.etc_drop_rate = 0;

		if(battle_config.potion_drop_rate < 0)
			battle_config.potion_drop_rate = 0;
		if(battle_config.arrow_drop_rate < 0)
			battle_config.arrow_drop_rate = 0;
		if(battle_config.petequip_drop_rate < 0)
			battle_config.petequip_drop_rate = 0;
		if(battle_config.weapon_drop_rate < 0)
			battle_config.weapon_drop_rate = 0;
		if(battle_config.other_drop_rate < 0)
			battle_config.other_drop_rate = 0;


		if(battle_config.heal_counterstop < 0)
			battle_config.heal_counterstop = 0;
		if (battle_config.finding_ore_drop_rate < 0)
			battle_config.finding_ore_drop_rate = 0;
		else if (battle_config.finding_ore_drop_rate > 10000)
			battle_config.finding_ore_drop_rate = 10000;

		if(battle_config.max_marionette_str < 1)
			battle_config.max_marionette_str = battle_config.max_parameter;
		if(battle_config.max_marionette_agi < 1)
			battle_config.max_marionette_agi = battle_config.max_parameter;
		if(battle_config.max_marionette_vit < 1)
			battle_config.max_marionette_vit = battle_config.max_parameter;
		if(battle_config.max_marionette_int < 1)
			battle_config.max_marionette_int = battle_config.max_parameter;
		if(battle_config.max_marionette_dex < 1)
			battle_config.max_marionette_dex = battle_config.max_parameter;
		if(battle_config.max_marionette_luk < 1)
			battle_config.max_marionette_luk = battle_config.max_parameter;

		add_timer_func_list(battle_delay_damage_sub, "battle_delay_damage_sub");
	}

	return 0;
}

