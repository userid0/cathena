// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _MAPOBJ_H_
#define _MAPOBJ_H_

#include "mmo.h"
#include "socket.h"
#include "db.h"

#include "coordinate.h"
#include "path.h"


#define MAX_QUICKOBJ 500000



///////////////////////////////////////////////////////////////////////////////
enum object_t { BL_NUL=0, BL_ALL=0, BL_PC, BL_NPC, BL_MOB, BL_ITEM, BL_CHAT, BL_SKILL, BL_PET, BL_HOM };	//xxx 0..7 -> 3bit
enum object_sub_t { WARP, SHOP, SCRIPT, MONS };											// 0..3 -> 2bit


///////////////////////////////////////////////////////////////////////////////
/// skill fail types.
enum skillfail_t
{
	SF_FAILED	= 0x0000,	//	btype==0 "skill failed"
	SF_NOEMOTION= 0x0100,	//	btype==1 "no emotions"
	SF_NOSIT	= 0x0200,	//	btype==2 "no sit"
	SF_NOCHAT	= 0x0300,	//	btype==3 "no chat"
	SF_NOPARTY	= 0x0400,	//	btype==4 "no party"
	SF_NOSHOUT	= 0x0500,	//	btype==5 "no shout"
	SF_NOPKING	= 0x0600,	//	btype==6 "no PKing"
	SF_NOALLIGN	= 0x0700,	//	btype==7 "no alligning"
	SF_SP		= 0x0001,	//	type==1 "insufficient SP"
	SF_HP		= 0x0002,	//	type==2 "insufficient HP"
	SF_MATERIAL	= 0x0003,	//	type==3 "insufficient materials"
	SF_DELAY	= 0x0004,	//	type==4 "there is a delay after using a skill"
	SF_ZENY		= 0x0005,	//	type==5 "insufficient zeny"
	SF_WEAPON	= 0x0006,	//	type==6 "wrong weapon"
	SF_REDGEM	= 0x0007,	//	type==7 "red jemstone needed"
	SF_BLUEGEM	= 0x0008,	//	type==8 "blue jemstone needed"
	SF_WEIGHT	= 0x0009,	//	type==9 "overweight"
	SF_FAILED2	= 0x0010,	//	type==10 "skill failed"
	SF_IGNORED	= 0x0011	//  type==11 ignored
};


enum status_t {	// map_session_data �� status_change�̔�?�e?�u��
// MAX_STATUSCHANGE��?�̓N���C�A���g�ւ̒ʒm����B
// 2-2���E�̒l�͂Ȃ񂩂߂��Ⴍ������ۂ��̂Ŏb��B���Ԃ�?�X����܂��B

	SC_PROVOKE			= 0,
	SC_ENDURE			= 1,
	SC_TWOHANDQUICKEN	= 2,
	SC_CONCENTRATE		= 3,
	SC_HIDING			= 4,
	SC_CLOAKING			= 5,
	SC_ENCPOISON		= 6,
	SC_POISONREACT		= 7,
	SC_QUAGMIRE			= 8,
	SC_ANGELUS			= 9,
	SC_BLESSING			= 10,
	SC_SIGNUMCRUCIS		= 11,
	SC_INCREASEAGI		= 12,
	SC_DECREASEAGI		= 13,
	SC_SLOWPOISON		= 14,
	SC_IMPOSITIO		= 15,
	SC_SUFFRAGIUM		= 16,
	SC_ASPERSIO			= 17,
	SC_BENEDICTIO		= 18,
	SC_KYRIE			= 19,
	SC_MAGNIFICAT		= 20,
	SC_GLORIA			= 21,
	SC_AETERNA			= 22,
	SC_ADRENALINE		= 23,
	SC_WEAPONPERFECTION	= 24,
	SC_OVERTHRUST		= 25,
	SC_MAXIMIZEPOWER	= 26,
	SC_RIDING			= 27,
	SC_FALCON			= 28,
	SC_TRICKDEAD		= 29,
	SC_LOUD				= 30,
	SC_ENERGYCOAT		= 31,
	SC_BROKNARMOR		= 32,
	SC_BROKNWEAPON		= 33,
	SC_HALLUCINATION	= 34,
	SC_WEIGHT50			= 35,
	SC_WEIGHT90			= 36,
	SC_SPEEDPOTION0		= 37,
	SC_SPEEDPOTION1		= 38,
	SC_SPEEDPOTION2		= 39,
	SC_SPEEDPOTION3		= 40,
	SC_SPEEDUP0			= 41, // for skill speedup
	SC_SPEEDUP1			= 42, // for skill speedup
	SC_ATKPOT			= 43,	// [Valaris]
	SC_MATKPOT			= 44,	// [Valaris]
	SC_WEDDING			= 45,	//�����p(�����ߏւɂȂ���?���̂�?���Ƃ�)
	SC_SLOWDOWN			= 46,
	SC_ANKLE			= 47,
	SC_KEEPING			= 48,
	SC_BARRIER			= 49,
	SC_STRIPWEAPON		= 50,
	SC_STRIPSHIELD		= 51,
	SC_STRIPARMOR		= 52,
	SC_STRIPHELM		= 53,
	SC_CP_WEAPON		= 54,
	SC_CP_SHIELD		= 55,
	SC_CP_ARMOR			= 56,
	SC_CP_HELM			= 57,
	SC_AUTOGUARD		= 58,
	SC_REFLECTSHIELD	= 59,
	SC_SPLASHER			= 60,	// �x�i���X�v���b�V��? 
	SC_PROVIDENCE		= 61,
	SC_DEFENDER			= 62,
	SC_MAGICROD			= 63,
	SC_SPELLBREAKER		= 64,
	SC_AUTOSPELL		= 65,
	SC_SIGHTTRASHER		= 66,
	SC_AUTOBERSERK		= 67,
	SC_SPEARSQUICKEN	= 68,
	SC_AUTOCOUNTER		= 69,
	SC_SIGHT			= 70,
	SC_SAFETYWALL		= 71,
	SC_RUWACH			= 72,	
	SC_PNEUMA			= 73,

