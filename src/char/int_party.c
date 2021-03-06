
#define _INT_PARTY_C_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inter.h"
#include "int_party.h"
#include "mmo.h"
#include "char.h"
#include "socket.h"
#include "db.h"
#include "lock.h"
#include "malloc.h"
#include "journal.h"


static int party_share_level = 10;


int party_check_empty(const struct party *p);

#ifdef TXT_ONLY

static char party_txt[1024]="save/party.txt";
static struct dbt *party_db;
static int party_newid=100;

#ifdef TXT_JOURNAL
static int party_journal_enable = 1;
static struct journal party_journal;
static char party_journal_file[1024]="./save/party.journal";
static int party_journal_cache = 1000;
#endif

// パーティデータの文字列への変換
int party_tostr(char *str,struct party *p)
{
	int i,len;
	len=sprintf(str,"%d\t%s\t%d,%d\t",
		p->party_id,p->name,p->exp,p->item);
	for(i=0;i<MAX_PARTY;i++){
		struct party_member *m = &p->member[i];
		len+=sprintf(str+len,"%d,%d\t%s\t",
			m->account_id,m->leader,
			((m->account_id>0)?m->name:"NoMember"));
	}
	return 0;
}

// パーティデータの文字列からの変換
int party_fromstr(char *str,struct party *p)
{
	int i,j,s;
	int tmp_int[16];
	char tmp_str[256];
	
	memset(p,0,sizeof(struct party));
	
//	printf("sscanf party main info\n");
	s=sscanf(str,"%d\t%[^\t]\t%d,%d\t",&tmp_int[0],
		tmp_str,&tmp_int[1],&tmp_int[2]);
	if(s!=4)
		return 1;
	
	p->party_id=tmp_int[0];
	strncpy(p->name,tmp_str,24);
	p->exp=tmp_int[1];
	p->item=tmp_int[2];	
//	printf("%d [%s] %d %d\n",tmp_int[0],tmp_str[0],tmp_int[1],tmp_int[2]);
	
	for(j=0;j<3 && str!=NULL;j++)
		str=strchr(str+1,'\t');
	
	for(i=0;i<MAX_PARTY;i++){
		struct party_member *m = &p->member[i];
		if(str==NULL)
			return 1;
//		printf("sscanf party member info %d\n",i);

		s=sscanf(str+1,"%d,%d\t%[^\t]\t",
			&tmp_int[0],&tmp_int[1],tmp_str);
		if(s!=3)
			return 1;
		
		m->account_id=tmp_int[0];
		m->leader=tmp_int[1];
		strncpy(m->name,tmp_str,sizeof(m->name));
//		printf(" %d %d [%s]\n",tmp_int[0],tmp_int[1],tmp_str);
		
		
		for(j=0;j<2 && str!=NULL;j++)
			str=strchr(str+1,'\t');
	}
	return 0;
}

#ifdef TXT_JOURNAL
// ==========================================
// パーティーのジャーナルのロールフォワード用コールバック関数
// ------------------------------------------
int party_journal_rollforward( int key, void* buf, int flag )
{
	struct party* p = numdb_search( party_db, key );
	
	// 念のためチェック
	if( flag == JOURNAL_FLAG_WRITE && key != ((struct party*)buf)->party_id )
	{
		printf("int_party: journal: key != party_id !\n");
		return 0;
	}
	
	// データの置き換え
	if( p )
	{
		if( flag == JOURNAL_FLAG_DELETE ) {
			numdb_erase( party_db, key );
			aFree( p );
		} else {
			memcpy( p, buf, sizeof(struct party) );
		}
		return 1;
	}
	
	// 追加
	if( flag != JOURNAL_FLAG_DELETE )
	{
		p = (struct party*) aCalloc( 1, sizeof( struct party ) );
		memcpy( p, buf, sizeof(struct party) );
		numdb_insert( party_db, key, p );
		if( p->party_id >= party_newid)
			party_newid=p->party_id+1;
		return 1;
	}
	
	return 0;
}
int party_txt_sync(void);
#endif

