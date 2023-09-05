/**
 * @file redis_op.h
 * @brief redis数据库操作接口封装
 */

#ifndef _REDIS_OP_H_
#define _REDIS_OP_H_

#include <hiredis/hiredis.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdint.h>
#include "./log.h"

#define REDIS_COMMAND_SIZE  300     // redis command 指令最大长度
#define FIELD_ID_SIZE       100     // redis hash表 field域字段长度
#define VALUE_ID_SIZE       1024    // redis hash表 value域字段长度

// typedef char (*RCOMMANDS)[REDIS_COMMAND_SIZE];
using RCOMMANDS = char(*)[REDIS_COMMAND_SIZE];  // redis 存放命令的字符串数组
using RFIELDS = char(*)[FIELD_ID_SIZE];         // redis 存放hash表field字符串数组
using RVALUES = char(*)[VALUE_ID_SIZE];         // redis 存放hash表value字符串数组

/**
 * @brief redis tcp 模式链接 无密码
 *
 * @param ip_str    (in)    redis数据库服务器IP地址
 * @param port_str  (in)    redis数据库服务器port
 *
 * @returns 判断函数是否执行成功
 *   @retval 链接句柄 success
 *   @retval NULL     failed
 */
redisContext* rop_connectdb_nopwd(char* ip_str, char* port_str);

/**
 * @brief redis tcp 模式链接 需要密码
 *
 * @param ip_str    (in)    redis数据库服务器IP地址
 * @param port_str  (in)    redis数据库服务器port
 * @param pwd       (in)    redis数据库密码
 *
 * @returns 判断函数是否执行成功
 *   @retval 链接句柄 success
 *   @retval NULL     failed
 */
redisContext* rop_connectdb(char* ip_str, char* port_str, char* pwd);

/**
 * @brief unix域模式链接
 *
 * @param sock_path (in)    unix域socket文件
 * @param pwd       (in)    redis数据库密码
 *
 * @returns 判断函数是否执行成功
 *   @retval 链接句柄 success
 *   @retval NULL     failed
 */
redisContext* rop_connectdb_unix(char* sock_path, char* pwd);

/**
 * @brief tcp 连接 redis 超时等待
 *
 * @param ip_str    (in)    redis数据库服务器IP地址
 * @param port_str  (in)    redis数据库服务器port
 * @param timeout   (in)    连接超时等待时长
 *
 * @returns 判断函数是否执行成功
 *   @retval 链接句柄 success
 *   @retval NULL     failed
 */
redisContext* rop_connectdb_timeout(char* ip_str, char* port_str, struct timeval* timeout);

/**
 * @brief 关闭指定的链接句柄
 *
 * @param conn  (in)    需要关闭的redis链接句柄
 */
void rop_disconnect(redisContext* conn);

/**
 * @brief 选择 redis 其中的一个数据库
 *
 * @param conn  (in)    已连接的redis数据库链接句柄
 * @param db_no (in)    需要操作的数据的编号
 *
 * @returns 判断函数是否执行成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int rop_selectdatabase(redisContext* conn, unsigned int db_no);

/**
 * @brief 清空当其数据库所有信息
 *
 * @param conn  (in)    已连接的redis数据库链接句柄
 *
 * @returns 判断函数是否执行成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int rop_flush_database(redisContext* conn);

/**
 * @brief 判断key值是否存在
 *
 * @param conn  (in)    已连接的redis数据库链接句柄
 * @param key   (in)    需要查询的key
 *
 * @returns 判断函数是否执行成功
 *   @retval 0  不存在
 *   @retval 1  存在
 *   @retval -1 失败
 */
int rop_is_key_exist(redisContext* conn, char* key);

