/**
 * @file upload_cgi.cpp
 * @brief 上传文件后台CGI
 */

#include "../../include/log.h"
#include "../../include/cfg.h"
#include "../../include/deal_mysql.h"
#include "../../include/util_cgi.h"

#include <cstdlib>
#include <fcgi_stdio.h>
#include <fcgi_config.h>
#include <mysql/mysql.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <fastdfs/fdfs_client.h>
#include <sys/wait.h>


static char mysql_user[128l] = {0};
static char mysql_password[128] = {0};
static char mysql_db[128] = {0};


/**
 * @brief 读取配置文件
 */
void read_cfg();

/* -------------------------------------------*/
/**
 * @brief  解析上传的post数据 保存到本地临时路径
 *         同时得到文件上传者、文件名称、文件大小
 *
 * @param len       (in)    post数据的长度
 * @param account   (out)   文件上传者
 * @param fileName  (out)   文件的文件名
 * @param md5       (out)   文件的MD5码
 * @param p_size    (out)   文件大小
 *
 * @returns
 *          0 success, -1 failed
 */
/* -------------------------------------------*/
int recv_save_file(long len, char *account, char *fileName, char *md5, long *p_size);


/* -------------------------------------------*/
/**
 * @brief  上传到 FastDFS 中
 *
 * @param fileName  (in)   文件的文件名
 * @param confPath  (in)   配置文件路径
 * @param fileId    (out)  得到上传之后的文件ID路径
 *
 * @returns
 *          0 success, -1 failed
 */
/* -------------------------------------------*/
int upload_to_dstorage_1(char *fileName, char *confPath, char *fileId);


/* -------------------------------------------*/
/**
 * @brief  将一个本地文件上传到后台分布式文件系统中
 *
 * @param fileName  (in)    文件的文件名
 * @param fileId    (out)   得到上传之后的文件ID路径
 *                          group1/M00/00/00/....
 *
 * @returns
 *      0 success, -1 failed
 */
/* -------------------------------------------*/
int upload_to_dstorage(char *fileName, char *fileId);


/* -------------------------------------------*/
/**
 * @brief  封装文件存储在分布式系统中的完整 url
 *
 * @param fileId        (in)    文件分布式ID路径
 * @param fdfs_file_url (out)   文件的完整url地址
 *                              http://IP:PORT/group1/M00/00/00/....
 *
 * @returns
 *      0 success, -1 failed
 */
/* -------------------------------------------*/
int make_file_url(char *fileId, char *fdfs_file_url);


/* -------------------------------------------*/
/**
 * @brief  将文件的信息存储到 mysql 数据库中
 *
 * @param account       (in)    文件拥有者
 * @param fileName      (in)    文件的文件名
 * @param md5           (in)    文件的md5值
 * @param size          (in)    文件的大小
 * @param fileId        (in)    文件分布式ID路径
 * @param fdfs_file_url (in)    文件的完整url地址
 *
 * @returns
 *      0 success, -1 failed
 */
/* -------------------------------------------*/
int store_fileinfo_to_mysql(char *account, char *fileName, char *md5, long size, char *fileId, char *fdfs_file_url);


// =================================================================================>


void read_cfg() {
    // get_cfg_value(CFG_PATH, "mysql", "user", mysql_user);
    // get_cfg_value(CFG_PATH, "mysql", "password", mysql_password);
    // get_cfg_value(CFG_PATH, "mysql", "database", mysql_db);
    get_mysql_info(mysql_user, mysql_password, mysql_db);
    TLOG_INFO("mysql:[user=%s, password=%s, database=%s]", mysql_user, mysql_password, mysql_db);
}

