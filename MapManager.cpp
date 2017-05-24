#include "MapManager.h"

#include "UnitManager.h"
#include "DetectionManager.h"

//#include <algorithm>

BWAPI::TilePosition startPos;

Neolib::MapManager mapManager;

struct sortLocations {
	static bool fromStartPos(const BWEM::Base *lhs, const BWEM::Base *rhs) {
		return lhs->Location().getApproxDistance(startPos) < rhs->Location().getApproxDistance(startPos);
	}
};

namespace Neolib {

	void MapManager::init() {
		BWEM::Map::Instance().Initialize();
		BWEM::Map::Instance().EnableAutomaticPathAnalysis();

		for (auto &a : BWEM::Map::Instance().Areas())
			for (auto &b : a.Bases())
				allBases.push_back(&b);

		for (auto &b : allBases) {
			if (BWAPI::Broodwar->isVisible(b->Location())) {
				startPos = b->Location();
				std::sort(allBases.begin(), allBases.end(), sortLocations::fromStartPos);
			}
		}
	}

	void MapManager::onFrame() {
		for (auto it = unexploredBases.begin(); it != unexploredBases.end(); ++it) {
			if (BWAPI::Broodwar->isVisible((*it)->Location())) {
				unexploredBases.erase(it);
				break;
			}
		}
	}

	const std::vector <const BWEM::Base*> MapManager::getAllBases() const {
		return allBases;
	}
	const std::set    <const BWEM::Base*> MapManager::getUnexploredBases() const {
		return unexploredBases;
	}

	const BWAPI::TilePosition MapManager::getNextBasePosition() const {
		for (auto &b : allBases) {
			if (!BWAPI::Broodwar->isExplored(b->Location()))
				detectionManager.requestDetection((BWAPI::Position)b->Location());
			if (BWAPI::Broodwar->canBuildHere(b->Location(), BWAPI::UnitTypes::Terran_Command_Center)) {
				for (auto &eu : unitManager.getVisibleEnemies())
					if (eu.lastPosition.getApproxDistance(BWAPI::Position(b->Location())) < 1000)
						goto skiplocation;
				for (auto &eu : unitManager.getNonVisibleEnemies())
					if (eu.lastPosition.getApproxDistance(BWAPI::Position(b->Location())) < 1000)
						goto skiplocation;
				
				return b->Location();
				skiplocation:;
			}
		}

		return BWAPI::TilePosition(-1, -1);
	}
}