/**
 * @brief 删除一个 key
 *
 * @param conn  (in)    已连接的redis数据库链接句柄
 * @param key   (in)    需要删除的key
 *
 * @returns 判断函数是否执行成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int rop_del_key(redisContext* conn, char* key);

/**
 * @brief 打印库中所有匹配 pattern 的key
 *
 * @param conn      (in)    已连接的redis数据库链接句柄
 * @param pattern   (in)    待匹配的 pattern 字符串
 */
void rop_show_keys(redisContext* conn, char* pattern);

/**
 * @brief 设置一个key的删除时间，系统到达一定时间将会自动删除该key
 *
 * @param conn      (in)    已连接的redis数据库链接句柄
 * @param key       (in)    需要删除的key
 * @param time_t    (in)    等待时长
 *
 * @returns 判断函数是否执行成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int rop_set_key_lifecycle(redisContext* conn, char* key, time_t delete_time);

static char* make_hmset_command(char* key, unsigned int element_num, RFIELDS fields, RVALUES values);

/**
 * @brief 创建或者覆盖一个hash表
 *
 * @param conn          (in)    已连接的redis数据库链接句柄
 * @param key           (in)    需要创建或者覆盖的hash表的表名
 * @param element_num   (in)    表的字段和值数组中的元素数量
 * @param fields        (in)    hash 表中的字段数组
 * @param values        (in)    hash 表中字段对应的值
 *
 * @returns 判断函数是否执行成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int rop_create_or_replace_hash_table(redisContext* conn, char* key, unsigned int element_num, RFIELDS fields, RVALUES values);

/**
 * @brief 给指定hash表指定的field对应的value自增num
 *
 * @param conn  (in)    已连接的redis数据库链接句柄
 * @param key   (in)    需要修改的hash表的表名
 * @param field (in)    需要修改的字段名
 * @param num   (in)    字段对应的值自增的值
 *
 * @returns 判断函数是否执行成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int rop_hincrement_one_field(redisContext* conn, char* key, char* field, unsigned int num);

/**
 * @brief 批量执行链表插入命令，插入链表头部
 *
 * @param conn      (in)    已连接的redis数据库链接句柄
 * @param key       (in)    待操作的链表名
 * @param fields    (in)    封装好的字段数组
 * @param values    (in)    字段对应的值
 * @param val_num   (in)    字段和数组中的元素数量
 *
 * @returns 判断函数是否执行成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int rop_hash_set_append(redisContext* conn, char* key, RFIELDS fields, RVALUES values, int val_num);

/**
 * @brief 向一个hash表中添加一条 key-value 数据
 *
 * @param conn  (in)    已连接的redis数据库链接句柄
 * @param key   (in)    待操作的hash表
 * @param field (in)    待插入的键
 * @param value (in)    键对应的值
 *
 * @returns 判断函数是否执行成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int rop_hash_set(redisContext* conn, char* key, char* field, char* value);

/**
 * @brief 从一个hash表中取出一条 key-value 数据
 *
 * @param conn  (in)    已连接的redis数据库链接句柄
 * @param key   (in)    待操作的hash表
 * @param field (in)    需要获得的值对应的字段名
 * @param value (out)    需要获得的值
 *
 * @returns 判断函数是否执行成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int rop_hash_get(redisContext* conn, char* key, char* field, char* value);

/**
 * @brief 删除指定的hash集合中的字段
 *
 * @param conn  (in)    已连接的redis数据库链接句柄
 * @param key   (in)    待操作的hash表
 * @param field (in)    需要删除的字段名
 *
 * @returns 判断函数是否执行成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int rop_hash_del(redisContext* conn, char* key, char* field);

/**
 * @brief 批量执行链表插入命令，插入链表头部
 *
 * @param conn      (in)    已连接的redis数据库链接句柄
 * @param key       (in)    待插入数据的链表名
 * @param values    (in)    需要插入的值数组
 * @param val_num   (in)    值数组的个数
 *
 * @returns 判断函数是否执行成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int rop_list_push_append(redisContext* conn, char* key, RVALUES values, int val_num);

/**
 * @brief 单条数据插入链表
 *
 * @param conn      (in)    已连接的redis数据库链接句柄
 * @param key       (in)    待插入数据的链表名
 * @param value     (in)    需要插入的值
 *
 * @returns 判断函数是否执行成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int rop_list_push(redisContext* conn, char* key, char* value);

/**
 * @brief 获取链表中元素个数
 *
 * @param conn      (in)    已连接的redis数据库链接句柄
 * @param key       (in)    待操作的链表名
 *
 * @returns 判断函数是否执行成功
 *   @retval >=0    success
 *   @retval -1     failed
 */
