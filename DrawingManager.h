#pragma once

#include "Neohuman.h"
#include "Util.h"

#include <set>

namespace Neolib {

	struct DrawerSettings {
#ifdef _DEBUG

		bool enableTopInfo = true;
		bool enableBWEMOverlay = false;
		bool enableTimerInfo = true;
		bool enableEnemyOverlay = true;
		bool enableHealthBars = true;
		bool enableNukeSpots = true;

#else

#ifdef SSCAIT

		bool enableTopInfo = true;
		bool enableBWEMOverlay = false;
		bool enableTimerInfo = false;
		bool enableEnemyOverlay = false;
		bool enableHealthBars = true;
		bool enableNukeSpots = true;

#else

		bool enableTopInfo = true;
		bool enableBWEMOverlay = false;
		bool enableTimerInfo = false;
		bool enableEnemyOverlay = false;
		bool enableHealthBars = true;
		bool enableNukeSpots = true;

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
