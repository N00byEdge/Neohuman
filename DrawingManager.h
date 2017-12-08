#pragma once

#include "Neohuman.h"
#include "Util.h"

#include <set>

namespace Neolib {

	struct DrawerSettings {
		bool enableTopInfo = true;
		bool enableResourceOverlay = true;
		bool enableBWEMOverlay = false;
		bool enableComsatInfo = true;
		bool enableListBuildingQueue = true;
		bool enableSaturationInfo = isDebugBuild();
		bool enableTimerInfo = isDebugBuild();
		bool enableEnemyOverlay = true;
		bool enableDeathMatrix = false;
		bool enableBaseOverlay = isDebugBuild();
		bool enableHealthBars = true;
		bool enableFailedLocations = isDebugBuild();
		bool enableNukeOverlay = true;
		bool enableNukeSpots = true;
		bool enableCombatSimOverlay = true;
		bool enableSquadOverlay = true;
	};

	class DrawingManager {
		public:
			DrawingManager();
			DrawingManager(DrawerSettings);
			void onFrame() const;

			std::set <std::pair<BWAPI::TilePosition, BWAPI::UnitType>> failedLocations;

		private:
			DrawerSettings s;
			static int getNextColumnY(int &columnY);
	};

}

extern Neolib::DrawingManager drawingManager;
