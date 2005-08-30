
// �X�e�[�^�X�v�Z�A��Ԉُ폈��
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include "pc.h"
#include "map.h"
#include "pet.h"
#include "mob.h"
#include "clif.h"
#include "timer.h"
#include "skill.h"
#include "itemdb.h"
#include "battle.h"
#include "status.h"
#include "nullpo.h"
#include "script.h"
#include "guild.h"

static int max_weight_base[MAX_PC_CLASS];
static int hp_coefficient[MAX_PC_CLASS];
static int hp_coefficient2[MAX_PC_CLASS];
static int hp_sigma_val[MAX_PC_CLASS][MAX_LEVEL];
static int sp_coefficient[MAX_PC_CLASS];
static int aspd_base[MAX_PC_CLASS][20];
static int refinebonus[5][3];	// ���B�{�[�i�X�e�[�u��(refine_db.txt)
static int percentrefinery[5][10];	// ���B������(refine_db.txt)
static int atkmods[3][20];	// ����ATK�T�C�Y�C��(size_fix.txt)
static char job_bonus[3][MAX_PC_CLASS][MAX_LEVEL];
int current_equip_item_index;//�X�e�[�^�X�v�Z�p
int current_equip_card_id;
/*==========================================
 * ���B�{�[�i�X
 *------------------------------------------
 */
int status_getrefinebonus(int lv,int type)
{
	if(lv >= 0 && lv < 5 && type >= 0 && type < 3)
		return refinebonus[lv][type];
	return 0;
}

/*==========================================
 * ���B������
 *------------------------------------------
 */
int status_percentrefinery(struct map_session_data *sd,struct item *item)
{
	int percent;

	nullpo_retr(0, item);
	percent=percentrefinery[itemdb_wlv(item->nameid)][(int)item->refine];

	percent += pc_checkskill(sd,BS_WEAPONRESEARCH);	// ���팤���X�L������

	// �m���̗L���͈̓`�F�b�N
	if( percent > 100 ){
		percent = 100;
	}
	if( percent < 0 ){
		percent = 0;
	}

	return percent;
}

/*==========================================
 * �p�����[�^�v�Z
 * first==0�̎��A�v�Z�Ώۂ̃p�����[�^���Ăяo���O����
 * �� �������ꍇ������send���邪�A
 * �\���I�ɕω��������p�����[�^�͎��O��send����悤��
 *------------------------------------------
 */

