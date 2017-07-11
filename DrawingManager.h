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
		bool enableSaturationInfo = true;
		bool enableTimerInfo = true;
		bool enableEnemyOverlay = true;
		bool enableDeathMatrix = false;
		bool enableBaseOverlay = true;
		bool enableHealthBars = true;
		bool enableFailedLocations = false;
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
