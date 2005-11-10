
#ifndef _WIN32
#	include <unistd.h>
#	include <sys/time.h>
#endif

#include <ctype.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "mmo.h"
#include "version.h"
#include "db.h"
#include "httpd.h"
#include "socket.h"
#include "malloc.h"
#include "timer.h"
#include "md5calc.h"


static const char configfile[]="./conf/httpd.conf";	// ���ʃR���t�B�O

static char logfile[1024]="./log/httpd.log";	// ���O�t�@�C����
void httpd_set_logfile( const char *str ){ strcpy( logfile, str ); }

static int log_no_flush = 0;	// ���O���t���b�V�����Ȃ����ǂ���

static int tz = -1;	// �^�C���]�[��

static int auth_digest_period = 600*1000;	// Digest �F�؂ł� nonce �L������
void httpd_set_auth_digest_period( int i ){ auth_digest_period = i; }

static int max_persist_requests = 32;	// �����ʐM�ł̍ő僊�N�G�X�g��
void httpd_set_max_persist_requests( int i ){ max_persist_requests = i; }

static int request_timeout[] = { 2500, 60*1000 };	// �^�C���A�E�g(�ŏ��A����)
void httpd_set_request_timeout( int idx, int time ){ request_timeout[idx]=time; }

static char document_root[256]="./httpd/";	// �h�L�������g���[�g
void httpd_set_document_root( const char *str ){ strcpy( document_root,str ); }

static int bigfile_threshold = 256*1024;	// ����t�@�C���]�����[�h�ɓ���臒l
static int bigfile_splitsize = 256*1024;	// ����t�@�C���]�����[�h�� FIFO �T�C�Y(128KB�ȏ�)

static int max_uri_length = 255;	// URI �̒����𐧌�

static char servername[] = "Athena httpd";

static int server_max_requests_per_second = 10;		// �S�̂̕b�Ԃ̃��N�G�X�g������
static int server_max_requests_period = 5000;		// �S�̂̃��N�G�X�g�������̃`�F�b�N�Ԋu
static int server_max_requests_count = 0;
static int server_max_requests_tick = 0;


static const char *weekdaymsg[]={ "Sun","Mon","Tue","Wed","Thu","Fri","Sat" };
static const char *monthmsg[]={ "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec" };


enum HTTPD_STATUS {
	HTTPD_REQUEST_WAIT = 0,		// ���N�G�X�g�҂�
	HTTPD_REQUEST_WAIT_POST,	// ���N�G�X�g�҂�(post)
	HTTPD_REQUEST_OK,			// ���N�G�X�g���ߊ���
	HTTPD_SEND_HEADER,			// �w�b�_���M����
	HTTPD_SENDING_BIGFILE,		// ����t�@�C�����M��
	HTTPD_WAITING_SEND			// �f�[�^�����M���I���܂ő҂��Ă�����
};


enum httpd_enum {
	HTTPD_ACCESS_ALWAYS		= 0,

	HTTPD_ACCESS_AUTH_NONE	= 0,
	HTTPD_ACCESS_AUTH_BASIC	= 1,
	HTTPD_ACCESS_AUTH_DIGEST= 2,
	HTTPD_ACCESS_AUTH_MASK	= 3,

	HTTPD_ACCESS_IP_NONE	= 0,
	HTTPD_ACCESS_IP_ALLOW	= 4,
	HTTPD_ACCESS_IP_DENY	= 8,
	HTTPD_ACCESS_IP_MUTUAL	=12,
	HTTPD_ACCESS_IP_MASK	=12,

	HTTPD_ACCESS_SAT_ANY	=16,
	HTTPD_ACCESS_SAT_ALL	= 0,
	HTTPD_ACCESS_SAT_MASK	=16,
	
	HTTPD_USER_PASSWD_PLAIN = 0,
	HTTPD_USER_PASSWD_MD5	= 1,
	HTTPD_USER_PASSWD_DIGEST= 2,
	HTTPD_USER_PASSWD_MASK	= 3,
	
	HTTPD_RESHEAD_LASTMOD	= 1,
	HTTPD_RESHEAD_ACCRANGE	= 2,
	
	HTTPD_PRECOND_NONE		= 0,
	HTTPD_PRECOND_IFMOD		= 1,
	HTTPD_PRECOND_IFUNMOD	= 2,
	HTTPD_PRECOND_IFRANGE	= 4,
};

// ���[�U�[�F�ؗp�̍\����
struct httpd_access_user {
	int type;
	unsigned char name[32];
	unsigned char passwd[32];
};

// �A�N�Z�X����f�[�^�̍\����
struct httpd_access {
	unsigned char url[256];
	int type, urllen;

	int dip_count, aip_count, dip_max, aip_max;
	unsigned int *dip, *aip;

	unsigned char realm[256], privkey[32];
	int user_count, user_max;
	struct httpd_access_user *user;
};
static int htaccess_count=0, htaccess_max=0;
static struct httpd_access ** htaccess=NULL;

// Digest �F�؂� nonce-count ���O�p�\����
struct httpd_auth_nonce {
	unsigned char nonce[48];
	unsigned int nc;
};
static struct httpd_auth_nonce nonce_log[256];
static int nonce_log_pos = 0;

static FILE *logfp=NULL;

// ==========================================
// �^�C���]�[���ݒ�
// ------------------------------------------
void httpd_set_timezone( int tz3 )
{
	// �^�C���]�[�������߂�
	int tz2;
	time_t time_temp = time(&time_temp);
	time_t time_local = mktime(localtime(&time_temp));
	time_t time_global = mktime(gmtime(&time_temp));
	tz2 = (time_local - time_global) / (-60);
	
	tz = tz3;
	if( tz==-1 )
		tz=tz2;
	else if( tz!=tz2 )
		printf("*notice* : user defined timezone[%d(%d)] != system timezone[%d(%d)]\n", -tz/60, tz, -tz2/60, tz2);
}

// ==========================================
// �A�N�Z�X���O�̓f���o��
// ------------------------------------------
void httpd_log( struct httpd_session_data *sd, int status, int len )
{
	static int first=1;
	char timestr[256], lenstr[16];
	unsigned char *ip;
	static const char sign[]={'-','+'};

	// ����Ăяo�����͂��낢��Ƃ�邱�Ƃ�����
	if( first )
	{
		first = 0;
		
		// ���O�t�@�C�����J��
		if( (logfp = fopen(logfile, "a") ) == 0 )
		{
			printf("*WARNING* : can't open log file [%s]\n", logfile);
		}
	}

	// ���O�t�@�C���J���ĂȂ�
	if( !logfp )
		return;

	// ���ԕ���������߂�
	{
		time_t time_;
		time(&time_);
		strftime(timestr,sizeof(timestr),"%d/%b/%Y:%H:%M:%S",localtime(&time_) );
	}
	sprintf(timestr+20, " %c%02d%02d", sign[(tz<0)?1:0], abs(tz)/60, abs(tz)%60 );
	
	ip = (unsigned char*) &session[sd->fd]->client_addr.sin_addr;
	
	// �T�C�Y
	if( len<0 )
	{
		lenstr[0]='-';
		lenstr[1]='\0';
	}
	else
	{
		sprintf(lenstr,"%d",len );
	}
	
	
	// ���O���o��
	fprintf( logfp,"%d.%d.%d.%d - %s [%s] \"%s\" %d %s\n",
		ip[0],ip[1],ip[2],ip[3], ((*sd->user)? (char*)sd->user: "-") , timestr, sd->request_line, status, lenstr );
	
	// ���O�̃t���b�V��
	if( !log_no_flush )
		fflush( logfp );
}

// ==========================================
// strncasecmp �̎���
// ------------------------------------------
int httpd_strcasencmp(const char *s1, const char *s2,int len) {
	while(len-- && (*s1 || *s2) ) {
		if((*s1 | 0x20) != (*s2 | 0x20)) {
			return ((*s1 | 0x20) > (*s2 | 0x20) ? 1 : -1);
		}
		s1++; s2++;
	}
	return 0;
}

// httpd �ɓ����Ă���y�[�W�ƁA�Ăяo���R�[���o�b�N�֐��̈ꗗ
struct dbt* httpd_files;

// ==========================================
// httpd_files �̊J������
// ------------------------------------------
static int httpd_db_final(void *key,void *data,va_list ap)
{
	char *url = key;

	free(url);

	return 0;
}

// ==========================================
// httpd �I������
// ------------------------------------------
static void do_final_httpd(void) {
	int i;
	db_final(httpd_files,httpd_db_final);
	
	for( i=0; i<htaccess_count; i++ )
	{
		if(htaccess[i]->user) aFree( htaccess[i]->user );
		if(htaccess[i]->aip)  aFree( htaccess[i]->aip );
		if(htaccess[i]->dip)  aFree( htaccess[i]->dip );
		aFree( htaccess[i] );
	}
	aFree( htaccess );
	htaccess = NULL;
	htaccess_count = htaccess_max = 0;
}

// ==========================================
// httpd ����������
// ------------------------------------------
void do_init_httpd(void) {

//	httpd_config_read(configfile);	// �����������̊֌W�ŁAsocket_config_read2() ����Ă΂��

	httpd_files = strdb_init(0);
	httpd_pages( "/socketctrl", socket_httpd_page );
	atexit(do_final_httpd);
}


