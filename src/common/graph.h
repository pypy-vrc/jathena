#ifndef _GRAPH_H_
#define _GRAPH_H_

void   do_init_graph(void);
struct graph* graph_create(unsigned int x,unsigned int y);
void graph_pallet(struct graph* g, int index,unsigned long c);
const unsigned char* graph_output(struct graph* g,int *len);
void graph_setpixel(struct graph* g,int x,int y,int color);
void graph_scroll(struct graph* g,int n,int color);
void graph_square(struct graph* g,int x,int y,int xe,int ye,int color);

// athena�̏�Ԃ𒲍�����Z���T�[��ǉ�����B
// string        : �Z���T�[�̖���(Login Users �Ȃ�)
// inetrval      : �Z���T�[�̒l����������Ԋu(msec)
// callback_func : �Z���T�[�̒l��Ԃ��֐�( unsigned int login_users(void); �Ȃ�) 

void graph_add_sensor(const char* string, int interval, double (*callback_func)(void));

#define graph_rgb(r,g,b) (((r) << 16) | ((g) << 8) | (b))

#endif

