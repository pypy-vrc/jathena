#ifndef _DB_H_
#define _DB_H_

#include <stdarg.h>

#define HASH_SIZE (256+27)

#define RED 0
#define BLACK 1

struct dbn {
	struct dbn *parent,*left,*right;
	int color;
	void *key;
	void *data;
	int  deleted;	// 削除済みフラグ(db_foreach)
};
struct dbt {
	int (*cmp)(struct dbt*,void*,void*);
	unsigned int (*hash)(struct dbt*,void*);
	int maxlen;
	struct dbn *ht[HASH_SIZE];
	int item_count; // 要素の数
	const char* alloc_file; // DBが初期化されたファイル
	int         alloc_line; // DBが初期化された行

	// db_foreach 内部でdb_erase される対策として、
	// db_foreach が終わるまでロックすることにする
	struct db_free {
		struct dbn *z;
		struct dbn **root;
	} *free_list;
	int free_count;
	int free_max;
	int free_lock;
};

#define strdb_search(t,k)   db_search((t),(void*)(k))
#define strdb_insert(t,k,d) db_insert((t),(void*)(k),(void*)(d))
#define strdb_erase(t,k)    db_erase ((t),(void*)(k))
#define strdb_foreach       db_foreach
#define strdb_final         db_final
#define numdb_search(t,k)   db_search((t),(void*)(k))
#define numdb_insert(t,k,d) db_insert((t),(void*)(k),(void*)(d))
#define numdb_erase(t,k)    db_erase ((t),(void*)(k))
#define numdb_foreach       db_foreach
#define numdb_final         db_final

#define strdb_init(a)       strdb_init_(a,__FILE__,__LINE__)
#define numdb_init()        numdb_init_(__FILE__,__LINE__)

struct dbt* strdb_init_(int maxlen,const char *file,int line);
struct dbt* numdb_init_(const char *file,int line);
void* db_search(struct dbt *table,void* key);
struct dbn* db_insert(struct dbt *table,void* key,void* data);
void* db_erase(struct dbt *table,void* key);
void db_foreach(struct dbt*,int(*)(void*,void*,va_list),...);
void db_foreach_sub(struct dbt*,int(*)(void*,void*,va_list), va_list ap);
void db_final(struct dbt*,int(*)(void*,void*,va_list),...);
void exit_dbn(void);

// リンクリストDB -- treedb よりも規模が小さいやつ向けのデータベース
// 　・キーの重複チェックはreplace のみ
// 　・負荷対策のため、事前の初期化はheadにNULLを代入するだけ
// 　・linkdb_node は関数内で確保するため、利用側はポインタ１つを宣言するだけ

struct linkdb_node {
	struct linkdb_node *next;
	struct linkdb_node *prev;
	void               *key;
	void               *data;
};

void  linkdb_insert ( struct linkdb_node** head, void *key, void* data); // 重複を考慮しない
void  linkdb_replace( struct linkdb_node** head, void *key, void* data); // 重複を考慮する
void* linkdb_search ( struct linkdb_node** head, void *key);
void* linkdb_erase  ( struct linkdb_node** head, void *key);
void  linkdb_final  ( struct linkdb_node** head );

#endif
