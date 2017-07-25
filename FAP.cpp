#include "FAP.h"

#include "Util.h"

Neolib::FastAPproximation fap;

namespace Neolib {

	FastAPproximation::FastAPproximation() {

	}

	void FastAPproximation::addUnitPlayer1(FAPUnit fu) {
		player1.push_back(fu);
	}

	void FastAPproximation::addIfCombatUnitPlayer1(FAPUnit fu) {
		if (fu.groundDamage || fu.airDamage || fu.unitType == BWAPI::UnitTypes::Terran_Medic)
			addUnitPlayer1(fu);
	}

	void FastAPproximation::addUnitPlayer2(FAPUnit fu) {
		player2.push_back(fu);
	}

	void FastAPproximation::addIfCombatUnitPlayer2(FAPUnit fu) {
		if (fu.groundDamage || fu.airDamage || fu.unitType == BWAPI::UnitTypes::Terran_Medic)
			addUnitPlayer2(fu);
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

	std::pair <int, int> FastAPproximation::playerScores() const {
		std::pair <int, int> res;

		for (auto & u : player1)
			if (u.health && u.maxHealth)
				res.first += (u.score * u.health) / (u.maxHealth * 2);

		for (auto & u : player2)
			if (u.health && u.maxHealth)
				res.second += (u.score * u.health) / (u.maxHealth * 2);

		return res;
	}

	std::pair <int, int> FastAPproximation::playerScoresUnits() const {
		std::pair <int, int> res;

		for (auto & u : player1)
			if (u.health && u.maxHealth && !u.unitType.isBuilding())
				res.first += (u.score * u.health) / (u.maxHealth * 2);

		for (auto & u : player2)
			if (u.health && u.maxHealth && !u.unitType.isBuilding())
				res.second += (u.score * u.health) / (u.maxHealth * 2);

		return res;
	}

	std::pair <int, int> FastAPproximation::playerScoresBuildings() const {
		std::pair <int, int> res;

		for (auto & u : player1)
			if (u.health && u.maxHealth && u.unitType.isBuilding())
				res.first += (u.score * u.health) / (u.maxHealth * 2);

		for (auto & u : player2)
			if (u.health && u.maxHealth && u.unitType.isBuilding())
				res.second += (u.score * u.health) / (u.maxHealth * 2);

		return res;
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

#ifdef _DEBUG

		damage = MAX(1, damage - fu.armor);
		fu.damageTaken += damage;
		fu.health -= damage;

#else

		fu.health -= MAX(1, damage - fu.armor);

#endif
	}

	int inline FastAPproximation::dist(const FastAPproximation::FAPUnit &u1, const FastAPproximation::FAPUnit &u2) const {
		return (u1.x - u2.x)*(u1.x - u2.x) + (u1.y - u2.y)*(u1.y - u2.y);
	}

	bool FastAPproximation::isSuicideUnit(BWAPI::UnitType ut) {
		return (ut == BWAPI::UnitTypes::Zerg_Scourge || ut == BWAPI::UnitTypes::Terran_Vulture_Spider_Mine || ut == BWAPI::UnitTypes::Zerg_Infested_Terran);
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
			else {
				dealDamage(*closestEnemy, fu.groundDamage, fu.groundDamageType);
				fu.attackCooldownRemaining = fu.groundCooldown;
				if (fu.elevation != -1 && closestEnemy->elevation != -1)
					if (closestEnemy->elevation > fu.elevation)
						fu.attackCooldownRemaining += fu.groundCooldown;
			}

			if (closestEnemy->health < 1) {
				auto temp = *closestEnemy;
				*closestEnemy = enemyUnits.back();
				enemyUnits.pop_back();
				unitDeath(temp, enemyUnits);
			}

			didSomething = true;
			return;
		}
		else if(closestEnemy != enemyUnits.end() && closestDist > fu.speed) {
			int dx = closestEnemy->x - fu.x, dy = closestEnemy->y - fu.y;

			fu.x += (int)(dx*(fu.speed / sqrt(dx*dx + dy*dy)));
			fu.y += (int)(dy*(fu.speed / sqrt(dx*dx + dy*dy)));
				
			didSomething = true;
			return;
		}
	}

	void FastAPproximation::medicsim(const FAPUnit & fu, std::vector<FAPUnit> &friendlyUnits) {
		auto closestHealable = friendlyUnits.end();
		int closestDist;

		for (auto it = friendlyUnits.begin(); it != friendlyUnits.end(); ++it) {
			if (it->isOrganic && it->health < it->maxHealth && !it->didHealThisFrame) {
				int d = dist(fu, *it);
				if (closestHealable == friendlyUnits.end() || d < closestDist) {
					closestHealable = it;
					closestDist = d;
				}
			}
		}

		if (closestHealable != friendlyUnits.end()) {
			fu.x = closestHealable->x;
			fu.y = closestHealable->y;

			closestHealable->health += (closestHealable->healTimer += 400) / 256;
			closestHealable->healTimer %= 256;

			if (closestHealable->health > closestHealable->maxHealth)
				closestHealable->health = closestHealable->maxHealth;

			closestHealable->didHealThisFrame = false;
		}
	}

