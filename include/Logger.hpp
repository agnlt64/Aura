#pragma once

#include <string>
#include <stdio.h>
#include <mutex>
#include <ctime>


#define LOG_TRACE(Message,    ...) (Logger::Trace    (__LINE__, __FILE__, Message, __VA_ARGS__))
#define LOG_INFO(Message,     ...) (Logger::Info     (__LINE__, __FILE__, Message, __VA_ARGS__))
#define LOG_DEBUG(Message,    ...) (Logger::Debug    (__LINE__, __FILE__, Message, __VA_ARGS__))
#define LOG_WARNING(Message,  ...) (Logger::Warning  (__LINE__, __FILE__, Message, __VA_ARGS__))
#define LOG_ERROR(Message,    ...) (Logger::Error    (__LINE__, __FILE__, Message, __VA_ARGS__))
#define LOG_CRITICAL(Message, ...) (Logger::Critical (__LINE__, __FILE__, Message, __VA_ARGS__))


enum TracebackLevel
{
    InfoLevel     = 0,
    DebugLevel    = 1,
    ReleaseLevel  = 2,
    WarningLevel  = 3,
    ErrorLevel    = 4,
    CriticalLevel = 5,
    DevLevel      = 5,
};


class Logger
{
public:
    static void SetPriority(TracebackLevel p)
    {
        get_instance().priority = p;
    }

    static void EnableFileOutput()
    {
        get_instance().filepath = "logs.txt";
        get_instance().enable_file_output();
    }

    static void EnableFileOutput(const char* custom_filepath)
    {
        get_instance().filepath = custom_filepath;
        get_instance().enable_file_output();
    }

    static void EnableTraceback()
    {
        get_instance().traceback = true;
    }

    static void DisableTraceback()
    {
        get_instance().traceback = false;
    }

    template <typename... Args>
    static void Trace(const char* message, Args... args)
    {
        get_instance().log("[Trace Message] ", InfoLevel, message, args...);
    }

    template <typename... Args>
    static void Info(const char* message, Args... args)
    {
        get_instance().log("[Info Message] ", InfoLevel, message, args...);
    }

    template <typename... Args>
    static void Debug(const char* message, Args... args)
    {
        get_instance().log("[Debug Message] ", DebugLevel, message, args...);
    }

    template <typename... Args>
    static void Warning(const char* message, Args... args)
    {
        get_instance().log("[Warning] ", WarningLevel, message, args...);
    }

    template <typename... Args>
    static void Error(const char* message, Args... args)
    {
        get_instance().log("[Error] ", ErrorLevel, message, args...);
    }

    template <typename... Args>
    static void Critical(const char* message, Args... args)
    {
        get_instance().log("[Critical Error] ", CriticalLevel, message, args...);
    }


    template <typename... Args>
    static void Trace(int line, const char* source_file,const char* message, Args... args)
    {
        get_instance().log(line, source_file, "[Trace Message] ", InfoLevel, message, args...);
    }

    template <typename... Args>
    static void Info(int line, const char* source_file, const char* message, Args... args)
    {
        get_instance().log(line, source_file, "[Info Message] ", InfoLevel, message, args...);
    }

    template <typename... Args>
    static void Debug(int line, const char* source_file, const char* message, Args... args)
    {
        get_instance().log(line, source_file, "[Debug Message] ", DebugLevel, message, args...);
    }

    template <typename... Args>
    static void Warning(int line, const char* source_file, const char* message, Args... args)
    {
        get_instance().log(line, source_file, "[Warning] ", WarningLevel, message, args...);
    }

    template <typename... Args>
    static void Error(int line, const char* source_file, const char* message, Args... args)
    {
        get_instance().log(line, source_file, "[Error] ", ErrorLevel, message, args...);
    }

    template <typename... Args>
    static void Critical(int line, const char* source_file, const char* message, Args... args)
    {
        get_instance().log(line, source_file, "[Critical Error] ", CriticalLevel, message, args...);
    }


private:
    TracebackLevel priority = DevLevel;
    std::mutex logMutex;
    FILE* file = nullptr;
    const char* filepath = nullptr;
    char buffer[80];
    bool traceback = false;

private:
    Logger(const Logger&) = delete;
    Logger& operator = (const Logger&) = delete;
    
    Logger() {}
    ~Logger()
    {
        free_file();
    }

    static Logger& get_instance()
    {
        static Logger logger;
        return logger;
    }

    template <typename... Args>
    void log(const char* indicator, TracebackLevel messagePriority, std::string message, Args... args)
    {
        if (priority <= messagePriority)
        {
            std::time_t current_time = time(NULL);
            std::tm* timestamp = std::localtime(&current_time);

            strftime(buffer, 80, "%c", timestamp);

            std::scoped_lock lock(logMutex);

            printf("%s\t", buffer);
            printf(indicator);
            printf(message.c_str(), args...);
            printf("\n");
        }

        if(file)
        {
            fprintf(file, "%s  ", buffer);
            fprintf(file, indicator);
            fprintf(file, message.c_str(), args...);
            fprintf(file, "\n");
        }
    }

    template <typename... Args>
    void log(int line, const char* source_file, const char* indicator, TracebackLevel messagePriority, std::string message, Args... args)
    {
        if (priority <= messagePriority)
        {
            std::time_t current_time = time(NULL);
            std::tm* timestamp = std::localtime(&current_time);

            strftime(buffer, 80, "%c", timestamp);

            std::scoped_lock lock(logMutex);

            printf("%s  ", buffer);
            printf(indicator);
            printf(message.c_str(), args...);
            if(traceback)
                printf("  Traceback: In file %s, at line %d \n", source_file, line);
            printf("\n");
        }

        if(file)
        {
            fprintf(file, "%s  ", buffer);
            fprintf(file, indicator);
            fprintf(file, message.c_str(), args...);
            if(traceback)
                fprintf(file, "Traceback: In file %s, at line %d \n", source_file, line);
            fprintf(file, "\n");
        }
    }

    void enable_file_output()
    {
        if(file != nullptr)
            fclose(file);

        file = fopen(filepath, "a");

        if(file == nullptr)
            Error("Failed to open file at path %s", filepath);
    }

    void free_file()
    {
        fclose(file);
    }
};
