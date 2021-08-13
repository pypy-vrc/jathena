// $Id: socket.c,v 1.2 2004/09/15 00:17:17 running_pinata Exp $
// original : core.c 2003/02/26 18:03:12 Rev 1.7

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#ifdef _WIN32
	#include <winsock.h>
	#pragma comment(lib,"ws2_32.lib")
#else
	#include <sys/socket.h>
	#include <sys/select.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <unistd.h>
	#include <sys/time.h>
#endif
#include <fcntl.h>
#include <string.h>

#include "mmo.h"
#include "httpd.h"
#include "timer.h"
#include "socket.h"
#include "malloc.h"

// socket.h でdefine されたcloseを置き換え
#ifdef _WIN32
	#undef close
	#define close(id) do{ if(session[id]) closesocket(session[id]->socket); } while(0);
	#define sock(fd)  (session[fd]->socket)
	int SessionInsertSocket(const SOCKET elem,int pos2);
	int SessionRemoveSocket(const SOCKET elem);
#else
	#undef close
	#define sock(fd)  (fd)
#endif

#ifdef MEMWATCH
#include "memwatch.h"
#endif

fd_set readfds;
int fd_max;

int rfifo_size = 65536;
int wfifo_size = 65536;

struct socket_data *session[FD_SETSIZE];
static int null_parse(int fd);
static int (*default_func_parse)(int) = null_parse;
static int (*default_func_destruct)(int) = NULL;

static int httpd_enable = 1;
void socket_enable_httpd( int flag ){ if( flag>=0 ) httpd_enable = flag; }

static int socket_ctrl_panel_httpd = 1;
void socket_enable_ctrl_panel_httpd( int flag ){ socket_ctrl_panel_httpd = flag; }
static char socket_ctrl_panel_url[256]="/socketctrl";
const char* get_socket_ctrl_panel_url(void) { return socket_ctrl_panel_url; }

static int unauth_timeout = 10*1000;
static int auth_timeout = 10*60*1000;

static int send_limit_buffer_size = 128*1024;

static int recv_limit_rate_enable		= 1;
static int recv_limit_rate_period		= 500;
static int recv_limit_rate_bytes		= 1024;
static int recv_limit_rate_wait_max		= 2000;
static int recv_limit_rate_disconnect	= 5000;

static int connect_check(unsigned int ip);
/*======================================
 *	CORE : Set function
 *--------------------------------------
 */
void set_defaultparse(int (*defaultparse)(int))
{
	default_func_parse = defaultparse;
}

void set_sock_destruct(int (*func_destruct)(int))
{
	default_func_destruct = func_destruct;
}

/*======================================
 *	CORE : Socket Sub Function
 *--------------------------------------
 */

static int recv_to_fifo(int fd)
{
	int len;
	unsigned int tick = gettick();

	//printf("recv_to_fifo : %d %d\n",fd,session[fd]->eof);

	if(session[fd]->eof || 
		( recv_limit_rate_enable && session[fd]->auth >=0 && DIFF_TICK( session[fd]->rlr_tick, tick )>0 ) )	// 帯域制限中
	{
		return -1;
	}

	len=recv(sock(fd),session[fd]->rdata_size,RFIFOSPACE(fd),0);
	//{ int i; printf("recv %d : ",fd); for(i=0;i<len;i++){ printf("%02x ",session[fd]->rdata_size[i]); } printf("\n");}
	if(len>0){
		session[fd]->rdata_size += len;
		
//		printf("rs: %d %d\n",len, session[fd]->auth );
		// 帯域制限用の計算
		if( session[fd]->auth>=0 )
		{
			struct socket_data* sd = session[fd];
			int tick_diff = DIFF_TICK( tick, sd->rlr_tick );
			sd->rlr_bytes += len;

			// 帯域の制限
			if( tick_diff >= recv_limit_rate_period )
			{
				int rate = sd->rlr_bytes * 1000 / tick_diff;
//				printf("rlr: %d %d\n", sd->rlr_bytes, rate );
				if( rate > recv_limit_rate_bytes )
				{
					int wait = ( rate - recv_limit_rate_bytes ) * tick_diff / recv_limit_rate_bytes;
					if( wait > recv_limit_rate_wait_max )
						wait = recv_limit_rate_wait_max;
					sd->rlr_tick += wait;
					if( (sd->rlr_disc += wait) > recv_limit_rate_disconnect )
					{
						sd->eof = 1;
						return -1;
					}
//					printf("rlr: on! %d %d tick wait\a\n", ( rate - recv_limit_rate_bytes ) * tick_diff / recv_limit_rate_bytes, wait );
				}
				else
				{
					sd->rlr_tick = tick;
					sd->rlr_disc = 0;
				}

				sd->rlr_bytes = 0;
			}
		}

#ifdef _WIN32
	} else if(len == 0 || len == SOCKET_ERROR){
		// printf("set eof :%d\n",fd);
		session[fd]->eof=1;
#else
	} else if(len<=0){
		// printf("set eof :%d\n",fd);
		session[fd]->eof=1;
#endif
	}
	return 0;
}

static int send_from_fifo(int fd)
{
	int len;
	struct socket_data *sd = session[fd];

	//printf("send_from_fifo : %d\n",fd);
	if(sd->eof || WFIFOREST(fd) == 0)
		return -1;
	len=send(sock(fd),sd->wdata_pos,WFIFOREST(fd),0);
	//{ int i; printf("send %d : ",fd);  for(i=0;i<len;i++){ printf("%02x ",session[fd]->wdata_pos[i]); } printf("\n");}
	if(len>0){
		sd->wdata_pos += len;
		if(sd->wdata_pos == sd->wdata_size) {
			sd->wdata_size = sd->wdata;
			sd->wdata_pos  = sd->wdata;
		} else if((sd->wdata_pos - sd->wdata) * 8 > (sd->max_wdata - sd->wdata)) {
			// クリアする間隔を減らしてみる
			memmove(sd->wdata,sd->wdata_pos,WFIFOREST(fd));
			sd->wdata_size = sd->wdata + WFIFOREST(fd);
			sd->wdata_pos  = sd->wdata;
		}
#ifdef _WIN32
	} else if(len == 0 || len == SOCKET_ERROR) {
		// printf("set eof :%d\n",fd);
		sd->eof=1;
#else
	} else {
		// printf("set eof :%d\n",fd);
		sd->eof=1;
#endif
	}
	return 0;
}