	SC_STONE			= 74,
	SC_FREEZE			= 75,
	SC_STUN				= 76,
	SC_SLEEP			= 77,
	SC_POISON			= 78,
	SC_CURSE			= 79,
	SC_SILENCE			= 80,
	SC_CONFUSION		= 81,
	SC_BLIND			= 82,
	SC_DIVINA			= SC_SILENCE,
	SC_BLEEDING			= 83,
	SC_DPOISON			= 84,		// �ғ� 

	SC_EXTREMITYFIST	= 85,
	SC_EXPLOSIONSPIRITS	= 86,
	SC_COMBO			= 87,
	SC_BLADESTOP_WAIT	= 88,
	SC_BLADESTOP		= 89,
	SC_FLAMELAUNCHER	= 90,
	SC_FROSTWEAPON		= 91,
	SC_LIGHTNINGLOADER	= 92,
	SC_SEISMICWEAPON	= 93,	
	SC_VOLCANO			= 94,
	SC_DELUGE			= 95,
	SC_VIOLENTGALE		= 96,
	SC_WATK_ELEMENT		= 97,
	SC_LANDPROTECTOR	= 98,
// 99
	SC_NOCHAT			= 100,	//�ԃG��?��
	SC_BABY				= 101,
// <-- 102 = gloria
	SC_AURABLADE		= 103, // �I?���u��?�h 
	SC_PARRYING			= 104, // �p���C���O 
	SC_CONCENTRATION	= 105, // �R���Z���g��?�V���� 
	SC_TENSIONRELAX		= 106, // �e���V���������b�N�X 
	SC_BERSERK			= 107, // �o?�T?�N 
	SC_FURY				= 108,
	SC_GOSPEL			= 109,
	SC_ASSUMPTIO		= 110, // �A�V�����v�e�B�I 
	SC_BASILICA			= 111,
	SC_GUILDAURA        = 112,
	SC_MAGICPOWER		= 113, // ���@��?�� 
	SC_EDP				= 114, // �G�t�F�N�g������������ړ� 
	SC_TRUESIGHT		= 115, // �g�D��?�T�C�g 
	SC_WINDWALK			= 116, // �E�C���h�E�H?�N 
	SC_MELTDOWN			= 117, // �����g�_�E�� 
	SC_CARTBOOST		= 118, // �J?�g�u?�X�g 
	SC_CHASEWALK		= 119,
	SC_REJECTSWORD		= 120, // ���W�F�N�g�\?�h 
	SC_MARIONETTE		= 121, // �}���I�l�b�g�R���g��?�� 
	SC_MARIONETTE2		= 122, // Marionette target
	SC_MOONLIT			= 123,
	SC_HEADCRUSH		= 124, // �w�b�h�N���b�V�� 
	SC_JOINTBEAT		= 125, // �W���C���g�r?�g 
	SC_MINDBREAKER		= 126,
	SC_MEMORIZE			= 127,		// �������C�Y 
	SC_FOGWALL			= 128,
	SC_SPIDERWEB		= 129,		// �X�p�C�_?�E�F�b�u 
	SC_DEVOTION			= 130,
	SC_SACRIFICE		= 131,	// �T�N���t�@�C�X 
	SC_STEELBODY		= 132,
// 133
// 134 = wobbles the character's sprite when SC starts or ends
	SC_READYSTORM		= 135,
	SC_STORMKICK		= 136,
	SC_READYDOWN		= 137,
	SC_DOWNKICK			= 138,
	SC_READYCOUNTER		= 139,
    SC_COUNTER			= 140,
	SC_READYTURN		= 141,
	SC_TURNKICK			= 142,
	SC_DODGE			= 143,
// 144
	SC_RUN				= 145,
// 146 = korean letter
	SC_ADRENALINE2      = 147,
// 148 = another korean letter
	SC_DANCING			= 149,
	SC_LULLABY			= 150,	//0x96
	SC_RICHMANKIM		= 151,
	SC_ETERNALCHAOS		= 152,
	SC_DRUMBATTLE		= 153,
	SC_NIBELUNGEN		= 154,
	SC_ROKISWEIL		= 155,
	SC_INTOABYSS		= 156,
	SC_SIEGFRIED		= 157,
	SC_DISSONANCE		= 158,	//0x9E
	SC_WHISTLE			= 159,
	SC_ASSNCROS			= 160,
	SC_POEMBRAGI		= 161,
	SC_APPLEIDUN		= 162,
	SC_UGLYDANCE		= 163,
	SC_HUMMING			= 164,
	SC_DONTFORGETME		= 165,
	SC_FORTUNE			= 166,	//0xA6
	SC_SERVICE4U		= 167,
// 168
// 169 = sun
// 170 = moon
// 171 = stars
	SC_INCALLSTATUS		= 172,		// �S�ẴX�e�[�^�X���㏸(���̂Ƃ���S�X�y���p) 
	SC_INCHIT			= 173,		// HIT�㏸(���̂Ƃ���S�X�y���p) 
	SC_INCFLEE			= 174,		// FLEE�㏸(���̂Ƃ���S�X�y���p) 
	SC_INCMHP2			= 175,		// MHP��%�㏸(���̂Ƃ���S�X�y���p) 
	SC_INCMSP2			= 176,		// MSP��%�㏸(���̂Ƃ���S�X�y���p) 
	SC_INCATK2			= 177,		// ATK��%�㏸(���̂Ƃ���S�X�y���p) 
	SC_INCMATK2			= 178,		// ATK��%�㏸(���̂Ƃ���S�X�y���p) 
	SC_INCHIT2			= 179,		// HIT��%�㏸(���̂Ƃ���S�X�y���p) 
	SC_INCFLEE2			= 180,		// FLEE��%�㏸(���̂Ƃ���S�X�y���p) 
	SC_PRESERVE         = 181,
	SC_BATTLEORDERS		= 182,	// unsure
	SC_REGENERATION		= 184,
// 184
// 185
	SC_DOUBLECAST		= 186,
	SC_GRAVITATION		= 187,
	SC_MAXOVERTHRUST	= 188,
	SC_LONGING			= 189,
	SC_HERMODE			= 190,
	SC_TAROT			= 191,	// unsure
//<-- 192 = gloria
//<-- 193 = gloria
	SC_INCDEF2			= 194,
	SC_INCSTR			= 195,
	SC_INCAGI			= 196,
	SC_INCVIT			= 197,
	SC_INCINT			= 198,
	SC_INCDEX			= 199,
	SC_INCLUK			= 200,
// <-- 201 = two hand quicken
/*
	SC_PROVOKE				= 0,	// �v���{�b�N 
	SC_ENDURE				= 1,	// �C���f���A 
	SC_TWOHANDQUICKEN		= 2,	// �c�[�n���h�N�C�b�P�� 
	SC_CONCENTRATE			= 3,	// �W���͌��� 
	SC_HIDING				= 4,	// �n�C�f�B���O 
	SC_CLOAKING				= 5,	// �N���[�L���O 
	SC_ENCPOISON			= 6,	// �G���`�����g�|�C�Y�� 
	SC_POISONREACT			= 7,	// �|�C�Y�����A�N�g 
	SC_QUAGMIRE				= 8,	// �N�@�O�}�C�A 
	SC_ANGELUS				= 9,	// �G���W�F���X 
	SC_BLESSING				=10,	// �u���b�V���O 
	SC_SIGNUMCRUCIS			=11,	// �V�O�i���N���V�X�H 
	SC_INCREASEAGI			=12,	//  
	SC_DECREASEAGI			=13,	//  
	SC_SLOWPOISON			=14,	// �X���[�|�C�Y�� 
	SC_IMPOSITIO			=15,	// �C���|�V�e�B�I�}�k�X 
	SC_SUFFRAGIUM			=16,	// �T�t���M�E�� 
	SC_ASPERSIO				=17,	// �A�X�y���V�I 
	SC_BENEDICTIO			=18,	// ���̍~�� 
	SC_KYRIE				=19,	// �L���G�G���C�\�� 
	SC_MAGNIFICAT			=20,	// �}�O�j�t�B�J�[�g 
	SC_GLORIA				=21,	// �O�����A 
	SC_AETERNA				=22,	//  
	SC_ADRENALINE			=23,	// �A�h���i�������b�V�� 
	SC_WEAPONPERFECTION		=24,	// �E�F�|���p�[�t�F�N�V���� 
	SC_OVERTHRUST			=25,	// �I�[�o�[�g���X�g 
	SC_MAXIMIZEPOWER		=26,	// �}�L�V�}�C�Y�p���[ 
	SC_RIDING				=27,	// ���C�f�B���O 
	SC_FALCON				=28,	// �t�@���R���}�X�^���[ 
	SC_TRICKDEAD			=29,	// ���񂾂ӂ� 
	SC_LOUD					=30,	// ���E�h�{�C�X 
	SC_ENERGYCOAT			=31,	// �G�i�W�[�R�[�g 
	SC_PK_PENALTY			=32,	//PK�̃y�i���e�B
	SC_REVERSEORCISH		=33,    //���o�[�X�I�[�L�b�V��
	SC_HALLUCINATION		=34,	// �n���l�[�V�����E�H�[�N�H 
	SC_WEIGHT50				=35,	// �d��50���I�[�o�[ 
	SC_WEIGHT90				=36,	// �d��90���I�[�o�[ 
	SC_SPEEDPOTION0			=37,	// ���x�|�[�V�����H 
	SC_SPEEDPOTION1			=38,	// �X�s�[�h�A�b�v�|�[�V�����H 
	SC_SPEEDPOTION2			=39,	// �n�C�X�s�[�h�|�[�V�����H 
	SC_SPEEDPOTION3			=40,	// �o�[�T�[�N�|�[�V���� 
	SC_ITEM_DELAY			=41,
	//
	//
	//
	//
	//
	//
	//
	//
	SC_STRIPWEAPON			=50,	// �X�g���b�v�E�F�|�� 
	SC_STRIPSHIELD			=51,	// �X�g���b�v�V�[���h 
	SC_STRIPARMOR			=52,	// �X�g���b�v�A�[�}�[ 
	SC_STRIPHELM			=53,	// �X�g���b�v�w���� 
	SC_CP_WEAPON			=54,	// �P�~�J���E�F�|���`���[�W 
	SC_CP_SHIELD			=55,	// �P�~�J���V�[���h�`���[�W 
	SC_CP_ARMOR				=56,	// �P�~�J���A�[�}�[�`���[�W 
	SC_CP_HELM				=57,	// �P�~�J���w�����`���[�W 
	SC_AUTOGUARD			=58,	// �I�[�g�K�[�h 
	SC_REFLECTSHIELD		=59,	// ���t���N�g�V�[���h 
	SC_DEVOTION				=60,	// �f�B�{�[�V���� 
	SC_PROVIDENCE			=61,	// �v�����B�f���X 
	SC_DEFENDER				=62,	// �f�B�t�F���_�[ 
	SC_SANTA				=63,	//�T���^
	//
	SC_AUTOSPELL			=65,	// �I�[�g�X�y�� 
	//
	//
	SC_SPEARSQUICKEN		=68,	// �X�s�A�N�C�b�P�� 
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	SC_EXPLOSIONSPIRITS		=86,	// �����g�� 
	SC_STEELBODY			=87,	// ���� 
	//
	SC_COMBO				=89,
	SC_FLAMELAUNCHER		=90,	// �t���C�������`���[ 
	SC_FROSTWEAPON			=91,	// �t���X�g�E�F�|�� 
	SC_LIGHTNINGLOADER		=92,	// ���C�g�j���O���[�_�[ 
	SC_SEISMICWEAPON		=93,	// �T�C�Y�~�b�N�E�F�|�� 
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	SC_AURABLADE			=103,	// �I�[���u���[�h 
	SC_PARRYING				=104,	// �p���C���O 
	SC_CONCENTRATION		=105,	// �R���Z���g���[�V���� 
	SC_TENSIONRELAX			=106,	// �e���V���������b�N�X 
	SC_BERSERK				=107,	// �o�[�T�[�N 
	//
	//
	//
	SC_ASSUMPTIO			=110,	// �A�X���v�e�B�I 
	//
	//
	SC_MAGICPOWER			=113,	// ���@�͑��� 
	SC_EDP					=114,	// �G�t�F�N�g������������ړ� 
	SC_TRUESIGHT			=115,	// �g�D���[�T�C�g 
	SC_WINDWALK				=116,	// �E�C���h�E�H�[�N 
	SC_MELTDOWN				=117,	// �����g�_�E�� 
	SC_CARTBOOST			=118,	// �J�[�g�u�[�X�g 
	SC_CHASEWALK			=119,	// �`�F�C�X�E�H�[�N 
	SC_REJECTSWORD			=120,	// ���W�F�N�g�\�[�h 
	SC_MARIONETTE			=121,	// �}���I�l�b�g�R���g���[��  //�����p
	SC_MARIONETTE2			=122,	// �}���I�l�b�g�R���g���[��  //�^�[�Q�b�g�p
	//
	SC_HEADCRUSH			=124,	// �w�b�h�N���b�V�� 
	SC_JOINTBEAT			=125,	// �W���C���g�r�[�g 
	//
	//
	SC_STONE				=128,	// ��Ԉُ�F�Ή� 
	SC_FREEZE				=129,	// ��Ԉُ�F�X�� 
	SC_STUN					=130,	// ��Ԉُ�F�X�^�� 
	SC_SLEEP				=131,	// ��Ԉُ�F���� 
	SC_POISON				=132,	// ��Ԉُ�F�� 
	SC_CURSE				=133,	// ��Ԉُ�F�� 
	SC_SILENCE				=134,	// ��Ԉُ�F���� 
	SC_CONFUSION			=135,	// ��Ԉُ�F���� 
	SC_BLIND				=136,	// ��Ԉُ�F�È� 
	SC_BLEED				=137,	// ��Ԉُ�F�o�� 
	SC_DIVINA				= SC_SILENCE,	// ��Ԉُ�F���� 
	//138
	//139
	SC_SAFETYWALL			=140,	// �Z�[�t�e�B�[�E�H�[�� 
	SC_PNEUMA				=141,	// �j���[�} 
	//
	SC_ANKLE				=143,	// �A���N���X�l�A 
	SC_DANCING				=144,	//  
	SC_KEEPING				=145,	//  
	SC_BARRIER				=146,	//  
	//
	//
	SC_MAGICROD				=149,	//  
	SC_SIGHT				=150,	//  
	SC_RUWACH				=151,	//  
	SC_AUTOCOUNTER			=152,	//  
	SC_VOLCANO				=153,	//  
	SC_DELUGE				=154,	//  
	SC_VIOLENTGALE			=155,	//  
	SC_BLADESTOP_WAIT		=156,	//  
	SC_BLADESTOP			=157,	//  
	SC_EXTREMITYFIST		=158,	//  
	SC_GRAFFITI				=159,	//  
	SC_LULLABY				=160,	//  
	SC_RICHMANKIM			=161,	//  
	SC_ETERNALCHAOS			=162,	//  
	SC_DRUMBATTLE			=163,	//  
	SC_NIBELUNGEN			=164,	//  
	SC_ROKISWEIL			=165,	//  
	SC_INTOABYSS			=166,	//  
	SC_SIEGFRIED			=167,	//  
	SC_DISSONANCE			=168,	//  
	SC_WHISTLE				=169,	//  
	SC_ASSNCROS				=170,	//  
	SC_POEMBRAGI			=171,	//  
	SC_APPLEIDUN			=172,	//  
	SC_UGLYDANCE			=173,	//  
	SC_HUMMING				=174,	//  
	SC_DONTFORGETME			=175,	//  
	SC_FORTUNE				=176,	//  
	SC_SERVICE4U			=177,	//  
	SC_BASILICA				=178,	//  
	SC_MINDBREAKER			=179,	//  
	SC_SPIDERWEB			=180,	// �X�p�C�_�[�E�F�b�u 
	SC_MEMORIZE				=181,	// �������C�Y 
	SC_DPOISON				=182,	// �ғ� 
	//
	SC_SACRIFICE			=184,	// �T�N���t�@�C�X 
	SC_INCATK				=185,	//item 682�p
	SC_INCMATK				=186,	//item 683�p
	SC_WEDDING				=187,	//�����p(�����ߏւɂȂ��ĕ����̂��x���Ƃ�)
	SC_NOCHAT				=188,	//�ԃG�����
	SC_SPLASHER				=189,	// �x�i���X�v���b�V���[ 
	SC_SELFDESTRUCTION		=190,	// ���� 
	SC_MAGNUM				=191,	// �}�O�i���u���C�N 
	SC_GOSPEL				=192,	// �S�X�y�� 
	SC_INCALLSTATUS			=193,	// �S�ẴX�e�[�^�X���㏸(���̂Ƃ���S�X�y���p) 
	SC_INCHIT				=194,	// HIT�㏸(���̂Ƃ���S�X�y���p) 
	SC_INCFLEE				=195,	// FLEE�㏸(���̂Ƃ���S�X�y���p) 
	SC_INCMHP2				=196,	// MHP��%�㏸(���̂Ƃ���S�X�y���p) 
	SC_INCMSP2				=197,	// MSP��%�㏸(���̂Ƃ���S�X�y���p) 
	SC_INCATK2				=198,	// ATK��%�㏸(���̂Ƃ���S�X�y���p) 
	SC_INCHIT2				=199,	// HIT��%�㏸(���̂Ƃ���S�X�y���p) 
	SC_INCFLEE2				=200,	// FLEE��%�㏸(���̂Ƃ���S�X�y���p) 
	SC_PRESERVE				=201,	// �v���U�[�u 
	SC_OVERTHRUSTMAX		=202,	// �I�[�o�[�g���X�g�}�b�N�X 
	SC_CHASEWALK_STR		=203,	//STR�㏸�i�`�F�C�X�E�H�[�N�p�j
	SC_WHISTLE_				=204,
	SC_ASSNCROS_			=205,
	SC_POEMBRAGI_			=206,
	SC_APPLEIDUN_			=207,
	SC_HUMMING_				=209,
	SC_DONTFORGETME_		=210,
	SC_FORTUNE_				=211,
	SC_SERVICE4U_			=212,
	SC_BATTLEORDER			=213,	//�M���h�X�L��
	SC_REGENERATION			=214,
	SC_BATTLEORDER_DELAY	=215,
	SC_REGENERATION_DELAY	=216,
	SC_RESTORE_DELAY		=217,
	SC_EMERGENCYCALL_DELAY	=218,
	SC_POISONPOTION			=219,
	SC_THE_MAGICIAN			=220,
	SC_STRENGTH				=221,
	SC_THE_DEVIL			=222,
	SC_THE_SUN				=223,
	SC_MEAL_INCSTR			=224,	//�H���p
	SC_MEAL_INCAGI			=225,
	SC_MEAL_INCVIT			=226,
	SC_MEAL_INCINT			=227,
	SC_MEAL_INCDEX			=228,
	SC_MEAL_INCLUK			=229,
	SC_RUN 					= 230,
	SC_SPURT 				= 231,
	SC_TKCOMBO 				= 232,	//�e�R���̃R���{�p
	SC_DODGE				= 233,
	SC_HERMODE				= 234,
	SC_TRIPLEATTACK_RATE_UP	= 235,	//�O�i�������A�b�v
	SC_COUNTER_RATE_UP		= 236,	//�J�E���^�[�L�b�N�������A�b�v
	SC_SUN_WARM				= 237,
	SC_MOON_WARM			= 238,
	SC_STAR_WARM			= 239,
	SC_SUN_COMFORT			= 240,
	SC_MOON_COMFORT			= 241,
	SC_STAR_COMFORT			= 242,
	SC_FUSION				= 243,
	SC_ALCHEMIST			= 244,	//��
	SC_MONK					= 245,
	SC_STAR					= 246,
	SC_SAGE					= 247,
	SC_CRUSADER				= 248,
	SC_SUPERNOVICE			= 249,
	SC_KNIGHT				= 250,
	SC_WIZARD				= 251,
	SC_PRIEST				= 252,
	SC_BARDDANCER			= 253,
	SC_ROGUE				= 254,
	SC_ASSASIN				= 255,
	SC_BLACKSMITH			= 256,
	SC_HUNTER				= 257,
	SC_SOULLINKER			= 258,
	SC_HIGH					= 259,
	//���̒ǉ����������炢���Ȃ��̂ŗ\��
	//�Ȃ���ΓK���ɂ��Ƃ��疄�߂�
	//= 260,	//�E�҂̍��̗\��H
	//= 261,	//�K���X�����K�[�̍��̗\��H
	//= 262,	//�H�H�H�̍��̗\��H
	SC_ADRENALINE2			= 263,
	SC_KAIZEL				= 264,
	SC_KAAHI				= 265,
	SC_KAUPE				= 266,
	SC_KAITE				= 267,
	SC_SMA					= 268,	//�G�X�}�r���\���ԗp
	SC_SWOO					= 269,
	SC_SKE					= 270,
	SC_SKA					= 271,
	SC_ONEHAND				= 272,
	SC_READYSTORM			= 273,
	SC_READYDOWN			= 274,
	SC_READYTURN			= 275,
	SC_READYCOUNTER			= 276,
	SC_DODGE_DELAY			= 277,
	SC_AUTOBERSERK			= 278,
	SC_DEVIL				= 279,
	SC_DOUBLECASTING 		= 280,	//�_�u���L���X�e�B���O
	SC_ELEMENTFIELD			= 281,	//������
	SC_DARKELEMENT			= 282,	//��
	SC_ATTENELEMENT			= 283,	//�O
	SC_MIRACLE				= 284,	//���z�ƌ��Ɛ��̊��
	SC_ANGEL				= 285,	//���z�ƌ��Ɛ��̓V�g
	SC_HIGHJUMP				= 286,	//�n�C�W�����v
	SC_DOUBLE				= 287,	//�_�u���X�g���C�t�B���O���
	SC_ACTION_DELAY			= 288,	//
	SC_BABY					= 289,	//�p�p�A�}�}�A��D��
	SC_LONGINGFREEDOM		= 290,
	SC_SHRINK				= 291,	//#�V�������N#
	SC_CLOSECONFINE			= 292,	//#�N���[�Y�R���t�@�C��#
	SC_SIGHTBLASTER			= 293,	//#�T�C�g�u���X�^�[#
	SC_ELEMENTWATER			= 294,	//#�G���������^���`�F���W��#
	//�H���p2
	SC_MEAL_INCHIT			= 295,
	SC_MEAL_INCFLEE			= 296,
	SC_MEAL_INCFLEE2		= 297,
	SC_MEAL_INCCRITICAL		= 298,
	SC_MEAL_INCDEF			= 299,
	SC_MEAL_INCMDEF			= 300,
	SC_MEAL_INCATK			= 301,
	SC_MEAL_INCMATK			= 302,
	SC_MEAL_INCEXP			= 303,
	SC_MEAL_INCJOB			= 304,
	//
	SC_ELEMENTGROUND		= 305,	//�y(�Z)
	SC_ELEMENTFIRE			= 306,	//��(�Z)
	SC_ELEMENTWIND			= 307,	//��(�Z)
	SC_WINKCHARM			= 308,
	SC_ELEMENTPOISON		= 309,	//��(�Z)
	SC_ELEMENTDARK			= 310,	//��(�Z)
	SC_ELEMENTELEKINESIS	= 311,	//�O(�Z)
	SC_ELEMENTUNDEAD		= 312,	//�s��(�Z)
	SC_UNDEADELEMENT		= 313,	//�s��(��)
	SC_ELEMENTHOLY			= 314,	//��(�Z)
	SC_NPC_DEFENDER			= 315,
	SC_RESISTWATER			= 316,	//�ϐ�
	SC_RESISTGROUND			= 317,	//�ϐ�
	SC_RESISTFIRE			= 318,	//�ϐ�
	SC_RESISTWIND			= 319,	//�ϐ�
	SC_RESISTPOISON			= 320,	//�ϐ�
	SC_RESISTHOLY			= 321,	//�ϐ�
	SC_RESISTDARK			= 322,	//�ϐ�
	SC_RESISTTELEKINESIS	= 323,	//�ϐ�
	SC_RESISTUNDEAD			= 324,	//�ϐ�
	SC_RESISTALL			= 325,	//�ϐ�
	//�푰�ύX�H
	SC_RACEUNKNOWN			= 326,	//���`
	SC_RACEUNDEAD			= 327,	//�s���푰
	SC_RACEBEAST			= 328,
	SC_RACEPLANT			= 329,
	SC_RACEINSECT			= 330,
	SC_RACEFISH				= 332,
	SC_RACEDEVIL			= 333,
	SC_RACEHUMAN			= 334,
	SC_RACEANGEL			= 335,
	SC_RACEDRAGON			= 336,
	SC_TIGEREYE				= 337,
	SC_GRAVITATION_USER		= 338,
	SC_GRAVITATION			= 339,
	SC_FOGWALL				= 340,
	SC_FOGWALLPENALTY		= 341,
	SC_REDEMPTIO			= 342,
	SC_TAROTCARD			= 343,
	SC_HOLDWEB				= 344,
	SC_INVISIBLE			= 345,
	SC_DETECTING			= 346,
	//
	SC_FLING				= 347,
	SC_MADNESSCANCEL		= 348,
	SC_ADJUSTMENT			= 349,
	SC_INCREASING			= 350,
	SC_DISARM				= 351,
	SC_GATLINGFEVER			= 352,
	SC_FULLBUSTER			= 353,
	//�j���W���X�L��
	SC_TATAMIGAESHI			= 354,
	SC_UTSUSEMI				= 355,	//#NJ_UTSUSEMI#
	SC_BUNSINJYUTSU			= 356,	//#NJ_BUNSINJYUTSU#
	SC_SUITON				= 357,	//#NJ_SUITON#
	SC_NEN					= 358,	//#NJ_NEN#
	SC_AVOID				= 359,	//#�ً}���#
	SC_CHANGE				= 360,	//#�����^���`�F���W#
	SC_DEFENCE				= 361,	//#�f�B�t�F���X#
	SC_BLOODLUST			= 362,	//#�u���b�h���X�g#
	SC_FLEET				= 363,	//#�t���[�g���[�u#
	SC_SPEED				= 364,	//#�I�[�o�[�h�X�s�[�h#
	
	//start�ł͎g���Ȃ�resist���A�C�e�����őS�ăN���A���邽�߂̕�
	SC_RESISTCLEAR			= 1001,
	SC_RACECLEAR			= 1002,
	SC_SOUL					= 1003,
	SC_SOULCLEAR			= 1004,
*/
	SC_SUITON				= 201,	//#NJ_SUITON#
	SC_AVOID				= 202,	//#�ً}���#
	SC_CHANGE				= 203,	//#�����^���`�F���W#
	SC_DEFENCE				= 204,	//#�f�B�t�F���X#
	SC_BLOODLUST			= 205,	//#�u���b�h���X�g#
	SC_FLEET				= 206,	//#�t���[�g���[�u#
	SC_SPEED				= 207,	//#�I�[�o�[�h�X�s�[�h#

