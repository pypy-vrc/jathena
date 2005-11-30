#ifndef _SCRIPT_H_
#define _SCRIPT_H_

struct script_data {
	int type;
	union {
		int num;
		char *str;
	} u;
};

struct script_state {
	struct script_stack {
		int sp,sp_max,defsp,new_defsp;
		struct script_data *stack_data;
	} *stack;
	int start,end;
	int pos,state;
	int rid,oid;
	char *script, *scriptroot;
	int  sleep;
};

#define SCRIPT_CONF_NAME	"conf/script_athena.conf"

unsigned char * parse_script(unsigned char *,int);
void run_script(unsigned char *,int,int,int);

struct dbt* script_get_label_db(void);
struct dbt* script_get_userfunc_db(void);

int script_config_read(char *cfgName);
int do_init_script(void);
int do_final_script(void);

extern char mapreg_txt[];

#endif