// ==========================================
// httpd �Ƀy�[�W��ǉ�
//  for �ȂǂŃy�[�W���������ł���悤�ɁAkey ��strdup()�������̂��g��
// ------------------------------------------
void httpd_pages(const char* url,void (*httpd_func)(struct httpd_session_data*,const char*)) {
	if(strdb_search(httpd_files,url+1) == NULL) {
		strdb_insert(httpd_files,strdup(url+1),httpd_func);
	} else {
		strdb_insert(httpd_files,url+1,httpd_func);
	}
}

static void(*httpd_default)(struct httpd_session_data* sd,const char* url);

// ==========================================
// �f�t�H���g�̃y�[�W�����֐���ݒ�
// ------------------------------------------
void httpd_default_page(void(*httpd_func)(struct httpd_session_data* sd,const char* url)) {
	httpd_default = httpd_func;
}

// ==========================================
// �X�e�[�^�X�R�[�h�����b�Z�[�W�ɕϊ�
// ------------------------------------------
const char *httpd_get_error( struct httpd_session_data* sd, int* status )
{
	const char* msg;
	// httpd �̃X�e�[�^�X�����߂�
	switch(*status) {
	case 200: msg = "OK";              break;
	case 206: msg = "Partial Content"; break;
	case 304: msg = "Not Modified";    break;
	case 400: msg = "Bad Request";     break;
	case 401: msg = "Unauthorized";    break;
	case 403: msg = "Forbidden";       break;
	case 404: msg = "Not Found";       break;
	case 408: msg = "Request Timedout";         break;
	case 411: msg = "Length Required";          break;
	case 412: msg = "Precondition failed";      break;
	case 413: msg = "Request Entity Too Large"; break;
	case 414: msg = "Request-URI Too Long";     break;
	case 416: msg = "Requested Range Not Satisfiable"; break;
	case 503: msg = "Service Unavailable";      break;
	default:
		*status = 500; msg = "Internal Server Error"; break;
	}
	return msg;
}


// ==========================================
// ���t�̉�͗p�i Date �w�b�_�Ȃǁj
// ------------------------------------------
int httpd_get_date( const char *str, time_t *date )
{
	char wday[8],month[8];
	int mday, year, hour, minute, second;
	int i;
	struct tm t;
	
	if( sscanf( str, "%3s, %02d %3s %04d %02d:%02d:%02d GMT", wday, &mday, month, &year, &hour, &minute, &second )!=7 &&
		sscanf( str, "%6s, %02d-%3s-%02d %02d:%02d:%02d GMT", wday, &mday, month, &year, &hour, &minute, &second )!=7 &&
		sscanf( str, "%3s %3s %2d %02d:%02d:%02d %04d",  wday, month, &mday, &hour, &minute, &second, &year )!=7 )
		return 0;
	
	for( i=0; i<12; i++ )
	{
		if( httpd_strcasencmp( monthmsg[i], month, 3 )==0 )
			break;
	}
	if( i==12 )
		return 0;
	t.tm_mon = i;
	
	for( i=0; i<7; i++ )
	{
		if( httpd_strcasencmp( weekdaymsg[i], wday, 3 )==0 )
			break;
	}
	if( i==7 )
		return 0;
	t.tm_wday  = i;
	
	t.tm_sec  = second;
	t.tm_min  = minute;
	t.tm_hour = hour;
	t.tm_mday = mday;
	t.tm_year = (year>=1900)? year-1900 : year;
	t.tm_isdst= 0;
	
	*date = mktime( &t ) - tz*60;
	
	return 1;
}

// ==========================================
// �G���[�̑��M
// ------------------------------------------
void httpd_send_error(struct httpd_session_data* sd,int status) {
	const char* msg = httpd_get_error( sd, &status );
	httpd_send(sd,status,"text/plain",strlen(msg),msg);
}

// ==========================================
// ���t�̕�����쐬
// ------------------------------------------
int httpd_make_date( char* dst, time_t date )
{
	const struct tm *t = gmtime( &date );
	return sprintf( dst,
		"%s, %02d %s %04d %02d:%02d:%02d GMT\r\n",
		weekdaymsg[t->tm_wday], t->tm_mday, monthmsg[t->tm_mon], (t->tm_year<1900)? t->tm_year+1900: t->tm_year, 
		t->tm_hour, t->tm_min, t->tm_sec );
}

// ==========================================
// ���X�|���X�w�b�_���M
// ------------------------------------------
void httpd_send_head(struct httpd_session_data* sd,int status,const char *content_type,int content_len) {
	char head[1024];
	int  len;
	const char* msg;

	if(sd->status != HTTPD_REQUEST_OK) return;
	msg = httpd_get_error( sd, &status );
	
	// �F�؊֘A�̃f�[�^���Ȃ񂩂�������
	if( status==401 && ( !sd->access || !(sd->access->type & HTTPD_ACCESS_AUTH_MASK) ) )
		status = 500;
	
	if( (status != 200 && status != 304  && status != 401 && status != 412 ) || ++sd->request_count >= max_persist_requests ) {
		// �����ؒf����(status ��200�A304�A401�A412 �ȊO or ���N�G�X�g���E����)
		sd->persist = 0;
	}

	len = sprintf( head,
		"HTTP/1.%d %d %s\r\n"
		"Server: %s/mod%d\r\n", sd->http_ver,status,msg, servername, ATHENA_MOD_VERSION );
	
	if( content_type )	// �R���e���g�^�C�v
	{
		len += sprintf( head + len, "Content-Type: %s\r\n",content_type );
	}
	
	if( content_len == -1 )		// ������������Ȃ�
	{
		if( status!=304 )
			sd->persist = 0;
	} else {
		len += sprintf(head + len,"Content-Length: %d\r\n",content_len);
	}
	
	if( status==206 )	// Content-Range �ʒm
	{
		len += sprintf(head + len,"Content-Range: bytes %d-%d/%d\r\n", sd->range_start, sd->range_end, sd->inst_len );
	}
	
	if( sd->persist==0 )	// ���������ǂ���
	{
		len += sprintf(head + len,"Connection: close\r\n");
	}
	else if( sd->http_ver==0 ) // HTTP/1.0 �Ȃ� Keep-Alive �ʒm
	{
		len += sprintf(head + len,"Connection: Keep-Alive\r\n");
	}

	if( status==401 )	// �F�؂��K�v
	{
		if( (sd->access->type & HTTPD_ACCESS_AUTH_MASK) == HTTPD_ACCESS_AUTH_DIGEST )
		{
			static const char *stale[]={"false","true"};
			char nonce[128];

			// nonce �쐬
			{
				char buf[128];
				sprintf( buf, "%08x:%s", gettick(), sd->access->privkey );
				sprintf( nonce, "%08x", gettick() );
				MD5_String( buf, nonce+8 ); 

				nonce_log[ nonce_log_pos ].nc = 1;
				strcpy( nonce_log[ nonce_log_pos ].nonce, nonce );
				nonce_log_pos = ( nonce_log_pos + 1 )%( sizeof(nonce_log)/sizeof(nonce_log[0]) );
			}
			
			len += sprintf( head+len,
				"WWW-Authenticate: Digest realm=\"%s\", nonce=\"%s\", algorithm=MD5, qop=\"auth\", stale=%s\r\n",
				sd->access->realm, nonce, stale[sd->auth_digest_stale] );
		}
		else
		{
			len += sprintf( head+len,
				"WWW-Authenticate: Basic realm=\"%s\"\r\n", sd->access->realm );
		}
	}

	if( status==503 )	// Retry-after �ʒm
	{
		len += sprintf( head + len, "Retry-After: %d\r\n", (server_max_requests_period+999)/1000 );
	}
	
	if( sd->reshead_flag & HTTPD_RESHEAD_ACCRANGE )	// Accept-Ranges �ʒm
	{
		len += sprintf( head + len, "Accept-Ranges: bytes\r\n" );
	}
	
	if( sd->date && (sd->reshead_flag & HTTPD_RESHEAD_LASTMOD))	// Last-modified �̒ʒm
	{
		strcpy( head+len, "Last-Modified: " );
		len += 15;
		len += httpd_make_date( head+len, sd->date );
	}
	
	if( status!=500 )	// Date �̒ʒm
	{
		time_t tmp = time( &tmp );
		strcpy( head+len, "Date: " );
		len += 6;
		len += httpd_make_date( head+len, tmp );
	}
	
	httpd_log( sd, status, content_len );	// ���O�ɋL�^
	
	len += sprintf( head+len, "\r\n" );
	memcpy(WFIFOP(sd->fd,0),head,len);
	WFIFOSET(sd->fd,len);
	sd->status   = HTTPD_SEND_HEADER;
	sd->data_len = content_len;
}

// ==========================================
// �f�[�^���M
// ------------------------------------------
void httpd_send_data(struct httpd_session_data* sd,int content_len,const void *data) {
	const char* msg = (const char*)data;
	if(sd->status == HTTPD_REQUEST_OK) {
		// �w�b�_�̑��M�Y��Ă���̂ŁA�K���ɕ₤
		httpd_send_head(sd,200,"application/octet-stream",-1);
	} else if(sd->status != HTTPD_SEND_HEADER && sd->status != HTTPD_WAITING_SEND) {
		return;
	}
	sd->data_len -= content_len;

	// ����ȃT�C�Y�̃t�@�C�������M�o����悤�ɕ������đ���
	while(content_len > 0) {
		int send_byte = content_len;
		if(send_byte > 12*1024) send_byte = 12*1024;
		memcpy(WFIFOP(sd->fd,0),msg,send_byte);
		WFIFOSET(sd->fd,send_byte);
		msg += send_byte; content_len -= send_byte;
	}
	sd->status = HTTPD_WAITING_SEND;
}

