#include "./cacao_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>
void login_queue_init(login_queue * login_q)
{
	pthread_cond_init(&login_q->login_cond, NULL);
	pthread_mutex_init(&login_q->login_mutex, NULL);

	login_q->server_stat = STOP;
	login_q->start = NULL;
	login_q->queue_count = 0;
	login_q->end = NULL;
}

//logine_queue 에 사용되는 Logine_info 구조체 생성 함수.
static struct Login_info * login_queue_make(int user_sock, char * user_id, char * user_pass, int chanelNum)
{
	struct Login_info * info = NULL;

	//메모리 할당
	info = (struct Login_info*)malloc(sizeof(struct Login_info));
	if(info == NULL)
		return LOGIN_QUEUE_MAKE_FAILED; //Malloc failed
	
	//초기화.
	info->user_sock = user_sock;			//소켓
	sprintf(info->user_id, "%s", user_id);		//id
	sprintf(info->user_pass, "%s", user_pass);	//password
	info->chanelNum = chanelNum;

	info->next = NULL;
	return info;
}

int login_queue_add(login_queue * login_q, int user_sock, char * user_id, char * user_pass, int chanelNum)
{
	struct Login_info * temp = login_queue_make(user_sock, user_id, user_pass, chanelNum);
	if(temp == LOGIN_QUEUE_MAKE_FAILED)
		return LOGIN_QUEUE_MAKE_FAILED_INT;
	
	//end 가 NULL 이면 큐가 비어있는 상태.
	if(login_q->end == NULL)
		login_q->start = login_q->end = temp;
	else //큐가 비어있지 않은 상태.
	{
		login_q->end->next = temp;
		login_q->end = login_q->end->next;
	}

	//Login queue 의 개수 증가
	login_q->queue_count++;
	//조건변수 SIGNAL
	pthread_cond_signal(&login_q->login_cond);

	return SUCCESS;
}

//첫 큐 제거함수.
static void login_queue_del(login_queue * login_q)
{
	login_info * temp;
	temp = login_q->start;
	//다음 큐가 NULL 이면 마지막 큐.
	if(temp->next == NULL)
		login_q->start = login_q->end = NULL;
	else
		login_q->start = temp->next;

	login_q->queue_count--;
	free(temp);
}

login_info login_queue_get(login_queue * login_q)
{
	login_info info;

	info.next = NULL;

	info.user_sock = login_q->start->user_sock;
	sprintf(info.user_id, "%s", login_q->start->user_id);
	sprintf(info.user_pass, "%s", login_q->start->user_pass);
	info.chanelNum = login_q->start->chanelNum;
	
	//첫 큐를 제거한다.
	login_queue_del(login_q);

	return info;
}

//Login thread
void * login_thread(void * arg)
{
	struct login_thread_arg * t_arg = (struct login_thread_arg*)arg;
	login_queue * login_q = t_arg->login_arg;
	chanel_manager * chanel_man = t_arg->chanel_arg;
	chanel_manager * chanel_temp = NULL;
	
	login_info user_info; //User의 정보 저장.
	struct epoll_event ep;	///Main epoll 추가를 위해.
	
	//스레드 종료요청에 대한 처리.
	//요청이 오면 바로 종료하도록 지정.
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	if(ca_db_connect(&login_q->db, "root", "950214"))
	{
		printf("==Login thread==\n");
		printf("ca_db_connect() Error\n");
		return (void*)0;
	}
	while(1)
	{
		pthread_mutex_lock(&(login_q->login_mutex));
		//큐의 남은 개수가 0 이면.
		if(login_q->queue_count == 0)
			pthread_cond_wait(&login_q->login_cond, &login_q->login_mutex);
		
		user_info = login_queue_get(login_q); //데이터를 가져온다.

		if(!user_check(&login_q->db, user_info.user_id, user_info.user_pass))
		{//User check 성공시
			chanel_temp = chanel_manager_find(chanel_man, user_info.chanelNum);
			
			if(chanel_temp == CHANEL_NOT_FOUND)
				//채널을 못찾았을 경우 Error 처리. 
				send(user_info.user_sock, "Not Found Chanel", 16, 0);
			else //채널을 찾았으면
			{
				//해당 채널에 유저가 있는지 검사한다.
				if(check_user(chanel_temp->list_start, user_info.user_id))
				{
					send(user_info.user_sock, "USER_NOW_LOGIN", 14, 0);
					goto RETURN_MAIN;
				}
				//채널에 유저를 추가한다.
				if(chanel_add_user(chanel_man, user_info.chanelNum, user_info))
				{
					//유저 추가 실패시 수행.
					printf("USER ADD Error\n");
					send(user_info.user_sock, "Server Error", 12, 0);
					close(user_info.user_sock);
				}
				send(user_info.user_sock, "JOIN", 4,0);
			}
		}
		else //user 없음
		{
			send(user_info.user_sock, "LOGIN ERROR", 11, 0);
RETURN_MAIN: //이미 접속중인 계정은 이곳으로 점프하여 처리
			ep.data.fd = user_info.user_sock;
			ep.events = EPOLLIN;
			epoll_ctl(login_q->main_epoll, EPOLL_CTL_ADD, user_info.user_sock, &ep);
		}

		pthread_mutex_unlock(&(login_q->login_mutex));
	}

	ca_db_close(&login_q->db);
	return (void *)0;
}

int login_thread_start(login_queue * login_q, chanel_manager * chanel_man)
{
	struct login_thread_arg * t_arg; //스레드에 넘겨줄 구조체.
	t_arg = malloc(sizeof(struct login_thread_arg));

	t_arg->login_arg = login_q;
	t_arg->chanel_arg = chanel_man;

	login_q->server_stat = RUN;
	
	if(pthread_create(&login_q->login_thread, 0, login_thread, (void*)t_arg))
		return LOGIN_THREAD_CREATE_ERROR;
	return SUCCESS;
}

