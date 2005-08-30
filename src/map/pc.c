#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "timer.h"
#include "db.h"
#include "nullpo.h"
#include "malloc.h"
#include "map.h"
#include "chrif.h"
#include "clif.h"
#include "intif.h"
#include "pc.h"
#include "npc.h"
#include "mob.h"
#include "pet.h"
#include "itemdb.h"
#include "script.h"
#include "battle.h"
#include "skill.h"
#include "party.h"
#include "guild.h"
#include "chat.h"
#include "trade.h"
#include "storage.h"
#include "vending.h"
#include "status.h"
#include "socket.h"
#include "friend.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#ifdef _MSC_VER
	#define snprintf _snprintf
#endif

#define PVP_CALCRANK_INTERVAL 1000	// PVP���ʌv�Z�̊Ԋu

static int exp_table[15][MAX_LEVEL];

extern int current_equip_item_index;
extern int current_equip_card_id;

static int dirx[8]={0,-1,-1,-1,0,1,1,1};
static int diry[8]={1,1,0,-1,-1,-1,0,1};

static unsigned int equip_pos[11]={0x0080,0x0008,0x0040,0x0004,0x0001,0x0200,0x0100,0x0010,0x0020,0x0002,0x8000};

static char GM_account_filename[1024] = "conf/GM_account.txt";
static struct dbt *gm_account_db;

static struct {
	int id;
	int max;
	struct {
		short id,lv;
	} need[6];
	short base_level;
	short job_level;
} skill_tree[3][MAX_PC_CLASS][100];

void pc_set_gm_account_fname(char *str)
{
	strncpy(GM_account_filename,str,1024);
}

int pc_isGM(struct map_session_data *sd)
{
	struct gm_account *p;

	nullpo_retr(0, sd);

	if( (p = numdb_search(gm_account_db,sd->status.account_id)) == NULL )
		return 0;
	return p->level;
}
int pc_numisGM(int account_id)
{
	struct gm_account *p;

	if( (p = numdb_search(gm_account_db,account_id)) == NULL )
		return 0;
	return p->level;
}
/*==========================================
 * PC���I���ł��邩�ǂ����̔���
 *------------------------------------------
 */
int pc_isquitable(struct map_session_data *sd)
{
	unsigned int tick=gettick();
	struct skill_unit_group* sg;
	int c=0;

	nullpo_retr(0, sd);

	c=pc_counttargeted(sd,NULL,0);

	if(!pc_isdead(sd) && (sd->opt1 || sd->opt2))
		return 1;
	if(sd->skilltimer != -1)
		return 1;
	if(DIFF_TICK(tick , sd->canact_tick) < 0)
		return 1;
	if(sd->sc_data && sd->sc_data[SC_DANCING].timer!=-1 && sd->sc_data[SC_DANCING].val4 && (sg=(struct skill_unit_group *)sd->sc_data[SC_DANCING].val2) && sg->src_id == sd->bl.id)
		return 1;
	if(c > 0)
		return 1;

	return 0;
}

static int distance(int x0,int y0,int x1,int y1)
{
	int dx,dy;

	dx=abs(x0-x1);
	dy=abs(y0-y1);
	return dx>dy ? dx : dy;
}

static int pc_invincible_timer(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd;

	if( (sd=(struct map_session_data *)map_id2sd(id)) == NULL || sd->bl.type!=BL_PC )
		return 1;

	if(sd->invincible_timer != tid){
		if(battle_config.error_log)
			printf("invincible_timer %d != %d\n",sd->invincible_timer,tid);
		return 0;
	}
	sd->invincible_timer=-1;
	skill_unit_move(&sd->bl,tick,1);
	return 0;
}

int pc_setinvincibletimer(struct map_session_data *sd,int val)
{
	nullpo_retr(0, sd);

	if(sd->invincible_timer != -1)
		delete_timer(sd->invincible_timer,pc_invincible_timer);
	sd->invincible_timer = add_timer(gettick()+val,pc_invincible_timer,sd->bl.id,0);
	return 0;
}

int pc_delinvincibletimer(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	if(sd->invincible_timer != -1) {
		delete_timer(sd->invincible_timer,pc_invincible_timer);
		sd->invincible_timer = -1;
		skill_unit_move(&sd->bl,gettick(),1);
	}
	return 0;
}

static int pc_spiritball_timer(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd;
	int i;

	if( (sd=(struct map_session_data *)map_id2sd(id)) == NULL || sd->bl.type!=BL_PC )
		return 1;

	if(sd->spirit_timer[0] != tid){
		if(battle_config.error_log)
			printf("spirit_timer %d != %d\n",sd->spirit_timer[0],tid);
		return 0;
	}
	sd->spirit_timer[0]=-1;
	for(i=1;i<sd->spiritball;i++) {
		sd->spirit_timer[i-1] = sd->spirit_timer[i];
		sd->spirit_timer[i] = -1;
	}
	sd->spiritball--;
	if(sd->spiritball < 0)
		sd->spiritball = 0;
	clif_spiritball(sd);

	return 0;
}

int pc_addspiritball(struct map_session_data *sd,int interval,int max)
{
	int i;

	nullpo_retr(0, sd);

	if(max > MAX_SKILL_LEVEL)
		max = MAX_SKILL_LEVEL;
	if(sd->spiritball < 0)
		sd->spiritball = 0;

	if(sd->spiritball >= max) {
		if(sd->spirit_timer[0] != -1) {
			delete_timer(sd->spirit_timer[0],pc_spiritball_timer);
			sd->spirit_timer[0] = -1;
		}
		for(i=1;i<max;i++) {
			sd->spirit_timer[i-1] = sd->spirit_timer[i];
			sd->spirit_timer[i] = -1;
		}
	}
	else
		sd->spiritball++;

	sd->spirit_timer[sd->spiritball-1] = add_timer(gettick()+interval,pc_spiritball_timer,sd->bl.id,0);
	clif_spiritball(sd);

	return 0;
}


int pc_delspiritball(struct map_session_data *sd,int count,int type)
{
	int i;

	nullpo_retr(0, sd);

	if(sd->spiritball <= 0) {
		sd->spiritball = 0;
		return 0;
	}

	if(count > sd->spiritball)
		count = sd->spiritball;
	sd->spiritball -= count;
	if(count > MAX_SKILL_LEVEL)
		count = MAX_SKILL_LEVEL;

	for(i=0;i<count;i++) {
		if(sd->spirit_timer[i] != -1) {
			delete_timer(sd->spirit_timer[i],pc_spiritball_timer);
			sd->spirit_timer[i] = -1;
		}
	}
	for(i=count;i<MAX_SKILL_LEVEL;i++) {
		sd->spirit_timer[i-count] = sd->spirit_timer[i];
		sd->spirit_timer[i] = -1;
	}

	if(!type)
		clif_spiritball(sd);

	return 0;
}

int pc_setrestartvalue(struct map_session_data *sd,int type)
{
	//�]����{�q�̏ꍇ�̌��̐E�Ƃ��Z�o����
	struct pc_base_job s_class;

	nullpo_retr(0, sd);

	s_class = pc_calc_base_job(sd->status.class);

	//-----------------------
	// ���S����
	if(sd->special_state.restart_full_recover) {	// �I�V���X�J�[�h
		sd->status.hp=sd->status.max_hp;
		sd->status.sp=sd->status.max_sp;
	}
	else {
		if(s_class.job == 0 && battle_config.restart_hp_rate < 50) { //�m�r�͔�����
			sd->status.hp=(sd->status.max_hp)/2;
		}
		else {
			if(battle_config.restart_hp_rate <= 0)
				sd->status.hp = 1;
			else {
				sd->status.hp = sd->status.max_hp * battle_config.restart_hp_rate /100;
				if(sd->status.hp <= 0)
					sd->status.hp = 1;
			}
		}
		if(battle_config.restart_sp_rate > 0) {
			int sp = sd->status.max_sp * battle_config.restart_sp_rate /100;
			if(sd->status.sp < sp)
				sd->status.sp = sp;
		}
	}
	if(type&1) {
		clif_updatestatus(sd,SP_HP);
		clif_updatestatus(sd,SP_SP);
	}

	if(type&2) {
		if(!(battle_config.death_penalty_type&1) ) { //�f�X�y�i
			if((s_class.job != 0) && !map[sd->bl.m].flag.nopenalty && !map[sd->bl.m].flag.gvg){
				if(battle_config.death_penalty_type&2 && battle_config.death_penalty_base > 0)
					sd->status.base_exp -= (int)((atn_bignumber)pc_nextbaseexp(sd) * battle_config.death_penalty_base/10000);
				else if(battle_config.death_penalty_base > 0) {
					if(pc_nextbaseexp(sd) > 0)
						sd->status.base_exp -= (int)((atn_bignumber)sd->status.base_exp * battle_config.death_penalty_base/10000);
				}
				if(sd->status.base_exp < 0)
					sd->status.base_exp = 0;
				if(type&1)
					clif_updatestatus(sd,SP_BASEEXP);

				if(battle_config.death_penalty_type&2 && battle_config.death_penalty_job > 0)
					sd->status.job_exp -= (int)((atn_bignumber)pc_nextjobexp(sd) * battle_config.death_penalty_job/10000);
				else if(battle_config.death_penalty_job > 0) {
					if(pc_nextjobexp(sd) > 0)
						sd->status.job_exp -= (int)((atn_bignumber)sd->status.job_exp * battle_config.death_penalty_job/10000);
				}
				if(sd->status.job_exp < 0)
					sd->status.job_exp = 0;
				if(type&1)
					clif_updatestatus(sd,SP_JOBEXP);
			}
		}
		if(battle_config.zeny_penalty > 0&&!map[sd->bl.m].flag.nozenypenalty) {
			int zeny = (int)((atn_bignumber)sd->status.zeny * battle_config.zeny_penalty / 10000 );
			if(zeny < 1) zeny = 1;
			sd->status.zeny -= zeny;
			if(sd->status.zeny < 0) sd->status.zeny = 0;
			if(type&1)
				clif_updatestatus(sd,SP_ZENY);
		}
	}

	return 0;
}

/*==========================================
 * ���������b�N���Ă���MOB�̐��𐔂���(foreachclient)
 *------------------------------------------
 */
static int pc_counttargeted_sub(struct block_list *bl,va_list ap)
{
	int id,*c,target_lv;
	struct block_list *src;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	id=va_arg(ap,int);

	nullpo_retr(0, c=va_arg(ap,int *));

	src=va_arg(ap,struct block_list *);
	target_lv=va_arg(ap,int);
	if(id == bl->id || (src && id == src->id)) return 0;
	if(bl->type == BL_PC) {
		struct map_session_data *sd=(struct map_session_data *)bl;
		if( sd && sd->attacktarget == id && sd->attacktimer != -1 && sd->attacktarget_lv >= target_lv)
			(*c)++;
	}
	else if(bl->type == BL_MOB) {
		struct mob_data *md = (struct mob_data *)bl;
//		if(md && md->target_id == id && md->timer != -1 && md->state.state == MS_ATTACK && md->target_lv >= target_lv)
		if(md && md->target_id == id && md->target_lv >= target_lv)
			(*c)++;
		//printf("md->target_lv:%d, target_lv:%d\n",((struct mob_data *)bl)->target_lv,target_lv);
	}
	return 0;
}

int pc_counttargeted(struct map_session_data *sd,struct block_list *src,int target_lv)
{
	int c=0;
	map_foreachinarea(pc_counttargeted_sub, sd->bl.m,
		sd->bl.x-AREA_SIZE,sd->bl.y-AREA_SIZE,
		sd->bl.x+AREA_SIZE,sd->bl.y+AREA_SIZE,0,sd->bl.id,&c,src,target_lv);
	return c;
}

/*==========================================
 * ���[�J���v���g�^�C�v�錾 (�K�v�ȕ��̂�)
 *------------------------------------------
 */
static int pc_walktoxy_sub(struct map_session_data *);


/*==========================================
 * save�ɕK�v�ȃX�e�[�^�X�C�����s�Ȃ�
 *------------------------------------------
 */
int pc_makesavestatus(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	// ���̐F�͐F�X���Q�������̂ŕۑ��Ώۂɂ͂��Ȃ�
	if(!battle_config.save_clothcolor)
		sd->status.clothes_color=0;

	// �ؒf�҂��̎��͏������Ȃ�
	if(!sd->state.waitingdisconnect) {
		// ���S��Ԃ������̂�hp��1�A�ʒu���Z�[�u�ꏊ�ɕύX
		if(pc_isdead(sd)){
			pc_setrestartvalue(sd,0);
			memcpy(&sd->status.last_point,&sd->status.save_point,sizeof(sd->status.last_point));
		} else {
			memcpy(sd->status.last_point.map,sd->mapname,24);
			sd->status.last_point.x = sd->bl.x;
			sd->status.last_point.y = sd->bl.y;
		}

		// �Z�[�u�֎~�}�b�v�������̂Ŏw��ʒu�Ɉړ�
		if(map[sd->bl.m].flag.nosave){
			struct map_data *m=&map[sd->bl.m];
			if(strcmp(m->save.map,"SavePoint")==0)
				memcpy(&sd->status.last_point,&sd->status.save_point,sizeof(sd->status.last_point));
			else
				memcpy(&sd->status.last_point,&m->save,sizeof(sd->status.last_point));
		}
	}

	//�}�i�[�|�C���g���v���X�������ꍇ0��
	if(sd->status.manner > 0)
		sd->status.manner = 0;
	return 0;
}


/*==========================================
 * �ڑ����̏�����
 *------------------------------------------
 */
int pc_setnewpc(struct map_session_data *sd,int account_id,int char_id,int login_id1,int client_tick,int sex,int fd)
{
	int i;
	unsigned int tick = gettick();
	
	nullpo_retr(0, sd);

	sd->bl.id        = account_id;
	sd->char_id      = char_id;
	sd->login_id1    = login_id1;
	sd->client_tick  = client_tick;
	sd->sex          = sex;
	sd->state.auth   = 0;
	sd->bl.type      = BL_PC;
	sd->canact_tick = sd->canmove_tick = tick;
	sd->state.waitingdisconnect=0;
	for(i=0; i<MAX_SKILL_DB; i++)
		sd->skillstatictimer[i] = tick;

	return 0;
}


int pc_equippoint(struct map_session_data *sd,int n)
{
	int ep = 0;
	//�]����{�q�̏ꍇ�̌��̐E�Ƃ��Z�o����
	struct pc_base_job s_class;

	nullpo_retr(0, sd);

	s_class = pc_calc_base_job(sd->status.class);

	if(sd->inventory_data[n]) {
		ep = sd->inventory_data[n]->equip;
		if(sd->inventory_data[n]->look == 1 || sd->inventory_data[n]->look == 2 || sd->inventory_data[n]->look == 6) {
			if(ep == 2 && (pc_checkskill(sd,AS_LEFT) > 0 || s_class.job == 12))
				return 34;
		}
	}
	return ep;
}

int pc_setinventorydata(struct map_session_data *sd)
{
	int i,id;

	nullpo_retr(0, sd);

	for(i=0;i<MAX_INVENTORY;i++) {
		id = sd->status.inventory[i].nameid;
		sd->inventory_data[i] = itemdb_search(id);
	}
	return 0;
}

int pc_calcweapontype(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	if(sd->weapontype1 != 0 &&	sd->weapontype2 == 0)
		sd->status.weapon = sd->weapontype1;
	if(sd->weapontype1 == 0 &&	sd->weapontype2 != 0)// ���蕐�� Only
		sd->status.weapon = sd->weapontype2;
	else if(sd->weapontype1 == 1 && sd->weapontype2 == 1)// �o�Z��
		sd->status.weapon = 0x11;
	else if(sd->weapontype1 == 2 && sd->weapontype2 == 2)// �o�P�茕
		sd->status.weapon = 0x12;
	else if(sd->weapontype1 == 6 && sd->weapontype2 == 6)// �o�P�蕀
		sd->status.weapon = 0x13;
	else if( (sd->weapontype1 == 1 && sd->weapontype2 == 2) ||
		(sd->weapontype1 == 2 && sd->weapontype2 == 1) ) // �Z�� - �P�茕
		sd->status.weapon = 0x14;
	else if( (sd->weapontype1 == 1 && sd->weapontype2 == 6) ||
		(sd->weapontype1 == 6 && sd->weapontype2 == 1) ) // �Z�� - ��
		sd->status.weapon = 0x15;
	else if( (sd->weapontype1 == 2 && sd->weapontype2 == 6) ||
		(sd->weapontype1 == 6 && sd->weapontype2 == 2) ) // �P�茕 - ��
		sd->status.weapon = 0x16;
	else
		sd->status.weapon = sd->weapontype1;

	return 0;
}

int pc_setequipindex(struct map_session_data *sd)
{
	int i,j;

	nullpo_retr(0, sd);

	for(i=0;i<11;i++)
		sd->equip_index[i] = -1;

	for(i=0;i<MAX_INVENTORY;i++) {
		if(sd->status.inventory[i].nameid <= 0)
			continue;
		if(sd->status.inventory[i].equip) {
			for(j=0;j<11;j++)
				if(sd->status.inventory[i].equip & equip_pos[j])
					sd->equip_index[j] = i;
			if(sd->status.inventory[i].equip & 0x0002) {
				if(sd->inventory_data[i])
					sd->weapontype1 = sd->inventory_data[i]->look;
				else
					sd->weapontype1 = 0;
			}
			if(sd->status.inventory[i].equip & 0x0020) {
				if(sd->inventory_data[i]) {
					if(sd->inventory_data[i]->type == 4) {
						if(sd->status.inventory[i].equip == 0x0020)
							sd->weapontype2 = sd->inventory_data[i]->look;
						else
							sd->weapontype2 = 0;
					}
					else
						sd->weapontype2 = 0;
				}
				else
					sd->weapontype2 = 0;
			}
		}
	}
	pc_calcweapontype(sd);

	return 0;
}

int pc_isequip(struct map_session_data *sd,int n)
{
	struct item_data *item;
	struct status_change *sc_data;
	//�]����{�q�̏ꍇ�̌��̐E�Ƃ��Z�o����
	struct pc_base_job s_class;

	nullpo_retr(0, sd);

	item = sd->inventory_data[n];
	sc_data = status_get_sc_data(&sd->bl);
	s_class = pc_calc_base_job(sd->status.class);

 	if( battle_config.gm_allequip>0 && pc_isGM(sd)>=battle_config.gm_allequip )
		return 1;

	if(item == NULL)
		return 0;
		
	//�X�p�m�r�̍�
	if(item->equip & 0x0002 && sd->status.base_level>=96 && sd->sc_data[SC_SUPERNOVICE].timer!=-1)
	{
		if(sc_data && sc_data[SC_STRIPWEAPON].timer != -1)
			return 0;
		if(item->wlv == 4)
		{
			if(item->type==1 || item->type==2 || item->type== 6 || item->type==8 ||item->type== 10)
				return 1;
		}
	}
	//���֌W
	if(sd->status.base_level>=90 && sd->sc_data[SC_SUPERNOVICE].timer!=-1)
	{
		//�ꉞ���x�������`�F�b�N
		if(item->elv > 0 && sd->status.base_level < item->elv)
			return 0;
		//���ŃX�g���b�v�Ȃ玸�s
		if(item->equip & 0x0100 && sc_data && sc_data[SC_STRIPHELM].timer != -1)
			return 0;
		//���Ȃ琬��
		//����i
		if(item->equip & 0x0100)
			return 1;
		//�����i
		if(item->equip & 0x0200)
			return 1;
		//�����i
		if(item->equip & 0x0002)
			return 1;
	}
	
	if(battle_config.equip_sex)
		if(item->sex != 2 && sd->sex != item->sex)
			return 0;
	if(item->elv > 0 && sd->status.base_level < item->elv)
		return 0;
	if(((1<<s_class.job)&item->class) == 0)
		return 0;
	if(map[sd->bl.m].flag.pvp && (item->flag.no_equip==1 || item->flag.no_equip==3))
		return 0;
	if(map[sd->bl.m].flag.gvg && (item->flag.no_equip==2 || item->flag.no_equip==3))
		return 0;
	if(item->equip & 0x0002 && sc_data && sc_data[SC_STRIPWEAPON].timer != -1)
		return 0;
	if(item->equip & 0x0020 && sc_data && sc_data[SC_STRIPSHIELD].timer != -1)
		return 0;
	if(item->equip & 0x0010 && sc_data && sc_data[SC_STRIPARMOR].timer != -1)
		return 0;
	if(item->equip & 0x0100 && sc_data && sc_data[SC_STRIPHELM].timer != -1)
		return 0;
	return 1;
}

/*==========================================
 * session id�ɖ�薳��
 * char�I���瑗���Ă����X�e�[�^�X��ݒ�
 *------------------------------------------
 */
int pc_authok(int id,struct mmo_charstatus *st)
{
	struct map_session_data *sd = NULL;
	struct party *p = NULL;
	struct guild *g = NULL;
	int i;
	unsigned long tick = gettick();

	sd = map_id2sd(id);
	if(sd==NULL)
		return 1;
	if(sd->new_fd){
		// 2�dlogin��Ԃ������̂ŁA��������
		clif_authfail_fd(sd->fd,2);	// same id
		if(session[sd->new_fd] && ((struct block_list*)session[sd->new_fd])->id == id) {
			clif_authfail_fd(sd->new_fd,2);	// same id
		}
		return 1;
	}
	
	memcpy(&sd->status,st,sizeof(*st));

	if(sd->status.char_id != sd->char_id){
		clif_authfail_fd(sd->fd,0);
		return 1;
	}

	session[sd->fd]->auth = 1; // �F�؏I���� socket.c �ɓ`����

	memset(&sd->state,0,sizeof(sd->state));
	// ��{�I�ȏ�����
	sd->state.connect_new = 1;
	sd->bl.prev = sd->bl.next = NULL;
	sd->weapontype1 = sd->weapontype2 = 0;
	sd->view_class = sd->status.class;
	sd->speed = DEFAULT_WALK_SPEED;
	sd->state.dead_sit=0;
	sd->dir=0;
	sd->head_dir=0;
	sd->state.auth=1;
	sd->walktimer=-1;
	sd->attacktimer=-1;
	sd->skilltimer=-1;
	sd->skillitem=-1;
	sd->skillitemlv=-1;
	sd->invincible_timer=-1;

	sd->deal_locked =0;
	sd->deal_mode =0;
	sd->trade_partner=0;

	sd->inchealhptick = 0;
	sd->inchealsptick = 0;
	sd->hp_sub = 0;
	sd->sp_sub = 0;
	sd->inchealspirithptick = 0;
	sd->inchealspiritsptick = 0;
	sd->canact_tick = tick;
	sd->canmove_tick = tick;
	sd->attackabletime = tick;
	
	sd->inchealresthptick = 0;
	sd->inchealrestsptick = 0;

	sd->doridori_counter = 0;
	sd->tk_doridori_counter_hp = 0;
	sd->tk_doridori_counter_sp = 0;

	sd->sm_autoberserk_on = 1;

	sd->spiritball = 0;
	sd->wis_all = 0;

	sd->repair_target = NULL;

	for(i=0;i<MAX_SKILL_LEVEL;i++)
		sd->spirit_timer[i] = -1;
	for(i=0;i<MAX_SKILLTIMERSKILL;i++)
		sd->skilltimerskill[i].timer = -1;
	for(i=0;i<MAX_SKILL_DB;i++)
		sd->skillstatictimer[i] = tick;

	memset(&sd->dev,0,sizeof(struct square));
	for(i=0;i<5;i++){
		sd->dev.val1[i] = 0;
		sd->dev.val2[i] = 0;
	}

	// �A�J�E���g�ϐ��̑��M�v��
	intif_request_accountreg(sd);

	// �A�C�e���`�F�b�N
	pc_setinventorydata(sd);
	pc_checkitem(sd);

	// pet
	sd->petDB = NULL;
	sd->pd = NULL;
	sd->pet_hungry_timer = -1;
	memset(&sd->pet,0,sizeof(struct s_pet));

	// �X�e�[�^�X�ُ�̏�����
	for(i=0;i<MAX_STATUSCHANGE;i++) {
		sd->sc_data[i].timer=-1;
		sd->sc_data[i].val1 = sd->sc_data[i].val2 = sd->sc_data[i].val3 = sd->sc_data[i].val4 = 0;
	}
	
	sd->sc_count=0;
	sd->status.option&=OPTION_MASK;
	//�I�[�g�o�[�T�N��ON
	status_change_start(&sd->bl,SC_AUTOBERSERK,1,0,0,0,0,0);

	// �X�L�����j�b�g�֌W�̏�����
	memset(sd->skillunit,0,sizeof(sd->skillunit));
	memset(sd->skillunittick,0,sizeof(sd->skillunittick));

	// �p�[�e�B�[�֌W�̏�����
	sd->party_sended=0;
	sd->party_invite=0;
	sd->party_x=-1;
	sd->party_y=-1;
	sd->party_hp=-1;

	// �M���h�֌W�̏�����
	sd->guild_sended=0;
	sd->guild_invite=0;
	sd->guild_alliance=0;

	// �C�x���g�֌W�̏�����
	memset(sd->eventqueue,0,sizeof(sd->eventqueue));
	for(i=0;i<MAX_EVENTTIMER;i++)
		sd->eventtimer[i] = -1;

	// �ʒu�̐ݒ�
	pc_setpos(sd,sd->status.last_point.map ,
		sd->status.last_point.x , sd->status.last_point.y, 0);

	// pet
	if(sd->status.pet_id > 0)
		intif_request_petdata(sd->status.account_id,sd->status.char_id,sd->status.pet_id);

	// �p�[�e�B�A�M���h�f�[�^�̗v��
	if( sd->status.party_id>0 && (p=party_search(sd->status.party_id))==NULL)
		party_request_info(sd->status.party_id);
	if( sd->status.guild_id>0 && (g=guild_search(sd->status.guild_id))==NULL)
		guild_request_info(sd->status.guild_id);

	// pvp�̐ݒ�
	sd->pvp_rank=0;
	sd->pvp_point=0;
	sd->pvp_timer=-1;

	// �ʒm
	clif_authok(sd);
	map_addnickdb(sd);
//	if( map_charid2nick(sd->status.char_id)==NULL )
		map_addchariddb(sd->status.char_id,sd->status.name,sd->status.account_id,clif_getip(),clif_getport());

	//�X�p�m�r�p���ɃJ�E���^�[�̃X�N���v�g�ϐ�����̓ǂݏo����sd�ւ̃Z�b�g
	sd->die_counter = pc_readglobalreg(sd,"PC_DIE_COUNTER");

	//�����p�ꏊ�f�[�^�̓ǂݍ���
	for(i=0;i<3;i++){
		strcpy(sd->feel_map[i].name,"");
		sd->feel_map[i].m = map_mapname2mapid(sd->feel_map[i].name);
		sd->hate_mob[i]=-1;
	}
	/*
	//���z�ƌ��Ɛ��̑����� �܂��ۑ��͕ۗ���
	sd->hate_mob[0] = pc_readglobalreg(sd,"PC_HATE_MOB_SUN");
	sd->hate_mob[1] = pc_readglobalreg(sd,"PC_HATE_MOB_MOON");
	sd->hate_mob[2] = pc_readglobalreg(sd,"PC_HATE_MOB_STAR");
	*/
	
	// �X�e�[�^�X�����v�Z�Ȃ�
	status_calc_pc(sd,1);

	// Message of the Day�̑��M
	{
		char buf[256];
		FILE *fp;
		if(	(fp = fopen(motd_txt, "r"))!=NULL){
			while (fgets(buf, 250, fp) != NULL){
				int i;
				for( i=0; buf[i]; i++){
					if( buf[i]=='\r' || buf[i]=='\n'){
						buf[i]=0;
						break;
					}
				}
				clif_displaymessage(sd->fd,buf);
			}
			fclose(fp);
		}
	}

	return 0;
}


/*==========================================
 * session id�ɖ�肠��Ȃ̂Ō�n��
 *------------------------------------------
 */
int pc_authfail(int id)
{
	struct map_session_data *sd;

	sd = map_id2sd(id);
	if(sd==NULL)
		return 1;
	if(sd->new_fd){
		// 2�dlogin��Ԃ������̂ŁA�V�����ڑ��̂ݗ���
		clif_authfail_fd(sd->new_fd,0);

		sd->new_fd=0;
		return 0;
	}
	clif_authfail_fd(sd->fd,0);
	return 0;
}


static int pc_calc_skillpoint(struct map_session_data* sd)
{
	int  i,skill,skill_point=0;

	nullpo_retr(0, sd);

	for(i=1;i<MAX_SKILL;i++){
		if( (skill = pc_checkskill(sd,i)) > 0) {
			if(!(skill_get_inf2(i)&0x01) || battle_config.quest_skill_learn) {
				if(!sd->status.skill[i].flag)
					skill_point += skill;
				else if(sd->status.skill[i].flag > 2 && sd->status.skill[i].flag != 13) {
					skill_point += (sd->status.skill[i].flag - 2);
				}
			}
		}
	}

	return skill_point;
}

/*==========================================
 * �o������X�L���̌v�Z
 *------------------------------------------
 */
int pc_calc_skilltree(struct map_session_data *sd)
{
	int i,id=0,flag;
	int c=0, s=0;
	//�]����{�q�̏ꍇ�̌��̐E�Ƃ��Z�o����
	struct pc_base_job s_class;

	nullpo_retr(0, sd);

	s_class = pc_calc_base_job(sd->status.class);
	c = s_class.job;
	s = (s_class.upper==1) ? 1 : 0 ; //�]���ȊO�͒ʏ�̃X�L���H
	if(battle_config.skillup_limit && c >= 0 && c < MAX_VALID_PC_CLASS) {
		int skill_point = pc_calc_skillpoint(sd);
		if(skill_point < 9)
			c = 0;
		else if(sd->status.skill_point >= sd->status.job_level && skill_point < 58 && c > 6) {
			switch(c) {
				case 7:
				case 14:
					c = 1;
					break;
				case 8:
				case 15:
					c = 4;
					break;
				case 9:
				case 16:
					c = 2;
					break;
				case 10:
				case 18:
					c = 5;
					break;
				case 11:
				case 19:
				case 20:
					c = 3;
					break;
				case 12:
				case 17:
					c = 6;
					break;
			}
		}
	}

	for(i=0;i<MAX_SKILL;i++){
		if (sd->status.skill[i].flag != 13) sd->status.skill[i].id=0;
		if (sd->status.skill[i].flag && sd->status.skill[i].flag != 13){	// card�X�L���Ȃ�A
			sd->status.skill[i].lv=(sd->status.skill[i].flag==1)?0:sd->status.skill[i].flag-2;	// �{����lv��
			sd->status.skill[i].flag=0;	// flag��0�ɂ��Ă���
		}
	}

	if (battle_config.gm_allskill > 0 && pc_isGM(sd) >= battle_config.gm_allskill){
		// �S�ẴX�L��
		for(i=1;i<158;i++)
			sd->status.skill[i].id=i;
		for(i=210;i<291;i++)
			sd->status.skill[i].id=i;
		if(battle_config.gm_allskill_addabra){ // �A�u���J�^�u����p�X�L��
			for(i=291;i<304;i++)
				sd->status.skill[i].id=i;
		}
		for(i=304;i<337;i++)
			if(i!=331)
				sd->status.skill[i].id=i;
		if(battle_config.enable_upper_class){ //conf�Ŗ����łȂ���Γǂݍ���
			for(i=355;i<MAX_SKILL;i++)
				sd->status.skill[i].id=i;
		}
	}else{
		// �ʏ�̌v�Z
		do{
			flag=0;
			for(i=0;(id=skill_tree[s][c][i].id)>0;i++){
				int j,f=1;
				if(!battle_config.skillfree) {
					for(j=0;j<5;j++) {
						if( skill_tree[s][c][i].need[j].id &&
							pc_checkskill(sd,skill_tree[s][c][i].need[j].id) < skill_tree[s][c][i].need[j].lv)
							f=0;
					}
					if(sd->status.base_level < skill_tree[s][c][i].base_level)
						f = 0;
					if(sd->status.job_level < skill_tree[s][c][i].job_level)
						f = 0;
				}
				if(f && sd->status.skill[id].id==0 ){
					sd->status.skill[id].id=id;
					flag=1;
				}
			}
		}while(flag);
	}
//	if(battle_config.etc_log)
//		printf("calc skill_tree\n");
	return 0;
}

