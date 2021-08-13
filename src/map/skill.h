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

// スキルデータベース
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
#define MAX_SQUARE_LAYOUT		5	// 11*11のユニット配置が最大
#define MAX_SKILL_UNIT_COUNT ((MAX_SQUARE_LAYOUT*2+1)*(MAX_SQUARE_LAYOUT*2+1))
struct skill_unit_layout {
	int count;
	int dx[MAX_SKILL_UNIT_COUNT];
	int dy[MAX_SKILL_UNIT_COUNT];
};

enum {
	UF_DEFNOTENEMY		= 0x0001,	// defnotenemy 設定でBCT_NOENEMYに切り替え
	UF_NOREITERATION	= 0x0002,	// 重複置き禁止
	UF_NOFOOTSET		= 0x0004,	// 足元置き禁止
	UF_NOOVERLAP		= 0x0008,	// ユニット効果が重複しない
	UF_PATHCHECK		= 0x0010,	// オブジェクト発生時に射線チェック
	UF_DANCE			= 0x0100,	// ダンススキル
	UF_ENSEMBLE			= 0x0200,	// 合奏スキル
};

extern struct skill_db skill_db[MAX_SKILL_DB+MAX_HOMSKILL_DB+MAX_GUILDSKILL_DB];

// アイテム作成データベース
struct skill_produce_db {
	int nameid, trigger;
	int req_skill,req_skilllv,itemlv;
	int mat_id[MAX_PRODUCE_RESOURCE],mat_amount[MAX_PRODUCE_RESOURCE];
};
extern struct skill_produce_db skill_produce_db[MAX_SKILL_PRODUCE_DB];

// 矢作成データベース
struct skill_arrow_db {
	int nameid, trigger;
	int cre_id[5],cre_amount[5];
};
extern struct skill_arrow_db skill_arrow_db[MAX_SKILL_ARROW_DB];

// アブラカダブラデータベース
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
//マクロから関数化
//ギルドIDも使用可能に
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

// スキルの使用
void skill_castend_map(struct map_session_data *sd, int skill_num, const char *map);

int skill_cleartimerskill(struct block_list *src);
int skill_addtimerskill(struct block_list *src,unsigned int tick,int target,int x,int y,int skill_id,int skill_lv,int type,int flag);

// 追加効果
int skill_additional_effect( struct block_list* src, struct block_list *bl,int skillid,int skilllv,int attack_type,unsigned int tick);

int skill_delunit_by_ganbatein(struct block_list *bl, va_list ap );

enum {	//吹き飛ばしフラグ
	SAB_NOMALBLOW   = 0x00000,
	SAB_REVERSEBLOW = 0x10000,
	SAB_NODAMAGE    = 0x20000,
	SAB_NOPATHSTOP  = 0x40000,
};

#define	SAB_NORMAL	0x00010000
#define	SAB_SKIDTRAP	0x00020000
int skill_add_blown( struct block_list *src, struct block_list *target,int skillid,int flag);

//カード効果のオートスペル
#define AS_ATTACK	0x00050003
#define AS_REVENGE	0x00060003
int skill_use_bonus_autospell(struct block_list * src,struct block_list * bl,int skill_id,int skill_lv,int rate,long skill_flag,int tick,int flag);
int skill_bonus_autospell(struct block_list * src,struct block_list * bl,long mode,int tick,int flag);

// ユニットスキル
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

// 詠唱キャンセル
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

// その他
int skill_check_cloaking(struct block_list *bl);

// スキル使用かどうかの判定。

// 判定関数に渡す構造体。関数内部でデータが上書きされるので、
// 戻った後に変更するのを忘れないように。
struct skill_condition {
	int id;
	int lv;
	int x;
	int y;
	int target;
};

int skill_check_condition(struct block_list *bl, int type);
int skill_check_condition2(struct block_list *bl, struct skill_condition *sc, int type);

// アイテム作成
int skill_can_produce_mix( struct map_session_data *sd, int nameid, int trigger );
void skill_produce_mix(struct map_session_data *sd, int nameid, int slot1, int slot2, int slot3);
int skill_am_twilight1(struct map_session_data* sd);
int skill_am_twilight2(struct map_session_data* sd);
int skill_am_twilight3(struct map_session_data* sd);

void skill_arrow_create(struct map_session_data *sd, int nameid);
int skill_can_repair( struct map_session_data *sd, int nameid );
int skill_repair_weapon(struct map_session_data *sd, int idx);

