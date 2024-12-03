#include "logger.h"
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

#include <string_view>

namespace Logger {

using namespace std::literals;
namespace logging = boost::log;
namespace keywords = boost::log::keywords;
namespace sinks = boost::log::sinks;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(data, "Data", std::string_view)
BOOST_LOG_ATTRIBUTE_KEYWORD(msg, "Msg", std::string_view)


static void JsonFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) { 
    auto ts = *rec[timestamp];
    strm << "{\"timestamp\":\"" << to_iso_extended_string(ts) << "\",";
    strm << "\"data\":" << rec[data] << ",";
    strm << "\"message\":\"" << rec[msg] << "\"}" << std::endl;
}

void InitBoostLogFilter() {
    logging::add_common_attributes();
    logging::add_console_log(
        std::cout,
        keywords::format = &JsonFormatter,
        keywords::auto_flush = true
    );
}

void info(std::string_view data_, std::string_view message_) {
    BOOST_LOG_TRIVIAL(info)
        << logging::add_value(data, data_)
        << logging::add_value(msg, message_);
}

void LogStart(const net::ip::address& address, net::ip::port_type port) {
    info(CreateLogMessage(ServerAddressLogData(address.to_string(), port)), LogMsg::SERVER_START);
}

void LogError(beast::error_code ec, std::string_view what){
    info(CreateLogMessage(ExceptionLogData(ec.value(),ec.message(),what)), LogMsg::ERROR);    
}

void LogStop(const std::exception& ex) {
    info(CreateLogMessage(ServerStopLogData(EXIT_FAILURE, ex.what())),  LogMsg::SERVER_STOP);
}

void LogStop(int code){
    info(CreateLogMessage(ServerStopLogData(code)), LogMsg::SERVER_STOP);    
}

}