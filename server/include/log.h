/**
 * @file log.h
 * @brief log 日志库
 */

#ifndef LOG_HPP
#define LOG_HPP

#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>

#define DEFAULT_PATH "./record/LogFile"
/******************************** definition ********************************/

enum class LogType : char { DEBUG, INFO, WARN, ERROR, FATAL };

class TinyLog {
public:
    // 单例懒汉模式创建对象
    static TinyLog* get_instance();

    // 异步模式线程回调函数
    static void* flush_log_thread(void* args);

    // 可选择的参数有日志文件名，日志缓冲区大小，最大日志行数，日志队列容量
    bool init(const char* file_name, bool close_log = false, int log_buf_size = 8192, 
              int split_lines = 5000000, int queue_capacity = 0);

    // 写入日志文件
    void write_log(LogType type, const char* file, int32_t line, const char* format, ...);

    // 刷新缓冲区
    void flush(void);

    bool get_close_log();
    void set_close_log(const bool& close_log);
    void set_dir_name(const char* dirName);


private:
    TinyLog();
    virtual ~TinyLog();
    void* async_write_log();

private:
    char dir_name[128];         // 日志文件路径
    char log_name[128];         // log 文件名
    int m_split_lines;          // 日志最大行数
    int m_log_buf_size;         // 日志缓冲区大小
    long long m_count;          // 日志行数记录
    int m_today;                // 记录当天是哪一天
    FILE* m_fp;                 // 打开 log 的文件指针
    char* m_buf;                // 日志缓冲区指针
    bool m_is_async;            // 是否同步标志位
    bool m_close_log;           // 关闭日志
    int m_queue_capacity;       // 队列容量
    std::mutex m_mutex;                     // 互斥锁
    std::queue<std::string> m_log_queue;   // 阻塞队列
};

static TinyLog* LogInstance = TinyLog::get_instance();
extern TinyLog* LogInstance;

#define TLOG_DEBUG(format, ...) \
    if (!LogInstance->get_close_log()) { LogInstance->write_log(LogType::DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__); LogInstance->flush(); }
#define TLOG_INFO(format, ...) \
    if (!LogInstance->get_close_log()) { LogInstance->write_log(LogType::INFO, __FILE__, __LINE__, format, ##__VA_ARGS__); LogInstance->flush(); }
#define TLOG_WARN(format, ...) \
    if (!LogInstance->get_close_log()) { LogInstance->write_log(LogType::WARN, __FILE__, __LINE__, format, ##__VA_ARGS__); LogInstance->flush(); }
#define TLOG_ERROR(format, ...) \
    if (!LogInstance->get_close_log()) { LogInstance->write_log(LogType::ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__); LogInstance->flush(); }
#define TLOG_FATAL(format, ...) \
    if (!LogInstance->get_close_log()) { LogInstance->write_log(LogType::FATAL, __FILE__, __LINE__, format, ##__VA_ARGS__); LogInstance->flush(); }

#endif