	SC_MAX //Automatically updated max, used in for's and at startup to check we are within bounds. [Skotlex]
};



/*
external base classes
doubled linked list	-> 2 pointers -> 8[16] byte

mapbase			a virtual interface for maps
				derives:
				data:
				members:
				............
				just the base class for map and foreign_map (which are "maps on other host")
				............

map				the actual map
				derives:	mapbase
				data:		mapdata, blockroots ...
				members:	pathsearch, mapiterators (replacement for the foreach_..)
				............


mapobject		something that can be added to a map, a simple immobile object with id
				derives:	doubled linked list
				data:		ui id, us map, us posx, us posy, (ch bdir,ch hdir)	-> +10 (+12) byte
				members:	whatever is done with mapobjects
				............
				the directions might fit in here since they fill the data structure to a 4byte boundary
				(which is done by the compiler anyway) and (almost) any derived element is using it
				maybe also combine it with a status bitfield since dir's only use 3 bit
				............

movable			move interface, allows moving objects
				derives:	block
				data:		
				members:	
				............

battleobj		battle interface, hp/sp, skill
				derives:	movable
				data:		
				members:	
				............

script			script interface
				derives:	
				data:		
				members:	
				............

npc/warp/shop	script interface, move interface (script controlled) [warp,shop as special derivated, no unions]
				derives:	movable, script
				data:		
				members:	
				............

pet/mob/merc	script interface, move/battle interface (script/ai controlled)
				derives:	battleobj, (script)
				data:		
				members:	
				............

player/homun	client contolled  move/battle interface, socket processor
				derives:	battleobj
				data:		
				members:	
				............

*/


