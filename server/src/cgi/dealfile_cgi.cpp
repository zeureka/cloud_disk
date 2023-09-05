/**
 * @file dealfile_cgi.cpp
 * @brief 分享、删除文件、文件downloads字段处理CGI程序
 */

#include "../../include/cJSON.h"
#include "../../include/log.h"
#include "../../include/cfg.h"
#include "../../include/deal_mysql.h"
#include "../../include/redis_op.h"
#include "../../include/redis_keys.h"
#include "../../include/util_cgi.h"

#include <fcgi_stdio.h>
#include <fcgi_config.h>
#include <mysql/mysql.h>

// mysql 数据库配置信息
static char mysql_user[128] = {0};
static char mysql_password[128] = {0};
static char mysql_db[128] = {0};

// redis 数据库配置信息
static char redis_ip[30] = {0};
static char redis_port[10] = {0};
static char redis_password[30] = {0};

/**
 * @brief 读取数据库配置信息
 */
void read_cfg();

// 解析 JSON 包
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
int get_json_info(char* buf, char* account, char* token, char* md5, char* fileName);

/**
 * @brief 分享文件处理函数
 *
 * @param account   (in)    分享文件的操作者
 * @param md5       (in)    分享的文件的md5值
 * @param fileName  (in)    分享的文件的文件名
 *
 * @return
 *   @retval 0  success
 *   @retval -1 failed
 *   @retval -2 已有此文件的分享记录
 */
int share_file(char* account, char* md5, char* fileName);

/**
 * @brief 从 storage 删除指定的文件，参数为文件的id
 *
 * @param fileId    (in)    被删除的文件的文件ID
 *
 * @return
 *   @retval 0  success
 *   @retval -1 failed
 */
int remove_file_from_storage(char* fileId);

// 删除文件
/**
 * @brief 删除文件的处理函数
 *
 * @param account   (in)    被删除的文件的拥有着
 * @param md5       (in)    被删除的文件的md5值
 * @param fileName  (in)    被删除的文件的文件名
 *
 * @return
 *   @retval 0  success
 *   @retval -1 failed
 */
int delete_file(char* account, char* md5, char* fileName);

// 文件下载标志处理
/**
 * @brief 文件下载字段(downloads)处理函数
 *
 * @param account   (in)    被操作的文件的拥有着
 * @param md5       (in)    被操作的文件的md5值
 * @param fileName  (in)    被操作的文件的文件名
 *
 * @return
 *   @retval 0  success
 *   @retval -1 failed
 */
int downloads_file(char* account, char* md5, char* fileName);


// =========================================================================>


void read_cfg() {
    get_mysql_info(mysql_user, mysql_password, mysql_db);
    // get_cfg_value(CFG_PATH, "mysql", "user", mysql_user);
    // get_cfg_value(CFG_PATH, "mysql", "password", mysql_password);
    // get_cfg_value(CFG_PATH, "mysql", "database", mysql_db);
    TLOG_INFO("mysql:[user=%s, password=%s, database=%s]", mysql_user, mysql_password, mysql_db);

    // get_cfg_value(CFG_PATH, "redis", "ip", redis_ip);
    // get_cfg_value(CFG_PATH, "redis", "port", redis_port);
    get_redis_info(redis_ip, redis_port, redis_password);
    TLOG_INFO("redis:[ip=%s, port=%s, password=%s]", redis_ip, redis_port, redis_password);
}


