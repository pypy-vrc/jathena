
#include <stdio.h>
#include <stdlib.h>

#include "timer.h"
#include "nullpo.h"
#include "map.h"
#include "pc.h"
#include "mob.h"
#include "pet.h"
#include "skill.h"
#include "unit.h"
#include "battle.h"
#include "status.h"
#include "clif.h"
#include "party.h"
#include "npc.h"
#include "chat.h"
#include "trade.h"
#include "guild.h"
#include "friend.h"
#include "malloc.h"
#include "mob.h"
#include "db.h"

static int dirx[8]={0,-1,-1,-1,0,1,1,1};
static int diry[8]={1,1,0,-1,-1,-1,0,1};

/*==========================================
 * ��_�Ԃ̋�����Ԃ�
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int unit_distance(int x0,int y0,int x1,int y1)
{
	int dx,dy;

	dx=abs(x0-x1);
	dy=abs(y0-y1);
	return dx>dy ? dx : dy;
}

int unit_distance2( struct block_list *bl, struct block_list *bl2)
{
	nullpo_retr(0, bl);
	nullpo_retr(0, bl2);

	return unit_distance(bl->x,bl->y,bl2->x,bl2->y);
}

struct unit_data* unit_bl2ud(struct block_list *bl) {
	if( bl == NULL) return NULL;
	if( bl->type == BL_PC)  return &((struct map_session_data*)bl)->ud;
	if( bl->type == BL_MOB) return &((struct mob_data*)bl)->ud;
	if( bl->type == BL_PET) return &((struct pet_data*)bl)->ud;
	return NULL;
}

static int unit_walktoxy_timer(int tid,unsigned int tick,int id,int data);

int unit_walktoxy_sub(struct block_list *bl)
{
	int i;
	struct walkpath_data wpd;
	struct map_session_data *sd = NULL;
	struct pet_data         *pd = NULL;
	struct mob_data         *md = NULL;
	struct unit_data        *ud = NULL;

	nullpo_retr(1, bl);

	if( BL_CAST( BL_PC,  bl, sd ) ) {
		ud = &sd->ud;
	} else if( BL_CAST( BL_MOB, bl, md ) ) {
		ud = &md->ud;
	} else if( BL_CAST( BL_PET, bl, pd ) ) {
		ud = &pd->ud;
	}
	if(ud == NULL) return 1;

	if(sd && pc_iscloaking(sd))// �N���[�L���O���Čv�Z
		status_calc_pc(sd,0);

	if(sd && sd->sc_data && sd->sc_data[SC_HIGHJUMP].timer!=-1) {
		if(path_search2(&wpd,bl->m,bl->x,bl->y,ud->to_x,ud->to_y,0))
			return 1;
	} else {
		if(path_search(&wpd,bl->m,bl->x,bl->y,ud->to_x,ud->to_y,0))
			return 1;
	}

	if(md) {
		int x = md->bl.x+dirx[wpd.path[0]];
		int y = md->bl.y+diry[wpd.path[0]];
		if (map_getcell(bl->m,x,y,CELL_CHKBASILICA) && !(status_get_mode(bl)&0x20)) {
			ud->state.change_walk_target=0;
			return 1;
		}
	}

	memcpy(&ud->walkpath,&wpd,sizeof(wpd));

	if(sd) {
		clif_walkok(sd);
	} else if(md) {
		clif_movemob(md);
	} else if(pd) {
		clif_movepet(pd);
	}

	ud->state.change_walk_target=0;

    if(ud->walkpath.path_pos>=ud->walkpath.path_len)
		i = -1;
	else if(ud->walkpath.path[ud->walkpath.path_pos]&1)
		i = status_get_speed(bl)*14/10;
	else
		i = status_get_speed(bl);
	if( i  > 0) {
		i = i>>1;
		ud->walktimer = add_timer(gettick()+i,unit_walktoxy_timer,bl->id,0);
	}
	if(sd) {
		clif_movechar(sd);
	}

	return 0;
}


static int unit_walktoxy_timer(int tid,unsigned int tick,int id,int data)
{
	int i;
	int moveblock;
	int x,y,dx,dy,dir;
	struct map_session_data *sd = NULL;
	struct pet_data         *pd = NULL;
	struct mob_data         *md = NULL;
	struct block_list       *bl;
	struct unit_data        *ud = NULL;

	bl=map_id2bl(id);
	if(bl == NULL)
		return 0;
	if( BL_CAST( BL_PC,  bl, sd ) ) {
		ud = &sd->ud;
	} else if( BL_CAST( BL_MOB, bl, md ) ) {
		ud = &md->ud;
	} else if( BL_CAST( BL_PET, bl, pd ) ) {
		ud = &pd->ud;
	}
	if(ud == NULL) return 0;

	if(ud->walktimer != tid){
		if(battle_config.error_log)
			printf("unit_walk_timer %d != %d\n",ud->walktimer,tid);
		return 0;
	}
	ud->walktimer=-1;

	if(ud->walkpath.path_pos>=ud->walkpath.path_len || ud->walkpath.path_pos!=data)
		return 0;
	
	//�������̂ő����̃^�C�}�[��������
	if(sd) {
		sd->inchealspirithptick = 0;
		sd->inchealspiritsptick = 0;
	}
	ud->walkpath.path_half ^= 1;
	if(ud->walkpath.path_half==0){ // �}�X�ڒ��S�֓���
		ud->walkpath.path_pos++;
		if(ud->state.change_walk_target) {
			unit_walktoxy_sub(bl);
			return 0;
		}
	} else { // �}�X�ڋ��E�֓���
		if( (md || pd) && ud->walkpath.path[ud->walkpath.path_pos]>=8)
			return 1;
		x = bl->x;
		y = bl->y;

		dir = ud->walkpath.path[ud->walkpath.path_pos];
		if(sd) sd->dir = sd->head_dir = dir;
		if(md) md->dir = dir;
		if(pd) pd->dir = dir;

		dx = dirx[(int)dir];
		dy = diry[(int)dir];

		// ��Q���ɓ�������
		if(map_getcell(bl->m,x+dx,y+dy,CELL_CHKNOPASS))
		{
			if(!sd || sd->sc_data[SC_HIGHJUMP].timer==-1) {
				clif_fixwalkpos(bl);
				return 0;
			}
		}
		// �o�V���J����
		if(md && map_getcell(bl->m,x+dx,y+dy,CELL_CHKBASILICA) && !(status_get_mode(bl)&0x20)) {
			clif_fixwalkpos(bl);
			return 0;
		}

		moveblock = ( x/BLOCK_SIZE != (x+dx)/BLOCK_SIZE || y/BLOCK_SIZE != (y+dy)/BLOCK_SIZE);

		ud->walktimer = 1;
		if(sd) {
			map_foreachinmovearea(clif_pcoutsight,bl->m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,dx,dy,0,sd);
		} else if(md) {
			map_foreachinmovearea(clif_moboutsight,bl->m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,dx,dy,BL_PC,md);
		} else if(pd) {
			map_foreachinmovearea(clif_petoutsight,bl->m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,dx,dy,BL_PC,pd);
		}
		ud->walktimer = -1;

		x += dx;
		y += dy;

		if(md && md->min_chase>13)
			md->min_chase--;

		if(!pd) skill_unit_move(bl,tick,0);
		if(moveblock) map_delblock(bl);
		bl->x = x;
		bl->y = y;
		if(moveblock) map_addblock(bl);
		if(!pd) skill_unit_move(bl,tick,1);
		

		if(sd && (sd->sc_data[SC_DANCING].timer != -1 && sd->sc_data[SC_LONGINGFREEDOM].timer == -1)) // Not �S�����Ȃ���
		{
			skill_unit_move_unit_group((struct skill_unit_group *)sd->sc_data[SC_DANCING].val2,sd->bl.m,dx,dy);
			sd->dance.x += dx;
			sd->dance.y += dy;
		}

		ud->walktimer = 1;
		if(sd) {
			map_foreachinmovearea(clif_pcinsight,bl->m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,-dx,-dy,0,sd);
		} else if(md) {
			map_foreachinmovearea(clif_mobinsight,bl->m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,-dx,-dy,BL_PC,md);
		} else if(pd) {
			map_foreachinmovearea(clif_petinsight,bl->m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,-dx,-dy,BL_PC,pd);
		}
		ud->walktimer = -1;

		if(md) {
			if(md->option&4)
				skill_check_cloaking(bl);
		}

		if(sd) {
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
			if(sd->sc_data)
			{
				/* ��f�B�{�[�V�������� */
				if(sd->sc_data[SC_DEVOTION].val1){
						skill_devotion2(&sd->bl,sd->sc_data[SC_DEVOTION].val1);
				}
				/* �}���I�l�b�g���� */
				if(sd->sc_data[SC_MARIONETTE].timer!=-1){
					skill_marionette(&sd->bl,sd->sc_data[SC_MARIONETTE].val2);
				}
				/* ��}���I�l�b�g���� */
				if(sd->sc_data[SC_MARIONETTE2].timer!=-1){
					skill_marionette2(&sd->bl,sd->sc_data[SC_MARIONETTE2].val2);
				}
				//�_���X�`�F�b�N
				if(sd->sc_data[SC_LONGINGFREEDOM].timer!=-1)
				{
					//�͈͊O�ɏo����~�߂�
					if(unit_distance(sd->bl.x,sd->bl.y,sd->dance.x,sd->dance.y)>4)
					{
						skill_stop_dancing(&sd->bl,0);
					}
				}
				//�w�����[�h�`�F�b�N
				if(battle_config.hermode_wp_check && 
					sd->sc_data[SC_DANCING].timer !=-1 && sd->sc_data[SC_DANCING].val1 ==CG_HERMODE)
				{
					if(skill_hermode_wp_check(&sd->bl,battle_config.hermode_wp_check_range)==0)
						skill_stop_dancing(&sd->bl,0);
				}
			}
			//�M���h�X�L���L��
			pc_check_guild_skill_effective_range(sd);

			if(map_getcell(sd->bl.m,x,y,CELL_CHKNPC))
				npc_touch_areanpc(sd,sd->bl.m,x,y);
			else
				sd->areanpc_id=0;
		}
	}

    if(ud->walkpath.path_pos>=ud->walkpath.path_len)
		i = -1;
	else if(ud->walkpath.path[ud->walkpath.path_pos]&1)
		i = status_get_speed(bl)*14/10;
	else
		i = status_get_speed(bl);

	if(i > 0) {
		i = i>>1;
//		if(i < 1 && ud->walkpath.path_half == 0)
//			i = 1;
		ud->walktimer = add_timer(tick+i,unit_walktoxy_timer,id,ud->walkpath.path_pos);
	} else {
		// �ړI�n�ɒ�����
		if(sd && sd->sc_data){
			//�p������
			if(sd->sc_data[SC_RUN].timer!=-1)
			{
				pc_runtodir(sd);
				return 0;
			}
			if(sd->sc_data[SC_HIGHJUMP].timer!=-1 && sd->sc_data[SC_HIGHJUMP].val4==0)
			{
				sd->sc_data[SC_HIGHJUMP].val4++;
				pc_walktodir(sd,1);
				return 0;
			}
			
			if(sd->sc_data[SC_HIGHJUMP].timer!=-1 && 
				sd->sc_data[SC_HIGHJUMP].val4==1){
				sd->sc_data[SC_HIGHJUMP].val4++;
			//	status_change_end(&sd->bl,SC_HIGHJUMP,-1);
			}
		}

		// �Ƃ܂����Ƃ��̈ʒu�̍đ��M�͕s�v�i�J�N�J�N���邽�߁j
		// clif_fixwalkpos(bl);

	}

	return 0;
}

