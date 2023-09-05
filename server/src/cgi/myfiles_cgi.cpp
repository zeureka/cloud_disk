/**
 * @file myfiles_cgi.cpp
 * @brief 用户列表展示CGI程序
 */

#include <cstdlib>
#include <iostream>
#include <fcgi_stdio.h>
#include <fcgi_config.h>
#include <cstring>
#include <mysql/mysql.h>

#include "../../include/util_cgi.h"
#include "../../include/cJSON.h"
#include "../../include/cfg.h"
#include "../../include/log.h"
#include "../../include/deal_mysql.h"


// mysql数据据配置信息
static char mysql_user[128] = {0};
static char mysql_password[128] = {0};
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
 *
 * @return 解析过程中，是否成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int get_json_info(char* buf, char* account, char* token);

/**
 * @brief 返回前端情况，返回文件数量
 *
 * @param num           (in)    文件数量
 * @param token_flag    (in)    调用此函数的函数中传来的参数
 */
void return_line(long num, int token_flag);

/**
 * @brief 获取用户文件个数
 *
 * @param account   (in)    用户账号
 * @param fileNum   (out)   该用户拥有的文件数量
 */
void get_account_files_count(char* account, int fileNum);

/**
 * @brief 解析JSON包
 *
 * @param buf       (in)    带有数据信息的JSON格式字符串
 * @param account   (out)   用户账号
 * @param token     (out)   此账号对应的token
 * @param p_start   (out)   文件起点
 * @param p_count   (out)   此用户的文件数量
 *
 * @return 解析过程中，是否成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int get_fileList_json_info(char* buf, char* account, char* token, int* p_start, int* p_count);

/**
 * @brief 获取用户文件列表
 *        获取用户文件信息 172.18.46.97:8888/myfiles?cmd=normal
 *        按下载量升序 172.18.46.97:8888/myfiles?cmd=downloadsasc
 *        按下载量降序 172.18.46.97:8888/myfiles?cmd=downloadsdesc
 *
 * @param cmd       (in)    url字符串
 * @param account   (in)    用户账号
 * @param start     (out)   文件起点
 * @param count     (out)   文件数量
 */
int get_account_fileList(char* cmd, char* account, int start, int count);


// ===========================================================>


void read_cfg() {
    // 获取mysql的配置信息
    // get_cfg_value(CFG_PATH, "mysql", "user", mysql_user);
    // get_cfg_value(CFG_PATH, "mysql", "password", mysql_password);
    // get_cfg_value(CFG_PATH, "mysql", "database", mysql_db);
    get_mysql_info(mysql_user, mysql_password, mysql_db);
    TLOG_INFO("mysql:[user=%s, password=%s, database=%s]", mysql_user, mysql_password, mysql_db);
}

int get_json_info(char* buf, char* account, char* token) {
    /*JSON数据包结构
     *{
        "account": xxxx,
        "token": "sdfa9sfasfs7f9asdf09asd8f90as8f09a"
     *}
    **/

    cJSON* root = cJSON_Parse(buf);
    if (NULL == root) {
        TLOG_INFO("cJSON_Parse() error");
        return -1;
    }

    cJSON* account_child = cJSON_GetObjectItem(root, "account");
    if (NULL == account_child) {
        TLOG_INFO("cJSON_GetObjectItem() error");
        cJSON_Delete(root);
        root = NULL;
        return -1;
    }
    strcpy(account, account_child->valuestring);

    cJSON* token_chile = cJSON_GetObjectItem(root, "token");
    if (NULL == token_chile) {
        TLOG_INFO("cJSON_GetObjectItem() error");
        cJSON_Delete(root);
        root= NULL;
        return -1;
    }
    strcpy(token, token_chile->valuestring);

	cJSON_Delete(root);
	root= NULL;
    return 0;
}

void return_line(long num, int token_flag) {
    char* out = NULL;
    char* code;
    char num_buf[128] = {0};

    if (0 == token_flag) {
        code = "110"; // 成功
    } else {
        code = "111"; // 失败
    }

    sprintf(num_buf, "%ld", num);

    // 创建一个JSON对象
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "num", num_buf);
    cJSON_AddStringToObject(root, "code", code);
    out = cJSON_Print(root);
    cJSON_Delete(root);
    root = NULL;

    if (NULL != out) {
        printf("%s", out);
        free(out);
    }
}