// パーティデータのロード
int party_txt_init(void)
{
	char line[8192];
	struct party *p;
	FILE *fp;
	int c=0;
	
	party_db=numdb_init();
	
	if( (fp=fopen(party_txt,"r"))==NULL )
		return 1;
	while(fgets(line,sizeof(line),fp)){
		int i,j=0;
		if( sscanf(line,"%d\t%%newid%%\n%n",&i,&j)==1 && j>0 && party_newid<=i){
			party_newid=i;
			continue;
		}
	
		p=(struct party *)aCalloc(1,sizeof(struct party));
		if(party_fromstr(line,p)==0 && p->party_id>0){
			if( p->party_id >= party_newid)
				party_newid=p->party_id+1;
			if(party_check_empty(p)) {
				// 空パーティ
				free(p);
			} else {
				numdb_insert(party_db,p->party_id,p);
			}
		}
		else{
			printf("int_party: broken data [%s] line %d\n",party_txt,c+1);
			free(p);
		}
		c++;
	}
	fclose(fp);
//	printf("int_party: %s read done (%d parties)\n",party_txt,c);

#ifdef TXT_JOURNAL
	if( party_journal_enable )
	{
		// ジャーナルデータのロールフォワード
		if( journal_load( &party_journal, sizeof(struct party), party_journal_file ) )
		{
			int c = journal_rollforward( &party_journal, party_journal_rollforward );
			
			printf("int_party: journal: roll-forward (%d)\n", c );
			
			// ロールフォワードしたので、txt データを保存する ( journal も新規作成される)
			party_txt_sync();
		}
		else
		{
			// ジャーナルを新規作成する
			journal_final( &party_journal );
			journal_create( &party_journal, sizeof(struct party), party_journal_cache, party_journal_file );
		}
	}
#endif

	return 0;
}

// パーティーデータのセーブ用
int party_txt_sync_sub(void *key,void *data,va_list ap)
{
	char line[8192];
	FILE *fp;
	party_tostr(line,(struct party *)data);
	fp=va_arg(ap,FILE *);
	fprintf(fp,"%s" RETCODE,line);
	return 0;
}

// パーティーデータのセーブ
int party_txt_sync(void)
{
	FILE *fp;
	int  lock;
	
	if( !party_db )
		return 1;
	
	if( (fp=lock_fopen(party_txt,&lock))==NULL ){
		printf("int_party: cant write [%s] !!! data is lost !!!\n",party_txt);
		return 1;
	}
	numdb_foreach(party_db,party_txt_sync_sub,fp);
//	fprintf(fp,"%d\t%%newid%%\n",party_newid);
	lock_fclose(fp,party_txt,&lock);
//	printf("int_party: %s saved.\n",party_txt);

#ifdef TXT_JOURNAL
	if( party_journal_enable )
	{
		// コミットしたのでジャーナルを新規作成する
		journal_final( &party_journal );
		journal_create( &party_journal, sizeof(struct party), party_journal_cache, party_journal_file );
	}
#endif

	return 0;
}

// パーティ名検索用
int party_txt_load_name_sub(void *key,void *data,va_list ap)
{
	struct party *p=(struct party *)data,**dst;
	char *str;
	str=va_arg(ap,char *);
	dst=va_arg(ap,struct party **);
	if(strcmpi(p->name,str)==0)
		*dst=p;
	return 0;
}

// パーティ名検索
const struct party* party_txt_load_str(char *str)
{
	struct party *p=NULL;
	numdb_foreach(party_db,party_txt_load_name_sub,str,&p);
	return p;
}

const struct party* party_txt_load_num(int party_id)
{
	return numdb_search(party_db,party_id);
}

