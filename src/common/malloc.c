#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define _MALLOC_C_
#include "malloc.h"

// �Ǝ��������}�l�[�W�����g�p����ꍇ�A���̃R�����g���O���Ă��������B
#define USE_MEMMGR

#ifndef USE_MEMMGR

void* aMalloc_( size_t size, const char *file, int line, const char *func )
{
	void *ret;
	
	// printf("%s:%d: in func %s: malloc %d\n",file,line,func,size);
#ifdef MEMWATCH
	ret=mwMalloc(size,file,line);
#else
	ret=malloc(size);
#endif
	if(ret==NULL){
		printf("%s:%d: in func %s: malloc error out of memory!\n",file,line,func);
		exit(1);

	}
	return ret;
}

void* aCalloc_( size_t num, size_t size, const char *file, int line, const char *func )
{
	void *ret;
	
	// printf("%s:%d: in func %s: calloc %d %d\n",file,line,func,num,size);
#ifdef MEMWATCH
	ret=mwCalloc(num,size,file,line);
#else
	ret=calloc(num,size);
#endif
	if(ret==NULL){
		printf("%s:%d: in func %s: calloc error out of memory!\n",file,line,func);
		exit(1);

	}
	return ret;
}

void* aRealloc_( void *p, size_t size, const char *file, int line, const char *func )
{
	void *ret;
	
	// printf("%s:%d: in func %s: realloc %p %d\n",file,line,func,p,size);
#ifdef MEMWATCH
	ret=mwRealloc(p,size,file,line);
#else
	ret=realloc(p,size);
#endif
	if(ret==NULL){
		printf("%s:%d: in func %s: realloc error out of memory!\n",file,line,func);
		exit(1);

	}
	return ret;
}

void* aStrdup_( const void *p, const char *file, int line, const char *func )
{
	void *ret;
	
	// printf("%s:%d: in func %s: strdup %p\n",file,line,func,p);
#ifdef MEMWATCH
	ret=mwStrdup(p,file,line);
#else
	ret=strdup(p);
#endif
	if(ret==NULL){
		printf("%s:%d: in func %s: strdup error out of memory!\n",file,line,func);
		exit(1);

	}
	return ret;
}

void aFree_( void *p, const char *file, int line, const char *func )
{
	// printf("%s:%d: in func %s: free %p\n",file,line,func,p);
#ifdef MEMWATCH
	mwFree(p,file,line);
#else
	free(p);
#endif
}

int do_init_memmgr(const char* file) {
	return 0;
}

double memmgr_usage(void) {
	return 0;
}

#else /* USE_MEMMGR */

/*
 * �������}�l�[�W��
 *     malloc , free �̏����������I�ɏo����悤�ɂ������́B
 *     ���G�ȏ������s���Ă���̂ŁA�኱�d���Ȃ邩������܂���B
 *
 * �f�[�^�\���Ȃǁi��������ł����܂���^^; �j
 *     �E�������𕡐��́u�u���b�N�v�ɕ����āA����Ƀu���b�N�𕡐��́u���j�b�g�v
 *       �ɕ����Ă��܂��B���j�b�g�̃T�C�Y�́A�P�u���b�N�̗e�ʂ𕡐��ɋϓ��z��
 *       �������̂ł��B���Ƃ��΁A�P���j�b�g32KB�̏ꍇ�A�u���b�N�P��32Byte�̃�
 *       �j�b�g���A1024�W�܂��ďo���Ă�����A64Byte�̃��j�b�g�� 512�W�܂���
 *       �o���Ă����肵�܂��B�ipadding,unit_head �������j
 *
 *     �E�u���b�N���m�̓����N���X�g(block_prev,block_next) �łȂ���A�����T�C
 *       �Y�����u���b�N���m�������N���X�g(samesize_prev,samesize_nect) �ł�
 *       �����Ă��܂��B����ɂ��A�s�v�ƂȂ����������̍ė��p�������I�ɍs���܂��B
 */

/* �u���b�N�ɓ���f�[�^�� */
#define BLOCK_DATA_SIZE	80*1024

/* ��x�Ɋm�ۂ���u���b�N�̐��B */
#define BLOCK_ALLOC		32

