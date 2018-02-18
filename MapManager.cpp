#include "MapManager.h"

#include "BaseManager.h"
#include "BuildingQueue.h"
#include "DetectionManager.h"
#include "UnitManager.h"

//#include <algorithm>

BWAPI::TilePosition startPos;

Neolib::MapManager mapManager;

struct sortLocations {
  static bool fromStartPos(const BWEM::Base *lhs, const BWEM::Base *rhs) {
    auto ldist = mapManager.getGroundDistance((BWAPI::Position)lhs->Location(),
                                              (BWAPI::Position)startPos);
    auto rdist = mapManager.getGroundDistance((BWAPI::Position)rhs->Location(),
                                              (BWAPI::Position)startPos);
    if (ldist == -1)
      return false;
    if (rdist == -1)
      return true;
    return ldist < rdist;
  }
};

namespace Neolib {

void MapManager::init() {
  BWEM::Map::Instance().Initialize();
  BWEM::Map::Instance().EnableAutomaticPathAnalysis();

  for (auto &a : BWEM::Map::Instance().Areas())
    for (auto &b : a.Bases())
      allBases.push_back(&b);

  for (auto &b : allBases) {
    if (BWAPI::Broodwar->isVisible(b->Location())) {
      startPos = b->Location();
      std::sort(allBases.begin(), allBases.end(), sortLocations::fromStartPos);
      break;
    }
  }

  for (auto &b : BWEM::Map::Instance().StartingLocations()) {
    exploreBases.push(b);
  }

  for (auto &b : allBases)
    if (!b->Starting() &&
        getGroundDistance((BWAPI::Position)startPos, b->Center()) != -1)
      exploreBases.push(b->Location());
}

void MapManager::onFrame() {
  for (auto it = unexploredBases.begin(); it != unexploredBases.end(); ++it) {
    if (BWAPI::Broodwar->isVisible((*it)->Location())) {
      unexploredBases.erase(it);
      break;
    }
  }

  if (BWAPI::Broodwar->isVisible(exploreBases.front())) {
    auto firstBase = exploreBases.front();
    exploreBases.pop();
    exploreBases.push(firstBase);
    while (BWAPI::Broodwar->isVisible(exploreBases.front())) {
      if (exploreBases.front() == firstBase)
        break;
      exploreBases.push(exploreBases.front());
      exploreBases.pop();
    }
  }

  auto comsatEnergy = detectionManager.highestComsatEnergy();
  if (comsatEnergy >= 150)
    detectionManager.requestDetection((BWAPI::Position)exploreBases.front());
}

void MapManager::pathTo(BWAPI::Unit u, BWAPI::Position target) {
  bool moved = false;
  const auto move = [&](BWAPI::Position p) {
    if (u->getDistance(p) < 100) return;
    if (moved)
      u->move(p, true);
    else
      u->move(p);
    moved = true;
  };

  for (auto cpp : BWEM::Map::Instance().GetPath(u->getPosition(), target))
    move((BWAPI::Position)cpp->Center());
  move(target);
}

int MapManager::getGroundDistance(BWAPI::Position start, BWAPI::Position end) {
  int result;
  BWEM::Map::Instance().GetPath(start, end, &result);
  return result;
}

const std::vector<const BWEM::Base *> &MapManager::getAllBases() const {
  return allBases;
}
const std::set<const BWEM::Base *> &MapManager::getUnexploredBases() const {
  return unexploredBases;
}

BWAPI::TilePosition MapManager::getNextBasePosition() const {
  for (auto &b : allBases) {
    for (auto &ob : baseManager.getAllBases())
      if (ob.BWEMBase == b)
        goto skiplocation;

    for (auto &o : buildingQueue.buildingsQueued())
      if (o.buildingType.isResourceDepot() &&
          o.designatedLocation == b->Location())
        goto skiplocation;

    if (!BWAPI::Broodwar->isExplored(b->Location()) &&
        detectionManager.highestComsatEnergy() >= 100)
      detectionManager.requestDetection((BWAPI::Position)b->Location());
    if (!BWAPI::Broodwar->isExplored(b->Location()) ||
        BWAPI::Broodwar->canBuildHere(b->Location(),
                                      BWAPI::UnitTypes::Terran_Command_Center,
                                      nullptr, true)) {
      for (auto &eu : unitManager.getVisibleEnemies())
        if (eu->lastPosition.getApproxDistance(BWAPI::Position(b->Location())) <
            1000)
          goto skiplocation;
      for (auto &eu : unitManager.getNonVisibleEnemies())
        if (eu->lastPosition.getApproxDistance(BWAPI::Position(b->Location())) <
            500)
          goto skiplocation;

      return b->Location();
    skiplocation:;
    }
  }

  return BWAPI::TilePositions::None;
}

BWAPI::TilePosition MapManager::getNextBaseToScout() {
  return exploreBases.front();
}

std::set<std::pair<BWAPI::TilePosition, BWAPI::TilePosition>>
MapManager::getNoBuildRects() {
  std::set<std::pair<BWAPI::TilePosition, BWAPI::TilePosition>> rects;

  for (auto &area : BWEM::Map::Instance().Areas()) {
    for (auto &base : area.Bases()) {
      std::pair<BWAPI::TilePosition, BWAPI::TilePosition> rect;
      rect.first = base.Location() - BWAPI::TilePosition(-5, -5);
      rect.second = base.Location() +
                    BWAPI::UnitTypes::Terran_Command_Center.tileSize() +
                    BWAPI::TilePosition(5, 5);
    }
  }

  return rects;
}

BWAPI::Position MapManager::getStartPosition() {
  return (BWAPI::Position)startPos;
}

} // namespace Neolib
