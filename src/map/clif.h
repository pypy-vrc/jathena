#ifndef _CLIF_H_
#define _CLIF_H_

#ifndef _WIN32
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#else
	#include <winsock.h>
 	#define in_addr_t unsigned long
#endif
#include "map.h"

#define MAX_PACKET_DB			0x260

struct packet_db {
	short len;
	void (*func)(int fd,struct map_session_data *sd, int cmd);
	short pos[20];
};

void clif_setip(char*);
void clif_setport(int);

in_addr_t clif_getip(void);
int clif_getport(void);
int clif_countusers(void);
void clif_setwaitclose(int);
int clif_parse(int fd);

int clif_authok(struct map_session_data *);
int clif_authfail_fd(int,int);
int clif_charselectok(int);
int clif_dropflooritem(struct flooritem_data *);
int clif_clearflooritem(struct flooritem_data *,int);
int clif_clearchar(struct block_list*,int);	// area or fd
int clif_clearchar_delay(unsigned int,struct block_list *,int);
#define clif_clearchar_area(bl,type) clif_clearchar(bl,type)
int clif_clearchar_id(int,int,int);
int clif_spawnpc(struct map_session_data*);	//area
int clif_spawnnpc(struct npc_data*);	// area
int clif_spawnmob(struct mob_data*);	// area
int clif_spawnpet(struct pet_data*);	// area
int clif_walkok(struct map_session_data*);	// self
int clif_movechar(struct map_session_data*);	// area
int clif_movemob(struct mob_data*);	//area
int clif_movepet(struct pet_data *pd);	//area
int clif_changemap(struct map_session_data*,char*,int,int);	//self
int clif_changemapserver(struct map_session_data*,char*,int,int,int,int);	//self
int clif_fixpos(struct block_list *bl);	// area
int clif_fixpos2(struct block_list *bl, int x[4], int y[4]);	// custom
int clif_fixwalkpos(struct block_list *bl);	// area
int clif_npcbuysell(struct map_session_data*,int);	//self
int clif_buylist(struct map_session_data*,struct npc_data*);	//self
int clif_selllist(struct map_session_data*);	//self
int clif_scriptmes(struct map_session_data*,int,char*);	//self
int clif_scriptnext(struct map_session_data*,int);	//self
int clif_scriptclose(struct map_session_data*,int);	//self
int clif_scriptmenu(struct map_session_data*,int,char*);	//self
int clif_scriptinput(struct map_session_data*,int);	//self
int clif_scriptinputstr(struct map_session_data *sd,int npcid);	// self
int clif_cutin(struct map_session_data*,char*,int);	//self
int clif_viewpoint(struct map_session_data*,int,int,int,int,int,int);	//self
int clif_additem(struct map_session_data*,int,int,int);	//self
int clif_delitem(struct map_session_data*,int,int);	//self
int clif_updatestatus(struct map_session_data*,int);	//self
int clif_changestatus(struct block_list*,int,int);	//area
int clif_damage(struct block_list *,struct block_list *,unsigned int,int,int,int,int,int,int);	// area
#define clif_takeitem(src,dst) clif_damage(src,dst,0,0,0,0,0,1,0)
int clif_changelook(struct block_list *,int,int);	// area
int clif_arrowequip(struct map_session_data *sd,int val); //self
int clif_arrow_fail(struct map_session_data *sd,int type); //self
int clif_arrow_create_list(struct map_session_data *sd);	//self
int clif_statusupack(struct map_session_data *,int,int,int);	// self
int clif_equipitemack(struct map_session_data *,int,int,int);	// self
int clif_unequipitemack(struct map_session_data *,int,int,int);	// self
int clif_misceffect(struct block_list*,int);	// area
int clif_misceffect2(struct block_list *bl,int type);	// area
int clif_misceffect3(struct block_list *bl,int type);	// self
int clif_changeoption(struct block_list*);	// area
int clif_useitemack(struct map_session_data*,int,int,int);	// self
void clif_GlobalMessage(struct block_list *bl,char *message);
int clif_createchat(struct map_session_data*,int);	// self
int clif_dispchat(struct chat_data*,int);	// area or fd
int clif_joinchatfail(struct map_session_data*,int);	// self
int clif_joinchatok(struct map_session_data*,struct chat_data*);	// self
int clif_addchat(struct chat_data*,struct map_session_data*);	// chat
int clif_changechatowner(struct chat_data*,struct map_session_data*);	// chat
int clif_clearchat(struct chat_data*,int);	// area or fd
int clif_leavechat(struct chat_data*,struct map_session_data*);	// chat
int clif_changechatstatus(struct chat_data*);	// chat

