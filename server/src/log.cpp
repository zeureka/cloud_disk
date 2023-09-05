#include "../include/log.h"
/******************************** implementation ********************************/

TinyLog::TinyLog() {
    m_count = 1;
    m_is_async = false;
    init(DEFAULT_PATH);
}

TinyLog::~TinyLog() {
    // 防止关闭日志系统时，任务队列不为空
    TinyLog::get_instance()->async_write_log();

    if (NULL != m_fp) {
        fclose(m_fp);
    }

    if (NULL != m_buf) {
        delete[] m_buf;
    }
}

TinyLog* TinyLog::get_instance() {
    // 使用静态局部变量创建一个单例对象，能实现线程安全
    static TinyLog instance;
    return &instance;
}

void* TinyLog::flush_log_thread(void* args) {
    TinyLog::get_instance()->async_write_log();
    return NULL;
}

bool TinyLog::get_close_log() {
    return this->m_close_log;
}

void TinyLog::set_close_log(const bool& close_log) {
    this->m_close_log = close_log;
}

void TinyLog::set_dir_name(const char* dirName) {
    init(dirName);
}

bool TinyLog::init(const char* file_name, bool close_log, int log_buf_size, int split_lines, int queue_capacity) {
    m_close_log = close_log;
    m_split_lines = split_lines;
    m_log_buf_size = log_buf_size;
    m_queue_capacity = queue_capacity;
    m_buf = new char[m_log_buf_size];

    memset(m_buf, '\0', m_log_buf_size);
    memset(dir_name, '\0', sizeof(dir_name));
    memset(log_name, '\0', sizeof(log_name));

    time_t t = time(NULL);
    struct tm* sys_tm = localtime(&t); // 将 time_t 表示的时间转换为没有经过时区转换的UTC时间
    struct tm my_tm = *sys_tm;
    m_today = my_tm.tm_mday;

    // 在 file_name 所指向的字符串中搜索最后一次出现 '/' 的位置
    const char* p = strrchr(file_name, '/');
    char log_full_name[256] = {0};

    if (NULL == p) {
        strcpy(log_name, file_name);
        // 表示该日志文件在本级目录里面
        snprintf(log_full_name, 255, "%d_%02d_%02d_%s", 
                 my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, file_name);
    } else {
        // 表示该日志文件不在本级目录里面，需要把日志文件名和日志文件目录给记录下来
        strcpy(log_name, p + 1);
        strncpy(dir_name, file_name, p - file_name + 1);
        snprintf(log_full_name, 255, "%s%d_%02d_%02d_%s", 
                dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);
    }

    // fopen(打开的文件名，文件的访问模式)，"a":在尾部追加， 文件名不存在会自动创建
    m_fp = fopen(log_full_name, "a");

    if (NULL == m_fp) {
        return false;
    }

    // 如果设置了 max_queue_size，则设置为异步
    if (queue_capacity >= 1) {
        m_is_async = true;
        pthread_t tid;
        pthread_create(&tid, NULL, flush_log_thread, NULL);
        pthread_detach(tid);
    }

    return true;
}

void TinyLog::write_log(LogType type, const char* file, int32_t line, const char* format, ...) {
    struct timeval now{0, 0};
    gettimeofday(&now, NULL); // 获取当前系统时间
    time_t t = now.tv_sec;
    struct tm* sys_em = localtime(&t);
    struct tm my_tm = *sys_em;

    char typeStr[16] = {0};
    switch (type) {
    case LogType::DEBUG:
        strcpy(typeStr, "[DEBUG]: ");
        break;
    case LogType::INFO:
        strcpy(typeStr, "[INFO] : ");
        break;
    case LogType::WARN:
        strcpy(typeStr, "[WARN] : ");
        break;
    case LogType::ERROR:
        strcpy(typeStr, "[ERROR]: ");
        break;
    case LogType::FATAL:
        strcpy(typeStr, "[FATAL]: ");
        break;
    default:
        break;
    }

    // 写入一个 log
    m_mutex.lock();

    // 时间发生变化，或者 log 日志行数为最大日志行数的倍数
    if (m_today != my_tm.tm_mday || m_count % m_split_lines == 0) {
        char new_log[256] = {0};
        fflush(m_fp);
        fclose(m_fp); // 关闭流 m_fp，释放文件指针和有关的缓冲区;
        char tail[16] = {0};

        snprintf(tail, 16, "%d_%02d_%02d_", my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);

        if (m_today != my_tm.tm_mday) {
            snprintf(new_log, 255, "%s%s%s", dir_name, tail, log_name);
            m_today = my_tm.tm_mday;
            m_count = 0;
        } else {
            snprintf(new_log, 255, "%s%s%s.%lld", dir_name, tail, log_name, m_count / m_split_lines);
        }

        m_fp = fopen(new_log, "a");
    }

    m_mutex.unlock();

    // 类型 存储可变参数的信息
    va_list valst;
    // 宏定义 开始使用可变参数列表
    va_start(valst, format);

    m_mutex.lock();
    // 写入具体时间
    int n = snprintf(m_buf, 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s:%d %s", 
            my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, 
            my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_sec, file, line, typeStr);
    // 将一个可变参数（valst）格式化（format）输出到一个限定最大长度（m_log_buf_size - n - 1）的字符串缓冲区（m_buf）中
    int m = vsnprintf(m_buf + n, m_log_buf_size - n - 1, format, valst);

    m_buf[m + n] = '\n';
    m_buf[m + n + 1] = '\0';

    m_mutex.unlock();

    m_mutex.lock();
    if (m_is_async && m_log_queue.size() < m_queue_capacity) {
        m_log_queue.push(std::string(m_buf));
    } else {
        fputs(m_buf, m_fp);
        m_count++;
    }
    m_mutex.unlock();

    // 宏定义 结束使用可变参数列表
    va_end(valst);
}

void TinyLog::flush(void) {
    m_mutex.lock();
    // 刷新缓冲区，将缓冲区的数据写入 m_fp 所指的文件中
    fflush(m_fp);
    m_mutex.unlock();
}

void* TinyLog::async_write_log() {
    std::string single_log{""};

    // 从阻塞队列中取出一个日志，写入文件
    m_mutex.lock();
    while (!m_log_queue.empty()) {
        single_log = m_log_queue.front();
        m_log_queue.pop();
        fputs(single_log.c_str(), m_fp);
        m_count++;
    }
    m_mutex.unlock();

    return NULL;
}

