/**
 * @file dealsharefile_cgi.cpp
 * @brief 共享文件downloads字段处理，取消分享，转存文件CGI程序
 */

#include "../../include/deal_mysql.h"
#include "../../include/cfg.h"
#include "../../include/redis_op.h"
#include "../../include/cJSON.h"
#include "../../include/log.h"
#include "../../include/redis_keys.h"
#include "../../include/util_cgi.h"

#include <cstdlib>
#include <fcgi_stdio.h>
#include <fcgi_config.h>

/// MySQL 数据库配置信息
static char mysql_user[128] = {0};
static char mysql_password[128] = {0};
static char mysql_db[128] = {0};

/// Redis 数据库配置信息
static char redis_ip[30] = {0};
static char redis_port[10] = {0};
static char redis_password[30] = {0};

/**
 * @brief 读取配置文件信息
 */
void read_cfg();

/**
 * @brief 解析 JSON 包
 *
 * @param buf       (in)    JSON包数据
 * @param account   (out)   用户账号
 * @param md5       (out)   文件的md5值
 * @param fileName  (out)   文件名
 *
 * @return
 *   @retval 0  success
 *   @retval -1 failed
 */
int get_json_info(char* buf, char* account, char* md5, char* fileName);

/**
 * @brief 文件下载字段(downloads)处理
 *
 * @param md5       (in)    待操作的文件md5值
 * @param fileName  (in)    待操作的文件的文件名
 *
 * @return
 *   @retval 0  success
 *   @retval -1 failed
 */
int downloads_file(char* md5, char* fileName);

/**
 * @brief 取消分享文件处理函数
 *
 * @param account   (in)    操作此文件的账号
 * @param md5       (in)    待操作的文件md5值
 * @param fileName  (in)    待操作的文件的文件名
 *
 * @return
 *   @retval 0  success
 *   @retval -1 failed
 */
int cancel_share_file(char* account, char* md5, char* fileName);

/**
 * @brief 转存文件
 *
 * @param account   (in)    操作此文件的账号
 * @param md5       (in)    待操作的文件md5值
 * @param fileName  (in)    待操作的文件的文件名
 *
 * @return
 *   @retval 0  success
 *   @retval -1 转存失败
 *   @retval -2 文件已存在
 */
int save_file(char* account, char* md5, char* fileName);


// =====================================================================>


void read_cfg() {
    get_mysql_info(mysql_user, mysql_password, mysql_db);
    TLOG_INFO("mysql:[user=%s, password=%s, database=%s]", mysql_user, mysql_password, mysql_db);

    get_redis_info(redis_ip, redis_port, redis_password);
    TLOG_INFO("redis:[ip=%s, port=%s], password=%s", redis_ip, redis_port, redis_password);
}

int get_json_info(char* buf, char* account, char* md5, char* fileName) {
    cJSON* root = cJSON_Parse(buf);
    if (NULL == root) {
        TLOG_INFO("cJSON_Parse() failed");
        return -1;
    }

    cJSON* account_child = cJSON_GetObjectItem(root, "account");
    if (NULL == account_child) {
        TLOG_INFO("cJSON_GetObjectItem() failed");
        cJSON_Delete(root);
        root = NULL;
        return -1;
    }
    strcpy(account, account_child->valuestring);

    cJSON* md5_child = cJSON_GetObjectItem(root, "md5");
    if (NULL == md5) {
        TLOG_INFO("cJSON_GetObjectItem() failed");
        cJSON_Delete(root);
        root = NULL;
        return -1;
    }
    strcpy(md5, md5_child->valuestring);

    cJSON* fileName_child = cJSON_GetObjectItem(root, "fileName");
    if (NULL == fileName_child) {
        TLOG_INFO("cJSON_GetObjectItem() failed");
        cJSON_Delete(root);
        root = NULL;
        return -1;
    }
    strcpy(fileName, fileName_child->valuestring);

    cJSON_Delete(root);
    root = NULL;
    return 0;
}

