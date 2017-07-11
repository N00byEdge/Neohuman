#include "Timer.h"

void Neolib::Timer::reset() {
	tStart = std::chrono::high_resolution_clock::now();
}

void Neolib::Timer::start() {
	// Timer pause not implemented yet
}

void Neolib::Timer::pause() {
	// Timer pause not implemented yet
}

void Neolib::Timer::stop() {
	lastMeasuredTime = std::chrono::duration <double, std::milli>(std::chrono::high_resolution_clock::now() - tStart).count();
	if (lastMeasuredTime > highestMeasuredTime)
		highestMeasuredTime = lastMeasuredTime;
	++nRuns, totMeasuredTime += lastMeasuredTime;
}

double Neolib::Timer::avgMeasuredTime() const {
	return totMeasuredTime / nRuns;
}
