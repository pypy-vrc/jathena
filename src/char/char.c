#define DUMP_UNKNOWN_PACKET	1

#include <sys/types.h>
#ifndef _WIN32
	#include <sys/socket.h>
	#include <sys/ioctl.h>
	#include <unistd.h>
	#include <signal.h>
	#include <fcntl.h>
	#include <netdb.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <sys/time.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _CHAR_C_

#include "core.h"
#include "socket.h"
#include "timer.h"
#include "db.h"
#include "mmo.h"
#include "version.h"
#include "lock.h"
#include "nullpo.h"
#include "malloc.h"
#include "char.h"
#include "httpd.h"
#include "graph.h"
#include "journal.h"

#include "inter.h"
#include "int_pet.h"
#include "int_homun.h"
#include "int_guild.h"
#include "int_party.h"
#include "int_storage.h"
#include "int_mail.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

struct mmo_map_server server[MAX_MAP_SERVERS];
int server_fd[MAX_MAP_SERVERS];

int  login_fd,char_fd,char_sfd;
char userid[24];
char passwd[24];
char server_name[20];
char login_ip_str[16];
int  login_ip;
int  login_port = 6900;
char char_ip_str[16];
int  char_ip;
int  char_port = 6121;
char char_sip_str[16];
unsigned long  char_sip = 0;
int  char_sport = 0;
int  char_loginaccess_autorestart;
int  char_maintenance;
int  char_new;
char unknown_char_name[1024]="Unknown";
char char_log_filename[1024]="log/char.log";
char GM_account_filename[1024] = "conf/GM_account.txt";
char default_map_name[16]="prontera.gat";
int  default_map_type=0;

int char_log(char *fmt,...);
int parse_char(int fd);

#ifdef TXT_JOURNAL
static int char_journal_enable = 1;
static struct journal char_journal;
static char char_journal_file[1024]="./save/athena.journal";
static int char_journal_cache = 1000;
#endif

// online DB
static struct dbt *char_online_db;

#define CHAR_STATE_WAITAUTH 0
#define CHAR_STATE_AUTHOK 1

struct {
  int account_id,char_id,login_id1,login_id2,ip,tick,delflag,sex;
} auth_fifo[AUTH_FIFO_SIZE];
int auth_fifo_pos=0;

int max_connect_user=0;
int autosave_interval=DEFAULT_AUTOSAVE_INTERVAL_CS;
int start_zeny = 500;

// 初期位置（confファイルから再設定可能）
struct point start_point={"new_1-1.gat",53,111};
int search_mapserver(char *map);

#ifdef TXT_ONLY
static struct mmo_charstatus *char_dat;
static int  char_num,char_max;
static char char_txt[1024];
static int  char_id_count=150000;

static int mmo_char_tostr(char *str,struct mmo_charstatus *p)
{
	int i;
	char *str_p = str;
	unsigned short sk_lv;

	nullpo_retr(-1,p);

	str_p += sprintf(str_p,"%d\t%d,%d\t%s\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
		"\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
		"\t%s,%d,%d\t%s,%d,%d,%d,%d,%d,%d\t",
		p->char_id,p->account_id,p->char_num,p->name, //
		p->class,p->base_level,p->job_level,
		p->base_exp,p->job_exp,p->zeny,
		p->hp,p->max_hp,p->sp,p->max_sp,
		p->str,p->agi,p->vit,p->int_,p->dex,p->luk,
		p->status_point,p->skill_point,
		p->option,p->karma,p->manner,	//
		p->party_id,p->guild_id,p->pet_id,p->homun_id,
		p->hair,p->hair_color,p->clothes_color,
		p->weapon,p->shield,p->head_top,p->head_mid,p->head_bottom,
		p->last_point.map,p->last_point.x,p->last_point.y, //
		p->save_point.map,p->save_point.x,p->save_point.y,
		p->partner_id,p->parent_id[0],p->parent_id[1],p->baby_id
	);
	for(i = 0; i < MAX_PORTAL_MEMO; i++)
		if(p->memo_point[i].map[0])
			str_p += sprintf(str_p,"%s,%d,%d",p->memo_point[i].map,p->memo_point[i].x,p->memo_point[i].y);

	*(str_p++)='\t';

	for(i=0;i<MAX_INVENTORY;i++)
		if(p->inventory[i].nameid){
			str_p += sprintf(str_p,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d ",
			p->inventory[i].id,p->inventory[i].nameid,p->inventory[i].amount,p->inventory[i].equip,
			p->inventory[i].identify,p->inventory[i].refine,p->inventory[i].attribute,
			p->inventory[i].card[0],p->inventory[i].card[1],p->inventory[i].card[2],p->inventory[i].card[3]);
		}
	*(str_p++)='\t';

	for(i=0;i<MAX_CART;i++)
		if(p->cart[i].nameid){
			str_p += sprintf(str_p,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d ",
			p->cart[i].id,p->cart[i].nameid,p->cart[i].amount,p->cart[i].equip,
			p->cart[i].identify,p->cart[i].refine,p->cart[i].attribute,
			p->cart[i].card[0],p->cart[i].card[1],p->cart[i].card[2],p->cart[i].card[3]);
		}
	*(str_p++)='\t';

	for(i=0;i<MAX_SKILL;i++)
		if(p->skill[i].id && p->skill[i].flag!=1 && p->skill[i].flag!=CLONE_SKILL_FLAG){
			if(p->skill[i].flag == CLONE_SKILL_FLAG)
				sk_lv = p->skill[i].lv + CLONE_SKILL_FLAG;
			else
				sk_lv = (p->skill[i].flag==0)?p->skill[i].lv:p->skill[i].flag-2;
			str_p += sprintf(str_p,"%d,%d ",p->skill[i].id,sk_lv);
		}
	*(str_p++)='\t';

	for(i=0;i<p->global_reg_num;i++)
		str_p += sprintf(str_p,"%s,%d ",p->global_reg[i].str,p->global_reg[i].value);
	*(str_p++)='\t';

	for(i=0;i<p->friend_num;i++)
		str_p += sprintf(str_p,"%d,%d ",p->friend_data[i].account_id, p->friend_data[i].char_id );
	*(str_p++)='\t';

	*str_p='\0';
	return 0;
}

static int mmo_char_fromstr(char *str,struct mmo_charstatus *p)
{
	int tmp_int[256];
	int set,next,len,i;

	nullpo_retr(0,p);
	// 1882以降の形式読み込み
	if( (set=sscanf(str,"%d\t%d,%d\t%[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
		"\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
		"\t%[^,],%d,%d\t%[^,],%d,%d,%d,%d,%d,%d%n",
		&tmp_int[0],&tmp_int[1],&tmp_int[2],p->name, //
		&tmp_int[3],&tmp_int[4],&tmp_int[5],
		&tmp_int[6],&tmp_int[7],&tmp_int[8],
		&tmp_int[9],&tmp_int[10],&tmp_int[11],&tmp_int[12],
		&tmp_int[13],&tmp_int[14],&tmp_int[15],&tmp_int[16],&tmp_int[17],&tmp_int[18],
		&tmp_int[19],&tmp_int[20],
		&tmp_int[21],&tmp_int[22],&tmp_int[23], //
		&tmp_int[24],&tmp_int[25],&tmp_int[26],&tmp_int[43],
		&tmp_int[27],&tmp_int[28],&tmp_int[29],
		&tmp_int[30],&tmp_int[31],&tmp_int[32],&tmp_int[33],&tmp_int[34],
		p->last_point.map,&tmp_int[35],&tmp_int[36], //
		p->save_point.map,&tmp_int[37],&tmp_int[38],&tmp_int[39],&tmp_int[40],&tmp_int[41],&tmp_int[42],&next
		)
	)!=47 ){
		tmp_int[43]=0;
		// 1426以降の形式読み込み
		if( (set=sscanf(str,"%d\t%d,%d\t%[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
			"\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
			"\t%[^,],%d,%d\t%[^,],%d,%d,%d,%d,%d,%d%n",
			&tmp_int[0],&tmp_int[1],&tmp_int[2],p->name, //
			&tmp_int[3],&tmp_int[4],&tmp_int[5],
			&tmp_int[6],&tmp_int[7],&tmp_int[8],
			&tmp_int[9],&tmp_int[10],&tmp_int[11],&tmp_int[12],
			&tmp_int[13],&tmp_int[14],&tmp_int[15],&tmp_int[16],&tmp_int[17],&tmp_int[18],
			&tmp_int[19],&tmp_int[20],
			&tmp_int[21],&tmp_int[22],&tmp_int[23], //
			&tmp_int[24],&tmp_int[25],&tmp_int[26],
			&tmp_int[27],&tmp_int[28],&tmp_int[29],
			&tmp_int[30],&tmp_int[31],&tmp_int[32],&tmp_int[33],&tmp_int[34],
			p->last_point.map,&tmp_int[35],&tmp_int[36], //
			p->save_point.map,&tmp_int[37],&tmp_int[38],&tmp_int[39],&tmp_int[40],&tmp_int[41],&tmp_int[42],&next
			)
		)!=46 ){
			tmp_int[40]=0;
			tmp_int[41]=0;
			tmp_int[42]=0;
			 // 1008以降の形式読み込み
			if( (set=sscanf(str,"%d\t%d,%d\t%[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
				"\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
				"\t%[^,],%d,%d\t%[^,],%d,%d,%d%n",
				&tmp_int[0],&tmp_int[1],&tmp_int[2],p->name, //
				&tmp_int[3],&tmp_int[4],&tmp_int[5],
				&tmp_int[6],&tmp_int[7],&tmp_int[8],
				&tmp_int[9],&tmp_int[10],&tmp_int[11],&tmp_int[12],
				&tmp_int[13],&tmp_int[14],&tmp_int[15],&tmp_int[16],&tmp_int[17],&tmp_int[18],
				&tmp_int[19],&tmp_int[20],
				&tmp_int[21],&tmp_int[22],&tmp_int[23], //
				&tmp_int[24],&tmp_int[25],&tmp_int[26],
				&tmp_int[27],&tmp_int[28],&tmp_int[29],
				&tmp_int[30],&tmp_int[31],&tmp_int[32],&tmp_int[33],&tmp_int[34],
				p->last_point.map,&tmp_int[35],&tmp_int[36], //
				p->save_point.map,&tmp_int[37],&tmp_int[38],&tmp_int[39],&next
				)
			)!=43 ){
				// 384以降1008以前の形式読み込み
				tmp_int[39]=0;
				if( (set=sscanf(str,"%d\t%d,%d\t%[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
					"\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
					"\t%[^,],%d,%d\t%[^,],%d,%d%n",
					&tmp_int[0],&tmp_int[1],&tmp_int[2],p->name, //
					&tmp_int[3],&tmp_int[4],&tmp_int[5],
					&tmp_int[6],&tmp_int[7],&tmp_int[8],
					&tmp_int[9],&tmp_int[10],&tmp_int[11],&tmp_int[12],
					&tmp_int[13],&tmp_int[14],&tmp_int[15],&tmp_int[16],&tmp_int[17],&tmp_int[18],
					&tmp_int[19],&tmp_int[20],
					&tmp_int[21],&tmp_int[22],&tmp_int[23], //
					&tmp_int[24],&tmp_int[25],&tmp_int[26],
					&tmp_int[27],&tmp_int[28],&tmp_int[29],
					&tmp_int[30],&tmp_int[31],&tmp_int[32],&tmp_int[33],&tmp_int[34],
					p->last_point.map,&tmp_int[35],&tmp_int[36], //
					p->save_point.map,&tmp_int[37],&tmp_int[38],&next
					)
				)!=42 ){
				// 384以前の形式の読み込み
					tmp_int[26]=0;
					set=sscanf(str,"%d\t%d,%d\t%[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
						"\t%d,%d,%d\t%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
						"\t%[^,],%d,%d\t%[^,],%d,%d%n",
						&tmp_int[0],&tmp_int[1],&tmp_int[2],p->name, //
						&tmp_int[3],&tmp_int[4],&tmp_int[5],
						&tmp_int[6],&tmp_int[7],&tmp_int[8],
						&tmp_int[9],&tmp_int[10],&tmp_int[11],&tmp_int[12],
						&tmp_int[13],&tmp_int[14],&tmp_int[15],&tmp_int[16],&tmp_int[17],&tmp_int[18],
						&tmp_int[19],&tmp_int[20],
						&tmp_int[21],&tmp_int[22],&tmp_int[23], //
						&tmp_int[24],&tmp_int[25],//
						&tmp_int[27],&tmp_int[28],&tmp_int[29],
						&tmp_int[30],&tmp_int[31],&tmp_int[32],&tmp_int[33],&tmp_int[34],
						p->last_point.map,&tmp_int[35],&tmp_int[36], //
						p->save_point.map,&tmp_int[37],&tmp_int[38],&next
						);
					set+=6;
					printf("char: old char data ver.1\n");
				}else{// 383~1008Verでの読み込みに成功しているならsetを正常値へ
					set+=5;
					printf("char: old char data ver.2\n");
				}
			}else{
				set+=4;
				printf("char: old char data ver.3\n");
			}
		}else{
			set+=1;
			printf("char: old char data ver.4\n");
		}
	}
	p->char_id=tmp_int[0];
	p->account_id=tmp_int[1];
	p->char_num=tmp_int[2];
	p->class=tmp_int[3];
	p->base_level=tmp_int[4];
	p->job_level=tmp_int[5];
	p->base_exp=tmp_int[6];
	p->job_exp=tmp_int[7];
	p->zeny=tmp_int[8];
	p->hp=tmp_int[9];
	p->max_hp=tmp_int[10];
	p->sp=tmp_int[11];
	p->max_sp=tmp_int[12];
	p->str=tmp_int[13];
	p->agi=tmp_int[14];
	p->vit=tmp_int[15];
	p->int_=tmp_int[16];
	p->dex=tmp_int[17];
	p->luk=tmp_int[18];
	p->status_point=tmp_int[19];
	p->skill_point=tmp_int[20];
	p->option=tmp_int[21];
	p->karma=tmp_int[22];
	p->manner=tmp_int[23];
	p->party_id=tmp_int[24];
	p->guild_id=tmp_int[25];
	p->pet_id=tmp_int[26];
	p->homun_id=tmp_int[43];
	p->hair=tmp_int[27];
	p->hair_color=tmp_int[28];
	p->clothes_color=tmp_int[29];
	p->weapon=tmp_int[30];
	p->shield=tmp_int[31];
	p->head_top=tmp_int[32];
	p->head_mid=tmp_int[33];
	p->head_bottom=tmp_int[34];
	p->last_point.x=tmp_int[35];
	p->last_point.y=tmp_int[36];
	p->save_point.x=tmp_int[37];
	p->save_point.y=tmp_int[38];
	p->partner_id=tmp_int[39];
	p->parent_id[0]=tmp_int[40];
	p->parent_id[1]=tmp_int[41];
	p->baby_id=tmp_int[42];
	if(set!=47)
		return 0;
	if(str[next]=='\n' || str[next]=='\r')
		return 1;	// 新規データ
	next++;
	for(i = 0; str[next] && str[next] != '\t' && i < MAX_PORTAL_MEMO; i++) {
		set=sscanf(str+next,"%[^,],%d,%d%n",p->memo_point[i].map,&tmp_int[0],&tmp_int[1],&len);
		if(set!=3)
	    	return 0;
		p->memo_point[i].x=tmp_int[0];
		p->memo_point[i].y=tmp_int[1];
		next+=len;
		if(str[next]==' ')
			next++;
	}
	next++;
	for(i=0;str[next] && str[next]!='\t';i++){
		set=sscanf(str+next,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
		&tmp_int[0],&tmp_int[1],&tmp_int[2],&tmp_int[3],
		&tmp_int[4],&tmp_int[5],&tmp_int[6],
		&tmp_int[7],&tmp_int[8],&tmp_int[9],&tmp_int[10],&len);
		if(set!=11)
			return 0;
		p->inventory[i].id=tmp_int[0];
		p->inventory[i].nameid=tmp_int[1];
		p->inventory[i].amount=tmp_int[2];
		p->inventory[i].equip=tmp_int[3];
		p->inventory[i].identify=tmp_int[4];
		p->inventory[i].refine=tmp_int[5];
		p->inventory[i].attribute=tmp_int[6];
		p->inventory[i].card[0]=tmp_int[7];
		p->inventory[i].card[1]=tmp_int[8];
		p->inventory[i].card[2]=tmp_int[9];
		p->inventory[i].card[3]=tmp_int[10];
		next+=len;
		if(str[next]==' ')
			next++;
	}
	next++;
	for(i=0;str[next] && str[next]!='\t';i++){
		set=sscanf(str+next,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
		&tmp_int[0],&tmp_int[1],&tmp_int[2],&tmp_int[3],
		&tmp_int[4],&tmp_int[5],&tmp_int[6],
		&tmp_int[7],&tmp_int[8],&tmp_int[9],&tmp_int[10],&len);
		if(set!=11)
			return 0;
		p->cart[i].id=tmp_int[0];
		p->cart[i].nameid=tmp_int[1];
		p->cart[i].amount=tmp_int[2];
		p->cart[i].equip=tmp_int[3];
		p->cart[i].identify=tmp_int[4];
		p->cart[i].refine=tmp_int[5];
		p->cart[i].attribute=tmp_int[6];
		p->cart[i].card[0]=tmp_int[7];
		p->cart[i].card[1]=tmp_int[8];
		p->cart[i].card[2]=tmp_int[9];
		p->cart[i].card[3]=tmp_int[10];
		next+=len;
		if(str[next]==' ')
			next++;
	}
	next++;
	for(i=0;str[next] && str[next]!='\t';i++){
		set=sscanf(str+next,"%d,%d%n",
		&tmp_int[0],&tmp_int[1],&len);
		if(set!=2)
			return 0;
		if(ATHENA_MOD_VERSION<=1586 && tmp_int[1]==11){
			next+=len;
			continue;
		}
		p->skill[tmp_int[0]].id=tmp_int[0];
		if(tmp_int[1] > CLONE_SKILL_FLAG){
			p->skill[tmp_int[0]].lv = tmp_int[1] - CLONE_SKILL_FLAG;
			p->skill[tmp_int[0]].flag = CLONE_SKILL_FLAG;
		}else
			p->skill[tmp_int[0]].lv = tmp_int[1];
		next+=len;
		if(str[next]==' ')
			next++;
	}
	next++;
	for(i=0;str[next] && str[next]!='\t' && str[next]!='\n' && str[next]!='\r';i++){ //global_reg実装以前のathena.txt互換のため一応'\n'チェック
		set=sscanf(str+next,"%[^,],%d%n",
		p->global_reg[i].str,&p->global_reg[i].value,&len);
		if(set!=2)
			return 0;
		next+=len;
		if(str[next]==' ')
			next++;
	}
	p->global_reg_num=i;
	next++;

	for(i=0;str[next] && str[next]!='\t' && str[next]!='\n' && str[next]!='\r';i++){ //friend実装以前のathena.txt互換のため一応'\n'チェック
		set=sscanf(str+next,"%d,%d%n",
			&p->friend_data[i].account_id, &p->friend_data[i].char_id, &len ); // name は後で解決する
		if(set!=2)
			return 0;
		next+=len;
		if(str[next]==' ')
			next++;
	}
	p->friend_num=i;
	next++;

	return 1;
}