int unit_walktoxy( struct block_list *bl, int x, int y) {
	struct unit_data        *ud = NULL;
	struct map_session_data *sd = NULL;
	struct pet_data         *pd = NULL;
	struct mob_data         *md = NULL;

	nullpo_retr(0, bl);
	if( BL_CAST( BL_PC,  bl, sd ) ) {
		ud = &sd->ud;
	} else if( BL_CAST( BL_MOB, bl, md ) ) {
		ud = &md->ud;
	} else if( BL_CAST( BL_PET, bl, pd ) ) {
		ud = &pd->ud;
	}
	if( ud == NULL) return 0;
	if( ud->canmove_tick > gettick() ) return 0;

	// �ړ��o���Ȃ����j�b�g�͒e��
	if( ! ( status_get_mode( bl ) & 1) ) return 0;

	//�����т͂܂������Ȃ̂ŏ��O
	if(sd && sd->sc_data[SC_HIGHJUMP].timer==-1 && sd->sc_data[SC_CONFUSION].timer!=-1)
	{
		ud->to_x = sd->bl.x + atn_rand()%7 - 3;
		ud->to_y = sd->bl.y + atn_rand()%7 - 3;
	}else{
		ud->to_x = x;
		ud->to_y = y;
	}

	if(ud->walktimer != -1) {
		// ���ݕ����Ă���Œ��̖ړI�n�ύX�Ȃ̂Ń}�X�ڂ̒��S�ɗ�������
		// timer�֐�����unit_walktoxy_sub���ĂԂ悤�ɂ���
		ud->state.change_walk_target = 1;
		return 0;
	} else {
		return unit_walktoxy_sub(bl);
	}
}

