#pragma once

enum MessageType
{
    MessageTypeInfo,
    MessageTypeWarning,
    MessageTypeError,
    Count
};
#define MAX_SINGLE_LOG_LENGTH 1024

void InitializeMessageLog();
void updateMessageLog(float dt);
void LogMessageImpl(MessageType type, const char* file, const char* function, uint32 line, const char* format, ...);
#define LOG_INFO(message, ...) LogMessageImpl(MessageTypeInfo, __FILE__, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#define LOG_MESSAGE(message, ...) LOG_INFO( message, __VA_ARGS__)
#define LOG_WARNING(message, ...) LogMessageImpl(MessageTypeWarning, __FILE__, __FUNCTION__, __LINE__, message, __VA_ARGS__)
#define LOG_ERROR(message, ...) LogMessageImpl(MessageTypeError, __FILE__, __FUNCTION__, __LINE__, message, __VA_ARGS__)