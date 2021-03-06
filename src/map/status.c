// Xe[^XvZAóÔÙí

#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include "pc.h"
#include "map.h"
#include "pet.h"
#include "homun.h"
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
#include "unit.h"
#include "db.h"
#include "malloc.h"
static int max_weight_base[MAX_PC_CLASS];
static int hp_coefficient[MAX_PC_CLASS];
static int hp_coefficient2[MAX_PC_CLASS];
static int hp_sigma_val[MAX_PC_CLASS][MAX_LEVEL];
static int sp_coefficient[MAX_PC_CLASS];
static int aspd_base[MAX_PC_CLASS][30];
static int refinebonus[5][3];	// ¸B{[iXe[u(refine_db.txt)
static int percentrefinery[5][10];	// ¸B¬÷¦(refine_db.txt)
static int atkmods[3][30];	// íATKTCYC³(size_fix.txt)
static char job_bonus[3][MAX_PC_CLASS][MAX_LEVEL];
int current_equip_item_index;//Xe[^XvZp
int current_equip_card_id;
static char race_name[11][5] = {{"³`"},{"s"},{"®¨"},{"A¨"},{"©"},{""},{"L"},{"«"},{"lÔ"},{"Vg"},{"³"}};
struct status_change dummy_sc_data[MAX_STATUSCHANGE];
/*==========================================
 * ¸B{[iX
 *------------------------------------------
 */
int status_getrefinebonus(int lv,int type)
{
	if(lv >= 0 && lv < 5 && type >= 0 && type < 3)
		return refinebonus[lv][type];
	return 0;
}

/*==========================================
 * ¸B¬÷¦
 *------------------------------------------
 */
int status_percentrefinery(struct map_session_data *sd,struct item *item)
{
	int percent;

	nullpo_retr(0, item);
	percent=percentrefinery[itemdb_wlv(item->nameid)][(int)item->refine];

	percent += pc_checkskill(sd,BS_WEAPONRESEARCH);	// í¤XL

	// m¦ÌLøÍÍ`FbN
	if( percent > 100 ){
		percent = 100;
	}
	if( percent < 0 ){
		percent = 0;
	}

	return percent;
}

/*==========================================
 * ¸B¬÷¦ ª¦
 *------------------------------------------
 */
int status_percentrefinery_weaponrefine(struct map_session_data *sd,struct item *item)
{
	int percent;
	int joblv;

	nullpo_retr(0, sd);
	nullpo_retr(0, item);
	joblv = sd->status.job_level > 70? 70 : sd->status.job_level;

	percent = percentrefinery[itemdb_wlv(item->nameid)][(int)item->refine]*100 + (joblv - 50)*50;

	if(battle_config.allow_weaponrearch_to_weaponrefine)
		percent += pc_checkskill(sd,BS_WEAPONRESEARCH)*100;	// í¤XL

	// m¦ÌLøÍÍ`FbN
	if( percent > 10000 ){
		percent = 10000;
	}
	if( percent < 0 ){
		percent = 0;
	}

	return percent;
}

/*==========================================
 * p[^vZ
 * first==0ÌAvZÎÛÌp[^ªÄÑoµO©ç
 * Ï »µ½ê©®Åsend·éªA
 * \®IÉÏ»³¹½p[^Í©OÅsend·éæ¤É
 *------------------------------------------
 */

