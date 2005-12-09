#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "timer.h"
#include "socket.h"
#include "db.h"
#include "nullpo.h"
#include "malloc.h"
#include "map.h"
#include "clif.h"
#include "intif.h"
#include "pc.h"
#include "mob.h"
#include "guild.h"
#include "itemdb.h"
#include "skill.h"
#include "battle.h"
#include "party.h"
#include "npc.h"
#include "status.h"
#include "date.h"
#include "unit.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#ifdef _MSC_VER
	#define snprintf _snprintf
#endif

#define MIN_MOBTHINKTIME 100

#define MOB_LAZYMOVEPERC 50		// 手抜きモードMOBの移動確率（千分率）
#define MOB_LAZYWARPPERC 20		// 手抜きモードMOBのワープ確率（千分率）

struct mob_db mob_db_real[MOB_ID_MAX-MOB_ID_MIN+1];
struct mob_db *mob_db = &mob_db_real[-MOB_ID_MIN];

/*	簡易設定	*/
#define CLASSCHANGE_BOSS_NUM 21

/*==========================================
 * ローカルプロトタイプ宣言 (必要な物のみ)
 *------------------------------------------
 */
static int mob_makedummymobdb(int);
int mobskill_use(struct mob_data *md,unsigned int tick,int event);
int mobskill_deltimer(struct mob_data *md );
int mob_skillid2skillidx(int class,int skillid);
int mobskill_use_id(struct mob_data *md,struct block_list *target,int skill_idx);
int mob_unlocktarget(struct mob_data *md,int tick);

/*==========================================
 * mobを名前で検索
 *------------------------------------------
 */
int mobdb_searchname(const char *str)
{
	int i;

	for(i=MOB_ID_MIN;i<MOB_ID_MAX;i++){
		if( strcmpi(mob_db[i].name,str)==0 || strcmp(mob_db[i].jname,str)==0 ||
			memcmp(mob_db[i].name,str,24)==0 || memcmp(mob_db[i].jname,str,24)==0)
			return i;
	}
	return 0;
}
/*==========================================
 * MOB出現用の最低限のデータセット
 *------------------------------------------
 */
int mob_spawn_dataset(struct mob_data *md,const char *mobname,int class)
{
	nullpo_retr(0, md);

	md->bl.type=BL_MOB;
	md->bl.prev=NULL;
	md->bl.next=NULL;
	if(strcmp(mobname,"--en--")==0)
		memcpy(md->name,mob_db[class].name,24);
	else if(strcmp(mobname,"--ja--")==0)
		memcpy(md->name,mob_db[class].jname,24);
	else
		memcpy(md->name,mobname,24);

	md->n = 0;
	md->base_class = md->class = class;
	md->bl.id= npc_get_new_npc_id();

	memset(&md->state,0,sizeof(md->state));
	md->target_id=0;
	md->attacked_id=0;
	md->guild_id=0;
	md->speed=mob_db[class].speed;

	unit_dataset( &md->bl );

	return 0;
}


/*==========================================
 * 一発MOB出現(スクリプト用)
 *------------------------------------------
 */
int mob_once_spawn(struct map_session_data *sd,char *mapname,
	int x,int y,const char *mobname,int class,int amount,const char *event)
{
	struct mob_data *md=NULL;
	int m,count,lv=255,r=class;

	if( sd )
		lv=sd->status.base_level;

	if( sd && strcmp(mapname,"this")==0)
		m=sd->bl.m;
	else
		m=map_mapname2mapid(mapname);

	if(m<0 || amount<=0 || (class>=0 && class<=1000) || class>MOB_ID_MAX)	// 値が異常なら召喚を止める
		return 0;

	if(class<0){	// ランダムに召喚
		int i=0;
		int j=-class-1;
		int k;
		if(j>=0 && j<MAX_RANDOMMONSTER){
			do{
				class=rand()%1000+1001;
				k=rand()%1000000;
			}while((mob_db[class].max_hp <= 0 || mob_db[class].summonper[j] <= k ||
				 (lv<mob_db[class].lv && battle_config.random_monster_checklv)) && (i++) < MOB_ID_MAX);
			if(i>=MOB_ID_MAX){
				class=mob_db[MOB_ID_MIN].summonper[j];
			}
		}else{
			return 0;
		}
//		if(battle_config.etc_log)
//			printf("mobclass=%d try=%d\n",class,i);
	}
	if(sd){
		if(x<=0) x=sd->bl.x;
		if(y<=0) y=sd->bl.y;
	}else if(x<=0 || y<=0){
		printf("mob_once_spawn: ??\n");
	}

	for(count=0;count<amount;count++){
		md=(struct mob_data *)aCalloc(1,sizeof(struct mob_data));
		memset(md, '\0', sizeof *md);
		if(mob_db[class].mode&0x02)
			md->lootitem=(struct item *)aCalloc(LOOTITEM_SIZE,sizeof(struct item));
		else
			md->lootitem=NULL;

		mob_spawn_dataset(md,mobname,class);
		md->bl.m=m;
		md->bl.x=x;
		md->bl.y=y;
		if(r<0&&battle_config.dead_branch_active) md->mode=mob_db[class].mode|(0x1+0x4+0x80); //移動してアクティブで反撃する
		md->m =m;
		md->x0=x;
		md->y0=y;
		md->xs=0;
		md->ys=0;
		md->spawndelay1=-1;	// 一度のみフラグ
		md->spawndelay2=-1;	// 一度のみフラグ
		md->state.nodrop=0;
		md->state.noexp=0;
		md->state.nomvp=0;

		memcpy(md->npc_event,event,sizeof(md->npc_event));

		md->bl.type=BL_MOB;
		map_addiddb(&md->bl);
		mob_spawn(md->bl.id);

	}
	return (amount>0)?md->bl.id:0;
}
/*==========================================
 * 一発MOB出現(スクリプト用＆エリア指定)
 *------------------------------------------
 */
int mob_once_spawn_area(struct map_session_data *sd,char *mapname,
	int x0,int y0,int x1,int y1,
	const char *mobname,int class,int amount,const char *event)
{
	int x,y,i,max,lx=-1,ly=-1,id=0;
	int m;

	m=map_mapname2mapid(mapname);

	max=(y1-y0+1)*(x1-x0+1)*3;
	if(x0<=0 && y0<=0) max=50;	//mob_spawnに倣って50
	if(max>1000)max=1000;

	if(m<0 || amount<=0 || (class>=0 && class<=1000) || class>MOB_ID_MAX)	// 値が異常なら召喚を止める
		return 0;

	for(i=0;i<amount;i++){
		int j=0;
		do{
			if(x0<=0 && y0<=0){	//x0,y0が0以下のとき位置ランダム湧き
				x=rand()%(map[m].xs-2)+1;
				y=rand()%(map[m].ys-2)+1;
			} else {
				x=rand()%(x1-x0+1)+x0;
				y=rand()%(y1-y0+1)+y0;
			}
		} while(map_getcell(m,x,y,CELL_CHKNOPASS)&& (++j)<max);
		if(j>=max){
			if(lx>=0){	// 検索に失敗したので以前に沸いた場所を使う
				x=lx;
				y=ly;
			}else
				return 0;	// 最初に沸く場所の検索を失敗したのでやめる
		}
			if(x==0||y==0) printf("xory=0, x=%d,y=%d,x0=%d,y0=%d\n",x,y,x0,y0);
		id=mob_once_spawn(sd,mapname,x,y,mobname,class,1,event);
		lx=x;
		ly=y;
	}
	return id;
}
/*==========================================
 * mobの見かけ所得
 *------------------------------------------
 */
int mob_get_viewclass(int class)
{
	return mob_db[class].view_class;
}
int mob_get_sex(int class)
{
	return mob_db[class].sex;
}
short mob_get_hair(int class)
{
	return mob_db[class].hair;
}
short mob_get_hair_color(int class)
{
	return mob_db[class].hair_color;
}
short mob_get_weapon(int class)
{
	return mob_db[class].weapon;
}
short mob_get_shield(int class)
{
	return mob_db[class].shield;
}
short mob_get_head_top(int class)
{
	return mob_db[class].head_top;
}
short mob_get_head_mid(int class)
{
	return mob_db[class].head_mid;
}
short mob_get_head_buttom(int class)
{
	return mob_db[class].head_buttom;
}

/*==========================================
 * MOBが現在移動可能な状態にあるかどうか
 *------------------------------------------
 */
int mob_can_move(struct mob_data *md)
{
	nullpo_retr(0, md);

	if(md->ud.canmove_tick > gettick() || (md->opt1 > 0 && md->opt1 != 6) || md->option&2)
		return 0;
	// アンクル中で動けないとか
	if( md->sc_data[SC_ANKLE].timer != -1 || //アンクルスネア
		md->sc_data[SC_AUTOCOUNTER].timer != -1 || //オートカウンター
		md->sc_data[SC_BLADESTOP].timer != -1 || //白刃取り
		md->sc_data[SC_CLOSECONFINE].timer!=-1 ||//
		md->sc_data[SC_HOLDWEB].timer!=-1 ||//ホールドウェブ
		md->sc_data[SC_SPIDERWEB].timer != -1  ||//スパイダーウェッブ
		//md->sc_data[SC_RUN].timer != -1		 || //駆け足
		//md->sc_data[SC_HIGHJUMP].timer != -1 ||	//ハイジャンプ
		md->sc_data[SC_GRAVITATION_USER].timer != -1 ||	//グラビテーションフィールド使用者
		(battle_config.hermode_no_walking && md->sc_data[SC_DANCING].timer !=-1 && md->sc_data[SC_DANCING].val1 == CG_HERMODE) ||
		(md->sc_data[SC_DANCING].timer !=-1 && md->sc_data[SC_DANCING].val1>=BD_LULLABY && md->sc_data[SC_DANCING].val1<=BD_SIEGFRIED) ||
		(md->sc_data[SC_DANCING].timer !=-1 && md->sc_data[SC_DANCING].val1 == CG_MOONLIT)
	)
		return 0;

	return 1;
}

/*==========================================
 * delay付きmob spawn (timer関数)
 *------------------------------------------
 */
int mob_delayspawn(int tid,unsigned int tick,int m,int n)
{
	mob_spawn(m);
	return 0;
}

/*==========================================
 * mob出現。色々初期化もここで
 *------------------------------------------
 */
int mob_spawn(int id)
{
	int x=0,y=0,i=0,c;
	unsigned int tick = gettick();
	struct mob_data *md;
	struct block_list *bl;

	nullpo_retr(-1, bl=map_id2bl(id));
	nullpo_retr(-1, md=(struct mob_data*)bl);

	if(md->bl.type!=BL_MOB)
		return -1;

	md->last_spawntime=tick;
	if( md->bl.prev!=NULL ){
//		clif_clearchar_area(&md->bl,3);
//		skill_unit_move(&md->bl,tick,0);  // sc_dataは初期化される為必要ない
		map_delblock(&md->bl);
	}
	else{
		if(md->class != md->base_class){	// クラスチェンジしたMob
			memcpy(md->name,mob_db[md->base_class].jname,24);
			md->speed=mob_db[md->base_class].speed;
		}
		md->class = md->base_class;
	}

	md->bl.m =md->m;
	do {
		if(md->x0==0 && md->y0==0){
			x=rand()%(map[md->bl.m].xs-2)+1;
			y=rand()%(map[md->bl.m].ys-2)+1;
		} else {
			x=md->x0+rand()%(md->xs+1)-md->xs/2;
			y=md->y0+rand()%(md->ys+1)-md->ys/2;
		}
		i++;
	} while(map_getcell(md->bl.m,x,y,CELL_CHKNOPASS)&& i<50);

	if(i>=50){
		if(md->spawndelay1==-1 && md->spawndelay2==-1){
			// 一度しか沸かないのは、スタック位置でも出す
			if(map_getcell(md->bl.m,x,y,CELL_GETTYPE)!=5)
				return 1;
		} else {
	//		if(battle_config.error_log)
	//			printf("MOB spawn error %d @ %s\n",id,map[md->bl.m].name);
			add_timer(tick+5000,mob_delayspawn,id,0);
			return 1;
		}
	}

	md->ud.to_x=md->bl.x=x;
	md->ud.to_y=md->bl.y=y;
	md->dir=0;

	memset(&md->state,0,sizeof(md->state));
	md->attacked_id = 0;
	md->target_id = 0;
	md->move_fail_count = 0;

	if(!md->speed)
		md->speed = mob_db[md->class].speed;
	md->def_ele = mob_db[md->class].element;
	md->master_id=0;
	md->master_dist=0;

	md->state.skillstate = MSS_IDLE;
	md->last_thinktime = tick;
	md->next_walktime = tick+rand()%50+5000;

	md->state.nodrop=0;
	md->state.noexp=0;
	md->state.nomvp=0;
//	md->guild_id = 0;
	if(md->class == 1288 || md->class == 1287 || md->class == 1286 || md->class == 1285) {
		struct guild_castle *gc=guild_mapname2gc(map[md->bl.m].name);
		if(gc)
			md->guild_id = gc->guild_id;
	}

	md->deletetimer=-1;

	for(i=0,c=tick-1000*3600*10;i<MAX_MOBSKILL;i++)
		md->skilldelay[i] = c;

	if(md->lootitem)
		memset(md->lootitem,0,sizeof(md->lootitem));
	md->lootitem_count = 0;

	for(i=0;i<MAX_STATUSCHANGE;i++) {
		md->sc_data[i].timer=-1;
		md->sc_data[i].val1 = md->sc_data[i].val2 = md->sc_data[i].val3 = md->sc_data[i].val4 =0;
	}
	md->sc_count=0;
	md->opt1=md->opt2=md->opt3=md->option=0;

	md->hp = status_get_max_hp(&md->bl);
	if(md->hp<=0){
		mob_makedummymobdb(md->class);
		md->hp = status_get_max_hp(&md->bl);
	}
	unit_dataset( &md->bl );

	map_addblock(&md->bl);
	skill_unit_move(&md->bl,tick,1);	// sc_data初期化後の必要がある

	clif_spawnmob(md);

	return 0;
}

/*==========================================
 * 指定IDの存在場所への到達可能性
 *------------------------------------------
 */
int mob_can_reach(struct mob_data *md,struct block_list *bl,int range)
{
	int dx,dy;
	struct walkpath_data wpd;
	int i;

	nullpo_retr(0, md);
	nullpo_retr(0, bl);

	dx=abs(bl->x - md->bl.x);
	dy=abs(bl->y - md->bl.y);

	//=========== guildcastle guardian no search start===========
	//when players are the guild castle member not attack them !
	if(md->class == 1285 || md->class == 1286 || md->class == 1287) {
		struct map_session_data *sd;
		struct guild *g;
		struct guild_castle *gc=guild_mapname2gc(map[bl->m].name);
		if(bl->type == BL_PC){
			nullpo_retr(0, sd=(struct map_session_data *)bl);
			if(!gc)
				return 0;
			g=guild_search(sd->status.guild_id);
			if((g && g->guild_id == gc->guild_id) || !map[bl->m].flag.gvg)
				return 0;
		}
	}
	//========== guildcastle guardian no search eof==============
	else if(md->guild_id > 0){
		struct map_session_data *sd;
		if(bl->type == BL_PC){
			nullpo_retr(0, sd=(struct map_session_data *)bl);
			if(md->guild_id == sd->status.guild_id)
				return 0;
		}
	}

	if( md->bl.m != bl-> m)	// 違うマップ
		return 0;

	if( range>0 && range < ((dx>dy)?dx:dy) )	// 遠すぎる
		return 0;

	if( md->bl.x==bl->x && md->bl.y==bl->y )	// 同じマス
		return 1;

	if(mob_db[md->class].range > 6)				// 遠距離攻撃を許可
		return 1;

	// 障害物判定
	wpd.path_len=0;
	wpd.path_pos=0;
	wpd.path_half=0;
	if(path_search(&wpd,md->bl.m,md->bl.x,md->bl.y,bl->x,bl->y,0)!=-1
	 && wpd.path_len<=AREA_SIZE)
		return 1;

	if(bl->type!=BL_PC && bl->type!=BL_MOB)
		return 0;

	// 隣接可能かどうか判定
	dx=(dx>0)?1:((dx<0)?-1:0);
	dy=(dy>0)?1:((dy<0)?-1:0);
	if(path_search(&wpd,md->bl.m,md->bl.x,md->bl.y,bl->x-dx,bl->y-dy,0)!=-1
	 && wpd.path_len<=AREA_SIZE)
		return 1;
	for(i=0;i<9;i++){
		if(path_search(&wpd,md->bl.m,md->bl.x,md->bl.y,bl->x-1+i/3,bl->y-1+i%3,0)!=-1
		 && wpd.path_len<=AREA_SIZE)
			return 1;
	}
	return 0;
}

