// $Id: itemdb.c,v 1.3 2005/09/08 00:29:53 running_pinata Exp $
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "grfio.h"
#include "nullpo.h"
#include "malloc.h"
#include "map.h"
#include "battle.h"
#include "itemdb.h"
#include "script.h"
#include "pc.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define MAX_RANDITEM	2000

// ** ITEMDB_OVERRIDE_NAME_VERBOSE **
//   ��`����ƁAitemdb.txt��grf�Ŗ��O���قȂ�ꍇ�A�\�����܂�.
//#define ITEMDB_OVERRIDE_NAME_VERBOSE	1

static struct dbt* item_db;

static struct random_item_data blue_box[MAX_RANDITEM],violet_box[MAX_RANDITEM],card_album[MAX_RANDITEM],
				gift_box[MAX_RANDITEM],scroll[MAX_RANDITEM], finding_ore[MAX_RANDITEM],arrow_quiver[MAX_RANDITEM],
				diamond_weapon[MAX_RANDITEM],diamond_armor[MAX_RANDITEM],diamond_hood[MAX_RANDITEM],diamond_helm[MAX_RANDITEM],diamond_shoes[MAX_RANDITEM],diamond_shield[MAX_RANDITEM];
static int blue_box_count,violet_box_count,card_album_count,gift_box_count,scroll_count,finding_ore_count,arrow_quiver_count,
				diamond_weapon_count,diamond_armor_count,diamond_hood_count,diamond_helm_count,diamond_shoes_count,diamond_shield_count;
static int blue_box_default=0,violet_box_default=0,card_album_default=0,gift_box_default=0,scroll_default=0, finding_ore_default = 0,arrow_quiver_default = 0,
				diamond_weapon_default=0,diamond_armor_default=0,diamond_hood_default=0,diamond_helm_default=0,diamond_shoes_default=0,diamond_shield_default=0;

static int itemdb_readdb(void);
static int itemdb_read_randomitem(void);
static int itemdb_read_itemavail(void);
static int itemdb_read_itemnametable(void);
static int itemdb_read_noequip(void);
void itemdb_reload(void);

/*==========================================
 * ���O�Ō����p
 *------------------------------------------
 */
int itemdb_searchname_sub(void *key,void *data,va_list ap)
{
	struct item_data *item=(struct item_data *)data,**dst;
	char *str;
	str=va_arg(ap,char *);
	dst=va_arg(ap,struct item_data **);
	if( strcmpi(item->name,str)==0 || strcmp(item->jname,str)==0 ||
		memcmp(item->name,str,24)==0 || memcmp(item->jname,str,24)==0 )
		*dst=item;
	return 0;
}
/*==========================================
 * ���O�Ō���
 *------------------------------------------
 */
struct item_data* itemdb_searchname(const char *str)
{
	struct item_data *item=NULL;
	numdb_foreach(item_db,itemdb_searchname_sub,str,&item);
	return item;
}

/*==========================================
 * ���n�A�C�e������
 *------------------------------------------
 */
int itemdb_searchrandomid(int flags)
{
	int nameid=0,i,index,count;
	struct random_item_data *list=NULL;

	struct {
		int nameid,count;
		struct random_item_data *list;
	} data[14];

	// for BCC32 compile error
	data[0].nameid = 0;							data[0].count = 0; 						data[0].list = NULL;
	data[1].nameid = blue_box_default;			data[1].count = blue_box_count;			data[1].list = blue_box;
	data[2].nameid = violet_box_default;		data[2].count = violet_box_count;		data[2].list = violet_box;
	data[3].nameid = card_album_default;		data[3].count = card_album_count;		data[3].list = card_album;
	data[4].nameid = gift_box_default;			data[4].count = gift_box_count;			data[4].list = gift_box;
	data[5].nameid = scroll_default;			data[5].count = scroll_count;			data[5].list = scroll;
	data[6].nameid = finding_ore_default;		data[6].count = finding_ore_count;		data[6].list = finding_ore;
	data[7].nameid = arrow_quiver_default;		data[7].count = arrow_quiver_count;		data[7].list = arrow_quiver;
	data[8].nameid = diamond_weapon_default;	data[8].count = diamond_weapon_count;	data[8].list = diamond_weapon;
	data[9].nameid = diamond_armor_default;		data[9].count = diamond_armor_count;	data[9].list = diamond_armor;
	data[10].nameid = diamond_hood_default;		data[10].count = diamond_hood_count;	data[10].list = diamond_hood;
	data[11].nameid = diamond_helm_default;		data[11].count = diamond_helm_count;	data[11].list = diamond_helm;
	data[12].nameid = diamond_shoes_default;	data[12].count = diamond_shoes_count;	data[12].list = diamond_shoes;
	data[13].nameid = diamond_shield_default;	data[13].count = diamond_shield_count;	data[13].list = diamond_shield;

	if(flags>=1 && flags<=13){
		nameid=data[flags].nameid;
		count=data[flags].count;
		list=data[flags].list;

		if(count > 0) {
			for(i=0;i<1000;i++) {
				index = atn_rand()%count;
				if(	atn_rand()%1000000 < list[index].per) {
					nameid = list[index].nameid;
					break;
				}
			}
		}
	}
	return nameid;
}

