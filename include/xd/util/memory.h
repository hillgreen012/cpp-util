#ifndef __XD_UTIL_MEMORY_H__
#define __XD_UTIL_MEMORY_H__

#include <string.h>             // for GNU strerror_r
#include <time.h>

#include <stdexcept>
#include <cstdlib>              // for ::free(3)
#include <string>
#include <cstdarg>
#include <new>
#include <memory>

#include "xd/topdef.h"

using std::string;

namespace xd { namespace util {

typedef enum {
    SIT_NONE,
    SIT_COOKED,
    SIT_GENERAL,
    SIT_VOID,
} smptr_instance_type;

namespace internal {

class refcount {
public:
    refcount() : m_left(this), m_right(this) {
        // NOTHING
    }
    ~refcount() {
        delref();
    }
    void mkref(refcount& rhs) {
        m_left = &rhs;
        m_right = rhs.m_right;
        m_left->m_right = m_right->m_left = this;
    }
    bool delref() {
        bool result = (m_left == this);
        m_right->m_left = m_left;
        m_left->m_right = m_right;
        m_left = m_right = this;
        return result;
    }

private:
    refcount(const refcount& orig);
    refcount& operator=(const refcount& orig);

private:
    refcount* volatile m_left;
    refcount* volatile m_right;
};

template <typename T, smptr_instance_type SIT>
struct gckit {
    // NOTHING
};

template <typename T>
struct gckit<T, SIT_NONE> {
    static void delref(T* obj, void*) {
        assert(obj == 0);
        return;
    }
};

template <typename T>
struct gckit<T, SIT_COOKED> {
    static void delref(T* obj, void*) {
        delete obj;
        return;
    }
};

template <typename T>
struct gckit<T, SIT_GENERAL> {
    static void delref(T* obj, void* free_funcp) {
        assert(free_funcp != 0);
        (*(reinterpret_cast<void (*)(T*)>(free_funcp)))(obj); // NOLINT
        return;
    }
};

template <typename T>
struct gckit<T, SIT_VOID> {
    static void delref(T* obj, void* free_funcp) {
        if (free_funcp != 0) {
            (*(reinterpret_cast<void (*)(void*)>(free_funcp)))(obj); // NOLINT
        }
        else {
            ::free(obj);
        }
        return;
    }
};

}  // namespace internal

template <typename T, smptr_instance_type SIT = SIT_GENERAL>
class smptr {
public:
    typedef T value_type;
    typedef T* pointer_type;
    typedef const T* const_pointer_type;

public:
    smptr(): m_obj(0), m_free_funcp(0) {
    }
    explicit smptr(T* obj): m_obj(obj), m_free_funcp(0) {
        assert(SIT_COOKED == SIT);
    }
    smptr(T* obj, void (*general_free_funcp)(T*)):
      m_obj(obj),
      m_free_funcp(reinterpret_cast<void*>(general_free_funcp)) {
        assert(SIT_GENERAL == SIT);
        assert(m_free_funcp != 0);
    }
    smptr(T* obj, void (*void_free_funcp)(void*)):
      m_obj(obj),
      m_free_funcp(reinterpret_cast<void*>(void_free_funcp)) {
        assert(SIT_VOID == SIT);
    }
    smptr(const smptr& orig):
      m_obj(orig.m_obj),
      m_free_funcp(orig.m_free_funcp) {
        m_rc.mkref(orig.m_rc);
    }
    ~smptr() {
        delref();
    }
    operator T*() const {
        return m_obj;
    }
    smptr& operator=(const smptr& orig) {
        if (&orig != this) {
            delref();
            m_obj = orig.m_obj;
            m_free_funcp = orig.m_free_funcp;
            m_rc.mkref(orig.m_rc);
        }
        return *this;
    }
    T& operator*() {
        assert(m_obj != 0);
        return *m_obj;
    }
    const T& operator*() const {
        assert(m_obj != 0);
        return *m_obj;
    }
    T* operator->() {
        assert(m_obj != 0);
        return m_obj;
    }
    const T* operator->() const {
        assert(m_obj != 0);
        return m_obj;
    }
    operator bool() const {
        return m_obj != 0;
    }
    bool operator!() const {
        return m_obj == 0;
    }
    T* get() const {
        return m_obj;
    }
    void swap(smptr& rhs) {
        smptr tmp(*this);
        *this = rhs;
        rhs = tmp;
    }
    void reset() {
        delref();
        *this = smptr();
    }

private:
    void delref() {
        if (m_rc.delref() != true) return;
        if (m_obj == 0) return;
        internal::gckit<T, SIT>::delref(m_obj, m_free_funcp);
        m_obj = 0;
        return;
    }

private:
    mutable internal::refcount m_rc;
    T* m_obj;
    void* m_free_funcp;
};

template <typename T>
class scoped_ptr {
private:
    T* m_px;
    scoped_ptr(const scoped_ptr&);
    scoped_ptr& operator=(const scoped_ptr&);
    typedef scoped_ptr<T> this_type;
    void operator==(const scoped_ptr&) const;
    void operator!=(const scoped_ptr&) const;

public:
    typedef T element_type;
    explicit scoped_ptr(T* p = 0) : m_px(0) {
    }
    explicit scoped_ptr(std::auto_ptr<T> p): m_px(p.release()) {
    }
    ~scoped_ptr() {
        delete m_px;
    }
    void reset(T* p = 0) {
        assert(p == 0 || p!= m_px);
        this_type(p).swap(*this);
    }
    T& operator*() const {
        assert(m_px != 0);
        if (m_px == 0) {
            throw std::runtime_error(_("Null pointer dereferenced"));
        }
        return *m_px;
    }
    T* operator->() const {
        assert(m_px != 0);
        if (m_px == 0) {
            throw std::runtime_error(_("Null pointer dereferenced"));
        }
        return m_px;
    }
    T* get() const {
        return m_px;
    }
    operator bool() const {
        return m_px != 0;
    }
    bool operator!() const {
        return m_px == 0;
    }
    void swap(scoped_ptr& b) {
        T* tmp = b.m_px;
        b.m_px = m_px;
        m_px = tmp;
    }
};

template <typename T> inline
void swap(scoped_ptr<T>& a, scoped_ptr<T>& b) {
    a.swap(b);
}

template <typename T> inline
T* get_pointer(const scoped_ptr<T>& p) {
    return p.get();
}

template <typename T>
class scoped_array {
private:
    T* m_px;
    scoped_array(const scoped_array&);
    scoped_array& operator=(const scoped_array&);
    typedef scoped_array<T> this_type;
    void operator==(const scoped_array&) const;
    void operator!=(const scoped_array&) const;

public:
    typedef T element_type;
    explicit scoped_array(T* p = 0) : m_px(p) {
        // NOTHING
    }
    explicit scoped_array(size_t n): m_px(new T[n]) {
        if (m_px == 0) {
            throw std::bad_alloc(_("Bad allocatation"));
        }
    }
    ~scoped_array() {
        delete[] m_px;
    }
    void reset(T* p = 0) {
        assert(p == 0 || p != m_px);
        this_type(p).swap(*this);
    }
    T& operator[](size_t i) const {
        assert(m_px != 0);
        return m_px[i];
    }
    T* get() const {
        return m_px;
    }
    operator bool() const {
        return m_px != 0;
    }
    bool operator!() const {
        return m_px == 0;
    }
    void swap(scoped_array& b) { // NOLINT
        T* tmp = b.m_px;
        b.m_px = m_px;
        m_px = tmp;
    }
};

template <typename T> inline
void swap(scoped_array<T>& a, scoped_array<T>& b) {
    a.swap(b);
}

} // namespace util
}  // namespace xd

#endif  // !__XD_UTIL_MEMORY_H__