// ==========================================
// �w�b�_�ƃf�[�^�̈ꊇ���M
// ------------------------------------------
void httpd_send(struct httpd_session_data* sd,int status,const char *content_type,int content_len,const void *data) {
	httpd_send_head(sd,status,content_type,content_len);
	httpd_send_data(sd,content_len,data);
}

// ==========================================
// �ؒf�҂�
// ------------------------------------------
int httpd_disconnect(int fd) {
	// nothing to do
	return 0;
}

int  httpd_parse_header(struct httpd_session_data* sd);
void httpd_parse_request_ok(struct httpd_session_data *sd);
void httpd_send_bigfile( struct httpd_session_data* sd );

// ==========================================
// httpd �̃p�P�b�g���
// ------------------------------------------
int  httpd_parse(int fd) {
	struct httpd_session_data *sd = (struct httpd_session_data*)session[fd]->session_data2;
	if(sd == NULL) {
		sd = (struct httpd_session_data*)aCalloc(sizeof(struct httpd_session_data),1);
		sd->fd  = fd;
		session[fd]->func_destruct = httpd_disconnect;
		session[fd]->session_data2  = sd;
		sd->tick = gettick();
		sd->range_start= 0;
		sd->range_end  = -1;
	}
	
	// ��x�̃��N�G�X�g�� 32KB �ȏ㑗�M�����Ɛؒf����
	if( RFIFOREST(fd) > 32768 )
	{
		session[fd]->eof = 1;
		return 0;
	}
	
	switch(sd->status) {
		
	case HTTPD_REQUEST_WAIT:
		// ���N�G�X�g�҂�
	
		if(RFIFOREST(fd) > 1024) {
			// ���N�G�X�g����������̂ŁA�G���[��������
			sd->status = HTTPD_REQUEST_OK;
			httpd_send_error(sd,400); // Bad Request
		}
		else if( (int)( gettick() - sd->tick ) > request_timeout[sd->persist] )
		{
			// ���N�G�X�g�Ɏ��Ԃ������肷���Ă���̂ŁA�G���[��������
			sd->status = HTTPD_REQUEST_OK;
			httpd_send_error(sd,408); // Request Timeout
		}
		else if(sd->header_len == RFIFOREST(fd))
		{
			// ��Ԃ��ȑO�Ɠ����Ȃ̂ŁA���N�G�X�g���ĉ�͂���K�v�͖���
		}
		else
		{
			int limit = RFIFOREST(fd);
			unsigned char *req = RFIFOP(fd,0);

			// �b�ԏ����������̃`�F�b�N
			if( server_max_requests_count >= server_max_requests_per_second*server_max_requests_period/1000 )
			{
				int tick = gettick();
				if( DIFF_TICK( server_max_requests_tick + server_max_requests_period, tick ) < 0 )
				{
					server_max_requests_count = 0;
					server_max_requests_tick = tick;
				}
				else
				{
					// �T�[�o�[�̏����������z���Ă���
					sd->status = HTTPD_REQUEST_OK;
					httpd_send_error(sd,503); // Service Unavailable
				}
			}

			// ���N�G�X�g�̉��
			sd->header_len = RFIFOREST(fd);
			do {
				if(*req == '\n' && limit > 0) {
					limit--; req++;
					if(*req == '\r' && limit > 0) { limit--; req++; }
					if(*req == '\n') {
						int status;
						// HTTP�w�b�_�̏I�_��������
						*req   = 0;
						sd->header_len = (req - RFIFOP(fd,0)) + 1;
						status = httpd_parse_header(sd);
						if(sd->status == HTTPD_REQUEST_WAIT) {
							sd->status = HTTPD_REQUEST_OK;
							httpd_send_error(sd,status); 
						}
						server_max_requests_count++;
						break;
					}
				}
			} while(req++,--limit > 0);
		}
		break;
		
	case HTTPD_REQUEST_WAIT_POST:
		// POST ���\�b�h�ł̃G���e�B�e�B��M�҂�
		
		if( (int)( gettick() - sd->tick ) > request_timeout[sd->persist] ) {
			// ���N�G�X�g�Ɏ��Ԃ������肷���Ă���̂ŁA�G���[��������
		} else
		if(RFIFOREST(sd->fd) >= sd->header_len) {
			unsigned char temp = RFIFOB(sd->fd,sd->header_len);
			RFIFOB(sd->fd,sd->header_len) = 0;
			httpd_parse_request_ok(sd);
			RFIFOB(sd->fd,sd->header_len) = temp;
		}
		break;

	case HTTPD_REQUEST_OK:
	case HTTPD_SEND_HEADER:
		// ���N�G�X�g���I������܂܉������M����Ă��Ȃ���ԂȂ̂ŁA
		// �����ؒf
		session[fd]->eof = 1;
		break;
		
	case HTTPD_SENDING_BIGFILE:
		// ����t�@�C�����M��
		httpd_send_bigfile(sd);
		break;

	case HTTPD_WAITING_SEND:
		// �f�[�^�̑��M���I���܂őҋ@
		if(session[fd]->wdata_size == session[fd]->wdata_pos) {
			// HTTP/1.0�͎蓮�ؒf
//			if(sd->http_ver == 0) {
			if(sd->persist == 0) {
				session[fd]->eof = 1;
			}
			// RFIFO ���烊�N�G�X�g�f�[�^�̏����ƍ\���̂̏�����
			RFIFOSKIP(fd,sd->header_len);
			sd->status     = HTTPD_REQUEST_WAIT;
			sd->tick       = gettick();
			sd->header_len = 0;
			sd->query      = NULL;
			sd->auth       = NULL;
			sd->precond    = HTTPD_PRECOND_NONE;
			sd->date       = 0;
			sd->range_start= 0;
			sd->range_end  = -1;
//			sd->http_ver   = 0;	// ver �͕ێ�
			sd->user[0]    = 0;
			sd->method     = HTTPD_METHOD_UNKNOWN;
			// printf("httpd_parse: [% 3d] request sended RFIFOREST:%d\n",fd,RFIFOREST(fd));
		}
		break;
	}
	return 0;
}

// ==========================================
// URL �̃f�R�[�h
// ------------------------------------------
int httpd_decode_url( char *url )
{
	int s=0, d=0;
	// url �f�R�[�h
	while( url[s]!='\0' && url[s]!='?' )
	{
		if( url[s]=='%' )
		{
			#define HEX2INT(a) ( ( a>='0' && a<='9' )? a-'0' : (a>='A' && a<='F' )? a-'A'+10 : (a>='a' && a<='f' )? a-'a'+10 : -1 )
			int a1 = HEX2INT(url[s+1]), a2 = HEX2INT(url[s+2]);
			#undef HEX2INT
			if( a1>=2 && a2>=0)	// 0x20�`
			{
				s+=3;
				url[ d++ ] = (a1<<4) + a2;
			}
			else if( a1>=0 && a1<2 )	// url �ɐ��䕶�������悤�Ƃ���
			{
				return 0;
			}
			continue;
		}
		else if( url[s]=='+' )
		{
			url[ d++ ]= ' ';
			s++;
			continue;
		}
		url[ d++ ] = url[ s++ ];
	}
	url[d] = '\0';

	// sjis ���Ɛ���ɂ͂����Ȃ����Ƃ�����
	s=d=0;
	while( url[s]!='\0' && url[s]!='?' )
	{
		int c0 = url[s], c1 = url[s+1], c2 = url[s+2], c3 = url[s+3];
/*		if( url[s]=='\\' )	// �o�b�N�N�H�[�g���܂܂�Ă���ƒ����ɃG���[
		{
			return 0;
		}*/

		// �֎~�����`�F�b�N
		if( url[s]=='|' || url[s]=='<' || url[s]=='>' || url[s]=='?'  || url[s]=='*' )
			return 0;
		
		if( c0=='/' && (c1=='/' || c1=='\\') )	// �A�������X���b�V���F�Ӗ��������̂ō폜
		{
			s++;
			url[s]='/';
			continue;
		}
		else if( (c0=='/' || c0=='\\') && c1=='.' && (c2=='/' || c2=='\\') )	// �J�����g�f�B���N�g���F�Ӗ����Ȃ��̂ō폜
		{
			s+=2;
			url[s]='/';
			continue;
		}
		else if( (c0=='/' || c0=='\\') && c1=='.' && c2=='.' && (c3=='/' || c3=='\\') )	// �e�f�B���N�g���F�W�J�܂��̓G���[
		{
			int r;
			if( d<=0 )
				return 0;
			for( r=d-1; r>=0 && url[r]!='/'; r-- );
			if( r<0 )
				return 0;
			s+=3;
			d = r;
			url[s]='/';
			continue;
		}
		
		url[ d++ ] = url[ s++ ];
	}
	url[d] = '\0';

	return 1;
}

