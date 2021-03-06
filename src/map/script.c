//#define DEBUG_FUNCIN
//#define DEBUG_DISP

//コンバートされた中間言語を表示
//#define DEBUG_DISASM

// 実行過程を表示
//#define DEBUG_RUN

//使用した変数名一覧を表示
//#define DEBUG_VARS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifndef _WIN32
	#include <sys/time.h>
#endif
#include <time.h>
#include <setjmp.h>

#include "socket.h"
#include "timer.h"
#include "malloc.h"
#include "lock.h"

#include "mmo.h"

#include "map.h"
#include "guild.h"
#include "clif.h"
#include "chrif.h"
#include "itemdb.h"
#include "pc.h"
#include "script.h"
#include "storage.h"
#include "mob.h"
#include "npc.h"
#include "pet.h"
#include "intif.h"
#include "db.h"
#include "skill.h"
#include "chat.h"
#include "battle.h"
#include "party.h"
#include "atcommand.h"
#include "status.h"
#include "itemdb.h"
#include "unit.h"
#include "nullpo.h"
#include "homun.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

// for VC.NET 2005
#if _MSC_VER >= 1400
#pragma warning(disable : 4996)
#endif

#define SCRIPT_BLOCK_SIZE 512
enum { LABEL_NEXTLINE=1,LABEL_START };
static unsigned char * script_buf;
static int script_pos,script_size;

static char *str_buf;
static int str_pos,str_size;
static struct {
	int type;
	int str;
	int backpatch;
	int label;
	int (*func)(struct script_state *st);
	int val;
	int next;
} *str_data;
int str_num=LABEL_START,str_data_size;
int str_hash[16];

static struct dbt *mapreg_db=NULL;
static struct dbt *mapregstr_db=NULL;
static int mapreg_dirty=-1;
char mapreg_txt[256]="save/mapreg.txt";
#define MAPREG_AUTOSAVE_INTERVAL	(300*1000)

static struct dbt *scriptlabel_db=NULL;
static struct dbt *userfunc_db=NULL;
struct dbt* script_get_label_db(){ return scriptlabel_db; }
struct dbt* script_get_userfunc_db(){ if(!userfunc_db) userfunc_db=strdb_init(50); return userfunc_db; }

static int scriptlabel_final(void *k,void *d,va_list ap){ return 0; }
static char pos[11][100] = {"頭","体","左手","右手","ローブ","靴","アクセサリー1","アクセサリー2","頭2","頭3","装着していない"};

static struct Script_Config {
	int warn_func_no_comma;
	int warn_cmd_no_comma;
	int warn_func_mismatch_paramnum;
	int warn_cmd_mismatch_paramnum;
	int check_cmdcount;
	int check_gotocount;
} script_config;
static int parse_cmd;

// エラー処理
static jmp_buf error_jump;
static char*   error_msg;
static char*   error_pos;

// if , switch の実装
enum { TYPE_NULL = 0 , TYPE_IF , TYPE_SWITCH , TYPE_WHILE , TYPE_FOR , TYPE_DO , TYPE_USERFUNC};
static struct {
	struct {
		int type;
		int index;
		int count;
		int flag;
	} curly[256];		// 右カッコの情報
	int curly_count;	// 右カッコの数
	int index;			// スクリプト内で使用した構文の数
} syntax;
unsigned char* parse_curly_close(unsigned char *p);
unsigned char* parse_syntax_close(unsigned char *p);
unsigned char* parse_syntax_close_sub(unsigned char *p,int *flag);
unsigned char* parse_syntax(unsigned char *p);
static int parse_syntax_for_flag = 0;

#ifdef DEBUG_VARS
static int vars_count;
static struct dbt* vars_db;
struct vars_info {
	int use_count;
	char *file;
	int   line;
};
static int varsdb_final(void *key,void *data,va_list ap);
#endif

int get_com(unsigned char *script,int *pos);
int get_num(unsigned char *script,int *pos);

extern struct script_function {
	int (*func)(struct script_state *st);
	char *name;
	char *arg;
} buildin_func[];

static struct linkdb_node *sleep_db;

/*==========================================
 * ローカルプロトタイプ宣言 (必要な物のみ)
 *------------------------------------------
 */
unsigned char* parse_subexpr(unsigned char *,int);
void push_val(struct script_stack *stack,int type,int val);
int run_func(struct script_state *st);
#if !defined(NO_CSVDB) && !defined(NO_CSVDB_SCRIPT)
int script_csvinit( void );
int script_csvfinal( void );
#endif

int mapreg_setreg(int num,int val);
int mapreg_setregstr(int num,const char *str);

enum {
	C_NOP,C_POS,C_INT,C_PARAM,C_FUNC,C_STR,C_CONSTSTR,C_ARG,
	C_NAME,C_EOL, C_RETINFO,
	C_USERFUNC, C_USERFUNC_POS, // ユーザー定義関数群

	C_OP3,C_LOR,C_LAND,C_LE,C_LT,C_GE,C_GT,C_EQ,C_NE,   //operator
	C_XOR,C_OR,C_AND,C_ADD,C_SUB,C_MUL,C_DIV,C_MOD,C_NEG,C_LNOT,C_NOT,C_R_SHIFT,C_L_SHIFT
};

enum {
	MF_NOMEMO,
	MF_NOTELEPORT,
	MF_NOPORTAL,
	MF_NORETURN,
	MF_MONSTER_NOTELEPORT,
	MF_NOBRANCH,
	MF_NOPENALTY,
	MF_PVP_NOPARTY,
	MF_PVP_NOGUILD,
	MF_PVP_NOCALCRANK,
	MF_GVG_NOPARTY,
	MF_NOZENYPENALTY,
	MF_NOTRADE,
	MF_NOSKILL,
	MF_NOABRA,
	MF_NODROP,
	MF_NOICEWALL,
	MF_TURBO,
	MF_NOREVIVE
};

/*==========================================
 * 文字列のハッシュを計算
 *------------------------------------------
 */
static int calc_hash(const unsigned char *p)
{
	int h=0;
	while(*p){
		h=(h<<1)+(h>>3)+(h>>5)+(h>>8);
		h+=*p++;
	}
	return h&15;
}

/*==========================================
 * str_dataの中に名前があるか検索する
 *------------------------------------------
 */
// 既存のであれば番号、無ければ-1
static int search_str(const unsigned char *p)
{
	int i;
	i=str_hash[calc_hash(p)];
	while(i){
		if(strcmp(str_buf+str_data[i].str,p)==0){
			return i;
		}
		i=str_data[i].next;
	}
	return -1;
}

/*==========================================
 * str_dataに名前を登録
 *------------------------------------------
 */
// 既存のであれば番号、無ければ登録して新規番号
static int add_str(const unsigned char *p)
{
	int i;
	char *lowcase;

	lowcase=strdup(p);
	for(i=0;lowcase[i];i++)
		lowcase[i]=tolower(lowcase[i]);
	if((i=search_str(lowcase))>=0){
		free(lowcase);
		return i;
	}
	free(lowcase);

	i=calc_hash(p);
	if(str_hash[i]==0){
		str_hash[i]=str_num;
	} else {
		i=str_hash[i];
		for(;;){
			if(strcmp(str_buf+str_data[i].str,p)==0){
				return i;
			}
			if(str_data[i].next==0)
				break;
			i=str_data[i].next;
		}
		str_data[i].next=str_num;
	}
	if(str_num>=str_data_size){
		str_data_size+=128;
		str_data=aRealloc(str_data,sizeof(str_data[0])*str_data_size);
		memset(str_data + (str_data_size - 128), '\0', 128);
	}
	while(str_pos+(int)strlen(p)+1>=str_size){
		str_size+=256;
		str_buf=(char *)aRealloc(str_buf,str_size);
		memset(str_buf + (str_size - 256), '\0', 256);
	}
	strcpy(str_buf+str_pos,p);
	str_data[str_num].type=C_NOP;
	str_data[str_num].str=str_pos;
	str_data[str_num].next=0;
	str_data[str_num].func=NULL;
	str_data[str_num].backpatch=-1;
	str_data[str_num].label=-1;
	str_pos+=strlen(p)+1;
	return str_num++;
}


/*==========================================
 * スクリプトバッファサイズの確認と拡張
 *------------------------------------------
 */
static void check_script_buf(int size)
{
	if(script_pos+size>=script_size){
		script_size+=SCRIPT_BLOCK_SIZE;
		script_buf=(char *)aRealloc(script_buf,script_size);
		//memset(script_buf + script_size - SCRIPT_BLOCK_SIZE, '\0',
		//	SCRIPT_BLOCK_SIZE);
	}
}

/*==========================================
 * スクリプトバッファに１バイト書き込む
 *------------------------------------------
 */
 
#define add_scriptb(a) if( script_pos+1>=script_size ) check_script_buf(1); script_buf[script_pos++]=(a);

#if 0
static void add_scriptb(int a)
{
	check_script_buf(1);
	script_buf[script_pos++]=a;
}
#endif

/*==========================================
 * スクリプトバッファにデータタイプを書き込む
 *------------------------------------------
 */
static void add_scriptc(int a)
{
	while(a>=0x40){
		add_scriptb((a&0x3f)|0x40);
		a=(a-0x40)>>6;
	}
	add_scriptb(a&0x3f);
}

/*==========================================
 * スクリプトバッファに整数を書き込む
 *------------------------------------------
 */
static void add_scripti(int a)
{
	while(a>=0x40){
		add_scriptb(a|0xc0);
		a=(a-0x40)>>6;
	}
	add_scriptb(a|0x80);
}

/*==========================================
 * スクリプトバッファにラベル/変数/関数を書き込む
 *------------------------------------------
 */
// 最大16Mまで
static void add_scriptl(int l)
{
	int backpatch = str_data[l].backpatch;

	switch(str_data[l].type){
	case C_POS:
	case C_USERFUNC_POS:
		add_scriptc(C_POS);
		add_scriptb(str_data[l].label);
		add_scriptb(str_data[l].label>>8);
		add_scriptb(str_data[l].label>>16);
		break;
	case C_NOP:
	case C_USERFUNC:
		// ラベルの可能性があるのでbackpatch用データ埋め込み
		add_scriptc(C_NAME);
		str_data[l].backpatch=script_pos;
		add_scriptb(backpatch);
		add_scriptb(backpatch>>8);
		add_scriptb(backpatch>>16);
		break;
	case C_INT:
		add_scripti(str_data[l].val);
		break;
	default:
		// もう他の用途と確定してるので数字をそのまま
		add_scriptc(C_NAME);
		add_scriptb(l);
		add_scriptb(l>>8);
		add_scriptb(l>>16);
		break;
	}
}

/*==========================================
 * ラベルを解決する
 *------------------------------------------
 */
void set_label(int l,int pos)
{
	int i,next;

	str_data[l].type=(str_data[l].type == C_USERFUNC ? C_USERFUNC_POS : C_POS);
	str_data[l].label=pos;
	for(i=str_data[l].backpatch;i>=0 && i!=0x00ffffff;){
		next=(*(int*)(script_buf+i)) & 0x00ffffff;
		script_buf[i-1]=(str_data[l].type == C_USERFUNC ? C_USERFUNC_POS : C_POS);
		script_buf[i]=pos;
		script_buf[i+1]=pos>>8;
		script_buf[i+2]=pos>>16;
		i=next;
	}
	// printf("set_label pos:%d str:%s\n",pos,&str_buf[str_data[l].str]);
}

/*==========================================
 * スペース/コメント読み飛ばし
 *------------------------------------------
 */
static unsigned char *skip_space(unsigned char *p)
{
	while(1){
		while(isspace(*p))
			p++;
		if(p[0]=='/' && p[1]=='/'){
			while(*p && *p!='\n')
				p++;
		} else if(p[0]=='/' && p[1]=='*'){
			p++;
			while(*p && (p[-1]!='*' || p[0]!='/'))
				p++;
			if(*p) p++;
		} else
			break;
	}
	return p;
}

/*==========================================
 * １単語スキップ
 *------------------------------------------
 */
static unsigned char *skip_word(unsigned char *p)
{
	// prefix
	if(*p=='\'')p++;	// スクリプト依存変数
	if(*p=='$') p++;	// MAP鯖内共有変数用
	if(*p=='@') p++;	// 一時的変数用(like weiss)
	if(*p=='#') p++;	// account変数用
	if(*p=='#') p++;	// ワールドaccount変数用

	while(isalnum(*p)||*p=='_'|| *p>=0x81)
		if(*p>=0x81 && p[1]){
			p+=2;
		} else
			p++;

	// postfix
	if(*p=='$') p++;	// 文字列変数

	return p;
}

/*==========================================
 * エラーメッセージ出力
 *------------------------------------------
 */
static void disp_error_message(const char *mes,unsigned char *pos)
{
	error_msg = aStrdup(mes);
	error_pos = pos;
	longjmp( error_jump, 1 );
}

/*==========================================
 * 項の解析
 *------------------------------------------
 */
unsigned char* parse_simpleexpr(unsigned char *p)
{
	int i;
	p=skip_space(p);

#ifdef DEBUG_FUNCIN
	if(battle_config.etc_log)
		printf("parse_simpleexpr %s\n",p);
#endif
	if(*p==';' || *p==','){
		disp_error_message("unexpected expr end",p);
		exit(1);
	}
	if(*p=='('){
		p=parse_subexpr(p+1,-1);
		p=skip_space(p);
		if((*p++)!=')'){
			disp_error_message("unmatch ')'",p);
			exit(1);
		}
	} else if(isdigit(*p) || ((*p=='-' || *p=='+') && isdigit(p[1]))){
		char *np;
		i=strtoul(p,&np,0);
		add_scripti(i);
		p=np;
	} else if(*p=='"'){
		add_scriptc(C_STR);
		p++;
		while(*p && *p!='"'){
			if(p[-1]<=0x7e && *p=='\\')
				p++;
			else if(*p=='\n'){
				disp_error_message("unexpected newline @ string",p);
				exit(1);
			}
			add_scriptb(*p++);
		}
		if(!*p){
			disp_error_message("unexpected eof @ string",p);
			exit(1);
		}
		add_scriptb(0);
		p++;	//'"'
	} else {
		int c,l;
		char *p2;
		// label , register , function etc
		if(skip_word(p)==p){
			disp_error_message("unexpected charactor",p);
			exit(1);
		}
		p2=skip_word(p);
		c=*p2;	*p2=0;	// 名前をadd_strする
		l=add_str(p);

		parse_cmd=l;	// warn_*_mismatch_paramnumのために必要
		// 廃止予定のl14/l15,およびプレフィックスｌの警告
		if(	strcmp(str_buf+str_data[l].str,"l14")==0 ||
			strcmp(str_buf+str_data[l].str,"l15")==0 ){
			disp_error_message("l14 and l15 is DEPRECATED. use @menu instead of l15.",p);
		}else if(str_buf[str_data[l].str]=='l'){
			disp_error_message("prefix 'l' is DEPRECATED. use prefix '@' instead.",p2);
		}

		*p2=c;	p=p2;

		if(str_data[l].type!=C_FUNC && c=='['){
			// array(name[i] => getelementofarray(name,i) )
			add_scriptl(search_str("getelementofarray"));
			add_scriptc(C_ARG);
			add_scriptl(l);
			p=parse_subexpr(p+1,-1);
			p=skip_space(p);
			if((*p++)!=']'){
				disp_error_message("unmatch ']'",p);
				exit(1);
			}
			add_scriptc(C_FUNC);
		} else if(str_data[l].type == C_USERFUNC || str_data[l].type == C_USERFUNC_POS) {
			add_scriptl(search_str("callsub"));
			add_scriptc(C_ARG);
			add_scriptl(l);
		} else {
			add_scriptl(l);
		}
	}

#ifdef DEBUG_FUNCIN
	if(battle_config.etc_log)
		printf("parse_simpleexpr end %s\n",p);
#endif
	return p;
}

/*==========================================
 * 式の解析
 *------------------------------------------
 */
unsigned char* parse_subexpr(unsigned char *p,int limit)
{
	int op,opl,len;
	char *tmpp;

#ifdef DEBUG_FUNCIN
	if(battle_config.etc_log)
		printf("parse_subexpr %s\n",p);
#endif
	p=skip_space(p);

	if(*p=='-'){
		tmpp=skip_space(p+1);
		if(*tmpp==';' || *tmpp==','){
			add_scriptl(LABEL_NEXTLINE);
			p++;
			return p;
		}
	}
	tmpp=p;
	if((op=C_NEG,*p=='-') || (op=C_LNOT,*p=='!') || (op=C_NOT,*p=='~')){
		p=parse_subexpr(p+1,10);
		add_scriptc(op);
	} else
		p=parse_simpleexpr(p);
	p=skip_space(p);
	while(((op=C_OP3,opl=0,len=1,*p=='?') ||
		   (op=C_ADD,opl=8,len=1,*p=='+') ||
		   (op=C_SUB,opl=8,len=1,*p=='-') ||
		   (op=C_MUL,opl=9,len=1,*p=='*') ||
		   (op=C_DIV,opl=9,len=1,*p=='/') ||
		   (op=C_MOD,opl=9,len=1,*p=='%') ||
		   (op=C_FUNC,opl=11,len=1,*p=='(') ||
		   (op=C_LAND,opl=2,len=2,*p=='&' && p[1]=='&') ||
		   (op=C_AND,opl=6,len=1,*p=='&') ||
		   (op=C_LOR,opl=1,len=2,*p=='|' && p[1]=='|') ||
		   (op=C_OR,opl=5,len=1,*p=='|') ||
		   (op=C_XOR,opl=4,len=1,*p=='^') ||
		   (op=C_EQ,opl=3,len=2,*p=='=' && p[1]=='=') ||
		   (op=C_NE,opl=3,len=2,*p=='!' && p[1]=='=') ||
		   (op=C_R_SHIFT,opl=7,len=2,*p=='>' && p[1]=='>') ||
		   (op=C_GE,opl=3,len=2,*p=='>' && p[1]=='=') ||
		   (op=C_GT,opl=3,len=1,*p=='>') ||
		   (op=C_L_SHIFT,opl=7,len=2,*p=='<' && p[1]=='<') ||
		   (op=C_LE,opl=3,len=2,*p=='<' && p[1]=='=') ||
		   (op=C_LT,opl=3,len=1,*p=='<')) && opl>limit){
		p+=len;
		if(op==C_FUNC){
			int i=0,func;
			char *plist[128];

			if(str_data[parse_cmd].type == C_FUNC){
				// 通常の関数
				add_scriptc(C_ARG);
			} else if(str_data[parse_cmd].type == C_USERFUNC || str_data[parse_cmd].type == C_USERFUNC_POS) {
				// ユーザー定義関数呼び出し
				parse_cmd = search_str("callsub");
				i++;
			} else {
				disp_error_message(
					"expect command, missing function name or calling undeclared function",p
				);
				exit(0);
			}
			func=parse_cmd;

			while(*p && *p!=')' && i<128) {
				plist[i]=p;
				p=parse_subexpr(p,-1);
				p=skip_space(p);
				if(*p==',') p++;
				else if(*p!=')' && script_config.warn_func_no_comma){
					disp_error_message("expect ',' or ')' at func params",p);
				}
				p=skip_space(p);
				i++;
			};
			plist[i]=p;
			if(*(p++)!=')'){
				disp_error_message("func request '(' ')'",p);
				exit(1);
			}

			if( str_data[func].type==C_FUNC && script_config.warn_func_mismatch_paramnum){
				char *arg=buildin_func[str_data[func].val].arg;
				int j=0;
				for(j=0;arg[j];j++) if(arg[j]=='*')break;
				if( (arg[j]==0 && i!=j) || (arg[j]=='*' && i<j) ){
					disp_error_message("illeagal number of parameters",plist[(i<j)?i:j]);
				}
			}
		} else if(op == C_OP3) {
			p=parse_subexpr(p,-1);
			p=skip_space(p);
			if( *(p++) != ':')
				disp_error_message("need ':'", p);
			p=parse_subexpr(p,-1);
		} else {
			p=parse_subexpr(p,opl);
		}
		add_scriptc(op);
		p=skip_space(p);
	}
#ifdef DEBUG_FUNCIN
	if(battle_config.etc_log)
		printf("parse_subexpr end %s\n",p);
#endif
	return p;  /* return first untreated operator */
}

/*==========================================
 * 式の評価
 *------------------------------------------
 */
unsigned char* parse_expr(unsigned char *p)
{
#ifdef DEBUG_FUNCIN
	if(battle_config.etc_log)
		printf("parse_expr %s\n",p);
#endif
	switch(*p){
	case ')': case ';': case ':': case '[': case ']':
	case '}':
		disp_error_message("unexpected char",p);
		exit(1);
	}
	if(*p == '(') {
		unsigned char *p2 = skip_space(p + 1);
		if(*p2 == ')') {
			return p2 + 1;
		}
	}
	p=parse_subexpr(p,-1);
#ifdef DEBUG_FUNCIN
	if(battle_config.etc_log)
		printf("parse_expr end %s\n",p);
#endif
	return p;
}

/*==========================================
 * 行の解析
 *------------------------------------------
 */
unsigned char* parse_line(unsigned char *p)
{
	int i=0,cmd;
	char *plist[128];
	char *p2;
	char end;

	p=skip_space(p);
	if(*p==';')
		return p+1;

	p = skip_space(p);
	if(p[0] == '{') {
		syntax.curly[syntax.curly_count].type  = TYPE_NULL;
		syntax.curly[syntax.curly_count].count = -1;
		syntax.curly[syntax.curly_count].index = -1;
		syntax.curly_count++;
		return p + 1;
	} else if(p[0] == '}') {
		return parse_curly_close(p);
	}

	// 構文関連の処理
	p2 = parse_syntax(p);
	if(p2 != NULL) { return p2; }

	// 最初は関数名
	p2=p;
	p=parse_simpleexpr(p);
	p=skip_space(p);

	if(str_data[parse_cmd].type == C_FUNC){
		// 通常の関数
		add_scriptc(C_ARG);
	} else if(str_data[parse_cmd].type == C_USERFUNC || str_data[parse_cmd].type == C_USERFUNC_POS) {
		// ユーザー定義関数呼び出し
		parse_cmd = search_str("callsub");
		i++;
	} else {
		disp_error_message(
			"expect command, missing function name or calling undeclared function",p2
		);
		exit(0);
	}
	cmd=parse_cmd;

	if(parse_syntax_for_flag) {
		end = ')';
	} else {
		end = ';';
	}
	while(p && *p && *p != end && i<128){
		plist[i]=p;

		p=parse_expr(p);
		p=skip_space(p);
		// 引数区切りの,処理
		if(*p==',') p++;
		else if(*p!=end && script_config.warn_cmd_no_comma){
			if(parse_syntax_for_flag) {
				disp_error_message("expect ',' or ')' at cmd params",p);
			} else {
				disp_error_message("expect ',' or ';' at cmd params",p);
			}
		}
		p=skip_space(p);
		i++;
	}
	plist[i]=p;
	if(!p || *(p++)!=end){
		if(parse_syntax_for_flag) {
			disp_error_message("need ')'",p);
		} else {
			disp_error_message("need ';'",p);
		}
		exit(1);
	}
	add_scriptc(C_FUNC);

	// if, for , while の閉じ判定
	p = parse_syntax_close(p);

	if( str_data[cmd].type==C_FUNC && script_config.warn_cmd_mismatch_paramnum){
		const char *arg=buildin_func[str_data[cmd].val].arg;
		int j=0;
		for(j=0;arg[j];j++) if(arg[j]=='*')break;
		if( (arg[j]==0 && i!=j) || (arg[j]=='*' && i<j) ){
			disp_error_message("illeagal number of parameters",plist[(i<j)?i:j]);
		}
	}
	return p;
}

// { ... } の閉じ処理
unsigned char* parse_curly_close(unsigned char *p) {
	if(syntax.curly_count <= 0) {
		disp_error_message("unexpected string",p);
		return p + 1;
	} else if(syntax.curly[syntax.curly_count-1].type == TYPE_NULL) {
		syntax.curly_count--;
		// if, for , while の閉じ判定
		p = parse_syntax_close(p + 1);
		return p;
	} else if(syntax.curly[syntax.curly_count-1].type == TYPE_SWITCH) {
		// switch() 閉じ判定
		int pos = syntax.curly_count-1;
		char label[256];
		int l;
		// 一時変数を消す
		sprintf(label,"set $@__SW%x_VAL,0;",syntax.curly[pos].index);
		syntax.curly[syntax.curly_count++].type = TYPE_NULL;
		parse_line(label);
		syntax.curly_count--;

		// 無条件で終了ポインタに移動
		sprintf(label,"goto __SW%x_FIN;",syntax.curly[pos].index);
		syntax.curly[syntax.curly_count++].type = TYPE_NULL;
		parse_line(label);
		syntax.curly_count--;

		// 現在地のラベルを付ける
		sprintf(label,"__SW%x_%x",syntax.curly[pos].index,syntax.curly[pos].count);
		l=add_str(label);
		if(str_data[l].label!=-1){
			disp_error_message("dup label ",p);
			exit(1);
		}
		set_label(l,script_pos);

		if(syntax.curly[pos].flag) {
			// default が存在する
			sprintf(label,"goto __SW%x_DEF;",syntax.curly[pos].index);
			syntax.curly[syntax.curly_count++].type = TYPE_NULL;
			parse_line(label);
			syntax.curly_count--;
		}

		// 終了ラベルを付ける
		sprintf(label,"__SW%x_FIN",syntax.curly[pos].index);
		l=add_str(label);
		if(str_data[l].label!=-1){
			disp_error_message("dup label ",p);
			exit(1);
		}
		set_label(l,script_pos);

		syntax.curly_count--;
		return p+1;
	} else {
		disp_error_message("unexpected string",p);
		return p + 1;
	}
}

// 構文関連の処理
//	 break, case, continue, default, do, for, function,
//	 if, switch, while をこの内部で処理します。
unsigned char* parse_syntax(unsigned char *p) {
	switch(p[0]) {
	case 'b':
		if(!strncmp(p,"break",5) && !isalpha(*(p + 5))) {
			// break の処理
			char label[256];
			int pos = syntax.curly_count - 1;
			while(pos >= 0) {
				if(syntax.curly[pos].type == TYPE_DO) {
					sprintf(label,"goto __DO%x_FIN;",syntax.curly[pos].index);
					break;
				} else if(syntax.curly[pos].type == TYPE_FOR) {
					sprintf(label,"goto __FR%x_FIN;",syntax.curly[pos].index);
					break;
				} else if(syntax.curly[pos].type == TYPE_WHILE) {
					sprintf(label,"goto __WL%x_FIN;",syntax.curly[pos].index);
					break;
				} else if(syntax.curly[pos].type == TYPE_SWITCH) {
					sprintf(label,"goto __SW%x_FIN;",syntax.curly[pos].index);
					break;
				}
				pos--;
			}
			if(pos < 0) {
				disp_error_message("unexpected 'break'",p);
			} else {
				syntax.curly[syntax.curly_count++].type = TYPE_NULL;
				parse_line(label);
				syntax.curly_count--;
			}
			p = skip_word(p);
			p++;
			// if, for , while の閉じ判定
			p = parse_syntax_close(p + 1);
			return p;
		}
		break;
	case 'c':
		if(!strncmp(p,"case",4) && !isalpha(*(p + 4))) {
			// case の処理
			if(syntax.curly_count <= 0 || syntax.curly[syntax.curly_count - 1].type != TYPE_SWITCH) {
				disp_error_message("unexpected 'case' ",p);
				return p+1;
			} else {
				char *p2;
				char label[256];
				int  l;
				int pos = syntax.curly_count-1;
				if(syntax.curly[pos].count != 1) {
					// FALLTHRU 用のジャンプ
					sprintf(label,"goto __SW%x_%xJ;",syntax.curly[pos].index,syntax.curly[pos].count);
					syntax.curly[syntax.curly_count++].type = TYPE_NULL;
					parse_line(label);
					syntax.curly_count--;

					// 現在地のラベルを付ける
					sprintf(label,"__SW%x_%x",syntax.curly[pos].index,syntax.curly[pos].count);
					l=add_str(label);
					if(str_data[l].label!=-1){
						disp_error_message("dup label ",p);
						exit(1);
					}
					set_label(l,script_pos);
				}
				// switch 判定文
				p = skip_word(p);
				p = skip_space(p);
				p2 = p;
				p = skip_word(p);
				p = skip_space(p);
				if(*p != ':') {
					disp_error_message("expect ':'",p);
					exit(1);
				}
				*p = 0;
				sprintf(label,"if(%s != $@__SW%x_VAL) goto __SW%x_%x;",
					p2,syntax.curly[pos].index,syntax.curly[pos].index,syntax.curly[pos].count+1);
				syntax.curly[syntax.curly_count++].type = TYPE_NULL;
				*p = ':';
				// ２回parse しないとダメ
				p2 = parse_line(label);
				parse_line(p2);
				syntax.curly_count--;
				if(syntax.curly[pos].count != 1) {
					// FALLTHRU 終了後のラベル
					sprintf(label,"__SW%x_%xJ",syntax.curly[pos].index,syntax.curly[pos].count);
					l=add_str(label);
					if(str_data[l].label!=-1){
						disp_error_message("dup label ",p);
						exit(1);
					}
					set_label(l,script_pos);
				}
				// 一時変数を消す
				sprintf(label,"set $@__SW%x_VAL,0;",syntax.curly[pos].index);
				syntax.curly[syntax.curly_count++].type = TYPE_NULL;
				parse_line(label);
				syntax.curly_count--;
				syntax.curly[pos].count++;
			}
			return p + 1;
		} else if(!strncmp(p,"continue",8) && !isalpha(*(p + 8))) {
			// continue の処理
			char label[256];
			int pos = syntax.curly_count - 1;
			while(pos >= 0) {
				if(syntax.curly[pos].type == TYPE_DO) {
					sprintf(label,"goto __DO%x_NXT;",syntax.curly[pos].index);
					syntax.curly[pos].flag = 1; // continue 用のリンク張るフラグ
					break;
				} else if(syntax.curly[pos].type == TYPE_FOR) {
					sprintf(label,"goto __FR%x_NXT;",syntax.curly[pos].index);
					break;
				} else if(syntax.curly[pos].type == TYPE_WHILE) {
					sprintf(label,"goto __WL%x_NXT;",syntax.curly[pos].index);
					break;
				}
				pos--;
			}
			if(pos < 0) {
				disp_error_message("unexpected 'continue'",p);
			} else {
				syntax.curly[syntax.curly_count++].type = TYPE_NULL;
				parse_line(label);
				syntax.curly_count--;
			}
			p = skip_word(p);
			p++;
			// if, for , while の閉じ判定
			p = parse_syntax_close(p + 1);
			return p;
		}
		break;
	case 'd':
		if(!strncmp(p,"default",7) && !isalpha(*(p + 7))) {
			// switch - default の処理
			if(syntax.curly_count <= 0 || syntax.curly[syntax.curly_count - 1].type != TYPE_SWITCH) {
				disp_error_message("unexpected 'default'",p);
				return p+1;
			} else if(syntax.curly[syntax.curly_count - 1].flag) {
				disp_error_message("dup 'default'",p);
				return p+1;
			} else {
				char label[256];
				int l;
				int pos = syntax.curly_count-1;
				// 現在地のラベルを付ける
				p = skip_word(p);
				p = skip_space(p);
				if(*p != ':') {
					disp_error_message("need ':'",p);
				}
				p++;
				sprintf(label,"__SW%x_%x",syntax.curly[pos].index,syntax.curly[pos].count);
				l=add_str(label);
				if(str_data[l].label!=-1){
					disp_error_message("dup label ",p);
					exit(1);
				}
				set_label(l,script_pos);

				// 無条件で次のリンクに飛ばす
				sprintf(label,"goto __SW%x_%x;",syntax.curly[pos].index,syntax.curly[pos].count+1);
				syntax.curly[syntax.curly_count++].type = TYPE_NULL;
				parse_line(label);
				syntax.curly_count--;

				// default のラベルを付ける
				sprintf(label,"__SW%x_DEF",syntax.curly[pos].index);
				l=add_str(label);
				if(str_data[l].label!=-1){
					disp_error_message("dup label ",p);
					exit(1);
				}
				set_label(l,script_pos);

				syntax.curly[syntax.curly_count - 1].flag = 1;
				syntax.curly[pos].count++;

				p = skip_word(p);
				return p + 1;
			}
		} else if(!strncmp(p,"do",2) && !isalpha(*(p + 2))) {
			int l;
			char label[256];
			p=skip_word(p);
			p=skip_space(p);

			syntax.curly[syntax.curly_count].type  = TYPE_DO;
			syntax.curly[syntax.curly_count].count = 1;
			syntax.curly[syntax.curly_count].index = syntax.index++;
			syntax.curly[syntax.curly_count].flag  = 0;
			// 現在地のラベル形成する
			sprintf(label,"__DO%x_BGN",syntax.curly[syntax.curly_count].index);
			l=add_str(label);
			if(str_data[l].label!=-1){
				disp_error_message("dup label ",p);
				exit(1);
			}
			set_label(l,script_pos);
			syntax.curly_count++;
			return p;
		}
		break;
	case 'f':
		if(!strncmp(p,"for",3) && !isalpha(*(p + 3))) {
			int l;
			char label[256];
			int  pos = syntax.curly_count;
			syntax.curly[syntax.curly_count].type  = TYPE_FOR;
			syntax.curly[syntax.curly_count].count = 1;
			syntax.curly[syntax.curly_count].index = syntax.index++;
			syntax.curly[syntax.curly_count].flag  = 0;
			syntax.curly_count++;

			p=skip_word(p);
			p=skip_space(p);

			if(*p != '(') {
				disp_error_message("need '('",p);
				return p+1;
			}
			p++;

			// 初期化文を実行する
			syntax.curly[syntax.curly_count++].type = TYPE_NULL;
			p=parse_line(p);
			syntax.curly_count--;

			// 条件判断開始のラベル形成する
			sprintf(label,"__FR%x_J",syntax.curly[pos].index);
			l=add_str(label);
			if(str_data[l].label!=-1){
				disp_error_message("dup label ",p);
				exit(1);
			}
			set_label(l,script_pos);

			if(*p == ';') {
				// for(;;) のパターンなので必ず真
				;
			} else {
				// 条件が偽なら終了地点に飛ばす
				sprintf(label,"__FR%x_FIN",syntax.curly[pos].index);
				add_scriptl(add_str("jump_zero"));
				add_scriptc(C_ARG);
				p=parse_expr(p);
				p=skip_space(p);
				add_scriptl(add_str(label));
				add_scriptc(C_FUNC);
			}
			if(*p != ';') {
				disp_error_message("need ';'",p);
				return p+1;
			}
			p++;

			// ループ開始に飛ばす
			sprintf(label,"goto __FR%x_BGN;",syntax.curly[pos].index);
			syntax.curly[syntax.curly_count++].type = TYPE_NULL;
			parse_line(label);
			syntax.curly_count--;

			// 次のループへのラベル形成する
			sprintf(label,"__FR%x_NXT",syntax.curly[pos].index);
			l=add_str(label);
			if(str_data[l].label!=-1){
				disp_error_message("dup label ",p);
				exit(1);
			}
			set_label(l,script_pos);

			// 次のループに入る時の処理
			// for 最後の '(' を ';' として扱うフラグ
			parse_syntax_for_flag = 1;
			syntax.curly[syntax.curly_count++].type = TYPE_NULL;
			p=parse_line(p);
			syntax.curly_count--;
			parse_syntax_for_flag = 0;

			// 条件判定処理に飛ばす
			sprintf(label,"goto __FR%x_J;",syntax.curly[pos].index);
			syntax.curly[syntax.curly_count++].type = TYPE_NULL;
			parse_line(label);
			syntax.curly_count--;

			// ループ開始のラベル付け
			sprintf(label,"__FR%x_BGN",syntax.curly[pos].index);
			l=add_str(label);
			if(str_data[l].label!=-1){
				disp_error_message("dup label ",p);
				exit(1);
			}
			set_label(l,script_pos);
			return p;
		} else if(!strncmp(p,"function",8) && !isalpha(*(p + 8))) {
			unsigned char *func_name;
			// function
			p=skip_word(p);
			p=skip_space(p);
			// function - name
			func_name = p;
			p=skip_word(p);
			if(*skip_space(p) == ';') {
				// 関数の宣言 - 名前を登録して終わり
				unsigned char c = *p;
				int l;
				*p = 0;
				l=add_str(func_name);
				*p = c;
				if(str_data[l].type == C_NOP) {
					str_data[l].type = C_USERFUNC;
				}
				return skip_space(p) + 1;
			} else {
				// 関数の中身
				char label[256];
				unsigned char c = *p;
				int l;
				syntax.curly[syntax.curly_count].type  = TYPE_USERFUNC;
				syntax.curly[syntax.curly_count].count = 1;
				syntax.curly[syntax.curly_count].index = syntax.index++;
				syntax.curly[syntax.curly_count].flag  = 0;
				syntax.curly_count++;

				// 関数終了まで飛ばす
				sprintf(label,"goto __FN%x_FIN;",syntax.curly[syntax.curly_count-1].index);
				syntax.curly[syntax.curly_count++].type = TYPE_NULL;
				parse_line(label);
				syntax.curly_count--;

				// 関数名のラベルを付ける
				*p = 0;
				l=add_str(func_name);
				if(str_data[l].type == C_NOP) {
					str_data[l].type = C_USERFUNC;
				}
				if(str_data[l].label!=-1){
					*p=c;
					disp_error_message("dup label ",p);
					exit(1);
				}
				set_label(l,script_pos);
				strdb_insert(scriptlabel_db,func_name,script_pos);	// 外部用label db登録
				*p = c;
				return skip_space(p);
			}
		}
		break;
	case 'i':
		if(!strncmp(p,"if",2) && !isalpha(*(p + 2))) {
			// if() の処理
			char label[256];
			p=skip_word(p);
			p=skip_space(p);

			syntax.curly[syntax.curly_count].type  = TYPE_IF;
			syntax.curly[syntax.curly_count].count = 1;
			syntax.curly[syntax.curly_count].index = syntax.index++;
			syntax.curly[syntax.curly_count].flag  = 0;
			sprintf(label,"__IF%x_%x",syntax.curly[syntax.curly_count].index,syntax.curly[syntax.curly_count].count);
			syntax.curly_count++;
			add_scriptl(add_str("jump_zero"));
			add_scriptc(C_ARG);
			p=parse_expr(p);
			p=skip_space(p);
			add_scriptl(add_str(label));
			add_scriptc(C_FUNC);
			return p;
		}
		break;
	case 's':
		if(!strncmp(p,"switch",6) && !isalpha(*(p + 6))) {
			// switch() の処理
			char label[256];
			syntax.curly[syntax.curly_count].type  = TYPE_SWITCH;
			syntax.curly[syntax.curly_count].count = 1;
			syntax.curly[syntax.curly_count].index = syntax.index++;
			syntax.curly[syntax.curly_count].flag  = 0;
			sprintf(label,"$@__SW%x_VAL",syntax.curly[syntax.curly_count].index);
			syntax.curly_count++;
			add_scriptl(add_str("set"));
			add_scriptc(C_ARG);
			add_scriptl(add_str(label));
			p=skip_word(p);
			p=skip_space(p);
			p=parse_expr(p);
			p=skip_space(p);
			if(*p != '{') {
				disp_error_message("need '{'",p);
			}
			add_scriptc(C_FUNC);
			return p + 1;
		}
		break;
	case 'w':
		if(!strncmp(p,"while",5) && !isalpha(*(p + 5))) {
			int l;
			char label[256];
			p=skip_word(p);
			p=skip_space(p);

			syntax.curly[syntax.curly_count].type  = TYPE_WHILE;
			syntax.curly[syntax.curly_count].count = 1;
			syntax.curly[syntax.curly_count].index = syntax.index++;
			syntax.curly[syntax.curly_count].flag  = 0;
			// 条件判断開始のラベル形成する
			sprintf(label,"__WL%x_NXT",syntax.curly[syntax.curly_count].index);
			l=add_str(label);
			if(str_data[l].label!=-1){
				disp_error_message("dup label ",p);
				exit(1);
			}
			set_label(l,script_pos);

			// 条件が偽なら終了地点に飛ばす
			sprintf(label,"__WL%x_FIN",syntax.curly[syntax.curly_count].index);
			add_scriptl(add_str("jump_zero"));
			add_scriptc(C_ARG);
			p=parse_expr(p);
			p=skip_space(p);
			add_scriptl(add_str(label));
			add_scriptc(C_FUNC);
			syntax.curly_count++;
			return p;
		}
		break;
	}
	return NULL;
}