void char_txt_sync(void);

#ifdef TXT_JOURNAL
// ==========================================
// キャラクターデータのジャーナルのロールフォワード用コールバック関数
// ------------------------------------------
int char_journal_rollforward( int key, void* buf, int flag )
{
	int i=0;

	// 念のためチェック
	if( flag == JOURNAL_FLAG_WRITE && key != ((struct mmo_charstatus*)buf)->char_id )
	{
		printf("char_journal: key != char_id !\n");
		return 0;
	}

	// データの置き換え
	for( i=0; i<char_num; i++ )
	{
		if( char_dat[i].char_id == key )
		{
			if( flag == JOURNAL_FLAG_DELETE ) {
				memset( &char_dat[i], 0, sizeof(struct mmo_charstatus) );
			} else {
				memcpy( &char_dat[i], buf, sizeof(struct mmo_charstatus) );
			}
			return 1;
		}
	}

	// 追加
	if( flag != JOURNAL_FLAG_DELETE )
	{
		if(char_num>=char_max)
		{
			// メモリが足りないなら拡張
			char_max+=256;
			char_dat=(struct mmo_charstatus *)aRealloc(char_dat,sizeof(char_dat[0])*char_max);
			memset(char_dat + (char_max - 256), '\0', 256 * sizeof(*char_dat));
    	}

		memcpy( &char_dat[i], buf, sizeof(struct mmo_charstatus) );
		if(char_dat[i].char_id>=char_id_count)
			char_id_count=char_dat[i].char_id+1;
		char_num++;
		return 1;
	}

	return 0;
}
#endif

int char_txt_init(void)
{
	char line[65536];
	int ret, i;
	FILE *fp;

	fp=fopen(char_txt,"r");
	char_dat=(struct mmo_charstatus *)aCalloc(256,sizeof(char_dat[0]));
	char_max=256;
	if(fp==NULL)
		return 0;
	while(fgets(line,65535,fp)){
		int i,j = -1;
		if( sscanf(line,"%d\t%%newid%%%n",&i,&j)==1 && j > 0 && (line[j]=='\n' || line[j]=='\r') ){
			if(char_id_count<i)
				char_id_count=i;
			continue;
		}

		if(char_num>=char_max){
			char_max+=256;
			char_dat=(struct mmo_charstatus *)aRealloc(char_dat,sizeof(char_dat[0])*char_max);
			memset(char_dat + (char_max - 256), '\0', 256 * sizeof(*char_dat));
    	}
		memset(&char_dat[char_num],0,sizeof(char_dat[0]));
		ret=mmo_char_fromstr(line,&char_dat[char_num]);
		if(ret){
			if(char_dat[char_num].char_id>=char_id_count)
				char_id_count=char_dat[char_num].char_id+1;
			char_num++;
		}
	}
	fclose(fp);

#ifdef TXT_JOURNAL
	if( char_journal_enable )
	{
		// ジャーナルデータのロールフォワード
		if( journal_load( &char_journal, sizeof(struct mmo_charstatus), char_journal_file ) )
		{
			int c = journal_rollforward( &char_journal, char_journal_rollforward );

			printf("char: journal: roll-forward (%d)\n", c );

			// ロールフォワードしたので、txt データを保存する ( journal も新規作成される)
			char_txt_sync();
		}
		else
		{
			// ジャーナルを新規作成する
			journal_final( &char_journal );
			journal_create( &char_journal, sizeof(struct mmo_charstatus), char_journal_cache, char_journal_file );
		}
	}
#endif

	// 友達リストの名前を解決
	for( i=0; i<char_num; i++ )
	{
		int j;
		for( j=0; j<char_dat[i].friend_num; j++ )
		{
			struct friend_data* frd = char_dat[i].friend_data;
			const struct mmo_charstatus* p = char_txt_load( frd[j].char_id);
			if( p )
				memcpy( frd[j].name, p->name, 24 );
			else
			{
				char_dat[i].friend_num--;
				memmove( &frd[j], &frd[j+1], sizeof(frd[0])*( char_dat[i].friend_num - j ) );
				j--;
				continue;
			}
		}
	}
	return 0;
}

void char_txt_sync(void)
{
	char line[65536];
	int i,lock;
	FILE *fp;

	if( !char_dat )
		return;

	fp=lock_fopen(char_txt,&lock);
	if(fp==NULL)
		return;
	for(i=0;i<char_num;i++){
		if(char_dat[i].char_id > 0) {
			mmo_char_tostr(line,&char_dat[i]);
			fprintf(fp,"%s" RETCODE,line);
		}
	}
	fprintf(fp,"%d\t%%newid%%" RETCODE,char_id_count);
	lock_fclose(fp,char_txt,&lock);

#ifdef TXT_JOURNAL
	if( char_journal_enable )
	{
		// コミットしたのでジャーナルを新規作成する
		journal_final( &char_journal );
		journal_create( &char_journal, sizeof(struct mmo_charstatus), char_journal_cache, char_journal_file );
	}
#endif
}

void char_txt_final(void) {
//	char_txt_sync(); // do_final で呼んでるはず
	free(char_dat);

#ifdef TXT_JOURNAL
	if( char_journal_enable )
	{
		journal_final( &char_journal );
	}
#endif
}

const struct mmo_charstatus *char_txt_make(struct char_session_data *sd,unsigned char *dat,int *flag)
{
	int i;
	int make_char_max=9;	// 作成できるキャラ数

	for(i=0;i<24 && dat[i];i++){
		if(dat[i]<0x20 || dat[i]==0x7f)
			return NULL;
	}
	for(i = 24;i<=29;i++) {
		if(dat[i] > 9) return NULL;
	}
	if( make_char_max > 9)
		make_char_max = 9;
	if(dat[30] >= make_char_max){
		*flag = 0x02;
		printf("make new char over slot!! (%d / %d)\n",dat[30]+1,make_char_max+1);
		return NULL;
	}
	if(dat[24]+dat[25]+dat[26]+dat[27]+dat[28]+dat[29]>5*6 ||
		 dat[30]>=9 || dat[33]==0 || dat[33]>=24 || dat[31]>=9
	){
		char_log(
			"make new char error %d %s %d,%d,%d,%d,%d,%d %d,%d",
			dat[30],dat,dat[24],dat[25],dat[26],dat[27],dat[28],dat[29],dat[33],dat[31]
		);
		return NULL;
	}
	char_log("make new char %d %s",dat[30],dat);

	for(i=0;i<char_num;i++){
		if(strcmp(char_dat[i].name,dat)==0 || (char_dat[i].account_id==sd->account_id && char_dat[i].char_num==dat[30]))
			break;
	}
	if(i!=char_num)
		return NULL;
	if(char_num>=char_max) {
		// realloc() するとchar_datの位置が変わるので、session のデータを見て
		// 強制的に置き換える処理をしないとダメ。
		int i,j;
		struct mmo_charstatus *char_dat_old = char_dat;
		struct mmo_charstatus *char_dat_new = aMalloc(sizeof(struct mmo_charstatus) * (char_max + 256));
		memcpy(char_dat_new,char_dat_old,sizeof(struct mmo_charstatus) * char_max);
		memset(char_dat_new + char_max,0,256 * sizeof(struct mmo_charstatus));
		char_max += 256;
		for(i = 0; i <= fd_max; i++) {
			struct char_session_data *sd;
			if(session[i] && session[i]->func_parse == parse_char && (sd = session[i]->session_data)) {
				for(j = 0; j < 9; j++) {
					if(sd->found_char[j]) {
						sd->found_char[j] = char_dat_new + (sd->found_char[j] - char_dat_old);
					}
				}
			}
		}
		char_dat = char_dat_new;
		aFree(char_dat_old);
	}
	memset(&char_dat[i],0,sizeof(char_dat[0]));

	char_dat[i].char_id=char_id_count++;
	char_dat[i].account_id=sd->account_id;
	char_dat[i].char_num=dat[30];
	strncpy(char_dat[i].name,dat,24);
	char_dat[i].class=0;
	char_dat[i].base_level=1;
	char_dat[i].job_level=1;
	char_dat[i].base_exp=0;
	char_dat[i].job_exp=0;
	char_dat[i].zeny=start_zeny;
	char_dat[i].str=dat[24];
	char_dat[i].agi=dat[25];
	char_dat[i].vit=dat[26];
	char_dat[i].int_=dat[27];
	char_dat[i].dex=dat[28];
	char_dat[i].luk=dat[29];
	char_dat[i].max_hp=40 * (100 + char_dat[i].vit)/100;
	char_dat[i].max_sp=11 * (100 + char_dat[i].int_)/100;
	char_dat[i].hp=char_dat[i].max_hp;
	char_dat[i].sp=char_dat[i].max_sp;
	char_dat[i].status_point=0;
	char_dat[i].skill_point=0;
	char_dat[i].option=0;
	char_dat[i].karma=0;
	char_dat[i].manner=0;
	char_dat[i].party_id=0;
	char_dat[i].guild_id=0;
	char_dat[i].hair=dat[33];
	char_dat[i].hair_color=dat[31];
	char_dat[i].clothes_color=0;
	char_dat[i].inventory[0].nameid = 1201; /* Knife */
	char_dat[i].inventory[0].amount = 1;
	char_dat[i].inventory[0].equip = 0x02;
	char_dat[i].inventory[0].identify = 1;
	char_dat[i].inventory[1].nameid = 2301; /* Cotton Shirt */
	char_dat[i].inventory[1].amount = 1;
	char_dat[i].inventory[1].equip = 0x10;
	char_dat[i].inventory[1].identify = 1;
	char_dat[i].weapon = 1;
	char_dat[i].shield=0;
	char_dat[i].head_top=0;
	char_dat[i].head_mid=0;
	char_dat[i].head_bottom=0;
	memcpy(&char_dat[i].last_point,&start_point,sizeof(start_point));
	memcpy(&char_dat[i].save_point,&start_point,sizeof(start_point));
	char_num++;

#ifdef TXT_JOURNAL
	if( char_journal_enable )
		journal_write( &char_journal, char_dat[i].char_id, &char_dat[i] );
#endif

	return &char_dat[i];
}

int char_txt_load_all(struct char_session_data* sd,int account_id) {
	int i;
	int found_num = 0;
	for(i=0 ; i < char_num ; i++){
		if(char_dat[i].account_id == account_id){
			sd->found_char[found_num++] = &char_dat[i];
			if(found_num == 9)
				break;
		}
	}
	for(i = found_num; i < 9 ; i++) {
		sd->found_char[i] = NULL;
	}
	return found_num;
}

const struct mmo_charstatus* char_txt_load(int char_id) {
	int i;
	for(i=0;i<char_num;i++){
		if(char_dat[i].char_id == char_id) {
			return &char_dat[i];
		}
	}
	return NULL;
}

int char_txt_save(struct mmo_charstatus *st) {
	int i;
	for(i=0;i<char_num;i++){
		if(char_dat[i].account_id == st->account_id && char_dat[i].char_id == st->char_id) {
			memcpy(&char_dat[i],st,sizeof(char_dat[0]));
#ifdef TXT_JOURNAL
			if( char_journal_enable )
				journal_write( &char_journal, st->char_id, st );
#endif
			return 1;
		}
	}
	return 0;
}

int char_txt_delete_sub(int char_id) {
	int i;
	for(i=0;i<char_num;i++){
		if(char_dat[i].char_id == char_id) {
			memset(&char_dat[i],0,sizeof(char_dat[0]));
#ifdef TXT_JOURNAL
			if( char_journal_enable )
				journal_write( &char_journal, char_id, NULL );
#endif
			return 1;
		}
	}
	return 0;
}

int char_txt_config_read_sub(const char* w1,const char* w2) {
	if(strcmpi(w1,"char_txt")==0){
		strncpy(char_txt,w2,1024);
	}
	else if(strcmpi(w1,"gm_account_filename")==0){
		strncpy(GM_account_filename,w2,1023);
	}
#ifdef TXT_JOURNAL
	else if(strcmpi(w1,"char_journal_enable")==0){
		char_journal_enable = atoi( w2 );
	}
	else if(strcmpi(w1,"char_journal_file")==0){
		strncpy( char_journal_file, w2, sizeof(char_journal_file) );
	}
	else if(strcmpi(w1,"char_journal_cache_interval")==0){
		char_journal_cache = atoi( w2 );
	}
#endif
	return 0;
}
int char_txt_nick2id(const char *char_name)
{
	int i;
	for(i=0;i<char_num;i++){
		if(strcmp(char_dat[i].name,char_name)==0) {
			return char_dat[i].char_id;
		}
	}
	return -1;
}

#define char_make       char_txt_make
#define char_init       char_txt_init
#define char_sync       char_txt_sync
#define char_load       char_txt_load
#define char_save       char_txt_save
#define char_final      char_txt_final
#define char_load_all   char_txt_load_all
#define char_delete_sub char_txt_delete_sub
#define char_config_read_sub char_txt_config_read_sub

#else /* TXT_ONLY */

#include <mysql.h>
MYSQL mysql_handle;

#ifdef _MSC_VER
#pragma comment(lib,"libmysql.lib")
#endif

char tmp_sql[65535];

static struct dbt *char_db_;
static int  char_server_port        = 3306;
static char char_server_ip[32]      = "127.0.0.1";
static char char_server_id[32]      = "ragnarok";
static char char_server_pw[32]      = "ragnarok";
static char char_server_db[32]      = "ragnarok";
static char char_server_charset[32] = "";

char char_db[256]                   = "char";
char reg_db[256]                    = "global_reg_value";
char friend_db[256]                 = "friend";
static char cart_db[256]            = "cart_inventory";
static char inventory_db[256]       = "inventory";
static char charlog_db[256]         = "charlog";
// static char interlog_db[256]        = "interlog";
static char skill_db[256]           = "skill";
static char memo_db[256]            = "memo";

char* strecpy (char* pt,const char* spt) {
	//copy from here
	mysql_real_escape_string(&mysql_handle,pt,spt,strlen(spt));
	return pt;
}