int get_json_info(char* buf, char* account, char* token, char* md5, char* fileName) {
    /**
     * JSON 结构如下
     * {
     *      "account": "mio",
     *      "token": "xxxxx",
     *      "md5": "xxxxxxx",
     *      "filename": "xx",
     * }
    **/

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

    cJSON* token_child = cJSON_GetObjectItem(root, "token");
    if (NULL == token_child) {
        TLOG_INFO("cJSON_GetObjectItem() failed");
        cJSON_Delete(root);
        root = NULL;
        return -1;
    }
    strcpy(token, token_child->valuestring);

    cJSON* md5_child = cJSON_GetObjectItem(root, "md5");
    if (NULL == md5_child) {
        TLOG_INFO("cJSON_GetObjectItem() failed");
        cJSON_Delete(root);
        root = NULL;
        return -1;
    }
    strcpy(md5, md5_child->valuestring);

    cJSON* fileName_child = cJSON_GetObjectItem(root, "filename");
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


int share_file(char* account, char* md5, char* fileName) {
    /**
     *  a) 先判断此文件是否已经由分享记录，判断集合有没有这个文件，如果有，说明别人已经分享此文件，中断此操作（redis操作）
     *  b) 如果集合没有此文件的分享记录，可能因为redis没有记录，再从mysql中查询，如果mysql也没有，说明真没有（mysql操作）
     *  c) 如果mysql中有记录，而redis中没有，说明mysql和redis没有同步信息，redis先保存此文件信息，然后中断操作（redis操作）
     *  d) 如果此文件没有被分享，mysql保存一份持久化操作（mysql操作）
     *  e) redis集合中增加一个元素（redis操作）
     *  f) redis对应俄hash也需要改变（redis操作）
    **/

    int ret = 0;
    int ret2 = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    MYSQL* conn_mysql = NULL;
    redisContext* conn_redis = NULL;
    char* out = NULL;
    char tmp[512] = {0};
    char fileId[1024] = {0};

    time_t now;
    char create_time[TIME_STRING_LEN] = {0};

    // 连接redis数据库
    conn_redis = rop_connectdb(redis_ip, redis_port, redis_password);
    if (NULL == conn_redis) {
        TLOG_INFO("redis connected failed");
        ret = -1;
        goto END;
    }

    // 连接mysql数据库
    conn_mysql = mysql_conn(mysql_user, mysql_password, mysql_db);
    if (NULL == conn_mysql) {
        TLOG_INFO("mysql connected failed");
        ret = -1;
        goto END;
    }
	TLOG_INFO("database connect success");

    mysql_query(conn_mysql, "SET NAMES utf8mb4");

    // 文件标识，md5 + 文件名
    sprintf(fileId, "%s%s", md5, fileName);

    // a) 先判断此文件是否已经由分享记录，判断集合有没有这个文件，如果有，说明别人已经分享此文件，中断此操作（redis操作）
    ret2 = rop_zset_exist(conn_redis, FILE_PUBLIC_ZSET, fileId);
	TLOG_INFO("rop_zset_exist() return: %d", ret2);
    if (1 == ret2) {
        TLOG_INFO("该文件已有分享记录");
        ret = -2;
        goto END;
    } else if (0 == ret) {
        // b) 如果集合没有此文件的分享记录，可能因为redis没有记录，再从mysql中查询，如果mysql也没有，说明真没有（mysql操作）
        sprintf(sql_cmd, "SELECT * FROM share_file_list WHERE md5 = '%s' and file_name = '%s'", md5, fileName);
		TLOG_INFO("start exec sql_cmd[ %s ]", sql_cmd);
        ret2 = process_result_one(conn_mysql, sql_cmd, NULL);
        if (2 == ret2) {
            // c) 如果mysql中有记录，而redis中没有，说明mysql和redis没有同步信息，redis先保存此文件信息，然后中断操作（redis操作）
            // redis 保存此文件信息
			TLOG_INFO("start exec rop_zset_add");
            rop_zset_add(conn_redis, FILE_PUBLIC_ZSET, 0, fileId);
            TLOG_INFO("该文件已有分享记录");
            ret = -2;
            goto END;
        }
		TLOG_INFO("该文件没有分享记录");
    } else {
        ret = -1;
        goto END;
    }

    // d) 如果此文件没有被分享，mysql保存一份持久化操作（mysql操作）

    // 更新此文件的共享标志字段
    sprintf(sql_cmd, "UPDATE account_file_list SET shared_status = 1 WHERE account = '%s' AND md5 = '%s' AND file_name = '%s'", account, md5, fileName);
	TLOG_INFO("start exec sql_cmd[ %s ]", sql_cmd);
    if (mysql_query(conn_mysql, sql_cmd) != 0) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
        ret = -1;
        goto END;
    }
	TLOG_INFO("file shared_status field update success!!!");

    // 分享文件的信息，额外保存在share_file_list列表中
    /**
     *  -- account
     *  -- md5
     *  -- create_time
     *  -- file_name
     *  -- downloads
    **/

	now = time(NULL);
	strftime(create_time, TIME_STRING_LEN - 1, "%Y-%m-%d %H:%M:%S", localtime(&now));
    sprintf(sql_cmd, "INSERT INTO share_file_list (account, md5, create_time, file_name, downloads) VALUES ('%s', '%s', '%s', '%s', %d)", account, md5, create_time, fileName, 0);
	TLOG_INFO("start exec sql_cmd[ %s ]", sql_cmd);
    if (mysql_query(conn_mysql, sql_cmd) != 0) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
        ret = -1;
        goto END;
    }

    // 查询共享文件数量
    sprintf(sql_cmd, "SELECT count FROM account_file_count WHERE account = '%s'", "xxx_share_xxx_file_xxx_list_xxx_count_xxx");

	TLOG_INFO("start exec sql_cmd[ %s ]", sql_cmd);
    ret2 = process_result_one(conn_mysql, sql_cmd, tmp);
    if (1 == ret2) {
        // 没有记录
        sprintf(sql_cmd, "INSERT INTO account_file_count (account, count) VALUES ('%s', %d)", "xxx_share_xxx_file_xxx_list_xxx_count_xxx", 1);
    } else if (0 == ret2) {
        // 更新用户文件数量count字段
        sprintf(sql_cmd, "UPDATE account_file_count SET count = %d WHERE account = '%s'", atoi(tmp) + 1, "xxx_share_xxx_file_xxx_list_xxx_count_xxx");
    }

	TLOG_INFO("start exec sql_cmd[ %s ]", sql_cmd);
    if (mysql_query(conn_mysql, sql_cmd) != 0) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
        ret = -1;
        goto END;
    }

    // e) redis集合中增加一个元素（redis操作）
    rop_zset_add(conn_redis, FILE_PUBLIC_ZSET, 0, fileId);

    // f) redis对应的 hash 也需要改变（redis操作）
    rop_hash_set(conn_redis, FILE_NAME_HASH, fileId, fileName);

