#include "UnitManager.h"

#include "BuildingQueue.h"
#include "SoundDatabase.h"
#include "FAP.h"

#include "SquadManager.h"

std::set <std::shared_ptr<Neolib::EnemyData>> emptyEnemyDataSet;
std::set <BWAPI::Unit> emptyUnitset;

#define PROTOSSSHEILDREGEN	7
#define ZERGREGEN			8

Neolib::UnitManager unitManager;

int deathMatrixGround[(256 * 4)*(256 * 4)], deathMatrixAir[(256 * 4)*(256 * 4)];

namespace Neolib {

	EnemyData::EnemyData() {

	}

	EnemyData::EnemyData(BWAPI::Unit unit): u(unit) {
		if (unit)
			initFromUnit();
	}

	EnemyData::EnemyData(BWAPI::UnitType ut) : u(nullptr), lastType(ut), lastPlayer(BWAPI::Broodwar->self()), lastPosition({}), lastHealth(ut.maxHitPoints()), lastShields(ut.maxShields()) {
		
	}

	void EnemyData::initFromUnit() const {
		lastType = u->getType();
		lastPlayer = u->getPlayer();
		unitID = u->getID();
		updateFromUnit(u);
	}

	void EnemyData::updateFromUnit() const {
		updateFromUnit(u);
	}

	void EnemyData::updateFromUnit(const BWAPI::Unit unit) const {
		frameLastSeen = BWAPI::Broodwar->getFrameCount();
		lastPosition = unit->getPosition();
		if (!unit->isDetected())
			return;
		frameLastDetected = BWAPI::Broodwar->getFrameCount();
		lastHealth = unit->getHitPoints();
		lastShields = unit->getShields();
		isCompleted = unit->isCompleted();
	}

	int EnemyData::expectedHealth() const {
		if (lastType.getRace() == BWAPI::Races::Zerg)
			return MIN(((BWAPI::Broodwar->getFrameCount() - frameLastDetected) * ZERGREGEN) / 256 + lastHealth, lastType.maxHitPoints());
		return lastHealth;
	}

	int EnemyData::expectedShields() const {
		if (lastType.getRace() == BWAPI::Races::Protoss)
			return MIN(((BWAPI::Broodwar->getFrameCount() - frameLastDetected) * PROTOSSSHEILDREGEN) / 256 + lastShields, lastType.maxShields());
		return lastShields;
	}

	bool EnemyData::isFriendly() const {
		return lastPlayer == BWAPI::Broodwar->self();
	}

