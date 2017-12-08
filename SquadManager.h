#pragma once

#include "BWAPI.h"
#include "UnitManager.h"
#include "FAP.h"

namespace Neolib {

	struct Squad {
		Squad();
		const int id;
		virtual bool isEnemy() = 0;
		virtual bool isFriendly() = 0;
		virtual void addUnit(BWAPI::Unit) = 0;
		virtual void removeUnit(BWAPI::Unit) = 0;
		virtual void updatePosition() = 0;
		virtual unsigned long long numUnits() = 0;
		BWAPI::Position pos;
		int radius1;
		int radius2;
	};

	struct FriendlySquad;

	struct EnemySquad : public Squad {
		virtual bool isEnemy() override { return true; }
		virtual bool isFriendly() override { return false; }
		virtual void addUnit(BWAPI::Unit) override;
		virtual void removeUnit(BWAPI::Unit) override;
		virtual void updatePosition() override;
		virtual unsigned long long numUnits() override { return units.size(); }
		void squadWasRemoved(FriendlySquad *sq) { engagedSquads.erase(sq); }
		std::set <std::shared_ptr<EnemyData>> units;

		std::set <FriendlySquad *> engagedSquads;
	};

	struct FriendlySquad : public Squad {
		virtual bool isEnemy() override { return false; }
		virtual bool isFriendly() override { return true; }
		virtual void addUnit(BWAPI::Unit) override;
		virtual void removeUnit(BWAPI::Unit) override;
		virtual void updatePosition() override;
		virtual unsigned long long numUnits() override { return units.size(); }
		void squadWasRemoved(EnemySquad *sq) { engagedSquads.erase(sq); }
		std::set <BWAPI::Unit> units;

		SimResults simres;

		std::set <EnemySquad *> engagedSquads;
	};

	class SquadManager {
		public:
			static bool shouldAttack(SimResults sr);

			void onFrame();
			void onUnitDeath(BWAPI::Unit);
			void onUnitCreate(BWAPI::Unit);
			void onUnitRenegade(BWAPI::Unit);

			void onEnemyRecognize(std::shared_ptr<EnemyData>);
			void onEnemyLose(std::shared_ptr<EnemyData>);

			std::shared_ptr<Squad> getSquad(BWAPI::Unit);
			std::shared_ptr<Squad> addToNewSquad(BWAPI::Unit);
			void addUnitToSquad(BWAPI::Unit, std::shared_ptr<Squad> sq);

			std::shared_ptr<Squad> addNewFriendlySquad();
			std::shared_ptr<Squad> addNewEnemySquad();

			bool isInSquad(BWAPI::Unit);
			void removeFromSquad(BWAPI::Unit);

			std::vector<std::shared_ptr<EnemySquad>> &getEnemySquads();
			std::vector<std::shared_ptr<FriendlySquad>> &getFriendlySquads();

		private:
			void removeSquad(FriendlySquad *);
			void removeSquad(EnemySquad *);
			void removeSquadReferences(Squad *);

			std::set <BWAPI::Unit> unmanagedUnits;
			std::set <std::shared_ptr<EnemyData>> unmanagedEnemyUnits;
			std::map <BWAPI::Unit, std::shared_ptr<Squad>> squadLookup;
			std::vector <std::shared_ptr<EnemySquad>> enemySquads;
			std::vector <std::shared_ptr<FriendlySquad>> friendlySquads;
	};

}

extern Neolib::SquadManager squadManager;
