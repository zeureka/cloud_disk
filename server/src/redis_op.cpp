#include "../include/redis_op.h"

// redis tcp 模式链接；成功返回链接句柄，失败返回NULL
redisContext* rop_connectdb_nopwd(char* ip_str, char* port_str) {
    redisContext* conn = NULL;
    uint16_t port = atoi(port_str);
    conn = redisConnect(ip_str, port);

    if (NULL == conn) {
        TLOG_INFO("[-][GMS_REDIS]Connect %s:%d ERROR: Can't allocate redis context!", ip_str, port);
        goto END;
    }

    if (conn->err) {
        TLOG_INFO("[-][GMS_REDIS]Connect %s:%d ERROR: %s", ip_str, port, conn->errstr);
        redisFree(conn);
        conn = NULL;
        goto END;
    }

    TLOG_INFO("[+][GMS_REDIS]Connect %s:%d SUCCESS!", ip_str, port);

END:
    return conn;
}

// redis tcp 模式链接
redisContext* rop_connectdb(char* ip_str, char* port_str, char* pwd) {
    redisContext* conn = NULL;
    uint16_t port = atoi(port_str);
    char auth_cmd[REDIS_COMMAND_SIZE];
    redisReply* reply = NULL;
    conn = redisConnect(ip_str, port);

    if (NULL == conn) {
        TLOG_INFO("[-][GMS_REDIS]Connect %s:%d Error:Can't allocate redis context!\n", ip_str, port);
        goto END;
    }

    if (conn->err) {
        TLOG_INFO("[-][GMS_REDIS]Connect %s:%d Error:%s\n", ip_str, port, conn->errstr);
        redisFree(conn);
        conn = NULL;
        goto END;
    }

    sprintf(auth_cmd, "auth %s", pwd);
    reply = (redisReply*)redisCommand(conn, auth_cmd);

    if (NULL == reply) {
        TLOG_INFO("[-][GMS_REDIS]Command : auth %s ERROR!\n", pwd);
        conn = NULL;
        goto END;
    }

    freeReplyObject(reply);


    TLOG_INFO("[+][GMS_REDIS]Connect %s:%d SUCCESS!\n", ip_str, port);

END:
    return conn;
}

// redis unix域模式链接；成功返回链接句柄，失败返回NULL
redisContext* rop_connectdb_unix(char* sock_path, char* pwd) {
    redisContext* conn = NULL;
    char auth_cmd[REDIS_COMMAND_SIZE];
    conn = redisConnectUnix(sock_path);
    redisReply* reply = NULL;

    if (NULL == conn) {
        TLOG_INFO("[-][GMS_REDIS]Connect domain-unix:%s ERROR: Can't allocate redis context!", sock_path);
        goto END;
    }

    if (conn->err) {
        TLOG_INFO("[-][GMS_REDIS]Connect domain-unix: %s ERROR: %s", sock_path, conn->errstr);
        redisFree(conn);
        conn = NULL;
        goto END;
    }

    // redisReply* reply = NULL;
    // goto 语句如果跳过了一个变量的初始化，可能会造成这个变量处于未初始化状态，
    // 可能会导致程序出现为定义行为，所以要把这个 reply 定义及初始化上移，否则会报错
    sprintf(auth_cmd, "auth %s", pwd);
    reply = (redisReply*)redisCommand(conn, auth_cmd);

    if (NULL == reply) {
        TLOG_INFO("[-][GMS_REDIS]Command : auth %s ERROR!", pwd);
        conn = NULL;
        goto END;
    }

    freeReplyObject(reply);
    TLOG_INFO("[+][GMS_REDIS]Connect domain-unix: %s SUCCESS!", sock_path);

END:
    return conn;
}

