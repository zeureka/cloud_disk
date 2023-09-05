#include "../include/deal_mysql.h"
#include <stdio.h>
#include <string.h>

// 打印操作数据库出错时的错误信息
void print_error(MYSQL* conn, const char* title) {
    // %u 返回错误编号，%s 返回错误信息
    fprintf(stderr, "%s:\nERROR %u (%s)\n", title, mysql_errno(conn), mysql_error(conn));
}

// 连接数据库
MYSQL* mysql_conn(char* user_name, char* password, char* db_name) {
    MYSQL* conn = NULL;
    conn = mysql_init(NULL);

    if (NULL == conn) {
        fprintf(stderr, "MySQL init failed\n");
        return NULL;
    }

    // mysql_real_connect()尝试与运行在主机上的MySQL数据库引擎建立连接
    // conn: 是已有MYSQL结构的地址。调用mysql_real_connect()之前，必须调用mysql_init()来初始化MYSQL结构。
    // NULL: 值必须是主机名或IP地址。如果值是NULL或字符串"localhost"，连接将被视为与本地主机的连接。
    // user_name: 用户的MySQL登录ID
    // passwd: 参数包含用户的密码

    if (mysql_real_connect(conn, MYSQL_CONTAINER_ADDRESS, user_name, password, db_name, 3306, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_conn failed: ERROR %u(%s)\n", mysql_errno(conn), mysql_error(conn));
        mysql_close(conn);
        return NULL;
    }

    return conn;
}

// 处理数据库查询结果
void process_result_test(MYSQL* conn, MYSQL_RES* res_set) {
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(res_set)) != NULL) {
        // 处理每一行数据中的各个字段
        for (uint i = 0; i < mysql_num_fields(res_set); ++i) {
            if (i > 0) {
                fputc('\t', stdout);
            }
            printf("%s", (row[i] != NULL) ? row[i] : "NULL");
        }
        fputc('\n', stdout);
    }

    if (0 != mysql_errno(conn)) {
        print_error(conn, "mysql_fetch_row() failed");
    } else {
        printf("%lu rows returned \n", (ulong)mysql_num_rows(res_set));
    }
}

// 处理数据库查询结果，结果集保存在 buf 中，只处理一条记录，一个字段，
// 如果 buf 为NULL，无需保存结果结果集，只做判断有没有此记录
// 返回值：成功 0, 没有记录集 1, 有记录集但是没有保存 2, 失败 -1
int process_result_one(MYSQL *conn, char* sql_cmd, char* buf) {
    int ret = 0;
    MYSQL_RES* res_set = NULL; // 结果集结构的指针

    if(mysql_query(conn, sql_cmd) != 0) {
        print_error(conn, "mysql_query error!\n");
        return -1;
    }

    res_set = mysql_store_result(conn); // 生成结果集
    if (NULL == res_set) {
        print_error(conn, "mysql_store_result error!\n");
        return -1;
    }

    MYSQL_ROW row;
    ulong line = 0;

    // mysql_num_rows接受由mysql_store_result返回的结果结构集，并返回结构集中的行数
    line = mysql_num_rows(res_set);
    if (0 == line) {
        mysql_free_result(res_set);
        return 1;
    } else if (line > 0 && NULL == buf) {
        // 如果buf为NULL，无需保存结果集，只做判断有没有此记录
        mysql_free_result(res_set);
        return 2;
    }

    // mysql_fetch_row从结果结构中提取一行，并把它放到一个行结构中。当数据用完或发生错误时返回NULL.
    if ((row = mysql_fetch_row(res_set)) != NULL) {
        if (NULL != row[0]) {
            strcpy(buf, row[0]);
        }
    }

    mysql_free_result(res_set);

    return ret;
}