int status_calc_pc(struct map_session_data* sd,int first)
{
	// ÓF±±ÅÍÏÌé¾ÌÝÉÆÇßAú»ÍL_RECALCÌãÉâé±ÆB
	int b_speed,b_max_hp,b_max_sp,b_hp,b_sp,b_weight,b_max_weight,b_paramb[6],b_parame[6],b_hit,b_flee;
	int b_aspd,b_watk,b_def,b_watk2,b_def2,b_flee2,b_critical,b_attackrange,b_matk1,b_matk2,b_mdef,b_mdef2,b_class;
	int b_base_atk;
	struct skill b_skill[MAX_SKILL];
	int i,bl,index;
	int skill,aspd_rate,wele,wele_,def_ele,refinedef;
	int pele,pdef_ele;
	int str,dstr,dex;
	struct guild *g;
	struct map_session_data* gmsd;
	struct pc_base_job s_class;
	int    calclimit = 2; // ñÍuse scriptÝÅÀs

	nullpo_retr(0, sd);

	if(sd->stop_status_calc_pc)
	{
		sd->call_status_calc_pc_while_stopping++;
		return 0;
	}

	sd->call_status_calc_pc_while_stopping = 0;

	// status_calc_pc ÌÉstatus_calc_pcªÄÑo³êéÆÅÉvZµÄ¢é
	// lª¶¤Â\«ª éBÜ½A±ÌÖªÄÎêéÆ¢¤±ÆÍALÌóÔª
	// Ï»µÄ¢é±ÆðÃ¦µÄ¢éÌÅAvZÊðÌÄÄÄvZµÈ¯êÎ¢¯È¢B
	// ÖÌI¹_ÅÄÑoµª êÎAL_RECALCÉòÎ·æ¤É·éB
	if( sd->status_calc_pc_process++ ) return 0;

	// ÈOÌóÔÌÛ¶
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
	b_base_atk = sd->base_atk;

L_RECALC:
	// {ÌvZJn(³Ìp[^ðXVµÈ¢ÌÍAvZÉvZªÄÎê½Æ«Ì
	// ½fªàV½ÉM·é½ß)B

	pele=0;
	pdef_ele=0;
	refinedef=0;
	sd->view_class = sd->status.class;
	if(sd->view_class == PC_CLASS_GS || sd->view_class==PC_CLASS_NJ)
		sd->view_class = sd->view_class-4;

	//]¶â{qÌêÌ³ÌEÆðZo·é
	s_class = pc_calc_base_job(sd->status.class);
	//Mhæ¾
	g = (struct guild *)guild_search(sd->status.guild_id);
	if(g)
		gmsd = (struct map_session_data*)guild_get_guildmaster_sd(g);
	else
		gmsd = NULL;

	sd->race = 7;
	sd->ranker_weapon_bonus  = 0;
	sd->ranker_weapon_bonus_ = 0;
	sd->infinite_tigereye    = 0;

	pc_calc_skilltree(sd);	// XLc[ÌvZ

	sd->max_weight = max_weight_base[s_class.job]+sd->status.str*300;

	if(battle_config.baby_weight_rate !=100 && pc_isbaby(sd))
		sd->max_weight = sd->max_weight*battle_config.baby_weight_rate/100;


//yRRæ¦éæ¤Ú®
	if( (skill=pc_checkskill(sd,MC_INCCARRY))>0)	// ÊÁ
		sd->max_weight += skill*2000;

	if( (skill=pc_checkskill(sd,SG_KNOWLEDGE))>0)// ¾zÆÆ¯Ìm¯
	{
	 	if(battle_config.check_knowlege_map)//ê`FbNðsÈ¤
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
	memset(sd->addenemy,0,sizeof(sd->addenemy));
	memset(sd->addsize,0,sizeof(sd->addsize));
	memset(sd->addele_,0,sizeof(sd->addele_));
	memset(sd->addrace_,0,sizeof(sd->addrace_));
	memset(sd->addenemy_,0,sizeof(sd->addenemy_));
	memset(sd->addsize_,0,sizeof(sd->addsize_));
	memset(sd->subele,0,sizeof(sd->subele));
	memset(sd->subrace,0,sizeof(sd->subrace));
	memset(sd->subenemy,0,sizeof(sd->subenemy));
	memset(sd->addeff,0,sizeof(sd->addeff));
	memset(sd->addeff2,0,sizeof(sd->addeff2));
	memset(sd->reseff,0,sizeof(sd->reseff));
	memset(sd->addeff_range_flag,0,sizeof(sd->addeff));
	memset(&sd->special_state,0,sizeof(sd->special_state));
	memset(sd->weapon_coma_ele,0,sizeof(sd->weapon_coma_ele));
	memset(sd->weapon_coma_race,0,sizeof(sd->weapon_coma_race));
	memset(sd->weapon_coma_ele2,0,sizeof(sd->weapon_coma_ele2));
	memset(sd->weapon_coma_race2,0,sizeof(sd->weapon_coma_race2));
	memset(sd->weapon_atk,0,sizeof(sd->weapon_atk));
	memset(sd->weapon_atk_rate,0,sizeof(sd->weapon_atk_rate));
	memset(sd->auto_status_calc_pc,0,sizeof(sd->auto_status_calc_pc));
	memset(sd->eternal_status_change,0,sizeof(sd->eternal_status_change));

	sd->watk_ = 0;			//ñ¬p(¼)
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
	sd->ignore_def_ele = sd->ignore_def_race = sd->ignore_def_enemy = 0;
	sd->ignore_def_ele_ = sd->ignore_def_race_ = sd->ignore_def_enemy_ = 0;
	sd->ignore_mdef_ele = sd->ignore_mdef_race = sd->ignore_mdef_enemy = 0;
	sd->arrow_cri = 0;
	sd->magic_def_rate = sd->misc_def_rate = 0;
	memset(sd->arrow_addele,0,sizeof(sd->arrow_addele));
	memset(sd->arrow_addrace,0,sizeof(sd->arrow_addrace));
	memset(sd->arrow_addenemy,0,sizeof(sd->arrow_addenemy));
	memset(sd->arrow_addsize,0,sizeof(sd->arrow_addsize));
	memset(sd->arrow_addeff,0,sizeof(sd->arrow_addeff));
	memset(sd->arrow_addeff2,0,sizeof(sd->arrow_addeff2));
	memset(sd->magic_addele,0,sizeof(sd->magic_addele));
	memset(sd->magic_addrace,0,sizeof(sd->magic_addrace));
	memset(sd->magic_addenemy,0,sizeof(sd->magic_addenemy));
	memset(sd->magic_subrace,0,sizeof(sd->magic_subrace));
	sd->perfect_hit = 0;
	sd->critical_rate = sd->hit_rate = sd->flee_rate = sd->flee2_rate = 100;
	sd->def_rate = sd->def2_rate = sd->mdef_rate = sd->mdef2_rate = 100;
	sd->def_ratio_atk_ele = sd->def_ratio_atk_race = sd->def_ratio_atk_enemy = 0;
	sd->def_ratio_atk_ele_ = sd->def_ratio_atk_race_ = sd->def_ratio_atk_enemy_ = 0;
	sd->get_zeny_num = sd->get_zeny_num2 = 0;
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
	sd->sp_gain_value = 0;
	sd->hp_gain_value = 0;
	sd->speed_add_rate = sd->aspd_add_rate = 100;
	sd->double_add_rate = sd->perfect_hit_add = sd->get_zeny_add_num = sd->get_zeny_add_num2 = 0;
	sd->splash_range = sd->splash_add_range = 0;
	sd->autospell_id = sd->autospell_lv = sd->autospell_rate = 0;
	sd->autospell_flag = 0;
	sd->hp_drain_rate = sd->hp_drain_per = sd->sp_drain_rate = sd->sp_drain_per = 0;
	sd->hp_drain_rate_ = sd->hp_drain_per_ = sd->sp_drain_rate_ = sd->sp_drain_per_ = 0;
	sd->hp_drain_value = sd->hp_drain_value_ = sd->sp_drain_value = sd->sp_drain_value_ = 0;
	sd->short_weapon_damage_return = sd->long_weapon_damage_return = sd->magic_damage_return = 0;
	sd->break_weapon_rate = sd->break_armor_rate = 0;
	sd->add_steal_rate = 0;
	sd->unbreakable_equip = 0;
	//VJ[hpú»
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
	sd->add_attackrange = 0;
	sd->add_attackrange_rate = 100;
	sd->special_state.item_no_use = 0;

	for(i=0;i<10;i++) {
		index = sd->equip_index[i];
		current_equip_item_index = i;//(Ê`FbNp)
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
					for(j=0;j<sd->inventory_data[index]->slot;j++){	// J[h
						int c=sd->status.inventory[index].card[j];
						current_equip_card_id = c;		//I[gXy(d¡`FbNp)
						if(c>0){
							if(i == 8 && sd->status.inventory[index].equip == 0x20)
								sd->state.lr_flag = 1;
							if(calclimit == 2)
								run_script(itemdb_usescript(c),0,sd->bl.id,0);
							run_script(itemdb_equipscript(c),0,sd->bl.id,0);
							sd->state.lr_flag = 0;
						}
					}
				}
			}
			else if(sd->inventory_data[index]->type==5){ // hï
				if(sd->status.inventory[index].card[0]!=0x00ff && sd->status.inventory[index].card[0]!=0x00fe && sd->status.inventory[index].card[0]!=(short)0xff00) {
					int j;
					for(j=0;j<sd->inventory_data[index]->slot;j++){	// J[h
						int c=sd->status.inventory[index].card[j];
						current_equip_card_id = c;		//I[gXy(d¡`FbNp)
						if(c>0) {
							if(calclimit == 2)
								run_script(itemdb_usescript(c),0,sd->bl.id,0);
							run_script(itemdb_equipscript(c),0,sd->bl.id,0);
						}
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

	// õiÉæéXe[^XÏ»Í±±ÅÀs
	for(i=0;i<10;i++) {
		index = sd->equip_index[i];
		current_equip_item_index = i;//index;	//Ê`FbNp
		current_equip_card_id = index;		//I[gXy(d¡`FbNp)
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
					//ñ¬pf[^üÍ
					sd->watk_ += sd->inventory_data[index]->atk;
					sd->watk_2 = (r=sd->status.inventory[index].refine)*	// ¸BUÍ
						refinebonus[wlv][0];
					if( (r-=refinebonus[wlv][2])>0 )	// ßè¸B{[iX
						sd->overrefine_ = r*refinebonus[wlv][1];

					if(sd->status.inventory[index].card[0]==0x00ff){	// »¢í
						sd->star_ = (sd->status.inventory[index].card[1]>>8);	// ¯Ì©¯ç
						if(sd->star_ == 15) sd->star_ = 40;
						wele_= (sd->status.inventory[index].card[1]&0x0f);	// ® «
						//LO{[iX
						if(ranking_get_id2rank( *((unsigned long *)(&sd->status.inventory[index].card[2])) ,RK_BLACKSMITH))
							sd->ranker_weapon_bonus_ = 10;
					}
					sd->attackrange_ += sd->inventory_data[index]->range;
					sd->state.lr_flag = 1;
					if(calclimit == 2)
						run_script(sd->inventory_data[index]->use_script,0,sd->bl.id,0);
					run_script(sd->inventory_data[index]->equip_script,0,sd->bl.id,0);
					sd->state.lr_flag = 0;
				}
				else {	//ñ¬íÈO
					sd->watk += sd->inventory_data[index]->atk;
					sd->watk2 += (r=sd->status.inventory[index].refine)*	// ¸BUÍ
						refinebonus[wlv][0];
					if( (r-=refinebonus[wlv][2])>0 )	// ßè¸B{[iX
						sd->overrefine += r*refinebonus[wlv][1];

					if(sd->status.inventory[index].card[0]==0x00ff){	// »¢í
						sd->star += (sd->status.inventory[index].card[1]>>8);	// ¯Ì©¯ç
						if(sd->star == 15) sd->star = 40;
						wele = (sd->status.inventory[index].card[1]&0x0f);	// ® «
						//LO{[iX
						if(ranking_get_id2rank(*((unsigned long *)(&sd->status.inventory[index].card[2])),RK_BLACKSMITH))
							sd->ranker_weapon_bonus = 10;
					}
					sd->attackrange += sd->inventory_data[index]->range;
					if(calclimit == 2)
						run_script(sd->inventory_data[index]->use_script,0,sd->bl.id,0);
					run_script(sd->inventory_data[index]->equip_script,0,sd->bl.id,0);
				}
			}
			else if(sd->inventory_data[index]->type == 5) {
				sd->watk += sd->inventory_data[index]->atk;
				refinedef += sd->status.inventory[index].refine*refinebonus[0][0];
				if(calclimit == 2)
					run_script(sd->inventory_data[index]->use_script,0,sd->bl.id,0);
				run_script(sd->inventory_data[index]->equip_script,0,sd->bl.id,0);
			}
		}
	}

	if(sd->equip_index[10] >= 0){ // î
		index = sd->equip_index[10];
		if(sd->inventory_data[index]){		//Ü¾®«ªüÁÄ¢È¢
			sd->state.lr_flag = 2;
			if(calclimit == 2)
				run_script(sd->inventory_data[index]->use_script,0,sd->bl.id,0);
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
	sd->get_zeny_num = (sd->get_zeny_num + sd->get_zeny_add_num > 100) ? 100 : (sd->get_zeny_num + sd->get_zeny_add_num);
	sd->get_zeny_num2 = (sd->get_zeny_num2 + sd->get_zeny_add_num2 > 100) ? 100 : (sd->get_zeny_num2 + sd->get_zeny_add_num2);
	sd->splash_range += sd->splash_add_range;
	if(sd->speed_add_rate != 100)
		sd->speed_rate += sd->speed_add_rate - 100;
	if(sd->aspd_add_rate != 100)
		sd->aspd_rate += sd->aspd_add_rate - 100;

	// íATKTCYâ³ (Eè)
	sd->atkmods[0] = atkmods[0][sd->weapontype1];
	sd->atkmods[1] = atkmods[1][sd->weapontype1];
	sd->atkmods[2] = atkmods[2][sd->weapontype1];
	//íATKTCYâ³ (¶è)
	sd->atkmods_[0] = atkmods[0][sd->weapontype2];
	sd->atkmods_[1] = atkmods[1][sd->weapontype2];
	sd->atkmods_[2] = atkmods[2][sd->weapontype2];

	// job{[iXª
	for(i=0;i<sd->status.job_level && i<MAX_LEVEL;i++){
		if(job_bonus[s_class.upper][s_class.job][i])
			sd->paramb[job_bonus[s_class.upper][s_class.job][i]-1]++;
	}

	if( (skill=pc_checkskill(sd,AC_OWL))>0 )	// Ó­ë¤ÌÚ
		sd->paramb[4] += skill;

	if( pc_checkskill(sd,BS_HILTBINDING) ) {	// qgoCfBO
		sd->paramb[0] += 1;
		sd->watk += 4;
	}

	if( (skill=pc_checkskill(sd,SA_DRAGONOLOGY))>0 ){// hSmW[
		sd->paramb[3] += (int)((skill+1)*0.5);
	}

	//}[_[{[iX
	if(ranking_get_point(sd,RK_PK) >= 400)
	{
		sd->paramb[0]+= 5;
		sd->paramb[1]+= 5;
		sd->paramb[2]+= 5;
		sd->paramb[3]+= 5;
		sd->paramb[4]+= 5;
		sd->paramb[5]+= 5;

		sd->atk_rate += 10;
		sd->matk_rate += 10;
	}else if(ranking_get_point(sd,RK_PK) >=100)
	{
		sd->paramb[0]+= 3;
		sd->paramb[1]+= 3;
		sd->paramb[2]+= 3;
		sd->paramb[3]+= 3;
		sd->paramb[4]+= 3;
		sd->paramb[5]+= 3;

		sd->atk_rate += 10;
		sd->matk_rate += 10;
	}
	//1xàñÅÈ¢Job70XpmrÉ+10
	if(s_class.job == 23 && (sd->die_counter == 0 || sd->repeal_die_counter == 1)&& sd->status.job_level >= 70){
		sd->paramb[0]+= 10;
		sd->paramb[1]+= 10;
		sd->paramb[2]+= 10;
		sd->paramb[3]+= 10;
		sd->paramb[4]+= 10;
		sd->paramb[5]+= 10;
	}

	//MhXL
	//XLLø && MhL && }X^[Ú± && ©ª!=}X^[ && ¯¶}bv
	if(battle_config.guild_hunting_skill_available && g && gmsd
		&& ((battle_config.allow_me_guild_skill==1) || (gmsd != sd))
		&& sd->bl.m == gmsd->bl.m)//
	{
		int dx,dy,range;
		//£»èðs¤
		if(battle_config.guild_skill_check_range){
			dx = abs(sd->bl.x - gmsd->bl.x);
			dy = abs(sd->bl.y - gmsd->bl.y);
			if(battle_config.guild_skill_effective_range > 0)//¯ê£ÅvZ
			{
				range = battle_config.guild_skill_effective_range;
				if(dx <=range &&  dy <= range){
					sd->paramb[0] += guild_checkskill(g,GD_LEADERSHIP);//str
					sd->paramb[1] += guild_checkskill(g,GD_SOULCOLD);//agi
					sd->paramb[2] += guild_checkskill(g,GD_GLORYWOUNDS);//vit
					sd->paramb[4] += guild_checkskill(g,GD_HAWKEYES);//dex
					sd->under_the_influence_of_the_guild_skill = range+1;//(0>Åe¿º,dÈéêà éÌÅ+1)
				}else
					sd->under_the_influence_of_the_guild_skill = 0;
			}else{//ÂÊ£
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

		}else{//}bvSÌ
			sd->paramb[0] += guild_checkskill(g,GD_LEADERSHIP);//str
			sd->paramb[1] += guild_checkskill(g,GD_SOULCOLD);//agi
			sd->paramb[2] += guild_checkskill(g,GD_GLORYWOUNDS);//vit
			sd->paramb[4] += guild_checkskill(g,GD_HAWKEYES);//dex
			sd->under_the_influence_of_the_guild_skill = battle_config.guild_skill_effective_range+1;
		}
	}else{//}bvªáÁ½èc³ø¾Á½è
		sd->under_the_influence_of_the_guild_skill = 0;
	}

	// Xe[^XÏ»Éæéî{p[^â³
	if(sd->sc_count){

		// WÍüã
		if(sd->sc_data[SC_CONCENTRATE].timer!=-1 && sd->sc_data[SC_QUAGMIRE].timer == -1){
			sd->paramb[1]+= (sd->status.agi+sd->paramb[1]+sd->parame[1]-sd->paramcard[1])*(2+sd->sc_data[SC_CONCENTRATE].val1)/100;
			sd->paramb[4]+= (sd->status.dex+sd->paramb[4]+sd->parame[4]-sd->paramcard[4])*(2+sd->sc_data[SC_CONCENTRATE].val1)/100;
		}
		//SXyALL+20
		if(sd->sc_data[SC_INCALLSTATUS].timer!=-1){
			sd->paramb[0]+= sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[1]+= sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[2]+= sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[3]+= sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[4]+= sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[5]+= sd->sc_data[SC_INCALLSTATUS].val1;
		}

		//ãÊêEÌ°
		//äÈÌÅLV/10Á
		if(sd->sc_data[SC_HIGH].timer!=-1)
		{
			sd->paramb[0]+= sd->status.base_level/10;
			sd->paramb[1]+= sd->status.base_level/10;
			sd->paramb[2]+= sd->status.base_level/10;
			sd->paramb[3]+= sd->status.base_level/10;
			sd->paramb[4]+= sd->status.base_level/10;
			sd->paramb[5]+= sd->status.base_level/10;
		}

		//Hp
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

		//ì¯«ÌATK +10*LV
		if(sd->sc_data[SC_SPURT].timer!=-1)
			sd->base_atk += 10*sd->sc_data[SC_SPURT].val1;

		//MhXL ÕíÔ¨
		if(sd->sc_data[SC_BATTLEORDER].timer!=-1){
			sd->paramb[0]+= 5*sd->sc_data[SC_BATTLEORDER].val1;
			sd->paramb[3]+= 5*sd->sc_data[SC_BATTLEORDER].val1;
			sd->paramb[4]+= 5*sd->sc_data[SC_BATTLEORDER].val1;
		}

		if(sd->sc_data[SC_CHASEWALK_STR].timer!=-1)
			sd->paramb[0] += sd->sc_data[SC_CHASEWALK_STR].val1;

		if(sd->sc_data[SC_INCREASEAGI].timer!=-1)	// ¬xÁ
			sd->paramb[1]+= 2+sd->sc_data[SC_INCREASEAGI].val1;

		if(sd->sc_data[SC_DECREASEAGI].timer!=-1)	// ¬x¸­(agiÍbattle.cÅ)
			sd->paramb[1]-= 2+sd->sc_data[SC_DECREASEAGI].val1;

		if(sd->sc_data[SC_BLESSING].timer!=-1){	// ubVO
			sd->paramb[0]+= sd->sc_data[SC_BLESSING].val1;
			sd->paramb[3]+= sd->sc_data[SC_BLESSING].val1;
			sd->paramb[4]+= sd->sc_data[SC_BLESSING].val1;
		}
		if(sd->sc_data[SC_NEN].timer!=-1){	// O
			sd->paramb[0]+= sd->sc_data[SC_NEN].val1;
			sd->paramb[3]+= sd->sc_data[SC_NEN].val1;
		}
		if(sd->sc_data[SC_SUITON].timer!=-1){	// Ù
			if(sd->sc_data[SC_SUITON].val3)
				sd->paramb[1]+=sd->sc_data[SC_SUITON].val3;
			if(sd->sc_data[SC_SUITON].val4)
				sd->speed = sd->speed*2;
		}

		if(sd->sc_data[SC_GLORIA].timer!=-1)	// OA
			sd->paramb[5]+= 30;

		if(sd->sc_data[SC_LOUD].timer!=-1 && sd->sc_data[SC_QUAGMIRE].timer == -1)	// Eh{CX
			sd->paramb[0]+= 4;

		if(sd->sc_data[SC_TRUESIGHT].timer!=-1){	// gD[TCg
			sd->paramb[0]+= 5;
			sd->paramb[1]+= 5;
			sd->paramb[2]+= 5;
			sd->paramb[3]+= 5;
			sd->paramb[4]+= 5;
			sd->paramb[5]+= 5;
		}

		if(sd->sc_data[SC_INCREASING].timer!=-1) // CN[VOALAV[
		{
			sd->paramb[1]+= 4;
			sd->paramb[4]+= 4;
		}
		
		//fBtFX
		if(sd->sc_data[SC_DEFENCE].timer!=-1)
			sd->paramb[2]+= sd->sc_data[SC_DEFENCE].val1*2;
		
		if(sd->sc_data[SC_QUAGMIRE].timer!=-1){	// N@O}CA
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

		//
		if(sd->sc_data[SC_MARIONETTE].timer!=-1){
			sd->paramb[0]-= sd->status.str/2;
			sd->paramb[1]-= sd->status.agi/2;
			sd->paramb[2]-= sd->status.vit/2;
			sd->paramb[3]-= sd->status.int_/2;
			sd->paramb[4]-= sd->status.dex/2;
			sd->paramb[5]-= sd->status.luk/2;
		}

		//
		if(sd->sc_data[SC_MARIONETTE2].timer!=-1)
		{
			struct map_session_data* ssd = map_id2sd(sd->sc_data[SC_MARIONETTE2].val2);
			if(ssd){
				if(battle_config.marionette_type){
					sd->paramb[0]+= ssd->status.str/2;
					sd->paramb[1]+= ssd->status.agi/2;
					sd->paramb[2]+= ssd->status.vit/2;
					sd->paramb[3]+= ssd->status.int_/2;
					sd->paramb[4]+= ssd->status.dex/2;
					sd->paramb[5]+= ssd->status.luk/2;
				}else{
					//str
					if(sd->paramb[0]+sd->parame[0]+sd->status.str < battle_config.max_marionette_str)
					{
						sd->paramb[0]+= ssd->status.str/2;
						if(sd->paramb[0]+sd->parame[0]+sd->status.str > battle_config.max_marionette_str)
							sd->paramb[0] = battle_config.max_marionette_str - sd->status.str;
					}
					//agi
					if(sd->paramb[1]+sd->parame[1]+sd->status.agi < battle_config.max_marionette_agi)
					{
						sd->paramb[1]+= ssd->status.agi/2;
						if(sd->paramb[1]+sd->parame[1]+sd->status.agi > battle_config.max_marionette_agi)
							sd->paramb[1] = battle_config.max_marionette_agi - sd->status.agi;
					}
					//vit
					if(sd->paramb[2]+sd->parame[2]+sd->status.vit < battle_config.max_marionette_vit)
					{
						sd->paramb[2]+= ssd->status.vit/2;
						if(sd->paramb[2]+sd->parame[2]+sd->status.vit > battle_config.max_marionette_vit)
							sd->paramb[2] = battle_config.max_marionette_vit - sd->status.vit;
					}
					//int
					if(sd->paramb[3]+sd->parame[3]+sd->status.int_ < battle_config.max_marionette_int)
					{
						sd->paramb[3]+= ssd->status.int_/2;
						if(sd->paramb[3]+sd->parame[3]+sd->status.int_ > battle_config.max_marionette_int)
							sd->paramb[3] = battle_config.max_marionette_int - sd->status.int_;
					}
					//dex
					if(sd->paramb[4]+sd->parame[4]+sd->status.dex < battle_config.max_marionette_dex)
					{
					sd->paramb[4]+= ssd->status.dex/2;
						if(sd->paramb[4]+sd->parame[4]+sd->status.dex > battle_config.max_marionette_dex)
							sd->paramb[4] = battle_config.max_marionette_dex - sd->status.dex;
					}
					//luk
					if(sd->paramb[5]+sd->parame[5]+sd->status.luk < battle_config.max_marionette_luk)
					{
						sd->paramb[5]+= ssd->status.luk/2;
						if(sd->paramb[5]+sd->parame[5]+sd->status.luk > battle_config.max_marionette_luk)
							sd->paramb[5] = battle_config.max_marionette_luk - sd->status.luk;
					}
				}
			}
		}
	}

	sd->paramc[0]=sd->status.str+sd->paramb[0]+sd->parame[0];
	sd->paramc[1]=sd->status.agi+sd->paramb[1]+sd->parame[1];
	sd->paramc[2]=sd->status.vit+sd->paramb[2]+sd->parame[2];
	sd->paramc[3]=sd->status.int_+sd->paramb[3]+sd->parame[3];
	sd->paramc[4]=sd->status.dex+sd->paramb[4]+sd->parame[4];
	sd->paramc[5]=sd->status.luk+sd->paramb[5]+sd->parame[5];

	for(i=0;i<6;i++)
		if(sd->paramc[i] < 0) sd->paramc[i] = 0;

	//BASEATKvZ
	if(sd->status.weapon == 11 || sd->status.weapon == 13 || sd->status.weapon == 14
			|| (sd->status.weapon>=17 && sd->status.weapon<=21)) {
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

	//ACeâ³
	if(sd->sc_data){
		if(sd->sc_data[SC_MEAL_INCATK].timer!=-1)
			sd->base_atk += sd->sc_data[SC_MEAL_INCATK].val1;

		if(sd->sc_data[SC_MEAL_INCMATK].timer!=-1){
			sd->matk1 += sd->sc_data[SC_MEAL_INCMATK].val1;
			sd->matk2 += sd->sc_data[SC_MEAL_INCMATK].val1;
		}
	}

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
	//ACeâ³
	if(sd->sc_data){
		if(sd->sc_data[SC_MEAL_INCHIT].timer!=-1)
			sd->hit += sd->sc_data[SC_MEAL_INCHIT].val1;
		if(sd->sc_data[SC_MEAL_INCFLEE].timer!=-1)
			sd->flee += sd->sc_data[SC_MEAL_INCFLEE].val1;
		if(sd->sc_data[SC_MEAL_INCFLEE2].timer!=-1)
			sd->flee2 += sd->sc_data[SC_MEAL_INCFLEE2].val1;
		if(sd->sc_data[SC_MEAL_INCCRITICAL].timer!=-1)
			sd->critical += sd->sc_data[SC_MEAL_INCCRITICAL].val1*10;
		if(sd->sc_data[SC_MEAL_INCDEF].timer!=-1)
			sd->def+=sd->sc_data[SC_MEAL_INCDEF].val1;
		if(sd->sc_data[SC_MEAL_INCMDEF].timer!=-1)
			sd->mdef+=sd->sc_data[SC_MEAL_INCMDEF].val1;
	}

	if(sd->base_atk < 1)
		sd->base_atk = 1;
	if(sd->critical_rate != 100)
		sd->critical = (sd->critical*sd->critical_rate)/100;
	if(sd->critical < 10)
		sd->critical = 10;
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

	// ñ¬ ASPD C³
	if (sd->status.weapon <= 22)
		sd->aspd += aspd_base[s_class.job][sd->status.weapon]-(sd->paramc[1]*4+sd->paramc[4])*aspd_base[s_class.job][sd->status.weapon]/1000;
	else
		sd->aspd += (
			(aspd_base[s_class.job][sd->weapontype1]-(sd->paramc[1]*4+sd->paramc[4])*aspd_base[s_class.job][sd->weapontype1]/1000) +
			(aspd_base[s_class.job][sd->weapontype2]-(sd->paramc[1]*4+sd->paramc[4])*aspd_base[s_class.job][sd->weapontype2]/1000)
			) * 140 / 200;

	aspd_rate = sd->aspd_rate;

	//U¬xÁ

	//AhoXhubN
	if(sd->weapontype1 == 0x0f && (skill = pc_checkskill(sd,SA_ADVANCEDBOOK)) > 0)
	{
		aspd_rate -= skill/2;
	}
	//VOANV
	if(sd->status.weapon >=17 && sd->status.weapon<22 && (skill = pc_checkskill(sd,GS_SINGLEACTION)) > 0)
	{
		aspd_rate -= skill/2;
		sd->hit += skill*2;
	}
	//¾zÆÆ¯Ì«
	if((skill = pc_checkskill(sd,SG_DEVIL)) > 0 && sd->status.job_level>=50)
	{
		aspd_rate -= skill*3;
		if(sd->sc_data[SC_DEVIL].timer!=-1 || sd->sc_data[SC_DEVIL].val1<skill)
			status_change_start(&sd->bl,SC_DEVIL,skill,0,0,0,5000,0);
		clif_status_change(&sd->bl,SI_DEVIL,1);
	}

	//¾zÆÆ¯ÌZ
	if(sd && sd->sc_data[SC_FUSION].timer!=-1)
	{
		aspd_rate -= 20;
		sd->perfect_hit += 100;
	//	if(sd->view_class==PC_CLASS_SG)
	//		sd->view_class = sd->view_class+1;
	}
	if(sd && sd->sc_data[SC_SANTA].timer!=-1)
	{
		sd->view_class = 26;
	}

	if( (skill=pc_checkskill(sd,AC_VULTURE))>0){	// VÌÚ
		sd->hit += skill;
		if(sd->status.weapon == 11)
			sd->attackrange += skill;
	}
	if( (skill=pc_checkskill(sd,GS_SNAKEEYE))>0){	// Xl[NAC
		if(sd->status.weapon>=17 && sd->status.weapon<22)
		{
			sd->attackrange += skill;
			sd->hit += skill;
		}
	}
	if( (skill=pc_checkskill(sd,BS_WEAPONRESEARCH))>0)	// í¤Ì½¦Á
		sd->hit += skill*2;

	if(sd->status.option&2 && (skill = pc_checkskill(sd,RG_TUNNELDRIVE))>0 )	// glhCu	// glhCu
		sd->speed += (short)(1.2*DEFAULT_WALK_SPEED - skill*9);

	if (pc_iscarton(sd) && (skill=pc_checkskill(sd,MC_PUSHCART))>0)	// J[gÉæé¬xáº
		sd->speed += (short)((10-skill) * (DEFAULT_WALK_SPEED * 0.1));
	else if (pc_isriding(sd)){// yRyRæèÉæé¬xÁ
			sd->max_weight += battle_config.riding_weight; // Weight+¿(úÝèÍ0)10000Å{I;
			if(sd->sc_data[SC_DEFENDER].timer != -1)//fBtF_[Íàs¬xÆ¯¶
				sd->speed -= 0;
			else
			sd->speed -= (short)(0.25 * DEFAULT_WALK_SPEED);
	}

	if(sd->sc_count && sd->sc_data){
		int sc_speed_rate=100;
		if(sd->sc_data[SC_AVOID].timer!=-1)//Ù}ñð
			sc_speed_rate -= sd->sc_data[SC_AVOID].val1*10;
		if((sd->sc_data[SC_INCREASEAGI].timer!=-1)&&(sc_speed_rate > 75))	// ¬xÁÉæéÚ®¬xÁ
				sc_speed_rate = 75;
		if((sd->sc_data[SC_RUN].timer!=-1)&&(sc_speed_rate > 75))	// ì¯«ÉæéÚ®¬xÁ
				sc_speed_rate = 75;
		if((sd->sc_data[SC_BERSERK].timer!=-1)&&(sc_speed_rate > 75))	// o[T[NÉæéÚ®¬xÁ
				sc_speed_rate = 75;
		if((sd->sc_data[SC_CARTBOOST].timer!=-1)&&(sc_speed_rate > 80))	// J[gu[XgÉæéÚ®¬xÁ
				sc_speed_rate = 80;
		if((sd->sc_data[SC_WINDWALK].timer!=-1)&&(sc_speed_rate > 100-(sd->sc_data[SC_WINDWALK].val1*2)))	// EBhEH[NÉæéÚ®¬xÁ
				sc_speed_rate = 100-(sd->sc_data[SC_WINDWALK].val1*2);
		if( s_class.job == 12 && (skill=pc_checkskill(sd,TF_MISS))>0 &&(sc_speed_rate > 100-skill))	// ATVnÌñð¦ã¸ÉæéÚ®¬xÁ
				sc_speed_rate = 100-skill;

		sd->speed = sd->speed*sc_speed_rate/100;


		if(sd->sc_data[SC_CLOAKING].timer!=-1)//N[LOÉæé¬xÏ»
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
					// ½nÚ®¬x
					sd->speed += ((10-skill) * 3);
				}else{
					// Ç¢Ú®¬x
					int cloak_speed_table[11]={100,100,103,106,109,112,115,118,121,124,125};
					sd->speed -= (sd->speed * (cloak_speed_table[skill]-100) )/100;
				}
			}
		}


		if(sd->sc_data[SC_CHASEWALK].timer!=-1)/*`FCXEH[NÉæé¬xÏ»*/
		{
			if(sd->sc_data[SC_ROGUE].timer==-1)
				sd->speed += sd->speed*(35 - (5*sd->sc_data[SC_CHASEWALK].val1))/100;
		}

		if(sd->sc_data[SC_DECREASEAGI].timer!=-1){	// ¬x¸­(agiÍbattle.cÅ)
			if(sd->sc_data[SC_DEFENDER].timer != -1)//fBtF_[Í¬xáºµÈ¢
				sd->speed += 0;
			else
				sd->speed = sd->speed *((sd->sc_data[SC_DECREASEAGI].val1>5)?150:133)/100;
		}

		if(sd->sc_data[SC_WEDDING].timer!=-1)	//¥Íà­Ìªx¢
			sd->speed = 2*DEFAULT_WALK_SPEED;
	}

	if(s_class.job == 23 && sd->status.base_level >= 99)
	{
		if(pc_isupper(sd))
			 sd->status.max_hp +=  2000*(100 + sd->paramc[2])/100 * battle_config.upper_hp_rate/100;
		else if(pc_isbaby(sd))//{qÌêÅåHP70%
			 sd->status.max_hp +=  2000*(100 + sd->paramc[2])/100 * battle_config.baby_hp_rate/100;
		else sd->status.max_hp +=  2000*(100 + sd->paramc[2])/100 * battle_config.normal_hp_rate/100;
	}

	if((skill=pc_checkskill(sd,CR_TRUST))>0) { // tFCX
		sd->status.max_hp += skill*200;
		sd->subele[6] += skill*5;
	}

	if((skill=pc_checkskill(sd,BS_SKINTEMPER))>0){ // XLepO
		sd->subele[3] += skill*5;
		sd->subele[0] += skill*1;
	}

	bl=sd->status.base_level;

	//bAtkRange2,bAtkRangeRate2ÌËövZ
	sd->attackrange += sd->add_attackrange;
	sd->attackrange_ += sd->add_attackrange;
	sd->attackrange = sd->attackrange * sd->add_attackrange_rate /100;
	sd->attackrange_ = sd->attackrange_ * sd->add_attackrange_rate /100;
	if(sd->attackrange < 1) sd->attackrange = 1;
	if(sd->attackrange_ < 1) sd->attackrange_ = 1;
	if(sd->attackrange < sd->attackrange_)
		sd->attackrange = sd->attackrange_;

	//ÅåHPvZ
	//]¶EÌêÅåHP25%UP
	if(pc_isupper(sd))
		sd->status.max_hp += ((3500 + bl*hp_coefficient2[s_class.job] + hp_sigma_val[s_class.job][(bl > 0)? bl-1:0])/100 * (100 + sd->paramc[2])/100 + (sd->parame[2] - sd->paramcard[2])) * battle_config.upper_hp_rate/100;
	else if(pc_isbaby(sd))//{qÌêÅåHP70%
		sd->status.max_hp += ((3500 + bl*hp_coefficient2[s_class.job] + hp_sigma_val[s_class.job][(bl > 0)? bl-1:0])/100 * (100 + sd->paramc[2])/100 + (sd->parame[2] - sd->paramcard[2])) * battle_config.baby_hp_rate/100;
	else sd->status.max_hp +=((3500 + bl*hp_coefficient2[s_class.job] + hp_sigma_val[s_class.job][(bl > 0)? bl-1:0])/100 * (100 + sd->paramc[2])/100 + (sd->parame[2] - sd->paramcard[2])) * battle_config.normal_hp_rate/100;

	if(sd->hprate!=100)
		sd->status.max_hp = sd->status.max_hp*sd->hprate/100;

	if(sd->sc_data && sd->sc_data[SC_BERSERK].timer!=-1){	// o[T[N
		sd->status.max_hp = sd->status.max_hp * 3;
	}
	if(sd->sc_data && sd->sc_data[SC_INCMHP2].timer!=-1){
		sd->status.max_hp *= (100+sd->sc_data[SC_INCMHP2].val1)/100;
	}

	// ÅåSPvZ
	//]¶EÌêÅåSP125%
	if(pc_isupper(sd))
		sd->status.max_sp += (((sp_coefficient[s_class.job] * bl) + 1000)/100 * (100 + sd->paramc[3])/100 + (sd->parame[3] - sd->paramcard[3])) * battle_config.upper_sp_rate/100;
	else if(pc_isbaby(sd)) //{qÌêÅåSP70%
		sd->status.max_sp += (((sp_coefficient[s_class.job] * bl) + 1000)/100 * (100 + sd->paramc[3])/100 + (sd->parame[3] - sd->paramcard[3])) * battle_config.baby_sp_rate/100;
	else sd->status.max_sp +=(((sp_coefficient[s_class.job] * bl) + 1000)/100 * (100 + sd->paramc[3])/100 + (sd->parame[3] - sd->paramcard[3])) * battle_config.normal_sp_rate/100;

	if(sd->sprate!=100)
		sd->status.max_sp = sd->status.max_sp*sd->sprate/100;

	if((skill=pc_checkskill(sd,HP_MEDITATIO))>0) // fB^eBI
		sd->status.max_sp += sd->status.max_sp*skill/100;
	if((skill=pc_checkskill(sd,HW_SOULDRAIN))>0) /* \EhC */
		sd->status.max_sp += sd->status.max_sp*2*skill/100;
	if(sd->sc_data && sd->sc_data[SC_INCMSP2].timer!=-1) {
		sd->status.max_sp *= (100+sd->sc_data[SC_INCMSP2].val1)/100;
	}
	if((skill=pc_checkskill(sd,SL_KAINA))>0) /* JCi */
		sd->status.max_sp += 30*skill;

	//©RñHP
	sd->nhealhp = 1 + (sd->paramc[2]/5) + (sd->status.max_hp/200);
	if((skill=pc_checkskill(sd,SM_RECOVERY)) > 0) {	/* HPñÍüã */
		sd->nshealhp = skill*5 + (sd->status.max_hp*skill/500);
		if(sd->nshealhp > 0x7fff) sd->nshealhp = 0x7fff;
	}
	if((skill=pc_checkskill(sd,TK_HPTIME)) > 0) {	// Àç©Èx§
		sd->tk_nhealhp = skill*30 + (sd->status.max_hp*skill/500);
		if(sd->tk_nhealhp > 0x7fff) sd->tk_nhealhp = 0x7fff;
	}
	if(sd->sc_data && sd->sc_data[SC_BERSERK].timer!=-1){
		sd->nhealhp = 0;
	}
	//©RñSP
	sd->nhealsp = 1 + (sd->paramc[3]/6) + (sd->status.max_sp/100);
	if(sd->paramc[3] >= 120)
		sd->nhealsp += ((sd->paramc[3]-120)>>1) + 4;
	if((skill=pc_checkskill(sd,MG_SRECOVERY)) > 0) { /* SPñÍüã */
		sd->nshealsp = skill*3 + (sd->status.max_sp*skill/500);
		if(sd->nshealsp > 0x7fff) sd->nshealsp = 0x7fff;
	}
	if((skill=pc_checkskill(sd,NJ_NINPOU)) > 0) { /* E@Cû */
		sd->nshealsp = skill*3 + (sd->status.max_sp*skill/500);
		if(sd->nshealsp > 0x7fff) sd->nshealsp = 0x7fff;
	}

	if((skill = pc_checkskill(sd,MO_SPIRITSRECOVERY)) > 0) {
		sd->nsshealhp = skill*4 + (sd->status.max_hp*skill/500);
		sd->nsshealsp = skill*2 + (sd->status.max_sp*skill/500);
		if(sd->nsshealhp > 0x7fff) sd->nsshealhp = 0x7fff;
		if(sd->nsshealsp > 0x7fff) sd->nsshealsp = 0x7fff;
	}
	if((skill=pc_checkskill(sd,TK_SPTIME)) > 0) { // yµ¢x§
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
		// fB^eBIÍSPRÅÍÈ­©RñÉ©©é
		sd->nhealsp += (sd->nhealsp)*3*skill/100;
		if(sd->nhealsp > 0x7fff) sd->nhealsp = 0x7fff;
	}

	// í°Ï«i±êÅ¢¢ÌH fBoCveNVÆ¯¶ª¢é©àj
	if( (skill=pc_checkskill(sd,SA_DRAGONOLOGY))>0 ){	// hSmW[
		skill = skill*4;
		sd->addrace[9]+=skill;
		sd->addrace_[9]+=skill;
		sd->subrace[9]+=skill;
	}
	//Fleeã¸
	if( (skill=pc_checkskill(sd,TF_MISS))>0 ) {	// ñð¦Á
		if( s_class.job == 12 || s_class.job == 17 )
			sd->flee += skill*4;
		else
			sd->flee += skill*3;
	}
	if( (skill=pc_checkskill(sd,MO_DODGE))>0 )	// ©Øè
		sd->flee += (skill*3)>>1;
	if( sd->sc_data ) {
		if( sd->sc_data[SC_INCFLEE].timer!=-1 )
			sd->flee += sd->sc_data[SC_INCFLEE].val1;
		if( sd->sc_data[SC_INCFLEE2].timer!=-1 )
			sd->flee += sd->sc_data[SC_INCFLEE2].val1;
	}

	// XLâXe[^XÙíÉæécèÌp[^â³
	if(sd->sc_count){
		//¾zÌÀy DEFÁ
		if(sd->sc_data[SC_SUN_COMFORT].timer!=-1 && sd->bl.m == sd->feel_map[0].m)
			sd->def2 += (sd->status.base_level + sd->status.dex + sd->status.luk)/2;
			//sd->def += (sd->status.base_level + sd->status.dex + sd->status.luk + sd->paramb[4] + sd->paramb[5])/10;

		//ÌÀy
		if(sd->sc_data[SC_MOON_COMFORT].timer!=-1 && sd->bl.m == sd->feel_map[1].m)
			sd->flee += (sd->status.base_level + sd->status.dex + sd->status.luk)/10;
			//sd->flee += (sd->status.base_level + sd->status.dex + sd->status.luk + sd->paramb[4] + sd->paramb[5])/10;

		//¯ÌÀy
		if(sd->sc_data[SC_STAR_COMFORT].timer!=-1 && sd->bl.m == sd->feel_map[2].m)
			aspd_rate -= (sd->status.base_level + sd->status.dex + sd->status.luk)/10;
			//aspd_rate += (sd->status.base_level + sd->status.dex + sd->status.luk + sd->paramb[0] + sd->paramb[4] + sd->paramb[5])/10;

		//N[YRt@C
		if(sd->sc_data[SC_CLOSECONFINE].timer!=-1)
			sd->flee += 10;

		// ATK/DEFÏ»`
		if(sd->sc_data[SC_ANGELUS].timer!=-1)	// GWFX
			sd->def2 = sd->def2*(110+5*sd->sc_data[SC_ANGELUS].val1)/100;
		if(sd->sc_data[SC_IMPOSITIO].timer!=-1)	{// C|VeBI}kX
			sd->watk += sd->sc_data[SC_IMPOSITIO].val1*5;
			index = sd->equip_index[8];
			// ¶èÉÍKpµÈ¢
			//if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == 4)
			//	sd->watk_ += sd->sc_data[SC_IMPOSITIO].val1*5;
		}
		if(sd->sc_data[SC_PROVOKE].timer!=-1){	// v{bN
			sd->def2 = sd->def2*(100-6*sd->sc_data[SC_PROVOKE].val1)/100;
			sd->base_atk = sd->base_atk*(100+2*sd->sc_data[SC_PROVOKE].val1)/100;
			sd->watk = sd->watk*(100+2*sd->sc_data[SC_PROVOKE].val1)/100;
			index = sd->equip_index[8];
			// ¶èÉÍKpµÈ¢
			//if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == 4)
			//	sd->watk_ = sd->watk_*(100+2*sd->sc_data[SC_PROVOKE].val1)/100;
		}
		if(sd->sc_data[SC_POISON].timer!=-1)	// ÅóÔ
			sd->def2 = sd->def2*75/100;

		//^bgJ[h
		if(sd->sc_data[SC_TAROTCARD].timer!=-1 && sd->sc_data[SC_TAROTCARD].val4 == SC_THE_MAGICIAN)
		{
			//THE MAGICIAN ATK¼¸
			//ATK
			sd->base_atk = sd->base_atk * 50/100;
			sd->watk = sd->watk * 50/100;
			index = sd->equip_index[8];
			if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == 4)
				sd->watk_ = sd->watk_ * 50/100;
		}
		else if(sd->sc_data[SC_TAROTCARD].timer!=-1 && sd->sc_data[SC_TAROTCARD].val4 == SC_STRENGTH)
		{	//STRENGTH MATK¼¸
			//MATK
			sd->matk1 = sd->matk1*50/100;
			sd->matk2 = sd->matk2*50/100;
		}
		else if(sd->sc_data[SC_TAROTCARD].timer!=-1 && sd->sc_data[SC_TAROTCARD].val4 == SC_THE_DEVIL)
		{	//THE DEVIL ATK¼ªAMATK¼ª
			//ATK
			sd->base_atk = sd->base_atk * 50/100;
			sd->watk = sd->watk * 50/100;
			index = sd->equip_index[8];
			if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == 4)
				sd->watk_ = sd->watk_ * 50/100;

			//MATK
			sd->matk1 = sd->matk1*50/100;
			sd->matk2 = sd->matk2*50/100;
		}
		else if(sd->sc_data[SC_TAROTCARD].timer!=-1 && sd->sc_data[SC_TAROTCARD].val4 == SC_THE_SUN)
		{
			//THE SUN ATKAMATKAñðA½AhäÍªSÄ20%¸Âº·é 536
			//ATK
			sd->base_atk = sd->base_atk * 80/100;
			sd->watk = sd->watk * 80/100;
			index = sd->equip_index[8];
			if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == 4)
				sd->watk_ = sd->watk_ * 80/100;
			//MATK
			sd->matk1 = sd->matk1*80/100;
			sd->matk2 = sd->matk2*80/100;
			//
			sd->flee = sd->flee * 80/100;
			//
			sd->hit  = sd->hit * 80/100;
			//häÍ
			sd->def = sd->def * 80/100;
			sd->def2 = sd->def2 * 80/100;
		}

		if(sd->sc_data[SC_DRUMBATTLE].timer!=-1){	// í¾ÛÌ¿«
			sd->watk += sd->sc_data[SC_DRUMBATTLE].val2;
			sd->def  += sd->sc_data[SC_DRUMBATTLE].val3;
			index = sd->equip_index[8];
			// ¶èÉÍKpµÈ¢
			//if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == 4)
			//	sd->watk_ += sd->sc_data[SC_DRUMBATTLE].val2;
		}
		if(sd->sc_data[SC_NIBELUNGEN].timer!=-1) {	// j[xOÌwÖ
			index = sd->equip_index[9];
			if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->wlv >= 4)
				sd->watk += sd->sc_data[SC_NIBELUNGEN].val2;
			index = sd->equip_index[8];
			// ¶èÉÍKpµÈ¢
			//if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->wlv >= 4)
			//	sd->watk_ += sd->sc_data[SC_NIBELUNGEN].val2;
		}

		if(sd->sc_data[SC_VOLCANO].timer!=-1 && sd->def_ele==3){	// {P[m
			sd->watk += sd->sc_data[SC_VOLCANO].val3;
		}
		if(sd->sc_data[SC_INCATK2].timer!=-1) {
			sd->watk = sd->watk*(100+sd->sc_data[SC_INCATK2].val1)/100;
		}

		if(sd->sc_data[SC_SIGNUMCRUCIS].timer!=-1)
			sd->def = sd->def * (100 - sd->sc_data[SC_SIGNUMCRUCIS].val2)/100;
		if(sd->sc_data[SC_ETERNALCHAOS].timer!=-1)	// G^[iJIX
			sd->def2=0;

		if(sd->sc_data[SC_CONCENTRATION].timer!=-1){ //RZg[V
			sd->base_atk = sd->base_atk * (100 + 5*sd->sc_data[SC_CONCENTRATION].val1)/100;
			sd->watk = sd->watk * (100 + 5*sd->sc_data[SC_CONCENTRATION].val1)/100;
			index = sd->equip_index[8];
			if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == 4)
				sd->watk_ = sd->watk_ * (100 + 5*sd->sc_data[SC_CONCENTRATION].val1)/100;
			sd->def = sd->def * (100 - 5*sd->sc_data[SC_CONCENTRATION].val1)/100;
			sd->def2 = sd->def2 * (100 - 5*sd->sc_data[SC_CONCENTRATION].val1)/100;
		}

		if(sd->sc_data[SC_INCATK].timer!=-1)	//item 682p
			sd->watk += sd->sc_data[SC_INCATK].val1;
		if(sd->sc_data[SC_INCMATK].timer!=-1){	//item 683p
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

		// ASPD/Ú®¬xÏ»n
		if(sd->sc_data[SC_TWOHANDQUICKEN].timer != -1 && sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1 && sd->sc_data[SC_DECREASEAGI].timer==-1 )	// 2HQ
			aspd_rate -= 30;

		if(sd->sc_data[SC_ONEHAND].timer != -1 && sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1 && sd->sc_data[SC_DECREASEAGI].timer==-1)	// 1HQ
			aspd_rate -= 30;

		if(sd->sc_data[SC_ADRENALINE2].timer != -1 && sd->sc_data[SC_TWOHANDQUICKEN].timer == -1 && sd->sc_data[SC_ONEHAND].timer == -1 &&
			sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1 && sd->sc_data[SC_DECREASEAGI].timer==-1) {	// AhibV2
			if(sd->sc_data[SC_ADRENALINE2].val2 || !battle_config.party_skill_penaly)
				aspd_rate -= 30;
			else
				aspd_rate -= 25;
		}else if(sd->sc_data[SC_ADRENALINE].timer != -1 && sd->sc_data[SC_TWOHANDQUICKEN].timer == -1 && sd->sc_data[SC_ONEHAND].timer == -1 &&
			sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1 && sd->sc_data[SC_DECREASEAGI].timer==-1) {	// AhibV
			if(sd->sc_data[SC_ADRENALINE].val2 || !battle_config.party_skill_penaly)
				aspd_rate -= 30;
			else
				aspd_rate -= 25;
		}
		if(sd->sc_data[SC_SPEARSQUICKEN].timer != -1 && sd->sc_data[SC_ADRENALINE].timer == -1 && sd->sc_data[SC_ADRENALINE2].timer == -1 &&
			sd->sc_data[SC_TWOHANDQUICKEN].timer == -1 && sd->sc_data[SC_ONEHAND].timer == -1 &&
			sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1 && sd->sc_data[SC_DECREASEAGI].timer==-1)	// XsANBbP
			aspd_rate -= sd->sc_data[SC_SPEARSQUICKEN].val2;

		if(sd->sc_data[SC_ASSNCROS].timer!=-1 && // [zÌATVNX
			sd->sc_data[SC_TWOHANDQUICKEN].timer==-1 && sd->sc_data[SC_ONEHAND].timer==-1 &&
			sd->sc_data[SC_ADRENALINE].timer==-1 && sd->sc_data[SC_ADRENALINE2].timer==-1 &&
			sd->sc_data[SC_SPEARSQUICKEN].timer==-1 && sd->sc_data[SC_DONTFORGETME].timer == -1 && sd->status.weapon != 11 && !(sd->status.weapon>=17 && sd->status.weapon<=21))
				aspd_rate -= 5+sd->sc_data[SC_ASSNCROS].val1+sd->sc_data[SC_ASSNCROS].val2+sd->sc_data[SC_ASSNCROS].val3;
		else if(sd->sc_data[SC_ASSNCROS_].timer!=-1 && // [zÌATVNX
			sd->sc_data[SC_TWOHANDQUICKEN].timer==-1 && sd->sc_data[SC_ONEHAND].timer==-1 &&
			sd->sc_data[SC_ADRENALINE].timer==-1 && sd->sc_data[SC_ADRENALINE2].timer==-1 &&
			sd->sc_data[SC_SPEARSQUICKEN].timer==-1 && sd->sc_data[SC_DONTFORGETME].timer == -1 && sd->status.weapon != 11 && !(sd->status.weapon>=17 && sd->status.weapon<=21))
				aspd_rate -= 5+sd->sc_data[SC_ASSNCROS_].val1+sd->sc_data[SC_ASSNCROS_].val2+sd->sc_data[SC_ASSNCROS_].val3;

		if(sd->sc_data[SC_DONTFORGETME].timer!=-1){		// ðYêÈ¢Å
			aspd_rate += sd->sc_data[SC_DONTFORGETME].val1*3 + sd->sc_data[SC_DONTFORGETME].val2 + (sd->sc_data[SC_DONTFORGETME].val3>>16);
			sd->speed= sd->speed*(100+sd->sc_data[SC_DONTFORGETME].val1*2 + sd->sc_data[SC_DONTFORGETME].val2 + (sd->sc_data[SC_DONTFORGETME].val3&0xffff))/100;
		}else if(sd->sc_data[SC_DONTFORGETME_].timer!=-1){		// ðYêÈ¢Å
			aspd_rate += sd->sc_data[SC_DONTFORGETME_].val1*3 + sd->sc_data[SC_DONTFORGETME_].val2 + (sd->sc_data[SC_DONTFORGETME_].val3>>16);
			sd->speed= sd->speed*(100+sd->sc_data[SC_DONTFORGETME_].val1*2 + sd->sc_data[SC_DONTFORGETME_].val2 + (sd->sc_data[SC_DONTFORGETME_].val3&0xffff))/100;
		}

		if(sd->sc_data[SC_GRAVITATION].timer!=-1)
		{
			aspd_rate += sd->sc_data[SC_GRAVITATION].val1*5;
			if(battle_config.player_gravitation_type)
				sd->speed = sd->speed*(100+sd->sc_data[SC_GRAVITATION].val1*5)/100;

		}
		if(sd->sc_data[SC_BERSERK].timer!=-1){
			aspd_rate -= 30;
		}
		if(sd->sc_data[SC_POISONPOTION].timer!=-1){
			aspd_rate -= 25;
		}
		if(	sd->sc_data[i=SC_SPEEDPOTION3].timer!=-1 ||
			sd->sc_data[i=SC_SPEEDPOTION2].timer!=-1 ||
			sd->sc_data[i=SC_SPEEDPOTION1].timer!=-1 ||
			sd->sc_data[i=SC_SPEEDPOTION0].timer!=-1)	//  ¬|[V
			aspd_rate -= sd->sc_data[i].val2;

		// HIT/FLEEÏ»n
		if(sd->sc_data[SC_WHISTLE].timer!=-1){  // ûJ
			sd->flee += sd->flee * (sd->sc_data[SC_WHISTLE].val1
					+sd->sc_data[SC_WHISTLE].val2+(sd->sc_data[SC_WHISTLE].val3>>16))/100;
			sd->flee2+= (sd->sc_data[SC_WHISTLE].val1+sd->sc_data[SC_WHISTLE].val2+(sd->sc_data[SC_WHISTLE].val3&0xffff)) * 10;
		}else if(sd->sc_data[SC_WHISTLE_].timer!=-1){  // ûJ
			sd->flee += sd->flee * (sd->sc_data[SC_WHISTLE_].val1
					+sd->sc_data[SC_WHISTLE_].val2+(sd->sc_data[SC_WHISTLE_].val3>>16))/100;
			sd->flee2+= (sd->sc_data[SC_WHISTLE_].val1+sd->sc_data[SC_WHISTLE_].val2+(sd->sc_data[SC_WHISTLE_].val3&0xffff)) * 10;
		}

		if(sd->sc_data[SC_HUMMING].timer!=-1){  // n~O
			sd->hit += (sd->sc_data[SC_HUMMING].val1*2+sd->sc_data[SC_HUMMING].val2
					+sd->sc_data[SC_HUMMING].val3) * sd->hit/100;
		}else if(sd->sc_data[SC_HUMMING_].timer!=-1){  // n~O
			sd->hit += (sd->sc_data[SC_HUMMING_].val1*2+sd->sc_data[SC_HUMMING_].val2
					+sd->sc_data[SC_HUMMING_].val3) * sd->hit/100;
		}

		if(sd->sc_data[SC_VIOLENTGALE].timer!=-1 && sd->def_ele==4){	// oCIgQC
			sd->flee += sd->flee*sd->sc_data[SC_VIOLENTGALE].val3/100;
		}
		if(sd->sc_data[SC_BLIND].timer!=-1){	// Ã
			sd->hit -= sd->hit*25/100;
			sd->flee -= sd->flee*25/100;
		}
		if(sd->sc_data[SC_WINDWALK].timer!=-1) // EBhEH[N
			sd->flee += sd->flee*(sd->sc_data[SC_WINDWALK].val2)/100;
		if(sd->sc_data[SC_SPIDERWEB].timer!=-1) //XpC_[EFu
			sd->flee -= sd->flee*50/100;
		if(sd->sc_data[SC_TRUESIGHT].timer!=-1) //gD[TCg
			sd->hit += 3*(sd->sc_data[SC_TRUESIGHT].val1);
		if(sd->sc_data[SC_CONCENTRATION].timer!=-1) //RZg[V
			sd->hit += 10*(sd->sc_data[SC_CONCENTRATION].val1);
		if(sd->sc_data[SC_INCHIT].timer!=-1)
			sd->hit += sd->sc_data[SC_INCHIT].val1;
		if(sd->sc_data[SC_INCHIT2].timer!=-1)
			sd->hit *= (100+sd->sc_data[SC_INCHIT2].val1)/100;
		if(sd->sc_data[SC_BERSERK].timer!=-1)
			sd->flee -= sd->flee*50/100;
		if(sd->sc_data[SC_INCFLEE].timer!=-1) // ¬x­»
			sd->flee += sd->flee*(sd->sc_data[SC_INCFLEE].val2)/100;

		//KXK[XL
		if(sd->sc_data[SC_FLING].timer!=-1)			// tCO
			sd->def = sd->def * (100 - 5*sd->sc_data[SC_FLING].val1)/100;
		if(sd->sc_data[SC_MADNESSCANCEL].timer!=-1){ // }bhlXLZ[
			sd->base_atk += 100;
			aspd_rate -= 20;
		}
		if(sd->sc_data[SC_ADJUSTMENT].timer!=-1){ // AWXgg
			sd->hit -= 30;
			sd->flee += 30;
		}
		if(sd->sc_data[SC_INCREASING].timer!=-1) // CN[VOALAV[
			sd->hit += 20;
		if(sd->sc_data[SC_GATLINGFEVER].timer!=-1){ // KgOtB[o[
			aspd_rate -= 20;
			sd->flee -= sd->flee*(sd->sc_data[SC_GATLINGFEVER].val1*5)/100;
			sd->speed += sd->flee*(sd->sc_data[SC_GATLINGFEVER].val1*5)/100;
		}

		// Ï«
		if(sd->sc_data[SC_RESISTWATER].timer!=-1)
			sd->subele[1] += sd->sc_data[SC_RESISTWATER].val1;
		if(sd->sc_data[SC_RESISTGROUND].timer!=-1)
			sd->subele[2] += sd->sc_data[SC_RESISTGROUND].val1;
		if(sd->sc_data[SC_RESISTFIRE].timer!=-1)
			sd->subele[3] += sd->sc_data[SC_RESISTFIRE].val1;
		if(sd->sc_data[SC_RESISTWIND].timer!=-1)
			sd->subele[4] += sd->sc_data[SC_RESISTWIND].val1;
		if(sd->sc_data[SC_RESISTPOISON].timer!=-1)
			sd->subele[5] += sd->sc_data[SC_RESISTPOISON].val1;
		if(sd->sc_data[SC_RESISTHOLY].timer!=-1)
			sd->subele[6] += sd->sc_data[SC_RESISTHOLY].val1;
		if(sd->sc_data[SC_RESISTDARK].timer!=-1)
			sd->subele[7] += sd->sc_data[SC_RESISTDARK].val1;
		if(sd->sc_data[SC_RESISTTELEKINESIS].timer!=-1)
			sd->subele[8] += sd->sc_data[SC_RESISTTELEKINESIS].val1;
		if(sd->sc_data[SC_RESISTUNDEAD].timer!=-1)
			sd->subele[9] += sd->sc_data[SC_RESISTUNDEAD].val1;

		// Ï«
		if(sd->sc_data[SC_RESISTALL].timer!=-1){  // sgÌW[Nt[h
			sd->subele[1] += sd->sc_data[SC_RESISTALL].val1;
			sd->subele[2] += sd->sc_data[SC_RESISTALL].val1;
			sd->subele[3] += sd->sc_data[SC_RESISTALL].val1;
			sd->subele[4] += sd->sc_data[SC_RESISTALL].val1;
			sd->subele[5] += sd->sc_data[SC_RESISTALL].val1;	// SÄÉÏ«Á
			sd->subele[6] += sd->sc_data[SC_RESISTALL].val1;
			sd->subele[7] += sd->sc_data[SC_RESISTALL].val1;
			sd->subele[8] += sd->sc_data[SC_RESISTALL].val1;
			sd->subele[9] += sd->sc_data[SC_RESISTALL].val1;
		}
		// Ï«
		if(sd->sc_data[SC_SIEGFRIED].timer!=-1){  // sgÌW[Nt[h
			sd->subele[1] += sd->sc_data[SC_SIEGFRIED].val2;
			sd->subele[2] += sd->sc_data[SC_SIEGFRIED].val2;
			sd->subele[3] += sd->sc_data[SC_SIEGFRIED].val2;
			sd->subele[4] += sd->sc_data[SC_SIEGFRIED].val2;
			sd->subele[5] += sd->sc_data[SC_SIEGFRIED].val2;	// SÄÉÏ«Á
			sd->subele[6] += sd->sc_data[SC_SIEGFRIED].val2;
			sd->subele[7] += sd->sc_data[SC_SIEGFRIED].val2;
			sd->subele[8] += sd->sc_data[SC_SIEGFRIED].val2;
			sd->subele[9] += sd->sc_data[SC_SIEGFRIED].val2;
		}
		if(sd->sc_data[SC_PROVIDENCE].timer!=-1){	// vBfX
			sd->subele[6] += sd->sc_data[SC_PROVIDENCE].val2;	// Î ¹®«
			sd->subrace[6] += sd->sc_data[SC_PROVIDENCE].val2;	// Î «
		}

		// »Ì¼
		if(sd->sc_data[SC_BERSERK].timer!=-1){	// o[T[N
			sd->def = 0;
			sd->def2 = 0;
			sd->mdef = 0;
			sd->mdef2 = 0;
		}
		if(sd->sc_data[SC_JOINTBEAT].timer != -1) {	// WCgr[g
			switch (sd->sc_data[SC_JOINTBEAT].val4) {
				case 0:		// «ñ
					sd->speed += (sd->speed * 50)/100;
					break;
				case 1:		// èñ
					sd->aspd -= (sd->aspd * 25)/100;
					break;
				case 2:		// G
					sd->speed += (sd->speed * 30)/100;
					sd->aspd  -= (sd->aspd * 10)/100;
					break;
				case 3:		// ¨
					sd->def2 -= (sd->def2 * 50)/100;
					break;
				case 4:		// 
					sd->def2     -= (sd->def2 * 25)/100;
					sd->base_atk -= (sd->base_atk * 25)/100;
					break;
				case 5:		// ñ
					sd->critical_def -= (sd->critical_def * 50)/100;
					break;
			}
		}
		if(sd->sc_data[SC_APPLEIDUN].timer!=-1){	// ChDÌÑç
			sd->status.max_hp += ((5+sd->sc_data[SC_APPLEIDUN].val1*2+((sd->sc_data[SC_APPLEIDUN].val2+1)>>1)
						+sd->sc_data[SC_APPLEIDUN].val3/10) * sd->status.max_hp)/100;

		}else if(sd->sc_data[SC_APPLEIDUN_].timer!=-1){	// ChDÌÑç
			sd->status.max_hp += ((5+sd->sc_data[SC_APPLEIDUN_].val1*2+((sd->sc_data[SC_APPLEIDUN_].val2+1)>>1)
						+sd->sc_data[SC_APPLEIDUN_].val3/10) * sd->status.max_hp)/100;
		}

		if(sd->sc_data[SC_DELUGE].timer!=-1 && sd->def_ele==1){	// f[W
			sd->status.max_hp += sd->status.max_hp*sd->sc_data[SC_DELUGE].val3/100;
		}
		if(sd->sc_data[SC_SERVICE4U].timer!=-1) {	// T[rXtH[[
			sd->status.max_sp += sd->status.max_sp*(10+sd->sc_data[SC_SERVICE4U].val1+sd->sc_data[SC_SERVICE4U].val2
						+sd->sc_data[SC_SERVICE4U].val3)/100;

			sd->dsprate-=(10+sd->sc_data[SC_SERVICE4U].val1*3+sd->sc_data[SC_SERVICE4U].val2
					+sd->sc_data[SC_SERVICE4U].val3);
			if(sd->dsprate<0)sd->dsprate=0;
		}else if(sd->sc_data[SC_SERVICE4U_].timer!=-1) {	// T[rXtH[[
			sd->status.max_sp += sd->status.max_sp*(10+sd->sc_data[SC_SERVICE4U_].val1+sd->sc_data[SC_SERVICE4U_].val2
						+sd->sc_data[SC_SERVICE4U_].val3)/100;

			sd->dsprate-=(10+sd->sc_data[SC_SERVICE4U_].val1*3+sd->sc_data[SC_SERVICE4U_].val2
					+sd->sc_data[SC_SERVICE4U_].val3);
			if(sd->dsprate<0)sd->dsprate=0;
		}

		if(sd->sc_data[SC_FORTUNE].timer!=-1){	// K^ÌLX
			sd->critical += (10+sd->sc_data[SC_FORTUNE].val1+sd->sc_data[SC_FORTUNE].val2
						+sd->sc_data[SC_FORTUNE].val3)*10;
		}else if(sd->sc_data[SC_FORTUNE_].timer!=-1){	// K^ÌLX
			sd->critical += (10+sd->sc_data[SC_FORTUNE_].val1+sd->sc_data[SC_FORTUNE_].val2
						+sd->sc_data[SC_FORTUNE_].val3)*10;
		}

		if(sd->sc_data[SC_EXPLOSIONSPIRITS].timer!=-1){	// ôg®
			if(s_class.job==23)
				sd->critical += sd->sc_data[SC_EXPLOSIONSPIRITS].val1*100;
			else
				sd->critical += sd->sc_data[SC_EXPLOSIONSPIRITS].val2;
		}

		if(sd->sc_data[SC_STEELBODY].timer!=-1){	// à
			sd->def = 90;
			sd->mdef = 90;
			aspd_rate += 25;
			sd->speed = (sd->speed * 135) / 100;
		}
		if(sd->sc_data[SC_DEFENDER].timer != -1) {	// fBtF_[
			sd->aspd += (25 - sd->sc_data[SC_DEFENDER].val1*5);
			sd->speed = (sd->speed * (155 - sd->sc_data[SC_DEFENDER].val1*5)) / 100;
		}
		if(sd->sc_data[SC_ENCPOISON].timer != -1)
			sd->addeff[4] += sd->sc_data[SC_ENCPOISON].val2;

		if (sd->sc_data[SC_DANCING].timer != -1 && sd->sc_data[SC_BARDDANCER].timer == -1) // xè/t
		{
			if (sd->sc_data[SC_LONGINGFREEDOM].timer != -1)
			{
				if (sd->sc_data[SC_LONGINGFREEDOM].val1 < 5)
				{
					sd->speed = sd->speed * (200-20*sd->sc_data[SC_LONGINGFREEDOM].val1)/100;
					sd->aspd  = sd->aspd * (200-20*sd->sc_data[SC_LONGINGFREEDOM].val1)/100;
				}
			}
			else
			{
				int lesson = pc_checkskill(sd,BA_MUSICALLESSON) > pc_checkskill(sd,DC_DANCINGLESSON) ?
					pc_checkskill(sd,BA_MUSICALLESSON) : pc_checkskill(sd,DC_DANCINGLESSON);
				sd->speed = sd->speed * (400-20*lesson)/100;
			}
		}

		if(sd->sc_data[SC_CURSE].timer!=-1){
			if(sd->sc_data[SC_DEFENDER].timer != -1)//fBtF_[Íô¢Å¬xáºµÈ¢
				sd->speed += 0;
			else
				sd->speed += 450;
		}
		if(sd->sc_data[SC_TRUESIGHT].timer!=-1) //gD[TCg
			sd->critical += 10*(sd->sc_data[SC_TRUESIGHT].val1);

/*		if(sd->sc_data[SC_VOLCANO].timer!=-1)	// G`g|CY(®«Íbattle.cÅ)
			sd->addeff[2]+=sd->sc_data[SC_VOLCANO].val2;//% of granting
		if(sd->sc_data[SC_DELUGE].timer!=-1)	// G`g|CY(®«Íbattle.cÅ)
			sd->addeff[0]+=sd->sc_data[SC_DELUGE].val2;//% of granting
		*/
	}
	//eRJ[{[iX
	if(sd->status.class==PC_CLASS_TK && sd->status.base_level>=90 && ranking_get_pc_rank(sd,RK_TAEKWON)>0)
	{
		sd->status.max_hp*=3;
		sd->status.max_sp*=3;
	}

	if(sd->speed_rate != 100)
		sd->speed = sd->speed*sd->speed_rate/100;
	if(sd->speed < 1) sd->speed = 1;
	if(aspd_rate != 100)
		sd->aspd = sd->aspd*aspd_rate/100;
	if(pc_isriding(sd))							// RºCû
		sd->aspd = sd->aspd*(100 + 10*(5 - pc_checkskill(sd,KN_CAVALIERMASTERY)))/ 100;
	if(sd->aspd < battle_config.max_aspd) sd->aspd = battle_config.max_aspd;
	sd->amotion = sd->aspd;
	sd->dmotion = 800-sd->paramc[1]*4;
	if(sd->dmotion<400)
		sd->dmotion = 400;
	if(sd->ud.skilltimer != -1 && (skill = pc_checkskill(sd,SA_FREECAST)) > 0) {
		sd->prev_speed = sd->speed;
		sd->speed = sd->speed*(175 - skill*5)/100;
	}

	if(sd->matk_rate!=100){
		sd->matk1 = sd->matk1*sd->matk_rate/100;
		sd->matk2 = sd->matk2*sd->matk_rate/100;
	}
	if(sd->matk1<1)
	 	sd->matk1 = 1;
	if(sd->matk2<1)
		sd->matk2 = 1;

	if(sd->status.max_hp>battle_config.max_hp)
		sd->status.max_hp=battle_config.max_hp;
	if(sd->status.max_sp>battle_config.max_sp)
		sd->status.max_sp=battle_config.max_sp;

	if(sd->status.max_hp<=0)
		sd->status.max_hp=1;
	if(sd->status.max_sp<=0)
		sd->status.max_sp=1;

	if(sd->status.hp>sd->status.max_hp)
		sd->status.hp=sd->status.max_hp;
	if(sd->status.sp>sd->status.max_sp)
		sd->status.sp=sd->status.max_sp;

	// vZ±±ÜÅ
	if( sd->status_calc_pc_process > 1 ) {
		// ±ÌÖªÄAIÉÄÎê½ÌÅAÄvZ·é
		if( --calclimit ) {
			sd->status_calc_pc_process = 1;
			goto L_RECALC;
		} else {
			// ³À[vÉÈÁ½ÌÅvZÅ¿Øè
			printf("status_calc_pc: infinity loop!\n");
		}
	}
	sd->status_calc_pc_process = 0;

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
		clif_skillinfoblock(sd);	// XLM

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
		(sd->sc_data[SC_PROVOKE].timer==-1 || sd->sc_data[SC_PROVOKE].val2==0 ) && !unit_isdead(&sd->bl))
		// I[go[T[N­®
		status_change_start(&sd->bl,SC_PROVOKE,10,1,0,0,0,0);

	return 0;
}

