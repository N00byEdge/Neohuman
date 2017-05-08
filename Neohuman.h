#pragma once
#include <BWAPI.h>

#include <random>

#include "Triple.h"

#include "BWEM/bwem.h"

// Remember not to use "Broodwar" in any global class constructor!

class Neohuman: public BWAPI::AIModule {
public:
  virtual void onStart();
  virtual void onEnd(bool isWinner);
  virtual void onFrame();
  virtual void onSendText(std::string text);
  virtual void onReceiveText(BWAPI::Player player, std::string text);
  virtual void onPlayerLeft(BWAPI::Player player);
  virtual void onNukeDetect(BWAPI::Position target);
  virtual void onUnitDiscover(BWAPI::Unit unit);
  virtual void onUnitEvade(BWAPI::Unit unit);
  virtual void onUnitShow(BWAPI::Unit unit);
  virtual void onUnitHide(BWAPI::Unit unit);
  virtual void onUnitCreate(BWAPI::Unit unit);
  virtual void onUnitDestroy(BWAPI::Unit unit);
  virtual void onUnitMorph(BWAPI::Unit unit);
  virtual void onUnitRenegade(BWAPI::Unit unit);
  virtual void onSaveGame(std::string gameName);
  virtual void onUnitComplete(BWAPI::Unit unit);

  // Things

  std::vector <const BWEM::Base*> _allBases;
  std::set <const BWEM::Base*> _unexploredBases;

  std::map <BWAPI::Unit, std::pair<BWAPI::Position, BWAPI::UnitType>> _knownEnemies;
  std::map <BWAPI::Unit, BWAPI::UnitType> _unitsInTypeSet;

  std::map <BWAPI::UnitType, std::set <BWAPI::Unit>> _enemyUnitsByType;
  std::map <BWAPI::UnitType, std::set <BWAPI::Unit>> _unitsByType;

  std::vector <std::pair <Triple <int, BWAPI::UnitType, BWAPI::TilePosition>, bool>> _buildingQueue;

  bool doBuild(BWAPI::Unit, BWAPI::UnitType, BWAPI::TilePosition);
  std::pair <int, int> getQueuedResources() const;

  std::pair <int, int> getSpendableResources() const;
  int getQueuedSupply() const;

  int Neohuman::wantedSupplyOverhead() const;
  int Neohuman::additionalWantedSupply() const;

  bool canAfford(BWAPI::UnitType t) const;
  int countUnit(BWAPI::UnitType t, const BWAPI::UnitFilter &filter, bool countQueued) const;

  unsigned Neohuman::countFriendly(BWAPI::UnitType t, bool onlyWithWeapons, bool countQueued);
  unsigned Neohuman::countEnemies(BWAPI::UnitType t, bool onlyWithWeapons);

  void manageBuildingQueue();

  BWAPI::Unit getClosestBuilder(BWAPI::Unit) const;
  BWAPI::Unit getAnyBuilder() const;
  bool isWorkerBuilding(BWAPI::Unit u) const;

  BWAPI::TilePosition getNextExpansion();

  bool _didUseScanThisFrame = false;
  bool requestScan(BWAPI::Position);

  std::mt19937 _rand = std::mt19937(std::default_random_engine{});
  int randint(int, int);
  template <typename T>
  T randele(std::vector <T>&);

};