// tcp 连接 redis 超时等待；成功返回链接句柄，失败返回NULL
redisContext* rop_connectdb_timeout(char* ip_str, char* port_str, struct timeval* timeout) {
    redisContext* conn = NULL;
    uint16_t port = atoi(port_str);

    conn = redisConnectWithTimeout(ip_str, port, *timeout);
    if (NULL == conn) {
        TLOG_INFO("[-][GMS_REDIS]Connect %s:%d Error:Can't allocate redis context!", ip_str, port);
        goto END;
    }

    if (conn->err) {
        TLOG_INFO("[-][GMS_REDIS]Connect %s:%d Error:%s\n", ip_str, port, conn->errstr);	
        redisFree(conn);
        conn = NULL;
        goto END;
    }

    TLOG_INFO("[+][GMS_REDIS]Connect %s:%d SUCCESS!", ip_str, port);

END:
    return conn;
}

// 关闭指定的链接句柄
void rop_disconnect(redisContext* conn) {
    if (NULL == conn) {
        return;
    }

    redisFree(conn);
    conn = NULL;
    TLOG_INFO("[+][GMS_REDIS]Disconnect SUCCESS!\n");
}

// 选择 redis 其中的一个数据库；成功返回0，失败返回-1
int rop_selectdatabase(redisContext* conn, unsigned int db_no) {
    int ret = 0;
    redisReply* reply = NULL;
    reply = (redisReply*)redisCommand(conn, "select %d", db_no);

    if (NULL == reply) {
        fprintf(stderr, "[-][GMS_REDIS]Select database %d error!\n", db_no);
        TLOG_INFO("[-][GMS_REDIS]Select database %d error!%s", db_no, conn->errstr);
        ret = -1;
        goto END;
    }

    printf("[+][GMS_REDIS]Select database %d SUCCESS!\n", db_no);
    TLOG_INFO("[+][GMS_REDIS]Select database %d SUCCESS!", db_no);

END:
    freeReplyObject(reply);
    return ret;
}

// 清空当其数据库所有信息；成功返回0，失败返回-1
int rop_flush_database(redisContext* conn) {
    int ret = 0;
    redisReply* reply = NULL;
    reply = (redisReply*)redisCommand(conn, "FLUSHDB");

    if (NULL == reply) {
        fprintf(stderr, "[-][GMS_REDIS]Clear all data error\n");
        TLOG_INFO("[-][GMS_REDIS]Clear all data error");
        ret = -1;
        goto END;
    }

    printf("[+][GMS_REDIS]Clear all data!!\n");
    TLOG_INFO("[+][GMS_REDIS]Clear all data!!");

END:
    freeReplyObject(reply);
    return ret;
}

// 判断key值是否存在；存在返回1，不存在返回0,失败返回-1
int rop_is_key_exist(redisContext* conn, char* key) {
    int ret = 0;
    redisReply* reply = NULL;
    reply = (redisReply*)redisCommand(conn, "EXISTS %s", key);

    if (REDIS_REPLY_INTEGER != reply->type) {
        fprintf(stderr, "[-][GMS_REDIS]is key exist get wrong type!\n");
        TLOG_INFO("[-][GMS_REDIS]is key exist get wrong type! %s", conn->errstr);
        ret = -1;
        goto END;
    }

    if (1 == reply->integer) {
        ret = 1;
    } else {
        ret = 0;
    }

END:
    freeReplyObject(reply);
    return ret;
}

// 删除一个 key；成功返回0，失败返回-1
int rop_del_key(redisContext* conn, char* key) {
    int ret = 0;
    redisReply* reply = NULL;
    reply = (redisReply*)redisCommand(conn, "DEL %s", key);

    if (REDIS_REPLY_INTEGER != reply->type) {
        fprintf(stderr, "[-][GMS_REDIS] DEL key %s ERROR!\n", key);
        TLOG_INFO("[-][GMS_REDIS] DEL key %s ERROR!%s", conn->errstr);
        ret = -1;
        goto END;
    }

    if (0 < reply->integer) {
        ret = 0;
    } else {
        ret = -1;
    }

END:
    freeReplyObject(reply);
    return ret;
}