int downloads_file(char* md5, char* fileName) {
    int ret = 0;
    int ret2 = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    MYSQL* conn_mysql = NULL;
    char* out = NULL;
    char tmp[512] = {0};
    redisContext* conn_redis = NULL;
    char fileId[1024] = {0};
    int downloads = 0;

    conn_redis = rop_connectdb(redis_ip, redis_port, redis_password);
    if (NULL == conn_redis) {
        TLOG_INFO("redis connected failed");
        ret = -1;
        goto END;
    }

    conn_mysql = mysql_conn(mysql_user, mysql_password, mysql_db);
    if (NULL == conn_mysql) {
        TLOG_INFO("mysql connected failed");
        ret = -1;
        goto END;
    }

    // 文件标识 md5 + fileName
    sprintf(fileId, "%s%s", md5, fileName);
    mysql_query(conn_mysql, "SET names utf8mb4");

    // a) mysql downloads字段加1
    sprintf(sql_cmd, "SELECT downloads FROM share_file_list WHERE md5 = '%s' AND file_name = '%s'", md5, fileName);
    ret2 = process_result_one(conn_mysql, sql_cmd, tmp);
    if (0 != ret2) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
        ret = -1;
        goto END;
    }

    downloads = atoi(tmp) + 1;
    // 更新此文件的downloads字段
    sprintf(sql_cmd, "UPDATE share_file_list SET downloads = %d WHERE md5 = '%s' AND file_name = '%s'", downloads, md5, fileName);
    if (mysql_query(conn_mysql, sql_cmd) != 0) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
        ret = -1;
        goto END;
    }

    // 判断此文件是否在redis的集合中
    ret2 = rop_zset_exist(conn_redis, FILE_PUBLIC_ZSET, fileId);
    if (1 == ret2) {
        // 若存在, redis中的有序集合score 加1
        ret = rop_zset_increment(conn_redis, FILE_PUBLIC_ZSET, fileId);
        if (0 != ret) {
            TLOG_INFO("rop_zset_increment() failed");
        }
    } else if (0 == ret2) {
        // 若不存在，从mysql中导入数据, 并且redis中增加一个元素
        rop_zset_add(conn_redis, FILE_PUBLIC_ZSET, downloads, fileId);
        rop_hash_set(conn_redis, FILE_NAME_HASH, fileId, fileName);
    } else {
        ret = -1;
        goto END;
    }

END:
    /**
     * 下载文件downloads字段处理
     *   成功：{"code":"016"}
     *   失败：{"code":"017"}
     */

    out = NULL;
    if (0 == ret) {
        out = return_status("016");
    } else {
        out = return_status("017");
    }

    if (NULL != out) {
        printf("%s", out);
        free(out);
		out = NULL;
    }

    if (NULL != conn_redis) {
        rop_disconnect(conn_redis);
		conn_redis = NULL;
    }

    if (NULL != conn_mysql) {
        mysql_close(conn_mysql);
		conn_mysql = NULL;
    }
    return ret;
}

