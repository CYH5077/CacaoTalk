#include "./server/cacao_server.h"
#include "./server/parse.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

/*
 * epoll 생성
 * list epoll_event 구조체
 * 반환값
 * 성공시 epoll 디스크립터
 * 실패시 -1
 */
int epoll_make()
{
	int epoll_fd = epoll_create(10);
	if(epoll_fd == -1)
		return -1;

	return epoll_fd;
}

/*
 * epoll에 디스크립터 추
 * epoll_fd - epoll 디스크립터, fd - 추가 할 디스크립터
 * 반환값
 * 성공시 - 0
 * 실패시 - -1
 */
int epoll_add(int epoll_fd, int fd)
{
	struct epoll_event ep;
	ep.data.fd = fd;
	ep.events = EPOLLIN;

	if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ep) == -1)
		return -1; //FAILED
	return 0; //SUCCESS
}
/* 위와 같음 (제거) */
int epoll_del(int epoll_fd, int fd)
{
	if(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1)
		return -1; //FAILED
	return 0; //SUCCESS
}

/*
 * bind, listen 수행. 
 * server_sock - 서버 소켓, serv_addr - 초기화 한 sockaddr_in 구조체.
 * 반환값
 * 성공시 소켓 디스크립터.
 * 실패시 -1
 */
int server_register(struct sockaddr_in serv_addr)
{
	int server_sock;
	server_sock = socket(PF_INET, SOCK_STREAM, 0);
	if(server_sock == -1)
		return -1;
	if(bind(server_sock, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr_in)) == -1)
		return -1;
	if(listen(server_sock, 5) == -1)
		return -1;
	return server_sock;
}

/*
 * 클라이언트로 부터 받은 데이터 처리 함수.
 * client_sock - 클라이언트 소켓, message - 받은 데이터
 * 반환값
 * 성공시 0
 * 실패시 -1
 */
int parse_run(int client_sock, char * message, login_queue * login_qu, chanel_manager * chanel_man)
{
	char user_id[50] = {0,}, user_pass[50] = {0,};
	char recv_buf[1024] = {0,};
	int chanelNum = 0;
	int ret_val = 0;

	ret_val = parse(LOGIN_THREAD, message, user_id, user_pass, &chanelNum);
	switch(ret_val)
	{
		case CACAO_CHANEL_INFO:
			get_chanel_list(chanel_man, recv_buf);
			send(client_sock, recv_buf, 1024, 0);
			break;
		case CACAO_LOGIN:
			if(login_queue_add(login_qu, client_sock, user_id, user_pass, chanelNum))
				return -1;
			break;
		case CACAO_ADD_USER:
			if(user_add(&login_qu->db, user_id, user_pass))
				send(client_sock, "USER_ADD_ERROR", 14, 0);
			else
				send(client_sock, "ADD_SUCCESS", 11, 0);
			break;
		case CACAO_DEL_USER:
			if(user_del(&login_qu->db, user_id))
				send(client_sock, "USER_DEL_ERROR", 14, 0);
			else
				send(client_sock, "DEL_SUCCESS", 11, 0);
			break;
		default:
			send(client_sock, "Your data error", 15, 0);
			return -1;
	}
	return ret_val;
}

/* 
 * 서버 요청 처리 함수
 * server_sock - 서버 소켓, epoll_fd - epoll 디스크립터, ep_list 메모리 할당된 epoll_event 구조체
 * login_qu - login_queue 구조체.
 * 반환값
 * 성공시 0
 * 실패시 -1
 */
int run_server(int server_sock, int epoll_fd, login_queue * login_qu, chanel_manager * chanel_man)
{
	int client_sock;
	struct sockaddr_in client_addr;
	socklen_t addr_size = sizeof(client_addr);
	int event_count, loop;
	char message[1024] = {0,};
	struct epoll_event ep_list[10];
	
	if(epoll_add(epoll_fd,server_sock) == -1)
	{
		perror("epoll_add failed");
		return -1;
	}
	while(1)
	{
		event_count = epoll_wait(epoll_fd, ep_list, 10, -1);
		if(event_count == -1)
		{
			perror("epoll_wait() failed");
			return -1;
		}
		for(loop = 0; loop < event_count; loop++)
		{
			
			//클라이언트 접속 요청 처리.
			if(ep_list[loop].data.fd == server_sock)
			{
				client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
				if(client_sock == -1)
					continue;
				if(epoll_add(epoll_fd, client_sock))
					continue;
			}
			else //서비스 요청
			{
				//버퍼 초기화
				memset(message, 0x00, 1024);

				//클라이언트 종료 요청,
				if(recv(ep_list[loop].data.fd, message, 1024, 0) == 0){
					epoll_del(epoll_fd, ep_list[loop].data.fd);
					close(ep_list[loop].data.fd);
				}
				else{
					if(parse_run(ep_list[loop].data.fd, message, login_qu, chanel_man) == CACAO_LOGIN)
						epoll_del(epoll_fd, ep_list[loop].data.fd);
				}
			}
		} //for
	} //while

	return 0;
}
int main(void)
{
	int epoll_fd; //epoll 디스크립터.;
	
	int loop;

	login_queue login_qu;
	chanel_manager * chanel_man = NULL;

	int server_sock;	//소켓 디스크립터.
	struct sockaddr_in serv_addr;	

	serv_addr.sin_family = PF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(10000);

	server_sock = server_register(serv_addr);
	

	//epoll 생성
	epoll_fd = epoll_make();
	if(epoll_fd == -1){
		printf("epoll_make() failed\n");
		return -1;
	}
	printf("START\n");

	//login_queue 초기화
	login_queue_init(&login_qu);
	login_qu.main_epoll = epoll_fd;

	//채널 5개 생성
	for(loop = 0 ; loop < 5; loop++)
	{
		if(chanel_manager_add(&chanel_man, 5) == CHANEL_ADD_ERROR)
		{
			printf("chanel_manager_add() failed\n");
			chanel_close(chanel_man);
			return -1;
		}
	}
	//채널 스레드 시작.
	if(chanel_start(chanel_man)){
		printf("chanel_start() failde\n");
		chanel_close(chanel_man);
		return -1;
	}

	//login 스레드 시작
	if(login_thread_start(&login_qu, chanel_man) == LOGIN_THREAD_CREATE_ERROR){
		printf("login_thread_start() failed\n");
		return -1;
	}
	
	//서비스 시작.
	if(run_server(server_sock, epoll_fd, &login_qu, chanel_man)){
		printf("run_server() failed\n");
		return -1;
	}

	chanel_close(chanel_man);
	printf("Finish\n");
	return 0;
}
