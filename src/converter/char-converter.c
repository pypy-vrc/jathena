// $Id: char-converter.c,v 1.1 2005/03/21 12:00:14 aboon Exp $
// original : char2.c 2003/03/14 11:58:35 Rev.1.5

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STORAGE_MEMINC	16

#include "converter.h"
#include "../common/mmo.h"
#include "../char/char.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#include "../common/socket.h"
#include <mysql.h>

int sql_fields, sql_cnt;

int char_sql_saveitem(struct item *item, int max, int id, int tableswitch) {
	int i;
	const char *tablename;
	const char *selectoption;
	char *p;
	char sep = ' ';

	switch (tableswitch) {
	case TABLE_INVENTORY:
		tablename    = "inventory"; // no need for sprintf here as *_db are char*.
		selectoption = "char_id";
		break;
	case TABLE_CART:
		tablename    = "cart_inventory";
		selectoption = "char_id";
		break;
	case TABLE_STORAGE:
		tablename    = "storage";
		selectoption = "account_id";
		break;
	case TABLE_GUILD_STORAGE:
		tablename    = "guild_storage";
		selectoption = "guild_id";
		break;
	default:
		printf("Invalid table name!\n");
		return 1;
	}

	// delete
	sprintf(tmp_sql,"DELETE FROM `%s` WHERE `%s`='%d'",tablename,selectoption,id);
	if(mysql_query(&mysql_handle, tmp_sql)) {
		printf("DB server Error (delete `%s`)- %s\n",tablename,mysql_error(&mysql_handle));
	}

	p  = tmp_sql;
	p += sprintf(
		p,"INSERT INTO `%s`(`%s`, `nameid`, `amount`, `equip`, `identify`, `refine`, "
		"`attribute`, `card0`, `card1`, `card2`, `card3` ) VALUES",tablename,selectoption
	);

	for(i = 0 ; i < max ; i++) {
		if(item[i].nameid) {
			p += sprintf(
				p,"%c('%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d')",
				sep,id,item[i].nameid,item[i].amount,item[i].equip,item[i].identify,
				item[i].refine,item[i].attribute,item[i].card[0],item[i].card[1],
				item[i].card[2],item[i].card[3]
			);
			sep = ',';
		}
	}
	if(sep == ',') {
		if(mysql_query(&mysql_handle, tmp_sql)) {
			printf("DB server Error (INSERT `%s`)- %s\n", tablename, mysql_error(&mysql_handle));
		}
	}

	return 0;
}

int inter_pet_fromstr(char *str, struct s_pet *p)
{
	int s;
	int tmp_int[16];
	char tmp_str[256];

	memset(p, 0, sizeof(struct s_pet));

//	printf("sscanf pet main info\n");
	s=sscanf(str,"%d, %d,%[^\t]\t%d, %d, %d, %d, %d, %d, %d, %d, %d", &tmp_int[0], &tmp_int[1], tmp_str, &tmp_int[2],
		&tmp_int[3], &tmp_int[4], &tmp_int[5], &tmp_int[6], &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10]);

	if(s!=12)
		return 1;

	p->pet_id = tmp_int[0];
	p->class  = tmp_int[1];
	memcpy(p->name, tmp_str, 24);
	p->account_id = tmp_int[2];
	p->char_id = tmp_int[3];
	p->level = tmp_int[4];
	p->egg_id = tmp_int[5];
	p->equip = tmp_int[6];
	p->intimate = tmp_int[7];
	p->hungry = tmp_int[8];
	p->rename_flag = tmp_int[9];
	p->incubate = tmp_int[10];

	if(p->hungry < 0)
		p->hungry = 0;
	else if(p->hungry > 100)
		p->hungry = 100;
	if(p->intimate < 0)
		p->intimate = 0;
	else if(p->intimate > 1000)
		p->intimate = 1000;

	return 0;
}
//---------------------------------------------------------
int inter_pet_tosql(int pet_id, struct s_pet *p) {
	//`pet` (`pet_id`, `class`,`name`,`account_id`,`char_id`,`level`,`egg_id`,`equip`,`intimate`,`hungry`,`rename_flag`,`incubate`)

	char tmp_sql[65535];
	char t_name[256];
	MYSQL_RES* 	sql_res ;
	MYSQL_ROW	sql_row ;

	strecpy (t_name, p->name);
	if(p->hungry < 0)
		p->hungry = 0;
	else if(p->hungry > 100)
		p->hungry = 100;
	if(p->intimate < 0)
		p->intimate = 0;
	else if(p->intimate > 1000)
		p->intimate = 1000;
	sprintf(tmp_sql,"SELECT * FROM `pet` WHERE `pet_id`='%d'",pet_id);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
			printf("DB server Error - %s\n", mysql_error(&mysql_handle) );
	}
	sql_res = mysql_store_result(&mysql_handle) ;
	sql_row = mysql_fetch_row(sql_res);	//row fetching
	if (!sql_row) //no row -> insert
		sprintf(tmp_sql,"INSERT INTO `pet` (`pet_id`, `class`,`name`,`account_id`,`char_id`,`level`,`egg_id`,`equip`,`intimate`,`hungry`,`rename_flag`,`incubate`) VALUES ('%d', '%d', '%s', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d')",
			p->pet_id, p->class, t_name, p->account_id, p->char_id, p->level, p->egg_id,
			p->equip, p->intimate, p->hungry, p->rename_flag, p->incubate);
	else //row reside -> updating
		sprintf(tmp_sql, "UPDATE `pet` SET `class`='%d',`name`='%s',`account_id`='%d',`char_id`='%d',`level`='%d',`egg_id`='%d',`equip`='%d',`intimate`='%d',`hungry`='%d',`rename_flag`='%d',`incubate`='%d' WHERE `pet_id`='%d'",
			p->class,  t_name, p->account_id, p->char_id, p->level, p->egg_id,
			p->equip, p->intimate, p->hungry, p->rename_flag, p->incubate, p->pet_id);
	mysql_free_result(sql_res) ; //resource free
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		printf("DB server Error - %s\n", mysql_error(&mysql_handle) );
	}

	return 0;
}

