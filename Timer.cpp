#include "Timer.h"

void Neolib::Timer::reset() {
	tStart = std::chrono::high_resolution_clock::now();
}

void Neolib::Timer::stop() {
	lastMeasuredTime = std::chrono::duration <double, std::milli>(std::chrono::high_resolution_clock::now() - tStart).count();
}