int clif_baby_target_display(struct map_session_data *sd);//
int clif_refine_select(struct map_session_data *sd);
void clif_emotion(struct block_list *bl,int type);
void clif_talkiebox(struct block_list *bl,char* talkie);
void clif_wedding_effect(struct block_list *bl);
void clif_callpartner(struct map_session_data *sd);
void clif_sitting(struct map_session_data *sd);
void clif_soundeffect(struct map_session_data *sd,struct block_list *bl,char *name,int type);

int clif_getareachar(struct block_list* bl,va_list ap);
// trade
int clif_traderequest(struct map_session_data *sd,char *name);
int clif_tradestart(struct map_session_data *sd,int type);
int clif_tradeadditem(struct map_session_data *sd,struct map_session_data *tsd,int index,int amount);
int clif_tradeitemok(struct map_session_data *sd,int index,int fail);
int clif_tradedeal_lock(struct map_session_data *sd,int fail);
int clif_tradecancelled(struct map_session_data *sd);
int clif_tradecompleted(struct map_session_data *sd,int fail);

// storage
#include "storage.h"
int clif_storageitemlist(struct map_session_data *sd,struct storage *stor);
int clif_storageequiplist(struct map_session_data *sd,struct storage *stor);
int clif_updatestorageamount(struct map_session_data *sd,struct storage *stor);
int clif_storageitemadded(struct map_session_data *sd,struct storage *stor,int index,int amount);
int clif_storageitemremoved(struct map_session_data *sd,int index,int amount);
int clif_storageclose(struct map_session_data *sd);
int clif_guildstorageitemlist(struct map_session_data *sd,struct guild_storage *stor);
int clif_guildstorageequiplist(struct map_session_data *sd,struct guild_storage *stor);
int clif_updateguildstorageamount(struct map_session_data *sd,struct guild_storage *stor);
int clif_guildstorageitemadded(struct map_session_data *sd,struct guild_storage *stor,int index,int amount);

int clif_pcinsight(struct block_list *,va_list);	// map_forallinmovearea callback
int clif_pcoutsight(struct block_list *,va_list);	// map_forallinmovearea callback
int clif_mobinsight(struct block_list *,va_list);	// map_forallinmovearea callback
int clif_moboutsight(struct block_list *,va_list);	// map_forallinmovearea callback
int clif_petoutsight(struct block_list *bl,va_list ap);
int clif_petinsight(struct block_list *bl,va_list ap);

int clif_class_change(struct block_list *bl,int class,int type);

int clif_skillinfo(struct map_session_data *sd,int skillid,int type,int range);
int clif_skillinfoblock(struct map_session_data *sd);
int clif_skillup(struct map_session_data *sd,int skill_num);

int clif_skillcasting(struct block_list* bl,
	int src_id,int dst_id,int dst_x,int dst_y,int skill_num,int casttime);
int clif_skillcastcancel(struct block_list* bl);
int clif_skill_fail(struct map_session_data *sd,int skill_id,int type,int btype);
int clif_skill_damage(struct block_list *src,struct block_list *dst,
	unsigned int tick,int sdelay,int ddelay,int damage,int div,
	int skill_id,int skill_lv,int type);
int clif_skill_damage2(struct block_list *src,struct block_list *dst,
	unsigned int tick,int sdelay,int ddelay,int damage,int div,
	int skill_id,int skill_lv,int type);
int clif_skill_nodamage(struct block_list *src,struct block_list *dst,
	int skill_id,int heal,int fail);
int clif_skill_poseffect(struct block_list *src,int skill_id,
	int val,int x,int y,int tick);