// ==========================================
// IP �`�F�b�N
// ------------------------------------------
int httpd_check_access_ip( struct httpd_access *a, struct httpd_session_data *sd )
{
	int i;
	int fa=0, fd=0;
	int order = a->type & HTTPD_ACCESS_IP_MASK;
	unsigned int ip = *(unsigned int *)(&session[sd->fd]->client_addr.sin_addr);
	
	for( i=0; i<a->aip_count; i+=2 )
	{
		if( (ip & a->aip[i+1]) == a->aip[i] )
			fa=1;
	}
	for( i=0; i<a->dip_count; i+=2 )
	{
		if( (ip & a->dip[i+1]) == a->dip[i] )
			fd=1;
	}
	
	return	( order == HTTPD_ACCESS_IP_DENY  )?  (fd ? fa : 1)  :
			( order == HTTPD_ACCESS_IP_ALLOW )?  (fa ? !fd : 0) :
			( order == HTTPD_ACCESS_IP_MUTUAL)?  (fa & !fd )    : 0;
}


// ==========================================
// ���[�U�[�F��(�_�C�W�F�X�g�F��)
// ------------------------------------------
int httpd_check_access_user_digest( struct httpd_access *a, struct httpd_session_data *sd )
{
	char buf[1024];
	char response[33]="",nonce[128]="",uri[1024]="";
	char username[33]="",realm[256]="",cnonce[256]="",nc[9]="";
	struct httpd_access_user *au=NULL;
	int i = 7 ,n;

	sd->auth_digest_stale = 0;

	if( !sd->auth || httpd_strcasencmp( sd->auth, "Digest ", 7 )!=0 )
		return 0;
	
	// �w�b�_�̉��
	while( sd->auth[i] )
	{
		if( httpd_strcasencmp( sd->auth+i, "username=\"", 10 )==0 )		// ���[�U�[��
		{
			if( sscanf(sd->auth+i+10,"%[^\"]%n", buf, &n )==1 )
			{
				if( n>=sizeof(username) )
					return 0;
				strcpy( username, buf );
				i+= 11 + n;
			}
			else
				return 0;
		}
		if( httpd_strcasencmp( sd->auth+i, "realm=\"", 7 )==0 )		// �̈於
		{
			if( sscanf(sd->auth+i+7,"%[^\"]%n", buf, &n )==1 )
			{
				if( n>=sizeof(realm) || strcmp(buf,a->realm)!=0 )
					return 0;
				strcpy( realm, buf );
				i+= 8 + n;
			}
			else
				return 0;
		}
		if( httpd_strcasencmp( sd->auth+i, "nonce=\"", 7 )==0 )		// nonce
		{
			if( sscanf(sd->auth+i+7,"%[^\"]%n", buf, &n )==1 )
			{
				if( n>=sizeof(nonce) || n<40  )
					return 0;
				strcpy( nonce, buf );
				i+= 8 + n;
			}
			else
				return 0;
		}
		if( httpd_strcasencmp( sd->auth+i, "uri=\"", 5)==0 )		// URI
		{
			if( sscanf(sd->auth+i+5,"%[^\"]%n", uri, &n )==1 )
			{
				i+= 6 + n;
			}
			else
				return 0;
		}
		if( httpd_strcasencmp( sd->auth+i, "algorithm=", 10) ==0 )	// �A���S���Y���iMD5�Œ�j
		{
			if( httpd_strcasencmp(sd->auth+i+10, "MD5", 3)==0 )
				i+= 14;
			else if( httpd_strcasencmp(sd->auth+i+10, "\"MD5\"", 5)==0 )	// ie �̃o�O���z��
				i+= 16;
			else
				return 0;
		}
		if( httpd_strcasencmp( sd->auth+i, "qop=", 4) ==0 )			// qop�iauth�Œ�j
		{
			if( httpd_strcasencmp(sd->auth+i+4, "auth", 4)==0 )
				i+= 8;
			else if( httpd_strcasencmp(sd->auth+i+4, "\"auth\"", 6)==0 )	// ie �̃o�O���z��
				i+=10;
			else
				return 0;
		}
		if( httpd_strcasencmp( sd->auth+i, "cnonce=\"", 8 )==0 )	// cnonce
		{
			if( sscanf(sd->auth+i+8,"%[^\"]%n", buf, &n )==1 )
			{
				if( n>=sizeof(cnonce))
					return 0;
				strcpy( cnonce, buf );
				i+= 9 + n;
			}
			else
				return 0;
		}
		if( httpd_strcasencmp( sd->auth+i, "nc=", 3 )==0 )			// nonce count
		{
			if( sscanf(sd->auth+i+3,"%[^, ]%n", buf, &n)==1 )
			{
				if( n!=8 )
					return 0;
				strcpy( nc, buf );
				i+= 11;
			}
			else
				return 0;
		}
		if( httpd_strcasencmp( sd->auth+i, "response=\"", 10 )==0 )	// response
		{
			if( sscanf(sd->auth+i+10,"%[^\"]%n", buf, &n)==1 )
			{
				if( n!=32 )
					return 0;
				strcpy( response, buf );
				i+= 43;
			}
			else
				return 0;
		}
		if( httpd_strcasencmp( sd->auth+i, "auth_param=", 11 )==0 )	// auth_param
		{
			int c=',';
			i+=11;
			if( sd->auth[i]=='\"' ) c='\"';
			while( sd->auth[i] && sd->auth[i]!=c )
				i++;
			i++;
		}
		
		if( sd->auth[i]!=' ' && sd->auth[i]!=',' && sd->auth[i] )
			return 0;

		while( sd->auth[i]==' ' || sd->auth[i]==',' )
		{
			i++;
		}
	}

//	printf("response=[%s]\nnonce=[%s]\nusername=[%s]\nrealm=[%s]\ncnonce=[%s]\nnc=[%s]\nuri=[%s]\n",
//		response,nonce,username,realm,cnonce,nc,uri);

	// ���[�U�[���̊m�F
	for( i=0; i<a->user_count; i++ )
	{
		if( a->user[i].type != HTTPD_USER_PASSWD_MD5 &&
			strcmp( a->user[i].name, username ) == 0 )
		{
			au = &a->user[i];
			break;
		}
	}
	if( i==a->user_count )
		return 0;

	// uri �̑Ó�������
	{
		int i=0;
		char req_uri[1024];
		const char *line = sd->request_line;
		while( *line && *line!=' ' ) line++;
		line++;
		while( *line && *line!=' ' )
			req_uri[i++] = *(line++);
		req_uri[i]='\0';
		
		if( strcmpi( req_uri, uri )!=0 )
		{
			// ie �̃o�O�z��
			for( i=0; req_uri[i] && req_uri[i]!='?'; i++ );
			if( req_uri[i]!='?' )
				return 0;
			req_uri[i]='\0';
			if( strcmpi( req_uri, uri )!=0 )	// uri ������Ȃ�
				return 0;
		}
	}

	// nonce �̑Ó�������
	{
		char buf2[33];
		unsigned int tick;
		if( sscanf(nonce,"%08x",&tick)!=1 )	// tick ���Ȃ�
			return 0;
		
		sprintf( buf, "%08x:%s", tick, a->privkey );
		MD5_String( buf, buf2 );
		if( strcmpi( nonce+8, buf2 )!=0 )	// �v�Z���@���Ⴄ
			return 0;
		
		if( (int)( gettick() - tick ) > auth_digest_period )	// �L���������؂ꂽ�̂� stale �t���O�ݒ�
		{
			sd->auth_digest_stale = 1;
			return 0;
		}
	}
	
	// nc �̑Ó�������
	{
		int i;
		unsigned int nci;
		
		if( sscanf(nc,"%08x",&nci )!=1 )	// nc ���Ȃ�
			return 0;
		
		if( nci<=0 )
			return 0;

		for( i=0; i<sizeof(nonce_log)/sizeof(nonce_log[0]); i++ )
		{
			if( strcmpi( nonce_log[i].nonce, nonce )==0 )
			{
				if( nonce_log[i].nc != nci )		// nc ������Ȃ�
					return 0;
				break;
			}
		}
		if( i==sizeof(nonce_log)/sizeof(nonce_log[0]) )	// �Â�����(�H) nonce ���O�ɖ����̂� stale �t���O�ݒ�
		{
			sd->auth_digest_stale = 1;
			return 0;
		}
		nonce_log[i].nc++;
	}
	
	// rensponse �̑Ó�������
	{
		static const char *method[]={"","GET","POST"};
		char a1[33],a2[33],res[33];
		
		// A1 �v�Z
		if( au->type == HTTPD_USER_PASSWD_PLAIN )
		{
			sprintf( buf, "%s:%s:%s", username, realm, au->passwd );
			MD5_String( buf, a1 );
		}
		else if( au->type == HTTPD_USER_PASSWD_DIGEST )
		{
			strcpy( a1, au->passwd );
		}
		else
			return 0;

		// A2 �v�Z
		sprintf( buf,"%s:%s", method[sd->method], uri );
		MD5_String( buf, a2 );
		
		// response �v�Z
		sprintf( buf,"%s:%s:%s:%s:%s:%s", a1, nonce, nc, cnonce, "auth", a2 );
		MD5_String( buf, res );

		if( strcmpi( res, response )==0 )
		{
			strcpy( sd->user, username );
			return 1;
		}
	}
	
	
	return 0;
}

