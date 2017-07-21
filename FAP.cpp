#include "FAP.h"

#include "Util.h"

namespace Neolib {

	FastAPproximation::FastAPproximation() {

	}

	void FastAPproximation::addUnitPlayer1(FAPUnit fu) {
		player1.push_back(fu);
	}

	void FastAPproximation::addUnitPlayer2(FAPUnit fu) {
		player2.push_back(fu);
	}

	void FastAPproximation::simulate(int nFrames) {
		while (nFrames--) {
			if (!player1.size() || !player2.size())
				break;

			didSomething = false;

			isimulate();

			if (!didSomething)
				break;
		}
	}

	int FastAPproximation::getStatus() {
		int score = 0;

		for (auto & u : player1)
			if (u.health && u.maxHealth)
				score += (u.score * u.health) / u.maxHealth;

		for (auto & u : player2)
			if (u.health && u.maxHealth)
				score -= (u.score * u.health) / u.maxHealth;

		return score;
	}

	std::pair<std::vector<FastAPproximation::FAPUnit>*, std::vector<FastAPproximation::FAPUnit>*> FastAPproximation::getState() {
		return { &player1, &player2 };
	}

	void FastAPproximation::clearState() {
		player1.clear(), player2.clear();
	}

	void FastAPproximation::dealDamage(const FastAPproximation::FAPUnit &fu, int damage, BWAPI::DamageType damageType) const {
		if (fu.shields >= damage - fu.shieldArmor) {
			fu.shields -= damage - fu.shieldArmor;
			return;
		}
		else if(fu.shields) {
			damage -= (fu.shields + fu.shieldArmor);
			fu.shields = 0;
		}


		if (!damage)
			return;

#ifdef SSCAIT

		if (damageType == BWAPI::DamageTypes::Concussive) {
			if(fu.unitSize == BWAPI::UnitSizeTypes::Large)
				damage = damage / 2;
			else if(fu.unitSize == BWAPI::UnitSizeTypes::Medium)
				damage = (damage * 3) / 4;
		}
		else if (damageType == BWAPI::DamageTypes::Explosive) {
			if (fu.unitSize == BWAPI::UnitSizeTypes::Small)
				damage = damage / 2;
			else if (fu.unitSize == BWAPI::UnitSizeTypes::Medium)
				damage = (damage * 3) / 4;
		}

#else

		switch (damageType) {
			case BWAPI::DamageTypes::Concussive:
				switch (fu.unitSize) {
					case BWAPI::UnitSizeTypes::Large:
						damage = damage / 2;
						break;
					case BWAPI::UnitSizeTypes::Medium:
						damage = (damage * 3) / 4;
						break;
				}
				break;

			case BWAPI::DamageTypes::Explosive:
				switch (fu.unitSize) {
					case BWAPI::UnitSizeTypes::Small:
						damage = damage / 2;
						break;
					case BWAPI::UnitSizeTypes::Medium:
						damage = (damage * 3) / 4;
						break;
				}
				break;
		}

#endif

		fu.health -= MAX(1, damage - fu.armor);
	}

	int FastAPproximation::dist(const FastAPproximation::FAPUnit &u1, const FastAPproximation::FAPUnit &u2) const {
		return (int)sqrt((u1.x - u2.x)*(u1.x - u2.x) + (u1.y - u2.y)*(u1.y - u2.y));
	}