void get_account_files_count(char* account, int ret) {
    char sql_cmd[SQL_MAX_LEN] = {0};
    MYSQL* conn = NULL;
    long line = 0;

    conn = mysql_conn(mysql_user, mysql_password, mysql_db);
    if (NULL == conn) {
        TLOG_INFO("mysql_conn() error");
        return;
    }

    mysql_query(conn, "set names utf8mb4");
    sprintf(sql_cmd, "SELECT count FROM account_file_count WHERE account='%s'", account);
    char tmp[512] = {0};
    int ret2 = process_result_one(conn, sql_cmd, tmp);

    if (0 != ret2) {
        TLOG_INFO("%s Command execution failed", sql_cmd);
        mysql_close(conn);
        return;
    }

    line = atol(tmp);
	mysql_close(conn);
    TLOG_INFO("line = %ld", line);

    return_line(line, ret);
}

int get_fileList_json_info(char* buf, char* account, char* token, int* p_start, int* p_count) {
    /*JSON数据包结构
     * {
     *      "account": "mio",
     *      "token": "xxxx",
     *      "start": xxxx,
     *      "count": xxxx
     * }
     */

    cJSON* root = cJSON_Parse(buf);
    if (NULL == root) {
        TLOG_INFO("cJSON_Parse() error");
        return -1;
    }

    cJSON* account_child = cJSON_GetObjectItem(root, "account");
    if (NULL == account_child) {
        TLOG_INFO("cJSON_GetObjectItem() error");
        cJSON_Delete(root);
        root = NULL;
        return -1;
    }
    strcpy(account, account_child->valuestring);

    cJSON* token_child = cJSON_GetObjectItem(root, "token");
    if (NULL == token_child) {
        TLOG_INFO("cJSON_GetObjectItem() error");
        cJSON_Delete(root);
        root = NULL;
        return -1;
    }
    strcpy(token, token_child->valuestring);

    // 文件起点
    cJSON* start_child = cJSON_GetObjectItem(root, "start");
    if (NULL == start_child) {
        TLOG_INFO("cJSON_GetObjectItem() error");
        cJSON_Delete(root);
        root = NULL;
        return -1;
    }
    *p_start = start_child->valueint;

    // 文件请求个数
    cJSON* count_child = cJSON_GetObjectItem(root, "count");
    if (NULL == count_child) {
        TLOG_INFO("cJSON_GetObjectItem() error");
        cJSON_Delete(root);
        root = NULL;
        return -1;
    }
    *p_count = count_child->valueint;

    return 0;
}