// char to storage
int storage_fromstr(char *str, struct storage *p)
{
	int tmp_int[256];
	int set, next, len, i;

	set=sscanf(str,"%d, %d%n", &tmp_int[0], &tmp_int[1], &next);
	p->storage_amount=tmp_int[1];

	if(set!=2)
		return 0;
	if(str[next]=='\n' || str[next]=='\r')
		return 1;
	next++;
	for(i=0;str[next] && str[next]!='\t';i++){
		if(sscanf(str + next, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
		      &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3],
		      &tmp_int[4], &tmp_int[5], &tmp_int[6],
		      &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10], &len) == 11) {
			p->storage[i].id = tmp_int[0];
			p->storage[i].nameid = tmp_int[1];
			p->storage[i].amount = tmp_int[2];
			p->storage[i].equip = tmp_int[3];
			p->storage[i].identify = tmp_int[4];
			p->storage[i].refine = tmp_int[5];
			p->storage[i].attribute = tmp_int[6];
			p->storage[i].card[0] = tmp_int[7];
			p->storage[i].card[1] = tmp_int[8];
			p->storage[i].card[2] = tmp_int[9];
			p->storage[i].card[3] = tmp_int[10];
			next += len;
			if (str[next] == ' ')
				next++;
		}

		else return 0;
	}
	return 1;
}

