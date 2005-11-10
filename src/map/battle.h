#ifndef _BATTLE_H_
#define _BATTLE_H_

#include <stdarg.h>

// ダメージ
struct Damage {
	int damage,damage2;
	int type,div_;
	int amotion,dmotion;
	int blewcount;
	int flag;
	int dmg_lv;	//囲まれ減算計算用　0:スキル攻撃 ATK_LUCKY,ATK_FLEE,ATK_DEF
};

// 属性表（読み込みはpc.c、battle_attr_fixで使用）
extern int attr_fix_table[4][10][10];

struct map_session_data;
struct mob_data;
struct block_list;

// ダメージ計算

struct Damage battle_calc_attack(	int attack_type,
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag);
struct Damage battle_calc_weapon_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag);
struct Damage battle_calc_magic_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag);
struct Damage  battle_calc_misc_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag);

// 属性修正計算
int battle_attr_fix(int damage,int atk_elem,int def_elem);

// ダメージ最終計算
int battle_calc_damage(struct block_list *src,struct block_list *bl,int damage,int div_,int skill_num,int skill_lv,int flag);
enum {	// 最終計算のフラグ
	BF_WEAPON	= 0x0001,
	BF_MAGIC	= 0x0002,
	BF_MISC		= 0x0004,
	BF_SHORT	= 0x0010,
	BF_LONG		= 0x0040,
	BF_SKILL	= 0x0100,
	BF_NORMAL	= 0x0200,
	BF_WEAPONMASK=0x000f,
	BF_RANGEMASK= 0x00f0,
	BF_SKILLMASK= 0x0f00,
};

// 実際にHPを増減
int battle_delay_damage(unsigned int tick,struct block_list *src,struct block_list *target,int damage,int flag);
int battle_damage(struct block_list *bl,struct block_list *target,int damage,int flag);
int battle_heal(struct block_list *bl,struct block_list *target,int hp,int sp,int flag);

//吸収処理(PC専用)
int battle_attack_drain(struct block_list *bl,struct block_list *target,int damage,int damage2,int calc_per_drain_flag);

// 攻撃処理まとめ
int battle_weapon_attack( struct block_list *bl,struct block_list *target,
	 unsigned int tick,int flag);
int battle_skill_attack(int attack_type,struct block_list* src,struct block_list *dsrc,
	 struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag);
int battle_skill_attack_area(struct block_list *bl,va_list ap);

enum {
	BCT_NOENEMY	=0x00000,
	BCT_PARTY	=0x10000,
	BCT_ENEMY	=0x40000,
	BCT_NOPARTY	=0x50000,
	BCT_ALL		=0x20000,
	BCT_NOONE	=0x60000,
};

int battle_check_undead(int race,int element);
int battle_check_target( struct block_list *src, struct block_list *target,int flag);
int battle_check_range(struct block_list *src,struct block_list *bl,int range);