	void FastAPproximation::unitsim(const FastAPproximation::FAPUnit &fu, std::vector <FastAPproximation::FAPUnit> &enemyUnits) {
		if (fu.attackCooldownRemaining) {
			didSomething = true;
			return;
		}

		auto closestEnemy = enemyUnits.end();
		int closestDist;

		for (auto enemyIt = enemyUnits.begin(); enemyIt != enemyUnits.end(); ++ enemyIt) {
			if (enemyIt->flying) {
				if (fu.airDamage) {
					int d = dist(fu, *enemyIt);
					if ((closestEnemy == enemyUnits.end() || d < closestDist) && d >= fu.airMinRange) {
						closestDist = d;
						closestEnemy = enemyIt;
					}
				}
			}
			else {
				if (fu.groundDamage) {
					int d = dist(fu, *enemyIt);
					if ((closestEnemy == enemyUnits.end() || d < closestDist) && d >= fu.groundMinRange) {
						closestDist = d;
						closestEnemy = enemyIt;
					}
				}
			}
		}

		if (closestEnemy != enemyUnits.end() && closestDist <= fu.speed && !(fu.x == closestEnemy->x && fu.y == closestEnemy->y)) {
			fu.x = closestEnemy->x;
			fu.y = closestEnemy->y;
			closestDist = 0;

			didSomething = true;
		}

		if (closestEnemy != enemyUnits.end() && closestDist <= (closestEnemy->flying ? fu.groundMaxRange : fu.airMinRange)) {
			if (closestEnemy->flying)
				dealDamage(*closestEnemy, fu.airDamage, fu.airDamageType), fu.attackCooldownRemaining = fu.airCooldown;
			else
				dealDamage(*closestEnemy, fu.groundDamage, fu.groundDamageType), fu.attackCooldownRemaining = fu.groundCooldown;

			if (closestEnemy->health < 1) {
				*closestEnemy = enemyUnits.back();
				enemyUnits.pop_back();
			}

			didSomething = true;
			return;
		}
		else if(closestEnemy != enemyUnits.end() && closestDist > fu.speed) {
			int dx = closestEnemy->x - fu.x, dy = closestEnemy->y - fu.y;
			if (dx || dy) {
				fu.x += (int)(dx*(fu.speed / sqrt(dx*dx + dy*dy)));
				fu.y += (int)(dy*(fu.speed / sqrt(dx*dx + dy*dy)));
				
				didSomething = true;
				return;
			}
		}
	}

	void FastAPproximation::isimulate() {
		for (auto &fu : player1)
			unitsim(fu, player2);

		for (auto &fu : player2)
			unitsim(fu, player1);

		for (auto &fu : player1)
			if (fu.attackCooldownRemaining)
				--fu.attackCooldownRemaining;

		for (auto &fu : player2)
			if (fu.attackCooldownRemaining)
				--fu.attackCooldownRemaining;
	}

	FastAPproximation::FAPUnit::FAPUnit(BWAPI::Unit u): FAPUnit(EnemyData(u)) {

	}

	FastAPproximation::FAPUnit::FAPUnit(EnemyData ed) :
		x(ed.lastPosition.x),
		y(ed.lastPosition.y),

		speed(ed.lastPlayer->topSpeed(ed.lastType)),

		health(ed.expectedHealth()),
		maxHealth(ed.lastType.maxHitPoints()),

		shields(ed.expectedShields()),
		shieldArmor(ed.lastPlayer->getUpgradeLevel(BWAPI::UpgradeTypes::Protoss_Plasma_Shields)),
		maxShields(ed.lastType.maxShields()),
		armor(ed.lastPlayer->armor(ed.lastType)),
		flying(ed.lastType.isFlyer()),

		groundDamage(ed.lastPlayer->damage(ed.lastType.groundWeapon())),
		groundCooldown(ed.lastType.groundWeapon().damageFactor() && ed.lastType.maxGroundHits() ? ed.lastPlayer->weaponDamageCooldown(ed.lastType) / (ed.lastType.groundWeapon().damageFactor() * ed.lastType.maxGroundHits()) : 0),
		//groundInnerRadius(ed.lastType.groundWeapon().innerSplashRadius()),
		//groundOuterRadius(ed.lastType.groundWeapon().outerSplashRadius()),
		groundMaxRange(ed.lastPlayer->weaponMaxRange(ed.lastType.groundWeapon())),
		groundMinRange(ed.lastType.groundWeapon().minRange()), 
		groundDamageType(ed.lastType.groundWeapon().damageType()),

		airDamage(ed.lastPlayer->damage(ed.lastType.airWeapon())), 
		airCooldown(ed.lastType.airWeapon().damageFactor() && ed.lastType.maxAirHits() ? ed.lastType.airWeapon().damageCooldown() / (ed.lastType.airWeapon().damageFactor() * ed.lastType.maxAirHits()) : 0),
		//airInnerRadius(ed.lastType.airWeapon().innerSplashRadius()),
		//airOuterRadius(ed.lastType.airWeapon().outerSplashRadius()),
		airMaxRange(ed.lastPlayer->weaponMaxRange(ed.lastType.airWeapon())),
		airMinRange(ed.lastType.airWeapon().minRange()),
		airDamageType(ed.lastType.airWeapon().damageType()),

