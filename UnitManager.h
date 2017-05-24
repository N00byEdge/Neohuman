#pragma once

#include "BWAPI.h"

namespace Neolib {

	struct EnemyData {
		EnemyData();
		EnemyData(BWAPI::Unit);

		BWAPI::Unit u;
		mutable BWAPI::UnitType lastType;
		mutable BWAPI::Position lastPosition;
		mutable int lastShiedls;
		mutable int lastHealth;
		mutable int frameLastSeen;
		mutable bool positionInvalidated;

		int expectedHealth() const;
		int expectedShields() const;

		bool operator< (const EnemyData &other) const;
		bool operator==(const EnemyData &other) const;

		struct hash {
			std::size_t operator()(const EnemyData &ed) const {
				return std::hash <BWAPI::Unit>()(ed.u);
			}
		};
	};

	class UnitManager {
		public:
			const std::map <BWAPI::UnitType, std::unordered_set <EnemyData, EnemyData::hash>>   &getEnemyUnitsByType() const;
			const std::map <BWAPI::UnitType, std::unordered_set <BWAPI::Unit>> &getFriendlyUnitsByType() const;

			const std::unordered_set <EnemyData, EnemyData::hash> &getNonVisibleEnemies() const;
			const std::unordered_set <EnemyData, EnemyData::hash> &getVisibleEnemies() const;
			const std::unordered_set <EnemyData, EnemyData::hash> &getInvalidatedEnemies() const;

			const std::unordered_set <EnemyData, EnemyData::hash>   &getEnemyUnitsByType(BWAPI::UnitType ut) const;
			const std::unordered_set <BWAPI::Unit> &getFriendlyUnitsByType(BWAPI::UnitType ut) const;

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
			// All enemies in here
			std::unordered_set <EnemyData, EnemyData::hash> knownEnemies;

			// All enemies in one of these
			std::unordered_set <EnemyData, EnemyData::hash> nonVisibleEnemies;
			std::unordered_set <EnemyData, EnemyData::hash> visibleEnemies;
			std::unordered_set <EnemyData, EnemyData::hash> invalidatedEnemies;

			std::map <BWAPI::UnitType, std::unordered_set <EnemyData, EnemyData::hash>> enemyUnitsByType;

			std::map <BWAPI::Unit, BWAPI::UnitType> friendlyUnits;
			std::map <BWAPI::UnitType, std::unordered_set <BWAPI::Unit>> friendlyUnitsByType;
	};

}

extern Neolib::UnitManager unitManager;
