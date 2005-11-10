#include <stdio.h>
#include <string.h>

#include "clif.h"
#include "itemdb.h"
#include "map.h"
#include "trade.h"
#include "pc.h"
#include "npc.h"
#include "battle.h"
#include "nullpo.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

/*==========================================
 * ����v���𑊎�ɑ���
 *------------------------------------------
 */
void trade_traderequest(struct map_session_data *sd,int target_id)
{
	struct map_session_data *target_sd;

	nullpo_retv(sd);

	if(sd->npc_id)
		npc_event_dequeue(sd);

	if((target_sd = map_id2sd(target_id)) != NULL){
		if(target_sd->vender_id != 0) {
				clif_tradestart(sd,4);
				return;
		}
		if(!battle_config.invite_request_check) {
			if(target_sd->guild_invite>0 || target_sd->party_invite>0){
				clif_tradestart(sd,2);	// �����PT�v������Guild�v����
				return;
			}
		}
		if(target_sd->trade_partner !=0) {
			clif_tradestart(sd,2); //person is in another trade
		}
		else{
			if(sd->bl.m != target_sd->bl.m
			 || (sd->bl.x - target_sd->bl.x < -2 || sd->bl.x - target_sd->bl.x > 2)
			 || (sd->bl.y - target_sd->bl.y < -2 || sd->bl.y - target_sd->bl.y > 2)) {
				clif_tradestart(sd,0); //too far
			}else{
				target_sd->trade_partner = sd->status.account_id;
				sd->trade_partner = target_sd->status.account_id;
				clif_traderequest(target_sd,sd->status.name);
			}
		}
	}
	else{
		clif_tradestart(sd,1); //character does not exist
	}
}

/*==========================================
 * ����v��
 *------------------------------------------
 */
void trade_tradeack(struct map_session_data *sd,int type)
{
	struct map_session_data *target_sd;

	nullpo_retv(sd);

	if(sd->npc_id != 0)
		npc_event_dequeue(sd);

	if((target_sd = map_id2sd(sd->trade_partner)) != NULL){
		if(sd->bl.m != target_sd->bl.m
		 || (sd->bl.x - target_sd->bl.x < -2 || sd->bl.x - target_sd->bl.x > 2)
		 || (sd->bl.y - target_sd->bl.y < -2 || sd->bl.y - target_sd->bl.y > 2)) {
			trade_tradecancel(sd);
			return;
		}
		clif_tradestart(target_sd,type);
		clif_tradestart(sd,type);
		if(type == 4){ // Cancel
			sd->deal_locked=0;
			sd->deal_mode=0;
			sd->trade_partner=0;
			target_sd->deal_locked=0;
			target_sd->deal_mode=0;
			target_sd->trade_partner=0;
		}else{
			sd->deal_mode=1;
			target_sd->deal_mode=1;
		}
	}
}

/*==========================================
 * �A�C�e���ǉ�
 *------------------------------------------
 */
void trade_tradeadditem(struct map_session_data *sd,int index,int amount)
{
	struct map_session_data *target_sd;

	int trade_i;
	int trade_weight=0;

	nullpo_retv(sd);

	if(((target_sd = map_id2sd(sd->trade_partner)) != NULL) && (sd->deal_locked < 1)){
		if(index<2 || index>=MAX_INVENTORY+2){
			if(index == 0 && amount > 0 && amount <= sd->status.zeny){
				if(
					target_sd->status.zeny + amount <= MAX_ZENY &&
					target_sd->status.zeny + amount >= 0 // overflow �΍�
				) {
					sd->deal_zeny=amount;
					clif_tradeadditem(sd,target_sd,0,amount);
				} else {
					clif_tradeitemok(sd,index,1); //fail to add item
				}
			}
		}else if(amount <= sd->status.inventory[index-2].amount && amount > 0 && itemdb_isdropable(sd->status.inventory[index-2].nameid)){
			for(trade_i=0; trade_i<10;trade_i++){
				if(sd->deal_item_amount[trade_i] == 0){
					trade_weight+=sd->inventory_data[index-2]->weight*amount;
					if(target_sd->weight + trade_weight > target_sd->max_weight){
						clif_tradeitemok(sd,index,1); //fail to add item -- the player was over weighted.
					}else{
						sd->deal_item_index[trade_i] =index;
						sd->deal_item_amount[trade_i]=amount;
						clif_tradeitemok(sd,index,0); //success to add item
						clif_tradeadditem(sd,target_sd,index,amount);
					}
					break;
				}else{
					trade_weight+=sd->inventory_data[sd->deal_item_index[trade_i]-2]->weight*sd->deal_item_amount[trade_i];
				}
			}
		}
	}
}

/*==========================================
 * �A�C�e���ǉ�����(ok����)
 *------------------------------------------
 */
void trade_tradeok(struct map_session_data *sd)
{
	struct map_session_data *target_sd;

	nullpo_retv(sd);

	if((target_sd = map_id2sd(sd->trade_partner)) != NULL){
		sd->deal_locked=1;
		clif_tradeitemok(sd,0,0);
		clif_tradedeal_lock(sd,0);
		clif_tradedeal_lock(target_sd,1);
	}
}

/*==========================================
 * ����L�����Z��
 *------------------------------------------
 */
