#ifndef _UNIT_H_
#define _UNIT_H_

// PC, MOB, PET に共通する処理を１つにまとめる計画

// 歩行開始
//     戻り値は、0 ( 成功 ), 1 ( 失敗 )
int unit_walktoxy( struct block_list *bl, int x, int y);

int unit_distance2( struct block_list *bl, struct block_list *bl2);
// 歩行停止
// typeは以下の組み合わせ : 
//     1: 位置情報の送信( この関数の後に位置情報を送信する場合は不要 )
//     2: ダメージディレイ有り
//     4: 不明(MOBのみ？)
int unit_stop_walking(struct block_list *bl,int type);

// 位置の強制移動(吹き飛ばしなど)
int unit_movepos(struct block_list *bl,int dst_x,int dst_y,int flag);
int unit_setdir(struct block_list *bl,int dir);
int unit_getdir(struct block_list *bl);

// そこまで歩行でたどり着けるかの判定
int unit_can_reach(struct block_list *bl,int x,int y);

// 攻撃関連
void unit_stopattack(struct block_list *bl);
int unit_attack(struct block_list *src,int target_id,int type);

// int unit_setpos( struct block_list *bl, const char* map, int x, int y);

// スキル使用
int unit_skilluse_id(struct block_list *src, int target_id, int skill_num, int skill_lv);
int unit_skilluse_pos(struct block_list *src, int skill_x, int skill_y, int skill_num, int skill_lv);

// スキル使用( 補正済みキャスト時間、キャンセル不可設定付き )
int unit_skilluse_id2(struct block_list *src, int target_id, int skill_num, int skill_lv, int casttime, int castcancel);
int unit_skilluse_pos2( struct block_list *src, int skill_x, int skill_y, int skill_num, int skill_lv, int casttime, int castcancel);

// 詠唱キャンセル
int unit_skillcastcancel(struct block_list *bl,int type);

int unit_counttargeted(struct block_list *bl,int target_lv);

// unit_data の初期化処理
int unit_dataset(struct block_list *bl);

int unit_heal(struct block_list *bl,int hp,int sp);
int unit_fixdamage(struct block_list *src,struct block_list *target,unsigned int tick,int sdelay,int ddelay,int damage,int div,int type,int damage2);
// その他
int unit_isdead(struct block_list *bl);
int unit_iscasting(struct block_list *bl);
int unit_iswalking(struct block_list *bl);

struct unit_data* unit_bl2ud(struct block_list *bl);
int unit_remove_map(struct block_list *bl, int clrtype);
int unit_distance(int x0,int y0,int x1,int y1);
int unit_free(struct block_list *bl, int clrtype);
int unit_changeviewsize(struct block_list *bl,short size);

// 初期化ルーチン
int do_init_unit(void);
int do_final_unit(void);

#endif /* _UNIT_H_ */
