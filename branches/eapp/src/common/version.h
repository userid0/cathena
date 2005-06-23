// $Id: version.h,v 1.2 2004/09/22 09:49:06 PoW Exp $
#ifndef _VERSION_H_
#define _VERSION_H_

#define ATHENA_MAJOR_VERSION	1	// Major Version
#define ATHENA_MINOR_VERSION	0	// Minor Version
#define ATHENA_REVISION			0	// Revision

#define ATHENA_RELEASE_FLAG		1	// 1=Develop,0=Stable
#define ATHENA_OFFICIAL_FLAG	1	// 1=Mod,0=Official


// ATHENA_MOD_VERSION�̓p�b�`�ԍ��ł��B
// ����͖����ɕς��Ȃ��Ă��C����������ς�����x�̈����ŁB
// �i����A�b�v���[�h�̓x�ɕύX����̂��ʓ|�Ǝv���邵�A��������
// �@���̍��ڂ��Q�Ƃ���l�����邩�ǂ����ŋ^�₾����B�j
// ���̒��x�̈����Ȃ̂ŁA�T�[�o�[�ɖ₢���킹�鑤���A�����܂Ŗڈ����x�̈�����
// ����܂�M�p���Ȃ����ƁB
// �Isnapshot�̎���A�傫�ȕύX���������ꍇ�͐ݒ肵�Ăق����ł��B
// C����̎d�l��A�ŏ���0��t�����8�i���ɂȂ�̂ŊԈႦ�Ȃ��ŉ������B
#define ATHENA_MOD_VERSION	1249	// mod version (patch No.)

#define ATHENA_SVN_VERSION	2001	// svn version

typedef enum 
{
	ATHENA_SERVER_NONE	=	0x00,	// not defined
	ATHENA_SERVER_LOGIN	=	0x01,	// login server
	ATHENA_SERVER_CHAR	=	0x02,	// char server
	ATHENA_SERVER_INTER	=	0x04,	// inter server
	ATHENA_SERVER_MAP	=	0x08,	// map server
	ATHENA_SERVER_CORE	=	0x10,	// core component
	ATHENA_SERVER_xxx1	=	0x20,
	ATHENA_SERVER_xxx2	=	0x40,
	ATHENA_SERVER_xxx3	=	0x80,
	ATHENA_SERVER_ALL	=	0xFF
} ServerType;



#endif
