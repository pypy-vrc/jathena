#ifndef _CHRIF_H_
#define _CHRIF_H_

void chrif_setuserid(char*);
void chrif_setpasswd(char*);
void chrif_setip(char*);
void chrif_setport(int);
int chrif_parse(int fd);

int chrif_isconnect(void);

int chrif_authreq(struct map_session_data *);
int chrif_save(struct map_session_data*);
int chrif_charselectreq(struct map_session_data *);

int chrif_changemapserver(struct map_session_data *sd,char *name,int x,int y,int ip,short port);

void chrif_searchcharid(int char_id);
int chrif_changegm(int id,const char *pass,int len);
int chrif_changesex(int id,int sex);
int chrif_saveaccountreg2(struct map_session_data *sd);

int chrif_mapactive(int active);
int chrif_maintenance(int maintenance);
int chrif_chardisconnect(struct map_session_data *sd);

int chrif_reqdivorce(int char_id);

int chrif_friend_delete( struct map_session_data* sd, int account_id, int char_id );
int chrif_friend_online( struct map_session_data *sd, int flag );

int do_final_chrif(void);
int do_init_chrif(void);

#endif