///////////////////////////////////////////////////////////////////////////////
// predeclarations
struct block_list;
struct skill_unit_group;
struct item_data;
struct pet_db;

struct movable;
struct affectable;
struct fightable;
struct map_session_data;
struct npc_data;
struct npcscript_data;
struct npcwarp_data;
struct npcshop_data;
struct mob_data;
struct pet_data;
class flooritem_data;
class chat_data;
class npcchat_data;
struct skill_unit;
class homun_data;





///////////////////////////////////////////////////////////////////////////////
/// processing base class.
/// process member is called for each block
///////////////////////////////////////////////////////////////////////////////
class CMapProcessor : public basics::noncopyable
{
public:
	ICL_EMPTY_COPYCONSTRUCTOR(CMapProcessor)
protected:
	CMapProcessor()				{}
public:
	virtual ~CMapProcessor()	{}
	virtual int process(block_list& bl) const = 0;
};

// predeclaration
struct map_intern;



///////////////////////////////////////////////////////////////////////////////
/// object on a map
struct block_list : public coordinate
{
//private:
	friend struct map_intern;
	/////////////////////////////////////////////////////////////////
	static block_list bl_head;
	static dbt* id_db;
public:
	/// returns blocklist from given id
	static block_list* from_blid(uint32 id);
	/// returns blocklist from name.
	/// only implemented  for individual block types
	static block_list* from_name(const char *name)	{ return NULL; }

