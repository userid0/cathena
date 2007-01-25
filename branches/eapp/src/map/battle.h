// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _BATTLE_H_
#define _BATTLE_H_

// �_���[�W
struct Damage
{
	int damage;
	int damage2;
	int type;
	int div_;
	int amotion;
	int dmotion;
	int blewcount;
	int flag;
	int dmg_lv;	//�͂܂ꌸ�Z�v�Z�p�@0:�X�L���U�� ATK_LUCKY,ATK_FLEE,ATK_DEF

	Damage() :
		damage(0),
		damage2(0),
		type(0),
		div_(0),
		amotion(0),
		dmotion(0),
		blewcount(0),
		flag(0),
		dmg_lv(0)
	{}
};

// �����\�i�ǂݍ��݂�pc.c�Abattle_attr_fix�Ŏg�p�j
extern int attr_fix_table[4][10][10];

struct map_session_data;
struct mob_data;
struct block_list;

// �_���[�W�v�Z

struct Damage battle_calc_attack(	int attack_type,
	block_list *bl,block_list *target,int skill_num,int skill_lv,int flag);
struct Damage battle_calc_weapon_attack(
	block_list *bl,block_list *target,int skill_num,int skill_lv,int flag);
struct Damage battle_calc_magic_attack(
	block_list *bl,block_list *target,int skill_num,int skill_lv,int flag);
struct Damage  battle_calc_misc_attack(
	block_list *bl,block_list *target,int skill_num,int skill_lv,int flag);

// �����C���v�Z
int battle_attr_fix(int damage,int atk_elem,int def_elem);

// �_���[�W�ŏI�v�Z
int battle_calc_damage(block_list *src,block_list *bl,int damage,int div_,int skill_num,short skill_lv,int flag);
enum {	// �ŏI�v�Z�̃t���O
	BF_WEAPON	= 0x0001,
	BF_MAGIC	= 0x0002,
	BF_MISC		= 0x0004,
	BF_SHORT	= 0x0010,
	BF_LONG		= 0x0040,
	BF_SKILL	= 0x0100,
	BF_NORMAL	= 0x0200,
	BF_WEAPONMASK=0x000f,
	BF_RANGEMASK= 0x00f0,
	BF_SKILLMASK= 0x0f00
};

// ���ۂ�HP�𑝌�
int battle_delay_damage(unsigned long tick, block_list &src, block_list &target, int damage, int flag);
int battle_damage(block_list *bl,block_list *target,int damage,int flag);
int battle_heal(block_list *bl,block_list *target,int hp,int sp,int flag);


// �ʏ�U�������܂Ƃ�
int battle_weapon_attack( block_list *bl,block_list *target,unsigned long tick,int flag);

// �e��p�����[�^�𓾂�
unsigned int battle_counttargeted(block_list &bl,block_list *src, unsigned short target_lv);
block_list* battle_gettargeted(block_list &target);


enum {
	BCT_NOONE	=0x000000,
	BCT_SELF	=0x010000,
	BCT_ENEMY	=0x020000,
	BCT_NOENEMY	=0x1d0000,//(~BCT_ENEMY&BCT_ALL)
	BCT_PARTY	=0x050000,//Party includes self (0x04|0x01)
	BCT_NOPARTY	=0x1b0000,//(~BCT_PARTY&BCT_ALL)
	BCT_GUILD	=0x090000,//Guild includes self (0x08|0x01)
	BCT_NOGUILD	=0x170000,//(~BCT_GUILD&BCT_ALL)
	BCT_ALL		=0x1f0000,
	BCT_NEUTRAL =0x100000
};



int battle_check_target(const block_list *src, const block_list *target, int flag);
bool battle_check_range(const block_list *src, const block_list *bl, unsigned int range);
void battle_init();


#endif
