#ifndef _RANKING_H_
#define _RANKING_H_
#include "map.h"

#define MAX_RANKING 10

int ranking_check_bsrank(int id);
int ranking_check_amrank(int id);
int ranking_check_tkrank(int id);

int ranking_get_bs_point(struct map_session_data * sd);
int ranking_get_am_point(struct map_session_data * sd);
int ranking_get_tk_point(struct map_session_data * sd);



#endif