void trade_tradecancel(struct map_session_data *sd)
{
	struct map_session_data *target_sd;
	int trade_i;

	nullpo_retv(sd);

	if((target_sd = map_id2sd(sd->trade_partner)) != NULL){
		for(trade_i=0; trade_i<10;trade_i++) { //give items back (only virtual)
			if(sd->deal_item_amount[trade_i] != 0) {
				clif_additem(sd,sd->deal_item_index[trade_i]-2,sd->deal_item_amount[trade_i],0);
				sd->deal_item_index[trade_i] =0;
				sd->deal_item_amount[trade_i]=0;
			}
			if(target_sd->deal_item_amount[trade_i] != 0) {
				clif_additem(target_sd,target_sd->deal_item_index[trade_i]-2,target_sd->deal_item_amount[trade_i],0);
				target_sd->deal_item_index[trade_i] =0;
				target_sd->deal_item_amount[trade_i]=0;
			}
		}
		if(sd->deal_zeny) {
			clif_updatestatus(sd,SP_ZENY);
			sd->deal_zeny=0;
		}
		if(target_sd->deal_zeny) {
			clif_updatestatus(target_sd,SP_ZENY);
			target_sd->deal_zeny=0;
		}
		sd->deal_locked =0;
		sd->deal_mode=0;
		sd->trade_partner=0;
		target_sd->deal_locked=0;
		target_sd->deal_mode=0;
		target_sd->trade_partner=0;
		clif_tradecancelled(sd);
		clif_tradecancelled(target_sd);
	}
}

/*==========================================
 * �������(trade����)
 *------------------------------------------
 */
void trade_tradecommit(struct map_session_data *sd)
{
	struct map_session_data *target_sd;
	int trade_i,n,flag;

	nullpo_retv(sd);

	if((target_sd = map_id2sd(sd->trade_partner)) != NULL){
		if( (sd->deal_locked >=1) && (target_sd->deal_locked >=1) ){ // both have pressed 'ok'
			if(sd->deal_locked < 2) {sd->deal_locked=2;} // set locked to 2
			if(target_sd->deal_locked==2) { // the other one pressed 'trade' too
				for(trade_i=0; trade_i<10;trade_i++) {
					if(sd->deal_item_amount[trade_i] != 0) {
						n = sd->deal_item_index[trade_i]-2;
						if(sd->status.inventory[n].amount < sd->deal_item_amount[trade_i]){
							sd->deal_item_amount[trade_i]=sd->status.inventory[n].amount;
							trade_tradecancel(sd);
							return;
						}
					}
					if(target_sd->deal_item_amount[trade_i] != 0) {
						n = target_sd->deal_item_index[trade_i]-2;
						if(target_sd->status.inventory[n].amount < target_sd->deal_item_amount[trade_i]){
							target_sd->deal_item_amount[trade_i]=target_sd->status.inventory[n].amount;
							trade_tradecancel(sd);
							return;
						}
					}
				}
				if(sd->deal_zeny > sd->status.zeny || 
				target_sd->deal_zeny > target_sd->status.zeny){
					trade_tradecancel(sd);
					return;
				}
				for(trade_i=0; trade_i<10;trade_i++) {
					if(sd->deal_item_amount[trade_i] != 0) {
						n = sd->deal_item_index[trade_i]-2;
						if(itemdb_isdropable(sd->status.inventory[n].nameid)){
							flag = pc_additem(target_sd,&sd->status.inventory[n],sd->deal_item_amount[trade_i]);
							if(flag==0)
								pc_delitem(sd,n,sd->deal_item_amount[trade_i],1);
							else
								clif_additem(sd,n,sd->deal_item_amount[trade_i],0);
						}
						sd->deal_item_index[trade_i] =0;
						sd->deal_item_amount[trade_i]=0;
					}
					if(target_sd->deal_item_amount[trade_i] != 0) {
						n = target_sd->deal_item_index[trade_i]-2;
						if(itemdb_isdropable(target_sd->status.inventory[n].nameid)){
							flag = pc_additem(sd,&target_sd->status.inventory[n],target_sd->deal_item_amount[trade_i]);
							if(flag==0)
								pc_delitem(target_sd,n,target_sd->deal_item_amount[trade_i],1);
							else
								clif_additem(target_sd,n,target_sd->deal_item_amount[trade_i],0);
						}
						target_sd->deal_item_index[trade_i] =0;
						target_sd->deal_item_amount[trade_i]=0;
					}
				}
				if(sd->deal_zeny) {
					sd->status.zeny -= sd->deal_zeny;
					clif_updatestatus(sd,SP_ZENY);
					target_sd->status.zeny += sd->deal_zeny;
					clif_updatestatus(target_sd,SP_ZENY);
					sd->deal_zeny=0;
				}
				if(target_sd->deal_zeny) {
					target_sd->status.zeny -= target_sd->deal_zeny;
					clif_updatestatus(target_sd,SP_ZENY);
					sd->status.zeny += target_sd->deal_zeny;
					clif_updatestatus(sd,SP_ZENY);
					target_sd->deal_zeny=0;
				}
				sd->deal_locked =0;
				sd->deal_mode=0;
				sd->trade_partner=0;
				target_sd->deal_locked=0;
				target_sd->deal_mode=0;
				target_sd->trade_partner=0;
				clif_tradecompleted(sd,0);
				clif_tradecompleted(target_sd,0);
			}
		}
	}
}
