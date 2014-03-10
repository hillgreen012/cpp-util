#ifndef __XD_UTIL_LOG_H__
#define __XD_UTIL_LOG_H__

#include <IceUtil/Mutex.h>
#include <IceUtil/Thread.h>
#include <IceUtil/Time.h>
#include <pthread.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>

#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdexcept>

#include <xd/topdef.h>
#include <xd/util/strconv.h>

namespace xd { namespace util {

class log: public IceUtil::Thread {
  public:
    static const unsigned MAX_FILE_SIZE = 1*512*1024*1024;      // 512M
    static const unsigned MAX_ITEM_LENGTH = 2 * 1024;           // 2K, as varchar2
    static const unsigned CHANGE_FILE_NAME_INTERVAL = 60 * 60;  // 1 hour
    static const unsigned MAX_CACHE_SIZE = 1024;
    static const char* const DEFAULT_TIME_STRING_FORMAT;
    static const char DEFAULT_FIELD_SEPERATOR = ':';
    static const unsigned DEFAULT_FLUSH_INTERVAL = 5;          // in second
    static volatile int FLUSH_INTERVAL;

  public:
    typedef enum {
        OFF = 0,
        ERROR,
        WARN,
        INFO,
        DEBUG,
        ALL,
    } level_type;

  public:
    static const char* level2string(level_type level) {
        static const char* const level_names[] = {"OFF", "ERROR", "WARN", "INFO", "DEBUG", "ALL"};
        return level_names[level];
    }
    static level_type string2level(const std::string& level_name) {
        if (strcasecmp("OFF", level_name.c_str()) == 0) {
            return OFF;
        }
        else if (strcasecmp("ERROR", level_name.c_str()) == 0) {
            return ERROR;
        }
        else if (strcasecmp("WARN", level_name.c_str()) == 0) {
            return WARN;
        }
        else if (strcasecmp("INFO", level_name.c_str()) == 0) {
            return INFO;
        }
        else if (strcasecmp("DEBUG", level_name.c_str()) == 0) {
            return DEBUG;
        }
        else if (strcasecmp("ALL", level_name.c_str()) == 0) {
            return ALL;
        }
        else {
            throw std::invalid_argument(_("invalid level name") + std::string(" -- ") + level_name);
        }
    }

  public:
    log(const std::string& log_path,
        const std::string& log_name,
        const std::string& app_name,
        level_type level = INFO,
        bool print2screen_flag = false):
      m_log_path(log_path),
      m_log_name(log_name),
      m_app_name(app_name),
      m_level(level),
      m_print2screen_flag(print2screen_flag),
      m_loop_flag(1) {
          time_t t = curtime();
          m_last_log_name = next_log_name(t);
          m_last_log_fos.open(m_last_log_name.c_str());
          if (!m_last_log_fos.good()) {
              throw std::runtime_error(_("open file error") + std::string(" -- ") + m_last_log_name);
          }
          m_last_log_size = 0;
          m_last_log_time = t;
      }
    ~log() {
        try {
            flush_cache();
        }
        catch (...) {
            // NOTHING
        }
        if (m_last_log_fos.is_open()) {
            m_last_log_fos.close();
            m_last_log_fos.clear();
        }
    }
#ifdef LOG_FUNCTION_SPECIFICATION
#   undef LOG_FUNCTION_SPECIFICATION
#endif
#define LOG_FUNCTION_SPECIFICATION(func, level) \
    void func(const char* format, ...) {        \
        va_list ap;                             \
        va_start(ap, format);                   \
        try {                                   \
            record(level, format, ap);          \
        }                                       \
        catch (...) {                           \
            va_end(ap);                         \
            throw;                              \
        }                                       \
        va_end(ap);                             \
        return;                                 \
    }
    LOG_FUNCTION_SPECIFICATION(error, ERROR);
    LOG_FUNCTION_SPECIFICATION(warn, WARN);
    LOG_FUNCTION_SPECIFICATION(info, INFO);
    LOG_FUNCTION_SPECIFICATION(debug, DEBUG);
#undef LOG_FUNCTION_SPECIFICATION
    virtual void run(void) {
        while (m_loop_flag) {
            try {
                sleep();
                IceUtil::Mutex::Lock lock(m_mutex);
                flush_cache();
            }
            catch (const IceUtil::Exception& e) {
                std::clog << compose(ERROR, "%s|%ld|%s", __func__, __LINE__, e.what()).second;
            }
            catch (const std::exception& e) {
                std::clog << compose(ERROR, "%s|%ld|%s", __func__, __LINE__, e.what()).second;
            }
            catch (...) {
                std::clog << compose(ERROR, "%s|%ld|%s", __func__, __LINE__, _("unknown exception")).second;
            }
        }
    }
    void stop(void) {
        m_loop_flag = 0;
    }

