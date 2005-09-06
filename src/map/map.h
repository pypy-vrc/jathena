#ifndef _MAP_H_
#define _MAP_H_

#include <stdarg.h>
#include "mmo.h"
#include "script.h"
#include "ranking.h"

#ifdef TKSGSL
#define MAX_VALID_PC_CLASS 28
#else
#define MAX_VALID_PC_CLASS 24
#endif

//���̂����������̂Œǉ�
#define PC_CLASS_NV      0  //�m�r
#define PC_CLASS_NV2  4001  //�]���m�r
#define PC_CLASS_NV3  4023  //�{�q�m�r
#define PC_CLASS_SNV    23  //�X�p�m�r
#define PC_CLASS_SNV3 4045  //�{�q�X�p�m�r
#define PC_CLASS_TK   4046	//�e�R��
#define PC_CLASS_SG   4047	//����
#define PC_CLASS_SG2  4048	//����2
#define PC_CLASS_SL   4049	//�\�E�������J�[


#define MAX_PC_CLASS (1+6+6+1+6+1+1+1+1+4)
#define PC_CLASS_BASE 0
#define PC_CLASS_BASE2 (PC_CLASS_BASE + 4001)
#define PC_CLASS_BASE3 (PC_CLASS_BASE2 + 22)
#define MAX_NPC_PER_MAP 512
#define BLOCK_SIZE 8
//#define AREA_SIZE 20
#define AREA_SIZE 14
#define PT_AREA_SIZE 20
#define LOCAL_REG_NUM 16
#define LIFETIME_FLOORITEM 60
#define DAMAGELOG_SIZE 30
#define LOOTITEM_SIZE 10
#define MAX_SKILL_ID 500
#define MAX_SKILL_LEVEL 100
#define MAX_STATUSCHANGE 290
#define MAX_SKILLUNITGROUP	32
#define MAX_MOBSKILLUNITGROUP	8
#define MAX_SKILLUNITGROUPTICKSET	32
#define MAX_SKILLTIMERSKILL 32
#define MAX_MOBSKILLTIMERSKILL 10
#define MAX_MOBSKILL	32
#define MAX_EVENTQUEUE	2
#define MAX_EVENTTIMER	32
#define NATURAL_HEAL_INTERVAL 500
#define MAX_FLOORITEM 500000
#define MAX_LEVEL 255
#define MAX_WALKPATH 32
#define MAX_DROP_PER_MAP 48
#define MAX_WIS_REFUSAL 14
#define MAX_MOBGROUP	10
#define MAX_ITEMGROUP	10
#define MAX_SKILL_DAMAGE_UP	10	//�X�L���������ł��鐔
#define MAX_SKILL_BLOW  5		//�X�L���𐁂���΂���
#define MAX_BONUS_AUTOSPELL  16	//�I�[�g�X�y���̗e��
#define	MAX_ADDRATE_AUTOSPELL 5
#define MAX_BAN_AUTOSPELL	  5
#define MAX_RANKING 3	//�����L���O��
#define MAX_RANKER  10	//�����L���O�l��


#ifndef DEFAULT_AUTOSAVE_INTERVAL
#define DEFAULT_AUTOSAVE_INTERVAL 60*1000
#endif

#define MAP_CONF_NAME	"conf/map_athena.conf"

enum { BL_NUL, BL_PC, BL_NPC, BL_MOB, BL_ITEM, BL_CHAT, BL_SKILL , BL_PET };
enum { WARP, SHOP, SCRIPT, MONS };
struct block_list {
	struct block_list *next,*prev;
	int id;
	short m,x,y;
	unsigned char type;
	unsigned char subtype;
};

struct walkpath_data {
	unsigned char path_len,path_pos,path_half;
	unsigned char path[MAX_WALKPATH];
};

struct shootpath_data {
	int rx,ry,len;
	int x[MAX_WALKPATH];
	int y[MAX_WALKPATH];
};

struct script_reg {
	int index;
	int data;
};
struct script_regstr {
	int index;
	char data[256];
};
struct status_change {
	int timer;
	int val1,val2,val3,val4;
};
struct vending {
	short index;
	short amount;
	int value;
};

struct skill_unit_group;
struct skill_unit {
	struct block_list bl;

	struct skill_unit_group *group;

	int limit;
	int val1,val2;
	short alive,range;
};
struct skill_unit_group {
	int src_id;
	int party_id;
	int guild_id;
	int map;
	int target_flag;
	unsigned int tick;
	int limit,interval;

	int skill_id,skill_lv;
	int val1,val2,val3;
	char *valstr;
	int unit_id;
	int group_id;
	int unit_count,alive_count;
	struct skill_unit *unit;
};

struct skill_unit_group_tickset {
	unsigned int tick;
	int id;
};

