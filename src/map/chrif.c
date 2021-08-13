// $Id: chrif.c,v 1.6 2003/06/29 05:52:56 lemit Exp $
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

#include "socket.h"
#include "timer.h"
#include "malloc.h"
#include "nullpo.h"
#include "map.h"
#include "battle.h"
#include "chrif.h"
#include "clif.h"
#include "intif.h"
#include "npc.h"
#include "pc.h"
#include "atcommand.h"
#include "friend.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

static const int packet_len_table[0x30]={
	60, 3,-1, 3,14,-1, 7, 6,		// 2af8-2aff
	 6,-1,10, 7,-1,41,40, 0,		// 2b00-2b07
	 6,40,-1,10, 9, 7, 0, 0,		// 2b08-2b0f
	-1,-1, 6, 3, 3, 3,-1, 6,		// 2b10-2b17
	10, 6, 6, 0, 0, 0, 0, 0,		// 2b18-2b1f
	 6, 0, 0, 0,18,18,-1,-1,		// 2b20-2b27
};

int char_fd;
static char char_ip_str[16];
static int char_ip;
static int char_port = 6121;
static char userid[24],passwd[24];
static int chrif_state;

// �ݒ�t�@�C���ǂݍ��݊֌W
/*==========================================
 *
 *------------------------------------------
 */
void chrif_setuserid(char *id)
{
	memcpy(userid,id,24);
}

/*==========================================
 *
 *------------------------------------------
 */
void chrif_setpasswd(char *pwd)
{
	memcpy(passwd,pwd,24);
}

/*==========================================
 *
 *------------------------------------------
 */
void chrif_setip(char *ip)
{
	memcpy(char_ip_str,ip,16);
	char_ip=inet_addr(char_ip_str);
}

/*==========================================
 *
 *------------------------------------------
 */
