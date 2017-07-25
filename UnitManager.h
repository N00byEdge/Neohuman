#pragma once

#include "BWAPI.h"
#include "Util.h"

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

		void updateFromUnit() const;
		void updateFromUnit(const BWAPI::Unit unit) const;

		int expectedHealth() const;
		int expectedShields() const;

		bool operator< (const EnemyData &other) const;
		bool operator==(const EnemyData &other) const;

		struct hash {
			std::size_t operator()(const EnemyData &ed) const;
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

			EnemyData getClosestEnemy(BWAPI::Unit from, const BWAPI::UnitFilter &filter = nullptr, bool onlyWithWeapons = false) const;
			EnemyData getClosestEnemy(BWAPI::Unit from, bool onlyWithWeapons = false) const;

			EnemyData getClosestVisibleEnemy(BWAPI::Unit from, const BWAPI::UnitFilter &filter = nullptr, bool onlyWithWeapons = false) const;
			EnemyData getClosestVisibleEnemy(BWAPI::Unit from, bool onlyWithWeapons = false) const;

			EnemyData getClosestNonVisibleEnemy(BWAPI::Unit from, const BWAPI::UnitFilter &filter = nullptr, bool onlyWithWeapons = false) const;
			EnemyData getClosestNonVisibleEnemy(BWAPI::Unit from, bool onlyWithWeapons = false) const;

			EnemyData getBestTarget(BWAPI::Unit from);
			int getNukeScore(BWAPI::Position pos, BWAPI::Unit from) const;
			BWAPI::Position getBestNuke(BWAPI::Unit from) const;

			static int targetPriority(BWAPI::Unit f, BWAPI::Unit ag);
			static int deathPerHealth(EnemyData ed, bool flyingTarget);
			static int targetPriority(BWAPI::Unit f, EnemyData ed);

			int countUnit    (BWAPI::UnitType t = BWAPI::UnitTypes::AllUnits, const BWAPI::UnitFilter &filter = nullptr, bool countQueued = true) const;
			int countFriendly(BWAPI::UnitType t = BWAPI::UnitTypes::AllUnits, bool onlyWithWeapons = false, bool countQueued = true) const;
			int countEnemy   (BWAPI::UnitType t = BWAPI::UnitTypes::AllUnits, bool onlyWithWeapons = false) const;

			bool isAllowedToLockdown(BWAPI::Unit target, BWAPI::Unit own) const;
			void reserveLockdown(BWAPI::Unit target, BWAPI::Unit own);

			unsigned getNumArmedSilos() const;
			unsigned getNumArmingSilos() const;
			unsigned getNumUnarmedSilos() const;

			static bool isOnFire(EnemyData building);
			static int unitDeathGround(BWAPI::UnitType ut);
			static int unitDeathAir(BWAPI::UnitType ut);
			static int unitDeath(BWAPI::UnitType ut);
			static int deathPerHealth(BWAPI::UnitType ut, int health);
			static int deathPerHealth(BWAPI::Unit unit);
			static void addToDeathMatrix(BWAPI::Position pos, BWAPI::UnitType ut, BWAPI::Player p);
			static bool reallyHasWeapon(const BWAPI::UnitType &unitType);

			BWAPI::Position lastKnownEnemyPosition(BWAPI::Unit) const;

			int getScore() const;
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
			// All enemies in here
			std::unordered_set <EnemyData, EnemyData::hash> knownEnemies;

			// All enemies in one of these
			std::unordered_set <EnemyData, EnemyData::hash> nonVisibleEnemies;
			std::unordered_set <EnemyData, EnemyData::hash> visibleEnemies;
			std::unordered_set <EnemyData, EnemyData::hash> invalidatedEnemies;

			std::map <BWAPI::UnitType, std::unordered_set <EnemyData, EnemyData::hash>> enemyUnitsByType;

			std::map <BWAPI::Unit, BWAPI::UnitType> friendlyUnits;
			std::map <BWAPI::UnitType, std::unordered_set <BWAPI::Unit>> friendlyUnitsByType;

			std::map <BWAPI::Unit, std::pair <BWAPI::Unit, int>> lockdownDB;

			bool score;
			std::map <BWAPI::UnitType, int> multikillDetector;

			unsigned launchedNukeCount = 0;
	};

}

extern Neolib::UnitManager unitManager;
extern int deathMatrixGround[(256 * 4)*(256 * 4)], deathMatrixAir[(256 * 4)*(256 * 4)];

#define deathMatrixSideLen (256 * 4)
#define deathMatrixSize (deathMatrixSideLen*deathMatrixSideLen)
