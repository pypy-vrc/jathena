// original : core.h 2003/03/14 11:55:25 Rev 1.4

#ifndef	_SOCKET_H_
#define _SOCKET_H_

#include <stdio.h>
#include <sys/types.h>
#ifdef _WIN32
	#include <winsock.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
#endif

// �����ŕ���̂ł͂Ȃ��Asocket.c �ŕ���
#define close(id) do{ if(session[id]) session[id]->eof = 1; } while(0);

// define declaration

#define RFIFOSPACE(fd) (session[fd]->max_rdata-session[fd]->rdata_size)
#define RFIFOP(fd,pos) (session[fd]->rdata_pos+(pos))
#define RFIFOB(fd,pos) (*(unsigned char*)RFIFOP(fd,pos))
#define RFIFOW(fd,pos) (*(unsigned short*)RFIFOP(fd,pos))
#define RFIFOL(fd,pos) (*(unsigned int*)RFIFOP(fd,pos))
#define RFIFOREST(fd)  (session[fd]->rdata_size-session[fd]->rdata_pos)
#define RFIFOSKIP(fd,len) (session[fd]->rdata_pos+=(len))

#define WFIFOSPACE(fd) (session[fd]->max_wdata-session[fd]->wdata_size)
#define WFIFOP(fd,pos) (session[fd]->wdata_size+(pos))
#define WFIFOB(fd,pos) (*(unsigned char*)WFIFOP(fd,pos))
#define WFIFOW(fd,pos) (*(unsigned short*)WFIFOP(fd,pos))
#define WFIFOL(fd,pos) (*(unsigned int*)WFIFOP(fd,pos))
#define WFIFOREST(fd)  (session[fd]->wdata_size-session[fd]->wdata_pos)

// use function instead of macro.
//#define WFIFOSET(fd,len) (session[fd]->wdata_size = (session[fd]->wdata_size+(len)+2048 < session[fd]->max_wdata) ? session[fd]->wdata_size+len : session[fd]->wdata_size)

#define RBUFP(p,pos) ((unsigned char*)(p)+(pos))
#define RBUFB(p,pos) (*(unsigned char*)RBUFP(p,pos))
#define RBUFW(p,pos) (*(unsigned short*)RBUFP(p,pos))
#define RBUFL(p,pos) (*(unsigned int*)RBUFP(p,pos))
#define WBUFP(p,pos) (((unsigned char*)(p))+(pos))
#define WBUFB(p,pos) (*(unsigned char*)WBUFP(p,pos))
#define WBUFW(p,pos) (*(unsigned short*)WBUFP(p,pos))
#define WBUFL(p,pos) (*(unsigned int*)WBUFP(p,pos))

#ifdef __INTERIX
#define FD_SETSIZE 4096
#endif	// __INTERIX

// Struct declaration

struct socket_data{
	int eof;
	int auth;
	unsigned int tick;
	unsigned char *rdata,*wdata;
	unsigned char *max_rdata ,*max_wdata;
	unsigned char *rdata_size,*rdata_pos;
	unsigned char *wdata_size,*wdata_pos;
	struct sockaddr_in client_addr;
	int (*func_recv)(int);
	int (*func_send)(int);
	int (*func_parse)(int);
	int (*func_destruct)(int);
	int flag_destruct;	// �f�X�g���N�^���ēx�Ă΂�Ȃ��ׂ̃t���O
	int flag_httpd;		// 0:httpd ����� 1;��͍ς�
	void* session_data;
	void* session_data2;
#ifdef _WIN32
	SOCKET socket;
#endif
};

// Data prototype declaration

extern struct socket_data *session[FD_SETSIZE];

extern int rfifo_size,wfifo_size;
extern int fd_max;

// Function prototype declaration

int make_listen_port(int);
int make_connection(long,int);
int delete_session(int);
int realloc_fifo(int fd,int rfifo_size,int wfifo_size);
int WFIFOSET(int fd,int len);
int WFIFORESERVE(int fd,int len);

int do_sendrecv(int next);
int do_parsepacket(void);
int parsepacket_timer(int tid, unsigned int tick, int id, int data);
void do_socket(void);

void set_defaultparse(int (*defaultparse)(int));
void set_sock_destruct(int (*func_destruct)(int));

void socket_enable_httpd(int);

struct httpd_session_data;
void socket_httpd_page(struct httpd_session_data* sd,const char* url);

void socket_set_httpd_page_connection_func( void (*func)(int fd,char*,char*,char*) );

#endif	// _SOCKET_H_