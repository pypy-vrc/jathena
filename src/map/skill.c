/* �X�L���֌W */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "timer.h"
#include "nullpo.h"
#include "malloc.h"

#include "mmo.h"
#include "guild.h"
#include "skill.h"
#include "map.h"
#include "clif.h"
#include "pc.h"
#include "pet.h"
#include "mob.h"
#include "battle.h"
#include "party.h"
#include "itemdb.h"
#include "script.h"
#include "intif.h"
#include "status.h"
#include "date.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define SKILLUNITTIMER_INVERVAL	100

/* �X�L���ԍ������X�e�[�^�X�ُ�ԍ��ϊ��e�[�u�� */
int SkillStatusChangeTable[]={	/* skill.h��enum��SC_***�Ƃ��킹�邱�� */
/* 0- */
	-1,-1,-1,-1,-1,-1,
	SC_PROVOKE,			/* �v���{�b�N */
	SC_MAGNUM,			/* �}�O�i���u���C�N */
	1,-1,
/* 10- */
	SC_SIGHT,			/* �T�C�g */
	-1,
	SC_SAFETYWALL,		/* �Z�[�t�e�B�[�E�H�[�� */
	-1,-1,
	SC_FREEZE,			/* �t���X�g�_�C�o�[ */
	SC_STONE,			/* �X�g�[���J�[�X */
	-1,-1,-1,
/* 20- */
	-1,-1,-1,-1,
	SC_RUWACH,			/* ���A�t */
	SC_PNEUMA,			/* �j���[�} */
	-1,-1,-1,
	SC_INCREASEAGI,		/* ���x���� */
/* 30- */
	SC_DECREASEAGI,		/* ���x���� */
	-1,
	SC_SIGNUMCRUCIS,	/* �V�O�i���N���V�X */
	SC_ANGELUS,			/* �G���W�F���X */
	SC_BLESSING,		/* �u���b�V���O */
	-1,-1,-1,-1,-1,
/* 40- */
	-1,-1,-1,-1,-1,
	SC_CONCENTRATE,		/* �W���͌��� */
	-1,-1,-1,-1,
/* 50- */
	-1,
	SC_HIDING,			/* �n�C�f�B���O */
	-1,-1,-1,-1,-1,-1,-1,-1,
/* 60- */
	SC_TWOHANDQUICKEN,	/* 2HQ */
	SC_AUTOCOUNTER,
	-1,-1,-1,-1,
	SC_IMPOSITIO,		/* �C���|�V�e�B�I�}�k�X */
	SC_SUFFRAGIUM,		/* �T�t���M�E�� */
	SC_ASPERSIO,		/* �A�X�y���V�I */
	SC_BENEDICTIO,		/* ���̍~�� */
/* 70- */
	-1,
	SC_SLOWPOISON,
	-1,
	SC_KYRIE,			/* �L���G�G���C�\�� */
	SC_MAGNIFICAT,		/* �}�O�j�t�B�J�[�g */
	SC_GLORIA,			/* �O�����A */
	SC_DIVINA,			/* ���b�N�X�f�B�r�[�i */
	-1,
	SC_AETERNA,			/* ���b�N�X�G�[�e���i */
	-1,
/* 80- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 90- */
	-1,-1,
	SC_QUAGMIRE,		/* �N�@�O�}�C�A */
	-1,-1,-1,-1,-1,-1,-1,
/* 100- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 110- */
	-1,
	SC_ADRENALINE,		/* �A�h���i�������b�V�� */
	SC_WEAPONPERFECTION,/* �E�F�|���p�[�t�F�N�V���� */
	SC_OVERTHRUST,		/* �I�[�o�[�g���X�g */
	SC_MAXIMIZEPOWER,	/* �}�L�V�}�C�Y�p���[ */
	-1,-1,-1,-1,-1,
/* 120- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 130- */
	-1,-1,-1,-1,-1,
	SC_CLOAKING,		/* �N���[�L���O */
	SC_STAN,			/* �\�j�b�N�u���[ */
	-1,
	SC_ENCPOISON,		/* �G���`�����g�|�C�Y�� */
	SC_POISONREACT,		/* �|�C�Y�����A�N�g */
/* 140- */
	SC_POISON,			/* �x�m���_�X�g */
	SC_SPLASHER,		/* �x�i���X�v���b�V���[ */
	-1,
	SC_TRICKDEAD,		/* ���񂾂ӂ� */
	-1,-1,SC_AUTOBERSERK,-1,-1,-1,
/* 150- */
	-1,-1,-1,-1,-1,
	SC_LOUD,			/* ���E�h�{�C�X */
	-1,
	SC_ENERGYCOAT,		/* �G�i�W�[�R�[�g */
	-1,-1,
/* 160- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,
	SC_SELFDESTRUCTION,
	-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,
	SC_KEEPING,
	-1,-1,
	SC_BARRIER,
	-1,-1,
	SC_HALLUCINATION,
	-1,-1,
/* 210- */
	-1,-1,-1,-1,-1,
	SC_STRIPWEAPON,
	SC_STRIPSHIELD,
	SC_STRIPARMOR,
	SC_STRIPHELM,
	-1,
/* 220- */
	SC_GRAFFITI,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 230- */
	-1,-1,-1,-1,
	SC_CP_WEAPON,
	SC_CP_SHIELD,
	SC_CP_ARMOR,
	SC_CP_HELM,
	-1,-1,
/* 240- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,
	SC_AUTOGUARD,
/* 250- */
	-1,-1,
	SC_REFLECTSHIELD,
	-1,-1,
	SC_DEVOTION,
	SC_PROVIDENCE,
	SC_DEFENDER,
	SC_SPEARSQUICKEN,
	-1,
/* 260- */
	-1,-1,-1,-1,-1,-1,-1,-1,
	SC_STEELBODY,
	SC_BLADESTOP_WAIT,
/* 270- */
	SC_EXPLOSIONSPIRITS,
	SC_EXTREMITYFIST,
	-1,-1,-1,-1,
	SC_MAGICROD,
	-1,-1,-1,
/* 280- */
	SC_FLAMELAUNCHER,
	SC_FROSTWEAPON,
	SC_LIGHTNINGLOADER,
	SC_SEISMICWEAPON,
	-1,
	SC_VOLCANO,
	SC_DELUGE,
	SC_VIOLENTGALE,
	-1,-1,
/* 290- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 300- */
	-1,-1,-1,-1,-1,-1,
	SC_LULLABY,
	SC_RICHMANKIM,
	SC_ETERNALCHAOS,
	SC_DRUMBATTLE,
/* 310- */
	SC_NIBELUNGEN,
	SC_ROKISWEIL,
	SC_INTOABYSS,
	SC_SIEGFRIED,
	-1,-1,-1,
	SC_DISSONANCE,
	-1,
	SC_WHISTLE,
/* 320- */
	SC_ASSNCROS,
	SC_POEMBRAGI,
	SC_APPLEIDUN,
	-1,-1,
	SC_UGLYDANCE,
	-1,
	SC_HUMMING,
	SC_DONTFORGETME,
	SC_FORTUNE,
/* 330- */
	SC_SERVICE4U,
	SC_SELFDESTRUCTION,
	-1,-1,-1,-1,-1,-1,-1,-1,
/* 340- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 350- */
	-1,-1,-1,-1,-1,
	SC_AURABLADE,
	SC_PARRYING,
	SC_CONCENTRATION,
	SC_TENSIONRELAX,
	SC_BERSERK,
/* 360- */
	-1,
	SC_ASSUMPTIO,
	SC_BASILICA,
	-1,-1,-1,
	SC_MAGICPOWER,
	-1,
	SC_SACRIFICE,
	SC_GOSPEL,
/* 370- */
	-1,-1,-1,-1,-1,-1,-1,-1,
	SC_EDP,
	-1,
/* 380- */
	SC_TRUESIGHT,
	-1,-1,
	SC_WINDWALK,
	SC_MELTDOWN,
	-1,-1,
	SC_CARTBOOST,
	-1,SC_CHASEWALK,
/* 390- */
	SC_REJECTSWORD,
	-1,-1,-1,-1,-1,
	SC_MARIONETTE,
	-1,
	SC_HEADCRUSH,
	SC_JOINTBEAT,
/* 400- */
	-1,-1,
	SC_MINDBREAKER,
	SC_MEMORIZE,
	-1,
	SC_SPIDERWEB,
	-1,-1,-1,-1,
/* 410- */
	-1,SC_RUN,SC_READYSTORM,-1,SC_READYDOWN,-1,SC_READYTURN,-1,SC_READYCOUNTER,-1,
/* 420- */
	SC_DODGE,-1,-1,-1,-1,SC_SEVENWIND,-1,-1,SC_SUN_WARM,SC_MOON_WARM,
/* 430- */
	SC_STAR_WARM,SC_SUN_COMFORT,SC_MOON_COMFORT,SC_STAR_COMFORT,-1,-1,-1,-1,-1,-1,
/* 440- */
	-1,-1,-1,-1,SC_FUSION,SC_ALCHEMIST,-1,SC_MONK,SC_STAR,SC_SAGE,
/* 450- */
	SC_CRUSADER,SC_SUPERNOVICE,SC_KNIGHT,SC_WIZARD,SC_PRIEST,SC_BARDDANCER,SC_ROGUE,SC_ASSASIN,SC_BLACKSMITH,SC_ADRENALINE2,
/* 460- */
	SC_HUNTER,SC_SOULLINKER,SC_KAIZEL,SC_KAAHI,SC_KAUPE,SC_KAITE,-1,-1,-1,SC_SMA,
/* 470- */
	SC_SWOO,SC_SKE,SC_SKA,-1,-1,SC_PRESERVE,-1,-1,-1,-1,
/* 480- */
	-1,-1,SC_DOUBLECASTING,-1,-1,-1,SC_OVERTHRUSTMAX,-1,-1,-1,
/* 490- */
	-1,-1,-1,-1,SC_HIGH,SC_ONEHAND,-1,-1,-1,-1,
};


/* (�X�L���ԍ� - GD_SKILLBASE)�����X�e�[�^�X�ُ�ԍ��ϊ��e�[�u�� */
/* */
int GuildSkillStatusChangeTable[]={	/* skill.h��enum��SC_***�Ƃ��킹�邱�� */
/* 0- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 10- */
	SC_BATTLEORDER,SC_REGENERATION,-1,-1,-1,-1,-1,-1,-1,-1,
};


/* (�X�L���ԍ� - GD_SKILLBASE)�����X�e�[�^�X�ُ�ԍ��ϊ��e�[�u�� */
/* */
int StatusIconChangeTable[] = {
/* 0- */
	SI_PROVOKE,SI_ENDURE,SI_TWOHANDQUICKEN,SI_CONCENTRATE,SI_HIDING,SI_CLOAKING,SI_ENCPOISON,SI_POISONREACT,SI_QUAGMIRE,SI_ANGELUS,
/* 10- */
	SI_BLESSING,SI_SIGNUMCRUCIS,SI_INCREASEAGI,SI_DECREASEAGI,SI_SLOWPOISON,SI_IMPOSITIO,SI_SUFFRAGIUM,SI_ASPERSIO,SI_BENEDICTIO,SI_KYRIE,
/* 20- */
	SI_MAGNIFICAT,SI_GLORIA,SI_AETERNA,SI_ADRENALINE,SI_WEAPONPERFECTION,SI_OVERTHRUST,SI_MAXIMIZEPOWER,SI_RIDING,SI_FALCON,SI_TRICKDEAD,
/* 30- */
	SI_LOUD,SI_ENERGYCOAT,SI_BLANK,SI_BLANK,SI_HALLUCINATION,SI_WEIGHT50,SI_WEIGHT90,SI_SPEEDPOTION0,SI_SPEEDPOTION1,SI_SPEEDPOTION2,
/* 40- */
	SI_SPEEDPOTION3,SI_INCREASEAGI2,SI_INCREASEAGI3,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 50- */
	SI_STRIPWEAPON,SI_STRIPSHIELD,SI_STRIPARMOR,SI_STRIPHELM,SI_CP_WEAPON,SI_CP_SHIELD,SI_CP_ARMOR,SI_CP_HELM,SI_AUTOGUARD,SI_REFLECTSHIELD,
/* 60- */
	SI_DEVOTION,SI_PROVIDENCE,SI_DEFENDER,SI_BLANK,SI_BLANK,SI_AUTOSPELL,SI_BLANK,SI_BLANK,SI_SPEARSQUICKEN,SI_BLANK,
/* 70- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 80- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_EXPLOSIONSPIRITS,SI_STEELBODY,SI_BLANK,SI_COMBO,
/* 90- */
	SI_FLAMELAUNCHER,SI_FROSTWEAPON,SI_LIGHTNINGLOADER,SI_SEISMICWEAPON,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 100- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_AURABLADE,SI_PARRYING,SI_CONCENTRATION,SI_TENSIONRELAX,SI_BERSERK,SI_BLANK,SI_BLANK,
/* 110- */
	SI_ASSUMPTIO,SI_BLANK,SI_BLANK,SI_MAGICPOWER,SI_EDP,SI_TRUESIGHT,SI_WINDWALK,SI_MELTDOWN,SI_CARTBOOST,SI_CHASEWALK,
/* 120- */
	SI_REJECTSWORD,SI_MARIONETTE,SI_BLANK,SI_BLANK,SI_HEADCRUSH,SI_JOINTBEAT,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 130- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 140- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 150- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_ELEMENTFIELD,SI_ELEMENTFIELD,SI_ELEMENTFIELD,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 160- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 170- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 180- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 190- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 200- */
	SI_BLANK,SI_PRESERVE,SI_OVERTHRUSTMAX,SI_CHASEWALK_STR,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 210- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 220- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 230- */
	SI_RUN,SI_BLANK,SI_BLANK,SI_DODGE,SI_BLANK,SI_BLANK,SI_BLANK,SI_SUN_WARM,SI_MOON_WARM,SI_STAR_WARM,
/* 240- */
	SI_SUN_COMFORT,SI_MOON_COMFORT,SI_STAR_COMFORT,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 250- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 260- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_KAIZEL,SI_KAAHI,SI_KAUPE,SI_KAITE,SI_BLANK,SI_BLANK,
/* 270- */
	SI_BLANK,SI_BLANK,SI_ONEHAND,SI_READYSTORM,SI_READYDOWN,SI_READYTURN,SI_READYCOUNTER,SI_BLANK,SI_BLANK,SI_DEVIL,
/* 280- */
	SI_DOUBLECASTING,SI_ELEMENTFIELD,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
};
	
static const int dirx[8]={0,-1,-1,-1,0,1,1,1};
static const int diry[8]={1,1,0,-1,-1,-1,0,1};

static int rdamage;

/* �X�L���f�[�^�x�[�X */
struct skill_db skill_db[MAX_SKILL_DB+MAX_GUILDSKILL_DB];

/* �A�C�e���쐬�f�[�^�x�[�X */
struct skill_produce_db skill_produce_db[MAX_SKILL_PRODUCE_DB];

/* ��쐬�X�L���f�[�^�x�[�X */
struct skill_arrow_db skill_arrow_db[MAX_SKILL_ARROW_DB];

/* �A�u���J�_�u�������X�L���f�[�^�x�[�X */
struct skill_abra_db skill_abra_db[MAX_SKILL_ABRA_DB];

/* �v���g�^�C�v */
int skill_check_condition( struct map_session_data *sd,int type);
int skill_castend_damage_id( struct block_list* src, struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag );
int skill_frostjoke_scream(struct block_list *bl,va_list ap);
int skill_attack_area(struct block_list *bl,va_list ap);
int skill_abra_dataset(int skilllv);
int skill_clear_element_field(struct block_list *bl);
int skill_landprotector(struct block_list *bl, va_list ap );
int skill_trap_splash(struct block_list *bl, va_list ap );
int skill_count_target(struct block_list *bl, va_list ap );
struct skill_unit_group_tickset *skill_unitgrouptickset_search(struct block_list *bl,struct skill_unit_group *sg,int tick);
int skill_unit_onplace(struct skill_unit *src,struct block_list *bl,unsigned int tick);
int skill_unit_effect(struct block_list *bl,va_list ap);
int skill_castend_delay (struct block_list* src, struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag);
int skill_castend_delay_sub (int tid, unsigned int tick, int id, int data);

static int distance(int x0,int y0,int x1,int y1)
{
	int dx,dy;

	dx=abs(x0-x1);
	dy=abs(y0-y1);
	return dx>dy ? dx : dy;
}
/* �X�L�����j�b�g�̔z�u����Ԃ� */
struct skill_unit_layout skill_unit_layout[MAX_SKILL_UNIT_LAYOUT];
int firewall_unit_pos;
int icewall_unit_pos;

struct skill_unit_layout *skill_get_unit_layout(int skillid,int skilllv,struct block_list *src,int x,int y)
{
	
	int pos = skill_get_unit_layout_type(skillid,skilllv);
	int dir;

	if (pos!=-1)
		return &skill_unit_layout[pos];

	if (src->x==x && src->y==y)
		dir = 2;
	else
		dir = map_calc_dir(src,x,y);

	if (skillid==MG_FIREWALL)
		return &skill_unit_layout[firewall_unit_pos+dir];
	else if (skillid==WZ_ICEWALL)
		return &skill_unit_layout[icewall_unit_pos+dir];

	printf("unknown unit layout for skill %d, %d\n",skillid,skilllv);
	return &skill_unit_layout[0];
}
/**/
/*
*/
int GetSkillStatusChangeTable(int id)
{
	if(id < GD_SKILLBASE)
		return SkillStatusChangeTable[id];
		
	return 	GuildSkillStatusChangeTable[id - GD_SKILLBASE];
}
int skill_get_hit(int id)
{
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].hit;
}
int skill_get_inf(int id)
{
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].inf;
}
int skill_get_pl(int id)
{		
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].pl;
}
int skill_get_nk(int id)
{		
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].nk;
}
int skill_get_max(int id)
{	
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].max;
}
int skill_get_range(int id,int lv)
{
	if(lv<=0)
		return 0;
		
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].range[lv-1];
}
int skill_get_hp(int id,int lv)
{
	if(lv<=0) 
		return 0;
		
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].hp[lv-1];
}
int skill_get_sp(int id,int lv)
{	
	if(lv<=0) 
		return 0;
		
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].sp[lv-1];
}
int skill_get_zeny(int id,int lv)
{
	if(lv<=0) 
		return 0;
	
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].zeny[lv-1];
}
int skill_get_num(int id,int lv)
{
	if(lv<=0) 
		return 0;
	
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].num[lv-1];
}
int skill_get_cast(int id,int lv)
{
	if(lv<=0) return 0;
	
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].cast[lv-1];
}
int skill_get_fixedcast(int id ,int lv)
{	
	if(lv<=0)
		return 0;

	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].fixedcast[lv-1];
}
int skill_get_delay(int id,int lv)
{
	if(lv<=0)
		return 0;
		
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].delay[lv-1];
}
int skill_get_time(int id ,int lv)
{
	if(lv<=0) return 0;
	
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].upkeep_time[lv-1];
}
int skill_get_time2(int id,int lv)
{	
	if(lv<=0) return 0;
	
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].upkeep_time2[lv-1];
}
int skill_get_castdef(int id)
{
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].cast_def_rate;
}
int skill_get_weapontype(int id)
{	
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].weapon;
}
int skill_get_inf2(int id)
{
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].inf2;
}
int skill_get_maxcount(int id)
{	
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].maxcount;
}
int skill_get_blewcount(int id,int lv)
{	
	if(lv<=0) return 0;
	
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].blewcount[lv-1];
}
int skill_get_unit_id(int id,int flag)
{
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].unit_id[flag];
}
int skill_get_unit_layout_type(int id,int lv)
{	
	if(lv<=0) return 0;
	
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].unit_layout_type[lv-1];
}
int skill_get_unit_interval(int id)
{
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].unit_interval;
}
int skill_get_unit_range(int id)
{
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].unit_range;
}
int skill_get_unit_target(int id)
{	
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].unit_target;
}
int skill_get_unit_flag(int id)
{	
	if(id >= GD_SKILLBASE)
		id = id -GD_SKILLBASE + MAX_SKILL_DB;
	return skill_db[id].unit_flag;
}
int skill_get_skilldb_id(int id)
{
	if(id>=GD_SKILLBASE)
		return id - GD_SKILLBASE + MAX_SKILL_DB;
	
	return id;
}
/*==========================================
 * �X�L���ǉ�����
 *------------------------------------------
 */
int skill_additional_effect( struct block_list* src, struct block_list *bl,int skillid,int skilllv,int attack_type,unsigned int tick)
{
	/* MOB�ǉ����ʃX�L���p */
	const int sc[]={
		SC_POISON, SC_BLIND, SC_SILENCE, SC_STAN,
		SC_STONE, SC_CURSE, SC_SLEEP
	};
	const int sc2[]={
		MG_STONECURSE,MG_FROSTDIVER,NPC_STUNATTACK,
		NPC_SLEEPATTACK,TF_POISON,NPC_CURSEATTACK,
		NPC_SILENCEATTACK,0,NPC_BLINDATTACK
	};

	struct map_session_data *sd=NULL;
	struct map_session_data *dstsd=NULL;
	struct mob_data *md=NULL;
	struct mob_data *dstmd=NULL;

	int skill,skill2;
	int rate,luk;

	int sc_def_mdef,sc_def_vit,sc_def_int,sc_def_luk;
	int sc_def_mdef2,sc_def_vit2,sc_def_int2,sc_def_luk2;

	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	if(skilllv < 0) return 0;

	//PC,MOB,PET�ȊO�͒ǉ����ʂ̑ΏۊO
	if(!(bl->type==BL_PC || bl->type==BL_MOB || bl->type ==BL_PET))
		return 0;

	if(src->type==BL_PC){
		nullpo_retr(0, sd=(struct map_session_data *)src);
	}else if(src->type==BL_MOB){
		nullpo_retr(0, md=(struct mob_data *)src); //���g�p�H
	}

	//�Ώۂ̑ϐ�
	luk = status_get_luk(bl);
	sc_def_mdef=100 - (3 + status_get_mdef(bl) + luk/3);
	sc_def_vit=100 - (3 + status_get_vit(bl) + luk/3);
	sc_def_int=100 - (3 + status_get_int(bl) + luk/3);
	sc_def_luk=100 - (3 + luk);
	//�����̑ϐ�
	luk = status_get_luk(src);
	sc_def_mdef2=100 - (3 + status_get_mdef(src) + luk/3);
	sc_def_vit2=100 - (3 + status_get_vit(src) + luk/3);
	sc_def_int2=100 - (3 + status_get_int(src) + luk/3);
	sc_def_luk2=100 - (3 + luk);
	if(bl->type==BL_PC)
		dstsd=(struct map_session_data *)bl;
	else if(bl->type==BL_MOB){
		dstmd=(struct mob_data *)bl; //���g�p�H
		if(sc_def_mdef<50)
			sc_def_mdef=50;
		if(sc_def_vit<50)
			sc_def_vit=50;
		if(sc_def_int<50)
			sc_def_int=50;
		if(sc_def_luk<50)
			sc_def_luk=50;
	}
	if(sc_def_mdef<0)
		sc_def_mdef=0;
	if(sc_def_vit<0)
		sc_def_vit=0;
	if(sc_def_int<0)
		sc_def_int=0;

/*�R���p�C���ˑ��̃o�O������܂��AGCC 3.3.0/3.3.1 �̐l�p*/
/*�R���p�C���Ōv�Z���O�^�O�̎��ɕϐ��̍ő�l����������o�O*/
/*������悤�ł��A�������v�Z�ɂȂ�Ȃ��ł����ǁA�_���[�W�v�Z*/
/*��X�e�[�^�X�ω����\�z�ȏ�ɓK�p����Ȃ��ꍇ�A�ȉ���L���ɂ��Ă�������*/
/*	if(sc_def_mdef<1)       */
/*		sc_def_mdef=1;  */
/*	if(sc_def_vit<1)        */
/*		sc_def_vit=1;   */
/*	if(sc_def_int<1)        */
/*		sc_def_int=1;   */

	switch(skillid){
	case 0:					/* �ʏ�U�� */
		/* ������ */
		if( sd && pc_isfalcon(sd) && sd->status.weapon == 11 && (skill=pc_checkskill(sd,HT_BLITZBEAT))>0 &&
			atn_rand()%1000 <= sd->paramc[5]*10/3+1 ) {
			int lv=(sd->status.job_level+9)/10;
			skill_castend_damage_id(src,bl,HT_BLITZBEAT,(skill<lv)?skill:lv,tick,0xf00000);
		}
		// �X�i�b�`���[
		if(sd && sd->status.weapon != 11 && (skill=pc_checkskill(sd,RG_SNATCHER)) > 0)
			if((skill*15 + 55) + (skill2 = pc_checkskill(sd,TF_STEAL))*10 > atn_rand()%1000) {
				if(pc_steal_item(sd,bl))
					clif_skill_nodamage(src,bl,TF_STEAL,skill2,1);
				else if (battle_config.display_snatcher_skill_fail)
					clif_skill_fail(sd,skillid,0,0);
		}
		// �G���`�����g�f�b�g���[�|�C�Y��(�ғŌ���)
		if (sd && sd->sc_data[SC_EDP].timer != -1 && atn_rand() % 10000 < sd->sc_data[SC_EDP].val2 * sc_def_vit) {
			int lvl = sd->sc_data[SC_EDP].val1;
			status_change_start(bl,SC_DPOISON,lvl,0,0,0,skill_get_time2(ASC_EDP,lvl),0);
		}
		// �����g�_�E��(����E�Z�j��)
		if (sd && sd->sc_data[SC_MELTDOWN].timer != -1) {
			if (atn_rand() % 100 < sd->sc_data[SC_MELTDOWN].val1) {
				// ����j��
				if (dstsd) {
					pc_break_equip(dstsd, EQP_WEAPON);
				} else {
					status_change_start(bl,SC_STRIPWEAPON,1,0,0,0,skill_get_time2(WS_MELTDOWN,sd->sc_data[SC_MELTDOWN].val1),0);
				}
			}
			if (atn_rand() % 1000 < sd->sc_data[SC_MELTDOWN].val1*7) {
				// �Z�j��
				if (dstsd) {
					pc_break_equip(dstsd, EQP_ARMOR);
				} else {
					status_change_start(bl,SC_STRIPARMOR,1,0,0,0,skill_get_time2(WS_MELTDOWN,sd->sc_data[SC_MELTDOWN].val1),0);
				}
			}
		}
		break;
	case SM_BASH:			/* �o�b�V���i�}���U���j */
		if( sd && (skill=pc_checkskill(sd,SM_FATALBLOW))>0 ){
			if( atn_rand()%100 < (5*(skilllv-5)+(sd->status.base_level/3))*sc_def_vit/100 )
				status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(SM_FATALBLOW,skilllv),0);
		}
		break;

	case TF_POISON:			/* �C���x�i�� */
	case AS_SPLASHER:		/* �x�i���X�v���b�V���[ */
		if(atn_rand()%100< (2*skilllv+10)*sc_def_vit/100 )
			status_change_start(bl,SC_POISON,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		else{
			if(sd && skillid==TF_POISON)
				clif_skill_fail(sd,skillid,0,0);
		}
		break;

	case AS_SONICBLOW:		/* �\�j�b�N�u���[ */
		if( atn_rand()%100 < (2*skilllv+10)*sc_def_vit/100 )
			status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case HT_FREEZINGTRAP:	/* �t���[�W���O�g���b�v */
		rate=skilllv*3+35;
		if(atn_rand()%100 < rate*sc_def_mdef/100)
			status_change_start(bl,SC_FREEZE,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case WZ_METEOR:		/* ���e�I�X�g�[�� */
		if(atn_rand()%100 < 3*skilllv)
			status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case WZ_VERMILION:		/* ���[�h�I�u���@�[�~���I�� */
		if(atn_rand()%100 < 4*skilllv)
			status_change_start(bl,SC_BLIND,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case WZ_FROSTNOVA:		/* �t���X�g�m���@ */
		{
			struct status_change *sc_data = status_get_sc_data(bl);
			rate = (skilllv*5+33)*sc_def_mdef/100-(status_get_int(bl)+status_get_luk(bl))/15;
			if (rate <= 5)
				rate = 5;
			if(sc_data && sc_data[SC_FREEZE].timer == -1 && atn_rand()%100 < rate)
				status_change_start(bl,SC_FREEZE,skilllv,0,0,0,skill_get_time2(skillid,skilllv)*(1-sc_def_mdef/100),0);
		}
		break;

	case WZ_STORMGUST:		/* �X�g�[���K�X�g */
		{
			struct status_change *sc_data = status_get_sc_data(bl);
			if(sc_data) {
				sc_data[SC_FREEZE].val3++;
				if(sc_data[SC_FREEZE].val3 >= 3)
					status_change_start(bl,SC_FREEZE,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
			}
		}
		break;

	case HT_LANDMINE:		/* �����h�}�C�� */
		if( atn_rand()%100 < (5*skilllv+30)*sc_def_vit/100 )
			status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case HT_SHOCKWAVE:				/* �V���b�N�E�F�[�u�g���b�v */
		if(map[bl->m].flag.pvp && dstsd){
			dstsd->status.sp -= dstsd->status.sp*(5+15*skilllv)/100;
			status_calc_pc(dstsd,0);
		}
		break;
	case HT_SANDMAN:		/* �T���h�}�� */
		if( atn_rand()%100 < (5*skilllv+30)*sc_def_int/100 )
			status_change_start(bl,SC_SLEEP,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case TF_SPRINKLESAND:	/* ���܂� */
		if( atn_rand()%100 < 20*sc_def_int/100 )
			status_change_start(bl,SC_BLIND,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case TF_THROWSTONE:		/* �Γ��� */
		if( atn_rand()%100 < 5*sc_def_vit/100 )
			status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case CR_HOLYCROSS:		/* �z�[���[�N���X */
		if( atn_rand()%100 < 3*skilllv*sc_def_int/100 )
			status_change_start(bl,SC_BLIND,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case CR_GRANDCROSS:		/* �O�����h�N���X */
	case NPC_DARKGRANDCROSS:	/*�ŃO�����h�N���X*/
		{
			int race = status_get_race(bl);
			if( (battle_check_undead(race,status_get_elem_type(bl)) || race == 6) && atn_rand()%100 < 100000*sc_def_int/100)	//�����t�^�������S�ϐ��ɂ͖���
				status_change_start(bl,SC_BLIND,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		}
		break;

	case CR_SHIELDCHARGE:		/* �V�[���h�`���[�W */
		if( atn_rand()%100 < (15 + skilllv*5)*sc_def_vit/100 )
			status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case RG_RAID:		/* �T�v���C�Y�A�^�b�N */
		if( atn_rand()%100 < (10+3*skilllv)*sc_def_vit/100 )
			status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		if( atn_rand()%100 < (10+3*skilllv)*sc_def_int/100 )
			status_change_start(bl,SC_BLIND,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case BA_FROSTJOKE:
		if(atn_rand()%100 < (15+5*skilllv)*sc_def_mdef/100)
			status_change_start(bl,SC_FREEZE,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case DC_SCREAM:
		if( atn_rand()%100 < (25+5*skilllv)*sc_def_vit/100 )
			status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case BD_LULLABY:	/* �q��S */
		if( atn_rand()%100 < 15*sc_def_int/100 )
			status_change_start(bl,SC_SLEEP,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	/* MOB�̒ǉ����ʕt���X�L�� */

	case NPC_PETRIFYATTACK:
		if(atn_rand()%100 < sc_def_mdef)
			status_change_start(bl,sc[skillid-NPC_POISON],skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case NPC_POISON:
	case NPC_SILENCEATTACK:
	case NPC_STUNATTACK:
		if(atn_rand()%100 < sc_def_vit)
			status_change_start(bl,sc[skillid-NPC_POISON],skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case NPC_CURSEATTACK:
		if(atn_rand()%100 < sc_def_luk)
			status_change_start(bl,sc[skillid-NPC_POISON],skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case NPC_SLEEPATTACK:
	case NPC_BLINDATTACK:
		if(atn_rand()%100 < sc_def_int)
			status_change_start(bl,sc[skillid-NPC_POISON],skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case NPC_MENTALBREAKER:
		if(dstsd) {
			int sp = dstsd->status.max_sp*(10+skilllv)/100;
			if(sp < 1) sp = 1;
			pc_heal(dstsd,0,-sp);
		}
		break;
	case NPC_BREAKARMOR:
		if(bl->type == BL_PC)
			pc_break_equip((struct map_session_data *)bl, EQP_ARMOR);
		break;
	case NPC_BREAKWEAPON:
		if(bl->type == BL_PC)
			pc_break_equip((struct map_session_data *)bl, EQP_WEAPON);
		break;
	case NPC_BREAKHELM:
		if(bl->type == BL_PC)
			pc_break_equip((struct map_session_data *)bl, EQP_HELM);
		break;
	case NPC_BREAKSIELD:
		if(bl->type == BL_PC)
			pc_break_equip((struct map_session_data *)bl, EQP_SHIELD);
		break;
	case LK_HEADCRUSH:				/* �w�b�h�N���b�V�� */
		{//�������ǂ�������Ȃ��̂œK����
			int race=status_get_race(bl);
			if( !(battle_check_undead(race,status_get_elem_type(bl)) || race == 6) && atn_rand()%100 < (2*skilllv+10)*sc_def_vit/100 )
				status_change_start(bl,SC_HEADCRUSH,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		}
			break;
	case LK_JOINTBEAT:				/* �W���C���g�r�[�g */
		//�������ǂ�������Ȃ��̂œK����
		if( atn_rand()%100 < (2*skilllv+10)*sc_def_vit/100 )
			status_change_start(bl,SC_JOINTBEAT,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case PF_SPIDERWEB:		/* �X�p�C�_�[�E�F�b�u */
		{
			int sec=skill_get_time2(skillid,skilllv);
			if(map[src->m].flag.pvp) //PvP�ł͍S�����Ԕ����H
				sec = sec/2;
			battle_stopwalking(bl,1);
			status_change_start(bl,SC_SPIDERWEB,skilllv,0,0,0,sec,0);
		}
		break;
	case ASC_METEORASSAULT:			/* ���e�I�A�T���g */
		if( atn_rand()%100 < (15 + skilllv*5)*sc_def_vit/100 ) //��Ԉُ�͏ڍׂ�������Ȃ��̂œK����
			status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		if( atn_rand()%100 < (10+3*skilllv)*sc_def_int/100 )
			status_change_start(bl,SC_BLIND,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case MO_EXTREMITYFIST:			/* ���C���e���� */
		//���C�����g����5���Ԏ��R�񕜂��Ȃ��悤�ɂȂ�
		status_change_start(src,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time2(skillid,skilllv),0 );
		break;
	case HW_NAPALMVULCAN:			/* �i�p�[���o���J�� */
		// skilllv*5%�̊m���Ŏ�
		if (atn_rand()%10000 < 5*skilllv*sc_def_luk)
			status_change_start(bl,SC_CURSE,7,0,0,0,skill_get_time2(NPC_CURSEATTACK,7),0);
		break;
	case PA_PRESSURE:	/* �v���b�V���[ */
		// �Ώۂ�15% + skilllv*5%��SP�U��(�K��)
		if(dstsd) {
			int sp = dstsd->status.sp*(15+5*skilllv)/100;
			pc_heal(dstsd,0,-sp);
		}
		break;
	case WS_CARTTERMINATION:
		// skilllv*5%�̊m���ŃX�^��
		if (atn_rand()%10000 < 5*skilllv*sc_def_luk)
			status_change_start(bl,SC_STAN,7,0,0,0,skill_get_time2(NPC_STUNATTACK,7),0);
		break;
	case CR_ACIDDEMONSTRATION:	/* �A�V�b�h�f�����X�g���[�V���� */
		if(bl->type == BL_PC && atn_rand()%100 <= skilllv) {
			pc_break_equip((struct map_session_data *)bl, EQP_WEAPON);
		}
		if(bl->type == BL_PC && atn_rand()%100 <= skilllv) {
			pc_break_equip((struct map_session_data *)bl, EQP_ARMOR);
		}
		break;
	case CG_TAROTCARD:
		break;
	}
//	if(sd && skillid != MC_CARTREVOLUTION && attack_type&BF_WEAPON){	/* �J�[�h�ɂ��ǉ����� */
	if(sd && attack_type&BF_WEAPON){	/* �J�[�h�ɂ��ǉ����� */
		int i;
		int sc_def_card=100;

		for(i=SC_STONE;i<=SC_BLIND;i++){
			//�Ώۂɏ�Ԉُ�
			if(i==SC_STONE || i==SC_FREEZE)
				sc_def_card=sc_def_mdef;
			else if(i==SC_STAN || i==SC_POISON || i==SC_SILENCE)
				sc_def_card=sc_def_vit;
			else if(i==SC_SLEEP || i==SC_CONFUSION || i==SC_BLIND)
				sc_def_card=sc_def_int;
			else if(i==SC_CURSE)
				sc_def_card=sc_def_luk;

			if(!sd->state.arrow_atk) {
				if(atn_rand()%10000 < (sd->addeff[i-SC_STONE])*sc_def_card/100 ){
					if(battle_config.battle_log)
						printf("PC %d skill_addeff: card�ɂ��ُ픭�� %d %d\n",sd->bl.id,i,sd->addeff[i-SC_STONE]);
					status_change_start(bl,i,7,0,0,0,(i==SC_CONFUSION)? 10000+7000:skill_get_time2(sc2[i-SC_STONE],7),0);
				}
			}
			else {
				if(atn_rand()%10000 < (sd->addeff[i-SC_STONE]+sd->arrow_addeff[i-SC_STONE])*sc_def_card/100 ){
					if(battle_config.battle_log)
						printf("PC %d skill_addeff: card�ɂ��ُ픭�� %d %d\n",sd->bl.id,i,sd->addeff[i-SC_STONE]);
					status_change_start(bl,i,7,0,0,0,(i==SC_CONFUSION)? 10000+7000:skill_get_time2(sc2[i-SC_STONE],7),0);
				}
			}
			//�����ɏ�Ԉُ�
			if(i==SC_STONE || i==SC_FREEZE)
				sc_def_card=sc_def_mdef2;
			else if(i==SC_STAN || i==SC_POISON || i==SC_SILENCE)
				sc_def_card=sc_def_vit2;
			else if(i==SC_SLEEP || i==SC_CONFUSION || i==SC_BLIND)
				sc_def_card=sc_def_int2;
			else if(i==SC_CURSE)
				sc_def_card=sc_def_luk2;

			if(!sd->state.arrow_atk) {
				if(atn_rand()%10000 < (sd->addeff2[i-SC_STONE])*sc_def_card/100 ){
					if(battle_config.battle_log)
						printf("PC %d skill_addeff: card�ɂ��ُ픭�� %d %d\n",src->id,i,sd->addeff2[i-SC_STONE]);
					status_change_start(src,i,7,0,0,0,(i==SC_CONFUSION)? 10000+7000:skill_get_time2(sc2[i-SC_STONE],7),0);
				}
			}
			else {
				if(atn_rand()%10000 < (sd->addeff2[i-SC_STONE]+sd->arrow_addeff2[i-SC_STONE])*sc_def_card/100 ){
					if(battle_config.battle_log)
						printf("PC %d skill_addeff: card�ɂ��ُ픭�� %d %d\n",src->id,i,sd->addeff2[i-SC_STONE]);
					status_change_start(src,i,7,0,0,0,(i==SC_CONFUSION)? 10000+7000:skill_get_time2(sc2[i-SC_STONE],7),0);
				}
			}
		}
	}
	
	//�����ɂ���
	if(sd && sd->curse_by_muramasa > 0 && attack_type&BF_WEAPON)
	{
		if(status_get_luk(src) < sd->status.base_level)
		{
			if(atn_rand()%10000 < sd->curse_by_muramasa*sc_def_luk2/100 )
				status_change_start(src,SC_CURSE,7,0,0,0,skill_get_time2(NPC_CURSEATTACK,7),0);
		}
	}
	
	//�����ăA�C�e������
	if(sd && sd->loss_equip_flag&0x0010 && attack_type&BF_WEAPON)
	{
		int i;
		for(i = 0;i<11;i++)
		{
			if(atn_rand()%10000 < sd->loss_equip_rate_when_attack[i])
			{
				pc_lossequipitem(sd,i,0);
			}
		}
	}
	
	//�����ăA�C�e���u���C�N
	if(sd && sd->loss_equip_flag&0x0100 && attack_type&BF_WEAPON)
	{
		int i;
		for(i = 0;i<11;i++)
		{
			if(atn_rand()%10000 < sd->break_myequip_rate_when_attack[i])
			{
				pc_break_equip2(sd,(unsigned short)i);
			}
		}
	}
	
	//������mob�ω�
	if(sd && dstmd && mob_db[dstmd->class].race != 7 
		&& !(mob_db[dstmd->class].mode&32) && attack_type&BF_WEAPON && dstmd->class != 1288)
	{
		if(atn_rand()%10000 < sd->mob_class_change_rate)
		{
			//clif_skill_nodamage(src,bl,SA_CLASSCHANGE,1,1);
			mob_class_change_randam((struct mob_data *)bl,sd->status.base_level);
		}
	}
	
	return 0;
}

/*=========================================================================
 * �X�L���U��������΂�����
 *  count: 0x00XYZZZZ
 *         X: ������΂������w��(�t����)
 *         Y: �t���O
 *            0x1: src��target�̈ʒu�֌W�Ő���΂�����������
 *            0x2: �X�L�b�h�g���b�v�̐�����΂�
 *         Z: ������΂��Z����
-------------------------------------------------------------------------*/
int skill_blown( struct block_list *src, struct block_list *target,int count)
{
	int dx=0,dy=0,nx,ny;
	int x=target->x,y=target->y;
	int dir,ret,prev_state=MS_IDLE;
	int moveblock;
	struct map_session_data *sd=NULL;
	struct mob_data *md=NULL;
	struct pet_data *pd=NULL;
	struct skill_unit *su=NULL;

	nullpo_retr(0, src);
	nullpo_retr(0, target);
	
	//�V�[�Y�Ȃ琁����΂����s
	if(map[target->m].flag.gvg)
			return 0;
			
	if(target->type==BL_PC){
		sd=(struct map_session_data *)target;
		// �o�W���J���͐�����΂���Ȃ�
		if (sd->sc_data[SC_BASILICA].timer!=-1 &&
				sd->sc_data[SC_BASILICA].val2==target->id)
			return 0;
	}else if(target->type==BL_MOB){
		md=(struct mob_data *)target;
	}else if(target->type==BL_PET){
		pd=(struct pet_data *)target;
	}else if(target->type==BL_SKILL){
		su=(struct skill_unit *)target;
	}else return 0;

	if (count&0xf00000)
		dir = (count>>20)&0xf;
	else if (count&0x10000 || (target->x==src->x && target->y==src->y))
		dir = status_get_dir(target);
	else
		dir = map_calc_dir(target,src->x,src->y);
	if (dir>=0 && dir<8){
		dx = -dirx[dir];
		dy = -diry[dir];
	}

	ret=path_blownpos(target->m,x,y,dx,dy,count&0xffff);
	nx=ret>>16;
	ny=ret&0xffff;
	moveblock=( x/BLOCK_SIZE != nx/BLOCK_SIZE || y/BLOCK_SIZE != ny/BLOCK_SIZE);

	if(count&0x20000) {
		battle_stopwalking(target,1);
		if(sd){
			sd->to_x=nx;
			sd->to_y=ny;
			sd->walktimer = 1;
			clif_walkok(sd);
			clif_movechar(sd);
		}
		else if(md) {
			md->to_x=nx;
			md->to_y=ny;
			prev_state = md->state.state;
			md->state.state = MS_WALK;
			clif_fixmobpos(md);
		}
		else if(pd) {
			pd->to_x=nx;
			pd->to_y=ny;
			prev_state = pd->state.state;
			pd->state.state = MS_WALK;
			clif_fixpetpos(pd);
		}
	}
	else
		battle_stopwalking(target,2);

	dx = nx - x;
	dy = ny - y;

	if(sd)	/* ��ʊO�ɏo���̂ŏ��� */
		map_foreachinmovearea(clif_pcoutsight,target->m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,dx,dy,0,sd);
	else if(md)
		map_foreachinmovearea(clif_moboutsight,target->m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,dx,dy,BL_PC,md);
	else if(pd)
		map_foreachinmovearea(clif_petoutsight,target->m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,dx,dy,BL_PC,pd);

	if(su){
		skill_unit_move_unit_group(su->group,target->m,dx,dy);
	}else{
		int tick = gettick();
		skill_unit_move(target,tick,0);
		if(moveblock) map_delblock(target);
		target->x=nx;
		target->y=ny;
		if(moveblock) map_addblock(target);
		skill_unit_move(target,tick,1);
	}

	if(sd) {	/* ��ʓ��ɓ����Ă����̂ŕ\�� */
		map_foreachinmovearea(clif_pcinsight,target->m,nx-AREA_SIZE,ny-AREA_SIZE,nx+AREA_SIZE,ny+AREA_SIZE,-dx,-dy,0,sd);
		if(count&0x20000)
			sd->walktimer = -1;
	}
	else if(md) {
		map_foreachinmovearea(clif_mobinsight,target->m,nx-AREA_SIZE,ny-AREA_SIZE,nx+AREA_SIZE,ny+AREA_SIZE,-dx,-dy,BL_PC,md);
		if(count&0x20000)
			md->state.state = prev_state;
	}
	else if(pd) {
		map_foreachinmovearea(clif_petinsight,target->m,nx-AREA_SIZE,ny-AREA_SIZE,nx+AREA_SIZE,ny+AREA_SIZE,-dx,-dy,BL_PC,pd);
		if(count&0x20000)
			pd->state.state = prev_state;
	}

	return 1;
}

/*=========================================================================
 * �X�L���U��������΂�����(�J�[�h�ǉ����ʗp)
 *  SAB_NORMAL     0x10000: src��target�̈ʒu�֌W�Ő���΂�����������
 *  SAB_SKIDTRAP   0x20000: �X�L�b�h�g���b�v�̐�����΂�
-------------------------------------------------------------------------*/
int skill_add_blown( struct block_list *src, struct block_list *target,int skillid,int flag)
{
	int i;
	struct map_session_data* sd = (struct map_session_data*)src;
	nullpo_retr(0, src);
	if(src->type != BL_PC)
		return 0;
	for(i = 0;i<sd->skill_blow.count;i++)
	{
		if(sd->skill_blow.id[i] == skillid)
		{
			 skill_blown(src,target,sd->skill_blow.grid[i]|flag);
			 return 1;
		}	
	}
	return 0;	
}

/*
 * =========================================================================
 * �X�L���U�����ʏ����܂Ƃ�
 * flag�̐����B16�i�}
 * 	00XRTTff
 *  ff	= magic�Ōv�Z�ɓn�����
 *	TT	= �p�P�b�g��type����(0�Ńf�t�H���g)
 *  X   = �p�P�b�g�̃X�L��Lv
 *  R	= �\��iskill_area_sub�Ŏg�p����)
 *-------------------------------------------------------------------------
 */
int skill_attack(int attack_type,struct block_list* src,struct block_list *dsrc,
	 struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag)
{
	struct Damage dmg;
	struct status_change *sc_data;
	int type,lv,damage;

	rdamage = 0;
	nullpo_retr(0, src);
	nullpo_retr(0, dsrc);
	nullpo_retr(0, bl);

	sc_data = status_get_sc_data(bl);

//�������Ȃ����肱������
	if(dsrc->m != bl->m) //�Ώۂ������}�b�v�ɂ��Ȃ���Ή������Ȃ�
		return 0;
	if(src->prev == NULL || dsrc->prev == NULL || bl->prev == NULL) //prev�悭�킩��Ȃ���
		return 0;
	if(src->type == BL_PC && pc_isdead((struct map_session_data *)src)) //�p�ҁH��PC�ł��łɎ���ł����牽�����Ȃ�
		return 0;
	if(dsrc->type == BL_PC && pc_isdead((struct map_session_data *)dsrc)) //�p�ҁH��PC�ł��łɎ���ł����牽�����Ȃ�
		return 0;
	if(bl->type == BL_PC && pc_isdead((struct map_session_data *)bl)) //�Ώۂ�PC�ł��łɎ���ł����牽�����Ȃ�
		return 0;
	
	if(sc_data && sc_data[SC_HIDING].timer != -1) { //�n�C�f�B���O��Ԃ�
		if(skill_get_pl(skillid) != 2) //�X�L���̑������n�����łȂ���Ή������Ȃ�
			return 0;
	}
	
	//�`�F�C�X�E�H�[�N��ԂŃ��A�t����
	if(sc_data && (sc_data[SC_CHASEWALK].timer != -1) && (skillid == AL_RUWACH)){
			return 0;
	}
	if(sc_data && sc_data[SC_TRICKDEAD].timer != -1) //���񂾂ӂ蒆�͉������Ȃ�
		return 0;
		
	if(skillid == WZ_STORMGUST) { //�g�p�X�L�����X�g�[���K�X�g��
		if(sc_data && sc_data[SC_FREEZE].timer != -1) //������ԂȂ牽�����Ȃ�
			return 0;
	}
	if(skillid == WZ_FROSTNOVA && dsrc->x == bl->x && dsrc->y == bl->y) //�g�p�X�L�����t���X�g�m���@�ŁAdsrc��bl�������ꏊ�Ȃ牽�����Ȃ�
		return 0;
	if(src->type == BL_PC && ((struct map_session_data *)src)->chatID) //�p�҂�PC�Ń`���b�g���Ȃ牽�����Ȃ�
		return 0;
	if(dsrc->type == BL_PC && ((struct map_session_data *)dsrc)->chatID) //�p�҂�PC�Ń`���b�g���Ȃ牽�����Ȃ�
		return 0;
	if(src->type == BL_PC && bl && mob_gvmobcheck(((struct map_session_data *)src),bl)==0)
		return 0;
	
//�������Ȃ����肱���܂�

	type=-1;
	lv=(flag>>20)&0xf;
	dmg=battle_calc_attack(attack_type,src,bl,skillid,skilllv,flag&0xff ); //�_���[�W�v�Z

//�}�W�b�N���b�h������������
	if(attack_type&BF_MAGIC && sc_data && sc_data[SC_MAGICROD].timer != -1 && src == dsrc) { //���@�U���Ń}�W�b�N���b�h��Ԃ�src=dsrc�Ȃ�
		dmg.damage = dmg.damage2 = 0; //�_���[�W0
		if(bl->type == BL_PC) { //�Ώۂ�PC�̏ꍇ
			int sp = skill_get_sp(skillid,skilllv); //�g�p���ꂽ�X�L����SP���z��
			sp = sp * sc_data[SC_MAGICROD].val2 / 100; //�z�����v�Z
			if(skillid == WZ_WATERBALL && skilllv > 1) //�E�H�[�^�[�{�[��Lv1�ȏ�
				sp = sp/((skilllv|1)*(skilllv|1)); //����Ɍv�Z�H
			if(sp > 0x7fff) sp = 0x7fff; //SP�������̏ꍇ�͗��_�ő�l
			else if(sp < 1) sp = 1; //1�ȉ��̏ꍇ��1
			if(((struct map_session_data *)bl)->status.sp + sp > ((struct map_session_data *)bl)->status.max_sp) { //��SP+���݂�SP��MSP���傫���ꍇ
				sp = ((struct map_session_data *)bl)->status.max_sp - ((struct map_session_data *)bl)->status.sp; //SP��MSP-����SP�ɂ���
				((struct map_session_data *)bl)->status.sp = ((struct map_session_data *)bl)->status.max_sp; //���݂�SP��MSP����
			}
			else //��SP+���݂�SP��MSP��菬�����ꍇ�͉�SP�����Z
				((struct map_session_data *)bl)->status.sp += sp;
			clif_heal(((struct map_session_data *)bl)->fd,SP_SP,sp); //SP�񕜃G�t�F�N�g�̕\��
			((struct map_session_data *)bl)->canact_tick = tick + skill_delayfix(bl, skill_get_delay(SA_MAGICROD,sc_data[SC_MAGICROD].val1), skill_get_cast(SA_MAGICROD,sc_data[SC_MAGICROD].val1)); //
		}
		clif_skill_nodamage(bl,bl,SA_MAGICROD,sc_data[SC_MAGICROD].val1,1); //�}�W�b�N���b�h�G�t�F�N�g��\��
	}
//�}�W�b�N���b�h���������܂�

	damage = dmg.damage + dmg.damage2;

	if(lv==15)
		lv=-1;

	if( flag&0xff00 )
		type=(flag&0xff00)>>8;

	if(damage <= 0 || damage < dmg.div_) //������΂�����H��
		dmg.blewcount = 0;
	
	
	//dmg.blewcount = 5;

	if(skillid == CR_GRANDCROSS||skillid == NPC_DARKGRANDCROSS) {//�O�����h�N���X
		if(battle_config.gx_disptype) dsrc = src;	// �G�_���[�W�������\��
		if( src == bl) type = 4;	// �����̓_���[�W���[�V�����Ȃ�
	}

//�g�p�҂�PC�̏ꍇ�̏�����������
	if(src->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)src;
		nullpo_retr(0, sd);
//�A�ŏ�(MO_CHAINCOMBO)��������
		if(skillid == MO_CHAINCOMBO) {
			int delay = 1000 - 4 * status_get_agi(src) - 2 *  status_get_dex(src); //��{�f�B���C�̌v�Z
			if(damage < status_get_hp(bl)) { //�_���[�W���Ώۂ�HP��菬�����ꍇ
				if(pc_checkskill(sd, MO_COMBOFINISH) > 0 && sd->spiritball > 0){ //�җ���(MO_COMBOFINISH)�擾���C���ێ�����+300ms
					delay += 300 * battle_config.combo_delay_rate /100; //�ǉ��f�B���C��conf�ɂ�蒲��				
					//�R���{���͎��Ԃ̍Œ�ۏ�ǉ�
					if(delay < battle_config.combo_delay_lower_limits)
						delay = battle_config.combo_delay_lower_limits;
				}
				status_change_start(src,SC_COMBO,MO_CHAINCOMBO,skilllv,0,0,delay,0); //�R���{��Ԃ�
			}
			sd->attackabletime = sd->canmove_tick = tick + delay;
			clif_combo_delay(src,delay); //�R���{�f�B���C�p�P�b�g�̑��M
		}
//�A�ŏ�(MO_CHAINCOMBO)�����܂�
//�җ���(MO_COMBOFINISH)��������
		else if(skillid == MO_COMBOFINISH) {
			int delay = 700 - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
			if(damage < status_get_hp(bl)) {
				//���C���e����(MO_EXTREMITYFIST)�擾���C��4�ێ��������g��(MO_EXPLOSIONSPIRITS)��Ԏ���+300ms
				//���Ռ�(CH_TIGERFIST)�擾����+300ms
				if((pc_checkskill(sd, MO_EXTREMITYFIST) > 0 && sd->spiritball >= 4 && sd->sc_data[SC_EXPLOSIONSPIRITS].timer != -1) || 
				(pc_checkskill(sd, CH_TIGERFIST) > 0 && sd->spiritball > 0) ||
				(pc_checkskill(sd, CH_CHAINCRUSH) > 0 && sd->spiritball > 1))
				{
					delay += 300 * battle_config.combo_delay_rate /100; //�ǉ��f�B���C��conf�ɂ�蒲��
					//�R���{���͎��ԍŒ�ۏ�ǉ�
					if(delay < battle_config.combo_delay_lower_limits)
						delay = battle_config.combo_delay_lower_limits;
				}
				status_change_start(src,SC_COMBO,MO_COMBOFINISH,skilllv,0,0,delay,0); //�R���{��Ԃ�
			}
			sd->attackabletime = sd->canmove_tick = tick + delay;
			clif_combo_delay(src,delay); //�R���{�f�B���C�p�P�b�g�̑��M
		}
//�җ���(MO_COMBOFINISH)�����܂�
//���Ռ�(CH_TIGERFIST)��������
		else if(skillid == CH_TIGERFIST) {
			int delay = 1000 - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
			if(damage < status_get_hp(bl)) {
				if(pc_checkskill(sd, CH_CHAINCRUSH) > 0){ //�A������(CH_CHAINCRUSH)�擾����+300ms
					delay += 300 * battle_config.combo_delay_rate /100; //�ǉ��f�B���C��conf�ɂ�蒲��
					//�R���{���͎��ԍŒ�ۏ�ǉ�
					if(delay < battle_config.combo_delay_lower_limits)
						delay = battle_config.combo_delay_lower_limits;	
				}

				status_change_start(src,SC_COMBO,CH_TIGERFIST,skilllv,0,0,delay,0); //�R���{��Ԃ�
			}
			sd->attackabletime = sd->canmove_tick = tick + delay;
			clif_combo_delay(src,delay); //�R���{�f�B���C�p�P�b�g�̑��M
		}
//���Ռ�(CH_TIGERFIST)�����܂�
//�A������(CH_CHAINCRUSH)��������
		else if(skillid == CH_CHAINCRUSH) {
			int delay = 1000 - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
			if(damage < status_get_hp(bl)) {
				//���C���e����(MO_EXTREMITYFIST)�擾���C��4�ێ��������g��(MO_EXPLOSIONSPIRITS)��Ԏ���+300ms
				if(pc_checkskill(sd, MO_EXTREMITYFIST) > 0 && sd->spiritball >= 4 && sd->sc_data[SC_EXPLOSIONSPIRITS].timer != -1)
				{
					delay += 300 * battle_config.combo_delay_rate /100; //�ǉ��f�B���C��conf�ɂ�蒲��
					//�R���{���͎��ԍŒ�ۏ�ǉ�
					if(delay < battle_config.combo_delay_lower_limits)
						delay = battle_config.combo_delay_lower_limits;	
				}
				status_change_start(src,SC_COMBO,CH_CHAINCRUSH,skilllv,0,0,delay,0); //�R���{��Ԃ�
			}
			sd->attackabletime = sd->canmove_tick = tick + delay;
			clif_combo_delay(src,delay); //�R���{�f�B���C�p�P�b�g�̑��M
		}
//�A������(CH_CHAINCRUSH)�����܂�

		//TK�R���{
		//�����ɂ��̂����e�R�������J�[�����ǉ��H
		if(skillid == TK_STORMKICK){
			status_change_end(src,SC_TKCOMBO,-1); //TK�R���{�I��
		}else if(skillid == TK_STORMKICK+2){
			status_change_end(src,SC_TKCOMBO,-1); //TK�R���{�I��
		}else if(skillid == TK_STORMKICK+4){
			status_change_end(src,SC_TKCOMBO,-1); //TK�R���{�I��
		}
	}
//�g�p�҂�PC�̏ꍇ�̏��������܂�
//����X�L���H��������
	if(attack_type&BF_WEAPON && damage > 0 && src != bl && src == dsrc) { //����X�L�����_���[�W���聕�g�p�҂ƑΏێ҂��Ⴄ��src=dsrc
		if(dmg.flag&BF_SHORT) { //�ߋ����U�����H��
			if(bl->type == BL_PC) { //�Ώۂ�PC�̎�
				struct map_session_data *tsd = (struct map_session_data *)bl;
				nullpo_retr(0, tsd);
				if(tsd->short_weapon_damage_return > 0) { //�ߋ����U�����˕Ԃ��H��
					rdamage += damage * tsd->short_weapon_damage_return / 100;
					if(rdamage < 1) rdamage = 1;
				}
			}
			if(sc_data && sc_data[SC_REFLECTSHIELD].timer != -1) { //���t���N�g�V�[���h��
				rdamage += damage * sc_data[SC_REFLECTSHIELD].val2 / 100; //���˕Ԃ��v�Z
				if(rdamage < 1) rdamage = 1;
			}
		}
		else if(dmg.flag&BF_LONG) { //�������U�����H��
			if(bl->type == BL_PC) { //�Ώۂ�PC�̎�
				struct map_session_data *tsd = (struct map_session_data *)bl;
				nullpo_retr(0, tsd);
				if(tsd->long_weapon_damage_return > 0) { //�������U�����˕Ԃ��H��
					rdamage += damage * tsd->long_weapon_damage_return / 100;
					if(rdamage < 1) rdamage = 1;
				}
			}
		}
		if(rdamage > 0)
			clif_damage(src,src,tick, dmg.amotion,0,rdamage,1,4,0);
	}
	if(attack_type&BF_MAGIC && damage > 0 && src != bl && src == dsrc) { //���@�X�L�����_���[�W���聕�g�p�҂ƑΏێ҂��Ⴄ
		if(bl->type == BL_PC) { //�Ώۂ�PC�̎�
			struct map_session_data *tsd = (struct map_session_data *)bl;
			nullpo_retr(0, tsd);
			if(tsd->magic_damage_return > 0 && atn_rand()%100 < tsd->magic_damage_return) { //���@�U�����˕Ԃ��H��
				rdamage = damage;
			}
		}
		//�J�C�g
		if(sc_data && sc_data[SC_KAITE].timer!=-1)
		{
			if(src->type == BL_PC || status_get_lv(src) < 80)
			{
				rdamage = damage;
				sc_data[SC_KAITE].val2--;
				if(sc_data[SC_KAITE].val2==0)
					status_change_end(bl,SC_KAITE,-1);
			}
			
		}
		if(rdamage > 0)
			clif_damage(src,src,tick, dmg.amotion,0,rdamage,1,4,0);
	}
//����X�L���H�����܂�

	switch(skillid){
	case AS_SPLASHER:
		clif_skill_damage(dsrc,bl,tick,dmg.amotion,dmg.dmotion, damage, dmg.div_, skillid, -1, 5);
		break;
	case NPC_SELFDESTRUCTION:
	case NPC_SELFDESTRUCTION2:
		break;
	default:
		clif_skill_damage(dsrc,bl,tick,dmg.amotion,dmg.dmotion, damage, dmg.div_, skillid, (lv!=0)?lv:skilllv, (skillid==0)? 5:type );
	}
	/* ������΂������Ƃ��̃p�P�b�g */
	if (dmg.blewcount>0 && bl->type!=BL_SKILL && !map[src->m].flag.gvg) {
		skill_blown(dsrc,bl,dmg.blewcount);
		if(bl->type == BL_MOB)
			clif_fixmobpos((struct mob_data *)bl);
		else if(bl->type == BL_PET)
			clif_fixpetpos((struct pet_data *)bl);
		else
			clif_fixpos(bl);
	}

	// ������΂������Ƃ��̃p�P�b�g �J�[�h���� ??
	if (dsrc->type == BL_PC && bl->type!=BL_SKILL && !map[src->m].flag.gvg)
	{
		if(skill_add_blown(dsrc,bl,skillid,SAB_NORMAL))
		{
			if(bl->type == BL_MOB)
				clif_fixmobpos((struct mob_data *)bl);
			else if(bl->type == BL_PET)
				clif_fixpetpos((struct pet_data *)bl);
			else
				clif_fixpos(bl);
		}
	}
	
	map_freeblock_lock();
	/* ���ۂɃ_���[�W�������s�� */
	if (skillid || flag) {
		if (attack_type&BF_WEAPON)
		{
			battle_delay_damage(tick+dmg.amotion,src,bl,damage,0);
		}else{
			battle_damage(src,bl,damage,0);
		}
	}
	if(skillid == RG_INTIMIDATE && damage > 0 && !(status_get_mode(bl)&0x20) && !map[src->m].flag.gvg ) {
		int s_lv = status_get_lv(src),t_lv = status_get_lv(bl);
		int rate = 50 + skilllv * 5;
		rate = rate + (s_lv - t_lv);
		if(atn_rand()%100 < rate)
			skill_addtimerskill(src,tick + 800,bl->id,0,0,skillid,skilllv,0,flag);
	}
	
	//�N���[���X�L��
	if(damage > 0 && dmg.flag&BF_SKILL && bl->type==BL_PC 
		&& pc_checkskill((struct map_session_data *)bl,RG_PLAGIARISM) && sc_data[SC_PRESERVE].timer == -1){
		struct map_session_data *tsd = (struct map_session_data *)bl;
		struct pc_base_job s_class;
		s_class = pc_calc_base_job(tsd->status.class);
		nullpo_retr(0, tsd);
		
		//�]���E�]���ŃX�L�����N���[���ł���ꍇ
		if(battle_config.extended_cloneskill &&  s_class.upper == 1
			&& !tsd->status.skill[skillid].id && !tsd->status.skill[skillid].lv
			&& skillid <= CR_CULTIVATION
			&& !(skillid >= NPC_PIERCINGATT && skillid <= NPC_SUMMONMONSTER)
			&& !(skillid >= NPC_SELFDESTRUCTION2 && skillid <= NPC_RECALL)
			)
				goto Extended_CloneSkill_Label;
		
		if(!tsd->status.skill[skillid].id && !tsd->status.skill[skillid].lv
			&&  skillid < NPC_SELFDESTRUCTION2
			&& !(skillid >= NPC_PIERCINGATT && skillid <= NPC_SUMMONMONSTER)
			)
		{
		
		Extended_CloneSkill_Label:
		
			//�T���N�`���A�����󂯂��ꍇ�A��Lv�̃q�[�����N���[��
			if(skillid == PR_SANCTUARY) skillid = AL_HEAL;
			
			//�����N���[���ł��Ȃ��悤�Ɏb�菈��
			if(!tsd->cloneskill_id && !tsd->cloneskill_lv )
			{
				int i;
				int clone_count = 0;
				for(i=0;i<MAX_SKILL;i++)
				{
					if(tsd->status.skill[i].flag==13)
					{
						clone_count++;
						tsd->cloneskill_id	=	tsd->status.skill[i].id;
						tsd->cloneskill_lv	=	tsd->status.skill[i].lv;
					}
				}
				if(clone_count > 1)//�Ă΂�Ȃ��͂��c
				{
					//�����̏ꍇ�P�ɂ��鏈������肽����
					//��������܂����̂ŕۗ�
					printf("error:cloneskill��%d���݂��܂�\n",clone_count);
				}
			}
			
			//���ɓ���ł���X�L��������ΊY���X�L��������
			if (tsd->cloneskill_id && tsd->cloneskill_lv && tsd->status.skill[tsd->cloneskill_id].flag==13){
				tsd->status.skill[tsd->cloneskill_id].id=0;
				tsd->status.skill[tsd->cloneskill_id].lv=0;
				tsd->status.skill[tsd->cloneskill_id].flag=0;
			}
			tsd->cloneskill_id=skillid;
			tsd->cloneskill_lv=skilllv;
			tsd->status.skill[skillid].id=skillid;
//			tsd->status.skill[skillid].lv=(pc_checkskill(tsd,RG_PLAGIARISM) > skill_get_max(skillid))?
//							skill_get_max(skillid):pc_checkskill(tsd,RG_PLAGIARISM);
			tsd->status.skill[skillid].lv=(skilllv < pc_checkskill(tsd,RG_PLAGIARISM))?
							skilllv : pc_checkskill(tsd,RG_PLAGIARISM);
			tsd->status.skill[skillid].flag=13;//cloneskill flag
			clif_skillinfoblock(tsd);
			
		}
	}
	/* �_���[�W������Ȃ�ǉ����ʔ��� */
	if(bl->prev != NULL){
		struct map_session_data *sd = (struct map_session_data *)bl;
		nullpo_retr(0, sd);
		if( bl->type != BL_PC || (sd && !pc_isdead(sd)) ) {
			if(damage > 0)
				skill_additional_effect(src,bl,skillid,skilllv,attack_type,tick);
			if(bl->type==BL_MOB && src!=bl)	/* �X�L���g�p������MOB�X�L�� */
			{
				struct mob_data *md=(struct mob_data *)bl;
				nullpo_retr(0, md);
				if(battle_config.mob_changetarget_byskill == 1)
				{
					int target;
					target=md->target_id;
					if(src->type == BL_PC)
						md->target_id=src->id;
					mobskill_use(md,tick,MSC_SKILLUSED|(skillid<<16));
					md->target_id=target;
				}
				else
					mobskill_use(md,tick,MSC_SKILLUSED|(skillid<<16));
			}
		}
	}

	if(src->type == BL_PC && dmg.flag&BF_WEAPON && src != bl && src == dsrc && damage > 0) {
		struct map_session_data *sd = (struct map_session_data *)src;
		int hp = 0,sp = 0;
		nullpo_retr(0, sd);
		if(sd->hp_drain_rate && sd->hp_drain_per > 0 && dmg.damage > 0 && atn_rand()%100 < sd->hp_drain_rate) {
			hp += (dmg.damage * sd->hp_drain_per)/100;
			if(sd->hp_drain_rate > 0 && hp < 1) hp = 1;
			else if(sd->hp_drain_rate < 0 && hp > -1) hp = -1;
		}
		if(sd->hp_drain_rate_ && sd->hp_drain_per_ > 0 && dmg.damage2 > 0 && atn_rand()%100 < sd->hp_drain_rate_) {
			hp += (dmg.damage2 * sd->hp_drain_per_)/100;
			if(sd->hp_drain_rate_ > 0 && hp < 1) hp = 1;
			else if(sd->hp_drain_rate_ < 0 && hp > -1) hp = -1;
		}
		if(sd->sp_drain_rate > 0 && sd->sp_drain_per > 0 && dmg.damage > 0 && atn_rand()%100 < sd->sp_drain_rate) {
			sp += (dmg.damage * sd->sp_drain_per)/100;
			if(sd->sp_drain_rate > 0 && sp < 1) sp = 1;
			else if(sd->sp_drain_rate < 0 && sp > -1) sp = -1;
		}
		if(sd->sp_drain_rate_ > 0 && sd->sp_drain_per_ > 0 && dmg.damage2 > 0 && atn_rand()%100 < sd->sp_drain_rate_) {
			sp += (dmg.damage2 * sd->sp_drain_per_)/100;
			if(sd->sp_drain_rate_ > 0 && sp < 1) sp = 1;
			else if(sd->sp_drain_rate_ < 0 && sp > -1) sp = -1;
		}
		if(hp || sp) pc_heal(sd,hp,sp);
	}
	//�J�A�q
	if(skillid==0 && damage >0 && dmg.flag&BF_WEAPON && sc_data && sc_data[SC_KAAHI].timer!=-1)
	{
		int kaahi_lv = sc_data[SC_KAAHI].val1;
		int hp = 0,sp = 0;
		if(bl->type == BL_PC)
		{
			struct map_session_data *sd = (struct map_session_data *)bl;
			if(sd->status.sp >= 5*kaahi_lv)
			{
				sp = 5*kaahi_lv;
				hp = 200*kaahi_lv;
				if(hp || sp) pc_heal(sd,hp,-sp);
			}
		}
		else if(bl->type == BL_MOB)
		{
			mob_heal((struct mob_data*)bl,200*kaahi_lv);
		}
	}
	
	if ((skillid || flag) && rdamage>0) {
		if (attack_type&BF_WEAPON)
		{
			battle_delay_damage(tick+dmg.amotion,bl,src,rdamage,0);
			//���˃_���[�W�̃I�[�g�X�y��
			if(battle_config.weapon_reflect_autospell)
			{
				skill_bonus_autospell(bl,src,AS_ATTACK,0,0);
			}
			
			if(battle_config.weapon_reflect_drain)
				battle_attack_drain(bl,src,rdamage,0,battle_config.weapon_reflect_drain_per_enable);
		}
		else
		{
			battle_damage(bl,src,rdamage,0);
			//���˃_���[�W�̃I�[�g�X�y��
			if(battle_config.magic_reflect_autospell)
			{
				skill_bonus_autospell(bl,src,AS_ATTACK,0,0);
			}
			if(battle_config.magic_reflect_drain)
				battle_attack_drain(bl,src,rdamage,0,battle_config.magic_reflect_drain_per_enable);
		}
	}

	if(attack_type&BF_WEAPON && sc_data && sc_data[SC_AUTOCOUNTER].timer != -1 && sc_data[SC_AUTOCOUNTER].val4 > 0) {
		if(sc_data[SC_AUTOCOUNTER].val3 == dsrc->id)
			battle_weapon_attack(bl,dsrc,tick,0x8000|sc_data[SC_AUTOCOUNTER].val1);
		status_change_end(bl,SC_AUTOCOUNTER,-1);
	}
	/* �_�u���L���X�e�B���O */
	if (( skillid == MG_COLDBOLT || skillid == MG_FROSTDIVER ||
		skillid == MG_FIREBOLT || skillid == MG_FIREBALL ||
		skillid == MG_LIGHTNINGBOLT) &&
		(sc_data = status_get_sc_data(src)) &&
		sc_data[SC_DOUBLECASTING].timer != -1 &&
		rand() % 100 < 40+10*skilllv) {
		if (!(flag & 1))
			skill_castend_delay (src, bl, skillid, skilllv, tick + dmg.div_*dmg.amotion, flag|1);
	}

	map_freeblock_unlock();

	return (dmg.damage+dmg.damage2);	/* �^�_����Ԃ� */
}

/*==========================================
 * �X�L���͈͍U���p(map_foreachinarea����Ă΂��)
 * flag�ɂ��āF16�i�}���m�F
 * MSB <- 00fTffff ->LSB
 *	T	=�^�[�Q�b�g�I��p(BCT_*)
 *  ffff=���R�Ɏg�p�\
 *  0	=�\��B0�ɌŒ�
 *------------------------------------------
 */
static int skill_area_temp[8];	/* �ꎞ�ϐ��B�K�v�Ȃ�g���B */
typedef int (*SkillFunc)(struct block_list *,struct block_list *,int,int,unsigned int,int);
int skill_area_sub( struct block_list *bl,va_list ap )
{
	struct block_list *src;
	int skill_id,skill_lv,flag;
	unsigned int tick;
	SkillFunc func;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	if(bl->type!=BL_PC && bl->type!=BL_MOB && bl->type!=BL_SKILL)
		return 0;

	src=va_arg(ap,struct block_list *); //�����ł�src�̒l���Q�Ƃ��Ă��Ȃ��̂�NULL�`�F�b�N�͂��Ȃ�
	skill_id=va_arg(ap,int);
	skill_lv=va_arg(ap,int);
	tick=va_arg(ap,unsigned int);
	flag=va_arg(ap,int);
	func=va_arg(ap,SkillFunc);

	if(battle_check_target(src,bl,flag) > 0)
		func(src,bl,skill_id,skill_lv,tick,flag);
	return 0;
}

static int skill_check_unit_range_sub( struct block_list *bl,va_list ap )
{
	struct skill_unit *unit;
	int *c;
	int skillid,unit_id;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, unit = (struct skill_unit *)bl);
	nullpo_retr(0, c = va_arg(ap,int *));

	if (bl->prev==NULL || bl->type!=BL_SKILL)
		return 0;

	if (!unit->alive)
		return 0;

	skillid = va_arg(ap,int);
	unit_id = unit->group->unit_id;

	if (skillid==MG_SAFETYWALL || skillid==AL_PNEUMA) {
		if(unit_id != 0x7e && unit_id != 0x85)
			return 0;
	} else if (skillid==AL_WARP) {
		if ((unit_id<0x8f || unit_id>0x99) && unit_id!=0x92)
			return 0;
	} else if ((skillid>=HT_SKIDTRAP && skillid<=HT_CLAYMORETRAP) || skillid==HT_TALKIEBOX) {
		if ((unit_id<0x8f || unit_id>0x99) && unit_id!=0x92)
			return 0;
	} else if (skillid==WZ_FIREPILLAR) {
		if (unit_id!=0x87)
			return 0;
	} else if (skillid==HP_BASILICA) {
		if ((unit_id<0x8f || unit_id>0x99) && unit_id!=0x92 && unit_id!=0x83)
			return 0;
	} else
		return 0;

	(*c)++;

	return 0;
}

int skill_check_unit_range(int m,int x,int y,int skillid,int skilllv)
{
	int c = 0;
	int range = skill_get_unit_range(skillid);
	int layout_type = skill_get_unit_layout_type(skillid,skilllv);
	if (layout_type==-1 || layout_type>MAX_SQUARE_LAYOUT) {
		printf("skill_check_unit_range: unsupported layout type %d for skill %d\n",layout_type,skillid);
		return 0;
	}

	// �Ƃ肠���������`�̃��j�b�g���C�A�E�g�̂ݑΉ�
	range += layout_type;
	map_foreachinarea(skill_check_unit_range_sub,m,
			x-range,y-range,x+range,y+range,BL_SKILL,&c,skillid);

	return c;
}

static int skill_check_unit_range2_sub( struct block_list *bl,va_list ap )
{
	int *c;
	int skillid;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, c = va_arg(ap,int *));

	if(bl->prev == NULL || (bl->type != BL_PC && bl->type != BL_MOB))
		return 0;

	if(bl->type == BL_PC && pc_isdead((struct map_session_data *)bl))
		return 0;

	skillid = va_arg(ap,int);
	if (skillid==HP_BASILICA && bl->type==BL_PC)
		return 0;

	(*c)++;

	return 0;
}

int skill_check_unit_range2(int m,int x,int y,int skillid, int skilllv)
{
	int c = 0;
	int range = skill_get_unit_range(skillid);
	int layout_type = skill_get_unit_layout_type(skillid,skilllv);
	if (layout_type==-1 || layout_type>MAX_SQUARE_LAYOUT) {
		printf("skill_check_unit_range2: unsupported layout type %d for skill %d\n",layout_type,skillid);
		return 0;
	}

	// �Ƃ肠���������`�̃��j�b�g���C�A�E�g�̂ݑΉ�
	range += layout_type;
	map_foreachinarea(skill_check_unit_range2_sub,m,
			x-range,y-range,x+range,y+range,0,&c,skillid);

	return c;
}

struct castend_delay {
	struct block_list *src;
	int target;
	int id;
	int lv;
	int flag;
};

int skill_castend_delay (struct block_list* src, struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag)
{
	struct castend_delay *dat;
	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	dat = (struct castend_delay *)aCalloc(1, sizeof(struct castend_delay));
	dat->src = src;
	dat->target = bl->id;
	dat->id = skillid;
	dat->lv = skilllv;
	dat->flag = flag;
	add_timer (tick, skill_castend_delay_sub, src->id, (int)dat);

	return 0;
}

int skill_castend_delay_sub (int tid, unsigned int tick, int id, int data)
{
	struct castend_delay *dat = (struct castend_delay *)data;
	struct block_list *target = map_id2bl(dat->target);
	
	if (target && dat && map_id2bl(id) == dat->src && target->prev != NULL)
		skill_castend_damage_id(dat->src, target, dat->id, dat->lv, tick, dat->flag);
	aFree(dat);
	return 0;
}

/*=========================================================================
 * �͈̓X�L���g�p������������������
 */
/* �Ώۂ̐����J�E���g����B�iskill_area_temp[0]�����������Ă������Ɓj */
int skill_area_sub_count(struct block_list *src,struct block_list *target,int skillid,int skilllv,unsigned int tick,int flag)
{
	if(skill_area_temp[0] < 0xffff)
		skill_area_temp[0]++;
	return 0;
}

int skill_count_water(struct block_list *src,int range)
{
	int i,x,y,cnt = 0,size = range*2+1;
	struct skill_unit *unit;
	
	for (i=0;i<size*size;i++) {
		x = src->x+(i%size-range);
		y = src->y+(i/size-range);
		if (map_getcell(src->m,x,y,CELL_CHKWATER)) {
			cnt++;
			continue;
		}
		unit = map_find_skill_unit_oncell(src,x,y,SA_DELUGE,NULL);
		if (unit) {
			cnt++;
			skill_delunit(unit);
		}
	}
	return cnt;
}
/*==========================================
 *
 *------------------------------------------
 */
static int skill_timerskill(int tid, unsigned int tick, int id,int data )
{
	struct map_session_data *sd = NULL;
	struct mob_data *md = NULL;
	struct block_list *src = map_id2bl(id),*target;
	struct skill_timerskill *skl = NULL;
	int range;

	nullpo_retr(0, src);

	if(src->prev == NULL)
		return 0;

	if(src->type == BL_PC) {
		nullpo_retr(0, sd = (struct map_session_data *)src);
		skl = &sd->skilltimerskill[data];
	}
	else if(src->type == BL_MOB) {
		nullpo_retr(0, md = (struct mob_data *)src);
		skl = &md->skilltimerskill[data];
	}
	else
		return 0;

	nullpo_retr(0, skl);

	skl->timer = -1;
	if(skl->target_id) {
		struct block_list tbl;
		target = map_id2bl(skl->target_id);
		if(skl->skill_id == RG_INTIMIDATE) {
			if(target == NULL) {
				target = &tbl; //���������ĂȂ��̂ɃA�h���X�˂�����ł����̂��ȁH
				target->type = BL_NUL;
				target->m = src->m;
				target->prev = target->next = NULL;
			}
		}
		if(target == NULL)
			return 0;
		if(target->prev == NULL && skl->skill_id != RG_INTIMIDATE)
			return 0;
		if(src->m != target->m)
			return 0;
		if(sd && pc_isdead(sd))
			return 0;
		if(target->type == BL_PC && pc_isdead((struct map_session_data *)target) && skl->skill_id != RG_INTIMIDATE)
			return 0;

		switch(skl->skill_id) {
			case TF_BACKSLIDING:
				clif_skill_nodamage(src,src,skl->skill_id,skl->skill_lv,1);
				break;
			case RG_INTIMIDATE:
				if(sd && !map[src->m].flag.noteleport) {
					int x,y,i,j;
					pc_randomwarp(sd,3);
					for(i=0;i<16;i++) {
						j = atn_rand()%8;
						x = sd->bl.x + dirx[j];
						y = sd->bl.y + diry[j];
						if(map_getcell(sd->bl.m,x,y,CELL_CHKPASS))
							break;
					}
					if(i >= 16) {
						x = sd->bl.x;
						y = sd->bl.y;
					}
					if(target->prev != NULL) {
						if(target->type == BL_PC && !pc_isdead((struct map_session_data *)target))
							pc_setpos((struct map_session_data *)target,map[sd->bl.m].name,x,y,3);
						else if(target->type == BL_MOB)
							mob_warp((struct mob_data *)target,-1,x,y,3);
					}
				}
				else if(md && !map[src->m].flag.monster_noteleport) {
					int x,y,i,j;
					mob_warp(md,-1,-1,-1,3);
					for(i=0;i<16;i++) {
						j = atn_rand()%8;
						x = md->bl.x + dirx[j];
						y = md->bl.y + diry[j];
						if(map_getcell(md->bl.m,x,y,CELL_CHKPASS))
							break;
					}
					if(i >= 16) {
						x = md->bl.x;
						y = md->bl.y;
					}
					if(target->prev != NULL) {
						if(target->type == BL_PC && !pc_isdead((struct map_session_data *)target))
							pc_setpos((struct map_session_data *)target,map[md->bl.m].name,x,y,3);
						else if(target->type == BL_MOB)
							mob_warp((struct mob_data *)target,-1,x,y,3);
					}
				}
				break;

			case BA_FROSTJOKE:			/* �����W���[�N */
			case DC_SCREAM:				/* �X�N���[�� */
				range=15;		//���E�S��
				map_foreachinarea(skill_frostjoke_scream,src->m,src->x-range,src->y-range,
					src->x+range,src->y+range,0,src,skl->skill_id,skl->skill_lv,tick);
				break;
			case WZ_WATERBALL:
				if (skl->type>1) {
					skl->timer = 0;	// skill_addtimerskill�Ŏg�p����Ȃ��悤��
					skill_addtimerskill(src,tick+150,target->id,0,0,skl->skill_id,skl->skill_lv,skl->type-1,skl->flag);
					skl->timer = -1;
				}
				skill_attack(BF_MAGIC,src,src,target,skl->skill_id,skl->skill_lv,tick,skl->flag);
				break;
			default:
				skill_attack(skl->type,src,src,target,skl->skill_id,skl->skill_lv,tick,skl->flag);
				break;
		}
	}
	else {
		if(src->m != skl->map)
			return 0;
		switch(skl->skill_id) {
			case WZ_METEOR:
				if(skl->type >= 0) {
					skill_unitsetting(src,skl->skill_id,skl->skill_lv,skl->type>>16,skl->type&0xFFFF,0);
					clif_skill_poseffect(src,skl->skill_id,skl->skill_lv,skl->x,skl->y,tick);
				}
				else
					skill_unitsetting(src,skl->skill_id,skl->skill_lv,skl->x,skl->y,0);
				break;
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int skill_addtimerskill(struct block_list *src,unsigned int tick,int target,int x,int y,int skill_id,int skill_lv,int type,int flag)
{
	int i;

	nullpo_retr(1, src);

	if(src->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)src;
		nullpo_retr(1, sd);
		for(i=0;i<MAX_SKILLTIMERSKILL;i++) {
			if(sd->skilltimerskill[i].timer == -1) {
				sd->skilltimerskill[i].timer = add_timer(tick, skill_timerskill, src->id, i);
				sd->skilltimerskill[i].src_id = src->id;
				sd->skilltimerskill[i].target_id = target;
				sd->skilltimerskill[i].skill_id = skill_id;
				sd->skilltimerskill[i].skill_lv = skill_lv;
				sd->skilltimerskill[i].map = src->m;
				sd->skilltimerskill[i].x = x;
				sd->skilltimerskill[i].y = y;
				sd->skilltimerskill[i].type = type;
				sd->skilltimerskill[i].flag = flag;

				return 0;
			}
		}
		return 1;
	}
	else if(src->type == BL_MOB) {
		struct mob_data *md = (struct mob_data *)src;
		nullpo_retr(1, md);
		for(i=0;i<MAX_MOBSKILLTIMERSKILL;i++) {
			if(md->skilltimerskill[i].timer == -1) {
				md->skilltimerskill[i].timer = add_timer(tick, skill_timerskill, src->id, i);
				md->skilltimerskill[i].src_id = src->id;
				md->skilltimerskill[i].target_id = target;
				md->skilltimerskill[i].skill_id = skill_id;
				md->skilltimerskill[i].skill_lv = skill_lv;
				md->skilltimerskill[i].map = src->m;
				md->skilltimerskill[i].x = x;
				md->skilltimerskill[i].y = y;
				md->skilltimerskill[i].type = type;
				md->skilltimerskill[i].flag = flag;

				return 0;
			}
		}
		return 1;
	}

	return 1;
}

/*==========================================
 *
 *------------------------------------------
 */
int skill_cleartimerskill(struct block_list *src)
{
	int i;

	nullpo_retr(0, src);

	if(src->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)src;
		nullpo_retr(0, sd);
		for(i=0;i<MAX_SKILLTIMERSKILL;i++) {
			if(sd->skilltimerskill[i].timer != -1) {
				delete_timer(sd->skilltimerskill[i].timer, skill_timerskill);
				sd->skilltimerskill[i].timer = -1;
			}
		}
	}
	else if(src->type == BL_MOB) {
		struct mob_data *md = (struct mob_data *)src;
		nullpo_retr(0, md);
		for(i=0;i<MAX_MOBSKILLTIMERSKILL;i++) {
			if(md->skilltimerskill[i].timer != -1) {
				delete_timer(md->skilltimerskill[i].timer, skill_timerskill);
				md->skilltimerskill[i].timer = -1;
			}
		}
	}

	return 0;
}

/* �͈̓X�L���g�p���������������܂�
 * -------------------------------------------------------------------------
 */

/*==========================================
 * �X�L���g�p�i�r�������AID�w��U���n�j
 * �i�X�p�Q�b�e�B�Ɍ����ĂP���O�i�I(�_���|)�j
 *------------------------------------------
 */
int skill_castend_damage_id( struct block_list* src, struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag )
{
	struct map_session_data *sd=NULL;
	int i;

	nullpo_retr(1, src);
	nullpo_retr(1, bl);

	if(src->type==BL_PC)
		sd=(struct map_session_data *)src;
	if(sd && pc_isdead(sd))
		return 1;

	if((skillid == CR_GRANDCROSS || skillid == NPC_DARKGRANDCROSS) && src != bl)
		bl = src;
	if(bl->prev == NULL)
		return 1;
	if(bl->type == BL_PC && pc_isdead((struct map_session_data *)bl))
		return 1;
	map_freeblock_lock();
	switch(skillid)
	{
	/* ����U���n�X�L�� */
	case SM_BASH:			/* �o�b�V�� */
	case MC_MAMMONITE:		/* ���}�[�i�C�g */
	case AC_DOUBLE:			/* �_�u���X�g���C�t�B���O */
	case AS_SONICBLOW:		/* �\�j�b�N�u���[ */
	case KN_PIERCE:			/* �s�A�[�X */
	case KN_SPEARBOOMERANG:	/* �X�s�A�u�[������ */
	case TF_POISON:			/* �C���x�i�� */
	case TF_SPRINKLESAND:	/* ���܂� */
	case AC_CHARGEARROW:	/* �`���[�W�A���[ */
	case RG_RAID:			/* �T�v���C�Y�A�^�b�N */
	case ASC_METEORASSAULT:	/* ���e�I�A�T���g */
	case RG_INTIMIDATE:		/* �C���e�B�~�f�C�g */
	case BA_MUSICALSTRIKE:	/* �~���[�W�J���X�g���C�N */
	case DC_THROWARROW:		/* ��� */
	case BA_DISSONANCE:		/* �s���a�� */
	case CR_HOLYCROSS:		/* �z�[���[�N���X */
	case CR_SHIELDCHARGE:
	case CR_SHIELDBOOMERANG:

	/* �ȉ�MOB��p */
	/* �P�̍U���ASP�����U���A�������U���A�h�䖳���U���A���i�U�� */
	case NPC_PIERCINGATT:
	case NPC_MENTALBREAKER:
	case NPC_RANGEATTACK:
	case NPC_CRITICALSLASH:
	case NPC_COMBOATTACK:
	/* �K���U���A�ōU���A�Í��U���A���ٍU���A�X�^���U�� */
	case NPC_GUIDEDATTACK:
	case NPC_POISON:
	case NPC_BLINDATTACK:
	case NPC_SILENCEATTACK:
	case NPC_STUNATTACK:
	/* �Ή��U���A�􂢍U���A�����U���A�����_��ATK�U�� */
	case NPC_PETRIFYATTACK:
	case NPC_CURSEATTACK:
	case NPC_SLEEPATTACK:
	case NPC_RANDOMATTACK:
	/* �������U���A�n�����U���A�Α����U���A�������U�� */
	case NPC_WATERATTACK:
	case NPC_GROUNDATTACK:
	case NPC_FIREATTACK:
	case NPC_WINDATTACK:
	/* �ő����U���A�������U���A�ő����U���A�O�����U���ASP�����U�� */
	case NPC_POISONATTACK:
	case NPC_HOLYATTACK:
	case NPC_DARKNESSATTACK:
	case NPC_TELEKINESISATTACK:
	case NPC_UNDEADATTACK:
	case NPC_BREAKARMOR:
	case NPC_BREAKWEAPON:
	case NPC_BREAKHELM:
	case NPC_BREAKSIELD:
	case LK_SPIRALPIERCE:		/* �X�p�C�����s�A�[�X */
	case LK_HEADCRUSH:			/* �w�b�h�N���b�V�� */
	case LK_JOINTBEAT:			/* �W���C���g�r�[�g */
	case PA_PRESSURE:			/* �v���b�V���[ */
	case CG_ARROWVULCAN:		/* �A���[�o���J�� */
	case ASC_BREAKER:			/* �\�E���u���[�J�[ */
	case HW_MAGICCRASHER:		/* �}�W�b�N�N���b�V���[ */
	case KN_BRANDISHSPEAR:		/* �u�����f�B�b�V���X�s�A */
	case PA_SHIELDCHAIN:		/* �V�[���h�`�F�C�� */
	case WS_CARTTERMINATION:	/* �J�[�g�^�[�~�l�[�V���� */
	case CR_ACIDDEMONSTRATION:	/* �A�V�b�h�f�����X�g���[�V���� */
	case ITM_TOMAHAWK:			/* �g�}�z�[�N���� */
		skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
		break;
	case NPC_DARKBREATH:
		clif_emotion(src,7);
		skill_attack(BF_MISC,src,src,bl,skillid,skilllv,tick,flag);
		break;
	case MO_INVESTIGATE:	/* ���� */
		{
			struct status_change *sc_data = status_get_sc_data(src);
			skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
			if(sc_data[SC_BLADESTOP].timer != -1)
				status_change_end(src,SC_BLADESTOP,-1);
		}
		break;
	case SN_FALCONASSAULT:			/* �t�@���R���A�T���g */
		if(!pc_isfalcon(sd)) break;
		skill_attack(BF_MISC,src,src,bl,skillid,skilllv,tick,flag);
		break;
	case RG_BACKSTAP:		/* �o�b�N�X�^�u */
		{
			int dir = map_calc_dir(src,bl->x,bl->y),t_dir = status_get_dir(bl);
			int dist = distance(src->x,src->y,bl->x,bl->y);
			if((dist > 0 && !map_check_dir(dir,t_dir)) || bl->type == BL_SKILL) {
				struct status_change *sc_data = status_get_sc_data(src);
				if(sc_data && sc_data[SC_HIDING].timer != -1)
					status_change_end(src, SC_HIDING, -1);	// �n�C�f�B���O����
				skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
			}
			else if(src->type == BL_PC)
				clif_skill_fail(sd,sd->skillid,0,0);
		}
		break;

	case AM_ACIDTERROR:		/* �A�V�b�h�e���[ */
		skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
		if(bl->type == BL_PC && atn_rand()%100 < skill_get_time(skillid,skilllv)) {
			pc_break_equip((struct map_session_data *)bl, EQP_ARMOR);
			clif_emotion(bl,23);
		}
		break;
	case MO_FINGEROFFENSIVE:	/* �w�e */
		{
			struct status_change *sc_data = status_get_sc_data(src);

			if(!battle_config.finger_offensive_type)
				skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
			else {
				skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
				if(sd) {
					for(i=1;i<sd->spiritball_old;i++)
						skill_addtimerskill(src,tick+i*200,bl->id,0,0,skillid,skilllv,BF_WEAPON,flag);
					sd->canmove_tick = tick + (sd->spiritball_old-1)*200;
				}
			}
			if(sc_data && sc_data[SC_BLADESTOP].timer != -1)
				status_change_end(src,SC_BLADESTOP,-1);
		}
		break;
	case MO_CHAINCOMBO:		/* �A�ŏ� */
		{
			struct status_change *sc_data = status_get_sc_data(src);
			skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
			if(sc_data && sc_data[SC_BLADESTOP].timer != -1)
				status_change_end(src,SC_BLADESTOP,-1);
		}
		break;
	case TK_STORMKICK:
	case TK_DOWNKICK:
	case TK_TURNKICK:
	case TK_COUNTER:
		skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
		break;
	case TK_JUMPKICK:
	{
			if(sd) {
				struct walkpath_data wpd;
				int dx,dy;

				dx = bl->x - sd->bl.x;
				dy = bl->y - sd->bl.y;
				if(dx > 0) dx++;
				else if(dx < 0) dx--;
				if(dy > 0) dy++;
				else if(dy < 0) dy--;
				if(dx == 0 && dy == 0) dx++;
				if(path_search(&wpd,src->m,sd->bl.x,sd->bl.y,sd->bl.x+dx,sd->bl.y+dy,1) == -1) {
					dx = bl->x - sd->bl.x;
					dy = bl->y - sd->bl.y;
					if(path_search(&wpd,src->m,sd->bl.x,sd->bl.y,sd->bl.x+dx,sd->bl.y+dy,1) == -1) {
						clif_skill_fail(sd,sd->skillid,0,0);
						break;
					}
				}
				sd->to_x = sd->bl.x + dx;
				sd->to_y = sd->bl.y + dy;
				skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
				clif_walkok(sd);
				clif_movechar(sd);
				if(dx < 0) dx = -dx;
				if(dy < 0) dy = -dy;
				sd->attackabletime = sd->canmove_tick = tick + 100 + sd->speed * ((dx > dy)? dx:dy);
				if(sd->canact_tick < sd->canmove_tick)
					sd->canact_tick = sd->canmove_tick;
				pc_movepos(sd,sd->to_x,sd->to_y);
			}
			else
				skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
		break;
	}
	case MO_COMBOFINISH:	/* �җ��� */
	case CH_TIGERFIST:		/* ���Ռ� */
	case CH_CHAINCRUSH:		/* �A������ */
	case CH_PALMSTRIKE:		/* �ҌՍd�h�R */
		skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
		break;
	case MO_EXTREMITYFIST:	/* ���C���e�P�� */
		{
			struct status_change *sc_data = status_get_sc_data(src);

			if(sd) {
				struct walkpath_data wpd;
				int dx,dy;

				dx = bl->x - sd->bl.x;
				dy = bl->y - sd->bl.y;
				if(dx > 0) dx++;
				else if(dx < 0) dx--;
				if(dy > 0) dy++;
				else if(dy < 0) dy--;
				if(dx == 0 && dy == 0) dx++;
				if(path_search(&wpd,src->m,sd->bl.x,sd->bl.y,sd->bl.x+dx,sd->bl.y+dy,1) == -1) {
					dx = bl->x - sd->bl.x;
					dy = bl->y - sd->bl.y;
					if(path_search(&wpd,src->m,sd->bl.x,sd->bl.y,sd->bl.x+dx,sd->bl.y+dy,1) == -1) {
						clif_skill_fail(sd,sd->skillid,0,0);
						break;
					}
				}
				sd->to_x = sd->bl.x + dx;
				sd->to_y = sd->bl.y + dy;
				skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
				clif_walkok(sd);
				clif_movechar(sd);
				if(dx < 0) dx = -dx;
				if(dy < 0) dy = -dy;
				sd->attackabletime = sd->canmove_tick = tick + 100 + sd->speed * ((dx > dy)? dx:dy);
				if(sd->canact_tick < sd->canmove_tick)
					sd->canact_tick = sd->canmove_tick;
				pc_movepos(sd,sd->to_x,sd->to_y);
				status_change_end(&sd->bl,SC_COMBO,-1);
			}
			else
				skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
			status_change_end(src, SC_EXPLOSIONSPIRITS, -1);
			if(sc_data && sc_data[SC_BLADESTOP].timer != -1)
				status_change_end(src,SC_BLADESTOP,-1);
		}
		break;
	/* ����n�͈͍U���X�L�� */
	case AC_SHOWER:			/* �A���[�V�����[ */
	case SM_MAGNUM:			/* �}�O�i���u���C�N */
	case AS_GRIMTOOTH:		/* �O�����g�D�[�X */
	case MC_CARTREVOLUTION:	/* �J�[�g�����H�����[�V���� */
	case NPC_SPLASHATTACK:	/* �X�v���b�V���A�^�b�N */
	case AS_SPLASHER:		/* �x�i���X�v���b�V���[ */
		if(flag&1){
			/* �ʂɃ_���[�W��^���� */
			if(bl->id!=skill_area_temp[1])
				skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0x0500);
		}else{
			int ar=2;
			int x=bl->x,y=bl->y;
			switch (skillid) {
				case SM_MAGNUM:			/* �}�O�i���u���C�N */
					x = src->x;
					y = src->y;
					break;
				case AC_SHOWER:			/* �A���[�V�����[ */
					break;
				case NPC_SPLASHATTACK:	/* �X�v���b�V���A�^�b�N */
					ar=3;
					break;
				case AS_SPLASHER:		/* �x�i���X�v���b�V���[ */
					break;
			}
			skill_area_temp[1]=bl->id;
			skill_area_temp[2]=x;
			skill_area_temp[3]=y;
			if (skillid==SM_MAGNUM) {
				/* �X�L���G�t�F�N�g�\�� */
				clif_skill_nodamage(src,bl,skillid,skilllv,1);
			} else {
				/* �܂��^�[�Q�b�g�ɍU���������� */
				skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0);
			}
			/* ���̌�^�[�Q�b�g�ȊO�͈͓̔��̓G�S�̂ɏ������s�� */
			map_foreachinarea(skill_area_sub,
				bl->m,x-ar,y-ar,x+ar,y+ar,0,
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_damage_id);
		}
		break;

	case KN_BOWLINGBASH:	/* �{�E�����O�o�b�V�� */
		if(flag&1){
			/* �ʂɃ_���[�W��^���� */
			if(bl->id!=skill_area_temp[1])
				skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0x0500);
		} else {
				int i,c;	/* ���l���畷���������Ȃ̂ŊԈ���Ă�\���偕������������������ */
				/* �܂��^�[�Q�b�g�ɍU���������� */
				if (!skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0))
					break;
				c = skill_get_blewcount(skillid,skilllv);
				if(map[bl->m].flag.gvg) c = 0;
				for(i=0;i<c;i++){
					skill_blown(src,bl,1);
					if(bl->type == BL_MOB)
						clif_fixmobpos((struct mob_data *)bl);
					else if(bl->type == BL_PET)
						clif_fixpetpos((struct pet_data *)bl);
					else
						clif_fixpos(bl);
					skill_area_temp[0]=0;
					map_foreachinarea(skill_area_sub,
						bl->m,bl->x-1,bl->y-1,bl->x+1,bl->y+1,0,
						src,skillid,skilllv,tick, flag|BCT_ENEMY ,
						skill_area_sub_count);
					if(skill_area_temp[0]>1) break;
				}
				skill_area_temp[1]=bl->id;
				/* ���̌�^�[�Q�b�g�ȊO�͈͓̔��̓G�S�̂ɏ������s�� */
				map_foreachinarea(skill_area_sub,
					bl->m,bl->x-1,bl->y-1,bl->x+1,bl->y+1,0,
					src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
					skill_castend_damage_id);
		}
		break;
	case KN_SPEARSTAB:		/* �X�s�A�X�^�u */
		if(flag&1){
			/* �ʂɃ_���[�W��^���� */
			if (bl->id==skill_area_temp[1])
				break;
			if (skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0x0500))
				skill_blown(src,bl,skill_area_temp[2]);
		} else {
			int x=bl->x,y=bl->y,i,dir;
			/* �܂��^�[�Q�b�g�ɍU���������� */
			dir = map_calc_dir(bl,src->x,src->y);
			skill_area_temp[1] = bl->id;
			skill_area_temp[2] = skill_get_blewcount(skillid,skilllv)|dir<<20;
			if (skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0))
				skill_blown(src,bl,skill_area_temp[2]);
			for (i=0;i<4;i++) {
				map_foreachinarea(skill_area_sub,bl->m,x,y,x,y,0,
					src,skillid,skilllv,tick,flag|BCT_ENEMY|1,
					skill_castend_damage_id);
				x += dirx[dir];
				y += diry[dir];
			}
		}
		break;
	case SN_SHARPSHOOTING:			/* �V���[�v�V���[�e�B���O */
		if(flag&1){
			/* �ʂɃ_���[�W��^���� */
			skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,(skill_area_temp[1] == 0 ? 0 : 0x0500));
			skill_area_temp[1]++;
		} else {
			int dir = map_calc_dir(src,bl->x,bl->y);
			skill_area_temp[1] = 0;
			map_foreachinpath(
				skill_area_sub,bl->m,src->x,src->y,src->x + 12*dirx[dir],src->y + 12*diry[dir],0,
				src,skillid,skilllv,tick,flag|BCT_ENEMY|1,skill_castend_damage_id
			);
			if(diry[dir] == 0) {
				map_foreachinpath(
					skill_area_sub,bl->m,src->x,src->y + 1,src->x + 12*dirx[dir],src->y + 12*diry[dir] + 1,0,
					src,skillid,skilllv,tick,flag|BCT_ENEMY|1,skill_castend_damage_id
				);
				map_foreachinpath(
					skill_area_sub,bl->m,src->x,src->y - 1,src->x + 12*dirx[dir],src->y + 12*diry[dir] - 1,0,
					src,skillid,skilllv,tick,flag|BCT_ENEMY|1,skill_castend_damage_id
				);
			} else {
				map_foreachinpath(
					skill_area_sub,bl->m,src->x + 1,src->y,src->x + 12*dirx[dir] + 1,src->y + 12*diry[dir],0,
					src,skillid,skilllv,tick,flag|BCT_ENEMY|1,skill_castend_damage_id
				);
				map_foreachinpath(
					skill_area_sub,bl->m,src->x - 1,src->y,src->x + 12*dirx[dir] - 1,src->y + 12*diry[dir],0,
					src,skillid,skilllv,tick,flag|BCT_ENEMY|1,skill_castend_damage_id
				);
			}
			if(skill_area_temp[1] == 0) {
				/* �^�[�Q�b�g�ɍU�� */
				skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0);
			}
		}
		break;
	case ALL_RESURRECTION:		/* ���U���N�V���� */
	case PR_TURNUNDEAD:			/* �^�[���A���f�b�h */
		if(bl->type != BL_PC && battle_check_undead(status_get_race(bl),status_get_elem_type(bl)))
			skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,flag);
		else {
			map_freeblock_unlock();
			return 1;
		}
		break;

	/* ���@�n�X�L�� */
	case MG_SOULSTRIKE:			/* �\�E���X�g���C�N */
	case NPC_DARKSOULSTRIKE:	/* �Ń\�E���X�g���C�N */
	case MG_COLDBOLT:			/* �R�[���h�{���g */
	case MG_FIREBOLT:			/* �t�@�C�A�[�{���g */
	case MG_LIGHTNINGBOLT:		/* ���C�g�j���O�{���g*/
	case WZ_EARTHSPIKE:			/* �A�[�X�X�p�C�N */
	case AL_HEAL:				/* �q�[�� */
	case AL_HOLYLIGHT:			/* �z�[���[���C�g */
	case WZ_JUPITEL:			/* ���s�e���T���_�[ */
	case NPC_DARKJUPITEL:		/* �Ń��s�e�� */
	case NPC_MAGICALATTACK:		/* MOB:���@�Ō��U�� */
	case PR_ASPERSIO:			/* �A�X�y���V�I */
	case HW_NAPALMVULCAN:		/* �i�p�[���o���J�� */
	case SL_SMA: //�G�X�}
	case SL_STUN: //�G�X�^��
	case SL_STIN: //�G�X�e�B��
		skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,flag);
		break;
/*	case MG_COLDBOLT:			// �R�[���h�{���g
	case MG_FIREBOLT:			// �t�@�C�A�[�{���g
	case MG_LIGHTNINGBOLT:		// ���C�g�j���O�{���g
		skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,flag);
		//�_�u���L���X�e�B���O
		if(sd && sd->sc_data && sd->sc_data[SC_DOUBLECASTING].timer!=-1)
		{
			if(atn_rand()%10 < (pc_checkskill(sd,PF_DOUBLECASTING)+3))
				skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,flag);
		}
		break; */
	case CG_TAROTCARD:		/*�^���̃^���b�g�J�[�h*/
		skill_tarot_card_of_fate(src,bl,skillid,skilllv,tick,flag,0);
		//return 1;
		break;
	case MG_FROSTDIVER:		/* �t���X�g�_�C�o�[ */
	{
		struct status_change *sc_data = status_get_sc_data(bl);
		int sc_def_mdef, rate, damage, eff_tick;
		sc_def_mdef = 100 - (3 + status_get_mdef(bl) + status_get_luk(bl)/3);
		rate = (skilllv*3+35)*sc_def_mdef/100-(status_get_int(bl)+status_get_luk(bl))/15;
		rate = rate<=5?5:rate;
		if (sc_data && sc_data[SC_FREEZE].timer != -1) {
			skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,flag);
			if (sd)
				clif_skill_fail(sd,skillid,0,0);
			break;
		}
		damage = skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,flag);
		if (status_get_hp(bl) > 0 && damage > 0 && atn_rand()%100 < rate) {
			eff_tick = skill_get_time2(skillid,skilllv)*(1-sc_def_mdef/100);
			status_change_start(bl,SC_FREEZE,skilllv,0,0,0,eff_tick,0);
		} else if (sd) {
			clif_skill_fail(sd,skillid,0,0);
		}
		break;
	}
	case WZ_WATERBALL:			/* �E�H�[�^�[�{�[�� */
		skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,flag);
		if (skilllv>1) {
			int cnt,range;
			range = skilllv>5?2:skilllv/2;
			if(sd && !map[sd->bl.m].flag.rain)
				cnt = skill_count_water(src,range)-1;
			else
				cnt = skill_get_num(skillid,skilllv)-1;
			if (cnt>0)
				skill_addtimerskill(src,tick+150,bl->id,0,0,
					skillid,skilllv,cnt,flag);
		}
		break;

	case PR_BENEDICTIO:			/* ���̍~�� */
		{
		int race=status_get_race(bl);
		if(battle_check_undead(race,status_get_elem_type(bl)) || race == 6)
			skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,flag);
		}
		break;

	/* ���@�n�͈͍U���X�L�� */
	case MG_NAPALMBEAT:			/* �i�p�[���r�[�g */
	case MG_FIREBALL:			/* �t�@�C���[�{�[�� */
	case WZ_SIGHTRASHER:		/* �T�C�g���b�V���[ */
	case WZ_FROSTNOVA:			/* �t���X�g�m���@ */
		if (flag&1) {
			/* �ʂɃ_���[�W��^���� */
			if(bl->id!=skill_area_temp[1]){
				if(skillid==MG_FIREBALL){
					/* �t�@�C���[�{�[���Ȃ璆�S����̋������v�Z */
					int dx=abs(bl->x - skill_area_temp[2]);
					int dy=abs(bl->y - skill_area_temp[3]);
					skill_area_temp[0]=((dx>dy)?dx:dy);
				}
				skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,
						skill_area_temp[0]| 0x0500);
			}
		} else {
			int ar;
			skill_area_temp[0]=0;
			skill_area_temp[1]=bl->id;
			switch (skillid) {
				case MG_NAPALMBEAT:
					ar = 1;
					/* �i�p�[���r�[�g�͕��U�_���[�W�Ȃ̂œG�̐��𐔂��� */
					map_foreachinarea(skill_area_sub,
							bl->m,bl->x-ar,bl->y-ar,bl->x+ar,bl->y+ar,0,
							src,skillid,skilllv,tick,flag|BCT_ENEMY,
							skill_area_sub_count);
					break;
				case MG_FIREBALL:
					ar = 2;
					skill_area_temp[2]=bl->x;
					skill_area_temp[3]=bl->y;
					/* �^�[�Q�b�g�ɍU����������(�X�L���G�t�F�N�g�\��) */
					skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,
							skill_area_temp[0]);
					break;
				case WZ_FROSTNOVA:
					ar = 2;
					skill_area_temp[2]=bl->x;
					skill_area_temp[3]=bl->y;
					bl = src;
					break;
				case WZ_SIGHTRASHER:
				default:
					ar = 7;
					bl = src;
					status_change_end(src,SC_SIGHT,-1);
					break;
			}
			if (skillid==WZ_SIGHTRASHER || skillid==WZ_FROSTNOVA) {
				/* �X�L���G�t�F�N�g�\�� */
				clif_skill_nodamage(src,bl,skillid,skilllv,1);
			} else {
				/* �^�[�Q�b�g�ɍU����������(�X�L���G�t�F�N�g�\��) */
				skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,
						skill_area_temp[0]);
			}
			/* �^�[�Q�b�g�ȊO�͈͓̔��̓G�S�̂ɏ������s�� */
			map_foreachinarea(skill_area_sub,
					bl->m,bl->x-ar,bl->y-ar,bl->x+ar,bl->y+ar,0,
					src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
					skill_castend_damage_id);
		}
		break;

	/* ���̑� */
	case HT_BLITZBEAT:			/* �u���b�c�r�[�g */
		if(!pc_isfalcon(sd)) break;
		if(flag&1){
			/* �ʂɃ_���[�W��^���� */
			if(bl->id!=skill_area_temp[1])
				skill_attack(BF_MISC,src,src,bl,skillid,skilllv,tick,skill_area_temp[0]|(flag&0xf00000));
		}else{
			skill_area_temp[0]=0;
			skill_area_temp[1]=bl->id;
			if(flag&0xf00000)
				map_foreachinarea(skill_area_sub,bl->m,bl->x-1,bl->y-1,bl->x+1,bl->y+1,0,
					src,skillid,skilllv,tick, flag|BCT_ENEMY ,skill_area_sub_count);
			/* �܂��^�[�Q�b�g�ɍU���������� */
			skill_attack(BF_MISC,src,src,bl,skillid,skilllv,tick,skill_area_temp[0]|(flag&0xf00000));
			/* ���̌�^�[�Q�b�g�ȊO�͈͓̔��̓G�S�̂ɏ������s�� */
			map_foreachinarea(skill_area_sub,
				bl->m,bl->x-1,bl->y-1,bl->x+1,bl->y+1,0,
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_damage_id);
		}
		break;

	case CR_GRANDCROSS:			/* �O�����h�N���X */
	case NPC_DARKGRANDCROSS:		/*�ŃO�����h�N���X*/
		/* �X�L�����j�b�g�z�u */
		skill_castend_pos2(src,bl->x,bl->y,skillid,skilllv,tick,0);
		if(sd)
			sd->canmove_tick = tick + 1000;
		else if(src->type == BL_MOB)
			mob_changestate((struct mob_data *)src,MS_DELAY,1000);
		break;
	case PF_SOULBURN:		/* �\�E���o�[�� */
		if (sd && bl->type==BL_PC && (map[src->m].flag.pvp || map[src->m].flag.gvg)) {
			struct map_session_data *tsd = (struct map_session_data *)bl;
			int sp,mdef1,mdef2,damage;
			if (atn_rand()%100>=(skilllv==5?70:10*skilllv+30)) {
				clif_skill_fail(sd,skillid,0,0);
				if (skilllv<5)
					break;
				tsd = sd;	// �����ɑ΂��ă_���[�W
				clif_skill_nodamage(src,src,skillid,skilllv,1);
			} else
				clif_skill_nodamage(src,bl,skillid,skilllv,1);
			// SP��0�ɂ���
			sp = tsd->status.sp;
			pc_heal(tsd,0,-sp);
			if (skilllv<5)
				break;
			// SP*2�̃_���[�W��^����(MDEF�Ōv�Z)
			mdef1 = tsd->mdef;
			mdef2 = tsd->mdef2 + (tsd->paramc[2]>>1);
			damage = sp*2;
			damage = (damage*(100-mdef1))/100 - mdef2;
			if (damage<1)
				damage = 1;
			battle_damage(src,&tsd->bl,damage,0);
		}
		break;
	case TF_THROWSTONE:			/* �Γ��� */
	case NPC_SMOKING:			/* �X���[�L���O */
		skill_attack(BF_MISC,src,src,bl,skillid,skilllv,tick,0 );
		break;

	case NPC_SELFDESTRUCTION:	/* ���� */
	case NPC_SELFDESTRUCTION2:	/* ����2 */
		if(flag&1){
			/* �ʂɃ_���[�W��^���� */
			if(src->type==BL_MOB){
				struct mob_data* mb = (struct mob_data*)src;
				nullpo_retr(1, mb);
				mb->hp=skill_area_temp[2];
				if(bl->id!=skill_area_temp[1])
					skill_attack(BF_MISC,src,src,bl,NPC_SELFDESTRUCTION,skilllv,tick,flag );
				mb->hp=1;
			}
		}else{
			struct mob_data *md;
			if((md=(struct mob_data *)src)){
				skill_area_temp[1]=bl->id;
				skill_area_temp[2]=status_get_hp(src);
				clif_skill_nodamage(src,src,NPC_SELFDESTRUCTION,-1,1);
				map_foreachinarea(skill_area_sub,
					bl->m,bl->x-5,bl->y-5,bl->x+5,bl->y+5,0,
					src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
					skill_castend_damage_id);
				battle_damage(src,src,md->hp,0);
			}
		}
		break;

	/* HP�z��/HP�z�����@ */
	case NPC_BLOODDRAIN:
	case NPC_ENERGYDRAIN:
		{
			int heal;
			heal = skill_attack((skillid==NPC_BLOODDRAIN)?BF_WEAPON:BF_MAGIC,src,src,bl,skillid,skilllv,tick,flag);
			if( heal > 0 ){
				struct block_list tbl;
				tbl.id = 0;
				tbl.m = src->m;
				tbl.x = src->x;
				tbl.y = src->y;
				clif_skill_nodamage(&tbl,src,AL_HEAL,heal,1);
				battle_heal(NULL,src,heal,0,0);
			}
		}
		break;
	case 0:
		if(sd) {
			if(flag&3){
				if(bl->id!=skill_area_temp[1])
					skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0x0500);
			}
			else{
				int ar=sd->splash_range;
				skill_area_temp[1]=bl->id;
				map_foreachinarea(skill_area_sub,
					bl->m, bl->x - ar, bl->y - ar, bl->x + ar, bl->y + ar, 0,
					src, skillid, skilllv, tick, flag | BCT_ENEMY | 1,
					skill_castend_damage_id);
			}
		}
		break;
	default:
		map_freeblock_unlock();
		return 1;
	}
	map_freeblock_unlock();

	return 0;
}

/*==========================================
 * �X�L���g�p�i�r�������AID�w��x���n�j
 *------------------------------------------
 */
int skill_castend_nodamage_id( struct block_list *src, struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag )
{
	struct map_session_data *sd=NULL;
	struct map_session_data *dstsd=NULL;
	struct mob_data *md=NULL;
	struct mob_data *dstmd=NULL;
	struct status_change *sc_data;
	int i,abra_skillid=0,abra_skilllv;
	int sc_def_vit,sc_def_mdef,strip_time,strip_per;

	//�N���X�`�F���W�p�{�X�����X�^�[ID
	int changeclass[]={1038,1039,1046,1059,1086,1087,1112,1115
				,1157,1159,1190,1272,1312,1373,1492};
	int poringclass[]={1002,1002};

	nullpo_retr(1, src);
	nullpo_retr(1, bl);

	if(src->type==BL_PC)
		sd=(struct map_session_data *)src;
	else if(src->type==BL_MOB)
		md=(struct mob_data *)src;
	
	sc_def_vit = 100 - (3 + status_get_vit(bl) + status_get_luk(bl)/3);
	sc_def_mdef = 100 - (3 + status_get_mdef(bl) + status_get_luk(bl)/3);

	if(bl->type==BL_PC){
		nullpo_retr(1, dstsd=(struct map_session_data *)bl);
	}else if(bl->type==BL_MOB){
		nullpo_retr(1, dstmd=(struct mob_data *)bl);
		if(sc_def_vit>50)
			sc_def_vit=50;
		if(sc_def_mdef>50)
			sc_def_mdef=50;
	}
	if(sc_def_vit < 0)
		sc_def_vit=0;
	if(sc_def_mdef < 0)
		sc_def_mdef=0;

	if(bl == NULL || bl->prev == NULL)
		return 1;
	if(sd && pc_isdead(sd))
		return 1;
	if(dstsd && pc_isdead(dstsd) && skillid != ALL_RESURRECTION)
		return 1;
	if(status_get_class(bl) == 1288)
		return 1;

	map_freeblock_lock();
	switch(skillid)
	{
	case AL_HEAL:				/* �q�[�� */
		{
			int heal=skill_calc_heal( src, skilllv );
			int heal_get_jobexp;
			int skill;
			struct pc_base_job s_class;
			sc_data=status_get_sc_data(bl);
			if(battle_config.heal_counterstop){
				if(skilllv>=battle_config.heal_counterstop)
					heal=9999; //9999�q�[��
			}
			if( dstsd && dstsd->special_state.no_magic_damage )
				heal=0;	/* ����峃J�[�h�i�q�[���ʂO�j */
			if(sc_data && sc_data[SC_BERSERK].timer!=-1) /* �o�[�T�[�N���̓q�[���O */
				heal=0;
			if (sd){
				s_class = pc_calc_base_job(sd->status.class);
				if((skill=pc_checkskill(sd,HP_MEDITATIO))>0) // ���f�B�e�C�e�B�I 
					heal += heal*(skill*2)/100;
				if(sd && dstsd && sd->status.partner_id == dstsd->status.char_id && s_class.job == 23 && sd->sex == 0) //�������Ώۂ�PC�A�Ώۂ������̃p�[�g�i�[�A�������X�p�m�r�A���������Ȃ�
					heal = heal*2;	//�X�p�m�r�̉ł��U�߂Ƀq�[�������2�{�ɂȂ�
			}
			

			clif_skill_nodamage(src,bl,skillid,heal,1);
			heal_get_jobexp = battle_heal(NULL,bl,heal,0,0);

			// JOB�o���l�l��
			if(src->type == BL_PC && bl->type==BL_PC && heal > 0 && src != bl && battle_config.heal_exp > 0){
				heal_get_jobexp = heal_get_jobexp * battle_config.heal_exp / 100;
				if(heal_get_jobexp <= 0)
					heal_get_jobexp = 1;
				pc_gainexp((struct map_session_data *)src,0,heal_get_jobexp);
			}
		}
		break;

	case ALL_RESURRECTION:		/* ���U���N�V���� */
		if(dstsd){
			int per=0;
			if( (map[bl->m].flag.pvp) && dstsd->pvp_point<0 )
				break;			/* PVP�ŕ����s�\��� */

			if(pc_isdead(dstsd)){	/* ���S���� */
				clif_skill_nodamage(src,bl,skillid,skilllv,1);
				switch(skilllv){
				case 1: per=10; break;
				case 2: per=30; break;
				case 3: per=50; break;
				case 4: per=80; break;
				}
				dstsd->status.hp=dstsd->status.max_hp*per/100;
				if(dstsd->status.hp<=0) dstsd->status.hp=1;
				if(dstsd->special_state.restart_full_recover ){	/* �I�V���X�J�[�h */
					dstsd->status.hp=dstsd->status.max_hp;
					dstsd->status.sp=dstsd->status.max_sp;
				}
				pc_setstand(dstsd);
				if(battle_config.pc_invincible_time > 0)
					pc_setinvincibletimer(dstsd,battle_config.pc_invincible_time);
				clif_updatestatus(dstsd,SP_HP);
				clif_resurrection(&dstsd->bl,1);
				if(src != bl && sd && battle_config.resurrection_exp > 0) {
					int exp = 0,jexp = 0;
					int lv = dstsd->status.base_level - sd->status.base_level, jlv = dstsd->status.job_level - sd->status.job_level;
					if(lv > 0) {
						exp = (int)((double)dstsd->status.base_exp * (double)lv * (double)battle_config.resurrection_exp / 1000000.);
						if(exp < 1) exp = 1;
					}
					if(jlv > 0) {
						jexp = (int)((double)dstsd->status.job_exp * (double)lv * (double)battle_config.resurrection_exp / 1000000.);
						if(jexp < 1) jexp = 1;
					}
					if(exp > 0 || jexp > 0)
						pc_gainexp(sd,exp,jexp);
				}
			}
		}
		break;

	case AL_DECAGI:			/* ���x���� */
		if( bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage )
			break;
		if( atn_rand()%100 < (50+skilllv*3+(status_get_lv(src)+status_get_int(src)/5)-sc_def_mdef) ) {
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		}
		break;

	case AL_CRUCIS:
		if(flag&1) {
			int race = status_get_race(bl),ele = status_get_elem_type(bl);
			if(battle_check_target(src,bl,BCT_ENEMY) && (race == 6 || battle_check_undead(race,ele))) {
				int slv=status_get_lv(src),tlv=status_get_lv(bl),rate;
				rate = 23 + skilllv*4 + slv - tlv;
				if(atn_rand()%100 < rate)
					status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,0,0);
			}
		}
		else {
			int range = 15;
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			map_foreachinarea(skill_area_sub,
				src->m,src->x-range,src->y-range,src->x+range,src->y+range,0,
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_nodamage_id);
		}
		break;

	case PR_LEXDIVINA:		/* ���b�N�X�f�B�r�[�i */
		{
			struct status_change *sc_data = status_get_sc_data(bl);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if( bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage )
				break;
			if(sc_data && sc_data[SC_DIVINA].timer != -1)
				status_change_end(bl,SC_DIVINA,-1);
			else if( atn_rand()%100 < sc_def_vit ) {
				status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
			}
		}
		break;
	case SA_ABRACADABRA:
		if(map[src->m].flag.noabra)
			break;
		do{
			abra_skillid=skill_abra_dataset(skilllv);
		}while(abra_skillid == 0);
		abra_skilllv=skill_get_max(abra_skillid)>pc_checkskill(sd,SA_ABRACADABRA)?pc_checkskill(sd,SA_ABRACADABRA):skill_get_max(abra_skillid);
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		sd->skillitem=abra_skillid;
		sd->skillitemlv=abra_skilllv;
		clif_item_skill(sd,abra_skillid,abra_skilllv,"�A�u���J�_�u��");
		break;
	case SA_COMA:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage )
			break;
		if(dstsd){
			dstsd->status.hp=1;
			dstsd->status.sp=1;
			clif_updatestatus(dstsd,SP_HP);
			clif_updatestatus(dstsd,SP_SP);
		}
		if(dstmd) dstmd->hp=1;
		break;
	case SA_FULLRECOVERY:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage )
			break;
		if(dstsd) pc_heal(dstsd,dstsd->status.max_hp,dstsd->status.max_sp);
		if(dstmd) dstmd->hp=status_get_max_hp(&dstmd->bl);
		break;
	case SA_SUMMONMONSTER:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if (sd) mob_once_spawn(sd,map[sd->bl.m].name,sd->bl.x,sd->bl.y,"--ja--",-1,1,"");
		break;
	case SA_LEVELUP:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if (sd && pc_nextbaseexp(sd)) pc_gainexp(sd,pc_nextbaseexp(sd)*10/100,0);
		break;

	case SA_INSTANTDEATH:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if (sd) pc_damage(NULL,sd,sd->status.max_hp);
		break;

	case SA_QUESTION:
	case SA_GRAVITY:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		break;
	case SA_CLASSCHANGE:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(dstmd) mob_class_change(dstmd,changeclass,sizeof(changeclass)/sizeof(int));
		break;
	case SA_MONOCELL:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(dstmd) mob_class_change(dstmd,poringclass,sizeof(poringclass)/sizeof(int));
		break;
	case SA_DEATH:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if (dstsd) pc_damage(NULL,dstsd,dstsd->status.max_hp);
		if (dstmd) mob_damage(NULL,dstmd,dstmd->hp,1);
		break;
	case SA_REVERSEORCISH:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if (dstsd) pc_setoption(dstsd,dstsd->status.option|0x0800);
		break;
	case SA_FORTUNE:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(sd) pc_getzeny(sd,status_get_lv(bl)*100);
		break;
	case SA_TAMINGMONSTER:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if (dstmd){
			for(i=0;i<MAX_PET_DB;i++){
				if(dstmd->class == pet_db[i].class){
					pet_catch_process1(sd,dstmd->class);
					break;
				}
			}
		}
		break;
	case AL_INCAGI:			/* ���x���� */
	case AL_BLESSING:		/* �u���b�V���O */
	case PR_SLOWPOISON:
	case PR_IMPOSITIO:		/* �C���|�V�e�B�I�}�k�X */
	case PR_LEXAETERNA:		/* ���b�N�X�G�[�e���i */
	case PR_SUFFRAGIUM:		/* �T�t���M�E�� */
	case PR_BENEDICTIO:		/* ���̍~�� */
	case CR_PROVIDENCE:		/* �v�����B�f���X */
	case SA_FLAMELAUNCHER:	/* �t���C�������`���[ */
	case SA_FROSTWEAPON:	/* �t���X�g�E�F�|�� */
	case SA_LIGHTNINGLOADER:/* ���C�g�j���O���[�_�[ */
	case SA_SEISMICWEAPON:	/* �T�C�Y�~�b�N�E�F�|�� */
	case CG_MARIONETTE:		/* �}���I�l�b�g�R���g���[�� */
		if( dstsd && dstsd->special_state.no_magic_damage ){
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}else{
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;
	case TK_RUN://�삯��
		if(sd && sd->sc_data)
		{
			if(sd->sc_data[SC_RUN].timer!=-1)	
			{
				if(skilllv>=7 && sd->weapontype1 == 0 && sd->weapontype2 == 0)
					status_change_start(&dstsd->bl,SC_RUN_STR,10,0,0,0,150000,0);
				status_change_end(bl,SC_RUN,-1);
			}else{
				//clif_skill_nodamage(src,bl,skillid,skilllv,1);
				status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
			}
		}
		break;
		/*
	case TK_READYSTORM:
		if(sd)//��������ON/OFF
		{
			char output[64];
			if(sd->sc_data[].timer!=-1)
			{
				sd->tk_readystorm_on = 0;
				strcpy(output,"��������:OFF");
			}else{ 
				sd->tk_readystorm_on = 1;
				strcpy(output,"��������:ON");
			}
			clif_disp_onlyself(sd,output,strlen(output));
		}
		break;
	case TK_READYDOWN:
		if(sd)//���i����ON/OFF
		{
			char output[64];
			if(sd->tk_readydown_on)
			{
				sd->tk_readydown_on = 0;
				strcpy(output,"���i����:OFF");
			}else{ 
				sd->tk_readydown_on = 1;
				strcpy(output,"���i����:ON");
			}
			clif_disp_onlyself(sd,output,strlen(output));
		}
		break;
	case TK_READYTURN:
		if(sd)//��]����ON/OFF
		{
			char output[64];
			if(sd->tk_readyturn_on)
			{
				sd->tk_readyturn_on = 0;
				strcpy(output,"��]����:OFF");
			}else{ 
				sd->tk_readyturn_on = 1;
				strcpy(output,"��]����:ON");
			}
			clif_disp_onlyself(sd,output,strlen(output));
		}
		break;
	case TK_READYCOUNTER:
		if(sd)//�J�E���^�[����ON/OFF
		{
			char output[64];
			if(sd->tk_readycounter_on)
			{
				sd->tk_readycounter_on = 0;
				strcpy(output,"�J�E���^�[����:OFF");
			}else{ 
				sd->tk_readycounter_on = 1;
				strcpy(output,"�J�E���^�[����:ON");
			}
			clif_disp_onlyself(sd,output,strlen(output));
		}
		break;
	case TK_DODGE:
		if(sd)//���@ON/OFF
		{
			char output[64];
			if(sd->tk_dodge_on)
			{
				sd->tk_dodge_on = 0;
				strcpy(output,"���@:OFF");
			}else{ 
				sd->tk_dodge_on = 1;
				strcpy(output,"���@:ON");
			}
			clif_disp_onlyself(sd,output,strlen(output));
		}
		break;
		*/
/* 	case PF_DOUBLECASTING: */
	case TK_READYSTORM:
	case TK_READYDOWN:
	case TK_READYTURN:
	case TK_READYCOUNTER:
	case TK_DODGE:
		status_change_start(bl,GetSkillStatusChangeTable(skillid),skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		break;
	case SM_AUTOBERSERK:
		if(sd)//�I�[�g�o�[�T�[�NON/OFF
		{
			char output[64];
			if(sd->sc_data[SC_AUTOBERSERK].timer!=-1)
				strcpy(output,"�I�[�g�o�[�T�[�N:OFF");
			else
				strcpy(output,"�I�[�g�o�[�T�[�N:ON");
			clif_disp_onlyself(sd,output,strlen(output));
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			status_change_start(bl,GetSkillStatusChangeTable(skillid),skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		}
		break;
	case SG_SUN_WARM://���z�̉�����
	case SG_MOON_WARM://���̉�����
	case SG_STAR_WARM://���̉�����
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,GetSkillStatusChangeTable(skillid),skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
	case TK_SEVENWIND: //�g������
	case SL_KAIZEL://�J�C�[��
	case SL_KAAHI://�J�A�q
	case SL_KAITE://�J�C�g
	case SL_KAUPE://�J�E�v
	case SL_SWOO://�G�X�E
	case SL_SKA://�G�X�J
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		break;
	case SL_SKE://�G�X�N
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		status_change_start(bl,SC_SMA,skilllv,0,0,0,3000,0);
		break;
	case PR_ASPERSIO:		/* �A�X�y���V�I */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( dstsd && dstsd->special_state.no_magic_damage )
			break;
		if(bl->type==BL_MOB)
			break;
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		break;
	case PR_KYRIE:			/* �L���G�G���C�\�� */
		clif_skill_nodamage(bl,bl,skillid,skilllv,1);
		if( dstsd && dstsd->special_state.no_magic_damage )
			break;
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		break;
	case KN_AUTOCOUNTER:		/* �I�[�g�J�E���^�[ */
	case KN_TWOHANDQUICKEN:	/* �c�[�n���h�N�C�b�P�� */
	case CR_SPEARQUICKEN:	/* �X�s�A�N�C�b�P�� */
	case CR_REFLECTSHIELD:
	case AS_ENCHANTPOISON:	/* �G���`�����g�|�C�Y�� */
	case AS_POISONREACT:	/* �|�C�Y�����A�N�g */
	case MC_LOUD:			/* ���E�h�{�C�X */
	case MG_ENERGYCOAT:		/* �G�i�W�[�R�[�g */
	case MG_SIGHT:			/* �T�C�g */
	case AL_RUWACH:			/* ���A�t */
	case MO_EXPLOSIONSPIRITS:	// �����g��
	case MO_STEELBODY:		// ����
	case LK_AURABLADE:		/* �I�[���u���[�h */
	case LK_PARRYING:		/* �p���C���O */
	case HP_ASSUMPTIO:		/*  */
	case WS_CARTBOOST:		/* �J�[�g�u�[�X�g */
	case SN_SIGHT:			/* �g�D���[�T�C�g */
	case WS_MELTDOWN:		/* �����g�_�E�� */
	case ST_REJECTSWORD:	/* ���W�F�N�g�\�[�h */
	case HW_MAGICPOWER:		/* ���@�͑��� */
	case PF_MEMORIZE:		/* �������C�Y */
	case ASC_EDP:			/* �G���`�����g�f�b�h���[�|�C�Y�� */
	case PA_SACRIFICE:		/* �T�N���t�@�C�X */
	case ST_PRESERVE:		/* �v���U�[�u */
	case WS_OVERTHRUSTMAX:		/* �I�[�o�[�g���X�g�}�b�N�X */
	case KN_ONEHAND:	/* �c�[�n���h�N�C�b�P�� */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		break;
	case LK_CONCENTRATION:	/* �R���Z���g���[�V���� */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		if(sd) sd->skillstatictimer[SM_ENDURE] = tick; //�f�B���C�̓R���Z�Ɠ������炢�H
		status_change_start(bl,SkillStatusChangeTable[SM_ENDURE],1,0,0,0,skill_get_time(SM_ENDURE,skilllv),0 );
		break;
	case LK_BERSERK:		/* �o�[�T�[�N */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		sd->status.hp = sd->status.max_hp;
		clif_updatestatus(sd,SP_HP);
		break;
	case SM_ENDURE:			/* �C���f���A */
		if(sd) sd->skillstatictimer[SM_ENDURE] = tick + 10000;
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		break;
	case NPC_EXPLOSIONSPIRITS:	//NPC�����g��
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,SC_EXPLOSIONSPIRITS,skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		break;
	case NPC_INCREASEFLEE:	//���x����
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,SC_INCFLEE,skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		break;
	case LK_TENSIONRELAX:	/* �e���V���������b�N�X */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		pc_setsit(sd);
		clif_sitting(sd);
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		break;
	case MC_CHANGECART:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		break;
	case AC_CONCENTRATION:	/* �W���͌��� */
		{
			int range = 1;
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
			map_foreachinarea( status_change_timer_sub,
				src->m, src->x-range, src->y-range, src->x+range,src->y+range,0,
				src,SkillStatusChangeTable[skillid],tick);
		}
		break;
	case SM_PROVOKE:		/* �v���{�b�N */
		{
			struct status_change *sc_data = status_get_sc_data(bl);

			/* MVPmob�ƕs���ɂ͌����Ȃ� */
			if((bl->type==BL_MOB && status_get_mode(bl)&0x20) || battle_check_undead(status_get_race(bl),status_get_elem_type(bl))) //�s���ɂ͌����Ȃ�
			{
				map_freeblock_unlock();
				return 1;
			}

			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
			// �r���W�Q
			if(dstmd && dstmd->skilltimer!=-1 && dstmd->state.skillcastcancel)
				skill_castcancel(bl,0);
			if(dstsd && dstsd->skilltimer!=-1 && (!dstsd->special_state.no_castcancel || map[bl->m].flag.gvg)
				&& dstsd->state.skillcastcancel	&& !dstsd->special_state.no_castcancel2)
				skill_castcancel(bl,0);

			if(sc_data){
				if(sc_data[SC_FREEZE].timer!=-1)
					status_change_end(bl,SC_FREEZE,-1);
				if(sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2==0)
					status_change_end(bl,SC_STONE,-1);
				if(sc_data[SC_SLEEP].timer!=-1)
					status_change_end(bl,SC_SLEEP,-1);
			}

			if(bl->type==BL_MOB) {
				int range = skill_get_range(skillid,skilllv);
				if(range < 0)
					range = status_get_range(src) - (range + 1);
				mob_target((struct mob_data *)bl,src,range);
			}
		}
		break;

	case CR_DEVOTION:		/* �f�B�{�[�V���� */
		if(sd && dstsd){
			//�]����{�q�̏ꍇ�̌��̐E�Ƃ��Z�o����
			struct pc_base_job dst_s_class = pc_calc_base_job(dstsd->status.class);

			int lv = sd->status.base_level-dstsd->status.base_level;
			lv = (lv<0)?-lv:lv;
			if((dstsd->bl.type!=BL_PC)	// �����PC����Ȃ��Ƃ���
			 ||(sd->bl.id == dstsd->bl.id)	// ���肪�����͂���
			 ||(lv > battle_config.devotion_level_difference)	// ���x����
			 ||(!sd->status.party_id && !sd->status.guild_id)	// PT�ɂ��M���h�ɂ����������͂���
			 ||((sd->status.party_id != dstsd->status.party_id)	// �����p�[�e�B�[���A
			  &&(sd->status.guild_id != dstsd->status.guild_id))	// �����M���h����Ȃ��Ƃ���
			 ||(dst_s_class.job==14||dst_s_class.job==21)){	// �N���Z����
				clif_skill_fail(sd,skillid,0,0);
				map_freeblock_unlock();
				return 1;
			}
			for(i=0;i<skilllv;i++){
				if(!sd->dev.val1[i]){		// �󂫂�������������
					sd->dev.val1[i] = bl->id;
					sd->dev.val2[i] = bl->id;
					break;
				}else if(i==skilllv-1){		// �󂫂��Ȃ�����
					clif_skill_fail(sd,skillid,0,0);
					map_freeblock_unlock();
					return 1;
				}
			}
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			clif_devotion(sd,bl->id);
			status_change_start(bl,SkillStatusChangeTable[skillid],src->id,1,0,0,1000*(15+15*skilllv),0 );
		}
		else	clif_skill_fail(sd,skillid,0,0);
		break;
	case MO_CALLSPIRITS:	// �C��
		if(sd) {
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			pc_addspiritball(sd,skill_get_time(skillid,skilllv),skilllv);
		}
		break;
	case CH_SOULCOLLECT:	// ���C��
		if(sd) {
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			pc_delspiritball(sd,sd->spiritball,0);
			for(i=0;i<5;i++)
				pc_addspiritball(sd,skill_get_time(skillid,skilllv),5);
		}
		break;
	case MO_BLADESTOP:	// ���n���
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(src,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		break;
	case MO_ABSORBSPIRITS:	// �C�D
		i=0;
		if(sd && dstsd) {
			if(sd == dstsd || map[sd->bl.m].flag.pvp || map[sd->bl.m].flag.gvg) {
				if(dstsd->spiritball > 0) {
					clif_skill_nodamage(src,bl,skillid,skilllv,1);
					i = dstsd->spiritball * 7;
					pc_delspiritball(dstsd,dstsd->spiritball,0);
					if(i > 0x7FFF)
						i = 0x7FFF;
					if(sd->status.sp + i > sd->status.max_sp)
						i = sd->status.max_sp - sd->status.sp;
				}
			}
		}else if(sd && dstmd){ //�Ώۂ������X�^�[�̏ꍇ
			//20%�̊m���őΏۂ�Lv*2��SP���񕜂���B���������Ƃ��̓^�[�Q�b�g(�ЁK�D�K)�ЃQ�b�c!!
			if(atn_rand()%100<20){
				i=2*mob_db[dstmd->class].lv;
				mob_target(dstmd,src,0);
			}
		}
		if(i){
			sd->status.sp += i;
			clif_heal(sd->fd,SP_SP,i);
		}
		else
			clif_skill_nodamage(src,bl,skillid,skilllv,0);
		break;

	case AC_MAKINGARROW:		/* ��쐬 */
		if(sd) {
			clif_arrow_create_list(sd);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;

	case AM_PHARMACY:			/* �|�[�V�����쐬 */
		if(sd) {
			clif_skill_produce_mix_list(sd,32);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;
	case ASC_CDP:				/* �f�b�h���[�|�C�Y���쐬 */
		if(sd) {
			clif_skill_produce_mix_list(sd,256);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;
	case WS_CREATECOIN:			/* �N���G�C�g�R�C�� */
		if(sd) {
			clif_skill_produce_mix_list(sd,64);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;
	case WS_CREATENUGGET:			/* �򐻑� */
		if(sd) {
			clif_skill_produce_mix_list(sd,128);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;
	case BS_HAMMERFALL:		/* �n���}�[�t�H�[�� */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( dstsd && dstsd->special_state.no_weapon_damage )
			break;
		if( atn_rand()%100 < (20+ 10*skilllv)*sc_def_vit/100 ) {
			status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		}
		break;

	case RG_RAID:			/* �T�v���C�Y�A�^�b�N */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		map_foreachinarea(skill_area_sub,
			bl->m,bl->x-1,bl->y-1,bl->x+1,bl->y+1,0,
			src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
			skill_castend_damage_id);
		status_change_end(src, SC_HIDING, -1);	// �n�C�f�B���O����
		break;
	case ASC_METEORASSAULT:	/* ���e�I�A�T���g */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		map_foreachinarea(skill_area_sub,
			bl->m,bl->x-2,bl->y-2,bl->x+2,bl->y+2,0,
			src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
			skill_castend_damage_id);
		break;
	case KN_BRANDISHSPEAR:	/*�u�����f�B�b�V���X�s�A*/
		{
			int c,n=4,ar;
			int dir = map_calc_dir(src,bl->x,bl->y);
			struct square tc;
			int x=bl->x,y=bl->y;
			ar=skilllv/3;
			skill_brandishspear_first(&tc,dir,x,y);
			skill_brandishspear_dir(&tc,dir,4);
			/* �͈͇C */
			if(skilllv == 10){
				for(c=1;c<4;c++){
					map_foreachinarea(skill_area_sub,
						bl->m,tc.val1[c],tc.val2[c],tc.val1[c],tc.val2[c],0,
						src,skillid,skilllv,tick, flag|BCT_ENEMY|n,
						skill_castend_damage_id);
				}
			}
			/* �͈͇B�A */
			if(skilllv > 6){
				skill_brandishspear_dir(&tc,dir,-1);
				n--;
			}else{
				skill_brandishspear_dir(&tc,dir,-2);
				n-=2;
			}

			if(skilllv > 3){
				for(c=0;c<5;c++){
					map_foreachinarea(skill_area_sub,
						bl->m,tc.val1[c],tc.val2[c],tc.val1[c],tc.val2[c],0,
						src,skillid,skilllv,tick, flag|BCT_ENEMY|n,
						skill_castend_damage_id);
					if(skilllv > 6 && n==3 && c==4){
						skill_brandishspear_dir(&tc,dir,-1);
						n--;c=-1;
					}
				}
			}
			/* �͈͇@ */
			for(c=0;c<10;c++){
				if(c==0||c==5) skill_brandishspear_dir(&tc,dir,-1);
				map_foreachinarea(skill_area_sub,
					bl->m,tc.val1[c%5],tc.val2[c%5],tc.val1[c%5],tc.val2[c%5],0,
					src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
					skill_castend_damage_id);
			}
		}
		break;
	/* �p�[�e�B�X�L�� */
	case AL_ANGELUS:		/* �G���W�F���X */
	case PR_MAGNIFICAT:		/* �}�O�j�t�B�J�[�g */
	case PR_GLORIA:			/* �O�����A */
	case SN_WINDWALK:		/* �E�C���h�E�H�[�N */
		if(sd == NULL || sd->status.party_id==0 || (flag&1) ){
			/* �ʂ̏��� */
			clif_skill_nodamage(bl,bl,skillid,skilllv,1);
			if( dstsd && dstsd->special_state.no_magic_damage )
				break;
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		}
		else{
			/* �p�[�e�B�S�̂ւ̏��� */
			party_foreachsamemap(skill_area_sub,
				sd,1,
				src,skillid,skilllv,tick, flag|BCT_PARTY|1,
				skill_castend_nodamage_id);
		}
		break;
	case BS_ADRENALINE:		/* �A�h���i�������b�V�� */
	case BS_ADRENALINE2:		/* �A�h���i�������b�V�� */
	case BS_WEAPONPERFECT:	/* �E�F�|���p�[�t�F�N�V���� */
	case BS_OVERTHRUST:		/* �I�[�o�[�g���X�g */
		if(sd == NULL || sd->status.party_id==0 || (flag&1) ){
			/* �ʂ̏��� */
			clif_skill_nodamage(bl,bl,skillid,skilllv,1);
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,(src == bl)? 1:0,0,0,skill_get_time(skillid,skilllv),0);
		}
		else{
			/* �p�[�e�B�S�̂ւ̏��� */
			party_foreachsamemap(skill_area_sub,
				sd,1,
				src,skillid,skilllv,tick, flag|BCT_PARTY|1,
				skill_castend_nodamage_id);
		}
		break;
	/*�i�t���Ɖ������K�v�j */
	case BS_MAXIMIZE:		/* �}�L�V�}�C�Y�p���[ */
	case NV_TRICKDEAD:		/* ���񂾂ӂ� */
	case CR_DEFENDER:		/* �f�B�t�F���_�[ */
	case CR_AUTOGUARD:		/* �I�[�g�K�[�h */
		{
			struct status_change *tsc_data = status_get_sc_data(bl);
			int sc=SkillStatusChangeTable[skillid];
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if( tsc_data ){
				if( tsc_data[sc].timer==-1 )
					/* �t������ */
					status_change_start(bl,sc,skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
				else
					/* �������� */
					status_change_end(bl, sc, -1);
			}
		}
		break;

	case TF_HIDING:			/* �n�C�f�B���O */
		{
			struct status_change *tsc_data = status_get_sc_data(bl);
			int sc=SkillStatusChangeTable[skillid];
			clif_skill_nodamage(src,bl,skillid,-1,1);
			if( tsc_data ){
				if( tsc_data[sc].timer==-1 )
					/* �t������ */
					status_change_start(bl,sc,skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
				else
					/* �������� */
					status_change_end(bl, sc, -1);
			}
		}
		break;

	case AS_CLOAKING:		/* �N���[�L���O */
		{
			struct status_change *tsc_data = status_get_sc_data(bl);
			int sc=SkillStatusChangeTable[skillid];
			clif_skill_nodamage(src,bl,skillid,-1,1);
			if( tsc_data ){
				if( tsc_data[sc].timer==-1 )
				{	/* �t������ */
					status_change_start(bl,sc,skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
				}else
					/* �������� */
					status_change_end(bl, sc, -1);
			}
			if(skilllv < 3)
				skill_check_cloaking(bl);
		}
		break;


	case ST_CHASEWALK:		/* �`�F�C�X�E�H�[�N */
		{
			struct status_change *tsc_data = status_get_sc_data(bl);
			int sc=SkillStatusChangeTable[skillid];
			clif_skill_nodamage(src,bl,skillid,-1,1);
			if(tsc_data && tsc_data[sc].timer!=-1 )
				/* �������� */
				status_change_end(bl, sc, -1);
			else
				/* �t������ */
				status_change_start(bl,sc,skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		}
		break;
		
	/* �Βn�X�L�� */
	case HP_BASILICA:			/* �o�W���J */
		sc_data = status_get_sc_data(src);
		if (sc_data && sc_data[SC_BASILICA].timer!=-1) {
			int i;
			struct mob_data *md = (src->type == BL_MOB ? (struct mob_data *)src : NULL);
			for (i=0;i<MAX_SKILLUNITGROUP;i++) {
				if (sd && sd->skillunit[i].skill_id==HP_BASILICA)
					skill_delunitgroup(&sd->skillunit[i]);
				if (md && md->skillunit[i].skill_id==HP_BASILICA)
					skill_delunitgroup(&md->skillunit[i]);
			}
			status_change_end(bl,SC_BASILICA,-1);
			break;
		}
		status_change_start(bl,SC_BASILICA,skilllv,bl->id,0,0,
			skill_get_time(skillid,skilllv),0);
		// fall through
	case BD_LULLABY:			/* �q��S */
	case BD_RICHMANKIM:			/* �j�����h�̉� */
	case BD_ETERNALCHAOS:		/* �i���̍��� */
	case BD_DRUMBATTLEFIELD:	/* �푾�ۂ̋��� */
	case BD_RINGNIBELUNGEN:		/* �j�[�x�����O�̎w�� */
	case BD_ROKISWEIL:			/* ���L�̋��� */
	case BD_INTOABYSS:			/* �[���̒��� */
	case BD_SIEGFRIED:			/* �s���g�̃W�[�N�t���[�h */
	case BA_DISSONANCE:			/* �s���a�� */
	case BA_POEMBRAGI:			/* �u���M�̎� */
	case BA_WHISTLE:			/* ���J */
	case BA_ASSASSINCROSS:		/* �[�z�̃A�T�V���N���X */
	case BA_APPLEIDUN:			/* �C�h�D���̗ь� */
	case DC_UGLYDANCE:			/* ��������ȃ_���X */
	case DC_HUMMING:			/* �n�~���O */
	case DC_DONTFORGETME:		/* ����Y��Ȃ��Łc */
	case DC_FORTUNEKISS:		/* �K�^�̃L�X */
	case DC_SERVICEFORYOU:		/* �T�[�r�X�t�H�[���[ */
	case CG_MOONLIT:			/* ������̉��� */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		skill_unitsetting(src,skillid,skilllv,src->x,src->y,0);
		break;
	case PA_GOSPEL:				/* �S�X�y�� */
		sc_data = status_get_sc_data(src);
		if (sc_data && sc_data[SC_GOSPEL].timer!=-1) {
			skill_delunitgroup((struct skill_unit_group *)sc_data[SC_GOSPEL].val3);
			status_change_end(bl,SC_GOSPEL,-1);
			break;
		}
		for(i=0;i<201;i++){
			if(i==SC_RIDING || i== SC_FALCON || i==SC_HALLUCINATION || i==SC_WEIGHT50
				|| i==SC_WEIGHT90 || i==SC_STRIPWEAPON || i==SC_STRIPSHIELD || i==SC_STRIPARMOR
				|| i==SC_STRIPHELM || i==SC_CP_WEAPON || i==SC_CP_SHIELD || i==SC_CP_ARMOR
				|| i==SC_CP_HELM || i==SC_COMBO || i==SC_TKCOMBO)
					continue;
			if(i==136) { i=192; continue; }
			status_change_end(bl,i,-1);
		}
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,SC_GOSPEL,skilllv,bl->id,
			(int)(skill_unitsetting(src,skillid,skilllv,src->x,src->y,0)),0,skill_get_time(skillid,skilllv),0);
		break;

	case BD_ADAPTATION:			/* �A�h���u */
		{
			struct status_change *sc_data = status_get_sc_data(src);
			if(sc_data && sc_data[SC_DANCING].timer!=-1){
				clif_skill_nodamage(src,bl,skillid,skilllv,1);
				skill_stop_dancing(src,0);
			}
		}
		break;

	case BA_FROSTJOKE:			/* �����W���[�N */
	case DC_SCREAM:				/* �X�N���[�� */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		skill_addtimerskill(src,tick+3000,bl->id,0,0,skillid,skilllv,0,flag);
		if(md){		// Mob�͒���Ȃ�����A�X�L���������΂��Ă݂�
			char temp[100];
			if(skillid == BA_FROSTJOKE)
				sprintf(temp,"%s : �����W���[�N !!",md->name);
			else
				sprintf(temp,"%s : �X�N���[�� !!",md->name);
			clif_GlobalMessage(&md->bl,temp);
		}
		break;

	case TF_STEAL:			// �X�e�B�[��
		if(sd) {
			if(pc_steal_item(sd,bl))
				clif_skill_nodamage(src,bl,skillid,skilllv,1);
			else
				clif_skill_fail(sd,skillid,0x0a,0);
		}
		break;

	case RG_STEALCOIN:		// �X�e�B�[���R�C��
		if(sd) {
			if(pc_steal_coin(sd,bl)) {
				int range = skill_get_range(skillid,skilllv);
				if(range < 0)
					range = status_get_range(src) - (range + 1);
				clif_skill_nodamage(src,bl,skillid,skilllv,1);
				mob_target((struct mob_data *)bl,src,range);
			}
			else
				clif_skill_fail(sd,skillid,0,0);
		}
		break;

	case MG_STONECURSE:			/* �X�g�[���J�[�X */
	{
		struct status_change *sc_data = status_get_sc_data(bl);
		// Level 6-10 doesn't consume a red gem if it fails [celest]
		int i, gem_flag = 1, fail_flag = 0;
		if (dstmd && status_get_mode(bl)&0x20) {
			clif_skill_fail(sd,sd->skillid,0,0);
			break;
		}
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if (dstsd && dstsd->special_state.no_magic_damage)
			break;
		if (sc_data && sc_data[SC_STONE].timer != -1) {
			status_change_end(bl,SC_STONE,-1);
			if (sd){
				fail_flag = 1;
				clif_skill_fail(sd,skillid,0,0);
			}
		} else if (atn_rand()%100 < skilllv*4+20 && !battle_check_undead(status_get_race(bl),status_get_elem_type(bl))) {
			status_change_start(bl,SC_STONE,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		} else if (sd) {
			if (skilllv > 5) gem_flag = 0;
			clif_skill_fail(sd,skillid,0,0);
			fail_flag = 1;
		}
		if (dstmd)
			mob_target(dstmd,src,skill_get_range(skillid,skilllv));
		if (sd && gem_flag) {
			if ((i=pc_search_inventory(sd, skill_db[skillid].itemid[0])) < 0 ) {
				if (!fail_flag) clif_skill_fail(sd,sd->skillid,0,0);
				break;
			}
			pc_delitem(sd, i, skill_db[skillid].amount[0], 0);
		}
		break;
	}
	case NV_FIRSTAID:			/* ���}�蓖 */
		clif_skill_nodamage(src,bl,skillid,5,1);
		battle_heal(NULL,bl,5,0,0);
		break;

	case AL_CURE:				/* �L���A�[ */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( dstsd && dstsd->special_state.no_magic_damage )
			break;
		status_change_end(bl, SC_SILENCE	, -1 );
		status_change_end(bl, SC_BLIND	, -1 );
		status_change_end(bl, SC_CONFUSION, -1 );
		break;

	case TF_DETOXIFY:			/* ��� */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_end(bl, SC_POISON	, -1 );
		status_change_end(bl, SC_DPOISON	, -1 );
		break;

	case PR_STRECOVERY:			/* ���J�o���[ */
		{
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if( dstsd && dstsd->special_state.no_magic_damage )
				break;
			status_change_end(bl, SC_FREEZE	, -1 );
			status_change_end(bl, SC_STONE	, -1 );
			status_change_end(bl, SC_SLEEP	, -1 );
			status_change_end(bl, SC_STAN		, -1 );
			if( battle_check_undead(status_get_race(bl),status_get_elem_type(bl)) ){//�A���f�b�h�Ȃ�ÈŌ���
				int blind_time;
				//blind_time=30-status_get_vit(bl)/10-status_get_int/15;
				blind_time=30*(100-(status_get_int(bl)+status_get_vit(bl))/2)/100;
				if(atn_rand()%100 < (100-(status_get_int(bl)/2+status_get_vit(bl)/3+status_get_luk(bl)/10)))
					status_change_start(bl, SC_BLIND,1,0,0,0,blind_time,0);
			}
			if(dstmd){
				dstmd->attacked_id=0;
				dstmd->target_id=0;
				dstmd->state.targettype = NONE_ATTACKABLE;
				dstmd->state.skillstate=MSS_IDLE;
				dstmd->next_walktime=tick+atn_rand()%3000+3000;
			}
		}
		break;

	case WZ_ESTIMATION:			/* �����X�^�[��� */
		if(sd){
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			clif_skill_estimation((struct map_session_data *)src,bl);
		}
		break;

	case MC_IDENTIFY:			/* �A�C�e���Ӓ� */
		if(sd)
			clif_item_identify_list(sd);
		break;

	case BS_REPAIRWEAPON:			/* ����C�� */
		if(sd && dstsd)
			clif_item_repair_list(sd,dstsd);
		break;

	case MC_VENDING:			/* �I�X�J�� */
		if(sd && pc_iscarton(sd))
			clif_openvendingreq(sd,2+sd->skilllv);
		break;

	case AL_TELEPORT:			/* �e���|�[�g */
		if( sd ){
			if(map[sd->bl.m].flag.noteleport){	/* �e���|�֎~ */
				clif_skill_teleportmessage(sd,0);
				break;
			}
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if( sd->skilllv==1 )
				clif_skill_warppoint(sd,sd->skillid,"Random","","","");
			else{
				clif_skill_warppoint(sd,sd->skillid,"Random",
					sd->status.save_point.map,"","");
			}
		}else if( bl->type==BL_MOB )
			mob_warp((struct mob_data *)bl,-1,-1,-1,3);
		break;

	case AL_HOLYWATER:			/* �A�N�A�x�l�f�B�N�^ */
		if(sd) {
			int eflag;
			struct item item_tmp;
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			memset(&item_tmp,0,sizeof(item_tmp));
			item_tmp.nameid = 523;
			item_tmp.identify = 1;
			if(battle_config.holywater_name_input) {
				item_tmp.card[0] = 0xfe;
				item_tmp.card[1] = 0;
				*((unsigned long *)(&item_tmp.card[2]))=sd->char_id;	/* �L����ID */
			}
			eflag = pc_additem(sd,&item_tmp,1);
			if(eflag) {
				clif_additem(sd,0,0,eflag);
				map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
			}
		}
		break;
	case TF_PICKSTONE:
		if(sd) {
			int eflag;
			struct item item_tmp;
			struct block_list tbl;
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			memset(&item_tmp,0,sizeof(item_tmp));
			memset(&tbl,0,sizeof(tbl));
			item_tmp.nameid = 7049;
			item_tmp.identify = 1;
			tbl.id   = 0;
			tbl.type = BL_NUL;
			clif_takeitem(&sd->bl,&tbl);
			eflag = pc_additem(sd,&item_tmp,1);
			if(eflag) {
				clif_additem(sd,0,0,eflag);
				map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
			}
		}
		break;

	case RG_STRIPWEAPON:		/* �X�g���b�v�E�F�|�� */
	case RG_STRIPSHIELD:		/* �X�g���b�v�V�[���h */
	case RG_STRIPARMOR:			/* �X�g���b�v�A�[�}�[ */
	case RG_STRIPHELM:			/* �X�g���b�v�w���� */
	{
		struct status_change *tsc_data = status_get_sc_data(bl);
		int cp_scid,scid, equip, strip_fix;
		scid = SkillStatusChangeTable[skillid];
		switch (skillid) {
			case RG_STRIPWEAPON:
				equip = EQP_WEAPON;
				cp_scid = SC_CP_WEAPON;
				break;
			case RG_STRIPSHIELD:
				equip = EQP_SHIELD;
				cp_scid = SC_CP_SHIELD;
				break;
			case RG_STRIPARMOR:
				equip = EQP_ARMOR;
				cp_scid = SC_CP_ARMOR;
				break;
			case RG_STRIPHELM:
				equip = EQP_HELM;
				cp_scid = SC_CP_HELM;
				break;
			default:
				map_freeblock_unlock();
				return 1;
		}

		if (tsc_data &&
				(tsc_data[scid].timer!=-1 || tsc_data[cp_scid].timer!=-1))
			break;

		strip_fix = status_get_dex(src) - status_get_dex(bl);
		if(strip_fix < 0)
			strip_fix=0;
		strip_per = 5+5*skilllv+strip_fix/5;
		if (atn_rand()%100 >= strip_per)
			break;

		if (dstsd) {
			for (i=0;i<MAX_INVENTORY;i++) {
				if (dstsd->status.inventory[i].equip && (dstsd->status.inventory[i].equip & equip)){
					pc_unequipitem(dstsd,i,0);
					break;
				}
			}
//			if (i == MAX_INVENTORY)
//				break;
		}
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		strip_time = skill_get_time(skillid,skilllv)+strip_fix/2;
		status_change_start(bl,scid,skilllv,0,0,0,strip_time,0 );
		break;
	}
	case ST_FULLSTRIP:		/* �t���X�g���b�v */
	{
		struct status_change *tsc_data = status_get_sc_data(bl);
		int strip_fix;

		if (tsc_data &&	
		((tsc_data[SC_CP_WEAPON].timer!=-1 ||
		tsc_data[SC_CP_SHIELD].timer!=-1 ||
		tsc_data[SC_CP_ARMOR].timer!=-1 ||
		tsc_data[SC_CP_HELM].timer!=-1) ||
		(tsc_data[SkillStatusChangeTable[RG_STRIPWEAPON]].timer!=-1 &&
		tsc_data[SkillStatusChangeTable[RG_STRIPSHIELD]].timer!=-1 &&
		tsc_data[SkillStatusChangeTable[RG_STRIPARMOR]].timer!=-1 &&
		tsc_data[SkillStatusChangeTable[RG_STRIPHELM]].timer!=-1)))
			break;

		strip_fix = status_get_dex(src) - status_get_dex(bl);
		if(strip_fix < 0)
			strip_fix=0;
		strip_per = 5+2*skilllv;
		if (atn_rand()%100 >= strip_per)
			break;

		if (dstsd) {
			for (i=0;i<MAX_INVENTORY;i++) {
				if(dstsd->status.inventory[i].equip && (
				(dstsd->status.inventory[i].equip & EQP_WEAPON)||
				(dstsd->status.inventory[i].equip & EQP_SHIELD)||
				(dstsd->status.inventory[i].equip & EQP_ARMOR)||
				(dstsd->status.inventory[i].equip & EQP_HELM)
				)){
					pc_unequipitem(dstsd,i,0);
				}
			}
		}
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		strip_time = skill_get_time(skillid,skilllv)+strip_fix/2;
		status_change_start(bl,SkillStatusChangeTable[RG_STRIPWEAPON],skilllv,0,0,0,strip_time,0 );
		status_change_start(bl,SkillStatusChangeTable[RG_STRIPSHIELD],skilllv,0,0,0,strip_time,0 );
		status_change_start(bl,SkillStatusChangeTable[RG_STRIPARMOR],skilllv,0,0,0,strip_time,0 );
		status_change_start(bl,SkillStatusChangeTable[RG_STRIPHELM],skilllv,0,0,0,strip_time,0 );
		break;
	}
	case AM_POTIONPITCHER:		/* �|�[�V�����s�b�`���[ */
		{
			struct block_list tbl;
			int i,x,hp = 0,sp = 0;
			if(sd) {
				x = skilllv%11 - 1;
				i = pc_search_inventory(sd,skill_db[skillid].itemid[x]);
				if(i < 0 || skill_db[skillid].itemid[x] <= 0) {
					clif_skill_fail(sd,skillid,0,0);
					map_freeblock_unlock();
					return 1;
				}
				if(sd->inventory_data[i] == NULL || sd->status.inventory[i].amount < skill_db[skillid].amount[x]) {
					clif_skill_fail(sd,skillid,0,0);
					map_freeblock_unlock();
					return 1;
				}
				sd->state.potionpitcher_flag = 1;
				sd->potion_hp = sd->potion_sp = sd->potion_per_hp = sd->potion_per_sp = 0;
				sd->skilltarget = bl->id;
				run_script(sd->inventory_data[i]->use_script,0,sd->bl.id,0);
				pc_delitem(sd,i,skill_db[skillid].amount[x],0);
				sd->state.potionpitcher_flag = 0;
				if(sd->potion_per_hp > 0 || sd->potion_per_sp > 0) {
					hp = status_get_max_hp(bl) * sd->potion_per_hp / 100;
					hp = hp * (100 + pc_checkskill(sd,AM_POTIONPITCHER)*10 + pc_checkskill(sd,AM_LEARNINGPOTION)*5)/100;
					if(dstsd) {
						sp = dstsd->status.max_sp * sd->potion_per_sp / 100;
						sp = sp * (100 + pc_checkskill(sd,AM_POTIONPITCHER) + pc_checkskill(sd,AM_LEARNINGPOTION)*5)/100;
					}
				}
				else {
					if(sd->potion_hp > 0) {
						hp = sd->potion_hp * (100 + pc_checkskill(sd,AM_POTIONPITCHER)*10 + pc_checkskill(sd,AM_LEARNINGPOTION)*5)/100;
						hp = hp * (100 + (status_get_vit(bl)<<1)) / 100;
						if(dstsd)
							hp = hp * (100 + pc_checkskill(dstsd,SM_RECOVERY)*10) / 100;
					}
					if(sd->potion_sp > 0) {
						sp = sd->potion_sp * (100 + pc_checkskill(sd,AM_POTIONPITCHER) + pc_checkskill(sd,AM_LEARNINGPOTION)*5)/100;
						sp = sp * (100 + (status_get_int(bl)<<1)) / 100;
						if(dstsd)
							sp = sp * (100 + pc_checkskill(dstsd,MG_SRECOVERY)*10) / 100;
					}
				}
			}
			else {
				hp = (1 + atn_rand()%400) * (100 + skilllv*10) / 100;
				hp = hp * (100 + (status_get_vit(bl)<<1)) / 100;
				if(dstsd)
					hp = hp * (100 + pc_checkskill(dstsd,SM_RECOVERY)*10) / 100;
			}
			tbl.id = 0;
			tbl.m = src->m;
			tbl.x = src->x;
			tbl.y = src->y;
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if(hp > 0 || (hp <= 0 && sp <= 0))
				clif_skill_nodamage(&tbl,bl,AL_HEAL,hp,1);
			if(sp > 0)
				clif_skill_nodamage(&tbl,bl,MG_SRECOVERY,sp,1);
			battle_heal(src,bl,hp,sp,0);
		}
		break;
		
	case CR_SLIMPITCHER://�X�����|�[�V�����s�b�`���[
		{
			if (sd && flag&1) {
				struct block_list tbl;
				int hp = sd->potion_hp * (100 + pc_checkskill(sd,CR_SLIMPITCHER)*10 + pc_checkskill(sd,AM_POTIONPITCHER)*10 + pc_checkskill(sd,AM_LEARNINGPOTION)*5)/100;
				hp = hp * (100 + (status_get_vit(bl)<<1))/100;
				if (dstsd) {
					hp = hp * (100 + pc_checkskill(dstsd,SM_RECOVERY)*10)/100;
				}
				tbl.id = 0;
				tbl.m = src->m;
				tbl.x = src->x;
				tbl.y = src->y;
				clif_skill_nodamage(&tbl,bl,AL_HEAL,hp,1);
				battle_heal(NULL,bl,hp,0,0);
			}
		}
		break;
		
	case AM_BERSERKPITCHER:		/* �o�[�T�[�N�s�b�`���[ */
		{
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			status_change_start(bl,SC_SPEEDPOTION2,1,0,0,0,900,0 );
		}
		break;
	case AM_CP_WEAPON:
		{
			struct status_change *tsc_data = status_get_sc_data(bl);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if(tsc_data && tsc_data[SC_STRIPWEAPON].timer != -1)
				status_change_end(bl, SC_STRIPWEAPON, -1 );
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		}
		break;
	case AM_CP_SHIELD:
		{
			struct status_change *tsc_data = status_get_sc_data(bl);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if(tsc_data && tsc_data[SC_STRIPSHIELD].timer != -1)
				status_change_end(bl, SC_STRIPSHIELD, -1 );
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		}
		break;
	case AM_CP_ARMOR:
		{
			struct status_change *tsc_data = status_get_sc_data(bl);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if(tsc_data && tsc_data[SC_STRIPARMOR].timer != -1)
				status_change_end(bl, SC_STRIPARMOR, -1 );
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		}
		break;
	case AM_CP_HELM:
		{
			struct status_change *tsc_data = status_get_sc_data(bl);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if(tsc_data && tsc_data[SC_STRIPHELM].timer != -1)
				status_change_end(bl, SC_STRIPHELM, -1 );
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		}
		break;
	case CR_FULLPROTECTION:			/* �t���P�~�J���`���[�W */
		{

			struct status_change *tsc_data = status_get_sc_data(bl);
			if(tsc_data && tsc_data[SC_STRIPWEAPON].timer != -1)
				status_change_end(bl, SC_STRIPWEAPON, -1 );
			status_change_start(bl,SkillStatusChangeTable[AM_CP_WEAPON],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
			if(tsc_data && tsc_data[SC_STRIPSHIELD].timer != -1)
				status_change_end(bl, SC_STRIPSHIELD, -1 );
			status_change_start(bl,SkillStatusChangeTable[AM_CP_SHIELD],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if(tsc_data && tsc_data[SC_STRIPARMOR].timer != -1)
				status_change_end(bl, SC_STRIPARMOR, -1 );
			status_change_start(bl,SkillStatusChangeTable[AM_CP_ARMOR],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
			if(tsc_data && tsc_data[SC_STRIPHELM].timer != -1)
				status_change_end(bl, SC_STRIPHELM, -1 );
			status_change_start(bl,SkillStatusChangeTable[AM_CP_HELM],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );

		}
		break;
	case SA_DISPELL:			/* �f�B�X�y�� */
		{
			int i;
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if( dstsd && dstsd->special_state.no_magic_damage )
				break;
			//���[�O�̍�
			if(dstsd && dstsd->sc_data[SC_ROGUE].timer!=-1)
				break;
			//�\�E�������J�[�͖���
			if(dstsd && dstsd->status.class == (PC_CLASS_BASE3+27))
				break;
				
			for(i=0;i<136;i++){
				if(i==SC_RIDING || i== SC_FALCON || i==SC_HALLUCINATION || i==SC_WEIGHT50
					|| i==SC_WEIGHT90 || i==SC_STRIPWEAPON || i==SC_STRIPSHIELD || i==SC_STRIPARMOR
					|| i==SC_STRIPHELM || i==SC_CP_WEAPON || i==SC_CP_SHIELD || i==SC_CP_ARMOR
					|| i==SC_CP_HELM || i==SC_COMBO || i==SC_TKCOMBO)
						continue;
				status_change_end(bl,i,-1);
			}
		}
		break;

	case TF_BACKSLIDING:		/* �o�b�N�X�e�b�v */
		battle_stopwalking(src,1);
		skill_blown(src,bl,skill_get_blewcount(skillid,skilllv)|0x10000);
		if(src->type == BL_MOB)
			clif_fixmobpos((struct mob_data *)src);
		else if(src->type == BL_PET)
			clif_fixpetpos((struct pet_data *)src);
		else if(src->type == BL_PC)
			clif_fixpos(src);
		skill_addtimerskill(src,tick + 200,src->id,0,0,skillid,skilllv,0,flag);
		break;

	case SA_CASTCANCEL:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		skill_castcancel(src,1);
		if(sd) {
			int sp = skill_get_sp(sd->skillid_old,sd->skilllv_old);
			sp = sp * (90 - (skilllv-1)*20) / 100;
			if(sp < 0) sp = 0;
			pc_heal(sd,0,-sp);
		}
		break;
	case SA_SPELLBREAKER:	// �X�y���u���C�J�[
		{
			struct status_change *sc_data = status_get_sc_data(bl);
			int sp;
			if(sc_data && sc_data[SC_MAGICROD].timer != -1) {
				if(dstsd) {
					sp = skill_get_sp(skillid,skilllv);
					sp = sp * sc_data[SC_MAGICROD].val2 / 100;
					if(sp > 0x7fff) sp = 0x7fff;
					else if(sp < 1) sp = 1;
					if(dstsd->status.sp + sp > dstsd->status.max_sp) {
						sp = dstsd->status.max_sp - dstsd->status.sp;
						dstsd->status.sp = dstsd->status.max_sp;
					}
					else
						dstsd->status.sp += sp;
					clif_heal(dstsd->fd,SP_SP,sp);
				}
				clif_skill_nodamage(bl,bl,SA_MAGICROD,sc_data[SC_MAGICROD].val1,1);
				if(sd) {
					sp = sd->status.max_sp/5;
					if(sp < 1) sp = 1;
					pc_heal(sd,0,-sp);
				}
			}
			else {
				int bl_skillid=0,bl_skilllv=0;
				if(dstsd) {
					if(dstsd->skilltimer != -1) {
						bl_skillid = dstsd->skillid;
						bl_skilllv = dstsd->skilllv;
					}
				}
				else if(dstmd) {
					if(dstmd->skilltimer != -1) {
						bl_skillid = dstmd->skillid;
						bl_skilllv = dstmd->skilllv;
					}
				}
				if(bl_skillid > 0 && skill_db[bl_skillid].skill_type == BF_MAGIC) {
					clif_skill_nodamage(src,bl,skillid,skilllv,1);
					skill_castcancel(bl,0);
					sp = skill_get_sp(bl_skillid,bl_skilllv);
					if(dstsd)
						pc_heal(dstsd,0,-sp);
					if(sd) {
						sp = sp*(25*(skilllv-1))/100;
						if(skilllv > 1 && sp < 1) sp = 1;
						if(sp > 0x7fff) sp = 0x7fff;
						else if(sp < 1) sp = 1;
						if(sd->status.sp + sp > sd->status.max_sp) {
							sp = sd->status.max_sp - sd->status.sp;
							sd->status.sp = sd->status.max_sp;
						}
						else
							sd->status.sp += sp;
						clif_heal(sd->fd,SP_SP,sp);
					}
				}
				else if(sd)
					clif_skill_fail(sd,skillid,0,0);
			}
		}
		break;
	case SA_MAGICROD:
		if( dstsd && dstsd->special_state.no_magic_damage )
			break;
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		break;
	case SA_AUTOSPELL:			/* �I�[�g�X�y�� */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(sd)
			clif_autospell(sd,skilllv);
		else {
			int maxlv=1,spellid=0;
			static const int spellarray[3] = { MG_COLDBOLT,MG_FIREBOLT,MG_LIGHTNINGBOLT };
			if(skilllv >= 10) {
				spellid = MG_FROSTDIVER;
				maxlv = skilllv - 9;
			}
			else if(skilllv >=8) {
				spellid = MG_FIREBALL;
				maxlv = skilllv - 7;
			}
			else if(skilllv >=5) {
				spellid = MG_SOULSTRIKE;
				maxlv = skilllv - 4;
			}
			else if(skilllv >=2) {
				int i = atn_rand()%3;
				spellid = spellarray[i];
				maxlv = skilllv - 1;
			}
			else if(skilllv > 0) {
				spellid = MG_NAPALMBEAT;
				maxlv = 3;
			}
			if(spellid > 0)
				status_change_start(src,SC_AUTOSPELL,skilllv,spellid,maxlv,0,
					skill_get_time(SA_AUTOSPELL,skilllv),0);
		}
		break;
	case PF_MINDBREAKER:
		if (atn_rand()%100<(55+skilllv*5)) {
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,
				0,0,0,skill_get_time(skillid,skilllv),0);
		} else if (sd)
			clif_skill_fail(sd,skillid,0,0);
		break;
	case PF_SOULCHANGE:		/* �\�E���`�F���W */
		if (sd && bl->type==BL_PC) {
			struct map_session_data *tsd = (struct map_session_data *)bl;
			int sp,tsp;
			if (battle_check_target(src,bl,BCT_PARTY)<=0 &&
					!map[src->m].flag.pvp && !map[src->m].flag.gvg)
				break;	/* PVP/GVG�ȊO�ł�PT�����o�[�ɂ̂ݎg�p�\ */
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			sp = sd->status.sp;
			tsp = tsd->status.sp;
			pc_heal(sd,0,tsp-sp);
			pc_heal(tsd,0,sp-tsp);
		}

	/* �����_�������ω��A�������ω��A�n�A�΁A�� */
	case NPC_ATTRICHANGE:
	case NPC_CHANGEWATER:
	case NPC_CHANGEGROUND:
	case NPC_CHANGEFIRE:
	case NPC_CHANGEWIND:
	/* �ŁA���A�O�A�� */
	case NPC_CHANGEPOISON:
	case NPC_CHANGEHOLY:
	case NPC_CHANGEDARKNESS:
	case NPC_CHANGETELEKINESIS:
		if(md){
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			md->def_ele=skill_get_pl(skillid);
			if(md->def_ele==0)			/* �����_���ω��A�������A*/
				md->def_ele=atn_rand()%10;	/* �s�������͏��� */
			md->def_ele+=(1+atn_rand()%4)*20;	/* �������x���̓����_�� */
		}
		break;

	case NPC_PROVOCATION:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(md)
			clif_pet_performance(src,mob_db[md->class].skill[md->skillidx].val[0]);
		break;

	case NPC_HALLUCINATION:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( dstsd && dstsd->special_state.no_magic_damage )
			break;
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		break;

	case NPC_KEEPING:
	case NPC_BARRIER:
		{
			int skill_time = skill_get_time(skillid,skilllv);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_time,0 );
			mob_changestate((struct mob_data *)src,MS_DELAY,skill_time);
		}
		break;

	case NPC_DARKBLESSING:
		{
			int sc_def = 100 - status_get_mdef(bl);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if(dstsd && dstsd->special_state.no_magic_damage )
				break;
			if(status_get_elem_type(bl) == 7 || status_get_race(bl) == 6)
				break;
			if(atn_rand()%100 < sc_def*(50+skilllv*5)/100) {
				if(dstsd) {
					int hp = status_get_hp(bl)-1;
					pc_heal(dstsd,-hp,0);
				}
				else if(dstmd)
					dstmd->hp = 1;
			}
		}
		break;

	case NPC_SELFDESTRUCTION:	/* ���� */
	case NPC_SELFDESTRUCTION2:	/* ����2 */
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,skillid,0,0,skill_get_time(skillid,skilllv),0);
		break;
	case NPC_LICK:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( dstsd && dstsd->special_state.no_weapon_damage )
			break;
		if(dstsd)
			pc_heal(dstsd,0,-100);
		if(atn_rand()%100 < (skilllv*5)*sc_def_vit/100)
			status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case NPC_SUICIDE:			/* ���� */
		if(md){
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			mob_damage(NULL,md,md->hp,0);
		}
		break;

	case NPC_SUMMONSLAVE:		/* �艺���� */
	case NPC_SUMMONMONSTER:		/* MOB���� */
		if(md)
			mob_summonslave(md,mob_db[md->class].skill[md->skillidx].val,skilllv,(skillid==NPC_SUMMONSLAVE)?1:0);
		break;
	case NPC_RECALL:		//��芪���Ăі߂�
		if(md){
			int mobcount;
			md->recallcount=0;//������
			md->recall_flag=0;
			mobcount=mob_countslave(md);
				if(mobcount>0){
				md->recall_flag=1; //mob.c��[��芪�������X�^�[�̏���]�ŗ��p
				md->recallmob_count=mobcount;
				}
		}
		break;

	case NPC_RUNAWAY:		//���
		if(md){
			int check;
			int dist=skilllv;//��ނ��鋗��
			check = md->dir; //�������ǂ̕����Ɍ����Ă邩�`�F�b�N
			md->attacked_id=0;
			md->target_id=0;
			md->state.targettype = NONE_ATTACKABLE;
			md->state.skillstate=MSS_IDLE;
			if(check==0)				//�����̌����Ă�����Ƌt�Ɉړ�����
				mob_walktoxy(md,md->bl.x,md->bl.y-dist,0);//�����āA�ړ�����
			else if (check==1)
				mob_walktoxy(md,md->bl.x-dist,md->bl.y-dist,0);
			else if (check==2)
				mob_walktoxy(md,md->bl.x+dist,md->bl.y,0);
			else if (check==3)
				mob_walktoxy(md,md->bl.x+dist,md->bl.y+dist,0);
			else if (check==4)
				mob_walktoxy(md,md->bl.x,md->bl.y+dist,0);
			else if (check==5)
				mob_walktoxy(md,md->bl.x-dist,md->bl.y+dist,0);
			else if (check==6)
				mob_walktoxy(md,md->bl.x-dist,md->bl.y,0);
			else if (check==7)
				mob_walktoxy(md,md->bl.x-dist,md->bl.y-dist,0);
			}
		break;

	case NPC_TRANSFORMATION:
	case NPC_METAMORPHOSIS:
		if(md)
			mob_class_change(md,mob_db[md->class].skill[md->skillidx].val,sizeof(mob_db[md->class].skill[md->skillidx].val)/sizeof(mob_db[md->class].skill[md->skillidx].val[0]));
		break;

	case NPC_EMOTION:			/* �G���[�V���� */
		if(md)
			clif_emotion(&md->bl,mob_db[md->class].skill[md->skillidx].val[0]);
		break;

	case NPC_DEFENDER:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		break;

	case WE_MALE:				/* �N�����͌��� */
		if(sd && dstsd){
			if(battle_config.new_marrige_skill)
			{
				int hp_rate=(skilllv <= 0)? 0:skill_db[skillid].hp_rate[skilllv-1];
				int gain_hp=dstsd->status.max_hp*abs(hp_rate)/100;// 15%
				clif_skill_nodamage(src,bl,skillid,gain_hp,1);
				battle_heal(NULL,bl,gain_hp,0,0);
			}else{
				int hp_rate=(skilllv <= 0)? 0:skill_db[skillid].hp_rate[skilllv-1];
				int gain_hp=sd->status.max_hp*abs(hp_rate)/100;// 15%
				clif_skill_nodamage(src,bl,skillid,gain_hp,1);
				battle_heal(NULL,bl,gain_hp,0,0);
			}
		}
		break;
	case WE_FEMALE:				/* ���Ȃ��ׂ̈ɋ]���ɂȂ�܂� */
		if(sd && dstsd){
			if(battle_config.new_marrige_skill)
			{
				int sp_rate=(skilllv <= 0)? 0:skill_db[skillid].sp_rate[skilllv-1];
				int gain_sp=dstsd->status.max_sp*abs(sp_rate)/100;// 15%
				clif_skill_nodamage(src,bl,skillid,gain_sp,1);
				battle_heal(NULL,bl,0,gain_sp,0);
			}else{
				int sp_rate=(skilllv <= 0)? 0:skill_db[skillid].sp_rate[skilllv-1];
				int gain_sp=sd->status.max_sp*abs(sp_rate)/100;// 15%
				clif_skill_nodamage(src,bl,skillid,gain_sp,1);
				battle_heal(NULL,bl,0,gain_sp,0);
			}
		}
		break;

	case WE_CALLPARTNER:			/* ���Ȃ��Ɉ������� */
		if(sd){
			int maxcount, i, d, x, y;
			if(map[sd->bl.m].flag.nomemo){
				clif_skill_teleportmessage(sd,1);
				map_freeblock_unlock();
				return 0;
			}
			clif_callpartner(sd);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if((maxcount = skill_get_maxcount(sd->skillid)) > 0) {
				int c;
				for(i=c=0;i<MAX_SKILLUNITGROUP;i++) {
					if(sd->skillunit[i].alive_count > 0 && sd->skillunit[i].skill_id == sd->skillid)
						c++;
				}
				if(c >= maxcount) {
					clif_skill_fail(sd,sd->skillid,0,0);
					sd->canact_tick = gettick();
					sd->canmove_tick = gettick();
					sd->skillitem = sd->skillitemlv = -1;
					map_freeblock_unlock();
					return 0;
				}
			}

			// �ڂ̑O�ɌĂяo��
			for( i = 0; i < 8; i++ ){
				if( i & 1 )
					d = (sd->dir-((i+1)>>1))&7;
				else
					d = (sd->dir+((i+1)>>1))&7;

				x = sd->bl.x + dirx[d];
				y = sd->bl.y + diry[d];

				if(map_getcell(sd->bl.m,x,y,CELL_CHKPASS))
					break;
			}

			if( i >= 8 )
				skill_unitsetting(src,skillid,skilllv,sd->bl.x,sd->bl.y,0);
			else
				skill_unitsetting(src,skillid,skilllv,x,y,0);
		}
		break;

	case PF_HPCONVERSION:			/* ���C�t�u������ */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(sd){
			int conv_hp=0,conv_sp=0;
			conv_hp=sd->status.hp/10; //��{��HP��10%
			sd->status.hp -= conv_hp; //HP�����炷
			conv_sp=conv_hp*20*skilllv/100;
			conv_sp=(sd->status.sp+conv_sp>sd->status.max_sp)?sd->status.max_sp-sd->status.sp:conv_sp;
			sd->status.sp += conv_sp; //SP�𑝂₷
			pc_heal(sd,-conv_hp,conv_sp);
			clif_heal(sd->fd,SP_SP,conv_sp);
		}
		break;
	case HT_REMOVETRAP:				/* �����[�u�g���b�v */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		{
			struct skill_unit *su=NULL;
			struct item item_tmp;
			int flag;
			if((bl->type==BL_SKILL) &&
			   (su=(struct skill_unit *)bl) &&
			   (su->group->src_id == src->id || map[bl->m].flag.pvp || map[bl->m].flag.gvg) &&
			   (su->group->unit_id >= 0x8f && su->group->unit_id <= 0x99) &&
			   (su->group->unit_id != 0x92)){ //㩂����Ԃ�
				if(sd){
					if(battle_config.skill_removetrap_type == 1){
						for(i=0;i<10;i++) {
							if(skill_db[su->group->skill_id].itemid[i] > 0){
								memset(&item_tmp,0,sizeof(item_tmp));
								item_tmp.nameid = skill_db[su->group->skill_id].itemid[i];
								item_tmp.identify = 1;
								if(item_tmp.nameid && (flag=pc_additem(sd,&item_tmp,skill_db[su->group->skill_id].amount[i]))){
									clif_additem(sd,0,0,flag);
									map_addflooritem(&item_tmp,skill_db[su->group->skill_id].amount[i],sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
								}
							}
						}
					}else{
						memset(&item_tmp,0,sizeof(item_tmp));
						item_tmp.nameid = 1065;
						item_tmp.identify = 1;
						if(item_tmp.nameid && (flag=pc_additem(sd,&item_tmp,1))){
							clif_additem(sd,0,0,flag);
							map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
						}
					}
					
				}
				if(su->group->unit_id == 0x91 && su->group->val2){
					struct block_list *target=map_id2bl(su->group->val2);
					if(target && target->type == BL_PC)
						status_change_end(target,SC_ANKLE,-1);
				}
				skill_delunit(su);
			}
		}
		break;
	case HT_SPRINGTRAP:				/* �X�v�����O�g���b�v */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		{
			struct skill_unit *su=NULL;
			if((bl->type==BL_SKILL) && (su=(struct skill_unit *)bl) && (su->group) ){
				switch(su->group->unit_id){
					case 0x8f:	/* �u���X�g�}�C�� */
					case 0x90:	/* �X�L�b�h�g���b�v */
					case 0x93:	/* �����h�}�C�� */
					case 0x94:	/* �V���b�N�E�F�[�u�g���b�v */
					case 0x95:	/* �T���h�}�� */
					case 0x96:	/* �t���b�V���[ */
					case 0x97:	/* �t���[�W���O�g���b�v */
					case 0x98:	/* �N���C���A�[�g���b�v */
					case 0x99:	/* �g�[�L�[�{�b�N�X */
						su->group->unit_id = 0x8c;
						clif_changelook(bl,LOOK_BASE,su->group->unit_id);
						su->group->limit=DIFF_TICK(tick+1500,su->group->tick);
						su->limit=DIFF_TICK(tick+1500,su->group->tick);
				}
			}
		}
		break;
	case BD_ENCORE:					/* �A���R�[�� */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(sd)
			skill_use_id(sd,src->id,sd->skillid_dance,sd->skilllv_dance);
		break;
	case AS_SPLASHER:		/* �x�i���X�v���b�V���[ */
		if((double)status_get_max_hp(bl)*3/4 < status_get_hp(bl)) {
			//HP��3/4�ȏ�c���Ă����玸�s
			map_freeblock_unlock();
			return 1;
		}
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,skillid,src->id,0,skill_get_time(skillid,skilllv),0 );
		break;
		
	//�M���h�X�L���͂������牺�ɒǉ�
	case GD_BATTLEORDER://#�Ր�Ԑ�#
		if(sd){
			int mi,range;
			struct guild *g = guild_search(sd->status.guild_id);
			struct map_session_data * member = NULL;
			range = skill_get_range(skillid,skilllv);
			if(g)
			for(mi = 0;mi < g->max_member;mi++)
			{
				member = g->member[mi].sd;
				if(member == NULL)
					continue;
				
				if(sd->bl.m != member->bl.m)
					continue;
				
				if( abs(sd->bl.x - member->bl.x)<=range && abs(sd->bl.y - member->bl.y)<=range)
				{
					clif_skill_nodamage(src,&member->bl,skillid,skilllv,1);
					status_change_start(&member->bl,GetSkillStatusChangeTable(skillid),skilllv,skillid,0,0,skill_get_time(skillid,skilllv),0 );
				}
			}
			status_change_start(src,SC_BATTLEORDER_DELAY,0,0,0,0,300000,0 );
		}
		break;
	case GD_REGENERATION://#����#
		
		if(sd){
			int mi,range;
			struct guild * g = guild_search(sd->status.guild_id);
			struct map_session_data * member = NULL;
			range = skill_get_range(skillid,skilllv);
			for(mi = 0;mi < g->max_member;mi++)
			{
				member = g->member[mi].sd;
				if(member == NULL)
					continue;
				
				if(sd->bl.m != member->bl.m)
					continue;
				
				if( abs(sd->bl.x - member->bl.x)<=range && abs(sd->bl.y - member->bl.y)<=range)
				{
					clif_skill_nodamage(src,&member->bl,skillid,skilllv,1);
					status_change_start(&member->bl,GetSkillStatusChangeTable(skillid),skilllv,skillid,0,0,skill_get_time(skillid,skilllv),0 );
				}
			}
			status_change_start(src,SC_REGENERATION_DELAY,0,0,0,0,300000,0 );
		}
		break;
	case GD_RESTORE://##����
		if(sd){
			int mi,range;
			struct guild * g = guild_search(sd->status.guild_id);
			struct map_session_data * member = NULL;
			range = skill_get_range(skillid,skilllv);
			for(mi = 0;mi < g->max_member;mi++)
			{
				member = g->member[mi].sd;
				if(member == NULL)
					continue;
				
				if(sd->bl.m != member->bl.m)
					continue;
				
				if( abs(sd->bl.x - member->bl.x)<=range && abs(sd->bl.y - member->bl.y)<=range)
				{
					clif_skill_nodamage(src,&member->bl,skillid,skilllv,1);
					pc_heal(member,member->status.max_hp*90/100,member->status.max_sp*90/100);
				}
			}
			status_change_start(src,SC_RESTORE_DELAY,0,0,0,0,300000,0 );
		}
		break;
	case GD_EMERGENCYCALL://#�ً}���W#
		if(sd){
			int mi,px,py,d;
			struct guild * g = guild_search(sd->status.guild_id);
			struct map_session_data * member = NULL;
			clif_skill_nodamage(src,src,skillid,skilllv,1);
			if(battle_config.emergencycall_point_randam)
			{
				int i;
				// �ڂ̑O�ɌĂяo��
				for( i = 0; i < 8; i++ ){
					if( i & 1 )
						d = (sd->dir-((i+1)>>1))&7;
					else
						d = (sd->dir+((i+1)>>1))&7;

					px = sd->bl.x + dirx[d];
					py = sd->bl.y + diry[d];
	
					if(map_getcell(sd->bl.m,px,py,CELL_CHKPASS))
						break;
				}

				if( i >= 8 )
				{
					px = sd->bl.x;
					py = sd->bl.y;	
				}
			}else{//����
				px = sd->bl.x;
				py = sd->bl.y;	
			}
			for(mi = 1;mi < g->max_member;mi++)
			{
				member = g->member[mi].sd;
				if(member == NULL)
					continue;
				
				//���}�b�v�̂�
				if(battle_config.emergencycall_call_limit && sd->bl.m != member->bl.m)
					continue;
					
				if(member->refuse_emergencycall)
					continue;
				
				pc_setpos(member,map[sd->bl.m].name,px,py,3);
			}
			status_change_start(src,SC_EMERGENCYCALL_DELAY,0,0,0,0,300000,0 );
		}
		break;

	case SG_FEEL:
		if(sd)
		{
			char output[128];
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if(sd->feel_map[skilllv-1].m==-1)
			{
				strcpy(sd->feel_map[skilllv-1].name,map[sd->bl.m].name);
				sd->feel_map[skilllv-1].m = sd->bl.m;
			}
			switch(skilllv){
				case 1:
					sprintf(output,"���z�̏ꏊ:%s",sd->feel_map[skilllv-1].name);
					clif_disp_onlyself(sd,output,strlen(output));
					break;
				case 2:
					sprintf(output,"���̏ꏊ:%s",sd->feel_map[skilllv-1].name);
					clif_disp_onlyself(sd,output,strlen(output));
					break;
				case 3:
					sprintf(output,"���̏ꏊ:%s",sd->feel_map[skilllv-1].name);
					clif_disp_onlyself(sd,output,strlen(output));
					break;
				default:
					break;
			}
		}
		break;
	case SG_HATE:
		if(sd)
		{
			char output[128];
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if(dstsd)//�o�^���肪PC
			{
				if(sd->hate_mob[skilllv-1] == -1)
				{
					struct pc_base_job s_class;
					s_class = pc_calc_base_job(dstsd->status.class);
					sd->hate_mob[skilllv-1] = s_class.job;
				}
			}else if(dstmd)//�o�^���肪MOB
			{
				switch(skilllv)
				{
					case 1:
						if(sd->hate_mob[0] == -1 && status_get_size(bl)==0)
							sd->hate_mob[0] = dstmd->class;
						break;
					case 2:
						if(sd->hate_mob[1] == -1 && status_get_size(bl)==1 && status_get_max_hp(bl)>=6000)
							sd->hate_mob[1] = dstmd->class;
						break;
					case 3:
						if(sd->hate_mob[2] == -1 && status_get_size(bl)==2 && status_get_max_hp(bl)>=20000)
							sd->hate_mob[2] = dstmd->class;
						break;
				}
			}
			
			
			//�\��
			if(sd->hate_mob[skilllv-1]!=-1)
			{
				switch(skilllv){
					case 1:
					{
						if(sd->hate_mob[0] >=1000)
							sprintf(output,"���z�̑���:%s(%d)",mob_db[sd->hate_mob[0]].jname,sd->hate_mob[0]);
						else
							sprintf(output,"���̑���:JOB(%d)",sd->hate_mob[0]);
						clif_disp_onlyself(sd,output,strlen(output));
					}
						break;
					case 2:
					{
						if(sd->hate_mob[1] >=1000)
							sprintf(output,"���̑���:%s(%d)",mob_db[sd->hate_mob[1]].jname,sd->hate_mob[1]);
						else
							sprintf(output,"���̑���:JOB(%d)",sd->hate_mob[1]);
						clif_disp_onlyself(sd,output,strlen(output));
					}
						break;
					case 3:
					{
						if(sd->hate_mob[2] >=1000)
							sprintf(output,"���̑���:%s(%d)",mob_db[sd->hate_mob[2]].jname,sd->hate_mob[2]);
						else
							sprintf(output,"���̑���:JOB(%d)",sd->hate_mob[2]);
						clif_disp_onlyself(sd,output,strlen(output));
					}
						break;
					default:
						break;
				}
			}else{
				strcpy(output,"�o�^���s");
				clif_disp_onlyself(sd,output,strlen(output));
			}
		}
		break;
	case SG_FUSION:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,GetSkillStatusChangeTable(skillid),skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		break;
	case SG_SUN_COMFORT:
	case SG_MOON_COMFORT:
	case SG_STAR_COMFORT:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,GetSkillStatusChangeTable(skillid),skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		break;
	//��
	case SL_ALCHEMIST://#�A���P�~�X�g�̍�#
	case SL_MONK://#�����N�̍�#
	case SL_STAR://#�P���Z�C�̍�#
	case SL_SAGE://#�Z�[�W�̍�#
	case SL_CRUSADER://#�N���Z�C�_�[�̍�#
	case SL_KNIGHT://#�i�C�g�̍�#
	case SL_WIZARD://#�E�B�U�[�h�̍�#	
	case SL_PRIEST://#�v���[�X�g�̍�#
	case SL_SUPERNOVICE://#�X�[�p�[�m�[�r�X�̍�#
	case SL_BARDDANCER://#�o�[�h�ƃ_���T�[�̍�#
	case SL_ROGUE://#���[�O�̍�#
	case SL_ASSASIN://#�A�T�V���̍�#
	case SL_BLACKSMITH://#�u���b�N�X�~�X�̍�#
	case SL_HUNTER://#�n���^�[�̍�#
	case SL_SOULLINKER://#�\�E�������J�[�̍�#
	case SL_HIGH://#�ꎟ��ʐE�Ƃ̍�#
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,GetSkillStatusChangeTable(skillid),skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		break;
	case PF_DOUBLECASTING:		/* �_�u���L���X�e�B���O */
		if (rand() % 100 > 30 + skilllv * 10) {
			clif_skill_fail(sd,skillid,0,0);
			map_freeblock_unlock();
			return 0;
		}
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		break;
	default:
		printf("skill_castend_nodamage_id: Unknown skill used:%d\n",skillid);
		map_freeblock_unlock();
		return 1;
	}

	map_freeblock_unlock();
	return 0;
}

/*==========================================
 * �X�L���g�p�i�r�������AID�w��j
 *------------------------------------------
 */
int skill_castend_id( int tid, unsigned int tick, int id,int data )
{
	struct map_session_data* sd = map_id2sd(id)/*,*target_sd=NULL*/;
	struct block_list *bl;
	int inf2;

	nullpo_retr(0, sd);
	
	if( sd->bl.prev == NULL ) //prev�������̂͂���Ȃ́H
		return 0;

	if(sd->skillid != SA_CASTCANCEL && sd->skilltimer != tid )	/* �^�C�}ID�̊m�F */
		return 0;
	if(sd->skillid != SA_CASTCANCEL && sd->skilltimer != -1 && pc_checkskill(sd,SA_FREECAST) > 0) {
		sd->speed = sd->prev_speed;
		clif_updatestatus(sd,SP_SPEED);
	}
	if(sd->skillid != SA_CASTCANCEL)
		sd->skilltimer=-1;

	if((bl=map_id2bl(sd->skilltarget))==NULL || bl->prev==NULL) {
		sd->canact_tick = tick;
		sd->canmove_tick = tick;
		sd->skillitem = sd->skillitemlv = -1;
		return 0;
	}
	if(sd->bl.m != bl->m || pc_isdead(sd)) { //�}�b�v���Ⴄ������������ł���
		sd->canact_tick = tick;
		sd->canmove_tick = tick;
		sd->skillitem = sd->skillitemlv = -1;
		return 0;
	}

	if(sd->skillid == PR_LEXAETERNA) {
		struct status_change *sc_data = status_get_sc_data(bl);
		if(sc_data && (sc_data[SC_FREEZE].timer != -1 || (sc_data[SC_STONE].timer != -1 && sc_data[SC_STONE].val2 == 0))) {
			clif_skill_fail(sd,sd->skillid,0,0);
			sd->canact_tick = tick;
			sd->canmove_tick = tick;
			sd->skillitem = sd->skillitemlv = -1;
			return 0;
		}
	}
	else if(sd->skillid == RG_BACKSTAP) {
		int dir = map_calc_dir(&sd->bl,bl->x,bl->y),t_dir = status_get_dir(bl);
		int dist = distance(sd->bl.x,sd->bl.y,bl->x,bl->y);
		if(bl->type != BL_SKILL && (dist == 0 || map_check_dir(dir,t_dir))) {
			clif_skill_fail(sd,sd->skillid,0,0);
			sd->canact_tick = tick;
			sd->canmove_tick = tick;
			sd->skillitem = sd->skillitemlv = -1;
			return 0;
		}
	}

	inf2 = skill_get_inf2(sd->skillid);
	if( ( (skill_get_inf(sd->skillid)&1) || inf2&4 ) &&	// �މ�G�Ί֌W�`�F�b�N
		battle_check_target(&sd->bl,bl, BCT_ENEMY)<=0 ) {
		sd->canact_tick = tick;
		sd->canmove_tick = tick;
		sd->skillitem = sd->skillitemlv = -1;
		return 0;
	}
	if(inf2 & 0xC00 && sd->bl.id != bl->id) {
		int fail_flag = 1;
		if(inf2 & 0x400 && battle_check_target(&sd->bl,bl, BCT_PARTY) > 0)
			fail_flag = 0;
		if(inf2 & 0x800 && sd->status.guild_id > 0 && sd->status.guild_id == status_get_guild_id(bl))
			fail_flag = 0;
		if(fail_flag) {
			clif_skill_fail(sd,sd->skillid,0,0);
			sd->canact_tick = tick;
			sd->canmove_tick = tick;
			sd->skillitem = sd->skillitemlv = -1;
			return 0;
		}
	}

	if(skill_get_nk(sd->skillid)&4)
		if(!path_search_long(NULL,sd->bl.m,sd->bl.x,sd->bl.y,bl->x,bl->y)) {//�ː��`�F�b�N
			clif_skill_fail(sd,sd->skillid,0,0);
			sd->canact_tick = tick;
			sd->canmove_tick = tick;
			if(!battle_config.skill_out_range_consume)
				sd->skillitem = sd->skillitemlv = -1;
			return 0;
		}
	if(!skill_check_condition(sd,1)) {		/* �g�p�����`�F�b�N */
		sd->canact_tick = tick;
		sd->canmove_tick = tick;
		sd->skillitem = sd->skillitemlv = -1;
		return 0;
	}
	sd->skillitem = sd->skillitemlv = -1;

	if(battle_config.pc_skill_log)
		printf("PC %d skill castend skill=%d\n",sd->bl.id,sd->skillid);
	pc_stop_walking(sd,0);

	switch( skill_get_nk(sd->skillid)&3 )
	{
	/* �U���n/������΂��n */
	case 0:	case 2:
		skill_castend_damage_id(&sd->bl,bl,sd->skillid,sd->skilllv,tick,0);
		break;
	case 1:/* �x���n */
		if( (sd->skillid==AL_HEAL || sd->skillid==PR_SANCTUARY || sd->skillid==ALL_RESURRECTION || sd->skillid==PR_ASPERSIO) && battle_check_undead(status_get_race(bl),status_get_elem_type(bl)))
			if(bl->type!=BL_PC)
				skill_castend_damage_id(&sd->bl,bl,sd->skillid,sd->skilllv,tick,0);
			else if (map[sd->bl.m].flag.pvp || map[sd->bl.m].flag.gvg)
				skill_castend_damage_id(&sd->bl,bl,sd->skillid,sd->skilllv,tick,0);
			else break;
		else
			skill_castend_nodamage_id(&sd->bl,bl,sd->skillid,sd->skilllv,tick,0);
		break;
	}

	return 0;
}

/*==========================================
 * �X�L���g�p�i�r�������A�ꏊ�w��̎��ۂ̏����j
 *------------------------------------------
 */
int skill_castend_pos2( struct block_list *src, int x,int y,int skillid,int skilllv,unsigned int tick,int flag)
{
	struct map_session_data *sd=NULL;
	int i,tmpx = 0,tmpy = 0, x1 = 0, y1 = 0;

	nullpo_retr(0, src);

	if(src->type==BL_PC){
		nullpo_retr(0, sd=(struct map_session_data *)src);
	}
	if( skillid != WZ_METEOR && 
		skillid != AM_CANNIBALIZE &&
		skillid != AM_SPHEREMINE)
		clif_skill_poseffect(src,skillid,skilllv,x,y,tick);

	switch(skillid)
	{
	case PR_BENEDICTIO:			/* ���̍~�� */
		skill_area_temp[1]=src->id;
		map_foreachinarea(skill_area_sub,
			src->m,x-1,y-1,x+1,y+1,0,
			src,skillid,skilllv,tick, flag|BCT_NOENEMY|1,
			skill_castend_nodamage_id);
		map_foreachinarea(skill_area_sub,
			src->m,x-1,y-1,x+1,y+1,0,
			src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
			skill_castend_damage_id);
		break;

/*	case BS_HAMMERFALL:			// �n���}�[�t�H�[��
		skill_area_temp[1]=src->id;
		skill_area_temp[2]=x;
		skill_area_temp[3]=y;
		map_foreachinarea(skill_area_sub,
			src->m,x-2,y-2,x+2,y+2,0,
			src,skillid,skilllv,tick, flag|BCT_ENEMY|2,
			skill_castend_nodamage_id);
		break;
*/
	case BS_HAMMERFALL:
		{
			int x0, y0, x1, y1;

			if (skilllv > 5)
			{
				x0 = x - 14;
				y0 = y - 14;
				x1 = x + 14;
				y1 = y + 14;
				skilllv = 5;	// �X�^�����オ�肷���邽�ߌv�Z��Lv5�ŌŒ�
			}
			else
			{
				x0 = x - 2;
				y0 = y - 2;
				x1 = x + 2;
				y1 = y + 2;
			}
			
			skill_area_temp[1] = src->id;
			skill_area_temp[2] = x;
			skill_area_temp[3] = y;
			map_foreachinarea(skill_area_sub, src->m, x0, y0, x1, y1, 0, src, skillid, skilllv, tick, flag|BCT_ENEMY|2, skill_castend_nodamage_id);
		}
		break;
	case HT_DETECTING:				/* �f�B�e�N�e�B���O */
		{
			const int range=7;
			if(src->x!=x)
				x+=(src->x-x>0)?-range:range;
			if(src->y!=y)
				y+=(src->y-y>0)?-range:range;
			map_foreachinarea( status_change_timer_sub,
				src->m, x-range, y-range, x+range,y+range,0,
				src,SC_SIGHT,tick);
		}
		break;

	case MG_SAFETYWALL:			/* �Z�C�t�e�B�E�H�[�� */
	case MG_FIREWALL:			/* �t�@�C���[�E�H�[�� */
	case MG_THUNDERSTORM:		/* �T���_�[�X�g�[�� */
	case AL_PNEUMA:				/* �j���[�} */
	case WZ_ICEWALL:			/* �A�C�X�E�H�[�� */
	case WZ_FIREPILLAR:			/* �t�@�C�A�s���[ */
	case WZ_QUAGMIRE:			/* �N�@�O�}�C�A */
	case WZ_VERMILION:			/* ���[�h�I�u���@�[�~���I�� */
	case WZ_STORMGUST:			/* �X�g�[���K�X�g */
	case WZ_HEAVENDRIVE:		/* �w�����Y�h���C�u */
	case PR_SANCTUARY:			/* �T���N�`���A�� */
	case PR_MAGNUS:				/* �}�O�k�X�G�N�\�V�Y�� */
	case CR_GRANDCROSS:			/* �O�����h�N���X */
	case NPC_DARKGRANDCROSS:	/*�ŃO�����h�N���X*/
	case HT_SKIDTRAP:			/* �X�L�b�h�g���b�v */
	case HT_LANDMINE:			/* �����h�}�C�� */
	case HT_ANKLESNARE:			/* �A���N���X�l�A */
	case HT_SHOCKWAVE:			/* �V���b�N�E�F�[�u�g���b�v */
	case HT_SANDMAN:			/* �T���h�}�� */
	case HT_FLASHER:			/* �t���b�V���[ */
	case HT_FREEZINGTRAP:		/* �t���[�W���O�g���b�v */
	case HT_BLASTMINE:			/* �u���X�g�}�C�� */
	case HT_CLAYMORETRAP:		/* �N���C���A�[�g���b�v */
	case AS_VENOMDUST:			/* �x�m���_�X�g */
	case AM_DEMONSTRATION:		/* �f�����X�g���[�V���� */
	case PF_SPIDERWEB:			/* �X�p�C�_�[�E�F�b�u */
	case PF_FOGWALL:			/* �t�H�O�E�H�[�� */
	case HT_TALKIEBOX:			/* �g�[�L�[�{�b�N�X */
		skill_unitsetting(src,skillid,skilllv,x,y,0);
		break;
	case RG_GRAFFITI:			/* �O���t�B�e�B */
		status_change_start(src,SkillStatusChangeTable[skillid],skilllv,x,y,0,skill_get_time(skillid,skilllv),0 );
		break;

	case SA_VOLCANO:		/* �{���P�[�m */
	case SA_DELUGE:			/* �f�����[�W */
	case SA_VIOLENTGALE:	/* �o�C�I�����g�Q�C�� */
	case SA_LANDPROTECTOR:	/* �����h�v���e�N�^�[ */
		skill_clear_element_field(src);//���Ɏ������������Ă��鑮������N���A
		skill_unitsetting(src,skillid,skilllv,x,y,0);
		break;

	case WZ_METEOR:				//���e�I�X�g�[��
		{
			int flag=0;
			for(i=0;i<2+(skilllv>>1);i++) {
				int j=0;
				do {
					tmpx = x + (atn_rand()%7 - 3);
					tmpy = y + (atn_rand()%7 - 3);
					if(tmpx < 0)
						tmpx = 0;
					else if(tmpx >= map[src->m].xs)
						tmpx = map[src->m].xs - 1;
					if(tmpy < 0)
						tmpy = 0;
					else if(tmpy >= map[src->m].ys)
						tmpy = map[src->m].ys - 1;
					j++;
				} while((map_getcell(src->m,tmpx,tmpy,CELL_CHKNOPASS)) && j<100);
				if(j >= 100)
					continue;
				if(flag==0){
					clif_skill_poseffect(src,skillid,skilllv,tmpx,tmpy,tick);
					flag=1;
				}
				if(i > 0)
					skill_addtimerskill(src,tick+i*1000,0,tmpx,tmpy,skillid,skilllv,(x1<<16)|y1,flag);
				x1 = tmpx;
				y1 = tmpy;
			}
			skill_addtimerskill(src,tick+i*1000,0,tmpx,tmpy,skillid,skilllv,-1,flag);
		}
		break;

	case AL_WARP:				/* ���[�v�|�[�^�� */
		if(sd) {
			if(battle_config.noportal_flag){
				if(map[sd->bl.m].flag.noportal)	break;	/* noportal�ŋ֎~ */
			}else{
				if(map[sd->bl.m].flag.noteleport)	break;	/* noteleport�ŋ֎~ */
			}
			clif_skill_warppoint(sd,sd->skillid,sd->status.save_point.map,
				(sd->skilllv>1)?sd->status.memo_point[0].map:"",
				(sd->skilllv>2)?sd->status.memo_point[1].map:"",
				(sd->skilllv>3)?sd->status.memo_point[2].map:"");
		}
		break;
	case MO_BODYRELOCATION://�c�e
		if(sd){
			pc_movepos(sd,x,y);
			sd->skillstatictimer[MO_EXTREMITYFIST] = tick + 2000;
		}else if( src->type==BL_MOB )
			mob_warp((struct mob_data *)src,-1,x,y,0);
		clif_skill_poseffect(src,skillid,skilllv,x,y,tick);
		break;
	case TK_HIGHJUMP://���荂����
		if(sd){
			pc_movepos(sd,x,y);
		}else if( src->type==BL_MOB )
			mob_warp((struct mob_data *)src,-1,x,y,0);
		break;
	case AM_CANNIBALIZE:	// �o�C�I�v�����g
		if(sd){
			int mx,my,id=0;
			int summons[5] = { 1589, 1579, 1575, 1555, 1590 };

			struct mob_data *md;

			mx = x;// + (atn_rand()%10 - 5);
			my = y;// + (atn_rand()%10 - 5);

			id = mob_once_spawn(sd,"this", mx, my, sd->status.name, summons[skilllv-1], 1, "");

			if( (md=(struct mob_data *)map_id2bl(id)) !=NULL ){
				md->master_id=sd->bl.id;
				md->hp=1500+skilllv*200+sd->status.base_level*10;
				md->state.special_mob_ai=1;
				//��ړ��ŃA�N�e�B�u�Ŕ�������[0x0:��ړ� 0x1:�ړ� 0x4:ACT 0x8:��ACT 0x40:������ 0x80:�����L]
				md->mode=0x0+0x4+0x80;
				md->deletetimer=add_timer(gettick()+skill_get_time(skillid,skilllv),mob_timer_delete,id,0);
				
				md->state.nodrop= battle_config.cannibalize_no_drop;
				md->state.noexp = battle_config.cannibalize_no_exp;
				md->state.nomvp = battle_config.cannibalize_no_mvp;
			}
			
			clif_skill_poseffect(src,skillid,skilllv,x,y,tick);
		}
		break;
	case AM_SPHEREMINE:	// �X�t�B�A�[�}�C��
		if(sd){
			int mx,my,id=0;
			struct mob_data *md;

			mx = x;// + (atn_rand()%10 - 5);
			my = y;// + (atn_rand()%10 - 5);
			id = mob_once_spawn(sd,"this", mx, my, sd->status.name, 1142, 1, "");

			if( (md=(struct mob_data *)map_id2bl(id)) !=NULL ){
				md->master_id=sd->bl.id;
				md->hp=1000+skilllv*200;
				md->state.special_mob_ai=2;
				md->deletetimer=add_timer(gettick()+skill_get_time(skillid,skilllv),mob_timer_delete,id,0);
			}
			clif_skill_poseffect(src,skillid,skilllv,x,y,tick);
		}
		break;
	// Slim Pitcher [Celest]
	case CR_SLIMPITCHER:
		{
			if(battle_config.slimpitcher_nocost && map[sd->bl.m].flag.pvp==0 && map[sd->bl.m].flag.gvg==0)
			{
				if (sd) {
					int itemid[10] = {501,501,501,501,501,503,503,503,503,504};
					int i = skilllv%11 - 1;
					int j = pc_search_inventory(sd,itemid[i]);
					if(j < 0 || itemid[i] <= 0 || sd->inventory_data[j] == NULL ||
						sd->status.inventory[j].amount < skill_db[skillid].amount[i]) {
						clif_skill_fail(sd,skillid,0,0);
						return 1;
					}
					sd->state.potionpitcher_flag = 1;
					sd->potion_hp = 0;
					run_script(sd->inventory_data[j]->use_script,0,sd->bl.id,0);
					pc_delitem(sd,j,skill_db[skillid].amount[i],0);
					sd->state.potionpitcher_flag = 0;
					clif_skill_poseffect(src,skillid,skilllv,x,y,tick);
					if(sd->potion_hp > 0) {
						map_foreachinarea(skill_area_sub,
							src->m,x-3,y-3,x+3,y+3,0,
							src,skillid,skilllv,tick,flag|BCT_PARTY|1,
							skill_castend_nodamage_id);
					}
				}
			}
			else
			{
				if (sd) {
					int i = skilllv%11 - 1;
					int j = pc_search_inventory(sd,skill_db[skillid].itemid[i]);
					if(j < 0 || skill_db[skillid].itemid[i] <= 0 || sd->inventory_data[j] == NULL ||
						sd->status.inventory[j].amount < skill_db[skillid].amount[i]) {
						clif_skill_fail(sd,skillid,0,0);
						return 1;
					}
					sd->state.potionpitcher_flag = 1;
					sd->potion_hp = 0;
					run_script(sd->inventory_data[j]->use_script,0,sd->bl.id,0);
					pc_delitem(sd,j,skill_db[skillid].amount[i],0);
					sd->state.potionpitcher_flag = 0;
					clif_skill_poseffect(src,skillid,skilllv,x,y,tick);
					if(sd->potion_hp > 0) {
						map_foreachinarea(skill_area_sub,
							src->m,x-3,y-3,x+3,y+3,0,
							src,skillid,skilllv,tick,flag|BCT_PARTY|1,
							skill_castend_nodamage_id);
					}
				}
			}
			
		}
		break;

	}

	return 0;
}

/*==========================================
 * �X�L���g�p�i�r�������Amap�w��j
 *------------------------------------------
 */
int skill_castend_map( struct map_session_data *sd,int skill_num, const char *map)
{
	int x=0,y=0;

	nullpo_retr(0, sd);
	if( sd->bl.prev == NULL || pc_isdead(sd) )
		return 0;

	if( sd->opt1>0 || sd->status.option&2 )
		return 0;
	//�X�L�����g���Ȃ���Ԉُ풆
	if(sd->sc_data){
		if( sd->sc_data[SC_DIVINA].timer!=-1 ||
			sd->sc_data[SC_ROKISWEIL].timer!=-1 ||
			sd->sc_data[SC_AUTOCOUNTER].timer != -1 ||
			sd->sc_data[SC_STEELBODY].timer != -1 ||
			sd->sc_data[SC_DANCING].timer!=-1 ||
			sd->sc_data[SC_BERSERK].timer != -1 )
			return 0;
	}

	if( skill_num != sd->skillid)	/* �s���p�P�b�g�炵�� */
		return 0;

	pc_stopattack(sd);

	if(battle_config.pc_skill_log)
		printf("PC %d skill castend skill =%d map=%s\n",sd->bl.id,skill_num,map);
	pc_stop_walking(sd,0);

	if(strcmp(map,"cancel")==0)
		return 0;

	switch(skill_num){
	case AL_TELEPORT:		/* �e���|�[�g */
		if(strcmp(map,"Random")==0)
			pc_randomwarp(sd,3);
		else
			pc_setpos(sd,sd->status.save_point.map,
				sd->status.save_point.x,sd->status.save_point.y,3);
		break;

	case AL_WARP:			/* ���[�v�|�[�^�� */
		{
			const struct point *p[4];
			struct skill_unit_group *group;
			int i;
			int maxcount=0;
			p[0] = &sd->status.save_point;
			p[1] = &sd->status.memo_point[0];
			p[2] = &sd->status.memo_point[1];
			p[3] = &sd->status.memo_point[2];

			if((maxcount = skill_get_maxcount(sd->skillid)) > 0) {
				int c;
				for(i=c=0;i<MAX_SKILLUNITGROUP;i++) {
					if(sd->skillunit[i].alive_count > 0 && sd->skillunit[i].skill_id == sd->skillid)
						c++;
				}
				if(c >= maxcount) {
					clif_skill_fail(sd,sd->skillid,0,0);
					sd->canact_tick = gettick();
					sd->canmove_tick = gettick();
					sd->skillitem = sd->skillitemlv = -1;
					return 0;
				}
			}

			for(i=0;i<sd->skilllv;i++){
				if(strcmp(map,p[i]->map)==0){
					x=p[i]->x;
					y=p[i]->y;
					break;
				}
			}
			if(x==0 || y==0)	/* �s���p�P�b�g�H */
				return 0;

			if(!skill_check_condition(sd,3))
				return 0;
			if((group=skill_unitsetting(&sd->bl,sd->skillid,sd->skilllv,sd->skillx,sd->skilly,0))==NULL)
				return 0;
			group->valstr=(char *)aCalloc(24,sizeof(char));
			memcpy(group->valstr,map,24);
			group->val2=(x<<16)|y;
		}
		break;
	}

	return 0;
}

/*==========================================
 * �X�L�����j�b�g�ݒ菈��
 *------------------------------------------
 */
struct skill_unit_group *skill_unitsetting( struct block_list *src, int skillid,int skilllv,int x,int y,int flag)
{
	struct skill_unit_group *group;
	int i,limit,val1=0,val2=0,val3=0;
	int target,interval,range,unit_flag;
	struct skill_unit_layout *layout;

	nullpo_retr(0, src);

	limit = skill_get_time(skillid,skilllv);
	range = skill_get_unit_range(skillid);
	interval = skill_get_unit_interval(skillid);
	target = skill_get_unit_target(skillid);
	unit_flag = skill_get_unit_flag(skillid);
	layout = skill_get_unit_layout(skillid,skilllv,src,x,y);

	if (unit_flag&UF_DEFNOTENEMY && battle_config.defnotenemy)
		target = BCT_NOENEMY;

	switch (skillid) {
	case MG_SAFETYWALL:			/* �Z�C�t�e�B�E�H�[�� */
		val2 = skilllv+1;
		break;
	case WZ_METEOR:
		if(skilllv>10)			//�L�͈̓��e�I
		range = 10;
		break;
	case WZ_VERMILION:
		if(skilllv>10)			//�L�͈�LOV
		range = 25;
		break;
	case MG_FIREWALL:			/* �t�@�C���[�E�H�[�� */
		val2 = 4+skilllv;
		break;
	case AL_WARP:				/* ���[�v�|�[�^�� */
		val1 = skilllv+6;
		if(flag==0)
			limit=2000;
		break;
	case PR_SANCTUARY:			/* �T���N�`���A�� */
		val1 = skilllv*2+6;
		val2 = (skilllv>6)?777:skilllv*100;
		interval = interval + 500;
		break;
	case WZ_FIREPILLAR:			/* �t�@�C�A�[�s���[ */
		if (flag!=0)
			limit = 150;
		val1 = skilllv+2;
		break;
	case HT_SHOCKWAVE:			/* �V���b�N�E�F�[�u�g���b�v */
		val1 = skilllv*15+10;
		break;
	break;

	case BA_WHISTLE:			/* ���J */
		if(src->type == BL_PC)
			val1 = (pc_checkskill((struct map_session_data *)src,BA_MUSICALLESSON)+1)>>1;
		val2 = ((status_get_agi(src)/10)&0xffff)<<16;
		val2 |= (status_get_luk(src)/10)&0xffff;
		break;
	case DC_HUMMING:			/* �n�~���O */
		if(src->type == BL_PC)
			val1 = (pc_checkskill((struct map_session_data *)src,DC_DANCINGLESSON)+1)>>1;
		val2 = status_get_dex(src)/10;
		break;
	case DC_DONTFORGETME:		/* ����Y��Ȃ��Łc */
		if(src->type == BL_PC)
			val1 = (pc_checkskill((struct map_session_data *)src,DC_DANCINGLESSON)+1)>>1;
		val2 = ((status_get_str(src)/20)&0xffff)<<16;
		val2 |= (status_get_agi(src)/10)&0xffff;
		break;
	case BA_POEMBRAGI:			/* �u���M�̎� */
		if(src->type == BL_PC)
			val1 = pc_checkskill((struct map_session_data *)src,BA_MUSICALLESSON);
		val2 = ((status_get_dex(src)/10)&0xffff)<<16;
		val2 |= (status_get_int(src)/5)&0xffff;
		break;
	case BA_APPLEIDUN:			/* �C�h�D���̗ь� */
		if (src->type==BL_PC)
			val1 = pc_checkskill((struct map_session_data *)src,BA_MUSICALLESSON);
		val2 = status_get_vit(src);
		val3 = 0;
	break;
	case DC_SERVICEFORYOU:		/* �T�[�r�X�t�H�[���[ */
		if(src->type == BL_PC)
			val1 = (pc_checkskill((struct map_session_data *)src,DC_DANCINGLESSON)+1)>>1;
		val2 = status_get_int(src)/10;
		break;
	case BA_ASSASSINCROSS:		/* �[�z�̃A�T�V���N���X */
		if(src->type == BL_PC)
			val1 = (pc_checkskill((struct map_session_data *)src,BA_MUSICALLESSON)+1)>>1;
		val2 = status_get_agi(src)/20;
		break;
	case DC_FORTUNEKISS:		/* �K�^�̃L�X */
		if(src->type == BL_PC)
			val1 = (pc_checkskill((struct map_session_data *)src,DC_DANCINGLESSON)+1)>>1;
		val2 = status_get_luk(src)/10;
		break;
	case HP_BASILICA:
		val1 = src->id;
		break;
	}

	nullpo_retr(NULL, group=skill_initunitgroup(src,layout->count,skillid,skilllv,skill_get_unit_id(skillid,flag&1)));
	group->limit=limit;
	group->val1=val1;
	group->val2=val2;
	group->val3=val3;
	group->target_flag=target;
	group->interval=interval;
	if(skillid==HT_TALKIEBOX ||
	   skillid==RG_GRAFFITI){
		struct map_session_data *sd;
		group->valstr=(char *)aCalloc(80,sizeof(char));
		if(src->type == BL_PC && (sd=(struct map_session_data *)src))
			memcpy(group->valstr,sd->message,80);
	}

	for (i=0;i<layout->count;i++){
		struct skill_unit *unit;
		int ux,uy,val1=skilllv,val2=0,limit=group->limit,alive=1;
		ux = x + layout->dx[i];
		uy = y + layout->dy[i];
		switch (skillid) {
			case MG_FIREWALL:		/* �t�@�C���[�E�H�[�� */
				val2 = group->val2;
				break;
			case WZ_ICEWALL:		/* �A�C�X�E�H�[�� */
				if(skilllv <= 1)
					val1 = 500;
				else
					val1 = 200+200*skilllv;
				break;
		}
		//����X�L���̏ꍇ�ݒu���W��Ƀ����h�v���e�N�^�[���Ȃ����`�F�b�N
		if(range<=0)
			map_foreachinarea(skill_landprotector,src->m,ux,uy,ux,uy,BL_SKILL,skillid,&alive);

		if(skillid==WZ_ICEWALL && alive){
			val2=map_getcell(src->m,ux,uy,CELL_GETTYPE);
			if(val2==5 || val2==1)
				alive=0;
			else {
				map_setcell(src->m,ux,uy,5);
				clif_changemapcell(src->m,ux,uy,5,0);
			}
		}

		if(unit_flag&UF_PATHCHECK) { //�ː��`�F�b�N
			if(!path_search_long(NULL,src->m,src->x,src->y,ux,uy))
				alive=0;
		}

		if(alive){
			nullpo_retr(NULL, unit=skill_initunit(group,i,ux,uy));
			unit->val1=val1;
			unit->val2=val2;
			unit->limit=limit;
			unit->range=range;
			if (range==0)
				map_foreachinarea(skill_unit_effect,unit->bl.m
					,unit->bl.x,unit->bl.y,unit->bl.x,unit->bl.y
					,0,&unit->bl,gettick(),1);
		}
	}
	return group;
}


/*==========================================
 * �X�L�����j�b�g�̔����C�x���g(�ʒu����)
 *------------------------------------------
 */
int skill_unit_onplace(struct skill_unit *src,struct block_list *bl,unsigned int tick)
{
	struct skill_unit_group *sg;
	struct block_list *ss;
	struct skill_unit *unit2;
	struct status_change *sc_data;
	int type;

	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	if (bl->prev==NULL || !src->alive ||
			(bl->type == BL_PC && pc_isdead((struct map_session_data *)bl)))
		return 0;

	nullpo_retr(0, sg=src->group);
	nullpo_retr(0, ss=map_id2bl(sg->src_id));

	if (battle_check_target(&src->bl,bl,sg->target_flag)<=0)
		return 0;

	// �Ώۂ�LP��ɋ���ꍇ�͖���
	if (map_find_skill_unit_oncell(bl,bl->x,bl->y,SA_LANDPROTECTOR,NULL))
		return 0;

	switch (sg->unit_id) {
	case 0x85:	/* �j���[�} */
	case 0x7e:	/* �Z�C�t�e�B�E�H�[�� */
		type = SkillStatusChangeTable[sg->skill_id];
		sc_data = status_get_sc_data(bl);
		if (sc_data[type].timer==-1)
			status_change_start(bl,type,sg->skill_lv,(int)src,0,0,0,0);
		break;

	case 0x80:	/* ���[�v�|�[�^��(������) */
		if (bl->type==BL_PC) {
			struct map_session_data *sd = (struct map_session_data *)bl;
			if(sd && src->bl.m==bl->m && src->bl.x==bl->x && src->bl.y==bl->y && src->bl.x==sd->to_x && src->bl.y==sd->to_y) {
				if (battle_config.chat_warpportal || !sd->chatID){
					char mapname[24];
					int  x = sg->val2>>16;
					int  y = sg->val2&0xffff;
					strncpy(mapname,sg->valstr,24);
					if (sg->src_id==bl->id || (strcmp(map[src->bl.m].name,sg->valstr)==0 && src->bl.x==(sg->val2>>16) && src->bl.y==(sg->val2&0xffff)))
						skill_delunitgroup(sg);
					if (--sg->val1<=0)
						skill_delunitgroup(sg);
					pc_setpos(sd,mapname,x,y,3);
				}
			}
		} else if(bl->type==BL_MOB && battle_config.mob_warpportal) {
			int m = map_mapname2mapid(sg->valstr);
			mob_warp((struct mob_data *)bl,m,sg->val2>>16,sg->val2&0xffff,3);
		}
		break;

	case 0x8e:	/* �N�@�O�}�C�A */
		sc_data = status_get_sc_data(bl);
		type = SkillStatusChangeTable[sg->skill_id];
		if (bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage)
			break;
		if (sc_data[type].timer!=-1)
			break;
		status_change_start(bl,type,sg->skill_lv,(int)src,0,0,
				skill_get_time2(sg->skill_id,sg->skill_lv),0);
		break;
	case 0x9a:	/* �{���P�[�m */
	case 0x9b:	/* �f�����[�W */
	case 0x9c:	/* �o�C�I�����g�Q�C�� */
		type = SkillStatusChangeTable[sg->skill_id];
		sc_data = status_get_sc_data(bl);
		if (sc_data[type].timer!=-1) {
			unit2 = (struct skill_unit *)sc_data[type].val2;
			if (unit2==src || DIFF_TICK(sg->tick,unit2->group->tick)<=0)
				break;
		}
		status_change_start(bl,type,sg->skill_lv,(int)src,0,0,
				skill_get_time2(sg->skill_id,sg->skill_lv),0);
		break;

	case 0x9e:	/* �q��S */
	case 0x9f:	/* �j�����h�̉� */
	case 0xa0:	/* �i���̍��� */
	case 0xa1:	/* �푾�ۂ̋��� */
	case 0xa2:	/* �j�[�x�����O�̎w�� */
	case 0xa3:	/* ���L�̋��� */
	case 0xa4:	/* �[���̒��� */
	case 0xa5:	/* �s���g�̃W�[�N�t���[�h */
	case 0xa6:	/* �s���a�� */
	case 0xa7:	/* ���J */
	case 0xa8:	/* �[�z�̃A�T�V���N���X */
	case 0xa9:	/* �u���M�̎� */
	case 0xaa:	/* �C�h�D���̗ь� */
	case 0xab:	/* ��������ȃ_���X */
	case 0xac:	/* �n�~���O */
	case 0xad:	/* ����Y��Ȃ��Łc */
	case 0xae:	/* �K�^�̃L�X */
	case 0xaf:	/* �T�[�r�X�t�H�[���[ */
		type = SkillStatusChangeTable[sg->skill_id];
		sc_data = status_get_sc_data(bl);
		if (sg->src_id==bl->id)
			break;
		if (sc_data[type].timer!=-1) {
			unit2 = (struct skill_unit *)sc_data[type].val4;
			if (unit2==src || DIFF_TICK(sg->tick,unit2->group->tick)<=0)
				break;
		}
		status_change_start(bl,type,sg->skill_lv,sg->val1,sg->val2,
				(int)src,skill_get_time2(sg->skill_id,sg->skill_lv),0);
		break;
	case 0xb2:				/* ���Ȃ���_������ł� */
	case 0xb6:				/* �t�H�O�E�H�[�� */
	//�Ƃ肠�����������Ȃ�
		break;
/*	default:
		if(battle_config.error_log)
			printf("skill_unit_onplace: Unknown skill unit id=%d block=%d\n",sg->unit_id,bl->id);
		break;*/
	}

	return 0;
}

/*==========================================
 * �X�L�����j�b�g�̔����C�x���g(�^�C�}�[����)
 *------------------------------------------
 */
int skill_unit_onplace_timer(struct skill_unit *src,struct block_list *bl,unsigned int tick)
{
	struct skill_unit_group *sg;
	struct block_list *ss;
	int splash_count=0;
	struct status_change *sc_data;
	struct skill_unit_group_tickset *ts;
	int type;
	int diff=0;

	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	if (bl->type!=BL_PC && bl->type!=BL_MOB)
		return 0;
	
	if (bl->prev==NULL || !src->alive ||
			(bl->type==BL_PC && pc_isdead((struct map_session_data *)bl)))
		return 0;

	nullpo_retr(0, sg=src->group);
	nullpo_retr(0, ss=map_id2bl(sg->src_id));

	// �Ώۂ�LP��ɋ���ꍇ�͖���
	if (map_find_skill_unit_oncell(bl,bl->x,bl->y,SA_LANDPROTECTOR,NULL))
		return 0;

	// �O�ɉe�����󂯂Ă���interval�̊Ԃ͉e�����󂯂Ȃ�
	nullpo_retr(0,ts = skill_unitgrouptickset_search(bl,sg,tick));
	diff = DIFF_TICK(tick,ts->tick);
	if (sg->skill_id==PR_SANCTUARY)
		diff += 500; // �V�K�ɉ񕜂������j�b�g�����J�E���g���邽�߂̎d�|��
	if (diff<0)
		return 0;
	ts->tick = tick+sg->interval;
	// GX�͏d�Ȃ��Ă�����3HIT���Ȃ�
	if (sg->skill_id==CR_GRANDCROSS && !battle_config.gx_allhit)
		ts->tick += sg->interval*(map_count_oncell(bl->m,bl->x,bl->y)-1);

	switch (sg->unit_id) {
	case 0x83:	/* �T���N�`���A�� */
	{
		int race=status_get_race(bl);
		sc_data=status_get_sc_data(bl);

		if (battle_check_undead(race,status_get_elem_type(bl)) || race==6) {
			if (bl->type==BL_PC)
				if(!(map[bl->m].flag.pvp || map[bl->m].flag.gvg))
				break;
			if (skill_attack(BF_MAGIC,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0))
				sg->val1--;	// �`���b�g�L�����Z���ɑΉ�
		} else {
			int heal = sg->val2;
			if (status_get_hp(bl)>=status_get_max_hp(bl))
				break;
			if(bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage)
				heal=0;	/* ����峃J�[�h�i�q�[���ʂO�j */
			if(sc_data && sc_data[SC_BERSERK].timer!=-1) /* �o�[�T�[�N���̓q�[���O */
				heal=0;
			clif_skill_nodamage(&src->bl,bl,AL_HEAL,heal,1);
			battle_heal(NULL,bl,heal,0,0);
			if (diff>=500)
				sg->val1--;	// �V�K�ɓ��������j�b�g�����J�E���g
		}
		if (sg->val1<=0)
			skill_delunitgroup(sg);
		break;
	}
	case 0x84:	/* �}�O�k�X�G�N�\�V�Y�� */
	{
		int race = status_get_race(bl);
		if (!battle_check_undead(race,status_get_elem_type(bl)) && race!=6)
			return 0;
		skill_attack(BF_MAGIC,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
		src->val2++;
		break;
	}
	case 0x7f:	/* �t�@�C���[�E�H�[�� */
		skill_attack(BF_MAGIC,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
		if (--src->val2<=0)
			skill_delunit(src);
		break;
	case 0x86:	/* ���[�h�I�u���@�[�~���I��(TS,MS,FN,SG,HD,GX,��GX) */
		skill_attack(BF_MAGIC,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
		break;
	case 0x87:	/* �t�@�C�A�[�s���[(�����O) */
		skill_delunit(src);
		skill_unitsetting(ss,sg->skill_id,sg->skill_lv,src->bl.x,src->bl.y,1);
		break;
	case 0x88:	/* �t�@�C�A�[�s���[(������) */
		{
			int i = src->range;
			if(sg->skill_lv>5)
				i += 2;
			map_foreachinarea(skill_attack_area,src->bl.m,src->bl.x-i,src->bl.y-i,src->bl.x+i,src->bl.y+i,0,
				BF_MAGIC,ss,&src->bl,sg->skill_id,sg->skill_lv,tick,0,BCT_ENEMY);
		}
//		skill_attack(BF_MAGIC,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
		break;
	case 0x90:	/* �X�L�b�h�g���b�v */
		{
			int i,c = skill_get_blewcount(sg->skill_id,sg->skill_lv);
			if(map[bl->m].flag.gvg) c = 0;
			for(i=0;i<c;i++)
				skill_blown(&src->bl,bl,1|0x30000);
			sg->unit_id = 0x8c;
			clif_changelook(&src->bl,LOOK_BASE,sg->unit_id);
			sg->limit=DIFF_TICK(tick,sg->tick)+1500;
		}
		break;

	case 0x93:	/* �����h�}�C�� */
		skill_attack(BF_MISC,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
		sg->unit_id = 0x8c;
		clif_changelook(&src->bl,LOOK_BASE,0x88);
		sg->limit=DIFF_TICK(tick,sg->tick)+1500;
		break;

	case 0x8f:	/* �u���X�g�}�C�� */
	case 0x94:	/* �V���b�N�E�F�[�u�g���b�v */
	case 0x95:	/* �T���h�}�� */
	case 0x96:	/* �t���b�V���[ */
	case 0x97:	/* �t���[�W���O�g���b�v */
	case 0x98:	/* �N���C���A�[�g���b�v */
		map_foreachinarea(skill_count_target,src->bl.m
					,src->bl.x-src->range,src->bl.y-src->range
					,src->bl.x+src->range,src->bl.y+src->range
					,0,&src->bl,&splash_count);
		map_foreachinarea(skill_trap_splash,src->bl.m
					,src->bl.x-src->range,src->bl.y-src->range
					,src->bl.x+src->range,src->bl.y+src->range
					,0,&src->bl,tick,splash_count);
		sg->unit_id = 0x8c;
		clif_changelook(&src->bl,LOOK_BASE,sg->unit_id);
		sg->limit=DIFF_TICK(tick,sg->tick)+1500;
		break;

	case 0x91:	/* �A���N���X�l�A */
		sc_data=status_get_sc_data(bl);
		if (sg->val2==0 && sc_data[SC_ANKLE].timer==-1) {
			int moveblock = ( bl->x/BLOCK_SIZE != src->bl.x/BLOCK_SIZE || bl->y/BLOCK_SIZE != src->bl.y/BLOCK_SIZE);
			int sec=(int)(skill_get_time2(sg->skill_id,sg->skill_lv) - (double)status_get_agi(bl)*0.1);
			if(status_get_mode(bl)&0x20)
				sec = sec/5;
			battle_stopwalking(bl,1);
			status_change_start(bl,SC_ANKLE,sg->skill_lv,0,0,0,sec,0);

			skill_unit_move(bl,tick,0);
			if(moveblock) map_delblock(bl);
			bl->x = src->bl.x;
			bl->y = src->bl.y;
			if(moveblock) map_addblock(bl);
			skill_unit_move(bl,tick,1);
 			if(bl->type == BL_MOB)
 				clif_fixmobpos((struct mob_data *)bl);
 			else if(bl->type == BL_PET)
 				clif_fixpetpos((struct pet_data *)bl);
 			else
 				clif_fixpos(bl);
			clif_01ac(&src->bl);
			sg->limit=DIFF_TICK(tick,sg->tick) + sec;
			sg->val2=bl->id;
			sg->interval = -1;
			src->range = 0;
		}
		break;
	case 0x92:	/* �x�m���_�X�g */
		type = SkillStatusChangeTable[sg->skill_id];
		sc_data = status_get_sc_data(bl);
		if (sc_data[type].timer!=-1)
			break;
		status_change_start(bl,type,sg->skill_lv,(int)src,0,0,
				skill_get_time2(sg->skill_id,sg->skill_lv),0);
		break;
	case 0xb1:	/* �f�����X�g���[�V���� */
		skill_attack(BF_WEAPON,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
		if(bl->type == BL_PC && atn_rand()%100 < sg->skill_lv)
			pc_break_equip((struct map_session_data *)bl, EQP_WEAPON);
		break;
	case 0x99:				/* �g�[�L�[�{�b�N�X */
		if(sg->src_id == bl->id) //����������ł��������Ȃ�
			break;
		if(sg->val2==0){
			clif_talkiebox(&src->bl,sg->valstr);
			sg->unit_id = 0x8c;
			clif_changelook(&src->bl,LOOK_BASE,sg->unit_id);
			sg->limit=DIFF_TICK(tick,sg->tick)+5000;
			sg->val2=-1; //����
		}
		break;
	case 0xb3:	/* �S�X�y�� */
		if (sg->src_id==bl->id) {
			struct map_session_data *sd = (struct map_session_data *)bl;
			int hp = (sg->skill_lv <= 5) ? 30 : 45;
			int sp = (sg->skill_lv <= 5) ? 20 : 35;
			if((sd->status.hp - hp)<=0 || (sd->status.sp - sp)<=0){
				status_change_end(bl,SC_GOSPEL,-1);
				skill_delunitgroup(sg);
				break;
			}
			pc_heal((struct map_session_data *)bl,-hp,-sp);
			break;
		}
		if (bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage)
			break;
		if (atn_rand()%100 < 50+sg->skill_lv*5) {
			if (battle_check_target(&src->bl,bl,BCT_ENEMY)>0) {		// �G�Ώ�
				switch(atn_rand()%8) {
				case 0:		// �����_���_���[�W(1000�`9999�H)
					skill_attack(BF_MAGIC,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
					break;
				case 1:		// �􂢌��ʕt�^
					status_change_start(bl,SC_CURSE,sg->skill_lv,0,0,0,
						skill_get_time2(sg->skill_id,sg->skill_lv),0);
					break;
				case 2:		// �Í����ʕt�^
					status_change_start(bl,SC_BLIND,sg->skill_lv,0,0,0,
						skill_get_time2(sg->skill_id,sg->skill_lv),0);
					break;
				case 3:		// �Ō��ʕt�^
					status_change_start(bl,SC_POISON,sg->skill_lv,0,0,0,
						skill_get_time2(sg->skill_id,sg->skill_lv),0);
					break;
				case 4:		// �v���{�b�NLv10���ʕt�^
					status_change_start(bl,SC_PROVOKE,10,0,0,0,
						skill_get_time(SM_PROVOKE,10),0);
					break;
				case 5:		// ATK��0�Ɍ���(��������20�b)
					status_change_start(bl,SC_INCATK2,-100,0,0,0,20000,0);
					break;
				case 6:		// FLEE��0�Ɍ���(��������20�b)
					status_change_start(bl,SC_INCFLEE2,-100,0,0,0,20000,0);
					break;
				case 7:		// HIT��0�Ɍ���(��������50�b)
					status_change_start(bl,SC_INCHIT2,-100,0,0,0,50000,0);
					break;
				}
			}
			if (battle_check_target(&src->bl,bl,BCT_PARTY)>0) {	// ����(PT)�Ώ�
				switch(atn_rand()%10) {
				case 0:		// HP����(1000�`9999�H)
					battle_heal(NULL,bl,1000+atn_rand()%9000,0,0);
					break;
				case 1:		// MHP��100%����(��������60�b)
					status_change_start(bl,SC_INCMHP2,100,0,0,0,60000,0);
					break;
				case 2:		// MSP��100%����(��������60�b)
					status_change_start(bl,SC_INCMSP2,100,0,0,0,60000,0);
					break;
				case 3:		// �S�ẴX�e�[�^�X+20(��������200�b)
					status_change_start(bl,SC_INCALLSTATUS,20,0,0,0,200000,0);
					break;
				case 4:		// �u���b�V���OLv10���ʕt�^
					status_change_start(bl,SC_BLESSING,10,0,0,0,skill_get_time(AL_BLESSING,10),0);
					break;
				case 5:		// ���x����Lv10���ʕt�^
					status_change_start(bl,SC_INCREASEAGI,10,0,0,0,skill_get_time(AL_INCAGI,10),0);
					break;
				case 6:		// ����ɐ��������ʕt�^
					status_change_start(bl,SC_ASPERSIO,sg->skill_lv,0,0,0,
						skill_get_time2(sg->skill_id,sg->skill_lv),0);
					break;
				case 7:		// �Z�ɐ��������ʕt�^
					status_change_start(bl,SC_BENEDICTIO,sg->skill_lv,0,0,0,
						skill_get_time2(sg->skill_id,sg->skill_lv),0);
					break;
				case 8:		// ATK��100%����
					status_change_start(bl,SC_INCATK2,100,0,0,0,
						skill_get_time2(sg->skill_id,sg->skill_lv),0);
					break;
				case 9:		// HIT, FLEE��+50(��������90�b)
					status_change_start(bl,SC_INCHIT,50,0,0,0,90000,0);
					status_change_start(bl,SC_INCFLEE,50,0,0,0,90000,0);
					break;
				}
			}
		}
		break;
	case 0xb4:				/* �o�W���J */
	   	if (battle_check_target(&src->bl,bl,BCT_ENEMY)>0 &&
				!(status_get_mode(bl)&0x20))
			skill_blown(&src->bl,bl,1);
		if (sg->src_id==bl->id)
			break;
		if (battle_check_target(&src->bl,bl,BCT_NOENEMY)>0) {
			type = SkillStatusChangeTable[sg->skill_id];
			status_change_start(bl,type,sg->skill_lv,sg->val1,sg->val2,
				(int)src,sg->interval+100,0);
		}
		break;
	case 0xb7:	/* �X�p�C�_�[�E�F�b�u */
		if(sg->val2==0){
			int moveblock = ( bl->x/BLOCK_SIZE != src->bl.x/BLOCK_SIZE || bl->y/BLOCK_SIZE != src->bl.y/BLOCK_SIZE);
			skill_additional_effect(ss,bl,sg->skill_id,sg->skill_lv,BF_MISC,tick);
			skill_unit_move(bl,tick,0);
			if(moveblock) map_delblock(bl);
			bl->x = src->bl.x;
			bl->y = src->bl.y;
			if(moveblock) map_addblock(bl);
			skill_unit_move(bl,tick,1);
 			if(bl->type == BL_MOB)
 				clif_fixmobpos((struct mob_data *)bl);
 			else if(bl->type == BL_PET)
 				clif_fixpetpos((struct pet_data *)bl);
 			else
 				clif_fixpos(bl);
			sg->limit = DIFF_TICK(tick,sg->tick)+skill_get_time2(sg->skill_id,sg->skill_lv);
			sg->val2=bl->id;
			sg->interval = -1;
			src->range = 0;
		}
		break;
	}

	if(bl->type==BL_MOB && ss!=bl)	/* �X�L���g�p������MOB�X�L�� */
	{
		if(battle_config.mob_changetarget_byskill == 1)
		{
			int target=((struct mob_data *)bl)->target_id;
			if(ss->type == BL_PC)
				((struct mob_data *)bl)->target_id=ss->id;
			mobskill_use((struct mob_data *)bl,tick,MSC_SKILLUSED|(sg->skill_id<<16));
			((struct mob_data *)bl)->target_id=target;
		}
		else
			mobskill_use((struct mob_data *)bl,tick,MSC_SKILLUSED|(sg->skill_id<<16));
	}
	return 0;
}

/*==========================================
 * �X�L�����j�b�g���痣�E
 *------------------------------------------
 */
int skill_unit_onout(struct skill_unit *src,struct block_list *bl,unsigned int tick)
{
	struct skill_unit_group *sg;
	struct status_change *sc_data;
	int type;

	nullpo_retr(0, src);
	nullpo_retr(0, bl);
	nullpo_retr(0, sg=src->group);

	if (bl->prev==NULL || !src->alive ||
			(bl->type == BL_PC && pc_isdead((struct map_session_data *)bl)))
		return 0;

	switch(sg->unit_id){
	case 0x7e:	/* �Z�C�t�e�B�E�H�[�� */
	case 0x85:	/* �j���[�} */
	case 0x8e:	/* �N�@�O�}�C�A */
	case 0x9a:	/* �{���P�[�m */
	case 0x9b:	/* �f�����[�W */
	case 0x9c:	/* �o�C�I�����g�Q�C�� */
		sc_data = status_get_sc_data(bl);
		type = SkillStatusChangeTable[sg->skill_id];
		if (type==SC_QUAGMIRE && bl->type==BL_MOB)
			break;
		if (sc_data[type].timer!=-1 && sc_data[type].val2==(int)src) {
			status_change_end(bl,type,-1);
		}
		break;
	case 0x91:	/* �A���N���X�l�A */
	{
		struct block_list *target=map_id2bl(sg->val2);
		if( target && target==bl ){
			status_change_end(bl,SC_ANKLE,-1);
			sg->limit=DIFF_TICK(tick,sg->tick)+1000;
		}
		break;
	}
		break;
	case 0x9e:	/* �q��S */
	case 0x9f:	/* �j�����h�̉� */
	case 0xa0:	/* �i���̍��� */
	case 0xa1:	/* �푾�ۂ̋��� */
	case 0xa2:	/* �j�[�x�����O�̎w�� */
	case 0xa3:	/* ���L�̋��� */
	case 0xa4:	/* �[���̒��� */
	case 0xa5:	/* �s���g�̃W�[�N�t���[�h */
	case 0xa6:	/* �s���a�� */
	case 0xa7:	/* ���J */
	case 0xa8:	/* �[�z�̃A�T�V���N���X */
	case 0xa9:	/* �u���M�̎� */
	case 0xaa:	/* �C�h�D���̗ь� */
	case 0xab:	/* ��������ȃ_���X */
	case 0xac:	/* �n�~���O */
	case 0xae:	/* �K�^�̃L�X */
	case 0xaf:	/* �T�[�r�X�t�H�[���[ */
	case 0xad:	/* ����Y��Ȃ��Łc */
	case 0xb4:	/* �o�W���J */
		sc_data = status_get_sc_data(bl);
		type = SkillStatusChangeTable[sg->skill_id];
		if (sc_data[type].timer!=-1 && sc_data[type].val4==(int)src) {
			status_change_end(bl,type,-1);
		}
		break;
	case 0xb7:	/* �X�p�C�_�[�E�F�b�u */
	{
		struct block_list *target = map_id2bl(sg->val2);
		if (target && target==bl)
			status_change_end(bl,SC_SPIDERWEB,-1);
		sg->limit = DIFF_TICK(tick,sg->tick)+1000;
		break;
	}

/*	default:
		if(battle_config.error_log)
			printf("skill_unit_onout: Unknown skill unit id=%d block=%d\n",sg->unit_id,bl->id);
		break;*/
	}
	return 0;
}

/*==========================================
 * �X�L�����j�b�g���ʔ���/���E����(foreachinarea)
 *    bl: ���j�b�g(BL_PC/BL_MOB)
 *------------------------------------------
 */
int skill_unit_effect(struct block_list *bl,va_list ap)
{
	struct skill_unit *unit;
	struct skill_unit_group *group;
	int flag;
	unsigned int tick;
	static int called = 0;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, unit=va_arg(ap,struct skill_unit*));
	tick = va_arg(ap,unsigned int);
	flag = va_arg(ap,unsigned int);

	if (bl->type!=BL_PC && bl->type!=BL_MOB)
		return 0;

	if (!unit->alive || bl->prev==NULL)
		return 0;

	nullpo_retr(0, group=unit->group);

	if (flag)
		skill_unit_onplace(unit,bl,tick);
	else {
		skill_unit_onout(unit,bl,tick);
		unit = map_find_skill_unit_oncell(bl,bl->x,bl->y,group->skill_id,unit);
		if (unit && called == 0) {
			called = 1;
			skill_unit_onplace(unit,bl,tick);
			called = 0;
		}
	}

	return 0;
}


/*==========================================
 * �X�L�����j�b�g�̌��E�C�x���g
 *------------------------------------------
 */
int skill_unit_onlimit(struct skill_unit *src,unsigned int tick)
{
	struct skill_unit_group *sg;

	nullpo_retr(0, src);
	nullpo_retr(0, sg=src->group);

	switch(sg->unit_id){
	case 0x8d:	/* �A�C�X�E�H�[�� */
		map_setcell(src->bl.m,src->bl.x,src->bl.y,src->val2);
		clif_changemapcell(src->bl.m,src->bl.x,src->bl.y,src->val2,1);
		break;
	case 0xb2:	/* ���Ȃ��Ɉ������� */
		{
			struct map_session_data *sd = NULL;
//			struct map_session_data *p_sd = NULL;
			if((sd = (struct map_session_data *)(map_id2bl(sg->src_id))) == NULL)
				return 0;
/*
			if((p_sd = pc_get_partner(sd)) == NULL)
				return 0;

			pc_setpos(p_sd,map[src->bl.m].name,src->bl.x,src->bl.y,3);
*/
//			intif_charmovereq(sd,map_charid2nick(sd->status.partner_id),0);
			intif_charmovereq2(sd,map_charid2nick(sd->status.partner_id),map[src->bl.m].name,src->bl.x,src->bl.y,0);
	}
		break;
	}
	return 0;
}
/*==========================================
 * �X�L�����j�b�g�̃_���[�W�C�x���g
 *------------------------------------------
 */
int skill_unit_ondamaged(struct skill_unit *src,struct block_list *bl,
	int damage,unsigned int tick)
{
	struct skill_unit_group *sg;

	nullpo_retr(0, src);
	nullpo_retr(0, sg=src->group);

	switch(sg->unit_id){
	case 0x8d:	/* �A�C�X�E�H�[�� */
		src->val1-=damage;
		break;
	case 0x8f:	/* �u���X�g�}�C�� */
	case 0x98:	/* �N���C���A�[�g���b�v */
		skill_blown(bl,&src->bl,2); //������΂��Ă݂�
		break;
	default:
		damage = 0;
		break;
	}
	return damage;
}


/*---------------------------------------------------------------------------- */



/*==========================================
 * �X�L���g�p�i�r�������A�ꏊ�w��j
 *------------------------------------------
 */
int skill_castend_pos( int tid, unsigned int tick, int id,int data )
{
	struct map_session_data* sd=map_id2sd(id)/*,*target_sd=NULL*/;
	int range,maxcount;

	nullpo_retr(0, sd);

	if( sd->bl.prev == NULL )
		return 0;
	if( sd->skilltimer != tid )	/* �^�C�}ID�̊m�F */
		return 0;
	if(sd->skilltimer != -1 && pc_checkskill(sd,SA_FREECAST) > 0) {
		sd->speed = sd->prev_speed;
		clif_updatestatus(sd,SP_SPEED);
	}
	sd->skilltimer=-1;
	if(pc_isdead(sd)) {
		sd->canact_tick = tick;
		sd->canmove_tick = tick;
		sd->skillitem = sd->skillitemlv = -1;
		return 0;
	}

	if (!battle_config.pc_skill_reiteration &&
			skill_get_unit_flag(sd->skillid)&UF_NOREITERATION &&
			skill_check_unit_range(sd->bl.m,sd->skillx,sd->skilly,sd->skillid,sd->skilllv)) {
		clif_skill_fail(sd,sd->skillid,0,0);
		sd->canact_tick = tick;
		sd->canmove_tick = tick;
		sd->skillitem = sd->skillitemlv = -1;
		return 0;
	}
	if (battle_config.pc_skill_nofootset &&
			skill_get_unit_flag(sd->skillid)&UF_NOFOOTSET &&
			skill_check_unit_range2(sd->bl.m,sd->skillx,sd->skilly,sd->skillid,sd->skilllv)) {
		clif_skill_fail(sd,sd->skillid,0,0);
		sd->canact_tick = tick;
		sd->canmove_tick = tick;
		sd->skillitem = sd->skillitemlv = -1;
		return 0;
	}

	if(battle_config.pc_land_skill_limit) {
		maxcount = skill_get_maxcount(sd->skillid);
		if(maxcount > 0) {
			int i,c;
			for(i=c=0;i<MAX_SKILLUNITGROUP;i++) {
				if(sd->skillunit[i].alive_count > 0 && sd->skillunit[i].skill_id == sd->skillid)
					c++;
			}
			if(c >= maxcount) {
				clif_skill_fail(sd,sd->skillid,0,0);
				sd->canact_tick = tick;
				sd->canmove_tick = tick;
				sd->skillitem = sd->skillitemlv = -1;
				return 0;
			}
		}
	}

	range = skill_get_range(sd->skillid,sd->skilllv);
	if(range < 0)
		range = status_get_range(&sd->bl) - (range + 1);
	range += battle_config.pc_skill_add_range;
	if(!battle_config.skill_out_range_consume) {
		if(range < distance(sd->bl.x,sd->bl.y,sd->skillx,sd->skilly)) {
			clif_skill_fail(sd,sd->skillid,0,0);
			sd->canact_tick = tick;
			sd->canmove_tick = tick;
			sd->skillitem = sd->skillitemlv = -1;
			return 0;
		}
	}
	if(!skill_check_condition(sd,1)) {		/* �g�p�����`�F�b�N */
		sd->canact_tick = tick;
		sd->canmove_tick = tick;
		sd->skillitem = sd->skillitemlv = -1;
		return 0;
	}
	sd->skillitem = sd->skillitemlv = -1;
	if(battle_config.skill_out_range_consume) {
		if(range < distance(sd->bl.x,sd->bl.y,sd->skillx,sd->skilly)) {
			clif_skill_fail(sd,sd->skillid,0,0);
			sd->canact_tick = tick;
			sd->canmove_tick = tick;
			return 0;
		}
	}

	if(battle_config.pc_skill_log)
		printf("PC %d skill castend skill=%d\n",sd->bl.id,sd->skillid);
	pc_stop_walking(sd,0);

	skill_castend_pos2(&sd->bl,sd->skillx,sd->skilly,sd->skillid,sd->skilllv,tick,0);

	return 0;
}

/*==========================================
 * �͈͓��L�������݊m�F���菈��(foreachinarea)
 *------------------------------------------
 */

static int skill_check_condition_char_sub(struct block_list *bl,va_list ap)
{
	int *c;
	struct block_list *src;
	struct map_session_data *sd;
	struct map_session_data *ssd;
	struct pc_base_job s_class;
	struct pc_base_job ss_class;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd=(struct map_session_data*)bl);
	nullpo_retr(0, src=va_arg(ap,struct block_list *));
	nullpo_retr(0, c=va_arg(ap,int *));
	nullpo_retr(0, ssd=(struct map_session_data*)src);

	s_class = pc_calc_base_job(sd->status.class);
	//�`�F�b�N���Ȃ��ݒ�Ȃ�c�ɂ��肦�Ȃ��傫�Ȑ�����Ԃ��ďI��
	if(!battle_config.player_skill_partner_check){	//�{����foreach�̑O�ɂ�肽�����ǐݒ�K�p�ӏ����܂Ƃ߂邽�߂ɂ�����
		(*c)=99;
		return 0;
	}

	;
	ss_class = pc_calc_base_job(ssd->status.class);

	switch(ssd->skillid){
	case PR_BENEDICTIO:				/* ���̍~�� */
		if(sd != ssd && (s_class.job == 4 || s_class.job == 8 || s_class.job == 15) && (sd->bl.x == ssd->bl.x - 1 || sd->bl.x == ssd->bl.x + 1) && sd->status.sp >= 10)
			(*c)++;
		break;
	case BD_LULLABY:				/* �q��� */
	case BD_RICHMANKIM:				/* �j�����h�̉� */
	case BD_ETERNALCHAOS:			/* �i���̍��� */
	case BD_DRUMBATTLEFIELD:		/* �푾�ۂ̋��� */
	case BD_RINGNIBELUNGEN:			/* �j�[�x�����O�̎w�� */
	case BD_ROKISWEIL:				/* ���L�̋��� */
	case BD_INTOABYSS:				/* �[���̒��� */
	case BD_SIEGFRIED:				/* �s���g�̃W�[�N�t���[�h */
	case BD_RAGNAROK:				/* �_�X�̉��� */
	case CG_MOONLIT:				/* ������̐�ɗ�����Ԃт� */
		if(sd != ssd &&
		 ((ss_class.job==19 && s_class.job==20) ||
		 (ss_class.job==20 && s_class.job==19)) &&
		 pc_checkskill(sd,ssd->skillid) > 0 &&
		 (*c)==0 &&
		 sd->status.party_id == ssd->status.party_id &&
		 !pc_issit(sd) &&
		 sd->sc_data[SC_DANCING].timer==-1
		 )
			(*c)=pc_checkskill(sd,ssd->skillid);
		break;
	}
	return 0;
}
/*==========================================
 * �͈͓��L�������݊m�F�����X�L���g�p����(foreachinarea)
 *------------------------------------------
 */

static int skill_check_condition_use_sub(struct block_list *bl,va_list ap)
{
	int *c;
	struct block_list *src;
	struct map_session_data *sd;
	struct map_session_data *ssd;
	struct pc_base_job s_class;
	struct pc_base_job ss_class;
	int skillid,skilllv;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd=(struct map_session_data*)bl);
	nullpo_retr(0, src=va_arg(ap,struct block_list *));
	nullpo_retr(0, c=va_arg(ap,int *));
	nullpo_retr(0, ssd=(struct map_session_data*)src);

	s_class = pc_calc_base_job(sd->status.class);

	
	//�`�F�b�N���Ȃ��ݒ�Ȃ�c�ɂ��肦�Ȃ��傫�Ȑ�����Ԃ��ďI��
	if(!battle_config.player_skill_partner_check){	//�{����foreach�̑O�ɂ�肽�����ǐݒ�K�p�ӏ����܂Ƃ߂邽�߂ɂ�����
		(*c)=99;
		return 0;
	}

	ss_class = pc_calc_base_job(ssd->status.class);
	skillid=ssd->skillid;
	skilllv=ssd->skilllv;
	switch(skillid){
	case PR_BENEDICTIO:				/* ���̍~�� */
		if(sd != ssd && (s_class.job == 4 || s_class.job == 8) && (sd->bl.x == ssd->bl.x - 1 || sd->bl.x == ssd->bl.x + 1) && sd->status.sp >= 10){
			sd->status.sp -= 10;
			status_calc_pc(sd,0);
			(*c)++;
		}
		break;
	case BD_LULLABY:				/* �q��� */
	case BD_RICHMANKIM:				/* �j�����h�̉� */
	case BD_ETERNALCHAOS:			/* �i���̍��� */
	case BD_DRUMBATTLEFIELD:		/* �푾�ۂ̋��� */
	case BD_RINGNIBELUNGEN:			/* �j�[�x�����O�̎w�� */
	case BD_ROKISWEIL:				/* ���L�̋��� */
	case BD_INTOABYSS:				/* �[���̒��� */
	case BD_SIEGFRIED:				/* �s���g�̃W�[�N�t���[�h */
	case BD_RAGNAROK:				/* �_�X�̉��� */
	case CG_MOONLIT:				/* ������̉��� */
		if(sd != ssd && //�{�l�ȊO��
		  ((ss_class.job==19 && s_class.job==20) || //�������o�[�h�Ȃ�_���T�[��
		   (ss_class.job==20 && s_class.job==19)) && //�������_���T�[�Ȃ�o�[�h��
		   pc_checkskill(sd,skillid) > 0 && //�X�L���������Ă���
		   (*c)==0 && //�ŏ��̈�l��
		   sd->status.party_id == ssd->status.party_id && //�p�[�e�B�[��������
		   !pc_issit(sd) && //�����ĂȂ�
		   sd->sc_data[SC_DANCING].timer==-1 //�_���X������Ȃ�
		  ){
			ssd->sc_data[SC_DANCING].val4=bl->id;
			clif_skill_nodamage(bl,src,skillid,skilllv,1);
			status_change_start(bl,SC_DANCING,skillid,ssd->sc_data[SC_DANCING].val2,0,src->id,skill_get_time(skillid,skilllv)+1000,0);
			sd->skillid_dance=sd->skillid=skillid;
			sd->skilllv_dance=sd->skilllv=skilllv;
			(*c)++;
		}
		break;
	}
	return 0;
}
/*==========================================
 * �͈͓��o�C�I�v�����g�A�X�t�B�A�}�C���pMob���݊m�F���菈��(foreachinarea)
 *------------------------------------------
 */

static int skill_check_condition_mob_master_sub(struct block_list *bl,va_list ap)
{
	int *c,src_id=0,mob_class=0;
	struct mob_data *md;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, md=(struct mob_data*)bl);
	nullpo_retr(0, src_id=va_arg(ap,int));
	nullpo_retr(0, mob_class=va_arg(ap,int));
	nullpo_retr(0, c=va_arg(ap,int *));

	if(md->class==mob_class && md->master_id==src_id)
		(*c)++;
	return 0;
}

/*==========================================
 * �X�L���g�p�����i�U�Ŏg�p���s�j
 *------------------------------------------
 */
int skill_check_condition(struct map_session_data *sd,int type)
{
	int i,hp,sp,hp_rate,sp_rate,zeny,weapon,state,spiritball,skill,lv,skilldb_id,mana;
	int	index[10],itemid[10],amount[10];

	nullpo_retr(0, sd);
	
		
	//GM�n�C�h���ŁA�R���t�B�O�Ńn�C�h���U���s�� GM���x�����w����傫���ꍇ
	if(sd->status.option&0x40 && battle_config.hide_attack == 0	&& pc_isGM(sd)<battle_config.gm_hide_attack_lv)
		return 0;	// �B��ăX�L���g���Ȃ�Ĕڋ���GM�޽�

	if( battle_config.gm_skilluncond>0 && pc_isGM(sd)>= battle_config.gm_skilluncond ) {
		sd->skillitem = sd->skillitemlv = -1;
		return 1;
	}

	if( sd->opt1>0) {
		clif_skill_fail(sd,sd->skillid,0,0);
		sd->skillitem = sd->skillitemlv = -1;
		return 0;
	}
	if(pc_is90overweight(sd)) {
		clif_skill_fail(sd,sd->skillid,9,0);
		sd->skillitem = sd->skillitemlv = -1;
		return 0;
	}

	if(sd->skillid == AC_MAKINGARROW &&	sd->state.make_arrow_flag == 1) {
		sd->skillitem = sd->skillitemlv = -1;
		return 0;
	}
	if((sd->skillid == AM_PHARMACY || sd->skillid == ASC_CDP)
					&& sd->state.produce_flag == 1) {
		sd->skillitem = sd->skillitemlv = -1;
		return 0;
	}

	if(sd->skillitem == sd->skillid) {	/* �A�C�e���̏ꍇ���������� */
		if(type&1)
			sd->skillitem = sd->skillitemlv = -1;
		return 1;
	}
	if( sd->opt1>0 ){
		clif_skill_fail(sd,sd->skillid,0,0);
		return 0;
	}
	if(sd->sc_data){
		if( sd->sc_data[SC_DIVINA].timer!=-1 ||
			sd->sc_data[SC_ROKISWEIL].timer!=-1 ||
			(sd->sc_data[SC_AUTOCOUNTER].timer != -1 && sd->skillid != KN_AUTOCOUNTER) ||
			sd->sc_data[SC_STEELBODY].timer != -1 ||
			sd->sc_data[SC_BERSERK].timer != -1 
		){
			clif_skill_fail(sd,sd->skillid,0,0);
			return 0;	/* ��Ԉُ�Ⓘ�قȂ� */
		}
		
		//�삯�����ɃX�L�����g�����ꍇ�I��
		if(sd->skillid!=TK_RUN && sd->sc_data[SC_RUN].timer!=-1)	
		{
			if(pc_checkskill(sd,TK_RUN)>=7 && sd->weapontype1 == 0 && sd->weapontype2 == 0)
				status_change_start(&sd->bl,SC_RUN_STR,10,0,0,0,150000,0);
			status_change_end(&sd->bl,SC_RUN,-1);
		}
	}
	skill = sd->skillid;
	skilldb_id = skill_get_skilldb_id(skill);
	lv = sd->skilllv;
	
	hp=skill_get_hp(skill, lv);	/* ����HP */
	sp=skill_get_sp(skill, lv);	/* ����SP */
	if((sd->skillid_old == BD_ENCORE) && skill==sd->skillid_dance)
		sp=sp/2;	//�A���R�[������SP�������
	hp_rate = (lv <= 0)? 0:skill_db[skilldb_id].hp_rate[lv-1];
	sp_rate = (lv <= 0)? 0:skill_db[skilldb_id].sp_rate[lv-1];
	zeny = skill_get_zeny(skill,lv);
	weapon = skill_db[skilldb_id].weapon;
	state = skill_db[skilldb_id].state;
	spiritball = (lv <= 0)? 0:skill_db[skilldb_id].spiritball[lv-1];
	for(i=0;i<10;i++) {
		itemid[i] = skill_db[skilldb_id].itemid[i];
		amount[i] = skill_db[skilldb_id].amount[i];
	}
	if(hp_rate > 0)
		hp += (sd->status.hp * hp_rate)/100;
	else
		hp += (sd->status.max_hp * abs(hp_rate))/100;
	if(sp_rate > 0)
		sp += (sd->status.sp * sp_rate)/100;
	else
		sp += (sd->status.max_sp * abs(sp_rate))/100;
	if((mana = pc_checkskill(sd,HP_MANARECHARGE)) > 0) 
		sp = sp-(sp*mana/25);	//�}�i���`���[�W�Ŏg�pSP����
	if(sd->dsprate!=100)
		sp=sp*sd->dsprate/100;	/* ����SP�C�� */

	//�z�[���[���C�g�̏���ʑ���(�v�[���X�g�̍���)
	if(skill == AL_HOLYLIGHT && sd->sc_data[SC_PRIEST].timer!=-1)
	{
		sp = sp * 5;
	}
	
	//�삯���X�g�b�v
	if(sd->skillid==TK_RUN && sd->sc_data[SC_RUN].timer!=-1)
	{
		sp = 0;
	}
	
	switch(skill) {
	case SA_CASTCANCEL:
		if(sd->skilltimer == -1) {
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case BS_MAXIMIZE:		/* �}�L�V�}�C�Y�p���[ */
	case NV_TRICKDEAD:		/* ���񂾂ӂ� */
	case TF_HIDING:			/* �n�C�f�B���O */
	case AS_CLOAKING:		/* �N���[�L���O */
	case CR_AUTOGUARD:				/* �I�[�g�K�[�h */
	case CR_DEFENDER:				/* �f�B�t�F���_�[ */
	case ST_CHASEWALK:		/*�`�F�C�X�E�H�[�N*/
		if(sd->sc_data[SkillStatusChangeTable[skill]].timer!=-1)
			return 1;			/* ��������ꍇ��SP����Ȃ� */
		break;
	case AL_TELEPORT:
		if(map[sd->bl.m].flag.noteleport) {
			clif_skill_teleportmessage(sd,0);
			return 0;
		}
		break;
	case AL_WARP:
		if(map[sd->bl.m].flag.noportal) {
			clif_skill_teleportmessage(sd,0);
			return 0;
		}
		break;
	case MO_CALLSPIRITS:	/* �C�� */
		if(sd->spiritball >= lv) {
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case MO_BODYRELOCATION:	//�c�e
		if(sd->sc_data[SC_EXPLOSIONSPIRITS].timer != -1)
			spiritball = 0;
		break;
	case CH_SOULCOLLECT:	/* ���C�� */
		if(battle_config.soulcollect_max_fail)
		if(sd->spiritball >= 5) {
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case MO_FINGEROFFENSIVE:				//�w�e
		if (sd->spiritball > 0 && sd->spiritball < spiritball) {
			spiritball = sd->spiritball;
			sd->spiritball_old = sd->spiritball;
		}
		else sd->spiritball_old = lv;
		break;
	case MO_CHAINCOMBO:						//�A�ŏ�
		if(sd->sc_data[SC_BLADESTOP].timer==-1){
			if(sd->sc_data[SC_COMBO].timer == -1 || sd->sc_data[SC_COMBO].val1 != MO_TRIPLEATTACK)
				return 0;
		}
		break;
	case MO_COMBOFINISH:					//�җ���
		if(sd->sc_data[SC_COMBO].timer == -1 || sd->sc_data[SC_COMBO].val1 != MO_CHAINCOMBO)
			return 0;
		break;
	case CH_TIGERFIST:						//���Ռ�
		if(sd->sc_data[SC_COMBO].timer == -1 || sd->sc_data[SC_COMBO].val1 != MO_COMBOFINISH)
			return 0;
		break;
	case CH_CHAINCRUSH:						//�A������
		if(sd->sc_data[SC_COMBO].timer == -1)
			return 0;
		if(sd->sc_data[SC_COMBO].val1 != MO_COMBOFINISH && sd->sc_data[SC_COMBO].val1 != CH_TIGERFIST)
			return 0;
		break;
	case MO_EXTREMITYFIST:					// ���C���e�P��
		if((sd->sc_data[SC_COMBO].timer != -1 && (sd->sc_data[SC_COMBO].val1 == MO_COMBOFINISH || sd->sc_data[SC_COMBO].val1 == CH_CHAINCRUSH)) || sd->sc_data[SC_BLADESTOP].timer!=-1)
			spiritball--;
		break;
		
	case TK_STORMKICK://�����R��
		if(sd->sc_data[SC_TKCOMBO].timer == -1 || sd->sc_data[SC_TKCOMBO].val1 != TK_STORMKICK)
			return 0;
		break;
	case TK_DOWNKICK://TK_DOWNKICK
		if(sd->sc_data[SC_TKCOMBO].timer == -1 || sd->sc_data[SC_TKCOMBO].val1 != TK_DOWNKICK)
			return 0;
		break;
	case TK_TURNKICK://TK_TURNKICK
		if(sd->sc_data[SC_TKCOMBO].timer == -1 || sd->sc_data[SC_TKCOMBO].val1 != TK_TURNKICK)
			return 0;
		break;
	case TK_COUNTER://TK_COUNTER
		if(sd->sc_data[SC_TKCOMBO].timer == -1 || sd->sc_data[SC_TKCOMBO].val1 != TK_COUNTER)
			return 0;
		break;
	case BD_ADAPTATION:				/* �A�h���u */
		{
			struct skill_unit_group *group=NULL;
			if(sd->sc_data[SC_DANCING].timer==-1 || ((group=(struct skill_unit_group*)sd->sc_data[SC_DANCING].val2) && (skill_get_time(sd->sc_data[SC_DANCING].val1,group->skill_lv) - sd->sc_data[SC_DANCING].val3*1000) <= skill_get_time2(skill,lv))){ //�_���X���Ŏg�p��5�b�ȏ�̂݁H
				clif_skill_fail(sd,skill,0,0);
				return 0;
			}
		}
		break;
	case PR_BENEDICTIO:				/* ���̍~�� */
		{
			int range=1;
			int c=0;
			if(!(type&1)){
				map_foreachinarea(skill_check_condition_char_sub,sd->bl.m,
					sd->bl.x-range,sd->bl.y-range,
					sd->bl.x+range,sd->bl.y+range,BL_PC,&sd->bl,&c);
				if(c<2){
					clif_skill_fail(sd,skill,0,0);
					return 0;
				}
			}else{
				map_foreachinarea(skill_check_condition_use_sub,sd->bl.m,
					sd->bl.x-range,sd->bl.y-range,
					sd->bl.x+range,sd->bl.y+range,BL_PC,&sd->bl,&c);
			}
		}
		break;
	case WE_CALLPARTNER:		/* ���Ȃ��Ɉ������� */
		if(!sd->status.partner_id){
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case AM_CANNIBALIZE:		/* �o�C�I�v�����g */
	case AM_SPHEREMINE:			/* �X�t�B�A�[�}�C�� */
		if(type&1){
			int c,n=0;
			int summons[5] = { 1589, 1579, 1575, 1555, 1590 };
			int maxcount = (skill==AM_CANNIBALIZE)? 6-lv : skill_get_maxcount(skill);

			if(battle_config.pc_land_skill_limit && maxcount>0) {
				do{
					c=0;
					map_foreachinarea(skill_check_condition_mob_master_sub ,sd->bl.m, 0, 0, map[sd->bl.m].xs, map[sd->bl.m].ys, BL_MOB, sd->bl.id, (skill==AM_CANNIBALIZE)? summons[n] :1142, &c );
					// ���񏢊�����mob�Ƃ͕ʂ̎�ނ�mob���������Ă��Ȃ������`�F�b�N
					if((skill==AM_CANNIBALIZE && ((c > 0 && n != lv-1) || (n == lv-1 && c >= maxcount)))
						|| (skill==AM_SPHEREMINE && c >= maxcount)){
						clif_skill_fail(sd,skill,0,0);
						return 0;
					}
				}while(skill != AM_SPHEREMINE && ++n < 5);
			}
		}
		break;
	case WZ_FIREPILLAR: // celest
		if (lv <= 5)	// no gems required at level 1-5
			itemid[0] = 0;
	case MG_FIREWALL:		/* �t�@�C�A�[�E�H�[�� */
		/* ������ */
		if(battle_config.pc_land_skill_limit) {
			int maxcount = skill_get_maxcount(skill);
			if(maxcount > 0) {
				int i,c;
				for(i=c=0;i<MAX_SKILLUNITGROUP;i++) {
					if(sd->skillunit[i].alive_count > 0 && sd->skillunit[i].skill_id == skill)
						c++;
				}
				if(c >= maxcount) {
					clif_skill_fail(sd,skill,0,0);
					return 0;
				}
			}
		}
		break;
	case WS_CARTTERMINATION:				/* �J�[�g�^�[�~�l�[�V���� */
		{
			if(sd->sc_data[SC_CARTBOOST].timer==-1){ //�J�[�g�u�[�X�g���̂�
				clif_skill_fail(sd,skill,0,0);
				return 0;
			}
		}
		break;
	case TK_HIGHJUMP:
	
		break;
	case SG_SUN_WARM:
		if(sd && sd->bl.m != sd->feel_map[0].m){
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case SG_SUN_COMFORT:
		if(sd)
		{
			if(sd->bl.m == sd->feel_map[0].m && (battle_config.allow_skill_without_day || is_day_of_sun()))
				break;
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case SG_MOON_WARM:
		if(sd && sd->bl.m != sd->feel_map[1].m){
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case SG_MOON_COMFORT:
		if(sd)
		{
			if(sd->bl.m == sd->feel_map[1].m && (battle_config.allow_skill_without_day || is_day_of_moon()))
				break;
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case SG_STAR_WARM:
		if(sd && sd->bl.m != sd->feel_map[2].m){
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case SG_STAR_COMFORT:
		if(sd)
		{
			if(sd->bl.m == sd->feel_map[2].m && (battle_config.allow_skill_without_day || is_day_of_star()))
				break;
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case AM_BERSERKPITCHER:
		if(sd && sd->sc_data[SC_ALCHEMIST].timer==-1){//�A���P�~�X�g�̍����
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case BS_ADRENALINE2:
		if(sd && sd->sc_data[SC_BLACKSMITH].timer==-1){//�u���b�N�X�~�X�̍����
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
		
	case SG_FUSION:
		if(sd && sd->sc_data[SC_STAR].timer==-1){//�P���Z�C�̍����
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case KN_ONEHAND:
		if(sd && sd->sc_data[SC_KNIGHT].timer==-1){//�i�C�g�̍����
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	}
	
	//�M���h�X�L��
	switch(skill)
	{
		case GD_BATTLEORDER://#�Ր�Ԑ�#
		case GD_REGENERATION://#����#
		case GD_RESTORE://##����
		case GD_EMERGENCYCALL://#�ً}���W#
			if(!battle_config.guild_skill_available){
				clif_skill_fail(sd,skill,0,0);
				return 0;
			}
			if(battle_config.allow_guild_skill_in_gvg_only && !map[sd->bl.m].flag.gvg){
				clif_skill_fail(sd,skill,0,0);
				return 0;
			}
			if(battle_config.guild_skill_in_pvp_limit && map[sd->bl.m].flag.pvp){
				clif_skill_fail(sd,skill,0,0);
				return 0;
			}
			if(sd->sc_data[SC_BATTLEORDER_DELAY + skill - GD_BATTLEORDER].timer != -1){
				clif_skill_fail(sd,skill,0,0);
				return 0;
			}
		break;
		default:
			break;
	}

	if(!(type&2)){
		if( hp>0 && sd->status.hp < hp) {				/* HP�`�F�b�N */
			clif_skill_fail(sd,skill,2,0);		/* HP�s���F���s�ʒm */
			return 0;
		}
		if( sp>0 && sd->status.sp < sp) {				/* SP�`�F�b�N */
			clif_skill_fail(sd,skill,1,0);		/* SP�s���F���s�ʒm */
			return 0;
		}
		if( zeny>0 && sd->status.zeny < zeny) {
			clif_skill_fail(sd,skill,5,0);
			return 0;
		}
		if(!(weapon & (1<<sd->status.weapon) ) ) {
			clif_skill_fail(sd,skill,6,0);
			return 0;
		}
		if( spiritball > 0 && sd->spiritball < spiritball) {
			clif_skill_fail(sd,skill,0,0);		// �����s��
			return 0;
		}
	}

	switch(state) {
	case ST_HIDING:
		if(!(sd->status.option&2)) {
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
		/*
	case ST_CLOAKING:
		if(!(sd->status.option&4)) {
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case ST_CHASEWALK:
		if(!(sd->status.option&0x4000)) {
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
		*/
	case ST_CLOAKING:
		if(!pc_iscloaking(sd)) {
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case ST_CHASEWALK:
		if(!pc_ischasewalk(sd)) {
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case ST_HIDDEN:
		if(!pc_ishiding(sd)) {
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case ST_RIDING:
		if(!pc_isriding(sd)) {
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case ST_FALCON:
		if(!pc_isfalcon(sd)) {
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case ST_CART:
		if(!pc_iscarton(sd)) {
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case ST_SHIELD:
		if(sd->status.shield <= 0) {
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case ST_SIGHT:
		if(sd->sc_data[SC_SIGHT].timer == -1 && type&1) {
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case ST_EXPLOSIONSPIRITS:
		if(sd->sc_data[SC_EXPLOSIONSPIRITS].timer == -1) {
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case ST_RECOV_WEIGHT_RATE:
		if(battle_config.natural_heal_weight_rate <= 100 && sd->weight*100/sd->max_weight >= battle_config.natural_heal_weight_rate) {
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case ST_MOVE_ENABLE:
		{
			struct walkpath_data wpd;
			if(path_search(&wpd,sd->bl.m,sd->bl.x,sd->bl.y,sd->skillx,sd->skilly,1)==-1) {
				clif_skill_fail(sd,skill,0,0);
				return 0;
			}
		}
		break;
	case ST_WATER:
		if(!map[sd->bl.m].flag.rain)
		{
			if((!map_getcell(sd->bl.m,sd->bl.x,sd->bl.y,CELL_CHKWATER))&& (sd->sc_data[SC_DELUGE].timer==-1)){	//���ꔻ��
				clif_skill_fail(sd,skill,0,0);
				return 0;
			}
		}
		break;
	}
	//GVG PVP�ȊO�̃}�b�v�ł̓��ꏈ��
	if(map[sd->bl.m].flag.pvp==0 && map[sd->bl.m].flag.gvg==0)
	{
		switch(skill)
		{
			case AM_DEMONSTRATION:
				if(battle_config.demonstration_nocost)
					goto ITEM_NOCOST;
				break;
			case AM_ACIDTERROR:
				if(battle_config.acidterror_nocost)
					goto ITEM_NOCOST;
				break;
			case AM_CANNIBALIZE:
				if(battle_config.cannibalize_nocost)
					goto ITEM_NOCOST;
				break;
			case AM_SPHEREMINE:
				if(battle_config.spheremine_nocost)
					goto ITEM_NOCOST;
				break;
			case	AM_CP_WEAPON:
			case	AM_CP_SHIELD:
			case	AM_CP_ARMOR:
			case	AM_CP_HELM:
			case	CR_FULLPROTECTION:
				if(battle_config.chemical_nocost)
					goto ITEM_NOCOST;
				break;
			case	CR_ACIDDEMONSTRATION:
				if(battle_config.aciddemonstration_nocost)
					goto ITEM_NOCOST;
				break;
			case CR_SLIMPITCHER:
				if(battle_config.slimpitcher_nocost)
				{
					//�ԃ|
					for(i=0;i<5;i++)
					{
						
						itemid[i] = 501;
						amount[i] = 1;
					}
					//
					for(;i<9;i++)
					{
						
						itemid[i] = 503;
						amount[i] = 1;
					}
					//���|
					itemid[i] = 504;
					amount[i] = 1;
				}
			default:
				break;
		}
	}



	for(i=0;i<10;i++) {
		int x = lv%11 - 1;
		index[i] = -1;
		if(itemid[i] <= 0)
			continue;
			
		//�E�B�U�[�h�̍�
		if(itemid[i] >= 715 && itemid[i] <= 717 && (sd->special_state.no_gemstone || sd->sc_data[SC_WIZARD].timer!=-1) )
			continue;

		if(((itemid[i] >= 715 && itemid[i] <= 717) || itemid[i] == 1065) && sd->sc_data[SC_INTOABYSS].timer != -1)
			continue;

		if((skill == AM_POTIONPITCHER ||
			skill == CR_SLIMPITCHER)&& i != x)
			continue;

		index[i] = pc_search_inventory(sd,itemid[i]);
		if(index[i] < 0 || sd->status.inventory[index[i]].amount < amount[i]) {
			if(itemid[i] == 716 || itemid[i] == 717)
				clif_skill_fail(sd,skill,(7+(itemid[i]-716)),0);
			else
				clif_skill_fail(sd,skill,0,0);
			return 0;
		}
	}

	if(!(type&1))
		return 1;

	if((skill != AM_POTIONPITCHER) && (skill != CR_SLIMPITCHER) && (skill != MG_STONECURSE)) {
		if(skill == AL_WARP && !(type&2))
			return 1;
		for(i=0;i<10;i++) {
			if(index[i] >= 0)
				pc_delitem(sd,index[i],amount[i],0);		// �A�C�e������
		}
	}

ITEM_NOCOST:

	if(!(type&1))
		return 1;

	if(type&2)
		return 1;

	if(sp > 0) {					// SP����
		sd->status.sp-=sp;
		clif_updatestatus(sd,SP_SP);
	}
	if(hp > 0) {					// HP����
		sd->status.hp-=hp;
		clif_updatestatus(sd,SP_HP);
	}
	if(zeny > 0)					// Zeny����
		pc_payzeny(sd,zeny);
	if(spiritball > 0)				// ��������
		pc_delspiritball(sd,spiritball,0);


	return 1;
}

/*==========================================
 * �r�����Ԍv�Z
 *------------------------------------------
 */
int skill_castfix( struct block_list *bl, int time )
{
	struct status_change *sc_data;
	int dex;
	int castrate=100;
	
	nullpo_retr(0, bl);

	sc_data = status_get_sc_data(bl);
	dex=status_get_dex(bl);

	// ���@�͑����̌��ʏI��
	if(sc_data && sc_data[SC_MAGICPOWER].timer != -1) {
		if (sc_data[SC_MAGICPOWER].val2 > 0) {
			/* �ŏ��ɒʂ������ɂ̓A�C�R���������� */
			sc_data[SC_MAGICPOWER].val2--;
			clif_status_change(bl, SC_MAGICPOWER, 0);
		} else {
			status_change_end( bl, SC_MAGICPOWER, -1);
		}
	}

	/* �T�t���M�E�� */
	if (sc_data && sc_data[SC_SUFFRAGIUM].timer != -1){
		time = time * (100 - sc_data[SC_SUFFRAGIUM].val1 * 15) / 100;
		status_change_end(bl, SC_SUFFRAGIUM, -1);
	}

	if(time==0)
		return 0;
	if(bl->type==BL_PC) {
		castrate=((struct map_session_data *)bl)->castrate;
		/* ���r��DEX */
		if(battle_config.no_spel_dex1 && battle_config.no_spel_dex2) time=time*castrate*(battle_config.no_spel_dex1- dex)/battle_config.no_spel_dex2;
		else if(battle_config.no_spel_dex1) time=time*castrate*(battle_config.no_spel_dex1- dex)/15000;
		else if(battle_config.no_spel_dex2) time=time*castrate*(150- dex)/battle_config.no_spel_dex2;
		else time=time*castrate*(150- dex)/15000;
		time=time*battle_config.cast_rate/100;
	}

	/* �u���M�̎� */
	if(sc_data && sc_data[SC_POEMBRAGI].timer!=-1 )
	{
		time=time*(100-(sc_data[SC_POEMBRAGI].val1*3+sc_data[SC_POEMBRAGI].val2
				+(sc_data[SC_POEMBRAGI].val3>>16)))/100;
	}else if(sc_data && sc_data[SC_POEMBRAGI_].timer!=-1 )
	{
		time=time*(100-(sc_data[SC_POEMBRAGI_].val1*3+sc_data[SC_POEMBRAGI_].val2
				+(sc_data[SC_POEMBRAGI_].val3>>16)))/100;
	}

	return (time>0)?time:0;
}
/*==========================================
 * �f�B���C�v�Z
 *------------------------------------------
 */
int skill_delayfix( struct block_list *bl, int time, int cast )
{
	struct status_change *sc_data;
	
	nullpo_retr(0, bl);

	sc_data = status_get_sc_data(bl);
	if(time<=0 && cast<=0)
		return ( status_get_adelay(bl) / 2 );

	if(bl->type == BL_PC) {
		if( battle_config.delay_dependon_dex )	/* dex�̉e�����v�Z���� */
			time=time*(150- status_get_dex(bl))/150;
		time=time*battle_config.delay_rate/100;
	}

	/* �u���M�̎� */
	if(sc_data && sc_data[SC_POEMBRAGI].timer!=-1 ){
		time=time*(100-(sc_data[SC_POEMBRAGI].val1*3+sc_data[SC_POEMBRAGI].val2
				+(sc_data[SC_POEMBRAGI].val3&0xffff)))/100;
	}else if(sc_data && sc_data[SC_POEMBRAGI_].timer!=-1 ){
		time=time*(100-(sc_data[SC_POEMBRAGI_].val1*3+sc_data[SC_POEMBRAGI_].val2
				+(sc_data[SC_POEMBRAGI_].val3&0xffff)))/100;
	}

	return (time>0)?time:0;
}

/*==========================================
 * �X�L���g�p�iID�w��j
 *------------------------------------------
 */
int skill_use_id( struct map_session_data *sd, int target_id,
	int skill_num, int skill_lv)
{
	unsigned int tick;
	int casttime=0,delay=0,skill,skilldb_id,range;
	struct map_session_data* target_sd=NULL;
	int forcecast=0;
	struct block_list *bl;
	struct status_change *sc_data;
	tick=gettick();

	nullpo_retr(0, sd);
	skilldb_id = skill_get_skilldb_id(skill_num);
	
	if( (bl=map_id2bl(target_id)) == NULL ){
/*		if(battle_config.error_log)
			printf("skill target not found %d\n",target_id); */
		return 0;
	}
	if(sd->bl.m != bl->m || pc_isdead(sd))
		return 0;

	sc_data=sd->sc_data;
	
	/* ���ق�ُ�i�������A�O�����Ȃǂ̔��������j */
	if( sd->opt1>0 )
		return 0;
	if(sd->sc_data){
		if(
			sd->sc_data[SC_DIVINA].timer!=-1 ||
			sd->sc_data[SC_ROKISWEIL].timer!=-1 ||
			(sd->sc_data[SC_AUTOCOUNTER].timer != -1 && sd->skillid != KN_AUTOCOUNTER) ||
			sd->sc_data[SC_STEELBODY].timer != -1 ||
			sd->sc_data[SC_BERSERK].timer != -1 ){
			return 0;	/* ��Ԉُ�Ⓘ�قȂ� */
		}

		if(sc_data[SC_BLADESTOP].timer != -1){
			int lv = sc_data[SC_BLADESTOP].val1;
			if(sc_data[SC_BLADESTOP].val2==1) return 0;//���H���ꂽ���Ȃ̂Ń_��
			if(lv==1) return 0;
			if(lv==2 && skill_num!=MO_FINGEROFFENSIVE) return 0;
			if(lv==3 && skill_num!=MO_FINGEROFFENSIVE && skill_num!=MO_INVESTIGATE) return 0;
			if(lv==4 && skill_num!=MO_FINGEROFFENSIVE && skill_num!=MO_INVESTIGATE && skill_num!=MO_CHAINCOMBO) return 0;
			if(lv==5 && skill_num!=MO_FINGEROFFENSIVE && skill_num!=MO_INVESTIGATE && skill_num!=MO_CHAINCOMBO && skill_num!=MO_EXTREMITYFIST) return 0;
		}
	}

	//�`�F�C�X�A�n�C�h�A�N���[�L���O���̃X�L��
	//if(pc_iscloaking(sd) && skill_num==TF_HIDING)//�N���[�L���O
	//	return 0;
	if(sd->status.option&2 && skill_num!=TF_HIDING && skill_num!=AS_GRIMTOOTH && skill_num!=RG_BACKSTAP && skill_num!=RG_RAID )
		return 0;
	if(pc_ischasewalk(sd) && skill_num != ST_CHASEWALK)//�`�F�C�X�E�H�[�N
	 	return 0;

	//GVG�M���h�X�L��
	if(skill_num>=GD_SKILLBASE)
	{
		switch(skill_num){
		case GD_BATTLEORDER://#�Ր�Ԑ�#
		case GD_REGENERATION://#����#
		case GD_RESTORE://##����
		case GD_EMERGENCYCALL://#�ً}���W#
			if(!battle_config.guild_skill_available)
				return 0;
			if(battle_config.allow_guild_skill_in_gvg_only && !map[sd->bl.m].flag.gvg)
				return 0;
			if(battle_config.guild_skill_in_pvp_limit && map[sd->bl.m].flag.pvp)
				return 0;
			if(sd->sc_data[SC_BATTLEORDER_DELAY + skill_num - GD_BATTLEORDER].timer != -1)
				return 0;
		break;
		default:
			break;
		}
	}

	
	if(map[sd->bl.m].flag.gvg){ //GvG�Ŏg�p�ł��Ȃ��X�L��
		switch(skill_num){
		case AL_TELEPORT:
		case AL_WARP:
		case WZ_ICEWALL:
		case TF_BACKSLIDING:
		case HP_BASILICA:
			return 0;
			break;
		case HP_ASSUMPTIO:
			if(!battle_config.allow_assumptop_in_gvg)
				return 0;
			break;
		}
		
	}

	/* ���t/�_���X�� */
	if( sc_data && sc_data[SC_DANCING].timer!=-1 ){
//		if(battle_config.pc_skill_log)
//			printf("dancing! %d\n",skill_num);
		if( sc_data[SC_DANCING].val4 && skill_num!=BD_ADAPTATION ) //���t���̓A�h���u�ȊO�s��
			return 0;
		if(skill_num!=BD_ADAPTATION && skill_num!=BA_MUSICALSTRIKE && skill_num!=DC_THROWARROW){
			return 0;
		}
	}

	if(skill_get_inf2(skill_num)&0x200 && sd->bl.id == target_id)
		return 0;
	//���O�̃X�L���������o����K�v�̂���X�L��
	switch(skill_num){
	case SA_CASTCANCEL:
		if(sd->skillid != skill_num){ //�L���X�g�L�����Z�����̂͊o���Ȃ�
			sd->skillid_old = sd->skillid;
			sd->skilllv_old = sd->skilllv;
			break;
		}
	case BD_ENCORE:					/* �A���R�[�� */
		if(!sd->skillid_dance){ //�O��g�p�����x�肪�Ȃ��Ƃ���
			clif_skill_fail(sd,skill_num,0,0);
			return 0;
		}else{
			sd->skillid_old = skill_num;
		}
		break;
	}
	
	//���O�̏�ԃ`�F�b�N���K�v
	switch(skill_num)
	{
		case SL_SMA://�G�X�}
			if(sd && sd->sc_data[SC_SMA].timer==-1){//�G�X�}�r���\���
				clif_skill_fail(sd,skill_num,0,0);
				return 0;
			}
			break;
	}

	sd->skillid = skill_num;
	sd->skilllv = skill_lv;

	switch(skill_num){ //���O�Ƀ��x�����ς�����肷��X�L��
	case BD_LULLABY:				/* �q��� */
	case BD_RICHMANKIM:				/* �j�����h�̉� */
	case BD_ETERNALCHAOS:			/* �i���̍��� */
	case BD_DRUMBATTLEFIELD:		/* �푾�ۂ̋��� */
	case BD_RINGNIBELUNGEN:			/* �j�[�x�����O�̎w�� */
	case BD_ROKISWEIL:				/* ���L�̋��� */
	case BD_INTOABYSS:				/* �[���̒��� */
	case BD_SIEGFRIED:				/* �s���g�̃W�[�N�t���[�h */
	case BD_RAGNAROK:				/* �_�X�̉��� */
	case CG_MOONLIT:				/* ������̐�ɗ�����Ԃт� */
		{
			int range=1;
			int c=0;
			map_foreachinarea(skill_check_condition_char_sub,sd->bl.m,
				sd->bl.x-range,sd->bl.y-range,
				sd->bl.x+range,sd->bl.y+range,BL_PC,&sd->bl,&c);
			if(c<1){
				clif_skill_fail(sd,skill_num,0,0);
				return 0;
			}else if(c==99){ //�����s�v�ݒ肾����
				;
			}else{
				sd->skilllv=(c + skill_lv)/2;
			}
		}
		break;
	}

	if(!skill_check_condition(sd,0)) return 0;

	/* �˒��Ə�Q���`�F�b�N */
	range = skill_get_range(skill_num,skill_lv);
	if(range < 0)
		range = status_get_range(&sd->bl) - (range + 1);
	if (!battle_check_range(&sd->bl,bl,range + 1))
		return 0;

	if(bl->type==BL_PC) {
		target_sd=(struct map_session_data*)bl;
		if(target_sd && skill_num == ALL_RESURRECTION && !pc_isdead(target_sd))
			return 0;
	}
	if((skill_num != MO_CHAINCOMBO &&
	    skill_num != MO_COMBOFINISH &&
	    skill_num != MO_EXTREMITYFIST &&
	    skill_num != CH_TIGERFIST &&
	    skill_num != CH_CHAINCRUSH &&
	    skill_num != TK_STORMKICK &&
	    skill_num != TK_DOWNKICK &&
	    skill_num != TK_TURNKICK &&
	    skill_num != TK_COUNTER) ||
		(skill_num == MO_EXTREMITYFIST && sd->state.skill_flag) )
		pc_stopattack(sd);

	casttime=skill_castfix(&sd->bl, skill_get_cast( skill_num,skill_lv) ) + skill_get_fixedcast(skill_num,skill_lv);
	if(skill_num != SA_MAGICROD)
		delay=skill_delayfix(&sd->bl, skill_get_delay( skill_num,skill_lv), skill_get_cast( skill_num,skill_lv) );
	sd->state.skillcastcancel = skill_db[skilldb_id].castcancel;

	switch(skill_num){	/* ��������ȏ������K�v */
//	case AL_HEAL:	/* �q�[�� */
//		if(battle_check_undead(status_get_race(bl),status_get_elem_type(bl)))
//			forcecast=1;	/* �q�[���A�^�b�N�Ȃ�r���G�t�F�N�g�L�� */
//		break;
	case ALL_RESURRECTION:	/* ���U���N�V���� */
		if(bl->type != BL_PC && battle_check_undead(status_get_race(bl),status_get_elem_type(bl))){	/* �G���A���f�b�h�Ȃ� */
			forcecast=1;	/* �^�[���A���f�b�g�Ɠ����r������ */
			casttime=skill_castfix(&sd->bl, skill_get_cast(PR_TURNUNDEAD,skill_lv) ) + skill_get_fixedcast(PR_TURNUNDEAD,skill_lv);
		}
		break;
	case MO_FINGEROFFENSIVE:	/* �w�e */
		casttime += casttime * ((skill_lv > sd->spiritball)? sd->spiritball:skill_lv);
		break;
	case MO_CHAINCOMBO:		/*�A�ŏ�*/
		target_id = sd->attacktarget;
		if( sc_data && sc_data[SC_BLADESTOP].timer!=-1 ){
			struct block_list *tbl;
			if((tbl=(struct block_list *)sc_data[SC_BLADESTOP].val4) == NULL) //�^�[�Q�b�g�����Ȃ��H
				return 0;
			target_id = tbl->id;
		}
		break;
	case TK_STORMKICK://�����R��
	case TK_DOWNKICK://���i�R��
	case TK_TURNKICK://��]�R��
		target_id = sd->attacktarget;
		break;
	case TK_COUNTER://�J�E���^�[�R��
		if(sc_data)	target_id = sc_data[SC_TKCOMBO].val3;
		else return 0;
		break;
	case MO_COMBOFINISH:	/*�җ���*/
	case CH_TIGERFIST:		/* ���Ռ� */
	case CH_CHAINCRUSH:		/* �A������ */
		target_id = sd->attacktarget;
		break;
	case MO_EXTREMITYFIST:	/*���C���e�P��*/
		if(sc_data && sc_data[SC_COMBO].timer != -1 && (sc_data[SC_COMBO].val1 == MO_COMBOFINISH || sc_data[SC_COMBO].val1 == CH_CHAINCRUSH)) {
			casttime = 0;
			target_id = sd->attacktarget;
		}
		forcecast=1;
		break;
	case SA_MAGICROD:
	case SA_SPELLBREAKER:
		forcecast=1;
		break;
	case WE_MALE:
	case WE_FEMALE:
		{
		struct map_session_data *p_sd = NULL;
		if((p_sd = pc_get_partner(sd)) == NULL)
			return 0;
		target_id = p_sd->bl.id;
		//range������1�񌟍�
		range = skill_get_range(skill_num,skill_lv);
		if(range < 0)
			range = status_get_range(&sd->bl) - (range + 1);
		if(!battle_check_range(&sd->bl,&p_sd->bl,range) ){
			return 0;
			}
		}
		break;
	case AS_SPLASHER:				/* �x�i���X�v���b�V���[ */
		/* �ŏ�Ԃɂ���K�v�͂Ȃ� */
		break;
	case SA_ABRACADABRA:
		{
			delay=skill_get_delay(SA_ABRACADABRA,skill_lv);
		}
		break;
	case HP_BASILICA:		/* �o�W���J */
		if (skill_check_unit_range(sd->bl.m,sd->bl.x,sd->bl.y,sd->skillid,sd->skilllv)) {
			clif_skill_fail(sd,sd->skillid,0,0);
			return 0;
		}
		if (skill_check_unit_range2(sd->bl.m,sd->bl.x,sd->bl.y,sd->skillid,sd->skilllv)) {
			clif_skill_fail(sd,sd->skillid,0,0);
			return 0;
		}
		break;

	}

	//�������C�Y��ԂȂ�L���X�g�^�C����1/2
	if(sc_data && sc_data[SC_MEMORIZE].timer != -1 && casttime > 0){
		casttime = casttime/2;
		if((--sc_data[SC_MEMORIZE].val2)<=0)
			status_change_end(&sd->bl, SC_MEMORIZE, -1);
	}

	if(battle_config.pc_skill_log)
		printf("PC %d skill use target_id=%d skill=%d lv=%d cast=%d\n",sd->bl.id,target_id,skill_num,skill_lv,casttime);

//	if(sd->skillitem == skill_num)
//		casttime = delay = 0;

	if( casttime>0 || forcecast ){ /* �r�����K�v */
		struct mob_data *md;
		clif_skillcasting( &sd->bl,
			sd->bl.id, target_id, 0,0, skill_num,casttime);

		/* �r�����������X�^�[ */
		if( bl->type==BL_MOB && (md=(struct mob_data *)bl) && mob_db[md->class].mode&0x10 &&
			md->state.state!=MS_ATTACK && sd->invincible_timer == -1){
				md->target_id=sd->bl.id;
				md->state.targettype = ATTACKABLE;
				md->min_chase=13;
		}
	}

	if( casttime<=0 )	/* �r���̖������̂̓L�����Z������Ȃ� */
		sd->state.skillcastcancel=0;

	sd->skilltarget	= target_id;
/*	sd->cast_target_bl	= bl; */
	sd->skillx		= 0;
	sd->skilly		= 0;
	sd->canact_tick = tick + casttime + delay;
	sd->canmove_tick = tick;
	
	if(!(battle_config.pc_cloak_check_type&2) && sc_data && sc_data[SC_CLOAKING].timer != -1 && sd->skillid != AS_CLOAKING)
		status_change_end(&sd->bl,SC_CLOAKING,-1);
		
	if(casttime > 0) {
		sd->skilltimer = add_timer( tick+casttime, skill_castend_id, sd->bl.id, 0 );
		if((skill = pc_checkskill(sd,SA_FREECAST)) > 0) {
			sd->prev_speed = sd->speed;
			sd->speed = sd->speed*(175 - skill*5)/100;
			clif_updatestatus(sd,SP_SPEED);
		}
		else
			pc_stop_walking(sd,0);
	}
	else {
		if(skill_num != SA_CASTCANCEL)
			sd->skilltimer = -1;
		skill_castend_id(sd->skilltimer,tick,sd->bl.id,0);
	}

	return 0;
}

/*==========================================
 * �X�L���g�p�i�ꏊ�w��j
 *------------------------------------------
 */
int skill_use_pos( struct map_session_data *sd,
	int skill_x, int skill_y, int skill_num, int skill_lv)
{
	struct block_list bl;
	struct status_change *sc_data;
	unsigned int tick;
	int casttime=0,delay=0,skill,range,skilldb_id;
	nullpo_retr(0, sd);

	if(pc_isdead(sd))
		return 0;

	sc_data=sd->sc_data;
	skilldb_id = skill_get_skilldb_id(skill_num);

	if( sd->opt1>0 )
		return 0;
	if(sc_data){
		if( sc_data[SC_DIVINA].timer!=-1 ||
			sc_data[SC_ROKISWEIL].timer!=-1 ||
			sc_data[SC_AUTOCOUNTER].timer != -1 ||
			sc_data[SC_STEELBODY].timer != -1 ||
			sc_data[SC_DANCING].timer!=-1 ||
			sc_data[SC_BERSERK].timer != -1 )
			return 0;	/* ��Ԉُ�Ⓘ�قȂ� */
	}

	if(sd->status.option&2)
		return 0;

	//�`�F�C�X�E�H�[�N���Ɛݒu�n���s
	if(pc_ischasewalk(sd))
	 	return 0;

	if(map[sd->bl.m].flag.gvg && (skill_num == AL_TELEPORT || skill_num == AL_WARP || skill_num == WZ_ICEWALL ||
		skill_num == TF_BACKSLIDING))
		return 0;


	sd->skillid = skill_num;
	sd->skilllv = skill_lv;
	sd->skillx = skill_x;
	sd->skilly = skill_y;
	if(!skill_check_condition(sd,0)) return 0;

	/* �˒��Ə�Q���`�F�b�N */
	bl.type = BL_NUL;
	bl.m = sd->bl.m;
	bl.x = skill_x;
	bl.y = skill_y;
	range = skill_get_range(skill_num,skill_lv);
	if(range < 0)
		range = status_get_range(&sd->bl) - (range + 1);
	if (!battle_check_range(&sd->bl,&bl,range + 1))
		return 0;

	pc_stopattack(sd);

	casttime=skill_castfix(&sd->bl, skill_get_cast( skill_num,skill_lv) ) + skill_get_fixedcast(skill_num,skill_lv);;
	delay=skill_delayfix(&sd->bl, skill_get_delay( skill_num,skill_lv), skill_get_cast( skill_num,skill_lv) );
	sd->state.skillcastcancel = skill_db[skilldb_id].castcancel;

	if(battle_config.pc_skill_log)
		printf("PC %d skill use target_pos=(%d,%d) skill=%d lv=%d cast=%d\n",sd->bl.id,skill_x,skill_y,skill_num,skill_lv,casttime);

//	if(sd->skillitem == skill_num)
//		casttime = delay = 0;
	//�������C�Y��ԂȂ�L���X�g�^�C����1/2
	if(sc_data && sc_data[SC_MEMORIZE].timer != -1 && casttime > 0){
		casttime = casttime/2;
		if((--sc_data[SC_MEMORIZE].val2)<=0)
			status_change_end(&sd->bl, SC_MEMORIZE, -1);
	}

	if( casttime>0 )	/* �r�����K�v */
		clif_skillcasting( &sd->bl,
			sd->bl.id, 0, skill_x,skill_y, skill_num,casttime);

	if( casttime<=0 )	/* �r���̖������̂̓L�����Z������Ȃ� */
		sd->state.skillcastcancel=0;

	sd->skilltarget	= 0;
/*	sd->cast_target_bl	= NULL; */
	tick=gettick();
	sd->canact_tick = tick + casttime + delay;
	sd->canmove_tick = tick;
	
	if(!(battle_config.pc_cloak_check_type&2) && sc_data && sc_data[SC_CLOAKING].timer != -1)
		status_change_end(&sd->bl,SC_CLOAKING,-1);
	if(casttime > 0) {
		sd->skilltimer = add_timer( tick+casttime, skill_castend_pos, sd->bl.id, 0 );
		if((skill = pc_checkskill(sd,SA_FREECAST)) > 0) {
			sd->prev_speed = sd->speed;
			sd->speed = sd->speed*(175 - skill*5)/100;
			clif_updatestatus(sd,SP_SPEED);
		}
		else
			pc_stop_walking(sd,0);
	}
	else {
		sd->skilltimer = -1;
		skill_castend_pos(sd->skilltimer,tick,sd->bl.id,0);
	}

	return 0;
}

/*==========================================
 * �X�L���r���L�����Z��
 *------------------------------------------
 */
int skill_castcancel(struct block_list *bl,int type)
{
	int inf;
	int ret=0;

	nullpo_retr(0, bl);

	if(bl->type==BL_PC){
		struct map_session_data *sd=(struct map_session_data *)bl;
		unsigned long tick=gettick();
		nullpo_retr(0, sd);
		sd->canact_tick=tick;
		sd->canmove_tick = tick;
		if( sd->skilltimer!=-1){
			if(pc_checkskill(sd,SA_FREECAST) > 0) {
				sd->speed = sd->prev_speed;
				clif_updatestatus(sd,SP_SPEED);
			}
			if(!type) {
				if((inf = skill_get_inf( sd->skillid )) == 2 || inf == 32)
					ret=delete_timer( sd->skilltimer, skill_castend_pos );
				else
					ret=delete_timer( sd->skilltimer, skill_castend_id );
				if(ret<0)
					printf("delete timer error : skillid : %d\n",sd->skillid);
			}
			else {
				if((inf = skill_get_inf( sd->skillid_old )) == 2 || inf == 32)
					ret=delete_timer( sd->skilltimer, skill_castend_pos );
				else
					ret=delete_timer( sd->skilltimer, skill_castend_id );
				if(ret<0)
					printf("delete timer error : skillid : %d\n",sd->skillid_old);
			}
			sd->skilltimer=-1;
			clif_skillcastcancel(bl);
		}

		return 0;
	}else if(bl->type==BL_MOB){
		struct mob_data *md=(struct mob_data *)bl;
		nullpo_retr(0, md);
		if( md->skilltimer!=-1 ){
			if((inf = skill_get_inf( md->skillid )) == 2 || inf == 32)
				ret=delete_timer( md->skilltimer, mobskill_castend_pos );
			else
				ret=delete_timer( md->skilltimer, mobskill_castend_id );
			md->skilltimer=-1;
			clif_skillcastcancel(bl);
		}
		if(ret<0)
			printf("delete timer error : skillid : %d\n",md->skillid);
		return 0;
	}
	return 1;
}
/*=========================================
 * �u�����f�B�b�V���X�s�A �����͈͌���
 *----------------------------------------
 */
void skill_brandishspear_first(struct square *tc,int dir,int x,int y){

	nullpo_retv(tc);

	if(dir == 0){
		tc->val1[0]=x-2;
		tc->val1[1]=x-1;
		tc->val1[2]=x;
		tc->val1[3]=x+1;
		tc->val1[4]=x+2;
		tc->val2[0]=
		tc->val2[1]=
		tc->val2[2]=
		tc->val2[3]=
		tc->val2[4]=y-1;
	}
	else if(dir==2){
		tc->val1[0]=
		tc->val1[1]=
		tc->val1[2]=
		tc->val1[3]=
		tc->val1[4]=x+1;
		tc->val2[0]=y+2;
		tc->val2[1]=y+1;
		tc->val2[2]=y;
		tc->val2[3]=y-1;
		tc->val2[4]=y-2;
	}
	else if(dir==4){
		tc->val1[0]=x-2;
		tc->val1[1]=x-1;
		tc->val1[2]=x;
		tc->val1[3]=x+1;
		tc->val1[4]=x+2;
		tc->val2[0]=
		tc->val2[1]=
		tc->val2[2]=
		tc->val2[3]=
		tc->val2[4]=y+1;
	}
	else if(dir==6){
		tc->val1[0]=
		tc->val1[1]=
		tc->val1[2]=
		tc->val1[3]=
		tc->val1[4]=x-1;
		tc->val2[0]=y+2;
		tc->val2[1]=y+1;
		tc->val2[2]=y;
		tc->val2[3]=y-1;
		tc->val2[4]=y-2;
	}
	else if(dir==1){
		tc->val1[0]=x-1;
		tc->val1[1]=x;
		tc->val1[2]=x+1;
		tc->val1[3]=x+2;
		tc->val1[4]=x+3;
		tc->val2[0]=y-4;
		tc->val2[1]=y-3;
		tc->val2[2]=y-1;
		tc->val2[3]=y;
		tc->val2[4]=y+1;
	}
	else if(dir==3){
		tc->val1[0]=x+3;
		tc->val1[1]=x+2;
		tc->val1[2]=x+1;
		tc->val1[3]=x;
		tc->val1[4]=x-1;
		tc->val2[0]=y-1;
		tc->val2[1]=y;
		tc->val2[2]=y+1;
		tc->val2[3]=y+2;
		tc->val2[4]=y+3;
	}
	else if(dir==5){
		tc->val1[0]=x+1;
		tc->val1[1]=x;
		tc->val1[2]=x-1;
		tc->val1[3]=x-2;
		tc->val1[4]=x-3;
		tc->val2[0]=y+3;
		tc->val2[1]=y+2;
		tc->val2[2]=y+1;
		tc->val2[3]=y;
		tc->val2[4]=y-1;
	}
	else if(dir==7){
		tc->val1[0]=x-3;
		tc->val1[1]=x-2;
		tc->val1[2]=x-1;
		tc->val1[3]=x;
		tc->val1[4]=x+1;
		tc->val2[1]=y;
		tc->val2[0]=y+1;
		tc->val2[2]=y-1;
		tc->val2[3]=y-2;
		tc->val2[4]=y-3;
	}

}

/*=========================================
 * �u�����f�B�b�V���X�s�A �������� �͈͊g��
 *-----------------------------------------
 */
void skill_brandishspear_dir(struct square *tc,int dir,int are){

	int c;

	nullpo_retv(tc);

	for(c=0;c<5;c++){
		if(dir==0){
			tc->val2[c]+=are;
		}else if(dir==1){
			tc->val1[c]-=are; tc->val2[c]+=are;
		}else if(dir==2){
			tc->val1[c]-=are;
		}else if(dir==3){
			tc->val1[c]-=are; tc->val2[c]-=are;
		}else if(dir==4){
			tc->val2[c]-=are;
		}else if(dir==5){
			tc->val1[c]+=are; tc->val2[c]-=are;
		}else if(dir==6){
			tc->val1[c]+=are;
		}else if(dir==7){
			tc->val1[c]+=are; tc->val2[c]+=are;
		}
	}
}
/*----------------------------------------------------------------------------
 * �ʃX�L���̊֐�
 */

/*==========================================
 * �f�B�{�[�V���� �L���m�F
 *------------------------------------------
 */
void skill_devotion(struct map_session_data *md,int target)
{
	// ���m�F
	int n;

	nullpo_retv(md);

	for(n=0;n<5;n++){
		if(md->dev.val1[n]){
			struct map_session_data *sd = map_id2sd(md->dev.val1[n]);
			// ���肪������Ȃ� // ������f�B�{���Ă�̂���������Ȃ� // ����������Ă�
			if( sd == NULL || (sd->sc_data && (md->bl.id != sd->sc_data[SC_DEVOTION].val1)) || skill_devotion3(&md->bl,md->dev.val1[n])){
				skill_devotion_end(md,sd,n);
			}
		}
	}
}
void skill_devotion2(struct block_list *bl,int crusader)
{
	// ��f�B�{�[�V���������������̋����`�F�b�N
	struct map_session_data *sd = map_id2sd(crusader);

	nullpo_retv(bl);

	if(sd) skill_devotion3(&sd->bl,bl->id);
}
int skill_devotion3(struct block_list *bl,int target)
{
	// �N���Z�����������̋����`�F�b�N
	struct map_session_data *md;
	struct map_session_data *sd;
	int n,r=0;

	nullpo_retr(1, bl);
	md = (struct map_session_data *)bl;

	if ((sd = map_id2sd(target))==NULL)
		return 1;
	else
		r = distance(bl->x,bl->y,sd->bl.x,sd->bl.y);

	if(pc_checkskill(md,CR_DEVOTION)+6 < r){	// ���e�͈͂𒴂��Ă�
		for(n=0;n<5;n++)
			if(md->dev.val1[n]==target)
				md->dev.val2[n]=0;	// ���ꂽ���́A����؂邾��
		clif_devotion(md,sd->bl.id);
		return 1;
	}
	return 0;
}

void skill_devotion_end(struct map_session_data *md,struct map_session_data *sd,int target)
{
	// �N���Z�Ɣ�f�B�{�L�����̃��Z�b�g
	nullpo_retv(md);
	nullpo_retv(sd);

	md->dev.val1[target]=md->dev.val2[target]=0;
	if(sd && sd->sc_data){
	//	status_change_end(&sd->bl,SC_DEVOTION,-1);
		sd->sc_data[SC_DEVOTION].val1=0;
		sd->sc_data[SC_DEVOTION].val2=0;
		clif_status_change(&sd->bl,SC_DEVOTION,0);
		clif_devotion(md,sd->bl.id);
	}
}
/*==========================================
 * �I�[�g�X�y��
 *------------------------------------------
 */
int skill_autospell(struct map_session_data *sd,int skillid)
{
	int skilllv;
	int maxlv=1,lv;

	nullpo_retr(0, sd);

	skilllv = pc_checkskill(sd,SA_AUTOSPELL);

	if(skillid==MG_NAPALMBEAT)	maxlv=3;
	else if(skillid==MG_COLDBOLT || skillid==MG_FIREBOLT || skillid==MG_LIGHTNINGBOLT){
		if(skilllv==2) maxlv=1;
		else if(skilllv==3) maxlv=2;
		else if(skilllv>=4) maxlv=3;
	}
	else if(skillid==MG_SOULSTRIKE){
		if(skilllv==5) maxlv=1;
		else if(skilllv==6) maxlv=2;
		else if(skilllv>=7) maxlv=3;
	}
	else if(skillid==MG_FIREBALL){
		if(skilllv==8) maxlv=1;
		else if(skilllv>=9) maxlv=2;
	}
	else if(skillid==MG_FROSTDIVER) maxlv=1;
	else return 0;

	if(maxlv > (lv=pc_checkskill(sd,skillid)))
		maxlv = lv;

	status_change_start(&sd->bl,SC_AUTOSPELL,skilllv,skillid,maxlv,0,	// val1:�X�L��ID val2:�g�p�ő�Lv
		skill_get_time(SA_AUTOSPELL,skilllv),0);// �ɂ��Ă݂�����bscript�������Ղ��E�E�E�H
	return 0;
}

/*==========================================
 * �M�����O�X�^�[�p���_�C�X���菈��(foreachinarea)
 *------------------------------------------
 */

static int skill_gangster_count(struct block_list *bl,va_list ap)
{
	int *c;
	struct map_session_data *sd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	sd=(struct map_session_data*)bl;
	c=va_arg(ap,int *);

	if(sd && c && pc_issit(sd) && pc_checkskill(sd,RG_GANGSTER) > 0)
		(*c)++;
	return 0;
}

static int skill_gangster_in(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	sd=(struct map_session_data*)bl;
	if(sd && pc_issit(sd) && pc_checkskill(sd,RG_GANGSTER) > 0)
		sd->state.gangsterparadise=1;
	return 0;
}

static int skill_gangster_out(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	sd=(struct map_session_data*)bl;
	if(sd && sd->state.gangsterparadise)
		sd->state.gangsterparadise=0;
	return 0;
}

int skill_gangsterparadise(struct map_session_data *sd ,int type)
{
	int range=1;
	int c=0;

	nullpo_retr(0, sd);

	if(pc_checkskill(sd,RG_GANGSTER) <= 0)
		return 0;

	if(type==1) {/* ���������̏��� */
		map_foreachinarea(skill_gangster_count,sd->bl.m,
			sd->bl.x-range,sd->bl.y-range,
			sd->bl.x+range,sd->bl.y+range,BL_PC,&c);
		if(c > 1) {/*�M�����O�X�^�[���������玩���ɂ��M�����O�X�^�[�����t�^*/
			map_foreachinarea(skill_gangster_in,sd->bl.m,
				sd->bl.x-range,sd->bl.y-range,
				sd->bl.x+range,sd->bl.y+range,BL_PC);
			sd->state.gangsterparadise = 1;
		}
		return 0;
	}
	else if(type==0) {/* �����オ�����Ƃ��̏��� */
		map_foreachinarea(skill_gangster_count,sd->bl.m,
			sd->bl.x-range,sd->bl.y-range,
			sd->bl.x+range,sd->bl.y+range,BL_PC,&c);
		if(c < 2)
			map_foreachinarea(skill_gangster_out,sd->bl.m,
				sd->bl.x-range,sd->bl.y-range,
				sd->bl.x+range,sd->bl.y+range,BL_PC);
		sd->state.gangsterparadise = 0;
		return 0;
	}
	return 0;
}
/*==========================================
 * �����W���[�N�E�X�N���[�����菈��(foreachinarea)
 *------------------------------------------
 */
int skill_frostjoke_scream(struct block_list *bl,va_list ap)
{
	struct block_list *src;
	int skillnum,skilllv;
	unsigned int tick;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, src=va_arg(ap,struct block_list*));

	skillnum=va_arg(ap,int);
	skilllv=va_arg(ap,int);
	tick=va_arg(ap,unsigned int);

	if(src == bl)//�����ɂ͌����Ȃ�
		return 0;

	if(battle_check_target(src,bl,BCT_ENEMY) > 0)
		skill_additional_effect(src,bl,skillnum,skilllv,BF_MISC,tick);
	else if(battle_check_target(src,bl,BCT_PARTY) > 0) {
		if(atn_rand()%100 < 10)//PT�����o�ɂ���m���ł�����(�Ƃ肠����10%)
			skill_additional_effect(src,bl,skillnum,skilllv,BF_MISC,tick);
	}

	return 0;
}

/*==========================================
 *�A�u���J�_�u���̎g�p�X�L������(����X�L�����_���Ȃ�0��Ԃ�)
 *------------------------------------------
 */
int skill_abra_dataset(int skilllv)
{
	int skill = atn_rand()%331;
	//db�Ɋ�Â����x���E�m������
	if(skill_abra_db[skill].req_lv > skilllv || atn_rand()%10000 >= skill_abra_db[skill].per) return 0;
	//NPC�X�L���̓_��
	if(skill >= NPC_PIERCINGATT && skill <= NPC_SUMMONMONSTER) return 0;
	//���t�X�L���̓_��
	if (skill_get_unit_flag(skill)&UF_DANCE) return 0;

	return skill;
}

/*==========================================
 * �o�W���J�̃Z����ݒ肷��
 *------------------------------------------
 */
void skill_basilica_cell(struct skill_unit *unit,int flag)
{
	int i,x,y,range = skill_get_unit_range(HP_BASILICA);
	int size = range*2+1;

	for (i=0;i<size*size;i++) {
		x = unit->bl.x+(i%size-range);
		y = unit->bl.y+(i/size-range);
		map_setcell(unit->bl.m,x,y,flag);
	}
}

/*==========================================
 *
 *------------------------------------------
 */
int skill_attack_area(struct block_list *bl,va_list ap)
{
	struct block_list *src,*dsrc;
	int atk_type,skillid,skilllv,flag,type;
	unsigned int tick;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	atk_type = va_arg(ap,int);
	if((src=va_arg(ap,struct block_list*)) == NULL)
		return 0;
	if((dsrc=va_arg(ap,struct block_list*)) == NULL)
		return 0;
	skillid=va_arg(ap,int);
	skilllv=va_arg(ap,int);
	tick=va_arg(ap,unsigned int);
	flag=va_arg(ap,int);
	type=va_arg(ap,int);

	if(battle_check_target(dsrc,bl,type) > 0)
		skill_attack(atk_type,src,dsrc,bl,skillid,skilllv,tick,flag);

	return 0;
}
/*==========================================
 *
 *------------------------------------------
 */
int skill_clear_element_field(struct block_list *bl)
{
	struct mob_data *md=NULL;
	struct map_session_data *sd=NULL;
	int i,max,skillid;

	nullpo_retr(0, bl);

	if (bl->type==BL_MOB) {
		max = MAX_MOBSKILLUNITGROUP;
		md = (struct mob_data *)bl;
	} else if(bl->type==BL_PC) {
		max = MAX_SKILLUNITGROUP;
		sd = (struct map_session_data *)bl;
	} else
		return 0;

	for (i=0;i<max;i++) {
		if(sd){
			skillid=sd->skillunit[i].skill_id;
			if(skillid==SA_DELUGE||skillid==SA_VOLCANO||skillid==SA_VIOLENTGALE||skillid==SA_LANDPROTECTOR)
				skill_delunitgroup(&sd->skillunit[i]);
		}else if(md){
			skillid=md->skillunit[i].skill_id;
			if(skillid==SA_DELUGE||skillid==SA_VOLCANO||skillid==SA_VIOLENTGALE||skillid==SA_LANDPROTECTOR)
				skill_delunitgroup(&md->skillunit[i]);
		}
	}
	return 0;
}
/*==========================================
 * �����h�v���e�N�^�[�`�F�b�N(foreachinarea)
 *------------------------------------------
 */
int skill_landprotector(struct block_list *bl, va_list ap )
{
	int skillid;
	int *alive;
	struct skill_unit *unit;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	skillid=va_arg(ap,int);
	alive=va_arg(ap,int *);
	if((unit=(struct skill_unit *)bl) == NULL)
		return 0;

	if(skillid==SA_LANDPROTECTOR){
		if(alive && unit->group->skill_id==SA_LANDPROTECTOR)
			(*alive)=0;
		skill_delunit(unit);
	}else{
		if(alive && unit->group->skill_id==SA_LANDPROTECTOR)
			(*alive)=0;
	}
	return 0;
}
/*==========================================
 * �C�h�D���̗ь�̉񕜏���(foreachinarea)
 *------------------------------------------
 */
int skill_idun_heal(struct block_list *bl, va_list ap )
{
	struct skill_unit *unit;
	struct skill_unit_group *sg;
	int heal;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, unit = va_arg(ap,struct skill_unit *));
	nullpo_retr(0, sg = unit->group);

	heal=30+sg->skill_lv*5+((sg->val1)>>16)*5+((sg->val2)&0xfff)/2;

	if(bl->type == BL_SKILL || bl->id == sg->src_id)
		return 0;
	
	if(bl->type == BL_PC || bl->type == BL_MOB){
		clif_skill_nodamage(&unit->bl,bl,AL_HEAL,heal,1);
		battle_heal(NULL,bl,heal,0,0);
	}
	return 0;
}


int skill_tarot_card_of_fate(struct block_list *src,struct block_list *target,int skillid,int skilllv,int tick,int flag,int wheel)
{
//	struct map_session_data* sd=NULL;
//	struct mob_data* md=NULL;
	struct map_session_data* tsd=NULL;
	struct mob_data* tmd=NULL;
	int card_num;
	
	if(skillid != CG_TAROTCARD)
		return 0;
	if(src == NULL)
		return 0;
	if(target==NULL)
		return 0;
	/*
	if(src->type == BL_PC)
		sd = (struct map_session_data*)src;
	else if(src->type == BL_MOB)
		md = (struct mob_data*)src;
	else return 0;
	*/
	if(target->type == BL_PC)
		tsd = (struct map_session_data*)target;
	else if(target->type == BL_MOB)
		tmd = (struct mob_data*)target;
	else return 0;
	
	//�^���̗ւ���100%����
	if(wheel == 0 && atn_rand()%10000 >= skilllv*800)
		return 0;

	card_num = atn_rand()%14;

	if(wheel == 0)//�^���̗ւ��ƃG�t�F�N�g�Ȃ��H
	{
		switch(battle_config.tarotcard_display_position)
		{
			case 1:
				clif_misceffect2(src,523+card_num);
				break;
			case 2:
				clif_misceffect2(target,523+card_num);
				break;
			case 3:
				clif_misceffect2(src,523+card_num);
				clif_misceffect2(target,523+card_num);
				break;
			default: break;
		}
	}
	switch(card_num)
	{
		case 0: //����(The Fool) 523
			if(tsd)//SP0
			{
				tsd->status.sp = 0;
				clif_updatestatus(tsd,SP_SP);
			}
			break;
		case 1://���@�t(The Magician) - 30�b��Matk�������ɗ����� 
			//clif_skill_nodamage(src,target,524,1,1);
			if(tmd ==NULL || !(tmd->mode & 0x20))//PC �� �{�X�����ȊO
				status_change_start(target,SC_THE_MAGICIAN,1,0,0,0,30000,0);
			break;
		case 2://���Վi(The High Priestess) - ���ׂĂ̕⏕���@��������
		{
			struct status_change *sc_data = status_get_sc_data(target);
			if(sc_data[SC_BLESSING].timer!=-1)
				status_change_end(target,SC_BLESSING,-1);
			if(sc_data[SC_ASSUMPTIO].timer!=-1)
				status_change_end(target,SC_ASSUMPTIO,-1);
			if(sc_data[SC_KYRIE].timer!=-1)
				status_change_end(target,SC_KYRIE,-1);
			if(sc_data[SC_INCREASEAGI].timer!=-1 )	// ���x�㏸���� 
				status_change_end(target,SC_INCREASEAGI,-1);
			/* ��{�I�Ȃ̂����؂낤
			if(sc_data[SC_MAGNIFICAT].timer!=-1 )	//
				status_change_end(target,SC_MAGNIFICAT,-1);
			if(sc_data[SC_GLORIA].timer!=-1 )	//
				status_change_end(target,SC_GLORIA,-1);
			if(sc_data[SC_AL_ANGELUS].timer!=-1 )	//
				status_change_end(target,SC_AL_ANGELUS,-1);
			*/
			//�t�^��؂�
			skill_encchant_eremental_end(target,-1);
			/*	���@�H�؂��H
			if(sc_data[SC_TWOHANDQUICKEN].timer!=-1 )
				status_change_end(target,SC_TWOHANDQUICKEN,-1);
			if(sc_data[SC_SPEARSQUICKEN].timer!=-1 )
				status_change_end(target,SC_SPEARSQUICKEN,-1);
			if(sc_data[SC_ADRENALINE].timer!=-1 )
				status_change_end(target,SC_ADRENALINE,-1);
			if(sc_data[SC_ASSNCROS].timer!=-1 )
				status_change_end(target,SC_ASSNCROS,-1);
			if(sc_data[SC_TRUESIGHT].timer!=-1 )	// �g�D���[�T�C�g
				status_change_end(target,SC_TRUESIGHT,-1);
			if(sc_data[SC_WINDWALK].timer!=-1 )	// �E�C���h�E�H�[�N 
				status_change_end(target,SC_WINDWALK,-1);
			if(sc_data[SC_CARTBOOST].timer!=-1 )	// �J�[�g�u�[�X�g
				status_change_end(target,SC_CARTBOOST,-1);
			*/
			//clif_skill_nodamage(src,target,525,1,1);
		}
			break;
		case 3://���(The Chariot) - �h��͖�����1000�_���[�W �h������_���Ɉ�j�󂳂��if(tsd)
			if(tsd){
				switch(atn_rand()%4)
				{
					case 0:
						pc_break_equip(tsd,EQP_WEAPON);
						break;
					case 1:
						pc_break_equip(tsd,EQP_ARMOR);
						break;
					case 2:
						pc_break_equip(tsd,EQP_SHIELD);
						break;
					case 3:
						pc_break_equip(tsd,EQP_HELM);
						break;
				}
			}
			//clif_skill_damage(src,target,tick,0,0,1000,1,NPC_CRITICALSLASH,1,0);
			clif_damage(src,target,tick,0,0,1000,1,4,0);
			if(tsd)	pc_damage(src,tsd,1000);
			else if(tmd) mob_damage(src,tmd,1000,0);
			break;
		case 4://��(Strength) - 30�b��ATK�������ɗ�����
			if(tmd ==NULL || !(tmd->mode & 0x20))//PC �� �{�X�����ȊO
				status_change_start(target,SC_STRENGTH,1,0,0,0,30000,0);
			break;
		case 5://���l(The Lovers) - �ǂ����Ƀe���|�[�g������- HP��2000�񕜂����
			if(tsd){
				//if(!map[tsd->bl.m].flag.noteleport)
				//{
					pc_heal(tsd,2000,0);
					//clif_misceffect2(target,227);
					pc_randomwarp(tsd,0);
				//}
			}else if(tmd){
				//if(!map[tmd->bl.m].flag.monster_noteleport)
				//{
					mob_heal(tmd,2000);
					//clif_misceffect2(target,227);
					mob_warp(tmd,tmd->bl.m,-1,-1,0);
				//}
			}
			break;
		case 6://�^���̗�(Wheel of Fortune) - �����_���ɑ��̃^���b�g�J�[�h�񖇂̌��ʂ𓯎��ɗ^����
			if(wheel == 1)//����1�x���s
			{
				skill_tarot_card_of_fate(src,target,skillid,skilllv,tick,flag,1);
			}else{//�Q���s
				skill_tarot_card_of_fate(src,target,skillid,skilllv,tick,flag,1);
				skill_tarot_card_of_fate(src,target,skillid,skilllv,tick,flag,1);
			}
			break;
		case 7://�݂�ꂽ�j(The Hanged Man) - �����A�����A�Ή��̒�������������������
		
			switch(atn_rand()%3)
			{
				case 0://����
					status_change_start(target,SC_SLEEP,7,0,0,0,skill_get_time2(NPC_SLEEPATTACK,7),0);
					break;
				case 1://����
					status_change_start(target,SC_FREEZE,7,0,0,0,skill_get_time2(MG_FROSTDIVER,7),0);
					break;
				case 2://�Ή�
					status_change_start(target,SC_STONE,7,0,0,0,skill_get_time2(MG_STONECURSE,7),0);
					break;
			}
			break;
		case 8://���_(Death) - �� + �R�[�} + �łɂ�����
			
			status_change_start(target,SC_CURSE,7,0,0,0,skill_get_time2(NPC_CURSEATTACK,7),0);
			status_change_start(target,SC_POISON,7,0,0,0,skill_get_time2(TF_POISON,7),0);
			//�R�[�}
			if(tsd){
				tsd->status.hp = 1;
				clif_updatestatus(tsd,SP_HP);
			}else if(tmd && !(tmd->mode & 0x20))
				 tmd->hp = 1;
			break;
		case 9://�ߐ�(Temperance) - 30�b�ԍ����ɂ�����
			//clif_skill_nodamage(src,target,532,1,1);
			break;
		case 10://����(The Devil) - �h��͖���6666�_���[�W + 30�b��ATK�����AMATK�����A�� 
			status_change_start(target,SC_CURSE,7,0,0,0,skill_get_time2(NPC_CURSEATTACK,7),0);
			
			if(tmd ==NULL || !(tmd->mode & 0x20))//PC �� �{�X�����ȊO
				status_change_start(target,SC_THE_DEVIL,1,0,0,0,30000,0);
				
			//clif_skill_damage(src,target,tick,0,0,6666,1,NPC_CRITICALSLASH,1,0);
			clif_damage(src,target,tick,0,0,6666,1,4,0);
			if(tsd) pc_damage(src,tsd,6666);
			else if(tmd) mob_damage(src,tmd,6666,0);
			//clif_skill_nodamage(src,target,533,1,1);
			break;
		case 11://��(The Tower) - �h��͖���4444�Œ�_���[�Wif(tsd)
			//clif_skill_damage(src,target,tick,0,0,4444,1,NPC_CRITICALSLASH,1,0);
			clif_damage(src,target,tick,0,0,4444,1,4,0);
			if(tsd) pc_damage(src,tsd,4444);
			else if(tmd) mob_damage(src,tmd,4444,0);
			//clif_damage(NPC_CRITICALSLASH,);
			//clif_skill_nodamage(src,target,534,1,1);
			break;
		case 12://��(The Star) - ������� ���Ȃ킿�A5�b�ԃX�^���ɂ�����
			status_change_start(target,SC_STAN,7,0,0,0,5000,0);
			//clif_skill_nodamage(src,target,535,1,1);
			break;
		case 13://���z(The Sun) - 30�b��ATK�AMATK�A����A�����A�h��͂��S��20%���������� 536
			if(tmd ==NULL || !(tmd->mode & 0x20))//PC �� �{�X�����ȊO
				status_change_start(target,SC_THE_SUN,1,0,0,0,30000,0);
			//clif_skill_nodamage(src,target,85,1,1);
			break;
	}
	//clif_skill_nodamage(src,target,85,1,1);
	return 1;
}
/*==========================================
 * �w��͈͓���src�ɑ΂��ėL���ȃ^�[�Q�b�g��bl�̐��𐔂���(foreachinarea)
 *------------------------------------------
 */
int skill_count_target(struct block_list *bl, va_list ap )
{
	struct block_list *src;
	int *c;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	if((src = va_arg(ap,struct block_list *)) == NULL)
		return 0;
	if((c = va_arg(ap,int *)) == NULL)
		return 0;
	if(battle_check_target(src,bl,BCT_ENEMY) > 0)
		(*c)++;
	return 0;
}
/*==========================================
 * �g���b�v�͈͏���(foreachinarea)
 *------------------------------------------
 */
int skill_trap_splash(struct block_list *bl, va_list ap )
{
	struct block_list *src;
	int tick;
	int splash_count;
	struct skill_unit *unit;
	struct skill_unit_group *sg;
	struct block_list *ss;
	int i;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, src = va_arg(ap,struct block_list *));
	nullpo_retr(0, unit = (struct skill_unit *)src);
	nullpo_retr(0, sg = unit->group);
	nullpo_retr(0, ss = map_id2bl(sg->src_id));

	tick = va_arg(ap,int);
	splash_count = va_arg(ap,int);

	if(battle_check_target(src,bl,BCT_ENEMY) > 0){
		switch(sg->unit_id){
			case 0x95:	/* �T���h�}�� */
			case 0x96:	/* �t���b�V���[ */
			case 0x94:	/* �V���b�N�E�F�[�u�g���b�v */
				skill_additional_effect(ss,bl,sg->skill_id,sg->skill_lv,BF_MISC,tick);
				break;
			case 0x8f:	/* �u���X�g�}�C�� */
			case 0x98:	/* �N���C���A�[�g���b�v */
				for(i=0;i<splash_count;i++){
					skill_attack(BF_MISC,ss,src,bl,sg->skill_id,sg->skill_lv,tick,(sg->val2)?0x0500:0);
				}
			case 0x97:	/* �t���[�W���O�g���b�v */
					skill_attack(BF_WEAPON,	ss,src,bl,sg->skill_id,sg->skill_lv,tick,(sg->val2)?0x0500:0);
				break;
			default:
				break;
		}
	}

	return 0;
}
/*----------------------------------------------------------------------------
 * �X�e�[�^�X�ُ�
 *----------------------------------------------------------------------------
 */

/*==========================================
 * �X�e�[�^�X�ُ�I��
 *------------------------------------------
 */
int skill_encchant_eremental_end(struct block_list *bl,int type)
{
	struct status_change *sc_data;
	
	nullpo_retr(0, bl);
	nullpo_retr(0, sc_data=status_get_sc_data(bl));

	if( type!=SC_ENCPOISON && sc_data[SC_ENCPOISON].timer!=-1 )	/* �G���`�����g�|�C�Y������ */
		status_change_end(bl,SC_ENCPOISON,-1);
	if( type!=SC_ASPERSIO && sc_data[SC_ASPERSIO].timer!=-1 )	/* �A�X�y���V�I���� */
		status_change_end(bl,SC_ASPERSIO,-1);
	if( type!=SC_FLAMELAUNCHER && sc_data[SC_FLAMELAUNCHER].timer!=-1 )	/* �t���C�������`������ */
		status_change_end(bl,SC_FLAMELAUNCHER,-1);
	if( type!=SC_FROSTWEAPON && sc_data[SC_FROSTWEAPON].timer!=-1 )	/* �t���X�g�E�F�|������ */
		status_change_end(bl,SC_FROSTWEAPON,-1);
	if( type!=SC_LIGHTNINGLOADER && sc_data[SC_LIGHTNINGLOADER].timer!=-1 )	/* ���C�g�j���O���[�_�[���� */
		status_change_end(bl,SC_LIGHTNINGLOADER,-1);
	if( type!=SC_SEISMICWEAPON && sc_data[SC_SEISMICWEAPON].timer!=-1 )	/* �T�C�X�~�b�N�E�F�|������ */
		status_change_end(bl,SC_SEISMICWEAPON,-1);
	if( type!=SC_SEVENWIND && sc_data[SC_SEVENWIND].timer!=-1 )	/* �g���������� */
		status_change_end(bl,SC_SEVENWIND,-1);
		
	return 0;
}

/* �N���[�L���O�����i����Ɉړ��s�\�n�т����邩�j */
int skill_check_cloaking(struct block_list *bl)
{
	static int dx[]={-1, 0, 1,-1, 1,-1, 0, 1};
	static int dy[]={-1,-1,-1, 0, 0, 1, 1, 1};
	int end=1,i;
	
	nullpo_retr(0, bl);

	if(bl->type == BL_PC && battle_config.pc_cloak_check_type&1)
		return 0;
	if(bl->type == BL_MOB && battle_config.monster_cloak_check_type&1)
		return 0;

	for(i=0;i<sizeof(dx)/sizeof(dx[0]);i++){
			if(map_getcell(bl->m,bl->x+dx[i],bl->y+dy[i],CELL_CHKNOPASS)) end=0;
	}
	if(end){
		status_change_end(bl, SC_CLOAKING, -1);
		*status_get_option(bl)&=~4;	/* �O�̂��߂̏��� */
	}
	return end;
}

/*
 *----------------------------------------------------------------------------
 * �X�L�����j�b�g
 *----------------------------------------------------------------------------
 */

/*==========================================
 * ���t/�_���X����߂�
 * flag 1�ō��t���Ȃ瑊���Ƀ��j�b�g��C����
 * 
 *------------------------------------------
 */
void skill_stop_dancing(struct block_list *src, int flag)
{
	struct status_change* sc_data;
	struct skill_unit_group* group;
	
	nullpo_retv(src);

	sc_data=status_get_sc_data(src);
	if(sc_data && sc_data[SC_DANCING].timer==-1)
		return;
	group=(struct skill_unit_group *)sc_data[SC_DANCING].val2; //�_���X�̃X�L�����j�b�gID��val2�ɓ����Ă�
	if(group && src->type==BL_PC && sc_data && sc_data[SC_DANCING].val4){ //���t���f
		struct map_session_data* dsd=map_id2sd(sc_data[SC_DANCING].val4); //������sd�擾
		if(flag){ //���O�A�E�g�ȂǕЕ��������Ă����t���p�������
			if(dsd && src->id == group->src_id){ //�O���[�v�������Ă�PC��������
				group->src_id=sc_data[SC_DANCING].val4; //�����ɃO���[�v��C����
				if(flag&1) //���O�A�E�g
					dsd->sc_data[SC_DANCING].val4=0; //�����̑�����0�ɂ��č��t�I�����ʏ�̃_���X���
				if(flag&2) //�n�G��тȂ�
					return; //���t���_���X��Ԃ��I�������Ȃ����X�L�����j�b�g�͒u���Ă��ڂ�
			}else if(dsd && dsd->bl.id == group->src_id){ //�������O���[�v�������Ă���PC��������(�����̓O���[�v�������Ă��Ȃ�)
				if(flag&1) //���O�A�E�g
					dsd->sc_data[SC_DANCING].val4=0; //�����̑�����0�ɂ��č��t�I�����ʏ�̃_���X���
				if(flag&2) //�n�G��тȂ�
					return; //���t���_���X��Ԃ��I�������Ȃ����X�L�����j�b�g�͒u���Ă��ڂ�
			}
			status_change_end(src,SC_DANCING,-1);//�����̃X�e�[�^�X���I��������
			//�����ăO���[�v�͏����Ȃ��������Ȃ��̂ŃX�e�[�^�X�v�Z������Ȃ��H
			return;
		}else{
			if(dsd && src->id == group->src_id){ //�O���[�v�������Ă�PC���~�߂�
				status_change_end((struct block_list *)dsd,SC_DANCING,-1);//����̃X�e�[�^�X���I��������
			}
			if(dsd && dsd->bl.id == group->src_id){ //�������O���[�v�������Ă���PC���~�߂�(�����̓O���[�v�������Ă��Ȃ�)
				status_change_end(src,SC_DANCING,-1);//�����̃X�e�[�^�X���I��������
			}
		}
	}
	if(flag&2 && group && src->type==BL_PC){ //�n�G�Ŕ�񂾂Ƃ��Ƃ��̓��j�b�g�����
		struct map_session_data *sd = (struct map_session_data *)src;
		skill_unit_move_unit_group(group, sd->bl.m,(sd->to_x - sd->bl.x),(sd->to_y - sd->bl.y));
		return;
	}
	skill_delunitgroup(group);
	if(src->type==BL_PC)
		status_calc_pc((struct map_session_data *)src,0);
}

/*==========================================
 * �X�L�����j�b�g������
 *------------------------------------------
 */
struct skill_unit *skill_initunit(struct skill_unit_group *group,int idx,int x,int y)
{
	struct skill_unit *unit;

	nullpo_retr(NULL, group);
	nullpo_retr(NULL, unit=&group->unit[idx]);

	if(!unit->alive)
		group->alive_count++;

	unit->bl.id=map_addobject(&unit->bl);
	unit->bl.type=BL_SKILL;
	unit->bl.m=group->map;
	unit->bl.x=x;
	unit->bl.y=y;
	unit->group=group;
	unit->val1=unit->val2=0;
	unit->alive=1;

	map_addblock(&unit->bl);
	clif_skill_setunit(unit);

	if (group->skill_id==HP_BASILICA)
		skill_basilica_cell(unit,CELL_SETBASILICA);

	return unit;
}

/*==========================================
 * �X�L�����j�b�g�폜
 *------------------------------------------
 */
int skill_delunit(struct skill_unit *unit)
{
	struct skill_unit_group *group;

	nullpo_retr(0, unit);
	if(!unit->alive)
		return 0;
	nullpo_retr(0, group=unit->group);

	/* onlimit�C�x���g�Ăяo�� */
	skill_unit_onlimit(unit,gettick());

	/* onout�C�x���g�Ăяo�� */
	if (!unit->range) {
		map_foreachinarea(skill_unit_effect,unit->bl.m,
			unit->bl.x,unit->bl.y,unit->bl.x,unit->bl.y,0,
			&unit->bl,gettick(),0);
	}

	if (group->skill_id==HP_BASILICA)
		skill_basilica_cell(unit,CELL_CLRBASILICA);

	clif_skill_delunit(unit);

	unit->group=NULL;
	unit->alive=0;
	map_delobjectnofree(unit->bl.id);
	if(group->alive_count>0 && (--group->alive_count)<=0)
		skill_delunitgroup(group);

	return 0;
}
/*==========================================
 * �X�L�����j�b�g�O���[�v������
 *------------------------------------------
 */
static int skill_unit_group_newid = MAX_SKILL_DB;
struct skill_unit_group *skill_initunitgroup(struct block_list *src,
	int count,int skillid,int skilllv,int unit_id)
{
	int i;
	struct skill_unit_group *group=NULL, *list=NULL;
	int maxsug=0;

	nullpo_retr(NULL, src);

	if(src->type==BL_PC){
		list=((struct map_session_data *)src)->skillunit;
		maxsug=MAX_SKILLUNITGROUP;
	}else if(src->type==BL_MOB){
		list=((struct mob_data *)src)->skillunit;
		maxsug=MAX_MOBSKILLUNITGROUP;
	}
	if(list){
		for(i=0;i<maxsug;i++)	/* �󂢂Ă�����̌��� */
			if(list[i].group_id==0){
				group=&list[i];
				break;
			}

		if(group==NULL){	/* �󂢂ĂȂ��̂ŌÂ����̌��� */
			int j=0;
			unsigned maxdiff=0,x,tick=gettick();
			for(i=0;i<maxsug;i++)
				if((x=DIFF_TICK(tick,list[i].tick))>maxdiff){
					maxdiff=x;
					j=i;
				}
			skill_delunitgroup(&list[j]);
			group=&list[j];
		}
	}

	if(group==NULL){
		printf("skill_initunitgroup: error unit group !\n");
		exit(1);
	}

	group->src_id=src->id;
	group->party_id=status_get_party_id(src);
	group->guild_id=status_get_guild_id(src);
	group->group_id=skill_unit_group_newid++;
	if(skill_unit_group_newid<=0)
		skill_unit_group_newid = MAX_SKILL_DB;
	group->unit=(struct skill_unit *)aCalloc(count,sizeof(struct skill_unit));
	group->unit_count=count;
	group->val1=group->val2=0;
	group->skill_id=skillid;
	group->skill_lv=skilllv;
	group->unit_id=unit_id;
	group->map=src->m;
	group->limit=10000;
	group->interval=1000;
	group->tick=gettick();
	group->valstr=NULL;

	if (skill_get_unit_flag(skillid)&UF_DANCE) {
		struct map_session_data *sd = NULL;
		if( src->type==BL_PC && (sd = (struct map_session_data *)src) ){
			sd->skillid_dance=skillid;
			sd->skilllv_dance=skilllv;
		}
		status_change_start(src,SC_DANCING,skillid,(int)group,0,0,skill_get_time(skillid,skilllv)+1000,0);
		//���t�X�L���͑������_���X��Ԃɂ���
		if (sd && skill_get_unit_flag(skillid)&UF_ENSEMBLE) {
			int c=0;
			map_foreachinarea(skill_check_condition_use_sub,sd->bl.m,
				sd->bl.x-1,sd->bl.y-1,sd->bl.x+1,sd->bl.y+1,BL_PC,&sd->bl,&c);
		}
	}
	return group;
}

/*==========================================
 * �X�L�����j�b�g�O���[�v�폜
 *------------------------------------------
 */
int skill_delunitgroup(struct skill_unit_group *group)
{
	struct block_list *src;
	int i;

	nullpo_retr(0, group);
	if(group->unit_count<=0)
		return 0;

	src=map_id2bl(group->src_id);
	//�_���X�X�L���̓_���X��Ԃ���������
	if (skill_get_unit_flag(group->skill_id)&UF_DANCE) {
		if(src)
			status_change_end(src,SC_DANCING,-1);
	}

	group->alive_count=0;
	if(group->unit!=NULL){
		for(i=0;i<group->unit_count;i++)
			if(group->unit[i].alive)
				skill_delunit(&group->unit[i]);
	}
	if(group->valstr!=NULL){
		map_freeblock(group->valstr);
		group->valstr=NULL;
	}

	map_freeblock(group->unit);	/* free()�̑ւ�� */
	group->unit=NULL;
	group->src_id=0;
	group->group_id=0;
	group->unit_count=0;
	return 0;
}

/*==========================================
 * �X�L�����j�b�g�O���[�v�S�폜
 *------------------------------------------
 */
int skill_clear_unitgroup(struct block_list *src)
{
	struct skill_unit_group *group=NULL;
	int i, maxsug=0;

	nullpo_retr(0, src);
	
	if (src->type==BL_PC) {
		group=((struct map_session_data *)src)->skillunit;
		maxsug=MAX_SKILLUNITGROUP;
	} else if(src->type==BL_MOB){
		group=((struct mob_data *)src)->skillunit;
		maxsug=MAX_MOBSKILLUNITGROUP;
	} else
		return 0;

	for(i=0;i<maxsug;i++) {
		if(group[i].group_id>0 && group[i].src_id == src->id)
			skill_delunitgroup(&group[i]);
	}
	return 0;
}

/*==========================================
 * �X�L�����j�b�g�O���[�v�̔�e��tick����
 *------------------------------------------
 */
struct skill_unit_group_tickset *skill_unitgrouptickset_search(
	struct block_list *bl,struct skill_unit_group *group,int tick)
{
	int i,j=-1,k,s,id;
	struct skill_unit_group_tickset *set;

	nullpo_retr(0, bl);
	if (group->interval==-1)
		return NULL;

	if (bl->type == BL_PC)
		set = ((struct map_session_data *)bl)->skillunittick;
	else if (bl->type == BL_MOB)
		set = ((struct mob_data *)bl)->skillunittick;
	else
		return 0;

	if (skill_get_unit_flag(group->skill_id)&UF_NOOVERLAP)
		id = s = group->skill_id;
	else
		id = s = group->group_id;

	for (i=0;i<MAX_SKILLUNITGROUPTICKSET;i++) {
		k = (i+s)%MAX_SKILLUNITGROUPTICKSET;
		if (set[k].id == id)
			return &set[k];
		else if (j==-1 && (DIFF_TICK(tick,set[k].tick)>0 || set[k].id==0))
			j=k;
	}

	if (j==-1) {
		if(battle_config.error_log) {
			printf("skill_unitgrouptickset_search: tickset is full\n");
		}
		j = id%MAX_SKILLUNITGROUPTICKSET;
	}

	set[j].id = id;
	set[j].tick = tick;
	return &set[j];
}
/*==========================================
 * �X�L�����j�b�g�^�C�}�[���������p(foreachinarea)
 *------------------------------------------
 */
int skill_unit_timer_sub_onplace(struct block_list *bl, va_list ap)
{
	struct skill_unit *unit;
	struct skill_unit_group *group;
	unsigned int tick;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	unit = va_arg(ap,struct skill_unit *);
	tick = va_arg(ap,unsigned int);

	if (bl->type!=BL_PC && bl->type!=BL_MOB)
		return 0;
	if (!unit->alive || bl->prev==NULL)
		return 0;

	nullpo_retr(0, group=unit->group);

	if (battle_check_target(&unit->bl,bl,group->target_flag)<=0)
		return 0;

	skill_unit_onplace_timer(unit,bl,tick);

	return 0;
}

/*==========================================
 * �X�L�����j�b�g�^�C�}�[�����p(foreachobject)
 *------------------------------------------
 */
int skill_unit_timer_sub( struct block_list *bl, va_list ap )
{
	struct skill_unit *unit;
	struct skill_unit_group *group;
	int range;
	unsigned int tick;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, unit=(struct skill_unit *)bl);
	tick=va_arg(ap,unsigned int);

	if(!unit->alive)
		return 0;

	nullpo_retr(0, group=unit->group);
	range = unit->range;

	/* onplace_timer�C�x���g�Ăяo�� */
	if (range>=0 && group->interval!=-1) {
		map_foreachinarea(skill_unit_timer_sub_onplace, bl->m,
			bl->x-range,bl->y-range,bl->x+range,bl->y+range,0,bl,tick);
		if (!unit->alive)
			return 0;
		// �}�O�k�X�͔����������j�b�g�͍폜����
		if (group->skill_id==PR_MAGNUS && unit->val2) {
			skill_delunit(unit);
			return 0;
		}
	}
	// �C�h�D���̗ь�ɂ���
	if (group->unit_id==0xaa && DIFF_TICK(tick,group->tick)>=6000*group->val3) {
		struct block_list *src = map_id2bl(group->src_id);
		int range = skill_get_unit_layout_type(group->skill_id,group->skill_lv);
		nullpo_retr(0, src);
		map_foreachinarea(skill_idun_heal,src->m,
			src->x-range,src->y-range,src->x+range,src->y+range,0,unit);
		group->val3++;
	}
	/* ���Ԑ؂�폜 */
	if((DIFF_TICK(tick,group->tick)>=group->limit || DIFF_TICK(tick,group->tick)>=unit->limit)){
		switch(group->unit_id){
			case 0x81:	/* ���[�v�|�[�^��(�����O) */
				group->unit_id = 0x80;
				clif_changelook(bl,LOOK_BASE,group->unit_id);
				group->limit=skill_get_time(group->skill_id,group->skill_lv);
				unit->limit=skill_get_time(group->skill_id,group->skill_lv);
				break;
			case 0x8f:	/* �u���X�g�}�C�� */
				group->unit_id = 0x8c;
				clif_changelook(bl,LOOK_BASE,group->unit_id);
				group->limit=DIFF_TICK(tick+1500,group->tick);
				unit->limit=DIFF_TICK(tick+1500,group->tick);
				break;
			case 0x90:	/* �X�L�b�h�g���b�v */
			case 0x91:	/* �A���N���X�l�A */
			case 0x93:	/* �����h�}�C�� */
			case 0x94:	/* �V���b�N�E�F�[�u�g���b�v */
			case 0x95:	/* �T���h�}�� */
			case 0x96:	/* �t���b�V���[ */
			case 0x97:	/* �t���[�W���O�g���b�v */
			case 0x98:	/* �N���C���A�[�g���b�v */
			case 0x99:	/* �g�[�L�[�{�b�N�X */
				{
					struct block_list *src=map_id2bl(group->src_id);
					if(group->unit_id == 0x91 && group->val2);
					else{
						if(src && src->type==BL_PC){
							struct item item_tmp;
							memset(&item_tmp,0,sizeof(item_tmp));
							item_tmp.nameid=1065;
							item_tmp.identify=1;
							map_addflooritem(&item_tmp,1,bl->m,bl->x,bl->y,NULL,NULL,NULL,0);	// 㩕Ԋ�
						}
					}
				}
			default:
				skill_delunit(unit);
		}
	}

	if (group->unit_id==0x8d) {	// �A�C�X�E�H�[��
		unit->val1 -= 5;
		if(unit->val1 <= 0 && unit->limit + group->tick > tick + 700)
			unit->limit = DIFF_TICK(tick+700,group->tick);
	}

	return 0;
}
/*==========================================
 * �X�L�����j�b�g�^�C�}�[����
 *------------------------------------------
 */
int skill_unit_timer( int tid,unsigned int tick,int id,int data)
{
	map_freeblock_lock();

	map_foreachobject( skill_unit_timer_sub, BL_SKILL, tick );

	map_freeblock_unlock();

	return 0;
}
/*==========================================
 * ���j�b�g�ړ������p(foreachinarea)
 *------------------------------------------
 */
int skill_unit_move_sub(struct block_list *bl, va_list ap)
{
	struct skill_unit *unit = (struct skill_unit *)bl;
	struct skill_unit_group *group;
	struct block_list *target;
	unsigned int tick,flag;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, target=va_arg(ap,struct block_list*));
	tick = va_arg(ap,unsigned int);
	flag = va_arg(ap,int);

	if (target->type!=BL_PC && target->type!=BL_MOB)
		return 0;

	nullpo_retr(0, group=unit->group);
	if (group->interval!=-1)
		return 0;

	if (!unit->alive || target->prev==NULL)
		return 0;

	if (flag)
		skill_unit_onplace(unit,target,tick);
	else
		skill_unit_onout(unit,target,tick);

	return 0;
}

/*==========================================
 * ���j�b�g�ړ�������
 *    flag  0:�ړ��O����(���j�b�g�ʒu�̃X�L�����j�b�g�𗣒E)
 *          1:�ړ��㏈��(���j�b�g�ʒu�̃X�L�����j�b�g�𔭓�)
 *------------------------------------------
 */
int skill_unit_move(struct block_list *bl,unsigned int tick,int flag)
{
	nullpo_retr(0,bl);

	if (bl->prev==NULL)
		return 0;

	map_foreachinarea(skill_unit_move_sub,
			bl->m,bl->x,bl->y,bl->x,bl->y,BL_SKILL,bl,tick,flag);

	return 0;
}
/*==========================================
 * �X�L�����j�b�g���̂̈ړ�������
 * �����̓O���[�v�ƈړ���
 *------------------------------------------
 */
int skill_unit_move_unit_group( struct skill_unit_group *group, int m,int dx,int dy)
{
	int i,j;
	int tick = gettick();
	int *m_flag;
	struct skill_unit *unit1;
	struct skill_unit *unit2;

	nullpo_retr(0, group);
	if (group->unit_count<=0)
		return 0;
	if (group->unit==NULL)
		return 0;

	// �ړ��\�ȃX�L���̓_���X�n�ƁA�u���X�g�}�C���A�N���C���A�[�g���b�v�̂�
	if (!(skill_get_unit_flag(group->skill_id)&UF_DANCE) &&
			group->skill_id!=HT_CLAYMORETRAP && group->skill_id!=HT_BLASTMINE)
		return 0;

	m_flag = malloc(sizeof(int)*group->unit_count);
	memset(m_flag,0,sizeof(int)*group->unit_count);// �ړ��t���O
	// ��Ƀt���O��S�����߂�
	//    m_flag
	//		0: �P���ړ�
	//      1: ���j�b�g���ړ�����(���ʒu���烆�j�b�g���Ȃ��Ȃ�)
	//      2: �c�����V�ʒu���ړ���ƂȂ�(�ړ���Ƀ��j�b�g�����݂��Ȃ�)
	//      3: �c��
	for(i=0;i<group->unit_count;i++){
		unit1=&group->unit[i];
		if (!unit1->alive || unit1->bl.m!=m)
			continue;
		for(j=0;j<group->unit_count;j++){
			unit2=&group->unit[j];
			if (!unit2->alive)
				continue;
			if (unit1->bl.x+dx==unit2->bl.x && unit1->bl.y+dy==unit2->bl.y){
				// �ړ���Ƀ��j�b�g�����Ԃ��Ă���
				m_flag[i] |= 0x1;
			}
			if (unit1->bl.x-dx==unit2->bl.x && unit1->bl.y-dy==unit2->bl.y){
				// ���j�b�g�����̏ꏊ�ɂ���Ă���
				m_flag[i] |= 0x2;
			}
		}
	}
	// �t���O�Ɋ�Â��ă��j�b�g�ړ�
	// �t���O��1��unit��T���A�t���O��2��unit�̈ړ���Ɉڂ�
	j = 0;
	for (i=0;i<group->unit_count;i++) {
		unit1=&group->unit[i];
		if (!unit1->alive)
			continue;
		if (!(m_flag[i]&0x2)) {
			// ���j�b�g���Ȃ��Ȃ�ꏊ�ŃX�L�����j�b�g�e��������
			map_foreachinarea(skill_unit_effect,unit1->bl.m,
				unit1->bl.x,unit1->bl.y,unit1->bl.x,unit1->bl.y,0,
				&unit1->bl,tick,0);
		}
		if (m_flag[i]==0) {
			// �P���ړ�
			map_delblock(&unit1->bl);
			unit1->bl.m = m;
			unit1->bl.x += dx;
			unit1->bl.y += dy;
			map_addblock(&unit1->bl);
			clif_skill_setunit(unit1);
		} else if (m_flag[i]==1) {
			// �t���O��2�̂��̂�T���Ă��̃��j�b�g�̈ړ���Ɉړ�
			for(;j<group->unit_count;j++) {
				if (m_flag[j]==2) {
					// �p���ړ�
					unit2 = &group->unit[j];
					if (!unit2->alive)
						continue;
					map_delblock(&unit1->bl);
					unit1->bl.m = m;
					unit1->bl.x = unit2->bl.x+dx;
					unit1->bl.y = unit2->bl.y+dy;
					map_addblock(&unit1->bl);
					clif_skill_setunit(unit1);
					j++;
					break;
				}
			}
		}
		if (!(m_flag[i]&0x2)) {
			// �ړ���̏ꏊ�ŃX�L�����j�b�g�𔭓�
			map_foreachinarea(skill_unit_effect,unit1->bl.m,
				unit1->bl.x,unit1->bl.y,unit1->bl.x,unit1->bl.y,0,
				&unit1->bl,tick,1);
		}
	}
	free(m_flag);
	return 0;
}

/*----------------------------------------------------------------------------
 * �A�C�e������
 *----------------------------------------------------------------------------
 */

/*==========================================
 * �A�C�e�������\����
 *------------------------------------------
 */
int skill_can_produce_mix( struct map_session_data *sd, int nameid, int trigger )
{
	int i,j;

	nullpo_retr(0, sd);

	if(nameid<=0)
		return 0;

	for(i=0;i<MAX_SKILL_PRODUCE_DB;i++){
		if(skill_produce_db[i].nameid == nameid )
			break;
	}
	if( i >= MAX_SKILL_PRODUCE_DB )	/* �f�[�^�x�[�X�ɂȂ� */
		return 0;

	if(trigger>=0){
		if(trigger == 32 || trigger == 16 || trigger==64 || trigger == 256) {
			if(skill_produce_db[i].itemlv != trigger)	/* �t�@�[�}�V�[���|�[�V�����ނƗn�z�F���z�ΈȊO�͂��� */
				return 0;
		}else{
			if(skill_produce_db[i].itemlv>=16)		/* ����ȊO�͂��� */
				return 0;
			if( itemdb_wlv(nameid)>trigger )	/* ����Lv���� */
				return 0;
		}
	}
	if( (j=skill_produce_db[i].req_skill)>0 && pc_checkskill(sd,j)<=0 )
		return 0;		/* �X�L��������Ȃ� */

	for(j=0;j<MAX_PRODUCE_RESOURCE;j++){
		int id,x,y;
		if( (id=skill_produce_db[i].mat_id[j]) <= 0 )	/* ����ȏ�͍ޗ��v��Ȃ� */
			continue;
		if(skill_produce_db[i].mat_amount[j] <= 0) {
			if(pc_search_inventory(sd,id) < 0)
				return 0;
		}
		else {
			for(y=0,x=0;y<MAX_INVENTORY;y++)
				if( sd->status.inventory[y].nameid == id )
					x+=sd->status.inventory[y].amount;
			if(x<skill_produce_db[i].mat_amount[j])	/* �A�C�e��������Ȃ� */
				return 0;
		}
	}
	return i+1;
}


/*==========================================
 * �A�C�e�������\����
 *------------------------------------------
 */
int skill_produce_mix( struct map_session_data *sd,
	int nameid, int slot1, int slot2, int slot3 )
{
	int slot[3];
	int i,sc,ele,idx,equip,wlv,make_per,flag;

	nullpo_retr(0, sd);

	if( !(idx=skill_can_produce_mix(sd,nameid,-1)) )	/* �����s�� */
		return 0;
	idx--;
	slot[0]=slot1;
	slot[1]=slot2;
	slot[2]=slot3;

	/* ���ߍ��ݏ��� */
	for(i=0,sc=0,ele=0;i<3;i++){
		int j;
		if( slot[i]<=0 )
			continue;
		j = pc_search_inventory(sd,slot[i]);
		if(j < 0)	/* �s���p�P�b�g(�A�C�e������)�`�F�b�N */
			continue;
		if(slot[i]==1000){	/* ���̂����� */
			pc_delitem(sd,j,1,1);
			sc++;
		}
		if(slot[i]>=994 && slot[i]<=997 && ele==0){	/* ������ */
			static const int ele_table[4]={3,1,4,2};
			pc_delitem(sd,j,1,1);
			ele=ele_table[slot[i]-994];
		}
	}

	/* �ޗ����� */
	for(i=0;i<MAX_PRODUCE_RESOURCE;i++){
		int j,id,x;
		if( (id=skill_produce_db[idx].mat_id[i]) <= 0 )
			continue;
		x=skill_produce_db[idx].mat_amount[i];	/* �K�v�Ȍ� */
		do{	/* �Q�ȏ�̃C���f�b�N�X�ɂ܂������Ă��邩������Ȃ� */
			int y=0;
			j = pc_search_inventory(sd,id);

			if(j >= 0){
				y = sd->status.inventory[j].amount;
				if(y>x)y=x;	/* ����Ă��� */
				pc_delitem(sd,j,y,0);
			}else {
				if(battle_config.error_log)
					printf("skill_produce_mix: material item error\n");
			}

			x-=y;	/* �܂�����Ȃ������v�Z */
		}while( j>=0 && x>0 );	/* �ޗ�������邩�A�G���[�ɂȂ�܂ŌJ��Ԃ� */
	}

	/* �m������ */
	equip = itemdb_isequip(nameid);
	if(!equip) {
		if(skill_produce_db[idx].req_skill==AM_PHARMACY) {
			make_per = pc_checkskill(sd,AM_LEARNINGPOTION)*100
					+pc_checkskill(sd,AM_PHARMACY)*300+sd->status.job_level*20
					+sd->status.dex*10+sd->status.int_*5;
			if (nameid==501 || nameid==503 || nameid==504) // ���ʃ|�[�V����
				make_per += 2000;
			else if (nameid==545 || nameid==505) // �ԃX���� or �|�[�V����
				make_per -= 500;
			else if (nameid==546) // ���X����
				make_per -= 700;
			else if (nameid==547 || nameid==7139) // ���X���� or �R�[�e�B���O��
				make_per -= 1000;
			else if(nameid == 970) // �A���R�[��
				make_per += 1000;
		} else if (skill_produce_db[idx].req_skill==ASC_CDP) {
			make_per = 2000 + sd->paramc[4] * 40 + sd->paramc[5] * 20;
		} else {
			if(nameid == 998)
				make_per = 2000 + sd->status.base_level*30 + sd->paramc[4]*20 + sd->paramc[5]*10 + pc_checkskill(sd,skill_produce_db[idx].req_skill)*600;
			else if(nameid == 985)
				make_per = 1000 + sd->status.base_level*30 + sd->paramc[4]*20 + sd->paramc[5]*10 + (pc_checkskill(sd,skill_produce_db[idx].req_skill)-1)*500;
			else
				make_per = 1000 + sd->status.base_level*30 + sd->paramc[4]*20 + sd->paramc[5]*10 + pc_checkskill(sd,skill_produce_db[idx].req_skill)*500;
		}
	} else {
		int add_per;
		if(pc_search_inventory(sd,989) >= 0) add_per = 750;
		else if(pc_search_inventory(sd,988) >= 0) add_per = 500;
		else if(pc_search_inventory(sd,987) >= 0) add_per = 250;
		else if(pc_search_inventory(sd,986) >= 0) add_per = 0;
		else add_per = -500;
		if(ele) add_per -= 500;
		add_per -= sc*500;
		wlv = itemdb_wlv(nameid);
		make_per = ((250 + sd->status.base_level*15 + sd->paramc[4]*10 + sd->paramc[5]*5 + pc_checkskill(sd,skill_produce_db[idx].req_skill)*500 +
			add_per) * (100 - (wlv - 1)*20))/100 + pc_checkskill(sd,BS_WEAPONRESEARCH)*100 + ((wlv >= 3)? pc_checkskill(sd,BS_ORIDEOCON)*100 : 0);
	}

	if(make_per < 1) make_per = 1;

	if(skill_produce_db[idx].req_skill==AM_PHARMACY) {
		if( battle_config.pp_rate!=100 )
			make_per=make_per*battle_config.pp_rate/100;
	} else if (skill_produce_db[idx].req_skill == ASC_CDP) {
		if (battle_config.cdp_rate != 100)
			make_per = make_per*battle_config.cdp_rate/100;
	} else {
		if( battle_config.wp_rate!=100 )	/* �m���␳ */
			make_per=make_per*battle_config.wp_rate/100;
	}

//	if(battle_config.etc_log)
//		printf("make rate = %d\n",make_per);
	
	//�{�q�̐�����70%
	if(sd->status.class >= PC_CLASS_BASE3)
		make_per = make_per*70/100;

	if(atn_rand()%10000 < make_per){
		/* ���� */
		struct item tmp_item;
		memset(&tmp_item,0,sizeof(tmp_item));
		tmp_item.nameid=nameid;
		tmp_item.amount=1;
		tmp_item.identify=1;
		if(equip){	/* ����̏ꍇ */
			tmp_item.card[0]=0x00ff; /* ��������t���O */
			tmp_item.card[1]=((sc*5)<<8)+ele;	/* �����Ƃ悳 */
			*((unsigned long *)(&tmp_item.card[2]))=sd->char_id;	/* �L����ID */
		}
		else if((battle_config.produce_item_name_input && skill_produce_db[idx].req_skill!=AM_PHARMACY) ||
			(battle_config.produce_potion_name_input && skill_produce_db[idx].req_skill==AM_PHARMACY)) {
			tmp_item.card[0]=0x00fe;
			tmp_item.card[1]=0;
			*((unsigned long *)(&tmp_item.card[2]))=sd->char_id;	/* �L����ID */
		}

		switch (skill_produce_db[idx].req_skill) {
			case AM_PHARMACY:
				clif_produceeffect(sd,2,nameid);/* ����G�t�F�N�g */
				clif_misceffect(&sd->bl,5); /* ���l�ɂ�������ʒm */
				break;
			case ASC_CDP:
				clif_produceeffect(sd,2,nameid);/* �b��Ő���G�t�F�N�g */
				clif_misceffect(&sd->bl,5);
				break;
			default:  /* ���퐻���A�R�C������ */
				clif_produceeffect(sd,0,nameid); /* ���퐻���G�t�F�N�g */
				clif_misceffect(&sd->bl,3);
				break;
		}

		if((flag = pc_additem(sd,&tmp_item,1))) {
			clif_additem(sd,0,0,flag);
			map_addflooritem(&tmp_item,1,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
		}
	} else {
		// ���s
		switch (skill_produce_db[idx].req_skill) {
			case AM_PHARMACY:
				clif_produceeffect(sd,3,nameid);/* ���򎸔s�G�t�F�N�g */
				clif_misceffect(&sd->bl,6); /* ���l�ɂ����s��ʒm */
				break;
			case ASC_CDP:
				clif_produceeffect(sd,3,nameid); /* �b��Ő���G�t�F�N�g */
				clif_misceffect(&sd->bl,6); /* ���l�ɂ����s��ʒm */
				pc_heal(sd, -(sd->status.max_hp>>2), 0);
				status_get_hp(&sd->bl);
				break;
			default:
				clif_produceeffect(sd,1,nameid);/* ���퐻�����s�G�t�F�N�g */
				clif_misceffect(&sd->bl,2); /* ���l�ɂ����s��ʒm */
				break;
		}
	}
	return 0;
}

int skill_arrow_create( struct map_session_data *sd,int nameid)
{
	int i,j,flag,index=-1;
	struct item tmp_item;

	nullpo_retr(0, sd);

	if(nameid <= 0)
		return 1;

	for(i=0;i<MAX_SKILL_ARROW_DB;i++)
		if(nameid == skill_arrow_db[i].nameid) {
			index = i;
			break;
		}

	if(index < 0 || (j = pc_search_inventory(sd,nameid)) < 0)
		return 1;

	pc_delitem(sd,j,1,0);
	for(i=0;i<5;i++) {
		memset(&tmp_item,0,sizeof(tmp_item));
		tmp_item.identify = 1;
		tmp_item.nameid = skill_arrow_db[index].cre_id[i];
		tmp_item.amount = skill_arrow_db[index].cre_amount[i];
		if(battle_config.making_arrow_name_input) {
			tmp_item.card[0]=0x00fe;
			tmp_item.card[1]=0;
			*((unsigned long *)(&tmp_item.card[2]))=sd->char_id;	/* �L����ID */
		}
		if(tmp_item.nameid <= 0 || tmp_item.amount <= 0)
			continue;
		if((flag = pc_additem(sd,&tmp_item,tmp_item.amount))) {
			clif_additem(sd,0,0,flag);
			map_addflooritem(&tmp_item,tmp_item.amount,sd->bl.m,sd->bl.x,sd->bl.y,NULL,NULL,NULL,0);
		}
	}

	return 0;
}
/*==========================================
 * ����C��
 *------------------------------------------
 */
int skill_can_repair( struct map_session_data *sd, int nameid )
{
	int wlv;

	nullpo_retr(0, sd);

	if(nameid <= 0 || itemdb_isequip(nameid)==0)
		return 0;

	wlv = itemdb_wlv(nameid);
	if(itemdb_isequip(nameid) && itemdb_type(nameid)!=4 && itemdb_type(nameid)!=7)
		wlv=5;

	switch(wlv){
		case 1:
			if(pc_search_inventory(sd,1002) >= 0)	//�S�z��
				return 1002;
			break;
		case 2:
			if(pc_search_inventory(sd,998) >= 0)	//�S
				return 998;
			break;
		case 3:
		case 5:
			if(pc_search_inventory(sd,999) >= 0)	//�|�S
				return 999;
			break;
		case 4:
			if(pc_search_inventory(sd,756) >= 0)	//�I���f�I�R������
				return 756;
			break;
		default:
			break;
	}
	return 0;
}
int skill_repair_weapon( struct map_session_data *sd,int i )
{
	int nameid,material;
	struct map_session_data *dstsd=sd->repair_target;

	nullpo_retr(0, sd);

	if(dstsd && (nameid=dstsd->status.inventory[i].nameid) > 0
		&& dstsd->status.inventory[i].attribute!=0 && (material=skill_can_repair(sd,nameid))){
		//���я���
		pc_delitem(sd,pc_search_inventory(sd,material),1,0);
		dstsd->status.inventory[i].attribute=0;
		clif_delitem(dstsd,i,1);
		clif_additem(dstsd,i,1,0);
		return nameid;
	}
	return 0;
}

//int mode	�U����1 ���� 2
//�I�[�g�X�y��
int skill_use_bonus_autospell(struct block_list * src,struct block_list * bl,int skill_id,int skill_lv,int rate,int skill_flag,int tick,int flag)
{
	int t_race = 7, t_ele = 0;
	int skillid	=skill_id;
	int skilllv	=skill_lv;
	int asflag  =skill_flag;
	struct block_list *spell_target;
	int f=0,sp = 0;
	struct map_session_data *sd=(struct map_session_data *)src;
	nullpo_retr(0, sd);
	nullpo_retr(0, bl);
	
	if(sd->bl.type != BL_PC)
		return 0;

	//AS���̂�����
	//if(!(asflag&EAS_ENABLE))
	// 	return 0;
			
	//��������
	if(skillid <= 0 || skilllv <= 0 || atn_rand()%100 > rate)
			return 0;
			
	//���̊Ԃɂ�����������ł���
	if(pc_isdead(sd)) 
		return 0;
	
	//�U���Ώۂ������Ă���H
	if(bl->type == BL_PC && pc_isdead((struct map_session_data *)bl)) 
		return 0;
	else if(bl->type == BL_MOB && mob_isdead((struct block_list *)bl)) 
		return 0;
		
	//�X�y���Ώ�
	if(asflag&EAS_SELF) 
		spell_target = (struct block_list *)sd;//����
	else if(asflag&EAS_TARGET_RAND)
	{
		if(atn_rand()%100 < 50)
			spell_target = (struct block_list *)sd;//����
		else 
			spell_target = (struct block_list *)bl;//����
	}else spell_target = (struct block_list *)bl;//����
		
	t_race = status_get_race(spell_target);
	t_ele  = status_get_element(spell_target);
		
	//���x������
	if(asflag&EAS_USEMAX && (pc_checkskill(sd,skillid) == skill_get_max(skillid)))//Max������ꍇ�̂�
	{
		skilllv = pc_checkskill(sd,skillid);
	}else if(asflag&EAS_USEBETTER && (pc_checkskill(sd,skillid) > skilllv))//����ȏ�̃��x��������ꍇ�̂�
	{
		skilllv = pc_checkskill(sd,skillid);
	}//���̂܂�
	
	//���x���̕ϓ�
	if(asflag&EAS_FLUCT) //���x���ϓ� ����`�r�p
	{
		int j = atn_rand()%100;
		if( j >= 50) skilllv -= 2;
		else if(j >= 15) skilllv--;
		if(skilllv < 1) skilllv = 1;
	}else if(asflag&EAS_USERANDAM)//1�`�w��܂ł̃����_��
		skilllv = atn_rand()%skilllv+1;
	
	//SP����
	sp = skill_get_sp(skillid,skilllv);	
	if(battle_config.equip_autospell_nocost)
		sp = 0;
	else if(asflag&EAS_NOSP)
		sp = 0;
	else if(asflag&EAS_SPCOST1)
		sp = sp*2/3;	
	else if(asflag&EAS_SPCOST2)
		sp = sp/2;
	else if(asflag&EAS_SPCOST3)
		sp = sp*3/2;
	
	//SP������Ȃ��I
	if(sd->status.sp < sp)
		return 0;
	
	//���s
	if((skill_get_inf(skillid) == 2) || (skill_get_inf(skillid) == 32)) //�ꏊ���(�ݒu�n�X�L��)
		f = skill_castend_pos2(&sd->bl,spell_target->x,spell_target->y,skillid,skilllv,tick,flag);
	else {
		switch( skill_get_nk(skillid)&3 ) {
			case 0://�ʏ�
			case 2://������΂�
				f = skill_castend_damage_id(&sd->bl,spell_target,skillid,skilllv,tick,flag);
				break;
			case 1:// �x���n
				if((skillid==AL_HEAL || (skillid==ALL_RESURRECTION && spell_target->type != BL_PC)) && battle_check_undead(t_race,t_ele))
					f = skill_castend_damage_id(&sd->bl,spell_target,skillid,skilllv,tick,flag);
				else
					f = skill_castend_nodamage_id(&sd->bl,spell_target,skillid,skilllv,tick,flag);
				break;
		}
	}
	if(!f) pc_heal(sd,0,-sp);
	return 1;//����	
}

int skill_bonus_autospell(struct block_list * src,struct block_list * bl,int mode,int tick,int flag)
{
	int i;
	struct map_session_data* sd = (struct map_session_data*)src;
	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	if(src->type != BL_PC)
		return 0;
		
	if(battle_config.expand_autospell)
	{
		for(i=0;i<sd->autospell.count;i++)
		{
			if(!(sd->autospell.flag[i]&mode))
				continue;
					
			if(battle_config.once_autospell)//�I�[�g�X�y���͈�x�����������Ȃ�
			{
				if(skill_use_bonus_autospell(src,bl,sd->autospell.id[i],sd->autospell.lv[i],sd->autospell.rate[i],sd->autospell.flag[i],tick,flag))
					return 1;
			}else{
				skill_use_bonus_autospell(src,bl,sd->autospell.id[i],sd->autospell.lv[i],sd->autospell.rate[i],sd->autospell.flag[i],tick,flag);
			}
		}
	}else{
		if(mode == 	AS_ATTACK)
			return 	skill_use_bonus_autospell(src,bl,sd->autospell.id[0],sd->autospell.lv[0],sd->autospell.rate[0],sd->autospell.flag[0],tick,flag);
		else if(mode == AS_REVENGE)
				skill_use_bonus_autospell(src,bl,sd->autospell.id[1],sd->autospell.lv[1],sd->autospell.rate[1],sd->autospell.flag[1],tick,flag);
	}
	return 1;
}

/*----------------------------------------------------------------------------
 * �������n
 */

/*
 * �����񏈗�
 *        ',' �ŋ�؂��� val �ɖ߂�
 */
int skill_split_str(char *str,char **val,int num)
{
	int i;

	for (i=0; i<num && str; i++){
		val[i] = str;
		str = strchr(str,',');
		if (str)
			*str++=0;
	}
	return i;
}
/*
 * �����񏈗�
 *      ':' �ŋ�؂���atoi����val�ɖ߂�
 */
int skill_split_atoi(char *str,int *val)
{
	int i, max = 0;

	for (i=0; i<MAX_SKILL_LEVEL; i++) {
		if (str) {
			val[i] = max = atoi(str);
			str = strchr(str,':');
			if (str)
				*str++=0;
		} else {
			val[i] = max;
		}
	}
	return i;
}

/*
 * �X�L�����j�b�g�̔z�u���쐬
 */
void skill_init_unit_layout(void)
{
	int i,j,size,pos = 0;

	memset(skill_unit_layout,0,sizeof(skill_unit_layout));
	// ��`�̃��j�b�g�z�u���쐬����
	for (i=0; i<=MAX_SQUARE_LAYOUT; i++) {
		size = i*2+1;
		skill_unit_layout[i].count = size*size;
		for (j=0; j<size*size; j++) {
			skill_unit_layout[i].dx[j] = (j%size-i);
			skill_unit_layout[i].dy[j] = (j/size-i);
		}
	}
	pos = i;
	// ��`�ȊO�̃��j�b�g�z�u���쐬����
	for (i=0;i<MAX_SKILL_DB;i++) {
		if (!skill_db[i].unit_id[0] || skill_db[i].unit_layout_type[0] != -1)
			continue;
		switch (i) {
			case MG_FIREWALL:
			case WZ_ICEWALL:
				// �t�@�C�A�[�E�H�[���A�A�C�X�E�H�[���͕����ŕς��̂ŕʏ���
				break;
			case PR_SANCTUARY:
			{
				static const int dx[] = {
					-1, 0, 1,-2,-1, 0, 1, 2,-2,-1,
					 0, 1, 2,-2,-1, 0, 1, 2,-1, 0, 1};
				static const int dy[]={
					-2,-2,-2,-1,-1,-1,-1,-1, 0, 0,
					 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2};
				skill_unit_layout[pos].count = 21;
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
				break;
			}
			case PR_MAGNUS:
			{
				static const int dx[] = {
					-1, 0, 1,-1, 0, 1,-3,-2,-1, 0,
					 1, 2, 3,-3,-2,-1, 0, 1, 2, 3,
					-3,-2,-1, 0, 1, 2, 3,-1, 0, 1,-1, 0, 1};
				static const int dy[] = {
					-3,-3,-3,-2,-2,-2,-1,-1,-1,-1,
					-1,-1,-1, 0, 0, 0, 0, 0, 0, 0,
					 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3};
				skill_unit_layout[pos].count = 33;
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
				break;
			}
			case AS_VENOMDUST:
			{
				static const int dx[] = {-1, 0, 0, 0, 1};
				static const int dy[] = { 0,-1, 0, 1, 0};
				skill_unit_layout[pos].count = 5;
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
				break;
			}
			case CR_GRANDCROSS:
			case NPC_DARKGRANDCROSS:
			{
				static const int dx[] = {
					 0, 0,-1, 0, 1,-2,-1, 0, 1, 2,
					-4,-3,-2,-1, 0, 1, 2, 3, 4,-2,
					-1, 0, 1, 2,-1, 0, 1, 0, 0};
				static const int dy[] = {
					-4,-3,-2,-2,-2,-1,-1,-1,-1,-1,
					 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					 1, 1, 1, 1, 2, 2, 2, 3, 4};
				skill_unit_layout[pos].count = 29;
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
				break;
			}
			case PF_FOGWALL:
			{
				static const int dx[] = {
					-2,-1, 0, 1, 2,-2,-1, 0, 1, 2,-2,-1, 0, 1, 2};
				static const int dy[] = {
					-1,-1,-1,-1,-1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1};
				skill_unit_layout[pos].count = 15;
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
				break;
			}
			case PA_GOSPEL:
			{
				static const int dx[] = {
					-1, 0, 1,-1, 0, 1,-3,-2,-1, 0,
					 1, 2, 3,-3,-2,-1, 0, 1, 2, 3,
					-3,-2,-1, 0, 1, 2, 3,-1, 0, 1,
					-1, 0, 1};
				static const int dy[] = {
					-3,-3,-3,-2,-2,-2,-1,-1,-1,-1,
					-1,-1,-1, 0, 0, 0, 0, 0, 0, 0,
					 1, 1, 1, 1, 1, 1, 1, 2, 2, 2,
					 3, 3, 3};
				skill_unit_layout[pos].count = 33;
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
				break;
			}
			default:
				printf("unknown unit layout at skill %d\n",i);
				break;
		}
		if (!skill_unit_layout[pos].count)
			continue;
		for (j=0;j<MAX_SKILL_LEVEL;j++)
			skill_db[i].unit_layout_type[j] = pos;
		pos++;
	}
	// �t�@�C���[�E�H�[��
	firewall_unit_pos = pos;
	for (i=0;i<8;i++) {
		if (i&1) {	/* �΂ߔz�u */
			skill_unit_layout[pos].count = 5;
			if (i&0x2) {
				int dx[] = {-1,-1, 0, 0, 1};
				int dy[] = { 1, 0, 0,-1,-1};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			} else {
				int dx[] = { 1, 1 ,0, 0,-1}; 
				int dy[] = { 1, 0, 0,-1,-1}; 
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			}
		} else {	/* �c���z�u */
			skill_unit_layout[pos].count = 3;
			if (i%4==0) {	/* �㉺ */
				int dx[] = {-1, 0, 1};
				int dy[] = { 0, 0, 0};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			} else {			/* ���E */
				int dx[] = { 0, 0, 0};
				int dy[] = {-1, 0, 1};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			}
		}
		pos++;
	}
	// �A�C�X�E�H�[��
	icewall_unit_pos = pos;
	for (i=0;i<8;i++) {
		skill_unit_layout[pos].count = 5;
		if (i&1) {	/* �΂ߔz�u */
			if (i&0x2) {
				int dx[] = {-2,-1, 0, 1, 2};
				int dy[] = { 2, 1, 0,-1,-2};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			} else {
				int dx[] = { 2, 1 ,0,-1,-2}; 
				int dy[] = { 2, 1, 0,-1,-2}; 
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			}
		} else {	/* �c���z�u */
			if (i%4==0) {	/* �㉺ */
				int dx[] = {-2,-1, 0, 1, 2};
				int dy[] = { 0, 0, 0, 0, 0};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			} else {			/* ���E */
				int dx[] = { 0, 0, 0, 0, 0};
				int dy[] = {-2,-1, 0, 1, 2};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			}
		}
		pos++;
	}
}

/*==========================================
 * �X�L���֌W�t�@�C���ǂݍ���
 * skill_db.txt �X�L���f�[�^
 * skill_cast_db.txt �X�L���̉r�����Ԃƃf�B���C�f�[�^
 * produce_db.txt �A�C�e���쐬�X�L���p�f�[�^
 * create_arrow_db.txt ��쐬�X�L���p�f�[�^
 * abra_db.txt �A�u���J�_�u�������X�L���f�[�^
 *------------------------------------------
 */
int skill_readdb(void)
{
	int i,j,k,l,m;
	FILE *fp;
	char line[1024],*p;
	char *filename[]={"db/produce_db.txt","db/produce_db2.txt"};

	/* �X�L���f�[�^�x�[�X */
	memset(skill_db,0,sizeof(skill_db));
	fp=fopen("db/skill_db.txt","r");
	if(fp==NULL){
		printf("can't read db/skill_db.txt\n");
		return 1;
	}
	k = 0;
	while(fgets(line,1020,fp)){
		char *split[50];
		if(line[0]=='/' && line[1]=='/')
			continue;
		j = skill_split_str(line,split,14);
		if(split[13]==NULL || j<14)
			continue;

		i=atoi(split[0]);
		//if(i<0 || i>MAX_SKILL_DB) continue;

		if(! ( (0<=i && i<=MAX_SKILL_DB) || (GD_SKILLBASE <= i && i<= (GD_SKILLBASE+MAX_GUILDSKILL_DB)) ) )
			continue;
		
		if(i>=GD_SKILLBASE)
			i = i-GD_SKILLBASE + MAX_SKILL_DB;
			
/*		printf("skill id=%d\n",i); */
		skill_split_atoi(split[1],skill_db[i].range);
		skill_db[i].hit=atoi(split[2]);
		skill_db[i].inf=atoi(split[3]);
		skill_db[i].pl=atoi(split[4]);
		skill_db[i].nk=atoi(split[5]);
		skill_db[i].max=atoi(split[6]);
		skill_split_atoi(split[7],skill_db[i].num);

		if(strcmpi(split[8],"yes") == 0)
			skill_db[i].castcancel=1;
		else
			skill_db[i].castcancel=0;
		skill_db[i].cast_def_rate=atoi(split[9]);
		skill_db[i].inf2=atoi(split[10]);
		skill_db[i].maxcount=atoi(split[11]);
		if(strcmpi(split[12],"weapon") == 0)
			skill_db[i].skill_type=BF_WEAPON;
		else if(strcmpi(split[12],"magic") == 0)
			skill_db[i].skill_type=BF_MAGIC;
		else if(strcmpi(split[12],"misc") == 0)
			skill_db[i].skill_type=BF_MISC;
		else
			skill_db[i].skill_type=0;
		skill_split_atoi(split[13],skill_db[i].blewcount);
		k++;
	}
	fclose(fp);
	printf("read db/skill_db.txt done (count=%d)\n",k);

	fp=fopen("db/skill_require_db.txt","r");
	if(fp==NULL){
		printf("can't read db/skill_require_db.txt\n");
		return 1;
	}
	k = 0;
	while(fgets(line,1020,fp)){
		char *split[50];
		if(line[0]=='/' && line[1]=='/')
			continue;
		j = skill_split_str(line,split,29);
		if(split[28]==NULL || j<29)
			continue;

		i=atoi(split[0]);
		//if(i<0 || i>MAX_SKILL_DB) continue;
		if(! ( (0<=i && i<=MAX_SKILL_DB) || (GD_SKILLBASE <= i && i<= (GD_SKILLBASE+MAX_GUILDSKILL_DB)) ) )
			continue;
			
		if(i>=GD_SKILLBASE)
			i = i-GD_SKILLBASE + MAX_SKILL_DB;
			
		skill_split_atoi(split[1],skill_db[i].hp);
		skill_split_atoi(split[2],skill_db[i].sp);
		skill_split_atoi(split[3],skill_db[i].hp_rate);
		skill_split_atoi(split[4],skill_db[i].sp_rate);
		skill_split_atoi(split[5],skill_db[i].zeny);

		p = split[6];
		for(j=0;j<32;j++){
			l = atoi(p);
			if (l==99) {
				skill_db[i].weapon = 0xffffffff;
				break;
			}
			else
				skill_db[i].weapon |= 1<<l;
			p=strchr(p,':');
			if(!p)
				break;
			p++;
		}

		if( strcmpi(split[7],"hiding")==0 ) skill_db[i].state=ST_HIDING;
		else if( strcmpi(split[7],"cloaking")==0 ) skill_db[i].state=ST_CLOAKING;
		else if( strcmpi(split[7],"hidden")==0 ) skill_db[i].state=ST_HIDDEN;
		else if( strcmpi(split[7],"riding")==0 ) skill_db[i].state=ST_RIDING;
		else if( strcmpi(split[7],"falcon")==0 ) skill_db[i].state=ST_FALCON;
		else if( strcmpi(split[7],"cart")==0 ) skill_db[i].state=ST_CART;
		else if( strcmpi(split[7],"shield")==0 ) skill_db[i].state=ST_SHIELD;
		else if( strcmpi(split[7],"sight")==0 ) skill_db[i].state=ST_SIGHT;
		else if( strcmpi(split[7],"explosionspirits")==0 ) skill_db[i].state=ST_EXPLOSIONSPIRITS;
		else if( strcmpi(split[7],"recover_weight_rate")==0 ) skill_db[i].state=ST_RECOV_WEIGHT_RATE;
		else if( strcmpi(split[7],"move_enable")==0 ) skill_db[i].state=ST_MOVE_ENABLE;
		else if( strcmpi(split[7],"water")==0 ) skill_db[i].state=ST_WATER;
		else skill_db[i].state=ST_NONE;

		skill_split_atoi(split[8],skill_db[i].spiritball);
		skill_db[i].itemid[0]=atoi(split[9]);
		skill_db[i].amount[0]=atoi(split[10]);
		skill_db[i].itemid[1]=atoi(split[11]);
		skill_db[i].amount[1]=atoi(split[12]);
		skill_db[i].itemid[2]=atoi(split[13]);
		skill_db[i].amount[2]=atoi(split[14]);
		skill_db[i].itemid[3]=atoi(split[15]);
		skill_db[i].amount[3]=atoi(split[16]);
		skill_db[i].itemid[4]=atoi(split[17]);
		skill_db[i].amount[4]=atoi(split[18]);
		skill_db[i].itemid[5]=atoi(split[19]);
		skill_db[i].amount[5]=atoi(split[20]);
		skill_db[i].itemid[6]=atoi(split[21]);
		skill_db[i].amount[6]=atoi(split[22]);
		skill_db[i].itemid[7]=atoi(split[23]);
		skill_db[i].amount[7]=atoi(split[24]);
		skill_db[i].itemid[8]=atoi(split[25]);
		skill_db[i].amount[8]=atoi(split[26]);
		skill_db[i].itemid[9]=atoi(split[27]);
		skill_db[i].amount[9]=atoi(split[28]);
		k++;
	}
	fclose(fp);
	printf("read db/skill_require_db.txt done (count=%d)\n",k);

	/* �L���X�e�B���O�f�[�^�x�[�X */
	fp=fopen("db/skill_cast_db.txt","r");
	if(fp==NULL){
		printf("can't read db/skill_cast_db.txt\n");
		return 1;
	}
	k = 0;
	while(fgets(line,1020,fp)){
		char *split[50];
		if(line[0]=='/' && line[1]=='/')
			continue;
		j = skill_split_str(line,split,6);
		if(split[5]==NULL || j<6)
			continue;

		i=atoi(split[0]);
		if(! ( (0<=i && i<=MAX_SKILL_DB) || (GD_SKILLBASE <= i && i<= (GD_SKILLBASE+MAX_GUILDSKILL_DB)) ))
			continue;
		
		if(i>=GD_SKILLBASE)
			i = i-GD_SKILLBASE + MAX_SKILL_DB;
		
		skill_split_atoi(split[1],skill_db[i].cast);
		skill_split_atoi(split[2],skill_db[i].fixedcast);
		skill_split_atoi(split[3],skill_db[i].delay);
		skill_split_atoi(split[4],skill_db[i].upkeep_time);
		skill_split_atoi(split[5],skill_db[i].upkeep_time2);
		k++;
	}
	fclose(fp);
	printf("read db/skill_cast_db.txt done (count=%d)\n",k);

	/* �X�L�����j�b�g�f�[�^�x�[�X */
	fp = fopen("db/skill_unit_db.txt","r");
	if (fp==NULL) {
		printf("can't read db/skill_unit_db.txt\n");
		return 1;
	}
	k = 0;
	while (fgets(line,1020,fp)) {
		char *split[50];
		if (line[0]=='/' && line[1]=='/')
			continue;
		j = skill_split_str(line,split,8);
		if (split[7]==NULL || j<8)
			continue;

		i=atoi(split[0]);
		
		//if(i<0 || i>MAX_SKILL_DB)	continue;
		if(! ( (0<=i && i<=MAX_SKILL_DB) || (GD_SKILLBASE <= i && i<= (GD_SKILLBASE+MAX_GUILDSKILL_DB)) ))
			continue;
		
		if(i>=GD_SKILLBASE)
			i = i-GD_SKILLBASE + MAX_SKILL_DB;
			
		skill_db[i].unit_id[0] = strtol(split[1],NULL,16);
		skill_db[i].unit_id[1] = strtol(split[2],NULL,16);
		skill_split_atoi(split[3],skill_db[i].unit_layout_type);
		skill_db[i].unit_range = atoi(split[4]);
		skill_db[i].unit_interval = atoi(split[5]);
		skill_db[i].unit_target = strtol(split[6],NULL,16);
		skill_db[i].unit_flag = strtol(split[7],NULL,16);
		k++;
	}
	fclose(fp);
	printf("read db/skill_unit_db.txt done (count=%d)\n",k);
	skill_init_unit_layout();

	/* �����n�X�L���f�[�^�x�[�X */
	memset(skill_produce_db,0,sizeof(skill_produce_db));
	for(m=0;m<2;m++){
		fp=fopen(filename[m],"r");
		if(fp==NULL){
			if(m>0)
				continue;
			printf("can't read %s\n",filename[m]);
			return 1;
		}
		k=0;
		while(fgets(line,1020,fp)){
			char *split[6 + MAX_PRODUCE_RESOURCE * 2];
			int x,y;
			if(line[0]=='/' && line[1]=='/')
				continue;
			memset(split,0,sizeof(split));
			for(j=0,p=line;j<3 + MAX_PRODUCE_RESOURCE * 2 && p;j++){
				split[j]=p;
				p=strchr(p,',');
				if(p) *p++=0;
			}
			if(split[0]==NULL)
				continue;
			i=atoi(split[0]);
			if(i<=0)
				continue;

			skill_produce_db[k].nameid=i;
			skill_produce_db[k].itemlv=atoi(split[1]);
			skill_produce_db[k].req_skill=atoi(split[2]);

			for(x=3,y=0; split[x] && split[x+1] && y<MAX_PRODUCE_RESOURCE; x+=2,y++){
				skill_produce_db[k].mat_id[y]=atoi(split[x]);
				skill_produce_db[k].mat_amount[y]=atoi(split[x+1]);
			}
			k++;
			if(k >= MAX_SKILL_PRODUCE_DB)
				break;
		}
		fclose(fp);
		printf("read %s done (count=%d)\n",filename[m],k);
	}

	memset(skill_arrow_db,0,sizeof(skill_arrow_db));
	fp=fopen("db/create_arrow_db.txt","r");
	if(fp==NULL){
		printf("can't read db/create_arrow_db.txt\n");
		return 1;
	}
	k=0;
	while(fgets(line,1020,fp)){
		char *split[16];
		int x,y;
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(split,0,sizeof(split));
		for(j=0,p=line;j<13 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(split[0]==NULL)
			continue;
		i=atoi(split[0]);
		if(i<=0)
			continue;

		skill_arrow_db[k].nameid=i;

		for(x=1,y=0;split[x] && split[x+1] && y<5;x+=2,y++){
			skill_arrow_db[k].cre_id[y]=atoi(split[x]);
			skill_arrow_db[k].cre_amount[y]=atoi(split[x+1]);
		}
		k++;
		if(k >= MAX_SKILL_ARROW_DB)
			break;
	}
	fclose(fp);
	printf("read db/create_arrow_db.txt done (count=%d)\n",k);

	memset(skill_abra_db,0,sizeof(skill_abra_db));
	fp=fopen("db/abra_db.txt","r");
	if(fp==NULL){
		printf("can't read db/abra_db.txt\n");
		return 1;
	}
	k=0;
	while(fgets(line,1020,fp)){
		char *split[16];
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(split,0,sizeof(split));
		for(j=0,p=line;j<13 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(split[0]==NULL)
			continue;
		i=atoi(split[0]);
		if(i<=0)
			continue;

		skill_abra_db[i].req_lv=atoi(split[2]);
		skill_abra_db[i].per=atoi(split[3]);

		k++;
		if(k >= MAX_SKILL_ABRA_DB)
			break;
	}
	fclose(fp);
	printf("read db/abra_db.txt done (count=%d)\n",k);

	return 0;
}

void skill_reload(void)
{
	/*
	<empty skill database>
	<?>
	*/
	/*do_init_skill();*/
	skill_readdb();
}

/*==========================================
 * �X�L���֌W����������
 *------------------------------------------
 */
int do_init_skill(void)
{
	skill_readdb();

	add_timer_func_list(skill_unit_timer,"skill_unit_timer");
	add_timer_func_list(skill_castend_id,"skill_castend_id");
	add_timer_func_list(skill_castend_pos,"skill_castend_pos");
	add_timer_func_list(skill_timerskill,"skill_timerskill");
	add_timer_func_list(skill_castend_delay_sub,"skill_castend_delay_sub");

	add_timer_interval(gettick()+SKILLUNITTIMER_INVERVAL,skill_unit_timer,0,0,SKILLUNITTIMER_INVERVAL);

	return 0;
}
