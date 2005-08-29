
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mmo.h"
#include "converter.h"
#include "login-converter.h"
#include "char-converter.h"

MYSQL mysql_handle;

int  db_server_port = 3306;
char db_server_ip[16] = "127.0.0.1";
char db_server_id[32] = "ragnarok";
char db_server_pw[32] = "ragnarok";
char db_server_logindb[32] = "ragnarok";
char db_server_charset[32] = "";

char login_db[256]       = "login";
char login_account_id[256] = "account_id";
char login_userid[256]     = "userid";
char login_user_pass[256]  = "user_pass";
char login_level[256]      = "level";

char pet_txt[256]          = "save/pet.txt";
char storage_txt[256]      = "save/storage.txt";
char party_txt[1024]       = "save/party.txt";
char guild_txt[1024]       = "save/guild.txt";
char guild_storage_txt[1024]="save/g_storage.txt";
char tmp_sql[65535];

#ifdef _MSC_VER
#pragma comment(lib,"libmysql.lib")
#endif

char* strecpy (char* pt,const char* spt) {
	//copy from here
	mysql_real_escape_string(&mysql_handle,pt,spt,strlen(spt));
	return pt;
}

int config_read(const char *cfgName) {
	int i;
	char line[1024], w1[1024], w2[1024];
	FILE *fp;

	printf ("Start reading converter configuration: %s\n",cfgName);

	fp=fopen(cfgName,"r");
	if(fp==NULL){
		printf("File not found: %s\n", cfgName);
		return 1;
	}
	while(fgets(line, 1020, fp)){
		i=sscanf(line,"%[^:]:%s", w1, w2);
		if(i!=2)
			continue;
		if(strcmpi(w1,"storage_txt")==0){
			printf ("set storage_txt : %s\n",w2);
			strncpy(storage_txt, w2, sizeof(storage_txt));
		} else if(strcmpi(w1,"pet_txt")==0){
			printf ("set pet_txt : %s\n",w2);
			strncpy(pet_txt, w2, sizeof(pet_txt));
		}
		//add for DB connection
		else if(strcmpi(w1,"db_server_ip")==0){
			strcpy(db_server_ip, w2);
			printf ("set db_server_ip : %s\n",w2);
		}
		else if(strcmpi(w1,"db_server_port")==0){
			db_server_port=atoi(w2);
			printf ("set db_server_port : %s\n",w2);
		}
		else if(strcmpi(w1,"db_server_id")==0){
			strcpy(db_server_id, w2);
			printf ("set db_server_id : %s\n",w2);
		}
		else if(strcmpi(w1,"db_server_pw")==0){
			strcpy(db_server_pw, w2);
			printf ("set db_server_pw : %s\n",w2);
		}
		else if(strcmpi(w1,"db_server_charset")==0){
			strcpy(db_server_charset, w2);
			printf ("set db_server_charset : %s\n",w2);
		}
		else if(strcmpi(w1,"db_server_logindb")==0){
			strcpy(db_server_logindb, w2);
			printf ("set db_server_logindb : %s\n",w2);
			//support the import command, just like any other config
		}else if(strcmpi(w1,"import")==0){
			config_read(w2);
		}
	}
	fclose(fp);

	printf("Reading converter configuration: Done\n");

	return 0;
}

int do_init(int argc,char **argv) {
	// read config
	config_read("conf/converter_athena.conf");

	// DB connection initialized
	mysql_init(&mysql_handle);
	printf("Connect DB server");
	if(db_server_charset[0]) {
		printf(" (charset: %s)",db_server_charset);
	}
	printf("...\n");
	if(!mysql_real_connect(&mysql_handle, db_server_ip, db_server_id, db_server_pw,
		db_server_logindb ,db_server_port, (char *)NULL, 0)) {
			//pointer check
		printf("%s\n",mysql_error(&mysql_handle));
		exit(1);
	}
	else {
		printf ("connect success! (inter server)\n");
	}
	if(db_server_charset[0]) {
		sprintf(tmp_sql,"SET NAMES %s",db_server_charset);
		if (mysql_query(&mysql_handle, tmp_sql)) {
			printf("DB server Error (charset)- %s\n", mysql_error(&mysql_handle));
		}
	}

	printf("Warning : Make sure you backup your databases before continuing!\n");
	printf ("Convert start...\n");

	login_convert();
	char_convert();

	printf ("Everything's been converted!\n");
	exit (0);

	return 0;
}

void do_final(void) {
	// nothing to do
}