	// functions that work on block_lists
	static int foreachpartymemberonmap(const CMapProcessor& elem, map_session_data &sd, bool area);
	static int foreachobject(const CMapProcessor& elem, object_t type);
	static skill_unit *skillunit_oncell(block_list &target, int x, int y, ushort skill_id, skill_unit *out_unit);


	/////////////////////////////////////////////////////////////////
	unsigned short m;	// redo coordinate to also hold maps
	uint32 id;
private:
	// data for internally used double-link list
	block_list *next;
	block_list *prev;
public:

	/////////////////////////////////////////////////////////////////
	/// default constructor.
	block_list(uint32 i=0) : 
		m(0),
		id(i),
		next(NULL),
		prev(NULL)
	{}
	/////////////////////////////////////////////////////////////////
	/// destructor.
	virtual ~block_list()	
	{	// not yet removed from map
		if(this->prev)
			this->delblock();
		// and just to be sure that it's removed from iddb
		this->unregister_id();
	}
	// missing:
	// all interactions with map 
	// which should be directly wrapped into a static interface
	// any comments? [Hinoko]

	// we'll leave it global right now and wrap it later
	// just move the basic functions here for the moment
	// and take care for the necessary refinements [Shinomori]

	/// add block to identifier database
	uint32 register_id(uint32 iid);
	/// add block to identifier database.
	/// generates the ID quicklist, returns 0 when failed
	uint32 register_id();
	/// remove block from identifier database
	uint32 unregister_id();

