#ifndef _SKILL_H_
#define _SKILL_H_

#include "map.h"
#include "mmo.h"

#define MAX_SKILL_DB			MAX_SKILL
#define MAX_HOMSKILL_DB			MAX_HOMSKILL
#define MAX_GUILDSKILL_DB		MAX_GUILDSKILL
#define MAX_SKILL_PRODUCE_DB	300
#define MAX_PRODUCE_RESOURCE	10
#define MAX_SKILL_ARROW_DB	 150
#define MAX_SKILL_ABRA_DB	 350

// �X�L���f�[�^�x�[�X
struct skill_db {
	int range[MAX_SKILL_LEVEL],hit,inf,pl,nk,max;
	int num[MAX_SKILL_LEVEL];
	int cast[MAX_SKILL_LEVEL],fixedcast[MAX_SKILL_LEVEL],delay[MAX_SKILL_LEVEL];
	int upkeep_time[MAX_SKILL_LEVEL],upkeep_time2[MAX_SKILL_LEVEL];
	int castcancel,cast_def_rate;
	int inf2,maxcount,skill_type;
	int blewcount[MAX_SKILL_LEVEL];
	int hp[MAX_SKILL_LEVEL],sp[MAX_SKILL_LEVEL],hp_rate[MAX_SKILL_LEVEL],sp_rate[MAX_SKILL_LEVEL],zeny[MAX_SKILL_LEVEL];
	int weapon,state,spiritball[MAX_SKILL_LEVEL],coin[MAX_SKILL_LEVEL],arrow_cost[MAX_SKILL_LEVEL],arrow_type;
	int itemid[10],amount[10];
	int unit_id[4];
	int unit_layout_type[MAX_SKILL_LEVEL];
	int unit_range;
	int unit_interval;
	int unit_target;
	int unit_flag;
	int cloneable;
	int misfire;
	int zone;
	int damage_rate[4];
};

#define MAX_SKILL_UNIT_LAYOUT	50
#define MAX_SQUARE_LAYOUT		5	// 11*11�̃��j�b�g�z�u���ő�
#define MAX_SKILL_UNIT_COUNT ((MAX_SQUARE_LAYOUT*2+1)*(MAX_SQUARE_LAYOUT*2+1))
struct skill_unit_layout {
	int count;
	int dx[MAX_SKILL_UNIT_COUNT];
	int dy[MAX_SKILL_UNIT_COUNT];
};

enum {
	UF_DEFNOTENEMY		= 0x0001,	// defnotenemy �ݒ��BCT_NOENEMY�ɐ؂�ւ�
	UF_NOREITERATION	= 0x0002,	// �d���u���֎~
	UF_NOFOOTSET		= 0x0004,	// �����u���֎~
	UF_NOOVERLAP		= 0x0008,	// ���j�b�g���ʂ��d�����Ȃ�
	UF_PATHCHECK		= 0x0010,	// �I�u�W�F�N�g�������Ɏː��`�F�b�N
	UF_DANCE			= 0x0100,	// �_���X�X�L��
	UF_ENSEMBLE			= 0x0200,	// ���t�X�L��
};

extern struct skill_db skill_db[MAX_SKILL_DB+MAX_HOMSKILL_DB+MAX_GUILDSKILL_DB];

// �A�C�e���쐬�f�[�^�x�[�X
struct skill_produce_db {
	int nameid, trigger;
	int req_skill,req_skilllv,itemlv;
	int mat_id[MAX_PRODUCE_RESOURCE],mat_amount[MAX_PRODUCE_RESOURCE];
};
extern struct skill_produce_db skill_produce_db[MAX_SKILL_PRODUCE_DB];

// ��쐬�f�[�^�x�[�X
struct skill_arrow_db {
	int nameid, trigger;
	int cre_id[5],cre_amount[5];
};
extern struct skill_arrow_db skill_arrow_db[MAX_SKILL_ARROW_DB];

// �A�u���J�_�u���f�[�^�x�[�X
struct skill_abra_db {
	int nameid;
	int req_lv;
	int per;
};
extern struct skill_abra_db skill_abra_db[MAX_SKILL_ABRA_DB];

struct block_list;
struct map_session_data;
struct skill_unit;
struct skill_unit_group;

int do_init_skill(void);

//
int GetSkillStatusChangeTable(int id);
//�}�N������֐���
//�M���hID���g�p�\��
int skill_get_skilldb_id(int id);
int skill_get_hit(int id);
int skill_get_inf(int id);
int skill_get_pl(int id);
int skill_get_nk(int id);
int skill_get_max(int id);
int skill_get_range(int id,int lv);
int skill_get_hp(int id,int lv);
int skill_get_sp(int id,int lv);
int skill_get_zeny(int id,int lv);
int skill_get_num(int id,int lv);
int skill_get_cast(int id,int lv);
int skill_get_fixedcast(int id ,int lv);
int skill_get_delay(int id,int lv);
int skill_get_time(int id ,int lv);
int skill_get_time2(int id,int lv);
int skill_get_castdef(int id);
int skill_get_weapontype(int id);
int skill_get_inf2(int id);
int skill_get_maxcount(int id);
int skill_get_blewcount(int id,int lv);
int skill_get_unit_id(int id,int flag);
int skill_get_unit_layout_type(int id,int lv);
int skill_get_unit_interval(int id);
int skill_get_unit_range(int id);
int skill_get_unit_target(int id);
int skill_get_unit_flag(int id);
int skill_get_arrow_cost(int id,int lv);
int skill_get_arrow_type(int id);
int skill_get_cloneable(int id);
int skill_get_misfire(int id);
int skill_get_zone(int id);
int skill_get_damage_rate(int id,int type);

// �X�L���̎g�p
void skill_castend_map(struct map_session_data *sd, int skill_num, const char *map);

int skill_cleartimerskill(struct block_list *src);
int skill_addtimerskill(struct block_list *src,unsigned int tick,int target,int x,int y,int skill_id,int skill_lv,int type,int flag);

// �ǉ�����
int skill_additional_effect( struct block_list* src, struct block_list *bl,int skillid,int skilllv,int attack_type,unsigned int tick);

int skill_delunit_by_ganbatein(struct block_list *bl, va_list ap );

enum {	//������΂��t���O
	SAB_NOMALBLOW   = 0x00000,
	SAB_REVERSEBLOW = 0x10000,
	SAB_NODAMAGE    = 0x20000,
	SAB_NOPATHSTOP  = 0x40000,
};

#define	SAB_NORMAL	0x00010000
#define	SAB_SKIDTRAP	0x00020000
int skill_add_blown( struct block_list *src, struct block_list *target,int skillid,int flag);

//�J�[�h���ʂ̃I�[�g�X�y��
#define AS_ATTACK	0x00050003
#define AS_REVENGE	0x00060003
int skill_use_bonus_autospell(struct block_list * src,struct block_list * bl,int skill_id,int skill_lv,int rate,long skill_flag,int tick,int flag);
int skill_bonus_autospell(struct block_list * src,struct block_list * bl,long mode,int tick,int flag);

// ���j�b�g�X�L��
struct skill_unit *skill_initunit(struct skill_unit_group *group,int idx,int x,int y);
int skill_delunit(struct skill_unit *unit);
struct skill_unit_group *skill_initunitgroup(struct block_list *src,
	int count,int skillid,int skilllv,int unit_id);
int skill_delunitgroup(struct skill_unit_group *group);
int skill_clear_unitgroup(struct block_list *src);

int skill_unit_ondamaged(struct skill_unit *src,struct block_list *bl,
	int damage,unsigned int tick);

int skill_castfix( struct block_list *bl, int time );
int skill_delayfix( struct block_list *bl, int time, int cast );
int skill_check_unit_range(int m,int x,int y,int skillid, int skilllv);
int skill_check_unit_range2(int m,int x,int y,int skillid, int skilllv);
int skill_unit_move(struct block_list *bl,unsigned int tick,int flag);
int skill_unit_move_unit_group( struct skill_unit_group *group, int m,int dx,int dy);


