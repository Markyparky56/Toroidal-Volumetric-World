#pragma once
#include <stdexcept>
#include "ExceptionMessage.hpp"

#ifdef EXECPTIONS_OFF
#define throw(e) std::cerr << "Unrecoverable Exception!\n"\
                          << "What: " << e.what() << "\n"\
                          << e.message()\
                          << "\nFile: " << e.fileName()\
                          << "\nFunction: " << e.funcName()\
                          << "\nLine: " << e.lineNumber() << std::endl;
#else
#define throw(e) throw
#endif

template<class BaseExceptionType>
class UnrecoverableException : public BaseExceptionType
{
public:
  UnrecoverableException(ExceptionMessage _exceptMsg, std::string _Error)
    : BaseExceptionType(_Error), exceptMsg(_exceptMsg)
  {}
  UnrecoverableException(ExceptionMessage _exceptMsg, BaseExceptionType const &e)
    : BaseExceptionType(e), exceptMsg(_exceptMsg)
  {}

  virtual const char *message() const
  {
    return exceptMsg.message.c_str();
  }

  virtual const char *funcName() const
  {
    return exceptMsg.funcName.c_str();
  }

  virtual const char *fileName() const
  {
    return exceptMsg.fileName.c_str();
  }

  virtual const int lineNumber() const
  {
    return exceptMsg.lineNumber;
  }

  virtual const ExceptionMessage GetExceptionMessage() const
  {
    return exceptMsg;
  }

protected:
  ExceptionMessage exceptMsg;
};

using UnrecoverableVulkanException = UnrecoverableException<std::system_error>;
using UnrecoverableRuntimeException = UnrecoverableException<std::runtime_error>;