struct skill_timerskill {
	int timer;
	int src_id;
	int target_id;
	int map;
	short x,y;
	short skill_id,skill_lv;
	int type;
	int flag;
};

//�g���I�[�g�X�y��
//EQUIP_AUTOSPELL_FLAG
enum 	{
		EAS_ATTACK   	= 0x0001,//�U���p
		EAS_REVENGE   	= 0x0002,//�����p
		EAS_LONG_ATTACK = 0x0004,//�U���p
		EAS_LONG_REVENGE= 0x0008,//�����p
		EAS_TARGET 		= 0x0010,//�����Ɏg��
		EAS_SELF   		= 0x0020,//�����Ɏg��
		EAS_TARGET_RAND = 0x0040,//�������U���ΏۂɎg��
		//EAS_TARGET_ = 0x0080,//�������U���ΏۂɎg��
		EAS_FLUCT  		= 0x0100,//��AS�p 1�`3�̂���
		EAS_RANDOM  	= 0x0200,//1�`�w��܂Ń����_��
		EAS_USEMAX		= 0x0400,//MAX���x��������΂����
		EAS_USEBETTER	= 0x0800,//�w��ȏ�̂��̂�����΂����(MAX����Ȃ��Ă��\)
		EAS_NOSP 	 	= 0x1000,// SP0
		EAS_SPCOST1	 	= 0x2000,// SP2/3
		EAS_SPCOST2	 	= 0x4000,// SP1/2
		EAS_SPCOST3	 	= 0x8000,// SP1.5�{
};

struct npc_data;
struct pet_db;
struct item_data;
struct square;

struct map_session_data {
	struct block_list bl;
	struct {
		unsigned auth : 1 ;
		unsigned change_walk_target : 1 ;
		unsigned attack_continue : 1 ;
		unsigned menu_or_input : 1;
		unsigned dead_sit : 2;
		unsigned skillcastcancel : 1;
		unsigned waitingdisconnect : 1;
		unsigned lr_flag : 2;
		unsigned connect_new : 1;
		unsigned arrow_atk : 1;
		unsigned attack_type : 3;
		unsigned skill_flag : 1;
		unsigned gangsterparadise : 1;
		unsigned produce_flag : 1;
		unsigned make_arrow_flag : 1;
		unsigned potionpitcher_flag : 1;
		unsigned storage_flag : 1;
	} state;
	struct {
		unsigned restart_full_recover : 1;
		unsigned no_castcancel : 1;
		unsigned no_castcancel2 : 1;
		unsigned no_sizefix : 1;
		unsigned no_magic_damage : 1;
		unsigned no_weapon_damage : 1;
		unsigned no_gemstone : 1;
		unsigned infinite_endure : 1;
	} special_state;
	int char_id,login_id1,login_id2,sex;
	struct mmo_charstatus status;
	struct item_data *inventory_data[MAX_INVENTORY];
	short equip_index[11];
	unsigned short unbreakable_equip;
	int weight,max_weight;
	int cart_weight,cart_max_weight,cart_num,cart_max_num;
	char mapname[24];
	int fd,new_fd;
	short to_x,to_y;
	short speed,prev_speed;
	short opt1,opt2,opt3;
	char dir,head_dir;
	unsigned int client_tick,server_tick;
	struct walkpath_data walkpath;
	int walktimer;
	int npc_id,areanpc_id,npc_shopid;
	int npc_allowuseitem;
	int npc_pos;
	int npc_menu;
	int npc_amount;
	struct script_stack *stack;
	unsigned char *npc_script,*npc_scriptroot;
	int npc_scriptstate;
	char npc_str[256];
	unsigned int chatID;

	char wis_refusal[MAX_WIS_REFUSAL][24];	//Wis���ۃ��X�g
	int wis_all;	//Wis�S���ۋ��t���O
	int refuse_emergencycall;

	int attacktimer;
	int attacktarget;
	short attacktarget_lv;
	unsigned int attackabletime;

	short attackrange,attackrange_;
	int skilltimer;
	int skilltarget;
	unsigned int skillstatictimer[MAX_SKILL_ID];
	short skillx,skilly;
	short skillid,skilllv;
	short skillitem,skillitemlv;
	short skillid_old,skilllv_old;
	short skillid_dance,skilllv_dance;
	struct skill_unit_group skillunit[MAX_SKILLUNITGROUP];
	struct skill_unit_group_tickset skillunittick[MAX_SKILLUNITGROUPTICKSET];
	struct skill_timerskill skilltimerskill[MAX_SKILLTIMERSKILL];
	int cloneskill_id,cloneskill_lv;
	int potion_hp,potion_sp,potion_per_hp,potion_per_sp;