int char_sql_loaditem(struct item *item, int max, int id, int tableswitch) {
	int i = 0;
	const char *tablename;
	const char *selectoption;
	MYSQL_RES* sql_res;
	MYSQL_ROW  sql_row = NULL;
	memset(item,0,sizeof(struct item) * max);
	switch (tableswitch) {
	case TABLE_INVENTORY:
		tablename    = inventory_db; // no need for sprintf here as *_db are char*.
		selectoption = "char_id";
		break;
	case TABLE_CART:
		tablename    = cart_db;
		selectoption = "char_id";
		break;
	case TABLE_STORAGE:
		tablename    = storage_db_;
		selectoption = "account_id";
		break;
	case TABLE_GUILD_STORAGE:
		tablename    = guild_storage_db_;
		selectoption = "guild_id";
		break;
	default:
		printf("Invalid table name!\n");
		return 1;
	}

	sprintf(
		tmp_sql,"SELECT `id`, `nameid`, `amount`, `equip`, `identify`, `refine`, `attribute`, "
		"`card0`, `card1`, `card2`, `card3` FROM `%s` WHERE `%s`='%d'", tablename, selectoption, id
	); // TBR

	if (mysql_query(&mysql_handle, tmp_sql)) {
		printf("DB server Error (select `%s`)- %s\n", tablename, mysql_error(&mysql_handle));
	}

	sql_res = mysql_store_result(&mysql_handle);
	if (sql_res) {
		for(i=0;(sql_row = mysql_fetch_row(sql_res)) && i < max;i++){
			item[i].id        = atoi(sql_row[0]);
			item[i].nameid    = atoi(sql_row[1]);
			item[i].amount    = atoi(sql_row[2]);
			item[i].equip     = atoi(sql_row[3]);
			item[i].identify  = atoi(sql_row[4]);
			item[i].refine    = atoi(sql_row[5]);
			item[i].attribute = atoi(sql_row[6]);
			item[i].card[0]   = atoi(sql_row[7]);
			item[i].card[1]   = atoi(sql_row[8]);
			item[i].card[2]   = atoi(sql_row[9]);
			item[i].card[3]   = atoi(sql_row[10]);
		}
		mysql_free_result(sql_res);
	}
	return i;
}

int char_sql_saveitem(struct item *item, int max, int id, int tableswitch) {
	int i;
	const char *tablename;
	const char *selectoption;
	char *p;
	char sep = ' ';

	switch (tableswitch) {
	case TABLE_INVENTORY:
		tablename    = inventory_db; // no need for sprintf here as *_db are char*.
		selectoption = "char_id";
		break;
	case TABLE_CART:
		tablename    = cart_db;
		selectoption = "char_id";
		break;
	case TABLE_STORAGE:
		tablename    = storage_db_;
		selectoption = "account_id";
		break;
	case TABLE_GUILD_STORAGE:
		tablename    = guild_storage_db_;
		selectoption = "guild_id";
		break;
	default:
		printf("Invalid table name!\n");
		return 1;
	}

	// delete
	sprintf(tmp_sql,"DELETE FROM `%s` WHERE `%s`='%d'",tablename,selectoption,id);
	if(mysql_query(&mysql_handle, tmp_sql)) {
		printf("DB server Error (DELETE `%s`)- %s\n", tablename, mysql_error(&mysql_handle));
	}

	p  = tmp_sql;
	p += sprintf(
		p,"INSERT INTO `%s`(`%s`, `nameid`, `amount`, `equip`, `identify`, `refine`, "
		"`attribute`, `card0`, `card1`, `card2`, `card3` ) VALUES",tablename,selectoption
	);

	for(i = 0 ; i < max ; i++) {
		if(item[i].nameid) {
			p += sprintf(
				p,"%c('%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d')",
				sep,id,item[i].nameid,item[i].amount,item[i].equip,item[i].identify,
				item[i].refine,item[i].attribute,item[i].card[0],item[i].card[1],
				item[i].card[2],item[i].card[3]
			);
			sep = ',';
		}
	}
	if(sep == ',') {
		if(mysql_query(&mysql_handle, tmp_sql)) {
			printf("DB server Error (INSERT `%s`)- %s\n", tablename, mysql_error(&mysql_handle));
		}
	}

	return 0;
}

int char_sql_init(void) {
	char_db_ = numdb_init();

	//DB connection initialized
	mysql_init(&mysql_handle);
	printf("Connect Character DB server");
	if(char_server_charset[0]) {
		printf(" (charset: %s)",char_server_charset);
	}
	printf("...\n");

	if(!mysql_real_connect(&mysql_handle, char_server_ip, char_server_id, char_server_pw,
		char_server_db ,char_server_port, (char *)NULL, 0)
	) {
		printf("%s\n",mysql_error(&mysql_handle));
		exit(1);
	} else {
		printf ("Connect Success! (Character Server)\n");
	}
	if(char_server_charset[0]) {
		sprintf(tmp_sql,"SET NAMES %s",char_server_charset);
		if (mysql_query(&mysql_handle, tmp_sql)) {
			printf("DB server Error (charset)- %s\n", mysql_error(&mysql_handle));
		}
	}

	return 1;
}

void char_sql_sync(void) {
	// nothing to do
}

const struct mmo_charstatus* char_sql_load(int char_id) {
	int i, n;
	struct mmo_charstatus *p;
	MYSQL_RES* sql_res;
	MYSQL_ROW  sql_row = NULL;
	p = (struct mmo_charstatus*)numdb_search(char_db_,char_id);
	if (p && p->char_id == char_id) {
		// 既にキャッシュが存在する
		return p;
	}
	if (p == NULL) {
		p = aMalloc(sizeof(struct mmo_charstatus));
		numdb_insert(char_db_,char_id,p);
	}
	memset(p,0,sizeof(struct mmo_charstatus));

	p->char_id = char_id;
	// printf("Request load char (%6d)[",char_id);

	sprintf(tmp_sql, "SELECT * FROM `%s` WHERE `char_id` = '%d'",char_db, char_id);

	if (mysql_query(&mysql_handle, tmp_sql)) {
		printf("DB server Error - %s\n", mysql_error(&mysql_handle));
	}

	sql_res = mysql_store_result(&mysql_handle);

	if (sql_res) {
		// 0       1          2        3    4     5          6         7        8       9    10
		// char_id account_id char_num name class base_level job_level base_exp job_exp zeny str
		// 11  12  13  14  15  16     17 18     19 20
		// agi vit int dex luk max_hp hp max_sp sp status_point
		// 21          22     23    24     25       26       27     28   29         30
		// skill_point option karma manner party_id guild_id pet_id hair hair_color clothes_color
		// 31     32     33       34       35          36       37     38     39       40
		// weapon shield head_top head_mid head_bottom last_map last_x last_y save_map save_x
		// 41     42         43           44           45      46     47
		// save_y partner_id parent_id[0] parent_id[1] baby_id online homun_id

		sql_row = mysql_fetch_row(sql_res);
		if(sql_row == NULL) {
			// char not found
			printf("char - failed\n");
			p->char_id = -1;
			return NULL;
		}
		p->char_id       = char_id;
		p->account_id    = atoi(sql_row[ 1]);
		p->char_num      = atoi(sql_row[ 2]);
		strcpy(p->name, sql_row[3]);
		p->class         = atoi(sql_row[ 4]);
		p->base_level    = atoi(sql_row[ 5]);
		p->job_level     = atoi(sql_row[ 6]);
		p->base_exp      = atoi(sql_row[ 7]);
		p->job_exp       = atoi(sql_row[ 8]);
		p->zeny          = atoi(sql_row[ 9]);
		p->str           = atoi(sql_row[10]);
		p->agi           = atoi(sql_row[11]);
		p->vit           = atoi(sql_row[12]);
		p->int_          = atoi(sql_row[13]);
		p->dex           = atoi(sql_row[14]);
		p->luk           = atoi(sql_row[15]);
		p->max_hp        = atoi(sql_row[16]);
		p->hp            = atoi(sql_row[17]);
		p->max_sp        = atoi(sql_row[18]);
		p->sp            = atoi(sql_row[19]);
		p->status_point  = atoi(sql_row[20]);
		p->skill_point   = atoi(sql_row[21]);
		p->option        = atoi(sql_row[22]);
		p->karma         = atoi(sql_row[23]);
		p->manner        = atoi(sql_row[24]);
		p->party_id      = atoi(sql_row[25]);
		p->guild_id      = atoi(sql_row[26]);
		p->pet_id        = atoi(sql_row[27]);
		p->homun_id      = (mysql_num_fields(sql_res) >= 48 ? atoi(sql_row[47]) : 0);
		p->hair          = atoi(sql_row[28]);
		p->hair_color    = atoi(sql_row[29]);
		p->clothes_color = atoi(sql_row[30]);
		p->weapon        = atoi(sql_row[31]);
		p->shield        = atoi(sql_row[32]);
		p->head_top      = atoi(sql_row[33]);
		p->head_mid      = atoi(sql_row[34]);
		p->head_bottom   = atoi(sql_row[35]);
		strcpy(p->last_point.map,sql_row[36]);
		p->last_point.x  = atoi(sql_row[37]);
		p->last_point.y  = atoi(sql_row[38]);
		strcpy(p->save_point.map,sql_row[39]);
		p->save_point.x  = atoi(sql_row[40]);
		p->save_point.y  = atoi(sql_row[41]);
		p->partner_id    = atoi(sql_row[42]);
		p->parent_id[0]	 = atoi(sql_row[43]);
		p->parent_id[1]	 = atoi(sql_row[44]);
		p->baby_id		 = atoi(sql_row[45]);
		//free mysql result.
		mysql_free_result(sql_res);
	} else {
		printf("char - failed\n");	//Error?! ERRRRRR WHAT THAT SAY!?
		p->char_id = -1;
		return NULL;
	}
	// printf("char ");

	//read memo data
	//`memo` (`memo_id`,`char_id`,`map`,`x`,`y`)
	sprintf(tmp_sql, "SELECT `map`,`x`,`y` FROM `%s` WHERE `char_id`='%d'",memo_db, char_id); // TBR
	if (mysql_query(&mysql_handle, tmp_sql)) {
		printf("DB server Error (select `memo`)- %s\n", mysql_error(&mysql_handle));
	}
	sql_res = mysql_store_result(&mysql_handle);

	if (sql_res) {
		for(i = 0; i < MAX_PORTAL_MEMO && (sql_row = mysql_fetch_row(sql_res)); i++) {
			strcpy (p->memo_point[i].map,sql_row[0]);
			p->memo_point[i].x=atoi(sql_row[1]);
			p->memo_point[i].y=atoi(sql_row[2]);
			//i ++;
		}
		mysql_free_result(sql_res);
	}
	// printf("memo ");

	//read inventory
	char_sql_loaditem(p->inventory,MAX_INVENTORY,p->char_id,TABLE_INVENTORY);
	// printf("inventory ");

	//read cart.
	char_sql_loaditem(p->cart,MAX_CART,p->char_id,TABLE_CART);
	// printf("cart ");

	//read skill
	//`skill` (`char_id`, `id`, `lv`)
	sprintf(tmp_sql, "SELECT `id`, `lv` FROM `%s` WHERE `char_id`='%d'",skill_db, char_id); // TBR
	if (mysql_query(&mysql_handle, tmp_sql)) {
		printf("DB server Error (select `skill`)- %s\n", mysql_error(&mysql_handle));
	}
	sql_res = mysql_store_result(&mysql_handle);
	if (sql_res) {
		for(i=0;(sql_row = mysql_fetch_row(sql_res));i++){
			n = atoi(sql_row[0]);
			p->skill[n].id = n; //memory!? shit!.
			if(atoi(sql_row[1]) > CLONE_SKILL_FLAG){
				p->skill[n].lv = atoi(sql_row[1]) - CLONE_SKILL_FLAG;
				p->skill[n].flag = CLONE_SKILL_FLAG;
			}else
				p->skill[n].lv = atoi(sql_row[1]);
		}
		mysql_free_result(sql_res);
	}
	// printf("skill ");

	// global_reg
	//`global_reg_value` (`char_id`, `str`, `value`)
	sprintf(tmp_sql, "SELECT `str`, `value` FROM `%s` WHERE `type`=3 AND `char_id`='%d'",reg_db, char_id); // TBR
	if (mysql_query(&mysql_handle, tmp_sql)) {
		printf("DB server Error (select `global_reg_value`)- %s\n", mysql_error(&mysql_handle));
	}
	i = 0;
	sql_res = mysql_store_result(&mysql_handle);
	if (sql_res) {
		for(i=0;(sql_row = mysql_fetch_row(sql_res));i++){
			strcpy (p->global_reg[i].str, sql_row[0]);
			p->global_reg[i].value = atoi (sql_row[1]);
		}
		mysql_free_result(sql_res);
	}
	p->global_reg_num=i;
	// printf("global_reg ");

	// friend list
	p->friend_num = 0;
	sprintf( tmp_sql, "SELECT `id1`, `id2`, `name` FROM `%s` WHERE `char_id`='%d'", friend_db, char_id );
	if (mysql_query(&mysql_handle, tmp_sql)) {
		printf("DB server Error (select `friend`)- %s\n", mysql_error(&mysql_handle));
	}
	sql_res = mysql_store_result( &mysql_handle );
	if( sql_res )
	{
		for( i=0; (sql_row = mysql_fetch_row(sql_res)); i++ )
		{
			p->friend_data[i].account_id = atoi( sql_row[0] );
			p->friend_data[i].char_id = atoi( sql_row[1] );
			strncpy( p->friend_data[i].name, sql_row[2], 24 );
		}
		mysql_free_result( sql_res );
		p->friend_num = i;
	}
	// friend list のチェックと訂正
	for(i=0; i<p->friend_num; i++ )
	{
		sprintf( tmp_sql, "SELECT `id1`, `name` FROM `%s` WHERE `char_id`='%d' AND `id2`='%d'", friend_db, p->friend_data[i].char_id, p->char_id );
		if (mysql_query(&mysql_handle, tmp_sql)) {
			printf("DB server Error (select `friend` / check )- %s\n", mysql_error(&mysql_handle));
		}
		sql_res = mysql_store_result( &mysql_handle );
		if( !sql_res )
		{
			// 相手に存在しないので、友達リストから削除する
			sprintf( tmp_sql, "DELETE FROM `%s` WHERE `char_id`='%d' AND `id2`='%d'", friend_db, p->char_id, p->friend_data[i].char_id );
			if (mysql_query(&mysql_handle, tmp_sql)) {
				printf("DB server Error (delete `friend` / correct)- %s\n", mysql_error(&mysql_handle));
			}
			p->friend_num--;
			memmove( &p->friend_data[i], &p->friend_data[i+1], sizeof(p->friend_data[0])* (p->friend_num - i ) );
			i--;
			printf("*friend_data_correct* ");
		}
		else
		{
			mysql_free_result( sql_res );
		}
	}
	// printf("friend ");

	// printf("]\n");	//ok. all data load successfuly!

	return p;
}

#define UPDATE_NUM(val,sql) \
	if(st1->val != st2->val) {\
		p += sprintf(p,"%c`"sql"` = '%d'",sep,st2->val); sep = ',';\
	}
#define UPDATE_STR(val,sql) \
	if(strcmp(st1->val,st2->val)) {\
		p += sprintf(p,"%c`"sql"` = '%s'",sep,strecpy(buf,st2->val)); sep = ',';\
	}