/*==========================================
 * モンスターの攻撃対象決定
 *------------------------------------------
 */
int mob_target(struct mob_data *md,struct block_list *bl,int dist)
{
	struct map_session_data *sd;
	struct status_change *sc_data;
	short *option;
	int mode,race;

	nullpo_retr(0, md);
	nullpo_retr(0, bl);

	sc_data = status_get_sc_data(bl);
	option  = status_get_option(bl);
	race    = status_get_race(&md->bl);
	mode    = status_get_mode(&md->bl);

	if(!(mode&0x80)) {
		md->target_id = 0;
		return 0;
	}
	// タゲ済みでタゲを変える気がないなら何もしない
	if( md->target_id > 0 && ( !(mode&0x04) || rand()%100>25) )
		return 0;

	if(mode&0x20 ||	// MVPMOBなら強制
		(sc_data && sc_data[SC_TRICKDEAD].timer == -1 && sc_data[SC_HIGHJUMP].timer == -1 && md->sc_data[SC_WINKCHARM].timer == -1 &&
		 ( (option && !(*option&0x06) ) || race==4 || race==6) ) ){
		if(bl->type == BL_PC) {
			nullpo_retr(0, sd = (struct map_session_data *)bl);
			if(sd->invincible_timer != -1 || pc_isinvisible(sd))
				return 0;
			if(!(mode&0x20) && race!=4 && race!=6 && sd->state.gangsterparadise)
				return 0;
		}

		if(bl->type == BL_PC || bl->type == BL_MOB) 
			md->target_id=bl->id;	// 妨害がなかったのでロック
		md->min_chase=dist+13;
		if(md->min_chase>26)
			md->min_chase=26;
	}
	return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
int mob_attacktarget(struct mob_data *md,struct block_list *target,int flag)
{
	nullpo_retr(0, md);
	nullpo_retr(0, target);
	if(flag==0 && md->target_id)
		return 0;
	if( !mob_can_move(md) )	// 動けない状態にある
			return 0;
	unit_stopattack(&md->bl);
	mob_target(md,target,unit_distance(md->bl.x,md->bl.y,target->x,target->y));
	//md->state.skillstate=MSS_CHASE;	// 突撃時スキル
	return 0;
}

/*==========================================
 * 策敵ルーティン
 *------------------------------------------
 */
static int mob_ai_sub_hard_search(struct block_list *bl,va_list ap)
{
	struct map_session_data *tsd=NULL;
	struct mob_data  *smd,*tmd=NULL;
	int mode,race,dist,range,*cnt,flag;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, smd=va_arg(ap,struct mob_data *));
	nullpo_retr(0, cnt=va_arg(ap,int *));
	nullpo_retr(0, flag=va_arg(ap,int));
	BL_CAST( BL_PC ,  bl, tsd);
	BL_CAST( BL_MOB,  bl, tmd);
	if( smd->bl.id == bl->id ) return 0; // self
	mode = status_get_mode( &smd->bl );
	race = status_get_race( &smd->bl );
	dist = unit_distance(smd->bl.x,smd->bl.y,bl->x,bl->y);
	range = (smd->sc_data[SC_BLIND].timer != -1 || smd->sc_data[SC_FOGWALLPENALTY].timer!=-1)?1:10;

	// アクティブ
	if( (flag & 1) && dist<=range && battle_check_target(&smd->bl,bl,BCT_ENEMY)>=1) {
		int active_flag = 0;
		// ターゲット射程内にいるなら、ロックする
		if(tsd && !unit_isdead(&tsd->bl) && tsd->invincible_timer == -1 && !pc_isinvisible(tsd) ) {
			// 妨害がないか判定
			if(mode&0x20) { active_flag = 1; }
			if( tsd->sc_data[SC_TRICKDEAD].timer == -1 && tsd->sc_data[SC_HIGHJUMP].timer==-1 &&
				smd->sc_data[SC_WINKCHARM].timer == -1
			) {
				if((!pc_ishiding(tsd) && !tsd->state.gangsterparadise) || race==4 || race==6) {
					active_flag = 1;
				}
			}
		} else if(tmd && dist<=range) {
			active_flag = 1;
		}
		if(active_flag) {
			if(mob_can_reach(smd,bl,range) && rand()%1000<1000/(++cnt[0]) ) {	// 範囲内PCで等確率にする
				smd->target_id=bl->id;
				smd->min_chase=13;
			}
		}
	}
	// ルート
	if( (flag & 2) && bl->type == BL_ITEM && dist<9) {
		if( mob_can_reach(smd,bl,12) && rand()%1000<1000/(++cnt[1]) ){	// 範囲内アイテムで等確率にする
			smd->target_id=bl->id;
			smd->min_chase=13;
		}
	}
	// リンク
	if( (flag & 4) && tmd && tmd->class==smd->class && !tmd->target_id && dist<13) {
		tmd->target_id = smd->target_id;
		tmd->min_chase = 13;
	}

	return 0;
}
/*==========================================
 * 取り巻きモンスターの処理
 *------------------------------------------
 */
static int mob_ai_sub_hard_slavemob(struct mob_data *md,unsigned int tick)
{
	struct mob_data *mmd=NULL;
	struct block_list *bl;
	int mode,race,old_dist;

	nullpo_retr(0, md);

	if((bl=map_id2bl(md->master_id)) != NULL )
		mmd=(struct mob_data *)bl;//主の情報

	if(!bl || status_get_hp(bl)<=0){	//主が死亡しているか見つからない
		if(md->state.special_mob_ai>0)
			unit_remove_map(&md->bl,3);
		else
			unit_remove_map(&md->bl,1);
		return 0;
	}
	if(md->state.special_mob_ai>0)		// 主がPCの場合は、以降の処理は要らない
		return 0;

	mode = status_get_mode( &md->bl );

	// 主ではない
	if(!mmd || mmd->bl.type!=BL_MOB || mmd->bl.id!=md->master_id)
		return 0;
	// 呼び戻し
	if(mmd->recall_flag == 1){
		if(mmd->recallcount < (mmd->recallmob_count+2) ){
			int dx,dy;
			dx=rand()%9-4+mmd->bl.x;
			dy=rand()%9-4+mmd->bl.y;
			mob_warp(md,-1,dx,dy,3);
			mmd->recallcount += 1;
		}
		else{
			mmd->recall_flag = 0;
			mmd->recallcount=0;
		}
		md->state.master_check = 1;
		return 0;
	}
	// 主が違うマップにいるのでテレポートして追いかける
	if( mmd->bl.m != md->bl.m ){
		mob_warp(md,mmd->bl.m,mmd->bl.x,mmd->bl.y,3);
		md->state.master_check = 1;
		return 0;
	}

	// 主との距離を測る
	old_dist=md->master_dist;
	md->master_dist=unit_distance(md->bl.x,md->bl.y,mmd->bl.x,mmd->bl.y);

	// 直前まで主が近くにいたのでテレポートして追いかける
	if( old_dist<10 && md->master_dist>18){
		mob_warp(md,-1,mmd->bl.x,mmd->bl.y,3);
		md->state.master_check = 1;
		return 0;
	}

	// 主がいるが、少し遠いので近寄る
	if(!md->target_id && mob_can_move(md) &&
		(md->ud.walktimer == -1) && md->master_dist<15){
		int i=0,dx,dy,ret;
		if(md->master_dist>4) {
			do {
				if(i<=5){
					dx=mmd->bl.x - md->bl.x;
					dy=mmd->bl.y - md->bl.y;
					if(dx<0) dx+=(rand()%( (dx<-3)?3:-dx )+1);
					else if(dx>0) dx-=(rand()%( (dx>3)?3:dx )+1);
					if(dy<0) dy+=(rand()%( (dy<-3)?3:-dy )+1);
					else if(dy>0) dy-=(rand()%( (dy>3)?3:dy )+1);
				}else{
					dx=mmd->bl.x - md->bl.x + rand()%7 - 3;
					dy=mmd->bl.y - md->bl.y + rand()%7 - 3;
				}

				ret=unit_walktoxy(&md->bl,md->bl.x+dx,md->bl.y+dy);
				i++;
			} while(ret && i<10);
		}
		else {
			do {
				dx = rand()%9 - 5;
				dy = rand()%9 - 5;
				if( dx == 0 && dy == 0) {
					dx = (rand()%1)? 1:-1;
					dy = (rand()%1)? 1:-1;
				}
				dx += mmd->bl.x;
				dy += mmd->bl.y;

				ret=unit_walktoxy(&md->bl,mmd->bl.x+dx,mmd->bl.y+dy);
				i++;
			} while(ret && i<10);
		}

		md->next_walktime=tick+500;
		md->state.master_check = 1;
	}

	// 主がいて、主がロックしていて自分はロックしていない
	if( mmd->target_id>0 && !md->target_id){
		struct map_session_data *sd=map_id2sd(mmd->target_id);
		if(sd!=NULL && !unit_isdead(&sd->bl) && sd->invincible_timer == -1 && !pc_isinvisible(sd)){

			race=mob_db[md->class].race;
			if(mode&0x20 ||
				(sd->sc_data[SC_TRICKDEAD].timer == -1 && sd->sc_data[SC_HIGHJUMP].timer == -1 && mmd->sc_data[SC_WINKCHARM].timer == -1 &&
				( (!pc_ishiding(sd) && !sd->state.gangsterparadise) || race==4 || race==6) ) ){	// 妨害がないか判定

				md->target_id=sd->bl.id;
				md->min_chase=5+unit_distance(md->bl.x,md->bl.y,sd->bl.x,sd->bl.y);
				md->state.master_check = 1;
			}
		}
	}

	// 主がいて、主がロックしてなくて自分はロックしている
/*	if( md->target_id>0 && !mmd->target_id ){
		struct map_session_data *sd=map_id2sd(md->target_id);
		if(sd!=NULL && !unit_isdead(&sd->bl) && sd->invincible_timer == -1 && !pc_isinvisible(sd)){

			race=mob_db[mmd->class].race;
			if(mode&0x20 ||
				(sd->sc_data[SC_TRICKDEAD].timer == -1 &&
				(!(sd->status.option&0x06) || race==4 || race==6)
				) ){	// 妨害がないか判定

				mmd->target_id=sd->bl.id;
				mmd->min_chase=5+distance(mmd->bl.x,mmd->bl.y,sd->bl.x,sd->bl.y);
			}
		}
	}*/

	return 0;
}

/*==========================================
 * ロックを止めて待機状態に移る。
 *------------------------------------------
 */
int mob_unlocktarget(struct mob_data *md,int tick)
{
	nullpo_retr(0, md);

	md->target_id=0;
	md->ud.attacktarget_lv = 0;
	md->state.skillstate=MSS_IDLE;
	md->next_walktime=tick+rand()%3000+3000;
	return 0;
}
/*==========================================
 * ランダム歩行
 *------------------------------------------
 */
static int mob_randomwalk(struct mob_data *md,int tick)
{
	const int retrycount=20;
	int speed;

	nullpo_retr(0, md);

	speed=status_get_speed(&md->bl);
	if(DIFF_TICK(md->next_walktime,tick)<0){
		int i,x,y,c,d=12-md->move_fail_count;
		if(d<4) d=4;
		if(d>6) d=6;
		for(i=0;i<retrycount;i++){	// 移動できる場所の探索
			int r=rand();
			x=md->bl.x+r%(d*2+1)-d;
			y=md->bl.y+r/(d*2+1)%(d*2+1)-d;
			if((map_getcell(md->bl.m,x,y,CELL_CHKPASS)) && unit_walktoxy(&md->bl,x,y)==0){
				md->move_fail_count=0;
				break;
			}
			if(i+1>=retrycount){
				md->move_fail_count++;
				if(md->move_fail_count>1000){
					if(battle_config.error_log)
						printf("MOB cant move. random spawn %d, class = %d\n",md->bl.id,md->class);
					md->move_fail_count=0;
					mob_spawn(md->bl.id);
				}
			}
		}
		for(i=c=0;i<md->ud.walkpath.path_len;i++){	// 次の歩行開始時刻を計算
			if(md->ud.walkpath.path[i]&1)
				c+=speed*14/10;
			else
				c+=speed;
		}
		md->next_walktime = tick+rand()%3000+3000+c;
		md->state.skillstate=MSS_WALK;
		return 1;
	}
	return 0;
}

/*==========================================
 * PCが近くにいるMOBのAI
 *------------------------------------------
 */
