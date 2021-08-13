// $Id: chat.c,v 1.7 2003/06/29 05:52:56 lemit Exp $
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "nullpo.h"
#include "malloc.h"
#include "map.h"
#include "clif.h"
#include "pc.h"
#include "chat.h"
#include "npc.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

int chat_triggerevent(struct chat_data *cd);


/*==========================================
 * �`���b�g���[���쐬
 *------------------------------------------
 */
void chat_createchat(struct map_session_data *sd, unsigned short limit, unsigned char pub, char* pass, char* title, int titlelen)
{
	struct chat_data *cd;

	nullpo_retv(sd);

	if(sd->joinchat)
		if(chat_leavechat(sd,0))
			return;

	cd = aCalloc(1,sizeof(struct chat_data));

	cd->limit = (unsigned char)limit;
	cd->pub = pub;
	cd->users = 1;
	memcpy(cd->pass,pass,8);
	if(titlelen>=sizeof(cd->title)-1) titlelen=sizeof(cd->title)-1;
	memcpy(cd->title,title,titlelen);
	cd->title[titlelen]=0;

	cd->owner = (struct block_list **)(&cd->usersd[0]);
	cd->usersd[0] = sd;
	cd->bl.m = sd->bl.m;
	cd->bl.x = sd->bl.x;
	cd->bl.y = sd->bl.y;
	cd->bl.type = BL_CHAT;
	cd->zeny = 0;
	cd->lowlv = 0;
	cd->highlv = MAX_LEVEL;
	cd->job = ~(1<<MAX_PC_CLASS);
	cd->upper = 0;

	cd->bl.id = map_addobject(&cd->bl);
	if(cd->bl.id==0){
		clif_createchat(sd,1);
		free(cd);
		return;
	}

	pc_setchatid(sd,cd->bl.id);
	sd->joinchat = 1;

	clif_createchat(sd,0);
	clif_dispchat(cd,0);

	return;
}

/*==========================================
 * �����`���b�g���[���ɎQ��
 *------------------------------------------
 */
void chat_joinchat(struct map_session_data *sd, int chatid, char* pass)
{
	struct chat_data *cd;
	struct pc_base_job s_class;

	nullpo_retv(sd);

	cd=(struct chat_data*)map_id2bl(chatid);
	if(cd == NULL || cd->bl.type != BL_CHAT)
		return;

	if(cd->bl.m != sd->bl.m || sd->vender_id || sd->joinchat){
		clif_joinchatfail(sd,3);
		return;
	}
	if(cd->limit <= cd->users) {
		clif_joinchatfail(sd,0);
		return;
	}
	if(cd->pub==0 && strncmp(pass,cd->pass,8)){
		clif_joinchatfail(sd,1);
		return;
	}
	if(linkdb_search(&cd->ban_list, (void*)sd->status.char_id)) {
		clif_joinchatfail(sd,2);
		return;
	}
	if(cd->zeny > sd->status.zeny){
		clif_joinchatfail(sd,4);
		return;
	}
	if(cd->lowlv > sd->status.base_level){
		clif_joinchatfail(sd,5);
		return;
	}
	if(cd->highlv < sd->status.base_level){
		clif_joinchatfail(sd,6);
		return;
	}

	s_class = pc_calc_base_job(sd->status.class);
	if(((1<<s_class.job)&cd->job) == 0){
		clif_joinchatfail(sd,7);
		return;
	}
	if(cd->upper) {
		if(((1<<s_class.upper)&cd->upper) == 0){
			clif_joinchatfail(sd,7);
			return;
		}
	}

	cd->usersd[cd->users] = sd;
	cd->users++;

	pc_setchatid(sd,cd->bl.id);
	sd->joinchat=1;

	clif_joinchatok(sd,cd);	// �V���ɎQ�������l�ɂ͑S���̃��X�g
	clif_addchat(cd,sd);	// ���ɒ��ɋ����l�ɂ͒ǉ������l�̕�
	clif_dispchat(cd,0);	// ���͂̐l�ɂ͐l���ω���

	chat_triggerevent(cd); // �C�x���g

	return;
}

/*==========================================
 * �`���b�g���[�����甲����
 *------------------------------------------
 */