static int null_parse(int fd)
{
	printf("null_parse : %d\n",fd);
	RFIFOSKIP(fd,RFIFOREST(fd));
	return 0;
}

/*======================================
 *	CORE : Socket Function
 *--------------------------------------
 */

static int connect_client(int listen_fd)
{
	int fd;
	struct sockaddr_in client_address;
	int len;
	unsigned long result;
	int yes = 1; // reuse fix
#ifdef _WIN32
	SOCKET socket;
#endif
	//printf("connect_client : %d\n",listen_fd);

	len=sizeof(client_address);

#ifdef _WIN32
	result = 1;
	fd=accept(session[listen_fd]->socket,(struct sockaddr*)&client_address,&len);
	ioctlsocket(fd,FIONBIO,&result);
#else
	fd=accept(listen_fd,(struct sockaddr*)&client_address,&len);
	result = fcntl(fd,F_SETFL,O_NONBLOCK);
#endif

//	setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,NULL,0);
	setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(char *)&yes,sizeof yes); // reuse fix
#ifdef SO_REUSEPORT
//	setsockopt(fd,SOL_SOCKET,SO_REUSEPORT,NULL,0);
	setsockopt(fd,SOL_SOCKET,SO_REUSEPORT,(char *)&yes,sizeof yes); //reuse fix
#endif
//	setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,NULL,0);
	setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,(char *)&yes,sizeof yes); // reuse fix

	if(fd==-1){
		printf("accept");
		return -1;
	} else if(!connect_check(*(unsigned int*)(&client_address.sin_addr))) {
#ifdef _WIN32
		closesocket(fd);
#else
		close(fd);
#endif
		return -1;
	} else {
		FD_SET(fd,&readfds);
	}
#ifdef _WIN32
	result = 1;
	ioctlsocket(fd,FIONBIO,&result);

	socket = fd;
	fd=2;
	while(session[fd] != NULL && fd<fd_max) {
		fd++;
	}
	SessionInsertSocket(socket,fd);
#else
	result = fcntl(fd, F_SETFL, O_NONBLOCK);
#endif

	session[fd] = (struct socket_data *)aCalloc(1,sizeof(*session[fd]));
	session[fd]->func_recv   = recv_to_fifo;
	session[fd]->func_send   = send_from_fifo;
	session[fd]->func_parse  = default_func_parse;
	session[fd]->client_addr = client_address;
#if _WIN32
	session[fd]->socket      = socket;
#endif
	session[fd]->tick = gettick();
	session[fd]->auth = 0;
	session[fd]->rlr_tick = gettick();
	session[fd]->rlr_bytes= 0;
	session[fd]->rlr_disc = 0;
	session[fd]->server_port = session[listen_fd]->server_port;
	
	session[fd]->func_destruct = default_func_destruct;
	realloc_fifo(fd,rfifo_size,wfifo_size);

	if(fd_max<=fd) fd_max=fd+1;

  //printf("new_session : %d %d\n",fd,session[fd]->eof);
  return fd;
}

int make_listen_port(int port,unsigned long sip)
{
	struct sockaddr_in server_address;
	int fd;
	unsigned long result;
	int yes = 1; // reuse fix
#ifdef _WIN32
	SOCKET sock;
#endif

	fd = socket( AF_INET, SOCK_STREAM, 0 );
#ifdef _WIN32
	result = 1;
	ioctlsocket(fd,FIONBIO,&result);
#else
	result = fcntl(fd,F_SETFL,O_NONBLOCK);
#endif

#ifndef _WIN32
//	setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,NULL,0);
	setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(char *)&yes,sizeof yes); // reuse fix
#ifdef SO_REUSEPORT
//	setsockopt(fd,SOL_SOCKET,SO_REUSEPORT,NULL,0);
	setsockopt(fd,SOL_SOCKET,SO_REUSEPORT,(char *)&yes,sizeof yes); //reuse fix
#endif
//	setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,NULL,0);
	setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,(char *)&yes,sizeof yes); // reuse fix
#endif

	server_address.sin_family      = AF_INET;
	server_address.sin_addr.s_addr = sip;		// 1710:INADDR_ANYから変更
	server_address.sin_port        = htons((unsigned short)port);

	result = bind(fd, (struct sockaddr*)&server_address, sizeof(server_address));
	if( result == -1 ) {
		perror("bind");
		exit(1);
	}
	result = listen( fd, 5 );
	if( result == -1 ) { /* error */
		perror("listen");
		exit(1);
	}

	FD_SET(fd, &readfds );

#ifdef _WIN32
	sock = fd;
	fd=2;
	while(session[fd] != NULL && fd<fd_max) {
		fd++;
	}
	SessionInsertSocket(sock,fd);
#endif

	session[fd] = (struct socket_data *)aCalloc(1,sizeof(*session[fd]));
	session[fd]->func_recv = connect_client;
	session[fd]->auth = -1;
	session[fd]->server_port = port;
#ifdef _WIN32
	session[fd]->socket = sock;
#endif
	if(fd_max<=fd) fd_max=fd+1;

	return fd;
}


int make_connection(long ip,int port)
{
	struct sockaddr_in server_address;
	int fd;
	unsigned long result;
	int yes = 1; // reuse fix
#ifdef _WIN32
	SOCKET sock;
#endif

	fd = socket( AF_INET, SOCK_STREAM, 0 );

	//	setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,NULL,0);
	setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,(char *)&yes,sizeof yes); // reuse fix
#ifdef SO_REUSEPORT
//	setsockopt(fd,SOL_SOCKET,SO_REUSEPORT,NULL,0);
	setsockopt(fd,SOL_SOCKET,SO_REUSEPORT,(char *)&yes,sizeof yes); //reuse fix
#endif
//	setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,NULL,0);
	setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,(char *)&yes,sizeof yes); // reuse fix

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = ip;
	server_address.sin_port = htons((unsigned short)port);

	result = connect(fd, (struct sockaddr *)(&server_address),sizeof(struct sockaddr_in));
	if(result != 0) {
		// 接続失敗
		printf("make_connection : connection failed. %08x:%d\n",(int)ip,port);
		close(fd);
		return 0;
	}

	// connect の前にノンブロックモードを設定すると、
	// connect の結果が分かる前に処理が帰ってくる
#ifdef _WIN32
	result = 1;
	ioctlsocket(fd,FIONBIO,&result);
