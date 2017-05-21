#pragma once

#include "BWAPI.h"

#include "Triple.h"
#include "Util.h"
#include "ResourceManager.h"
#include "SupplyManager.h"

namespace Neolib{

	class BuildingQueue {
		public:
			void update();

			bool doBuild(BWAPI::Unit, BWAPI::UnitType, BWAPI::TilePosition);

			ResourceCount getQueuedResources() const;
			SupplyCount getQueuedSupply(bool countResourceDepots = false) const;

			bool isWorkerBuilding(BWAPI::Unit) const;

			const std::vector <std::pair <Neolib::Triple <int, BWAPI::UnitType, BWAPI::TilePosition>, bool>> &buildingsQueued();

		private:
			std::vector <std::pair <Neolib::Triple <int, BWAPI::UnitType, BWAPI::TilePosition>, bool>> buildingQueue;
	};

}

extern Neolib::BuildingQueue buildingQueue;