		score(ed.lastType.destroyScore()) {

		static int nextId = 0;
		id = nextId++;

#ifdef SSCAIT

		if (ed.lastType == BWAPI::UnitTypes::Protoss_Carrier) {
			groundDamage = ed.lastPlayer->damage(BWAPI::UnitTypes::Protoss_Interceptor.groundWeapon());
			groundDamageType = BWAPI::UnitTypes::Protoss_Interceptor.groundWeapon().damageType();
			groundCooldown = 2;
			groundMaxRange = 32 * 8;

			airDamage = groundDamage;
			airDamageType = groundDamageType;
			airCooldown = groundCooldown;
			airMaxRange = groundMaxRange;
		}
		else if (ed.lastType == BWAPI::UnitTypes::Terran_Bunker) {
			groundDamage = ed.lastPlayer->damage(BWAPI::WeaponTypes::Gauss_Rifle);
			groundCooldown = BWAPI::UnitTypes::Terran_Marine.groundWeapon().damageCooldown() / 4;
			groundMaxRange = ed.lastPlayer->weaponMaxRange(BWAPI::UnitTypes::Terran_Marine.groundWeapon()) + 32;

			airDamage = groundDamage;
			airCooldown = groundCooldown;
			airMaxRange = groundMaxRange;
		}
		else if (ed.lastType == BWAPI::UnitTypes::Protoss_Reaver) {
			groundDamage = ed.lastPlayer->damage(BWAPI::WeaponTypes::Scarab);
		}

#else

		switch (ed.lastType) {
			case BWAPI::UnitTypes::Protoss_Carrier:
				groundDamage = ed.lastPlayer->damage(BWAPI::UnitTypes::Protoss_Interceptor.groundWeapon());
				groundDamageType = BWAPI::UnitTypes::Protoss_Interceptor.groundWeapon().damageType();
				groundCooldown = 5;
				groundMaxRange = 32 * 8;

				airDamage = groundDamage;
				airDamageType = groundDamageType;
				airCooldown = groundCooldown;
				airMaxRange = groundMaxRange;
				break;

			case BWAPI::UnitTypes::Terran_Bunker:
				groundDamage = ed.lastPlayer->damage(BWAPI::WeaponTypes::Gauss_Rifle);
				groundCooldown = BWAPI::UnitTypes::Terran_Marine.groundWeapon().damageCooldown() / 4;
				groundMaxRange = ed.lastPlayer->weaponMaxRange(BWAPI::UnitTypes::Terran_Marine.groundWeapon()) + 32;

				airDamage = groundDamage;
				airCooldown = groundCooldown;
				airMaxRange = groundMaxRange;
				break;

			case BWAPI::UnitTypes::Protoss_Reaver:
				groundDamage = ed.lastPlayer->damage(BWAPI::WeaponTypes::Scarab);
				break;

			default:
				break;
		}

#endif

		groundDamage *= 2;
		airDamage *= 2;

		shieldArmor *= 2;
		armor *= 2;

		health *= 2;
		shields *= 2;
	}

	FastAPproximation::FAPUnit &FastAPproximation::FAPUnit::operator=(const FAPUnit & other) {
		x = other.x, y = other.y;
		health = other.health, maxHealth = other.maxHealth;
		shields = other.shields, maxShields = other.maxShields;
		speed = other.speed, armor = other.armor, flying = other.flying, unitSize = other.unitSize;
		groundDamage = other.groundDamage, groundCooldown = other.groundCooldown, groundMaxRange = other.groundMaxRange, groundMinRange = other.groundMinRange, groundDamageType = other.groundDamageType;
		airDamage = other.airDamage, airCooldown = other.airCooldown, airMaxRange = other.airMaxRange, airMinRange = other.airMinRange, airDamageType = other.airDamageType;
		score = other.score;
		attackCooldownRemaining = other.attackCooldownRemaining;

		return *this;
	}

	bool FastAPproximation::FAPUnit::operator<(const FAPUnit & other) const {
		return id < other.id;
	}

}