// ==========================================
// ���[�U�[�F��(�x�[�V�b�N�F��)
// ------------------------------------------
int httpd_check_access_user_basic( struct httpd_access *a, struct httpd_session_data *sd )
{
	// ���[�U�[�`�F�b�N
	char buf[1024], name[1024], passwd[1024];
	int i;
	
	if( !sd->auth || httpd_strcasencmp( sd->auth, "Basic ", 6 )!=0 )
		return 0;
	
	if( httpd_decode_base64( buf, sd->auth+6 ) && sscanf(buf,"%[^:]:%[^\r]",name,passwd) == 2 )
	{
		char *apass[4]={NULL,NULL,NULL,""};
		apass[0]=passwd;
		apass[1]=buf;
		apass[2]=buf+64;
		
		sprintf( buf+128, "%s:%s", name, passwd );
		MD5_String( buf+128, buf );
		sprintf( buf+128, "%s:%s:%s", name, a->realm, passwd );
		MD5_String( buf+128, buf+64 );

		for( i=0; i<a->user_count; i++ )
		{
					
			if( strcmp( a->user[i].name, name ) == 0 &&
				strcmp( a->user[i].passwd, apass[a->user[i].type & HTTPD_USER_PASSWD_MASK] )==0 )
			{
				strcpy( sd->user, name );
				return 1;
			}
		}
	}
	return 0;
}

// ==========================================
// �A�N�Z�X�����̃`�F�b�N
// ------------------------------------------
int httpd_check_access( struct httpd_session_data *sd, int *st )
{
	int i;
	int n=-1, len=0;
	struct httpd_access *a;

	// ��Ԓ����}�b�`���������T��
	for( i=0; i<htaccess_count; i++ )
	{
		struct httpd_access *a = htaccess[i];
		if( a->urllen > len && httpd_strcasencmp( a->url, sd->url-1, a->urllen )==0 ) {
			n=i; len = a->urllen;
		}
	}
	if( n<0 )
		return 1;
	
	a = htaccess[n];
	
	// IP �`�F�b�N
	if( a->type & HTTPD_ACCESS_IP_MASK )
	{
		int f = httpd_check_access_ip( a, sd );
		
		if( f && (a->type & HTTPD_ACCESS_SAT_ANY || !(a->type & HTTPD_ACCESS_AUTH_MASK) ) )
			return 1;
		else if( !f && ( !(a->type & HTTPD_ACCESS_SAT_ANY) || !(a->type & HTTPD_ACCESS_AUTH_MASK) ) )
		{
			*st=403;
			return 0;
		}
	}
	
	// ���[�U�[�F��
	if( a->type & HTTPD_ACCESS_AUTH_MASK )
	{
		int f = ( (a->type & HTTPD_ACCESS_AUTH_MASK) == HTTPD_ACCESS_AUTH_BASIC )?
				httpd_check_access_user_basic( a, sd ) : httpd_check_access_user_digest( a, sd );
		
		if( !f )
		{
			*st=401;
			sd->access = a;
			return 0;
		}
		return 1;
	}

	*st = 403;
	return 0;
}

// ==========================================
// ���N�G�X�g�w�b�_�̉��
// ------------------------------------------
int httpd_parse_header(struct httpd_session_data* sd) {
	int i;
	int content_len    = -1;
	int persist        = -1;
	unsigned char* req = RFIFOP(sd->fd,0);
	int  c;
	int status;
	
	// �܂��w�b�_�̉��s������null�����ɒu�������A�擪�s�ȊO�̉�͂�����
	while(*req) {
		if(*req == '\r' || *req == '\n') {
			*req = 0;
			// Content-Length: �̒���
			if(!httpd_strcasencmp(req+1,"Content-Length: ",16)) {
				content_len = atoi(req + 17);
			}
			// Connection: �̒���
			if(!httpd_strcasencmp(req+1,"Connection: ",12)) {
				if( httpd_strcasencmp(req+13,"close",5)==0)
					persist = 0;
				if( httpd_strcasencmp(req+13,"Keep-Alive",10)==0)
					persist = 1;
			}
			// Authorization: �̒���
			if(!httpd_strcasencmp(req+1,"Authorization: ",15)) {
				sd->auth = req+16;
			}
			// Range: �̒���
			if(!httpd_strcasencmp(req+1,"Range: ", 7)){
				if( httpd_strcasencmp(req+8,"bytes=",6 ))	// �o�C�g�����W����Ȃ�
					return 416;
				sd->range_start = atoi( req+14 );
				req += 15;
				while( *req>='0' && *req<='9' ) req++;
				if( *req!='-' && sd->range_start>=0 )	// �J�n�ʒu��������Ȃ��̂Ƀn�C�t��������
					return 400;
				
				if( *req=='-' )
				{
					req++;
					if( *req>='0' && *req<='9' )
					{
						sd->range_end = atoi( req );
						if( sd->range_end < sd->range_start )	// �T�C�Y���}�C�i�X�ɂȂ�
							return 416;
						while( *req>='0' && *req<='9' ) req++;
					}
				}
				if( *req!='\r' && *req!='\n' )	// range-set ���������邩�s��
					return 400;
				req--;
			}
			// If-Modified-Since �̒���
			if( !httpd_strcasencmp(req+1,"If-Modified-Since: ",19) && httpd_get_date( req+20, &sd->date ) ) {
				if( sd->precond )
					return 400;
				sd->precond |= HTTPD_PRECOND_IFMOD;
			}
			// If-Unmodified-Since �̒���
			if( !httpd_strcasencmp(req+1,"If-Unmodified-Since: ",21) && httpd_get_date( req+22, &sd->date ) ) {
				if( sd->precond )
					return 400;
				sd->precond |= HTTPD_PRECOND_IFUNMOD;
			}
			// If-Range �̒���
			if(!httpd_strcasencmp(req+1,"If-Range: ",10) ) {
				if( sd->precond || !httpd_get_date( req+11, &sd->date ) )
					return 400;
				sd->precond |= HTTPD_PRECOND_IFRANGE;
			}
		}
		req++;
	}
	req = RFIFOP(sd->fd,0);
	strncpy( sd->request_line, req, sizeof(sd->request_line) );

	if(!strncmp(req,"GET /",5)) {		// GET ���N�G�X�g
		req += 5;
		for(i = 0;req[i]; i++) {
			c = req[i];
			if(c == ' ' || c == '?') break;
			if(!isalnum(c) && c != '.' && c != '_' && c != '-' && c !='/' && c != '%') break;
		}
		if(req[i] == ' ') {
			req[i]     = 0;
			sd->url    = req;
			sd->query  = NULL;
		} else if(req[i] == '?') {
			req[i]    = 0;
			sd->query = &req[++i];
			for(;req[i];i++) {
				c = req[i];
				if(c == ' ') break;
				if(!isalnum(c) && c != '+' && c == '%' && c == '&' && c == '=') break;
			}
			if(req[i] != ' ') return 400; // Bad Request
			req[i]     = 0;
			sd->url    = req;
		} else {
			return 400; // Bad Request
		}
		
		// URI ����������
		if( i > max_uri_length )
		{
			sd->request_line[ max_uri_length+5 ]='\0';
			return 414;		// Request-URI Too Long
		}
		
		// �w�b�_���
		if(!strncmp(&req[i+1] ,"HTTP/1.1",8)) {
			sd->http_ver = 1;
			sd->persist  = (persist == -1 ? 1 : persist);
		} else {
			sd->http_ver = 0;
			sd->persist  = (persist == -1 ? 0 : persist);
		}
		sd->method = HTTPD_METHOD_GET;
		
		// URL �f�R�[�h
		if( !httpd_decode_url( req - 1 ) ) return 400; // Bad Request

		// �A�N�Z�X����̃`�F�b�N
		if( !httpd_check_access( sd, &status ) )
			return status;

		// printf("httpd: request %s %s\n",sd->url,sd->query);
		httpd_parse_request_ok(sd);
		

	} else if(!strncmp(req,"POST /",6)) {	// POST ���N�G�X�g
	
		req += 6;
		for(i = 0;req[i]; i++) {
			c = req[i];
			if(c == ' ') break;
			if(	!isalnum(c) && c != '.' && c != '_' && c != '-' && c !='/' && c != '%' ) break;
		}
		if(req[i] != ' ') return 400; // Bad Request
		req[i]     = 0;
		sd->url    = req;
		
		// URI ����������
		if( i > max_uri_length )
		{
			sd->request_line[ max_uri_length+6 ]='\0';
			return 414;		// Request-URI Too Long
		}

		// �w�b�_���
		if(!strncmp(&req[i+1] ,"HTTP/1.1",8)) {
			sd->http_ver = 1;
			if(sd->persist == -1) sd->persist  = 1;
		} else {
			sd->http_ver = 0;
			if(sd->persist == -1) sd->persist  = 0;
		}
		sd->method = HTTPD_METHOD_POST;

		// URL �f�R�[�h
		if( !httpd_decode_url( req - 1 ) ) return 400; // Bad Request

		if(content_len <= 0 || content_len >= 32*1024) {
			// �Ƃ肠����32KB�ȏ�̃��N�G�X�g�͕s������
			return ( content_len==0 )? 411 : ( content_len >= 32*1024 )? 413 : 400;
		}
		// �A�N�Z�X����̃`�F�b�N
		if( !httpd_check_access( sd, &status ) )
			return status;

		sd->query      =  RFIFOP(sd->fd,sd->header_len);
		sd->header_len += content_len;
		if(RFIFOREST(sd->fd) >= sd->header_len) {
			unsigned char temp = RFIFOB(sd->fd,sd->header_len);
			RFIFOB(sd->fd,sd->header_len) = 0;
			httpd_parse_request_ok(sd);
			RFIFOB(sd->fd,sd->header_len) = temp;
		} else {
			// POST�̃f�[�^�������Ă���̂�҂�
			sd->status = HTTPD_REQUEST_WAIT_POST;
		}
	} else {
		return 400; // Bad Request
	}
	return 200; // Ok
}

