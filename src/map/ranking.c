#include "ranking.h"
#include "nullpo.h"

int bs_ranking[MAX_RANKING];
int am_ranking[MAX_RANKING];
int tk_ranking[MAX_RANKING];

// BSのランキングを返す
// 0 : ランク外
int ranking_check_bsrank(int id)
{
	int i=0;
	if(id <= 0)
		return 0;

	for(i=0;i<;i++)
	{
		if(bs_ranking[i] == id)
			return i+1;
	}
	return 0;
}

// アルケミのランキングを返す
// 0 : ランク外
int ranking_check_amrank(int id)
{
	int i=0;
	if(id <= 0)
		return 0;

	for(i=0;i<;i++)
	{
		if(am_ranking[i] == id)
			return i+1;
	}
	return 0;
}

// テコンのランキングを返す
// 0 : ランク外
int ranking_check_tkrank(int id)
{
	int i=0;
	if(id <= 0)
		return 0;

	for(i=0;i<;i++)
	{
		if(tk_ranking[i] == id)
			return i+1;
	}
	return 0;
}

//BSのランキングポイントを取得する
int ranking_get_bs_point(struct map_session_data * sd)
{
	
	nullpo_retr(0, sd);
	return sd->bs_point;
}

//アルケミのランキングポイントを取得する
int ranking_get_am_point(struct map_session_data * sd)
{
	nullpo_retr(0, sd);
	return sd->am_point;
}

//テコンのランキングポイントを取得する
int ranking_get_tk_point(struct map_session_data * sd)
{
	nullpo_retr(0,sd);
	return sd->tk_point;
}