int status_calc_pc(struct map_session_data* sd,int first)
{
	int b_speed,b_max_hp,b_max_sp,b_hp,b_sp,b_weight,b_max_weight,b_paramb[6],b_parame[6],b_hit,b_flee;
	int b_aspd,b_watk,b_def,b_watk2,b_def2,b_flee2,b_critical,b_attackrange,b_matk1,b_matk2,b_mdef,b_mdef2,b_class;
	int b_base_atk;
	struct skill b_skill[MAX_SKILL];
	int i,bl,index;
	int skill,aspd_rate,wele,wele_,def_ele,refinedef=0;
	int pele=0,pdef_ele=0;
	int str,dstr,dex;
	struct guild *g = NULL;
	struct map_session_data* gmsd = NULL;
	struct pc_base_job s_class;

	nullpo_retr(0, sd);

	//�]����{�q�̏ꍇ�̌��̐E�Ƃ��Z�o����
	s_class = pc_calc_base_job(sd->status.class);
	//�M���h�擾
	g = (struct guild *)guild_search(sd->status.guild_id);
	if(g)
		gmsd = (struct map_session_data*)guild_get_guildmaster_sd(g);
		
	b_speed = sd->speed;
	b_max_hp = sd->status.max_hp;
	b_max_sp = sd->status.max_sp;
	b_hp = sd->status.hp;
	b_sp = sd->status.sp;
	b_weight = sd->weight;
	b_max_weight = sd->max_weight;
	memcpy(b_paramb,&sd->paramb,sizeof(b_paramb));
	memcpy(b_parame,&sd->paramc,sizeof(b_parame));
	memcpy(b_skill,&sd->status.skill,sizeof(b_skill));
	b_hit = sd->hit;
	b_flee = sd->flee;
	b_aspd = sd->aspd;
	b_watk = sd->watk;
	b_def = sd->def;
	b_watk2 = sd->watk2;
	b_def2 = sd->def2;
	b_flee2 = sd->flee2;
	b_critical = sd->critical;
	b_attackrange = sd->attackrange;
	b_matk1 = sd->matk1;
	b_matk2 = sd->matk2;
	b_mdef = sd->mdef;
	b_mdef2 = sd->mdef2;
	b_class = sd->view_class;
	sd->view_class = sd->status.class;
	b_base_atk = sd->base_atk;

	pc_calc_skilltree(sd);	// �X�L���c���[�̌v�Z

	sd->max_weight = max_weight_base[s_class.job]+sd->status.str*300;
	if(pc_checkskill(sd,KN_RIDING)==1) // ���C�f�B���O�X�L������
		sd->max_weight +=battle_config.riding_weight; // Weight+��(�����ݒ��0)
	if( (skill=pc_checkskill(sd,MC_INCCARRY))>0 )	// �����ʑ���
		sd->max_weight += skill*2000;
	if( (skill=pc_checkskill(sd,SG_KNOWLEDGE))>0)// ���z�ƌ��Ɛ��̒m��
	{
	 	if(battle_config.check_knowlege_map)//�ꏊ�`�F�b�N���s�Ȃ�
	 	{
			if(sd->bl.m == sd->feel_map[0].m || sd->bl.m == sd->feel_map[1].m || sd->bl.m == sd->feel_map[2].m)
				sd->max_weight += sd->max_weight*skill/10;
		}else 
			sd->max_weight += sd->max_weight*skill/10;
	}
	
	if(first&1) {
		sd->weight=0;
		for(i=0;i<MAX_INVENTORY;i++){
			if(sd->status.inventory[i].nameid==0 || sd->inventory_data[i] == NULL)
				continue;
			sd->weight += sd->inventory_data[i]->weight*sd->status.inventory[i].amount;
		}
		sd->cart_max_weight=battle_config.max_cart_weight;
		sd->cart_weight=0;
		sd->cart_max_num=MAX_CART;
		sd->cart_num=0;
		for(i=0;i<MAX_CART;i++){
			if(sd->status.cart[i].nameid==0)
				continue;
			sd->cart_weight+=itemdb_weight(sd->status.cart[i].nameid)*sd->status.cart[i].amount;
			sd->cart_num++;
		}
	}

	memset(sd->paramb,0,sizeof(sd->paramb));
	memset(sd->parame,0,sizeof(sd->parame));
	sd->hit = 0;
	sd->flee = 0;
	sd->flee2 = 0;
	sd->critical = 0;
	sd->aspd = 0;
	sd->watk = 0;
	sd->def = 0;
	sd->mdef = 0;
	sd->watk2 = 0;
	sd->def2 = 0;
	sd->mdef2 = 0;
	sd->status.max_hp = 0;
	sd->status.max_sp = 0;
	sd->attackrange = 0;
	sd->attackrange_ = 0;
	sd->atk_ele = 0;
	sd->def_ele = 0;
	sd->star =0;
	sd->overrefine =0;
	sd->matk1 =0;
	sd->matk2 =0;
	sd->speed = DEFAULT_WALK_SPEED ;
	sd->hprate=battle_config.hp_rate;
	sd->sprate=battle_config.sp_rate;
	sd->castrate=100;
	sd->dsprate=100;
	sd->base_atk=0;
	sd->arrow_atk=0;
	sd->arrow_ele=0;
	sd->arrow_hit=0;
	sd->arrow_range=0;
	sd->nhealhp=sd->nhealsp=sd->nshealhp=sd->nshealsp=sd->nsshealhp=sd->nsshealsp=0;
	memset(sd->addele,0,sizeof(sd->addele));
	memset(sd->addrace,0,sizeof(sd->addrace));
	memset(sd->addsize,0,sizeof(sd->addsize));
	memset(sd->addele_,0,sizeof(sd->addele_));
	memset(sd->addrace_,0,sizeof(sd->addrace_));
	memset(sd->addsize_,0,sizeof(sd->addsize_));
	memset(sd->subele,0,sizeof(sd->subele));
	memset(sd->subrace,0,sizeof(sd->subrace));
	memset(sd->addeff,0,sizeof(sd->addeff));
	memset(sd->addeff2,0,sizeof(sd->addeff2));
	memset(sd->reseff,0,sizeof(sd->reseff));
	memset(&sd->special_state,0,sizeof(sd->special_state));
	memset(sd->weapon_coma_ele,0,sizeof(sd->weapon_coma_ele));
	memset(sd->weapon_coma_race,0,sizeof(sd->weapon_coma_race));
	memset(sd->weapon_coma_ele2,0,sizeof(sd->weapon_coma_ele2));
	memset(sd->weapon_coma_race2,0,sizeof(sd->weapon_coma_race2));
	memset(sd->weapon_atk,0,sizeof(sd->weapon_atk));
	memset(sd->weapon_atk_rate,0,sizeof(sd->weapon_atk_rate));

	sd->watk_ = 0;			//�񓁗��p(��)
	sd->watk_2 = 0;
	sd->atk_ele_ = 0;
	sd->star_ = 0;
	sd->overrefine_ = 0;

	sd->aspd_rate = 100;
	sd->speed_rate = 100;
	sd->hprecov_rate = 100;
	sd->sprecov_rate = 100;
	sd->critical_def = 0;
	sd->double_rate = 0;
	sd->near_attack_def_rate = sd->long_attack_def_rate = 0;
	sd->atk_rate = sd->matk_rate = 100;
	sd->ignore_def_ele = sd->ignore_def_race = 0;
	sd->ignore_def_ele_ = sd->ignore_def_race_ = 0;
	sd->ignore_mdef_ele = sd->ignore_mdef_race = 0;
	sd->arrow_cri = 0;
	sd->magic_def_rate = sd->misc_def_rate = 0;
	memset(sd->arrow_addele,0,sizeof(sd->arrow_addele));
	memset(sd->arrow_addrace,0,sizeof(sd->arrow_addrace));
	memset(sd->arrow_addsize,0,sizeof(sd->arrow_addsize));
	memset(sd->arrow_addeff,0,sizeof(sd->arrow_addeff));
	memset(sd->arrow_addeff2,0,sizeof(sd->arrow_addeff2));
	memset(sd->magic_addele,0,sizeof(sd->magic_addele));
	memset(sd->magic_addrace,0,sizeof(sd->magic_addrace));
	memset(sd->magic_subrace,0,sizeof(sd->magic_subrace));
	sd->perfect_hit = 0;
	sd->critical_rate = sd->hit_rate = sd->flee_rate = sd->flee2_rate = 100;
	sd->def_rate = sd->def2_rate = sd->mdef_rate = sd->mdef2_rate = 100;
	sd->def_ratio_atk_ele = sd->def_ratio_atk_ele_ = 0;
	sd->def_ratio_atk_race = sd->def_ratio_atk_race_ = 0;
	sd->get_zeny_num = 0;
	sd->add_damage_class_count = sd->add_damage_class_count_ = sd->add_magic_damage_class_count = 0;
	sd->add_def_class_count = sd->add_mdef_class_count = 0;
	sd->monster_drop_item_count = 0;
	memset(sd->add_damage_classrate,0,sizeof(sd->add_damage_classrate));
	memset(sd->add_damage_classrate_,0,sizeof(sd->add_damage_classrate_));
	memset(sd->add_magic_damage_classrate,0,sizeof(sd->add_magic_damage_classrate));
	memset(sd->add_def_classrate,0,sizeof(sd->add_def_classrate));
	memset(sd->add_mdef_classrate,0,sizeof(sd->add_mdef_classrate));
	memset(sd->monster_drop_race,0,sizeof(sd->monster_drop_race));
	memset(sd->monster_drop_itemrate,0,sizeof(sd->monster_drop_itemrate));
	sd->speed_add_rate = sd->aspd_add_rate = 100;
	sd->double_add_rate = sd->perfect_hit_add = sd->get_zeny_add_num = 0;
	sd->splash_range = sd->splash_add_range = 0;
	sd->autospell_id = sd->autospell_lv = sd->autospell_rate = sd->autospell_flag = 0;
	sd->hp_drain_rate = sd->hp_drain_per = sd->sp_drain_rate = sd->sp_drain_per = 0;
	sd->hp_drain_rate_ = sd->hp_drain_per_ = sd->sp_drain_rate_ = sd->sp_drain_per_ = 0;
	sd->hp_drain_value = sd->hp_drain_value_ = sd->sp_drain_value = sd->sp_drain_value_ = 0;
	sd->short_weapon_damage_return = sd->long_weapon_damage_return = sd->magic_damage_return = 0;
	sd->break_weapon_rate = sd->break_armor_rate = 0;
	sd->add_steal_rate = 0;
	sd->unbreakable_equip = 0;
	//�V�J�[�h�p������
	//sd->revautospell_id = sd->revautospell_lv=sd->revautospell_rate = sd->revautospell_flag = 0;
	sd->critical_damage=0;
	sd->hp_recov_stop = sd->sp_recov_stop = 0;
	memset(sd->critical_race,0,sizeof(sd->critical_race));
	memset(sd->critical_race_rate,0,sizeof(sd->critical_race_rate));
	memset(sd->subsize,0,sizeof(sd->subsize));
	memset(sd->magic_subsize,0,sizeof(sd->magic_subsize));
	memset(sd->exp_rate,0,sizeof(sd->exp_rate));
	memset(sd->job_rate,0,sizeof(sd->job_rate));
	memset(sd->hp_drain_rate_race,0,sizeof(sd->hp_drain_rate_race));
	memset(sd->sp_drain_rate_race,0,sizeof(sd->sp_drain_rate_race));
	memset(sd->hp_drain_value_race,0,sizeof(sd->hp_drain_value_race));
	memset(sd->sp_drain_value_race,0,sizeof(sd->sp_drain_value_race));
	memset(sd->addreveff,0,sizeof(sd->addreveff));
	sd->addreveff_flag = 0;
	memset(sd->addgroup,0,sizeof(sd->addgroup));
	memset(sd->addgroup_,0,sizeof(sd->addgroup_));
	memset(sd->arrow_addgroup,0,sizeof(sd->arrow_addgroup));
	memset(sd->subgroup,0,sizeof(sd->subgroup));
	sd->hp_penalty_time 	= 0;
	sd->hp_penalty_value 	= 0;
 	sd->sp_penalty_time 	= 0;
	sd->sp_penalty_value 	= 0;
	memset(sd->hp_penalty_unrig_value,0,sizeof(sd->hp_penalty_unrig_value));
	memset(sd->sp_penalty_unrig_value,0,sizeof(sd->sp_penalty_unrig_value));
	memset(sd->hp_rate_penalty_unrig,0,sizeof(sd->hp_rate_penalty_unrig));
	memset(sd->sp_rate_penalty_unrig,0,sizeof(sd->sp_rate_penalty_unrig));
	sd->mob_class_change_rate = 0;
	memset(&sd->skill_dmgup,0,sizeof(sd->skill_dmgup));
	memset(&sd->skill_blow,0,sizeof(sd->skill_blow));
	memset(&sd->autospell,0,sizeof(sd->autospell));
	memset(&sd->itemheal_rate,0,sizeof(sd->itemheal_rate));
	memset(&sd->autoraise,0,sizeof(sd->autoraise));
	sd->hp_vanish_rate = 0;
	sd->hp_vanish_per  = 0;
	sd->sp_vanish_rate = 0;
	sd->sp_vanish_per  = 0;
	sd->bonus_damage = 0;
	sd->curse_by_muramasa = 0;
	memset(sd->loss_equip_rate_when_die,0,sizeof(sd->loss_equip_rate_when_die));
	memset(sd->loss_equip_rate_when_attack,0,sizeof(sd->loss_equip_rate_when_attack));
	memset(sd->loss_equip_rate_when_hit,0,sizeof(sd->loss_equip_rate_when_hit));
	memset(sd->break_myequip_rate_when_attack,0,sizeof(sd->break_myequip_rate_when_attack));
	memset(sd->break_myequip_rate_when_hit,0,sizeof(sd->break_myequip_rate_when_hit));
	sd->loss_equip_flag = 0;
	sd->short_weapon_damege_rate = sd->long_weapon_damege_rate = 0;
 	//status_calc_pc_itemeffect_init(sd);

	for(i=0;i<10;i++) {
		index = sd->equip_index[i];
		current_equip_item_index = i;//(���ʃ`�F�b�N�p)
		if(index < 0)
			continue;
		if(i == 9 && sd->equip_index[8] == index)
			continue;
		if(i == 5 && sd->equip_index[4] == index)
			continue;
		if(i == 6 && (sd->equip_index[5] == index || sd->equip_index[4] == index))
			continue;
		
		if(sd->inventory_data[index]) {
			if(sd->inventory_data[index]->type == 4) {
				if(sd->status.inventory[index].card[0]!=0x00ff && sd->status.inventory[index].card[0]!=0x00fe && sd->status.inventory[index].card[0]!=(short)0xff00) {
					int j;
					for(j=0;j<sd->inventory_data[index]->slot;j++){	// �J�[�h
						int c=sd->status.inventory[index].card[j];
						current_equip_card_id = c;		//�I�[�g�X�y��(�d���`�F�b�N�p)
						if(c>0){
							if(i == 8 && sd->status.inventory[index].equip == 0x20)
								sd->state.lr_flag = 1;
							run_script(itemdb_equipscript(c),0,sd->bl.id,0);
							sd->state.lr_flag = 0;
						}
					}
				}
			}
			else if(sd->inventory_data[index]->type==5){ // �h��
				if(sd->status.inventory[index].card[0]!=0x00ff && sd->status.inventory[index].card[0]!=0x00fe && sd->status.inventory[index].card[0]!=(short)0xff00) {
					int j;
					for(j=0;j<sd->inventory_data[index]->slot;j++){	// �J�[�h
						int c=sd->status.inventory[index].card[j];
						current_equip_card_id = c;		//�I�[�g�X�y��(�d���`�F�b�N�p)
						if(c>0)
							run_script(itemdb_equipscript(c),0,sd->bl.id,0);
					}
				}
			}
		}
	}
	
	
	wele = sd->atk_ele;
	wele_ = sd->atk_ele_;
	def_ele = sd->def_ele;
	if(battle_config.pet_status_support) {
		if(sd->status.pet_id > 0 && sd->petDB && sd->pet.intimate > 0)
			run_script(sd->petDB->script,0,sd->bl.id,0);
		pele = sd->atk_ele;
		pdef_ele = sd->def_ele;
		sd->atk_ele = sd->def_ele = 0;
	}
	memcpy(sd->paramcard,sd->parame,sizeof(sd->paramcard));

	// �����i�ɂ��X�e�[�^�X�ω��͂����Ŏ��s
	for(i=0;i<10;i++) {
		index = sd->equip_index[i];
		current_equip_item_index = i;//index;	//���ʃ`�F�b�N�p
		current_equip_card_id = index;		//�I�[�g�X�y��(�d���`�F�b�N�p)
		if(index < 0)
			continue;
		if(i == 9 && sd->equip_index[8] == index)
			continue;
		if(i == 5 && sd->equip_index[4] == index)
			continue;
		if(i == 6 && (sd->equip_index[5] == index || sd->equip_index[4] == index))
			continue;
		if(sd->inventory_data[index]) {
			sd->def += sd->inventory_data[index]->def;
			if(sd->inventory_data[index]->type == 4) {
				int r,wlv = sd->inventory_data[index]->wlv;
				if(i == 8 && sd->status.inventory[index].equip == 0x20) {
					//�񓁗��p�f�[�^����
					sd->watk_ += sd->inventory_data[index]->atk;
					sd->watk_2 = (r=sd->status.inventory[index].refine)*	// ���B�U����
						refinebonus[wlv][0];
					if( (r-=refinebonus[wlv][2])>0 )	// �ߏ萸�B�{�[�i�X
						sd->overrefine_ = r*refinebonus[wlv][1];

					if(sd->status.inventory[index].card[0]==0x00ff){	// ��������
						sd->star_ = (sd->status.inventory[index].card[1]>>8);	// ���̂�����
						if(sd->star_ == 15) sd->star_ = 40;
						wele_= (sd->status.inventory[index].card[1]&0x0f);	// �� ��
					}
					sd->attackrange_ += sd->inventory_data[index]->range;
					sd->state.lr_flag = 1;
					run_script(sd->inventory_data[index]->equip_script,0,sd->bl.id,0);
					sd->state.lr_flag = 0;
				}
				else {	//�񓁗�����ȊO
					sd->watk += sd->inventory_data[index]->atk;
					sd->watk2 += (r=sd->status.inventory[index].refine)*	// ���B�U����
						refinebonus[wlv][0];
					if( (r-=refinebonus[wlv][2])>0 )	// �ߏ萸�B�{�[�i�X
						sd->overrefine += r*refinebonus[wlv][1];

					if(sd->status.inventory[index].card[0]==0x00ff){	// ��������
						sd->star += (sd->status.inventory[index].card[1]>>8);	// ���̂�����
						if(sd->star == 15) sd->star = 40;
						wele = (sd->status.inventory[index].card[1]&0x0f);	// �� ��
					}
					sd->attackrange += sd->inventory_data[index]->range;
					run_script(sd->inventory_data[index]->equip_script,0,sd->bl.id,0);
				}
			}
			else if(sd->inventory_data[index]->type == 5) {
				sd->watk += sd->inventory_data[index]->atk;
				refinedef += sd->status.inventory[index].refine*refinebonus[0][0];
				run_script(sd->inventory_data[index]->equip_script,0,sd->bl.id,0);
			}
		}
	}
	//�����{�[�i�X�̌�n��
	//���݂̓I�[�g�X�y���̋֎~�������������x�̏���
	status_calc_pc_itemeffect_finish(sd);
	
	if(sd->equip_index[10] >= 0){ // ��
		index = sd->equip_index[10];
		if(sd->inventory_data[index]){		//�܂������������Ă��Ȃ�
			sd->state.lr_flag = 2;
			run_script(sd->inventory_data[index]->equip_script,0,sd->bl.id,0);
			sd->state.lr_flag = 0;
			sd->arrow_atk += sd->inventory_data[index]->atk;
		}
	}
	
	sd->def += (refinedef+50)/100;

	if(sd->attackrange < 1) sd->attackrange = 1;
	if(sd->attackrange_ < 1) sd->attackrange_ = 1;
	if(sd->attackrange < sd->attackrange_)
		sd->attackrange = sd->attackrange_;
	if(sd->status.weapon == 11)
		sd->attackrange += sd->arrow_range;
	if(wele > 0)
		sd->atk_ele = wele;
	if(wele_ > 0)
		sd->atk_ele_ = wele_;
	if(def_ele > 0)
		sd->def_ele = def_ele;
	if(battle_config.pet_status_support) {
		if(pele > 0 && !sd->atk_ele)
			sd->atk_ele = pele;
		if(pdef_ele > 0 && !sd->def_ele)
			sd->def_ele = pdef_ele;
	}
	sd->double_rate += sd->double_add_rate;
	sd->perfect_hit += sd->perfect_hit_add;
	sd->get_zeny_num += sd->get_zeny_add_num;
	sd->splash_range += sd->splash_add_range;
	if(sd->speed_add_rate != 100)
		sd->speed_rate += sd->speed_add_rate - 100;
	if(sd->aspd_add_rate != 100)
		sd->aspd_rate += sd->aspd_add_rate - 100;

	// ����ATK�T�C�Y�␳ (�E��)
	sd->atkmods[0] = atkmods[0][sd->weapontype1];
	sd->atkmods[1] = atkmods[1][sd->weapontype1];
	sd->atkmods[2] = atkmods[2][sd->weapontype1];
	//����ATK�T�C�Y�␳ (����)
	sd->atkmods_[0] = atkmods[0][sd->weapontype2];
	sd->atkmods_[1] = atkmods[1][sd->weapontype2];
	sd->atkmods_[2] = atkmods[2][sd->weapontype2];

	// job�{�[�i�X��
	for(i=0;i<sd->status.job_level && i<MAX_LEVEL;i++){
		if(job_bonus[s_class.upper][s_class.job][i])
			sd->paramb[job_bonus[s_class.upper][s_class.job][i]-1]++;
	}

	if( (skill=pc_checkskill(sd,AC_OWL))>0 )	// �ӂ��낤�̖�
		sd->paramb[4] += skill;

	if( pc_checkskill(sd,BS_HILTBINDING) ) {	// �q���g�o�C���f�B���O
		sd->paramb[0] += 1;
		sd->watk += 4;
	}
	
	if( (skill=pc_checkskill(sd,SA_DRAGONOLOGY))>0 ){// �h���S�m���W�[
		sd->paramb[3] += (int)((skill+1)*0.5);
	}
	if( (skill=pc_checkskill(sd,TF_DOUBLE))>0 ){// �_�u���A�^�b�N
		sd->hit += skill;
	}
	
	// �X�e�[�^�X�ω��ɂ���{�p�����[�^�␳
	if(sd->sc_count){
		if(sd->sc_data[SC_INCALLSTATUS].timer!=-1){
			sd->paramb[0]+= sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[1]+= sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[2]+= sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[3]+= sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[4]+= sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[5]+= sd->sc_data[SC_INCALLSTATUS].val1;
		}
		
		//��ʈꎟ�E�̍�
		//��Ȃ̂�LV/10����
		if(sd->sc_data[SC_HIGH].timer!=-1)
		{
			sd->paramb[0]+= sd->status.base_level/10;
			sd->paramb[1]+= sd->status.base_level/10;
			sd->paramb[2]+= sd->status.base_level/10;
			sd->paramb[3]+= sd->status.base_level/10;
			sd->paramb[4]+= sd->status.base_level/10;
			sd->paramb[5]+= sd->status.base_level/10;
		}
		
		//�H���p
		if(sd->sc_data[SC_MEAL_INCSTR].timer!=-1)
			sd->paramb[0]+= sd->sc_data[SC_MEAL_INCSTR].val1;
		if(sd->sc_data[SC_MEAL_INCAGI].timer!=-1)
			sd->paramb[1]+= sd->sc_data[SC_MEAL_INCAGI].val1;
		if(sd->sc_data[SC_MEAL_INCVIT].timer!=-1)
			sd->paramb[2]+= sd->sc_data[SC_MEAL_INCVIT].val1;
		if(sd->sc_data[SC_MEAL_INCINT].timer!=-1)
			sd->paramb[3]+= sd->sc_data[SC_MEAL_INCINT].val1;
		if(sd->sc_data[SC_MEAL_INCDEX].timer!=-1)
			sd->paramb[4]+= sd->sc_data[SC_MEAL_INCDEX].val1;
		if(sd->sc_data[SC_MEAL_INCLUK].timer!=-1)
			sd->paramb[5]+= sd->sc_data[SC_MEAL_INCLUK].val1;
			
		//�삯����STR +10
		if(sd->sc_data[SC_SPURT].timer!=-1)
			sd->paramb[0] += 10;
			
		//�M���h�X�L�� �Ր�Ԑ�
		if(sd->sc_data[SC_BATTLEORDER].timer!=-1){
			sd->paramb[0]+= 5*sd->sc_data[SC_BATTLEORDER].val1;
			sd->paramb[3]+= 5*sd->sc_data[SC_BATTLEORDER].val1;
			sd->paramb[4]+= 5*sd->sc_data[SC_BATTLEORDER].val1;
		}
		
		if(sd->sc_data[SC_CHASEWALK_STR].timer!=-1)
			sd->paramb[0] += sd->sc_data[SC_CHASEWALK_STR].val1;
			
		if(sd->sc_data[SC_CONCENTRATE].timer!=-1 && sd->sc_data[SC_QUAGMIRE].timer == -1){	// �W���͌���
			sd->paramb[1]+= (sd->status.agi+sd->paramb[1]+sd->parame[1]-sd->paramcard[1])*(2+sd->sc_data[SC_CONCENTRATE].val1)/100;
			sd->paramb[4]+= (sd->status.dex+sd->paramb[4]+sd->parame[4]-sd->paramcard[4])*(2+sd->sc_data[SC_CONCENTRATE].val1)/100;
		}
		if(sd->sc_data[SC_INCREASEAGI].timer!=-1) {	// ���x����
			sd->paramb[1]+= 2+sd->sc_data[SC_INCREASEAGI].val1;
			sd->speed -= sd->speed *25/100;
		}
		if(sd->sc_data[SC_DECREASEAGI].timer!=-1){	// ���x����(agi��battle.c��)
			sd->paramb[1]-= 2+sd->sc_data[SC_DECREASEAGI].val1;
			if(sd->sc_data[SC_DEFENDER].timer != -1)//�f�B�t�F���_�[���͑��x�ቺ���Ȃ�
				sd->speed += 0;
			else
				sd->speed = sd->speed *125/100;
		}
		if(sd->sc_data[SC_BLESSING].timer!=-1){	// �u���b�V���O
			sd->paramb[0]+= sd->sc_data[SC_BLESSING].val1;
			sd->paramb[3]+= sd->sc_data[SC_BLESSING].val1;
			sd->paramb[4]+= sd->sc_data[SC_BLESSING].val1;
		}
		if(sd->sc_data[SC_GLORIA].timer!=-1)	// �O�����A
			sd->paramb[5]+= 30;
		if(sd->sc_data[SC_LOUD].timer!=-1 && sd->sc_data[SC_QUAGMIRE].timer == -1)	// ���E�h�{�C�X
			sd->paramb[0]+= 4;
		if(sd->sc_data[SC_QUAGMIRE].timer!=-1){	// �N�@�O�}�C�A
			short subagi = 0;
			short subdex = 0;
			subagi = (sd->status.agi/2 < sd->sc_data[SC_QUAGMIRE].val1*10) ? sd->status.agi/2 : sd->sc_data[SC_QUAGMIRE].val1*10;
			subdex = (sd->status.dex/2 < sd->sc_data[SC_QUAGMIRE].val1*10) ? sd->status.dex/2 : sd->sc_data[SC_QUAGMIRE].val1*10;
			if(map[sd->bl.m].flag.pvp || map[sd->bl.m].flag.gvg){
				subagi/= 2;
				subdex/= 2;
			}
			sd->speed = sd->speed*4/3;
			sd->paramb[1]-= subagi;
			sd->paramb[4]-= subdex;
		}
		if(sd->sc_data[SC_TRUESIGHT].timer!=-1){	// �g�D���[�T�C�g
			sd->paramb[0]+= 5;
			sd->paramb[1]+= 5;
			sd->paramb[2]+= 5;
			sd->paramb[3]+= 5;
			sd->paramb[4]+= 5;
			sd->paramb[5]+= 5;
		}
	}

	//1�x������łȂ�Job70�X�p�m�r��+10
	if(s_class.job == 23 && (sd->die_counter == 0 || sd->repeal_die_counter == 1)&& sd->status.job_level >= 70){
		sd->paramb[0]+= 10;
		sd->paramb[1]+= 10;
		sd->paramb[2]+= 10;
		sd->paramb[3]+= 10;
		sd->paramb[4]+= 10;
		sd->paramb[5]+= 10;
	}
	
	//�M���h�X�L��
	//�X�L���L�� && �M���h�L && �}�X�^�[�ڑ� && ����!=�}�X�^�[ && �����}�b�v
	if(battle_config.guild_hunting_skill_available && g && gmsd
		&& ((battle_config.allow_me_guild_skill==1) || (gmsd != sd))
		&& sd->bl.m == gmsd->bl.m)//
	{
		int dx,dy,range;
		//����������s��
		if(battle_config.guild_skill_check_range){
			dx = abs(sd->bl.x - gmsd->bl.x);
			dy = abs(sd->bl.y - gmsd->bl.y);
			if(battle_config.guild_skill_effective_range > 0)//���ꋗ���Ōv�Z
			{
				range = battle_config.guild_skill_effective_range;
				if(dx <=range &&  dy <= range){
					sd->paramb[0] += guild_checkskill(g,GD_LEADERSHIP);//str
					sd->paramb[1] += guild_checkskill(g,GD_SOULCOLD);//agi
					sd->paramb[2] += guild_checkskill(g,GD_GLORYWOUNDS);//vit
					sd->paramb[4] += guild_checkskill(g,GD_HAWKEYES);//dex
					sd->under_the_influence_of_the_guild_skill = range+1;//(0>�ŉe����,�d�Ȃ�ꍇ������̂�+1)
				}else
					sd->under_the_influence_of_the_guild_skill = 0;
			}else{//�ʋ���
				int min_range=999;
				range = skill_get_range(GD_LEADERSHIP,guild_skill_get_lv(g,GD_LEADERSHIP));
				if(dx <=range &&  dy <= range){
					sd->paramb[0] += guild_checkskill(g,GD_LEADERSHIP);//str
					if(min_range>range) min_range = range;
				}
				range = skill_get_range(GD_SOULCOLD,guild_skill_get_lv(g,GD_SOULCOLD));	
				if(dx <=range &&  dy <= range){
					sd->paramb[1] += guild_checkskill(g,GD_SOULCOLD);//agi
					if(min_range>range) min_range = range;
				}
				range = skill_get_range(GD_GLORYWOUNDS,guild_skill_get_lv(g,GD_GLORYWOUNDS));
				if(dx <=range &&  dy <= range){
					sd->paramb[2] += guild_checkskill(g,GD_GLORYWOUNDS);//vit
					if(min_range>range) min_range = range;
				}
				
				range = skill_get_range(GD_HAWKEYES,guild_skill_get_lv(g,GD_HAWKEYES));
				if(dx <=range &&  dy <= range){
					sd->paramb[4] += guild_checkskill(g,GD_HAWKEYES);//dex
					if(min_range>range) min_range = range;
				}
				if(min_range == 999) sd->under_the_influence_of_the_guild_skill = 0;
				else sd->under_the_influence_of_the_guild_skill = min_range+1;
			}

		}else{//�}�b�v�S��
			sd->paramb[0] += guild_checkskill(g,GD_LEADERSHIP);//str
			sd->paramb[1] += guild_checkskill(g,GD_SOULCOLD);//agi
			sd->paramb[2] += guild_checkskill(g,GD_GLORYWOUNDS);//vit
			sd->paramb[4] += guild_checkskill(g,GD_HAWKEYES);//dex
			sd->under_the_influence_of_the_guild_skill = battle_config.guild_skill_effective_range+1;
		}
	}else{//�}�b�v���������c������������
		sd->under_the_influence_of_the_guild_skill = 0;
	}
	
	
	sd->paramc[0]=sd->status.str+sd->paramb[0]+sd->parame[0];
	sd->paramc[1]=sd->status.agi+sd->paramb[1]+sd->parame[1];
	sd->paramc[2]=sd->status.vit+sd->paramb[2]+sd->parame[2];
	sd->paramc[3]=sd->status.int_+sd->paramb[3]+sd->parame[3];
	sd->paramc[4]=sd->status.dex+sd->paramb[4]+sd->parame[4];
	sd->paramc[5]=sd->status.luk+sd->paramb[5]+sd->parame[5];
	for(i=0;i<6;i++)
		if(sd->paramc[i] < 0) sd->paramc[i] = 0;

	if(sd->status.weapon == 11 || sd->status.weapon == 13 || sd->status.weapon == 14) {
		str = sd->paramc[4];
		dex = sd->paramc[0];
	}
	else {
		str = sd->paramc[0];
		dex = sd->paramc[4];
	}
	dstr = str/10;
	sd->base_atk += str + dstr*dstr + dex/5 + sd->paramc[5]/5;
	sd->matk1 += sd->paramc[3]+(sd->paramc[3]/5)*(sd->paramc[3]/5);
	sd->matk2 += sd->paramc[3]+(sd->paramc[3]/7)*(sd->paramc[3]/7);
	if(sd->matk1 < sd->matk2) {
		int temp = sd->matk2;
		sd->matk2 = sd->matk1;
		sd->matk1 = temp;
	}
	sd->hit += sd->paramc[4] + sd->status.base_level;
	sd->flee += sd->paramc[1] + sd->status.base_level;
	sd->def2 += sd->paramc[2];
	sd->mdef2 += sd->paramc[3];
	sd->flee2 += sd->paramc[5]+10;
	sd->critical += (sd->paramc[5]*3)+10;

	if(sd->base_atk < 1)
		sd->base_atk = 1;
	if(sd->critical_rate != 100)
		sd->critical = (sd->critical*sd->critical_rate)/100;
	if(sd->critical < 10) sd->critical = 10;
	if(sd->hit_rate != 100)
		sd->hit = (sd->hit*sd->hit_rate)/100;
	if(sd->hit < 1) sd->hit = 1;
	if(sd->flee_rate != 100)
		sd->flee = (sd->flee*sd->flee_rate)/100;
	if(sd->flee < 1) sd->flee = 1;
	if(sd->flee2_rate != 100)
		sd->flee2 = (sd->flee2*sd->flee2_rate)/100;
	if(sd->flee2 < 10) sd->flee2 = 10;
	if(sd->def_rate != 100)
		sd->def = (sd->def*sd->def_rate)/100;
	if(sd->def < 0) sd->def = 0;
	if(sd->def2_rate != 100)
		sd->def2 = (sd->def2*sd->def2_rate)/100;
	if(sd->def2 < 1) sd->def2 = 1;
	if(sd->mdef_rate != 100)
		sd->mdef = (sd->mdef*sd->mdef_rate)/100;
	if(sd->mdef < 0) sd->mdef = 0;
	if(sd->mdef2_rate != 100)
		sd->mdef2 = (sd->mdef2*sd->mdef2_rate)/100;
	if(sd->mdef2 < 1) sd->mdef2 = 1;

	// �񓁗� ASPD �C��
	if (sd->status.weapon <= 16)
		sd->aspd += aspd_base[s_class.job][sd->status.weapon]-(sd->paramc[1]*4+sd->paramc[4])*aspd_base[s_class.job][sd->status.weapon]/1000;
	else
		sd->aspd += (
			(aspd_base[s_class.job][sd->weapontype1]-(sd->paramc[1]*4+sd->paramc[4])*aspd_base[s_class.job][sd->weapontype1]/1000) +
			(aspd_base[s_class.job][sd->weapontype2]-(sd->paramc[1]*4+sd->paramc[4])*aspd_base[s_class.job][sd->weapontype2]/1000)
			) * 140 / 200;

	aspd_rate = sd->aspd_rate;

	//�U�����x����

	//�A�h�o���X�h�u�b�N
	if(sd->weapontype1 == 0x0f && (skill = pc_checkskill(sd,SA_ADVANCEDBOOK)) > 0)
	{
		aspd_rate -= skill/2;
	}
	
	//���z�ƌ��Ɛ��̈���
	if((skill = pc_checkskill(sd,SG_DEVIL)) > 0)
	{
		aspd_rate -= skill*3;
		if(sd->sc_data[SC_DEVIL].timer!=-1 || sd->sc_data[SC_DEVIL].val1<skill)
			status_change_start(&sd->bl,SC_DEVIL,skill,0,0,0,5000,0);
		clif_status_change(&sd->bl,SI_DEVIL,1);
	}
	
	//���z�ƌ��Ɛ��̗Z��
	if(sd && sd->sc_data[SC_FUSION].timer!=-1)
	{
		aspd_rate -= 20;
	}
	
	if( (skill=pc_checkskill(sd,AC_VULTURE))>0){	// ���V�̖�
		sd->hit += skill;
		if(sd->status.weapon == 11)
			sd->attackrange += skill;
	}

	if( (skill=pc_checkskill(sd,BS_WEAPONRESEARCH))>0)	// ���팤���̖���������
		sd->hit += skill*2;
	if(sd->status.option&2 && (skill = pc_checkskill(sd,RG_TUNNELDRIVE))>0 )	// �g���l���h���C�u	// �g���l���h���C�u
		sd->speed += (short)(1.2*DEFAULT_WALK_SPEED - skill*9);
	if (pc_iscarton(sd) && (skill=pc_checkskill(sd,MC_PUSHCART))>0)	// �J�[�g�ɂ�鑬�x�ቺ
		sd->speed += (short)((10-skill) * (DEFAULT_WALK_SPEED * 0.1));
	else if (pc_isriding(sd))	// �y�R�y�R���ɂ�鑬�x����
		sd->speed -= (short)(0.25 * DEFAULT_WALK_SPEED);
	if( s_class.job == 12 && (skill=pc_checkskill(sd,TF_MISS))>0 )	// �A�T�V���n�̉�𗦏㏸�ɂ�鑬�x����
		sd->speed -= (short)(skill*DEFAULT_WALK_SPEED/100);

	if(sd->sc_count){
		if(sd->sc_data[SC_CLOAKING].timer!=-1)//�N���[�L���O�ɂ�鑬�x�ω�
		{
			skill=pc_checkskill(sd,AS_CLOAKING);
			if ((skill=pc_checkskill(sd,AS_CLOAKING))>2)
			{
				static int dx[]={-1, 0, 1,-1, 1,-1, 0, 1};
				static int dy[]={-1,-1,-1, 0, 0, 1, 1, 1};
				struct block_list *bl = &sd->bl;
				int check=1,ii;
				nullpo_retr(0, bl);
				for(ii=0;ii<sizeof(dx)/sizeof(dx[0]);ii++){
					if(map_getcell(bl->m,bl->x+dx[ii],bl->y+dy[ii],CELL_CHKNOPASS)){
						check=0;
						break;
					}
				}
				if(check){
					// ���n�ړ����x 
					sd->speed += ((10-skill) * 3);
				}else{
					// �ǉ����ړ����x 
					int cloak_speed_table[11]={100,100,103,106,109,112,115,118,121,124,125};
					sd->speed -= (sd->speed * (cloak_speed_table[skill]-100) )/100;
				}
			}
		}
		if(sd->sc_data[SC_CHASEWALK].timer!=-1)/*�`�F�C�X�E�H�[�N�ɂ�鑬�x�ω�*/
			sd->speed = sd->speed * sd->sc_data[SC_CHASEWALK].val3 /100;
		if(sd->sc_data[SC_WINDWALK].timer!=-1) 	//�E�B���h�E�H�[�N����Lv*2%���Z
			sd->speed -= sd->speed *(sd->sc_data[SC_WINDWALK].val1*2)/100;
		if(sd->sc_data[SC_CARTBOOST].timer!=-1)	// �J�[�g�u�[�X�g
			sd->speed -= (DEFAULT_WALK_SPEED * 20)/100;
		if(sd->sc_data[SC_BERSERK].timer!=-1)	//�o�[�T�[�N����IA�Ɠ������炢�����H
			sd->speed -= sd->speed *25/100;
		if(sd->sc_data[SC_WEDDING].timer!=-1)	//�������͕����̂��x��
			sd->speed = 2*DEFAULT_WALK_SPEED;
	}

	if((skill=pc_checkskill(sd,CR_TRUST))>0) { // �t�F�C�X
		sd->status.max_hp += skill*200;
		sd->subele[6] += skill*5;
	}
	if((skill=pc_checkskill(sd,BS_SKINTEMPER))>0){ // �X�L���e���p�����O
		sd->subele[3] += skill*5;
		sd->subele[0] += skill*1;
	}

	bl=sd->status.base_level;

	//�ő�HP�v�Z
	//�]���E�̏ꍇ�ő�HP25%UP
	if(pc_isupper(sd))
		sd->status.max_hp += ((3500 + bl*hp_coefficient2[s_class.job] + hp_sigma_val[s_class.job][(bl > 0)? bl-1:0])/100 * (100 + sd->paramc[2])/100 + (sd->parame[2] - sd->paramcard[2])) * 125/100;
	else if(pc_isadoptee(sd))//�{�q�̏ꍇ�ő�HP70%
		sd->status.max_hp += ((3500 + bl*hp_coefficient2[s_class.job] + hp_sigma_val[s_class.job][(bl > 0)? bl-1:0])/100 * (100 + sd->paramc[2])/100 + (sd->parame[2] - sd->paramcard[2])) * 70/100;
	else sd->status.max_hp += (3500 + bl*hp_coefficient2[s_class.job] + hp_sigma_val[s_class.job][(bl > 0)? bl-1:0])/100 * (100 + sd->paramc[2])/100 + (sd->parame[2] - sd->paramcard[2]);

	if(sd->hprate!=100)
		sd->status.max_hp = sd->status.max_hp*sd->hprate/100;

	if(sd->sc_data && sd->sc_data[SC_BERSERK].timer!=-1){	// �o�[�T�[�N
		sd->status.max_hp = sd->status.max_hp * 3;
	}
	if(s_class.job == 23 && sd->status.base_level >= 99){
		sd->status.max_hp = sd->status.max_hp + 2000;
	}
	if(sd->sc_data && sd->sc_data[SC_INCMHP2].timer!=-1){
		sd->status.max_hp *= (100+sd->sc_data[SC_INCMHP2].val1)/100;
	}

	if(sd->status.max_hp < 0 || sd->status.max_hp > battle_config.max_hp)
		sd->status.max_hp = battle_config.max_hp;

	// �ő�SP�v�Z
	//�]���E�̏ꍇ�ő�SP125%
	if(pc_isupper(sd))
		sd->status.max_sp += (((sp_coefficient[s_class.job] * bl) + 1000)/100 * (100 + sd->paramc[3])/100 + (sd->parame[3] - sd->paramcard[3])) * 125/100;
	else if(pc_isadoptee(sd)) //�{�q�̏ꍇ�ő�SP70%
		sd->status.max_sp += (((sp_coefficient[s_class.job] * bl) + 1000)/100 * (100 + sd->paramc[3])/100 + (sd->parame[3] - sd->paramcard[3])) * 70/100;
	else sd->status.max_sp += ((sp_coefficient[s_class.job] * bl) + 1000)/100 * (100 + sd->paramc[3])/100 + (sd->parame[3] - sd->paramcard[3]);

	if(sd->sprate!=100)
		sd->status.max_sp = sd->status.max_sp*sd->sprate/100;

	if((skill=pc_checkskill(sd,HP_MEDITATIO))>0) // ���f�B�e�C�e�B�I 
		sd->status.max_sp += sd->status.max_sp*skill/100;
	if((skill=pc_checkskill(sd,HW_SOULDRAIN))>0) /* �\�E���h���C�� */
		sd->status.max_sp += sd->status.max_sp*2*skill/100;
	if(sd->sc_data && sd->sc_data[SC_INCMSP2].timer!=-1) {
		sd->status.max_sp *= (100+sd->sc_data[SC_INCMSP2].val1)/100;
	}
	if((skill=pc_checkskill(sd,SL_KAINA))>0) /* �J�C�i */
		sd->status.max_sp += 30*skill;

	if(sd->status.max_sp < 0 || sd->status.max_sp > battle_config.max_sp)
		sd->status.max_sp = battle_config.max_sp;

	//���R��HP
	sd->nhealhp = 1 + (sd->paramc[2]/5) + (sd->status.max_hp/200);
	if((skill=pc_checkskill(sd,SM_RECOVERY)) > 0) {	/* HP�񕜗͌��� */
		sd->nshealhp = skill*5 + (sd->status.max_hp*skill/500);
		if(sd->nshealhp > 0x7fff) sd->nshealhp = 0x7fff;
	}
	if((skill=pc_checkskill(sd,TK_HPTIME)) > 0) {	// ���炩�ȋx�� 
		sd->tk_nhealhp = skill*30 + (sd->status.max_hp*skill/500);
		if(sd->tk_nhealhp > 0x7fff) sd->tk_nhealhp = 0x7fff;
	}
	if(sd->sc_data && sd->sc_data[SC_BERSERK].timer!=-1){
		sd->nhealhp = 0;
	}
	//���R��SP
	sd->nhealsp = 1 + (sd->paramc[3]/6) + (sd->status.max_sp/100);
	if(sd->paramc[3] >= 120)
		sd->nhealsp += ((sd->paramc[3]-120)>>1) + 4;
	if((skill=pc_checkskill(sd,MG_SRECOVERY)) > 0) { /* SP�񕜗͌��� */
		sd->nshealsp = skill*3 + (sd->status.max_sp*skill/500);
		if(sd->nshealsp > 0x7fff) sd->nshealsp = 0x7fff;
	}

	
	if((skill = pc_checkskill(sd,MO_SPIRITSRECOVERY)) > 0) {
		sd->nsshealhp = skill*4 + (sd->status.max_hp*skill/500);
		sd->nsshealsp = skill*2 + (sd->status.max_sp*skill/500);
		if(sd->nsshealhp > 0x7fff) sd->nsshealhp = 0x7fff;
		if(sd->nsshealsp > 0x7fff) sd->nsshealsp = 0x7fff;
	}
	if((skill=pc_checkskill(sd,TK_SPTIME)) > 0) { // �y�����x��
		sd->tk_nhealsp = skill*3 + (sd->status.max_sp*skill/500);
		if(sd->tk_nhealsp > 0x7fff) sd->tk_nhealsp = 0x7fff;
	}
	if(sd->hprecov_rate != 100) {
		sd->nhealhp = sd->nhealhp*sd->hprecov_rate/100;
		if(sd->nhealhp < 1) sd->nhealhp = 1;
	}
	if(sd->sprecov_rate != 100) {
		sd->nhealsp = sd->nhealsp*sd->sprecov_rate/100;
		if(sd->nhealsp < 1) sd->nhealsp = 1;
	}
	if((skill=pc_checkskill(sd,HP_MEDITATIO)) > 0) {
		// ���f�B�e�C�e�B�I��SPR�ł͂Ȃ����R�񕜂ɂ�����
		sd->nhealsp += (sd->nhealsp)*3*skill/100;
		if(sd->nhealsp > 0x7fff) sd->nhealsp = 0x7fff;
	}

	// �푰�ϐ��i����ł����́H �f�B�o�C���v���e�N�V�����Ɠ������������邩���j
	if( (skill=pc_checkskill(sd,SA_DRAGONOLOGY))>0 ){	// �h���S�m���W�[
		skill = skill*4;
		sd->addrace[9]+=skill;
		sd->addrace_[9]+=skill;
		sd->subrace[9]+=skill;
	}

	//Flee�㏸
	if( (skill=pc_checkskill(sd,TF_MISS))>0 ) {	// ��𗦑���
		if( s_class.job == 12 || s_class.job == 17 )
			sd->flee += skill*4;
		else
			sd->flee += skill*3;
	}
	if( (skill=pc_checkskill(sd,MO_DODGE))>0 )	// ���؂�
		sd->flee += (skill*3)>>1;
	if( sd->sc_data ) {
		if( sd->sc_data[SC_INCFLEE].timer!=-1 )
			sd->flee += sd->sc_data[SC_INCFLEE].val1;
		if( sd->sc_data[SC_INCFLEE2].timer!=-1 )
			sd->flee += sd->sc_data[SC_INCFLEE2].val1;
	}
			
	// �X�L����X�e�[�^�X�ُ�ɂ��c��̃p�����[�^�␳
	if(sd->sc_count){
		
		if(sd->sc_data)
		{
			//���z�̈��y DEF����
			if(sd->sc_data[SC_SUN_COMFORT].timer!=-1 && sd->bl.m == sd->feel_map[0].m)
				sd->def += (sd->status.base_level + sd->status.dex + sd->status.luk)/10;
				//sd->def += (sd->status.base_level + sd->status.dex + sd->status.luk + sd->paramb[4] + sd->paramb[5])/10;
			
			//���̈��y
			if(sd->sc_data[SC_MOON_COMFORT].timer!=-1 && sd->bl.m == sd->feel_map[1].m)
				sd->flee += (sd->status.base_level + sd->status.dex + sd->status.luk)/10;
				//sd->flee += (sd->status.base_level + sd->status.dex + sd->status.luk + sd->paramb[4] + sd->paramb[5])/10;
			
			//���̈��y
			if(sd->sc_data[SC_STAR_COMFORT].timer!=-1 && sd->bl.m == sd->feel_map[2].m)
				aspd_rate -= (sd->status.base_level + sd->status.dex + sd->status.luk)/10;
				//aspd_rate += (sd->status.base_level + sd->status.dex + sd->status.luk + sd->paramb[0] + sd->paramb[4] + sd->paramb[5])/10;
		}
		// ATK/DEF�ω��`
		if(sd->sc_data[SC_ANGELUS].timer!=-1)	// �G���W�F���X
			sd->def2 = sd->def2*(110+5*sd->sc_data[SC_ANGELUS].val1)/100;
		if(sd->sc_data[SC_IMPOSITIO].timer!=-1)	{// �C���|�V�e�B�I�}�k�X
			sd->watk += sd->sc_data[SC_IMPOSITIO].val1*5;
			index = sd->equip_index[8];
			if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == 4)
				sd->watk_ += sd->sc_data[SC_IMPOSITIO].val1*5;
		}
		if(sd->sc_data[SC_PROVOKE].timer!=-1){	// �v���{�b�N
			sd->def2 = sd->def2*(100-6*sd->sc_data[SC_PROVOKE].val1)/100;
			sd->base_atk = sd->base_atk*(100+2*sd->sc_data[SC_PROVOKE].val1)/100;
			sd->watk = sd->watk*(100+2*sd->sc_data[SC_PROVOKE].val1)/100;
			index = sd->equip_index[8];
			if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == 4)
				sd->watk_ = sd->watk_*(100+2*sd->sc_data[SC_PROVOKE].val1)/100;
		}
		if(sd->sc_data[SC_POISON].timer!=-1)	// �ŏ��
			sd->def2 = sd->def2*75/100;
		
		//pc_get_atk1()?
		if(sd->sc_data[SC_THE_MAGICIAN].timer!=-1)	//THE MAGICIAN ATK����
		{
			//ATK
			sd->base_atk = sd->base_atk * 50/100;
			sd->watk = sd->watk * 50/100;
			index = sd->equip_index[8];
			if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == 4)
				sd->watk_ = sd->watk * 50/100;
		}
		//
		if(sd->sc_data[SC_THE_MAGICIAN].timer!=-1)	//STRENGTH MATK����
		{
			//MATK
			sd->matk1 = sd->matk1*50/100;
			sd->matk2 = sd->matk2*50/100;
		}
		//
		if(sd->sc_data[SC_THE_DEVIL].timer!=-1)	//THE DEVIL ATK�����AMATK����
		{
			//ATK
			if(sd->sc_data[SC_THE_MAGICIAN].timer==-1)
			{
				sd->base_atk = sd->base_atk * 50/100;
				sd->watk = sd->watk * 50/100;
				index = sd->equip_index[8];
				if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == 4)
					sd->watk_ = sd->watk * 50/100;
			}
			//MATK
			if(sd->sc_data[SC_THE_MAGICIAN].timer==-1)
			{
				sd->matk1 = sd->matk1*50/100;
				sd->matk2 = sd->matk2*50/100;
			}
		}
		if(sd->sc_data[SC_THE_SUN].timer!=-1)	//THE SUN ATK�AMATK�A����A�����A�h��͂��S��20%���������� 536
		{
			//ATK
			if(sd->sc_data[SC_THE_MAGICIAN].timer==-1)
			{
				sd->base_atk = sd->base_atk * 80/100;
				sd->watk = sd->watk * 80/100;
				index = sd->equip_index[8];
				if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == 4)
					sd->watk_ = sd->watk * 80/100;
			}
			//MATK
			if(sd->sc_data[SC_THE_MAGICIAN].timer==-1)
			{
				sd->matk1 = sd->matk1*80/100;
				sd->matk2 = sd->matk2*80/100;
			}
			//
			sd->flee = sd->hit * 80/100;
			//
			sd->hit  = sd->hit * 80/100;
			//�h���
			sd->def = sd->def * 80/100;
			sd->def2 = sd->def2 * 80/100;
		}
		if(sd->sc_data[SC_DRUMBATTLE].timer!=-1){	// �푾�ۂ̋���
			sd->watk += sd->sc_data[SC_DRUMBATTLE].val2;
			sd->def  += sd->sc_data[SC_DRUMBATTLE].val3;
			index = sd->equip_index[8];
			if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == 4)
				sd->watk_ += sd->sc_data[SC_DRUMBATTLE].val2;
		}
		if(sd->sc_data[SC_NIBELUNGEN].timer!=-1) {	// �j�[�x�����O�̎w��
			index = sd->equip_index[9];
			if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->wlv >= 4)
				sd->watk += sd->sc_data[SC_NIBELUNGEN].val2;
			index = sd->equip_index[8];
			if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->wlv >= 4)
				sd->watk_ += sd->sc_data[SC_NIBELUNGEN].val2;
		}

		if(sd->sc_data[SC_VOLCANO].timer!=-1 && sd->def_ele==3){	// �{���P�[�m
			sd->watk += sd->sc_data[SC_VOLCANO].val3;
		}
		if(sd->sc_data[SC_INCATK2].timer!=-1) {
			sd->watk *= (100+sd->sc_data[SC_INCATK2].val1)/100;
		}

		if(sd->sc_data[SC_SIGNUMCRUCIS].timer!=-1)
			sd->def = sd->def * (100 - sd->sc_data[SC_SIGNUMCRUCIS].val2)/100;
		if(sd->sc_data[SC_ETERNALCHAOS].timer!=-1)	// �G�^�[�i���J�I�X
			sd->def=0;

		if(sd->sc_data[SC_CONCENTRATION].timer!=-1){ //�R���Z���g���[�V����
			sd->base_atk = sd->base_atk * (100 + 5*sd->sc_data[SC_CONCENTRATION].val1)/100;
			sd->watk = sd->watk * (100 + 5*sd->sc_data[SC_CONCENTRATION].val1)/100;
			index = sd->equip_index[8];
			if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == 4)
				sd->watk_ = sd->watk * (100 + 5*sd->sc_data[SC_CONCENTRATION].val1)/100;
			sd->def = sd->def * (100 - 5*sd->sc_data[SC_CONCENTRATION].val1)/100;
			sd->def2 = sd->def2 * (100 - 5*sd->sc_data[SC_CONCENTRATION].val1)/100;
		}

		if(sd->sc_data[SC_INCATK].timer!=-1)	//item 682�p
			sd->watk += sd->sc_data[SC_INCATK].val1;
		if(sd->sc_data[SC_INCMATK].timer!=-1){	//item 683�p
			sd->matk1 += sd->sc_data[SC_INCMATK].val1;
			sd->matk2 += sd->sc_data[SC_INCMATK].val1;
		}
		if (sd->sc_data[SC_MINDBREAKER].timer!=-1) {
			sd->matk1 += (sd->matk1*20*sd->sc_data[SC_MINDBREAKER].val1)/100;
			sd->matk2 += (sd->matk2*20*sd->sc_data[SC_MINDBREAKER].val1)/100;
			sd->mdef -= (sd->mdef*12*sd->sc_data[SC_MINDBREAKER].val1)/100;
		}
		if (sd->sc_data[SC_ENDURE].timer!=-1) {
			sd->mdef += sd->sc_data[SC_ENDURE].val1;
		}

		// ASPD/�ړ����x�ω��n
		if(sd->sc_data[SC_TWOHANDQUICKEN].timer != -1 && sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1)	// 2HQ
			aspd_rate -= 30;
		if(sd->sc_data[SC_ONEHAND].timer != -1 && sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1)	// 1HQ
			aspd_rate -= 30;

		if(sd->sc_data[SC_ADRENALINE2].timer != -1 && sd->sc_data[SC_TWOHANDQUICKEN].timer == -1 && sd->sc_data[SC_ONEHAND].timer == -1 &&
			sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1) {	// �A�h���i�������b�V��2
			if(sd->sc_data[SC_ADRENALINE2].val2 || !battle_config.party_skill_penaly)
				aspd_rate -= 30;
			else
				aspd_rate -= 25;
		}else if(sd->sc_data[SC_ADRENALINE].timer != -1 && sd->sc_data[SC_TWOHANDQUICKEN].timer == -1 && sd->sc_data[SC_ONEHAND].timer == -1 &&
			sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1) {	// �A�h���i�������b�V��
			if(sd->sc_data[SC_ADRENALINE].val2 || !battle_config.party_skill_penaly)
				aspd_rate -= 30;
			else
				aspd_rate -= 25;
		}
		if(sd->sc_data[SC_SPEARSQUICKEN].timer != -1 && sd->sc_data[SC_ADRENALINE].timer == -1 && sd->sc_data[SC_ADRENALINE2].timer == -1 &&
			sd->sc_data[SC_TWOHANDQUICKEN].timer == -1 && sd->sc_data[SC_ONEHAND].timer == -1 && 
			sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1)	// �X�s�A�N�B�b�P��
			aspd_rate -= sd->sc_data[SC_SPEARSQUICKEN].val2;
			
		if(sd->sc_data[SC_ASSNCROS_].timer!=-1 && // �[�z�̃A�T�V���N���X
			sd->sc_data[SC_TWOHANDQUICKEN].timer==-1 && sd->sc_data[SC_ONEHAND].timer==-1 &&
			sd->sc_data[SC_ADRENALINE].timer==-1 && sd->sc_data[SC_ADRENALINE2].timer==-1 &&
			sd->sc_data[SC_SPEARSQUICKEN].timer==-1 && sd->sc_data[SC_DONTFORGETME].timer == -1 && sd->status.weapon != 11)
				aspd_rate -= 5+sd->sc_data[SC_ASSNCROS].val1+sd->sc_data[SC_ASSNCROS].val2+sd->sc_data[SC_ASSNCROS].val3;
		else if(sd->sc_data[SC_ASSNCROS_].timer!=-1 && // �[�z�̃A�T�V���N���X
			sd->sc_data[SC_TWOHANDQUICKEN].timer==-1 && sd->sc_data[SC_ONEHAND].timer==-1 &&
			sd->sc_data[SC_ADRENALINE].timer==-1 && sd->sc_data[SC_ADRENALINE2].timer==-1 && 
			sd->sc_data[SC_SPEARSQUICKEN].timer==-1 && sd->sc_data[SC_DONTFORGETME].timer == -1 && sd->status.weapon != 11)
				aspd_rate -= 5+sd->sc_data[SC_ASSNCROS_].val1+sd->sc_data[SC_ASSNCROS_].val2+sd->sc_data[SC_ASSNCROS_].val3;
				
		if(sd->sc_data[SC_DONTFORGETME].timer!=-1){		// ����Y��Ȃ���
			aspd_rate += sd->sc_data[SC_DONTFORGETME].val1*3 + sd->sc_data[SC_DONTFORGETME].val2 + (sd->sc_data[SC_DONTFORGETME].val3>>16);
			sd->speed= sd->speed*(100+sd->sc_data[SC_DONTFORGETME].val1*2 + sd->sc_data[SC_DONTFORGETME].val2 + (sd->sc_data[SC_DONTFORGETME].val3&0xffff))/100;
		}else if(sd->sc_data[SC_DONTFORGETME_].timer!=-1){		// ����Y��Ȃ���
			aspd_rate += sd->sc_data[SC_DONTFORGETME_].val1*3 + sd->sc_data[SC_DONTFORGETME_].val2 + (sd->sc_data[SC_DONTFORGETME_].val3>>16);
			sd->speed= sd->speed*(100+sd->sc_data[SC_DONTFORGETME_].val1*2 + sd->sc_data[SC_DONTFORGETME_].val2 + (sd->sc_data[SC_DONTFORGETME_].val3&0xffff))/100;
		}
		
		if(sd->sc_data[SC_BERSERK].timer!=-1){
			aspd_rate -= 30;
		}
		if(sd->sc_data[SC_POISONPOTION].timer!=-1){
			aspd_rate -= 25;
		}
		if(	sd->sc_data[i=SC_SPEEDPOTION2].timer!=-1 ||
			sd->sc_data[i=SC_SPEEDPOTION1].timer!=-1 ||
			sd->sc_data[i=SC_SPEEDPOTION0].timer!=-1)	// �� ���|�[�V����
			aspd_rate -= sd->sc_data[i].val2;

		// HIT/FLEE�ω��n
		if(sd->sc_data[SC_WHISTLE].timer!=-1){  // ���J
			sd->flee += sd->flee * (sd->sc_data[SC_WHISTLE].val1
					+sd->sc_data[SC_WHISTLE].val2+(sd->sc_data[SC_WHISTLE].val3>>16))/100;
			sd->flee2+= (sd->sc_data[SC_WHISTLE].val1+sd->sc_data[SC_WHISTLE].val2+(sd->sc_data[SC_WHISTLE].val3&0xffff)) * 10;
		}else if(sd->sc_data[SC_WHISTLE_].timer!=-1){  // ���J
			sd->flee += sd->flee * (sd->sc_data[SC_WHISTLE_].val1
					+sd->sc_data[SC_WHISTLE_].val2+(sd->sc_data[SC_WHISTLE_].val3>>16))/100;
			sd->flee2+= (sd->sc_data[SC_WHISTLE_].val1+sd->sc_data[SC_WHISTLE_].val2+(sd->sc_data[SC_WHISTLE_].val3&0xffff)) * 10;
		}
		
		if(sd->sc_data[SC_HUMMING].timer!=-1){  // �n�~���O
			sd->hit += (sd->sc_data[SC_HUMMING].val1*2+sd->sc_data[SC_HUMMING].val2
					+sd->sc_data[SC_HUMMING].val3) * sd->hit/100;
		}else if(sd->sc_data[SC_HUMMING_].timer!=-1){  // �n�~���O
			sd->hit += (sd->sc_data[SC_HUMMING_].val1*2+sd->sc_data[SC_HUMMING_].val2
					+sd->sc_data[SC_HUMMING_].val3) * sd->hit/100;
		}
					
		if(sd->sc_data[SC_VIOLENTGALE].timer!=-1 && sd->def_ele==4){	// �o�C�I�����g�Q�C��
			sd->flee += sd->flee*sd->sc_data[SC_VIOLENTGALE].val3/100;
		}
		if(sd->sc_data[SC_BLIND].timer!=-1){	// �Í�
			sd->hit -= sd->hit*25/100;
			sd->flee -= sd->flee*25/100;
		}
		if(sd->sc_data[SC_WINDWALK].timer!=-1) // �E�B���h�E�H�[�N
			sd->flee += sd->flee*(sd->sc_data[SC_WINDWALK].val2)/100;
		if(sd->sc_data[SC_SPIDERWEB].timer!=-1) //�X�p�C�_�[�E�F�u
			sd->flee -= sd->flee*50/100;
		if(sd->sc_data[SC_TRUESIGHT].timer!=-1) //�g�D���[�T�C�g
			sd->hit += 3*(sd->sc_data[SC_TRUESIGHT].val1);
		if(sd->sc_data[SC_CONCENTRATION].timer!=-1) //�R���Z���g���[�V����
			sd->hit += (sd->hit*(10*(sd->sc_data[SC_CONCENTRATION].val1)))/100;
		if(sd->sc_data[SC_INCHIT].timer!=-1)
			sd->hit += sd->sc_data[SC_INCHIT].val1;
		if(sd->sc_data[SC_INCHIT2].timer!=-1)
			sd->hit *= (100+sd->sc_data[SC_INCHIT2].val1)/100;
		if(sd->sc_data[SC_BERSERK].timer!=-1)
			sd->flee -= sd->flee*50/100;
		if(sd->sc_data[SC_INCFLEE].timer!=-1) // ���x����
			sd->flee += sd->flee*(sd->sc_data[SC_INCFLEE].val2)/100;

		// �ϐ�
		if(sd->sc_data[SC_SIEGFRIED].timer!=-1){  // �s���g�̃W�[�N�t���[�h
			sd->subele[1] += sd->sc_data[SC_SIEGFRIED].val2;
			sd->subele[2] += sd->sc_data[SC_SIEGFRIED].val2;
			sd->subele[3] += sd->sc_data[SC_SIEGFRIED].val2;
			sd->subele[4] += sd->sc_data[SC_SIEGFRIED].val2;
			sd->subele[5] += sd->sc_data[SC_SIEGFRIED].val2;	// �S�Ăɑϐ�����
			sd->subele[6] += sd->sc_data[SC_SIEGFRIED].val2;
			sd->subele[7] += sd->sc_data[SC_SIEGFRIED].val2;
			sd->subele[8] += sd->sc_data[SC_SIEGFRIED].val2;
			sd->subele[9] += sd->sc_data[SC_SIEGFRIED].val2;
		}
		if(sd->sc_data[SC_PROVIDENCE].timer!=-1){	// �v�����B�f���X
			sd->subele[6] += sd->sc_data[SC_PROVIDENCE].val2;	// �� ������
			sd->subrace[6] += sd->sc_data[SC_PROVIDENCE].val2;	// �� ����
		}

		// ���̑�
		if(sd->sc_data[SC_BERSERK].timer!=-1){	// �o�[�T�[�N
			sd->def = 0;
			sd->mdef = 0;
		}
		if(sd->sc_data[SC_APPLEIDUN].timer!=-1){	// �C�h�D���̗ь�
			sd->status.max_hp += ((5+sd->sc_data[SC_APPLEIDUN].val1*2+((sd->sc_data[SC_APPLEIDUN].val2+1)>>1)
						+sd->sc_data[SC_APPLEIDUN].val3/10) * sd->status.max_hp)/100;
			if(sd->status.max_hp < 0 || sd->status.max_hp > battle_config.max_hp)
				sd->status.max_hp = battle_config.max_hp;
		}else if(sd->sc_data[SC_APPLEIDUN_].timer!=-1){	// �C�h�D���̗ь�
			sd->status.max_hp += ((5+sd->sc_data[SC_APPLEIDUN_].val1*2+((sd->sc_data[SC_APPLEIDUN_].val2+1)>>1)
						+sd->sc_data[SC_APPLEIDUN_].val3/10) * sd->status.max_hp)/100;
			if(sd->status.max_hp < 0 || sd->status.max_hp > battle_config.max_hp)
				sd->status.max_hp = battle_config.max_hp;
		}
		
		if(sd->sc_data[SC_DELUGE].timer!=-1 && sd->def_ele==1){	// �f�����[�W
			sd->status.max_hp += sd->status.max_hp*sd->sc_data[SC_DELUGE].val3/100;
			if(sd->status.max_hp < 0 || sd->status.max_hp > battle_config.max_hp)
				sd->status.max_hp = battle_config.max_hp;
		}
		if(sd->sc_data[SC_SERVICE4U].timer!=-1) {	// �T�[�r�X�t�H�[���[
			sd->status.max_sp += sd->status.max_sp*(10+sd->sc_data[SC_SERVICE4U].val1+sd->sc_data[SC_SERVICE4U].val2
						+sd->sc_data[SC_SERVICE4U].val3)/100;
			if(sd->status.max_sp < 0 || sd->status.max_sp > battle_config.max_sp)
				sd->status.max_sp = battle_config.max_sp;
			sd->dsprate-=(10+sd->sc_data[SC_SERVICE4U].val1*3+sd->sc_data[SC_SERVICE4U].val2
					+sd->sc_data[SC_SERVICE4U].val3);
			if(sd->dsprate<0)sd->dsprate=0;
		}else if(sd->sc_data[SC_SERVICE4U_].timer!=-1) {	// �T�[�r�X�t�H�[���[
			sd->status.max_sp += sd->status.max_sp*(10+sd->sc_data[SC_SERVICE4U_].val1+sd->sc_data[SC_SERVICE4U_].val2
						+sd->sc_data[SC_SERVICE4U_].val3)/100;
			if(sd->status.max_sp < 0 || sd->status.max_sp > battle_config.max_sp)
				sd->status.max_sp = battle_config.max_sp;
			sd->dsprate-=(10+sd->sc_data[SC_SERVICE4U_].val1*3+sd->sc_data[SC_SERVICE4U_].val2
					+sd->sc_data[SC_SERVICE4U_].val3);
			if(sd->dsprate<0)sd->dsprate=0;
		}

		if(sd->sc_data[SC_FORTUNE].timer!=-1){	// �K�^�̃L�X
			sd->critical += (10+sd->sc_data[SC_FORTUNE].val1+sd->sc_data[SC_FORTUNE].val2
						+sd->sc_data[SC_FORTUNE].val3)*10;
		}else if(sd->sc_data[SC_FORTUNE_].timer!=-1){	// �K�^�̃L�X
			sd->critical += (10+sd->sc_data[SC_FORTUNE_].val1+sd->sc_data[SC_FORTUNE_].val2
						+sd->sc_data[SC_FORTUNE_].val3)*10;
		}

		if(sd->sc_data[SC_EXPLOSIONSPIRITS].timer!=-1){	// �����g��
			if(s_class.job==23)
				sd->critical += sd->sc_data[SC_EXPLOSIONSPIRITS].val1*100;
			else
				sd->critical += sd->sc_data[SC_EXPLOSIONSPIRITS].val2;
		}

		if(sd->sc_data[SC_STEELBODY].timer!=-1){	// ����
			sd->def = 90;
			sd->mdef = 90;
			aspd_rate += 25;
			sd->speed = (sd->speed * 125) / 100;
		}
		if(sd->sc_data[SC_DEFENDER].timer != -1) {
			sd->aspd += (250 - sd->sc_data[SC_DEFENDER].val1*50);
			sd->speed = (sd->speed * (155 - sd->sc_data[SC_DEFENDER].val1*5)) / 100;
		}
		if(sd->sc_data[SC_ENCPOISON].timer != -1)
			sd->addeff[4] += sd->sc_data[SC_ENCPOISON].val2;

		if( sd->sc_data[SC_DANCING].timer!=-1 )		// ���t/�_���X�g�p��
			sd->speed*=4;
		if(sd->sc_data[SC_CURSE].timer!=-1){
			if(sd->sc_data[SC_DEFENDER].timer != -1)//�f�B�t�F���_�[���͎􂢂ő��x�ቺ���Ȃ�
				sd->speed += 0;
			else
				sd->speed += 450;
		}
		if(sd->sc_data[SC_TRUESIGHT].timer!=-1) //�g�D���[�T�C�g
			sd->critical += sd->critical*(sd->sc_data[SC_TRUESIGHT].val1)/100;