	int invincible_timer;
	unsigned int canact_tick;
	unsigned int canmove_tick;
	int hp_sub,sp_sub;
	int inchealhptick,inchealsptick,inchealspirithptick,inchealspiritsptick;

	short view_class;
	short weapontype1,weapontype2;
	int paramb[6],paramc[6],parame[6],paramcard[6];
	int hit,flee,flee2,aspd,amotion,dmotion;
	int watk,watk2,atkmods[3];
	int def,def2,mdef,mdef2,critical,matk1,matk2;
	int atk_ele,def_ele,star,overrefine;
	int castrate,hprate,sprate,dsprate;
	int addele[10],addrace[12],addsize[3],subele[10],subrace[12];
	int addeff[10],addeff2[10],reseff[10];
	int watk_,watk_2,atkmods_[3],addele_[10],addrace_[12],addsize_[3];	//�񓁗��̂��߂ɒǉ�
	int atk_ele_,star_,overrefine_;				//�񓁗��̂��߂ɒǉ�
	int base_atk,atk_rate;
	int weapon_atk[16],weapon_atk_rate[16];	//�w��
	int arrow_atk,arrow_ele,arrow_cri,arrow_hit,arrow_range;
	int arrow_addele[10],arrow_addrace[12],arrow_addsize[3],arrow_addeff[10],arrow_addeff2[10];
	int nhealhp,nhealsp,nshealhp,nshealsp,nsshealhp,nsshealsp;
	int aspd_rate,speed_rate,hprecov_rate,sprecov_rate,critical_def,double_rate;
	int near_attack_def_rate,long_attack_def_rate,magic_def_rate,misc_def_rate;
	int matk_rate,ignore_def_ele,ignore_def_race,ignore_def_ele_,ignore_def_race_;
	int ignore_mdef_ele,ignore_mdef_race;
	int magic_addele[10],magic_addrace[12],magic_subrace[12];
	int perfect_hit,get_zeny_num;
	int critical_rate,hit_rate,flee_rate,flee2_rate,def_rate,def2_rate,mdef_rate,mdef2_rate;
	int def_ratio_atk_ele,def_ratio_atk_ele_,def_ratio_atk_race,def_ratio_atk_race_;
	int add_damage_class_count,add_damage_class_count_,add_magic_damage_class_count;
	short add_damage_classid[10],add_damage_classid_[10],add_magic_damage_classid[10];
	int add_damage_classrate[10],add_damage_classrate_[10],add_magic_damage_classrate[10];
	short add_def_class_count,add_mdef_class_count;
	short add_def_classid[10],add_mdef_classid[10];
	int add_def_classrate[10],add_mdef_classrate[10];
	short monster_drop_item_count;
	short monster_drop_itemid[10];
	int monster_drop_race[10],monster_drop_itemrate[10];
	int double_add_rate,speed_add_rate,aspd_add_rate,perfect_hit_add, get_zeny_add_num;
	short splash_range,splash_add_range;
	short autospell_id,autospell_lv,autospell_rate,autospell_flag;
	short hp_drain_rate,hp_drain_per,sp_drain_rate,sp_drain_per;
	short hp_drain_rate_,hp_drain_per_,sp_drain_rate_,sp_drain_per_;
	short hp_drain_value,sp_drain_value,hp_drain_value_,sp_drain_value_;
	int short_weapon_damage_return,long_weapon_damage_return,magic_damage_return;
	int weapon_coma_ele[10],weapon_coma_race[12];
	int weapon_coma_ele2[10],weapon_coma_race2[12];
	short break_weapon_rate,break_armor_rate;
	short add_steal_rate;
	//�V�J�[�h�p
	//short revautospell_id,revautospell_lv,revautospell_rate,revautospell_flag;//�����pAS
	int	critical_damage;
	short hp_recov_stop;
	short sp_recov_stop;
	int addreveff[10];
	int addreveff_flag;
	int	critical_race[10];
	int	critical_race_rate[10];
	int subsize[3],magic_subsize[3];
	int	exp_rate[10],job_rate[10];
	short hp_drain_rate_race[10],sp_drain_rate_race[10];
	short hp_drain_value_race[10],sp_drain_value_race[10];
	short addgroup[MAX_MOBGROUP];
	short addgroup_[MAX_MOBGROUP];
	short arrow_addgroup[MAX_MOBGROUP];
	short subgroup[MAX_MOBGROUP];
	int hp_penalty_time;
	int sp_penalty_time;
	int hp_penalty_tick;
	int sp_penalty_tick;
	short hp_penalty_value;
	short sp_penalty_value;
	//������������HP/SP�y�i���e�B
	short hp_penalty_unrig_value[11];
	short sp_penalty_unrig_value[11];
	short hp_rate_penalty_unrig[11];
	short sp_rate_penalty_unrig[11];
	short mob_class_change_rate;//mob��ω�������m��
	short curse_by_muramasa;
	//
	short loss_equip_rate_when_die[11];
	short loss_equip_rate_when_attack[11];
	short loss_equip_rate_when_hit[11];
	short break_myequip_rate_when_attack[11];
	short break_myequip_rate_when_hit[11];
	short loss_equip_flag;
	