/////////////////////////////////
int mmo_char_fromstr(char *str, struct mmo_charstatus *p) {
	int tmp_int[256];
	int set, next, len, i;

	// initilialise character
	memset(p, '\0', sizeof(struct mmo_charstatus));
	// 1882以降の形式読み込み
	if( (set=sscanf(str,"%d\t%d,%d\t%[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
		"\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
		"\t%[^,],%d,%d\t%[^,],%d,%d,%d,%d,%d,%d%n",
		&tmp_int[0],&tmp_int[1],&tmp_int[2],p->name, //
		&tmp_int[3],&tmp_int[4],&tmp_int[5],
		&tmp_int[6],&tmp_int[7],&tmp_int[8],
		&tmp_int[9],&tmp_int[10],&tmp_int[11],&tmp_int[12],
		&tmp_int[13],&tmp_int[14],&tmp_int[15],&tmp_int[16],&tmp_int[17],&tmp_int[18],
		&tmp_int[19],&tmp_int[20],
		&tmp_int[21],&tmp_int[22],&tmp_int[23], //
		&tmp_int[24],&tmp_int[25],&tmp_int[26],&tmp_int[43],
		&tmp_int[27],&tmp_int[28],&tmp_int[29],
		&tmp_int[30],&tmp_int[31],&tmp_int[32],&tmp_int[33],&tmp_int[34],
		p->last_point.map,&tmp_int[35],&tmp_int[36], //
		p->save_point.map,&tmp_int[37],&tmp_int[38],&tmp_int[39],&tmp_int[40],&tmp_int[41],&tmp_int[42],&next
		)
	)!=47 ){
		tmp_int[43]=0;
		// If it's not char structure of version 1425 and after
		if((set = sscanf(str, "%d\t%d,%d\t%[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
		   		"\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
		   		"\t%[^,],%d,%d\t%[^,],%d,%d,%d,%d,%d,%d%n",
		   	&tmp_int[0], &tmp_int[1], &tmp_int[2], p->name, //
		   	&tmp_int[3], &tmp_int[4], &tmp_int[5],
		   	&tmp_int[6], &tmp_int[7], &tmp_int[8],
		   	&tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
		   	&tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
		   	&tmp_int[19], &tmp_int[20],
		   	&tmp_int[21], &tmp_int[22], &tmp_int[23], //
		   	&tmp_int[24], &tmp_int[25], &tmp_int[26],
		   	&tmp_int[27], &tmp_int[28], &tmp_int[29],
		   	&tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
		   	p->last_point.map, &tmp_int[35], &tmp_int[36], //
		   	p->save_point.map, &tmp_int[37], &tmp_int[38], &tmp_int[39], &tmp_int[40],
		   	&tmp_int[41],&tmp_int[42],&next)) != 46) 
		{	
			tmp_int[40] = 0;
		   	tmp_int[41] = 0;
		   	tmp_int[42] = 0;
			// If it's not char structure of version 1008 and after
			if ((set = sscanf(str, "%d\t%d,%d\t%[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
		   		"\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
		   		"\t%[^,],%d,%d\t%[^,],%d,%d,%d%n",
		   	&tmp_int[0], &tmp_int[1], &tmp_int[2], p->name, //
		   	&tmp_int[3], &tmp_int[4], &tmp_int[5],
		   	&tmp_int[6], &tmp_int[7], &tmp_int[8],
		   	&tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
		   	&tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
		   	&tmp_int[19], &tmp_int[20],
		   	&tmp_int[21], &tmp_int[22], &tmp_int[23], //
		   	&tmp_int[24], &tmp_int[25], &tmp_int[26],
		   	&tmp_int[27], &tmp_int[28], &tmp_int[29],
		   	&tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
		   	p->last_point.map, &tmp_int[35], &tmp_int[36], //
		   	p->save_point.map, &tmp_int[37], &tmp_int[38], &tmp_int[39], &next)) != 43) 
		   	{
				tmp_int[39] = 0; // partner id
				// If not char structure from version 384 to 1007
				if ((set = sscanf(str, "%d\t%d,%d\t%[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
				   "\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
				   "\t%[^,],%d,%d\t%[^,],%d,%d%n",
				   &tmp_int[0], &tmp_int[1], &tmp_int[2], p->name, //
				   &tmp_int[3], &tmp_int[4], &tmp_int[5],
				   &tmp_int[6], &tmp_int[7], &tmp_int[8],
				   &tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
				   &tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
				   &tmp_int[19], &tmp_int[20],
				   &tmp_int[21], &tmp_int[22], &tmp_int[23], //
				   &tmp_int[24], &tmp_int[25], &tmp_int[26],
				   &tmp_int[27], &tmp_int[28], &tmp_int[29],
				   &tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
				   p->last_point.map, &tmp_int[35], &tmp_int[36], //
				   p->save_point.map, &tmp_int[37], &tmp_int[38], &next)) != 42) 
				  {
					// It's char structure of a version before 384
					tmp_int[26] = 0; // pet id
					set = sscanf(str, "%d\t%d,%d\t%[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
				      	"\t%d,%d,%d\t%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
				      	"\t%[^,],%d,%d\t%[^,],%d,%d%n",
				      	&tmp_int[0], &tmp_int[1], &tmp_int[2], p->name, //
				      	&tmp_int[3], &tmp_int[4], &tmp_int[5],
				      	&tmp_int[6], &tmp_int[7], &tmp_int[8],
				      	&tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
				      	&tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
				      	&tmp_int[19], &tmp_int[20],
				      	&tmp_int[21], &tmp_int[22], &tmp_int[23], //
				      	&tmp_int[24], &tmp_int[25], //
				      	&tmp_int[27], &tmp_int[28], &tmp_int[29],
				      	&tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
				      	p->last_point.map, &tmp_int[35], &tmp_int[36], //
				      	p->save_point.map, &tmp_int[37], &tmp_int[38], &next);
					set += 6;
				//printf("char: old char data ver.1\n");
				// Char structure of version 1007 or older
				} else {
					set+=5;
					//printf("char: old char data ver.2\n");
				}
			// Char structure of version 1008+
			} else {
				//printf("char: new char data ver.3\n");
				set+=4;
			}
		} else {
			set += 1;
		}
	}
	if (set != 47)
		return 0;

	p->char_id = tmp_int[0];
	p->account_id = tmp_int[1];
	p->char_num = tmp_int[2];
	p->class    = tmp_int[3];
	p->base_level = tmp_int[4];
	p->job_level = tmp_int[5];
	p->base_exp = tmp_int[6];
	p->job_exp = tmp_int[7];
	p->zeny = tmp_int[8];
	p->hp = tmp_int[9];
	p->max_hp = tmp_int[10];
	p->sp = tmp_int[11];
	p->max_sp = tmp_int[12];
	p->str = tmp_int[13];
	p->agi = tmp_int[14];
	p->vit = tmp_int[15];
	p->int_ = tmp_int[16];
	p->dex = tmp_int[17];
	p->luk = tmp_int[18];
	p->status_point = tmp_int[19];
	p->skill_point = tmp_int[20];
	p->option = tmp_int[21];
	p->karma = tmp_int[22];
	p->manner = tmp_int[23];
	p->party_id = tmp_int[24];
	p->guild_id = tmp_int[25];
	p->pet_id = tmp_int[26];
	p->hair = tmp_int[27];
	p->hair_color = tmp_int[28];
	p->clothes_color = tmp_int[29];
	p->weapon = tmp_int[30];
	p->shield = tmp_int[31];
	p->head_top = tmp_int[32];
	p->head_mid = tmp_int[33];
	p->head_bottom = tmp_int[34];
	p->last_point.x = tmp_int[35];
	p->last_point.y = tmp_int[36];
	p->save_point.x = tmp_int[37];
	p->save_point.y = tmp_int[38];
	p->partner_id 	= tmp_int[39];
	p->parent_id[0] = tmp_int[40];
	p->parent_id[1] = tmp_int[41];
	p->baby_id 		= tmp_int[42];
	p->homun_id     = tmp_int[43];

	if (str[next] == '\n' || str[next] == '\r')
		return 1;	// 新規データ

	next++;

	for(i = 0; str[next] && str[next] != '\t' && i < MAX_PORTAL_MEMO; i++) {
		set = sscanf(str+next, "%[^,],%d,%d%n", p->memo_point[i].map, &tmp_int[0], &tmp_int[1], &len);
		if (set != 3)
			return -3;
		p->memo_point[i].x = tmp_int[0];
		p->memo_point[i].y = tmp_int[1];
		next += len;
		if (str[next] == ' ')
			next++;
	}

	next++;

	for(i = 0; str[next] && str[next] != '\t'; i++) {
		if(sscanf(str + next, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
		          &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3],
		          &tmp_int[4], &tmp_int[5], &tmp_int[6],
		          &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10], &len) == 11) {
			tmp_int[11] = 0; // broken doesn't exist in this version -> 0
		} else // invalid structure
			return -4;
		p->inventory[i].id = tmp_int[0];
		p->inventory[i].nameid = tmp_int[1];
		p->inventory[i].amount = tmp_int[2];
		p->inventory[i].equip = tmp_int[3];
		p->inventory[i].identify = tmp_int[4];
		p->inventory[i].refine = tmp_int[5];
		p->inventory[i].attribute = tmp_int[6];
		p->inventory[i].card[0] = tmp_int[7];
		p->inventory[i].card[1] = tmp_int[8];
		p->inventory[i].card[2] = tmp_int[9];
		p->inventory[i].card[3] = tmp_int[10];
		next += len;
		if (str[next] == ' ')
			next++;
	}

	next++;

	for(i = 0; str[next] && str[next] != '\t'; i++) {
		if (sscanf(str + next, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
		           &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3],
		           &tmp_int[4], &tmp_int[5], &tmp_int[6],
		           &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10], &len) == 11) {
			tmp_int[11] = 0; // broken doesn't exist in this version -> 0
		} else // invalid structure
			return -5;
		p->cart[i].id = tmp_int[0];
		p->cart[i].nameid = tmp_int[1];
		p->cart[i].amount = tmp_int[2];
		p->cart[i].equip = tmp_int[3];
		p->cart[i].identify = tmp_int[4];
		p->cart[i].refine = tmp_int[5];
		p->cart[i].attribute = tmp_int[6];
		p->cart[i].card[0] = tmp_int[7];
		p->cart[i].card[1] = tmp_int[8];
		p->cart[i].card[2] = tmp_int[9];
		p->cart[i].card[3] = tmp_int[10];
		next += len;
		if (str[next] == ' ')
			next++;
	}

	next++;

	for(i = 0; str[next] && str[next] != '\t'; i++) {
		set = sscanf(str + next, "%d,%d%n", &tmp_int[0], &tmp_int[1], &len);
		if (set != 2)
			return -6;
		p->skill[tmp_int[0]].id = tmp_int[0];
		p->skill[tmp_int[0]].lv = tmp_int[1];
		next += len;
		if (str[next] == ' ')
			next++;
	}

	next++;

	for(i = 0; str[next] && str[next] != '\t' && str[next] != '\n' && str[next] != '\r'; i++) { // global_reg実装以前のathena.txt互換のため一応'\n'チェック
		set = sscanf(str + next, "%[^,],%d%n", p->global_reg[i].str, &p->global_reg[i].value, &len);
		if (set != 2) {
			// because some scripts are not correct, the str can be "". So, we must check that.
			// If it's, we must not refuse the character, but just this REG value.
			// Character line will have something like: nov_2nd_cos,9 ,9 nov_1_2_cos_c,1 (here, ,9 is not good)
			if (str[next] == ',' && sscanf(str + next, ",%d%n", &p->global_reg[i].value, &len) == 1)
				i--;
			else
				return -7;
		}
		next += len;
		if (str[next] == ' ')
			next++;
	}
	p->global_reg_num = i;

	return 1;
}

