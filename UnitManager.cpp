#include "UnitManager.h"

#include "BuildingQueue.h"

std::set <BWAPI::Unit> emptyUnitset;

namespace Neolib{

	const std::map <BWAPI::Unit, std::pair<BWAPI::Position, BWAPI::UnitType>> &UnitManager::getKnownEnemies() const {
		return knownEnemies;
	}

	const std::map <BWAPI::Unit, BWAPI::UnitType> &UnitManager::getUnitsInTypeSet() const {
		return unitsInTypeSet;
	}

	const std::map <BWAPI::UnitType, std::set <BWAPI::Unit>> &UnitManager::getEnemyUnitsByType() const {
		return enemyUnitsByType;
	}

	const std::map <BWAPI::UnitType, std::set <BWAPI::Unit>> &UnitManager::getFriendlyUnitsByType() const {
		return friendlyUnitsByType;
	}

	const std::set <BWAPI::Unit> &UnitManager::getEnemyUnitsByType(BWAPI::UnitType ut) const {
		if (enemyUnitsByType.count(ut))
			return enemyUnitsByType.at(ut);
		else return emptyUnitset;
	}

	const std::set <BWAPI::Unit> &UnitManager::getFriendlyUnitsByType(BWAPI::UnitType ut) const {
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
					if (onlyWithWeapons && ut.first.groundWeapon() || ut.first.airWeapon())
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
			for (auto &u : unitManager.knownEnemies)
				if (reallyHasWeapon(u.second.second)) {
					if (!(f)(u.first))
						continue;
					if (!closest) {
						closest = u.first;
						dist = from->getPosition().getApproxDistance(u.second.first);
						continue;
					}
					int thisDist = from->getPosition().getApproxDistance(u.second.first);
					if (thisDist < dist) {
						closest = u.first;
						dist = thisDist;
					}
				}
		}
		else {
			for (auto &u : unitManager.knownEnemies) {
				if (!(f)(u.first))
					continue;
				if (!closest) {
					closest = u.first;
					dist = from->getPosition().getApproxDistance(u.second.first);
					continue;
				}
				int thisDist = from->getPosition().getApproxDistance(u.second.first);
				if (thisDist < dist) {
					closest = u.first;
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
			for (auto &u : unitManager.knownEnemies)
				if (reallyHasWeapon(u.second.second)) {
					if (!closest) {
						closest = u.first;
						dist = from->getPosition().getApproxDistance(u.second.first);
						continue;
					}
					int thisDist = from->getPosition().getApproxDistance(u.second.first);
					if (thisDist < dist) {
						closest = u.first;
						dist = thisDist;
					}
				}
		}
		else {
			for (auto &u : unitManager.knownEnemies) {
				if (!closest) {
					closest = u.first;
					dist = from->getPosition().getApproxDistance(u.second.first);
					continue;
				}
				int thisDist = from->getPosition().getApproxDistance(u.second.first);
				if (thisDist < dist) {
					closest = u.first;
					dist = thisDist;
				}
			}
		}

		return closest;
	}

	bool UnitManager::reallyHasWeapon(const BWAPI::UnitType &ut) {
		return ut.groundWeapon() || ut.airWeapon() || ut == BWAPI::UnitTypes::Terran_Bunker || ut == BWAPI::UnitTypes::Protoss_High_Templar;
	}

	BWAPI::Position UnitManager::lastKnownEnemyPosition(BWAPI::Unit unit) const {
		return knownEnemies.at(unit).first;
	}

	void UnitManager::onFrame() {
		for (auto &u : knownEnemies)
			if (u.first->isVisible())
				u.second.first = u.first->getPosition();
	}

	void UnitManager::onUnitDiscover(BWAPI::Unit unit) {
		if (unit->getPlayer() == BWAPI::Broodwar->enemy() && !unit->getType().isAddon()) {

			if (knownEnemies.count(unit) && knownEnemies[unit].second != unit->getType()) // If there is a new type for the unit
				enemyUnitsByType[knownEnemies[unit].second].erase(unit);

			if (knownEnemies.count(unit) && enemyUnitsByType[knownEnemies[unit].second].empty())
				enemyUnitsByType.erase(knownEnemies[unit].second);

			if (knownEnemies.find(unit) == knownEnemies.end())
				enemyUnitsByType[unit->getType()].insert(unit);

			knownEnemies[unit] = { unit->getPosition(), unit->getType() };
		}
	}

	void UnitManager::onUnitShow(BWAPI::Unit unit) {

	}

	void UnitManager::onUnitHide(BWAPI::Unit unit) {

	}

	void UnitManager::onUnitCreate(BWAPI::Unit unit) {

	}

	void UnitManager::onUnitDestroy(BWAPI::Unit unit) {

		if (unit->getPlayer() == BWAPI::Broodwar->enemy() && !unit->getType().isAddon()) {
			knownEnemies.erase(unit);
			enemyUnitsByType[unit->getType()].erase(unit);
			if (enemyUnitsByType[unit->getType()].empty())
				enemyUnitsByType.erase(unit->getType());
		}

		if (unit->getPlayer() == BWAPI::Broodwar->self()) {
			friendlyUnitsByType[unit->getType()].erase(unit);
			unitsInTypeSet.erase(unit);
		}

	}

	void UnitManager::onUnitMorph(BWAPI::Unit unit) {

	}

	void UnitManager::onUnitRenegade(BWAPI::Unit unit) {

	}

	void UnitManager::onUnitComplete(BWAPI::Unit unit) {
		if (unit->getPlayer() == BWAPI::Broodwar->self()) {
			if (unitsInTypeSet.count(unit))
				friendlyUnitsByType[unitsInTypeSet[unit]].erase(unit);

			unitsInTypeSet[unit] = unit->getType();
			friendlyUnitsByType[unit->getType()].insert(unit);
		}
	}

}