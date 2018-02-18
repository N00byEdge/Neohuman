#include "BaseManager.h"
#include "Util.h"

#include "BuildingQueue.h"
#include "MapManager.h"
#include "UnitManager.h"

Neolib::BaseManager baseManager;

namespace Neolib {

Base::Base(BWAPI::Unit rd) : resourceDepot(rd), race(rd->getType().getRace()) {
  for (auto &b : mapManager.getAllBases())
    if (b->Location() == rd->getTilePosition())
      BWEMBase = b;

  // If we can't find the base, just take something close enough.
  if (!BWEMBase)
    for (auto &sl : BWEM::Map::Instance().StartingLocations())
      if (sl == rd->getTilePosition())
        for (auto &b : mapManager.getAllBases())
          if (ABS(b->Location().x - sl.x) + ABS(b->Location().y - sl.y) < 10)
            BWEMBase = b;

  if (BWEMBase == nullptr) {
    BWAPI::Broodwar->sendText("Unable to find BWEM base");
    return;
  }

  mainPos = BWEMBase->Location();

  for (auto &m : BWEMBase->Minerals())
    if (m->Unit()->exists())
      mineralMiners[m->Unit()];

  for (auto &gg : BWEMBase->Geysers()) {
    gasGeysers.insert(gg->Unit());
    for (auto &ut : unitManager.getFriendlyUnitsByType())
      if (ut.first.isRefinery())
        for (auto &b : ut.second)
          if (b->getTilePosition() == gg->TopLeft()) {
            gasGeysers.erase(b);
            gasMiners[b];
          }
  }
}

ResourceCount Base::additionalWantedWorkers() const {
  ResourceCount sum;
  for (auto &m : mineralMiners)
    if (m.second.size() < 2)
      sum.minerals += 2 - (int)m.second.size();

  for (auto &g : gasMiners)
    if (g.second.size() < 3)
      sum.gas += 3 - (int)g.second.size();

  return sum;
}

ResourceCount Base::numWorkers() const {
  ResourceCount num;

  for (auto &m : mineralMiners)
    num.minerals += (int)m.second.size();

  for (auto &g : gasMiners)
    num.gas += (int)g.second.size();

  return num;
}

ResourceCount Base::calculateIncome() const {
  ResourceCount income;

  if (redAlert != -1)
    return income;

  for (auto &mf : mineralMiners)
    if (mf.second.size() == 0)
      continue;
    else if (mf.second.size() == 1)
      income.minerals += 65;
    else if (mf.second.size() == 2)
      income.minerals += 126;
    else
      income.minerals += 143;

  for (auto &gg : gasMiners)
    if (gg.first->getResources())
      income.gas += 103 * MIN((int)gg.second.size(), 3);
    else
      income.gas += 26 * MIN((int)gg.second.size(), 3);

  if (race == BWAPI::Races::Protoss)
    return ResourceCount((int)(1.055f * income.minerals), income.gas);
  else if (race == BWAPI::Races::Zerg)
    return ResourceCount((int)(1.032f * income.minerals), income.gas);
  else
    return income;
}

bool Base::operator<(const Base &other) const {
  return resourceDepot < other.resourceDepot;
}

bool Base::operator==(const Base &other) const {
  return resourceDepot == other.resourceDepot;
}

std::pair<BWAPI::TilePosition, BWAPI::TilePosition>
Base::getNoBuildRegion() const {
  int minX = resourceDepot->getTilePosition().x,
      maxX = minX + resourceDepot->getType().tileWidth() +
             (race == BWAPI::Races::Terran) * 2,
      minY = resourceDepot->getTilePosition().y,
      maxY = minY + resourceDepot->getType().tileHeight();

  for (auto &mf : mineralMiners) {
    MINE(minX, mf.first->getTilePosition().x);
    MINE(minY, mf.first->getTilePosition().y);
    MAXE(maxX, mf.first->getTilePosition().x + mf.first->getType().tileWidth());
    MAXE(maxY,
         mf.first->getTilePosition().y + mf.first->getType().tileHeight());
  }

  for (auto &gg : BWEMBase->Geysers()) {
    MINE(minX, gg->TopLeft().x);
    MINE(minY, gg->TopLeft().y);
    MAXE(maxX, gg->BottomRight().x);
    MAXE(maxY, gg->BottomRight().y);
  }

  return {BWAPI::TilePosition(minX - 1, minY - 1),
          BWAPI::TilePosition(maxX + 1, maxY + 1)};
}

std::size_t Base::hash::operator()(const Base &b) const {
  return std::hash<BWAPI::Unit>()(b.resourceDepot);
}

BaseManager::BaseManager() {}

BaseManager::~BaseManager() {}

void BaseManager::onUnitComplete(BWAPI::Unit unit) {
  if (unit->getType().isResourceDepot() &&
      unit->getPlayer() == BWAPI::Broodwar->self())
    bases.insert(Base(unit));

  if (unit->getType().isWorker() &&
      unit->getPlayer() == BWAPI::Broodwar->self())
    homelessWorkers.insert(unit);

  if (unit->getType().isRefinery() &&
      unit->getPlayer() == BWAPI::Broodwar->self())
    for (auto &b : bases)
      for (auto &g : b.BWEMBase->Geysers())
        if (g->TopLeft() == unit->getTilePosition())
          b.gasMiners[unit];
}

void BaseManager::onUnitDestroy(BWAPI::Unit unit) {
  // Check if base died
  if (unit->getType().isResourceDepot()) {
    auto it = bases.find(unit);

    // If this is a base
    if (it != bases.end()) {
      for (auto it = workerBaseLookup.begin(); it != workerBaseLookup.end();) {
        if (it->second->resourceDepot == unit)
          it = workerBaseLookup.erase(it);
        else
          ++it;
      }

      // Make workers idle
      for (auto &mf : it->mineralMiners) {
        for (auto &w : mf.second) {
          homelessWorkers.insert(w);
          workerBaseLookup.erase(w);
        }
      }

      for (auto &mf : it->gasMiners) {
        for (auto &w : mf.second) {
          homelessWorkers.insert(w);
          workerBaseLookup.erase(w);
        }
      }

      for (auto it2 = builders.begin(); it2 != builders.end();)
        if (it2->second == &(*it))
          it2 = builders.erase(it2);
        else
          ++it2;

      bases.erase(it);
    }
  }

  // Check if worker died
  if (unit->getType().isWorker()) {
    homelessWorkers.erase(unit);
    builders.erase(unit);
    // If managed, cleanup
    auto bpIt = workerBaseLookup.find(unit);

    if (bpIt != workerBaseLookup.end() && bpIt->second &&
        bpIt->second->resourceDepot->exists()) {
      for (auto &mf : bpIt->second->mineralMiners) {
        auto mineralIt = mf.second.find(unit);
        if (mineralIt != mf.second.end())
          mf.second.erase(mineralIt);
      }

      for (auto &gg : bpIt->second->gasMiners) {
        auto gasIt = gg.second.find(unit);
        if (gasIt != gg.second.end())
          gg.second.erase(gasIt);
      }

      workerBaseLookup.erase(bpIt);
    }
  }

  // Check if mineral field/gas died
  if (unit->getType().isMineralField() || unit->getType().isRefinery()) {
    for (auto &b : bases) {
      auto minIt = b.mineralMiners.find(unit);
      if (minIt != b.mineralMiners.end()) {
        for (auto &w : minIt->second) {
          workerBaseLookup.erase(w);
          homelessWorkers.insert(w);
        }
        b.mineralMiners.erase(minIt);
      }

      auto gasIt = b.gasMiners.find(unit);
      if (gasIt != b.gasMiners.end()) {
        for (auto &w : gasIt->second) {
          workerBaseLookup.erase(w);
          homelessWorkers.insert(w);
        }
        b.gasMiners.erase(gasIt);
      }
    }
  }

  for (auto &b : bases)
    b.gasGeysers.erase(unit);
}

void BaseManager::onUnitRenegade(BWAPI::Unit unit) {
  onUnitDestroy(unit);
  onUnitDiscover(unit);
}

void BaseManager::onUnitDiscover(BWAPI::Unit unit) {
  if (unit->getType().isMineralField())
    for (auto &b : bases)
      for (auto &mf : b.BWEMBase->Minerals())
        if (mf->Unit() == unit)
          b.mineralMiners[unit];

  if (unit->getType() == BWAPI::UnitTypes::Resource_Vespene_Geyser) {
    auto pos = unit->getTilePosition();
    for (auto &b : bases)
      if (b.BWEMBase)
        for (auto &gg : b.BWEMBase->Geysers())
          if (gg->Unit()->getTilePosition() == pos)
            b.gasGeysers.insert(unit);
  }
}

void BaseManager::onFrame() {
  for (auto it = homelessWorkers.begin(); it != homelessWorkers.end();) {
    if (!(*it)->isCompleted()) {
      ++it;
      continue;
    }

    ResourceCount wantedWorkers;
    ResourceCount numWorkers;

    for (auto &b : bases)
      wantedWorkers += b.additionalWantedWorkers(),
          numWorkers += b.numWorkers();

    if (wantedWorkers.gas &&
        (numWorkers.minerals > numWorkers.gas / 3 || !wantedWorkers.minerals)) {
      assignTo<AssignmentType::Gas>(*it);
      it = homelessWorkers.erase(it);
      continue;
    }

    if (wantedWorkers.minerals) {
      assignTo<AssignmentType::Minerals>(*it);
      it = homelessWorkers.erase(it);
      continue;
    }

    ++it;
  }

  for (auto &b : builders)
    if (b.first->isUnderAttack())
      b.second->redAlert = BWAPI::Broodwar->getFrameCount();

  for (auto &b : bases) {
    if (b.resourceDepot->isUnderAttack())
      b.redAlert = BWAPI::Broodwar->getFrameCount();

    for (auto &mf : b.mineralMiners)
      for (auto &w : mf.second)
        if (w->isUnderAttack())
          b.redAlert = BWAPI::Broodwar->getFrameCount();

    for (auto &gg : b.gasMiners)
      for (auto &w : gg.second)
        if (w->isUnderAttack())
          b.redAlert = BWAPI::Broodwar->getFrameCount();

    if (b.redAlert != -1) {
      if (b.redAlert < BWAPI::Broodwar->getFrameCount() - 32)
        b.redAlert = -1;
    }
  }

  for (auto &b : bases) {
    for (auto &mf : b.mineralMiners)
      for (auto w = mf.second.begin(); w != mf.second.end();) {
        auto bit = builders.find(*w);

        if (bit != builders.end() || (*w)->isConstructing())
          w = mf.second.erase(w);

        else {
          auto enemyUnit = unitManager.getClosestVisibleEnemy(
              *w, BWAPI::Filter::IsDetected, true);
          if (b.redAlert != -1 && enemyUnit &&
              ((*w)->getHitPoints() >= 20 ||
               (*w)->getDistance(mf.first) < 50) &&
              (*w)->getDistance(enemyUnit->u) <= workerAggroRadius &&
              (*w)->getOrderTarget() != mf.first)
            (*w)->attack(enemyUnit->u);
          else {
            if (((*w)->isCarryingMinerals() || (*w)->isCarryingGas()) &&
                ((*w)->getOrder() != BWAPI::Orders::ReturnGas &&
                 (*w)->getOrder() != BWAPI::Orders::ReturnMinerals))
              (*w)->returnCargo();
            else if (!((*w)->isCarryingMinerals() || (*w)->isCarryingGas()) &&
                     (*w)->getOrderTarget() != mf.first)
              (*w)->gather(mf.first);
          }

          ++w;
        }
      }

    for (auto &gg : b.gasMiners)
      for (auto w = gg.second.begin(); w != gg.second.end();) {
        auto bit = builders.find(*w);

        if (bit != builders.end() || (*w)->isConstructing())
          w = gg.second.erase(w);

        else {
          auto enemyUnit = unitManager.getClosestVisibleEnemy(
              *w, BWAPI::Filter::IsDetected, true);
          if (b.redAlert != -1 && enemyUnit.get() &&
              (*w)->getDistance(enemyUnit->u) <= workerAggroRadius)
            (*w)->attack(enemyUnit->u);
          else {
            if (((*w)->isCarryingMinerals() || (*w)->isCarryingGas()) &&
                (*w)->getOrderTarget() != b.resourceDepot)
              (*w)->returnCargo();
            else if (!((*w)->isCarryingMinerals() || (*w)->isCarryingGas()) &&
                     (*w)->getOrderTarget() != gg.first)
              (*w)->gather(gg.first);
          }

          ++w;
        }
      }
  }
}

void BaseManager::takeUnit(BWAPI::Unit unit) {
  auto it = homelessWorkers.find(unit);
  if (it != homelessWorkers.end()) {
    homelessWorkers.erase(unit);
    return;
  }

  auto baseIt = workerBaseLookup.find(unit);

  if (baseIt != workerBaseLookup.end()) {
    for (auto &mf : baseIt->second->mineralMiners) {
      auto it = mf.second.find(unit);
      if (it != mf.second.end()) {
        mf.second.erase(it);
        builders[unit] = &(*baseIt->second);
        return;
      }
    }

    for (auto &mf : baseIt->second->gasMiners) {
      auto it = mf.second.find(unit);
      if (it != mf.second.end()) {
        mf.second.erase(it);
        builders[unit] = &(*baseIt->second);
        return;
      }
    }
  }
}

void BaseManager::returnUnit(BWAPI::Unit unit) {
  builders.erase(unit);
  homelessWorkers.insert(unit);
}

BWAPI::Unit BaseManager::findBuilder(BWAPI::UnitType builderType) {
  if (!builderType.isBuilding() &&
      builderType.getRace() == BWAPI::Races::Zerg) {
    for (auto &b : bases) {
      if (b.race == BWAPI::Races::Zerg) {
        auto &larva = b.resourceDepot->getLarva();
        if (larva.size())
          return b.resourceDepot;
      }
    }
  }

  if (!builderType.isWorker()) {
    for (auto u : unitManager.getFriendlyUnitsByType(builderType))
      if (u->isIdle())
        return u;

    return false;
  }

  for (auto it = homelessWorkers.begin(); it != homelessWorkers.end(); ++it) {
    if ((*it)->getType() == builderType && (*it)->isCompleted()) {
      auto ret = *it;
      homelessWorkers.erase(it);
      return ret;
    }
  }

  {
    unsigned workersAtBestPatch;
    BWAPI::Unit preferredUnit = nullptr;

    for (auto &b : bases) {
      if (b.race != builderType.getRace())
        continue;

      for (auto min : b.mineralMiners) {
        if (min.second.size()) {
          if (preferredUnit == nullptr) {
            for (auto &u : min.second) {
              if (!u->isCarryingMinerals() &&
                  !buildingQueue.isWorkerBuilding(u)) {
                preferredUnit = *(min.second.begin());
                workersAtBestPatch = (int)min.second.size();
                continue;
              }
            }
          }

          if (preferredUnit == nullptr ||
              min.second.size() > workersAtBestPatch) {
            for (auto &u : min.second) {
              if (!u->isCarryingMinerals() &&
                  !buildingQueue.isWorkerBuilding(u)) {
                preferredUnit = *(min.second.begin());
                workersAtBestPatch = (int)min.second.size();
                continue;
              }
            }
          }
        }
      }
    }

    if (preferredUnit != nullptr)
      return preferredUnit;
  }

  {
    unsigned workersAtBestPatch;
    BWAPI::Unit preferredUnit = nullptr;

    for (auto &b : bases) {
      if (b.race != builderType.getRace())
        continue;

      for (auto min : b.mineralMiners) {
        if (min.second.size()) {
          if (preferredUnit == nullptr) {
            for (auto &u : min.second) {
              if (u->isCarryingMinerals() &&
                  !buildingQueue.isWorkerBuilding(u) &&
                  u->getHitPoints() == u->getType().maxHitPoints()) {
                preferredUnit = *(min.second.begin());
                workersAtBestPatch = (int)min.second.size();
                continue;
              }
            }
          }

          if (min.second.size() > workersAtBestPatch) {
            for (auto &u : min.second) {
              if (u->isCarryingMinerals() &&
                  !buildingQueue.isWorkerBuilding(u) &&
                  u->getHitPoints() == u->getType().maxHitPoints()) {
                preferredUnit = *(min.second.begin());
                workersAtBestPatch = (int)min.second.size();
                continue;
              }
            }
          }
        }
      }
    }

    if (preferredUnit != nullptr)
      return preferredUnit;
  }

  return nullptr;
}

BWAPI::Unit BaseManager::findClosestBuilder(BWAPI::UnitType builderType,
                                            BWAPI::Position at) {
  return findBuilder(builderType);
}

template <BaseManager::AssignmentType assignment>
void BaseManager::assignTo(BWAPI::Unit unit) {
  constexpr unsigned maxWorkers =
      assignment == AssignmentType::Minerals ? 2 : 3;
  // Clear worker for previous work
  for (auto &b : baseManager.getAllBases()) {
    for (auto &mf : b.mineralMiners)
      mf.second.erase(unit);
    for (auto &mf : b.gasMiners)
      mf.second.erase(unit);
  }

  std::remove_reference_t<decltype(*Base::mineralMiners.begin())>
      *bestResourceContainer = nullptr;
  Base const *bestBase = nullptr;
  unsigned bestResources;
  unsigned bestDistance;
  unsigned bestWorkers;

  const auto select = [&](const Base &b, const auto resourceContainer,
                          unsigned resources, unsigned distance,
                          unsigned workers) {
    bestResourceContainer = resourceContainer;
    bestBase = &b;
    bestResources = resources;
    bestDistance = distance;
    bestWorkers = workers;
  };

  for (auto &b : baseManager.getAllBases()) {
    auto &resource =
        assignment == AssignmentType::Minerals ? b.mineralMiners : b.gasMiners;

    for (auto &resourceContainer : resource) {
      decltype(bestResources) resources =
          static_cast<unsigned>(resourceContainer.first->getResources());
      decltype(bestDistance) distance = static_cast<unsigned>(
          resourceContainer.first->getDistance(b.resourceDepot));
      decltype(bestWorkers) workers =
          static_cast<unsigned>(resourceContainer.second.size());

      if (!bestResourceContainer && workers < maxWorkers) {
        select(b, &resourceContainer, resources, distance, workers);
        continue;
      }

      if (workers < bestWorkers) {
        select(b, &resourceContainer, resources, distance, workers);
        continue;
      }

      if (workers > bestWorkers) {
        continue;
      }

      if (distance < bestDistance) {
        select(b, &resourceContainer, resources, distance, workers);
        continue;
      }

      if (distance > bestDistance) {
        continue;
      }

      if (resources < bestResources) {
        select(b, &resourceContainer, resources, distance, workers);
        continue;
      }
    }
  }

  if (bestResourceContainer) {
    workerBaseLookup[unit] = bestBase;
    bestResourceContainer->second.insert(unit);
  }
}

const std::set<Base> &BaseManager::getAllBases() const { return bases; }

const std::set<BWAPI::Unit> &BaseManager::getHomelessWorkers() const {
  return homelessWorkers;
}

} // namespace Neolib
