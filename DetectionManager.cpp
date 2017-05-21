#include "DetectionManager.h"

#include "Neohuman.h"
#include "UnitManager.h"

namespace Neolib {

	bool DetectionManager::requestDetection(BWAPI::Position p) {
		// At the moment, there is only support for scans

		// Scan
		int framecount = BWAPI::Broodwar->getFrameCount();
		if (scanLastUsed + SCANCOOLDOWN > framecount)
			return false;

		BWAPI::Unit highestEnergy = nullptr;
		for (auto & u : unitManager.getFriendlyUnitsByType(BWAPI::UnitTypes::Terran_Comsat_Station))
			if (!highestEnergy || u->getEnergy() > highestEnergy->getEnergy())
				highestEnergy = u;

		if (highestEnergy == nullptr)
			return false;

		if (highestEnergy->useTech(BWAPI::TechTypes::Scanner_Sweep, p)) {
			scanLastUsed = framecount;
			return true;
		}

		return false;
	}

}
