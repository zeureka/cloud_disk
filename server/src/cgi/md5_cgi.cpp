/**
 * @file md5_cgi.cpp
 * @brief 妙传后台CGI程序
 */

#include "../../include/cJSON.h"
#include "../../include/cfg.h"
#include "../../include/log.h"
#include "../../include/deal_mysql.h"
#include "../../include/util_cgi.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <mysql/mysql.h>
#include <string.h>
#include <sys/select.h>
#include <fcgi_stdio.h>
#include <fcgi_config.h>

// mysql 数据库配置信息
static char mysql_user[128] = {0};
static char mysql_pwd[128] = {0};
static char mysql_db[128] = {0};

/**
 * @brief 读取配置信息
 */
void read_cfg();

/**
 * @brief 解析JSON包的数据
 *
 * @param buf       (in)    带有数据信息的JSON格式字符串
 * @param account   (out)   用户账号
 * @param token     (out)   此账号对应的token
 * @param md5       (out)   文件的md5
 * @param fileName  (out)   文件名
 *
 * @return 解析过程中，是否成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int get_md5_info(char* buf, char* account, char* token, char* md5, char* fileName);

/**
 * @brief 秒传处理函数
 *
 * @param account   (in)   用户账号
 * @param md5       (in)   文件的md5
 * @param fileName  (in)   文件名
 *
 * @return 解析过程中，是否成功
 *   @retval 0  success
 *   @retval -1 error
 *   @retval -2 此用户已拥有此文件
 *   @retval -3 failed
 */
int deal_md5(char* account, char* md5, char* fileName);


// ==========================================================================>


// 读取配置信息
void read_cfg() {
    get_mysql_info(mysql_user, mysql_pwd, mysql_db);
    // get_cfg_value(CFG_PATH, "mysql", "user", mysql_user);
    // get_cfg_value(CFG_PATH, "mysql", "password", mysql_pwd);
    // get_cfg_value(CFG_PATH, "mysql", "database", mysql_db);
    TLOG_INFO("mysql:[user=%s, pwd=%s, database=%s]", mysql_user, mysql_pwd, mysql_db);
}

// 解析秒传信息的JSON包
int get_md5_info(char* buf, char* account, char* token, char* md5, char* fileName) {
    /* JSON数据包结构
     * {
     *      account: xxxx,
     *      token: xxxx,
     *      md5: xxxx,
     *      filename: xxxx
     * }
     * */

    cJSON* root = cJSON_Parse(buf);
    if (NULL == root) {
        TLOG_INFO("cJSON_Parse error");
        return -1;
    }

    cJSON* account_child = cJSON_GetObjectItem(root, "account");
    if (NULL == account_child) {
        TLOG_INFO("cJSON_GetObjectItem error");
        cJSON_Delete(root);
		root = NULL;
        return -1;
    }
    strcpy(account, account_child->valuestring);

    cJSON* token_child = cJSON_GetObjectItem(root, "token");
    if (NULL == token_child) {
        TLOG_INFO("cJSON_GetObjectItem error");
        cJSON_Delete(root);
		root = NULL;
        return -1;
    }
    strcpy(token, token_child->valuestring);

    cJSON* md5_child = cJSON_GetObjectItem(root, "md5");
    if (NULL == md5_child) {
        TLOG_INFO("cJSON_GetObjectItem error");
        cJSON_Delete(root);
		root = NULL;
        return -1;
    }
    strcpy(md5, md5_child->valuestring);

    cJSON* fileName_child = cJSON_GetObjectItem(root, "fileName");
    if (NULL == fileName_child) {
        TLOG_INFO("cJSON_GetObjectItem error");
        cJSON_Delete(root);
		root = NULL;
        return -1;
    }
    strcpy(fileName, fileName_child->valuestring);

	cJSON_Delete(root);
	root = NULL;
    return 0;
}

