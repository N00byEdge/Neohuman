#pragma once

#include "Neohuman.h"

namespace Neolib {

	class BuildingPlacer {
		public:
			void init();

			BWAPI::TilePosition getBuildLocation(BWAPI::UnitType ut, BWAPI::TilePosition suggestedPosition);
			bool canBuildAt(BWAPI::TilePosition tp);
			bool canPlaceBuilding(BWAPI::UnitType ut, BWAPI::TilePosition location);

		private:
			int mapHeight, mapWidth;
			bool mapCanBuild[256 * 256];
			bool canBuild[256 * 256];
	};

}

extern Neolib::BuildingPlacer buildingPlacer;
