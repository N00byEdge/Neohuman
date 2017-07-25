#include "UnitManager.h"

#include "BuildingQueue.h"

#include "CombatSimulator.h"
#include "SoundDatabase.h"

std::unordered_set <Neolib::EnemyData, Neolib::EnemyData::hash> emptyenemydataset;
std::unordered_set <BWAPI::Unit> emptyUnitset;

#define PROTOSSSHEILDREGEN	7
#define ZERGREGEN			8

Neolib::UnitManager unitManager;

int deathMatrixGround[(256 * 4)*(256 * 4)], deathMatrixAir[(256 * 4)*(256 * 4)];

namespace Neolib {

	EnemyData::EnemyData() {

	}

	EnemyData::EnemyData(BWAPI::Unit unit): u(unit) {
		if (unit)
			updateFromUnit(u);
	}

	void EnemyData::updateFromUnit() const {
		frameLastSeen = BWAPI::Broodwar->getFrameCount();
		lastPosition = u->getPosition();
		lastType = u->getType();
		unitID = u->getID();
		if (!u->isDetected())
			return;
		lastHealth = u->getHitPoints();
		lastPlayer = u->getPlayer();
		lastShields = u->getShields();
		isCompleted = u->isCompleted();
	}

	void EnemyData::updateFromUnit(const BWAPI::Unit unit) const {
		frameLastSeen = BWAPI::Broodwar->getFrameCount();
		lastType = unit->getType();
		unitID = unit->getID();
		lastPosition = unit->getPosition();
		if (!unit->isDetected())
			return;
		lastHealth = unit->getHitPoints();
		lastPlayer = unit->getPlayer();
		lastShields = unit->getShields();
		isCompleted = unit->isCompleted();
	}

	int EnemyData::expectedHealth() const {
		if (lastType.getRace() == BWAPI::Races::Zerg)
			return MIN(((BWAPI::Broodwar->getFrameCount() - frameLastSeen) * ZERGREGEN) / 256 + lastHealth, lastType.maxHitPoints());
		return lastHealth;
	}

	int EnemyData::expectedShields() const {
		if (lastType.getRace() == BWAPI::Races::Protoss)
			return MIN(((BWAPI::Broodwar->getFrameCount() - frameLastSeen) * PROTOSSSHEILDREGEN) / 256 + lastShields, lastType.maxShields());
		return lastShields;
	}

	bool EnemyData::operator<(const EnemyData &other) const {
		return u < other.u;
	}

	bool EnemyData::operator==(const EnemyData& other) const {
		return u == other.u;
	}

	std::size_t EnemyData::hash::operator()(const EnemyData &ed) const {
		return std::hash <BWAPI::Unit>()(ed.u);
	}

	// const std::unordered_set <EnemyData> &UnitManager::getKnownEnemies() const {
	// 	return knownEnemies;
	// }

	const std::map <BWAPI::UnitType, std::unordered_set <EnemyData, EnemyData::hash>> &UnitManager::getEnemyUnitsByType() const {
		return enemyUnitsByType;
	}

	const std::map <BWAPI::UnitType, std::unordered_set <BWAPI::Unit>> &UnitManager::getFriendlyUnitsByType() const {
		return friendlyUnitsByType;
	}

	const std::unordered_set <EnemyData, EnemyData::hash> &UnitManager::getNonVisibleEnemies() const {
		return nonVisibleEnemies;
	}

	const std::unordered_set <EnemyData, EnemyData::hash> &UnitManager::getVisibleEnemies() const {
		return visibleEnemies;
	}
	
	const std::unordered_set <EnemyData, EnemyData::hash> &UnitManager::getInvalidatedEnemies() const {
		return invalidatedEnemies;
	}

	const std::unordered_set <EnemyData, EnemyData::hash> &UnitManager::getEnemyUnitsByType(BWAPI::UnitType ut) const {
		if (enemyUnitsByType.count(ut))
			return enemyUnitsByType.at(ut);
		else return emptyenemydataset;
	}

