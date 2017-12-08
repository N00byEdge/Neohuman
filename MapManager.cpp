#include "MapManager.h"

#include "UnitManager.h"

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

	int MapManager::getGroundDistance(BWAPI::Position start, BWAPI::Position end) {
		int dist = 0;

		for (auto cpp : BWEM::Map::Instance().GetPath(start, end)) {
			auto center = BWAPI::Position{ cpp->Center() };
			dist += start.getDistance(center);
			start = center;
		}

		return dist += start.getDistance(end);
	}

	const std::vector <const BWEM::Base*> MapManager::getAllBases() const {
		return allBases;
	}
	const std::set <const BWEM::Base*> MapManager::getUnexploredBases() const {
		return unexploredBases;
	}

}
