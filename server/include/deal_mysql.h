/**
 * @file deal_mysql.h
 * @brief mysql数据库部分接口包装
 */

#ifndef _DEAL_MYSQL_H_
#define _DEAL_MYSQL_H_

#include <mysql/mysql.h>

#define SQL_MAX_LEN 512
#define MYSQL_CONTAINER_ADDRESS "172.18.46.97"

/**
 * @brief 打印操作数据库出错时的错误信息
 *
 * @param conn  (in)    数据库句柄
 * @param title (in)    错误信息
 */
void print_error(MYSQL* conn, const char* title);

/**
 * @brief 连接数据库函数
 *
 * @param user_name (in)    数据库用户名(root)
 * @param password  (in)    数据库密码
 * @param db_name   (in)    需要操作的数据库
 *
 * @return 返回数据库的句柄
 *   @retval 数据库句柄 success
 *   @retval NULL       failed
 */
MYSQL* mysql_conn(char* user_name, char* password, char* db_name);

/**
 * @brief 处理数据库查询结果
 *
 * @param conn      (in)    数据库句柄
 * @param res_set   (in)    数据结果集
 */
void process_result_test(MYSQL* conn, MYSQL_RES* res_set);

/**
 * @brief 处理数据库查询结果
 * @detail 处理数据库查询结果，结果集保存在 buf 中，只处理一条记录，一个字段，
 *         如果 buf 为NULL，无需保存结果结果集，只做判断有没有此记录
 *
 * @param conn      (in)    数据库句柄
 * @param sql_cmd   (in)    需要执行的sql语句
 * @param buf       (out)   存放sql语句执行结果的缓冲区
 *
 * @return 函数是否执行成功
 *   @retval 0  success
 *   @retval 1  没有结果集
 *   @retval 2  有结果集，单数没有保存(即buf为NULL)
 *   @retval -1 failed
 */
int process_result_one(MYSQL *conn, char* sql_cmd, char* buf);

#endif // _DEAL_MYSQL_H_