int party_txt_save(struct party* p2) {
	struct party *p1 = numdb_search(party_db,p2->party_id);
	if(p1 == NULL) {
		p1 = aMalloc(sizeof(struct party));
		numdb_insert(party_db,p2->party_id,p1);
	}
	memcpy(p1,p2,sizeof(struct party));
#ifdef TXT_JOURNAL
	if( party_journal_enable )
		journal_write( &party_journal, p1->party_id, p1 );
#endif
	return 1;
}

int party_txt_delete(int party_id) {
	struct party *p = numdb_search(party_db,party_id);
	if(p) {
		numdb_erase(party_db,p->party_id);
		free(p);
#ifdef TXT_JOURNAL
		if( party_journal_enable )
			journal_write( &party_journal, party_id, NULL );
#endif
		return 1;
	}
	return 0;
}

int party_txt_config_read_sub(const char *w1,const char *w2) {
	if(strcmpi(w1,"party_txt")==0){
		strncpy(party_txt,w2,sizeof(party_txt));
	}
#ifdef TXT_JOURNAL
	else if(strcmpi(w1,"party_journal_enable")==0){
		party_journal_enable = atoi( w2 );
	}
	else if(strcmpi(w1,"party_journal_file")==0){
		strncpy( party_journal_file, w2, sizeof(party_journal_file) );
	}
	else if(strcmpi(w1,"party_journal_cache_interval")==0){
		party_journal_cache = atoi( w2 );
	}
#endif
	return 0;
}

int party_txt_new(struct party *p) {
	p->party_id = party_newid++;
	numdb_insert(party_db,p->party_id,p);
#ifdef TXT_JOURNAL
	if( party_journal_enable )
		journal_write( &party_journal, p->party_id, p );
#endif
	return 1;
}

static int party_txt_final_sub(void *key,void *data,va_list ap)
{
	struct party *p=data;

	free(p);

	return 0;
}

void party_txt_final(void)
{
	if(party_db)
		numdb_final(party_db,party_txt_final_sub);
		
#ifdef TXT_JOURNAL
	if( party_journal_enable )
	{
		journal_final( &party_journal );
	}
#endif
}

#define party_new      party_txt_new
#define party_init     party_txt_init
#define party_sync     party_txt_sync
#define party_save     party_txt_save
#define party_delete   party_txt_delete
#define party_load_str party_txt_load_str
#define party_load_num party_txt_load_num
#define party_config_read_sub party_txt_config_read_sub

#else /* TXT_ONLY */

static struct dbt *party_db;
static char party_db_[256] = "party";

int party_sql_init(void) {
	party_db = numdb_init();
	return 0;
}

int party_sql_sync(void) {
	// nothing to do
	return 0;
}

const struct party* party_sql_load_str(char *str) {
	int  id_num = -1;
	char buf[256];
	MYSQL_RES* sql_res;
	MYSQL_ROW  sql_row = NULL;

	sprintf(
		tmp_sql,"SELECT `party_id` FROM `%s` WHERE `name` = '%s'",
		party_db_,strecpy(buf,str)
	);
	if (mysql_query(&mysql_handle, tmp_sql)) {
		printf("DB server Error - %s\n", mysql_error(&mysql_handle));
	}
	sql_res = mysql_store_result(&mysql_handle) ;
	if (sql_res) {
		sql_row = mysql_fetch_row(sql_res);
		if(sql_row) {
			id_num  = atoi(sql_row[0]);
		}
	}
	mysql_free_result(sql_res);
	if(id_num >= 0) {
		return party_sql_load_num(id_num);
	} else {
		return NULL;
	}
	return NULL;
}