int clif_skill_estimation(struct map_session_data *sd,struct block_list *dst);
int clif_skill_warppoint(struct map_session_data *sd,int skill_num,
	const char *map1,const char *map2,const char *map3,const char *map4);
int clif_skill_memo(struct map_session_data *sd,int flag);
int clif_skill_teleportmessage(struct map_session_data *sd,int flag);
int clif_skill_produce_mix_list(struct map_session_data *sd,int trigger);

int clif_produceeffect(struct map_session_data *sd,int flag,int nameid);

int clif_skill_setunit(struct skill_unit *unit);
int clif_skill_delunit(struct skill_unit *unit);

int clif_01ac(struct block_list *bl);

int clif_autospell(struct map_session_data *sd,int skilllv);
int clif_devotion(struct map_session_data *sd,int target);
int clif_marionette(struct map_session_data *sd,int target);

int clif_spiritball(struct map_session_data *sd);
int clif_combo_delay(struct block_list *src,int wait);
int clif_bladestop(struct block_list *src,struct block_list *dst,int bool);
int clif_changemapcell(int m,int x,int y,int cell_type,int type);

int clif_status_change(struct block_list *bl,int type,int flag);

int clif_wis_message(int fd,char *nick,char *mes,int mes_len);
int clif_wis_end(int fd,int flag);

int clif_solved_charname(struct map_session_data *sd,int char_id);

int clif_use_card(struct map_session_data *sd,int idx);
int clif_insert_card(struct map_session_data *sd,int idx_equip,int idx_card,int flag);

int clif_itemlist(struct map_session_data *sd);
int clif_equiplist(struct map_session_data *sd);

int clif_cart_additem(struct map_session_data*,int,int,int);
int clif_cart_delitem(struct map_session_data*,int,int);
int clif_cart_itemlist(struct map_session_data *sd);
int clif_cart_equiplist(struct map_session_data *sd);

int clif_item_identify_list(struct map_session_data *sd);
int clif_weapon_refine_list(struct map_session_data *sd);
int clif_item_identified(struct map_session_data *sd,int idx,int flag);
int clif_item_repair_list(struct map_session_data *sd,struct map_session_data *dstsd);
int clif_item_repaireffect(struct map_session_data *sd,int flag,int nameid);

int clif_item_skill(struct map_session_data *sd,int skillid,int skilllv,const char *name);

int clif_mvp_effect(struct map_session_data *sd);
int clif_mvp_item(struct map_session_data *sd,int nameid);
int clif_mvp_exp(struct map_session_data *sd,int exp);

// vending
int clif_openvendingreq(struct map_session_data *sd,int num);
int clif_showvendingboard(struct block_list* bl,char *message,int fd);
int clif_closevendingboard(struct block_list* bl,int fd);
int clif_vendinglist(struct map_session_data *sd,int id,struct vending *vending);
int clif_buyvending(struct map_session_data *sd,int index,int amount,int fail);
int clif_openvending(struct map_session_data *sd,int id,struct vending *vending);
int clif_vendingreport(struct map_session_data *sd,int index,int amount);

int clif_movetoattack(struct map_session_data *sd,struct block_list *bl);

// party
int clif_party_created(struct map_session_data *sd,int flag);
int clif_party_info(struct party *p,int fd);
int clif_party_invite(struct map_session_data *sd,struct map_session_data *tsd);
int clif_party_inviteack(struct map_session_data *sd,char *nick,int flag);
int clif_party_option(struct party *p,struct map_session_data *sd,int flag);
int clif_party_leaved(struct party *p,struct map_session_data *sd,int account_id,char *name,int flag);
int clif_party_message(struct party *p,int account_id,char *mes,int len);
int clif_party_move(struct party *p,struct map_session_data *sd,int online);
int clif_party_xy(struct party *p,struct map_session_data *sd);
int clif_party_hp(struct party *p,struct map_session_data *sd);

int clif_hpmeter(struct map_session_data *sd);