/*==========================================
 * ÎÛÌgroupðÔ·(Äp)
 * ßèÍ®Å0Èã
 *------------------------------------------
 */
int status_get_group(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return mob_db[((struct mob_data *)bl)->class].group_id;
	//PC PETÍ0i¢Ýè)

	return 0;
}
/*==========================================
 * ÎÛÌClassðÔ·(Äp)
 * ßèÍ®Å0Èã
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
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return ((struct homun_data *)bl)->status.class;
	else
		return 0;
}
/*==========================================
 * ÎÛÌûüðÔ·(Äp)
 * ßèÍ®Å0Èã
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
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return ((struct homun_data *)bl)->dir;
	else
		return 0;
}
/*==========================================
 * ÎÛÌxðÔ·(Äp)
 * ßèÍ®Å0Èã
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
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return ((struct homun_data *)bl)->status.base_level;
	else
		return 0;
}

/*==========================================
 * ÎÛÌËöðÔ·(Äp)
 * ßèÍ®Å0Èã
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
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return 2;//((struct homun_data *)bl)->attackrange;
	else
		return 0;
}
/*==========================================
 * ÎÛÌHPðÔ·(Äp)
 * ßèÍ®Å0Èã
 *------------------------------------------
 */
int status_get_hp(struct block_list *bl)
{
	nullpo_retr(1, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return ((struct mob_data *)bl)->hp;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->status.hp;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return ((struct homun_data *)bl)->status.hp;
	else
		return 1;
}
/*==========================================
 * ÎÛÌSPðÔ·(Äp)
 * ßèÍ®Å0Èã
 *------------------------------------------
 */
int status_get_sp(struct block_list *bl)
{
	nullpo_retr(1, bl);

	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->status.sp;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return ((struct homun_data *)bl)->status.sp;
	else
		return 0;
}
/*==========================================
 * ÎÛÌMHPðÔ·(Äp)
 * ßèÍ®Å0Èã
 *------------------------------------------
 */
int status_get_max_hp(struct block_list *bl)
{
	nullpo_retr(1, bl);
	if(bl->type==BL_PC && ((struct map_session_data *)bl))
		return ((struct map_session_data *)bl)->status.max_hp;
	else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		return ((struct homun_data *)bl)->max_hp;
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
 * ÎÛÌStrðÔ·(Äp)
 * ßèÍ®Å0Èã
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
	else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		str = ((struct homun_data *)bl)->status.str;

	if(sc_data && bl->type!=BL_HOM) {
		if(sc_data[SC_LOUD].timer!=-1 && sc_data[SC_QUAGMIRE].timer == -1 && bl->type != BL_PC)
			str += 4;
		if( sc_data[SC_BLESSING].timer != -1 && bl->type != BL_PC){	// ubVO
			int race=status_get_race(bl);
			if(battle_check_undead(race,status_get_elem_type(bl)) || race==6 )	str >>= 1;	// « /s
			else str += sc_data[SC_BLESSING].val1;	// »Ì¼
		}
		if(sc_data[SC_TRUESIGHT].timer!=-1 && bl->type != BL_PC)	// gD[TCg
			str += 5;
		if(sc_data[SC_CHASEWALK_STR].timer!=-1)
			str += sc_data[SC_CHASEWALK_STR].val1;
	}
	if(str < 0) str = 0;
	return str;
}
/*==========================================
 * ÎÛÌAgiðÔ·(Äp)
 * ßèÍ®Å0Èã
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
	else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		agi = ((struct homun_data *)bl)->agi;

	if(sc_data && bl->type!=BL_HOM) {
		if(sc_data[SC_INCFLEE].timer!=-1 && bl->type != BL_PC) //¬x­»
			agi *= 3;
		if( sc_data[SC_INCREASEAGI].timer!=-1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1 &&
			bl->type != BL_PC)	// ¬xÁ(PCÍpc.cÅ)
			agi += 2+sc_data[SC_INCREASEAGI].val1;

		if(sc_data[SC_SUITON].timer!=-1 && sc_data[SC_SUITON].val3!=0 && bl->type != BL_PC)//Ù
			agi += sc_data[SC_SUITON].val3;

		if(sc_data[SC_CONCENTRATE].timer!=-1 && sc_data[SC_QUAGMIRE].timer == -1 && bl->type != BL_PC)
			agi += agi*(2+sc_data[SC_CONCENTRATE].val1)/100;

		if(sc_data[SC_DECREASEAGI].timer!=-1)	// ¬x¸­
			agi -= 2+sc_data[SC_DECREASEAGI].val1;

		if(sc_data[SC_QUAGMIRE].timer!=-1 )	// N@O}CA
			agi >>= 1;
		if(sc_data[SC_TRUESIGHT].timer!=-1 && bl->type != BL_PC)	// gD[TCg
			agi += 5;
	}
	if(agi < 0) agi = 0;
	return agi;
}
/*==========================================
 * ÎÛÌVitðÔ·(Äp)
 * ßèÍ®Å0Èã
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
	else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		vit = ((struct homun_data *)bl)->vit;
	if(sc_data && bl->type!=BL_HOM) {
		if(sc_data[SC_STRIPARMOR].timer != -1 && bl->type!=BL_PC)
			vit = vit*60/100;
		if(sc_data[SC_TRUESIGHT].timer!=-1 && bl->type != BL_PC)	// gD[TCg
			vit += 5;
	}

	if(vit < 0) vit = 0;
	return vit;
}
/*==========================================
 * ÎÛÌIntðÔ·(Äp)
 * ßèÍ®Å0Èã
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
	else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		int_= ((struct homun_data *)bl)->int_;

	if(sc_data && bl->type!=BL_HOM) {
		if( sc_data[SC_BLESSING].timer != -1 && bl->type != BL_PC){	// ubVO
			int race=status_get_race(bl);
			if(battle_check_undead(race,status_get_elem_type(bl)) || race==6 )	int_ >>= 1;	// « /s
			else int_ += sc_data[SC_BLESSING].val1;	// »Ì¼
		}
		if( sc_data[SC_STRIPHELM].timer != -1 && bl->type != BL_PC)
			int_ = int_*90/100;
		if(sc_data[SC_TRUESIGHT].timer!=-1 && bl->type != BL_PC)	// gD[TCg
			int_ += 5;
	}
	if(int_ < 0) int_ = 0;
	return int_;
}
/*==========================================
 * ÎÛÌDexðÔ·(Äp)
 * ßèÍ®Å0Èã
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
	else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		dex = ((struct homun_data *)bl)->dex;

	if(sc_data && bl->type!=BL_HOM) {
		if(sc_data[SC_EXPLOSIONSPIRITS].timer!=-1 && bl->type != BL_PC)
			dex *= 3; //mobôg®
		if(sc_data[SC_CONCENTRATE].timer!=-1 && sc_data[SC_QUAGMIRE].timer == -1 && bl->type != BL_PC)
			dex += dex*(2+sc_data[SC_CONCENTRATE].val1)/100;

		if( sc_data[SC_BLESSING].timer != -1 && bl->type != BL_PC){	// ubVO
			int race=status_get_race(bl);
			if(battle_check_undead(race,status_get_elem_type(bl)) || race==6 )	dex >>= 1;	// « /s
			else dex += sc_data[SC_BLESSING].val1;	// »Ì¼
		}

		if(sc_data[SC_QUAGMIRE].timer!=-1 )	// N@O}CA
			dex >>= 1;
		if(sc_data[SC_TRUESIGHT].timer!=-1 && bl->type != BL_PC)	// gD[TCg
			dex += 5;

	}
	if(dex < 0) dex = 0;
	return dex;
}
/*==========================================
 * ÎÛÌLukðÔ·(Äp)
 * ßèÍ®Å0Èã
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
	else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		luk = ((struct homun_data *)bl)->luk;

	if(sc_data && bl->type!=BL_HOM) {
		if(sc_data[SC_GLORIA].timer!=-1 && bl->type != BL_PC)	// OA(PCÍpc.cÅ)
			luk += 30;
		if(sc_data[SC_CURSE].timer!=-1 )		// ô¢
			luk=0;
		if(sc_data[SC_TRUESIGHT].timer!=-1 && bl->type != BL_PC)	// gD[TCg
			luk += 5;
	}
	if(luk < 0) luk = 0;
	return luk;
}

/*==========================================
 * ÎÛÌFleeðÔ·(Äp)
 * ßèÍ®Å1Èã
 *------------------------------------------
 */
int status_get_flee(struct block_list *bl)
{
	int flee=1, target_count=1;
	struct status_change *sc_data;

	nullpo_retr(1, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		flee=((struct map_session_data *)bl)->flee;
	else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		flee = ((struct homun_data *)bl)->flee;
	else
		flee=status_get_agi(bl) + status_get_lv(bl);

	if(sc_data && bl->type!=BL_HOM) {
		if(sc_data[SC_WHISTLE].timer!=-1 && bl->type != BL_PC)
			flee += flee*(sc_data[SC_WHISTLE].val1+sc_data[SC_WHISTLE].val2
					+(sc_data[SC_WHISTLE].val3>>16))/100;
		if(sc_data[SC_BLIND].timer!=-1 && bl->type != BL_PC)
			flee -= flee*25/100;
		if(sc_data[SC_WINDWALK].timer!=-1 && bl->type != BL_PC) // EBhEH[N
			flee += flee*(sc_data[SC_WINDWALK].val2)/100;
		if(sc_data[SC_SPIDERWEB].timer!=-1 && bl->type != BL_PC) //XpC_[EFu
			flee -= flee*50/100;
			//THE SUN
		if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_SUN && bl->type != BL_PC)
			flee = flee*80/100;
		if(sc_data[SC_GATLINGFEVER].timer!=-1 && bl->type != BL_PC)	//KgOtB[o[
			flee = flee*(100-sc_data[SC_GATLINGFEVER].val1*5)/100;
		if(sc_data[SC_ADJUSTMENT].timer!=-1 && bl->type != BL_PC)	//AWXgg
			flee += 30;

	}

	// ñð¦â³
	if(bl->type!=BL_HOM)
		target_count = unit_counttargeted(bl,battle_config.agi_penaly_count_lv);

	if(battle_config.agi_penaly_type > 0 && target_count >= battle_config.agi_penaly_count) {
		//yieBÝèæèÎÛª½¢
		if(battle_config.agi_penaly_type == 1) {
			//ñð¦ªagi_penaly_num%¸Â¸­
			int flee_rate = (target_count-(battle_config.agi_penaly_count-1)) * battle_config.agi_penaly_num;
			flee = flee * (100 - flee_rate) / 100;
		} else if(battle_config.agi_penaly_type == 2) {
			//ñð¦ªagi_penaly_numª¸­
			flee -= (target_count - (battle_config.agi_penaly_count - 1))*battle_config.agi_penaly_num;
		}
	}
	// PvPMAPÅÌ¸­øÊ
	if(battle_config.gvg_flee_penaly & 1 && map[bl->m].flag.gvg ) {
		flee = flee*(200 - (battle_config.gvg_flee_rate))/100;//ÀÛÉGvGÅFleeð¸­
	} else if(battle_config.gvg_flee_penaly & 2 && map[bl->m].flag.pvp) {
		flee = flee*(200 - (battle_config.gvg_flee_rate))/100;//ÀÛÉPvPÅFleeð¸­
	}
	if(flee < 1) flee = 1;

	return flee;
}
/*==========================================
 * ÎÛÌHitðÔ·(Äp)
 * ßèÍ®Å1Èã
 *------------------------------------------
 */
int status_get_hit(struct block_list *bl)
{
	int hit=1;
	struct status_change *sc_data;

	nullpo_retr(1, bl);
	if (bl->type==BL_PC)
		return ((struct map_session_data *)bl)->hit;
	else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		return ((struct homun_data *)bl)->hit;
	else
		hit=status_get_dex(bl) + status_get_lv(bl);

	sc_data = status_get_sc_data(bl);
	if (sc_data) {
		if(sc_data[SC_HUMMING].timer!=-1)	//
			hit += hit*(sc_data[SC_HUMMING].val1*2+sc_data[SC_HUMMING].val2
					+sc_data[SC_HUMMING].val3)/100;
		if(sc_data[SC_TRUESIGHT].timer!=-1)		// gD[TCg
			hit += 3*(sc_data[SC_TRUESIGHT].val1);
		if(sc_data[SC_CONCENTRATION].timer!=-1) //RZg[V
			hit += 10*(sc_data[SC_CONCENTRATION].val1);
			//THE SUN
		if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_SUN && bl->type != BL_PC)
			hit = hit*80/100;
		if(sc_data[SC_ADJUSTMENT].timer!=-1 && bl->type != BL_PC) // AWXgg
			hit -= 30;
		if(sc_data[SC_INCREASING].timer!=-1 && bl->type != BL_PC) // CN[VOALAV[
			hit += 20;
	}
	if(hit < 1) hit = 1;
	return hit;
}
/*==========================================
 * ÎÛÌ®SñððÔ·(Äp)
 * ßèÍ®Å1Èã
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
 * ÎÛÌNeBJðÔ·(Äp)
 * ßèÍ®Å1Èã
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
	else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		critical = ((struct homun_data *)bl)->critical;
	else
		critical=status_get_luk(bl)*3 + 1;

	if(sc_data) {
		if(sc_data[SC_FORTUNE].timer!=-1 && bl->type != BL_PC)
			critical += (10+sc_data[SC_FORTUNE].val1+sc_data[SC_FORTUNE].val2
					+sc_data[SC_FORTUNE].val3)*10;
		if(sc_data[SC_EXPLOSIONSPIRITS].timer!=-1 && bl->type != BL_PC)
			critical += sc_data[SC_EXPLOSIONSPIRITS].val2;
		if(sc_data[SC_TRUESIGHT].timer!=-1 && bl->type != BL_PC) //gD[TCg
			critical += 10*sc_data[SC_TRUESIGHT].val1;
	}
	if(critical < 1) critical = 1;
	return critical;
}
/*==========================================
 * base_atkÌæ¾
 * ßèÍ®Å1Èã
 *------------------------------------------
 */