/* �u���b�N�̃A���C�����g */
#define BLOCK_ALIGNMENT	64

/* �u���b�N */
struct block {
	int    block_no;		/* �u���b�N�ԍ� */
	struct block* block_prev;		/* �O�Ɋm�ۂ����̈� */
	struct block* block_next;		/* ���Ɋm�ۂ����̈� */
	int    samesize_no;     /* �����T�C�Y�̔ԍ� */
	struct block* samesize_prev;	/* �����T�C�Y�̑O�̗̈� */
	struct block* samesize_next;	/* �����T�C�Y�̎��̗̈� */
	size_t unit_size;		/* ���j�b�g�̃o�C�g�� 0=���g�p */
	size_t unit_hash;		/* ���j�b�g�̃n�b�V�� */
	int    unit_count;		/* ���j�b�g�̐� */
	int    unit_used;		/* �g�p�ς݃��j�b�g */
	char   data[BLOCK_DATA_SIZE];
};

struct unit_head {
	struct block* block;
	size_t        size;
	const  char*  file;
	int           line;
	unsigned int  checksum;
};

static struct block* block_first  = NULL;
static struct block* block_last   = NULL;
static struct block* block_unused = NULL;

/* ���j�b�g�ւ̃n�b�V���B80KB/64Byte = 1280�� */
static struct block* unit_first[BLOCK_DATA_SIZE/BLOCK_ALIGNMENT];		/* �ŏ� */
static struct block* unit_unfill[BLOCK_DATA_SIZE/BLOCK_ALIGNMENT];	/* ���܂��ĂȂ� */
static struct block* unit_last[BLOCK_DATA_SIZE/BLOCK_ALIGNMENT];		/* �Ō� */

/* ���������g���񂹂Ȃ��̈�p�̃f�[�^ */
struct unit_head_large {
	struct unit_head_large* prev;
	struct unit_head_large* next;
	struct unit_head        unit_head;
};

static struct unit_head_large *unit_head_large_first = NULL;

static struct block* block_malloc(void);
static void   block_free(struct block* p);
static void   memmgr_info(void);
static void   memmgr_warning(const char* format,...);
static unsigned int memmgr_usage_bytes;