// 打印库中所有匹配 pattern 的key
void rop_show_keys(redisContext* conn, char* pattern) {
    redisReply* reply = NULL;
    reply = (redisReply*)redisCommand(conn, "keys %s", pattern);

    if (REDIS_REPLY_ARRAY != reply->type) {
        fprintf(stderr, "[-][GMS_REDIS] show all keys and data wrong type!\n");
        TLOG_INFO("[-][GMS_REDIS] show all keys and data wrong type! %s", conn->errstr);
        goto END;
    }

    for (int i = 0; i < reply->elements; ++i) {
        printf("======[%s]======\n", reply->element[i]->str);
    }

END:
    freeReplyObject(reply);
}

// 设置一个key的删除时间，系统到达一定时间将会自动删除该key；成功返回0,失败返回-1
int rop_set_key_lifecycle(redisContext* conn, char* key, time_t delete_time) {
    int ret = 0;
    redisReply* reply = NULL;
    reply = (redisReply*)redisCommand(conn, "EXPIREAT %s %d", key, delete_time);

    if (REDIS_REPLY_INTEGER != reply->type) {
        fprintf(stderr, "[-][GMS_REDIS]Set key:%s delete time ERROR!\n", key);
        TLOG_INFO("[-][GMS_REDIS]Set key:%s delete time ERROR! %s", key, conn->errstr);
        ret = -1;
    }

    if (1 == reply->integer) {
        // SUCCESS
        ret = 0;
    } else {
        // ERROR
        ret = -1;
    }

    freeReplyObject(reply);
    return ret;
}

char* make_hmset_command(char* key, unsigned int element_num, RFIELDS fields, RVALUES values) {
    char* cmd = NULL;
    unsigned int buf_size = 0;
    unsigned int use_size = 0;

    cmd = (char*)malloc(1024*1024);
    if (NULL == cmd) {
        goto END;
    }

    memset(cmd, 0, 1024 * 1024);
    buf_size += 1024 * 1024;

    strncat(cmd, "hmset", 6);
    use_size += 5;
    strncat(cmd, " ", 1);
    use_size += 1;
    strncat(cmd, key, 200);
    use_size += 200;

    for (unsigned int i = 0; i < element_num; ++i) {
        strncat(cmd, " ", 1);
        use_size += 1;

        if (use_size >= buf_size) {
            cmd = (char*)realloc(cmd, use_size + 1024 * 1024);

            if (NULL == cmd) {
                goto END;
            }

            buf_size += 1024 * 1024;
        }

        strncat(cmd, fields[i], FIELD_ID_SIZE);
        use_size += strlen(fields[i]);

        if (use_size >= buf_size) {
            cmd = (char*)realloc(cmd, use_size + 1024 * 1024);

            if (NULL == cmd) {
                goto END;
            }

            buf_size += 1024 * 1024;
        }

        strncat(cmd, " ", 1);
        use_size += 1;

        if (use_size >= buf_size) {
            cmd = (char*)realloc(cmd, use_size + 1024 * 1024);

            if (NULL == cmd) {
                goto END;
            }

            buf_size += 1024 * 1024;
        }

        strncat(cmd, values[i], VALUE_ID_SIZE);
        use_size += strlen(values[i]);

        if (use_size >= buf_size) {
            cmd = (char*)realloc(cmd, use_size + 1024 * 1024);

            if (NULL == cmd) {
                goto END;
            }

            buf_size += 1024 * 1024;
        }
    }

END:
    return cmd;
}

