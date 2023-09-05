/**
 * @file sharefiles_cgi.cpp
 * @brief 共享文件列表展示 CGI 程序
 */

#include "../../include/cJSON.h"
#include "../../include/cfg.h"
#include "../../include/deal_mysql.h"
#include "../../include/redis_op.h"
#include "../../include/redis_keys.h"
#include "../../include/log.h"
#include "../../include/util_cgi.h"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fcgi_stdio.h>
#include <hiredis/hiredis.h>
#include <mysql/mysql.h>


static char mysql_user[128] = {0};
static char mysql_password[128] = {0};
static char mysql_db[128] = {0};

static char redis_ip[30] = {0};
static char redis_port[10] = {0};
static char redis_password[30] = {0};

/**
 * @brief 读取配置信息
 */
void read_cfg();

/**
 * @brief 获取共享文件个数
 */
void get_share_files_count();

/**
 * @brief 解析JSON包
 *
 * @param buf       (in)    带有数据信息的JSON格式字符串
 * @param p_start   (out)   文件起点
 * @param p_count   (out)   此用户的文件数量
 *
 * @return 解析过程中，是否成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int get_filesList_json_info(char* buf, int* p_start, int* p_count);

/**
 * @brief 获取共享文件列表
 *        获取用户文件信息 ip:port/sharefiles?cmd=normal
 *
 * @param start     (in) 文件起点
 * @param count     (in) 文件数量
 *
 * @return 解析过程中，是否成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int get_share_fileList(int start, int count);

/**
 * @brief 获取共享文件排序版
 *        按下载量降序 ip:port/sharefiles?cmd=downloadsdesc
 *
 * @param start     (in) 文件起点
 * @param count     (in) 文件数量
 *
 * @return 解析过程中，是否成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int get_ranking_fileList(int start, int count);


// ================================================================>


void read_cfg() {
    get_mysql_info(mysql_user, mysql_password, mysql_db);
    TLOG_INFO("mysql:[user=%s, password=%s, database=%s]", mysql_user, mysql_password, mysql_db);

    get_redis_info(redis_ip, redis_port, redis_password);
    TLOG_INFO("redis:[ip=%s, port=%s, password=%s]", redis_ip, redis_port, redis_password);
}

void get_share_files_count() {
	int ret = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    MYSQL* conn = NULL;
    long line = 0;
    char tmp[512] = {0};

    conn = mysql_conn(mysql_user, mysql_password, mysql_db);
    if (NULL == conn) {
        TLOG_INFO("mysql_conn() error");
		goto END;
    }

    mysql_query(conn, "set names utf8mb4");

    sprintf(sql_cmd, "SELECT count FROM account_file_count WHERE account = '%s'", "xxx_share_xxx_file_xxx_list_xxx_count_xxx");
    ret = process_result_one(conn, sql_cmd, tmp);
    if (0 != ret) {
        TLOG_INFO("%s Command execution failed", sql_cmd);
		goto END;
    }

    line = atol(tmp);

END:
	if (NULL != conn) {
		mysql_close(conn);
		conn = NULL;
	}

    TLOG_INFO("line = %ld", line);
    printf("%ld", line); // 给前端反馈信息
}

int get_filesList_json_info(char* buf, int* p_start, int* p_count) {
    /*{
     *      "start": 0,
     *      "count": 100
     *}
     */

    cJSON* root = cJSON_Parse(buf);
    if (NULL == root) {
        TLOG_INFO("cJSON_Parse() error");
        return -1;
    }

    cJSON* start_child = cJSON_GetObjectItem(root, "start");
    if (NULL == start_child) {
        TLOG_INFO("cJSON_GetObjectItem() error");
        cJSON_Delete(root);
		root = NULL;
        return -1;
    }
    *p_start = start_child->valueint;

    cJSON* count_child = cJSON_GetObjectItem(root, "count");
    if (NULL == count_child) {
        TLOG_INFO("cJSON_GetObjectItem() error");
        cJSON_Delete(root);
		root = NULL;
        return -1;
    }
    *p_count = count_child->valueint;

	cJSON_Delete(root);
	root = NULL;
    return 0;
}

int get_share_fileList_error_deal(MYSQL* conn, cJSON* root, MYSQL_RES* res_set, char* out, int ret) {
    if (0 == ret) {
        printf("%s", out);
    } else {
        out = return_status("015");
		printf("%s", out);
    }

    if (NULL != res_set) {
        mysql_free_result(res_set);
        res_set = NULL;
    }

    if (NULL != conn) {
        mysql_close(conn);
        conn = NULL;
    }

    if (NULL != out) {
        free(out);
        out = NULL;
    }

    if (NULL != root) {
        cJSON_Delete(root);
        root = NULL;
    }

    return ret;
}

