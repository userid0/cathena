
// �X�e�[�^�X�v�Z�A��Ԉُ폈��
#include "base.h"
#include "pc.h"
#include "map.h"
#include "pet.h"
#include "mob.h"
#include "clif.h"
#include "guild.h"
#include "skill.h"
#include "itemdb.h"
#include "battle.h"
#include "chrif.h"
#include "status.h"

#include "timer.h"
#include "nullpo.h"
#include "script.h"
#include "showmsg.h"
#include "utils.h"


/* �X�L����?�����X�e?�^�X�ُ��??���e?�u�� */
int SkillStatusChangeTable[]={	/* status.h��enum��SC_***�Ƃ��킹�邱�� */
/* 0- */
	-1,-1,-1,-1,-1,-1,
	SC_PROVOKE,			/* �v���{�b�N */
	-1, 1,-1,
/* 10- */
	SC_SIGHT,			/* �T�C�g */
	-1,
	SC_SAFETYWALL,		/* �Z�[�t�e�B�[�E�H�[�� */
	-1,-1,-1,
	SC_FREEZE,			/* �t���X�g�_�C�o? */
	SC_STONE,			/* �X�g?���J?�X */
	-1,-1,
/* 20- */
	-1,-1,-1,-1,
	SC_RUWACH,			/* ���A�t */
	SC_PNEUMA,			/* �j���[�} */
	-1,-1,-1,
	SC_INCREASEAGI,		/* ���x?�� */
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
	SC_BENEDICTIO,		/* ��?�~�� */
/* 70- */
	-1,
	SC_SLOWPOISON,
	-1,
	SC_KYRIE,			/* �L���G�G���C�\�� */
	SC_MAGNIFICAT,		/* �}�O�j�t�B�J?�g */
	SC_GLORIA,			/* �O�����A */
	SC_DIVINA,			/* ���b�N�X�f�B�r?�i */
	-1,
	SC_AETERNA,			/* ���b�N�X�G?�e���i */
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
	SC_WEAPONPERFECTION,/* �E�F�|���p?�t�F�N�V���� */
	SC_OVERTHRUST,		/* �I?�o?�g���X�g */
	SC_MAXIMIZEPOWER,	/* �}�L�V�}�C�Y�p��? */
	-1,-1,-1,-1,-1,
/* 120- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 130- */
	-1,-1,-1,-1,-1,
	SC_CLOAKING,		/* �N��?�L���O */
	SC_STAN,			/* �\�j�b�N�u��? */
	-1,
	SC_ENCPOISON,		/* �G���`�����g�|�C�Y�� */
	SC_POISONREACT,		/* �|�C�Y�����A�N�g */
/* 140- */
	SC_POISON,			/* �x�m���_�X�g */
	SC_SPLASHER,		/* �x�i���X�v���b�V��? */
	-1,
	SC_TRICKDEAD,		/* ���񂾂ӂ� */
	-1,-1,
	SC_AUTOBERSERK,
	-1,-1,-1,
/* 150- */
	-1,-1,-1,-1,-1,
	SC_LOUD,			/* ���E�h�{�C�X */
	-1,
	SC_ENERGYCOAT,		/* �G�i�W?�R?�g */
	-1,-1,
/* 160- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
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
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
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
	SC_LANDPROTECTOR,
	-1,
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
	-1,-1,-1,-1,-1,-1,-1,-1,-1,
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
	SC_BERSERK,
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
	-1,
	SC_CHASEWALK,
/* 390- */
	SC_REJECTSWORD,
	-1,-1,-1,-1,
	SC_MOONLIT,
	SC_MARIONETTE,
	-1,
	SC_BLEEDING,
	SC_JOINTBEAT,
/* 400 */
	-1,-1,
	SC_MINDBREAKER,
	SC_MEMORIZE,
	SC_FOGWALL,
	SC_SPIDERWEB,
	-1,-1,
	SC_BABY,
	-1,
/* 410- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 420- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 430- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 440- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 450- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 460- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 470- */
	-1,-1,-1,-1,-1,
	SC_PRESERVE,
	-1,-1,-1,-1,
/* 480- */
	-1,-1,
	SC_DOUBLECAST,
	-1,
	SC_GRAVITATION,
	-1,
	SC_MAXOVERTHRUST,
	SC_LONGING,
	SC_HERMODE,
	-1,
/* 490- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
};

static int max_weight_base[MAX_PC_CLASS];
static int hp_coefficient[MAX_PC_CLASS];
static int hp_coefficient2[MAX_PC_CLASS];
static int hp_sigma_val[MAX_PC_CLASS][MAX_LEVEL];
static int sp_coefficient[MAX_PC_CLASS];
static int aspd_base[MAX_PC_CLASS][20];
static int refinebonus[MAX_REFINE_BONUS][3];	// ���B�{�[�i�X�e�[�u��(refine_db.txt)
int percentrefinery[5][MAX_REFINE+1];	// ���B������(refine_db.txt)
static int atkmods[3][20];	// ����ATK�T�C�Y�C��(size_fix.txt)
static char job_bonus[3][MAX_PC_CLASS][MAX_LEVEL];

int current_equip_item_index; //Contains inventory index of an equipped item. To pass it into the EQUP_SCRIPT [Lupus]
//we need it for new cards 15 Feb 2005, to check if the combo cards are insrerted into the CURRENT weapon only
//to avoid cards exploits

/*==========================================
 * ���B�{�[�i�X
 *------------------------------------------
 */
int status_getrefinebonus(int lv,int type)
{
	if (lv >= 0 && lv < MAX_REFINE_BONUS && type >= 0 && type < 3)
		return refinebonus[lv][type];
	return 0;
}

/*==========================================
 * ���B������
 *------------------------------------------
 */
int status_percentrefinery(struct map_session_data &sd,struct item &item)
{
	int percent;

	percent=percentrefinery[itemdb_wlv(item.nameid)][item.refine];

	percent += pc_checkskill(sd,BS_WEAPONRESEARCH);	// ���팤���X�L������

	// �m���̗L���͈̓`�F�b�N
	if( percent > 100 )
		percent = 100;
	if( percent < 0 )
		percent = 0;
	return percent;
}

//Skotlex: Calculates the stats of the given pet.
int status_calc_pet(struct map_session_data &sd, bool first)
{
	struct pet_data *pd;
	
	if (sd.status.pet_id == 0 || sd.pd == NULL)
		return 1;

	pd = sd.pd;
	
	if (battle_config.pet_lv_rate && pd->status)
	{
		sd.pet.level = sd.status.base_level*battle_config.pet_lv_rate/100;
		if (sd.pet.level < 0)
			sd.pet.level = 1;
		if( pd->status && pd->status->level != sd.pet.level)
		{
			if (!first) //Lv Up animation
				clif_misceffect(pd->bl, 0);
			pd->status->level = sd.pet.level;
			pd->status->atk1 = (mob_db[pd->class_].atk1*pd->status->level)/mob_db[pd->class_].lv;
			pd->status->atk2 = (mob_db[pd->class_].atk2*pd->status->level)/mob_db[pd->class_].lv;
			pd->status->str = (mob_db[pd->class_].str*pd->status->level)/mob_db[pd->class_].lv;
			pd->status->agi = (mob_db[pd->class_].agi*pd->status->level)/mob_db[pd->class_].lv;
			pd->status->vit = (mob_db[pd->class_].vit*pd->status->level)/mob_db[pd->class_].lv;
			pd->status->int_ = (mob_db[pd->class_].int_*pd->status->level)/mob_db[pd->class_].lv;
			pd->status->dex = (mob_db[pd->class_].dex*pd->status->level)/mob_db[pd->class_].lv;
			pd->status->luk = (mob_db[pd->class_].luk*pd->status->level)/mob_db[pd->class_].lv;
		
			if (pd->status->atk1 > battle_config.pet_max_atk1) pd->status->atk1 = battle_config.pet_max_atk1;
			if (pd->status->atk2 > battle_config.pet_max_atk2) pd->status->atk2 = battle_config.pet_max_atk2;

			if (pd->status->str > battle_config.pet_max_stats) pd->status->str = battle_config.pet_max_stats;
			if (pd->status->agi > battle_config.pet_max_stats) pd->status->agi = battle_config.pet_max_stats;
			if (pd->status->vit > battle_config.pet_max_stats) pd->status->vit = battle_config.pet_max_stats;
			if (pd->status->int_ > battle_config.pet_max_stats) pd->status->int_ = battle_config.pet_max_stats;
			if (pd->status->dex > battle_config.pet_max_stats) pd->status->dex = battle_config.pet_max_stats;
			if (pd->status->luk > battle_config.pet_max_stats) pd->status->luk = battle_config.pet_max_stats;

			if (!first)	//Not done the first time because the pet is not visible yet
				clif_send_petstatus(sd);
		}
	}
	//Support rate modifier (1000 = 100%)
	pd->rate_fix = 1000*(sd.pet.intimate - battle_config.pet_support_min_friendly)/(1000- battle_config.pet_support_min_friendly) +500;
	if(battle_config.pet_support_rate != 100)
		pd->rate_fix = pd->rate_fix*battle_config.pet_support_rate/100;
	return 0;
}	

/*==========================================
 * �p�����[�^�v�Z
 * first==0�̎��A�v�Z�Ώۂ̃p�����[�^���Ăяo���O����
 * �� �������ꍇ������send���邪�A
 * �\���I�ɕω��������p�����[�^�͎��O��send����悤��
 *------------------------------------------
 */

int status_calc_pc(struct map_session_data& sd, int first)
{
	int b_speed,b_max_hp,b_max_sp,b_hp,b_sp,b_weight,b_max_weight,b_paramb[6],b_parame[6],b_hit,b_flee;
	int b_aspd,b_watk,b_def,b_watk2,b_def2,b_flee2,b_critical,b_attackrange,b_matk1,b_matk2,b_mdef,b_mdef2,b_class;
	int b_base_atk;
	struct skill b_skill[MAX_SKILL];
	int i;
	size_t j;
	int bl,index;
	int skill,aspd_rate,wele,wele_,def_ele,refinedef=0;
	int pele=0,pdef_ele=0;
	int str,dstr,dex;
	struct pc_base_job s_class;

	//Calculate Common Class and Baby/High/Common flags
	s_class = pc_calc_base_job(sd.status.class_);

	b_speed = sd.speed;
	b_max_hp = sd.status.max_hp;
	b_max_sp = sd.status.max_sp;
	b_hp = sd.status.hp;
	b_sp = sd.status.sp;
	b_weight = sd.weight;
	b_max_weight = sd.max_weight;
	memcpy(b_paramb,&sd.paramb,sizeof(b_paramb));
	memcpy(b_parame,&sd.paramc,sizeof(b_parame));
	memcpy(b_skill,&sd.status.skill,sizeof(b_skill));
	b_hit = sd.hit;
	b_flee = sd.flee;
	b_aspd = sd.aspd;
	b_watk = sd.right_weapon.watk;
	b_def = sd.def;
	b_watk2 = sd.right_weapon.watk2;
	b_def2 = sd.def2;
	b_flee2 = sd.flee2;
	b_critical = sd.critical;
	b_attackrange = sd.attackrange;
	b_matk1 = sd.matk1;
	b_matk2 = sd.matk2;
	b_mdef = sd.mdef;
	b_mdef2 = sd.mdef2;
	b_class = sd.view_class;
	sd.view_class = sd.status.class_;
	b_base_atk = sd.base_atk;

	pc_calc_skilltree(sd);	// �X�L���c��?�̌v�Z

	sd.max_weight = max_weight_base[s_class.job]+sd.status.str*300;

	if(first&1)
	{
		sd.weight=0;
		for(i=0;i<MAX_INVENTORY;i++){
			if(sd.status.inventory[i].nameid==0 || sd.inventory_data[i] == NULL)
				continue;
			sd.weight += sd.inventory_data[i]->weight*sd.status.inventory[i].amount;
		}
		sd.cart_max_weight=battle_config.max_cart_weight;
		sd.cart_weight=0;
		sd.cart_max_num=MAX_CART;
		sd.cart_num=0;
		for(i=0;i<MAX_CART;i++){
			if(sd.status.cart[i].nameid==0)
				continue;
			sd.cart_weight+=itemdb_weight(sd.status.cart[i].nameid)*sd.status.cart[i].amount;
			sd.cart_num++;
		}
	}

	memset(sd.paramb,0,sizeof(sd.paramb));
	memset(sd.parame,0,sizeof(sd.parame));
	sd.hit = 0;
	sd.flee = 0;
	sd.flee2 = 0;
	sd.critical = 0;
	sd.aspd = 0;
	sd.right_weapon.watk = 0;
	sd.def = 0;
	sd.mdef = 0;
	sd.right_weapon.watk2 = 0;
	sd.def2 = 0;
	sd.mdef2 = 0;
	sd.status.max_hp = 0;
	sd.status.max_sp = 0;
	sd.attackrange = 0;
	sd.attackrange_ = 0;
	sd.right_weapon.atk_ele = 0;
	sd.def_ele = 0;
	sd.right_weapon.star = 0;
	sd.right_weapon.overrefine = 0;
	sd.matk1 = 0;
	sd.matk2 = 0;
	sd.speed = DEFAULT_WALK_SPEED ;
	sd.hprate=battle_config.hp_rate;
	sd.sprate=battle_config.sp_rate;
	sd.castrate=100;
	sd.delayrate=100;
	sd.dsprate=100;
	sd.base_atk=0;
	sd.arrow_atk=0;
	sd.arrow_ele=0;
	sd.arrow_hit=0;
	sd.arrow_range=0;
	sd.nhealhp=sd.nhealsp=sd.nshealhp=sd.nshealsp=sd.nsshealhp=sd.nsshealsp=0;
	memset(sd.right_weapon.addele,0,sizeof(sd.right_weapon.addele));
	memset(sd.right_weapon.addrace,0,sizeof(sd.right_weapon.addrace));
	memset(sd.right_weapon.addsize,0,sizeof(sd.right_weapon.addsize));
	memset(sd.left_weapon.addele,0,sizeof(sd.left_weapon.addele));
	memset(sd.left_weapon.addrace,0,sizeof(sd.left_weapon.addrace));
	memset(sd.left_weapon.addsize,0,sizeof(sd.left_weapon.addsize));
	memset(sd.subele,0,sizeof(sd.subele));
	memset(sd.subrace,0,sizeof(sd.subrace));
	memset(sd.addeff,0,sizeof(sd.addeff));
	memset(sd.addeff2,0,sizeof(sd.addeff2));
	memset(sd.reseff,0,sizeof(sd.reseff));
	sd.state.killer = 0;
	sd.state.killable = 0;
	sd.state.restart_full_recover = 0;
	sd.state.no_castcancel = 0;
	sd.state.no_castcancel2 = 0;
	sd.state.no_sizefix = 0;
	sd.state.no_magic_damage = 0;
	sd.state.no_weapon_damage = 0;
	sd.state.no_gemstone = 0;
	sd.state.infinite_endure = 0;
	memset(sd.weapon_coma_ele,0,sizeof(sd.weapon_coma_ele));
	memset(sd.weapon_coma_race,0,sizeof(sd.weapon_coma_race));
	memset(sd.weapon_atk,0,sizeof(sd.weapon_atk));
	memset(sd.weapon_atk_rate,0,sizeof(sd.weapon_atk_rate));

	sd.left_weapon.watk = 0;			//�񓁗��p(?)
	sd.left_weapon.watk2 = 0;
	sd.left_weapon.atk_ele = 0;
	sd.left_weapon.star = 0;
	sd.left_weapon.overrefine = 0;

	sd.aspd_rate = 100;
	sd.speed_rate = 100;
	sd.hprecov_rate = 100;
	sd.sprecov_rate = 100;
	sd.critical_def = 0;
	sd.double_rate = 0;
	sd.near_attack_def_rate = sd.long_attack_def_rate = 0;
	sd.atk_rate = sd.matk_rate = 100;
	sd.right_weapon.ignore_def_ele = sd.right_weapon.ignore_def_race = 0;
	sd.left_weapon.ignore_def_ele = sd.left_weapon.ignore_def_race = 0;
	sd.ignore_mdef_ele = sd.ignore_mdef_race = 0;
	sd.arrow_cri = 0;
	sd.magic_def_rate = sd.misc_def_rate = 0;
	memset(sd.arrow_addele,0,sizeof(sd.arrow_addele));
	memset(sd.arrow_addrace,0,sizeof(sd.arrow_addrace));
	memset(sd.arrow_addsize,0,sizeof(sd.arrow_addsize));
	memset(sd.arrow_addeff,0,sizeof(sd.arrow_addeff));
	memset(sd.arrow_addeff2,0,sizeof(sd.arrow_addeff2));
	memset(sd.magic_addele,0,sizeof(sd.magic_addele));
	memset(sd.magic_addrace,0,sizeof(sd.magic_addrace));
	memset(sd.magic_subrace,0,sizeof(sd.magic_subrace));
	sd.perfect_hit = 0;
	sd.critical_rate = sd.hit_rate = sd.flee_rate = sd.flee2_rate = 100;
	sd.def_rate = sd.def2_rate = sd.mdef_rate = sd.mdef2_rate = 100;
	sd.right_weapon.def_ratio_atk_ele = sd.left_weapon.def_ratio_atk_ele = 0;
	sd.right_weapon.def_ratio_atk_race = sd.left_weapon.def_ratio_atk_race = 0;
	sd.get_zeny_num = 0;
	sd.right_weapon.add_damage_class_count = sd.left_weapon.add_damage_class_count = sd.add_magic_damage_class_count = 0;
	sd.add_def_class_count = sd.add_mdef_class_count = 0;
	sd.monster_drop_item_count = 0;
	memset(sd.right_weapon.add_damage_classrate,0,sizeof(sd.right_weapon.add_damage_classrate));
	memset(sd.left_weapon.add_damage_classrate,0,sizeof(sd.left_weapon.add_damage_classrate));
	memset(sd.add_magic_damage_classrate,0,sizeof(sd.add_magic_damage_classrate));
	memset(sd.add_def_classrate,0,sizeof(sd.add_def_classrate));
	memset(sd.add_mdef_classrate,0,sizeof(sd.add_mdef_classrate));
	memset(sd.monster_drop_race,0,sizeof(sd.monster_drop_race));
	memset(sd.monster_drop_itemrate,0,sizeof(sd.monster_drop_itemrate));
	sd.speed_add_rate = sd.aspd_add_rate = 100;
	sd.double_add_rate = sd.perfect_hit_add = sd.get_zeny_add_num = 0;
	sd.splash_range = sd.splash_add_range = 0;
	memset(sd.autospell_id,0,sizeof(sd.autospell_id));
	memset(sd.autospell_lv,0,sizeof(sd.autospell_lv));
	memset(sd.autospell_rate,0,sizeof(sd.autospell_rate));
	sd.right_weapon.hp_drain_rate = sd.right_weapon.hp_drain_per = sd.right_weapon.sp_drain_rate = sd.right_weapon.sp_drain_per = 0;
	sd.left_weapon.hp_drain_rate = sd.left_weapon.hp_drain_per = sd.left_weapon.sp_drain_rate = sd.left_weapon.sp_drain_per = 0;
	sd.short_weapon_damage_return = sd.long_weapon_damage_return = 0;
	sd.magic_damage_return = 0; //AppleGirl Was Here
	sd.random_attack_increase_add = sd.random_attack_increase_per = 0;
	sd.right_weapon.hp_drain_value = sd.left_weapon.hp_drain_value = sd.right_weapon.sp_drain_value = sd.left_weapon.sp_drain_value = 0;
	sd.unbreakable_equip = 0;

	sd.break_weapon_rate = sd.break_armor_rate = 0;
	sd.add_steal_rate = 0;
	sd.crit_atk_rate = 0;
	sd.no_regen = 0;
	sd.unstripable_equip = 0;
	memset(sd.autospell2_id,0,sizeof(sd.autospell2_id));
	memset(sd.autospell2_lv,0,sizeof(sd.autospell2_lv));
	memset(sd.autospell2_rate,0,sizeof(sd.autospell2_rate));
	memset(sd.critaddrace,0,sizeof(sd.critaddrace));
	memset(sd.addeff3,0,sizeof(sd.addeff3));
	memset(sd.addeff3_type,0,sizeof(sd.addeff3_type));
	memset(sd.skillatk,0,sizeof(sd.skillatk));
	sd.right_weapon.add_damage_class_count = sd.left_weapon.add_damage_class_count = sd.add_magic_damage_class_count = 0;
	sd.add_def_class_count = sd.add_mdef_class_count = 0;
	sd.add_damage_class_count2 = 0;
	memset(sd.right_weapon.add_damage_classid,0,sizeof(sd.right_weapon.add_damage_classid));
	memset(sd.left_weapon.add_damage_classid,0,sizeof(sd.left_weapon.add_damage_classid));
	memset(sd.add_magic_damage_classid,0,sizeof(sd.add_magic_damage_classid));
	memset(sd.right_weapon.add_damage_classrate,0,sizeof(sd.right_weapon.add_damage_classrate));
	memset(sd.left_weapon.add_damage_classrate,0,sizeof(sd.left_weapon.add_damage_classrate));
	memset(sd.add_magic_damage_classrate,0,sizeof(sd.add_magic_damage_classrate));
	memset(sd.add_def_classid,0,sizeof(sd.add_def_classid));
	memset(sd.add_def_classrate,0,sizeof(sd.add_def_classrate));
	memset(sd.add_mdef_classid,0,sizeof(sd.add_mdef_classid));
	memset(sd.add_mdef_classrate,0,sizeof(sd.add_mdef_classrate));
	memset(sd.add_damage_classid2,0,sizeof(sd.add_damage_classid2));
	memset(sd.add_damage_classrate2,0,sizeof(sd.add_damage_classrate2));
	sd.sp_gain_value = 0;
	sd.right_weapon.ignore_def_mob = sd.left_weapon.ignore_def_mob = 0;
	sd.hp_loss_rate = sd.hp_loss_value = sd.hp_loss_type = 0;
	sd.sp_loss_rate = sd.sp_loss_value = 0;
	memset(sd.right_weapon.addrace2,0,sizeof(sd.right_weapon.addrace2));
	memset(sd.left_weapon.addrace2,0,sizeof(sd.left_weapon.addrace2));
	sd.hp_gain_value = sd.sp_drain_type = 0;
	memset(sd.subsize,0,sizeof(sd.subsize));
	memset(sd.unequip_losehp,0,sizeof(sd.unequip_losehp));
	memset(sd.unequip_losesp,0,sizeof(sd.unequip_losesp));
	memset(sd.subrace2,0,sizeof(sd.subrace2));
	memset(sd.expaddrace,0,sizeof(sd.expaddrace));
	memset(sd.sp_gain_race,0,sizeof(sd.sp_gain_race));
	sd.setitem_hash = 0;


	

	for(i=0;i<10;i++)
	{	//We pass INDEX to current_equip_item_index - for EQUIP_SCRIPT (new cards solution) [Lupus]
		current_equip_item_index = index = sd.equip_index[i]; 
		if(index >= MAX_INVENTORY)
			continue;
		if(i == 9 && sd.equip_index[8] == index)
			continue;
		if(i == 5 && sd.equip_index[4] == index)
			continue;
		if(i == 6 && (sd.equip_index[5] == index || sd.equip_index[4] == index))
			continue;

		if(sd.inventory_data[index])
		{
			if(sd.inventory_data[index]->type == 4)
			{	// Weapon cards
				if(sd.status.inventory[index].card[0]!=0x00ff && sd.status.inventory[index].card[0]!=0x00fe && sd.status.inventory[index].card[0]!=0xff00)
				{
					for(j=0;j<sd.inventory_data[index]->flag.slot;j++)
					{	// �J?�h
						int c=sd.status.inventory[index].card[j];
						if(c>0){
							if(i==8 && sd.status.inventory[index].equip==0x20)
								sd.state.lr_flag = 1;
							run_script(itemdb_equipscript(c),0,sd.bl.id,0);
							sd.state.lr_flag = 0;
						}
					}
				}
			}
			else if(sd.inventory_data[index]->type==5)
			{	// Non-weapon equipment cards
				if(sd.status.inventory[index].card[0]!=0x00ff && sd.status.inventory[index].card[0]!=0x00fe && sd.status.inventory[index].card[0]!=0xff00)
				{
					for(j=0;j<sd.inventory_data[index]->flag.slot;j++)
					{	// �J?�h
						int c=sd.status.inventory[index].card[j];
						if(c>0)
						{
							run_script(itemdb_equipscript(c),0,sd.bl.id,0);
						}
					}
				}
			}
		}
	}
	wele = sd.right_weapon.atk_ele;
	wele_ = sd.left_weapon.atk_ele;
	def_ele = sd.def_ele;

	if(sd.status.pet_id > 0) { // Pet
		struct pet_data *pd=sd.pd;
		if((pd && battle_config.pet_status_support) && (!battle_config.pet_equip_required || pd->equip_id > 0))
		{
			if(sd.pet.intimate > 0 && pd->state.skillbonus == 1 && pd->bonus)
			{	//Skotlex: Readjusted for pets
				pc_bonus(sd,pd->bonus->type, pd->bonus->val);
			}
			pele = sd.right_weapon.atk_ele;
			pdef_ele = sd.def_ele;
			sd.right_weapon.atk_ele = sd.def_ele = 0;
		}
	}
	memcpy(sd.paramcard,sd.parame,sizeof(sd.paramcard));

	// ?���i�ɂ��X�e?�^�X?���͂�����?�s
	for(i=0;i<10;i++) {
		current_equip_item_index = index = sd.equip_index[i]; //We pass INDEX to current_equip_item_index - for EQUIP_SCRIPT (new cards solution) [Lupus]
		if(index >=MAX_INVENTORY)
			continue;
		if(i == 9 && sd.equip_index[8] == index)
			continue;
		if(i == 5 && sd.equip_index[4] == index)
			continue;
		if(i == 6 && (sd.equip_index[5] == index || sd.equip_index[4] == index))
			continue;
		if(sd.inventory_data[index]) {
			sd.def += sd.inventory_data[index]->def;
			if(sd.inventory_data[index]->type == 4) {
				int r,wlv = sd.inventory_data[index]->wlv;
				if (wlv >= MAX_REFINE_BONUS) 
					wlv = MAX_REFINE_BONUS - 1;
				if(i == 8 && sd.status.inventory[index].equip == 0x20) { // Left-hand weapon
					sd.left_weapon.watk += sd.inventory_data[index]->atk;
					sd.left_weapon.watk2 = (r=sd.status.inventory[index].refine)*	// ��?�U?��
						refinebonus[wlv][0];
					if( (r-=refinebonus[wlv][2])>0 )	// ��?��?�{?�i�X
						sd.left_weapon.overrefine = r*refinebonus[wlv][1];

					if(sd.status.inventory[index].card[0]==0x00ff){	// Forged weapon
						sd.left_weapon.star = (sd.status.inventory[index].card[1]>>8);	// ���̂�����
						if(sd.left_weapon.star >= 15) sd.left_weapon.star = 40; // 3 Star Crumbs now give +40 dmg
						wele_= (sd.status.inventory[index].card[1]&0x0f);	// ? ��
						sd.left_weapon.fameflag = pc_istop10fame( MakeDWord(sd.status.inventory[index].card[2],sd.status.inventory[index].card[3]) ,0);
					}
					sd.attackrange_ += sd.inventory_data[index]->range;
					sd.state.lr_flag = 1;
					run_script(sd.inventory_data[index]->equip_script,0,sd.bl.id,0);
					sd.state.lr_flag = 0;
				}
				else {	// Right-hand weapon
					sd.right_weapon.watk += sd.inventory_data[index]->atk;
					sd.right_weapon.watk2 += (r=sd.status.inventory[index].refine)*	// ��?�U?��
						refinebonus[wlv][0];
					if( (r-=refinebonus[wlv][2])>0 )	// ��?��?�{?�i�X
						sd.right_weapon.overrefine += r*refinebonus[wlv][1];

					if(sd.status.inventory[index].card[0]==0x00ff){	// Forged weapon
						sd.right_weapon.star += (sd.status.inventory[index].card[1]>>8);	// ���̂�����
						if(sd.right_weapon.star >= 15) sd.right_weapon.star = 40; // 3 Star Crumbs now give +40 dmg
						wele = (sd.status.inventory[index].card[1]&0x0f);	// ? ��
						sd.right_weapon.fameflag = pc_istop10fame( MakeDWord(sd.status.inventory[index].card[2],sd.status.inventory[index].card[3]) ,0);
					}
					sd.attackrange += sd.inventory_data[index]->range;
					run_script(sd.inventory_data[index]->equip_script,0,sd.bl.id,0);
				}
			}
			else if(sd.inventory_data[index]->type == 5) {
				sd.right_weapon.watk += sd.inventory_data[index]->atk;
				refinedef += sd.status.inventory[index].refine*refinebonus[0][0];
				run_script(sd.inventory_data[index]->equip_script,0,sd.bl.id,0);
			}
		}
	}

	if(sd.equip_index[10] < MAX_INVENTORY){ // ��
		index = sd.equip_index[10];
		if(sd.inventory_data[index])
		{	// Arrows
			sd.state.lr_flag = 2;
			run_script(sd.inventory_data[index]->equip_script,0,sd.bl.id,0);
			sd.state.lr_flag = 0;
			sd.arrow_atk += sd.inventory_data[index]->atk;
		}
	}
	sd.def += (refinedef+50)/100;

	if(sd.attackrange < 1) sd.attackrange = 1;
	if(sd.attackrange_ < 1) sd.attackrange_ = 1;
	if(sd.attackrange < sd.attackrange_)
		sd.attackrange = sd.attackrange_;
	if(sd.status.weapon == 11)
		sd.attackrange += sd.arrow_range;
	if(wele > 0)
		sd.right_weapon.atk_ele = wele;
	if(wele_ > 0)
		sd.left_weapon.atk_ele = wele_;
	if(def_ele > 0)
		sd.def_ele = def_ele;
	if(battle_config.pet_status_support) {
		if(pele > 0 && !sd.right_weapon.atk_ele)
			sd.right_weapon.atk_ele = pele;
		if(pdef_ele > 0 && !sd.def_ele)
			sd.def_ele = pdef_ele;
	}
	sd.double_rate += sd.double_add_rate;
	sd.perfect_hit += sd.perfect_hit_add;
	sd.get_zeny_num += sd.get_zeny_add_num;
	sd.splash_range += sd.splash_add_range;
	if(sd.speed_add_rate != 100)	
		sd.speed_rate = sd.speed_rate*sd.speed_add_rate/100;
	if(sd.aspd_add_rate != 100)
		sd.aspd_rate = sd.aspd_rate*sd.aspd_add_rate/100;

	// ����ATK�T�C�Y�␳ (�E��)
	sd.right_weapon.atkmods[0] = atkmods[0][sd.weapontype1];
	sd.right_weapon.atkmods[1] = atkmods[1][sd.weapontype1];
	sd.right_weapon.atkmods[2] = atkmods[2][sd.weapontype1];
	//����ATK�T�C�Y�␳ (����)
	sd.left_weapon.atkmods[0] = atkmods[0][sd.weapontype2];
	sd.left_weapon.atkmods[1] = atkmods[1][sd.weapontype2];
	sd.left_weapon.atkmods[2] = atkmods[2][sd.weapontype2];

	// job�{?�i�X��
	for(i=0;i<sd.status.job_level && i<MAX_LEVEL;i++){
		if(job_bonus[s_class.upper][s_class.job][i])
			sd.paramb[job_bonus[s_class.upper][s_class.job][i]-1]++;
	}

	if( (skill=pc_checkskill(sd,MC_INCCARRY))>0 )	// skill can be used with an item now, thanks to orn [Valaris]
		sd.max_weight += skill*2000;

	if( (skill=pc_checkskill(sd,AC_OWL))>0 )	// �ӂ��낤�̖�
		sd.paramb[4] += skill;

	if((skill=pc_checkskill(sd,BS_HILTBINDING))>0) {   // Hilt binding gives +1 str +4 atk
		sd.paramb[0] ++;
		sd.base_atk += 4;
	}
	if((skill=pc_checkskill(sd,SA_DRAGONOLOGY))>0 ){ // Dragonology increases +1 int every 2 levels
		sd.paramb[3] += (skill+1)/2;
	}
	if((skill=pc_checkskill(sd,HP_MANARECHARGE))>0 ){
		sd.dsprate -= 4 * skill;
	}

	// �X�e?�^�X?���ɂ���{�p����?�^�␳

	{
		if(sd.sc_data[SC_INCALLSTATUS].timer!=-1){
			sd.paramb[0]+= sd.sc_data[SC_INCALLSTATUS].val1;
			sd.paramb[1]+= sd.sc_data[SC_INCALLSTATUS].val1;
			sd.paramb[2]+= sd.sc_data[SC_INCALLSTATUS].val1;
			sd.paramb[3]+= sd.sc_data[SC_INCALLSTATUS].val1;
			sd.paramb[4]+= sd.sc_data[SC_INCALLSTATUS].val1;
			sd.paramb[5]+= sd.sc_data[SC_INCALLSTATUS].val1;
		}
		if(sd.sc_data[SC_INCSTR].timer!=-1)
			sd.paramb[0] += sd.sc_data[SC_INCSTR].val1;
		if(sd.sc_data[SC_INCAGI].timer!=-1)
			sd.paramb[1] += sd.sc_data[SC_INCAGI].val1;
		if(sd.sc_data[SC_CONCENTRATE].timer!=-1 && sd.sc_data[SC_QUAGMIRE].timer == -1){	// �W���͌���
			sd.paramb[1]+= (sd.status.agi+sd.paramb[1]+sd.parame[1]-sd.paramcard[1])*(2+sd.sc_data[SC_CONCENTRATE].val1)/100;
			sd.paramb[4]+= (sd.status.dex+sd.paramb[4]+sd.parame[4]-sd.paramcard[4])*(2+sd.sc_data[SC_CONCENTRATE].val1)/100;
		}
		if(sd.sc_data[SC_INCREASEAGI].timer!=-1){	// ���x?��
			sd.paramb[1]+= 2 + sd.sc_data[SC_INCREASEAGI].val1;
			sd.speed_rate -= 25;
		}
		if(sd.sc_data[SC_DECREASEAGI].timer!=-1) {	// ���x����(agi��battle.c��)
			sd.paramb[1] -= 2 + sd.sc_data[SC_DECREASEAGI].val1;	// reduce agility [celest]
			sd.speed_rate += 25;
		}
		if(sd.sc_data[SC_CLOAKING].timer!=-1) {
			sd.critical_rate += 100; // critical increases
			sd.speed = sd.speed * (sd.sc_data[SC_CLOAKING].val3-sd.sc_data[SC_CLOAKING].val1*3) /100;
		}
		if(sd.sc_data[SC_CHASEWALK].timer!=-1)
			sd.speed = sd.speed * sd.sc_data[SC_CHASEWALK].val3 /100; // slow down by chasewalk
		if(sd.sc_data[SC_SLOWDOWN].timer!=-1)
			sd.speed_rate += 50;
		if(sd.sc_data[SC_SPEEDUP0].timer!=-1 && sd.sc_data[SC_INCREASEAGI].timer==-1)
			sd.speed_rate -= 25;
		if(sd.sc_data[SC_BLESSING].timer!=-1){	// �u���b�V���O
			sd.paramb[0]+= sd.sc_data[SC_BLESSING].val1;
			sd.paramb[3]+= sd.sc_data[SC_BLESSING].val1;
			sd.paramb[4]+= sd.sc_data[SC_BLESSING].val1;
		}
		if(sd.sc_data[SC_GLORIA].timer!=-1)	// �O�����A
			sd.paramb[5]+= 30;
		if(sd.sc_data[SC_LOUD].timer!=-1)	// ���E�h�{�C�X
			sd.paramb[0]+= 4;
		if(sd.sc_data[SC_QUAGMIRE].timer!=-1){	// �N�@�O�}�C�A
			sd.paramb[1]-= sd.sc_data[SC_QUAGMIRE].val1*5;
			sd.paramb[4]-= sd.sc_data[SC_QUAGMIRE].val1*5;
			sd.speed_rate += 50;
		}
		if(sd.sc_data[SC_TRUESIGHT].timer!=-1){	// �g�D��?�T�C�g
			sd.paramb[0]+= 5;
			sd.paramb[1]+= 5;
			sd.paramb[2]+= 5;
			sd.paramb[3]+= 5;
			sd.paramb[4]+= 5;
			sd.paramb[5]+= 5;
		}
		if(sd.sc_data[SC_MARIONETTE].timer!=-1){
			// skip partner checking -- should be handled in status_change_timer
			//struct map_session_data *psd = map_id2sd(sd.sc_data[SC_MARIONETTE2].val3);
			//if (psd) {	// if partner is found
				sd.paramb[0]-= sd.status.str/2;	// bonuses not included
				sd.paramb[1]-= sd.status.agi/2;
				sd.paramb[2]-= sd.status.vit/2;
				sd.paramb[3]-= sd.status.int_/2;
				sd.paramb[4]-= sd.status.dex/2;
				sd.paramb[5]-= sd.status.luk/2;
			//}
		}
		else if(sd.sc_data[SC_MARIONETTE2].timer!=-1){
			struct map_session_data *psd = map_id2sd(sd.sc_data[SC_MARIONETTE2].val3);
			if (psd) {	// if partner is found
				sd.paramb[0] += sd.status.str+psd->status.str/2 > 99 ? 99-sd.status.str : psd->status.str/2;
				sd.paramb[1] += sd.status.agi+psd->status.agi/2 > 99 ? 99-sd.status.agi : psd->status.agi/2;
				sd.paramb[2] += sd.status.vit+psd->status.vit/2 > 99 ? 99-sd.status.vit : psd->status.vit/2;
				sd.paramb[3] += sd.status.int_+psd->status.int_/2 > 99 ? 99-sd.status.int_ : psd->status.int_/2;
				sd.paramb[4] += sd.status.dex+psd->status.dex/2 > 99 ? 99-sd.status.dex : psd->status.dex/2;
				sd.paramb[5] += sd.status.luk+psd->status.luk/2 > 99 ? 99-sd.status.luk : psd->status.luk/2;
			}
		}
		if(sd.sc_data[SC_GOSPEL].timer!=-1 && sd.sc_data[SC_GOSPEL].val4 == BCT_PARTY){
			if (sd.sc_data[SC_GOSPEL].val3 == 6) {
				sd.paramb[0]+= 2;
				sd.paramb[1]+= 2;
				sd.paramb[2]+= 2;
				sd.paramb[3]+= 2;
				sd.paramb[4]+= 2;
				sd.paramb[5]+= 2;
			}
		}
		// New guild skills - Celest
		if (sd.sc_data[SC_BATTLEORDERS].timer != -1) {
			sd.paramb[0]+= 5;
			sd.paramb[3]+= 5;
			sd.paramb[4]+= 5;
		}
		if (sd.sc_data[SC_GUILDAURA].timer != -1) {
			int guildflag = sd.sc_data[SC_GUILDAURA].val4;
			for (i = 16; i >= 0; i -= 4) {
				skill = guildflag >> i;
				switch (i) {
					// guild skills
					case 16: sd.paramb[0] += skill; break;
					case 12: sd.paramb[2] += skill; break;
					case 8: sd.paramb[1] += skill; break;
					case 4: sd.paramb[4] += skill; break;
					case 0:
						// custom stats, since there's no info on how much it actually gives ^^; [Celest]
						if (skill) {
							sd.hit += 10;
							sd.flee += 10;
						}
						break;
					default:
						break;
				}
				guildflag ^= skill << i;
			}
		}
	}

	// If Super Novice / Super Baby Never Died till Job70 they get bonus: AllStats +10
	if(s_class.job == 23 && sd.die_counter == 0 && sd.status.job_level >= 70){
		sd.paramb[0]+= 10;
		sd.paramb[1]+= 10;
		sd.paramb[2]+= 10;
		sd.paramb[3]+= 10;
		sd.paramb[4]+= 10;
		sd.paramb[5]+= 10;
	}
	sd.paramc[0]=sd.status.str+sd.paramb[0]+sd.parame[0];
	sd.paramc[1]=sd.status.agi+sd.paramb[1]+sd.parame[1];
	sd.paramc[2]=sd.status.vit+sd.paramb[2]+sd.parame[2];
	sd.paramc[3]=sd.status.int_+sd.paramb[3]+sd.parame[3];
	sd.paramc[4]=sd.status.dex+sd.paramb[4]+sd.parame[4];
	sd.paramc[5]=sd.status.luk+sd.paramb[5]+sd.parame[5];
	for(i=0;i<6;i++)
		if(sd.paramc[i] < 0) sd.paramc[i] = 0;

	if (sd.sc_data[SC_CURSE].timer!=-1)
		sd.paramc[5] = 0;

	if(sd.status.weapon == 11 || sd.status.weapon == 13 || sd.status.weapon == 14) {
		str = sd.paramc[4];
		dex = sd.paramc[0];
	}
	else {
		str = sd.paramc[0];
		dex = sd.paramc[4];
	}
	dstr = str/10;
	sd.base_atk += str + dstr*dstr + dex/5 + sd.paramc[5]/5;
	sd.matk1 += sd.paramc[3]+(sd.paramc[3]/5)*(sd.paramc[3]/5);
	sd.matk2 += sd.paramc[3]+(sd.paramc[3]/7)*(sd.paramc[3]/7);
	if(sd.matk1 < sd.matk2) {
		int temp = sd.matk2;
		sd.matk2 = sd.matk1;
		sd.matk1 = temp;
	}
	sd.hit += sd.paramc[4] + sd.status.base_level;
	sd.flee += sd.paramc[1] + sd.status.base_level;
	sd.def2 += (unsigned short)sd.paramc[2];
	sd.mdef2 += (unsigned short)sd.paramc[3];
	sd.flee2 += sd.paramc[5]+10;
	sd.critical += (sd.paramc[5]*3)+10;

	if(sd.base_atk < 1)
		sd.base_atk = 1;
	if(sd.critical_rate != 100)
		sd.critical = (sd.critical*sd.critical_rate)/100;
	if(sd.critical < 10) sd.critical = 10;
	if(sd.hit_rate != 100)
		sd.hit = (sd.hit*sd.hit_rate)/100;
	if(sd.hit < 1) sd.hit = 1;
	if(sd.flee_rate != 100)
		sd.flee = (sd.flee*sd.flee_rate)/100;
	if(sd.flee < 1) sd.flee = 1;
	if(sd.flee2_rate != 100)
		sd.flee2 = (sd.flee2*sd.flee2_rate)/100;
	if(sd.flee2 < 10) sd.flee2 = 10;
	if(sd.def_rate != 100)
		sd.def = (sd.def*sd.def_rate)/100;
	if(sd.def2_rate != 100)
		sd.def2 = (sd.def2*sd.def2_rate)/100;
	if(sd.def2 < 1) sd.def2 = 1;
	if(sd.mdef_rate != 100)
		sd.mdef = (sd.mdef*sd.mdef_rate)/100;
	if(sd.mdef2_rate != 100)
		sd.mdef2 = (sd.mdef2*sd.mdef2_rate)/100;
	if(sd.mdef2 < 1) sd.mdef2 = 1;

	// �񓁗� ASPD �C��
	if (sd.status.weapon <= 16)
	{
		int ofs = (sd.paramc[1]*4+sd.paramc[4])*aspd_base[s_class.job][sd.status.weapon]/1000;
		sd.aspd += aspd_base[s_class.job][sd.status.weapon];
		if( sd.aspd>10+ofs )
			sd.aspd-=ofs;
		else
			sd.aspd = 10; // minimum	
	}
	else
	{
		int ofs = 
			(
			 (-aspd_base[s_class.job][sd.weapontype1]+(sd.paramc[1]*4+sd.paramc[4])*aspd_base[s_class.job][sd.weapontype1]/1000) +
			 (-aspd_base[s_class.job][sd.weapontype2]+(sd.paramc[1]*4+sd.paramc[4])*aspd_base[s_class.job][sd.weapontype2]/1000)
			) * 140 / 200;

		if( sd.aspd>10+ofs )
			sd.aspd-=ofs;
		else
			sd.aspd = 10; // minimum	

	}

	aspd_rate = sd.aspd_rate;

	//�U?���x?��

	if((skill=pc_checkskill(sd,AC_VULTURE))>0){	// ���V�̖�
		sd.hit += skill;
		if(sd.status.weapon == 11)
			sd.attackrange += skill;
	}

	if( (skill=pc_checkskill(sd,BS_WEAPONRESEARCH))>0)	// ����?���̖�����?��
		sd.hit += skill*2;
	if(sd.status.option&2 && (skill = pc_checkskill(sd,RG_TUNNELDRIVE))>0 )	// �g���l���h���C�u	// �g���l���h���C�u
		sd.speed += (100-16*skill)*DEFAULT_WALK_SPEED/100;
	if (pc_iscarton(sd) && (skill=pc_checkskill(sd,MC_PUSHCART))>0)	// �J?�g�ɂ�鑬�x�ቺ
		sd.speed += (10-skill) * DEFAULT_WALK_SPEED/10;
	if (pc_isriding(sd)) {	// �y�R�y�R?��ɂ�鑬�x?��
		sd.speed -= DEFAULT_WALK_SPEED/4;
		sd.max_weight += 10000;
	}
	if((skill=pc_checkskill(sd,CR_TRUST))>0) { // �t�F�C�X
		sd.status.max_hp += skill*200;
		sd.subele[6] += skill*5;
	}
	if((skill=pc_checkskill(sd,BS_SKINTEMPER))>0) {
		sd.subele[0] += skill;
		sd.subele[3] += skill*5;
	}
	if((skill=pc_checkskill(sd,SA_ADVANCEDBOOK))>0 )
		aspd_rate -= skill/2;

	bl = sd.status.base_level;
	sd.status.max_hp += (3500 + bl*hp_coefficient2[s_class.job] +
		hp_sigma_val[s_class.job][(bl > 0)? bl-1:0])/100 *
		(100 + sd.paramc[2])/100 + (sd.parame[2] - sd.paramcard[2]);

	if (s_class.upper==1) // [MouseJstr]
		sd.status.max_hp = sd.status.max_hp * 130/100;
	else if (s_class.upper==2)
		sd.status.max_hp = sd.status.max_hp * 70/100;

		{
			if(sd.sc_data[SC_INCMHP2].timer!=-1)
			sd.status.max_hp += sd.status.max_hp * sd.sc_data[SC_INCMHP2].val1 / 100;
		if (sd.sc_data[SC_BERSERK].timer!=-1){	// �o?�T?�N
			sd.status.max_hp = sd.status.max_hp * 3;			
		}
		}
	if(s_class.job == 23 && sd.status.base_level >= 99){
		sd.status.max_hp = sd.status.max_hp + 2000;
	}

	if (sd.hprate <= 0)
		sd.hprate = 1;
	if(sd.hprate!=100)
		sd.status.max_hp = sd.status.max_hp * sd.hprate/100;

	if(sd.status.hp > (long)battle_config.max_hp)
		sd.status.hp = battle_config.max_hp;
	if(sd.status.max_hp > (long)battle_config.max_hp)
		sd.status.max_hp = battle_config.max_hp;
	if(sd.status.max_hp <= 0) sd.status.max_hp = 1; // end

	// �ő�SP�v�Z
	sd.status.max_sp += ((sp_coefficient[s_class.job] * bl) + 1000)/100 * (100 + sd.paramc[3])/100 + (sd.parame[3] - sd.paramcard[3]);
	if (s_class.upper == 1) // [MouseJstr]
		sd.status.max_sp = sd.status.max_sp * 130/100;
	else if (s_class.upper==2)
		sd.status.max_sp = sd.status.max_sp * 70/100;
	
	if((skill=pc_checkskill(sd,HP_MEDITATIO))>0) // ���f�B�e�C�e�B�I
		sd.status.max_sp += sd.status.max_sp*skill/100;
	if((skill=pc_checkskill(sd,HW_SOULDRAIN))>0) /* �\�E���h���C�� */
		sd.status.max_sp += sd.status.max_sp*2*skill/100;
	if(sd.sc_data && sd.sc_data[SC_INCMSP2].timer!=-1) {
		sd.status.max_sp += sd.status.max_sp*sd.sc_data[SC_INCMSP2].val1/100;
	}

	if (sd.sprate <= 0)
		sd.sprate = 1;
	if(sd.sprate!=100)
		sd.status.max_sp = sd.status.max_sp*sd.sprate/100;
	if(sd.status.max_sp < 0 || sd.status.max_sp > (long)battle_config.max_sp)
		sd.status.max_sp = battle_config.max_sp;

	//���R��HP
	sd.nhealhp = 1 + (sd.paramc[2]/5) + (sd.status.max_hp/200);
	if((skill=pc_checkskill(sd,SM_RECOVERY)) > 0) {	/* HP�񕜗͌��� */
		sd.nshealhp = skill*5 + (sd.status.max_hp*skill/500);
		if(sd.nshealhp > 0x7fff) sd.nshealhp = 0x7fff;
	}
	//���R��SP
	sd.nhealsp = 1 + (sd.paramc[3]/6) + (sd.status.max_sp/100);
	if(sd.paramc[3] >= 120)
		sd.nhealsp += ((sd.paramc[3]-120)>>1) + 4;
	if((skill=pc_checkskill(sd,MG_SRECOVERY)) > 0) { /* SP�񕜗͌��� */
		sd.nshealsp = skill*3 + (sd.status.max_sp*skill/500);
		if(sd.nshealsp > 0x7fff) sd.nshealsp = 0x7fff;
	}
	if((skill = pc_checkskill(sd,MO_SPIRITSRECOVERY)) > 0) {
		sd.nsshealhp = skill*4 + (sd.status.max_hp*skill/500);
		sd.nsshealsp = skill*2 + (sd.status.max_sp*skill/500);
		if(sd.nsshealhp > 0x7fff) sd.nsshealhp = 0x7fff;
		if(sd.nsshealsp > 0x7fff) sd.nsshealsp = 0x7fff;
	}
	if((skill=pc_checkskill(sd,HP_MEDITATIO)) > 0) {
		// ���f�B�e�C�e�B�I��SPR�ł͂Ȃ����R�񕜂ɂ�����
		sd.nhealsp += (sd.nhealsp)*3*skill/100;
		if(sd.nhealsp > 0x7fff) sd.nhealsp = 0x7fff;
	}

	if(sd.hprecov_rate != 100) {
		sd.nhealhp = sd.nhealhp*sd.hprecov_rate/100;
		if(sd.nhealhp < 1) sd.nhealhp = 1;
		if(sd.nhealhp > 0x7fff) sd.nhealhp = 0x7fff;
	}
	if(sd.sprecov_rate != 100) {
		sd.nhealsp = sd.nhealsp*sd.sprecov_rate/100;
		if(sd.nhealsp < 1) sd.nhealsp = 1;
		if(sd.nhealsp > 0x7fff) sd.nhealsp = 0x7fff;
	}

	// �푰�ϐ��i����ł����́H �f�B�o�C���v���e�N�V�����Ɠ���?�������邩���j
	if( (skill=pc_checkskill(sd,SA_DRAGONOLOGY))>0 ){ // �h���S�m���W?
		skill = skill*4;
		sd.right_weapon.addrace[9]+=skill;
		sd.left_weapon.addrace[9]+=skill;
		sd.subrace[9]+=skill;
		sd.magic_addrace[9]+=skill;
		sd.magic_subrace[9]-=skill;
	}

	//Flee�㏸
	if( (skill=pc_checkskill(sd,TF_MISS))>0 ){	// ���?��
		sd.flee += skill*((sd.status.class_==12 || sd.status.class_==17 || sd.status.class_==4013 || sd.status.class_==4018) ? 4 : 3);
		if( (sd.status.class_==12 || sd.status.class_==4013) && sd.sc_data[SC_CLOAKING].timer==-1)
			sd.speed -= DEFAULT_WALK_SPEED * skill*3/2/100;
	}
	if( (skill=pc_checkskill(sd,MO_DODGE))>0 )	// ���؂�
		sd.flee += (skill*3)>>1;


	if( sd.sc_data[SC_INCFLEE].timer!=-1 )
		sd.flee += (unsigned short)sd.sc_data[SC_INCFLEE].val1;
	if( sd.sc_data[SC_INCFLEE2].timer!=-1 )
		sd.flee += (unsigned short)sd.sc_data[SC_INCFLEE2].val1;


	// �X�L����X�e?�^�X�ُ�ɂ��?��̃p����?�^�␳


	// ATK/DEF?���`
	if(sd.sc_data[SC_ANGELUS].timer!=-1)	// �G���W�F���X
		sd.def2 = sd.def2*(110+5*sd.sc_data[SC_ANGELUS].val1)/100;
	if(sd.sc_data[SC_IMPOSITIO].timer!=-1)	{// �C���|�V�e�B�I�}�k�X
		sd.right_weapon.watk += sd.sc_data[SC_IMPOSITIO].val1*5;
		index = sd.equip_index[8];
		if(index < MAX_INVENTORY && sd.inventory_data[index] && sd.inventory_data[index]->type == 4)
			sd.left_weapon.watk += sd.sc_data[SC_IMPOSITIO].val1*5;
	}
	if(sd.sc_data[SC_PROVOKE].timer!=-1){	// �v���{�b�N
		sd.def2 = sd.def2*(100-6*sd.sc_data[SC_PROVOKE].val1)/100;
		sd.base_atk = sd.base_atk*(100+2*sd.sc_data[SC_PROVOKE].val1)/100;
		sd.right_weapon.watk = sd.right_weapon.watk*(100+2*sd.sc_data[SC_PROVOKE].val1)/100;
		index = sd.equip_index[8];
		if(index < MAX_INVENTORY && sd.inventory_data[index] && sd.inventory_data[index]->type == 4)
			sd.left_weapon.watk = sd.left_weapon.watk*(100+2*sd.sc_data[SC_PROVOKE].val1)/100;
	}
	if(sd.sc_data[SC_ENDURE].timer!=-1)
		sd.mdef2 += (unsigned short)sd.sc_data[SC_ENDURE].val1;
	if(sd.sc_data[SC_MINDBREAKER].timer!=-1){	// �v���{�b�N
		sd.mdef2 = sd.mdef2*(100-6*sd.sc_data[SC_MINDBREAKER].val1)/100;
		sd.matk1 = sd.matk1*(100+2*sd.sc_data[SC_MINDBREAKER].val1)/100;
		sd.matk2 = sd.matk2*(100+2*sd.sc_data[SC_MINDBREAKER].val1)/100;
	}
	if(sd.sc_data[SC_INCMATK2].timer!=-1)	// �v���{�b�N
		sd.matk1 += sd.matk1*sd.sc_data[SC_INCMATK2].val1/100;
	
	if(sd.sc_data[SC_POISON].timer!=-1)	// ��?��
		sd.def2 = sd.def2*75/100;
	if(sd.sc_data[SC_CURSE].timer!=-1){
		sd.base_atk = sd.base_atk*75/100;
		sd.right_weapon.watk = sd.right_weapon.watk*75/100;
		index = sd.equip_index[8];
		if(index < MAX_INVENTORY && sd.inventory_data[index] && sd.inventory_data[index]->type == 4)
			sd.left_weapon.watk = sd.left_weapon.watk*75/100;
	}
	if(sd.sc_data[SC_DRUMBATTLE].timer!=-1){	// ?���ۂ̋���
		sd.right_weapon.watk += sd.sc_data[SC_DRUMBATTLE].val2;
		sd.def  += sd.sc_data[SC_DRUMBATTLE].val3;
		index = sd.equip_index[8];
		if(index < MAX_INVENTORY && sd.inventory_data[index] && sd.inventory_data[index]->type == 4)
			sd.left_weapon.watk += sd.sc_data[SC_DRUMBATTLE].val2;
	}
	if(sd.sc_data[SC_NIBELUNGEN].timer!=-1) {	// �j?�x�����O�̎w��
		index = sd.equip_index[9];
		if(index < MAX_INVENTORY && sd.inventory_data[index] && sd.inventory_data[index]->wlv == 4)
			sd.right_weapon.watk2 += sd.sc_data[SC_NIBELUNGEN].val3;
		index = sd.equip_index[8];
		if(index < MAX_INVENTORY && sd.inventory_data[index] && sd.inventory_data[index]->wlv == 4)
			sd.left_weapon.watk2 += sd.sc_data[SC_NIBELUNGEN].val3;
	}

	if(sd.sc_data[SC_VOLCANO].timer != -1 && sd.def_ele == 3)	// �{���P?�m
		sd.right_weapon.watk += sd.sc_data[SC_VOLCANO].val3;
	if(sd.sc_data[SC_INCATK2].timer != -1)
		sd.right_weapon.watk += sd.right_weapon.watk * sd.sc_data[SC_INCATK2].val1 / 100;
	if(sd.sc_data[SC_SIGNUMCRUCIS].timer != -1)
		sd.def = sd.def * (100 - sd.sc_data[SC_SIGNUMCRUCIS].val2)/100;
	if(sd.sc_data[SC_ETERNALCHAOS].timer != -1)	// �G�^?�i���J�I�X
		sd.def = 0;

	if(sd.sc_data[SC_CONCENTRATION].timer!=-1){ //�R���Z���g��?�V����
		sd.right_weapon.watk = sd.right_weapon.watk * (100 + 5*sd.sc_data[SC_CONCENTRATION].val1)/100;
		index = sd.equip_index[8];
		if(index < MAX_INVENTORY && sd.inventory_data[index] && sd.inventory_data[index]->type == 4)
			sd.left_weapon.watk = sd.left_weapon.watk * (100 + 5*sd.sc_data[SC_CONCENTRATION].val1)/100;
		sd.def = sd.def * (100 - 10*sd.sc_data[SC_CONCENTRATION].val1)/100;
	}

	if(sd.sc_data[SC_MAGICPOWER].timer!=-1){ //���@��?��
		sd.matk1 = sd.matk1*(100+5*sd.sc_data[SC_MAGICPOWER].val1)/100;
		sd.matk2 = sd.matk2*(100+5*sd.sc_data[SC_MAGICPOWER].val1)/100;
	}
	if(sd.sc_data[SC_ATKPOT].timer!=-1)
		sd.right_weapon.watk += sd.sc_data[SC_ATKPOT].val1;
	if(sd.sc_data[SC_MATKPOT].timer!=-1){
		sd.matk1 += (unsigned short)sd.sc_data[SC_MATKPOT].val1;
		sd.matk2 += (unsigned short)sd.sc_data[SC_MATKPOT].val1;
	}

	// ASPD/�ړ����x?���n
	if(sd.sc_data[SC_TWOHANDQUICKEN].timer != -1 && sd.sc_data[SC_QUAGMIRE].timer == -1 && sd.sc_data[SC_DONTFORGETME].timer == -1)	// 2HQ
		aspd_rate -= 30;
	if(sd.sc_data[SC_ADRENALINE].timer != -1 && sd.sc_data[SC_TWOHANDQUICKEN].timer == -1 &&
		sd.sc_data[SC_QUAGMIRE].timer == -1 && sd.sc_data[SC_DONTFORGETME].timer == -1) {	// �A�h���i�������b�V��
		if(sd.sc_data[SC_ADRENALINE].val2 || !battle_config.party_skill_penalty)
			aspd_rate -= 30;
		else
			aspd_rate -= 25;
	}
	if(sd.sc_data[SC_SPEARSQUICKEN].timer != -1 && sd.sc_data[SC_ADRENALINE].timer == -1 &&
		sd.sc_data[SC_TWOHANDQUICKEN].timer == -1 && sd.sc_data[SC_QUAGMIRE].timer == -1 && sd.sc_data[SC_DONTFORGETME].timer == -1)	// �X�s�A�N�B�b�P��
		aspd_rate -= sd.sc_data[SC_SPEARSQUICKEN].val2;
	if(sd.sc_data[SC_ASSNCROS].timer!=-1 && // �[�z�̃A�T�V���N���X
		sd.sc_data[SC_TWOHANDQUICKEN].timer==-1 && sd.sc_data[SC_ADRENALINE].timer==-1 && sd.sc_data[SC_SPEARSQUICKEN].timer==-1 &&
		sd.sc_data[SC_DONTFORGETME].timer == -1)
			aspd_rate -= 5+sd.sc_data[SC_ASSNCROS].val1+sd.sc_data[SC_ASSNCROS].val2+sd.sc_data[SC_ASSNCROS].val3;
	if(sd.sc_data[SC_DONTFORGETME].timer!=-1){		// ����Y��Ȃ���
		aspd_rate += sd.sc_data[SC_DONTFORGETME].val1*3 + sd.sc_data[SC_DONTFORGETME].val2 + (sd.sc_data[SC_DONTFORGETME].val3>>16);
		sd.speed_rate += sd.sc_data[SC_DONTFORGETME].val1*2 + sd.sc_data[SC_DONTFORGETME].val2 + (sd.sc_data[SC_DONTFORGETME].val3&0xffff);
	}
	if(	sd.sc_data[i=SC_SPEEDPOTION3].timer!=-1 ||
		sd.sc_data[i=SC_SPEEDPOTION2].timer!=-1 ||
		sd.sc_data[i=SC_SPEEDPOTION1].timer!=-1 ||
		sd.sc_data[i=SC_SPEEDPOTION0].timer!=-1)	// ? ���|?�V����
		aspd_rate -= sd.sc_data[i].val2;
	if(sd.sc_data[SC_GRAVITATION].timer!=-1)
		aspd_rate += sd.sc_data[SC_GRAVITATION].val2;
	if(sd.sc_data[SC_WINDWALK].timer!=-1 && sd.sc_data[SC_INCREASEAGI].timer==-1)	//�E�B���h�E�H?�N�b�Lv*2%���Z
		sd.speed_rate -= sd.sc_data[SC_WINDWALK].val1*2;
	if(sd.sc_data[SC_CARTBOOST].timer!=-1)	// �J?�g�u?�X�g
		sd.speed_rate -= 20;
	if(sd.sc_data[SC_BERSERK].timer!=-1)	//�o?�T?�N����IA�Ɠ������炢�����H
		sd.speed_rate -= 25;
	if(sd.sc_data[SC_WEDDING].timer!=-1)	//��������?���̂�?��
		sd.speed_rate += 100;

	// HIT/FLEE?���n
	if(sd.sc_data[SC_WHISTLE].timer!=-1){  // ���J
		sd.flee += sd.flee * (sd.sc_data[SC_WHISTLE].val1
				+sd.sc_data[SC_WHISTLE].val2+(sd.sc_data[SC_WHISTLE].val3>>16))/100;
		sd.flee2+= (sd.sc_data[SC_WHISTLE].val1+sd.sc_data[SC_WHISTLE].val2+(sd.sc_data[SC_WHISTLE].val3&0xffff)) * 10;
	}
	if(sd.sc_data[SC_HUMMING].timer!=-1)  // �n�~���O
		sd.hit += (sd.sc_data[SC_HUMMING].val1*2+sd.sc_data[SC_HUMMING].val2
				+sd.sc_data[SC_HUMMING].val3) * sd.hit/100;
	if(sd.sc_data[SC_VIOLENTGALE].timer != -1 && sd.def_ele == 4)	// �o�C�I�����g�Q�C��
		sd.flee += sd.flee * sd.sc_data[SC_VIOLENTGALE].val3 / 100;
	if(sd.sc_data[SC_BLIND].timer != -1)
	{	// ��?
		sd.hit -= sd.hit * 25 / 100;
		sd.flee -= sd.flee * 25 / 100;
	}
	if(sd.sc_data[SC_WINDWALK].timer != -1)
	{	// �E�B���h�E�H?�N
		sd.flee += sd.flee * sd.sc_data[SC_WINDWALK].val2 / 100;
	}
	if(sd.sc_data[SC_SPIDERWEB].timer != -1) //�X�p�C�_?�E�F�u
	{
		sd.flee = sd.flee * 50 / 100;
	}
	if(sd.sc_data[SC_TRUESIGHT].timer != -1) //�g�D��?�T�C�g
		sd.hit += 3 * sd.sc_data[SC_TRUESIGHT].val1;
	if(sd.sc_data[SC_CONCENTRATION].timer != -1) //�R���Z���g��?�V����
		sd.hit += sd.hit * 10 * sd.sc_data[SC_CONCENTRATION].val1 / 100;
	if(sd.sc_data[SC_INCHIT].timer != -1)
		sd.hit += (unsigned short)sd.sc_data[SC_INCHIT].val1;
	if(sd.sc_data[SC_INCHIT2].timer != -1)
		sd.hit += sd.hit * sd.sc_data[SC_INCHIT2].val1 / 100;

	// �ϐ�
	if(sd.sc_data[SC_SIEGFRIED].timer!=-1){  // �s���g�̃W?�N�t��?�h
		sd.subele[1] += sd.sc_data[SC_SIEGFRIED].val2;	// ��
		sd.subele[2] += sd.sc_data[SC_SIEGFRIED].val2;	// ��
		sd.subele[3] += sd.sc_data[SC_SIEGFRIED].val2;	// ��
		sd.subele[4] += sd.sc_data[SC_SIEGFRIED].val2;	// ��
		sd.subele[5] += sd.sc_data[SC_SIEGFRIED].val2;	// ��
		sd.subele[6] += sd.sc_data[SC_SIEGFRIED].val2;	// ��
		sd.subele[7] += sd.sc_data[SC_SIEGFRIED].val2;	// ��
		sd.subele[8] += sd.sc_data[SC_SIEGFRIED].val2;	// ��
		sd.subele[9] += sd.sc_data[SC_SIEGFRIED].val2;	// ��
	}
	if(sd.sc_data[SC_PROVIDENCE].timer!=-1){	// �v�����B�f���X
		sd.subele[6] += sd.sc_data[SC_PROVIDENCE].val2;	// ? ��?��
		sd.subrace[6] += sd.sc_data[SC_PROVIDENCE].val2;	// ? ?��
	}

	// ���̑�
	if(sd.sc_data[SC_APPLEIDUN].timer!=-1){	// �C�h�D���̗ь�
		sd.status.max_hp +=
				(5 + sd.sc_data[SC_APPLEIDUN].val1 * 2 + sd.sc_data[SC_APPLEIDUN].val2
				+ sd.sc_data[SC_APPLEIDUN].val3 / 10) * sd.status.max_hp / 100;
		if(sd.status.max_hp < 0 || sd.status.max_hp > (long)battle_config.max_hp)
			sd.status.max_hp = battle_config.max_hp;
	}
	if(sd.sc_data[SC_DELUGE].timer!=-1 && sd.def_ele==1){	// �f����?�W
		sd.status.max_hp += sd.status.max_hp * deluge_eff[sd.sc_data[SC_DELUGE].val1-1]/100;
		if(sd.status.max_hp < 0 || sd.status.max_hp > (long)battle_config.max_hp)
			sd.status.max_hp = battle_config.max_hp;
	}
	if(sd.sc_data[SC_SERVICE4U].timer!=-1) {	// �T?�r�X�t�H?��?
		sd.status.max_sp += sd.status.max_sp*(10+sd.sc_data[SC_SERVICE4U].val1+sd.sc_data[SC_SERVICE4U].val2
					+sd.sc_data[SC_SERVICE4U].val3)/100;
		if(sd.status.max_sp < 0 || sd.status.max_sp > (long)battle_config.max_sp)
			sd.status.max_sp = battle_config.max_sp;
		sd.dsprate-=(10+sd.sc_data[SC_SERVICE4U].val1*3+sd.sc_data[SC_SERVICE4U].val2
				+sd.sc_data[SC_SERVICE4U].val3);			
	}

	if(sd.sc_data[SC_FORTUNE].timer!=-1)	// �K�^�̃L�X
		sd.critical += (10+sd.sc_data[SC_FORTUNE].val1+sd.sc_data[SC_FORTUNE].val2
					+sd.sc_data[SC_FORTUNE].val3)*10;

	if(sd.sc_data[SC_EXPLOSIONSPIRITS].timer!=-1){	// �����g��
		if(s_class.job==23)
			sd.critical += sd.sc_data[SC_EXPLOSIONSPIRITS].val1*100;
		else
		sd.critical += sd.sc_data[SC_EXPLOSIONSPIRITS].val2;
	}

	if(sd.sc_data[SC_STEELBODY].timer!=-1){	// ����
		sd.def = 90;
		sd.mdef = 90;
		aspd_rate += 25;
		sd.speed_rate += 25;
	}
	if(sd.sc_data[SC_DEFENDER].timer != -1) {
		aspd_rate += 25 - sd.sc_data[SC_DEFENDER].val1*5;
		sd.speed += 55 - sd.sc_data[SC_DEFENDER].val1*5;
	}
	if(sd.sc_data[SC_ENCPOISON].timer != -1)
		sd.addeff[4] += sd.sc_data[SC_ENCPOISON].val2;

	if( sd.sc_data[SC_DANCING].timer!=-1 ){		// ���t/�_���X�g�p��
		int s_rate = 600 - 40 * pc_checkskill(sd,((s_class.job == 19) ? BA_MUSICALLESSON : DC_DANCINGLESSON));
		if (sd.sc_data[SC_LONGING].timer != -1)
			s_rate -= 20 * sd.sc_data[SC_LONGING].val1;
		sd.speed = sd.speed * s_rate / 100;
		// is attack speed affected?
		//aspd_rate = 600 - 40 * pc_checkskill(*sd, ((s_class.job == 19) ? BA_MUSICALLESSON : DC_DANCINGLESSON));
		//if (sd.sc_data[SC_LONGING].timer != -1)
		//	aspd_rate -= 20 * sd.sc_data[SC_LONGING].val1;
		//sd.speed*=4;
		sd.nhealsp = 0;
		sd.nshealsp = 0;
		sd.nsshealsp = 0;
	}
	if(sd.sc_data[SC_CURSE].timer!=-1)
		sd.speed += 450;

	if(sd.sc_data[SC_TRUESIGHT].timer!=-1) //�g�D��?�T�C�g
		//sd.critical += sd.critical*(sd.sc_data[SC_TRUESIGHT].val1)/100;
		sd.critical += sd.sc_data[SC_TRUESIGHT].val1; // not +10% CRIT but +CRIT!! [Lupus] u can see it in any RO calc stats

	if(sd.sc_data[SC_BERSERK].timer!=-1) {	//All Def/MDef reduced to 0 while in Berserk [DracoRPG]
		sd.def = sd.def2 = 0;
		sd.mdef = sd.mdef2 = 0;
		sd.flee -= sd.flee*50/100;
		aspd_rate -= 30;
	}
	if(sd.sc_data[SC_INCDEF2].timer!=-1)
		sd.def += sd.def * sd.sc_data[SC_INCDEF2].val1/100;
	if(sd.sc_data[SC_KEEPING].timer!=-1)
		sd.def = 100;
	if(sd.sc_data[SC_BARRIER].timer!=-1)
		sd.mdef = 100;

	if(sd.sc_data[SC_JOINTBEAT].timer!=-1)
	{	// Random break [DracoRPG]
		switch(sd.sc_data[SC_JOINTBEAT].val2) {
		case 0: //Ankle break
			sd.speed_rate += 50;
			break;
		case 1:	//Wrist	break
			sd.speed_rate += 25;
			break;
		case 2:	//Knee break
			sd.speed_rate += 30;
			sd.aspd_rate += 10;
			break;
		case 3:	//Shoulder break
			sd.def2 -= sd.def2*50/100;
			break;
		case 4:	//Waist	break
			sd.def2 -= sd.def2*50/100;
			sd.base_atk -= sd.base_atk*25/100;
			break;
		}
	}
	if(sd.sc_data[SC_GOSPEL].timer!=-1) {
		if (sd.sc_data[SC_GOSPEL].val4 == BCT_PARTY){
			switch (sd.sc_data[SC_GOSPEL].val3)
			{
			case 4:
				sd.status.max_hp += sd.status.max_hp * 25 / 100;
				if(sd.status.max_hp > (long)battle_config.max_hp)
					sd.status.max_hp = battle_config.max_hp;
				break;
			case 5:
				sd.status.max_sp += sd.status.max_sp * 25 / 100;
				if(sd.status.max_sp > (long)battle_config.max_sp)
					sd.status.max_sp = battle_config.max_sp;
				break;
			case 11:
				sd.def += sd.def * 25 / 100;
				sd.def2 += sd.def2 * 25 / 100;
				break;
			case 12:
				sd.base_atk += sd.base_atk * 8 / 100;
				break;
			case 13:
				sd.flee += sd.flee * 5 / 100;
				break;
			case 14:
				sd.hit += sd.hit * 5 / 100;
				break;
			}
		} else if (sd.sc_data[SC_GOSPEL].val4 == BCT_ENEMY){
			switch (sd.sc_data[SC_GOSPEL].val3)
			{
				case 5:
					sd.def = 0;
					sd.def2 = 0;
					break;
				case 6:
					sd.base_atk = 0;
					sd.right_weapon.watk = 0;
					sd.right_weapon.watk2 = 0;
					break;
				case 7:
					sd.flee = 0;
					break;
				case 8:
					sd.speed_rate += 75;
					aspd_rate += 75;
					break;
			}
		}
	}

	if (sd.speed_rate <= 0)
		sd.speed_rate = 1;

	if(sd.speed_rate != 100)
		sd.speed = sd.speed*sd.speed_rate/100;

	if(sd.skilltimer != -1 && (skill = pc_checkskill(sd,SA_FREECAST)) > 0) {
		sd.speed = sd.speed*(175 - skill*5)/100;
	}

	if(sd.speed < MAX_WALK_SPEED/battle_config.max_walk_speed)
		sd.speed = MAX_WALK_SPEED/battle_config.max_walk_speed;

	if(pc_isriding(sd))
		sd.aspd += sd.aspd * 10*(5 - pc_checkskill(sd,KN_CAVALIERMASTERY))/100;
	if(aspd_rate != 100)
		sd.aspd = sd.aspd*aspd_rate/100;
	if(pc_isriding(sd))							// �R���C��
		sd.aspd = sd.aspd*(100 + 10*(5 - pc_checkskill(sd,KN_CAVALIERMASTERY)))/ 100;
	if(sd.aspd < battle_config.max_aspd_interval) sd.aspd = battle_config.max_aspd_interval;
	sd.amotion = sd.aspd;
	if( sd.paramc[1] < 100 )
		sd.dmotion = 800-sd.paramc[1]*4;
	else
		sd.dmotion = 400;
	if(sd.dsprate < 0)
		sd.dsprate = 0;
	if(sd.status.hp>sd.status.max_hp)
		sd.status.hp=sd.status.max_hp;
	if(sd.status.sp>sd.status.max_sp)
		sd.status.sp=sd.status.max_sp;

	if(first&4)
		return 0;
	if(first&3) {
		clif_updatestatus(sd,SP_SPEED);
		clif_updatestatus(sd,SP_MAXHP);
		clif_updatestatus(sd,SP_MAXSP);
		if(first&1) {
			clif_updatestatus(sd,SP_HP);
			clif_updatestatus(sd,SP_SP);
		}
		return 0;
	}

	if(b_class != sd.view_class) {
		clif_changelook(sd.bl,LOOK_BASE,sd.view_class);
#if PACKETVER < 4
		clif_changelook(sd.bl,LOOK_WEAPON,sd.status.weapon);
		clif_changelook(sd.bl,LOOK_SHIELD,sd.status.shield);
#else
		clif_changelook(sd.bl,LOOK_WEAPON,0);
#endif
	//Restoring cloth dye color after the view class changes. [Skotlex]
	if(battle_config.save_clothcolor && sd.status.clothes_color > 0 &&
		(sd.view_class != 22 || !battle_config.wedding_ignorepalette))
			clif_changelook(sd.bl,LOOK_CLOTHES_COLOR,sd.status.clothes_color);
	}

	if( memcmp(b_skill,sd.status.skill,sizeof(sd.status.skill)) || b_attackrange != sd.attackrange)
		clif_skillinfoblock(sd);	// �X�L�����M

	if(b_speed != sd.speed)
		clif_updatestatus(sd,SP_SPEED);
	if(b_weight != (int)sd.weight)
		clif_updatestatus(sd,SP_WEIGHT);
	if(b_max_weight != (int)sd.max_weight) {
		clif_updatestatus(sd,SP_MAXWEIGHT);
		pc_checkweighticon(sd);
	}
	for(i=0;i<6;i++)
		if(b_paramb[i] + b_parame[i] != sd.paramb[i] + sd.parame[i])
			clif_updatestatus(sd,SP_STR+i);
	if(b_hit != sd.hit)
		clif_updatestatus(sd,SP_HIT);
	if(b_flee != sd.flee)
		clif_updatestatus(sd,SP_FLEE1);
	if(b_aspd != (long)sd.aspd)
		clif_updatestatus(sd,SP_ASPD);
	if(b_watk != sd.right_weapon.watk || b_base_atk != sd.base_atk)
		clif_updatestatus(sd,SP_ATK1);
	if(b_def != sd.def)
		clif_updatestatus(sd,SP_DEF1);
	if(b_watk2 != sd.right_weapon.watk2)
		clif_updatestatus(sd,SP_ATK2);
	if(b_def2 != sd.def2)
		clif_updatestatus(sd,SP_DEF2);
	if(b_flee2 != sd.flee2)
		clif_updatestatus(sd,SP_FLEE2);
	if(b_critical != sd.critical)
		clif_updatestatus(sd,SP_CRITICAL);
	if(b_matk1 != sd.matk1)
		clif_updatestatus(sd,SP_MATK1);
	if(b_matk2 != sd.matk2)
		clif_updatestatus(sd,SP_MATK2);
	if(b_mdef != sd.mdef)
		clif_updatestatus(sd,SP_MDEF1);
	if(b_mdef2 != sd.mdef2)
		clif_updatestatus(sd,SP_MDEF2);
	if(b_attackrange != sd.attackrange)
		clif_updatestatus(sd,SP_ATTACKRANGE);
	if(b_max_hp != sd.status.max_hp)
		clif_updatestatus(sd,SP_MAXHP);
	if(b_max_sp != sd.status.max_sp)
		clif_updatestatus(sd,SP_MAXSP);
	if(b_hp != sd.status.hp)
		clif_updatestatus(sd,SP_HP);
	if(b_sp != sd.status.sp)
		clif_updatestatus(sd,SP_SP);

	//if(sd.status.hp<sd.status.max_hp>>2 && pc_checkskill(*sd,SM_AUTOBERSERK)>0 &&
	if(sd.status.hp<sd.status.max_hp>>2 && sd.sc_data[SC_AUTOBERSERK].timer != -1 &&
		(sd.sc_data[SC_PROVOKE].timer==-1 || sd.sc_data[SC_PROVOKE].val2==0 ) && !pc_isdead(sd))
		// �I?�g�o?�T?�N?��
		status_change_start(&sd.bl,SC_PROVOKE,10,1,0,0,0,0);

	return 0;
}

/*==========================================
 * For quick calculating [Celest]
 *------------------------------------------
 */
int status_calc_speed_old (struct map_session_data &sd)
{
	int b_speed, skill;
	struct pc_base_job s_class;

	s_class = pc_calc_base_job(sd.status.class_);

	b_speed = sd.speed;
	sd.speed = DEFAULT_WALK_SPEED ;


	if(sd.sc_data[SC_INCREASEAGI].timer!=-1 && sd.sc_data[SC_QUAGMIRE].timer == -1 && sd.sc_data[SC_DONTFORGETME].timer == -1){	// ���x?��
		sd.speed -= sd.speed *25/100;
		}
	if(sd.sc_data[SC_DECREASEAGI].timer!=-1) {
		sd.speed = sd.speed *125/100;
		}
	if(sd.sc_data[SC_CLOAKING].timer!=-1) {
		sd.speed = sd.speed * (sd.sc_data[SC_CLOAKING].val3-sd.sc_data[SC_CLOAKING].val1*3) /100;
		}
	if(sd.sc_data[SC_CHASEWALK].timer!=-1) {
		sd.speed = sd.speed * sd.sc_data[SC_CHASEWALK].val3 /100;
		}
	if(sd.sc_data[SC_QUAGMIRE].timer!=-1){
		sd.speed = sd.speed*3/2;
		}
	if(sd.sc_data[SC_WINDWALK].timer!=-1 && sd.sc_data[SC_INCREASEAGI].timer==-1) {
		sd.speed -= sd.speed *(sd.sc_data[SC_WINDWALK].val1*2)/100;
		}
	if(sd.sc_data[SC_CARTBOOST].timer!=-1) {
		sd.speed -= (DEFAULT_WALK_SPEED * 20)/100;
		}
	if(sd.sc_data[SC_BERSERK].timer!=-1) {
		sd.speed -= sd.speed *25/100;
		}
	if(sd.sc_data[SC_WEDDING].timer!=-1) {
		sd.speed = 2*DEFAULT_WALK_SPEED;
		}
	if(sd.sc_data[SC_DONTFORGETME].timer!=-1){
		sd.speed= sd.speed*(100+sd.sc_data[SC_DONTFORGETME].val1*2 + sd.sc_data[SC_DONTFORGETME].val2 + (sd.sc_data[SC_DONTFORGETME].val3&0xffff))/100;
		}
	if(sd.sc_data[SC_STEELBODY].timer!=-1){
		sd.speed = (sd.speed * 125) / 100;
		}
	if(sd.sc_data[SC_DEFENDER].timer != -1) {
		sd.speed = (sd.speed * (155 - sd.sc_data[SC_DEFENDER].val1*5)) / 100;
		}
	if( sd.sc_data[SC_DANCING].timer!=-1 ){
			int s_rate = 600 - 40 * pc_checkskill(sd, ((s_class.job == 19) ? BA_MUSICALLESSON : DC_DANCINGLESSON));
		if (sd.sc_data[SC_LONGING].timer != -1)
			s_rate -= 20 * sd.sc_data[SC_LONGING].val1;
		sd.speed = sd.speed * s_rate / 100;			
		}
	if(sd.sc_data[SC_CURSE].timer!=-1)
		sd.speed += 450;
	if(sd.sc_data[SC_SLOWDOWN].timer!=-1)
		sd.speed = sd.speed*150/100;
	if(sd.sc_data[SC_SPEEDUP0].timer!=-1)
		sd.speed -= sd.speed*25/100;


	if(sd.status.option&2 && (skill = pc_checkskill(sd,RG_TUNNELDRIVE))>0 )
		sd.speed += (100-16*skill)*DEFAULT_WALK_SPEED/100;
	if (pc_iscarton(sd) && (skill=pc_checkskill(sd,MC_PUSHCART))>0)
		sd.speed += (10-skill) * DEFAULT_WALK_SPEED/10;
	else if (pc_isriding(sd)) {
		sd.speed -= DEFAULT_WALK_SPEED/4;
	}
	if((skill=pc_checkskill(sd,TF_MISS))>0)
		if(s_class.job==12)
			sd.speed -= sd.speed *(skill*3/2)/100;

	if(sd.speed_rate != 100)
		sd.speed = sd.speed*sd.speed_rate/100;
	if(sd.speed < DEFAULT_WALK_SPEED/4)
		sd.speed = DEFAULT_WALK_SPEED/4;

	if(sd.skilltimer != -1 && (skill = pc_checkskill(sd,SA_FREECAST)) > 0) {
		sd.speed = sd.speed*(175 - skill*5)/100;
	}

	if(b_speed != sd.speed)
		clif_updatestatus(sd,SP_SPEED);

	return 0;
}
/*==========================================
 * For quick calculating [Celest] Adapted by [Skotlex]
 *------------------------------------------
 */
int status_calc_speed(struct map_session_data &sd, unsigned short skill_num, unsigned short skill_lv, int start)
{
	// [Skotlex]
	// This function individually changes a character's speed upon a skill change and restores it upon it's ending.
	// Should only be used on non-inclusive skills to avoid exploits.
	// Currently used for freedom of cast
	// and when cloaking changes it's val3 (in which case the new val3 value comes in the level.
	
	int b_speed = sd.speed;
	
	switch (skill_num)
	{
	case SA_FREECAST:
	{
		int val = 175 - skill_lv*5;
		if (start) // do round integers properly
			sd.speed = (sd.speed*val + 100/2)/100;
		else//stop
			sd.speed = (sd.speed*100 + val/2)/val;
printf("speed set to %i (%i)\n",sd.speed, val);

		break;
	}
	case AS_CLOAKING:
		if (start && sd.sc_data[SC_CLOAKING].timer != -1)
		{	//There shouldn't be an "stop" case here.
			//If the previous upgrade was 
			//SPEED_ADD_RATE(3*sd->sc_data[SC_CLOAKING].val1 -sd->sc_data[SC_CLOAKING].val3);
			//Then just changing val3 should be a net difference of....
			if (3*sd.sc_data[SC_CLOAKING].val1 != sd.sc_data[SC_CLOAKING].val3)	//This reverts the previous value.
				sd.speed = sd.speed * 100 /(sd.sc_data[SC_CLOAKING].val3-3*sd.sc_data[SC_CLOAKING].val1);
			sd.sc_data[SC_CLOAKING].val3 = skill_lv;
				sd.speed = sd.speed * (sd.sc_data[SC_CLOAKING].val3-sd.sc_data[SC_CLOAKING].val1*3) /100;
		}
		break;
	}

	if(sd.speed < MAX_WALK_SPEED/battle_config.max_walk_speed)
		sd.speed = MAX_WALK_SPEED/battle_config.max_walk_speed;

	if(b_speed != sd.speed)
		clif_updatestatus(sd,SP_SPEED);

	return 0;
}
/*==========================================
 * �Ώۂ�Class��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_class(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return ((struct mob_data *)bl)->class_;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->status.class_;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return ((struct pet_data *)bl)->class_;
	else
		return 0;
}
/*==========================================
 * �Ώۂ̕�����Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_dir(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return ((struct mob_data *)bl)->dir;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->dir;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return ((struct pet_data *)bl)->dir;
	else
		return 0;
}
int status_get_headdir(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return ((struct mob_data *)bl)->dir;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->head_dir;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return ((struct pet_data *)bl)->dir;
	else
		return 0;
}
/*==========================================
 * �Ώۂ̃��x����Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_lv(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return ((struct mob_data *)bl)->level;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->status.base_level;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return ((struct pet_data *)bl)->msd->pet.level;
	else
		return 0;
}

/*==========================================
 * �Ώۂ̎˒���Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_range(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return mob_db[((struct mob_data *)bl)->class_].range;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->attackrange;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return mob_db[((struct pet_data *)bl)->class_].range;
	else
		return 0;
}
/*==========================================
 * �Ώۂ�HP��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_hp(struct block_list *bl)
{
	nullpo_retr(1, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return ((struct mob_data *)bl)->hp;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->status.hp;
	else
		return 1;
}
/*==========================================
 * �Ώۂ�MHP��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_max_hp(struct block_list *bl)
{
	nullpo_retr(1, bl);

	if(bl->type==BL_PC && ((struct map_session_data *)bl))
		return ((struct map_session_data *)bl)->status.max_hp;
	else {
		struct status_change *sc_data;		
		int max_hp = 1;

		if(bl->type == BL_MOB) {
			struct mob_data *md;
			nullpo_retr(1, md = (struct mob_data *)bl);
			max_hp = md->max_hp;

			if(battle_config.mobs_level_up) // mobs leveling up increase [Valaris]
				max_hp += (md->level - mob_db[md->class_].lv) * status_get_vit(bl);
			if(md->state.size==1) // change for sized monsters [Valaris]
					max_hp/=2;
			else if(md->state.size==2)
					max_hp*=2;

/* Moved this code to where the mob_db is read in mob.c [Skotlex]
			if(mob_db[md->class_].mexp > 0) { //MVP Monsters
				if(battle_config.mvp_hp_rate != 100) {
					int hp = max_hp * battle_config.mvp_hp_rate / 100;
					max_hp = (hp > 0x7FFFFFFF ? 0x7FFFFFFF : hp);
				}
			}
			else {	//Common MONSTERS
				if(battle_config.monster_hp_rate != 100) {
					int hp = max_hp * battle_config.monster_hp_rate / 100;
					max_hp = (hp > 0x7FFFFFFF ? 0x7FFFFFFF : hp);
				}
			}
*/
		}
		else if(bl->type == BL_PET) {
			struct pet_data *pd;
			nullpo_retr(1, pd = (struct pet_data*)bl);
			max_hp = mob_db[pd->class_].max_hp;
/* Moved this code to where the mob_db is read in mob.c [Skotlex]
			if(mob_db[pd->class_].mexp > 0) { //MVP Monsters 
				if(battle_config.mvp_hp_rate != 100)
					max_hp = (max_hp * battle_config.mvp_hp_rate)/100;
			}
			else {	//Common MONSTERS
				if(battle_config.monster_hp_rate != 100)
					max_hp = (max_hp * battle_config.monster_hp_rate)/100;
			}
*/
		}

		sc_data = status_get_sc_data(bl);
		if(sc_data) {
			if(sc_data[SC_APPLEIDUN].timer != -1)
				max_hp += ((5 + sc_data[SC_APPLEIDUN].val1 * 2 + ((sc_data[SC_APPLEIDUN].val2 + 1) >> 1)
					+ sc_data[SC_APPLEIDUN].val3 / 10) * max_hp)/100;
			if(sc_data[SC_GOSPEL].timer != -1 &&
				sc_data[SC_GOSPEL].val4 == BCT_PARTY &&
				sc_data[SC_GOSPEL].val3 == 4)
				max_hp += max_hp * 25 / 100;
			if(sc_data[SC_INCMHP2].timer!=-1)
				max_hp *= (100+ sc_data[SC_INCMHP2].val1)/100;
		}
		if(max_hp < 1) max_hp = 1;
		return max_hp;
	}
}
/*==========================================
 * �Ώۂ�Str��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_str(struct block_list *bl)
{
	int str = 0;
	nullpo_retr(0, bl);

	if (bl->type == BL_PC && ((struct map_session_data *)bl))
		return ((struct map_session_data *)bl)->paramc[0];
	else {		
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);

		if(bl->type == BL_MOB && ((struct mob_data *)bl)) {
			str = mob_db[((struct mob_data *)bl)->class_].str;
			if(battle_config.mobs_level_up) // mobs leveling up increase [Valaris]
				str += ((struct mob_data *)bl)->level - mob_db[((struct mob_data *)bl)->class_].lv;
			if(((struct mob_data*)bl)->state.size==1) // change for sized monsters [Valaris]
				str/=2;
			else if(((struct mob_data*)bl)->state.size==2)
				str*=2;
		}
		else if(bl->type == BL_PET && ((struct pet_data *)bl))
		{	//<Skotlex> Use pet's stats
			if (battle_config.pet_lv_rate && ((struct pet_data *)bl)->status)
				str = ((struct pet_data *)bl)->status->str;
			else
				str = mob_db[((struct pet_data *)bl)->class_].str;
		}
		if(sc_data) {
			if(sc_data[SC_LOUD].timer != -1)
				str += 4;
			if( sc_data[SC_BLESSING].timer != -1){	// �u���b�V���O
				int race = status_get_race(bl);
				if(battle_check_undead(race,status_get_elem_type(bl)) || race == 6)
					str >>= 1;	// �� ��/�s��
				else str += sc_data[SC_BLESSING].val1;	// ���̑�
			}
			if(sc_data[SC_TRUESIGHT].timer!=-1)	// �g�D���[�T�C�g
				str += 5;
			if(sc_data[SC_INCALLSTATUS].timer!=-1)
				str += sc_data[SC_INCALLSTATUS].val1;
			if(sc_data[SC_INCSTR].timer!=-1)
				str += sc_data[SC_INCSTR].val1;
		}
	}
	if(str < 0) str = 0;
	return str;
}
/*==========================================
 * �Ώۂ�Agi��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */

int status_get_agi(struct block_list *bl)
{
	int agi=0;
	nullpo_retr(0, bl);

	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->paramc[1];
	else {
		struct status_change *sc_data;	
		sc_data = status_get_sc_data(bl);
		if(bl->type == BL_MOB && (struct mob_data *)bl) {
			agi = mob_db[((struct mob_data *)bl)->class_].agi;
			if(battle_config.mobs_level_up) // increase of mobs leveling up [Valaris]
				agi += ((struct mob_data *)bl)->level - mob_db[((struct mob_data *)bl)->class_].lv;
			if(((struct mob_data*)bl)->state.size==1) // change for sized monsters [Valaris]
				agi/=2;
			else if(((struct mob_data*)bl)->state.size==2)
				agi*=2;
		}
		else if(bl->type == BL_PET && (struct pet_data *)bl)
		{	//<Skotlex> Use pet's stats
			if (battle_config.pet_lv_rate && ((struct pet_data *)bl)->status)
				agi = ((struct pet_data *)bl)->status->agi;
			else
				agi = mob_db[((struct pet_data *)bl)->class_].agi;
		}
		if(sc_data) {
			if(sc_data[SC_INCREASEAGI].timer!=-1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// ���x����(PC��pc.c��)
				agi += 2 + sc_data[SC_INCREASEAGI].val1;
			if(sc_data[SC_CONCENTRATE].timer!=-1 && sc_data[SC_QUAGMIRE].timer == -1)
				agi += agi * (2 + sc_data[SC_CONCENTRATE].val1)/100;
			if(sc_data[SC_DECREASEAGI].timer!=-1)	// ���x����
			{
				agi -= 2+sc_data[SC_DECREASEAGI].val1;
			}
			if(sc_data[SC_QUAGMIRE].timer!=-1 ) {	// �N�@�O�}�C�A
				//agi >>= 1;
				//int agib = agi*(sc_data[SC_QUAGMIRE].val1*10)/100;
				//agi -= agib > 50 ? 50 : agib;
				agi -= sc_data[SC_QUAGMIRE].val1*10;
			}
			if(sc_data[SC_TRUESIGHT].timer!=-1)	// �g�D���[�T�C�g
				agi += 5;
			if(sc_data[SC_INCALLSTATUS].timer!=-1)
				agi += sc_data[SC_INCALLSTATUS].val1;
			if(sc_data[SC_INCAGI].timer!=-1)
				agi += sc_data[SC_INCAGI].val1;
		}
	}
	if(agi < 0) agi = 0;
	return agi;
}
/*==========================================
 * �Ώۂ�Vit��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_vit(struct block_list *bl)
{
	int vit = 0;
	nullpo_retr(0, bl);

	if(bl->type == BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->paramc[2];
	else {
		struct status_change *sc_data;	
		sc_data = status_get_sc_data(bl);
		if(bl->type == BL_MOB && (struct mob_data *)bl) {
			vit = mob_db[((struct mob_data *)bl)->class_].vit;
			if(battle_config.mobs_level_up) // increase from mobs leveling up [Valaris]
				vit += ((struct mob_data *)bl)->level - mob_db[((struct mob_data *)bl)->class_].lv;
			if(((struct mob_data*)bl)->state.size==1) // change for sized monsters [Valaris]
				vit/=2;
			else if(((struct mob_data*)bl)->state.size==2)
				vit*=2;
		}	
		else if(bl->type == BL_PET && (struct pet_data *)bl)
		{	//<Skotlex> Use pet's stats
			if (battle_config.pet_lv_rate && ((struct pet_data *)bl)->status)
				vit = ((struct pet_data *)bl)->status->vit;
			else
				vit = mob_db[((struct pet_data *)bl)->class_].vit;
		}
		if(sc_data) {
			if(sc_data[SC_STRIPARMOR].timer != -1)
				vit = vit*60/100;
			if(sc_data[SC_TRUESIGHT].timer!=-1)	// �g�D���[�T�C�g
				vit += 5;
			if(sc_data[SC_INCALLSTATUS].timer!=-1)
				vit += sc_data[SC_INCALLSTATUS].val1;
		}
	}
	if(vit < 0) vit = 0;
	return vit;
}
/*==========================================
 * �Ώۂ�Int��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_int(struct block_list *bl)
{
	int int_=0;
	nullpo_retr(0, bl);

	if(bl->type == BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->paramc[3];
	else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);
		if(bl->type == BL_MOB && (struct mob_data *)bl){
			int_ = mob_db[((struct mob_data *)bl)->class_].int_;
			if(battle_config.mobs_level_up) // increase from mobs leveling up [Valaris]
				int_ += ((struct mob_data *)bl)->level - mob_db[((struct mob_data *)bl)->class_].lv;
			if(((struct mob_data*)bl)->state.size==1) // change for sized monsters [Valaris]
				int_/=2;
			else if(((struct mob_data*)bl)->state.size==2)
				int_*=2;
		}		
		else if(bl->type == BL_PET && (struct pet_data *)bl)
		{	//<Skotlex> Use pet's stats
			if (battle_config.pet_lv_rate && ((struct pet_data *)bl)->status)
				int_ = ((struct pet_data *)bl)->status->int_;
			else
				int_ = mob_db[((struct pet_data *)bl)->class_].int_;
		}

		if(sc_data) {
			if(sc_data[SC_BLESSING].timer != -1){	// �u���b�V���O
				int race = status_get_race(bl);
				if(battle_check_undead(race,status_get_elem_type(bl)) || race == 6 )
					int_ >>= 1;	// �� ��/�s��
				else
					int_ += sc_data[SC_BLESSING].val1;	// ���̑�
			}
			if(sc_data[SC_STRIPHELM].timer != -1)
				int_ = int_*60/100;
			if(sc_data[SC_TRUESIGHT].timer!=-1)	// �g�D���[�T�C�g
				int_ += 5;
			if(sc_data[SC_INCALLSTATUS].timer!=-1)
				int_ += sc_data[SC_INCALLSTATUS].val1;
		}
	}
	if(int_ < 0) int_ = 0;
	return int_;
}
/*==========================================
 * �Ώۂ�Dex��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_dex(struct block_list *bl)
{
	int dex = 0;
	nullpo_retr(0, bl);

	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->paramc[4];
	else {
		struct status_change *sc_data;	
		sc_data = status_get_sc_data(bl);
		if(bl->type == BL_MOB && (struct mob_data *)bl) {
			dex = mob_db[((struct mob_data *)bl)->class_].dex;
			if(battle_config.mobs_level_up) // increase from mobs leveling up [Valaris]
				dex += ((struct mob_data *)bl)->level - mob_db[((struct mob_data *)bl)->class_].lv;
			if(((struct mob_data*)bl)->state.size==1) // change for sized monsters [Valaris]
				dex/=2;
			else if(((struct mob_data*)bl)->state.size==2)
				dex*=2;
		}		
		else if(bl->type == BL_PET && (struct pet_data *)bl)
		{	//<Skotlex> Use pet's stats
			if (battle_config.pet_lv_rate && ((struct pet_data *)bl)->status)
				dex = ((struct pet_data *)bl)->status->dex;
			else
				dex = mob_db[((struct pet_data *)bl)->class_].dex;
		}

		if(sc_data) {
			if(sc_data[SC_CONCENTRATE].timer!=-1 && sc_data[SC_QUAGMIRE].timer == -1)
				dex += dex*(2+sc_data[SC_CONCENTRATE].val1)/100;
			if(sc_data[SC_BLESSING].timer != -1){	// �u���b�V���O
				int race = status_get_race(bl);
				if(battle_check_undead(race,status_get_elem_type(bl)) || race == 6 )
					dex >>= 1;	// �� ��/�s��
				else dex += sc_data[SC_BLESSING].val1;	// ���̑�
			}
			if(sc_data[SC_QUAGMIRE].timer!=-1)	{ // �N�@�O�}�C�A
				// dex >>= 1;
				//int dexb = dex*(sc_data[SC_QUAGMIRE].val1*10)/100;
				//dex -= dexb > 50 ? 50 : dexb;
				dex -= sc_data[SC_QUAGMIRE].val1*10;
			}
			if(sc_data[SC_TRUESIGHT].timer!=-1)	// �g�D���[�T�C�g
				dex += 5;
			if(sc_data[SC_INCALLSTATUS].timer!=-1)
				dex += sc_data[SC_INCALLSTATUS].val1;
		}
	}
	if(dex < 0) dex = 0;
	return dex;
}
/*==========================================
 * �Ώۂ�Luk��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_luk(struct block_list *bl)
{
	int luk = 0;
	nullpo_retr(0, bl);

	if(bl->type == BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->paramc[5];
	else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);
		if(bl->type == BL_MOB && (struct mob_data *)bl) {
			luk = mob_db[((struct mob_data *)bl)->class_].luk;
			if(battle_config.mobs_level_up) // increase from mobs leveling up [Valaris]
				luk += ((struct mob_data *)bl)->level - mob_db[((struct mob_data *)bl)->class_].lv;
			if(((struct mob_data*)bl)->state.size==1) // change for sized monsters [Valaris]
				luk/=2;
			else if(((struct mob_data*)bl)->state.size==2)
				luk*=2;
		}		
		else if(bl->type == BL_PET && (struct pet_data *)bl)
		{	//<Skotlex> Use pet's stats
			if (battle_config.pet_lv_rate && ((struct pet_data *)bl)->status)
				luk = ((struct pet_data *)bl)->status->luk;
			else
				luk = mob_db[((struct pet_data *)bl)->class_].luk;
		}
		if(sc_data) {
			if(sc_data[SC_GLORIA].timer!=-1)	// �O�����A(PC��pc.c��)
				luk += 30;
			if(sc_data[SC_TRUESIGHT].timer!=-1)	// �g�D���[�T�C�g
				luk += 5;
			if(sc_data[SC_CURSE].timer!=-1)		// ��
				luk = 0;
			if(sc_data[SC_INCALLSTATUS].timer!=-1)
				luk += sc_data[SC_INCALLSTATUS].val1;
		}
	}
	if(luk < 0) luk = 0;
	return luk;
}

/*==========================================
 * �Ώۂ�Flee��Ԃ�(�ėp)
 * �߂�͐�����1�ȏ�
 *------------------------------------------
 */
int status_get_flee(struct block_list *bl)
{
	int flee = 1;
	nullpo_retr(1, bl);

	if(bl->type == BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->flee;
	else {
		struct status_change *sc_data = status_get_sc_data(bl);
		flee = status_get_agi(bl) + status_get_lv(bl);

		if(sc_data){
			if(sc_data[SC_WHISTLE].timer!=-1)
				flee += flee*(sc_data[SC_WHISTLE].val1+sc_data[SC_WHISTLE].val2
					+(sc_data[SC_WHISTLE].val3>>16))/100;
			if(sc_data[SC_BLIND].timer!=-1)
				flee -= flee*25/100;
			if(sc_data[SC_WINDWALK].timer!=-1) // �E�B���h�E�H�[�N
				flee += flee*(sc_data[SC_WINDWALK].val2)/100;
			if(sc_data[SC_SPIDERWEB].timer!=-1) //�X�p�C�_�[�E�F�u
				flee -= flee*50/100;
			if(sc_data[SC_GOSPEL].timer!=-1) {
				if (sc_data[SC_GOSPEL].val4 == BCT_PARTY &&
					sc_data[SC_GOSPEL].val3 == 13)
					flee += flee*5/100;
				else if (sc_data[SC_GOSPEL].val4 == BCT_ENEMY &&
					sc_data[SC_GOSPEL].val3 == 7)
					flee = 0;
			}
			if(sc_data[SC_INCFLEE].timer!=-1)
				flee += flee * sc_data[SC_INCFLEE].val1 / 100;
		}
	}
	if(flee < 1) flee = 1;
	return flee;
}
/*==========================================
 * �Ώۂ�Hit��Ԃ�(�ėp)
 * �߂�͐�����1�ȏ�
 *------------------------------------------
 */
int status_get_hit(struct block_list *bl)
{
	int hit = 1;
	nullpo_retr(1, bl);
	if (bl->type == BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->hit;
	else {
		struct status_change *sc_data = status_get_sc_data(bl);
		hit = status_get_dex(bl) + status_get_lv(bl);

		if (sc_data) {
			if (sc_data[SC_HUMMING].timer != -1)
				hit += hit * (sc_data[SC_HUMMING].val1 * 2 + sc_data[SC_HUMMING].val2
					+ sc_data[SC_HUMMING].val3) / 100;
			if (sc_data[SC_BLIND].timer != -1)	// ��
				hit -= hit * 25 / 100;
			if (sc_data[SC_TRUESIGHT].timer != -1)	// �g�D���[�T�C�g
				hit += 3 * sc_data[SC_TRUESIGHT].val1;
			if (sc_data[SC_CONCENTRATION].timer != -1) //�R���Z���g���[�V����
				hit += hit * 10 * sc_data[SC_CONCENTRATION].val1 / 100;
			if (sc_data[SC_GOSPEL].timer != -1 &&
				sc_data[SC_GOSPEL].val4 == BCT_PARTY &&
				sc_data[SC_GOSPEL].val3 == 14)
				hit += hit * 5 / 100;
			if (sc_data[SC_EXPLOSIONSPIRITS].timer != -1)
				hit += 20 * sc_data[SC_EXPLOSIONSPIRITS].val1;
			if (sc_data[SC_INCHIT].timer != -1)
				hit += hit * sc_data[SC_INCHIT].val1 / 100;
		}
	}
	if(hit < 1) hit = 1;
	return hit;
}
/*==========================================
 * �Ώۂ̊��S�����Ԃ�(�ėp)
 * �߂�͐�����1�ȏ�
 *------------------------------------------
 */
int status_get_flee2(struct block_list *bl)
{
	int flee2 = 1;
	nullpo_retr(1, bl);

	if( bl->type == BL_PC && (struct map_session_data *)bl){
		return ((struct map_session_data *)bl)->flee2;
	} else {
		struct status_change *sc_data = status_get_sc_data(bl);
		flee2 = status_get_luk(bl)+1;

		if (sc_data) {
			if (sc_data[SC_WHISTLE].timer!=-1)
				flee2 += (sc_data[SC_WHISTLE].val1 + sc_data[SC_WHISTLE].val2
					+ (sc_data[SC_WHISTLE].val3&0xffff)) * 10;
		}
	}
	if (flee2 < 1) flee2 = 1;
	return flee2;
}
/*==========================================
 * �Ώۂ̃N���e�B�J����Ԃ�(�ėp)
 * �߂�͐�����1�ȏ�
 *------------------------------------------
 */
int status_get_critical(struct block_list *bl)
{
	int critical = 1;
	nullpo_retr(1, bl);

	if (bl->type == BL_PC && (struct map_session_data *)bl) {
		return ((struct map_session_data *)bl)->critical;
	} else {
		struct status_change *sc_data = status_get_sc_data(bl);
		critical = status_get_luk(bl)*3 + 1;

		if(sc_data) {
			if (sc_data[SC_FORTUNE].timer != -1)
				critical += 10 + sc_data[SC_FORTUNE].val1 + sc_data[SC_FORTUNE].val2
					+ sc_data[SC_FORTUNE].val3 * 10;
			if (sc_data[SC_EXPLOSIONSPIRITS].timer != -1)
				critical += sc_data[SC_EXPLOSIONSPIRITS].val2;
			if (sc_data[SC_TRUESIGHT].timer != -1) //�g�D���[�T�C�g
				critical += critical*sc_data[SC_TRUESIGHT].val1 / 100;
		}
	}
	if (critical < 1) critical = 1;
	return critical;
}
/*==========================================
 * base_atk�̎擾
 * �߂�͐�����1�ȏ�
 *------------------------------------------
 */
int status_get_baseatk(struct block_list *bl)
{
	int batk = 1;
	nullpo_retr(1, bl);
	
	if(bl->type==BL_PC && (struct map_session_data *)bl) {
		batk = ((struct map_session_data *)bl)->base_atk; //�ݒ肳��Ă���base_atk
		if (((struct map_session_data *)bl)->status.weapon < 16)
			batk += ((struct map_session_data *)bl)->weapon_atk[((struct map_session_data *)bl)->status.weapon];
	} else { //����ȊO�Ȃ�
		struct status_change *sc_data;
		int str,dstr;
		str = status_get_str(bl); //STR
		dstr = str/10;
		batk = dstr*dstr + str; //base_atk���v�Z����
		sc_data = status_get_sc_data(bl);

		if(sc_data) { //��Ԉُ킠��
			if(sc_data[SC_PROVOKE].timer!=-1) //PC�Ńv���{�b�N(SM_PROVOKE)���
				batk = batk*(100+2*sc_data[SC_PROVOKE].val1)/100; //base_atk����
			if(sc_data[SC_CURSE].timer!=-1) //����Ă�����
				batk -= batk*25/100; //base_atk��25%����
			if(sc_data[SC_CONCENTRATION].timer!=-1) //�R���Z���g���[�V����
				batk += batk*(5*sc_data[SC_CONCENTRATION].val1)/100;
			if(sc_data[SC_INCATK2].timer!=-1)
				batk *= (100+ sc_data[SC_INCATK2].val1)/100;
		}
	}
	if(batk < 1) batk = 1; //base_atk�͍Œ�ł�1
	return batk;
}
/*==========================================
 * �Ώۂ�Atk��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_atk(struct block_list *bl)
{
	int atk = 0;
	nullpo_retr(0, bl);

	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data*)bl)->right_weapon.watk;
	else {
		struct status_change *sc_data;
		sc_data=status_get_sc_data(bl);
		
		if(bl->type == BL_MOB && (struct mob_data *)bl)
			atk = mob_db[((struct mob_data*)bl)->class_].atk1;
		else if(bl->type == BL_PET && (struct pet_data *)bl)
		{	//<Skotlex> Use pet's stats
			if (battle_config.pet_lv_rate && ((struct pet_data *)bl)->status)
				atk = ((struct pet_data *)bl)->status->atk1;
			else
				atk = mob_db[((struct pet_data*)bl)->class_].atk1;
		}
		if(sc_data) {
			if(sc_data[SC_PROVOKE].timer!=-1)
				atk = atk*(100+2*sc_data[SC_PROVOKE].val1)/100;
			if(sc_data[SC_CURSE].timer!=-1)
				atk -= atk*25/100;
			if(sc_data[SC_CONCENTRATION].timer!=-1) //�R���Z���g���[�V����
				atk += atk*(5*sc_data[SC_CONCENTRATION].val1)/100;
			if(sc_data[SC_EXPLOSIONSPIRITS].timer!=-1)
				atk += (1000*sc_data[SC_EXPLOSIONSPIRITS].val1);
			if(sc_data[SC_STRIPWEAPON].timer!=-1)
				atk -= atk*10/100;

			if(sc_data[SC_GOSPEL].timer!=-1) {
				if (sc_data[SC_GOSPEL].val4 == BCT_PARTY &&
					sc_data[SC_GOSPEL].val3 == 12)
					atk += atk*8/100;
				else if (sc_data[SC_GOSPEL].val4 == BCT_ENEMY &&
					sc_data[SC_GOSPEL].val3 == 6)
					atk = 0;
			}
			if(sc_data[SC_INCATK2].timer!=-1)
				atk += atk * sc_data[SC_INCATK2].val1 / 100;
		}
	}
	if(atk < 0) atk = 0;
	return atk;
}
/*==========================================
 * �Ώۂ̍���Atk��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_atk_(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl){
		int atk=((struct map_session_data*)bl)->left_weapon.watk;
		return atk;
	}
	else
		return 0;
}
/*==========================================
 * �Ώۂ�Atk2��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_atk2(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data*)bl)->right_weapon.watk2;
	else {
		struct status_change *sc_data=status_get_sc_data(bl);
		int atk2=0;
		if(bl->type==BL_MOB && (struct mob_data *)bl)
			atk2 = mob_db[((struct mob_data*)bl)->class_].atk2;
		else if(bl->type==BL_PET && (struct pet_data *)bl)
		{	//<Skotlex> Use pet's stats
			if (battle_config.pet_lv_rate && ((struct pet_data *)bl)->status)
				atk2 = ((struct pet_data *)bl)->status->atk2;
			else
				atk2 = mob_db[((struct pet_data*)bl)->class_].atk2;
		}		  
		if(sc_data) {
			if( sc_data[SC_IMPOSITIO].timer!=-1)
				atk2 += sc_data[SC_IMPOSITIO].val1*5;
			if( sc_data[SC_PROVOKE].timer!=-1 )
				atk2 = atk2*(100+2*sc_data[SC_PROVOKE].val1)/100;
			if( sc_data[SC_CURSE].timer!=-1 )
				atk2 -= atk2*25/100;
			if(sc_data[SC_DRUMBATTLE].timer!=-1)
				atk2 += sc_data[SC_DRUMBATTLE].val2;
			if(sc_data[SC_NIBELUNGEN].timer!=-1 && (status_get_element(bl)/10) >= 8 )
				atk2 += sc_data[SC_NIBELUNGEN].val3;
			if(sc_data[SC_STRIPWEAPON].timer!=-1)
				atk2 = atk2*sc_data[SC_STRIPWEAPON].val2/100;
			if(sc_data[SC_CONCENTRATION].timer!=-1) //�R���Z���g���[�V����
				atk2 += atk2*(5*sc_data[SC_CONCENTRATION].val1)/100;
			if(sc_data[SC_EXPLOSIONSPIRITS].timer!=-1)
				atk2 += (1000*sc_data[SC_EXPLOSIONSPIRITS].val1);
		}
		if(atk2 < 0) atk2 = 0;
		return atk2;
	}
}
/*==========================================
 * �Ώۂ̍���Atk2��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_atk_2(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data*)bl)->left_weapon.watk2;
	else
		return 0;
}
/*==========================================
 * �Ώۂ�MAtk1��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_matk1(struct block_list *bl)
{
	int matk = 0;
	nullpo_retr(0, bl);

	if(bl->type == BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->matk1;
	else {
		struct status_change *sc_data;
		int int_ = status_get_int(bl);
		matk = int_+(int_/5)*(int_/5);

		sc_data = status_get_sc_data(bl);
		if(sc_data) {
			if(sc_data[SC_MINDBREAKER].timer!=-1)
				matk = matk*(100+2*sc_data[SC_MINDBREAKER].val1)/100;
			if(sc_data[SC_INCMATK2].timer!=-1)
				matk += matk * sc_data[SC_INCMATK2].val1 / 100;
		}
	}
	return matk;
}
/*==========================================
 * �Ώۂ�MAtk2��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_matk2(struct block_list *bl)
{
	int matk = 0;
	nullpo_retr(0, bl);

	if(bl->type == BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->matk2;
	else {
		struct status_change *sc_data = status_get_sc_data(bl);
		int int_ = status_get_int(bl);
		matk = int_+(int_/7)*(int_/7);

		if(sc_data) {
			if(sc_data[SC_MINDBREAKER].timer!=-1)
				matk = matk*(100+2*sc_data[SC_MINDBREAKER].val1)/100;
			if(sc_data[SC_INCMATK2].timer!=-1)
				matk += matk * sc_data[SC_INCMATK2].val1 / 100;
		}
	}
	return matk;
}
/*==========================================
 * �Ώۂ�Def��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_def(struct block_list *bl)
{
	struct status_change *sc_data;
	int def=0,skilltimer=-1,skillid=0;

	nullpo_retr(0, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl){
		def = ((struct map_session_data *)bl)->def;
		skilltimer = ((struct map_session_data *)bl)->skilltimer;
		skillid = ((struct map_session_data *)bl)->skillid;
	}
	else if(bl->type==BL_MOB && (struct mob_data *)bl) {
		def = mob_db[((struct mob_data *)bl)->class_].def;
		skilltimer = ((struct mob_data *)bl)->skilltimer;
		skillid = ((struct mob_data *)bl)->skillid;
	}
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		def = mob_db[((struct pet_data *)bl)->class_].def;

	if(def < 1000000) {
		if(sc_data) {
			//�����A�Ή����͉E�V�t�g
			if(sc_data[SC_FREEZE].timer != -1 || (sc_data[SC_STONE].timer != -1 && sc_data[SC_STONE].val2 == 0))
				def >>= 1;

			if (bl->type != BL_PC) {
				//�L�[�s���O����DEF100
				if( sc_data[SC_KEEPING].timer!=-1)
					def = 100;
				//�v���{�b�N���͌��Z
				if( sc_data[SC_PROVOKE].timer!=-1)
					def = (def*(100 - 6*sc_data[SC_PROVOKE].val1)+50)/100;
				//�푾�ۂ̋������͉��Z
				if( sc_data[SC_DRUMBATTLE].timer!=-1)
					def += sc_data[SC_DRUMBATTLE].val3;
				//�łɂ������Ă��鎞�͌��Z
				if(sc_data[SC_POISON].timer!=-1)
					def = def*75/100;
				//�X�g���b�v�V�[���h���͌��Z
				if(sc_data[SC_STRIPSHIELD].timer!=-1)
					def = def*sc_data[SC_STRIPSHIELD].val2/100;
				//�V�O�i���N���V�X���͌��Z
				if(sc_data[SC_SIGNUMCRUCIS].timer!=-1)
					def = def * (100 - sc_data[SC_SIGNUMCRUCIS].val2)/100;
				//�i���̍��׎���DEF0�ɂȂ�
				if(sc_data[SC_ETERNALCHAOS].timer!=-1)
					def = 0;
				//�R���Z���g���[�V�������͌��Z
				if( sc_data[SC_CONCENTRATION].timer!=-1)
					def = (def*(100 - 10*sc_data[SC_CONCENTRATION].val1))/100;

				if(sc_data[SC_GOSPEL].timer!=-1) {
					if (sc_data[SC_GOSPEL].val4 == BCT_PARTY &&
						sc_data[SC_GOSPEL].val3 == 11)
						def += def*25/100;
					else if (sc_data[SC_GOSPEL].val4 == BCT_ENEMY &&
						sc_data[SC_GOSPEL].val3 == 5)
						def = 0;
				}
				if(sc_data[SC_JOINTBEAT].timer!=-1) {
					if (sc_data[SC_JOINTBEAT].val2 == 3)
						def -= def*50/100;
					else if (sc_data[SC_JOINTBEAT].val2 == 4)
						def -= def*25/100;
				}
				if(sc_data[SC_INCDEF2].timer!=-1)
					def += def * sc_data[SC_INCDEF2].val1 / 100;
			}
		}
		//�r�����͉r�������Z���Ɋ�Â��Č��Z
		if(skilltimer != -1) {
			int def_rate = skill_get_castdef(skillid);
			if(def_rate != 0)
				def = (def * (100 - def_rate))/100;
		}
	}
	if(def < 0) def = 0;
	return def;
}
/*==========================================
 * �Ώۂ�MDef��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_mdef(struct block_list *bl)
{
	struct status_change *sc_data;
	int mdef=0;

	nullpo_retr(0, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		mdef = ((struct map_session_data *)bl)->mdef;
	else if(bl->type==BL_MOB && (struct mob_data *)bl)
		mdef = mob_db[((struct mob_data *)bl)->class_].mdef;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		mdef = mob_db[((struct pet_data *)bl)->class_].mdef;

	if(mdef < 1000000) {
		if(sc_data) {
			//�o���A�[��Ԏ���MDEF100
			if(sc_data[SC_BARRIER].timer != -1)
				mdef = 100;
			//�����A�Ή�����1.25�{
			if(sc_data[SC_FREEZE].timer != -1 || (sc_data[SC_STONE].timer != -1 && sc_data[SC_STONE].val2 == 0))
				mdef += mdef/4; // == *1.25
			if( sc_data[SC_MINDBREAKER].timer!=-1 && bl->type != BL_PC)
				mdef -= (mdef*6*sc_data[SC_MINDBREAKER].val1)/100;
		}
	}
	if(mdef < 0) mdef = 0;
	return mdef;
}
/*==========================================
 * �Ώۂ�Def2��Ԃ�(�ėp)
 * �߂�͐�����1�ȏ�
 *------------------------------------------
 */
int status_get_def2(struct block_list *bl)
{
	int def2 = 1;
	nullpo_retr(1, bl);
	
	if(bl->type==BL_PC)
		return ((struct map_session_data *)bl)->def2;
	else {
		struct status_change *sc_data;

		if(bl->type==BL_MOB)
			def2 = mob_db[((struct mob_data *)bl)->class_].vit;
		else if(bl->type==BL_PET)
		{	//<Skotlex> Use pet's stats
			if (battle_config.pet_lv_rate && ((struct pet_data *)bl)->status)
				def2 = ((struct pet_data *)bl)->status->vit;
			else
				def2 = mob_db[((struct pet_data *)bl)->class_].vit;
		}
		sc_data = status_get_sc_data(bl);
		if(sc_data) {
			if(sc_data[SC_ANGELUS].timer != -1)
				def2 = def2*(110+5*sc_data[SC_ANGELUS].val1)/100;
			if(sc_data[SC_PROVOKE].timer!=-1)
				def2 = (def2*(100 - 6*sc_data[SC_PROVOKE].val1)+50)/100;
			if(sc_data[SC_POISON].timer!=-1)
				def2 = def2*75/100;
			//�R���Z���g���[�V�������͌��Z
			if( sc_data[SC_CONCENTRATION].timer!=-1)
				def2 = def2*(100 - 10*sc_data[SC_CONCENTRATION].val1)/100;

			if(sc_data[SC_GOSPEL].timer!=-1) {
				if (sc_data[SC_GOSPEL].val4 == BCT_PARTY &&
					sc_data[SC_GOSPEL].val3 == 11)
					def2 += def2*25/100;
				else if (sc_data[SC_GOSPEL].val4 == BCT_ENEMY &&
					sc_data[SC_GOSPEL].val3 == 5)
					def2 = 0;
			}
		}
	}
	if(def2 < 1) def2 = 1;
	return def2;
}
/*==========================================
 * �Ώۂ�MDef2��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_mdef2(struct block_list *bl)
{
	int mdef2 = 0;
	nullpo_retr(0, bl);

	if(bl->type == BL_PC)
		return ((struct map_session_data *)bl)->mdef2 + (((struct map_session_data *)bl)->paramc[2]>>1);
	else {
		struct status_change *sc_data = status_get_sc_data(bl);
		if(bl->type == BL_MOB)
			mdef2 = mob_db[((struct mob_data *)bl)->class_].int_ + (mob_db[((struct mob_data *)bl)->class_].vit>>1);
		else if(bl->type == BL_PET)
		{	//<Skotlex> Use pet's stats
			if (battle_config.pet_lv_rate && ((struct pet_data *)bl)->status)
				mdef2 = ((struct pet_data *)bl)->status->int_ +(((struct pet_data *)bl)->status->vit>>1);
			else
				mdef2 = mob_db[((struct pet_data *)bl)->class_].int_ + (mob_db[((struct pet_data *)bl)->class_].vit>>1);
		}
		if(sc_data) {
			if(sc_data[SC_MINDBREAKER].timer!=-1)
				mdef2 -= (mdef2*6*sc_data[SC_MINDBREAKER].val1)/100;
		}
	}
	if(mdef2 < 0) mdef2 = 0;
		return mdef2;
}
/*==========================================
 * �Ώۂ�Speed(�ړ����x)��Ԃ�(�ėp)
 * �߂�͐�����1�ȏ�
 * Speed�͏������ق����ړ����x������
 *------------------------------------------
 */
int status_get_speed(struct block_list *bl)
{
	nullpo_retr(1000, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->speed;
	else {
		struct status_change *sc_data=status_get_sc_data(bl);
		int speed = 1000;
		if(bl->type==BL_MOB && (struct mob_data *)bl) {
			speed = ((struct mob_data *)bl)->speed;
			if(battle_config.mobs_level_up) // increase from mobs leveling up [Valaris]
				speed-=((struct mob_data *)bl)->level - mob_db[((struct mob_data *)bl)->class_].lv;
		}
		else if(bl->type==BL_PET && (struct pet_data *)bl) {
			speed = ((struct pet_data *)bl)->msd->petDB->speed;
		} else if(bl->type==BL_NPC && (struct npc_data *)bl)	//Added BL_NPC (Skotlex)
			speed = ((struct npc_data *)bl)->speed;

		if(sc_data) {
			//���x��������25%���Z
			if(sc_data[SC_INCREASEAGI].timer!=-1)
				speed -= speed*25/100;
			//�E�B���h�E�H�[�N����Lv*2%���Z
			else if(sc_data[SC_WINDWALK].timer!=-1)
				speed -= (speed*(sc_data[SC_WINDWALK].val1*2))/100;
			//���x��������25%���Z
			if(sc_data[SC_DECREASEAGI].timer!=-1)
				speed = speed*125/100;
			//�N�@�O�}�C�A����50%���Z
			if(sc_data[SC_QUAGMIRE].timer!=-1)
				speed = speed*3/2;
			//����Y��Ȃ��Łc���͉��Z
			if(sc_data[SC_DONTFORGETME].timer!=-1)
				speed = speed*(100+sc_data[SC_DONTFORGETME].val1*2 + sc_data[SC_DONTFORGETME].val2 + (sc_data[SC_DONTFORGETME].val3&0xffff))/100;
			//��������25%���Z
			if(sc_data[SC_STEELBODY].timer!=-1)
				speed = speed*125/100;
			//�f�B�t�F���_�[���͉��Z
			if(sc_data[SC_DEFENDER].timer!=-1)
				speed = (speed * (155 - sc_data[SC_DEFENDER].val1*5)) / 100;
			//�x���Ԃ�4�{�x��
			if(sc_data[SC_DANCING].timer!=-1 )
				speed *= 6;
			//�􂢎���450���Z
			if(sc_data[SC_CURSE].timer!=-1)
				speed = speed + 450;
			if(sc_data[SC_SLOWDOWN].timer!=-1)
				speed = speed*150/100;
			if(sc_data[SC_SPEEDUP0].timer!=-1)
				speed -= speed*25/100;
			if(sc_data[SC_GOSPEL].timer!=-1 &&
				sc_data[SC_GOSPEL].val4 == BCT_ENEMY &&
				sc_data[SC_GOSPEL].val3 == 8)
				speed = speed*125/100;
			if(sc_data[SC_JOINTBEAT].timer!=-1) {
				if (sc_data[SC_JOINTBEAT].val2 == 0)
					speed = speed*150/100;
				else if (sc_data[SC_JOINTBEAT].val2 == 2)
					speed = speed*130/100;				
			}
		}
		if(speed < 1) speed = 1;
		return speed;
	}
}


/*==========================================
 * �Ώۂ�aDelay(�U�����f�B���C)��Ԃ�(�ėp)
 * aDelay�͏������ق����U�����x������
 *------------------------------------------
 */
int status_get_adelay(struct block_list *bl)
{
	nullpo_retr(4000, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return (((struct map_session_data *)bl)->aspd<<1);
	else {
		struct status_change *sc_data=status_get_sc_data(bl);
		int adelay=4000,aspd_rate = 100,i;
		if(bl->type==BL_MOB && (struct mob_data *)bl)
			adelay = mob_db[((struct mob_data *)bl)->class_].adelay;
		else if(bl->type==BL_PET && (struct pet_data *)bl)
			adelay = mob_db[((struct pet_data *)bl)->class_].adelay;

		if(sc_data) {
			//�c�[�n���h�N�C�b�P���g�p���ŃN�@�O�}�C�A�ł�����Y��Ȃ��Łc�ł��Ȃ�����3�����Z
			if(sc_data[SC_TWOHANDQUICKEN].timer != -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// 2HQ
				aspd_rate -= 30;
			//�A�h���i�������b�V���g�p���Ńc�[�n���h�N�C�b�P���ł��N�@�O�}�C�A�ł�����Y��Ȃ��Łc�ł��Ȃ�����
			if(sc_data[SC_ADRENALINE].timer != -1 && sc_data[SC_TWOHANDQUICKEN].timer == -1 &&
				sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1) {	// �A�h���i�������b�V��
				//�g�p�҂ƃp�[�e�B�����o�[�Ŋi�����o��ݒ�łȂ����3�����Z
				if(sc_data[SC_ADRENALINE].val2 || !battle_config.party_skill_penalty)
					aspd_rate -= 30;
				//�����łȂ����2.5�����Z
				else
					aspd_rate -= 25;
			}
			//�X�s�A�N�B�b�P�����͌��Z
			if(sc_data[SC_SPEARSQUICKEN].timer != -1 && sc_data[SC_ADRENALINE].timer == -1 &&
				sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// �X�s�A�N�B�b�P��
				aspd_rate -= sc_data[SC_SPEARSQUICKEN].val2;
			//�[���̃A�T�V���N���X���͌��Z
			if(sc_data[SC_ASSNCROS].timer!=-1 && // �[�z�̃A�T�V���N���X
				sc_data[SC_TWOHANDQUICKEN].timer==-1 && sc_data[SC_ADRENALINE].timer==-1 && sc_data[SC_SPEARSQUICKEN].timer==-1 &&
				sc_data[SC_DONTFORGETME].timer == -1)
				aspd_rate -= 5+sc_data[SC_ASSNCROS].val1+sc_data[SC_ASSNCROS].val2+sc_data[SC_ASSNCROS].val3;
			//����Y��Ȃ��Łc���͉��Z
			if(sc_data[SC_DONTFORGETME].timer!=-1)		// ����Y��Ȃ���
				aspd_rate += sc_data[SC_DONTFORGETME].val1*3 + sc_data[SC_DONTFORGETME].val2 + (sc_data[SC_DONTFORGETME].val3>>16);
			//������25%���Z
			if(sc_data[SC_STEELBODY].timer!=-1)	// ����
				aspd_rate += 25;
			//�����|�[�V�����g�p���͌��Z
			if(	sc_data[i=SC_SPEEDPOTION3].timer!=-1 || sc_data[i=SC_SPEEDPOTION2].timer!=-1 || sc_data[i=SC_SPEEDPOTION1].timer!=-1 || sc_data[i=SC_SPEEDPOTION0].timer!=-1)
				aspd_rate -= sc_data[i].val2;
			//�f�B�t�F���_�[���͉��Z
			if(sc_data[SC_DEFENDER].timer != -1)
				aspd_rate += (25 - sc_data[SC_DEFENDER].val1*5);
				//adelay += (1100 - sc_data[SC_DEFENDER].val1*100);
			if(sc_data[SC_GOSPEL].timer!=-1 &&
				sc_data[SC_GOSPEL].val4 == BCT_ENEMY &&
				sc_data[SC_GOSPEL].val3 == 8)
				aspd_rate += 25;
			if(sc_data[SC_JOINTBEAT].timer!=-1) {
				if (sc_data[SC_JOINTBEAT].val2 == 1)
					aspd_rate += 25;
				else if (sc_data[SC_JOINTBEAT].val2 == 2)
					aspd_rate += 10;
			}
			if(sc_data[SC_GRAVITATION].timer!=-1)
				aspd_rate += sc_data[SC_GRAVITATION].val2;
		}
		if(aspd_rate != 100)
			adelay = adelay*aspd_rate/100;
		if(adelay < 2*(int)battle_config.monster_max_aspd_interval) adelay = 2*battle_config.monster_max_aspd_interval;
		return adelay;
	}
}

int status_get_amotion(struct block_list *bl)
{
	nullpo_retr(2000, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->amotion;
	else {
		struct status_change *sc_data=status_get_sc_data(bl);
		int amotion=2000,aspd_rate = 100,i;
		if(bl->type==BL_MOB && (struct mob_data *)bl)
			amotion = mob_db[((struct mob_data *)bl)->class_].amotion;
		else if(bl->type==BL_PET && (struct pet_data *)bl)
			amotion = mob_db[((struct pet_data *)bl)->class_].amotion;

		if(sc_data) {
			if(sc_data[SC_TWOHANDQUICKEN].timer != -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// 2HQ
				aspd_rate -= 30;
			if(sc_data[SC_ADRENALINE].timer != -1 && sc_data[SC_TWOHANDQUICKEN].timer == -1 &&
				sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1) {	// �A�h���i�������b�V��
				if(sc_data[SC_ADRENALINE].val2 || !battle_config.party_skill_penalty)
					aspd_rate -= 30;
				else
					aspd_rate -= 25;
			}
			if(sc_data[SC_SPEARSQUICKEN].timer != -1 && sc_data[SC_ADRENALINE].timer == -1 &&
				sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// �X�s�A�N�B�b�P��
				aspd_rate -= sc_data[SC_SPEARSQUICKEN].val2;
			if(sc_data[SC_ASSNCROS].timer!=-1 && // �[�z�̃A�T�V���N���X
				sc_data[SC_TWOHANDQUICKEN].timer==-1 && sc_data[SC_ADRENALINE].timer==-1 && sc_data[SC_SPEARSQUICKEN].timer==-1 &&
				sc_data[SC_DONTFORGETME].timer == -1)
				aspd_rate -= 5+sc_data[SC_ASSNCROS].val1+sc_data[SC_ASSNCROS].val2+sc_data[SC_ASSNCROS].val3;
			if(sc_data[SC_DONTFORGETME].timer!=-1)		// ����Y��Ȃ���
				aspd_rate += sc_data[SC_DONTFORGETME].val1*3 + sc_data[SC_DONTFORGETME].val2 + (sc_data[SC_DONTFORGETME].val3>>16);
			if(sc_data[SC_STEELBODY].timer!=-1)	// ����
				aspd_rate += 25;
			if(	sc_data[i=SC_SPEEDPOTION3].timer!=-1 || sc_data[i=SC_SPEEDPOTION2].timer!=-1 || sc_data[i=SC_SPEEDPOTION1].timer!=-1 || sc_data[i=SC_SPEEDPOTION0].timer!=-1)
				aspd_rate -= sc_data[i].val2;
			if(sc_data[SC_DEFENDER].timer != -1)
				aspd_rate += (25 - sc_data[SC_DEFENDER].val1*5);
				//amotion += (550 - sc_data[SC_DEFENDER].val1*50);
			if(sc_data[SC_GRAVITATION].timer!=-1)
				aspd_rate += sc_data[SC_GRAVITATION].val2;
		}
		if(aspd_rate != 100)
			amotion = amotion*aspd_rate/100;
		if(amotion < (int)battle_config.monster_max_aspd_interval) amotion = battle_config.monster_max_aspd_interval;
		return amotion;
	}
}


int status_get_dmotion(struct block_list *bl)
{
	int ret;
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	sc_data = status_get_sc_data(bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl){
		ret=mob_db[((struct mob_data *)bl)->class_].dmotion;
		if(battle_config.monster_damage_delay_rate != 100)
			ret = ret*battle_config.monster_damage_delay_rate/100;
	}
	else if(bl->type==BL_PC && (struct map_session_data *)bl){
		ret=((struct map_session_data *)bl)->dmotion;
		if(battle_config.pc_damage_delay_rate != 100)
			ret = ret*battle_config.pc_damage_delay_rate/100;
	}
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		ret=mob_db[((struct pet_data *)bl)->class_].dmotion;
	else
		return 2000;

	if((sc_data && (sc_data[SC_ENDURE].timer!=-1 || sc_data[SC_BERSERK].timer!=-1)) ||
		(bl->type == BL_PC && ((struct map_session_data *)bl)->state.infinite_endure))
		ret=0;

	//Let's apply a random damage modifier to prevent 'stun-lock' abusers. [Skotlex]
	ret = ret*(95+rand()%10)/100;	//Currently: +/- 5%
	return ret;
}

int status_get_element(struct block_list *bl)
{
	int ret = 20;
	struct status_change *sc_data;

	nullpo_retr(ret, bl);
	sc_data = status_get_sc_data(bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)	// 10�̈ʁ�Lv*2�A�P�̈ʁ�����
		ret=((struct mob_data *)bl)->def_ele;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		ret=20+((struct map_session_data *)bl)->def_ele;	// �h�䑮��Lv1
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		ret = mob_db[((struct pet_data *)bl)->class_].element;

	if(sc_data) {
		if( sc_data[SC_BENEDICTIO].timer!=-1 )	// ���̍~��
			ret=26;
		if( sc_data[SC_FREEZE].timer!=-1 )	// ����
			ret=21;
		if( sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2==0)
			ret=22;
	}

	return ret;
}

int status_get_attack_element(struct block_list *bl)
{
	int ret = 0;
	struct status_change *sc_data=status_get_sc_data(bl);

	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		ret=0;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		ret=((struct map_session_data *)bl)->right_weapon.atk_ele;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		ret=0;

	if(sc_data) {
		if( sc_data[SC_FROSTWEAPON].timer!=-1)	// �t���X�g�E�F�|��
			ret=1;
		if( sc_data[SC_SEISMICWEAPON].timer!=-1)	// �T�C�Y�~�b�N�E�F�|��
			ret=2;
		if( sc_data[SC_FLAMELAUNCHER].timer!=-1)	// �t���[�������`���[
			ret=3;
		if( sc_data[SC_LIGHTNINGLOADER].timer!=-1)	// ���C�g�j���O���[�_�[
			ret=4;
		if( sc_data[SC_ENCPOISON].timer!=-1)	// �G���`�����g�|�C�Y��
			ret=5;
		if( sc_data[SC_ASPERSIO].timer!=-1)		// �A�X�y���V�I
			ret=6;
	}

	return ret;
}
int status_get_attack_element2(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl) {
		int ret = ((struct map_session_data *)bl)->left_weapon.atk_ele;
		struct status_change *sc_data = ((struct map_session_data *)bl)->sc_data;

		if(sc_data) {
			if( sc_data[SC_FROSTWEAPON].timer!=-1)	// �t���X�g�E�F�|��
				ret=1;
			if( sc_data[SC_SEISMICWEAPON].timer!=-1)	// �T�C�Y�~�b�N�E�F�|��
				ret=2;
			if( sc_data[SC_FLAMELAUNCHER].timer!=-1)	// �t���[�������`���[
				ret=3;
			if( sc_data[SC_LIGHTNINGLOADER].timer!=-1)	// ���C�g�j���O���[�_�[
				ret=4;
			if( sc_data[SC_ENCPOISON].timer!=-1)	// �G���`�����g�|�C�Y��
				ret=5;
			if( sc_data[SC_ASPERSIO].timer!=-1)		// �A�X�y���V�I
				ret=6;
		}
		return ret;
	}
	return 0;
}

unsigned long status_get_party_id(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC)
		return ((struct map_session_data *)bl)->status.party_id;
	else if(bl->type==BL_MOB){
		struct mob_data *md=(struct mob_data *)bl;
		if( md->master_id>0 )
			return md->master_id & 0x80000000;
		return md->bl.id & 0x80000000;
	}
	else if(bl->type==BL_SKILL && (struct skill_unit *)bl)
		return ((struct skill_unit *)bl)->group->party_id;
	else
		return 0;
}
unsigned long status_get_guild_id(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->status.guild_id;
	else if(bl->type==BL_MOB && (struct mob_data *)bl)
		return ((struct mob_data *)bl)->class_;
	else if(bl->type==BL_SKILL && (struct skill_unit *)bl)
		return ((struct skill_unit *)bl)->group->guild_id;
	else
		return 0;
}
int status_get_race(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return mob_db[((struct mob_data *)bl)->class_].race;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return 7;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return mob_db[((struct pet_data *)bl)->class_].race;
	else
		return 0;
}
int status_get_size(struct block_list *bl)
{
	nullpo_retr(1, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return mob_db[((struct mob_data *)bl)->class_].size;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return mob_db[((struct pet_data *)bl)->class_].size;
	else if(bl->type==BL_PC)
	{
		struct map_session_data *sd = (struct map_session_data *)bl;

		//Baby Class Peco Rider + enabled option -> size = 1, else 0
		if (pc_calc_upper(sd->status.class_)==2)			
			return (pc_isriding(*sd)!=0 && battle_config.character_size&2); 
		
		//Peco Rider + enabled option -> size = 2, else 1
		return 1+(pc_isriding(*sd)!=0 && battle_config.character_size&1);
	} else
		return 1;
}
int status_get_mode(struct block_list *bl)
{
	nullpo_retr(0x01, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return mob_db[((struct mob_data *)bl)->class_].mode;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return mob_db[((struct pet_data *)bl)->class_].mode;
	else
		return 0x01;	// �Ƃ肠���������Ƃ������Ƃ�1
}

int status_get_mexp(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return mob_db[((struct mob_data *)bl)->class_].mexp;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return mob_db[((struct pet_data *)bl)->class_].mexp;
	else
		return 0;
}
int status_get_race2(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type == BL_MOB && (struct mob_data *)bl)
		return mob_db[((struct mob_data *)bl)->class_].race2;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return mob_db[((struct pet_data *)bl)->class_].race2;
	else
		return 0;
}
int status_isdead(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type == BL_MOB)
		return ((struct mob_data *)bl)->state.state == MS_DEAD;
	else if(bl->type==BL_PC)
		return pc_isdead(*((struct map_session_data *)bl));
	else
		return 0;
}
int status_isimmune(struct block_list *bl)
{
	struct map_session_data *sd = NULL;
	
	nullpo_retr(0, bl);
	if (bl->type == BL_PC && (sd = (struct map_session_data *)bl)) {
		if(sd->state.no_magic_damage)
			return 1;
		else if(sd->sc_data[SC_HERMODE].timer != -1)
			return 1;
	}	
	return 0;
}

// StatusChange�n�̏���
struct status_change *status_get_sc_data(struct block_list *bl)
{
	nullpo_retr(NULL, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return ((struct mob_data*)bl)->sc_data;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data*)bl)->sc_data;
	return NULL;
}

short *status_get_opt1(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return &((struct mob_data*)bl)->opt1;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return &((struct map_session_data*)bl)->opt1;
	else if(bl->type==BL_NPC && (struct npc_data *)bl)
		return &((struct npc_data*)bl)->opt1;
	return 0;
}
short *status_get_opt2(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return &((struct mob_data*)bl)->opt2;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return &((struct map_session_data*)bl)->opt2;
	else if(bl->type==BL_NPC && (struct npc_data *)bl)
		return &((struct npc_data*)bl)->opt2;
	return 0;
}
short *status_get_opt3(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return &((struct mob_data*)bl)->opt3;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return &((struct map_session_data*)bl)->opt3;
	else if(bl->type==BL_NPC && (struct npc_data *)bl)
		return &((struct npc_data*)bl)->opt3;
	return 0;
}
short *status_get_option(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return &((struct mob_data*)bl)->option;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return &((struct map_session_data*)bl)->status.option;
	else if(bl->type==BL_NPC && (struct npc_data *)bl)
		return &((struct npc_data*)bl)->option;
	return 0;
}

int status_get_sc_def(struct block_list *bl, int type)
{
	int sc_def;
	nullpo_retr(0, bl);
	
	switch (type)
	{
	case SP_MDEF1:	// mdef
		sc_def = 100 - (3 + status_get_mdef(bl) + status_get_luk(bl)/3);
		break;
	case SP_MDEF2:	// int
		sc_def = 100 - (3 + status_get_int(bl) + status_get_luk(bl)/3);
		break;
	case SP_DEF1:	// def
		sc_def = 100 - (3 + status_get_def(bl) + status_get_luk(bl)/3);
		break;
	case SP_DEF2:	// vit
		sc_def = 100 - (3 + status_get_vit(bl) + status_get_luk(bl)/3);
		break;
	case SP_LUK:	// luck
		sc_def = 100 - (3 + status_get_luk(bl));
		break;

	case SC_STONE:
	case SC_FREEZE:
		sc_def = 100 - (3 + status_get_mdef(bl) + status_get_luk(bl)/3);
		break;
	case SC_STAN:
	case SC_POISON:
	case SC_SILENCE:
		sc_def = 100 - (3 + status_get_vit(bl) + status_get_luk(bl)/3);
		break;	
	case SC_SLEEP:
	case SC_CONFUSION:
	case SC_BLIND:
		sc_def = 100 - (3 + status_get_int(bl) + status_get_luk(bl)/3);
		break;
	case SC_CURSE:
		sc_def = 100 - (3 + status_get_luk(bl));
		break;	

	default:
		sc_def = 100;
		break;
	}

	if(bl->type == BL_MOB) {
		struct mob_data *md = (struct mob_data *)bl;
		if (md && md->class_ == 1288)
			return 0;
		if (sc_def < 50)
			sc_def = 50;
	} else if(bl->type == BL_PC) {
		struct status_change* sc_data = status_get_sc_data(bl);
		if (sc_data && sc_data[SC_GOSPEL].timer != -1 &&
			sc_data[SC_GOSPEL].val4 == BCT_PARTY &&
			sc_data[SC_GOSPEL].val3 == 3)
			sc_def -= 25;
	}

	return (sc_def < 0) ? 0 : sc_def;
}

/*==========================================
 * �X�e�[�^�X�ُ�J�n
 *------------------------------------------
 */
int status_change_start(struct block_list *bl,int type,int val1,int val2,int val3,int val4,int tick,int flag)
{
	struct map_session_data *sd = NULL;
	struct status_change* sc_data;
	short*option, *opt1, *opt2, *opt3;
	int opt_flag = 0, calc_flag = 0,updateflag = 0, save_flag = 0, race, mode, elem, undead_flag;
	int scdef = 0;

	nullpo_retr(0, bl);
	if(bl->type == BL_SKILL)
		return 0;
	if(bl->type == BL_MOB)
		if (status_isdead(bl)) return 0;
	if(bl->type == BL_PET)	//Pets cannot have status effects
		return 0;

	nullpo_retr(0, sc_data=status_get_sc_data(bl));
	nullpo_retr(0, option=status_get_option(bl));
	nullpo_retr(0, opt1=status_get_opt1(bl));
	nullpo_retr(0, opt2=status_get_opt2(bl));
	nullpo_retr(0, opt3=status_get_opt3(bl));

	race=status_get_race(bl);
	mode=status_get_mode(bl);
	elem=status_get_elem_type(bl);
	undead_flag=battle_check_undead(race,elem);

	if(type == SC_AETERNA && (sc_data[SC_STONE].timer != -1 || sc_data[SC_FREEZE].timer != -1) )
		return 0;

	switch(type){
		case SC_STONE:
		case SC_FREEZE:
			scdef=3+status_get_mdef(bl)+status_get_luk(bl)/3;
			break;
		case SC_STAN:
		case SC_SILENCE:
		case SC_POISON:
		case SC_DPOISON:
			scdef=3+status_get_vit(bl)+status_get_luk(bl)/3;
			break;
		case SC_SLEEP:
		case SC_BLIND:
			scdef=3+status_get_int(bl)+status_get_luk(bl)/3;
			break;
		case SC_CURSE:
			scdef=3+status_get_luk(bl);
			break;
		default:
			scdef=0;
	}
	if(scdef>=100)
		return 0;
	if(bl->type==BL_PC){
		sd=(struct map_session_data *)bl;
		if( sd && type == SC_ADRENALINE && !(skill_get_weapontype(BS_ADRENALINE)&(1<<sd->status.weapon)))
			return 0;

		if(SC_STONE<=type && type<=SC_BLIND){	/* �J?�h�ɂ��ϐ� */
			if( sd && sd->reseff[type-SC_STONE] > 0 && rand()%10000<sd->reseff[type-SC_STONE]){
				if(battle_config.battle_log)
					ShowMessage("PC %d skill_sc_start: card�ɂ��ُ�ϐ�?��\n",sd->bl.id);
				return 0;
			}
		}
	}
	else if(bl->type == BL_MOB) {		
	}
	else {
		if(battle_config.error_log)
			ShowMessage("status_change_start: neither MOB nor PC !\n");
		return 0;
	}

	if((type==SC_FREEZE || type==SC_STONE) && undead_flag && !(flag&1))
	//I've been informed that undead chars are inmune to stone curse too. [Skotlex]
		return 0;
	
	
	if (type==SC_BLESSING && (bl->type==BL_PC || (!undead_flag && race!=6))) {
		if (sc_data[SC_CURSE].timer!=-1)
			status_change_end(bl,SC_CURSE,-1);
		if (sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2==0)
			status_change_end(bl,SC_STONE,-1);
	}

	if((type == SC_ADRENALINE || type == SC_WEAPONPERFECTION || type == SC_OVERTHRUST) &&
		sc_data[type].timer != -1 && sc_data[type].val2 && !val2)
		return 0;

	if(mode & 0x20 && (type==SC_STONE || type==SC_FREEZE ||
		type==SC_STAN || type==SC_SLEEP || type==SC_SILENCE || type==SC_QUAGMIRE || type == SC_DECREASEAGI || type == SC_SIGNUMCRUCIS || type == SC_PROVOKE ||
		(type == SC_BLESSING && (undead_flag || race == 6))) && !(flag&1)){
		/* �{�X�ɂ�?���Ȃ�(�������J?�h�ɂ��?�ʂ͓K�p�����) */
		return 0;
	}
	if(type==SC_FREEZE || type==SC_STAN || type==SC_SLEEP)
		battle_stopwalking(bl,1);

	if(sc_data[type].timer != -1)
	{	/* ���łɓ����ُ�ɂȂ��Ă���ꍇ�^�C�}���� */
		if(sc_data[type].val1 > val1 && type != SC_COMBO && type != SC_DANCING && type != SC_DEVOTION &&
			type != SC_SPEEDPOTION0 && type != SC_SPEEDPOTION1 && type != SC_SPEEDPOTION2 && type != SC_SPEEDPOTION3
			&& type != SC_ATKPOT && type != SC_MATKPOT) // added atk and matk potions [Valaris]
			return 0;

		if ((type >=SC_STAN && type <= SC_BLIND) || type == SC_DPOISON)
			return 0;/* ?���������ł��Ȃ�?�Ԉُ�ł��鎞��?�Ԉُ���s��Ȃ� */

		delete_timer(sc_data[type].timer, status_change_timer);
		sc_data[type].timer = -1;
	}

	// �N�A�O�}�C�A/����Y��Ȃ��Œ��͖����ȃX�L��
	if ((sc_data[SC_QUAGMIRE].timer!=-1 || sc_data[SC_DONTFORGETME].timer!=-1) &&
		(type==SC_CONCENTRATE || type==SC_INCREASEAGI ||
		type==SC_TWOHANDQUICKEN || type==SC_SPEARSQUICKEN ||
		type==SC_ADRENALINE || type==SC_TRUESIGHT ||
		type==SC_WINDWALK || type==SC_CARTBOOST || type==SC_ASSNCROS))
	return 0;

	switch(type){	/* �ُ�̎�ނ��Ƃ�?�� */
		case SC_PROVOKE:			/* �v���{�b�N */
			calc_flag = 1;
			if(tick <= 0) tick = 1000;	/* (�I?�g�o?�T?�N) */
			break;
		case SC_ENDURE:				/* �C���f���A */
			calc_flag = 1; // for updating mdef
			if(tick <= 0) tick = 1000 * 60;
			val2 = 7; // [Celest]
			break;
		case SC_AUTOBERSERK:
			{
				tick = 60*1000;
				if (bl->type == BL_PC && sd->status.hp<sd->status.max_hp>>2 &&
					(sc_data[SC_PROVOKE].timer==-1 || sc_data[SC_PROVOKE].val2==0))
					status_change_start(bl,SC_PROVOKE,10,1,0,0,0,0);
			}
			break;
		
		case SC_INCREASEAGI:		/* ���x�㏸ */
			calc_flag = 1;
			if(sc_data[SC_DECREASEAGI].timer!=-1 )
				status_change_end(bl,SC_DECREASEAGI,-1);
			// the effect will still remain [celest]
//			if(sc_data[SC_WINDWALK].timer!=-1 )	/* �E�C���h�E�H?�N */
//				status_change_end(bl,SC_WINDWALK,-1);
			break;
		case SC_DECREASEAGI:		/* ���x���� */
			if (bl->type == BL_PC)	// Celest
				tick>>=1;
			calc_flag = 1;
			if(sc_data[SC_INCREASEAGI].timer!=-1 )
				status_change_end(bl,SC_INCREASEAGI,-1);
			if(sc_data[SC_ADRENALINE].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE,-1);
			if(sc_data[SC_SPEARSQUICKEN].timer!=-1 )
				status_change_end(bl,SC_SPEARSQUICKEN,-1);
			if(sc_data[SC_TWOHANDQUICKEN].timer!=-1 )
				status_change_end(bl,SC_TWOHANDQUICKEN,-1);
			break;
		case SC_SIGNUMCRUCIS:		/* �V�O�i���N���V�X */
			calc_flag = 1;
			val2 = 10 + val1*2;
			tick = 600*1000;
			clif_emotion(*bl,4);
			break;
		case SC_SLOWPOISON:
			if (sc_data[SC_POISON].timer == -1 && sc_data[SC_DPOISON].timer == -1)
				return 0;
			break;
		case SC_TWOHANDQUICKEN:		/* 2HQ */
			if(sc_data[SC_DECREASEAGI].timer!=-1)
				return 0;
			*opt3 |= 1;
			calc_flag = 1;
			break;
		case SC_ADRENALINE:			/* �A�h���i�������b�V�� */
			if(sc_data[SC_DECREASEAGI].timer!=-1)
				return 0;
			if(bl->type == BL_PC)
				if(pc_checkskill(*sd,BS_HILTBINDING)>0)
					tick += tick / 10;
			calc_flag = 1;
			break;
		case SC_WEAPONPERFECTION:	/* �E�F�|���p?�t�F�N�V���� */
			if(bl->type == BL_PC)
				if(pc_checkskill(*sd,BS_HILTBINDING)>0)
					tick += tick / 10;
			break;
		case SC_OVERTHRUST:			/* �I?�o?�X���X�g */
			if(bl->type == BL_PC)
				if(pc_checkskill(*sd,BS_HILTBINDING)>0)
					tick += tick / 10;
			*opt3 |= 2;
			break;
		case SC_MAXIMIZEPOWER:		/* �}�L�V�}�C�Y�p��?(SP��1���鎞��,val2�ɂ�) */
			if(bl->type == BL_PC)
				val2 = tick;
			else
				tick = 5000*val1;
			break;
		case SC_ENCPOISON:			/* �G���`�����g�|�C�Y�� */
			calc_flag = 1;
			val2=(((val1 - 1) / 2) + 3)*100;	/* �ŕt?�m�� */
			skill_enchant_elemental_end(bl,SC_ENCPOISON);
			break;
		case SC_EDP:	// [Celest]
			val2 = val1 + 2;			/* �ғŕt?�m��(%) */
			calc_flag = 1;
			break;
		case SC_POISONREACT:	/* �|�C�Y�����A�N�g */
			val2=val1/2 + val1%2; // [Celest]
			break;
		case SC_ASPERSIO:			/* �A�X�y���V�I */
			skill_enchant_elemental_end(bl,SC_ASPERSIO);
			break;
		case SC_ENERGYCOAT:			/* �G�i�W?�R?�g */
			*opt3 |= 4;
			break;
		case SC_MAGICROD:
			val2 = val1*20;
			break;
		case SC_KYRIE:				/* �L���G�G���C�\�� */
			val2 = status_get_max_hp(bl) * (val1 * 2 + 10) / 100;/* �ϋv�x */
			val3 = (val1 / 2 + 5);	/* ��? */
// -- moonsoul (added to undo assumptio status if target has it)
			if(sc_data[SC_ASSUMPTIO].timer!=-1 )
				status_change_end(bl,SC_ASSUMPTIO,-1);
			break;
		case SC_MINDBREAKER:
			calc_flag = 1;
			if(tick <= 0) tick = 1000;	/* (�I?�g�o?�T?�N) */
		case SC_TRICKDEAD:			/* ���񂾂ӂ� */
			if (bl->type == BL_PC) {
				pc_stopattack(*sd);
			}
			break;
		case SC_QUAGMIRE:			/* �N�@�O�}�C�A */
			calc_flag = 1;
			if(sc_data[SC_CONCENTRATE].timer!=-1 )	/* �W���͌������ */
				status_change_end(bl,SC_CONCENTRATE,-1);
			if(sc_data[SC_INCREASEAGI].timer!=-1 )	/* ���x�㏸���� */
				status_change_end(bl,SC_INCREASEAGI,-1);
			if(sc_data[SC_TWOHANDQUICKEN].timer!=-1 )
				status_change_end(bl,SC_TWOHANDQUICKEN,-1);
			if(sc_data[SC_SPEARSQUICKEN].timer!=-1 )
				status_change_end(bl,SC_SPEARSQUICKEN,-1);
			if(sc_data[SC_ADRENALINE].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE,-1);
			if(sc_data[SC_TRUESIGHT].timer!=-1 )	/* �g�D��?�T�C�g */
				status_change_end(bl,SC_TRUESIGHT,-1);
			if(sc_data[SC_WINDWALK].timer!=-1 )	/* �E�C���h�E�H?�N */
				status_change_end(bl,SC_WINDWALK,-1);
			if(sc_data[SC_CARTBOOST].timer!=-1 )	/* �J?�g�u?�X�g */
				status_change_end(bl,SC_CARTBOOST,-1);
			break;
		case SC_MAGICPOWER:
			calc_flag = 1;
			val2 = 1;
			break;
		case SC_SACRIFICE:
			val2 = 5;
			break;
		case SC_FLAMELAUNCHER:		/* �t��?�������`��? */
			skill_enchant_elemental_end(bl,SC_FLAMELAUNCHER);
			break;
		case SC_FROSTWEAPON:		/* �t���X�g�E�F�|�� */
			skill_enchant_elemental_end(bl,SC_FROSTWEAPON);
			break;
		case SC_LIGHTNINGLOADER:	/* ���C�g�j���O��?�_? */
			skill_enchant_elemental_end(bl,SC_LIGHTNINGLOADER);
			break;
		case SC_SEISMICWEAPON:		/* �T�C�Y�~�b�N�E�F�|�� */
			skill_enchant_elemental_end(bl,SC_SEISMICWEAPON);
			break;
		case SC_PROVIDENCE:			/* �v�����B�f���X */
			calc_flag = 1;
			val2=val1*5;
			break;
		case SC_REFLECTSHIELD:
			val2=10+val1*3;
			break;
		case SC_STRIPWEAPON:
			if (val2==0) val2=90;
			break;
		case SC_STRIPSHIELD:
			if (val2==0) val2=85;
			break;

		case SC_AUTOSPELL:			/* �I?�g�X�y�� */
			val4 = 5 + val1*2;
			break;

		case SC_VOLCANO:
			calc_flag = 1;
			val3 = val1*10;
			break;
		case SC_DELUGE:
			calc_flag = 1;
			if (sc_data[SC_FOGWALL].timer != -1 && sc_data[SC_BLIND].timer != -1)
				status_change_end(bl,SC_BLIND,-1);
			break;
		case SC_VIOLENTGALE:
			calc_flag = 1;
			val3 = val1*3;
			break;

		case SC_SPEARSQUICKEN:		/* �X�s�A�N�C�b�P�� */
			calc_flag = 1;
			val2 = 20+val1;
			*opt3 |= 1;
			break;

		case SC_BLADESTOP:		/* ���n��� */
			if(val2==2) clif_bladestop(*((struct block_list *)val3),*((struct block_list *)val4),1);
			*opt3 |= 32;
			break;

		case SC_LULLABY:			/* �q��S */
			val2 = 11;
			break;
		case SC_RICHMANKIM:
			break;
		case SC_ETERNALCHAOS:		/* �G�^?�i���J�I�X */
			calc_flag = 1;
			break;
		case SC_DRUMBATTLE:			/* ?���ۂ̋��� */
			calc_flag = 1;
			val2 = (val1+1)*25;
			val3 = (val1+1)*2;
			break;
		case SC_NIBELUNGEN:			/* �j?�x�����O�̎w�� */
			calc_flag = 1;
			//val2 = (val1+2)*50;
			val3 = (val1+2)*25;
			break;
		case SC_ROKISWEIL:			/* ���L�̋��� */
			break;
		case SC_INTOABYSS:			/* �[���̒��� */
			break;
		case SC_SIEGFRIED:			/* �s���g�̃W?�N�t��?�h */
			calc_flag = 1;
			val2 = 55 + val1*5;
			val3 = val1*10;
			break;
		case SC_DISSONANCE:			/* �s���a�� */
			val2 = 10;
			break;
		case SC_WHISTLE:			/* ���J */
			calc_flag = 1;
			break;
		case SC_ASSNCROS:			/* �[�z�̃A�T�V���N���X */
			calc_flag = 1;
			break;
		case SC_POEMBRAGI:			/* �u���M�̎� */
			break;
		case SC_APPLEIDUN:			/* �C�h�D���̗ь� */
			calc_flag = 1;
			break;
		case SC_UGLYDANCE:			/* ��������ȃ_���X */
			val2 = 10;
			break;
		case SC_HUMMING:			/* �n�~���O */
			calc_flag = 1;
			break;
		case SC_DONTFORGETME:		/* ����Y��Ȃ��� */
			calc_flag = 1;
			if(sc_data[SC_INCREASEAGI].timer!=-1 )	/* ���x�㏸���� */
				status_change_end(bl,SC_INCREASEAGI,-1);
			if(sc_data[SC_TWOHANDQUICKEN].timer!=-1 )
				status_change_end(bl,SC_TWOHANDQUICKEN,-1);
			if(sc_data[SC_SPEARSQUICKEN].timer!=-1 )
				status_change_end(bl,SC_SPEARSQUICKEN,-1);
			if(sc_data[SC_ADRENALINE].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE,-1);
			if(sc_data[SC_ASSNCROS].timer!=-1 )
				status_change_end(bl,SC_ASSNCROS,-1);
			if(sc_data[SC_TRUESIGHT].timer!=-1 )	/* �g�D��?�T�C�g */
				status_change_end(bl,SC_TRUESIGHT,-1);
			if(sc_data[SC_WINDWALK].timer!=-1 )	/* �E�C���h�E�H?�N */
				status_change_end(bl,SC_WINDWALK,-1);
			if(sc_data[SC_CARTBOOST].timer!=-1 )	/* �J?�g�u?�X�g */
				status_change_end(bl,SC_CARTBOOST,-1);
			break;
		case SC_FORTUNE:			/* �K�^�̃L�X */
			calc_flag = 1;
			break;
		case SC_SERVICE4U:			/* �T?�r�X�t�H?��? */
			calc_flag = 1;
			break;
		case SC_MOONLIT:
			val2 = bl->id;
			break;
		case SC_DANCING:			/* �_���X/���t�� */
			calc_flag = 1;
			val3= tick / 1000;
			tick = 1000;
			break;

		case SC_EXPLOSIONSPIRITS:	// �����g��
			calc_flag = 1;
			val2 = 75 + 25*val1;
			*opt3 |= 8;
			break;
		case SC_STEELBODY:			// ����
			calc_flag = 1;
			*opt3 |= 16;
			break;

		case SC_AUTOCOUNTER:
			val3 = val4 = 0;
			break;

		case SC_SPEEDPOTION0:		/* ?���|?�V���� */
		case SC_SPEEDPOTION1:
		case SC_SPEEDPOTION2:
		case SC_SPEEDPOTION3:
			calc_flag = 1;
			tick = 1000 * tick;
			val2 = 5*(2+type-SC_SPEEDPOTION0);
			break;

		/* atk & matk potions [Valaris] */
		case SC_ATKPOT:
		case SC_MATKPOT:
			calc_flag = 1;
			tick = 1000 * tick;
			break;
		case SC_WEDDING:	//�����p(�����ߏւɂȂ���?���̂�?���Ƃ�)
			{
				time_t timer;

				calc_flag = 1;
				tick = 10000;
				if(!val2)
					val2 = (int)time(&timer);
			}
			break;
		case SC_NOCHAT:	//�`���b�g�֎~?��
			{
				time_t timer;

				if(!battle_config.muting_players)
					break;

				tick = 60000;
				if(!val2)
					val2 = (int)time(&timer);
				updateflag = SP_MANNER;
				save_flag = 1; // celest
			}
			break;

		/* option1 */
		case SC_STONE:				/* �Ή� */
			if(!(flag&2)) {
				int sc_def = status_get_mdef(bl)*200;
				tick = tick - sc_def;
			}
			val3 = tick/1000;
			if(val3 < 1) val3 = 1;
			tick = 5000;
			val2 = 1;
			break;
		case SC_SLEEP:				/* ���� */
			if(!(flag&2)) {
				tick = 30000;//�����̓X�e?�^�X�ϐ���?��炸30�b
			}
			break;
		case SC_FREEZE:				/* ���� */
			if(!(flag&2)) {
				int sc_def = 100 - status_get_mdef(bl);
				tick = tick * sc_def / 100;
			}
			break;
		case SC_STAN:				/* �X�^���ival2�Ƀ~���b�Z�b�g�j */
			if(!(flag&2)) {
				int sc_def = status_get_sc_def_vit(bl);
				tick = tick * sc_def / 100;
			}
			break;

			/* option2 */
		case SC_DPOISON:			/* �ғ� */
		{
			int mhp = status_get_max_hp(bl);
			int hp = status_get_hp(bl);
			// MHP?1/4????????
			if (hp > mhp>>2) {
				if(bl->type == BL_PC) {
					int diff = mhp*10/100;
					if (hp - diff < mhp>>2)
						hp = hp - (mhp>>2);
					pc_heal(*((struct map_session_data *)bl), -hp, 0);
				} else if(bl->type == BL_MOB) {
					struct mob_data *md = (struct mob_data *)bl;
					hp -= mhp*15/100;
					if (hp > mhp>>2)
						md->hp = hp;
					else
						md->hp = mhp>>2;
				}
			}
		}	// fall through
		case SC_POISON:				/* �� */
			calc_flag = 1;
			if(!(flag&2)) {
				int sc_def = 100 - (status_get_vit(bl) + status_get_luk(bl)/5);
				tick = tick * sc_def / 100;
			}
			val3 = tick/1000;
			if(val3 < 1) val3 = 1;
			tick = 1000;
			break;
		case SC_SILENCE:			/* ��?�i���b�N�X�f�r?�i�j */
			if (sc_data && sc_data[SC_GOSPEL].timer!=-1) {
				skill_delunitgroup((struct skill_unit_group *)sc_data[SC_GOSPEL].val3);
				status_change_end(bl,SC_GOSPEL,-1);
				break;
			}
			if(!(flag&2)) {
				int sc_def = 100 - status_get_vit(bl);
				tick = tick * sc_def / 100;
			}
			break;
		case SC_CONFUSION:
			val2 = tick;
			tick = 100;
			clif_emotion(*bl,1);
			if (sd) {
				pc_stop_walking (*sd, 0);
			}
			break;
		case SC_BLIND:				/* ��? */
			calc_flag = 1;
			if(!(flag&2)) {
				int sc_def = status_get_lv(bl)/10 + status_get_int(bl)/15;
				tick = 30000 - sc_def;
			}
			break;
		case SC_CURSE:
			calc_flag = 1;
			if(!(flag&2)) {
				int sc_def = 100 - status_get_vit(bl);
				tick = tick * sc_def / 100;
			}
			break;

		/* option */
		case SC_HIDING:		/* �n�C�f�B���O */
			calc_flag = 1;
			if(bl->type == BL_PC) {
				val2 = tick / 1000;		/* ��?���� */
				tick = 1000;
			}
			break;
		case SC_CHASEWALK:
		case SC_CLOAKING:		/* �N��?�L���O */
			if(bl->type == BL_PC) {
				calc_flag = 1; // [Celest]
				val2 = tick;
				val3 = type==SC_CLOAKING ? 130-val1*3 : 135-val1*5;
			}
			else
				tick = 5000*val1;
			break;
		case SC_SIGHT:			/* �T�C�g/���A�t */
		case SC_RUWACH:
			val2 = tick/250;
			tick = 10;
			break;

		/* �Z?�t�e�B�E�H?���A�j��?�} */
		case SC_SAFETYWALL:
		case SC_PNEUMA:
			tick=((struct skill_unit *)val2)->group->limit;
			break;

		/* �X�L������Ȃ�/���Ԃ�?�W���Ȃ� */
		case SC_RIDING:
			calc_flag = 1;
			tick = 600*1000;
			break;
		case SC_FALCON:
		case SC_WEIGHT50:
		case SC_WEIGHT90:
		case SC_BROKNWEAPON:
		case SC_BROKNARMOR:
			tick=600*1000;
			break;

		case SC_AUTOGUARD:
			{
				int i,t;
				for(i=val2=0;i<val1;i++) {
					t = 5-(i>>1);
					val2 += (t < 0)? 1:t;
				}
			}
			break;

		case SC_DEFENDER:
			calc_flag = 1;
			val2 = 5 + val1*15;
			break;

		case SC_CONCENTRATION:	/* �R���Z���g��?�V���� */
			*opt3 |= 1;
			calc_flag = 1;
			break;

		case SC_TENSIONRELAX:	/* �e���V���������b�N�X */
			if(bl->type == BL_PC) {
				tick = 10000;
			} else return 0;
			break;

		case SC_PARRYING:		/* �p���C���O */
		    val2 = 20 + val1*3;
			break;

		case SC_WINDWALK:		/* �E�C���h�E�H?�N */
			calc_flag = 1;
			val2 = (val1 / 2); //Flee�㏸��
			break;
		
		case SC_JOINTBEAT: // Random break [DracoRPG]
			calc_flag = 1;
			val2 = rand()%6;
			if (val2 == 5) status_change_start(bl,SC_BLEEDING,val1,0,0,0,skill_get_time2(type,val1),0);
			break;

		case SC_BERSERK:		/* �o?�T?�N */
			if(sd){
				sd->status.hp = sd->status.max_hp * 3;
				sd->status.sp = 0;
				clif_updatestatus(*sd,SP_HP);
				clif_updatestatus(*sd,SP_SP);
				sd->canregen_tick = gettick() + 300000;
			}
			*opt3 |= 128;
			tick = 10000;
			calc_flag = 1;
			break;

		case SC_ASSUMPTIO:		/* �A�X���v�e�B�I */
			if(sc_data[SC_KYRIE].timer!=-1 )
			{
				status_change_end(bl,SC_KYRIE,-1);
				break;
			}
			*opt3 |= 2048;
			break;

		case SC_GOSPEL:
			if (val4 == BCT_SELF) {	// self effect
				if (sd) {
					sd->canact_tick += tick;
					sd->canmove_tick += tick;
				}
				val2 = tick;
				tick = 1000;
				status_change_clear_buffs(bl);
			}
			break;

		case SC_MARIONETTE:		/* �}���I�l�b�g�R���g��?�� */
		case SC_MARIONETTE2:
			val2 = tick;
			if (!val3)
				return 0;
			tick = 1000;
			calc_flag = 1;
			*opt3 |= 1024;
			break;

		case SC_REJECTSWORD:	/* ���W�F�N�g�\?�h */
			val2 = 3; //3��U?�𒵂˕Ԃ�
			break;

		case SC_MEMORIZE:		/* �������C�Y */
			val2 = 5; //��r����1/3�ɂ���
			break;

		case SC_GRAVITATION:
			if (sd) {
				if (val3 == BCT_SELF) {
					sd->canmove_tick += tick;
					sd->canact_tick += tick;
				} else calc_flag = 1;
			}
			//val2 = val1*5;
			break;

		case SC_HERMODE:
			status_change_clear_buffs(bl);
			break;

		case SC_BLEEDING:
			{
				//val4 = 10000;
				val4 = tick;
				tick = 10000;
			}
			break;

		case SC_REGENERATION:
			val1 = 2;
		case SC_BATTLEORDERS:
			tick = 60000; // 1 minute
			calc_flag = 1;
			break;
		case SC_GUILDAURA:
			calc_flag = 1;
			tick = 1000;
			break;

		case SC_CONCENTRATE:		/* �W���͌��� */
		case SC_BLESSING:			/* �u���b�V���O */
		case SC_ANGELUS:			/* �A���[���X */
		case SC_IMPOSITIO:			/* �C���|�V�e�B�I�}�k�X */
		case SC_GLORIA:				/* �O�����A */
		case SC_LOUD:				/* ���E�h�{�C�X */
		case SC_DEVOTION:			/* �f�B�{?�V���� */
		case SC_KEEPING:
		case SC_BARRIER:
		case SC_MELTDOWN:		/* �����g�_�E�� */
		case SC_CARTBOOST:		/* �J?�g�u?�X�g */
		case SC_TRUESIGHT:		/* �g�D��?�T�C�g */
		case SC_SPIDERWEB:		/* �X�p�C�_?�E�F�b�u */
		case SC_SLOWDOWN:
		case SC_SPEEDUP0:
		case SC_INCALLSTATUS:
		case SC_INCHIT:			/* HIT�㏸ */
		case SC_INCFLEE:		/* FLEE�㏸ */
		case SC_INCMHP2:		/* MHP%�㏸ */
		case SC_INCMSP2:		/* MSP%�㏸ */
		case SC_INCATK2:		/* ATK%�㏸ */
		case SC_INCMATK2:
		case SC_INCHIT2:		/* HIT%�㏸ */
		case SC_INCFLEE2:		/* FLEE%�㏸ */
		case SC_INCDEF2:
		case SC_INCSTR:
		case SC_INCAGI:
			calc_flag = 1;
			break;

		case SC_SUFFRAGIUM:			/* �T�t���M�� */
		case SC_BENEDICTIO:			/* ��? */
		case SC_MAGNIFICAT:			/* �}�O�j�t�B�J?�g */
		case SC_AETERNA:			/* �G?�e���i */
  		case SC_STRIPARMOR:
		case SC_STRIPHELM:
		case SC_CP_WEAPON:
		case SC_CP_SHIELD:
		case SC_CP_ARMOR:
		case SC_CP_HELM:
		case SC_EXTREMITYFIST:		/* ���C���e���� */
		case SC_ANKLE:	/* �A���N�� */
		case SC_COMBO:
		case SC_BLADESTOP_WAIT:		/* ���n���(�҂�) */
		case SC_HALLUCINATION:
		case SC_BASILICA: // [celest]
		case SC_SPLASHER:		/* �x�i���X�v���b�V��? */
		case SC_FOGWALL:
		case SC_PRESERVE:
		case SC_DOUBLECAST:
		case SC_MAXOVERTHRUST:
        case SC_AURABLADE:		/* �I?���u��?�h */
       	case SC_BABY:
			break;

		default:
			if(battle_config.error_log)
				ShowMessage("UnknownStatusChange [%d]\n", type);
			return 0;
	}

	if (bl->type == BL_PC && (battle_config.display_hallucination || type != SC_HALLUCINATION))
		clif_status_change(*bl,type,1);	/* �A�C�R���\�� */

	/* option��?�X */
	switch(type){
		case SC_STONE:
		case SC_FREEZE:
		case SC_STAN:
		case SC_SLEEP:
			battle_stopattack(bl);	/* �U?��~ */
			skill_stop_dancing(bl,0);	/* ���t/�_���X�̒�? */
			{	/* �����Ɋ|����Ȃ��X�e?�^�X�ُ������ */
				int i;
				for(i = SC_STONE; i <= SC_SLEEP; i++){
					if(sc_data[i].timer != -1){
						delete_timer(sc_data[i].timer, status_change_timer);
						sc_data[i].timer = -1;
					}
				}
			}
			if(type == SC_STONE)
				*opt1 = 6;
			else
				*opt1 = type - SC_STONE + 1;
			opt_flag = 1;
			break;
		case SC_POISON:
		case SC_CURSE:
		case SC_SILENCE:
		case SC_BLIND:
			*opt2 |= 1<<(type-SC_POISON);
			opt_flag = 1;
			break;
		case SC_DPOISON:	// �b��œł̃G�t�F�N�g���g�p
			*opt2 |= 1;
			opt_flag = 1;
			break;
		case SC_SIGNUMCRUCIS:
			*opt2 |= 0x40;
			opt_flag = 1;
			break;
		case SC_HIDING:
		case SC_CLOAKING:
			battle_stopattack(bl);	/* �U?��~ */
			*option |= ((type==SC_HIDING)?2:4);
			opt_flag =1 ;
			break;
		case SC_CHASEWALK:
			battle_stopattack(bl);	/* �U?��~ */
			*option |= 16388;
			opt_flag =1 ;
			break;
		case SC_SIGHT:
			*option |= 1;
			opt_flag = 1;
			break;
		case SC_RUWACH:
			*option |= 8192;
			opt_flag = 1;
			break;
		case SC_WEDDING:
			*option |= 4096;
			opt_flag = 1;
	}

	if(opt_flag)	/* option��?�X */
		clif_changeoption(*bl);

	sc_data[type].val1 = val1;
	sc_data[type].val2 = val2;
	sc_data[type].val3 = val3;
	sc_data[type].val4 = val4;
	/* �^�C�}?�ݒ� */
	sc_data[type].timer = add_timer(gettick() + tick, status_change_timer, bl->id, type);

	if(bl->type==BL_PC && calc_flag)
		status_calc_pc(*sd,0);	/* �X�e?�^�X�Čv�Z */

	if(bl->type==BL_PC && save_flag)
		chrif_save(*sd); // save the player status

	if(bl->type==BL_PC && updateflag)
		clif_updatestatus(*sd,updateflag);	/* �X�e?�^�X���N���C�A���g�ɑ��� */

	if (bl->type==BL_PC && sd->pd)
		pet_sc_check(*sd, type); //Skotlex: Pet Status Effect Healing
	return 0;
}
/*==========================================
 * �X�e�[�^�X�ُ�S����
 *------------------------------------------
 */
int status_change_clear(struct block_list *bl,int type)
{
	struct status_change* sc_data;
	short *option, *opt1, *opt2, *opt3;
	int i;

	nullpo_retr(0, bl);
	nullpo_retr(0, sc_data = status_get_sc_data(bl));
	nullpo_retr(0, option = status_get_option(bl));
	nullpo_retr(0, opt1 = status_get_opt1(bl));
	nullpo_retr(0, opt2 = status_get_opt2(bl));
	nullpo_retr(0, opt3 = status_get_opt3(bl));

	for(i = 0; i < MAX_STATUSCHANGE; i++)
	{
		if(sc_data[i].timer != -1){	/* �ُ킪����Ȃ�^�C�}?���폜���� */
			status_change_end(bl, i, -1);
		}
	}
	*opt1 = 0;
	*opt2 = 0;
	*opt3 = 0;
	*option &= OPTION_MASK;

	if(!type || type&2)
		clif_changeoption(*bl);

	return 0;
}

/*==========================================
 * �X�e�[�^�X�ُ�I��
 *------------------------------------------
 */
int status_change_end( struct block_list* bl, int type, int tid )
{
	struct status_change* sc_data;
	int opt_flag=0, calc_flag = 0;
	//!!short *sc_count;
	short *option, *opt1, *opt2, *opt3;

	nullpo_retr(0, bl);
	if(bl->type!=BL_PC && bl->type!=BL_MOB) {
		if(battle_config.error_log)
			ShowMessage("status_change_end: neither MOB nor PC !\n");
		return 0;
	}
	nullpo_retr(0, sc_data = status_get_sc_data(bl));
	nullpo_retr(0, option = status_get_option(bl));
	nullpo_retr(0, opt1 = status_get_opt1(bl));
	nullpo_retr(0, opt2 = status_get_opt2(bl));
	nullpo_retr(0, opt3 = status_get_opt3(bl));

	if( sc_data[type].timer != -1 && (sc_data[type].timer == tid || tid == -1))
	{
		if (tid == -1)	// �^�C�}����Ă΂�Ă��Ȃ��Ȃ�^�C�}�폜������
		{
			delete_timer(sc_data[type].timer,status_change_timer);
		}
		// �Y?�ُ̈�𐳏��?�� 
		sc_data[type].timer=-1;

		switch(type){	/* �ُ�̎�ނ��Ƃ�?�� */
			case SC_PROVOKE:			/* �v���{�b�N */
			case SC_ENDURE: // celest
			case SC_CONCENTRATE:		/* �W���͌��� */
			case SC_BLESSING:			/* �u���b�V���O */
			case SC_ANGELUS:			/* �A���[���X */
			case SC_INCREASEAGI:		/* ���x�㏸ */
			case SC_DECREASEAGI:		/* ���x���� */
			case SC_SIGNUMCRUCIS:		/* �V�O�i���N���V�X */
			case SC_HIDING:
			case SC_TWOHANDQUICKEN:		/* 2HQ */
			case SC_ADRENALINE:			/* �A�h���i�������b�V�� */
			case SC_ENCPOISON:			/* �G���`�����g�|�C�Y�� */
			case SC_IMPOSITIO:			/* �C���|�V�e�B�I�}�k�X */
			case SC_GLORIA:				/* �O�����A */
			case SC_LOUD:				/* ���E�h�{�C�X */
			case SC_QUAGMIRE:			/* �N�@�O�}�C�A */
			case SC_PROVIDENCE:			/* �v�����B�f���X */
			case SC_SPEARSQUICKEN:		/* �X�s�A�N�C�b�P�� */
			case SC_VOLCANO:
			case SC_DELUGE:
			case SC_VIOLENTGALE:
			case SC_ETERNALCHAOS:		/* �G�^?�i���J�I�X */
			case SC_DRUMBATTLE:			/* ?���ۂ̋��� */
			case SC_NIBELUNGEN:			/* �j?�x�����O�̎w�� */
			case SC_SIEGFRIED:			/* �s���g�̃W?�N�t��?�h */
			case SC_WHISTLE:			/* ���J */
			case SC_ASSNCROS:			/* �[�z�̃A�T�V���N���X */
			case SC_HUMMING:			/* �n�~���O */
			case SC_DONTFORGETME:		/* ����Y��Ȃ��� */
			case SC_FORTUNE:			/* �K�^�̃L�X */
			case SC_SERVICE4U:			/* �T?�r�X�t�H?��? */
			case SC_EXPLOSIONSPIRITS:	// �����g��
			case SC_STEELBODY:			// ����
			case SC_DEFENDER:
			case SC_SPEEDPOTION0:		/* ?���|?�V���� */
			case SC_SPEEDPOTION1:
			case SC_SPEEDPOTION2:
			case SC_SPEEDPOTION3:
			case SC_APPLEIDUN:			/* �C�h�D���̗ь� */
			case SC_RIDING:
			case SC_BLADESTOP_WAIT:
			case SC_CONCENTRATION:		/* �R���Z���g��?�V���� */
			case SC_ASSUMPTIO:			/* �A�V�����v�e�B�I */
			case SC_WINDWALK:		/* �E�C���h�E�H?�N */
			case SC_TRUESIGHT:		/* �g�D��?�T�C�g */
			case SC_SPIDERWEB:		/* �X�p�C�_?�E�F�b�u */
			case SC_MAGICPOWER:		/* ���@��?�� */
			case SC_CHASEWALK:
			case SC_ATKPOT:		/* attack potion [Valaris] */
			case SC_MATKPOT:		/* magic attack potion [Valaris] */
			case SC_WEDDING:	//�����p(�����ߏւɂȂ���?���̂�?���Ƃ�)
			case SC_MELTDOWN:		/* �����g�_�E�� */
			case SC_CARTBOOST:
			case SC_MINDBREAKER:		/* �}�C���h�u���[�J�[ */
			case SC_BERSERK:
			// Celest
			case SC_EDP:
			case SC_SLOWDOWN:
			case SC_SPEEDUP0:
			case SC_INCALLSTATUS:
			case SC_INCHIT:
			case SC_INCFLEE:
			case SC_INCMHP2:
			case SC_INCMSP2:
			case SC_INCATK2:
			case SC_INCMATK2:
			case SC_INCHIT2:
			case SC_INCFLEE2:
			case SC_INCDEF2:
			case SC_INCSTR:
			case SC_INCAGI:
			case SC_BATTLEORDERS:
			case SC_REGENERATION:
			case SC_GUILDAURA:
				calc_flag = 1;
				break;
			case SC_AUTOBERSERK:
				if (sc_data[SC_PROVOKE].timer != -1)
					status_change_end(bl,SC_PROVOKE,-1);
				break;
			case SC_DEVOTION:		/* �f�B�{?�V���� */
				{
					struct map_session_data *md = map_id2sd(sc_data[type].val1);
					sc_data[type].val1=sc_data[type].val2=0;
					if(md) skill_devotion(md,bl->id);
					calc_flag = 1;
				}
				break;
			case SC_BLADESTOP:
				{
					struct status_change *t_sc_data = status_get_sc_data((struct block_list *)sc_data[type].val4);
					//�Е����؂ꂽ�̂ő���̔��n?�Ԃ��؂�ĂȂ��̂Ȃ����
					if(t_sc_data && t_sc_data[SC_BLADESTOP].timer!=-1)
						status_change_end((struct block_list *)sc_data[type].val4,SC_BLADESTOP,-1);

					if(sc_data[type].val2==2)
						clif_bladestop(*((struct block_list *)sc_data[type].val3),*((struct block_list *)sc_data[type].val4),0);
				}
				break;
			case SC_DANCING:
				{
					struct map_session_data *dsd;
					struct status_change *d_sc_data;
					if(sc_data[type].val4 && (dsd=map_id2sd(sc_data[type].val4))){
						d_sc_data = dsd->sc_data;
						//���t�ő��肪����ꍇ�����val4��0�ɂ���
						if(d_sc_data && d_sc_data[type].timer!=-1)
							d_sc_data[type].val4=0;
					}
				}
				if (sc_data[SC_LONGING].timer!=-1)
					status_change_end(bl,SC_LONGING,-1);				
				calc_flag = 1;
				break;
			case SC_NOCHAT:	//�`���b�g�֎~?��
				{
					struct map_session_data *sd=NULL;
					if(bl->type == BL_PC && (sd=(struct map_session_data *)bl)){
						if (sd->status.manner >= 0) // weeee ^^ [celest]
							sd->status.manner = 0;
						clif_updatestatus(*sd,SP_MANNER);
					}
				}
				break;
			case SC_SPLASHER:		/* �x�i���X�v���b�V��? */
				{
					struct block_list *src=map_id2bl(sc_data[type].val3);
					if(src && tid!=-1){
						//�����Ƀ_��?�W����?3*3�Ƀ_��?�W
						skill_castend_damage_id(src, bl,(unsigned short)sc_data[type].val2,(unsigned short)sc_data[type].val1,gettick(),0 );
					}
				}
				break;

		/* option1 */
			case SC_FREEZE:
				sc_data[type].val3 = 0;
				break;

		/* option2 */
			case SC_POISON:				/* �� */
			case SC_BLIND:				/* ��? */
			case SC_CURSE:
				calc_flag = 1;
				break;

			// celest
			case SC_CONFUSION:
				{
					struct map_session_data *sd=NULL;
					if(bl->type == BL_PC && (sd=(struct map_session_data *)bl)){
						sd->next_walktime = 0;
					}
				}
				break;

			case SC_MARIONETTE:		/* �}���I�l�b�g�R���g��?�� */
			case SC_MARIONETTE2:	/// Marionette target
				{
					// check for partner and end their marionette status as well
					int type2 = (type == SC_MARIONETTE) ? SC_MARIONETTE2 : SC_MARIONETTE;
					struct block_list *pbl = map_id2bl(sc_data[type].val3);
					if (pbl) {
						struct status_change* sc_data;
						if(//*status_get_sc_count(pbl) > 0 &&
							(sc_data = status_get_sc_data(pbl)) &&
							sc_data[type2].timer != -1)
							status_change_end(pbl, type2, -1);
					}
					calc_flag = 1;
				}
				break;

			case SC_GRAVITATION:
				if (bl->type == BL_PC) {
					if (sc_data[type].val3 == BCT_SELF) {
						struct map_session_data *sd = (struct map_session_data *)bl;
						if (sd) {
							int tick = gettick();
							sd->canmove_tick = tick;
							sd->canact_tick = tick;
						}
					} else calc_flag = 1;
				}
				break;

			case SC_BABY:
				break;
			}

		if (bl->type == BL_PC && (battle_config.display_hallucination || type != SC_HALLUCINATION))
			clif_status_change(*bl,type,0);	/* �A�C�R������ */

		switch(type){	/* �����?��Ƃ��Ȃɂ�?�����K�v */
		case SC_STONE:
		case SC_FREEZE:
		case SC_STAN:
		case SC_SLEEP:
			*opt1 = 0;
			opt_flag = 1;
			break;

		case SC_POISON:
			if (sc_data[SC_DPOISON].timer != -1)	//
				break;						// DPOISON�p�̃I�v�V����
			*opt2 &= ~1;					// ��?�p�ɗp�ӂ��ꂽ�ꍇ�ɂ�
			opt_flag = 1;					// �����͍폜����
			break;							//
		case SC_CURSE:
		case SC_SILENCE:
		case SC_BLIND:
			*opt2 &= ~(1<<(type-SC_POISON));
			opt_flag = 1;
			break;
		case SC_DPOISON:
			if (sc_data[SC_POISON].timer != -1)	// DPOISON�p�̃I�v�V������
				break;							// �p�ӂ��ꂽ��폜
			*opt2 &= ~1;	// ��?�ԉ���
			opt_flag = 1;
			break;
		case SC_SIGNUMCRUCIS:
			*opt2 &= ~0x40;
			opt_flag = 1;
			break;

		case SC_HIDING:
		case SC_CLOAKING:
			*option &= ~((type == SC_HIDING) ? 2 : 4);
			calc_flag = 1;	// orn
			opt_flag = 1 ;
			break;

		case SC_CHASEWALK:
			*option &= ~16388;
			opt_flag = 1 ;
			break;

		case SC_SIGHT:
			*option &= ~1;
			opt_flag = 1;
			break;
		case SC_WEDDING:	//�����p(�����ߏւɂȂ���?���̂�?���Ƃ�)
			*option &= ~4096;
			opt_flag = 1;
			break;
		case SC_RUWACH:
			*option &= ~8192;
			opt_flag = 1;
			break;

		//opt3
		case SC_TWOHANDQUICKEN:		/* 2HQ */
		case SC_SPEARSQUICKEN:		/* �X�s�A�N�C�b�P�� */
		case SC_CONCENTRATION:		/* �R���Z���g��?�V���� */
			*opt3 &= ~1;
			break;
		case SC_OVERTHRUST:			/* �I?�o?�X���X�g */
			*opt3 &= ~2;
			break;
		case SC_ENERGYCOAT:			/* �G�i�W?�R?�g */
			*opt3 &= ~4;
			break;
		case SC_EXPLOSIONSPIRITS:	// �����g��
			*opt3 &= ~8;
			break;
		case SC_STEELBODY:			// ����
			*opt3 &= ~16;
			break;
		case SC_BLADESTOP:		/* ���n��� */
			*opt3 &= ~32;
			break;
		case SC_BERSERK:		/* �o?�T?�N */
			*opt3 &= ~128;
			break;
		case SC_MARIONETTE:		/* �}���I�l�b�g�R���g��?�� */
		case SC_MARIONETTE2:
			*opt3 &= ~1024;
			break;
		case SC_ASSUMPTIO:		/* �A�X���v�e�B�I */
			*opt3 &= ~2048;
			break;
		}

		if(opt_flag)	/* option��?�X��?���� */
			clif_changeoption(*bl);

		if (bl->type == BL_PC && calc_flag)
			status_calc_pc(*((struct map_session_data *)bl),0);	/* �X�e?�^�X�Čv�Z */
	}

	return 0;
}


/*==========================================
 * �X�e�[�^�X�ُ�I���^�C�}�[
 *------------------------------------------
 */
int status_change_timer(int tid,unsigned long tick,int id,int data)
{
	int type = data;
	
	struct status_change *sc_data;
	struct map_session_data *sd=NULL;
	struct mob_data *md=NULL;
	struct block_list *bl=map_id2bl(id);

#ifdef nullpo_retr_f
	nullpo_retr_f(0, bl, "id=%d data=%d",id,data);
#else
	nullpo_retr(0, bl);
#endif

	nullpo_retr(0, sc_data=status_get_sc_data(bl));

	if(sc_data[type].timer != tid) {
		if(battle_config.error_log)
			ShowMessage("status_change_timer %d != %d\n",tid,sc_data[type].timer);
		return 0;
	}
	// security system to prevent forgetting timer removal
	int temp_timerid = sc_data[type].timer;
	sc_data[type].timer = -1;


	if(bl->type==BL_PC)
		sd=(struct map_session_data *)bl;
	else if(bl->type==BL_MOB)
		md=(struct mob_data *)bl;

	switch(type){	/* �����?���ɂȂ�ꍇ */
	case SC_MAXIMIZEPOWER:	/* �}�L�V�}�C�Y�p��? */
	case SC_CLOAKING:
		if(sd){
			if( sd->status.sp > 0 ){ /* SP�؂��܂Ŏ�? */
				sd->status.sp--;
				clif_updatestatus(*sd,SP_SP);
				/* �^�C�}?�Đݒ� */
				sc_data[type].timer=add_timer(sc_data[type].val2+tick, status_change_timer, bl->id, data);
				return 0;
			}
		}
		break;

	case SC_CHASEWALK:
		if(sd){
			long sp = 10+sc_data[SC_CHASEWALK].val1*2;
			if (map[sd->bl.m].flag.gvg) sp *= 5;
			if (sd->status.sp > sp){
				sd->status.sp -= sp; // update sp cost [Celest]
				clif_updatestatus(*sd,SP_SP);
				if ((++sc_data[SC_CHASEWALK].val4) == 1) {
					status_change_start(bl, SC_INCSTR, 1<<(sc_data[SC_CHASEWALK].val1-1), 0, 0, 0, 30000, 0);
					status_calc_pc (*sd, 0);
				}
				/* �^�C�}?�Đݒ� */
				sc_data[type].timer=add_timer(sc_data[type].val2+tick, status_change_timer, bl->id, data);
				return 0;
			}
		}
	break;

	case SC_HIDING:		/* �n�C�f�B���O */
		if(sd){		/* SP�������āA���Ԑ����̊Ԃ͎�? */
			if( sd->status.sp > 0 && (--sc_data[type].val2)>0 ){
				if(sc_data[type].val2 % (sc_data[type].val1+3) ==0 ){
					sd->status.sp--;
					clif_updatestatus(*sd,SP_SP);
				}
				/* �^�C�}?�Đݒ� */
				sc_data[type].timer=add_timer(1000+tick, status_change_timer,bl->id, data);
				return 0;
			}
		}
	break;

	case SC_SIGHT:	/* �T�C�g */
	case SC_RUWACH:	/* ���A�t */
		{
			int range = 5;
			if ( type == SC_SIGHT ) range = 7;
			map_foreachinarea( status_change_timer_sub,
				bl->m, ((int)bl->x)-range, ((int)bl->y)-range, ((int)bl->x)+range,((int)bl->y)+range,0,
				bl,type,tick);

			if( (--sc_data[type].val2)>0 ){
				/* �^�C�}?�Đݒ� */
				sc_data[type].timer=add_timer(250+tick, status_change_timer,bl->id, data);
				return 0;
			}
		}
		break;

	case SC_SIGNUMCRUCIS:		/* �V�O�i���N���V�X */
		{
			int race = status_get_race(bl);
			if(race == 6 || battle_check_undead(race,status_get_elem_type(bl))) {
				sc_data[type].timer=add_timer(1000*600+tick,status_change_timer, bl->id, data );
				return 0;
			}
		}
		break;

	case SC_PROVOKE:	/* �v���{�b�N/�I?�g�o?�T?�N */
		if(sc_data[type].val2!=0){	/* �I?�g�o?�T?�N�i�P�b���Ƃ�HP�`�F�b�N�j */
			if(sd && sd->status.hp>sd->status.max_hp>>2)	/* ��~ */
				break;
			sc_data[type].timer=add_timer( 1000+tick,status_change_timer, bl->id, data );
			return 0;
		}
		break;

	case SC_ENDURE:	/* �C���f���A */
	case SC_AUTOBERSERK: // Celest
		if(sd && sd->state.infinite_endure) {
			sc_data[type].timer=add_timer( 1000*60+tick,status_change_timer, bl->id, data );
			return 0;
		}
		break;

	case SC_STONE:
		if(sc_data[type].val2 != 0) {
			short *opt1 = status_get_opt1(bl);
			sc_data[type].val2 = 0;
			sc_data[type].val4 = 0;
			battle_stopwalking(bl,1);
			if(opt1) {
				*opt1 = 1;
				clif_changeoption(*bl);
			}
			sc_data[type].timer=add_timer(1000+tick,status_change_timer, bl->id, data );
			return 0;
		}
		else if( (--sc_data[type].val3) > 0) {
			int hp = status_get_max_hp(bl);
			if((++sc_data[type].val4)%5 == 0 && status_get_hp(bl) > hp>>2) {
				hp = hp/100;
				if(hp < 1) hp = 1;
				if(sd)
					pc_heal(*sd,-hp,0);
				else if(md){
					md->hp -= hp;
				}
			}
			sc_data[type].timer=add_timer(1000+tick,status_change_timer, bl->id, data );
			return 0;
		}
		break;
	case SC_POISON:
	case SC_DPOISON:
		if (sc_data[SC_SLOWPOISON].timer == -1 && (--sc_data[type].val3) > 0) {
			int hp = status_get_max_hp(bl);
			if (type == SC_POISON && status_get_hp(bl) < hp>>2)
				break;
			if(sd) {
				hp = (type == SC_DPOISON) ? 3 + hp/50 : 3 + hp*3/200;
				pc_heal(*sd, -hp, 0);
			} else if(md) {
				hp = (type == SC_DPOISON) ? 3 + hp/100 : 3 + hp/200;
				md->hp -= hp;
			}
		}
		if (sc_data[type].val3 > 0 && !status_isdead(bl))
		{
			sc_data[type].timer = add_timer (1000 + tick, status_change_timer, bl->id, data );
			return 0;
		}
		break;
	case SC_TENSIONRELAX:	/* �e���V���������b�N�X */
		if(sd){		/* SP�������āAHP��?�^���łȂ����?? */
			if( sd->status.sp > 12 && sd->status.max_hp > sd->status.hp ){
				/* �^�C�}?�Đݒ� */
				sc_data[type].timer=add_timer(10000+tick, status_change_timer,bl->id, data);
				return 0;
			}
			if(sd->status.max_hp <= sd->status.hp)
			{
				status_change_end(&sd->bl,SC_TENSIONRELAX,-1);
				return 0;
			}
		}
		break;
	case SC_BLEEDING:	// [celest]
		// i hope i haven't interpreted it wrong.. which i might ^^;
		// Source:
		// - 10�����Ȫ�HP�����
		// - ����Ϊުޫ�?����Ѫ��������ƪ�?����Ἢ��ʪ�
		// To-do: bleeding effect increases damage taken?
		if ((sc_data[type].val4 -= 10000) > 0) {
			int hp = rand()%300 + 400;
			if(sd)
			{
				pc_heal(*sd,-hp,0);
			}
			else if(md)
			{
				md->hp -= hp;
			}
			if (!status_isdead(bl)) {
				// walking and casting effect is lost
				battle_stopwalking (bl, 1);
				skill_castcancel (bl, 0);
				sc_data[type].timer = add_timer(10000 + tick, status_change_timer, bl->id, data );
			}
			return 0;
		}
		break;

	/* ���Ԑ؂ꖳ���H�H */
	case SC_AETERNA:
	case SC_TRICKDEAD:
	case SC_RIDING:
	case SC_FALCON:
	case SC_WEIGHT50:
	case SC_WEIGHT90:
	case SC_MAGICPOWER:		/* ���@��?�� */
	case SC_REJECTSWORD:	/* ���W�F�N�g�\?�h */
	case SC_MEMORIZE:	/* �������C�Y */
	case SC_BROKNWEAPON:
	case SC_BROKNARMOR:
	case SC_SACRIFICE:
		sc_data[type].timer=add_timer( tick+1000*600,status_change_timer, bl->id, data );
		return 0;
	case SC_DANCING: //�_���X�X�L���̎���SP����
		{
			int s = 0, sp = 1;
			if(sd && (--sc_data[type].val3) > 0) {
				switch(sc_data[type].val1){
				case BD_RICHMANKIM:				/* �j�����h�̉� 3�b��SP1 */
				case BD_DRUMBATTLEFIELD:		/* ?���ۂ̋��� 3�b��SP1 */
				case BD_RINGNIBELUNGEN:			/* �j?�x�����O�̎w�� 3�b��SP1 */
				case BD_SIEGFRIED:				/* �s���g�̃W?�N�t��?�h 3�b��SP1 */
				case BA_DISSONANCE:				/* �s���a�� 3�b��SP1 */
				case BA_ASSASSINCROSS:			/* �[�z�̃A�T�V���N���X 3�b��SP1 */
				case DC_UGLYDANCE:				/* ��������ȃ_���X 3�b��SP1 */
					s=3;
					break;
				case BD_LULLABY:				/* �q��� 4�b��SP1 */
				case BD_ETERNALCHAOS:			/* �i���̍��� 4�b��SP1 */
				case BD_ROKISWEIL:				/* ���L�̋��� 4�b��SP1 */
				case DC_FORTUNEKISS:			/* �K�^�̃L�X 4�b��SP1 */
					s=4;
					break;
				case BD_INTOABYSS:				/* �[���̒��� 5�b��SP1 */
				case BA_WHISTLE:				/* ���J 5�b��SP1 */
				case DC_HUMMING:				/* �n�~���O 5�b��SP1 */
				case BA_POEMBRAGI:				/* �u���M�̎� 5�b��SP1 */
				case DC_SERVICEFORYOU:			/* �T?�r�X�t�H?��? 5�b��SP1 */
				case CG_HERMODE:				// Wand of Hermod
					s=5;
					break;
				case BA_APPLEIDUN:				/* �C�h�D���̗ь� 6�b��SP1 */
					s=6;
					break;
				case DC_DONTFORGETME:			/* ����Y��Ȃ��Łc 10�b��SP1 */
				case CG_MOONLIT:				/* ������̐�ɗ�����Ԃт� 10�b��SP1�H */
					s=10;
					break;
				}
				if (s && ((sc_data[type].val3 % s) == 0)) {
					if (sc_data[SC_LONGING].timer != -1 ||
						sc_data[type].val1 == CG_HERMODE) {
						sp = s;						
					}
					if (sp > sd->status.sp)
						sd->status.sp  = 0;
					else
					sd->status.sp -= sp;
					clif_updatestatus(*sd,SP_SP);
				}
				/* �^�C�}?�Đݒ� */
				sc_data[type].timer=add_timer(1000+tick, status_change_timer,bl->id, data);
				return 0;
			}
		}
		break;

	case SC_BERSERK:		/* �o?�T?�N */
		if(sd){		/* HP��100�ȏ�Ȃ�?? */
			if( (sd->status.hp - sd->status.max_hp*5/100) > 100 ){	// 5% every 10 seconds [DracoRPG]
				sd->status.hp -= sd->status.max_hp*5/100;	// changed to max hp [celest]
				clif_updatestatus(*sd,SP_HP);
				/* �^�C�}?�Đݒ� */
				sc_data[type].timer = add_timer(10000+tick, status_change_timer,bl->id, data);
				return 0;
			}
		}
		break;
	case SC_WEDDING:	//�����p(�����ߏւɂȂ���?���̂�?���Ƃ�)
		if(sd){
			time_t timer;
			if(time(&timer) < ((sc_data[type].val2) + 3600)){	//1���Ԃ����Ă��Ȃ��̂�??
				/* �^�C�}?�Đݒ� */
				sc_data[type].timer=add_timer(10000+tick, status_change_timer,bl->id, data);
				return 0;
			}
		}
		break;
	case SC_NOCHAT:	//�`���b�g�֎~?��
		if(sd && battle_config.muting_players){
			time_t timer;
			if((++sd->status.manner) && time(&timer) < ((sc_data[type].val2) + 60*(0-sd->status.manner))){	//�J�n����status.manner��?���ĂȂ��̂�??
				clif_updatestatus(*sd,SP_MANNER);
				/* �^�C�}?�Đݒ�(60�b) */
				sc_data[type].timer=add_timer(60000+tick, status_change_timer,bl->id, data);
				return 0;
			}
		}
		break;

	case SC_SPLASHER:
		if (sc_data[type].val4 % 1000 == 0) {
			char timer[2];
			sprintf (timer, "%ld", sc_data[type].val4/1000);
			clif_message(*bl, timer);
		}
		if((sc_data[type].val4 -= 500) > 0) {
			sc_data[type].timer = add_timer(500 + tick, status_change_timer,bl->id, data);
				return 0;
		}
		break;

	case SC_MARIONETTE:		/* �}���I�l�b�g�R���g��?�� */
	case SC_MARIONETTE2:
		{
			struct block_list *pbl = map_id2bl(sc_data[type].val3);
			if(pbl && battle_check_range(bl, pbl, 7) && (sc_data[type].val2 -= 1000)>0) {
				sc_data[type].timer = add_timer(1000 + tick, status_change_timer,bl->id, data);
					return 0;
			}
		}
		break;

	// Celest
	case SC_CONFUSION:
		{
			int i = 3000;
			if(sd) 
			{
				pc_randomwalk (*sd, gettick());
				sd->next_walktime = tick + (i=1000 + rand()%1000);
			} 
			else if(md && md->mode&1 && mob_can_move(*md)) 
			{
				md->state.state=MS_WALK;
				if( DIFF_TICK(md->next_walktime,tick) > 7000 &&
					(md->walkpath.path_len==0 || md->walkpath.path_pos>=md->walkpath.path_len) )
					md->next_walktime = tick + 3000*rand()%2000;
				mob_randomwalk(*md,tick);
			}
			if((sc_data[type].val2 -= 1000) > 0) 
			{
				sc_data[type].timer = add_timer(i + tick, status_change_timer,bl->id, data);
					return 0;
			}
		}
		break;

	case SC_GOSPEL:
		{
			int calc_flag = 0;
			if (sc_data[type].val3 > 0) {
				sc_data[type].val3 = 0;
				calc_flag = 1;
			}
			if(sd && sc_data[type].val4 == BCT_SELF){
				int hp, sp;
				hp = (sc_data[type].val1 > 5) ? 45 : 30;
				sp = (sc_data[type].val1 > 5) ? 35 : 20;
				if(sd->status.hp - hp > 0 &&
					sd->status.sp - sp > 0){
					sd->status.hp -= hp;
					sd->status.sp -= sp;
					clif_updatestatus(*sd,SP_HP);
					clif_updatestatus(*sd,SP_SP);
					if ((sc_data[type].val2 -= 10000) > 0) {
						sc_data[type].timer = add_timer(10000+tick, status_change_timer,bl->id, data);
						return 0;
					}
				}
			} else if (sd && sc_data[type].val4 == BCT_PARTY) {
				int i;
				switch ((i = rand() % 12)) {
				case 1: // heal between 100-1000
					{
						struct block_list tbl;
						int heal = rand() % 900 + 100;
						tbl.id = 0;
						tbl.m = bl->m;
						tbl.x = bl->x;
						tbl.y = bl->y;
						clif_skill_nodamage(tbl,*bl,AL_HEAL,heal,1);
						battle_heal(NULL,bl,heal,0,0);
					}
					break;
				case 2: // end negative status
					status_change_clear_debuffs (bl);
					break;
				case 3:	// +25% resistance to negative status
				case 4: // +25% max hp
				case 5: // +25% max sp
				case 6: // +2 to all stats
				case 11: // +25% armor and vit def
				case 12: // +8% atk
				case 13: // +5% flee
				case 14: // +5% hit
					sc_data[type].val3 = i;
					if (i == 6 ||
						(i >= 11 && i <= 14))
						calc_flag = 1;
					break;
				case 7: // level 5 bless
					{
						struct block_list tbl;
						tbl.id = 0;
						tbl.m = bl->m;
						tbl.x = bl->x;
						tbl.y = bl->y;
						clif_skill_nodamage(tbl,*bl,AL_BLESSING,5,1);
						status_change_start(bl,SkillStatusChangeTable[AL_BLESSING],5,0,0,0,10000,0 );
					}
					break;
				case 8: // level 5 increase agility
					{
						struct block_list tbl;
						tbl.id = 0;
						tbl.m = bl->m;
						tbl.x = bl->x;
						tbl.y = bl->y;
						clif_skill_nodamage(tbl,*bl,AL_INCAGI,5,1);
						status_change_start(bl,SkillStatusChangeTable[AL_INCAGI],5,0,0,0,10000,0 );
					}
					break;
				case 9: // holy element to weapon
					{
						struct block_list tbl;
						tbl.id = 0;
						tbl.m = bl->m;
						tbl.x = bl->x;
						tbl.y = bl->y;
						clif_skill_nodamage(tbl,*bl,PR_ASPERSIO,1,1);
						status_change_start(bl,SkillStatusChangeTable[PR_ASPERSIO],1,0,0,0,10000,0 );
					}
					break;
				case 10: // holy element to armour
					{
						struct block_list tbl;
						tbl.id = 0;
						tbl.m = bl->m;
						tbl.x = bl->x;
						tbl.y = bl->y;
						clif_skill_nodamage(tbl,*bl,PR_BENEDICTIO,1,1);
						status_change_start(bl,SkillStatusChangeTable[PR_BENEDICTIO],1,0,0,0,10000,0 );
					}
					break;
				default:
					break;
				}
			} else if (sc_data[type].val4 == BCT_ENEMY) {
				int i;
				switch ((i = rand() % 8)) {
				case 1: // damage between 300-800
				case 2: // damage between 150-550 (ignore def)
					battle_damage(NULL, bl, rand() % 500,0); // temporary damage
					break;
				case 3: // random status effect
					{
						int effect[3] = {
							SC_CURSE,
							SC_BLIND,
							SC_POISON };
						status_change_start(bl,effect[rand()%3],1,0,0,0,10000,0 );
					}
					break;
				case 4: // level 10 provoke
					{
						struct block_list tbl;
						tbl.id = 0;
						tbl.m = bl->m;
						tbl.x = bl->x;
						tbl.y = bl->y;
						clif_skill_nodamage(tbl,*bl,SM_PROVOKE,1,1);
						status_change_start(bl,SkillStatusChangeTable[SM_PROVOKE],10,0,0,0,10000,0 );
					}
					break;
				case 5: // 0 def
				case 6: // 0 atk
				case 7: // 0 flee
				case 8: // -75% move speed and aspd
					sc_data[type].val3 = i;
					calc_flag = 1;
					break;
				default:
					break;
				}
			}
			if (sd && calc_flag)
				status_calc_pc (*sd, 0);
		}
		break;

	case SC_GUILDAURA:
		{
			struct block_list *tbl = map_id2bl(sc_data[type].val2);
			if( tbl && battle_check_range(bl, tbl, 2) )
			{
				sc_data[type].timer = add_timer(1000 + tick, status_change_timer,bl->id, data);
					return 0;
		}
		}
		break;
	}
	
	// default for all non-handled control paths
	// security system to prevent forgetting timer removal
	sc_data[type].timer = temp_timerid; // to have status_change_end handle a valid timer

	return status_change_end( bl,type,tid );
}

/*==========================================
 * �X�e�[�^�X�ُ�^�C�}�[�͈͏���
 *------------------------------------------
 */
int status_change_timer_sub(struct block_list &bl, va_list ap )
{
	struct block_list *src;
	int type;
	unsigned long tick;

	nullpo_retr(0, ap);
	nullpo_retr(0, src=va_arg(ap,struct block_list*));
	type=va_arg(ap,int);
	tick=(unsigned long)va_arg(ap,int);

	if(bl.type!=BL_PC && bl.type!=BL_MOB)
		return 0;

	switch( type ){
	case SC_SIGHT:	/* �T�C�g */
	case SC_CONCENTRATE:
		if( (*status_get_option(&bl))&6 ){
			status_change_end( &bl, SC_HIDING, -1);
			status_change_end( &bl, SC_CLOAKING, -1);
		}
		break;
	case SC_RUWACH:	/* ���A�t */
		if( (*status_get_option(&bl))&6 ){
			struct status_change *sc_data = status_get_sc_data(&bl);	// check whether the target is hiding/cloaking [celest]
			if (sc_data && (sc_data[SC_HIDING].timer != -1 ||	// if the target is using a special hiding, i.e not using normal hiding/cloaking, don't bother
				sc_data[SC_CLOAKING].timer != -1)) {
				status_change_end( &bl, SC_HIDING, -1);
				status_change_end( &bl, SC_CLOAKING, -1);
				if(battle_check_target( src, &bl, BCT_ENEMY ) > 0)
					skill_attack(BF_MAGIC,src,src,&bl,AL_RUWACH,1,tick,0);
			}
		}
		break;
	}
	return 0;
}

int status_change_clear_buffs (struct block_list *bl)
{
	int i;
	struct status_change *sc_data = status_get_sc_data(bl);
	if (!sc_data)
		return 0;		
	for (i = 0; i <= 26; i++) {
		if(sc_data[i].timer != -1)
			status_change_end(bl,i,-1);
	}
	for (i = 37; i <= 44; i++) {
		if(sc_data[i].timer != -1)
			status_change_end(bl,i,-1);
	}
	for (i = 46; i <= 73; i++) {
		if(sc_data[i].timer != -1)
			status_change_end(bl,i,-1);
	}
	for (i = 90; i <= 93; i++) {
		if(sc_data[i].timer != -1)
			status_change_end(bl,i,-1);
	}
	for (i = 103; i <= 106; i++) {
		if(sc_data[i].timer != -1)
			status_change_end(bl,i,-1);
	}
	for (i = 109; i <= 132; i++) {
		if(sc_data[i].timer != -1)
			status_change_end(bl,i,-1);
	}
	for (i = 172; i <= 188; i++) {
		if(sc_data[i].timer != -1)
			status_change_end(bl,i,-1);
	}
	return 0;
}
int status_change_clear_debuffs (struct block_list *bl)
{
	int i;
	struct status_change *sc_data = status_get_sc_data(bl);
	if (!sc_data)
		return 0;
	for (i = SC_STONE; i <= SC_DPOISON; i++) {
		if(sc_data[i].timer != -1)
			status_change_end(bl,i,-1);
	}

	return 0;
}

int status_calc_sigma(void)
{
	int i,j,k;

	for(i=0;i<MAX_PC_CLASS;i++) {
		memset(hp_sigma_val[i],0,sizeof(hp_sigma_val[i]));
		for(k=0,j=2;j<=MAX_LEVEL;j++) {
			k += hp_coefficient[i]*j + 50;
			k -= k%100;
			hp_sigma_val[i][j-1] = k;
		}
	}
	return 0;
}

int status_readdb(void) {
	int i,j,k;
	FILE *fp;
	char line[1024],*p;

	// JOB�␳?�l�P
	fp=safefopen("db/job_db1.txt","r");
	if(fp==NULL){
		ShowWarning("can't read db/job_db1.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line, sizeof(line)-1, fp)){
		char *split[50];
		if( !skip_empty_line(line) )
			continue;
		for(j=0,p=line;j<21 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(j<21)
			continue;
		max_weight_base[i]=atoi(split[0]);
		hp_coefficient[i]=atoi(split[1]);
		hp_coefficient2[i]=atoi(split[2]);
		sp_coefficient[i]=atoi(split[3]);
		for(j=0;j<17;j++)
			aspd_base[i][j]=atoi(split[j+4]);
		i++;
// -- moonsoul (below two lines added to accommodate high numbered new class ids)
		if(i==24)
			i=4001;
		if(i==MAX_PC_CLASS)
			break;
	}
	fclose(fp);
	ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n","db/job_db1.txt");

	// JOB�{?�i�X
	memset(job_bonus,0,sizeof(job_bonus));
	fp=safefopen("db/job_db2.txt","r");
	if(fp==NULL){
		ShowWarning("can't read db/job_db2.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line, sizeof(line)-1, fp)){
		if( !skip_empty_line(line) )
			continue;
		for(j=0,p=line;j<MAX_LEVEL && p;j++){
			if(sscanf(p,"%d",&k)==0)
				break;
			job_bonus[0][i][j]=k;
			job_bonus[2][i][j]=k; //�{�q�E�̃{?�i�X�͕�����Ȃ��̂�?
			p=strchr(p,',');
			if(p) p++;
		}
		i++;
// -- moonsoul (below two lines added to accommodate high numbered new class ids)
		if(i==24)
			i=4001;
		if(i==MAX_PC_CLASS)
			break;
	}
	fclose(fp);
	ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n","db/job_db2.txt");

	// JOB�{?�i�X2 ?���E�p
	fp=safefopen("db/job_db2-2.txt","r");
	if(fp==NULL){
		ShowWarning("can't read db/job_db2-2.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line, sizeof(line)-1, fp)){
		if( !skip_empty_line(line) )
			continue;
		for(j=0,p=line;j<MAX_LEVEL && p;j++){
			if(sscanf(p,"%d",&k)==0)
				break;
			job_bonus[1][i][j]=k;
			p=strchr(p,',');
			if(p) p++;
		}
		i++;
		if(i==MAX_PC_CLASS)
			break;
	}
	fclose(fp);
	ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n","db/job_db2-2.txt");

	// �T�C�Y�␳�e?�u��
	for(i=0;i<3;i++)
		for(j=0;j<20;j++)
			atkmods[i][j]=100;
	fp=safefopen("db/size_fix.txt","r");
	if(fp==NULL){
		ShowWarning("can't read db/size_fix.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line, sizeof(line)-1, fp)){
		char *split[20];
		if( !skip_empty_line(line) )
			continue;
		if(atoi(line)<=0)
			continue;
		memset(split,0,sizeof(split));
		for(j=0,p=line;j<20 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		for(j=0;j<20 && split[j];j++)
			atkmods[i][j]=atoi(split[j]);
		i++;
	}
	fclose(fp);
	ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n","db/size_fix.txt");

	// ��?�f?�^�e?�u��
	for(i=0;i<MAX_REFINE_BONUS;i++){
		for(j=0;j<MAX_REFINE; j++)
			percentrefinery[i][j]=100;
		percentrefinery[i][j]=0; //Slot MAX+1 always has 0% success chance [Skotlex]
		refinebonus[i][0]=0;
		refinebonus[i][1]=0;
		refinebonus[i][2]=10;
	}
	fp=safefopen("db/refine_db.txt","r");
	if(fp==NULL){
		ShowWarning("can't read db/refine_db.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line, sizeof(line)-1, fp) && i<MAX_REFINE_BONUS){
		char *split[16];
		if( !skip_empty_line(line) )
			continue;
		if(atoi(line)<=0)
			continue;
		memset(split,0,sizeof(split));
		for(j=0,p=line;j<16 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		refinebonus[i][0]=atoi(split[0]);	// ��?�{?�i�X
		refinebonus[i][1]=atoi(split[1]);	// ��?��?�{?�i�X
		refinebonus[i][2]=atoi(split[2]);	// ���S��?���E
		for(j=0;j<MAX_REFINE && split[j];j++)
			percentrefinery[i][j]=atoi(split[j+3]);
		i++;
	}
	fclose(fp); //Lupus. close this file!!!
	ShowStatus("Done reading '"CL_WHITE"%s"CL_RESET"'.\n","db/refine_db.txt");

	return 0;
}

/*==========================================
 * �X�L���֌W����������
 *------------------------------------------
 */
int do_init_status(void)
{
	add_timer_func_list(status_change_timer,"status_change_timer");
	status_readdb();
	status_calc_sigma();
	return 0;
}