/*		if(sd->sc_data[SC_VOLCANO].timer!=-1)	// �G���`�����g�|�C�Y��(������battle.c��)
			sd->addeff[2]+=sd->sc_data[SC_VOLCANO].val2;//% of granting
		if(sd->sc_data[SC_DELUGE].timer!=-1)	// �G���`�����g�|�C�Y��(������battle.c��)
			sd->addeff[0]+=sd->sc_data[SC_DELUGE].val2;//% of granting
		*/
	}

	if(sd->speed_rate != 100)
		sd->speed = sd->speed*sd->speed_rate/100;
	if(sd->speed < 1) sd->speed = 1;
	if(aspd_rate != 100)
		sd->aspd = sd->aspd*aspd_rate/100;
	if(pc_isriding(sd))							// �R���C��
		sd->aspd = sd->aspd*(100 + 10*(5 - pc_checkskill(sd,KN_CAVALIERMASTERY)))/ 100;
	if(sd->aspd < battle_config.max_aspd) sd->aspd = battle_config.max_aspd;
	sd->amotion = sd->aspd;
	sd->dmotion = 800-sd->paramc[1]*4;
	if(sd->dmotion<400)
		sd->dmotion = 400;
	if(sd->skilltimer != -1 && (skill = pc_checkskill(sd,SA_FREECAST)) > 0) {
		sd->prev_speed = sd->speed;
		sd->speed = sd->speed*(175 - skill*5)/100;
	}

	if(sd->status.hp>sd->status.max_hp)
		sd->status.hp=sd->status.max_hp;
	if(sd->status.sp>sd->status.max_sp)
		sd->status.sp=sd->status.max_sp;

	if(first&4)
		return 0;
	if(first&3) {
		clif_updatestatus(sd,SP_SPEED);
		clif_updatestatus(sd,SP_MAXHP);
		clif_updatestatus(sd,SP_MAXSP);
		if(first&1) {
			clif_updatestatus(sd,SP_HP);
			clif_updatestatus(sd,SP_SP);
		}
		return 0;
	}

	if(b_class != sd->view_class) {
		clif_changelook(&sd->bl,LOOK_BASE,sd->view_class);
#if PACKETVER < 4
		clif_changelook(&sd->bl,LOOK_WEAPON,sd->status.weapon);
		clif_changelook(&sd->bl,LOOK_SHIELD,sd->status.shield);
#else
		clif_changelook(&sd->bl,LOOK_WEAPON,0);
#endif
	}

	if( memcmp(b_skill,sd->status.skill,sizeof(sd->status.skill)) || b_attackrange != sd->attackrange)
		clif_skillinfoblock(sd);	// �X�L�����M

	if(b_speed != sd->speed)
		clif_updatestatus(sd,SP_SPEED);
	if(b_weight != sd->weight)
		clif_updatestatus(sd,SP_WEIGHT);
	if(b_max_weight != sd->max_weight) {
		clif_updatestatus(sd,SP_MAXWEIGHT);
		pc_checkweighticon(sd);
	}
	for(i=0;i<6;i++)
		if(b_paramb[i] + b_parame[i] != sd->paramb[i] + sd->parame[i])
			clif_updatestatus(sd,SP_STR+i);
	if(b_hit != sd->hit)
		clif_updatestatus(sd,SP_HIT);
	if(b_flee != sd->flee)
		clif_updatestatus(sd,SP_FLEE1);
	if(b_aspd != sd->aspd)
		clif_updatestatus(sd,SP_ASPD);
	if(b_watk != sd->watk || b_base_atk != sd->base_atk)
		clif_updatestatus(sd,SP_ATK1);
	if(b_def != sd->def)
		clif_updatestatus(sd,SP_DEF1);
	if(b_watk2 != sd->watk2)
		clif_updatestatus(sd,SP_ATK2);
	if(b_def2 != sd->def2)
		clif_updatestatus(sd,SP_DEF2);
	if(b_flee2 != sd->flee2)
		clif_updatestatus(sd,SP_FLEE2);
	if(b_critical != sd->critical)
		clif_updatestatus(sd,SP_CRITICAL);
	if(b_matk1 != sd->matk1)
		clif_updatestatus(sd,SP_MATK1);
	if(b_matk2 != sd->matk2)
		clif_updatestatus(sd,SP_MATK2);
	if(b_mdef != sd->mdef)
		clif_updatestatus(sd,SP_MDEF1);
	if(b_mdef2 != sd->mdef2)
		clif_updatestatus(sd,SP_MDEF2);
	if(b_attackrange != sd->attackrange)
		clif_updatestatus(sd,SP_ATTACKRANGE);
	if(b_max_hp != sd->status.max_hp)
		clif_updatestatus(sd,SP_MAXHP);
	if(b_max_sp != sd->status.max_sp)
		clif_updatestatus(sd,SP_MAXSP);
	if(b_hp != sd->status.hp)
		clif_updatestatus(sd,SP_HP);
	if(b_sp != sd->status.sp)
		clif_updatestatus(sd,SP_SP);

/*	if(before.cart_num != before.cart_num || before.cart_max_num != before.cart_max_num ||
		before.cart_weight != before.cart_weight || before.cart_max_weight != before.cart_max_weight )
		clif_updatestatus(sd,SP_CARTINFO);*/

	if(sd->status.hp<sd->status.max_hp>>2 && pc_checkskill(sd,SM_AUTOBERSERK)>0 && sd->sc_data[SC_AUTOBERSERK].timer!=-1 && 
		(sd->sc_data[SC_PROVOKE].timer==-1 || sd->sc_data[SC_PROVOKE].val2==0 ) && !pc_isdead(sd))
		// �I�[�g�o�[�T�[�N����
		status_change_start(&sd->bl,SC_PROVOKE,10,1,0,0,0,0);

	//���z�ƌ��Ɛ��̗Z�� �K���i�\���͂��̂܂܁c�j
	if(sd->sc_data[SC_FUSION].timer != -1)
		sd->hit = 10000;
		
	return 0;
}