int skill_hermode_wp_check(struct block_list *bl,int range);
int skill_hermode_wp_check_sub(struct block_list *bl, va_list ap );

struct skill_unit_group *skill_check_dancing( struct block_list *src );
void skill_stop_dancing(struct block_list *src, int flag);
void skill_stop_gravitation(struct block_list *src);

// �r���L�����Z��
int skill_castcancel(struct block_list *bl,int type);

int skill_gangsterparadise(struct map_session_data *sd ,int type);
void skill_brandishspear_first(struct square *tc,int dir,int x,int y);
void skill_brandishspear_dir(struct square *tc,int dir,int are);
void skill_autospell(struct map_session_data *sd, int skillid);
void skill_devotion(struct map_session_data *md,int target);
void skill_devotion2(struct block_list *bl,int crusader);
int skill_devotion3(struct block_list *bl,int target);
void skill_devotion_end(struct map_session_data *md,struct map_session_data *sd,int target);
int skill_marionette(struct block_list *bl,int target);
void skill_marionette2(struct block_list *bl,int src);
int skill_tarot_card_of_fate(struct block_list *src,struct block_list *target,int skillid,int skilllv,int tick,int flag,int wheel);

#define skill_calc_heal(bl,skill_lv) (( status_get_lv(bl)+status_get_int(bl) )/8 *(4+ skill_lv*8))
int skill_castend_id( int tid, unsigned int tick, int id,int data );
int skill_castend_pos( int tid, unsigned int tick, int id,int data );

// ���̑�
int skill_check_cloaking(struct block_list *bl);

// �X�L���g�p���ǂ����̔���B

// ����֐��ɓn���\���́B�֐������Ńf�[�^���㏑�������̂ŁA
// �߂�����ɕύX����̂�Y��Ȃ��悤�ɁB
struct skill_condition {
	int id;
	int lv;
	int x;
	int y;
	int target;
};

int skill_check_condition(struct block_list *bl, int type);
int skill_check_condition2(struct block_list *bl, struct skill_condition *sc, int type);

// �A�C�e���쐬
int skill_can_produce_mix( struct map_session_data *sd, int nameid, int trigger );
void skill_produce_mix(struct map_session_data *sd, int nameid, int slot1, int slot2, int slot3);
int skill_am_twilight1(struct map_session_data* sd);
int skill_am_twilight2(struct map_session_data* sd);
int skill_am_twilight3(struct map_session_data* sd);

void skill_arrow_create(struct map_session_data *sd, int nameid);
int skill_can_repair( struct map_session_data *sd, int nameid );
int skill_repair_weapon(struct map_session_data *sd, int idx);

// mob�X�L���̂���
int skill_castend_nodamage_id( struct block_list *src, struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag );
int skill_castend_damage_id( struct block_list* src, struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag );
int skill_castend_pos2( struct block_list *src, int x,int y,int skillid,int skilllv,unsigned int tick,int flag);

int skill_cloneable(int skillid);
int skill_upperskill(int skillid);
int skill_mobskill(int skillid);
int skill_abraskill(int skillid);
int skill_clone(struct map_session_data* sd,int skillid,int skilllv);


void skill_weapon_refine(struct map_session_data *sd, int idx);
int skill_success_weaponrefine(struct map_session_data *sd,int idx);
int skill_fail_weaponrefine(struct map_session_data *sd,int idx);

// �X�L���U���ꊇ����
int skill_blown( struct block_list *src, struct block_list *target,int count);

int skill_castend_delay (struct block_list* src, struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag);

// �o�V���J������~
void skill_basilica_cancel( struct block_list *bl );

void skill_reload(void);

enum {
	ST_NONE,ST_HIDING,ST_CLOAKING,ST_HIDDEN,ST_RIDING,ST_FALCON,ST_CART,ST_SHIELD,ST_SIGHT,ST_EXPLOSIONSPIRITS,
	ST_RECOV_WEIGHT_RATE,ST_MOVE_ENABLE,ST_WATER,
};

enum {	// struct map_session_data �� status_change�̔ԍ��e�[�u��
// SC_SENDMAX�����̓N���C�A���g�ւ̒ʒm����B
// 2-2���E�̒l�͂Ȃ񂩂߂��Ⴍ������ۂ��̂Ŏb��B���Ԃ�ύX����܂��B
	//SC_SENDMAX				=128,