/*==========================================
 * �d�ʃA�C�R���̊m�F
 *------------------------------------------
 */
int pc_checkweighticon(struct map_session_data *sd)
{
	int flag=0;

	nullpo_retr(0, sd);

	if(sd->weight*2 >= sd->max_weight)
		flag=1;
	if(sd->weight*10 >= sd->max_weight*9)
		flag=2;

	if(flag==1){
		if(sd->sc_data[SC_WEIGHT50].timer==-1)
			status_change_start(&sd->bl,SC_WEIGHT50,0,0,0,0,0,0);
	}else{
		status_change_end(&sd->bl,SC_WEIGHT50,-1);
	}
	if(flag==2){
		if(sd->sc_data[SC_WEIGHT90].timer==-1)
			status_change_start(&sd->bl,SC_WEIGHT90,0,0,0,0,0,0);
	}else{
		status_change_end(&sd->bl,SC_WEIGHT90,-1);
	}
	return 0;
}

/*==========================================
 * �����i�ɂ��\�͓��̃{�[�i�X�ݒ�
 *------------------------------------------
 */
int pc_bonus(struct map_session_data *sd,int type,int val)
{
	nullpo_retr(0, sd);

	switch(type){
	case SP_STR:
	case SP_AGI:
	case SP_VIT:
	case SP_INT:
	case SP_DEX:
	case SP_LUK:
		if(sd->state.lr_flag != 2)
			sd->parame[type-SP_STR]+=val;
		break;
	case SP_ATK1:
		if(!sd->state.lr_flag)
			sd->watk+=val;
		else if(sd->state.lr_flag == 1)
			sd->watk_+=val;
		break;
	case SP_ATK2:
		if(!sd->state.lr_flag)
			sd->watk2+=val;
		else if(sd->state.lr_flag == 1)
			sd->watk_2+=val;
		break;
	case SP_BASE_ATK:
		if(sd->state.lr_flag != 2)
			sd->base_atk+=val;
		break;
	case SP_MATK1:
		if(sd->state.lr_flag != 2)
			sd->matk1 += val;
		break;
	case SP_MATK2:
		if(sd->state.lr_flag != 2)
			sd->matk2 += val;
		break;
	case SP_MATK:
		if(sd->state.lr_flag != 2) {
			sd->matk1 += val;
			sd->matk2 += val;
		}
		break;
	case SP_DEF1:
		if(sd->state.lr_flag != 2)
			sd->def+=val;
		break;
	case SP_MDEF1:
		if(sd->state.lr_flag != 2)
			sd->mdef+=val;
		break;
	case SP_MDEF2:
		if(sd->state.lr_flag != 2)
			sd->mdef+=val;
		break;
	case SP_HIT:
		if(sd->state.lr_flag != 2)
			sd->hit+=val;
		else
			sd->arrow_hit+=val;
		break;
	case SP_FLEE1:
		if(sd->state.lr_flag != 2)
			sd->flee+=val;
		break;
	case SP_FLEE2:
		if(sd->state.lr_flag != 2)
			sd->flee2+=val*10;
		break;
	case SP_CRITICAL:
		if(sd->state.lr_flag != 2)
			sd->critical+=val*10;
		else
			sd->arrow_cri += val*10;
		break;
	case SP_ATKELE:
		if(!sd->state.lr_flag)
			sd->atk_ele=val;
		else if(sd->state.lr_flag == 1)
			sd->atk_ele_=val;
		else if(sd->state.lr_flag == 2)
			sd->arrow_ele=val;
		break;
	case SP_DEFELE:
		if(sd->state.lr_flag != 2)
			sd->def_ele=val;
		break;
	case SP_MAXHP:
		if(sd->state.lr_flag != 2)
			sd->status.max_hp+=val;
		break;
	case SP_MAXSP:
		if(sd->state.lr_flag != 2)
			sd->status.max_sp+=val;
		break;
	case SP_CASTRATE:
		if(sd->state.lr_flag != 2)
			sd->castrate+=val;
		break;
	case SP_MAXHPRATE:
		if(sd->state.lr_flag != 2)
			sd->hprate+=val;
		break;
	case SP_MAXSPRATE:
		if(sd->state.lr_flag != 2)
			sd->sprate+=val;
		break;
	case SP_SPRATE:
		if(sd->state.lr_flag != 2)
			sd->dsprate+=val;
		break;
	case SP_ATTACKRANGE:
		if(!sd->state.lr_flag)
			sd->attackrange += val;
		else if(sd->state.lr_flag == 1)
			sd->attackrange_ += val;
		else if(sd->state.lr_flag == 2)
			sd->arrow_range += val;
		break;
	case SP_ADD_SPEED:
		if(sd->state.lr_flag != 2)
			sd->speed -= val;
		break;
	case SP_SPEED_RATE:
		if(sd->state.lr_flag != 2) {
			if(sd->speed_rate > 100-val)
				sd->speed_rate = 100-val;
		}
		break;
	case SP_SPEED_ADDRATE:
		if(sd->state.lr_flag != 2)
			sd->speed_add_rate = sd->speed_add_rate * (100-val)/100;
		break;
	case SP_ASPD:
		if(sd->state.lr_flag != 2)
			sd->aspd -= val*10;
		break;
	case SP_ASPD_RATE:
		if(sd->state.lr_flag != 2) {
			if(sd->aspd_rate > 100-val)
				sd->aspd_rate = 100-val;
		}
		break;
	case SP_ASPD_ADDRATE:
		if(sd->state.lr_flag != 2)
			sd->aspd_add_rate = sd->aspd_add_rate * (100-val)/100;
		break;
	case SP_HP_RECOV_RATE:
		if(sd->state.lr_flag != 2)
			sd->hprecov_rate += val;
		break;
	case SP_SP_RECOV_RATE:
		if(sd->state.lr_flag != 2)
			sd->sprecov_rate += val;
		break;
	case SP_CRITICAL_DEF:
		if(sd->state.lr_flag != 2)
			sd->critical_def += val;
		break;
	case SP_NEAR_ATK_DEF:
		if(sd->state.lr_flag != 2)
			sd->near_attack_def_rate += val;
		break;
	case SP_LONG_ATK_DEF:
		if(sd->state.lr_flag != 2)
			sd->long_attack_def_rate += val;
		break;
	case SP_DOUBLE_RATE:
		if(sd->state.lr_flag == 0 && sd->double_rate < val)
			sd->double_rate = val;
		break;
	case SP_DOUBLE_ADD_RATE:
		if(sd->state.lr_flag == 0)
			sd->double_add_rate += val;
		break;
	case SP_MATK_RATE:
		if(sd->state.lr_flag != 2)
			sd->matk_rate += val;
		break;
	case SP_IGNORE_DEF_ELE:
		if(!sd->state.lr_flag)
			sd->ignore_def_ele |= 1<<val;
		else if(sd->state.lr_flag == 1)
			sd->ignore_def_ele_ |= 1<<val;
		break;
	case SP_IGNORE_DEF_RACE:
		if(!sd->state.lr_flag)
			sd->ignore_def_race |= 1<<val;
		else if(sd->state.lr_flag == 1)
			sd->ignore_def_race_ |= 1<<val;
		break;
	case SP_ATK_RATE:
		if(sd->state.lr_flag != 2)
			sd->atk_rate += val;
		break;
	case SP_MAGIC_ATK_DEF:
		if(sd->state.lr_flag != 2)
			sd->magic_def_rate += val;
		break;
	case SP_MISC_ATK_DEF:
		if(sd->state.lr_flag != 2)
			sd->misc_def_rate += val;
		break;
	case SP_IGNORE_MDEF_ELE:
		if(sd->state.lr_flag != 2)
			sd->ignore_mdef_ele |= 1<<val;
		break;
	case SP_IGNORE_MDEF_RACE:
		if(sd->state.lr_flag != 2)
			sd->ignore_mdef_race |= 1<<val;
		break;
	case SP_PERFECT_HIT_RATE:
		if(sd->state.lr_flag != 2 && sd->perfect_hit < val)
			sd->perfect_hit = val;
		break;
	case SP_PERFECT_HIT_ADD_RATE:
		if(sd->state.lr_flag != 2)
			sd->perfect_hit_add += val;
		break;
	case SP_CRITICAL_RATE:
		if(sd->state.lr_flag != 2)
			sd->critical_rate+=val;
		break;
	case SP_GET_ZENY_NUM:
		if(sd->state.lr_flag != 2 && sd->get_zeny_num < val)
			sd->get_zeny_num = val;
		break;
	case SP_ADD_GET_ZENY_NUM:
		if(sd->state.lr_flag != 2)
			sd->get_zeny_add_num += val;
		break;
	case SP_DEF_RATIO_ATK_ELE:
		if(!sd->state.lr_flag)
			sd->def_ratio_atk_ele |= 1<<val;
		else if(sd->state.lr_flag == 1)
			sd->def_ratio_atk_ele_ |= 1<<val;
		break;
	case SP_DEF_RATIO_ATK_RACE:
		if(!sd->state.lr_flag)
			sd->def_ratio_atk_race |= 1<<val;
		else if(sd->state.lr_flag == 1)
			sd->def_ratio_atk_race_ |= 1<<val;
		break;
	case SP_HIT_RATE:
		if(sd->state.lr_flag != 2)
			sd->hit_rate += val;
		break;
	case SP_FLEE_RATE:
		if(sd->state.lr_flag != 2)
			sd->flee_rate += val;
		break;
	case SP_FLEE2_RATE:
		if(sd->state.lr_flag != 2)
			sd->flee2_rate += val;
		break;
	case SP_DEF_RATE:
		if(sd->state.lr_flag != 2)
			sd->def_rate += val;
		break;
	case SP_DEF2_RATE:
		if(sd->state.lr_flag != 2)
			sd->def2_rate += val;
		break;
	case SP_MDEF_RATE:
		if(sd->state.lr_flag != 2)
			sd->mdef_rate += val;
		break;
	case SP_MDEF2_RATE:
		if(sd->state.lr_flag != 2)
			sd->mdef2_rate += val;
		break;
	case SP_RESTART_FULL_RECORVER:
		if(sd->state.lr_flag != 2)
			sd->special_state.restart_full_recover = 1;
		break;
	case SP_NO_CASTCANCEL:
		if(sd->state.lr_flag != 2)
			sd->special_state.no_castcancel = 1;
		break;
	case SP_NO_CASTCANCEL2:
		if(sd->state.lr_flag != 2)
			sd->special_state.no_castcancel2 = 1;
		break;
	case SP_NO_SIZEFIX:
		if(sd->state.lr_flag != 2)
			sd->special_state.no_sizefix = 1;
		break;
	case SP_NO_MAGIC_DAMAGE:
		if(sd->state.lr_flag != 2)
			sd->special_state.no_magic_damage = 1;
		break;
	case SP_NO_WEAPON_DAMAGE:
		if(sd->state.lr_flag != 2)
			sd->special_state.no_weapon_damage = 1;
		break;
	case SP_NO_GEMSTONE:
		if(sd->state.lr_flag != 2)
			sd->special_state.no_gemstone = 1;
		break;
	case SP_INFINITE_ENDURE:
		if(sd->state.lr_flag != 2)
			sd->special_state.infinite_endure = 1;
		break;
	case SP_UNBREAKABLE_WEAPON:
		if(sd->state.lr_flag != 2)
			sd->unbreakable_equip |= EQP_WEAPON;
		break;
	case SP_UNBREAKABLE_ARMOR:
		if(sd->state.lr_flag != 2)
			sd->unbreakable_equip |= EQP_ARMOR;
		break;
	case SP_UNBREAKABLE_HELM:
		if(sd->state.lr_flag != 2)
			sd->unbreakable_equip |= EQP_HELM;
		break;
	case SP_UNBREAKABLE_SHIELD:
		if(sd->state.lr_flag != 2)
			sd->unbreakable_equip |= EQP_SHIELD;
		break;
	case SP_SPLASH_RANGE:
		if(sd->state.lr_flag != 2 && sd->splash_range < val)
			sd->splash_range = val;
		break;
	case SP_SPLASH_ADD_RANGE:
		if(sd->state.lr_flag != 2)
			sd->splash_add_range += val;
		break;
	case SP_SHORT_WEAPON_DAMAGE_RETURN:
		if(sd->state.lr_flag != 2)
			sd->short_weapon_damage_return += val;
		break;
	case SP_LONG_WEAPON_DAMAGE_RETURN:
		if(sd->state.lr_flag != 2)
			sd->long_weapon_damage_return += val;
		break;
	case SP_BREAK_WEAPON_RATE:
		if(sd->state.lr_flag != 2)
			sd->break_weapon_rate += val;
		break;
	case SP_BREAK_ARMOR_RATE:
		if(sd->state.lr_flag != 2)
			sd->break_armor_rate += val;
		break;
	case SP_ADD_STEAL_RATE:
		if(sd->state.lr_flag != 2)
			sd->add_steal_rate += val;
		break;
	case SP_CRITICAL_DAMAGE:
		if(sd->state.lr_flag != 2)
			sd->critical_damage += val;
		break;
	case SP_HP_RECOV_STOP:
		if(sd->state.lr_flag != 2)
			sd->hp_recov_stop = 1;
		break;
	case SP_SP_RECOV_STOP:
		if(sd->state.lr_flag != 2)
			sd->sp_recov_stop = 1;
		break;
	case SP_BONUS_DAMAGE:
		if(sd->state.lr_flag != 2)
			sd->bonus_damage += val;
		break;
	case SP_HP_PENALTY_UNRIG:
		if(sd->state.lr_flag != 2)
			sd->hp_penalty_unrig_value[current_equip_item_index] += val;

		break;
	case SP_SP_PENALTY_UNRIG:
		if(sd->state.lr_flag != 2)
			sd->sp_penalty_unrig_value[current_equip_item_index] += val;
		break;
	case SP_HP_RATE_PENALTY_UNRIG:
		if(sd->state.lr_flag != 2)
			sd->hp_rate_penalty_unrig[current_equip_item_index] += val;

		break;
	case SP_SP_RATE_PENALTY_UNRIG:
		if(sd->state.lr_flag != 2)
			sd->sp_rate_penalty_unrig[current_equip_item_index] += val;
		break;
	case	SP_MOB_CLASS_CHANGE:
		sd->mob_class_change_rate += val;
		break;
	case	SP_BAN_AUTO_SPELLL:
		if(sd->state.lr_flag != 2)
			pc_bonus_ban_autospell(sd,val);
		break;
	case SP_CURSE_BY_MURAMASA:
		if(sd->state.lr_flag != 2)
			sd->curse_by_muramasa += val;
		break;
	case SP_LOSS_EQUIP_WHEN_DIE:
		if(sd->state.lr_flag != 2){
			sd->loss_equip_rate_when_die[current_equip_item_index] += val;
			sd->loss_equip_flag |= 0x0001;
		}
		break;
	case SP_LOSS_EQUIP_WHEN_ATTACK:
		if(sd->state.lr_flag != 2){
			sd->loss_equip_rate_when_attack[current_equip_item_index] += val;
			sd->loss_equip_flag |= 0x0010;
		}
		break;
	case SP_LOSS_EQUIP_WHEN_HIT:
		if(sd->state.lr_flag != 2){
			sd->loss_equip_rate_when_hit[current_equip_item_index] += val;
			sd->loss_equip_flag |= 0x0020;
		}
		break;
	case SP_BREAK_MYEQUIP_WHEN_ATTACK:
		if(sd->state.lr_flag != 2)
		{
			sd->break_myequip_rate_when_attack[current_equip_item_index] += val;
			sd->loss_equip_flag |= 0x0100;
		}
		break;
	case SP_BREAK_MYEQUIP_WHEN_HIT:
		if(sd->state.lr_flag != 2){
			sd->break_myequip_rate_when_hit[current_equip_item_index] += val;
			sd->loss_equip_flag |= 0x1000;
		}
		break;
	case SP_MAGIC_DAMAGE_RETURN:
		if(sd->state.lr_flag != 2)
			sd->magic_damage_return += val;
		break;
	case SP_ADD_SHORT_WEAPON_DAMAGE:
		if(sd->state.lr_flag != 2)
			sd->short_weapon_damege_rate += val;
		break;
	case SP_ADD_LONG_WEAPON_DAMAGE:
		if(sd->state.lr_flag != 2)
			sd->long_weapon_damege_rate += val;
		break;
	default:
		if(battle_config.error_log)
			printf("pc_bonus: unknown type %d %d !\n",type,val);
		break;
	}
	return 0;
}

/*==========================================
 * �� ���i�ɂ��\�͓��̃{�[�i�X�ݒ�
 *------------------------------------------
 */
int pc_bonus2(struct map_session_data *sd,int type,int type2,int val)
{
	int i;

	nullpo_retr(0, sd);

	switch(type){
	case SP_ADDELE:
		if(!sd->state.lr_flag)
			sd->addele[type2]+=val;
		else if(sd->state.lr_flag == 1)
			sd->addele_[type2]+=val;
		else if(sd->state.lr_flag == 2)
			sd->arrow_addele[type2]+=val;
		break;
	case SP_ADDRACE:
		if(!sd->state.lr_flag)
			sd->addrace[type2]+=val;
		else if(sd->state.lr_flag == 1)
			sd->addrace_[type2]+=val;
		else if(sd->state.lr_flag == 2)
			sd->arrow_addrace[type2]+=val;
		break;
	case SP_ADDSIZE:
		if(!sd->state.lr_flag)
			sd->addsize[type2]+=val;
		else if(sd->state.lr_flag == 1)
			sd->addsize_[type2]+=val;
		else if(sd->state.lr_flag == 2)
			sd->arrow_addsize[type2]+=val;
		break;
	case SP_SUBELE:
		if(sd->state.lr_flag != 2)
			sd->subele[type2]+=val;
		break;
	case SP_SUBRACE:
		if(sd->state.lr_flag != 2)
			sd->subrace[type2]+=val;
		break;
	case SP_ADDEFF:
		if(sd->state.lr_flag != 2)
			sd->addeff[type2]+=val;
		else
			sd->arrow_addeff[type2]+=val;
		break;
	case SP_ADDEFF2:
		if(sd->state.lr_flag != 2)
			sd->addeff2[type2]+=val;
		else
			sd->arrow_addeff2[type2]+=val;
		break;
	case SP_RESEFF:
		if(sd->state.lr_flag != 2)
			sd->reseff[type2]+=val;
		break;
	case SP_MAGIC_ADDELE:
		if(sd->state.lr_flag != 2)
			sd->magic_addele[type2]+=val;
		break;
	case SP_MAGIC_ADDRACE:
		if(sd->state.lr_flag != 2)
			sd->magic_addrace[type2]+=val;
		break;
	case SP_MAGIC_SUBRACE:
		if(sd->state.lr_flag != 2)
			sd->magic_subrace[type2]+=val;
		break;
	case SP_ADD_DAMAGE_CLASS:
		if(!sd->state.lr_flag) {
			for(i=0;i<sd->add_damage_class_count;i++) {
				if(sd->add_damage_classid[i] == type2) {
					sd->add_damage_classrate[i] += val;
					break;
				}
			}
			if(i >= sd->add_damage_class_count && sd->add_damage_class_count < 10) {
				sd->add_damage_classid[sd->add_damage_class_count] = type2;
				sd->add_damage_classrate[sd->add_damage_class_count] += val;
				sd->add_damage_class_count++;
			}
		}
		else if(sd->state.lr_flag == 1) {
			for(i=0;i<sd->add_damage_class_count_;i++) {
				if(sd->add_damage_classid_[i] == type2) {
					sd->add_damage_classrate_[i] += val;
					break;
				}
			}
			if(i >= sd->add_damage_class_count_ && sd->add_damage_class_count_ < 10) {
				sd->add_damage_classid_[sd->add_damage_class_count_] = type2;
				sd->add_damage_classrate_[sd->add_damage_class_count_] += val;
				sd->add_damage_class_count_++;
			}
		}
		break;
	case SP_ADD_MAGIC_DAMAGE_CLASS:
		if(sd->state.lr_flag != 2) {
			for(i=0;i<sd->add_magic_damage_class_count;i++) {
				if(sd->add_magic_damage_classid[i] == type2) {
					sd->add_magic_damage_classrate[i] += val;
					break;
				}
			}
			if(i >= sd->add_magic_damage_class_count && sd->add_magic_damage_class_count < 10) {
				sd->add_magic_damage_classid[sd->add_magic_damage_class_count] = type2;
				sd->add_magic_damage_classrate[sd->add_magic_damage_class_count] += val;
				sd->add_magic_damage_class_count++;
			}
		}
		break;
	case SP_ADD_DEF_CLASS:
		if(sd->state.lr_flag != 2) {
			for(i=0;i<sd->add_def_class_count;i++) {
				if(sd->add_def_classid[i] == type2) {
					sd->add_def_classrate[i] += val;
					break;
				}
			}
			if(i >= sd->add_def_class_count && sd->add_def_class_count < 10) {
				sd->add_def_classid[sd->add_def_class_count] = type2;
				sd->add_def_classrate[sd->add_def_class_count] += val;
				sd->add_def_class_count++;
			}
		}
		break;
	case SP_ADD_MDEF_CLASS:
		if(sd->state.lr_flag != 2) {
			for(i=0;i<sd->add_mdef_class_count;i++) {
				if(sd->add_mdef_classid[i] == type2) {
					sd->add_mdef_classrate[i] += val;
					break;
				}
			}
			if(i >= sd->add_mdef_class_count && sd->add_mdef_class_count < 10) {
				sd->add_mdef_classid[sd->add_mdef_class_count] = type2;
				sd->add_mdef_classrate[sd->add_mdef_class_count] += val;
				sd->add_mdef_class_count++;
			}
		}
		break;
	case SP_HP_DRAIN_RATE:
		if(!sd->state.lr_flag) {
			sd->hp_drain_rate += type2;
			sd->hp_drain_per += val;
		}
		else if(sd->state.lr_flag == 1) {
			sd->hp_drain_rate_ += type2;
			sd->hp_drain_per_ += val;
		}
		break;
	case SP_HP_DRAIN_VALUE:
		if(!sd->state.lr_flag) {
			sd->hp_drain_rate += type2;
			sd->hp_drain_value += val;
		}
		else if(sd->state.lr_flag == 1) {
			sd->hp_drain_rate_ += type2;
			sd->hp_drain_value_ += val;
		}
		break;
	case SP_SP_DRAIN_RATE:
		if(!sd->state.lr_flag) {
			sd->sp_drain_rate += type2;
			sd->sp_drain_per += val;
		}
		else if(sd->state.lr_flag == 1) {
			sd->sp_drain_rate_ += type2;
			sd->sp_drain_per_ += val;
		}
		break;
	case SP_SP_DRAIN_VALUE:
		if(!sd->state.lr_flag) {
			sd->sp_drain_rate += type2;
			sd->sp_drain_value += val;
		}
		else if(sd->state.lr_flag == 1) {
			sd->sp_drain_rate_ += type2;
			sd->sp_drain_value_ += val;
		}
		break;
	case SP_WEAPON_COMA_ELE:
		if(sd->state.lr_flag != 2)
			sd->weapon_coma_ele[type2] += val;
		break;
	case SP_WEAPON_COMA_RACE:
		if(sd->state.lr_flag != 2)
			sd->weapon_coma_race[type2] += val;
		break;
	case SP_WEAPON_COMA_ELE2:
		if(sd->state.lr_flag != 2)
			sd->weapon_coma_ele2[type2] += val;
		break;
	case SP_WEAPON_COMA_RACE2:
		if(sd->state.lr_flag != 2)
			sd->weapon_coma_race2[type2] += val;
		break;
	case SP_WEAPON_ATK:
		if(sd->state.lr_flag != 2)
			sd->weapon_atk[type2]+=val;
		break;
	case SP_WEAPON_ATK_RATE:
		if(sd->state.lr_flag != 2)
			sd->weapon_atk_rate[type2]+=val;
		break;
	case SP_CRITICALRACE:
		if(type2<0 || type2>10) break;
		if(type2 == 10){
			for(i=0;i<10;i++)
			{
				sd->critical_race[i] += val*10;	
			}
		}else{
				sd->critical_race[type2] += val*10;
		}
		break;
	case SP_CRITICALRACERATE:
		if(type2<0 || type2>10) break;
		if(type2 == 10){
			for(i=0;i<10;i++)
			{
				sd->critical_race_rate[i] += val*10;	
			}
		}else{
				sd->critical_race_rate[type2] += val*10;
		}
		break;
	case SP_ADDREVEFF:
		if(type2<0 || type2>10) break;
		sd->addreveff[type2] += val;
		sd->addreveff_flag = 1;
		break;
	case SP_SUB_SIZE:
		if(type2>=0 && type2<3)
			sd->subsize[type2] += val;
		break;
	case SP_MAGIC_SUB_SIZE:
		if(type2>=0 && type2<3)
			sd->magic_subsize[type2] += val;
		break;
	case SP_EXP_RATE:
		if(type2<0 || type2>10) break;
		if(type2 == 10){
			for(i=0;i<10;i++)
			{
				sd->exp_rate[i] += val;	
			}
		}else{
				sd->exp_rate[type2] += val;
		}
		break;
	case SP_JOB_RATE:
		if(type2<0 || type2>10) break;
		if(type2 == 10){
			for(i=0;i<10;i++)
			{
				sd->job_rate[i] += val;	
			}
		}else{
				sd->job_rate[type2] += val;
		}
		break;
	case	SP_ADD_SKILL_DAMAGE_RATE:
		//update
		for(i=0;i<sd->skill_dmgup.count;i++)
		{
			if(sd->skill_dmgup.id[i] == type2)
			{
				sd->skill_dmgup.rate[i] += val;
				return 0;	
			}	
		}
		//full
		if(sd->skill_dmgup.count == MAX_SKILL_DAMAGE_UP)
		 	break;
		//add
		sd->skill_dmgup.id[sd->skill_dmgup.count] = type2;
		sd->skill_dmgup.rate[sd->skill_dmgup.count] = val;
		sd->skill_dmgup.count++;
		break;
	case SP_ADD_GROUP:
		if(type2<0 || type2>=MAX_MOBGROUP)
		 	break;
		if(!sd->state.lr_flag)
			sd->addgroup[type2]+=val;
		else if(sd->state.lr_flag == 1)
			sd->addgroup_[type2]+=val;
		else if(sd->state.lr_flag == 2)
			sd->arrow_addgroup[type2]+=val;
		break;
	case SP_SUB_GROUP:
			if(type2<0 || type2>=MAX_MOBGROUP)
			 	break;
			sd->subgroup[type2] += val;
		break;
	case SP_HP_PENALTY_TIME:
			sd->hp_penalty_time = type2;
			sd->hp_penalty_value = val;
		break;
	case SP_SP_PENALTY_TIME:
			sd->sp_penalty_time = type2;
			sd->sp_penalty_value = val;
		break;
	case	SP_AUTO_SPELL_RATE:
		 pc_bonus_add_autospell_rate(sd,type2,val);
		break;
	case	SP_ADD_SKILL_BLOW:
		//update
		for(i=0;i<sd->skill_blow.count;i++)
		{
			if(sd->skill_blow.id[i] == type2)
			{
				if(sd->skill_blow.grid[i] < val)
					sd->skill_blow.grid[i] = val;
				return 0;	
			}	
		}
		//full
		if(sd->skill_blow.count == MAX_SKILL_BLOW)
		 	break;
		//add
		sd->skill_blow.id[sd->skill_blow.count] = type2;
		sd->skill_blow.grid[sd->skill_blow.count] = val;
		sd->skill_blow.count++;
		break;
	case SP_ADD_ITEMHEAL_RATE_GROUP:
		if(type2< 0 || MAX_ITEMGROUP<type2)
			 break;
		sd->itemheal_rate[type2] += val;
		break;
	case SP_HPVANISH:
		sd->hp_vanish_rate +=type2;
		sd->hp_vanish_per  +=val;
		break;
	case SP_SPVANISH:
		sd->sp_vanish_rate +=type2;
		sd->sp_vanish_per  +=val;
		break;
	case SP_RAISE:
		sd->autoraise.hp_per = val;
		sd->autoraise.sp_per = 0;
		sd->autoraise.rate 	 = type2;
		sd->autoraise.flag	 = 0;
		break;
	case SP_BREAK_MYEQUIP_WHEN_ATTACK:
		sd->break_myequip_rate_when_attack[type2] += val;
		sd->loss_equip_flag |= 0x0100;
		break;
	case SP_BREAK_MYEQUIP_WHEN_HIT:
		sd->break_myequip_rate_when_hit[type2] += val;
		sd->loss_equip_flag |= 0x1000;
		break;
	default:
		if(battle_config.error_log)
			printf("pc_bonus2: unknown type %d %d %d!\n",type,type2,val);
		break;
	}

	return 0;
}

