#pragma once

#include "BWAPI.h"
#include "UnitManager.h"

#include <set>

namespace Neolib {

	class FastAPproximation {
		struct FAPUnit {
			FAPUnit(BWAPI::Unit u);
			FAPUnit(EnemyData ed);
			FAPUnit &operator= (const FAPUnit &other);

			int id = 0;

			mutable int x = 0, y = 0;

			mutable int health = 0;
			mutable int maxHealth = 0;
			mutable int armor = 0;

			mutable int shields = 0;
			mutable int shieldArmor = 0;
			mutable int maxShields = 0;

			mutable double speed = 0;
			mutable bool flying = 0;
			mutable BWAPI::UnitSizeType unitSize;

			mutable int groundDamage = 0;
			mutable int groundCooldown = 0;
			/*mutable int groundInnerRadius = 0;
			mutable int groundOuterRadius = 0;*/
			mutable int groundMaxRange = 0;
			mutable int groundMinRange = 0;
			mutable BWAPI::DamageType groundDamageType;

			mutable int airDamage = 0;
			mutable int airCooldown = 0;
			/*mutable int airInnerRadius = 0;
			mutable int airOuterRadius = 0;*/
			mutable int airMaxRange = 0;
			mutable int airMinRange = 0;
			mutable BWAPI::DamageType airDamageType;

			mutable int score = 0;

			mutable int attackCooldownRemaining = 0;

			bool operator< (const FAPUnit &other) const;
		};

		public:

			FastAPproximation();

			void addUnitPlayer1(FAPUnit fu);
			void addUnitPlayer2(FAPUnit fu);

			void simulate(int nFrames = -1);

			std::pair <int, int> getStatus() const;
			std::pair <std::vector <FAPUnit> *, std::vector <FAPUnit> *> getState();
			void clearState();

		private:
			std::vector <FAPUnit> player1, player2;

			bool didSomething;
			void dealDamage(const FastAPproximation::FAPUnit & fu, int damage, BWAPI::DamageType damageType) const;
			int dist(const FastAPproximation::FAPUnit & u1, const FastAPproximation::FAPUnit & u2) const;
			void unitsim(const FAPUnit & fu, std::vector <FAPUnit> &enemyUnits);
			void isimulate();

	};

}