const struct party* party_sql_load_num(int party_id) {
	int leader_id=0;
	struct party *p = numdb_search(party_db,party_id);
	MYSQL_RES* sql_res;
	MYSQL_ROW  sql_row = NULL;

	if(p && p->party_id == party_id) {
		return p;
	}
	if(p == NULL) {
		p = aMalloc(sizeof(struct party));
		numdb_insert(party_db,party_id,p);
	}
	memset(p, 0, sizeof(struct party));
	// printf("Request load party(%06d)[",party_id);
	sprintf(
		tmp_sql,
		"SELECT `party_id`, `name`,`exp`,`item`, `leader_id` FROM `%s` WHERE `party_id`='%d'",
		party_db_, party_id
	);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		printf("DB server Error (select `party`)- %s\n", mysql_error(&mysql_handle) );
		p->party_id = -1;
		return NULL;
	}

	sql_res = mysql_store_result(&mysql_handle) ;
	if (sql_res!=NULL && mysql_num_rows(sql_res)>0) {
		sql_row     = mysql_fetch_row(sql_res);
		p->party_id = party_id;
		strcpy(p->name, sql_row[1]);
		p->exp      = atoi(sql_row[2]);
		p->item     = atoi(sql_row[3]);
		leader_id   = atoi(sql_row[4]);
	} else {
		mysql_free_result(sql_res);
		p->party_id = -1;
		return NULL;
	}

	mysql_free_result(sql_res);

	// Load members
	sprintf(
		tmp_sql,
		"SELECT `account_id`,`name`,`base_level`,`last_map` FROM `%s` WHERE `party_id`='%d'",
		char_db, party_id
	);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		printf("DB server Error (select `party`)- %s\n", mysql_error(&mysql_handle) );
		p->party_id = -1;
		return NULL;
	}
	sql_res = mysql_store_result(&mysql_handle) ;
	if (sql_res) {
		int i;
		for(i=0;(sql_row = mysql_fetch_row(sql_res));i++){
			struct party_member *m = &p->member[i];
			m->account_id = atoi(sql_row[0]);
			m->leader     = (m->account_id == leader_id) ? 1 : 0;
			strncpy(m->name,sql_row[1],sizeof(m->name));
			m->lv         = atoi(sql_row[2]);
			strncpy(m->map,sql_row[3],sizeof(m->map));
		}
	}
	mysql_free_result(sql_res);
	// printf("]\n");
	return p;
}

int party_sql_save(struct party* p2) {
	// 'party' ('party_id','name','exp','item','leader')
	const struct party *p1 = party_sql_load_num(p2->party_id);
	char t_name[64];
	// printf("Request save party(%06d)[",p2->party_id);

	if(p1 == NULL) return 0;

	if(strcmp(p1->name,p2->name) || p1->exp != p2->exp || p1->item != p2->item) {
		sprintf(
			tmp_sql,
			"UPDATE `%s` SET `name`='%s', `exp`='%d', `item`='%d' WHERE `party_id`='%d'",
			party_db_,strecpy(t_name,p2->name),p2->exp,p2->item,p2->party_id
		);
		if(mysql_query(&mysql_handle, tmp_sql) ) {
			printf("DB server Error (update `party`)- %s\n", mysql_error(&mysql_handle) );
		}
	}
	// printf("]\n");
	{
		struct party *p3 = numdb_search(party_db,p2->party_id);
		if(p3)
			memcpy(p3,p2,sizeof(struct party));
	}
	return 0;
}

int party_sql_delete(int party_id) {
	// Delete the party
	struct party *p = numdb_search(party_db,party_id);
	if(p) {
		numdb_erase(party_db,p->party_id);
		aFree(p);
	}
	// printf("Request del party(%06d)[",party_id);
	sprintf(tmp_sql,"DELETE FROM `%s` WHERE `party_id`='%d'",party_db_, party_id);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		printf("DB server Error - %s\n", mysql_error(&mysql_handle) );
	}
	// printf("]\n");
	return 0;
}

int party_sql_config_read_sub(const char *w1,const char *w2) {
	// nothing to do
	return 0;
}

