#ifndef __CACAOTALK_CA_DB_H__
#define __CACAOTALK_CA_DB_H__

#include <stdio.h>
#include <mysql.h>
#include <errno.h>

//DB Name
#define CA_DB_NAME "cacaotalk"
//Mysql port
#define CA_DB_PORT 3306

typedef struct CACAO_DB {
	MYSQL db;	//MYSQL
} ca_DB;


/*
 * MYSQL 초기화 및 연결함수
 * db - ca_DB 구조체 포인터, id - MYSQL 계정,  passwd - MYSQL 암호
 * 반환값
 * 성공: 0 , 실패: -1
 */
int ca_db_connect(ca_DB * db, char * mysql_id, char * mysql_passwd);

/*
 * MYSQL close
 * db - ca_DB 구조체 포인터.
 */
void ca_db_close(ca_DB * db);

/*
 * 계정, 암호 체크 함수
 * db - ca_DB 구조체 포인터, id - User 계정, passwd - 계정 암호
 * passwd == NULL 이면 id 만 체크.
 * 반환값
 * 성공: 0, 실패: -1
 */
int user_check(ca_DB * db, char * id, char * passwd);

/*
 * 사용자 추가 함수
 * db - ca_DB 구조체 포인터, id - 추가 계정, passwd - 계정 암호
 * 반환값
 * 성공: 0, 실패: -1
 */
int user_add(ca_DB * db, char * id, char * passwd);

/*
 * 사용자 제거 함수
 * db - ca_DB 구조체 포인터, id - 제거할 계정
 * 성공: 0, 실패: -1
 */
int user_del(ca_DB * db, char * id);
#endif
