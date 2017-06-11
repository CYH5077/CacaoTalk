#include "./cacao_server.h"
#include "./parse.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>

void chanel_manager_init(chanel_manager * chanel_man, int max_user_count)
{
	chanel_man->chanel_num = 0;
	chanel_man->user_count = 0;
	chanel_man->max_user_count = max_user_count; //우선 값 지정.

	//큐 포인터 초기화.
	chanel_man->list_count = 0;
	chanel_man->list_start = NULL;
	chanel_man->list_end = NULL;

	chanel_man->next = NULL;
}

//새로운 채널을 생성하고 반환.
static chanel_manager * chanel_manager_make(int chanel_num, int max_user_count)
{
	chanel_manager * chanel = (chanel_manager *)malloc(sizeof(chanel_manager));
	if(chanel == NULL)
		return NULL;
	chanel_manager_init(chanel, max_user_count);
	chanel->chanel_num = chanel_num;

	return chanel;
}

int chanel_manager_add(chanel_manager ** chanel_man, int max_user_count)
{
	static int chanel_num = 1;

	if(*chanel_man == NULL)
	{
		*chanel_man = chanel_manager_make(chanel_num, max_user_count);
		if((*chanel_man) == NULL)
			return CHANEL_ADD_ERROR;
		chanel_num++;
		return chanel_num;
	}
	return chanel_num + chanel_manager_add(&((*chanel_man)->next), max_user_count);
}

chanel_manager * chanel_manager_find(chanel_manager * chanel_man, int chanelNum)
{
	chanel_manager * chanel = chanel_man;

	while(chanel != NULL)
	{
		if(chanel->chanel_num == chanelNum)
			return chanel;
		else
			chanel = chanel->next;
	}
	return CHANEL_NOT_FOUND;
}


//채널의 큐 생성  함수
static user_list * chanel_user_make(login_info user_info)
{
	//새로운 큐 할당.
	user_list * user = (user_list*)malloc(sizeof(user_list));
	if(user == NULL)
		return CHANEL_QUEUE_MAKE_ERROR;
	
	//초기화.
	user->user_sock = user_info.user_sock;
	sprintf(user->user_id, "%s", user_info.user_id);
	user->next = NULL;

	return user;
}
//채널의 워크큐에 작업을 추가하는 함수
static int chanel_add_user_list(chanel_manager * chanel_man, login_info user_info)
{
	user_list * user = chanel_user_make(user_info);
	if(user == CHANEL_QUEUE_MAKE_ERROR)
		return CHANEL_QUEUE_MAKE_ERROR_INT;
	
	//NULL 이면 현재 큐가 없다는 뜻.
	if(chanel_man->list_end == NULL)
	{
		chanel_man->list_start = chanel_man->list_end = user;
	}
	else{
		chanel_man->list_end->next = user;
		chanel_man->list_end = chanel_man->list_end->next;
	}
	
	//접속 유저의 수 증가.
	chanel_man->user_count++;

	//epoll 등록
	chanel_man->ep.data.fd = user->user_sock;
	chanel_man->ep.events = EPOLLIN;
	epoll_ctl(chanel_man->epoll_fd, EPOLL_CTL_ADD, user->user_sock, &chanel_man->ep);
	
	
	return SUCCESS;
}

int chanel_add_user(chanel_manager * chanel_man, int chanelNum, login_info user_info)
{
	//채널 구조체를 찾는다.
	chanel_manager * chanel = chanel_manager_find(chanel_man, chanelNum);
	if(chanel == CHANEL_NOT_FOUND)
		return CHANEL_NOT_FOUND_INT;

	//해당 채널에 작업을 추가한다.
	if(chanel_add_user_list(chanel, user_info))
		return CHANEL_QUEUE_MAKE_ERROR_INT;

	return SUCCESS;
}


int chanel_del_user(chanel_manager * chanel_man, int user_sock)
{
	user_list * before = NULL;
	user_list * list = chanel_man->list_start;

	while(list != NULL){
		if(list->user_sock == user_sock){
			//첫 부분에 있을 경우.
			if(before == NULL){
				if(list->next == NULL)
					chanel_man->list_start = chanel_man->list_end = NULL;
				else
					chanel_man->list_start = list->next;
			}
			else{
				if(list->next == NULL)
					chanel_man->list_end = before;
				else
					before->next = list->next;
			}

			free(list);
			chanel_man->user_count--;
			if(chanel_man->user_count == 0)
				chanel_man->list_start = chanel_man->list_end = NULL;
			epoll_ctl(chanel_man->epoll_fd, EPOLL_CTL_DEL, user_sock, NULL);
			return SUCCESS;
		}
		before = list;
		list = list->next;
	}
	//찾지 못하였을 경우.
	return USER_DEL_ERROR;
}


//모든 클라이언트에게 전송, 전송에 실패한 클라이언트의 수만큼 반환
//성공시 0
static int broadcast_message(user_list * user, char * message)
{
	int send_error = 0;
	if(user == NULL)
		return send_error;
	else
	{
		if(send(user->user_sock, message, strlen(message), 0) == -1)
			send_error++;
	}
	return send_error + broadcast_message(user->next, message);
}