int party_sql_new(struct party *p) {
	// Add new party
	int i = 0;
	int leader_id = -1;
	char t_name[64];
	MYSQL_RES* sql_res;
	MYSQL_ROW  sql_row = NULL;
	// printf("Request make party(------)[");

	for(i = 0; i < MAX_PARTY; i++) {
		if(p->member[i].account_id > 0 && p->member[i].leader) {
			leader_id = p->member[i].account_id;
			break;
		}
	}
	if ( leader_id == -1 ) { return 0; }
	// パーティIDを読み出す
	sprintf(
		tmp_sql,
		"SELECT MAX(`party_id`) FROM `%s`",
		party_db_
	);
	if(mysql_query(&mysql_handle, tmp_sql)){
		printf("failed (get party_id), SQL error: %s\n", mysql_error(&mysql_handle));
		return 0;
	} else {
		//query ok -> get the data!
		sql_res = mysql_store_result(&mysql_handle);
		if(!sql_res){
			printf("failed (get party_id), SQL error: %s\n", mysql_error(&mysql_handle));
			return 0;
		}
		sql_row = mysql_fetch_row(sql_res);
		if(sql_row[0]) {
			p->party_id = atoi(sql_row[0]) + 1;
		} else {
			p->party_id = 100;
		}
		mysql_free_result(sql_res);
	}

	// DBに挿入
	sprintf(
		tmp_sql,
		"INSERT INTO `%s`  (`party_id`, `name`, `exp`, `item`, `leader_id`) "
		"VALUES ('%d','%s', '%d', '%d', '%d')",
		party_db_,p->party_id,strecpy(t_name,p->name), p->exp, p->item,leader_id
	);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		printf("DB server Error (inset `party`)- %s\n", mysql_error(&mysql_handle) );
		return 0;
	}
	// printf("]\n");
	numdb_insert(party_db,p->party_id,p);
	return 1;
}

static int party_sql_final_sub(void *key,void *data,va_list ap)
{
	struct party *p=data;

	free(p);

	return 0;
}

void party_sql_final(void)
{
	numdb_final(party_db,party_sql_final_sub);
}

#define party_new      party_sql_new
#define party_init     party_sql_init
#define party_sync     party_sql_sync
#define party_save     party_sql_save
#define party_delete   party_sql_delete
#define party_load_str party_sql_load_str
#define party_load_num party_sql_load_num
#define party_config_read_sub party_sql_config_read_sub

#endif

// EXP公平分配できるかチェック
int party_check_exp_share(struct party *p)
{
	int i;
	int maxlv=0,minlv=0x7fffffff;
	for(i=0;i<MAX_PARTY;i++){
		int lv=p->member[i].lv;
		if( p->member[i].online ){
			if( lv < minlv ) minlv=lv;
			if( maxlv < lv ) maxlv=lv;
		}
	}
	return (maxlv==0 || maxlv-minlv<=party_share_level);
}

// パーティが空かどうかチェック
int party_check_empty(const struct party *p)
{
	int i;
//	printf("party check empty %08X\n",(int)p);
	for(i=0;i<MAX_PARTY;i++){
		if(p->member[i].account_id>0){
			return 0;
		}
	}
	return 1;
}

//-------------------------------------------------------------------
// map serverへの通信

// パーティ作成可否
int mapif_party_created(int fd,int account_id,struct party *p)
{
	WFIFOW(fd,0)=0x3820;
	WFIFOL(fd,2)=account_id;
	if(p!=NULL){
		WFIFOB(fd,6)=0;
		WFIFOL(fd,7)=p->party_id;
		memcpy(WFIFOP(fd,11),p->name,24);
		printf("int_party: created! %d %s\n",p->party_id,p->name);
	} else {
		WFIFOB(fd,6)=1;
		WFIFOL(fd,7)=0;
		memcpy(WFIFOP(fd,11),"error",24);
	}
	WFIFOSET(fd,35);
	return 0;
}

// パーティ情報見つからず
int mapif_party_noinfo(int fd,int party_id)
{
	WFIFOW(fd,0)=0x3821;
	WFIFOW(fd,2)=8;
	WFIFOL(fd,4)=party_id;
	WFIFOSET(fd,8);
	printf("int_party: info not found %d\n",party_id);
	return 0;
}

