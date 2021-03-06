#ifndef _STORAGE_H_
#define _STORAGE_H_

#include "mmo.h"

int storage_storageopen(struct map_session_data *sd);
void storage_storageadd(struct map_session_data *sd, int idx, int amount);
void storage_storageget(struct map_session_data *sd, int idx, int amount);
void storage_storageaddfromcart(struct map_session_data *sd, int idx, int amount);
void storage_storagegettocart(struct map_session_data *sd, int idx, int amount);
void storage_storageclose(struct map_session_data *sd);
void do_init_storage(void);
void do_final_storage(void);
struct storage *account2storage(int account_id);
void storage_delete(int account_id);
void storage_storage_quit(struct map_session_data *sd);
void storage_storage_save(struct map_session_data *sd);

struct guild_storage *guild2storage(int guild_id);
void guild_storage_delete(int guild_id);
int storage_guild_storageopen(struct map_session_data *sd);
int guild_storage_additem(struct map_session_data *sd,struct guild_storage *stor,struct item *item_data,int amount);
void storage_guild_storageadd(struct map_session_data *sd, int idx, int amount);
void storage_guild_storageget(struct map_session_data *sd, int idx, int amount);
void storage_guild_storageaddfromcart(struct map_session_data *sd, int idx, int amount);
void storage_guild_storagegettocart(struct map_session_data *sd, int idx, int amount);
void storage_guild_storageclose(struct map_session_data *sd);
void storage_guild_storage_quit(struct map_session_data *sd, char flag);
void storage_guild_storagesave(struct map_session_data *sd);

#endif