	struct {
		short hp_per;
		short sp_per;
		short rate;
		short flag;
	}autoraise;
	
	struct {
		short id[MAX_SKILL_DAMAGE_UP];
		short rate[MAX_SKILL_DAMAGE_UP];
		short count;	
	}skill_dmgup;
	
	struct {
		short id[MAX_SKILL_BLOW];
		short grid[MAX_SKILL_BLOW];
		short count;	
	}skill_blow;
	
	struct {
		short lv[MAX_BONUS_AUTOSPELL];
		short id[MAX_BONUS_AUTOSPELL];
		short rate[MAX_BONUS_AUTOSPELL];
		short flag[MAX_BONUS_AUTOSPELL];
		short card_id[MAX_BONUS_AUTOSPELL];
		short count;
		//�������ǉ�
		short add_id[MAX_ADDRATE_AUTOSPELL];
		short add_rate[MAX_ADDRATE_AUTOSPELL];
		short add_count;
		//�֎~
		short ban[MAX_BAN_AUTOSPELL];
		short ban_count;
	}autospell;

	short hp_vanish_rate;
	short hp_vanish_per;
	short sp_vanish_rate;
	short sp_vanish_per;

	short short_weapon_damege_rate,long_weapon_damege_rate;

	short itemheal_rate[MAX_ITEMGROUP];
	short use_itemid;
	int   bonus_damage;//�K���_���[�W
	short spiritball, spiritball_old;
	int spirit_timer[MAX_SKILL_LEVEL];
	
	int die_counter;
	short repeal_die_counter;
	short doridori_counter;
	
	int reg_num;
	struct script_reg *reg;
	int regstr_num;
	struct script_regstr *regstr;

	struct status_change sc_data[MAX_STATUSCHANGE];
	short sc_count;
	struct square dev;

	int trade_partner;
	int deal_item_index[10];
	int deal_item_amount[10];
	int deal_zeny;
	short deal_locked;
	short deal_mode;

	int party_sended,party_invite,party_invite_account;
	int party_hp,party_x,party_y;

	int guild_sended,guild_invite,guild_invite_account;
	int guild_emblem_id,guild_alliance,guild_alliance_account;

	int friend_invite, friend_invite_char;
	char friend_invite_name[24];

	int vender_id;
	int vend_num;
	char message[80];
	struct vending vending[12];

	int catch_target_class;
	struct s_pet pet;
	struct pet_db *petDB;
	struct pet_data *pd;
	int pet_hungry_timer;
	
	//�M���h�X�L���v�Z�p 0:�e���O 0>�e����
	//�ړ����̔���Ɏg�p
	short under_the_influence_of_the_guild_skill;

	//�����p
	struct{
		int  m;//m�̓}�b�v�̒ǉ��╪�U�Ȃǂŕω�(?)�������Ȃ̂�
		char name[24];//�ۑ�����Ȃ�}�b�v����
	}feel_map[3];
	int hate_mob[3];
	
	int tk_nhealhp,tk_nhealsp;//���炩�ȋx��,�y�����x��
	int inchealresthptick,inchealrestsptick;
	short tk_doridori_counter_hp;
	short tk_doridori_counter_sp;
	
	int ranking_point[MAX_RANKING];
	short am_pharmacy_success;
	short tk_capture_count;//�ߊl��
	int tk_rank_target_class;//�e�R��
	short ranker_weapon_bonus;
	short ranker_weapon_bonus_;

	int pvp_point,pvp_rank,pvp_timer,pvp_lastusers;

	struct map_session_data *repair_target;

	char eventqueue[MAX_EVENTQUEUE][50];
	int eventtimer[MAX_EVENTTIMER];
};

