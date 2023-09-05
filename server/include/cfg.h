/**
 * @file cfg.h
 * @brief 读取配置文件
 */

#ifndef _CFG_H_
#define _CFG_H_

#define CFG_PATH    "./conf/config.json"

/**
 * @brief 从配置文件中得到相应的数据
 *
 * @param path  (in)    配置文件路径
 * @param title (in)    配置文件名称(eg: mysql)
 * @param key   (in)    配置文件关键字(eg: database)
 * @param value (out)   关键字对应的值(eg: cloud_disk)
 *
 * @return 函数执行成功或失败
 *   @retval 0  success
 *   @retval -1 failed
 */
extern int get_cfg_value(const char* path, char* title, char* key, char* value);

/**
 * @brief 获取 mysql 数据库用户名，用户密码，数据库名等信息
 *
 * @param mysql_user    (out)   数据库用户名
 * @param mysql_pwd     (out)   用户密码
 * @param mysql_db      (out)   操作的数据库名
 *
 * @return 函数执行成功或失败
 *   @retval 0  success
 *   @retval -1 failed
 */
extern int get_mysql_info(char* mysql_user, char* mysql_pwd, char* mysql_db);

/**
 * @brief 获取 redis 数据库IP，用户PORT，Password等信息
 *
 * @param redis_ip    	    (out)   数据库IP
 * @param redis_port        (out)   数据库PORT
 * @param redis_password    (out)   数据库密码
 *
 * @return 函数执行成功或失败
 *   @retval 0  success
 *   @retval -1 failed
 */
extern int get_redis_info(char* redis_ip, char* redis_port, char* redis_password);

#endif