int recv_save_file(long len, char *account, char *fileName, char *md5, long *p_size) {
    int ret = 0;            // 错误判断返回标识
    int fd = 0;             // 文件描述符，用来标识创建的临时文件
    char* file_buf = NULL;  // 文件内容缓冲区
    char* begin = NULL;     // 对字符串操作时的起始位置，操作过程中会修改其值，使其一直指向要操作的字符串的起始位置
    char* p = NULL;         // 操作字符串时的辅助指针
    char* q = NULL;         // 操作字符串时的辅助指针 
    char* k = NULL;         // 操作字符串时的辅助指针
    char tmp[256] = {0};    // 用来保存文件大小的数字字符串

    // 文件头部信息
    char content_text[TEMP_BUF_MAX_LEN] = {0};

    // 分界线信息
    char boundary[TEMP_BUF_MAX_LEN] = {0};

    // =========== 开辟存放文件的内存空间 ============
    file_buf = (char*)malloc(len);
    if (NULL == file_buf) {
        TLOG_INFO("file_buf malloc failed! file size is to big!!!");
        return -1;
    }

    // 从标准输入（web服务器）中读取内容
    int ret2 = fread(file_buf, 1, len, stdin);
    if (0 == ret2) {
        TLOG_INFO("fread(file_buf, 1, len, stdin) failed");
        ret = -1;
        goto END;
    }

    // =========== 处理前端发送过来的 post 数据格式 ===========
    begin = file_buf;
    p = begin;

    /*
       ------WebKitFormBoundary88asdgewtgewx\r\n
       Content-Disposition: form-data; account="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
       Content-Type: application/octet-stream\r\n
       \r\n
       真正的文件内容\r\n
       ------WebKitFormBoundary88asdgewtgewx
       */

    p = strstr(begin, "\r\n");
    if (NULL == p) {
        TLOG_INFO("wrong no  boundary!");
        ret = -1;
        goto END;
    }
    strncpy(boundary, begin, p - begin);
    boundary[p - begin] = '\0';
    // TLOG_INFO("boundary: [%s]", boundary);

    // 移动到'\r\n' 的下一个字符
    p += 2;
    len -= p - begin;


    // get content text head
    // Content-Disposition: form-data; account="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
    begin = p;
    p = strstr(begin, "\r\n");
    if (NULL == p) {
        TLOG_INFO("get content text failed, no fileName?");
        ret = -1;
        goto END;
    }
    strncpy(content_text, begin, p - begin);
    content_text[p - begin] = '\0';

    p += 2;
    len -= p - begin;

    // ==========> get account <==========
    // Content-Disposition: form-data; account="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
    //                                 ↑
    q = begin;
    q = strstr(begin, "account=");

    // Content-Disposition: form-data; account="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
    //                                          ↑
    q += strlen("account=");
    ++q;

    // Content-Disposition: form-data; account="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
    //                                              ↑
    k = strchr(q, '"');
    strncpy(account, q, k - q);
    account[k - q] = '\0';
    trim_space(account);

    // =========> get fileName <==========
    //account="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
    //                ↑
    begin = k;
    q = begin;
    q = strstr(begin, "filename=");

    // account="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
    //                           ↑
    q += strlen("filename=");
    ++q;

    // account="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
    //                                  ↑
    k = strchr(q, '"');
    strncpy(fileName, q, k - q);
    fileName[k - q] = '\0';
    trim_space(fileName);

    // =========> get md5 <==========
    // account="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
    //                                     ↑
    begin = k;
    q = begin;
    q = strstr(begin, "md5=");

    // account="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
    //                                          ↑
    q += strlen("md5=");
    ++q;

    // account="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
    //                                              ↑
    k = strchr(q, '"');
    strncpy(md5, q, k - q);
    md5[k - q] = '\0';
    trim_space(md5);

    // =========> get size <==========
    // account="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
    //                                                 ↑
    begin = k;
    q = begin;
    q = strstr(begin, "size=");

    // account="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
    //                                                      ↑
    q += strlen("size=");

    // account="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
    //                                                           ↑
    k = strstr(q, "\r\n");
    memset(tmp, 0, sizeof(tmp));
    strncpy(tmp, q, k - q);
	tmp[k - q] = '\0';

    // 将 tmp 这个字符串转换为长整形，进制采用10进制
    *p_size = strtol(tmp, NULL, 10);

    begin = p;
    p = strstr(begin, "\r\n");
    p += 4; // "\r\n\r\n"
    len -= p - begin;

    // 获取文件的真正内容

    /*
       ------WebKitFormBoundary88asdgewtgewx\r\n
       Content-Disposition: form-data; account="mike"; filename="xxx.jpg"; md5="xxxx"; size=10240\r\n
       Content-Type: application/octet-stream\r\n
       \r\n
       真正的文件内容\r\n
       ------WebKitFormBoundary88asdgewtgewx
    */
    begin = p;

    // find file's end;
    p = memstr(begin, len, boundary); // util_cgi.h
    if (NULL == p) {
        TLOG_INFO("memstr(begin, len, boundary) failed");
        ret = -1;
        goto END;
    } else {
        p -= 2; // \r\n
    }

    fd = open(fileName, O_CREAT | O_WRONLY, 0644);
    if (fd < 0) {
        TLOG_INFO("open %s failed", fileName);
        ret = -1;
        goto END;
    }

    // 将fd指定的文件大小修改为(p - begin)
    ftruncate(fd, p - begin);
    write(fd, begin, p - begin);
    close(fd);

END:
    if (NULL != file_buf) {
        free(file_buf);
		file_buf = NULL;
    }
    return ret;
}

