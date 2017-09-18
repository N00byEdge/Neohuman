#pragma once

#include "bwem.h"

#include <set>

namespace Neolib {

	class MapManager {
		public:
			void init();

			void onFrame();

			const std::vector <const BWEM::Base*> getAllBases() const;
			const std::set    <const BWEM::Base*> getUnexploredBases() const;

		private:
			std::vector <const BWEM::Base*> allBases;
			std::set    <const BWEM::Base*> unexploredBases;
	};

}

extern Neolib::MapManager mapManager;