END:
    /**
     *  分享文件：
     *  成功：{"code":"010"}
     *  失败：{"code":"011"}
     *  别人已经分享此文件：{"code", "012"}
    **/

    out = NULL;
    if (0 == ret) {
        out = return_status("010");
    } else if (-1 == ret) {
        out = return_status("011");
    } else if (-2 == ret) {
        out = return_status("012");
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


int remove_file_from_storage(char* fileId) {
    int ret = 0;
    // 读取fdfs client 配置文件的路径
    char fdfs_cli_conf_path[256] = {0};
    get_cfg_value(CFG_PATH, "dfs_path", "client", fdfs_cli_conf_path);

    char cmd[2 * 1024] = {0};
    // sprintf(cmd, "fdfs_delete_file %s %s", fdfs_cli_conf_path, fileId);
    sprintf(cmd, "docker exec tracker fdfs_delete_file %s %s", fdfs_cli_conf_path, fileId);
    ret = system(cmd);
    TLOG_INFO("remove_file_from_storage ret = %d", ret);

    return ret;
}


int delete_file(char* account, char* md5, char* fileName) {
    /**
     * a)先判断此文件是否已经分享
     * b)判断集合有没有这个文件，如果有，说明别人已经分享此文件(redis操作)
     * c)如果集合没有此元素，可能因为redis中没有记录，再从mysql中查询，如果mysql也没有，说明真没有(mysql操作)
     * d)如果mysql有记录，而redis没有记录，那么分享文件处理只需要处理mysql (mysql操作)
     * e)如果redis有记录，mysql和redis都需要处理，删除相关记录
    */

    int ret = 0;
    int ret2 = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    MYSQL* conn_mysql = NULL;
    redisContext* conn_redis = NULL;
    char* out = NULL;
    char tmp[512] = {0};
    char fileId[1024] = {0};
    int count = 0;
    int share = 0;  // 共享状态
    int flag = 0;   // 标志redis是否有记录

    // 连接redis数据库
    conn_redis = rop_connectdb(redis_ip, redis_port, redis_password);
    if (NULL == conn_redis) {
        TLOG_INFO("redis connected failed");
        ret = -1;
        goto END;
    }

    // 连接mysql数据库
    conn_mysql = mysql_conn(mysql_user, mysql_password, mysql_db);
    if (NULL == conn_mysql) {
        TLOG_INFO("mysql connected failed");
        ret = -1;
        goto END;
    }

    mysql_query(conn_mysql, "SET NAMES utf8mb4");

    // 文件标识，md5 + 文件名
    sprintf(fileId, "%s%s", md5, fileName);

    // a) 先判断此文件是否已经分享, 判断集合有没有这个文件，如果有，说明别人已经分享此文件(redis操作)
    ret2 = rop_zset_exist(conn_redis, FILE_PUBLIC_ZSET, fileId);
    if (1 == ret2) {
        // 该文件存在
        share = 1;  // 表示此文件有共享记录
        flag = 1;   // redis 有记录
    } else if (0 == ret2) {
        // b) 如果集合没有此元素，可能因为redis中没有记录，再从mysql中查询，如果mysql也没有，说明真没有(mysql操作)
        sprintf(sql_cmd, "SELECT shared_status FROM account_file_list WHERE account = '%s' AND md5 = '%s' AND file_name = '%s'", account, md5, fileName);
        ret2 = process_result_one(conn_mysql, sql_cmd, tmp);
        if (0 == ret2) {
            // shared_status 字段
            share = atoi(tmp);
        } else if (-1 == ret2) {
            // 失败
            TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
            ret = -1;
            goto END;
        }
    } else {
        // 出错
        ret = -1;
        goto END;
    }

    if (1 == share) {
        // c) 如果mysql有记录，而redis没有记录，那么分享文件处理只需要处理mysql (mysql操作)
        sprintf(sql_cmd, "DELETE FROM share_file_list WHERE account = '%s' AND md5 = '%s' AND file_name = '%s'", account, md5, fileName);
        if (mysql_query(conn_mysql, sql_cmd) != 0) {
            TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
            ret = -1;
            goto END;
        }

        // 共享文件数量减1
        // 查询共享文件数量
        sprintf(sql_cmd, "SELECT count FROM account_file_count WHERE account = '%s'", "xxx_share_xxx_file_xxx_list_xxx_count_xxx");
        ret2 = process_result_one(conn_mysql, sql_cmd, tmp);
        if (0 != ret2) {
            TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
            ret = -1;
            goto END;
        }

        count = atoi(tmp);
        sprintf(sql_cmd, "UPDATE account_file_count SET count = %d WHERE account = '%s'", count - 1, "xxx_share_xxx_file_xxx_list_xxx_count_xxx");
        if (mysql_query(conn_mysql, sql_cmd) != 0) {
            TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
            ret = -1;
            goto END;
        }

        // * e)如果redis有记录，mysql和redis都需要处理，删除相关记录
        if (1 == flag) {
            // 有序集合删除指定成员
            rop_zset_zrem(conn_redis, FILE_PUBLIC_ZSET, fileId);

            // 从hash移除相应记录
            rop_hash_del(conn_redis, FILE_NAME_HASH, fileId);
        }
    }

    // 用户文件数量-1
    // 查询用户文件数量
    sprintf(sql_cmd, "SELECT count FROM account_file_count WHERE account = '%s'", account);
    ret2 = process_result_one(conn_mysql, sql_cmd, tmp);
    if (0 != ret2) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
        ret = -1;
        goto END;
    }

    count = atoi(tmp);
    if (1 == count) {
        sprintf(sql_cmd, "DELETE FROM account_file_count WHERE account = '%s'", account);
    } else {
        sprintf(sql_cmd, "UPDATE account_file_count SET count = %d WHERE account = '%s'", count - 1, account);
    }

    if (mysql_query(conn_mysql, sql_cmd) != 0) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
        ret = -1;
        goto END;
    }

    // 删除用户文件列表数据
    sprintf(sql_cmd, "DELETE FROM account_file_list WHERE account = '%s' AND md5 = '%s' AND file_name = '%s'", account, md5, fileName);
    if (mysql_query(conn_mysql, sql_cmd) != 0) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
        ret = -1;
        goto END;
    }

    // 文件信息表的文件引用计数count减1
    sprintf(sql_cmd, "SELECT count FROM file_info WHERE md5 = '%s'", md5);
    ret2 = process_result_one(conn_mysql, sql_cmd, tmp);
    if (0 != ret2) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
        ret = -1;
        goto END;
    }

    count = atoi(tmp) - 1;
    sprintf(sql_cmd, "UPDATE file_info SET count = %d WHERE md5 = '%s'", count, md5);
    if (0 != mysql_query(conn_mysql, sql_cmd)) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
        ret = -1;
        goto END;
    }

    if (0 == count) {
        // 说明没有用户引用此文件，需要在storage删除此文件
        sprintf(sql_cmd, "SELECT file_id FROM file_info WHERE md5 = '%s'", md5);
        ret2 = process_result_one(conn_mysql, sql_cmd, tmp);
        if (0 != ret2) {
            TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
            ret = -1;
            goto END;
        }

        sprintf(sql_cmd, "DELETE FROM file_info WHERE md5 = '%s'", md5);
        if (mysql_query(conn_mysql, sql_cmd) != 0) {
            TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
            ret = -1;
            goto END;
        }

        ret2 = remove_file_from_storage(tmp);
        if (0 != ret2) {
            TLOG_INFO("remove_file_from_storage() failed");
            ret = -1;
            goto END;
        }
    }