#else
	result = fcntl(fd, F_SETFL, O_NONBLOCK);
#endif
	
	FD_SET(fd,&readfds);

#ifdef _WIN32
	sock = fd;
	fd=2;
	while(session[fd] != NULL && fd<fd_max) {
		fd++;
	}
	SessionInsertSocket(sock,fd);
#endif

	session[fd] = (struct socket_data *)aCalloc(1,sizeof(*session[fd]));
	session[fd]->func_recv  = recv_to_fifo;
	session[fd]->func_send  = send_from_fifo;
	session[fd]->func_parse = default_func_parse;
#if _WIN32
	session[fd]->socket = sock;
#endif
	session[fd]->func_destruct = default_func_destruct;
	session[fd]->tick = gettick();
	session[fd]->auth = 0;
	session[fd]->rlr_tick = gettick();
	session[fd]->rlr_bytes= 0;
	session[fd]->rlr_disc = 0;
	realloc_fifo(fd,rfifo_size,wfifo_size);
	if(fd_max<=fd) fd_max=fd+1;

	return fd;
}

int delete_session(int fd)
{
	if(fd<=0 || fd>=FD_SETSIZE)
		return -1;
	if(session[fd]){
		// ２重呼び出しの防止
		if(session[fd]->flag_destruct) {
			return 0;
		}
		session[fd]->flag_destruct = 1;
		// デストラクタを呼び出す
		if(session[fd]->func_destruct) {
			session[fd]->func_destruct(fd);
		}
		close(fd);
		FD_CLR(sock(fd),&readfds);
		#ifdef _WIN32
			SessionRemoveSocket(sock(fd));
		#endif
		if(session[fd]->rdata)
			free(session[fd]->rdata);
		if(session[fd]->wdata)
			free(session[fd]->wdata);
		if(session[fd]->session_data)
			free(session[fd]->session_data);
		if(session[fd]->session_data2)
			free(session[fd]->session_data2);
		free(session[fd]);
	}
	session[fd]=NULL;
	//printf("delete_session:%d\n",fd);
	return 0;
}

int realloc_fifo(int fd,int rfifo_size,int wfifo_size)
{
	struct socket_data *s = session[fd];
	if( fd < 0) return 0;
	if( s->max_rdata - s->rdata < rfifo_size) {
		unsigned char * p = s->rdata;
		s->rdata      = (unsigned char *)aRealloc(s->rdata, rfifo_size);
		s->rdata_pos  = s->rdata + (s->rdata_pos  - p);
		s->rdata_size = s->rdata + (s->rdata_size - p);
		s->max_rdata  = s->rdata + rfifo_size;
	}
	if( s->max_wdata  - s->wdata < wfifo_size) {
		unsigned char * p = s->wdata;
		s->wdata      = (unsigned char *)aRealloc(s->wdata, wfifo_size);
		s->wdata_pos  = s->wdata + (s->wdata_pos  - p);
		s->wdata_size = s->wdata + (s->wdata_size - p);
		s->max_wdata  = s->wdata + wfifo_size;
	}
	return 0;
}

int WFIFORESERVE(int fd,int len)
{
	struct socket_data *s = session[fd];
	while( len+16384 > (s->max_wdata - s->wdata) )
	{
		int new_size = (s->max_wdata - s->wdata) <<1;

		// 送信バッファの制限サイズ超過チェック
		if( s->auth >= 0 && new_size > send_limit_buffer_size )
		{
			printf("socket: %d wdata (%d) exceed limited size.\n", fd, new_size );
			s->wdata_pos = s->wdata_size = s->wdata;	// データを消してとりあえず空きを作る
			// 空きスペースが足りないかもしれないので、再確保
			realloc_fifo(fd, 0, len);
			s->eof = 1;
			return 0;
		}
	
		realloc_fifo(fd,s->max_rdata - s->rdata,new_size );
		printf("socket: %d wdata expanded to %d bytes.\n",fd,s->max_wdata - s->wdata);
	}
	return 0;
}

int WFIFOSET(int fd,int len)
{
	struct socket_data *s = session[fd];
	if(fd <= 0 || s->eof ) return 0; 
	if( s->wdata_size + len <= s->max_wdata ) {
		s->wdata_size += len;
	} else {
		printf("socket: %d wdata lost !!\n",fd);
		s->wdata_pos = s->wdata_size = s->wdata;
		s->eof = 1;
		return 0;	// アクセス違反してるはずなのでサーバー落としたほうがいいかも？
	}
	WFIFORESERVE( fd, s->wdata_size - s->wdata );
	return 0;
}

#ifdef _WIN32

// the windows fd_set structures are simple
// typedef struct fd_set {
//        u_int fd_count;               /* how many are SET? */
//        SOCKET  fd_array[FD_SETSIZE];   /* an array of SOCKETs */
// } fd_set;
//
// the select sets the correct fd_count and the array
// so just access the signaled sockets one by one

// this could be realized also with an additional element in the session data
// or an array with "struct {socket_data*, SOCKET}[]"
// anyway it is working that way better because of less changes
// otherwise it might be necessary to change access from extern structs
// and it might be a bit slower to access then

struct {
	SOCKET sock;	// array of corrosponding sockets
	int    pos;
} sessionsockets[FD_SETSIZE];
int sessioncount;

///////////////////////////////////////////////////////////////////////////////
// binary search implementation
// might be not that efficient in this implementation
// it first checks the boundaries so calls outside
// the list range are handles much faster
// at the expence of some extra code
// runtime behaviour much faster if often called for outside data
///////////////////////////////////////////////////////////////////////////////
int SessionFindSocket(const SOCKET elem, size_t *retpos)
{	// do a binary search with smallest first
	// make some initial stuff
	int ret = 0;
	size_t a=0, b=sessioncount-1, c;
	size_t pos = 0;

	// just to be sure we have to do something
	if( sessioncount==0 )
	{	ret = 0;
	}
	else if( elem < sessionsockets[a].sock )
	{	// less than lower
		pos = a;
		ret = 0;
	}
	else if( elem > sessionsockets[b].sock )
	{	// larger than upper
		pos = b + 1;
		ret = 0;
	}
	else if( elem == sessionsockets[a].sock )
	{	// found at first position
		pos = a;
		ret = 1;
	}
	else if( elem == sessionsockets[b].sock )
	{	// found at last position
		pos = b;
		ret = 1;
	}
	else
	{	// binary search
		// search between first and last
		do
		{
			c=(a+b)/2;
			if( elem == sessionsockets[c].sock )
			{	// found it
				b=c;
				ret = 1;
				break;
			}
			else if( elem < sessionsockets[c].sock )
				b=c;
			else
				a=c;
		}while( (a+1) < b );
		pos = b;
		// return the next larger element to the given
		// or the found element so we can insert a new element there
	}
	// just to make sure we call this with a valid pointer,
	// on c++ it would be a reference and you could omitt the NULL check
	if(retpos) *retpos = pos;
	return ret;
}