	/// add block to map
	bool addblock();
	/// remove block from map
	bool delblock();

	/// check if a block in on a map
	bool is_on_map() const
	{
		return this->prev!=NULL;
	}

	// collision free deleting of blocks
	int freeblock();
	static int freeblock_lock(void);
	static int freeblock_unlock(void);


	// this has to be overloaded and could also used with masks instead pure enums
	virtual bool is_type(object_t t) const
	{
		return (t==BL_NUL); // non-type
	}
	bool operator==(object_t t) const
	{
		return this->is_type(t);
	}
	bool operator!=(object_t t) const
	{
		return !this->is_type(t);
	}

	///////////////////////////////////////////////////////////////////////////
	/// upcasting overloads.
	virtual map_session_data*		get_sd()				{ return NULL; }
	virtual const map_session_data* get_sd() const			{ return NULL; }

	virtual pet_data*				get_pd()				{ return NULL; }
	virtual const pet_data*			get_pd() const			{ return NULL; }

	virtual mob_data*				get_md()				{ return NULL; }
	virtual const mob_data*			get_md() const			{ return NULL; }

	virtual npc_data*				get_nd()				{ return NULL; }
	virtual const npc_data*			get_nd() const			{ return NULL; }

	virtual npcshop_data*			get_shop()				{ return NULL; }
	virtual const npcshop_data*		get_shop() const		{ return NULL; }

