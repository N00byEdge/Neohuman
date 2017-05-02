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

  int _nSupplyOverhead = 0;
  int _wantedExtraSupply = 0;
  int _timeLastQueuedSupply = 0;
  bool _isExpanding = false;
  bool _isBuildingBarracks = false;
  int _nBarracks = 0;


  std::vector <std::pair <Triple <int, BWAPI::UnitType, BWAPI::TilePosition>, bool>> _buildingQueue;

  bool doBuild(BWAPI::Unit, BWAPI::UnitType, BWAPI::TilePosition);
  std::pair <int, int> getQueuedResources() const;

  std::pair <int, int> getSpendableResources() const;
  int getQueuedSupply() const;

  void manageBuildingQueue();

  std::mt19937 _rand = std::mt19937(std::default_random_engine{});
  int randint(int, int);
  template <typename T>
  T randele(std::vector <T>&);

};