int chat_leavechat(struct map_session_data *sd, unsigned char flag)
{
	struct chat_data *cd;
	int i,leavechar;

	nullpo_retr(1, sd);

	cd=(struct chat_data*)map_id2bl(sd->chatID);
	if (cd == NULL || cd->bl.type != BL_CHAT)
		return 1;

	leavechar = -1;
	for(i = 0; i < cd->users; i++) {
		if(cd->usersd[i] == sd){
			leavechar=i;
			break;
		}
	}
	if(leavechar<0)	// ����chat�ɏ������Ă��Ȃ��炵�� (�o�O���̂�)
		return -1;

	if(leavechar==0 && cd->users>1 && (*cd->owner)->type==BL_PC){
		// ���L�҂�����&���ɐl������&PC�̃`���b�g
		clif_changechatowner(cd,cd->usersd[1]);
		clif_clearchat(cd,0);
	}

	// ������PC�ɂ�����̂�users�����炷�O�Ɏ��s
	clif_leavechat(cd,sd,flag);

	cd->users--;
	pc_setchatid(sd,0);
	sd->joinchat=0;

	if(cd->users == 0 && (*cd->owner)->type==BL_PC){
			// �S�����Ȃ��Ȃ���&PC�̃`���b�g�Ȃ̂ŏ���
		clif_clearchat(cd,0);
		linkdb_final(&cd->ban_list);
		map_delobject(cd->bl.id);	// free�܂ł��Ă����
	} else {
		for(i=leavechar;i < cd->users;i++)
			cd->usersd[i] = cd->usersd[i+1];
		if(leavechar==0 && (*cd->owner)->type==BL_PC){
			// PC�̃`���b�g�Ȃ̂ŏ��L�҂��������̂ňʒu�ύX
			cd->bl.x=cd->usersd[0]->bl.x;
			cd->bl.y=cd->usersd[0]->bl.y;
		}
		clif_dispchat(cd,0);
	}

	return 0;
}

/*==========================================
 * �`���b�g���[���̎����������
 *------------------------------------------
 */
void chat_changechatowner(struct map_session_data *sd, char *nextownername)
{
	struct chat_data *cd;
	struct map_session_data *tmp_sd;
	int i,nextowner;

	nullpo_retv(sd);

	cd=(struct chat_data*)map_id2bl(sd->chatID);
	if (cd == NULL || cd->bl.type != BL_CHAT || (struct block_list *)sd != (*cd->owner))
		return;

	nextowner = -1;
	for(i = 1;i < cd->users; i++) {
		if (strncmp(cd->usersd[i]->status.name, nextownername, 24) == 0) {
			nextowner=i;
			break;
		}
	}
	if(nextowner<0) // ����Ȑl�͋��Ȃ�
		return;

	clif_changechatowner(cd,cd->usersd[nextowner]);
	// ��U����
	clif_clearchat(cd,0);

	// userlist�̏��ԕύX (0�����L�҂Ȃ̂�)
	if( (tmp_sd = cd->usersd[0]) == NULL )
		return; //���肦��̂��ȁH
	cd->usersd[0] = cd->usersd[nextowner];
	cd->usersd[nextowner] = tmp_sd;

	// �V�������L�҂̈ʒu�֕ύX
	cd->bl.x=cd->usersd[0]->bl.x;
	cd->bl.y=cd->usersd[0]->bl.y;

	// �ēx�\��
	clif_dispchat(cd,0);

	return;
}

/*==========================================
 * �`���b�g�̏��(�^�C�g����)��ύX
 *------------------------------------------
 */
void chat_changechatstatus(struct map_session_data *sd, unsigned short limit, unsigned char pub, char* pass, char* title, int titlelen)
{
	struct chat_data *cd;

	nullpo_retv(sd);

	cd=(struct chat_data*)map_id2bl(sd->chatID);
	if (cd == NULL || cd->bl.type != BL_CHAT || (struct block_list *)sd != (*cd->owner))
		return;

	cd->limit = (unsigned char)limit;
	cd->pub = pub;
	memcpy(cd->pass,pass,8);
	if(titlelen>=sizeof(cd->title)-1) titlelen=sizeof(cd->title)-1;
	memcpy(cd->title,title,titlelen);
	cd->title[titlelen]=0;

	clif_changechatstatus(cd);
	clif_dispchat(cd,0);

	return;
}

/*==========================================
 * �`���b�g���[������R��o��
 *------------------------------------------
 */
