#include "./parse.h"
#include <stdlib.h>

//CHANEL_THREAD 플래그 헤더
static int check_chanel_thread(char * message)
{
	if(strcmp(message, "CHANEL_USER") == 0)
		return CACAO_CHANEL_USER;
	else if(strcmp(message, "CHANEL_JOIN") == 0)
		return CACAO_CHANEL_JOIN;
	else
		return CACAO_CHANEL_CHAT;
	return -1;
}

//LOGIN_THREAD 플래그 헤더
static int check_login_thread(char * message)
{
	if(strcmp(message, "CHANEL_INFO") == 0)
		return CACAO_CHANEL_INFO;
	else if(strcmp(message, "USER_LOGIN") == 0)
		return CACAO_LOGIN;
	else if(strcmp(message, "USER_ADD") == 0)
		return CACAO_ADD_USER;
	else if(strcmp(message, "USER_DEL") == 0)
		return CACAO_DEL_USER;
	
	return -1;
}

/*
 * 로그인 또는 계정 제거 데이터에서
 * 계정과 암호를 추출한다.
 */
static char * get_login_data(char * message, char * id, char * pass, int * chanelNum)
{
	char * next_ptr;
	char * chanel;
	char * token;

	token = strtok_r(message, "\n", &next_ptr);
	if(token == NULL)
		return NULL;
	strcpy(id, token);

	token = strtok_r(NULL, "\n", &next_ptr);
	if(token == NULL)
		return NULL;
	strcpy(pass, token);

	chanel = strtok_r(NULL, "\n", &next_ptr);
	if(chanel != NULL)
		*chanelNum = atoi(chanel);

	return message;
}

int parse(int thread_flag, char * message, char * user_id, char * user_pass, int * chanelNum)
{
	char * token;
	char * next_ptr;
	int message_type;
	
	token = strtok_r(message,"\n", &next_ptr);
	if(token == NULL)
		return -1;

	if(thread_flag == CHANEL_THREAD) //CHANEL_THREAD
		message_type = check_chanel_thread(message);
	else	// LOGIN_THREAD
	{
		message_type = check_login_thread(message);
		if(message_type == CACAO_CHANEL_INFO)
			return message_type;
		if(get_login_data(next_ptr, user_id, user_pass, chanelNum) == NULL)
			return -1;
	}
	
	return message_type;
}