unsigned char* parse_syntax_close(unsigned char *p) {
	// if(...) for(...) hoge(); のように、１度閉じられたら再度閉じられるか確認する
	int flag;

	do {
		p = parse_syntax_close_sub(p,&flag);
	} while(flag);
	return p;
}

// if, for , while , do の閉じ判定
//	 flag == 1 : 閉じられた
//	 flag == 0 : 閉じられない
unsigned char* parse_syntax_close_sub(unsigned char *p,int *flag) {
	char label[256];
	int pos = syntax.curly_count - 1;
	int l;
	*flag = 1;

	if(syntax.curly_count <= 0) {
		*flag = 0;
		return p;
	} else if(syntax.curly[pos].type == TYPE_IF) {
		char *p2 = p;
		// if 最終場所へ飛ばす
		sprintf(label,"goto __IF%x_FIN;",syntax.curly[pos].index);
		syntax.curly[syntax.curly_count++].type = TYPE_NULL;
		parse_line(label);
		syntax.curly_count--;

		// 現在地のラベルを付ける
		sprintf(label,"__IF%x_%x",syntax.curly[pos].index,syntax.curly[pos].count);
		l=add_str(label);
		if(str_data[l].label!=-1){
			disp_error_message("dup label ",p);
			exit(1);
		}
		set_label(l,script_pos);

		syntax.curly[pos].count++;
		p = skip_space(p);
		if(!syntax.curly[pos].flag && !strncmp(p,"else",4) && !isalpha(*(p + 4))) {
			// else  or else - if
			p = skip_word(p);
			p = skip_space(p);
			if(!strncmp(p,"if",2) && !isalpha(*(p + 2))) {
				// else - if
				p=skip_word(p);
				p=skip_space(p);
				sprintf(label,"__IF%x_%x",syntax.curly[pos].index,syntax.curly[pos].count);
				add_scriptl(add_str("jump_zero"));
				add_scriptc(C_ARG);
				p=parse_expr(p);
				p=skip_space(p);
				add_scriptl(add_str(label));
				add_scriptc(C_FUNC);
				*flag = 0;
				return p;
			} else {
				// else
				if(!syntax.curly[pos].flag) {
					syntax.curly[pos].flag = 1;
					*flag = 0;
					return p;
				}
			}
		}
		// if 閉じ
		syntax.curly_count--;
		// 最終地のラベルを付ける
		sprintf(label,"__IF%x_FIN",syntax.curly[pos].index);
		l=add_str(label);
		if(str_data[l].label!=-1){
			disp_error_message("dup label ",p);
			exit(1);
		}
		set_label(l,script_pos);
		if(syntax.curly[pos].flag == 1) {
			// このifに対するelseじゃないのでポインタの位置は同じ
			return p2;
		}
		return p;
	} else if(syntax.curly[pos].type == TYPE_DO) {
		int l;
		char label[256];
		unsigned char *p2;

		if(syntax.curly[pos].flag) {
			// 現在地のラベル形成する(continue でここに来る)
			sprintf(label,"__DO%x_NXT",syntax.curly[pos].index);
			l=add_str(label);
			if(str_data[l].label!=-1){
				disp_error_message("dup label ",p);
				exit(1);
			}
			set_label(l,script_pos);
		}

		// 条件が偽なら終了地点に飛ばす
		p = skip_space(p);
		p2 = skip_word(p);
		if(p2 - p != 5 || strncmp("while",p,5)) {
			disp_error_message("need 'while'",p);
		}
		p = p2;

		sprintf(label,"__DO%x_FIN",syntax.curly[pos].index);
		add_scriptl(add_str("jump_zero"));
		add_scriptc(C_ARG);
		p=parse_expr(p);
		p=skip_space(p);
		add_scriptl(add_str(label));
		add_scriptc(C_FUNC);

		// 開始地点に飛ばす
		sprintf(label,"goto __DO%x_BGN;",syntax.curly[pos].index);
		syntax.curly[syntax.curly_count++].type = TYPE_NULL;
		parse_line(label);
		syntax.curly_count--;

		// 条件終了地点のラベル形成する
		sprintf(label,"__DO%x_FIN",syntax.curly[pos].index);
		l=add_str(label);
		if(str_data[l].label!=-1){
			disp_error_message("dup label ",p);
			exit(1);
		}
		set_label(l,script_pos);
		p = skip_space(p);
		if(*p != ';') {
			disp_error_message("need ';'",p);
			return p+1;
		}
		p++;
		syntax.curly_count--;
		return p;
	} else if(syntax.curly[pos].type == TYPE_FOR) {
		// 次のループに飛ばす
		sprintf(label,"goto __FR%x_NXT;",syntax.curly[pos].index);
		syntax.curly[syntax.curly_count++].type = TYPE_NULL;
		parse_line(label);
		syntax.curly_count--;

		// for 終了のラベル付け
		sprintf(label,"__FR%x_FIN",syntax.curly[pos].index);
		l=add_str(label);
		if(str_data[l].label!=-1){
			disp_error_message("dup label ",p);
			exit(1);
		}
		set_label(l,script_pos);
		syntax.curly_count--;
		return p;
	} else if(syntax.curly[pos].type == TYPE_WHILE) {
		// while 条件判断へ飛ばす
		sprintf(label,"goto __WL%x_NXT;",syntax.curly[pos].index);
		syntax.curly[syntax.curly_count++].type = TYPE_NULL;
		parse_line(label);
		syntax.curly_count--;

		// while 終了のラベル付け
		sprintf(label,"__WL%x_FIN",syntax.curly[pos].index);
		l=add_str(label);
		if(str_data[l].label!=-1){
			disp_error_message("dup label ",p);
			exit(1);
		}
		set_label(l,script_pos);
		syntax.curly_count--;
		return p;
	} else if(syntax.curly[syntax.curly_count-1].type == TYPE_USERFUNC) {
		int pos = syntax.curly_count-1;
		char label[256];
		int l;
		// 戻す
		sprintf(label,"return;");
		syntax.curly[syntax.curly_count++].type = TYPE_NULL;
		parse_line(label);
		syntax.curly_count--;

		// 現在地のラベルを付ける
		sprintf(label,"__FN%x_FIN",syntax.curly[pos].index);
		l=add_str(label);
		if(str_data[l].label!=-1){
			disp_error_message("dup label ",p);
			exit(1);
		}
		set_label(l,script_pos);
		syntax.curly_count--;
		return p + 1;
	} else {
		*flag = 0;
		return p;
	}
}

/*==========================================
 * 組み込み関数の追加
 *------------------------------------------
 */
static void add_buildin_func(void)
{
	int i,n;
	for(i=0;buildin_func[i].func;i++){
		n=add_str(buildin_func[i].name);
		str_data[n].type=C_FUNC;
		str_data[n].val=i;
		str_data[n].func=buildin_func[i].func;
	}
}

/*==========================================
 * 定数データベースの読み込み
 *------------------------------------------
 */
static void read_constdb(void)
{
	FILE *fp;
	char line[1024],name[1024];
	int val,n,i,type;

	fp=fopen("db/const.txt","r");
	if(fp==NULL){
		printf("can't read db/const.txt\n");
		return ;
	}
	while(fgets(line,1020,fp)){
		if(line[0]=='/' && line[1]=='/')
			continue;
		type=0;
		if(sscanf(line,"%[A-Za-z0-9_],%d,%d",name,&val,&type)>=2 ||
		   sscanf(line,"%[A-Za-z0-9_] %d %d",name,&val,&type)>=2){
			for(i=0;name[i];i++)
				name[i]=tolower(name[i]);
			n=add_str(name);
			if(type==0)
				str_data[n].type=C_INT;
			else
				str_data[n].type=C_PARAM;
			str_data[n].val=val;
		}
	}
	fclose(fp);
}

/*==========================================
 * エラー表示
 *------------------------------------------
 */

const char* script_print_line( const char *p, const char *mark, int line );

void script_error(char *src,const char *file,int start_line, const char *error_msg, const char *error_pos) {
	// エラーが発生した行を求める
	int j;
	int line = start_line;
	const char *p;
	const char *linestart[5] = { NULL, NULL, NULL, NULL, NULL };

	for(p=src;p && *p;line++){
		char *lineend=strchr(p,'\n');
		if(lineend==NULL || error_pos<lineend){
			break;
		}
		for( j = 0; j < 4; j++ ) {
			linestart[j] = linestart[j+1];
		}
		linestart[4] = p;
		p=lineend+1;
	}

	printf("\a\n");
	printf("script error on %s line %d\n", file, line);
	printf("    %s\n", error_msg);
	for(j = 0; j < 5; j++ ) {
		script_print_line( linestart[j], NULL, line + j - 5);
	}
	p = script_print_line( p, error_pos, -line);
	for(j = 0; j < 5; j++) {
		p = script_print_line( p, NULL, line + j + 1 );
	}
}

const char* script_print_line( const char *p, const char *mark, int line ) {
	int i;
	if( p == NULL || !p[0] ) return NULL;
	if( line < 0 ) 
		printf("*% 5d : ", -line);
	else
		printf(" % 5d : ", line);
	for(i=0;p[i] && p[i] != '\n';i++){
		if(p + i != mark)
			printf("%c",p[i]);
		else
			printf("\'%c\'",p[i]);
	}
	printf("\n");
	return p+i+(p[i] == '\n' ? 1 : 0);
}

/*==========================================
 * スクリプトの解析
 *------------------------------------------
 */

struct script_code* parse_script(unsigned char *src,const char *file,int line)
{
	unsigned char *p,*tmpp;
	int i;
	struct script_code *code;
	static int first=1;

	memset(&syntax,0,sizeof(syntax));
	if(first){
		add_buildin_func();
		read_constdb();
	}
	first=0;
	script_buf=(unsigned char *)aCalloc(SCRIPT_BLOCK_SIZE,sizeof(unsigned char));
	script_pos=0;
	script_size=SCRIPT_BLOCK_SIZE;
	str_data[LABEL_NEXTLINE].type=C_NOP;
	str_data[LABEL_NEXTLINE].backpatch=-1;
	str_data[LABEL_NEXTLINE].label=-1;
	for(i=LABEL_START;i<str_num;i++){
		if(
			str_data[i].type==C_POS || str_data[i].type==C_NAME ||
			str_data[i].type==C_USERFUNC || str_data[i].type == C_USERFUNC_POS
		){
			str_data[i].type=C_NOP;
			str_data[i].backpatch=-1;
			str_data[i].label=-1;
		}
	}

	// 外部用label dbの初期化
	if(scriptlabel_db!=NULL)
		strdb_final(scriptlabel_db,scriptlabel_final);
	scriptlabel_db=strdb_init(50);

	if( setjmp( error_jump ) != 0 ) {
		script_error(src,file,line,error_msg,error_pos);
		// 後始末
		aFree( error_msg );
		aFree( script_buf );
		script_pos  = 0;
		script_size = 0;
		script_buf  = NULL;
		for(i=LABEL_START;i<str_num;i++){
			if(str_data[i].type == C_NOP) str_data[i].type = C_NAME;
		}
		return NULL;
	}

	p=src;
	p=skip_space(p);
	if(*p!='{'){
		disp_error_message("not found '{'",p);
		return NULL;
	}
	for(p++;p && *p && (*p!='}' || syntax.curly_count != 0);) {
		p=skip_space(p);
		// labelだけ特殊処理
		tmpp=skip_space(skip_word(p));
		if(*tmpp==':' && !(!strncmp(p,"default:",8) && p + 7 == tmpp)){
			int l,c;

			c=*skip_word(p);
			*skip_word(p)=0;
			l=add_str(p);
			if(str_data[l].label!=-1){
				*skip_word(p)=c;
				disp_error_message("dup label ",p);
				exit(1);
			}
			set_label(l,script_pos);
			strdb_insert(scriptlabel_db,p,script_pos);	// 外部用label db登録
			*skip_word(p)=c;
			p=tmpp+1;
			continue;
		}

		// 他は全部一緒くた
		p=parse_line(p);
		p=skip_space(p);
		add_scriptc(C_EOL);

		set_label(LABEL_NEXTLINE,script_pos);
		str_data[LABEL_NEXTLINE].type=C_NOP;
		str_data[LABEL_NEXTLINE].backpatch=-1;
		str_data[LABEL_NEXTLINE].label=-1;
	}

	add_scriptc(C_NOP);

	script_size = script_pos;
	script_buf=(char *)aRealloc(script_buf,script_pos);

	// 未解決のラベルを解決
	for(i=LABEL_START;i<str_num;i++){
		if(str_data[i].type==C_NOP){
			int j,next;
			str_data[i].type=C_NAME;
			str_data[i].label=i;
#ifdef DEBUG_VARS
			if( str_data[i].backpatch != -1 ) {
				struct vars_info *v = numdb_search(vars_db, i);
				if( v == NULL ) {
					v            = aMalloc( sizeof( struct vars_info ) );
					v->file      = aStrdup( file );
					v->line      = line;
					v->use_count = 1;
					numdb_insert(vars_db, i, v);
					vars_count++;
				} else {
					v->use_count++;
				}
			}
#endif
			for(j=str_data[i].backpatch;j>=0 && j!=0x00ffffff;){
				next=(*(int*)(script_buf+j)) & 0x00ffffff;
				script_buf[j]=i;
				script_buf[j+1]=i>>8;
				script_buf[j+2]=i>>16;
				j=next;
			}
		}
	}

#ifdef DEBUG_DISP
	for(i=0;i<script_pos;i++){
		if((i&15)==0) printf("%04x : ",i);
		printf("%02x ",script_buf[i]);
		if((i&15)==15) printf("\n");
	}
	printf("\n");
#endif
#ifdef DEBUG_DISASM
	{
		int i = 0,j;
		while(i < script_pos) {
			printf("%06x ",i);
			j = i;
			switch(get_com(script_buf,&i)) {
			case C_EOL:	 printf("C_EOL"); break;
			case C_INT:	 printf("C_INT %d",get_num(script_buf,&i)); break;
			case C_POS:
				printf("C_POS  0x%06x",*(int*)(script_buf+i)&0xffffff);
				i += 3;
				break;
			case C_NAME:
				j = (*(int*)(script_buf+i)&0xffffff);
				printf("C_NAME %s",j == 0xffffff ? "?? unknown ??" : str_buf + str_data[j].str);
				i += 3;
				break;
			case C_ARG:		printf("C_ARG"); break;
			case C_FUNC:	printf("C_FUNC"); break;
			case C_ADD:	 	printf("C_ADD"); break;
			case C_SUB:		printf("C_SUB"); break;
			case C_MUL:		printf("C_MUL"); break;
			case C_DIV:		printf("C_DIV"); break;
			case C_MOD:		printf("C_MOD"); break;
			case C_EQ:		printf("C_EQ"); break;
			case C_NE:		printf("C_NE"); break;
			case C_GT:		printf("C_GT"); break;
			case C_GE:		printf("C_GE"); break;
			case C_LT:		printf("C_LT"); break;
			case C_LE:		printf("C_LE"); break;
			case C_AND:		printf("C_AND"); break;
			case C_OR:		printf("C_OR"); break;
			case C_XOR:		printf("C_XOR"); break;
			case C_LAND:	printf("C_LAND"); break;
			case C_LOR:		printf("C_LOR"); break;
			case C_R_SHIFT:	printf("C_R_SHIFT"); break;
			case C_L_SHIFT:	printf("C_L_SHIFT"); break;
			case C_NEG:		printf("C_NEG"); break;
			case C_NOT:		printf("C_NOT"); break;
			case C_LNOT:	printf("C_LNOT"); break;
			case C_NOP:		printf("C_NOP"); break;
			case C_OP3:		printf("C_OP3"); break;
			case C_STR:
				j = strlen(script_buf + i);
				printf("C_STR %s",script_buf + i);
				i+= j+1;
				break;
			default:
				printf("unknown");
			}
			printf("\n");
		}
	}
#endif

	code = aCalloc(1, sizeof(struct script_code));
	code->script_buf  = script_buf;
	code->script_size = script_size;
	code->script_vars = NULL;
	return code;
}

//
// 実行系
//
enum {RUN = 0,STOP,END,RERUNLINE,GOTO,RETFUNC};

/*==========================================
 * ridからsdへの解決
 *------------------------------------------
 */
struct map_session_data *script_rid2sd(struct script_state *st)
{
	struct map_session_data *sd=map_id2sd(st->rid);
	if(!sd){
		printf("script_rid2sd: fatal error ! player not attached!\n");
	}
	return sd;
}


/*==========================================
 * 変数の読み取り
 *------------------------------------------
 */
int get_val(struct script_state*st,struct script_data* data)
{
	struct map_session_data *sd=NULL;
	if(data->type==C_NAME){
		char *name=str_buf+str_data[data->u.num&0x00ffffff].str;
		char prefix=*name;
		char postfix=name[strlen(name)-1];

		if(prefix!='$' && prefix != '\''){
			if((sd=script_rid2sd(st))==NULL)
				printf("get_val error name?:%s\n",name);
		}
		if(postfix=='$'){

			data->type=C_CONSTSTR;
			if( prefix=='@' || prefix=='l' ){
				if(sd)
					data->u.str = pc_readregstr(sd,data->u.num);
			}else if(prefix=='$'){
				data->u.str = (char *)numdb_search(mapregstr_db,data->u.num);
			} else if(prefix=='\'') {
				struct linkdb_node **n;
				if( data->ref ) {
					n = data->ref;
				} else if( name[1] == '@' ) {
					n = st->stack->var_function;
				} else {
					n = &st->script->script_vars;
				}
				data->u.str = linkdb_search(n, (void*)data->u.num );
			}else{
				printf("script: get_val: illeagal scope string variable.\n");
				data->u.str = "!!ERROR!!";
			}
			if( data->u.str == NULL )
				data->u.str ="";

		}else{

			data->type=C_INT;
			if(str_data[data->u.num&0x00ffffff].type==C_INT){
				data->u.num = str_data[data->u.num&0x00ffffff].val;
			}else if(str_data[data->u.num&0x00ffffff].type==C_PARAM){
				if(sd)
					data->u.num = pc_readparam(sd,str_data[data->u.num&0x00ffffff].val);
			}else if(prefix=='@' || prefix=='l'){
				if(sd)
					data->u.num = pc_readreg(sd,data->u.num);
			}else if(prefix=='$'){
				data->u.num = (int)numdb_search(mapreg_db,data->u.num);
			}else if(prefix=='#'){
				if( name[1]=='#'){
					if(sd)
						data->u.num = pc_readaccountreg2(sd,name);
				}else{
					if(sd)
						data->u.num = pc_readaccountreg(sd,name);
				}
			} else if(prefix=='\''){
				struct linkdb_node **n;
				if( data->ref ) {
					n = data->ref;
				} else if( name[1] == '@' ) {
					n = st->stack->var_function;
				} else {
					n = &st->script->script_vars;
				}
				data->u.num = (int)linkdb_search(n, (void*)data->u.num);
			}else{
				if(sd)
					data->u.num = pc_readglobalreg(sd,name);
			}
		}
	}
	return 0;
}
/*==========================================
 * 変数の読み取り2
 *------------------------------------------
 */
void* get_val2(struct script_state*st,int num,struct linkdb_node **ref)
{
	struct script_data dat;
	dat.type  = C_NAME;
	dat.u.num = num;
	dat.ref   = ref;
	get_val(st,&dat);
	if( dat.type==C_INT ) return (void*)dat.u.num;
	else return (void*)dat.u.str;
}

/*==========================================
 * 変数設定用
 *------------------------------------------
 */
static int set_reg(struct script_state*st,struct map_session_data *sd,int num,char *name,void *v,struct linkdb_node** ref)
{
	char prefix=*name;
	char postfix=name[strlen(name)-1];

	if( postfix=='$' ){
		char *str=(char*)v;
		if( prefix=='@' || prefix=='l'){
			pc_setregstr(sd,num,str);
		}else if(prefix=='$') {
			mapreg_setregstr(num,str);
		}else if(prefix=='\'') {
			char *p;
			struct linkdb_node **n;
			if( ref ) {
				n = ref;
			} else if( name[1] == '@' ) {
				n = st->stack->var_function;
			} else {
				n = &st->script->script_vars;
			}
			p = linkdb_search(n, (void*)num);
			if(p) {
				linkdb_erase(n, (void*)num);
				aFree(p);
			}
			if( ((char*)v)[0] )
				linkdb_insert(n, (void*)num, aStrdup(v));
		}else{
			printf("script: set_reg: illeagal scope string variable !");
		}
	}else{
		// 数値
		int val = (int)v;
		if(str_data[num&0x00ffffff].type==C_PARAM){
			pc_setparam(sd,str_data[num&0x00ffffff].val,val);
		}else if(prefix=='@' || prefix=='l') {
			pc_setreg(sd,num,val);
		}else if(prefix=='$') {
			mapreg_setreg(num,val);
		}else if(prefix=='#') {
			if( name[1]=='#' )
				pc_setaccountreg2(sd,name,val);
			else
				pc_setaccountreg(sd,name,val);
		}else if(prefix == '\'') {
			struct linkdb_node **n;
			if( ref ) {
				n = ref;
			} else if( name[1] == '@' ) {
				n = st->stack->var_function;
			} else {
				n = &st->script->script_vars;
			}
			if( val == 0 ) {
				linkdb_erase(n, (void*)num);
			} else {
				linkdb_replace(n, (void*)num, (void*)val);
			}
		}else{
			pc_setglobalreg(sd,name,val);
		}
	}
	return 0;
}

/*==========================================
 * 文字列への変換
 *------------------------------------------
 */
char* conv_str(struct script_state *st,struct script_data *data)
{
	get_val(st,data);
	if(data->type==C_INT){
		char *buf;
		buf=(char *)aCalloc(16,sizeof(char));
		sprintf(buf,"%d",data->u.num);
		data->type=C_STR;
		data->u.str=buf;
#if 1
	} else if(data->type==C_NAME){
		// テンポラリ。本来無いはず
		data->type=C_CONSTSTR;
		data->u.str=str_buf+str_data[data->u.num].str;
#endif
	}
	return data->u.str;
}

/*==========================================
 * 数値へ変換
 *------------------------------------------
 */
int conv_num(struct script_state *st,struct script_data *data)
{
	char *p;
	get_val(st,data);
	if(data->type==C_STR || data->type==C_CONSTSTR){
		p=data->u.str;
		data->u.num = atoi(p);
		if(data->type==C_STR)
			free(p);
		data->type=C_INT;
	}
	return data->u.num;
}

/*==========================================
 * スタックへ数値をプッシュ
 *------------------------------------------
 */
void push_val(struct script_stack *stack,int type,int val)
{
	if(stack->sp >= stack->sp_max){
		stack->sp_max += 64;
		stack->stack_data = (struct script_data *)aRealloc(stack->stack_data,
			sizeof(stack->stack_data[0]) * stack->sp_max);
		memset(stack->stack_data + (stack->sp_max - 64), 0,
			64 * sizeof(*(stack->stack_data)));
	}
//	if(battle_config.etc_log)
//		printf("push (%d,%d)-> %d\n",type,val,stack->sp);
	stack->stack_data[stack->sp].type  = type;
	stack->stack_data[stack->sp].u.num = val;
	stack->stack_data[stack->sp].ref   = NULL;
	stack->sp++;
}

/*==========================================
 * スタックへ数値＋リファレンスをプッシュ
 *------------------------------------------
 */

void push_val2(struct script_stack *stack,int type,int val,struct linkdb_node** ref) {
	push_val(stack,type,val);
	stack->stack_data[stack->sp-1].ref = ref;
}

/*==========================================
 * スタックへ文字列をプッシュ
 *------------------------------------------
 */
void push_str(struct script_stack *stack,int type,unsigned char *str)
{
	if(stack->sp>=stack->sp_max){
		stack->sp_max += 64;
		stack->stack_data = (struct script_data *)aRealloc(stack->stack_data,
			sizeof(stack->stack_data[0]) * stack->sp_max);
		memset(stack->stack_data + (stack->sp_max - 64), '\0',
			64 * sizeof(*(stack->stack_data)));
	}
//	if(battle_config.etc_log)
//		printf("push (%d,%x)-> %d\n",type,str,stack->sp);
	stack->stack_data[stack->sp].type  = type;
	stack->stack_data[stack->sp].u.str = str;
	stack->stack_data[stack->sp].ref   = NULL;
	stack->sp++;
}

/*==========================================
 * スタックへ複製をプッシュ
 *------------------------------------------
 */
void push_copy(struct script_stack *stack,int pos)
{
	switch(stack->stack_data[pos].type){
	case C_CONSTSTR:
		push_str(stack,C_CONSTSTR,stack->stack_data[pos].u.str);
		break;
	case C_STR:
		push_str(stack,C_STR,strdup(stack->stack_data[pos].u.str));
		break;
	default:
		push_val2(
			stack,stack->stack_data[pos].type,stack->stack_data[pos].u.num,
			stack->stack_data[pos].ref
		);
		break;
	}
}

/*==========================================
 * スタックからポップ
 *------------------------------------------
 */
void pop_stack(struct script_stack* stack,int start,int end)
{
	int i;
	for(i=start;i<end;i++){
		if(stack->stack_data[i].type==C_STR){
			free(stack->stack_data[i].u.str);
		}
	}
	if(stack->sp>end){
		memmove(&stack->stack_data[start],&stack->stack_data[end],sizeof(stack->stack_data[0])*(stack->sp-end));
	}
	stack->sp-=end-start;
}

/*==========================================
 * スクリプト依存変数、関数依存変数の解放
 *------------------------------------------
 */
void script_free_vars(struct linkdb_node **node) {
	struct linkdb_node *n = *node;
	while(n) {
		char *name   = str_buf + str_data[(int)(n->key)&0x00ffffff].str;
		char postfix = name[strlen(name)-1];
		if( postfix == '$' ) {
			// 文字型変数なので、データ削除
			aFree(n->data);
		}
		n = n->next;
	}
	linkdb_final( node );
}

/*==========================================
 * スタックの解放
 *------------------------------------------
 */
void script_free_stack(struct script_stack *stack) {
	int i;
	for(i = 0; i < stack->sp; i++) {
		if( stack->stack_data[i].type == C_STR ) {
			aFree( stack->stack_data[i].u.str );
		} else if( i > 0 && stack->stack_data[i].type == C_RETINFO ) {
			struct linkdb_node** n = (struct linkdb_node**)stack->stack_data[i-1].u.num;
			script_free_vars( n );
			aFree( n );
		}
	}
	script_free_vars( stack->var_function );
	aFree(stack->var_function);
	aFree(stack->stack_data);
	aFree(stack);
}

/*==========================================
 * スクリプトコードの解放
 *------------------------------------------
 */

void script_free_code(struct script_code* code) {
	script_free_vars( &code->script_vars );
	aFree( code->script_buf );
	aFree( code );
}

//
// 実行部main
//
/*==========================================
 * コマンドの読み取り
 *------------------------------------------
 */
static int unget_com_data=-1;
int get_com(unsigned char *script,int *pos)
{
	int i,j;
	if(unget_com_data>=0){
		i=unget_com_data;
		unget_com_data=-1;
		return i;
	}
	if(script[*pos]>=0x80){
		return C_INT;
	}
	i=0; j=0;
	while(script[*pos]>=0x40){
		i=script[(*pos)++]<<j;
		j+=6;
	}
	return i+(script[(*pos)++]<<j);
}

/*==========================================
 * コマンドのプッシュバック
 *------------------------------------------
 */
void unget_com(int c)
{
	if(unget_com_data!=-1){
		if(battle_config.error_log)
			printf("unget_com can back only 1 data\n");
	}
	unget_com_data=c;
}

/*==========================================
 * 数値の所得
 *------------------------------------------
 */
int get_num(unsigned char *script,int *pos)
{
	int i,j;
	i=0; j=0;
	while(script[*pos]>=0xc0){
		i+=(script[(*pos)++]&0x7f)<<j;
		j+=6;
	}
	return i+((script[(*pos)++]&0x7f)<<j);
}

/*==========================================
 * スタックから値を取り出す
 *------------------------------------------
 */
int pop_val(struct script_state* st)
{
	if(st->stack->sp<=0)
		return 0;
	st->stack->sp--;
	get_val(st,&(st->stack->stack_data[st->stack->sp]));
	if(st->stack->stack_data[st->stack->sp].type==C_INT)
		return st->stack->stack_data[st->stack->sp].u.num;
	return 0;
}

int isstr(struct script_data *c) {
	if( c->type == C_STR || c->type == C_CONSTSTR )
		return 1;
	else if( c->type == C_NAME ) {
		char *p = str_buf + str_data[c->u.num & 0xffffff].str;
		char postfix = p[strlen(p)-1];
		return (postfix == '$');
	}
	return 0;
}

/*==========================================
 * 加算演算子
 *------------------------------------------
 */
void op_add(struct script_state* st)
{
	st->stack->sp--;
	get_val(st,&(st->stack->stack_data[st->stack->sp]));
	get_val(st,&(st->stack->stack_data[st->stack->sp-1]));

	if(isstr(&st->stack->stack_data[st->stack->sp]) || isstr(&st->stack->stack_data[st->stack->sp-1])){
		conv_str(st,&(st->stack->stack_data[st->stack->sp]));
		conv_str(st,&(st->stack->stack_data[st->stack->sp-1]));
	}
	if(st->stack->stack_data[st->stack->sp].type==C_INT){ // ii
		int *i1 = &st->stack->stack_data[st->stack->sp-1].u.num;
		int *i2 = &st->stack->stack_data[st->stack->sp].u.num;
		int ret = *i1 + *i2;
		double ret_double = (double)*i1 + (double)*i2;
		if(ret_double > 0x7FFFFFFF || ret_double < -1 * 0x7FFFFFFF) {
			printf("script::op_add overflow detected op:%d rid:%d\n",C_ADD,st->rid);
			ret = (ret_double > 0x7FFFFFFF ? 0x7FFFFFFF : -1 * 0x7FFFFFFF);
		}
		*i1 = ret;
	} else { // ssの予定
		char *buf;
		buf=(char *)aCalloc(strlen(st->stack->stack_data[st->stack->sp-1].u.str)+
				strlen(st->stack->stack_data[st->stack->sp].u.str)+1,sizeof(char));
		strcpy(buf,st->stack->stack_data[st->stack->sp-1].u.str);
		strcat(buf,st->stack->stack_data[st->stack->sp].u.str);
		if(st->stack->stack_data[st->stack->sp-1].type==C_STR)
			free(st->stack->stack_data[st->stack->sp-1].u.str);
		if(st->stack->stack_data[st->stack->sp].type==C_STR)
			free(st->stack->stack_data[st->stack->sp].u.str);
		st->stack->stack_data[st->stack->sp-1].type= C_STR;
		st->stack->stack_data[st->stack->sp-1].u.str=buf;
	}
	st->stack->stack_data[st->stack->sp-1].ref = NULL;
}

/*==========================================
 * 三項演算子
 *------------------------------------------
 */

void op_3(struct script_state *st) {
	int flag = 0;
	if( isstr(&st->stack->stack_data[st->stack->sp-3])) {
		char *str = conv_str(st,& (st->stack->stack_data[st->stack->sp-3]));
		flag = str[0];
	} else {
		flag = conv_num(st,& (st->stack->stack_data[st->stack->sp-3]));
	}
	if( flag ) {
		push_copy(st->stack, st->stack->sp-2 );
	} else {
		push_copy(st->stack, st->stack->sp-1 );
	}
	pop_stack(st->stack,st->stack->sp-4,st->stack->sp-1);
}

/*==========================================
 * 二項演算子(文字列)
 *------------------------------------------
 */
void op_2str(struct script_state *st,int op,int sp1,int sp2)
{
	char *s1=st->stack->stack_data[sp1].u.str,
		 *s2=st->stack->stack_data[sp2].u.str;
	int a=0;

	switch(op){
	case C_EQ:
		a= (strcmp(s1,s2)==0);
		break;
	case C_NE:
		a= (strcmp(s1,s2)!=0);
		break;
	case C_GT:
		a= (strcmp(s1,s2)> 0);
		break;
	case C_GE:
		a= (strcmp(s1,s2)>=0);
		break;
	case C_LT:
		a= (strcmp(s1,s2)< 0);
		break;
	case C_LE:
		a= (strcmp(s1,s2)<=0);
		break;
	default:
		printf("Illeagal string operater\n");
		break;
	}
	if(st->stack->stack_data[sp1].type==C_STR)	free(s1);
	if(st->stack->stack_data[sp2].type==C_STR)	free(s2);

	push_val(st->stack,C_INT,a);
}
/*==========================================
 * 二項演算子(数値)
 *------------------------------------------
 */
void op_2num(struct script_state *st,int op,int i1,int i2)
{
	int ret = 0;
	double ret_double = 0;
	switch(op){
	case C_MOD:  ret = i1 % i2;		break;
	case C_AND:  ret = i1 & i2;		break;
	case C_OR:   ret = i1 | i2;		break;
	case C_XOR:  ret = i1 ^ i2;		break;
	case C_LAND: ret = (i1 && i2);	break;
	case C_LOR:  ret = (i1 || i2);	break;
	case C_EQ:   ret = (i1 == i2);	break;
	case C_NE:   ret = (i1 != i2);	break;
	case C_GT:   ret = (i1 >  i2);	break;
	case C_GE:   ret = (i1 >= i2);	break;
	case C_LT:   ret = (i1 <  i2);	break;
	case C_LE:   ret = (i1 <= i2);	break;
	case C_R_SHIFT: ret = i1>>i2;	break;
	case C_L_SHIFT: ret = i1<<i2;	break;
	default:
		switch(op) {
		case C_SUB: ret = i1 - i2; ret_double = (double)i1 - (double)i2; break;
		case C_MUL: ret = i1 * i2; ret_double = (double)i1 * (double)i2; break;
		case C_DIV:
			if(i2 == 0) {
				printf("script::op_2num division by zero.\n");
				ret = 0x7FFFFFFF;
				ret_double = 0; // doubleの精度が怪しいのでオーバーフロー対策を飛ばす
			} else {
				ret = i1 / i2; ret_double = (double)i1 / (double)i2;
			}
			break;
		}
		if(ret_double > 0x7FFFFFFF || ret_double < -1 * 0x7FFFFFFF) {
			printf("script::op_2num overflow detected op:%d rid:%d\n",op,st->rid);
			ret = (ret_double > 0x7FFFFFFF ? 0x7FFFFFFF : -1 * 0x7FFFFFFF);
		}
	}
	push_val(st->stack,C_INT,ret);
}
/*==========================================
 * 二項演算子
 *------------------------------------------
 */
void op_2(struct script_state *st,int op)
{
	int i1,i2;
	char *s1=NULL,*s2=NULL;

	i2=pop_val(st);
	if( isstr(&st->stack->stack_data[st->stack->sp]) )
		s2=st->stack->stack_data[st->stack->sp].u.str;

	i1=pop_val(st);
	if( isstr(&st->stack->stack_data[st->stack->sp]) )
		s1=st->stack->stack_data[st->stack->sp].u.str;

	if( s1!=NULL && s2!=NULL ){
		// ss => op_2str
		op_2str(st,op,st->stack->sp,st->stack->sp+1);
	}else if( s1==NULL && s2==NULL ){
		// ii => op_2num
		op_2num(st,op,i1,i2);
	}else{
		// si,is => error
		printf("script: op_2: int&str, str&int not allow.\n");
		push_val(st->stack,C_INT,0);
	}
}

/*==========================================
 * 単項演算子
 *------------------------------------------
 */
void op_1num(struct script_state *st,int op)
{
	int i1;
	i1=pop_val(st);
	switch(op){
	case C_NEG:
		i1=-i1;
		break;
	case C_NOT:
		i1=~i1;
		break;
	case C_LNOT:
		i1=!i1;
		break;
	}
	push_val(st->stack,C_INT,i1);
}


/*==========================================
 * 関数の実行
 *------------------------------------------
 */