int SessionInsertSocket(const SOCKET elem,int pos2)
{
	size_t pos;
	if(sessioncount<FD_SETSIZE) // max number of allowed sockets
	if( !SessionFindSocket(elem, &pos) )
	{
		if((size_t)sessioncount!=pos)
		{	// shift up one position
			memmove( sessionsockets+pos+1, sessionsockets+pos,(sessioncount-pos)*sizeof(sessionsockets[0]));
		}
		sessionsockets[pos].sock = elem;
		sessionsockets[pos].pos  = pos2;
		sessioncount++;
		return pos;
	}
	// otherwise the socket is already in the list
	return -1;
}

int SessionRemoveSocket(const SOCKET elem)
{
	size_t pos;
	if( SessionFindSocket(elem, &pos) )
	{	// shift down one position
		// and just overwrite the pointers here,
		// so clear the session pointer before calling this
		memmove( sessionsockets+pos, sessionsockets+pos+1,(sessioncount-pos-1)*sizeof(sessionsockets[0]));
		sessioncount--;
		return 1;
	}
	// otherwise the socket is not in the list
	return 0;
}

void process_fdset(fd_set* rfd, fd_set* wfd) {
	size_t i;
	size_t fd;
	for(i=0;i<rfd->fd_count;i++)
	{
		if( SessionFindSocket( rfd->fd_array[i], &fd ) ) {
			fd = sessionsockets[fd].pos;
			if( session[fd] && (session[fd]->func_recv) ) {
				session[fd]->func_recv(fd);
			}
		}
	}
	for(i=0;i<wfd->fd_count;i++)
	{
		if( SessionFindSocket( wfd->fd_array[i], &fd ) ) {
			fd = sessionsockets[fd].pos;
			if( session[fd] && (session[fd]->func_send) ) {
				session[fd]->func_send(fd);
			}
		}
	}
}

#else /* _WIN32 */

// some unix, might work on darwin as well

// unix uses a bit array where the socket number equals the
// position in the array, so finding sockets inside that array
// is not that easy exept the socket is knows before
// so this method here goes through the bit array
// and build the socket number from the position
// where a set bit was found.
// since we can skip 32 sockets all together when none is set
// we can travel quite fast through the array

#ifndef howmany
	#define howmany(x,y) (((x)+((y)-1))/(y)) 
#endif

// Find the log base 2 of an N-bit integer in O(lg(N)) operations
// in this case for 32bit input it would be 11 operations

inline unsigned long socket_log2(unsigned long v)
{
	register unsigned long c = 0;
	if (v & 0xFFFF0000) { v >>= 0x10; c |= 0x10; }
	if (v & 0x0000FF00) { v >>= 0x08; c |= 0x08; }
	if (v & 0x000000F0) { v >>= 0x04; c |= 0x04; }
	if (v & 0x0000000C) { v >>= 0x02; c |= 0x02; }
	if (v & 0x00000002) { v >>= 0x01; c |= 0x01; }
	return c;
}

void process_fdset(fd_set* rfd, fd_set* wfd) {
	unsigned int	sock;
	unsigned long	val;
	unsigned long	bits;
	unsigned long	nfd=0;
	// usually go up to 'howmany(FD_SETSIZE, NFDBITS)'
	unsigned long	max = howmany(fd_max, NFDBITS);
	
	while( nfd <  max )
	{	// while something is set in the ulong at position nfd
		bits = rfd->fds_bits[nfd];
//		val = 0;
		while( bits )
		{	// calc the highest bit with log2
			// and clear it from the field
			// this method is especially fast
			// when only a few bits are set in the field
			// which usually happens on read events
			val = socket_log2( bits );
			bits ^= (1<<val);	
			// build the socket number
			sock = nfd*NFDBITS + val;

			///////////////////////////////////////////////////
			// call the user function
			if( session[sock] && session[sock]->func_recv )
				session[sock]->func_recv(sock);
		}
		// go to next field position
		nfd++;
	}

	// vars are declared above already
	nfd=0;
	while( nfd < max  )
	{	// while something is set in the ulong at position nfd
		bits = wfd->fds_bits[nfd];
//		val = 0;
		while( bits )
		{	// calc the highest bit with log2
			// and clear it from the field
			// this method is especially fast
			// when only a few bits are set in the field
			// which usually happens on read events
			val = socket_log2( bits );
			bits ^= (1<<val);	
			// build the socket number
			sock = nfd*NFDBITS + val;

			///////////////////////////////////////////////////
			// call the user function
			if( session[sock] && (session[sock]->func_send) )
				session[sock]->func_send(sock);
		}
		// go to next field position
		nfd++;
	}
}

#endif /* _WIN32 */

int do_sendrecv(int next)
{
	fd_set rfd,wfd;
	struct timeval timeout;
	int ret,i;
	unsigned int tick = gettick();

	// select するための準備
	memcpy(&rfd,&readfds,sizeof(fd_set));
	FD_ZERO(&wfd);
	for(i=0;i<fd_max;i++){
		struct socket_data *sd = session[i];

		// バッファにデータがあるなら送信可能かチェックする
		if( sd && sd->wdata_size != sd->wdata_pos )
			FD_SET(sock(i),&wfd);
		
		// 受信帯域制限中ならこの socket は受信可能かチェックしない
		if( sd && ( recv_limit_rate_enable && sd->auth >=0 && DIFF_TICK( sd->rlr_tick, tick )>0 )  )
			FD_CLR(sock(i),&rfd);
	}
	
	// タイムアウトの設定（最大1秒）
	if( next > 1000 )
		next = 1000;
	timeout.tv_sec  = next/1000;
	timeout.tv_usec = next%1000*1000;

	// select で通信を待つ
	ret = select(fd_max,&rfd,&wfd,NULL,&timeout);
	if(ret<=0) {
		return 0;
	}
	
	// select 結果にしたがって送受信する
	process_fdset(&rfd,&wfd);
	return 0;
}