// 创建或者覆盖一个hash表；成功返回0,失败返回-1
// key：表名，element_num：表区域个数，fields：hash表区域名称数组；balues：hash表值数组
int rop_create_or_replace_hash_table(redisContext* conn, char* key, unsigned int element_num, RFIELDS fields, RVALUES values) {
    int ret = 0;
    redisReply* reply = NULL;
    char* cmd = make_hmset_command(key, element_num, fields, values);

    if (NULL == cmd) {
        TLOG_INFO("[-][GMS_REDIS]create hash table %s error", key);
        ret = -1;
        goto END_WITHOUT_FREE;
    }

    reply = (redisReply*)redisCommand(conn, cmd);
    if (strcmp(reply->str, "OK") != 0) {
        TLOG_INFO("[-][GMS_REDIS]Create hash table %s Error:%s,%s", key, reply->str, conn->errstr);
        ret = -1;
        goto END;
    }

END:
    free(cmd);
    freeReplyObject(reply);

END_WITHOUT_FREE:
    return ret;
}

// 给指定的hash表指定field对应的value自增num；成功返回0,失败返回-1
int rop_hincrement_one_field(redisContext* conn, char* key, char* field, unsigned int num) {
    int ret = 0;
    redisReply* reply = NULL;
    reply = (redisReply*)redisCommand(conn, "HINCRBY %s %s %d", key, field, num);

    if (NULL == reply) {
        TLOG_INFO("[-][GMS_REDIS]increment %s %s error %s", key, field, conn->errstr);	
        ret =  -1;
        goto END;
    }

END:
    freeReplyObject(reply);
    return ret;
}

// 批量执行链表插入命令，插入链表头部；成功返回0,失败返回-1
// key 链表名，fields 封装号的域名，values 封装好的值数组，val_num 值个数
int rop_hash_set_append(redisContext* conn, char* key, RFIELDS fields, RVALUES values, int val_num) {
    int ret = 0;
    redisReply* reply = NULL;

    // 批量插入命令到缓冲命令管道
    for (int i = 0; i < val_num; ++i) {
        ret = redisAppendCommand(conn, "HSET %s %s %s", key, fields[i], values[i]);
        if (REDIS_OK != ret) {
            TLOG_INFO("[-][GMS_REDIS]HSET %s %s %s ERROR![%s]", key, fields[i], values[i], conn->errstr);
            ret = -1;
            goto END;
        }
        ret = 0;
    }

    // submit command
    for(int i = 0; i < val_num; ++i){
        ret = redisGetReply(conn, (void**)&reply);
        if (REDIS_OK != ret) {
            ret = -1;
            TLOG_INFO("[-][GMS_REDIS]Commit HSET %s %s %s ERROR![%s]\n", key, fields[i], values[i], conn->errstr);
            freeReplyObject(reply);
            break;
        }
        freeReplyObject(reply);
        ret = 0;
    }

END:
    return ret;
}

// 向一个hash表中添加一条key-value数据；成功返回0,失败返回-1
int rop_hash_set(redisContext* conn, char* key, char* field, char* value) {
    int ret = 0;
    redisReply *reply = NULL;

    reply =  (redisReply*)redisCommand(conn, "hset %s %s %s", key, field, value);
    if (reply == NULL || reply->type != REDIS_REPLY_INTEGER) {
        TLOG_INFO("[-][GMS_REDIS]hset %s %s %s error %s", key, field, value, conn->errstr);
        ret =  -1;
    }

    freeReplyObject(reply);
    return ret;
}

// 从一个hash表中取出一条 key-value 数据 ；成功返回0,失败返回-1
int rop_hash_get(redisContext* conn, char* key, char* field, char* value) {
    int ret = 0;
    int len = 0;
    redisReply* reply = NULL;
    reply = (redisReply*)redisCommand(conn, "hget %s %s",key, field);

    if (NULL == reply || reply->type != REDIS_REPLY_STRING) {
        TLOG_INFO("[-][GMS_REDIS]hget %s %s  error %s", key, field, conn->errstr);
        ret = -1;
        goto END;
    }

    len = reply->len > VALUE_ID_SIZE ? VALUE_ID_SIZE : reply->len;
    strncpy(value, reply->str, len);
    value[len] = '\0';

END:
    freeReplyObject(reply);
    return ret;
}