int status_get_baseatk(struct block_list *bl)
{
	struct status_change *sc_data;
	int batk=1;

	nullpo_retr(1, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl){
		batk = ((struct map_session_data *)bl)->base_atk; //Ýè³êÄ¢ébase_atk
		batk += ((struct map_session_data *)bl)->weapon_atk[((struct map_session_data *)bl)->status.weapon];
	}else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		batk = 1;
	else { //»êÈOÈç
		int str,dstr;
		str = status_get_str(bl); //STR
		dstr = str/10;
		batk = dstr*dstr + str; //base_atkðvZ·é
	}
	if(sc_data) { //óÔÙí è
		if(sc_data[SC_PROVOKE].timer!=-1 && bl->type != BL_PC) //PCÅv{bN(SM_PROVOKE)óÔ
			batk = batk*(100+2*sc_data[SC_PROVOKE].val1)/100; //base_atkÁ
		if(sc_data[SC_CURSE].timer!=-1 ) //ôíêÄ¢½ç
			batk -= batk*25/100; //base_atkª25%¸­
		if(sc_data[SC_CONCENTRATION].timer!=-1 && bl->type != BL_PC) //RZg[V
			batk += batk*(5*sc_data[SC_CONCENTRATION].val1)/100;
		if(sc_data[SC_JOINTBEAT].timer != -1 && sc_data[SC_JOINTBEAT].val4 == 4)	//WCgr[gÅ
			batk -= batk*25/100;
		if(sc_data[SC_MADNESSCANCEL].timer!=-1 && bl->type != BL_PC) //}bhlXLZ[
			batk += 100;
		//^bgøÊÍPÂ¾¯H
		if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_MAGICIAN)
				batk = batk*50/100;
		else if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_DEVIL)
				batk = batk*50/100;
		else if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_SUN)
				batk = batk*80/100;
		//GXN
		if(sc_data[SC_SKE].timer!=-1 && bl->type == BL_MOB)
			batk *= 4;
	}
	if(batk < 1) batk = 1; //base_atkÍÅáÅà1
	return batk;
}
/*==========================================
 * ÎÛÌAtkðÔ·(Äp)
 * ßèÍ®Å0Èã
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
	else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		atk = ((struct homun_data *)bl)->atk-((struct homun_data *)bl)->atk/10;
	else if(bl->type==BL_MOB && (struct mob_data *)bl)
	{
		int guardup_lv = ((struct mob_data*)bl)->guardup_lv;
		if(guardup_lv>0)
		{
			atk = mob_db[((struct mob_data*)bl)->class].atk1;
			atk += 1000*guardup_lv;
		}else
			atk = mob_db[((struct mob_data*)bl)->class].atk1;

	}else if(bl->type==BL_PET && (struct pet_data *)bl)
		atk = mob_db[((struct pet_data*)bl)->class].atk1;

	if(sc_data) {
		if(sc_data[SC_PROVOKE].timer!=-1 && bl->type != BL_PC)
			atk = atk*(100+2*sc_data[SC_PROVOKE].val1)/100;
		if(sc_data[SC_CURSE].timer!=-1 )
			atk -= atk*25/100;
		if(sc_data[SC_CONCENTRATION].timer!=-1 && bl->type != BL_PC) //RZg[V
			atk += atk*(5*sc_data[SC_CONCENTRATION].val1)/100;
		if(sc_data[SC_EXPLOSIONSPIRITS].timer!=-1 && bl->type != BL_PC)
			atk *= 3; //mobôg®
		if(sc_data[SC_STRIPWEAPON].timer!=-1 && bl->type != BL_PC)
			atk -= atk*10/100;
		if(sc_data[SC_DISARM].timer!=-1 && bl->type != BL_PC)		//fBXA[
			atk -= atk*25/100;
		if(sc_data[SC_MADNESSCANCEL].timer!=-1 && bl->type != BL_PC)	//}bhlXLZ[
			atk += 100;

		if(bl->type != BL_PC && sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_MAGICIAN)
				atk = atk*50/100;
		else if(bl->type != BL_PC && sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_DEVIL)
				atk = atk*50/100;
		else if(bl->type != BL_PC && sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_SUN)
				atk = atk*80/100;

		//GXN
		if(sc_data[SC_SKE].timer!=-1 && bl->type == BL_MOB)
			atk *=4;
	}
	if(atk < 0) atk = 0;
	return atk;
}
/*==========================================
 * ÎÛÌ¶èAtkðÔ·(Äp)
 * ßèÍ®Å0Èã
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
 * ÎÛÌAtk2ðÔ·(Äp)
 * ßèÍ®Å0Èã
 *------------------------------------------
 */
int status_get_atk2(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data*)bl)->watk2;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return ((struct homun_data *)bl)->atk;
	else {
		struct status_change *sc_data=status_get_sc_data(bl);
		int atk2=0;
		if(bl->type==BL_MOB && (struct mob_data *)bl){

			int guardup_lv = ((struct mob_data*)bl)->guardup_lv;
			if(guardup_lv>0)
			{
				atk2 = mob_db[((struct mob_data*)bl)->class].atk2;
				atk2 += 1000*guardup_lv;
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
			if(sc_data[SC_CONCENTRATION].timer!=-1) //RZg[V
				atk2 += atk2*(5*sc_data[SC_CONCENTRATION].val1)/100;
			if(sc_data[SC_EXPLOSIONSPIRITS].timer!=-1 && bl->type != BL_PC)
				atk2 *= 3; //mobôg®
			if(sc_data[SC_DISARM].timer!=-1 && bl->type != BL_PC)		//fBXA[
				atk2 -= atk2*25/100;
			if(sc_data[SC_MADNESSCANCEL].timer!=-1 && bl->type != BL_PC)	//}bhlXLZ[
				atk2 += 100;

			//^bgøÊ
			if(bl->type != BL_PC && sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_MAGICIAN)
					atk2 = atk2*50/100;
			else if(bl->type != BL_PC && sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_DEVIL)
					atk2 = atk2*50/100;
			else if(bl->type != BL_PC && sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_SUN)
					atk2 = atk2*80/100;
			//GXN
			if(sc_data[SC_SKE].timer!=-1 && bl->type == BL_MOB)
				atk2 *= 4;

		}
		if(atk2 < 0) atk2 = 0;
		return atk2;
	}
	return 0;
}
/*==========================================
 * ÎÛÌ¶èAtk2ðÔ·(Äp)
 * ßèÍ®Å0Èã
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
 * ÎÛÌMAtk1ðÔ·(Äp)
 * ßèÍ®Å0Èã
 *------------------------------------------
 */
int status_get_matk1(struct block_list *bl)
{
	struct status_change *sc_data;
	int matk1,int_;
	nullpo_retr(0, bl);

	if (bl->type==BL_PC)
		return ((struct map_session_data *)bl)->matk1;
	else if (bl->type==BL_HOM)
		return ((struct homun_data *)bl)->matk-((struct homun_data *)bl)->matk/10;
	else if (bl->type!=BL_PET && bl->type!=BL_MOB)
		return 0;

	int_=status_get_int(bl);
	matk1 = int_+(int_/5)*(int_/5);
	sc_data = status_get_sc_data(bl);
	if (sc_data) {
		if (sc_data[SC_MINDBREAKER].timer!=-1)
			matk1 += (matk1*20*sc_data[SC_MINDBREAKER].val1)/100;

		if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_STRENGTH)
			matk1 = matk1*50/100;
		else if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_DEVIL)
			matk1 = matk1*50/100;
		else if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_SUN)
			matk1 = matk1*80/100;
	}
	return matk1;
}
/*==========================================
 * ÎÛÌMAtk2ðÔ·(Äp)
 * ßèÍ®Å0Èã
 *------------------------------------------
 */
int status_get_matk2(struct block_list *bl)
{
	struct status_change *sc_data;
	int matk2,int_;
	nullpo_retr(0, bl);

	if (bl->type==BL_PC)
		return ((struct map_session_data *)bl)->matk2;
	else if (bl->type==BL_HOM)
		return ((struct homun_data *)bl)->matk;
	else if (bl->type!=BL_PET && bl->type!=BL_MOB)
		return 0;

	int_=status_get_int(bl);
	matk2 = int_+(int_/7)*(int_/7);
	sc_data = status_get_sc_data(bl);
	if (sc_data) {
		if (sc_data[SC_MINDBREAKER].timer!=-1)
			matk2 += (matk2*20*sc_data[SC_MINDBREAKER].val1)/100;

		if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_STRENGTH)
			matk2 = matk2*50/100;
		else if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_DEVIL)
			matk2 = matk2*50/100;
		else if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_SUN)
			matk2 = matk2*80/100;
	}
	return matk2;
}
/*==========================================
 * ÎÛÌDefðÔ·(Äp)
 * ßèÍ®Å0Èã
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
		skilltimer = ((struct map_session_data *)bl)->ud.skilltimer;
		skillid = ((struct map_session_data *)bl)->ud.skillid;
	}
	else if(bl->type==BL_MOB && (struct mob_data *)bl) {
		def = mob_db[((struct mob_data *)bl)->class].def;
		skilltimer = ((struct mob_data *)bl)->ud.skilltimer;
		skillid = ((struct mob_data *)bl)->ud.skillid;
	}
	else if(bl->type==BL_HOM && (struct homun_data *)bl) {
		def = 0;
		skilltimer = ((struct homun_data *)bl)->ud.skilltimer;
		skillid = ((struct homun_data *)bl)->ud.skillid;
	}
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		def = mob_db[((struct pet_data *)bl)->class].def;

	if(def < 1000000) {
		if(sc_data) {
			//L[sOÍDEF100
			if( sc_data[SC_KEEPING].timer!=-1)
				def *= 2;
			//v{bNÍ¸Z
			if( sc_data[SC_PROVOKE].timer!=-1 && bl->type != BL_PC)
				def = (def*(100 - 6*sc_data[SC_PROVOKE].val1)+50)/100;
			//í¾ÛÌ¿«ÍÁZ
			if( sc_data[SC_DRUMBATTLE].timer!=-1 && bl->type != BL_PC)
				def += sc_data[SC_DRUMBATTLE].val3;
			//ÅÉ©©ÁÄ¢éÍ¸Z
			if(sc_data[SC_POISON].timer!=-1 && bl->type != BL_PC)
				def = def*75/100;
			//XgbvV[hÍ¸Z
			if(sc_data[SC_STRIPSHIELD].timer!=-1 && bl->type != BL_PC)
				def = def*85/100;
			//VOiNVXÍ¸Z
			if(sc_data[SC_SIGNUMCRUCIS].timer!=-1 && bl->type != BL_PC)
				def = def * (100 - sc_data[SC_SIGNUMCRUCIS].val2)/100;
			//iÌ¬×ÍPCÈODEFª0ÉÈé
			if(sc_data[SC_ETERNALCHAOS].timer!=-1 && bl->type != BL_PC)
				def = 0;
			//AÎ»ÍEVtg
			if(sc_data[SC_FREEZE].timer != -1 || (sc_data[SC_STONE].timer != -1 && sc_data[SC_STONE].val2 == 0))
				def >>= 1;
			//RZg[VÍ¸Z
			if( sc_data[SC_CONCENTRATION].timer!=-1 && bl->type != BL_PC)
				def = (def*(100 - 5*sc_data[SC_CONCENTRATION].val1))/100;
			//NPCfBtF_[
			if(sc_data[SC_NPC_DEFENDER].timer!=-1 && bl->type != BL_PC)
				def += 100;
			//THE SUN
			if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_SUN && bl->type != BL_PC)
				def = def*80/100;
			if(sc_data[SC_FLING].timer!=-1 && bl->type != BL_PC)			// tCO
				def = def * (100 - 5*sc_data[SC_FLING].val1)/100;
			//GXN
			if(sc_data[SC_SKE].timer!=-1 && bl->type == BL_MOB)
				def = def/2;
			//GXJ
			if(sc_data[SC_SKA].timer!=-1 && bl->type == BL_MOB)
				def = 90;
		}
		//r¥Ír¥¸Z¦ÉîÃ¢Ä¸Z
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
 * ÎÛÌMDefðÔ·(Äp)
 * ßèÍ®Å0Èã
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
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		mdef = ((struct homun_data *)bl)->mdef;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		mdef = mob_db[((struct pet_data *)bl)->class].mdef;

	if(mdef < 1000000) {
		if(sc_data) {
			//GXJ
			if(sc_data[SC_SKA].timer!=-1 && bl->type == BL_MOB)
				mdef = 90;
			//oA[óÔÍMDEF100
			if(sc_data[SC_BARRIER].timer != -1)
				mdef += 100;
			//AÎ»Í1.25{
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
 * ÎÛÌDef2ðÔ·(Äp)
 * ßèÍ®Å1Èã
 *------------------------------------------
 */
int status_get_def2(struct block_list *bl)
{
	struct status_change *sc_data;
	int def2=1;

	nullpo_retr(1, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		def2 = ((struct map_session_data *)bl)->def2;
	else if(bl->type==BL_MOB && (struct mob_data *)bl)
		def2 = mob_db[((struct mob_data *)bl)->class].vit;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		def2 = mob_db[((struct pet_data *)bl)->class].vit;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		def2 = ((struct homun_data *)bl)->def;

	if(sc_data) {
		if( sc_data[SC_ANGELUS].timer!=-1 && bl->type != BL_PC)
			def2 = def2*(110+5*sc_data[SC_ANGELUS].val1)/100;
		if( sc_data[SC_PROVOKE].timer!=-1 && bl->type != BL_PC)
			def2 = (def2*(100 - 6*sc_data[SC_PROVOKE].val1)+50)/100;
		if(sc_data[SC_POISON].timer!=-1 && bl->type != BL_PC)
			def2 = def2*75/100;
		//RZg[VÍ¸Z
		if( sc_data[SC_CONCENTRATION].timer!=-1 && bl->type != BL_PC)
			def2 = def2*(100 - 5*sc_data[SC_CONCENTRATION].val1)/100;
		//WCgr[gÈç¸Z
		if(sc_data[SC_JOINTBEAT].timer != -1) {
			if(sc_data[SC_JOINTBEAT].val4 == 3)	//¨
				def2 -= def2*50/100;
			if(sc_data[SC_JOINTBEAT].val4 == 4)	//
				def2 -= def2*25/100;
		}
		//iÌ¬×ÍDEF2ª0ÉÈé
		if(sc_data[SC_ETERNALCHAOS].timer!=-1)
			def2 = 0;
		//THE SUN
		if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_SUN && bl->type != BL_PC)
			def2 = def2*80/100;
	}
	if(def2 < 1) def2 = 1;
	return def2;
}
/*==========================================
 * ÎÛÌMDef2ðÔ·(Äp)
 * ßèÍ®Å0Èã
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
	else if (bl->type==BL_HOM && (struct homun_data *)bl)
		return ((struct homun_data *)bl)->mdef;
	else
		return 0;
}
/*==========================================
 * ÎÛÌSpeed(Ú®¬x)ðÔ·(Äp)
 * ßèÍ®Å1Èã
 * SpeedÍ¬³¢Ù¤ªÚ®¬xª¬¢
 *------------------------------------------
 */
