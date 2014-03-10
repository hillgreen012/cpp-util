#ifndef __XD_UTIL_SIGNAL_EXCEPTION_H__
#define __XD_UTIL_SIGNAL_EXCEPTION_H__

#include <stdexcept>
#include <csignal>
#include <iostream>

#include <xd/topdef.h>

namespace xd { namespace util {

template <typename SE>
class signal_transformer {
  private:
    class singleton_transformer {
      public:
        singleton_transformer() {
            struct sigaction act;
            act.sa_handler = hack_signal;
            sigemptyset(&act.sa_mask);
            act.sa_flags = 0;
            /**
             * Do not prevent the signal from being received 
             * from within its own signal handler. _this 
             * flag is only meaningful when establishing a 
             * signal handler.
             */
            act.sa_flags |= SA_NODEFER;
            if (0 > sigaction(SE::signo(), &act, NULL)) {
                throw std::runtime_error(_("signal registery error"));
            }
        }
        static void hack_signal(int signo) {
            throw SE();
        }
    };

  public:
    signal_transformer() {
        static singleton_transformer s_transformer;
    }
};

class signal_throwable {
  public:
    virtual ~signal_throwable() {
    }
};

class sigsegv_exception: public signal_throwable {
  public:
    static int signo() {
        return SIGSEGV;
    }
};

class sigint_exception: public signal_throwable {
  public:
    static int signo() {
        return SIGINT;
    }
};

class sigquit_exception: public signal_throwable {
  public:
    static int signo() {
        return SIGQUIT;
    }
};

class sigfpe_exception: public signal_throwable {
  public:
    static int signo() {
        return SIGFPE;
    }
};

class sighup_exception: public signal_throwable {
  public:
    static int signo() {
        return SIGHUP;
    }
};

class sigterm_exception: public signal_throwable {
  public:
    static int signo() {
        return SIGTERM;
    }
};

class sigusr1_exception: public signal_throwable {
  public:
    static int signo() {
        return SIGUSR1;
    }
};

class sigusr2_exception: public signal_throwable {
  public:
    static int signo() {
        return SIGUSR2;
    }
};

} // namespace util
} // namespace xd

#endif