static int mob_ai_sub_hard(struct block_list *bl,va_list ap)
{
	struct mob_data *md,*tmd=NULL;
	struct map_session_data *tsd=NULL;
	struct block_list *tbl=NULL;
	struct flooritem_data *fitem;
	unsigned int tick;
	int i,dx,dy,ret,dist;
	int attack_type=0;
	int mode, race, search_flag = 0;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, md=(struct mob_data*)bl);

	tick=va_arg(ap,unsigned int);


	if(DIFF_TICK(tick,md->last_thinktime)<MIN_MOBTHINKTIME)
		return 0;
	md->last_thinktime=tick;
	if(  md->ud.attacktimer != -1 || md->ud.skilltimer != -1)
		return 0;
	// 歩行中は３歩ごとにAI処理を行う
	if( md->ud.walktimer != -1 && md->ud.walkpath.path_pos <= 1)
		return 0;

	if( md->ud.skilltimer!=-1 || md->bl.prev==NULL ){	// スキル詠唱中か死亡中
		if(DIFF_TICK(tick,md->next_walktime)>MIN_MOBTHINKTIME)
			md->next_walktime=tick;
		return 0;
	}

	mode = status_get_mode( &md->bl );
	race = status_get_race( &md->bl );

	// 異常
	if((md->opt1 > 0 && md->opt1 != 6) || md->sc_data[SC_BLADESTOP].timer != -1)
		return 0;

	if(md->target_id > 0 && (!(mode&0x80) || md->sc_data[SC_BLIND].timer != -1 || md->sc_data[SC_FOGWALLPENALTY].timer!=-1) )
		md->target_id = 0;

	if( md->target_id == 0 )
		md->ud.attacktarget_lv = 0;

	// まず攻撃されたか確認（アクティブなら25%の確率でターゲット変更）
	if( mode>0 && md->attacked_id>0 && (!md->target_id || (mode&0x04 && rand()%100<25 ) ) ){
		struct block_list *abl=map_id2bl(md->attacked_id);
		struct map_session_data *asd=NULL;
		md->attacked_players = 0;
		if(abl){
			if(abl->type==BL_PC)
				asd=(struct map_session_data *)abl;
			if(asd==NULL || md->bl.m != abl->m || abl->prev == NULL || asd->invincible_timer != -1 || pc_isinvisible(asd) ||
				(dist=unit_distance(md->bl.x,md->bl.y,abl->x,abl->y))>=32 || battle_check_target(bl,abl,BCT_ENEMY)==0)
				md->attacked_id=0;
			else {
				//距離が遠い場合はタゲを変更しない
				if (!md->target_id || (unit_distance(md->bl.x,md->bl.y,abl->x,abl->y)<3)) {
					md->target_id=md->attacked_id; // set target
					attack_type = 1;
					md->attacked_id=0;
					md->min_chase=dist+13;
					if(md->min_chase>26)
						md->min_chase=26;
				}
			}
		}
	}

	md->state.master_check = 0;
	// 取り巻きモンスターの処理
	if( md->master_id > 0) {
		mob_ai_sub_hard_slavemob(md,tick);
		if(md->bl.prev == NULL) return 0; // 親と同時に死んだ
	}

	// リンク / アクティヴ / ルートモンスターの検索
	// アクティヴ判定
	if( !md->target_id && mode&0x04 && !md->state.master_check && battle_config.monster_active_enable){
		search_flag |= 1;
	}

	// ルートモンスターのアイテムサーチ
	if( !md->target_id && mode&0x02 && !md->state.master_check){
		if(md->lootitem && (battle_config.monster_loot_type == 0 || md->lootitem_count < LOOTITEM_SIZE) ) {
			search_flag |= 2;
		}
	}

	// リンク判定
	if(md->target_id > 0 && mode&0x08 && md->ud.attacktarget_lv > 0) { 
		struct map_session_data *asd = map_id2sd(md->target_id);
		if(asd && asd->invincible_timer == -1 && !pc_isinvisible(asd)){
			search_flag |= 4;
		}
	}

	if( search_flag ) {
		int count[3] = {0, 0, 0};
		// 射程がアクティブ(12), ルート(12), リンク(12) なので、
		// 周囲AREA_SIZE以上の範囲を詮索する必要は無い
		map_foreachinarea(mob_ai_sub_hard_search,md->bl.m,
						  md->bl.x-AREA_SIZE,md->bl.y-AREA_SIZE,
						  md->bl.x+AREA_SIZE,md->bl.y+AREA_SIZE,
						  0,md,count,search_flag);
	}

	if(
		md->target_id == 0 || (tbl=map_id2bl(md->target_id)) == NULL ||
		tbl->m != md->bl.m || tbl->prev == NULL || 
		(dist=unit_distance(md->bl.x,md->bl.y,tbl->x,tbl->y)) >= md->min_chase
	) {
		// 対象が居ない / どこかに消えた / 視界外
		if(md->target_id > 0){
			mob_unlocktarget(md,tick);
			if(md->ud.walktimer != -1)
				unit_stop_walking(&md->bl,5);	// 歩行中なら停止
			return 0;
		}
	} else if(tbl->type==BL_PC || tbl->type==BL_MOB) {
		// 対象がPCもしくはMOB
		if(tbl->type==BL_PC)
			tsd=(struct map_session_data *)tbl;
		else if(tbl->type==BL_MOB)
			tmd=(struct mob_data *)tbl;
		if( tsd && !(mode&0x20) && (tsd->sc_data[SC_TRICKDEAD].timer != -1 || tsd->sc_data[SC_HIGHJUMP].timer!=-1 
			 || md->sc_data[SC_WINKCHARM].timer != -1 || ((pc_ishiding(tsd) || tsd->state.gangsterparadise) && race!=4 && race!=6)) )
			mob_unlocktarget(md,tick);	// スキルなどによる策敵妨害
		else if(!battle_check_range(&md->bl,tbl,mob_db[md->class].range)){
			// 攻撃範囲外なので移動
			if(!(mode&1)){	// 移動しないモード
				mob_unlocktarget(md,tick);
				return 0;
			}
			if( !mob_can_move(md) )	// 動けない状態にある
				return 0;
			md->state.skillstate=MSS_CHASE;	// 突撃時スキル
			mobskill_use(md,tick,-1);
			if(md->ud.walktimer != -1 && unit_distance(md->ud.to_x,md->ud.to_y,tbl->x,tbl->y)<2)
				return 0; // 既に移動中
			if( !mob_can_reach(md,tbl,(md->min_chase>13)?md->min_chase:13) )
				mob_unlocktarget(md,tick);	// 移動できないのでタゲ解除（IWとか？）
			else {
				// 追跡
				i=0;
				do {
					if(i==0) {	// 最初はAEGISと同じ方法で検索
						dx=tbl->x - md->bl.x; dy=tbl->y - md->bl.y;
						if(dx<0) dx++; else if(dx>0) dx--;
						if(dy<0) dy++; else if(dy>0) dy--;
					}else{	// だめならAthena式(ランダム)
						dx=tbl->x - md->bl.x + rand()%3 - 1;
						dy=tbl->y - md->bl.y + rand()%3 - 1;
					}
					ret=unit_walktoxy(&md->bl,md->bl.x+dx,md->bl.y+dy);
					i++;
				} while(ret && i<5);
				if(ret){ // 移動不可能な所からの攻撃なら2歩下る
					if(dx<0) dx=2; else if(dx>0) dx=-2;
					if(dy<0) dy=2; else if(dy>0) dy=-2;
					unit_walktoxy(&md->bl,md->bl.x+dx,md->bl.y+dy);
				}
			}
		} else { // 攻撃射程範囲内
			md->state.skillstate=MSS_ATTACK;
			if(md->ud.walktimer != -1)
				unit_stop_walking(&md->bl,1);	// 歩行中なら停止
			if(md->ud.attacktimer != -1 || md->ud.canact_tick > gettick())
				return 0; // 既に攻撃中
			unit_attack( &md->bl, md->target_id, attack_type);
		}
		return 0;
	} else if(tbl->type == BL_ITEM && md->lootitem) {
		if(dist > 0) {
			// アイテムまで移動
			if(!(mode&1)){	// 移動不可モンスター
				mob_unlocktarget(md,tick);
				return 0;
			}
			if( !mob_can_move(md) )	// 動けない状態にある
				return 0;
			md->state.skillstate=MSS_LOOT;	// ルート時スキル使用
			mobskill_use(md,tick,-1);
			if(md->ud.walktimer != -1 && unit_distance(md->ud.to_x,md->ud.to_y,tbl->x,tbl->y) <= 0)
				return 0; // 既に移動中
			ret=unit_walktoxy(&md->bl,tbl->x,tbl->y);
			if(ret)
				mob_unlocktarget(md,tick);// 移動できないのでタゲ解除（IWとか？）
		} else {
			// アイテムまでたどり着いた
			if(md->ud.attacktimer != -1)
				return 0; // 攻撃中
			if(md->ud.walktimer != -1)
				unit_stop_walking(&md->bl,1);	// 歩行中なら停止
			fitem = (struct flooritem_data *)tbl;
			if(md->lootitem_count < LOOTITEM_SIZE)
				memcpy(&md->lootitem[md->lootitem_count++],&fitem->item_data,sizeof(md->lootitem[0]));
			else if(battle_config.monster_loot_type == 1 && md->lootitem_count >= LOOTITEM_SIZE) {
				mob_unlocktarget(md,tick);
				return 0;
			}
			else {
				if(md->lootitem[0].card[0] == (short)0xff00)
					intif_delete_petdata(*((long *)(&md->lootitem[0].card[1])));
				for(i=0;i<LOOTITEM_SIZE-1;i++)
					memcpy(&md->lootitem[i],&md->lootitem[i+1],sizeof(md->lootitem[0]));
				memcpy(&md->lootitem[LOOTITEM_SIZE-1],&fitem->item_data,sizeof(md->lootitem[0]));
			}
			map_clearflooritem(tbl->id);
			mob_unlocktarget(md,tick);
		}
		return 0;
	} else {
		// 攻撃対象以外をターゲッティングした（バグ
		if(md->target_id > 0){
			mob_unlocktarget(md,tick);
			if(md->ud.walktimer != -1)
				unit_stop_walking(&md->bl,5);	// 歩行中なら停止
			return 0;
		}
	}

	// 歩行時/待機時スキル使用
	if( mobskill_use(md,tick,-1) )
		return 0;

	// 歩行処理
	if( mode&1 && mob_can_move(md) &&	// 移動可能MOB&動ける状態にある
		(md->master_id==0 || md->state.special_mob_ai || md->master_dist>10) ){	//取り巻きMOBじゃない

		if( DIFF_TICK(md->next_walktime,tick) > + 7000 && md->ud.walktimer == -1 ){
			md->next_walktime = tick + rand()%2000 + 1000;
		}

		// ランダム移動
		if( mob_randomwalk(md,tick) )
			return 0;
	}

	// 歩き終わってるので待機
	if( md->ud.walktimer == -1 )
		md->state.skillstate=MSS_IDLE;
	return 0;
}

/*==========================================
 * PC視界内のmob用まじめ処理(foreachclient)
 *------------------------------------------
 */
static int mob_ai_sub_foreachclient(struct map_session_data *sd,va_list ap)
{
	unsigned int tick;
	nullpo_retr(0, sd);
	nullpo_retr(0, ap);

	tick=va_arg(ap,unsigned int);
	map_foreachinarea(mob_ai_sub_hard,sd->bl.m,
					  sd->bl.x-AREA_SIZE*2,sd->bl.y-AREA_SIZE*2,
					  sd->bl.x+AREA_SIZE*2,sd->bl.y+AREA_SIZE*2,
					  BL_MOB,tick);

	return 0;
}

/*==========================================
 * PC視界内のmob用まじめ処理 (interval timer関数)
 *------------------------------------------
 */
static int mob_ai_hard(int tid,unsigned int tick,int id,int data)
{
	clif_foreachclient(mob_ai_sub_foreachclient,tick);

	return 0;
}

/*==========================================
 * 手抜きモードMOB AI（近くにPCがいない）
 *------------------------------------------
 */
static int mob_ai_sub_lazy(void * key,void * data,va_list ap)
{
	struct mob_data *md=data;
	unsigned int tick;
	struct mob_data *mmd=NULL;
	struct block_list *bl;

	nullpo_retr(0, md);

	if(md->bl.type!=BL_MOB)
		return 0;
	if((bl=map_id2bl(md->master_id)) != NULL && bl->type == BL_MOB)
		mmd=(struct mob_data *)bl;//自分のBOSSの情報

	tick=va_arg(ap,unsigned int);

	if(DIFF_TICK(tick,md->last_thinktime)<MIN_MOBTHINKTIME*10)
		return 0;
	md->last_thinktime=tick;

	if(md->bl.prev==NULL || md->ud.skilltimer!=-1){
		if(DIFF_TICK(tick,md->next_walktime)>MIN_MOBTHINKTIME*10)
			md->next_walktime=tick;
		return 0;
	}

	// 取り巻きモンスターの処理（呼び戻しされた時）
	if( md->master_id > 0 && md->state.special_mob_ai==0 && mmd && mmd->recall_flag == 1){
		mob_ai_sub_hard_slavemob(md,tick);
		return 0;
	}

	if(DIFF_TICK(md->next_walktime,tick)<0 &&
		(mob_db[md->class].mode&1) && mob_can_move(md) ){

		if( map[md->bl.m].users>0 ){
			// 同じマップにPCがいるので、少しましな手抜き処理をする

			// 時々移動する
			if(rand()%1000<MOB_LAZYMOVEPERC)
				mob_randomwalk(md,tick);

			// 召喚MOBでなく、BOSSでもないMOBは時々、沸きなおす
			else if( rand()%1000<MOB_LAZYWARPPERC && md->x0<=0 && md->master_id!=0 &&
				mob_db[md->class].mexp <= 0 && !(mob_db[md->class].mode & 0x20))
				mob_spawn(md->bl.id);

		}else{
			// 同じマップにすらPCがいないので、とっても適当な処理をする

			// 召喚MOBでない、BOSSでもないMOBは場合、時々ワープする
			if( rand()%1000<MOB_LAZYWARPPERC && md->x0<=0 && md->master_id!=0 &&
				mob_db[md->class].mexp <= 0 && !(mob_db[md->class].mode & 0x20))
				mob_warp(md,-1,-1,-1,-1);
		}

		md->next_walktime = tick+rand()%10000+5000;
	}
	return 0;
}

/*==========================================
 * PC視界外のmob用手抜き処理 (interval timer関数)
 *------------------------------------------
 */
static int mob_ai_lazy(int tid,unsigned int tick,int id,int data)
{
	map_foreachiddb(mob_ai_sub_lazy,tick);

	return 0;
}


/*==========================================
 * delay付きitem drop用構造体
 * timer関数に渡せるのint 2つだけなので
 * この構造体にデータを入れて渡す
 *------------------------------------------
 */
struct delay_item_drop {
	int m,x,y;
	int nameid,amount;
	struct map_session_data *first_sd,*second_sd,*third_sd;
};

struct delay_item_drop2 {
	int m,x,y;
	struct item item_data;
	struct map_session_data *first_sd,*second_sd,*third_sd;
};

/*==========================================
 * delay付きitem drop (timer関数)
 *------------------------------------------
 */
static int mob_delay_item_drop(int tid,unsigned int tick,int id,int data)
{
	struct delay_item_drop *ditem;
	struct item temp_item;
	int flag;

	nullpo_retr(0, ditem=(struct delay_item_drop *)id);

	memset(&temp_item,0,sizeof(temp_item));
	temp_item.nameid = ditem->nameid;
	temp_item.amount = ditem->amount;
	temp_item.identify = !itemdb_isequip3(temp_item.nameid);

	if(battle_config.item_auto_get){
		if(ditem->first_sd && (flag = pc_additem(ditem->first_sd,&temp_item,ditem->amount))){
			clif_additem(ditem->first_sd,0,0,flag);
			map_addflooritem(&temp_item,1,ditem->m,ditem->x,ditem->y,ditem->first_sd,ditem->second_sd,ditem->third_sd,0);
		}
		free(ditem);
		return 0;
	}

	map_addflooritem(&temp_item,1,ditem->m,ditem->x,ditem->y,ditem->first_sd,ditem->second_sd,ditem->third_sd,0);

	free(ditem);
	return 0;
}

/*==========================================
 * delay付きitem drop (timer関数) - lootitem
 *------------------------------------------
 */
static int mob_delay_item_drop2(int tid,unsigned int tick,int id,int data)
{
	struct delay_item_drop2 *ditem;
	int flag;

	nullpo_retr(0, ditem=(struct delay_item_drop2 *)id);

	if(battle_config.item_auto_get){
		if(ditem->first_sd && (flag = pc_additem(ditem->first_sd,&ditem->item_data,ditem->item_data.amount))){
			clif_additem(ditem->first_sd,0,0,flag);
			map_addflooritem(&ditem->item_data,ditem->item_data.amount,ditem->m,ditem->x,ditem->y,ditem->first_sd,ditem->second_sd,ditem->third_sd,0);
		}
		free(ditem);
		return 0;
	}

	map_addflooritem(&ditem->item_data,ditem->item_data.amount,ditem->m,ditem->x,ditem->y,ditem->first_sd,ditem->second_sd,ditem->third_sd,0);

	free(ditem);
	return 0;
}

