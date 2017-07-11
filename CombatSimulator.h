#pragma once

#include "SparCraft.h"

namespace Neolib {

	class CombatSimulator {
		public:
			CombatSimulator();

			void init();

			void addUnitToSimulator(int id, BWAPI::Position pos, BWAPI::UnitType ut, bool isEnemy, int health, int shields);
			void clear();
			bool simulate();

			void onStart();

		private:

			SparCraft::Map myMap;
			std::shared_ptr <SparCraft::Map> myMapPtr;
			SparCraft::GameState state;

	};

}

extern Neolib::CombatSimulator combatSimulator;
