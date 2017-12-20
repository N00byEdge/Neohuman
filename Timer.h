#pragma once

#include <chrono>
#include <ctime>

namespace Neolib {
struct Timer {
  void reset();
  void start();
  void pause();
  void stop();
  double lastMeasuredTime = 0;
  double highestMeasuredTime = 0;
  double avgMeasuredTime() const;

private:
#ifdef WIN32
  std::chrono::steady_clock::time_point tStart;
#else
  std::chrono::system_clock::time_point tStart;
#endif
  std::clock_t _clock;
  double totMeasuredTime = 0;
  unsigned nRuns;
};
} // namespace Neolib
