/**
 * @file myinfo_cgi.cpp
 * @brief 用户信息展示CGI程序
 */

#include "../../include/cfg.h"
#include "../../include/util_cgi.h"
#include "../../include/log.h"
#include "../../include/cJSON.h"
#include "../../include/deal_mysql.h"

#include <cstdlib>
#include <cstring>
#include <fcgi_stdio.h>
#include <fcgi_config.h>
#include <mysql/mysql.h>
#include <stdio.h>


// mysql数据据配置信息
static char mysql_user[128] = {0};
static char mysql_password[128] = {0};
static char mysql_db[128] = {0};

/**
 * @brief 读取配置信息
 */
void read_cfg();

/**
 * @brief 解析修改密码的JSON包数据
 *
 * @param buf           (in)    带有数据信息的JSON格式字符串
 * @param account       (out)   用户账号
 * @param oldPassword   (out)   旧密码
 * @param newPassword   (out)   新密码
 *
 * @return 解析过程中，是否成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int get_change_password_json_info(char* buf, char* account, char* oldPassword, char* newPassword);

/**
 * @brief 解析编辑资料的JSON包数据
 *
 * @param buf     (in)    带有数据信息的JSON格式字符串
 * @param account (out)   用户账号
 * @param phone   (out)   新电话号码
 * @param email   (out)   新邮箱地址
 *
 * @return 解析过程中，是否成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int get_change_material_json_info(char* buf, char* account, char* phone, char* email);

/**
 * @brief 解析获取信息的JSON包数据
 *
 * @param buf     (in)    带有数据信息的JSON格式字符串
 * @param account (out)   用户账号
 *
 * @return 解析过程中，是否成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int get_json_info(char* buf, char* account);

/**
 * @brief 获取用户信息
 *
 * @param account  (in)    用户账号
 *
 * @return 解析过程中，是否成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int get_my_info(char* account);

/**
 * @brief 修改密码
 *
 * @param account       (in)    用户账号
 * @param oldPassword   (in)    旧密码
 * @param newPassword   (in)    新密码
 *
 * @return 解析过程中，是否成功
 *   @retval 0  success
 *   #retval 1  原密码错误
 *   @retval -1 failed
 */
int change_password(char* account, char* oldPassword, char* newPassword);

/**
 * @brief 编辑资料
 *
 * @param account  (in)    用户账号
 * @param phone   (out)    新电话号码
 * @param email   (out)    新邮箱地址
 *
 * @return 解析过程中，是否成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int change_material(char* account, char* phone, char* email);


void read_cfg() {
    get_mysql_info(mysql_user, mysql_password, mysql_db);
    TLOG_INFO("mysql:[user=%s, password=%s, database=%s]", mysql_user, mysql_password, mysql_db);
}

int get_change_password_json_info(char* buf, char* account, char* oldPassword, char* newPassword) {
	/**
	 * data format
	 * {
	 *  "account" : "mio",
	 *  "oldPassword" : "xxxx",
	 *  "newPassword" : "xxxx"
	 * }
	 */

	TLOG_INFO("change password buf: %s", buf);

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

    cJSON* oldPassword_child = cJSON_GetObjectItem(root, "oldPassword");
    if (NULL == oldPassword_child) {
        TLOG_INFO("cJSON_GetObjectItem() error");
        cJSON_Delete(root);
        root = NULL;
        return -1;
    }
    strcpy(oldPassword, oldPassword_child->valuestring);

    cJSON* newPassword_child = cJSON_GetObjectItem(root, "newPassword");
    if (NULL == newPassword_child) {
        TLOG_INFO("cJSON_GetObjectItem() error");
        cJSON_Delete(root);
        root = NULL;
        return -1;
    }
    strcpy(newPassword, newPassword_child->valuestring);

	TLOG_INFO("account: %s, oldPassword: %s, newPassword: %s", account, oldPassword, newPassword);

    cJSON_Delete(root);
    root = NULL;
    return 0;
}

int get_change_material_json_info(char* buf, char* account, char* phone, char* email) {
    /**
     * data format
     * {
     *  "acccount" : "mio",
     *  "phone" : "xxxx",
     *  "email" : "xxxx"
     * }
     */

	TLOG_INFO("change material buf: %s", buf);

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

    cJSON* phone_child = cJSON_GetObjectItem(root, "phone");
    if (NULL == phone_child) {
        TLOG_INFO("cJSON_GetObjectItem() error");
        cJSON_Delete(root);
        root = NULL;
        return -1;
    }
    strcpy(phone, phone_child->valuestring);

    cJSON* email_child = cJSON_GetObjectItem(root, "email");
    if (NULL == email_child) {
        TLOG_INFO("cJSON_GetObjectItem() error");
        cJSON_Delete(root);
        root = NULL;
        return -1;
    }
    strcpy(email, email_child->valuestring);

	TLOG_INFO("account: %s, phone: %s, email: %s", account, phone, email);

    cJSON_Delete(root);
    root = NULL;
    return 0;
}