int mob_timer_delete(int tid, unsigned int tick, int id, int data)
{
	struct block_list *bl=map_id2bl(id);

	nullpo_retr(0, bl);

	unit_remove_map(bl,3);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_deleteslave_sub(struct block_list *bl,va_list ap)
{
	struct mob_data *md;
	int id;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, md = (struct mob_data *)bl);

	id=va_arg(ap,int);
	if(md->master_id > 0 && md->master_id == id )
		unit_remove_map(&md->bl,1);
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int mob_deleteslave(struct mob_data *md)
{
	nullpo_retr(0, md);

	map_foreachinarea(mob_deleteslave_sub, md->bl.m,
		0,0,map[md->bl.m].xs,map[md->bl.m].ys,
		BL_MOB,md->bl.id);
	return 0;
}

/*==========================================
 * mdにsdからdamageのダメージ
 *------------------------------------------
 */
int mob_damage(struct block_list *src,struct mob_data *md,int damage,int type)
{
	int i,count;
	struct map_session_data *sd = NULL,**tmpsd = NULL;
	struct {
		struct party *p;
		int id;
		atn_bignumber base_exp,job_exp;
	} *pt = NULL;
	struct {
		struct map_session_data *sd;
		int dmg;
	} mvp[3];
	int pnum=0;
	int mvp_damage,max_hp;
	unsigned int tick = gettick();
	atn_bignumber tdmg,temp;
	struct item item;
	int ret;
	int drop_rate;
	int skill,sp;
	int race_id = 7;
	struct linkdb_node *node;

	nullpo_retr(0, md); //srcはNULLで呼ばれる場合もあるので、他でチェック

	max_hp = status_get_max_hp(&md->bl);

	if(src && src->type == BL_PC) {
		sd = (struct map_session_data *)src;
	}

//	if(battle_config.battle_log)
//		printf("mob_damage %d %d %d\n",md->hp,max_hp,damage);
	if(md->bl.prev==NULL){
		if(battle_config.error_log)
			printf("mob_damage : BlockError!!\n");
		return 0;
	}

	if(md->hp<=0) {
		if(md->bl.prev != NULL) {
			mobskill_use(md,tick,-1);	// 死亡時スキル
			unit_remove_map( &md->bl ,1);
		}
		return 0;
	}

	if(md->sc_data[SC_ENDURE].timer == -1)
		unit_stop_walking(&md->bl,3);
	if(damage > max_hp>>2)
		skill_stop_dancing(&md->bl,0);

	if(md->hp > max_hp)
		md->hp = max_hp;

	// over kill分は丸める
	if(damage>md->hp)
		damage=md->hp;

	if(!(type&2)) {
		if(sd!=NULL) {
			int damage2 = damage + (int)linkdb_search( &md->dmglog, (void*)sd->status.char_id );
			linkdb_replace( &md->dmglog, (void*)sd->status.char_id, (void*)damage2 );

			if(md->attacked_id <= 0 && md->state.special_mob_ai==0 &&
				atn_rand() % 1000 < 1000 / (++md->attacked_players)
			) {
				md->attacked_id = sd->bl.id;
			}
		}
		if(src && src->type == BL_PET && battle_config.pet_attack_exp_to_master) {
			struct pet_data *pd = (struct pet_data *)src;
			int damage2;
			nullpo_retr(0, pd);
			damage2 = damage*(battle_config.pet_attack_exp_rate)/100;
			damage2 += (int)linkdb_search( &md->dmglog, (void*)pd->msd->status.char_id );
			linkdb_replace( &md->dmglog, (void*)pd->msd->status.char_id, (void*)damage2 );
		}
		if(src && src->type == BL_MOB && ((struct mob_data*)src)->state.special_mob_ai){
			int damage2;
			struct mob_data *md2 = (struct mob_data *)src;
			struct map_session_data *msd = map_id2sd(md2->master_id);
			nullpo_retr(0, md2);
			nullpo_retr(0, msd);

			damage2 = damage + (int)linkdb_search( &md->dmglog, (void*)msd->status.char_id );
			linkdb_replace( &md->dmglog, (void*)msd->status.char_id, (void*)damage2 );

			if(md->attacked_id <= 0 && md->state.special_mob_ai==0 &&
				atn_rand() % 1000 < 1000 / (++md->attacked_players)
			) {
				md->attacked_id = md2->master_id;
			}
		}
	}

	md->hp-=damage;

	if(md->option&2 )
		status_change_end(&md->bl, SC_HIDING, -1);
	if(md->option&4 )
		status_change_end(&md->bl, SC_CLOAKING, -1);

	if(md->state.special_mob_ai == 2){//スフィアーマイン
		int skillidx=0;

		if((skillidx=mob_skillid2skillidx(md->class,NPC_SELFDESTRUCTION2))>=0){
			md->mode |= 0x1;
			md->next_walktime=tick;
			mobskill_use_id(md,&md->bl,skillidx);//自爆詠唱開始
			md->state.special_mob_ai++;
		}
	}

	if(md->hp>0){
		return 0;
	}

	// ----- ここから死亡処理 -----

	map_freeblock_lock();
	mobskill_use(md,tick,-1);	// 死亡時スキル

	memset(mvp,0,sizeof(mvp));

	max_hp = status_get_max_hp(&md->bl);

	if(src && src->type == BL_MOB)
		mob_unlocktarget((struct mob_data *)src,tick);

	/* ソウルドレイン */
	if(sd && sd->ud.skillid>0 && (skill=pc_checkskill(sd,HW_SOULDRAIN))>0 ){
		if(skill_get_inf(sd->ud.skillid)==1 && skill_db[sd->ud.skillid].skill_type&BF_MAGIC){
			clif_skill_nodamage(src,&md->bl,HW_SOULDRAIN,skill,1);
			sp = (status_get_lv(&md->bl))*(95+15*skill)/100;
			if(sd->status.sp + sp > sd->status.max_sp)
				sp = sd->status.max_sp - sd->status.sp;
			sd->status.sp += sp;
			clif_heal(sd->fd,SP_SP,sp);
		}
	}

	if(sd && md)
	{
		if(sd->status.class == PC_CLASS_TK && md->class == sd->tk_mission_target)
		{
			ranking_gain_point(sd,RK_TAEKWON,1);
			ranking_setglobalreg(sd,RK_TAEKWON);
			ranking_update(sd,RK_TAEKWON);
			if(ranking_get_point(sd,RK_TAEKWON)%100==0)
			{
				while(1){
					sd->tk_mission_target = 1000 + atn_rand()%1001;
					if(mob_db[sd->tk_mission_target].max_hp <= 0)
						continue;
					if(mob_db[sd->tk_mission_target].summonper[0]==0) //枝で呼ばれないのは除外
						continue;
					if(mob_db[sd->tk_mission_target].mode&0x20)//ボス属性除外
						continue;
					break;
				}
				pc_setglobalreg(sd,"PC_MISSION_TARGET",sd->tk_mission_target);
			}
		}
	}
	
	//カードによる死亡時HPSP吸収処理
	//種族取得
	if(md)
		race_id = status_get_race(&md->bl);
	//
	if(sd && md)
	{
		int hp = 0,sp = 0;
		sp += sd->sp_gain_value;
		hp += sd->hp_gain_value;
		if(rand()%100 < sd->hp_drain_rate_race[race_id])
			hp = sd->hp_drain_value_race[race_id];

		if(rand()%100 < sd->sp_drain_rate_race[race_id])
			sp = sd->sp_drain_value_race[race_id];

		if(hp || sp) pc_heal(sd,hp,sp);
	}

	//無条件

	// map外に消えた人は計算から除くので
	// overkill分は無いけどsumはmax_hpとは違う
	i     = 0;
	node  = md->dmglog;
	while( node ) {
		node = node->next;
		i++;
	}
	if( i > 0 ) {
		pt    = aCalloc( i, sizeof(*pt) );
		tmpsd = aCalloc( i, sizeof(*tmpsd) );
	}

	i     = 0;
	tdmg  = 0;
	count = 0;
	node  = md->dmglog;
	mvp_damage = 0;
	while( node ) {
		int damage2;
		tmpsd[i] = map_nick2sd(map_charid2nick( (int)node->key ) );
		if(tmpsd[i] == NULL) {
			node = node->next; i++;
			continue;
		}
		count++;
		if(tmpsd[i]->bl.m != md->bl.m || unit_isdead(&tmpsd[i]->bl)) {
			node = node->next; i++;
			continue;
		}

		damage2 = (int)node->data;
		tdmg += (atn_bignumber)damage2;
		if(mvp_damage < damage2) {
			if( mvp[0].sd == NULL || damage2 > mvp[0].dmg ) {
				// 一番大きいダメージ
				mvp[2] = mvp[1];
				mvp[1] = mvp[0];
				mvp[0].sd  = tmpsd[i];
				mvp[0].dmg = damage2;
				mvp_damage = (mvp[2].sd == NULL ? 0 : mvp[2].dmg );
			} else if( mvp[1].sd == NULL || damage2 > mvp[1].dmg ) {
				// ２番目に大きいダメージ
				mvp[2]     = mvp[1];
				mvp[1].sd  = tmpsd[i];
				mvp[1].dmg = damage2;
				mvp_damage = (mvp[2].sd == NULL ? 0 : mvp[2].dmg );
			} else {
				// ３番目に大きいダメージ
				mvp[2].sd  = tmpsd[i];
				mvp[2].dmg = damage2;
				mvp_damage = damage2;
			}
		}
		node = node->next; i++;
	}

//	if((double)max_hp < tdmg)
//		dmg_rate = ((double)max_hp) / tdmg;
//	else dmg_rate = 1;

	// 経験値の分配
	if(!md->state.noexp && tdmg > 0) {
		i    = 0;
		node = md->dmglog;
		while( node ){
			int pid,flag=1;
			atn_bignumber per,base_exp,job_exp;
			int base_exp_rate,job_exp_rate;
			int tk_exp_rate;
			struct party *p;
			if(tmpsd[i]==NULL || tmpsd[i]->bl.m != md->bl.m || unit_isdead(&tmpsd[i]->bl)) {
				node = node->next; i++;
				continue;
			}

			if(map[md->bl.m].base_exp_rate)
				base_exp_rate=(map[md->bl.m].base_exp_rate<0)?0:map[md->bl.m].base_exp_rate;
			else
				base_exp_rate=battle_config.base_exp_rate;
			if(map[md->bl.m].job_exp_rate)
				job_exp_rate =(map[md->bl.m].job_exp_rate <0)?0:map[md->bl.m].job_exp_rate;
			else
				job_exp_rate =battle_config.job_exp_rate;

			per = (10000/10) * (atn_bignumber)((int)node->data)*(9+((count > 6)? 6:count))/tdmg;
			base_exp = (((atn_bignumber)mob_db[md->class].base_exp * base_exp_rate ) /100 ) * per / 10000;
			if(mob_db[md->class].base_exp > 0 && base_exp < 1) base_exp = 1;
			if(base_exp < 0) base_exp = 0;
			job_exp = (((atn_bignumber)mob_db[md->class].job_exp * job_exp_rate  ) / 100 ) * per / 10000;
			if(mob_db[md->class].job_exp > 0 && job_exp < 1) job_exp = 1;
			if(job_exp < 0) job_exp = 0;

			//太陽と月と星の憎悪
			tk_exp_rate = 0;

			if(tmpsd[i]->sc_data[SC_MIRACLE].timer!=-1)
			{
				tk_exp_rate = 20*pc_checkskill(tmpsd[i],SG_STAR_BLESS);
			}else{
				if((battle_config.allow_skill_without_day || is_day_of_sun()) && md->class == tmpsd[i]->hate_mob[0])
					tk_exp_rate = 10*pc_checkskill(tmpsd[i],SG_SUN_BLESS);
				else if((battle_config.allow_skill_without_day || is_day_of_moon()) && md->class == tmpsd[i]->hate_mob[1])
					tk_exp_rate = 10*pc_checkskill(tmpsd[i],SG_MOON_BLESS);
				else if((battle_config.allow_skill_without_day || is_day_of_star()) && md->class == tmpsd[i]->hate_mob[2])
					tk_exp_rate = 20*pc_checkskill(tmpsd[i],SG_STAR_BLESS);
			}

			if((pid=tmpsd[i]->status.party_id)>0){	// パーティに入っている
				int j=0;
				for(j=0;j<pnum;j++)	// 公平パーティリストにいるかどうか
					if(pt[j].id==pid)
						break;
				if(j==pnum){	// いないときは公平かどうか確認
					if((p=party_search(pid))!=NULL && p->exp!=0){
						pt[pnum].id=pid;
						pt[pnum].p=p;
						pt[pnum].base_exp= base_exp + (base_exp*(tmpsd[i]->exp_rate[race_id]+tk_exp_rate))/100;
						pt[pnum].job_exp = job_exp  + (job_exp *(tmpsd[i]->job_rate[race_id]+tk_exp_rate))/100;
						pnum++;
						flag=0;
					}
				}else{	// いるときは公平
					pt[j].base_exp += base_exp + (base_exp*(tmpsd[i]->exp_rate[race_id]+tk_exp_rate))/100;
					pt[j].job_exp  += job_exp  + (job_exp *(tmpsd[i]->job_rate[race_id]+tk_exp_rate))/100;
					flag=0;
				}
			}
			if(flag)// 各自所得
			{
				atn_bignumber base_exp_,job_exp_;
				base_exp_ = base_exp + (base_exp*(tmpsd[i]->exp_rate[race_id]+tk_exp_rate))/100;
				job_exp_  = job_exp  + (job_exp *(tmpsd[i]->exp_rate[race_id]+tk_exp_rate))/100;
				if( !tmpsd[i]->sc_data ||
						((tmpsd[i]->sc_data[SC_TRICKDEAD].timer == -1 || !battle_config.noexp_trickdead ) && 	// 死んだふり していない
						 (tmpsd[i]->sc_data[SC_HIDING].timer == -1    || !battle_config.noexp_hiding    ) ) )	// ハイド していない
					pc_gainexp(tmpsd[i],
						(base_exp_ > 0x7fffffff)? 0x7fffffff : (int)base_exp_,
						(job_exp_  > 0x7fffffff)? 0x7fffffff : (int)job_exp_  );
			}
			node = node->next; i++;
		}
		// 公平分配
		for(i=0;i<pnum;i++)
			party_exp_share(pt[i].p, md->bl.m, pt[i].base_exp, pt[i].job_exp);
	}
	aFree( pt );
	aFree( tmpsd );

	// item drop
	if(!(type&1) && !map[md->bl.m].flag.nodrop) {
		if(!md->state.nodrop) {
			for(i=0;i<10;i++){
				struct delay_item_drop *ditem;
				int drop_rate;

				if(mob_db[md->class].dropitem[i].nameid <= 0)
					continue;
				drop_rate = mob_db[md->class].dropitem[i].p;
				if(drop_rate <= 0 && battle_config.drop_rate0item)
					drop_rate = 1;
				if(drop_rate <= rand()%10000)
					continue;

				ditem=(struct delay_item_drop *)aCalloc(1,sizeof(struct delay_item_drop));
				ditem->nameid = mob_db[md->class].dropitem[i].nameid;
				ditem->amount = 1;
				ditem->m = md->bl.m;
				ditem->x = md->bl.x;
				ditem->y = md->bl.y;
				ditem->first_sd  = mvp[0].sd;
				ditem->second_sd = mvp[1].sd;
				ditem->third_sd  = mvp[2].sd;
				add_timer(tick+500+i,mob_delay_item_drop,(int)ditem,0);
			}
		}
		if(sd){// && (sd->state.attack_type == BF_WEAPON)) {
			for(i=0;i<sd->monster_drop_item_count;i++) {
				struct delay_item_drop *ditem;
				int race = status_get_race(&md->bl);
				int mode = status_get_mode(&md->bl);
				if(sd->monster_drop_itemid[i] <= 0)
					continue;
				if(sd->monster_drop_race[i] & (1<<race) ||
					(mode & 0x20 && sd->monster_drop_race[i] & 1<<10) ||
					(!(mode & 0x20) && sd->monster_drop_race[i] & 1<<11) ) {
					if(sd->monster_drop_itemrate[i] <= rand()%10000)
						continue;

					ditem=(struct delay_item_drop *)aCalloc(1,sizeof(struct delay_item_drop));
					ditem->nameid = sd->monster_drop_itemid[i];
					ditem->amount = 1;
					ditem->m = md->bl.m;
					ditem->x = md->bl.x;
					ditem->y = md->bl.y;
					ditem->first_sd  = mvp[0].sd;
					ditem->second_sd = mvp[1].sd;
					ditem->third_sd  = mvp[2].sd;
					add_timer(tick+520+i,mob_delay_item_drop,(int)ditem,0);
				}
			}
			if(sd->get_zeny_num > 0)
				pc_getzeny(sd,mob_db[md->class].lv*10 + rand()%(sd->get_zeny_num+1));
			if(sd->get_zeny_num2 > 0 && rand()%100 < sd->get_zeny_num2)
				pc_getzeny(sd,mob_db[md->class].lv*10);
		}
		// 鉱石発見処理
		if (sd == mvp[0].sd && pc_checkskill(sd, BS_FINDINGORE) > 0) {
			int rate = battle_config.finding_ore_drop_rate;
			rate *= battle_config.item_rate;
			rate /= 100;
			if (rate < 0)
				rate = 0;
			else if (rate > 10000)
				rate = 10000;
			if (rate > rand() % 10000) {
				struct delay_item_drop *ditem;
				ditem = (struct delay_item_drop*)aCalloc(1, sizeof (struct delay_item_drop));
				ditem->nameid = itemdb_searchrandomid(6);
				ditem->amount = 1;
				ditem->m = md->bl.m;
				ditem->x = md->bl.x;
				ditem->y = md->bl.y;
				ditem->first_sd  = mvp[0].sd;
				ditem->second_sd = mvp[1].sd;
				ditem->third_sd  = mvp[2].sd;
				add_timer(tick + 520 + i, mob_delay_item_drop, (int)ditem, 0);
			}
		}
		if(md->lootitem) {
			for(i=0;i<md->lootitem_count;i++) {
				struct delay_item_drop2 *ditem;

				ditem=(struct delay_item_drop2 *)aCalloc(1,sizeof(struct delay_item_drop2));
				memcpy(&ditem->item_data,&md->lootitem[i],sizeof(md->lootitem[0]));
				ditem->m = md->bl.m;
				ditem->x = md->bl.x;
				ditem->y = md->bl.y;
				ditem->first_sd  = mvp[0].sd;
				ditem->second_sd = mvp[1].sd;
				ditem->third_sd  = mvp[2].sd;
				add_timer(tick+540+i,mob_delay_item_drop2,(int)ditem,0);
			}
		}
	}

	// mvp処理
	if(mvp[0].sd && mob_db[md->class].mexp > 0 && !md->state.nomvp){
		int j;
		int mexp;
		temp = ((atn_bignumber)mob_db[md->class].mexp * battle_config.mvp_exp_rate * (9+count)/1000);
		mexp = (temp > 2147483647)? 0x7fffffff:(int)temp;
		if(mexp < 1) mexp = 1;
		if(battle_config.mpv_announce){
			char output[256];
			snprintf(output, sizeof output,
				"【MVP情報】%sさんが%sを倒しました！",mvp[0].sd->status.name,mob_db[md->class].jname);
			clif_GMmessage(&mvp[0].sd->bl,output,strlen(output)+1,0x10);
		}
		clif_mvp_effect(mvp[0].sd);					// エフェクト
		if(mob_db[md->class].mexpper > rand()%10000){
			clif_mvp_exp(mvp[0].sd,mexp);
			pc_gainexp(mvp[0].sd,mexp,0);
		}
		for(j=0;j<3;j++){
			i = rand() % 3;
			if(mob_db[md->class].mvpitem[i].nameid <= 0)
				continue;
			drop_rate = mob_db[md->class].mvpitem[i].p;
			if(drop_rate <= 0 && battle_config.drop_rate0item)
				drop_rate = 1;
			if(drop_rate <= rand()%10000)
				continue;
			memset(&item,0,sizeof(item));
			item.nameid=mob_db[md->class].mvpitem[i].nameid;
			item.identify=!itemdb_isequip3(item.nameid);
			clif_mvp_item(mvp[0].sd,item.nameid);
			if(mvp[0].sd->weight*2 > mvp[0].sd->max_weight)
				map_addflooritem(&item,1,mvp[0].sd->bl.m,mvp[0].sd->bl.x,mvp[0].sd->bl.y,mvp[0].sd,mvp[1].sd,mvp[2].sd,1);
			else if((ret = pc_additem(mvp[0].sd,&item,1))) {
				clif_additem(sd,0,0,ret);
				map_addflooritem(&item,1,mvp[0].sd->bl.m,mvp[0].sd->bl.x,mvp[0].sd->bl.y,mvp[0].sd,mvp[1].sd,mvp[2].sd,1);
			}
			break;
		}
	}

	// <Agit> NPC Event [OnAgitBreak]
	if(md->npc_event[0] && strcmp(((md->npc_event)+strlen(md->npc_event)-13),"::OnAgitBreak") == 0) {
		printf("MOB.C: Run NPC_Event[OnAgitBreak].\n");
		if (agit_flag == 1) //Call to Run NPC_Event[OnAgitBreak]
			guild_agit_break(md);
	}

		// SCRIPT実行
	if(md->npc_event[0]){
//		if(battle_config.battle_log)
//			printf("mob_damage : run event : %s\n",md->npc_event);
		if(src && src->type == BL_PET)
			sd = ((struct pet_data *)src)->msd;
		if(sd == NULL) {
			if(mvp[0].sd != NULL)
				sd = mvp[0].sd;
			else {
				struct map_session_data *tmpsd;
				int i;
				for(i=0;i<fd_max;i++){
					if(session[i] && (tmpsd=session[i]->session_data) && tmpsd->state.auth) {
						if(md->bl.m == tmpsd->bl.m) {
							sd = tmpsd;
							break;
						}
					}
				}
			}
		}
		if(sd)
			npc_event(sd,md->npc_event);
	}
	
	//実は復活していた！？
	if(md->hp > 0)
		return 0;
	
	unit_remove_map(&md->bl, 1);

	map_freeblock_unlock();

	return 0;
}

/*==========================================
 *	ランダムクラスチェンジ
 *------------------------------------------
 */
int mob_class_change_randam(struct mob_data *md,int lv)
{
	int i=0,j=0,k;
	int class = 1002;//ポリン
	nullpo_retr(0, md);
	if(md->bl.type != BL_MOB)
	 	return 0;

	// ランダムに召喚
	if(j>=0 && j<MAX_RANDOMMONSTER){
		do{
			class=rand()%1000+1001;
			k=rand()%1000000;
		}while((mob_db[class].max_hp <= 0 || mob_db[class].summonper[j] <= k ||
			 (lv<mob_db[class].lv && battle_config.random_monster_checklv)) && (i++) < MOB_ID_MAX);
		if(i>=MOB_ID_MAX){
			class=mob_db[MOB_ID_MIN].summonper[j];
		}
	}

	return mob_class_change_id(md,class);
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_class_change_id(struct mob_data *md,int mob_id)
{
	unsigned int tick = gettick();
	int i,c,hp_rate,max_hp,class;

	nullpo_retr(0, md);

	if(md->bl.prev == NULL) return 0;

	class = mob_id;
	if(class<=1000 || class>MOB_ID_MAX) return 0;

	max_hp = status_get_max_hp(&md->bl);	// max_hp>0は保証
	hp_rate = md->hp*100/max_hp;
	clif_class_change(&md->bl,mob_get_viewclass(class),1);
	md->class = class;
	max_hp = status_get_max_hp(&md->bl);
	if(battle_config.monster_class_change_full_recover) {
		md->hp = max_hp;
		linkdb_final( &md->dmglog );
	}
	else
		md->hp = max_hp*hp_rate/100;
	if(md->hp > max_hp) md->hp = max_hp;
	else if(md->hp < 1) md->hp = 1;

	memcpy(md->name,mob_db[class].jname,24);
	memset(&md->state,0,sizeof(md->state));
	md->attacked_id = 0;
	md->target_id = 0;
	md->move_fail_count = 0;

	md->speed = mob_db[md->class].speed;
	md->def_ele = mob_db[md->class].element;

	unit_skillcastcancel(&md->bl,0);
	md->state.skillstate = MSS_IDLE;
	md->last_thinktime = tick;
	md->next_walktime = tick+rand()%50+5000;

	md->state.nodrop=0;
	md->state.noexp=0;
	md->state.nomvp=0;

	for(i=0,c=tick-1000*3600*10;i<MAX_MOBSKILL;i++)
		md->skilldelay[i] = c;
	md->ud.skillid=0;
	md->ud.skilllv=0;

	if(md->lootitem == NULL && mob_db[class].mode&0x02)
		md->lootitem=(struct item *)aCalloc(LOOTITEM_SIZE,sizeof(struct item));

	skill_clear_unitgroup(&md->bl);
	skill_cleartimerskill(&md->bl);

	clif_clearchar_area(&md->bl,0);
	clif_spawnmob(md);

	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int mob_class_change(struct mob_data *md,int *value,int value_count)
{
	int count = 0;

	nullpo_retr(0, md);
	nullpo_retr(0, value);

	if(value[0]<=1000 || value[0]>MOB_ID_MAX)
		return 0;
	if(md->bl.prev == NULL) return 0;

	while(count < value_count && value[count] > 1000 && value[count] <= MOB_ID_MAX) count++;
	if(count < 1) return 0;

	return mob_class_change_id(md,value[rand()%count]);
}

int mob_change_summon_monster_data(struct mob_data* ma)
{
	return 0;
}

/*==========================================
 * mob回復
 *------------------------------------------
 */
int mob_heal(struct mob_data *md,int heal)
{
	int max_hp = status_get_max_hp(&md->bl);

	nullpo_retr(0, md);

	md->hp += heal;
	if( max_hp < md->hp )
		md->hp = max_hp;
	return 0;
}

/*==========================================
 * mobワープ
 *------------------------------------------
 */
int mob_warp(struct mob_data *md,int m,int x,int y,int type)
{
	int i=0,xs=0,ys=0,bx=x,by=y;
	int tick = gettick();

	nullpo_retr(0, md);

	if( md->bl.prev==NULL )
		return 0;

	if( m<0 ) m=md->bl.m;

	if(type >= 0) {
		if(map[md->bl.m].flag.monster_noteleport)
			return 0;
		clif_clearchar_area(&md->bl,type);
	}
	skill_unit_move(&md->bl,tick,0);
	map_delblock(&md->bl);

	if(bx>0 && by>0){	// 位置指定の場合周囲９セルを探索
		xs=ys=9;
	}

	while( ( x<0 || y<0 || map_getcell(m,x,y,CELL_CHKNOPASS)) && (i++)<1000 ){
		if( xs>0 && ys>0 && i<250 ){	// 指定位置付近の探索
			x=bx+rand()%xs-xs/2;
			y=by+rand()%ys-ys/2;
		}else{			// 完全ランダム探索
			x=rand()%(map[m].xs-2)+1;
			y=rand()%(map[m].ys-2)+1;
		}
	}
	md->dir=0;
	if(i<1000){
		md->bl.x=md->ud.to_x=x;
		md->bl.y=md->ud.to_y=y;
		md->bl.m=m;
	}else {
		m=md->bl.m;
		if(battle_config.error_log)
			printf("MOB %d warp failed, class = %d\n",md->bl.id,md->class);
	}

	md->target_id   = 0;	// タゲを解除する
	md->attacked_id = 0;
	md->state.skillstate=MSS_IDLE;

	if(type>0 && i==1000) {
		if(battle_config.battle_log)
			printf("MOB %d warp to (%d,%d), class = %d\n",md->bl.id,x,y,md->class);
	}

	map_addblock(&md->bl);
	skill_unit_move(&md->bl,tick,1);
	if(type>0)
		clif_spawnmob(md);
	return 0;
}

/*==========================================
 * 画面内の取り巻きの数計算用(foreachinarea)
 *------------------------------------------
 */
int mob_countslave_sub(struct block_list *bl,va_list ap)
{
	int id,*c;
	struct mob_data *md;

	id=va_arg(ap,int);

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, c=va_arg(ap,int *));
	nullpo_retr(0, md = (struct mob_data *)bl);


	if( md->master_id==id )
		(*c)++;
	return 0;
}
/*==========================================
 * 画面内の取り巻きの数計算
 *------------------------------------------
 */
int mob_countslave(struct mob_data *md)
{
	int c=0;

	nullpo_retr(0, md);

	map_foreachinarea(mob_countslave_sub, md->bl.m,
		0,0,map[md->bl.m].xs-1,map[md->bl.m].ys-1,
		BL_MOB,md->bl.id,&c);
	return c;
}
/*==========================================
 * 画面内の取り巻きの数計算用(foreachinarea)
 *------------------------------------------
 */
int mob_countslave_area_sub(struct block_list *bl,va_list ap)
{
	int id,*c;
	struct mob_data *md;

	id=va_arg(ap,int);

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, c=va_arg(ap,int *));
	nullpo_retr(0, md = (struct mob_data *)bl);


	if( md->master_id==id )
		(*c)++;
	return 0;
}
/*==========================================
 * 画面内の取り巻きの数計算
 *------------------------------------------
 */
int mob_countslave_area(struct mob_data *md,int range)
{
	int c=0;

	nullpo_retr(0, md);

	map_foreachinarea(mob_countslave_area_sub,md->bl.m,
		md->bl.x-range,md->bl.y-range,md->bl.x+range,md->bl.y+range,
		BL_MOB,md->bl.id,&c);
	return c;
}
/*==========================================
 * 手下MOB召喚
 *------------------------------------------
 */
int mob_summonslave(struct mob_data *md2,int *value,int amount,int flag)
{
	struct mob_data *md;
	int bx,by,m,count = 0,class,k,a = amount;

	nullpo_retr(0, md2);
	nullpo_retr(0, value);

	bx=md2->bl.x;
	by=md2->bl.y;
	m=md2->bl.m;

	if(value[0]<=1000 || value[0]>MOB_ID_MAX)	// 値が異常なら召喚を止める
		return 0;
	while(count < 5 && value[count] > 1000 && value[count] <= MOB_ID_MAX) count++;
	if(count < 1) return 0;

	for(k=0;k<count;k++) {
		amount = a;
		class = value[k];
		if(class<=1000 || class>MOB_ID_MAX) continue;
		for(;amount>0;amount--){
			int x=0,y=0,i=0;
			md=(struct mob_data *)aCalloc(1,sizeof(struct mob_data));
			if(mob_db[class].mode&0x02)
				md->lootitem=(struct item *)aCalloc(LOOTITEM_SIZE,sizeof(struct item));
			else
				md->lootitem=NULL;

			while((x<=0 || y<=0 || map_getcell(m,x,y,CELL_CHKNOPASS)) && (i++)<100){
				x=rand()%9-4+bx;
				y=rand()%9-4+by;
			}
			if(i>=100){
				x=bx;
				y=by;
			}

			mob_spawn_dataset(md,"--ja--",class);
			md->bl.m=m;
			md->bl.x=x;
			md->bl.y=y;

			md->m =m;
			md->x0=x;
			md->y0=y;
			md->xs=0;
			md->ys=0;
			md->speed=md2->speed;
			md->spawndelay1=-1;	// 一度のみフラグ
			md->spawndelay2=-1;	// 一度のみフラグ


			memset(md->npc_event,0,sizeof(md->npc_event));
			md->bl.type=BL_MOB;
			map_addiddb(&md->bl);
			mob_spawn(md->bl.id);

			clif_skill_nodamage(&md->bl,&md->bl,(flag)? NPC_SUMMONSLAVE:NPC_SUMMONMONSTER,a,1);

			if(flag){
				md->master_id=md2->bl.id;
				md->state.nodrop = battle_config.summonslave_no_drop;
				md->state.noexp  = battle_config.summonslave_no_exp;
				md->state.nomvp  = battle_config.summonslave_no_mvp;
			}else{
				md->state.nodrop = battle_config.summonmonster_no_drop;
				md->state.noexp  = battle_config.summonmonster_no_exp;
				md->state.nomvp  = battle_config.summonmonster_no_mvp;
			}

		}
	}
	return 0;
}

/*==========================================
 *MOBskillから該当skillidのskillidxを返す
 *------------------------------------------
 */
int mob_skillid2skillidx(int class,int skillid)
{
	int i;
	struct mob_skill *ms=mob_db[class].skill;

	if(ms==NULL)
		return -1;

	for(i=0;i<mob_db[class].maxskill;i++){
		if(ms[i].skill_id == skillid)
			return i;
	}
	return -1;

}

/*==========================================
 * スキル使用（詠唱開始、ID指定）
 *------------------------------------------
 */
int mobskill_use_id(struct mob_data *md,struct block_list *target,int skill_idx)
{
	struct mob_skill *ms;
	int casttime;


	nullpo_retr(0, md);
	nullpo_retr(0, ms=&mob_db[md->class].skill[skill_idx]);

	casttime                  = skill_castfix(&md->bl, ms->casttime);
	md->skillidx              = skill_idx;
	md->skilldelay[skill_idx] = gettick() + casttime;
	return unit_skilluse_id2(
		&md->bl, target ? target->id : md->target_id, ms->skill_id,
		ms->skill_lv, casttime, ms->cancel
	);
}

int mobskill_command_use_id_sub(struct block_list *bl, va_list ap )
{
	int skill_idx;
	int target_id;
	int target_type;
	int casttime;
	int commander_id;
	int *flag;
	struct block_list *src=NULL;
	struct mob_data* md=NULL;
	struct mob_skill *ms;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	commander_id = va_arg(ap,int);
	src =va_arg(ap,struct block_list *);
	target_type=va_arg(ap,int);
	skill_idx=va_arg(ap,int);
	flag = va_arg(ap,int*);
	
	if(*flag==1)
		return 0;
	if(src->type!=BL_MOB)
		return 0;
	
	md = (struct mob_data*)src;

	nullpo_retr(0, ms=&mob_db[md->class].skill[skill_idx]);
	casttime = skill_castfix(bl, ms->casttime);
	md->skillidx = skill_idx;
	md->skilldelay[skill_idx] = gettick() + casttime;
		
	switch(target_type)
	{
		case MCT_TARGET:
			if(md->bl.id == bl->id)
				return 0;
			if(md->target_id>0)
				target_id = md->target_id;
			else if(bl->id != md->target_id)
				target_id = bl->id;
			else// if(bl->id != md->target_id)
				target_id = bl->id;
			break;
		case MCT_FRIEND:
			if(bl->type!=BL_MOB)
				return 0;
			if(md->bl.id == bl->id)
				return 0;
			target_id = bl->id;
			*flag = 1;
			break;
		case MCT_FRIENDS:
			if(bl->type!=BL_MOB)
				return 0;
			if(md->bl.id == bl->id)
				return 0;
			target_id = bl->id;
			break;
		case MCT_SLAVE:
			if(bl->type!=BL_MOB)
				return 0;
			if(md->bl.id == bl->id)
				return 0;
			if(md->bl.id != ((struct mob_data*)bl)->master_id)
				return 0;
			target_id = bl->id;
			*flag = 1;
			break;
		case MCT_SLAVES:
			if(bl->type!=BL_MOB)
				return 0;
			if(md->bl.id == bl->id)
				return 0;
			if(md->bl.id != ((struct mob_data*)bl)->master_id)
				return 0;
			target_id = bl->id;
			break;
		default:
			puts("mobskill_command_use_id_sub :target_type error\n");
			return 0;
			break;
	}
	return unit_skilluse_id2(&md->bl,target_id, ms->skill_id,
		ms->skill_lv, casttime, ms->cancel);
	
}
int mobskill_command(struct block_list *bl, va_list ap )
{
	int casttime;
	int commander_id,target_id;
	int skill_id,skill_idx;
	int command_target_type;
	int range;
	int target_type;
	int *flag;
	int once_flag = 0;
	struct mob_data* md=NULL;
	struct mob_skill *ms;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	commander_id=va_arg(ap,int);
	skill_id=va_arg(ap,int);
	command_target_type=va_arg(ap,int);
	target_type=va_arg(ap,int);
	range = va_arg(ap,int);
	flag = va_arg(ap,int*);
	
	if(bl->type!=BL_MOB)
		return 0;
	md = (struct mob_data*)bl;
	
	skill_idx = mob_skillid2skillidx(md->class,skill_id);
	if(skill_idx==-1)
		return 0;
	
	nullpo_retr(0, ms=&mob_db[md->class].skill[skill_idx]);

	switch(command_target_type)
	{
		case MCT_SELF:
			if(bl->id != md->bl.id)
				return 0;
			if(*flag==1)
				return 0;
			break;
		case MCT_FRIEND:
			if(commander_id == md->master_id)
				return 0;
			if(*flag==1)
				return 0;
			break;
		case MCT_FRIENDS:
			if(commander_id == md->master_id)
				return 0;
			break;
		case MCT_SLAVE:
			if(commander_id != md->master_id)
				return 0;
			break;
		case MCT_SLAVES:
			if(commander_id != md->master_id)
				return 0;
			break;
	}
	
	//ターゲット選別
	switch(target_type)
	{
		case MCT_MASTER:
			if(md->master_id>0)
				target_id = md->master_id;
			else	
				return 0;
			break;
		case MCT_COMMANDER:
			if(commander_id>0)
				target_id = commander_id;
			else
				return 0;
			break;
		case MCT_SELF:
			target_id = bl->id;
			break;
		case MCT_TARGET:
			map_foreachinarea(mobskill_command_use_id_sub,bl->m,bl->x-range,bl->y-range,bl->x+range,bl->y+range,
				BL_PC|BL_MOB,commander_id,bl,target_type,skill_idx,&once_flag);
			*flag = 1;
			return *flag;
			break;
		case MCT_FRIEND:
		case MCT_SLAVE:
			map_foreachinarea( mobskill_command_use_id_sub,bl->m,bl->x-range,bl->y-range,bl->x+range,bl->y+range,
				BL_MOB,commander_id,bl,target_type,skill_idx,&once_flag);
			*flag = 1;
			return *flag;
			break;
		default:
			puts("mobskill_command_use_id_sub :target_type error\n");
			return 0;
			break;
	}
	*flag = 1;
	casttime = skill_castfix(bl, ms->casttime);
	md->skillidx = skill_idx;
	md->skilldelay[skill_idx] = gettick() + casttime;
	unit_skilluse_id2(&md->bl,target_id, ms->skill_id,
		ms->skill_lv, casttime, ms->cancel);
	return *flag;					
}

int mobskill_modechange(struct block_list *bl, va_list ap )
{
	int commander_id,target_type,mode;
	int *flag;
	struct mob_data* md=NULL;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	commander_id=va_arg(ap,int);
	target_type=va_arg(ap,int);
	mode = va_arg(ap,int);
	flag = va_arg(ap,int*);
	if(bl->type!=BL_MOB)
		return 0;
	md = (struct mob_data*)bl;
	switch(target_type)
	{
		case MCT_SELF:
			if(commander_id != bl->id)
				return 0;
			if(*flag==1)
				return 0;
			break;
		case MCT_FRIEND:
			if(commander_id == bl->id)
				return 0;
			if(*flag==1)
				return 0;
			break;
		case MCT_FRIENDS:
			if(commander_id == bl->id)
				return 0;
			break;
		case MCT_SLAVE:
			if(commander_id != md->master_id)
				return 0;
			if(*flag==1)
				return 0;
			break;
		case MCT_SLAVES:
			if(commander_id != md->master_id)
				return 0;
			break;
	}
	*flag = 1;
	md->mode = mode;
	return	*flag;
}
/*==========================================
 * スキル使用（場所指定）
 *------------------------------------------
 */
int mobskill_use_pos( struct mob_data *md,
	int skill_x, int skill_y, int skill_idx)
{
	int casttime=0;
	struct mob_skill *ms;

	nullpo_retr(0, md);
	nullpo_retr(0, ms=&mob_db[md->class].skill[skill_idx]);

	casttime                  = skill_castfix(&md->bl, ms->casttime);
	md->skillidx              = skill_idx;
	md->skilldelay[skill_idx] = gettick() + casttime;
	return unit_skilluse_pos2( &md->bl, skill_x, skill_y, ms->skill_id, ms->skill_lv, casttime, ms->cancel);
}

/*==========================================
 * 近くのMOBでHPの減っているものを探す
 *------------------------------------------
 */
int mob_getfriendhpltmaxrate_sub(struct block_list *bl,va_list ap)
{
	int rate;
	struct mob_data **fr, *md, *mmd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, mmd=va_arg(ap,struct mob_data *));

	md=(struct mob_data *)bl;

	if( mmd->bl.id == bl->id )
		return 0;
	rate=va_arg(ap,int);
	fr=va_arg(ap,struct mob_data **);
	if( md->hp < mob_db[md->class].max_hp*rate/100 )
		(*fr)=md;
	return 0;
}
struct mob_data *mob_getfriendhpltmaxrate(struct mob_data *md,int rate)
{
	struct mob_data *fr=NULL;
	const int r=8;