//==========================================================================================================
int mmo_char_tosql(int char_id, struct mmo_charstatus *p){
	char name[256];
	int i,save_flag;

	save_flag = char_id;
	//`char`( `char_id`,`account_id`,`char_num`,`name`,`class`,`base_level`,`job_level`,`base_exp`,`job_exp`,`zeny`, //9
	//`str`,`agi`,`vit`,`int`,`dex`,`luk`, //15
	//`max_hp`,`hp`,`max_sp`,`sp`,`status_point`,`skill_point`, //21
	//`option`,`karma`,`manner`,`party_id`,`guild_id`,`pet_id`, //27
	//`hair`,`hair_color`,`clothes_color`,`weapon`,`shield`,`head_top`,`head_mid`,`head_bottom`, //35
	//`last_map`,`last_x`,`last_y`,`save_map`,`save_x`,`save_y`,`partner_id`, `parent_id`,`parent_id2`,`baby_id`)
	sprintf(tmp_sql ,"INSERT INTO `char` SET `char_id`='%d', `account_id`='%d', `char_num`='%d', `name`='%s', `class`='%d', `base_level`='%d', `job_level`='%d',"
		"`base_exp`='%d', `job_exp`='%d', `zeny`='%d',"
		"`max_hp`='%d',`hp`='%d',`max_sp`='%d',`sp`='%d',`status_point`='%d',`skill_point`='%d',"
		"`str`='%d',`agi`='%d',`vit`='%d',`int`='%d',`dex`='%d',`luk`='%d',"
		"`option`='%d',`karma`='%d',`manner`='%d',`party_id`='%d',`guild_id`='%d',`pet_id`='%d',"
		"`hair`='%d',`hair_color`='%d',`clothes_color`='%d',`weapon`='%d',`shield`='%d',`head_top`='%d',`head_mid`='%d',`head_bottom`='%d',"
		"`last_map`='%s',`last_x`='%d',`last_y`='%d',`save_map`='%s',`save_x`='%d',`save_y`='%d',"
		"`partner_id` = '%d', `parent_id` = '%d', `parent_id2` = '%d', `baby_id` = '%d', `homun_id` = '%d'",
		char_id,p->account_id,p->char_num,strecpy(name,p->name),p->class , p->base_level, p->job_level,
		p->base_exp, p->job_exp, p->zeny,
		p->max_hp, p->hp, p->max_sp, p->sp, p->status_point, p->skill_point,
		p->str, p->agi, p->vit, p->int_, p->dex, p->luk,
		p->option, p->karma, p->manner, p->party_id, p->guild_id, p->pet_id,
		p->hair, p->hair_color, p->clothes_color,
		p->weapon, p->shield, p->head_top, p->head_mid, p->head_bottom,
		p->last_point.map, p->last_point.x, p->last_point.y,
		p->save_point.map, p->save_point.x, p->save_point.y,
		p->partner_id , p->parent_id[0] ,p->parent_id[1] , p->baby_id, p->homun_id
	);

	if(mysql_query(&mysql_handle, tmp_sql) ) {
			printf("DB server Error (update `char`)- %s\n", mysql_error(&mysql_handle) );
	}

	//`memo` (`memo_id`,`char_id`,`map`,`x`,`y`)
	sprintf(tmp_sql,"DELETE FROM `memo` WHERE `char_id`='%d'",char_id);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
			printf("DB server Error (delete `memo`)- %s\n", mysql_error(&mysql_handle) );
	}

	//insert here.
	for(i = 0; i < MAX_PORTAL_MEMO; i++) {
		if(p->memo_point[i].map[0]){
			sprintf(tmp_sql,"INSERT INTO `memo`(`char_id`,`map`,`x`,`y`) VALUES ('%d', '%s', '%d', '%d')",
				char_id, p->memo_point[i].map, p->memo_point[i].x, p->memo_point[i].y);
			if(mysql_query(&mysql_handle, tmp_sql) )
				printf("DB server Error (insert `memo`)- %s\n", mysql_error(&mysql_handle) );
		}
	}

	char_sql_saveitem(p->inventory,MAX_INVENTORY,p->char_id,TABLE_INVENTORY);
	char_sql_saveitem(p->cart     ,MAX_CART     ,p->char_id,TABLE_CART);

	//`skill` (`char_id`, `id`, `lv`)
	sprintf(tmp_sql,"DELETE FROM `skill` WHERE `char_id`='%d'",char_id);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
			printf("DB server Error (delete `skill`)- %s\n", mysql_error(&mysql_handle) );
	}

	//insert here.
	for(i=0;i<MAX_SKILL;i++){
		if(p->skill[i].id){
			if (p->skill[i].id && p->skill[i].flag!=1) {
				sprintf(tmp_sql,"INSERT INTO `skill`(`char_id`,`id`, `lv`) VALUES ('%d', '%d', '%d')",
					char_id, p->skill[i].id, (p->skill[i].flag==0)?p->skill[i].lv:p->skill[i].flag-2);
				if(mysql_query(&mysql_handle, tmp_sql) ) {
					printf("DB server Error (insert `skill`)- %s\n", mysql_error(&mysql_handle) );
				}
			}
		}
	}
	//`global_reg_value` (`char_id`, `str`, `value`)
	sprintf(tmp_sql,"DELETE FROM `global_reg_value` WHERE `char_id`='%d'",char_id);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
			printf("DB server Error (delete `global_reg_value`)- %s\n", mysql_error(&mysql_handle) );
	}

	//insert here.
	for(i=0;i<p->global_reg_num;i++){
		if(p->global_reg[i].value !=0){
			sprintf(tmp_sql,"INSERT INTO `global_reg_value` (`char_id`, `str`, `value`) VALUES ('%d', '%s','%d')",
				char_id, p->global_reg[i].str, p->global_reg[i].value);
			if(mysql_query(&mysql_handle, tmp_sql) ) {
				printf("DB server Error (insert `global_reg_value`)- %s\n", mysql_error(&mysql_handle) );
			}
		}
	}
	save_flag = 0;

  return 0;
}
//==========================================================================================================