	SC_PROVOKE				= 0,	/* �v���{�b�N */
	SC_ENDURE				= 1,	/* �C���f���A */
	SC_TWOHANDQUICKEN		= 2,	/* �c�[�n���h�N�C�b�P�� */
	SC_CONCENTRATE			= 3,	/* �W���͌��� */
	SC_HIDING				= 4,	/* �n�C�f�B���O */
	SC_CLOAKING				= 5,	/* �N���[�L���O */
	SC_ENCPOISON			= 6,	/* �G���`�����g�|�C�Y�� */
	SC_POISONREACT			= 7,	/* �|�C�Y�����A�N�g */
	SC_QUAGMIRE				= 8,	/* �N�@�O�}�C�A */
	SC_ANGELUS				= 9,	/* �G���W�F���X */
	SC_BLESSING				=10,	/* �u���b�V���O */
	SC_SIGNUMCRUCIS			=11,	/* �V�O�i���N���V�X�H */
	SC_INCREASEAGI			=12,	/*  */
	SC_DECREASEAGI			=13,	/*  */
	SC_SLOWPOISON			=14,	/* �X���[�|�C�Y�� */
	SC_IMPOSITIO			=15,	/* �C���|�V�e�B�I�}�k�X */
	SC_SUFFRAGIUM			=16,	/* �T�t���M�E�� */
	SC_ASPERSIO				=17,	/* �A�X�y���V�I */
	SC_BENEDICTIO			=18,	/* ���̍~�� */
	SC_KYRIE				=19,	/* �L���G�G���C�\�� */
	SC_MAGNIFICAT			=20,	/* �}�O�j�t�B�J�[�g */
	SC_GLORIA				=21,	/* �O�����A */
	SC_AETERNA				=22,	/*  */
	SC_ADRENALINE			=23,	/* �A�h���i�������b�V�� */
	SC_WEAPONPERFECTION		=24,	/* �E�F�|���p�[�t�F�N�V���� */
	SC_OVERTHRUST			=25,	/* �I�[�o�[�g���X�g */
	SC_MAXIMIZEPOWER		=26,	/* �}�L�V�}�C�Y�p���[ */
	SC_RIDING				=27,	/* ���C�f�B���O */
	SC_FALCON				=28,	/* �t�@���R���}�X�^���[ */
	SC_TRICKDEAD			=29,	/* ���񂾂ӂ� */
	SC_LOUD					=30,	/* ���E�h�{�C�X */
	SC_ENERGYCOAT			=31,	/* �G�i�W�[�R�[�g */
	SC_PK_PENALTY			=32,	//PK�̃y�i���e�B
	SC_REVERSEORCISH		=33,    //���o�[�X�I�[�L�b�V��
	SC_HALLUCINATION		=34,	/* �n���l�[�V�����E�H�[�N�H */
	SC_WEIGHT50				=35,	/* �d��50���I�[�o�[ */
	SC_WEIGHT90				=36,	/* �d��90���I�[�o�[ */
	SC_SPEEDPOTION0			=37,	/* ���x�|�[�V�����H */
	SC_SPEEDPOTION1			=38,	/* �X�s�[�h�A�b�v�|�[�V�����H */
	SC_SPEEDPOTION2			=39,	/* �n�C�X�s�[�h�|�[�V�����H */
	SC_SPEEDPOTION3			=40,	/* �o�[�T�[�N�|�[�V���� */
	SC_ITEM_DELAY			=41,
	//
	//
	//
	//
	//
	//
	//
	//
	SC_STRIPWEAPON			=50,	/* �X�g���b�v�E�F�|�� */
	SC_STRIPSHIELD			=51,	/* �X�g���b�v�V�[���h */
	SC_STRIPARMOR			=52,	/* �X�g���b�v�A�[�}�[ */
	SC_STRIPHELM			=53,	/* �X�g���b�v�w���� */
	SC_CP_WEAPON			=54,	/* �P�~�J���E�F�|���`���[�W */
	SC_CP_SHIELD			=55,	/* �P�~�J���V�[���h�`���[�W */
	SC_CP_ARMOR				=56,	/* �P�~�J���A�[�}�[�`���[�W */
	SC_CP_HELM				=57,	/* �P�~�J���w�����`���[�W */
	SC_AUTOGUARD			=58,	/* �I�[�g�K�[�h */
	SC_REFLECTSHIELD		=59,	/* ���t���N�g�V�[���h */
	SC_DEVOTION				=60,	/* �f�B�{�[�V���� */
	SC_PROVIDENCE			=61,	/* �v�����B�f���X */
	SC_DEFENDER				=62,	/* �f�B�t�F���_�[ */
	SC_SANTA				=63,	//�T���^
	//
	SC_AUTOSPELL			=65,	/* �I�[�g�X�y�� */
	//
	//
	SC_SPEARSQUICKEN		=68,	/* �X�s�A�N�C�b�P�� */
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	SC_EXPLOSIONSPIRITS		=86,	/* �����g�� */
	SC_STEELBODY			=87,	/* ���� */
	//
	SC_COMBO				=89,
	SC_FLAMELAUNCHER		=90,	/* �t���C�������`���[ */
	SC_FROSTWEAPON			=91,	/* �t���X�g�E�F�|�� */
	SC_LIGHTNINGLOADER		=92,	/* ���C�g�j���O���[�_�[ */
	SC_SEISMICWEAPON		=93,	/* �T�C�Y�~�b�N�E�F�|�� */
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	SC_AURABLADE			=103,	/* �I�[���u���[�h */
	SC_PARRYING				=104,	/* �p���C���O */
	SC_CONCENTRATION		=105,	/* �R���Z���g���[�V���� */
	SC_TENSIONRELAX			=106,	/* �e���V���������b�N�X */
	SC_BERSERK				=107,	/* �o�[�T�[�N */
	//
	//
	//
	SC_ASSUMPTIO			=110,	/* �A�X���v�e�B�I */
	//
	//
	SC_MAGICPOWER			=113,	/* ���@�͑��� */
	SC_EDP					=114,	/* �G�t�F�N�g������������ړ� */
	SC_TRUESIGHT			=115,	/* �g�D���[�T�C�g */
	SC_WINDWALK				=116,	/* �E�C���h�E�H�[�N */
	SC_MELTDOWN				=117,	/* �����g�_�E�� */
	SC_CARTBOOST			=118,	/* �J�[�g�u�[�X�g */
	SC_CHASEWALK			=119,	/* �`�F�C�X�E�H�[�N */
	SC_REJECTSWORD			=120,	/* ���W�F�N�g�\�[�h */
	SC_MARIONETTE			=121,	/* �}���I�l�b�g�R���g���[�� */ //�����p
	SC_MARIONETTE2			=122,	/* �}���I�l�b�g�R���g���[�� */ //�^�[�Q�b�g�p
	//
	SC_HEADCRUSH			=124,	/* �w�b�h�N���b�V�� */
	SC_JOINTBEAT			=125,	/* �W���C���g�r�[�g */
	//
	//
	SC_STONE				=128,	/* ��Ԉُ�F�Ή� */
	SC_FREEZE				=129,	/* ��Ԉُ�F�X�� */
	SC_STAN					=130,	/* ��Ԉُ�F�X�^�� */
	SC_SLEEP				=131,	/* ��Ԉُ�F���� */
	SC_POISON				=132,	/* ��Ԉُ�F�� */
	SC_CURSE				=133,	/* ��Ԉُ�F�� */
	SC_SILENCE				=134,	/* ��Ԉُ�F���� */
	SC_CONFUSION			=135,	/* ��Ԉُ�F���� */
	SC_BLIND				=136,	/* ��Ԉُ�F�È� */
	SC_BLEED				=137,	/* ��Ԉُ�F�o�� */
	SC_DIVINA				= SC_SILENCE,	/* ��Ԉُ�F���� */
	//138
	//139
	SC_SAFETYWALL			=140,	/* �Z�[�t�e�B�[�E�H�[�� */
	SC_PNEUMA				=141,	/* �j���[�} */
	//
	SC_ANKLE				=143,	/* �A���N���X�l�A */
	SC_DANCING				=144,	/*  */
	SC_KEEPING				=145,	/*  */
	SC_BARRIER				=146,	/*  */
	//
	//
	SC_MAGICROD				=149,	/*  */
	SC_SIGHT				=150,	/*  */
	SC_RUWACH				=151,	/*  */
	SC_AUTOCOUNTER			=152,	/*  */
	SC_VOLCANO				=153,	/*  */
	SC_DELUGE				=154,	/*  */
	SC_VIOLENTGALE			=155,	/*  */
	SC_BLADESTOP_WAIT		=156,	/*  */
	SC_BLADESTOP			=157,	/*  */
	SC_EXTREMITYFIST		=158,	/*  */
	SC_GRAFFITI				=159,	/*  */
	SC_LULLABY				=160,	/*  */
	SC_RICHMANKIM			=161,	/*  */
	SC_ETERNALCHAOS			=162,	/*  */
	SC_DRUMBATTLE			=163,	/*  */
	SC_NIBELUNGEN			=164,	/*  */
	SC_ROKISWEIL			=165,	/*  */
	SC_INTOABYSS			=166,	/*  */
	SC_SIEGFRIED			=167,	/*  */
	SC_DISSONANCE			=168,	/*  */
	SC_WHISTLE				=169,	/*  */
	SC_ASSNCROS				=170,	/*  */
	SC_POEMBRAGI			=171,	/*  */
	SC_APPLEIDUN			=172,	/*  */
	SC_UGLYDANCE			=173,	/*  */
	SC_HUMMING				=174,	/*  */
	SC_DONTFORGETME			=175,	/*  */
	SC_FORTUNE				=176,	/*  */
	SC_SERVICE4U			=177,	/*  */
	SC_BASILICA				=178,	/*  */
	SC_MINDBREAKER			=179,	/*  */
	SC_SPIDERWEB			=180,	/* �X�p�C�_�[�E�F�b�u */
	SC_MEMORIZE				=181,	/* �������C�Y */
	SC_DPOISON				=182,	/* �ғ� */
	//
	SC_SACRIFICE			=184,	/* �T�N���t�@�C�X */
	SC_INCATK				=185,	//item 682�p
	SC_INCMATK				=186,	//item 683�p
	SC_WEDDING				=187,	//�����p(�����ߏւɂȂ��ĕ����̂��x���Ƃ�)
	SC_NOCHAT				=188,	//�ԃG�����
	SC_SPLASHER				=189,	/* �x�i���X�v���b�V���[ */
	SC_SELFDESTRUCTION		=190,	/* ���� */
	SC_MAGNUM				=191,	/* �}�O�i���u���C�N */
	SC_GOSPEL				=192,	/* �S�X�y�� */
	SC_INCALLSTATUS			=193,	/* �S�ẴX�e�[�^�X���㏸(���̂Ƃ���S�X�y���p) */
	SC_INCHIT				=194,	/* HIT�㏸(���̂Ƃ���S�X�y���p) */
	SC_INCFLEE				=195,	/* FLEE�㏸(���̂Ƃ���S�X�y���p) */
	SC_INCMHP2				=196,	/* MHP��%�㏸(���̂Ƃ���S�X�y���p) */
	SC_INCMSP2				=197,	/* MSP��%�㏸(���̂Ƃ���S�X�y���p) */
	SC_INCATK2				=198,	/* ATK��%�㏸(���̂Ƃ���S�X�y���p) */
	SC_INCHIT2				=199,	/* HIT��%�㏸(���̂Ƃ���S�X�y���p) */
	SC_INCFLEE2				=200,	/* FLEE��%�㏸(���̂Ƃ���S�X�y���p) */
	SC_PRESERVE				=201,	/* �v���U�[�u */
	SC_OVERTHRUSTMAX		=202,	/* �I�[�o�[�g���X�g�}�b�N�X */
	SC_CHASEWALK_STR		=203,	/*STR�㏸�i�`�F�C�X�E�H�[�N�p�j*/
	SC_WHISTLE_				=204,
	SC_ASSNCROS_			=205,
	SC_POEMBRAGI_			=206,
	SC_APPLEIDUN_			=207,
	SC_HUMMING_				=209,
	SC_DONTFORGETME_		=210,
	SC_FORTUNE_				=211,
	SC_SERVICE4U_			=212,
	SC_BATTLEORDER			=213,//�M���h�X�L��
	SC_REGENERATION			=214,
	SC_BATTLEORDER_DELAY	=215,
	SC_REGENERATION_DELAY	=216,
	SC_RESTORE_DELAY		=217,
	SC_EMERGENCYCALL_DELAY	=218,
	SC_POISONPOTION			=219,
	SC_THE_MAGICIAN			=220,
	SC_STRENGTH				=221,
	SC_THE_DEVIL			=222,
	SC_THE_SUN				=223,
	SC_MEAL_INCSTR			=224,//�H���p
	SC_MEAL_INCAGI			=225,
	SC_MEAL_INCVIT			=226,
	SC_MEAL_INCINT			=227,
	SC_MEAL_INCDEX			=228,
	SC_MEAL_INCLUK			=229,
	SC_RUN 					= 230,
	SC_SPURT 				= 231,
	SC_TKCOMBO 				= 232,	//�e�R���̃R���{�p
	SC_DODGE				= 233,
	SC_HERMODE				= 234,
	SC_TRIPLEATTACK_RATE_UP	= 235,	//�O�i�������A�b�v
	SC_COUNTER_RATE_UP		= 236,	//�J�E���^�[�L�b�N�������A�b�v
	SC_SUN_WARM				= 237,
	SC_MOON_WARM			= 238,
	SC_STAR_WARM			= 239,
	SC_SUN_COMFORT			= 240,
	SC_MOON_COMFORT			= 241,
	SC_STAR_COMFORT			= 242,
	SC_FUSION				= 243,
	SC_ALCHEMIST			= 244,//��
	SC_MONK					= 245,
	SC_STAR					= 246,
	SC_SAGE					= 247,
	SC_CRUSADER				= 248,
	SC_SUPERNOVICE			= 249,
	SC_KNIGHT				= 250,
	SC_WIZARD				= 251,
	SC_PRIEST				= 252,
	SC_BARDDANCER			= 253,
	SC_ROGUE				= 254,
	SC_ASSASIN				= 255,
	SC_BLACKSMITH			= 256,
	SC_HUNTER				= 257,
	SC_SOULLINKER			= 258,
	SC_HIGH					= 259,
	//���̒ǉ����������炢���Ȃ��̂ŗ\��
	//�Ȃ���ΓK���ɂ��Ƃ��疄�߂�
	//= 260,	//�E�҂̍��̗\��H
	//= 261,	//�K���X�����K�[�̍��̗\��H
	//= 262,	//�H�H�H�̍��̗\��H
	SC_ADRENALINE2			= 263,
	SC_KAIZEL				= 264,
	SC_KAAHI				= 265,
	SC_KAUPE				= 266,
	SC_KAITE				= 267,
	SC_SMA					= 268,	//�G�X�}�r���\���ԗp
	SC_SWOO					= 269,
	SC_SKE					= 270,
	SC_SKA					= 271,
	SC_ONEHAND				= 272,
	SC_READYSTORM			= 273,
	SC_READYDOWN			= 274,
	SC_READYTURN			= 275,
	SC_READYCOUNTER			= 276,
	SC_DODGE_DELAY			= 277,
	SC_AUTOBERSERK			= 278,
	SC_DEVIL				= 279,
	SC_DOUBLECASTING 		= 280,//�_�u���L���X�e�B���O
	SC_ELEMENTFIELD			= 281,//������
	SC_DARKELEMENT			= 282,//��
	SC_ATTENELEMENT			= 283,//�O
	SC_MIRACLE				= 284,//���z�ƌ��Ɛ��̊��
	SC_ANGEL				= 285,//���z�ƌ��Ɛ��̓V�g
	SC_HIGHJUMP				= 286,//�n�C�W�����v
	SC_DOUBLE				= 287,//�_�u���X�g���C�t�B���O���
	SC_ACTION_DELAY			= 288,//
	SC_BABY					= 289,//�p�p�A�}�}�A��D��
	SC_LONGINGFREEDOM		= 290,
	SC_SHRINK				= 291,//#�V�������N#
	SC_CLOSECONFINE			= 292,//#�N���[�Y�R���t�@�C��#
	SC_SIGHTBLASTER			= 293,//#�T�C�g�u���X�^�[#
	SC_ELEMENTWATER			= 294,//#�G���������^���`�F���W��#
	//�H���p2
	SC_MEAL_INCHIT			= 295,
	SC_MEAL_INCFLEE			= 296,
	SC_MEAL_INCFLEE2		= 297,
	SC_MEAL_INCCRITICAL		= 298,
	SC_MEAL_INCDEF			= 299,
	SC_MEAL_INCMDEF			= 300,
	SC_MEAL_INCATK			= 301,
	SC_MEAL_INCMATK			= 302,
	SC_MEAL_INCEXP			= 303,
	SC_MEAL_INCJOB			= 304,
	//
	SC_ELEMENTGROUND		= 305,//�y(�Z)
	SC_ELEMENTFIRE			= 306,//��(�Z)
	SC_ELEMENTWIND			= 307,//��(�Z)
	SC_WINKCHARM			= 308,
	SC_ELEMENTPOISON		= 309,//��(�Z)
	SC_ELEMENTDARK			= 310,//��(�Z)
	SC_ELEMENTELEKINESIS	= 311,//�O(�Z)
	SC_ELEMENTUNDEAD		= 312,//�s��(�Z)
	SC_UNDEADELEMENT		= 313,//�s��(��)
	SC_ELEMENTHOLY			= 314,//��(�Z)
	SC_NPC_DEFENDER			= 315,
	SC_RESISTWATER			= 316,//�ϐ�
	SC_RESISTGROUND			= 317,//�ϐ�
	SC_RESISTFIRE			= 318,//�ϐ�
	SC_RESISTWIND			= 319,//�ϐ�
	SC_RESISTPOISON			= 320,//�ϐ�
	SC_RESISTHOLY			= 321,//�ϐ�
	SC_RESISTDARK			= 322,//�ϐ�
	SC_RESISTTELEKINESIS	= 323,//�ϐ�
	SC_RESISTUNDEAD			= 324,//�ϐ�
	SC_RESISTALL			= 325,//�ϐ�
	//�푰�ύX�H
	SC_RACEUNKNOWN			= 326,//���`
	SC_RACEUNDEAD			= 327,//�s���푰
	SC_RACEBEAST			= 328,
	SC_RACEPLANT			= 329,
	SC_RACEINSECT			= 330,
	SC_RACEFISH				= 332,
	SC_RACEDEVIL			= 333,
	SC_RACEHUMAN			= 334,
	SC_RACEANGEL			= 335,
	SC_RACEDRAGON			= 336,
	SC_TIGEREYE				= 337,
	SC_GRAVITATION_USER		= 338,
	SC_GRAVITATION			= 339,
	SC_FOGWALL				= 340,
	SC_FOGWALLPENALTY		= 341,
	SC_REDEMPTIO			= 342,
	SC_TAROTCARD			= 343,
	SC_HOLDWEB				= 344,
	SC_INVISIBLE			= 345,
	SC_DETECTING			= 346,
	//
	SC_FLING				= 347,
	SC_MADNESSCANCEL		= 348,
	SC_ADJUSTMENT			= 349,
	SC_INCREASING			= 350,
	SC_DISARM				= 351,
	SC_GATLINGFEVER			= 352,
	SC_FULLBUSTER			= 353,
	//�j���W���X�L��
	SC_TATAMIGAESHI			= 354,
	SC_UTSUSEMI				= 355,//#NJ_UTSUSEMI#
	SC_BUNSINJYUTSU			= 356,//#NJ_BUNSINJYUTSU#
	SC_SUITON				= 357,//#NJ_SUITON#
	SC_NEN					= 358,//#NJ_NEN#
	SC_AVOID				= 359,//#�ً}���#
	SC_CHANGE				= 360,//#�����^���`�F���W#
	SC_DEFENCE				= 361,//#�f�B�t�F���X#
	SC_BLOODLUST			= 362,//#�u���b�h���X�g#
	SC_FLEET				= 363,//#�t���[�g���[�u#
	SC_SPEED				= 364,//#�I�[�o�[�h�X�s�[�h#
	