int  char_sql_save(struct mmo_charstatus *st2) {
	const struct mmo_charstatus *st1 = char_sql_load(st2->char_id);
	char sep = ' ';
	char buf[256];
	char *p = tmp_sql;
	int  i;
	unsigned short sk_lv;

	if(st1 == NULL) return 0;
	// printf("Request save char (%6d)[",st2->char_id);

	// basic information
	p += sprintf(p,"UPDATE `%s` SET",char_db);

	UPDATE_NUM(account_id    ,"account_id");
	UPDATE_NUM(char_num      ,"char_num");
	UPDATE_STR(name          ,"name");
	UPDATE_NUM(class         ,"class");
	UPDATE_NUM(base_level    ,"base_level");
	UPDATE_NUM(job_level     ,"job_level");
	UPDATE_NUM(base_exp      ,"base_exp");
	UPDATE_NUM(job_exp       ,"job_exp");
	UPDATE_NUM(zeny          ,"zeny");
	UPDATE_NUM(str           ,"str");
	UPDATE_NUM(agi           ,"agi");
	UPDATE_NUM(vit           ,"vit");
	UPDATE_NUM(int_          ,"int");
	UPDATE_NUM(dex           ,"dex");
	UPDATE_NUM(luk           ,"luk");
	UPDATE_NUM(max_hp        ,"max_hp");
	UPDATE_NUM(hp            ,"hp");
	UPDATE_NUM(max_sp        ,"max_sp");
	UPDATE_NUM(sp            ,"sp");
	UPDATE_NUM(status_point  ,"status_point");
	UPDATE_NUM(skill_point   ,"skill_point");
	UPDATE_NUM(option        ,"option");
	UPDATE_NUM(karma         ,"karma");
	UPDATE_NUM(manner        ,"manner");
	UPDATE_NUM(party_id      ,"party_id");
	UPDATE_NUM(guild_id      ,"guild_id");
	UPDATE_NUM(pet_id        ,"pet_id");
	UPDATE_NUM(hair          ,"hair");
	UPDATE_NUM(hair_color    ,"hair_color");
	UPDATE_NUM(clothes_color ,"clothes_color");
	UPDATE_NUM(weapon        ,"weapon");
	UPDATE_NUM(shield        ,"shield");
	UPDATE_NUM(head_top      ,"head_top");
	UPDATE_NUM(head_mid      ,"head_mid");
	UPDATE_NUM(head_bottom   ,"head_bottom");
	UPDATE_STR(last_point.map,"last_map");
	UPDATE_NUM(last_point.x  ,"last_x");
	UPDATE_NUM(last_point.y  ,"last_y");
	UPDATE_STR(save_point.map,"save_map");
	UPDATE_NUM(save_point.x  ,"save_x");
	UPDATE_NUM(save_point.y  ,"save_y");
	UPDATE_NUM(partner_id    ,"partner_id");
	UPDATE_NUM(parent_id[0]  ,"parent_id");
	UPDATE_NUM(parent_id[1]  ,"parent_id2");
	UPDATE_NUM(baby_id       ,"baby_id");
	UPDATE_NUM(homun_id      ,"homun_id");

	if(sep == ',') {
		sprintf(p," WHERE `char_id` = '%d'",st2->char_id);
		if (mysql_query(&mysql_handle, tmp_sql)) {
			printf("DB server Error - %s\n", mysql_error(&mysql_handle));
		}
		// printf("char ");
	}

	// memo
	if (memcmp(st1->memo_point,st2->memo_point,sizeof(st1->memo_point))) {
		//`memo` (`memo_id`,`char_id`,`map`,`x`,`y`)
		sprintf(tmp_sql,"DELETE FROM `%s` WHERE `char_id`='%d'",memo_db, st2->char_id);
		if(mysql_query(&mysql_handle, tmp_sql)) {
			printf("DB server Error (delete `memo`)- %s\n", mysql_error(&mysql_handle));
		}

		//insert here.
		for(i = 0; i < MAX_PORTAL_MEMO; i++) {
			if(st2->memo_point[i].map[0]) {
				sprintf(
					tmp_sql,"INSERT INTO `%s`(`char_id`,`map`,`x`,`y`) VALUES ('%d', '%s', '%d', '%d')",
					memo_db, st2->char_id, st2->memo_point[i].map, st2->memo_point[i].x, st2->memo_point[i].y
				);
				if(mysql_query(&mysql_handle, tmp_sql))
					printf("DB server Error (insert `memo`)- %s\n", mysql_error(&mysql_handle));
			}
		}
		// printf("memo ");
	}

	// inventory
	if (memcmp(st1->inventory, st2->inventory, sizeof(st1->inventory))) {
		char_sql_saveitem(st2->inventory,MAX_INVENTORY,st2->char_id,TABLE_INVENTORY);
		// printf("inventory ");
	}

	// cart
	if (memcmp(st1->cart, st2->cart, sizeof(st1->cart))) {
		char_sql_saveitem(st2->cart,MAX_CART,st2->char_id,TABLE_CART);
		// printf("cart ");
	}

	// skill
	if(memcmp(st1->skill,st2->skill,sizeof(st1->skill))) {
		//printf("- Save skill data to MySQL!\n");
		//`skill` (`char_id`, `id`, `lv`)
		sprintf(tmp_sql,"DELETE FROM `%s` WHERE `char_id`='%d'",skill_db, st2->char_id);
		if(mysql_query(&mysql_handle, tmp_sql)) {
			printf("DB server Error (delete `skill`)- %s\n", mysql_error(&mysql_handle));
		}
		//printf("- Insert skill \n");
		//insert here.
		p  = tmp_sql;
		p += sprintf(p,"INSERT INTO `%s`(`char_id`, `id`, `lv`) VALUES",skill_db);
		sep = ' ';
		for(i=0;i<MAX_SKILL;i++){
			if(st2->skill[i].flag == CLONE_SKILL_FLAG)
				sk_lv = st2->skill[i].lv + CLONE_SKILL_FLAG;
			else
				sk_lv = (st2->skill[i].flag==0)?st2->skill[i].lv:st2->skill[i].flag-2;
			if(st2->skill[i].id && st2->skill[i].flag!=1 && st2->skill[i].flag!=CLONE_SKILL_FLAG){
				p += sprintf(p,"%c('%d','%d','%d')",sep,st2->char_id, st2->skill[i].id, sk_lv);
				sep = ',';
			}
		}
		if(sep == ',') {
			if(mysql_query(&mysql_handle, tmp_sql)) {
				printf("DB server Error (insert `skill`)- %s\n", mysql_error(&mysql_handle));
			}
		}
		// printf("skill ");
	}

	// global_reg
	if(
		memcmp(st1->global_reg,st2->global_reg,sizeof(st1->global_reg)) ||
		st1->global_reg_num != st2->global_reg_num
	) {
		//printf("- Save global_reg_value data to MySQL!\n");
		//`global_reg_value` (`char_id`, `str`, `value`)
		sprintf(tmp_sql,"DELETE FROM `%s` WHERE `type`=3 AND `char_id`='%d'",reg_db, st2->char_id);
		if (mysql_query(&mysql_handle, tmp_sql)) {
			printf("DB server Error (delete `global_reg_value`)- %s\n", mysql_error(&mysql_handle));
		}

		//insert here.
		for(i=0;i<st2->global_reg_num;i++){
			if (st2->global_reg[i].str && st2->global_reg[i].value !=0) {
				sprintf(
					tmp_sql,"INSERT INTO `%s` (`char_id`, `str`, `value`) VALUES ('%d', '%s','%d')",
					reg_db, st2->char_id, strecpy(buf,st2->global_reg[i].str), st2->global_reg[i].value
				);
				if(mysql_query(&mysql_handle, tmp_sql)) {
					printf("DB server Error (insert `global_reg_value`)- %s\n", mysql_error(&mysql_handle));
				}
			}
		}
		// printf("global_reg ");
	}

	// friend
	if( st1->friend_num != st2->friend_num ||
		memcmp(st1->friend_data, st2->friend_data, sizeof(st1->friend_data)) != 0 )
	{
		sprintf( tmp_sql, "DELETE FROM `%s` WHERE `char_id`='%d'", friend_db, st2->char_id );
		if (mysql_query(&mysql_handle, tmp_sql)) {
			printf("DB server Error (delete `friend`)- %s\n", mysql_error(&mysql_handle));
		}

		for( i=0; i<st2->friend_num; i++ )
		{
			sprintf( tmp_sql, "INSERT INTO `%s` (`char_id`, `id1`, `id2`, `name`) VALUES ('%d', '%d', '%d', '%s')",
				friend_db, st2->char_id, st2->friend_data[i].account_id, st2->friend_data[i].char_id,
				strecpy( buf, st2->friend_data[i].name ) );
			if(mysql_query(&mysql_handle, tmp_sql)) {
				printf("DB server Error (insert `friend`)- %s\n", mysql_error(&mysql_handle));
			}
		}
		// printf("friend ");
	}

	// printf("]\n");
	{
		struct mmo_charstatus *st3 = numdb_search(char_db_,st2->char_id);
		if(st3)
			memcpy(st3,st2,sizeof(struct mmo_charstatus));
	}
	return 0;
}

const struct mmo_charstatus* char_sql_make(struct char_session_data *sd,unsigned char *dat,int *flag) {
	int  i;
	int  char_id;
	int make_char_max=9;	// 作成できるキャラ数
	char buf[256];
	MYSQL_RES* sql_res;
	MYSQL_ROW  sql_row = NULL;
	for(i=0;i<24 && dat[i];i++){
		if(dat[i]<0x20 || dat[i]==0x7f)
//		MySQLのバグをAthena側で抑制
//		if(dat[i]<0x20 || dat[i]==0x7f || dat[i]>=0xfd)
			return NULL;
	}
	for(i = 24;i<=29;i++) {
		if(dat[i] > 9) return NULL;
	}
	if( make_char_max > 9)
		make_char_max = 9;
	if(dat[30] >= make_char_max){
		*flag = 0x02;
		printf("make new char over slot!! (%d / %d)\n",dat[30]+1,make_char_max+1);
		return NULL;
	}
	if(dat[24]+dat[25]+dat[26]+dat[27]+dat[28]+dat[29]>5*6 ||
		 dat[30]>=9 || dat[33]==0 || dat[33]>=24 || dat[31]>=9
	){
		char_log(
			"make new char error %d %s %d,%d,%d,%d,%d,%d %d,%d",
			dat[30],dat,dat[24],dat[25],dat[26],dat[27],dat[28],dat[29],dat[33],dat[31]
		);
		return NULL;
	}
	char_log("make new char %d %s",dat[30],dat);
	// 同名チェック
	sprintf(
		tmp_sql,
		"SELECT COUNT(*) FROM `%s` WHERE (`account_id` = '%d' AND `char_num` = '%d') OR "
		"`name` = '%s'",char_db, sd->account_id,dat[30],strecpy(buf,dat)
	);
	if (mysql_query(&mysql_handle, tmp_sql)) {
		printf("DB server Error - %s\n", mysql_error(&mysql_handle));
		return 0;
	}
	sql_res = mysql_store_result(&mysql_handle);
	if (!sql_res) return NULL;
	sql_row = mysql_fetch_row(sql_res);
	i = atoi(sql_row[0]);
	mysql_free_result(sql_res);
	if(i) return NULL;

	//Insert the char to the 'chardb' ^^
	sprintf(
		tmp_sql,
		"INSERT INTO `%s` (`account_id`,`char_num`,`name`,`zeny`,`str`,`agi`,`vit`,`int`,"
		"`dex`,`luk`,`max_hp`,`hp`,`max_sp`,`sp`,`hair`,`hair_color`,`last_map`,`last_x`,"
		"`last_y`,`save_map`,`save_x`,`save_y`) VALUES ('%d','%d','%s','%d','%d','%d','%d',"
		"'%d','%d','%d','%d','%d','%d','%d','%d','%d','%s','%d','%d','%s','%d','%d')",
		char_db,sd->account_id,dat[30],strecpy(buf,dat),start_zeny,dat[24],dat[25],dat[26],
		dat[27],dat[28],dat[29],40 * (100 + dat[26])/100,40 * (100 + dat[26])/100,
		11 * (100 + dat[27])/100,11 * (100 + dat[27])/100,dat[33],dat[31],start_point.map,
		start_point.x, start_point.y, start_point.map, start_point.x,start_point.y
	);
	if(mysql_query(&mysql_handle, tmp_sql)){
		printf("failed (insert in chardb), SQL error: %s\n", mysql_error(&mysql_handle));
		return NULL; //No, stop the procedure!
	}

	//Now we need the charid from sql!
	char_id = (int)mysql_insert_id(&mysql_handle);

	// Give the char the default items
	// knife
	sprintf(
		tmp_sql,"INSERT INTO `%s` (`char_id`,`nameid`, `amount`, `equip`, `identify`) "
		"VALUES ('%d', '%d', '%d', '%d', '%d')",inventory_db, char_id, 1201,1,0x02,1
	);
	if (mysql_query(&mysql_handle, tmp_sql)){
		printf("fail (insert in inventory  the 'knife'), SQL error: %s\n", mysql_error(&mysql_handle));
		return NULL;
	}
	//cotton shirt
	sprintf(tmp_sql,"INSERT INTO `%s` (`char_id`,`nameid`, `amount`, `equip`, `identify`) "
		"VALUES ('%d', '%d', '%d', '%d', '%d')", inventory_db, char_id, 2301,1,0x10,1
	);
	if (mysql_query(&mysql_handle, tmp_sql)){
		printf("fail (insert in inventroxy the 'cotton shirt'), SQL error: %s\n", mysql_error(&mysql_handle));
		return NULL; //end....
	}

	//printf("making new char success - id:(\033[1;32m%d\033[0m\tname:\033[1;32%s\033[0m\n", char_id, t_name);
	printf("success, aid: %d, cid: %d, slot: %d, name: %s\n", sd->account_id, char_id, dat[30], strecpy(buf,dat));

	return char_sql_load(char_id);
}

int  char_sql_load_all(struct char_session_data* sd,int account_id) {
	int i,j;
	int found_id[9];
	int found_num = 0;
	MYSQL_RES* sql_res;
	MYSQL_ROW  sql_row = NULL;

	memset(&found_id,0,sizeof(found_id));
	//search char.
	sprintf(tmp_sql, "SELECT `char_id` FROM `%s` WHERE `account_id` = '%d'",char_db, account_id);
	if (mysql_query(&mysql_handle, tmp_sql)) {
		printf("DB server Error - %s\n", mysql_error(&mysql_handle));
		return 0;
	}
	sql_res = mysql_store_result(&mysql_handle);
	if (sql_res) {
		while((sql_row = mysql_fetch_row(sql_res))) {
			found_id[found_num++] = atoi(sql_row[0]);
		}
		mysql_free_result(sql_res);
	}
	j = 0;
	for(i = 0;i < found_num ; i++) {
		sd->found_char[j] = char_sql_load(found_id[i]);
		if(sd->found_char[j]) j++;
	}
	for(i = j; i < 9; i++) {
		sd->found_char[i] = NULL;
	}
	return j;
}

int  char_sql_delete_sub(int char_id) {
	struct mmo_charstatus *p = numdb_search(char_db_,char_id);
	if(p) {
		numdb_erase(char_db_,char_id);
		aFree(p);
	}

	// char
	sprintf(tmp_sql,"DELETE FROM `%s` WHERE `char_id`='%d'",char_db, char_id);
	if(mysql_query(&mysql_handle, tmp_sql)) {
		printf("DB server Error - %s\n", mysql_error(&mysql_handle));
	}

	// memo
	sprintf(tmp_sql,"DELETE FROM `%s` WHERE `char_id`='%d'",memo_db, char_id);
	if(mysql_query(&mysql_handle, tmp_sql)) {
		printf("DB server Error - %s\n", mysql_error(&mysql_handle));
	}

	// inventory
	sprintf(tmp_sql,"DELETE FROM `%s` WHERE `char_id`='%d'",inventory_db, char_id);
	if(mysql_query(&mysql_handle, tmp_sql)) {
		printf("DB server Error - %s\n", mysql_error(&mysql_handle));
	}

	// cart
	sprintf(tmp_sql,"DELETE FROM `%s` WHERE `char_id`='%d'",cart_db, char_id);
	if(mysql_query(&mysql_handle, tmp_sql)) {
		printf("DB server Error - %s\n", mysql_error(&mysql_handle));
	}

	// skill
	sprintf(tmp_sql,"DELETE FROM `%s` WHERE `char_id`='%d'",skill_db, char_id);
	if(mysql_query(&mysql_handle, tmp_sql)) {
		printf("DB server Error - %s\n", mysql_error(&mysql_handle));
	}

	// global_reg
	sprintf(tmp_sql,"DELETE FROM `%s` WHERE `type`=3 AND `char_id`='%d'",reg_db, char_id);
	if (mysql_query(&mysql_handle, tmp_sql)) {
		printf("DB server Error - %s\n", mysql_error(&mysql_handle));
	}

	// friend
	sprintf( tmp_sql, "DELETE FROM `%s` WHERE `char_id`='%d'", friend_db, char_id );
	if (mysql_query(&mysql_handle, tmp_sql)) {
		printf("DB server Error - %s\n", mysql_error(&mysql_handle));
	}

	return 1;
}

static int char_db_final(void *key,void *data,va_list ap)
{
	struct mmo_charstatus *p=data;

	free(p);

	return 0;
}

void char_sql_final(void) {
	mysql_close(&mysql_handle);
	printf("close DB connect....\n");

	numdb_final(char_db_,char_db_final);
	return;
}

int  char_sql_config_read_sub(const char* w1,const char* w2) {
	if(strcmpi(w1,"char_server_ip")==0){
		strcpy(char_server_ip, w2);
	}
	else if(strcmpi(w1,"char_server_port")==0){
		char_server_port=atoi(w2);
	}
	else if(strcmpi(w1,"char_server_id")==0){
		strcpy(char_server_id, w2);
	}
	else if(strcmpi(w1,"char_server_pw")==0){
		strcpy(char_server_pw, w2);
	}
	else if(strcmpi(w1,"char_server_db")==0){
		strcpy(char_server_db, w2);
	}
	else if(strcmpi(w1,"char_server_charset")==0){
		strcpy(char_server_charset, w2);
	}
	return 0;
}
int char_sql_nick2id(const char *char_name)
{
	int char_id;
	char buf[64];
	MYSQL_RES* sql_res;
	MYSQL_ROW  sql_row = NULL;
	sprintf(tmp_sql,"SELECT `char_id` FROM `%s` WHERE `name` = '%s'",char_db,strecpy(buf,char_name));
	if(mysql_query(&mysql_handle, tmp_sql)){
		return -1;
	}
	sql_res = mysql_store_result(&mysql_handle);
	if(!sql_res){
		return -1;
	}
	sql_row = mysql_fetch_row(sql_res);
	char_id = atoi(sql_row[0]);
	mysql_free_result(sql_res);
	return char_id;
}

#define char_make       char_sql_make
#define char_init       char_sql_init
#define char_sync       char_sql_sync
#define char_load       char_sql_load
#define char_save       char_sql_save
#define char_final      char_sql_final
#define char_load_all   char_sql_load_all
#define char_delete_sub char_sql_delete_sub
#define char_config_read_sub char_sql_config_read_sub

#endif /* TXT_ONLY */

static struct dbt *gm_account_db;
int isGM(int account_id)
{
	struct gm_account *p;
	p = numdb_search(gm_account_db,account_id);
	if( p == NULL)
		return 0;
	return p->level;
}