/*==========================================
 * �Ώۂ�group��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_group(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return mob_db[((struct mob_data *)bl)->class].group_id;
	//PC PET��0�i���ݒ�)
	
	return 0;
}
/*==========================================
 * �Ώۂ�Class��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_class(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return ((struct mob_data *)bl)->class;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->status.class;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return ((struct pet_data *)bl)->class;
	else
		return 0;
}
/*==========================================
 * �Ώۂ̕�����Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_dir(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return ((struct mob_data *)bl)->dir;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->dir;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return ((struct pet_data *)bl)->dir;
	else
		return 0;
}
/*==========================================
 * �Ώۂ̃��x����Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_lv(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return mob_db[((struct mob_data *)bl)->class].lv;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->status.base_level;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return ((struct pet_data *)bl)->msd->pet.level;
	else
		return 0;
}

/*==========================================
 * �Ώۂ̎˒���Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_range(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return mob_db[((struct mob_data *)bl)->class].range;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->attackrange;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return mob_db[((struct pet_data *)bl)->class].range;
	else
		return 0;
}
/*==========================================
 * �Ώۂ�HP��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_hp(struct block_list *bl)
{
	nullpo_retr(1, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return ((struct mob_data *)bl)->hp;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->status.hp;
	else
		return 1;
}
/*==========================================
 * �Ώۂ�SP��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_sp(struct block_list *bl)
{
	nullpo_retr(1, bl);;
	
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->status.sp;
	else
		return 0;
}
/*==========================================
 * �Ώۂ�MHP��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_max_hp(struct block_list *bl)
{
	nullpo_retr(1, bl);
	if(bl->type==BL_PC && ((struct map_session_data *)bl))
		return ((struct map_session_data *)bl)->status.max_hp;
	else {
		struct status_change *sc_data=status_get_sc_data(bl);
		int max_hp=1;
		if(bl->type==BL_MOB && ((struct mob_data*)bl)) {
			max_hp = mob_db[((struct mob_data*)bl)->class].max_hp;
			if(mob_db[((struct mob_data*)bl)->class].mexp > 0) {
				if(battle_config.mvp_hp_rate != 100) {
					double hp = (double)max_hp * battle_config.mvp_hp_rate / 100.0;
					max_hp = (hp > 0x7FFFFFFF ? 0x7FFFFFFF : (int)hp);
				}
			}
			else {
				if(battle_config.monster_hp_rate != 100)
					max_hp = (max_hp * battle_config.monster_hp_rate)/100;
			}
		}
		else if(bl->type==BL_PET && ((struct pet_data*)bl)) {
			max_hp = mob_db[((struct pet_data*)bl)->class].max_hp;
			if(mob_db[((struct pet_data*)bl)->class].mexp > 0) {
				if(battle_config.mvp_hp_rate != 100) {
					double hp = (double)max_hp * battle_config.mvp_hp_rate / 100.0;
					max_hp = (hp > 0x7FFFFFFF ? 0x7FFFFFFF : (int)hp);
				}
			}
			else {
				if(battle_config.monster_hp_rate != 100)
					max_hp = (max_hp * battle_config.monster_hp_rate)/100;
			}
		}
		if(sc_data) {
			if(sc_data[SC_APPLEIDUN].timer!=-1)
				max_hp += ((5+sc_data[SC_APPLEIDUN].val1*2+((sc_data[SC_APPLEIDUN].val2+1)>>1)
						+sc_data[SC_APPLEIDUN].val3/10) * max_hp)/100;
		}
		if(max_hp < 1) max_hp = 1;
		return max_hp;
	}
	return 1;
}
/*==========================================
 * �Ώۂ�Str��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_str(struct block_list *bl)
{
	int str=0;
	struct status_change *sc_data;
	
	nullpo_retr(0, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_MOB && ((struct mob_data *)bl))
		str = mob_db[((struct mob_data *)bl)->class].str;
	else if(bl->type==BL_PC && ((struct map_session_data *)bl))
		return ((struct map_session_data *)bl)->paramc[0];
	else if(bl->type==BL_PET && ((struct pet_data *)bl))
		str = mob_db[((struct pet_data *)bl)->class].str;

	if(sc_data) {
		if(sc_data[SC_LOUD].timer!=-1 && sc_data[SC_QUAGMIRE].timer == -1 && bl->type != BL_PC)
			str += 4;
		if( sc_data[SC_BLESSING].timer != -1 && bl->type != BL_PC){	// �u���b�V���O
			int race=status_get_race(bl);
			if(battle_check_undead(race,status_get_elem_type(bl)) || race==6 )	str >>= 1;	// �� ��/�s��
			else str += sc_data[SC_BLESSING].val1;	// ���̑�
		}
		if(sc_data[SC_TRUESIGHT].timer!=-1 && bl->type != BL_PC)	// �g�D���[�T�C�g
			str += 5;
		if(sc_data[SC_CHASEWALK_STR].timer!=-1)
			str += sc_data[SC_CHASEWALK_STR].val1;
	}
	if(str < 0) str = 0;
	return str;
}
/*==========================================
 * �Ώۂ�Agi��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */

int status_get_agi(struct block_list *bl)
{
	int agi=0;
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		agi=mob_db[((struct mob_data *)bl)->class].agi;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		agi=((struct map_session_data *)bl)->paramc[1];
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		agi=mob_db[((struct pet_data *)bl)->class].agi;

	if(sc_data) {
		if( sc_data[SC_INCREASEAGI].timer!=-1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1 &&
			bl->type != BL_PC)	// ���x����(PC��pc.c��)
			agi += 2+sc_data[SC_INCREASEAGI].val1;

		if(sc_data[SC_CONCENTRATE].timer!=-1 && sc_data[SC_QUAGMIRE].timer == -1 && bl->type != BL_PC)
			agi += agi*(2+sc_data[SC_CONCENTRATE].val1)/100;

		if(sc_data[SC_DECREASEAGI].timer!=-1)	// ���x����
			agi -= 2+sc_data[SC_DECREASEAGI].val1;

		if(sc_data[SC_QUAGMIRE].timer!=-1 )	// �N�@�O�}�C�A
			agi >>= 1;
		if(sc_data[SC_TRUESIGHT].timer!=-1 && bl->type != BL_PC)	// �g�D���[�T�C�g
			agi += 5;
	}
	if(agi < 0) agi = 0;
	return agi;
}
/*==========================================
 * �Ώۂ�Vit��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_vit(struct block_list *bl)
{
	int vit=0;
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		vit=mob_db[((struct mob_data *)bl)->class].vit;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		vit=((struct map_session_data *)bl)->paramc[2];
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		vit=mob_db[((struct pet_data *)bl)->class].vit;
	if(sc_data) {
		if(sc_data[SC_STRIPARMOR].timer != -1 && bl->type!=BL_PC)
			vit = vit*60/100;
		if(sc_data[SC_TRUESIGHT].timer!=-1 && bl->type != BL_PC)	// �g�D���[�T�C�g
			vit += 5;
	}

	if(vit < 0) vit = 0;
	return vit;
}
/*==========================================
 * �Ώۂ�Int��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_int(struct block_list *bl)
{
	int int_=0;
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		int_=mob_db[((struct mob_data *)bl)->class].int_;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		int_=((struct map_session_data *)bl)->paramc[3];
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		int_=mob_db[((struct pet_data *)bl)->class].int_;

	if(sc_data) {
		if( sc_data[SC_BLESSING].timer != -1 && bl->type != BL_PC){	// �u���b�V���O
			int race=status_get_race(bl);
			if(battle_check_undead(race,status_get_elem_type(bl)) || race==6 )	int_ >>= 1;	// �� ��/�s��
			else int_ += sc_data[SC_BLESSING].val1;	// ���̑�
		}
		if( sc_data[SC_STRIPHELM].timer != -1 && bl->type != BL_PC)
			int_ = int_*90/100;
		if(sc_data[SC_TRUESIGHT].timer!=-1 && bl->type != BL_PC)	// �g�D���[�T�C�g
			int_ += 5;
	}
	if(int_ < 0) int_ = 0;
	return int_;
}
/*==========================================
 * �Ώۂ�Dex��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_dex(struct block_list *bl)
{
	int dex=0;
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		dex=mob_db[((struct mob_data *)bl)->class].dex;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		dex=((struct map_session_data *)bl)->paramc[4];
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		dex=mob_db[((struct pet_data *)bl)->class].dex;

	if(sc_data) {
		if(sc_data[SC_CONCENTRATE].timer!=-1 && sc_data[SC_QUAGMIRE].timer == -1 && bl->type != BL_PC)
			dex += dex*(2+sc_data[SC_CONCENTRATE].val1)/100;

		if( sc_data[SC_BLESSING].timer != -1 && bl->type != BL_PC){	// �u���b�V���O
			int race=status_get_race(bl);
			if(battle_check_undead(race,status_get_elem_type(bl)) || race==6 )	dex >>= 1;	// �� ��/�s��
			else dex += sc_data[SC_BLESSING].val1;	// ���̑�
		}

		if(sc_data[SC_QUAGMIRE].timer!=-1 )	// �N�@�O�}�C�A
			dex >>= 1;
		if(sc_data[SC_TRUESIGHT].timer!=-1 && bl->type != BL_PC)	// �g�D���[�T�C�g
			dex += 5;
	}
	if(dex < 0) dex = 0;
	return dex;
}
/*==========================================
 * �Ώۂ�Luk��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_luk(struct block_list *bl)
{
	int luk=0;
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		luk=mob_db[((struct mob_data *)bl)->class].luk;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		luk=((struct map_session_data *)bl)->paramc[5];
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		luk=mob_db[((struct pet_data *)bl)->class].luk;

	if(sc_data) {
		if(sc_data[SC_GLORIA].timer!=-1 && bl->type != BL_PC)	// �O�����A(PC��pc.c��)
			luk += 30;
		if(sc_data[SC_CURSE].timer!=-1 )		// ��
			luk=0;
		if(sc_data[SC_TRUESIGHT].timer!=-1 && bl->type != BL_PC)	// �g�D���[�T�C�g
			luk += 5;
	}
	if(luk < 0) luk = 0;
	return luk;
}

/*==========================================
 * �Ώۂ�Flee��Ԃ�(�ėp)
 * �߂�͐�����1�ȏ�
 *------------------------------------------
 */