int get_share_fileList(int start, int count) {
    int ret = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    MYSQL* conn = NULL;
    cJSON* root = NULL;
    cJSON* array = NULL;
    char* out = NULL;
    MYSQL_RES* res_set = NULL;

    conn = mysql_conn(mysql_user, mysql_password, mysql_db);
    if (NULL == conn) {
        TLOG_INFO("mysql_conn() error");
        ret = get_share_fileList_error_deal(conn, root, res_set, out, -1);
        return ret;
    }

    mysql_query(conn, "set names utf8mb4");

    sprintf(sql_cmd, "SELECT share_file_list.*, file_info.url, file_info.size, file_info.type FROM file_info, share_file_list WHERE file_info.md5 = share_file_list.md5 limit %d, %d", start, count);
    TLOG_INFO("%s 在操作", sql_cmd);

    if (mysql_query(conn, sql_cmd) != 0) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn));
        ret = get_share_fileList_error_deal(conn, root, res_set, out, -1);
        return ret;
    }

    res_set = mysql_store_result(conn);
    if (NULL == res_set) {
        TLOG_INFO("mysql_store_result() error");
        ret = get_share_fileList_error_deal(conn, root, res_set, out, -1);
        return ret;
    }

    ulong line = mysql_num_rows(res_set);
    if (0 == line) {
        TLOG_INFO("mysql_num_rows() failed");
        ret = get_share_fileList_error_deal(conn, root, res_set, out, -1);
        return ret;
    }

    MYSQL_ROW row;
    root = cJSON_CreateObject();
    array = cJSON_CreateArray();

    while ((row = mysql_fetch_row(res_set)) != NULL) {
        /*
        *{
        *   "account": "mio",
        *   "md5": "e8ea6031b779ac26c319ddf949ad9d8d",
        *   "file_name": "test.mp4",
        *   "share_status": 1,
        *   "downloads": 0,
        *   "create_time": "2020-06-21 21:35:25",
        *   "url": "http://192.168.31.109:80/group1/M00/00/00/wKgfbViy2Z2AJ-FTAaM3As-g3Z0782.mp4",
        *   "size": 27473666,
        *   "type": "mp4"
        *}
        */

        cJSON* item = cJSON_CreateObject();
		// 从下标1开始是因为表中的第一个字段是id，用来表示主键，跟文件没有关系
        int column_index = 1;
        if (NULL != row[column_index]) {
            cJSON_AddStringToObject(item, "account", row[column_index]);
        }

        ++column_index;
        if (NULL != row[column_index]) {
            cJSON_AddStringToObject(item, "md5", row[column_index]);
        }

        ++column_index;
        if (NULL != row[column_index]) {
            cJSON_AddStringToObject(item, "file_name", row[column_index]);
        }

        cJSON_AddNumberToObject(item, "share_status", 1);

        ++column_index;
        if (NULL != row[column_index]) {
            cJSON_AddNumberToObject(item, "downloads", atol(row[column_index]));
        }

        ++column_index;
        if (NULL != row[column_index]) {
            cJSON_AddStringToObject(item, "create_time", row[column_index]);
        }

        ++column_index;
        if (NULL != row[column_index]) {
            cJSON_AddStringToObject(item, "url", row[column_index]);
        }

        ++column_index;
        if (NULL != row[column_index]) {
            cJSON_AddNumberToObject(item, "size", atol(row[column_index]));
        }

        ++column_index;
        if (NULL != row[column_index]) {
            cJSON_AddStringToObject(item, "type", row[column_index]);
        }

        cJSON_AddItemToArray(array, item);
    }
    cJSON_AddItemToObject(root, "files", array);
    out = cJSON_Print(root);
    // TLOG_INFO("share files: %s", out);

    ret = get_share_fileList_error_deal(conn, root, res_set, out, 0);
    return ret;
}

int get_ranking_fileList_error_deal(MYSQL* conn_mysql, redisContext* conn_redis, RVALUES value, cJSON* root, MYSQL_RES* res_set, char* out, int ret) {
    if (0 == ret) {
        printf("%s", out);
    } else {
        out = return_status("015");
		printf("%s", out);
    }

    if (NULL != res_set) {
        mysql_free_result(res_set);
		res_set = NULL;
    }

    if (NULL != conn_mysql) {
        mysql_close(conn_mysql);
		conn_mysql = NULL;
    }

    if (NULL != conn_redis) {
        rop_disconnect(conn_redis);
		conn_redis = NULL;
    }

    if (NULL != value) {
        free(value);
		value = NULL;
    }

    if (NULL != root) {
        cJSON_Delete(root);
		root = NULL;
    }

    if (NULL != out) {
        free(out);
		out = NULL;
    }

    return ret;
}