	//start�ł͎g���Ȃ�resist���A�C�e�����őS�ăN���A���邽�߂̕�
	SC_RESISTCLEAR			= 1001,
	SC_RACECLEAR			= 1002,
	SC_SOUL					= 1003,
	SC_SOULCLEAR			= 1004,
};

enum {
	NV_BASIC = 1,

	SM_SWORD,
	SM_TWOHAND,
	SM_RECOVERY,
	SM_BASH,
	SM_PROVOKE,
	SM_MAGNUM,
	SM_ENDURE,

	MG_SRECOVERY,
	MG_SIGHT,
	MG_NAPALMBEAT,
	MG_SAFETYWALL,
	MG_SOULSTRIKE,
	MG_COLDBOLT,
	MG_FROSTDIVER,
	MG_STONECURSE,
	MG_FIREBALL,
	MG_FIREWALL,
	MG_FIREBOLT,
	MG_LIGHTNINGBOLT,
	MG_THUNDERSTORM,
	AL_DP,
	AL_DEMONBANE,
	AL_RUWACH,
	AL_PNEUMA,
	AL_TELEPORT,
	AL_WARP,
	AL_HEAL,
	AL_INCAGI,
	AL_DECAGI,
	AL_HOLYWATER,
	AL_CRUCIS,
	AL_ANGELUS,
	AL_BLESSING,
	AL_CURE,

