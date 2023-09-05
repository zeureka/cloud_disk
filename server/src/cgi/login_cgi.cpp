/**
 * @file login_cgi.cpp
 * @brief 登录后台CGI程序
 */

#include <hiredis/hiredis.h>
#include <mysql/mysql.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <fcgi_config.h>
#include <fcgi_stdio.h>
#include <sys/types.h>

#include "../../include/cJSON.h"
#include "../../include/log.h"
#include "../../include/cfg.h"
#include "../../include/deal_mysql.h"
#include "../../include/redis_op.h"
#include "../../include/util_cgi.h"
#include "../../include/des.h"
#include "../../include/md5.h"
#include "../../include/base64.h"

/**
 * @brief 解析用户登录信息
 *
 * @param login_buf (in)    用户登录信息的JSON数据
 * @param account   (out)   账号
 * @param password  (out)   密码
 *
 * @return 函数是否执行成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int get_login_info(char* login_buf, char* account, char* password);

/**
 * @brief 判断用户登录情况
 *
 * @param account   (in)    账号
 * @param password  (in)    密码
 *
 * @return 函数是否执行成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int check_account_password(char* account, char* password);

/**
 * @brief 生成token字符串
 *
 * @param account   (in)    账号
 * @param token     (out)   生成的token字符串
 *
 * @return 函数是否执行成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int set_token(char* account, char* token);

/**
 * @brief 返回给前端登录的情况
 *
 * @param status_num    (in)    状态码
 * @param token         (in)    用户的token
 */
void return_login_status(const char* status_num, char* token);


// =======================================================================>


int get_login_info(char* login_buf, char* account, char* password) {
    cJSON* root = cJSON_Parse(login_buf);
    if (NULL == root) {
        TLOG_INFO("cJSON_Parse error");
        return -1;
    }

    // 返回指定字符串对应的 JSON 对象
    cJSON* child_account = cJSON_GetObjectItem(root, "account");
    if (NULL == child_account) {
        TLOG_INFO("cJSON_GetObjectItem('account') error");
        cJSON_Delete(root);
        root = NULL;
        return -1;
    }

    strcpy(account, child_account->valuestring);

    cJSON* child_password = cJSON_GetObjectItem(root, "password");
    if (NULL == child_password) {
        TLOG_INFO("cJSON_GetObjectItem('password') error");
        cJSON_Delete(root);
        root = NULL;
        return -1;
    }

    strcpy(password, child_password->valuestring);

	cJSON_Delete(root);
	root = NULL;
    return 0;
}

int check_account_password(char* account, char* password) {
    int ret = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};

    char mysql_user[256] = {0};
    char mysql_password[256] = {0};
    char mysql_db[256] = {0};

    // 从配置文件中读取 MySQL 的配置信息
    get_mysql_info(mysql_user, mysql_password, mysql_db);
    TLOG_INFO("mysql_user=%s, mysql_password=%s, mysql_db=%s", mysql_user, mysql_password, mysql_db);

    MYSQL* conn = mysql_conn(mysql_user, mysql_password, mysql_db);
    if (NULL == conn) {
        TLOG_INFO("mysql_conn error");
        return -1;
    }

    mysql_query(conn, "set names utf8mb4");
    sprintf(sql_cmd, "select password from account_info where account='%s'", account);

    // deal result
    char tmp[PASSWORD_LEN] = {0};

    // 返回值：0成功并保存记录集，1没有记录集，2有记录集但没有保存，-1失败
    process_result_one(conn, sql_cmd, tmp);
    TLOG_INFO("login password judge db_Password = %s, password = %s", tmp, password);
    if (0 == strcmp(tmp, password)) {
        ret = 0;
    } else {
        ret = -1;
    }

    mysql_close(conn);
    return ret;
}

