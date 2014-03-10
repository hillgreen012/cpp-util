#ifndef __XD_DEBUG_TRACER_H__
#define __XD_DEBUG_TRACER_H__

#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <string>
#include <cassert>
#include <cstdlib>

#ifdef DEBUG_TRACE
#   ifdef TRACE_MINE
#       undef TRACE_MINE
#   endif
#   define TRACE_MINE(id) XD::Debug::tracer tracer_##id(__func__, __FILE__, __LINE__)
#else
#   ifdef TRACE_MINE
#       undef TRACE_MINE
#   endif
#   define TRACE_MINE(id)
#endif

namespace xd { namespace debug {

class tracer {
  private:
    class channel {
      public:
        explicit channel(std::ostream* orig_os = &std::cout): m_os(orig_os) {
        }
        void reset(std::ostream* orig_os = new std::ofstream("/dev/null")) {
            if (orig_os != m_os) {
                m_os->flush();
                m_os->clear();
                m_os = orig_os;
            }
            return;
        }
        std::ostream& os() const {
            assert(m_os != 0);
            return *m_os;
        }

      private:
        std::ostream* m_os;
    };                          // class channel
    enum {
        FUNC_WIDTH = 30,
        FILE_WIDTH = 20,
        LINE_WIDTH = 5,
        OBJ_WIDTH = 5,
    };

  public:
    tracer(const char* orig_func, const char* orig_file, unsigned orig_line):
      m_func(orig_func),
      m_file(orig_file),
      m_line(orig_line),
      m_chan(tracer::s_channel) {
          if (m_chan.os()) {
              m_chan.os() << "E:"
                  << std::left << std::setw(FUNC_WIDTH) << m_func
                  << std::right
                  << ':'
                  << std::setw(FILE_WIDTH) << m_file
                  << ":" << m_line
                  << std::endl;
          }
      }
    ~tracer() {
        if (m_chan.os()) {
            m_chan.os() << "X:"
                << std::left << std::setw(FUNC_WIDTH)
                << m_func
                << std::right
                << ':';
            if (m_file) {
                m_chan.os() << std::setw(FILE_WIDTH) << m_file;
            }
            else {
                m_chan.os() << std::setw(FILE_WIDTH) << " ";
            }
            m_chan.os() << ":";
            if (m_line > 0) {
                m_chan.os() << m_line;
                m_chan.os() << std::endl;
            }
        }
    }
    static void reset(std::ostream* orig_os = new std::ofstream("/dev/null")) {
        s_channel.reset(orig_os);
        return;
    }

  private:
    static channel s_channel;

  private:
    const char* m_func;
    const char* m_file;
    const unsigned m_line;
    channel& m_chan;
};                              // class tracer

}  // namespace debug;
}  // namespace xd

#endif  // !__XD_DEBUG_TRACER_H__