/*==========================================
 * DB�̑��݊m�F
 *------------------------------------------
 */
struct item_data* itemdb_exists(int nameid)
{
	return numdb_search(item_db,nameid);
}
/*==========================================
 * DB�̌���
 *------------------------------------------
 */
struct item_data* itemdb_search(int nameid)
{
	struct item_data *id;

	id=numdb_search(item_db,nameid);
	if(id) return id;

	id=(struct item_data *)aCalloc(1,sizeof(struct item_data));
	numdb_insert(item_db,nameid,id);

	id->nameid=nameid;
	id->value_buy=10;
	id->value_sell=id->value_buy/2;
	id->weight=10;
	id->sex=2;
	id->elv=0;
	id->class=0xffffffff;
	id->flag.available=0;
	id->flag.value_notdc=0;  //�ꉞ�E�E�E
	id->flag.value_notoc=0;
	id->flag.no_equip=0;
	id->view_id=0;

	if(nameid>500 && nameid<600)
		id->type=0;   //heal item
	else if(nameid>600 && nameid<700)
		id->type=2;   //use item
	else if((nameid>700 && nameid<1100) ||
			(nameid>7000 && nameid<8000))
		id->type=3;   //correction
	else if(nameid>=1750 && nameid<1771)
		id->type=10;  //arrow
	else if(nameid>1100 && nameid<2000)
		id->type=4;   //weapon
	else if((nameid>2100 && nameid<3000) ||
			(nameid>5000 && nameid<6000))
		id->type=5;   //armor
	else if(nameid>4000 && nameid<5000)
		id->type=6;   //card
	else if(nameid>9000 && nameid<10000)
		id->type=7;   //egg
	else if(nameid>10000)
		id->type=8;   //petequip

	return id;
}

/*==========================================
 *
 *------------------------------------------
 */
int itemdb_isequip(int nameid)
{
	int type=itemdb_type(nameid);
	if(type==0 || type==2 || type==3 || type==6 || type==10)
		return 0;
	return 1;
}
/*==========================================
 *
 *------------------------------------------
 */
