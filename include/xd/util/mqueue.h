#ifndef __XD_UTIL_MQUEUE_H__
#define __XD_UTIL_MQUEUE_H__

#include <IceUtil/Mutex.h>
#include <IceUtil/Cond.h>

#include <deque>
#include <list>
#include <vector>
#include <limits>

namespace xd { namespace util {

template <typename T, typename Container = std::deque<T> >
class mqueue {
  public:
    mqueue(size_t volumn = std::numeric_limits<size_t>::max()):
      m_volumn(volumn),
      m_nWaitingReader(0),
      m_nWaitingWriter(0) {
    }
    T get(void) {
        IceUtil::Mutex::Lock lock(m_mutex);
        while (m_queue.empty()) {
            try {
                ++m_nWaitingReader;
                m_forNotEmpty.wait(lock);
                --m_nWaitingReader;
            } catch (...) {
                --m_nWaitingReader;
                throw;
            }
        }
        T item(*m_queue.begin());
        m_queue.erase(m_queue.begin());
        if (m_nWaitingWriter > 0) {
            m_forNotFull.broadcast();
        }
        return item;
    }
    template <typename OutputIterator>
    size_t get(OutputIterator oit, size_t n) {
        IceUtil::Mutex::Lock lock(m_mutex);
        while (m_queue.empty()) {
            try {
                ++m_nWaitingReader;
                m_forNotEmpty.wait(lock);
                --m_nWaitingReader;
            } catch (...) {
                --m_nWaitingReader;
                throw;
            }
        }
        typename Container::iterator it = m_queue.begin();
        size_t i;
        for (i = 0; i < n && it != m_queue.end(); i++) {
            *oit = *it;
            oit++;
            it++;
        }
        m_queue.erase(m_queue.begin(), it);
        if (m_nWaitingWriter > 0) {
            m_forNotFull.broadcast();
        }
        return i;
    }
    template <typename Carrier>
    void get(size_t n, Carrier* carrier) {
        assert(carrier != 0);
        IceUtil::Mutex::Lock lock(m_mutex);
        while (m_queue.empty()) {
            try {
                ++m_nWaitingReader;
                m_forNotEmpty.wait(lock);
                --m_nWaitingReader;
            } catch (...) {
                --m_nWaitingReader;
                throw;
            }
        }
        typename Container::iterator it = m_queue.begin();
        for (size_t i = 0; i < n && it != m_queue.end(); i++, it++) {
            continue;
        }
        carrier->assign(m_queue.begin(), it);
        m_queue.erase(m_queue.begin(), it);
        if (m_nWaitingWriter > 0) {
            m_forNotFull.broadcast();
        }
        return;
    }
    bool timed_get(unsigned usecs, T* item) {
        IceUtil::Time timeout =
                IceUtil::Time::microSeconds(static_cast<IceUtil::Int64>(usecs));
        bool waited = true;
        IceUtil::Mutex::Lock lock(m_mutex);
        while (waited && m_queue.empty()) {
            try {
                ++m_nWaitingReader;
                waited = m_forNotEmpty.timedWait(lock, timeout);
                --m_nWaitingReader;
            } catch (...) {
                --m_nWaitingReader;
                throw;
            }
        }
        if (waited) {
            T item(*m_queue.begin());
            m_queue.erase(m_queue.begin());
            if (m_nWaitingWriter > 0) {
                m_forNotFull.broadcast();
            }
        } 
        return waited;
    }
    /**
     * passes as 0, indicating unlimited
     * (false, _)       -- have not gotten a item for mut-ex
     * (true, false)    -- have not gotten a item for always emptiness.
     * (true, true)     -- bingo
     */
    std::pair<bool, bool> timed_get(unsigned usecs, unsigned passes, T* item) {
        if (passes == 0) {
            return std::make_pair<bool, bool>(timed_get(usecs, item), true);
        }
        IceUtil::Time timeout =
                IceUtil::Time::microSeconds(static_cast<IceUtil::Int64>(usecs));
        bool waited = true;
        unsigned pass = 0;
        IceUtil::Mutex::Lock lock(m_mutex);
        while (waited && m_queue.empty() && pass < passes) {
            try {
                ++m_nWaitingReader;
                waited = m_forNotEmpty.timedWait(lock, timeout);
                --m_nWaitingReader;
            } catch (...) {
                --m_nWaitingReader;
                throw;
            }
            pass++;
        }
        if (waited && !m_queue.empty()) {
            T item(*m_queue.begin());
            m_queue.erase(m_queue.begin());
            if (m_nWaitingWriter > 0) {
                m_forNotFull.broadcast();
            }
            // no matter whether pass is less than passes or not.
            return std::make_pair<bool, bool>(true, true);
        }
        else {
            return std::make_pair<bool, bool>(waited, pass < passes);
        }
    }
    template <typename Carrier>
    bool timed_get(unsigned usecs, size_t n, Carrier* carrier) {
        assert(carrier != 0);
        IceUtil::Time timeout =
                IceUtil::Time::microSeconds(static_cast<IceUtil::Int64>(usecs));
        bool waited = true;
        IceUtil::Mutex::Lock lock(m_mutex);
        while (waited && m_queue.empty()) {
            try {
                ++m_nWaitingReader;
                waited = m_forNotEmpty.timedWait(lock, timeout);
                --m_nWaitingReader;
            } catch (...) {
                --m_nWaitingReader;
                throw;
            }
        }
        if (waited) {
            typename Container::iterator it = m_queue.begin();
            for (size_t i = 0; i < n; i++) {
                if (it == m_queue.end()) break;
                it++;
            }
            carrier->assign(m_queue.begin(), it);
            m_queue.erase(m_queue.begin(), it);
            if (m_nWaitingWriter > 0) {
                m_forNotFull.broadcast();
            }
        } 
        return waited;
    }
    template <typename Carrier>
    std::pair<bool, bool> timed_get(unsigned usecs, unsigned passes, size_t n, Carrier* carrier) {
        if (passes == 0) {
            return std::make_pair<bool, bool>(timed_get(usecs, n, carrier), true);
        }
        assert(carrier != 0);
        IceUtil::Time timeout =
                IceUtil::Time::microSeconds(static_cast<IceUtil::Int64>(usecs));
        bool waited = true;
        unsigned pass = 0;
        IceUtil::Mutex::Lock lock(m_mutex);
        while (waited && m_queue.empty() && pass < passes) {
            try {
                ++m_nWaitingReader;
                waited = m_forNotEmpty.timedWait(lock, timeout);
                --m_nWaitingReader;
            } catch (...) {
                --m_nWaitingReader;
                throw;
            }
            pass++;
        }
        if (waited && !m_queue.empty()) {
            typename Container::iterator it = m_queue.begin();
            for (size_t i = 0; i < n; i++) {
                if (it == m_queue.end()) break;
                it++;
            }
            carrier->assign(m_queue.begin(), it);
            m_queue.erase(m_queue.begin(), it);
            if (m_nWaitingWriter > 0) {
                m_forNotFull.broadcast();
            }
            return std::make_pair<bool, bool>(true, true);
        }
        else {
            return std::make_pair<bool, bool>(waited, pass < passes);
        }
    }
    template <typename OutputIterator>
    bool timed_get(unsigned usecs, OutputIterator oit, size_t n, size_t* m = 0) {
        IceUtil::Time timeout =
                IceUtil::Time::microSeconds(static_cast<IceUtil::Int64>(usecs));
        bool waited = true;
        IceUtil::Mutex::Lock lock(m_mutex);
        while (waited && m_queue.empty()) {
            try {
                ++m_nWaitingReader;
                waited = m_forNotEmpty.timedWait(lock, timeout);
                --m_nWaitingReader;
            } catch (...) {
                --m_nWaitingReader;
                throw;
            }
        }
        if (waited) {
            typename Container::iterator it = m_queue.begin();
            size_t i;
            for (i = 0; i < n && it != m_queue.end(); i++) {
                *oit = *it;
                oit++;
                it++;
            }
            m_queue.erase(m_queue.begin(), it);
            if (m) *m = i;
            if (m_nWaitingWriter > 0) {
                m_forNotFull.broadcast();
            }
        } 
        return waited;
    }
    template <typename OutputIterator>
    std::pair<bool, bool> timed_get(unsigned usecs, unsigned passes, 
                                    OutputIterator oit, size_t n, size_t* m = 0) {
        if (passes = 0) {
            return std::make_pair<bool, bool>(time_get(usecs, oit, n, m), true);
        }
        IceUtil::Time timeout = IceUtil::Time::microSeconds(static_cast<IceUtil::Int64>(usecs));
        bool waited = true;
        unsigned pass = 0;
        IceUtil::Mutex::Lock lock(m_mutex);
        while (waited && m_queue.empty() && pass < passes) {
            try {
                ++m_nWaitingReader;
                waited = m_forNotEmpty.timedWait(lock, timeout);
                --m_nWaitingReader;
            } catch (...) {
                --m_nWaitingReader;
                throw;
            }
        }
        if (waited && !m_queue.empty()) {
            typename Container::iterator it = m_queue.begin();
            size_t i;
            for (i = 0; i < n && it != m_queue.end(); i++) {
                *oit = *it;
                oit++;
                it++;
            }
            m_queue.erase(m_queue.begin(), it);
            if (m) *m = i;
            if (m_nWaitingWriter > 0) {
                m_forNotFull.broadcast();
            }
	    return std::make_pair<bool, bool>(true, true);
        } 
        else {
            return std::make_pair<bool, bool>(waited, pass < passes);
        }
    }
    void put(const T& item) {
        IceUtil::Mutex::Lock lock(m_mutex);
        while (m_queue.size() >= m_volumn) {
            try {
                ++m_nWaitingWriter;
                m_forNotFull.wait(lock);
                --m_nWaitingWriter;
            } catch (...) {
                --m_nWaitingWriter;
                throw;
            }
        }
        m_queue.push_back(item);
        if (m_nWaitingReader > 0) {
            m_forNotEmpty.broadcast();
        }
        return;
    }
    bool timed_put(unsigned usecs, const T& item) {
        IceUtil::Time timeout =
                IceUtil::Time::microSeconds(static_cast<IceUtil::Int64>(usecs));
        bool waited = true;
        IceUtil::Mutex::Lock lock(m_mutex);
        while (waited && m_queue.size() >= m_volumn) {
            try {
                ++m_nWaitingWriter;
                m_forNotFull.wait(lock);
                --m_nWaitingWriter;
            } catch (...) {
                --m_nWaitingWriter;
                throw;
            }
        }
        if (waited) {
            m_queue.push_back(item);
            if (m_nWaitingReader > 0) {
                m_forNotEmpty.broadcast();
            }
        }
        return waited;
    }
    std::pair<bool, bool> timed_put(unsigned usecs, unsigned passes, const T& item) {
        if (passes == 0) {
            return std::make_pair<bool, bool>(timed_put(usecs, item), true);
        }
        IceUtil::Time timeout =
                IceUtil::Time::microSeconds(static_cast<IceUtil::Int64>(usecs));
        bool waited = true;
        unsigned pass = 0;
        IceUtil::Mutex::Lock lock(m_mutex);
        while (waited && m_queue.size() >= m_volumn && pass < passes) {
            try {
                ++m_nWaitingWriter;
                m_forNotFull.wait(lock);
                --m_nWaitingWriter;
            } catch (...) {
                --m_nWaitingWriter;
                throw;
            }
            pass++;
        }
        if (waited && m_queue.size() < m_volumn) {
            m_queue.push_back(item);
            if (m_nWaitingReader > 0) {
                m_forNotEmpty.broadcast();
            }
            return std::make_pair<bool, bool>(true, true);
        }
        else {
            return std::make_pair<bool, bool>(waited, pass < passes);
        }
    }
    template <typename Carrier>
    void put(const Carrier& carrier) {
        IceUtil::Mutex::Lock lock(m_mutex);
        while (m_queue.size() >= m_volumn) {
            try {
                ++m_nWaitingWriter;
                m_forNotFull.wait(lock);
                --m_nWaitingWriter;
            }
            catch (...) {
                --m_nWaitingWriter;
                throw;
            }
        }
        m_queue.insert(m_queue.end(), carrier.begin(), carrier.end());
        if (m_nWaitingReader > 0) {
            m_forNotEmpty.broadcast();
        }
        return;
    }
    template <typename Carrier>
    bool timed_put(unsigned usecs, const Carrier& carrier) {
        IceUtil::Time timeout =
                IceUtil::Time::microSeconds(static_cast<IceUtil::Int64>(usecs));
        bool waited = true;
        IceUtil::Mutex::Lock lock(m_mutex);
        while (waited && m_queue.size() >= m_volumn) {
            try {
                ++m_nWaitingWriter;
                m_forNotFull.wait(lock);
                --m_nWaitingWriter;
            } catch (...) {
                --m_nWaitingWriter;
                throw;
            }
        }
        if (waited) {
            m_queue.insert(m_queue.end(), carrier.begin(), carrier.end());
            if (m_nWaitingReader > 0) {
                m_forNotEmpty.broadcast();
            }
        }
        return waited;
    }
    template <typename Carrier>
    std::pair<bool, bool> timed_put(unsigned usecs, unsigned passes, const Carrier& carrier) {
        if (passes == 0) {
            return std::make_pair<bool, bool>(timed_put(usecs, carrier), true);
        }
        IceUtil::Time timeout =
                IceUtil::Time::microSeconds(static_cast<IceUtil::Int64>(usecs));
        bool waited = true;
        unsigned pass = 0;
        IceUtil::Mutex::Lock lock(m_mutex);
        while (waited && m_queue.size() >= m_volumn && pass < passes) {
            try {
                ++m_nWaitingWriter;
                m_forNotFull.wait(lock);
                --m_nWaitingWriter;
            } catch (...) {
                --m_nWaitingWriter;
                throw;
            }
            pass++;
        }
        if (waited && m_queue.size() < m_volumn) {
            m_queue.insert(m_queue.end(), carrier.begin(), carrier.end());
            if (m_nWaitingReader > 0) {
                m_forNotEmpty.broadcast();
            }
            return std::make_pair<bool, bool>(true, true);
        }
        else {
            return std::make_pair<bool, bool>(waited, pass < passes);
        }
    }
    template <typename InputIterator>
    void put(InputIterator first, InputIterator last) {
        IceUtil::Mutex::Lock lock(m_mutex);
        while (m_queue.size() >= m_volumn) {
            try {
                ++m_nWaitingWriter;
                m_forNotFull.wait(lock);
                --m_nWaitingWriter;
            } catch (...) {
                --m_nWaitingWriter;
                throw;
            }
        }
        m_queue.insert(m_queue.end(), first, last);
        if (m_nWaitingReader > 0) {
            m_forNotEmpty.broadcast();
        }
        return;
    }
    template <typename InputIterator>
    bool timed_put(unsigned int usecs, InputIterator first, InputIterator last) {
        IceUtil::Time timeout =
                IceUtil::Time::microSeconds(static_cast<IceUtil::Int64>(usecs));
        bool waited = true;
        IceUtil::Mutex::Lock lock(m_mutex);
        while (waited && m_queue.size() >= m_volumn) {
            try {
                ++m_nWaitingWriter;
                m_forNotFull.wait(lock);
                --m_nWaitingWriter;
            } catch (...) {
                --m_nWaitingWriter;
                throw;
            }
        }
        if (waited) {
            m_queue.insert(m_queue.end(), first, last);
            if (m_nWaitingReader > 0) {
                m_forNotEmpty.broadcast();
            }
        }
        return waited;
    }
    template <typename InputIterator>
    std::pair<bool, bool> timed_put(unsigned usecs, unsigned passes, InputIterator first, InputIterator last) {
        if (passes == 0) {
            return std::make_pair<bool, bool>(timed_put(usecs, first, last), true);
        }
        IceUtil::Time timeout =
                IceUtil::Time::microSeconds(static_cast<IceUtil::Int64>(usecs));
        bool waited = true;
        unsigned pass = 0;
        IceUtil::Mutex::Lock lock(m_mutex);
        while (waited && m_queue.size() >= m_volumn && pass < passes) {
            try {
                ++m_nWaitingWriter;
                m_forNotFull.wait(lock);
                --m_nWaitingWriter;
            } catch (...) {
                --m_nWaitingWriter;
                throw;
            }
        }
        if (waited && m_queue.size() < m_volumn) {
            m_queue.insert(m_queue.end(), first, last);
            if (m_nWaitingReader > 0) {
                m_forNotEmpty.broadcast();
            }
            return std::make_pair<bool, bool>(true, true);
        }
        else {
            return std::make_pair<bool, bool>(waited, pass < passes);
        }
    }

  private:
    unsigned long m_volumn;
    Container m_queue;
    short m_nWaitingWriter;
    short m_nWaitingReader;

    IceUtil::Mutex m_mutex;
    IceUtil::Cond m_forNotFull;
    IceUtil::Cond m_forNotEmpty;
};

} // namespace util
} // namespace xd

#endif  // !__XDUTIL_MQUEUE_H__