int pc_bonus3(struct map_session_data *sd,int type,int type2,int type3,int val)
{
	int i;
	switch(type){
	case SP_ADD_MONSTER_DROP_ITEM:
		if(sd->state.lr_flag != 2) {
			for(i=0;i<sd->monster_drop_item_count;i++) {
				if(sd->monster_drop_itemid[i] == type2) {
					sd->monster_drop_race[i] |= 1<<type3;
					if(sd->monster_drop_itemrate[i] < val)
						sd->monster_drop_itemrate[i] = val;
					break;
				}
			}
			if(i >= sd->monster_drop_item_count && sd->monster_drop_item_count < 10) {
				sd->monster_drop_itemid[sd->monster_drop_item_count] = type2;
				sd->monster_drop_race[sd->monster_drop_item_count] |= 1<<type3;
				sd->monster_drop_itemrate[sd->monster_drop_item_count] = val;
				sd->monster_drop_item_count++;
			}
		}
		break;
	case SP_DEF_HP_DRAIN_VALUE:
		if(sd->state.lr_flag != 2){
			if(type2<0 || type2>10) break;
			if(type2 == 10)
			{
				for(i=0;i<10;i++)
				{
					sd->hp_drain_rate_race[i]   += type3;
					sd->hp_drain_value_race[i] += val;
				}
			}else{
				
				sd->hp_drain_rate_race[type2]   += type3;
				sd->hp_drain_value_race[type2] += val;
				
			}
		}
		break;
	case SP_DEF_SP_DRAIN_VALUE:
		if(sd->state.lr_flag != 2){
			if(type2<0 || type2>10) break;
			if(type2 == 10)
			{
				for(i=0;i<10;i++)
				{
					sd->sp_drain_rate_race[i]   += type3;
					sd->sp_drain_value_race[i] += val;
				}
			}else{
				sd->sp_drain_rate_race[type2]   += type3;
				sd->sp_drain_value_race[type2] += val;
				
			}
		}
		break;
		
	case SP_AUTOSPELL:
		if(sd->state.lr_flag == 2)
			break;
		if(battle_config.expand_autospell)
		{
			pc_bonus_autospell(sd,type2,type3,val,EAS_FLUCT|EAS_ENABLE|EAS_ATTACK);
		}else{
			
			sd->autospell.id[0] = type2;
			sd->autospell.lv[0] = type3;
			sd->autospell.rate[0] = val;
			sd->autospell.flag[0] = EAS_FLUCT|EAS_ENABLE|EAS_ATTACK;
			sd->autospell.count = 2;
		}
		break;
	case SP_AUTOSPELL2:
		if(sd->state.lr_flag == 2)
			break;
		if(battle_config.expand_autospell)
		{
			pc_bonus_autospell(sd,type2,type3,val,EAS_ENABLE|EAS_ATTACK|EAS_USEBETTER);
		}else{
			
			sd->autospell.id[0] = type2;
			sd->autospell.lv[0] = type3;
			sd->autospell.rate[0] = val;
			sd->autospell.flag[0] = EAS_ENABLE|EAS_ATTACK|EAS_USEBETTER;
			sd->autospell.count = 2;
		}
		break;
	case SP_AUTOSELFSPELL:
		if(sd->state.lr_flag == 2)
			break;
		if(battle_config.expand_autospell)
		{
			pc_bonus_autospell(sd,type2,type3,val,EAS_SELF|EAS_ENABLE|EAS_ATTACK);
		}else{
			sd->autospell.id[0] = type2;
			sd->autospell.lv[0] = type3;
			sd->autospell.rate[0] = val;
			sd->autospell.flag[0] = EAS_SELF|EAS_ENABLE|EAS_ATTACK;
			sd->autospell.count = 2;
		}
		break;
	case SP_AUTOSELFSPELL2:
		if(sd->state.lr_flag == 2)
			break;
		if(battle_config.expand_autospell)
		{
			pc_bonus_autospell(sd,type2,type3,val,EAS_ENABLE|EAS_SELF|EAS_ATTACK|EAS_USEBETTER);
		}else{
			sd->autospell.id[0] = type2;
			sd->autospell.lv[0] = type3;
			sd->autospell.rate[0] = val;
			sd->autospell.flag[0] = EAS_ENABLE|EAS_SELF|EAS_ATTACK|EAS_USEBETTER;
			sd->autospell.count = 2;
		}
		break;
	case SP_REVAUTOSPELL://�����p�I�[�g�X�y��
		if(sd->state.lr_flag == 2)
			break;
		if(battle_config.expand_autospell)
		{
			pc_bonus_autospell(sd,type2,type3,val,EAS_ENABLE|EAS_REVENGE);
		}else{
			sd->autospell.id[1] = type2;
			sd->autospell.lv[1] = type3;
			sd->autospell.rate[1] = val;
			sd->autospell.flag[1] = EAS_ENABLE|EAS_REVENGE;
			sd->autospell.count = 2;
		}
		break;
	case SP_REVAUTOSPELL2:
		if(sd->state.lr_flag == 2)
			break;
		if(battle_config.expand_autospell)
		{
			pc_bonus_autospell(sd,type2,type3,val,EAS_ENABLE|EAS_REVENGE|EAS_USEBETTER);
		}else{
			
			sd->autospell.id[1] = type2;
			sd->autospell.lv[1] = type3;
			sd->autospell.rate[1] = val;
			sd->autospell.flag[1] = EAS_ENABLE|EAS_REVENGE|EAS_USEBETTER;
			sd->autospell.count = 2;
		}
		break;
	case SP_REVAUTOSELFSPELL:
		if(sd->state.lr_flag == 2)
			break;
		if(battle_config.expand_autospell)
		{
			pc_bonus_autospell(sd,type2,type3,val,EAS_ENABLE|EAS_SELF|EAS_REVENGE);
		}else{
			
			sd->autospell.id[1] = type2;
			sd->autospell.lv[1] = type3;
			sd->autospell.rate[1] = val;
			sd->autospell.flag[1] = EAS_ENABLE|EAS_SELF|EAS_REVENGE;
			sd->autospell.count = 2;
		}
		break;
	case SP_REVAUTOSELFSPELL2:
		if(sd->state.lr_flag == 2)
			break;
		if(battle_config.expand_autospell)
		{
			pc_bonus_autospell(sd,type2,type3,val,EAS_ENABLE|EAS_SELF|EAS_REVENGE|EAS_USEBETTER);
		}else{
			sd->autospell.id[1] = type2;
			sd->autospell.lv[1] = type3;
			sd->autospell.rate[1] = val;
			sd->autospell.flag[1] = EAS_ENABLE|EAS_SELF|EAS_REVENGE|EAS_USEBETTER;
			sd->autospell.count = 2;
		}
		break;
	case SP_RAISE:
		sd->autoraise.hp_per = type3;
		sd->autoraise.sp_per = val;
		sd->autoraise.rate 	 = type2;
		sd->autoraise.flag	 = 1;
		break;
	default:
		if(battle_config.error_log)
			printf("pc_bonus3: unknown type %d %d %d %d!\n",type,type2,type3,val);
		break;
	}

	return 0;
}

/*==========================================
 * �I�[�g�X�y��
 *------------------------------------------
 */
int pc_bonus_autospell(struct map_session_data* sd,int skillid,int skilllv,int rate, short flag)
{
	int i;
	nullpo_retr(0, sd);
	
	if(sd->bl.type != BL_PC)
		return 0;

	if(!battle_config.allow_same_autospell){
		for(i=0;i<sd->autospell.count;i++){
			if(sd->autospell.card_id[i] == current_equip_card_id)
				return 0;
		}
	}

	//��t
	if(sd->autospell.count == MAX_BONUS_AUTOSPELL)
	 	return 0;

	//���ɒǉ�
	sd->autospell.id[sd->autospell.count] = skillid;
	sd->autospell.lv[sd->autospell.count] = skilllv;
	sd->autospell.rate[sd->autospell.count] = rate;
	sd->autospell.flag[sd->autospell.count] = flag|EAS_ENABLE;
	sd->autospell.card_id[sd->autospell.count] = current_equip_card_id;
	sd->autospell.count++;
	
	return 0;	
}

/*==========================================
 * �I�[�g�X�y���̔������㏸
 *------------------------------------------
 */
int pc_bonus_add_autospell_rate(struct map_session_data* sd,int skillid,int val)
{
	int i;
	nullpo_retr(0, sd);
	
	if(sd->bl.type != BL_PC)
		return 0;
	
	for(i=0;i<sd->autospell.add_count;i++)
	{
		if(sd->autospell.add_id[i] == skillid)
		{
			sd->autospell.add_rate[i] += val; //���Z
			return 0;
		}
	}
	
	//��t
	if(sd->autospell.add_count == MAX_ADDRATE_AUTOSPELL)
	 	return 0;

	//�V�K�Ȃ���ɒǉ�
	sd->autospell.add_id[sd->autospell.add_count] 	= skillid;
	sd->autospell.add_rate[sd->autospell.add_count] = val;
	sd->autospell.add_count++;
	return 0;	
}

/*==========================================
 * �֎~�I�[�g�X�y��
 *------------------------------------------
 */
int pc_bonus_ban_autospell(struct map_session_data* sd,int skillid)
{
	int i;
	nullpo_retr(0, sd);
	
	if(sd->bl.type != BL_PC)
		return 0;
	
	for(i=0;i<sd->autospell.ban_count;i++)
	{
		if(sd->autospell.ban[i] == skillid)
			return 0;
	}
	//��t
	if(sd->autospell.ban_count == MAX_BAN_AUTOSPELL)
	 	return 0;

	//�V�K�Ȃ���ɒǉ�
	sd->autospell.ban[sd->autospell.ban_count] 	= skillid;
	sd->autospell.ban_count++;
	return 0;	
}

/*==========================================
 * �X�N���v�g�ɂ��X�L������
 *------------------------------------------
 */
int pc_skill(struct map_session_data *sd,int id,int level,int flag)
{
	nullpo_retr(0, sd);

	if(level>MAX_SKILL_LEVEL){
		if(battle_config.error_log)
			printf("support card skill only!\n");
		return 0;
	}
	if(!flag && (sd->status.skill[id].id == id || level == 0)){	// �N�G�X�g�����Ȃ炱���ŏ������m�F���đ��M����
		sd->status.skill[id].lv=level;
		status_calc_pc(sd,0);
		clif_skillinfoblock(sd);
	}
	else if(sd->status.skill[id].lv < level){	// �o�����邪lv���������Ȃ�
		if(sd->status.skill[id].id==id)
			sd->status.skill[id].flag=sd->status.skill[id].lv+2;	// lv���L��
		else {
			sd->status.skill[id].id=id;
			sd->status.skill[id].flag=1;	// card�X�L���Ƃ���
		}
		sd->status.skill[id].lv=level;
	}

	return 0;
}

/*==========================================
 * �J�[�h�}��
 *------------------------------------------
 */
int pc_insert_card(struct map_session_data *sd,int idx_card,int idx_equip)
{
	nullpo_retr(0, sd);

	if(idx_card >= 0 && idx_card < MAX_INVENTORY && idx_equip >= 0 && idx_equip < MAX_INVENTORY && sd->inventory_data[idx_card]) {
		int i;
		int nameid=sd->status.inventory[idx_equip].nameid;
		int cardid=sd->status.inventory[idx_card].nameid;
		int ep=sd->inventory_data[idx_card]->equip;

		if( nameid <= 0 || sd->inventory_data[idx_equip] == NULL ||
			(sd->inventory_data[idx_equip]->type!=4 && sd->inventory_data[idx_equip]->type!=5)||	// �� ������Ȃ�
			( sd->status.inventory[idx_equip].identify==0 ) ||		// ���Ӓ�
			( sd->status.inventory[idx_equip].card[0]==0x00ff) ||		// ��������
			( sd->status.inventory[idx_equip].card[0]==0x00fe) ||
			( (sd->inventory_data[idx_equip]->equip&ep)==0 ) ||					// �� �����Ⴂ
			( sd->inventory_data[idx_equip]->type==4 && ep==32) ||			// �� �蕐��Ə��J�[�h
			(sd->inventory_data[idx_card]->type!=6)|| // Prevent Hack [Ancyker]
			( sd->status.inventory[idx_equip].card[0]==(short)0xff00) || sd->status.inventory[idx_equip].equip){

			clif_insert_card(sd,idx_equip,idx_card,1);
			return 0;
		}
		for(i=0;i<sd->inventory_data[idx_equip]->slot;i++){
			if( sd->status.inventory[idx_equip].card[i] == 0){
			// �󂫃X���b�g���������̂ō�������
				sd->status.inventory[idx_equip].card[i]=cardid;

			// �J�[�h�͌��炷
				clif_insert_card(sd,idx_equip,idx_card,0);
				pc_delitem(sd,idx_card,1,1);
				return 0;
			}
		}
	}
	else
		clif_insert_card(sd,idx_equip,idx_card,1);

	return 0;
}

//
// �A�C�e����
//


/*==========================================
 * �X�L���ɂ�锃���l�C��
 *------------------------------------------
 */
int pc_modifybuyvalue(struct map_session_data *sd,int orig_value)
{
	int skill,val = orig_value,rate1 = 0,rate2 = 0;
	if((skill=pc_checkskill(sd,MC_DISCOUNT))>0)	// �f�B�X�J�E���g
		rate1 = 5+skill*2-((skill==10)? 1:0);
	if((skill=pc_checkskill(sd,RG_COMPULSION))>0)	// �R���p���V�����f�B�X�J�E���g
		rate2 = 5+skill*4;
	if(rate1 < rate2) rate1 = rate2;
	if(rate1)
		val = (int)((atn_bignumber)orig_value*(100-rate1)/100);
	if(val < 0) val = 0;
	if(orig_value > 0 && val < 1) val = 1;

	return val;
}


/*==========================================
 * �X�L���ɂ�锄��l�C��
 *------------------------------------------
 */
int pc_modifysellvalue(struct map_session_data *sd,int orig_value)
{
	int skill,val = orig_value,rate = 0;
	if((skill=pc_checkskill(sd,MC_OVERCHARGE))>0)	// �I�[�o�[�`���[�W
		rate = 5+skill*2-((skill==10)? 1:0);
	if(rate)
		val = (int)((atn_bignumber)orig_value*(100+rate)/100);
	if(val < 0) val = 0;
	if(orig_value > 0 && val < 1) val = 1;

	return val;
}


/*==========================================
 * �A�C�e���𔃂������ɁA�V�����A�C�e�������g�����A
 * 3�������ɂ����邩�m�F
 *------------------------------------------
 */
int pc_checkadditem(struct map_session_data *sd,int nameid,int amount)
{
	int i;

	nullpo_retr(0, sd);

	if(itemdb_isequip(nameid))
		return ADDITEM_NEW;

	for(i=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid==nameid){
			if(sd->status.inventory[i].amount+amount > MAX_AMOUNT)
				return ADDITEM_OVERAMOUNT;
			return ADDITEM_EXIST;
		}
	}

	if(amount > MAX_AMOUNT)
		return ADDITEM_OVERAMOUNT;
	return ADDITEM_NEW;
}


/*==========================================
 * �󂫃A�C�e�����̌�
 *------------------------------------------
 */
int pc_inventoryblank(struct map_session_data *sd)
{
	int i,b;

	nullpo_retr(0, sd);

	for(i=0,b=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid==0)
			b++;
	}

	return b;
}


/*==========================================
 * �����𕥂�
 *------------------------------------------
 */
int pc_payzeny(struct map_session_data *sd,int zeny)
{
	atn_bignumber z;

	nullpo_retr(0, sd);

	z = (atn_bignumber)sd->status.zeny;
	if(sd->status.zeny<zeny || z - (atn_bignumber)zeny > MAX_ZENY)
		return 1;
	sd->status.zeny-=zeny;
	clif_updatestatus(sd,SP_ZENY);

	return 0;
}


/*==========================================
 * �����𓾂�
 *------------------------------------------
 */
int pc_getzeny(struct map_session_data *sd,int zeny)
{
	atn_bignumber z;

	nullpo_retr(0, sd);

	z = (atn_bignumber)sd->status.zeny;
	if(z + zeny > MAX_ZENY) {
		zeny = 0;
		sd->status.zeny = MAX_ZENY;
	}
	sd->status.zeny+=zeny;
	clif_updatestatus(sd,SP_ZENY);

	return 0;
}


/*==========================================
 * �A�C�e����T���āA�C���f�b�N�X��Ԃ�
 *------------------------------------------
 */
int pc_search_inventory(struct map_session_data *sd,int item_id)
{
	int i;

	nullpo_retr(-1, sd);

	for(i=0;i<MAX_INVENTORY;i++) {
		if(sd->status.inventory[i].nameid == item_id &&
		 (sd->status.inventory[i].amount > 0 || item_id == 0))
			return i;
	}

	return -1;
}


/*==========================================
 * �A�C�e���ǉ��B���̂�item�\���̂̐����𖳎�
 *------------------------------------------
 */
int pc_additem(struct map_session_data *sd,struct item *item_data,int amount)
{
	struct item_data *data;
	int i,w;

	nullpo_retr(1, sd);
	nullpo_retr(1, item_data);

	if(item_data->nameid <= 0 || amount <= 0)
		return 1;
	data = itemdb_search(item_data->nameid);
	if((w = data->weight*amount) + sd->weight > sd->max_weight)
		return 2;

	i = MAX_INVENTORY;

	if(!itemdb_isequip2(data)){
		// �� ���i�ł͂Ȃ��̂ŁA�����L�i�Ȃ���̂ݕω�������
		for(i=0;i<MAX_INVENTORY;i++)
		if(sd->status.inventory[i].nameid == item_data->nameid &&
			sd->status.inventory[i].card[0] == item_data->card[0] && sd->status.inventory[i].card[1] == item_data->card[1] &&
			sd->status.inventory[i].card[2] == item_data->card[2] && sd->status.inventory[i].card[3] == item_data->card[3]) {
			if(sd->status.inventory[i].amount+amount > MAX_AMOUNT)
				return 5;
			sd->status.inventory[i].amount+=amount;
			clif_additem(sd,i,amount,0);
			break;
		}
	}
	if(i >= MAX_INVENTORY){
		// �� ���i�������L�i�������̂ŋ󂫗��֒ǉ�
		i = pc_search_inventory(sd,0);
		if(i >= 0) {
			memcpy(&sd->status.inventory[i],item_data,sizeof(sd->status.inventory[0]));
			if(itemdb_isequip2(data)){
				sd->status.inventory[i].amount=1;
				amount=1;
			}else{
				sd->status.inventory[i].amount=amount;
			}
			sd->inventory_data[i]=data;
			clif_additem(sd,i,amount,0);
		}
		else return 4;
	}
	sd->weight += w;
	clif_updatestatus(sd,SP_WEIGHT);

	return 0;
}

/*==========================================
 * �����A�C�e��������
 *------------------------------------------
 */
int pc_lossequipitem(struct map_session_data *sd,int pos,int type)
{
	int n;
	if(sd == NULL) return 0;
	n = sd->equip_index[pos];
	pc_unequipitem(sd,n,type);
	pc_delitem(sd,n,1,type);
	return 0;
}
/*==========================================
 * �A�C�e�������炷
 *------------------------------------------
 */
int pc_delitem(struct map_session_data *sd,int n,int amount,int type)
{
	nullpo_retr(1, sd);

	if(sd->status.inventory[n].nameid==0 || amount <= 0 || sd->status.inventory[n].amount<amount || sd->inventory_data[n] == NULL)
		return 1;

	sd->status.inventory[n].amount -= amount;
	sd->weight -= sd->inventory_data[n]->weight*amount ;
	if(sd->status.inventory[n].amount<=0){
		if(sd->status.inventory[n].equip)
			pc_unequipitem(sd,n,0);
		memset(&sd->status.inventory[n],0,sizeof(sd->status.inventory[0]));
		sd->inventory_data[n] = NULL;
	}
	if(!(type&1))
		clif_delitem(sd,n,amount);
	if(!(type&2))
		clif_updatestatus(sd,SP_WEIGHT);

	return 0;
}


/*==========================================
 * �A�C�e���𗎂�
 *------------------------------------------
 */
