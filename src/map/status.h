#ifndef _STATUS_H_
#define _STATUS_H_

// パラメータ所得系 battle.c より移動
int status_get_class(struct block_list *bl);
int status_get_dir(struct block_list *bl);
int status_get_lv(struct block_list *bl);
int status_get_range(struct block_list *bl);
int status_get_group(struct block_list *bl);
int status_get_hp(struct block_list *bl);
int status_get_sp(struct block_list *bl);
int status_get_max_hp(struct block_list *bl);
int status_get_str(struct block_list *bl);
int status_get_agi(struct block_list *bl);
int status_get_vit(struct block_list *bl);
int status_get_int(struct block_list *bl);
int status_get_dex(struct block_list *bl);
int status_get_luk(struct block_list *bl);
int status_get_hit(struct block_list *bl);
int status_get_flee(struct block_list *bl);
int status_get_def(struct block_list *bl);
int status_get_mdef(struct block_list *bl);
int status_get_flee2(struct block_list *bl);
int status_get_def2(struct block_list *bl);
int status_get_mdef2(struct block_list *bl);
int status_get_baseatk(struct block_list *bl);
int status_get_atk(struct block_list *bl);
int status_get_atk2(struct block_list *bl);
int status_get_speed(struct block_list *bl);
int status_get_adelay(struct block_list *bl);
int status_get_amotion(struct block_list *bl);
int status_get_dmotion(struct block_list *bl);
int status_get_element(struct block_list *bl);
int status_get_attack_element(struct block_list *bl);
int status_get_attack_element2(struct block_list *bl);  //左手武器属性取得
int status_get_sevenwind_element(struct block_list *bl);
int status_get_sevenwind_element2(struct block_list *bl);  //左手武器属性取得
#define status_get_elem_type(bl)	(status_get_element(bl)%10)
#define status_get_elem_level(bl)	(status_get_element(bl)/10/2)
int status_get_party_id(struct block_list *bl);
int status_get_guild_id(struct block_list *bl);
int status_get_race(struct block_list *bl);
int status_get_size(struct block_list *bl);
int status_get_mode(struct block_list *bl);
int status_get_mexp(struct block_list *bl);
int status_get_enemy_type(struct block_list *bl);

struct status_change *status_get_sc_data(struct block_list *bl);
short *status_get_sc_count(struct block_list *bl);
short *status_get_opt1(struct block_list *bl);
short *status_get_opt2(struct block_list *bl);
short *status_get_opt3(struct block_list *bl);
short *status_get_option(struct block_list *bl);

int status_get_matk1(struct block_list *bl);
int status_get_matk2(struct block_list *bl);
int status_get_critical(struct block_list *bl);
int status_get_atk_(struct block_list *bl);
int status_get_atk_2(struct block_list *bl);
int status_get_atk2(struct block_list *bl);
int status_get_aspd(struct block_list *bl);

// 状態異常関連
int status_change_start(struct block_list *bl,int type,int val1,int val2,int val3,int val4,int tick,int flag);
int status_change_end( struct block_list* bl , int type,int tid );
int status_change_pretimer(struct block_list *bl,int type,int val1,int val2,int val3,int val4,int tick,int flag,int pre_tick);
int status_change_timer(int tid, unsigned int tick, int id, int data);
int status_change_timer_sub(struct block_list *bl, va_list ap );
int status_change_end_by_jumpkick( struct block_list* bl);
int status_support_magic_skill_end( struct block_list* bl);
int status_change_race_end(struct block_list *bl,int type);
int status_change_soulstart(struct block_list *bl,int val1,int val2,int val3,int val4,int tick,int flag);
int status_change_soulclear(struct block_list *bl);
int status_change_resistclear(struct block_list *bl);
int status_enchant_armor_eremental_end(struct block_list *bl,int type);
int status_encchant_eremental_end(struct block_list *bl,int type);
int status_change_clear(struct block_list *bl,int type);
int status_clearpretimer(struct block_list *bl);

//状態チェック
int status_check_tigereye(struct block_list *bl);
int status_check_attackable_by_tigereye(struct block_list *bl);
int status_check_no_magic_damage(struct block_list *bl);
#ifdef DYNAMIC_SC_DATA
int status_calloc_sc_data(struct block_list *bl);
int status_free_sc_data(struct block_list *bl);
int status_check_dummy_sc_data(struct block_list *bl);
#endif
// ステータス計算
int status_calc_pc_stop_begin(struct block_list *bl);
int status_calc_pc_stop_end(struct block_list *bl);
int status_calc_pc(struct map_session_data* sd,int first);
int status_calc_skilltree(struct map_session_data *sd);
int status_getrefinebonus(int lv,int type);
int status_percentrefinery(struct map_session_data *sd,struct item *item);
int status_percentrefinery_weaponrefine(struct map_session_data *sd,struct item *item);
extern int current_equip_item_index;
extern int current_equip_card_id;

extern struct status_change dummy_sc_data[MAX_STATUSCHANGE];
//DB再読込用
int status_readdb(void);

int do_init_status(void);

#endif