int do_parsepacket(void)
{
	int i;
	unsigned int tick = gettick();
	for(i=0;i<fd_max;i++){
		struct socket_data *sd = session[i];
		if(!sd)
			continue;
		if(	sd->eof ||
			( sd->flag_destruct==0 && sd->auth>=0 &&
			  DIFF_TICK( tick, sd->tick ) > ((sd->auth)? auth_timeout : unauth_timeout) )	// タイムアウト
			)
		{
			delete_session(i);
		}
		else
		{
			
			// パケットの解析
			if(sd->func_parse && sd->rdata_size != sd->rdata_pos) {
				int s = RFIFOREST(i);
#ifdef NO_HTTPD
				sd->func_parse(i);
#else
				if(!sd->flag_httpd && httpd_enable) {
					// httpd に回すどうかの判定がまだ行われてない
					// 先頭２バイトが GE or POならhttpd に回してみる
					if(sd->rdata_size - sd->rdata >= 2) {
						if(sd->rdata[0] == 'G' && sd->rdata[1] == 'E') {
							sd->func_parse = httpd_parse;
							sd->auth = -1;
						} else if(sd->rdata[0] == 'P' && sd->rdata[1] == 'O') {
							sd->func_parse = httpd_parse;
							sd->auth = -1;
						}
						sd->flag_httpd = 1;
					}
				}
				if(sd->flag_httpd || !httpd_enable) {
					sd->func_parse(i);
				}
#endif
				// 認証が終了してるなら受信があれば tick を更新
				if( sd->auth && s != RFIFOREST(i) )
				{
					sd->tick = tick;
				}
			}
			
			// クリアする間隔を減らしてみる
			if(sd->rdata_pos == sd->rdata_size) {
				sd->rdata_size = sd->rdata;
				sd->rdata_pos  = sd->rdata;
			} else if( (sd->rdata_pos - sd->rdata) * 8 > (sd->max_rdata - sd->rdata) ) {
				memmove(sd->rdata,RFIFOP(i,0),RFIFOREST(i));
				sd->rdata_size = sd->rdata + RFIFOREST(i);
				sd->rdata_pos  = sd->rdata;
			}
		}
	}
	return 0;
}

int parsepacket_timer(int tid, unsigned int tick, int id, int data) {
	do_parsepacket();
	return 0;
}
/* DDoS 攻撃対策 */

enum {
	ACO_DENY_ALLOW=0,
	ACO_ALLOW_DENY,
	ACO_MUTUAL_FAILURE,
};

struct _access_control {
	unsigned int ip;
	unsigned int mask;
};

static struct _access_control *access_allow;
static struct _access_control *access_deny;
static int access_order=ACO_DENY_ALLOW;
static int access_allownum=0;
static int access_denynum=0;
static int access_debug;
static int ddos_count     = 10;
static int ddos_interval  = 3000;
static int ddos_autoreset = 600*1000;

struct _connect_history {
	struct _connect_history *next;
	struct _connect_history *prev;
	int    status;
	int    count;
	unsigned int ip;
	unsigned int tick;
};
static struct _connect_history *connect_history[0x10000];
static int connect_check_(unsigned int ip);

// 接続できるかどうかの確認
//   false : 接続OK
//   true  : 接続NG
static int connect_check(unsigned int ip) {
	int result = connect_check_(ip);
	if(access_debug) {
		printf("connect_check: connection from %08x %s\n",
			ip,result ? "allowed" : "denied");
	}
	return result;
}

static int connect_check_(unsigned int ip) {
	struct _connect_history *hist     = connect_history[ip & 0xFFFF];
	struct _connect_history *hist_new;
	int    i,is_allowip = 0,is_denyip = 0,connect_ok = 0;

	// allow , deny リストに入っているか確認
	for(i = 0;i < access_allownum; i++) {
		if((ip & access_allow[i].mask) == (access_allow[i].ip & access_allow[i].mask)) {
			if(access_debug) {
				printf("connect_check: match allow list from:%08x ip:%08x mask:%08x\n",
					ip,access_allow[i].ip,access_allow[i].mask);
			}
			is_allowip = 1;
			break;
		}
	}
	for(i = 0;i < access_denynum; i++) {
		if((ip & access_deny[i].mask) == (access_deny[i].ip & access_deny[i].mask)) {
			if(access_debug) {
				printf("connect_check: match deny list  from:%08x ip:%08x mask:%08x\n",
					ip,access_deny[i].ip,access_deny[i].mask);
			}
			is_denyip = 1;
			break;
		}
	}
	// コネクト出来るかどうか確認
	// connect_ok
	//   0 : 無条件に拒否
	//   1 : 田代砲チェックの結果次第
	//   2 : 無条件に許可
	switch(access_order) {
	case ACO_DENY_ALLOW:
	default:
		if(is_allowip) {
			connect_ok = 2;
		} else if(is_denyip) {
			connect_ok = 0;
		} else {
			connect_ok = 1;
		}
		break;
	case ACO_ALLOW_DENY:
		if(is_denyip) {
			connect_ok = 0;
		} else if(is_allowip) {
			connect_ok = 2;
		} else {
			connect_ok = 1;
		}
		break;
	case ACO_MUTUAL_FAILURE:
		if(is_allowip) {
			connect_ok = 2;
		} else {
			connect_ok = 0;
		}
		break;
	}

	// 接続履歴を調べる
	while(hist) {
		if(ip == hist->ip) {
			// 同じIP発見
			if(hist->status) {
				// ban フラグが立ってる
				return (connect_ok == 2 ? 1 : 0);
			} else if(DIFF_TICK(gettick(),hist->tick) < ddos_interval) {
				// ddos_interval秒以内にリクエスト有り
				hist->tick = gettick();
				if(hist->count++ >= ddos_count) {
					// ddos 攻撃を検出
					hist->status = 1;
					printf("connect_check: ddos attack detected (%d.%d.%d.%d)\n",
						ip & 0xFF,(ip >> 8) & 0xFF,(ip >> 16) & 0xFF,ip >> 24);
					return (connect_ok == 2 ? 1 : 0);
				} else {
					return connect_ok;
				}
			} else {
				// ddos_interval秒以内にリクエスト無いのでタイマークリア
				hist->tick  = gettick();
				hist->count = 0;
				return connect_ok;
			}
		}
		hist = hist->next;
	}
	// IPリストに無いので新規作成
	hist_new = calloc(1,sizeof(struct _connect_history));
	hist_new->ip   = ip;
	hist_new->tick = gettick();
	if(connect_history[ip & 0xFFFF] != NULL) {
		hist = connect_history[ip & 0xFFFF];
		hist->prev = hist_new;
		hist_new->next = hist;
	}
	connect_history[ip & 0xFFFF] = hist_new;
	return connect_ok;
}