int cancel_share_file(char* account, char* md5, char* fileName) {
    int ret = 0;
    int ret2 = 0;
    int count = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    MYSQL* conn_mysql = NULL;
    redisContext* conn_redis = NULL;
    char* out = NULL;
    char fileId[1024] = {0};
    char tmp[512] = {0};

    conn_redis = rop_connectdb(redis_ip, redis_port, redis_password);
    if (NULL == conn_redis) {
        TLOG_INFO("redis connected failed");
        ret = -1;
        goto END;
    }

    conn_mysql = mysql_conn(mysql_user, mysql_password, mysql_db);
    if (NULL == conn_mysql) {
        TLOG_INFO("mysql connected failed");
        ret = -1;
        goto END;
    }

    mysql_query(conn_mysql, "SET names utf8mb4");

    // 文件标识
    sprintf(fileId, "%s%s", md5, fileName);

    // mysql记录操作
    snprintf(sql_cmd, SQL_MAX_LEN, "UPDATE account_file_list SET shared_status = 0 WHERE account = '%s' AND md5 = '%s' AND file_name = '%s'", account, md5, fileName);
    if (mysql_query(conn_mysql, sql_cmd) != 0) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
        ret = -1;
        goto END;
    }

    // 查询共享文件数量
    snprintf(sql_cmd, SQL_MAX_LEN, "SELECT count FROM account_file_count WHERE account = '%s'", "xxx_share_xxx_file_xxx_list_xxx_count_xxx");
    ret2 = process_result_one(conn_mysql, sql_cmd, tmp);
    if (0 != ret2) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
        ret = -1;
        goto END;
    }

    // 更新用户文件数量count字段
    count = atoi(tmp);
    if (1 == count) {
        snprintf(sql_cmd, SQL_MAX_LEN, "DELETE FROM account_file_count WHERE account = '%s'", "xxx_share_xxx_file_xxx_list_xxx_count_xxx");
    } else {
        snprintf(sql_cmd, SQL_MAX_LEN, "UPDATE account_file_count SET count = %d WHERE account = '%s'", count - 1, "xxx_share_xxx_file_xxx_list_xxx_count_xxx");
    }

    if (mysql_query(conn_mysql, sql_cmd) != 0) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
        ret = -1;
        goto END;
    }

    // 删除在共享列表的数据
    snprintf(sql_cmd, SQL_MAX_LEN, "DELETE FROM share_file_list WHERE account = '%s' AND md5 = '%s' AND file_name = '%s'", account, md5, fileName);
    if (mysql_query(conn_mysql, sql_cmd) != 0) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
        ret = -1;
        goto END;
    }

    // 删除有序集合中的指定成员
    ret = rop_zset_zrem(conn_redis, FILE_PUBLIC_ZSET, fileId);
    if (0 != ret) {
        TLOG_INFO("rop_zset_zrem() Command execution failed");
        goto END;
    }

    // 从hash表中删除对应记录
    ret = rop_hash_del(conn_redis, FILE_NAME_HASH, fileId);
    if (0 != ret) {
        TLOG_INFO("rop_hash_del() Command execution failed");
        goto END;
    }

END:
    /**
     * 取消分享：
     * 成功：{"code":"018"}
     * 失败：{"code":"019"}
     */

    if (0 == ret) {
        out = return_status("018");
    } else {
        out = return_status("019");
    }

    if (NULL != out) {
        printf("%s", out);
        free(out);
        out = NULL;
    }

    if (NULL != conn_mysql) {
        mysql_close(conn_mysql);
        conn_mysql = NULL;
    }

    if (NULL != conn_redis) {
        rop_disconnect(conn_redis);
        conn_redis = NULL;
    }

    return ret;
}