int status_get_flee(struct block_list *bl)
{
	int flee=1;
	struct status_change *sc_data;

	nullpo_retr(1, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		flee=((struct map_session_data *)bl)->flee;
	else
		flee=status_get_agi(bl) + status_get_lv(bl);

	if(sc_data) {
		if(sc_data[SC_WHISTLE].timer!=-1 && bl->type != BL_PC)
			flee += flee*(sc_data[SC_WHISTLE].val1+sc_data[SC_WHISTLE].val2
					+(sc_data[SC_WHISTLE].val3>>16))/100;
		if(sc_data[SC_BLIND].timer!=-1 && bl->type != BL_PC)
			flee -= flee*25/100;
		if(sc_data[SC_WINDWALK].timer!=-1 && bl->type != BL_PC) // �E�B���h�E�H�[�N
			flee += flee*(sc_data[SC_WINDWALK].val2)/100;
		if(sc_data[SC_SPIDERWEB].timer!=-1 && bl->type != BL_PC) //�X�p�C�_�[�E�F�u
			flee -= flee*50/100;
		if(sc_data[SC_INCFLEE].timer!=-1 && bl->type != BL_PC) //���x����
			flee += flee*(50*sc_data[SC_INCFLEE].val1);
			//THE SUN
		if(sc_data[SC_THE_SUN].timer!=-1 && bl->type != BL_PC)
			flee = flee*80/100;
	}
	if(flee < 1) flee = 1;
	return flee;
}
/*==========================================
 * �Ώۂ�Hit��Ԃ�(�ėp)
 * �߂�͐�����1�ȏ�
 *------------------------------------------
 */
int status_get_hit(struct block_list *bl)
{
	int hit=1;
	struct status_change *sc_data;

	nullpo_retr(1, bl);
	if (bl->type==BL_PC)
		return ((struct map_session_data *)bl)->hit;
	else
		hit=status_get_dex(bl) + status_get_lv(bl);

	sc_data = status_get_sc_data(bl);
	if (sc_data) {
		if(sc_data[SC_HUMMING].timer!=-1)	// 
			hit += hit*(sc_data[SC_HUMMING].val1*2+sc_data[SC_HUMMING].val2
					+sc_data[SC_HUMMING].val3)/100;
		if(sc_data[SC_BLIND].timer!=-1)	// �È�
			hit -= hit*25/100;
		if(sc_data[SC_TRUESIGHT].timer!=-1)		// �g�D���[�T�C�g
			hit += 3*(sc_data[SC_TRUESIGHT].val1);
		if(sc_data[SC_CONCENTRATION].timer!=-1) //�R���Z���g���[�V����
			hit += (hit*(10*(sc_data[SC_CONCENTRATION].val1)))/100;
		if(sc_data[SC_EXPLOSIONSPIRITS].timer!=-1 && bl->type != BL_PC)
			hit += (20*sc_data[SC_EXPLOSIONSPIRITS].val1); //�m�o�b�����g��
			//THE SUN
		if(sc_data[SC_THE_SUN].timer!=-1 && bl->type != BL_PC)
			hit = hit*80/100;
	}
	if(hit < 1) hit = 1;
	return hit;
}
/*==========================================
 * �Ώۂ̊��S�����Ԃ�(�ėp)
 * �߂�͐�����1�ȏ�
 *------------------------------------------
 */
int status_get_flee2(struct block_list *bl)
{
	int flee2=1;
	struct status_change *sc_data;

	nullpo_retr(1, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl){
		flee2 = status_get_luk(bl) + 10;
		flee2 += ((struct map_session_data *)bl)->flee2 - (((struct map_session_data *)bl)->paramc[5] + 10);
	}
	else
		flee2=status_get_luk(bl)+1;

	if(sc_data) {
		if(sc_data[SC_WHISTLE].timer!=-1 && bl->type != BL_PC)
			flee2 += (sc_data[SC_WHISTLE].val1+sc_data[SC_WHISTLE].val2
					+(sc_data[SC_WHISTLE].val3&0xffff))*10;
	}
	if(flee2 < 1) flee2 = 1;
	return flee2;
}
/*==========================================
 * �Ώۂ̃N���e�B�J����Ԃ�(�ėp)
 * �߂�͐�����1�ȏ�
 *------------------------------------------
 */
int status_get_critical(struct block_list *bl)
{
	int critical=1;
	struct status_change *sc_data;

	nullpo_retr(1, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl){
		critical = status_get_luk(bl)*3 + 10;
		critical += ((struct map_session_data *)bl)->critical - ((((struct map_session_data *)bl)->paramc[5]*3) + 10);
	}
	else
		critical=status_get_luk(bl)*3 + 1;

	if(sc_data) {
		if(sc_data[SC_FORTUNE].timer!=-1 && bl->type != BL_PC)
			critical += (10+sc_data[SC_FORTUNE].val1+sc_data[SC_FORTUNE].val2
					+sc_data[SC_FORTUNE].val3)*10;
		if(sc_data[SC_EXPLOSIONSPIRITS].timer!=-1 && bl->type != BL_PC)
			critical += sc_data[SC_EXPLOSIONSPIRITS].val2;
		if(sc_data[SC_TRUESIGHT].timer!=-1 && bl->type != BL_PC) //�g�D���[�T�C�g
			critical += critical*sc_data[SC_TRUESIGHT].val1/100;
	}
	if(critical < 1) critical = 1;
	return critical;
}
/*==========================================
 * base_atk�̎擾
 * �߂�͐�����1�ȏ�
 *------------------------------------------
 */
int status_get_baseatk(struct block_list *bl)
{
	struct status_change *sc_data;
	int batk=1;

	nullpo_retr(1, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl){
		batk = ((struct map_session_data *)bl)->base_atk; //�ݒ肳��Ă���base_atk
		batk += ((struct map_session_data *)bl)->weapon_atk[((struct map_session_data *)bl)->status.weapon];
	}else { //����ȊO�Ȃ�
		int str,dstr;
		str = status_get_str(bl); //STR
		dstr = str/10;
		batk = dstr*dstr + str; //base_atk���v�Z����
	}
	if(sc_data) { //��Ԉُ킠��
		if(sc_data[SC_PROVOKE].timer!=-1 && bl->type != BL_PC) //PC�Ńv���{�b�N(SM_PROVOKE)���
			batk = batk*(100+2*sc_data[SC_PROVOKE].val1)/100; //base_atk����
		if(sc_data[SC_CURSE].timer!=-1 ) //����Ă�����
			batk -= batk*25/100; //base_atk��25%����
		if(sc_data[SC_CONCENTRATION].timer!=-1 && bl->type != BL_PC) //�R���Z���g���[�V����
			batk += batk*(5*sc_data[SC_CONCENTRATION].val1)/100;
		//�^���b�g���ʂ͂P�����H
		if(bl->type != BL_PC && (sc_data[SC_THE_MAGICIAN].timer!=-1 || sc_data[SC_THE_DEVIL].timer!=-1))
				batk = batk*50/100;
		else if(sc_data[SC_THE_SUN].timer!=-1 && bl->type != BL_PC)//THE SUN
				batk = batk*80/100;
				
		//�G�X�N
		if(sc_data[SC_SKE].timer!=-1 && bl->type == BL_MOB)
			batk *= 4;
	}
	if(batk < 1) batk = 1; //base_atk�͍Œ�ł�1
	return batk;
}
/*==========================================
 * �Ώۂ�Atk��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_atk(struct block_list *bl)
{
	struct status_change *sc_data;
	int atk=0;

	nullpo_retr(0, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		atk = ((struct map_session_data*)bl)->watk;
	else if(bl->type==BL_MOB && (struct mob_data *)bl)
	{
		int guardup_lv = ((struct mob_data*)bl)->guardup_lv;
		if(guardup_lv>0)
		{
			atk = mob_db[((struct mob_data*)bl)->class].atk1;
			atk += atk*20*guardup_lv/100;
		}else 
			atk = mob_db[((struct mob_data*)bl)->class].atk1;
			
	}else if(bl->type==BL_PET && (struct pet_data *)bl)
		atk = mob_db[((struct pet_data*)bl)->class].atk1;

	if(sc_data) {
		if(sc_data[SC_PROVOKE].timer!=-1 && bl->type != BL_PC)
			atk = atk*(100+2*sc_data[SC_PROVOKE].val1)/100;
		if(sc_data[SC_CURSE].timer!=-1 )
			atk -= atk*25/100;
		if(sc_data[SC_CONCENTRATION].timer!=-1 && bl->type != BL_PC) //�R���Z���g���[�V����
			atk += atk*(5*sc_data[SC_CONCENTRATION].val1)/100;
		if(sc_data[SC_EXPLOSIONSPIRITS].timer!=-1 && bl->type != BL_PC)
			atk += (1000*sc_data[SC_EXPLOSIONSPIRITS].val1); //�m�o�b�����g��
		if(sc_data[SC_STRIPWEAPON].timer!=-1 && bl->type != BL_PC)
			atk -= atk*10/100;
		//�^���b�g���ʂ͂P�����H
		if(bl->type != BL_PC && (sc_data[SC_THE_MAGICIAN].timer!=-1 || sc_data[SC_THE_DEVIL].timer!=-1))
		{
				atk = atk*50/100;
		}else if(sc_data[SC_THE_SUN].timer!=-1 && bl->type != BL_PC)//THE SUN
				atk = atk*80/100;
			
		//�G�X�N
		if(sc_data[SC_SKE].timer!=-1 && bl->type == BL_MOB)
			atk *=4;
	}
	if(atk < 0) atk = 0;
	return atk;
}
/*==========================================
 * �Ώۂ̍���Atk��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_atk_(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl){
		int atk=((struct map_session_data*)bl)->watk_;

		if(((struct map_session_data *)bl)->sc_data[SC_CURSE].timer!=-1 )
			atk -= atk*25/100;
		return atk;
	}
	else
		return 0;
}
/*==========================================
 * �Ώۂ�Atk2��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_atk2(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data*)bl)->watk2;
	else {
		struct status_change *sc_data=status_get_sc_data(bl);
		int atk2=0;
		if(bl->type==BL_MOB && (struct mob_data *)bl){
		
			int guardup_lv = ((struct mob_data*)bl)->guardup_lv;
			if(guardup_lv>0)
			{
				atk2 = mob_db[((struct mob_data*)bl)->class].atk2;
				atk2 += atk2*20*guardup_lv/100;
			}else 
				atk2 = mob_db[((struct mob_data*)bl)->class].atk2;
				
		
		}
		else if(bl->type==BL_PET && (struct pet_data *)bl)
			atk2 = mob_db[((struct pet_data*)bl)->class].atk2;
		if(sc_data) {
			if( sc_data[SC_IMPOSITIO].timer!=-1)
				atk2 += sc_data[SC_IMPOSITIO].val1*5;
			if( sc_data[SC_PROVOKE].timer!=-1 )
				atk2 = atk2*(100+2*sc_data[SC_PROVOKE].val1)/100;
			if( sc_data[SC_CURSE].timer!=-1 )
				atk2 -= atk2*25/100;
			if(sc_data[SC_DRUMBATTLE].timer!=-1)
				atk2 += sc_data[SC_DRUMBATTLE].val2;
			if(sc_data[SC_NIBELUNGEN].timer!=-1 && (status_get_element(bl)/10) >= 8 )
				atk2 += sc_data[SC_NIBELUNGEN].val2;
			if(sc_data[SC_STRIPWEAPON].timer!=-1)
				atk2 -= atk2*10/100;
			if(sc_data[SC_CONCENTRATION].timer!=-1) //�R���Z���g���[�V����
				atk2 += atk2*(5*sc_data[SC_CONCENTRATION].val1)/100;
			if(sc_data[SC_EXPLOSIONSPIRITS].timer!=-1 && bl->type != BL_PC)
				atk2 += (1000*sc_data[SC_EXPLOSIONSPIRITS].val1); //�m�o�b�����g��
				
			//�^���b�g���ʂ͂P�����H
			if(sc_data[SC_THE_MAGICIAN].timer!=-1 || sc_data[SC_THE_DEVIL].timer!=-1)
					atk2 = atk2*50/100;
			else if(sc_data[SC_THE_SUN].timer!=-1)//THE SUN
					atk2 = atk2*80/100;
					
			//�G�X�N
			if(sc_data[SC_SKE].timer!=-1 && bl->type == BL_MOB)
				atk2 *= 4;
				
		}
		if(atk2 < 0) atk2 = 0;
		return atk2;
	}
	return 0;
}
/*==========================================
 * �Ώۂ̍���Atk2��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_atk_2(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data*)bl)->watk_2;
	else
		return 0;
}
/*==========================================
 * �Ώۂ�MAtk1��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_matk1(struct block_list *bl)
{
	struct status_change *sc_data;
	int matk1,int_;
	nullpo_retr(0, bl);

	if (bl->type==BL_PC)
		return ((struct map_session_data *)bl)->matk1;
	else if (bl->type!=BL_PET && bl->type!=BL_MOB)
		return 0;

	int_=status_get_int(bl);
	matk1 = int_+(int_/5)*(int_/5);
	sc_data = status_get_sc_data(bl);
	if (sc_data) {
		if (sc_data[SC_MINDBREAKER].timer!=-1)
			matk1 += (matk1*20*sc_data[SC_MINDBREAKER].val1)/100;
		//�^���b�g���ʂ͂P�����H
		if(sc_data[SC_STRENGTH].timer!=-1 || sc_data[SC_THE_DEVIL].timer!=-1)
			matk1 = matk1*50/100;
		else if(sc_data[SC_THE_SUN].timer!=-1)//THE SUN
			matk1 = matk1*80/100;
	}
	return matk1;
}
/*==========================================
 * �Ώۂ�MAtk2��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_matk2(struct block_list *bl)
{
	struct status_change *sc_data;
	int matk2,int_;
	nullpo_retr(0, bl);

	if (bl->type==BL_PC)
		return ((struct map_session_data *)bl)->matk2;
	else if (bl->type!=BL_PET && bl->type!=BL_MOB)
		return 0;

	int_=status_get_int(bl);
	matk2 = int_+(int_/7)*(int_/7);
	sc_data = status_get_sc_data(bl);
	if (sc_data) {
		if (sc_data[SC_MINDBREAKER].timer!=-1)
			matk2 += (matk2*20*sc_data[SC_MINDBREAKER].val1)/100;
			
		//�^���b�g���ʂ͂P�����H
		if(sc_data[SC_STRENGTH].timer!=-1 || sc_data[SC_THE_DEVIL].timer!=-1)
			matk2 = matk2*50/100;
		else if(sc_data[SC_THE_SUN].timer!=-1)//THE SUN
			matk2 = matk2*80/100;
	}
	return matk2;
}
/*==========================================
 * �Ώۂ�Def��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_def(struct block_list *bl)
{
	struct status_change *sc_data;
	int def=0,skilltimer=-1,skillid=0;

	nullpo_retr(0, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl){
		def = ((struct map_session_data *)bl)->def;
		skilltimer = ((struct map_session_data *)bl)->skilltimer;
		skillid = ((struct map_session_data *)bl)->skillid;
	}
	else if(bl->type==BL_MOB && (struct mob_data *)bl) {
		def = mob_db[((struct mob_data *)bl)->class].def;
		skilltimer = ((struct mob_data *)bl)->skilltimer;
		skillid = ((struct mob_data *)bl)->skillid;
	}
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		def = mob_db[((struct pet_data *)bl)->class].def;

	if(def < 1000000) {
		if(sc_data) {
			//�L�[�s���O����DEF100
			if( sc_data[SC_KEEPING].timer!=-1)
				def = 100;
			//�v���{�b�N���͌��Z
			if( sc_data[SC_PROVOKE].timer!=-1 && bl->type != BL_PC)
				def = (def*(100 - 6*sc_data[SC_PROVOKE].val1)+50)/100;
			//�푾�ۂ̋������͉��Z
			if( sc_data[SC_DRUMBATTLE].timer!=-1 && bl->type != BL_PC)
				def += sc_data[SC_DRUMBATTLE].val3;
			//�łɂ������Ă��鎞�͌��Z
			if(sc_data[SC_POISON].timer!=-1 && bl->type != BL_PC)
				def = def*75/100;
			//�X�g���b�v�V�[���h���͌��Z
			if(sc_data[SC_STRIPSHIELD].timer!=-1 && bl->type != BL_PC)
				def = def*85/100;
			//�V�O�i���N���V�X���͌��Z
			if(sc_data[SC_SIGNUMCRUCIS].timer!=-1 && bl->type != BL_PC)
				def = def * (100 - sc_data[SC_SIGNUMCRUCIS].val2)/100;
			//�i���̍��׎���DEF0�ɂȂ�
			if(sc_data[SC_ETERNALCHAOS].timer!=-1 && bl->type != BL_PC)
				def = 0;
			//�����A�Ή����͉E�V�t�g
			if(sc_data[SC_FREEZE].timer != -1 || (sc_data[SC_STONE].timer != -1 && sc_data[SC_STONE].val2 == 0))
				def >>= 1;
			//�R���Z���g���[�V�������͌��Z
			if( sc_data[SC_CONCENTRATION].timer!=-1 && bl->type != BL_PC)
				def = (def*(100 - 5*sc_data[SC_CONCENTRATION].val1))/100;
				
			//THE SUN
			if(sc_data[SC_THE_SUN].timer!=-1 && bl->type != BL_PC)
				def = def*80/100;
			//�G�X�N
			if(sc_data[SC_SKE].timer!=-1 && bl->type == BL_MOB)
				def = def/2;
			//�G�X�J
			if(sc_data[SC_SKA].timer!=-1 && bl->type == BL_MOB)
				def = 90;
		}
		//�r�����͉r�������Z���Ɋ�Â��Č��Z
		if(skilltimer != -1) {
			int def_rate = skill_get_castdef(skillid);
			if(def_rate != 0)
				def = (def * (100 - def_rate))/100;
		}
	}
	if(def < 0) def = 0;
	return def;
}
/*==========================================
 * �Ώۂ�MDef��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_mdef(struct block_list *bl)
{
	struct status_change *sc_data;
	int mdef=0;

	nullpo_retr(0, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		mdef = ((struct map_session_data *)bl)->mdef;
	else if(bl->type==BL_MOB && (struct mob_data *)bl)
		mdef = mob_db[((struct mob_data *)bl)->class].mdef;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		mdef = mob_db[((struct pet_data *)bl)->class].mdef;

	if(mdef < 1000000) {
		if(sc_data) {
			//�G�X�J
			if(sc_data[SC_SKA].timer!=-1 && bl->type == BL_MOB)
				mdef = 90;
			//�o���A�[��Ԏ���MDEF100
			if(sc_data[SC_BARRIER].timer != -1)
				mdef = 100;
			//�����A�Ή�����1.25�{
			if(sc_data[SC_FREEZE].timer != -1 || (sc_data[SC_STONE].timer != -1 && sc_data[SC_STONE].val2 == 0))
				mdef = mdef*125/100;
			if (bl->type!=BL_PC && sc_data[SC_MINDBREAKER].timer!=-1)
				mdef -= (mdef*12*sc_data[SC_MINDBREAKER].val1)/100;
		}
	}
	if(mdef < 0) mdef = 0;
	return mdef;
}
/*==========================================
 * �Ώۂ�Def2��Ԃ�(�ėp)
 * �߂�͐�����1�ȏ�
 *------------------------------------------
 */
int status_get_def2(struct block_list *bl)
{
	struct status_change *sc_data;
	int def2=1;

	nullpo_retr(1, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_PC)
		def2 = ((struct map_session_data *)bl)->def2;
	else if(bl->type==BL_MOB)
		def2 = mob_db[((struct mob_data *)bl)->class].vit;
	else if(bl->type==BL_PET)
		def2 = mob_db[((struct pet_data *)bl)->class].vit;

	if(sc_data) {
		if( sc_data[SC_ANGELUS].timer!=-1 && bl->type != BL_PC)
			def2 = def2*(110+5*sc_data[SC_ANGELUS].val1)/100;
		if( sc_data[SC_PROVOKE].timer!=-1 && bl->type != BL_PC)
			def2 = (def2*(100 - 6*sc_data[SC_PROVOKE].val1)+50)/100;
		if(sc_data[SC_POISON].timer!=-1 && bl->type != BL_PC)
			def2 = def2*75/100;
		//�R���Z���g���[�V�������͌��Z
		if( sc_data[SC_CONCENTRATION].timer!=-1 && bl->type != BL_PC)
			def2 = def2*(100 - 5*sc_data[SC_CONCENTRATION].val1)/100;
		//THE SUN
		if(sc_data[SC_THE_SUN].timer!=-1 && bl->type != BL_PC)
			def2 = def2*80/100;
			
	}
	if(def2 < 1) def2 = 1;
	return def2;
}
/*==========================================
 * �Ώۂ�MDef2��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_mdef2(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return mob_db[((struct mob_data *)bl)->class].int_ + (mob_db[((struct mob_data *)bl)->class].vit>>1);
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->mdef2 + (((struct map_session_data *)bl)->paramc[2]>>1);
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return mob_db[((struct pet_data *)bl)->class].int_ + (mob_db[((struct pet_data *)bl)->class].vit>>1);
	else
		return 0;
}
/*==========================================
 * �Ώۂ�Speed(�ړ����x)��Ԃ�(�ėp)
 * �߂�͐�����1�ȏ�
 * Speed�͏������ق����ړ����x������
 *------------------------------------------
 */
int status_get_speed(struct block_list *bl)
{
	nullpo_retr(1000, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->speed;
	else {
		struct status_change *sc_data=status_get_sc_data(bl);
		int speed = 1000;
		if(bl->type==BL_MOB && (struct mob_data *)bl)
//			speed = mob_db[((struct mob_data *)bl)->class].speed;
			speed = ((struct mob_data *)bl)->speed;
		else if(bl->type==BL_PET && (struct pet_data *)bl)
			speed = ((struct pet_data *)bl)->msd->petDB->speed;

		if(sc_data) {
			//���x��������25%���Z
			//�E�B���h�E�H�[�N����Lv*2%���Z(���x�������͖���)
			if(sc_data[SC_INCREASEAGI].timer!=-1)
				speed -= speed*25/100;
			else if(sc_data[SC_WINDWALK].timer!=-1)
				speed -= (speed*(sc_data[SC_WINDWALK].val1*2))/100;
			//���x��������25%���Z
			if(sc_data[SC_INCFLEE].timer!=-1)
				speed -= speed*25/100;
			//���x��������25%���Z
			if(sc_data[SC_DECREASEAGI].timer!=-1)
				speed = speed*125/100;
			//�N�@�O�}�C�A����1/3���Z
			if(sc_data[SC_QUAGMIRE].timer!=-1)
				speed = speed*4/3;
			//����Y��Ȃ��Łc���͉��Z
			if(sc_data[SC_DONTFORGETME].timer!=-1)
				speed = speed*(100+sc_data[SC_DONTFORGETME].val1*2 + sc_data[SC_DONTFORGETME].val2 + (sc_data[SC_DONTFORGETME].val3&0xffff))/100;
			//��������25%���Z
			if(sc_data[SC_STEELBODY].timer!=-1)
				speed = speed*125/100;
			//�f�B�t�F���_�[���͉��Z
			if(sc_data[SC_DEFENDER].timer!=-1)
				speed = (speed * (155 - sc_data[SC_DEFENDER].val1*5)) / 100;
			//�x���Ԃ�4�{�x��
			if(sc_data[SC_DANCING].timer!=-1 )
				speed*=4;
			//�G�X�E��4�{�x��
			if(sc_data[SC_SWOO].timer!=-1 )
				speed*=4;
			//�􂢎���450���Z�i�f�B�t�F���_�[���͑��x�ቺ�����j
			if(sc_data[SC_CURSE].timer!=-1){
				if(sc_data[SC_DEFENDER].timer != -1)
					speed += 0;
				else
					speed += 450;
			}
//			if(sc_data[SC_CURSE].timer!=-1)
//				speed = speed + 450;
		}
		if(speed < 1) speed = 1;
		return speed;
	}

	return 1000;
}
/*==========================================
 * �Ώۂ�aDelay(�U�����f�B���C)��Ԃ�(�ėp)
 * aDelay�͏������ق����U�����x������
 *------------------------------------------
 */
int status_get_adelay(struct block_list *bl)
{
	nullpo_retr(4000, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return (((struct map_session_data *)bl)->aspd<<1);
	else {
		struct status_change *sc_data=status_get_sc_data(bl);
		int adelay=4000,aspd_rate = 100,i;
		
		if(bl->type==BL_MOB && (struct mob_data *)bl)
		{
				int guardup_lv = ((struct mob_data*)bl)->guardup_lv;
				adelay = mob_db[((struct mob_data *)bl)->class].adelay;
		
				if(guardup_lv>0)
					aspd_rate -= 5 + 5*guardup_lv;
		}else if(bl->type==BL_PET && (struct pet_data *)bl)
			adelay = mob_db[((struct pet_data *)bl)->class].adelay;

			
		if(sc_data) {
			//�c�[�n���h�N�C�b�P���g�p���ŃN�@�O�}�C�A�ł�����Y��Ȃ��Łc�ł��Ȃ�����3�����Z
			if(sc_data[SC_TWOHANDQUICKEN].timer != -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// 2HQ
				aspd_rate -= 30;
			//�����n���h�N�C�b�P���g�p���ŃN�@�O�}�C�A�ł�����Y��Ȃ��Łc�ł��Ȃ�����3�����Z
			if(sc_data[SC_ONEHAND].timer != -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// 1HQ
				aspd_rate -= 30;
			//�A�h���i�������b�V���g�p���Ńc�[�n���h�N�C�b�P���ł��N�@�O�}�C�A�ł�����Y��Ȃ��Łc�ł��Ȃ�����
			if(sc_data[SC_ADRENALINE2].timer != -1 && sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_ONEHAND].timer == -1 &&
				sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1) {	// �A�h���i�������b�V��
				//�g�p�҂ƃp�[�e�B�����o�[�Ŋi�����o��ݒ�łȂ����3�����Z
				if(sc_data[SC_ADRENALINE2].val2 || !battle_config.party_skill_penaly)
					aspd_rate -= 30;
				//�����łȂ����2.5�����Z
				else
					aspd_rate -= 25;
			}else if(sc_data[SC_ADRENALINE].timer != -1 && sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_ONEHAND].timer == -1 &&
				sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1) {	// �A�h���i�������b�V��
				//�g�p�҂ƃp�[�e�B�����o�[�Ŋi�����o��ݒ�łȂ����3�����Z
				if(sc_data[SC_ADRENALINE].val2 || !battle_config.party_skill_penaly)
					aspd_rate -= 30;
				//�����łȂ����2.5�����Z
				else
					aspd_rate -= 25;
			}
			//�X�s�A�N�B�b�P�����͌��Z
			if(sc_data[SC_SPEARSQUICKEN].timer != -1 && sc_data[SC_ADRENALINE].timer == -1 && sc_data[SC_ADRENALINE2].timer == -1 &&
				sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_ONEHAND].timer == -1 && 
				sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// �X�s�A�N�B�b�P��
				aspd_rate -= sc_data[SC_SPEARSQUICKEN].val2;
			//�[���̃A�T�V���N���X���͌��Z
			if(sc_data[SC_ASSNCROS].timer!=-1 && // �[�z�̃A�T�V���N���X
				sc_data[SC_TWOHANDQUICKEN].timer==-1 && sc_data[SC_ONEHAND].timer == -1 && 
				sc_data[SC_ADRENALINE].timer==-1 && sc_data[SC_ADRENALINE2].timer==-1 && sc_data[SC_SPEARSQUICKEN].timer==-1 &&
				sc_data[SC_DONTFORGETME].timer == -1)
				aspd_rate -= 5+sc_data[SC_ASSNCROS].val1+sc_data[SC_ASSNCROS].val2+sc_data[SC_ASSNCROS].val3;
			//����Y��Ȃ��Łc���͉��Z
			if(sc_data[SC_DONTFORGETME].timer!=-1)		// ����Y��Ȃ���
				aspd_rate += sc_data[SC_DONTFORGETME].val1*3 + sc_data[SC_DONTFORGETME].val2 + (sc_data[SC_DONTFORGETME].val3>>16);
			//������25%���Z
			if(sc_data[SC_STEELBODY].timer!=-1)	// ����
				aspd_rate += 25;
			//�Ŗ�̕r�g�p���͌��Z
			if(	sc_data[SC_POISONPOTION].timer!=-1)
				aspd_rate -= 25;
			//�����|�[�V�����g�p���͌��Z
			if(	sc_data[i=SC_SPEEDPOTION2].timer!=-1 || sc_data[i=SC_SPEEDPOTION1].timer!=-1 || sc_data[i=SC_SPEEDPOTION0].timer!=-1)
				aspd_rate -= sc_data[i].val2;
			//�f�B�t�F���_�[���͉��Z
			if(sc_data[SC_DEFENDER].timer != -1)
				adelay += (1100 - sc_data[SC_DEFENDER].val1*100);
		}
		if(aspd_rate != 100)
			adelay = adelay*aspd_rate/100;
		if(adelay < battle_config.monster_max_aspd<<1) adelay = battle_config.monster_max_aspd<<1;
		return adelay;
	}
	return 4000;
}
int status_get_amotion(struct block_list *bl)
{
	nullpo_retr(2000, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->amotion;
	else {
		struct status_change *sc_data=status_get_sc_data(bl);
		int amotion=2000,aspd_rate = 100,i;
		
		if(bl->type==BL_MOB && (struct mob_data *)bl)
		{
			int guardup_lv = ((struct mob_data*)bl)->guardup_lv;
			amotion = mob_db[((struct mob_data *)bl)->class].amotion;
			if(guardup_lv>0)
				aspd_rate -= 5 + 5*guardup_lv;
		}else if(bl->type==BL_PET && (struct pet_data *)bl)
			amotion = mob_db[((struct pet_data *)bl)->class].amotion;

			
		if(sc_data) {
			if(sc_data[SC_TWOHANDQUICKEN].timer != -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// 2HQ
				aspd_rate -= 30;
			if(sc_data[SC_ONEHAND].timer != -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// 1HQ
				aspd_rate -= 30;
			
			if(sc_data[SC_ADRENALINE2].timer != -1 && sc_data[SC_TWOHANDQUICKEN].timer == -1 &&
				sc_data[SC_ONEHAND].timer == -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1) {	// �t���A�h���i�������b�V
				if(sc_data[SC_ADRENALINE2].val2 || !battle_config.party_skill_penaly)
					aspd_rate -= 30;
				else
					aspd_rate -= 25;
			}else if(sc_data[SC_ADRENALINE].timer != -1 && sc_data[SC_TWOHANDQUICKEN].timer == -1 &&
				sc_data[SC_ONEHAND].timer == -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1) {	// �A�h���i�������b�V��
				if(sc_data[SC_ADRENALINE].val2 || !battle_config.party_skill_penaly)
					aspd_rate -= 30;
				else
					aspd_rate -= 25;
			}
			if(sc_data[SC_SPEARSQUICKEN].timer != -1 && sc_data[SC_ADRENALINE].timer == -1 && sc_data[SC_ADRENALINE2].timer == -1 &&
				sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_ONEHAND].timer==-1 &&
				sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// �X�s�A�N�B�b�P��
				aspd_rate -= sc_data[SC_SPEARSQUICKEN].val2;
			if(sc_data[SC_ASSNCROS].timer!=-1 && // �[�z�̃A�T�V���N���X
				sc_data[SC_TWOHANDQUICKEN].timer==-1 && sc_data[SC_ONEHAND].timer==-1 &&
				sc_data[SC_ADRENALINE].timer==-1 && sc_data[SC_ADRENALINE2].timer==-1 && sc_data[SC_SPEARSQUICKEN].timer==-1 &&
				sc_data[SC_DONTFORGETME].timer == -1)
				aspd_rate -= 5+sc_data[SC_ASSNCROS].val1+sc_data[SC_ASSNCROS].val2+sc_data[SC_ASSNCROS].val3;
			if(sc_data[SC_DONTFORGETME].timer!=-1)		// ����Y��Ȃ���
				aspd_rate += sc_data[SC_DONTFORGETME].val1*3 + sc_data[SC_DONTFORGETME].val2 + (sc_data[SC_DONTFORGETME].val3>>16);
			if(	sc_data[SC_POISONPOTION].timer!=-1)
				aspd_rate -= 25;
			if(sc_data[SC_STEELBODY].timer!=-1)	// ����
				aspd_rate += 25;
			if(	sc_data[i=SC_SPEEDPOTION2].timer!=-1 || sc_data[i=SC_SPEEDPOTION1].timer!=-1 || sc_data[i=SC_SPEEDPOTION0].timer!=-1)
				aspd_rate -= sc_data[i].val2;
			if(sc_data[SC_DEFENDER].timer != -1)
				amotion += (550 - sc_data[SC_DEFENDER].val1*50);
		}
		if(aspd_rate != 100)
			amotion = amotion*aspd_rate/100;
		if(amotion < battle_config.monster_max_aspd) amotion = battle_config.monster_max_aspd;
		return amotion;
	}
	return 2000;
}
int status_get_dmotion(struct block_list *bl)
{
	int ret;
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	sc_data = status_get_sc_data(bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl){
		ret=mob_db[((struct mob_data *)bl)->class].dmotion;
		if(battle_config.monster_damage_delay_rate != 100)
			ret = ret*battle_config.monster_damage_delay_rate/100;
	}
	else if(bl->type==BL_PC && (struct map_session_data *)bl){
		ret=((struct map_session_data *)bl)->dmotion;
		if(battle_config.pc_damage_delay_rate != 100)
			ret = ret*battle_config.pc_damage_delay_rate/100;
	}
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		ret=mob_db[((struct pet_data *)bl)->class].dmotion;
	else
		return 2000;

	if((sc_data && sc_data[SC_ENDURE].timer!=-1 &&
		(bl->type == BL_PC && !map[((struct map_session_data *)bl)->bl.m].flag.gvg)) ||
		(bl->type == BL_PC && ((struct map_session_data *)bl)->special_state.infinite_endure))
		ret=0;

	return ret;
}
int status_get_element(struct block_list *bl)
{
	int ret = 20;
	struct status_change *sc_data;

	nullpo_retr(ret, bl);
	sc_data = status_get_sc_data(bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)	// 10�̈ʁ�Lv*2�A�P�̈ʁ�����
		ret=((struct mob_data *)bl)->def_ele;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		ret=20+((struct map_session_data *)bl)->def_ele;	// �h�䑮��Lv1
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		ret = mob_db[((struct pet_data *)bl)->class].element;

	if(sc_data) {
		if( sc_data[SC_BENEDICTIO].timer!=-1 )	// ���̍~��
			ret=26;
		if( sc_data[SC_FREEZE].timer!=-1 )	// ����
			ret=21;
		if( sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2==0)
			ret=22;
	}

	return ret;
}

int status_get_attack_element(struct block_list *bl)
{
	int ret = 0;
	struct status_change *sc_data=status_get_sc_data(bl);

	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		ret=0;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		ret=((struct map_session_data *)bl)->atk_ele;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		ret=0;

	if(sc_data) {
		
		if( sc_data[SC_FROSTWEAPON].timer!=-1)	// �t���X�g�E�F�|��
			ret=1;
		if( sc_data[SC_SEISMICWEAPON].timer!=-1)	// �T�C�Y�~�b�N�E�F�|��
			ret=2;
		if( sc_data[SC_FLAMELAUNCHER].timer!=-1)	// �t���[�������`���[
			ret=3;
		if( sc_data[SC_LIGHTNINGLOADER].timer!=-1)	// ���C�g�j���O���[�_�[
			ret=4;
		if( sc_data[SC_ENCPOISON].timer!=-1)	// �G���`�����g�|�C�Y��
			ret=5;
		if( sc_data[SC_ASPERSIO].timer!=-1)		// �A�X�y���V�I
			ret=6;
		if( sc_data[SC_DARKELEMENT].timer!=-1)		// �ő���
			ret=7;
		if( sc_data[SC_ATTENELEMENT].timer!=-1)		// �O����
			ret=8;
	}
	return ret;
}
int status_get_attack_element2(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl) {
		int ret = ((struct map_session_data *)bl)->atk_ele_;
		struct status_change *sc_data = ((struct map_session_data *)bl)->sc_data;

		if(sc_data) {
			
			if( sc_data[SC_FROSTWEAPON].timer!=-1)	// �t���X�g�E�F�|��
				ret=1;
			if( sc_data[SC_SEISMICWEAPON].timer!=-1)	// �T�C�Y�~�b�N�E�F�|��
				ret=2;
			if( sc_data[SC_FLAMELAUNCHER].timer!=-1)	// �t���[�������`���[
				ret=3;
			if( sc_data[SC_LIGHTNINGLOADER].timer!=-1)	// ���C�g�j���O���[�_�[
				ret=4;
			if( sc_data[SC_ENCPOISON].timer!=-1)	// �G���`�����g�|�C�Y��
				ret=5;
			if( sc_data[SC_ASPERSIO].timer!=-1)		// �A�X�y���V�I
				ret=6;
			if( sc_data[SC_DARKELEMENT].timer!=-1)		// �ő���
				ret=7;
			if( sc_data[SC_ATTENELEMENT].timer!=-1)		// �O����
				ret=8;

		}
		return ret;
	}
	return 0;
}

int status_get_party_id(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->status.party_id;
	else if(bl->type==BL_MOB && (struct mob_data *)bl){
		struct mob_data *md=(struct mob_data *)bl;
		if( md->master_id>0 )
				return -md->master_id;
			return -md->bl.id;
	}
	else if(bl->type==BL_SKILL && (struct skill_unit *)bl)
		return ((struct skill_unit *)bl)->group->party_id;
	else
	return 0;
}
int status_get_guild_id(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->status.guild_id;
	else if(bl->type==BL_MOB && (struct mob_data *)bl)
		return ((struct mob_data *)bl)->class;
	else if(bl->type==BL_SKILL && (struct skill_unit *)bl)
		return ((struct skill_unit *)bl)->group->guild_id;
	else
		return 0;
}
int status_get_race(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return mob_db[((struct mob_data *)bl)->class].race;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return 7;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return mob_db[((struct pet_data *)bl)->class].race;
	else
		return 0;
}
int status_get_size(struct block_list *bl)
{
	nullpo_retr(1, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return mob_db[((struct mob_data *)bl)->class].size;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
	{
		//�]���ȉ��Ȃ璆�^
		if(((struct mob_data *)bl)->class < PC_CLASS_BASE3)
			return 1;
		//�{�q
		return 0;
	}else if(bl->type==BL_PET && (struct pet_data *)bl)
		return mob_db[((struct pet_data *)bl)->class].size;
	else
		return 1;
}
int status_get_mode(struct block_list *bl)
{
	nullpo_retr(0x01, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return mob_db[((struct mob_data *)bl)->class].mode;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return mob_db[((struct pet_data *)bl)->class].mode;
	else
		return 0x01;	// �Ƃ肠���������Ƃ������Ƃ�1
}

int status_get_mexp(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return mob_db[((struct mob_data *)bl)->class].mexp;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return mob_db[((struct pet_data *)bl)->class].mexp;
	else
		return 0;
}

// StatusChange�n�̏���
struct status_change *status_get_sc_data(struct block_list *bl)
{
	nullpo_retr(NULL, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return ((struct mob_data*)bl)->sc_data;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data*)bl)->sc_data;
	return NULL;
}
short *status_get_sc_count(struct block_list *bl)
{
	nullpo_retr(NULL, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return &((struct mob_data*)bl)->sc_count;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return &((struct map_session_data*)bl)->sc_count;
	return NULL;
}
short *status_get_opt1(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return &((struct mob_data*)bl)->opt1;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return &((struct map_session_data*)bl)->opt1;
	else if(bl->type==BL_NPC && (struct npc_data *)bl)
		return &((struct npc_data*)bl)->opt1;
	return 0;
}
short *status_get_opt2(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return &((struct mob_data*)bl)->opt2;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return &((struct map_session_data*)bl)->opt2;
	else if(bl->type==BL_NPC && (struct npc_data *)bl)
		return &((struct npc_data*)bl)->opt2;
	return 0;
}
short *status_get_opt3(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return &((struct mob_data*)bl)->opt3;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return &((struct map_session_data*)bl)->opt3;
	else if(bl->type==BL_NPC && (struct npc_data *)bl)
		return &((struct npc_data*)bl)->opt3;
	return 0;
}
short *status_get_option(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return &((struct mob_data*)bl)->option;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return &((struct map_session_data*)bl)->status.option;
	else if(bl->type==BL_NPC && (struct npc_data *)bl)
		return &((struct npc_data*)bl)->option;
	return 0;
}

//
int status_calc_pc_itemeffect_init(struct map_session_data* sd)
{
	return 0;
}
//
int	status_calc_pc_itemeffect_finish(struct map_session_data* sd)
{
	//�I�[�g�X�y���̍Čv�Z
	status_calc_pc_autospell(sd);
	return 0;
}
//�I�[�g�X�y��
int status_calc_pc_autospell(struct map_session_data* sd)
{
	int i,j;
	nullpo_retr(0, sd);
	if(sd->bl.type != BL_PC)
		return 0;
	//���������Z
	for(i = 0;sd->autospell.add_count;i++)
	{	
		for(j=0;j<sd->autospell.count;j++)
			if(sd->autospell.id[j] == sd->autospell.add_id[i])
				sd->autospell.rate[j] += sd->autospell.add_rate[i];
	}
	//�֎~
	for(i = 0;i<sd->autospell.ban_count;i++)
	{	
		for(j=0;j<sd->autospell.count;j++)
			if(sd->autospell.id[j] == sd->autospell.ban[i])
				sd->autospell.flag[j] = 0;// = sd->autospell.flag[j]&(~EAS_ENABLE); //
	}	
	return 0;
}
/*==========================================
 * �X�e�[�^�X�ُ�J�n
 *------------------------------------------
 */
int status_change_start(struct block_list *bl,int type,int val1,int val2,int val3,int val4,int tick,int flag)
{
	struct map_session_data *sd = NULL;
	struct status_change* sc_data;
	short *sc_count, *option, *opt1, *opt2, *opt3;
	int opt_flag = 0, calc_flag = 0, updateflag = 0, race, mode, elem, undead_flag;
	int scdef=0;

	nullpo_retr(0, bl);
	if(bl->type == BL_SKILL)
		return 0;
	nullpo_retr(0, sc_data=status_get_sc_data(bl));
	nullpo_retr(0, sc_count=status_get_sc_count(bl));
	nullpo_retr(0, option=status_get_option(bl));
	nullpo_retr(0, opt1=status_get_opt1(bl));
	nullpo_retr(0, opt2=status_get_opt2(bl));
	nullpo_retr(0, opt3=status_get_opt3(bl));


	race=status_get_race(bl);
	mode=status_get_mode(bl);
	elem=status_get_elem_type(bl);
	undead_flag=(elem == 9 || race == 1)?1:0;


	if(type == SC_AETERNA && (sc_data[SC_STONE].timer != -1 || sc_data[SC_FREEZE].timer != -1) )
		return 0;

	//ON/OFF
	switch(type)
	{
		case SC_AUTOBERSERK:
		case SC_READYSTORM:
		case SC_READYDOWN:
		case SC_READYTURN:
		case SC_READYCOUNTER:
		case SC_DODGE:
			if(sc_data[type].timer!=-1)
			{
				status_change_end(bl,type,-1);
				return 0;
			}
			break;
	}
	
	switch(type){
		case SC_STONE:
		case SC_FREEZE:
			scdef=3+status_get_mdef(bl)+status_get_luk(bl)/3;
			break;
		case SC_STAN:
		case SC_SILENCE:
		case SC_POISON:
		case SC_DPOISON:
			scdef=3+status_get_vit(bl)+status_get_luk(bl)/3;
			break;
		case SC_SLEEP:
		case SC_BLIND:
			scdef=3+status_get_int(bl)+status_get_luk(bl)/3;
			break;
		case SC_CURSE:
			scdef=3+status_get_luk(bl);
			break;

//		case SC_CONFUSION:
		default:
			scdef=0;
	}
	if(scdef>=100)
		return 0;
	if(bl->type==BL_PC){
		sd=(struct map_session_data *)bl;
		if( sd && type == SC_ADRENALINE && !(skill_get_weapontype(BS_ADRENALINE)&(1<<sd->status.weapon)))
			return 0;

		if(SC_STONE<=type && type<=SC_BLIND){	/* �J�[�h�ɂ��ϐ� */
			if( sd && sd->reseff[type-SC_STONE] > 0 && atn_rand()%10000<sd->reseff[type-SC_STONE]){
				if(battle_config.battle_log)
					printf("PC %d skill_sc_start: card�ɂ��ُ�ϐ�����\n",sd->bl.id);
				return 0;
			}
		}
	}
	else if(bl->type == BL_MOB) {
	}
	else {
		if(battle_config.error_log)
			printf("status_change_start: neither MOB nor PC !\n");
		return 0;
	}

	if(type==SC_FREEZE && undead_flag && !(flag&1))
		return 0;

	//�Ή�����
	if(type==SC_STONE && undead_flag && !(flag&1))
		return 0;
		
	if((type == SC_ADRENALINE || type==SC_ADRENALINE2 || type == SC_WEAPONPERFECTION || type == SC_OVERTHRUST) &&
		sc_data[type].timer != -1 && sc_data[type].val2 && !val2)
		return 0;

	if(mode & 0x20 && (type==SC_STONE || type==SC_FREEZE ||
		type==SC_STAN || type==SC_SLEEP || type==SC_SILENCE || type==SC_QUAGMIRE || type == SC_DECREASEAGI || type == SC_PROVOKE ||
		(type == SC_BLESSING && (undead_flag || race == 6))) && !(flag&1)){
		/* �{�X�ɂ͌����Ȃ�(�������J�[�h�ɂ����ʂ͓K�p�����) */
		return 0;
	}
	if(type==SC_FREEZE || type==SC_STAN || type==SC_SLEEP)
		battle_stopwalking(bl,1);

	if (type==SC_BLESSING && (bl->type==BL_PC || (!undead_flag && race!=6))) {
		if (sc_data[SC_CURSE].timer!=-1)
			status_change_end(bl,SC_CURSE,-1);
		if (sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2==0)
			status_change_end(bl,SC_STONE,-1);
	}

	if(sc_data[type].timer != -1){	/* ���łɓ����ُ�ɂȂ��Ă���ꍇ�^�C�}���� */
		if(sc_data[type].val1 > val1 && type != SC_COMBO && type != SC_DANCING && type != SC_DEVOTION &&
			type != SC_SPEEDPOTION0 && type != SC_SPEEDPOTION1 && type != SC_SPEEDPOTION2 && type!= SC_DEVIL)
			return 0;
		if ((type >=SC_STAN && type <= SC_BLIND) || type == SC_DPOISON)
			return 0;/* �p���������ł��Ȃ���Ԉُ�ł��鎞�͏�Ԉُ���s��Ȃ� */
		if(type == SC_GRAFFITI){	//�ُ풆�ɂ�����x��Ԉُ�ɂȂ������ɉ������Ă���ēx������
			status_change_end(bl,type,-1);
		}else{
			(*sc_count)--;
			delete_timer(sc_data[type].timer, status_change_timer);
			sc_data[type].timer = -1;
		}
	}
	// �N�A�O�}�C�A/����Y��Ȃ��Œ��͖����ȃX�L��
	if ((sc_data[SC_QUAGMIRE].timer!=-1 || sc_data[SC_DONTFORGETME].timer!=-1) &&
			(type==SC_CONCENTRATE || type==SC_INCREASEAGI ||
			type==SC_TWOHANDQUICKEN || type==SC_ONEHAND || type==SC_SPEARSQUICKEN ||
			type==SC_ADRENALINE || type==SC_ADRENALINE2 || type==SC_LOUD || type==SC_TRUESIGHT ||
			type==SC_WINDWALK || type==SC_CARTBOOST || type==SC_ASSNCROS ||
			type==SC_INCFLEE))
		return 0;

	//���p ������ł��`�F�b�N
	if(bl->type == BL_PC && battle_config.job_soul_check && type<=SC_ALCHEMIST && type<=SC_HIGH)
	{
		struct pc_base_job s_class;
		struct map_session_data* sd = (struct map_session_data*)bl;
		s_class = pc_calc_base_job(sd->status.class);
		switch(type)
		{
			case SC_ALCHEMIST://#�A���P�~�X�g�̍�#
				if(s_class.job != 18)
				 	return 0;
				 break;
			case SC_MONK://#�����N�̍�#
				if(s_class.job != 15)
				 	return 0;
				 break;
			case SC_STAR://#�P���Z�C�̍�#
				 if(s_class.job == 25 || s_class.job == 26) break;
				 else return 0;
				 break;
			case SC_SAGE://#�Z�[�W�̍�#
				if(s_class.job != 16)
				 	return 0;
				 break;
			case SC_CRUSADER://#�N���Z�C�_�[�̍�#
				if(s_class.job != 14)
				 	return 0;
				 break;
			case SC_SUPERNOVICE://#�X�[�p�[�m�[�r�X�̍�#
				if(s_class.job != 23)
				 	return 0;
				 break;
			case SC_KNIGHT://#�i�C�g�̍�#
				if(s_class.job != 7)
				 	return 0;
				 break;
			case SC_WIZARD://#�E�B�U�[�h�̍�#	
				if(s_class.job != 9)
				 	return 0;
				 break;
			case SC_PRIEST://#�v���[�X�g�̍�#
				if(s_class.job != 8)
				 	return 0;
				 break;
			case SC_BARDDANCER://#�o�[�h�ƃ_���T�[�̍�#
				if(s_class.job == 19 || s_class.job ==20)	break;
				 else	return 0;
				 break;
			case SC_ROGUE://#���[�O�̍�#
				if(s_class.job != 17)
				 	return 0;
				 break;
			case SC_ASSASIN://#�A�T�V���̍�#
				if(s_class.job != 12)
				 	return 0;
				 break;
			case SC_BLACKSMITH://#�u���b�N�X�~�X�̍�#
				if(s_class.job != 10)
				 	return 0;
				 break;
			case SC_HUNTER://#�n���^�[�̍�#
				if(s_class.job != 11)
				 	return 0;
				 break;
			case SC_SOULLINKER://#�\�E�������J�[�̍�#
				if(s_class.job != 27)
					return 0;
				 break;
			case SC_HIGH://�ꎟ��ʐE�Ƃ̍�
				if((s_class.job>=1 && s_class.job <=6) && s_class.upper==1)
					break;
				return 0;
				break;
			default:
				break;
		}
	}
	
	switch(type){	/* �ُ�̎�ނ��Ƃ̏��� */
		case SC_PROVOKE:			/* �v���{�b�N */
			calc_flag = 1;
			if(tick <= 0) tick = 1000;	/* (�I�[�g�o�[�T�[�N) */
			break;
		case SC_ENDURE:				/* �C���f���A */
			if(tick <= 0) tick = 1000 * 60;
			calc_flag = 1;
			val2 = 7;	// 7��U�����ꂽ�����
			break;
		case SC_CONCENTRATE:		/* �W���͌��� */
		case SC_BLESSING:			/* �u���b�V���O */
		case SC_ANGELUS:			/* �A���[���X */
			calc_flag = 1;
			break;
		case SC_INCREASEAGI:		/* ���x�㏸ */
			calc_flag = 1;
			if(sc_data[SC_DECREASEAGI].timer!=-1 )
				status_change_end(bl,SC_DECREASEAGI,-1);
			if(sc_data[SC_WINDWALK].timer!=-1 )	/* �E�C���h�E�H�[�N */
				status_change_end(bl,SC_WINDWALK,-1);
			break;
		case SC_DECREASEAGI:		/* ���x���� */
			calc_flag = 1;
			if(sc_data[SC_INCREASEAGI].timer!=-1 )
				status_change_end(bl,SC_INCREASEAGI,-1);
			break;
		case SC_SIGNUMCRUCIS:		/* �V�O�i���N���V�X */
			calc_flag = 1;
			val2 = 10 + val1*4;
			tick = 600*1000;
			clif_emotion(bl,4);
			break;
		case SC_SLOWPOISON:
			if (sc_data[SC_POISON].timer == -1 && sc_data[SC_DPOISON].timer == -1)
				return 0;
			break;
		case SC_ONEHAND:			/* 1HQ */
			if(type!=SC_SPEEDPOTION0 && sc_data[SC_SPEEDPOTION0].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION0,-1);
			if(type!=SC_SPEEDPOTION1 && sc_data[SC_SPEEDPOTION1].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION1,-1);
			if(type!=SC_SPEEDPOTION2 && sc_data[SC_SPEEDPOTION2].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION2,-1);
			*opt3 |= 1;
			calc_flag = 1;
			break;
		case SC_TWOHANDQUICKEN:		/* 2HQ */
			*opt3 |= 1;
			calc_flag = 1;
			break;
		case SC_ADRENALINE:			/* �A�h���i�������b�V�� */
			if(sc_data[SC_ADRENALINE2].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE2,-1);
			calc_flag = 1;
			if(bl->type == BL_PC)
				if(pc_checkskill(sd,BS_HILTBINDING)>0)
					tick += tick / 10;
			break;
		case SC_ADRENALINE2:			/* �t���A�h���i�������b�V�� */
			calc_flag = 1;
			if(sc_data[SC_ADRENALINE].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE,-1);
			if(bl->type == BL_PC)
				if(pc_checkskill(sd,BS_HILTBINDING)>0)
					tick += tick / 10;
			break;
		case SC_WEAPONPERFECTION:	/* �E�F�|���p�[�t�F�N�V���� */
			if(bl->type == BL_PC)
				if(pc_checkskill(sd,BS_HILTBINDING)>0)
					tick += tick / 10;
			break;
		case SC_OVERTHRUST:			/* �I�[�o�[�g���X�g */
			if(sc_data[SC_OVERTHRUSTMAX].timer!=-1)
				status_change_end(bl,SC_OVERTHRUSTMAX,-1);
			*opt3 |= 2;
			if(bl->type == BL_PC)
				if(pc_checkskill(sd,BS_HILTBINDING)>0)
					tick += tick / 10;
			break;
		case SC_MAXIMIZEPOWER:		/* �}�L�V�}�C�Y�p���[(SP��1���鎞��,val2�ɂ�) */
			if(bl->type == BL_PC)
				val2 = tick;
			else
				tick = 5000*val1;
			break;
		case SC_ENCPOISON:			/* �G���`�����g�|�C�Y�� */
			calc_flag = 1;
			val2=(((val1 - 1) / 2) + 3)*100;	/* �ŕt�^�m�� */
			skill_encchant_eremental_end(bl,SC_ENCPOISON);
			break;
		case SC_EDP:			/* �G���`�����g�f�b�h���[�|�C�Y�� */
		{
		//	struct map_session_data *sd = (struct map_session_data *)bl;
		//	clif_displaymessage(sd->fd, " ����ɖғő������t�^����܂���");
			val2 = val1 + 2;			/* �ғŕt�^�m��(%) */
			//  calc_flag�͕K�v�Ȃ�
			break;
		}
		case SC_POISONREACT:	/* �|�C�Y�����A�N�g */
			break;
		case SC_IMPOSITIO:			/* �C���|�V�e�B�I�}�k�X */
			calc_flag = 1;
			break;
		case SC_ASPERSIO:			/* �A�X�y���V�I */
			skill_encchant_eremental_end(bl,SC_ASPERSIO);
			break;
		case SC_SUFFRAGIUM:			/* �T�t���M�� */
		case SC_BENEDICTIO:			/* ���� */
		case SC_MAGNIFICAT:			/* �}�O�j�t�B�J�[�g */
		case SC_AETERNA:			/* �G�[�e���i */
		case SC_BASILICA:			/* �o�W���J */
			break;
		case SC_ENERGYCOAT:			/* �G�i�W�[�R�[�g */
			*opt3 |= 4;
			break;
		case SC_MAGICROD:
			val2 = val1*20;
			break;
		case SC_KYRIE:				/* �L���G�G���C�\�� */
			/* �A�X�����|�����Ă������������ */
			if(sc_data[SC_ASSUMPTIO].timer!=-1)
			status_change_end(bl,SC_ASSUMPTIO,-1);
		
			/* �L���G���|���� */
			val2 = (int)((double)status_get_max_hp(bl) * (val1 * 2 + 10) / 100);/* �ϋv�x */
			val3 = (val1 / 2 + 5);	/* �� */
			break;
		case SC_GLORIA:				/* �O�����A */
			calc_flag = 1;
			break;
		case SC_LOUD:				/* ���E�h�{�C�X */
		case SC_MINDBREAKER:		/* �}�C���h�u���[�J�[ */
			calc_flag = 1;
			break;
		case SC_TRICKDEAD:			/* ���񂾂ӂ� */
			break;
		case SC_QUAGMIRE:			/* �N�@�O�}�C�A */
			calc_flag = 1;
			if(sc_data[SC_CONCENTRATE].timer!=-1 )	/* �W���͌������ */
				status_change_end(bl,SC_CONCENTRATE,-1);
			if(sc_data[SC_INCREASEAGI].timer!=-1 )	/* ���x�㏸���� */
				status_change_end(bl,SC_INCREASEAGI,-1);
			if(sc_data[SC_TWOHANDQUICKEN].timer!=-1 )
				status_change_end(bl,SC_TWOHANDQUICKEN,-1);
			if(sc_data[SC_SPEARSQUICKEN].timer!=-1 )
				status_change_end(bl,SC_SPEARSQUICKEN,-1);
			if(sc_data[SC_ADRENALINE].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE,-1);
			if(sc_data[SC_ADRENALINE2].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE2,-1);
			if(sc_data[SC_LOUD].timer!=-1 )
				status_change_end(bl,SC_LOUD,-1);
			if(sc_data[SC_TRUESIGHT].timer!=-1 )	/* �g�D���[�T�C�g */
				status_change_end(bl,SC_TRUESIGHT,-1);
			if(sc_data[SC_WINDWALK].timer!=-1 )	/* �E�C���h�E�H�[�N */
				status_change_end(bl,SC_WINDWALK,-1);
			if(sc_data[SC_CARTBOOST].timer!=-1 )	/* �J�[�g�u�[�X�g */
				status_change_end(bl,SC_CARTBOOST,-1);
			if(sc_data[SC_INCFLEE].timer!=-1 )	/* ���x�������� */
				status_change_end(bl,SC_INCFLEE,-1);
			if(sc_data[SC_ONEHAND].timer!=-1 )
				status_change_end(bl,SC_ONEHAND,-1);
			break;
		case SC_MAGICPOWER:			/* ���@�͑��� */
			val2 = 1;				// ��x��������
			break;
		case SC_SACRIFICE:			/* �T�N���t�@�C�X */
			val2 = 5;				// 5��̍U���ŗL��
			break;
		case SC_FLAMELAUNCHER:		/* �t���[�������`���[ */
			skill_encchant_eremental_end(bl,SC_FLAMELAUNCHER);
			break;
		case SC_FROSTWEAPON:		/* �t���X�g�E�F�|�� */
			skill_encchant_eremental_end(bl,SC_FROSTWEAPON);
			break;
		case SC_LIGHTNINGLOADER:	/* ���C�g�j���O���[�_�[ */
			skill_encchant_eremental_end(bl,SC_LIGHTNINGLOADER);
			break;
		case SC_SEISMICWEAPON:		/* �T�C�Y�~�b�N�E�F�|�� */
			skill_encchant_eremental_end(bl,SC_SEISMICWEAPON);
			break;
		case SC_DARKELEMENT:		/* ��ݑ��� */
			skill_encchant_eremental_end(bl,SC_DARKELEMENT);
			break;
		case SC_ATTENELEMENT:		/* �O���� */
			skill_encchant_eremental_end(bl,SC_ATTENELEMENT);
			break;
		case SC_DEVOTION:			/* �f�B�{�[�V���� */
			calc_flag = 1;
			break;
		case SC_PROVIDENCE:			/* �v�����B�f���X */
			calc_flag = 1;
			val2=val1*5;
			break;
		case SC_REFLECTSHIELD:
			val2=10+val1*3;
			break;
		case SC_STRIPWEAPON:
		case SC_STRIPSHIELD:
		case SC_STRIPARMOR:
		case SC_STRIPHELM:
		case SC_CP_WEAPON:
		case SC_CP_SHIELD:
		case SC_CP_ARMOR:
		case SC_CP_HELM:
			break;

		case SC_AUTOSPELL:			/* �I�[�g�X�y�� */
			val4 = 5 + val1*2;
			break;

		case SC_VOLCANO:
			calc_flag = 1;
			val3 = val1*10;
			val4 = val1>=5?20: (val1==4?19: (val1==3?17: ( val1==2?14:10 ) ) );
			break;
		case SC_DELUGE:
			calc_flag = 1;
			val3 = val1>=5?15: (val1==4?14: (val1==3?12: ( val1==2?9:5 ) ) );
			val4 = val1>=5?20: (val1==4?19: (val1==3?17: ( val1==2?14:10 ) ) );
			break;
		case SC_VIOLENTGALE:
			calc_flag = 1;
			val3 = val1*3;
			val4 = val1>=5?20: (val1==4?19: (val1==3?17: ( val1==2?14:10 ) ) );
			break;

		case SC_SPEARSQUICKEN:		/* �X�s�A�N�C�b�P�� */
			calc_flag = 1;
			val2 = 20+val1;
			*opt3 |= 1;
			break;
		case SC_COMBO:
			break;
		case SC_BLADESTOP_WAIT:		/* ���n���(�҂�) */
			break;
		case SC_BLADESTOP:		/* ���n��� */
			if(val2==2) clif_bladestop((struct block_list *)val3,(struct block_list *)val4,1);
			*opt3 |= 32;
			break;

		case SC_LULLABY:			/* �q��S */
			val2 = 11;
			break;
		case SC_RICHMANKIM:
			break;
		case SC_ETERNALCHAOS:		/* �G�^�[�i���J�I�X */
			calc_flag = 1;
			break;
		case SC_DRUMBATTLE:			/* �푾�ۂ̋��� */
			calc_flag = 1;
			val2 = (val1+1)*25;
			val3 = (val1+1)*2;
			break;
		case SC_NIBELUNGEN:			/* �j�[�x�����O�̎w�� */
			calc_flag = 1;
			val2 = (val1+2)*50;
			break;
		case SC_ROKISWEIL:			/* ���L�̋��� */
			break;
		case SC_INTOABYSS:			/* �[���̒��� */
			break;
		case SC_SIEGFRIED:			/* �s���g�̃W�[�N�t���[�h */
			calc_flag = 1;
			val2 = 5 + val1*15;
			break;
		case SC_DISSONANCE:			/* �s���a�� */
			val2 = 10;
			break;
		case SC_WHISTLE:			/* ���J */
			calc_flag = 1;
			break;
		case SC_ASSNCROS:			/* �[�z�̃A�T�V���N���X */
			calc_flag = 1;
			break;
		case SC_POEMBRAGI:			/* �u���M�̎� */
			break;
		case SC_APPLEIDUN:			/* �C�h�D���̗ь� */
			calc_flag = 1;
			break;
		case SC_UGLYDANCE:			/* ��������ȃ_���X */
			val2 = 10;
			break;
		case SC_HUMMING:			/* �n�~���O */
			calc_flag = 1;
			break;
		case SC_DONTFORGETME:		/* ����Y��Ȃ��� */
			calc_flag = 1;
			if(sc_data[SC_INCREASEAGI].timer!=-1 )	/* ���x�㏸���� */
				status_change_end(bl,SC_INCREASEAGI,-1);
			if(sc_data[SC_TWOHANDQUICKEN].timer!=-1 )
				status_change_end(bl,SC_TWOHANDQUICKEN,-1);
			if(sc_data[SC_SPEARSQUICKEN].timer!=-1 )
				status_change_end(bl,SC_SPEARSQUICKEN,-1);
			if(sc_data[SC_ADRENALINE].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE,-1);
			if(sc_data[SC_ADRENALINE2].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE2,-1);
			if(sc_data[SC_ASSNCROS].timer!=-1 )
				status_change_end(bl,SC_ASSNCROS,-1);
			if(sc_data[SC_TRUESIGHT].timer!=-1 )	/* �g�D���[�T�C�g */
				status_change_end(bl,SC_TRUESIGHT,-1);
			if(sc_data[SC_WINDWALK].timer!=-1 )	/* �E�C���h�E�H�[�N */
				status_change_end(bl,SC_WINDWALK,-1);
			if(sc_data[SC_CARTBOOST].timer!=-1 )	/* �J�[�g�u�[�X�g */
				status_change_end(bl,SC_CARTBOOST,-1);
			if(sc_data[SC_ONEHAND].timer!=-1 )
				status_change_end(bl,SC_ONEHAND,-1);
			break;
		case SC_FORTUNE:			/* �K�^�̃L�X */
			calc_flag = 1;
			break;
		case SC_SERVICE4U:			/* �T�[�r�X�t�H�[���[ */
			calc_flag = 1;
			break;
		//�ړ��H
		case SC_WHISTLE_:			/* ���J */
			calc_flag = 1;
			break;
		case SC_ASSNCROS_:			/* �[�z�̃A�T�V���N���X */
			calc_flag = 1;
			break;
		case SC_POEMBRAGI_:			/* �u���M�̎� */
			break;
		case SC_APPLEIDUN_:			/* �C�h�D���̗ь� */
			calc_flag = 1;
			break;
		case SC_HUMMING_:			/* �n�~���O */
		puts("SC_HUMMING_");
			calc_flag = 1;
			break;
		case SC_DONTFORGETME_:		/* ����Y��Ȃ��� */
			calc_flag = 1;
			/*
			if(sc_data[SC_INCREASEAGI].timer!=-1 )	// ���x�㏸���� 
				status_change_end(bl,SC_INCREASEAGI,-1);
			if(sc_data[SC_TWOHANDQUICKEN].timer!=-1 )
				status_change_end(bl,SC_TWOHANDQUICKEN,-1);
			if(sc_data[SC_SPEARSQUICKEN].timer!=-1 )
				status_change_end(bl,SC_SPEARSQUICKEN,-1);
			if(sc_data[SC_ADRENALINE].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE,-1);
			if(sc_data[SC_ASSNCROS].timer!=-1 )
				status_change_end(bl,SC_ASSNCROS,-1);
			if(sc_data[SC_TRUESIGHT].timer!=-1 )	// �g�D���[�T�C�g
				status_change_end(bl,SC_TRUESIGHT,-1);
			if(sc_data[SC_WINDWALK].timer!=-1 )	// �E�C���h�E�H�[�N 
				status_change_end(bl,SC_WINDWALK,-1);
			if(sc_data[SC_CARTBOOST].timer!=-1 )	// �J�[�g�u�[�X�g
				status_change_end(bl,SC_CARTBOOST,-1);
			if(sc_data[SC_ONEHAND].timer!=-1 )
				status_change_end(bl,SC_ONEHAND,-1);
				*/
			break;
		case SC_FORTUNE_:			/* �K�^�̃L�X */
			calc_flag = 1;
			break;
		case SC_SERVICE4U_:			/* �T�[�r�X�t�H�[���[ */
			calc_flag = 1;
			break;
		case SC_DANCING:			/* �_���X/���t�� */
			calc_flag = 1;
			val3= tick / 1000;
			tick = 1000;
			break;
		case SC_EXPLOSIONSPIRITS:	// �����g��
			calc_flag = 1;
			val2 = 75 + 25*val1;
			*opt3 |= 8;
			break;
		case SC_STEELBODY:			// ����
			calc_flag = 1;
			*opt3 |= 16;
			break;
		case SC_EXTREMITYFIST:		/* ���C���e���� */
			break;
		case SC_AUTOCOUNTER:
			val3 = val4 = 0;
			break;
		case SC_POISONPOTION:		/* �Ŗ�̕r */
			calc_flag = 1;
			tick = 1000 * tick;
			val2 = 25;
			break;
		case SC_SPEEDPOTION0:		/* �����|�[�V���� */
		case SC_SPEEDPOTION1:
		case SC_SPEEDPOTION2:
			if(type!=SC_SPEEDPOTION0 && sc_data[SC_SPEEDPOTION0].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION0,-1);
			if(type!=SC_SPEEDPOTION1 && sc_data[SC_SPEEDPOTION1].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION1,-1);
			if(type!=SC_SPEEDPOTION2 && sc_data[SC_SPEEDPOTION2].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION2,-1);
			calc_flag = 1;
			tick = 1000 * tick;
			val2 = 5*(2+type-SC_SPEEDPOTION0);
			break;

		case SC_INCATK:		//item 682�p
		case SC_INCMATK:	//item 683�p
			calc_flag = 1;
			tick = 1000 * tick;
			break;
		case SC_WEDDING:	//�����p(�����ߏւɂȂ��ĕ����̂��x���Ƃ�)
			{
				time_t timer;
	
				calc_flag = 1;
				tick = 10000;
				if(!val2)
					val2 = time(&timer);
				if( bl->type == BL_PC ){
					pc_setglobalreg(sd,"PC_WEDDING_TIME",val2);
				}
			}
			break;
		case SC_NOCHAT:	//�`���b�g�֎~���
			{
				time_t timer;
				tick = 60000;
				if(!val2)
					val2 = time(&timer);
				updateflag = SP_MANNER;
			}
			break;
		case SC_SELFDESTRUCTION: //����
			clif_skillcasting(bl,bl->id, bl->id,0,0,331,skill_get_time(val2,val1));
			val3 = tick / 1000;
			tick = 1000;
			break;

		/* option1 */
		case SC_STONE:				/* �Ή� */
			if(!(flag&2)) {
				int sc_def = status_get_mdef(bl)*200;
				tick = tick - sc_def;
			}
			val3 = tick/1000;
			if(val3 < 1) val3 = 1;
			tick = 5000;
			val2 = 1;
			break;
		case SC_SLEEP:				/* ���� */
			if(!(flag&2)) {
//				int sc_def = 100 - (status_get_int(bl) + status_get_luk(bl)/3);
//				tick = tick * sc_def / 100;
//				if(tick < 1000) tick = 1000;
				tick = 30000;//�����̓X�e�[�^�X�ϐ��Ɋւ�炸30�b
			}
			break;
		case SC_FREEZE:				/* ���� */
			if(!(flag&2)) {
				int sc_def = 100 - status_get_mdef(bl);
				tick = tick * sc_def / 100;
			}
			break;
		case SC_STAN:				/* �X�^���ival2�Ƀ~���b�Z�b�g�j */
			if(!(flag&2)) {
				int sc_def = 100 - (status_get_vit(bl) + status_get_luk(bl)/3);
				tick = tick * sc_def / 100;
			}
			break;
		/* option2 */
		case SC_DPOISON:			/* �ғ� */
		{
			int mhp = status_get_max_hp(bl);
			int hp = status_get_hp(bl);
			// MHP��1/4�ȉ��ɂ͂Ȃ�Ȃ�
			if (hp > mhp>>2) {
				if(bl->type == BL_PC) {
					int diff = mhp*10/100;
					if (hp - diff < mhp>>2)
						hp = hp - (mhp>>2);
					pc_heal((struct map_session_data *)bl, -hp, 0);
				} else if(bl->type == BL_MOB) {
					struct mob_data *md = (struct mob_data *)bl;
					hp -= mhp*15/100;
					if (hp > mhp>>2)
						md->hp = hp;
					else
						md->hp = mhp>>2;
				}
			}
		}	// fall through
		case SC_POISON:				/* �� */
			calc_flag = 1;
			if(!(flag&2)) {
				int sc_def = 100 - (status_get_vit(bl) + status_get_luk(bl)/5);
				tick = tick * sc_def / 100;
			}
			val3 = tick/1000;
			if(val3 < 1) val3 = 1;
			tick = 1000;
			break;
		case SC_SILENCE:			/* ���فi���b�N�X�f�r�[�i�j */
			if (sc_data && sc_data[SC_GOSPEL].timer!=-1) {
				skill_delunitgroup((struct skill_unit_group *)sc_data[SC_GOSPEL].val3);
				status_change_end(bl,SC_GOSPEL,-1);
				break;
			}
			if(!(flag&2)) {
				int sc_def = 100 - status_get_vit(bl);
				tick = tick * sc_def / 100;
			}
			break;
		case SC_BLIND:				/* �Í� */
			calc_flag = 1;
			if(!(flag&2)) {
				int sc_def = status_get_lv(bl)/10 + status_get_int(bl)/15;
				tick = 30000 - sc_def;
			}
			break;
		case SC_CURSE:
			calc_flag = 1;
			if(!(flag&2)) {
				int sc_def = 100 - status_get_vit(bl);
				tick = tick * sc_def / 100;
			}
			break;

		/* option */
		case SC_HIDING:		/* �n�C�f�B���O */
			calc_flag = 1;
			if(bl->type == BL_PC) {
				val2 = tick / 1000;		/* �������� */
				tick = 1000;
			}
			break;
		case SC_CHASEWALK:		/*�`�F�C�X�E�H�[�N*/
		case SC_CLOAKING:		/* �N���[�L���O */
			if(bl->type == BL_PC)
			{
				calc_flag = 1;
				val2 = tick;
				val3 = type==SC_CLOAKING ? 130-val1*3 : 135-val1*5;
			}
			else
				tick = 5000*val1;
			break;
		case SC_SIGHT:			/* �T�C�g/���A�t */
		case SC_RUWACH:
			val2 = tick/250;
			tick = 10;
			break;

		/* �Z�[�t�e�B�E�H�[���A�j���[�} */
		case SC_SAFETYWALL:	case SC_PNEUMA:
			tick=((struct skill_unit *)val2)->group->limit;
			break;

		/* �A���N�� */
		case SC_ANKLE:
			break;

		/* �X�L������Ȃ�/���ԂɊ֌W���Ȃ� */
		case SC_RIDING:
			calc_flag = 1;
			tick = 600*1000;
			break;
		case SC_FALCON:	case SC_WEIGHT50:	case SC_WEIGHT90:
			tick=600*1000;
			break;

		case SC_AUTOGUARD:
			{
				int i,t;
				for(i=val2=0;i<val1;i++) {
					t = 5-(i>>1);
					val2 += (t < 0)? 1:t;
				}
			}
			break;

		case SC_DEFENDER:
			calc_flag = 1;
			val2 = 5 + val1*15;
			break;

		case SC_KEEPING:
		case SC_BARRIER:
		case SC_HALLUCINATION:
			break;
		case SC_CONCENTRATION:	/* �R���Z���g���[�V���� */
			*opt3 |= 1;
			calc_flag = 1;
			break;
		case SC_TENSIONRELAX:	/* �e���V���������b�N�X */
			if(bl->type == BL_PC) {
				tick = 10000;
			} else
				return 0;
			break;
		case SC_AURABLADE:		/* �I�[���u���[�h */
		case SC_PARRYING:		/* �p���C���O */
		case SC_HEADCRUSH:		/* �w�b�h�N���b�V�� */
		case SC_JOINTBEAT:		/* �W���C���g�r�[�g */
		case SC_MELTDOWN:		/* �����g�_�E�� */

			//�Ƃ肠�����蔲��
			break;
		case SC_WINDWALK:		/* �E�C���h�E�H�[�N */
			calc_flag = 1;
			val2 = (val1 / 2); //Flee�㏸��
			break;
		case SC_BERSERK:		/* �o�[�T�[�N */
			if(sd){
				sd->status.sp = 0;
				clif_updatestatus(sd,SP_SP);
				clif_status_change(bl,SC_INCREASEAGI,1);	/* �A�C�R���\�� */
			}
			*opt3 |= 128;
			tick = 1000;
			calc_flag = 1;
			break;
		case SC_ASSUMPTIO:		/* �A�X���v�e�B�I */
			/* �L���G���|�����Ă������������ */
			if(sc_data[SC_KYRIE].timer!=-1)
				status_change_end(bl,SC_KYRIE,-1);
			/* �J�C�g���|�����Ă������������ */
			if(sc_data[SC_KAITE].timer!=-1)
			status_change_end(bl,SC_KAITE,-1);

			/* �A�X���̃t���O�𗧂Ă� */
			*opt3 |= 2048;
			break;
		case SC_MARIONETTE:		/* �}���I�l�b�g�R���g���[�� */
			*opt3 |= 1024;
			break;

		case SC_CARTBOOST:		/* �J�[�g�u�[�X�g */
		case SC_TRUESIGHT:		/* �g�D���[�T�C�g */
		case SC_SPIDERWEB:		/* �X�p�C�_�[�E�F�b�u */
			calc_flag = 1;
			break;
		case SC_REJECTSWORD:	/* ���W�F�N�g�\�[�h */
			val2 = 3; //3��U���𒵂˕Ԃ�
			break;
		case SC_MEMORIZE:		/* �������C�Y */
			val2 = 5; //5��r����1/2�ɂ���
			break;
		case SC_GRAFFITI:		/* �O���t�B�e�B */
			{
				struct skill_unit_group *sg = skill_unitsetting(bl,RG_GRAFFITI,val1,val2,val3,0);
				if(sg)
					val4 = (int)sg;
			}
			break;
		case SC_SPLASHER:		/* �x�i���X�v���b�V���[ */
			break;
		case SC_GOSPEL:			/* �S�X�y�� */
			break;
		case SC_INCHIT:			/* HIT�㏸ */
			calc_flag = 1;
			break;
		case SC_INCFLEE:		/* FLEE�㏸ */
			calc_flag = 1;
			break;
		case SC_INCMHP2:		/* MHP%�㏸ */
			calc_flag = 1;
			break;
		case SC_INCMSP2:		/* MSP%�㏸ */
			calc_flag = 1;
			break;
		case SC_INCATK2:		/* ATK%�㏸ */
			calc_flag = 1;
			break;
		case SC_INCHIT2:		/* HIT%�㏸ */
			calc_flag = 1;
			break;
		case SC_INCFLEE2:		/* FLEE%�㏸ */
			calc_flag = 1;
			break;
		case SC_INCALLSTATUS:	/* �S�X�e�[�^�X�{20 */
			calc_flag = 1;
			break;
		case SC_PRESERVE:		/* �v���U�[�u */
			calc_flag = 1;
			break;
		case SC_OVERTHRUSTMAX:		/* �I�[�o�[�g���X�g�}�b�N�X */
			//if(sc_data[SC_OVERTHRUST].timer!=-1)
			//	status_change_end(bl,SC_OVERTHRUST,-1);
			calc_flag = 1;
			break;
			
		case SC_CHASEWALK_STR:			/* STR�㏸ */
			calc_flag = 1;
			break;
		case SC_BATTLEORDER://�Ր�Ԑ�
			calc_flag = 1;
			break;
		case SC_REGENERATION://����
			break;
			
		case SC_BATTLEORDER_DELAY:
		case SC_REGENERATION_DELAY:
		case SC_RESTORE_DELAY:
		case SC_EMERGENCYCALL_DELAY:
			break;
		case SC_THE_MAGICIAN:
		case SC_STRENGTH:
		case SC_THE_DEVIL:
		case SC_THE_SUN:
			calc_flag = 1;
			break;
		case SC_MEAL_INCSTR://�H���p
		case SC_MEAL_INCAGI:
		case SC_MEAL_INCVIT:
		case SC_MEAL_INCINT:
		case SC_MEAL_INCDEX:
		case SC_MEAL_INCLUK:
		case SC_SPURT://�삯���pSTR
			calc_flag = 1;
			break;
		case SC_SUN_COMFORT://#���z�̈��y#
		case SC_MOON_COMFORT://#���̈��y#
		case SC_STAR_COMFORT://#���̈��y#
		case SC_FUSION://#���z�ƌ��Ɛ��̗Z��#
			calc_flag = 1;
			break;
		case SC_RUN://�삯��
			break;
		case SC_TKCOMBO://�e�R���n�p�R���{
		case SC_SUN_WARM://#���z�̉�����#
		case SC_MOON_WARM://#���̉�����#
		case SC_STAR_WARM://#���̉�����#
		case SC_KAIZEL://#�J�C�[��#
		case SC_KAAHI://#�J�A�q#
		case SC_KAUPE://#�J�E�v#
		case SC_SMA://#�G�X�}#
			break;
		case SC_KAITE://#�J�C�g#
			/* �A�X�����|�����Ă������������ */
			if(sc_data[SC_ASSUMPTIO].timer!=-1)
				status_change_end(bl,SC_ASSUMPTIO,-1);
			//���ˉ�
			if(val1 == 7) val2 = 2;
			else val2 = 1;

			break;
		case SC_SWOO://#�G�X�E#
		case SC_SKE://#�G�X�N#
		case SC_SKA://#�G�X�J#
			if(bl->type!=BL_MOB)
				return 0;
			break;
		case SC_ALCHEMIST://#�A���P�~�X�g�̍�#
		case SC_MONK://#�����N�̍�#
		case SC_STAR://#�P���Z�C�̍�#
		case SC_SAGE://#�Z�[�W�̍�#
		case SC_CRUSADER://#�N���Z�C�_�[�̍�#
		case SC_KNIGHT://#�i�C�g�̍�#
		case SC_WIZARD://#�E�B�U�[�h�̍�#	
		case SC_PRIEST://#�v���[�X�g�̍�#
		case SC_BARDDANCER://#�o�[�h�ƃ_���T�[�̍�#
		case SC_ROGUE://#���[�O�̍�#
		case SC_ASSASIN://#�A�T�V���̍�#
		case SC_BLACKSMITH://#�u���b�N�X�~�X�̍�#
		case SC_HUNTER://#�n���^�[�̍�#
		case SC_SOULLINKER://#�\�E�������J�[�̍�#
		case SC_HIGH://#�ꎟ��ʐE�Ƃ̍�#
			if(battle_config.disp_job_soul_state_change && bl->type == BL_PC){
				char output[64];
				strcpy(output,"����ԂɂȂ�܂���");
				clif_disp_onlyself((struct map_session_data*)bl,output,strlen(output));
			}
			calc_flag = 1;
			break;
		case SC_SUPERNOVICE://#�X�[�p�[�m�[�r�X�̍�#
			if(sd && sd->status.base_level >=90 
				&& atn_rand()%10000 < battle_config.repeal_die_counter_rate)//1%�Ŏ��S�t���O�����H
				sd->repeal_die_counter = 1;
			if(battle_config.disp_job_soul_state_change && bl->type == BL_PC){
				char output[64];
				strcpy(output,"����ԂɂȂ�܂���");
				clif_disp_onlyself((struct map_session_data*)bl,output,strlen(output));
			}
			calc_flag = 1;
			break;
		
		case SC_AUTOBERSERK:
		case SC_READYSTORM:
		case SC_READYDOWN:
		case SC_READYTURN:
		case SC_READYCOUNTER:
		case SC_DODGE:
		case SC_DODGE_DELAY:
		case SC_DOUBLECASTING://�_�u���L���X�e�B���O
			break;
		case SC_DEVIL:
			break;
		default:
			if(battle_config.error_log)
				printf("UnknownStatusChange [%d]\n", type);
			return 0;
	}
	
	if(battle_config.debug_new_disp_status_icon_system)
	{
		if(bl->type==BL_PC && StatusIconChangeTable[type] != SI_BLANK)
			clif_status_change(bl,StatusIconChangeTable[type],1);	// �A�C�R���\��
	}else{
		
		if(bl->type==BL_PC && type<SC_SENDMAX)
			clif_status_change(bl,type,1);	/* �A�C�R���\�� */
		else if(bl->type==BL_PC && type>=SC_SENDMAX)
		{
			if(StatusIconChangeTable[type] != SI_BLANK)
				clif_status_change(bl,StatusIconChangeTable[type],1);	/* �A�C�R���\�� */
		}
	}
	
	/* option�̕ύX */
	switch(type){
		case SC_STONE:
		case SC_FREEZE:
		case SC_STAN:
		case SC_SLEEP:
			battle_stopattack(bl);	/* �U����~ */
			skill_stop_dancing(bl,0);	/* ���t/�_���X�̒��f */
			{	/* �����Ɋ|����Ȃ��X�e�[�^�X�ُ������ */
				int i;
				for(i = SC_STONE; i <= SC_SLEEP; i++){
					if(sc_data[i].timer != -1){
						(*sc_count)--;
						delete_timer(sc_data[i].timer, status_change_timer);
						sc_data[i].timer = -1;
					}
				}
			}
			if(type == SC_STONE)
				*opt1 = 6;
			else
				*opt1 = type - SC_STONE + 1;
			opt_flag = 1;
			break;
		case SC_POISON:
		case SC_CURSE:
		case SC_SILENCE:
		case SC_BLIND:
			*opt2 |= 1<<(type-SC_POISON);
			opt_flag = 1;
			break;
		case SC_DPOISON:	// �b��œł̃G�t�F�N�g���g�p
			*opt2 |= 1;
			opt_flag = 1;
			break;
		case SC_SIGNUMCRUCIS:
			*opt2 |= 0x40;
			opt_flag = 1;
			break;
		case SC_HIDING:
		case SC_CLOAKING:
			battle_stopattack(bl);	/* �U����~ */
			*option |= ((type==SC_HIDING)?2:4);
			opt_flag =1 ;
			break;
		case SC_CHASEWALK:
			battle_stopattack(bl);	/* �U?��~ */
			*option |= 16388;
			opt_flag =1 ;
			break;
		case SC_SIGHT:
			*option |= 1;
			opt_flag = 1;
			break;
		case SC_RUWACH:
			*option |= 8192;
			opt_flag = 1;
			break;
		case SC_WEDDING:
			*option |= 4096;
			opt_flag = 1;
			break;
	}

	if(opt_flag)	/* option�̕ύX */
		clif_changeoption(bl);

	(*sc_count)++;	/* �X�e�[�^�X�ُ�̐� */

	sc_data[type].val1 = val1;
	sc_data[type].val2 = val2;
	sc_data[type].val3 = val3;
	sc_data[type].val4 = val4;
	/* �^�C�}�[�ݒ� */
	sc_data[type].timer = add_timer(
		gettick() + tick, status_change_timer, bl->id, type);

	if(bl->type==BL_PC && calc_flag)
		status_calc_pc(sd,0);	/* �X�e�[�^�X�Čv�Z */

	if(bl->type==BL_PC && updateflag)
		clif_updatestatus(sd,updateflag);	/* �X�e�[�^�X���N���C�A���g�ɑ��� */

	return 0;
}
/*==========================================
 * �X�e�[�^�X�ُ�S����
 *------------------------------------------
 */
int skill_status_change_clear(struct block_list *bl,int type)
{
	struct status_change* sc_data;
	short *sc_count, *option, *opt1, *opt2, *opt3;
	int i;

	nullpo_retr(0, bl);
	nullpo_retr(0, sc_data=status_get_sc_data(bl));
	nullpo_retr(0, sc_count=status_get_sc_count(bl));
	nullpo_retr(0, option=status_get_option(bl));
	nullpo_retr(0, opt1=status_get_opt1(bl));
	nullpo_retr(0, opt2=status_get_opt2(bl));
	nullpo_retr(0, opt3=status_get_opt3(bl));

	if(*sc_count == 0)
		return 0;
	for(i = 0; i < MAX_STATUSCHANGE; i++){
		if(sc_data[i].timer != -1){	/* �ُ킪����Ȃ�^�C�}�[���폜���� */
/*
			delete_timer(sc_data[i].timer, status_change_timer);
			sc_data[i].timer = -1;

			if(!type && i<SC_SENDMAX)
				clif_status_change(bl,i,0);
*/

			status_change_end(bl,i,-1);
		}
	}
	*sc_count = 0;
	*opt1 = 0;
	*opt2 = 0;
	*opt3 = 0;
	*option &= OPTION_MASK;

	if(!type || type&2)
		clif_changeoption(bl);

	return 0;
}

/*==========================================
 * �X�e�[�^�X�ُ�I��
 *------------------------------------------
 */
int status_change_end( struct block_list* bl , int type,int tid )
{
	struct status_change* sc_data;
	int opt_flag=0, calc_flag = 0;
	short *sc_count, *option, *opt1, *opt2, *opt3;

	nullpo_retr(0, bl);
	if(bl->type!=BL_PC && bl->type!=BL_MOB) {
		if(battle_config.error_log)
			printf("status_change_end: neither MOB nor PC !\n");
		return 0;
	}
	nullpo_retr(0, sc_data=status_get_sc_data(bl));
	nullpo_retr(0, sc_count=status_get_sc_count(bl));
	nullpo_retr(0, option=status_get_option(bl));
	nullpo_retr(0, opt1=status_get_opt1(bl));
	nullpo_retr(0, opt2=status_get_opt2(bl));
	nullpo_retr(0, opt3=status_get_opt3(bl));
	
	if((*sc_count)>0 && sc_data[type].timer!=-1 &&
		(sc_data[type].timer==tid || tid==-1) ){

		if(tid==-1)	/* �^�C�}����Ă΂�Ă��Ȃ��Ȃ�^�C�}�폜������ */
			delete_timer(sc_data[type].timer,status_change_timer);

		/* �Y���ُ̈�𐳏�ɖ߂� */
		sc_data[type].timer=-1;
		(*sc_count)--;

		switch(type){	/* �ُ�̎�ނ��Ƃ̏��� */
			case SC_PROVOKE:			/* �v���{�b�N */
			case SC_ENDURE:				/* �C���f���A */
			case SC_CONCENTRATE:		/* �W���͌��� */
			case SC_BLESSING:			/* �u���b�V���O */
			case SC_ANGELUS:			/* �A���[���X */
			case SC_INCREASEAGI:		/* ���x�㏸ */
			case SC_DECREASEAGI:		/* ���x���� */
			case SC_SIGNUMCRUCIS:		/* �V�O�i���N���V�X */
			case SC_HIDING:
			case SC_CLOAKING:
			case SC_TWOHANDQUICKEN:		/* 2HQ */
			case SC_ONEHAND:			/* 1HQ */
			case SC_ADRENALINE:			/* �A�h���i�������b�V�� */
			case SC_ENCPOISON:			/* �G���`�����g�|�C�Y�� */
			case SC_IMPOSITIO:			/* �C���|�V�e�B�I�}�k�X */
			case SC_GLORIA:				/* �O�����A */
			case SC_LOUD:				/* ���E�h�{�C�X */
			case SC_MINDBREAKER:		/* �}�C���h�u���[�J�[ */
			case SC_QUAGMIRE:			/* �N�@�O�}�C�A */
			case SC_PROVIDENCE:			/* �v�����B�f���X */
			case SC_SPEARSQUICKEN:		/* �X�s�A�N�C�b�P�� */
			case SC_VOLCANO:
			case SC_DELUGE:
			case SC_VIOLENTGALE:
			case SC_ETERNALCHAOS:		/* �G�^�[�i���J�I�X */
			case SC_DRUMBATTLE:			/* �푾�ۂ̋��� */
			case SC_NIBELUNGEN:			/* �j�[�x�����O�̎w�� */
			case SC_SIEGFRIED:			/* �s���g�̃W�[�N�t���[�h */
			case SC_EXPLOSIONSPIRITS:	// �����g��
			case SC_STEELBODY:			// ����
			case SC_DEFENDER:
			case SC_POISONPOTION:		/* �Ŗ�̕r */
			case SC_SPEEDPOTION0:		/* �����|�[�V���� */
			case SC_SPEEDPOTION1:
			case SC_SPEEDPOTION2:
			case SC_RIDING:
			case SC_BLADESTOP_WAIT:
			case SC_CONCENTRATION:		/* �R���Z���g���[�V���� */
			case SC_ASSUMPTIO:			/* �A�V�����v�e�B�I */
			case SC_WINDWALK:		/* �E�C���h�E�H�[�N */
			case SC_TRUESIGHT:		/* �g�D���[�T�C�g */
			case SC_SPIDERWEB:		/* �X�p�C�_�[�E�F�b�u */
			case SC_INCATK:		//item 682�p
			case SC_INCMATK:	//item 683�p
			case SC_WEDDING:	//�����p(�����ߏւɂȂ��ĕ����̂��x���Ƃ�)
			case SC_INCALLSTATUS:
			case SC_INCHIT:
			case SC_INCFLEE:
			case SC_INCMHP2:
			case SC_INCMSP2:
			case SC_INCATK2:
			case SC_INCHIT2:
			case SC_INCFLEE2:
			case SC_PRESERVE:
			case SC_OVERTHRUSTMAX:
			case SC_CHASEWALK:	/*�`�F�C�X�E�H�[�N*/
			case SC_CHASEWALK_STR:
			case SC_BATTLEORDER:
			case SC_THE_MAGICIAN:
			case SC_STRENGTH:
			case SC_THE_DEVIL:
			case SC_THE_SUN:
			case SC_MEAL_INCSTR://�H���p
			case SC_MEAL_INCAGI:
			case SC_MEAL_INCVIT:
			case SC_MEAL_INCINT:
			case SC_MEAL_INCDEX:
			case SC_MEAL_INCLUK:
			case SC_SPURT:
				calc_flag = 1;
				break;
			case SC_RUN://�삯��
			case SC_SUN_WARM://#���z�̉�����#
			case SC_MOON_WARM://#���̉�����#
			case SC_STAR_WARM://#���̉�����#
			case SC_READYSTORM:
			case SC_READYDOWN:
			case SC_READYTURN:
			case SC_READYCOUNTER:
			case SC_DODGE:
			case SC_DODGE_DELAY:
			case SC_AUTOBERSERK:
				break;
			case SC_SUN_COMFORT://#���z�̈��y#
			case SC_MOON_COMFORT://#���̈��y#
			case SC_STAR_COMFORT://#���̈��y#
			case SC_FUSION://#���z�ƌ��Ɛ��̗Z��#
			case SC_ADRENALINE2://#�t���A�h���i�������b�V��#
				calc_flag = 1;
				break;
			case SC_KAIZEL://#�J�C�[��#
			case SC_KAAHI://#�J�A�q#
			case SC_KAUPE://#�J�E�v#
			case SC_KAITE://#�J�C�g#
			case SC_SMA://#�G�X�}#
			case SC_SWOO://#�G�X�E#
			case SC_SKE://#�G�X�N#
			case SC_SKA://#�G�X�J#
				break;
			case SC_ALCHEMIST://#�A���P�~�X�g�̍�#
			case SC_MONK://#�����N�̍�#
			case SC_STAR://#�P���Z�C�̍�#
			case SC_SAGE://#�Z�[�W�̍�#
			case SC_CRUSADER://#�N���Z�C�_�[�̍�#
			case SC_KNIGHT://#�i�C�g�̍�#
			case SC_WIZARD://#�E�B�U�[�h�̍�#	
			case SC_PRIEST://#�v���[�X�g�̍�#
			case SC_BARDDANCER://#�o�[�h�ƃ_���T�[�̍�#
			case SC_ROGUE://#���[�O�̍�#
			case SC_ASSASIN://#�A�T�V���̍�#
			case SC_BLACKSMITH://#�u���b�N�X�~�X�̍�#
			case SC_HUNTER://#�n���^�[�̍�#
			case SC_SOULLINKER://#�\�E�������J�[�̍�#
				if(battle_config.disp_job_soul_state_change && bl->type == BL_PC){
					char output[64];
					strcpy(output,"����Ԃ��I�����܂���");
					clif_disp_onlyself((struct map_session_data*)bl,output,strlen(output));
				}
				break;
			case SC_HIGH://#�ꎟ��ʐE�Ƃ̍�#
				if(battle_config.disp_job_soul_state_change && bl->type == BL_PC){
					char output[64];
					strcpy(output,"����Ԃ��I�����܂���");
					clif_disp_onlyself((struct map_session_data*)bl,output,strlen(output));
				}
				calc_flag = 1;
				break;
			case SC_SUPERNOVICE://#�X�[�p�[�m�[�r�X�̍�#
				if(battle_config.disp_job_soul_state_change && bl->type == BL_PC){
					char output[64];
					strcpy(output,"����Ԃ��I�����܂���");
					clif_disp_onlyself((struct map_session_data*)bl,output,strlen(output));
				}
				if(bl->type==BL_PC)
					((struct map_session_data*)bl)->repeal_die_counter = 0;
				calc_flag = 1;
				break;
			case SC_POEMBRAGI:			/* �u���M */
			case SC_WHISTLE:			/* ���J */
			case SC_ASSNCROS:			/* �[�z�̃A�T�V���N���X */
			case SC_APPLEIDUN:			/* �C�h�D���̗ь� */
			case SC_HUMMING:			/* �n�~���O */
			case SC_DONTFORGETME:		/* ����Y��Ȃ��� */
			case SC_FORTUNE:			/* �K�^�̃L�X */
			case SC_SERVICE4U:			/* �T�[�r�X�t�H�[���[ */
				calc_flag = 1;
				//�x�艉�t�����Z�b�g
				if(sc_data && sc_data[type + SC_WHISTLE_ - SC_WHISTLE].timer==-1)
					status_change_start(bl,type + SC_WHISTLE_ - SC_WHISTLE,sc_data[type].val1,
						sc_data[type].val2,sc_data[type].val3,sc_data[type].val4,battle_config.dance_and_play_duration,0);
				break;
			case SC_POEMBRAGI_:			/* �u���M */
			case SC_WHISTLE_:			/* ���J */
			case SC_ASSNCROS_:			/* �[�z�̃A�T�V���N���X */
			case SC_APPLEIDUN_:			/* �C�h�D���̗ь� */
			case SC_HUMMING_:			/* �n�~���O */
			case SC_DONTFORGETME_:		/* ����Y��Ȃ��� */
			case SC_FORTUNE_:			/* �K�^�̃L�X */
			case SC_SERVICE4U_:			/* �T�[�r�X�t�H�[���[ */
				calc_flag = 1;
				break;
			case SC_EDP:		// �G�t�F�N�g���������ꂽ��폜����
			{
			//	struct map_session_data *sd = (struct map_session_data *)bl;
			//	clif_displaymessage(sd->fd, " �ғő�������������܂���");
				break;
			}
			case SC_BERSERK:			/* �o�[�T�[�N */
				calc_flag = 1;
				clif_status_change(bl,SC_INCREASEAGI,0);	/* �A�C�R������ */
				break;
			case SC_DEVOTION:		/* �f�B�{�[�V���� */
				{
					struct map_session_data *md = map_id2sd(sc_data[type].val1);
					sc_data[type].val1=sc_data[type].val2=0;
					if (md)
						skill_devotion(md,bl->id);
					calc_flag = 1;
				}
				break;
			case SC_BLADESTOP:
				{
					struct status_change *t_sc_data = status_get_sc_data((struct block_list *)sc_data[type].val4);
					//�Е����؂ꂽ�̂ő���̔��n��Ԃ��؂�ĂȂ��̂Ȃ����
					if(t_sc_data && t_sc_data[SC_BLADESTOP].timer!=-1)
						status_change_end((struct block_list *)sc_data[type].val4,SC_BLADESTOP,-1);

					if(sc_data[type].val2==2)
						clif_bladestop((struct block_list *)sc_data[type].val3,(struct block_list *)sc_data[type].val4,0);
				}
				break;
			case SC_DANCING:
				{
					struct map_session_data *dsd;
					struct status_change *d_sc_data;
					if(sc_data[type].val4 && (dsd=map_id2sd(sc_data[type].val4))){
						d_sc_data = dsd->sc_data;
						//���t�ő��肪����ꍇ�����val4��0�ɂ���
						if(d_sc_data && d_sc_data[type].timer!=-1)
							d_sc_data[type].val4=0;
					}
				}
				calc_flag = 1;
				break;
			case SC_GRAFFITI:
				{
					struct skill_unit_group *sg=(struct skill_unit_group *)sc_data[type].val4;	//val4���O���t�B�e�B��group_id
					if(sg)
						skill_delunitgroup(sg);
				}
				break;
			case SC_NOCHAT:	//�`���b�g�֎~���
				{
					struct map_session_data *sd;
					if(bl->type == BL_PC && (sd=(struct map_session_data *)bl)){
//						sd->status.manner = 0;
						clif_updatestatus(sd,SP_MANNER);
					}
				}
				break;
			case SC_SPLASHER:		/* �x�i���X�v���b�V���[ */
				{
					struct block_list *src=map_id2bl(sc_data[type].val3);
					if(src && tid!=-1){
						//�����Ƀ_���[�W������3*3�Ƀ_���[�W
						skill_castend_damage_id(src, bl,sc_data[type].val2,sc_data[type].val1,gettick(),0 );
					}
				}
				break;
			case SC_SELFDESTRUCTION:		/* ���� */
				{
					//�����̃_���[�W��0�ɂ���
					struct mob_data *md;
					if(bl->type == BL_MOB && (md=(struct mob_data*)bl) && md->state.state != MS_DEAD) {
						skill_castend_damage_id(bl, bl,sc_data[type].val2,sc_data[type].val1,gettick(),0 );
						// ���̎��_��bl�͖����ɂȂ��Ă���̂ŁA�����ɖ߂�K�v������
						return 0;
					}
				}
				break;

		/* option1 */
			case SC_FREEZE:
				sc_data[type].val3 = 0;
				break;

		/* option2 */
			case SC_POISON:				/* �� */
			case SC_BLIND:				/* �Í� */
			case SC_CURSE:
				calc_flag = 1;
				break;
		}
		
		if(battle_config.debug_new_disp_status_icon_system)
		{
			if(bl->type==BL_PC && StatusIconChangeTable[type] != SI_BLANK)
			clif_status_change(bl,StatusIconChangeTable[type],0);	// �A�C�R������
		}
		else
		{
			if(bl->type==BL_PC && type<SC_SENDMAX)
				clif_status_change(bl,type,0);	/* �A�C�R������ */
			else if(bl->type==BL_PC && type>=SC_SENDMAX)
			{
				if(StatusIconChangeTable[type] != SI_BLANK)
					clif_status_change(bl,StatusIconChangeTable[type],0);	/* �A�C�R������ */
			}
		}

		switch(type){	/* ����ɖ߂�Ƃ��Ȃɂ��������K�v */
		case SC_STONE:
		case SC_FREEZE:
		case SC_STAN:
		case SC_SLEEP:
			*opt1 = 0;
			opt_flag = 1;
			break;

		case SC_POISON:
			if (sc_data[SC_DPOISON].timer != -1)	//
				break;						// DPOISON�p�̃I�v�V����
			*opt2 &= ~1;					// ����p�ɗp�ӂ��ꂽ�ꍇ�ɂ� 
			opt_flag = 1;					// �����͍폜���� 
			break;							//
		case SC_CURSE:
		case SC_SILENCE:
		case SC_BLIND:
			*opt2 &= ~(1<<(type-SC_POISON));
			opt_flag = 1;
			break;
		case SC_DPOISON:
			if (sc_data[SC_POISON].timer != -1)	// DPOISON�p�̃I�v�V������	
				break;							// �p�ӂ��ꂽ��폜
			*opt2 &= ~1;	// �ŏ�ԉ���
			opt_flag = 1;
			break;
		case SC_SIGNUMCRUCIS:
			*opt2 &= ~0x40;
			opt_flag = 1;
			break;

		case SC_HIDING:
		case SC_CLOAKING:
			*option &= ~((type==SC_HIDING)?2:4);
			opt_flag = 1 ;
			break;
		case SC_CHASEWALK:	/*�`�F�C�X�E�H�[�N*/
			*option &= ~16388;
			opt_flag = 1 ;
			break;

		case SC_SIGHT:
			*option &= ~1;
			opt_flag = 1;
			break;
		case SC_WEDDING:	//�����p(�����ߏւɂȂ��ĕ����̂��x���Ƃ�)
			{
				time_t timer;
				struct map_session_data *sd = (struct map_session_data *)bl;

				if( time(&timer) >= ((sc_data[type].val2) + battle_config.wedding_time) ){	//1���Ԍo��
					if( bl->type == BL_PC ){
						pc_setglobalreg(sd,"PC_WEDDING_TIME",0);
					}

					*option &= ~4096;
					opt_flag = 1;
				}
			}
			break;
		case SC_RUWACH:
			*option &= ~8192;
			opt_flag = 1;
			break;

		//opt3
		case SC_ONEHAND:			/* 1HQ */
		case SC_TWOHANDQUICKEN:		/* 2HQ */
		case SC_SPEARSQUICKEN:		/* �X�s�A�N�C�b�P�� */
		case SC_CONCENTRATION:		/* �R���Z���g���[�V���� */
			*opt3 &= ~1;
			break;
		case SC_OVERTHRUST:			/* �I�[�o�[�g���X�g */
			*opt3 &= ~2;
			break;
		case SC_ENERGYCOAT:			/* �G�i�W�[�R�[�g */
			*opt3 &= ~4;
			break;
		case SC_EXPLOSIONSPIRITS:	// �����g��
			*opt3 &= ~8;
			break;
		case SC_STEELBODY:			// ����
			*opt3 &= ~16;
			break;
		case SC_BLADESTOP:		/* ���n��� */
			*opt3 &= ~32;
			break;
		case SC_BERSERK:		/* �o�[�T�[�N */
			*opt3 &= ~128;
			break;
		case SC_MARIONETTE:		/* �}���I�l�b�g�R���g���[�� */
			*opt3 &= ~1024;
			break;
		case SC_ASSUMPTIO:		/* �A�X���v�e�B�I */
			*opt3 &= ~2048;
			break;
		}

		if(opt_flag)	/* option�̕ύX��`���� */
			clif_changeoption(bl);

		if(bl->type==BL_PC && calc_flag)
			status_calc_pc((struct map_session_data *)bl,0);	/* �X�e�[�^�X�Čv�Z */
	}

	return 0;
}


/*==========================================
 * �X�e�[�^�X�ُ�I���^�C�}�[
 *------------------------------------------
 */
int status_change_timer(int tid, unsigned int tick, int id, int data)
{
	int type=data;
	struct block_list *bl;
	struct map_session_data *sd=NULL;
	struct status_change *sc_data;
	//short *sc_count; //�g���ĂȂ��H

	if( (bl=map_id2bl(id)) == NULL )
		return 0; //�Y��ID�����łɏ��ł��Ă���Ƃ����̂͂����ɂ����肻���Ȃ̂ŃX���[���Ă݂�
	nullpo_retr(0, sc_data=status_get_sc_data(bl));

	if(bl->type==BL_PC)
		sd=(struct map_session_data *)bl;

	//sc_count=status_get_sc_count(bl); //�g���ĂȂ��H

	if(sc_data[type].timer != tid) {
		if(battle_config.error_log)
			printf("status_change_timer %d != %d\n",tid,sc_data[type].timer);
		return 0;
	}


	switch(type){	/* ����ȏ����ɂȂ�ꍇ */
	case SC_MAXIMIZEPOWER:	/* �}�L�V�}�C�Y�p���[ */
		if(sd){
			if( sd->status.sp > 0 ){	/* SP�؂��܂Ŏ��� */
				sd->status.sp--;
				clif_updatestatus(sd,SP_SP);
				sc_data[type].timer=add_timer(	/* �^�C�}�[�Đݒ� */
					sc_data[type].val2+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
		}
		break;
	case SC_CLOAKING:		/* �N���[�L���O */
		if(sd){
			if( sd->status.sp > 0 ){	/* SP�؂��܂Ŏ��� */
				sd->status.sp--;
				clif_updatestatus(sd,SP_SP);
				sc_data[type].timer=add_timer(	/* �^�C�}�[�Đݒ� */
					sc_data[type].val2+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
		}
		break;

	case SC_CHASEWALK:	/*�`�F�C�X�E�H�[�N*/
		if(sd){
			int sp = 10+sc_data[SC_CHASEWALK].val1*2;
			if (map[sd->bl.m].flag.gvg) sp *= 5;
			if (sd->status.sp > sp){
				sd->status.sp -= sp; // update sp cost [Celest]
				clif_updatestatus(sd,SP_SP);
				if ((++sc_data[SC_CHASEWALK].val4) == 1) {
					
					//���[�O�̍�
					if(sd->sc_data[SC_ROGUE].timer!=-1)
						status_change_start(bl, SC_CHASEWALK_STR, 1<<(sc_data[SC_CHASEWALK].val1-1), 0, 0, 0, 300000, 0);
					else status_change_start(bl, SC_CHASEWALK_STR, 1<<(sc_data[SC_CHASEWALK].val1-1), 0, 0, 0, 30000, 0);
					status_calc_pc(sd, 0);
				}
				sc_data[type].timer = add_timer( /* �^�C�}?�Đݒ� */
					sc_data[type].val2+tick, status_change_timer, bl->id, data);
				return 0;
			}
		}
	break;

	case SC_HIDING:		/* �n�C�f�B���O */
		if(sd){		/* SP�������āA���Ԑ����̊Ԃ͎��� */
			if( sd->status.sp > 0 && (--sc_data[type].val2)>0 ){
				if(sc_data[type].val2 % (sc_data[type].val1+3) ==0 ){
					sd->status.sp--;
					clif_updatestatus(sd,SP_SP);
				}
				sc_data[type].timer=add_timer(	/* �^�C�}�[�Đݒ� */
					1000+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
		}
		break;

	case SC_SIGHT:	/* �T�C�g */
	case SC_RUWACH:	/* ���A�t */
		{
			//const int range=10; �C���O�͗���10x10�������̂ł�
			// �X�L���C��: �T�C�g�͈̔�=7x7 ���A�t�͈̔�=5x5
			int range = 2;
			if ( type == SC_SIGHT ) range = 3;

			map_foreachinarea( status_change_timer_sub,
				bl->m, bl->x-range, bl->y-range, bl->x+range,bl->y+range,0,
				bl,type,tick);

			if( (--sc_data[type].val2)>0 ){
				sc_data[type].timer=add_timer(	/* �^�C�}�[�Đݒ� */
					250+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
		}
		break;

	case SC_SIGNUMCRUCIS:		/* �V�O�i���N���V�X */
		{
			int race = status_get_race(bl);
			if(race == 6 || battle_check_undead(race,status_get_elem_type(bl))) {
				sc_data[type].timer=add_timer(1000*600+tick,status_change_timer, bl->id, data );
				return 0;
			}
		}
		break;

	case SC_PROVOKE:	/* �v���{�b�N/�I�[�g�o�[�T�[�N */
		if(sc_data[type].val2!=0){	/* �I�[�g�o�[�T�[�N�i�P�b���Ƃ�HP�`�F�b�N�j */
			if(sd && sd->status.hp>sd->status.max_hp>>2)	/* ��~ */
				break;
			sc_data[type].timer=add_timer( 1000+tick,status_change_timer, bl->id, data );
			return 0;
		}
		break;

	case SC_DISSONANCE:	/* �s���a�� */
		if( (--sc_data[type].val2)>0){
			struct skill_unit *unit=
				(struct skill_unit *)sc_data[type].val4;
			struct block_list *src;

			if(!unit || !unit->group)
				break;
			src=map_id2bl(unit->group->src_id);
			if(!src)
				break;
			skill_attack(BF_MISC,src,&unit->bl,bl,unit->group->skill_id,sc_data[type].val1,tick,0);
			sc_data[type].timer=add_timer(skill_get_time2(unit->group->skill_id,unit->group->skill_lv)+tick,
				status_change_timer, bl->id, data );
			return 0;
		}
		break;

	case SC_LULLABY:	/* �q��S */
		if( (--sc_data[type].val2)>0){
			struct skill_unit *unit=
				(struct skill_unit *)sc_data[type].val4;
			if(!unit || !unit->group || unit->group->src_id==bl->id)
				break;
			skill_additional_effect(bl,bl,unit->group->skill_id,sc_data[type].val1,BF_LONG|BF_SKILL|BF_MISC,tick);
			sc_data[type].timer=add_timer(skill_get_time(unit->group->skill_id,unit->group->skill_lv)/10+tick,
				status_change_timer, bl->id, data );
			return 0;
		}
		break;

	case SC_STONE:
		if(sc_data[type].val2 != 0) {
			short *opt1 = status_get_opt1(bl);
			sc_data[type].val2 = 0;
			sc_data[type].val4 = 0;
			battle_stopwalking(bl,1);
			if(opt1) {
				*opt1 = 1;
				clif_changeoption(bl);
			}
			sc_data[type].timer=add_timer(1000+tick,status_change_timer, bl->id, data );
			return 0;
		}
		else if( (--sc_data[type].val3) > 0) {
			int hp = status_get_max_hp(bl);
			if((++sc_data[type].val4)%5 == 0 && status_get_hp(bl) > hp>>2) {
				hp = hp/100;
				if(hp < 1) hp = 1;
				if(bl->type == BL_PC)
					pc_heal((struct map_session_data *)bl,-hp,0);
				else if(bl->type == BL_MOB){
					struct mob_data *md;
					if((md=((struct mob_data *)bl)) == NULL)
						break;
					md->hp -= hp;
				}
			}
			sc_data[type].timer=add_timer(1000+tick,status_change_timer, bl->id, data );
			return 0;
		}
		break;
	case SC_POISON:
		if (sc_data[SC_SLOWPOISON].timer == -1 && (--sc_data[type].val3) > 0) {
			int hp = status_get_max_hp(bl);
			if (status_get_hp(bl) > hp>>2) {
				if(bl->type == BL_PC) {
					hp = 3 + hp*3/200;
					pc_heal((struct map_session_data *)bl,-hp,0);
				} else if (bl->type == BL_MOB) {
					struct mob_data *md;
					if ((md=((struct mob_data *)bl)) == NULL)
						break;
					hp = 3 + hp/200;
					md->hp -= hp;
				}
			}
		}
		if (sc_data[type].val3 > 0)
			sc_data[type].timer=add_timer(1000+tick,status_change_timer, bl->id, data );
		break;
	case SC_DPOISON:
		if (sc_data[SC_SLOWPOISON].timer == -1 && (--sc_data[type].val3) > 0) {
			int hp = status_get_max_hp(bl);
			if (status_get_hp(bl) > hp>>2) {
				if(bl->type == BL_PC) {
					hp = 3 + hp/50;
					pc_heal((struct map_session_data *)bl, -hp, 0);
				} else if (bl->type == BL_MOB) {
					struct mob_data *md;
					if ((md=((struct mob_data *)bl)) == NULL)
						break;
					hp = 3 + hp/100;
					md->hp -= hp;
				}
			}
		}
		if (sc_data[type].val3 > 0)
			sc_data[type].timer=add_timer(1000+tick,status_change_timer, bl->id, data );
		break;
	case SC_TENSIONRELAX:	/* �e���V���������b�N�X */
		if(sd){		/* SP�������āAHP�����^���łȂ���Όp�� */
			if( sd->status.sp > 12 && sd->status.max_hp > sd->status.hp ){
				if(sc_data[type].val2 % (sc_data[type].val1+3) ==0 ){
					sd->status.sp -= 12;
					clif_updatestatus(sd,SP_SP);
				}
				sc_data[type].timer=add_timer(	/* �^�C�}�[�Đݒ� */
					10000+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
			if(sd->status.max_hp <= sd->status.hp)
				status_change_end(&sd->bl,SC_TENSIONRELAX,-1);
		}
		break;

	/* ���Ԑ؂ꖳ���H�H */
	case SC_AETERNA:
	case SC_TRICKDEAD:
	case SC_RIDING:
	case SC_FALCON:
	case SC_WEIGHT50:
	case SC_WEIGHT90:
	case SC_MAGICPOWER:		/* ���@�͑��� */
	case SC_REJECTSWORD:	/* ���W�F�N�g�\�[�h */
	case SC_MEMORIZE:		/* �������C�Y */
	case SC_SACRIFICE:		/* �T�N���t�@�C�X */
	case SC_READYSTORM:
	case SC_READYDOWN:
	case SC_READYTURN:
	case SC_READYCOUNTER:
	case SC_DODGE:
	case SC_AUTOBERSERK:
		sc_data[type].timer=add_timer( 1000*600+tick,status_change_timer, bl->id, data );
		return 0;
	case SC_DEVIL:
		clif_status_change(bl,SI_DEVIL,1);
		sc_data[type].timer=add_timer( 1000*5+tick,status_change_timer, bl->id, data );
	case SC_DANCING: //�_���X�X�L���̎���SP����
		{
			int s=0;
			if(sd){
				if(sd->status.sp > 0 && (--sc_data[type].val3)>0){
					switch(sc_data[type].val1){
					case BD_RICHMANKIM:				/* �j�����h�̉� 3�b��SP1 */
					case BD_DRUMBATTLEFIELD:		/* �푾�ۂ̋��� 3�b��SP1 */
					case BD_RINGNIBELUNGEN:			/* �j�[�x�����O�̎w�� 3�b��SP1 */
					case BD_SIEGFRIED:				/* �s���g�̃W�[�N�t���[�h 3�b��SP1 */
					case BA_DISSONANCE:				/* �s���a�� 3�b��SP1 */
					case BA_ASSASSINCROSS:			/* �[�z�̃A�T�V���N���X 3�b��SP1 */
					case DC_UGLYDANCE:				/* ��������ȃ_���X 3�b��SP1 */
						s=3;
						break;
					case BD_LULLABY:				/* �q��� 4�b��SP1 */
					case BD_ETERNALCHAOS:			/* �i���̍��� 4�b��SP1 */
					case BD_ROKISWEIL:				/* ���L�̋��� 4�b��SP1 */
					case DC_FORTUNEKISS:			/* �K�^�̃L�X 4�b��SP1 */
						s=4;
						break;
					case BD_INTOABYSS:				/* �[���̒��� 5�b��SP1 */
					case BA_WHISTLE:				/* ���J 5�b��SP1 */
					case DC_HUMMING:				/* �n�~���O 5�b��SP1 */
					case BA_POEMBRAGI:				/* �u���M�̎� 5�b��SP1 */
					case DC_SERVICEFORYOU:			/* �T�[�r�X�t�H�[���[ 5�b��SP1 */
						s=5;
						break;
					case BA_APPLEIDUN:				/* �C�h�D���̗ь� 6�b��SP1 */
						s=6;
						break;
					case DC_DONTFORGETME:			/* ����Y��Ȃ��Łc 10�b��SP1 */
					case CG_MOONLIT:				/* ������̐�ɗ�����Ԃт� 10�b��SP1�H */
						s=10;
						break;
					}
					if(s && ((sc_data[type].val3 % s) == 0)){
						sd->status.sp--;
						clif_updatestatus(sd,SP_SP);
					}
					sc_data[type].timer=add_timer(	/* �^�C�}�[�Đݒ� */
						1000+tick, status_change_timer,
						bl->id, data);
					return 0;
				}
			}
		}
		break;
	case SC_BERSERK:		/* �o�[�T�[�N */
		if(sd){		/* HP��100�ȏ�Ȃ�p�� */
			if( (sd->status.hp - (sd->status.max_hp*5)/100) > 100 ){
				sd->status.hp -= (sd->status.max_hp*5)/100;
				clif_updatestatus(sd,SP_HP);
				sc_data[type].timer=add_timer(	/* �^�C�}�[�Đݒ� */
					10000+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
			else
				sd->status.hp = 100;
		}
		break;
	case SC_WEDDING:	//�����p(�����ߏւɂȂ��ĕ����̂��x���Ƃ�)
		if(sd){
			time_t timer;
			if(time(&timer) < ((sc_data[type].val2) + battle_config.wedding_time)){	//1���Ԃ����Ă��Ȃ��̂Ōp��
				sc_data[type].timer=add_timer(	/* �^�C�}�[�Đݒ� */
					10000+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
		}
		break;
	case SC_NOCHAT:	//�`���b�g�֎~���
		if(sd){
			time_t timer;
			if((++sd->status.manner) && time(&timer) < ((sc_data[type].val2) + 60*(0-sd->status.manner))){	//�J�n����status.manner���o���ĂȂ��̂Ōp��
				clif_updatestatus(sd,SP_MANNER);
				sc_data[type].timer=add_timer(	/* �^�C�}�[�Đݒ�(60�b) */
					60000+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
		}
		break;
	case SC_SELFDESTRUCTION:		/* ���� */
		if(--sc_data[type].val3>0){
			struct mob_data *md;
			if(bl->type==BL_MOB && (md=(struct mob_data *)bl) && md->speed > 250){
				md->speed -= 250;
				md->next_walktime=tick;
			}
			sc_data[type].timer=add_timer(	/* �^�C�}�[�Đݒ� */
				1000+tick, status_change_timer,
				bl->id, data);
				return 0;
		}
		break;
	}

	return status_change_end( bl,type,tid );
}

/*==========================================
 * �X�e�[�^�X�ُ�^�C�}�[�͈͏���
 *------------------------------------------
 */
int status_change_timer_sub(struct block_list *bl, va_list ap )
{
	struct block_list *src;
	int type;
	unsigned int tick;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, src=va_arg(ap,struct block_list*));
	type=va_arg(ap,int);
	tick=va_arg(ap,unsigned int);

	if(bl->type!=BL_PC && bl->type!=BL_MOB)
		return 0;

	switch( type ){
	case SC_SIGHT:	/* �T�C�g */
	case SC_CONCENTRATE:
		if( (*status_get_option(bl))&6 ){
			status_change_end( bl, SC_HIDING, -1);
			status_change_end( bl, SC_CLOAKING, -1);
		}
		break;
	case SC_RUWACH:	/* ���A�t */
		if( (*status_get_option(bl))&6 ){
			status_change_end( bl, SC_HIDING, -1);
			status_change_end( bl, SC_CLOAKING, -1);
			if(battle_check_target( src,bl, BCT_ENEMY ) > 0) {
				struct status_change *sc_data = status_get_sc_data(bl);
				skill_attack(BF_MAGIC,src,src,bl,AL_RUWACH,sc_data[type].val1,tick,0);
			}
		}
		break;
	}
	return 0;
}


static int status_calc_sigma(void)
{
	int i,j,k;

	for(i=0;i<MAX_PC_CLASS;i++) {
		memset(hp_sigma_val[i],0,sizeof(hp_sigma_val[i]));
		for(k=0,j=2;j<=MAX_LEVEL;j++) {
			k += hp_coefficient[i]*j + 50;
			k -= k%100;
			hp_sigma_val[i][j-1] = k;
		}
	}
	return 0;
}

int status_readdb(void) {
	int i,j,k;
	FILE *fp;
	char line[1024],*p;

	// JOB�␳���l�P
	fp=fopen("db/job_db1.txt","r");
	if(fp==NULL){
		printf("can't read db/job_db1.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line,1020,fp)){
		char *split[50];
		if(line[0]=='/' && line[1]=='/')
			continue;
		for(j=0,p=line;j<21 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(j<21)
			continue;
		max_weight_base[i]=atoi(split[0]);
		hp_coefficient[i]=atoi(split[1]);
		hp_coefficient2[i]=atoi(split[2]);
		sp_coefficient[i]=atoi(split[3]);
		for(j=0;j<17;j++)
			aspd_base[i][j]=atoi(split[j+4]);
		i++;
		if(i==MAX_VALID_PC_CLASS)
			break;
	}
	fclose(fp);
	printf("read db/job_db1.txt done\n");

	// JOB�{�[�i�X
	fp=fopen("db/job_db2.txt","r");
	if(fp==NULL){
		printf("can't read db/job_db2.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line,1020,fp)){
		if(line[0]=='/' && line[1]=='/')
			continue;
		for(j=0,p=line;j<MAX_LEVEL && p;j++){
			if(sscanf(p,"%d",&k)==0)
				break;
			job_bonus[0][i][j]=k;
			job_bonus[2][i][j]=k; //�{�q�E�̃{�[�i�X�͕�����Ȃ��̂ŉ�
			p=strchr(p,',');
			if(p) p++;
		}
		i++;
		if(i==MAX_VALID_PC_CLASS)
			break;
	}
	fclose(fp);
	printf("read db/job_db2.txt done\n");

	// JOB�{�[�i�X2 �]���E�p
	fp=fopen("db/job_db2-2.txt","r");
	if(fp==NULL){
		printf("can't read db/job_db2-2.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line,1020,fp)){
		if(line[0]=='/' && line[1]=='/')
			continue;
		for(j=0,p=line;j<MAX_LEVEL && p;j++){
			if(sscanf(p,"%d",&k)==0)
				break;
			job_bonus[1][i][j]=k;
			p=strchr(p,',');
			if(p) p++;
		}
		i++;
		if(i==MAX_VALID_PC_CLASS)
			break;
	}
	fclose(fp);
	printf("read db/job_db2-2.txt done\n");

	// ���B�f�[�^�e�[�u��
	for(i=0;i<5;i++){
		for(j=0;j<10;j++)
			percentrefinery[i][j]=100;
		refinebonus[i][0]=0;
		refinebonus[i][1]=0;
		refinebonus[i][2]=10;
	}
	fp=fopen("db/refine_db.txt","r");
	if(fp==NULL){
		printf("can't read db/refine_db.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line,1020,fp)){
		char *split[16];
		if(line[0]=='/' && line[1]=='/')
			continue;
		if(atoi(line)<=0)
			continue;
		memset(split,0,sizeof(split));
		for(j=0,p=line;j<16 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		refinebonus[i][0]=atoi(split[0]);	// ���B�{�[�i�X
		refinebonus[i][1]=atoi(split[1]);	// �ߏ萸�B�{�[�i�X
		refinebonus[i][2]=atoi(split[2]);	// ���S���B���E
		for(j=0;j<10 && split[j];j++)
			percentrefinery[i][j]=atoi(split[j+3]);
		i++;
	}
	fclose(fp);
		printf("read db/refine_db.txt done\n");

	// �T�C�Y�␳�e�[�u��
	for(i=0;i<3;i++)
		for(j=0;j<20;j++)
			atkmods[i][j]=100;
	fp=fopen("db/size_fix.txt","r");
	if(fp==NULL){
		printf("can't read db/size_fix.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line,1020,fp)){
		char *split[20];
		if(line[0]=='/' && line[1]=='/')
			continue;
		if(atoi(line)<=0)
			continue;
		memset(split,0,sizeof(split));
		for(j=0,p=line;j<20 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		for(j=0;j<20 && split[j];j++)
			atkmods[i][j]=atoi(split[j]);
		i++;
	}
	fclose(fp);
	printf("read db/size_fix.txt done\n");

	return 0;
}

/*==========================================
 * �X�L���֌W����������
 *------------------------------------------
 */
int do_init_status(void)
{
	add_timer_func_list(status_change_timer,"status_change_timer");
	status_readdb();
	status_calc_sigma();
	return 0;
}
