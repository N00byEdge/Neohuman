#pragma once

#include "BWAPI.h"

namespace Neolib {

	class UnitManager {
		public:
			const std::map <BWAPI::Unit, std::pair<BWAPI::Position, BWAPI::UnitType>> &getKnownEnemies() const;
			const std::map <BWAPI::Unit, BWAPI::UnitType> &getUnitsInTypeSet() const;

			const std::map <BWAPI::UnitType, std::set <BWAPI::Unit>> &getEnemyUnitsByType() const;
			const std::map <BWAPI::UnitType, std::set <BWAPI::Unit>> &getFriendlyUnitsByType() const;

			const std::set <BWAPI::Unit> &getEnemyUnitsByType   (BWAPI::UnitType ut) const;
			const std::set <BWAPI::Unit> &getFriendlyUnitsByType(BWAPI::UnitType ut) const;

			BWAPI::Unit getClosestBuilder(BWAPI::Unit u) const;
			BWAPI::Unit getAnyBuilder() const;

			BWAPI::Unit getClosestEnemy(BWAPI::Unit from, const BWAPI::UnitFilter &filter = nullptr, bool onlyWithWeapons = false) const;
			BWAPI::Unit getClosestEnemy(BWAPI::Unit from, bool onlyWithWeapons = false) const;

			int countUnit    (BWAPI::UnitType t = BWAPI::UnitTypes::AllUnits, const BWAPI::UnitFilter &filter = nullptr, bool countQueued = true) const;
			int countFriendly(BWAPI::UnitType t = BWAPI::UnitTypes::AllUnits, bool onlyWithWeapons = false, bool countQueued = true) const;
			int countEnemy   (BWAPI::UnitType t = BWAPI::UnitTypes::AllUnits, bool onlyWithWeapons = false) const;

			static bool reallyHasWeapon(const BWAPI::UnitType &unitType);

			BWAPI::Position lastKnownEnemyPosition(BWAPI::Unit) const;

			void onFrame();
			void onUnitDiscover(BWAPI::Unit unit);
			void onUnitShow(BWAPI::Unit unit);
			void onUnitHide(BWAPI::Unit unit);
			void onUnitCreate(BWAPI::Unit unit);
			void onUnitDestroy(BWAPI::Unit unit);
			void onUnitMorph(BWAPI::Unit unit);
			void onUnitRenegade(BWAPI::Unit unit);
			void onUnitComplete(BWAPI::Unit unit);

		private:
			std::map <BWAPI::Unit, std::pair<BWAPI::Position, BWAPI::UnitType>> knownEnemies;
			std::map <BWAPI::Unit, BWAPI::UnitType> unitsInTypeSet;

			std::map <BWAPI::UnitType, std::set <BWAPI::Unit>> enemyUnitsByType;
			std::map <BWAPI::UnitType, std::set <BWAPI::Unit>> friendlyUnitsByType;
	};

}

extern Neolib::UnitManager unitManager;