int run_func(struct script_state *st)
{
	int i,start_sp,end_sp,func;

	end_sp=st->stack->sp;
#ifdef DEBUG_RUN
	if(battle_config.etc_log) {
		printf("stack dump :");
		for(i=0;i<end_sp;i++){
			switch(st->stack->stack_data[i].type){
			case C_INT:
				printf(" int(%d)",st->stack->stack_data[i].u.num);
				break;
			case C_NAME:
				printf(" name(%s)",str_buf+str_data[st->stack->stack_data[i].u.num & 0xffffff].str);
				break;
			case C_ARG:
				printf(" arg");
				break;
			case C_POS:
				printf(" pos(%d)",st->stack->stack_data[i].u.num);
				break;
			case C_STR:
				printf(" str(%s)",st->stack->stack_data[i].u.str);
				break;
			case C_CONSTSTR:
				printf(" cstr(%s)",st->stack->stack_data[i].u.str);
				break;
			default:
				printf(" etc(%d,%d)",st->stack->stack_data[i].type,st->stack->stack_data[i].u.num);
			}
		}
		printf("\n");
	}
#endif
	for(i=end_sp-1;i>=0 && st->stack->stack_data[i].type!=C_ARG;i--);
	if(i==0){
		if(battle_config.error_log)
			printf("function not found\n");
//		st->stack->sp=0;
		st->state=END;
		return 0;
	}
	start_sp=i-1;
	st->start=i-1;
	st->end=end_sp;

	func=st->stack->stack_data[st->start].u.num;
	if(str_data[func].type!=C_FUNC ){
		printf("run_func: not function and command! \n");
//		st->stack->sp=0;
		st->state=END;
		return 0;
	}
#ifdef DEBUG_RUN
	printf("run_func : %s (func_no : %d , func_type : %d pos : 0x%x)\n",
		str_buf+str_data[func].str,func,str_data[func].type,st->pos
	);
#endif
	if(str_data[func].func){
		str_data[func].func(st);
	} else {
		if(battle_config.error_log)
			printf("run_func : %s? (%d(%d))\n",str_buf+str_data[func].str,func,str_data[func].type);
		push_val(st->stack,C_INT,0);
	}

	if(st->state != RERUNLINE) {
		pop_stack(st->stack,start_sp,end_sp);
	}

	if(st->state==RETFUNC){
		// ユーザー定義関数からの復帰
		int olddefsp=st->stack->defsp;
		int i;

		pop_stack(st->stack,st->stack->defsp,start_sp);	// 復帰に邪魔なスタック削除
		if(st->stack->defsp<5 || st->stack->stack_data[st->stack->defsp-1].type!=C_RETINFO){
			printf("script:run_func(return) return without callfunc or callsub!\n");
			st->state=END;
			return 0;
		}
		// 関数依存変数の削除
		script_free_vars( st->stack->var_function );
		aFree(st->stack->var_function);

		i = conv_num(st,& (st->stack->stack_data[st->stack->defsp-5]));									// 引数の数所得
		st->pos=conv_num(st,& (st->stack->stack_data[st->stack->defsp-1]));								// スクリプト位置の復元
		st->script=(struct script_code*)conv_num(st,& (st->stack->stack_data[st->stack->defsp-3]));		// スクリプトを復元
		st->stack->var_function = (struct linkdb_node**)st->stack->stack_data[st->stack->defsp-2].u.num; // 関数依存変数

		st->stack->defsp=conv_num(st,& (st->stack->stack_data[st->stack->defsp-4]));	// 基準スタックポインタを復元
		pop_stack(st->stack,olddefsp-5-i,olddefsp);		// 要らなくなったスタック(引数と復帰用データ)削除

		st->state=GOTO;
	}

	return 0;
}


/*==========================================
 * スクリプトの実行
 *------------------------------------------
 */
int run_script_main(struct script_state *st);

void run_script(struct script_code *rootscript,int pos,int rid,int oid)
{
	struct script_state *st;
	struct map_session_data *sd=map_id2sd(rid);

	if(rootscript==NULL || pos<0)
		return;
	st = calloc(sizeof(struct script_state), 1);
	if(sd && sd->stack && sd->npc_scriptroot == rootscript){
		// 前回のスタックを復帰
		st->script = sd->npc_script;
		st->stack  = sd->stack;
		st->state  = sd->npc_scriptstate;
		sd->stack           = NULL; // プレイヤーがデタッチされても良いようにクリア
		sd->npc_script	    = NULL;
		sd->npc_scriptroot  = NULL;
		sd->npc_scriptstate = 0;
	} else {
		// スタック初期化
		st->stack = calloc(1,sizeof(struct script_stack));
		st->stack->sp=0;
		st->stack->sp_max=64;
		st->stack->stack_data   = (struct script_data *)aCalloc(st->stack->sp_max,sizeof(st->stack->stack_data[0]));
		st->stack->defsp        = st->stack->sp;
		st->stack->var_function = aCalloc(1, sizeof(struct linkdb_node*));
		st->state  = RUN;
		st->script = rootscript;
	}
	st->pos = pos;
	st->rid = rid;
	st->oid = oid;
	st->scriptroot = rootscript;
	st->sleep.timer = -1;
	run_script_main(st); // st のfreeも含めてこの内部で処理する
}

/*==========================================
 * 指定ノードをsleep_dbから削除
 *------------------------------------------
 */
struct linkdb_node* script_erase_sleepdb(struct linkdb_node *n)
{
	struct linkdb_node *retnode;

	if( n == NULL)
		return NULL;
	if( n->prev == NULL )
		sleep_db = n->next;
	else
		n->prev->next = n->next;
	if( n->next )
		n->next->prev = n->prev;
	retnode = n->next;
	aFree( n );
	return retnode;		// 次のノードを返す
}

/*==========================================
 * sleep用タイマー関数
 *------------------------------------------
 */
int run_script_timer(int tid, unsigned int tick, int id, int data)
{
	struct script_state *st     = (struct script_state *)data;
	struct linkdb_node *node    = (struct linkdb_node *)sleep_db;
	struct map_session_data *sd = map_id2sd(st->rid);

	if( sd && sd->char_id != id ) {
		st->rid = 0;
	}
	while( node && st->sleep.timer != -1 ) {
		if( (int)node->key == st->oid && ((struct script_state *)node->data)->sleep.timer == st->sleep.timer ) {
			script_erase_sleepdb(node);
			st->sleep.timer = -1;
			break;
		}
		node = node->next;
	}
	run_script_main(st);
	return 0;
}

/*==========================================
 * スクリプトの実行メイン部分
 *------------------------------------------
 */
int run_script_main(struct script_state *st)
{
	int c;
	int cmdcount=script_config.check_cmdcount;
	int gotocount=script_config.check_gotocount;
	struct map_session_data *sd;

	if(st->state == RERUNLINE) {
		st->state = RUN;
		run_func(st);
		if(st->state == GOTO){
			st->state = RUN;
		}
	} else {
		st->state = RUN;
	}
	while(st->state == RUN){
		switch(c=get_com(st->script->script_buf,&st->pos)){
		case C_EOL:
			if(st->stack->sp!=st->stack->defsp){
				if(battle_config.error_log)
					printf("stack.sp(%d) != default(%d)\n",st->stack->sp,st->stack->defsp);
				st->stack->sp=st->stack->defsp;
			}
			break;
		case C_INT:
			push_val(st->stack,C_INT,get_num(st->script->script_buf,&st->pos));
			break;
		case C_POS:
		case C_NAME:
			push_val(st->stack,c,(*(int*)(st->script->script_buf+st->pos))&0xffffff);
			st->pos+=3;
			break;
		case C_ARG:
			push_val(st->stack,c,0);
			break;
		case C_STR:
			push_str(st->stack,C_CONSTSTR,st->script->script_buf+st->pos);
			while(st->script->script_buf[st->pos++]);
			break;
		case C_FUNC:
			run_func(st);
			if(st->state==GOTO){
				st->state = RUN;
				if( gotocount>0 && (--gotocount)<=0 ){
					printf("run_script: infinity loop !\n");
					st->state=END;
				}
			}
			break;

		case C_ADD:
			op_add(st);
			break;

		case C_SUB:
		case C_MUL:
		case C_DIV:
		case C_MOD:
		case C_EQ:
		case C_NE:
		case C_GT:
		case C_GE:
		case C_LT:
		case C_LE:
		case C_AND:
		case C_OR:
		case C_XOR:
		case C_LAND:
		case C_LOR:
		case C_R_SHIFT:
		case C_L_SHIFT:
			op_2(st,c);
			break;

		case C_NEG:
		case C_NOT:
		case C_LNOT:
			op_1num(st,c);
			break;

		case C_OP3:
			op_3(st);
			break;

		case C_NOP:
			st->state=END;
			break;

		default:
			if(battle_config.error_log)
				printf("unknown command : %d @ %d\n",c,st->pos);
			st->state=END;
			break;
		}
		if( cmdcount>0 && (--cmdcount)<=0 ){
			printf("run_script: infinity loop !\n");
			st->state=END;
		}
	}

	sd = map_id2sd(st->rid);
	if(st->sleep.tick > 0) {
		// スタック情報をsleep_dbに保存
		unsigned int tick = gettick()+st->sleep.tick;
		st->sleep.charid = sd ? sd->char_id : 0;
		st->sleep.timer  = add_timer(tick, run_script_timer, st->sleep.charid, (int)st);
		linkdb_insert(&sleep_db, (void*)st->oid, st);
	}
	else if(st->state != END && sd){
		// 再開するためにスタック情報を保存
		sd->npc_script		= st->script;
		sd->npc_scriptroot	= st->scriptroot;
		sd->npc_scriptstate	= st->state;
		sd->stack			= st->stack;
		sd->npc_id          = st->oid;
		sd->npc_pos         = st->pos;
		free(st);
	} else {
		// 実行が終わった or 続行不可能なのでスタッククリア
		if(sd && st->oid == sd->npc_id) {
			sd->npc_pos = -1;
			sd->npc_id  = 0;
			npc_event_dequeue(sd);
		}
		st->pos = -1;
		script_free_stack(st->stack);
		free(st);
	}

	return 0;
}

/*==========================================
 * マップ変数の変更
 *------------------------------------------
 */
int mapreg_setreg(int num,int val)
{
	if(val!=0)
		numdb_insert(mapreg_db,num,val);
	else
		numdb_erase(mapreg_db,num);

	mapreg_dirty=1;
	return 0;
}
/*==========================================
 * 文字列型マップ変数の変更
 *------------------------------------------
 */
int mapreg_setregstr(int num,const char *str)
{
	char *p;

	if( (p=numdb_search(mapregstr_db,num))!=NULL )
		free(p);

	if( str==NULL || *str==0 ){
		numdb_erase(mapregstr_db,num);
		mapreg_dirty=1;
		return 0;
	}
	p=(char *)aCalloc(strlen(str)+1, sizeof(char));
	strcpy(p,str);
	numdb_insert(mapregstr_db,num,p);
	mapreg_dirty=1;
	return 0;
}

/*==========================================
 * 永続的マップ変数の読み込み
 *------------------------------------------
 */
static int script_load_mapreg(void)
{
	FILE *fp;
	char line[1024];

	if( (fp=fopen(mapreg_txt,"rt"))==NULL )
		return -1;

	while(fgets(line,sizeof(line),fp)){
		char buf1[256],buf2[1024],*p;
		int n,v,s,i;
		if( sscanf(line,"%255[^,],%d\t%n",buf1,&i,&n)!=2 &&
			(i=0,sscanf(line,"%[^\t]\t%n",buf1,&n)!=1) )
			continue;
		if( buf1[strlen(buf1)-1]=='$' ){
			if( sscanf(line+n,"%[^\n\r]",buf2)!=1 ){
				printf("%s: %s broken data !\n",mapreg_txt,buf1);
				continue;
			}
			p=(char *)aCalloc(strlen(buf2) + 1,sizeof(char));
			strcpy(p,buf2);
			s=add_str(buf1);
			numdb_insert(mapregstr_db,(i<<24)|s,p);
		}else{
			if( sscanf(line+n,"%d",&v)!=1 ){
				printf("%s: %s broken data !\n",mapreg_txt,buf1);
				continue;
			}
			s=add_str(buf1);
			numdb_insert(mapreg_db,(i<<24)|s,v);
		}
	}
	fclose(fp);
	mapreg_dirty=0;
	return 0;
}
/*==========================================
 * 永続的マップ変数の書き込み
 *------------------------------------------
 */
