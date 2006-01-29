#ifndef _HTTPD_H_
#define _HTTPD_H_

struct httpd_session_data;

// NOTE by Celest: This file is not used by httpd.c, but included only as an API reference.

// ����
//   1.athena������httpd �ő傫�ȃt�@�C���𑗐M���邱�Ƃ͂����߂��܂���B
//     200KB �𒴂���悤�ȃt�@�C���́A�ʂ̃\�t�g�𗘗p���邱�Ƃ����߂܂��B
//   2.�t�@�C�����Ɏg���镶���́A[A-Za-z0-9-_\.] �ł��B���̕������g���ƁA
//     BAD REQUEST �Œe����܂��B



void httpd_pages(const char* url,void(*httpd_func)(struct httpd_session_data* sd,const char* url));

// �w�肳�ꂽURL �ɑ΂���R�[���o�b�N�֐���ݒ肷��B���̊֐��́A�ȉ��̂悤��
// ��������K�v������B
//
// 1. URL �́A�擪�̃X���b�V�����Ȃ��ꂽ�t�@�C�����ł��B�Ⴆ�΁A"GET / HTTP/1.0"
//    �Ƃ������Ƀ��N�G�X�g���ꂽ���AURL �ɂ�""(�󕶎�)������A"GET /hoge HTTP/1.0"
//    �̎��ɂ́A"hoge"������܂��B
// 2. ���N�G�X�g���ꂽ�y�[�W������������Ahttpd_send() �܂��́Ahttpd_send_head()
//    ��httpd_send_data() �̑g���Ăяo���A�f�[�^���o�͂���B
// 3. httpd_send_file ���w�肷��ƁAhttpd/ �ȉ��ɂ���t�@�C�����o�͂���B�t�@�C����
//    �󕶎����w�肳�ꂽ���́Aindex.html���w�肳�ꂽ���̂Ƃ݂Ȃ����B



char* httpd_get_value(struct httpd_session_data* sd,const char* val);

// ���N�G�X�g���ꂽ�A�h���X�ɓn���ꂽ�t�H�[���f�[�^�̂����A�Y�����镶�����Ԃ��B
// �Ⴆ�΁A"GET /status/graph?image=users HTTP/1.0"�Ƃ������N�G�X�g�̏ꍇ�A
// httpd_get_value(sd,"image"); �́A "users"��Ԃ��B���̊֐��̖߂�l�́A�Ăяo������
// ������Ȃ���΂Ȃ�Ȃ��B�܂��A�Y�����镶���񂪖������́A��̕������Ԃ��B

unsigned int httpd_get_ip(struct httpd_session_data *sd);

// �N���C�A���g��IP��Ԃ��B


void httpd_default_page(void(*httpd_func)(struct httpd_session_data* sd,const char* url));

// �w�肳�ꂽURL ���o�^����Ă��Ȃ����ɌĂяo���֐���ݒ肷��B���̊֐����Ăяo���Ȃ����A
// �֐��̈�����NULL���w�肷��ƁA404 Not Found ��Ԃ��B




void httpd_send(struct httpd_session_data* sd,int status,const char *content_type,int content_len,const void *data);

//	HTTP�w�b�_�A�f�[�^��g�ɂ��đ��M����B���̊֐����Ăяo������ɁAhttpd_send_data ��
//  �Ăяo���Ă͂Ȃ�Ȃ��B
// 
//		sd           : httpd_set_parse_func() �ɓn���ꂽ���̂����̂܂ܓn�����ƁB
//		status       : HTTP�w�b�_�ɉ�����status�B�ʏ��200�B
//		content_type : ���M����f�[�^�̃^�C�v�Btext/html , image/jpeg �ȂǁB
//		content_len  : ���M����f�[�^�̒����B
//		data         : ���M����f�[�^�ւ̃|�C���^



void httpd_send_head(struct httpd_session_data* sd,int status,const char *content_type,int content_len);

//	HTTP�w�b�_�𑗐M����B
//
//		sd           : ����
//		status       : ����
//		content_type : ����
//      content_len  : content_len��-1�Ɏw�肷�邱�ƂŁA���̊֐����Ă΂ꂽ���_��
//                     ������������Ȃ��f�[�^�𑗐M���邱�Ƃ��ł���B���̏ꍇ��
//                     �����I��HTTP/1.0 �ڑ��ƂȂ�A�I�[�o�[�w�b�h���傫���Ȃ�̂ŁA
//                     ���܂肨���߂͂��Ȃ��B




void httpd_send_data(struct httpd_session_data* sd,int content_len,const void *data);

// �f�[�^�𑗐M����B���̊֐����Ahttpd_send_head() ���Ăяo���O�ɌĂяo���ꂽ�ꍇ�A
// content_type = application/octet-stream, content_len = -1 �Ƃ��ăw�b�_�����M�����B
//		sd           : ����
//      content_len  : ���M����f�[�^��data�������w�肷��B
//      data         : ���M����f�[�^



void httpd_send_file(struct httpd_session_data* sd,const char* url);

// �t�@�C���𑗐M����B���̊֐��́Ahttpd_send_head() ���Ăяo���O�ɌĂяo���Ȃ����
// �Ȃ�Ȃ��B�t�@�C���ɋ󕶎����w�肳�ꂽ�Ƃ��́Aindex.html���w�肳�ꂽ�ƌ��Ȃ����B



void httpd_send_error(struct httpd_session_data* sd,int status);

// HTTP�G���[���b�Z�[�W�𑗐M����Bstatus ��HTTP�̃G���[�R�[�h�Ɠ����B
// 	400 Bad Request, 404 Not Found, 500 Internal Server Error �ȂǁB

int  httpd_parse(int fd);

// ����������
void do_init_httpd(void);
void do_final_httpd(void);

#endif
