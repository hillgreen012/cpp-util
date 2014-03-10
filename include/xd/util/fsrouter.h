#ifndef __XD_UTIL_fsrouter_H__
#define __XD_UTIL_fsrouter_H__

#include <xd/util/strconv.h>
#include <xd/util/log.h>

#include <string>
#include <vector>
#include <sstream>

namespace g {
extern xd::util::log logger;
}

namespace xd { namespace util {

class fsrouter {
  public:
    fsrouter(const std::string& dir, 
             const std::string& prefix, 
             const std::string& ext = std::string("pcap")):
      m_dir(dir), 
      m_prefix(prefix), 
      m_ext(ext), 
      m_count(0) {
          // assert(!m_dir.empty());
          // assert(!m_prefix.empty());
          // assert(!m_ext.empty());
      }
    fsrouter(const fsrouter& orig):
      m_dir(orig.m_dir),
      m_prefix(orig.m_prefix),
      m_ext(orig.m_ext),
      m_count(0) {
      }
    fsrouter& operator=(const fsrouter& orig) {
        if (this == &orig) {
            return *this;
        }
        this->m_dir = orig.m_dir;
        this->m_prefix = orig.m_prefix;
        this->m_ext = orig.m_ext;
        this->m_count = orig.m_count;
        return *this;
    }
    std::string spawn(void) {
        m_count++;
        std::ostringstream oss;
        oss << m_dir
            << ((!m_dir.empty() && *(m_dir.rbegin()) != '/') ? "/" : "")
            << m_prefix
            << (!m_prefix.empty() ? "-" : "")
            << m_count
            << xd::util::time2string(xd::util::curtime(), "-%Y_%m_%dT%H_%M_%S")
            << (!m_ext.empty() ? "." : "")
            << m_ext;
        return oss.str();
    }

  private:
    std::string m_dir;
    std::string m_prefix;
    std::string m_ext;
    size_t m_count;
};

}
}
}
#endif // __XD_UTIL_fsrouter_H__