void* aMalloc_(size_t size, const char *file, int line, const char *func) {
	int i;
	struct block *block;
	size_t size_hash = (size+BLOCK_ALIGNMENT-1) / BLOCK_ALIGNMENT;

	if(size == 0) {
		return NULL;
	}
	memmgr_usage_bytes += size;

	/* �u���b�N���𒴂���̈�̊m�ۂɂ́Amalloc() ��p���� */
	/* ���̍ہAunit_head.block �� NULL �������ċ�ʂ��� */
	if(size_hash * BLOCK_ALIGNMENT > BLOCK_DATA_SIZE - sizeof(struct unit_head)) {
#ifdef MEMWATCH
		struct unit_head_large* p = (struct unit_head_large*)mwMalloc(sizeof(struct unit_head_large) + size,file,line);
#else
		struct unit_head_large* p = (struct unit_head_large*)malloc(sizeof(struct unit_head_large) + size);
#endif
		if(p != NULL) {
			p->unit_head.block = NULL;
			p->unit_head.size  = size;
			p->unit_head.file  = file;
			p->unit_head.line  = line;
			if(unit_head_large_first == NULL) {
				unit_head_large_first = p;
				p->next = NULL;
				p->prev = NULL;
			} else {
				unit_head_large_first->prev = p;
				p->prev = NULL;
				p->next = unit_head_large_first;
				unit_head_large_first = p;
			}
			*(int*)((char*)p + sizeof(struct unit_head_large) - sizeof(int) + size) = 0xdeadbeaf;
			return (char *)p + sizeof(struct unit_head_large) - sizeof(int);
		} else {
			printf("MEMMGR::memmgr_alloc failed.\n");
			exit(1);
		}
	}

	/* ����T�C�Y�̃u���b�N���m�ۂ���Ă��Ȃ����A�V���Ɋm�ۂ��� */
	if(unit_unfill[size_hash] == NULL) {
		block = block_malloc();
		if(unit_first[size_hash] == NULL) {
			/* ����m�� */
			unit_first[size_hash] = block;
			unit_last[size_hash] = block;
			block->samesize_no = 0;
			block->samesize_prev = NULL;
			block->samesize_next = NULL;
		} else {
			/* �A����� */
			unit_last[size_hash]->samesize_next = block;
			block->samesize_no   = unit_last[size_hash]->samesize_no + 1;
			block->samesize_prev = unit_last[size_hash];
			block->samesize_next = NULL;
			unit_last[size_hash] = block;
		}
		unit_unfill[size_hash] = block;
		block->unit_size  = size_hash * BLOCK_ALIGNMENT + sizeof(struct unit_head);
		block->unit_count = (int)(BLOCK_DATA_SIZE / block->unit_size);
		block->unit_used  = 0;
		block->unit_hash  = size_hash;
		/* ���g�pFlag�𗧂Ă� */
		for(i=0;i<block->unit_count;i++) {
			((struct unit_head*)(&block->data[block->unit_size * i]))->block = NULL;
		}
	}
	/* ���j�b�g�g�p�����Z */
	block = unit_unfill[size_hash];
	block->unit_used++;

	/* ���j�b�g����S�Ďg���ʂ����� */
	if(block->unit_count == block->unit_used) {
		do {
			unit_unfill[size_hash] = unit_unfill[size_hash]->samesize_next;
		} while(
			unit_unfill[size_hash] != NULL &&
			unit_unfill[size_hash]->unit_count == unit_unfill[size_hash]->unit_used
		);
	}

	/* �u���b�N�̒��̋󂫃��j�b�g�{�� */
	for(i=0;i<block->unit_count;i++) {
		struct unit_head *head = (struct unit_head*)(&block->data[block->unit_size * i]);
		if(head->block == NULL) {
			head->block = block;
			head->size  = size;
			head->line  = line;
			head->file  = file;
			*(int*)((char*)head + sizeof(struct unit_head) - sizeof(int) + size) = 0xdeadbeaf;
			return (char *)head + sizeof(struct unit_head) - sizeof(int);
		}
	}
	// �����ɗ��Ă͂����Ȃ��B
	printf("MEMMGR::memmgr_malloc() serious error.\n");
	memmgr_info();
	exit(1);
	return NULL;
};

void* aCalloc_(size_t num, size_t size, const char *file, int line, const char *func ) {
	void *p = aMalloc_(num * size,file,line,func);
	memset(p,0,num * size);
	return p;
}

void* aRealloc_(void *memblock, size_t size, const char *file, int line, const char *func ) {
	size_t old_size;
	if(memblock == NULL) {
		return aMalloc_(size,file,line,func);
	}

	old_size = ((struct unit_head *)((char *)memblock - sizeof(struct unit_head) + sizeof(int)))->size;
	if(old_size > size) {
		// �T�C�Y�k�� -> ���̂܂ܕԂ��i�蔲���j
		return memblock;
	}  else {
		// �T�C�Y�g��
		void *p = aMalloc_(size,file,line,func);
		if(p != NULL) {
			memcpy(p,memblock,old_size);
		}
		aFree_(memblock,file,line,func);
		return p;
	}
}

void* aStrdup_(const void* string, const char *file, int line, const char *func ) {
	if(string == NULL) {
		return NULL;
	} else {
		size_t  len = strlen(string);
		char    *p  = (char *)aMalloc_(len + 1,file,line,func);
		memcpy(p,string,len+1);
		return p;
	}
}

