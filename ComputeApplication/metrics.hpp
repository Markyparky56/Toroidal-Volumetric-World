#pragma once
#include <chrono>
#include <fstream>
#include <string>
#include "syncout.hpp"

using hr_clock = std::chrono::high_resolution_clock;
using tp = hr_clock::time_point;
using std::chrono::duration_cast;
using std::chrono::microseconds;
using std::chrono::nanoseconds;

inline std::ofstream createLogFile()
{
  std::string filename;
  filename = "ComputeApp_LogMetrics_";
  filename.append(std::to_string(static_cast<int64_t>(time(NULL))));
  filename.append(".log");

  return std::ofstream(filename, std::ios::out | std::ios::trunc);
}

struct logEntryData
{
  tp  registered  // When task created 
    , start       // When task starts executing
    , heightStart // When height map is started
    , heightEnd   // When height map is finished
    , volumeStart // when volume is started
    , volumeEnd   // When volume is finished
    , surfaceStart// When surface is started
    , surfaceEnd // When surface is finished
    , end;
  uint64_t key;
  bool loadedFromCache;
};

inline void insertEntry(std::ofstream & logFile
  , logEntryData const data)
{
  if (data.loadedFromCache) return; // Cached data isn't important

  synclog(logFile) << data.key << ","          
          // Some calculated data points
          << duration_cast<nanoseconds>(data.heightEnd - data.heightStart).count()   << "," // heightElapsed
          << duration_cast<nanoseconds>(data.volumeEnd - data.volumeStart).count()   << "," // volumeElapsed
          << duration_cast<nanoseconds>(data.surfaceEnd - data.surfaceStart).count() << "," // surfaceElapsed
          << duration_cast<nanoseconds>(data.end - data.start).count()               << "," // timeElapsed
          << duration_cast<nanoseconds>(data.end - data.registered).count()          << "," // timesinceRegistered
          << std::endl; // End of entry
}