static int connect_check_clear(int tid,unsigned int tick,int id,int data) {
	int i;
	int clear = 0;
	int list  = 0;
	struct _connect_history *hist , *hist2;
	for(i = 0;i < 0x10000 ; i++) {
		hist = connect_history[i];
		while(hist) {
			if(
				(DIFF_TICK(tick,hist->tick) > ddos_interval * 3 && !hist->status) ||
				(DIFF_TICK(tick,hist->tick) > ddos_autoreset && hist->status)
			) {
				// clear data
				hist2 = hist->next;
				if(hist->prev) {
					hist->prev->next = hist->next;
				} else {
					connect_history[i] = hist->next;
				}
				if(hist->next) {
					hist->next->prev = hist->prev;
				}
				free(hist);
				hist = hist2;
				clear++;
			} else {
				hist = hist->next;
				list++;
			}
		}
	}
	if(access_debug) {
		printf("connect_check_clear: clear = %d list = %d\n",clear,list);
	}
	return list;
}

// IPマスクチェック
int access_ipmask(const char *str,struct _access_control* acc)
{
	unsigned int mask=0,i=0,m,ip, a0,a1,a2,a3;
	if( !strcmp(str,"all") ) {
		ip   = 0;
		mask = 0;
	} else {
		if( sscanf(str,"%d.%d.%d.%d%n",&a0,&a1,&a2,&a3,&i)!=4 || i==0) {
			printf("access_ipmask: unknown format %s\n",str);
			return 0;
		}
		ip = (a3 << 24) | (a2 << 16) | (a1 << 8) | a0;

		if(sscanf(str+i,"/%d.%d.%d.%d",&a0,&a1,&a2,&a3)==4 ){
			mask = (a3 << 24) | (a2 << 16) | (a1 << 8) | a0;
		} else if(sscanf(str+i,"/%d",&m) == 1) {
			for(i=0;i<m;i++) {
				mask = (mask >> 1) | 0x80000000;
			}
			mask = ntohl(mask);
		} else {
			mask = 0xFFFFFFFF;
		}
	}
	if(access_debug) {
		printf("access_ipmask: ip:%08x mask:%08x %s\n",ip,mask,str);
	}
	acc->ip   = ip;
	acc->mask = mask;
	return 1;
}


int socket_config_read2( const char *filename ) {
	int i;
	char line[1024],w1[1024],w2[1024];
	FILE *fp;
	
	fp=fopen(filename,"r");
	if(fp==NULL){
		printf("file not found: %s\n","conf/socket.conf");
		return 1;
	}
	while(fgets(line,1020,fp)){
		if(line[0] == '/' && line[1] == '/')
			continue;

		i=sscanf(line,"%[^:]: %[^\r\n]",w1,w2);
		if(i!=2)
			continue;

		if(strcmpi(w1,"order")==0)
		{
			access_order=atoi(w2);
			if(strcmpi(w2,"deny,allow")==0) access_order=ACO_DENY_ALLOW;
			if(strcmpi(w2,"allow,deny")==0) access_order=ACO_ALLOW_DENY;
			if(strcmpi(w2,"mutual-failure")==0) access_order=ACO_MUTUAL_FAILURE;
		}
		else if(strcmpi(w1,"allow")==0)
		{
			if( strcmpi(w2,"clear")==0 )
			{
				aFree( access_allow );
				access_allow = NULL;
				access_allownum = 0;
			}
			else
			{
				access_allow = aRealloc(access_allow,(access_allownum+1)*sizeof(struct _access_control));
				if(access_ipmask(w2,&access_allow[access_allownum])) {
					access_allownum++;
				}
			}
		}
		else if(strcmpi(w1,"deny")==0)
		{
			if( strcmpi(w2,"clear")==0 )
			{
				aFree( access_deny );
				access_deny = NULL;
				access_denynum = 0;
			}
			else
			{
				access_deny = aRealloc(access_deny,(access_denynum+1)*sizeof(struct _access_control));
				if(access_ipmask(w2,&access_deny[access_denynum])) {
					access_denynum++;
				}
			}
		}
		else if(
			!strcmpi(w1,"httpd") ||
			!strcmpi(w1,"httpd_request_timeout_first") ||
			!strcmpi(w1,"httpd_request_timeout_persist") ||
			!strcmpi(w1,"httpd_max_persist_requests") )
		{	
			// socket.conf の httpd に関する設定は、httpd.conf に統合しました。
			printf("socket_config_read: httpd setting in socket_athena.conf is no more\n");
			printf("                    supported. Please use httpd.conf instead of this.\n\a");
		}
		else if(!strcmpi(w1,"httpd_config"))
		{
			httpd_config_read( w2 );
		}
		else if(!strcmpi(w1,"socket_ctrl_panel_url"))
		{
			httpd_erase_pages( socket_ctrl_panel_url );
			strcpy( socket_ctrl_panel_url, w2 );
			httpd_pages( socket_ctrl_panel_url, socket_httpd_page );
		}
		else if(!strcmpi(w1,"import"))
		{
			socket_config_read2( w2 );
		}
		else {
		
			static const struct
			{
				char name[64];
				int* ptr;
			}
				list[] = 
			{
				{	"debug",						&access_debug				}, 
				{	"socket_ctrl_panel",			&socket_ctrl_panel_httpd	}, 
				{	"ddos_interval",				&ddos_interval				}, 
				{	"ddos_count",					&ddos_count					}, 
				{	"ddos_autoreset",				&ddos_autoreset				}, 
				{	"recv_limit_rate_enable",		&recv_limit_rate_enable		}, 
				{	"recv_limit_rate_period",		&recv_limit_rate_period		}, 
				{	"recv_limit_rate_bytes",		&recv_limit_rate_bytes		}, 
				{	"recv_limit_rate_wait_max",		&recv_limit_rate_wait_max	}, 
				{	"recv_limit_rate_disconnect",	&recv_limit_rate_disconnect	}, 
				{	"send_limit_buffer_size",		&send_limit_buffer_size		},
			};
		
			for( i=0; i<sizeof(list)/sizeof(list[0]); i++ )
			{
				if( strcmpi( w1, list[i].name ) == 0 )
				{
					*list[i].ptr = atoi( w2 );
					break;
				}
			}
		
			if( i==sizeof(list)/sizeof(list[0]) )
			{
				printf("socket_config_read: unknown config: %s",line);
			}
		}
	}
	fclose(fp);
	return 0;
}