void chrif_setport(int port)
{
	char_port=port;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_isconnect(void)
{
	return chrif_state==2;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_save(struct map_session_data *sd)
{
	nullpo_retr(-1, sd);

	if(char_fd<=0)
		return -1;

	pc_makesavestatus(sd);

	WFIFOW(char_fd,0)=0x2b01;
	WFIFOW(char_fd,2)=sizeof(sd->status)+12;
	WFIFOL(char_fd,4)=sd->bl.id;
	WFIFOL(char_fd,8)=sd->char_id;
	memcpy(WFIFOP(char_fd,12),&sd->status,sizeof(sd->status));
	WFIFOSET(char_fd,WFIFOW(char_fd,2));
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_connect(int fd)
{
	WFIFOW(fd,0)=0x2af8;
	memcpy(WFIFOP(fd,2),userid,24);
	memcpy(WFIFOP(fd,26),passwd,24);
	WFIFOL(fd,50)=0;
	WFIFOL(fd,54)=clif_getip();
	WFIFOW(fd,58)=clif_getport();
	WFIFOSET(fd,60);

	return 0;
}

/*==========================================
 * �}�b�v���M
 *------------------------------------------
 */
int chrif_sendmap(int fd)
{
	int i;

	WFIFOW(fd,0)=0x2afa;
	for(i=0;i<map_num;i++)
		memcpy(WFIFOP(fd,4+i*16),map[i].name,16);
	WFIFOW(fd,2)=4+i*16;
	WFIFOSET(fd,WFIFOW(fd,2));

	return 0;
}
/*==========================================
 * �}�b�v��M
 *------------------------------------------
 */
int chrif_recvmap(int fd)
{
	int i,j,ip,port;
	unsigned char *p=(unsigned char *)&ip;

	if(chrif_state<2)	// �܂�������
		return -1;

	ip=RFIFOL(fd,4);
	port=RFIFOW(fd,8);
	for(i=12,j=0;i<RFIFOW(fd,2);i+=16,j++){
		map_setipport(RFIFOP(fd,i),ip,port);
//		if(battle_config.etc_log)
//			printf("recv map %d %s\n",j,RFIFOP(fd,i));
	}
	if(battle_config.etc_log)
		printf("recv map on %d.%d.%d.%d:%d (%d maps)\n",p[0],p[1],p[2],p[3],port,j);

	return 0;
}
/*==========================================
 * �폜�}�b�v��M
 *------------------------------------------
 */
int chrif_recverasemap(int fd)
{
	int i,j,ip,port,ret = 0;
	unsigned char *p=(unsigned char *)&ip;

	if(chrif_state<2)	// �܂�������
		return -1;

	ip=RFIFOL(fd,4);
	port=RFIFOW(fd,8);
	for(i=12,j=0;i<RFIFOW(fd,2);i+=16,j++){
		ret |= map_eraseipport(RFIFOP(fd,i),ip,port);
//		if(battle_config.etc_log)
//			printf("recv map %d %s\n",j,RFIFOP(fd,i));
	}
	if(battle_config.etc_log)
		printf("recv erase map on %d.%d.%d.%d:%d (%d maps)\n",p[0],p[1],p[2],p[3],port,j);
	if(ret) {
		chrif_sendmap(fd);
	}
	return 0;
}
/*==========================================
 * �}�b�v�I�Ԉړ��̂��߂̃f�[�^�����v��
 *------------------------------------------
 */
int chrif_changemapserver(struct map_session_data *sd,char *name,int x,int y,int ip,short port)
{

	nullpo_retr(-1, sd);

	WFIFOW(char_fd,0)=0x2b05;
	WFIFOL(char_fd,2)=sd->bl.id;
	WFIFOL(char_fd,6)=sd->login_id1;
	WFIFOL(char_fd,10)=sd->status.char_id;
	memcpy(WFIFOP(char_fd,14),name,16);
	WFIFOW(char_fd,30)=x;
	WFIFOW(char_fd,32)=y;
	WFIFOL(char_fd,34)=ip;
	WFIFOW(char_fd,38)=port;
	WFIFOB(char_fd,40)=sd->sex;
	WFIFOSET(char_fd,41);
	return 0;
}
/*==========================================
 * �}�b�v�I�Ԉړ�ack
 *------------------------------------------
 */
int chrif_changemapserverack(int fd)
{
	struct map_session_data *sd=map_id2sd(RFIFOL(fd,2));
	if(sd==NULL || sd->status.char_id!=RFIFOL(fd,10) )
		return -1;
	if(RFIFOL(fd,6)==1){
		if(battle_config.error_log)
			printf("map server change failed.\n");
		pc_authfail(sd->fd);
		return 0;
	}
	clif_changemapserver(sd,RFIFOP(fd,14),RFIFOW(fd,30),RFIFOW(fd,32),
		RFIFOL(fd,34),RFIFOW(fd,38));
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int chrif_connectack(int fd)
{
	if(RFIFOB(fd,2)){
		printf("chrif : connect char server failed %d\n",RFIFOB(fd,2));
		exit(1);
	}
	chrif_state = 1;
	session[fd]->auth = -1; // �F�؏I���� socket.c �ɓ`����

	chrif_sendmap(fd);

	printf("chrif: OnCharIfInit event done. (%d events)\n",
		npc_event_doall("OnCharIfInit") );
	printf("chrif: OnInterIfInit event done. (%d events)\n",
		npc_event_doall("OnInterIfInit") );

	// <Agit> Run Event [AgitInit]
//	printf("NPC_Event:[OnAgitInit] do (%d) events (Agit Initialize).\n", npc_event_doall("OnAgitInit"));

	return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
int chrif_sendmapack(int fd)
{
	if(RFIFOB(fd,2)){
		printf("chrif : send map list to char server failed %d\n",RFIFOB(fd,2));
		exit(1);
	}
	chrif_state = 2;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_authreq(struct map_session_data *sd)
{
	nullpo_retr(-1, sd);

	if (sd->sex < 0 || sd->sex > 1) // invalid sex
		return -1;

	WFIFOW(char_fd, 0) = 0x2afc;
	WFIFOL(char_fd, 2) = sd->bl.id;
	WFIFOL(char_fd, 6) = sd->char_id;
	WFIFOL(char_fd,10) = sd->login_id1;
	WFIFOL(char_fd,14) = sd->login_id2;
	WFIFOL(char_fd,18) = session[sd->fd]->client_addr.sin_addr.s_addr;
	WFIFOB(char_fd,22) = sd->sex;
	WFIFOSET(char_fd,23);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_authok(int fd)
{
	struct map_session_data *sd=map_id2sd(RFIFOL(fd,4));
	if(sd)
		sd->login_id2=RFIFOL(fd,8);

	return pc_authok(RFIFOL(fd,4),(struct mmo_charstatus*)RFIFOP(fd,12));
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_charselectreq(struct map_session_data *sd)
{
	int i,ip=0;

	nullpo_retr(-1, sd);

	for(i=0;i<fd_max;i++){
		if(session[i] && session[i]->session_data==sd){
			ip=session[i]->client_addr.sin_addr.s_addr;
			break;
		}
	}

	WFIFOW(char_fd, 0) = 0x2b02;
	WFIFOL(char_fd, 2) = sd->bl.id;
	WFIFOL(char_fd, 6) = sd->login_id1;
	WFIFOL(char_fd,10) = sd->login_id2;
	WFIFOL(char_fd,14) = ip;
	WFIFOB(char_fd,18) = sd->sex;
	WFIFOSET(char_fd,19);
	return 0;
}

/*==========================================
 * �L�������₢���킹
 *------------------------------------------
 */
void chrif_searchcharid(int char_id)
{
	WFIFOW(char_fd,0)=0x2b08;
	WFIFOL(char_fd,2)=char_id;
	WFIFOSET(char_fd,6);

	return;
}

/*==========================================
 * GM�ɕω��v��
 *------------------------------------------
 */
int chrif_changegm(int id,const char *pass,int len)
{
	WFIFOW(char_fd,0)=0x2b0a;
	WFIFOW(char_fd,2)=len+8;
	WFIFOL(char_fd,4)=id;
	memcpy(WFIFOP(char_fd,8),pass,len);
//	if(battle_config.etc_log)
//		printf("chrif_changegm: %d %s %d\n",WFIFOL(char_fd,4),WFIFOP(char_fd,8),WFIFOW(char_fd,2));
	WFIFOSET(char_fd,len+8);
	return 0;
}
/*==========================================
 * ���ʕω��v��
 *------------------------------------------
 */
int chrif_changesex(int id,int sex)
{
	WFIFOW(char_fd,0)=0x2b0c;
	WFIFOW(char_fd,2)=9;
	WFIFOL(char_fd,4)=id;
	WFIFOB(char_fd,8)=sex;
	printf("chrif : sended 0x2b0c\n");
	WFIFOSET(char_fd,9);
	return 0;
}
/*==========================================
 * GM�ɕω��I��
 *------------------------------------------
 */
int chrif_changedgm(int fd)
{
	int oldacc,newacc;
	oldacc=RFIFOL(fd,2);
	newacc=RFIFOL(fd,6);
	if(battle_config.etc_log)
		printf("chrif_changedgm %d -> %d\n",oldacc,newacc);
	if(newacc>0){
		struct map_session_data *sd=map_id2sd(oldacc);
		if(sd!=NULL){	// GM�ύX�ɂ�鋭���ؒf
			clif_displaymessage(sd->fd,"GM�ύX�����B�Đڑ����ĉ������B");
			clif_setwaitclose(sd->fd);
		}
	}else{
		struct map_session_data *sd=map_id2sd(oldacc);
		if(sd!=NULL){
			clif_displaymessage(sd->fd,"GM�ύX���s");
		}
	}
	return 0;
}
/*==========================================
 * ���ʕω��I��
 *------------------------------------------
 */
int chrif_changedsex(int fd)
{
	int acc;
	struct map_session_data *sd;
	acc=RFIFOL(fd,2);
	if(battle_config.etc_log)
		printf("chrif_changedsex %d \n",acc);
	sd=map_id2sd(acc);
	if(acc>0){
		if(sd!=NULL){	// �ύX�ɂ�鋭���ؒf
			clif_setwaitclose(sd->fd);
		}
	}else{
		if(sd!=NULL){
			printf("chrif_changedsex failed\n");
		}
	}
	return 0;
}
/*==========================================
 * �A�J�E���g�ϐ��ۑ��v��
 *------------------------------------------
 */
int chrif_saveaccountreg2(struct map_session_data *sd)
{
	int p,j;
	nullpo_retr(-1, sd);

	for(p=8,j=0;j<sd->status.account_reg2_num;j++){
		struct global_reg *reg=&sd->status.account_reg2[j];
		if(reg->str[0] && reg->value!=0){
			memcpy(WFIFOP(char_fd,p),reg->str,32);
			WFIFOL(char_fd,p+32)=reg->value;
			p+=36;
		}
	}
	WFIFOW(char_fd,0)=0x2b10;
	WFIFOW(char_fd,2)=p;
	WFIFOL(char_fd,4)=sd->bl.id;
	WFIFOSET(char_fd,p);
	return 0;
}
/*==========================================
 * �A�J�E���g�ϐ��ʒm
 *------------------------------------------
 */
int chrif_accountreg2(int fd)
{
	int j,p;
	struct map_session_data *sd;
	if( (sd=map_id2sd(RFIFOL(fd,4)))==NULL )
		return 1;

	for(p=8,j=0;p<RFIFOW(fd,2) && j<ACCOUNT_REG2_NUM;p+=36,j++){
		memcpy(sd->status.account_reg2[j].str,RFIFOP(fd,p),32);
		sd->status.account_reg2[j].value=RFIFOL(fd,p+32);
	}
	sd->status.account_reg2_num=j;
//	printf("chrif: accountreg2\n");
	return 0;
}
/*==========================================
 * ������񓯊��v��
 *------------------------------------------
 */
int chrif_reqdivorce(int char_id)
{
	WFIFOW(char_fd,0)=0x2b20;
	WFIFOL(char_fd,2)=char_id;
	WFIFOSET(char_fd,6);
	return 0;
}
/*==========================================
 * ������񓯊���t
 *------------------------------------------
 */
int chrif_divorce(int char_id){
	struct map_session_data *sd = NULL;

	if( !char_id )
		return 0;
	if( (sd = map_nick2sd(map_charid2nick(char_id))) == NULL )
		return 0;

	if( sd->status.partner_id ){
		int i;
		// ����
		sd->status.partner_id = 0;

		// �����w�ւ𔍒D
		for(i=0;i<MAX_INVENTORY;i++)
			if(sd->status.inventory[i].nameid == WEDDING_RING_M || sd->status.inventory[i].nameid == WEDDING_RING_F)
				pc_delitem(sd,i,1,0);
	}
	return 0;
}
/*==========================================
 * map�T�[�o�A�N�e�B�u�v��
 *------------------------------------------
 */
int chrif_mapactive(int active){
	if(char_fd<=0)
		return -1;

	WFIFOW(char_fd,0)=0x2b13;
	WFIFOB(char_fd,2)=active;
	WFIFOSET(char_fd,3);
	printf("chrif: map_server_active %d\n",active);
	return 0;
}
/*==========================================
 * char�T�[�o�����e�i���X�v��
 *------------------------------------------
 */
int chrif_maintenance(int maintenance){
	if(char_fd<=0)
		return -1;

	WFIFOW(char_fd,0)=0x2b14;
	WFIFOB(char_fd,2)=maintenance;
	WFIFOSET(char_fd,3);
	printf("chrif: char_server_maintenance %d\n",maintenance);
	return 0;

}
/*==========================================
 * char�T�[�o�����e�i���X����
 *------------------------------------------
 */
int chrif_maintenanceack(int maintenance)
{
	if(maintenance)
		clif_GMmessage(NULL,msg_txt(82),strlen(msg_txt(82))+1,0);
	else
		clif_GMmessage(NULL,msg_txt(83),strlen(msg_txt(83))+1,0);

	return 0;
}
/*==========================================
 * �L�����N�^�[�ؒf�ʒm
 *------------------------------------------
 */
int chrif_chardisconnect(struct map_session_data *sd)
{
	nullpo_retr(-1, sd);

	if(char_fd<=0)
		return -1;

	WFIFOW(char_fd,0)=0x2b18;
	WFIFOL(char_fd,2)=sd->status.account_id;
	WFIFOL(char_fd,6)=sd->status.char_id;
	WFIFOSET(char_fd,10);
	//printf("chrif: char disconnect: %d %s\n",sd->bl.id,sd->status.name);
	return 0;

}
/*==========================================
 * char����L�����N�^�[�����ؒf�v��
 *------------------------------------------
 */
int chrif_parse_chardisconnectreq(int account_id)
{
	struct map_session_data *sd=map_id2sd(account_id);

	if(sd)
		clif_setwaitclose(sd->fd);

	return 0;
}

// �Q�d���O�C�����̏���
int chrif_parse_chardisconnect_doublelogin(int account_id)
{
	struct map_session_data *sd=map_id2sd(account_id);
	if(sd) {
		clif_authfail_fd(sd->fd,2);
		clif_setwaitclose(sd->fd);
	}
	return 0;
}


/*==========================================
 * �F�B���X�g�̍폜�ʒm
 *------------------------------------------
 */
int chrif_friend_delete( struct map_session_data* sd, int account_id, int char_id )
{
	if(char_fd<=0)
		return -1;

	WFIFOW( char_fd, 0 ) = 0x2b24;
	WFIFOL( char_fd, 2 ) = account_id;
	WFIFOL( char_fd, 6 ) = char_id;
	WFIFOL( char_fd,10 ) = sd->bl.id;
	WFIFOL( char_fd,14 ) = sd->status.char_id;
	WFIFOSET( char_fd, 18 );
	return 0;
}
/*==========================================
 * �F�B���X�g�̍폜�ʒm
 *------------------------------------------
 */
int chrif_parse_friend_delete( int fd )
{
	friend_del_from_otherserver( RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10), RFIFOL(fd,14) );
	return 0;
}
/*==========================================
 * �F�B���X�g�̃I�����C�����ʒm
 *------------------------------------------
 */
int chrif_friend_online( struct map_session_data *sd, int flag )
{
	int i, len;

	if(char_fd<=0)
		return -1;

	WFIFOW( char_fd, 0 ) = 0x2b26;
	WFIFOL( char_fd, 4 ) = sd->bl.id;
	WFIFOL( char_fd, 8 ) = sd->status.char_id;
	WFIFOL( char_fd,12 ) = flag;
	for( i=0, len=16; i<sd->status.friend_num; i++, len+=8 )
	{
		WFIFOL( char_fd, len   ) = sd->status.friend_data[i].account_id;
		WFIFOL( char_fd, len+4 ) = sd->status.friend_data[i].char_id;
	}
	WFIFOW( char_fd, 2 ) = len;
	WFIFOSET( char_fd, len );
	return 0;
}
/*==========================================
 * �F�B���X�g�̃I�����C�����ʒm��M
 *------------------------------------------
 */
int chrif_parse_friend_online( int fd )
{
	friend_send_online_from_otherserver( RFIFOL(fd,4), RFIFOL(fd,8), RFIFOL(fd,12), ( (RFIFOW(fd,2)-16)/8 ), (int*)RFIFOP( fd,16 ) );
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */

int chrif_disconnect_sub(struct map_session_data* sd,va_list va) {
	delete_session(sd->fd);
	return 0;
}

int chrif_disconnect(int fd) {
	if(fd==char_fd) {
		char_fd=0;
		// ���������Ȃ��̂ƁA�Đڑ��������ɖ�肪�N��\��������̂ŁA
		// map �I�ɐڑ����Ă���L������S�Đؒf����B
		printf("chrif: connection to the char server was closed. kick all chars\n");
		clif_foreachclient(chrif_disconnect_sub);
		// ����map �I�̃f�[�^������
		map_eraseallipport();

		// �q�ɃL���b�V��������
		do_final_storage();
		do_init_storage();
	}
	close(fd);
	return 0;
}

int chrif_parse(int fd)
{
	int packet_len,cmd;
	while(RFIFOREST(fd)>=2){
		cmd = RFIFOW(fd,0);
		if(cmd<0x2af8 || cmd>=0x2af8+(sizeof(packet_len_table)/sizeof(packet_len_table[0])) ||
		   packet_len_table[cmd-0x2af8]==0){

			int r=intif_parse(fd);// intif�ɓn��

			if( r==1 )	continue;	// intif�ŏ�������
			if( r==2 )	return 0;	// intif�ŏ����������A�f�[�^������Ȃ�

			close(fd);	// intif�ŏ����ł��Ȃ�����
			session[fd]->eof = 1;
			return 0;
		}
		packet_len = packet_len_table[cmd-0x2af8];
		if(packet_len==-1){
			if(RFIFOREST(fd)<4)
				return 0;
			packet_len = RFIFOW(fd,2);
		}
		if(RFIFOREST(fd)<packet_len)
			return 0;

		switch(cmd){
		case 0x2af9: chrif_connectack(fd); break;
		case 0x2afb: chrif_sendmapack(fd); break;
		case 0x2afd: chrif_authok(fd); break;
		case 0x2afe: pc_authfail(RFIFOL(fd,2)); break;
		case 0x2b00: map_setusers(RFIFOL(fd,2)); break;
		case 0x2b03: clif_charselectok(RFIFOL(fd,2)); break;
		case 0x2b04: chrif_recvmap(fd); break;
		case 0x2b06: chrif_changemapserverack(fd); break;
		case 0x2b09: map_addchariddb(RFIFOL(fd,2),RFIFOP(fd,6),RFIFOL(fd,30),RFIFOL(fd,34),RFIFOW(fd,38)); break;
		case 0x2b0b: chrif_changedgm(fd); break;
		case 0x2b0d: chrif_changedsex(fd); break;
		case 0x2b11: chrif_accountreg2(fd); break;
		case 0x2b12: chrif_divorce(RFIFOL(fd,2)); break;
		case 0x2b15: chrif_maintenanceack(RFIFOB(fd,2)); break;
		case 0x2b16: chrif_recverasemap(fd); break;
		case 0x2b17: map_delchariddb(RFIFOL(fd,2)); break;
		case 0x2b19: chrif_parse_chardisconnectreq(RFIFOL(fd,2)); break;
		case 0x2b1a: chrif_parse_chardisconnect_doublelogin(RFIFOL(fd,2)); break;
		case 0x2b25: chrif_parse_friend_delete(fd); break;
		case 0x2b27: chrif_parse_friend_online(fd); break;

		default:
			if(battle_config.error_log)
				printf("chrif_parse : unknown packet %d %d\n",fd,RFIFOW(fd,0));
			close(fd);
			session[fd]->eof=1;
			return 0;
		}
		RFIFOSKIP(fd,packet_len);
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
// timer�֐�
// ������map�I�Ɍq�����Ă���N���C�A���g�l����char�I�֑���
int send_users_tochar(int tid,unsigned int tick,int id,int data)
{
	if(char_fd<=0 || session[char_fd]==NULL)
		return 0;

	WFIFOW(char_fd,0)=0x2aff;
	WFIFOL(char_fd,2)=clif_countusers();
	WFIFOSET(char_fd,6);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
// timer�֐�
// char�I�Ƃ̐ڑ����m�F���A�����؂�Ă�����ēx�ڑ�����
int check_connect_char_server(int tid,unsigned int tick,int id,int data)
{
	if(char_fd<=0 || session[char_fd]==NULL){
		chrif_state = 0;
		char_fd=make_connection(char_ip,char_port);
		if(char_fd <= 0) { return 0; }
		session[char_fd]->func_parse=chrif_parse;
		session[char_fd]->func_destruct=chrif_disconnect;
		realloc_fifo(char_fd,FIFOSIZE_SERVERLINK,FIFOSIZE_SERVERLINK);

		chrif_connect(char_fd);
		chrif_mapactive(1);
	}
	return 0;
}
/*==========================================
 * �I��
 *------------------------------------------
 */
int do_final_chrif(void)
{
	delete_session(char_fd);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int do_init_chrif(void)
{
	add_timer_func_list(check_connect_char_server,"check_connect_char_server");
	add_timer_func_list(send_users_tochar,"send_users_tochar");
	add_timer_interval(gettick()+1000,check_connect_char_server,0,0,10*1000);
	add_timer_interval(gettick()+1000,send_users_tochar,0,0,5*1000);

	return 0;
}