int pc_dropitem(struct map_session_data *sd,int n,int amount)
{
	nullpo_retr(1, sd);

	if (n < 0 || n >= MAX_INVENTORY)
		return 1;

	if (amount <= 0)
		return 1;

	if(sd->status.inventory[n].nameid <=0 || sd->status.inventory[n].amount < amount)
		return 1;

	if(itemdb_isdropable(sd->status.inventory[n].nameid) == 0)
		return 1;
	if(pc_candrop(sd,sd->status.inventory[n].nameid))
		return 1;

	map_addflooritem(&sd->status.inventory[n],amount,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
	pc_delitem(sd,n,amount,0);

	return 0;
}


/*==========================================
 * �A�C�e�����E��
 *------------------------------------------
 */
int pc_takeitem(struct map_session_data *sd,struct flooritem_data *fitem)
{
	int flag;
	unsigned int tick = gettick();
	struct map_session_data *first_sd = NULL,*second_sd = NULL,*third_sd = NULL;

	nullpo_retr(0, sd);
	nullpo_retr(0, fitem);

	if(distance(fitem->bl.x,fitem->bl.y,sd->bl.x,sd->bl.y)>2)
		return 0;	// ����������

	if(fitem->first_get_id > 0) {
		first_sd = map_id2sd(fitem->first_get_id);
		if(tick < fitem->first_get_tick) {
			if(fitem->first_get_id != sd->bl.id && !(first_sd && first_sd->status.party_id == sd->status.party_id)) {
				clif_additem(sd,0,0,6);
				return 0;
			}
		}
		else if(fitem->second_get_id > 0) {
			second_sd = map_id2sd(fitem->second_get_id);
			if(tick < fitem->second_get_tick) {
				if(fitem->first_get_id != sd->bl.id && fitem->second_get_id != sd->bl.id &&
					!(first_sd && first_sd->status.party_id == sd->status.party_id) && !(second_sd && second_sd->status.party_id == sd->status.party_id)) {
					clif_additem(sd,0,0,6);
					return 0;
				}
			}
			else if(fitem->third_get_id > 0) {
				third_sd = map_id2sd(fitem->third_get_id);
				if(tick < fitem->third_get_tick) {
					if(fitem->first_get_id != sd->bl.id && fitem->second_get_id != sd->bl.id && fitem->third_get_id != sd->bl.id &&
						!(first_sd && first_sd->status.party_id == sd->status.party_id) && !(second_sd && second_sd->status.party_id == sd->status.party_id) &&
						!(third_sd && third_sd->status.party_id == sd->status.party_id)) {
						clif_additem(sd,0,0,6);
						return 0;
					}
				}
			}
		}
	}
	if((flag = pc_additem(sd,&fitem->item_data,fitem->item_data.amount)))
		// �d��over�Ŏ擾���s
		clif_additem(sd,0,0,flag);
	else {
		/* �擾���� */
		if(sd->attacktimer != -1)
			pc_stopattack(sd);
		clif_takeitem(&sd->bl,&fitem->bl);
		map_clearflooritem(fitem->bl.id);
	}
	return 0;
}

int pc_isUseitem(struct map_session_data *sd,int n)
{
	struct item_data *item;
	int nameid;
	struct pc_base_job s_class;

	nullpo_retr(0, sd);

	item = sd->inventory_data[n];
	nameid = sd->status.inventory[n].nameid;

	//�]����{�q�̏ꍇ�̌��̐E�Ƃ��Z�o����
	s_class = pc_calc_base_job(sd->status.class);
	if(item == NULL)
		return 0;
	if(item->type != 0 && item->type != 2)
		return 0;
	if((nameid == 605) && map[sd->bl.m].flag.gvg)
		return 0;
	if(nameid == 601 && (map[sd->bl.m].flag.noteleport || map[sd->bl.m].flag.gvg)) {
		clif_skill_teleportmessage(sd,0);
		return 0;
	}
	if(nameid == 602 && map[sd->bl.m].flag.noreturn)
		return 0;
	if(nameid == 604 && (map[sd->bl.m].flag.nobranch || map[sd->bl.m].flag.gvg))
		return 0;
	if(item->sex != 2 && sd->sex != item->sex)
		return 0;
	if(item->elv > 0 && sd->status.base_level < item->elv)
		return 0;
	if(((1<<s_class.job)&item->class) == 0)
		return 0;
	return 1;
}

/*==========================================
 * �A�C�e�����g��
 *------------------------------------------
 */
int pc_useitem(struct map_session_data *sd,int n)
{
	int nameid,amount;

	nullpo_retr(1, sd);

	if(n >=0 && n < MAX_INVENTORY) {
		char *script;
		nameid = sd->status.inventory[n].nameid;
		amount = sd->status.inventory[n].amount;
		if(sd->status.inventory[n].nameid <= 0 ||
			sd->status.inventory[n].amount <= 0 ||
			!pc_isUseitem(sd,n) ) {
			clif_useitemack(sd,n,0,0);
			return 1;
		}
		sd->use_itemid = nameid;
		script = sd->inventory_data[n]->use_script;

//		amount = sd->status.inventory[n].amount;
//		clif_useitemack(sd,n,amount-1,1);
//		pc_delitem(sd,n,1,1);
		if (battle_config.Item_res) {
			amount = sd->status.inventory[n].amount;
			clif_useitemack(sd,n,amount-1,1);
			pc_delitem(sd,n,1,1);
		} else clif_useitemack(sd,n,amount,1);
		run_script(script,0,sd->bl.id,0);
	}

	return 0;
}


/*==========================================
 * �J�[�g�A�C�e���ǉ��B���̂�item�\���̂̐����𖳎�
 *------------------------------------------
 */
int pc_cart_additem(struct map_session_data *sd,struct item *item_data,int amount)
{
	struct item_data *data;
	int i,w;

	nullpo_retr(1, sd);
	nullpo_retr(1, item_data);

	if(item_data->nameid <= 0 || amount <= 0)
		return 1;
	data = itemdb_search(item_data->nameid);

	if((w=data->weight*amount) + sd->cart_weight > sd->cart_max_weight)
		return 1;

	i=MAX_CART;
	if(!itemdb_isequip2(data)){
		// �� ���i�ł͂Ȃ��̂ŁA�����L�i�Ȃ���̂ݕω�������
		for(i=0;i<MAX_CART;i++){
			if(sd->status.cart[i].nameid==item_data->nameid &&
				sd->status.cart[i].card[0] == item_data->card[0] && sd->status.cart[i].card[1] == item_data->card[1] &&
				sd->status.cart[i].card[2] == item_data->card[2] && sd->status.cart[i].card[3] == item_data->card[3]){
				if(sd->status.cart[i].amount+amount > MAX_AMOUNT)
					return 1;
				sd->status.cart[i].amount+=amount;
				clif_cart_additem(sd,i,amount,0);
				break;
			}
		}
	}
	if(i >= MAX_CART){
		// �� ���i�������L�i�������̂ŋ󂫗��֒ǉ�
		for(i=0;i<MAX_CART;i++){
			if(sd->status.cart[i].nameid==0){
				memcpy(&sd->status.cart[i],item_data,sizeof(sd->status.cart[0]));
				if(itemdb_isequip2(data)){
					sd->status.cart[i].amount=1;
					amount=1;
				}else{
					sd->status.cart[i].amount=amount;
				}
				sd->cart_num++;
				clif_cart_additem(sd,i,amount,0);
				break;
			}
		}
		if(i >= MAX_CART)
			return 1;
	}
	sd->cart_weight += w;
	clif_updatestatus(sd,SP_CARTINFO);

	return 0;
}


/*==========================================
 * �J�[�g�A�C�e�������炷
 *------------------------------------------
 */
int pc_cart_delitem(struct map_session_data *sd,int n,int amount,int type)
{
	nullpo_retr(1, sd);

	if(sd->status.cart[n].nameid==0 ||
	   sd->status.cart[n].amount<amount)
		return 1;

	sd->status.cart[n].amount -= amount;
	sd->cart_weight -= itemdb_weight(sd->status.cart[n].nameid)*amount ;
	if(sd->status.cart[n].amount <= 0){
		memset(&sd->status.cart[n],0,sizeof(sd->status.cart[0]));
		sd->cart_num--;
	}
	if(!type) {
		clif_cart_delitem(sd,n,amount);
		clif_updatestatus(sd,SP_CARTINFO);
	}

	return 0;
}


/*==========================================
 * �J�[�g�փA�C�e���ړ�
 *------------------------------------------
 */
int pc_putitemtocart(struct map_session_data *sd,int idx,int amount)
{
	struct item *item_data;

	nullpo_retr(0, sd);
	nullpo_retr(0, item_data=&sd->status.inventory[idx]);

	if(itemdb_isdropable(sd->status.inventory[idx].nameid) == 0)
		return 1;
	if(pc_candrop(sd,sd->status.inventory[idx].nameid)==1)
		return 1;

	if( item_data->nameid==0 || item_data->amount<amount || sd->vender_id )
		return 1;
	if(pc_cart_additem(sd,item_data,amount)==0)
		return pc_delitem(sd,idx,amount,0);

	return 1;
}

/*==========================================
 * �J�[�g���̃A�C�e�����m�F(���̍�����Ԃ�)
 *------------------------------------------
 */
int pc_cartitem_amount(struct map_session_data *sd,int idx,int amount)
{
	struct item *item_data;

	nullpo_retr(-1, sd);
	nullpo_retr(-1, item_data=&sd->status.cart[idx]);

	if( item_data->nameid==0 || !item_data->amount)
		return -1;
	return item_data->amount-amount;
}
/*==========================================
 * �J�[�g����A�C�e���ړ�
 *------------------------------------------
 */

int pc_getitemfromcart(struct map_session_data *sd,int idx,int amount)
{
	struct item *item_data;
	int flag;

	nullpo_retr(0, sd);
	nullpo_retr(0, item_data=&sd->status.cart[idx]);


	if( item_data->nameid==0 || item_data->amount<amount || sd->vender_id )
		return 1;
	if((flag = pc_additem(sd,item_data,amount)) == 0)
		return pc_cart_delitem(sd,idx,amount,0);

	clif_additem(sd,0,0,flag);
	return 1;
}


/*==========================================
 * �A�C�e���Ӓ�
 *------------------------------------------
 */
int pc_item_identify(struct map_session_data *sd,int idx)
{
	int flag=1;

	nullpo_retr(0, sd);

	if(idx >= 0 && idx < MAX_INVENTORY) {
		if(sd->status.inventory[idx].nameid > 0 && sd->status.inventory[idx].identify == 0 ){
			flag=0;
			sd->status.inventory[idx].identify=1;
		}
		clif_item_identified(sd,idx,flag);
	}
	else
		clif_item_identified(sd,idx,flag);

	return !flag;
}

/*==========================================
 * �X�e�B���i���J
 *------------------------------------------
 */
int pc_show_steal(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	int itemid;
	int type;

	struct item_data *item=NULL;
	char output[100];

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd=va_arg(ap,struct map_session_data *));

	itemid=va_arg(ap,int);
	type=va_arg(ap,int);

	if(!type){
		if((item=itemdb_exists(itemid))==NULL)
			sprintf(output,"%s��Unknown_Item���X�e�B�[�����܂���",sd->status.name);
		else
			sprintf(output,"%s��%s���X�e�B�[�����܂���",sd->status.name,item->jname);
		clif_displaymessage( ((struct map_session_data *)bl)->fd, output);
	}else{
		sprintf(output,"%s�͏d�ʃI�[�o�[�ɂ��A�C�e�����擾�ł��܂���ł���",sd->status.name);
		clif_displaymessage( ((struct map_session_data *)bl)->fd, output);
	}

	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int pc_steal_item(struct map_session_data *sd,struct block_list *bl)
{
	if(sd != NULL && bl != NULL && bl->type == BL_MOB) {
		int i,skill,rate,itemid,flag;
		struct mob_data *md=(struct mob_data *)bl;
		if( md && !md->state.steal_flag && mob_db[md->class].mexp <= 0 && !(mob_db[md->class].mode&0x20) &&
			md->sc_data && md->sc_data[SC_STONE].timer == -1 && md->sc_data[SC_FREEZE].timer == -1) {
			skill = sd->paramc[4] - mob_db[md->class].dex + pc_checkskill(sd,TF_STEAL) * 3 + 10;
			if(0 < skill) {
				for(i=0;i<8;i++) {
					itemid = mob_db[md->class].dropitem[i].nameid;
					if(itemid > 0 && itemdb_type(itemid) != 6) {
						rate = (mob_db[md->class].dropitem[i].p /battle_config.item_rate * 100 * skill)/100;
						rate += sd->add_steal_rate;
						if(atn_rand()%10000 < rate) {
							struct item tmp_item;
							memset(&tmp_item,0,sizeof(tmp_item));
							tmp_item.nameid = itemid;
							tmp_item.amount = 1;
							tmp_item.identify = 1;
							flag = pc_additem(sd,&tmp_item,1);
							if(battle_config.show_steal_in_same_party)
								party_foreachsamemap(pc_show_steal,sd,1,sd,tmp_item.nameid,0);
							if(flag){
								if(battle_config.show_steal_in_same_party)
									party_foreachsamemap(pc_show_steal,sd,1,sd,tmp_item.nameid,1);
								clif_additem(sd,0,0,flag);
							}
							md->state.steal_flag = 1;
							return 1;
						}
					}
				}
			}
		}
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_steal_coin(struct map_session_data *sd,struct block_list *bl)
{
	if(sd != NULL && bl != NULL && bl->type == BL_MOB) {
		int rate,skill;
		struct mob_data *md=(struct mob_data *)bl;
		if(md && !md->state.steal_coin_flag && md->sc_data && md->sc_data[SC_STONE].timer == -1 && md->sc_data[SC_FREEZE].timer == -1) {
			skill = pc_checkskill(sd,RG_STEALCOIN)*10;
			rate = skill + (sd->status.base_level - mob_db[md->class].lv)*3 + sd->paramc[4]*2 + sd->paramc[5]*2;
			if(atn_rand()%1000 < rate) {
				pc_getzeny(sd,mob_db[md->class].lv*10 + atn_rand()%100);
				md->state.steal_coin_flag = 1;
				return 1;
			}
		}
	}

	return 0;
}
//
//
//

/*==========================================
 * PC���}�b�v���痣�E����
 *------------------------------------------
 */

int pc_remove_map(struct map_session_data *sd,int clrtype) {
	nullpo_retr(0, sd);

	// map ��ɓo�^����Ă��Ȃ�
	if(!sd->bl.prev) 
		return 1;

	// �`���b�g����o��
	if(sd->chatID)
		chat_leavechat(sd);

	// ����𒆒f����
	if(sd->trade_partner)
		trade_tradecancel(sd);

	// �q�ɂ��J���Ă�Ȃ�ۑ�����
	if(sd->state.storage_flag)
		storage_guild_storage_quit(sd,0);
	else
		storage_storage_quit(sd);

	// �F�B���X�g���U�����ۂ���
	if(sd->friend_invite>0)
		friend_add_reply(sd,sd->friend_invite,sd->friend_invite_char,0);

	// �p�[�e�B���U�����ۂ���
	if(sd->party_invite>0)
		party_reply_invite(sd,sd->party_invite_account,0);

	// �M���h���U�����ۂ���
	if(sd->guild_invite>0)
		guild_reply_invite(sd,sd->guild_invite,0);

	// �M���h�������U�����ۂ���
	if(sd->guild_alliance>0)
		guild_reply_reqalliance(sd,sd->guild_alliance_account,0);

	// �������̓����O���Ă�1���Ԃ͌p��
//	if(sd->sc_data && sd->sc_data[SC_WEDDING].timer!=-1)
//		pc_setglobalreg(sd,"PC_WEDDING_TIME",sd->sc_data[SC_WEDDING].val2);
//	else
//		pc_setglobalreg(sd,"PC_WEDDING_TIME",0);

	pc_stop_walking(sd,0);		// ���s���f
	pc_stopattack(sd);			// �U�����f
	pc_delinvincibletimer(sd);	// ���G�^�C�}�[�폜

	// �u���[�h�X�g�b�v���I��点��
	if(sd->sc_data[SC_BLADESTOP].timer!=-1)
		status_change_end(&sd->bl,SC_BLADESTOP,-1);

	// �o�V���J�폜
	if (sd->sc_data[SC_BASILICA].timer!=-1) {
		int i;
		for (i=0;i<MAX_SKILLUNITGROUP;i++)
			if (sd->skillunit[i].skill_id==HP_BASILICA)
				skill_delunitgroup(&sd->skillunit[i]);
		status_change_end(&sd->bl,SC_BASILICA,-1);
	}
	skill_castcancel(&sd->bl,0);			// �r�����f
	skill_gangsterparadise(sd,0);			// �M�����O�X�^�[�p���_�C�X�폜
	skill_unit_move(&sd->bl,gettick(),0);	// �X�L�����j�b�g���痣�E
	skill_cleartimerskill(&sd->bl);			// �^�C�}�[�X�L���N���A
	skill_clear_unitgroup(&sd->bl);			// �X�L�����j�b�g�O���[�v�̍폜

	clif_clearchar_area(&sd->bl,clrtype&0xffff);
	map_delblock(&sd->bl);
	return 0;
}

/*==========================================
 * PC�̈ʒu�ݒ�
 *------------------------------------------
 */
int pc_setpos(struct map_session_data *sd,char *mapname_org,int x,int y,int clrtype)
{
	char mapname[24];
	int m;

	nullpo_retr(0, sd);
	memcpy(mapname,mapname_org,24);
	mapname[16]=0;
	if(strstr(mapname,".gat")==NULL && strlen(mapname)<16){
		strcat(mapname,".gat");
	}
	m=map_mapname2mapid(mapname);
	if(m<0){
		int ip,port;
		if(map_mapname2ipport(mapname,&ip,&port) == 0) {
			// �Ⴄ�}�b�v�T�[�o�[�Ɋ��蓖�Ă��Ă���}�b�v�Ɉړ�
			if(sd->status.pet_id > 0 && sd->pd) {
				if(sd->pd->bl.m != m && sd->pet.intimate <= 0) {
					pet_remove_map(sd);
					intif_delete_petdata(sd->status.pet_id);
					sd->status.pet_id = 0;
					sd->pd = NULL;
					sd->petDB = NULL;
					if(battle_config.pet_status_support)
						status_calc_pc(sd,2);
				}
				else if(sd->pet.intimate > 0) {
					pet_stopattack(sd->pd);
					pet_changestate(sd->pd,MS_IDLE,0);
					clif_clearchar_area(&sd->pd->bl,clrtype&0xffff);
					map_delblock(&sd->pd->bl);
				}
			}
			if(sd->status.pet_id > 0 && sd->pd)
				intif_save_petdata(sd->status.account_id,&sd->pet);

			friend_send_online( sd, 1 );			// �F�B���X�g�̃��O�A�E�g���b�Z�[�W���M
			party_send_logout(sd);					// �p�[�e�B�̃��O�A�E�g���b�Z�[�W���M
			guild_send_memberinfoshort(sd,0);		// �M���h�̃��O�A�E�g���b�Z�[�W���M
			skill_status_change_clear(&sd->bl,1);	// �X�e�[�^�X�ُ����������
			skill_stop_dancing(&sd->bl,1);			// �_���X/���t���f
			pc_cleareventtimer(sd);					// �C�x���g�^�C�}��j������
			pc_delspiritball(sd,sd->spiritball,1);	// �C���폜
			pc_remove_map(sd,clrtype);
			memcpy(sd->status.last_point.map,mapname,24);
			sd->status.last_point.x = x;
			sd->status.last_point.y = y;
			sd->state.waitingdisconnect = 1;
			chrif_save(sd);
			storage_storage_save(sd);
			storage_delete(sd->status.account_id);
			chrif_changemapserver(sd,mapname,x,y,ip,(short)port);
			return 0;
		}
#if 0
		clif_authfail_fd(sd->fd,0);	// cancel
		clif_setwaitclose(sd->fd);
#endif
		return 1;
	}

	// �X�^�b�N����
	if(x <0 || x >= map[m].xs || y <0 || y >= map[m].ys)
		x=y=0;
	if((x==0 && y==0) || map_getcell(m,x,y,CELL_CHKNOPASS)){
		if(x||y) {
			if(battle_config.error_log)
				printf("stacked (%d,%d)\n",x,y);
		}
		do {
			x=atn_rand()%(map[m].xs-2)+1;
			y=atn_rand()%(map[m].ys-2)+1;
		} while(map_getcell(m,x,y,CELL_CHKNOPASS));
	}

	if(m == sd->bl.m) {
		// �����}�b�v�Ȃ̂Ń_���X���j�b�g�����p��
		sd->to_x = x;
		sd->to_y = y;
		skill_stop_dancing(&sd->bl, 2); //�ړ���Ƀ��j�b�g���ړ����邩�ǂ����̔��f������
	} else {
		// �Ⴄ�}�b�v�Ȃ̂Ń_���X���j�b�g�폜
		skill_stop_dancing(&sd->bl, 1);
	}
	if(sd->bl.prev != NULL){
		pc_remove_map(sd,clrtype);
		if(sd->status.pet_id > 0 && sd->pd) {
			if(sd->pd->bl.m != m && sd->pet.intimate <= 0) {
				pet_remove_map(sd);
				intif_delete_petdata(sd->status.pet_id);
				sd->status.pet_id = 0;
				sd->pd = NULL;
				sd->petDB = NULL;
				if(battle_config.pet_status_support)
					status_calc_pc(sd,2);
			}
			else if(sd->pet.intimate > 0) {
				pet_stopattack(sd->pd);
				pet_changestate(sd->pd,MS_IDLE,0);
				clif_clearchar_area(&sd->pd->bl,clrtype&0xffff);
				map_delblock(&sd->pd->bl);
			}
		}
		clif_changemap(sd,mapname,x,y);
	}
	memcpy(sd->mapname,mapname,24);
	sd->bl.m = m;
	sd->bl.x = x;
	sd->bl.y = y;


	if(sd->status.option&2)
		status_change_end(&sd->bl, SC_HIDING, -1);
	if(pc_iscloaking(sd))
		status_change_end(&sd->bl, SC_CLOAKING, -1);
	if(pc_ischasewalk(sd))
		status_change_end(&sd->bl, SC_CHASEWALK, -1);
		
	// �y�b�g�̈ړ�
	if(sd->status.pet_id > 0 && sd->pd && sd->pet.intimate > 0) {
		sd->pd->bl.m = m;
		sd->pd->bl.x = sd->pd->to_x = x;
		sd->pd->bl.y = sd->pd->to_y = y;
		sd->pd->dir = sd->dir;
	}

//	map_addblock(&sd->bl);	/// �u���b�N�o�^��spawn��
//	clif_spawnpc(sd);		// clif_parse_LoadEndAck�ōs��

	return 0;
}


/*
*	�M���h�X�L���̗L���`�F�b�N
*	�ω�������΃X�e�[�^�X�̍Čv�Z
*/
int pc_check_guild_skill_effective_range(struct map_session_data *sd)
{
	int mi,dx,dy,range,min_range;
	struct map_session_data * gmsd = NULL;
	struct map_session_data * member = NULL;
	struct guild*  g =NULL;	
	
	//
	if(sd == NULL)
		return 0;
	//�M���h�ɓ����Ă��Ȃ�
	if(sd->status.guild_id == 0)
		return 0;
	//�M���h�X�L��������
	if(!battle_config.guild_hunting_skill_available)
		return 0;
		
	g = guild_search(sd->status.guild_id);
	
	//�M���h�擾�ł��Ȃ�����
	if(g == NULL)
	 	return 0;
	
	gmsd = g->member[0].sd;

	//�}�X�^�[�ڑ����Ă��Ȃ�
	if(gmsd == NULL)
	{
		//���ʔ͈͓�������
		if(sd->under_the_influence_of_the_guild_skill > 0)
			status_calc_pc(sd,0);
		return 0;
	}

	//�X�L���P�ȏ�
	if(pc_checkskill(gmsd,GD_LEADERSHIP) > 0 || pc_checkskill(gmsd,GD_SOULCOLD) > 0 
		||pc_checkskill(gmsd,GD_GLORYWOUNDS) > 0 ||pc_checkskill(gmsd,GD_HAWKEYES) > 0)
	{
		min_range = 999;
		if(sd == gmsd)//�������M���}�X
		{
			if(battle_config.allow_me_guild_skill && sd->under_the_influence_of_the_guild_skill == 0)
				status_calc_pc(sd,0);
				
			//�X�L����������
			for(mi = 1;mi < g->max_member;mi++)
			{
				member = g->member[mi].sd;
				
				//�����o�[�ڑ��Ȃ�
				if(member == NULL)
					continue;
				
				//�}�b�v���Ⴄ
				if(sd->bl.m != member->bl.m)
				{
					//�O��͌��ʔ͈͓�������
					if(member->under_the_influence_of_the_guild_skill > 0)
						status_calc_pc(sd,0);
					continue;
				}
				
				dx = abs(sd->bl.x - member->bl.x);
				dy = abs(sd->bl.y - member->bl.y);
				
				if(battle_config.guild_skill_effective_range > 0)//�X�L���𓯈ꋗ�����肷��
				{
					range = battle_config.guild_skill_effective_range;
					if(dx <=range &&  dy <= range)//�͈͓�
					{
						if(member->under_the_influence_of_the_guild_skill==0)///���ʊO������
							status_calc_pc(member,0);
							
					}else{//�͈͊O
						if(member->under_the_influence_of_the_guild_skill>0)//���ʓ�������
							status_calc_pc(member,0);
					}
				}else{//�X�L�����ʂɋ������肷��
					min_range = 999;
					range = skill_get_range(GD_LEADERSHIP,guild_skill_get_lv(g,GD_LEADERSHIP));
					if(guild_skill_get_lv(g,GD_LEADERSHIP) > 0 && dx <=range &&  dy <= range)
						if(min_range > range) min_range = range;
			
					range = skill_get_range(GD_SOULCOLD,guild_skill_get_lv(g,GD_SOULCOLD));	
					if(guild_skill_get_lv(g,GD_SOULCOLD) > 0 && dx <=range &&  dy <= range)
						if(min_range > range) min_range = range;
				
					range = skill_get_range(GD_GLORYWOUNDS,guild_skill_get_lv(g,GD_GLORYWOUNDS));
					if(guild_skill_get_lv(g,GD_GLORYWOUNDS) > 0 && dx <=range &&  dy <= range)
						if(min_range > range) min_range = range;
				
					range = skill_get_range(GD_HAWKEYES,guild_skill_get_lv(g,GD_HAWKEYES));
					if(guild_skill_get_lv(g,GD_HAWKEYES) > 0 && dx <=range &&  dy <= range)
						if(min_range > range) min_range = range;
						
					if(min_range == 999) //���ʔ͈͊O
					{
						//�O�͌��ʔ͈͓�������
						if(member->under_the_influence_of_the_guild_skill>0)
							status_calc_pc(member,0);
					}else{ //���ʔ͈͓�
						 if( member->under_the_influence_of_the_guild_skill==0)//�O�͔͈͊O������
						 { 
						 	status_calc_pc(member,0);
						 }	  //�O�͔͈͓��ō��񋗗����ς����->������ʕϓ�
						 else if((min_range+1) != member->under_the_influence_of_the_guild_skill)
						 {
							 status_calc_pc(member,0);
						 }
					}
				}
			}
		}else{//�M������
			//����}�b�v
			if(sd->bl.m == gmsd->bl.m)
			{
				dx = abs(sd->bl.x - gmsd->bl.x);
				dy = abs(sd->bl.y - gmsd->bl.y);
				
				if(battle_config.guild_skill_effective_range > 0)//�X�L���𓯈ꋗ�����肷��
				{
					range = battle_config.guild_skill_effective_range;
					if(dx <=range &&  dy <= range)//�͈͓�
					{
						if(sd->under_the_influence_of_the_guild_skill==0)///���ʊO������
							status_calc_pc(sd,0);
							
					}else{//�͈͊O
						if(sd->under_the_influence_of_the_guild_skill>0)//���ʓ�������
							status_calc_pc(sd,0);
					}
				}else{//�X�L�����ʂɋ������肷��
					min_range = 999;
					range = skill_get_range(GD_LEADERSHIP,guild_skill_get_lv(g,GD_LEADERSHIP));
					if(guild_skill_get_lv(g,GD_LEADERSHIP) > 0 && dx <=range &&  dy <= range)
						if(min_range > range) min_range = range;
			
					range = skill_get_range(GD_SOULCOLD,guild_skill_get_lv(g,GD_SOULCOLD));	
					if(guild_skill_get_lv(g,GD_SOULCOLD) > 0 && dx <=range &&  dy <= range)
						if(min_range > range) min_range = range;
				
					range = skill_get_range(GD_GLORYWOUNDS,guild_skill_get_lv(g,GD_GLORYWOUNDS));
					if(guild_skill_get_lv(g,GD_GLORYWOUNDS) > 0 && dx <=range &&  dy <= range)
						if(min_range > range) min_range = range;
				
					range = skill_get_range(GD_HAWKEYES,guild_skill_get_lv(g,GD_HAWKEYES));
					if(guild_skill_get_lv(g,GD_HAWKEYES) > 0 && dx <=range &&  dy <= range)
						if(min_range > range) min_range = range;
						
					if(min_range == 999) //���ʔ͈͊O
					{
						//�O�͌��ʔ͈͓�������
						if(sd->under_the_influence_of_the_guild_skill>0)
							status_calc_pc(sd,0);
					}else{ //���ʔ͈͓�
						 if( sd->under_the_influence_of_the_guild_skill==0)//�O�͔͈͊O������
						 { 
						 	status_calc_pc(sd,0);
						 }	  //�O�͔͈͓��ō��񋗗����ς����->������ʕϓ�
						 else if((min_range+1) != sd->under_the_influence_of_the_guild_skill)
						 {
							 status_calc_pc(sd,0);
						 }
					}
				}
			}else{//�}�b�v���Ⴄ
				 if(sd->under_the_influence_of_the_guild_skill>0)//�O�͌��ʔ͈͓�������
					status_calc_pc(sd,0);
			}
		}
	}
	return 1;
}

/*==========================================
 * PC�̃����_�����[�v
 *------------------------------------------
 */
int pc_randomwarp(struct map_session_data *sd,int type)
{
	int x,y,i=0;
	int m;

	nullpo_retr(0, sd);

	m=sd->bl.m;

	if(map[sd->bl.m].flag.noteleport)	// �e���|�[�g�֎~
		return 0;

	do{
		x=atn_rand()%(map[m].xs-2)+1;
		y=atn_rand()%(map[m].ys-2)+1;
	}while( map_getcell(m,x,y,CELL_CHKNOPASS)&& (i++)<1000 );

	if(i<1000)
		pc_setpos(sd,map[m].name,x,y,type);

	return 0;
}


/*==========================================
 * ���݈ʒu�̃���
 *------------------------------------------
 */
int pc_memo(struct map_session_data *sd,int i)
{
	int skill;
	int j;

	nullpo_retr(0, sd);

	skill=pc_checkskill(sd,AL_WARP);

	if(i>=MIN_PORTAL_MEMO)
		i-=MIN_PORTAL_MEMO;
	else if(map[sd->bl.m].flag.nomemo){
		clif_skill_teleportmessage(sd,1);
		return 0;
	}

	if(skill < 1) {
		clif_skill_memo(sd,2);
	}

	if(skill<2 || i<-1 || i>2){
		clif_skill_memo(sd,1);
		return 0;
	}

	for(j=0;j<3;j++){
		if(strcmp(sd->status.memo_point[j].map,map[sd->bl.m].name)==0){
			i=j;
			break;
		}
	}

	if(i==-1){
		for(i=skill-3;i>=0;i--){
			memcpy(&sd->status.memo_point[i+1],&sd->status.memo_point[i],
				sizeof(struct point));
		}
		i=0;
	}
	memcpy(sd->status.memo_point[i].map,map[sd->bl.m].name,24);
	sd->status.memo_point[i].x=sd->bl.x;
	sd->status.memo_point[i].y=sd->bl.y;

	clif_skill_memo(sd,0);

	return 1;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_can_reach(struct map_session_data *sd,int x,int y)
{
	struct walkpath_data wpd;

	nullpo_retr(0, sd);

	if( sd->bl.x==x && sd->bl.y==y )	// �����}�X
		return 1;

	// ��Q������
	wpd.path_len=0;
	wpd.path_pos=0;
	wpd.path_half=0;
	return (path_search(&wpd,sd->bl.m,sd->bl.x,sd->bl.y,x,y,0)!=-1)?1:0;
}

//
// �� �s��
//
/*==========================================
 * ����1���ɂ����鎞�Ԃ��v�Z
 *------------------------------------------
 */
static int calc_next_walk_step(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	if(sd->walkpath.path_pos>=sd->walkpath.path_len)
		return -1;
	if(sd->walkpath.path[sd->walkpath.path_pos]&1)
		return sd->speed*14/10;

	return sd->speed;
}


/*==========================================
 * �����i��(timer�֐�)
 *------------------------------------------
 */
static int pc_walk(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd;
	int i;
	int moveblock;
	int x,y,dx,dy;
	
	sd=map_id2sd(id);
	if(sd==NULL)
		return 0;

	if(sd->walktimer != tid){
		if(battle_config.error_log)
			printf("pc_walk %d != %d\n",sd->walktimer,tid);
		return 0;
	}
	sd->walktimer=-1;
	if(sd->walkpath.path_pos>=sd->walkpath.path_len || sd->walkpath.path_pos!=data)
		return 0;

	//�������̂ő����̃^�C�}�[��������
	sd->inchealspirithptick = 0;
	sd->inchealspiritsptick = 0;
	
	sd->walkpath.path_half ^= 1;
	if(sd->walkpath.path_half==0){ // �}�X�ڒ��S�֓���
		sd->walkpath.path_pos++;
		if(sd->state.change_walk_target){
			pc_walktoxy_sub(sd);
			return 0;
		}
	} else { // �}�X�ڋ��E�֓���
		if(sd->walkpath.path[sd->walkpath.path_pos]>=8)
			return 1;

		x = sd->bl.x;
		y = sd->bl.y;
		if(map_getcell(sd->bl.m,x,y,CELL_CHKNOPASS))
		{
			pc_stop_walking(sd,1);
			return 0;
		}
		sd->dir=sd->head_dir=sd->walkpath.path[sd->walkpath.path_pos];
		dx = dirx[(int)sd->dir];
		dy = diry[(int)sd->dir];
		if(map_getcell(sd->bl.m,x,y,CELL_CHKNOPASS))
		{
			pc_walktoxy_sub(sd);
			return 0;
		}

		moveblock = ( x/BLOCK_SIZE != (x+dx)/BLOCK_SIZE || y/BLOCK_SIZE != (y+dy)/BLOCK_SIZE);

		sd->walktimer = 1;
		map_foreachinmovearea(clif_pcoutsight,sd->bl.m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,dx,dy,0,sd);

		x += dx;
		y += dy;

		skill_unit_move(&sd->bl,tick,0);
		if(moveblock) map_delblock(&sd->bl);
		sd->bl.x = x;
		sd->bl.y = y;
		if(moveblock) map_addblock(&sd->bl);
		skill_unit_move(&sd->bl,tick,1);

		if(sd->sc_data[SC_DANCING].timer!=-1)
			skill_unit_move_unit_group((struct skill_unit_group *)sd->sc_data[SC_DANCING].val2,sd->bl.m,dx,dy);

		map_foreachinmovearea(clif_pcinsight,sd->bl.m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,-dx,-dy,0,sd);
		sd->walktimer = -1;

		if(sd->status.party_id>0){	// �p�[�e�B�̂g�o���ʒm����
			struct party *p=party_search(sd->status.party_id);
			if(p!=NULL){
				int p_flag=0;
				map_foreachinmovearea(party_send_hp_check,sd->bl.m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,-dx,-dy,BL_PC,sd->status.party_id,&p_flag);
				if(p_flag)
					sd->party_hp=-1;
			}
		}
		if(pc_iscloaking(sd))	// �N���[�L���O�̏��Ō���
		{
			if(pc_checkskill(sd,AS_CLOAKING) < 3)
				skill_check_cloaking(&sd->bl);
		}
		/* �f�B�{�[�V�������� */
		for(i=0;i<5;i++)
			if(sd->dev.val1[i]){
				skill_devotion3(&sd->bl,sd->dev.val1[i]);
				break;
			}
		/* ��f�B�{�[�V�������� */
		if( sd->sc_data && sd->sc_data[SC_DEVOTION].val1){
				skill_devotion2(&sd->bl,sd->sc_data[SC_DEVOTION].val1);
		}

		//�M���h�X�L���L��
		pc_check_guild_skill_effective_range(sd);
		
		if(map_getcell(sd->bl.m,x,y,CELL_CHKNPC))
			npc_touch_areanpc(sd,sd->bl.m,x,y);
		else
			sd->areanpc_id=0;
	}
	if((i=calc_next_walk_step(sd))>0) {
		i = i>>1;
		if(i < 1 && sd->walkpath.path_half == 0)
			i = 1;
		sd->walktimer=add_timer(tick+i,pc_walk,id,sd->walkpath.path_pos);
	}


	return 0;
}


/*==========================================
 * �ړ��\���m�F���āA�\�Ȃ���s�J�n
 *------------------------------------------
 */
static int pc_walktoxy_sub(struct map_session_data *sd)
{
	struct walkpath_data wpd;
	int i;

	nullpo_retr(1, sd);
	
	if(pc_iscloaking(sd))// �N���[�L���O���Čv�Z
		status_calc_pc(sd,0);

	if(path_search(&wpd,sd->bl.m,sd->bl.x,sd->bl.y,sd->to_x,sd->to_y,0))
		return 1;
	memcpy(&sd->walkpath,&wpd,sizeof(wpd));

	clif_walkok(sd);
	sd->state.change_walk_target=0;

	if((i=calc_next_walk_step(sd))>0){
		i = i>>2;
		sd->walktimer=add_timer(gettick()+i,pc_walk,sd->bl.id,0);
	}
	clif_movechar(sd);

	return 0;
}


/*==========================================
 * pc�� �s�v��
 *------------------------------------------
 */
int pc_walktoxy(struct map_session_data *sd,int x,int y)
{

	nullpo_retr(0, sd);

	sd->to_x=x;
	sd->to_y=y;

	if(sd->walktimer != -1 && sd->state.change_walk_target==0){
		// ���ݕ����Ă���Œ��̖ړI�n�ύX�Ȃ̂Ń}�X�ڂ̒��S�ɗ�������
		// timer�֐�����pc_walktoxy_sub���ĂԂ悤�ɂ���
		sd->state.change_walk_target=1;
	} else {
		pc_walktoxy_sub(sd);
	}

	return 0;
}

/*==========================================
 * �� �s��~
 *------------------------------------------
 */
int pc_stop_walking(struct map_session_data *sd,int type)
{
	nullpo_retr(0, sd);

	if(sd->walktimer != -1) {
		delete_timer(sd->walktimer,pc_walk);
		sd->walktimer=-1;
	}
	sd->walkpath.path_len=0;
	sd->to_x = sd->bl.x;
	sd->to_y = sd->bl.y;
	if(type&0x01)
		clif_fixpos(&sd->bl);
	if(type&0x02 && battle_config.pc_damage_delay) {
		unsigned int tick = gettick();
		int delay = status_get_dmotion(&sd->bl);
		if(sd->canmove_tick < tick)
			sd->canmove_tick = tick + delay;
	}

	return 0;
}


/*==========================================
 *
 *------------------------------------------
 */
int pc_movepos(struct map_session_data *sd,int dst_x,int dst_y)
{
	int moveblock;
	int dx,dy;
	int tick = gettick();

	struct walkpath_data wpd;

	nullpo_retr(0, sd);

	if(path_search(&wpd,sd->bl.m,sd->bl.x,sd->bl.y,dst_x,dst_y,0))
		return 1;

	sd->dir = sd->head_dir = map_calc_dir(&sd->bl, dst_x,dst_y);

	dx = dst_x - sd->bl.x;
	dy = dst_y - sd->bl.y;

	moveblock = ( sd->bl.x/BLOCK_SIZE != dst_x/BLOCK_SIZE || sd->bl.y/BLOCK_SIZE != dst_y/BLOCK_SIZE);

	map_foreachinmovearea(clif_pcoutsight,sd->bl.m,sd->bl.x-AREA_SIZE,sd->bl.y-AREA_SIZE,sd->bl.x+AREA_SIZE,sd->bl.y+AREA_SIZE,dx,dy,0,sd);

	skill_unit_move(&sd->bl,tick,0);
	if(moveblock) map_delblock(&sd->bl);
	sd->bl.x = dst_x;
	sd->bl.y = dst_y;
	if(moveblock) map_addblock(&sd->bl);
	skill_unit_move(&sd->bl,tick,1);

	map_foreachinmovearea(clif_pcinsight,sd->bl.m,sd->bl.x-AREA_SIZE,sd->bl.y-AREA_SIZE,sd->bl.x+AREA_SIZE,sd->bl.y+AREA_SIZE,-dx,-dy,0,sd);

	if(sd->status.party_id>0){	// �p�[�e�B�̂g�o���ʒm����
		struct party *p=party_search(sd->status.party_id);
		if(p!=NULL){
			int flag=0;
			map_foreachinmovearea(party_send_hp_check,sd->bl.m,sd->bl.x-AREA_SIZE,sd->bl.y-AREA_SIZE,sd->bl.x+AREA_SIZE,sd->bl.y+AREA_SIZE,-dx,-dy,BL_PC,sd->status.party_id,&flag);
			if(flag)
				sd->party_hp=-1;
		}
	}


	if(!(sd->status.option&0x4000) && sd->status.option&4)	// �N���[�L���O�̏��Ō���
	{
		if(pc_checkskill(sd,AS_CLOAKING) < 3)
			skill_check_cloaking(&sd->bl);
	}
		
	if(map_getcell(sd->bl.m,sd->bl.x,sd->bl.y,CELL_CHKNPC))
		npc_touch_areanpc(sd,sd->bl.m,sd->bl.x,sd->bl.y);
	else
		sd->areanpc_id=0;

	return 0;
}

//
// ����퓬
//
/*==========================================
 * �X�L���̌��� ���L���Ă����ꍇLv���Ԃ�
 *------------------------------------------
 */
int pc_checkskill(struct map_session_data *sd,int skill_id)
{
	if(sd == NULL) return 0;
	if( skill_id>=10000 ){
		struct guild *g;
		if( sd->status.guild_id>0 && (g=guild_search(sd->status.guild_id))!=NULL)
			return guild_checkskill(g,skill_id);
		return 0;
	}

	if(sd->status.skill[skill_id].id == skill_id)
		return (sd->status.skill[skill_id].lv);

	return 0;
}

/*==========================================
 * ����ύX�ɂ��X�L���̌p���`�F�b�N
 * �����F
 *   struct map_session_data *sd	�Z�b�V�����f�[�^
 *   int nameid						�����iID
 * �Ԃ�l�F
 *   0		�ύX�Ȃ�
 *   -1		�X�L��������
 *------------------------------------------
 */
int pc_checkallowskill(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	if( sd->sc_data == NULL )
		return 0;

	if(!(skill_get_weapontype(KN_TWOHANDQUICKEN)&(1<<sd->status.weapon)) && sd->sc_data[SC_TWOHANDQUICKEN].timer!=-1) {	// 2HQ
		status_change_end(&sd->bl,SC_TWOHANDQUICKEN,-1);	// 2HQ������
		return -1;
	}
	if(!(skill_get_weapontype(KN_ONEHAND)&(1<<sd->status.weapon)) && sd->sc_data[SC_ONEHAND].timer!=-1) {	// 1HQ
		status_change_end(&sd->bl,SC_ONEHAND,-1);	// 1HQ������
		return -1;
	}
	if(!(skill_get_weapontype(LK_AURABLADE)&(1<<sd->status.weapon)) && sd->sc_data[SC_AURABLADE].timer!=-1) {	/* �I�[���u���[�h */
		status_change_end(&sd->bl,SC_AURABLADE,-1);	/* �I�[���u���[�h������ */
		return -1;
	}
	if(!(skill_get_weapontype(LK_PARRYING)&(1<<sd->status.weapon)) && sd->sc_data[SC_PARRYING].timer!=-1) {	/* �p���C���O */
		status_change_end(&sd->bl,SC_PARRYING,-1);	/* �p���C���O������ */
		return -1;
	}
	if(!(skill_get_weapontype(LK_CONCENTRATION)&(1<<sd->status.weapon)) && sd->sc_data[SC_CONCENTRATION].timer!=-1) {	/* �R���Z���g���[�V���� */
		status_change_end(&sd->bl,SC_CONCENTRATION,-1);	/* �R���Z���g���[�V���������� */
		return -1;
	}
	if(!(skill_get_weapontype(CR_SPEARQUICKEN)&(1<<sd->status.weapon)) && sd->sc_data[SC_SPEARSQUICKEN].timer!=-1){	// �X�s�A�N�B�b�P��
		status_change_end(&sd->bl,SC_SPEARSQUICKEN,-1);	// �X�s�A�N�C�b�P��������
		return -1;
	}
	if(!(skill_get_weapontype(BS_ADRENALINE)&(1<<sd->status.weapon)) && sd->sc_data[SC_ADRENALINE].timer!=-1){	// �A�h���i�������b�V��
		status_change_end(&sd->bl,SC_ADRENALINE,-1);	// �A�h���i�������b�V��������
		return -1;
	}
	if(!(skill_get_weapontype(BS_ADRENALINE2)&(1<<sd->status.weapon)) && sd->sc_data[SC_ADRENALINE2].timer!=-1){	// �t���A�h���i�������b�V��
		status_change_end(&sd->bl,SC_ADRENALINE2,-1);	// �t���A�h���i�������b�V��������
		return -1;
	}
	
	if((sd->weapontype1 != 0 || sd->weapontype2 != 0) && sd->sc_data[SC_SPURT].timer!=-1){
		status_change_end(&sd->bl,SC_SPURT,-1);	// �삯��STR
		return -1;
	}

	if(sd->status.shield <= 0) {
		if(sd->sc_data[SC_AUTOGUARD].timer!=-1){	// �I�[�g�K�[�h
			status_change_end(&sd->bl,SC_AUTOGUARD,-1);
			return -1;
		}
		if(sd->sc_data[SC_DEFENDER].timer!=-1){	// �f�B�t�F���_�[
			status_change_end(&sd->bl,SC_DEFENDER,-1);
			return -1;
		}
		if(sd->sc_data[SC_REFLECTSHIELD].timer!=-1){ //���t���N�g�V�[���h
			status_change_end(&sd->bl,SC_REFLECTSHIELD,-1);
			return -1;
		}
	}

	return 0;
}


/*==========================================
 * �� ���i�̃`�F�b�N
 *------------------------------------------
 */
int pc_checkequip(struct map_session_data *sd,int pos)
{
	int i;

	nullpo_retr(-1, sd);

	for(i=0;i<11;i++){
		if(pos & equip_pos[i])
			return sd->equip_index[i];
	}

	return -1;
}

/*==========================================
 * �]���E��{�q�E�̌��̐E�Ƃ�Ԃ�
 *------------------------------------------
 */
struct pc_base_job pc_calc_base_job(int b_class)
{
	struct pc_base_job bj;
	//�]����{�q�̏ꍇ�̌��̐E�Ƃ��Z�o����
	/*
	if(b_class < MAX_VALID_PC_CLASS){ //�ʏ�
		bj.job = b_class;
		bj.upper = 0;
	}else if(b_class >= PC_CLASS_BASE2 && b_class < PC_CLASS_BASE3){ //�]���E
		bj.job = b_class - PC_CLASS_BASE2;
		bj.upper = 1;
	}else if(b_class == 23 + PC_CLASS_BASE3 -1){ //�{�q�X�p�m�r
		bj.job = b_class - (PC_CLASS_BASE3 - 1);
		bj.upper = 2;
	}else{ //�{�q�X�p�m�r�ȊO�̗{�q
		bj.job = b_class - PC_CLASS_BASE3 + 1;
		bj.upper = 2;
			
	}
	*/
	if(b_class <= PC_CLASS_SNV)
	{
		bj.job = b_class;
		bj.upper = 0;
	}
	else if(b_class >= PC_CLASS_BASE2 && b_class < PC_CLASS_BASE3)
	{
		bj.job = b_class - PC_CLASS_BASE2;
		bj.upper = 1;
	}
	else if(b_class >= PC_CLASS_BASE3 && b_class< PC_CLASS_SNV3)
	{
		bj.job = b_class - PC_CLASS_BASE3;
		bj.upper = 2;
	}else if(b_class == PC_CLASS_SNV3)
	{
		bj.job   = 23;
		bj.upper = 2;
	}
	else if(b_class>=PC_CLASS_TK && b_class<= PC_CLASS_SL)//�e�R���`
	{
		bj.job = 24 + b_class - PC_CLASS_TK;
		bj.upper = 2;
	}
	

	if(battle_config.enable_upper_class==0){ //conf�Ŗ����ɂȂ��Ă�����upper=0
		bj.upper = 0;
	}

	if(bj.job == 0){
		bj.type = 0;
	}else if(bj.job < 7){
		bj.type = 1;
	}else{
		bj.type = 2;
	}
	
	return bj;
}


/*==========================================
 * PC�̍U�� (timer�֐�)
 *------------------------------------------
 */
int pc_attack_timer(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd;
	struct block_list *bl;
	struct status_change *sc_data;
	short *opt;
	int dist,skill,range;

	sd=map_id2sd(id);
	if(sd == NULL)
		return 0;
	if(sd->attacktimer != tid){
		if(battle_config.error_log)
			printf("pc_attack_timer %d != %d\n",sd->attacktimer,tid);
		return 0;
	}
	sd->attacktimer=-1;

	if(sd->bl.prev == NULL)
		return 0;

	bl=map_id2bl(sd->attacktarget);
	if(bl==NULL || bl->prev == NULL)
		return 0;

	if(bl->type == BL_PC && pc_isdead((struct map_session_data *)bl))
		return 0;

	// ����map�łȂ��Ȃ�U�����Ȃ�
	// PC������łĂ��U�����Ȃ�
	if(sd->bl.m != bl->m || pc_isdead(sd))
		return 0;

	if( sd->opt1>0 || sd->status.option&2 || pc_ischasewalk(sd) )	// �ُ�ȂǂōU���ł��Ȃ�
		return 0;

	if(sd->sc_data[SC_AUTOCOUNTER].timer != -1)
		return 0;
	if(sd->sc_data[SC_BLADESTOP].timer != -1)
		return 0;

	if((opt = status_get_option(bl)) != NULL && *opt&0x46)
		return 0;
	if((sc_data = status_get_sc_data(bl)) != NULL && sc_data[SC_TRICKDEAD].timer != -1)
		return 0;

	if(sd->skilltimer != -1 && pc_checkskill(sd,SA_FREECAST) <= 0)
		return 0;

	if(!battle_config.sdelay_attack_enable && pc_checkskill(sd,SA_FREECAST) <= 0) {
		if(DIFF_TICK(tick , sd->canact_tick) < 0) {
			clif_skill_fail(sd,1,4,0);
			return 0;
		}
	}

	dist = distance(sd->bl.x,sd->bl.y,bl->x,bl->y);
	range = sd->attackrange;
	if(sd->status.weapon != 11) range++;
	if( dist > range ){	// �� ���Ȃ��̂ňړ�
		if(pc_can_reach(sd,bl->x,bl->y))
			clif_movetoattack(sd,bl);
		return 0;
	}

	if(dist <= range && !battle_check_range(&sd->bl,bl,range) ) {
		if(pc_can_reach(sd,bl->x,bl->y) && sd->canmove_tick < tick && sd->sc_data[SC_ANKLE].timer == -1)
			pc_walktoxy(sd,bl->x,bl->y);
		sd->attackabletime = tick + (sd->aspd<<1);
	}
	else {
		if(battle_config.pc_attack_direction_change)
			sd->dir=sd->head_dir=map_calc_dir(&sd->bl, bl->x,bl->y );	// �����ݒ�

		if(sd->walktimer != -1)
			pc_stop_walking(sd,1);

		if(sd->sc_data[SC_COMBO].timer == -1) {
			map_freeblock_lock();
			pc_stop_walking(sd,0);
			sd->attacktarget_lv = battle_weapon_attack(&sd->bl,bl,tick,0);
			if(!(battle_config.pc_cloak_check_type&2) && sd->sc_data[SC_CLOAKING].timer != -1)
				status_change_end(&sd->bl,SC_CLOAKING,-1);
			if(sd->status.pet_id > 0 && sd->pd && sd->petDB && battle_config.pet_attack_support)
				pet_target_check(sd,bl,0);
			map_freeblock_unlock();
			if(sd->skilltimer != -1 && (skill = pc_checkskill(sd,SA_FREECAST)) > 0 ) // �t���[�L���X�g
				sd->attackabletime = tick + ((sd->aspd<<1)*(150 - skill*5)/100);
			else
				sd->attackabletime = tick + (sd->aspd<<1);
		}
		else if(sd->attackabletime <= tick) {
			if(sd->skilltimer != -1 && (skill = pc_checkskill(sd,SA_FREECAST)) > 0 ) // �t���[�L���X�g
				sd->attackabletime = tick + ((sd->aspd<<1)*(150 - skill*5)/100);
			else
				sd->attackabletime = tick + (sd->aspd<<1);
		}
		if(sd->attackabletime <= tick) sd->attackabletime = tick + (battle_config.max_aspd<<1);
	}

	if(sd->state.attack_continue) {
		sd->attacktimer=add_timer(sd->attackabletime,pc_attack_timer,sd->bl.id,0);
	}

	return 0;
}


/*==========================================
 * �U���v��
 * type��1�Ȃ�p���U��
 *------------------------------------------
 */
int pc_attack(struct map_session_data *sd,int target_id,int type)
{
	struct block_list *bl;
	int d;

	nullpo_retr(0, sd);

	bl=map_id2bl(target_id);
	if(bl==NULL)
		return 1;
	if(battle_check_target(&sd->bl,bl,BCT_ENEMY)<=0)
		return 1;
	if(sd->attacktimer != -1)
		pc_stopattack(sd);
	sd->attacktarget=target_id;
	sd->state.attack_continue=type;

	d=DIFF_TICK(sd->attackabletime,gettick());
	if(d>0 && d<2000){	// �U��delay��
		sd->attacktimer=add_timer(sd->attackabletime,pc_attack_timer,sd->bl.id,0);
	} else {
		// �{��timer�֐��Ȃ̂ň��������킹��
		pc_attack_timer(-1,gettick(),sd->bl.id,0);
	}

	return 0;
}


/*==========================================
 * �p���U����~
 *------------------------------------------
 */
int pc_stopattack(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	if(sd->attacktimer != -1) {
		delete_timer(sd->attacktimer,pc_attack_timer);
		sd->attacktimer=-1;
	}
	sd->attacktarget=0;
	sd->state.attack_continue=0;

	return 0;
}


int pc_checkbaselevelup(struct map_session_data *sd)
{
	int next = pc_nextbaseexp(sd);

	nullpo_retr(0, sd);

	if(sd->status.base_exp >= next && next > 0){
		struct pc_base_job 	s_class = pc_calc_base_job(sd->status.class);

		// base�����x���A�b�v����
		sd->status.base_exp -= next;

		sd->status.base_level ++;
		sd->status.status_point += (sd->status.base_level+14) / 5 ;
		clif_updatestatus(sd,SP_STATUSPOINT);
		clif_updatestatus(sd,SP_BASELEVEL);
		clif_updatestatus(sd,SP_NEXTBASEEXP);
		status_calc_pc(sd,0);
		pc_heal(sd,sd->status.max_hp,sd->status.max_sp);

		//�X�p�m�r�̓L���G�A�C���|�A�}�j�s�A�O���A�T�t����������
		if(s_class.job == 23){
			status_change_start(&sd->bl,SkillStatusChangeTable[PR_KYRIE],10,0,0,0,skill_get_time(PR_KYRIE,10),0 );
			status_change_start(&sd->bl,SkillStatusChangeTable[PR_IMPOSITIO],5,0,0,0,skill_get_time(PR_IMPOSITIO,5),0 );
			status_change_start(&sd->bl,SkillStatusChangeTable[PR_MAGNIFICAT],5,0,0,0,skill_get_time(PR_MAGNIFICAT,5),0 );
			status_change_start(&sd->bl,SkillStatusChangeTable[PR_GLORIA],5,0,0,0,skill_get_time(PR_GLORIA,5),0 );
			status_change_start(&sd->bl,SkillStatusChangeTable[PR_SUFFRAGIUM],3,0,0,0,skill_get_time(PR_SUFFRAGIUM,3),0 );
		}

		clif_misceffect(&sd->bl,0);
		//���x���A�b�v�����̂Ńp�[�e�B�[�����X�V����
		//(�����͈̓`�F�b�N)
		party_send_movemap(sd);
		return 1;
	}

	return 0;
}


int pc_checkjoblevelup(struct map_session_data *sd)
{
	int next = pc_nextjobexp(sd);

	nullpo_retr(0, sd);

	if(sd->status.job_exp >= next && next > 0){
		// job�����x���A�b�v����
		sd->status.job_exp -= next;
		sd->status.job_level ++;
		clif_updatestatus(sd,SP_JOBLEVEL);
		clif_updatestatus(sd,SP_NEXTJOBEXP);
		sd->status.skill_point ++;
		clif_updatestatus(sd,SP_SKILLPOINT);
		status_calc_pc(sd,0);

		clif_misceffect(&sd->bl,1);
		return 1;
	}

	return 0;
}


/*==========================================
 * �o���l�擾
 *------------------------------------------
 */
int pc_gainexp(struct map_session_data *sd,int base_exp,int job_exp)
{
	char output[128];
	int next;
	atn_bignumber per;
	nullpo_retr(0, sd);

	if(sd->bl.prev == NULL || pc_isdead(sd))
		return 0;

	if(sd->sc_data[SC_RICHMANKIM].timer != -1) {
		atn_bignumber bexp, jexp;
		bexp = ((atn_bignumber)base_exp)*(125 + sd->sc_data[SC_RICHMANKIM].val1*25)/100;
		jexp = ((atn_bignumber)job_exp )*(125 + sd->sc_data[SC_RICHMANKIM].val1*25)/100;
		base_exp = (bexp>0x7fffffff) ? 0x7fffffff: (int)bexp;
		job_exp = (bexp>0x7fffffff) ? 0x7fffffff: (int)jexp;

//		base_exp += base_exp*(25 + sd->sc_data[SC_RICHMANKIM].val1*25)/100;
//		job_exp += job_exp*(25 + sd->sc_data[SC_RICHMANKIM].val1*25)/100;
	}

	if(sd->status.guild_id>0){	// �M���h�ɏ�[
		base_exp-=guild_payexp(sd,base_exp);
		if(base_exp < 0)
			base_exp = 0;
	}

	if(battle_config.disp_experience){
		snprintf(output, sizeof output,
			"Base:%d Job:%d �̌o���l���l��",base_exp,job_exp);
		clif_disp_onlyself(sd,output,strlen(output));
	}

//------------- Base ----------------
/*
	sd->status.base_exp += base_exp;
	if(sd->status.base_exp < 0)
		sd->status.base_exp = 0;

	if(battle_config.next_exp_limit==0){
		next = pc_nextbaseexp(sd);
		pc_checkbaselevelup(sd);
		if (sd->status.base_exp>=next)
			sd->status.base_exp = next-1;
	} else {
		while(pc_checkbaselevelup(sd)) ;
	}
*/
	per = battle_config.next_exp_limit;
	if(base_exp>0){
		if((next=pc_nextbaseexp(sd))>0){
			while((sd->status.base_exp + base_exp) >= next){	// LvUP
				int temp_exp;
				temp_exp = next - sd->status.base_exp;
				if( (per-(100-(atn_bignumber)sd->status.base_exp*100/next)) <0)
					break;
				per -= (100-(atn_bignumber)sd->status.base_exp*100/next);
				sd->status.base_exp = next;
				if(!pc_checkbaselevelup(sd) || (next = pc_nextbaseexp(sd))<=0) break;
				base_exp -= temp_exp;
			}
			if((next=pc_nextbaseexp(sd))>0 && ((atn_bignumber)base_exp * 100 / next) > per)
				sd->status.base_exp = (int)( next * per / 100 );
			else
				sd->status.base_exp += base_exp;

			if(sd->status.base_exp < 0)
				sd->status.base_exp = 0;
			pc_checkbaselevelup(sd);
		}else{
			sd->status.base_exp += base_exp;
		}
		clif_updatestatus(sd,SP_BASEEXP);
	}
//------------- Job ----------------
/*
	sd->status.job_exp += job_exp;
	if(sd->status.job_exp < 0)
		sd->status.job_exp = 0;

	if(battle_config.next_exp_limit==0){
		next = pc_nextjobexp(sd);
		pc_checkjoblevelup(sd);
		if (sd->status.job_exp>=next)
			sd->status.job_exp = next-1;
	} else {
		while(pc_checkjoblevelup(sd)) ;
	}
*/
	per = battle_config.next_exp_limit;
	if(job_exp>0){
		if((next = pc_nextjobexp(sd))>0){
			while((sd->status.job_exp + job_exp) >= next){	// LvUP
				int temp_exp;
				temp_exp = next - sd->status.job_exp;
				if( (per-(100-(atn_bignumber)sd->status.job_exp*100 / next))<=0)
					break;
				per -= (100-(atn_bignumber)sd->status.job_exp*100 / next);
				sd->status.job_exp = next;
				if(!pc_checkjoblevelup(sd) || (next = pc_nextjobexp(sd))<=0) break;
				job_exp -= temp_exp;
			}
			if((next = pc_nextjobexp(sd))>0 && ((atn_bignumber)job_exp * 100 / next) > per)
				sd->status.job_exp = (int)( next * per / 100 );
			else
				sd->status.job_exp += job_exp;

			if(sd->status.job_exp < 0)
				sd->status.job_exp = 0;
			pc_checkjoblevelup(sd);
		}else{
			sd->status.job_exp += job_exp;
		}
		clif_updatestatus(sd,SP_JOBEXP);
	}

	return 0;
}

/*==========================================
 * base level���K�v�o���l�v�Z
 *------------------------------------------
 */
int pc_nextbaseexp(struct map_session_data *sd)
{
	int i;

	nullpo_retr(0, sd);

	if(sd->status.base_level>=MAX_LEVEL || sd->status.base_level<=0)
		return 0;

	if(sd->status.class==0) i=0;
	else if(sd->status.class<=6) i=1;
	else if(sd->status.class<=22) i=2;
	else if(sd->status.class==23) i=3;
	else if(sd->status.class==PC_CLASS_BASE2) 	   	i=4;
	else if(sd->status.class<=PC_CLASS_BASE2 + 6)  	i=5;
	else if(sd->status.class<=PC_CLASS_BASE2 + 22) 	i=6;
	else if(sd->status.class == PC_CLASS_BASE3)    	i=0;//�{�q�m�r
	else if(sd->status.class<= PC_CLASS_BASE3+6)   	i=1;//�{�q�ꎟ
	else if(sd->status.class<= PC_CLASS_BASE3+21)  	i=2;//�{�q��
	else if(sd->status.class== PC_CLASS_SNV3)  	   	i=3;//�{�q�X�p�m�r
	else if(sd->status.class<= PC_CLASS_SL)      	i=1;//�ǉ��E �]���O�̒l
	else  i=1;//����ȊO�Ȃ�]���O�@�ǉ�����Ă��E�҂Ȃǂ����炢����
	
	return exp_table[i][sd->status.base_level-1];
}

/*==========================================
 * job level���K�v�o���l�v�Z
 *------------------------------------------
 */
int pc_nextjobexp(struct map_session_data *sd)
{
	int i;

	nullpo_retr(0, sd);

	if(sd->status.job_level>=MAX_LEVEL || sd->status.job_level<=0)
		return 0;

	if(sd->status.class==0) i=7;
	else if(sd->status.class<=6) i=8;
	else if(sd->status.class<=22) i=9;
	else if(sd->status.class==23) i=10;
	else if(sd->status.class==PC_CLASS_BASE2) 		i=11;
	else if(sd->status.class<=PC_CLASS_BASE2 + 6)  	i=12;
	else if(sd->status.class<=PC_CLASS_BASE2 + 22) 	i=13;
	else if(sd->status.class == PC_CLASS_BASE3) 	i=7; //�{�q�m�r
	else if(sd->status.class<= PC_CLASS_BASE3+6) 	i=8; //�{�q�ꎟ
	else if(sd->status.class<= PC_CLASS_BASE3+21) 	i=9;//�{�q��
	else if(sd->status.class== PC_CLASS_SNV3) 		i=10;//�{�q�X�p�m�r
	else if(sd->status.class== PC_CLASS_TK)   		i=8;//�e�R��
	else if(sd->status.class<= PC_CLASS_SG2) 		i=9;//�P���Z�C
	else if(sd->status.class== PC_CLASS_SL) 		i=14;//�\�E�������J�[
	else i = 9;//����ȊO�Ȃ�񎟃e�[�u���@�ǉ�����Ă��E�҂Ȃǂ����炢����

	return exp_table[i][sd->status.job_level-1];
}

/*==========================================
 * �K�v�X�e�[�^�X�|�C���g�v�Z
 *------------------------------------------
 */
int pc_need_status_point(struct map_session_data *sd,int type)
{
	int val;

	nullpo_retr(-1, sd);

	if(type<SP_STR || type>SP_LUK)
		return -1;
	val =
		type==SP_STR ? sd->status.str :
		type==SP_AGI ? sd->status.agi :
		type==SP_VIT ? sd->status.vit :
		type==SP_INT ? sd->status.int_:
		type==SP_DEX ? sd->status.dex : sd->status.luk;

	return (val+9)/10+1;
}


/*==========================================
 * �\�͒l����
 *------------------------------------------
 */
int pc_statusup(struct map_session_data *sd,int type)
{
	int need;
	int val=0;

	nullpo_retr(0, sd);

	need=pc_need_status_point(sd,type);
	if(type<SP_STR || type>SP_LUK || need<0 || need>sd->status.status_point){
		clif_statusupack(sd,type,0,0);
		return 1;
	}
	switch(type){
	case SP_STR:
		if(sd->status.str >= battle_config.max_parameter_str) {
			clif_statusupack(sd,type,0,0);
			return 1;
		}
		if(pc_isadoptee(sd) && sd->status.str >= 79){//�{�q
			clif_statusupack(sd,type,0,0);
			return 1;
		}
		val= ++sd->status.str;
		break;
	case SP_AGI:
		if(sd->status.agi >= battle_config.max_parameter_agi) {
			clif_statusupack(sd,type,0,0);
			return 1;
		}
		
		if(pc_isadoptee(sd) && sd->status.agi >= 79){//�{�q
			clif_statusupack(sd,type,0,0);
			return 1;
		}
		val= ++sd->status.agi;
		break;
	case SP_VIT:
		if(sd->status.vit >= battle_config.max_parameter_vit) {
			clif_statusupack(sd,type,0,0);
			return 1;
		}
		if(pc_isadoptee(sd) && sd->status.vit >= 79){//�{�q
			clif_statusupack(sd,type,0,0);
			return 1;
		}
		val= ++sd->status.vit;
		break;
	case SP_INT:
		if(sd->status.int_ >= battle_config.max_parameter_int) {
			clif_statusupack(sd,type,0,0);
			return 1;
		}
		if(pc_isadoptee(sd) && sd->status.int_ >= 79){//�{�q
			clif_statusupack(sd,type,0,0);
			return 1;
		}
		val= ++sd->status.int_;
		break;
	case SP_DEX:
		if(sd->status.dex >= battle_config.max_parameter_dex) {
			clif_statusupack(sd,type,0,0);
			return 1;
		}
		if(pc_isadoptee(sd) && sd->status.dex >= 79){//�{�q
			clif_statusupack(sd,type,0,0);
			return 1;
		}
		val= ++sd->status.dex;
		break;
	case SP_LUK:
		if(sd->status.luk >= battle_config.max_parameter_luk) {
			clif_statusupack(sd,type,0,0);
			return 1;
		}
		if(pc_isadoptee(sd) && sd->status.luk >= 79){//�{�q
			clif_statusupack(sd,type,0,0);
			return 1;
		}
		val= ++sd->status.luk;
		break;
	}
	sd->status.status_point-=need;
	if(need!=pc_need_status_point(sd,type)){
		clif_updatestatus(sd,type-SP_STR+SP_USTR);
	}
	clif_updatestatus(sd,SP_STATUSPOINT);
	clif_updatestatus(sd,type);
	status_calc_pc(sd,0);
	clif_statusupack(sd,type,1,val);

	return 0;
}

//�]���E�����������
int pc_isupper(struct map_session_data *sd)
{
	nullpo_retr(0, sd);
	if(sd->status.class >= PC_CLASS_NV2 && sd->status.class < PC_CLASS_NV3)
		return 1;
	return 0;
}
//�{�q�����肷��
int pc_isadoptee(struct map_session_data *sd)
{
	nullpo_retr(0, sd);
	if(sd->status.class >= PC_CLASS_NV3 && sd->status.class <= PC_CLASS_SNV3)
		return 1;
	return 0;
}

/*==========================================
 * �\�͒l����
 *------------------------------------------
 */
int pc_statusup2(struct map_session_data *sd,int type,int val)
{
	nullpo_retr(0, sd);

	if(type<SP_STR || type>SP_LUK){
		clif_statusupack(sd,type,0,0);
		return 1;
	}
	switch(type){
	case SP_STR:
		if(sd->status.str + val >= battle_config.max_parameter)
			val = battle_config.max_parameter;
		else if(sd->status.str + val < 1)
			val = 1;
		else
			val += sd->status.str;
		sd->status.str = val;
		break;
	case SP_AGI:
		if(sd->status.agi + val >= battle_config.max_parameter)
			val = battle_config.max_parameter;
		else if(sd->status.agi + val < 1)
			val = 1;
		else
			val += sd->status.agi;
		sd->status.agi = val;
		break;
	case SP_VIT:
		if(sd->status.vit + val >= battle_config.max_parameter)
			val = battle_config.max_parameter;
		else if(sd->status.vit + val < 1)
			val = 1;
		else
			val += sd->status.vit;
		sd->status.vit = val;
		break;
	case SP_INT:
		if(sd->status.int_ + val >= battle_config.max_parameter)
			val = battle_config.max_parameter;
		else if(sd->status.int_ + val < 1)
			val = 1;
		else
			val += sd->status.int_;
		sd->status.int_ = val;
		break;
	case SP_DEX:
		if(sd->status.dex + val >= battle_config.max_parameter)
			val = battle_config.max_parameter;
		else if(sd->status.dex + val < 1)
			val = 1;
		else
			val += sd->status.dex;
		sd->status.dex = val;
		break;
	case SP_LUK:
		if(sd->status.luk + val >= battle_config.max_parameter)
			val = battle_config.max_parameter;
		else if(sd->status.luk + val < 1)
			val = 1;
		else
			val = sd->status.luk + val;
		sd->status.luk = val;
		break;
	}
	clif_updatestatus(sd,type-SP_STR+SP_USTR);
	clif_updatestatus(sd,type);
	status_calc_pc(sd,0);
	clif_statusupack(sd,type,1,val);

	return 0;
}

/*==========================================
 * �X�L���|�C���g����U��
 *------------------------------------------
 */
int pc_skillup(struct map_session_data *sd,int skill_num)
{
	nullpo_retr(0, sd);

	if( skill_num>=10000 ){
		guild_skillup(sd,skill_num,0);
		return 0;
	}

	if( sd->status.skill_point>0 &&
		sd->status.skill[skill_num].id!=0 &&
		sd->status.skill[skill_num].lv < skill_get_max(skill_num) )
	{
		sd->status.skill[skill_num].lv++;
		sd->status.skill_point--;
		status_calc_pc(sd,0);
		clif_skillup(sd,skill_num);
		clif_updatestatus(sd,SP_SKILLPOINT);
		clif_skillinfoblock(sd);
	}

	return 0;
}

/*==========================================
 * /allskill
 *------------------------------------------
 */
int pc_allskillup(struct map_session_data *sd)
{
	int i,id;
	int c=0, s=0;
	//�]����{�q�̏ꍇ�̌��̐E�Ƃ��Z�o����
	struct pc_base_job s_class;

	nullpo_retr(0, sd);

	s_class = pc_calc_base_job(sd->status.class);
	c = s_class.job;
	s = (s_class.upper==1) ? 1 : 0 ; //�]���ȊO�͒ʏ�̃X�L���H

	for(i=0;i<MAX_SKILL;i++){
		sd->status.skill[i].id=0;
		if (sd->status.skill[i].flag && sd->status.skill[i].flag != 13){	// card�X�L���Ȃ�A
			sd->status.skill[i].lv=(sd->status.skill[i].flag==1)?0:sd->status.skill[i].flag-2;	// �{����lv��
			sd->status.skill[i].flag=0;	// flag��0�ɂ��Ă���
		}
	}

	if (battle_config.gm_allskill > 0 && pc_isGM(sd) >= battle_config.gm_allskill){
		// �S�ẴX�L��
		for(i=1;i<158;i++)
			sd->status.skill[i].lv=skill_get_max(i);
		for(i=210;i<291;i++)
			sd->status.skill[i].lv=skill_get_max(i);
		if(battle_config.gm_allskill_addabra) {
			for(i=291;i<304;i++)
				sd->status.skill[i].lv=skill_get_max(i);
		}
		for(i=304;i<MAX_SKILL;i++)
			sd->status.skill[i].lv=skill_get_max(i);
	}
	else {
		for(i=0;(id=skill_tree[s][c][i].id)>0;i++){
			if(sd->status.skill[id].id==0 && (!(skill_get_inf2(id)&0x01) || battle_config.quest_skill_learn) )
				sd->status.skill[id].lv=skill_get_max(id);
		}
	}
	status_calc_pc(sd,0);

	return 0;
}

/*==========================================
 * /resetstate
 *------------------------------------------
 */
int pc_resetstate(struct map_session_data* sd)
{
	#define sumsp(a) ((a)*((a-2)/10+2) - 5*((a-2)/10)*((a-2)/10) - 6*((a-2)/10) -2)
	int add=0;

	nullpo_retr(0, sd);

	add += sumsp(sd->status.str);
	add += sumsp(sd->status.agi);
	add += sumsp(sd->status.vit);
	add += sumsp(sd->status.int_);
	add += sumsp(sd->status.dex);
	add += sumsp(sd->status.luk);
	sd->status.status_point+=add;

	clif_updatestatus(sd,SP_STATUSPOINT);

	sd->status.str=1;
	sd->status.agi=1;
	sd->status.vit=1;
	sd->status.int_=1;
	sd->status.dex=1;
	sd->status.luk=1;

	clif_updatestatus(sd,SP_STR);
	clif_updatestatus(sd,SP_AGI);
	clif_updatestatus(sd,SP_VIT);
	clif_updatestatus(sd,SP_INT);
	clif_updatestatus(sd,SP_DEX);
	clif_updatestatus(sd,SP_LUK);

	status_calc_pc(sd,0);

	return 0;
}


/*==========================================
 * /resetskill
 *------------------------------------------
 */
int pc_resetskill(struct map_session_data* sd)
{
	int  i,skill;

	nullpo_retr(0, sd);

	for(i=1;i<MAX_SKILL;i++){
		if( (skill = pc_checkskill(sd,i)) > 0) {
			if(!(skill_get_inf2(i)&0x01) || battle_config.quest_skill_learn) {
				if(!sd->status.skill[i].flag)
					sd->status.skill_point += skill;
				else if(sd->status.skill[i].flag > 2 && sd->status.skill[i].flag != 13) {
					sd->status.skill_point += (sd->status.skill[i].flag - 2);
				}
				sd->status.skill[i].lv = 0;
			}
			else if(battle_config.quest_skill_reset)
				sd->status.skill[i].lv = 0;
			sd->status.skill[i].flag = 0;
		}
		else
			sd->status.skill[i].lv = 0;
	}
	clif_updatestatus(sd,SP_SKILLPOINT);
	clif_skillinfoblock(sd);
	status_calc_pc(sd,0);

	return 0;
}


/*==========================================
 * pc�Ƀ_���[�W��^����
 *------------------------------------------
 */
int pc_damage(struct block_list *src,struct map_session_data *sd,int damage)
{
	int i=0,j=0;
	struct pc_base_job s_class;
	int raise_flag = 0;
	int raise_hp_per = 0;
	int raise_sp_per = 0;
	int raise_sp_rec_flag = 0;

	nullpo_retr(0, sd);

	//�]����{�q�̏ꍇ�̌��̐E�Ƃ��Z�o����
	s_class = pc_calc_base_job(sd->status.class);
	// ���Ɏ���ł����疳��
	if(pc_isdead(sd))
		return 0;
	// �����Ă��痧���オ��
	if(pc_issit(sd)) {
		pc_setstand(sd);
		skill_gangsterparadise(sd,0);
	}

	// �� ���Ă����瑫���~�߂�
	if(sd->sc_data[SC_ENDURE].timer == -1 && !sd->special_state.infinite_endure)
		pc_stop_walking(sd,battle_config.pc_hit_stop_type);
	
	// ���t/�_���X�̒��f
	if(damage > sd->status.max_hp>>2)
		skill_stop_dancing(&sd->bl,0);

	sd->status.hp-=damage;
	if(sd->status.pet_id > 0 && sd->pd && sd->petDB && battle_config.pet_damage_support)
		pet_target_check(sd,src,1);

	if(sd->status.option&2)
		status_change_end(&sd->bl, SC_HIDING, -1);
	if(pc_iscloaking(sd))
		status_change_end(&sd->bl, SC_CLOAKING, -1);
	if(pc_ischasewalk(sd))
		status_change_end(&sd->bl, SC_CHASEWALK, -1);

	//�G�̍U�����󂯂�ƈ��m���ő���������
	if(sd->loss_equip_flag&0x1000 && damage > 0)//���@�ł�����
	{
		int i;
		for(i = 0;i<11;i++)
		{
			if(atn_rand()%10000 < sd->break_myequip_rate_when_hit[i])
			{
				pc_break_equip2(sd,(unsigned short)i);
			}
		}
	}
	
	//�G�̍U�����󂯂�ƈ��m���ő���������
	if(sd->loss_equip_flag&0x0020 && damage > 0)
	{
		for(i = 0;i<11;i++)
		{
			if(atn_rand()%10000 < sd->loss_equip_rate_when_hit[i])
			{
				pc_lossequipitem(sd,i,0);
			}
		}
	}

	if(sd->status.hp>0){
		// �܂������Ă���Ȃ�HP�X�V
		clif_updatestatus(sd,SP_HP);
		if(sd->status.hp<sd->status.max_hp>>2 && sd->sc_data && sd->sc_data[SC_AUTOBERSERK].timer!=-1 && pc_checkskill(sd,SM_AUTOBERSERK)>0 &&
			(sd->sc_data[SC_PROVOKE].timer==-1 || sd->sc_data[SC_PROVOKE].val2==0 ))
			// �I�[�g�o�[�T�[�N����
			status_change_start(&sd->bl,SC_PROVOKE,10,1,0,0,0,0);

		return 0;
	}
	//�X�p�m�r��Exp99%��HP��0�ɂȂ��HP���񕜂��ċ�����ԂɂȂ�
	if(s_class.job == 23 && pc_nextbaseexp(sd) && 100*sd->status.base_exp/pc_nextbaseexp(sd)>=99 && sd->sc_data && sd->sc_data[SC_STEELBODY].timer==-1){
		clif_skill_nodamage(&sd->bl,&sd->bl,MO_STEELBODY,5,1);
		status_change_start(&sd->bl,SkillStatusChangeTable[MO_STEELBODY],5,0,0,0,skill_get_time(MO_STEELBODY,5),0 );
		sd->status.hp = sd->status.max_hp;
		clif_updatestatus(sd,SP_HP);
		return 0;
	}
	
	//���S�O����
	
	//�J�C�[��
	if(sd->sc_data && sd->sc_data[SC_KAIZEL].timer!=-1)// && map[sd->bl.m].flag.gvg==0 && map[sd->bl.m].flag.pvp==0)
	{
		sd->status.hp = 1;
		clif_skill_nodamage(&sd->bl,&sd->bl,PR_KYRIE,sd->sc_data[SC_KAIZEL].val1,1);
		status_change_start(&sd->bl,SC_KYRIE,sd->sc_data[SC_KAIZEL].val1,0,0,0,180000,0);
		status_change_end(&sd->bl,SC_KAIZEL,-1);	
			clif_updatestatus(sd,SP_HP);
		return 0;
	}
	
	//�����h��
	if(atn_rand()%10000 < sd->autoraise.rate)
	{
		raise_flag = 1;
		raise_hp_per    	=  sd->autoraise.hp_per;
		raise_sp_per   	 	=  sd->autoraise.sp_per;
		raise_sp_rec_flag   =  sd->autoraise.flag;
	}
	//�A�C�e������
	if(sd->loss_equip_flag&0x0001)
	{
		for(i = 0;i<11;i++)
		{
			if(atn_rand()%10000 < sd->loss_equip_rate_when_die[i])
			{
				pc_lossequipitem(sd,i,0);
			}
		}
	}
	
	//�h��
	if(raise_flag)
	{
		//����ɂ����̂Ń��U�̃G�t�F�N�g
		clif_skill_nodamage(&sd->bl,&sd->bl,ALL_RESURRECTION,4,1);
		//HPSP��
		sd->status.hp = sd->status.max_hp*raise_hp_per/100;
		if(sd->status.hp < 1) sd->status.hp = 1;
		if(sd->status.hp > sd->status.max_hp) sd->status.hp = sd->status.max_hp;
		clif_updatestatus(sd,SP_HP);
		
		if(raise_sp_rec_flag){
			sd->status.sp = sd->status.max_sp*raise_sp_per/100;
			if(sd->status.sp < 0) sd->status.sp = 0;
			if(sd->status.sp > sd->status.max_sp) sd->status.sp = sd->status.max_sp;
			clif_updatestatus(sd,SP_SP);
		}
		return 0;
	}
	//
	sd->status.hp = 0;
	pc_setdead(sd);
	//
	if(sd->vender_id)
		vending_closevending(sd);

	if(sd->status.pet_id > 0 && sd->pd) {
		if(sd->petDB) {
			sd->pet.intimate -= sd->petDB->die;
			if(sd->pet.intimate < 0)
				sd->pet.intimate = 0;
			clif_send_petdata(sd,1,sd->pet.intimate);
		}
	}

	pc_stop_walking(sd,0);
	skill_castcancel(&sd->bl,0);	// �r���̒��~
	clif_clearchar_area(&sd->bl,1);
	skill_unit_move(&sd->bl,gettick(),0);
	if(sd->sc_data[SC_BLADESTOP].timer!=-1)//���n�͎��O�ɉ���
		status_change_end(&sd->bl,SC_BLADESTOP,-1);
	pc_setglobalreg(sd,"PC_DIE_COUNTER",++sd->die_counter); //���ɃJ�E���^�[��������
	skill_status_change_clear(&sd->bl,0);	// �X�e�[�^�X�ُ����������
	clif_updatestatus(sd,SP_HP);
	status_calc_pc(sd,0);

	if(battle_config.bone_drop==2
		|| (battle_config.bone_drop==1 && map[sd->bl.m].flag.pvp)){	// �h�N���h���b�v
		struct item item_tmp;
		memset(&item_tmp,0,sizeof(item_tmp));
		if(battle_config.bone_drop_itemid) item_tmp.nameid=battle_config.bone_drop_itemid;
		else item_tmp.nameid = 7005;
		item_tmp.identify=1;
		item_tmp.card[0]=0x00fe;
		item_tmp.card[1]=0;
		*((unsigned long *)(&item_tmp.card[2]))=sd->char_id;	/* �L����ID */
		map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
	}

	for(i=0;i<5;i++)
		if(sd->dev.val1[i]){
			status_change_end(&map_id2sd(sd->dev.val1[i])->bl,SC_DEVOTION,-1);
			sd->dev.val1[i] = sd->dev.val2[i]=0;
		}

	if(battle_config.death_penalty_type&1) {
		if(s_class.job != 0 && !map[sd->bl.m].flag.nopenalty && !map[sd->bl.m].flag.gvg){
			if(battle_config.death_penalty_type&2 && battle_config.death_penalty_base > 0)
				sd->status.base_exp -= (int)((atn_bignumber)pc_nextbaseexp(sd) * battle_config.death_penalty_base/10000);
			else if(battle_config.death_penalty_base > 0) {
				if(pc_nextbaseexp(sd) > 0)
					sd->status.base_exp -= (int)((atn_bignumber)sd->status.base_exp * battle_config.death_penalty_base/10000);
			}
			if(sd->status.base_exp < 0)
				sd->status.base_exp = 0;
			clif_updatestatus(sd,SP_BASEEXP);

			if(battle_config.death_penalty_type&2 && battle_config.death_penalty_job > 0)
				sd->status.job_exp -= (int)((atn_bignumber)pc_nextjobexp(sd) * battle_config.death_penalty_job/10000);
			else if(battle_config.death_penalty_job > 0) {
				if(pc_nextjobexp(sd) > 0)
					sd->status.job_exp -= (int)((atn_bignumber)sd->status.job_exp * battle_config.death_penalty_job/10000);
			}
			if(sd->status.job_exp < 0)
				sd->status.job_exp = 0;
			clif_updatestatus(sd,SP_JOBEXP);
		}
	}

	// pvp
	if( map[sd->bl.m].flag.pvp){
		//�����L���O�v�Z
		if(!map[sd->bl.m].flag.pvp_nocalcrank){
			sd->pvp_point-=5;
			if(src && src->type==BL_PC )
				((struct map_session_data *)src)->pvp_point++;
		}
		//�i�C�g���A���[�h�A�C�e���h���b�v
		if(map[sd->bl.m].flag.pvp_nightmaredrop){
			for(j=0;j<MAX_DROP_PER_MAP;j++){
				int id = map[sd->bl.m].drop_list[j].drop_id;
				int type = map[sd->bl.m].drop_list[j].drop_type;
				int per = map[sd->bl.m].drop_list[j].drop_per;
				if(id == 0)
					continue;
				if(id == -1){//�����_���h���b�v
					int eq_num=0,eq_n[MAX_INVENTORY];
					memset(eq_n,0,sizeof(eq_n));
					//�悸�������Ă���A�C�e�������J�E���g
					for(i=0;i<MAX_INVENTORY;i++){
						int k;
						if( (type == 1 && !sd->status.inventory[i].equip)
							|| (type == 2 && sd->status.inventory[i].equip)
							||  type == 3){
							//InventoryIndex���i�[
							for(k=0;k<MAX_INVENTORY;k++){
								if(eq_n[k] <= 0){
									eq_n[k]=i;
									break;
								}
							}
							eq_num++;
						}
					}
					if(eq_num > 0){
						int n = eq_n[atn_rand()%eq_num];//�Y���A�C�e���̒����烉���_��
						if(atn_rand()%10000 < per){
							if(sd->status.inventory[n].equip)
								pc_unequipitem(sd,n,0);
							pc_dropitem(sd,n,1);
						}
					}
				}
				else if(id > 0){
					for(i=0;i<MAX_INVENTORY;i++){
						if(sd->status.inventory[i].nameid == id//ItemID����v���Ă���
							&& atn_rand()%10000 < per//�h���b�v�������OK��
							&& ((type == 1 && !sd->status.inventory[i].equip)//�^�C�v�����OK�Ȃ�h���b�v
								|| (type == 2 && sd->status.inventory[i].equip)
								|| type == 3)
							){
							if(sd->status.inventory[i].equip)
								pc_unequipitem(sd,i,0);
							pc_dropitem(sd,i,1);
							break;
						}
					}
				}
			}
			pc_setdead(sd);
		}
		// ��������
		if( sd->pvp_point < 0 ){
			sd->pvp_point=0;
			pc_setstand(sd);
			pc_setrestartvalue(sd,3);
			pc_setpos(sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,0);
		}
	}
	//GvG
	if(map[sd->bl.m].flag.gvg){
		pc_setstand(sd);
		pc_setrestartvalue(sd,3);
		pc_setpos(sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,0);
	}

	return 0;
}


//
// script�� �A
//
/*==========================================
 * script�pPC�X�e�[�^�X�ǂݏo��
 *------------------------------------------
 */
int pc_readparam(struct map_session_data *sd,int type)
{
	int val=0;
	struct pc_base_job s_class;
	
	nullpo_retr(0, sd);

	s_class = pc_calc_base_job(sd->status.class);

	switch(type){
	case SP_SKILLPOINT:
		val= sd->status.skill_point;
		break;
	case SP_STATUSPOINT:
		val= sd->status.status_point;
		break;
	case SP_ZENY:
		val= sd->status.zeny;
		break;
	case SP_BASELEVEL:
		val= sd->status.base_level;
		break;
	case SP_JOBLEVEL:
		val= sd->status.job_level;
		break;
	case SP_CLASS:
		val= s_class.job;
		break;
	case SP_UPPER:
		val= s_class.upper;
		break;
	case SP_SEX:
		val= sd->sex;
		break;
	case SP_WEIGHT:
		val= sd->weight;
		break;
	case SP_MAXWEIGHT:
		val= sd->max_weight;
		break;
	case SP_BASEEXP:
		val= sd->status.base_exp;
		break;
	case SP_JOBEXP:
		val= sd->status.job_exp;
		break;
	case SP_NEXTBASEEXP:
		val= pc_nextbaseexp(sd);
		break;
	case SP_NEXTJOBEXP:
		val= pc_nextjobexp(sd);
		break;
	case SP_HP:
		val= sd->status.hp;
		break;
	case SP_MAXHP:
		val= sd->status.max_hp;
		break;
	case SP_SP:
		val= sd->status.sp;
		break;
	case SP_MAXSP:
		val= sd->status.max_sp;
		break;
	case SP_PARTNER:
		val= sd->status.partner_id;
		break;
	case SP_CART:
		val= sd->status.option&CART_MASK;
		break;
	case SP_STR:
		val= sd->status.str;
		break;
	case SP_AGI:
		val= sd->status.agi;
		break;
	case SP_VIT:
		val= sd->status.vit;
		break;
	case SP_INT:
		val= sd->status.int_;
		break;
	case SP_DEX:
		val= sd->status.dex;
		break;
	case SP_LUK:
		val= sd->status.luk;
		break;
	}

	return val;
}


/*==========================================
 * script�pPC�X�e�[�^�X�ݒ�
 *------------------------------------------
 */
int pc_setparam(struct map_session_data *sd,int type,int val)
{
	int i = 0,up_level = 50;
	struct pc_base_job s_class;

	nullpo_retr(0, sd);

	s_class = pc_calc_base_job(sd->status.class);

	switch(type){
	case SP_BASELEVEL:
		if (val > sd->status.base_level) {
			for (i = 1; i <= (val - sd->status.base_level); i++)
				sd->status.status_point += (sd->status.base_level + i + 14) / 5 ;
		}
		sd->status.base_level = val;
		sd->status.base_exp = 0;
		clif_updatestatus(sd, SP_BASELEVEL);
		clif_updatestatus(sd, SP_NEXTBASEEXP);
		clif_updatestatus(sd, SP_STATUSPOINT);
		clif_updatestatus(sd, SP_BASEEXP);
		status_calc_pc(sd, 0);
		pc_heal(sd, sd->status.max_hp, sd->status.max_sp);
		break;
	case SP_JOBLEVEL:
		if (s_class.job == 0)
			up_level -= 40;
		if ((s_class.job == 23) || (s_class.upper == 1 && s_class.type == 2))
			up_level += 20;
		if (val >= sd->status.job_level) {
			if (val > up_level)val = up_level;
			sd->status.skill_point += (val-sd->status.job_level);
		sd->status.job_level = val;
			sd->status.job_exp = 0;
			clif_updatestatus(sd, SP_JOBLEVEL);
			clif_updatestatus(sd, SP_NEXTJOBEXP);
			clif_updatestatus(sd, SP_JOBEXP);
			clif_updatestatus(sd, SP_SKILLPOINT);
			status_calc_pc(sd, 0);
			clif_misceffect(&sd->bl, 1);
		} else {
			sd->status.job_level = val;
			sd->status.job_exp = 0;
			clif_updatestatus(sd, SP_JOBLEVEL);
			clif_updatestatus(sd, SP_NEXTJOBEXP);
			clif_updatestatus(sd, SP_JOBEXP);
			status_calc_pc(sd, 0);
		}
		clif_updatestatus(sd,type);
		break;
	case SP_SKILLPOINT:
		sd->status.skill_point = val;
		break;
	case SP_STATUSPOINT:
		sd->status.status_point = val;
		break;
	case SP_ZENY:
		if(val <= MAX_ZENY) {
			// MAX_ZENY �ȉ��Ȃ���
			sd->status.zeny = val;
		} else {
			if(sd->status.zeny > val) {
				// Zeny ���������Ă���Ȃ���
				sd->status.zeny = val;
			} else if(sd->status.zeny <= MAX_ZENY) {
				// Zeny ���������Ă��āA���݂̒l��MAX_ZENY �ȉ��Ȃ�MAX_ZENY
				sd->status.zeny = MAX_ZENY;
			} else {
				// Zeny ���������Ă��āA���݂̒l��MAX_ZENY ��艺�Ȃ瑝�����𖳎�
				;
			}
		}
		break;
	case SP_BASEEXP:
		if(pc_nextbaseexp(sd) > 0) {
			sd->status.base_exp = val;
			if(sd->status.base_exp < 0)
				sd->status.base_exp=0;
			pc_checkbaselevelup(sd);
		}
		break;
	case SP_JOBEXP:
		if(pc_nextjobexp(sd) > 0) {
			sd->status.job_exp = val;
			if(sd->status.job_exp < 0)
				sd->status.job_exp=0;
			pc_checkjoblevelup(sd);
		}
		break;
	}
	clif_updatestatus(sd,type);

	return 0;
}


/*==========================================
 * HP/SP��
 *------------------------------------------
 */
int pc_heal(struct map_session_data *sd,int hp,int sp)
{
//	if(battle_config.battle_log)
//		printf("heal %d %d\n",hp,sp);

	nullpo_retr(0, sd);

	if(pc_checkoverhp(sd)) {
		if(hp > 0)
			hp = 0;
	}
	if(pc_checkoversp(sd)) {
		if(sp > 0)
			sp = 0;
	}

	// �o�[�T�[�N���͉񕜂����Ȃ�
	if(sd->sc_data && sd->sc_data[SC_BERSERK].timer!=-1) {
		if (sp > 0)
			sp = 0;
		if (hp > 0)
			hp = 0;
	}

	if(hp+sd->status.hp>sd->status.max_hp)
		hp=sd->status.max_hp-sd->status.hp;
	if(sp+sd->status.sp>sd->status.max_sp)
		sp=sd->status.max_sp-sd->status.sp;
	sd->status.hp+=hp;
	if(sd->status.hp <= 0) {
		sd->status.hp = 0;
		pc_damage(NULL,sd,1);
		hp = 0;
	}
	sd->status.sp+=sp;
	if(sd->status.sp <= 0)
		sd->status.sp = 0;
	if(hp)
		clif_updatestatus(sd,SP_HP);
	if(sp)
		clif_updatestatus(sd,SP_SP);

	return hp + sp;
}


/*==========================================
 * HP/SP��
 *------------------------------------------
 */
int pc_itemheal(struct map_session_data *sd,int hp,int sp)
{
	int bonus;
//	if(battle_config.battle_log)
//		printf("heal %d %d\n",hp,sp);

	nullpo_retr(0, sd);

	if(sd->state.potionpitcher_flag) {
		sd->potion_hp = hp;
		sd->potion_sp = sp;
		return 0;
	}

	if(pc_checkoverhp(sd)) {
		if(hp > 0)
			hp = 0;
	}
	if(pc_checkoversp(sd)) {
		if(sp > 0)
			sp = 0;
	}
	if(hp > 0) {
		bonus = (sd->paramc[2]<<1) + 100 + pc_checkskill(sd,SM_RECOVERY)*10;
		if(bonus != 100)
			hp = hp * bonus / 100;
		bonus = 100 + pc_checkskill(sd,AM_LEARNINGPOTION)*5;
		if(bonus != 100)
			hp = hp * bonus / 100;
		//card
		bonus = 100 + sd->itemheal_rate[itemdb_group(sd->use_itemid)];
		if(bonus != 100)
			hp = hp * bonus / 100;
	}
	if(sp > 0) {
		bonus = (sd->paramc[3]<<1) + 100 + pc_checkskill(sd,MG_SRECOVERY)*10;
		if(bonus != 100)
			sp = sp * bonus / 100;
		bonus = 100 + pc_checkskill(sd,AM_LEARNINGPOTION)*5;
		if(bonus != 100)
			sp = sp * bonus / 100;
		//card
		bonus = 100 + sd->itemheal_rate[itemdb_group(sd->use_itemid)];
		if(bonus != 100)
			sp  = sp * bonus / 100;
	}
	if(hp+sd->status.hp>sd->status.max_hp)
		hp=sd->status.max_hp-sd->status.hp;
	if(sp+sd->status.sp>sd->status.max_sp)
		sp=sd->status.max_sp-sd->status.sp;
	sd->status.hp+=hp;
	if(sd->status.hp <= 0) {
		sd->status.hp = 0;
		pc_damage(NULL,sd,1);
		hp = 0;
	}
	sd->status.sp+=sp;
	if(sd->status.sp <= 0)
		sd->status.sp = 0;
	if(hp)
		clif_updatestatus(sd,SP_HP);
	if(sp)
		clif_updatestatus(sd,SP_SP);

	return 0;
}


/*==========================================
 * HP/SP��
 *------------------------------------------
 */
int pc_percentheal(struct map_session_data *sd,int hp,int sp)
{
	nullpo_retr(0, sd);

	if(sd->state.potionpitcher_flag) {
		sd->potion_per_hp = hp;
		sd->potion_per_sp = sp;
		return 0;
	}

	if(pc_checkoverhp(sd)) {
		if(hp > 0)
			hp = 0;
	}
	if(pc_checkoversp(sd)) {
		if(sp > 0)
			sp = 0;
	}
	if(hp) {
		if(hp >= 100) {
			sd->status.hp = sd->status.max_hp;
		}
		else if(hp <= -100) {
			sd->status.hp = 0;
			pc_damage(NULL,sd,1);
		}
		else {
			sd->status.hp += sd->status.max_hp*hp/100;
			if(sd->status.hp > sd->status.max_hp)
				sd->status.hp = sd->status.max_hp;
			if(sd->status.hp <= 0) {
				sd->status.hp = 0;
				pc_damage(NULL,sd,1);
				hp = 0;
			}
		}
	}
	if(sp) {
		if(sp >= 100) {
			sd->status.sp = sd->status.max_sp;
		}
		else if(sp <= -100) {
			sd->status.sp = 0;
		}
		else {
			sd->status.sp += sd->status.max_sp*sp/100;
			if(sd->status.sp > sd->status.max_sp)
				sd->status.sp = sd->status.max_sp;
			if(sd->status.sp < 0)
				sd->status.sp = 0;
		}
	}
	if(hp)
		clif_updatestatus(sd,SP_HP);
	if(sp)
		clif_updatestatus(sd,SP_SP);

	return 0;
}


/*==========================================
 * �E�ύX
 * ����	job �E�� 0�`23
 *		upper �ʏ� 0, �]�� 1, �{�q 2, ���̂܂� -1
 *------------------------------------------
 */
int pc_jobchange(struct map_session_data *sd,int job, int upper)
{
	int i;
	int b_class = 0;
	int joblv_nochange = 0;
	struct pc_base_job s_class;

	nullpo_retr(0, sd);

	//�]����{�q�̏ꍇ�̌��̐E�Ƃ��Z�o����
	s_class = pc_calc_base_job(sd->status.class);
	
	if(job >= MAX_VALID_PC_CLASS)
		return 1;

	//�{�q<->�]���O�̏ꍇJOB1�ɂ��Ȃ�
	if(s_class.upper!=1 && upper!=1 && s_class.job == job)
		joblv_nochange = 1;
	//
	if(job >= PC_CLASS_TK && job <= PC_CLASS_SL)
		upper = 2;

	if(battle_config.enable_upper_class==0){ //conf�Ŗ����ɂȂ��Ă�����upper=0
		upper = 0;
	}
	
	if(upper < 0) //���ݓ]�����ǂ����𔻒f����
		upper = s_class.upper;

	if(upper == 0){ //�ʏ�E�Ȃ�job���̂܂��
		b_class = job;
	}else if(upper == 1){
		if(job >= PC_CLASS_SNV){ //�]���ɃX�p�m�r�ȍ~�͑��݂��Ȃ��̂ł��f��
			return 1;
		}else{
			b_class = job + PC_CLASS_BASE2;
		}
	}else if(upper == 2){ //�{�q�Ɍ����͂Ȃ����ǂǂ������ŏR���邩�炢����
		b_class = (job>=23)?(job + (PC_CLASS_BASE3 - 1)):(job + PC_CLASS_BASE3);
	}else{
		return 1;
	}

//#ifndef TKSGSL
	if((sd->sex == 0 && job == 19) || (sd->sex == 1 && job == 20) ||
	   job == 13 || job == 21 || job ==22 || job ==26 ||sd->status.class == b_class) //���̓o�[�h�ɂȂ�Ȃ��A���̓_���T�[�ɂȂ�Ȃ��A�����ߏւ����f��
		return 1;
//#else
//���̓o�[�h�ɂȂ�Ȃ��A���̓_���T�[�ɂȂ�Ȃ��A�����ߏւ����f�� �g���P�������f��
//	if((sd->sex == 0 && job == 19) || (sd->sex == 1 && job == 20) ||
//	   job == 13 || job == 21 || job ==22 || sd->status.class == b_class || job > 23 ) 
//		return 1;
//#endif

	sd->status.class = sd->view_class = b_class;
	clif_changelook(&sd->bl,LOOK_BASE,sd->view_class);
	if(sd->status.clothes_color > 0)
		clif_changelook(&sd->bl,LOOK_CLOTHES_COLOR,sd->status.clothes_color);

	if(sd->status.manner < 0)
		clif_changestatus(&sd->bl,SP_MANNER,sd->status.manner);

	if(joblv_nochange==0)
	{
		sd->status.job_level=1;
		sd->status.job_exp=0;
	}
	clif_updatestatus(sd,SP_JOBLEVEL);
	clif_updatestatus(sd,SP_JOBEXP);
	clif_updatestatus(sd,SP_NEXTJOBEXP);

	for(i=0;i<11;i++) {
		if(sd->equip_index[i] >= 0)
			if(!pc_isequip(sd,sd->equip_index[i]))
				pc_unequipitem(sd,sd->equip_index[i],1);	// �����O��
	}

	status_calc_pc(sd,0);
	pc_checkallowskill(sd);
	pc_equiplookall(sd);
	clif_equiplist(sd);

	return 0;
}


/*==========================================
 * �����ڕύX
 *------------------------------------------
 */
int pc_equiplookall(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

#if PACKETVER < 4
	clif_changelook(&sd->bl,LOOK_WEAPON,sd->status.weapon);
	clif_changelook(&sd->bl,LOOK_SHIELD,sd->status.shield);
#else
	clif_changelook(&sd->bl,LOOK_WEAPON,0);
	clif_changelook(&sd->bl,LOOK_SHOES,0);
#endif
	clif_changelook(&sd->bl,LOOK_HEAD_BOTTOM,sd->status.head_bottom);
	clif_changelook(&sd->bl,LOOK_HEAD_TOP,sd->status.head_top);
	clif_changelook(&sd->bl,LOOK_HEAD_MID,sd->status.head_mid);

	return 0;
}


/*==========================================
 * �����ڕύX
 *------------------------------------------
 */
int pc_changelook(struct map_session_data *sd,int type,int val)
{
	nullpo_retr(0, sd);

	switch(type){
	case LOOK_HAIR:
		sd->status.hair=val;
		break;
	case LOOK_WEAPON:
		sd->status.weapon=val;
		break;
	case LOOK_HEAD_BOTTOM:
		sd->status.head_bottom=val;
		break;
	case LOOK_HEAD_TOP:
		sd->status.head_top=val;
		break;
	case LOOK_HEAD_MID:
		sd->status.head_mid=val;
		break;
	case LOOK_HAIR_COLOR:
		sd->status.hair_color=val;
		break;
	case LOOK_CLOTHES_COLOR:
		sd->status.clothes_color=val;
		break;
	case LOOK_SHIELD:
		sd->status.shield=val;
		break;
	case LOOK_SHOES:
		break;
	}
	clif_changelook(&sd->bl,type,val);

	return 0;
}


/*==========================================
 * �t���i(��,�y�R,�J�[�g)�ݒ�
 *------------------------------------------
 */
int pc_setoption(struct map_session_data *sd,int type)
{
	nullpo_retr(0, sd);

	sd->status.option=type;
	clif_changeoption(&sd->bl);
	status_calc_pc(sd,0);

	return 0;
}


/*==========================================
 * �J�[�g�ݒ�
 *------------------------------------------
 */
int pc_setcart(struct map_session_data *sd,int type)
{
	int cart[6]={0x0000,0x0008,0x0080,0x0100,0x0200,0x0400};

	nullpo_retr(0, sd);

	if(pc_checkskill(sd,MC_PUSHCART)>0){ // �v�b�V���J�[�g�X�L������
		if(!pc_iscarton(sd)){ // �J�[�g��t���Ă��Ȃ�
			pc_setoption(sd,cart[type]);
			clif_cart_itemlist(sd);
			clif_cart_equiplist(sd);
			clif_updatestatus(sd,SP_CARTINFO);
			clif_status_change(&sd->bl,0x0c,0);
		}
		else{
			pc_setoption(sd,cart[type]);
		}
	}

	return 0;
}


/*==========================================
 * ��ݒ�
 *------------------------------------------
 */
int pc_setfalcon(struct map_session_data *sd)
{
	if(pc_checkskill(sd,HT_FALCON)>0){	// �t�@���R���}�X�^���[�X�L������
		pc_setoption(sd,0x0010);
	}

	return 0;
}


/*==========================================
 * �y�R�y�R�ݒ�
 *------------------------------------------
 */
int pc_setriding(struct map_session_data *sd)
{
	if(pc_checkskill(sd,KN_RIDING)>0){ // ���C�f�B���O�X�L������
		pc_setoption(sd,0x0020);
	}

	return 0;
}
/*==========================================
 * �A�C�e���h���b�v�s����
 *------------------------------------------
 */
int pc_candrop(struct map_session_data *sd,int item_id)
{
	if(pc_isGM(sd) && pc_isGM(sd) < battle_config.gm_can_drop_lv)
		return 1;
	return 0;
}

/*==========================================
 * script�p�ϐ��̒l��ǂ�
 *------------------------------------------
 */
int pc_readreg(struct map_session_data *sd,int reg)
{
	int i;

	nullpo_retr(0, sd);

	for(i=0;i<sd->reg_num;i++)
		if(sd->reg[i].index==reg)
			return sd->reg[i].data;

	return 0;
}
/*==========================================
 * script�p�ϐ��̒l��ݒ�
 *------------------------------------------
 */
int pc_setreg(struct map_session_data *sd,int reg,int val)
{
	int i;

	nullpo_retr(0, sd);

	for (i = 0; i < sd->reg_num; i++) {
		if (sd->reg[i].index == reg){
			sd->reg[i].data = val;
			return 0;
		}
	}
	sd->reg_num++;
	sd->reg = (struct script_reg *)aRealloc(sd->reg, sizeof(*(sd->reg)) * sd->reg_num);
/*	memset(sd->reg + (sd->reg_num - 1) * sizeof(*(sd->reg)), 0,
		sizeof(*(sd->reg)));
*/
	sd->reg[i].index = reg;
	sd->reg[i].data = val;

	return 0;
}


/*==========================================
 * script�p������ϐ��̒l��ǂ�
 *------------------------------------------
 */
char *pc_readregstr(struct map_session_data *sd,int reg)
{
	int i;

	nullpo_retr(0, sd);

	for(i=0;i<sd->regstr_num;i++)
		if(sd->regstr[i].index==reg)
			return sd->regstr[i].data;

	return NULL;
}
/*==========================================
 * script�p������ϐ��̒l��ݒ�
 *------------------------------------------
 */
int pc_setregstr(struct map_session_data *sd,int reg,char *str)
{
	int i;

	nullpo_retr(0, sd);

	if(strlen(str)+1 >= sizeof(sd->regstr[0].data)){
		printf("pc_setregstr: string too long !\n");
		return 0;
	}

	for(i=0;i<sd->regstr_num;i++)
		if(sd->regstr[i].index==reg){
			strncpy(sd->regstr[i].data,str,256);
			return 0;
		}
	sd->regstr_num++;
	sd->regstr = (struct script_regstr *)aRealloc(sd->regstr, sizeof(sd->regstr[0]) * sd->regstr_num);
/*	memset(sd->reg + (sd->reg_num - 1) * sizeof(*(sd->reg)), 0,
		sizeof(*(sd->reg)));
*/
	sd->regstr[i].index=reg;
	strncpy(sd->regstr[i].data,str,256);

	return 0;
}

/*==========================================
 * script�p�O���[�o���ϐ��̒l��ǂ�
 *------------------------------------------
 */
int pc_readglobalreg(struct map_session_data *sd,char *reg)
{
	int i;

	nullpo_retr(0, sd);

	for(i=0;i<sd->status.global_reg_num;i++){
		if(strcmp(sd->status.global_reg[i].str,reg)==0)
			return sd->status.global_reg[i].value;
	}

	return 0;
}


/*==========================================
 * script�p�O���[�o���ϐ��̒l��ݒ�
 *------------------------------------------
 */
int pc_setglobalreg(struct map_session_data *sd,char *reg,int val)
{
	int i;

	nullpo_retr(0, sd);

	//PC_DIE_COUNTER���X�N���v�g�ȂǂŕύX���ꂽ���̏���
	if(strcmp(reg,"PC_DIE_COUNTER") == 0 && sd->die_counter != val){
		sd->die_counter = val;
		status_calc_pc(sd,0);
	}
	if(val==0){
		for(i=0;i<sd->status.global_reg_num;i++){
			if(strcmp(sd->status.global_reg[i].str,reg)==0){
				sd->status.global_reg[i]=sd->status.global_reg[sd->status.global_reg_num-1];
				sd->status.global_reg_num--;
				break;
			}
		}
		return 0;
	}
	for(i=0;i<sd->status.global_reg_num;i++){
		if(strcmp(sd->status.global_reg[i].str,reg)==0){
			sd->status.global_reg[i].value=val;
			return 0;
		}
	}
	if(sd->status.global_reg_num<GLOBAL_REG_NUM){
		strncpy(sd->status.global_reg[i].str,reg,32);
		sd->status.global_reg[i].value=val;
		sd->status.global_reg_num++;
		return 0;
	}
	if(battle_config.error_log)
		printf("pc_setglobalreg : couldn't set %s (GLOBAL_REG_NUM = %d)\n", reg, GLOBAL_REG_NUM);

	return 1;
}

/*==========================================
 * script�p�A�J�E���g�ϐ��̒l��ǂ�
 *------------------------------------------
 */
int pc_readaccountreg(struct map_session_data *sd,char *reg)
{
	int i;

	nullpo_retr(0, sd);

	for(i=0;i<sd->status.account_reg_num;i++){
		if(strcmp(sd->status.account_reg[i].str,reg)==0)
			return sd->status.account_reg[i].value;
	}

	return 0;
}
/*==========================================
 * script�p�A�J�E���g�ϐ��̒l��ݒ�
 *------------------------------------------
 */
int pc_setaccountreg(struct map_session_data *sd,char *reg,int val)
{
	int i;

	nullpo_retr(0, sd);

	if(val==0){
		for(i=0;i<sd->status.account_reg_num;i++){
			if(strcmp(sd->status.account_reg[i].str,reg)==0){
				sd->status.account_reg[i]=sd->status.account_reg[sd->status.account_reg_num-1];
				sd->status.account_reg_num--;
				break;
			}
		}
		intif_saveaccountreg(sd);
		return 0;
	}
	for(i=0;i<sd->status.account_reg_num;i++){
		if(strcmp(sd->status.account_reg[i].str,reg)==0){
			sd->status.account_reg[i].value=val;
			intif_saveaccountreg(sd);
			return 0;
		}
	}
	if(sd->status.account_reg_num<ACCOUNT_REG_NUM){
		strncpy(sd->status.account_reg[i].str,reg,32);
		sd->status.account_reg[i].value=val;
		sd->status.account_reg_num++;
		intif_saveaccountreg(sd);
		return 0;
	}
	if(battle_config.error_log)
		printf("pc_setaccountreg : couldn't set %s (ACCOUNT_REG_NUM = %d)\n", reg, ACCOUNT_REG_NUM);

	return 1;
}
/*==========================================
 * script�p�A�J�E���g�ϐ�2�̒l��ǂ�
 *------------------------------------------
 */
int pc_readaccountreg2(struct map_session_data *sd,char *reg)
{
	int i;

	nullpo_retr(0, sd);

	for(i=0;i<sd->status.account_reg2_num;i++){
		if(strcmp(sd->status.account_reg2[i].str,reg)==0)
			return sd->status.account_reg2[i].value;
	}

	return 0;
}
/*==========================================
 * script�p�A�J�E���g�ϐ�2�̒l��ݒ�
 *------------------------------------------
 */
int pc_setaccountreg2(struct map_session_data *sd,char *reg,int val)
{
	int i;

	nullpo_retr(1, sd);

	if(val==0){
		for(i=0;i<sd->status.account_reg2_num;i++){
			if(strcmp(sd->status.account_reg2[i].str,reg)==0){
				sd->status.account_reg2[i]=sd->status.account_reg2[sd->status.account_reg2_num-1];
				sd->status.account_reg2_num--;
				break;
			}
		}
		chrif_saveaccountreg2(sd);
		return 0;
	}
	for(i=0;i<sd->status.account_reg2_num;i++){
		if(strcmp(sd->status.account_reg2[i].str,reg)==0){
			sd->status.account_reg2[i].value=val;
			chrif_saveaccountreg2(sd);
			return 0;
		}
	}
	if(sd->status.account_reg2_num<ACCOUNT_REG2_NUM){
		strncpy(sd->status.account_reg2[i].str,reg,32);
		sd->status.account_reg2[i].value=val;
		sd->status.account_reg2_num++;
		chrif_saveaccountreg2(sd);
		return 0;
	}
	if(battle_config.error_log)
		printf("pc_setaccountreg2 : couldn't set %s (ACCOUNT_REG2_NUM = %d)\n", reg, ACCOUNT_REG2_NUM);

	return 1;
}

/*==========================================
 * �C�x���g�^�C�}�[����
 *------------------------------------------
 */
int pc_eventtimer(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd=map_id2sd(id);
	char *p = (char *)data;
	int i;
	if(sd==NULL)
		return 0;

	for(i=0;i<MAX_EVENTTIMER;i++){
		if( sd->eventtimer[i]==tid ){
			sd->eventtimer[i]=-1;
			npc_event(sd,p);
			break;
		}
	}
	free(p);
	if(i==MAX_EVENTTIMER) {
		if(battle_config.error_log)
			printf("pc_eventtimer: no such event timer\n");
	}

	return 0;
}


/*==========================================
 * �C�x���g�^�C�}�[�ǉ�
 *------------------------------------------
 */
int pc_addeventtimer(struct map_session_data *sd,int tick,const char *name)
{
	int i;

	nullpo_retr(0, sd);

	for(i=0;i<MAX_EVENTTIMER;i++)
		if( sd->eventtimer[i]==-1 )
			break;
	if(i<MAX_EVENTTIMER){
		char *evname=strdup(name);
		sd->eventtimer[i]=add_timer(gettick()+tick,
			pc_eventtimer,sd->bl.id,(int)evname);
	}else {
		if(battle_config.error_log)
			printf("pc_addtimer: event timer is full !\n");
	}

	return 0;
}


/*==========================================
 * �C�x���g�^�C�}�[�폜
 *------------------------------------------
 */
int pc_deleventtimer(struct map_session_data *sd,const char *name)
{
	int i;

	nullpo_retr(0, sd);

	for(i=0;i<MAX_EVENTTIMER;i++)
		if( sd->eventtimer[i]!=-1 ) {
			char *p = (char *)(get_timer(sd->eventtimer[i])->data);
			if(strcmp(p, name)==0) {
				delete_timer(sd->eventtimer[i],pc_eventtimer);
				sd->eventtimer[i]=-1;
				free(p);
				break;
			}
		}

	return 0;
}


/*==========================================
 * �C�x���g�^�C�}�[�J�E���g�l�ǉ�
 *------------------------------------------
 */
int pc_addeventtimercount(struct map_session_data *sd,const char *name,int tick)
{
	int i;

	nullpo_retr(0, sd);

	for(i=0;i<MAX_EVENTTIMER;i++)
		if( sd->eventtimer[i]!=-1 && strcmp(
			(char *)(get_timer(sd->eventtimer[i])->data), name)==0 ){
				addtick_timer(sd->eventtimer[i],tick);
				break;
		}

	return 0;
}


/*==========================================
 * �C�x���g�^�C�}�[�S�폜
 *------------------------------------------
 */
int pc_cleareventtimer(struct map_session_data *sd)
{
	int i;

	nullpo_retr(0, sd);

	for(i=0;i<MAX_EVENTTIMER;i++)
		if( sd->eventtimer[i]!=-1 ){
			char *p = (char *)(get_timer(sd->eventtimer[i])->data);
			delete_timer(sd->eventtimer[i],pc_eventtimer);
			free(p);
			sd->eventtimer[i]=-1;
		}

	return 0;
}

//
// �� ����
//
/*==========================================
 * �A�C�e���𑕔�����
 *------------------------------------------
 */
int pc_equipitem(struct map_session_data *sd,int n,int pos)
{
	int i,nameid;
	struct item_data *id;
	//�]����{�q�̏ꍇ�̌��̐E�Ƃ��Z�o����
	struct pc_base_job s_class;

	nullpo_retr(0, sd);

	s_class = pc_calc_base_job(sd->status.class);

	nameid = sd->status.inventory[n].nameid;
	id = sd->inventory_data[n];
	pos = pc_equippoint(sd,n);
	if(battle_config.battle_log)
		printf("equip %d(%d) %x:%x\n",nameid,n,id->equip,pos);
	if(!pc_isequip(sd,n) || !pos) {
		clif_equipitemack(sd,n,0,0);	// fail
		return 0;
	}
	if(pos==0x88){ // �A�N�Z�T���p��O����
		int epor=0;
		if(sd->equip_index[0] >= 0)
			epor |= sd->status.inventory[sd->equip_index[0]].equip;
		if(sd->equip_index[1] >= 0)
			epor |= sd->status.inventory[sd->equip_index[1]].equip;
		epor &= 0x88;
		pos = epor == 0x08 ? 0x80 : 0x08;
	}

	// �񓁗�����
	if ((pos==0x22) // �ꉞ�A�����v���ӏ����񓁗����킩�`�F�b�N����
	 &&	(id->equip==2)	// �P �蕐��
	 &&	(pc_checkskill(sd, AS_LEFT) > 0 || s_class.job == 12) ) // ����C�B�L
	{
		int tpos=0;
		if(sd->equip_index[8] >= 0)
			tpos |= sd->status.inventory[sd->equip_index[8]].equip;
		if(sd->equip_index[9] >= 0)
			tpos |= sd->status.inventory[sd->equip_index[9]].equip;
		tpos &= 0x02;
		pos = tpos == 0x02 ? 0x20 : 0x02;
	}

	for(i=0;i<11;i++) {
		if(sd->equip_index[i] >= 0 && sd->status.inventory[sd->equip_index[i]].equip&pos) {
			pc_unequipitem(sd,sd->equip_index[i],1);
		}
	}
	// �|���
	if(pos==0x8000){
		clif_arrowequip(sd,n);
		clif_arrow_fail(sd,3);	// 3=������ł��܂���
	}
	else
		clif_equipitemack(sd,n,pos,1);

	for(i=0;i<11;i++) {
		if(pos & equip_pos[i])
			sd->equip_index[i] = n;
	}
	sd->status.inventory[n].equip=pos;

	if(sd->status.inventory[n].equip & 0x0002) {
		if(sd->inventory_data[n])
			sd->weapontype1 = sd->inventory_data[n]->look;
		else
			sd->weapontype1 = 0;
		pc_calcweapontype(sd);
		clif_changelook(&sd->bl,LOOK_WEAPON,sd->status.weapon);
	}
	if(sd->status.inventory[n].equip & 0x0020) {
		if(sd->inventory_data[n]) {
			if(sd->inventory_data[n]->type == 4) {
				sd->status.shield = 0;
				if(sd->status.inventory[n].equip == 0x0020)
					sd->weapontype2 = sd->inventory_data[n]->look;
				else
					sd->weapontype2 = 0;
			}
			else if(sd->inventory_data[n]->type == 5) {
				sd->status.shield = sd->inventory_data[n]->look;
				sd->weapontype2 = 0;
			}
		}
		else
			sd->status.shield = sd->weapontype2 = 0;
		pc_calcweapontype(sd);
		clif_changelook(&sd->bl,LOOK_SHIELD,sd->status.shield);
	}
	if(sd->status.inventory[n].equip & 0x0001) {
		if(sd->inventory_data[n])
			sd->status.head_bottom = sd->inventory_data[n]->look;
		else
			sd->status.head_bottom = 0;
		clif_changelook(&sd->bl,LOOK_HEAD_BOTTOM,sd->status.head_bottom);
	}
	if(sd->status.inventory[n].equip & 0x0100) {
		if(sd->inventory_data[n])
			sd->status.head_top = sd->inventory_data[n]->look;
		else
			sd->status.head_top = 0;
		clif_changelook(&sd->bl,LOOK_HEAD_TOP,sd->status.head_top);
	}
	if(sd->status.inventory[n].equip & 0x0200) {
		if(sd->inventory_data[n])
			sd->status.head_mid = sd->inventory_data[n]->look;
		else
			sd->status.head_mid = 0;
		clif_changelook(&sd->bl,LOOK_HEAD_MID,sd->status.head_mid);
	}
	if(sd->status.inventory[n].equip & 0x0040)
		clif_changelook(&sd->bl,LOOK_SHOES,0);

	pc_checkallowskill(sd);	// �����i�ŃX�L������������邩�`�F�b�N
	status_calc_pc(sd,0);

	if(sd->sc_data[SC_SIGNUMCRUCIS].timer != -1 && !battle_check_undead(7,sd->def_ele))
		status_change_end(&sd->bl,SC_SIGNUMCRUCIS,-1);
	if(sd->sc_data[SC_DANCING].timer!=-1 && (sd->status.weapon != 13 && sd->status.weapon !=14))
		skill_stop_dancing(&sd->bl,0);

	return 0;
}

/*==========================================
 * �� �����������O��
 *------------------------------------------
 */
int pc_unequipitem(struct map_session_data *sd,int n,int type)
{
	int hp=0,sp=0;
	nullpo_retr(0, sd);

	if(battle_config.battle_log)
		printf("unequip %d %x:%x\n",n,pc_equippoint(sd,n),sd->status.inventory[n].equip);
	if(sd->status.inventory[n].equip){
		int i;
		for(i=0;i<11;i++) {
			if(sd->status.inventory[n].equip & equip_pos[i])
			{
				sd->equip_index[i] = -1;
				
				//�������O����HP/SP�̃y�i���e�B����
				if(sd->hp_penalty_unrig_value[i] > 0) {
					hp += sd->hp_penalty_unrig_value[i];
					sd->hp_penalty_unrig_value[i] = 0;
				}
				if(sd->sp_penalty_unrig_value[i] > 0) {
					sp += sd->sp_penalty_unrig_value[i];
					sd->sp_penalty_unrig_value[i] = 0;
				}
				if(sd->hp_rate_penalty_unrig[i] > 0) {
					hp += sd->status.max_hp*sd->hp_rate_penalty_unrig[i];
					sd->hp_rate_penalty_unrig[i] = 0;
				}
				if(sd->sp_rate_penalty_unrig[i] > 0) {
					sp += sd->status.max_sp*sd->sp_rate_penalty_unrig[i];
					sd->sp_rate_penalty_unrig[i] = 0;
				}
				if(!battle_config.death_by_unrig_penalty)
				{
					if(sd->status.hp < hp)
					{
						hp = sd->status.hp-1;
					}
				}
				/*
				if(sd->status.sp < sp)
				{
					sp = sd->status.sp;	
				}
				*/
			}
		}
		if(sd->status.inventory[n].equip & 0x0002) {
			sd->weapontype1 = 0;
			sd->status.weapon = sd->weapontype2;
			pc_calcweapontype(sd);
			clif_changelook(&sd->bl,LOOK_WEAPON,sd->status.weapon);
		}
		if(sd->status.inventory[n].equip & 0x0020) {
			sd->status.shield = sd->weapontype2 = 0;
			pc_calcweapontype(sd);
			clif_changelook(&sd->bl,LOOK_SHIELD,sd->status.shield);
		}
		if(sd->status.inventory[n].equip & 0x0001) {
			sd->status.head_bottom = 0;
			clif_changelook(&sd->bl,LOOK_HEAD_BOTTOM,sd->status.head_bottom);
		}
		if(sd->status.inventory[n].equip & 0x0100) {
			sd->status.head_top = 0;
			clif_changelook(&sd->bl,LOOK_HEAD_TOP,sd->status.head_top);
		}
		if(sd->status.inventory[n].equip & 0x0200) {
			sd->status.head_mid = 0;
			clif_changelook(&sd->bl,LOOK_HEAD_MID,sd->status.head_mid);
		}
		if(sd->status.inventory[n].equip & 0x0040)
			clif_changelook(&sd->bl,LOOK_SHOES,0);

		clif_unequipitemack(sd,n,sd->status.inventory[n].equip,1);
		sd->status.inventory[n].equip=0;
		if(!type)
			pc_checkallowskill(sd);
		if(sd->weapontype1 == 0 && sd->weapontype2 == 0)
			skill_encchant_eremental_end(&sd->bl,-1);  //���펝�������͖������ő����t�^����
	} else {
		clif_unequipitemack(sd,n,0,0);
	}
	if(!type) {
		status_calc_pc(sd,0);
		if(sd->sc_data[SC_SIGNUMCRUCIS].timer != -1 && !battle_check_undead(7,sd->def_ele))
			status_change_end(&sd->bl,SC_SIGNUMCRUCIS,-1);
	}

	if(hp || sp)
	{
		pc_heal(sd,-hp,-sp);
	}

	return 0;
}

/*==========================================
 * �������Ă��邩�`�F�b�N���s�Ȃ�
 * �߂�l
 *	1 : �������Ă���
 *	0 : �������Ă��Ȃ�
 *------------------------------------------
 */
int pc_equippeditem(struct map_session_data *sd,int id)
{
	int i,j,index;
	nullpo_retr(0, sd);
	
	for(i=0;i<10;i++) {
		index = sd->equip_index[i];
		if(index < 0)
			continue;
		if(i == 9 && sd->equip_index[8] == index)
			continue;
		if(i == 5 && sd->equip_index[4] == index)
			continue;
		if(i == 6 && (sd->equip_index[5] == index || sd->equip_index[4] == index))
			continue;
		
		if(sd->inventory_data[index]) {
			if(sd->status.inventory[index].nameid == id)
			 	return 1;
			
			for(j=0;j<sd->inventory_data[index]->slot;j++){	// �J�[�h
				if(sd->status.inventory[index].card[j] == id) return 1;
			}
		}
	}
		
	return 0;
}

/*==========================================
 * �A�C�e����index�ԍ����l�߂���
 * �� ���i�̑����\�`�F�b�N���s�Ȃ�
 *------------------------------------------
 */
int pc_checkitem(struct map_session_data *sd)
{
	int i,j,k,id,calc_flag = 0;
	struct item_data *it=NULL;

	nullpo_retr(0, sd);

	// �����i�󂫋l��
	for(i=j=0;i<MAX_INVENTORY;i++){
		if( (id=sd->status.inventory[i].nameid)==0)
			continue;
		if( battle_config.item_check && !itemdb_available(id) ){
			if(battle_config.error_log)
				printf("illeagal item id %d in %d[%s] inventory.\n",id,sd->bl.id,sd->status.name);
			pc_delitem(sd,i,sd->status.inventory[i].amount,3);
			continue;
		}
		if(i>j){
			memcpy(&sd->status.inventory[j],&sd->status.inventory[i],sizeof(struct item));
			sd->inventory_data[j] = sd->inventory_data[i];
		}
		j++;
	}
	if(j < MAX_INVENTORY)
		memset(&sd->status.inventory[j],0,sizeof(struct item)*(MAX_INVENTORY-j));
	for(k=j;k<MAX_INVENTORY;k++)
		sd->inventory_data[k] = NULL;

	// �J�[�g���󂫋l��
	for(i=j=0;i<MAX_CART;i++){
		if( (id=sd->status.cart[i].nameid)==0 )
			continue;
		if( battle_config.item_check &&  !itemdb_available(id) ){
			if(battle_config.error_log)
				printf("illeagal item id %d in %d[%s] cart.\n",id,sd->bl.id,sd->status.name);
			pc_cart_delitem(sd,i,sd->status.cart[i].amount,1);
			continue;
		}
		if(i>j){
			memcpy(&sd->status.cart[j],&sd->status.cart[i],sizeof(struct item));
		}
		j++;
	}
	if(j < MAX_CART)
		memset(&sd->status.cart[j],0,sizeof(struct item)*(MAX_CART-j));

	// �� ���ʒu�`�F�b�N

	for(i=0;i<MAX_INVENTORY;i++){

		it=sd->inventory_data[i];

		if(sd->status.inventory[i].nameid==0)
			continue;
		if(sd->status.inventory[i].equip & ~pc_equippoint(sd,i)) {
			sd->status.inventory[i].equip=0;
			calc_flag = 1;
		}
		//���������`�F�b�N
		if(sd->status.inventory[i].equip && map[sd->bl.m].flag.pvp && (it->flag.no_equip==1 || it->flag.no_equip==3)){//PvP����
			sd->status.inventory[i].equip=0;
			calc_flag = 1;
		}else if(sd->status.inventory[i].equip && map[sd->bl.m].flag.gvg && (it->flag.no_equip==2 || it->flag.no_equip==3)){//GvG����
			sd->status.inventory[i].equip=0;
			calc_flag = 1;
		}
	}

	pc_setequipindex(sd);
	if(calc_flag)
		status_calc_pc(sd,2);

	return 0;
}


int pc_checkoverhp(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	if(sd->status.hp == sd->status.max_hp)
		return 1;
	if(sd->status.hp > sd->status.max_hp) {
		sd->status.hp = sd->status.max_hp;
		clif_updatestatus(sd,SP_HP);
		return 2;
	}

	return 0;
}

int pc_checkoversp(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	if(sd->status.sp == sd->status.max_sp)
		return 1;
	if(sd->status.sp > sd->status.max_sp) {
		sd->status.sp = sd->status.max_sp;
		clif_updatestatus(sd,SP_SP);
		return 2;
	}

	return 0;
}


/*==========================================
 * PVP���ʌv�Z�p(foreachinarea)
 *------------------------------------------
 */
int pc_calc_pvprank_sub(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd1,*sd2=NULL;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd1=(struct map_session_data *)bl);
	nullpo_retr(0, sd2=va_arg(ap,struct map_session_data *));

	if( sd1->pvp_point > sd2->pvp_point )
		sd2->pvp_rank++;
	return 0;
}
/*==========================================
 * PVP���ʌv�Z
 *------------------------------------------
 */
int pc_calc_pvprank(struct map_session_data *sd)
{
	int old;
	struct map_data *m;

	nullpo_retr(0, sd);
	nullpo_retr(0, m=&map[sd->bl.m]);
	
	old=sd->pvp_rank;

	if( !(m->flag.pvp) )
		return 0;
	sd->pvp_rank=1;
	map_foreachinarea(pc_calc_pvprank_sub,sd->bl.m,0,0,m->xs,m->ys,BL_PC,sd);
	if(old!=sd->pvp_rank || sd->pvp_lastusers!=m->users)
		clif_pvpset(sd,sd->pvp_rank,sd->pvp_lastusers=m->users,0);
	return sd->pvp_rank;
}
/*==========================================
 * PVP���ʌv�Z(timer)
 *------------------------------------------
 */
int pc_calc_pvprank_timer(int tid,unsigned int tick,int id,int data)
{
	struct map_session_data *sd=map_id2sd(id);
	if(sd==NULL)
		return 0;
	sd->pvp_timer=-1;
	if( pc_calc_pvprank(sd)>0 )
		sd->pvp_timer=add_timer(
			gettick()+PVP_CALCRANK_INTERVAL,
			pc_calc_pvprank_timer,id,data);
	return 0;
}

/*==========================================
 * sd�͌������Ă��邩(�����̏ꍇ�͑�����char_id��Ԃ�)
 *------------------------------------------
 */
int pc_ismarried(struct map_session_data *sd)
{
	if(sd == NULL)
		return -1;
	if(sd->status.partner_id > 0)
		return sd->status.partner_id;
	else
		return 0;
}
/*==========================================
 * sd��dstsd�ƌ���(dstsd��sd�̌��������������ɍs��)
 *------------------------------------------
 */
int pc_marriage(struct map_session_data *sd,struct map_session_data *dstsd)
{
	if(sd == NULL || dstsd == NULL || sd->status.partner_id > 0 || dstsd->status.partner_id > 0)
		return -1;
	sd->status.partner_id=dstsd->status.char_id;
	dstsd->status.partner_id=sd->status.char_id;
	return 0;
}


/*==========================================
 * sd������(�����sd->status.partner_id�Ɉ˂�)(����������ɗ����E�����w�֎������D)
 *------------------------------------------
 */
int pc_divorce(struct map_session_data *sd)
{
	struct map_session_data *p_sd=NULL;
	int i;

	if(sd == NULL || !pc_ismarried(sd))
		return -1;

	// �����̗�������
	if( (p_sd=pc_get_partner(sd)) != NULL ){
		if(p_sd->status.partner_id != sd->status.char_id || sd->status.partner_id != p_sd->status.char_id){
			printf("pc_divorce: Illegal partner_id sd=%d p_sd=%d\n",sd->status.partner_id,p_sd->status.partner_id);
			return -1;
		}
		p_sd->status.partner_id=0;
		for(i=0;i<MAX_INVENTORY;i++)
			if(p_sd->status.inventory[i].nameid == WEDDING_RING_M || p_sd->status.inventory[i].nameid == WEDDING_RING_F)
				pc_delitem(p_sd,i,1,0);

	}else{
		// ������������Ȃ��ꍇ��char�I�ɏ������˗�����
		chrif_reqdivorce(sd->status.partner_id);
		chrif_searchcharid(sd->status.partner_id);	// ���O�f�[�^�Ăяo��
	}

	// ����
	sd->status.partner_id=0;
	for(i=0;i<MAX_INVENTORY;i++)
		if(sd->status.inventory[i].nameid == WEDDING_RING_M || sd->status.inventory[i].nameid == WEDDING_RING_F)
			pc_delitem(sd,i,1,0);

	return 0;
}

/*==========================================
 * sd�̑�����map_session_data��Ԃ�
 *------------------------------------------
 */
struct map_session_data *pc_get_partner(struct map_session_data *sd)
{
	struct map_session_data *p_sd = NULL;
	char *nick;
	if(sd == NULL || !pc_ismarried(sd))
		return NULL;

	nick=map_charid2nick(sd->status.partner_id);

	if (nick==NULL)
		return NULL;

	if((p_sd=map_nick2sd(nick)) == NULL )
		return NULL;

	return p_sd;
}

//�����j��
int pc_break_equip(struct map_session_data *sd, unsigned short where)
{
	int i;
	int sc;

	if(sd == NULL)
		return -1;
	if(sd->unbreakable_equip & where)
		return 0;
	switch (where) {
		case EQP_WEAPON:
			sc = SC_CP_WEAPON;
			break;
		case EQP_ARMOR:
			sc = SC_CP_ARMOR;
			break;
		case EQP_SHIELD:
			sc = SC_CP_SHIELD;
			if(sd->equip_index[8]>=0 &&
				sd->inventory_data[sd->equip_index[8]]->type==4)//���肪����Ȃ�
				return 0;
			break;
		case EQP_HELM:
			sc = SC_CP_HELM;
			break;
		default:
			return 0;
	}
	
	if( sd->sc_data && sd->sc_data[sc].timer != -1 )
		return 0;

	for (i=0;i<MAX_INVENTORY;i++) {
		if (sd->status.inventory[i].equip & where) {
			sd->status.inventory[i].attribute = 1;
			pc_unequipitem(sd,i,0);
			break;
		}
	}
	clif_itemlist(sd);
	clif_equiplist(sd);
	return 0;
}

//�����j�� ����
//where
//0:�A�N�ZL(L_ACCE) 1:�A�N�ZR(R_ACCE) 2:�C(SHOES) 3:���(ROBE) 4:��3(HEAD)
//5:��2(HEAD) 6:��1(HEAD) 7:�Z(BODY) 8:����(L_HAND) 9:�E��(R_HAND) 10:��(ARROW)
int pc_break_equip2(struct map_session_data *sd, unsigned short where)
{
	if(sd == NULL)
		return -1;
	if(sd->bl.type != BL_PC)
		return -1;

	switch(where)
	{
		/*
		case 0: //L_ACCE
		case 1:	//R_ACCE
			break;
		case 2: //SHOES
			break;
		case 3:	//ROBE
			break;
		*/
		case 4:	//HEAD
		case 5:	//HEAD
		case 6:	//HEAD
			if(sd->unbreakable_equip&EQP_HELM)
			 	return 0;
			if( sd->sc_data && sd->sc_data[SC_CP_HELM].timer != -1)
				return 0;
			break;
		case 7:	 //BODY
			if(sd->unbreakable_equip&EQP_ARMOR)
			 	return 0;
			if( sd->sc_data && sd->sc_data[SC_CP_ARMOR].timer != -1 )
				return 0;
			break;
		case 8:	//L_HAND
			if(sd->equip_index[8]>=0 && sd->inventory_data[sd->equip_index[8]]->type==4){//����
				if(sd->unbreakable_equip&EQP_WEAPON)
			 		return 0;
				if( sd->sc_data && sd->sc_data[SC_CP_WEAPON].timer != -1 )
					return 0;
			}else{//��
				if(sd->unbreakable_equip&EQP_SHIELD)
			 		return 0;
				if( sd->sc_data && sd->sc_data[SC_CP_SHIELD].timer != -1 )
					return 0;
			}
			break;
		case 9:	//R_HAND
			if(sd->unbreakable_equip&EQP_WEAPON)
			 	return 0;
			if( sd->sc_data && sd->sc_data[SC_CP_WEAPON].timer != -1 )
				return 0;
			break;
		/*
		case 10: //ARROW
				return 0;
			break;
		*/
		default: 
			break;
	}
	if(sd->equip_index[where]>=0 && sd->status.inventory[sd->equip_index[where]].equip){
		sd->status.inventory[sd->equip_index[where]].attribute = 1;
		pc_unequipitem(sd,sd->equip_index[where],0);
	}
	
	clif_itemlist(sd);
	clif_equiplist(sd);
	return 0;
}

//
// ���R�񕜕�
//
/*==========================================
 * SP�񕜗ʌv�Z
 *------------------------------------------
 */
static int natural_heal_tick,natural_heal_prev_tick,natural_heal_diff_tick;
static int pc_spheal(struct map_session_data *sd)
{
	int a;

	nullpo_retr(0, sd);
	
	//���R�񕜒�~
	if(sd->sp_recov_stop) return 0;
	
	a = natural_heal_diff_tick;
	if(pc_issit(sd)) a += a;
	if( sd->sc_data[SC_MAGNIFICAT].timer!=-1 )	// �}�O�j�t�B�J�[�g
		a += a;
	if(sd->sc_data[SC_REGENERATION].timer!=-1)
	{
		switch(sd->sc_data[SC_REGENERATION].val1)
		{
			case 2:
				a += a;
				break;
			case 3:
				a = 3*a;
				break;	
		}
	}

	return a;
}

/*==========================================
 * HP�񕜗ʌv�Z
 *------------------------------------------
 */
static int pc_hpheal(struct map_session_data *sd)
{
	int a;

	nullpo_retr(0, sd);
	
	//���R�񕜒�~
	if(sd->hp_recov_stop) return 0;

	a = natural_heal_diff_tick;
	if(pc_issit(sd)) a += a;

	if(sd->sc_data[SC_REGENERATION].timer!=-1)
	{
		switch(sd->sc_data[SC_REGENERATION].val1)
		{
			case 1:
				a+=a;
				break;
			case 2:
				a += a;
				break;
			case 3:
				a = 3*a;
				break;	
		}
	}

	return a;
}

static int pc_natural_heal_hp(struct map_session_data *sd)
{
	int bhp;
	int inc_num,bonus,skill,hp_flag;

	nullpo_retr(0, sd);

	if(pc_checkoverhp(sd)) {
		sd->hp_sub = sd->inchealhptick = 0;
		return 0;
	}

	bhp=sd->status.hp;
	hp_flag = (pc_checkskill(sd,SM_MOVINGRECOVERY) > 0 && sd->walktimer != -1);

	if(sd->walktimer == -1) {
		inc_num = pc_hpheal(sd);
		if( sd->sc_data[SC_TENSIONRELAX].timer!=-1 ){	// �e���V���������b�N�X
			sd->hp_sub += 2*inc_num;
			sd->inchealhptick += 3*natural_heal_diff_tick;
		}else{
			sd->hp_sub += inc_num;
			sd->inchealhptick += natural_heal_diff_tick;
		}
	}
	else if(hp_flag) {
		inc_num = pc_hpheal(sd);
		sd->hp_sub += inc_num;
		sd->inchealhptick = 0;
	}
	else {
		sd->hp_sub = sd->inchealhptick = 0;
		return 0;
	}

	if(sd->hp_sub >= battle_config.natural_healhp_interval) {
		bonus = sd->nhealhp;
		if(hp_flag) {
			bonus >>= 2;
			if(bonus <= 0) bonus = 1;
		}
		while(sd->hp_sub >= battle_config.natural_healhp_interval) {
			sd->hp_sub -= battle_config.natural_healhp_interval;
			if(sd->status.hp + bonus <= sd->status.max_hp)
				sd->status.hp += bonus;
			else {
				sd->status.hp = sd->status.max_hp;
				sd->hp_sub = sd->inchealhptick = 0;
			}
		}
	}
	if(bhp!=sd->status.hp)
		clif_updatestatus(sd,SP_HP);

	if(sd->nshealhp > 0) {
		if(sd->inchealhptick >= battle_config.natural_heal_skill_interval && sd->status.hp < sd->status.max_hp) {
			bonus = sd->nshealhp;
			while(sd->inchealhptick >= battle_config.natural_heal_skill_interval) {
				sd->inchealhptick -= battle_config.natural_heal_skill_interval;
				if(sd->status.hp + bonus <= sd->status.max_hp)
					sd->status.hp += bonus;
				else {
					bonus = sd->status.max_hp - sd->status.hp;
					sd->status.hp = sd->status.max_hp;
					sd->hp_sub = sd->inchealhptick = 0;
				}
				clif_heal(sd->fd,SP_HP,bonus);
			}
		}
	}
	else sd->inchealhptick = 0;

	return 0;

	if(sd->sc_data[SC_APPLEIDUN].timer!=-1) { // Apple of Idun
		if(sd->inchealhptick >= 6000 && sd->status.hp < sd->status.max_hp) {
			bonus = skill*20;
			while(sd->inchealhptick >= 6000) {
				sd->inchealhptick -= 6000;
				if(sd->status.hp + bonus <= sd->status.max_hp)
					sd->status.hp += bonus;
				else {
					bonus = sd->status.max_hp - sd->status.hp;
					sd->status.hp = sd->status.max_hp;
					sd->hp_sub = sd->inchealhptick = 0;
				}
				clif_heal(sd->fd,SP_HP,bonus);
			}
		}
	}
	else sd->inchealhptick = 0;

	return 0;
}

static int pc_natural_heal_sp(struct map_session_data *sd)
{
	int bsp;
	int inc_num,bonus;
	struct pc_base_job s_class;

	nullpo_retr(0, sd);

	if(pc_checkoversp(sd)) {
		sd->sp_sub = sd->inchealsptick = 0;
		return 0;
	}

	bsp=sd->status.sp;

	s_class = pc_calc_base_job(sd->status.class);
	inc_num = pc_spheal(sd);
	if(s_class.job == 23 || sd->sc_data[SC_EXPLOSIONSPIRITS].timer == -1 || sd->sc_data[SC_MONK].timer!=-1)
		sd->sp_sub += inc_num;
	if(sd->walktimer == -1)
		sd->inchealsptick += natural_heal_diff_tick;
	else sd->inchealsptick = 0;

	if(sd->sp_sub >= battle_config.natural_healsp_interval){
		bonus = sd->nhealsp;;
		while(sd->sp_sub >= battle_config.natural_healsp_interval){
			sd->sp_sub -= battle_config.natural_healsp_interval;
			if(sd->status.sp + bonus <= sd->status.max_sp)
				sd->status.sp += bonus;
			else {
				sd->status.sp = sd->status.max_sp;
				sd->sp_sub = sd->inchealsptick = 0;
			}
		}
	}

	if(bsp != sd->status.sp)
		clif_updatestatus(sd,SP_SP);

	if(sd->nshealsp > 0) {
		if(sd->inchealsptick >= battle_config.natural_heal_skill_interval && sd->status.sp < sd->status.max_sp) {
			if(sd->doridori_counter && s_class.job == 23)
				bonus = sd->nshealsp*2;
			else
				bonus = sd->nshealsp;
			sd->doridori_counter = 0;
			while(sd->inchealsptick >= battle_config.natural_heal_skill_interval) {
				sd->inchealsptick -= battle_config.natural_heal_skill_interval;
				if(sd->status.sp + bonus <= sd->status.max_sp)
					sd->status.sp += bonus;
				else {
					bonus = sd->status.max_sp - sd->status.sp;
					sd->status.sp = sd->status.max_sp;
					sd->sp_sub = sd->inchealsptick = 0;
				}
				clif_heal(sd->fd,SP_SP,bonus);
			}
		}
	}
	else sd->inchealsptick = 0;

	return 0;
}

static int pc_spirit_heal_hp(struct map_session_data *sd,int level)
{
	int bonus_hp,interval = battle_config.natural_heal_skill_interval;

	nullpo_retr(0, sd);

	if(pc_checkoverhp(sd)) {
		sd->inchealspirithptick = 0;
		return 0;
	}

	sd->inchealspirithptick += natural_heal_diff_tick;

	if(sd->weight*100/sd->max_weight >= battle_config.natural_heal_weight_rate)
		interval += interval;

	if(sd->inchealspirithptick >= interval) {
		bonus_hp = sd->nsshealhp;
		while(sd->inchealspirithptick >= interval) {
			if(pc_issit(sd)) {
				sd->inchealspirithptick -= interval;
				if(sd->status.hp < sd->status.max_hp) {
					if(sd->status.hp + bonus_hp <= sd->status.max_hp)
						sd->status.hp += bonus_hp;
					else {
						bonus_hp = sd->status.max_hp - sd->status.hp;
						sd->status.hp = sd->status.max_hp;
					}
					clif_heal(sd->fd,SP_HP,bonus_hp);
					sd->inchealspirithptick = 0;
				}
			}else{
				sd->inchealspirithptick -= natural_heal_diff_tick;
				break;
			}
		}
	}

	return 0;
}
static int pc_spirit_heal_sp(struct map_session_data *sd,int level)
{
	int bonus_sp,interval = battle_config.natural_heal_skill_interval;

	nullpo_retr(0, sd);

	if(pc_checkoversp(sd)) {
		sd->inchealspiritsptick = 0;
		return 0;
	}

	sd->inchealspiritsptick += natural_heal_diff_tick;

	if(sd->weight*100/sd->max_weight >= battle_config.natural_heal_weight_rate)
		interval += interval;

	if(sd->inchealspiritsptick >= interval) {
		bonus_sp = sd->nsshealsp;
		while(sd->inchealspiritsptick >= interval) {
			if(pc_issit(sd)) {
				sd->inchealspiritsptick -= interval;
				if(sd->status.sp < sd->status.max_sp) {
					if(sd->status.sp + bonus_sp <= sd->status.max_sp)
						sd->status.sp += bonus_sp;
					else {
						bonus_sp = sd->status.max_sp - sd->status.sp;
						sd->status.sp = sd->status.max_sp;
					}
					clif_heal(sd->fd,SP_SP,bonus_sp);
					sd->inchealspiritsptick = 0;
				}
			}else{
				sd->inchealspiritsptick -= natural_heal_diff_tick;
				break;
			}
		}
	}

	return 0;
}

static int pc_rest_heal_hp(struct map_session_data *sd)
{
	int bonus_hp,interval = battle_config.natural_heal_skill_interval;

	nullpo_retr(0, sd);

	if(pc_checkoverhp(sd)) {
		sd->inchealresthptick = 0;
		return 0;
	}
	
	sd->inchealresthptick += natural_heal_diff_tick;
	
	//if(sd->weight*100/sd->max_weight >= battle_config.natural_heal_weight_rate)
	//	interval += interval;
	
	if(sd->inchealresthptick >= interval) {
		bonus_hp = sd->tk_nhealhp;
		if(sd->tk_doridori_counter_hp)
			bonus_hp += bonus_hp;
		sd->tk_doridori_counter_hp = 0;
			
		while(sd->inchealresthptick >= interval) {
			if(pc_issit(sd)) {
				sd->inchealresthptick -= interval;
				if(sd->status.hp < sd->status.max_hp) {
					if(sd->status.hp + bonus_hp <= sd->status.max_hp)
						sd->status.hp += bonus_hp;
					else {
						bonus_hp = sd->status.max_hp - sd->status.hp;
						sd->status.hp = sd->status.max_hp;
					}
					clif_heal(sd->fd,SP_HP,bonus_hp);
					sd->inchealresthptick = 0;
				}
			}else{
				sd->inchealresthptick -= natural_heal_diff_tick;
				break;
			}
		}
	}

	return 0;
}
static int pc_rest_heal_sp(struct map_session_data *sd)
{
	int bonus_sp,interval = battle_config.natural_heal_skill_interval;

	nullpo_retr(0, sd);

	if(pc_checkoversp(sd)) {
		sd->inchealrestsptick = 0;
		return 0;
	}

	sd->inchealrestsptick += natural_heal_diff_tick;

	//if(sd->weight*100/sd->max_weight >= battle_config.natural_heal_weight_rate)
	//	interval += interval;

	if(sd->inchealrestsptick >= interval) {
		bonus_sp = sd->tk_nhealsp;
		if(sd->tk_doridori_counter_sp)
			bonus_sp+=bonus_sp;
		sd->tk_doridori_counter_sp = 0;
		
		while(sd->inchealrestsptick >= interval) {
			if(pc_issit(sd)) {
				sd->inchealrestsptick -= interval;
				if(sd->status.sp < sd->status.max_sp) {
					if(sd->status.sp + bonus_sp <= sd->status.max_sp)
						sd->status.sp += bonus_sp;
					else {
						bonus_sp = sd->status.max_sp - sd->status.sp;
						sd->status.sp = sd->status.max_sp;
					}
					clif_heal(sd->fd,SP_SP,bonus_sp);
					sd->inchealrestsptick = 0;
				}
			}else{
				sd->inchealrestsptick -= natural_heal_diff_tick;
				break;
			}
		}
	}

	
	return 0;
}

static int pc_bleeding(struct map_session_data *sd)
{
	int hp = 0, sp = 0;
	nullpo_retr(0, sd);

	if (sd->hp_penalty_value > 0) {
		sd->hp_penalty_tick += natural_heal_diff_tick;
		if (sd->hp_penalty_tick >= sd->hp_penalty_time) {
			do {
				hp += sd->hp_penalty_value;
				sd->hp_penalty_tick -= sd->hp_penalty_time;
			} while (sd->hp_penalty_tick >= sd->hp_penalty_time);
			sd->hp_penalty_tick = 0;
		}
	}
	
	if (sd->sp_penalty_value > 0) {
		sd->sp_penalty_tick += natural_heal_diff_tick;
		if (sd->sp_penalty_tick >= sd->sp_penalty_time) {
			do {
				sp += sd->sp_penalty_value;
				sd->sp_penalty_tick -= sd->sp_penalty_time;
			} while (sd->sp_penalty_tick >= sd->sp_penalty_time);
			sd->sp_penalty_tick = 0;
		}
	}

	if (hp > 0 || sp > 0)
		pc_heal(sd,-hp,-sp);

	return 0;
}
/*==========================================
 * HP/SP ���R�� �e�N���C�A���g
 *------------------------------------------
 */

static int pc_natural_heal_sub(struct map_session_data *sd,va_list ap)
{
	int skill;

	nullpo_retr(0, sd);

	if( (battle_config.natural_heal_weight_rate > 100 || sd->weight*100/sd->max_weight < battle_config.natural_heal_weight_rate) &&
		!pc_isdead(sd) && 
		!pc_ishiding(sd) && 
		sd->sc_data[SC_POISON].timer == -1 &&
		sd->sc_data[SC_GOSPEL].timer == -1
	  ){
		pc_natural_heal_hp(sd);
		if( sd->sc_data && 
			sd->sc_data[SC_MAXIMIZEPOWER].timer == -1 &&	//�}�L�V�}�C�Y�p���[��Ԃł�SP���񕜂��Ȃ�
//			sd->sc_data[SC_EXPLOSIONSPIRITS].timer == -1 &&	//�����g����Ԃł�SP���񕜂��Ȃ�
			sd->sc_data[SC_EXTREMITYFIST].timer == -1 &&	//���C����Ԃł�SP���񕜂��Ȃ�
			sd->sc_data[SC_DANCING].timer == -1 &&	//�_���X��Ԃł�SP���񕜂��Ȃ�
			sd->sc_data[SC_BERSERK].timer == -1 &&	//�o�[�T�[�N��Ԃł�SP���񕜂��Ȃ�
			sd->sc_data[SC_GOSPEL].timer == -1		//�S�X�y����Ԃł�SP���񕜂��Ȃ�
		) 
			pc_natural_heal_sp(sd);
	}
	else {
		sd->hp_sub = sd->inchealhptick = 0;
		sd->sp_sub = sd->inchealsptick = 0;
	}
	if((skill = pc_checkskill(sd,MO_SPIRITSRECOVERY)) > 0 && !pc_ishiding(sd) && sd->sc_data[SC_POISON].timer == -1){
		pc_spirit_heal_hp(sd,skill);
		pc_spirit_heal_sp(sd,skill);
	}
	else {
		sd->inchealspirithptick = 0;
		sd->inchealspiritsptick = 0;
	}
	
	//���炩�ȋx��
	if((skill = pc_checkskill(sd,TK_HPTIME)) > 0 && !pc_ishiding(sd) && sd->sc_data[SC_POISON].timer == -1)
		pc_rest_heal_hp(sd);
	else
		sd->inchealresthptick = 0;
	
	//�y�����x��
	if((skill = pc_checkskill(sd,TK_SPTIME)) > 0 && !pc_ishiding(sd) && sd->sc_data[SC_POISON].timer == -1)
		pc_rest_heal_sp(sd);
	else 
		sd->inchealrestsptick = 0;
	
	if (sd->hp_penalty_value > 0 || sd->sp_penalty_value > 0)
		pc_bleeding(sd);
	else
		sd->hp_penalty_tick = sd->sp_penalty_tick = 0;
		
	return 0;
}

/*==========================================
 * HP/SP���R�� (interval timer�֐�)
 *------------------------------------------
 */
int pc_natural_heal(int tid,unsigned int tick,int id,int data)
{
	natural_heal_tick = tick;
	natural_heal_diff_tick = DIFF_TICK(natural_heal_tick,natural_heal_prev_tick);
	clif_foreachclient(pc_natural_heal_sub);

	natural_heal_prev_tick = tick;
	return 0;
}

/*==========================================
 * �Z�[�u�|�C���g�̕ۑ�
 *------------------------------------------
 */
int pc_setsavepoint(struct map_session_data *sd,char *mapname,int x,int y)
{
	nullpo_retr(0, sd);

	strncpy(sd->status.save_point.map,mapname,24);
	sd->status.save_point.x = x;
	sd->status.save_point.y = y;
	if(strcmp(sd->status.save_point.map,"SavePoint") &&
			strstr(sd->status.save_point.map,".gat")==NULL) {
		strcat(sd->status.save_point.map,".gat");
	}
	return 0;
}


/*==========================================
 * �����Z�[�u �e�N���C�A���g
 *------------------------------------------
 */
static int last_save_fd,save_flag;
static int pc_autosave_sub(struct map_session_data *sd,va_list ap)
{
	nullpo_retr(0, sd);

	if(save_flag==0 && sd->fd>last_save_fd && !sd->state.waitingdisconnect){
//		if(battle_config.save_log)
//			printf("autosave %d\n",sd->fd);
		// pet
		if(sd->status.pet_id > 0 && sd->pd)
			intif_save_petdata(sd->status.account_id,&sd->pet);
		pc_makesavestatus(sd);
		chrif_save(sd);
		storage_storage_save(sd);
		if(sd->state.storage_flag)
			storage_guild_storagesave(sd);
		save_flag=1;
		last_save_fd = sd->fd;
	}

	return 0;
}


/*==========================================
 * �����Z�[�u (timer�֐�)
 *------------------------------------------
 */
int pc_autosave(int tid,unsigned int tick,int id,int data)
{
	int interval;

	save_flag=0;
	clif_foreachclient(pc_autosave_sub);
	if(save_flag==0)
		last_save_fd=0;

	interval = autosave_interval/(clif_countusers()+1);
	if(interval <= 200)
		interval = 200;
	add_timer(gettick()+interval,pc_autosave,0,0);

	return 0;
}


int pc_read_gm_account()
{
	char line[8192];
	struct gm_account *p;
	FILE *fp;
	int c=0;

	gm_account_db=numdb_init();

	if( (fp=fopen(GM_account_filename,"r"))==NULL )
		return 1;
	while(fgets(line,sizeof(line),fp)){
		if(line[0] == '/' && line[1] == '/')
			continue;
		p=(struct gm_account *)aCalloc(1,sizeof(struct gm_account));
		if(sscanf(line,"%d %d",&p->account_id,&p->level) != 2 || p->level <= 0) {
			printf("gm_account: broken data [%s] line %d\n",GM_account_filename,c);
		}
		else {
			if(p->level > 99)
				p->level = 99;
			numdb_insert(gm_account_db,p->account_id,p);
		}
		c++;
	}
	fclose(fp);
	printf("read %s done (%d gm account ID)\n",GM_account_filename,c);

	return 0;
}

void pc_setstand(struct map_session_data *sd){
	nullpo_retv(sd);

	if(sd->sc_data && sd->sc_data[SC_TENSIONRELAX].timer!=-1)
		status_change_end(&sd->bl,SC_TENSIONRELAX,-1);

	sd->state.dead_sit = 0;
}

//
// ��������
//
/*==========================================
 * �ݒ�t�@�C���ǂݍ���
 * exp.txt �K�v�o���l
 * job_db1.txt �d��,hp,sp,�U�����x => status.c�@�Ɉړ�
 * job_db2.txt job�\�͒l�{�[�i�X
 * skill_tree.txt �e�E���̃X�L���c���[
 * attr_fix.txt �����C���e�[�u��
 * size_fix.txt �T�C�Y�␳�e�[�u��
 * refine_db.txt ���B�f�[�^�e�[�u��
 *------------------------------------------
 */
int pc_readdb(void)
{
	int i,j,k,upper=0;
	FILE *fp;
	char line[1024],*p;

	// �K�v�o���l�ǂݍ���

	fp=fopen("db/exp.txt","r");
	if(fp==NULL){
		printf("can't read db/exp.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line,1020,fp)){
		int bn,b1,b2,b3,b4,b5,b6,jn,j1,j2,j3,j4,j5,j6,j7;
		if(line[0]=='/' && line[1]=='/')
			continue;
		if(sscanf(line,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",&bn,&b1,&b2,&b3,&b4,&b5,&b6,&jn,&j1,&j2,&j3,&j4,&j5,&j6,&j7)!=15)
			continue;
		exp_table[0][i]=bn;
		exp_table[1][i]=b1;
		exp_table[2][i]=b2;
		exp_table[3][i]=b3;
		exp_table[4][i]=b4;
		exp_table[5][i]=b5;
		exp_table[6][i]=b6;
		exp_table[7][i]=jn;
		exp_table[8][i]=j1;
		exp_table[9][i]=j2;
		exp_table[10][i]=j3;
		exp_table[11][i]=j4;
		exp_table[12][i]=j5;
		exp_table[13][i]=j6;
		exp_table[14][i]=j7;
		i++;
		if(i >= MAX_LEVEL)
			break;
	}
	fclose(fp);
	printf("read db/exp.txt done\n");

	// �X�L���c���[
	memset(skill_tree,0,sizeof(skill_tree));
	fp=fopen("db/skill_tree.txt","r");
	if(fp==NULL){
		printf("can't read db/skill_tree.txt\n");
		return 1;
	}
	while(fgets(line,1020,fp)){
		char *split[50];
		if(line[0]=='/' && line[1]=='/')
			continue;
		for(j=0,p=line;j<16 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(j<16)
			continue;
		upper=atoi(split[0]);
		if(upper>0 && battle_config.enable_upper_class==0){ //conf�Ŗ����ɂȂ��Ă�����
			continue;
		}
		i=atoi(split[1]);
		for(j=0;skill_tree[upper][i][j].id;j++);
		skill_tree[upper][i][j].id=atoi(split[2]);
		skill_tree[upper][i][j].max=atoi(split[3]);
		for(k=0;k<5;k++){
			skill_tree[upper][i][j].need[k].id=atoi(split[k*2+4]);
			skill_tree[upper][i][j].need[k].lv=atoi(split[k*2+5]);
		}
		
		skill_tree[upper][i][j].base_level=atoi(split[14]);
		skill_tree[upper][i][j].job_level=atoi(split[15]);
	}
	fclose(fp);
	printf("read db/skill_tree.txt done\n");

	// �����C���e�[�u��
	for(i=0;i<4;i++)
		for(j=0;j<10;j++)
			for(k=0;k<10;k++)
				attr_fix_table[i][j][k]=100;
	fp=fopen("db/attr_fix.txt","r");
	if(fp==NULL){
		printf("can't read db/attr_fix.txt\n");
		return 1;
	}
	while(fgets(line,1020,fp)){
		char *split[10];
		int lv,n;
		if(line[0]=='/' && line[1]=='/')
			continue;
		for(j=0,p=line;j<3 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		lv=atoi(split[0]);
		n=atoi(split[1]);
//		printf("%d %d\n",lv,n);

		for(i=0;i<n;){
			if( !fgets(line,1024,fp) )
				break;
			if(line[0]=='/' && line[1]=='/')
				continue;

			for(j=0,p=line;j<n && p;j++){
				while(*p==32 && *p>0)
					p++;
				attr_fix_table[lv-1][i][j]=atoi(p);
				if(battle_config.attr_recover == 0 && attr_fix_table[lv-1][i][j] < 0)
					attr_fix_table[lv-1][i][j] = 0;
				p=strchr(p,',');
				if(p) *p++=0;
			}

			i++;
		}
	}
	fclose(fp);
	printf("read db/attr_fix.txt done\n");

	return 0;
}

/*==========================================
 * �I��
 *------------------------------------------
 */
static int gm_account_db_final(void *key,void *data,va_list ap)
{
	struct gm_account *p=data;

	free(p);

	return 0;
}
int do_final_pc(void)
{
	if(gm_account_db)
		numdb_final(gm_account_db,gm_account_db_final);
	return 0;
}
/*==========================================
 * pc�֌W������
 *------------------------------------------
 */
int do_init_pc(void)
{
	pc_readdb();
	pc_read_gm_account();

	add_timer_func_list(pc_walk,"pc_walk");
	add_timer_func_list(pc_attack_timer,"pc_attack_timer");
	add_timer_func_list(pc_natural_heal,"pc_natural_heal");
	add_timer_func_list(pc_invincible_timer,"pc_invincible_timer");
	add_timer_func_list(pc_eventtimer,"pc_eventtimer");
	add_timer_func_list(pc_calc_pvprank_timer,"pc_calc_pvprank_timer");
	add_timer_func_list(pc_autosave,"pc_autosave");
	add_timer_func_list(pc_spiritball_timer,"pc_spiritball_timer");
	add_timer_interval((natural_heal_prev_tick=gettick()+NATURAL_HEAL_INTERVAL),pc_natural_heal,0,0,NATURAL_HEAL_INTERVAL);
	add_timer(gettick()+autosave_interval,pc_autosave,0,0);

	return 0;
}