void chat_kickchat(struct map_session_data *sd, char *kickusername)
{
	struct chat_data *cd;
	int i;

	nullpo_retv(sd);

	cd=(struct chat_data*)map_id2bl(sd->chatID);
	if (cd == NULL || cd->bl.type != BL_CHAT || (struct block_list *)sd != (*cd->owner))
		return;

	for(i = 0;i < cd->users; i++) {
		if (strncmp(cd->usersd[i]->status.name, kickusername, 24) == 0) {
			linkdb_insert(&cd->ban_list, (void*)cd->usersd[i]->status.char_id, (void*)1);
			chat_leavechat(cd->usersd[i], 1);
			break;
		}
	}

	return;
}

/*==========================================
 * npc�`���b�g���[���쐬
 *------------------------------------------
 */
int chat_createnpcchat(
	struct npc_data *nd,int limit,int pub,int trigger,char* title,int titlelen,const char *ev,
	int zeny,int lowlv,int highlv,int job,int upper)
{
	struct chat_data *cd;

	nullpo_retr(1, nd);

	if( nd->chat_id && (cd=(struct chat_data*)map_id2bl(nd->chat_id)) ) {
		map_delobject(cd->bl.id);
		cd = NULL;
	}

	cd = aCalloc(1,sizeof(struct chat_data));

	cd->limit = cd->trigger = limit;
	if(trigger>0)
		cd->trigger = trigger;
	cd->pub = pub;
	cd->users = 0;
	memcpy(cd->pass,"",8);
	if(titlelen>=sizeof(cd->title)-1) titlelen=sizeof(cd->title)-1;
	memcpy(cd->title,title,titlelen);
	cd->title[titlelen]=0;

	cd->bl.m = nd->bl.m;
	cd->bl.x = nd->bl.x;
	cd->bl.y = nd->bl.y;
	cd->bl.type = BL_CHAT;
	cd->owner_ = (struct block_list *)nd;
	cd->owner = &cd->owner_;
	cd->zeny = zeny;
	cd->lowlv = lowlv;
	cd->highlv = highlv;
	cd->job = job;
	cd->upper = upper;
	memcpy(cd->npc_event,ev,sizeof(cd->npc_event));

	cd->bl.id = map_addobject(&cd->bl);
	if(cd->bl.id==0){
		free(cd);
		return 0;
	}
	nd->chat_id=cd->bl.id;

	clif_dispchat(cd,0);

	return 0;
}
/*==========================================
 * npc�`���b�g���[���폜
 *------------------------------------------
 */
int chat_deletenpcchat(struct npc_data *nd)
{
	struct chat_data *cd;

	nullpo_retr(0, nd);
	nullpo_retr(0, cd=(struct chat_data*)map_id2bl(nd->chat_id));

	chat_npckickall(cd);
	clif_clearchat(cd,0);
	map_delobject(cd->bl.id);	// free�܂ł��Ă����
	nd->chat_id=0;

	return 0;
}

/*==========================================
 * �K��l���ȏ�ŃC�x���g����`����Ă�Ȃ���s
 *------------------------------------------
 */
int chat_triggerevent(struct chat_data *cd)
{
	nullpo_retr(0, cd);

	if(cd->users>=cd->trigger && cd->npc_event[0])
		npc_event_do(cd->npc_event);
	return 0;
}

/*==========================================
 * �C�x���g�̗L����
 *------------------------------------------
 */
int chat_enableevent(struct chat_data *cd)
{
	nullpo_retr(0, cd);

	cd->trigger&=0x7f;
	chat_triggerevent(cd);
	return 0;
}
/*==========================================
 * �C�x���g�̖�����
 *------------------------------------------
 */
int chat_disableevent(struct chat_data *cd)
{
	nullpo_retr(0, cd);

	cd->trigger|=0x80;
	return 0;
}
/*==========================================
 * �`���b�g���[������S���R��o��
 *------------------------------------------
 */
int chat_npckickall(struct chat_data *cd)
{
	nullpo_retr(0, cd);

	while(cd->users>0){
		chat_leavechat(cd->usersd[cd->users-1],0);
	}
	return 0;
}

/*==========================================
 * �I��
 *------------------------------------------
 */
int do_final_chat(void)
{
	return 0;
}