int upload_to_dstorage_1(char *fileName, char *confPath, char *fileId) {
    char group_name[FDFS_GROUP_NAME_MAX_LEN + 1] = {0};
    ConnectionInfo* pTrackerServer = NULL;
    int result = 0;
    int store_path_index = 0;
    ConnectionInfo storageServer;

    // 初始化日志系统
    log_init();
    //设置日志级别，只记录错误的日志信息
    g_log_context.log_level = LOG_ERR;
    // 忽略SIGPIPE信号的函数。当进程试图写入另一端已关闭的管道或套接字时，会将此信号发送给该进程
    ignore_signal_pipe();

    // 加载配置文件，并初始化
    const char* conf_file = confPath;
    if ((result = fdfs_client_init(confPath)) != 0) {
        return result;
    }

    // 通过配置文件信息连接 tracker, 并得到一个可以访问的 tracker 句柄
    pTrackerServer = tracker_get_connection();
    if (NULL == pTrackerServer) {
		TLOG_INFO("tracker_get_connection() error, obtain tracker handle failed");
        fdfs_client_destroy();
        return errno != 0 ? errno : ECONNREFUSED;
    }

    *group_name = '\0';
    // 通过 tracker 句柄得到一个可以访问的 storage 句柄
    if ((result = tracker_query_storage_store(pTrackerServer, &storageServer, group_name, &store_path_index)) != 0) {
        fdfs_client_destroy();
        TLOG_INFO("tracker_query_storage_store() failed, error no: %d, error info: %s", result, STRERROR(result));
        return result;
    }

    // 通过得到的 storage 句柄，上传本地文件
    result = storage_upload_by_filename(pTrackerServer, &storageServer, store_path_index, fileName, NULL, NULL, 0, group_name, fileId);
    if (0 == result) {
        TLOG_INFO("fileId = %s", fileId);
    } else {
        TLOG_INFO("upload file failed, error no: %d, error info: %s", result, STRERROR(result));
    }

    // 断开连接，释放资源
    tracker_close_connection_ex(pTrackerServer, true);
    fdfs_client_destroy();

    return result;
}

int upload_to_dstorage(char *fileName, char *fileId) {
    int ret = 0;
    pid_t pid;
    int fd[2];

    // 创建无名管道
    if (pipe(fd) < 0) {
        TLOG_INFO("pip error");
        ret = -1;
        goto END;
    }

    // 创建进程
    pid = fork();
    if (pid < 0) {
        TLOG_INFO("fork error");
        ret = -1;
        goto END;
    }

    // 子进程
    if (0 == pid) {
        // 关闭读端
        close(fd[0]);

        // 将标准输出重定向写入管道
        dup2(fd[1], STDOUT_FILENO);

        // 读取fdfs client 配置文件的路径
        char fdfs_cli_conf_path[256] = {0};
        get_cfg_value(CFG_PATH, "dfs_path", "client", fdfs_cli_conf_path);

        // 通过 execlp 执行 fdfs_upload_file
        // execlp("fdfs_upload_file", "fdfs_upload_file", fdfs_cli_conf_path, fileName, NULL);
		char cmd[1024] = {0};
		snprintf(cmd, sizeof(cmd), "docker cp ./%s tracker:/tmp/%s && docker exec tracker fdfs_upload_file %s /tmp/%s && docker exec tracker rm /tmp/%s", fileName, fileName, fdfs_cli_conf_path, fileName, fileName);
		TLOG_INFO("execlp fdfs_upload_file, cmd = %s", cmd);
		execlp("sh", "sh", "-c", cmd, NULL);


        // 执行失败
        TLOG_INFO("execlp fdfs_upload_file failed");
        close(fd[1]);
    } else { // 父进程
        // 关闭写端
        close(fd[1]);

        // 从管道中读取数据
        read(fd[0], fileId, TEMP_BUF_MAX_LEN);
		TLOG_INFO("execlp fdfs_upload_file, return fileId = %s", fileId);

        // 去掉一个字符串两边的空白字符
        trim_space(fileId);

        if (strlen(fileId) == 0) {
            TLOG_INFO("[upload FAILED]");
            ret = -1;
            goto END;
        }

        TLOG_INFO("get [%s] success", fileId);

        // 等待子进程结束，回收其资源
        wait(NULL);
        close(fd[0]);
    }

END:
    return ret;
}