// mobスキルのため
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

// スキル攻撃一括処理
int skill_blown( struct block_list *src, struct block_list *target,int count);

int skill_castend_delay (struct block_list* src, struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag);

// バシリカ発動停止
void skill_basilica_cancel( struct block_list *bl );

void skill_reload(void);

enum {
	ST_NONE,ST_HIDING,ST_CLOAKING,ST_HIDDEN,ST_RIDING,ST_FALCON,ST_CART,ST_SHIELD,ST_SIGHT,ST_EXPLOSIONSPIRITS,
	ST_RECOV_WEIGHT_RATE,ST_MOVE_ENABLE,ST_WATER,
};

enum {	// struct map_session_data の status_changeの番号テーブル
// SC_SENDMAX未満はクライアントへの通知あり。
// 2-2次職の値はなんかめちゃくちゃっぽいので暫定。たぶん変更されます。
	//SC_SENDMAX				=128,

	SC_PROVOKE				= 0,	/* プロボック */
	SC_ENDURE				= 1,	/* インデュア */
	SC_TWOHANDQUICKEN		= 2,	/* ツーハンドクイッケン */
	SC_CONCENTRATE			= 3,	/* 集中力向上 */
	SC_HIDING				= 4,	/* ハイディング */
	SC_CLOAKING				= 5,	/* クローキング */
	SC_ENCPOISON			= 6,	/* エンチャントポイズン */
	SC_POISONREACT			= 7,	/* ポイズンリアクト */
	SC_QUAGMIRE				= 8,	/* クァグマイア */
	SC_ANGELUS				= 9,	/* エンジェラス */
	SC_BLESSING				=10,	/* ブレッシング */
	SC_SIGNUMCRUCIS			=11,	/* シグナムクルシス？ */
	SC_INCREASEAGI			=12,	/*  */
	SC_DECREASEAGI			=13,	/*  */
	SC_SLOWPOISON			=14,	/* スローポイズン */
	SC_IMPOSITIO			=15,	/* イムポシティオマヌス */
	SC_SUFFRAGIUM			=16,	/* サフラギウム */
	SC_ASPERSIO				=17,	/* アスペルシオ */
	SC_BENEDICTIO			=18,	/* 聖体降臨 */
	SC_KYRIE				=19,	/* キリエエレイソン */
	SC_MAGNIFICAT			=20,	/* マグニフィカート */
	SC_GLORIA				=21,	/* グロリア */
	SC_AETERNA				=22,	/*  */
	SC_ADRENALINE			=23,	/* アドレナリンラッシュ */
	SC_WEAPONPERFECTION		=24,	/* ウェポンパーフェクション */
	SC_OVERTHRUST			=25,	/* オーバートラスト */
	SC_MAXIMIZEPOWER		=26,	/* マキシマイズパワー */
	SC_RIDING				=27,	/* ライディング */
	SC_FALCON				=28,	/* ファルコンマスタリー */
	SC_TRICKDEAD			=29,	/* 死んだふり */
	SC_LOUD					=30,	/* ラウドボイス */
	SC_ENERGYCOAT			=31,	/* エナジーコート */
	SC_PK_PENALTY			=32,	//PKのペナルティ
	SC_REVERSEORCISH		=33,    //リバースオーキッシュ
	SC_HALLUCINATION		=34,	/* ハルネーションウォーク？ */
	SC_WEIGHT50				=35,	/* 重量50％オーバー */
	SC_WEIGHT90				=36,	/* 重量90％オーバー */
	SC_SPEEDPOTION0			=37,	/* 速度ポーション？ */
	SC_SPEEDPOTION1			=38,	/* スピードアップポーション？ */
	SC_SPEEDPOTION2			=39,	/* ハイスピードポーション？ */
	SC_SPEEDPOTION3			=40,	/* バーサークポーション */
	SC_ITEM_DELAY			=41,
	//
	//
	//
	//
	//
	//
	//
	//
	SC_STRIPWEAPON			=50,	/* ストリップウェポン */
	SC_STRIPSHIELD			=51,	/* ストリップシールド */
	SC_STRIPARMOR			=52,	/* ストリップアーマー */
	SC_STRIPHELM			=53,	/* ストリップヘルム */
	SC_CP_WEAPON			=54,	/* ケミカルウェポンチャージ */
	SC_CP_SHIELD			=55,	/* ケミカルシールドチャージ */
	SC_CP_ARMOR				=56,	/* ケミカルアーマーチャージ */
	SC_CP_HELM				=57,	/* ケミカルヘルムチャージ */
	SC_AUTOGUARD			=58,	/* オートガード */
	SC_REFLECTSHIELD		=59,	/* リフレクトシールド */
	SC_DEVOTION				=60,	/* ディボーション */
	SC_PROVIDENCE			=61,	/* プロヴィデンス */
	SC_DEFENDER				=62,	/* ディフェンダー */
	SC_SANTA				=63,	//サンタ
	//
	SC_AUTOSPELL			=65,	/* オートスペル */
	//
	//
	SC_SPEARSQUICKEN		=68,	/* スピアクイッケン */
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
	SC_EXPLOSIONSPIRITS		=86,	/* 爆裂波動 */
	SC_STEELBODY			=87,	/* 金剛 */
	//
	SC_COMBO				=89,
	SC_FLAMELAUNCHER		=90,	/* フレイムランチャー */
	SC_FROSTWEAPON			=91,	/* フロストウェポン */
	SC_LIGHTNINGLOADER		=92,	/* ライトニングローダー */
	SC_SEISMICWEAPON		=93,	/* サイズミックウェポン */
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
	SC_AURABLADE			=103,	/* オーラブレード */
	SC_PARRYING				=104,	/* パリイング */
	SC_CONCENTRATION		=105,	/* コンセントレーション */
	SC_TENSIONRELAX			=106,	/* テンションリラックス */
	SC_BERSERK				=107,	/* バーサーク */
	//
	//
	//
	SC_ASSUMPTIO			=110,	/* アスムプティオ */
	//
	//
	SC_MAGICPOWER			=113,	/* 魔法力増幅 */
	SC_EDP					=114,	/* エフェクトが判明したら移動 */
	SC_TRUESIGHT			=115,	/* トゥルーサイト */
	SC_WINDWALK				=116,	/* ウインドウォーク */
	SC_MELTDOWN				=117,	/* メルトダウン */
	SC_CARTBOOST			=118,	/* カートブースト */
	SC_CHASEWALK			=119,	/* チェイスウォーク */
	SC_REJECTSWORD			=120,	/* リジェクトソード */
	SC_MARIONETTE			=121,	/* マリオネットコントロール */ //自分用
	SC_MARIONETTE2			=122,	/* マリオネットコントロール */ //ターゲット用
	//
	SC_HEADCRUSH			=124,	/* ヘッドクラッシュ */
	SC_JOINTBEAT			=125,	/* ジョイントビート */
	//
	//
	SC_STONE				=128,	/* 状態異常：石化 */
	SC_FREEZE				=129,	/* 状態異常：氷結 */
	SC_STAN					=130,	/* 状態異常：スタン */
	SC_SLEEP				=131,	/* 状態異常：睡眠 */
	SC_POISON				=132,	/* 状態異常：毒 */
	SC_CURSE				=133,	/* 状態異常：呪い */
	SC_SILENCE				=134,	/* 状態異常：沈黙 */
	SC_CONFUSION			=135,	/* 状態異常：混乱 */
	SC_BLIND				=136,	/* 状態異常：暗闇 */
	SC_BLEED				=137,	/* 状態異常：出血 */
	SC_DIVINA				= SC_SILENCE,	/* 状態異常：沈黙 */
	//138
	//139
	SC_SAFETYWALL			=140,	/* セーフティーウォール */
	SC_PNEUMA				=141,	/* ニューマ */
	//
	SC_ANKLE				=143,	/* アンクルスネア */
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
	SC_SPIDERWEB			=180,	/* スパイダーウェッブ */
	SC_MEMORIZE				=181,	/* メモライズ */
	SC_DPOISON				=182,	/* 猛毒 */
	//
	SC_SACRIFICE			=184,	/* サクリファイス */
	SC_INCATK				=185,	//item 682用
	SC_INCMATK				=186,	//item 683用
	SC_WEDDING				=187,	//結婚用(結婚衣裳になって歩くのが遅いとか)
	SC_NOCHAT				=188,	//赤エモ状態
	SC_SPLASHER				=189,	/* ベナムスプラッシャー */
	SC_SELFDESTRUCTION		=190,	/* 自爆 */
	SC_MAGNUM				=191,	/* マグナムブレイク */
	SC_GOSPEL				=192,	/* ゴスペル */
	SC_INCALLSTATUS			=193,	/* 全てのステータスを上昇(今のところゴスペル用) */
	SC_INCHIT				=194,	/* HIT上昇(今のところゴスペル用) */
	SC_INCFLEE				=195,	/* FLEE上昇(今のところゴスペル用) */
	SC_INCMHP2				=196,	/* MHPを%上昇(今のところゴスペル用) */
	SC_INCMSP2				=197,	/* MSPを%上昇(今のところゴスペル用) */
	SC_INCATK2				=198,	/* ATKを%上昇(今のところゴスペル用) */
	SC_INCHIT2				=199,	/* HITを%上昇(今のところゴスペル用) */
	SC_INCFLEE2				=200,	/* FLEEを%上昇(今のところゴスペル用) */
	SC_PRESERVE				=201,	/* プリザーブ */
	SC_OVERTHRUSTMAX		=202,	/* オーバートラストマックス */
	SC_CHASEWALK_STR		=203,	/*STR上昇（チェイスウォーク用）*/
	SC_WHISTLE_				=204,
	SC_ASSNCROS_			=205,
	SC_POEMBRAGI_			=206,
	SC_APPLEIDUN_			=207,
	SC_HUMMING_				=209,
	SC_DONTFORGETME_		=210,
	SC_FORTUNE_				=211,
	SC_SERVICE4U_			=212,
	SC_BATTLEORDER			=213,//ギルドスキル
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
	SC_MEAL_INCSTR			=224,//食事用
	SC_MEAL_INCAGI			=225,
	SC_MEAL_INCVIT			=226,
	SC_MEAL_INCINT			=227,
	SC_MEAL_INCDEX			=228,
	SC_MEAL_INCLUK			=229,
	SC_RUN 					= 230,
	SC_SPURT 				= 231,
	SC_TKCOMBO 				= 232,	//テコンのコンボ用
	SC_DODGE				= 233,
	SC_HERMODE				= 234,
	SC_TRIPLEATTACK_RATE_UP	= 235,	//三段発動率アップ
	SC_COUNTER_RATE_UP		= 236,	//カウンターキック発動率アップ
	SC_SUN_WARM				= 237,
	SC_MOON_WARM			= 238,
	SC_STAR_WARM			= 239,
	SC_SUN_COMFORT			= 240,
	SC_MOON_COMFORT			= 241,
	SC_STAR_COMFORT			= 242,
	SC_FUSION				= 243,
	SC_ALCHEMIST			= 244,//魂
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
	//魂の追加があったらいけないので予約
	//なければ適当にあとから埋める
	//= 260,	//忍者の魂の予約？
	//= 261,	//ガンスリンガーの魂の予約？
	//= 262,	//？？？の魂の予約？
	SC_ADRENALINE2			= 263,
	SC_KAIZEL				= 264,
	SC_KAAHI				= 265,
	SC_KAUPE				= 266,
	SC_KAITE				= 267,
	SC_SMA					= 268,	//エスマ詠唱可能時間用
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
	SC_DOUBLECASTING 		= 280,//ダブルキャスティング
	SC_ELEMENTFIELD			= 281,//属性場
	SC_DARKELEMENT			= 282,//闇
	SC_ATTENELEMENT			= 283,//念
	SC_MIRACLE				= 284,//太陽と月と星の奇跡
	SC_ANGEL				= 285,//太陽と月と星の天使
	SC_HIGHJUMP				= 286,//ハイジャンプ
	SC_DOUBLE				= 287,//ダブルストレイフィング状態
	SC_ACTION_DELAY			= 288,//
	SC_BABY					= 289,//パパ、ママ、大好き
	SC_LONGINGFREEDOM		= 290,
	SC_SHRINK				= 291,//#シュリンク#
	SC_CLOSECONFINE			= 292,//#クローズコンファイン#
	SC_SIGHTBLASTER			= 293,//#サイトブラスター#
	SC_ELEMENTWATER			= 294,//#エルレメンタルチェンジ水#
	//食事用2
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
	SC_ELEMENTGROUND		= 305,//土(鎧)
	SC_ELEMENTFIRE			= 306,//火(鎧)
	SC_ELEMENTWIND			= 307,//風(鎧)
	SC_WINKCHARM			= 308,
	SC_ELEMENTPOISON		= 309,//毒(鎧)
	SC_ELEMENTDARK			= 310,//闇(鎧)
	SC_ELEMENTELEKINESIS	= 311,//念(鎧)
	SC_ELEMENTUNDEAD		= 312,//不死(鎧)
	SC_UNDEADELEMENT		= 313,//不死(武)
	SC_ELEMENTHOLY			= 314,//聖(鎧)
	SC_NPC_DEFENDER			= 315,
	SC_RESISTWATER			= 316,//耐性
	SC_RESISTGROUND			= 317,//耐性
	SC_RESISTFIRE			= 318,//耐性
	SC_RESISTWIND			= 319,//耐性
	SC_RESISTPOISON			= 320,//耐性
	SC_RESISTHOLY			= 321,//耐性
	SC_RESISTDARK			= 322,//耐性
	SC_RESISTTELEKINESIS	= 323,//耐性
	SC_RESISTUNDEAD			= 324,//耐性
	SC_RESISTALL			= 325,//耐性
	//種族変更？
	SC_RACEUNKNOWN			= 326,//無形
	SC_RACEUNDEAD			= 327,//不死種族
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
	//ニンジャスキル
	SC_TATAMIGAESHI			= 354,
	SC_UTSUSEMI				= 355,//#NJ_UTSUSEMI#
	SC_BUNSINJYUTSU			= 356,//#NJ_BUNSINJYUTSU#
	SC_SUITON				= 357,//#NJ_SUITON#
	SC_NEN					= 358,//#NJ_NEN#
	SC_AVOID				= 359,//#緊急回避#
	SC_CHANGE				= 360,//#メンタルチェンジ#
	SC_DEFENCE				= 361,//#ディフェンス#
	SC_BLOODLUST			= 362,//#ブラッドラスト#
	SC_FLEET				= 363,//#フリートムーブ#
	SC_SPEED				= 364,//#オーバードスピード#
	
