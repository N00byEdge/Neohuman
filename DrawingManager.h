#pragma once

namespace Neolib {

	class DrawingManager {
		public:
			DrawingManager(const DrawingManager &other);
			DrawingManager(bool enableTopInfo = true, bool enableResourceOverlay = true, bool enableBWEMOverlay = false, bool enableComsatInfo = true, bool enableListBuildingQueue = true, bool enableSaturationInfo = true, bool enableTimerInfo = true, bool enableEnemyOverlay = true);
			void onFrame() const;

			bool enableTopInfo, enableResourceOverlay, enableBWEMOverlay, enableComsatInfo, enableListBuildingQueue, enableSaturationInfo, enableTimerInfo, enableEnemyOverlay;

		private:
			static int getNextColumnY(int &columnY);
	};

}

extern Neolib::DrawingManager drawingManager;
