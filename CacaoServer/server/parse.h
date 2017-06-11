#ifndef __CACAOTALK_SERVER_PARSE_HEADER__
#define __CACAOTALK_SERVER_PARSE_HEADER__
#include <stdio.h>
#include <string.h>

#define LOGIN_THREAD 0
#define CHANEL_THREAD 1

#define CACAO_LOGIN 0
#define CACAO_ADD_USER 1
#define CACAO_DEL_USER 2
#define CACAO_CHANEL_INFO 3
#define CACAO_CHANEL_USER 4
#define CACAO_CHANEL_CHAT 0
#define CACAO_CHANEL_JOIN 5
/*
 * 받은 데이터 구분. (parse.c)
 * thread_flag - LOGIN_THREAD, CHANEL_THREAD 둘중 한가지, message - 파싱할 버퍼
 * 반환값
 * CACAO_LOGIN - 로그인 요청
 * CACAO_ADD_USER - 계정 추가
 * CACAO_DEL_USER - 계정 삭제
 * CACAO_CHANEL_INFO - 채널 리스트 요청
 * CACAO_CHANEL_USER - 채널에 있는 유저 리스트.
 * CACAO_CHANEL_CHAT - 채팅 메세지
 * 실패시 -1 반환.
 */
int parse(int thread_flag, char * message, char * user_id, char * user_pass, int * chanelNum);

#endif