// ==========================================
// ���N�G�X�g�̉�͊������y�[�W����������
// ------------------------------------------
void httpd_parse_request_ok(struct httpd_session_data *sd) {
	void (*httpd_parse_func)(struct httpd_session_data*,const char*);
	sd->status = HTTPD_REQUEST_OK;

	// �t�@�C���������܂����̂ŁA�y�[�W����������������
	// printf("httpd_parse: [% 3d] request /%s\n",fd,req);
	httpd_parse_func = strdb_search(httpd_files,sd->url);
	if(httpd_parse_func == NULL) {
		httpd_parse_func = httpd_default;
	}
	if(httpd_parse_func == NULL) {
		httpd_send_error(sd,404); // Not Found
	} else {
		httpd_parse_func(sd,sd->url);
		if(sd->status == HTTPD_REQUEST_OK) {
			httpd_send_error(sd,404); // Not Found
		}
	}
	if(sd->persist == 1 && sd->data_len && sd->status!=HTTPD_SENDING_BIGFILE ) {
		// �������ςȃf�[�^(����Ȃ̑���Ȃ�c)
		printf("httpd_parse: send size mismatch when parsing /%s\n",sd->url);
		session[sd->fd]->eof = 1;
	}
	if(sd->status == HTTPD_REQUEST_OK) {
		httpd_send_error(sd,404); 
	}
}

// ==========================================
// CGI �N�G���̒l�擾
// ------------------------------------------
char* httpd_get_value(struct httpd_session_data* sd,const char* val) {
	int src_len = strlen(val);
	const unsigned char* src_p = sd->query;
	if(src_p == NULL) return aStrdup("");

	do {
		if(!memcmp(src_p,val,src_len) && src_p[src_len] == '=') {
			break;
		}
		src_p = strchr(src_p + 1,'&');
		if(src_p) src_p++;
	} while(src_p);

	if(src_p != NULL) {
		// �ړI�̕������������
		const unsigned char* p2;
		int   dest_len;
		char* dest_p;
		src_p += src_len + 1;
		p2     = strchr(src_p,'&');
		if(p2 == NULL) {
			src_len = strlen(src_p);
		} else {
			src_len = (p2 - src_p);
		}
		dest_p   = aMalloc(src_len + 1);
		dest_len = 0;
		while(src_len > 0) {
			if(*src_p == '%' && src_len > 2) {
				int c1 = 0,c2 = 0;
				if(src_p[1] >= '0' && src_p[1] <= '9') c1 = src_p[1] - '0';
				if(src_p[1] >= 'A' && src_p[1] <= 'F') c1 = src_p[1] - 'A' + 10;
				if(src_p[1] >= 'a' && src_p[1] <= 'f') c1 = src_p[1] - 'a' + 10;
				if(src_p[2] >= '0' && src_p[2] <= '9') c2 = src_p[2] - '0';
				if(src_p[2] >= 'A' && src_p[2] <= 'F') c2 = src_p[2] - 'A' + 10;
				if(src_p[2] >= 'a' && src_p[2] <= 'f') c2 = src_p[2] - 'a' + 10;
				dest_p[dest_len++] = (c1 << 4) | c2;
				src_p += 3; src_len -= 3;
			} else if(*src_p == '+') {
				dest_p[dest_len++] = ' ';
				src_p++; src_len--;
			} else {
				dest_p[dest_len++] = *(src_p++); src_len--;
			}
		}
		dest_p[dest_len] = 0;
		return dest_p;
	}
	return aStrdup("");
}

// ==========================================
// ���\�b�h��Ԃ�
// ------------------------------------------
int httpd_get_method(struct httpd_session_data* sd) {
	return sd->method;
}

// ==========================================
// IP �A�h���X��Ԃ�
// ------------------------------------------
unsigned int httpd_get_ip(struct httpd_session_data *sd) {
	return *(unsigned int*)(&session[sd->fd]->client_addr.sin_addr);
}

// ==========================================
// MIME�^�C�v����
//  ��v�Ȃ��̂������肵�āA�c���application/octet-stream
// ------------------------------------------
static const char* httpd_mimetype(const char* url) {
	char *ext = strrchr(url,'.');
	if(ext) {
		if(!strcmp(ext,".html")) return "text/html";
		if(!strcmp(ext,".htm"))  return "text/html";
		if(!strcmp(ext,".css"))  return "text/css";
		if(!strcmp(ext,".js"))   return "text/javascript";
		if(!strcmp(ext,".txt"))  return "text/plain";
		if(!strcmp(ext,".gif"))  return "image/gif";
		if(!strcmp(ext,".jpg"))  return "image/jpeg";
		if(!strcmp(ext,".jpeg")) return "image/jpeg";
		if(!strcmp(ext,".png"))  return "image/png";
		if(!strcmp(ext,".xbm"))  return "image/xbm";
		if(!strcmp(ext,".zip"))  return "application/zip";
	}
	return "application/octet-stream";
}

// ==========================================
// �y�[�W���� - �ʏ�̃t�@�C�����M
// ------------------------------------------
void httpd_send_file(struct httpd_session_data* sd,const char* url) {
	FILE *fp;
	int  file_size;
	char file_buf[8192];
	
	if(sd->status != HTTPD_REQUEST_OK) return;

	// URL �������ꍇ�̃f�t�H���g
	if(url[0] == '\0') url = "index.html";
	
	// url �̍ő咷�͖�1010�o�C�g�Ȃ̂ŁA�o�b�t�@�I�[�o�[�t���[�̐S�z�͖���
	sprintf(file_buf,"%s%s",document_root,url);

	// ���W���[���\(Accept-Ranges �̒ʒm)
	sd->reshead_flag |= HTTPD_RESHEAD_ACCRANGE;

	// ���t�m�F
	{
		time_t date = 0;
#if defined(_WIN32) && defined(_MSC_VER)
		struct _stat st;
		if( _stat( file_buf, &st ) == 0 )
#else
		struct stat st;
		if( stat( file_buf, &st ) == 0 )
#endif
			date = st.st_mtime;
		
		if( date!=0 && sd->precond == HTTPD_PRECOND_IFMOD   && date == sd->date )	// If-Modified-Since �̏���
		{
			httpd_send_head( sd, 304, NULL, -1);	// 304 �̓G���e�B�e�B�������Ȃ�
			sd->data_len = 0;
			sd->status = HTTPD_WAITING_SEND;
			return;
		}
		
		if( sd->precond == HTTPD_PRECOND_IFUNMOD && (date != sd->date || date==0) )	// If-Unmodified-Since �̏���
		{
			httpd_send_error( sd, 412 );
			return;
		}
		
		if( sd->precond == HTTPD_PRECOND_IFRANGE && (date != sd->date || date==0) )	// If-Range �̏���
		{
			sd->range_start = 0;
			sd->range_end   = -1;
		}
		
		sd->reshead_flag |= HTTPD_RESHEAD_LASTMOD;
		sd->date = date;

	} // end ���t�m�F


	// �t�@�C���̑��M
	fp = fopen(file_buf,"rb");
	if(fp == NULL) {
		httpd_send_error(sd,404);
	} else {
		int status = 200;
		fseek(fp,0,SEEK_END);
		file_size = ftell(fp);

		if( sd->range_start!=0 )	// Range �̊J�n�ʒu�`�F�b�N
		{
			if( sd->range_start<0 ) sd->range_start += file_size;
			if( sd->range_start<0 )	// �J�n�n�_����
			{
				httpd_send_error( sd,416 );
				return;
			}
			status = 206;
		}
		if( sd->range_end==-1 )		// Range �̏I���ʒu�C��
			sd->range_end += file_size;
		else
		{
			status = 206;
			if( sd->range_end>file_size )
				sd->range_end = file_size-1;
		}

		sd->inst_len = file_size;
		file_size = sd->range_end - sd->range_start + 1;	// content-lenth �v�Z

		if( file_size<0 )	// �T�C�Y����
		{
			httpd_send_error( sd,416 );
			return;
		}


		if( file_size > bigfile_threshold )		// �傫�ȃt�@�C���͕����]��
		{
			fclose( fp );
			realloc_fifo( sd->fd, -1, bigfile_splitsize );
			httpd_send_head(sd,status,httpd_mimetype(url),file_size);
			sd->status = HTTPD_SENDING_BIGFILE;
			sd->file_pos = sd->range_start;
			httpd_send_bigfile( sd );
		}
		else		// �����ȃt�@�C���� FIFO �Ɉ�C�ɑ���
		{
			realloc_fifo( sd->fd, -1, file_size + 32768 );
			httpd_send_head(sd,status,httpd_mimetype(url),file_size);
			fseek( fp, sd->range_start, SEEK_SET );
			while(file_size > 0) {
				int read_byte = file_size;
				if(file_size > 8192) read_byte = 8192;
				if( fread(file_buf,1,read_byte,fp) != read_byte )
				{
					session[sd->fd]->eof = 1;	// �ǂݍ��݃G���[���N�����̂Őؒf
					return;
				}
				httpd_send_data(sd,read_byte,file_buf);
				file_size -= read_byte;
			}
			fclose(fp);
		}
	}

}

