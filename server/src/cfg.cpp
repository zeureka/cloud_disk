#include "../include/cfg.h"
#include "../include/cJSON.h"
#include "../include/log.h"
#include <cstdlib>
#include <stdio.h>

void destroy_1(cJSON* root, FILE* fp, char* buf) {
    if (NULL != root) {
        cJSON_Delete(root);
        root = NULL;
    }

    if (NULL != fp) {
        fclose(fp);
        fp = NULL;
    }

    if (NULL != buf) {
        free(buf);
        buf = NULL;
    }
}

// 从配置文件中得到响应的数据
extern int get_cfg_value(const char* path, char* title, char* key, char* value) {
    char* buf = NULL;
    FILE* fp = NULL;

    // 异常处理
    if (NULL == path || NULL == title || NULL == key) {
        return -1;
    }

    // 以只读方式打开一个必须存在的二进制文件
    fp = fopen(path, "rb");
    if (NULL == fp) {
        perror("fopen");
        TLOG_INFO("fopen error");
        return -1;
    }

    // 在文件fp中，光标相对于SEEK_END(末尾)偏移0
    fseek(fp, 0, SEEK_END);
    // 获取文件大小, ftell()函数返回FILE指针当前位置相对于文件首的偏移字节数
    long size = ftell(fp);
    // 光标移动到开头
    fseek(fp, 0, SEEK_SET);

    buf = (char*)calloc(1, size + 1);
    if (NULL == buf) {
        perror("calloc");
        TLOG_INFO("calloc error");
        fclose(fp);
        fp = NULL;
        return -1;
    }

    // 读取文件内容
    fread(buf, 1, size, fp);
    cJSON* root = cJSON_Parse(buf);
    if (NULL == root) {
        TLOG_INFO("cJSON_Parse error");
        destroy_1(root, fp, buf);
        return -1;
    }

    cJSON* father = cJSON_GetObjectItem(root, title);
    if (NULL == father) {
        TLOG_INFO("cJSON_GetObjectItem error");
        destroy_1(root, fp, buf);
        return -1;
    }

    cJSON* son = cJSON_GetObjectItem(father, key);
    if (NULL == son) {
        TLOG_INFO("cJSON_GetObjectItem error");
        destroy_1(root, fp, buf);
        return -1;
    }

    strcpy(value, son->valuestring);
    destroy_1(root, fp, buf);

    return 0;
}

// 获取数据库用户名，用户密码，数据库名等信息
extern int get_mysql_info(char* mysql_user, char* mysql_pwd, char* mysql_db) {
    // 调用 get_cfg_value 从配置文件中读取相应的数据
    if (-1 == get_cfg_value(CFG_PATH, "mysql", "user", mysql_user)) {
        TLOG_INFO("mysql_user error");
        return -1;
    }

    if (-1 == get_cfg_value(CFG_PATH, "mysql", "password", mysql_pwd)) {
        TLOG_INFO("mysql_pwd error");
        return -1;
    }

    if (-1 == get_cfg_value(CFG_PATH, "mysql", "database", mysql_db)) {
        TLOG_INFO("mysql_db error");
        return -1;
    }

    return 0;
}

// 获取数据库IP，PORT，password等信息
extern int get_redis_info(char* redis_ip, char* redis_port, char* redis_password) {
    if (-1 == get_cfg_value(CFG_PATH, "redis", "ip", redis_ip)) {
	TLOG_INFO("redis_ip error");
	return -1;
    }

    if (-1 == get_cfg_value(CFG_PATH, "redis", "port", redis_port)) {
	TLOG_INFO("redis_ip error");
	return -1;
    }

    if (-1 == get_cfg_value(CFG_PATH, "redis", "password", redis_password)) {
	TLOG_INFO("redis_ip error");
	return -1;
    }

    return 0;
}
