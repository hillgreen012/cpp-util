#ifndef __XD_UTIL_STRCONV_H__
#define __XD_UTIL_STRCONV_H__

// for strtof & strtold
#ifndef _isoc99_source
#   define _isoc99_source  1
#endif

#include <cstdlib>
#include <cassert>
#include <cmath>                // -std=99 -lm
#include <cerrno>
#include <climits>
#include <limits>
#include <string>
#include <stdexcept>

#include <xd/topdef.h>

namespace xd { namespace util {

using std::string;  // CAUTIONS

inline string ltrim(const string& orig) {
    size_t orig_size = orig.size();
    size_t i = 0;
    while (i < orig_size && isspace(orig[i])) i++;
    return orig.substr(i);
}
inline string rtrim(const string& orig) {
    size_t j = orig.size();
    while (j > 0 && isspace(orig[j-1])) j--;
    return orig.substr(0, j);
}
inline string trim(const string& orig) {
    size_t i = 0;
    size_t j = orig.size();
    while (i < j && isspace(orig[i])) i++;
    while (j > i && isspace(orig[j-1])) j--;
    return orig.substr(i, j-i);
}

template <typename t> inline
t string_to(const char* str);

template <> inline
long string_to<long>(const char* str) {
    assert(str != 0);
    if (str[0] == '\0') {
        throw std::invalid_argument(_("bad conversion for empty string"));
    }
    if (isspace(str[0])) {
        throw std::invalid_argument(_("space-padding string -- ") + std::string(str));
    }

    int errno_saved = errno;
    int errno_new = 0;
    char* endptr = 0;
    long val = 0;

    errno = 0;
    val = strtol(str, &endptr, 10);
    errno_new = errno;
    errno = errno_saved;
    if (*endptr != '\0') {
        throw std::invalid_argument(_("bad conversion for unrecogonized character -- ") +
                                    std::string(endptr));
    }
    if (errno_new != 0) {
        assert(errno_new == ERANGE);
        if (val == LONG_MAX) {
            throw std::overflow_error(_("overflow error -- ") + std::string(str));
        }
        else {
            throw std::underflow_error(_("underflow error -- ") + std::string(str));
        }
    }

    return val;
}

template <> inline
unsigned long string_to<unsigned long>(const char* str) {
    assert(str != 0);

    if (str[0] == '\0') {
        throw std::invalid_argument(_("bad conversion for empty string"));
    }
    if (isspace(str[0])) {
        throw std::invalid_argument(_("space-padding string -- ") + std::string(str));
    }

    int errno_saved = errno;
    int errno_new = 0;
    char* endptr = NULL;
    unsigned long val = 0;

    errno = 0;
    val = strtoul(str, &endptr, 10);
    errno_new = errno;
    errno = errno_saved;

    if (*endptr != '\0') {
        throw std::invalid_argument(_("bad conversion for unrecogonized character -- ") +
                                    std::string(endptr));
    }
    if (errno_new != 0) {
        assert(errno_new == ERANGE);
        throw std::overflow_error(_("overflow error -- ") + std::string(str));
    }

    return val;
}

template <> inline
long long string_to<long long>(const char* str) {
    assert(str != 0);

    if (str[0] == '\0') {
        throw std::invalid_argument(_("bad conversion for empty string"));
    }
    if (isspace(str[0])) {
        throw std::invalid_argument(_("space-padding string -- ") + std::string(str));
    }

    int errno_saved = errno;
    int errno_new = 0;
    char* endptr = NULL;
    long long val = 0;

    errno = 0;
    val = strtoll(str, &endptr, 10);
    errno_new = errno;
    errno = errno_saved;

    if (*endptr != '\0') {
        throw std::invalid_argument(_("bad conversion for unrecogonized character -- ") +
                                    std::string(endptr));
    }
    if (errno_new != 0) {
        assert(errno_new == ERANGE);
        if (val == LLONG_MAX) {
            throw std::overflow_error(_("overflow error -- ") + std::string(str));
        }
        else {
            throw std::underflow_error(_("underflow error -- ") + std::string(str));
        }
    }

    return val;
}

template <> inline
unsigned long long string_to<unsigned long long>(const char* str) {
    assert(str != 0);

    if (str[0] == '\0') {
        throw std::invalid_argument(_("bad conversion for empty string"));
    }
    if (isspace(str[0])) {
        throw std::invalid_argument(_("space-padding string -- ") + std::string(str));
    }

    int errno_saved = errno;
    int errno_new = 0;
    char* endptr = NULL;
    unsigned long long val = 0;

    errno = 0;
    val = strtoull(str, &endptr, 10);
    errno_new = errno;
    errno = errno_saved;

    if (*endptr != '\0') {
        throw std::invalid_argument(_("bad conversion for unrecogonized character -- ") +
                                    std::string(endptr));
    }
    if (errno_new != 0) {
        assert(errno_new == ERANGE);
        throw std::overflow_error(_("overflow error -- ") + std::string(str));
    }

    return val;
}

template <> inline
short string_to<short>(const char* str) {
    long val = string_to<long>(str);
    if (val > SHRT_MAX) {
        throw std::overflow_error(_("overflow error -- ") + std::string(str));
    }
    if (val < SHRT_MIN) {
        throw std::underflow_error(_("underflow error -- ") + std::string(str));
    }

    return static_cast<short>(val);
}

template <> inline
unsigned short string_to<unsigned short>(const char* str) {
    long val = string_to<long>(str);
    if (val > static_cast<long>(USHRT_MAX) ||
        val < -static_cast<long>(USHRT_MAX)) {
        throw std::overflow_error(_("overflow error -- ") + std::string(str));
    }
    return static_cast<unsigned short>(val);
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

namespace {

template <bool size_of_int_eq_long> inline
int string_to_int(const char* str);

template <> inline
int string_to_int<true>(const char* str) {
    return static_cast<int>(string_to<long>(str));
}

template <> inline
int string_to_int<false>(const char* str) {
    long val = string_to<long>(str);

    if (val > INT_MAX) {
        throw std::overflow_error(_("overflow error -- ") + std::string(str));
    }
    if (val < INT_MIN) {
        throw std::underflow_error(_("underflow error -- ") + std::string(str));
    }

    return static_cast<int>(val);
}

template <bool size_of_int_eq_long> inline
unsigned int string_to_uint(const char* str);

template <> inline
unsigned int string_to_uint<true>(const char* str) {
    return static_cast<unsigned int>(string_to<unsigned long>(str));
}

template <> inline
unsigned int string_to_uint<false>(const char* str) {
    long val = string_to<long>(str);
    if (val > static_cast<long>(UINT_MAX) ||
        val < -static_cast<long>(UINT_MAX)) {
        throw std::overflow_error(_("overflow error -- ") + std::string(str));
    }
    return static_cast<unsigned int>(val);
}

} // namespace

// ----------------------------------------------------------------------------

template <> inline
int string_to<int>(const char* str) {
    return string_to_int<sizeof(int) == sizeof(long)>(str);  // nolint
}

template <> inline
unsigned int string_to<unsigned int>(const char* str) {
    return string_to_uint<sizeof(int) == sizeof(long)>(str);  // nolint
}

#define string_to_floatpoint_specification(t, str2fp, maxval, minval)   \
    template <> inline                                                  \
    t string_to<t>(const char* str) {                                   \
        assert(str != 0);                                               \
        if (str[0] == '\0') {                                           \
            throw std::invalid_argument(_("bad conversion for empty string")); \
        }                                                               \
        if (isspace(str[0])) {                                          \
            throw std::invalid_argument(_("space-padding string -- ") + std::string(str)); \
        }                                                               \
                                                                        \
        t val = static_cast<t>(0);                                      \
        char* endptr = 0;                                               \
        int errno_saved = errno;                                        \
        int errno_new = 0;                                              \
                                                                        \
        errno = 0;                                                      \
        val = (str2fp)(str, &endptr);                                   \
        errno_new = errno;                                              \
        errno = errno_saved;                                            \
                                                                        \
        if (*endptr != '\0') {                                          \
            throw std::invalid_argument(_("bad conversion for unrecogonized character -- ") + \
                                        std::string(endptr));           \
        }                                                               \
        if (errno_new != 0) {                                           \
            assert(errno_new == ERANGE);                                \
            if (val == (maxval) || val == (minval)) {                   \
                throw std::overflow_error(_("overflow error -- ") + std::string(str)); \
            }                                                           \
            else {                                                      \
                throw std::underflow_error(_("underflow error -- ") + std::string(str)); \
            }                                                           \
        }                                                               \
                                                                        \
        return val;                                                     \
    }

string_to_floatpoint_specification(float,
                                   strtof,
                                   HUGE_VALF,     -HUGE_VALF);
string_to_floatpoint_specification(double,
                                   strtod,
                                   HUGE_VAL,      -HUGE_VAL);
#if have_long_double
string_to_floatpoint_specification(long double,
                                   strtold,
                                   HUGE_VALL,     -HUGE_VALL);
#endif

#undef string_to_floatpoint_specification

template <typename t> inline
std::string to_string(t obj);

#define to_string_specification(type, bufsize, fmtstr)                  \
    template <> inline                                                  \
    std::string to_string<type>(type obj) {                             \
        char buf[bufsize] = {0};                                        \
        int nbytes = snprintf(buf, bufsize, fmtstr, obj);               \
        if (nbytes >= bufsize || nbytes < 0) {                          \
            throw std::out_of_range(_("need more buffer to hold numeric value")); \
        }                                                               \
        return buf;                                                     \
    }
to_string_specification(short,              23,     "%hd");
to_string_specification(unsigned short,     23,     "%hu");
to_string_specification(int,                37,     "%d");
to_string_specification(unsigned int,       37,     "%u");
to_string_specification(long,               67,     "%ld");
to_string_specification(unsigned long,      67,     "%lu");
to_string_specification(long long,          83,     "%lld");
to_string_specification(unsigned long long, 83,     "%llu");
to_string_specification(float,              47,     "%g");
to_string_specification(double,             83,     "%g");
to_string_specification(long double,        203,    "%lg");
#undef to_string_specification

template <> inline
bool string_to<bool>(const char str[]) {
    if (str == 0) return false;
    bool ok, result = false;
    switch (str[0]) {
        case 0: {                 // nil
            result = false;
            ok = true;
            break;
        }
        case 'f': {              // f, false
            result = false;
            ok = (str[1] == '\0' || strcmp(str + 1, "alse") == 0);
            break;
        }
        case 'F': {              // F, False, FALSE
            result = false;
            ok = (str[1] == '\0' ||
                  strcmp(str + 1, "alse") == 0 ||
                  strcmp(str + 1, "ALSE") == 0);
            break;
        }
        case '0': {              // 0*: such as 000, 01, 001
            int i = string_to<int>(str);
            result = (i != 0);
            ok = (i == 0 || i == 1);
            break;
        }
        case '1': {              // 1
            result = true;
            ok = (str[1] == '\0');
            break;
        }
        case 't': {              // t, true
            result = true;
            ok = (str[1] == '\0' || strcmp(str + 1, "rue") == 0);
            break;
        }
        case 'T': {              // T, True, TRUE
            result = true;
            ok = (str[1] == '\0' ||
                  strcmp(str + 1, "rue") == 0 ||
                  strcmp(str + 1, "RUE") == 0);
            break;
        }
        default: {               // others
            ok = false;
            break;
        }
    }
    if (!ok) {
        throw std::invalid_argument(_("Bad convertion from string to bool -- ") + std::string(str));
    }
    return result;
}

template <> inline
std::string to_string<bool>(bool obj) {
    return obj ? "true" : "false";
}

inline string strerr(int err, char buf[], size_t len) {
    if (!buf || len < 1) {
        return _("no buffer provided for error message");
    }
    // GNU strerror_r returns error string which may be anywhere
    return ::strerror_r(err, buf, len);
}

inline time_t curtime(void) {
    time_t cur = time(NULL);
    if (cur == (time_t)-1) {
        throw std::runtime_error(_("time(2) error"));
    }
    return cur;
}

inline time_t string2time(const char* buf, const char* fmt = "%Y-%m-%dT%H:%M:%S") {
    struct tm tm;
    char* unprocessed;
    time_t t;
    std::memset(&tm, 0, sizeof(tm));
    // affeceted by TZ and LC_TIME
    unprocessed = strptime(buf, fmt, &tm);
    if (unprocessed == NULL || *unprocessed != '\0') {
        throw std::invalid_argument(std::string(buf) + _(" with ") + std::string(fmt));
    }
    if (tm.tm_isdst > 0) {
        throw std::invalid_argument(_("unsupported daylight saving time"));
    }
    t = mktime(&tm);            // affected by TZ
    if (t == (time_t)-1) {
        throw std::invalid_argument(std::string(buf));
    }
    return t;
}

template <size_t MAX_BUF_SIZE> inline
std::string time2string(time_t t, const char* fmt = "%Y-%m-%dT%H:%M:%S") {
    size_t placed;
    struct tm tm;
    char buf[MAX_BUF_SIZE];

    if (0 == localtime_r(&t, &tm)) {
        throw std::runtime_error(_("get local time error"));
    }
    // affected by TZ & LC_TIME
    placed = strftime(buf, MAX_BUF_SIZE, fmt, &tm);
    if (placed == MAX_BUF_SIZE) {
        throw std::runtime_error(_("not enough memory"));
    }
    buf[placed] = '\0';

    return buf;
}

inline std::string time2string(time_t t, const char* fmt = "%Y-%m-%dT%H:%M:%S") {
    size_t placed;
    struct tm tm;
    static const size_t MAX_BUF_SIZE = 32;
    char buf[MAX_BUF_SIZE];

    if (0 == localtime_r(&t, &tm)) {
        throw std::runtime_error(_("get local time error"));
    }
    // affected by TZ & LC_TIME
    placed = strftime(buf, MAX_BUF_SIZE, fmt, &tm);
    if (placed == MAX_BUF_SIZE) {
        throw std::runtime_error(_("not enough memory"));
    }
    buf[placed] = '\0';

    return buf;
}

} // namespace util
} // namespace xd

#endif  // !__XD_UTIL_STRCONV_H__