	const std::unordered_set <BWAPI::Unit> &UnitManager::getFriendlyUnitsByType(BWAPI::UnitType ut) const {
		if (friendlyUnitsByType.count(ut))
			return friendlyUnitsByType.at(ut);
		else return emptyUnitset;
	}

	int UnitManager::countUnit(BWAPI::UnitType t, const BWAPI::UnitFilter &filter, bool countQueued) const {
		if (!BWAPI::Broodwar->getAllUnits().size())
			return 0;

		int c = 0;

		for (auto u : BWAPI::Broodwar->getAllUnits())
			if (t == u->getType() && (filter)(u))
				c++;
		
		if (countQueued)
			for (auto &o : buildingQueue.buildingsQueued())
				if (o.buildingType == t)
					c++;

		return c;
	}

	int UnitManager::countFriendly(BWAPI::UnitType t, bool onlyWithWeapons, bool countQueued) const {
		int sum = 0;

		if (t == BWAPI::UnitTypes::AllUnits) {
			if (onlyWithWeapons)
				for (auto &ut : friendlyUnitsByType)
					if (reallyHasWeapon(ut.first))
						sum += ut.second.size();
				else
					for (auto &ut : friendlyUnitsByType)
						sum += ut.second.size();
		}

		else
			if (friendlyUnitsByType.count(t))
				sum = friendlyUnitsByType.at(t).size();

		if (countQueued) {
			if (t == BWAPI::UnitTypes::AllUnits)
				sum += buildingQueue.buildingsQueued().size();
			else
				for (auto &bo : buildingQueue.buildingsQueued())
					if (bo.buildingType == t)
						++sum;
		}

		return sum;
	}

	int UnitManager::countEnemy(BWAPI::UnitType t, bool onlyWithWeapons) const {
		int sum = 0;

		if (t == BWAPI::UnitTypes::AllUnits) {
			if (onlyWithWeapons)
				for (auto &ut : enemyUnitsByType)
					if (ut.first.groundWeapon() || ut.first.airWeapon())
						sum += ut.second.size();
					else
						for (auto &ut : enemyUnitsByType)
							sum += ut.second.size();
		}

		else
			sum = enemyUnitsByType.at(t).size();

		return sum;
	}

	bool UnitManager::isAllowedToLockdown(BWAPI::Unit target, BWAPI::Unit own) const {
		auto it = lockdownDB.find(target);
		return it == lockdownDB.end() || it->second.first == own;
	}

	void UnitManager::reserveLockdown(BWAPI::Unit target, BWAPI::Unit own) {
		lockdownDB[target] = { own, BWAPI::Broodwar->getFrameCount() };
	}

	unsigned UnitManager::getNumArmedSilos() const {
		unsigned num = 0;

		for (auto & ns : getFriendlyUnitsByType(BWAPI::UnitTypes::Terran_Nuclear_Silo))
			if (ns->hasNuke())
				++num;

		return num;
	}

	unsigned UnitManager::getNumArmingSilos() const {
		unsigned num = 0;

		for (auto & ns : getFriendlyUnitsByType(BWAPI::UnitTypes::Terran_Nuclear_Silo))
			if (!ns->isIdle() && !ns->hasNuke())
				++num;

		return num;
	}

	unsigned UnitManager::getNumUnarmedSilos() const {
		unsigned num = 0;

		for (auto & ns : getFriendlyUnitsByType(BWAPI::UnitTypes::Terran_Nuclear_Silo))
			if (ns->isIdle() && !ns->hasNuke())
				++num;

		return num;
	}

	/*BWAPI::Unit UnitManager::getClosestBuilder(BWAPI::Position pos, BWAPI::UnitType builds) const {
		BWAPI::Unit closest = nullptr;
		int lowestDist;
		for (auto &u : friendlyUnitsByType.at(builds.whatBuilds().first)) {
			if ((!BWAPI::Filter::IsCarryingMinerals && BWAPI::Filter::IsGatheringMinerals)(u)) {
				int dist = u->getDistance(pos);
				if (!closest || dist < lowestDist) {
					lowestDist = dist;
					closest = u;
				}
			}
		}

		return closest;
	}

	BWAPI::Unit UnitManager::getAnyBuilder(BWAPI::UnitType builds) const {
		for (auto &u : getFriendlyUnitsByType(builds.whatBuilds().first))
			if ((!BWAPI::Filter::IsCarryingMinerals && BWAPI::Filter::IsGatheringMinerals)(u))
				return u;

		return nullptr;
	} */

