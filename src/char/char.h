#ifndef _CHAR_H_
#define _CHAR_H_

#include "mmo.h"

#define MAX_MAP_SERVERS 30

#define CHAR_CONF_NAME	"conf/char_athena.conf"

#define DEFAULT_AUTOSAVE_INTERVAL_CS 300*1000

struct mmo_map_server {
	long ip;
	short port;
	int users;
	char map[MAX_MAP_PER_SERVER][16];
	short active;
};

struct char_session_data {
	int state;
	int account_id,login_id1,login_id2,sex;
	const struct mmo_charstatus *found_char[9];
};

int mapif_sendall(unsigned char *buf,unsigned int len);
int mapif_sendallwos(int fd,unsigned char *buf,unsigned int len);
int mapif_send(int fd,unsigned char *buf,unsigned int len);
int mapif_parse_CharConnectLimit(int fd);

extern int autosave_interval;
int char_log(char *fmt,...);
extern struct point start_point;
extern int start_zeny;

struct char_online {
	int account_id;
	int char_id;
	int ip;
	int port;
	char name[24];
};


#ifdef TXT_ONLY

const struct mmo_charstatus* char_txt_load(int char_id);
int char_txt_nick2id(const char *char_name);

#ifndef _CHAR_C_
#define char_load char_txt_load
#define char_nick2id char_txt_nick2id
#endif /* _CHAR_C_ */

#else

const struct mmo_charstatus* char_sql_load(int char_id);
int char_sql_nick2id(const char *char_name);

#ifndef _CHAR_C_
#define char_load char_sql_load
#define char_nick2id char_sql_nick2id
#endif /* _CHAR_C_ */

#include "socket.h"
#include <mysql.h>

// for sql
enum {
	TABLE_INVENTORY,
	TABLE_CART,
	TABLE_STORAGE,
	TABLE_GUILD_STORAGE,
};
int char_sql_saveitem(struct item *item, int max, int id, int tableswitch);
int char_sql_loaditem(struct item *item, int max, int id, int tableswitch);

extern MYSQL mysql_handle;
extern char tmp_sql[65535];
extern char char_db[256];
extern char reg_db[256];
char* strecpy (char* pt,const char* spt);

#endif /* TXT_ONLY */

#endif /* _CHAR_H_ */