int get_json_info(char* buf, char* account) {
    /**
     * data format
     * {
     *  "acccount" : "mio"
     * }
     */

	TLOG_INFO("get my json buf: %s", buf);

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

    cJSON_Delete(root);
    root = NULL;
    return 0;
}

int get_my_info(char* account) {
    int ret = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    char tmp[512] = {0};
    MYSQL_ROW row;
    MYSQL_RES* res_set = NULL;
    cJSON* root = NULL;
    char* out = NULL;
    MYSQL* conn = NULL;

    long files_number = 0;
    long share_files_number = 0;
    char phone[128] = {0};
    char email[128] = {0};

    conn = mysql_conn(mysql_user, mysql_password, mysql_db);
    if (NULL == conn) {
        TLOG_INFO("mysql_conn() error");
        ret = -1;
        goto END;
    }

    mysql_query(conn, "set names utf8mb4");
    // 设置事务提交方式, 手动提交, 针对于当前会话
    mysql_query(conn, "set @@autocommit = 0");
    // 开启事务
    mysql_query(conn, "select transaction");

	// ================ 获取用户分享文件数量 ===================
    snprintf(sql_cmd, sizeof(sql_cmd), "SELECT count(*) FROM account_file_list WHERE shared_status = 1 AND account = '%s'", account);
    ret = process_result_one(conn, sql_cmd, tmp);
    if (0 != ret) {
        TLOG_INFO("%s Operation failed: %s", sql_cmd, mysql_error(conn));
        ret = -1;
        goto END;
    }
    share_files_number = atol(tmp);
	TLOG_INFO("share files number: %ld", share_files_number);

	// ================ 获取用户文件数量 ===================
	snprintf(sql_cmd, sizeof(sql_cmd), "SELECT count FROM account_file_count WHERE account = '%s'", account);
	if (0 != mysql_query(conn, sql_cmd)) {
        TLOG_INFO("%s Operation failed: %s", sql_cmd, mysql_error(conn));
        ret = -1;
        goto END;
	}

	// 先判断表中有没有该用户的数据，即该用户有没有文件，没有文件则在 account_file_count 没有数据
	res_set = mysql_store_result(conn);
	if (0 == mysql_num_rows(res_set)) {
		TLOG_INFO("[%s]: No files");
		files_number = 0;
	} else {
		row = mysql_fetch_row(res_set);
		files_number = atol(row[0]);
		TLOG_INFO("files number: %ld", files_number);
	}
	mysql_free_result(res_set);
	res_set = NULL;

	// ================ 获取用户电话号码和邮箱地址信息 ==================
    snprintf(sql_cmd, sizeof(sql_cmd), "SELECT phone, email FROM account_info WHERE account = '%s'", account);
    if (0 != mysql_query(conn, sql_cmd)) {
        TLOG_INFO("%s Operation failed: %s", sql_cmd, mysql_error(conn));
        ret = -1;
        goto END;
    }

    res_set = mysql_store_result(conn);
    if (NULL == res_set) {
        TLOG_INFO("mysql_store_result() error");
        ret = -1;
        goto END;
    }

    row = mysql_fetch_row(res_set);
    strcpy(phone, row[0]);
    strcpy(email, row[1]);
	mysql_free_result(res_set);
	res_set = NULL;

	TLOG_INFO("[%s] phone: %s, email: %s", account, phone, email);

    root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "files_number", files_number);
    cJSON_AddNumberToObject(root, "share_files_number", share_files_number);
    cJSON_AddStringToObject(root, "phone", phone);
    cJSON_AddStringToObject(root, "email", email);

    out = cJSON_Print(root);
    TLOG_INFO("My info: %s", out);