// 从key 指定的hash集中一处指定的域(字段)；成功返回0,失败返回-1
int rop_hash_del(redisContext* conn, char* key, char* field) {
    int ret = 0;
    redisReply* reply = NULL;
    reply = (redisReply*)redisCommand(conn, "hdel %s %s", key, field);

    if (1 != reply->integer) {
        TLOG_INFO("[-][GMS_REDIS]hdel %s %s  error %s", key, field, conn->errstr);
        ret = -1;
    }

    freeReplyObject(reply);
    return ret;
}

// 批量执行链表插入命令，插入链表头部；成功返回0,失败返回-1
int rop_list_push_append(redisContext* conn, char* key, RVALUES values, int val_num) {
    int ret = 0;
    redisReply* reply = NULL;

    // 批量插入命令到缓冲命令管道
    for (int i = 0; i < val_num; ++i) {
        ret = redisAppendCommand(conn, "lpush %s %s", key, values[i]);
        if (REDIS_OK != ret) {
            TLOG_INFO("[-][GMS_REDIS]PLUSH %s %s ERROR! %s", key, values[i], conn->errstr);
            ret = -1;
            goto END;
        }

        ret = 0;
    }

    // submit command
    for (int i = 0; i < val_num; ++i) {
        ret = redisGetReply(conn, (void**)&reply);
        if (REDIS_OK != ret) {
            ret = -1;
            TLOG_INFO("[-][GMS_REDIS]Commit LPUSH %s %s ERROR! %s", key, values[i], conn->errstr);
            freeReplyObject(reply);
            goto END;
        }
        freeReplyObject(reply);
        ret = 0;
    }

END:
    return ret;
}

// 单条数据插入链表
int rop_list_push(redisContext* conn, char* key, char* value) {
    int ret = 0;
    redisReply* reply = NULL;
    reply = (redisReply*)redisCommand(conn, "LPUSH %s %s", key, value);

    if (REDIS_REPLY_INTEGER != reply->type) {
        TLOG_INFO("[-][GMS_REDIS]LPUSH %s %s error!%s", key, value, conn->errstr);
        ret = -1;
    }

    freeReplyObject(reply);
    return ret;
}

// 获取链表中元素个数；成功返回个数，失败返回-1
int rop_get_list_cnt(redisContext* conn, char* key) {
    int cnt = 0;
    redisReply* reply = NULL;
    reply = (redisReply*)redisCommand(conn, "LLEN %s", key);

    if (REDIS_REPLY_INTEGER != reply->type) {
        TLOG_INFO("[-][GMS_REDIS]LLEN %s error %s", key, conn->errstr);
        cnt = -1;
        goto END;
    }

    cnt = reply->integer;

END:
    freeReplyObject(reply);
    return cnt;
}

// 按照一定范围截断链表中的数据；成功返回0,失败返回-1
int rop_trim_list(redisContext* conn, char* key, int begin, int end) {
    int ret = 0;
    redisReply* reply = NULL;
    reply = (redisReply*)redisCommand(conn, "LTRIM %s %d %d", key, begin, end);

    if (REDIS_REPLY_STATUS != reply->type) {
        TLOG_INFO("[-][GMS_REDIS]LTRIM %s %d %d error!%s", key, begin, end, conn->errstr);
        ret = -1;
    }

    freeReplyObject(reply);
    return ret;
}

// 获得链表中的数据；成功返回0,失败返回-1
// values 得到表中的value数据，get_num 得到结果value 的个数
int rop_range_list(redisContext* conn, char* key, int from_pos, int end_pos, RVALUES values, int* get_num) {
    int ret = 0;
    redisReply* reply = NULL;
    int max_count = 0;
    int count = end_pos - from_pos + 1;
    reply = (redisReply*)redisCommand(conn, "LRANGE %s %d %d", key, from_pos, end_pos);

    if (0 == reply->elements || REDIS_REPLY_ARRAY != reply->type) {
        TLOG_INFO("[-][GMS_REDIS]LRANGE %s  error!%s", key, conn->errstr);
        ret = -1;
        goto END;
    }

    max_count = (reply->elements > count) ? count : reply->elements;
    *get_num = max_count;

    for (int i = 0; i < max_count; ++i) {
        strncpy(values[i], reply->element[i]->str, VALUE_ID_SIZE - 1);
    }

END:
    if (NULL != reply) {
        freeReplyObject(reply);
    }

    return ret;
}

