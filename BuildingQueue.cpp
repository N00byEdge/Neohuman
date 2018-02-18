#include "BuildingQueue.h"
#include "bwem.h"

#include "BaseManager.h"
#include "BuildingPlacer.h"
#include "DrawingManager.h"
#include "UnitManager.h"
#include "MapManager.h"

Neolib::BuildingQueue buildingQueue;

namespace Neolib {

void BuildingQueue::onFrame() {
  if (BWAPI::Broodwar->getFrameCount() % 5 != 0)
    return;
  for (auto it = buildingQueue.begin(); it != buildingQueue.end();) {
    if ((!it->buildingUnit ||
         it->buildingType.getRace() == BWAPI::Races::Terran) &&
        (!it->builder || !it->builder->exists())) {
      it->builder = baseManager.findClosestBuilder(
          it->buildingType.whatBuilds().first,
          static_cast<BWAPI::Position>(it->designatedLocation));
      if (it->builder) {
        if (it->buildingUnit)
          it->builder->rightClick(it->buildingUnit);
        else
          it->builder->build(it->buildingType, it->designatedLocation);

        baseManager.takeUnit(it->builder);
      }
    }

    if (it->builder && BWAPI::Broodwar->getFrameCount() % 5 == 0) {
      auto order = it->builder->getOrder();

      auto closeEnemy = unitManager.getClosestEnemy(
          it->builder, !BWAPI::Filter::IsWorker, true);
      if (closeEnemy
          && closeEnemy->lastPosition.getDistance(it->builder->getPosition()) < 200
          && closeEnemy) {
        baseManager.returnUnit(it->builder);
        goto skipdo;
      }

      if (order == BWAPI::Orders::ConstructingBuilding)
        goto skipdo;
      if (order == BWAPI::Orders::PlaceBuilding)
        goto skipdo;
      if (!BWAPI::Broodwar->isExplored(it->designatedLocation) &&
          order == BWAPI::Orders::Move &&
          static_cast<BWAPI::TilePosition>(it->builder->getOrderTargetPosition()) ==
          it->designatedLocation)
        goto skipdo;
      if (!it->designatedLocation.isValid())
        goto skipdo;

      baseManager.takeUnit(it->builder);

      if (it->buildingUnit &&
        it->buildingType.getRace() == BWAPI::Races::Terran)
        it->builder->rightClick(it->buildingUnit);
      else if (!BWAPI::Broodwar->isExplored(it->designatedLocation)
          || mapManager.getGroundDistance(it->builder->getPosition(), static_cast<BWAPI::Position>(it->designatedLocation)) > 300)
        mapManager.pathTo(it->builder, static_cast<BWAPI::Position>(it->designatedLocation));
      else if (!it->buildingUnit)
        it->builder->build(it->buildingType, it->designatedLocation);
    }

  skipdo:;

    if (it->buildingUnit &&
        it->buildingUnit->getHitPoints() <
            0.05f * it->buildingUnit->getType().maxHitPoints()) {
      it->buildingUnit->cancelConstruction();
      if (it->builder)
        baseManager.returnUnit(it->builder);
      it = buildingQueue.erase(it);
    } else
      ++it;
  }
}

void BuildingQueue::onUnitComplete(BWAPI::Unit unit) {
  // Check if our building is done
  for (auto it = buildingQueue.begin(); it != buildingQueue.end();) {
    if (it->buildingUnit == unit) {
      if (it->buildingType.getRace() == BWAPI::Races::Terran && it->builder)
        baseManager.returnUnit(it->builder);
      it = buildingQueue.erase(it);
    } else {
      ++it;
    }
  }
}

void BuildingQueue::onUnitCreate(BWAPI::Unit unit) {
  // If our building was placed
  for (auto &o : buildingQueue) {
    if ((unit->getType() == BWAPI::UnitTypes::Zerg_Egg &&
         o.buildingType == unit->getBuildType()) ||
        (o.buildingType == unit->getType() &&
         o.designatedLocation == unit->getTilePosition())) {
      o.buildingUnit = unit;
      o.designatedLocation = unit->getTilePosition();
      if (o.buildingType.getRace() != BWAPI::Races::Terran) {
        if (o.buildingType.getRace() == BWAPI::Races::Protoss)
          baseManager.returnUnit(o.builder);
        o.builder = nullptr;
      }
    }
  }
}

void BuildingQueue::onUnitDestroy(BWAPI::Unit unit) {
  // Check if our builder died
  for (auto it = buildingQueue.begin(); it != buildingQueue.end(); ++it) {
    if (it->builder == unit) {
      // Find replacement builder
      it->builder = baseManager.findClosestBuilder(
          it->buildingType.whatBuilds().first,
          (BWAPI::Position)it->designatedLocation);
      if (it->builder) {
        baseManager.takeUnit(it->builder);
        if (it->buildingUnit && it->buildingUnit->exists()) {
          it->builder->rightClick(it->buildingUnit);
        }
        if (!it->buildingUnit) {
          it->builder->build(it->buildingType, it->designatedLocation);
        }
      }
    }
  }

  // Check if our building died
  for (auto it = buildingQueue.begin(); it != buildingQueue.end(); ++it) {
    if (it->buildingUnit == unit) {
      it->buildingUnit = nullptr;
      if (it->builder && it->builder->exists())
        it->builder->build(it->buildingType, it->designatedLocation);
    }
  }
}

void BuildingQueue::onUnitMorph(BWAPI::Unit unit) { onUnitCreate(unit); }

bool BuildingQueue::doBuild(BWAPI::UnitType building, BWAPI::TilePosition at,
                            BWAPI::Unit u) {
  if (at == BWAPI::TilePositions::None) {
    if (!u && !(u = baseManager.findBuilder(
                    building.whatBuilds().first))) // Could not a find builder
      return false;
    at = buildingPlacer.getBuildLocation(building, u->getTilePosition());
    if (at == BWAPI::TilePositions::None)
      return false;
  } else if (!u && !(u = baseManager.findClosestBuilder(
                         building.whatBuilds().first,
                         (BWAPI::Position)at))) // Could not a find builder
    return false;

  if (!building.isBuilding() &&
      building.getRace() == BWAPI::Races::Zerg) { // If we're making a zerg unit
    if (u->morph(building) || !resourceManager.canAfford(building)) {
      ConstructionProject cp;
      cp.builder = u;
      cp.buildingType = building;
      cp.designatedLocation = at;

      buildingQueue.emplace_back(cp);
      return true;
    } else
      return false;
  }

  // If build location intersects another build location, nope out
  BWAPI::TilePosition buildingEnd(at + building.tileSize());
  for (auto &o : buildingQueue) {
    BWAPI::TilePosition otherEnd(o.designatedLocation +
                                 o.buildingType.tileSize());
    if (at.x < o.designatedLocation.x) {
      if (at.y < o.designatedLocation.y) {
        if (o.designatedLocation.x < buildingEnd.x &&
            o.designatedLocation.y < buildingEnd.y)
          return false;
        else if (o.designatedLocation.x < buildingEnd.x && at.y < otherEnd.y)
          return false;
      }
    } else {
      if (at.y < o.designatedLocation.y) {
        if (at.x < otherEnd.x && o.designatedLocation.y < buildingEnd.y)
          return false;
      } else if (at.x < otherEnd.x && at.y < otherEnd.y)
        return false;
    }
  }

  if (u)
    baseManager.takeUnit(u);

  if (u->build(building, at) || !resourceManager.canAfford(building)) {
    ConstructionProject cp;
    cp.builder = u;
    cp.buildingType = building;
    cp.designatedLocation = at;

    buildingQueue.emplace_back(cp);
    return true;
  } else if (!BWAPI::Broodwar->isExplored(at)) {
    u->move((BWAPI::Position)at);

    ConstructionProject cp;
    cp.builder = u;
    cp.buildingType = building;
    cp.designatedLocation = at;

    buildingQueue.emplace_back(cp);
    return true;
  } else {
    drawingManager.failedLocations.insert({at, building});
    BWAPI::Broodwar->pingMinimap((BWAPI::Position)at);
    return false;
  }
}

ResourceCount BuildingQueue::getQueuedResources() const {
  ResourceCount resourceCount;

  for (auto &o : this->buildingQueue)
    if (!o.buildingUnit)
      resourceCount += o.buildingType;

  return resourceCount;
}

SupplyCount BuildingQueue::getQueuedSupply(bool countResourceDepots) const {
  SupplyCount supplyCount;
  for (auto &o : buildingQueue) {
    // Terran
    if (o.buildingType == BWAPI::UnitTypes::Terran_Supply_Depot)
      supplyCount.terran += 16;
    else if (o.buildingType == BWAPI::UnitTypes::Terran_Command_Center &&
             countResourceDepots)
      supplyCount.terran += 20;

    // Protoss
    else if (o.buildingType == BWAPI::UnitTypes::Protoss_Pylon)
      supplyCount.protoss += 16;
    else if (o.buildingType == BWAPI::UnitTypes::Protoss_Nexus &&
             countResourceDepots)
      supplyCount.protoss += 18;

    // Zerg
    else if (o.buildingType == BWAPI::UnitTypes::Zerg_Overlord)
      supplyCount.zerg += 16;
    else if (o.buildingType == BWAPI::UnitTypes::Zerg_Hatchery &&
             countResourceDepots)
      supplyCount.zerg += 2;
  }

  return supplyCount;
}

bool BuildingQueue::isWorkerBuilding(BWAPI::Unit u) const {
  for (auto &o : buildingQueue)
    if (o.builder == u)
      return true;
  return false;
}

const std::list<ConstructionProject> &BuildingQueue::buildingsQueued() {
  return buildingQueue;
}

} // namespace Neolib