int get_ranking_fileList(int start,  int count) {
    int ret = 0;
    int ret2 = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    MYSQL* conn_mysql = NULL;
    redisContext* conn_redis = NULL;
    MYSQL_RES* res_set = NULL;
    cJSON* root = NULL;
    cJSON* array = NULL;
    RVALUES value = NULL;
    char* out = NULL;
    char tmp[512] = {0};

    /*
     * a) mysql 共享文件数量和 redis 共享文件数量对比，判断是否相等
     * b) 如果不相等，清空 redis 数据，从 mysql 中导入数据到 redis (mysql 和 redis 交互)
     * c) 从 redis 读取数据，给前端反馈相应信息
     * */

    conn_redis = rop_connectdb(redis_ip, redis_port, redis_password);
    if (NULL == conn_redis) {
        TLOG_INFO("redis connected error");
        ret = get_ranking_fileList_error_deal(conn_mysql, conn_redis, value, root, res_set, out, -1);
        return ret;
    }

    conn_mysql = mysql_conn(mysql_user, mysql_password, mysql_db);
    if (NULL == conn_mysql) {
        TLOG_INFO("mysql connected error");
        ret = get_ranking_fileList_error_deal(conn_mysql, conn_redis, value, root, res_set, out, -1);
        return ret;
    }

    mysql_query(conn_mysql, "set names utf8mb4");
    // mysql 共享文件数量
    sprintf(sql_cmd, "SELECT count FROM account_file_count WHERE account = '%s'", "xxx_share_xxx_file_xxx_list_xxx_count_xxx");

    ret2 = process_result_one(conn_mysql, sql_cmd, tmp);
    if (0 != ret2) {
        TLOG_INFO("%s Command execution failed", sql_cmd);
        ret = get_ranking_fileList_error_deal(conn_mysql, conn_redis, value, root, res_set, out, -1);
        return ret;
    }

    int sql_num = atoi(tmp);

    // redis中该用户的共享文件数量
    int redis_num = rop_zset_zcard(conn_redis, FILE_PUBLIC_ZSET);
    if (-1 == redis_num) {
        TLOG_INFO("rop_zset_zcard() error");
        ret = get_ranking_fileList_error_deal(conn_mysql, conn_redis, value, root, res_set, out, -1);
        return ret;
    }

    TLOG_INFO("sql_num = %d, redis_num = %d", sql_num, redis_num);

    // 如果两者不相等，清空redis数据，重新从 mysql 中导入数据到 redis
    if (redis_num != sql_num) {
        // 清空 redis 有序数据
        rop_del_key(conn_redis, FILE_PUBLIC_ZSET);
        rop_del_key(conn_redis, FILE_NAME_HASH);

        strcpy(sql_cmd, "SELECT md5, file_name, downloads FROM share_file_list order by downloads desc");
        TLOG_INFO("%s 在操作", sql_cmd);

        if (mysql_query(conn_mysql, sql_cmd) != 0) {
            TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
            ret = get_ranking_fileList_error_deal(conn_mysql, conn_redis, value, root, res_set, out, -1);
            return ret;
        }

        res_set = mysql_store_result(conn_mysql);
        if (NULL == res_set) {
            TLOG_INFO("mysql_store_result() error");
            ret = get_ranking_fileList_error_deal(conn_mysql, conn_redis, value, root, res_set, out, -1);
            return ret;
        }

        // mysql_num_rows() 接受由mysql_store_result() 返回的结果结构集，并返回结构集中的行数
        ulong line = mysql_num_rows(res_set);
        if (0 == line) {
            TLOG_INFO("mysql_num_rows() failed");
            ret = get_ranking_fileList_error_deal(conn_mysql, conn_redis, value, root, res_set, out, -1);
            return ret;
        }

        MYSQL_ROW row;
        // 遍历mysql_store_result()得到的结果结构集中的每一行，并把每一行的数据放到一个行结构中
        while ((row = mysql_fetch_row(res_set)) != NULL) {
            // row[0]:md5, row[1]:filename, row[2]:downloads
            if (NULL == row[0] || NULL == row[1] || NULL == row[2]) {
                TLOG_INFO("mysql_fetch_row() error");
                ret = get_ranking_fileList_error_deal(conn_mysql, conn_redis, value, root, res_set, out, -1);
                return ret;
            }

            char fileid[1024] = {0};

            // 文件标识，md5 + 文件名
            sprintf(fileid, "%s%s", row[0], row[1]);

            // 增加有序集合成员
            rop_zset_add(conn_redis, FILE_PUBLIC_ZSET, atoi(row[2]), fileid);

            // 增加 hash 记录
            rop_hash_set(conn_redis, FILE_NAME_HASH, fileid, row[1]);
        }
    }

    // 从redis 读取数据，给前端反馈相应的信息
    // char value[count][1024]; 分配堆区空间
    value = (RVALUES)calloc(count, VALUE_ID_SIZE);
    if (NULL == value) {
        ret = get_ranking_fileList_error_deal(conn_mysql, conn_redis, value, root, res_set, out, -1);
        return ret;
    }

    int n = 0;
    int end = start + count - 1; // 资源的结束位置
    // 降序获取有序集合的元素
    ret = rop_zset_zrevrange(conn_redis, FILE_PUBLIC_ZSET, start, end, value, &n);

    if (0 != ret) {
        TLOG_INFO("rop_zset_zrevrange() failed");
        ret = get_ranking_fileList_error_deal(conn_mysql, conn_redis, value, root, res_set, out, ret);
        return ret;
    }

    root = cJSON_CreateObject();
    array = cJSON_CreateObject();

    // 遍历redis数据库中的该用户的所有分享文件
    for (int i = 0; i < n; ++i) {
        /*{
         *  "filename": "test.png",
         *  "downloads": 1
         * }
         */
        cJSON* item = cJSON_CreateObject();
        char fileName[1024] = {0};
        ret = rop_hash_get(conn_redis, FILE_NAME_HASH, value[i], fileName);
        if (0 != ret) {
            TLOG_INFO("rop_hash_get() failed");
            ret = get_ranking_fileList_error_deal(conn_mysql, conn_redis, value, root, res_set, out, -1);
            return ret;
        }
        cJSON_AddStringToObject(item, "filename", fileName);

        int score = rop_zset_get_score(conn_redis, FILE_PUBLIC_ZSET, value[i]);
        if (-1 == score) {
            TLOG_INFO("rop_zset_get_score() failed");
            ret = get_ranking_fileList_error_deal(conn_mysql, conn_redis, value, root, res_set, out, -1);
            return ret;
        }

        cJSON_AddNumberToObject(item, "downloads", score);
        cJSON_AddItemToArray(array, item);
    }

    cJSON_AddItemToObject(root, "files", array);
    out = cJSON_Print(root);
    TLOG_INFO("shareFileList: %s", out);

    ret = get_ranking_fileList_error_deal(conn_mysql, conn_redis, value, root, res_set, out, 0);
    return ret;
}