int get_account_fileList_error_deal(MYSQL* conn, cJSON* root, char* out, MYSQL_RES* res_set, int ret) {
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


int get_account_fileList(char* cmd, char* account, int start, int count) {
    int ret = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    MYSQL* conn = NULL;;
    cJSON* root = NULL;
    cJSON* array = NULL;
    char* out = NULL;
    MYSQL_RES* res_set = NULL;

    conn = mysql_conn(mysql_user, mysql_password, mysql_db);
    if (NULL == conn) {
        TLOG_INFO("mysql_conn() error");
        ret = get_account_fileList_error_deal(conn, root, out, res_set, -1);
        return ret;
    }

    mysql_query(conn, "set names utf8mb4");

    // 多表指定行范围查询
    // 获取用户文件信息
    if (strcmp(cmd, "normal") == 0) {
        sprintf(sql_cmd, "SELECT account_file_list.*, file_info.url, file_info.size, file_info.type FROM file_info, account_file_list WHERE account = '%s' AND file_info.md5 = account_file_list.md5 limit %d, %d", account, start, count);
    } else if (strcmp(cmd, "downloadsasc") == 0) {
        sprintf(sql_cmd, "SELECT account_file_list.*, file_info.url, file_info.size, file_info.type FROM file_info, account_file_list WHERE account = '%s' AND file_info.md5 = account_file_list.md5 order by downloads  asc limit %d, %d", account, start, count);
    } else if (strcmp(cmd, "downloadsdesc") == 0) {
        sprintf(sql_cmd, "SELECT account_file_list.*, file_info.url, file_info.size, file_info.type FROM file_info, account_file_list WHERE account = '%s' AND file_info.md5 = account_file_list.md5 order by downloads desc limit %d, %d", account, start, count);
    }

    TLOG_INFO("%s 在操作", sql_cmd);
    if (mysql_query(conn, sql_cmd) != 0) {
        TLOG_INFO("%s Command execution failed：%s", sql_cmd, mysql_error(conn));
        ret = get_account_fileList_error_deal(conn, root, out, res_set, -1);
        return ret;
    }

    // 生成结果集
	TLOG_INFO("生成结果集");
    res_set = mysql_store_result(conn);
    if (NULL == res_set) {
        TLOG_INFO("mysql_store_result() error: %s", mysql_error(conn));
        ret = get_account_fileList_error_deal(conn, root, out, res_set, -1);
        return ret;
    }

    ulong line = 0;
    // mysql_num_rows() 接受由 mysql_store_result() 返回的结果结构集，并返回结构集中的行数
    line = mysql_num_rows(res_set);
    if (0 == line) {
        TLOG_INFO("mysql_num_rows() failed: %s", mysql_error(conn));
        ret = get_account_fileList_error_deal(conn, root, out, res_set, -1);
        return ret;
    }

    // 将mysql中查询的结果设置为 JSON 格式，返回给客户端
    /*
     * {
     *      "account": "mio",
     *      "md5": "foa7sdf9ashfa8sdffas",
     *      "create_time": "YYYY-MM-dd hh:mm:ss",
     *      "file_name": "test.png",
     *      "share_status": 0,
     *      "downloads": 0,
     *      "url": "http://IP:port/group1/M00/00/00/fsffasfsf890dfasffsd.png",
     *      "size": 236167,
     *      "type": "png"
     *
     *      "path": "~/folder/folder/folder"
     * }
     */
    MYSQL_ROW row;
    root = cJSON_CreateObject();
    array = cJSON_CreateArray();

	TLOG_INFO("生成返回给前端的JSON数据");
    while ((row = mysql_fetch_row(res_set)) != NULL) {
        cJSON* item = cJSON_CreateObject();
        int column_index = 1;
        if (row[column_index] != NULL) {
            cJSON_AddStringToObject(item, "account", row[column_index]);
        }

        ++column_index;
        if (row[column_index] != NULL) {
            cJSON_AddStringToObject(item, "md5", row[column_index]);
        }

        ++column_index;
        if (row[column_index] != NULL) {
            cJSON_AddStringToObject(item, "create_time", row[column_index]);
        }

        ++column_index;
        if (row[column_index] != NULL) {
            cJSON_AddStringToObject(item, "file_name", row[column_index]);
        }

        ++column_index;
        if (row[column_index] != NULL) {
            cJSON_AddNumberToObject(item, "share_status", atoi(row[column_index]));
        }

        ++column_index;
        if (row[column_index] != NULL) {
            cJSON_AddNumberToObject(item, "downloads", atol(row[column_index]));
        }

        ++column_index;
        if (row[column_index] != NULL) {
            cJSON_AddStringToObject(item, "url", row[column_index]);
        }

        ++column_index;
        if (row[column_index] != NULL) {
            cJSON_AddNumberToObject(item, "size", atol(row[column_index]));
        }

        ++column_index;
        if (row[column_index] != NULL) {
            cJSON_AddStringToObject(item, "type", row[column_index]);
        }

        cJSON_AddItemToArray(array, item);
    }
    cJSON_AddItemToObject(root, "files", array);
    out = cJSON_Print(root);
    // TLOG_INFO("%s Files Info: %s", account, out);

    ret = get_account_fileList_error_deal(conn, root, out, res_set, 0);
    return ret;
}

int main() {
    // count: 获取用户文件个数
    // display: 获取用户文件信息，展示到前端
    char cmd[20];
    char account[ACCOUNT_LEN];
    char token[TOKEN_LEN];

    // obtain database configuration information
    read_cfg();

    // wait user connect
    while (FCGI_Accept() >= 0) {
        // 获取 URL 地址 '?' 后面的内容
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
            len = atoi(contentLength);
        }

        if (len <= 0) {
            printf("No data from standard input.<p>");
            TLOG_INFO("len = 0, No data from standard input");
        } else {
            char buf[4 * 1024] = {0};
            int ret =0;
            ret = fread(buf, 1, len, stdin);

            if (0 == ret) {
                TLOG_INFO("fread(buf, 1, len, stdin) error");
                continue;
            }

            TLOG_INFO("buf = %s", buf);

            // 获取用户文件个数
            if (strcmp(cmd, "count") == 0) {
                // 通过json包获取用户名，token
                get_json_info(buf, account, token);

                // 验证登录token
                ret = verify_token(account, token);

                // 从mysql中获取用户文件个数
                get_account_files_count(account, ret);
            } else { // 获取用户文件信息
                int start = 0; // 文件起点
                int count = 0; // 文件个数
                get_fileList_json_info(buf, account, token, &start, &count);
                TLOG_INFO("account=%s, token=%s, start=%d, count=%d", account, token, start, count);

                ret = verify_token(account, token);
                if (0 == ret) {
                    get_account_fileList(cmd, account, start, count);
                } else {
                    char* out = return_status("111");
                    if (NULL != out) {
                        printf("%s", out);
                        free(out);
						out = NULL;
                    }
                }
            }
        }
    }
    return 0;
}