// ==========================================
// �y�[�W���� - ����t�@�C�����M
// ------------------------------------------
void httpd_send_bigfile( struct httpd_session_data* sd )
{
	char file_buf[1024];
	const char* url = sd->url;
	FILE *fp;
	
	if( WFIFOSPACE( sd->fd ) < 64*1024 )	// �o�b�t�@�̋󂫂����Ȃ��̂ŋA��
		return;
	
	if(url[0] == '\0') url = "index.html";	// URL �������ꍇ�̃f�t�H���g
	
	// url �̍ő咷�͖�1010�o�C�g�Ȃ̂ŁA�o�b�t�@�I�[�o�[�t���[�̐S�z�͖���
	sprintf(file_buf,"%s%s",document_root,url);

	// ���t�m�F
	if( sd->date )
	{
		time_t date = 0;
#if defined(_WIN32) && defined(_MSC_VER)
		struct _stat st;
		if( _stat( file_buf, &st ) == 0 )
#else
		struct stat st;
		if( stat( file_buf, &st ) == 0 )
#endif
			date = st.st_mtime;

		if( date != sd->date )	// �]�����Ƀt�@�C�����X�V���ꂽ�̂Őؒf
		{
			session[sd->fd]->eof = 1;
			return;
		}
	}

	// �t�@�C���̑��M
	fp = fopen(file_buf,"rb");
	if(fp == NULL) {
		session[sd->fd]->eof = 1;
		return;
	} else {
		int send_size = WFIFOSPACE( sd->fd ) - 32768;
		if( send_size > sd->data_len )
			send_size = sd->data_len;
		
		fseek( fp, sd->file_pos, SEEK_SET );
		if( fread( WFIFOP(sd->fd,0), 1, send_size, fp)!= send_size )
		{
			session[sd->fd]->eof = 1;	// �t�@�C���ǂݍ��݂ŃG���[�����������̂Őؒf
			return;
		}
		WFIFOSET( sd->fd, send_size );
		sd->data_len -= send_size;
		sd->file_pos = ftell( fp );
		fclose(fp);
	}
	if( sd->data_len==0 )
		sd->status = HTTPD_WAITING_SEND;
}

// ==========================================
// URL �G���R�[�h
// ------------------------------------------
char* httpd_binary_encode(const char* val) {
	char *buf = aMalloc(strlen(val) * 3 + 1);
	char *p   = buf;
	while(*val) {
		if(isalnum((unsigned char)*val)) {
			*(p++) = *(val++);
		} else {
			unsigned char c1 = *(val++);
			unsigned char c2 = (c1 >> 4);
			unsigned char c3 = (c1 & 0x0F);
			*(p++) = '%';
			*(p++) = c2 + (c2 >= 10 ? 'A'-10 : '0');
			*(p++) = c3 + (c3 >= 10 ? 'A'-10 : '0');
		}
	}
	*p = 0;
	return buf;
}

// ==========================================
// http �̃��^�����̃N�H�[�g
// ------------------------------------------
char* httpd_quote_meta(const char* p1) {
	char *buf = aMalloc(strlen(p1) * 6 + 1);
	char *p2  = buf;
	while(*p1) {
		switch(*p1) {
		case '<': memcpy(p2,"&lt;",4);   p2 += 4; p1++; break;
		case '>': memcpy(p2,"&gt;",4);   p2 += 4; p1++; break;
		case '&': memcpy(p2,"&amp;",5);  p2 += 5; p1++; break;
		case '"': memcpy(p2,"&quot;",6); p2 += 6; p1++; break;
		default: *(p2++) = *(p1++);
		}
	}
	*p2 = 0;
	return buf;
}

// ==========================================
// �R���t�B�O - �F�؃��[�U�[�̒ǉ�
// ------------------------------------------
void httpd_config_read_add_authuser( struct httpd_access *a, const char *name, const char *passwd )
{
	struct httpd_access_user* au;
	int type;

	if( strlen(name) >= sizeof(a->user->name) )
	{
		printf("httpd_config_read: authuser: too long name [%s]\n", name);
		return;
	}

	if( memcmp( passwd, "$MD5$", 5 )==0 && strlen( passwd )==32+5 )		// MD5 �p�X���[�h
	{
		type = HTTPD_USER_PASSWD_MD5;
		passwd += 5;
	}
	else if( memcmp( passwd, "$Digest$", 8 )==0 && strlen( passwd )==32+8 )	// Digest �F�؂� A1
	{
		type = HTTPD_USER_PASSWD_DIGEST;
		passwd += 8;
	}
	else	// �v���[��
	{
		if( strlen( passwd ) >= sizeof( au->passwd ) )
		{
			printf("httpd_config_read: authuser: too long passwd (user[%s])\n", name);
			return;
		}
		
		type = HTTPD_USER_PASSWD_PLAIN;
	}

	// �K�v�Ȃ烁�������g��
	if( a->user_count == a->user_max )
	{
		struct httpd_access_user* au = aMalloc( sizeof(struct httpd_access_user) * (a->user_max + 16) );
		if( a->user )
		{
			memcpy( au, a->user, sizeof(struct httpd_access_user) * a->user_count );
			aFree( a->user );
		}
		a->user = au;
		a->user_max += 16;
	}
	
	// ���[�U�[�ǉ�
	au = a->user + (a->user_count++);
	strcpy( au->name, name );
	strcpy( au->passwd, passwd );
	au->type = type;

	// Digest �F�؃��[�h�ŁA�v���[���p�X���[�h�������ꍇ�A���炩���� A1 ���v�Z���Ă���
	if( (a->type & HTTPD_ACCESS_AUTH_MASK) == HTTPD_ACCESS_AUTH_DIGEST && au->type == HTTPD_USER_PASSWD_PLAIN )
	{
		char buf[512];
		au->type = HTTPD_USER_PASSWD_DIGEST;
		sprintf( buf, "%s:%s:%s", name, a->realm, passwd );
		MD5_String( buf, au->passwd );
	}
}

// ==========================================
// �R���t�B�O - ����/�֎~ IP �̒ǉ�
// ------------------------------------------
void httpd_config_read_add_ip( unsigned int **list, int *count, int *max, const char *w2 )
{
	int i1,i2,i3,i4, m1,m2,m3,m4;
	unsigned int ip, mask;
	unsigned char *pip = (unsigned char *)&ip, *pmask = (unsigned char *)&mask;

	if( strcmpi( w2,"clear" ) == 0 )	// clear
	{
		aFree( *list );
		*list = NULL;
		*count = *max = 0;
	}
	else if( strcmpi( w2,"all") == 0 )	// all
	{
		ip = mask = 0;
	}
	else if( sscanf( w2, "%d.%d.%d.%d/%d.%d.%d.%d", &i1, &i2, &i3, &i4, &m1, &m2, &m3, &m4 )==8 )	// 192.168.0.0/255.255.255.0 �`��
	{
		pip[0] = i1;		pip[1] = i2;		pip[2] = i3;		pip[3] = i4;
		pmask[0] = m1;		pmask[1] = m2;		pmask[2] = m3;		pmask[3] = m4;
	}
	else if( sscanf( w2,"%d.%d.%d.%d/%d", &i1, &i2, &i3, &i4, &m1 )==5 )	// 192.168.0.0/24 �`��
	{
		int mask = 0xffffffff << (32-m1);
		pip[0] = i1;		pip[1] = i2;		pip[2] = i3;		pip[3] = i4;
		pmask[0] = (mask>>24);		pmask[1] = (mask>>16);
		pmask[2] = (mask>>8);		pmask[3] = mask;
	}
	else if( sscanf( w2,"%d.%d.%d.%d", &i1, &i2, &i3, &i4 )==4 )	// 192.168.0.1 �`�� (�T�u�l�b�g�}�X�N 255.255.255.255 )
	{
		pip[0] = i1;		pip[1] = i2;		pip[2] = i3;		pip[3] = i4;
		pmask[0] = 0xff;	pmask[1] = 0xff;	pmask[2] = 0xff;	pmask[3] = 0xff;
	}
	else if( sscanf( w2,"%d.%d.%d", &i1, &i2, &i3 )==3 )	// 192.168.0 �`���i�T�u�l�b�g�}�X�N 255.255.255.0�j
	{
		pip[0] = i1;		pip[1] = i2;		pip[2] = i3;		pip[3] = 0;
		pmask[0] = 0xff;	pmask[1] = 0xff;	pmask[2] = 0xff;	pmask[3] = 0;
	}
	else if( sscanf( w2,"%d.%d", &i1, &i2 )==2 )	// 192.168 �`���i�T�u�l�b�g�}�X�N 255.255.0.0�j
	{
		pip[0] = i1;		pip[1] = i2;		pip[2] = 0;			pip[3] = 0;
		pmask[0] = 0xff;	pmask[1] = 0xff;	pmask[2] = 0;		pmask[3] = 0;
	}
	else	// 192 �`���i�T�u�l�b�g�}�X�N 255.0.0.0 �j
	{
		pip[0] = i1;		pip[1] = 0;			pip[2] = 0;			pip[3] = 0;
		pmask[0] = 0xff;	pmask[1] = 0;		pmask[2] = 0;		pmask[3] = 0;
	}

	// �K�v�Ȃ烁�������g��
	if( *count == *max )
	{
		unsigned int *iplist = aMalloc( sizeof(unsigned int) * (*max + 16) );
		if( *list )
		{
			memcpy( iplist, *list, sizeof(unsigned int) * (*count) );
			aFree( *list );
		}
		*list = iplist;
		*max += 16;
	}
	
	// IP �ƃ}�X�N�ǉ�
	(*list)[ (*count)++ ] = ip&mask;
	(*list)[ (*count)++ ] = mask;
}