	bool FastAPproximation::suicideSim(const FAPUnit & fu, std::vector<FAPUnit>& enemyUnits) {
		auto closestEnemy = enemyUnits.end();
		int closestDist;

		for (auto enemyIt = enemyUnits.begin(); enemyIt != enemyUnits.end(); ++enemyIt) {
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

		if (closestEnemy != enemyUnits.end() && closestDist <= fu.speed) {
			if(closestEnemy->flying)
				dealDamage(*closestEnemy, fu.airDamage, fu.airDamageType);
			else 
				dealDamage(*closestEnemy, fu.groundDamage, fu.groundDamageType);

			if (closestEnemy->health < 1) {
				auto temp = *closestEnemy;
				*closestEnemy = enemyUnits.back();
				enemyUnits.pop_back();
				unitDeath(temp, enemyUnits);
			}

			didSomething = true;
			return true;
		}
		else if (closestEnemy != enemyUnits.end() && closestDist > fu.speed) {
			int dx = closestEnemy->x - fu.x, dy = closestEnemy->y - fu.y;

			fu.x += (int)(dx*(fu.speed / sqrt(dx*dx + dy*dy)));
			fu.y += (int)(dy*(fu.speed / sqrt(dx*dx + dy*dy)));

			didSomething = true;
		}

		return false;
	}

	void FastAPproximation::isimulate() {
		for (auto fu = player1.begin(); fu != player1.end();) {
			if (isSuicideUnit(fu->unitType)) {
				bool result = suicideSim(*fu, player2);
				if (result)
					fu = player1.erase(fu);
				else
					++fu;
			}
			else {
				if (fu->unitType == BWAPI::UnitTypes::Terran_Medic)
					medicsim(*fu, player1);
				else
					unitsim(*fu, player2);
				++fu;
			}
		}

		for (auto fu = player2.begin(); fu != player2.end();) {
			if (isSuicideUnit(fu->unitType)) {
				bool result = suicideSim(*fu, player1);
				if (result)
					fu = player2.erase(fu);
				else
					++fu;
			}
			else {
				if (fu->unitType == BWAPI::UnitTypes::Terran_Medic)
					medicsim(*fu, player2);
				else
					unitsim(*fu, player1);
				++fu;
			}
		}

		for (auto &fu : player1) {
			if (fu.attackCooldownRemaining)
				--fu.attackCooldownRemaining;
			if (fu.didHealThisFrame)
				fu.didHealThisFrame = false;
		}

		for (auto &fu : player2) {
			if (fu.attackCooldownRemaining)
				--fu.attackCooldownRemaining;
			if (fu.didHealThisFrame)
				fu.didHealThisFrame = false;
		}
	}

	void FastAPproximation::unitDeath(const FAPUnit &fu, std::vector<FAPUnit> &itsFriendlies) {
		if (fu.unitType == BWAPI::UnitTypes::Terran_Bunker) {
			convertToUnitType(fu, BWAPI::UnitTypes::Terran_Marine);

			for(unsigned i = 0; i < 4; ++ i)
				itsFriendlies.push_back(fu);
		}
	}

	void FastAPproximation::convertToUnitType(const FAPUnit &fu, BWAPI::UnitType ut) {
		EnemyData ed;
		ed.lastPosition = { fu.x, fu.y };
		ed.lastPlayer = fu.player;
		ed.lastType = ut;

		FAPUnit funew(ed);
		funew.attackCooldownRemaining = fu.attackCooldownRemaining;
		funew.elevation = fu.elevation;

		fu.operator=(funew);
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

		unitType(ed.lastType),
		isOrganic(ed.lastType.isOrganic()),
		score(ed.lastType.destroyScore()),
		player(ed.lastPlayer) {

		static int nextId = 0;
		id = nextId++;

#ifdef SSCAIT

		if (ed.lastType == BWAPI::UnitTypes::Protoss_Carrier) {
			groundDamage = ed.lastPlayer->damage(BWAPI::UnitTypes::Protoss_Interceptor.groundWeapon());
			groundDamageType = BWAPI::UnitTypes::Protoss_Interceptor.groundWeapon().damageType();
			groundCooldown = 5;
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

		if (ed.u && ed.u->isStimmed()) {
			groundCooldown /= 2;
			airCooldown /= 2;
		}

		if (ed.u && ed.u->isVisible() && !ed.u->isFlying())
			elevation = BWAPI::Broodwar->getGroundHeight(ed.u->getTilePosition());

		groundMaxRange *= groundMaxRange;
		groundMinRange *= groundMinRange;
		airMaxRange *= airMaxRange;
		airMinRange *= airMinRange;

		groundDamage *= 2;
		airDamage *= 2;

		shieldArmor *= 2;
		armor *= 2;

		health *= 2;
		maxHealth *= 2;
		shields *= 2;
		maxShields *= 2;
	}

	const FastAPproximation::FAPUnit &FastAPproximation::FAPUnit::operator=(const FAPUnit & other) const {
		x = other.x, y = other.y;
		health = other.health, maxHealth = other.maxHealth;
		shields = other.shields, maxShields = other.maxShields;
		speed = other.speed, armor = other.armor, flying = other.flying, unitSize = other.unitSize;
		groundDamage = other.groundDamage, groundCooldown = other.groundCooldown, groundMaxRange = other.groundMaxRange, groundMinRange = other.groundMinRange, groundDamageType = other.groundDamageType;
		airDamage = other.airDamage, airCooldown = other.airCooldown, airMaxRange = other.airMaxRange, airMinRange = other.airMinRange, airDamageType = other.airDamageType;
		score = other.score;
		attackCooldownRemaining = other.attackCooldownRemaining;
		unitType = other.unitType; isOrganic = other.isOrganic;
		healTimer = other.healTimer; didHealThisFrame = other.didHealThisFrame;
		elevation = other.elevation;
		player = other.player;

		return *this;
	}

	bool FastAPproximation::FAPUnit::operator<(const FAPUnit & other) const {
		return id < other.id;
	}

}