int rop_get_list_cnt(redisContext* conn, char* key);

/**
 * @brief 按照一定范围截断链表中的数据
 *
 * @param conn      (in)    已连接的redis数据库链接句柄
 * @param key       (in)    待操作的链表名
 * @param begin     (in)    范围的起始位置
 * @param end       (in)    范围的结束位置
 *
 * @returns 判断函数是否执行成功
 *   @retval 0    success
 *   @retval -1   failed
 */
int rop_trim_list(redisContext* conn, char* key, int begin, int end);

/**
 * @brief 获得表中某个范围的value数据
 *
 * @param conn      (in)    已连接的redis数据库链接句柄
 * @param key       (in)    待操作的链表名
 * @param from_pos  (in)    范围的起始位置
 * @param end_pos   (in)    范围的结束位置
 * @param values    (out)   得到的value数据数组
 * @param get_num   (out)   得到的value个数
 *
 * @returns 判断函数是否执行成功
 *   @retval 0    success
 *   @retval -1   failed
 */
int rop_range_list(redisContext* conn, char* key, int from_pos, int end_pos, RVALUES values, int* get_num);

/**
 * @brief 批量执行已经封装好的 redis 命令
 *
 * @param conn      (in)    已连接的redis数据库链接句柄
 * @param cmds      (in)    待执行的redis命令数组
 * @param cmd_num   (in)    redis命令数量
 *
 * @returns 判断函数是否执行成功
 *   @retval 0    success
 *   @retval -1   failed
 */
int rop_redis_append(redisContext* conn, RCOMMANDS cmds, int cmd_num);

/**
 * @brief 执行单向命令 无返回值 命令自行输入
 *
 * @param conn  (in)    已连接的redis数据库链接句柄
 * @param cmd   (in)    待执行的redis命令
 *
 * @returns 判断函数是否执行成功
 *   @retval 0    success
 *   @retval -1   failed
 */
int rop_redis_command(redisContext* conn, char* cmd);

/**
 * @brief 测试一个reply的结果类型，得到对应的类型用对应的方法获取数据
 *
 * @param reply   返回的命令结果
 */
void rop_test_reply_type(redisReply* reply);

/**
 * @brief 设置key对应的值为string类型的value
 * 
 * @param conn  (in)    redis数据库连接句柄
 * @param key   (in)    待改变的value值对应的key
 * @param value (in)    需要的value
 *
 * @returns 判断函数是否执行成功
 *   @retval 0    success
 *   @retval -1   failed
 */
int rop_set_string(redisContext* conn, char* key, char* value);

/**
 * @brief 设置key对应的值为string类型的value, 同时设置过期时间
 *
 * @param conn      (in)    redis数据库连接句柄
 * @param key       (in)    待改变的value值对应的key
 * @param value     (in)    需要的value
 * @param seconds   (in)    过期时间
 *
 * @returns 判断函数是否执行成功
 *   @retval 0    success
 *   @retval -1   failed
 */
int rop_setex_string(redisContext* conn, char* key, unsigned int seconds, char* value);

// 获取key对应的value
/**
 * @brief 获取key对应的value
 *
 * @param conn      (in)    redis数据库连接句柄
 * @param key       (in)    需要的value对应的key值
 * @param value     (out)   需要的value
 *
 * @returns 判断函数是否执行成功
 *   @retval 0    success
 *   @retval -1   failed
 */