// 秒传处理
// 成功：0, 出错：-1, 此用户已拥有此文件：-2, 失败：-3
int deal_md5(char* account, char* md5, char* fileName) {
    int ret = 0;
    int ret2 = 0;
    char tmp[512] = {0};
    char sql_cmd[SQL_MAX_LEN] = {0};
    MYSQL* conn = NULL;
    char* out = NULL;

    conn = mysql_conn(mysql_user, mysql_pwd, mysql_db);
    if (NULL == conn) {
        TLOG_INFO("mysql_conn() error");
        ret = -1;
        goto END;
    }

    mysql_query(conn, "set names utf8mb4");
    /*秒传文件JSON包结构
     * {
     *      文件已存在 {"code":"005"}
     *      秒传成功   {"code":"006"}
     *      秒传失败   {"code":"007"}
     * }
     * */

    // 查看数据库是否有此文件的md5
    // 如果没有，返回 {"code":"006"}，代表不能秒传
    // 如果有，修改file_info中的count字段 +1

    sprintf(sql_cmd, "select count from file_info where md5 = '%s'", md5);
    ret2 = process_result_one(conn, sql_cmd, tmp);
    if (0 == ret2) {
        int count = atoi(tmp); // 拥有此文件的用户数

        // 查看此用户书否已经有此文件，如果存在说明此文件已有上传记录，无需再上传
        sprintf(sql_cmd, "select * from account_file_list where account = '%s' and md5 = '%s' and file_name = '%s'", account, md5, fileName);
        ret2 = process_result_one(conn, sql_cmd, NULL);
        if (2 == ret2) {
            TLOG_INFO("%s[fileName:%s, md5:%s]已存在", account, fileName, md5);
            ret = -2;
            goto END;
        }

        // 修改file_info 中的count字段，+1
        sprintf(sql_cmd, "update file_info set count = %d where md5 = '%s'", ++count, md5);
        if (mysql_query(conn, sql_cmd) != 0) {
            TLOG_INFO("%s Command execution failed： %s", sql_cmd, mysql_error(conn));
            ret = -1;
            goto END;
        }

        // account_file_list, 用户文件列表插入一条数据
        struct timeval tv;
        struct tm* ptm;
        char time_str[128] = {0};

        // 获取时间，精度为微秒
        gettimeofday(&tv, NULL);
        // 把从1970-1-1零点零分到当前时间系统所偏移的秒数时间转换为本地时间
        ptm = localtime(&tv.tv_sec);
        // strftime() 函数根据区域设置格式化本地时间/日期，
        // 函数的功能将时间格式化，或者说格式化一个时间字符串
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", ptm);

        sprintf(sql_cmd, "insert into account_file_list(account, md5, create_time, file_name, shared_status, pv) values ('%s', '%s', '%s', '%s', %d, %d)", account, md5, time_str, fileName, 0, 0);
        if (mysql_query(conn, sql_cmd) != 0) {
            TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn));
            ret = -1;
            goto END;
        }

        // 查询用户文件数量
        sprintf(sql_cmd, "select count from account_file_count where account = '%s'", account);
        ret2 = process_result_one(conn, sql_cmd, tmp);
        if (1 == ret2) {
            sprintf(sql_cmd, "insert into account_file_count (account, count) value ('%s', %d)", account, 1);
        } else if (0 == ret2) {
            count = atoi(tmp);
            sprintf(sql_cmd, "update account_file_count set count = %d where account = %s",++count ,account);
        }

        // 更新用户文件拥有量数据
        if (mysql_query(conn, sql_cmd) != 0) {
            TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn));
            ret = -1;
            goto END;
        }
    } else if (1 == ret2) {
        ret = -3;
        goto END;
    }


END:
    if (0 == ret) {
        out = return_status("006");
    } else if (-2 == ret) {
        out = return_status("005");
    } else {
        out = return_status("007");
    }

    if (NULL != out) {
	printf("%s", out);
	free(out);
	out = NULL;
    }

    if (NULL != conn) {
        mysql_close(conn);
	conn = NULL;
    }

    return ret;
}

int main() {
    // 读取数据库配置信息
    read_cfg();

    // 阻塞等待用户连接
    while (FCGI_Accept() >= 0) {
        char* contentLength = getenv("CONTENT_LENGTH");
        int len = 0;
        printf("Content-Type: text/html\r\n\r\n");

        if (NULL == contentLength) {
            len = 0;
        } else {
            len = atoi(contentLength);
        }

        if (len <= 0) {
            printf("No data from standard input.<p>");
            TLOG_INFO("len = 0, No data from standard input");
        } else {
            // 获取登录用户信息
            char buf[4 * 1024] = {0};
            int ret = 0;
            ret = fread(buf, 1, len, stdin);
            if (0 == ret) {
                TLOG_INFO("fread(buf, 1, len, stdin) error");
                continue;
            }

            TLOG_INFO("buf = %s", buf);

            /* 解析JSON中的信息
             * {
             *      account: xxxx,
             *      token: xxxx,
             *      md5: xxxx,
             *      fileName: xxxx
             * }
             * */

            char account[128] = {0};
            char md5[256] = {0};
            char token[256] = {0};
            char fileName[128] = {0};
            ret = get_md5_info(buf, account, token, md5, fileName);
            if (0 != ret) {
                TLOG_INFO("get_md5_info() error");
                continue;
            }

            TLOG_INFO("account=%s, token=%s, md5=%s, fileName=%s", account, token, md5, fileName);
            // 验证登录token，成功：0, 失败：-1
            ret = verify_token(account, token);
            if (0 == ret) {
                deal_md5(account, md5, fileName);
            } else {
                char* out = return_status("111");
                if (NULL != out) {
                    // 给前端反馈错误码
                    printf("%s", out);
                    free(out);
                }
            }
       }
    }

    return 0;
}