// パーティデータの文字列からの変換
int party_fromstr(char *str,struct party *p)
{
	int i,j,s;
	int tmp_int[16];
	char tmp_str[256];
	
	memset(p,0,sizeof(struct party));
	
//	printf("sscanf party main info\n");
	s=sscanf(str,"%d\t%[^\t]\t%d,%d\t",&tmp_int[0],
		tmp_str,&tmp_int[1],&tmp_int[2]);
	if(s!=4)
		return 1;
	
	p->party_id=tmp_int[0];
	strncpy(p->name,tmp_str,24);
	p->exp=tmp_int[1];
	p->item=tmp_int[2];	
//	printf("%d [%s] %d %d\n",tmp_int[0],tmp_str[0],tmp_int[1],tmp_int[2]);
	
	for(j=0;j<3 && str!=NULL;j++)
		str=strchr(str+1,'\t');
	
	for(i=0;i<MAX_PARTY;i++){
		struct party_member *m = &p->member[i];
		if(str==NULL)
			return 1;
//		printf("sscanf party member info %d\n",i);

		s=sscanf(str+1,"%d,%d\t%[^\t]\t",
			&tmp_int[0],&tmp_int[1],tmp_str);
		if(s!=3)
			return 1;
		
		m->account_id=tmp_int[0];
		m->leader=tmp_int[1];
		strncpy(m->name,tmp_str,sizeof(m->name));
//		printf(" %d %d [%s]\n",tmp_int[0],tmp_int[1],tmp_str);
		
		
		for(j=0;j<2 && str!=NULL;j++)
			str=strchr(str+1,'\t');
	}
	return 0;
}

int party_tosql(struct party *p) {
	// Add new party
	int i = 0;
	int leader_id = -1;
	char t_name[64];

	for(i = 0; i < MAX_PARTY; i++) {
		if(p->member[i].account_id > 0 && p->member[i].leader) {
			leader_id = p->member[i].account_id;
			break;
		}
	}
	// DBに挿入
	sprintf(
		tmp_sql,
		"INSERT INTO `%s`  (`party_id`, `name`, `exp`, `item`, `leader_id`) "
		"VALUES ('%d','%s', '%d', '%d', '%d')",
		"party",p->party_id,strecpy(t_name,p->name), p->exp, p->item,leader_id
	);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		printf("DB server Error (inset `party`)- %s\n", mysql_error(&mysql_handle) );
		return 0;
	}
	return 1;
}