	nullpo_retr(NULL, md);

	map_foreachinarea(mob_getfriendhpltmaxrate_sub, md->bl.m,
		md->bl.x-r ,md->bl.y-r, md->bl.x+r, md->bl.y+r,
		BL_MOB,md,rate,&fr);
	return fr;
}
/*==========================================
 * 近くのMOBでステータス状態が合うものを探す
 *------------------------------------------
 */
int mob_getfriendstatus_sub(struct block_list *bl,va_list ap)
{
	int cond1,cond2;
	struct mob_data **fr, *md, *mmd;
	int flag=0;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, md=(struct mob_data *)bl);
	nullpo_retr(0, mmd=va_arg(ap,struct mob_data *));

	if( mmd->bl.id == bl->id )
		return 0;
	cond1=va_arg(ap,int);
	cond2=va_arg(ap,int);
	fr=va_arg(ap,struct mob_data **);
	if( cond2==-1 ){
		int j;
		for(j=SC_STONE;j<=SC_BLIND && !flag;j++){
			flag=(md->sc_data[j].timer!=-1 );
		}
	}else
		flag=( md->sc_data[cond2].timer!=-1 );
	if( flag^( cond1==MSC_FRIENDSTATUSOFF ) )
		(*fr)=md;

	return 0;
}
struct mob_data *mob_getfriendstatus(struct mob_data *md,int cond1,int cond2)
{
	struct mob_data *fr=NULL;
	const int r=8;