void aFree_(void *ptr, const char *file, int line, const char *func ) {
	struct unit_head *head;
	size_t size_hash;

	if(ptr == NULL) return; 

	head      = (struct unit_head *)((char *)ptr - sizeof(struct unit_head) + sizeof(int));
	size_hash = (head->size+BLOCK_ALIGNMENT-1) / BLOCK_ALIGNMENT;

	if(head->block == NULL) {
		if(size_hash * BLOCK_ALIGNMENT > BLOCK_DATA_SIZE - sizeof(struct unit_head)) {
			/* malloc() �Œ��Ɋm�ۂ��ꂽ�̈� */
			struct unit_head_large *head_large = (struct unit_head_large *)((char *)ptr - sizeof(struct unit_head_large) + sizeof(int));
			if(
				*(int*)((char*)head_large + sizeof(struct unit_head_large) - sizeof(int) + head->size)
				!= 0xdeadbeaf)
			{
				memmgr_warning("memmgr: args of aFree is overflowed pointer %s line %d\n",file,line);
			}
			if(head_large->prev) {
				head_large->prev->next = head_large->next;
			} else {
				unit_head_large_first  = head_large->next;
			}
			if(head_large->next) {
				head_large->next->prev = head_large->prev;
			}
			memmgr_usage_bytes -= head->size;
			head->block = NULL;
			free(head_large);
			return;
		} else {
			memmgr_warning("memmgr: args of aFree is freed pointer %s line %d\n",file,line);
		}
	} else {
		/* ���j�b�g��� */
		struct block *block = head->block;
		if((unsigned long)block % sizeof(struct block) != 0) {
			memmgr_warning("memmgr: args of aFree is not valid pointer %s line %d\n",file,line);
		} else if(*(int*)((char*)head + sizeof(struct unit_head) - sizeof(int) + head->size) != 0xdeadbeaf) {
			memmgr_warning("memmgr: args of aFree is overflowed pointer %s line %d\n",file,line);
		} else {
			head->block = NULL;
			memmgr_usage_bytes -= head->size;
			if(--block->unit_used == 0) {
				/* �u���b�N�̉�� */
				if(unit_unfill[block->unit_hash] == block) {
					/* �󂫃��j�b�g�Ɏw�肳��Ă��� */
					do {
						unit_unfill[block->unit_hash] = unit_unfill[block->unit_hash]->samesize_next;
					} while(
						unit_unfill[block->unit_hash] != NULL &&
						unit_unfill[block->unit_hash]->unit_count == unit_unfill[block->unit_hash]->unit_used
					);
				}
				if(block->samesize_prev == NULL && block->samesize_next == NULL) {
					/* �Ɨ��u���b�N�̉�� */
					unit_first[block->unit_hash]  = NULL;
					unit_last[block->unit_hash]   = NULL;
					unit_unfill[block->unit_hash] = NULL;
				} else if(block->samesize_prev == NULL) {
					/* �擪�u���b�N�̉�� */
					unit_first[block->unit_hash] = block->samesize_next;
					(block->samesize_next)->samesize_prev = NULL;
				} else if(block->samesize_next == NULL) {
					/* ���[�u���b�N�̉�� */
					unit_last[block->unit_hash] = block->samesize_prev; 
					(block->samesize_prev)->samesize_next = NULL;
				} else {
					/* ���ԃu���b�N�̉�� */
					(block->samesize_next)->samesize_prev = block->samesize_prev;
					(block->samesize_prev)->samesize_next = block->samesize_next;
				}
				block_free(block);
			} else {
				/* �󂫃��j�b�g�̍Đݒ� */
				if(
					unit_unfill[block->unit_hash] == NULL ||
					unit_unfill[block->unit_hash]->samesize_no > block->samesize_no
				) {
					unit_unfill[block->unit_hash] = block;
				}
			}
		}
	}
}

/* ���݂̏󋵂�\������ */
static void memmgr_info(void) {
	int i;
	struct block *p;
	printf("** Memory Maneger Information **\n");
	if(block_first == NULL) {
		printf("Uninitialized.\n");
		return;
	}
	printf(
		"Blocks: %04u , BlockSize: %06u Byte , Used: %08uKB\n",
		block_last->block_no+1,sizeof(struct block),
		(block_last->block_no+1) * sizeof(struct block) / 1024
	);
	p = block_first;
	for(i=0;i<=block_last->block_no;i++) {
		printf("    Block #%04u : ",p->block_no);
		if(p->unit_size == 0) {
			printf("unused.\n");
		} else {
			printf(
				"size: %05u byte. used: %04u/%04u prev:",
				p->unit_size - sizeof(struct unit_head),p->unit_used,p->unit_count
			);
			if(p->samesize_prev == NULL) {
				printf("NULL");
			} else {
				printf("%04u",(p->samesize_prev)->block_no);
			}
			printf(" next:");
			if(p->samesize_next == NULL) {
				printf("NULL");
			} else {
				printf("%04u",(p->samesize_next)->block_no);
			}
			printf("\n");
		}
		p = p->block_next;
	}
}