	EnemyData UnitManager::getClosestEnemy(BWAPI::Unit from, const BWAPI::UnitFilter &f, bool onlyWithWeapons) const {
		EnemyData closest = nullptr;
		int dist;

		if (onlyWithWeapons) {
			for (auto &u : unitManager.visibleEnemies)
				if (reallyHasWeapon(u.lastType)) {
					if (!(f)(u.u) || u.frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
						continue;
					if (!closest.u) {
						closest = u;
						dist = from->getPosition().getApproxDistance(u.lastPosition);
						continue;
					}
					int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
					if (thisDist < dist) {
						closest = u;
						dist = thisDist;
					}
				}
			for (auto &u : unitManager.nonVisibleEnemies)
				if (reallyHasWeapon(u.lastType)) {
					if (!(f)(u.u) || u.frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
						continue;
					if (!closest.u) {
						closest = u;
						dist = from->getPosition().getApproxDistance(u.lastPosition);
						continue;
					}
					int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
					if (thisDist < dist) {
						closest = u;
						dist = thisDist;
					}
				}
		}
		else {
			for (auto &u : unitManager.visibleEnemies) {
				if (!(f)(u.u) || u.frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
					continue;
				if (!closest.u) {
					closest = u;
					dist = from->getPosition().getApproxDistance(u.lastPosition);
					continue;
				}
				int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
				if (thisDist < dist) {
					closest = u;
					dist = thisDist;
				}
			}
			for (auto &u : unitManager.nonVisibleEnemies) {
				if (!(f)(u.u) || u.frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
					continue;
				if (!closest.u) {
					closest = u;
					dist = from->getPosition().getApproxDistance(u.lastPosition);
					continue;
				}
				int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
				if (thisDist < dist) {
					closest = u;
					dist = thisDist;
				}
			}
		}

		return closest;
	}

	EnemyData UnitManager::getClosestEnemy(BWAPI::Unit from, bool onlyWithWeapons) const {
		EnemyData closest = nullptr;
		int dist;

		if (onlyWithWeapons) {
			for (auto &u : visibleEnemies)
				if (reallyHasWeapon(u.lastType) && u.frameLastSeen + 1000 >= BWAPI::Broodwar->getFrameCount()) {
					if (!closest.u) {
						closest = u;
						dist = from->getPosition().getApproxDistance(u.lastPosition);
						continue;
					}
					int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
					if (thisDist < dist) {
						closest = u;
						dist = thisDist;
					}
				}
			for (auto &u : nonVisibleEnemies)
				if (reallyHasWeapon(u.lastType) && u.frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount()) {
					if (!closest.u) {
						closest = u;
						dist = from->getPosition().getApproxDistance(u.lastPosition);
						continue;
					}
					int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
					if (thisDist < dist) {
						closest = u;
						dist = thisDist;
					}
				}
		}
		else {
			for (auto &u : visibleEnemies) {
				if (u.positionInvalidated || u.frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
					continue;
				if (!closest.u) {
					closest = u;
					dist = from->getPosition().getApproxDistance(u.lastPosition);
					continue;
				}
				int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
				if (thisDist < dist) {
					closest = u;
					dist = thisDist;
				}
			}
			for (auto &u : nonVisibleEnemies) {
				if (u.positionInvalidated || u.frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
					continue;
				if (!closest.u) {
					closest = u;
					dist = from->getPosition().getApproxDistance(u.lastPosition);
					continue;
				}
				int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
				if (thisDist < dist) {
					closest = u;
					dist = thisDist;
				}
			}
		}
			
		return closest;
	}

	EnemyData UnitManager::getClosestVisibleEnemy(BWAPI::Unit from, const BWAPI::UnitFilter &f, bool onlyWithWeapons) const {
		EnemyData closest = nullptr;
		int dist;

		if (onlyWithWeapons) {
			for (auto &u : unitManager.visibleEnemies)
				if (reallyHasWeapon(u.lastType)) {
					if (!(f)(u.u) || u.frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
						continue;
					if (!closest.u) {
						closest = u;
						dist = from->getPosition().getApproxDistance(u.lastPosition);
						continue;
					}
					int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
					if (thisDist < dist) {
						closest = u;
						dist = thisDist;
					}
				}
		}
		else {
			for (auto &u : unitManager.visibleEnemies) {
				if (!(f)(u.u) || u.frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
					continue;
				if (!closest.u) {
					closest = u;
					dist = from->getPosition().getApproxDistance(u.lastPosition);
					continue;
				}
				int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
				if (thisDist < dist) {
					closest = u;
					dist = thisDist;
				}
			}
		}

		return closest;
	}

	EnemyData UnitManager::getClosestVisibleEnemy(BWAPI::Unit from, bool onlyWithWeapons) const {
		EnemyData closest = nullptr;
		int dist;

		if (onlyWithWeapons) {
			for (auto &u : visibleEnemies)
				if (reallyHasWeapon(u.lastType) && u.frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount()) {
					if (!closest.u) {
						closest = u;
						dist = from->getPosition().getApproxDistance(u.lastPosition);
						continue;
					}
					int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
					if (thisDist < dist) {
						closest = u;
						dist = thisDist;
					}
				}
		}
		else {
			for (auto &u : visibleEnemies) {
				if (u.positionInvalidated || u.frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
					continue;
				if (!closest.u) {
					closest = u;
					dist = from->getPosition().getApproxDistance(u.lastPosition);
					continue;
				}
				int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
				if (thisDist < dist) {
					closest = u;
					dist = thisDist;
				}
			}
		}

		return closest;
	}

	EnemyData UnitManager::getClosestNonVisibleEnemy(BWAPI::Unit from, const BWAPI::UnitFilter &f, bool onlyWithWeapons) const {
		EnemyData closest = nullptr;
		int dist;

		if (onlyWithWeapons) {
			for (auto &u : unitManager.nonVisibleEnemies)
				if (reallyHasWeapon(u.lastType)) {
					if (!(f)(u.u) || u.frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
						continue;
					if (!closest.u) {
						closest = u;
						dist = from->getPosition().getApproxDistance(u.lastPosition);
						continue;
					}
					int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
					if (thisDist < dist) {
						closest = u;
						dist = thisDist;
					}
				}
		}
		else {
			for (auto &u : unitManager.nonVisibleEnemies) {
				if (!(f)(u.u) || u.frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
					continue;
				if (!closest.u) {
					closest = u;
					dist = from->getPosition().getApproxDistance(u.lastPosition);
					continue;
				}
				int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
				if (thisDist < dist) {
					closest = u;
					dist = thisDist;
				}
			}
		}

		return closest;
	}

	EnemyData UnitManager::getClosestNonVisibleEnemy(BWAPI::Unit from, bool onlyWithWeapons) const {
		EnemyData closest = nullptr;
		int dist;

		if (onlyWithWeapons) {
			for (auto &u : nonVisibleEnemies)
				if (reallyHasWeapon(u.lastType) && u.frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount()) {
					if (!closest.u) {
						closest = u;
						dist = from->getPosition().getApproxDistance(u.lastPosition);
						continue;
					}
					int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
					if (thisDist < dist) {
						closest = u;
						dist = thisDist;
					}
				}
		}
		else {
			for (auto &u : nonVisibleEnemies) {
				if (u.positionInvalidated || u.frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
					continue;
				if (!closest.u) {
					closest = u;
					dist = from->getPosition().getApproxDistance(u.lastPosition);
					continue;
				}
				int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
				if (thisDist < dist) {
					closest = u;
					dist = thisDist;
				}
			}
		}

		return closest;
	}

	EnemyData UnitManager::getBestTarget(BWAPI::Unit from) {
		int bestVal = 0;
		EnemyData ret(nullptr);

		for (auto &u : visibleEnemies) {
			int val = targetPriority(from, u);
			if (ret.u == nullptr || val > bestVal) {
				bestVal = val;
				ret = u;
			}
		}

		for (auto &u : nonVisibleEnemies) {
			int val = targetPriority(from, u);
			if (ret.u == nullptr || val > bestVal) {
				bestVal = val;
				ret = u;
			}
		}

		return ret;
	}

	int UnitManager::getNukeScore(BWAPI::Position pos, BWAPI::Unit from) const {
		int sum = 0;

		for (auto u : visibleEnemies)
			if(u.lastPosition.getDistance(pos) < 10 * 32)
				sum += u.lastType.destroyScore();

		for (auto u : nonVisibleEnemies)
			if (u.lastPosition.getDistance(pos) < 10 * 32)
				sum += u.lastType.destroyScore();

		for (auto u : BWAPI::Broodwar->self()->getUnits())
			if(u->getType().isBuilding() && u->getPosition().getDistance(pos) < 10 * 32)
				sum -= u->getType().destroyScore();

		if (from)
			sum -= from->getDistance(pos);

		return sum;
	}

	BWAPI::Position UnitManager::getBestNuke(BWAPI::Unit from) const {
		BWAPI::Position pos = BWAPI::Positions::None;
		int bestScore;

		for (auto possibleTarget : visibleEnemies) {
			int score = getNukeScore(possibleTarget.lastPosition, from);

			for(auto &nd: BWAPI::Broodwar->getNukeDots())
				if (nd.getApproxDistance(possibleTarget.lastPosition) < 32 * 10)
					goto skipUnit;

			if (pos == BWAPI::Positions::None || score > bestScore) {
				pos = possibleTarget.lastPosition;
				bestScore = score;
			}

			skipUnit:;
		}

		if (pos == BWAPI::Positions::None || bestScore < 600)
			return BWAPI::Positions::None;
		else
			return pos;
	}

	int UnitManager::targetPriority(BWAPI::Unit f, BWAPI::Unit ag) {
		EnemyData ed(ag);
		return targetPriority(f, ed);
	}

	int UnitManager::deathPerHealth(EnemyData ed, bool flyingTarget) {
		int death = flyingTarget ? unitDeathAir(ed.lastType) : unitDeathGround(ed.lastType);
		if (!death || ed.expectedHealth() + ed.expectedShields() + ed.u->getDefenseMatrixPoints() == 0) return 0;
		return (death * 100) / (ed.expectedHealth() + ed.expectedShields());
	}

	int UnitManager::targetPriority(BWAPI::Unit f, EnemyData ed) {
		if (ed.lastType.isInvincible())
			return -100000000;

		int val = 0;

		if (ed.lastType == BWAPI::UnitTypes::Zerg_Egg || BWAPI::UnitTypes::Zerg_Larva)
			val -= 100;

		if (reallyHasWeapon(ed.lastType))
			val += 1000;

		if (ed.lastType == BWAPI::UnitTypes::Terran_Medic) {
			if (ed.lastHealth == 0)
				val += 1010;
			else
				val += 1010 + (unitDeathAir(BWAPI::UnitTypes::Terran_Marine) * 100) / ed.lastHealth;
		}

		if ((ed.lastType.canProduce() && !ed.lastType.requiresPsi()) || ed.lastType == BWAPI::UnitTypes::Protoss_Pylon)
			val += 980;

		val += deathPerHealth(ed, f->isFlying());
		val -= ed.lastPosition.getApproxDistance(f->getPosition());

		if (!ed.lastType.canProduce() && !reallyHasWeapon(ed.lastType) && isOnFire(ed))
			val -= 250;

		return val;
	}

	bool UnitManager::reallyHasWeapon(const BWAPI::UnitType &ut) {
		return ut.groundWeapon().damageAmount() || ut == BWAPI::UnitTypes::Terran_Bunker || ut == BWAPI::UnitTypes::Protoss_High_Templar || ut == BWAPI::UnitTypes::Protoss_Carrier || ut == BWAPI::UnitTypes::Protoss_Reaver;
	}

	BWAPI::Position UnitManager::lastKnownEnemyPosition(BWAPI::Unit unit) const {
		auto it = visibleEnemies.find(unit);
		if (it != visibleEnemies.end())
			return it->lastPosition;
		it = nonVisibleEnemies.find(unit);
		if (it != nonVisibleEnemies.end())
			return it->lastPosition;
		return BWAPI::Positions::Unknown;
	}

	inline bool UnitManager::isOnFire(EnemyData building) {
		return building.lastType.isBuilding() && building.isCompleted && building.lastType.getRace() == BWAPI::Races::Terran && building.lastHealth < building.lastType.maxHitPoints() / 3;
	}

	inline int UnitManager::unitDeathGround(BWAPI::UnitType ut) {
		if (ut.groundWeapon() == BWAPI::WeaponTypes::None) return 0;
		return ut.groundWeapon().damageAmount() * ut.groundWeapon().damageFactor() * 40 / ut.groundWeapon().damageCooldown();
	}

	inline int UnitManager::unitDeathAir(BWAPI::UnitType ut) {
		if (ut.airWeapon() == BWAPI::WeaponTypes::None) return 0;
		return ut.airWeapon().damageAmount() * ut.airWeapon().damageFactor() * 40 / ut.airWeapon().damageCooldown();
	}

	inline int UnitManager::unitDeath(BWAPI::UnitType ut) {
		return (unitDeathAir(ut) + unitDeathGround(ut))/2;
	}

	inline int UnitManager::deathPerHealth(BWAPI::UnitType ut, int health) {
		return unitDeath(ut) / health;
	}

	inline int UnitManager::deathPerHealth(BWAPI::Unit unit) {
		return deathPerHealth(unit->getType(), unit->getHitPoints());
	}

	inline void UnitManager::addToDeathMatrix(BWAPI::Position pos, BWAPI::UnitType ut, BWAPI::Player p) {
		const int mapH = BWAPI::Broodwar->mapHeight() * 4, mapW = BWAPI::Broodwar->mapWidth() * 4;
		if (ut.groundWeapon()) {
			const int range = p->weaponMaxRange(ut.groundWeapon()) / 8;
			const int death = unitDeathGround(ut);
			const int mx = pos.x + range > mapW ? mapW : pos.x + range;
			for (int dx = pos.x - range < 0 ? -pos.x : -range; dx <= mx; ++dx) {
				const int yw = (int)ceil(sqrt(range * range - dx * dx));
				const int minY = MAX(pos.y - yw, 0), maxY = MIN(pos.y + yw, mapH);
				for (int y = minY; y <= maxY; ++y)
					deathMatrixGround[y*deathMatrixSideLen + pos.x + dx] += death;
			}
		}

		if (ut.airWeapon()) {
			const int range = p->weaponMaxRange(ut.groundWeapon()) / 8;
			const int death = unitDeathAir(ut);
			const int mx = pos.x + range > mapW ? mapW : pos.x + range;
			for (int dx = pos.x - range < 0 ? -pos.x : -range; dx <= mx; ++dx) {
				const int yw = (int)ceil(sqrt(range * range - dx * dx));
				const int minY = MAX(pos.y - yw, 0), maxY = MIN(pos.y + yw, mapH);
				for (int y = minY; y <= maxY; ++y)
					deathMatrixAir[y*deathMatrixSideLen + pos.x + dx] += death;
			}
		}
	}

	int UnitManager::getScore() const {
		return score;
	}

	void UnitManager::onFrame() {
		// Update visible enemies
		for (auto unitIt = visibleEnemies.begin(); unitIt != visibleEnemies.end();) {
			if (!unitIt->u->isVisible()) {
				nonVisibleEnemies.insert(*unitIt);
				unitIt = visibleEnemies.erase(unitIt);
				continue;
			}
			unitIt->lastPosition = unitIt->u->getPosition();
			unitIt->frameLastSeen = BWAPI::Broodwar->getFrameCount();
			if (unitIt->u->isDetected()) {
				unitIt->lastShields = unitIt->u->getShields();
				unitIt->lastHealth = unitIt->u->getHitPoints();
			}
			if (unitIt->u->getType() != unitIt->lastType) {
				enemyUnitsByType[unitIt->lastType].erase(unitIt->u);
				unitIt->lastType = unitIt->u->getType();
				enemyUnitsByType[unitIt->lastType].insert(*unitIt);
			}
			else {
				auto typeIt = enemyUnitsByType.find(unitIt->lastType);
				if (typeIt != enemyUnitsByType.end()) {
					auto typeUnitIt = typeIt->second.find(unitIt->u);
					if (typeUnitIt != typeIt->second.end()) {
						typeUnitIt->updateFromUnit();
					}
				}
				
				++unitIt;
			}
		}

		// Invalidate positions
		for (auto it = nonVisibleEnemies.begin(); it != nonVisibleEnemies.end();) {
			if (BWAPI::Broodwar->isVisible(BWAPI::TilePosition(it->lastPosition))) {
				it->positionInvalidated = true;
				invalidatedEnemies.insert(*it);
				it = nonVisibleEnemies.erase(it);
			}
			else ++it;
		}

		// Clean up groups
		friendlyUnitsByType.clear();
		for (auto &u : friendlyUnits)
			friendlyUnitsByType[u.second].emplace(u.first);

		enemyUnitsByType.clear();
		for (auto &u : visibleEnemies)
			enemyUnitsByType[u.lastType].emplace(u);
		for (auto &u : nonVisibleEnemies)
			enemyUnitsByType[u.lastType].emplace(u);
		for (auto &u : invalidatedEnemies)
			enemyUnitsByType[u.lastType].emplace(u);

		/*for (auto it = friendlyUnitsByType.begin(); it != friendlyUnitsByType.end();) {
			if (it->second.empty())
				it = friendlyUnitsByType.erase(it);
			else ++it;
		}
		for (auto it = enemyUnitsByType.begin(); it != enemyUnitsByType.end();) {
			if (it->second.empty())
				it = enemyUnitsByType.erase(it);
			else ++it;
		}*/
		/*
		// Clear death matrices
		for (int i = 0; i < deathMatrixSize; ++i) {
			deathMatrixGround[i] = 0;
			deathMatrixAir[i] = 0;
		}

		// Construct death matrices
		for (auto unitIt = visibleEnemies.begin(); unitIt != visibleEnemies.end(); ++unitIt)
			addToDeathMatrix(BWAPI::Position(unitIt->lastPosition.x / 8, unitIt->lastPosition.y / 8), unitIt->lastType, unitIt->lastPlayer);
		for (auto unitIt = nonVisibleEnemies.begin(); unitIt != nonVisibleEnemies.end(); ++unitIt)
			addToDeathMatrix(BWAPI::Position(unitIt->lastPosition.x / 8, unitIt->lastPosition.y / 8), unitIt->lastType, unitIt->lastPlayer);
		*/

		if (BWAPI::Broodwar->getFrameCount() % 128 == 0) {
#ifdef ENABLE_SPARCRAFT
			combatSimulator.clear();
			for (auto &u : friendlyUnits)
				if (u.first->getType() != BWAPI::UnitTypes::Terran_SCV && SparCraft::System::UnitTypeSupported(u.first->getType()))
					combatSimulator.addUnitToSimulator(u.first->getID(), u.first->getPosition(), u.first->getType(), false, u.first->getHitPoints(), u.first->getShields());

			for (auto &u : visibleEnemies)
				if (SparCraft::System::UnitTypeSupported(u.lastType))
					combatSimulator.addUnitToSimulator(u.unitID, u.lastPosition, u.lastType, true, u.expectedHealth(), u.expectedShields());

			for (auto &u : nonVisibleEnemies)
				if (SparCraft::System::UnitTypeSupported(u.lastType))
					combatSimulator.addUnitToSimulator(u.unitID, u.lastPosition, u.lastType, true, u.expectedHealth(), u.expectedShields());
#endif // ENABLE_SPARCRAFT
		}

		for (auto it = lockdownDB.begin(); it != lockdownDB.end();)
			if (it->second.second > BWAPI::Broodwar->getFrameCount() + 24 * 3)
				it = lockdownDB.erase(it);
			else
				++it;
	}

	void UnitManager::onUnitDiscover(BWAPI::Unit unit) {
		if (unit->getPlayer()->isEnemy(BWAPI::Broodwar->self())) {
			auto ke = knownEnemies.find(unit);
			EnemyData ed;
			ed.u = unit;
			ed.frameLastSeen = BWAPI::Broodwar->getFrameCount();

			if (unit->isDetected()) {
				ed.lastHealth = unit->getHitPoints();
				ed.lastShields = unit->getShields();
			}

			ed.lastPosition = unit->getPosition();
			ed.lastType = unit->getType();
			ed.positionInvalidated = false;
			if (ke != knownEnemies.end()) {
				if (ke->lastType != unit->getType())
					enemyUnitsByType[ke->lastType].erase(unit);
				ke->lastType = unit->getType();

				enemyUnitsByType[ke->lastType].insert(ed);
			}
			else
				knownEnemies.insert(ed);

			invalidatedEnemies.erase(unit);
			nonVisibleEnemies.erase(unit);
			visibleEnemies.insert(ed);
		}
	}

	void UnitManager::onUnitShow(BWAPI::Unit unit) {

	}

	void UnitManager::onUnitHide(BWAPI::Unit unit) {

	}

	void UnitManager::onUnitCreate(BWAPI::Unit unit) {

	}

	void UnitManager::onUnitDestroy(BWAPI::Unit unit) {
		if (unit->getPlayer() != BWAPI::Broodwar->self() && unit->getType().isResourceDepot())
			soundDatabase.playRandomHappySound();
		if (unit->getPlayer() == BWAPI::Broodwar->self() && unit->getType().isResourceDepot())
			soundDatabase.playRandomSadSound();

		if (unit->getPlayer()->isEnemy(BWAPI::Broodwar->self())) {
			knownEnemies.erase(unit);
			enemyUnitsByType[unit->getType()].erase(unit);
			if (enemyUnitsByType[unit->getType()].empty())
				enemyUnitsByType.erase(unit->getType());
			visibleEnemies.erase(unit);
		}
		else if (unit->getPlayer() == BWAPI::Broodwar->self()) {
			friendlyUnitsByType[unit->getType()].erase(unit);
			friendlyUnits.erase(unit);

			for (auto it = lockdownDB.begin(); it != lockdownDB.end();) {
				if (it->second.first == unit)
					it = lockdownDB.erase(it);
				else
					++it;
			}
		}

	}

	void UnitManager::onUnitMorph(BWAPI::Unit unit) {
		
	}

	void UnitManager::onUnitEvade(BWAPI::Unit unit) {

	}

	void UnitManager::onUnitRenegade(BWAPI::Unit unit) {
		auto friendlyIt = friendlyUnits.find(unit);
		if (friendlyIt != friendlyUnits.end())  {
			friendlyUnits.erase(unit);
			friendlyUnitsByType[unit->getType()].erase(unit);
		}

		auto enemyIt = knownEnemies.find(unit);
		if (enemyIt != knownEnemies.end()) {
			knownEnemies.erase(unit);
			enemyUnitsByType[unit->getType()].erase(unit);
			visibleEnemies.erase(unit);
			nonVisibleEnemies.erase(unit);
			invalidatedEnemies.erase(unit);
		}

		onUnitDiscover(unit);
	}

	void UnitManager::onUnitComplete(BWAPI::Unit unit) {
		if (unit->getPlayer() == BWAPI::Broodwar->self()) {
			if (friendlyUnits.count(unit))
				friendlyUnitsByType[friendlyUnits[unit]].erase(unit);

			friendlyUnits[unit] = unit->getType();
			friendlyUnitsByType[unit->getType()].insert(unit);
		}
	}

}
