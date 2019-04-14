#pragma once
#include <mutex>
#include <iostream>
#include <fstream>

static std::mutex outputMutex;

// credit: https://stackoverflow.com/a/45046349/1941205
struct syncout {
  std::unique_lock<std::mutex> lock;

  syncout()
    : lock(outputMutex)
  {}

  template<typename T>
  syncout & operator<<(const T& t)
  {
    std::cout << t;
    return *this;
  }

  syncout & operator<<(std::ostream&(*fp)(std::ostream&))
  {
    std::cout << fp;
    return *this;
  }
};

static std::mutex logMutex;
// Takes a file to output to
struct synclog {
  std::unique_lock<std::mutex> lock;
  std::ostream &log;

  synclog(std::ofstream & log)
    : lock(logMutex)
    , log(log)
  {}

  template<typename T>
  synclog & operator<<(const T& t)
  {
    log << t;
    return *this;
  }

  synclog & operator<<(std::ostream&(fp)(std::ostream&))
  {
    log << fp;
    return *this;
  }
};