int set_token(char* account, char* token) {
    int ret = 0;
    redisContext* conn = NULL;

    char redis_ip[30] = {0};
    char redis_port[10] = {0};
    char redis_password[30] = {0};

    // 读取 redis 配置信息
    // get_cfg_value(CFG_PATH, "redis", "ip", redis_ip);
    // get_cfg_value(CFG_PATH, "redis", "port", redis_port);
    get_redis_info(redis_ip, redis_port, redis_password);
    TLOG_INFO("redis:[ip=%s, port=%s, password=%s]", redis_ip, redis_port, redis_password);

    // connect redis database
    conn = rop_connectdb(redis_ip, redis_port, redis_password);
    if (NULL == conn) {
        TLOG_INFO("redis connected error");
        return -1;
    }

    // 用随机数产生一个token
    // 产生4个1000以内的随机数
    int rand_num[4];
    // 设置随机数种子
    srand((unsigned int)time(NULL));

    for (int i = 0; i < 4; ++i) {
        rand_num[i] = rand() % 1000;
    }

    char tmp[1024] = {0};
    sprintf(tmp, "%s%d%d%d%d", account, rand_num[0], rand_num[1], rand_num[2], rand_num[3]);
    TLOG_INFO("tmp = %s", tmp);

    // 加密
    char enc_tmp[2048] = {0};
    int enc_len = 0;
    ret = DesEnc((unsigned char*)tmp, strlen(tmp), (unsigned char*)enc_tmp, &enc_len);
    if (0 != ret) {
        TLOG_INFO("DesEnc error");
        rop_disconnect(conn);
        conn = NULL;
        return -1;
    }

    // base64
    char base64[1024 * 3] = {0};
    base64_encode((const unsigned char*)enc_tmp, enc_len, base64);

    // md5
    MD5_CTX md5;
    MD5Init(&md5);
    unsigned char decrypt[16];
    MD5Update(&md5, (unsigned char*)base64, strlen(base64));
    MD5Final(&md5, decrypt);

    char str[100] = {0};
    for (int i = 0; i < 26; ++i) {
        sprintf(str, "%02x", decrypt[i]);
        strcat(token, str);
    }

    // 设置 token 有效期 24h
    ret = rop_setex_string(conn, account, 86400, token);

    return ret;
}

void return_login_status(const char* status_num, char* token) {
    char* out = NULL;
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "code", status_num);
    cJSON_AddStringToObject(root, "token", token);
    out = cJSON_Print(root);
    cJSON_Delete(root);

    if (NULL != out) {
        printf("%s", out);
        free(out);
    }
}


int main() {
    // Log* log = Log::get_instance();
    // log->init("server_log");

    // blacking waiting for user connection
    while (FCGI_Accept() >= 0) {
        // 搜索 CONTENT_LENGTH 所指向的环境变量字符串，并返回相关的值给字符串
        char* contentLength = getenv("CONTENT_LENGTH");
        int len = 0;
        char token[128] = {0};

        printf("Content-Type: text/html\r\n\r\n");

        if (NULL == contentLength) {
            len = 0;
        } else {
            len = atoi(contentLength);
        }

        // no login user information
        if (len <= 0) {
            printf("No data from standard input.<p>\n");
            TLOG_INFO("len = 0, No data from standard input");
        } else {
            // obtain login user information
            char buf[4096] = {0};
            int ret = 0;
            // 从 stdin 中读取数据到 ptr 字符数组中
            ret = fread(buf, 1, len, stdin);

            if (0 == ret) {
                TLOG_INFO("fread(buf, 1, len, stdin) error");
                continue;
            }

            TLOG_INFO("buf = %s", buf);

            char account[512] = {0};
            char password[512] = {0};

            get_login_info(buf, account, password);
            TLOG_INFO("account = %s, password = %s", account, password);

            // login judgment, success return 0, failure return -1
            ret = check_account_password(account, password);
            if (0 == ret) {
                // login success
                memset(token, 0, sizeof(token));
                ret = set_token(account, token);
                TLOG_INFO("token = %s", token);
            }

            if (0 == ret) {
                return_login_status(std::string("000").c_str(), token);
            } else {
                return_login_status(std::string("001").c_str(), "fail");
            }
        }
    }
    return 0;
}