int save_file(char* account, char* md5, char* fileName) {
    int ret = 0;
    int ret2 = 0;
    int count = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    MYSQL* conn = NULL;
    char* out = NULL;
    char tmp[512] = {0};
    struct timeval tv;
    struct tm* ptm = NULL;
    char time_str[128] = {0};

    conn = mysql_conn(mysql_user, mysql_password, mysql_db);
    if (NULL == conn) {
        TLOG_INFO("mysql_conn() failed");
        ret = -1;
        goto END;
    }

    mysql_query(conn, "SET names utf8mb4");

    // 查看此用户，文件名和md5是否存在，若存在，说明此文件存在
    snprintf(sql_cmd, SQL_MAX_LEN, "SELECT * FROM account_file_list WHERE account = '%s' AND md5 = '%s' AND file_name = '%s'", account, md5, fileName);
    ret2 = process_result_one(conn, sql_cmd, NULL);
    if (2 == ret2) {
        // 已有此文件
        TLOG_INFO("%s[fileName: %s, md5: %s] 已存在", fileName, md5);
        ret = -2;
        goto END;
    }

    // 文件信息表，查找该文件的计数器(count字段)
    snprintf(sql_cmd, SQL_MAX_LEN, "SELECT count FROM file_info WHERE md5 = '%s'", md5);
    ret2 = process_result_one(conn, sql_cmd, tmp);
    if (0 != ret2) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn));
        ret = -1;
        goto END;
    }

    count = atoi(tmp);
    snprintf(sql_cmd, SQL_MAX_LEN, "UPDATE file_info SET count = %d WHERE md5 = '%s'", count + 1, md5);
    if (mysql_query(conn, sql_cmd) != 0) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn));
        ret = -1;
        goto END;
    }

    // 向account_file_list插入一条数据
    gettimeofday(&tv, NULL);
    ptm = localtime(&tv.tv_sec);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", ptm);

    /**
     * =============================================== 用户文件列表
     * account          文件所属用户
     * md5              文件md5
     * create_time      文件创建时间
     * file_name        文件名字
     * shared_status    共享状态, 0为没有共享， 1为共享
     * downloads        文件下载量，默认值为0，下载一次加1
    */

    snprintf(sql_cmd, SQL_MAX_LEN, "INSERT INTO account_file_list(account, md5, create_time, file_name, shared_status, downloads) VALUES ('%s', '%s', '%s', '%s', %d, %d)", account, md5, time_str, fileName, 0, 0);
    if (mysql_query(conn, sql_cmd) != 0) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn));
        ret = -1;
        goto END;
    }

    // 查询用户文件数量，更新该字段
    snprintf(sql_cmd, SQL_MAX_LEN, "SELECT count FROM account_file_count WHERE account = '%s'", account);
    ret2 = process_result_one(conn, sql_cmd, tmp);
    if (1 == ret2) {
        // 没有记录
        snprintf(sql_cmd, SQL_MAX_LEN, "INSERT INTO account_file_count (account, count) VALUES ('%s', %d)", account, 1);
    } else if (0 == ret2){
        count = atoi(tmp);
        snprintf(sql_cmd, SQL_MAX_LEN, "UPDATE account_file_count SET count = %d WHERE account = '%s'", count + 1, account);
    }

    if (mysql_query(conn, sql_cmd) != 0) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn));
        ret = -1;
        goto END;
    }
END:
    /**
     * 返回值：0成功，-1转存失败，-2文件已存在
     * 转存文件：
     *  成功：{"code":"020"}
     *  文件已存在：{"code":"021"}
     *  失败：{"code":"022"}
    */

    if (0 == ret) {
        out = return_status("020");
    } else if (-1 == ret) {
        out = return_status("022");
    } else if (-2 == ret) {
        out = return_status("021");
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
    char cmd[20] = {0};
    char account[ACCOUNT_LEN] = {0};
    char md5[MD5_LEN] = {0};
    char fileName[FILE_NAME_LEN] = {0};

    // 读取数据库配置信息
    read_cfg();

    // 阻塞等待用户连接
    while (FCGI_Accept() >= 0) {
        // 获取URL地址 "?" 后面的内容
        char* query = getenv("QUERY_STRING");

        // 解析命令
        query_parse_key_value(query, "cmd", cmd, NULL);
        TLOG_INFO("cmd = %s", cmd);

        char* contentLength = getenv("CONTENT_LENGTH");
        int len = 0;
        printf("Content-Type: text/html\r\n\r\n");

        if (NULL == contentLength) {
            len = 0;
        } else {
            len =  atoi(contentLength);
        }

        if (len <= 0) {
            printf("No data from standard input.<p>");
            TLOG_INFO("No data from standard input");
        } else {
            char buf[4 * 1024] = {0};
            int ret = 0;
            ret = fread(buf, 1, len, stdin);
            if (0 == ret) {
                TLOG_INFO("fread(buf, 1, len, stdin) failed");
            }

            TLOG_INFO("buf = %s", buf);
            get_json_info(buf, account, md5, fileName);
            TLOG_INFO("account = %s, md5 = %s, fileName = %s", account, md5, fileName);

            if (strcmp(cmd, "downloads") == 0) {
                downloads_file(md5, fileName);
            } else if (strcmp(cmd, "cancel") == 0) {
                cancel_share_file(account, md5, fileName);
            } else if (strcmp(cmd, "save") == 0) {
                save_file(account, md5, fileName);
            }
        }
    }
    return 0;
}