int unit_movepos(struct block_list *bl,int dst_x,int dst_y)
{
	int moveblock;
	int dx,dy,dir,x[4],y[4];
	int tick = gettick();
	struct map_session_data *sd = NULL;
	struct pet_data         *pd = NULL;
	struct mob_data         *md = NULL;
	struct unit_data        *ud = NULL;
	struct walkpath_data wpd;

	nullpo_retr(0, bl);
	if( BL_CAST( BL_PC,  bl, sd ) ) {
		ud = &sd->ud;
	} else if( BL_CAST( BL_MOB, bl, md ) ) {
		ud = &md->ud;
	} else if( BL_CAST( BL_PET, bl, pd ) ) {
		ud = &pd->ud;
	}
	if( ud == NULL) return 1;

	unit_stop_walking(bl,1);
	unit_stopattack(bl);

	if(sd && sd->sc_data && sd->sc_data[SC_HIGHJUMP].timer!=-1) {
		if(path_search2(&wpd,bl->m,bl->x,bl->y,dst_x,dst_y,0))
			return 1;
	} else {
		if(path_search(&wpd,bl->m,bl->x,bl->y,dst_x,dst_y,0))
			return 1;
	}

	dir = map_calc_dir(bl, dst_x,dst_y);
	if(sd) {
		sd->dir = sd->head_dir = dir;
	}

	// �ʒu�ύX�O�A�ύX�����ʓ��ɂ���N���C�A���g�̍��W���L�^
	x[0] = bl->x-AREA_SIZE;
	x[1] = bl->x+AREA_SIZE;
	x[2] = dst_x-AREA_SIZE;
	x[3] = dst_x+AREA_SIZE;
	y[0] = bl->y-AREA_SIZE;
	y[1] = bl->y+AREA_SIZE;
	y[2] = dst_y-AREA_SIZE;
	y[3] = dst_y+AREA_SIZE;

	dx = dst_x - bl->x;
	dy = dst_y - bl->y;

	moveblock = ( bl->x/BLOCK_SIZE != dst_x/BLOCK_SIZE || bl->y/BLOCK_SIZE != dst_y/BLOCK_SIZE);

	if(sd)	/* ��ʊO�ɏo���̂ŏ��� */
		map_foreachinmovearea(clif_pcoutsight,bl->m,bl->x-AREA_SIZE,bl->y-AREA_SIZE,bl->x+AREA_SIZE,bl->y+AREA_SIZE,dx,dy,0,sd);
	else if(md)
		map_foreachinmovearea(clif_moboutsight,bl->m,bl->x-AREA_SIZE,bl->y-AREA_SIZE,bl->x+AREA_SIZE,bl->y+AREA_SIZE,dx,dy,BL_PC,md);
	else if(pd)
		map_foreachinmovearea(clif_petoutsight,bl->m,bl->x-AREA_SIZE,bl->y-AREA_SIZE,bl->x+AREA_SIZE,bl->y+AREA_SIZE,dx,dy,BL_PC,pd);

	if(!pd) skill_unit_move(bl,tick,0);
	if(moveblock) map_delblock(bl);
	bl->x = dst_x;
	bl->y = dst_y;
	if(moveblock) map_addblock(bl);
	if(!pd) skill_unit_move(bl,tick,1);

	if(sd) {	/* ��ʓ��ɓ����Ă����̂ŕ\�� */
		map_foreachinmovearea(clif_pcinsight,bl->m,bl->x-AREA_SIZE,bl->y-AREA_SIZE,bl->x+AREA_SIZE,bl->y+AREA_SIZE,-dx,-dy,0,sd);
	} else if(md) {
		map_foreachinmovearea(clif_mobinsight,bl->m,bl->x-AREA_SIZE,bl->y-AREA_SIZE,bl->x+AREA_SIZE,bl->y+AREA_SIZE,-dx,-dy,BL_PC,md);
	} else if(pd) {
		map_foreachinmovearea(clif_petinsight,bl->m,bl->x-AREA_SIZE,bl->y-AREA_SIZE,bl->x+AREA_SIZE,bl->y+AREA_SIZE,-dx,-dy,BL_PC,pd);
	}

	// �ʒu�ύX��񑗐M
	clif_fixpos2( bl, x, y );

	if(sd && sd->status.party_id>0){	// �p�[�e�B�̂g�o���ʒm����
		struct party *p=party_search(sd->status.party_id);
		if(p!=NULL){
			int flag=0;
			map_foreachinmovearea(party_send_hp_check,sd->bl.m,sd->bl.x-AREA_SIZE,sd->bl.y-AREA_SIZE,sd->bl.x+AREA_SIZE,sd->bl.y+AREA_SIZE,-dx,-dy,BL_PC,sd->status.party_id,&flag);
			if(flag)
				sd->party_hp=-1;
		}
	}

	if(sd && !(sd->status.option&0x4000) && sd->status.option&4)	// �N���[�L���O�̏��Ō���
	{
		if(pc_checkskill(sd,AS_CLOAKING) < 3)
			skill_check_cloaking(&sd->bl);
	}

	if(sd) {
		if(map_getcell(bl->m,bl->x,bl->y,CELL_CHKNPC))
			npc_touch_areanpc(sd,sd->bl.m,sd->bl.x,sd->bl.y);
		else
			sd->areanpc_id=0;
	}

	return 0;
}


int unit_setdir(struct block_list *bl,int dir)
{
	if(bl->type == BL_PC)
	{
		((struct map_session_data *)bl)->dir = dir;
		((struct map_session_data *)bl)->head_dir = dir;
	}else if(bl->type == BL_MOB)
		return ((struct mob_data *)bl)->dir = dir;
	else if(bl->type == BL_PET)
		return ((struct pet_data *)bl)->dir = dir;
	return 0;
}
int unit_getdir(struct block_list *bl)
{
	if(bl->type == BL_PC)
		return ((struct map_session_data *)bl)->dir;
	else if(bl->type == BL_MOB)
		return ((struct mob_data         *)bl)->dir;
	else if(bl->type == BL_PET)
		return ((struct pet_data         *)bl)->dir;
	return 0;
}

/*==========================================
 * ���s��~
 *------------------------------------------
 */
int unit_stop_walking(struct block_list *bl,int type)
{
	struct map_session_data *sd = NULL;
	struct pet_data         *pd = NULL;
	struct mob_data         *md = NULL;
	struct unit_data        *ud = NULL;
	nullpo_retr(0, bl);

	if( BL_CAST( BL_PC,  bl, sd ) ) {
		ud = &sd->ud;
	} else if( BL_CAST( BL_MOB, bl, md ) ) {
		ud = &md->ud;
	} else if( BL_CAST( BL_PET, bl, pd ) ) {
		ud = &pd->ud;
	}
	if( ud == NULL) return 0;

	ud->walkpath.path_len = 0;
	ud->walkpath.path_pos = 0;
	ud->to_x              = bl->x;
	ud->to_y              = bl->y;

	if(ud->walktimer == -1) return 0;

	delete_timer(ud->walktimer, unit_walktoxy_timer);
	ud->walktimer         = -1;
	if(md) { md->state.skillstate = MSS_IDLE; }
	if(type&0x01) { // �ʒu�␳���M���K�v
		clif_fixwalkpos(bl);
	}
	if(type&0x02) { // �_���[�W�H�炤
		unsigned int tick = gettick();
		int delay = status_get_dmotion(bl);
		if( (sd &&battle_config.pc_damage_delay) || (md && battle_config.monster_damage_delay) ) {
			ud->canmove_tick = tick + delay;
		}
	}
	if(type&0x04 && md) {
		int dx=ud->to_x-md->bl.x;
		int dy=ud->to_y-md->bl.y;
		if(dx<0) dx=-1; else if(dx>0) dx=1;
		if(dy<0) dy=-1; else if(dy>0) dy=1;
		if(dx || dy) {
			unit_walktoxy( bl, md->bl.x+dx, md->bl.y+dy );
		}
	}
	if(pd) {
		if(type&~0xff)
			ud->canmove_tick = gettick() + (type>>8);
	}

	return 0;
}

int unit_skilluse_id(struct block_list *src, int target_id, int skill_num, int skill_lv) {
	int id = skill_num;
	if(id >= GD_SKILLBASE)
		id = id - GD_SKILLBASE + MAX_SKILL_DB;

	if( id < 0 || id >= MAX_SKILL_DB+MAX_GUILDSKILL_DB) {
		return 0;
	} else {
		return unit_skilluse_id2(
			src, target_id, skill_num, skill_lv, 
			skill_castfix(src, skill_get_cast( skill_num, skill_lv) ) + skill_get_fixedcast(skill_num, skill_lv),
			skill_db[id].castcancel
		);
	}
}