  private:
    void record(level_type level, const char* format, va_list ap) {
        if (level > m_level) {
            return;
        }

        std::pair<time_t, std::string> item = vcompose(level, format, ap);

        if (m_print2screen_flag) {
            std::clog << item.second;
        }

        IceUtil::Mutex::Lock lock(m_mutex);

        m_cache.push_back(item);

        if (m_cache.size() > MAX_CACHE_SIZE) {
            flush_cache();
        }

        return;
    }
    std::pair<time_t, std::string> compose(level_type level, const char* format, ...) {
        va_list ap;
        va_start(ap, format);
        std::pair<time_t, std::string> item;
        try {
            item = vcompose(level, format, ap);
        }
        catch (...) {
            va_end(ap);
            throw;
        }
        va_end(ap);
        return item;
    }
    std::pair<time_t, std::string> vcompose(level_type level, const char* format, va_list ap) {
        size_t len;
        char item_buffer[MAX_ITEM_LENGTH] = {'\0'};
        len = static_cast<size_t>(vsnprintf(item_buffer, MAX_ITEM_LENGTH, format, ap));
        if (len >= MAX_ITEM_LENGTH) {
            item_buffer[MAX_ITEM_LENGTH - 1] = '\0';
            len = MAX_ITEM_LENGTH - 1;
        }

        time_t now = curtime();
        std::ostringstream sos;

        static unsigned pid = ::getpid();
        unsigned tid = ::pthread_self();

        sos << time2string(now, DEFAULT_TIME_STRING_FORMAT)
            << DEFAULT_FIELD_SEPERATOR
            << m_app_name
            << DEFAULT_FIELD_SEPERATOR
            << pid
            << DEFAULT_FIELD_SEPERATOR
            << tid
            << DEFAULT_FIELD_SEPERATOR
            << level2string(level)
            << DEFAULT_FIELD_SEPERATOR
            << item_buffer
            << std::endl;

        return std::make_pair<time_t, std::string>(now, sos.str());
    }
    void sleep(void) {
        getThreadControl().sleep(IceUtil::Time::seconds(static_cast<IceUtil::Int64>(FLUSH_INTERVAL)));
    }
    void flush_cache(void) {
        size_t cache_size = m_cache.size();
        for (size_t i = 0; i < cache_size; i++) {
            const time_t& current = m_cache[i].first;
            const std::string& item = m_cache[i].second;
            if (current >= m_last_log_time + CHANGE_FILE_NAME_INTERVAL ||
                m_last_log_size + item.size() > MAX_FILE_SIZE) {
                m_last_log_fos.close();
                m_last_log_fos.clear();
                m_last_log_name = next_log_name(current);
                m_last_log_fos.open(m_last_log_name.c_str());
                if (!m_last_log_fos.good()) {
                    throw std::runtime_error(_("open file error") + std::string(" -- ") + m_last_log_name);
                }
                m_last_log_size = 0;
                m_last_log_time = current;
            }
            m_last_log_fos << item;
            if (!m_last_log_fos.good()) {
                throw std::runtime_error(_("write file error") + std::string(" -- ") + m_last_log_name);
            }
            m_last_log_size += item.size();
        }
        m_cache.clear();
        if (m_last_log_fos.good()) {
            m_last_log_fos.flush();
        }
        return;
    }
    std::string next_log_name(const time_t& t) {
        std::ostringstream sos;
        sos << m_log_path 
            << (*m_log_path.rbegin() == '/' ? "" : "/")
            << m_log_name
            << '_'
            << m_app_name
            << '_'
            << time2string(t, DEFAULT_TIME_STRING_FORMAT)
            << ".log";
        return sos.str();
    }

  private:
    std::string m_log_path;
    std::string m_log_name;
    std::string m_app_name;
    level_type m_level;
    std::ofstream   m_last_log_fos;
    std::string     m_last_log_name;
    size_t          m_last_log_size;
    time_t          m_last_log_time;
    bool m_print2screen_flag;
    std::vector<std::pair<time_t, std::string> > m_cache;
    IceUtil::Mutex m_mutex;
    volatile int m_loop_flag;
};

}      // namespace util
}      // namespace xd

#endif  // !__XD_UTIL_LOG_H__