// ギルドデータの文字列からの変換
int guild_fromstr(char *str,struct guild *g)
{
	int i,j,c;
	int tmp_int[16];
	char tmp_str[4][256];
	char tmp_str2[4096];
	char *pstr;
	
	// 基本データ
	memset(g,0,sizeof(struct guild));
	if( sscanf(str,"%d\t%[^\t]\t%[^\t]\t%d,%d,%d,%d,%d\t%[^\t]\t%[^\t]\t",&tmp_int[0],
		tmp_str[0],tmp_str[1],
		&tmp_int[1],&tmp_int[2],&tmp_int[3],&tmp_int[4],&tmp_int[5],
		tmp_str[2],tmp_str[3]) <8)
		return 1;
	g->guild_id=tmp_int[0];
	g->guild_lv=tmp_int[1];
	g->max_member=tmp_int[2];
	g->exp=tmp_int[3];
	g->skill_point=tmp_int[4];
	g->castle_id=tmp_int[5];
	memcpy(g->name,tmp_str[0],24);
	memcpy(g->master,tmp_str[1],24);
	memcpy(g->mes1,tmp_str[2],60);
	memcpy(g->mes2,tmp_str[3],120);
	g->mes1[strlen(g->mes1)-1]=0;
	g->mes2[strlen(g->mes2)-1]=0;

	for(j=0;j<6 && str!=NULL;j++)	// 位置スキップ
		str=strchr(str+1,'\t');
//	printf("GuildBaseInfo OK\n");
	
	// メンバー
	for(i=0;i<g->max_member;i++){
		struct guild_member *m = &g->member[i];
		if( sscanf(str+1,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\t%[^\t]\t",
			&tmp_int[0],&tmp_int[1],&tmp_int[2],&tmp_int[3],&tmp_int[4],
			&tmp_int[5],&tmp_int[6],&tmp_int[7],&tmp_int[8],&tmp_int[9],
			tmp_str[0]) <11)
			return 1;
		m->account_id=tmp_int[0];
		m->char_id=tmp_int[1];
		m->hair=tmp_int[2];
		m->hair_color=tmp_int[3];
		m->gender=tmp_int[4];
		m->class=tmp_int[5];
		m->lv=tmp_int[6];
		m->exp=tmp_int[7];
		m->exp_payper=tmp_int[8];
		m->position=tmp_int[9];
		memcpy(m->name,tmp_str[0],24);
		
		for(j=0;j<2 && str!=NULL;j++)	// 位置スキップ
			str=strchr(str+1,'\t');
	}
//	printf("GuildMemberInfo OK\n");
	// 役職
	for(i=0;i<MAX_GUILDPOSITION;i++){
		struct guild_position *p = &g->position[i];
		if( sscanf(str+1,"%d,%d\t%[^\t]\t",
			&tmp_int[0],&tmp_int[1],tmp_str[0]) < 3)
			return 1;
		p->mode=tmp_int[0];
		p->exp_mode=tmp_int[1];
		tmp_str[0][strlen(tmp_str[0])-1]=0;
		memcpy(p->name,tmp_str[0],24);

		for(j=0;j<2 && str!=NULL;j++)	// 位置スキップ
			str=strchr(str+1,'\t');
	}
//	printf("GuildPositionInfo OK\n");
	// エンブレム
	tmp_int[1]=0;
	if( sscanf(str+1,"%d,%d,%[^\t]\t",&tmp_int[0],&tmp_int[1],tmp_str2)< 3 &&
		sscanf(str+1,"%d,%[^\t]\t",&tmp_int[0],tmp_str2) < 2	)
		return 1;
	g->emblem_len=tmp_int[0];
	g->emblem_id=tmp_int[1];
	for(i=0,pstr=tmp_str2;i<g->emblem_len;i++,pstr+=2){
		int c1=pstr[0],c2=pstr[1],x1=0,x2=0;
		if(c1>='0' && c1<='9')x1=c1-'0';
		if(c1>='a' && c1<='f')x1=c1-'a'+10;
		if(c1>='A' && c1<='F')x1=c1-'A'+10;
		if(c2>='0' && c2<='9')x2=c2-'0';
		if(c2>='a' && c2<='f')x2=c2-'a'+10;
		if(c2>='A' && c2<='F')x2=c2-'A'+10;
		g->emblem_data[i]=(x1<<4)|x2;
	}
//	printf("GuildEmblemInfo OK\n");
	str=strchr(str+1,'\t');	// 位置スキップ

	// 同盟リスト
	if( sscanf(str+1,"%d\t",&c)< 1)
		return 1;
	str=strchr(str+1,'\t');	// 位置スキップ
	for(i=0;i<c;i++){
		struct guild_alliance *a = &g->alliance[i];
		if( sscanf(str+1,"%d,%d\t%[^\t]\t",
			&tmp_int[0],&tmp_int[1],tmp_str[0]) < 3)
			return 1;
		a->guild_id=tmp_int[0];
		a->opposition=tmp_int[1];
		memcpy(a->name,tmp_str[0],24);

		for(j=0;j<2 && str!=NULL;j++)	// 位置スキップ
			str=strchr(str+1,'\t');	
	}
//	printf("GuildAllianceInfo OK\n");
	// 追放リスト
	if( sscanf(str+1,"%d\t",&c)< 1)
		return 1;
	str=strchr(str+1,'\t');	// 位置スキップ
	for(i=0;i<c;i++){
		struct guild_explusion *e = &g->explusion[i];
		if( sscanf(str+1,"%d,%d,%d,%d\t%[^\t]\t%[^\t]\t%[^\t]\t",
			&tmp_int[0],&tmp_int[1],&tmp_int[2],&tmp_int[3],
			tmp_str[0],tmp_str[1],tmp_str[2]) < 6)
			return 1;
		e->account_id=tmp_int[0];
		e->rsv1=tmp_int[1];
		e->rsv2=tmp_int[2];
		e->rsv3=tmp_int[3];
		memcpy(e->name,tmp_str[0],24);
		memcpy(e->acc,tmp_str[1],24);
		tmp_str[2][strlen(tmp_str[2])-1]=0;
		memcpy(e->mes,tmp_str[2],40);

		for(j=0;j<4 && str!=NULL;j++)	// 位置スキップ
			str=strchr(str+1,'\t');	
	}
//	printf("GuildExplusionInfo OK\n");
	// ギルドスキル
	for(i=0;i<MAX_GUILDSKILL;i++){
		if( sscanf(str+1,"%d,%d ",&tmp_int[0],&tmp_int[1]) <2)
			break;
		g->skill[i].id=tmp_int[0];
		g->skill[i].lv=tmp_int[1];
		str=strchr(str+1,' ');	
	}
	str=strchr(str+1,'\t');
//	printf("GuildSkillInfo OK\n");
	return 0;
}