int itemdb_isequip2(struct item_data *data)
{
	if(data) {
		int type=data->type;
		if(type==0 || type==2 || type==3 || type==6 || type==10)
			return 0;
		else
			return 1;
	}
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int itemdb_isequip3(int nameid)
{
	int type=itemdb_type(nameid);
	if(type==4 || type==5 || type == 8)
		return 1;
	return 0;
}

/*==========================================
 * �̂Ă���A�C�e����1�A�����łȂ��A�C�e����0
 *------------------------------------------
 */
int itemdb_isdropable(int nameid)
{
	//�����w�ւ͎̂Ă��Ȃ�
	switch(nameid){
	case 2634: //�����w��
	case 2635: //�����w��
		return 0;
	}

	return 1;
}

//
// ������
//
/*==========================================
 *
 *------------------------------------------
 */
static int itemdb_read_itemslottable(void)
{
	char *buf,*p;
	int s;

	buf=grfio_read("data\\itemslottable.txt");
	if(buf==NULL)
		return -1;
	s=grfio_size("data\\itemslottable.txt");
	buf[s]=0;
	for(p=buf;p-buf<s;){
		int nameid,equip;
		sscanf(p,"%d#%d#",&nameid,&equip);
		itemdb_search(nameid)->equip=equip;
		p=strchr(p,10);
		if(!p) break;
		p++;
		p=strchr(p,10);
		if(!p) break;
		p++;
	}
	free(buf);

	return 0;
}

/*==========================================
 * �A�C�e���f�[�^�x�[�X�̓ǂݍ���
 *------------------------------------------
 */
static int itemdb_readdb(void)
{
	FILE *fp;
	char line[1024];
	int ln=0,lines=0;
	int nameid,j;
	char *str[32],*p,*np;
	struct item_data *id;
	int i=0;
	char *filename[]={ "db/item_db.txt","db/item_db2.txt" };

	for(i=0;i<2;i++){

		fp=fopen(filename[i],"r");
		if(fp==NULL){
			if(i>0)
				continue;
			printf("can't read %s\n",filename[i]);
			exit(1);
		}

		lines=0;
		while(fgets(line,1020,fp)){
			lines++;
			if(line[0]=='/' && line[1]=='/')
				continue;
			memset(str,0,sizeof(str));
			for(j=0,np=p=line;j<18 && p;j++){
				str[j]=p;
				p=strchr(p,',');
				if(p){ *p++=0; np=p; }
			}
			if(str[0]==NULL)
				continue;

			nameid=atoi(str[0]);
			if(nameid<=0 || nameid>=20000)
				continue;
			ln++;

			//ID,Name,Jname,Type,Price,Sell,Weight,ATK,DEF,Range,Slot,Job,Gender,Loc,wLV,eLV,View,Refine
			id=itemdb_search(nameid);
			memcpy(id->name,str[1],24);
			memcpy(id->jname,str[2],24);
			id->type=atoi(str[3]);
			// buy��sell*2 �� item_value_db.txt �Ŏw�肵�Ă��������B
			if (atoi(str[5])) {		// sell�l��D��Ƃ���
				id->value_buy=atoi(str[5])*2;
				id->value_sell=atoi(str[5]);
			} else {
				id->value_buy=atoi(str[4]);
				id->value_sell=atoi(str[4])/2;
			}
			id->weight=atoi(str[6]);
			id->atk=atoi(str[7]);
			id->def=atoi(str[8]);
			id->range=atoi(str[9]);
			id->slot=atoi(str[10]);
			id->class=atoi(str[11]);
			id->sex=atoi(str[12]);
			if(id->equip != atoi(str[13])){
				id->equip=atoi(str[13]);
			}
			id->wlv=atoi(str[14]);
			id->elv=atoi(str[15]);
			id->look=atoi(str[16]);
			id->refine=atoi(str[17]);
			id->flag.available=1;
			id->flag.value_notdc=0;
			id->flag.value_notoc=0;
			id->view_id=0;
			id->group=0;

			id->use_script=NULL;
			id->equip_script=NULL;

			if((p=strchr(np,'{'))==NULL)
				continue;
			id->use_script = parse_script(p,lines);
			if((p=strchr(p+1,'{'))==NULL)
				continue;
			id->equip_script = parse_script(p,lines);
		}
		fclose(fp);
		printf("read %s done (count=%d)\n",filename[i],ln);
	}

	fp=fopen("db/item_group_db.txt","r");
	if(fp==NULL){
		printf("can't read db/item_group_db.txt\n");
		return 0;
	}

	while(fgets(line,1020,fp)){
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(str,0,sizeof(str));
		for(j=0,np=p=line;j<18 && p;j++){
			str[j]=p;
			p=strchr(p,',');
			if(p){ *p++=0; np=p; }
		}
		if(str[0]==NULL)
			continue;

		nameid=atoi(str[0]);
		if(nameid<=0 || nameid>=20000)
			continue;
		//ID,Name,Jname,Group
		id=itemdb_search(nameid);
		id->group=atoi(str[3]);
	}
	fclose(fp);
	printf("read db/item_group_db.txt done\n");
	return 0;
}

/*==========================================
 * �A�C�e�����i�e�[�u���̃I�[�o�[���C�h
 *------------------------------------------
 */
static int itemdb_read_itemvaluedb(void)
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int nameid,j;
	char *str[32],*p;
	struct item_data *id;

	if( (fp=fopen("db/item_value_db.txt","r"))==NULL){
		printf("can't read db/item_value_db.txt\n");
		return -1;
	}

	while(fgets(line,1020,fp)){
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(str,0,sizeof(str));
		for(j=0,p=line;j<7 && p;j++){
			str[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(str[0]==NULL)
			continue;

		nameid=atoi(str[0]);
		if(nameid<=0 || nameid>=20000)
			continue;

		if( !(id=itemdb_exists(nameid)) )
			continue;

		ln++;
		// ���ꂼ��L�q�������̂݃I�[�o�[���C�g
		if(str[3]!=NULL && *str[3]){
			id->value_buy=atoi(str[3]);
		}
		if(str[4]!=NULL && *str[4]){
			id->value_sell=atoi(str[4]);
		}
		if(str[5]!=NULL && *str[5]){
			id->flag.value_notdc=(atoi(str[5])==0)? 0:1;
		}
		if(str[6]!=NULL && *str[6]){
			id->flag.value_notoc=(atoi(str[6])==0)? 0:1;
		}
	}
	fclose(fp);
	printf("read db/item_value_db.txt done (count=%d)\n",ln);
	return 0;
}

/*==========================================
 * �����_���A�C�e���o���f�[�^�̓ǂݍ���
 *------------------------------------------
 */
static int itemdb_read_randomitem(void)
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int nameid,i,j;
	char *str[10],*p;

	const struct {
		char filename[64];
		struct random_item_data *pdata;
		int *pcount,*pdefault;
	} data[] = {
		{"db/item_bluebox.txt",		blue_box,	&blue_box_count,	&blue_box_default	},
		{"db/item_violetbox.txt",	violet_box,	&violet_box_count,	&violet_box_default	},
		{"db/item_cardalbum.txt",	card_album,	&card_album_count,	&card_album_default	},
		{"db/item_giftbox.txt",		gift_box,	&gift_box_count,	&gift_box_default	},
		{"db/item_scroll.txt",		scroll,		&scroll_count,		&scroll_default		},
		{"db/item_findingore.txt",	finding_ore,&finding_ore_count,	&finding_ore_default},
		{"db/item_arrowquiver.txt",	arrow_quiver,&arrow_quiver_count,	&arrow_quiver_default},
		{"db/item_diamond_weapon.txt",diamond_weapon	,&diamond_weapon_count,	&diamond_weapon_default},
		{"db/item_diamond_armor.txt",diamond_armor	,&diamond_armor_count,	&diamond_armor_default},
		{"db/item_diamond_hood.txt",	diamond_hood,&diamond_hood_count,	&diamond_hood_default},
		{"db/item_diamond_helm.txt",diamond_helm	,&diamond_helm_count,	&diamond_helm_default},
		{"db/item_diamond_shoes.txt",diamond_shoes,&diamond_shoes_count,	&diamond_shoes_default},
		{"db/item_diamond_shield.txt",diamond_shield	,&diamond_shield_count,	&diamond_shield_default},
	};
	// �ǂݍ��ޓx�A������
	blue_box_count		= 0;
	violet_box_count	= 0;
	card_album_count	= 0;
	gift_box_count		= 0;
	scroll_count		= 0;
	finding_ore_count	= 0;
	arrow_quiver_count	= 0;
	diamond_weapon_count= 0;
	diamond_armor_count	= 0;
	diamond_helm_count	= 0;
	diamond_helm_count	= 0;
	diamond_shoes_count	= 0;
	diamond_shield_count= 0;

	for(i=0;i<sizeof(data)/sizeof(data[0]);i++){
		struct random_item_data *pd=data[i].pdata;
		int *pc=data[i].pcount;
		int *pdefault=data[i].pdefault;
		const char *fn=data[i].filename;

		*pdefault = 0;
		if( (fp=fopen(fn,"r"))==NULL ){
			printf("can't read %s\n",fn);
			continue;
		}

		while(fgets(line,1020,fp)){
			if(line[0]=='/' && line[1]=='/')
				continue;
			memset(str,0,sizeof(str));
			for(j=0,p=line;j<3 && p;j++){
				str[j]=p;
				p=strchr(p,',');
				if(p) *p++=0;
			}

			if(str[0]==NULL)
				continue;

			nameid=atoi(str[0]);
			if(nameid<0 || nameid>=20000)
				continue;
			if(nameid == 0) {
				if(str[2])
					*pdefault = atoi(str[2]);
				continue;
			}

			if(str[2]){
				pd[ *pc   ].nameid = nameid;
				pd[(*pc)++].per = atoi(str[2]);
			}

			if(ln >= MAX_RANDITEM)
				break;
			ln++;
		}
		fclose(fp);
		printf("read %s done (count=%d)\n",fn,*pc);
	}

	return 0;
}
/*==========================================
 * �A�C�e���g�p�\�t���O�̃I�[�o�[���C�h
 *------------------------------------------
 */
static int itemdb_read_itemavail(void)
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int nameid,j,k;
	char *str[10],*p;

	if( (fp=fopen("db/item_avail.txt","r"))==NULL ){
		printf("can't read db/item_avail.txt\n");
		return -1;
	}

	while(fgets(line,1020,fp)){
		struct item_data *id;
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(str,0,sizeof(str));
		for(j=0,p=line;j<2 && p;j++){
			str[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}

		if(str[0]==NULL)
			continue;

		nameid=atoi(str[0]);
		if(nameid<0 || nameid>=20000 || !(id=itemdb_exists(nameid)) )
			continue;
		k=atoi(str[1]);
		if(k > 0) {
			id->flag.available = 1;
			id->view_id = k;
		}
		else
			id->flag.available = 0;
		ln++;
	}
	fclose(fp);
	printf("read db/item_avail.txt done (count=%d)\n",ln);
	return 0;
}

/*==========================================
 * �A�C�e���̖��O�e�[�u����ǂݍ���
 *------------------------------------------
 */
static int itemdb_read_itemnametable(void)
{
	char *buf,*p;
	int s;

	buf=grfio_reads("data\\idnum2itemdisplaynametable.txt",&s);

	if(buf==NULL)
		return -1;

	buf[s]=0;
	for(p=buf;p-buf<s;){
		int nameid;
		char buf2[64];

		if(	sscanf(p,"%d#%[^#]#",&nameid,buf2)==2 ){

#ifdef ITEMDB_OVERRIDE_NAME_VERBOSE
			if( itemdb_exists(nameid) &&
				strncmp(itemdb_search(nameid)->jname,buf2,24)!=0 ){
				printf("[override] %d %s => %s\n",nameid
					,itemdb_search(nameid)->jname,buf2);
			}
#endif

			memcpy(itemdb_search(nameid)->jname,buf2,24);
		}

		p=strchr(p,10);
		if(!p) break;
		p++;
	}
	free(buf);
	printf("read data\\idnum2itemdisplaynametable.txt done.\n");

	return 0;
}
/*==========================================
 * �J�[�h�C���X�g�̃��\�[�X���O�e�[�u����ǂݍ���
 *------------------------------------------
 */
static int itemdb_read_cardillustnametable(void)
{
	char *buf,*p;
	int s;

	buf=grfio_reads("data\\num2cardillustnametable.txt",&s);

	if(buf==NULL)
		return -1;

	buf[s]=0;
	for(p=buf;p-buf<s;){
		int nameid;
		char buf2[64];

		if(	sscanf(p,"%d#%[^#]#",&nameid,buf2)==2 ){
			strcat(buf2,".bmp");
			memcpy(itemdb_search(nameid)->cardillustname,buf2,64);
//			printf("%d %s\n",nameid,itemdb_search(nameid)->cardillustname);
		}

		p=strchr(p,10);
		if(!p) break;
		p++;
	}
	free(buf);
	printf("read data\\num2cardillustnametable.txt done.\n");

	return 0;
}
/*==========================================
 * ���������t�@�C���ǂݏo��
 *------------------------------------------
 */
static int itemdb_read_noequip(void)
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int nameid,j;
	char *str[32],*p;
	struct item_data *id;

	if( (fp=fopen("db/item_noequip.txt","r"))==NULL ){
		printf("can't read db/item_noequip.txt\n");
		return -1;
	}
	while(fgets(line,1020,fp)){
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(str,0,sizeof(str));
		for(j=0,p=line;j<2 && p;j++){
			str[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(str[0]==NULL)
			continue;

		nameid=atoi(str[0]);
		if(nameid<=0 || nameid>=20000 || !(id=itemdb_exists(nameid)))
			continue;

		id->flag.no_equip=atoi(str[1]);

		ln++;

	}
	fclose(fp);
	printf("read db/item_noequip.txt done (count=%d)\n",ln);
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
static int itemdb_final(void *key,void *data,va_list ap)
{
	struct item_data *id;

	nullpo_retr(0, id=data);

	if(id->use_script)
		free(id->use_script);
	if(id->equip_script)
		free(id->equip_script);
	free(id);

	return 0;
}


/*==========================================
 *
 *------------------------------------------
 */
void do_final_itemdb(void)
{
	if(item_db){
		numdb_final(item_db,itemdb_final);
		item_db=NULL;
	}
}
/*==========================================
 *
 *------------------------------------------
 */
void itemdb_reload(void)
{
	/*

	<empty item databases>
	itemdb_read();

	*/
	do_final_itemdb();
	do_init_itemdb();
}

/*
static FILE *dfp;
static int itemdebug(void *key,void *data,va_list ap){
//	struct item_data *id=(struct item_data *)data;
	fprintf(dfp,"%6d",(int)key);
	return 0;
}
void itemdebugtxt()
{
	dfp=fopen("itemdebug.txt","wt");
	numdb_foreach(item_db,itemdebug);
	fclose(dfp);
}
*/

/*==========================================
 *
 *------------------------------------------
 */
int do_init_itemdb(void)
{
	item_db = numdb_init();

	itemdb_read_itemslottable();
	itemdb_readdb();
	itemdb_read_itemvaluedb();
	itemdb_read_randomitem();
	itemdb_read_itemavail();
	itemdb_read_noequip();
	if(battle_config.item_name_override_grffile)
		itemdb_read_itemnametable();
	itemdb_read_cardillustnametable();
	return 0;
}
