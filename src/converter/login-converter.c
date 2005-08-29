// $Id: login-converter.c,v 1.1 2005/08/29 21:39:38 running_pinata Exp $
// original : login2.c 2003/01/28 02:29:17 Rev.1.1.1.1
// login data file to mysql conversion utility.
//
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "converter.h"
#include "../common/core.h"
#include "../login/login.h"
#include "../common/mmo.h"
#include "../common/db.h"

static struct dbt *gm_account_db;

int isGM(int account_id)
{
	struct gm_account *p;
	p = (struct gm_account*)numdb_search(gm_account_db,account_id);
	if( p == NULL)
		return 0;
	return p->level;
}

int read_gm_account()
{
	char line[8192];
	struct gm_account *p;
	FILE *fp;
	int c=0;

	gm_account_db = numdb_init();

	printf("Starting reading gm_account\n");

	if( (fp=fopen("conf/GM_account.txt","r"))==NULL )
		return 1;
	while(fgets(line,sizeof(line),fp)){
		if(line[0] == '/' || line[1] == '/' || line[2] == '/')
			continue;

		p = (struct gm_account*)malloc(sizeof(struct gm_account));
		if(p==NULL){
			printf("gm_account: out of memory!\n");
			exit(0);
		}

		if(sscanf(line,"%d %d",&p->account_id,&p->level) != 2 || p->level <= 0) {
			printf("gm_account: broken data [conf/GM_account.txt] line %d\n",c);
			continue;
		}
		else {
			if(p->level > 99)
				p->level = 99;
			numdb_insert(gm_account_db,p->account_id,p);
			c++;
			// printf("GM ID: %d Level: %d\n",p->account_id,p->level);
		}
	}
	fclose(fp);
	printf("%d ID of gm_accounts read.\n",c);
	return 0;
}

int login_convert(void)
{
	char tmpsql[1024];
	MYSQL_RES* 	sql_res ;
	MYSQL_ROW	sql_row ;
	FILE *fp;
	int account_id, logincount, user_level, state, n, i;
	char line[2048], userid[2048], pass[2048], lastlogin[2048], sex, email[2048], error_message[2048], last_ip[2048], memo[2048];
	time_t ban_until_time;
	time_t connect_until_time;
	char t_uid[256];
	char input;

	printf("\nDo you wish to convert your Login Database to SQL? (y/n) : ");
	input=getchar();
	if(input == 'y' || input == 'Y') {
		read_gm_account();
		fp=fopen("save/account.txt","r");
		if(fp==NULL)
			return 0;
		while(fgets(line,1023,fp)!=NULL){

			if(line[0]=='/' && line[1]=='/')
				continue;

			i = sscanf(line, "%d\t%[^\t]\t%[^\t]\t%[^\t]\t%c\t%d\t%d\t"
							 "%[^\t]\t%[^\t]\t%ld\t%[^\t]\t%[^\t]\t%ld%n",
				&account_id, userid, pass, lastlogin, &sex, &logincount, &state,
				email, error_message, &connect_until_time, last_ip, memo, &ban_until_time, &n);

			sprintf(
				tmpsql, "SELECT `%s`,`%s`,`%s`,`lastlogin`,`logincount`,`sex`,`connect_until`,`last_ip`,`ban_until`,`state`"
				" FROM `%s` WHERE `%s`='%s'", login_account_id, login_userid, login_user_pass, login_db, login_userid,
				strecpy(t_uid,userid)
			);

			if(mysql_query(&mysql_handle, tmpsql) ) {
				printf("DB server Error - %s\n", mysql_error(&mysql_handle) );
			}
			user_level = isGM(account_id);
			sql_res = mysql_store_result(&mysql_handle) ;
			sql_row = mysql_fetch_row(sql_res);	//row fetching
			if (!sql_row) //no row -> insert
				sprintf(tmpsql, "INSERT INTO `login` (`account_id`, `userid`, `user_pass`, `lastlogin`, `sex`, `logincount`, `email`, `level`) VALUES (%d, '%s', '%s', '%s', '%c', %d, 'user@athena', %d);",account_id , userid, pass,lastlogin,sex,logincount, user_level);
			else //row reside -> updating
				sprintf(tmpsql, "UPDATE `login` SET `account_id`='%d', `userid`='%s', `user_pass`='%s', `lastlogin`='%s', `sex`='%c', `logincount`='%d', `email`='user@athena', `level`='%d'\nWHERE `account_id`='%d';",account_id , userid, pass,lastlogin,sex,logincount, user_level, account_id);
			mysql_free_result(sql_res) ; //resource free
			if(mysql_query(&mysql_handle, tmpsql) ) {
				printf("DB server Error - %s\n", mysql_error(&mysql_handle) );
			}
		}
		fclose(fp);
	}
	while(getchar() != '\n');

	return 0;
}

