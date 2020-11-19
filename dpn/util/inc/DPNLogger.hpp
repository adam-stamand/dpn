#pragma once

#define DPN_LOG_LEVEL_TRACE       SPDLOG_LEVEL_TRACE
#define DPN_LOG_LEVEL_DEBUG       SPDLOG_LEVEL_DEBUG
#define DPN_LOG_LEVEL_INFO        SPDLOG_LEVEL_INFO
#define DPN_LOG_LEVEL_WARN        SPDLOG_LEVEL_WARN
#define DPN_LOG_LEVEL_ERROR       SPDLOG_LEVEL_ERROR
#define DPN_LOG_LEVEL_CRITICAL    SPDLOG_LEVEL_CRITICAL
#define DPN_LOG_LEVEL_OFF         SPDLOG_LEVEL_OFF

#ifndef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL DPN_LOG_LEVEL
#endif

#ifdef DPN_LOGGING_ENABLED

#include <spdlog/spdlog.h>
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/fmt/bin_to_hex.h"
#include <cstdint>

#define DPN_LOGGER_TRACE(logger, ...)            SPDLOG_LOGGER_TRACE(logger.GetSPDLogger(), __VA_ARGS__)
#define DPN_LOGGER_DEBUG(logger, ...)            SPDLOG_LOGGER_DEBUG(logger.GetSPDLogger(), __VA_ARGS__)
#define DPN_LOGGER_INFO(logger, ...)             SPDLOG_LOGGER_INFO(logger.GetSPDLogger(), __VA_ARGS__)
#define DPN_LOGGER_WARN(logger, ...)             SPDLOG_LOGGER_WARN(logger.GetSPDLogger(), __VA_ARGS__)
#define DPN_LOGGER_ERROR(logger, ...)            SPDLOG_LOGGER_ERROR(logger.GetSPDLogger(), __VA_ARGS__)
#define DPN_LOGGER_CRITICAL(logger, ...)         SPDLOG_LOGGER_CRITICAL(logger.GetSPDLogger(), __VA_ARGS__)


#define DPN_SYS_TRACE(...)            SPDLOG_LOGGER_TRACE(dpn::SYSLogger::GetLogger(), __VA_ARGS__)
#define DPN_SYS_DEBUG(...)            SPDLOG_LOGGER_DEBUG(dpn::SYSLogger::GetLogger(), __VA_ARGS__)
#define DPN_SYS_INFO(...)             SPDLOG_LOGGER_INFO(dpn::SYSLogger::GetLogger(), __VA_ARGS__)
#define DPN_SYS_WARN(...)             SPDLOG_LOGGER_WARN(dpn::SYSLogger::GetLogger(), __VA_ARGS__)
#define DPN_SYS_ERROR(...)            SPDLOG_LOGGER_ERROR(dpn::SYSLogger::GetLogger(), __VA_ARGS__)
#define DPN_SYS_CRITICAL(...)         SPDLOG_LOGGER_CRITICAL(dpn::SYSLogger::GetLogger(), __VA_ARGS__)

#else

#define DPN_LOGGER_TRACE(...)            (void)0
#define DPN_LOGGER_DEBUG(...)            (void)0
#define DPN_LOGGER_INFO(...)             (void)0
#define DPN_LOGGER_WARN(...)             (void)0
#define DPN_LOGGER_ERROR(...)            (void)0
#define DPN_LOGGER_CRITICAL(...)         (void)0

#define DPN_SYS_TRACE(...)            (void)0
#define DPN_SYS_DEBUG(...)            (void)0
#define DPN_SYS_INFO(...)             (void)0
#define DPN_SYS_WARN(...)             (void)0
#define DPN_SYS_ERROR(...)            (void)0
#define DPN_SYS_CRITICAL(...)         (void)0


#endif
namespace dpn
{


class Logger
{
public:
    using LoggerName = std::string;

    
    enum class LogLevel
    {
        trace = SPDLOG_LEVEL_TRACE,
        debug = SPDLOG_LEVEL_DEBUG,
        info = SPDLOG_LEVEL_INFO,
        warn = SPDLOG_LEVEL_WARN,
        err = SPDLOG_LEVEL_ERROR,
        critical = SPDLOG_LEVEL_CRITICAL,
        off = SPDLOG_LEVEL_OFF,
        n_levels
    };

    Logger(LoggerName name, std::string fileName = "log/log.txt", LogLevel level = LogLevel::debug, std::string pattern = "%E.%f [%^%l%$]\t(%s:%#)\t<%!> {%t} %v"): 
    name_(name), logPattern_(pattern), logFileName_(fileName), logLevel_(level)
    {
        SetupLogger();
        UpdateLogger();
    }

    ~Logger()
    {
        Close();
    }

    void SetPattern(std::string pattern)
    {
        logPattern_ = pattern;
        UpdateLogger();
    }
    
    void SetLogLevel(LogLevel level)
    {
        logLevel_ = level;
        UpdateLogger();
    }

    void Close()
    {
        spdlog::drop(name_);
    }

    std::shared_ptr<spdlog::logger>& GetSPDLogger(){return logger_;}
private:

    void UpdateLogger()
    {
        consoleSink_->set_level(static_cast<spdlog::level::level_enum>(logLevel_));    
        // fileSink_->set_level(static_cast<spdlog::level::level_enum>(logLevel_));
        logger_.get()->set_level(static_cast<spdlog::level::level_enum>(logLevel_));
        
        consoleSink_->set_pattern(logPattern_);    
        // fileSink_->set_pattern(logPattern_);
        logger_.get()->set_pattern(logPattern_);
    }

    void SetupLogger()
    {
        consoleSink_ = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        
        // fileSink_ = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logFileName_, 0x1000, 10);

        logger_ = std::make_shared<spdlog::logger>(name_);
        logger_.get()->sinks().push_back(consoleSink_);
        // logger_.get()->sinks().push_back(fileSink_);
        spdlog::register_logger(logger_);
    }

    LoggerName name_;
    std::string logPattern_;
    std::string logFileName_;
    LogLevel logLevel_;
    std::shared_ptr<spdlog::logger> logger_;
    std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> consoleSink_;
    std::shared_ptr<spdlog::sinks::rotating_file_sink_mt> fileSink_;
};


class SYSLogger : public Logger
{
public:
    static std::shared_ptr<spdlog::logger>& GetLogger(){return logger_.GetSPDLogger();}

    
private:
    static Logger logger_;

};

}