int status_get_speed(struct block_list *bl)
{
	nullpo_retr(1000, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->speed;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
	{
		if(battle_config.homun_speed_is_same_as_pc)
			return ((struct homun_data *)bl)->msd->speed;
		else
			return ((struct homun_data *)bl)->speed;
	}else {
		struct status_change *sc_data=status_get_sc_data(bl);
		int speed = 1000;
		if(bl->type==BL_MOB && (struct mob_data *)bl)
//			speed = mob_db[((struct mob_data *)bl)->class].speed;
			speed = ((struct mob_data *)bl)->speed;
		else if(bl->type==BL_PET && (struct pet_data *)bl)
			speed = ((struct pet_data *)bl)->speed;

		if(sc_data) {
			//àÍ35%ÁZ
			if(sc_data[SC_STEELBODY].timer!=-1)
				speed = speed*135/100;
			//¬xÁÍ25%¸Z(àÍ³ø)
			else if(sc_data[SC_INCREASEAGI].timer!=-1)
				speed -= speed*25/100;
			//EBhEH[NÍLv*2%¸Z(¬xÁÍ³ø)
			else if(sc_data[SC_WINDWALK].timer!=-1)
				speed -= (speed*(sc_data[SC_WINDWALK].val1*2))/100;
			//¬x­»Í50%¸Z
			if(sc_data[SC_INCFLEE].timer!=-1 && bl->type != BL_PC)
				speed -= speed*50/100;
			//¬x¸­Í33`50%ÁZ
			if(sc_data[SC_DECREASEAGI].timer!=-1 && sc_data[SC_DEFENDER].timer == -1)//ifBtF_[ÍÁZ³µj
				speed = speed*((sc_data[SC_DECREASEAGI].val1>5)?150:133)/100;
			//N@O}CAÍ1/3ÁZ
			if(sc_data[SC_QUAGMIRE].timer!=-1)
				speed = speed*4/3;
			//ðYêÈ¢ÅcÍÁZ
			if(sc_data[SC_DONTFORGETME].timer!=-1)
				speed = speed*(100+sc_data[SC_DONTFORGETME].val1*2 + sc_data[SC_DONTFORGETME].val2 + (sc_data[SC_DONTFORGETME].val3&0xffff))/100;
			//fBtF_[ÍÁZ
			if(sc_data[SC_DEFENDER].timer!=-1)
				speed = (speed * (155 - sc_data[SC_DEFENDER].val1*5)) / 100;
			// xèÍ4{x¢
			if (sc_data[SC_DANCING].timer != -1)
				speed *= 4;
			// WCgr[gÈçÁZ
			if(sc_data[SC_JOINTBEAT].timer != -1) {
				if(sc_data[SC_JOINTBEAT].val4 == 0)	//«ñ
					speed += speed*50/100;
				if(sc_data[SC_JOINTBEAT].val4 == 2)	//G
					speed += speed*30/100;
			}
			//GXEÍ4{x¢
			if(sc_data[SC_SWOO].timer!=-1 )
				speed*=4;
			//Ore[VtB[h
			if(sc_data[SC_GRAVITATION].timer!=-1&&battle_config.enemy_gravitation_type)
				speed = speed*(100+sc_data[SC_GRAVITATION].val1*5)/100;
			//ô¢Í450ÁZifBtF_[Í¬xáº³µj
			if(sc_data[SC_CURSE].timer!=-1){
				if(sc_data[SC_DEFENDER].timer != -1)
					speed += 0;
				else
					speed += 450;
			}
			if(sc_data[SC_SUITON].timer!=-1 && sc_data[SC_SUITON].val4==1)//Ù
				speed *= 2;
			//KgOtB[o[
			if(sc_data[SC_GATLINGFEVER].timer!=-1 )
				speed = speed*(100+sc_data[SC_GATLINGFEVER].val1*5)/100;
//			if(sc_data[SC_CURSE].timer!=-1)
//				speed = speed + 450;
		}
		if(speed < 1) speed = 1;
		return speed;
	}

	return 1000;
}
/*==========================================
 * ÎÛÌaDelay(UfBC)ðÔ·(Äp)
 * aDelayÍ¬³¢Ù¤ªU¬xª¬¢
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
		}
		else if(bl->type==BL_PET && (struct pet_data *)bl)
			adelay = mob_db[((struct pet_data *)bl)->class].adelay;
		else if(bl->type==BL_HOM && (struct homun_data *)bl)
			adelay = (((struct homun_data *)bl)->aspd<<1);

		if(sc_data) {
			//c[nhNCbPgpÅN@O}CAÅàðYêÈ¢ÅcÅàÈ¢Í3¸Z
			if(sc_data[SC_TWOHANDQUICKEN].timer != -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// 2HQ
				aspd_rate -= 30;
			//nhNCbPgpÅN@O}CAÅàðYêÈ¢ÅcÅàÈ¢Í3¸Z
			if(sc_data[SC_ONEHAND].timer != -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// 1HQ
				aspd_rate -= 30;
			//AhibVgpÅc[nhNCbPÅàN@O}CAÅàðYêÈ¢ÅcÅàÈ¢Í
			if(sc_data[SC_ADRENALINE2].timer != -1 && sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_ONEHAND].timer == -1 &&
				sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1) {	// AhibV
				//gpÒÆp[eBo[Åi·ªoéÝèÅÈ¯êÎ3¸Z
				if(sc_data[SC_ADRENALINE2].val2 || !battle_config.party_skill_penaly)
					aspd_rate -= 30;
				//»¤ÅÈ¯êÎ2.5¸Z
				else
					aspd_rate -= 25;
			}else if(sc_data[SC_ADRENALINE].timer != -1 && sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_ONEHAND].timer == -1 &&
				sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1) {	// AhibV
				//gpÒÆp[eBo[Åi·ªoéÝèÅÈ¯êÎ3¸Z
				if(sc_data[SC_ADRENALINE].val2 || !battle_config.party_skill_penaly)
					aspd_rate -= 30;
				//»¤ÅÈ¯êÎ2.5¸Z
				else
					aspd_rate -= 25;
			}
			//XsANBbPÍ¸Z
			if(sc_data[SC_SPEARSQUICKEN].timer != -1 && sc_data[SC_ADRENALINE].timer == -1 && sc_data[SC_ADRENALINE2].timer == -1 &&
				sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_ONEHAND].timer == -1 &&
				sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// XsANBbP
				aspd_rate -= sc_data[SC_SPEARSQUICKEN].val2;
			//[úÌATVNXÍ¸Z
			if(sc_data[SC_ASSNCROS].timer!=-1 && // [zÌATVNX
				sc_data[SC_TWOHANDQUICKEN].timer==-1 && sc_data[SC_ONEHAND].timer == -1 &&
				sc_data[SC_ADRENALINE].timer==-1 && sc_data[SC_ADRENALINE2].timer==-1 && sc_data[SC_SPEARSQUICKEN].timer==-1 &&
				sc_data[SC_DONTFORGETME].timer == -1)
				aspd_rate -= 5+sc_data[SC_ASSNCROS].val1+sc_data[SC_ASSNCROS].val2+sc_data[SC_ASSNCROS].val3;
			//ðYêÈ¢ÅcÍÁZ
			if(sc_data[SC_DONTFORGETME].timer!=-1)		// ðYêÈ¢Å
				aspd_rate += sc_data[SC_DONTFORGETME].val1*3 + sc_data[SC_DONTFORGETME].val2 + (sc_data[SC_DONTFORGETME].val3>>16);
			//à25%ÁZ
			if(sc_data[SC_STEELBODY].timer!=-1)	// à
				aspd_rate += 25;
			//ÅòÌrgpÍ¸Z
			if(	sc_data[SC_POISONPOTION].timer!=-1)
				aspd_rate -= 25;
			//¬|[VgpÍ¸Z
			if(	sc_data[i=SC_SPEEDPOTION2].timer!=-1 || sc_data[i=SC_SPEEDPOTION1].timer!=-1 || sc_data[i=SC_SPEEDPOTION0].timer!=-1)
				aspd_rate -= sc_data[i].val2;
			//fBtF_[ÍÁZaspd_rateÉÏX&}X^[ÁZ0É
			if(sc_data[SC_DEFENDER].timer != -1)
				aspd_rate += (25 - sc_data[SC_DEFENDER].val1*5);
			//WCgr[gÈçÁZ
			if(sc_data[SC_JOINTBEAT].timer != 1) {
				if(sc_data[SC_JOINTBEAT].val4 == 1)	//èñ
					aspd_rate += aspd_rate*25/100;
				if(sc_data[SC_JOINTBEAT].val4 == 2)	//G
					aspd_rate += aspd_rate*10/100;
			}
			//Ore[V
			if(sc_data[SC_GRAVITATION].timer!=-1)
				aspd_rate += sc_data[SC_GRAVITATION].val1*5;
			//KgOtB[o[
			if(sc_data[SC_GATLINGFEVER].timer!=-1)
				aspd_rate -= sc_data[SC_GATLINGFEVER].val1*2;
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
		else if(bl->type==BL_HOM && (struct homun_data *)bl && ((struct homun_data *)bl)->msd)
			amotion = ((struct homun_data *)bl)->aspd;

		if(sc_data) {
			if(sc_data[SC_TWOHANDQUICKEN].timer != -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// 2HQ
				aspd_rate -= 30;
			if(sc_data[SC_ONEHAND].timer != -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// 1HQ
				aspd_rate -= 30;

			if(sc_data[SC_ADRENALINE2].timer != -1 && sc_data[SC_TWOHANDQUICKEN].timer == -1 &&
				sc_data[SC_ONEHAND].timer == -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1) {	// tAhibV
				if(sc_data[SC_ADRENALINE2].val2 || !battle_config.party_skill_penaly)
					aspd_rate -= 30;
				else
					aspd_rate -= 25;
			}else if(sc_data[SC_ADRENALINE].timer != -1 && sc_data[SC_TWOHANDQUICKEN].timer == -1 &&
				sc_data[SC_ONEHAND].timer == -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1) {	// AhibV
				if(sc_data[SC_ADRENALINE].val2 || !battle_config.party_skill_penaly)
					aspd_rate -= 30;
				else
					aspd_rate -= 25;
			}
			if(sc_data[SC_SPEARSQUICKEN].timer != -1 && sc_data[SC_ADRENALINE].timer == -1 && sc_data[SC_ADRENALINE2].timer == -1 &&
				sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_ONEHAND].timer==-1 &&
				sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// XsANBbP
				aspd_rate -= sc_data[SC_SPEARSQUICKEN].val2;
			if(sc_data[SC_ASSNCROS].timer!=-1 && // [zÌATVNX
				sc_data[SC_TWOHANDQUICKEN].timer==-1 && sc_data[SC_ONEHAND].timer==-1 &&
				sc_data[SC_ADRENALINE].timer==-1 && sc_data[SC_ADRENALINE2].timer==-1 && sc_data[SC_SPEARSQUICKEN].timer==-1 &&
				sc_data[SC_DONTFORGETME].timer == -1)
				aspd_rate -= 5+sc_data[SC_ASSNCROS].val1+sc_data[SC_ASSNCROS].val2+sc_data[SC_ASSNCROS].val3;
			if(sc_data[SC_DONTFORGETME].timer!=-1)		// ðYêÈ¢Å
				aspd_rate += sc_data[SC_DONTFORGETME].val1*3 + sc_data[SC_DONTFORGETME].val2 + (sc_data[SC_DONTFORGETME].val3>>16);
			if(	sc_data[SC_POISONPOTION].timer!=-1)
				aspd_rate -= 25;
			if(sc_data[SC_STEELBODY].timer!=-1)	// à
				aspd_rate += 25;
			if(	sc_data[i=SC_SPEEDPOTION2].timer!=-1 || sc_data[i=SC_SPEEDPOTION1].timer!=-1 || sc_data[i=SC_SPEEDPOTION0].timer!=-1)
				aspd_rate -= sc_data[i].val2;
			//fBtF_[ÍÁZASPDÉÏX&}X^[ÁZ0É
			if(sc_data[SC_DEFENDER].timer != -1)
				aspd_rate += (25 - sc_data[SC_DEFENDER].val1*5);
			//WCgr[gÈçÁZ
			if(sc_data[SC_JOINTBEAT].timer != 1) {
				if(sc_data[SC_JOINTBEAT].val4 == 1)	//èñ
					aspd_rate += aspd_rate*25/100;
				if(sc_data[SC_JOINTBEAT].val4 == 2)	//G
					aspd_rate += aspd_rate*10/100;
			}
			if(sc_data[SC_GRAVITATION].timer!=-1)
				aspd_rate += sc_data[SC_GRAVITATION].val1*5;
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
	else if(bl->type==BL_HOM && (struct homun_data *)bl && ((struct homun_data *)bl)->msd){
		ret = 800 - ((struct homun_data *)bl)->status.agi*4;
		if( ret < 400 )
			ret = 400;
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
	if(bl->type==BL_MOB && (struct mob_data *)bl)	// 10ÌÊLv*2APÌÊ®«
		ret=((struct mob_data *)bl)->def_ele;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		ret=20+((struct map_session_data *)bl)->def_ele;	// hä®«Lv1
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		ret = mob_db[((struct pet_data *)bl)->class].element;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		ret = homun_db[((struct homun_data *)bl)->status.class-HOM_ID].element;

	if(sc_data) {
		if( sc_data[SC_BENEDICTIO].timer!=-1 )	// ¹Ì~
			ret=26;
		if( sc_data[SC_ELEMENTWATER].timer!=-1 )	// 
			ret=20*sc_data[SC_ELEMENTWATER].val1 + 1;
		if( sc_data[SC_ELEMENTGROUND].timer!=-1 )	// y
			ret=20*sc_data[SC_ELEMENTGROUND].val1 + 2;
		if( sc_data[SC_ELEMENTFIRE].timer!=-1 )		// Î
			ret=20*sc_data[SC_ELEMENTFIRE].val1 + 3;
		if( sc_data[SC_ELEMENTWIND].timer!=-1 )		// 
			ret=20*sc_data[SC_ELEMENTWIND].val1 + 4;
		if( sc_data[SC_ELEMENTPOISON].timer!=-1 )	// Å
			ret=20*sc_data[SC_ELEMENTPOISON].val1 + 5;
		if( sc_data[SC_ELEMENTHOLY].timer!=-1 )	// ¹
			ret=20*sc_data[SC_ELEMENTHOLY].val1 + 6;
		if( sc_data[SC_ELEMENTDARK].timer!=-1 )		// Å
			ret=20*sc_data[SC_ELEMENTDARK].val1 + 7;
		if( sc_data[SC_ELEMENTELEKINESIS].timer!=-1 )	// O
			ret=20*sc_data[SC_ELEMENTELEKINESIS].val1 + 8;
		if( sc_data[SC_ELEMENTUNDEAD].timer!=-1 )	// s
			ret=20*sc_data[SC_ELEMENTUNDEAD].val1 + 9;
		if( sc_data[SC_FREEZE].timer!=-1 )	// 
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
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		ret=-1;// ³®«

	if(sc_data) {
		if( sc_data[SC_FROSTWEAPON].timer!=-1)		// tXgEF|
			ret=1;
		if( sc_data[SC_SEISMICWEAPON].timer!=-1)	// TCY~bNEF|
			ret=2;
		if( sc_data[SC_FLAMELAUNCHER].timer!=-1)	// t[`[
			ret=3;
		if( sc_data[SC_LIGHTNINGLOADER].timer!=-1)	// CgjO[_[
			ret=4;
		if( sc_data[SC_ENCPOISON].timer!=-1)		// G`g|CY
			ret=5;
		if( sc_data[SC_ASPERSIO].timer!=-1)			// AXyVI
			ret=6;
		if( sc_data[SC_DARKELEMENT].timer!=-1)		// Å®«
			ret=7;
		if( sc_data[SC_ATTENELEMENT].timer!=-1)		// O®«
			ret=8;
		if( sc_data[SC_UNDEADELEMENT].timer!=-1)	// s®«
			ret=9;
	}
	return ret;
}
int status_get_attack_element2(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl) {
		int ret = ((struct map_session_data *)bl)->atk_ele_;
		struct status_change *sc_data = status_get_sc_data(bl);

		if(sc_data) {

			if( sc_data[SC_FROSTWEAPON].timer!=-1)		// tXgEF|
				ret=1;
			if( sc_data[SC_SEISMICWEAPON].timer!=-1)	// TCY~bNEF|
				ret=2;
			if( sc_data[SC_FLAMELAUNCHER].timer!=-1)	// t[`[
				ret=3;
			if( sc_data[SC_LIGHTNINGLOADER].timer!=-1)	// CgjO[_[
				ret=4;
			if( sc_data[SC_ENCPOISON].timer!=-1)		// G`g|CY
				ret=5;
			if( sc_data[SC_ASPERSIO].timer!=-1)			// AXyVI
				ret=6;
			if( sc_data[SC_DARKELEMENT].timer!=-1)		// Å®«
				ret=7;
			if( sc_data[SC_ATTENELEMENT].timer!=-1)		// O®«
				ret=8;
			if( sc_data[SC_UNDEADELEMENT].timer!=-1)	// s®«
				ret=9;

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
	else if(bl->type==BL_HOM && (struct homun_data *)bl){
		//struct homun_data *hd = (struct homun_data *)bl;
		//return status_get_party_id(&hd->msd->bl);
		return 0;
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
	else if(bl->type==BL_HOM && (struct homun_data *)bl){
		//struct homun_data *hd = (struct homun_data *)bl;
		//return status_get_guild_id(&hd->msd->bl);
		return 0;
	}
	else if(bl->type==BL_SKILL && (struct skill_unit *)bl)
		return ((struct skill_unit *)bl)->group->guild_id;
	else
		return 0;
}
int status_get_race(struct block_list *bl)
{
	int race;
	struct status_change *sc_data;

	nullpo_retr(0, bl);

	if(bl->type==BL_MOB && (struct mob_data *)bl)
		race = mob_db[((struct mob_data *)bl)->class].race;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		race = ((struct map_session_data *)bl)->race;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return mob_db[((struct pet_data *)bl)->class].race;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return homun_db[((struct homun_data *)bl)->status.class-HOM_ID].race;
	else
		return 0;

	sc_data = status_get_sc_data(bl);

	if(sc_data) {
		if( sc_data[SC_RACEUNKNOWN].timer!=-1)	//³`
			race=0;
		if( sc_data[SC_RACEUNDEAD].timer!=-1)	//s
			race=1;
		if( sc_data[SC_RACEBEAST].timer!=-1)	//®¨
			race=2;
		if( sc_data[SC_RACEPLANT].timer!=-1)	//A¨
			race=3;
		if( sc_data[SC_RACEINSECT].timer!=-1)	//©
			race=4;
		if( sc_data[SC_RACEFISH].timer!=-1)		//L
			race=5;
		if( sc_data[SC_RACEDEVIL].timer!=-1)	//«
			race=6;
		if( sc_data[SC_RACEHUMAN].timer!=-1)	//lÔ
			race=7;
		if( sc_data[SC_RACEANGEL].timer!=-1)	//Vg
			race=8;
		if( sc_data[SC_RACEDRAGON].timer!=-1)	//³
			race=9;
	}

	return race;
}
int status_get_size(struct block_list *bl)
{
	nullpo_retr(1, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return mob_db[((struct mob_data *)bl)->class].size;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
	{
		if(pc_isbaby((struct map_session_data *)bl))
			return 0;
		else
			return 1;
	}else if(bl->type==BL_PET && (struct pet_data *)bl)
		return mob_db[((struct pet_data *)bl)->class].size;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return homun_db[((struct homun_data *)bl)->status.class-HOM_ID].size;
	else
		return 1;
}
int status_get_mode(struct block_list *bl)
{
	nullpo_retr(0x01, bl);
	if(bl->type==BL_MOB) {
		struct mob_data* md = (struct mob_data*)bl;
		return (md->mode ? md->mode : mob_db[md->class].mode);
	}
	else if(bl->type==BL_PET)
		return mob_db[((struct pet_data *)bl)->class].mode;
	else
		return 0x01;	// Æè ¦¸®­Æ¢¤±ÆÅ1
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

int status_get_enemy_type(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if( bl->type == BL_PC )
		return 1;
	else if( bl->type == BL_MOB && !(status_get_mode(bl)&0x20) )
		return 2;
	else if( bl->type == BL_HOM )
		return 3;
	else
		return 0;
}

// StatusChangenÌ¾
struct status_change *status_get_sc_data(struct block_list *bl)
{
	nullpo_retr(NULL, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return ((struct mob_data*)bl)->sc_data;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data*)bl)->sc_data;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return ((struct homun_data*)bl)->sc_data;
	return NULL;
}
short *status_get_sc_count(struct block_list *bl)
{
	nullpo_retr(NULL, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return &((struct mob_data*)bl)->sc_count;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return &((struct map_session_data*)bl)->sc_count;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return &((struct homun_data*)bl)->sc_count;
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
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
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
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
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
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
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
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return &((struct homun_data*)bl)->status.option;
	return 0;
}

int status_check_attackable_by_tigereye(struct block_list *bl)
{
	int mode,race;
	nullpo_retr(0, bl);
	mode = status_get_mode(bl);
	race = status_get_race(bl);
	if(race==4 || race==6)
		return 1;
	if(mode&0x20)
		return 1;
	return 0;
}

int status_check_tigereye(struct block_list *bl)
{
	struct map_session_data* sd =NULL;
	int mode,race;
	nullpo_retr(0, bl);
	mode = status_get_mode(bl);
	race = status_get_race(bl);
	BL_CAST( BL_PC , bl , sd );
	if(race==4 || race==6)
		return 1;
	if(mode&0x20)
		return 1;
	if(sd && (sd->sc_data[SC_TIGEREYE].timer!=-1 || sd->infinite_tigereye))
		return 1;
	return 0;
}

int status_check_no_magic_damage(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC)
	{
		if(((struct map_session_data*)bl)->special_state.no_magic_damage)
			return 1;
	}
	return 0;
}
#ifdef DYNAMIC_SC_DATA
int status_calloc_sc_data(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(status_check_dummy_sc_data(bl) == 0)
		return 0;
	if(bl->type == BL_MOB)
	{
		int i;
		struct mob_data* md = (struct mob_data*)bl;
		md->sc_data = (struct status_change *)aCalloc(MAX_STATUSCHANGE,sizeof(struct status_change));
		for(i=0;i<MAX_STATUSCHANGE;i++) {
			md->sc_data[i].timer=-1;
			md->sc_data[i].val1 = md->sc_data[i].val2 = md->sc_data[i].val3 = md->sc_data[i].val4 =0;
		}
		md->sc_count = 0;
	}
	return 0;
}
int status_free_sc_data(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type == BL_MOB && status_check_dummy_sc_data(bl)==0)
	{
		struct mob_data *md = (struct mob_data *)bl;
		map_freeblock(md->sc_data);
		md->sc_data = dummy_sc_data;
		md->sc_count = 0;
	}
	return 0;
}
int status_check_dummy_sc_data(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type == BL_MOB)
	{
		if(((struct mob_data *)bl)->sc_data == dummy_sc_data)
			return 1;
	}
	return 0;
}
#endif
/*==========================================
 * Xe[^XÙíJn
 *------------------------------------------
 */
int status_change_start(struct block_list *bl,int type,int val1,int val2,int val3,int val4,int tick,int flag)
{
	struct map_session_data *sd = NULL;
	struct mob_data *md = NULL;
	struct homun_data *hd = NULL;
	struct status_change* sc_data;
	short *sc_count, *option, *opt1, *opt2, *opt3;
	int opt_flag = 0, calc_flag = 0, updateflag = 0, race, mode, elem;
	int scdef=0,soul_through = 0;

	nullpo_retr(0, bl);
	if(bl->type == BL_SKILL)
		return 0;
	if(bl->type == BL_HOM && !battle_config.allow_homun_status_change)
	{
		if(type<SC_AVOID || SC_SPEED<type)
			return 0;
	}
#ifdef DYNAMIC_SC_DATA
	status_calloc_sc_data(bl);
#endif
	nullpo_retr(0, sc_data=status_get_sc_data(bl));
	nullpo_retr(0, sc_count=status_get_sc_count(bl));
	nullpo_retr(0, option=status_get_option(bl));
	nullpo_retr(0, opt1=status_get_opt1(bl));
	nullpo_retr(0, opt2=status_get_opt2(bl));
	nullpo_retr(0, opt3=status_get_opt3(bl));

	race=status_get_race(bl);
	mode=status_get_mode(bl);
	elem=status_get_elem_type(bl);

	if(type == SC_AETERNA && (sc_data[SC_STONE].timer != -1 || sc_data[SC_FREEZE].timer != -1) )
		return 0;

	//Áên
	if(type >= MAX_STATUSCHANGE)
	{
		switch(type)
		{
			case SC_SOUL:
				status_change_soulstart(bl,val1,val2,val3,val4,tick,flag);
				break;
			default:
				if(battle_config.error_log)
					printf("UnknownStatusChange [%d]\n", type);
				break;
		}
		return 0;
	}

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
		case SC_BLEED:	// Ú×s¾ÈÌÅÆè ¦¸±±Å
			scdef=3+status_get_vit(bl)+status_get_luk(bl)/3;
			break;
		case SC_SLEEP:
		case SC_BLIND:
			scdef=3+status_get_int(bl)+status_get_luk(bl)/3;
			break;
		case SC_CURSE:
			scdef=3+status_get_luk(bl);
			break;

		case SC_CONFUSION:
		default:
			scdef=0;
	}
	if(!(flag&8) && scdef>=100)	//flagª+8Èç®SÏ«vZµÈ¢
		return 0;

	if( bl->type != BL_PC && bl->type != BL_MOB && bl->type != BL_HOM ) {
		if(battle_config.error_log)
			printf("status_change_start: neither MOB nor PC !\n");
		return 0;
	}

	BL_CAST( BL_PC,  bl, sd );
	BL_CAST( BL_MOB, bl, md );
	BL_CAST( BL_HOM, bl, hd );

	if( sd ) {
		if( type == SC_ADRENALINE && !(skill_get_weapontype(BS_ADRENALINE)&(1<<sd->status.weapon)) )
			return 0;
		if( SC_STONE <= type && type <= SC_BLIND ) {	/* J[hÉæéÏ« */
			if( !(flag&8) && sd->reseff[type-SC_STONE] > 0 && atn_rand()%10000 < sd->reseff[type-SC_STONE] ) {
				if(battle_config.battle_log)
					printf("PC %d skill_sc_start: cardÉæéÙíÏ«­®\n",sd->bl.id);
				return 0;
			}
		}
	}

	//AfbhÍEÎ»Eo³ø
	if((race==1 || elem==9) && !(flag&1) && (type==SC_STONE || type==SC_FREEZE || type==SC_BLEED))
		return 0;

	if((type == SC_ADRENALINE || type==SC_ADRENALINE2 || type == SC_WEAPONPERFECTION || type == SC_OVERTHRUST) &&
		sc_data[type].timer != -1 && sc_data[type].val2 && !val2)
		return 0;

	if(mode&0x20 && !(flag&1) &&
	(type==SC_STONE || type==SC_FREEZE || type==SC_STAN || type==SC_SLEEP ||
	type==SC_POISON || type==SC_CURSE || type==SC_SILENCE || type==SC_CONFUSION ||
	type==SC_BLIND || type==SC_BLEED || type==SC_DPOISON || type==SC_PROVOKE ||
	type==SC_QUAGMIRE || type==SC_DECREASEAGI || type==SC_FOGWALLPENALTY ||
	(type==SC_BLESSING && (battle_check_undead(race,elem) || race==6))))
	/* {XÉÍø©È¢(½¾µJ[hÉæéøÊÍKp³êé) */
		return 0;

	//if(type==SC_FREEZE || type==SC_STAN || type==SC_SLEEP)
	if(type==SC_STAN || type==SC_SLEEP)
		unit_stop_walking(bl,1);

	if (type==SC_BLESSING && (bl->type==BL_PC || (!battle_check_undead(race,elem) && race!=6))) {
		if (sc_data[SC_CURSE].timer!=-1)
			status_change_end(bl,SC_CURSE,-1);
		if (sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2==0)
			status_change_end(bl,SC_STONE,-1);
	}

	if(sc_data[type].timer != -1){	/* ·ÅÉ¯¶ÙíÉÈÁÄ¢éê^C}ð */
		if(sc_data[type].val1 > val1 && type != SC_COMBO && type != SC_DANCING && type != SC_DEVOTION &&
			type != SC_SPEEDPOTION0 && type != SC_SPEEDPOTION1 && type != SC_SPEEDPOTION2 && type != SC_SPEEDPOTION3 &&
			type!= SC_DEVIL && type!=SC_DOUBLE  && type != SC_TKCOMBO && type!=SC_DODGE && type!=SC_SPURT)
			return 0;
		if ((type >=SC_STAN && type <= SC_BLIND) || type == SC_DPOISON || type == SC_FOGWALLPENALTY)
			return 0;/* p¬«µªÅ«È¢óÔÙíÅ éÍóÔÙíðsíÈ¢ */
		if(type == SC_GRAFFITI){	//ÙíÉà¤êxóÔÙíÉÈÁ½ÉðµÄ©çÄx©©é
			status_change_end(bl,type,-1);
		}else{
			(*sc_count)--;
			delete_timer(sc_data[type].timer, status_change_timer);
			sc_data[type].timer = -1;
		}
	}
	// NAO}CA/ðYêÈ¢ÅÍ³øÈXL
	if ((sc_data[SC_QUAGMIRE].timer!=-1 || sc_data[SC_DONTFORGETME].timer!=-1) &&
			(type==SC_CONCENTRATE || type==SC_INCREASEAGI ||
			type==SC_TWOHANDQUICKEN || type==SC_ONEHAND || type==SC_SPEARSQUICKEN ||
			type==SC_ADRENALINE || type==SC_ADRENALINE2 || type==SC_LOUD || type==SC_TRUESIGHT ||
			type==SC_WINDWALK || type==SC_CARTBOOST || type==SC_ASSNCROS))
		return 0;
	// ¬x¸­Í³øÈXL
	if (sc_data[SC_DECREASEAGI].timer!=-1 && (type==SC_TWOHANDQUICKEN || type==SC_ONEHAND ||
			type==SC_SPEARSQUICKEN || type==SC_ADRENALINE || type==SC_ADRENALINE2))
		return 0;

	switch(type){	/* ÙíÌíÞ²ÆÌ */
		case SC_ACTION_DELAY:
		case SC_ITEM_DELAY:
		case SC_DOUBLE:				/* _uXgCtBO */
		case SC_SUFFRAGIUM:			/* TtM */
		case SC_MAGNIFICAT:			/* }OjtBJ[g */
		case SC_AETERNA:			/* G[ei */
		case SC_BASILICA:			/* oWJ */
		case SC_TRICKDEAD:			/* ñ¾Óè */
		case SC_STRIPWEAPON:
		case SC_STRIPSHIELD:
		case SC_STRIPARMOR:
		case SC_STRIPHELM:
		case SC_CP_WEAPON:
		case SC_CP_SHIELD:
		case SC_CP_ARMOR:
		case SC_CP_HELM:
		case SC_COMBO:
		case SC_EXTREMITYFIST:			/* ¢Ce */
		case SC_RICHMANKIM:
		case SC_ROKISWEIL:			/* LÌ©Ñ */
		case SC_INTOABYSS:			/* [£ÌÉ */
		case SC_POEMBRAGI:			/* uMÌ */
		case SC_ANKLE:				/* AN */
			break;

		case SC_CONCENTRATE:			/* WÍüã */
		case SC_BLESSING:			/* ubVO */
		case SC_ANGELUS:			/* A[X */
		case SC_RESISTWATER:
		case SC_RESISTGROUND:
		case SC_RESISTFIRE:
		case SC_RESISTWIND:
		case SC_RESISTPOISON:
		case SC_RESISTHOLY:
		case SC_RESISTDARK:
		case SC_RESISTTELEKINESIS:
		case SC_RESISTUNDEAD:
		case SC_RESISTALL:
		case SC_TAROTCARD:
		case SC_IMPOSITIO:			/* C|VeBI}kX */
		case SC_DEVOTION:			/* fB{[V */
		case SC_GLORIA:				/* OA */
		case SC_LOUD:				/* Eh{CX */
		case SC_MINDBREAKER:			/* }Chu[J[ */
		case SC_ETERNALCHAOS:			/* G^[iJIX */
		case SC_WHISTLE:			/* ûJ */
		case SC_ASSNCROS:			/* [zÌATVNX */
		case SC_APPLEIDUN:			/* ChDÌÑç */
		case SC_SANTA:
		case SC_TRUESIGHT:			/* gD[TCg */
		case SC_SPIDERWEB:			/* XpC_[EFbu */
			calc_flag = 1;
			break;

		//Ú®H
		case SC_POEMBRAGI_:			/* uMÌ */
		case SC_FOGWALLPENALTY:
		case SC_FOGWALL:
		case SC_REVERSEORCISH:
		case SC_REDEMPTIO:
		case SC_GRAVITATION_USER:
			break;

		case SC_HUMMING:			/* n~O */
		case SC_FORTUNE:			/* K^ÌLX */
		case SC_SERVICE4U:			/* T[rXtH[[ */
		case SC_WHISTLE_:			/* ûJ */
		case SC_ASSNCROS_:			/* [zÌATVNX */
		case SC_APPLEIDUN_:			/* ChDÌÑç */
		case SC_HUMMING_:			/* n~O */
		case SC_DONTFORGETME_:			/* ðYêÈ¢Å */
		case SC_FORTUNE_:			/* K^ÌLX */
		case SC_SERVICE4U_:			/* T[rXtH[[ */
		case SC_GRAVITATION:			/* Ore[VtB[h */
			calc_flag = 1;
			break;

		case SC_PROVOKE:			/* v{bN */
			calc_flag = 1;
			if(tick <= 0) tick = 1000;	/* (I[go[T[N) */
			break;
		case SC_ENDURE:				/* CfA */
			if(tick <= 0) tick = 1000 * 60;
			calc_flag = 1;
			val2 = 7;	// 7ñU³ê½çð
			break;
		case SC_INCREASEAGI:		/* ¬xÁ */
			calc_flag = 1;
			if(sc_data[SC_DECREASEAGI].timer!=-1 )
				status_change_end(bl,SC_DECREASEAGI,-1);
			break;
		case SC_DECREASEAGI:		/* ¬x¸­ */
			calc_flag = 1;
			if(sc_data[SC_INCREASEAGI].timer!=-1 )
				status_change_end(bl,SC_INCREASEAGI,-1);
			if(sc_data[SC_TWOHANDQUICKEN].timer!=-1 )
				status_change_end(bl,SC_TWOHANDQUICKEN,-1);
			if(sc_data[SC_SPEARSQUICKEN].timer!=-1 )
				status_change_end(bl,SC_SPEARSQUICKEN,-1);
			if(sc_data[SC_ADRENALINE].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE,-1);
			break;
		case SC_SIGNUMCRUCIS:		/* VOiNVX */
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
		case SC_ADRENALINE:			/* AhibV */
			calc_flag = 1;
			if(sc_data[SC_ADRENALINE2].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE2,-1);
			if(sd && pc_checkskill(sd,BS_HILTBINDING)>0)
				tick += tick / 10;
			break;
		case SC_ADRENALINE2:			/* tAhibV */
			calc_flag = 1;
			if(sc_data[SC_ADRENALINE].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE,-1);
			if(sd && pc_checkskill(sd,BS_HILTBINDING)>0)
				tick += tick / 10;
			break;
		case SC_WEAPONPERFECTION:		/* EF|p[tFNV */
			if(sd && pc_checkskill(sd,BS_HILTBINDING)>0)
				tick += tick / 10;
			break;
		case SC_OVERTHRUST:			/* I[o[gXg */
			if(sc_data[SC_OVERTHRUSTMAX].timer != -1)
				return 0;
			*opt3 |= 2;
			if(sd && pc_checkskill(sd,BS_HILTBINDING)>0)
				tick += tick / 10;
			break;
		case SC_MAXIMIZEPOWER:		/* }LV}CYp[(SPª1¸éÔ,val2Éà) */
			if(bl->type == BL_PC)
				val2 = tick;
			else
				tick = 5000*val1;
			break;
		case SC_ENCPOISON:			/* G`g|CY */
			calc_flag = 1;
			val2=(((val1 - 1) / 2) + 3)*100;	/* Åt^m¦ */
			status_encchant_eremental_end(bl,SC_ENCPOISON);
			break;
		case SC_EDP:			/* G`gfbh[|CY */
			val2 = val1 + 2;			/* ÒÅt^m¦(%) */
			break;
		case SC_POISONREACT:	/* |CYANg */
			val2 = val1/2 + 1/(val1)?val1:1;
			break;
		case SC_ASPERSIO:			/* AXyVI */
			status_encchant_eremental_end(bl,SC_ASPERSIO);
			break;
		case SC_BENEDICTIO:			/* ¹Ì */
			status_enchant_armor_eremental_end(bl,SC_BENEDICTIO);
			break;
		case SC_ELEMENTWATER:		// 
			status_enchant_armor_eremental_end(bl,SC_ELEMENTWATER);
			if(sd){
				clif_displaymessage(sd->fd,"hïÉ®«ªt^³êÜµ½B");
			}
			break;
		case SC_ELEMENTGROUND:		// y
			status_enchant_armor_eremental_end(bl,SC_ELEMENTGROUND);
			if(sd){
				clif_displaymessage(sd->fd,"hïÉy®«ªt^³êÜµ½B");
			}
			break;
		case SC_ELEMENTFIRE:		// Î
			status_enchant_armor_eremental_end(bl,SC_ELEMENTFIRE);
			if(sd){
				clif_displaymessage(sd->fd,"hïÉÎ®«ªt^³êÜµ½B");
			}
			break;
		case SC_ELEMENTWIND:		// 
			status_enchant_armor_eremental_end(bl,SC_ELEMENTWIND);
			if(sd){
				clif_displaymessage(sd->fd,"hïÉ®«ªt^³êÜµ½B");
			}
			break;
		case SC_ELEMENTHOLY:		// õ
			status_enchant_armor_eremental_end(bl,SC_ELEMENTHOLY);
			if(sd){
				clif_displaymessage(sd->fd,"hïÉ¹®«ªt^³êÜµ½B");
			}
			break;
		case SC_ELEMENTDARK:		// Å
			status_enchant_armor_eremental_end(bl,SC_ELEMENTDARK);
			if(sd){
				clif_displaymessage(sd->fd,"hïÉÅ®«ªt^³êÜµ½B");
			}
			break;
		case SC_ELEMENTELEKINESIS:		// O
			status_enchant_armor_eremental_end(bl,SC_ELEMENTELEKINESIS);
			if(sd){
				clif_displaymessage(sd->fd,"hïÉO®«ªt^³êÜµ½B");
			}
			break;
		case SC_ELEMENTPOISON:		// Å
			status_enchant_armor_eremental_end(bl,SC_ELEMENTPOISON);
			if(sd){
				clif_displaymessage(sd->fd,"hïÉÅ®«ªt^³êÜµ½B");
			}
			break;
		case SC_ELEMENTUNDEAD:		// s
			status_enchant_armor_eremental_end(bl,SC_ELEMENTUNDEAD);
			if(sd){
				clif_displaymessage(sd->fd,"hïÉs®«ªt^³êÜµ½B");
			}
			break;
		case SC_RACEUNKNOWN:
		case SC_RACEUNDEAD:
		case SC_RACEBEAST:
		case SC_RACEPLANT:
		case SC_RACEINSECT:
		case SC_RACEFISH:
		case SC_RACEDEVIL:
		case SC_RACEHUMAN:
		case SC_RACEANGEL:
		case SC_RACEDRAGON:
			status_change_race_end(bl,type);
			if(sd){
				char message[64];
				sprintf(message,"í°ª%sÉÈèÜµ½",race_name[type-SC_RACEUNKNOWN]);
				clif_displaymessage(sd->fd,message);
			}
			break;
		case SC_ENERGYCOAT:			/* GiW[R[g */
			*opt3 |= 4;
			break;
		case SC_MAGICROD:
			val2 = val1*20;
			break;
		case SC_KYRIE:				/* LGGC\ */
			/* AXª|©ÁÄ¢½çðµÄ */
			if(sc_data[SC_ASSUMPTIO].timer!=-1)
				status_change_end(bl,SC_ASSUMPTIO,-1);
			/* LGð|¯é */
			val2 = (int)((double)status_get_max_hp(bl) * (val1 * 2 + 10) / 100);/* Ïvx */
			val3 = (val1 / 2 + 5);	/* ñ */
			break;
		case SC_QUAGMIRE:			/* N@O}CA */
			calc_flag = 1;
			if(sc_data[SC_CONCENTRATE].timer!=-1 )	/* WÍüãð */
				status_change_end(bl,SC_CONCENTRATE,-1);
			if(sc_data[SC_INCREASEAGI].timer!=-1 )	/* ¬xã¸ð */
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
			if(sc_data[SC_TRUESIGHT].timer!=-1 )	/* gD[TCg */
				status_change_end(bl,SC_TRUESIGHT,-1);
			if(sc_data[SC_WINDWALK].timer!=-1 )	/* EChEH[N */
				status_change_end(bl,SC_WINDWALK,-1);
			if(sc_data[SC_CARTBOOST].timer!=-1 )	/* J[gu[Xg */
				status_change_end(bl,SC_CARTBOOST,-1);
			if(sc_data[SC_ONEHAND].timer!=-1 )
				status_change_end(bl,SC_ONEHAND,-1);
			break;
		case SC_MAGICPOWER:			/* @Í */
			val2 = 1;				// êx¾¯
			break;
		case SC_SACRIFICE:			/* TNt@CX */
			val2 = 5;				// 5ñÌUÅLø
			break;
		case SC_FLAMELAUNCHER:		/* t[`[ */
		case SC_FROSTWEAPON:		/* tXgEF| */
		case SC_LIGHTNINGLOADER:	/* CgjO[_[ */
		case SC_SEISMICWEAPON:		/* TCY~bNEF| */
		case SC_DARKELEMENT:		/* Å®« */
		case SC_ATTENELEMENT:		/* O®« */
		case SC_UNDEADELEMENT:		/* s®« */
			status_encchant_eremental_end(bl,type);
			break;
		case SC_PROVIDENCE:			/* vBfX */
			calc_flag = 1;
			val2=val1*5;
			break;
		case SC_REFLECTSHIELD:
			val2=10+val1*3;
			break;
		case SC_AUTOSPELL:			/* I[gXy */
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
		case SC_SPEARSQUICKEN:		/* XsANCbP */
			calc_flag = 1;
			val2 = 20+val1;
			*opt3 |= 1;
			break;
		case SC_BLADESTOP_WAIT:		/* næè(Ò¿) */
			break;
		case SC_BLADESTOP:		/* næè */
			if(val2==2) clif_bladestop((struct block_list *)val3,(struct block_list *)val4,1);
			*opt3 |= 32;
			break;
		case SC_LULLABY:			/* qçS */
			val2 = 11;
			break;
		case SC_DRUMBATTLE:			/* í¾ÛÌ¿« */
			calc_flag = 1;
			val2 = (val1+1)*25;
			val3 = (val1+1)*2;
			break;
		case SC_NIBELUNGEN:			/* j[xOÌwÖ */
			calc_flag = 1;
			val2 = (val1+2)*25;
			break;
		case SC_SIEGFRIED:			/* sgÌW[Nt[h */
			calc_flag = 1;
			val2 = 5 + val1*15;
			break;
		case SC_DISSONANCE:			/* s¦a¹ */
			val2 = 10;
			break;
		case SC_UGLYDANCE:			/* ©ªèÈ_X */
			val2 = 10;
			break;
		case SC_DONTFORGETME:		/* ðYêÈ¢Å */
			calc_flag = 1;
			if(sc_data[SC_INCREASEAGI].timer!=-1 )	/* ¬xã¸ð */
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
			if(sc_data[SC_TRUESIGHT].timer!=-1 )	/* gD[TCg */
				status_change_end(bl,SC_TRUESIGHT,-1);
			if(sc_data[SC_WINDWALK].timer!=-1 )	/* EChEH[N */
				status_change_end(bl,SC_WINDWALK,-1);
			if(sc_data[SC_CARTBOOST].timer!=-1 )	/* J[gu[Xg */
				status_change_end(bl,SC_CARTBOOST,-1);
			if(sc_data[SC_ONEHAND].timer!=-1 )
				status_change_end(bl,SC_ONEHAND,-1);
			break;
		case SC_LONGINGFREEDOM:		//ðS©µÈ¢Å
			calc_flag = 1;
			val3 = 1;
			tick = 1000;
			break;
		case SC_DANCING:			/* _X/t */
			calc_flag = 1;
			val3= tick / 1000;
			tick = 1000;
			break;
		case SC_EXPLOSIONSPIRITS:	// ôg®
			calc_flag = 1;
			val2 = 75 + 25*val1;
			*opt3 |= 8;
			break;
		case SC_STEELBODY:			// à
			calc_flag = 1;
			*opt3 |= 16;
			break;
		case SC_AUTOCOUNTER:
			val3 = val4 = 0;
			break;
		case SC_POISONPOTION:		/* ÅòÌr */
			calc_flag = 1;
			tick = 1000 * tick;
			val2 = 25;
			break;
		case SC_SPEEDPOTION0:		/* ¬|[V */
		case SC_SPEEDPOTION1:
		case SC_SPEEDPOTION2:
			if(type!=SC_SPEEDPOTION0 && sc_data[SC_SPEEDPOTION0].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION0,-1);
			if(type!=SC_SPEEDPOTION1 && sc_data[SC_SPEEDPOTION1].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION1,-1);
			if(type!=SC_SPEEDPOTION2 && sc_data[SC_SPEEDPOTION2].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION2,-1);
			if(sc_data[SC_SPEEDPOTION3].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION3,-1);
			calc_flag = 1;
			tick = 1000 * tick;
			val2 = 5*(2+type-SC_SPEEDPOTION0);
			break;
		case SC_SPEEDPOTION3: //o[T[Nsb`[
			if(type!=SC_SPEEDPOTION0 && sc_data[SC_SPEEDPOTION0].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION0,-1);
			if(type!=SC_SPEEDPOTION1 && sc_data[SC_SPEEDPOTION1].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION1,-1);
			if(type!=SC_SPEEDPOTION2 && sc_data[SC_SPEEDPOTION2].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION2,-1);
			if(type!=SC_SPEEDPOTION3 && sc_data[SC_SPEEDPOTION3].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION3,-1);
			calc_flag = 1;
			tick = 1000 * tick;
			val2 = 20;
			break;

		case SC_INCATK:		//item 682p
		case SC_INCMATK:	//item 683p
			calc_flag = 1;
			tick = 1000 * tick;
			break;
		case SC_WEDDING:	//¥p(¥ßÖÉÈÁÄà­Ìªx¢Æ©)
			{
				time_t timer;

				calc_flag = 1;
				tick = 10000;
				if(!val2)
					val2 = (int)time(&timer);
				if( bl->type == BL_PC ){
					pc_setglobalreg(sd,"PC_WEDDING_TIME",val2);
				}
			}
			break;
		case SC_NOCHAT:	//`bgÖ~óÔ
			{
				time_t timer;
				tick = 60000;
				if(!val2)
					val2 = (int)time(&timer);
				updateflag = SP_MANNER;
			}
			break;
		case SC_SELFDESTRUCTION: //©
			tick = 100;
			break;

		/* option1 */
		case SC_STONE:				/* Î» */
			if(!(flag&2)) {
				int sc_def = status_get_mdef(bl)*200;
				tick = tick - sc_def;
			}
			val3 = tick/1000;
			if(val3 < 1) val3 = 1;
			tick = 5000;
			val2 = 1;
			break;
		case SC_SLEEP:				/* ° */
			if(!(flag&2)) {
//				int sc_def = 100 - (status_get_int(bl) + status_get_luk(bl)/3);
//				tick = tick * sc_def / 100;
//				if(tick < 1000) tick = 1000;
				tick = 30000;//°ÍXe[^XÏ«ÉÖíç¸30b
			}
			break;
		case SC_FREEZE:				/*  */
			if(!(flag&2)) {
				int sc_def = 100 - status_get_mdef(bl);
				tick = tick * sc_def / 100;
			}
			break;
		case SC_STAN:				/* X^ival2É~bZbgj */
			if(!(flag&2)) {
				int sc_def = 100 - (status_get_vit(bl) + status_get_luk(bl)/3);
				tick = tick * sc_def / 100;
			}
			break;
		/* option2 */
		case SC_DPOISON:			/* ÒÅ */
		{
			int mhp = status_get_max_hp(bl);
			int hp = status_get_hp(bl);
			// MHPÌ1/4ÈºÉÍÈçÈ¢
			if (hp > mhp>>2) {
				int diff = 0;
				if(sd)
					diff = mhp*10/100;
				else if(md)
					diff = mhp*15/100;
				if (hp - diff < mhp>>2)
					diff = hp - (mhp>>2);
				unit_heal(bl, -diff, 0);
			}
		}	// fall through
		case SC_POISON:				/* Å */
			calc_flag = 1;
			if(!(flag&2)) {
				int sc_def = 100 - (status_get_vit(bl) + status_get_luk(bl)/5);
				tick = tick * sc_def / 100;
			}
			val3 = tick/1000;
			if(val3 < 1) val3 = 1;
			tick = 1000;
			break;
		case SC_SILENCE:			/* ¾ÙibNXfr[ij */
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
		case SC_BLIND:				/* Ã */
			calc_flag = 1;
			if(!(flag&2)) {
				int sc_def = status_get_lv(bl)/10 + status_get_int(bl)/15;
				tick = 30000 - sc_def;
			}
			break;
		case SC_CURSE:				/* ô¢ */
			calc_flag = 1;
			if(!(flag&2)) {
				int sc_def = 100 - status_get_vit(bl);
				tick = tick * sc_def / 100;
			}
			break;
		case SC_CONFUSION:			/* ¬ */
			break;
		case SC_BLEED:				/* o */
			if(!(flag&2)) {
				// øÊÔÌÚ×s¾ÈÌÅÆè ¦¸ÅÌàÌðg¤
				int sc_def = 100 - (status_get_vit(bl) + status_get_luk(bl)/5);
				tick = tick * sc_def / 100;
			}
			val3 = (tick < 10000)? 1: tick/10000;
			tick = 10000;	// _[W­¶Í10sec
			break;

		/* option */
		case SC_HIDING:		/* nCfBO */
			calc_flag = 1;
			if(bl->type == BL_PC) {
				val2 = tick / 1000;		/* ±Ô */
				tick = 1000;
			}
			break;
		case SC_CHASEWALK:		/*`FCXEH[N*/
		case SC_CLOAKING:		/* N[LO */
		case SC_INVISIBLE:		/* CrWu */
			if(bl->type == BL_PC)
			{
				calc_flag = 1;
				val2 = tick;
				val3 = type==SC_CLOAKING ? 130-val1*3 : 135-val1*5;
			}
			else
				tick = 5000*val1;
			break;
		case SC_SIGHTBLASTER:	//TCguX^[
		case SC_SIGHT:			/* TCg/At */
		case SC_RUWACH:
		case SC_DETECTING:
			val2 = tick/250;
			tick = 10;
			break;

		/* Z[teBEH[Aj[} */
		case SC_SAFETYWALL:
		case SC_PNEUMA:
			tick=((struct skill_unit *)val2)->group->limit;
			break;

		/* XL¶áÈ¢/ÔÉÖWµÈ¢ */
		case SC_RIDING:
			calc_flag = 1;
			tick = 600*1000;
			break;
		case SC_FALCON:
		case SC_WEIGHT50:
		case SC_WEIGHT90:
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
		case SC_CONCENTRATION:	/* RZg[V */
			*opt3 |= 1;
			calc_flag = 1;
			break;
		case SC_TENSIONRELAX:	/* eVbNX */
			if(bl->type == BL_PC) {
				tick = 10000;
			} else
				return 0;
			break;
		case SC_AURABLADE:		/* I[u[h */
		case SC_PARRYING:		/* pCO */
		case SC_HEADCRUSH:		/* wbhNbV */
		case SC_MELTDOWN:		/* g_E */
			//Æè ¦¸è²«
			break;
		case SC_JOINTBEAT:		/* WCgr[g */
			calc_flag = 1;
			val4 = atn_rand()%6;
			if(val4 == 5) {
				// ñÍ­§IÉotÁ
				status_change_start(bl,SC_BLEED,val1,0,0,0,skill_get_time2(LK_JOINTBEAT,val1),10);
			}
			tick = tick - (status_get_agi(bl)/10 + status_get_luk(bl)/4)*1000;
			break;
		case SC_WINDWALK:		/* EChEH[N */
			calc_flag = 1;
			val2 = (val1 / 2); //Fleeã¸¦
			break;
		case SC_BERSERK:		/* o[T[N */
			if(sd){
				sd->status.sp = 0;
				clif_updatestatus(sd,SP_SP);
				clif_status_change(bl,SC_INCREASEAGI,1);	/* ACR\¦ */
			}
			*opt3 |= 128;
			tick = 1000;
			calc_flag = 1;
			break;
		case SC_ASSUMPTIO:		/* AXveBI */
			/* LGª|©ÁÄ¢½çðµÄ */
			if(sc_data[SC_KYRIE].timer!=-1)
				status_change_end(bl,SC_KYRIE,-1);
			/* JCgª|©ÁÄ¢½çðµÄ */
			if(sc_data[SC_KAITE].timer!=-1)
				status_change_end(bl,SC_KAITE,-1);

			/* AXÌtOð§Äé */
			*opt3 |= 2048;
			opt_flag = 1;
			break;
		case SC_MARIONETTE:		/* }IlbgRg[ */
		case SC_MARIONETTE2:		/* }IlbgRg[ */
			calc_flag = 1;
			*opt3 |= 1024;
			break;

		case SC_CARTBOOST:		/* J[gu[Xg */
			calc_flag = 1;
			if(sc_data[SC_DECREASEAGI].timer!=-1 )
				status_change_end(bl,SC_DECREASEAGI,-1);
			break;
		case SC_REJECTSWORD:	/* WFNg\[h */
			val2 = 3; //3ñUðµËÔ·
			break;
		case SC_MEMORIZE:		/* CY */
			val2 = 5; //5ñr¥ð1/2É·é
			break;
		case SC_GRAFFITI:		/* OtBeB */
			{
				struct skill_unit_group *sg = skill_unitsetting(bl,RG_GRAFFITI,val1,val2,val3,0);
				if(sg)
					val4 = (int)sg;
			}
			break;
		case SC_SPLASHER:		/* xiXvbV[ */
			break;
		case SC_GOSPEL:			/* SXy */
			break;
		case SC_INCHIT:			/* HITã¸ */
		case SC_INCFLEE:		/* FLEEã¸ */
		case SC_INCMHP2:		/* MHP%ã¸ */
		case SC_INCMSP2:		/* MSP%ã¸ */
		case SC_INCATK2:		/* ATK%ã¸ */
		case SC_INCHIT2:		/* HIT%ã¸ */
		case SC_INCFLEE2:		/* FLEE%ã¸ */
		case SC_INCALLSTATUS:	/* SXe[^X{20 */
			calc_flag = 1;
			break;
		case SC_PRESERVE:		/* vU[u */
			break;
		case SC_OVERTHRUSTMAX:		/* I[o[gXg}bNX */
			if(sc_data[SC_OVERTHRUST].timer!=-1)
				status_change_end(bl,SC_OVERTHRUST,-1);
			calc_flag = 1;
			break;

		case SC_CHASEWALK_STR:			/* STRã¸ */
		case SC_BATTLEORDER://ÕíÔ¨
			calc_flag = 1;
			break;
		case SC_REGENERATION://ã
		case SC_BATTLEORDER_DELAY:
		case SC_REGENERATION_DELAY:
		case SC_RESTORE_DELAY:
		case SC_EMERGENCYCALL_DELAY:
			break;
		case SC_THE_MAGICIAN:
		case SC_STRENGTH:
		case SC_THE_DEVIL:
		case SC_THE_SUN:
		case SC_SPURT://ì¯«pSTR
		case SC_SUN_COMFORT://#¾zÌÀy#
		case SC_MOON_COMFORT://#ÌÀy#
		case SC_STAR_COMFORT://#¯ÌÀy#
		case SC_FUSION://#¾zÆÆ¯ÌZ#
		case SC_RUN://ì¯«
		case SC_MEAL_INCSTR://Hp
		case SC_MEAL_INCAGI:
		case SC_MEAL_INCVIT:
		case SC_MEAL_INCINT:
		case SC_MEAL_INCDEX:
		case SC_MEAL_INCLUK:
		case SC_MEAL_INCHIT:
		case SC_MEAL_INCFLEE:
		case SC_MEAL_INCFLEE2:
		case SC_MEAL_INCCRITICAL:
		case SC_MEAL_INCDEF:
		case SC_MEAL_INCMDEF:
		case SC_MEAL_INCATK:
		case SC_MEAL_INCMATK:
			calc_flag = 1;
			break;
		case SC_MEAL_INCEXP:
		case SC_MEAL_INCJOB:
		case SC_HIGHJUMP:
		case SC_TKCOMBO://eRnpR{
		case SC_TRIPLEATTACK_RATE_UP:
		case SC_COUNTER_RATE_UP:
		case SC_SUN_WARM://#¾zÌ·àè#
		case SC_MOON_WARM://#Ì·àè#
		case SC_STAR_WARM://#¯Ì·àè#
		case SC_KAIZEL://#JC[#
		case SC_KAAHI://#JAq#
		case SC_SMA://#GX}#
		case SC_ELEMENTFIELD: //®«ê
		case SC_MIRACLE://¾zÆÆ¯ÌïÕ
		case SC_ANGEL://¾zÆÆ¯ÌVg
		case SC_BABY://ppA}}AåD«
			break;
		case SC_KAUPE://#JEv#
			val2 = val1*33;
			if(val1>=3)
				val2 = 100;
			break;
		case SC_KAITE://#JCg#
			/* AXª|©ÁÄ¢½çðµÄ */
			if(sc_data[SC_ASSUMPTIO].timer!=-1)
				status_change_end(bl,SC_ASSUMPTIO,-1);
			//½Ëñ
			if(val1 >= 5) val2 = 2;
			else val2 = 1;
			break;
		case SC_SWOO://#GXE#
		case SC_SKE://#GXN#
		case SC_SKA://#GXJ#
			if(bl->type!=BL_MOB)
				return 0;
			break;
		case SC_MONK://#NÌ°#
		case SC_STAR://#PZCÌ°#
		case SC_SAGE://#Z[WÌ°#
		case SC_CRUSADER://#NZC_[Ì°#
		case SC_WIZARD://#EBU[hÌ°#
		case SC_PRIEST://#v[XgÌ°#
		case SC_ROGUE://#[OÌ°#
		case SC_ASSASIN://#ATVÌ°#
		case SC_SOULLINKER://#\EJ[Ì°#
			if(sd && battle_config.disp_job_soul_state_change) {
				char output[64];
				strcpy(output,"°óÔÉÈèÜµ½");
				clif_disp_onlyself(sd,output,strlen(output));
			}
			soul_through = 1;
			break;
		case SC_KNIGHT://#iCgÌ°#
		case SC_ALCHEMIST://#AP~XgÌ°#
		case SC_BARDDANCER://#o[hÆ_T[Ì°#
		case SC_BLACKSMITH://#ubNX~XÌ°#
		case SC_HUNTER://#n^[Ì°#
		case SC_HIGH://#êãÊEÆÌ°#
			if(sd && battle_config.disp_job_soul_state_change) {
				char output[64];
				strcpy(output,"°óÔÉÈèÜµ½");
				clif_disp_onlyself(sd,output,strlen(output));
			}
			soul_through = 1;
			calc_flag = 1;
			break;
		case SC_SUPERNOVICE://#X[p[m[rXÌ°#
			if(sd) {
				//1%ÅStOÁ·H
				if(sd->status.base_level >=90 && atn_rand()%10000 < battle_config.repeal_die_counter_rate)
					sd->repeal_die_counter = 1;
				if(battle_config.disp_job_soul_state_change) {
					char output[64];
					strcpy(output,"°óÔÉÈèÜµ½");
					clif_disp_onlyself(sd,output,strlen(output));
				}
			}
			soul_through = 1;
			calc_flag = 1;
			break;
		case SC_AUTOBERSERK:
		case SC_READYSTORM:
		case SC_READYDOWN:
		case SC_READYTURN:
		case SC_READYCOUNTER:
		case SC_DODGE:
		case SC_DODGE_DELAY:
		case SC_DEVIL:
		case SC_DOUBLECASTING://_uLXeBO
		case SC_SHRINK://VN
		case SC_WINKCHARM://£fÌEBN
		case SC_TIGEREYE:
		case SC_PK_PENALTY:
		case SC_HERMODE:
			break;
		case SC_CLOSECONFINE://N[YRt@C
		case SC_HOLDWEB://z[hEFu
		case SC_DISARM:				//fBXA[
		case SC_GATLINGFEVER:		//KgOtB[o[
		case SC_FLING:				//tCO
		case SC_MADNESSCANCEL:		// }bhlXLZ[
		case SC_ADJUSTMENT:			// AWXgg
		case SC_INCREASING:			// CN[WOALAV[
		case SC_FULLBUSTER:			// toX^[
		case SC_NEN:	// NJ_NEN
		case SC_SUITON:	// NJ_SUITON
			calc_flag = 1;
			break;
		case SC_UTSUSEMI:// NJ_UTSUSEMI
			val3 = (int)(val1+1)/2;
			break;
		case SC_BUNSINJYUTSU:// NJ_BUNSINJYUTSU
			val3 = (int)(val1+1)/2;
			break;
		case SC_TATAMIGAESHI://ôÔµ
		case SC_NPC_DEFENDER:
			break;
		case SC_AVOID://#Ù}ñð#
		case SC_CHANGE://#^`FW#
		case SC_DEFENCE://#fBtFX#
		case SC_BLOODLUST://#ubhXg#
		case SC_FLEET://#t[g[u#
		case SC_SPEED://#I[o[hXs[h#
			calc_flag = 1;
			break;

		default:
			if(battle_config.error_log)
				printf("UnknownStatusChange [%d]\n", type);
			return 0;
	}

	if(bl->type==BL_PC && StatusIconChangeTable[type] != SI_BLANK)
		clif_status_change(bl,StatusIconChangeTable[type],1);	// ACR\¦

	/* optionÌÏX */
	switch(type){
		case SC_STONE:
		case SC_FREEZE:
		case SC_STAN:
		case SC_SLEEP:
			unit_stopattack(bl);	/* Uâ~ */
			skill_stop_dancing(bl,0);	/* t/_XÌf */
			{	/* ¯É|©çÈ¢Xe[^XÙíðð */
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
			*opt2 |= 1<<(type-SC_POISON);
			opt_flag = 1;
			break;
		case SC_FOGWALLPENALTY:
		case SC_BLIND:
			if(sc_data[SC_FOGWALLPENALTY].timer==-1){
				*opt2 |= 16;
				opt_flag = 1;
				if(md && !(flag&2)) md->target_id = 0;
			}
			break;
		case SC_DPOISON:	// bèÅÅÌGtFNgðgp
			*opt2 |= 1;
			opt_flag = 1;
			break;
		case SC_SIGNUMCRUCIS:
			*opt2 |= 0x40;
			opt_flag = 1;
			break;
		case SC_HIDING:
		case SC_CLOAKING:
			unit_stopattack(bl);	/* Uâ~ */
			*option |= ((type==SC_HIDING)?2:4);
			opt_flag =1 ;
			break;
		case SC_INVISIBLE:
			unit_stopattack(bl);	/* U?â~ */
			*option |= 64;
			opt_flag =1 ;
			break;
		case SC_CHASEWALK:
			unit_stopattack(bl);	/* U?â~ */
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
		case SC_REVERSEORCISH:
			*option |= 0x0800;
			opt_flag = 1;
			break;
		//opt3
		case SC_ONEHAND:			/* 1HQ */
		case SC_TWOHANDQUICKEN:		/* 2HQ */
		case SC_SPEARSQUICKEN:		/* XsANCbP */
		case SC_CONCENTRATION:		/* RZg[V */
			*opt3 |= 1;
			opt_flag = 1;
			break;
		case SC_OVERTHRUST:			/* I[o[gXg */
			*opt3 |= 2;
			opt_flag = 1;
			break;
		case SC_ENERGYCOAT:			/* GiW[R[g */
			*opt3 |= 4;
			opt_flag = 1;
			break;
		case SC_EXPLOSIONSPIRITS:	// ôg®
			*opt3 = 8;
			opt_flag = 1;
			break;
		case SC_STEELBODY:			// à
			*opt3 |= 16;
			opt_flag = 1;
			break;
		case SC_BLADESTOP:		/* næè */
			*opt3 |= 32;
			opt_flag = 1;
			break;
		case SC_BERSERK:		/* o[T[N */
			*opt3 |= 128;
			opt_flag = 1;
			break;
		case SC_MARIONETTE:		/* }IlbgRg[ */
		case SC_MARIONETTE2:	/* }IlbgRg[ */
			*opt3 |= 1024;
			opt_flag = 1;
			break;
		case SC_ASSUMPTIO:		/* AXveBI */
			*opt3 |= 2048;
			opt_flag = 1;
			clif_misceffect2(bl,375);
			break;
		case SC_SUN_WARM://#¾zÌ·àè#
		case SC_MOON_WARM://#Ì·àè#
		case SC_STAR_WARM://#¯Ì·àè#
			*opt3 |= 4096;
			opt_flag = 1;
			break;
		case SC_KAITE:
			*opt3 |= 8192;
			opt_flag = 1;
			break;
	}
	if(soul_through){
		*opt3 |= 32768;
		clif_misceffect2(bl,424);
		opt_flag = 1;
	}

	if(opt_flag) {	/* optionÌÏX */
		clif_changeoption(bl);
		if(sd)
			clif_changelook(&sd->bl,LOOK_CLOTHES_COLOR,sd->status.clothes_color);
	}
	(*sc_count)++;	/* Xe[^XÙíÌ */

	sc_data[type].val1 = val1;
	sc_data[type].val2 = val2;
	sc_data[type].val3 = val3;
	sc_data[type].val4 = val4;
	/* ^C}[Ýè */
	sc_data[type].timer = add_timer(gettick() + tick, status_change_timer, bl->id, type);

	if(bl->type==BL_PC && calc_flag && !(flag&4))
		status_calc_pc(sd,0);	/* Xe[^XÄvZ */

	if(bl->type==BL_PC && updateflag)
		clif_updatestatus(sd,updateflag);	/* Xe[^XðNCAgÉé */

	if(bl->type==BL_HOM && calc_flag)
	{
		homun_calc_status((struct homun_data*)bl);
		clif_send_homstatus(((struct homun_data*)bl)->msd,0);
	}	
	//vZãÉç¹é
	switch(type){
		case SC_RUN://ì¯«
			if(sd) {
				pc_runtodir(sd);
			}
			break;
		case SC_HIGHJUMP:
			if(sd) {
				pc_highjumptodir(sd,val4);
			}
			break;
	}

	return 0;
}
/*==========================================
 * Xe[^XÙíSð
 *------------------------------------------
 */
int status_change_clear(struct block_list *bl,int type)
{
	struct status_change* sc_data;
	short *sc_count, *option, *opt1, *opt2, *opt3;
	int i;

	nullpo_retr(0, bl);
#ifdef DYNAMIC_SC_DATA
	if(status_check_dummy_sc_data(bl))
		return 0;
#endif
	nullpo_retr(0, sc_data=status_get_sc_data(bl));
	nullpo_retr(0, sc_count=status_get_sc_count(bl));
	nullpo_retr(0, option=status_get_option(bl));
	nullpo_retr(0, opt1=status_get_opt1(bl));
	nullpo_retr(0, opt2=status_get_opt2(bl));
	nullpo_retr(0, opt3=status_get_opt3(bl));

	if(*sc_count == 0)
		return 0;
	status_calc_pc_stop_begin(bl);
	for(i = 0; i < MAX_STATUSCHANGE; i++){
		if(i==SC_BABY || i==SC_REDEMPTIO)
		{
			if(unit_isdead(bl))
				continue;
		}
		if(sc_data[i].timer != -1){	/* Ùíª éÈç^C}[ðí·é */
/*
			delete_timer(sc_data[i].timer, status_change_timer);
			sc_data[i].timer = -1;

			if(!type && i<SC_SENDMAX)
				clif_status_change(bl,i,0);
*/

			status_change_end(bl,i,-1);
		}
	}
	status_calc_pc_stop_end(bl);
	*sc_count = 0;
	*opt1 = 0;
	*opt2 = 0;
	*opt3 = 0;
	*option &= OPTION_MASK;

	if(!type || type&2) {
		clif_changeoption(bl);
		if(bl->type==BL_PC)
			clif_changelook(bl,LOOK_CLOTHES_COLOR,((struct map_session_data *)bl)->status.clothes_color);
	}

	return 0;
}

/*==========================================
 * Xe[^XÙíI¹
 *------------------------------------------
 */
int status_change_end_by_jumpkick( struct block_list* bl)
{
	struct status_change* sc_data = NULL;

	nullpo_retr(0, bl);
	//X^[ÍgíÈ¢¾ë¤©ç³
	if(bl->type!=BL_PC)
		return 0;
	if(bl->type==BL_MOB && status_get_sc_data(bl)==NULL)
		return 0;
	if(bl->type!=BL_PC && bl->type!=BL_MOB) {
		if(battle_config.error_log)
			printf("status_change_end: neither MOB nor PC !\n");
		return 0;
	}

	//\EJ[Í³
	if(bl->type==BL_PC)
	{
		if(((struct map_session_data*)bl)->status.class == PC_CLASS_SL)
			return 0;
	}

	nullpo_retr(0, sc_data=status_get_sc_data(bl));

	if(sc_data)
	{
		//vU[uÅÍØêÈ¢
		if(sc_data[SC_PRESERVE].timer!=-1)
			return 0;

		status_calc_pc_stop_begin(bl);
		//o[T[Nsb`[
		if(sc_data[SC_SPEEDPOTION3].timer!=-1)
			status_change_end(bl,SC_SPEEDPOTION3,-1);
		if(sc_data[SC_SUN_WARM].timer!=-1)
			status_change_end(bl,SC_SUN_WARM,-1);
		if(sc_data[SC_MOON_WARM].timer!=-1)
			status_change_end(bl,SC_MOON_WARM,-1);
		if(sc_data[SC_STAR_WARM].timer!=-1)
			status_change_end(bl,SC_STAR_WARM,-1);
		if(sc_data[SC_SUN_COMFORT].timer!=-1)
			status_change_end(bl,SC_SUN_COMFORT,-1);
		if(sc_data[SC_MOON_COMFORT].timer!=-1)
			status_change_end(bl,SC_MOON_COMFORT,-1);
		if(sc_data[SC_STAR_COMFORT].timer!=-1)
			status_change_end(bl,SC_STAR_COMFORT,-1);
		if(sc_data[SC_FUSION].timer!=-1)
			status_change_end(bl,SC_FUSION,-1);
		//°ð
		if(sc_data[SC_ALCHEMIST].timer!=-1)
			status_change_end(bl,SC_ALCHEMIST,-1);
		if(sc_data[SC_MONK].timer!=-1)
			status_change_end(bl,SC_MONK,-1);
		if(sc_data[SC_STAR].timer!=-1)
			status_change_end(bl,SC_STAR,-1);
		if(sc_data[SC_SAGE].timer!=-1)
			status_change_end(bl,SC_SAGE,-1);
		if(sc_data[SC_CRUSADER].timer!=-1)
			status_change_end(bl,SC_CRUSADER,-1);
		if(sc_data[SC_SUPERNOVICE].timer!=-1)
			status_change_end(bl,SC_SUPERNOVICE,-1);
		if(sc_data[SC_KNIGHT].timer!=-1)
			status_change_end(bl,SC_KNIGHT,-1);
		if(sc_data[SC_WIZARD].timer!=-1)
			status_change_end(bl,SC_WIZARD,-1);
		if(sc_data[SC_PRIEST].timer!=-1)
			status_change_end(bl,SC_PRIEST,-1);
		if(sc_data[SC_BARDDANCER].timer!=-1)
			status_change_end(bl,SC_BARDDANCER,-1);
		if(sc_data[SC_ROGUE].timer!=-1)
			status_change_end(bl,SC_ROGUE,-1);
		if(sc_data[SC_ASSASIN].timer!=-1)
			status_change_end(bl,SC_ASSASIN,-1);
		if(sc_data[SC_BLACKSMITH].timer!=-1)
			status_change_end(bl,SC_BLACKSMITH,-1);
		if(sc_data[SC_HUNTER].timer!=-1)
			status_change_end(bl,SC_HUNTER,-1);
		if(sc_data[SC_HIGH].timer!=-1)
			status_change_end(bl,SC_HIGH,-1);
		//
		if(sc_data[SC_ADRENALINE2].timer!=-1)
			status_change_end(bl,SC_ADRENALINE2,-1);
		if(sc_data[SC_KAIZEL].timer!=-1)
			status_change_end(bl,SC_KAIZEL,-1);
		if(sc_data[SC_KAAHI].timer!=-1)
			status_change_end(bl,SC_KAAHI,-1);
		if(sc_data[SC_KAUPE].timer!=-1)
			status_change_end(bl,SC_KAUPE,-1);
		if(sc_data[SC_KAITE].timer!=-1)
			status_change_end(bl,SC_KAITE,-1);
		if(sc_data[SC_ONEHAND].timer!=-1)
			status_change_end(bl,SC_ONEHAND,-1);

		status_calc_pc_stop_end(bl);
	}

	return 0;
}
/*==========================================
 * x@I¹
 *------------------------------------------
 */
int status_support_magic_skill_end( struct block_list* bl)
{
	struct status_change* sc_data = NULL;

	nullpo_retr(0, bl);

	nullpo_retr(0, sc_data=status_get_sc_data(bl));

	if(sc_data)
	{
		status_calc_pc_stop_begin(bl);
		if(sc_data[SC_BLESSING].timer!=-1)
			status_change_end(bl,SC_BLESSING,-1);
		if(sc_data[SC_ASSUMPTIO].timer!=-1)
			status_change_end(bl,SC_ASSUMPTIO,-1);
		if(sc_data[SC_KYRIE].timer!=-1)
			status_change_end(bl,SC_KYRIE,-1);
		if(sc_data[SC_INCREASEAGI].timer!=-1 )
			status_change_end(bl,SC_INCREASEAGI,-1);
		if(sc_data[SC_MAGNIFICAT].timer!=-1 )
			status_change_end(bl,SC_MAGNIFICAT,-1);
		if(sc_data[SC_GLORIA].timer!=-1 )
			status_change_end(bl,SC_GLORIA,-1);
		if(sc_data[SC_ANGELUS].timer!=-1 )
			status_change_end(bl,SC_ANGELUS,-1);
		if(sc_data[SC_SLOWPOISON].timer!=-1)
			status_change_end(bl,SC_SLOWPOISON,-1);
		if(sc_data[SC_IMPOSITIO].timer!=-1)
			status_change_end(bl,SC_IMPOSITIO,-1);
		if(sc_data[SC_SUFFRAGIUM].timer!=-1)
			status_change_end(bl,SC_SUFFRAGIUM,-1);
		status_encchant_eremental_end(bl,-1);//í
		status_enchant_armor_eremental_end(bl,-1);//Z
		/*
		if(sc_data[SC_ENDURE].timer!=-1)
			status_change_end(bl,SC_ENDURE,-1);
		if(sc_data[SC_CONCENTRATE].timer!=-1)
			status_change_end(bl,SC_CONCENTRATE,-1);
		if(sc_data[SC_OVERTHRUST].timer!=-1)
			status_change_end(bl,SC_OVERTHRUST,-1);
		if(sc_data[SC_WEAPONPERFECTION].timer!=-1)
			status_change_end(bl,SC_WEAPONPERFECTION,-1);
		if(sc_data[SC_MAXIMIZEPOWER].timer!=-1)
			status_change_end(bl,SC_MAXIMIZEPOWER,-1);
		if(sc_data[SC_LOUD].timer!=-1)
			status_change_end(bl,SC_LOUD,-1);
		if(sc_data[SC_ENERGYCOAT].timer!=-1)
			status_change_end(bl,SC_ENERGYCOAT,-1);
		if(sc_data[SC_PARRYING].timer!=-1)
			status_change_end(bl,SC_PARRYING,-1);
		if(sc_data[SC_CONCENTRATION].timer!=-1 )
			status_change_end(bl,SC_CONCENTRATION,-1);
		if(sc_data[SC_TWOHANDQUICKEN].timer!=-1 )
			status_change_end(bl,SC_TWOHANDQUICKEN,-1);
		if(sc_data[SC_SPEARSQUICKEN].timer!=-1 )
			status_change_end(bl,SC_SPEARSQUICKEN,-1);
		if(sc_data[SC_ADRENALINE].timer!=-1 )
			status_change_end(bl,SC_ADRENALINE,-1);

		if(sc_data[SC_TRUESIGHT].timer!=-1 )	// gD[TCg
			status_change_end(bl,SC_TRUESIGHT,-1);
		if(sc_data[SC_WINDWALK].timer!=-1 )	// EChEH[N
			status_change_end(bl,SC_WINDWALK,-1);
		if(sc_data[SC_CARTBOOST].timer!=-1 )	// J[gu[Xg
			status_change_end(bl,SC_CARTBOOST,-1);

		//o[T[Nsb`[
		if(sc_data[SC_SPEEDPOTION3].timer!=-1)
			status_change_end(bl,SC_SPEEDPOTION3,-1);

		if(sc_data[SC_SUN_WARM].timer!=-1)
			status_change_end(bl,SC_SUN_WARM,-1);
		if(sc_data[SC_MOON_WARM].timer!=-1)
			status_change_end(bl,SC_MOON_WARM,-1);
		if(sc_data[SC_STAR_WARM].timer!=-1)
			status_change_end(bl,SC_STAR_WARM,-1);
		if(sc_data[SC_SUN_COMFORT].timer!=-1)
			status_change_end(bl,SC_SUN_COMFORT,-1);
		if(sc_data[SC_MOON_COMFORT].timer!=-1)
			status_change_end(bl,SC_MOON_COMFORT,-1);
		if(sc_data[SC_STAR_COMFORT].timer!=-1)
			status_change_end(bl,SC_STAR_COMFORT,-1);
		if(sc_data[SC_FUSION].timer!=-1)
			status_change_end(bl,SC_FUSION,-1);
		//°ð
		if(sc_data[SC_ALCHEMIST].timer!=-1)
			status_change_end(bl,SC_ALCHEMIST,-1);
		if(sc_data[SC_MONK].timer!=-1)
			status_change_end(bl,SC_MONK,-1);
		if(sc_data[SC_STAR].timer!=-1)
			status_change_end(bl,SC_STAR,-1);
		if(sc_data[SC_SAGE].timer!=-1)
			status_change_end(bl,SC_SAGE,-1);
		if(sc_data[SC_CRUSADER].timer!=-1)
			status_change_end(bl,SC_CRUSADER,-1);
		if(sc_data[SC_SUPERNOVICE].timer!=-1)
			status_change_end(bl,SC_SUPERNOVICE,-1);
		if(sc_data[SC_KNIGHT].timer!=-1)
			status_change_end(bl,SC_KNIGHT,-1);
		if(sc_data[SC_WIZARD].timer!=-1)
			status_change_end(bl,SC_WIZARD,-1);
		if(sc_data[SC_PRIEST].timer!=-1)
			status_change_end(bl,SC_PRIEST,-1);
		if(sc_data[SC_BARDDANCER].timer!=-1)
			status_change_end(bl,SC_BARDDANCER,-1);
		if(sc_data[SC_ROGUE].timer!=-1)
			status_change_end(bl,SC_ROGUE,-1);
		if(sc_data[SC_ASSASIN].timer!=-1)
			status_change_end(bl,SC_ASSASIN,-1);
		if(sc_data[SC_BLACKSMITH].timer!=-1)
			status_change_end(bl,SC_BLACKSMITH,-1);
		if(sc_data[SC_HUNTER].timer!=-1)
			status_change_end(bl,SC_HUNTER,-1);
		if(sc_data[SC_HIGH].timer!=-1)
			status_change_end(bl,SC_HIGH,-1);
		//
		if(sc_data[SC_ADRENALINE2].timer!=-1)
			status_change_end(bl,SC_ADRENALINE2,-1);
		if(sc_data[SC_KAIZEL].timer!=-1)
			status_change_end(bl,SC_KAIZEL,-1);
		if(sc_data[SC_KAAHI].timer!=-1)
			status_change_end(bl,SC_KAAHI,-1);
		if(sc_data[SC_KAUPE].timer!=-1)
			status_change_end(bl,SC_KAUPE,-1);
		if(sc_data[SC_KAITE].timer!=-1)
			status_change_end(bl,SC_KAITE,-1);
		if(sc_data[SC_ONEHAND].timer!=-1)
			status_change_end(bl,SC_ONEHAND,-1);
		*/
		status_calc_pc_stop_end(bl);
	}

	return 0;
}
/*==========================================
 * Xe[^XÙíI¹
 *------------------------------------------
 */
int status_change_end( struct block_list* bl , int type,int tid)
{
	struct map_session_data *sd = NULL;
	struct status_change* sc_data;
	int opt_flag=0, calc_flag = 0,soul_through=0;
	short *sc_count, *option, *opt1, *opt2, *opt3;

	nullpo_retr(0, bl);
	if(bl->type!=BL_PC && bl->type!=BL_MOB && bl->type!=BL_HOM) {
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

	if(type >= MAX_STATUSCHANGE)
	{
		switch(type)
		{
			case SC_RACECLEAR:
				status_change_race_end(bl,-1);
				break;
			case SC_RESISTCLEAR:
				status_change_resistclear(bl);
				break;
			case SC_SOUL:
			case SC_SOULCLEAR:
				status_change_soulclear(bl);
				break;
			default:
				if(battle_config.error_log)
					printf("UnknownStatusChangeEnd [%d]\n", type);
				break;
		}
		return 0;
	}

	if(bl->type == BL_PC)
		sd = (struct map_session_data *)bl;

	if((*sc_count)>0 && sc_data[type].timer!=-1 &&
		(sc_data[type].timer==tid || tid==-1) ){

		if(tid==-1)	/* ^C}©çÄÎêÄ¢È¢Èç^C}íð·é */
			delete_timer(sc_data[type].timer,status_change_timer);

		/* YÌÙíð³íÉß· */
		sc_data[type].timer=-1;
		(*sc_count)--;

		switch(type){	/* ÙíÌíÞ²ÆÌ */
			case SC_PROVOKE:			/* v{bN */
			case SC_ENDURE:				/* CfA */
			case SC_CONCENTRATE:		/* WÍüã */
			case SC_BLESSING:			/* ubVO */
			case SC_ANGELUS:			/* A[X */
			case SC_INCREASEAGI:		/* ¬xã¸ */
			case SC_DECREASEAGI:		/* ¬x¸­ */
			case SC_SIGNUMCRUCIS:		/* VOiNVX */
			case SC_HIDING:
			case SC_CLOAKING:
			case SC_TWOHANDQUICKEN:		/* 2HQ */
			case SC_ONEHAND:			/* 1HQ */
			case SC_ADRENALINE:			/* AhibV */
			case SC_ENCPOISON:			/* G`g|CY */
			case SC_IMPOSITIO:			/* C|VeBI}kX */
			case SC_GLORIA:				/* OA */
			case SC_LOUD:				/* Eh{CX */
			case SC_MINDBREAKER:		/* }Chu[J[ */
			case SC_QUAGMIRE:			/* N@O}CA */
			case SC_PROVIDENCE:			/* vBfX */
			case SC_SPEARSQUICKEN:		/* XsANCbP */
			case SC_VOLCANO:
			case SC_DELUGE:
			case SC_VIOLENTGALE:
			case SC_ETERNALCHAOS:		/* G^[iJIX */
			case SC_DRUMBATTLE:			/* í¾ÛÌ¿« */
			case SC_NIBELUNGEN:			/* j[xOÌwÖ */
			case SC_SIEGFRIED:			/* sgÌW[Nt[h */
			case SC_EXPLOSIONSPIRITS:	// ôg®
			case SC_STEELBODY:			// à
			case SC_DEFENDER:
			case SC_POISONPOTION:		/* ÅòÌr */
			case SC_SPEEDPOTION0:		/* ¬|[V */
			case SC_SPEEDPOTION1:
			case SC_SPEEDPOTION2:
			case SC_SPEEDPOTION3:
			case SC_RIDING:
			case SC_BLADESTOP_WAIT:
			case SC_CONCENTRATION:		/* RZg[V */
			case SC_WINDWALK:		/* EChEH[N */
			case SC_TRUESIGHT:		/* gD[TCg */
			case SC_SPIDERWEB:		/* XpC_[EFbu */
			case SC_INCATK:		//item 682p
			case SC_INCMATK:	//item 683p
			case SC_WEDDING:	//¥p(¥ßÖÉÈÁÄà­Ìªx¢Æ©)
			case SC_SANTA:
			case SC_INCALLSTATUS:
			case SC_INCHIT:
			case SC_INCFLEE:
			case SC_INCMHP2:
			case SC_INCMSP2:
			case SC_INCATK2:
			case SC_INCHIT2:
			case SC_INCFLEE2:
			//case SC_PRESERVE:
			case SC_OVERTHRUSTMAX:
			case SC_CHASEWALK:	/*`FCXEH[N*/
			case SC_CHASEWALK_STR:
			case SC_BATTLEORDER:
			//case SC_THE_MAGICIAN:
			//case SC_STRENGTH:
			//case SC_THE_DEVIL:
			//case SC_THE_SUN:
			case SC_MEAL_INCSTR://Hp
			case SC_MEAL_INCAGI:
			case SC_MEAL_INCVIT:
			case SC_MEAL_INCINT:
			case SC_MEAL_INCDEX:
			case SC_MEAL_INCLUK:
			case SC_MEAL_INCHIT:
			case SC_MEAL_INCFLEE:
			case SC_MEAL_INCFLEE2:
			case SC_MEAL_INCCRITICAL:
			case SC_MEAL_INCDEF:
			case SC_MEAL_INCMDEF:
			case SC_MEAL_INCATK:
			case SC_MEAL_INCMATK:
			case SC_SPURT:
			case SC_SUN_COMFORT://#¾zÌÀy#
			case SC_MOON_COMFORT://#ÌÀy#
			case SC_STAR_COMFORT://#¯ÌÀy#
			case SC_FUSION://#¾zÆÆ¯ÌZ#
			case SC_ADRENALINE2://#tAhibV#
			case SC_RESISTWATER:
			case SC_RESISTGROUND:
			case SC_RESISTFIRE:
			case SC_RESISTWIND:
			case SC_RESISTPOISON:
			case SC_RESISTHOLY:
			case SC_RESISTDARK:
			case SC_RESISTTELEKINESIS:
			case SC_RESISTUNDEAD:
			case SC_RESISTALL:
			case SC_INVISIBLE:
			case SC_TAROTCARD:
			case SC_DISARM:				/* fBXA[ */
			case SC_GATLINGFEVER:		/* KgOtB[o[ */
			case SC_FLING:				/* tCO */
			case SC_MADNESSCANCEL:		/* }bhlXLZ[ */
			case SC_ADJUSTMENT:			/* AWXgg */
			case SC_INCREASING:			/* CN[WOALAV[ */
			case SC_FULLBUSTER:			/* toX^[ */
				calc_flag = 1;
				break;
			case SC_ELEMENTWATER:		// 
			case SC_ELEMENTGROUND:		// y
			case SC_ELEMENTFIRE:		// Î
			case SC_ELEMENTWIND:		// 
			case SC_ELEMENTHOLY:		// õ
			case SC_ELEMENTDARK:		// Å
			case SC_ELEMENTELEKINESIS:	// O
			case SC_ELEMENTPOISON:		// Å
			case SC_ELEMENTUNDEAD:		// s
				if(sd){
					clif_displaymessage(sd->fd,"hïÌ®«ª³ÉßèÜµ½");
				}
				break;
			case SC_RACEUNKNOWN:
			case SC_RACEUNDEAD:
			case SC_RACEBEAST:
			case SC_RACEPLANT:
			case SC_RACEINSECT:
			case SC_RACEFISH:
			case SC_RACEDEVIL:
			case SC_RACEHUMAN:
			case SC_RACEANGEL:
			case SC_RACEDRAGON:
				if(sd)
					clif_displaymessage(sd->fd,"í°ª³ÉßèÜµ½");
				break;
			case SC_RUN://ì¯«
				unit_stop_walking(bl,0);
				calc_flag = 1;
				break;
			case SC_MONK://#NÌ°#
			case SC_STAR://#PZCÌ°#
			case SC_SAGE://#Z[WÌ°#
			case SC_CRUSADER://#NZC_[Ì°#
			case SC_WIZARD://#EBU[hÌ°#
			case SC_PRIEST://#v[XgÌ°#
			case SC_ROGUE://#[OÌ°#
			case SC_ASSASIN://#ATVÌ°#
			case SC_SOULLINKER://#\EJ[Ì°#
				if(sd && battle_config.disp_job_soul_state_change) {
					char output[64];
					strcpy(output,"°óÔªI¹µÜµ½");
					clif_disp_onlyself(sd,output,strlen(output));
				}
				soul_through = 1;
				break;
			case SC_KNIGHT://#iCgÌ°#
			case SC_ALCHEMIST://#AP~XgÌ°#
			case SC_BARDDANCER://#o[hÆ_T[Ì°#
			case SC_BLACKSMITH://#ubNX~XÌ°#
			case SC_HUNTER://#n^[Ì°#
			case SC_HIGH://#êãÊEÆÌ°#
				if(sd && battle_config.disp_job_soul_state_change) {
					char output[64];
					strcpy(output,"°óÔªI¹µÜµ½");
					clif_disp_onlyself(sd,output,strlen(output));
				}
				soul_through = 1;
				calc_flag = 1;
				break;
			case SC_SUPERNOVICE://#X[p[m[rXÌ°#
				if(sd) {
					if(battle_config.disp_job_soul_state_change) {
						char output[64];
						strcpy(output,"°óÔªI¹µÜµ½");
						clif_disp_onlyself(sd,output,strlen(output));
					}
					if(sd->repeal_die_counter)
						calc_flag = 1;
					sd->repeal_die_counter = 0;
				}
				soul_through = 1;
				break;
			case SC_POEMBRAGI:			/* uM */
			case SC_WHISTLE:			/* ûJ */
			case SC_ASSNCROS:			/* [zÌATVNX */
			case SC_APPLEIDUN:			/* ChDÌÑç */
			case SC_HUMMING:			/* n~O */
			case SC_DONTFORGETME:		/* ðYêÈ¢Å */
			case SC_FORTUNE:			/* K^ÌLX */
			case SC_SERVICE4U:			/* T[rXtH[[ */
				calc_flag = 1;
				//xèt±Zbg
				if(sc_data && sc_data[type + SC_WHISTLE_ - SC_WHISTLE].timer==-1)
					status_change_start(bl,type + SC_WHISTLE_ - SC_WHISTLE,sc_data[type].val1,
						sc_data[type].val2,sc_data[type].val3,sc_data[type].val4,battle_config.dance_and_play_duration,0);
				break;
			case SC_WHISTLE_:			/* ûJ */
			case SC_ASSNCROS_:			/* [zÌATVNX */
			case SC_APPLEIDUN_:			/* ChDÌÑç */
			case SC_HUMMING_:			/* n~O */
			case SC_DONTFORGETME_:		/* ðYêÈ¢Å */
			case SC_FORTUNE_:			/* K^ÌLX */
			case SC_SERVICE4U_:			/* T[rXtH[[ */
			case SC_GRAVITATION:
				calc_flag = 1;
				break;
			case SC_MARIONETTE:			/* }IlbgRg[ */
			case SC_MARIONETTE2:			/* }IlbgRg[ */
				{
					struct status_change *t_sc_data = status_get_sc_data( map_id2bl(sc_data[type].val2) );
					int tmp = (type==SC_MARIONETTE)? SC_MARIONETTE2: SC_MARIONETTE;
					//ûª}IlbgóÔÈç¢ÁµåÉð
					if(t_sc_data && t_sc_data[tmp].timer!=-1)
						status_change_end( map_id2bl(sc_data[type].val2),tmp,-1 );
					if(sd)
						clif_marionette(sd,0);
				}
				calc_flag = 1;
				break;
			case SC_BERSERK:			/* o[T[N */
				calc_flag = 1;
				clif_status_change(bl,SC_INCREASEAGI,0);	/* ACRÁ */
				break;
			case SC_DEVOTION:		/* fB{[V */
				{
					struct map_session_data *dsd = map_id2sd(sc_data[type].val1);
					sc_data[type].val1=sc_data[type].val2=0;
					if(dsd)
						skill_devotion(dsd,bl->id);
					calc_flag = 1;
				}
				break;
			case SC_BLADESTOP:
				{
					struct status_change *t_sc_data = status_get_sc_data((struct block_list *)sc_data[type].val4);
					//ÐûªØê½ÌÅèÌnóÔªØêÄÈ¢ÌÈçð
					if(t_sc_data && t_sc_data[SC_BLADESTOP].timer!=-1)
						status_change_end((struct block_list *)sc_data[type].val4,SC_BLADESTOP,-1);

					if(sc_data[type].val2==2)
						clif_bladestop((struct block_list *)sc_data[type].val3,(struct block_list *)sc_data[type].val4,0);
				}
				break;
			case SC_CLOSECONFINE://H¬p
				{
					struct status_change *t_sc_data = status_get_sc_data((struct block_list *)sc_data[type].val4);
					//ÐûªØê½ÌÅèÌCLOSECONFINEóÔªØêÄÈ¢ÌÈçð
					if(t_sc_data && t_sc_data[SC_CLOSECONFINE].timer!=-1)
						status_change_end((struct block_list *)sc_data[type].val4,SC_CLOSECONFINE,-1);

					if(sc_data[type].val2==2)
					{
						//½©ÁêÈ¢éH
					}
					calc_flag = 1;
				}
				break;
			case SC_HOLDWEB:
				{
					struct status_change *t_sc_data = status_get_sc_data((struct block_list *)sc_data[type].val4);
					//ÐûªØê½ÌÅèÌSC_HOLDWEBóÔªØêÄÈ¢ÌÈçð
					if(t_sc_data && t_sc_data[SC_HOLDWEB].timer!=-1)
						status_change_end((struct block_list *)sc_data[type].val4,SC_HOLDWEB,-1);

					calc_flag = 1;
				}
				break;
			case SC_DANCING:
				{
					struct map_session_data *dsd;
					struct status_change *d_sc_data;
					//¾è¾¯±±ÅACRÁ
					if(sc_data[type].val1==CG_MOONLIT)
						clif_status_change(bl,SI_MOONLIT,0);	// ACRÁ

					if(sc_data[type].val4 && (dsd=map_id2sd(sc_data[type].val4))){
						d_sc_data = dsd->sc_data;
						//tÅèª¢éêèÌval4ð0É·é
						if(d_sc_data && d_sc_data[type].timer!=-1)
							d_sc_data[type].val4=0;
					}
					if(sc_data[SC_LONGINGFREEDOM].timer!=-1)
						status_change_end(bl,SC_LONGINGFREEDOM,-1);
				}
				calc_flag = 1;
				break;
			case SC_GRAFFITI:
				{
					struct skill_unit_group *sg=(struct skill_unit_group *)sc_data[type].val4;	//val4ªOtBeBÌgroup_id
					if(sg)
						skill_delunitgroup(sg);
				}
				break;
			case SC_NOCHAT:	//`bgÖ~óÔ
				if(sd)
					clif_updatestatus(sd,SP_MANNER);
				break;
			case SC_SPLASHER:		/* xiXvbV[ */
				{
					struct block_list *src=map_id2bl(sc_data[type].val3);
					if(src && tid!=-1){
						//©ªÉ_[WüÍ3*3É_[W
						skill_castend_damage_id(src, bl,sc_data[type].val2,sc_data[type].val1,gettick(),0 );
					}
				}
				break;
			case SC_ANKLE:
				{
					struct skill_unit_group *sg = (struct skill_unit_group *)sc_data[SC_ANKLE].val2;
					// skill_delunitgroup©çstatus_change_end ªÄÎêÈ¢×ÉA
					// ê[­®µÄ¢È¢ÉµÄ©çO[ví·éB
					if(sg) {
						sg->val2 = 0;
						skill_delunitgroup(sg);
					}
				}
				break;
			case SC_SELFDESTRUCTION:	/* © */
				if(bl->type == BL_MOB) {
					struct mob_data *md = (struct mob_data *)bl;
					if(md) {
						md->mode &= ~0x1;
						md->state.special_mob_ai = 2;
					}
				}
				break;
			case SC_TATAMIGAESHI://ôÔµ
			case SC_UTSUSEMI://#NJ_UTSUSEMI#
			case SC_BUNSINJYUTSU://#NJ_BUNSINJYUTSU#
				break;
			case SC_SUITON://#NJ_SUITON#
			case SC_NEN://#NJ_NEN#
			case SC_AVOID://#Ù}ñð#
			case SC_CHANGE://#^`FW#
			case SC_DEFENCE://#fBtFX#
			case SC_BLOODLUST://#ubhXg#
			case SC_FLEET://#t[g[u#
			case SC_SPEED://#I[o[hXs[h#
				calc_flag = 1;
				break;
			/* option1 */
			case SC_FREEZE:
				sc_data[type].val3 = 0;
				break;

		/* option2 */
			case SC_POISON:				/* Å */
			case SC_BLIND:				/* Ã */
			case SC_CURSE:
				calc_flag = 1;
				break;
		}


		if(bl->type==BL_PC && StatusIconChangeTable[type] != SI_BLANK)
			clif_status_change(bl,StatusIconChangeTable[type],0);	// ACRÁ

		switch(type){	/* ³íÉßéÆ«ÈÉ©ªKv */
		case SC_STONE:
		case SC_FREEZE:
		case SC_STAN:
		case SC_SLEEP:
			*opt1 = 0;
			opt_flag = 1;
			break;

		case SC_POISON:
			if (sc_data[SC_DPOISON].timer != -1)	//
				break;						// DPOISONpÌIvV
			*opt2 &= ~1;					// ªêpÉpÓ³ê½êÉÍ
			opt_flag = 1;					// ±±Íí·é
			break;							//
		case SC_CURSE:
		case SC_SILENCE:
			*opt2 &= ~(1<<(type-SC_POISON));
			opt_flag = 1;
			break;
		case SC_FOGWALLPENALTY:
			if(sc_data[SC_BLIND].timer==-1){
				*opt2 &= ~16;
				opt_flag = 1;
			}
			break;
		case SC_BLIND:
			if(sc_data[SC_FOGWALLPENALTY].timer==-1){
				*opt2 &= ~16;
				opt_flag = 1;
			}
			break;
		case SC_DPOISON:
			if (sc_data[SC_POISON].timer != -1)	// DPOISONpÌIvVª
				break;							// pÓ³ê½çí
			*opt2 &= ~1;	// ÅóÔð
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
		case SC_CHASEWALK:	/*`FCXEH[N*/
			*option &= ~16388;
			opt_flag = 1 ;
			break;
		case SC_INVISIBLE:
			*option &= ~64;
			opt_flag = 1 ;
			break;

		case SC_SIGHT:
			*option &= ~1;
			opt_flag = 1;
			break;
		case SC_WEDDING:	//¥p(¥ßÖÉÈÁÄà­Ìªx¢Æ©)
			{
				time_t timer;
				if( time(&timer) >= ((sc_data[type].val2) + battle_config.wedding_time) ){	//1Ôoß
					if(sd)
						pc_setglobalreg(sd,"PC_WEDDING_TIME",0);

					*option &= ~4096;
					opt_flag = 1;
				}
			}
			break;
		case SC_REVERSEORCISH:
			*option &= ~0x0800;
			opt_flag = 1;
			break;
		case SC_RUWACH:
			*option &= ~8192;
			opt_flag = 1;
			break;

		//opt3
		case SC_ONEHAND:			/* 1HQ */
		case SC_TWOHANDQUICKEN:		/* 2HQ */
		case SC_SPEARSQUICKEN:		/* XsANCbP */
		case SC_CONCENTRATION:		/* RZg[V */
			*opt3 &= ~1;
			opt_flag = 1;
			break;
		case SC_OVERTHRUST:			/* I[o[gXg */
			*opt3 &= ~2;
			opt_flag = 1;
			break;
		case SC_ENERGYCOAT:			/* GiW[R[g */
			*opt3 &= ~4;
			opt_flag = 1;
			break;
		case SC_EXPLOSIONSPIRITS:	// ôg®
			*opt3 &= ~8;
			opt_flag = 1;
			break;
		case SC_STEELBODY:			// à
			*opt3 &= ~16;
			opt_flag = 1;
			break;
		case SC_BLADESTOP:		/* næè */
			*opt3 &= ~32;
			opt_flag = 1;
			break;
		case SC_BERSERK:		/* o[T[N */
			*opt3 &= ~128;
			opt_flag = 1;
			break;
		case SC_MARIONETTE:		/* }IlbgRg[ */
		case SC_MARIONETTE2:		/* }IlbgRg[ */
			*opt3 &= ~1024;
			opt_flag = 1;
			break;
		case SC_ASSUMPTIO:		/* AXveBI */
			*opt3 &= ~2048;
			opt_flag = 1;
			break;
		case SC_SUN_WARM://#¾zÌ·àè#
		case SC_MOON_WARM://#Ì·àè#
		case SC_STAR_WARM://#¯Ì·àè#
			if(sc_data[SC_SUN_WARM].timer==-1
				&& sc_data[SC_MOON_WARM].timer==-1
				&& sc_data[SC_STAR_WARM].timer==-1){
				*opt3 &= ~4096;
				opt_flag = 1;
			}
			break;
		case SC_KAITE:
			*opt3 &= ~8192;
			opt_flag = 1;
			break;
		}

		if(soul_through){
			*opt3 &= ~32768;
			opt_flag = 1;
		}

		if(opt_flag) {	/* optionÌÏX */
			clif_changeoption(bl);
			if(sd)
				clif_changelook(&sd->bl,LOOK_CLOTHES_COLOR,sd->status.clothes_color);
		}

		if(sd // && !(flag&4) //±êðÇÁ·éÆ_ª¦éÆv¤¯Ç
		 	&& (calc_flag || sd->auto_status_calc_pc[type]==1))
		{
			status_calc_pc(sd,0);	/* Xe[^XÄvZ */
		}
		if(bl->type==BL_HOM && calc_flag)
		{
			homun_calc_status((struct homun_data*)bl);
			clif_send_homstatus(((struct homun_data*)bl)->msd,0);
		}

	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_calc_pc_stop_begin(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC)
		((struct map_session_data *)bl)->stop_status_calc_pc++;
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int status_calc_pc_stop_end(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC){
		struct map_session_data *sd = (struct map_session_data *)bl;
		sd->stop_status_calc_pc--;
		if(sd->stop_status_calc_pc==0 && sd->call_status_calc_pc_while_stopping>0)
			status_calc_pc(sd,0);
		if(sd->stop_status_calc_pc<0){
			puts("status_calc_pc_stop_endªs³ÉÄÑo³êÄ¢Ü·");
		}
	}
	return 0;
}
/*==========================================
 * Xe[^XÙíJn^C}[
 *------------------------------------------
 */
static int status_pretimer(int tid, unsigned int tick, int id, int data)
{
	struct block_list *bl = map_id2bl(id),*target;
	struct status_pretimer *stpt = NULL;
	struct unit_data *ud;

	if((bl=map_id2bl(id)) == NULL)
		return 0;	//YIDª·ÅÉÁÅµÄ¢é

	nullpo_retr(0, bl);
	nullpo_retr(0, ud = unit_bl2ud(bl));

	stpt = (struct status_pretimer*)data;
	linkdb_erase(&ud->statuspretimer, stpt);

	if(bl->prev == NULL) {
		free(stpt);
		return 0;
	}

	stpt->timer = -1;

	if(stpt->target_id){
		target = map_id2bl(stpt->target_id);
		if( target == NULL || bl->m != target->m || unit_isdead(bl) || unit_isdead(target) ) {
			free(stpt);
			return 0;
		}
	}else{
		if(bl->m != stpt->map){
			free(stpt);
			return 0;
		}
	}

	status_change_start(bl, stpt->type, stpt->val1, stpt->val2, stpt->val3, stpt->val4, stpt->tick, stpt->flag);
	free(stpt);

	return 0;
}

int status_clearpretimer(struct block_list *bl)
{
	struct unit_data *ud;
	struct linkdb_node *node1, *node2;

	nullpo_retr(0, bl);
	nullpo_retr(0, ud = unit_bl2ud(bl));

	node1 = ud->statuspretimer;
	while(node1){
		struct status_pretimer *stpt = node1->data;
		if(stpt->timer != -1){
			delete_timer(stpt->timer, status_pretimer);
		}
		node2 = node1->next;
		free(stpt);
		node1 = node2;
	}
	linkdb_final(&ud->statuspretimer);

	return 0;
}

int status_change_pretimer(struct block_list *bl,int type,int val1,int val2,int val3,int val4,int tick,int flag,int pre_tick)
{
	struct unit_data *ud;
	struct status_pretimer *stpt;
	nullpo_retr(1, bl);
	nullpo_retr(1, ud = unit_bl2ud(bl));

	stpt = calloc(1, sizeof(struct status_pretimer));
	stpt->timer = add_timer(pre_tick, status_pretimer, bl->id, (int)stpt);
	stpt->target_id = bl->id;
	stpt->map = bl->m;
	stpt->type = type;
	stpt->val1 = val1;
	stpt->val2 = val2;
	stpt->val3 = val3;
	stpt->val4 = val4;
	stpt->tick = tick;
	stpt->flag = flag;

	linkdb_insert(&ud->statuspretimer, stpt, stpt);

	return 0;
}

/*==========================================
 * Xe[^XÙíI¹^C}[
 *------------------------------------------
 */
int status_change_timer(int tid, unsigned int tick, int id, int data)
{
	int type=data;
	struct block_list *bl;
	struct map_session_data *sd=NULL;
	struct status_change *sc_data;
	//short *sc_count; //gÁÄÈ¢H

	if( (bl=map_id2bl(id)) == NULL )
		return 0; //YIDª·ÅÉÁÅµÄ¢éÆ¢¤ÌÍ¢©Éà è»¤ÈÌÅX[µÄÝé
	nullpo_retr(0, sc_data=status_get_sc_data(bl));

	if(bl->type==BL_PC)
		sd=(struct map_session_data *)bl;

	//sc_count=status_get_sc_count(bl); //gÁÄÈ¢H

	if(sc_data[type].timer != tid) {
		if(battle_config.error_log)
			printf("status_change_timer %d != %d\n",tid,sc_data[type].timer);
		return 0;
	}

	switch(type){	/* ÁêÈÉÈéê */
	case SC_MAXIMIZEPOWER:	/* }LV}CYp[ */
		if(sd){
			if( sd->status.sp > 0 ){	/* SPØêéÜÅ± */
				sd->status.sp--;
				clif_updatestatus(sd,SP_SP);
				sc_data[type].timer=add_timer(	/* ^C}[ÄÝè */
					sc_data[type].val2+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
		}
		break;
	case SC_CLOAKING:		/* N[LO */
	case SC_INVISIBLE:		/* CrWu */
		if(sd){
			if( sd->status.sp > 0 ){	/* SPØêéÜÅ± */
				sd->status.sp--;
				clif_updatestatus(sd,SP_SP);
				sc_data[type].timer=add_timer(	/* ^C}[ÄÝè */
					sc_data[type].val2+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
		}
		break;

	case SC_CHASEWALK:	/*`FCXEH[N*/
		if(sd){
			int sp = 10+sc_data[SC_CHASEWALK].val1*2;
			if (map[sd->bl.m].flag.gvg) sp *= 5;
			if (sd->status.sp > sp){
				sd->status.sp -= sp; // update sp cost [Celest]
				clif_updatestatus(sd,SP_SP);
				if ((++sc_data[SC_CHASEWALK].val4) == 1) {

					//[OÌ°
					if(sd->sc_data[SC_ROGUE].timer!=-1)
						status_change_start(bl, SC_CHASEWALK_STR, 1<<(sc_data[SC_CHASEWALK].val1-1), 0, 0, 0, 300000, 0);
					else status_change_start(bl, SC_CHASEWALK_STR, 1<<(sc_data[SC_CHASEWALK].val1-1), 0, 0, 0, 30000, 0);
					status_calc_pc(sd, 0);
				}
				sc_data[type].timer = add_timer( /* ^C}?ÄÝè */
					sc_data[type].val2+tick, status_change_timer, bl->id, data);
				return 0;
			}
		}
		break;

	case SC_HIDING:		/* nCfBO */
		if(sd){		/* SPª ÁÄAÔ§ÀÌÔÍ± */
			if( sd->status.sp > 0 && (--sc_data[type].val2)>0 ){
				if(sc_data[type].val2 % (sc_data[type].val1+3) ==0 ){
					sd->status.sp--;
					clif_updatestatus(sd,SP_SP);
				}
				sc_data[type].timer=add_timer(	/* ^C}[ÄÝè */
					1000+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
		}
		break;
	case SC_SIGHTBLASTER:
	case SC_SIGHT:	/* TCg */
	case SC_RUWACH:	/* At */
	case SC_DETECTING:
		{
			//const int range=10; C³OÍ¼û10x10¾Á½ÌÅ·
			// XLC³: TCgÌÍÍ=7x7 AtÌÍÍ=5x5
			int range = 2;
			if ( type == SC_SIGHT || type == SC_SIGHTBLASTER) range = 3;

			map_foreachinarea( status_change_timer_sub,
				bl->m, bl->x-range, bl->y-range, bl->x+range,bl->y+range,0,
				bl,type,tick);

			if( (--sc_data[type].val2)>0 ){
				sc_data[type].timer=add_timer(	/* ^C}[ÄÝè */
					250+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
		}
		break;

	case SC_SIGNUMCRUCIS:		/* VOiNVX */
		{
			int race = status_get_race(bl);
			if(race == 6 || battle_check_undead(race,status_get_elem_type(bl))) {
				sc_data[type].timer=add_timer(1000*600+tick,status_change_timer, bl->id, data );
				return 0;
			}
		}
		break;

	case SC_PROVOKE:	/* v{bN/I[go[T[N */
		if(sc_data[type].val2!=0){	/* I[go[T[NiPb²ÆÉHP`FbNj */
			if(sd && sd->status.hp>sd->status.max_hp>>2)	/* â~ */
				break;
			sc_data[type].timer=add_timer( 1000+tick,status_change_timer, bl->id, data );
			return 0;
		}
		break;

	case SC_DISSONANCE:	/* s¦a¹ */
		if( (--sc_data[type].val2)>0){
			struct skill_unit *unit = (struct skill_unit *)sc_data[type].val4;
			struct block_list *src;
			if(!unit || !unit->group)
				break;
			src=map_id2bl(unit->group->src_id);
			if(!src)
				break;
			battle_skill_attack(BF_MISC,src,&unit->bl,bl,unit->group->skill_id,sc_data[type].val1,tick,0);
			sc_data[type].timer=add_timer(skill_get_time2(unit->group->skill_id,unit->group->skill_lv)+tick,
				status_change_timer, bl->id, data );
			return 0;
		}
		break;
	case SC_UGLYDANCE:	/* ©ªèÈ_X */
		if( (--sc_data[type].val2)>0){
			struct skill_unit *unit = (struct skill_unit *)sc_data[type].val4;
			struct block_list *src;
			if(!unit || !unit->group)
				break;
			src=map_id2bl(unit->group->src_id);
			if(!src)
				break;
			skill_additional_effect(src,bl,unit->group->skill_id,sc_data[type].val1,0,tick);
			sc_data[type].timer=add_timer(skill_get_time2(unit->group->skill_id,unit->group->skill_lv)+tick,
				status_change_timer, bl->id, data );
			return 0;
		}
		break;

	case SC_SUN_WARM://¾zÌ·àè
	case SC_MOON_WARM://Ì·àè
	case SC_STAR_WARM://¯Ì·àè
		if( (--sc_data[type].val2)>0){
			map_foreachinarea( status_change_timer_sub,
				bl->m, bl->x-sc_data[type].val3, bl->y-sc_data[type].val3, bl->x+sc_data[type].val3,bl->y+sc_data[type].val3,0,
				bl,type,tick);
			sc_data[type].timer=add_timer(1000+tick, status_change_timer,bl->id, data);
			return 0;
		}
		break;

	case SC_LULLABY:	/* qçS */
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
			unit_stop_walking(bl,1);
			if(opt1) {
				*opt1 = 1;
				clif_changeoption(bl);
				if(sd)
					clif_changelook(&sd->bl,LOOK_CLOTHES_COLOR,sd->status.clothes_color);
			}
			sc_data[type].timer=add_timer(1000+tick,status_change_timer, bl->id, data );
			return 0;
		}
		else if( (--sc_data[type].val3) > 0) {
			int hp = status_get_max_hp(bl);
			if((++sc_data[type].val4)%5 == 0 && status_get_hp(bl) > hp>>2) {
				hp = (hp < 100)? 1: hp/100;
				if(sd)
					pc_heal(sd,-hp,0);
				else if(bl->type == BL_MOB){
					struct mob_data *md = (struct mob_data *)bl;
					if(md)
						md->hp -= hp;
				}
				else if(bl->type == BL_HOM){
					struct homun_data *hd = (struct homun_data *)bl;
					if(hd)
						homun_heal(hd,-hp,0);
				}
			}
			sc_data[type].timer=add_timer(1000+tick,status_change_timer, bl->id, data );
			return 0;
		}
		break;
	case SC_POISON:
		if (sc_data[SC_SLOWPOISON].timer == -1 && (--sc_data[type].val3) > 0) {
			int hp = status_get_hp(bl), p_dmg = status_get_max_hp(bl);
			if (hp > p_dmg>>2) {		// ÅåHPÌ25%Èã
				p_dmg = 3 + p_dmg*3/200;
				if(p_dmg >= hp) 
					p_dmg = hp-1;	// ÅÅÍÈÈ¢
				unit_heal(bl, -p_dmg, 0);
			}
		}
		if (sc_data[type].val3 > 0)
			sc_data[type].timer=add_timer(1000+tick,status_change_timer, bl->id, data );
		break;
	case SC_DPOISON:
		if (sc_data[SC_SLOWPOISON].timer == -1 && (--sc_data[type].val3) > 0) {
			int hp = status_get_max_hp(bl);
			if (status_get_hp(bl) > hp>>2) {
				hp = 3 + hp/50;
				unit_heal(bl, -hp, 0);
			}
		}
		if (sc_data[type].val3 > 0)
			sc_data[type].timer=add_timer(1000+tick,status_change_timer, bl->id, data );
		break;
	case SC_BLEED:
		if (--sc_data[type].val3 > 0) {
			int dmg = atn_rand()%600 + 200;
			if(bl->type == BL_MOB) {
				struct mob_data *md = (struct mob_data *)bl;
				if(md)
					md->hp = (md->hp - dmg < 50)? 50: md->hp - dmg;	// mobÍHP50ÈºÉÈçÈ¢
			} else {
				unit_heal(bl, -dmg, 0);
			}
			if(!unit_isdead(bl))
				sc_data[type].timer = add_timer(10000+tick, status_change_timer, bl->id, data);
			return 0;
		}
		break;
	case SC_TENSIONRELAX:	/* eVbNX */
		if(sd){		/* SPª ÁÄAHPª^ÅÈ¯êÎp± */
			if( sd->status.sp > 12 && sd->status.max_hp > sd->status.hp ){
				if(sc_data[type].val2 % (sc_data[type].val1+3) ==0 ){
					sd->status.sp -= 12;
					clif_updatestatus(sd,SP_SP);
				}
				sc_data[type].timer=add_timer(	/* ^C}[ÄÝè */
					10000+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
			if(sd->status.max_hp <= sd->status.hp)
				status_change_end(&sd->bl,SC_TENSIONRELAX,-1);
		}
		break;

	/* ÔØê³µHH */
	case SC_AETERNA:
	case SC_TRICKDEAD:
	case SC_RIDING:
	case SC_FALCON:
	case SC_WEIGHT50:
	case SC_WEIGHT90:
	case SC_MAGICPOWER:		/* @Í */
	case SC_REJECTSWORD:	/* WFNg\[h */
	case SC_MEMORIZE:		/* CY */
	case SC_SACRIFICE:		/* TNt@CX */
	case SC_READYSTORM:
	case SC_READYDOWN:
	case SC_READYTURN:
	case SC_READYCOUNTER:
	case SC_DODGE:
	case SC_AUTOBERSERK:
	case SC_RUN:
	case SC_MARIONETTE:
	case SC_MARIONETTE2:
		sc_data[type].timer=add_timer( 1000*600+tick,status_change_timer, bl->id, data );
		return 0;
	case SC_DEVIL:
		clif_status_change(bl,SI_DEVIL,1);
		sc_data[type].timer=add_timer( 1000*5+tick,status_change_timer, bl->id, data );
		break;
	case SC_LONGINGFREEDOM:
		if(sd && sd->status.sp >= 3) {
			if (--sc_data[type].val3 <= 0)
			{
				sd->status.sp -= 3;
				clif_updatestatus(sd, SP_SP);
				sc_data[type].val3 = 3;
			}
			sc_data[type].timer=add_timer(	/* ^C}[ÄÝè */
				1000 + tick, status_change_timer, bl->id, data);
			return 0;
		}
		break;
	case SC_DANCING: //_XXLÌÔSPÁï
		if(sd) {
			int s=0, cost=1;
			if(sc_data[type].val1 == CG_HERMODE)
				cost = 5;
			if(sd->status.sp >= cost && (--sc_data[type].val3)>0){
				switch(sc_data[type].val1){
				case BD_RICHMANKIM:				/* jhÌ 3bÉSP1 */
				case BD_DRUMBATTLEFIELD:		/* í¾ÛÌ¿« 3bÉSP1 */
				case BD_RINGNIBELUNGEN:			/* j[xOÌwÖ 3bÉSP1 */
				case BD_SIEGFRIED:				/* sgÌW[Nt[h 3bÉSP1 */
				case BA_DISSONANCE:				/* s¦a¹ 3bÅSP1 */
				case BA_ASSASSINCROSS:			/* [zÌATVNX 3bÅSP1 */
				case DC_UGLYDANCE:				/* ©ªèÈ_X 3bÅSP1 */
					s=3;
					break;
				case BD_LULLABY:				/* qçÌ 4bÉSP1 */
				case BD_ETERNALCHAOS:			/* iÌ¬× 4bÉSP1 */
				case BD_ROKISWEIL:				/* LÌ©Ñ 4bÉSP1 */
				case DC_FORTUNEKISS:			/* K^ÌLX 4bÅSP1 */
					s=4;
					break;
				case BD_INTOABYSS:				/* [£ÌÉ 5bÉSP1 */
				case BA_WHISTLE:				/* ûJ 5bÅSP1 */
				case DC_HUMMING:				/* n~O 5bÅSP1 */
				case BA_POEMBRAGI:				/* uMÌ 5bÅSP1 */
				case DC_SERVICEFORYOU:			/* T[rXtH[[ 5bÅSP1 */
				case CG_HERMODE:				//w[hÌñ
					s=5;
					break;
				case BA_APPLEIDUN:				/* ChDÌÑç 6bÅSP1 */
					s=6;
					break;
				case DC_DONTFORGETME:			/* ðYêÈ¢Åc 10bÅSP1 */
				case CG_MOONLIT:				/* ¾èÌºÅ 10bÅSP1H */
					s=10;
				case CG_LONGINGFREEDOM:
					s=3;
					break;
				}
				if(s && ((sc_data[type].val3 % s) == 0)){
					sd->status.sp-=cost;
					clif_updatestatus(sd,SP_SP);
				}
				sc_data[type].timer=add_timer(	/* ^C}[ÄÝè */
					1000+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
		}
		break;
	case SC_BERSERK:		/* o[T[N */
		if(sd){		/* HPª100ÈãÈçp± */
			if( (sd->status.hp - (sd->status.max_hp*5)/100) > 100 ){
				sd->status.hp -= (sd->status.max_hp*5)/100;
				clif_updatestatus(sd,SP_HP);
				sc_data[type].timer=add_timer(	/* ^C}[ÄÝè */
					10000+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
			else
				sd->status.hp = 100;
		}
		break;
	case SC_WEDDING:	//¥p(¥ßÖÉÈÁÄà­Ìªx¢Æ©)
		if(sd){
			time_t timer;
			if(time(&timer) < ((sc_data[type].val2) + battle_config.wedding_time)){	//1Ô½ÁÄ¢È¢ÌÅp±
				sc_data[type].timer=add_timer(	/* ^C}[ÄÝè */
					10000+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
		}
		break;
	case SC_NOCHAT:	//`bgÖ~óÔ
		if(sd){
			time_t timer;
			if((++sd->status.manner) && time(&timer) < ((sc_data[type].val2) + 60*(0-sd->status.manner))){	//Jn©çstatus.mannerªoÁÄÈ¢ÌÅp±
				clif_updatestatus(sd,SP_MANNER);
				sc_data[type].timer=add_timer(	/* ^C}[ÄÝè(60b) */
					60000+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
		}
		break;
	case SC_SELFDESTRUCTION:		/* © */
		if(bl->type == BL_MOB) {
			struct mob_data *md = (struct mob_data *)bl;
			if(md && unit_iscasting(&md->bl) && md->state.special_mob_ai > 2 && md->mode&0x1 && md->speed > 0) {
				md->speed -= 5;
				if(md->speed <= 0)
					md->speed = 1;
				md->next_walktime = 0;
				mob_linerwalk(md,sc_data[type].val3,tick);

				/* ^C}[ÄÝè */
				sc_data[type].timer = add_timer(100+tick, status_change_timer, bl->id, data);
				return 0;
			}
		}
		break;
	}

	if(sd && sd->state.dead_sit!=1 && sd->eternal_status_change[type]>0)
	{
		sc_data[type].timer=add_timer(	/* ^C}[ÄÝè */
			sd->eternal_status_change[type]+tick, status_change_timer,
			bl->id, data);
		return 0;
	}

	return status_change_end( bl,type,tid );
}

/*==========================================
 * Xe[^XÙí^C}[ÍÍ
 *------------------------------------------
 */
int status_change_timer_sub(struct block_list *bl, va_list ap )
{
	struct block_list *src;
	int type;
	unsigned int tick;
	struct map_session_data *sd=NULL, *tsd=NULL;
	short *opt;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, src=va_arg(ap,struct block_list*));
	type=va_arg(ap,int);
	tick=va_arg(ap,unsigned int);

	if(bl->type!=BL_PC && bl->type!=BL_MOB)
		return 0;

	BL_CAST( BL_PC, src, sd  );
	BL_CAST( BL_PC, bl,  tsd );

	opt = status_get_option(bl);

	switch( type ){
	case SC_SIGHT:	/* TCg */
	case SC_CONCENTRATE:
		if( *opt&6 ){
			status_change_end( bl, SC_HIDING, -1);
			status_change_end( bl, SC_CLOAKING, -1);
		}
		break;
	case SC_RUWACH:	/* At */
		if( *opt&6 ){
			status_change_end( bl, SC_HIDING, -1);
			status_change_end( bl, SC_CLOAKING, -1);
			if(battle_check_target( src,bl, BCT_ENEMY ) > 0) {
				struct status_change *sc_data = status_get_sc_data(bl);
				if(sc_data)
					battle_skill_attack(BF_MAGIC,src,src,bl,AL_RUWACH,sc_data[type].val1,tick,0);
			}
		}
		break;
	case SC_SIGHTBLASTER:
		if( *opt&6 ){
			status_change_end( bl, SC_HIDING, -1);
			status_change_end( bl, SC_CLOAKING, -1);

		}
		if(battle_check_target( src,bl, BCT_ENEMY ) > 0) {
			struct status_change *sc_data = status_get_sc_data(src);
			if(sc_data) {
				battle_skill_attack(BF_MAGIC,src,src,bl,WZ_SIGHTBLASTER,sc_data[type].val1,tick,0);
				sc_data[type].val2 = 0;
			}
			status_change_end(src,SC_SIGHTBLASTER,-1);
		}
		break;
	case SC_DETECTING:
		if( *opt&70 ){
			status_change_end( bl, SC_HIDING, -1);
			status_change_end( bl, SC_CLOAKING, -1);
			status_change_end( bl, SC_INVISIBLE, -1);
		}
		break;
	case SC_SUN_WARM://¾zÌ·àè
	case SC_MOON_WARM://Ì·àè
	case SC_STAR_WARM://¯Ì·àè
		if(battle_check_target( src,bl, BCT_ENEMY ) > 0) {
			if(sd){
				if(sd->status.sp<2)
				{
					status_change_end(&sd->bl,type,-1);
				}else{
					sd->status.sp -= 2;
					clif_updatestatus(sd,SP_SP);
				}
			}
			if(tsd)
			{
				tsd->status.sp -= 5;
				if(tsd->status.sp < 0)
					tsd->status.sp = 0;
				clif_updatestatus(tsd,SP_SP);
			}
			battle_weapon_attack(src,bl,tick,0);
			//«òÎµ
			if(map[bl->m].flag.gvg==0)
			{
				skill_blown(src,bl,2);
			}
		}
		break;
	}
	return 0;
}


/*==========================================
 * Xe[^XÙíI¹
 *------------------------------------------
 */
int status_encchant_eremental_end(struct block_list *bl,int type)
{
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	nullpo_retr(0, sc_data=status_get_sc_data(bl));

	if( type!=SC_ENCPOISON && sc_data[SC_ENCPOISON].timer!=-1 )	/* G`g|CYð */
		status_change_end(bl,SC_ENCPOISON,-1);
	if( type!=SC_ASPERSIO && sc_data[SC_ASPERSIO].timer!=-1 )	/* AXyVIð */
		status_change_end(bl,SC_ASPERSIO,-1);
	if( type!=SC_FLAMELAUNCHER && sc_data[SC_FLAMELAUNCHER].timer!=-1 )	/* tC`ð */
		status_change_end(bl,SC_FLAMELAUNCHER,-1);
	if( type!=SC_FROSTWEAPON && sc_data[SC_FROSTWEAPON].timer!=-1 )	/* tXgEF|ð */
		status_change_end(bl,SC_FROSTWEAPON,-1);
	if( type!=SC_LIGHTNINGLOADER && sc_data[SC_LIGHTNINGLOADER].timer!=-1 )	/* CgjO[_[ð */
		status_change_end(bl,SC_LIGHTNINGLOADER,-1);
	if( type!=SC_SEISMICWEAPON && sc_data[SC_SEISMICWEAPON].timer!=-1 )	/* TCX~bNEF|ð */
		status_change_end(bl,SC_SEISMICWEAPON,-1);
	if( type!=SC_DARKELEMENT && sc_data[SC_DARKELEMENT].timer!=-1 )		//Å
		status_change_end(bl,SC_DARKELEMENT,-1);
	if( type!=SC_ATTENELEMENT && sc_data[SC_ATTENELEMENT].timer!=-1 )	//O
		status_change_end(bl,SC_ATTENELEMENT,-1);
	if( type!=SC_UNDEADELEMENT && sc_data[SC_UNDEADELEMENT].timer!=-1 )	//s
		status_change_end(bl,SC_UNDEADELEMENT,-1);

	return 0;
}

/*==========================================
 * Xe[^XÙí(ÌÌ®«)I¹
 *------------------------------------------
 */
int status_enchant_armor_eremental_end(struct block_list *bl,int type)
{
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	nullpo_retr(0, sc_data=status_get_sc_data(bl));

	if( type!=SC_BENEDICTIO && sc_data[SC_BENEDICTIO].timer!=-1 )//¹Ì
		status_change_end(bl,SC_BENEDICTIO,-1);
	if( type!=SC_ELEMENTWATER && sc_data[SC_ELEMENTWATER].timer!=-1 )	//
		status_change_end(bl,SC_ELEMENTWATER,-1);
	if( type!=SC_ELEMENTGROUND && sc_data[SC_ELEMENTGROUND].timer!=-1 )	//n
		status_change_end(bl,SC_ELEMENTGROUND,-1);
	if( type!=SC_ELEMENTWIND && sc_data[SC_ELEMENTWIND].timer!=-1 )		//
		status_change_end(bl,SC_ELEMENTWIND,-1);
	if( type!=SC_ELEMENTFIRE && sc_data[SC_ELEMENTFIRE].timer!=-1 )		//Î
		status_change_end(bl,SC_ELEMENTFIRE,-1);
	if( type!=SC_ELEMENTHOLY && sc_data[SC_ELEMENTHOLY].timer!=-1 )	//õ
		status_change_end(bl,SC_ELEMENTHOLY,-1);
	if( type!=SC_ELEMENTDARK && sc_data[SC_ELEMENTDARK].timer!=-1 )		//Å
		status_change_end(bl,SC_ELEMENTDARK,-1);
	if( type!=SC_ELEMENTELEKINESIS && sc_data[SC_ELEMENTELEKINESIS].timer!=-1 )	//O
		status_change_end(bl,SC_ELEMENTELEKINESIS,-1);
	if( type!=SC_ELEMENTPOISON && sc_data[SC_ELEMENTPOISON].timer!=-1 )	//Å
		status_change_end(bl,SC_ELEMENTPOISON,-1);
	if( type!=SC_ELEMENTUNDEAD && sc_data[SC_ELEMENTUNDEAD].timer!=-1 )	//s
		status_change_end(bl,SC_ELEMENTUNDEAD,-1);

	return 0;
}

/*==========================================
 * Xe[^XÙí(í°ÏX)I¹
 *------------------------------------------
 */
int status_change_race_end(struct block_list *bl,int type)
{
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	nullpo_retr(0, sc_data=status_get_sc_data(bl));

	if( type!=SC_RACEUNDEAD && sc_data[SC_RACEUNDEAD].timer!=-1 )
		status_change_end(bl,SC_RACEUNDEAD,-1);
	if( type!=SC_RACEBEAST && sc_data[SC_RACEBEAST].timer!=-1 )
		status_change_end(bl,SC_RACEBEAST,-1);
	if( type!=SC_RACEPLANT && sc_data[SC_RACEPLANT].timer!=-1 )
		status_change_end(bl,SC_RACEPLANT,-1);
	if( type!=SC_RACEINSECT && sc_data[SC_RACEINSECT].timer!=-1 )
		status_change_end(bl,SC_RACEINSECT,-1);
	if( type!=SC_RACEFISH && sc_data[SC_RACEFISH].timer!=-1 )
		status_change_end(bl,SC_RACEFISH,-1);
	if( type!=SC_RACEDEVIL && sc_data[SC_RACEDEVIL].timer!=-1 )
		status_change_end(bl,SC_RACEDEVIL,-1);
	if( type!=SC_RACEHUMAN && sc_data[SC_RACEHUMAN].timer!=-1 )
		status_change_end(bl,SC_RACEHUMAN,-1);
	if( type!=SC_RACEANGEL && sc_data[SC_RACEANGEL].timer!=-1 )
		status_change_end(bl,SC_RACEANGEL,-1);
	if( type!=SC_RACEDRAGON && sc_data[SC_RACEDRAGON].timer!=-1 )
		status_change_end(bl,SC_RACEDRAGON,-1);

	return 0;
}

/*==========================================
 * Xe[^XÙí(í°ÏX)I¹
 *------------------------------------------
 */
int status_change_resistclear(struct block_list *bl)
{
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	nullpo_retr(0, sc_data=status_get_sc_data(bl));
	// Ï«
	if(sc_data[SC_RESISTWATER].timer!=-1)
		status_change_end(bl,SC_RESISTWATER,-1);
	if(sc_data[SC_RESISTGROUND].timer!=-1)
		status_change_end(bl,SC_RESISTGROUND,-1);
	if(sc_data[SC_RESISTFIRE].timer!=-1)
		status_change_end(bl,SC_RESISTFIRE,-1);
	if(sc_data[SC_RESISTWIND].timer!=-1)
		status_change_end(bl,SC_RESISTWIND,-1);
	if(sc_data[SC_RESISTPOISON].timer!=-1)
		status_change_end(bl,SC_RESISTPOISON,-1);
	if(sc_data[SC_RESISTHOLY].timer!=-1)
		status_change_end(bl,SC_RESISTHOLY,-1);
	if(sc_data[SC_RESISTDARK].timer!=-1)
		status_change_end(bl,SC_RESISTDARK,-1);
	if(sc_data[SC_RESISTTELEKINESIS].timer!=-1)
		status_change_end(bl,SC_RESISTTELEKINESIS,-1);
	if(sc_data[SC_RESISTUNDEAD].timer!=-1)
		status_change_end(bl,SC_RESISTUNDEAD,-1);
	return 0;
}

/*==========================================
 * Xe[^XÙí(°)Jn
 *------------------------------------------
 */
int status_change_soulstart(struct block_list *bl,int val1,int val2,int val3,int val4,int tick,int flag)
{
	int type = -1;
	struct pc_base_job s_class;
	nullpo_retr(0, bl);
	if(bl->type!=BL_PC)
		return 0;
	s_class = pc_calc_base_job(status_get_class(bl));

	switch(s_class.job){
		case 15:
			type = SC_MONK;
			break;
		case 25:
			type = SC_STAR;
			break;
		case 16:
			type = SC_SAGE;
		 	break;
		case 14:
			type = SC_CRUSADER;
			break;
		case 9:
			type = SC_WIZARD;
			break;
		case 8:
			type = SC_PRIEST;
			break;
		case 17:
			type = SC_ROGUE;
			break;
		case 12:
			type = SC_ASSASIN;
			break;
		case 27:
			type = SC_SOULLINKER;
			break;
		case 7:
			type = SC_KNIGHT;
			break;
		case 18:
			type = SC_ALCHEMIST;
			break;
		case 19:
		case 20:
			type = SC_BARDDANCER;
			break;
		case 10:
			type = SC_BLACKSMITH;
			break;
		case 11:
			type = SC_HUNTER;
			break;
		case 23:
			type = SC_SUPERNOVICE;
			break;
		default:
			if(s_class.upper==1 && s_class.job>=1 && s_class.job<=6)
				type = SC_HIGH;
			break;
	}
	if(type >= 0)
		status_change_start(bl,type,val1,val2,val3,val4,tick,flag);
	return 0;
}

/*==========================================
 * Xe[^XÙí(í°ÏX)I¹
 *------------------------------------------
 */
int status_change_soulclear(struct block_list *bl)
{
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	nullpo_retr(0, sc_data=status_get_sc_data(bl));

	if(sc_data[SC_MONK].timer!=-1)
		status_change_end(bl,SC_MONK,-1);
	if(sc_data[SC_STAR].timer!=-1)
		status_change_end(bl,SC_STAR,-1);
	if(sc_data[SC_SAGE].timer!=-1)
		status_change_end(bl,SC_SAGE,-1);
	if(sc_data[SC_CRUSADER].timer!=-1)
		status_change_end(bl,SC_CRUSADER,-1);
	if(sc_data[SC_WIZARD].timer!=-1)
		status_change_end(bl,SC_WIZARD,-1);
	if(sc_data[SC_PRIEST].timer!=-1)
		status_change_end(bl,SC_PRIEST,-1);
	if(sc_data[SC_ROGUE].timer!=-1)
		status_change_end(bl,SC_ROGUE,-1);
	if(sc_data[SC_ASSASIN].timer!=-1)
		status_change_end(bl,SC_ASSASIN,-1);
	if(sc_data[SC_SOULLINKER].timer!=-1)
		status_change_end(bl,SC_SOULLINKER,-1);
	if(sc_data[SC_KNIGHT].timer!=-1)
		status_change_end(bl,SC_KNIGHT,-1);
	if(sc_data[SC_ALCHEMIST].timer!=-1)
		status_change_end(bl,SC_ALCHEMIST,-1);
	if(sc_data[SC_BARDDANCER].timer!=-1)
		status_change_end(bl,SC_BARDDANCER,-1);
	if(sc_data[SC_BLACKSMITH].timer!=-1)
		status_change_end(bl,SC_BLACKSMITH,-1);
	if(sc_data[SC_HUNTER].timer!=-1)
		status_change_end(bl,SC_HUNTER,-1);
	if(sc_data[SC_HIGH].timer!=-1)
		status_change_end(bl,SC_HIGH,-1);
	if(sc_data[SC_SUPERNOVICE].timer!=-1)
		status_change_end(bl,SC_SUPERNOVICE,-1);
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

	// JOBâ³lP
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
		for(j=0,p=line;j<27 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(j<27)
			continue;
		max_weight_base[i]=atoi(split[0]);
		hp_coefficient[i]=atoi(split[1]);
		hp_coefficient2[i]=atoi(split[2]);
		sp_coefficient[i]=atoi(split[3]);
		for(j=0;j<23;j++)
			aspd_base[i][j]=atoi(split[j+4]);
		i++;
		if(i==MAX_VALID_PC_CLASS)
			break;
	}
	fclose(fp);
	printf("read db/job_db1.txt done\n");

	// JOB{[iX
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
			job_bonus[2][i][j]=k; //{qEÌ{[iXÍª©çÈ¢ÌÅ¼
			p=strchr(p,',');
			if(p) p++;
		}
		i++;
		if(i==MAX_VALID_PC_CLASS)
			break;
	}
	fclose(fp);
	printf("read db/job_db2.txt done\n");

	// JOB{[iX2 ]¶Ep
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

	// ¸Bf[^e[u
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
		refinebonus[i][0]=atoi(split[0]);	// ¸B{[iX
		refinebonus[i][1]=atoi(split[1]);	// ßè¸B{[iX
		refinebonus[i][2]=atoi(split[2]);	// ÀS¸BÀE
		for(j=0;j<10 && split[j];j++)
			percentrefinery[i][j]=atoi(split[j+3]);
		i++;
	}
	fclose(fp);
		printf("read db/refine_db.txt done\n");

	// TCYâ³e[u
	for(i=0;i<3;i++)
		for(j=0;j<30;j++)
			atkmods[i][j]=100;
	fp=fopen("db/size_fix.txt","r");
	if(fp==NULL){
		printf("can't read db/size_fix.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line,1020,fp)){
		char *split[30];
		if(line[0]=='/' && line[1]=='/')
			continue;
		if(atoi(line)<=0)
			continue;
		memset(split,0,sizeof(split));
		for(j=0,p=line;j<30 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		for(j=0;j<30 && split[j];j++)
			atkmods[i][j]=atoi(split[j]);
		i++;
	}
	fclose(fp);
	printf("read db/size_fix.txt done\n");

	for(i=0;i<MAX_STATUSCHANGE;i++) {
		dummy_sc_data[i].timer=-1;
		dummy_sc_data[i].val1 = dummy_sc_data[i].val2 = dummy_sc_data[i].val3 = dummy_sc_data[i].val4 =0;
	}

#ifdef DYNAMIC_SC_DATA
	puts("status_readdb: enable dynamic sc_data.");
#endif
	return 0;
}

/*==========================================
 * XLÖWú»
 *------------------------------------------
 */
int do_init_status(void)
{
	add_timer_func_list(status_change_timer,"status_change_timer");
	add_timer_func_list(status_pretimer,"status_pretimer");
	status_readdb();
	status_calc_sigma();
	return 0;
}