void read_gm_account(void) {
	char line[8192];
	struct gm_account *p;
	FILE *fp;
	int c, l;
	int account_id, level;
	int i;
	int range, start_range, end_range;

	gm_account_db = numdb_init();

	if ((fp = fopen(GM_account_filename, "r")) == NULL) {
		printf("File not found: %s.\n", GM_account_filename);
		return;
	}

	line[sizeof(line)-1] = '\0';
	c = 0;
	l = 0;
	while(fgets(line, sizeof(line)-1, fp)) {
		l++;
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;

		if ((range = sscanf(line, "%d%*[-~]%d %d", &start_range, &end_range, &level)) == 3 ||
		    (range = sscanf(line, "%d%*[-~]%d:%d", &start_range, &end_range, &level)) == 3 ||
		    (range = sscanf(line, "%d %d", &start_range, &level)) == 2 ||
		    (range = sscanf(line, "%d:%d", &start_range, &level)) == 2 ||
		    (range = sscanf(line, "%d: %d", &start_range, &level)) == 2) {
			if (level <= 0) {
				printf("gm_account [%s]: invalid GM level [%ds] line %d\n", GM_account_filename, level, l);
			} else {
				if (level > 99)
					level = 99;
				if (range == 2)
					end_range = start_range;
				else if (end_range < start_range) {
					i = end_range;
					end_range = start_range;
					start_range = i;
				}
				for (account_id = start_range; account_id <= end_range; account_id++) {
					p = (struct gm_account *)aCalloc(1, sizeof(struct gm_account));
					p->account_id = account_id;
					p->level = level;
					numdb_insert(gm_account_db, account_id, p);
					c++;
				}
			}
		} else {
			printf("gm_account: broken data [%s] line %d\n", GM_account_filename, l);
		}
	}
	fclose(fp);
//	printf("gm_account: %s read done (%d gm account ID)\n", GM_account_filename, c);

	return;
}

int mmo_char_sync_timer(int tid,unsigned int tick,int id,int data)
{
	char_sync();
	inter_sync();
	return 0;
}
#ifdef TXT_ONLY
int char_log(char *fmt,...)
{
	FILE *logfp;
	va_list ap;
	va_start(ap,fmt);

	logfp=fopen(char_log_filename,"a");
	if(logfp){
		vfprintf(logfp,fmt,ap);
		fprintf(logfp,RETCODE);
		fclose(logfp);
	}

	va_end(ap);
	return 0;
}
#else
int char_log(char *fmt,...)
{
	char *log;
	char buf[512];
	va_list ap;

	if ((log = malloc(256)) == NULL)
                 return 0;

	va_start(ap,fmt);

	(void) vsnprintf(log,256,fmt,ap);
	va_end(ap);

	sprintf(
		tmp_sql,"INSERT INTO `%s` (`time`,`log`) VALUES (NOW(),'%s')",
		charlog_db,strecpy(buf,log)
	);
	if(mysql_query(&mysql_handle, tmp_sql) ){
		printf("DB server Error - %s\n", mysql_error(&mysql_handle) );
	}

	free(log);
	return 0;
}
#endif
int count_users(void)
{
	if(login_fd>0 && session[login_fd]){
		int i,users;
		for(i=0,users=0;i<MAX_MAP_SERVERS;i++)
			if(server_fd[i]>0)
				users+=server[i].users;
		return users;
	}
  return 0;
}

int mmo_char_send006b(int fd,struct char_session_data *sd)
{
	int i,found_num;
	const struct mmo_charstatus *st;
#ifdef NEW_006b
	int offset=24;
#else
	int offset=4;
#endif
	session[fd]->auth = 1; // 認証終了を socket.c に伝える

	sd->state = CHAR_STATE_AUTHOK;
	found_num = char_load_all(sd,sd->account_id);

	memset(WFIFOP(fd,0),0,offset+found_num*106);
	WFIFOW(fd,0)=0x6b;
	WFIFOW(fd,2)=offset+found_num*106;

	for( i = 0; i < 9 ; i++ ) {
		st = sd->found_char[i];
		if(st == NULL) continue;
		WFIFOL(fd,offset+(i*106)    ) = st->char_id;
		WFIFOL(fd,offset+(i*106)+  4) = st->base_exp;
		WFIFOL(fd,offset+(i*106)+  8) = st->zeny;
		WFIFOL(fd,offset+(i*106)+ 12) = st->job_exp;
		WFIFOL(fd,offset+(i*106)+ 16) = st->job_level;
		WFIFOL(fd,offset+(i*106)+ 20) = 0;
		WFIFOL(fd,offset+(i*106)+ 24) = 0;
		WFIFOL(fd,offset+(i*106)+ 28) = st->option;
		WFIFOL(fd,offset+(i*106)+ 32) = st->karma;
		WFIFOL(fd,offset+(i*106)+ 36) = st->manner;
		WFIFOW(fd,offset+(i*106)+ 40) = st->status_point;
		WFIFOW(fd,offset+(i*106)+ 42) = (st->hp     > 0x7fff) ? 0x7fff : st->hp;
		WFIFOW(fd,offset+(i*106)+ 44) = (st->max_hp > 0x7fff) ? 0x7fff : st->max_hp;
		WFIFOW(fd,offset+(i*106)+ 46) = (st->sp     > 0x7fff) ? 0x7fff : st->sp;
		WFIFOW(fd,offset+(i*106)+ 48) = (st->max_sp > 0x7fff) ? 0x7fff : st->max_sp;
		WFIFOW(fd,offset+(i*106)+ 50) = DEFAULT_WALK_SPEED; // char_dat[j].speed;
		if(st->class==28 || st->class==29)
			WFIFOW(fd,offset+(i*106)+ 52) = st->class-4;
		else
			WFIFOW(fd,offset+(i*106)+ 52) = st->class;
		WFIFOW(fd,offset+(i*106)+ 54) = st->hair;
		WFIFOW(fd,offset+(i*106)+ 56) = st->weapon;
		WFIFOW(fd,offset+(i*106)+ 58) = st->base_level;
		WFIFOW(fd,offset+(i*106)+ 60) = st->skill_point;
		WFIFOW(fd,offset+(i*106)+ 62) = st->head_bottom;
		WFIFOW(fd,offset+(i*106)+ 64) = st->shield;
		WFIFOW(fd,offset+(i*106)+ 66) = st->head_top;
		WFIFOW(fd,offset+(i*106)+ 68) = st->head_mid;
		WFIFOW(fd,offset+(i*106)+ 70) = st->hair_color;
		WFIFOW(fd,offset+(i*106)+ 72) = st->clothes_color;
		memcpy( WFIFOP(fd,offset+(i*106)+74), st->name, 24 );
		WFIFOB(fd,offset+(i*106)+ 98) = (st->str > 255)?  255:st->str;
		WFIFOB(fd,offset+(i*106)+ 99) = (st->agi > 255)?  255:st->agi;
		WFIFOB(fd,offset+(i*106)+100) = (st->vit > 255)?  255:st->vit;
		WFIFOB(fd,offset+(i*106)+101) = (st->int_ > 255)? 255:st->int_;
		WFIFOB(fd,offset+(i*106)+102) = (st->dex > 255)?  255:st->dex;
		WFIFOB(fd,offset+(i*106)+103) = (st->luk > 255)?  255:st->luk;
		WFIFOB(fd,offset+(i*106)+104) = st->char_num;

		// ロードナイト/パラディンのログイン時のエラー対策
		if (st->option == 32)
			WFIFOL(fd,offset+(i*106)+28) = 0;
		else
			WFIFOL(fd,offset+(i*106)+28) = st->option;
	}

	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}

int set_account_reg2(int acc,int num,struct global_reg *reg)
{
	int i;
	struct char_session_data sd;
	struct mmo_charstatus    st;
	int max = char_load_all(&sd,acc);
	for(i=0;i<max;i++) {
		memcpy(&st,sd.found_char[i],sizeof(struct mmo_charstatus));
		memcpy(st.account_reg2,reg,sizeof(st.account_reg2));
		st.account_reg2_num = num;
		char_save(&st);
	}
	return max;
}

// 離婚
int char_divorce(const struct mmo_charstatus *st){
	if(st == NULL) return 0;

	if(st->partner_id){
		int i;
		struct mmo_charstatus s1;
		memcpy(&s1,st,sizeof(struct mmo_charstatus));

		// 離婚
		s1.partner_id = 0;

		// 結婚指輪を剥奪
		for(i=0;i<MAX_INVENTORY;i++){
			if(s1.inventory[i].nameid == WEDDING_RING_M || s1.inventory[i].nameid == WEDDING_RING_F){
				memset(&s1.inventory[i],0,sizeof(s1.inventory[0]));
			}
		}
		char_save(&s1);
		return 1;
	}
	return 0;
}

// キャラ削除に伴うデータ削除
static int char_delete(const struct mmo_charstatus *cs)
{
	int j;
	char buf[64];

	nullpo_retr(-1,cs);

	printf("char_delete: %s\n",cs->name);
	// キャラが接続しているかもしれないのでmapに切断要求
	WBUFW(buf,0)=0x2b19;
	WBUFL(buf,2)=cs->account_id;
	mapif_sendall(buf,6);

	// ペット削除
	if(cs->pet_id)
		pet_delete(cs->pet_id);
	for(j=0;j<MAX_INVENTORY;j++)
		if(cs->inventory[j].card[0] == (short)0xff00)
			pet_delete(*((long *)(&cs->inventory[j].card[2])));
	for(j=0;j<MAX_CART;j++)
		if(cs->cart[j].card[0] == (short)0xff00)
			pet_delete(*((long *)(&cs->cart[j].card[2])));
	// ホムンクルス削除
	if(cs->homun_id)
		homun_delete(cs->homun_id);
	// ギルド脱退
	if(cs->guild_id)
		inter_guild_leave(cs->guild_id,cs->account_id,cs->char_id);
	// パーティー脱退
	if(cs->party_id)
		inter_party_leave(cs->party_id, cs->account_id, cs->name);
	// 離婚
	if(cs->partner_id){
		const struct mmo_charstatus *p_cs = char_load(cs->partner_id);
		if(p_cs && cs->partner_id == p_cs->char_id && p_cs->partner_id == cs->char_id) {
			// 相方が自分と結婚している場合
			// 相方の離婚情報をmapに通知
			WBUFW(buf,0)=0x2b12;
			WBUFL(buf,2)=p_cs->char_id;
			mapif_sendall(buf,6);

			// 相方の離婚処理
			char_divorce(p_cs);
		}

		// 自分の離婚処理
		char_divorce(cs);
	}
	char_delete_sub(cs->char_id);

	return 0;
}

// authfifoの比較
int cmp_authfifo(int i,int account_id,int login_id1,int login_id2,int ip)
{
	if(	auth_fifo[i].account_id==account_id &&
		auth_fifo[i].login_id1==login_id1 )
		return 1;
#ifdef CMP_AUTHFIFO_LOGIN2
//	printf("cmp_authfifo: id2 check %d %x %x = %08x %08x %08x\n",i,auth_fifo[i].login_id2,login_id2,
//		auth_fifo[i].account_id,auth_fifo[i].login_id1,auth_fifo[i].login_id2);
	if( auth_fifo[i].login_id2==login_id2 && login_id2 != 0)
		return 1;
#endif
#ifdef CMP_AUTHFIFO_IP
//	printf("cmp_authfifo: ip check %d %x %x = %08x %08x %08x\n",i,auth_fifo[i].ip,ip,
//		auth_fifo[i].account_id,auth_fifo[i].login_id1,auth_fifo[i].login_id2);
	if(auth_fifo[i].ip==ip && ip!=0 && ip!=-1)
		return 1;
#endif
	return 0;
}

// ソケットのデストラクタ
int parse_login_disconnect(int fd) {
	if(fd==login_fd)
		login_fd=0;
	return 0;
}

#define PC_CLASS_BASE 0
#define PC_CLASS_BASE2 (PC_CLASS_BASE + 4001)
#define PC_CLASS_BASE3 (PC_CLASS_BASE2 + 22)

int parse_tologin(int fd)
{
	int i,fdc;
	struct char_session_data *sd;

//	printf("parse_tologin : %d %d %d\n",fd,RFIFOREST(fd),RFIFOW(fd,0));
	sd=session[fd]->session_data;
	while(RFIFOREST(fd)>=2){
		switch(RFIFOW(fd,0)){
		case 0x2711:
			if(RFIFOREST(fd)<3)
				return 0;
			if(RFIFOB(fd,2)){
				printf("connect login server error : %d\n",RFIFOB(fd,2));
				exit(1);
			}
			RFIFOSKIP(fd,3);
			session[fd]->auth = -1; // 認証終了を socket.c に伝える
			break;
		case 0x2713:
			if(RFIFOREST(fd)<15)
				return 0;
			for(i=0;i<fd_max;i++){
				if(session[i] && (sd=session[i]->session_data)){
					if(sd->account_id==RFIFOL(fd,2))
						break;
				}
			}
			fdc=i;
			if(fdc==fd_max){
				RFIFOSKIP(fd,15);
				break;
			}
//			printf("parse_tologin 2713 : %d\n",RFIFOB(fd,6));
			if(RFIFOB(fd,6)!=0){
				WFIFOW(fdc,0)=0x6c;
				WFIFOB(fdc,2)=0x42;
				WFIFOSET(fdc,3);
				RFIFOSKIP(fd,15);
				break;
			}
			sd->account_id=RFIFOL(fd,7);
			sd->login_id1=RFIFOL(fd,11);

			if(char_maintenance && isGM(sd->account_id)==0){
				close(fd);
				session[fd]->eof=1;
				return 0;
			}
			if(max_connect_user > 0) {
				if(count_users() < max_connect_user  || isGM(sd->account_id)>0)
					mmo_char_send006b(fdc,sd);
				else {
					WFIFOW(fdc,0)=0x6c;
					WFIFOW(fdc,2)=0;
					WFIFOSET(fdc,3);
				}
			}
			else
				mmo_char_send006b(fdc,sd);

			RFIFOSKIP(fd,15);
			break;
#ifdef AC_MAIL
		case 0x2716: 	// キャラ削除(メールアドレス確認後)
			if(RFIFOREST(fd)<11)
				return 0;
			{
				int ch;
				for(i=0;i<fd_max;i++){
					if(session[i] && (sd=session[i]->session_data)){
						if(sd->account_id==RFIFOL(fd,2))
							break;
					}
				}
				fdc=i;
				if(fdc==fd_max){
					RFIFOSKIP(fd,15);
					break;
				}
				if(RFIFOB(fd,10)!=0){
					WFIFOW(fdc,0)=0x70;
					WFIFOB(fdc,2)=1;
					WFIFOSET(fdc,3);
				}else{
					for(i=0;i<9;i++){
						const struct mmo_charstatus *cs = sd->found_char[i];
						if(cs && cs->char_id == RFIFOL(fd,6)){
							char_delete(cs);
							for(ch=i;ch<9-1;ch++)
								sd->found_char[ch]=sd->found_char[ch+1];
							sd->found_char[8] = NULL;
							break;
						}
					}
					if( i==9 ){
						WFIFOW(fdc,0)=0x70;
						WFIFOB(fdc,2)=0;
						WFIFOSET(fdc,3);
					} else {
						WFIFOW(fdc,0)=0x6f;
						WFIFOSET(fdc,2);
					}
				}
				RFIFOSKIP(fd,11);
			}
			break;
#endif//AC_MAIL
		case 0x2721: 	// gm reply
			{
				// SQL 化が面倒くさいので保留
				unsigned char buf[64];
				if(RFIFOREST(fd)<10)
					return 0;
				RFIFOSKIP(fd,10);
				WBUFW(buf,0)=0x2b0b;
				WBUFL(buf,2)=RFIFOL(fd,2);
				WBUFL(buf,6)=RFIFOL(fd,6);
				mapif_sendall(buf,10);
	//			printf("char -> map\n");
			}
			break;
		case 0x2723:	// changesex reply
			if(RFIFOREST(fd)<7)
				return 0;
			{
				int i,j,sex = RFIFOB(fd,6);
				unsigned char buf[16];
				struct char_session_data sd;
				struct mmo_charstatus    st;
				int found_char = char_load_all(&sd,RFIFOL(fd,2));
				for(i=0;i<found_char;i++){
					int flag = 0;
					memcpy(&st,sd.found_char[i],sizeof(struct mmo_charstatus));
					//雷鳥は職も変更
					if(st.class == 19 || st.class == 20){
						flag = 1; st.class = (sex ? 19 : 20);
					} else if(st.class == 19 + PC_CLASS_BASE2 || st.class == 20 + PC_CLASS_BASE2) {
						flag = 1; st.class = (sex ? 19 : 20) + PC_CLASS_BASE2;
					} else if(st.class == 19 + PC_CLASS_BASE3 || st.class == 20 + PC_CLASS_BASE3) {
						flag = 1; st.class = (sex ? 19 : 20) + PC_CLASS_BASE3;
					}
					if(flag) {
						// 雷鳥装備外し
						for(j=0;j<MAX_INVENTORY;j++) {
							if(st.inventory[j].equip) st.inventory[j].equip=0;
						}
						// 雷鳥スキルリセット
						for(j=0;j<MAX_SKILL;j++) {
							if(st.skill[j].id>0 && !st.skill[j].flag){
								st.skill_point += st.skill[j].lv;
								st.skill[j].lv = 0;
							}
						}
						char_save(&st);	//キャラデータ変更のセーブ
					}
				}
				WBUFW(buf,0)=0x2b0d;
				WBUFL(buf,2)=RFIFOL(fd,2);
				WBUFB(buf,6)=RFIFOB(fd,6);
				mapif_sendall(buf,7);
				RFIFOSKIP(fd,7);
	//			printf("char -> map\n");
			}
			break;
			// account_reg2変更通知
		case 0x2729:
			{
				struct global_reg reg[ACCOUNT_REG2_NUM];
				unsigned char buf[4096];
				int j,p,acc;
				if(RFIFOREST(fd)<4)
					return 0;
				if(RFIFOREST(fd)<RFIFOW(fd,2))
					return 0;
				acc=RFIFOL(fd,4);
				for(p=8,j=0;p<RFIFOW(fd,2) && j<ACCOUNT_REG2_NUM;p+=36,j++){
					memcpy(reg[j].str,RFIFOP(fd,p),32);
					reg[j].value=RFIFOL(fd,p+32);
				}
				set_account_reg2(acc,j,reg);
				// 同垢ログインを禁止していれば送る必要は無い
				memcpy(buf,RFIFOP(fd,0),RFIFOW(fd,2));
				WBUFW(buf,0)=0x2b11;
				mapif_sendall(buf,WBUFW(buf,2));
				RFIFOSKIP(fd,RFIFOW(fd,2));
	//			printf("char: save_account_reg_reply\n");
			}
			break;
			// アカウント削除通知
		case 0x272a:
			{
				// 該当キャラクターの削除
				int i;
				struct char_session_data sd;
				int max = char_load_all(&sd,RFIFOL(fd,2));
				for(i=0;i<max;i++) {
					char_delete(sd.found_char[i]);
				}
				// 倉庫の削除
				storage_delete(RFIFOL(fd,2));
				RFIFOSKIP(fd,6);
			}
			break;
		//charメンテナンス状態変更応答
		case 0x272c:
			{
				unsigned char buf[10];
				if(RFIFOREST(fd)<3)
					return 0;
				WBUFW(buf,0)=0x2b15;
				WBUFB(buf,2)=RFIFOB(fd,2);
				mapif_sendall(buf,3);

				RFIFOSKIP(fd,3);
			}
			break;
		default:
			close(fd);
			session[fd]->eof=1;
			return 0;
		}
	}
	// RFIFOFLUSH(fd);
	return 0;
}