	virtual npcscript_data*			get_script()			{ return NULL; }
	virtual const npcscript_data*	get_script() const		{ return NULL; }

	virtual npcwarp_data*			get_warp()				{ return NULL; }
	virtual const npcwarp_data*		get_warp() const		{ return NULL; }

	virtual homun_data*				get_hd()				{ return NULL; }
	virtual const homun_data*		get_hd() const			{ return NULL; }

	virtual chat_data*				get_cd()				{ return NULL; }
	virtual const chat_data*		get_cd() const			{ return NULL; }

	virtual skill_unit*				get_sk()				{ return NULL; }
	virtual const skill_unit*		get_sk() const			{ return NULL; }

	virtual flooritem_data*			get_fd()				{ return NULL; }
	virtual const flooritem_data*	get_fd() const			{ return NULL; }

	virtual movable*				get_movable()			{ return NULL; }
	virtual const movable*			get_movable() const		{ return NULL; }

	virtual affectable*				get_affectable()		{ return NULL; }
	virtual const affectable*		get_affectable() const	{ return NULL; }

	virtual fightable*				get_fightable()			{ return NULL; }
	virtual const fightable*		get_fightable() const	{ return NULL; }



	///////////////////////////////////////////////////////////////////////////
	/// return head direction.
	/// default function, needs to be overloaded at specific implementations
	virtual dir_t get_headdir() const		{ return DIR_S; }
	/// return body direction.
	/// default function, needs to be overloaded at specific implementations
	virtual dir_t get_bodydir() const		{ return DIR_S; }
	/// alias to body direction.
	virtual dir_t get_dir() const			{ return DIR_S; }

	///////////////////////////////////////////////////////////////////////////
	/// set directions seperately.
	virtual void set_dir(dir_t b, dir_t h)	{}
	/// set both directions equally.
	virtual void set_dir(dir_t d)			{}
	/// set directions to look at target
	virtual void set_dir(const coordinate& to)	{}
	/// set body direction only.
	virtual void set_bodydir(dir_t d)		{}
	/// set head direction only.
	virtual void set_headdir(dir_t d)		{}

	/// check if withing AREA reange.
	virtual bool is_near(const block_list& bl) const;

	///////////////////////////////////////////////////////////////////////////
	// status functions
	// overloaded at derived classes

	/// checks for walking state
	virtual bool is_walking() const			{ return false; }
	/// checks for attack state
	virtual bool is_attacking() const		{ return false; }
	/// checks for skill state
	virtual bool is_casting() const		{ return false; }
	/// checks for dead state
	virtual bool is_dead() const			{ return false; }
	/// checks for sitting state
	virtual bool is_sitting() const			{ return false; }
	/// checks for idle state (alive+not sitting+not blocked by skill)
	virtual bool is_idle() const			{ return true; }

	/// sets the object to idle state
	virtual bool set_idle()					{ return false; }
	/// sets the object delay
	virtual void set_delay(ulong delaytick)	{ this->set_idle(); }
	/// sets the object to idle state
	virtual bool set_dead()					{ return false; }
	/// sets the object to sitting state
	virtual bool set_sit()					{ return false; }
	/// sets the object to standing state
	virtual bool set_stand()				{ return false; }