int make_file_url(char *fileId, char *fdfs_file_url) {
    int ret = 0;
    char* p = NULL;
    char* q = NULL;
    char* k = NULL;

    char fdfs_file_stat_buf[TEMP_BUF_MAX_LEN] = {0};

    pid_t pid;
    int fd[2] = {0};

    // 创建无名管道
    if (pipe(fd) < 0) {
        TLOG_INFO("pip failed");
        ret = -1;
        goto END;
    }

    // 创建进程
    pid = fork();
    if (pid < 0) {
        // 进程创建失败
        TLOG_INFO("fork failed");
        ret = -1;
        goto END;
    }

    if (0 == pid) { // 子进程
        // 关闭读端
        close(fd[0]);

        // 将标准输出重定向写入管道
		dup2(fd[1], STDOUT_FILENO);

		// 读取fdfs client 配置文件路径
        char fdfs_cli_conf_path[256] = {0};
        get_cfg_value(CFG_PATH, "dfs_path", "client", fdfs_cli_conf_path);

		char cmd[1024] = {0};
		snprintf(cmd, sizeof(cmd), "docker exec tracker fdfs_file_info %s %s", fdfs_cli_conf_path, fileId);
		TLOG_INFO("execlp fdfs_file_info, cmd = %s", cmd);
		execlp("sh", "sh", "-c", cmd, NULL);

        // 执行失败
        TLOG_INFO("execlp fdfs_file_info error");
        close(fd[1]);
    } else { // 父进程
        // 关闭写端
        close(fd[1]);

        // 从管道中读取数据
        read(fd[0], fdfs_file_stat_buf, TEMP_BUF_MAX_LEN);

        // 回收子进程资源
        wait(NULL);
        close(fd[0]);

        // 拼接上传文件的完整url地址(http://IP:Port/group1/M00/00/00/3123uufds.png)
        // p = strstr(fdfs_file_stat_buf, "source ip address: ");
        // q = p + strlen("source ip address: ");
        // k = strstr(q, "\n");
        // strncpy(fdfs_file_host_name, q, k - q);
        // fdfs_file_host_name[k - q] = '\0';

        // 读取 storage_web_server 服务器的端口
		char storage_web_server_ip[HOST_NAME_LEN] = {0};      // storage 所在的服务器 IP 地址
        char storage_web_server_port[20] = {0};
        get_cfg_value(CFG_PATH, "storage_web_server", "port", storage_web_server_port);
		get_cfg_value(CFG_PATH, "storage_web_server", "ip", storage_web_server_ip);
        strcat(fdfs_file_url, "http://");
        strcat(fdfs_file_url, storage_web_server_ip);
        strcat(fdfs_file_url, ":");
        strcat(fdfs_file_url, storage_web_server_port);
        strcat(fdfs_file_url, "/");
        strcat(fdfs_file_url, fileId);

        TLOG_INFO("file url is: %s", fdfs_file_url);
    }

END:
    return ret;
}

int store_fileinfo_to_mysql(char *account, char *fileName, char *md5, long size, char *fileId, char *fdfs_file_url) {
    int ret = 0;
    MYSQL* conn = NULL;
    time_t now;
    char create_time[TIME_STRING_LEN] = {0};
    char suffix[SUFFIX_LEN] = {0};
    char sql_cmd[SQL_MAX_LEN] = {0};

    int ret2 = 0;
    char tmp[512] = {0};
    int count = 0;

    conn = mysql_conn(mysql_user, mysql_password, mysql_db);
    if (NULL == conn) {
        TLOG_INFO("mysql_conn() connect failed");
        ret = -1;
        goto END;
    }

    mysql_query(conn, "set names utf8mb4");

    // 获得文件后缀字符串，如果后缀非法，返回"NULL"
    get_file_suffix(fileName, suffix);

    /*
       -- =============================================== 文件信息表
       -- md5 文件md5
       -- file_id 文件id
       -- url 文件url
       -- size 文件大小, 以字节为单位
       -- type 文件类型： png, zip, mp4……
       -- count 文件引用计数， 默认为1， 每增加一个用户拥有此文件，此计数器+1
    */
    sprintf(sql_cmd, "INSERT INTO file_info (md5, file_id, url, size, type, count) VALUES ('%s', '%s', '%s', %ld, '%s', %d)", md5, fileId, fdfs_file_url, size, suffix, 1);
    if (mysql_query(conn, sql_cmd) != 0) {
        TLOG_INFO("%s 插入失败: %s", sql_cmd, mysql_error(conn));
        ret = -1;
        goto END;
    }
    TLOG_INFO("%s 文件信息插入成功", sql_cmd);

    // 获取当前时间
    now = time(NULL);
    strftime(create_time, TIME_STRING_LEN - 1, "%Y-%m-%d %H:%M:%S", localtime(&now));

    /*
       -- =============================================== 用户文件列表
       -- account 文件所属用户
       -- md5 文件md5
       -- create_time 文件创建时间
       -- file_name 文件名字
       -- shared_status 共享状态, 0为没有共享， 1为共享
       -- pv 文件下载量，默认值为0，下载一次加1
    */

    sprintf(sql_cmd, "INSERT INTO account_file_list (account, md5, create_time, file_name, shared_status, downloads) VALUES ('%s', '%s', '%s', '%s', %d, %d)", account, md5, create_time, fileName, 0, 0);
    if (mysql_query(conn, sql_cmd) != 0) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn));
        ret = -1;
        goto END;
    }

    // 查询用户文件数量
    sprintf(sql_cmd, "SELECT count FROM account_file_count WHERE account = '%s'", account);
    ret2 = process_result_one(conn, sql_cmd, tmp);
    if (1 == ret2) {
        // 没有记录，插入记录
        sprintf(sql_cmd, "INSERT INTO account_file_count (account, count) VALUES ('%s', %d)", account, 1);
    } else if (0 == ret2) {
        // 更新用户文件count字段
		count = atoi(tmp);
        sprintf(sql_cmd, "UPDATE account_file_count SET count = %d WHERE account = '%s'", count + 1, account);
    }

    if (mysql_query(conn, sql_cmd) != 0) {
        TLOG_INFO("%s Command execution failed: %s", sql_cmd, mysql_error(conn));
        ret = -1;
        goto END;
    }