int unit_skilluse_id2(struct block_list *src, int target_id, int skill_num, int skill_lv, int casttime, int castcancel) {
	struct map_session_data *src_sd = NULL;
	struct pet_data         *src_pd = NULL;
	struct mob_data         *src_md = NULL;
	struct unit_data        *src_ud = NULL;
	unsigned int tick = gettick();
	int delay=0,skilldb_id,range;
	struct block_list      * target;
	struct map_session_data* target_sd = NULL;
	struct mob_data         *target_md = NULL;
	struct unit_data        *target_ud = NULL;
	int forcecast  = 0;
	struct status_change *sc_data;
	struct status_change *tsc_data;

	nullpo_retr(0, src);
	skilldb_id = skill_get_skilldb_id(skill_num);

	if( (target=map_id2bl(target_id)) == NULL ) return 0;
	if(src->m != target->m)                     return 0; // �����}�b�v���ǂ���
	if(!src->prev || !target->prev)             return 0; // map ��ɑ��݂��邩

	if( BL_CAST( BL_PC,  src, src_sd ) ) {
		src_ud = &src_sd->ud;
	} else if( BL_CAST( BL_MOB, src, src_md ) ) {
		src_ud = &src_md->ud;
	} else if( BL_CAST( BL_PET, src, src_pd ) ) {
		src_ud = &src_pd->ud;
	}
	if( src_ud == NULL) return 0;	

	if( BL_CAST( BL_PC,  target, target_sd ) ) {
		target_ud = &target_sd->ud;
	} else if( BL_CAST( BL_MOB, target, target_md ) ) {
		target_ud = &target_md->ud;
	}

	if(unit_isdead(src))			return 0; // ����ł��Ȃ���
	if(src_sd && src_sd->opt1>0 )   return 0; /* ���ق�ُ�i�������A�O�����Ȃǂ̔��������j */

	sc_data  = status_get_sc_data(src);
	tsc_data = status_get_sc_data(target);

	if(skill_get_inf2(skill_num)&0x200 && src->id == target_id)
		return 0;

	//���O�̃X�L���󋵂̋L�^
	if(src_sd) {
		switch(skill_num){
		case SA_CASTCANCEL:
			if(src_ud->skillid != skill_num){ //�L���X�g�L�����Z�����̂͊o���Ȃ�
				src_sd->skillid_old = src_ud->skillid;
				src_sd->skilllv_old = src_ud->skilllv;
				break;
			}
		case BD_ENCORE:					/* �A���R�[�� */
			 //�O��g�p�����x�肪�Ȃ��Ƃ���
			if(!src_sd->skillid_dance || (src_sd->skillid_dance && pc_checkskill(src_sd,src_sd->skillid_dance)<=0)){
				clif_skill_fail(src_sd,skill_num,0,0);
				return 0;
			}else{
				src_sd->skillid_old = skill_num;
			}
			break;
		}
	}

	// �R���f�B�V�����m�F
	{
		struct skill_condition sc;
		memset( &sc, 0, sizeof( struct skill_condition ) );
		
		sc.id     = skill_num;
		sc.lv     = skill_lv;
		sc.target = target_id;

		if(!skill_check_condition2(src, &sc, 0)) return 0;

		skill_num = sc.id;
		skill_lv  = sc.lv;
		target_id = sc.target;
	}

	/* �˒��Ə�Q���`�F�b�N */
	range = skill_get_range(skill_num,skill_lv);
	if(range < 0)
		range = status_get_range(src) - (range + 1);
	if (!battle_check_range(src,target,range + 1))
		return 0;

	if((skill_num != MO_CHAINCOMBO &&
		skill_num != MO_COMBOFINISH &&
		skill_num != MO_EXTREMITYFIST &&
		skill_num != CH_TIGERFIST &&
		skill_num != CH_CHAINCRUSH &&
		skill_num != TK_STORMKICK &&
		skill_num != TK_DOWNKICK &&
		skill_num != TK_TURNKICK &&
		skill_num != TK_COUNTER) ||
		(skill_num == MO_EXTREMITYFIST && src_sd && src_sd->state.skill_flag) )
		unit_stopattack(src);

	if(skill_num != SA_MAGICROD)
		delay=skill_delayfix(src, skill_get_delay( skill_num,skill_lv), skill_get_cast( skill_num,skill_lv) );
	src_ud->state.skillcastcancel = castcancel;

	/* ��������ȏ������K�v */
	// ���s�����skill_check_condition() �ɏ�������
	switch(skill_num){
//	case AL_HEAL:	/* �q�[�� */
//		if(battle_check_undead(status_get_race(bl),status_get_elem_type(bl)))
//			forcecast=1;	/* �q�[���A�^�b�N�Ȃ�r���G�t�F�N�g�L�� */
//		break;
	case ALL_RESURRECTION:	/* ���U���N�V���� */
		if( !target_sd && battle_check_undead(status_get_race(target),status_get_elem_type(target))){	/* �G���A���f�b�h�Ȃ� */
			forcecast=1;	/* �^�[���A���f�b�g�Ɠ����r������ */
			casttime=skill_castfix(src, skill_get_cast(PR_TURNUNDEAD,skill_lv) ) + skill_get_fixedcast(PR_TURNUNDEAD,skill_lv);
		}
		break;
	case MO_FINGEROFFENSIVE:	/* �w�e */
		casttime += casttime * ((skill_lv > src_sd->spiritball)? src_sd->spiritball:skill_lv);
		break;
	case MO_CHAINCOMBO:		/*�A�ŏ�*/
		if( !src_ud ) return 0;
		target_id = src_ud->attacktarget;
		if( sc_data && sc_data[SC_BLADESTOP].timer!=-1 ){
			struct block_list *tbl;
			if((tbl=(struct block_list *)sc_data[SC_BLADESTOP].val4) == NULL) //�^�[�Q�b�g�����Ȃ��H
				return 0;
			target_id = tbl->id;
		}
		break;
	case AS_SONICBLOW:
		if(sc_data && sc_data[SC_ASSASIN].timer!=-1 && map[src->m].flag.gvg==0)
			delay = delay/2;
		break;
	case TK_STORMKICK://�����R��
	case TK_DOWNKICK://���i�R��
	case TK_TURNKICK://��]�R��
	case TK_COUNTER://�J�E���^�[�R��
	case MO_COMBOFINISH:	/*�җ���*/
	case CH_TIGERFIST:		/* ���Ռ� */
	case CH_CHAINCRUSH:		/* �A������ */
		if(! src_ud) return 0;
		target_id = src_ud->attacktarget;
		break;
	case MO_EXTREMITYFIST:	/*���C���e�P��*/
		if(! src_ud || !sc_data) return 0;
		if(sc_data && sc_data[SC_COMBO].timer != -1 && (sc_data[SC_COMBO].val1 == MO_COMBOFINISH || sc_data[SC_COMBO].val1 == CH_CHAINCRUSH)) {
			casttime = 0;
			target_id = src_ud->attacktarget;
		}
		forcecast=1;
		break;
	case SA_MAGICROD:
	case SA_SPELLBREAKER:
		forcecast=1;
		break;
	case WE_MALE:
	case WE_FEMALE:
		{
		struct map_session_data *p_sd = NULL;
		if(! src_sd) return 0;
		if((p_sd = pc_get_partner(src_sd)) == NULL)
			return 0;
		target_id = p_sd->bl.id;
		//range������1�񌟍�
		range = skill_get_range(skill_num,skill_lv);
		if(range < 0)
			range = status_get_range(src) - (range + 1);
		if(!battle_check_range(src,&p_sd->bl,range) ){
			return 0;
			}
		}
		break;
	case SA_ABRACADABRA:
		delay=skill_get_delay(SA_ABRACADABRA,skill_lv);
		break;
	case KN_CHARGEATK:			//�`���[�W�A�^�b�N
		{
			int dist  = unit_distance(src->x,src->y,target->x,target->y);
			if(dist<=3);
			else if(dist<=6) casttime = casttime * 2;
			else 		casttime = casttime * 3;
		}
		break;
	}

	//�������C�Y��ԂȂ�L���X�g�^�C����1/2
	if(sc_data && sc_data[SC_MEMORIZE].timer != -1 && casttime > 0){
		casttime = casttime/2;
		if((--sc_data[SC_MEMORIZE].val2)<=0)
			status_change_end(src, SC_MEMORIZE, -1);
	}

	if(battle_config.pc_skill_log)
		printf("PC %d skill use target_id=%d skill=%d lv=%d cast=%d\n",src->id,target_id,skill_num,skill_lv,casttime);

//	if(src_sd->skillitem == skill_num)
//		casttime = delay = 0;

	if( casttime>0 || forcecast ){ /* �r�����K�v */
		clif_skillcasting( src, src->id, target_id, 0,0, skill_num,casttime);

		/* �r�����������X�^�[ */
		if(src_sd && target_md && mob_db[target_md->class].mode&0x10 && target_md->ud.attacktimer == -1) {
			if(src_sd->invincible_timer != -1) {
				;
			} else {
				target_md->target_id=src->id;
				target_md->min_chase=13;
			}
		}
	}

	if( casttime<=0 )	/* �r���̖������̂̓L�����Z������Ȃ� */
		src_ud->state.skillcastcancel=0;

	src_ud->canact_tick  = tick + casttime + delay;
	src_ud->canmove_tick = tick;
	src_ud->skilltarget  = target_id;
	src_ud->skillx       = 0;
	src_ud->skilly       = 0;
	src_ud->skillid      = skill_num;
	src_ud->skilllv      = skill_lv;

	if(
		(src_sd && !(battle_config.pc_cloak_check_type&2) ) ||
		(src_md && !(battle_config.monster_cloak_check_type&2) )
	) {
	 	if( sc_data[SC_CLOAKING].timer != -1 && skill_num != AS_CLOAKING)
			status_change_end(src,SC_CLOAKING,-1);
	}

	if(casttime > 0) {
		int skill;
		src_ud->skilltimer = add_timer( tick+casttime, skill_castend_id, src->id, 0 );
		if(src_sd && (skill = pc_checkskill(src_sd,SA_FREECAST)) > 0) {
			src_sd->prev_speed = src_sd->speed;
			src_sd->speed = src_sd->speed*(175 - skill*5)/100;
			clif_updatestatus(src_sd,SP_SPEED);
		}
		else
			unit_stop_walking(src,1);
	}
	else {
		if(skill_num != SA_CASTCANCEL)
			src_ud->skilltimer = -1;
		skill_castend_id(src_ud->skilltimer,tick,src->id,0);
	}
	return 1;
}