// ==========================================
// �ݒ�t�@�C����ǂݍ���
// ------------------------------------------
int httpd_config_read(char *cfgName)
{
	int i;
	char line[1024],w1[1024],w2[1024];
	FILE *fp;
	struct httpd_access *a = NULL;

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

		if(strcmpi(w1,"enable")==0)
		{
			socket_enable_httpd( atoi(w2) );
		}
		else if(strcmpi(w1,"log_filename")==0)
		{
			strcpy( logfile, w2 );
		}
		else if(strcmpi(w1,"request_timeout_first")==0)
		{
			httpd_set_request_timeout( 0, atoi(w2) );
		}
		else if(strcmpi(w1,"request_timeout_persist")==0)
		{
			httpd_set_request_timeout( 1, atoi(w2) );
		}
		else if(strcmpi(w1,"max_persist_requests")==0)
		{
			httpd_set_max_persist_requests( atoi(w2) );
		}
		else if(strcmpi(w1,"timezone")==0)
		{
			if( strcmpi(w2,"auto") == 0)
				httpd_set_timezone( -1 );
			else
				httpd_set_timezone( atoi(w2)*(-60) );
		}
		else if(strcmpi(w1,"auth_digest_period")==0)
		{
			httpd_set_auth_digest_period( atoi(w2) );
		}
		else if(strcmpi(w1,"log_no_flush")==0)
		{
			log_no_flush = atoi(w2);
		}
		else if(strcmpi(w1,"max_uri_length")==0)
		{
			max_uri_length = atoi(w2);
		}
		else if(strcmpi(w1,"server_max_requests_per_second")==0)
		{
			server_max_requests_per_second = atoi(w2);
		}
		else if(strcmpi(w1,"server_max_requests_period")==0)
		{
			server_max_requests_period = atoi(w2);
		}
		else if(strcmpi(w1,"document_root")==0)
		{
			httpd_set_document_root( w2 );
		}
		else if( strcmpi(w1,"target")==0 )
		{
			if( strcmpi( w2,"clear" )==0 )		// clear
			{
				int i;
				for( i=0; i<htaccess_count; i++ )
				{
					if(htaccess[i]->dip)  aFree( htaccess[i]->dip );
					if(htaccess[i]->aip)  aFree( htaccess[i]->aip );
					if(htaccess[i]->user) aFree( htaccess[i]->user );
					aFree( htaccess[i] );
				}
				aFree( htaccess );
				htaccess = NULL;
				htaccess_count = htaccess_max = 0;
			}
			else if( *w2!='/' )		// '/' ����n�܂��Ă��Ȃ�
			{
				printf("httpd_config_read: target url must start root (/).\n");
			}
			else
			{
				// �����̃f�[�^�̕ύX���ǂ���������
				int i;
				for( i=0; i<htaccess_count; i++ )
				{
					if( strcmp( htaccess[i]->url, w2 ) == 0 )
					{
						a = htaccess[i];
						break;
					}
				}
				
				if( i==htaccess_count )				// �V�K�쐬
				{
					// �K�v�Ȃ烁�������g��
					int j;
					if( htaccess_count==htaccess_max )
					{
						struct httpd_access **a = aMalloc( sizeof( struct httpd_access* ) * htaccess_max+16 );
						if( htaccess )
						{
							memcpy( a, htaccess, sizeof( struct httpd_access* ) * htaccess_count );
							aFree( htaccess );
						}
						htaccess = a;
						htaccess_max += 16;
					}
					// �f�[�^�̒ǉ���������
					a = htaccess[ htaccess_count++ ] = aMalloc( sizeof( struct httpd_access ) ) ;
					a->type = HTTPD_ACCESS_ALWAYS;
					a->aip_count = a->aip_max = 0;
					a->dip_count = a->dip_max = 0;
					a->user_count= a->user_max= 0;
					a->aip = a->dip = NULL;
					a->user = NULL;
					a->url[0]= '\0';
					a->urllen = 0;
					strcpy( a->realm, "Athena Authorization" );
					if( strlen(w2)>=sizeof(a->url) )
						printf("httpd_config_read: too long target url [%s]\n", w2);
					else
					{
						strcpy( a->url, w2 );
						a->urllen = strlen(a->url);
					}
					
					// digest �F�ؗp�̃v���C�x�[�g�L�[�쐬
					for( j=0; j<rand()%10+20; j++ )
					{
						a->privkey[j] = (rand()%250)+1;
					}
					a->privkey[j]='\0';
				}
			}
		}
		else if(strcmpi(w1,"satisfy")==0 )
		{
			int i = (strcmpi(w2,"any")==0)? HTTPD_ACCESS_SAT_ANY : (strcmpi(w2,"all")==0)? HTTPD_ACCESS_SAT_ALL : -1;
			if( i<0 )
				printf("httpd_config_read: satisfy: unknown satisfy option [%s]\n", w2);
			else
				a->type = (a->type & ~HTTPD_ACCESS_SAT_MASK) | i;
		}
		else if(strcmpi(w1,"authtype")==0)
		{
			int i = (strcmpi(w2,"basic")==0)? HTTPD_ACCESS_AUTH_BASIC :
					(strcmpi(w2,"digest")==0)? HTTPD_ACCESS_AUTH_DIGEST :
					(strcmpi(w2,"none")==0)? HTTPD_ACCESS_AUTH_NONE : -1;
			if( i<0 )
				printf("httpd_config_read: authtype: unknown authtype [%s]\n", w2);
			else
				a->type = (a->type & ~HTTPD_ACCESS_AUTH_MASK) | i;
		}
		else if(strcmpi(w1,"authname")==0 || strcmpi(w1,"authrealm")==0 )
		{
			if( strlen(w2)>=sizeof(a->realm) )
				printf("httpd_config_read: %s: too long realm [%s]\n", w1, w2);
			else
				strcpy( a->realm, w2 );
		}
		else if(strcmpi(w1,"authuser")==0 )
		{
			char u[1024], p[1024];
			if( sscanf(w2,"%[^:]:%s",u,p) == 2 )
			{
				httpd_config_read_add_authuser( a, u, p );
			}
			else if( strcmpi(w2,"clear") == 2 )
			{
				aFree( a->user );
				a->user_count = a->user_max = 0;
			}
			else	// �O���t�@�C���ǂݍ��߂��肷��Ƃ�����������
			{
				printf("httpd_config_read: authuser: [user:pass] needed\n");
			}
		}
		else if(strcmpi(w1,"order")==0 )
		{
			int i = (strcmpi(w2,"deny,allow"    )==0)? HTTPD_ACCESS_IP_DENY   :
					(strcmpi(w2,"allow,deny"    )==0)? HTTPD_ACCESS_IP_ALLOW  :
					(strcmpi(w2,"mutual-failure")==0)? HTTPD_ACCESS_IP_MUTUAL :
					(strcmpi(w2,"none"			)==0)? HTTPD_ACCESS_IP_NONE   : -1;
			if( i<0 )
				printf("httpd_config_read: order: unknown order option [%s]\n", w2);
			else
				a->type = (a->type & ~HTTPD_ACCESS_IP_MASK) | i;
		}
		else if(strcmpi(w1,"allow")==0 )
		{
			httpd_config_read_add_ip( &a->aip, &a->aip_count, &a->aip_max, w2 );
		}
		else if(strcmpi(w1,"deny")==0 )
		{
			httpd_config_read_add_ip( &a->dip, &a->dip_count, &a->dip_max, w2 );
		}
		else if(strcmpi(w1,"import")==0)
		{
			httpd_config_read(w2);
		}
		else
		{
			printf("httpd_config_read: unknown option [%s: %s]\n",w1,w2);
		}
	}
	fclose(fp);

	return 0;
}

// ==========================================
// Base64 �f�R�[�h�p�w���p
// ------------------------------------------
static int httpd_decode_base64_code2value(unsigned char c)
{
	if( (c >= 'A') && (c <= 'Z') ) {
		return c - 'A';
	}
	else if( (c >= 'a') && (c <= 'z') ) {
		return (c - 'a') +26;
	}
	else if( (c >= '0') && (c <= '9') ) {
		return (c - '0') +52;
	}
	else if( '+' == c ) {
		return 62;
	}
	else if( '/' == c ) {
		return 63;
	}
	else if( '=' == c ) {
		return 0;
	}
	else {
		return -1;
	}
}

// ==========================================
// Base64 �f�R�[�h
// ------------------------------------------
int httpd_decode_base64( char *dest, const char *src)
{
	int c=0;

	while( *src && *src>0x20 && c>=0 )
	{
		int j;
		int b=0;

		for( j=0; j<4; j++, src++ )
		{
			c=httpd_decode_base64_code2value( *src );
			b = ( b<<6 ) | ((c<0)?0:c);  
		}
		*(dest++) = b>>16;
		*(dest++) = b>>8;
		*(dest++) = b;
	}
	*dest = '\0';
	return 1;
}