// guild
int clif_guild_created(struct map_session_data *sd,int flag);
int clif_guild_belonginfo(struct map_session_data *sd,struct guild *g);
int clif_guild_basicinfo(struct map_session_data *sd);
int clif_guild_allianceinfo(struct map_session_data *sd);
int clif_guild_memberlist(struct map_session_data *sd);
int clif_guild_skillinfo(struct map_session_data *sd);
int clif_guild_memberlogin_notice(struct guild *g,int idx,int flag);
int clif_guild_invite(struct map_session_data *sd,struct guild *g);
int clif_guild_inviteack(struct map_session_data *sd,int flag);
int clif_guild_leave(struct map_session_data *sd,const char *name,const char *mes);
int clif_guild_explusion(struct map_session_data *sd,const char *name,const char *mes,
	int account_id);
int clif_guild_positionchanged(struct guild *g,int idx);
int clif_guild_memberpositionchanged(struct guild *g,int idx);
int clif_guild_emblem(struct map_session_data *sd,struct guild *g);
int clif_guild_notice(struct map_session_data *sd,struct guild *g);
int clif_guild_message(struct guild *g,int account_id,const char *mes,int len);
int clif_guild_skillup(struct map_session_data *sd,int skill_num,int lv);
int clif_guild_reqalliance(struct map_session_data *sd,int account_id,const char *name);
int clif_guild_allianceack(struct map_session_data *sd,int flag);
int clif_guild_delalliance(struct map_session_data *sd,int guild_id,int flag);
int clif_guild_oppositionack(struct map_session_data *sd,int flag);
int clif_guild_broken(struct map_session_data *sd,int flag);


// atcommand
int clif_onlymessage(struct map_session_data *sd,char *mes,int len);
int clif_displaymessage(const int fd,char* mes);
int clif_disp_onlyself(struct map_session_data *sd,char *mes,int len);
int clif_GMmessage(struct block_list *bl,char* mes,int len,int flag);
int clif_heal(int fd,int type,int val);
int clif_resurrection(struct block_list *bl,int type);
int clif_set0199(int fd,int type);
int clif_pvpset(struct map_session_data *sd, int pvprank, int pvpnum,int type);
int clif_send0199(int map,int type);
int clif_refine(int fd,struct map_session_data *sd,int fail,int index,int val);
int clif_send_packet(struct map_session_data *sd,const char *message);

//petsystem
int clif_catch_process(struct map_session_data *sd);
int clif_pet_rulet(struct map_session_data *sd,int data);
int clif_sendegg(struct map_session_data *sd);
int clif_send_petdata(struct map_session_data *sd,int type,int param);
int clif_send_petstatus(struct map_session_data *sd);
int clif_pet_emotion(struct pet_data *pd,int param);
int clif_pet_performance(struct block_list *bl,int param);
int clif_pet_equip(struct pet_data *pd,int nameid);
int clif_pet_food(struct map_session_data *sd,int foodid,int fail);
int clif_pet_hp(struct map_session_data *sd);

//friend
void clif_friend_send_info( struct map_session_data *sd );
void clif_friend_send_online(const int fd, int account_id, int char_id, int flag );
void clif_friend_add_request(const int fd, struct map_session_data *from_sd );
void clif_friend_add_ack(const int fd, int account_id, int char_id, char* name, int flag );
void clif_friend_del_ack(const int fd, int account_id, int char_id );

//ranking
void clif_blacksmith_point(const int fd,const int total,const int point);
void clif_alchemist_point(const int fd,const int total,const int point);
void clif_taekwon_point(const int fd,const int total,const int point);
void clif_pk_point(const int fd,const int total,const int point);
void clif_blacksmith_ranking(const int fd,char *charname[10],const int point[10]);
void clif_alchemist_ranking(const int fd,char *charname[10],const int point[10]);
void clif_taekwon_ranking(const int fd,char *charname[10],const int point[10]);
void clif_pk_ranking(const int fd,char *charname[10],const int point[10]);

int clif_GM_kickack(struct map_session_data *sd,int id);
int clif_GM_kick(struct map_session_data *sd,struct map_session_data *tsd,int type);

int clif_foreachclient(int (*)(struct map_session_data*,va_list),...);

int do_final_clif(void);
int do_init_clif(void);

// httpd chat system

struct httpd_session_data;
void clif_webchat_message(const char* head,const char *mes1,const char *mes2);
void clif_webchat(struct httpd_session_data* sd,const char* url);

#endif