int unit_skilluse_pos(struct block_list *src, int skill_x, int skill_y, int skill_num, int skill_lv) {
	int id = skill_num;
	if(id >= GD_SKILLBASE)
		id = id - GD_SKILLBASE + MAX_SKILL_DB;

	if( id < 0 || id >= MAX_SKILL_DB+MAX_GUILDSKILL_DB) {
		return 0;
	} else {
		return unit_skilluse_pos2(
			src, skill_x, skill_y, skill_num, skill_lv,
			skill_castfix(src, skill_get_cast( skill_num, skill_lv) ) + skill_get_fixedcast(skill_num, skill_lv),
			skill_db[id].castcancel
		);
	}
}

int unit_skilluse_pos2( struct block_list *src, int skill_x, int skill_y, int skill_num, int skill_lv, int casttime, int castcancel) {
	struct map_session_data *src_sd = NULL;
	struct pet_data         *src_pd = NULL;
	struct mob_data         *src_md = NULL;
	struct unit_data        *src_ud = NULL;
	unsigned int tick = gettick();
	int delay=0,skilldb_id,range;
	struct status_change *sc_data;
	struct block_list    bl;

	nullpo_retr(0, src);
	skilldb_id = skill_get_skilldb_id(skill_num);

	if(!src->prev) return 0; // map ��ɑ��݂��邩

	if( BL_CAST( BL_PC,  src, src_sd ) ) {
		src_ud = &src_sd->ud;
	} else if( BL_CAST( BL_MOB, src, src_md ) ) {
		src_ud = &src_md->ud;
	} else if( BL_CAST( BL_PET, src, src_pd ) ) {
		src_ud = &src_pd->ud;
	}
	if( src_ud == NULL) return 0;

	if(unit_isdead(src)) return 0;

	sc_data    = status_get_sc_data(src);
	skilldb_id = skill_get_skilldb_id(skill_num);

	//�`�F�C�X�E�H�[�N���Ɛݒu�n���s
	if(src_sd && pc_ischasewalk(src_sd))
	 	return 0;

	// �R���f�B�V�����m�F
	{
		struct skill_condition sc;
		memset( &sc, 0, sizeof( struct skill_condition ) );
		sc.id     = skill_num;
		sc.lv     = skill_lv;
		sc.x      = skill_x;
		sc.y      = skill_y;

		if(!skill_check_condition2(src, &sc ,0)) return 0;

		skill_num = sc.id;
		skill_lv  = sc.lv;
		skill_x   = sc.x;
		skill_y   = sc.y;
	}

	/* �˒��Ə�Q���`�F�b�N */
	bl.type = BL_NUL;
	bl.m = src->m;
	bl.x = skill_x;
	bl.y = skill_y;
	range = skill_get_range(skill_num,skill_lv);
	if(range < 0)
		range = status_get_range(src) - (range + 1);

	if(skill_num != TK_HIGHJUMP && !battle_check_range(src,&bl,range+1))
		return 0;
	
	if(skill_num == TK_HIGHJUMP && !map_getcell(src->m,skill_x,skill_y,CELL_CHKPASS))
		return 0;

	unit_stopattack(src);

	delay=skill_delayfix(src, skill_get_delay( skill_num,skill_lv), skill_get_cast( skill_num,skill_lv) );
	src_ud->state.skillcastcancel = castcancel;

	if(battle_config.pc_skill_log)
		printf("PC %d skill use target_pos=(%d,%d) skill=%d lv=%d cast=%d\n",src->id,skill_x,skill_y,skill_num,skill_lv,casttime);

//	if(src_sd->skillitem == skill_num)
//		casttime = delay = 0;
	//�������C�Y��ԂȂ�L���X�g�^�C����1/2
	if(sc_data && sc_data[SC_MEMORIZE].timer != -1 && casttime > 0){
		casttime = casttime/2;
		if((--sc_data[SC_MEMORIZE].val2)<=0)
			status_change_end(src, SC_MEMORIZE, -1);
	}

	if( casttime>0 ) {
		/* �r�����K�v */
		unit_stop_walking( src, 1);		// ���s��~
		clif_skillcasting( src,
			src->id, 0, skill_x,skill_y, skill_num,casttime);
	}

	if( casttime<=0 )	/* �r���̖������̂̓L�����Z������Ȃ� */
		src_ud->state.skillcastcancel=0;

	tick=gettick();
	src_ud->canact_tick  = tick + casttime + delay;
	src_ud->canmove_tick = tick;
	src_ud->skillid      = skill_num;
	src_ud->skilllv      = skill_lv;
	src_ud->skillx       = skill_x;
	src_ud->skilly       = skill_y;
	src_ud->skilltarget  = 0;

	if(src_sd && !(battle_config.pc_cloak_check_type&2) && sc_data[SC_CLOAKING].timer != -1)
		status_change_end(src,SC_CLOAKING,-1);
	if(src_md && !(battle_config.monster_cloak_check_type&2) && sc_data[SC_CLOAKING].timer != -1)
		status_change_end(src,SC_CLOAKING,-1);

	if(casttime > 0) {
		int skill;
		src_ud->skilltimer = add_timer( tick+casttime, skill_castend_pos, src->id, 0 );
		if(src_sd && (skill = pc_checkskill(src_sd,SA_FREECAST)) > 0) {
			src_sd->prev_speed = src_sd->speed;
			src_sd->speed = src_sd->speed*(175 - skill*5)/100;
			clif_updatestatus(src_sd,SP_SPEED);
		}
		else
			unit_stop_walking(src,1);
	}
	else {
		src_ud->skilltimer = -1;
		skill_castend_pos(src_ud->skilltimer,tick,src->id,0);
	}
	return 1;
}