	/// checks for flying state
	virtual bool is_flying() const			{ return false; }
	/// checks for confusion state
	virtual bool is_confuse() const			{ return false; }
	/// checks if this is attackable
	virtual bool is_attackable() const		{ return false; }


	virtual bool is_hiding() const			{ return false; }
	virtual bool is_cloaking() const		{ return false; }
	virtual bool is_chasewalk() const		{ return false; }
	virtual bool is_carton() const			{ return false; }
	virtual bool is_falcon() const			{ return false; }
	virtual bool is_riding() const			{ return false; }
	/// checks for invisible state
	virtual bool is_invisible() const		{ return false; }
	virtual bool is_50overweight() const	{ return false; }
	virtual bool is_90overweight() const	{ return false; }

	virtual int get_class() const			{ return 0; }
	virtual int get_lv() const				{ return 0; }
	virtual int get_range() const			{ return 0; }
	virtual int get_hp() const				{ return 1; }

	virtual int get_race() const			{ return 0; }
	virtual int get_race2() const			{ return 0; }
	virtual int get_mode() const			{ return 1; }// �Ƃ肠���������Ƃ������Ƃ�1
	virtual int get_mexp() const			{ return 0; }
	virtual int get_size() const			{ return 1; }

	virtual uint32 get_party_id() const		{ return 0; }
	virtual uint32 get_guild_id() const		{ return 0; }


	////////
	//## TODO
	virtual int get_max_hp() const;
	virtual int get_str() const;
	virtual int get_agi() const;
	virtual int get_vit() const;
	virtual int get_int() const;
	virtual int get_dex() const;
	virtual int get_luk() const;
	////////


	bool is_boss() const					{ return 0!=this->get_mexp(); }
	bool is_undead() const;


	///////////////////////////////////////////////////////////////////////////
	// walking functions
	// overloaded at derived classes

	/// walk to position
	virtual bool walktoxy(const coordinate& pos,bool easy=false)						{ return false; }
	/// walk to a coordinate
	virtual bool walktoxy(unsigned short x,unsigned short y,bool easy=false)			{ return false; }
	/// stop walking
	virtual bool stop_walking(int type=1)												{ return false; }
	/// instant position change
	virtual bool movepos(const coordinate &target)										{ return false; }
	/// instant position change
	virtual bool movepos(unsigned short x,unsigned short y)								{ return false; }
	/// warps to a given map/position. 
	virtual bool warp(unsigned short m, unsigned short x, unsigned short y, int type)	{ return false; }


	///////////////////////////////////////////////////////////////////////////
	// attack functions

	/// starts attack
	virtual bool start_attack(uint32 target_id, bool cont)		{ return false; }
	/// starts attack
	virtual bool start_attack(const block_list& target_bl, bool cont)	{ return false; }
	/// stops attack
	virtual bool stop_attack()							{ return false; }

	virtual int heal(int hp, int sp=0)					{ return 0; }

	///////////////////////////////////////////////////////////////////////////
	// targeting functions

	/// unlock from current target
	virtual void unlock_target()						{ unlock_target(0); }
	virtual void unlock_target(unsigned long tick)		{ }


	///////////////////////////////////////////////////////////////////////////
	// skill functions
	/// stops skill
	virtual bool stop_skill()							{ return false; }
	// skill failed message.
	virtual int  skill_check(ushort id)					{ return 0; }
	/// check if current skill can be canceled
	virtual bool skill_can_cancel() const				{ return false; }
	/// called when a skill has failed.
	/// does nothing on default
	virtual void skill_stopped(ushort skillid)			{}
	/// does nothing on default
	virtual void skill_failed(ushort skill_id, skillfail_t type=SF_FAILED)	{}


	///////////////////////////////////////////////////////////////////////////
	// status functions, new layout
	/// check if status exists
	virtual bool has_status(status_t status_id) const		{ return false; }
	virtual basics::numptr& get_statusvalue1(status_t status_id)	{ static basics::numptr dummy; return dummy; }
	virtual basics::numptr& get_statusvalue2(status_t status_id)	{ static basics::numptr dummy; return dummy; }
	virtual basics::numptr& get_statusvalue3(status_t status_id)	{ static basics::numptr dummy; return dummy; }
	virtual basics::numptr& get_statusvalue4(status_t status_id)	{ static basics::numptr dummy; return dummy; }
	virtual basics::numptr get_statusvalue1(status_t status_id)	const	{ return basics::numptr(); }
	virtual basics::numptr get_statusvalue2(status_t status_id) const	{ return basics::numptr(); }
	virtual basics::numptr get_statusvalue3(status_t status_id) const	{ return basics::numptr(); }
	virtual basics::numptr get_statusvalue4(status_t status_id) const	{ return basics::numptr(); }



	/// remove a status
	virtual bool remove_status(status_t status_id)		{ return false; }
	/// remove all status changes
	virtual bool remove_status()						{ return false; }
	/// create a new status. or replace an existing
	virtual bool create_status(status_t status_id, const basics::numptr& v1=basics::numptr(), const basics::numptr& v2=basics::numptr(), const basics::numptr& v3=basics::numptr(), const basics::numptr& v4=basics::numptr())
	{
		return false;
	}
};




int map_freeblock_timer (int tid, unsigned long tick, int id, basics::numptr data);













///////////////////////////////////////////////////////////////////////////////
/// Moonlit creates a 'safe zone' [celest]
class CSkillMoonlitCount : public CMapProcessor
{
	ICL_EMPTY_COPYCONSTRUCTOR(CSkillMoonlitCount)
	uint32 id;
public:
	CSkillMoonlitCount(uint32 i) : id(i)	{}
	~CSkillMoonlitCount()	{}
	virtual int process(block_list& bl) const
	{
		if( bl.block_list::id != id && bl.has_status(SC_MOONLIT) )
			return 1;
		return 0;
	}
};


#endif//_MAPOBJ_H_

