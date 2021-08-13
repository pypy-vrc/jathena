#ifndef _CONVERTER_H_

#include "../common/socket.h"
#include <mysql.h>

extern MYSQL mysql_handle;

extern int  db_server_port;
extern char db_server_ip[16];
extern char db_server_id[32];
extern char db_server_pw[32];
extern char db_server_logindb[32];

extern char login_db[1024];
extern char login_db_account_id[1024];
extern char login_db_userid[1024];
extern char login_db_user_pass[1024];
extern char login_db_level[1024];

extern char tmp_sql[65535];

extern char GM_account_filename[1024];
extern char pet_txt[1024];
extern char storage_txt[1024];
extern char party_txt[1024];
extern char guild_txt[1024];
extern char guild_storage_txt[1024];

char* strecpy(char* pt, const char* spt);

#endif