	nullpo_retr(0, md);

	map_foreachinarea(mob_getfriendstatus_sub, md->bl.m,
		md->bl.x-r ,md->bl.y-r, md->bl.x+r, md->bl.y+r,
		BL_MOB,md,cond1,cond2,&fr);
	return fr;
}

/*==========================================
 * スキル使用判定
 *------------------------------------------
 */
int mobskill_use(struct mob_data *md,unsigned int tick,int event)
{
	struct mob_skill *ms;
	struct map_session_data *tsd=NULL;
//	struct block_list *target=NULL;
	struct mob_data* master_md=NULL;
	int i,max_hp;

	nullpo_retr(0, md);
	nullpo_retr(0, ms = mob_db[md->class].skill);

	max_hp = status_get_max_hp(&md->bl);
	
	if(battle_config.mob_skill_use == 0 || md->ud.skilltimer != -1)
		return 0;

	if(md->state.special_mob_ai)
		return 0;

	if(md->sc_data[SC_SELFDESTRUCTION].timer!=-1)	//自爆中はスキルを使わない
		return 0;

	if(md->hp <= 0) {
		md->state.skillstate = MSS_DEAD;
	}
	if(md->ud.attacktimer != -1) {
		md->state.skillstate = MSS_ATTACK;
	}
	if(md->ud.walktimer != -1 && md->state.skillstate != MSS_CHASE) {
		md->state.skillstate = MSS_WALK;
	}
	tsd = map_id2sd(md->target_id);
	master_md = (struct mob_data*)map_id2bl(md->master_id);
	
	for(i=0;i<mob_db[md->class].maxskill;i++){
		int c2=ms[i].cond2,flag=0;
		struct mob_data *fmd=NULL;

		// ディレイ中
		if( DIFF_TICK(tick,md->skilldelay[i])<ms[i].delay )
			continue;
			
		//コマンド専用
		if(ms[i].state==MSS_COMMANDONLY || ms[i].state == MSS_DISABLE)
			continue;
		
		// 状態判定
		if(ms[i].state != MSS_ANY && ms[i].state!=md->state.skillstate )
			continue;
		
		// 条件判定
		flag=(event==ms[i].cond1);
		if(!flag){
			switch( ms[i].cond1 ){
			case MSC_ALWAYS:
				flag=1; break;
			case MSC_MYHPLTMAXRATE:		// HP< maxhp%
				flag=( md->hp < max_hp*c2/100 ); break;
			case MSC_MYSTATUSON:		// status[num] on
			case MSC_MYSTATUSOFF:		// status[num] off
				if( ms[i].cond2==-1 ){
					int j;
					for(j=SC_STONE;j<=SC_BLIND && !flag;j++){
						flag=(md->sc_data[j].timer!=-1 );
					}
				}else
					flag=( md->sc_data[ms[i].cond2].timer!=-1 );
				flag^=( ms[i].cond1==MSC_MYSTATUSOFF ); break;
			case MSC_FRIENDHPLTMAXRATE:	// friend HP < maxhp%
				flag=(( fmd=mob_getfriendhpltmaxrate(md,ms[i].cond2) )!=NULL ); break;
			case MSC_FRIENDSTATUSON:	// friend status[num] on
			case MSC_FRIENDSTATUSOFF:	// friend status[num] off
				flag=(( fmd=mob_getfriendstatus(md,ms[i].cond1,ms[i].cond2) )!=NULL ); break;
			case MSC_SLAVELT:		// slave < num
				flag=( mob_countslave(md) < c2 ); break;
			case MSC_ATTACKPCGT:	// attack pc > num
				flag=( unit_counttargeted(&md->bl,0) > c2 ); break;
			case MSC_SLAVELE:		// slave <= num
				flag=( mob_countslave(md) <= c2 ); break;
			case MSC_AREASLAVEGT:		// slave > num
				flag=( mob_countslave_area(md,ms[i].val[0]) > c2 ); break;
			case MSC_AREASLAVELE:		// slave <= num
				flag=( mob_countslave_area(md,ms[i].val[0]) <= c2 ); break;
			case MSC_ATTACKPCGE:	// attack pc >= num
				flag=( unit_counttargeted(&md->bl,0) >= c2 ); break;
			case MSC_SKILLUSED:		// specificated skill used
				flag=( (event&0xffff)==MSC_SKILLUSED && ((event>>16)==c2 || c2==0)); break;
			case MSC_TARGETHPGTMAXRATE:	// target HP < maxhp%
				if(tsd)
					flag= tsd->status.hp > tsd->status.max_hp*c2/100;
				break;
			case MSC_TARGETHPLTMAXRATE:	// target HP < maxhp%
				if(tsd)
					flag= tsd->status.hp < tsd->status.max_hp*c2/100;
				break;
			case MSC_TARGETHPGT:	// target HP < maxhp%
				if(tsd)
					flag= tsd->status.hp > c2;
				break;
			case MSC_TARGETHPLT:	// target HP < maxhp%
				if(tsd)
					flag= tsd->status.hp < c2;
				break;
			case MSC_TARGETSTATUSON:	// target status[num] on
			case MSC_TARGETSTATUSOFF:	// target status[num] off
				if(tsd){
					if( ms[i].cond2==-1 ){
						int j = 0;
						if(md->sc_data[SC_STONE].timer!=-1)
							flag= md->sc_data[SC_STONE].val2==0;
						for(j=SC_STONE+1;j<=SC_BLIND && !flag;j++){
							flag=(md->sc_data[j].timer!=-1 );
						}
					}else
						flag=( md->sc_data[ms[i].cond2].timer!=-1 );
					flag^=( ms[i].cond1==MSC_MYSTATUSOFF );
				}
				break;
			case MSC_MASTERHPGTMAXRATE:	// master HP < maxhp%
				if(master_md)
					flag= master_md->hp > status_get_max_hp(&master_md->bl)*c2/100;
				break;
			case MSC_MASTERHPLTMAXRATE:	// master HP < maxhp%
				if(master_md)
					flag= master_md->hp < status_get_max_hp(&master_md->bl)*c2/100;
				break;
			case MSC_MASTERSTATUSON:	// master status[num] on
			case MSC_MASTERSTATUSOFF:	// master status[num] off
				if(master_md){
					if( ms[i].cond2==-1 ){
						int j = 0;
						if(master_md->sc_data[SC_STONE].timer!=-1)
							flag= master_md->sc_data[SC_STONE].val2==0;
						for(j=SC_STONE+1;j<=SC_BLIND && !flag;j++){
							flag=(master_md->sc_data[j].timer!=-1 );
						}
					}else
						flag=( master_md->sc_data[ms[i].cond2].timer!=-1 );
					flag^=( ms[i].cond1==MSC_MYSTATUSOFF );
				}
				break;
			}
		}

		// 確率判定
		if( flag && rand()%10000 < ms[i].permillage ){

			if( skill_get_inf(ms[i].skill_id)&2 ){
				// 場所指定
				struct block_list *bl = NULL;
				int x=0,y=0;
				if( ms[i].target<=MST_AROUND ){
					bl= ((ms[i].target==MST_TARGET || ms[i].target==MST_AROUND5)? map_id2bl(md->target_id):
						 (ms[i].target==MST_FRIEND)? &fmd->bl : &md->bl);
					if(bl!=NULL){
						x=bl->x; y=bl->y;
					}
				}
				if( x<=0 || y<=0 )
					continue;
				// 自分の周囲
				if( ms[i].target>=MST_AROUND1 ){
					int bx=x, by=y, i=0,m=bl->m, r=ms[i].target-MST_AROUND1;
					do{
						bx=x + rand()%(r*2+3) - r;
						by=y + rand()%(r*2+3) - r;
					}while( ( bx<=0 || by<=0 || bx>=map[m].xs || by>=map[m].ys ||
						map_getcell(m,bx,by,CELL_CHKNOPASS)) && (i++)<1000);
					if(i<1000){
						x=bx; y=by;
					}
				}
				// 相手の周囲
				if( ms[i].target>=MST_AROUND5 ){
					int bx=x, by=y, i=0,m=bl->m, r=(ms[i].target-MST_AROUND5)+1;
					do{
						bx=x + rand()%(r*2+1) - r;
						by=y + rand()%(r*2+1) - r;
					}while( ( bx<=0 || by<=0 || bx>=map[m].xs || by>=map[m].ys ||
						map_getcell(m,bx,by,CELL_CHKNOPASS)) && (i++)<1000);
					if(i<1000){
						x=bx; y=by;
					}
				}
				if(!mobskill_use_pos(md,x,y,i))
					return 0;
			}else{
				switch(ms[i].target)
				{
					case MST_TARGET:
					case MST_SELF:
					case MST_FRIEND:
					{
						struct block_list *bl = NULL;
						bl= ((ms[i].target==MST_TARGET)? map_id2bl(md->target_id):
							 (ms[i].target==MST_FRIEND)? &fmd->bl : &md->bl);
						if(bl && !mobskill_use_id(md,bl,i))
							return 0;
					}
						break;
					case MST_MASTER:
						if(master_md && !mobskill_use_id(md,&master_md->bl,i))
							return 0;
						break;
					case MST_COMMAND:
					{
						int use_flag = 0;
						int range = ms[i].val[1];
						int once_flag = 0;
						switch(ms[i].val[0])
						{
						case MCT_GROUP:
							{
								use_flag = 1;
								if(md)
								{
									int casttime = skill_castfix(&md->bl, ms[i].casttime);
									md->skilldelay[i] = gettick() + casttime;
									md->skillidx = i;
									unit_skilluse_id2(&md->bl,md->bl.id, ms[i].skill_id,
										ms[i].skill_lv,casttime, ms[i].cancel);
								}
								//map_foreachinarea( mobskill_command,
								//	md->bl.m,md->bl.x-range,md->bl.y-range,md->bl.x+range,md->bl.y+range,BL_MOB,md->bl.id,ms[i].skill_id,MCT_SELF,ms[i].val[2],ms[i].val[3],&once_flag);
								if(md)
									map_foreachinarea( mobskill_command,
										md->bl.m,md->bl.x-range,md->bl.y-range,md->bl.x+range,md->bl.y+range,BL_MOB,md->bl.id,ms[i].skill_id,MCT_SLAVES,ms[i].val[2],ms[i].val[3],&once_flag);
							}
							break;
						case MCT_SELF:
						case MCT_FRIEND:
						case MCT_FRIENDS:
						case MCT_SLAVE:
						case MCT_SLAVES:
							{
								if(md)
									map_foreachinarea( mobskill_command,
										md->bl.m,md->bl.x-range,md->bl.y-range,md->bl.x+range,md->bl.y+range,BL_MOB,md->bl.id,ms[i].skill_id,ms[i].val[0],ms[i].val[2],ms[i].val[3],&once_flag);
							}
							break;
						}
						md->skilldelay[ i ] = gettick() + ms[i].delay;
					}
						break;
					case MST_MODECHANGE:
					{
						int once_flag=0;
						int range = ms[i].val[1];
						switch(ms[i].val[0])
						{
							case MCT_SELF:
								if(md)
									md->mode = ms[i].val[2];
								break;
							case MCT_SLAVE:
							case MCT_SLAVES:
							case MCT_FRIEND:
							case MCT_FRIENDS:
								if(md)
									map_foreachinarea( mobskill_modechange,
										md->bl.m,md->bl.x-range,md->bl.y-range,md->bl.x+range,md->bl.y+range,BL_MOB,md->bl.id,ms[i].val[0],ms[i].val[2],&once_flag);
								break;
							case MCT_GROUP:
							{
								if(md)
									md->mode = ms[i].val[2];
								if(md)
									map_foreachinarea( mobskill_modechange,
										md->bl.m,md->bl.x-range,md->bl.y-range,md->bl.x+range,md->bl.y+range,BL_MOB,md->bl.id,MCT_SLAVES,ms[i].val[2],&once_flag);
							}
							break;
						}
						md->skilldelay[ i ] = gettick() + ms[i].delay;
					}
						break;
					case MST_TARGETCHANGE:
						if(md)
							md->skilldelay[ i ] = gettick() + ms[i].delay;
						break;
				}
			}
			if(ms[i].emotion >= 0)
				clif_emotion(&md->bl,ms[i].emotion);
			return 1;
		}
	}

	return 0;
}

