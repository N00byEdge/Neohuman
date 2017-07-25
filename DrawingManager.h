#pragma once

#include "Neohuman.h"
#include "Util.h"

#include <set>

namespace Neolib {

	struct DrawerSettings {
#ifdef _DEBUG

		bool enableTopInfo = true;
		bool enableResourceOverlay = true;
		bool enableBWEMOverlay = false;
		bool enableComsatInfo = true;
		bool enableListBuildingQueue = true;
		bool enableSaturationInfo = true;
		bool enableTimerInfo = true;
		bool enableEnemyOverlay = true;
		bool enableDeathMatrix = false;
		bool enableBaseOverlay = true;
		bool enableHealthBars = true;
		bool enableFailedLocations = true;
		bool enableNukeOverlay = true;
		bool enableNukeSpots = true;
		bool enableCombatSimOverlay = true;

#else

#ifdef SSCAIT

		bool enableTopInfo = true;
		bool enableResourceOverlay = true;
		bool enableBWEMOverlay = false;
		bool enableComsatInfo = true;
		bool enableListBuildingQueue = true;
		bool enableSaturationInfo = false;
		bool enableTimerInfo = false;
		bool enableEnemyOverlay = false;
		bool enableDeathMatrix = false;
		bool enableBaseOverlay = false;
		bool enableHealthBars = true;
		bool enableFailedLocations = false;
		bool enableNukeOverlay = false;
		bool enableNukeSpots = true;
		bool enableCombatSimOverlay = true;

#else

		bool enableTopInfo = true;
		bool enableResourceOverlay = true;
		bool enableBWEMOverlay = false;
		bool enableComsatInfo = true;
		bool enableListBuildingQueue = true;
		bool enableSaturationInfo = false;
		bool enableTimerInfo = false;
		bool enableEnemyOverlay = false;
		bool enableDeathMatrix = false;
		bool enableBaseOverlay = false;
		bool enableHealthBars = true;
		bool enableFailedLocations = false;
		bool enableNukeOverlay = false;
		bool enableNukeSpots = true;
		bool enableCombatSimOverlay = true;

#endif

#endif
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