int socket_config_read(void)
{
	return socket_config_read2("conf/socket.conf");
}

void do_final_socket(void) {
	int i;
	struct _connect_history *hist , *hist2;
	for(i = 0;i < fd_max; i++) {
		if(session[i]) {
			delete_session(i);
		}
	}
	for(i = 0;i < 0x10000 ; i++) {
		hist = connect_history[i];
		while(hist) {
			hist2 = hist->next;
			free(hist);
			hist = hist2;
		}
	}
	free(access_allow);
	free(access_deny);

	// session[0] のダミーデータを削除
	free(session[0]->rdata);
	free(session[0]->wdata);
	free(session[0]);
}

void do_socket(void)
{
	FD_ZERO(&readfds);
#ifdef _WIN32
	{
		WSADATA  Data;
		if(WSAStartup(MAKEWORD(1,1),&Data) != 0) {
			MessageBox(NULL,"Winsock Dll Load Error","socket.c",MB_OK);
			exit(1);
		}
	}
#endif
	atexit(do_final_socket);
	socket_config_read();

	// session[0] にダミーデータを確保する
	session[0] = (struct socket_data *)aCalloc(1,sizeof(*session[0]));
	session[0]->auth = -1;
	realloc_fifo(0,rfifo_size,wfifo_size);

	// とりあえず５分ごとに不要なデータを削除する
	add_timer_interval(gettick()+1000,connect_check_clear,0,0,300*1000);
}


// ==========================================
// 出力
// ------------------------------------------
static void socket_httpd_page_send( int fd, const char *str )
{
	int len = strlen(str);
	memcpy( WFIFOP(fd,0), str, len );
	WFIFOSET( fd,len );
}

// ==========================================
// ヘッダ部分
// ------------------------------------------
static void socket_httpd_page_header( struct httpd_session_data *sd )
{
	httpd_send_head( sd, 200, "text/html", -1 );
	
	socket_httpd_page_send( sd->fd,
		"<html>\n<head>\n<title>Socket Control Panel</title>\n</head>\n<body>\n"
		"<h1>Socket control panel</h1>\n\n" );

}

// ==========================================
// フッタ部分
// ------------------------------------------
static void socket_httpd_page_footer( int fd )
{
	int len;
	len = sprintf( WFIFOP(fd,0),
				"<p><a href=\"/\">[site top]</a>"
				"<a href=\"%s\">[socket control panel top]</a></p>\n"
				"</body>\n</html>\n", socket_ctrl_panel_url );
	WFIFOSET( fd, len );
}

// ==========================================
// アクセス制御の設定確認
// ------------------------------------------
static void socket_httpd_page_access_settings( struct httpd_session_data *sd, const char *url )
{
	int i, len;
	int fd = sd->fd;
	socket_httpd_page_header( sd );

	len = sprintf( WFIFOP(fd,0),
		"<h2>View Access Control Settings</h2>\n"
		"<table border=1><tr><th>Order</th><td>%s</td></tr>\n"
		"<tr><th>allow</th><th>deny</th></tr>\n"
		"<tr><td>\n",
			(access_order==ACO_DENY_ALLOW)? "deny, allow" : 
			(access_order==ACO_ALLOW_DENY)? "allow, deny" : 
			(access_order==ACO_MUTUAL_FAILURE)? "mutual-failure" : "?" );
	WFIFOSET( fd, len );
	for( i=0; i< access_allownum; i++ )
	{
		unsigned char *ip = (unsigned char*)(&access_allow[i].ip);
		unsigned char *mask = (unsigned char*)(&access_allow[i].mask);
		len = sprintf( WFIFOP(fd,0), "%d.%d.%d.%d/%d.%d.%d.%d<br>\n",
			ip[0], ip[1], ip[2], ip[3], mask[0], mask[1], mask[2], mask[3] );
		WFIFOSET( fd, len );
	}
	socket_httpd_page_send( sd->fd, "</td><td>\n" );
	for( i=0; i< access_denynum; i++ )
	{
		unsigned char *ip = (unsigned char*)(&access_deny[i].ip);
		unsigned char *mask = (unsigned char*)(&access_deny[i].mask);
		len = sprintf( WFIFOP(fd,0), "%d.%d.%d.%d/%d.%d.%d.%d\n",
			ip[0], ip[1], ip[2], ip[3], mask[0], mask[1], mask[2], mask[3] );
		WFIFOSET( fd, len );
	}
	socket_httpd_page_send( sd->fd, "</td></tr></table>" );
	
	socket_httpd_page_footer( sd->fd );
}

