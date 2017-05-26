#include "UnitManager.h"

#include "BuildingQueue.h"

std::unordered_set <Neolib::EnemyData, Neolib::EnemyData::hash> emptyenemydataset;
std::unordered_set <BWAPI::Unit> emptyUnitset;

#define PROTOSSSHEILDREGEN	7
#define ZERGREGEN			8

Neolib::UnitManager unitManager;

namespace Neolib{

	EnemyData::EnemyData() {

	}

	EnemyData::EnemyData(BWAPI::Unit unit): u(unit) {

	}

	int EnemyData::expectedHealth() const {
		if (lastType.getRace() == BWAPI::Races::Zerg)
			return MIN(((BWAPI::Broodwar->getFrameCount() - frameLastSeen) * ZERGREGEN) / 256 + lastHealth, lastType.maxHitPoints());
		return lastHealth;
	}

	int EnemyData::expectedShields() const {
		if (lastType.getRace() == BWAPI::Races::Protoss)
			return MIN(((BWAPI::Broodwar->getFrameCount() - frameLastSeen) * PROTOSSSHEILDREGEN) / 256 + lastShiedls, lastType.maxShields());
		return lastShiedls;
	}

	bool EnemyData::operator<(const EnemyData &other) const {
		return u < other.u;
	}

	bool EnemyData::operator==(const EnemyData& other) const {
		return u == other.u;
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

		BWAPI::Unit anyUnit = *(BWAPI::Broodwar->getAllUnits().begin());
		int c = 0;

		for (auto &u : anyUnit->getUnitsInRadius(999999, filter))
			if (u->exists() && u->getType() == t)
				c++;

		if (countQueued)
			for (auto &o : buildingQueue.buildingsQueued())
				if (o.first.second == t && !o.second)
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
					if (bo.first.second == t)
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

	BWAPI::Unit UnitManager::getClosestBuilder(BWAPI::Unit u) const {
		return u->getClosestUnit(BWAPI::Filter::IsOwned && BWAPI::Filter::IsWorker && !BWAPI::Filter::IsCarryingMinerals && BWAPI::Filter::IsGatheringMinerals);
	}

	BWAPI::Unit UnitManager::getAnyBuilder() const {
		for (auto &ut : friendlyUnitsByType)
			if (ut.first.isWorker())

				for (auto &u : ut.second)
					if (!u->isCarryingMinerals() && u->isGatheringMinerals())
						return u;

		BWAPI::Broodwar->sendText("No builder found");

		return nullptr;
	} 

	BWAPI::Unit UnitManager::getClosestEnemy(BWAPI::Unit from, const BWAPI::UnitFilter &f, bool onlyWithWeapons) const {
		BWAPI::Unit closest = nullptr;
		int dist;

		if (onlyWithWeapons) {
			for (auto &u : unitManager.visibleEnemies)
				if (reallyHasWeapon(u.lastType)) {
					if (!(f)(u.u))
						continue;
					if (!closest) {
						closest = u.u;
						dist = from->getPosition().getApproxDistance(u.lastPosition);
						continue;
					}
					int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
					if (thisDist < dist) {
						closest = u.u;
						dist = thisDist;
					}
				}
			for (auto &u : unitManager.nonVisibleEnemies)
				if (reallyHasWeapon(u.lastType)) {
					if (!(f)(u.u))
						continue;
					if (!closest) {
						closest = u.u;
						dist = from->getPosition().getApproxDistance(u.lastPosition);
						continue;
					}
					int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
					if (thisDist < dist) {
						closest = u.u;
						dist = thisDist;
					}
				}
		}
		else {
			for (auto &u : unitManager.visibleEnemies) {
				if (!(f)(u.u))
					continue;
				if (!closest) {
					closest = u.u;
					dist = from->getPosition().getApproxDistance(u.lastPosition);
					continue;
				}
				int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
				if (thisDist < dist) {
					closest = u.u;
					dist = thisDist;
				}
			}
			for (auto &u : unitManager.nonVisibleEnemies) {
				if (!(f)(u.u))
					continue;
				if (!closest) {
					closest = u.u;
					dist = from->getPosition().getApproxDistance(u.lastPosition);
					continue;
				}
				int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
				if (thisDist < dist) {
					closest = u.u;
					dist = thisDist;
				}
			}
		}

		return closest;
	}

	BWAPI::Unit UnitManager::getClosestEnemy(BWAPI::Unit from, bool onlyWithWeapons) const {
		BWAPI::Unit closest = nullptr;
		int dist;

		if (onlyWithWeapons) {
			for (auto &u : visibleEnemies)
				if (reallyHasWeapon(u.lastType)) {
					if (!closest) {
						closest = u.u;
						dist = from->getPosition().getApproxDistance(u.lastPosition);
						continue;
					}
					int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
					if (thisDist < dist) {
						closest = u.u;
						dist = thisDist;
					}
				}
			for (auto &u : nonVisibleEnemies)
				if (reallyHasWeapon(u.lastType)) {
					if (!closest) {
						closest = u.u;
						dist = from->getPosition().getApproxDistance(u.lastPosition);
						continue;
					}
					int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
					if (thisDist < dist) {
						closest = u.u;
						dist = thisDist;
					}
				}
		}
		else {
			for (auto &u : visibleEnemies) {
				if (u.positionInvalidated)
					continue;
				if (!closest) {
					closest = u.u;
					dist = from->getPosition().getApproxDistance(u.lastPosition);
					continue;
				}
				int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
				if (thisDist < dist) {
					closest = u.u;
					dist = thisDist;
				}
			}
			for (auto &u : nonVisibleEnemies) {
				if (u.positionInvalidated)
					continue;
				if (!closest) {
					closest = u.u;
					dist = from->getPosition().getApproxDistance(u.lastPosition);
					continue;
				}
				int thisDist = from->getPosition().getApproxDistance(u.lastPosition);
				if (thisDist < dist) {
					closest = u.u;
					dist = thisDist;
				}
			}
		}

		return closest;
	}

	bool UnitManager::reallyHasWeapon(const BWAPI::UnitType &ut) {
		return ut.groundWeapon().isValid() || ut == BWAPI::UnitTypes::Terran_Bunker || ut == BWAPI::UnitTypes::Protoss_High_Templar || ut == BWAPI::UnitTypes::Protoss_Carrier;
	}

	BWAPI::Position UnitManager::lastKnownEnemyPosition(BWAPI::Unit unit) const {
		auto it = visibleEnemies.find(unit);
		if (it != visibleEnemies.end())
			return it->lastPosition;
		it = nonVisibleEnemies.find(unit);
		if (it != nonVisibleEnemies.end())
			return it->lastPosition;
		return BWAPI::Position(-1, -1);
	}

	void UnitManager::onFrame() {
		redoloop:;
		for (auto &u : visibleEnemies) {
			if (!u.u->isVisible()) {
				nonVisibleEnemies.insert(u);
				visibleEnemies.erase(u);
				goto redoloop;
			}
			u.lastPosition = u.u->getPosition();
			u.lastShiedls = u.u->getShields();
			u.lastHealth = u.u->getHitPoints();
			u.frameLastSeen = BWAPI::Broodwar->getFrameCount();
			auto it = enemyUnitsByType[u.lastType].find(u);
			if (it != enemyUnitsByType[u.lastType].end()) {
				it->lastPosition = u.lastPosition;
				it->lastShiedls = u.lastShiedls;
				it->lastHealth = u.lastHealth;
				it->frameLastSeen = u.frameLastSeen;
			}
		}

		redoloop2:;
		for (auto &u : nonVisibleEnemies) {
			if (BWAPI::Broodwar->isVisible(BWAPI::TilePosition(u.lastPosition))) {
				u.positionInvalidated = true;
				invalidatedEnemies.insert(u);
				nonVisibleEnemies.erase(u);
				goto redoloop2;
			}
		}
	}

	void UnitManager::onUnitDiscover(BWAPI::Unit unit) {
		if (unit->getPlayer()->isEnemy(BWAPI::Broodwar->self())) {
			auto ke = knownEnemies.find(unit);
			EnemyData ed;
			ed.u = unit;
			ed.frameLastSeen = BWAPI::Broodwar->getFrameCount();
			ed.lastHealth = unit->getHitPoints();
			ed.lastShiedls = unit->getShields();
			ed.lastPosition = unit->getPosition();
			ed.lastType = unit->getType();
			ed.positionInvalidated = false;
			if (ke != knownEnemies.end()) {
				if (ke->lastType != unit->getType()) {
					enemyUnitsByType[ke->lastType].erase(unit);
					if (enemyUnitsByType[ke->lastType].empty())
						enemyUnitsByType.erase(ke->lastType);
				}
				ke->lastType = unit->getType();

				enemyUnitsByType[ke->lastType].insert(ed);
			}
			else {
				knownEnemies.insert(ed);
			}

			ke = invalidatedEnemies.find(unit);
			if (ke != invalidatedEnemies.end())
				invalidatedEnemies.erase(ke);

			ke = nonVisibleEnemies.find(unit);
			if (ke != nonVisibleEnemies.end())
				nonVisibleEnemies.erase(ke);

			visibleEnemies.insert(ed);
		}

		else if (unit->getPlayer() == BWAPI::Broodwar->self()) {
			auto ku = friendlyUnits.find(unit);
			if (ku != friendlyUnits.end())
				friendlyUnitsByType[ku->second].erase(unit);
			friendlyUnits[unit] = unit->getType();
			friendlyUnitsByType[unit->getType()].insert(unit);
		}
	}

	void UnitManager::onUnitShow(BWAPI::Unit unit) {

	}

	void UnitManager::onUnitHide(BWAPI::Unit unit) {

	}

	void UnitManager::onUnitCreate(BWAPI::Unit unit) {

	}

	void UnitManager::onUnitDestroy(BWAPI::Unit unit) {

		if (unit->getPlayer()->isEnemy(BWAPI::Broodwar->self())) {
			knownEnemies.erase(unit);
			enemyUnitsByType[unit->getType()].erase(unit);
			if (enemyUnitsByType[unit->getType()].empty())
				enemyUnitsByType.erase(unit->getType());
			visibleEnemies.erase(unit);
		}
		else if (unit->getPlayer() == BWAPI::Broodwar->self()) {
			friendlyUnitsByType[unit->getType()].erase(unit);
		}

	}

	void UnitManager::onUnitMorph(BWAPI::Unit unit) {

	}

	void UnitManager::onUnitRenegade(BWAPI::Unit unit) {

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
