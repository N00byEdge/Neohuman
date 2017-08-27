#pragma once

#include "BWAPI.h"

#define SCANCOOLDOWN 100

namespace Neolib {

	class DetectionManager {
		public:
			bool requestDetection(BWAPI::Position p);
			int highestComsatEnergy();
		private:
			int scanLastUsed = -5;
	};

}

extern Neolib::DetectionManager detectionManager;