struct npc_timerevent_list {
	int timer,pos;
};
struct npc_label_list {
	char name[24];
	int pos;
};
struct npc_item_list {
	int nameid,value;
};
struct npc_data {
	struct block_list bl;
	short n;
	short class,dir;
	short speed;
	char name[24];
	char exname[24];
	int chat_id;
	short opt1,opt2,opt3,option;
	short flag;
	union {
		struct {
			char *script;
			short xs,ys;
			int guild_id;
			int timer,timerid,timeramount,nexttimer;
			unsigned int timertick;
			struct npc_timerevent_list *timer_event;
			int label_list_num;
			struct npc_label_list *label_list;
			int src_id;
		} scr;
		struct npc_item_list shop_item[1];
		struct {
			short xs,ys;
			short x,y;
			char name[16];
		} warp;
	} u;
	// �����Ƀ����o��ǉ����Ă͂Ȃ�Ȃ�(shop_item���ϒ��̈�)
};
struct mob_data {
	struct block_list bl;
	short n;
	short base_class,class,dir,mode;
	short m,x0,y0,xs,ys;
	char name[24];
	int spawndelay1,spawndelay2;
	struct {
		unsigned state : 8 ;
		unsigned skillstate : 8 ;
		unsigned targettype : 1 ;
		unsigned steal_flag : 1 ;
		unsigned steal_coin_flag : 1 ;
		unsigned skillcastcancel : 1 ;
		unsigned master_check : 1 ;
		unsigned change_walk_target : 1 ;
		unsigned walk_easy : 1 ;
		unsigned special_mob_ai : 3 ;
		unsigned nodrop : 1;
		unsigned noexp : 1;
		unsigned nomvp : 1;
	} state;
	int timer;
	short to_x,to_y;
	short speed;
	int hp;
	int target_id,attacked_id;
	short target_lv;
	struct walkpath_data walkpath;
	unsigned int next_walktime;
	unsigned int attackabletime;
	unsigned int last_deadtime,last_spawntime,last_thinktime;
	unsigned int canmove_tick;
	short move_fail_count;
	struct {
		int id;
		int dmg;
	} dmglog[DAMAGELOG_SIZE];
	struct item *lootitem;
	short lootitem_count;

	struct status_change sc_data[MAX_STATUSCHANGE];
	short sc_count;
	short opt1,opt2,opt3,option;
	short min_chase;

	int guild_id;

	int deletetimer;
	
	int skilltimer;
	int skilltarget;
	short skillx,skilly;
	short skillid,skilllv,skillidx;
	unsigned int skilldelay[MAX_MOBSKILL];
	int def_ele;
	int master_id,master_dist;
	struct skill_timerskill skilltimerskill[MAX_MOBSKILLTIMERSKILL];
	struct skill_unit_group skillunit[MAX_MOBSKILLUNITGROUP];
	struct skill_unit_group_tickset skillunittick[MAX_SKILLUNITGROUPTICKSET];
	char npc_event[50];
	short recall_flag;
	int recallmob_count;
	short recallcount;
	short guardup_lv;
};
struct pet_data {
	struct block_list bl;
	short class,dir;
	short speed;
	char name[24];
	struct {
		unsigned state : 8 ;
		unsigned skillstate : 8 ;
		unsigned change_walk_target : 1 ;
	} state;
	int timer;
	short to_x,to_y;
	short equip;
	struct walkpath_data walkpath;
	int target_id;
	short target_lv;
	int move_fail_count;
	unsigned int attackabletime,next_walktime,last_thinktime;
	struct item *lootitem;
	short lootitem_count;
	short lootitem_weight;
	int lootitem_timer;
	struct map_session_data *msd;
};

enum { MS_IDLE,MS_WALK,MS_ATTACK,MS_DEAD,MS_DELAY };

enum { NONE_ATTACKABLE,ATTACKABLE };

enum { ATK_LUCKY=1,ATK_FLEE,ATK_DEF};	// �͂܂�y�i���e�B�v�Z�p

// �����R�[�h
enum {
	EQP_WEAPON		= 0x0002,		// �E��
	EQP_ARMOR		= 0x0010,		// ��
	EQP_SHIELD		= 0x0020,		// ����
	EQP_HELM		= 0x0100,		// ����i
};

struct map_data {
	char name[24];
	unsigned char *gat;	// NULL�Ȃ牺��map_data_other_server�Ƃ��Ĉ���
	struct block_list **block;
	struct block_list **block_mob;
	int *block_count,*block_mob_count;
	int m;
	short xs,ys;
	short bxs,bys;
	int npc_num;
	int users;
	int base_exp_rate,job_exp_rate;
	struct {
		unsigned nomemo : 1;
		unsigned noteleport : 1;
		unsigned noportal : 1;
		unsigned noreturn : 1;
		unsigned monster_noteleport : 1;
		unsigned nosave : 1;
		unsigned nobranch : 1;
		unsigned nopenalty : 1;
		unsigned pvp : 1;
		unsigned pvp_noparty : 1;
		unsigned pvp_noguild : 1;
		unsigned pvp_nightmaredrop :1;
		unsigned pvp_nocalcrank : 1;
		unsigned gvg : 1;
		unsigned gvg_noparty : 1;
		unsigned nozenypenalty : 1;
		unsigned notrade : 1;
		unsigned noskill : 1;
		unsigned noabra : 1;
		unsigned nodrop : 1;
		unsigned snow : 1;
		unsigned fog : 1;
		unsigned sakura : 1;
		unsigned leaves : 1;
		unsigned rain : 1;
		unsigned fireworks : 1;
		unsigned cloud1 : 1;
		unsigned cloud2 : 1;
		unsigned cloud3 : 1;
	} flag;
	struct point save;
	struct npc_data *npc[MAX_NPC_PER_MAP];
	struct {
		int drop_id;
		int drop_type;
		int drop_per;
	} drop_list[MAX_DROP_PER_MAP];
};
struct map_data_other_server {
	char name[24];
	unsigned char *gat;	// NULL�Œ�ɂ��Ĕ��f
	unsigned long ip;
	unsigned int port;
	// ��x��map �T�[�o�[�̒S���ɂȂ��āA
	// ������x�����̒S���ɂȂ�ꍇ������̂őҔ������Ă���
	struct map_data* map;
};