int unit_attack_timer(int tid,unsigned int tick,int id,int data);

// �U����~
int unit_stopattack(struct block_list *bl)
{
	struct unit_data *ud = unit_bl2ud(bl);
	nullpo_retr(0, bl);

	if(!ud || ud->attacktimer == -1) {
		return 0;
	}
	delete_timer( ud->attacktimer, unit_attack_timer );
	ud->attacktimer = -1;
	if(bl->type==BL_MOB) {
		mob_unlocktarget( (struct mob_data*)bl, gettick());
	}
	return 0;
}

int unit_unattackable(struct block_list *bl) {
	if( bl == NULL || bl->type != BL_MOB) return 0;

	mob_unlocktarget( (struct mob_data*)bl, gettick()) ;
	return 0;
}

/*==========================================
 * �U���v��
 * type��1�Ȃ�p���U��
 *------------------------------------------
 */

int unit_attack(struct block_list *src,int target_id,int type)
{
	struct block_list *target;
	struct unit_data  *src_ud;
	int d;

	nullpo_retr(0, src_ud = unit_bl2ud(src));

	target=map_id2bl(target_id);
	if(target==NULL) {
		unit_unattackable(src);
		return 1;
	}
	if(battle_check_target(src,target,BCT_ENEMY)<=0) {
		unit_unattackable(src);
		return 1;
	}
	unit_stopattack(src);
	src_ud->attacktarget          = target_id;
	src_ud->state.attack_continue = type;

	d=DIFF_TICK(src_ud->attackabletime,gettick());
	if(d > 0){	// �U��delay��
		src_ud->attacktimer = add_timer(src_ud->attackabletime,unit_attack_timer,src->id,0);
	} else {
		// �{��timer�֐��Ȃ̂ň��������킹��
		unit_attack_timer(-1,gettick(),src->id,0);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int unit_can_reach(struct block_list *bl,int x,int y)
{
	struct walkpath_data wpd;

	nullpo_retr(0, bl);

	if( bl->x==x && bl->y==y )	// �����}�X
		return 1;

	// ��Q������
	wpd.path_len=0;
	wpd.path_pos=0;
	wpd.path_half=0;
	return (path_search(&wpd,bl->m,bl->x,bl->y,x,y,0)!=-1)?1:0;
}

/*==========================================
 * PC�̍U�� (timer�֐�)
 *------------------------------------------
 */
int unit_attack_timer_sub(int tid,unsigned int tick,int id,int data)
{
	struct block_list *src, *target;
	struct status_change *sc_data, *tsc_data;
	short *opt;
	int dist,skill,range;
	struct unit_data *src_ud;
	struct map_session_data *src_sd = NULL, *target_sd = NULL;
	struct pet_data         *src_pd = NULL;
	struct mob_data         *src_md = NULL, *target_md = NULL;
	if((src=map_id2bl(id))==NULL)        return 0;
	if((src_ud=unit_bl2ud(src)) == NULL) return 0;
	BL_CAST( BL_PC , src, src_sd);
	BL_CAST( BL_PET, src, src_pd);
	BL_CAST( BL_MOB, src, src_md);

	if(src_ud->attacktimer != tid){
		if(battle_config.error_log)
			printf("unit_attack_timer %d != %d\n",src_ud->attacktimer,tid);
		return 0;
	}
	src_ud->attacktimer=-1;

	target=map_id2bl(src_ud->attacktarget);

	if(src->prev == NULL)                    return 0;
	if(target==NULL || target->prev == NULL) return 0;
	BL_CAST( BL_PC , target, target_sd);
	BL_CAST( BL_MOB, target, target_md);

	// ����map�łȂ��Ȃ�U�����Ȃ�
	// PC������łĂ��U�����Ȃ�
	if(src->m != target->m || unit_isdead(src)) return 0;
	sc_data  = status_get_sc_data( src    );
	tsc_data = status_get_sc_data( target );

	if( src_md ) {
		int mode, race;
		if(src_md->opt1>0 || src_md->option&2)          return 0;
		if(src_md->sc_data[SC_AUTOCOUNTER].timer != -1) return 0;
		if(src_md->sc_data[SC_BLADESTOP].timer != -1)   return 0;
		if(src_md->sc_data[SC_WINKCHARM].timer != -1)   return 0;

		if(!src_md->mode)
			mode=mob_db[src_md->class].mode;
		else
			mode=src_md->mode;

		race=mob_db[src_md->class].race;
		if(!(mode&0x80)) return 0;

		if(!(mode&0x20) && tsc_data) {
			if( tsc_data[SC_TRICKDEAD].timer != -1) return 0;
			if( tsc_data[SC_HIGHJUMP].timer  != -1) return 0;
			if( tsc_data[SC_WINKCHARM].timer != -1) return 0;
		}
		if(!(mode&0x20) && target_sd && race!=4 && race!=6 ) {
			if ( pc_ishiding(target_sd)            ) return 0;
			if ( target_sd->state.gangsterparadise ) return 0;
		}
	}

	if( src_sd ) {
		// �ُ�ȂǂōU���ł��Ȃ�
		if(src_sd->opt1>0 || src_sd->status.option&2 || pc_ischasewalk(src_sd) ) {
			return 0;
		}
	}
	if( unit_isdead(target) ) return 0;
	if( target_sd ) {
		if( target_sd->invincible_timer != -1) return 0;
		if( pc_isinvisible(target_sd))         return 0;
	}

	if(sc_data && sc_data[SC_AUTOCOUNTER].timer != -1)   return 0;
	if(sc_data && sc_data[SC_BLADESTOP].timer != -1)     return 0;

	if((opt = status_get_option(target)) != NULL && *opt&0x46)
	{
		//�����E������ԂȂ牣���
		if(src_sd && src_sd->race!=4 && src_sd->race!=6)
			return 0;
	}

	if((tsc_data = status_get_sc_data(target)) && tsc_data[SC_TRICKDEAD].timer != -1)
		return 0;

	if(src_ud->skilltimer != -1 && (!src_sd || pc_checkskill(src_sd,SA_FREECAST) <= 0))
		return 0;

	if(!battle_config.sdelay_attack_enable && (!src_sd || pc_checkskill(src_sd,SA_FREECAST) <= 0)) {
		if(DIFF_TICK(tick , src_ud->canact_tick) < 0) {
			if(src_sd) clif_skill_fail(src_sd,1,4,0);
			return 0;
		}
	}

	dist  = unit_distance(src->x,src->y,target->x,target->y);
	range = status_get_range( src );
	if( src_md && status_get_mode( src ) & 1 ) range++;

	if(src_sd && src_sd->status.weapon != 11) range++;
	if( dist > range ){	// �͂��Ȃ��̂ňړ�
		if(!unit_can_reach(src,target->x,target->y))
			return 0;
        if(src_sd) clif_movetoattack(src_sd,target);
		return 1;
	}

	if(dist <= range && !battle_check_range(src,target,range) ) {
		if(unit_can_reach(src,target->x,target->y) && (sc_data && sc_data[SC_ANKLE].timer == -1)) 
			unit_walktoxy(src,target->x,target->y);
		src_ud->attackabletime = tick + status_get_adelay(src);
	}
	else {
		// �����ݒ�
		int dir = map_calc_dir(src, target->x,target->y );
		if(src_sd && battle_config.pc_attack_direction_change)
			src_sd->dir = src_sd->head_dir = dir;
		if(src_pd && battle_config.monster_attack_direction_change)
			src_pd->dir = dir;
		if(src_md && battle_config.monster_attack_direction_change)
			src_md->dir = dir;

		if(src_ud->walktimer != -1)
			unit_stop_walking(src,1);

		if( src_md && mobskill_use(src_md,tick,-2) ) {	// �X�L���g�p
			return 1;
		}

		if(!sc_data || (sc_data[SC_COMBO].timer == -1 && sc_data[SC_TKCOMBO].timer == -1)) {
			map_freeblock_lock();
			unit_stop_walking(src,1);

			src_ud->attacktarget_lv = battle_weapon_attack(src,target,tick,0);

			if(src_md && !(battle_config.monster_cloak_check_type&2) && sc_data[SC_CLOAKING].timer != -1)
				status_change_end(src,SC_CLOAKING,-1);
			if(src_sd && !(battle_config.pc_cloak_check_type&2) && sc_data[SC_CLOAKING].timer != -1)
				status_change_end(src,SC_CLOAKING,-1);
			if(src_sd && src_sd->status.pet_id > 0 && src_sd->pd && src_sd->petDB)
				pet_target_check(src_sd,target,0);
			map_freeblock_unlock();
			if(src_ud->skilltimer != -1 && src_sd && (skill = pc_checkskill(src_sd,SA_FREECAST)) > 0 ) // �t���[�L���X�g
				src_ud->attackabletime = tick + (status_get_adelay(src)*(150 - skill*5)/100);
			else
				src_ud->attackabletime = tick + status_get_adelay(src);
		}
		else if(src_ud->attackabletime <= tick) {
			if(src_ud->skilltimer != -1 && src_sd && (skill = pc_checkskill(src_sd,SA_FREECAST)) > 0 ) // �t���[�L���X�g
				src_ud->attackabletime = tick + (status_get_adelay(src)*(150 - skill*5)/100);
			else
				src_ud->attackabletime = tick + status_get_adelay(src);
		}
		if(src_ud->attackabletime <= tick)
			src_ud->attackabletime = tick + (battle_config.max_aspd<<1);
	}

	if(src_ud->state.attack_continue) {
		src_ud->attacktimer = add_timer(src_ud->attackabletime,unit_attack_timer,src->id,0);
	}
	return 1;
}

int unit_attack_timer(int tid,unsigned int tick,int id,int data) {
	if(unit_attack_timer_sub(tid, tick, id, data) == 0) {
		unit_unattackable( map_id2bl( id ) );
	}
	return 0;
}

/*==========================================
 * �X�L���r���L�����Z��
 *------------------------------------------
 */
int unit_skillcastcancel(struct block_list *bl,int type)
{
	int inf;
	int ret=0;
	struct map_session_data *sd = NULL;
	struct mob_data         *md = NULL;
	struct unit_data        *ud = unit_bl2ud( bl);
	unsigned long tick=gettick();
	nullpo_retr(0, bl);

	BL_CAST(BL_PC,  bl, sd);
	BL_CAST(BL_MOB, bl, md);
	
	ud->canact_tick=tick;
	ud->canmove_tick = tick;
	if( ud->skilltimer!=-1 ) {
		if( sd && pc_checkskill(sd,SA_FREECAST) > 0) {
			sd->speed = sd->prev_speed;
			clif_updatestatus(sd,SP_SPEED);
		}
		if(!type || !sd) {
			if((inf = skill_get_inf( ud->skillid )) == 2 || inf == 32)
				ret=delete_timer( ud->skilltimer, skill_castend_pos );
			else
				ret=delete_timer( ud->skilltimer, skill_castend_id );
			if(ret<0)
				printf("delete timer error : skillid : %d\n",ud->skillid);
		}
		else {
			if((inf = skill_get_inf( sd->skillid_old )) == 2 || inf == 32)
				ret=delete_timer( ud->skilltimer, skill_castend_pos );
			else
				ret=delete_timer( ud->skilltimer, skill_castend_id );
			if(ret<0)
				printf("delete timer error : skillid : %d\n",sd->skillid_old);
		}
		if( md ) {
			md->skillidx  = -1;
		}

		ud->skilltimer = -1;
		clif_skillcastcancel(bl);
	}
	return 1;
}

// unit_data �̏���������
int unit_dataset(struct block_list *bl) {
	struct unit_data *ud;
	nullpo_retr(0, ud = unit_bl2ud(bl));

	memset( ud, 0, sizeof( struct unit_data) );
	ud->bl             = bl;
	ud->walktimer      = -1;
	ud->skilltimer     = -1;
	ud->attacktimer    = -1;
	ud->attackabletime = gettick();
	ud->canact_tick    = gettick();
	ud->canmove_tick   = gettick();

	return 0;
}

/*==========================================
 * ���������b�N���Ă��郆�j�b�g�̐��𐔂���(foreachclient)
 *------------------------------------------
 */
static int unit_counttargeted_sub(struct block_list *bl, va_list ap)
{
	int id, *c, target_lv;
	nullpo_retr(0, bl);

	id        = va_arg(ap,int);
	c         = va_arg(ap,int *);
	target_lv = va_arg(ap,int);
	if(bl->id == id) {
		// ����
	} else if(bl->type == BL_PC) {
		struct map_session_data *sd=(struct map_session_data *)bl;
		if( sd && sd->ud.attacktarget == id && sd->ud.attacktimer != -1 && sd->ud.attacktarget_lv >= target_lv)
			(*c)++;
	} else if(bl->type == BL_MOB) {
		struct mob_data *md = (struct mob_data *)bl;
		if(md && md->target_id == id && md->ud.attacktarget_lv >= target_lv)
			(*c)++;
	} else if(bl->type == BL_PET) {
		struct pet_data *pd = (struct pet_data *)bl;
		if(pd->target_id == id && pd->ud.attacktimer != -1 && pd->ud.attacktimer != -1 && pd->ud.attacktarget_lv >= target_lv)
			(*c)++;
	}
	return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */

int unit_heal(struct block_list *bl,int hp,int sp)
{
	nullpo_retr(0, bl);
	if(bl->type == BL_PC)
	{
		pc_heal((struct map_session_data*)bl,hp,sp);
	}else if(bl->type == BL_MOB)
	{
		mob_heal((struct mob_data*)bl,hp);
	}
	return 0;
}

/*==========================================
 * 
 *------------------------------------------
 */
int unit_fixdamage(struct block_list *src,struct block_list *target,unsigned int tick,int sdelay,int ddelay,int damage,int div,int type,int damage2)
{

	nullpo_retr(0, target);
	
	if(damage+damage2 <= 0)
		return 0;
	//�^�Q
	if(target->type==BL_MOB){
		mob_attacktarget((struct mob_data*)target,src,0);
	}
	clif_damage(target,target,tick,sdelay,ddelay,damage,div,type,damage2);
	battle_damage(src,target,damage+damage2,0);
	return 0;
}
/*==========================================
 * ���������b�N���Ă���Ώۂ̐���Ԃ�
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int unit_counttargeted(struct block_list *bl,int target_lv)
{
	int c = 0;
	nullpo_retr(0, bl);
	map_foreachinarea(unit_counttargeted_sub, bl->m,
		bl->x-AREA_SIZE,bl->y-AREA_SIZE,
		bl->x+AREA_SIZE,bl->y+AREA_SIZE,0,bl->id,&c,target_lv
	);
	return c;
}

int unit_isdead(struct block_list *bl) {
	nullpo_retr(1, bl);
	if(bl->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data*)bl;
		return (sd->state.dead_sit == 1);
	} else if(bl->type == BL_MOB) {
		struct mob_data *md = (struct mob_data *)bl;
		return md->hp<=0;
	} else if(bl->type == BL_PET) {
		return 0;
	} else {
		return 0;
	}
}


/*==========================================
 * id���U�����Ă���PC�̍U�����~
 * clif_foreachclient��callback�֐�
 *------------------------------------------
 */
int unit_mobstopattacked(struct map_session_data *sd,va_list ap)
{
	int id;

	nullpo_retr(0, sd);
	nullpo_retr(0, ap);

	id=va_arg(ap,int);
	if(sd->ud.attacktarget==id)
		unit_stopattack(&sd->bl);
	return 0;
}

/*==========================================
 * �}�b�v���痣�E����
 *------------------------------------------
 */

int unit_remove_map(struct block_list *bl, int clrtype) {
	struct unit_data *ud = unit_bl2ud( bl );
	nullpo_retr(0, ud);

	if(bl->prev == NULL) {
		printf("unit_remove_map: nullpo bl->prev\n");
		return 1;
	}
	unit_stop_walking(bl,1);			// ���s���f
	unit_stopattack(bl);				// �U�����f
	unit_skillcastcancel(bl,0);			// �r�����f
	skill_unit_move(bl,gettick(),0);	// �X�L�����j�b�g���痣�E

	// tickset �폜
	linkdb_final( &ud->skilltickset );

	if(bl->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data*)bl;
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
		pc_delinvincibletimer(sd);		// ���G�^�C�}�[�폜
		// �u���[�h�X�g�b�v���I��点��
		if(sd->sc_data[SC_BLADESTOP].timer!=-1)
			status_change_end(&sd->bl,SC_BLADESTOP,-1);
		// �o�V���J�폜
		if (sd->sc_data[SC_BASILICA].timer!=-1) {
			skill_basilica_cancel( &sd->bl );
			status_change_end(&sd->bl,SC_BASILICA,-1);
		}
		skill_gangsterparadise(sd,0);			// �M�����O�X�^�[�p���_�C�X�폜
		skill_cleartimerskill(&sd->bl);			// �^�C�}�[�X�L���N���A
		skill_clear_unitgroup(&sd->bl);			// �X�L�����j�b�g�O���[�v�̍폜

		clif_clearchar_area(&sd->bl,clrtype&0xffff);
		map_delblock(&sd->bl);
	} else if(bl->type == BL_MOB) {
		struct mob_data *md = (struct mob_data*)bl;

//		mobskill_deltimer(md);
		// �o�V���J�폜
		if (md->sc_data[SC_BASILICA].timer!=-1) {
			skill_basilica_cancel( &md->bl );
			status_change_end(&md->bl,SC_BASILICA,-1);
		}
		md->state.skillstate=MSS_DEAD;
		md->last_deadtime=gettick();
		// ���񂾂̂ł���mob�ւ̍U���ґS���̍U�����~�߂�
		clif_foreachclient(unit_mobstopattacked,md->bl.id);
		status_change_clear(&md->bl,2);	// �X�e�[�^�X�ُ����������
		skill_clear_unitgroup(&md->bl);	// �S�ẴX�L�����j�b�g�O���[�v���폜����
		skill_cleartimerskill(&md->bl);
		if(md->deletetimer!=-1)
			delete_timer(md->deletetimer,mob_timer_delete);
		md->deletetimer=-1;
		md->hp=md->target_id=md->attacked_id=0;

		if(mob_get_viewclass(md->class) <= 1000) {
			clif_clearchar_area(&md->bl,clrtype);
			clif_clearchar_delay(gettick()+3000,&md->bl,0);
		} else {
			clif_clearchar_area(&md->bl,clrtype);
		}
		if(!battle_config.monster_damage_delay || battle_config.monster_damage_delay_rate == 0)
			mob_deleteslave(md);

		map_delblock(&md->bl);

		// �������Ȃ�MOB�̏���
		if(md->spawndelay1==-1 && md->spawndelay2==-1 && md->n==0){
			map_deliddb(&md->bl);
			if(md->lootitem) {
				map_freeblock(md->lootitem);
				md->lootitem=NULL;
			}
			map_freeblock(md);	// free�̂����
		} else {
			unsigned int spawntime,spawntime1,spawntime2,spawntime3;
			spawntime1=md->last_spawntime+md->spawndelay1;
			spawntime2=md->last_deadtime+md->spawndelay2;
			spawntime3=gettick()+5000;
			// spawntime = max(spawntime1,spawntime2,spawntime3);
			if(DIFF_TICK(spawntime1,spawntime2)>0){
				spawntime=spawntime1;
			} else {
				spawntime=spawntime2;
			}
			if(DIFF_TICK(spawntime3,spawntime)>0){
				spawntime=spawntime3;
			}

			add_timer(spawntime,mob_delayspawn,bl->id,0);
		}
	} else if(bl->type == BL_PET) {
		struct pet_data *pd         = (struct pet_data*)bl;
		struct map_session_data *sd = pd->msd;
		nullpo_retr(0, sd);
		if(sd->pet_hungry_timer != -1)
			pet_hungry_timer_delete(sd);
		clif_clearchar_area(&pd->bl,0);
		if (pd->a_skill)
		{
			aFree(pd->a_skill);
			pd->a_skill = NULL;
		}
		if (pd->s_skill)
		{
			if (pd->s_skill->timer != -1)
				delete_timer(sd->pd->s_skill->timer, pet_skill_support_timer);
			aFree(pd->s_skill);
			pd->s_skill = NULL;
		}
		map_delblock(&pd->bl);
		map_deliddb(&pd->bl);
		free(pd->lootitem);
		map_freeblock(pd);

		sd->pd = NULL;
	}
	return 0;
}

int do_init_unit(void) {
	add_timer_func_list(unit_attack_timer,  "unit_attack_timer");
	add_timer_func_list(unit_walktoxy_timer,"unit_walktoxy_timer");
	return 0;
}

int do_final_unit(void) {
	// nothing to do
	return 0;
}