/* �u���b�N���m�ۂ��� */
static struct block* block_malloc(void) {
	if(block_unused != NULL) {
		/* �u���b�N�p�̗̈�͊m�ۍς� */
		struct block* ret = block_unused;
		do {
			block_unused = block_unused->block_next;
		} while(block_unused != NULL && block_unused->unit_size != 0);
		return ret;
	} else {
		/* �u���b�N�p�̗̈��V���Ɋm�ۂ��� */
		int i;
		int  block_no;
		struct block* p;
		char *pb = (char *)calloc(sizeof(struct block),BLOCK_ALLOC + 1);
		if(pb == NULL) {
			printf("MEMMGR::block_alloc failed.\n");
			exit(1);
		}
		// �u���b�N�̃|�C���^�̐擪��sizeof(block) �A���C�����g�ɑ�����
		// ���̃A�h���X��free() ���邱�Ƃ͂Ȃ��̂ŁA���ڃ|�C���^��ύX���Ă���B
		pb += sizeof(struct block) - ((unsigned long)pb % sizeof(struct block));
		p   = (struct block*)pb;
		if(block_first == NULL) {
			/* ����m�� */
			block_no     = 0;
			block_first  = p;
		} else {
			block_no      = block_last->block_no + 1;
			block_last->block_next = p;
			p->block_prev = block_last;
		}
		block_last = &p[BLOCK_ALLOC - 1];
		/* �u���b�N��A�������� */
		for(i=0;i<BLOCK_ALLOC;i++) {
			if(i != 0) {
				p[i].block_prev = &p[i-1];
			}
			if(i != BLOCK_ALLOC -1) {
				p[i].block_next = &p[i+1];
			}
			p[i].block_no = block_no + i;
		}

		/* ���g�p�u���b�N�ւ̃|�C���^���X�V */
		block_unused = &p[1];
		p->unit_size = 1;
		return p;
	}
}

static void block_free(struct block* p) {
	/* free() �����ɁA���g�p�t���O��t���邾�� */
	p->unit_size = 0;
	/* ���g�p�|�C���^�[���X�V���� */
	if(block_unused == NULL) {
		block_unused = p;
	} else if(block_unused->block_no > p->block_no) {
		block_unused = p;
	}
}

static char memmer_logfile[128];

static void memmgr_warning(const char* format,...) {
	FILE *fp = fopen(memmer_logfile,"a");
	va_list ap;
	va_start(ap,format);

	if(fp) {
		vfprintf(fp,format,ap);
		fclose(fp);
	}
	vprintf(format,ap);
	va_end(ap);
}

static FILE* memmgr_log(void) {
	FILE *fp = fopen(memmer_logfile,"a");
	if(!fp) { fp = stdout; }
	fprintf(fp,"memmgr: memory leaks found\n");
	return fp;
}

static void memmer_exit(void) {
	FILE *fp = NULL;
	int i;
	int count = 0;
	struct block *block = block_first;
	struct unit_head_large *large = unit_head_large_first;
	while(block) {
		if(block->unit_size) {
			if(!fp) { fp = memmgr_log(); }
			for(i=0;i<block->unit_count;i++) {
				struct unit_head *head = (struct unit_head*)(&block->data[block->unit_size * i]);
				if(head->block != NULL) {
					fprintf(
						fp,"%04d : %s line %d size %d\n",++count,
						head->file,head->line,head->size
					);
				}
			}
		}
		block = block->block_next;
	}
	while(large) {
		if(!fp) { fp = memmgr_log(); }
		fprintf(
			fp,"%04d : %s line %d size %d\n",++count,
			large->unit_head.file,
			large->unit_head.line,large->unit_head.size
		);
		large = large->next;
	}
	if(!fp) {
		printf("memmgr: no memory leaks found.\n");
	} else {
		printf("memmgr: memory leaks found.\n");
	}
}

int do_init_memmgr(const char* file) {
	sprintf(memmer_logfile,"%s.log",file);
	atexit(memmer_exit);
	printf("memmgr: initialised: %s\n",memmer_logfile);
	return 0;
}

double memmgr_usage(void) {
	return memmgr_usage_bytes / 1024.0;
}

#endif
