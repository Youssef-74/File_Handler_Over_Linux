#include "Logger.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <boost/core/null_deleter.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <fstream>
#include <iostream>
#include <string>

namespace logging = boost::log;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace keywords = boost::log::keywords;

BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", logging::trivial::severity_level)



// Initialize the logger
void Log_Message(const std::string& LogMessage) {
  src::severity_logger_mt<boost::log::trivial::severity_level> logger;


  logging::formatter fmt = expr::stream
                           << std::setw(4) << std::setfill('0') << line_id
                           << std::setfill(' ') << " - "
                           << expr::format_date_time<boost::posix_time::ptime>(
                                  "TimeStamp", "%Y-%m-%d %H:%M:%S")
                           << ": [" << logging::trivial::severity << "] "
                           << expr::smessage;

  logging::add_file_log(keywords::file_name = "run_logs.log",
                        keywords::format = fmt

  );

  // Add the sink to the logger core
  logging::add_common_attributes();

  // add the App name to the logger attributs
  std::string severityString;
  std::string Message;
  // Extract the severity level from the log message
  std::string::size_type pos = LogMessage.find(':');
  if (pos != std::string::npos) {
    severityString = LogMessage.substr(0, pos);
    Message = LogMessage.substr(pos + 1);
  }  // Convert the severity level string to Boost Logger severity level
  boost::log::trivial::severity_level severityLevel = boost::log::trivial::info;
  if (severityString == "Debug") {
    severityLevel = boost::log::trivial::debug;
  } else if (severityString == "Info") {
    severityLevel = boost::log::trivial::info;
  } else if (severityString == "Warning") {
    severityLevel = boost::log::trivial::warning;
  } else if (severityString == "Error") {
    severityLevel = boost::log::trivial::error;
  } else if (severityString == "Fatal") {
    severityLevel = boost::log::trivial::fatal;
  }

  // Log the received message
  BOOST_LOG_SEV(logger, severityLevel) << Message;
}