END:

    /**
     *  删除文件：
     *  成功：{"code":"013"}
     *  失败：{"code":"014"}
    */

    out = NULL;
    if (0 == ret) {
        out = return_status("013");
    } else {
        out = return_status("014");
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


int downloads_file(char* account, char* md5, char* fileName) {
    int ret = 0;
    int ret2 = 0;
    int downloads = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    MYSQL* conn_mysql = NULL;
    char* out = NULL;
    char tmp[512] = {0};

    conn_mysql = mysql_conn(mysql_user, mysql_password, mysql_db);
    if (NULL == conn_mysql) {
        TLOG_INFO("mysql_conn() failed");
        ret = -1;
        goto END;
    }

    mysql_query(conn_mysql, "SET NAMES utf8mb4");

    // 查看该文件的downloads字段
    sprintf(sql_cmd, "SELECT downloads FROM account_file_list WHERE account = '%s' AND md5 = '%s' AND file_name = '%s'", account, md5, fileName);
    ret2 = process_result_one(conn_mysql, sql_cmd, tmp);
    if (0 != ret2) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
        ret = -1;
        goto END;
    }

    downloads = atoi(tmp);
    sprintf(sql_cmd, "UPDATE account_file_list SET downloads = %d WHERE account = '%s' AND md5 = '%s' AND file_name = '%s'", downloads + 1, account, md5, fileName);
    if (mysql_query(conn_mysql, sql_cmd) != 0) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn_mysql));
        ret = -1;
        goto END;
    }