// 批量执行已经封装好的 redis 命令；成功返回0,失败返回-1
int rop_redis_append(redisContext* conn, RCOMMANDS cmds, int cmd_num) {
    int ret = 0;
    redisReply* reply = NULL;

    for (int i = 0; i < cmd_num; ++i) {
        ret = redisAppendCommand(conn, cmds[i]);
        if (REDIS_OK != ret) {
            fprintf(stderr, "[-][GMS_REDIS]Append Command: %s ERROR!\n", cmds[i]);
            TLOG_INFO("[-][GMS_REDIS]Append Command: %s ERROR! %s", cmds[i], conn->errstr);
            ret = -1;
            goto END;
        }

        ret = 0;
    }

    for (int i = 0; i < cmd_num; ++i) {
        ret = redisGetReply(conn, (void**)&reply);
        if (REDIS_OK != ret) {
            ret = -1;
            fprintf(stderr, "[-][GMS_REDIS]Commit Command:%s ERROR!\n", cmds[i]);
            TLOG_INFO("[-][GMS_REDIS]Commit Command:%s ERROR! %s", cmds[i], conn->errstr);
            freeReplyObject(reply);
            break;
        }

        freeReplyObject(reply);
        ret = 0;
    }

END:
    return ret;
}

// 执行单向命令 无返回值 命令自行输入；成功返回0,失败返回-1
int rop_redis_command(redisContext* conn, char* cmd) {
    int ret = 0;
    redisReply* reply = NULL;
    reply = (redisReply*)redisCommand(conn, cmd);
    if (NULL == reply) {
        TLOG_INFO("[-][GMS_REDIS]Command : %s ERROR!%s", cmd, conn->errstr);
        ret = -1;
    }

    freeReplyObject(reply);
    return ret;
}

// 测试一个reply的结果类型，得到对应的类型用对应的方法获取数据
void rop_test_reply_type(redisReply* reply) {
    switch (reply->type) {
    case REDIS_REPLY_STATUS:
        TLOG_INFO("[+][GMS_REDIS]=REDIS_REPLY_STATUS=[string] use reply->str to get data, reply->len get data len\n");
        break;
    case REDIS_REPLY_ERROR:
        TLOG_INFO("[+][GMS_REDIS]=REDIS_REPLY_ERROR=[string] use reply->str to get data, reply->len get date len\n");
        break;
    case REDIS_REPLY_INTEGER:
        TLOG_INFO("[+][GMS_REDIS]=REDIS_REPLY_INTEGER=[long long] use reply->integer to get data\n");
        break;
    case REDIS_REPLY_NIL:
        TLOG_INFO("[+][GMS_REDIS]=REDIS_REPLY_NIL=[] data not exist\n");
        break;
    case REDIS_REPLY_ARRAY:
        TLOG_INFO("[+][GMS_REDIS]=REDIS_REPLY_ARRAY=[array] use reply->elements to get number of data, reply->element[index] to get (struct redisReply*) Object\n");
        break;
    case REDIS_REPLY_STRING:
        TLOG_INFO("[+][GMS_REDIS]=REDIS_REPLY_string=[string] use reply->str to get data, reply->len get data len\n");
        break;
    default:
        TLOG_INFO("[-][GMS_REDIS]Can't parse this type\n");
        break;
    }
}