	MC_INCCARRY,
	MC_DISCOUNT,
	MC_OVERCHARGE,
	MC_PUSHCART,
	MC_IDENTIFY,
	MC_VENDING,
	MC_MAMMONITE,

	AC_OWL,
	AC_VULTURE,
	AC_CONCENTRATION,
	AC_DOUBLE,
	AC_SHOWER,

	TF_DOUBLE,
	TF_MISS,
	TF_STEAL,
	TF_HIDING,
	TF_POISON,
	TF_DETOXIFY,

	ALL_RESURRECTION,

	KN_SPEARMASTERY,
	KN_PIERCE,
	KN_BRANDISHSPEAR,
	KN_SPEARSTAB,
	KN_SPEARBOOMERANG,
	KN_TWOHANDQUICKEN,
	KN_AUTOCOUNTER,
	KN_BOWLINGBASH,
	KN_RIDING,
	KN_CAVALIERMASTERY,

	PR_MACEMASTERY,
	PR_IMPOSITIO,
	PR_SUFFRAGIUM,
	PR_ASPERSIO,
	PR_BENEDICTIO,
	PR_SANCTUARY,
	PR_SLOWPOISON,
	PR_STRECOVERY,
	PR_KYRIE,
	PR_MAGNIFICAT,
	PR_GLORIA,
	PR_LEXDIVINA,
	PR_TURNUNDEAD,
	PR_LEXAETERNA,
	PR_MAGNUS,

	WZ_FIREPILLAR,
	WZ_SIGHTRASHER,
	WZ_FIREIVY,
	WZ_METEOR,
	WZ_JUPITEL,
	WZ_VERMILION,
	WZ_WATERBALL,
	WZ_ICEWALL,
	WZ_FROSTNOVA,
	WZ_STORMGUST,
	WZ_EARTHSPIKE,
	WZ_HEAVENDRIVE,
	WZ_QUAGMIRE,
	WZ_ESTIMATION,

	BS_IRON,
	BS_STEEL,
	BS_ENCHANTEDSTONE,
	BS_ORIDEOCON,
	BS_DAGGER,
	BS_SWORD,
	BS_TWOHANDSWORD,
	BS_AXE,
	BS_MACE,
	BS_KNUCKLE,
	BS_SPEAR,
	BS_HILTBINDING,
	BS_FINDINGORE,
	BS_WEAPONRESEARCH,
	BS_REPAIRWEAPON,
	BS_SKINTEMPER,
	BS_HAMMERFALL,
	BS_ADRENALINE,
	BS_WEAPONPERFECT,
	BS_OVERTHRUST,
	BS_MAXIMIZE,

	HT_SKIDTRAP,
	HT_LANDMINE,
	HT_ANKLESNARE,
	HT_SHOCKWAVE,
	HT_SANDMAN,
	HT_FLASHER,
	HT_FREEZINGTRAP,
	HT_BLASTMINE,
	HT_CLAYMORETRAP,
	HT_REMOVETRAP,
	HT_TALKIEBOX,
	HT_BEASTBANE,
	HT_FALCON,
	HT_STEELCROW,
	HT_BLITZBEAT,
	HT_DETECTING,
	HT_SPRINGTRAP,

	AS_RIGHT,
	AS_LEFT,
	AS_KATAR,
	AS_CLOAKING,
	AS_SONICBLOW,
	AS_GRIMTOOTH,
	AS_ENCHANTPOISON,
	AS_POISONREACT,
	AS_VENOMDUST,
	AS_SPLASHER,

	NV_FIRSTAID,
	NV_TRICKDEAD,
	SM_MOVINGRECOVERY,
	SM_FATALBLOW,
	SM_AUTOBERSERK,
	AC_MAKINGARROW,
	AC_CHARGEARROW,
	TF_SPRINKLESAND,
	TF_BACKSLIDING,
	TF_PICKSTONE,
	TF_THROWSTONE,
	MC_CARTREVOLUTION,
	MC_CHANGECART,
	MC_LOUD,
	AL_HOLYLIGHT,
	MG_ENERGYCOAT,

	NPC_PIERCINGATT=158,
	NPC_MENTALBREAKER,
	NPC_RANGEATTACK,
	NPC_ATTRICHANGE,
	NPC_CHANGEWATER,
	NPC_CHANGEGROUND,
	NPC_CHANGEFIRE,
	NPC_CHANGEWIND,
	NPC_CHANGEPOISON,
	NPC_CHANGEHOLY,
	NPC_CHANGETELEKINESIS,
	NPC_CHANGEDARKNESS,
	NPC_CRITICALSLASH,
	NPC_COMBOATTACK,
	NPC_GUIDEDATTACK,
	NPC_SELFDESTRUCTION,
	NPC_SPLASHATTACK,
	NPC_SUICIDE,
	NPC_POISON,
	NPC_BLINDATTACK,
	NPC_SILENCEATTACK,
	NPC_STUNATTACK,
	NPC_PETRIFYATTACK,
	NPC_CURSEATTACK,
	NPC_SLEEPATTACK,
	NPC_RANDOMATTACK,
	NPC_WATERATTACK,
	NPC_GROUNDATTACK,
	NPC_FIREATTACK,
	NPC_WINDATTACK,
	NPC_POISONATTACK,
	NPC_HOLYATTACK,
	NPC_DARKNESSATTACK,
	NPC_TELEKINESISATTACK,
	NPC_MAGICALATTACK,
	NPC_METAMORPHOSIS,
	NPC_PROVOCATION,
	NPC_SMOKING,
	NPC_SUMMONSLAVE,
	NPC_EMOTION,
	NPC_TRANSFORMATION,
	NPC_BLOODDRAIN,
	NPC_ENERGYDRAIN,
	NPC_KEEPING,
	NPC_DARKBREATH,
	NPC_DARKBLESSING,
	NPC_BARRIER,
	NPC_DEFENDER,
	NPC_LICK,
	NPC_HALLUCINATION,
	NPC_REBIRTH,
	NPC_SUMMONMONSTER,

