#ifndef Logger_h__
#define Logger_h__

#include <boost/log/trivial.hpp>
#include <boost/log/sources/global_logger_storage.hpp>

// the logs are also written to LOGFILE
#define LOGFILE "../run_logs.log"

// just log messages with severity >= SEVERITY_THRESHOLD are written
#define SEVERITY_THRESHOLD logging::trivial::warning

void Log_Message(const std::string& LogMessage);

#endif