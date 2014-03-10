#include <cstdlib>

#include <xd/debug/tracer.h>

namespace xd { namespace debug {

namespace {

const char *Tracepath = std::getenv("TRACE_PATH") != NULL ? std::getenv("TRACE_PATH") : "/dev/null";
std::ofstream fout(Tracepath);

}

tracer::channel tracer::s_channel(&fout);

} // namespace debug
} // namespace xd
