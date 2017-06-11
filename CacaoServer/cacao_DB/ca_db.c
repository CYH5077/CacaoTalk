#include "ca_db.h"
#include <string.h>

int ca_db_connect(ca_DB *db, char * mysql_id, char * mysql_passwd)
{
	//초기화
	mysql_init(&db->db);

	//Mysql 연결
	if(!mysql_real_connect(&db->db, NULL, mysql_id, mysql_passwd,
				CA_DB_NAME, CA_DB_PORT, (char*)NULL,0))
	{
		printf("DB connect Error : %s\n", mysql_error(&db->db));
		return -1;
	}
	return 0;
}

void ca_db_close(ca_DB * db)
{
	mysql_close(&db->db);
}

int user_check(ca_DB *db, char * id, char * passwd)
{
	char query[100] = {0,};

	MYSQL_RES * result = NULL;

	//계정만 체크.
	if(passwd == NULL)
		sprintf(query, "SELECT * FROM user WHERE id='%s'", id);
	else //계정과 암호 둘다 체크.
		sprintf(query, "SELECT * FROM user WHERE id='%s' AND pass='%s'", id, passwd);

	//쿼리 실행.
	if(mysql_query(&db->db, query))
		return -1;	//Error
	
	result = mysql_store_result(&db->db);
	if(mysql_fetch_row(result) == NULL) //반환된 결과가 없을 경우
	{
		mysql_free_result(result);
		return -1;
	}
	mysql_free_result(result);
	return 0;	//Success
}

int user_add(ca_DB *db, char * id, char * passwd)
{
	char query[100] = {0,};

	sprintf(query, "INSERT INTO user(id,pass) VALUES('%s', '%s')", id, passwd);
	//쿼리 실행 
	if(mysql_query(&db->db, query))
		return -1;

	return 0;
}

int user_del(ca_DB * db, char * id)
{
	char query[100] = {0, };

	sprintf(query, "DELETE FROM user WHERE id='%s'", id);
	
	//쿼리 실행
	if(mysql_query(&db->db, query))
		return -1;

	//영향을 받은 ROW 수
	if(mysql_affected_rows(&db->db) == 0)
		return -1;

	return 0;
}