/*==========================================
 * スキル使用イベント処理
 *------------------------------------------
 */
int mobskill_event(struct mob_data *md,int flag)
{
	nullpo_retr(0, md);

	if(flag==-1 && mobskill_use(md,gettick(),MSC_CASTTARGETED))
		return 1;
	if( (flag&BF_SHORT) && mobskill_use(md,gettick(),MSC_CLOSEDATTACKED))
		return 1;
	if( (flag&BF_LONG) && mobskill_use(md,gettick(),MSC_LONGRANGEATTACKED))
		return 1;
	return 0;
}
/*==========================================
 * Mobがエンペリウムなどの場合の判定
 *------------------------------------------
 */
int mob_gvmobcheck(struct map_session_data *sd, struct block_list *bl)
{
	struct mob_data *md=NULL;

	nullpo_retr(0,sd);
	nullpo_retr(0,bl);

	if(bl->type==BL_MOB && (md=(struct mob_data *)bl) &&
		(md->class == 1288 || md->class == 1287 || md->class == 1286 || md->class == 1285))
	{
		struct guild_castle *gc=guild_mapname2gc(map[sd->bl.m].name);
		struct guild *g=guild_search(sd->status.guild_id);

		if(g == NULL && md->class == 1288)
			return 0;//ギルド未加入ならダメージ無し
		else if(gc != NULL && !map[sd->bl.m].flag.gvg)
			return 0;//砦内でGvじゃないときはダメージなし
		else if(g && gc != NULL && g->guild_id == gc->guild_id)
			return 0;//自占領ギルドのエンペならダメージ無し
		else if(g && guild_checkskill(g,GD_APPROVAL) <= 0 && md->class == 1288)
			return 0;//正規ギルド承認がないとダメージ無し
		else if (g && gc && guild_check_alliance(gc->guild_id, g->guild_id, 0) == 1)
			return 0;	// 同盟ならダメージ無し

	}

	return 1;
}

/*==========================================
 * スキル用タイマー削除
 *------------------------------------------
 */
int mobskill_deltimer(struct mob_data *md )
{
	nullpo_retr(0, md);

	if( md->ud.skilltimer!=-1 ){
		if( skill_get_inf( md->ud.skillid )&2 )
			delete_timer( md->ud.skilltimer, skill_castend_pos );
		else
			delete_timer( md->ud.skilltimer, skill_castend_id );
		md->ud.skilltimer=-1;
	}
	return 0;
}
//
// 初期化
//
/*==========================================
 * 未設定mobが使われたので暫定初期値設定
 *------------------------------------------
 */
static int mob_makedummymobdb(int class)
{
	int i;

	sprintf(mob_db[class].name,"mob%d",class);
	sprintf(mob_db[class].jname,"mob%d",class);
	mob_db[class].lv=1;
	mob_db[class].max_hp=1000;
	mob_db[class].max_sp=1;
	mob_db[class].base_exp=2;
	mob_db[class].job_exp=1;
	mob_db[class].range=1;
	mob_db[class].atk1=7;
	mob_db[class].atk2=10;
	mob_db[class].def=0;
	mob_db[class].mdef=0;
	mob_db[class].str=1;
	mob_db[class].agi=1;
	mob_db[class].vit=1;
	mob_db[class].int_=1;
	mob_db[class].dex=6;
	mob_db[class].luk=2;
	mob_db[class].range2=10;
	mob_db[class].range3=10;
	mob_db[class].size=0;
	mob_db[class].race=0;
	mob_db[class].element=0;
	mob_db[class].mode=0;
	mob_db[class].speed=300;
	mob_db[class].adelay=1000;
	mob_db[class].amotion=500;
	mob_db[class].dmotion=500;
	mob_db[class].dropitem[0].nameid=909;	// Jellopy
	mob_db[class].dropitem[0].p=1000;
	for(i=1;i<10;i++){
		mob_db[class].dropitem[i].nameid=0;
		mob_db[class].dropitem[i].p=0;
	}
	mob_db[class].mexp=0;
	mob_db[class].mexpper=0;
	for(i=0;i<3;i++){
		mob_db[class].mvpitem[i].nameid=0;
		mob_db[class].mvpitem[i].p=0;
	}
	for(i=0;i<MAX_RANDOMMONSTER;i++)
		mob_db[class].summonper[i]=0;
	return 0;
}

/*==========================================
 * db/mob_db.txt読み込み
 *------------------------------------------
 */