// 设置key对应的值为斯string类型的value；成功返回0,失败返回-1
int rop_set_string(redisContext* conn, char* key, char* value) {
    int ret = 0;
    redisReply* reply = NULL;
    reply = (redisReply*)redisCommand(conn, "set %s %s", key, value);
    if (strcmp(reply->str, "OK") != 0) {
        ret = -1;
    }

    freeReplyObject(reply);
    return ret;
}

// 设置key对应的值为斯特日嗯类型的value, 同时设置过期时间；成功返回0,失败返回-1
int rop_setex_string(redisContext* conn, char* key, unsigned int seconds, char* value) {
    int ret = 0;
    redisReply* reply = NULL;
    reply = (redisReply*)redisCommand(conn, "setex %s %u %s", key, seconds, value);

    if (strcmp(reply->str, "OK") != 0) {
        ret = -1;
    	TLOG_INFO("rop_setex_string fail");
    }

    freeReplyObject(reply);
    return ret;
}

// 获取key对应的value
int rop_get_string(redisContext* conn, char* key, char* value) {
    int ret = 0;
    redisReply* reply = NULL;
    reply = (redisReply*)redisCommand(conn, "get %s", key);

    if (REDIS_REPLY_STRING != reply->type) {
        ret = -1;
    	TLOG_INFO("rop_get_string fail, key=%s", key);
        goto END;
    }

    strncpy(value, reply->str, reply->len);
    value[reply->len] = '\0';

END:
    freeReplyObject(reply);
    return ret;
}

//=======================有关集合相关操作===========================

// 将指定的zset表，添加新成员（key或成员不存在则创建）
// zset zset表名；score zset分数(权重)；member zset成员名
int rop_zset_add(redisContext* conn, char* key, long score, char* member) {
    int ret = 0;
    redisReply* reply = NULL;
    reply = (redisReply*)redisCommand(conn, "ZADD %s %ld %s", key, score, member);

    if (1 != reply->integer) {
        TLOG_INFO("[-][GMS_REDIS]ZADD: %s,member: %s Error:%s,%s\n", key, member,reply->str, conn->errstr);
        ret = -1;
    }

    freeReplyObject(reply);
    return ret;
}

// 删除指定的成员
int rop_zset_zrem(redisContext* conn, char* key, char* member) {
    int ret = 0;
    redisReply* reply = NULL;
    reply = (redisReply*)redisCommand(conn, "ZREM %s %s", key, member);

    if (1 != reply->integer) {
        TLOG_INFO("[-][GMS_REDIS]ZREM: %s,member: %s Error:%s,%s\n", key, member,reply->str, conn->errstr);
        ret = -1;
    }

    freeReplyObject(reply);
    return ret;

}

// 删除所有的成员
int rop_zset_del_all(redisContext* conn, char* key) {
    int ret = 0;
    redisReply* reply = NULL;

    reply = (redisReply*)redisCommand(conn, "ZREMRANGEBYRANK %s 0 -1", key);

    if (reply->type != REDIS_REPLY_INTEGER) //如果不是整型
    {
         TLOG_INFO("[-][GMS_REDIS]ZREMRANGEBYRANK: %s,Error:%s,%s\n", key,reply->str, conn->errstr);
        ret = -1;
        goto END;
    }

END:

    freeReplyObject(reply);
    return ret;
}

// 降序获取有序集合的元素
int rop_zset_zrevrange(redisContext* conn, char* key, int from_pos, int end_pos, RVALUES values, int* get_num) {
    int ret = 0;
    redisReply* reply = NULL;
    int max_count = 0;
    int count = end_pos - from_pos + 1;
    reply = (redisReply*)redisCommand(conn, "ZREVRANGE %s %d %d", key, from_pos, end_pos);

    if (REDIS_REPLY_ATTR != reply->type) {
        TLOG_INFO("[-][GMS_REDIS]ZREVRANGE %s  error!%s\n", key, conn->errstr);
        ret = -1;
        goto END;
    }

    max_count = (reply->elements > count) ? count : reply->elements;
    *get_num = max_count;

    for (int i = 0; i < max_count; ++i) {
        strncpy(values[i], reply->element[i]->str, VALUE_ID_SIZE - 1);
        values[i][VALUE_ID_SIZE - 1] = 0;
    }

END:
    if (NULL != reply) {
        freeReplyObject(reply);
    }

    return ret;
}