// パーティ情報まとめ送り
int mapif_party_info(int fd,const struct party *p)
{
	unsigned char buf[1024];
	WBUFW(buf,0)=0x3821;
	memcpy(buf+4,p,sizeof(struct party));
	WBUFW(buf,2)=4+sizeof(struct party);
	if(fd<0)
		mapif_sendall(buf,WBUFW(buf,2));
	else
		mapif_send(fd,buf,WBUFW(buf,2));
//	printf("int_party: info %d %s\n",p->party_id,p->name);
	return 0;
}

// パーティメンバ追加可否
void mapif_party_memberadded(int fd, int party_id, int account_id, const char * name, unsigned char flag)
{
	WFIFOW(fd,0)=0x3822;
	WFIFOL(fd,2)=party_id;
	WFIFOL(fd,6)=account_id;
	WFIFOB(fd,10)=flag;
	strncpy(WFIFOP(fd,11), name, 24);
	WFIFOSET(fd,35);

	return;
}

// パーティ設定変更通知
int mapif_party_optionchanged(int fd,struct party *p,int account_id,int flag)
{
	unsigned char buf[16];
	WBUFW(buf,0)=0x3823;
	WBUFL(buf,2)=p->party_id;
	WBUFL(buf,6)=account_id;
	WBUFW(buf,10)=p->exp;
	WBUFW(buf,12)=p->item;
	WBUFB(buf,14)=flag;
	if(flag==0)
		mapif_sendall(buf,15);
	else
		mapif_send(fd,buf,15);
	printf("int_party: option changed %d %d %d %d %d\n",p->party_id,account_id,p->exp,p->item,flag);
	return 0;
}

// パーティ脱退通知
int mapif_party_leaved(int party_id,int account_id,char *name)
{
	unsigned char buf[64];
	WBUFW(buf,0)=0x3824;
	WBUFL(buf,2)=party_id;
	WBUFL(buf,6)=account_id;
	memcpy(WBUFP(buf,10),name,24);
	mapif_sendall(buf,34);
	printf("int_party: party leaved %d %d %s\n",party_id,account_id,name);
	return 0;
}

// パーティマップ更新通知
static void mapif_party_membermoved(struct party *p,int idx)
{
	unsigned char buf[53];

	WBUFW(buf,0)=0x3825;
	WBUFL(buf,2)=p->party_id;
	WBUFL(buf,6)=p->member[idx].account_id;
	memcpy(WBUFP(buf,10),p->member[idx].map,16);
	WBUFB(buf,26)=p->member[idx].online;
	WBUFW(buf,27)=p->member[idx].lv;
	memcpy(WBUFP(buf,29),p->member[idx].name,24);
	mapif_sendall(buf,53);

	return;
}

// パーティ解散通知
int mapif_party_broken(int party_id,int flag)
{
	unsigned char buf[16];
	WBUFW(buf,0)=0x3826;
	WBUFL(buf,2)=party_id;
	WBUFB(buf,6)=flag;
	mapif_sendall(buf,7);
	printf("int_party: broken %d\n",party_id);
	return 0;
}

// パーティ内発言
int mapif_party_message(int party_id,int account_id,char *mes,int len)
{
	unsigned char buf[512];
	WBUFW(buf,0)=0x3827;
	WBUFW(buf,2)=len+12;
	WBUFL(buf,4)=party_id;
	WBUFL(buf,8)=account_id;
	memcpy(WBUFP(buf,12),mes,len);
	mapif_sendall(buf,len+12);
	return 0;
}

//-------------------------------------------------------------------
// map serverからの通信