// 設定
extern struct Battle_Config {
	int warp_point_debug;
	int enemy_critical;
	int enemy_critical_rate;
	int enemy_str;
	int enemy_perfect_flee;
	int cast_rate,delay_rate,delay_dependon_dex;
	int sdelay_attack_enable;
	int left_cardfix_to_right;
	int pc_skill_add_range;
	int skill_out_range_consume;
	int mob_skill_add_range;
	int pc_damage_delay;
	int pc_damage_delay_rate;
	int defnotenemy;
	int random_monster_checklv;
	int attr_recover;
	int flooritem_lifetime;
	int item_auto_get;
	int item_first_get_time;
	int item_second_get_time;
	int item_third_get_time;
	int mvp_item_first_get_time;
	int mvp_item_second_get_time;
	int mvp_item_third_get_time;
	int item_rate,base_exp_rate,job_exp_rate;
	int drop_rate0item;
	int death_penalty_type;
	int death_penalty_base,death_penalty_job;
	int zeny_penalty;
	int restart_hp_rate;
	int restart_sp_rate;
	int mvp_item_rate,mvp_exp_rate;
	int mvp_hp_rate;
	int monster_hp_rate;
	int monster_max_aspd;
	int atc_gmonly;
	int gm_allskill;
	int gm_allskill_addabra;
	int gm_allequip;
	int gm_skilluncond;
	int skillfree;
	int skillup_limit;
	int wp_rate;
	int pp_rate;
	int cdp_rate;
	int monster_active_enable;
	int monster_damage_delay_rate;
	int monster_loot_type;
	int mob_skill_use;
	int mob_count_rate;
	int mob_delay_rate;
	int quest_skill_learn;
	int quest_skill_reset;
	int basic_skill_check;
	int guild_emperium_check;
	int guild_exp_limit;
	int pc_invincible_time;
	int pet_catch_rate;
	int pet_rename;
	int pet_friendly_rate;
	int pet_hungry_delay_rate;
	int pet_hungry_friendly_decrease;
	int pet_skill_use;
	int pet_str;
	int pet_status_support;
	int pet_attack_support;
	int pet_damage_support;
	int pet_support_rate;
	int pet_attack_exp_to_master;
	int pet_attack_exp_rate;
	int skill_min_damage;
	int finger_offensive_type;
	int heal_exp;
	int resurrection_exp;
	int shop_exp;
	int combo_delay_rate;
	int item_check;
	int wedding_relog;
	int wedding_time;
	int wedding_modifydisplay;
	int natural_healhp_interval;
	int natural_healsp_interval;
	int natural_heal_skill_interval;
	int natural_heal_weight_rate;
	int item_name_override_grffile;
	int arrow_decrement;
	int max_aspd;
	int max_hp;
	int max_sp;
	int max_parameter;
	int max_cart_weight;
	int pc_skill_log;
	int mob_skill_log;
	int battle_log;
	int save_log;
	int error_log;
	int etc_log;
	int save_clothcolor;
	int undead_detect_type;
	int pc_auto_counter_type;
	int monster_auto_counter_type;
	int agi_penaly_type;
	int agi_penaly_count;
	int agi_penaly_num;
	int vit_penaly_type;
	int vit_penaly_count;
	int vit_penaly_num;
	int player_defense_type;
	int monster_defense_type;
	int pet_defense_type;
	int magic_defense_type;
	int pc_skill_reiteration;
	int monster_skill_reiteration;
	int pc_skill_nofootset;
	int monster_skill_nofootset;
	int pc_cloak_check_type;
	int monster_cloak_check_type;
	int gvg_short_damage_rate;
	int gvg_long_damage_rate;
	int gvg_magic_damage_rate;
	int gvg_misc_damage_rate;
	int gvg_eliminate_time;
	int mob_changetarget_byskill;
	int pc_attack_direction_change;
	int monster_attack_direction_change;
	int pc_land_skill_limit;
	int monster_land_skill_limit;
	int party_skill_penaly;
	int monster_class_change_full_recover;
	int produce_item_name_input;
	int produce_potion_name_input;
	int making_arrow_name_input;
	int holywater_name_input;
	int display_delay_skill_fail;
	int display_snatcher_skill_fail;
	int chat_warpportal;
	int mob_warpportal;
	int dead_branch_active;
	int vending_max_value;
	int pet_lootitem;
	int pet_weight;
	int show_steal_in_same_party;
	int enable_upper_class;
	int pet_attack_attr_none;
	int mob_attack_attr_none;
	int pc_attack_attr_none;

	int agi_penaly_count_lv;
	int vit_penaly_count_lv;

	int gx_allhit;
	int gx_cardfix;
	int gx_dupele;
	int gx_disptype;
	int devotion_level_difference;
	int player_skill_partner_check;
	int sole_concert_type;
	int hide_GM_session;
	int unit_movement_type;
	int invite_request_check;
	int skill_removetrap_type;
	int disp_experience;
	int castle_defense_rate;
	int riding_weight;
	int hp_rate;
	int sp_rate;
	int gm_can_drop_lv;
	int disp_hpmeter;
	int bone_drop;
	int bone_drop_itemid;
	int card_drop_rate;
	int equip_drop_rate;
	int refine_drop_rate;
	int Item_res;
	int next_exp_limit;
	int heal_counterstop;
	int finding_ore_drop_rate;
	int no_spel_dex1;
	int no_spel_dex2;
	int pt_bonus_b;
	int pt_bonus_j;
	int equip_autospell_nocost;
	int limit_gemstone;
	int mpv_announce;
	int petowneditem;
	int buyer_name;
	int once_autospell;
	//int expand_autospell;
	int allow_same_autospell;
	int combo_delay_lower_limits;
	int new_marrige_skill;
	int reveff_plus_addeff;
	int summonslave_no_drop;
	int summonslave_no_exp;
	int summonslave_no_mvp;
	int summonmonster_no_drop;
	int summonmonster_no_exp;
	int summonmonster_no_mvp;
	int cannibalize_no_drop;
	int cannibalize_no_exp;
	int cannibalize_no_mvp;
	int branch_mob_no_drop;
	int branch_mob_no_exp;
	int branch_mob_no_mvp;
	int branch_boss_no_drop;
	int branch_boss_no_exp;
	int branch_boss_no_mvp;
	int pc_hit_stop_type;
	int nomanner_mode;
	int death_by_unrig_penalty;
	int dance_and_play_duration;
	int soulcollect_max_fail;
	int gvg_flee_rate;
	int gvg_flee_penaly;
	int equip_sex;
	int weapon_attack_autospell;
	int magic_attack_autospell;
	int	misc_attack_autospell;
	int magic_attack_drain;
	int magic_attack_drain_per_enable;
	int	misc_attack_drain;
	int	misc_attack_drain_per_enable;
	int hallucianation_off;
	int weapon_reflect_autospell;
	int magic_reflect_autospell;
	int weapon_reflect_drain;
	int weapon_reflect_drain_per_enable;
	int	magic_reflect_drain;
	int	magic_reflect_drain_per_enable;
	int extended_cloneskill;
	int max_parameter_str;
	int max_parameter_agi;
	int max_parameter_vit;
	int max_parameter_int;
	int max_parameter_dex;
	int max_parameter_luk;
	int cannibalize_nocost;
	int spheremine_nocost;
	int demonstration_nocost;
	int acidterror_nocost;
	int aciddemonstration_nocost;
	int chemical_nocost;
	int slimpitcher_nocost;
	int allow_assumptop_in_gvg;
	int allow_falconassault_elemet;
	int allow_guild_invite_in_gvg;
	int allow_guild_leave_in_gvg;
	int guild_skill_available;
	int guild_hunting_skill_available;
	int guild_skill_check_range;
	int allow_guild_skill_in_gvg_only;
	int allow_me_guild_skill;
	int emergencycall_point_randam;
	int emergencycall_call_limit;
	int allow_guild_skill_in_gvgtime_only;
	int guild_skill_in_pvp_limit;
	int guild_exp_rate;
	int guild_skill_effective_range;
	int tarotcard_display_position;
	int job_soul_check;
	int repeal_die_counter_rate;
	int disp_job_soul_state_change;
	int check_knowlege_map;
	int tripleattack_rate_up_keeptime;
	int tk_counter_rate_up_keeptime;
	int allow_skill_without_day;
	int save_hate_mob;
	int twilight_party_check;
	int alchemist_point_type;
	int marionette_type;
	int baby_status_max;
	int baby_hp_rate;
	int baby_sp_rate;
	int baby_weight_rate;
	int no_emergency_call;
	int save_am_pharmacy_success;
	int save_all_ranking_point_when_logout;
	int soul_linker_battle_mode;
	int soul_linker_battle_mode_ka;
	int skillup_type;
	int debug_new_disp_status_icon_system;
	int allow_me_dance_effect;
	int allow_me_concert_effect;
	int pharmacy_get_point_type;
	int cheat_log;
	int soulskill_can_be_used_for_myself;
	int hermode_gvg_only;
	int hermode_wp_check_range;
	int hermode_wp_check;
	int hermode_no_walking;
	int atcommand_go_significant_values;
	int abraskill_coloneable;
	int expansion_job1_skill_cloneable;
	int expansion_job2_skill_cloneable;
	int questskill2_cloneable;
	int redemptio_penalty_type;
	int allow_weaponrearch_to_weaponrefine;
	int boss_no_knockbacking;
	int scroll_produce_rate;
	int allow_create_scroll;
	int scroll_item_name_input;
	int pet_leave;
	int pk_short_damage_rate;
	int pk_long_damage_rate;
	int pk_magic_damage_rate;
	int pk_misc_damage_rate;
	int cooking_rate;
	int making_rate;

	int item_rate_details,item_rate_1,item_rate_10,item_rate_100,item_rate_1000;	//ドロップレート詳細
	int item_rate_1_min,item_rate_10_min,item_rate_100_min,item_rate_1000_min;	//ドロップレート詳細min
	int item_rate_1_max,item_rate_10_max,item_rate_100_max,item_rate_1000_max;	//ドロップレート詳細max

	int monster_damage_delay;

	int noportal_flag;
	int noexp_hiding;
	int noexp_trickdead;

	int gm_hide_attack_lv;
	int hide_attack;

	int serverside_friendlist;
	int pet0078_hair_id;

	int mes_send_type;

} battle_config;

#define BATTLE_CONF_FILENAME	"conf/battle_athena.conf"
int battle_config_read(const char *cfgName);

#endif