	//startでは使えないresistをアイテム側で全てクリアするための物
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
	
	KN_CHARGEATK	=	1001,//#チャージアタック#
	CR_SHRINK		=	1002,//#シュリンク#
	AS_SONICACCEL	=	1003,//#ソニックアクセラレーション#
	AS_VENOMKNIFE	=	1004,//#ベナムナイフ#
	RG_CLOSECONFINE	=	1005,//#クローズコンファイン#
	WZ_SIGHTBLASTER	=	1006,//#サイトブラスター#
	SA_CREATECON	=	1007,//#エルレメンタルコンバータ製造#
	SA_ELEMENTWATER	=	1008,//#エルレメンタルチェンジ（水）#
	HT_PHANTASMIC	=	1009,//#ファンタスミックアロー#
	BA_PANGVOICE	=	1010,//#パンボイス#
	DC_WINKCHARM	=	1011,//#魅惑のウィンク#
	BS_UNFAIRLYTRICK=	1012,//#アンフェアリートリック#
	BS_GREED		=	1013,//#貪欲#
	PR_REDEMPTIO	=	1014,//#レデムプティオ#
	MO_KITRANSLATION=	1015,//#珍奇注入(振気注入)#
	MO_BALKYOUNG	=	1016,//#足頃(発勁)#
	SA_ELEMENTGROUND=	1017,
	SA_ELEMENTFIRE	=	1018,
	SA_ELEMENTWIND	=	1019,

