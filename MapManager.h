#pragma once

#include "bwem.h"

#include <set>

namespace Neolib {

struct MapManager {
  void init();

  void onFrame();

  void MapManager::pathTo(BWAPI::Unit u, BWAPI::Position target);
  int getGroundDistance(BWAPI::Position start, BWAPI::Position end);

  const std::vector<const BWEM::Base *> &getAllBases() const;
  const std::set<const BWEM::Base *> &getUnexploredBases() const;

  BWAPI::TilePosition getNextBasePosition() const;
  BWAPI::TilePosition getNextBaseToScout();

  std::set<std::pair<BWAPI::TilePosition, BWAPI::TilePosition>>
  getNoBuildRects();

  BWAPI::Position getStartPosition();

private:
  std::queue<BWAPI::TilePosition> exploreBases;
  std::vector<const BWEM::Base *> allBases;
  std::set<const BWEM::Base *> unexploredBases;
};

} // namespace Neolib

extern Neolib::MapManager mapManager;