static int mob_readdb(void)
{
	FILE *fp;
	char line[1024];
	char *filename[]={ "db/mob_db.txt","db/mob_db2.txt" };
	int n;

	memset(mob_db_real,0,sizeof(mob_db_real));

	for(n=0;n<2;n++){
		fp=fopen(filename[n],"r");
		if(fp==NULL){
			if(n>0)
				continue;
			return -1;
		}
		while(fgets(line,1020,fp)){
			int class,i,cov=0;
			char *str[57],*p,*np;

			if(line[0] == '/' && line[1] == '/')
				continue;

			for(i=0,p=line;i<57;i++){
				if((np=strchr(p,','))!=NULL){
					str[i]=p;
					*np=0;
					p=np+1;
				} else
					str[i]=p;
			}

			class=atoi(str[0]);
			if(class<=1000 || class>MOB_ID_MAX)
				continue;
			if(n==1 && mob_db[class].view_class == class)
				cov=1;	// item_db2による、すでに登録のあるIDの上書きかどうか

			mob_db[class].view_class=class;
			// ここから先は、item_db2では記述のある部分のみ反映
			if(!cov || strlen(str[1])>0)
				memcpy(mob_db[class].name,str[1],24);
			if(!cov || strlen(str[2])>0)
				memcpy(mob_db[class].jname,str[2],24);
			#define DB_ADD(a,b) a=((!cov)?atoi(str[b]):(strlen(str[b])>0)?atoi(str[b]):a)
			DB_ADD(mob_db[class].lv,3);
			DB_ADD(mob_db[class].max_hp,4);
			DB_ADD(mob_db[class].max_sp,5);
			DB_ADD(mob_db[class].base_exp,6);
			DB_ADD(mob_db[class].job_exp,7);
			DB_ADD(mob_db[class].range,8);
			DB_ADD(mob_db[class].atk1,9);
			DB_ADD(mob_db[class].atk2,10);
			DB_ADD(mob_db[class].def,11);
			DB_ADD(mob_db[class].mdef,12);
			DB_ADD(mob_db[class].str,13);
			DB_ADD(mob_db[class].agi,14);
			DB_ADD(mob_db[class].vit,15);
			DB_ADD(mob_db[class].int_,16);
			DB_ADD(mob_db[class].dex,17);
			DB_ADD(mob_db[class].luk,18);
			DB_ADD(mob_db[class].range2,19);
			DB_ADD(mob_db[class].range3,20);
			DB_ADD(mob_db[class].size,21);
			DB_ADD(mob_db[class].race,22);
			DB_ADD(mob_db[class].element,23);
			DB_ADD(mob_db[class].mode,24);
			DB_ADD(mob_db[class].speed,25);
			DB_ADD(mob_db[class].adelay,26);
			DB_ADD(mob_db[class].amotion,27);
			DB_ADD(mob_db[class].dmotion,28);
			// アイテムドロップの設定
			for(i=0;i<10;i++){
				int itemdrop,nameid;
				int* per = &mob_db[class].dropitem[i].p;
				nameid	 = ((!cov)?atoi(str[29+i*2]):(strlen(str[29+i*2])>0)?atoi(str[29+i*2]):mob_db[class].dropitem[i].nameid);
				mob_db[class].dropitem[i].nameid = (nameid==0)?512:nameid;	//id=0は、リンゴに置き換え
				if(cov && strlen(str[30+i*2])==0)
					continue;
				itemdrop = atoi(str[30+i*2]);

				if (battle_config.item_rate_details==1) {	//ドロップレート詳細項目が1の時 レート=x/100倍
					if (itemdrop < 10)
						*per = itemdrop*battle_config.item_rate_1/100;
					else if (itemdrop < 100)
						*per = itemdrop*battle_config.item_rate_10/100;
					else if (itemdrop < 1000)
						*per = itemdrop*battle_config.item_rate_100/100;
					else
						*per = itemdrop*battle_config.item_rate_1000/100;
				}
				else if (battle_config.item_rate_details==2) {	//ドロップレート詳細項目が2の時　レート=x/100倍 min max 指定
					#define SETRATE2( min1, max1, num )	\
						if (itemdrop >= min1 && itemdrop < max1 ) {	\
							if (itemdrop*battle_config.item_rate_##num/100 < battle_config.item_rate_##num##_min)	\
								*per = battle_config.item_rate_##num##_min;	\
							else if (itemdrop*battle_config.item_rate_##num/100 > battle_config.item_rate_##num##_max)	\
								*per = battle_config.item_rate_##num##_max;	\
							else	\
								*per = itemdrop*battle_config.item_rate_##num/100;	\
						} // end of SETRATE2

					SETRATE2(    1,   10,   1 );
					SETRATE2(   10,  100,  10 );
					SETRATE2(  100, 1000, 100 );
					SETRATE2( 1000,10001,1000 );
					#undef SETRATE2
				}
				else if (battle_config.item_rate_details==0)	//ドロップレート詳細項目が0の時
					*per = itemdrop*battle_config.item_rate/100;

				// カードや装備などアイテムの種類によるレート変化
				if (nameid >= 4001 && nameid<= 4331 ){
					*per = *per * battle_config.card_drop_rate/100;
				}
				else if ((nameid >= 1101 && nameid<= 2670 ) || (nameid >= 5001 && nameid<= 5150 )|| (nameid >= 13000 && nameid<= 13010 )){
					*per = *per * battle_config.equip_drop_rate/100;
				}
				else if (nameid == 756 || nameid == 757 || nameid == 984 || nameid == 985){
					*per = *per * battle_config.refine_drop_rate/100;
				}
			}
			DB_ADD(mob_db[class].mexp,49);
			DB_ADD(mob_db[class].mexpper,50);
			for(i=0;i<3;i++){
				mob_db[class].mvpitem[i].nameid	=	DB_ADD(mob_db[class].mvpitem[i].nameid,51+i*2);
				mob_db[class].mvpitem[i].p		=	(!cov)?atoi(str[52+i*2])*battle_config.mvp_item_rate/100:(atoi(str[52+i*2])>0)?atoi(str[52+i*2])*battle_config.mvp_item_rate/100:mob_db[class].mvpitem[i].p;
			}
			for(i=0;i<MAX_RANDOMMONSTER;i++)
				mob_db[class].summonper[i]=0;
			mob_db[class].maxskill=0;

			mob_db[class].sex=0;
			mob_db[class].hair=0;
			mob_db[class].hair_color=0;
			mob_db[class].weapon=0;
			mob_db[class].shield=0;
			mob_db[class].head_top=0;
			mob_db[class].head_mid=0;
			mob_db[class].head_buttom=0;
		}
		fclose(fp);
		printf("read %s done\n",filename[n]);
	}

	//group_db
	fp=fopen("db/mob_group_db.txt","r");
	if(fp==NULL) return 0;//無くても成功 group_id = 0 未分類のため

	while(fgets(line,1020,fp)){
		int class,i;
		char *str[55],*p,*np;

		if(line[0] == '/' && line[1] == '/')
			continue;

		for(i=0,p=line;i<3;i++){
			if((np=strchr(p,','))!=NULL)
			{
				str[i]=p;
				*np=0;
				p=np+1;
			} else
				str[i]=p;
		}

		class=atoi(str[0]);
		if(class<=1000 || class>MOB_ID_MAX)
			continue;

		mob_db[class].group_id=atoi(str[2]);
	}
	fclose(fp);

	printf("read db/mob_group_db.txt done\n");

	return 0;
}


/*==========================================
 * MOB表示グラフィック変更データ読み込み
 *------------------------------------------
 */
static int mob_readdb_mobavail(void)
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int class,j,k;
	char *str[20],*p,*np;

	if( (fp=fopen("db/mob_avail.txt","r"))==NULL ){
		printf("can't read db/mob_avail.txt\n");
		return -1;
	}

	while(fgets(line,1020,fp)){
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(str,0,sizeof(str));

		for(j=0,p=line;j<12;j++){
			if((np=strchr(p,','))!=NULL){
				str[j]=p;
				*np=0;
				p=np+1;
			} else
					str[j]=p;
			}

		if(str[0]==NULL)
			continue;

		class=atoi(str[0]);

		if(class<=1000 || class>MOB_ID_MAX)	// 値が異常なら処理しない。
			continue;
		k=atoi(str[1]);
		if(k >= 0)
			mob_db[class].view_class=k;

		if(mob_db[class].view_class >= 0 && mob_db[class].view_class < MAX_VALID_PC_CLASS) {
			mob_db[class].sex=atoi(str[2]);
			mob_db[class].hair=atoi(str[3]);
			mob_db[class].hair_color=atoi(str[4]);
			mob_db[class].weapon=atoi(str[5]);
			mob_db[class].shield=atoi(str[6]);
			mob_db[class].head_top=atoi(str[7]);
			mob_db[class].head_mid=atoi(str[8]);
			mob_db[class].head_buttom=atoi(str[9]);
			mob_db[class].option=atoi(str[10])&~0x46;
			mob_db[class].trans=atoi(str[11]);
		}
		ln++;
	}
	fclose(fp);
	printf("read db/mob_avail.txt done (count=%d)\n",ln);
	return 0;
}

/*==========================================
 * ランダムモンスターデータの読み込み
 *------------------------------------------
 */
static int mob_read_randommonster(void)
{
	FILE *fp;
	char line[1024];
	char *str[10],*p;
	int i,j;

	const char* mobfile[] = {
		"db/mob_branch.txt",
		"db/mob_poring.txt",
		"db/mob_boss.txt" };

	for(i=0;i<MAX_RANDOMMONSTER;i++){
		mob_db[MOB_ID_MIN].summonper[i] = 1002;	// 設定し忘れた場合はポリンが出るようにしておく
		fp=fopen(mobfile[i],"r");
		if(fp==NULL){
			printf("can't read %s\n",mobfile[i]);
			return -1;
		}
		while(fgets(line,1020,fp)){
			int class,per;
			if(line[0] == '/' && line[1] == '/')
				continue;
			memset(str,0,sizeof(str));
			for(j=0,p=line;j<3 && p;j++){
				str[j]=p;
				p=strchr(p,',');
				if(p) *p++=0;
			}

			if(str[0]==NULL || str[2]==NULL)
				continue;

			class = atoi(str[0]);
			per=atoi(str[2]);
			if(class>1000 && class<=MOB_ID_MAX)
				mob_db[class].summonper[i]=per;
			else if( class == 0 )
				mob_db[MOB_ID_MIN].summonper[i]=per;
		}
		fclose(fp);
		printf("read %s done\n",mobfile[i]);
	}
	return 0;
}
/*==========================================
 * db/mob_skill_db.txt読み込み
 *------------------------------------------
 */

/*==========================================
 * db/mob_skill_db.txt読み込み
 *------------------------------------------
 */
static int mob_readskilldb(void)
{
	FILE *fp;
	char line[1024];
	int i;

	const struct {
		char str[32];
		int id;
	} cond1[] = {
		{	"always",			MSC_ALWAYS				},
		{	"myhpltmaxrate",	MSC_MYHPLTMAXRATE		},
		{	"friendhpltmaxrate",MSC_FRIENDHPLTMAXRATE	},
		{	"mystatuson",		MSC_MYSTATUSON			},
		{	"mystatusoff",		MSC_MYSTATUSOFF			},
		{	"friendstatuson",	MSC_FRIENDSTATUSON		},
		{	"friendstatusoff",	MSC_FRIENDSTATUSOFF		},
		{	"attackpcgt",		MSC_ATTACKPCGT			},
		{	"attackpcge",		MSC_ATTACKPCGE			},
		{	"slavelt",			MSC_SLAVELT				},
		{	"slavele",			MSC_SLAVELE				},
		{	"closedattacked",	MSC_CLOSEDATTACKED		},
		{	"longrangeattacked",MSC_LONGRANGEATTACKED	},
		{	"skillused",		MSC_SKILLUSED			},
		{	"casttargeted",		MSC_CASTTARGETED		},
		{	"targethpgtmaxrate",MSC_TARGETHPGTMAXRATE	},
		{	"targethpltmaxrate",MSC_TARGETHPLTMAXRATE	},
		{	"targethpgt",		MSC_TARGETHPGT			},
		{	"targethplt",		MSC_TARGETHPLT			},
		{	"targetstatuson",	MSC_TARGETSTATUSON		},
		{	"targetstatusoff",	MSC_TARGETSTATUSOFF		},
		{	"masterhpgtmaxrate",MSC_MASTERHPGTMAXRATE	},
		{	"masterhpltmaxrate",MSC_MASTERHPLTMAXRATE	},
		{	"masterstatuson",	MSC_MASTERSTATUSON		},
		{	"masterstatusoff",	MSC_MASTERSTATUSOFF		},
		{	"areaslavegt",		MSC_AREASLAVEGT			},
		{	"areaslavele",		MSC_AREASLAVELE			},
	}, cond2[] ={
		{	"anybad",		-1				},
		{	"stone",		SC_STONE		},
		{	"freeze",		SC_FREEZE		},
		{	"stan",			SC_STAN			},
		{	"sleep",		SC_SLEEP		},
		{	"poison",		SC_POISON		},
		{	"curse",		SC_CURSE		},
		{	"silence",		SC_SILENCE		},
		{	"confusion",	SC_CONFUSION	},
		{	"blind",		SC_BLIND		},
		{	"hiding",		SC_HIDING		},
		{	"sight",		SC_SIGHT		},
		{	"lexaeterna",	SC_AETERNA		},
	}, state[] = {
		{	"any",		MSS_ANY		},
		{	"idle",		MSS_IDLE	},
		{	"walk",		MSS_WALK	},
		{	"attack",	MSS_ATTACK	},
		{	"dead",		MSS_DEAD	},
		{	"loot",		MSS_LOOT	},
		{	"chase",	MSS_CHASE	},
		{   "command",  MSS_COMMANDONLY}
	}, target[] = {
		{	"target",	MST_TARGET	},
		{	"self",		MST_SELF	},
		{	"friend",	MST_FRIEND	},
		{	"master",	MST_MASTER	},
		{	"slave",	MST_SLAVE	},
		{	"command",	MST_COMMAND },
		{	"modechange",MST_MODECHANGE	},
		{	"targetchange", MST_TARGETCHANGE},
		{	"around5",	MST_AROUND5	},
		{	"around6",	MST_AROUND6	},
		{	"around7",	MST_AROUND7	},
		{	"around8",	MST_AROUND8	},
		{	"around1",	MST_AROUND1	},
		{	"around2",	MST_AROUND2	},
		{	"around3",	MST_AROUND3	},
		{	"around4",	MST_AROUND4	},
		{	"around",	MST_AROUND	},
	}, command_target[] = {
		{ "target", 	MCT_TARGET 	},
		{ "self", 		MCT_SELF 	},
		{ "commander",	MCT_COMMANDER},
		{ "slave", 		MCT_SLAVE 	},
		{ "slaves", 	MCT_SLAVES	},
		{ "group", 		MCT_GROUP	},
		{ "friend",		MCT_FRIEND  },
		{ "friends",  	MCT_FRIENDS },
		{ "master",		MCT_MASTER	},
	};

	int x, lineno;
	char *filename[]={ "db/mob_skill_db.txt","db/mob_skill_db2.txt" };

	for(x=0;x<2;x++){

		fp=fopen(filename[x],"r");
		if(fp==NULL){
			if(x==0)
				printf("can't read %s\n",filename[x]);
			continue;
		}
		lineno = 0;
		while(fgets(line,1020,fp)){
			char *sp[20],*p;
			int mob_id;
			struct mob_skill *ms;
			int j=0;

			lineno++;
			if(line[0] == '/' && line[1] == '/')
				continue;

			memset(sp,0,sizeof(sp));
			for(i=0,p=line;i<18 && p;i++){
				sp[i]=p;
				if((p=strchr(p,','))!=NULL) {
					*p++=0;
				} else {
					break;
				}
			}
			if( (mob_id=atoi(sp[0]))<=0 )
				continue;

			if( strcmp(sp[1],"clear")==0 ){
				memset(mob_db[mob_id].skill,0,sizeof(mob_db[mob_id].skill));
				mob_db[mob_id].maxskill=0;
				continue;
			}

			if( i != 17 ) {
				printf("mob_skill: invalid param count(%d) line %d\n", i, lineno);
				continue;
			}

			for(i=0;i<MAX_MOBSKILL;i++)
				if( (ms=&mob_db[mob_id].skill[i])->skill_id == 0)
					break;
			if(i==MAX_MOBSKILL){
				printf("mob_skill: readdb: too many skill ! [%s] in %d[%s]\n",
					sp[1],mob_id,mob_db[mob_id].jname);
				continue;
			}

			// ms->state = -1;
			for(j=0;j<sizeof(state)/sizeof(state[0]);j++){
				if( strcmp(sp[2],state[j].str)==0) {
					ms->state=state[j].id;
					break;
				}
			}
			if( j == sizeof(state)/sizeof(state[0]) ) {
				ms->state = MSS_COMMANDONLY; // 無効にする
				printf("mob_skill: unknown state %s line %d\n", sp[2], lineno);
			}
			ms->skill_id=atoi(sp[3]);
			ms->skill_lv=atoi(sp[4]);
			ms->permillage=atoi(sp[5]);
			ms->casttime=atoi(sp[6]);
			ms->delay=atoi(sp[7]);
			ms->cancel=atoi(sp[8]);
			if( strcmp(sp[8],"yes")==0 ) ms->cancel=1;

			// ms->target=atoi(sp[9]);
			for(j=0;j<sizeof(target)/sizeof(target[0]);j++){
				if( strcmp(sp[9],target[j].str)==0) {
					ms->target=target[j].id;
					break;
				}
			}
			if( j == sizeof(target)/sizeof(target[0]) ) {
				ms->state = MSS_DISABLE; // 無効にする
				printf("mob_skill: unknown target %s line %d\n", sp[9], lineno);
			}

			// ms->cond1=-1;
			for(j=0;j<sizeof(cond1)/sizeof(cond1[0]);j++){
				if( strcmp(sp[10],cond1[j].str)==0) {
					ms->cond1=cond1[j].id;
					break;
				}
			}
			if( j == sizeof(cond1)/sizeof(cond1[0]) ) {
				ms->state = MSS_DISABLE; // 無効にする
				printf("mob_skill: unknown cond1 %s line %d\n", sp[10], lineno);
			}

			ms->cond2=atoi(sp[11]);
			for(j=0;j<sizeof(cond2)/sizeof(cond2[0]);j++){
				if( strcmp(sp[11],cond2[j].str)==0) {
					ms->cond2=cond2[j].id;
					break;
				}
			}
			if(
				(ms->cond1 == MSC_MYSTATUSON	    || ms->cond1 == MSC_MYSTATUSOFF	||
				 ms->cond1 == MSC_FRIENDSTATUSON	|| ms->cond1 == MSC_FRIENDSTATUSOFF) ^
				( j != sizeof(cond2)/sizeof(cond2[0]) )
			) {
				ms->state = MSS_DISABLE; // 無効にする
				printf("mob_skill: invalid combination (%s,%s) line %d\n", sp[10], sp[11], lineno );
			}

			ms->val[0]=atoi(sp[12]);
			for(j=0;j<sizeof(command_target)/sizeof(command_target[0]);j++){
				if( strcmp(sp[12],command_target[j].str)==0) {
					ms->val[0]=command_target[j].id;
					break;
				}
			}
			if( j == sizeof(command_target)/sizeof(command_target[0]) ) {
				if( isalpha( (unsigned char)sp[12][0] ) ) {
					ms->state = MSS_DISABLE; // 無効にする
					printf("mob_skill: unknown val0 %s line %d\n", sp[12], lineno );
				}
			}

			ms->val[1]=atoi(sp[13]);
			for(j=0;j<sizeof(command_target)/sizeof(command_target[0]);j++){
				if( strcmp(sp[13],command_target[j].str)==0) {
					ms->val[1]=command_target[j].id;
					break;
				}
			}
			if( j == sizeof(command_target)/sizeof(command_target[0]) ) {
				if( isalpha( (unsigned char)sp[13][0] ) ) {
					ms->state = MSS_DISABLE; // 無効にする
					printf("mob_skill: unknown val1 %s line %d\n", sp[13], lineno );
				}
			}

			ms->val[2]=atoi(sp[14]);
			for(j=0;j<sizeof(command_target)/sizeof(command_target[0]);j++){
				if( strcmp(sp[14],command_target[j].str)==0) {
					ms->val[2]=command_target[j].id;
					break;
				}
			}
			if( j == sizeof(command_target)/sizeof(command_target[0]) ) {
				if( isalpha( (unsigned char)sp[14][0] ) ) {
					ms->state = MSS_DISABLE; // 無効にする
					printf("mob_skill: unknown val2 %s line %d\n", sp[14], lineno );
				}
			}

			ms->val[3]=atoi(sp[15]);
			ms->val[4]=atoi(sp[16]);
			if(strlen(sp[17])>2)
				ms->emotion=atoi(sp[17]);
			else
				ms->emotion=-1;
			mob_db[mob_id].maxskill=i+1;
		}
		fclose(fp);
		printf("read %s done\n",filename[x]);
	}
	return 0;
}

void mob_reload(void)
{
	/*

	<empty monster database>
	mob_read();

	*/

	/*do_init_mob();*/
	mob_readdb();
	mob_readdb_mobavail();
	mob_read_randommonster();
	mob_readskilldb();
}

/*==========================================
 * mob周り初期化
 *------------------------------------------
 */
int do_init_mob(void)
{
	mob_readdb();
	mob_readdb_mobavail();
	mob_read_randommonster();
	mob_readskilldb();

	add_timer_func_list(mob_delayspawn,"mob_delayspawn");
	add_timer_func_list(mob_delay_item_drop,"mob_delay_item_drop");
	add_timer_func_list(mob_delay_item_drop2,"mob_delay_item_drop2");
	add_timer_func_list(mob_ai_hard,"mob_ai_hard");
	add_timer_func_list(mob_ai_lazy,"mob_ai_lazy");
	add_timer_func_list(mob_timer_delete,"mob_timer_delete");
	add_timer_interval(gettick()+MIN_MOBTHINKTIME,mob_ai_hard,0,0,MIN_MOBTHINKTIME);
	add_timer_interval(gettick()+MIN_MOBTHINKTIME*10,mob_ai_lazy,0,0,MIN_MOBTHINKTIME*10);

	return 0;
}
