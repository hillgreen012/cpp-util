#ifndef __XD_TOPDEF_H__
#define __XD_TOPDEF_H__

#include <cstdio>
#include <ctime>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <climits>
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <list>
#include <limits>

#include "xd/configure.h"
#include "xd/debug/tracer.h"

// #ifndef __STDC__
// #   define __STDC__         1
// #   include <libpq-fe.h>        // NOLINT
// #   undef __STDC__
// #else
// #   include <libpq-fe.h>        // NOLINT
// #endif

#ifdef ENABLE_NLS
#   include <locale.h>          // NOLINT Must be before gettext() games beblow
#   include <libintl.h>         // NOLINT
#   define _(x)                         gettext(x)
#   define gettext_noop(x)              x
#   define N_(x)                        gettext_noop(x)
// #   define textdomain(DOMAIN)
// #   define bindtextdomain(DOMAIN, DIRECTORY)
#else  /* !ENABLE_NLS */
#   define _(x)                         (x)
#   define N_(x)                        x
// #   define textdomain(DOMAIN)
// #   define bindtextdomain(DOMAIN, DIRECTORY)
#endif  /* !ENABLE_NLS */

#ifndef ASSERT
#   define ASSERT assert
#endif

/**
 * Use this to mark string constants as needing translation at some later
 * time, rather than immediately.   This is useful for cases where you need
 * access to the original string and translated string, and for cases where
 * immediate translation is not possible, like when initializing global
 * variables.
 *  http://www.gnu.org/software/autoconf/manual/gettext/Special-cases.html
 */
// #define gettext_noop(x)                 (x)

/**
 * CPP_AS_STRING
 *      Convert the argument to a string, using the C preprocessor.
 * CPP_CONCAT
 *      Concatenate two arguments together, using the C preprocessor.
 *
 * Note: the standard Autoconf macro AC_C_STRINGIZE actually only checks
 * whether #identifier works, but if we have that we likely have ## too.
 */
#if defined(HAVE_STRINGIZE)
#   define CPP_AS_STRING(identifier)    #identifier
#   define CPP_CONCAT(x, y)             x##y
#else   /* !HAVE_STRINGIZE */
#   define CPP_AS_STRING(identifier)    "identifier"
/**
 * CPP_IDENTITY -- On Reiser based cpp's this is used to concatenate
 *      two tokens.  That is
 *              CPP_IDENTITY(A)B ==> AB
 *      We renamed it to _private_CppIdentity because it should not
 *      be referenced outside this file.  On other cpp's it
 *      produces  A  B.
 */
#   define _PRIV_CPP_IDENTITY(x)        x
#   define CPP_CONCAT(x, y)             _PRIV_CPP_IDENTITY(x)y

#endif   /* !HAVE_STRINGIZE */

#define MAX(x, y)                       ((x) > (y) ? (x) : (y))
#define MIN(x, y)                       ((x) < (y) ? (x) : (y))
#define ABS(x)                          ((x) >= 0 ? (x) : -(x))

/**
 * STRNCPY
 *  Like standard library function strncpy(), except that result string
 *  is guaranteed to be null-terminated --- that is, at most N-1 bytes
 *  of the source string will be kept.
 *  Also, the macro returns no result (too hard to do that without
 *  evaluating the arguments multiple times, which seems worse).
 *
 *  BTW: when you need to copy a non-null-terminated string (like a text
 *  datum) and add a null, do not do it with StrNCpy(..., len+1).  That
 *  might seem to work, but it fetches one byte more than there is in the
 *  text object.  One fine day you'll have a SIGSEGV because there isn't
 *  another byte before the end of memory.  Don't laugh, we've had real
 *  live bug reports from real live users over exactly this mistake.
 *  Do it honestly with "memcpy(dst,src,len); dst[len] = '\0';", instead.
 */
#define STRNCPY(dst, src, len)                  \
    do {                                        \
        char * _dst = (dst);                    \
        size_t _len = (len);                    \
        if (_len > 0) {                         \
            strncpy(_dst, (src), _len);         \
            _dst[_len - 1] = '\0';              \
        }                                       \
    } while (0)

/* msb for char */
#define HIGHBIT                         (0x80)
#define IS_HIGHBIT_SET(ch)              ((unsigned char)(ch) & HIGHBIT)

/* gettext domain name mangling */

/**
 * To better support parallel installations of major PostgeSQL
 * versions as well as parallel installations of major library soname
 * versions, we mangle the gettext domain name by appending those
 * version numbers.  The coding rule ought to be that whereever the
 * domain name is mentioned as a literal, it must be wrapped into
 * PG_TEXTDOMAIN().  The macros below do not work on non-literals; but
 * that is somewhat intentional because it avoids having to worry
 * about multiple states of premangling and postmangling as the values
 * are being passed around.
 *
 * Make sure this matches the installation rules in nls-global.mk.
 */

/* 
 * need a second indirection because we want to stringize the macro
 * value, not the name 
 */
#define CPP_AS_STRING2(x)               CPP_AS_STRING(x)

/**
 * debug tools
 * 1. print debugging message
 * 2. DONOT intrude in RELEASE version
 * 3. easy to search
 * 4. non-buffering
 */
#ifdef XD_DEBUG_PRINT
#   undef XD_DEBUG_PRINT
#endif

#ifdef XD_DEBUG
#   define XD_DEBUG_PRINT(sth)               \
    do {                                        \
        std::cerr << "[XD_DEBUG]: "          \
        << "("                        \
        << __FILE__                   \
        << "/" << __func__            \
        << "/" << __LINE__            \
        << "): "                      \
        << (sth)                      \
        << std::endl;                 \
    } while (0)
#else
#   define XD_DEBUG_PRINT(sth)
#endif

#endif  // __XD_TOPDEF_H__