// ==========================================
// DoS アタックの状況確認
// ------------------------------------------
static void socket_httpd_page_dos_attack( struct httpd_session_data *sd, const char *url )
{
	int i, n, len;
	int fd = sd->fd;
	unsigned int tick = gettick();
	char *p;
	
	// DoS ブロック解除
	p = httpd_get_value( sd, "dosdelete" );
	if( *p )
	{
		for( i=0; i<100; i++ )
		{
			char buf[32];
			int ip;
			char* p2;
			sprintf( buf, "dosdelete%02x", i );
			p2 = httpd_get_value( sd, buf );
			if( sscanf( p, "%08x", &ip )==1 )
			{
				struct _connect_history *hist = connect_history[ip & 0xffff];
				while( hist )
				{
					if( hist->ip == ip )
						hist->status = 0;
					hist = hist->next;
				}
			}
			aFree( p2 );
		}
	}
	aFree( p );

	
	socket_httpd_page_header( sd );

	// DoS アタックのブロックリスト
	len = sprintf( WFIFOP(fd,0 ),
		"<h2>Anti-DoS Attack : blocking IP address list</h2>\n"
		"<form action=\"%s\" method=\"post\">\n"
		"<input type=\"hidden\" name=\"mode\" value=\"dosattack\">\n"
		"<input type=\"hidden\" name=\"dosdelete\" value=\"1\">\n"
		"<table border=1><tr><th>IP address</th><th>remain(sec.)</th>"
		"<th><input type=\"submit\" value=\"delete\"></th></tr>\n", socket_ctrl_panel_url );
	WFIFOSET( fd, len );
	
	for( i=0, n=0; i<0x10000; i++ )
	{
		struct _connect_history *hist = connect_history[i];
		while(hist)
		{
			int remain = DIFF_TICK( hist->tick + ddos_autoreset, tick );
			if( hist->status && remain>0 )
			{
				unsigned char *ip = (unsigned char *)( &hist->ip );
				if( n<100 )
				{
					len = sprintf( WFIFOP(fd,0),
							"<tr><th>%d.%d.%d.%d</th><td>%d</td>"
							"<td><input type=\"checkbox\" name=\"dosdelete%02x\" value=\"%08x\"></td></tr>\n",
								ip[0], ip[1], ip[2], ip[3], remain/1000, n, hist->ip );
					WFIFOSET( fd, len );
				}
				n++;
			}
			hist = hist->next;
		}
	}
	len = sprintf( WFIFOP(fd,0), "</table>\nblocking %d ip(s) ... </form>\n", n );
	WFIFOSET( fd, len );
	
	socket_httpd_page_footer( sd->fd );
}

void (*socket_httpd_page_connection_func)(int fd,char*,char*,char*);
void socket_set_httpd_page_connection_func( void (*func)(int fd,char*,char*,char*) ){ socket_httpd_page_connection_func=func; }

// ==========================================
// 接続状況確認
// ------------------------------------------
static void socket_httpd_page_connection( struct httpd_session_data *sd, const char *url )
{
	int i, n, len;
	int fd = sd->fd;
	char *p;

	// 強制切断
	p = httpd_get_value( sd, "disconnect" );
	if( *p )
	{
		for( i=1; i<fd_max; i++ )
		{
			char buf[32];
			char* p2;
			sprintf( buf, "discon%04x", i );
			p2 = httpd_get_value( sd, buf );
			if( *p2 && session[i]->func_recv != connect_client )
			{
				session[i]->eof = 1;
			}
			aFree( p2 );
		}
	}
	aFree( p );
	
	
	socket_httpd_page_header( sd );

	len = sprintf( WFIFOP(fd,0),
		"<h2>Connection list</h2>\n"
		"<form action=\"%s\" method=\"post\">"
		"<input type=\"hidden\" name=\"mode\" value=\"connection\">\n"
		"<input type=\"hidden\" name=\"disconnect\" value=\"1\">\n"
		"<table border=1>\n"
		"<tr><th>IP</th><th>usage</th><th>user</th><th>status</th>"
		"<th><input type=\"submit\" value=\"disconnect\"></th></tr>\n", socket_ctrl_panel_url );
	WFIFOSET( fd, len );

	for( i=1, n=0; i<fd_max; i++){
		struct socket_data *sd = session[i];
		unsigned char* ip;
		int type = 0;
		char usage[256]="", user[256]="", status[256]="";
		char *qusage, *quser, *qstatus;
		if( !sd || sd->eof )
			continue;
		
		ip = (unsigned char *)( &sd->client_addr.sin_addr );
		
		strcpy( usage,
			(sd->func_recv == connect_client)? "server" :
			(sd->func_parse == httpd_parse)? ((type=1),"httpd") : ((type=2),"unknown") );
		strcpy( status,
			(sd->func_recv == connect_client)? "listen" :
			(sd->auth==1) ? "authorized" :
			(sd->auth==0) ? "unauthorized" : "permanent connection" );
		
		if( type==1 )
			strcpy( user, ((struct httpd_session_data*)session[fd]->session_data2)->user );
		else if( type==2 && socket_httpd_page_connection_func!=NULL )
			socket_httpd_page_connection_func( i, usage, user, status );
		
		qusage = httpd_quote_meta( usage );
		quser  = httpd_quote_meta( user );
		qstatus= httpd_quote_meta( status );
		
		len = sprintf( WFIFOP(fd,0),
					"<th>%d.%d.%d.%d</th><td>%s</td><td>%s</td><td>%s</td>"
					"<td><input type=\"checkbox\" name=\"discon%04x\" value=\"1\"></td></tr>",
					ip[0], ip[1], ip[2], ip[3], qusage, quser, qstatus, i );
		WFIFOSET( fd, len );
		n++;
		
		aFree( qstatus );
		aFree( quser );
		aFree( qusage );
	}
	
	len = sprintf( WFIFOP(fd,0) , "</table>\n%d connection(s) found.\n", n );
	WFIFOSET( fd, len );
	
	socket_httpd_page_footer( sd->fd );
}

// socket コントロールパネル（do_init_httpd で httpd に登録される）
void socket_httpd_page(struct httpd_session_data* sd,const char* url)
{
	int i, len;
	char *p;
	
	struct
	{
		char mode[32];
		void (*func)(struct httpd_session_data*,const char*);
	} pages[] =
	{
		{ "dosattack",	 socket_httpd_page_dos_attack },
		{ "access",		 socket_httpd_page_access_settings },
		{ "connection",	 socket_httpd_page_connection },
	};
	
	if( socket_ctrl_panel_httpd == 0 )
	{
		httpd_send( sd, 403, "text/plain", 9, "Forbidden" );
		return;
	}

	p = httpd_get_value( sd, "mode" );
	for( i=0; i< sizeof(pages)/sizeof(pages[0]); i++ )
	{
		if( strcmp( p, pages[i].mode )==0 )
		{
			aFree(p);
			pages[i].func( sd, url );
			return;
		}
	}
	aFree(p);
	
	socket_httpd_page_header( sd );
	
	len = sprintf( WFIFOP( sd->fd, 0),
		"<ul>\n"
		"<li><a href=\"%s?mode=access\">View Access Control Settings</a>\n"
		"<li><a href=\"%s?mode=dosattack\">Anti-DoS Attack Status</a>\n"
		"<li><a href=\"%s?mode=connection\">Connection Status</a>\n"
		"</ul>\n", socket_ctrl_panel_url, socket_ctrl_panel_url, socket_ctrl_panel_url );
	WFIFOSET( sd->fd, len );
	
	socket_httpd_page_footer( sd->fd );
}