// パーティ
int mapif_parse_CreateParty(int fd,int account_id,char *name,char *nick,char *map,int lv)
{
	struct party *p;
	int i;
	
	for(i=0;i<24 && name[i];i++){
		if( !(name[i]&0xe0) || name[i]==0x7f){
			printf("int_party: illeagal party name [%s]\n",name);
			mapif_party_created(fd,account_id,NULL);
			return 0;
		}
	}

	if( party_load_str(name) != NULL){
		printf("int_party: same name party exists [%s]\n",name);
		mapif_party_created(fd,account_id,NULL);
		return 0;
	}
	p=(struct party *)aCalloc(1,sizeof(struct party));
	memcpy(p->name,name,24);
	p->exp=0;
	p->item=0;
	p->member[0].account_id=account_id;
	memcpy(p->member[0].name,nick,24);
	memcpy(p->member[0].map,map,16);
	p->member[0].leader=1;
	p->member[0].online=1;
	p->member[0].lv=lv;

	if(party_new(p)) {
		// 成功
		mapif_party_created(fd,account_id,p);
		mapif_party_info(fd,p);
	} else {
		// 失敗
		mapif_party_created(fd,account_id,NULL);
	}
	
	return 0;
}

// パーティ情報要求
int mapif_parse_PartyInfo(int fd,int party_id)
{
	const struct party *p = party_load_num(party_id);
	if(p!=NULL)
		mapif_party_info(fd,p);
	else
		mapif_party_noinfo(fd,party_id);
	return 0;
}

// パーティ追加要求
int mapif_parse_PartyAddMember(int fd,int party_id,int account_id,char *nick,char *map,int lv)
{
	const struct party *p1 = party_load_num(party_id);
	struct party p2;
	int i;
	if(p1 == NULL){
		mapif_party_memberadded(fd, party_id, account_id, nick, 1);
		return 0;
	}
	memcpy(&p2,p1,sizeof(struct party));
	
	for(i=0;i<MAX_PARTY;i++){
		if (p2.member[i].account_id==account_id &&
		    strncmp(p2.member[i].name, nick, 24) == 0)
			break;
		if(p2.member[i].account_id==0){
			int flag=0;
			
			p2.member[i].account_id=account_id;
			memcpy(p2.member[i].name,nick,24);
			memcpy(p2.member[i].map,map,16);
			p2.member[i].leader=0;
			p2.member[i].online=1;
			p2.member[i].lv=lv;
			mapif_party_memberadded(fd, party_id, account_id, nick, 0);
			mapif_party_info(-1,&p2);

			if( p2.exp>0 && !party_check_exp_share(&p2) ){
				p2.exp=0;
				flag=0x01;
			}
			if(flag)
				mapif_party_optionchanged(fd,&p2,0,0);
			party_save(&p2);
			return 0;
		}
	}
	mapif_party_memberadded(fd, party_id, account_id, nick, 1);
	party_save(&p2);
	return 0;
}

// パーティー設定変更要求
int mapif_parse_PartyChangeOption(int fd,int party_id,int account_id,int exp,int item)
{
	const struct party *p1 = party_load_num(party_id);
	struct party p2;
	int flag=0;
	if(p1 == NULL){
		return 0;
	}
	memcpy(&p2,p1,sizeof(struct party));
	
	p2.exp = exp;
	if( exp>0 && !party_check_exp_share(&p2) ){
		flag|=0x01;
		p2.exp=0;
	}
	
	p2.item=item;

	mapif_party_optionchanged(fd,&p2,account_id,flag);
	party_save(&p2);
	return 0;
}

// パーティ脱退要求
void mapif_parse_PartyLeave(int fd, int party_id, int account_id, const char * name)
{
	const struct party *p1 = party_load_num(party_id);
	if(p1 != NULL){
		int i;
		struct party p2;
		memcpy(&p2,p1,sizeof(struct party));
		for(i=0;i<MAX_PARTY;i++){
			if (p2.member[i].account_id == account_id &&
			    strncmp(p2.member[i].name, name, 24) == 0) {
				mapif_party_leaved(party_id,account_id,p2.member[i].name);
				memset(&p2.member[i],0,sizeof(struct party_member));
				if( party_check_empty(&p2) ) {
					// 空になったので解散
					mapif_party_broken(p2.party_id,0);
					party_delete(p2.party_id);
				} else {
					// まだ人がいるのでデータ送信
					mapif_party_info(-1,&p2);
					party_save(&p2);
				}
				return;
			}
		}
	}

	return;
}