int main() {
    char cmd[20];

    // 获取数据库配置信息
    read_cfg();

    // 阻塞等待用户连接
    while (FCGI_Accept() >= 0) {
        // 获取url地址 "?" 后面的内容
        char* query = getenv("QUERY_STRING");

        // 解析命令
        query_parse_key_value(query, "cmd", cmd, NULL);
        TLOG_INFO("cmd = %s", cmd);

        printf("Content-Type: text/html\r\n\r\n");

        if (strcmp(cmd, "count") == 0){
            // 获取共享文件个数
            get_share_files_count();
        } else {
            char* contentLength = getenv("CONTENT_LENGTH");
            int len = 0;

            if (NULL == contentLength) {
                len = 0;
            } else {
                len = atoi(contentLength);
            }

            if (len <= 0) {
                printf("No data from standard input.<p>");
                TLOG_INFO("len = 0, No data from standard input");
            } else {
                char buf[4 * 1024] = {0};
                int ret = 0;
                // 从标准输入（web服务器）读取内容
                ret = fread(buf, 1, len, stdin);

                if (0 == ret) {
                    TLOG_INFO("fread(buf, 1, len, stdin) failed");
                    continue;
                }

                TLOG_INFO("buf = %s", buf);

                int start = 0; // 文件起点
                int count = 0; // 文件个数
                get_filesList_json_info(buf, &start, &count); // 获取json包中的数据
                TLOG_INFO("start = %d, count = %d", start, count);

                if (strcmp(cmd, "normal") == 0) {
                    // 获取共享文件列表
                    get_share_fileList(start, count);
                } else if (strcmp(cmd, "downloadsdesc") == 0) {
                    // 获取共享文件（排序）
                    get_ranking_fileList(start, count);
                }
            }
        }
    }
    return 0;
}