struct flooritem_data {
	struct block_list bl;
	short subx,suby;
	int cleartimer;
	int first_get_id,second_get_id,third_get_id;
	unsigned int first_get_tick,second_get_tick,third_get_tick;
	struct item item_data;
};

enum {
	SP_SPEED,SP_BASEEXP,SP_JOBEXP,SP_KARMA,SP_MANNER,SP_HP,SP_MAXHP,SP_SP,	// 0-7
	SP_MAXSP,SP_STATUSPOINT,SP_0a,SP_BASELEVEL,SP_SKILLPOINT,SP_STR,SP_AGI,SP_VIT,	// 8-15
	SP_INT,SP_DEX,SP_LUK,SP_CLASS,SP_ZENY,SP_SEX,SP_NEXTBASEEXP,SP_NEXTJOBEXP,	// 16-23
	SP_WEIGHT,SP_MAXWEIGHT,SP_1a,SP_1b,SP_1c,SP_1d,SP_1e,SP_1f,	// 24-31
	SP_USTR,SP_UAGI,SP_UVIT,SP_UINT,SP_UDEX,SP_ULUK,SP_26,SP_27,	// 32-39
	SP_28,SP_ATK1,SP_ATK2,SP_MATK1,SP_MATK2,SP_DEF1,SP_DEF2,SP_MDEF1,	// 40-47
	SP_MDEF2,SP_HIT,SP_FLEE1,SP_FLEE2,SP_CRITICAL,SP_ASPD,SP_36,SP_JOBLEVEL,	// 48-55
	SP_UPPER,SP_PARTNER,SP_CART,	//56-
	SP_CARTINFO=99,	// 99