int rop_get_string(redisContext* conn, char* key, char* value);

//=======================有关集合相关操作===========================

/**
 * @brief 将指定的zset表，添加新成员（key或成员不存在则创建）
 *
 * @param conn   (in)    redis数据库连接句柄
 * @param key    (in)    zset 表名
 * @param score  (in)    zset分数(权重)
 * @param member (in)    zset待添加的成员名
 *
 * @returns 判断函数是否执行成功
 *   @retval 0    success
 *   @retval -1   failed
 */
int rop_zset_add(redisContext* conn, char* key, long score, char* member);

/**
 * @brief 删除指定成员
 * @param conn   (in)    redis数据库连接句柄
 * @param key    (in)    zset 表名
 * @param member (in)    zset待删除的成员名
 *
 * @returns 判断函数是否执行成功
 *   @retval 0    success
 *   @retval -1   failed
 */
int rop_zset_zrem(redisContext* conn, char* key, char* member);

/**
 * @brief 删除所有的成员
 * @param conn   (in)    redis数据库连接句柄
 * @param key    (in)    zset 表名
 *
 * @returns 判断函数是否执行成功
 *   @retval 0    success
 *   @retval -1   failed
 */
int rop_zset_del_all(redisContext* conn, char* key);

/**
 * @brief 降序获取有序集合的元素
 *
 * @param conn      (in)    redis数据库连接句柄
 * @param key       (in)    zset 表名
 * @param from_pos  (in)    获取的起始位置
 * @param end_pos   (in)    获取的结束位置
 * @param values    (out)   获得的有序元素集合
 * @param get_num   (out)   有序元素的个数
 *
 * @returns 判断函数是否执行成功
 *   @retval 0    success
 *   @retval -1   failed
 */
extern int rop_zset_zrevrange(redisContext* conn, char* key, int from_pos, int end_pos, RVALUES values, int* get_num);

/**
 * @brief 将指定的zset表，对应的成员，值自增1
 *
 * @param conn      (in)    redis数据库连接句柄
 * @param key       (in)    zset 表名
 * @param member    (in)    待修改的数据成员
 *
 * @returns 判断函数是否执行成功
 *   @retval 0    success
 *   @retval -1   failed
 */
int rop_zset_increment(redisContext* conn, char* key, char* member);

/**
 * @brief 获得集合中的元素个数
 *
 * @param conn      (in)    redis数据库连接句柄
 * @param key       (in)    zset 表名
 *
 * @returns 判断函数是否执行成功
 *   @retval >=0  success
 *   @retval -1   failed
 */
int rop_zset_zcard(redisContext* conn, char* key);

/**
 * @brief 得到zset一个member的score
 *
 * @param conn      (in)    redis数据库连接句柄
 * @param key       (in)    zset 表名
 * @param member    (in)    查找的member
 *
 * @returns 判断函数是否执行成功
 *   @retval >=0  success
 *   @retval -1   failed
 */
int rop_zset_get_score(redisContext* conn, char* key, char* member);

/**
 * @brief 判断某个成员是否存在
 *
 * @param conn      (in)    redis数据库连接句柄
 * @param key       (in)    zset 表名
 * @param member    (in)    待判断的member
 *
 * @returns 判断函数是否执行成功
 *   @retval 1  存在
 *   @retval 0  不存在
 *   @retval -1 failed
 */
extern int rop_zset_exist(redisContext* conn, char* key, char* member);

/**
 * @brief 批量将指定的zset表对应的成员的值自增1
 *
 * @param conn      (in)    redis数据库连接句柄
 * @param key       (in)    zset 表名
 * @param values    (in)    待自增的成员数组
 * @param val_num   (in)    待自增的成员个数
 *
 * @returns 判断函数是否执行成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int rop_zset_increment_append(redisContext* conn, char* key, RVALUES values, int val_num);

#endif 