	bool EnemyData::isEnemy() const {
		return !isFriendly();
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

	const std::map <BWAPI::UnitType, std::set <std::shared_ptr <EnemyData>>> &UnitManager::getEnemyUnitsByType() const {
		return enemyUnitsByType;
	}

	const std::set <std::shared_ptr <EnemyData>> &UnitManager::getEnemyUnitsByType(BWAPI::UnitType ut) const {
		auto res = enemyUnitsByType.find(ut);
		if (res != enemyUnitsByType.end()) {
			return res->second;
		}
		return emptyEnemyDataSet;
	}

	const std::map <BWAPI::UnitType, std::set <BWAPI::Unit>> &UnitManager::getFriendlyUnitsByType() const {
		return friendlyUnitsByType;
	}

	const std::set <BWAPI::Unit> &UnitManager::getFriendlyUnitsByType(BWAPI::UnitType ut) const {
		auto res = friendlyUnitsByType.find(ut);
		if (res != friendlyUnitsByType.end()) {
			return res->second;
		}
		return emptyUnitset;
	}

	const std::set<std::shared_ptr<EnemyData>>& UnitManager::getNonVisibleEnemies() const {
		return nonVisibleEnemies;
	}

	const std::set<std::shared_ptr<EnemyData>>& UnitManager::getVisibleEnemies() const {
		return visibleEnemies;
	}

	const std::set<std::shared_ptr<EnemyData>>& UnitManager::getInvalidatedEnemies() const {
		return invalidatedEnemies;
	}

	const std::set<std::shared_ptr<EnemyData>>& UnitManager::getDeadEnemies() const {
		return deadEnemies;
	}

	std::shared_ptr<EnemyData> UnitManager::getClosestEnemy(BWAPI::Unit from, const BWAPI::UnitFilter &f, bool onlyWithWeapons) const {
		std::shared_ptr<EnemyData> closest = nullptr;
		int dist;

		if (onlyWithWeapons) {
			for (auto &u : unitManager.visibleEnemies)
				if (reallyHasWeapon(u->lastType)) {
					if (!(f)(u->u) || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
						continue;
					if (!closest) {
						closest = u;
						dist = from->getPosition().getApproxDistance(u->lastPosition);
						continue;
					}
					int thisDist = from->getPosition().getApproxDistance(u->lastPosition);
					if (thisDist < dist) {
						closest = u;
						dist = thisDist;
					}
				}
			for (auto &u : unitManager.nonVisibleEnemies)
				if (reallyHasWeapon(u->lastType)) {
					if (!(f)(u->u) || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
						continue;
					if (!closest) {
						closest = u;
						dist = from->getPosition().getApproxDistance(u->lastPosition);
						continue;
					}
					int thisDist = from->getPosition().getApproxDistance(u->lastPosition);
					if (thisDist < dist) {
						closest = u;
						dist = thisDist;
					}
				}
		}
		else {
			for (auto &u : unitManager.visibleEnemies) {
				if (!(f)(u->u) || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
					continue;
				if (!closest) {
					closest = u;
					dist = from->getPosition().getApproxDistance(u->lastPosition);
					continue;
				}
				int thisDist = from->getPosition().getApproxDistance(u->lastPosition);
				if (thisDist < dist) {
					closest = u;
					dist = thisDist;
				}
			}
			for (auto &u : unitManager.nonVisibleEnemies) {
				if (!(f)(u->u) || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
					continue;
				if (!closest) {
					closest = u;
					dist = from->getPosition().getApproxDistance(u->lastPosition);
					continue;
				}
				int thisDist = from->getPosition().getApproxDistance(u->lastPosition);
				if (thisDist < dist) {
					closest = u;
					dist = thisDist;
				}
			}
		}

		return closest;
	}

	std::shared_ptr <EnemyData> UnitManager::getClosestEnemy(BWAPI::Unit from, bool onlyWithWeapons) const {
		std::shared_ptr <EnemyData> closest = nullptr;
		int dist;

		if (onlyWithWeapons) {
			for (auto &u : visibleEnemies)
				if (reallyHasWeapon(u->lastType) && u->frameLastSeen + 1000 >= BWAPI::Broodwar->getFrameCount()) {
					if (!closest) {
						closest = u;
						dist = from->getPosition().getApproxDistance(u->lastPosition);
						continue;
					}
					int thisDist = from->getPosition().getApproxDistance(u->lastPosition);
					if (thisDist < dist) {
						closest = u;
						dist = thisDist;
					}
				}
			for (auto &u : nonVisibleEnemies)
				if (reallyHasWeapon(u->lastType) && u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount()) {
					if (!closest) {
						closest = u;
						dist = from->getPosition().getApproxDistance(u->lastPosition);
						continue;
					}
					int thisDist = from->getPosition().getApproxDistance(u->lastPosition);
					if (thisDist < dist) {
						closest = u;
						dist = thisDist;
					}
				}
		}
		else {
			for (auto &u : visibleEnemies) {
				if (u->positionInvalidated || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
					continue;
				if (!closest) {
					closest = u;
					dist = from->getPosition().getApproxDistance(u->lastPosition);
					continue;
				}
				int thisDist = from->getPosition().getApproxDistance(u->lastPosition);
				if (thisDist < dist) {
					closest = u;
					dist = thisDist;
				}
			}
			for (auto &u : nonVisibleEnemies) {
				if (u->positionInvalidated || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
					continue;
				if (!closest) {
					closest = u;
					dist = from->getPosition().getApproxDistance(u->lastPosition);
					continue;
				}
				int thisDist = from->getPosition().getApproxDistance(u->lastPosition);
				if (thisDist < dist) {
					closest = u;
					dist = thisDist;
				}
			}
		}
			
		return closest;
	}

	std::shared_ptr <EnemyData> UnitManager::getClosestVisibleEnemy(BWAPI::Unit from, const BWAPI::UnitFilter &f, bool onlyWithWeapons) const {
		std::shared_ptr <EnemyData> closest = nullptr;
		int dist;

		if (onlyWithWeapons) {
			for (auto &u : unitManager.visibleEnemies)
				if (reallyHasWeapon(u->lastType)) {
					if (!(f)(u->u) || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
						continue;
					if (!closest) {
						closest = u;
						dist = from->getPosition().getApproxDistance(u->lastPosition);
						continue;
					}
					int thisDist = from->getPosition().getApproxDistance(u->lastPosition);
					if (thisDist < dist) {
						closest = u;
						dist = thisDist;
					}
				}
		}
		else {
			for (auto &u : unitManager.visibleEnemies) {
				if (!(f)(u->u) || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
					continue;
				if (!closest) {
					closest = u;
					dist = from->getPosition().getApproxDistance(u->lastPosition);
					continue;
				}
				int thisDist = from->getPosition().getApproxDistance(u->lastPosition);
				if (thisDist < dist) {
					closest = u;
					dist = thisDist;
				}
			}
		}

		return closest;
	}

	std::shared_ptr <EnemyData> UnitManager::getClosestVisibleEnemy(BWAPI::Unit from, bool onlyWithWeapons) const {
		std::shared_ptr <EnemyData> closest = nullptr;
		int dist;

		if (onlyWithWeapons) {
			for (auto &u : visibleEnemies)
				if (reallyHasWeapon(u->lastType) && u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount()) {
					if (!closest) {
						closest = u;
						dist = from->getPosition().getApproxDistance(u->lastPosition);
						continue;
					}
					int thisDist = from->getPosition().getApproxDistance(u->lastPosition);
					if (thisDist < dist) {
						closest = u;
						dist = thisDist;
					}
				}
		}
		else {
			for (auto &u : visibleEnemies) {
				if (u->positionInvalidated || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
					continue;
				if (!closest) {
					closest = u;
					dist = from->getPosition().getApproxDistance(u->lastPosition);
					continue;
				}
				int thisDist = from->getPosition().getApproxDistance(u->lastPosition);
				if (thisDist < dist) {
					closest = u;
					dist = thisDist;
				}
			}
		}

		return closest;
	}

	std::shared_ptr <EnemyData> UnitManager::getClosestNonVisibleEnemy(BWAPI::Unit from, const BWAPI::UnitFilter &f, bool onlyWithWeapons) const {
		std::shared_ptr <EnemyData> closest = nullptr;
		int dist;

		if (onlyWithWeapons) {
			for (auto &u : unitManager.nonVisibleEnemies)
				if (reallyHasWeapon(u->lastType)) {
					if (!(f)(u->u) || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
						continue;
					if (!closest) {
						closest = u;
						dist = from->getPosition().getApproxDistance(u->lastPosition);
						continue;
					}
					int thisDist = from->getPosition().getApproxDistance(u->lastPosition);
					if (thisDist < dist) {
						closest = u;
						dist = thisDist;
					}
				}
		}
		else {
			for (auto &u : unitManager.nonVisibleEnemies) {
				if (!(f)(u->u) || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
					continue;
				if (!closest) {
					closest = u;
					dist = from->getPosition().getApproxDistance(u->lastPosition);
					continue;
				}
				int thisDist = from->getPosition().getApproxDistance(u->lastPosition);
				if (thisDist < dist) {
					closest = u;
					dist = thisDist;
				}
			}
		}

		return closest;
	}

	std::shared_ptr <EnemyData> UnitManager::getClosestNonVisibleEnemy(BWAPI::Unit from, bool onlyWithWeapons) const {
		std::shared_ptr <EnemyData> closest = nullptr;
		int dist;

		if (onlyWithWeapons) {
			for (auto &u : nonVisibleEnemies)
				if (reallyHasWeapon(u->lastType) && u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount()) {
					if (!closest) {
						closest = u;
						dist = from->getPosition().getApproxDistance(u->lastPosition);
						continue;
					}
					int thisDist = from->getPosition().getApproxDistance(u->lastPosition);
					if (thisDist < dist) {
						closest = u;
						dist = thisDist;
					}
				}
		}
		else {
			for (auto &u : nonVisibleEnemies) {
				if (u->positionInvalidated || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
					continue;
				if (!closest) {
					closest = u;
					dist = from->getPosition().getApproxDistance(u->lastPosition);
					continue;
				}
				int thisDist = from->getPosition().getApproxDistance(u->lastPosition);
				if (thisDist < dist) {
					closest = u;
					dist = thisDist;
				}
			}
		}

		return closest;
	}

	std::shared_ptr<EnemyData> UnitManager::getEnemyData(BWAPI::Unit ptr) {
		auto it = enemyUnits.find(ptr->getID());
		if (it == enemyUnits.end())
			return nullptr;
		else return it->second;
	}

	std::shared_ptr<EnemyData> UnitManager::getBestTarget(BWAPI::Unit from) {
		int bestVal;
		std::shared_ptr<EnemyData> ret;

		for (auto &u : visibleEnemies) {
			int val = targetPriority(from, u.get());
			if (!ret || val > bestVal) {
				bestVal = val;
				ret = u;
			}
		}

		for (auto &u : nonVisibleEnemies) {
			int val = targetPriority(from, u.get());
			if (!ret || val > bestVal) {
				bestVal = val;
				ret = u;
			}
		}

		return ret;
	}

	int UnitManager::getNukeScore(BWAPI::Position pos, BWAPI::Unit from) const {
		int sum = 0;

		for (auto u : visibleEnemies)
			if(u->lastPosition.getDistance(pos) < 10 * 32)
				sum += u->lastType.destroyScore();

		for (auto u : nonVisibleEnemies)
			if (u->lastPosition.getDistance(pos) < 10 * 32)
				sum += u->lastType.destroyScore();

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
			int score = getNukeScore(possibleTarget->lastPosition, from);

			for(auto &nd: BWAPI::Broodwar->getNukeDots())
				if (nd.getApproxDistance(possibleTarget->lastPosition) < 32 * 10)
					goto skipUnit;

			if (pos == BWAPI::Positions::None || score > bestScore) {
				pos = possibleTarget->lastPosition;
				bestScore = score;
			}

			skipUnit:;
		}

		if (pos == BWAPI::Positions::None || getNukeScore(pos, nullptr) < 2000) // Compare absolute score, no ghost specific
			return BWAPI::Positions::None;
		else
			return pos;
	}

	int UnitManager::deathPerHealth(EnemyData *ed, bool flyingTarget) {
		int death = flyingTarget ? unitDeathAir(ed->lastType) : unitDeathGround(ed->lastType);
		if (!death || ed->expectedHealth() + ed->expectedShields() + ed->u->getDefenseMatrixPoints() == 0) return 0;
		return (death * 100) / (ed->expectedHealth() + ed->expectedShields());
	}

	int UnitManager::targetPriority(BWAPI::Unit f, EnemyData *ed) {
		if (ed->lastType.isInvincible())
			return -100000000;

		int val = 0;

		if (ed->lastType == BWAPI::UnitTypes::Zerg_Egg || BWAPI::UnitTypes::Zerg_Larva)
			val -= 100;

		if (reallyHasWeapon(ed->lastType))
			val += 1000;

		if (ed->lastType == BWAPI::UnitTypes::Terran_Medic) {
			if (ed->lastHealth == 0)
				val += 1010;
			else
				val += 1010 + (unitDeathAir(BWAPI::UnitTypes::Terran_Marine) * 100) / ed->lastHealth;
		}

		if ((ed->lastType.canProduce() && !ed->lastType.requiresPsi()) || ed->lastType == BWAPI::UnitTypes::Protoss_Pylon)
			val += 980;

		val += deathPerHealth(ed, f->isFlying());
		val -= ed->lastPosition.getApproxDistance(f->getPosition()) * 10;

		if (!ed->lastType.canProduce() && !reallyHasWeapon(ed->lastType) && isOnFire(*ed))
			val -= 250;

		return val;
	}

	bool UnitManager::isEnemy(BWAPI::Unit u) {
		return BWAPI::Broodwar->self()->isEnemy(u->getPlayer());
	}

	bool UnitManager::isOwn(BWAPI::Unit u) {
		return u->getPlayer() == BWAPI::Broodwar->self();
	}

	bool UnitManager::isNeutral(BWAPI::Unit u) {
		return !isEnemy(u) && !isOwn(u);
	}

	bool UnitManager::reallyHasWeapon(const BWAPI::UnitType &ut) {
		return ut.groundWeapon().damageAmount() || ut == BWAPI::UnitTypes::Terran_Bunker || ut == BWAPI::UnitTypes::Protoss_High_Templar || ut == BWAPI::UnitTypes::Protoss_Carrier || ut == BWAPI::UnitTypes::Protoss_Reaver;
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

	SimResults UnitManager::getSimResults() {
		return sr;
	}

	unsigned UnitManager::getLaunchedNukeCount() const {
		return launchedNukeCount;
	}

	void UnitManager::onFrame() {
		// Update visible enemies
		for (auto &u : visibleEnemies) 
			u->updateFromUnit();

		// Invalidate positions
		for (auto it = nonVisibleEnemies.begin(); it != nonVisibleEnemies.end();) {
			if (BWAPI::Broodwar->isVisible(BWAPI::TilePosition((*it)->lastPosition))) {
				(*it)->positionInvalidated = true;
				invalidatedEnemies.insert(*it);
				squadManager.onEnemyLose(*it);
				it = nonVisibleEnemies.erase(it);
			}
			else ++it;
		}

		for (auto it = lockdownDB.begin(); it != lockdownDB.end();)
			if (it->second.second > BWAPI::Broodwar->getFrameCount() + 24 * 3)
				it = lockdownDB.erase(it);
			else
				++it;

		doMultikillDetector();
		doCombatSim();
	}

	void UnitManager::onNukeDetect(BWAPI::Position target) {
		++launchedNukeCount;

		BWAPI::Broodwar->sendText("You may want to know that there is a nuke coming for you at %d %d", target.x, target.y);
		BWAPI::Broodwar->sendText("Nuke score: %d", getNukeScore(target, nullptr));
	}

	void UnitManager::onUnitDiscover(BWAPI::Unit unit) {
		if (isEnemy(unit)) {
			auto it = enemyUnits.find(unit->getID());

			if(it == enemyUnits.end()) {
				auto ed = std::make_shared <EnemyData>(unit);
				enemyUnits.insert({ unit->getID(), ed });
				enemyUnitsByType[unit->getType()].insert(ed);
				visibleEnemies.insert(ed);
			}

			else {
				if (it->second->lastPlayer != unit->getPlayer())
					onUnitRenegade(unit);
				
				if (it->second->lastType != unit->getType())
					onUnitMorph(unit);

				it->second->updateFromUnit();
			}
		} else if(isOwn(unit))
			friendlyUnitsByType[unit->getType()].insert(unit);
	}

	void UnitManager::onUnitShow(BWAPI::Unit unit) {
		auto ptr = enemyUnits.find(unit->getID());
		if (ptr != enemyUnits.end()) {
			nonVisibleEnemies.erase(ptr->second);
			invalidatedEnemies.erase(ptr->second);

			visibleEnemies.insert(ptr->second);
		}
	}

	void UnitManager::onUnitHide(BWAPI::Unit unit) {
		auto ptr = enemyUnits.find(unit->getID());
		if (ptr != enemyUnits.end()) {
			visibleEnemies.erase(ptr->second);

			nonVisibleEnemies.insert(ptr->second);
		}
	}

	void UnitManager::onUnitCreate(BWAPI::Unit unit) {

	}

	void UnitManager::onUnitDestroy(BWAPI::Unit unit) {
		if (unit->getPlayer() != BWAPI::Broodwar->self() && unit->getType().isResourceDepot())
			soundDatabase.playRandomHappySound();
		if (unit->getPlayer() == BWAPI::Broodwar->self() && unit->getType().isResourceDepot())
			soundDatabase.playRandomSadSound();

		if (isEnemy(unit)) {
			auto ed = enemyUnits[unit->getID()];
			enemyUnitsByType[unit->getType()].erase(ed);
			visibleEnemies.erase(ed);
			deadEnemies.insert(ed);
			squadManager.onEnemyLose(ed);

			++multikillDetector[unit->getType()];
		}

		else if (isOwn(unit)) {
			friendlyUnitsByType[unit->getType()].erase(unit);

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
		if (isOwn(unit)) {
			auto it = enemyUnits.find(unit->getID());
			if (it != enemyUnits.end()) {
				visibleEnemies.erase(it->second);
				enemyUnits.erase(it);
			}
		}
		else {
			friendlyUnitsByType[unit->getType()].erase(unit);
			if (!friendlyUnitsByType[unit->getType()].size())
				friendlyUnitsByType.erase(unit->getType());
			onUnitDiscover(unit);
		}
	}

	void UnitManager::onUnitComplete(BWAPI::Unit unit) {
		if (unit->getPlayer() == BWAPI::Broodwar->self()) {
			friendlyUnitsByType[unit->getType()].insert(unit);
		}
	}

	void UnitManager::doCombatSim() {
		fap = FastAPproximation();

		for (auto &u : getFriendlyUnitsByType(BWAPI::UnitTypes::Terran_Marine))
			fap.addIfCombatUnitPlayer1(u);

		for (auto &u : getFriendlyUnitsByType(BWAPI::UnitTypes::Terran_Ghost))
			fap.addIfCombatUnitPlayer1(u);

		for (auto &u : getVisibleEnemies())
			if (!(u->lastType.isWorker() && (u && u->u->isAttacking())))
				fap.addIfCombatUnitPlayer2(*u);

		for (auto &u : getNonVisibleEnemies())
			if (!(u->lastType.isWorker() && (u && u->u->isAttacking())))
				fap.addIfCombatUnitPlayer2(*u);

		sr.presim.scores = fap.playerScores();
		sr.presim.unitCounts = { fap.getState().first->size(), fap.getState().second->size() };

		fap.simulate(24 * 3); // 3 seconds of combat

		sr.shortsim.scores = fap.playerScores();
		sr.shortsim.unitCounts = { fap.getState().first->size(), fap.getState().second->size() };
		int theirLosses = sr.presim.scores.second - sr.shortsim.scores.second;
		int ourLosses = sr.presim.scores.first - sr.shortsim.scores.first;
		sr.shortLosses = theirLosses - ourLosses;

		fap.simulate(24 * 60); // 60 seconds of combat

		sr.postsim.scores = fap.playerScores();
		sr.postsim.unitCounts = { fap.getState().first->size(), fap.getState().second->size() };
	}

	void UnitManager::doMultikillDetector() {
		int unitsKilledThisFrame = 0;
		for (auto &ut : multikillDetector)
			unitsKilledThisFrame += ut.second;

		if (unitsKilledThisFrame > 3) {
			soundDatabase.playRandomKillSound();
			BWAPI::Broodwar->sendText("%cMULTIKILL! %d Units killed!", BWAPI::Text::Yellow, unitsKilledThisFrame);
			for (auto &ut : multikillDetector)
				BWAPI::Broodwar->sendText("    %c%s: %d", BWAPI::Text::Yellow, noRaceName(ut.first.c_str()), ut.second);
		}

		multikillDetector.clear();
	}

}
