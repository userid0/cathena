// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _NULLPO_H_
#define _NULLPO_H_

#include "basetypes.h"

// �S�̂̃X�C�b�`��錾���Ă���w�b�_�������
// �����Ɉړ����Ă����������
#define NULLPO_CHECK 1


#define NLP_MARK __FILE__, __LINE__, __func__

/*----------------------------------------------------------------------------
 * Macros
 *----------------------------------------------------------------------------
 */
/*======================================
 * Null�`�F�b�N �y�� ���o�͌� return
 *�E�W�J�����if�Ƃ�return�����o��̂�
 *  ��s�P�̂Ŏg���Ă��������B
 *�Enullpo_ret(x = func());
 *  �̂悤�Ȏg�p�@���z�肵�Ă��܂��B
 *--------------------------------------
 * nullpo_retv(t)
 *   �߂�l �Ȃ�
 * [����]
 *  t       �`�F�b�N�Ώ�
 *--------------------------------------
 * nullpo_retr(ret, t)
 *   �߂�l �w��
 * [����]
 *  ret     return(ret);
 *  t       �`�F�b�N�Ώ�
 *--------------------------------------
 * nullpo_break(t)
 *   �߂�l �Ȃ�
 * [����]
  *  t       �`�F�b�N�Ώ�
 *--------------------------------------
 */

#if NULLPO_CHECK

#define nullpo_retv(t)		if (nullpo_chk(NLP_MARK, (void *)(t))) {return;}

#define nullpo_retr(ret, t)	if (nullpo_chk(NLP_MARK, (void *)(t))) {return(ret);}

#define nullpo_break(t)		if (nullpo_chk(NLP_MARK, (void *)(t))) {break;}


#else
// No Nullpo check 

// if((t)){;}
// �ǂ����@���v�����Ȃ������̂ŁE�E�E����̍�ł��B
// �ꉞ���[�j���O�͏o�Ȃ��͂�


#define nullpo_retv(t)		if((t)){;}
#define nullpo_retr(ret, t)	if((t)){;}
#define nullpo_break(t)		if((t)){;}



#endif // NULLPO_CHECK

/*----------------------------------------------------------------------------
 * Functions
 *----------------------------------------------------------------------------
 */
/*======================================
 * nullpo_chk
 *   Null�`�F�b�N �y�� ���o��
 * [����]
 *  file    __FILE__
 *  line    __LINE__
 *  func    __func__ (�֐���)
 *    �����ɂ� NLP_MARK ���g���Ƃ悢
 *  target  �`�F�b�N�Ώ�
 * [�Ԃ�l]
 *  0 OK
 *  1 NULL
 *--------------------------------------
 */
int nullpo_chk(const char *file, int line, const char *func, const void *target);



/*======================================
 * nullpo_info
 *   nullpo���o��
 * [����]
 *  file    __FILE__
 *  line    __LINE__
 *  func    __func__ (�֐���)
 *    �����ɂ� NLP_MARK ���g���Ƃ悢
 *--------------------------------------
 */
void nullpo_info(const char *file, int line, const char *func);





#endif
