#pragma once
#include <chrono>
namespace Stopwatch {
  inline std::chrono::time_point<std::chrono::steady_clock> start_time, end_time;
  inline std::chrono::duration<double, std::milli> duration;

  inline void start() { start_time = std::chrono::steady_clock::now(); }
  inline void end() {
    end_time = std::chrono::steady_clock::now();

    duration = end_time - start_time;
  }
}  // namespace Stopwatch