// パーティマップ更新要求
static void mapif_parse_PartyChangeMap(int fd, int party_id, int account_id, char *map, int online, int lv, const char* name)
{
	const struct party *p1 = party_load_num(party_id);
	struct party p2;
	int i;

	if(p1 == NULL) {
		return;
	}

	memcpy(&p2,p1,sizeof(struct party));
	for(i=0;i<MAX_PARTY;i++){
		if (p2.member[i].account_id == account_id &&
		    strncmp(p2.member[i].name, name, 24) == 0) {
			int flag=0;

			memcpy(p2.member[i].map,map,16);
			p2.member[i].online=online;
			p2.member[i].lv=lv;
			mapif_party_membermoved(&p2,i);

			if( p2.exp>0 && !party_check_exp_share(&p2) ){
				p2.exp=0;
				flag=1;
			}
			if(flag)
				mapif_party_optionchanged(fd,&p2,0,0);
			break;
		}
	}
	party_save(&p2);

	return;
}

// パーティ解散要求
int mapif_parse_BreakParty(int fd,int party_id)
{
	const struct party *p = party_load_num(party_id);
	if(p==NULL){
		return 0;
	}
	party_delete(party_id);
	mapif_party_broken(fd,party_id);
	return 0;
}

// パーティメッセージ送信
int mapif_parse_PartyMessage(int fd,int party_id,int account_id,char *mes,int len)
{
	return mapif_party_message(party_id,account_id,mes,len);
}

// パーティチェック要求
int mapif_parse_PartyCheck(int fd,int party_id,int account_id,char *nick)
{
	// とりあえず無視
	return 0;
}

// map server からの通信
// ・１パケットのみ解析すること
// ・パケット長データはinter.cにセットしておくこと
// ・パケット長チェックや、RFIFOSKIPは呼び出し元で行われるので行ってはならない
// ・エラーなら0(false)、そうでないなら1(true)をかえさなければならない
int inter_party_parse_frommap(int fd)
{
	switch(RFIFOW(fd,0)){
	case 0x3020: mapif_parse_CreateParty(fd,RFIFOL(fd,2),RFIFOP(fd,6),RFIFOP(fd,30),RFIFOP(fd,54),RFIFOW(fd,70)); break;
	case 0x3021: mapif_parse_PartyInfo(fd,RFIFOL(fd,2)); break;
	case 0x3022: mapif_parse_PartyAddMember(fd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOP(fd,10),RFIFOP(fd,34),RFIFOW(fd,50)); break;
	case 0x3023: mapif_parse_PartyChangeOption(fd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOW(fd,10),RFIFOW(fd,12)); break;
	case 0x3024: mapif_parse_PartyLeave(fd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOP(fd,10)); break;
	case 0x3025: mapif_parse_PartyChangeMap(fd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOP(fd,10),RFIFOB(fd,26),RFIFOW(fd,27),RFIFOP(fd,29)); break;
	case 0x3026: mapif_parse_BreakParty(fd,RFIFOL(fd,2)); break;
	case 0x3027: mapif_parse_PartyMessage(fd,RFIFOL(fd,4),RFIFOL(fd,8),RFIFOP(fd,12),RFIFOW(fd,2)-12); break;
	case 0x3028: mapif_parse_PartyCheck(fd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOP(fd,10)); break;
	default:
		return 0;
	}
	return 1;
}

// サーバーから脱退要求（キャラ削除用）
void inter_party_leave(int party_id, int account_id, const char * name)
{
	mapif_parse_PartyLeave(-1, party_id, account_id, name);

	return;
}

// パーティー設定読み込み
void party_config_read(const char *w1,const char* w2) 
{
	if(strcmpi(w1,"party_share_level")==0)
	{
		party_share_level=atoi(w2);
		if(party_share_level < 0) party_share_level = 0;
	}
	else
	{
		party_config_read_sub(w1,w2);
	}
}
