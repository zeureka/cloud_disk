#pragma once

#include "./cfg.h"
#include "./cJSON.h"
#include "./redis_op.h"
#include <string.h>
#include <ctype.h>
#include <hiredis/hiredis.h>

#define FILE_NAME_LEN       (256)   ///< 文件名字长度
#define TEMP_BUF_MAX_LEN    (512)   ///< 临时缓冲区大小
#define FILE_URL_LEN        (512)   ///< 文件所存放storage的host_name长度
#define HOST_NAME_LEN       (30)    ///< 主机ip地址长度
#define ACCOUNT_LEN         (128)   ///< 用户名字长度
#define TOKEN_LEN           (128)   ///< 登陆token长度
#define MD5_LEN             (256)   ///< 文件md5长度
#define PASSWORD_LEN        (256)   ///< 密码长度
#define TIME_STRING_LEN     (25)    ///< 时间戳长度
#define SUFFIX_LEN          (8)     ///< 后缀名长度
#define PIC_NAME_LEN        (10)    ///< 图片资源名字长度
#define PIC_URL_LEN         (256)   ///< 图片资源url名字长度

/**
 * @brief 去掉一个字符串两边的空白字符
 *
 * @param inbuf (in)    待操作的字符串
 *
 * @return 判断函数是否执行成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int trim_space(char* inbuf);

/**
 * @brief 在字符串 full_data 中查找字符串 substr 第一次出现的位置
 *
 * @param full_data     (in)    被查找的字符串
 * @param full_data_len (in)    被查找的字符串长度
 * @param substr        (in)    需要知道位置的字符串
 *
 * @return 判断函数是否执行成功
 *   @retval 匹配字符串首地址 success
 *   @retval NULL             failed
 */
char* memstr(char* full_data, int full_data_len, char* substr);

/**
 * @brief 解析 url query 类似 abc=123&bbb=456 字符串, 传入一个key，得到相应的value
 *
 * @param url         (in)  待解析的url地址字符串
 * @param key         (in)  需要的key关键字
 * @param value       (out) 得到的value值
 * @param value_len_p (out) 得到的value值的长度
 *
 * @return 判断函数是否执行成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int query_parse_key_value(char* url, const char* key, char* value, int* value_len_p);

/**
 * @brief 通过文件名 file_name, 得到文件后缀字符串，保存在 suffix 如果非法文件后缀
 *
 * @param file_name (in)    待操作的文件名
 * @param suffix    (out)   文件名的后缀
 *
 * @return 判断函数是否执行成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int get_file_suffix(char* file_name, char* suffix);

/**
 * @brief 字符串 strSrc 中字符串 strFind，替换为strReplace
 *
 * @param strSrc        (in/out)    待操作的字符串
 * @param strFind       (in)        被替换的子串
 * @param str_replace   (in)        需要的子串
 *
 * @return 判断函数是否执行成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int str_replace(char* strSrc, char* strFind, char* strReplace);

/**
 * @brief 返回前端情况
 *
 * @param status_num    (in)    返回的code码
 *
 * @return 判断函数是否执行成功
 *   @retval 返回的指针不为空则需要free
 *   @retval NULL表示失败
 */
char* return_status(char* status_num);

/**
 * @brief 验证登录token
 *
 * @param account   (in)    被验证的账号
 * @param token     (in)    被验证的tokne
 *
 * @return 判断函数是否执行成功
 *   @retval 0  success
 *   @retval -1 failed
 */
int verify_token(char* account, char* token);