	RG_SNATCHER,
	RG_STEALCOIN,
	RG_BACKSTAP,
	RG_TUNNELDRIVE,
	RG_RAID,
	RG_STRIPWEAPON,
	RG_STRIPSHIELD,
	RG_STRIPARMOR,
	RG_STRIPHELM,
	RG_INTIMIDATE,
	RG_GRAFFITI,
	RG_FLAGGRAFFITI,
	RG_CLEANER,
	RG_GANGSTER,
	RG_COMPULSION,
	RG_PLAGIARISM,

	AM_AXEMASTERY,
	AM_LEARNINGPOTION,
	AM_PHARMACY,
	AM_DEMONSTRATION,
	AM_ACIDTERROR,
	AM_POTIONPITCHER,
	AM_CANNIBALIZE,
	AM_SPHEREMINE,
	AM_CP_WEAPON,
	AM_CP_SHIELD,
	AM_CP_ARMOR,
	AM_CP_HELM,
	AM_BIOETHICS,
	AM_BIOTECHNOLOGY,
	AM_CREATECREATURE,
	AM_CULTIVATION,
	AM_FLAMECONTROL,
	AM_CALLHOMUN,
	AM_REST,
	AM_DRILLMASTER,
	AM_HEALHOMUN,
	AM_RESURRECTHOMUN,

	CR_TRUST,
	CR_AUTOGUARD,
	CR_SHIELDCHARGE,
	CR_SHIELDBOOMERANG,
	CR_REFLECTSHIELD,
	CR_HOLYCROSS,
	CR_GRANDCROSS,
	CR_DEVOTION,
	CR_PROVIDENCE,
	CR_DEFENDER,
	CR_SPEARQUICKEN,

	MO_IRONHAND,
	MO_SPIRITSRECOVERY,
	MO_CALLSPIRITS,
	MO_ABSORBSPIRITS,
	MO_TRIPLEATTACK,
	MO_BODYRELOCATION,
	MO_DODGE,
	MO_INVESTIGATE,
	MO_FINGEROFFENSIVE,
	MO_STEELBODY,
	MO_BLADESTOP,
	MO_EXPLOSIONSPIRITS,
	MO_EXTREMITYFIST,
	MO_CHAINCOMBO,
	MO_COMBOFINISH,

	SA_ADVANCEDBOOK,
	SA_CASTCANCEL,
	SA_MAGICROD,
	SA_SPELLBREAKER,
	SA_FREECAST,
	SA_AUTOSPELL,
	SA_FLAMELAUNCHER,
	SA_FROSTWEAPON,
	SA_LIGHTNINGLOADER,
	SA_SEISMICWEAPON,
	SA_DRAGONOLOGY,
	SA_VOLCANO,
	SA_DELUGE,
	SA_VIOLENTGALE,
	SA_LANDPROTECTOR,
	SA_DISPELL,
	SA_ABRACADABRA,
	SA_MONOCELL,
	SA_CLASSCHANGE,
	SA_SUMMONMONSTER,
	SA_REVERSEORCISH,
	SA_DEATH,
	SA_FORTUNE,
	SA_TAMINGMONSTER,
	SA_QUESTION,
	SA_GRAVITY,
	SA_LEVELUP,
	SA_INSTANTDEATH,
	SA_FULLRECOVERY,
	SA_COMA,

	BD_ADAPTATION,
	BD_ENCORE,
	BD_LULLABY,
	BD_RICHMANKIM,
	BD_ETERNALCHAOS,
	BD_DRUMBATTLEFIELD,
	BD_RINGNIBELUNGEN,
	BD_ROKISWEIL,
	BD_INTOABYSS,
	BD_SIEGFRIED,
	BD_RAGNAROK,

	BA_MUSICALLESSON,
	BA_MUSICALSTRIKE,
	BA_DISSONANCE,
	BA_FROSTJOKE,
	BA_WHISTLE,
	BA_ASSASSINCROSS,
	BA_POEMBRAGI,
	BA_APPLEIDUN,

	DC_DANCINGLESSON,
	DC_THROWARROW,
	DC_UGLYDANCE,
	DC_SCREAM,
	DC_HUMMING,
	DC_DONTFORGETME,
	DC_FORTUNEKISS,
	DC_SERVICEFORYOU,

	WE_MALE = 334,
	WE_FEMALE,
	WE_CALLPARTNER,

	NPC_SELFDESTRUCTION2 = 331,
	ITM_TOMAHAWK = 337,
	NPC_DARKCROSS,
	NPC_DARKGRANDCROSS,
	NPC_DARKSOULSTRIKE,
	NPC_DARKJUPITEL,
	NPC_HOLDWEB,
	NPC_BREAKWEAPON,
	NPC_BREAKARMOR,
	NPC_BREAKHELM,
	NPC_BREAKSIELD,
	NPC_UNDEADATTACK,
	NPC_RUNAWAY = 348,
	NPC_EXPLOSIONSPIRITS,
	NPC_INCREASEFLEE,
	NPC_ELEMENTUNDEAD,
	NPC_INVISIBLE,
	NPC_RECALL = 354,

	LK_AURABLADE = 355,
	LK_PARRYING,
	LK_CONCENTRATION,
	LK_TENSIONRELAX,
	LK_BERSERK,
	LK_FURY,
	HP_ASSUMPTIO,
	HP_BASILICA,
	HP_MEDITATIO,
	HW_SOULDRAIN,
	HW_MAGICCRASHER,
	HW_MAGICPOWER,
	PA_PRESSURE,
	PA_SACRIFICE,
	PA_GOSPEL,
	CH_PALMSTRIKE,
	CH_TIGERFIST,
	CH_CHAINCRUSH,
	PF_HPCONVERSION,
	PF_SOULCHANGE,
	PF_SOULBURN,
	ASC_KATAR,
	ASC_HALLUCINATION,
	ASC_EDP,
	ASC_BREAKER,
	SN_SIGHT,
	SN_FALCONASSAULT,
	SN_SHARPSHOOTING,
	SN_WINDWALK,
	WS_MELTDOWN,
	WS_CREATECOIN,
	WS_CREATENUGGET,
	WS_CARTBOOST,
	WS_SYSTEMCREATE,
	ST_CHASEWALK,
	ST_REJECTSWORD,
	ST_STEALBACKPACK,
	CR_ALCHEMY,
	CR_SYNTHESISPOTION,
	CG_ARROWVULCAN,
	CG_MOONLIT,
	CG_MARIONETTE,
	LK_SPIRALPIERCE,
	LK_HEADCRUSH,
	LK_JOINTBEAT,
	HW_NAPALMVULCAN,
	CH_SOULCOLLECT,
	PF_MINDBREAKER,
	PF_MEMORIZE,
	PF_FOGWALL,
	PF_SPIDERWEB,
	ASC_METEORASSAULT,
	ASC_CDP,
	WE_BABY,
	WE_CALLPARENT,
	WE_CALLBABY,
	TK_RUN,
	TK_READYSTORM,
	TK_STORMKICK,
	TK_READYDOWN,
	TK_DOWNKICK,
	TK_READYTURN,
	TK_TURNKICK,
	TK_READYCOUNTER,
	TK_COUNTER,
	TK_DODGE,
	TK_JUMPKICK,
	TK_HPTIME,
	TK_SPTIME,
	TK_POWER,
	TK_SEVENWIND,
	TK_HIGHJUMP,
	SG_FEEL,
	SG_SUN_WARM,
	SG_MOON_WARM,
	SG_STAR_WARM,
	SG_SUN_COMFORT,
	SG_MOON_COMFORT,
	SG_STAR_COMFORT,
	SG_HATE,
	SG_SUN_ANGER,
	SG_MOON_ANGER,
	SG_STAR_ANGER,
	SG_SUN_BLESS,
	SG_MOON_BLESS,
	SG_STAR_BLESS,
	SG_DEVIL,
	SG_FRIEND,
	SG_KNOWLEDGE,
	SG_FUSION,
	SL_ALCHEMIST,
	AM_BERSERKPITCHER,
	SL_MONK,
	SL_STAR,
	SL_SAGE,
	SL_CRUSADER,
	SL_SUPERNOVICE,
	SL_KNIGHT,
	SL_WIZARD,
	SL_PRIEST,
	SL_BARDDANCER,
	SL_ROGUE,
	SL_ASSASIN,
	SL_BLACKSMITH,
	BS_ADRENALINE2,
	SL_HUNTER,
	SL_SOULLINKER,
	SL_KAIZEL,
	SL_KAAHI,
	SL_KAUPE,
	SL_KAITE,
	SL_KAINA,
	SL_STIN,
	SL_STUN,
	SL_SMA,
	SL_SWOO,
	SL_SKE,
	SL_SKA,