	HM_SKILLBASE	=8001,
	HLIF_HEAL		=8001,//#治癒の手助け(ヒール)#
	HLIF_AVOID		=8002,//#緊急回避#
	HLIF_BRAIN		=8003,//=//#脳手術#
	HLIF_CHANGE		=8004,//#メンタルチェンジ#
	HAMI_CASTLE		=8005,//#キャストリング#
	HAMI_DEFENCE	=8006,//#ディフェンス#
	HAMI_SKIN		=8007,//#アダマンティウムスキン#
	HAMI_BLOODLUST	=8008,//#ブラッドラスト#
	HFLI_MOON		=8009,//#ムーンライト#
	HFLI_FLEET		=8010,//#フリートムーブ#
	HFLI_SPEED		=8011,//#オーバードスピード#
	HFLI_SBR44		=8012,//#S.B.R.44#
	HVAN_CAPRICE	=8013,//#カプリス#
	HVAN_CHAOTIC	=8014,//#カオティックベネディクション#
	HVAN_INSTRUCT	=8015,//#チェンジインストラクション#
	HVAN_EXPLOSION	=8016,//#バイオエクスプロージョン#

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
	//GD_CHARISMA, -> GD_GUARDUPに変更あり
	// GD_GURADUP,	// GD)CHARISMAと同ID
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

//状態アイコン
//厳密にはキャラクターの色の変化なども含まれている(爆裂波動など)
enum {
	SI_BLANK				= 43,//送っても空白のもの

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
	SI_EXPLOSIONSPIRITS		=86,	/* 爆裂波動 */
	SI_STEELBODY			=87,	/* 金剛エフェクト */
	SI_COMBO				=89,
	SI_FLAMELAUNCHER		=90,	/* フレイムランチャー */
	SI_FROSTWEAPON			=91,	/* フロストウェポン */
	SI_LIGHTNINGLOADER		=92,	/* ライトニングローダー */
	SI_SEISMICWEAPON		=93,	/* サイズミックウェポン */
	SI_AURABLADE			=103,	/* オーラブレード */
	SI_PARRYING				=104,	/* パリイング */
	SI_CONCENTRATION		=105,	/* コンセントレーション */
	SI_TENSIONRELAX			=106,	/* テンションリラックス */
	SI_BERSERK				=107,	/* バーサーク */
	SI_ASSUMPTIO			=110,	/* アシャンプティオ */
	SI_ELEMENTFIELD 		=112,	/* 属性場 */
	SI_MAGICPOWER			=113,	/* 魔法力増幅 */
	SI_EDP					=114, 	//
	SI_TRUESIGHT			=115,	/* トゥルーサイト */
	SI_WINDWALK				=116,	/* ウインドウォーク */
	SI_MELTDOWN				=117,	/* メルトダウン */
	SI_CARTBOOST			=118,	/* カートブースト */
	SI_CHASEWALK			=119,	/* チェイスウォーク */
	SI_REJECTSWORD			=120,	/* リジェクトソード */
	SI_MARIONETTE			=121,	/* マリオネットコントロール */
	SI_MARIONETTE2			=122,	/* マリオネットコントロール2 */
	SI_MOONLIT 				=123,	//月明かりの下で
	SI_HEADCRUSH			=124,	/* ヘッドクラッシュ */
	SI_JOINTBEAT			=125,	/* ジョイントビート */
	SI_BABY					=130,	//パパ、ママ、大好き
	SI_AUTOBERSERK			=132,	//オートバーサーク
	SI_READYSTORM			=135,	//旋風準備
	SI_READYDOWN			=137,	//下段準備
	SI_READYTURN			=139,	//回転準備
	SI_READYCOUNTER			=141,	//カウンター準備
	SI_DODGE				=143,	//落法
	SI_RUN					=145,	//駆け足
	SI_DARKELEMENT			=146,	//暗
	SI_ADRENALINE2			=147,	//フルアドレナリンラッシュ
	SI_ATTENELEMENT			=148,	//念
	SI_SOULLINK				=149,	//魂状態
	SI_DEVIL				=152,	//太陽と月と星の悪魔
	SI_KAITE				=153,	//カイト
	SI_KAIZEL				=156,	//カイゼル
	SI_KAAHI				=157,	//カアヒ
	SI_KAUPE				=158,	//カウプ
	SI_ONEHAND				=161,	//ワンハンドクイッケン
	SI_SUN_WARM				=165,	//太陽の温もり
	SI_MOON_WARM			=166,	//月の温もり
	SI_STAR_WARM			=167,	//星の温もり
	SI_SUN_COMFORT			=169,	//太陽の安楽
	SI_MOON_COMFORT			=170,	//月の安楽
	SI_STAR_COMFORT			=171,	//星の安楽
	SI_PRESERVE				=181,	//プリザーブ
	SI_CHASEWALK_STR		=182,	//チェイスウォークのSTR?
	SI_SPURT				=182,	//スパート状態にも使おう
	SI_DOUBLECASTING		=186,	//ダブルキャスティング
	SI_OVERTHRUSTMAX		=188,	//オーバートラストマックス
	SI_TAROTCARD			=191,
	SI_SHRINK				=197,	//#シュリンク#
	SI_SIGHTBLASTER			=198,	//#サイトブラスター#
	SI_CLOSECONFINE			=200,	//#クローズコンファイン#
	SI_CLOSECONFINE2		=201,	//#クローズコンファイン#
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

