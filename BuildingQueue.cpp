#include "BuildingQueue.h"
#include "BWEM/bwem.h"

namespace Neolib {

	void BuildingQueue::update() {
		for (unsigned i = 0; i < buildingQueue.size(); ++i) {
			// Work on halted queue progress, SCV died
			if (BWAPI::Broodwar->getUnit(buildingQueue[i].first.first) == nullptr || !BWAPI::Broodwar->getUnit(buildingQueue[i].first.first)->exists() && !buildingQueue[i].second) {
				// Replace placing scv
				auto pos = BWAPI::Position(buildingQueue[i].first.third);
				BWAPI::Unit closestBuilder = BWAPI::Broodwar->getClosestUnit(pos, BWAPI::Filter::IsOwned && BWAPI::Filter::IsGatheringMinerals && !BWAPI::Filter::IsCarryingMinerals && BWAPI::Filter::CurrentOrder != BWAPI::Orders::PlaceBuilding && BWAPI::Filter::CurrentOrder != BWAPI::Orders::ConstructingBuilding);
				if (closestBuilder == nullptr || isWorkerBuilding(closestBuilder))
					continue;
				buildingQueue[i].first.first = closestBuilder->getID();
				closestBuilder->build(buildingQueue[i].second, buildingQueue[i].first.third);
			}
			if (BWAPI::Broodwar->getUnit(buildingQueue[i].first.first) == nullptr || !BWAPI::Broodwar->getUnit(buildingQueue[i].first.first)->exists() && buildingQueue[i].second) {
				// Replace building scv
				auto pos = BWAPI::Position(buildingQueue[i].first.third);
				BWAPI::Unit closestBuilder = BWAPI::Broodwar->getClosestUnit(pos, BWAPI::Filter::IsOwned && BWAPI::Filter::IsGatheringMinerals && !BWAPI::Filter::IsCarryingMinerals && BWAPI::Filter::CurrentOrder != BWAPI::Orders::PlaceBuilding && BWAPI::Filter::CurrentOrder != BWAPI::Orders::ConstructingBuilding);
				if (closestBuilder == nullptr || isWorkerBuilding(closestBuilder))
					continue;
				buildingQueue[i].first.first = closestBuilder->getID();
				closestBuilder->rightClick(BWAPI::Broodwar->getClosestUnit(pos, BWAPI::Filter::IsOwned && BWAPI::Filter::IsBeingConstructed));
			}

			// Building was placed, update status
			if (!BWAPI::Broodwar->getUnit(buildingQueue[i].first.first)->getOrder() != BWAPI::Orders::PlaceBuilding && BWAPI::Broodwar->getUnit(buildingQueue[i].first.first)->getOrder() == BWAPI::Orders::ConstructingBuilding)
				buildingQueue[i].second = true;

			// Building was placed and finished
			if (!BWAPI::Broodwar->getUnit(buildingQueue[i].first.first)->isConstructing() && buildingQueue[i].second && BWAPI::Broodwar->getClosestUnit(BWAPI::Position(buildingQueue[i].first.third), (BWAPI::Filter::IsBuilding))->getTilePosition() == buildingQueue[i].first.third) {
				buildingQueue.erase(buildingQueue.begin() + i--);
				continue;
			}

			// Building was not placed, retry placing:
			if (BWAPI::Broodwar->getUnit(buildingQueue[i].first.first)->getOrder() != BWAPI::Orders::PlaceBuilding && BWAPI::Broodwar->getUnit(buildingQueue[i].first.first)->getOrder() != BWAPI::Orders::ConstructingBuilding) {
				// Let's see if the location is buildable
				if (BWAPI::Broodwar->canBuildHere(buildingQueue[i].first.third, buildingQueue[i].first.second)) {
					// Let's just keep replacing there, and hope for the best.
					BWAPI::Broodwar->getUnit(buildingQueue[i].first.first)->build(buildingQueue[i].first.second, buildingQueue[i].first.third);
				}
				else if (buildingQueue[i].first.second != BWAPI::UnitTypes::Terran_Command_Center) {
					// If it's not a cc, let's find a new location for this building
					buildingQueue[i].first.third = BWAPI::Broodwar->getBuildLocation(buildingQueue[i].first.second, buildingQueue[i].first.third);
					BWAPI::Broodwar->getUnit(buildingQueue[i].first.first)->build(buildingQueue[i].first.second, buildingQueue[i].first.third);
				}
				else {
					// If cc fails to place, remove it from the queue
					buildingQueue.erase(buildingQueue.begin() + i--);
					continue;
				}
			}
		}
	}

	bool BuildingQueue::doBuild(BWAPI::Unit u, BWAPI::UnitType building, BWAPI::TilePosition at) {
		if (isWorkerBuilding(u))
			return false;

		// If this intersects with current building project
		for (auto &o : buildingQueue) {
			if (building == BWAPI::UnitTypes::Terran_Refinery || building == BWAPI::UnitTypes::Terran_Command_Center) {
				if (BWEM::utils::intersect(at.x, at.y, at.x + building.tileSize().x, at.x + building.tileSize().y,
					o.first.third.x, o.first.third.y, o.first.third.x + o.first.second.tileSize().x, o.first.third.y + o.first.second.tileSize().y))
					return false;
			}
			else {
				if (BWEM::utils::intersect(at.x, at.y, at.x + building.tileSize().x + 1, at.x + building.tileSize().y + 1,
					o.first.third.x, o.first.third.y, o.first.third.x + o.first.second.tileSize().x + 1, o.first.third.y + o.first.second.tileSize().y + 1))
					return false;
			}
		}


		if (u->build(building, at)) {
			buildingQueue.push_back({ Triple < int, BWAPI::UnitType, BWAPI::TilePosition > {u->getID(), building, at}, false });
			return true;
		}
		return false;
	}

	ResourceCount BuildingQueue::getQueuedResources() const {
		ResourceCount resourceCount;

		for (auto &o : this->buildingQueue)
			if (!o.second)
				resourceCount.minerals += o.first.second.mineralPrice(), resourceCount.gas += o.first.second.gasPrice();

		return resourceCount;
	}

	SupplyCount BuildingQueue::getQueuedSupply(bool countResourceDepots) const {
		SupplyCount supplyCount;
		for (auto &o : buildingQueue) {
			// Terran
			if (o.first.second == BWAPI::UnitTypes::Terran_Supply_Depot)
				supplyCount.terran += 16;
			else if (o.first.second == BWAPI::UnitTypes::Terran_Command_Center && countResourceDepots)
				supplyCount.terran += 20;

			// Protoss
			else if (o.first.second == BWAPI::UnitTypes::Protoss_Pylon)
				supplyCount.protoss += 16;
			else if (o.first.second == BWAPI::UnitTypes::Protoss_Nexus && countResourceDepots)
				supplyCount.protoss += 18;

			// Zerg
			else if (o.first.second == BWAPI::UnitTypes::Zerg_Overlord)
				supplyCount.zerg += 16;
			else if (o.first.second == BWAPI::UnitTypes::Zerg_Hatchery && countResourceDepots)
				supplyCount.zerg += 2;
		}

		return supplyCount;
	}

	bool BuildingQueue::isWorkerBuilding(BWAPI::Unit u) const {
		for (auto &o : buildingQueue)
			if (o.first.first == u->getID())
				return true;
		return false;
	}

	const std::vector <std::pair <Neolib::Triple <int, BWAPI::UnitType, BWAPI::TilePosition>, bool>> &BuildingQueue::buildingsQueued() {
		return buildingQueue;
	}

}
