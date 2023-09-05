/**
 * @file enroll_cgi.c
 * @brief  注册事件后台CGI程序
 */

#include "../../include/cfg.h"
#include "../../include/deal_mysql.h"
#include "../../include/util_cgi.h"
#include <ctime>
#include <mysql/mysql.h>
#include <sys/time.h>
#include <fcgi_stdio.h>
#include <fcgi_config.h>

/**
 * @brief 解析用户注册信息的JSON包
 *
 * @param enroll_buf    (in)    带有用户注册信息的字符串
 * @param account       (out)   用户账号
 * @param password      (out)   账号密码
 * @param phone         (out)   用户电话号码
 * @param email         (out)   用户电子邮箱
 *
 * @return 判断函数是否成功执行
 *   @retval 0  success
 *   @retval -1 failed
 */
int get_enroll_info(char* enroll_buf, char* account, char* password, char* phone, char* email);

/**
 * @brief 用户注册处理函数
 *
 * @param enroll_buf    (in)    带有用户注册信息的字符串
 *
 * @return 判断函数是否成功执行
 *   @retval 0  success
 *   @retval -1 failed
 */
int account_enroll(char* enroll_buf);

// =============================================================================>

int get_enroll_info(char* enroll_buf, char* account, char* password, char* phone, char* email) {
    /* JSON Data
     {
        account: xxxx,
        password: xxxx,
        phone: xxxx,
        email: xxxx
     }
     */

    cJSON* root = cJSON_Parse(enroll_buf);
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

    cJSON* password_child = cJSON_GetObjectItem(root, "password");
    if (NULL == password_child) {
        TLOG_INFO("cJSON_GetObjectItem error");
        cJSON_Delete(root);
		root = NULL;
        return -1;
    }
    strcpy(password, password_child->valuestring);

    cJSON* phone_child = cJSON_GetObjectItem(root, "phone");
    if (NULL == phone_child) {
        TLOG_INFO("cJSON_GetObjectItem error");
        cJSON_Delete(root);
		root = NULL;
        return -1;
    }
    strcpy(phone, phone_child->valuestring);

    cJSON* email_child = cJSON_GetObjectItem(root, "email");
    if (NULL == email_child) {
        TLOG_INFO("cJSON_GetObjectItem error");
        cJSON_Delete(root);
		root = NULL;
        return -1;
    }
    strcpy(email, email_child->valuestring);

    cJSON_Delete(root);
	root = NULL;
    return 0;
}

// 注册用户，成功：0, 失败：-1, 用户已存在：-2
int account_enroll(char* enroll_buf) {
    MYSQL* conn = NULL;
    int ret = 0;

    // 获取数据库信息
    char mysql_user[256] = {0};
    char mysql_password[256] = {0};
    char mysql_db[256] = {0};
    ret = get_mysql_info(mysql_user, mysql_password, mysql_db);
    if (0 != ret) {
        return -1;
    }
    TLOG_INFO("mysql_user=%s, mysql_password=%s, mysql_db=%s", mysql_user, mysql_password, mysql_db);


    // 获取注册用户信息
    char account[128] = {0};
    char password[128] = {0};
    char phone[128] = {0};
    char email[128] = {0};

    ret = get_enroll_info(enroll_buf, account, password, phone, email);
    if (0 != ret) {
        return -1;
    }
    TLOG_INFO("account=%s, password=%s, phone=%s, email=%s", account, password, phone, email);

    // connect the database
    conn = mysql_conn(mysql_user, mysql_password, mysql_db);
    if (NULL == conn) {
        TLOG_INFO("mysql_conn error");
        return -1;
    }

    mysql_query(conn, "set names utf8mb4");
    char sql_cmd[SQL_MAX_LEN] = {0};
    sprintf(sql_cmd, "SELECT * FROM account_info WHERE account = '%s'", account);
    TLOG_INFO("select account sql_cmd: %s", sql_cmd);
    

    // 查看该用户是否存在
    ret = process_result_one(conn, sql_cmd, NULL);
    if (2 == ret) {
        TLOG_INFO("【%s】该用户已存在", account);
        mysql_close(conn);
        return -2;
    }

    // 获取当前时间戳
    struct timeval tv;
    struct tm* ptm;
    char time_str[128];

    // 使用函数gettimeofday()函数来得到时间，精度可达到微
    gettimeofday(&tv, NULL);

    // 把时间从1970.1.1:00:00到当前时间系统的偏移秒数转换为本地时间
    ptm = localtime(&tv.tv_sec);

    // strftime() 函数会根据区域设置格式化本地时间/日期，函数的功能将时间格式化，或者说格式化一个时间字符串
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", ptm);

    // 插入注册信息
    sprintf(sql_cmd, "INSERT INTO account_info (account, password, phone, email, create_time) VALUES ('%s', '%s', '%s', '%s', '%s')", account, password, phone, email, time_str);
    ret = mysql_query(conn, sql_cmd);
    if (0 != ret) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn));
        mysql_close(conn);
        return -1;
    }

    return 0;
}

int main() {
    // 阻塞等待用户连接
    while(FCGI_Accept() >= 0) {
        // 获取环境变量(CONTENT_LENGTH)的值
        char* contentLength = getenv("CONTENT_LENGTH");
        int len = 0;
        printf("Content_Type: text/html\r\n\r\n");

        if (NULL == contentLength) {
            len = 0;
        } else {
            len = atoi(contentLength);
        }

        if (len <= 0) {
            // 没有登录用户信息
            printf("No data from standard input.<p>");
            TLOG_INFO("len = 0, No data from standard input");
        } else {
            // 获取登录用户信息
            char buf[4 * 1024] = {0};
            int ret = 0;
            char* out = NULL;
            // 从标准输入(web服务器)中读取内容
            ret = fread(buf, 1, len, stdin);
            if (0 == ret) {
                TLOG_INFO("fread(buf, 1, len, stdin) error");
                continue;
            }

            TLOG_INFO("buf=%s", buf);
            /*注册:
             *  成功: {code: "002"}
             *  该用户已存在: {code: "003"}
             *  失败: {code: "004"}
             */
            ret = account_enroll(buf);
	    
            if (0 == ret) {
                // 登录成功
                out = return_status("002");
            } else if (-1 == ret) {
                out = return_status("004");
            } else if (-2 == ret) {
                out = return_status("003");
            }

	    TLOG_INFO("ret = %d, return_status() = %s", ret, out);
            if (NULL != out) {
                printf("%s", out);
                free(out);
            }
        }
    }

    return 0;
}
