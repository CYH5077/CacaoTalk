#ifndef __CACAOTALK_SERVER_CACAOSERVER_H__
#define __CACAOTALK_SERVER_CACAOSERVER_H__

#include <pthread.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include "../cacao_DB/ca_db.h"

//공통
#define SUCCESS 0
#define RUN 1
#define STOP 0

//Login 관련 에러값
#define LOGIN_QUEUE_ADD_ERROR -1
#define LOGIN_THREAD_CREATE_ERROR -2
#define LOGIN_QUEUE_MAKE_FAILED NULL
#define LOGIN_QUEUE_MAKE_FAILED_INT -3

//Login_queue 큐 저장 내용
typedef struct Login_info{
	int user_sock;	//Socket 디스크립터.
	char user_id[50]; //User ID
	char user_pass[50]; //User Pass(MD5)
	int chanelNum;	//접속할 chanel
	struct Login_info * next;
} login_info;

//로그인 스레드에서 사용될 큐
typedef struct Login_queue{
	ca_DB db;	//DB

	pthread_t login_thread;
	pthread_cond_t login_cond; //로그인 스레드 조건변수
	pthread_mutex_t login_mutex; //로그인 스레드 뮤텍스

	int main_epoll;	//Main 스레드의 epoll 디스크립터.
	int server_stat; //스레드 동작, 종료 지정.
	int queue_count; //현재 큐의 개수	
	struct Login_info * start; //큐의 시작지점 (front)
	struct Login_info * end;   //큐의 끝 (rear)
} login_queue;


//채널 관련에러 값
#define CHANEL_ADD_ERROR 1
#define USER_NOT_FOUND NULL
#define CHANEL_NOT_FOUND NULL
#define CHANEL_NOT_FOUND_INT 2
#define CHANEL_QUEUE_MAKE_ERROR NULL
#define CHANEL_QUEUE_MAKE_ERROR_INT 3
#define USER_DEL_ERROR 4

//채널 스레드에서  사용될 유저 리스트
typedef struct User_list{
	int user_sock;	//Socket 디스크립터.
	char user_id[50]; //User ID

	struct User_list * next;
} user_list;



//채널을 관리할 큐.
typedef struct Chanel_manager{
	int chanel_num;	//채널 번호
	int user_count;	//현재 접속한 User의 수
	int max_user_count; //현재 채널의 최대 User 의 수
	
	//스레드 관련
	pthread_t chanel_thread;	//채널 스레드 id

	//Chanel 접속 큐
	int list_count;
	user_list * list_start;
	user_list * list_end;

	//epoll
	int epoll_fd;	//epoll 디스크립터
	struct epoll_event ep;	//epoll에 디스크립터를 등록할정보를 담을 구조체.
	struct epoll_event * ep_event;	//epoll 에 일어난 디스크립터의 이벤트 저장할 구조체 배열.

	int server_stat;	//서버의 동작 상태.

	struct Chanel_manager * next;
} chanel_manager;


//Login thread arg
struct login_thread_arg{
	login_queue * login_arg;
	chanel_manager * chanel_arg;
};

/*
 * login_queue 초기화  함수(ca_login.c)
 * login_q - 초기화할 login_queue 구조체 주소
 */
void login_queue_init(login_queue * login_q);

/*
 * logine_queue 큐에 추가 함수 (ca_login.c)
 * login_q - logine_queue 구조체 포인터, user_sock - 소켓 디스크립터, user_id - User id, user_pass - User pass
 * 반환값
 * 성공시 SUCCESS(0)
 * 실패시 LOGIN_QUEUE_ADD_ERROR(-1)
 */
int login_queue_add(login_queue * login_q, int user_sock, char * user_id, char * user_pass, int chanelNum);

/*
 * login_queue 에서 데이터를 가져온다. (ca_login.c)
 * login_q - logine queue 구조체 포인터
 * 반환값
 * 성공시 SUCCESS(0)
 * 실패시 NULL
 */
login_info login_queue_get(login_queue * login_q);

