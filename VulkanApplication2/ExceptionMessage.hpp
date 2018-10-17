#pragma once
#include <string>

class ExceptionMessage
{
public:
  ExceptionMessage( std::string _message
                  , std::string _funcName
                  , std::string _fileName
                  , int _lineNumber
                  , void *_dataPtr)
    : message(_message)
    , funcName(_funcName)
    , fileName(_fileName)
    , lineNumber(_lineNumber)
    , dataPtr(_dataPtr)
  {}

  // Exception Message, try to be brief but informative
  std::string message;

  // Function name, the specific function which threw the exception (__func__)
  std::string funcName;

  // File name (__FILE__)
  std::string fileName;

  // Line number (__LINE__)
  int lineNumber;

  // Pointer to something relevant, not necessarily required
  void *dataPtr;
};

#define CreateBasicExceptionMessage(MSG) ExceptionMessage(MSG, __func__, __FILE__, __LINE__, nullptr)