END:
    /**
     *  下载文件pv字段处理
     *  成功：{"code":"016"}
     *  失败：{"code":"017"}
    **/

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

    if (NULL != conn_mysql) {
        mysql_close(conn_mysql);
		conn_mysql = NULL;
    }
    return ret;
}


int main() {
    char cmd[20] = {0};
    char account[ACCOUNT_LEN] = {0};
    char token[TOKEN_LEN] = {0};
    char md5[TOKEN_LEN] = {0};
    char fileName[FILE_NAME_LEN] = {0};

    read_cfg();

    while (FCGI_Accept() >= 0) {
        char* query = getenv("QUERY_STRING");

        // 解析命令
        query_parse_key_value(query, "cmd", cmd, NULL);
        TLOG_INFO("cmd = %s", cmd);

        char* contentLength = getenv("CONTENT_LENGTH");
        int len = 0;

		printf("Content-type: text/html\r\n\r\n");

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
            // 从标准输入(web服务器)读取内容
            ret = fread(buf, 1, len, stdin);
            if (0 == ret) {
                TLOG_INFO("fread(buf, 1, len, stdin) failed");
                continue;
            }

            TLOG_INFO("buf = %s", buf);
            get_json_info(buf, account, token, md5, fileName);
            TLOG_INFO("account = %s, token = %s, md5 = %s, fileName = %s", account, token, md5, fileName);

            // 验证登录的token
            ret = verify_token(account, token);
            if (0 != ret) {
                char* out = return_status("111");
                if (NULL != out) {
                    printf("%s", out);
                    free(out);
					out = NULL;
                }
                continue;
            }

            if (strcmp(cmd, "share") == 0) {
                // 分享文件
				TLOG_INFO("start deal share file");
                share_file(account, md5, fileName);
				TLOG_INFO("end deal share file");
            } else if(strcmp(cmd, "delete") == 0) {
                // 删除文件
                delete_file(account, md5, fileName);
            } else if (strcmp(cmd, "downloads") == 0) {
                // 文件下载标志处理
                downloads_file(account, md5, fileName);
            }
        }
    }
    return 0;
}