/*
 * login thread 시작 함수 (ca_login.c)
 * login_q - login_queue 구조체 포인터
 * 반환값
 * 성공시 SUCCESS(0)
 * 실패시 NULL
 */
int login_thread_start(login_queue * login_q, chanel_manager * chanel_man);





/* 
 * chanel_manager 구조체 초기화 함수 (ca_chanel.c)
 * chanel_man - chanel_manager 구조체 포인터
 */
void chanel_manager_init(chanel_manager * chanel_man, int max_user_count);

/*
 * 채널을 추가한다 - 큐로 구성 (ca_chanel.c)
 * chanel_man - chanel_manager 구조체 포인터
 * 반환값
 * 성공시 SUCCESS(0)
 * 실패시 CHANEL_ADD_ERROR(-1)
 */
int chanel_manager_add(chanel_manager ** chanel_man, int max_user_count);

/*
 * 특정 채널의 구조체를 찾아 반환한다. (ca_chanel.c)
 * chanel_man - chanel_manager 구조체 포인터, chanelNum - 찾을 채널
 * 반환값
 * 성공시 찾은 채널의 구조체를 반환한다.
 * 실패시 CHANEL_NOT_FOUND(NULL) 반환.
 */
chanel_manager * chanel_manager_find(chanel_manager * chanel_man, int chanelNum);

/*
 * chanelNum 의 채널에 유저를  추가한다. (ca_chanel.c)
 * chanel_man - chanel_manager 구조체 포인터, chanelNum - 작업을 추가할 채널
 * work - 추가할 작업
 * 반환값
 * 성공시 SUCCESS(0)
 * 실패시 CHANEL_ADD_ERROR(1)
 */
int chanel_add_user(chanel_manager * chanel_man, int chanelNum, login_info user_info);

/*
 * user_sock 해당 유저를 chanel_man의 user_list에서 제거한다. (ca_chanel.c)
 * chanel_man - chanel_manager 구조체 포인터, user_sock - 제거할 유저 소켓 디스크립터
 * 반환값
 * 성공시 SUCCESS(0)
 * 실패시 USER_DEL_ERROR(4)
 */
int chanel_del_user(chanel_manager * chanel_man, int user_sock);

/* 
 * 채널 스레드 시작 함수 (ca_chanel.c)
 * chanel_man - chanel_manager 구조체 포인터, chanel_count - 생성할 채널의 개수
 * 반환값
 * 성공시 SUCCESS(0)
 * 실패시 CHANEL_ADD_ERROR(1)
 */
int chanel_start(chanel_manager * chanel_man);

/*
 * 채널을 종료한다.(ca_chanel.c)
 * chanel_man - chanel_manager 구조체 포인터.
 */
void chanel_close(chanel_manager * chanel_man);

/*
 * 채널 정보를 반환. (ca_chanel.c)
 * chanel_man - chanel_manager 구조체 포인터
 * chanel_buffer - 채널 정보를 저장할 버퍼.
 */
void get_chanel_list(chanel_manager * chanel_man, char * chanel_buffer);

/*
 * 유저 정보를 반환. (ca_chanel.c)
 * user_li - chanel_manager 구조체 내부의 user_list 구조체 포인터.
 * user_buffer - 유저의 정보를 저장할 버퍼.
 */
void get_user_list(user_list * user_li, char * user_buffer);

/*
 * 유저 id를 반환. (ca_chanel.c)
 * user_li - chanel_manager 의 user_list 구조체.
 * user_sock - 찾을 유저의 소켓 디스크립터
 * 반환값
 * 성공시 문자열 포인터 반환.
 * 실패시 USER_NOT_FOUND 
 */
char * get_user_id(user_list * user_li, int user_sock);

/*
 * user_id 가 채널에 존재하는지 확인(ca_chanel.c)
 * user_li - chanel_manager 의user_list 구조체
 * user_id - char * 문자열
 * 반환값
 * 찾았을 경우 1 반환
 * 못찾았을 경우 0 반환
 */
int check_user(user_list * user_li, char * user_id);

#endif