	ST_PRESERVE = 475,
	ST_FULLSTRIP,
	WS_WEAPONREFINE,
	CR_SLIMPITCHER,
	CR_FULLPROTECTION,
	PA_SHIELDCHAIN,
	HP_MANARECHARGE,
	PF_DOUBLECASTING,
	HW_GANBANTEIN,
	HW_GRAVITATION,
	WS_CARTTERMINATION,
	WS_OVERTHRUSTMAX,
	CG_LONGINGFREEDOM,
	CG_HERMODE,
	CG_TAROTCARD,
	CR_ACIDDEMONSTRATION,
	CR_CULTIVATION,
	// = 492,
	TK_MISSION		= 493,
	SL_HIGH			= 494,
	KN_ONEHAND		= 495,
	AM_TWILIGHT1	= 496,
	AM_TWILIGHT2	= 497,
	AM_TWILIGHT3	= 498,
	HT_POWER 		= 499,

	GS_GLITTERING   = 500,//#GS_GLITTERING#
	GS_FLING,//#GS_FLING#
	GS_TRIPLEACTION,//#GS_TRIPLEACTION#
	GS_BULLSEYE,//#GS_BULLSEYE#
	GS_MADNESSCANCEL,//#GS_MADNESSCANCEL#
	GS_ADJUSTMENT,//#GS_ADJUSTMENT#
	GS_INCREASING,//#GS_INCREASING#
	GS_MAGICALBULLET,//#GS_MAGICALBULLET#
	GS_CRACKER,//#GS_CRACKER#
	GS_SINGLEACTION,//#GS_SINGLEACTION#
	GS_SNAKEEYE,//#GS_SNAKEEYE#	
	GS_CHAINACTION,//#GS_CHAINACTION#
	GS_TRACKING,//#GS_TRACKING#
	GS_DISARM,//#GS_DISARM#
	GS_PIERCINGSHOT,//#GS_PIERCINGSHOT#
	GS_RAPIDSHOWER,//#GS_RAPIDSHOWER#
	GS_DESPERADO,//#GS_DESPERADO#
	GS_GATLINGFEVER,//#GS_GATLINGFEVER#
	GS_DUST,//#GS_DUST#
	GS_FULLBUSTER,//#GS_FULLBUSTER#
	GS_SPREADATTACK,//#GS_SPREADATTACK#
	GS_GROUNDDRIFT,//#GS_GROUNDDRIFT#

	NJ_TOBIDOUGU,//#NJ_TOBIDOUGU#
	NJ_SYURIKEN,//#NJ_SYURIKEN#
	NJ_KUNAI,//#NJ_KUNAI#
	NJ_HUUMA,//#NJ_HUUMA#
	NJ_ZENYNAGE,//#NJ_ZENYNAGE#
	NJ_TATAMIGAESHI,//#NJ_TATAMIGAESHI#
	NJ_KASUMIKIRI,//#NJ_KASUMIKIRI#
	NJ_SHADOWJUMP,//#NJ_SHADOWJUMP#
	NJ_KIRIKAGE,//#NJ_KIRIKAGE#
	NJ_UTSUSEMI,//#NJ_UTSUSEMI#
	NJ_BUNSINJYUTSU,//#NJ_BUNSINJYUTSU#
	NJ_NINPOU,//#NJ_NINPOU#
	NJ_KOUENKA,//#NJ_KOUENKA#
	NJ_KAENSIN,//#NJ_KAENSIN#
	NJ_BAKUENRYU,//#NJ_BAKUENRYU#
	NJ_HYOUSENSOU,//#NJ_HYOUSENSOU#
	NJ_SUITON,//#NJ_SUITON#
	NJ_HYOUSYOURAKU,//#NJ_HYOUSYOURAKU#
	NJ_HUUJIN,//#NJ_HUUJIN#
	NJ_RAIGEKISAI,//#NJ_RAIGEKISAI#
	NJ_KAMAITACHI,//#NJ_KAMAITACHI#
	NJ_NEN,//#NJ_NEN#
	NJ_ISSEN,//#NJ_ISSEN#
	
	KN_CHARGEATK	=	1001,//#�`���[�W�A�^�b�N#
	CR_SHRINK		=	1002,//#�V�������N#
	AS_SONICACCEL	=	1003,//#�\�j�b�N�A�N�Z�����[�V����#
	AS_VENOMKNIFE	=	1004,//#�x�i���i�C�t#
	RG_CLOSECONFINE	=	1005,//#�N���[�Y�R���t�@�C��#
	WZ_SIGHTBLASTER	=	1006,//#�T�C�g�u���X�^�[#
	SA_CREATECON	=	1007,//#�G���������^���R���o�[�^����#
	SA_ELEMENTWATER	=	1008,//#�G���������^���`�F���W�i���j#
	HT_PHANTASMIC	=	1009,//#�t�@���^�X�~�b�N�A���[#
	BA_PANGVOICE	=	1010,//#�p���{�C�X#
	DC_WINKCHARM	=	1011,//#���f�̃E�B���N#
	BS_UNFAIRLYTRICK=	1012,//#�A���t�F�A���[�g���b�N#
	BS_GREED		=	1013,//#�×~#
	PR_REDEMPTIO	=	1014,//#���f���v�e�B�I#
	MO_KITRANSLATION=	1015,//#�����(�U�C����)#
	MO_BALKYOUNG	=	1016,//#����(����)#
	SA_ELEMENTGROUND=	1017,
	SA_ELEMENTFIRE	=	1018,
	SA_ELEMENTWIND	=	1019,

	HM_SKILLBASE	=8001,
	HLIF_HEAL		=8001,//#�����̎菕��(�q�[��)#
	HLIF_AVOID		=8002,//#�ً}���#
	HLIF_BRAIN		=8003,//=//#�]��p#
	HLIF_CHANGE		=8004,//#�����^���`�F���W#
	HAMI_CASTLE		=8005,//#�L���X�g�����O#
	HAMI_DEFENCE	=8006,//#�f�B�t�F���X#
	HAMI_SKIN		=8007,//#�A�_�}���e�B�E���X�L��#
	HAMI_BLOODLUST	=8008,//#�u���b�h���X�g#
	HFLI_MOON		=8009,//#���[�����C�g#
	HFLI_FLEET		=8010,//#�t���[�g���[�u#
	HFLI_SPEED		=8011,//#�I�[�o�[�h�X�s�[�h#
	HFLI_SBR44		=8012,//#S.B.R.44#
	HVAN_CAPRICE	=8013,//#�J�v���X#
	HVAN_CHAOTIC	=8014,//#�J�I�e�B�b�N�x�l�f�B�N�V����#
	HVAN_INSTRUCT	=8015,//#�`�F���W�C���X�g���N�V����#
	HVAN_EXPLOSION	=8016,//#�o�C�I�G�N�X�v���[�W����#

//	move to common/mmo.h
//	GD_APPROVAL=10000,
//	GD_KAFRACONTACT,
//	GD_GUARDIANRESEARCH,
//	GD_CHARISMA,
//	GD_EXTENSION,
	//return from common/mmo.h
	GD_SKILLBASE=10000,
	GD_APPROVAL=10000,
	GD_KAFRACONTACT,
	GD_GUARDIANRESEARCH,
	GD_GUARDUP,
	//GD_CHARISMA, -> GD_GUARDUP�ɕύX����
	// GD_GURADUP,	// GD)CHARISMA�Ɠ�ID
	GD_EXTENSION,
	GD_GLORYGUILD,
	GD_LEADERSHIP,
	GD_GLORYWOUNDS,
	GD_SOULCOLD,
	GD_HAWKEYES,
	GD_BATTLEORDER,
	GD_REGENERATION,
	GD_RESTORE,
	GD_EMERGENCYCALL,
	GD_DEVELOPMENT,

};

