#pragma once

#include "BWAPI.h"

#include "ResourceManager.h"
#include "SupplyManager.h"
#include "Triple.h"
#include "Util.h"

namespace Neolib {

struct ConstructionProject {
  BWAPI::TilePosition designatedLocation;
  BWAPI::UnitType buildingType;

  BWAPI::Unit builder = nullptr;
  BWAPI::Unit buildingUnit = nullptr;
};

struct BuildingQueue {
  bool doBuild(BWAPI::UnitType building,
               BWAPI::TilePosition at = BWAPI::TilePositions::None,
               BWAPI::Unit u = nullptr);

  ResourceCount getQueuedResources() const;
  SupplyCount getQueuedSupply(bool countResourceDepots = false) const;

  bool isWorkerBuilding(BWAPI::Unit) const;

  const std::list<ConstructionProject> &buildingsQueued();

  void onFrame();
  void onUnitComplete(BWAPI::Unit unit);
  void onUnitCreate(BWAPI::Unit unit);
  void onUnitDestroy(BWAPI::Unit unit);
  void onUnitMorph(BWAPI::Unit unit);

private:
  std::list<ConstructionProject> buildingQueue;
};

} // namespace Neolib

extern Neolib::BuildingQueue buildingQueue;
