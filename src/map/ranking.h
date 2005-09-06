#ifndef _RANKING_H_
#define _RANKING_H_

#include "map.h"

struct Ranking_Data
{
	char name[24];
	int  point;
	int  char_id;
};

int ranking_get_pc_rank(struct map_session_data * sd,int ranking_id);
int ranking_get_id2rank(int char_id,int ranking_id);

int ranking_get_point(struct map_session_data * sd,int ranking_id);
int ranking_set_point(struct map_session_data * sd,int ranking_id,int point);
int ranking_gain_point(struct map_session_data * sd,int ranking_id,int point);

int ranking_readreg(struct map_session_data * sd);
int ranking_setglobalreg(struct map_session_data * sd,int ranking_id);
int ranking_setglobalreg_all(struct map_session_data * sd);

int ranking_update(struct map_session_data * sd,int ranking_id);
int ranking_display(struct map_session_data * sd,int ranking_id,int begin,int end);

int ranking_update_all(struct map_session_data * sd);
int ranking_sort(int ranking_id);

//ランキング
enum {
	RK_BLACKSMITH 	= 0,//ブラックスミス
	RK_ALCHEMIST 	= 1,//アルケミスト
	RK_TAEKWON		= 2,//テコンランカー
};

int do_init_ranking();

#endif