int  guild_tosql(struct guild* g2) {
	int  i;
	char buf[256],buf2[256],buf3[256],buf4[256];
	char *p;
	int len=0;
	char emblem_data[8192];
	char sep;

	// basic information
	sprintf(tmp_sql, "DELETE FROM `%s` WHERE `guild_id`='%d'","guild", g2->guild_id);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		printf("DB server Error (delete `guild`)- %s\n", mysql_error(&mysql_handle) );
	}
	for(i=0;i<g2->emblem_len;i++){
		len+=sprintf(emblem_data+len,"%02x",(unsigned char)(g2->emblem_data[i]));
		//printf("%02x",(unsigned char)(g2->emblem_data[i]));
	}
	emblem_data[len] = '\0';
	//printf("- emblem_len = %d \n",g->emblem_len);
	sprintf(
		tmp_sql,"INSERT INTO `%s` "
		"(`guild_id`, `name`,`master`,`guild_lv`,`connect_member`,`max_member`,`average_lv`,`exp`,"
		"`next_exp`,`skill_point`,`castle_id`,`mes1`,`mes2`,`emblem_len`,`emblem_id`,`emblem_data`) "
		"VALUES ('%d', '%s', '%s', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%s', '%s', '%d',"
		" '%d', '%s')","guild", g2->guild_id,strecpy(buf4,g2->name),strecpy(buf,g2->master),
		g2->guild_lv,g2->connect_member,g2->max_member,g2->average_lv,g2->exp,g2->next_exp,
		g2->skill_point,g2->castle_id,strecpy(buf2,g2->mes1),strecpy(buf3,g2->mes2),
		g2->emblem_len,g2->emblem_id,emblem_data
	);
	//printf(" %s\n",tmp_sql);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		printf("DB server Error (insert `guild`)- %s\n", mysql_error(&mysql_handle) );
	}

	sprintf(tmp_sql, "DELETE FROM `%s` WHERE `guild_id`='%d'","guild_member",g2->guild_id);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		printf("DB server Error (delete `guild_member`)- %s\n", mysql_error(&mysql_handle) );
	}

	p  = tmp_sql;
	p += sprintf(
		tmp_sql,
		"INSERT INTO `%s` (`guild_id`,`account_id`,`char_id`,`hair`,`hair_color`,`gender`,"
		"`class`,`lv`,`exp`,`exp_payper`,`online`,`position`,`rsv1`,`rsv2`,`name`) VALUES",
		"guild_member"
	);
	sep = ' ';

	// printf("- Insert guild %d to guild_member\n",g2->guild_id);
	for(i=0;i < g2->max_member;i++) {
		if (g2->member[i].account_id>0){
			struct guild_member *m = &g2->member[i];
			p += sprintf(
				p,
				"%c('%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%s')",
				sep,g2->guild_id,m->account_id,m->char_id,m->hair,m->hair_color,m->gender,
				m->class,m->lv,m->exp,m->exp_payper,m->online,m->position,0,0,
				strecpy(buf,m->name)
			);
			sep = ',';
		}
	}
	if(sep == ',') {
		if(mysql_query(&mysql_handle,tmp_sql)) {
			printf("DB server Error - %s\n", mysql_error(&mysql_handle) );
		}
	}

	//	printf("- Delete guild %d from guild_position\n",g2->guild_id);
	sprintf(tmp_sql, "DELETE FROM `%s` WHERE `guild_id`='%d'","guild_position", g2->guild_id);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		printf("DB server Error (delete `guild_position`)- %s\n", mysql_error(&mysql_handle) );
	}

	p  = tmp_sql;
	p += sprintf(
		tmp_sql,
		"INSERT INTO `%s` (`guild_id`,`position`,`name`,`mode`,`exp_mode`) VALUES",
		"guild_position"
	);
	sep = ' ';
	for(i=0;i<MAX_GUILDPOSITION;i++){
		struct guild_position *pos = &g2->position[i];
		p += sprintf(
			p,"%c('%d','%d','%s','%d','%d')",
			sep, g2->guild_id,i,strecpy(buf,pos->name),pos->mode,pos->exp_mode
		);
		sep = ',';
	}
	if(sep == ',') {
		if(mysql_query(&mysql_handle, tmp_sql) ) {
			printf("DB server Error (insert `guild_position`)- %s\n", mysql_error(&mysql_handle) );
		}
	}

	sprintf(
		tmp_sql,
		"DELETE FROM `%s` WHERE `guild_id`='%d'","guild_alliance",g2->guild_id
	);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		printf("DB server Error (delete `guild_alliance`)- %s\n", mysql_error(&mysql_handle) );
	}

	p  = tmp_sql;
	p += sprintf(
		tmp_sql,
		"INSERT INTO `%s` (`guild_id`,`opposition`,`alliance_id`,`name`) VALUES",
		"guild_alliance"
	);
	sep = ' ';
	for(i=0;i<MAX_GUILDALLIANCE;i++){
		struct guild_alliance *a = &g2->alliance[i];
		if(a->guild_id>0){
			p += sprintf(
				p,
				"%c('%d','%d','%d','%s')",
				sep,g2->guild_id,a->opposition,a->guild_id,strecpy(buf,a->name)
			);
			sep = ',';
		}
	}
	//printf(" %s\n",tmp_sql);
	if(sep == ',') {
		if(mysql_query(&mysql_handle, tmp_sql) ) {
			printf("DB server Error (insert `guild_alliance`)- %s\n", mysql_error(&mysql_handle) );
		}
	}

	sprintf(tmp_sql, "DELETE FROM `%s` WHERE `guild_id`='%d'","guild_expulsion", g2->guild_id);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		printf("DB server Error (delete `guild_expulsion`)- %s\n", mysql_error(&mysql_handle) );
	}
	p  = tmp_sql;
	p += sprintf(
		tmp_sql,
		"INSERT INTO `%s` (`guild_id`,`name`,`mes`,`acc`,`account_id`,`rsv1`,`rsv2`,`rsv3`) VALUES",
		"guild_expulsion"
	);
	sep = ' ';
	for(i=0;i<MAX_GUILDEXPLUSION;i++) {
		struct guild_explusion *e = &g2->explusion[i];
		if(e->account_id>0) {
			p += sprintf(
				p,"%c('%d','%s','%s','%s','%d','%d','%d','%d')",
				sep,g2->guild_id,strecpy(buf,e->name),strecpy(buf2,e->mes),e->acc,e->account_id,
				e->rsv1,e->rsv2,e->rsv3
			);
			sep = ',';
		}
	}
	if(sep == ',') {
		if(mysql_query(&mysql_handle, tmp_sql) ) {
			printf("DB server Error (insert `guild_expulsion`)- %s\n", mysql_error(&mysql_handle) );
		}
	}

	sprintf(tmp_sql,"DELETE FROM `%s` WHERE `guild_id`='%d'","guild_skill",g2->guild_id);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		printf("DB server Error (delete `guild_skill`)- %s\n", mysql_error(&mysql_handle) );
	}
	p  = tmp_sql;
	p += sprintf(
		tmp_sql,
		"INSERT INTO `%s` (`guild_id`,`id`,`lv`) VALUES",
		"guild_skill"
	);
	sep = ' ';
	for(i=0;i<MAX_GUILDSKILL;i++) {
		if (g2->skill[i].id > 0) {
			p += sprintf(
				p,"%c('%d','%d','%d')",
				sep,g2->guild_id,g2->skill[i].id,g2->skill[i].lv
			);
			sep = ',';
		}
	}
	if(sep == ',') {
		if(mysql_query(&mysql_handle, tmp_sql) ) {
			printf("DB server Error (insert `guild_skill`)- %s\n", mysql_error(&mysql_handle) );
		}
	}

	return 1;
}