//��ԃA�C�R��
//�����ɂ̓L�����N�^�[�̐F�̕ω��Ȃǂ��܂܂�Ă���(�����g���Ȃ�)
enum {
	SI_BLANK				= 43,//�����Ă��󔒂̂���

	SI_PROVOKE				= 0,
	SI_ENDURE				= 1,
	SI_TWOHANDQUICKEN		= 2,
	SI_CONCENTRATE			= 3,
	SI_HIDING				= 4,
	SI_CLOAKING				= 5,
	SI_ENCPOISON			= 6,
	SI_POISONREACT			= 7,
	SI_QUAGMIRE				= 8,
	SI_ANGELUS				= 9,
	SI_BLESSING				=10,
	SI_SIGNUMCRUCIS			=11,
	SI_INCREASEAGI			=12,
	SI_DECREASEAGI			=13,
	SI_SLOWPOISON			=14,
	SI_IMPOSITIO			=15,
	SI_SUFFRAGIUM			=16,
	SI_ASPERSIO				=17,
	SI_BENEDICTIO			=18,
	SI_KYRIE				=19,
	SI_MAGNIFICAT			=20,
	SI_GLORIA				=21,
	SI_AETERNA				=22,
	SI_ADRENALINE			=23,
	SI_WEAPONPERFECTION		=24,
	SI_OVERTHRUST			=25,
	SI_MAXIMIZEPOWER		=26,
	SI_RIDING				=27,
	SI_FALCON				=28,
	SI_TRICKDEAD			=29,
	SI_LOUD					=30,
	SI_ENERGYCOAT			=31,
	SI_HALLUCINATION		=34,
	SI_WEIGHT50				=35,
	SI_WEIGHT90				=36,
	SI_SPEEDPOTION0			=37,
	SI_SPEEDPOTION1			=38,
	SI_SPEEDPOTION2			=39,
	SI_SPEEDPOTION3			=40,
	SI_INCREASEAGI2			=41,
	SI_INCREASEAGI3			=42,
	SI_STRIPWEAPON			=50,
	SI_STRIPSHIELD			=51,
	SI_STRIPARMOR			=52,
	SI_STRIPHELM			=53,
	SI_CP_WEAPON			=54,
	SI_CP_SHIELD			=55,
	SI_CP_ARMOR				=56,
	SI_CP_HELM				=57,
	SI_AUTOGUARD			=58,
	SI_REFLECTSHIELD		=59,
	SI_DEVOTION				=60,
	SI_PROVIDENCE			=61,
	SI_DEFENDER				=62,
	SI_AUTOSPELL			=65,
	SI_SPEARSQUICKEN		=68,
	SI_EXPLOSIONSPIRITS		=86,	/* �����g�� */
	SI_STEELBODY			=87,	/* �����G�t�F�N�g */
	SI_COMBO				=89,
	SI_FLAMELAUNCHER		=90,	/* �t���C�������`���[ */
	SI_FROSTWEAPON			=91,	/* �t���X�g�E�F�|�� */
	SI_LIGHTNINGLOADER		=92,	/* ���C�g�j���O���[�_�[ */
	SI_SEISMICWEAPON		=93,	/* �T�C�Y�~�b�N�E�F�|�� */
	SI_AURABLADE			=103,	/* �I�[���u���[�h */
	SI_PARRYING				=104,	/* �p���C���O */
	SI_CONCENTRATION		=105,	/* �R���Z���g���[�V���� */
	SI_TENSIONRELAX			=106,	/* �e���V���������b�N�X */
	SI_BERSERK				=107,	/* �o�[�T�[�N */
	SI_ASSUMPTIO			=110,	/* �A�V�����v�e�B�I */
	SI_ELEMENTFIELD 		=112,	/* ������ */
	SI_MAGICPOWER			=113,	/* ���@�͑��� */
	SI_EDP					=114, 	//
	SI_TRUESIGHT			=115,	/* �g�D���[�T�C�g */
	SI_WINDWALK				=116,	/* �E�C���h�E�H�[�N */
	SI_MELTDOWN				=117,	/* �����g�_�E�� */
	SI_CARTBOOST			=118,	/* �J�[�g�u�[�X�g */
	SI_CHASEWALK			=119,	/* �`�F�C�X�E�H�[�N */
	SI_REJECTSWORD			=120,	/* ���W�F�N�g�\�[�h */
	SI_MARIONETTE			=121,	/* �}���I�l�b�g�R���g���[�� */
	SI_MARIONETTE2			=122,	/* �}���I�l�b�g�R���g���[��2 */
	SI_MOONLIT 				=123,	//��������̉���
	SI_HEADCRUSH			=124,	/* �w�b�h�N���b�V�� */
	SI_JOINTBEAT			=125,	/* �W���C���g�r�[�g */
	SI_BABY					=130,	//�p�p�A�}�}�A��D��
	SI_AUTOBERSERK			=132,	//�I�[�g�o�[�T�[�N
	SI_READYSTORM			=135,	//��������
	SI_READYDOWN			=137,	//���i����
	SI_READYTURN			=139,	//��]����
	SI_READYCOUNTER			=141,	//�J�E���^�[����
	SI_DODGE				=143,	//���@
	SI_RUN					=145,	//�삯��
	SI_DARKELEMENT			=146,	//��
	SI_ADRENALINE2			=147,	//�t���A�h���i�������b�V��
	SI_ATTENELEMENT			=148,	//�O
	SI_SOULLINK				=149,	//�����
	SI_DEVIL				=152,	//���z�ƌ��Ɛ��̈���
	SI_KAITE				=153,	//�J�C�g
	SI_KAIZEL				=156,	//�J�C�[��
	SI_KAAHI				=157,	//�J�A�q
	SI_KAUPE				=158,	//�J�E�v
	SI_ONEHAND				=161,	//�����n���h�N�C�b�P��
	SI_SUN_WARM				=165,	//���z�̉�����
	SI_MOON_WARM			=166,	//���̉�����
	SI_STAR_WARM			=167,	//���̉�����
	SI_SUN_COMFORT			=169,	//���z�̈��y
	SI_MOON_COMFORT			=170,	//���̈��y
	SI_STAR_COMFORT			=171,	//���̈��y
	SI_PRESERVE				=181,	//�v���U�[�u
	SI_CHASEWALK_STR		=182,	//�`�F�C�X�E�H�[�N��STR?
	SI_SPURT				=182,	//�X�p�[�g��Ԃɂ��g����
	SI_DOUBLECASTING		=186,	//�_�u���L���X�e�B���O
	SI_OVERTHRUSTMAX		=188,	//�I�[�o�[�g���X�g�}�b�N�X
	SI_TAROTCARD			=191,
	SI_SHRINK				=197,	//#�V�������N#
	SI_SIGHTBLASTER			=198,	//#�T�C�g�u���X�^�[#
	SI_CLOSECONFINE			=200,	//#�N���[�Y�R���t�@�C��#
	SI_CLOSECONFINE2		=201,	//#�N���[�Y�R���t�@�C��#
	SI_INVISIBLE			= SI_CLOAKING,
	SI_MADNESSCANCEL		=203,	//#GS_MADNESSCANCEL#
	SI_ADJUSTMENT			=209,	//#GS_ADJUSTMENT#
	SI_INCREASING			=210,	//#GS_INCREASING#
	SI_GATLINGFEVER			=204,	//#GS_GATLINGFEVER#
	SI_UTSUSEMI				=206,	//#NJ_UTSUSEMI#
	SI_BUNSINJYUTSU			=207,	//#NJ_BUNSINJYUTSU#
	SI_NEN					=208,	//#NJ_NEN#
};


extern int SkillStatusChangeTable[];
extern int StatusIconChangeTable[];
extern int GuildSkillStatusChangeTable[];

struct skill_unit_group *skill_unitsetting( struct block_list *src, int skillid,int skilllv,int x,int y,int flag);

#endif

