
#include "map.h"
#include "ranking.h"
#include "nullpo.h"
#include "clif.h"
#include <stdlib.h>

struct Ranking_Data ranking_data[MAX_RANKING][MAX_RANKER];

char ranking_title[][64] = 
{
	"�u���b�N�X�~�X�����L���O",
	"�A���P�~�X�g�����L���O",
	"�e�R�������J�[",	
};

char ranking_reg[][32] = 
{
	"PC_BLACKSMITH_POINT",
	"PC_ALCHEMIST_POINT",
	"PC_TAEKWON_POINT",	
};

//PC�̃����L���O��Ԃ�
// 0 : �����N�O
int ranking_get_pc_rank(struct map_session_data * sd,int ranking_id)
{
	int i;
	
	nullpo_retr(0, sd);
	
	//�����L���O�Ώۂ��Ȃ�
	if(ranking_id<0 || MAX_RANKING <= ranking_id)
		return 0;

	for(i = 0;i<MAX_RANKER;i++)
	{
		if(sd->status.char_id == ranking_data[ranking_id][i].char_id)
			return i+1;
	}
	
	return 0;
}

//id���烉���L���O�����߂�
// 0 : �����N�O
int ranking_get_id2rank(int char_id,int ranking_id)
{	
	int i;
	
	if(char_id <=0)
		return 0;

	//�����L���O�Ώۂ��Ȃ�
	if(ranking_id<0 || MAX_RANKING <= ranking_id)
		return 0;

	for(i = 0;i<MAX_RANKER;i++)
	{
		if(ranking_data[ranking_id][i].char_id == char_id)
			return i+1;
	}
	
	return 0;
}


//PC�̃����L���O��Ԃ�
int ranking_get_point(struct map_session_data * sd,int ranking_id)
{
	int i;
	
	nullpo_retr(0, sd);
	
	//�����L���O�Ώۂ��Ȃ�
	if(ranking_id<0 || MAX_RANKING <= ranking_id)
		return 0;

	return sd->ranking_point[ranking_id];
}

int ranking_set_point(struct map_session_data * sd,int ranking_id,int point)
{
	int i;
	
	nullpo_retr(0, sd);
	
	//�����L���O�Ώۂ��Ȃ�
	if(ranking_id<0 || MAX_RANKING <= ranking_id)
		return 0;

	sd->ranking_point[ranking_id] = point;
	
	return 1;
}

int ranking_gain_point(struct map_session_data * sd,int ranking_id,int point)
{
	int i;
	
	nullpo_retr(0, sd);
	
	//�����L���O�Ώۂ��Ȃ�
	if(ranking_id<0 || MAX_RANKING <= ranking_id)
		return 0;

	sd->ranking_point[ranking_id] += point;
	
	return 1;
}

int ranking_setglobalreg(struct map_session_data * sd,int ranking_id)
{
	nullpo_retr(0, sd);
	
	//�����L���O�Ώۂ��Ȃ�
	if(ranking_id<0 || MAX_RANKING <= ranking_id)
		return 0;

	pc_setglobalreg(sd,ranking_reg[ranking_id],sd->ranking_point[ranking_id]);
	
	return 1;
}

int ranking_setglobalreg_all(struct map_session_data * sd)
{
	int i;
	nullpo_retr(0, sd);
	
	for(i = 0;i<MAX_RANKING;i++)
		ranking_setglobalreg(sd,i);
	
	return 1;
}

int ranking_update(struct map_session_data * sd,int ranking_id)
{
	
	int i=0,update_flag=0;
	
	nullpo_retr(0, sd);
	
	//�����L���O�Ώۂ��Ȃ�
	if(ranking_id<0 || MAX_RANKING <= ranking_id)
		return 0;

	//�T��
	for(i = 0;i<MAX_RANKER;i++)
	{
		//���Ƀ����J�[�Ȃ��point�X�V
		if(sd->status.char_id == ranking_data[ranking_id][i].char_id)
		{
			ranking_data[ranking_id][i].point = sd->ranking_point[ranking_id];
			update_flag = 1;
			break;
		}
	}
	
	//���ʂɂ͂Ȃ�����
	if(MAX_RANKER == i)
	{
		//�ŉ��ʂ�荂���_�Ȃ�ŉ��ʂɃ����N�C��
		if(ranking_data[ranking_id][i].point<sd->ranking_point[ranking_id])
		{
			strcpy(ranking_data[ranking_id][MAX_RANKER-1].name,sd->status.name);
			ranking_data[ranking_id][MAX_RANKER-1].point = sd->ranking_point[ranking_id];
			ranking_data[ranking_id][MAX_RANKER-1].char_id = sd->status.char_id;
			update_flag = 1;
		}
	}
	
	if(update_flag)
		ranking_sort(ranking_id);

	return 1;
}

//�I�����ɂł�
int ranking_update_all(struct map_session_data * sd)
{
	int i=0,update_flag=0;
	
	nullpo_retr(0, sd);

	//�T��
	for(i = 0;i<MAX_RANKING;i++)
		ranking_update(sd,i);

	return 1;
}

int compare_ranking_data(const struct Ranking_Data *a,const struct Ranking_Data *b )
{
	if((a->point - b->point)>0)
		return -1;
		
	if(a->point == b->point)
		return 0;

    return 1;
}

int ranking_sort(int ranking_id)
{
	int i;
	//�����L���O�Ώۂ��Ȃ�
	if(ranking_id<0 || MAX_RANKING <= ranking_id)
		return 0;
		
	qsort(ranking_data[ranking_id],MAX_RANKER,sizeof(struct Ranking_Data),(int (*)(const void*,const void*))compare_ranking_data);
	
	return 1;
}

int ranking_display(struct map_session_data * sd,int ranking_id,int begin,int end)
{
	int i;
	char output[128];
	nullpo_retr(0, sd);
	memset(output,0,sizeof(output));
	//�����L���O�Ώۂ��Ȃ�
	if(ranking_id<0 || MAX_RANKING <= ranking_id)
		return 0;
	if(begin<0) begin = 0;
	if(end>=MAX_RANKER) end = MAX_RANKER-1;
	
	clif_displaymessage( sd->fd, ranking_title[ranking_id]);
		
	for(i = begin;i<=end;i++)
	{
		if(ranking_data[ranking_id][i].char_id)
			sprintf(output,"%2d��:%s�l POINT:%d",i+1,ranking_data[ranking_id][i].name,ranking_data[ranking_id][i].point);
		else
			sprintf(output,"%2d��:�Y���҂Ȃ�",i+1);
		clif_displaymessage( sd->fd, output);
	}
	sprintf(output,"%s�l��POINT��%d�ł�",sd->status.name,sd->ranking_point[ranking_id]);
	clif_displaymessage(sd->fd, output);
	return 1;
}

int ranking_readreg(struct map_session_data * sd)
{
	int i;
	nullpo_retr(0, sd);
	for(i=0;i<MAX_RANKING;i++)
		sd->ranking_point[i] = pc_readglobalreg(sd,ranking_reg[i]);
	return 1;
}

int ranking_init_data()
{
	int i,j;
	for(i=0;i<MAX_RANKING;i++)
	for(j=0;j<MAX_RANKER;j++)
	{
		strcpy(ranking_data[i][j].name,"�Y���҂Ȃ�");
		ranking_data[i][j].point = 0;
		ranking_data[i][j].char_id = 0;
	}
	return 1;
}

//������
int do_init_ranking()
{
	ranking_init_data();
	
	return 1;	
}



