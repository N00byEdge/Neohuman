#pragma once

#include "BWAPI.h"
#include "Util.h"

#include <set>

namespace Neolib {

	struct EnemyData {
		EnemyData();
		EnemyData(BWAPI::Unit);

		BWAPI::Unit u = nullptr;
		mutable BWAPI::Player lastPlayer = nullptr;
		mutable BWAPI::UnitType lastType;
		mutable BWAPI::Position lastPosition;
		mutable int lastShields;
		mutable int lastHealth;
		mutable int frameLastSeen = 0;
		mutable int frameLastDetected = 0;
		mutable int unitID;
		mutable bool positionInvalidated = true;
		mutable bool isCompleted = false;

		void initFromUnit() const;
		void updateFromUnit() const;
		void updateFromUnit(const BWAPI::Unit unit) const;

		int expectedHealth() const;
		int expectedShields() const;

		bool isFriendly() const;
		bool isEnemy() const;

		bool operator< (const EnemyData &other) const;
		bool operator==(const EnemyData &other) const;

		struct hash {
			std::size_t operator()(const EnemyData &ed) const;
		};
	};

	struct SimResults {
		struct SimState {
			std::pair <int, int> scores, unitCounts;
		};

		SimState presim, shortsim, postsim;
		int shortLosses;
	};

	class UnitManager {
		public:
			const std::map <BWAPI::UnitType, std::set<std::shared_ptr<EnemyData>>> &getEnemyUnitsByType() const;
			const std::set <std::shared_ptr<EnemyData>> &getEnemyUnitsByType(BWAPI::UnitType ut) const;

			const std::map <BWAPI::UnitType, std::set <BWAPI::Unit>> &getFriendlyUnitsByType() const;
			const std::set <BWAPI::Unit> &getFriendlyUnitsByType(BWAPI::UnitType ut) const;

			const std::set <std::shared_ptr<EnemyData>> &getNonVisibleEnemies() const;
			const std::set <std::shared_ptr<EnemyData>> &getVisibleEnemies() const;
			const std::set <std::shared_ptr<EnemyData>> &getInvalidatedEnemies() const;
			const std::set <std::shared_ptr<EnemyData>> &getDeadEnemies() const;

			std::shared_ptr<EnemyData> getClosestEnemy(BWAPI::Unit from, const BWAPI::UnitFilter &filter = nullptr, bool onlyWithWeapons = false) const;
			std::shared_ptr<EnemyData> getClosestEnemy(BWAPI::Unit from, bool onlyWithWeapons = false) const;

			std::shared_ptr<EnemyData> getClosestVisibleEnemy(BWAPI::Unit from, const BWAPI::UnitFilter &filter = nullptr, bool onlyWithWeapons = false) const;
			std::shared_ptr<EnemyData> getClosestVisibleEnemy(BWAPI::Unit from, bool onlyWithWeapons = false) const;

			std::shared_ptr<EnemyData> getClosestNonVisibleEnemy(BWAPI::Unit from, const BWAPI::UnitFilter &filter = nullptr, bool onlyWithWeapons = false) const;
			std::shared_ptr<EnemyData> getClosestNonVisibleEnemy(BWAPI::Unit from, bool onlyWithWeapons = false) const;

			std::shared_ptr<EnemyData> getEnemyData(BWAPI::Unit ptr);

			static bool isEnemy(BWAPI::Unit u);
			static bool isOwn(BWAPI::Unit u);
			static bool isNeutral(BWAPI::Unit u);

			int countUnit    (BWAPI::UnitType t = BWAPI::UnitTypes::AllUnits, const BWAPI::UnitFilter &filter = nullptr) const;
			int countFriendly(BWAPI::UnitType t = BWAPI::UnitTypes::AllUnits, bool onlyWithWeapons = false) const;
			int countEnemy   (BWAPI::UnitType t = BWAPI::UnitTypes::AllUnits, bool onlyWithWeapons = false) const;

			unsigned getNumArmedSilos() const;
			unsigned getNumArmingSilos() const;
			unsigned getNumUnarmedSilos() const;

			static bool isOnFire(EnemyData building);
			static bool reallyHasWeapon(const BWAPI::UnitType &unitType);

			SimResults getSimResults();
			unsigned getLaunchedNukeCount() const;

			void onFrame();
			void onNukeDetect(BWAPI::Position target);
			void onUnitDiscover(BWAPI::Unit unit);
			void onUnitShow(BWAPI::Unit unit);
			void onUnitHide(BWAPI::Unit unit);
			void onUnitCreate(BWAPI::Unit unit);
			void onUnitDestroy(BWAPI::Unit unit);
			void onUnitMorph(BWAPI::Unit unit);
			void onUnitRenegade(BWAPI::Unit unit);
			void onUnitEvade(BWAPI::Unit unit);
			void onUnitComplete(BWAPI::Unit unit);

		private:
			void doCombatSim();
			void doMultikillDetector();

			std::map <int, std::shared_ptr<EnemyData>> enemyUnits;
			std::set <std::shared_ptr<EnemyData>> nonVisibleEnemies, visibleEnemies, invalidatedEnemies, deadEnemies;

			std::map <BWAPI::UnitType, std::set<std::shared_ptr<EnemyData>>> enemyUnitsByType;
			std::map <BWAPI::UnitType, std::set<BWAPI::Unit>> friendlyUnitsByType;

			std::map <BWAPI::Unit, std::pair <BWAPI::Unit, int>> lockdownDB;

			std::map <BWAPI::UnitType, int> multikillDetector;

			SimResults sr;
			
			unsigned launchedNukeCount = 0;
	};

}

extern Neolib::UnitManager unitManager;
extern int deathMatrixGround[(256 * 4)*(256 * 4)], deathMatrixAir[(256 * 4)*(256 * 4)];

#define deathMatrixSideLen (256 * 4)
#define deathMatrixSize (deathMatrixSideLen*deathMatrixSideLen)