//채널 스레드 동작 함수.
static void * chanel_thread(void * arg)
{
	chanel_manager * chanel = (chanel_manager *)arg;
	int epoll_event;	//epoll 이벤트가 일어난 디스크립터의 개수.
	int i;	//lOOP
	int event_sock; //소켓 
	
	char buffer[1024] = {0,}; //버퍼
	char temp_buffer[1024] = {0,};
	int recv_size;	//받은 크기

	int parse_result = 0; //파싱 결과 저장.
	char * user_id;

	//스레드 종료 요청에 대한 처리.
	//요청이 오면 바로 종료하도록 지정.
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	//스레드 아직 구현 안함
	while(chanel->server_stat)
	{
		epoll_event = epoll_wait(chanel->epoll_fd, chanel->ep_event, 10, -1);
		for(i = 0; i < epoll_event; i++)
		{
			memset(buffer, 0x00, 1024);

			//이벤트가 일어난 디스크립터
			event_sock = chanel->ep_event[i].data.fd;
			recv_size = recv(event_sock, buffer, 1024, 0);
			
			sprintf(temp_buffer,"%s", buffer);

			parse_result = parse(CHANEL_THREAD, buffer, NULL, NULL, NULL);
			if(parse_result == CACAO_CHANEL_USER)
			{//USER 정보 요청.
				memset(buffer, 0x00, 1024);
				get_user_list(chanel->list_start, buffer);
				send(event_sock, buffer, 1024, 0);
			}
			else if(recv_size == 0) //종료 요청
			{
				user_id = get_user_id(chanel->list_start, event_sock);
				sprintf(buffer, "SERVER\n%s\nCLOSE", user_id);
				//종료요청이온 유저 제거.
				if(chanel_del_user(chanel, event_sock) == USER_DEL_ERROR)
					close(event_sock);//Error
				else
					close(event_sock);//SUCCESS
				broadcast_message(chanel->list_start,buffer);
			}
			else	//종료요청이 아닐경우.
			{
				if(parse_result == CACAO_CHANEL_JOIN)
				{
					user_id = get_user_id(chanel->list_start, event_sock);
					sprintf(buffer, "SERVER\n%s\nCONNECT", user_id);
				}
				else
					sprintf(buffer, "%s", temp_buffer);
			
				if(broadcast_message(chanel->list_start, buffer))
				{
					//Error
					/*
					 * C# 기반 클라이언트 접속 후 접속 종료시 에러가 발생하지만 정상 동작..
					 * C 기반 테스트용 클라이언트는 접속 종료시 에러가 나지않는다.
					 * 두 소스는 같은 동작을 하는데 C# 에서 뭔가 내부적으로 다른듯 하다.
					 */
					printf("broadCast Error\n");
				}
			}

		}
	}
	return (void*) 0;
}

int chanel_start(chanel_manager * chanel_man)
{
	chanel_manager * chanel = chanel_man;
	//스레드 생성에 실패한 개수.
	int thread_create_failed = 0;

	while(chanel != NULL)
	{
		//epoll 생성
		chanel->epoll_fd = epoll_create(chanel->max_user_count);
		if(chanel->epoll_fd == -1)
		{
			thread_create_failed++;
			chanel = chanel->next;
			continue;
		}
		//epoll 에서 일어난 이벤트 최대 10개 까지 저장.
		chanel->ep_event = (struct epoll_event *)malloc(sizeof(struct epoll_event)*10);

		chanel->server_stat = RUN;		
		if(pthread_create(&chanel->chanel_thread, 0, chanel_thread, (void*)chanel))
			thread_create_failed++;
		chanel = chanel->next;
	}
	return thread_create_failed;
}

void chanel_close(chanel_manager * chanel_man)
{
	chanel_manager * temp = NULL;

	while(chanel_man != NULL)
	{
		//스레드 종료 요청.
		pthread_cancel(chanel_man->chanel_thread);
		pthread_join(chanel_man->chanel_thread, NULL);
		
		close(chanel_man->epoll_fd);
		
		//chanel_manager 구조체 할당 제거.
		temp = chanel_man;
		chanel_man = chanel_man->next;
		free(temp);
	}
}

void get_chanel_list(chanel_manager * chanel_man, char * chanel_buffer)
{
	chanel_manager * chanel = chanel_man;
	
	while(chanel != NULL)
	{
		sprintf(chanel_buffer, "%s\n%d:%d:%d",chanel_buffer, chanel->chanel_num,
				chanel->user_count, chanel->max_user_count);
		chanel = chanel->next;
	}
}

void get_user_list(user_list * user_li, char * user_buffer)
{
	user_list * user = user_li;
	strcpy(user_buffer, "USER_LIST");
	while(user != NULL)
	{
		sprintf(user_buffer, "%s\n%s", user_buffer, user->user_id);
		user = user->next;
	}
}

char * get_user_id(user_list * user_li, int user_sock)
{
	user_list * list = user_li;
	
	while(list != NULL)
	{
		if(list->user_sock == user_sock)
			return list->user_id;
		list = list->next;
	}
	return USER_NOT_FOUND;
}

int check_user(user_list * user_li, char * user_id)
{
	user_list * list = user_li;
	while(list != NULL)
	{
		if(strcmp(user_id, list->user_id) == 0)
			return 1;
		list = list->next;
	}
	return 0;
}
