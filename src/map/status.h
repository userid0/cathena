#ifndef _STATUS_H_
#define _STATUS_H_

#include "map.h" // for basic type definitions, move to a seperated file


enum {	// struct map_session_data �� status_change�̔�?�e?�u��
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
	SC_STAN				= 76,
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
	SC_STAN					=130,	// ��Ԉُ�F�X�^�� 
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
extern int SkillStatusChangeTable[];

extern int current_equip_item_index;


int status_recalc_speed(block_list *bl);

// �p�����[�^�����n battle.c ���ړ�
int status_get_class(block_list *bl);
int status_get_lv(const block_list *bl);
int status_get_range(const block_list *bl);
int status_get_hp(block_list *bl);
int status_get_max_hp(block_list *bl);
int status_get_str(block_list *bl);
int status_get_agi(block_list *bl);
int status_get_vit(block_list *bl);
int status_get_int(block_list *bl);
int status_get_dex(block_list *bl);
int status_get_luk(block_list *bl);
int status_get_hit(block_list *bl);
int status_get_flee(block_list *bl);
int status_get_def(block_list *bl);
int status_get_mdef(block_list *bl);
int status_get_flee2(block_list *bl);
int status_get_def2(block_list *bl);
int status_get_mdef2(block_list *bl);
int status_get_baseatk(block_list *bl);
int status_get_atk(block_list *bl);
int status_get_atk2(block_list *bl);
int status_get_adelay(block_list *bl);
int status_get_amotion(const block_list *bl);
int status_get_dmotion(block_list *bl);
int status_get_element(block_list *bl);
int status_get_attack_element(block_list *bl);
int status_get_attack_element2(block_list *bl);  //���蕐�푮���擾
#define status_get_elem_type(bl)	(status_get_element(bl)%10)
#define status_get_elem_level(bl)	(status_get_element(bl)/10/2)
uint32 status_get_party_id(const block_list *bl);
uint32 status_get_guild_id(const block_list *bl);
int status_get_race(block_list *bl);
int status_get_size(block_list *bl);
int status_get_mode(block_list *bl);
int status_get_mexp(block_list *bl);
int status_get_race2(block_list *bl);

struct status_change *status_get_sc_data(block_list *bl);
short *status_get_opt1(block_list *bl);
short *status_get_opt2(block_list *bl);
short *status_get_opt3(block_list *bl);
short *status_get_option(block_list *bl);

int status_get_matk1(block_list *bl);
int status_get_matk2(block_list *bl);
int status_get_critical(block_list *bl);
int status_get_atk_(block_list *bl);
int status_get_atk_2(block_list *bl);
int status_get_atk2(block_list *bl);

int status_isimmune(block_list *bl);

int status_get_sc_def(block_list *bl, int type);
#define status_get_sc_def_mdef(bl)	(status_get_sc_def(bl, SP_MDEF1))
#define status_get_sc_def_vit(bl)	(status_get_sc_def(bl, SP_DEF2))
#define status_get_sc_def_int(bl)	(status_get_sc_def(bl, SP_MDEF2))
#define status_get_sc_def_luk(bl)	(status_get_sc_def(bl, SP_LUK))

// ��Ԉُ�֘A skill.c ���ړ�
int status_change_start(block_list *bl,int type,basics::numptr val1,basics::numptr val2,basics::numptr val3,basics::numptr val4,unsigned long tick,int flag);
int status_change_end(block_list* bl, int type,int tid );
int status_change_timer(int tid, unsigned long tick, int id, basics::numptr data);

class CStatusChangetimer : public CMapProcessor
{
	block_list &src;
	int type;
	unsigned long tick;
public:
	CStatusChangetimer(block_list &s, int ty, unsigned long t)
		: src(s), type(ty), tick(t)
	{}
	~CStatusChangetimer()	{}
	virtual int process(block_list& bl) const;
};

int status_change_clear(block_list *bl,int type);
int status_change_clear_buffs(block_list *bl);
int status_change_clear_debuffs(block_list *bl);

int status_calc_pet(struct map_session_data &sd, bool first); // [Skotlex]
// �X�e�[�^�X�v�Z pc.c ���番��
// pc_calcstatus

int status_calc_pc(struct map_session_data &sd,int first);
int status_calc_speed(struct map_session_data &sd, unsigned short skill_num, unsigned short skill_lv, int start);
int status_calc_speed_old(struct map_session_data &sd); // [Celest]
// int status_calc_skilltree(struct map_session_data *sd);
int status_getrefinebonus(int lv,int type);
int status_percentrefinery(struct map_session_data &sd,struct item &item);
//Use this to refer the max refinery level [Skotlex]

extern int percentrefinery[MAX_REFINE_BONUS][MAX_REFINE+1]; //The last slot always has a 0% success chance [Skotlex]

int status_readdb(void);
int do_init_status(void);

#endif