// 将指定的zset表，对应的成员，值自增1
int rop_zset_increment(redisContext* conn, char* key, char* member) {
    int ret = 0;
    redisReply* reply = NULL;
    reply = (redisReply*)redisCommand(conn, "ZINCRBY %s 1 %s", key, member);

    if (REDIS_REPLY_ERROR == reply->type) {
        TLOG_INFO("[-][GMS_REDIS]Add or increment table: %s, member: %s Error: %s, %s", key, member, reply->str, conn->errstr);
        ret = -1;
    }

    freeReplyObject(reply);
    return ret;
}

// 获得集合中的元素个数 失败返回-1
int rop_zset_zcard(redisContext* conn, char* key) {
    int cnt = 0;
    redisReply* reply = NULL;
    reply = (redisReply*)redisCommand(conn, "ZCARD %s", key);

    if (REDIS_REPLY_INTEGER != reply->type) {
        TLOG_INFO("[-][GMS_REDIS]ZCARD %s error %s\n", key, conn->errstr);
        cnt = -1;
        goto END;
    }

    cnt = reply->integer;

END:
    freeReplyObject(reply);
    return cnt;
}

// 得到zset一个member的score；>= 0 成功，失败返回-1
int rop_zset_get_score(redisContext* conn, char* key, char* member) {
    int score;
    redisReply* reply = NULL;
    reply = (redisReply*)redisCommand(conn, "ZSCORE %s %s", key, member);
    // rop_test_reply_type(reply);

    if(REDIS_REPLY_STRING != reply->type) {
        TLOG_INFO("[-][GMS_REDIS]ZSCORE %s %s error %s\n", key, member,conn->errstr);
        score = -1;
        goto END;
    }

    score = atoi(reply->str);

END:
    freeReplyObject(reply);
    return score;
}

// 判断某个成员是否存在；存在返回1,不存在返回0,出错返回-1
int rop_zset_exist(redisContext* conn, char* key, char* member) {
    int ret = 0;
    redisReply* reply = NULL;
    reply = (redisReply*)redisCommand(conn, "ZLEXCOUNT %s [%s [%s", key, member, member);

    if (REDIS_REPLY_INTEGER != reply->type) {
        TLOG_INFO("[-][GMS_REDIS]zlexcount: %s,member: %s Error:%s,%s\n", key, member,reply->str, conn->errstr);
        ret = -1;
        goto END;
    }

    ret = reply->integer;

END:
    freeReplyObject(reply);
    return ret;
}

// 批量将指定的zset表，对应的成员，值自增1
int rop_zset_increment_append(redisContext* conn, char* key, RVALUES values, int val_num) {
    int ret = 0;
    redisReply* reply;

    for(int i = 0; i < val_num; ++i) {
        ret = redisAppendCommand(conn, "ZINCRBY %s 1 %s", key, values[i]);

        if (REDIS_OK != ret) {
            TLOG_INFO("[-][GMS_REDIS]ZINCRBY %s 1 %s ERROR! %s\n", key, values[i], conn->errstr);
            ret = -1;
            goto END;
        }

        ret = 0;
    }

    for (int i = 0; i < val_num; ++i) {
        ret = redisGetReply(conn, (void **)&reply);

        if (REDIS_OK != ret) {
            TLOG_INFO("[-][GMS_REDIS]Commit ZINCRBY %s 1 %s ERROR!%s\n", key, values[i], conn->errstr);
            ret = -1;
            freeReplyObject(reply);
            break;
        }

        freeReplyObject(reply);
        ret = 0;
    }

END:
    return ret;
}