int char_erasemap(int fd,int id)
{
	unsigned char buf[16384];
	int i,j;

	WBUFW(buf,0)=0x2b16;
	WBUFL(buf,4)=server[id].ip;
	WBUFW(buf,8)=server[id].port;
	for(i=0,j=0;i<MAX_MAP_PER_SERVER;i++){
		if(server[id].map[i][0]>0){
			memcpy(WBUFP(buf,12+(j++)*16),server[id].map[i],16);
//			printf("char: map erase: %d %d %s\n",id,i,server[id].map[i]);
		}
	}
	WBUFW(buf,10)=j;
	WBUFW(buf,2)=j*16+12;
	mapif_sendallwos(fd,buf,WBUFW(buf,2));

	printf("char: map erase: %d (%d maps)\n",id,j);

	for(j=0;j<MAX_MAP_PER_SERVER;j++)
		server[id].map[j][0]=0;

	return 0;
}

int parse_map_disconnect_sub(void *key,void *data,va_list ap) {
	int fd   = va_arg(ap,int);
	int ip   = va_arg(ap,int);
	int port = va_arg(ap,int);
	struct char_online *c = (struct char_online*)data;
	unsigned char buf[10];

	if(c->ip == ip && c->port == port){
		printf("char: mapdisconnect %s %08x:%d\n",c->name,ip,port);
		WBUFW(buf,0) = 0x2b17;
		WBUFL(buf,2) = c->char_id;
		mapif_sendallwos(fd,buf,6);
		numdb_erase(char_online_db,key);
		free(c);
	}
	return 0;
}

int parse_map_disconnect(int fd) {
	int i,id;
	for(id=0;id<MAX_MAP_SERVERS;id++)
		if(server_fd[id]==fd)
			break;
	for(i=0;i<MAX_MAP_SERVERS;i++)
		if(server_fd[i]==fd)
			server_fd[i]=-1;
	char_erasemap(fd,id);
	// 残っていたキャラの切断を他map-serverに通知
	numdb_foreach(char_online_db,parse_map_disconnect_sub,fd,server[id].ip,server[id].port);
	close(fd);
	return 0;
}

int parse_frommap(int fd)
{
	int i,j;
	int id;

	for(id=0;id<MAX_MAP_SERVERS;id++)
		if(server_fd[id]==fd)
			break;
	if(id==MAX_MAP_SERVERS)
		session[fd]->eof=1;
	//printf("parse_frommap : %d %d %d\n",fd,RFIFOREST(fd),RFIFOW(fd,0));
	while(RFIFOREST(fd)>=2){
		switch(RFIFOW(fd,0)){
		// マップサーバーから担当マップ名を受信
		case 0x2afa:
			if(RFIFOREST(fd)<4 || RFIFOREST(fd)<RFIFOW(fd,2)) {
				return 0;
			}
			j=0;
			for(i=4;i<RFIFOW(fd,2);i+=16){
				int k = search_mapserver(RFIFOP(fd,i));
				if(k == -1 || k == id) {
					// 担当マップサーバーが決まってるか以前担当していたマップなら設定
					memcpy(server[id].map[j],RFIFOP(fd,i),16);
					j++;
				} else {
					// 担当マップサーバーが埋まっているなら失敗
					// printf("parse_frommap : set map %s failed.\n",RFIFOP(fd,i));
				}
			}
			i=server[id].ip;
			{
				unsigned char *p=(unsigned char *)&i;
				printf("set map %d from %d.%d.%d.%d:%d (%d maps)\n",
					id,p[0],p[1],p[2],p[3],server[id].port,j);
			}
			server[id].map[j][0]=0;
			RFIFOSKIP(fd,RFIFOW(fd,2));
			WFIFOW(fd,0)=0x2afb;
			WFIFOW(fd,2)=0;
			WFIFOSET(fd,3);
			{
				// 他のマップサーバーに担当マップ情報を送信
				// map 鯖はchar鯖からのこのパケットを受信して初めて、
				// 自分が担当するマップが分かる
				unsigned char buf[16384];
				int x;
				WBUFW(buf,0)=0x2b04;
				WBUFW(buf,2)=j*16+12;
				WBUFL(buf,4)=server[id].ip;
				WBUFW(buf,8)=server[id].port;
				WBUFW(buf,10)=i;
				for(i=0;i<j;i++){
					memcpy(WBUFP(buf,12+i*16),server[id].map[i],16);
				}
				mapif_sendall(buf,WBUFW(buf,2));
				// 他のマップサーバーの担当マップを送信
				for(x=0;x<MAX_MAP_SERVERS;x++){
					if(server_fd[x]>=0 && x!=id){
						WFIFOW(fd,0)=0x2b04;
						WFIFOL(fd,4)=server[x].ip;
						WFIFOW(fd,8)=server[x].port;
						for(i=0,j=0;i<MAX_MAP_PER_SERVER;i++){
							if(server[x].map[i][0]>0)
								memcpy(WFIFOP(fd,12+(j++)*16),server[x].map[i],16);
						}
						if(j>0){
							WFIFOW(fd,10)=j;
							WFIFOW(fd,2)=j*16+12;
							WFIFOSET(fd,WFIFOW(fd,2));
						}
					}
				}
			}
			break;
		// 認証要求
		case 0x2afc:
			if(RFIFOREST(fd)<23)
				return 0;
//			printf("auth_fifo search %08x %08x %08x %08x %08x\n",
//				RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),RFIFOL(fd,14),RFIFOL(fd,18));
			for(i=0;i<AUTH_FIFO_SIZE;i++){
				if( cmp_authfifo(i,RFIFOL(fd,2),RFIFOL(fd,10),RFIFOL(fd,14),RFIFOL(fd,18)) &&
					auth_fifo[i].char_id==RFIFOL(fd,6) &&
					!auth_fifo[i].delflag){
					auth_fifo[i].delflag=1;
					break;
				}
			}
			if(i == AUTH_FIFO_SIZE) {
				WFIFOW(fd,0)=0x2afe;
				WFIFOW(fd,2)=RFIFOL(fd,2);
				WFIFOB(fd,6)=0;
				WFIFOSET(fd,7);
				printf("auth_fifo search error!\n");
			} else {
				const struct mmo_charstatus *st = char_load(RFIFOL(fd,6));
				if(st == NULL || auth_fifo[i].sex != RFIFOB(fd,22)) {
					WFIFOW(fd,0)=0x2afe;
					WFIFOW(fd,2)=RFIFOL(fd,2);
					WFIFOB(fd,6)=0;
					WFIFOSET(fd,7);
					printf("auth_fifo search error!\n");
				} else {
					unsigned char buf[60];
					struct char_online *c;
					WFIFOW(fd,0) = 0x2afd;
					WFIFOW(fd,2) = 12 + sizeof(struct mmo_charstatus);
					WFIFOL(fd,4) = RFIFOL(fd,2); // account id
				//	WFIFOL(fd,8) = RFIFOL(fd,6); //
					WFIFOL(fd,8) = auth_fifo[i].login_id2;
					memcpy(WFIFOP(fd,12),st,sizeof(struct mmo_charstatus));
					WFIFOSET(fd,WFIFOW(fd,2));

					// オンラインdbに挿入
					c = numdb_search(char_online_db,RFIFOL(fd,2));
					if(c == NULL) {
						c = aCalloc(sizeof(struct char_online),1);
					}
					c->ip         = server[id].ip;
					c->port       = server[id].port;
					c->account_id = st->account_id;
					c->char_id    = st->char_id;
					memcpy(c->name,st->name,24);
					numdb_insert(char_online_db,RFIFOL(fd,2),c);

					// このmap-server以外にログインしたことを通知する
					WBUFW(buf, 0) = 0x2b09;
					WBUFL(buf, 2) = st->char_id;
					memcpy(WBUFP(buf,6),st->name,24);
					WBUFL(buf,30) = st->account_id;
					WBUFL(buf,34) = server[id].ip;
					WBUFW(buf,38) = server[id].port;
					mapif_sendallwos(fd,buf,40);
				}
			}
			RFIFOSKIP(fd,23);
			break;
		// MAPサーバー上のユーザー数受信
		case 0x2aff:
			if(RFIFOREST(fd)<6)
				return 0;
			server[id].users=RFIFOL(fd,2);
			RFIFOSKIP(fd,6);
			break;
		// キャラデータ保存
		case 0x2b01:
			if(RFIFOREST(fd)<4 || RFIFOREST(fd)<RFIFOW(fd,2))
				return 0;
			if( ((struct mmo_charstatus*)RFIFOP(fd,12))->char_id != RFIFOL(fd,8) ) {
				// キャラID違いのデータを送ってきたので強制切断
				char buf[256];
				WBUFW(buf,0) = 0x2b19;
				WBUFL(buf,2) = RFIFOL(fd,4);
				mapif_sendall(buf,6);
			} else {
				char_save((struct mmo_charstatus *)RFIFOP(fd,12));
			}
			RFIFOSKIP(fd,RFIFOW(fd,2));
			break;
		// キャラセレ要求
		case 0x2b02:
			if(RFIFOREST(fd)<19)
				return 0;

			if(auth_fifo_pos>=AUTH_FIFO_SIZE){
				auth_fifo_pos=0;
			}
//			printf("auth_fifo set 0x2b02 %d - %08x %08x %08x %08x\n",
//				auth_fifo_pos,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),RFIFOL(fd,14));
			auth_fifo[auth_fifo_pos].account_id = RFIFOL(fd,2);
			auth_fifo[auth_fifo_pos].char_id    = 0;
			auth_fifo[auth_fifo_pos].login_id1  = RFIFOL(fd,6);
			auth_fifo[auth_fifo_pos].login_id2  = RFIFOL(fd,10);
			auth_fifo[auth_fifo_pos].delflag    = 2;
			auth_fifo[auth_fifo_pos].tick       = gettick();
			auth_fifo[auth_fifo_pos].ip         = RFIFOL(fd,14);
			auth_fifo[auth_fifo_pos].sex        = WFIFOB(fd,18);
			auth_fifo_pos++;

			WFIFOW(fd,0) = 0x2b03;
			WFIFOL(fd,2) = RFIFOL(fd,2);
			WFIFOB(fd,6) = 0;
			WFIFOSET(fd,7);
			RFIFOSKIP(fd,19);
			break;

		// マップサーバー間移動要求
		case 0x2b05:
			if(RFIFOREST(fd)<41)
				return 0;

			if(auth_fifo_pos>=AUTH_FIFO_SIZE){
				auth_fifo_pos=0;
			}
			memcpy(WFIFOP(fd,2),RFIFOP(fd,2),38);
			WFIFOW(fd,0)=0x2b06;

//			printf("auth_fifo set 0x2b05 %d - %08x %08x\n",auth_fifo_pos,RFIFOL(fd,2),RFIFOL(fd,6));
			auth_fifo[auth_fifo_pos].account_id = RFIFOL(fd,2);
			auth_fifo[auth_fifo_pos].char_id    = RFIFOL(fd,10);
			auth_fifo[auth_fifo_pos].login_id1  = RFIFOL(fd,6);
			auth_fifo[auth_fifo_pos].delflag    = 0;
			auth_fifo[auth_fifo_pos].sex        = RFIFOB(fd,40);
			auth_fifo[auth_fifo_pos].tick       = gettick();
			auth_fifo[auth_fifo_pos].ip         = session[fd]->client_addr.sin_addr.s_addr;
			auth_fifo_pos++;

			WFIFOL(fd,6)=0;
			WFIFOSET(fd,40);
			RFIFOSKIP(fd,41);
			break;

		// キャラ名検索
		case 0x2b08:
			if(RFIFOREST(fd)<6)
				return 0;
			{
				const struct mmo_charstatus *st = char_load(RFIFOL(fd,2));
				WFIFOW(fd,0) = 0x2b09;
				WFIFOL(fd,2) = RFIFOL(fd,2);
				if(st){
					struct char_online* c = numdb_search(char_online_db,st->account_id);
					memcpy(WFIFOP(fd,6),st->name,24);
					WFIFOL(fd,30) = st->account_id;
					WFIFOL(fd,34) = (c && c->char_id == st->char_id ? c->ip   : 0);
					WFIFOW(fd,38) = (c && c->char_id == st->char_id ? c->port : 0);
				} else {
					memcpy(WFIFOP(fd,6),unknown_char_name,24);
					WFIFOL(fd,30)=0;
					WFIFOL(fd,34)=0;
					WFIFOW(fd,38)=0;
				}
				WFIFOSET(fd,40);
				RFIFOSKIP(fd,6);
			}
			break;

		// GMになりたーい
		case 0x2b0a:
			if(RFIFOREST(fd)<4)
				return 0;
			if(RFIFOREST(fd)<RFIFOW(fd,2))
				return 0;
			if(login_fd>0 && session[login_fd])
			{
				memcpy(WFIFOP(login_fd,2),RFIFOP(fd,2),RFIFOW(fd,2)-2);
				WFIFOW(login_fd,0)=0x2720;
				WFIFOSET(login_fd,RFIFOW(fd,2));
			}
//			printf("char : change gm -> login %d %s %d\n",RFIFOL(fd,4),RFIFOP(fd,8),RFIFOW(fd,2));
			RFIFOSKIP(fd,RFIFOW(fd,2));
			break;

		//性別変換要求
		case 0x2b0c:
			if(RFIFOREST(fd)<4)
				return 0;
			if(RFIFOREST(fd)<RFIFOW(fd,2))
				return 0;
			if(login_fd>0 && session[login_fd])
			{
				WFIFOW(login_fd,0) = 0x2722;
				WFIFOW(login_fd,2) = RFIFOW(fd,2);
				WFIFOL(login_fd,4) = RFIFOL(fd,4);
				WFIFOB(login_fd,8) = RFIFOB(fd,8);
				WFIFOSET(login_fd,RFIFOW(fd,2));
			}
//			printf("char : change sex -> login %d %d %d \n",RFIFOL(fd,4),RFIFOB(fd,8),RFIFOW(fd,2));
			RFIFOSKIP(fd,RFIFOW(fd,2));
			break;

		// account_reg保存要求
		case 0x2b10:
			{
				struct global_reg reg[ACCOUNT_REG2_NUM];
				int j,p,acc;
				if(RFIFOREST(fd)<4)
					return 0;
				if(RFIFOREST(fd)<RFIFOW(fd,2))
					return 0;
				acc=RFIFOL(fd,4);
				for(p=8,j=0;p<RFIFOW(fd,2) && j<ACCOUNT_REG2_NUM;p+=36,j++){
					memcpy(reg[j].str,RFIFOP(fd,p),32);
					reg[j].value=RFIFOL(fd,p+32);
				}
				set_account_reg2(acc,j,reg);
				// loginサーバーへ送る
				memcpy(WFIFOP(login_fd,0),RFIFOP(fd,0),RFIFOW(fd,2));
				if(login_fd>0 && session[login_fd])
				{
					WFIFOW(login_fd,0)=0x2728;
					WFIFOSET(login_fd,WFIFOW(login_fd,2));
				}
				// ワールドへの同垢ログインがなければmapサーバーに送る必要はない
				//memcpy(buf,RFIFOP(fd,0),RFIFOW(fd,2));
				//WBUFW(buf,0)=0x2b11;
				//mapif_sendall(buf,WBUFW(buf,2));
				RFIFOSKIP(fd,RFIFOW(fd,2));
	//			printf("char: save_account_reg (from map)\n");
			}
			break;
		//mapサーバ 有効化
		case 0x2b13:
			if(RFIFOREST(fd)<3)
				return 0;
			server[id].active=RFIFOB(fd,2);
			printf("char: map_server_active: %d %d\n",id,server[id].active);
			RFIFOSKIP(fd,3);
			break;
		//charサーバ メンテナンス状態に
		case 0x2b14:
			if(RFIFOREST(fd)<3)
				return 0;
			char_maintenance=RFIFOB(fd,2);
			printf("char: maintenance: %d\n",char_maintenance);
			//loginに通知
			if(login_fd>0 && session[login_fd])
			{
				WFIFOW(login_fd,0)=0x272b;
				WFIFOB(login_fd,2)=char_maintenance;
				WFIFOSET(login_fd,3);
			}
			RFIFOSKIP(fd,3);
			break;
		//キャラクター切断を他mapに通知
		case 0x2b18:
			if(RFIFOREST(fd)<10)
				return 0;
			{
				unsigned char buf[10];
				struct char_online *c = numdb_search(char_online_db,RFIFOL(fd,2));
				if(c){
					numdb_erase(char_online_db,RFIFOL(fd,2));
					aFree(c);
					WBUFW(buf,0) = 0x2b17;
					WBUFL(buf,2) = RFIFOL(fd,6);
					mapif_sendallwos(fd,buf,6);
				}
				RFIFOSKIP(fd,10);
			}
			break;

		//離婚
		case 0x2b20:
			if(RFIFOREST(fd)<6)
				return 0;
			{
				char buf[10];
				const struct mmo_charstatus *st1 = char_load(RFIFOL(fd,2));
				if( st1 && st1->partner_id ) {
					// 離婚情報をmapに通知
					WBUFW(buf,0)=0x2b12;
					WBUFL(buf,2)=st1->char_id;
					mapif_sendall(buf,6);
					// 離婚
					char_divorce(st1);
				}
				RFIFOSKIP(fd,6);
			}
			break;

		// 友達リスト削除
		case 0x2b24:
			if( RFIFOREST(fd)<18 )
				return 0;
			{
				const struct mmo_charstatus *cpst;

				if( (cpst = char_load(RFIFOL(fd,6)))!=NULL )
				{
					char buf[32];
					struct mmo_charstatus st = *cpst;
					int i;

					for( i=0; i<st.friend_num; i++ )
					{
						if( st.friend_data[i].account_id == RFIFOL(fd,10) &&
							st.friend_data[i].char_id == RFIFOL(fd,14) )
						{
							st.friend_num--;
							memmove( &st.friend_data[i], &st.friend_data[i+1], sizeof(st.friend_data[0])*(st.friend_num-i) );
							break;
						}
					}
					char_save(&st);

					memcpy( buf, RFIFOP(fd,0), 18 );
					WBUFW(buf,0) = 0x2b25;
					mapif_sendallwos(fd,buf,18);
				}
				RFIFOSKIP(fd,18);
			}
			break;
		// 友達リストオンライン通知
		case 0x2b26:
			if( RFIFOREST(fd)<4 || RFIFOREST(fd)<RFIFOW(fd,2) )
				return 0;
			{
				char buf[MAX_FRIEND*8+32];
				if( RFIFOW(fd,2) <= MAX_FRIEND*8+16 )
				{
					memcpy( buf, RFIFOP(fd,0), RFIFOW(fd,2) );
					WBUFW(buf,0) = 0x2b27;
					mapif_sendallwos(fd,buf,RFIFOW(fd,2));
				}
				RFIFOSKIP(fd,RFIFOW(fd,2));
			}
			break;

		default:
			// inter server処理に渡す
			{
				int r=inter_parse_frommap(fd);
				if( r==1 )	break;		// 処理できた
				if( r==2 )	return 0;	// パケット長が足りない
			}

			// inter server処理でもない場合は切断
			printf("char: unknown packet %x! (from map)\n",RFIFOW(fd,0));
			close(fd);
			session[fd]->eof=1;
			return 0;
		}
	}
	return 0;
}
/*==========================================
 * mapが含まれているmap-serverを探す
 *------------------------------------------
 */

