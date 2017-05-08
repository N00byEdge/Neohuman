#pragma once

#include <ctime>
#include <chrono>

namespace Neolib {
	class Timer {
		public:
			void reset();
			void stop();
			double lastMeasuredTime = 0;
		private:
			std::chrono::system_clock::time_point tStart;
			std::clock_t _clock;
	};
}
