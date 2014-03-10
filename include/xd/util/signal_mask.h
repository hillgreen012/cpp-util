#ifndef __XD_UTIL_SIGNAL_MASK_H__
#define __XD_UTIL_SIGNAL_MASK_H__

#include <pthread.h>

#include <csignal>
#include <cerrno>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <xd/topdef.h>

namespace xd { namespace util {

class signal_mask {
  public:
    signal_mask(void) {
        sigemptyset(&m_newmask);
        sigaddset(&m_newmask, SIGINT);
        sigaddset(&m_newmask, SIGQUIT);
        sigaddset(&m_newmask, SIGTERM);
        pthread_sigmask(SIG_BLOCK, &m_newmask, &m_origmask);
    }
    template <typename IT> 
    signal_mask(IT first, IT last) {
        sigemptyset(&m_newmask);
        for (IT it = 0; it != last; ++it) {
            if (*it < 1 || 64 < *it) {
                throw std::invalid_argument(_("Bad SIGNO"));
            }
            sigaddset(&m_newmask, *it);
        }
        (void)pthread_sigmask(SIG_BLOCK, &m_newmask, &m_origmask);
    }
    explicit signal_mask(const std::vector<int>& signals) {
        sigemptyset(&m_newmask);
        for (size_t i = 0; i < signals.size(); ++i) {
            if (signals[i] < 1 || 64 < signals[i]) {
                throw std::invalid_argument(_("Bad SIGNO"));
            }
            sigaddset(&m_newmask, signals[i]);
        }
        (void)pthread_sigmask(SIG_BLOCK, &m_newmask, &m_origmask);
    }
    ~signal_mask(void) {
        while (0 != pthread_sigmask(SIG_SETMASK, &m_origmask, NULL)) {
            continue;
        }
    }

  private:
    // non-copyable
    signal_mask(const signal_mask&);
    signal_mask& operator=(const signal_mask&);

    // signal_mask should not been created in heap
    void* operator new(size_t);
    void* operator new[](size_t);
    void operator delete(void*);
    void operator delete[](void*);

  private:
    sigset_t m_origmask;
    sigset_t m_newmask;
};

} // namespace util
} // namespace xd

#endif