int search_mapserver(char *map) {
	int i , j;
	char map_temp[32];
	strcpy(map_temp,map);
	if(strstr(map_temp,".gat") == NULL) {
		strcat(map_temp,".gat");
	}
	for(i=0;i<MAX_MAP_SERVERS;i++){
		if(server_fd[i]<0)
			continue;
		for(j=0;server[i].map[j][0];j++){
			if(!strcmp(server[i].map[j],map_temp)){
				return i;
			}
		}
	}
	return -1;
}

int search_mapserver_char(char *map, struct mmo_charstatus *cd)
{
	int i , j;
	i = search_mapserver(map);
	if(i != -1) {
//		printf("search_mapserver %s : success -> %d\n",map,i);
		return i;
	}
	if(cd) {
		for(i=0;i<MAX_MAP_SERVERS;i++){
			if(server_fd[i]<0)
				continue;
			for(j=0;server[i].map[j][0];j++){
				memcpy(cd->last_point.map,server[i].map[j],16);
				printf("search_mapserver %s : another map %s -> %d\n",map,server[i].map[j],i);
				return i;
			}
		}
	}
	printf("search_mapserver failed : %s\n",map);
	return -1;
}

// char_mapifの初期化処理（現在はinter_mapif初期化のみ）
static int char_mapif_init(int fd)
{
	return inter_mapif_init(fd);
}

int parse_char_disconnect(int fd) {
	if(fd==login_fd)
		login_fd=0;
	return 0;
}

// 他マップにログインしているキャラクター情報を送信する
int parse_char_sendonline(void *key,void *data,va_list ap) {
	int fd   = va_arg(ap,int);
	struct char_online *c = (struct char_online*)data;
	WFIFOW(fd, 0) = 0x2b09;
	WFIFOL(fd, 2) = c->char_id;
	memcpy(WFIFOP(fd,6),c->name,24);
	WFIFOL(fd,30) = c->account_id;
	WFIFOL(fd,34) = c->ip;
	WFIFOW(fd,38) = c->port;
	WFIFOSET(fd,40);
	return 0;
}

int parse_char(int fd)
{
	int i,ch;
	unsigned short cmd;
	struct char_session_data *sd;

	if(login_fd<=0) {
		session[fd]->eof=1;
		return 0;
	}
	sd=session[fd]->session_data;
	while(RFIFOREST(fd)>=2){
		cmd = RFIFOW(fd,0);
		// crc32のスキップ用
		if(	sd==NULL			&&	// 未ログインor管理パケット
			RFIFOREST(fd)>=4	&&	// 最低バイト数制限 ＆ 0x7530,0x7532管理パケ除去
			RFIFOREST(fd)<=21	&&	// 最大バイト数制限 ＆ サーバーログイン除去
			cmd!=0x20b			&&	// md5通知パケット除去
			cmd!=0x228			&&
			(RFIFOREST(fd)<6 || RFIFOW(fd,4)==0x65)	)	// 次に何かパケットが来てるなら、接続でないとだめ
		{
			RFIFOSKIP(fd,4);
			cmd = RFIFOW(fd,0);
//			printf("parse_char : %d crc32 skipped\n",fd);
			if(RFIFOREST(fd)==0)
				return 0;
		}

//		if(cmd<30000 && cmd!=0x187)
//			printf("parse_char : %d %d %d\n",fd,RFIFOREST(fd),cmd);

		// 不正パケットの処理
		if (sd == NULL && cmd != 0x65 && cmd != 0x20b && cmd != 0x187 && cmd!=0x258 && cmd!=0x228 &&
					 cmd != 0x2af8 && cmd != 0x7530 && cmd != 0x7532)
			cmd = 0xffff;	// パケットダンプを表示させる

		switch(cmd){
		case 0x20b:		//20040622暗号化ragexe対応
			if(RFIFOREST(fd)<19)
				return 0;
			RFIFOSKIP(fd,19);
			break;
		case 0x258:		//20051214 nProtect関係 Part 1
			memset(WFIFOP(fd,0),0,18);
			WFIFOW(fd,0)=0x0227;
			WFIFOSET(fd,18);
			RFIFOSKIP(fd,2);
			break;
		case 0x228:		//20051214 nProtect関係 Part 2
			if(RFIFOREST(fd)<18)
				return 0;
			WFIFOW(fd,0)=0x0259;
			WFIFOB(fd,2)=2;
			WFIFOSET(fd,3);
			RFIFOSKIP(fd,18);
			break;

		case 0x65:	// 接続要求
			if(RFIFOREST(fd)<17)
				return 0;
			if(sd==NULL)
				sd=session[fd]->session_data=(struct char_session_data *)aCalloc(1,sizeof(*sd));
			sd->account_id=RFIFOL(fd,2);
			sd->login_id1=RFIFOL(fd,6);
			sd->login_id2=RFIFOL(fd,10);
			sd->sex=RFIFOB(fd,16);
			sd->state=CHAR_STATE_WAITAUTH;

			WFIFOL(fd,0)=RFIFOL(fd,2);
			WFIFOSET(fd,4);

			for(i=0;i<AUTH_FIFO_SIZE;i++){
				if(cmp_authfifo(i,sd->account_id,sd->login_id1,sd->login_id2,session[fd]->client_addr.sin_addr.s_addr) &&
				   auth_fifo[i].delflag==2){
					auth_fifo[i].delflag=1;
					sd->account_id=auth_fifo[i].account_id;
					sd->login_id1=auth_fifo[i].login_id1;
					sd->login_id2=auth_fifo[i].login_id2;
					break;
				}
			}
			if(i==AUTH_FIFO_SIZE){
				if(login_fd>0 && session[login_fd])
				{
					WFIFOW(login_fd,0)=0x2712;
					WFIFOL(login_fd,2)=sd->account_id;
					WFIFOL(login_fd,6)=sd->login_id1;
					WFIFOL(login_fd,10)=sd->login_id2;
					WFIFOB(login_fd,14)=sd->sex;
					WFIFOL(login_fd,15)=session[fd]->client_addr.sin_addr.s_addr;
					WFIFOL(login_fd,19)=sd->account_id;
					WFIFOSET(login_fd,23);
				}
			} else {
				if(char_maintenance && isGM(sd->account_id)==0){
					close(fd);
					session[fd]->eof=1;
					return 0;
				}
				if(max_connect_user > 0) {
					if(count_users() < max_connect_user  || isGM(sd->account_id)>0)
						mmo_char_send006b(fd,sd);
					else {
						WFIFOW(fd,0)=0x6c;
						WFIFOW(fd,2)=0;
						WFIFOSET(fd,3);
					}
				}
				else
				{
					mmo_char_send006b(fd,sd);
				}
			}

			RFIFOSKIP(fd,17);
			break;
		case 0x66:	// キャラ選択
			if(RFIFOREST(fd)<3)
				return 0;
			{
				struct char_online *c;
				struct mmo_charstatus st;
				for(ch=0;ch<9;ch++)
					if(sd->found_char[ch] && sd->found_char[ch]->char_num == RFIFOB(fd,2))
						break;
				RFIFOSKIP(fd,3);
				if(ch == 9) break;

				char_log("char select %d-%d %s",sd->account_id,RFIFOB(fd,2),sd->found_char[ch]->name);
				memcpy(&st,sd->found_char[ch],sizeof(struct mmo_charstatus));
				i = search_mapserver_char(st.last_point.map,NULL);
				if(i < 0) {
					if(default_map_type & 1){
						memcpy(st.last_point.map,default_map_name,16);
						i=search_mapserver_char(st.last_point.map,NULL);
					}
					if(default_map_type & 2 && i < 0){
						i=search_mapserver_char(st.last_point.map,&st);
					}
					if(i >= 0) {
						// 現在地が書き換わったので上書き
						char_save(&st);
					}
				}
				if(strstr(st.last_point.map,".gat") == NULL) {
					strcat(st.last_point.map,".gat");
					char_save(&st);
				}
				if(i < 0 || server[i].active==0){
					WFIFOW(fd,0)=0x6c;
					WFIFOW(fd,2)=0;
					WFIFOSET(fd,3);
					break;
				}
				// ２重ログイン撃退（違うマップサーバの場合）
				// 同じマップサーバの場合は、マップサーバー内で処理される
				c = numdb_search(char_online_db,sd->found_char[ch]->account_id);
				if(c && (c->ip != server[i].ip || c->port != server[i].port) ) {
					// ２重ログイン検出
					// mapに切断要求
					char buf[256];
					WBUFW(buf,0) = 0x2b1a;
					WBUFL(buf,2) = sd->account_id;
					mapif_sendall(buf,6);

					// 接続失敗送信
					WFIFOW(fd,0)=0x6c;
					WFIFOW(fd,2)=0;
					WFIFOSET(fd,3);
					break;
				}

				WFIFOW(fd,0) = 0x71;
				WFIFOL(fd,2) = st.char_id;
				memcpy(WFIFOP(fd,6),st.last_point.map,16);
				WFIFOL(fd,22) = server[i].ip;
				WFIFOW(fd,26) = server[i].port;
				WFIFOSET(fd,28);

				if(auth_fifo_pos>=AUTH_FIFO_SIZE){
					auth_fifo_pos=0;
				}
//				printf("auth_fifo set 0x66 %d - %08x %08x %08x %08x\n",
//					auth_fifo_pos,sd->account_id,st.char_id,sd->login_id1,sd->login_id2);
				auth_fifo[auth_fifo_pos].account_id = sd->account_id;
				auth_fifo[auth_fifo_pos].char_id    = st.char_id;
				auth_fifo[auth_fifo_pos].login_id1  = sd->login_id1;
				auth_fifo[auth_fifo_pos].login_id2  = sd->login_id2;
				auth_fifo[auth_fifo_pos].delflag    = 0;
				auth_fifo[auth_fifo_pos].sex        = sd->sex;
				auth_fifo_pos++;
			}
			break;
		case 0x67:	// 作成
			if(RFIFOREST(fd)<37)
				return 0;
			{
				int flag=0x00;
				const struct mmo_charstatus *st = char_make(sd,RFIFOP(fd,2),&flag);
				if(st == NULL){
					WFIFOW(fd,0)=0x6e;
					WFIFOB(fd,2)=flag;
					WFIFOSET(fd,3);
					RFIFOSKIP(fd,37);
					break;
				}

				memset(WFIFOP(fd,2),0x00,106);
				WFIFOW(fd,0)   = 0x6d;
				WFIFOL(fd,2    ) = st->char_id;
				WFIFOL(fd,2+  4) = st->base_exp;
				WFIFOL(fd,2+  8) = st->zeny;
				WFIFOL(fd,2+ 12) = st->job_exp;
				WFIFOL(fd,2+ 16) = st->job_level;
				WFIFOL(fd,2+ 28) = st->karma;
				WFIFOL(fd,2+ 32) = st->manner;
				WFIFOW(fd,2+ 40) = 0x30;
				WFIFOW(fd,2+ 42) = (st->hp     > 0x7fff) ? 0x7fff : st->hp;
				WFIFOW(fd,2+ 44) = (st->max_hp > 0x7fff) ? 0x7fff : st->max_hp;
				WFIFOW(fd,2+ 46) = (st->sp     > 0x7fff) ? 0x7fff : st->sp;
				WFIFOW(fd,2+ 48) = (st->max_sp > 0x7fff) ? 0x7fff : st->max_sp;
				WFIFOW(fd,2+ 50) = DEFAULT_WALK_SPEED; // char_dat[i].speed;
				WFIFOW(fd,2+ 52) = st->class;
				WFIFOW(fd,2+ 54) = st->hair;
				WFIFOW(fd,2+ 58) = st->base_level;
				WFIFOW(fd,2+ 60) = st->skill_point;
				WFIFOW(fd,2+ 64) = st->shield;
				WFIFOW(fd,2+ 66) = st->head_top;
				WFIFOW(fd,2+ 68) = st->head_mid;
				WFIFOW(fd,2+ 70) = st->hair_color;
				memcpy( WFIFOP(fd,2+74), st->name, 24 );
				WFIFOB(fd,2+ 98) = (st->str  > 255) ? 255 : st->str;
				WFIFOB(fd,2+ 99) = (st->agi  > 255) ? 255 : st->agi;
				WFIFOB(fd,2+100) = (st->vit  > 255) ? 255 : st->vit;
				WFIFOB(fd,2+101) = (st->int_ > 255) ? 255 : st->int_;
				WFIFOB(fd,2+102) = (st->dex  > 255) ? 255 : st->dex;
				WFIFOB(fd,2+103) = (st->luk  > 255) ? 255 : st->luk;
				WFIFOB(fd,2+104) = st->char_num;
				WFIFOSET(fd,108);
				RFIFOSKIP(fd,37);
				for(ch=0;ch<9;ch++) {
					if(sd->found_char[ch] == NULL) {
						sd->found_char[ch] = st;
						break;
					}
				}
			}
		case 0x68:	// 削除
			if(RFIFOREST(fd)<46)
				return 0;
#if defined(TXT_ONLY) && defined(AC_MAIL)
			WFIFOW(login_fd,0)=0x2715;
			WFIFOL(login_fd,2)=sd->account_id;
			WFIFOL(login_fd,6)=RFIFOL(fd,2);
			memcpy( WFIFOP(login_fd,10), RFIFOP(fd,6), 40 );
			WFIFOSET(login_fd,50);
#else /* TXT_ONLY && AC_MAIL */
			for(i=0;i<9;i++){
				const struct mmo_charstatus *cs = sd->found_char[i];
				if(cs && cs->char_id == RFIFOL(fd,2)){
					char_delete(cs);
					for(ch=i;ch<9-1;ch++)
						sd->found_char[ch]=sd->found_char[ch+1];
					sd->found_char[8] = NULL;
					break;
				}
			}
			if( i==9 ){
				WFIFOW(fd,0)=0x70;
				WFIFOB(fd,2)=0;
				WFIFOSET(fd,3);
			} else {
				WFIFOW(fd,0)=0x6f;
				WFIFOSET(fd,2);
			}
#endif
			RFIFOSKIP(fd,46);
			break;
		case 0x2af8:	// マップサーバーログイン
			if(RFIFOREST(fd)<60)
				return 0;
			WFIFOW(fd,0)=0x2af9;
			for(i=0;i<MAX_MAP_SERVERS;i++){
				if(server_fd[i]<0)
					break;
			}
			if( char_sport != 0 && char_port != char_sport && session[fd]->server_port != char_sport ) {
				printf("server login failed: connected port %d\n", session[fd]->server_port);
				session[fd]->eof = 1;
				return 0;
			}
			if(i==MAX_MAP_SERVERS || strcmp(RFIFOP(fd,2),userid) || strcmp(RFIFOP(fd,26),passwd)){
				WFIFOB(fd,2)=3;
				WFIFOSET(fd,3);
				RFIFOSKIP(fd,60);
			} else {
				WFIFOB(fd,2)=0;
				session[fd]->func_parse=parse_frommap;
				session[fd]->func_destruct = parse_map_disconnect;
				server_fd[i]=fd;
				server[i].ip=RFIFOL(fd,54);
				server[i].port=RFIFOW(fd,58);
				server[i].users=0;
				memset(server[i].map,0,sizeof(server[i].map));
				WFIFOSET(fd,3);
				numdb_foreach(char_online_db,parse_char_sendonline,fd);
				RFIFOSKIP(fd,60);
				session[fd]->auth = -1; // 認証終了を socket.c に伝える
				realloc_fifo(fd,FIFOSIZE_SERVERLINK,FIFOSIZE_SERVERLINK);
				char_mapif_init(fd);
				return 0;
			}
			break;
		case 0x187:	// Alive信号？
			if (RFIFOREST(fd) < 6) {
				return 0;
			}
			WFIFOW(fd,0)=0x187;
			WFIFOL(fd,2)=sd->account_id;
			WFIFOSET(fd,6);
			RFIFOSKIP(fd, 6);
			break;

		case 0x7530:	// Athena情報所得
			WFIFOW(fd,0)=0x7531;
			WFIFOB(fd,2)=ATHENA_MAJOR_VERSION;
			WFIFOB(fd,3)=ATHENA_MINOR_VERSION;
			WFIFOB(fd,4)=ATHENA_REVISION;
			WFIFOB(fd,5)=ATHENA_RELEASE_FLAG;
			WFIFOB(fd,6)=ATHENA_OFFICIAL_FLAG;
			WFIFOB(fd,7)=ATHENA_SERVER_INTER | ATHENA_SERVER_CHAR;
			WFIFOW(fd,8)=ATHENA_MOD_VERSION;
			WFIFOSET(fd,10);
			RFIFOSKIP(fd,2);
			return 0;
		case 0x7532:	// 接続の切断(defaultと処理は一緒だが明示的にするため)
			close(fd);
			session[fd]->eof=1;
			return 0;

		default:
#ifdef DUMP_UNKNOWN_PACKET
			{
				int i;
				printf("---- 00-01-02-03-04-05-06-07-08-09-0A-0B-0C-0D-0E-0F");
				for(i=0;i<RFIFOREST(fd);i++){
					if((i&15)==0)
						printf("\n%04X ",i);
					printf("%02X ",RFIFOB(fd,i));
				}
				printf("\n");
			}
#endif
			close(fd);
			session[fd]->eof=1;
			return 0;
		}
	}
	// RFIFOFLUSH(fd);
	return 0;
}

