#include "Neohuman.h"

#include "ResourceManager.h"

#include "bwem.h"

#include <set>

constexpr int saturationRadius = 300;
constexpr int workerAggroRadius = 500;

namespace Neolib {

struct Base {
  Base(BWAPI::Unit resourceDepot);

  BWAPI::Unit resourceDepot = nullptr;

  mutable BWAPI::TilePosition mainPos;
  mutable BWAPI::Race race;

  mutable const BWEM::Base *BWEMBase = nullptr;

  ResourceCount additionalWantedWorkers() const;
  ResourceCount numWorkers() const;
  ResourceCount calculateIncome() const;

  bool operator<(const Base &other) const;
  bool operator==(const Base &other) const;

  std::pair<BWAPI::TilePosition, BWAPI::TilePosition> getNoBuildRegion() const;

  struct hash {
    std::size_t operator()(const Base &b) const;
  };

  mutable int redAlert = -1;

  mutable std::map<BWAPI::Unit, std::set<BWAPI::Unit>> mineralMiners;
  mutable std::map<BWAPI::Unit, std::set<BWAPI::Unit>> gasMiners;
  mutable std::set<BWAPI::Unit> gasGeysers;
};

struct BaseManager {
  BaseManager();
  ~BaseManager();

  void onUnitComplete(BWAPI::Unit unit);
  void onUnitDestroy(BWAPI::Unit unit);
  void onUnitRenegade(BWAPI::Unit unit);
  void onUnitDiscover(BWAPI::Unit unit);
  void onFrame();

  void takeUnit(BWAPI::Unit unit);
  void returnUnit(BWAPI::Unit unit);

  BWAPI::Unit findBuilder(BWAPI::UnitType builderType);
  BWAPI::Unit findClosestBuilder(BWAPI::UnitType builderType,
                                 BWAPI::Position at);

  enum struct AssignmentType : unsigned { Minerals, Gas };

  template <AssignmentType assignment> void assignTo(BWAPI::Unit unit);

  const std::set<Base> &getAllBases() const;
  const std::set<BWAPI::Unit> &getHomelessWorkers() const;

private:
  std::set<Base> bases;
  std::map<BWAPI::Unit, const Base *> builders;

  std::set<BWAPI::Unit> homelessWorkers;
  std::map<BWAPI::Unit, const Base *> workerBaseLookup;
};
} // namespace Neolib

extern Neolib::BaseManager baseManager;
