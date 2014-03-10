#include <xd/util/log.h>

namespace xd { namespace util {

const unsigned log::MAX_FILE_SIZE;
const unsigned log::MAX_ITEM_LENGTH;
const unsigned log::CHANGE_FILE_NAME_INTERVAL;
const unsigned log::MAX_CACHE_SIZE;
const char* const log::DEFAULT_TIME_STRING_FORMAT = "%Y-%m-%dT%H-%M-%S";
const char log::DEFAULT_FIELD_SEPERATOR;
const unsigned log::DEFAULT_FLUSH_INTERVAL;

volatile int log::FLUSH_INTERVAL = log::DEFAULT_FLUSH_INTERVAL;

} // namespace util
} // namespace xd
