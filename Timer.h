#pragma once

#include <ctime>
#include <chrono>

namespace Neolib {
	class Timer {
		public:
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
}