END:
    if (0 == ret) {
        printf("%s", out);
    } else {
        if (NULL != conn) {
            mysql_query(conn, "rollback");
        }
        char* out2 = return_status("028");
        printf("%s", out2);
        free(out2);
        out2 = NULL;
    }

    if (NULL != conn) {
        mysql_query(conn, "commit");
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

int change_password(char* account, char* oldPassword, char* newPassword) {
    int ret = 0;
    int ret2 = 0;
    MYSQL* conn = NULL;
    char* out = NULL;
    char sql_cmd[SQL_MAX_LEN] = {0};

    conn = mysql_conn(mysql_user, mysql_password, mysql_db);
    if (NULL == conn) {
        TLOG_INFO("mysql connected failed");
        ret = -1;
        goto END;
    }

    mysql_query(conn, "set names utf8mb4");
    // 设置事务提交方式, 手动提交, 针对于当前会话
    mysql_query(conn, "set @@autocommit = 0");
    // 开启事务
    mysql_query(conn, "select transaction");

    snprintf(sql_cmd, sizeof(sql_cmd), "SELECT * FROM account_info WHERE account = '%s' AND password = '%s'", account, oldPassword);
    ret2 = process_result_one(conn, sql_cmd, NULL);
    if (2 != ret2) {
        TLOG_INFO("oldPassword error");
        ret = 1;
        goto END;
    }

    snprintf(sql_cmd, sizeof(sql_cmd), "UPDATE account_info SET password = '%s' WHERE account = '%s'", newPassword, account);
    if (mysql_query(conn, sql_cmd) != 0) {
        TLOG_INFO("%s operation failed: %s", sql_cmd, mysql_error(conn));
        ret = -1;
        goto END;
    }

END:
    // 0  success
    // 1  原密码错误
    // -1 failed
    if (0 == ret) {
        out = return_status("025");
    } else if (1 == ret) {
        if (NULL != conn) {
            mysql_query(conn, "rollback");
        }
        out = return_status("026");
    } else if (-1 == ret) {
        if (NULL != conn) {
            mysql_query(conn, "rollback");
        }
        out = return_status("027");
    }

    if (NULL != out) {
        printf("%s", out);
        free(out);
        out = NULL;
    }

    if (NULL != conn) {
        mysql_query(conn, "commit");
        mysql_close(conn);
        conn = NULL;
    }

    return ret;
}

int change_material(char* account, char* phone, char* email) {
    int ret = 0;
    MYSQL* conn = NULL;
    char* out = NULL;
    char sql_cmd[SQL_MAX_LEN] = {0};

    conn = mysql_conn(mysql_user, mysql_password, mysql_db);
    if (NULL == conn) {
        TLOG_INFO("mysql connected failed");
        ret = -1;
        goto END;
    }

    mysql_query(conn, "set names utf8mb4");
    // 设置事务提交方式, 手动提交, 针对于当前会话
    mysql_query(conn, "set @@autocommit = 0");
    // 开启事务
    mysql_query(conn, "select transaction");

    snprintf(sql_cmd, sizeof(sql_cmd), "UPDATE account_info SET phone = '%s', email = '%s' WHERE account = '%s'", phone, email, account);
    if (mysql_query(conn, sql_cmd) != 0) {
        TLOG_INFO("%s operation failed: %s", sql_cmd, mysql_error(conn));
        ret = -1;
        goto END;
    }

END:
    if (0 == ret) {
        out = return_status("023");
    } else if (-1 == ret) {
        if (NULL != conn) {
            mysql_query(conn, "rollback");
        }
        out = return_status("024");
    }

    if (NULL != out) {
        printf("%s", out);
        free(out);
        out = NULL;
    }

    if (NULL != conn) {
        mysql_query(conn, "commit");
        mysql_close(conn);
        conn = NULL;
    }

    return ret;
}


int main() {
    char cmd[20] = {0};
    char account[ACCOUNT_LEN] = {0};

    // 读取mysql数据库配置
    read_cfg();

	TLOG_INFO("----- obtaining mysql configuration completed -----");
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
            int ret = 0;
            ret = fread(buf, 1, len, stdin);

            if (0 == ret) {
                TLOG_INFO("fread(buf, 1, len, stdin) error");
                continue;
            }

            if (strcmp(cmd, "info") == 0) {
                // 获取用户信息
                get_json_info(buf, account);
                get_my_info(account);
            } else if (strcmp(cmd, "changematerial") == 0) {
                // 编辑资料
                char phone[128] = {0};
                char email[128] = {0};
                get_change_material_json_info(buf, account, phone, email);
                change_material(account, phone, email);
            } else {
                // 修改密码
                char oldPassword[PASSWORD_LEN] = {0};
                char newPassword[PASSWORD_LEN] = {0};
                get_change_password_json_info(buf, account, oldPassword, newPassword);
                change_password(account, oldPassword, newPassword);
            }
        }
    }
}