	// original 1000-
	SP_ATTACKRANGE=1000,	SP_ATKELE,SP_DEFELE,	// 1000-1002
	SP_CASTRATE, SP_MAXHPRATE, SP_MAXSPRATE, SP_SPRATE, // 1003-1006
	SP_ADDELE, SP_ADDRACE, SP_ADDSIZE, SP_SUBELE, SP_SUBRACE, // 1007-1011
	SP_ADDEFF, SP_RESEFF,	// 1012-1013
	SP_BASE_ATK,SP_ASPD_RATE,SP_HP_RECOV_RATE,SP_SP_RECOV_RATE,SP_SPEED_RATE, // 1014-1018
	SP_CRITICAL_DEF,SP_NEAR_ATK_DEF,SP_LONG_ATK_DEF, // 1019-1021
	SP_DOUBLE_RATE, SP_DOUBLE_ADD_RATE, SP_MATK, SP_MATK_RATE, // 1022-1025
	SP_IGNORE_DEF_ELE,SP_IGNORE_DEF_RACE, // 1026-1027
	SP_ATK_RATE,SP_SPEED_ADDRATE,SP_ASPD_ADDRATE, // 1028-1030
	SP_MAGIC_ATK_DEF,SP_MISC_ATK_DEF, // 1031-1032
	SP_IGNORE_MDEF_ELE,SP_IGNORE_MDEF_RACE, // 1033-1034
	SP_MAGIC_ADDELE,SP_MAGIC_ADDRACE,SP_MAGIC_SUBRACE, // 1035-1037
	SP_PERFECT_HIT_RATE,SP_PERFECT_HIT_ADD_RATE,SP_CRITICAL_RATE,SP_GET_ZENY_NUM,SP_ADD_GET_ZENY_NUM, // 1038-1042
	SP_ADD_DAMAGE_CLASS,SP_ADD_MAGIC_DAMAGE_CLASS,SP_ADD_DEF_CLASS,SP_ADD_MDEF_CLASS, // 1043-1046
	SP_ADD_MONSTER_DROP_ITEM,SP_DEF_RATIO_ATK_ELE,SP_DEF_RATIO_ATK_RACE,SP_ADD_SPEED, // 1047-1050
	SP_HIT_RATE,SP_FLEE_RATE,SP_FLEE2_RATE,SP_DEF_RATE,SP_DEF2_RATE,SP_MDEF_RATE,SP_MDEF2_RATE, // 1051-1057
	SP_SPLASH_RANGE,SP_SPLASH_ADD_RANGE,SP_AUTOSPELL,SP_HP_DRAIN_RATE,SP_SP_DRAIN_RATE, // 1058-1062
	SP_SHORT_WEAPON_DAMAGE_RETURN,SP_LONG_WEAPON_DAMAGE_RETURN,SP_WEAPON_COMA_ELE,SP_WEAPON_COMA_RACE, // 1063-1066
	SP_ADDEFF2,SP_BREAK_WEAPON_RATE,SP_BREAK_ARMOR_RATE,SP_ADD_STEAL_RATE, // 1067-1070
	SP_HP_DRAIN_VALUE,SP_SP_DRAIN_VALUE,SP_WEAPON_ATK,SP_WEAPON_ATK_RATE,SP_AUTOSPELL2, // 1071-1075
	SP_AUTOSELFSPELL,SP_AUTOSELFSPELL2,SP_ADDREVEFF,SP_REVAUTOSPELL,SP_REVAUTOSPELL2,//1076-1080
	SP_REVAUTOSELFSPELL,SP_REVAUTOSELFSPELL2,SP_CRITICAL_DAMAGE,SP_HP_RECOV_STOP,SP_SP_RECOV_STOP,// 1081-1085
	SP_CRITICALRACE,SP_CRITICALRACERATE,SP_SUB_SIZE,SP_MAGIC_SUB_SIZE,SP_EXP_RATE,//1086-1090
	SP_JOB_RATE,SP_DEF_HP_DRAIN_VALUE,SP_DEF_SP_DRAIN_VALUE,SP_ADD_SKILL_DAMAGE_RATE,SP_ADD_GROUP, // 1091-1095
	SP_SUB_GROUP,SP_HP_PENALTY_TIME,SP_SP_PENALTY_TIME,SP_HP_PENALTY_UNRIG,SP_SP_PENALTY_UNRIG,	   // 1096-1100
	SP_AUTO_SPELL_RATE,SP_BAN_AUTO_SPELLL,SP_ADD_SKILL_BLOW,SP_MOB_CLASS_CHANGE,SP_ADD_ITEMHEAL_RATE_GROUP, //1101-1105
	SP_HPVANISH,SP_SPVANISH,SP_BONUS_DAMAGE,SP_LOSS_EQUIP_WHEN_DIE,SP_RAISE,//1106-1110
	SP_CURSE_BY_MURAMASA,SP_LOSS_EQUIP_WHEN_ATTACK,SP_LOSS_EQUIP_WHEN_HIT,SP_BREAK_MYEQUIP_WHEN_ATTACK,SP_BREAK_MYEQUIP_WHEN_HIT, //1111-1115
	SP_HP_RATE_PENALTY_UNRIG,SP_SP_RATE_PENALTY_UNRIG,SP_MAGIC_DAMAGE_RETURN,SP_ADD_SHORT_WEAPON_DAMAGE,SP_ADD_LONG_WEAPON_DAMAGE, //1116-1120
	SP_WEAPON_COMA_ELE2,SP_WEAPON_COMA_RACE2, //1121-1122
	
	SP_RESTART_FULL_RECORVER=2000,SP_NO_CASTCANCEL,SP_NO_SIZEFIX,SP_NO_MAGIC_DAMAGE,SP_NO_WEAPON_DAMAGE,SP_NO_GEMSTONE, // 2000-2005
	SP_NO_CASTCANCEL2,SP_INFINITE_ENDURE,SP_UNBREAKABLE_WEAPON,SP_UNBREAKABLE_ARMOR, // 2006-2009
	SP_UNBREAKABLE_HELM, SP_UNBREAKABLE_SHIELD	// 2010-2011
};

enum {
	LOOK_BASE,LOOK_HAIR,LOOK_WEAPON,LOOK_HEAD_BOTTOM,LOOK_HEAD_TOP,LOOK_HEAD_MID,LOOK_HAIR_COLOR,LOOK_CLOTHES_COLOR,LOOK_SHIELD,LOOK_SHOES
};

// CELL
#define CELL_MASK		0x0f
#define CELL_NPC		0x80	// NPC�Z��
#define CELL_BASILICA	0x40	// BASILICA�Z��
/*
 * map_getcell()�Ŏg�p�����t���O
 */
