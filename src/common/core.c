// $Id: core.c,v 1.1 2005/08/29 21:39:30 running_pinata Exp $
// original : core.c 2003/02/26 18:03:12 Rev 1.7

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#	include <windows.h>
#else
#	include <unistd.h>
#	include <signal.h>
#endif

#include "core.h"
#include "socket.h"
#include "timer.h"
#include "mmo.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#include "malloc.h"

int packet_parse_time = 0;

/*======================================
 *	CORE : Signal Sub Function
 *--------------------------------------
 */

static volatile int athena_is_running = 1;

#ifdef _WIN32
BOOL WINAPI core_CtrlHandlerRoutine( DWORD dwCtrlType )
{
	athena_is_running = 0;
	return TRUE;
}
#else
static void sig_proc(int sn)
{
	switch(sn){
	case SIGINT:
	case SIGTERM:
		athena_is_running = 0;
		break;
	}
}
#endif

/*======================================
 *	CORE : MAINROUTINE
 *--------------------------------------
 */

char pid_file[256];

void pid_delete(void) {
	unlink(pid_file);
}

void pid_create(const char* file) {
	FILE *fp;
	int len = strlen(file);
	strcpy(pid_file,file);
	if(len > 4 && pid_file[len - 4] == '.') {
		pid_file[len - 4] = 0;
	}
	strcat(pid_file,".pid");
	fp = fopen(pid_file,"w");
	if(fp) {
#ifdef _WIN32
		fprintf(fp,"%d",GetCurrentProcessId());
#else
		fprintf(fp,"%d",getpid());
#endif
		fclose(fp);
		atexit(pid_delete);
	}
}

double uptime(void) {
	static time_t boot = 0;
	time_t now;
	if(boot == 0) {
		time(&boot);
	}
	time(&now);
	return (now - boot) / 86400.0;
}

int main(int argc,char **argv)
{
	int next;
	pid_create(argv[0]);
	do_init_memmgr(argv[0]); // 一番最初に実行する必要がある
	do_socket();
#ifdef _WIN32
	SetConsoleCtrlHandler( core_CtrlHandlerRoutine, TRUE );
#else
	signal(SIGPIPE,SIG_IGN);
	signal(SIGTERM,sig_proc);
	signal(SIGINT,sig_proc);
#endif
	atexit(do_final);
	do_init(argc,argv);
	if (packet_parse_time > 0) {
		add_timer_func_list(parsepacket_timer,"parsepacket_timer");
		add_timer_interval(gettick()+packet_parse_time,parsepacket_timer,0,0,packet_parse_time);

		while(athena_is_running) {
			next=do_timer(gettick_nocache());
			do_sendrecv(next);
		}
	} else {
		while(athena_is_running) {
			next=do_timer(gettick_nocache());
			do_sendrecv(next);
			do_parsepacket();
		}
	}
	
	exit(0);
	return 0;
}

// mmo.h で定義されている / BCC のときに使用
#if defined(_WIN32) && defined(__BORLANDC__)
int strcasecmp(const char *s1, const char *s2) {
	while(*s1 && *s2) {
		if((*s1 | 0x20) != (*s2 | 0x20)) {
			return ((*s1 | 0x20) > (*s2 | 0x20) ? 1 : -1);
		}
		s1++; s2++;
	}
	if(*s1) return 1;
	if(*s2) return -1;
	return 0;
}
#endif

// mmo.h で定義されている
// 32bit 線形合同法( 戻り値は 24 ビット有効 )
#ifdef RANDOM32
static int seed32 = 1;

void atn_int24_srandom32( int seed )
{
	seed32 = seed;
}
int atn_int24_random32(void)
{
	seed32 = seed32 * 1103515245 + 12345;
	return ( seed32 >> 8 )&0x00ffffff;
}
#endif	// ifdef RANDOM32

// mmo.h で定義されている
// 64bit 線形合同法( 戻り値は 31 ビット有効 )
#ifdef RANDOM64
static atn_int64 seed64 = 0x1234ABCD330E;

void atn_int31_srandom64( int seed )
{
	seed64 = seed;
	seed64 = (seed64<<16)+0x330E;
}
int atn_int31_random64(void)
{
	seed64 = seed64 * 0x5DEECE66D + 0xB;
	return (int)( seed64>>17 )&0x7FFFFFFF;
}
#endif	// ifdef RANDOM64


// mmo.h で定義されている
// メルセンヌツイスター
// http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/mt.html
#ifdef RANDOMMT

/* Period parameters */  
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */

static unsigned long mt[N]; /* the array for the state vector  */
static int mti=N+1; /* mti==N+1 means mt[N] is not initialized */

/* initializes mt[N] with a seed */
void atn_srandommt(unsigned long s)
{
    mt[0]= s & 0xffffffffUL;
    for (mti=1; mti<N; mti++) {
        mt[mti] = 
	    (1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti); 
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array mt[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        mt[mti] &= 0xffffffffUL;
        /* for >32 bit machines */
    }
}

/* generates a random number on [0,0xffffffff]-interval */
static unsigned long atn_int32_randommt(void)
{
    unsigned long y;
    static unsigned long mag01[2]={0x0UL, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    if (mti >= N) { /* generate N words at one time */
        int kk;

        if (mti == N+1)   /* if init_genrand() has not been called, */
            atn_srandommt(5489UL); /* a default initial seed is used */

        for (kk=0;kk<N-M;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        for (;kk<N-1;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
        mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

        mti = 0;
    }
  
    y = mt[mti++];

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}

long atn_int31_randommt(void)
{
    return (long)(atn_int32_randommt()>>1);
}

#endif	// ifdef RANDOMMT