// 全てのMAPサーバーにデータ送信（送信したmap鯖の数を返す）
int mapif_sendall(unsigned char *buf,unsigned int len)
{
	int i,c;
	for(i=0,c=0;i<MAX_MAP_SERVERS;i++){
		int fd;
		if((fd=server_fd[i])>0){
			memcpy(WFIFOP(fd,0),buf,len);
			WFIFOSET(fd,len);
			c++;
		}
	}
	return c;
}
// 自分以外の全てのMAPサーバーにデータ送信（送信したmap鯖の数を返す）
int mapif_sendallwos(int sfd,unsigned char *buf,unsigned int len)
{
	int i,c;
	for(i=0,c=0;i<MAX_MAP_SERVERS;i++){
		int fd;
		if((fd=server_fd[i])>0 && fd!=sfd){
			memcpy(WFIFOP(fd,0),buf,len);
			WFIFOSET(fd,len);
			c++;
		}
	}
	return c;
}
// MAPサーバーにデータ送信（map鯖生存確認有り）
int mapif_send(int fd,unsigned char *buf,unsigned int len)
{
	int i;
	for(i=0;i<MAX_MAP_SERVERS;i++){
		if((fd==server_fd[i])>0){
			memcpy(WFIFOP(fd,0),buf,len);
			WFIFOSET(fd,len);
			return 1;
		}
	}
	return 0;
}

int send_users_tologin(int tid,unsigned int tick,int id,int data)
{
	if(login_fd>0 && session[login_fd]){
		int i,users;
		for(i=0,users=0;i<MAX_MAP_SERVERS;i++)
			if(server_fd[i]>0)
				users+=server[i].users;

		WFIFOW(login_fd,0)=0x2714;
		WFIFOL(login_fd,2)=users;
		WFIFOSET(login_fd,6);

		for(i=0;i<MAX_MAP_SERVERS;i++){
			int fd;
			if((fd=server_fd[i])>0){
				WFIFOW(fd,0)=0x2b00;
				WFIFOL(fd,2)=users;
				WFIFOSET(fd,6);
			}
		}
	}
	return 0;
}

int check_connect_login_server(int tid,unsigned int tick,int id,int data)
{
	if(login_fd<=0 || session[login_fd]==NULL){
		login_fd=make_connection(login_ip,login_port);
		if(login_fd <= 0) {
			if(char_loginaccess_autorestart>= 1) { exit(1); }
			return 0;
		}
		session[login_fd]->func_parse=parse_tologin;
		session[login_fd]->func_destruct = parse_login_disconnect;
		realloc_fifo(login_fd,FIFOSIZE_SERVERLINK,FIFOSIZE_SERVERLINK);
		WFIFOW(login_fd,0)=0x2710;
		memcpy(WFIFOP(login_fd,2),userid,24);
		memcpy(WFIFOP(login_fd,26),passwd,24);
		WFIFOL(login_fd,50)=0;
		WFIFOL(login_fd,54)=char_ip;
		WFIFOW(login_fd,58)=char_port;
		memcpy(WFIFOP(login_fd,60),server_name,20);
		WFIFOW(login_fd,80)=char_maintenance;
		WFIFOW(login_fd,82)=char_new;
		WFIFOSET(login_fd,84);
	}
	return 0;
}

int char_config_read(const char *cfgName)
{
	struct hostent *h=NULL;
	char line[1024],w1[1024],w2[1024];
	int i;
	FILE *fp=fopen(cfgName,"r");
	if(fp==NULL){
		printf("file not found: %s\n",cfgName);
		return 0;
	}

	while(fgets(line,1020,fp)){
		if(line[0] == '/' && line[1] == '/')
			continue;

		i=sscanf(line,"%[^:]: %[^\r\n]",w1,w2);
		if(i!=2)
			continue;
		if(strcmpi(w1,"userid")==0){
			memcpy(userid,w2,24);
		}
		else if(strcmpi(w1,"passwd")==0){
			memcpy(passwd,w2,24);
		}
		else if(strcmpi(w1,"server_name")==0){
			memcpy(server_name,w2,16);
		}
		else if(strcmpi(w1,"login_ip")==0){
			h = gethostbyname (w2);
			if(h != NULL) {
				printf("Login server IP address : %s -> %d.%d.%d.%d\n",w2,(unsigned char)h->h_addr[0],(unsigned char)h->h_addr[1],(unsigned char)h->h_addr[2],(unsigned char)h->h_addr[3]);
				sprintf(login_ip_str,"%d.%d.%d.%d",(unsigned char)h->h_addr[0],(unsigned char)h->h_addr[1],(unsigned char)h->h_addr[2],(unsigned char)h->h_addr[3]);
			}
			else
				memcpy(login_ip_str,w2,16);
		}
		else if(strcmpi(w1,"login_port")==0){
			login_port=atoi(w2);
		}
		else if(strcmpi(w1,"char_ip")==0){
			h = gethostbyname (w2);
			if(h != NULL) {
				printf("Character server IP address : %s -> %d.%d.%d.%d\n",w2,(unsigned char)h->h_addr[0],(unsigned char)h->h_addr[1],(unsigned char)h->h_addr[2],(unsigned char)h->h_addr[3]);
				sprintf(char_ip_str,"%d.%d.%d.%d",(unsigned char)h->h_addr[0],(unsigned char)h->h_addr[1],(unsigned char)h->h_addr[2],(unsigned char)h->h_addr[3]);
			}
			else
				memcpy(char_ip_str,w2,16);
		}
		else if(strcmpi(w1,"char_port")==0){
			char_port=atoi(w2);
		}
		else if(strcmpi(w1,"char_sip")==0){
			h = gethostbyname (w2);
			if(h != NULL) {
				printf("Character server sIP address : %s -> %d.%d.%d.%d\n",w2,(unsigned char)h->h_addr[0],(unsigned char)h->h_addr[1],(unsigned char)h->h_addr[2],(unsigned char)h->h_addr[3]);
				sprintf(char_sip_str,"%d.%d.%d.%d",(unsigned char)h->h_addr[0],(unsigned char)h->h_addr[1],(unsigned char)h->h_addr[2],(unsigned char)h->h_addr[3]);
			}
			else
				memcpy(char_sip_str,w2,16);
			char_sip  = inet_addr(char_sip_str);
		}
		else if(strcmpi(w1,"char_sport")==0){
			char_sport=atoi(w2);
		}
		else if(strcmpi(w1,"char_maintenance")==0){
			char_maintenance=atoi(w2);
		}
		else if(strcmpi(w1,"char_loginaccess_autorestart")==0){
			char_loginaccess_autorestart=atoi(w2);
		}
		else if(strcmpi(w1,"char_new")==0){
			char_new=atoi(w2);
		}
		else if(strcmpi(w1,"max_connect_user")==0){
			max_connect_user=atoi(w2);
		}
		else if(strcmpi(w1,"autosave_time")==0){
			autosave_interval=atoi(w2)*1000;
			if(autosave_interval <= 0)
				autosave_interval = DEFAULT_AUTOSAVE_INTERVAL_CS;
		}
		else if(strcmpi(w1,"start_point")==0){
			char map[32];
			int x,y;
			if( sscanf(w2,"%[^,],%d,%d",map,&x,&y)<3 )
				continue;
			memcpy(start_point.map,map,16);
			start_point.x=x;
			start_point.y=y;
		}
		else if(strcmpi(w1,"start_zeny")==0){
			start_zeny=atoi(w2);
			if(start_zeny < 0) start_zeny = 0;
		}
		else if(strcmpi(w1,"unknown_char_name")==0){
			strncpy(unknown_char_name,w2,1024);
			unknown_char_name[24] = 0;
		}
		else if(strcmpi(w1,"char_log_filename")==0){
			strncpy(char_log_filename,w2,1024);
		}
		else if(strcmpi(w1,"default_map_type")==0){
			default_map_type=atoi(w2);
		}
		else if(strcmpi(w1,"default_map_name")==0){
			strncpy(default_map_name,w2,16);
		}
		else if(strcmpi(w1,"httpd_enable")==0){
			socket_enable_httpd( atoi(w2) );
		}
		else if(strcmpi(w1,"httpd_document_root")==0){
			httpd_set_document_root( w2 );
		}
		else if(strcmpi(w1,"httpd_log_filename")==0){
			httpd_set_logfile( w2 );
		}
		else if(strcmpi(w1,"httpd_config")==0){
			httpd_config_read(w2);
		}
		else if(strcmpi(w1,"import")==0){
			char_config_read(w2);
		}
		else char_config_read_sub(w1,w2);
	}
	fclose(fp);

	return 0;
}


void char_socket_ctrl_panel_func(int fd,char* usage,char* user,char* status)
{
	struct socket_data *sd = session[fd];
	struct char_session_data *cd = sd->session_data;
	strcpy( usage,
		( sd->func_parse == parse_char )? "char user" :
		( sd->func_parse == parse_tologin )? "login server" :
		( sd->func_parse == parse_frommap)? "map server" : "unknown" );

	if( sd->func_parse == parse_tologin )
	{
		strcpy( user, userid );
	}
	else if( sd->func_parse == parse_char && sd->auth )
	{
		sprintf( user, "%d", cd->account_id );
	}
}


static int gm_account_db_final(void *key,void *data,va_list ap)
{
	struct gm_account *p=data;

	free(p);

	return 0;
}

static int char_online_db_final(void *key,void *data,va_list ap)
{
	struct char_online *p=data;

	free(p);

	return 0;
}

void do_final(void)
{
	int i;

	char_sync();
	inter_sync();
	do_final_inter();
	pet_final();
	homun_final();
	guild_final();
	party_final();
	storage_final();
	gstorage_final();
	mail_final();
	if(gm_account_db)
		numdb_final(gm_account_db,gm_account_db_final);
	delete_session(login_fd);
	delete_session(char_fd);
	if(char_sport != 0 && char_port != char_sport)
		delete_session(char_sfd);
	for(i=0;i<MAX_MAP_SERVERS;i++){
		if(server_fd[i]>0)
			delete_session(server_fd[i]);
	}
	numdb_final(char_online_db,char_online_db_final);
	char_final();
	exit_dbn();
	do_final_timer();

}

int do_init(int argc,char **argv)
{
	int i;

	printf("Athena Char  Server [%s] v%d.%d.%d mod%d\n",
#ifdef TXT_ONLY
		"TXT",
#else
		"SQL",
#endif
		ATHENA_MAJOR_VERSION,ATHENA_MINOR_VERSION,ATHENA_REVISION,
		ATHENA_MOD_VERSION
	);

	char_config_read((argc<2)? CHAR_CONF_NAME:argv[1]);

	login_ip = inet_addr(login_ip_str);
	char_ip  = inet_addr(char_ip_str);

	for(i=0;i<MAX_MAP_SERVERS;i++){
		server_fd[i]=-1;
		server[i].active=0;
	}
	char_online_db = numdb_init();
	char_init();
	read_gm_account();
	inter_init((argc>2)?argv[2]:inter_cfgName);	// inter server 初期化

	set_defaultparse(parse_char);
	set_sock_destruct(parse_char_disconnect);
	socket_set_httpd_page_connection_func( char_socket_ctrl_panel_func );

	char_fd = make_listen_port(char_port, 0);
	if(char_sport != 0 && char_port != char_sport)
		char_sfd = make_listen_port(char_sport, char_sip);

	add_timer_func_list(check_connect_login_server,"check_connect_login_server");
	add_timer_func_list(send_users_tologin,"send_users_tologin");
	add_timer_func_list(mmo_char_sync_timer,"mmo_char_sync_timer");

	i=add_timer_interval(gettick()+1000,check_connect_login_server,0,0,10*1000);
	i=add_timer_interval(gettick()+1000,send_users_tologin,0,0,5*1000);
	i=add_timer_interval(gettick()+autosave_interval,mmo_char_sync_timer,0,0,autosave_interval);

	// for httpd support
	do_init_httpd();
	do_init_graph();
	graph_add_sensor("Uptime(days)",60*1000,uptime);
	graph_add_sensor("Memory Usage(KB)",60*1000,memmgr_usage);
	httpd_default_page(httpd_send_file);

	return 0;
}
int mapif_parse_CharConnectLimit(int fd){
	int limit = RFIFOL(fd,2);
	if(limit < 0) limit=0;
	printf("char:max_connect_user change %d->%d\n",max_connect_user,limit);
	max_connect_user = limit;
	return 0;
}