typedef enum { 
	CELL_CHKWALL=0,		// ��(�Z���^�C�v1)
	CELL_CHKWATER,		// ����(�Z���^�C�v3)
	CELL_CHKGROUND,		// �n�ʏ�Q��(�Z���^�C�v5)
	CELL_CHKPASS,		// �ʉ߉\(�Z���^�C�v1,5�ȊO)
	CELL_CHKNOPASS,		// �ʉߕs��(�Z���^�C�v1,5)
	CELL_GETTYPE,		// �Z���^�C�v��Ԃ�
	CELL_CHKNPC=0x10,	// �^�b�`�^�C�v��NPC(�Z���^�C�v0x80�t���O)
	CELL_CHKBASILICA,	// �o�W���J(�Z���^�C�v0x40�t���O)
} cell_t;
// map_setcell()�Ŏg�p�����t���O
enum {
	CELL_SETNPC=0x10,	// �^�b�`�^�C�v��NPC���Z�b�g
	CELL_SETBASILICA,	// �o�W���J���Z�b�g
	CELL_CLRBASILICA,	// �o�W���J���N���A
};

struct chat_data {
	struct block_list bl;

	unsigned char pass[8];   /* password */
	unsigned char title[61]; /* room title MAX 60 */
	unsigned char limit;     /* join limit */
	unsigned char trigger;
	unsigned char users;     /* current users */
	unsigned char pub;       /* room attribute */
	struct map_session_data *usersd[20];
	struct block_list *owner_;
	struct block_list **owner;
	char npc_event[50];
};

extern struct map_data map[];
extern int map_num;
extern int autosave_interval;
extern int agit_flag;

// gat�֘A
int map_getcell(int,int,int,cell_t);
int map_getcellp(struct map_data*,int,int,cell_t);
void map_setcell(int,int,int,int);
extern int map_read_flag; // 0: grf�t�@�C�� 1: �L���b�V�� 2: �L���b�V��(���k)

extern char motd_txt[];
extern char help_txt[];

// �I�S�̏��
void map_setusers(int);
int map_getusers(void);
// block�폜�֘A
int map_freeblock( void *bl );
int map_freeblock_lock(void);
int map_freeblock_unlock(void);
// block�֘A
int map_addblock(struct block_list *);
int map_delblock(struct block_list *);
void map_foreachinarea(int (*)(struct block_list*,va_list),int,int,int,int,int,int,...);
void map_foreachinpath(int (*func)(struct block_list*,va_list),int m,int x0,int y0,int x1,int y1,int type,...);
void map_foreachinmovearea(int (*)(struct block_list*,va_list),int,int,int,int,int,int,int,int,...);
int map_countnearpc(int,int,int);
//block�֘A�ɒǉ�
int map_count_oncell(int m,int x,int y);
struct skill_unit *map_find_skill_unit_oncell(struct block_list *,int x,int y,int skill_id,struct skill_unit *);
// �ꎞ�Iobject�֘A
int map_addobject(struct block_list *);
int map_delobject(int);
int map_delobjectnofree(int id);
void map_foreachobject(int (*)(struct block_list*,va_list),int,...);
//
int map_quit(struct map_session_data *);
// npc
int map_addnpc(int,struct npc_data *);


// ���A�C�e���֘A
int map_clearflooritem_timer(int,unsigned int,int,int);
#define map_clearflooritem(id) map_clearflooritem_timer(0,0,id,1)
int map_addflooritem(struct item *,int,int,int,int,struct map_session_data *,struct map_session_data *,struct map_session_data *,int);

// �L����id�����L������ �ϊ��֘A
struct charid2nick *char_search(int char_id);
void map_addchariddb(int charid, char *name, int account_id, unsigned long ip, int port);
void map_delchariddb(int charid);
int map_reqchariddb(struct map_session_data * sd,int charid);
char * map_charid2nick(int);

struct map_session_data * map_id2sd(int);
struct block_list * map_id2bl(int);
int map_mapname2mapid(char*);
int map_mapname2ipport(char*,int*,int*);
int map_setipport(char *name,unsigned long ip,int port);
int map_eraseipport(char *name,unsigned long ip,int port);
int map_eraseallipport(void);
void map_addiddb(struct block_list *);
void map_deliddb(struct block_list *bl);
int map_foreachiddb(int (*)(void*,void*,va_list),...);
void map_addnickdb(struct map_session_data *);
struct map_session_data * map_nick2sd(char*);


// ���̑�
int map_check_dir(int s_dir,int t_dir);
int map_calc_dir( struct block_list *src,int x,int y);

// path.c���

int path_search_long(struct shootpath_data *,int,int,int,int,int);
int path_search(struct walkpath_data*,int,int,int,int,int,int);
int path_blownpos(int m,int x0,int y0,int dx,int dy,int count);

int map_who(int fd);


#endif