static int script_save_mapreg_intsub(void *key,void *data,va_list ap)
{
	FILE *fp=va_arg(ap,FILE*);
	int num=((int)key)&0x00ffffff, i=((int)key)>>24;
	char *name=str_buf+str_data[num].str;
	if( name[1]!='@' ){
		if(i==0)
			fprintf(fp,"%s\t%d\n", name, (int)data);
		else
			fprintf(fp,"%s,%d\t%d\n", name, i, (int)data);
	}
	return 0;
}
static int script_save_mapreg_strsub(void *key,void *data,va_list ap)
{
	FILE *fp=va_arg(ap,FILE*);
	int num=((int)key)&0x00ffffff, i=((int)key)>>24;
	char *name=str_buf+str_data[num].str;
	if( name[1]!='@' ){
		if(i==0)
			fprintf(fp,"%s\t%s\n", name, (char *)data);
		else
			fprintf(fp,"%s,%d\t%s\n", name, i, (char *)data);
	}
	return 0;
}
static int script_save_mapreg(void)
{
	FILE *fp;
	int lock;

	if( (fp=lock_fopen(mapreg_txt,&lock))==NULL )
		return -1;
	numdb_foreach(mapreg_db,script_save_mapreg_intsub,fp);
	numdb_foreach(mapregstr_db,script_save_mapreg_strsub,fp);
	lock_fclose(fp,mapreg_txt,&lock);
	mapreg_dirty=0;
	return 0;
}
static int script_autosave_mapreg(int tid,unsigned int tick,int id,int data)
{
	if(mapreg_dirty)
		script_save_mapreg();
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int set_posword(char *p)
{
	char* np,* str[15];
	int i=0;
	for(i=0;i<11;i++) {
		if((np=strchr(p,','))!=NULL) {
			str[i]=p;
			*np=0;
			p=np+1;
		} else {
			str[i]=p;
			p+=strlen(p);
		}
		if(str[i])
			strcpy(pos[i],str[i]);
	}
	return 0;
}

int script_config_read(char *cfgName)
{
	int i;
	char line[1024],w1[1024],w2[1024];
	FILE *fp;

	script_config.warn_func_no_comma=1;
	script_config.warn_cmd_no_comma=1;
	script_config.warn_func_mismatch_paramnum=1;
	script_config.warn_cmd_mismatch_paramnum=1;
	script_config.check_cmdcount=65536;
	script_config.check_gotocount=2048;

	fp=fopen(cfgName,"r");
	if(fp==NULL){
		printf("file not found: %s\n",cfgName);
		return 1;
	}
	while(fgets(line,1020,fp)){
		if(line[0] == '/' && line[1] == '/')
			continue;
		i=sscanf(line,"%[^:]: %[^\r\n]",w1,w2);
		if(i!=2)
			continue;
		if(strcmpi(w1,"refine_posword")==0) {
			set_posword(w2);
		}
		if(strcmpi(w1,"import")==0){
			script_config_read(w2);
		}
	}
	fclose(fp);

	return 0;
}

/*==========================================
 * @コマンドによる変数の操作
 *------------------------------------------
 */
char* script_operate_vars(struct map_session_data *sd,char *name,char *vars,int element,char *v,int flag)
{
	int num;
	char *output = (char*)aCalloc(strlen(vars)+40,sizeof(char));
	char prefix  = *vars;
	char postfix = vars[strlen(vars)-1];
	char array[6];
	struct script_state *st=NULL;
	struct npc_data *nd=NULL;

	for(num=LABEL_START;num<str_num;num++) {
		if(strcmp(vars, str_buf+str_data[num].str)==0) {
			num = (element<<24)+num;
			break;
		}
	}

	if(element > 0) {	// 出力用に[]部分を復元する
		sprintf(array, "[%d]", element);
		strcat(vars, array);
	}
	if(num == str_num) {
		sprintf(output, msg_txt(15), vars);
		return output;
	}
	if(prefix != '$' && prefix != '\'') {
		if(!sd) {
			sprintf(output, msg_txt(54), vars);
			return output;
		}
		if(flag) {		// 読み取り時のみstが必要
			st = (struct script_state*)aCalloc(1,sizeof(struct script_state));
			st->rid = sd->bl.id;
		}
	}
	if(prefix == '\'') {
		if(vars[1] == '@') {
			sprintf(output, msg_txt(56), vars);
			return output;
		}
		nd = npc_name2id(name);
		if(nd == NULL || nd->bl.subtype != SCRIPT || !nd->u.scr.script) {
			sprintf(output, msg_txt(58), vars);
			return output;
		}
	}

	if(flag) {
		void *ret;
		ret = get_val2(st, num, nd? &nd->u.scr.script->script_vars: NULL);

		if(postfix == '$') {
			char *str = (char*)ret;
			output = (char*)aRealloc(output, (strlen(vars)+strlen(str)+4)*sizeof(char));
			sprintf(output, "%s : %s", vars, str);
		} else {
			sprintf(output, "%s : %d", vars, (int)ret);
		}
		if(st)
			aFree(st);
	} else {
		set_reg(NULL, sd, num, vars, (postfix=='$')? (void*)v: (void*)strtol(v,NULL,0), nd? &nd->u.scr.script->script_vars: NULL);

		output = (char*)aRealloc(output, (strlen(vars)+strlen(v)+24)*sizeof(char));
		sprintf(output, msg_txt(67), vars, v);
	}
	return output;
}

/*==========================================
 * 終了
 *------------------------------------------
 */
static int mapreg_db_final(void *key,void *data,va_list ap)
{
	return 0;
}
static int mapregstr_db_final(void *key,void *data,va_list ap)
{
	free(data);
	return 0;
}
static int scriptlabel_db_final(void *key,void *data,va_list ap)
{
	return 0;
}
static int userfunc_db_final(void *key,void *data,va_list ap)
{
	free(key);
	script_free_code(data);
	return 0;
}

#ifdef DEBUG_VARS

static int varlist_sort1(void *key,void *data,va_list ap) {
	int *count  = va_arg(ap, int*);
	int *v      = va_arg(ap, int*);
	v[(*count)++] = (int)key;
	return 0;
}

static int varlist_sort2(const void *a, const void *b) {
	char *name1 = str_buf + str_data[*(const int*)a].str;
	char *name2 = str_buf + str_data[*(const int*)b].str;
	return strcmp(name1, name2);
}


static int varsdb_final(void *key,void *data,va_list ap) {
	struct vars_info *v = (struct vars_info*)data;
	
	aFree( v->file );
	aFree( v );
	return 0;
}
#endif

int do_final_script()
{
	if(mapreg_dirty>=0)
		script_save_mapreg();

	if(mapreg_db)
		numdb_final(mapreg_db,mapreg_db_final);
	if(mapregstr_db)
		strdb_final(mapregstr_db,mapregstr_db_final);
	if(scriptlabel_db)
		strdb_final(scriptlabel_db,scriptlabel_db_final);
	if(userfunc_db)
		strdb_final(userfunc_db,userfunc_db_final);
	if(sleep_db) {
		struct linkdb_node *n = (struct linkdb_node *)sleep_db;
		while(n) {
			struct script_state *st = (struct script_state *)n->data;
			script_free_stack(st->stack);
			free(st);
			n = n->next;
		}
		linkdb_final(&sleep_db);
	}

#ifdef DEBUG_VARS
	if( vars_db ) {
		FILE *fp = fopen("variables.txt", "wt");
		if( fp ) {
			int i;
			int *vlist = aMalloc( vars_count * sizeof(int) );
			int count  = 0;
			numdb_foreach(vars_db, varlist_sort1, &count, vlist);
			qsort( vlist, count, sizeof(int), varlist_sort2 );

			printf("do_final_script: enum used variables\n");
			for(i = 0; i < count; i++) {
				char *name = str_buf + str_data[ vlist[i] ].str;
				struct vars_info *v = numdb_search( vars_db, (void*)vlist[i] );
				if( strncmp(name, "$@__", 4) != 0 ) {
					// switch の内部変数は除外
					fprintf(fp, "%-20s % 4d %s line %d\n", name, v->use_count, v->file, v->line);
				}
			}
			aFree( vlist );
			fclose(fp);
		}
		numdb_final(vars_db, varsdb_final );
	}
#endif

	free(str_buf);
	free(str_data);

#if !defined(NO_CSVDB) && !defined(NO_CSVDB_SCRIPT)
	script_csvfinal();
#endif

	return 0;
}
/*==========================================
 * 初期化
 *------------------------------------------
 */
int do_init_script()
{
	mapreg_db=numdb_init();
	mapregstr_db=numdb_init();
	script_load_mapreg();

	add_timer_func_list(script_autosave_mapreg,"script_autosave_mapreg");
	add_timer_interval(gettick()+MAPREG_AUTOSAVE_INTERVAL,
		script_autosave_mapreg,0,0,MAPREG_AUTOSAVE_INTERVAL);

	// do init が呼ばれる順番が違う場合の対策
	if(scriptlabel_db) {
		strdb_final(scriptlabel_db,scriptlabel_final);
	}
	scriptlabel_db=strdb_init(50);

#if !defined(NO_CSVDB) && !defined(NO_CSVDB_SCRIPT)
	script_csvinit();
#endif

#ifdef DEBUG_VARS
	vars_db = numdb_init();
#endif

	return 0;
}

//
// 埋め込み関数
//

int buildin_mes(struct script_state *st);
int buildin_goto(struct script_state *st);
int buildin_callsub(struct script_state *st);
int buildin_callfunc(struct script_state *st);
int buildin_return(struct script_state *st);
int buildin_getarg(struct script_state *st);
int buildin_next(struct script_state *st);
int buildin_close(struct script_state *st);
int buildin_close2(struct script_state *st);
int buildin_menu(struct script_state *st);
int buildin_rand(struct script_state *st);
int buildin_warp(struct script_state *st);
int buildin_areawarp(struct script_state *st);
int buildin_heal(struct script_state *st);
int buildin_itemheal(struct script_state *st);
int buildin_percentheal(struct script_state *st);
int buildin_jobchange(struct script_state *st);
int buildin_input(struct script_state *st);
int buildin_setlook(struct script_state *st);
int buildin_getlook(struct script_state *st);
int buildin_set(struct script_state *st);
int buildin_setarray(struct script_state *st);
int buildin_cleararray(struct script_state *st);
int buildin_copyarray(struct script_state *st);
int buildin_getarraysize(struct script_state *st);
int buildin_deletearray(struct script_state *st);
int buildin_getelementofarray(struct script_state *st);
int buildin_getitem(struct script_state *st);
int buildin_getitem2(struct script_state *st);
int buildin_delitem(struct script_state *st);
int buildin_viewpoint(struct script_state *st);
int buildin_countitem(struct script_state *st);
int buildin_countcartitem(struct script_state *st);
int buildin_checkweight(struct script_state *st);
int buildin_readparam(struct script_state *st);
int buildin_getcharid(struct script_state *st);
int buildin_getcharname(struct script_state *st);
int buildin_getpartyname(struct script_state *st);
int buildin_getpartymember(struct script_state *st);
int buildin_getguildname(struct script_state *st);
int buildin_getguildmaster(struct script_state *st);
int buildin_getguildmasterid(struct script_state *st);
int buildin_strcharinfo(struct script_state *st);
int buildin_getequipid(struct script_state *st);
int buildin_getequipname(struct script_state *st);
int buildin_getequipisequiped(struct script_state *st);
int buildin_getequipisenableref(struct script_state *st);
int buildin_getequipisidentify(struct script_state *st);
int buildin_getequiprefinerycnt(struct script_state *st);
int buildin_getequipweaponlv(struct script_state *st);
int buildin_getequippercentrefinery(struct script_state *st);
int buildin_successrefitem(struct script_state *st);
int buildin_failedrefitem(struct script_state *st);
int buildin_cutin(struct script_state *st);
int buildin_cutincard(struct script_state *st);
int buildin_statusup(struct script_state *st);
int buildin_statusup2(struct script_state *st);
int buildin_bonus(struct script_state *st);
int buildin_bonus2(struct script_state *st);
int buildin_bonus3(struct script_state *st);
int buildin_bonus4(struct script_state *st);
int buildin_skill(struct script_state *st);
int buildin_guildskill(struct script_state *st);
int buildin_getskilllv(struct script_state *st);
int buildin_getgdskilllv(struct script_state *st);
int buildin_basicskillcheck(struct script_state *st);
int buildin_getgmlevel(struct script_state *st);
int buildin_end(struct script_state *st);
int buildin_checkoption(struct script_state *st);
int buildin_setoption(struct script_state *st);
int buildin_setcart(struct script_state *st);
int buildin_setfalcon(struct script_state *st);
int buildin_setriding(struct script_state *st);
int buildin_savepoint(struct script_state *st);
int buildin_gettimetick(struct script_state *st);
int buildin_gettime(struct script_state *st);
int buildin_gettimestr(struct script_state *st);
int buildin_openstorage(struct script_state *st);
int buildin_guildopenstorage(struct script_state *st);
int buildin_itemskill(struct script_state *st);
int buildin_produce(struct script_state *st);
int buildin_monster(struct script_state *st);
int buildin_areamonster(struct script_state *st);
int buildin_killmonster(struct script_state *st);
int buildin_killmonsterall(struct script_state *st);
int buildin_areakillmonster(struct script_state *st);
int buildin_doevent(struct script_state *st);
int buildin_donpcevent(struct script_state *st);
int buildin_addtimer(struct script_state *st);
int buildin_deltimer(struct script_state *st);
int buildin_addtimercount(struct script_state *st);
int buildin_initnpctimer(struct script_state *st);
int buildin_stopnpctimer(struct script_state *st);
int buildin_startnpctimer(struct script_state *st);
int buildin_setnpctimer(struct script_state *st);
int buildin_getnpctimer(struct script_state *st);
int buildin_announce(struct script_state *st);
int buildin_mapannounce(struct script_state *st);
int buildin_areaannounce(struct script_state *st);
int buildin_getusers(struct script_state *st);
int buildin_getmapusers(struct script_state *st);
int buildin_getareausers(struct script_state *st);
int buildin_getareadropitem(struct script_state *st);
int buildin_enablenpc(struct script_state *st);
int buildin_disablenpc(struct script_state *st);
int buildin_hideoffnpc(struct script_state *st);
int buildin_hideonnpc(struct script_state *st);
int buildin_sc_start(struct script_state *st);
int buildin_sc_start2(struct script_state *st);
int buildin_sc_starte(struct script_state *st);
int buildin_sc_start3(struct script_state *st);
int buildin_sc_end(struct script_state *st);
int buildin_sc_ison(struct script_state *st);
int buildin_getscrate(struct script_state *st);
int buildin_debugmes(struct script_state *st);
int buildin_catchpet(struct script_state *st);
int buildin_birthpet(struct script_state *st);
int buildin_resetstatus(struct script_state *st);
int buildin_resetskill(struct script_state *st);
int buildin_changebase(struct script_state *st);
int buildin_changesex(struct script_state *st);
int buildin_waitingroom(struct script_state *st);
int buildin_delwaitingroom(struct script_state *st);
int buildin_kickwaitingroom(struct script_state *st);
int buildin_kickwaitingroomall(struct script_state *st);
int buildin_enablewaitingroomevent(struct script_state *st);
int buildin_disablewaitingroomevent(struct script_state *st);
int buildin_getwaitingroomstate(struct script_state *st);
int buildin_warpwaitingpc(struct script_state *st);
int buildin_getwaitingpcid(struct script_state *st);
int buildin_attachrid(struct script_state *st);
int buildin_detachrid(struct script_state *st);
int buildin_isloggedin(struct script_state *st);
int buildin_setmapflagnosave(struct script_state *st);
int buildin_setmapflag(struct script_state *st);
int buildin_removemapflag(struct script_state *st);
int buildin_pvpon(struct script_state *st);
int buildin_pvpoff(struct script_state *st);
int buildin_gvgon(struct script_state *st);
int buildin_gvgoff(struct script_state *st);
int buildin_emotion(struct script_state *st);
int buildin_maprespawnguildid(struct script_state *st);
int buildin_agitstart(struct script_state *st);		// <Agit>
int buildin_agitend(struct script_state *st);
int buildin_agitcheck(struct script_state *st);		// <Agitcheck>
int buildin_flagemblem(struct script_state *st);	// Flag Emblem
int buildin_getcastlename(struct script_state *st);
int buildin_getcastledata(struct script_state *st);
int buildin_setcastledata(struct script_state *st);
int buildin_requestguildinfo(struct script_state *st);
int buildin_getequipcardcnt(struct script_state *st);
int buildin_successremovecards(struct script_state *st);
int buildin_failedremovecards(struct script_state *st);
int buildin_marriage(struct script_state *st);
int buildin_wedding_effect(struct script_state *st);
int buildin_divorce(struct script_state *st);
int buildin_getitemname(struct script_state *st);
int buildin_makepet(struct script_state *st);
int buildin_getinventorylist(struct script_state *st);
int buildin_getskilllist(struct script_state *st);
int buildin_clearitem(struct script_state *st);
int buildin_getrepairableitemcount(struct script_state *st);
int buildin_repairitem(struct script_state *st);
int buildin_classchange(struct script_state *st);
int buildin_misceffect(struct script_state *st);
int buildin_areamisceffect(struct script_state *st);
int buildin_soundeffect(struct script_state *st);
int buildin_areasoundeffect(struct script_state *st);
int buildin_gmcommand(struct script_state *st);
int buildin_getusersname(struct script_state *st);
int buildin_dispbottom(struct script_state *st);
int buildin_recovery(struct script_state *st);
int buildin_getpetinfo(struct script_state *st);
int buildin_gethomuninfo(struct script_state *st);
int buildin_checkequipedcard(struct script_state *st);
int buildin_globalmes(struct script_state *st);
int buildin_jump_zero(struct script_state *st);
int buildin_select(struct script_state *st);
int buildin_getmapmobs(struct script_state *st);
int buildin_getareamobs(struct script_state *st);
int buildin_getguildrelation(struct script_state *st);
int buildin_unequip(struct script_state *st);
int buildin_allowuseitem(struct script_state *st);
int buildin_equippeditem(struct script_state *st);
int buildin_getmapname(struct script_state *st);
int buildin_summon(struct script_state *st);	// [celest] from EA
int buildin_getmapxy(struct script_state *st);	//get map position for player/npc/pet/mob by Lorky [Lupus] from EA
int buildin_checkcart(struct script_state *st);	// check cart [Valaris] from EA
int buildin_checkfalcon(struct script_state *st);	// check falcon [Valaris] from EA
int buildin_checkriding(struct script_state *st);	// check for pecopeco [Valaris] from EA
int buildin_adoption(struct script_state *st);
int buildin_petskillattack(struct script_state *st);	// pet skill attacks [Skotlex]
int buildin_petskillsupport(struct script_state *st);	// pet support skill [Valaris]
int buildin_changepettype(struct script_state *st);
int buildin_making(struct script_state *st);
int buildin_getpkflag(struct script_state *st);
int buildin_guildgetexp(struct script_state *st);
int buildin_flagname(struct script_state *st);
int buildin_getnpcposition(struct script_state *st);
#if !defined(NO_CSVDB) && !defined(NO_CSVDB_SCRIPT)
int buildin_csvgetrows(struct script_state *st);
int buildin_csvgetcols(struct script_state *st);
int buildin_csvread(struct script_state *st);
int buildin_csvreadarray(struct script_state *st);
int buildin_csvfind(struct script_state *st);
int buildin_csvwrite(struct script_state *st);
int buildin_csvwritearray(struct script_state *st);
int buildin_csvreload(struct script_state *st);
int buildin_csvinsert(struct script_state *st);
int buildin_csvdelete(struct script_state *st);
int buildin_csvsort(struct script_state *st);
int buildin_csvflush(struct script_state *st);
#endif
int buildin_sleep(struct script_state *st);
int buildin_sleep2(struct script_state *st);
int buildin_awake(struct script_state *st);
int buildin_getvariableofnpc(struct script_state *st);
int buildin_changeviewsize(struct script_state *st);
int buildin_usediteminfo(struct script_state *st);
int buildin_getitemid(struct script_state *st);
int buildin_getguildmembers(struct script_state *st);
int buildin_npcskillattack(struct script_state *st);
int buildin_npcskillsupport(struct script_state *st);
int buildin_npcskillpos(struct script_state *st);
int buildin_strnpcinfo(struct script_state *st);
int buildin_getpartyleader(struct script_state *st);
int buildin_strcut(struct script_state *st);
int buildin_setusescript(struct script_state *st);
int buildin_setequipscript(struct script_state *st);
int buildin_distance(struct script_state *st);
int buildin_homundel(struct script_state *st);
int buildin_homunrename(struct script_state *st);

struct script_function buildin_func[] = {
	{buildin_mes,"mes","s"},
	{buildin_next,"next",""},
	{buildin_close,"close",""},
	{buildin_close2,"close2",""},
	{buildin_menu,"menu","*"},
	{buildin_goto,"goto","l"},
	{buildin_callsub,"callsub","i*"},
	{buildin_callfunc,"callfunc","s*"},
	{buildin_return,"return","*"},
	{buildin_getarg,"getarg","i"},
	{buildin_jobchange,"jobchange","i*"},
	{buildin_input,"input","*"},
	{buildin_warp,"warp","sii"},
	{buildin_areawarp,"areawarp","siiiisii"},
	{buildin_setlook,"setlook","ii"},
	{buildin_getlook,"getlook","i"},
	{buildin_set,"set","ii"},
	{buildin_setarray,"setarray","ii*"},
	{buildin_cleararray,"cleararray","iii"},
	{buildin_copyarray,"copyarray","iii"},
	{buildin_getarraysize,"getarraysize","i"},
	{buildin_deletearray,"deletearray","ii"},
	{buildin_getelementofarray,"getelementofarray","ii"},
	{buildin_getitem,"getitem","ii**"},
	{buildin_getitem2,"getitem2","iiiiiiiii*"},
	{buildin_delitem,"delitem","ii"},
	{buildin_cutin,"cutin","si"},
	{buildin_cutincard,"cutincard","i"},
	{buildin_viewpoint,"viewpoint","iiiii"},
	{buildin_heal,"heal","ii"},
	{buildin_itemheal,"itemheal","ii"},
	{buildin_percentheal,"percentheal","ii"},
	{buildin_rand,"rand","i*"},
	{buildin_countitem,"countitem","i"},
	{buildin_countcartitem,"countcartitem","i"},
	{buildin_checkweight,"checkweight","ii"},
	{buildin_readparam,"readparam","i*"},
	{buildin_getcharid,"getcharid","i*"},
	{buildin_getcharname,"getcharname","i*"},
	{buildin_getpartyname,"getpartyname","i"},
	{buildin_getpartymember,"getpartymember","i"},
	{buildin_getguildname,"getguildname","i"},
	{buildin_getguildmaster,"getguildmaster","i"},
	{buildin_getguildmasterid,"getguildmasterid","i"},
	{buildin_strcharinfo,"strcharinfo","i"},
	{buildin_getequipid,"getequipid","i"},
	{buildin_getequipname,"getequipname","i"},
	{buildin_getequipisequiped,"getequipisequiped","i"},
	{buildin_getequipisenableref,"getequipisenableref","i"},
	{buildin_getequipisidentify,"getequipisidentify","i"},
	{buildin_getequiprefinerycnt,"getequiprefinerycnt","i"},
	{buildin_getequipweaponlv,"getequipweaponlv","i"},
	{buildin_getequippercentrefinery,"getequippercentrefinery","i"},
	{buildin_successrefitem,"successrefitem","i"},
	{buildin_failedrefitem,"failedrefitem","i"},
	{buildin_statusup,"statusup","i"},
	{buildin_statusup2,"statusup2","ii"},
	{buildin_bonus,"bonus","ii"},
	{buildin_bonus2,"bonus2","iii"},
	{buildin_bonus3,"bonus3","iiii"},
	{buildin_bonus4,"bonus4","iiiii"},
	{buildin_skill,"skill","ii*"},
	{buildin_guildskill,"guildskill","ii*"},
	{buildin_getskilllv,"getskilllv","i"},
	{buildin_getgdskilllv,"getgdskilllv","ii"},
	{buildin_basicskillcheck,"basicskillcheck","*"},
	{buildin_getgmlevel,"getgmlevel","*"},
	{buildin_end,"end",""},
	{buildin_checkoption,"checkoption","i"},
	{buildin_setoption,"setoption","i"},
	{buildin_setcart,"setcart",""},
	{buildin_setfalcon,"setfalcon",""},
	{buildin_setriding,"setriding",""},
	{buildin_savepoint,"savepoint","sii"},
	{buildin_gettimetick,"gettimetick","i"},
	{buildin_gettime,"gettime","i"},
	{buildin_gettimestr,"gettimestr","si"},
	{buildin_openstorage,"openstorage",""},
	{buildin_guildopenstorage,"guildopenstorage","*"},
	{buildin_itemskill,"itemskill","iis*"},
	{buildin_produce,"produce","i"},
	{buildin_monster,"monster","siissi*"},
	{buildin_areamonster,"areamonster","siiiissi*"},
	{buildin_killmonster,"killmonster","ss"},
	{buildin_killmonsterall,"killmonsterall","s"},
	{buildin_areakillmonster,"areakillmonster","siiii"},
	{buildin_doevent,"doevent","s"},
	{buildin_donpcevent,"donpcevent","s"},
	{buildin_addtimer,"addtimer","is"},
	{buildin_deltimer,"deltimer","s"},
	{buildin_addtimercount,"addtimercount","si"},
	{buildin_initnpctimer,"initnpctimer","*"},
	{buildin_stopnpctimer,"stopnpctimer","*"},
	{buildin_startnpctimer,"startnpctimer","*"},
	{buildin_setnpctimer,"setnpctimer","*"},
	{buildin_getnpctimer,"getnpctimer","i*"},
	{buildin_announce,"announce","si*"},
	{buildin_mapannounce,"mapannounce","ssi*"},
	{buildin_areaannounce,"areaannounce","siiiisi*"},
	{buildin_getusers,"getusers","i"},
	{buildin_getmapusers,"getmapusers","s"},
	{buildin_getareausers,"getareausers","siiii"},
	{buildin_getareadropitem,"getareadropitem","siiiii"},
	{buildin_enablenpc,"enablenpc","*"},
	{buildin_disablenpc,"disablenpc","*"},
	{buildin_hideoffnpc,"hideoffnpc","*"},
	{buildin_hideonnpc,"hideonnpc","*"},
	{buildin_sc_start,"sc_start","iii*"},
	{buildin_sc_start2,"sc_start2","iiii*"},
	{buildin_sc_starte,"sc_starte","iii*"},
	{buildin_sc_start3,"sc_start3","iiiiiii*"},
	{buildin_sc_end,"sc_end","i*"},
	{buildin_sc_ison,"sc_ison","i"},
	{buildin_getscrate,"getscrate","ii*"},
	{buildin_debugmes,"debugmes","s"},
	{buildin_catchpet,"pet","i"},
	{buildin_birthpet,"bpet",""},
	{buildin_resetstatus,"resetstatus",""},
	{buildin_resetskill,"resetskill",""},
	{buildin_changebase,"changebase","i*"},
	{buildin_changesex,"changesex",""},
	{buildin_waitingroom,"waitingroom","si*"},
	{buildin_delwaitingroom,"delwaitingroom","*"},
	{buildin_kickwaitingroom,"kickwaitingroom","i"},
	{buildin_kickwaitingroomall,"kickwaitingroomall","*"},
	{buildin_enablewaitingroomevent,"enablewaitingroomevent","*"},
	{buildin_disablewaitingroomevent,"disablewaitingroomevent","*"},
	{buildin_getwaitingroomstate,"getwaitingroomstate","i*"},
	{buildin_warpwaitingpc,"warpwaitingpc","sii*"},
	{buildin_getwaitingpcid,"getwaitingpcid","i*"},
	{buildin_attachrid,"attachrid","i"},
	{buildin_detachrid,"detachrid",""},
	{buildin_isloggedin,"isloggedin","i"},
	{buildin_setmapflag,"setmapflagnosave","ssii"},
	{buildin_setmapflag,"setmapflag","si"},
	{buildin_removemapflag,"removemapflag","si"},
	{buildin_pvpon,"pvpon","s"},
	{buildin_pvpoff,"pvpoff","s"},
	{buildin_gvgon,"gvgon","s"},
	{buildin_gvgoff,"gvgoff","s"},
	{buildin_emotion,"emotion","i*"},
	{buildin_maprespawnguildid,"maprespawnguildid","sii"},
	{buildin_agitstart,"agitstart",""},	// <Agit>
	{buildin_agitend,"agitend",""},
	{buildin_agitcheck,"agitcheck","i"},	// <Agitcheck>
	{buildin_flagemblem,"flagemblem","i"},	// Flag Emblem
	{buildin_getcastlename,"getcastlename","s"},
	{buildin_getcastledata,"getcastledata","si*"},
	{buildin_setcastledata,"setcastledata","sii"},
	{buildin_requestguildinfo,"requestguildinfo","i*"},
	{buildin_getequipcardcnt,"getequipcardcnt","i"},
	{buildin_successremovecards,"successremovecards","i"},
	{buildin_failedremovecards,"failedremovecards","ii"},
	{buildin_marriage,"marriage","s"},
	{buildin_wedding_effect,"wedding",""},
	{buildin_divorce,"divorce","*"},
	{buildin_getitemname,"getitemname","i"},
	{buildin_makepet,"makepet","i"},
	{buildin_getinventorylist,"getinventorylist",""},
	{buildin_getskilllist,"getskilllist",""},
	{buildin_clearitem,"clearitem",""},
	{buildin_getrepairableitemcount,"getrepairableitemcount","*"},
	{buildin_repairitem,"repairitem",""},
	{buildin_classchange,"classchange","ii"},
	{buildin_misceffect,"misceffect","i*"},
	{buildin_areamisceffect,"areamisceffect","siiiii"},
	{buildin_soundeffect,"soundeffect","si"},
	{buildin_areasoundeffect,"areasoundeffect","siiiisi"},
	{buildin_gmcommand,"gmcommand","s"},
	{buildin_dispbottom,"dispbottom","s"},
	{buildin_getusersname,"getusersname","*"},
	{buildin_recovery,"recovery",""},
	{buildin_getpetinfo,"getpetinfo","i"},
	{buildin_gethomuninfo,"gethomuninfo","i"},
	{buildin_checkequipedcard,"checkequipedcard","i"},
	{buildin_jump_zero,"jump_zero","ii"},
	{buildin_select,"select","*"},
	{buildin_globalmes,"globalmes","s*"},
	{buildin_getmapmobs,"getmapmobs","s"},
	{buildin_getareamobs,"getareamobs","siiii"},
	{buildin_getguildrelation,"getguildrelation","i*"},
	{buildin_unequip,"unequip","*"},
	{buildin_allowuseitem,"allowuseitem","*"},
	{buildin_equippeditem,"equippeditem","i*"},
	{buildin_getmapname,"getmapname","s"},
	{buildin_summon,"summon","si*"},	// summons a slave monster [Celest]
	{buildin_getmapxy,"getmapxy","siii*"},	//by Lorky [Lupus]
	{buildin_checkcart,"checkcart","*"},		//fixed by Lupus (added '*')
	{buildin_checkfalcon,"checkfalcon","*"},	//fixed by Lupus (fixed wrong pointer, added '*')
	{buildin_checkriding,"checkriding","*"},	//fixed by Lupus (fixed wrong pointer, added '*')
	{buildin_adoption,"adoption","ss"},
	{buildin_petskillattack,"petskillattack","iiii"},	// [Skotlex]
	{buildin_petskillsupport,"petskillsupport","iiiii"},	// [Skotlex]
	{buildin_changepettype,"changepettype","i"},
	{buildin_making,"making","ii"},
	{buildin_getpkflag,"getpkflag","s"},
	{buildin_guildgetexp,"guildgetexp","i"},
	{buildin_flagname,"flagname","s"},
	{buildin_getnpcposition,"getnpcposition","s"},
	{buildin_homundel,"homundel",""},
	{buildin_homunrename,"homunrename","s*"},
#if !defined(NO_CSVDB) && !defined(NO_CSVDB_SCRIPT)
	{buildin_csvgetrows,"csvgetrows","s"},
	{buildin_csvgetcols,"csvgetcols","si"},
	{buildin_csvread,"csvread","sii"},
	{buildin_csvreadarray,"csvreadarray","sii"},
	{buildin_csvfind,"csvfind","sii"},
	{buildin_csvwrite,"csvwrite","siis"},
	{buildin_csvwritearray,"csvwritearray","sii"},
	{buildin_csvreload,"csvreload","s"},
	{buildin_csvinsert,"csvinsert","si"},
	{buildin_csvdelete,"csvdelete","si"},
	{buildin_csvsort,"csvsort","sii"},
	{buildin_csvflush,"csvflush","s"},
#endif
	{buildin_sleep,"sleep","i"},
	{buildin_sleep2,"sleep2","i"},
	{buildin_awake,"awake","s"},
	{buildin_getvariableofnpc,"getvariableofnpc","is"},
	{buildin_changeviewsize,"changeviewsize","ii"},
	{buildin_usediteminfo,"usediteminfo","i"},
	{buildin_getitemid,"getitemid","s"},
	{buildin_getguildmembers,"getguildmembers","i"},
	{buildin_npcskillattack,"npcskillattack","iii"},
	{buildin_npcskillsupport,"npcskillsupport","ii"},
	{buildin_npcskillpos,"npcskillpos","iiii"},
	{buildin_strnpcinfo,"strnpcinfo","i"},
	{buildin_getpartyleader,"getpartyleader","i"},
	{buildin_strcut,"strcut","si"},
	{buildin_setusescript,"setusescript","is"},
	{buildin_setequipscript,"setequipscript","is"},
	{buildin_distance,"distance","i*"},
	{NULL,NULL,NULL}
};

/*==========================================
 *
 *------------------------------------------
 */
int buildin_mes(struct script_state *st)
{
	conv_str(st,& (st->stack->stack_data[st->start+2]));
	clif_scriptmes(script_rid2sd(st),st->oid,st->stack->stack_data[st->start+2].u.str);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_goto(struct script_state *st)
{
	int pos;

	if( st->stack->stack_data[st->start+2].type!=C_POS ){
		printf("script: goto: not label !\n");
		st->state=END;
		return 0;
	}

	pos=conv_num(st,& (st->stack->stack_data[st->start+2]));
	st->pos=pos;
	st->state=GOTO;
	return 0;
}

/*==========================================
 * ユーザー定義関数の呼び出し
 *------------------------------------------
 */
int buildin_callfunc(struct script_state *st)
{
	struct script_code *scr, *oldscr;
	char *str=conv_str(st,& (st->stack->stack_data[st->start+2]));

	if( (scr=strdb_search(script_get_userfunc_db(),str)) ){
		int i,j;
		struct linkdb_node **oldval = st->stack->var_function;
		for(i=st->start+3,j=0;i<st->end;i++,j++)
			push_copy(st->stack,i);

		push_val(st->stack,C_INT,j);							// 引数の数をプッシュ
		push_val(st->stack,C_INT,st->stack->defsp);				// 現在の基準スタックポインタをプッシュ
		push_val(st->stack,C_INT,(int)st->script);				// 現在のスクリプトをプッシュ
		push_val(st->stack,C_INT,(int)st->stack->var_function);	// 現在の関数依存変数をプッシュ
		push_val(st->stack,C_RETINFO,st->pos);					// 現在のスクリプト位置をプッシュ

		oldscr = st->script;
		st->pos=0;
		st->script=scr;
		st->stack->defsp=st->start+5+j;
		st->state=GOTO;
		st->stack->var_function = (struct linkdb_node**)aCalloc(1, sizeof(struct linkdb_node*));

		// ' 変数の引き継ぎ
		for(i = 0; i < j; i++) {
			struct script_data *s = &st->stack->stack_data[st->stack->sp-6-i];
			if( s->type == C_NAME && !s->ref ) {
				char *name = str_buf+str_data[s->u.num&0x00ffffff].str;
				// '@ 変数の引き継ぎ
				if( name[0] == '\'' && name[1] == '@' ) {
					s->ref = oldval;
				} else if( name[0] == '\'' ) {
					s->ref = &oldscr->script_vars;
				}
			}
		}

	}else{
		printf("script:callfunc: function not found! [%s]\n",str);
		st->state=END;
	}
	return 0;
}
/*==========================================
 * サブルーティンの呼び出し
 *------------------------------------------
 */
int buildin_callsub(struct script_state *st)
{
	int pos  = conv_num(st,& (st->stack->stack_data[st->start+2]));
	int i,j;
	if(st->stack->stack_data[st->start+2].type != C_POS && st->stack->stack_data[st->start+2].type != C_USERFUNC_POS) {
		printf("script: callsub: not label !\n");
		st->state=END;
		return 0;
	} else {
		struct linkdb_node **oldval = st->stack->var_function;

		for(i=st->start+3,j=0;i<st->end;i++,j++) {
			push_copy(st->stack,i);
		}

		push_val(st->stack,C_INT,j);							// 引数の数をプッシュ
		push_val(st->stack,C_INT,st->stack->defsp);				// 現在の基準スタックポインタをプッシュ
		push_val(st->stack,C_INT,(int)st->script);				// 現在のスクリプトをプッシュ
		push_val(st->stack,C_INT,(int)st->stack->var_function);	// 現在の関数依存変数をプッシュ
		push_val(st->stack,C_RETINFO,st->pos);					// 現在のスクリプト位置をプッシュ

		st->pos=pos;
		st->stack->defsp=st->start+5+j;
		st->state=GOTO;
		st->stack->var_function = (struct linkdb_node**)aCalloc(1, sizeof(struct linkdb_node*));

		// ' 変数の引き継ぎ
		for(i = 0; i < j; i++) {
			struct script_data *s = &st->stack->stack_data[st->stack->sp-6-i];
			if( s->type == C_NAME && !s->ref ) {
				char *name = str_buf+str_data[s->u.num&0x00ffffff].str;
				// '@ 変数の引き継ぎ
				if( name[0] == '\'' && name[1] == '@' ) {
					s->ref = oldval;
				}
			}
		}

		return 0;
	}
}

/*==========================================
 * 引数の所得
 *------------------------------------------
 */
int buildin_getarg(struct script_state *st)
{
	int num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	int max,stsp;
	if( st->stack->defsp<5 || st->stack->stack_data[st->stack->defsp-1].type!=C_RETINFO ){
		printf("script:getarg without callfunc or callsub!\n");
		st->state=END;
		return 0;
	}
	max=conv_num(st,& (st->stack->stack_data[st->stack->defsp-5]));
	stsp=st->stack->defsp - max -5;
	if( num >= max ){
		printf("script:getarg arg1(%d) out of range(%d) !\n",num,max);
		st->state=END;
		return 0;
	}
	push_copy(st->stack,stsp+num);
	return 0;
}

/*==========================================
 * サブルーチン/ユーザー定義関数の終了
 *------------------------------------------
 */
int buildin_return(struct script_state *st)
{
	if(st->end>st->start+2){	// 戻り値有り
		struct script_data *sd;
		push_copy(st->stack,st->start+2);
		sd = &st->stack->stack_data[st->stack->sp-1];
		if(sd->type == C_NAME) {
			char *name = str_buf + str_data[sd->u.num&0x00ffffff].str;
			if( name[0] == '\'' && name[1] == '@') {
				// '@ 変数を参照渡しにすると危険なので値渡しにする
				get_val(st,sd);
			} else if( name[0] == '\'' && !sd->ref) {
				// ' 変数は参照渡しでも良いが、参照元が設定されていないと
				// 元のスクリプトの値を差してしまうので補正する。
				sd->ref = &st->script->script_vars;
			}
		}
	}
	st->state=RETFUNC;
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_next(struct script_state *st)
{
	st->state=STOP;
	clif_scriptnext(script_rid2sd(st),st->oid);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_close(struct script_state *st)
{
	st->state=END;
	clif_scriptclose(script_rid2sd(st),st->oid);
	return 0;
}
int buildin_close2(struct script_state *st)
{
	st->state=STOP;
	clif_scriptclose(script_rid2sd(st),st->oid);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_menu(struct script_state *st)
{
	char *buf;
	int len,i;
	struct map_session_data *sd = script_rid2sd(st);

	nullpo_retr(0, sd);

	if(sd->state.menu_or_input==0){
		st->state=RERUNLINE;
		sd->state.menu_or_input=1;
		if( (st->end - st->start - 2) % 2 == 1 ) {
			// 引数の数が奇数なのでエラー扱い
			printf("buildin_menu: illigal argument count(%d).\n", st->end - st->start - 2);
			sd->state.menu_or_input=0;
			st->state=END;
			return 0;
		}
		for(i=st->start+2,len=0;i<st->end;i+=2){
			conv_str(st,& (st->stack->stack_data[i]));
			len+=strlen(st->stack->stack_data[i].u.str)+1;
		}
		buf=(char *)aCalloc(len+1,sizeof(char));
		buf[0]=0;
		for(i=st->start+2,len=0;i<st->end;i+=2){
			if( st->stack->stack_data[i].u.str[0] ) {
				strcat(buf,st->stack->stack_data[i].u.str);
				strcat(buf,":");
			}
		}
		clif_scriptmenu(script_rid2sd(st),st->oid,buf);
		free(buf);
	} else if(sd->npc_menu==0xff){	// cancel
		sd->state.menu_or_input=0;
		st->state=END;
	} else {	// goto動作
		sd->state.menu_or_input=0;
		if(sd->npc_menu>0){
			//空文字メニューの補正
			for(i=st->start+2;i<=(st->start+sd->npc_menu*2) && sd->npc_menu<(st->end-st->start)/2;i+=2) {
				conv_str(st,& (st->stack->stack_data[i]));
				if(strlen(st->stack->stack_data[i].u.str) < 1)
					sd->npc_menu++;
			}
			if(sd->npc_menu >= (st->end-st->start)/2) {
				st->state=END;
				return 0;
			}
			// ragemu互換のため
			//pc_setreg(sd,add_str("l15"),sd->npc_menu);
			pc_setreg(sd,add_str("@menu"),sd->npc_menu);
			if( st->stack->stack_data[st->start+sd->npc_menu*2+1].type!=C_POS ){
				printf("script: menu: not label !\n");
				st->state=END;
				return 0;
			}
			st->pos=conv_num(st,& (st->stack->stack_data[st->start+sd->npc_menu*2+1]));
			st->state=GOTO;
		} else {
			// 不正な値なのでキャンセル扱い
			sd->state.menu_or_input=0;
			st->state=END;
		}
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_rand(struct script_state *st)
{
	int range,min,max;

	if(st->end>st->start+3){
		min=conv_num(st,& (st->stack->stack_data[st->start+2]));
		max=conv_num(st,& (st->stack->stack_data[st->start+3]));
		if(max<min){
			int tmp;
			tmp=min;
			min=max;
			max=tmp;
		}
		range=max-min+1;
		push_val(st->stack,C_INT,atn_rand()%range+min);
	} else {
		range=conv_num(st,& (st->stack->stack_data[st->start+2]));
		push_val(st->stack,C_INT,atn_rand()%range);
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_warp(struct script_state *st)
{
	int x,y;
	char *str;
	struct map_session_data *sd=script_rid2sd(st);

	nullpo_retr(0, sd);

	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	x=conv_num(st,& (st->stack->stack_data[st->start+3]));
	y=conv_num(st,& (st->stack->stack_data[st->start+4]));
	if(strcmp(str,"Random")==0)
		pc_randomwarp(sd,3);
	else if(strcmp(str,"SavePoint")==0){
		pc_setpos(sd,sd->status.save_point.map,
			sd->status.save_point.x,sd->status.save_point.y,3);
	}else
		pc_setpos(sd,str,x,y,0);

	if(unit_isdead(&sd->bl)) {
		pc_setstand(sd);
		pc_setrestartvalue(sd,3);
	}
	return 0;
}
/*==========================================
 * エリア指定ワープ
 *------------------------------------------
 */
int buildin_areawarp_sub(struct block_list *bl,va_list ap)
{
	int x,y;
	char *mapname;
	struct map_session_data *sd;

	mapname=va_arg(ap, char *);
	x=va_arg(ap,int);
	y=va_arg(ap,int);

	if((sd=(struct map_session_data *)bl) == NULL)
		return 0;

	if(strcmp(mapname,"Random")==0)
		pc_randomwarp(sd,3);
	else if(strcmp(mapname,"SavePoint")==0)
		pc_setpos(sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,3);
	else
		pc_setpos(sd,mapname,x,y,0);

	if(unit_isdead(&sd->bl)) {
		pc_setstand(sd);
		pc_setrestartvalue(sd,3);
	}
	return 0;
}
int buildin_areawarp(struct script_state *st)
{
	int x,y,m;
	char *str;
	char *mapname;
	int x0,y0,x1,y1;

	mapname=conv_str(st,& (st->stack->stack_data[st->start+2]));
	x0=conv_num(st,& (st->stack->stack_data[st->start+3]));
	y0=conv_num(st,& (st->stack->stack_data[st->start+4]));
	x1=conv_num(st,& (st->stack->stack_data[st->start+5]));
	y1=conv_num(st,& (st->stack->stack_data[st->start+6]));
	str=conv_str(st,& (st->stack->stack_data[st->start+7]));
	x=conv_num(st,& (st->stack->stack_data[st->start+8]));
	y=conv_num(st,& (st->stack->stack_data[st->start+9]));

	if(strcmp(mapname,"this")==0){
		struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);
		if(nd)
			m=nd->bl.m;
		else {
			struct map_session_data *sd=script_rid2sd(st);
			if(sd)
				m=sd->bl.m;
			else
				return 0;
		}
	}
	else if( (m=map_mapname2mapid(mapname))< 0)
		return 0;

	map_foreachinarea(buildin_areawarp_sub,
		m,x0,y0,x1,y1,BL_PC,	str,x,y );
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_heal(struct script_state *st)
{
	int hp,sp;

	hp=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sp=conv_num(st,& (st->stack->stack_data[st->start+3]));
	pc_heal(script_rid2sd(st),hp,sp);
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int buildin_itemheal(struct script_state *st)
{
	int hp,sp;

	hp=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sp=conv_num(st,& (st->stack->stack_data[st->start+3]));
	pc_itemheal(script_rid2sd(st),hp,sp);
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int buildin_percentheal(struct script_state *st)
{
	int hp,sp;

	hp=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sp=conv_num(st,& (st->stack->stack_data[st->start+3]));
	pc_percentheal(script_rid2sd(st),hp,sp);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_jobchange(struct script_state *st)
{
	int job, upper=-1;

	job=conv_num(st,& (st->stack->stack_data[st->start+2]));
	if( st->end>st->start+3 )
		upper=conv_num(st,& (st->stack->stack_data[st->start+3]));

	if(job>=MAX_VALID_PC_CLASS)
		return 0;

	if(job>=24)
		upper = 0;

	if ((job >= 0 && job < MAX_VALID_PC_CLASS))
		pc_jobchange(script_rid2sd(st),job, upper);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_input(struct script_state *st)
{
	struct map_session_data *sd = script_rid2sd(st);
	int num = (st->end>st->start+2)? st->stack->stack_data[st->start+2].u.num: 0;
	char *name = (st->end>st->start+2)? str_buf+str_data[num&0x00ffffff].str: "";
	char postfix=name[strlen(name)-1];

	nullpo_retr(0, sd);

	if(sd->state.menu_or_input){
		sd->state.menu_or_input=0;
		if( postfix=='$' ){
			// 文字列
			if(st->end>st->start+2){ // 引数1個
				set_reg(st,sd,num,name,(void*)sd->npc_str,st->stack->stack_data[st->start+2].ref);
			}else{
				printf("buildin_input: string discarded !!\n");
			}
		}else{
			// 数値
			if(st->end>st->start+2){ // 引数1個
				set_reg(st,sd,num,name,(void*)sd->npc_amount,st->stack->stack_data[st->start+2].ref);
			} else {
				// ragemu互換のため
				//pc_setreg(sd,add_str("l14"),sd->npc_amount);
			}
		}
	} else {
		st->state=RERUNLINE;
		if(postfix=='$')clif_scriptinputstr(sd,st->oid);
		else			clif_scriptinput(sd,st->oid);
		sd->state.menu_or_input=1;
	}
	return 0;
}

/*==========================================
 * 変数設定
 *------------------------------------------
 */
int buildin_set(struct script_state *st)
{
	struct map_session_data *sd=NULL;
	int num=st->stack->stack_data[st->start+2].u.num;
	char *name=str_buf+str_data[num&0x00ffffff].str;
	char prefix=*name;
	char postfix=name[strlen(name)-1];

	if( st->stack->stack_data[st->start+2].type!=C_NAME ){
		printf("script: buildin_set: not name\n");
		return 0;
	}

	if( prefix!='$' && prefix!='\'' )
		sd=script_rid2sd(st);

	if( postfix=='$' ){
		// 文字列
		char *str = conv_str(st,& (st->stack->stack_data[st->start+3]));
		set_reg(st,sd,num,name,(void*)str,st->stack->stack_data[st->start+2].ref);
	}else{
		// 数値
		int val = conv_num(st,& (st->stack->stack_data[st->start+3]));
		set_reg(st,sd,num,name,(void*)val,st->stack->stack_data[st->start+2].ref);
	}

	return 0;
}

/*==========================================
 * 配列変数設定
 *------------------------------------------
 */
int buildin_setarray(struct script_state *st)
{
	struct map_session_data *sd=NULL;
	int num=st->stack->stack_data[st->start+2].u.num;
	char *name=str_buf+str_data[num&0x00ffffff].str;
	char prefix=*name;
	char postfix=name[strlen(name)-1];
	int i,j;

	if( prefix!='$' && prefix!='@' && prefix!='\''){
		printf("buildin_setarray: illeagal scope !\n");
		return 0;
	}
	if( prefix!='$' && prefix!='\'' )
		sd=script_rid2sd(st);

	for(j=0,i=st->start+3; i<st->end && j<128;i++,j++){
		void *v;
		if( postfix=='$' )
			v=(void*)conv_str(st,& (st->stack->stack_data[i]));
		else
			v=(void*)conv_num(st,& (st->stack->stack_data[i]));
		set_reg( st, sd, num+(j<<24), name, v, st->stack->stack_data[st->start+2].ref);
	}
	return 0;
}
/*==========================================
 * 配列変数クリア
 *------------------------------------------
 */
int buildin_cleararray(struct script_state *st)
{
	struct map_session_data *sd=NULL;
	int num=st->stack->stack_data[st->start+2].u.num;
	char *name=str_buf+str_data[num&0x00ffffff].str;
	char prefix=*name;
	char postfix=name[strlen(name)-1];
	int sz=conv_num(st,& (st->stack->stack_data[st->start+4]));
	int i;
	void *v;

	if( prefix!='$' && prefix!='@' && prefix!='\''){
		printf("buildin_cleararray: illeagal scope !\n");
		return 0;
	}
	if( prefix!='$' && prefix!='\'')
		sd=script_rid2sd(st);

	if( postfix=='$' )
		v=(void*)conv_str(st,& (st->stack->stack_data[st->start+3]));
	else
		v=(void*)conv_num(st,& (st->stack->stack_data[st->start+3]));

	for(i=0;i<sz;i++)
		set_reg(st,sd,num+(i<<24),name,v,st->stack->stack_data[st->start+2].ref);
	return 0;
}
/*==========================================
 * 配列変数コピー
 *------------------------------------------
 */
int buildin_copyarray(struct script_state *st)
{
	struct map_session_data *sd=NULL;
	int num=st->stack->stack_data[st->start+2].u.num;
	char *name=str_buf+str_data[num&0x00ffffff].str;
	char prefix=*name;
	char postfix=name[strlen(name)-1];
	int num2=st->stack->stack_data[st->start+3].u.num;
	char *name2=str_buf+str_data[num2&0x00ffffff].str;
	char prefix2=*name2;
	char postfix2=name2[strlen(name2)-1];
	int sz=conv_num(st,& (st->stack->stack_data[st->start+4]));
	int i;

	if( prefix!='$' && prefix!='@' && prefix!='\'' ){
		printf("buildin_copyarray: illeagal scope !\n");
		return 0;
	}
	if( prefix2!='$' && prefix2!='@' && prefix2!='\'' ) {
		printf("buildin_copyarray: illeagal scope !\n");
		return 0;
	}
	if( (postfix=='$' || postfix2=='$') && postfix!=postfix2 ){
		printf("buildin_copyarray: type mismatch !\n");
		return 0;
	}
	if( (prefix!='$' && prefix != '\'') || (prefix2!='$' && prefix2 != '\'') )
		sd=script_rid2sd(st);

	if((num & 0x00FFFFFF) == (num2 & 0x00FFFFFF) && (num & 0xFF000000) > (num2 & 0xFF000000)) {
		// 同じ配列で、num > num2 の場合大きい方からコピーしないといけない
		for(i=sz-1;i>=0;i--)
			set_reg(
				st,sd,num+(i<<24),name,
				get_val2(st,num2+(i<<24),st->stack->stack_data[st->start+3].ref),
				st->stack->stack_data[st->start+2].ref
			);
	} else {
		for(i=0;i<sz;i++)
			set_reg(
				st,sd,num+(i<<24),name,
				get_val2(st,num2+(i<<24),st->stack->stack_data[st->start+3].ref),
				st->stack->stack_data[st->start+2].ref
			);
	}
	return 0;
}
/*==========================================
 * 配列変数のサイズ所得
 *------------------------------------------
 */
static int getarraysize(struct script_state *st,int num,int postfix,struct linkdb_node** ref)
{
	int i=(num>>24),c=(i==0? -1:i);
	for(;i<128;i++){
		void *v=get_val2(st,(num & 0x00FFFFFF)+(i<<24),ref);
		if(postfix=='$' && *((char*)v) ) c=i;
		if(postfix!='$' && (int)v )c=i;
	}
	return c+1;
}
int buildin_getarraysize(struct script_state *st)
{
	int num=st->stack->stack_data[st->start+2].u.num;
	char *name=str_buf+str_data[num&0x00ffffff].str;
	char prefix=*name;
	char postfix=name[strlen(name)-1];

	if( prefix!='$' && prefix!='@' && prefix!='\''){
		printf("buildin_getarraysize: illeagal scope !\n");
		return 0;
	}

	push_val(st->stack,C_INT,getarraysize(st,num,postfix,st->stack->stack_data[st->start+2].ref) );
	return 0;
}
/*==========================================
 * 配列変数から要素削除
 *------------------------------------------
 */
int buildin_deletearray(struct script_state *st)
{
	struct map_session_data *sd=NULL;
	int num=st->stack->stack_data[st->start+2].u.num;
	char *name=str_buf+str_data[num&0x00ffffff].str;
	char prefix=*name;
	char postfix=name[strlen(name)-1];
	int count=1;
	int i,j,sz;

	if( (st->end > st->start+3) )
		count=conv_num(st,& (st->stack->stack_data[st->start+3]));
	sz = getarraysize(st,num,postfix,st->stack->stack_data[st->start+2].ref)-(num>>24)-count;

	if( prefix!='$' && prefix!='@' && prefix!='\''){
		printf("buildin_deletearray: illeagal scope !\n");
		return 0;
	}
	if( prefix!='$' && prefix != '\'')
		sd=script_rid2sd(st);

	for(i=0;i<sz;i++){
		set_reg(
			st,sd,num+(i<<24),name,
			get_val2(st,num+((i+count)<<24),st->stack->stack_data[st->start+2].ref),
			st->stack->stack_data[st->start+2].ref
		);
	}
	for(j=0;j<count;j++){
		if( postfix!='$' ) set_reg(st,sd,num+((i+j)<<24),name, 0,st->stack->stack_data[st->start+2].ref);
		if( postfix=='$' ) set_reg(st,sd,num+((i+j)<<24),name, "",st->stack->stack_data[st->start+2].ref);
	}
	return 0;
}

/*==========================================
 * 指定要素を表す値(キー)を所得する
 *------------------------------------------
 */
int buildin_getelementofarray(struct script_state *st)
{
	if( st->stack->stack_data[st->start+2].type==C_NAME ){
		int i=conv_num(st,& (st->stack->stack_data[st->start+3]));
		if(i>127 || i<0){
			printf("script: getelementofarray (operator[]): param2 illeagal number %d\n",i);
			push_val(st->stack,C_INT,0);
		}else{
			push_val2(st->stack,C_NAME,
				(i<<24) | st->stack->stack_data[st->start+2].u.num,
				st->stack->stack_data[st->start+2].ref);
		}
	}else{
		printf("script: getelementofarray (operator[]): param1 not name !\n");
		push_val(st->stack,C_INT,0);
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_setlook(struct script_state *st)
{
	int type,val;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	val=conv_num(st,& (st->stack->stack_data[st->start+3]));

	pc_changelook(script_rid2sd(st),type,val);

	return 0;
}

/*==========================================
 * 見た目のパラメータを返す
 *   n：1,髪型、2,武器、3,頭上段、4,頭中段、5,頭下段、6,髪色、7,服色、8,盾
 *------------------------------------------
 */
int buildin_getlook(struct script_state *st)
{
	int type,val= -1;
	struct map_session_data *sd=script_rid2sd(st);
	type=conv_num(st,& (st->stack->stack_data[st->start+2]));

	if(sd==NULL){
		push_val(st->stack,C_INT,-1);
		return 0;
	}
	switch(type){
		case LOOK_HAIR:
			val=sd->status.hair;
			break;
		case LOOK_WEAPON:
			val=sd->status.weapon;
			break;
		case LOOK_HEAD_BOTTOM:
			val=sd->status.head_bottom;
			break;
		case LOOK_HEAD_TOP:
			val=sd->status.head_top;
			break;
		case LOOK_HEAD_MID:
			val=sd->status.head_mid;
			break;
		case LOOK_HAIR_COLOR:
			val=sd->status.hair_color;
			break;
		case LOOK_CLOTHES_COLOR:
			val=sd->status.clothes_color;
			break;
		case LOOK_SHIELD:
			val=sd->status.shield;
			break;
		case LOOK_SHOES:
			break;
		case LOOK_MOB:
			break;
	}
	push_val(st->stack,C_INT,val);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_cutin(struct script_state *st)
{
	int type;

	conv_str(st,& (st->stack->stack_data[st->start+2]));
	type=conv_num(st,& (st->stack->stack_data[st->start+3]));

	clif_cutin(script_rid2sd(st),st->stack->stack_data[st->start+2].u.str,type);

	return 0;
}
/*==========================================
 * カードのイラストを表示する
 *------------------------------------------
 */
int buildin_cutincard(struct script_state *st)
{
	int itemid;

	itemid=conv_num(st,& (st->stack->stack_data[st->start+2]));

	clif_cutin(script_rid2sd(st),itemdb_search(itemid)->cardillustname,4);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_viewpoint(struct script_state *st)
{
	int type,x,y,id,color;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	x=conv_num(st,& (st->stack->stack_data[st->start+3]));
	y=conv_num(st,& (st->stack->stack_data[st->start+4]));
	id=conv_num(st,& (st->stack->stack_data[st->start+5]));
	color=conv_num(st,& (st->stack->stack_data[st->start+6]));

	clif_viewpoint(script_rid2sd(st),st->oid,type,x,y,id,color);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_countitem(struct script_state *st)
{
	int nameid,count,i;
	struct map_session_data *sd = script_rid2sd(st);
	struct script_data *data;

	nullpo_retr(0, sd);

	data=&(st->stack->stack_data[st->start+2]);
	get_val(st,data);
	if( isstr(data) ){
		const char *name=conv_str(st,data);
		struct item_data *item_data = itemdb_searchname(name);
		nameid=512;
		if( item_data )
			nameid=item_data->nameid;
	}else
		nameid=conv_num(st,data);

	for(i=0,count=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid==nameid)
			count+=sd->status.inventory[i].amount;
	}

	push_val(st->stack,C_INT,count);

	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int buildin_countcartitem(struct script_state *st)
{
	int nameid,count,i;
	struct map_session_data *sd = script_rid2sd(st);
	struct script_data *data;

	nullpo_retr(0, sd);

	data=&(st->stack->stack_data[st->start+2]);
	get_val(st,data);
	if( isstr(data) ){
		const char *name=conv_str(st,data);
		struct item_data *item_data = itemdb_searchname(name);
		nameid=512;
		if( item_data )
			nameid=item_data->nameid;
	}else
		nameid=conv_num(st,data);

	for(i=0,count=0;i<MAX_CART;i++)
		if(sd->status.cart[i].nameid == nameid && sd->status.cart[i].amount)
			count+=sd->status.cart[i].amount;

	push_val(st->stack,C_INT,count);

	return 0;
}

/*==========================================
 * 重量チェック
 *------------------------------------------
 */
int buildin_checkweight(struct script_state *st)
{
	int nameid,amount;
	struct map_session_data *sd = script_rid2sd(st);
	struct script_data *data;

	nullpo_retr(0, sd);

	data=&(st->stack->stack_data[st->start+2]);
	get_val(st,data);
	if( isstr(data) ){
		const char *name=conv_str(st,data);
		struct item_data *item_data = itemdb_searchname(name);
		nameid=512;
		if( item_data )
			nameid=item_data->nameid;
	}else
		nameid=conv_num(st,data);

	amount=conv_num(st,& (st->stack->stack_data[st->start+3]));

	if(amount > MAX_AMOUNT || itemdb_weight(nameid)*amount + sd->weight > sd->max_weight || pc_search_inventory(sd,0) < 0){
		push_val(st->stack,C_INT,0);
	} else {
		push_val(st->stack,C_INT,1);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_getitem(struct script_state *st)
{
	int nameid,amount,flag = 0;
	struct item item_tmp;
	struct map_session_data *sd;
	struct script_data *data;

	sd = script_rid2sd(st);

	data=&(st->stack->stack_data[st->start+2]);
	get_val(st,data);
	if( isstr(data) ){
		const char *name=conv_str(st,data);
		struct item_data *item_data = itemdb_searchname(name);
		nameid=512;
		if( item_data )
			nameid=item_data->nameid;
	}else
		nameid=conv_num(st,data);

	amount=conv_num(st,& (st->stack->stack_data[st->start+3]));
	if( st->end>st->start+4 ) //鑑定した状態で渡すかどうか
		flag=(conv_num(st,& (st->stack->stack_data[st->start+4]))==0)?1:0;
	if(nameid<0) { // ランダム
		nameid=itemdb_searchrandomid(-nameid);
	//	flag = 1;
	}

	if(nameid > 0) {
		memset(&item_tmp,0,sizeof(item_tmp));
		item_tmp.nameid=nameid;
		if(!flag)
			item_tmp.identify=1;
		else
			item_tmp.identify=!itemdb_isequip3(nameid);
		if(battle_config.itemidentify)
			item_tmp.identify = 1;
		if( st->end>st->start+5 ) //アイテムを指定したIDに渡す
			sd=map_id2sd(conv_num(st,& (st->stack->stack_data[st->start+5])));
		if(sd == NULL) //アイテムを渡す相手がいなかったらお帰り
			return 0;
		if((flag = pc_additem(sd,&item_tmp,amount))) {
			clif_additem(sd,0,0,flag);
			if(!pc_candrop(sd,nameid))
				map_addflooritem(&item_tmp,amount,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_getitem2(struct script_state *st)
{
	int nameid,amount,flag = 0;
	int iden,ref,attr,c1,c2,c3,c4;
	struct item_data *item_data;
	struct item item_tmp;
	struct map_session_data *sd;
	struct script_data *data;

	sd = script_rid2sd(st);

	data=&(st->stack->stack_data[st->start+2]);
	get_val(st,data);
	if( isstr(data) ){
		const char *name=conv_str(st,data);
		struct item_data *item_data = itemdb_searchname(name);
		nameid=512;
		if( item_data )
			nameid=item_data->nameid;
	}else
		nameid=conv_num(st,data);

	amount=conv_num(st,& (st->stack->stack_data[st->start+3]));
	iden=conv_num(st,& (st->stack->stack_data[st->start+4]));
	ref=conv_num(st,& (st->stack->stack_data[st->start+5]));
	attr=conv_num(st,& (st->stack->stack_data[st->start+6]));
	c1=conv_num(st,& (st->stack->stack_data[st->start+7]));
	c2=conv_num(st,& (st->stack->stack_data[st->start+8]));
	c3=conv_num(st,& (st->stack->stack_data[st->start+9]));
	c4=conv_num(st,& (st->stack->stack_data[st->start+10]));
	if( st->end>st->start+11 ) //アイテムを指定したIDに渡す
		sd=map_id2sd(conv_num(st,& (st->stack->stack_data[st->start+11])));
	if(sd == NULL) //アイテムを渡す相手がいなかったらお帰り
		return 0;

	if(nameid<0) { // ランダム
		nameid=itemdb_searchrandomid(-nameid);
		flag = 1;
	}

	if(nameid > 0) {
		memset(&item_tmp,0,sizeof(item_tmp));
		item_data=itemdb_search(nameid);
		if(item_data->type==4 || item_data->type==5){
			if(ref > 10) ref = 10;
		}
		else if(item_data->type==7) {
			iden = 1;
			ref = 0;
		}
		else {
			iden = 1;
			ref = attr = 0;
		}

		item_tmp.nameid=nameid;
		if(!flag)
			item_tmp.identify=iden;
		else if(item_data->type==4 || item_data->type==5)
			item_tmp.identify=0;
		if(battle_config.itemidentify)
			item_tmp.identify = 1;
		item_tmp.refine=ref;
		item_tmp.attribute=attr;
		item_tmp.card[0]=c1;
		item_tmp.card[1]=c2;
		item_tmp.card[2]=c3;
		item_tmp.card[3]=c4;
		if((flag = pc_additem(sd,&item_tmp,amount))) {
			clif_additem(sd,0,0,flag);
			map_addflooritem(&item_tmp,amount,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_delitem(struct script_state *st)
{
	int nameid,amount,i;
	struct map_session_data *sd = script_rid2sd(st);
	struct script_data *data;

	nullpo_retr(0, sd);

	data=&(st->stack->stack_data[st->start+2]);
	get_val(st,data);
	if( isstr(data) ){
		const char *name=conv_str(st,data);
		struct item_data *item_data = itemdb_searchname(name);
		nameid=512;
		if( item_data )
			nameid=item_data->nameid;
	}else
		nameid=conv_num(st,data);

	amount=conv_num(st,& (st->stack->stack_data[st->start+3]));

	for(i=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid<=0 || sd->inventory_data[i] == NULL ||
		   sd->inventory_data[i]->type!=7 || sd->status.inventory[i].amount<=0)
			continue;
		if(sd->status.inventory[i].nameid == nameid){
			if(sd->status.inventory[i].card[0] == (short)0xff00){
				if(search_petDB_index(nameid, PET_EGG) >= 0){
					intif_delete_petdata(*((long *)(&sd->status.inventory[i].card[1])));
					break;
				}
			}
		}
	}
	for(i=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid==nameid){
			if(sd->status.inventory[i].amount>amount){
				pc_delitem(sd,i,amount,0);
				break;
			} else {
				amount-=sd->status.inventory[i].amount;
				pc_delitem(sd,i,sd->status.inventory[i].amount,0);
			}
		}
	}

	return 0;
}

/*==========================================
 *キャラ関係の装備取得
 *------------------------------------------
 */
int buildin_equippeditem(struct script_state *st)
{
	int id;
	struct map_session_data *sd;

	id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	if( st->end>st->start+3 )
		sd=map_nick2sd(conv_str(st,& (st->stack->stack_data[st->start+3])));
	else
	sd=script_rid2sd(st);

	if(sd==NULL){
		push_val(st->stack,C_INT,0);
		return 0;
	}

	push_val(st->stack,C_INT,pc_equippeditem(sd,id));

	return 0;
}
/*==========================================
 *キャラ関係のパラメータ取得
 *------------------------------------------
 */
int buildin_readparam(struct script_state *st)
{
	int type;
	struct map_session_data *sd;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	if( st->end>st->start+3 )
		sd=map_nick2sd(conv_str(st,& (st->stack->stack_data[st->start+3])));
	else
	sd=script_rid2sd(st);

	if(sd==NULL){
		push_val(st->stack,C_INT,-1);
		return 0;
	}

	push_val(st->stack,C_INT,pc_readparam(sd,type));

	return 0;
}
/*==========================================
 *キャラ関係のID取得
 *------------------------------------------
 */
int buildin_getcharid(struct script_state *st)
{
	int num;
	struct map_session_data *sd;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	if( st->end>st->start+3 )
		sd=map_nick2sd(conv_str(st,& (st->stack->stack_data[st->start+3])));
	else
		sd=script_rid2sd(st);
	if(sd==NULL){
		push_val(st->stack,C_INT,-1);
		return 0;
	}
	if(num==0)
		push_val(st->stack,C_INT,sd->status.char_id);
	if(num==1)
		push_val(st->stack,C_INT,sd->status.party_id);
	if(num==2)
		push_val(st->stack,C_INT,sd->status.guild_id);
	if(num==3)
		push_val(st->stack,C_INT,sd->status.account_id);
	return 0;
}
/*==========================================
 *指定IDのPT名取得
 *------------------------------------------
 */
char *buildin_getpartyname_sub(int party_id)
{
	struct party *p;

	p=NULL;
	p=party_search(party_id);

	if(p!=NULL){
		return aStrdup(p->name);
	}

	return 0;
}
int buildin_getpartyname(struct script_state *st)
{
	char *name;
	int party_id;

	party_id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	name=buildin_getpartyname_sub(party_id);
	if(name!=0)
		push_str(st->stack,C_STR,name);
	else
		push_str(st->stack,C_CONSTSTR,"");

	return 0;
}
/*==========================================
 *指定IDのキャラクタ名取得
 *------------------------------------------
 */
char *buildin_getcharname_sub(int char_id)
{
	struct charid2nick *c = NULL;

	c=char_search(char_id);
//	c=map_charid2nick(char_id);

	if(c!=NULL){
		return aStrdup(c->nick);
	}

	return 0;
}
int buildin_getcharname(struct script_state *st)
{
	char *name;
	int char_id;
	int type;

	char_id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	name=buildin_getcharname_sub(char_id);
	type=0;
	if( st->end>st->start+3 )	// type
		type=(conv_num(st,& (st->stack->stack_data[st->start+3]))!=0)?1:0;
//	printf("script getcharname: %d[%s] type: %d\n",char_id,name,type);
	if(type==0){
		if(name!=0)
			push_str(st->stack,C_STR,name);
		else
			push_str(st->stack,C_CONSTSTR,"");
	} else
		push_str(st->stack,C_CONSTSTR,"");

	return 0;
}
/*==========================================
 *指定IDのPT人数とメンバーID取得
 *------------------------------------------
 */
int buildin_getpartymember(struct script_state *st)
{
	struct party *p = NULL;
	int i,j=0;

	p=party_search(conv_num(st,& (st->stack->stack_data[st->start+2])));

	if(p!=NULL){
		for(i=0;i<MAX_PARTY;i++){
			if(p->member[i].account_id){
//				printf("name:%s %d\n",p->member[i].name,i);
				mapreg_setregstr(add_str("$@partymembername$")+(i<<24),p->member[i].name);
				j++;
			}
		}
	}
	mapreg_setreg(add_str("$@partymembercount"),j);

	return 0;
}
/*==========================================
 *指定IDのギルド名取得
 *------------------------------------------
 */
char *buildin_getguildname_sub(int guild_id)
{
	struct guild *g=NULL;
	g=guild_search(guild_id);

	if(g!=NULL){
		return aStrdup(g->name);
	}
	return 0;
}
int buildin_getguildname(struct script_state *st)
{
	char *name;
	int guild_id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	name=buildin_getguildname_sub(guild_id);
	if(name!=0)
		push_str(st->stack,C_STR,name);
	else
		push_str(st->stack,C_CONSTSTR,"");
	return 0;
}

/*==========================================
 *指定IDのGuildMaster名取得
 *------------------------------------------
 */
char *buildin_getguildmaster_sub(int guild_id)
{
	struct guild *g=NULL;
	g=guild_search(guild_id);

	if(g!=NULL){
		return aStrdup(g->master);
	}

	return 0;
}
int buildin_getguildmaster(struct script_state *st)
{
	char *master;
	int guild_id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	master=buildin_getguildmaster_sub(guild_id);
	if(master!=0)
		push_str(st->stack,C_STR,master);
	else
		push_str(st->stack,C_CONSTSTR,"");
	return 0;
}

int buildin_getguildmasterid(struct script_state *st)
{
	char *master;
	struct map_session_data *sd=NULL;
	int guild_id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	master=buildin_getguildmaster_sub(guild_id);
	if(master!=0){
		if((sd=map_nick2sd(master)) == NULL){
			push_val(st->stack,C_INT,0);
			return 0;
		}
		push_val(st->stack,C_INT,sd->status.char_id);
	}else{
		push_val(st->stack,C_INT,0);
	}
	return 0;
}

/*==========================================
 * キャラクタの名前
 *------------------------------------------
 */
int buildin_strcharinfo(struct script_state *st)
{
	struct map_session_data *sd;
	int num;
	char *buf;

	sd=script_rid2sd(st);
	num=conv_num(st,& (st->stack->stack_data[st->start+2]));

	if(sd) {
		switch (num) {
		case 0:
			push_str(st->stack,C_STR,aStrdup(sd->status.name));
			return 0;
		case 1:
			buf=buildin_getpartyname_sub(sd->status.party_id);
			if(buf!=0)
				push_str(st->stack,C_STR,buf);
			else
				push_str(st->stack,C_CONSTSTR,"");
			return 0;
		case 2:
			buf=buildin_getguildname_sub(sd->status.guild_id);
			if(buf!=0)
				push_str(st->stack,C_STR,buf);
			else
				push_str(st->stack,C_CONSTSTR,"");
			return 0;
		}
	}
	push_str(st->stack,C_CONSTSTR,"");
	return 0;
}

unsigned int equip[10]={0x0100,0x0010,0x0020,0x0002,0x0004,0x0040,0x0008,0x0080,0x0200,0x0001};

/*==========================================
 * GetEquipID(Pos);	 Pos: 1-10
 *------------------------------------------
 */
int buildin_getequipid(struct script_state *st)
{
	int i,num;
	struct map_session_data *sd;
	struct item_data* item;

	sd=script_rid2sd(st);
	if(sd == NULL)
	{
		printf("getequipid: sd == NULL\n");
		return 0;
	}
	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	i=pc_checkequip(sd,equip[num-1]);
	if(i >= 0){
		item=sd->inventory_data[i];
		if(item)
			push_val(st->stack,C_INT,item->nameid);
		else
			push_val(st->stack,C_INT,0);
	}else{
		push_val(st->stack,C_INT,-1);
	}
	return 0;
}

/*==========================================
 * 装備名文字列（精錬メニュー用）
 *------------------------------------------
 */
int buildin_getequipname(struct script_state *st)
{
	int i,num;
	struct map_session_data *sd;
	struct item_data* item;
	char *buf;

	buf=(char *)aCalloc(64,sizeof(char));
	sd=script_rid2sd(st);
	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	i=pc_checkequip(sd,equip[num-1]);
	if(i >= 0){
		item=sd->inventory_data[i];
		if(item)
			sprintf(buf,"%s-[%s]",pos[num-1],item->jname);
		else
			sprintf(buf,"%s-[%s]",pos[num-1],pos[10]);
	}else{
		sprintf(buf,"%s-[%s]",pos[num-1],pos[10]);
	}
	push_str(st->stack,C_STR,buf);

	return 0;
}

/*==========================================
 * 装備チェック
 *------------------------------------------
 */
int buildin_getequipisequiped(struct script_state *st)
{
	int i,num;
	struct map_session_data *sd;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=script_rid2sd(st);
	i=pc_checkequip(sd,equip[num-1]);
	if(i >= 0){
		push_val(st->stack,C_INT,1);
	}else{
		push_val(st->stack,C_INT,0);
	}

	return 0;
}

/*==========================================
 * 装備品精錬可能チェック
 *------------------------------------------
 */
int buildin_getequipisenableref(struct script_state *st)
{
	int i,num;
	struct map_session_data *sd;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=script_rid2sd(st);
	i=pc_checkequip(sd,equip[num-1]);
//	if(i >= 0 && num<7 && sd->inventory_data[i] && (num!=1 || sd->inventory_data[i]->def > 1
//				 || (sd->inventory_data[i]->def==1 && sd->inventory_data[i]->equip_script==NULL)
//				 || (sd->inventory_data[i]->def<=0 && sd->inventory_data[i]->equip_script!=NULL)){
	if(i >= 0 && sd->inventory_data[i] && (sd->inventory_data[i]->refine != 0))
		push_val(st->stack,C_INT,1);
	else
		push_val(st->stack,C_INT,0);

	return 0;
}

/*==========================================
 * 装備品鑑定チェック
 *------------------------------------------
 */
int buildin_getequipisidentify(struct script_state *st)
{
	int i,num;
	struct map_session_data *sd;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=script_rid2sd(st);
	i=pc_checkequip(sd,equip[num-1]);
	if(i >= 0)
		push_val(st->stack,C_INT,sd->status.inventory[i].identify);
	else
		push_val(st->stack,C_INT,0);

	return 0;
}

/*==========================================
 * 装備品精錬度
 *------------------------------------------
 */
int buildin_getequiprefinerycnt(struct script_state *st)
{
	int i,num;
	struct map_session_data *sd;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=script_rid2sd(st);
	i=pc_checkequip(sd,equip[num-1]);
	if(i >= 0)
		push_val(st->stack,C_INT,sd->status.inventory[i].refine);
	else
		push_val(st->stack,C_INT,0);

	return 0;
}

/*==========================================
 * 装備品武器LV
 *------------------------------------------
 */
int buildin_getequipweaponlv(struct script_state *st)
{
	int i,num;
	struct map_session_data *sd;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=script_rid2sd(st);
	i=pc_checkequip(sd,equip[num-1]);
	if(i >= 0 && sd->inventory_data[i])
		push_val(st->stack,C_INT,sd->inventory_data[i]->wlv);
	else
		push_val(st->stack,C_INT,0);

	return 0;
}

/*==========================================
 * 装備品精錬成功率
 *------------------------------------------
 */
int buildin_getequippercentrefinery(struct script_state *st)
{
	int i,num;
	struct map_session_data *sd;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=script_rid2sd(st);
	i=pc_checkequip(sd,equip[num-1]);
	if(i >= 0)
		push_val(st->stack,C_INT,status_percentrefinery(sd,&sd->status.inventory[i]));
	else
		push_val(st->stack,C_INT,0);

	return 0;
}

/*==========================================
 * 精錬成功
 *------------------------------------------
 */
int buildin_successrefitem(struct script_state *st)
{
	int i,num,ep;
	struct map_session_data *sd;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=script_rid2sd(st);
	i=pc_checkequip(sd,equip[num-1]);
	if(i >= 0) {
		ep=sd->status.inventory[i].equip;

		sd->status.inventory[i].refine++;
		pc_unequipitem(sd,i,0);
		clif_refine(sd->fd,sd,0,i,sd->status.inventory[i].refine);
		clif_delitem(sd,i,1);
		clif_additem(sd,i,1,0);
		pc_equipitem(sd,i,ep);
		clif_misceffect(&sd->bl,3);

		//ブラックスミス 名声値
		if(sd->status.inventory[i].refine==10 && (*((unsigned long *)(&sd->status.inventory[i].card[2]))) == sd->status.char_id)
		{
			switch(itemdb_wlv(sd->status.inventory[i].nameid))
			{
				case 1:
					ranking_gain_point(sd,RK_BLACKSMITH,1);
					ranking_setglobalreg(sd,RK_BLACKSMITH);
					ranking_update(sd,RK_BLACKSMITH);
					break;
				case 2:
					ranking_gain_point(sd,RK_BLACKSMITH,25);
					ranking_setglobalreg(sd,RK_BLACKSMITH);
					ranking_update(sd,RK_BLACKSMITH);
					break;
				case 3:
					ranking_gain_point(sd,RK_BLACKSMITH,1000);
					ranking_setglobalreg(sd,RK_BLACKSMITH);
					ranking_update(sd,RK_BLACKSMITH);
					break;
				default:
					break;
			};
		}
	}

	return 0;
}

/*==========================================
 * 精錬失敗
 *------------------------------------------
 */
int buildin_failedrefitem(struct script_state *st)
{
	int i,num;
	struct map_session_data *sd;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=script_rid2sd(st);
	i=pc_checkequip(sd,equip[num-1]);
	if(i >= 0) {
		sd->status.inventory[i].refine = 0;
		pc_unequipitem(sd,i,0);
		// 精錬失敗エフェクトのパケット
		clif_refine(sd->fd,sd,1,i,sd->status.inventory[i].refine);
		pc_delitem(sd,i,1,0);
		// 他の人にも失敗を通知
		clif_misceffect(&sd->bl,2);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_statusup(struct script_state *st)
{
	unsigned short type;

	type = (unsigned short)conv_num(st, &(st->stack->stack_data[st->start+2]));
	pc_statusup(script_rid2sd(st),type);

	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int buildin_statusup2(struct script_state *st)
{
	int type,val;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	val=conv_num(st,& (st->stack->stack_data[st->start+3]));
	pc_statusup2(script_rid2sd(st),type,val);

	return 0;
}
/*==========================================
 * 装備品による能力値ボーナス
 *------------------------------------------
 */
int buildin_bonus(struct script_state *st)
{
	int type,val;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	val=conv_num(st,& (st->stack->stack_data[st->start+3]));

	pc_bonus(script_rid2sd(st),type,val);
	return 0;
}
/*==========================================
 * 装備品による能力値ボーナス
 *------------------------------------------
 */
int buildin_bonus2(struct script_state *st)
{
	int type,type2,val;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	type2=conv_num(st,& (st->stack->stack_data[st->start+3]));
	val=conv_num(st,& (st->stack->stack_data[st->start+4]));

	pc_bonus2(script_rid2sd(st),type,type2,val);
	return 0;
}
/*==========================================
 * 装備品による能力値ボーナス
 *------------------------------------------
 */
int buildin_bonus3(struct script_state *st)
{
	int type,type2,type3,val;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	type2=conv_num(st,& (st->stack->stack_data[st->start+3]));
	type3=conv_num(st,& (st->stack->stack_data[st->start+4]));
	val=conv_num(st,& (st->stack->stack_data[st->start+5]));

	pc_bonus3(script_rid2sd(st),type,type2,type3,val);
	return 0;
}
/*==========================================
 * 装備品による能力値ボーナス
 *------------------------------------------
 */
int buildin_bonus4(struct script_state *st)
{
	int type,type2,type3,type4;
	long val;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	type2=conv_num(st,& (st->stack->stack_data[st->start+3]));
	type3=conv_num(st,& (st->stack->stack_data[st->start+4]));
	type4=conv_num(st,& (st->stack->stack_data[st->start+5]));
	val=conv_num(st,& (st->stack->stack_data[st->start+6]));

	pc_bonus4(script_rid2sd(st),type,type2,type3,type4,val);
	return 0;
}
/*==========================================
 * スキル所得
 *------------------------------------------
 */
int buildin_skill(struct script_state *st)
{
	int id,level,flag=1;

	id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	level=conv_num(st,& (st->stack->stack_data[st->start+3]));
	if( st->end>st->start+4 )
		flag=conv_num(st,&(st->stack->stack_data[st->start+4]) );

	pc_skill(script_rid2sd(st),id,level,flag);
	return 0;
}
/*==========================================
 * ギルドスキル取得
 *------------------------------------------
 */
int buildin_guildskill(struct script_state *st)
{
	int id,level,flag=0;
	int i=0;

	id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	level=conv_num(st,& (st->stack->stack_data[st->start+3]));
	if( st->end>st->start+4 )
		flag=conv_num(st,&(st->stack->stack_data[st->start+4]) );
	for(i=0;i<level;i++)
		guild_skillup(script_rid2sd(st),id,flag);

	return 0;
}
/*==========================================
 * スキルレベル所得
 *------------------------------------------
 */
int buildin_getskilllv(struct script_state *st)
{
	int id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	push_val(st->stack,C_INT, pc_checkskill2( script_rid2sd(st) ,id) );
	return 0;
}
/*==========================================
 * getgdskilllv(Guild_ID, Skill_ID);
 *------------------------------------------
 */
int buildin_getgdskilllv(struct script_state *st)
{
	int guild_id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	int skill_id=conv_num(st,& (st->stack->stack_data[st->start+3]));
	struct guild *g=guild_search(guild_id);
	push_val(st->stack,C_INT, (g==NULL)?-1:guild_checkskill(g,skill_id) );
	return 0;
/*
	struct map_session_data *sd=NULL;
	struct guild *g=NULL;
	int skill_id;

	skill_id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=script_rid2sd(st);
	if(sd && sd->status.guild_id > 0) g=guild_search(sd->status.guild_id);
	if(sd && g) {
		push_val(st->stack,C_INT, guild_checkskill(g,skill_id+9999) );
	} else {
		push_val(st->stack,C_INT,-1);
	}
	return 0;
*/
}
/*==========================================
 *
 *------------------------------------------
 */
int buildin_basicskillcheck(struct script_state *st)
{
	push_val(st->stack,C_INT, battle_config.basic_skill_check);
	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int buildin_getgmlevel(struct script_state *st)
{
	push_val(st->stack,C_INT, pc_isGM(script_rid2sd(st)));
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_end(struct script_state *st)
{
	st->state = END;
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_checkoption(struct script_state *st)
{
	int type;
	struct map_session_data *sd = script_rid2sd(st);

	nullpo_retr(0, sd);

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));

	if(sd->status.option & type){
		push_val(st->stack,C_INT,1);
	} else {
		push_val(st->stack,C_INT,0);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_setoption(struct script_state *st)
{
	short type;

	type = (short)conv_num(st,& (st->stack->stack_data[st->start+2]));
	pc_setoption(script_rid2sd(st),type);

	return 0;
}

/*==========================================
 * カートを付ける
 *------------------------------------------
 */
int buildin_setcart(struct script_state *st)
{
	pc_setcart( script_rid2sd(st),1 );
	return 0;
}

/*==========================================
 * 鷹を付ける
 *------------------------------------------
 */
int buildin_setfalcon(struct script_state *st)
{
	pc_setfalcon( script_rid2sd(st) );
	return 0;
}

/*==========================================
 * ペコペコ乗り
 *------------------------------------------
 */
int buildin_setriding(struct script_state *st)
{
	pc_setriding( script_rid2sd(st) );
	return 0;
}

/*==========================================
 *	セーブポイントの保存
 *------------------------------------------
 */
int buildin_savepoint(struct script_state *st)
{
	int x,y;
	char *str;

	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	x=conv_num(st,& (st->stack->stack_data[st->start+3]));
	y=conv_num(st,& (st->stack->stack_data[st->start+4]));
	pc_setsavepoint(script_rid2sd(st),str,x,y);
	return 0;
}

/*==========================================
 * GetTimeTick(0: System Tick, 1: Time Second Tick)
 *------------------------------------------
 */
int buildin_gettimetick(struct script_state *st)	/* Asgard Version */
{
	int type;
	time_t timer;
	struct tm *t;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));

	switch(type){
	case 2:
		//type 2:(Get the number of seconds elapsed since 00:00 hours, Jan 1, 1970 UTC
		//        from the system clock.)
		push_val(st->stack,C_INT,(int)time(NULL));
		break;
	case 1:
		//type 1:(Second Ticks: 0-86399, 00:00:00-23:59:59)
		time(&timer);
		t=localtime(&timer);
		push_val(st->stack,C_INT,((t->tm_hour)*3600+(t->tm_min)*60+t->tm_sec));
		break;
	case 0:
	default:
		//type 0:(System Ticks)
		push_val(st->stack,C_INT,gettick());
		break;
	}
	return 0;
}

/*==========================================
 * GetTime(Type);
 * 1: Sec	 2: Min	 3: Hour
 * 4: WeekDay	 5: MonthDay	 6: Month
 * 7: Year
 *------------------------------------------
 */
int buildin_gettime(struct script_state *st)	/* Asgard Version */
{
	int type;
	time_t timer;
	struct tm *t;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));

	time(&timer);
	t=localtime(&timer);

	switch(type){
	case 1://Sec(0~59)
		push_val(st->stack,C_INT,t->tm_sec);
		break;
	case 2://Min(0~59)
		push_val(st->stack,C_INT,t->tm_min);
		break;
	case 3://Hour(0~23)
		push_val(st->stack,C_INT,t->tm_hour);
		break;
	case 4://WeekDay(0~6)
		push_val(st->stack,C_INT,t->tm_wday);
		break;
	case 5://MonthDay(01~31)
		push_val(st->stack,C_INT,t->tm_mday);
		break;
	case 6://Month(01~12)
		push_val(st->stack,C_INT,t->tm_mon+1);
		break;
	case 7://Year(20xx)
		push_val(st->stack,C_INT,t->tm_year+1900);
		break;
	default://(format error)
		push_val(st->stack,C_INT,-1);
		break;
	}
	return 0;
}

/*==========================================
 * GetTimeStr("TimeFMT", Length);
 *------------------------------------------
 */
int buildin_gettimestr(struct script_state *st)
{
	char *tmpstr;
	char *fmtstr;
	int maxlen;
	time_t now = time(NULL);

	fmtstr=conv_str(st,& (st->stack->stack_data[st->start+2]));
	maxlen=conv_num(st,& (st->stack->stack_data[st->start+3]));

	tmpstr=(char *)aCalloc(maxlen+1,sizeof(char));
	strftime(tmpstr,maxlen,fmtstr,localtime(&now));
	tmpstr[maxlen]='\0';

	push_str(st->stack,C_STR,tmpstr);
	return 0;
}

/*==========================================
 * カプラ倉庫を開く
 *------------------------------------------
 */
int buildin_openstorage(struct script_state *st)
{
	storage_storageopen(script_rid2sd(st));
	return 0;
}

int buildin_guildopenstorage(struct script_state *st)
{
	struct map_session_data *sd=script_rid2sd(st);
	int ret;
	ret = storage_guild_storageopen(sd);
	push_val(st->stack,C_INT,ret);
	return 0;
}

/*==========================================
 * アイテムによるスキル発動
 *------------------------------------------
 */
int buildin_itemskill(struct script_state *st)
{
	int id,lv;
	short flag=0;
	char *str;
	struct map_session_data *sd=script_rid2sd(st);

	nullpo_retr(0, sd);

	id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	lv=conv_num(st,& (st->stack->stack_data[st->start+3]));
	str=conv_str(st,& (st->stack->stack_data[st->start+4]));
	if( st->end > st->start+5 )
		flag=(short)conv_num(st,& (st->stack->stack_data[st->start+5]));

	// 詠唱中にスキルアイテムは使用できない
	if(sd->ud.skilltimer != -1)
		return 0;

	sd->skillitem=id;
	sd->skillitemlv=lv;
	sd->skillitem_flag = (flag)? 1: 0;
	clif_item_skill(sd,id,lv,str);
	return 0;
}
/*==========================================
 * アイテム作成
 *------------------------------------------
 */
int buildin_produce(struct script_state *st)
{
	int trigger;
	struct map_session_data *sd=script_rid2sd(st);

	nullpo_retr(0, sd);

	if(sd->state.produce_flag != 1) {
		trigger=conv_num(st,& (st->stack->stack_data[st->start+2]));
		clif_skill_produce_mix_list(sd,trigger);
	}
	return 0;
}
/*==========================================
 * NPCでペット作る
 *------------------------------------------
 */
int buildin_makepet(struct script_state *st)
{
	struct map_session_data *sd = script_rid2sd(st);
	struct script_data *data;
	int id,pet_id;

	data=&(st->stack->stack_data[st->start+2]);
	get_val(st,data);

	id=conv_num(st,data);

	pet_id = search_petDB_index(id, PET_CLASS);

	if (pet_id < 0)
		pet_id = search_petDB_index(id, PET_EGG);
	if (pet_id >= 0 && sd) {
		sd->catch_target_class = pet_db[pet_id].class;
		intif_create_pet(
			sd->status.account_id, sd->status.char_id,
			pet_db[pet_id].class, mob_db[pet_db[pet_id].class].lv,
			pet_db[pet_id].EggID, 0, pet_db[pet_id].intimate,
			100, 0, 1, pet_db[pet_id].jname);
	}

	return 0;
}

/*==========================================
 * モンスター発生
 *------------------------------------------
 */
int buildin_monster(struct script_state *st)
{
	int class,amount,x,y,guild_id=0,id;
	struct mob_data *md;
	char *str,*map,*mobname,*event="";
	struct guild* g;

	map	=conv_str(st,& (st->stack->stack_data[st->start+2]));
	x	=conv_num(st,& (st->stack->stack_data[st->start+3]));
	y	=conv_num(st,& (st->stack->stack_data[st->start+4]));
	str	=conv_str(st,& (st->stack->stack_data[st->start+5]));
	mobname	=conv_str(st,& (st->stack->stack_data[st->start+6]));
	if((class = atoi(mobname))==0)
		class = mobdb_searchname(mobname);

	amount=conv_num(st,& (st->stack->stack_data[st->start+7]));
	if( st->end > st->start+9 ){	// Guild_ID入り
		guild_id=conv_num(st,& (st->stack->stack_data[st->start+8]));
		event=conv_str(st,& (st->stack->stack_data[st->start+9]));
	}
	else if(st->end > st->start+8)
		event=conv_str(st,& (st->stack->stack_data[st->start+8]));

	if ((class>=0 && class<=1000) || class >MOB_ID_MAX)
		return 0;

	id = mob_once_spawn(map_id2sd(st->rid),map,x,y,str,class,amount,event);
	if(!id) return 0;
	md = (struct mob_data *)map_id2bl(id);
	if(!md) return 0;
	md->guardup_lv = 0;
	if((g = guild_search(guild_id))!=NULL){
		md->guild_id = guild_id;
		//ガーディアンならギルドスキル適用
		md->guardup_lv = guild_checkskill(g,GD_GUARDUP);
	}

	//ランダム召還じゃないならドロップあり
	if(class!=-1)
		return 0;

	if(md->mode&0x20)//手抜きボス属性
	{
		md->state.nodrop= battle_config.branch_boss_no_drop;
		md->state.noexp = battle_config.branch_boss_no_exp;
		md->state.nomvp = battle_config.branch_boss_no_mvp;
	}else{
		md->state.nodrop= battle_config.branch_mob_no_drop;
		md->state.noexp = battle_config.branch_mob_no_exp;
		md->state.nomvp = battle_config.branch_mob_no_mvp;
	}

	return 0;
}
/*==========================================
 * モンスター発生
 *------------------------------------------
 */
int buildin_areamonster(struct script_state *st)
{
	int class,amount,x0,y0,x1,y1;
	char *str,*map,*mobname,*event="";

	map	=conv_str(st,& (st->stack->stack_data[st->start+2]));
	x0	=conv_num(st,& (st->stack->stack_data[st->start+3]));
	y0	=conv_num(st,& (st->stack->stack_data[st->start+4]));
	x1	=conv_num(st,& (st->stack->stack_data[st->start+5]));
	y1	=conv_num(st,& (st->stack->stack_data[st->start+6]));
	str	=conv_str(st,& (st->stack->stack_data[st->start+7]));
	mobname	=conv_str(st,& (st->stack->stack_data[st->start+8]));
	if((class = atoi(mobname))==0)
		class = mobdb_searchname(mobname);

	amount=conv_num(st,& (st->stack->stack_data[st->start+9]));
	if( st->end>st->start+10 )
		event=conv_str(st,& (st->stack->stack_data[st->start+10]));

	mob_once_spawn_area(map_id2sd(st->rid),map,x0,y0,x1,y1,str,class,amount,event);
	return 0;
}
/*==========================================
 * モンスター削除
 *------------------------------------------
 */
int buildin_killmonster_sub(struct block_list *bl,va_list ap)
{
	char *event=va_arg(ap,char *);
	int allflag=va_arg(ap,int);

	if(!allflag){
		if(strcmp(event,((struct mob_data *)bl)->npc_event)==0)
			unit_remove_map(bl,1);
		return 0;
	}else if(allflag){
		if(((struct mob_data *)bl)->spawndelay1==-1 && ((struct mob_data *)bl)->spawndelay2==-1)
			unit_remove_map(bl,1);
		return 0;
	}
	return 0;
}
int buildin_killmonster(struct script_state *st)
{
	char *mapname,*event;
	int m,allflag=0;
	mapname=conv_str(st,& (st->stack->stack_data[st->start+2]));
	event=conv_str(st,& (st->stack->stack_data[st->start+3]));
	if(strcmp(mapname,"this")==0){
		struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);
		if(nd)
			m=nd->bl.m;
		else {
			struct map_session_data *sd=script_rid2sd(st);
			if(sd)
				m=sd->bl.m;
			else
				return 0;
		}
	}
	else if( (m=map_mapname2mapid(mapname))<0 )
		return 0;

	if(strcmp(event,"All")==0)
		allflag = 1;
	map_foreachinarea(buildin_killmonster_sub,
		m,0,0,map[m].xs,map[m].ys,BL_MOB, event ,allflag);
	return 0;
}

int buildin_killmonsterall_sub(struct block_list *bl,va_list ap)
{
	unit_remove_map(bl,1);
	return 0;
}
int buildin_killmonsterall(struct script_state *st)
{
	char *mapname;
	int m;
	mapname=conv_str(st,& (st->stack->stack_data[st->start+2]));

	if(strcmp(mapname,"this")==0){
		struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);
		if(nd)
			m=nd->bl.m;
		else {
			struct map_session_data *sd=script_rid2sd(st);
			if(sd)
				m=sd->bl.m;
			else
				return 0;
		}
	}
	else if( (m=map_mapname2mapid(mapname))< 0)
		return 0;
	map_foreachinarea(buildin_killmonsterall_sub,
		m,0,0,map[m].xs,map[m].ys,BL_MOB);
	return 0;
}
int buildin_areakillmonster(struct script_state *st)
{
	char *mapname;
	int m,x0,y0,x1,y1;
	mapname=conv_str(st,& (st->stack->stack_data[st->start+2]));
	x0=conv_num(st,& (st->stack->stack_data[st->start+3]));
	y0=conv_num(st,& (st->stack->stack_data[st->start+4]));
	x1=conv_num(st,& (st->stack->stack_data[st->start+5]));
	y1=conv_num(st,& (st->stack->stack_data[st->start+6]));
	if(strcmp(mapname,"this")==0){
		struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);
		if(nd)
			m=nd->bl.m;
		else {
			struct map_session_data *sd=script_rid2sd(st);
			if(sd)
				m=sd->bl.m;
			else
				return 0;
		}
	}
	else if( (m=map_mapname2mapid(mapname))< 0)
		return 0;
	map_foreachinarea(buildin_killmonsterall_sub,
		m,x0,y0,x1,y1,BL_MOB);
	return 0;
}
/*==========================================
 * イベント実行
 *------------------------------------------
 */
int buildin_doevent(struct script_state *st)
{
	char *event;
	event=conv_str(st,& (st->stack->stack_data[st->start+2]));
	npc_event(map_id2sd(st->rid),event);	// sdがNULLでもいい
	return 0;
}
/*==========================================
 * NPC主体イベント実行
 *------------------------------------------
 */
int buildin_donpcevent(struct script_state *st)
{
	char *event;
	event=conv_str(st,& (st->stack->stack_data[st->start+2]));
	npc_event_do(event);
	return 0;
}
/*==========================================
 * イベントタイマー追加
 *------------------------------------------
 */
int buildin_addtimer(struct script_state *st)
{
	char *event;
	int tick;
	tick=conv_num(st,& (st->stack->stack_data[st->start+2]));
	event=conv_str(st,& (st->stack->stack_data[st->start+3]));
	pc_addeventtimer(script_rid2sd(st),tick,event);
	return 0;
}
/*==========================================
 * イベントタイマー削除
 *------------------------------------------
 */
int buildin_deltimer(struct script_state *st)
{
	char *event;
	event=conv_str(st,& (st->stack->stack_data[st->start+2]));
	pc_deleventtimer(script_rid2sd(st),event);
	return 0;
}
/*==========================================
 * イベントタイマーのカウント値追加
 *------------------------------------------
 */
int buildin_addtimercount(struct script_state *st)
{
	char *event;
	int tick;
	event=conv_str(st,& (st->stack->stack_data[st->start+2]));
	tick=conv_num(st,& (st->stack->stack_data[st->start+3]));
	pc_addeventtimercount(script_rid2sd(st),event,tick);
	return 0;
}

/*==========================================
 * NPCタイマー初期化
 *------------------------------------------
 */
int buildin_initnpctimer(struct script_state *st)
{
	struct npc_data *nd;
	if( st->end > st->start+2 )
		nd=npc_name2id(conv_str(st,& (st->stack->stack_data[st->start+2])));
	else
		nd=(struct npc_data *)map_id2bl(st->oid);

	npc_settimerevent_tick(nd,0);
	npc_timerevent_start(nd);
	return 0;
}
/*==========================================
 * NPCタイマー開始
 *------------------------------------------
 */
int buildin_startnpctimer(struct script_state *st)
{
	struct npc_data *nd;
	if( st->end > st->start+2 )
		nd=npc_name2id(conv_str(st,& (st->stack->stack_data[st->start+2])));
	else
		nd=(struct npc_data *)map_id2bl(st->oid);

	npc_timerevent_start(nd);
	return 0;
}
/*==========================================
 * NPCタイマー停止
 *------------------------------------------
 */
int buildin_stopnpctimer(struct script_state *st)
{
	struct npc_data *nd;
	if( st->end > st->start+2 )
		nd=npc_name2id(conv_str(st,& (st->stack->stack_data[st->start+2])));
	else
		nd=(struct npc_data *)map_id2bl(st->oid);

	npc_timerevent_stop(nd);
	return 0;
}
/*==========================================
 * NPCタイマー情報所得
 *------------------------------------------
 */
int buildin_getnpctimer(struct script_state *st)
{
	struct npc_data *nd;
	int type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	int val=0;
	if( st->end > st->start+3 )
		nd=npc_name2id(conv_str(st,& (st->stack->stack_data[st->start+3])));
	else
		nd=(struct npc_data *)map_id2bl(st->oid);

	nullpo_retr(0, nd);

	switch(type){
		case 0: val = npc_gettimerevent_tick(nd); break;
		case 1: val = (nd->u.scr.nexttimer >= 0); break;
		case 2: val = nd->u.scr.timeramount; break;
	}
	push_val(st->stack,C_INT,val);
	return 0;
}
/*==========================================
 * NPCタイマー値設定
 *------------------------------------------
 */
int buildin_setnpctimer(struct script_state *st)
{
	int tick;
	struct npc_data *nd;
	tick=conv_num(st,& (st->stack->stack_data[st->start+2]));
	if( st->end > st->start+3 )
		nd=npc_name2id(conv_str(st,& (st->stack->stack_data[st->start+3])));
	else
		nd=(struct npc_data *)map_id2bl(st->oid);

	npc_settimerevent_tick(nd,tick);
	return 0;
}

/*==========================================
 * 天の声アナウンス
 *------------------------------------------
 */
int buildin_announce(struct script_state *st)
{
	char *str,*color=NULL;
	int flag;
	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	flag=conv_num(st,& (st->stack->stack_data[st->start+3]));
	if (st->end>st->start+4)
		color=conv_str(st,& (st->stack->stack_data[st->start+4]));

	if(flag&0x07) {
		struct block_list *bl=(flag&0x08)? map_id2bl(st->oid):
						(struct block_list *)script_rid2sd(st);
		if(color)
			clif_announce(bl,str,strlen(str)+1,strtol(color,NULL,0),flag);
		else
			clif_GMmessage(bl,str,strlen(str)+1,flag);
	}
	else {
		if(color)
			intif_announce(str,strlen(str)+1,strtol(color,NULL,0),flag);
		else
			intif_GMmessage(str,strlen(str)+1,flag);
	}
	return 0;
}
/*==========================================
 * 天の声アナウンス（特定マップ）
 *------------------------------------------
 */
int buildin_mapannounce_sub(struct block_list *bl,va_list ap)
{
	char *str,*color;
	int len,flag;
	str=va_arg(ap,char *);
	len=va_arg(ap,int);
	flag=va_arg(ap,int);
	color=va_arg(ap,char *);

	if(color)
		clif_announce(bl,str,len,strtol(color,NULL,0),flag|3);
	else
		clif_GMmessage(bl,str,len,flag|3);
	return 0;
}
int buildin_mapannounce(struct script_state *st)
{
	char *mapname,*str,*color=NULL;
	int flag,m;

	mapname=conv_str(st,& (st->stack->stack_data[st->start+2]));
	str=conv_str(st,& (st->stack->stack_data[st->start+3]));
	flag=conv_num(st,& (st->stack->stack_data[st->start+4]));
	if (st->end>st->start+5)
		color=conv_str(st,& (st->stack->stack_data[st->start+5]));

	if( (m=map_mapname2mapid(mapname))<0 )
		return 0;
	map_foreachinarea(buildin_mapannounce_sub,
		m,0,0,map[m].xs,map[m].ys,BL_PC, str,strlen(str)+1,flag&0x10,color);
	return 0;
}
/*==========================================
 * 天の声アナウンス（特定エリア）
 *------------------------------------------
 */
int buildin_areaannounce(struct script_state *st)
{
	char *map,*str,*color=NULL;
	int flag,m;
	int x0,y0,x1,y1;

	map=conv_str(st,& (st->stack->stack_data[st->start+2]));
	x0=conv_num(st,& (st->stack->stack_data[st->start+3]));
	y0=conv_num(st,& (st->stack->stack_data[st->start+4]));
	x1=conv_num(st,& (st->stack->stack_data[st->start+5]));
	y1=conv_num(st,& (st->stack->stack_data[st->start+6]));
	str=conv_str(st,& (st->stack->stack_data[st->start+7]));
	flag=conv_num(st,& (st->stack->stack_data[st->start+8]));
	if (st->end>st->start+9)
		color=conv_str(st,& (st->stack->stack_data[st->start+9]));

	if( (m=map_mapname2mapid(map))<0 )
		return 0;

	map_foreachinarea(buildin_mapannounce_sub,
		m,x0,y0,x1,y1,BL_PC, str,strlen(str)+1,flag&0x10,color);
	return 0;
	return 0;
}
/*==========================================
 * ユーザー数取得
 *------------------------------------------
 */
int buildin_getusers(struct script_state *st)
{
	int flag=conv_num(st,& (st->stack->stack_data[st->start+2]));
	struct block_list *bl=map_id2bl( (flag&0x08)? st->oid: st->rid );
	int val = -1;

	switch(flag&0x07){
		case 0:
			if(bl)
				val=map[bl->m].users;
			break;
		case 1:
			val=map_getusers();
			break;
	}
	push_val(st->stack,C_INT,val);
	return 0;
}
/*==========================================
 * 繋いでるユーザーの全員の名前取得(@who)
 *------------------------------------------
 */
int buildin_getusersname(struct script_state *st)
{
	struct map_session_data *pl_sd = NULL;
	int i=0,disp_num=1;

	for (i=0;i<fd_max;i++)
		if(session[i] && (pl_sd=session[i]->session_data) && pl_sd->state.auth){
			if( !(battle_config.hide_GM_session && pc_isGM(pl_sd)) ){
				if((disp_num++)%10==0)
					clif_scriptnext(script_rid2sd(st),st->oid);
				clif_scriptmes(script_rid2sd(st),st->oid,pl_sd->status.name);
			}
		}
	return 0;
}
/*==========================================
 * マップ指定ユーザー数取得
 *------------------------------------------
 */
int buildin_getmapusers(struct script_state *st)
{
	char *str;
	int m;
	str=conv_str(st,& (st->stack->stack_data[st->start+2]));

	if(strcmp(str,"this")==0){
		struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);
		if(nd)
			m=nd->bl.m;
		else {
			struct map_session_data *sd=script_rid2sd(st);
			if(sd)
				m=sd->bl.m;
			else {
				push_val(st->stack,C_INT,-1);
				return 0;
			}
		}
	}
	else if( (m=map_mapname2mapid(str))< 0){
		push_val(st->stack,C_INT,-1);
		return 0;
	}
	push_val(st->stack,C_INT,map[m].users);
	return 0;
}
/*==========================================
 * エリア指定ユーザー数所得
 *------------------------------------------
 */
int buildin_getareausers_sub(struct block_list *bl,va_list ap)
{
	int *users=va_arg(ap,int *);
	(*users)++;
	return 0;
}
int buildin_getareausers(struct script_state *st)
{
	char *str;
	int m,x0,y0,x1,y1,users=0;
	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	x0=conv_num(st,& (st->stack->stack_data[st->start+3]));
	y0=conv_num(st,& (st->stack->stack_data[st->start+4]));
	x1=conv_num(st,& (st->stack->stack_data[st->start+5]));
	y1=conv_num(st,& (st->stack->stack_data[st->start+6]));
	if(strcmp(str,"this")==0){
		struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);
		if(nd)
			m=nd->bl.m;
		else {
			struct map_session_data *sd=script_rid2sd(st);
			if(sd)
				m=sd->bl.m;
			else {
				push_val(st->stack,C_INT,-1);
				return 0;
			}
		}
	}
	else if( (m=map_mapname2mapid(str))< 0){
		push_val(st->stack,C_INT,-1);
		return 0;
	}
	map_foreachinarea(buildin_getareausers_sub,
		m,x0,y0,x1,y1,BL_PC,&users);
	push_val(st->stack,C_INT,users);
	return 0;
}

/*==========================================
 * エリア指定ドロップアイテム数所得
 *------------------------------------------
 */
int buildin_getareadropitem_sub(struct block_list *bl,va_list ap)
{
	int item=va_arg(ap,int);
	int *amount=va_arg(ap,int *);
	struct flooritem_data *drop=(struct flooritem_data *)bl;

	if(drop->item_data.nameid==item)
		(*amount)+=drop->item_data.amount;

	return 0;
}
int buildin_getareadropitem(struct script_state *st)
{
	char *str;
	int m,x0,y0,x1,y1,item,amount=0;
	struct script_data *data;

	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	x0=conv_num(st,& (st->stack->stack_data[st->start+3]));
	y0=conv_num(st,& (st->stack->stack_data[st->start+4]));
	x1=conv_num(st,& (st->stack->stack_data[st->start+5]));
	y1=conv_num(st,& (st->stack->stack_data[st->start+6]));

	data=&(st->stack->stack_data[st->start+7]);
	get_val(st,data);
	if( isstr(data) ){
		const char *name=conv_str(st,data);
		struct item_data *item_data = itemdb_searchname(name);
		item=512;
		if( item_data )
			item=item_data->nameid;
	}else
		item=conv_num(st,data);

	if( (m=map_mapname2mapid(str))< 0){
		push_val(st->stack,C_INT,-1);
		return 0;
	}
	map_foreachinarea(buildin_getareadropitem_sub,
		m,x0,y0,x1,y1,BL_ITEM,item,&amount);
	push_val(st->stack,C_INT,amount);
	return 0;
}
/*==========================================
 * NPCの有効化
 *------------------------------------------
 */
int buildin_enablenpc(struct script_state *st)
{
	struct npc_data *nd;
	if( st->end > st->start+2 ) {
		char *name = conv_str(st,& (st->stack->stack_data[st->start+2]));
		nd=npc_name2id(name);
		if( nd == NULL ) {
			//printf("buildin_enablenpc: can't find npc name: %s\n", name);
			return 0;
		}
	} else {
		nd=(struct npc_data *)map_id2bl(st->oid);
		if( nd == NULL ) {
			printf("buildin_enablenpc: fatal error: npc not attached\n");
			return 0;
		}
	}
	npc_enable(nd->exname,1);
	return 0;
}
/*==========================================
 * NPCの無効化
 *------------------------------------------
 */
int buildin_disablenpc(struct script_state *st)
{
	struct npc_data *nd;
	if( st->end > st->start+2 ) {
		char *name = conv_str(st,& (st->stack->stack_data[st->start+2]));
		nd=npc_name2id(name);
		if( nd == NULL ) {
			//printf("buildin_disablenpc: can't find npc name: %s\n", name);
			return 0;
		}
	} else {
		nd=(struct npc_data *)map_id2bl(st->oid);
		if( nd == NULL ) {
			printf("buildin_disablenpc: fatal error: npc not attached\n");
			return 0;
		}
	}
	npc_enable(nd->exname,0);
	return 0;
}
/*==========================================
 * 隠れているNPCの表示
 *------------------------------------------
 */
int buildin_hideoffnpc(struct script_state *st)
{
	struct npc_data *nd;
	if( st->end > st->start+2 ) {
		char *name = conv_str(st,& (st->stack->stack_data[st->start+2]));
		nd=npc_name2id(name);
		if( nd == NULL ) {
			//printf("buildin_hideoffnpc: can't find npc name: %s\n", name);
			return 0;
		}
	} else {
		nd=(struct npc_data *)map_id2bl(st->oid);
		if( nd == NULL ) {
			printf("buildin_hideoffnpc: fatal error: npc not attached\n");
			return 0;
		}
	}
	npc_enable(nd->exname,2);
	return 0;
}
/*==========================================
 * NPCをハイディング
 *------------------------------------------
 */
int buildin_hideonnpc(struct script_state *st)
{
	struct npc_data *nd;
	if( st->end > st->start+2 ) {
		char *name = conv_str(st,& (st->stack->stack_data[st->start+2]));
		nd=npc_name2id(name);
		if( nd == NULL ) {
			//printf("buildin_hideonnpc: can't find npc name: %s\n", name);
			return 0;
		}
	} else {
		nd=(struct npc_data *)map_id2bl(st->oid);
		if( nd == NULL ) {
			printf("buildin_hideonnpc: fatal error: npc not attached\n");
			return 0;
		}
	}
	npc_enable(nd->exname,4);
	return 0;
}

/*==========================================
 * 状態異常にかかる
 *------------------------------------------
 */
int buildin_sc_start(struct script_state *st)
{
	struct block_list *bl;
	int type,tick,val1;
	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	tick=conv_num(st,& (st->stack->stack_data[st->start+3]));
	val1=conv_num(st,& (st->stack->stack_data[st->start+4]));
	if( st->end>st->start+5 ) //指定したキャラを状態異常にする
		bl = map_id2bl(conv_num(st,& (st->stack->stack_data[st->start+5])));
	else
		bl = map_id2bl(st->rid);

	if(bl && bl->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)bl;
		if(sd && sd->state.potionpitcher_flag)
			bl = map_id2bl(sd->ud.skilltarget);
	}
	if(bl && !unit_isdead(bl))
		status_change_start(bl,type,val1,0,0,0,tick,0);
	return 0;
}

/*==========================================
 * 状態異常にかかる(確率指定)
 *------------------------------------------
 */
int buildin_sc_start2(struct script_state *st)
{
	struct block_list *bl;
	int type,tick,val1,per;
	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	tick=conv_num(st,& (st->stack->stack_data[st->start+3]));
	val1=conv_num(st,& (st->stack->stack_data[st->start+4]));
	per=conv_num(st,& (st->stack->stack_data[st->start+5]));
	if( st->end>st->start+6 ) //指定したキャラを状態異常にする
		bl = map_id2bl(conv_num(st,& (st->stack->stack_data[st->start+6])));
	else
		bl = map_id2bl(st->rid);

	if(bl && bl->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)bl;
		if(sd && sd->state.potionpitcher_flag)
			bl = map_id2bl(sd->ud.skilltarget);
	}
	if(atn_rand()%10000 < per && bl && !unit_isdead(bl))
		status_change_start(bl,type,val1,0,0,0,tick,0);
	return 0;
}

/*==========================================
 * 状態異常にかかる
 *------------------------------------------
 */
int buildin_sc_starte(struct script_state *st)
{
	struct block_list *bl;
	int type,tick,val1;
	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	tick=conv_num(st,& (st->stack->stack_data[st->start+3]));
	val1=conv_num(st,& (st->stack->stack_data[st->start+4]));
	if( st->end>st->start+5 ) //指定したキャラを状態異常にする
		bl = map_id2bl(conv_num(st,& (st->stack->stack_data[st->start+5])));
	else
		bl = map_id2bl(st->rid);

	if(bl && bl->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)bl;
		if(sd && sd->state.potionpitcher_flag)
			bl = map_id2bl(sd->ud.skilltarget);
	}
	if(bl && !unit_isdead(bl))
		status_change_start(bl,type,val1,0,0,0,tick,4);
	return 0;
}

/*==========================================
 * 状態異常にかかる
 *------------------------------------------
 */
int buildin_sc_start3(struct script_state *st)
{
	struct block_list *bl;
	int type,tick,val1,val2,val3,val4,flag;
	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	val1=conv_num(st,& (st->stack->stack_data[st->start+3]));
	val2=conv_num(st,& (st->stack->stack_data[st->start+4]));
	val3=conv_num(st,& (st->stack->stack_data[st->start+5]));
	val4=conv_num(st,& (st->stack->stack_data[st->start+6]));
	tick=conv_num(st,& (st->stack->stack_data[st->start+7]));
	flag=conv_num(st,& (st->stack->stack_data[st->start+8]));
	if( st->end>st->start+9 ) //指定したキャラを状態異常にする
		bl = map_id2bl(conv_num(st,& (st->stack->stack_data[st->start+9])));
	else
		bl = map_id2bl(st->rid);

	if(bl && bl->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)bl;
		if(sd && sd->state.potionpitcher_flag)
			bl = map_id2bl(sd->ud.skilltarget);
	}
	if(bl && !unit_isdead(bl))
		status_change_start(bl,type,val1,val2,val3,val4,tick,flag);
	return 0;
}

/*==========================================
 * 状態異常が直る
 *------------------------------------------
 */
int buildin_sc_end(struct script_state *st)
{
	struct block_list *bl;
	int type;
	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	if( st->end>st->start+3 ) //指定したキャラの状態異常を解除する
		bl = map_id2bl(conv_num(st,& (st->stack->stack_data[st->start+3])));
	else
		bl = map_id2bl(st->rid);

	if(bl && bl->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)bl;
		if(sd && sd->state.potionpitcher_flag)
			bl = map_id2bl(sd->ud.skilltarget);
	}
	if(bl)
		status_change_end(bl,type,-1);
//	if(battle_config.etc_log)
//		printf("sc_end : %d %d\n",st->rid,type);
	return 0;
}

/*==========================================
 * 状態異常中かどうか返す
 *------------------------------------------
 */
int buildin_sc_ison(struct script_state *st)
{
	struct block_list *bl = map_id2bl(st->rid);
	struct status_change *sc_data;
	int type;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));

	if(bl && bl->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)bl;
		if(sd && sd->state.potionpitcher_flag)
			bl = map_id2bl(sd->ud.skilltarget);
	}
	if(bl) {
		sc_data = status_get_sc_data(bl);
		if(sc_data && sc_data[type].timer != -1)
			push_val(st->stack,C_INT,1);
	} else {
		push_val(st->stack,C_INT,0);
	}
	return 0;
}

/*==========================================
 * 状態異常耐性を計算した確率を返す
 *------------------------------------------
 */
int buildin_getscrate(struct script_state *st)
{
	struct block_list *bl;
	int sc_def=100;
	int type,rate=0,luk;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	rate=conv_num(st,& (st->stack->stack_data[st->start+3]));
	if( st->end>st->start+4 ) //指定したキャラの耐性を計算する
		bl = map_id2bl(conv_num(st,& (st->stack->stack_data[st->start+6])));
	else
		bl = map_id2bl(st->rid);

	if(bl) {
		luk = status_get_luk(bl);

		switch (type) {
			case SC_STONE:
			case SC_FREEZE:
				sc_def = 100 - (3 + status_get_mdef(bl) + luk/3);
				break;
			case SC_STAN:
			case SC_POISON:
			case SC_SILENCE:
			case SC_BLEED:
				sc_def = 100 - (3 + status_get_vit(bl) + luk/3);
				break;
			case SC_SLEEP:
			case SC_CONFUSION:
			case SC_BLIND:
				sc_def = 100 - (3 + status_get_int(bl) + luk/3);
				break;
			case SC_CURSE:
				sc_def = 100 - (3 + luk);
				break;
		}
	}
	rate=rate*sc_def/100;
	push_val(st->stack,C_INT,rate);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_debugmes(struct script_state *st)
{
	conv_str(st,& (st->stack->stack_data[st->start+2]));
	printf("script debug : %d %d : %s\n",st->rid,st->oid,st->stack->stack_data[st->start+2].u.str);
	return 0;
}

/*==========================================
 *捕獲アイテム使用
 *------------------------------------------
 */
int buildin_catchpet(struct script_state *st)
{
	int pet_id;

	pet_id= conv_num(st,& (st->stack->stack_data[st->start+2]));

	pet_catch_process1(script_rid2sd(st),pet_id);
	return 0;
}

/*==========================================
 *携帯卵孵化機使用
 *------------------------------------------
 */
int buildin_birthpet(struct script_state *st)
{
	clif_sendegg( script_rid2sd(st) );
	return 0;
}

/*==========================================
 * ステータスリセット
 *------------------------------------------
 */
int buildin_resetstatus(struct script_state *st)
{
	pc_resetstate( script_rid2sd(st) );
	return 0;
}

/*==========================================
 * スキルリセット
 *------------------------------------------
 */
int buildin_resetskill(struct script_state *st)
{
	pc_resetskill( script_rid2sd(st) );
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int buildin_changebase(struct script_state *st)
{
	struct map_session_data *sd;
	int vclass;

	if( st->end>st->start+3 )
		sd=map_id2sd(conv_num(st,& (st->stack->stack_data[st->start+3])));
	else
		sd=script_rid2sd(st);

	if(sd == NULL)
		return 0;

	vclass = conv_num(st,& (st->stack->stack_data[st->start+2]));
	if(vclass == 22 && !battle_config.wedding_modifydisplay)
		return 0;

	if(vclass==22)
		pc_unequipitem(sd,sd->equip_index[9],1);	// 装備外し

	if(vclass==26)
		pc_unequipitem(sd,sd->equip_index[9],1);	// 装備外し

	sd->view_class = vclass;

	return 0;
}

/*==========================================
 * 性別変換
 *------------------------------------------
 */
int buildin_changesex(struct script_state *st)
{
	struct map_session_data *sd = script_rid2sd(st);
	struct pc_base_job s_class;

	nullpo_retr(0, sd);

	//転生や養子の場合の元の職業を算出する
	s_class = pc_calc_base_job(sd->status.class);

	if(sd->sex==0){
		sd->sex=1;
		if(s_class.job == 20)
			sd->status.class -= 1;
		chrif_changesex(sd->status.account_id,1);
	}else if(sd->sex==1){
		sd->sex=0;
		if(s_class.job == 19)
			sd->status.class += 1;
		chrif_changesex(sd->status.account_id,0);

	}
	chrif_save(sd);
	return 0;
}

/*==========================================
 * npcチャット作成
 *------------------------------------------
 */
int buildin_waitingroom(struct script_state *st)
{
	char *name,*ev="";
	int limit, trigger = 0,pub=1;
	int zeny=0,lowlv=0,highlv=MAX_LEVEL;
	int job = ~(1<<MAX_PC_CLASS), upper = 0;

	name=conv_str(st,& (st->stack->stack_data[st->start+2]));
	limit= conv_num(st,& (st->stack->stack_data[st->start+3]));
	if(limit==0)
		pub=3;

	if( (st->end > st->start+5) ){
		struct script_data* data=&(st->stack->stack_data[st->start+5]);
		get_val(st,data);
		if(data->type==C_INT){
			// 新Athena仕様(旧Athena仕様と互換性あり)
			ev=conv_str(st,& (st->stack->stack_data[st->start+4]));
			trigger=conv_num(st,& (st->stack->stack_data[st->start+5]));
		}else{
			// eathena仕様
			trigger=conv_num(st,& (st->stack->stack_data[st->start+4]));
			ev=conv_str(st,& (st->stack->stack_data[st->start+5]));
		}
	}else{
		// 旧Athena仕様
		if( st->end > st->start+4 )
			ev=conv_str(st,& (st->stack->stack_data[st->start+4]));
	}

	if(st->end>st->start+8) {
		zeny 	= conv_num(st,& (st->stack->stack_data[st->start+6]));
		lowlv 	= conv_num(st,& (st->stack->stack_data[st->start+7]));
		highlv 	= conv_num(st,& (st->stack->stack_data[st->start+8]));
	}
	if(st->end>st->start+10) {
		job = conv_num(st,& (st->stack->stack_data[st->start+9]));
		upper = conv_num(st,& (st->stack->stack_data[st->start+10]));
	}
	chat_createnpcchat( (struct npc_data *)map_id2bl(st->oid),
		limit,pub,trigger,name,strlen(name)+1,ev,zeny,lowlv,highlv,job,upper);
	return 0;
}

/*==========================================
 * NPCがオープンチャットで発言
 *------------------------------------------
 */
int buildin_globalmes(struct script_state *st)
{
	struct npc_data *nd = (struct npc_data *)map_id2bl(st->oid);
	char *name=NULL,*mes;

	mes=conv_str(st,& (st->stack->stack_data[st->start+2]));	// メッセージの取得
	if(mes==NULL) return 0;

	if(st->end>st->start+3){	// NPC名の取得(123#456)
		name=conv_str(st,& (st->stack->stack_data[st->start+3]));
	} else if(nd){
		name=nd->exname;
	}

	npc_globalmessage(name,mes);	// グローバルメッセージ送信

	return 0;
}

/*==========================================
 * npcチャット削除
 *------------------------------------------
 */
int buildin_delwaitingroom(struct script_state *st)
{
	struct npc_data *nd;
	if( st->end > st->start+2 )
		nd=npc_name2id(conv_str(st,& (st->stack->stack_data[st->start+2])));
	else
		nd=(struct npc_data *)map_id2bl(st->oid);
	chat_deletenpcchat(nd);
	return 0;
}

/*==========================================
 * チャットから指定プレイヤーを蹴り出す
 *------------------------------------------
 */
int buildin_kickwaitingroom(struct script_state *st)
{
	struct chat_data *cd;
	struct map_session_data *sd;

	sd=map_id2sd(conv_num(st,& (st->stack->stack_data[st->start+2])));
	if(sd==NULL || (cd=(struct chat_data *)map_id2bl(sd->chatID))==NULL)
		return 0;
	chat_leavechat(sd,1);
	return 0;
}

/*==========================================
 * npcチャットから全員蹴り出す
 *------------------------------------------
 */
int buildin_kickwaitingroomall(struct script_state *st)
{
	struct npc_data *nd;
	struct chat_data *cd;

	if( st->end > st->start+2 )
		nd=npc_name2id(conv_str(st,& (st->stack->stack_data[st->start+2])));
	else
		nd=(struct npc_data *)map_id2bl(st->oid);

	if(nd==NULL || (cd=(struct chat_data *)map_id2bl(nd->chat_id))==NULL )
		return 0;
	chat_npckickall(cd);
	return 0;
}

/*==========================================
 * npcチャットイベント有効化
 *------------------------------------------
 */
int buildin_enablewaitingroomevent(struct script_state *st)
{
	struct npc_data *nd;
	struct chat_data *cd;

	if( st->end > st->start+2 )
		nd=npc_name2id(conv_str(st,& (st->stack->stack_data[st->start+2])));
	else
		nd=(struct npc_data *)map_id2bl(st->oid);

	if(nd==NULL || (cd=(struct chat_data *)map_id2bl(nd->chat_id))==NULL )
		return 0;
	chat_enableevent(cd);
	return 0;
}

/*==========================================
 * npcチャットイベント無効化
 *------------------------------------------
 */
int buildin_disablewaitingroomevent(struct script_state *st)
{
	struct npc_data *nd;
	struct chat_data *cd;

	if( st->end > st->start+2 )
		nd=npc_name2id(conv_str(st,& (st->stack->stack_data[st->start+2])));
	else
		nd=(struct npc_data *)map_id2bl(st->oid);

	if(nd==NULL || (cd=(struct chat_data *)map_id2bl(nd->chat_id))==NULL )
		return 0;
	chat_disableevent(cd);
	return 0;
}
/*==========================================
 * npcチャット状態所得
 *------------------------------------------
 */
int buildin_getwaitingroomstate(struct script_state *st)
{
	struct npc_data *nd;
	struct chat_data *cd;
	int val=-1,type;
	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	if( st->end > st->start+3 )
		nd=npc_name2id(conv_str(st,& (st->stack->stack_data[st->start+3])));
	else
		nd=(struct npc_data *)map_id2bl(st->oid);

	if(nd==NULL || (cd=(struct chat_data *)map_id2bl(nd->chat_id))==NULL ){
		push_val(st->stack,C_INT,-1);
		return 0;
	}

	switch(type){
	case 0: val=cd->users; break;
	case 1: val=cd->limit; break;
	case 2: val=cd->trigger&0x7f; break;
	case 3: val=((cd->trigger&0x80)>0); break;
	case 6: val=cd->zeny; break;
	case 7: val=cd->lowlv; break;
	case 8: val=cd->highlv; break;
	case 9: val=cd->job; break;
	case 10: val=cd->upper; break;
	case 32: val=(cd->users >= cd->limit); break;
	case 33: val=(cd->users >= cd->trigger); break;

	case 4:
		push_str(st->stack,C_CONSTSTR,cd->title);
		return 0;
	case 5:
		push_str(st->stack,C_CONSTSTR,cd->pass);
		return 0;
	case 16:
		push_str(st->stack,C_CONSTSTR,cd->npc_event);
		return 0;
	}
	push_val(st->stack,C_INT,val);
	return 0;
}

/*==========================================
 * チャットメンバー(規定人数)ワープ
 *------------------------------------------
 */
int buildin_warpwaitingpc(struct script_state *st)
{
	int x,y,i,n;
	char *str;
	struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);
	struct chat_data *cd;

	if(nd==NULL || (cd=(struct chat_data *)map_id2bl(nd->chat_id))==NULL )
		return 0;

	n=cd->trigger&0x7f;
	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	x=conv_num(st,& (st->stack->stack_data[st->start+3]));
	y=conv_num(st,& (st->stack->stack_data[st->start+4]));

	if( st->end > st->start+5 )
		n=conv_num(st,& (st->stack->stack_data[st->start+5]));

	for(i=0;i<n;i++){
		struct map_session_data *sd=cd->usersd[0];	// リスト先頭のPCを次々に。
		if( sd == NULL ) continue;

		mapreg_setreg(add_str("$@warpwaitingpc")+(i<<24),sd->bl.id);

		if(strcmp(str,"Random")==0)
			pc_randomwarp(sd,3);
		else if(strcmp(str,"SavePoint")==0){
			pc_setpos(sd,sd->status.save_point.map,
				sd->status.save_point.x,sd->status.save_point.y,3);
		}else
			pc_setpos(sd,str,x,y,0);
	}
	mapreg_setreg(add_str("$@warpwaitingpcnum"),n);
	return 0;
}

/*==========================================
 * NPCチャット内に居るPCのIDをリストアップする
 *------------------------------------------
 */
int buildin_getwaitingpcid(struct script_state *st)
{
	int i,j=0;
	struct map_session_data *sd = NULL;
	struct npc_data *nd;
	struct chat_data *cd;
	int num=st->stack->stack_data[st->start+2].u.num;
	char *name=str_buf+str_data[num&0x00ffffff].str;
	char prefix  = *name;
	char postfix = name[strlen(name)-1];

	if( prefix!='$' && prefix!='@' && prefix!='\'' ){
		printf("buildin_getwaitingpcid: illeagal scope !\n");
		return 0;
	}
	if( prefix!='$' && prefix !='\'')
		sd=script_rid2sd(st);

	if(st->end > st->start+3)
		nd=npc_name2id(conv_str(st,& (st->stack->stack_data[st->start+3])));
	else
		nd=(struct npc_data *)map_id2bl(st->oid);

	if(nd==NULL || (cd=(struct chat_data *)map_id2bl(nd->chat_id))==NULL)
		return 0;

	for(i=0; i<cd->users && j<128 ; i++) {
		struct map_session_data *pl_sd;
		pl_sd=cd->usersd[i];
		if(pl_sd) {
			void *v;
			char str[16];
			if( postfix == '$' ) {
				sprintf(str,"%d",pl_sd->bl.id);
				v = (void*)str;
			} else {
				v = (void*)pl_sd->bl.id;
			}
			set_reg(st,sd,num+(j<<24),name,v,st->stack->stack_data[st->start+2].ref);
			j++;
		}
	}
	return 0;
}

/*==========================================
 * RIDのアタッチ
 *------------------------------------------
 */
int buildin_attachrid(struct script_state *st)
{
	struct map_session_data *sd;
	st->rid=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd = map_id2sd(st->rid);
	push_val(st->stack,C_INT, (sd!=NULL));
	if(sd && sd->npc_id == 0) {
		sd->npc_id = st->oid;
	}
	return 0;
}

/*==========================================
 * RIDのデタッチ
 *------------------------------------------
 */
int buildin_detachrid(struct script_state *st)
{
	struct map_session_data *sd = map_id2sd(st->rid);
	st->rid=0;
	if(sd && sd->npc_id == st->oid) {
		sd->npc_id = 0;
	}
	return 0;
}
/*==========================================
 * 存在チェック
 *------------------------------------------
 */
int buildin_isloggedin(struct script_state *st)
{
	push_val(st->stack,C_INT, map_id2sd(
		conv_num(st,& (st->stack->stack_data[st->start+2])) )!=NULL );
	return 0;
}


/*==========================================
 *
 *------------------------------------------
 */
int buildin_setmapflagnosave(struct script_state *st)
{
	int m,x,y;
	char *str,*str2;

	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	str2=conv_str(st,& (st->stack->stack_data[st->start+3]));
	x=conv_num(st,& (st->stack->stack_data[st->start+4]));
	y=conv_num(st,& (st->stack->stack_data[st->start+5]));
	m = map_mapname2mapid(str);
	if(m >= 0) {
		map[m].flag.nosave=1;
		memcpy(map[m].save.map,str2,16);
		map[m].save.x=x;
		map[m].save.y=y;
	}

	return 0;
}

static int script_change_mapflag(int m,int type,int val)
{
	if( m >= 0 && (val == 0 || val == 1) ) {
		switch(type) {
			case MF_NOMEMO:
				map[m].flag.nomemo = val;
				break;
			case MF_NOTELEPORT:
				map[m].flag.noteleport = val;
				break;
			case MF_NOPORTAL:
				map[m].flag.noportal = val;
				break;
			case MF_NORETURN:
				map[m].flag.noreturn = val;
				break;
			case MF_MONSTER_NOTELEPORT:
				map[m].flag.monster_noteleport = val;
				break;
			case MF_NOBRANCH:
				map[m].flag.nobranch = val;
				break;
			case MF_NOPENALTY:
				map[m].flag.nopenalty = val;
				break;
			case MF_PVP_NOPARTY:
				map[m].flag.pvp_noparty = val;
				break;
			case MF_PVP_NOGUILD:
				map[m].flag.pvp_noguild = val;
				break;
			case MF_PVP_NOCALCRANK:
				map[m].flag.pvp_nocalcrank = val;
				break;
			case MF_GVG_NOPARTY:
				map[m].flag.gvg_noparty = val;
				break;
			case MF_NOZENYPENALTY:
				map[m].flag.nozenypenalty = val;
				break;
			case MF_NOTRADE:
				map[m].flag.notrade = val;
				break;
			case MF_NOSKILL:
				map[m].flag.noskill = val;
				break;
			case MF_NOABRA:
				map[m].flag.noabra = val;
				break;
			case MF_NODROP:
				map[m].flag.nodrop = val;
				break;
			case MF_NOICEWALL:
				map[m].flag.noicewall = val;
				break;
			case MF_TURBO:
				map[m].flag.turbo = val;
				break;
			case MF_NOREVIVE:
				map[m].flag.norevive = val;
				break;
			default:
				return 0;
		}
		map_field_setting();
	}
	return 0;
}

int buildin_setmapflag(struct script_state *st)
{
	int m,type;
	char *str;

	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	type=conv_num(st,& (st->stack->stack_data[st->start+3]));
	m = map_mapname2mapid(str);

	if(m >= 0)
		script_change_mapflag(m, type, 1);
	return 0;
}

int buildin_removemapflag(struct script_state *st)
{
	int m,type;
	char *str;

	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	type=conv_num(st,& (st->stack->stack_data[st->start+3]));
	m = map_mapname2mapid(str);

	if(m >= 0)
		script_change_mapflag(m, type, 0);
	return 0;
}

int buildin_pvpon(struct script_state *st)
{
	int m,i;
	char *str;
	struct map_session_data *pl_sd=NULL;

	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	m = map_mapname2mapid(str);
	if(m >= 0 && !map[m].flag.pvp) {
		map[m].flag.pvp = 1;
		clif_send0199(m,1);
		for(i=0;i<fd_max;i++){	//人数分ループ
			if(session[i] && (pl_sd=session[i]->session_data) && pl_sd->state.auth){
				if(m == pl_sd->bl.m && pl_sd->pvp_timer == -1) {
					pl_sd->pvp_timer=add_timer(gettick()+200,pc_calc_pvprank_timer,pl_sd->bl.id,0);
					pl_sd->pvp_rank=0;
					pl_sd->pvp_lastusers=0;
					pl_sd->pvp_point=5;
				}
			}
		}
		map_field_setting();
	}
	return 0;
}

int buildin_pvpoff(struct script_state *st)
{
	int m,i;
	char *str;
	struct map_session_data *pl_sd=NULL;

	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	m = map_mapname2mapid(str);
	if(m >= 0 && map[m].flag.pvp) {
		map[m].flag.pvp = 0;
		clif_send0199(m,0);
		for(i=0;i<fd_max;i++){	//人数分ループ
			if(session[i] && (pl_sd=session[i]->session_data) && pl_sd->state.auth){
				if(m == pl_sd->bl.m) {
					clif_pvpset(pl_sd,0,0,2);
					if(pl_sd->pvp_timer != -1) {
						delete_timer(pl_sd->pvp_timer,pc_calc_pvprank_timer);
						pl_sd->pvp_timer = -1;
					}
				}
			}
		}
		map_field_setting();
	}

	return 0;
}

int buildin_gvgon(struct script_state *st)
{
	int m;
	char *str;

	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	m = map_mapname2mapid(str);
	if(m >= 0 && !map[m].flag.gvg) {
		map[m].flag.gvg = 1;
		clif_send0199(m,3);
		map_field_setting();
	}

	return 0;
}
int buildin_gvgoff(struct script_state *st)
{
	int m;
	char *str;

	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	m = map_mapname2mapid(str);
	if(m >= 0 && map[m].flag.gvg) {
		map[m].flag.gvg = 0;
		clif_send0199(m,0);
		map_field_setting();
	}

	return 0;
}
/*==========================================
 *	NPCエモーション
 *------------------------------------------
 */
int buildin_emotion(struct script_state *st)
{
	struct npc_data *nd;
	int type;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	if( st->end > st->start+3 )
		nd=npc_name2id(conv_str(st,& (st->stack->stack_data[st->start+3])));
	else
		nd=(struct npc_data *)map_id2bl(st->oid);

	if(nd && type >= 0 && type <= 100) 
		clif_emotion(&nd->bl,type);
	return 0;
}

int buildin_maprespawnguildid_sub(struct block_list *bl,va_list ap)
{
	int g_id=va_arg(ap,int);
	int flag=va_arg(ap,int);
	struct map_session_data *sd=NULL;
	struct mob_data *md=NULL;

	if(bl->type == BL_PC)
		sd=(struct map_session_data*)bl;
	if(bl->type == BL_MOB)
		md=(struct mob_data *)bl;

	if(sd){
		if(flag&1 && g_id && sd->status.guild_id == g_id)
			pc_setpos(sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,3);
		else if(flag&2 && (!g_id || sd->status.guild_id != g_id))
			pc_setpos(sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,3);
	}
	if(flag&4 && md){
		if(md->class < 1285 || md->class > 1288)
			unit_remove_map(&md->bl,1);
	}
	return 0;
}
int buildin_maprespawnguildid(struct script_state *st)
{
	char *mapname=conv_str(st,& (st->stack->stack_data[st->start+2]));
	int g_id=conv_num(st,& (st->stack->stack_data[st->start+3]));
	int flag=conv_num(st,& (st->stack->stack_data[st->start+4]));

	int m = map_mapname2mapid(mapname);
	if(m >= 0)
		map_foreachinarea(buildin_maprespawnguildid_sub,m,0,0,map[m].xs-1,map[m].ys-1,BL_NUL,g_id,flag);
	return 0;
}

int buildin_agitstart(struct script_state *st)
{
	if(agit_flag==1) return 1;	  // Agit already Start.
	agit_flag=1;
	guild_agit_start();

	return 0;
}

int buildin_agitend(struct script_state *st)
{
	if(agit_flag==0) return 1;	  // Agit already End.
	agit_flag=0;
	guild_agit_end();
	return 0;
}
/*==========================================
 * agitcheck 1;	// choice script
 * if(@agit_flag == 1) goto agit;
 * if(agitcheck(0) == 1) goto agit;
 *------------------------------------------
 */
 /*
int buildin_agitcheck(struct script_state *st)
{
	struct map_session_data *sd;
	int cond;

//	sd=script_rid2sd(st);
	cond=conv_num(st,& (st->stack->stack_data[st->start+2]));

	if(cond == 0) {
		if (agit_flag==1) push_val(st->stack,C_INT,1);
		if (agit_flag==0) push_val(st->stack,C_INT,0);
	} else {
		if (agit_flag==1) pc_setreg(sd,add_str("@agit_flag"),1);
		if (agit_flag==0) pc_setreg(sd,add_str("@agit_flag"),0);
	}
	return 0;
}
*/
int buildin_agitcheck(struct script_state *st)
{
	if(conv_num(st,& (st->stack->stack_data[st->start+2])) == 0)
		push_val(st->stack,C_INT,agit_flag);
	else
		pc_setreg(script_rid2sd(st),add_str("@agit_flag"),agit_flag);
	return 0;
}

int buildin_flagemblem(struct script_state *st)
{
	int g_id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	struct block_list *bl=map_id2bl(st->oid);
	struct npc_data *nd;

	if(g_id < 0) return 0;

//	printf("Script.c: [FlagEmblem] GuildID=%d, Emblem=%d.\n", g->guild_id, g->emblem_id);
	if(bl && (nd=(struct npc_data *)bl)){
		nd->u.scr.guild_id = g_id;
		return 1;
	}

	return 0;
}

int buildin_getcastlename(struct script_state *st)
{
	char *mapname=conv_str(st,& (st->stack->stack_data[st->start+2]));
	struct guild_castle *gc;
	int i;
	char *buf=NULL;
	for(i=0;i<MAX_GUILDCASTLE;i++){
		if( (gc=guild_castle_search(i)) != NULL ){
			if(strcmp(mapname,gc->map_name)==0){
				buf=aStrdup(gc->castle_name);
				break;
			}
		}
	}
	if(buf)
		push_str(st->stack,C_STR,buf);
	else
		push_str(st->stack,C_CONSTSTR,"");
	return 0;
}

int buildin_getcastledata(struct script_state *st)
{
	char *mapname=conv_str(st,& (st->stack->stack_data[st->start+2]));
	int index=conv_num(st,& (st->stack->stack_data[st->start+3]));
	char *event=NULL;
	struct guild_castle *gc;
	int i,j;

	if( st->end>st->start+4 && index==0){
		for(i=0,j=-1;i<MAX_GUILDCASTLE;i++)
			if( (gc=guild_castle_search(i)) != NULL &&
				strcmp(mapname,gc->map_name)==0 )
				j=i;
		if(j>=0){
			event=conv_str(st,& (st->stack->stack_data[st->start+4]));
			guild_addcastleinfoevent(j,17,event);
		}
	}

	for(i=0;i<MAX_GUILDCASTLE;i++){
		if( (gc=guild_castle_search(i)) != NULL ){
			if(strcmp(mapname,gc->map_name)==0){
				switch(index){
				case 0: for(j=1;j<18;j++) guild_castledataload(gc->castle_id,j); break;  // Initialize[AgitInit]
				case 1: push_val(st->stack,C_INT,gc->guild_id); break;
				case 2: push_val(st->stack,C_INT,gc->economy); break;
				case 3: push_val(st->stack,C_INT,gc->defense); break;
				case 4: push_val(st->stack,C_INT,gc->triggerE); break;
				case 5: push_val(st->stack,C_INT,gc->triggerD); break;
				case 6: push_val(st->stack,C_INT,gc->nextTime); break;
				case 7: push_val(st->stack,C_INT,gc->payTime); break;
				case 8: push_val(st->stack,C_INT,gc->createTime); break;
				case 9: push_val(st->stack,C_INT,gc->visibleC); break;
				case 10: push_val(st->stack,C_INT,gc->visibleG0); break;
				case 11: push_val(st->stack,C_INT,gc->visibleG1); break;
				case 12: push_val(st->stack,C_INT,gc->visibleG2); break;
				case 13: push_val(st->stack,C_INT,gc->visibleG3); break;
				case 14: push_val(st->stack,C_INT,gc->visibleG4); break;
				case 15: push_val(st->stack,C_INT,gc->visibleG5); break;
				case 16: push_val(st->stack,C_INT,gc->visibleG6); break;
				case 17: push_val(st->stack,C_INT,gc->visibleG7); break;
				default:
					push_val(st->stack,C_INT,0); break;
				}
				return 0;
			}
		}
	}
	push_val(st->stack,C_INT,0);
	return 0;
}

int buildin_setcastledata(struct script_state *st)
{
	char *mapname=conv_str(st,& (st->stack->stack_data[st->start+2]));
	int index=conv_num(st,& (st->stack->stack_data[st->start+3]));
	int value=conv_num(st,& (st->stack->stack_data[st->start+4]));
	struct guild_castle *gc;
	int i;

	for(i=0;i<MAX_GUILDCASTLE;i++){
		if( (gc=guild_castle_search(i)) != NULL ){
			if(strcmp(mapname,gc->map_name)==0){
				// Save Data byself First
				switch(index){
				case 1: gc->guild_id = value; break;
				case 2: gc->economy = value; break;
				case 3: gc->defense = value; break;
				case 4: gc->triggerE = value; break;
				case 5: gc->triggerD = value; break;
				case 6: gc->nextTime = value; break;
				case 7: gc->payTime = value; break;
				case 8: gc->createTime = value; break;
				case 9: gc->visibleC = value; break;
				case 10: gc->visibleG0 = value; break;
				case 11: gc->visibleG1 = value; break;
				case 12: gc->visibleG2 = value; break;
				case 13: gc->visibleG3 = value; break;
				case 14: gc->visibleG4 = value; break;
				case 15: gc->visibleG5 = value; break;
				case 16: gc->visibleG6 = value; break;
				case 17: gc->visibleG7 = value; break;
				default: return 0;
				}
				guild_castledatasave(gc->castle_id,index,value);
				return 0;
			}
		}
	}
	return 0;
}

/* =====================================================================
 * ギルド情報を要求する
 * ---------------------------------------------------------------------
 */
int buildin_requestguildinfo(struct script_state *st)
{
	int guild_id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	char *event=NULL;

	if( st->end>st->start+3 )
		event=conv_str(st,& (st->stack->stack_data[st->start+3]));

	if(guild_id>0)
		guild_npc_request_info(guild_id,event);
	return 0;
}

/* =====================================================================
 * カードの数を得る
 * ---------------------------------------------------------------------
 */
int buildin_getequipcardcnt(struct script_state *st)
{
	int i,num;
	struct map_session_data *sd;
	int c=4;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=script_rid2sd(st);
	i=pc_checkequip(sd,equip[num-1]);

	if(i >= 0) {
		if(sd->status.inventory[i].card[0] == 0x00ff){ // 製造武器はカードなし
			push_val(st->stack,C_INT,0);
			return 0;
		}
		do{
			if( (sd->status.inventory[i].card[c-1] > 4000) &&
				(sd->status.inventory[i].card[c-1] < 5000)){

				push_val(st->stack,C_INT,(c));
				return 0;
			}
		}while(c--);
	}
	push_val(st->stack,C_INT,0);
	return 0;
}

/* ================================================================
 * カード取り外し成功
 * ----------------------------------------------------------------
 */
int buildin_successremovecards(struct script_state *st)
{
	int i,num,cardflag=0,flag;
	struct map_session_data *sd;
	struct item item_tmp;
	int c=4;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	sd=script_rid2sd(st);
	i=pc_checkequip(sd,equip[num-1]);

	if(i >= 0) {
		if(sd->status.inventory[i].card[0]==0x00ff){ // 製造武器は処理しない
			return 0;
		}
		do{
			if( (sd->status.inventory[i].card[c-1] > 4000) &&
				(sd->status.inventory[i].card[c-1] < 5000)){

				cardflag = 1;
				item_tmp.id=0,item_tmp.nameid=sd->status.inventory[i].card[c-1];
				item_tmp.equip=0,item_tmp.identify=1,item_tmp.refine=0;
				item_tmp.attribute=0;
				item_tmp.card[0]=0,item_tmp.card[1]=0,item_tmp.card[2]=0,item_tmp.card[3]=0;

				if((flag=pc_additem(sd,&item_tmp,1))){	// 持てないならドロップ
					clif_additem(sd,0,0,flag);
					map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
				}
			}
		}while(c--);
	}

	if(sd && cardflag == 1){	// カードを取り除いたアイテム所得
		flag=0;
		item_tmp.id=0,item_tmp.nameid=sd->status.inventory[i].nameid;
		item_tmp.equip=0,item_tmp.identify=1,item_tmp.refine=sd->status.inventory[i].refine;
		item_tmp.attribute=sd->status.inventory[i].attribute;
		item_tmp.card[0]=0,item_tmp.card[1]=0,item_tmp.card[2]=0,item_tmp.card[3]=0;
		pc_delitem(sd,i,1,0);
		if((flag=pc_additem(sd,&item_tmp,1))){	// もてないならドロップ
			clif_additem(sd,0,0,flag);
			map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
		}
		clif_misceffect(&sd->bl,3);
	}
	return 0;
}

/* ================================================================
 * カード取り外し失敗 slot,type
 * type=0: 両方損失、1:カード損失、2:武具損失、3:損失無し
 * ----------------------------------------------------------------
 */
int buildin_failedremovecards(struct script_state *st)
{
	int i,num,cardflag=0,flag,typefail;
	struct map_session_data *sd;
	struct item item_tmp;
	int c=4;

	num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	typefail=conv_num(st,& (st->stack->stack_data[st->start+3]));
	sd=script_rid2sd(st);
	i=pc_checkequip(sd,equip[num-1]);

	if(i >= 0) {
		if(sd->status.inventory[i].card[0]==0x00ff){ // 製造武器は処理しない
			return 0;
		}
		do{
			if(( sd->status.inventory[i].card[c-1] > 4000) &&
				 (sd->status.inventory[i].card[c-1] < 5000)){

				cardflag = 1;

				if(typefail == 2){ // 武具のみ損失なら、カードは受け取らせる
					item_tmp.id=0,item_tmp.nameid=sd->status.inventory[i].card[c-1];
					item_tmp.equip=0,item_tmp.identify=1,item_tmp.refine=0;
					item_tmp.attribute=0;
					item_tmp.card[0]=0,item_tmp.card[1]=0,item_tmp.card[2]=0,item_tmp.card[3]=0;
					if((flag=pc_additem(sd,&item_tmp,1))){
						clif_additem(sd,0,0,flag);
						map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
					}
				}
			}
		}while(c--);
	}

	if(sd && cardflag == 1){
		if(typefail == 0 || typefail == 2){	// 武具損失
			pc_delitem(sd,i,1,0);
		}
		else if(typefail == 1){	// カードのみ損失（武具を返す）
			flag=0;
			item_tmp.id=0,item_tmp.nameid=sd->status.inventory[i].nameid;
			item_tmp.equip=0,item_tmp.identify=1,item_tmp.refine=sd->status.inventory[i].refine;
			item_tmp.attribute=sd->status.inventory[i].attribute;
			item_tmp.card[0]=0,item_tmp.card[1]=0,item_tmp.card[2]=0,item_tmp.card[3]=0;
			pc_delitem(sd,i,1,0);
			if((flag=pc_additem(sd,&item_tmp,1))){
				clif_additem(sd,0,0,flag);
				map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
			}
		}
		clif_misceffect(&sd->bl,2);
	}
	return 0;
}

/* ================================================================
 * 結婚処理
 * ----------------------------------------------------------------
 */
int buildin_marriage(struct script_state *st)
{
	char *partner=conv_str(st,& (st->stack->stack_data[st->start+2]));
	struct map_session_data *sd=script_rid2sd(st);
	struct map_session_data *p_sd=map_nick2sd(partner);

	//養子キャラか判定追加 新職用
	if(sd==NULL || p_sd==NULL || pc_isbaby(sd) || pc_marriage(sd,p_sd) < 0)
	{
		push_val(st->stack,C_INT,0);
		return 0;
	}
	push_val(st->stack,C_INT,sd->status.partner_id);
	return 0;
}
/* ================================================================
 * 結婚式用のエフェクト
 * ----------------------------------------------------------------
 */
int buildin_wedding_effect(struct script_state *st)
{
	struct map_session_data *sd=script_rid2sd(st);

	if(sd==NULL)
		return 0;
	clif_wedding_effect(&sd->bl);
	return 0;
}
/* ================================================================
 * 離婚処理
 * ----------------------------------------------------------------
 */
int buildin_divorce(struct script_state *st)
{
	int num;
	int partner_id=0;
	struct map_session_data *sd=script_rid2sd(st);

	if( st->end>st->start+2 )
		num=conv_num(st,& (st->stack->stack_data[st->start+2]));
	else
		num=0;
	if(sd==NULL){
		push_val(st->stack,C_INT,0);
		return 0;
	}
	partner_id = sd->status.partner_id;
	if(num==0){
		if(pc_divorce(sd) < 0){
			push_val(st->stack,C_INT,0);
			return 0;
		}
		push_val(st->stack,C_INT,partner_id);
	}
	if(num==1)
		push_val(st->stack,C_INT,partner_id);

	return 0;
}

/* ================================================================
 * 養子処理
 * ----------------------------------------------------------------
 */

int buildin_adoption(struct script_state *st)
{
	char *papa_name=conv_str(st,& (st->stack->stack_data[st->start+2]));
	char *mama_name=conv_str(st,& (st->stack->stack_data[st->start+3]));
	struct map_session_data *sd=script_rid2sd(st);
	struct map_session_data *papa_sd=map_nick2sd(papa_name);
	struct map_session_data *mama_sd=map_nick2sd(mama_name);
	//成功
	if(pc_adoption_sub(sd,papa_sd,mama_sd))
	{
		push_val(st->stack,C_INT,1);
		return 1;
	}

	//失敗
	push_val(st->stack,C_INT,0);
	return 0;
}

/*==========================================
 * IDからItem名
 *------------------------------------------
 */
int buildin_getitemname(struct script_state *st)
{
	int item_id;
	struct item_data *i_data;

	item_id=conv_num(st,& (st->stack->stack_data[st->start+2]));

	i_data = itemdb_search(item_id);
	push_str(st->stack,C_CONSTSTR,i_data->jname);
	return 0;
}
/*==========================================
 * PCの所持品情報読み取り
 *------------------------------------------
 */
int buildin_getinventorylist(struct script_state *st)
{
	struct map_session_data *sd=script_rid2sd(st);
	int i,j=0;
	if(!sd) return 0;
	for(i=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid > 0 && sd->status.inventory[i].amount > 0){
			pc_setreg(sd,add_str("@inventorylist_id")+(j<<24),sd->status.inventory[i].nameid);
			pc_setreg(sd,add_str("@inventorylist_amount")+(j<<24),sd->status.inventory[i].amount);
			pc_setreg(sd,add_str("@inventorylist_equip")+(j<<24),sd->status.inventory[i].equip);
			pc_setreg(sd,add_str("@inventorylist_refine")+(j<<24),sd->status.inventory[i].refine);
			pc_setreg(sd,add_str("@inventorylist_identify")+(j<<24),sd->status.inventory[i].identify);
			pc_setreg(sd,add_str("@inventorylist_attribute")+(j<<24),sd->status.inventory[i].attribute);
			pc_setreg(sd,add_str("@inventorylist_card1")+(j<<24),sd->status.inventory[i].card[0]);
			pc_setreg(sd,add_str("@inventorylist_card2")+(j<<24),sd->status.inventory[i].card[1]);
			pc_setreg(sd,add_str("@inventorylist_card3")+(j<<24),sd->status.inventory[i].card[2]);
			pc_setreg(sd,add_str("@inventorylist_card4")+(j<<24),sd->status.inventory[i].card[3]);
			j++;
		}
	}
	pc_setreg(sd,add_str("@inventorylist_count"),j);
	return 0;
}

int buildin_getskilllist(struct script_state *st)
{
	struct map_session_data *sd=script_rid2sd(st);
	int i,j=0;
	if(!sd) return 0;
	for(i=0;i<MAX_SKILL;i++){
		if(sd->status.skill[i].id > 0 && sd->status.skill[i].lv > 0){
			pc_setreg(sd,add_str("@skilllist_id")+(j<<24),sd->status.skill[i].id);
			pc_setreg(sd,add_str("@skilllist_lv")+(j<<24),sd->status.skill[i].lv);
			pc_setreg(sd,add_str("@skilllist_flag")+(j<<24),sd->status.skill[i].flag);
			j++;
		}
	}
	pc_setreg(sd,add_str("@skilllist_count"),j);
	return 0;
}

int buildin_clearitem(struct script_state *st)
{
	struct map_session_data *sd=script_rid2sd(st);
	int i;
	if(sd==NULL) return 0;
	for (i=0; i<MAX_INVENTORY; i++) {
		if (sd->status.inventory[i].amount)
			pc_delitem(sd, i, sd->status.inventory[i].amount, 0);
	}
	return 0;
}
/*==========================================
 * アイテム修理関連
 * 修理可能アイテムを数える buildin_getrepairableitemcount
 * 修理可能アイテムを修理する buildin_repairitem
 *------------------------------------------
 */
int buildin_getrepairableitemcount(struct script_state *st)
{
	struct map_session_data *sd=script_rid2sd(st);
	int i,c=0;

	if(sd){
		for(i=0;i<MAX_INVENTORY;i++){
			if(sd->status.inventory[i].nameid > 0 && sd->status.inventory[i].amount > 0 && sd->status.inventory[i].attribute){
				c++;
			}
		}
		push_val(st->stack,C_INT,c);
	}
	return 0;
}
int buildin_repairitem(struct script_state *st)
{
	struct map_session_data *sd=script_rid2sd(st);
	int i,c=0;
	if(sd){
		for(i=0;i<MAX_INVENTORY;i++){
			if(sd->status.inventory[i].nameid > 0 && sd->status.inventory[i].amount > 0 && sd->status.inventory[i].attribute){
				sd->status.inventory[i].attribute = 0;
				c++;
			}
		}
		clif_itemlist(sd);
		clif_equiplist(sd);
	}
	return 0;
}
/*==========================================
 * NPCクラスチェンジ
 * classは変わりたいclass
 * typeは通常0なのかな？
 *------------------------------------------
 */
int buildin_classchange(struct script_state *st)
{
	int class,type;
	struct block_list *bl=map_id2bl(st->oid);

	if(bl==NULL) return 0;

	class=conv_num(st,& (st->stack->stack_data[st->start+2]));
	type=conv_num(st,& (st->stack->stack_data[st->start+3]));
	clif_class_change(bl,class,type);
	return 0;
}

/*==========================================
 * NPCから発生するエフェクト
 *------------------------------------------
 */
int buildin_misceffect(struct script_state *st)
{
	struct npc_data *nd;
	int type;

	type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	if( st->end > st->start+3 )
		nd=npc_name2id(conv_str(st,& (st->stack->stack_data[st->start+3])));
	else
		nd=(struct npc_data *)map_id2bl(st->oid);
	if(nd)
		clif_misceffect2(&nd->bl,type);
	else{
		struct map_session_data *sd=script_rid2sd(st);
		if(sd)
			clif_misceffect2(&sd->bl,type);
	}
	return 0;
}

/*==========================================
 * エリア内のPCに発生するエフェクト
 *------------------------------------------
 */
int buildin_misceffect_sub(struct block_list *bl,va_list ap)
{
	int type=va_arg(ap,int);

	clif_misceffect3(bl,type);
	return 0;
}

int buildin_areamisceffect(struct script_state *st)
{
	char *str;
	int type=0,m,x0,y0,x1,y1;

	str  = conv_str(st,& (st->stack->stack_data[st->start+2]));
	x0   = conv_num(st,& (st->stack->stack_data[st->start+3]));
	y0   = conv_num(st,& (st->stack->stack_data[st->start+4]));
	x1   = conv_num(st,& (st->stack->stack_data[st->start+5]));
	y1   = conv_num(st,& (st->stack->stack_data[st->start+6]));
	type = conv_num(st,& (st->stack->stack_data[st->start+7]));

	if(strcmp(str,"this")==0){
		struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);
		if(nd)
			m=nd->bl.m;
		else {
			struct map_session_data *sd=script_rid2sd(st);
			if(sd)
				m=sd->bl.m;
			else
				return 0;
		}
	}
	else if( (m=map_mapname2mapid(str))< 0)
		return 0;
	map_foreachinarea(buildin_misceffect_sub,m,x0,y0,x1,y1,BL_PC,type);
	return 0;
}

/*==========================================
 * サウンドエフェクト
 *------------------------------------------
 */
int buildin_soundeffect(struct script_state *st)
{
	struct map_session_data *sd=script_rid2sd(st);
	char *name;
	int type=0;

	name=conv_str(st,& (st->stack->stack_data[st->start+2]));
	type=conv_num(st,& (st->stack->stack_data[st->start+3]));
	if(sd){
		if(st->oid)
			clif_soundeffect(sd,map_id2bl(st->oid),name,type);
		else{
			clif_soundeffect(sd,&sd->bl,name,type);
		}
	}
	return 0;
}

/*==========================================
 * 範囲指定サウンドエフェクト
 *------------------------------------------
 */
int buildin_soundeffect_sub(struct block_list *bl,va_list ap)
{
	char *name=va_arg(ap,char *);
	int type=va_arg(ap,int);
	struct map_session_data *sd=(struct map_session_data *)bl;

	if(sd)
		clif_soundeffect(sd,bl,name,type);
	return 0;
}

int buildin_areasoundeffect(struct script_state *st)
{
	char *name,*str;
	int type=0,m,x0,y0,x1,y1;

	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	x0=conv_num(st,& (st->stack->stack_data[st->start+3]));
	y0=conv_num(st,& (st->stack->stack_data[st->start+4]));
	x1=conv_num(st,& (st->stack->stack_data[st->start+5]));
	y1=conv_num(st,& (st->stack->stack_data[st->start+6]));
	name=conv_str(st,& (st->stack->stack_data[st->start+7]));
	type=conv_num(st,& (st->stack->stack_data[st->start+8]));

	if(strcmp(str,"this")==0){
		struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);
		if(nd)
			m=nd->bl.m;
		else {
			struct map_session_data *sd=script_rid2sd(st);
			if(sd)
				m=sd->bl.m;
			else
				return 0;
		}
	}
	else if( (m=map_mapname2mapid(str))< 0)
		return 0;
	map_foreachinarea(buildin_soundeffect_sub,m,x0,y0,x1,y1,BL_PC,name,type);
	return 0;
}

/*==========================================
 * gmcommand
 * suggested on the forums...
 *------------------------------------------
 */
int buildin_gmcommand(struct script_state *st)
{
	struct map_session_data *sd = map_id2sd(st->rid);
	char cmd[100];
	int dummy=0;

	// 人が居ないイベント系NPCで使った場合のためのダミーmap_session_data
	if(sd==NULL){
		struct npc_data *nd = (struct npc_data *)map_id2bl(st->oid);
		sd = (struct map_session_data *)aCalloc(1,sizeof(struct map_session_data));
		sd->fd = 0;
		sd->bl.m = nd->bl.m;
		sd->bl.x = nd->bl.x;
		sd->bl.y = nd->bl.y;
		strcpy(sd->status.name,"dummy");
		dummy=1;
	}
	sprintf(cmd ,"%s : %s",sd->status.name,conv_str(st,& (st->stack->stack_data[st->start+2])));

	is_atcommand(sd->fd, sd, cmd, 99);

	if(dummy==1)
		aFree(sd);

	return 0;
}
/*==========================================
 * コメント欄にメッセージ表示
 *------------------------------------------
 */
int buildin_dispbottom(struct script_state *st)
{
	struct map_session_data *sd=script_rid2sd(st);
	char *message;
	message=conv_str(st,& (st->stack->stack_data[st->start+2]));
	if(sd)
		clif_disp_onlyself(sd,message,strlen(message));
	return 0;
}
/*==========================================
 * サーバー上の全員を全回復(蘇生+HP/SP全回復)
 *------------------------------------------
 */
int buildin_recovery(struct script_state *st)
{
	int i = 0;
	for (i = 0; i < fd_max; i++) {
		if (session[i]){
			struct map_session_data *sd = session[i]->session_data;
			if (sd && sd->state.auth) {
				sd->status.hp = sd->status.max_hp;
				sd->status.sp = sd->status.max_sp;
				clif_updatestatus(sd, SP_HP);
				clif_updatestatus(sd, SP_SP);
				if(unit_isdead(&sd->bl)){
					pc_setstand(sd);
					clif_resurrection(&sd->bl, 1);
				}
				clif_displaymessage(sd->fd,"完全回復しました！");
			}
		}
	}
	return 0;
}
/*==========================================
 * 孵化させて連れ歩いているペットの情報取得
 * 0:pet_id 1:pet_class 2:pet_name
 * 3:friendly 4:hungry
 * 連れてない時は、いずれの場合も0を返す
 *------------------------------------------
 */
int buildin_getpetinfo(struct script_state *st)
{
	struct map_session_data *sd=script_rid2sd(st);
	int type=conv_num(st,& (st->stack->stack_data[st->start+2]));

	if(sd && sd->status.pet_id){
		switch(type){
			case 0:
				push_val(st->stack,C_INT,sd->status.pet_id);
				break;
			case 1:
				if(sd->pet.class)
					push_val(st->stack,C_INT,sd->pet.class);
				else
					push_val(st->stack,C_INT,0);
				break;
			case 2:
				if(sd->pet.name)
					push_str(st->stack,C_STR,aStrdup(sd->pet.name));
				else
					push_val(st->stack,C_INT,0);
				break;
			case 3:
				//if(sd->pet.intimate)
				push_val(st->stack,C_INT,sd->pet.intimate);
				break;
			case 4:
				//if(sd->pet.hungry)
				push_val(st->stack,C_INT,sd->pet.hungry);
				break;
			default:
				push_val(st->stack,C_INT,0);
				break;
		}
	}else{
		push_val(st->stack,C_INT,0);
	}
	return 0;
}
/*==========================================
 * 連れ歩いているホムンクルスの情報取得
 * 0:homun_id 1:homun_base_lv 2:homun_name
 * 3:friendly 4:hungry 5:homun_class
 * 連れてない時は、いずれの場合も0を返す
 *------------------------------------------
 */
int buildin_gethomuninfo(struct script_state *st)
{
	struct map_session_data *sd=script_rid2sd(st);
	int type=conv_num(st,& (st->stack->stack_data[st->start+2]));

	if(sd && sd->status.homun_id){
		switch(type){
			case 0:
				push_val(st->stack,C_INT,sd->status.homun_id);
				break;
			case 1:
				if(sd->hd && sd->hd->status.base_level)
					push_val(st->stack,C_INT,sd->hd->status.base_level);
				else
					push_val(st->stack,C_INT,0);
				break;
			case 2:
				if(sd->hd && sd->hd->status.name)
					push_str(st->stack,C_STR,aStrdup(sd->hd->status.name));
				else
					push_val(st->stack,C_INT,0);
				break;
			case 3:
				//if(sd->hd->intimate)
				if( sd->hd ) 
					push_val(st->stack,C_INT,sd->hd->intimate);
				else
					push_val(st->stack,C_INT,0);
				break;
			case 4:
				//if(sd->hd->status.hungry)
				if( sd->hd )
					push_val(st->stack,C_INT,sd->hd->status.hungry);
				else
					push_val(st->stack,C_INT,0);
				break;
			case 5:
				//if(sd->hd->status.class)
				if( sd->hd )
					push_val(st->stack,C_INT,sd->hd->status.class);
				else
					push_val(st->stack,C_INT,0);
				break;
			default:
				push_val(st->stack,C_INT,0);
				break;
		}
	}else{
		push_val(st->stack,C_INT,0);
	}
	return 0;
}
/*==========================================
 * 指定IDのカードを付けた武具がないか検査(あったら1,無ければ0を返す)
 *------------------------------------------
 */
int buildin_checkequipedcard(struct script_state *st)
{
	struct map_session_data *sd=script_rid2sd(st);
	int n,i,c=0;
	c=conv_num(st,& (st->stack->stack_data[st->start+2]));

	if(sd){
		// 所持アイテム
		for(i=0;i<MAX_INVENTORY;i++){
			if(sd->status.inventory[i].nameid > 0 && sd->status.inventory[i].amount){
				for(n=0;n<4;n++){
					if(sd->status.inventory[i].card[n]==c){
						push_val(st->stack,C_INT,1);
						return 0;
					}
				}
			}
		}
		// カート内アイテム
		for(i=0;i<MAX_CART;i++){
			if(sd->status.cart[i].nameid > 0 && sd->status.cart[i].amount){
				for(n=0;n<4;n++){
					if(sd->status.cart[i].card[n]==c){
						push_val(st->stack,C_INT,1);
						return 0;
					}
				}
			}
		}
	}
	push_val(st->stack,C_INT,0);
	return 0;
}

int buildin_jump_zero(struct script_state *st) {
	int sel;
	sel=conv_num(st,& (st->stack->stack_data[st->start+2]));
	if(!sel) {
		int pos;
		if( st->stack->stack_data[st->start+3].type!=C_POS ){
			printf("script: jump_zero: not label !\n");
			st->state=END;
			return 0;
		}

		pos=conv_num(st,& (st->stack->stack_data[st->start+3]));
		st->pos=pos;
		st->state=GOTO;
		// printf("script: jump_zero: jumpto : %d\n",pos);
	} else {
		// printf("script: jump_zero: fail\n");
	}
	return 0;
}

int buildin_select(struct script_state *st)
{
	char *buf;
	int len,i;
	struct map_session_data *sd = script_rid2sd(st);

	nullpo_retr(0, sd);

	if(sd->state.menu_or_input==0){
		st->state=RERUNLINE;
		sd->state.menu_or_input=1;
		for(i=st->start+2,len=0;i<st->end;i++){
			conv_str(st,& (st->stack->stack_data[i]));
			len+=strlen(st->stack->stack_data[i].u.str)+1;
		}
		buf=(char *)aCalloc(len+1,sizeof(char));
		buf[0]=0;
		for(i=st->start+2,len=0;i<st->end;i++){
			if( st->stack->stack_data[i].u.str[0] ) {
				strcat(buf,st->stack->stack_data[i].u.str);
				strcat(buf,":");
			}
		}
		clif_scriptmenu(script_rid2sd(st),st->oid,buf);
		free(buf);
	} else if(sd->npc_menu==0xff){	// cancel
		sd->state.menu_or_input=0;
		st->state=END;
	} else {
		sd->state.menu_or_input=0;
		if(sd->npc_menu>0){
			//空文字メニューの補正
			for(i=st->start+2;i<=(st->start+sd->npc_menu+1) && sd->npc_menu < st->end-st->start-1;i++) {
				conv_str(st,& (st->stack->stack_data[i]));
				if(strlen(st->stack->stack_data[i].u.str) < 1)
					sd->npc_menu++;
			}
			if(sd->npc_menu >= st->end-st->start-1) {
				st->state=END;
				return 0;
			}
			//pc_setreg(sd,add_str("l15"),sd->npc_menu);
			pc_setreg(sd,add_str("@menu"),sd->npc_menu);
			push_val(st->stack,C_INT,sd->npc_menu);
		} else {
			// 不正な値なのでキャンセル扱い
			sd->state.menu_or_input=0;
			st->state=END;
		}
	}
	return 0;
}
/*==========================================
 * マップ指定mob数取得
 *------------------------------------------
 */
int buildin_getmapmobs_sub(struct block_list *bl,va_list ap)
{
	int *count=va_arg(ap,int *);
	(*count)++;
	return 0;
}
int buildin_getmapmobs(struct script_state *st)
{
	char *str;
	int m,count=0;
	str=conv_str(st,& (st->stack->stack_data[st->start+2]));

	if(strcmp(str,"this")==0){
		struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);
		if(nd)
			m=nd->bl.m;
		else {
			struct map_session_data *sd=script_rid2sd(st);
			if(sd)
				m=sd->bl.m;
			else {
				push_val(st->stack,C_INT,-1);
				return 0;
			}
		}
	}else
		m=map_mapname2mapid(str);

	if(m < 0){
		push_val(st->stack,C_INT,-1);
		return 0;
	}
	map_foreachinarea(buildin_getmapmobs_sub,m,0,0,map[m].xs,map[m].ys,BL_MOB,&count);
	push_val(st->stack,C_INT,count);
	return 0;
}

int buildin_getareamobs(struct script_state *st)
{
	char *str;
	int m,x0,y0,x1,y1,count=0;
	str=conv_str(st,& (st->stack->stack_data[st->start+2]));
	x0=conv_num(st,& (st->stack->stack_data[st->start+3]));
	y0=conv_num(st,& (st->stack->stack_data[st->start+4]));
	x1=conv_num(st,& (st->stack->stack_data[st->start+5]));
	y1=conv_num(st,& (st->stack->stack_data[st->start+6]));

	if(strcmp(str,"this")==0){
		struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);
		if(nd)
			m=nd->bl.m;
		else {
			struct map_session_data *sd=script_rid2sd(st);
			if(sd)
				m=sd->bl.m;
			else {
				push_val(st->stack,C_INT,-1);
				return 0;
			}
		}
	}else
		m=map_mapname2mapid(str);

	if(m < 0){
		push_val(st->stack,C_INT,-1);
		return 0;
	}
	map_foreachinarea(buildin_getmapmobs_sub,m,x0,y0,x1,y1,BL_MOB,&count);
	push_val(st->stack,C_INT,count);
	return 0;
}

/*==========================================
 *ギルド同士の関係を調べる
 *------------------------------------------
 */
int buildin_getguildrelation(struct script_state *st)
{
	int gld1, gld2, result;
	struct map_session_data *sd;
	struct guild *g=NULL;

	gld1 = conv_num(st,& (st->stack->stack_data[st->start+2]));
	g = guild_search(gld1);
	if(g == NULL)
	{
		push_val(st->stack,C_INT,-1);
		return 0;
	}


	if( st->end>st->start+3 )
		gld2 = conv_num(st,& (st->stack->stack_data[st->start+3]));
	else
	{
		sd = script_rid2sd(st);
		if(sd==NULL)
		{
			push_val(st->stack,C_INT,-1);
			return 0;
		}
		gld2 = sd->status.guild_id;
	}

	g = NULL;
	g = guild_search(gld2);
	if(g == NULL)
	{
		push_val(st->stack,C_INT,-1);
		return 0;
	}

	result = 0;
	if(gld1 == gld2)
		result += 1;
	if(guild_check_alliance(gld1, gld2, 0))
		result += 2;
	if(guild_check_alliance(gld1, gld2, 1))
		result += 4;
	if(guild_check_alliance(gld2, gld1, 1))
		result += 8;

	push_val(st->stack,C_INT,result);
	return 0;
}

/*==========================================
 * 武装解除
 *------------------------------------------
 */
int buildin_unequip(struct script_state *st)
{
	struct map_session_data *sd=script_rid2sd(st);
	int i = -1;

	nullpo_retr(0, sd);

	if( st->end>st->start+2 )
		i = conv_num(st,& (st->stack->stack_data[st->start+2]));

	if( i>=0 )
	{
		if(sd->equip_index[i] >= 0 )
			pc_unequipitem(sd,sd->equip_index[i],0);
	}
	else
	{
		for(i=0;i<11;i++) {
			if(sd->equip_index[i] >= 0 )
				pc_unequipitem(sd,sd->equip_index[i],0);
		}
	}
	return 0;
}

/*==========================================
 * アイテム使用許可
 *------------------------------------------
 */
int buildin_allowuseitem(struct script_state *st)
{
	struct map_session_data *sd=script_rid2sd(st);
	struct script_data *data;
	int i = 0;

	nullpo_retr(0, sd);

	if( st->end>st->start+2 )
	{
		data=&(st->stack->stack_data[st->start+2]);
		get_val(st,data);
		if( isstr(data) ){
			const char *name=conv_str(st,data);
			struct item_data *item_data = itemdb_searchname(name);
			i=-1;
			if( item_data )
				i=item_data->nameid;
		}else
			i=conv_num(st,data);
	}
	sd->npc_allowuseitem = i;
	return 0;
}
/*==========================================
 * 指定キャラの居るマップ名取得
 *------------------------------------------
 */
int buildin_getmapname(struct script_state *st)
{
	struct map_session_data *sd;
	char *char_name = conv_str(st,& (st->stack->stack_data[st->start+2]));

	if( strlen(char_name) >= 4 )
		sd = map_nick2sd(char_name);
	else
		sd = script_rid2sd(st);

	if(sd)
		push_str(st->stack,C_STR,aStrdup(sd->mapname));
	else
		push_str(st->stack,C_CONSTSTR,"");

	return 0;
}

int buildin_summon(struct script_state *st)
{
	int _class, id;
	char *str,*event="";
	struct map_session_data *sd = script_rid2sd(st);
	struct mob_data *md;

	if (sd) {
		int tick = gettick();
		str	=conv_str(st,& (st->stack->stack_data[st->start+2]));
		_class=conv_num(st,& (st->stack->stack_data[st->start+3]));
		if( st->end>st->start+4 )
			event=conv_str(st,& (st->stack->stack_data[st->start+4]));

		id=mob_once_spawn(sd, "this", 0, 0, str,_class,1,event);
		if((md=(struct mob_data *)map_id2bl(id))){
			md->master_id=sd->bl.id;
			md->state.special_mob_ai=1;
			md->mode=mob_db[md->class].mode|0x04;
			md->deletetimer=add_timer(tick+60000,mob_timer_delete,id,0);
			clif_misceffect2(&md->bl,344);
		}
		clif_skill_poseffect(&sd->bl,AM_CALLHOMUN,1,sd->bl.x,sd->bl.y,tick);
	}

	return 0;
}

/*==========================================
  * Get position for  char/npc/pet/mob objects. Added by Lorky
  *
  *	 int getMapXY(MapName$,MaxX,MapY,type,[CharName$]);
  *		 where type:
  *			 MapName$ - String variable for output map name
  *			 MapX	 - Integer variable for output coord X
  *			 MapY	 - Integer variable for output coord Y
  *			 type	 - type of object
  *				0 - Character coord
  *				1 - NPC coord
  *				2 - Pet coord
  *				3 - Mob coord (not released)
  *				4 - HOM coord
  *			 CharName$ - Name object. If miss or "this" the current object
  *
  *		 Return:
  *			 0	- success
  *			 -1	   - some error, MapName$,MapX,MapY contains unknown value.
  *------------------------------------------
*/ //[#cbf145a0]
int buildin_getmapxy(struct script_state *st){
	struct map_session_data *sd=NULL;
	struct npc_data *nd;
	struct pet_data *pd;
	struct homun_data *hd;

	int num;
	char *name;
	char prefix;

	int x,y,type;
	char mapname[24];

	if( st->stack->stack_data[st->start+2].type!=C_NAME ){
		printf("script: buildin_getmapxy: not mapname variable\n");
		push_val(st->stack,C_INT,-1);
		return 0;
	}
	if( st->stack->stack_data[st->start+3].type!=C_NAME ){
		printf("script: buildin_getmapxy: not mapx variable\n");
		push_val(st->stack,C_INT,-1);
		return 0;
	}
	if( st->stack->stack_data[st->start+4].type!=C_NAME ){
		printf("script: buildin_getmapxy: not mapy variable\n");
		push_val(st->stack,C_INT,-1);
		return 0;
	}

	//??????????? >>>  Possible needly check function parameters on C_STR,C_INT,C_INT <<< ???????????//
	type=conv_num(st,& (st->stack->stack_data[st->start+5]));

	memset(&mapname,'\0',sizeof(mapname));
	switch (type){
		case 0:						 //Get Character Position
			if( st->end>st->start+6 )
				sd=map_nick2sd(conv_str(st,& (st->stack->stack_data[st->start+6])));
			else
				sd=script_rid2sd(st);

			if ( sd==NULL ) {		//wrong char name or char offline
				push_val(st->stack,C_INT,-1);
				return 0;
			}
			x=sd->bl.x;
			y=sd->bl.y;
			memcpy(mapname,sd->mapname,24);
			break;
		case 1:						//Get NPC Position
			if( st->end > st->start+6 )
				nd=npc_name2id(conv_str(st,& (st->stack->stack_data[st->start+6])));
			else
				nd=(struct npc_data *)map_id2bl(st->oid);
			if ( nd==NULL ) {		//wrong npc name or char offline
				push_val(st->stack,C_INT,-1);
				return 0;
			}
			x=nd->bl.x;
			y=nd->bl.y;
			if(nd->bl.m == -1)
				mapname[0] = '-';		//マップに配置されないNPC
			else
				memcpy(mapname,map[nd->bl.m].name,24);
			break;
		case 2:						//Get Pet Position
			if( st->end>st->start+6 )
				sd=map_nick2sd(conv_str(st,& (st->stack->stack_data[st->start+6])));
			else
				sd=script_rid2sd(st);
			if ( sd==NULL ) {		//wrong char name or char offline
				push_val(st->stack,C_INT,-1);
				return 0;
			}
			pd=sd->pd;
			if ( pd==NULL ){			//pet data not found
				push_val(st->stack,C_INT,-1);
				return 0;
			}
			x=pd->bl.x;
			y=pd->bl.y;
			memcpy(mapname,map[pd->bl.m].name,24);
			break;
		case 3:						//Get Mob Position
			push_val(st->stack,C_INT,-1);
			return 0;
		case 4:
			if( st->end>st->start+6 )
				sd=map_nick2sd(conv_str(st,& (st->stack->stack_data[st->start+6])));
			else
				sd=script_rid2sd(st);
			if ( sd==NULL ) {			//wrong char name or char offline
				push_val(st->stack,C_INT,-1);
				return 0;
			}
			hd=sd->hd;
			if ( hd==NULL ){			//homun data not found
				push_val(st->stack,C_INT,-1);
				return 0;
			}
			x=hd->bl.x;
			y=hd->bl.y;
			memcpy(mapname,map[hd->bl.m].name,24);
			break;
		default:					//Wrong type parameter
			push_val(st->stack,C_INT,-1);
			return 0;
	}

	//Set MapName$
	num=st->stack->stack_data[st->start+2].u.num;
	name=(char *)(str_buf+str_data[num&0x00ffffff].str);
	prefix=*name;

	if( prefix!='$' && prefix != '\'')
		sd=script_rid2sd(st);
	else
		sd=NULL;

	set_reg(st,sd,num,name,(void*)mapname,st->stack->stack_data[st->start+2].ref);

	//Set MapX
	num=st->stack->stack_data[st->start+3].u.num;
	name=(char *)(str_buf+str_data[num&0x00ffffff].str);
	prefix=*name;

	if( prefix!='$' && prefix != '\'')
		sd=script_rid2sd(st);
	else
		sd=NULL;
	set_reg(st,sd,num,name,(void*)x,st->stack->stack_data[st->start+3].ref);

	//Set MapY
	num=st->stack->stack_data[st->start+4].u.num;
	name=(char *)(str_buf+str_data[num&0x00ffffff].str);
	prefix=*name;

	if( prefix!='$' && prefix != '\'')
		sd=script_rid2sd(st);
	else
		sd=NULL;

	set_reg(st,sd,num,name,(void*)y,st->stack->stack_data[st->start+4].ref);

	//Return Success value
	push_val(st->stack,C_INT,0);
	return 0;
}

/*==========================================
 * Checkcart [Valaris]
 *------------------------------------------
 */

int buildin_checkcart(struct script_state *st)
{
	struct map_session_data *sd = script_rid2sd(st);

	if(pc_iscarton(sd)){
		push_val(st->stack,C_INT,1);
	} else {
		push_val(st->stack,C_INT,0);
	}
	return 0;
}
/*==========================================
 * checkfalcon [Valaris]
 *------------------------------------------
 */

int buildin_checkfalcon(struct script_state *st)
{
	struct map_session_data *sd = script_rid2sd(st);

	if(pc_isfalcon(sd)){
		push_val(st->stack,C_INT,1);
	} else {
		push_val(st->stack,C_INT,0);
	}

	return 0;
}
/*==========================================
 * Checkriding [Valaris]
 *------------------------------------------
 */

int buildin_checkriding(struct script_state *st)
{
	struct map_session_data *sd = script_rid2sd(st);

	if(pc_isriding(sd)){
		push_val(st->stack,C_INT,1);
	} else {
		push_val(st->stack,C_INT,0);
	}

	return 0;
}

/*==========================================
 * pet attack skills [Valaris] //Rewritten by [Skotlex]
 *------------------------------------------
 */
int buildin_petskillattack(struct script_state *st)
{
	struct pet_data *pd;
	struct map_session_data *sd=script_rid2sd(st);

	if(sd==NULL || sd->pd==NULL)
		return 0;

	pd=sd->pd;
	if (pd->a_skill == NULL)
		pd->a_skill = (struct pet_skill_attack *)aCalloc(1, sizeof(struct pet_skill_attack));

	pd->a_skill->id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	if(pd->a_skill->id == -1) {
		// remove pet skills
		aFree(pd->a_skill);
		pd->a_skill = NULL;
	} else {
		pd->a_skill->lv=conv_num(st,& (st->stack->stack_data[st->start+3]));
		pd->a_skill->div_ = 0;
		pd->a_skill->rate=conv_num(st,& (st->stack->stack_data[st->start+4]));
		pd->a_skill->bonusrate=conv_num(st,& (st->stack->stack_data[st->start+5]));
	}
	return 0;
}

/*==========================================
 * pet support skills [Skotlex]
 *------------------------------------------
 */
int buildin_petskillsupport(struct script_state *st)
{
	struct pet_data *pd;
	struct map_session_data *sd=script_rid2sd(st);

	if(sd==NULL || sd->pd==NULL)
		return 0;

	pd=sd->pd;
	if (pd->s_skill)
	{ //Clear previous skill
		if (pd->s_skill->timer != -1)
			delete_timer(pd->s_skill->timer, pet_skill_support_timer);
	} else //init memory
		pd->s_skill = (struct pet_skill_support *) aCalloc(1, sizeof(struct pet_skill_support));

	pd->s_skill->id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	if(pd->s_skill->id == -1) {
		// remove pet skills
		aFree(pd->s_skill);
		pd->s_skill = NULL;
	} else {
		pd->s_skill->lv=conv_num(st,& (st->stack->stack_data[st->start+3]));
		pd->s_skill->delay=conv_num(st,& (st->stack->stack_data[st->start+4]));
		pd->s_skill->hp=conv_num(st,& (st->stack->stack_data[st->start+5]));
		pd->s_skill->sp=conv_num(st,& (st->stack->stack_data[st->start+6]));

		//Use delay as initial offset to avoid skill/heal exploits
		//if (battle_config.pet_equip_required && pd->equip == 0)
		//	pd->s_skill->timer=-1;
		//else
			pd->s_skill->timer=add_timer(gettick()+pd->s_skill->delay*1000, pet_skill_support_timer ,sd->bl.id,0);
	}

	return 0;
}
/*==========================================
 * ペットのルートタイプの変更
 *------------------------------------------
 */
int buildin_changepettype(struct script_state *st)
{
	struct map_session_data *sd;
	struct pet_data *pd=NULL;
	int type=conv_num(st,& (st->stack->stack_data[st->start+2]));

	sd=script_rid2sd(st);

	if(type>2 || type<0)
		type=0;

	if(sd && sd->status.pet_id)
		pd=sd->pd;
	if(pd)
		pd->loottype=(short)type;
	return 0;
}

/*==========================================
 * 料理用
 *------------------------------------------
 */
int buildin_making(struct script_state *st)
{
	struct map_session_data *sd;
	int makeid=conv_num(st,& (st->stack->stack_data[st->start+2]));

	sd=script_rid2sd(st);
	if(sd){
		clif_skill_produce_mix_list(sd,makeid);
		sd->making_base_success_per = conv_num(st,& (st->stack->stack_data[st->start+3]));
	}
	return 0;
}

/*==========================================
 *指定マップのpvp,gvgフラグを調べる
 *------------------------------------------
 */
int buildin_getpkflag(struct script_state *st)
{
	char *str=NULL;
	int m=-1;
	int count=0;

	str=conv_str(st,& (st->stack->stack_data[st->start+2]));

	if(strcmp(str,"this")==0){
		struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);
		if(nd)
			m=nd->bl.m;
		else {
			struct map_session_data *sd=script_rid2sd(st);
			if(sd)
				m=sd->bl.m;
			else {
				push_val(st->stack,C_INT,-1);
				return 0;
			}
		}
	}else
		m=map_mapname2mapid(str);

	if(m < 0){
		push_val(st->stack,C_INT,-1);
		return 0;
	}

	if(map[m].flag.pvp)
		count+=1;
	if(map[m].flag.gvg)
		count+=2;
//	if(map[m].flag.pk)
//		count+=4;

	push_val(st->stack,C_INT,count);
	return 0;
}

/*==========================================
 * Gain guild exp [Celest]
 * guildgetexp <exp>
 *------------------------------------------
 */
int buildin_guildgetexp(struct script_state *st)
{
	struct map_session_data *sd = script_rid2sd(st);
	int exp;

	exp = conv_num(st,& (st->stack->stack_data[st->start+2]));

	if(exp > 0 && sd && sd->status.guild_id > 0)
		guild_getexp(sd, exp);

	return 0;
}

/*==========================================
 * flagname [Sapientia]
 *------------------------------------------
 */
int buildin_flagname(struct script_state *st)
{
	struct block_list *bl=map_id2bl(st->oid);
	struct npc_data *nd;
	char *name;

	name=conv_str(st,& (st->stack->stack_data[st->start+2]));

	if(bl && (nd=(struct npc_data *)bl) && strlen(name)>0){
		memset(nd->name, '\0', sizeof(nd->name));
		strncpy(nd->name, name, sizeof(nd->name)-1);
		return 1;
	}

	return 0;
}

/*==========================================
 * getnpcposition [Sapientia]
 *------------------------------------------
 */
int buildin_getnpcposition(struct script_state *st)
{
	struct block_list *bl=map_id2bl(st->oid);
	struct npc_data *nd;

	char *str;
	str=conv_str(st,& (st->stack->stack_data[st->start+2]));

	if(bl && (nd=(struct npc_data *)bl) && strlen(str)>0){
		memset(nd->position, '\0', sizeof(nd->position));
		strncpy(nd->name, str, sizeof(nd->name)-1);
		return 1;
	}
	return 0;
}

#if !defined(NO_CSVDB) && !defined(NO_CSVDB_SCRIPT)
static struct dbt* script_csvdb;

int script_csvinit( void ) {
	script_csvdb = strdb_init(0);
	return 0;
}

static int script_csvfinal_sub( void *key, void *data, va_list ap ) {
	aFree(key);
	csvdb_close( data );
	return 0;
}

int script_csvfinal( void ) {
	strdb_final( script_csvdb, script_csvfinal_sub );
	return 0;
}

static struct csvdb_data* script_csvload( const char *file ) {
	struct csvdb_data *csv = strdb_search( script_csvdb, file);
	if( csv == NULL ) {
		// ファイル名に変なものが入っていないか確認
		int i;
		for(i = 0; file[i]; i++) {
			switch(file[i]) {
			case '.':
				if(file[i+1] != '.') break;
				// fall through
			case '<':
			case '>':
			case '|':
				printf("script_csvload: invalid file name %s\n", file);
				return NULL;
			}
		}
		csv = csvdb_open( file, 0 );
		if( csv ) {
			printf("script_csvload: %s load successfully\n", file);
			strdb_insert( script_csvdb, aStrdup(file), csv );
		}
	}
	return csv;
}

int buildin_csvgetrows(struct script_state *st) {
	char *file   = conv_str(st,& (st->stack->stack_data[st->start+2]));
	struct csvdb_data *csv = script_csvload( file );
	if( csv == NULL ) {
		push_val(st->stack,C_INT,-1);
	} else {
		push_val(st->stack,C_INT,csvdb_get_rows(csv));
	}
	return 0;
}

int buildin_csvgetcols(struct script_state *st) {
	char *file   = conv_str(st,& (st->stack->stack_data[st->start+2]));
	int  row     = conv_num(st,& (st->stack->stack_data[st->start+3]));
	struct csvdb_data *csv = script_csvload( file );
	if( csv == NULL) {
		push_val(st->stack,C_INT,-1);
	} else {
		push_val(st->stack,C_INT,csvdb_get_columns(csv, row));
	}
	return 0;
}

// csvread <file>, <row>, <cow>
int buildin_csvread(struct script_state *st)
{
	char *file = conv_str(st,& (st->stack->stack_data[st->start+2]));
	int  row   = conv_num(st,& (st->stack->stack_data[st->start+3]));
	int  col   = conv_num(st,& (st->stack->stack_data[st->start+4]));
	const char *v;
	struct csvdb_data *csv = script_csvload( file );
	v = (csv ? csvdb_get_str(csv, row, col) : NULL);

	if( v ) {
		push_str(st->stack,C_STR,aStrdup(v));
	} else {
		push_str(st->stack,C_CONSTSTR,"");
	}
	return 0;
}

// csvreadarray <file>, <row>, <array>
int buildin_csvreadarray(struct script_state *st)
{
	int i;
	struct map_session_data *sd = NULL;
	char *file   = conv_str(st,& (st->stack->stack_data[st->start+2]));
	int  row     = conv_num(st,& (st->stack->stack_data[st->start+3]));
	int  num     = st->stack->stack_data[st->start+4].u.num;
	char *name   = str_buf+str_data[num&0x00ffffff].str;
	char prefix  = *name;
	char postfix = name[strlen(name)-1];
	struct csvdb_data* csv = script_csvload( file );

	// clear array
	if( prefix!='$' && prefix!='@' && prefix!='\'' ){
		printf("buildin_csvreadarray: illeagal scope !\n");
		return 0;
	}
	if( prefix!='$' && prefix != '\'') {
		sd=script_rid2sd(st);
		if( sd == NULL) return 0;
	}

	for(i = getarraysize(st,num,postfix,st->stack->stack_data[st->start+4].ref) - (num >> 24) - 1;i >= 0;i--) {
		if( postfix=='$' )
			set_reg(st,sd,num+(i<<24),name,"",st->stack->stack_data[st->start+4].ref);
		else
			set_reg(st,sd,num+(i<<24),name,0,st->stack->stack_data[st->start+4].ref);
	}

	if( csv ) {
		int i, max = csvdb_get_columns( csv, row );
		if( max + (num >> 24) > 127 ) {
			max = 127 - (num>>24);
		}
		for( i = 0; i < max; i++ ) {
			if( postfix == '$' ) {
				// set_regはconstが付いてないので、一端strdupしている
				char *v = aStrdup(csvdb_get_str(csv, row, i));
				set_reg(st,sd,num+(i<<24),name,v,st->stack->stack_data[st->start+4].ref);
				aFree(v);
			} else {
				set_reg(st,sd,num+(i<<24),name,(void*)csvdb_get_num(csv, row, i),st->stack->stack_data[st->start+4].ref);
			}
		}
	}
	return 0;
}

// csvfind <file>, <col>, <val>
int buildin_csvfind(struct script_state *st)
{
	char *file = conv_str(st,& (st->stack->stack_data[st->start+2]));
	int   col  = conv_num(st,& (st->stack->stack_data[st->start+3]));
	struct csvdb_data *csv = script_csvload( file );

	if( !csv ) {
		push_val(st->stack,C_INT,-1);
	} else if( isstr(&st->stack->stack_data[st->start+4])) {
		char *str = conv_str(st,& (st->stack->stack_data[st->start+4]));
		push_val(st->stack,C_INT,csvdb_find_str(csv, col, str));
	} else {
		int  val  = conv_num(st,& (st->stack->stack_data[st->start+4]));
		push_val(st->stack,C_INT,csvdb_find_num(csv, col, val));
	}
	return 0;
}

// csvwrite <file>, <row>, <col>, <val>
int buildin_csvwrite(struct script_state *st)
{
	char *file = conv_str(st,& (st->stack->stack_data[st->start+2]));
	int  row   = conv_num(st,& (st->stack->stack_data[st->start+3]));
	int  col   = conv_num(st,& (st->stack->stack_data[st->start+4]));
	struct csvdb_data *csv;
	int  i;

	// ファイル名が妥当なものかチェックする
	for(i = 0; file[i]; i++) {
		if( !isalnum( (unsigned char)file[i] ) ) {
			printf("buildin_csvwrite: invalid file name %s\n", file);
			return 0;
		}
	}
	csv = script_csvload( file );

	if( isstr(&st->stack->stack_data[st->start+5]) ) {
		char *str = conv_str(st,& (st->stack->stack_data[st->start+5]));
		csvdb_set_str( csv, row, col, str);
	} else {
		int   val = conv_num(st,& (st->stack->stack_data[st->start+5]));
		csvdb_set_num( csv, row, col, val);
	}
	return 0;
}

// csvwritearray <file>, <row>, <array>
int buildin_csvwritearray(struct script_state *st)
{
	int i;
	char *file   = conv_str(st,& (st->stack->stack_data[st->start+2]));
	int  row     = conv_num(st,& (st->stack->stack_data[st->start+3]));
	int  num     = st->stack->stack_data[st->start+4].u.num;
	char *name   = str_buf+str_data[num&0x00ffffff].str;
	char prefix  = *name;
	char postfix = name[strlen(name)-1];
	struct csvdb_data* csv;

	if( prefix!='$' && prefix!='@' && prefix!='\'' ){
		printf("buildin_csvwritearray: illeagal scope !\n");
		return 0;
	}

	// ファイル名が妥当なものかチェックする
	for(i = 0; file[i]; i++) {
		if( !isalnum( (unsigned char)file[i] ) ) {
			printf("buildin_csvwrite: invalid file name %s\n", file);
			return 0;
		}
	}

	csv = script_csvload( file );
	if( csv ) {
		int max = getarraysize(st, num, postfix, st->stack->stack_data[st->start+4].ref) - (num >> 24);
		csvdb_clear_row( csv, row );
		for( i = 0; i < max; i++ ) {
			if( postfix == '$' ) {
				csvdb_set_str(csv, row, i, get_val2(st, num+(i<<24),st->stack->stack_data[st->start+4].ref));
			} else {
				csvdb_set_num(csv, row, i, (int)get_val2(st, num+(i<<24),st->stack->stack_data[st->start+4].ref));
			}
		}
	}
	return 0;
}

static int script_csvreload_sub( void *key, void *data, va_list ap ) {
	char *file = va_arg(ap, char*);
	if( strcmp(key, file) == 0 ) {
		char *p = key;
		strdb_erase( script_csvdb, key );
		aFree( p );
	}
	return 0;
}

// csvreload <file>
int buildin_csvreload(struct script_state *st) {
	char *file = conv_str(st,& (st->stack->stack_data[st->start+2]));
	struct csvdb_data *csv = strdb_search( script_csvdb, file );
	if( csv ) {
		//strdb_insert( script_csvdb, file, NULL );
		strdb_foreach( script_csvdb, script_csvreload_sub, file );
		csvdb_close( csv );
	}
	return 0;
}

// csvinsert <file>, <row>
int buildin_csvinsert(struct script_state *st) {
	int  i;
	char *file  = conv_str(st,& (st->stack->stack_data[st->start+2]));
	int   row   = conv_num(st,& (st->stack->stack_data[st->start+3]));
	struct csvdb_data *csv;

	// ファイル名が妥当なものかチェックする
	for(i = 0; file[i]; i++) {
		if( !isalnum( (unsigned char)file[i] ) ) {
			printf("buildin_csvinsert: invalid file name %s\n", file);
			return 0;
		}
	}
	csv = script_csvload( file );

	if( csv ) {
		csvdb_insert_row(csv, row);
	}
	return 0;
}

// csvdelete <file>, <row>
int buildin_csvdelete(struct script_state *st) {
	int  i;
	char *file  = conv_str(st,& (st->stack->stack_data[st->start+2]));
	int   row   = conv_num(st,& (st->stack->stack_data[st->start+3]));
	struct csvdb_data *csv;

	// ファイル名が妥当なものかチェックする
	for(i = 0; file[i]; i++) {
		if( !isalnum( (unsigned char)file[i] ) ) {
			printf("buildin_csvdelete: invalid file name %s\n", file);
			return 0;
		}
	}
	csv = script_csvload( file );

	if( csv ) {
		csvdb_delete_row(csv, row);
	}
	return 0;

}

// csvsort <file>, <col>, <order>
int buildin_csvsort(struct script_state *st) {
	int  i;
	char *file  = conv_str(st,& (st->stack->stack_data[st->start+2]));
	int   col   = conv_num(st,& (st->stack->stack_data[st->start+3]));
	int   order = conv_num(st,& (st->stack->stack_data[st->start+4]));
	struct csvdb_data *csv;

	// ファイル名が妥当なものかチェックする
	for(i = 0; file[i]; i++) {
		if( !isalnum( (unsigned char)file[i] ) ) {
			printf("buildin_csvsort: invalid file name %s\n", file);
			return 0;
		}
	}
	csv = script_csvload( file );

	if( csv ) {
		csvdb_sort(csv, col, order);
	}
	return 0;
}

// csvflush <file>
int buildin_csvflush(struct script_state *st) {
	char *file  = conv_str(st,& (st->stack->stack_data[st->start+2]));
	int i;
	struct csvdb_data *csv;

	// ファイル名が妥当なものかチェックする
	for(i = 0; file[i]; i++) {
		if( !isalnum( (unsigned char)file[i] ) ) {
			printf("buildin_csvflush: invalid file name %s\n", file);
			return 0;
		}
	}

	csv = strdb_search( script_csvdb, file );
	if( csv ) {
		csvdb_flush( csv );
	}
	return 0;
}

#endif

// sleep <mili sec>
int buildin_sleep(struct script_state *st) {
	int tick = conv_num(st,& (st->stack->stack_data[st->start+2]));
	struct map_session_data *sd = map_id2sd(st->rid);
	if(sd && sd->npc_id == st->oid) {
		sd->npc_id = 0;
	}
	st->rid = 0;
	if(tick <= 0) {
		// 何もしない
	} else if( !st->sleep.tick ) {
		// 初回実行
		st->state = RERUNLINE;
		st->sleep.tick = tick;
	} else {
		// 続行
		st->sleep.tick = 0;
	}
	return 0;
}

// sleep2 <mili sec>
int buildin_sleep2(struct script_state *st) {
	int tick = conv_num(st,& (st->stack->stack_data[st->start+2]));
	if( tick <= 0 ) {
		// 0ms の待機時間を指定された
		push_val(st->stack,C_INT,map_id2sd(st->rid) != NULL);
	} else if( !st->sleep.tick ) {
		// 初回実行時
		st->state = RERUNLINE;
		st->sleep.tick = tick;
	} else {
		push_val(st->stack,C_INT,map_id2sd(st->rid) != NULL);
		st->sleep.tick = 0;
	}
	return 0;
}

/*==========================================
 * 指定NPCの全てのsleepを再開する
 *------------------------------------------
 */
int buildin_awake(struct script_state *st)
{
	struct npc_data *nd;
	struct linkdb_node *node = (struct linkdb_node *)sleep_db;

	nd = npc_name2id(conv_str(st,& (st->stack->stack_data[st->start+2])));
	if(nd == NULL)
		return 0;

	while( node ) {
		if( (int)node->key == nd->bl.id) {
			struct script_state *tst    = node->data;
			struct map_session_data *sd = map_id2sd(tst->rid);

			if( tst->sleep.timer == -1 ) {
				node = node->next;
				continue;
			}
			if( sd && sd->char_id != tst->sleep.charid )
				tst->rid = 0;

			delete_timer(tst->sleep.timer, run_script_timer);
			node = script_erase_sleepdb(node);
			tst->sleep.timer = -1;
			run_script_main(tst);
		} else {
			node = node->next;
		}
	}
	return 0;
}

// getvariableofnpc(<param>, <npc name>);
int buildin_getvariableofnpc(struct script_state *st)
{
	if( st->stack->stack_data[st->start+2].type != C_NAME ) {
		// 第一引数が変数名じゃない
		printf("getvariableofnpc: param not name\n");
		push_val(st->stack,C_INT,0);
	} else {
		int num = st->stack->stack_data[st->start+2].u.num;
		char *var_name = str_buf+str_data[num&0x00ffffff].str;
		char *npc_name = conv_str(st,& (st->stack->stack_data[st->start+3]));
		struct npc_data *nd = npc_name2id(npc_name);
		if( var_name[0] != '\'' || var_name[1] == '@' ) {
			// ' 変数以外はダメ
			printf("getvariableofnpc: invalid scope %s\n", var_name);
			push_val(st->stack,C_INT,0);
		} else if( nd == NULL || nd->bl.subtype != SCRIPT || !nd->u.scr.script) {
			// NPC が見つからない or SCRIPT以外のNPC
			printf("getvariableofnpc: can't find npc %s\n", npc_name);
			push_val(st->stack,C_INT,0);
		} else {
			push_val2(st->stack,C_NAME,num, &nd->u.scr.script->script_vars );
		}
	}
	return 0;
}
/*==========================================
 * 見た目サイズの変更
 * type1=PC type2=PET type3=HOM type4=NPC
 * 小 size=-1 大 size=1
 *------------------------------------------
 */
int buildin_changeviewsize(struct script_state *st)
{
	struct block_list *bl = NULL;
	struct map_session_data *sd;
	int type=conv_num(st,& (st->stack->stack_data[st->start+2]));
	short size=(short)conv_num(st,& (st->stack->stack_data[st->start+3]));

	sd = map_id2sd(st->rid);
	if(sd == NULL && type != 4)
		return 1;

	size = (size<0)? -1: (size>0)? 1: 0;

	switch (type) {
		case 1:
			bl = &sd->bl;
			break;
		case 2:
			if(sd->pd)
				bl = &sd->pd->bl;
			break;
		case 3:
			if(sd->hd)
				bl = &sd->hd->bl;
			break;
		case 4:
			bl = map_id2bl(st->oid);
			break;
		default:
			break;
	}
	if(bl)
		unit_changeviewsize(bl,size);

	return 0;
}

/*==========================================
 * アイテム使用時、使ったアイテムのIDや製作者IDを返す
 * usediteminfo flag; 0=使用アイテムID 1=製作者ID
 *------------------------------------------
 */
int buildin_usediteminfo(struct script_state *st)
{
	struct map_session_data *sd;
	int flag=conv_num(st,& (st->stack->stack_data[st->start+2]));
	int ret;

	if((sd=script_rid2sd(st))==NULL){
		push_val(st->stack,C_INT,-1);
		return -1;
	}
	if(flag==0)
		ret = sd->use_itemid;
	else
		ret = sd->use_nameditem;
	push_val(st->stack,C_INT,ret);
	return 0;
}
/*==========================================
 * アイテム名から、IDを取得
 *------------------------------------------
 */
int buildin_getitemid(struct script_state *st)
{
	int itemid;
	struct item_data *item_data;
	char *item_name = conv_str(st,& (st->stack->stack_data[st->start+2]));

	item_data = itemdb_searchname(item_name);
	if(item_data!=NULL)
		itemid=item_data->nameid;
	else
		itemid=-1;
	push_val(st->stack,C_INT,itemid);
	return 0;
}
/*==========================================
 * ギルドのメンバー数を取得(接続数ではなく登録数)
 *------------------------------------------
 */
int buildin_getguildmembers(struct script_state *st)
{
	int gid,i,n=0;
	struct guild *g=NULL;

	gid = conv_num(st,& (st->stack->stack_data[st->start+2]));
	g = guild_search(gid);
	if(g == NULL){
		push_val(st->stack,C_INT,-1);
		return 0;
	}
	for(i=0;i<g->max_member;i++) {
		if(g->member[i].account_id!=0)
			n++;
	}

	push_val(st->stack,C_INT,n);
	return 0;
}

/*==========================================
 * NPC攻撃スキルエフェクト
 *------------------------------------------
 */
int buildin_npcskillattack(struct script_state *st)
{
	struct map_session_data *sd = script_rid2sd(st);
	struct npc_data *nd = (struct npc_data *)map_id2bl(st->oid);

	int id=conv_num(st,& (st->stack->stack_data[st->start+2]));
	int lv=conv_num(st,& (st->stack->stack_data[st->start+3]));
	int damage=conv_num(st,& (st->stack->stack_data[st->start+4]));

	nullpo_retr(0, sd);
	nullpo_retr(0, nd);

	clif_skill_damage(&nd->bl,&sd->bl,gettick(),status_get_amotion(&nd->bl),status_get_dmotion(&sd->bl),
				damage, skill_get_num(id,lv), id, lv, skill_get_hit(id));
	return 0;
}

/*==========================================
 * NPC支援/回復スキルエフェクト
 *------------------------------------------
 */
int buildin_npcskillsupport(struct script_state *st)
{
	struct map_session_data *sd = script_rid2sd(st);
	struct npc_data *nd = (struct npc_data *)map_id2bl(st->oid);

	int skillid=conv_num(st,& (st->stack->stack_data[st->start+2]));
	int heal=conv_num(st,& (st->stack->stack_data[st->start+3]));

	nullpo_retr(0, sd);
	nullpo_retr(0, nd);

	clif_skill_nodamage(&nd->bl,&sd->bl,skillid,heal,1);

	return 0;
}

/*==========================================
 * NPC場所指定スキルエフェクト
 *------------------------------------------
 */
int buildin_npcskillpos(struct script_state *st)
{
	struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);

	int skillid=conv_num(st,& (st->stack->stack_data[st->start+2]));
	int skilllv=conv_num(st,& (st->stack->stack_data[st->start+3]));
	int x=conv_num(st,& (st->stack->stack_data[st->start+4]));
	int y=conv_num(st,& (st->stack->stack_data[st->start+5]));

	nullpo_retr(0, nd);

	clif_skill_poseffect(&nd->bl,skillid,skilllv,x,y,gettick());

	return 0;
}

/*==========================================
 * 呼び出し元のNPC情報を取得する
 *------------------------------------------
 */
int buildin_strnpcinfo(struct script_state *st)
{
	struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);
	int type;
	char *buf,*name=NULL;
	type=conv_num(st,& (st->stack->stack_data[st->start+2]));

	if(nd==NULL) {
		push_str(st->stack,C_CONSTSTR,"");
		return 0;
	}
	switch(type) {
		case 0:
			name=aStrdup(nd->name);
			break;
		case 1:
			name=strtok(aStrdup(nd->name),"#");
			break;
		case 2:
			if((buf=strchr(nd->name,'#')) != NULL)
				name=aStrdup(buf+1);
			break;
		case 3:
			name=aStrdup(nd->exname);
			break;
		case 4:
			name=strtok(aStrdup(nd->exname),"#");
			break;
		case 5:
			if((buf=strchr(nd->exname,'#')) != NULL)
				name=aStrdup(buf+1);
			break;
		case 6:
			name=aStrdup(nd->position);
			break;
	}
	if(name)
		push_str(st->stack,C_STR,name);
	else
		push_str(st->stack,C_CONSTSTR,"");
	return 0;
}

/*==========================================
 * 指定PTのPTリーダーのキャラ名を返す
 *------------------------------------------
 */
int buildin_getpartyleader(struct script_state *st)
{
	int i,pt_id;
	struct party *p=NULL;

	pt_id=conv_num(st,& (st->stack->stack_data[st->start+2]));

	p=party_search(pt_id);
	if(p==NULL) {
		push_str(st->stack,C_CONSTSTR,"");
		return 0;
	}
	for(i=0;i<MAX_PARTY;i++){
		if(p->member[i].leader) {
			push_str(st->stack,C_STR,aStrdup(p->member[i].name));
			return 0;
		}
	}
	push_str(st->stack,C_CONSTSTR,"");	//リーダーが見つからない
	return 0;
}

/*==========================================
 * 文字列中の<n>番目の文字１バイトを返す
 *------------------------------------------
 */
int buildin_strcut(struct script_state *st)
{
	char *str,buf[2];
	int n,len;

	str = conv_str(st,& (st->stack->stack_data[st->start+2]));
	n = conv_num(st,& (st->stack->stack_data[st->start+3]));

	len = strlen(str);
	if(len > 0 && n >= 0 && n < len) {
		buf[0] = *(str+n);
		buf[1] = '\0';
		push_str(st->stack,C_STR,aStrdup(buf));
	} else {
		push_str(st->stack,C_CONSTSTR,"");
	}
	return 0;
}

/*==========================================
 * 指定アイテムのUseScriptを変更する
 *------------------------------------------
 */
int buildin_setusescript(struct script_state *st)
{
	int nameid, type;
	char *p;
	struct item_data *id;

	nameid	= conv_num(st,& (st->stack->stack_data[st->start+2]));
	p = conv_str(st,& (st->stack->stack_data[st->start+3]));
	type = st->stack->stack_data[st->start+3].type;

	if( (id=itemdb_exists(nameid)) == NULL )
		return 0;
	if(*p != '{') {
		printf("buildin_setusescript: not found '{'\n");
		return 0;
	}
	if(id->use_script) {
		script_free_code(id->use_script);
	}
	if( type == C_CONSTSTR ) {
		p = aStrdup( p );
	}
	id->use_script=parse_script(p,"buildin_setusescript",0);
	if( type == C_CONSTSTR ) {
		aFree( p );
	}
	return 0;
}

/*==========================================
 * 指定アイテムのEquipScriptを変更する
 *------------------------------------------
 */
int buildin_setequipscript(struct script_state *st)
{
	int nameid, type;
	char *p;
	struct item_data *id;

	nameid	= conv_num(st,& (st->stack->stack_data[st->start+2]));
	p = conv_str(st,& (st->stack->stack_data[st->start+3]));
	type = st->stack->stack_data[st->start+3].type;

	if( (id=itemdb_exists(nameid)) == NULL )
		return 0;
	if(*p != '{') {
		printf("buildin_setequipscript: not found '{'\n");
		return 0;
	}
	if(id->equip_script) {
		script_free_code(id->equip_script);
	}
	if( type == C_CONSTSTR ) {
		p = aStrdup( p );
	}
	id->equip_script=parse_script(p,"buildin_setequipscript",0);
	if( type == C_CONSTSTR ) {
		aFree( p );
	}
	return 0;
}

/*==========================================
 * PCとNPC間の距離を返す
 *------------------------------------------
 */
int buildin_distance(struct script_state *st)
{
	struct map_session_data *sd;
	struct npc_data *nd;

	sd = map_id2sd(conv_num(st,& (st->stack->stack_data[st->start+2])));
	if(st->end > st->start+3)
		nd = npc_name2id(conv_str(st,& (st->stack->stack_data[st->start+3])));
	else
		nd = (struct npc_data *)map_id2bl(st->oid);

	if(sd && nd && sd->bl.m == nd->bl.m)
		push_val(st->stack,C_INT,unit_distance2(&sd->bl,&nd->bl));
	else
		push_val(st->stack,C_INT,-1);
	return 0;
}

/*==========================================
 * ホムンクルス削除
 *------------------------------------------
 */
int buildin_homundel(struct script_state *st)
{
	homun_delete_data( script_rid2sd(st) );
	return 0;
}

/*==========================================
 * ホムンクルスリネーム
 *------------------------------------------
 */
int buildin_homunrename(struct script_state *st)
{
	char *homname;
	short flag=0;
	struct map_session_data *sd = script_rid2sd(st);

	homname	= conv_str(st,& (st->stack->stack_data[st->start+2]));
	if( st->end > st->start+3 ){
		flag = (short)conv_num(st,& (st->stack->stack_data[st->start+3]));	
	}

	if(sd && sd->hd) {
		if(flag == 0 && sd->hd->status.rename_flag == 0) {
			// 変更前の状態に戻す
			flag = 1;
		}
		sd->hd->status.rename_flag = 0;
		homun_change_name(sd,homname);

		// 変更可能にする
		if(flag == 1)
			sd->hd->status.rename_flag = 0;
	}
	return 0;
}
