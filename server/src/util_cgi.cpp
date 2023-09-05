#include "../include/util_cgi.h"


// 去掉一个字符串两边的空白字符
// 成功：0, 失败：-1
int trim_space(char* inbuf) {
    int begin = 0;
    int end = strlen(inbuf) - 1;
    char* str = inbuf;

    if (NULL == str) {
        return -1;
    }

    while (isspace(str[begin]) && str[begin] != '\0') {
        ++begin;
    }

    while (isspace(str[end]) && end > begin) {
        --end;
    }

    strncpy(inbuf, str + begin, end - begin + 1);
    inbuf[end - begin + 1] = '\0';

    return 0;
}

// 在字符串 full_data 中查找字符串 substr 第一次出现的位置
// 成功：匹配字符串首地址, 失败：NULL
char* memstr(char* full_data, int full_data_len, char* substr) {
    if (NULL == full_data || NULL == substr || full_data_len <= 0) {
        return NULL;
    }

    if ('\0' == *substr) {
        return 0;
    }

    int sublen = strlen(substr);
    char* cur = full_data;
    int last_possible = full_data_len - sublen + 1;

    for (int i = 0; i < last_possible; ++i) {
        if (*cur == *substr) {
            if (memcmp(cur, substr, sublen) == 0) {
                return cur;
            }
        }

        ++cur;
    }

    return NULL;
}

// 解析 url query 类似 abc=123&bbb=456 字符串
// 传入一个key，得到相应的value
// 成功：0, 失败：-1
int query_parse_key_value(char* url, const char* key, char* value, int* value_len_p) {
    char* tmp = NULL;
    char* end = NULL;

    // 判断url中是否有key
    tmp = strstr(url, key);
    if (NULL == tmp) {
        return -1;
    }

    tmp += strlen(key);
    ++tmp; // tmp指向key对应的value的第一个字符
    end = tmp;

    while('\0' != *end && '#' != *end && '&' != *end) {
        ++end;
    }

    strncpy(value, tmp, end - tmp);
    value[end - tmp] = '\0';

    if (NULL != value_len_p) {
        *value_len_p = end - tmp;
    }

    return 0;
}

// 通过文件名 file_name, 得到文件后缀字符串，保存在 suffix 如果非法文件后缀，返回NULL
int get_file_suffix(char* file_name, char* suffix) {
    char* ret = strrchr(file_name, '.');

    if (NULL == suffix) {
        strncpy(suffix, "null", 5);
    } else {
        ++ret;
        strncpy(suffix, ret, strlen(ret));
        suffix[strlen(ret)] = '\0';
    }

    return 0;
}

// 字符串 strSrc 中字符串 strFind，替换为strReplace
int str_replace(char *strSrc, char *strFind, char *strReplace) {
    char *p, *q;
    int lenFind = strlen(strFind);
    int lenReplace = strlen(strReplace);

    while ((p = strstr(strSrc, strFind)) != NULL) {
        memmove(q = p + lenReplace, p + lenFind, strlen(p + lenFind) + 1);
        memcpy(p, strReplace, lenReplace);
        strSrc = q;
    }

    return 0;
}

// 返回前端情况，NULL代表失败，返回的指针不为空则需要free
char* return_status(char* status_num) {
    char* out = NULL;
    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "code", status_num);
    out = cJSON_Print(root);
    cJSON_Delete(root);
    return out;
}

// 验证登录token
// 成功：0, 失败：-1
int verify_token(char* account, char* token) {
    int ret = 0;
    redisContext* redis_conn = NULL;
    char tmp_token[128] = {0};

    char redis_ip[30] = {0};
    char redis_port[10] = {0};
    char redis_password[30] = {0};

    // get_cfg_value(CFG_PATH, "redis", "ip", redis_ip);
    // get_cfg_value(CFG_PATH, "redis", "port", redis_port);
    get_redis_info(redis_ip, redis_port, redis_password);

    // connect redis database
    redis_conn = rop_connectdb(redis_ip, redis_port, redis_password);
    if (NULL == redis_conn) {
        TLOG_INFO("redis connected error");
        ret = -1;
        goto END;
    }

    ret = rop_get_string(redis_conn, account, tmp_token);
    TLOG_INFO("token=%s, tmp_token=%s", token, tmp_token);
    if (0 == ret) {
        if (strcmp(token, tmp_token) != 0) {
            ret = -1;
        }
    }

END:
    if (NULL != redis_conn) {
        rop_disconnect(redis_conn);
    }
    return ret;
}
