// $Id: login-converter.c,v 1.1 2005/03/21 12:00:14 aboon Exp $
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

void read_gm_account() {
	char line[8192];
	struct gm_account *p;
	FILE *fp;
	int c, l;
	int account_id, level;
	int i;
	int range, start_range, end_range;

	gm_account_db = numdb_init();

	printf("Starting reading gm_account\n");

	if ((fp = fopen(GM_account_filename, "r")) == NULL) {
		printf("File not found: %s.\n", GM_account_filename);
		return;
	}

	line[sizeof(line)-1] = '\0';
	c = 0;
	l = 0;
	while(fgets(line, sizeof(line)-1, fp)) {
		l++;
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;

		if ((range = sscanf(line, "%d%*[-~]%d %d", &start_range, &end_range, &level)) == 3 ||
		    (range = sscanf(line, "%d%*[-~]%d:%d", &start_range, &end_range, &level)) == 3 ||
		    (range = sscanf(line, "%d %d", &start_range, &level)) == 2 ||
		    (range = sscanf(line, "%d:%d", &start_range, &level)) == 2 ||
		    (range = sscanf(line, "%d: %d", &start_range, &level)) == 2) {
			if (level <= 0) {
				printf("gm_account [%s]: invalid GM level [%ds] line %d\n", GM_account_filename, level, l);
			} else {
				if (level > 99)
					level = 99;
				if (range == 2)
					end_range = start_range;
				else if (end_range < start_range) {
					i = end_range;
					end_range = start_range;
					start_range = i;
				}
				for (account_id = start_range; account_id <= end_range; account_id++) {
					p = (struct gm_account*)malloc(sizeof(struct gm_account));
					if (p == NULL) {
						printf("gm_account: out of memory!\n");
						exit(0);
					}
					p->account_id = account_id;
					p->level = level;
					numdb_insert(gm_account_db, account_id, p);
					//	printf("GM ID: %d Level: %d\n", account_id, level);
					c++;
				}
			}
		} else {
			printf("gm_account: broken data [%s] line %d\n", GM_account_filename, l);
		}
	}
	fclose(fp);
	printf("gm_account: %s read done (%d gm account ID)\n", GM_account_filename, c);

	return;
}

int login_convert(void)
{
	char tmpsql[1024];
	MYSQL_RES* sql_res ;
	MYSQL_ROW  sql_row ;
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
				        " FROM `%s` WHERE `%s`='%s'",
				login_db_account_id, login_db_userid, login_db_user_pass,
				login_db, login_db_userid, strecpy(t_uid,userid)
			);

			if(mysql_query(&mysql_handle, tmpsql) ) {
				printf("DB server Error - %s\n", mysql_error(&mysql_handle) );
			}
			user_level = isGM(account_id);
			sql_res = mysql_store_result(&mysql_handle) ;
			sql_row = mysql_fetch_row(sql_res);	//row fetching
			if (!sql_row) //no row -> insert
				sprintf(tmpsql, "INSERT INTO `%s` (`%s`, `%s`, `%s`, `lastlogin`, `sex`, `logincount`, `email`, `%s`)"
				                " VALUES (%d, '%s', '%s', '%s', '%c', %d, 'user@athena', %d);",
				login_db, login_db_account_id, login_db_userid, login_db_user_pass, login_db_level,
				account_id , userid, pass, lastlogin, sex, logincount, user_level);
			else //row reside -> updating
				sprintf(tmpsql, "UPDATE `%s` SET `%s`='%d', `%s`='%s', `%s`='%s', `lastlogin`='%s',"
				                " `sex`='%c', `logincount`='%d', `email`='user@athena', `%s`='%d'\nWHERE `account_id`='%d';",
				login_db, login_db_account_id, account_id , login_db_userid, userid, login_db_user_pass, pass, lastlogin,
				sex, logincount, login_db_level, user_level, account_id);
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