END:
    if (NULL != conn) {
        mysql_close(conn);
    }
    return ret;
}


// ========> 将文件的相关 FastDFS 相关信息存入 MySQL 中<========
int main() {
    char fileName[FILE_NAME_LEN] = {0};
    char account[ACCOUNT_LEN] = {0};
    char md5[MD5_LEN] = {0};
    char fileId[TEMP_BUF_MAX_LEN] = {0};
    char fdfs_file_url[FILE_URL_LEN] = {0};
    long size = 0;

    // 读取数据库配置信息
    read_cfg();

    // 阻塞等待用户连接
    while (FCGI_Accept() >= 0) {
        char* contentLength = getenv("CONTENT_LENGTH");
        long len = 0;
        int ret = 0;

        printf("Content-Type: text/html\r\n\r\n");
        if (NULL != contentLength) {
            // 转换为10进制的long型整数
            len = strtol(contentLength, NULL, 10);
        } else {
            len = 0;
        }

        if (len <= 0) {
            printf("No data from standard input");
            TLOG_INFO("len = 0, No data from standard input");
            ret = -1;
        } else {
            // ============> 上传文件 <==============
            if (recv_save_file(len, account, fileName, md5, &size) < 0) {
                ret = -1;
                goto END;
            }
            TLOG_INFO("%s上传成功[%s, 大小: %ld, md5码: %s] 到本地", account, fileName, size, md5);

            // ============> 将文件存入 FastDFS, 并得到文件的fileId <==============
            if (upload_to_dstorage(fileName, fileId) < 0) {
                ret = -1;
                goto END;
            }

            // ============> 删除本地临时存放的上传文件 <==============
            unlink(fileName); // 对该文件的链接数减1, fileName: 路径+名称的一个字符串

            // ============> 得到文件所存放在 storage 上的 host_name <==============
            if (make_file_url(fileId, fdfs_file_url) < 0) {
                ret = -1;
                goto END;
            }

            // ============> 将该文件的FastDFS 相关信息存入 mysql 中 <============
            if (store_fileinfo_to_mysql(account, fileName, md5, size, fileId, fdfs_file_url) < 0) {
                ret = -1;
                goto END;
            }
END:
            memset(fileName, 0, FILE_NAME_LEN);
            memset(account, 0, ACCOUNT_LEN);
            memset(md5, 0, MD5_LEN);
            memset(fileId, 0,TEMP_BUF_MAX_LEN);
            memset(fdfs_file_url, 0, FILE_URL_LEN);

            char* out = NULL;
            // 给前端返回上传情况
            /**
             * 上传文件:
             * 成功: {"code": "008"}
             * 失败: {"code": "009"}
            **/
            if (0 == ret) {
                out = return_status("008");
            } else {
                out = return_status("009");
            }

            if (NULL != out) {
                printf("%s", out);
                free(out);
				out = NULL;
            }
        }
    } // while
    return 0;
}