int gstorage_fromstr(char *str,struct guild_storage *p)
{
	int tmp_int[256];
	int set,next,len,i;

	set=sscanf(str,"%d,%d%n",&tmp_int[0],&tmp_int[1],&next);
	p->storage_amount=tmp_int[1];

	if(set!=2)
		return 1;
	if(str[next]=='\n' || str[next]=='\r')
		return 0;	
	next++;
	for(i=0;str[next] && str[next]!='\t';i++){
		set=sscanf(str+next,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
		  &tmp_int[0],&tmp_int[1],&tmp_int[2],&tmp_int[3],
		  &tmp_int[4],&tmp_int[5],&tmp_int[6],
		  &tmp_int[7],&tmp_int[8],&tmp_int[9],&tmp_int[10],&len);
		if(set!=11)
			return 1;
		p->storage[i].id=tmp_int[0];
		p->storage[i].nameid=tmp_int[1];
		p->storage[i].amount=tmp_int[2];
		p->storage[i].equip=tmp_int[3];
		p->storage[i].identify=tmp_int[4];
		p->storage[i].refine=tmp_int[5];
		p->storage[i].attribute=tmp_int[6];
		p->storage[i].card[0]=tmp_int[7];
		p->storage[i].card[1]=tmp_int[8];
		p->storage[i].card[2]=tmp_int[9];
		p->storage[i].card[3]=tmp_int[10];

		next+=len;
		if(str[next]==' ')
			next++;
	}
	return 0;
}

int char_convert(void){
	char line[65536];
	int ret;
	int i=0,set,tmp_int[2], c= 0;
	char input;
	FILE *fp;

	printf("\nDo you wish to convert your Character Database to SQL? (y/n) : ");
	input=getchar();
	if(input == 'y' || input == 'Y'){
		printf("\nConverting Character Database...\n");
		fp=fopen("save/athena.txt","r");
		if(fp==NULL)
			return 0;
		while(fgets(line, 65535, fp)) {
			struct mmo_charstatus p;
			memset(&p, 0, sizeof(p));
			ret=mmo_char_fromstr(line, &p);
			if(ret) {
				mmo_char_tosql(p.char_id , &p);
			}
		}
		printf("char data convert end\n");
		fclose(fp);
	}

	while(getchar() != '\n');
	printf("\nDo you wish to convert your Storage Database to SQL? (y/n) : ");
	input=getchar();
	if(input == 'y' || input == 'Y') {
		printf("\nConverting Storage Database...\n");
		fp=fopen(storage_txt,"r");
		if(fp==NULL){
			printf("cant't read : %s\n",storage_txt);
			return 0;
		}

		while(fgets(line,65535,fp)){
			set=sscanf(line,"%d,%d",&tmp_int[0],&tmp_int[1]);
			if(set==2) {
				struct storage p;
				memset(&p,0,sizeof(struct storage));
				p.account_id=tmp_int[0];
				storage_fromstr(line,&p);
				char_sql_saveitem(p.storage,MAX_STORAGE,p.account_id,TABLE_STORAGE);
				i++;
			}
		}
		fclose(fp);
	}

	while(getchar() != '\n');
	printf("\nDo you wish to convert your Pet Database to SQL? (y/n) : ");
	input=getchar();
	if(input == 'y' || input == 'Y') {
		struct s_pet p;
		printf("\nConverting Pet Database...\n");
		if( (fp=fopen(pet_txt,"r")) ==NULL )
			return 1;
		while(fgets(line, sizeof(line), fp)){
			if(inter_pet_fromstr(line, &p)==0 && p.pet_id>0){
				//pet dump
				inter_pet_tosql(p.pet_id,&p);
			}else{
				printf("int_pet: broken data [%s] line %d\n", pet_txt, c);
			}
			c++;
		}
		fclose(fp);
	}
	while(getchar() != '\n');
	printf("\nDo you wish to convert your Party Database to SQL? (y/n) : ");
	input=getchar();
	if(input == 'y' || input == 'Y') {
		if( (fp=fopen(party_txt,"r"))==NULL )
			return 1;
		while(fgets(line,sizeof(line),fp)){
			int i,j=0;
			struct party p;
			if( sscanf(line,"%d\t%%newid%%\n%n",&i,&j)==1 && j>0){
				continue;
			}
			if(party_fromstr(line,&p)==0 && p.party_id>0){
				party_tosql(&p);
			}
		}
		fclose(fp);
	}
	while(getchar() != '\n');
	printf("\nDo you wish to convert your Guild Database to SQL? (y/n) : ");
	input=getchar();
	if(input == 'y' || input == 'Y') {
		struct guild g;
		if( (fp=fopen(guild_txt,"r"))==NULL )
			return 1;
		while(fgets(line,sizeof(line),fp)){
			int i,j=0;
			if( sscanf(line,"%d\t%%newid%%\n%n",&i,&j)==1 && j>0){
				continue;
			}
			if(guild_fromstr(line,&g)==0 && g.guild_id>0){
				guild_tosql(&g);
			}else{
				printf("int_guild: broken data [%s] line %d\n",guild_txt,c);
			}
		}
		fclose(fp);
	}
	while(getchar() != '\n');
	printf("\nDo you wish to convert your Guild Storage Database to SQL? (y/n) : ");
	input=getchar();
	if(input == 'y' || input == 'Y') {
		struct guild_storage gs;
		fp=fopen(guild_storage_txt,"r");
		if(fp==NULL){
			printf("cant't read : %s\n",guild_storage_txt);
			return 1;
		}
		while(fgets(line,65535,fp)){
			memset( &gs, 0, sizeof( struct guild_storage) );
			sscanf(line,"%d",&tmp_int[0]);
			gs.guild_id=tmp_int[0];
			if(gs.guild_id > 0 && gstorage_fromstr(line,&gs) == 0) {
				char_sql_saveitem(gs.storage,MAX_GUILD_STORAGE,gs.guild_id,TABLE_GUILD_STORAGE);
			}
			else{
				printf("int_storage: broken data [%s] line %d\n",guild_storage_txt,c);
			}
		}
		fclose(fp);
	}
	return 0;
}
