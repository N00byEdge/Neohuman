#pragma once
#include <BWAPI.h>

#include <random>

#include "Triple.h"

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
  int _supplyBeingMade = 0;
  int _queuedSupply = 0;
  int _timeLastQueuedSupply = 0;
  bool _isExpanding = false;
  bool _isBuildingBarracks = false;
  int _nBarracks = 0;

  std::vector <Triple <int, BWAPI::UnitType, BWAPI::TilePosition>> _buildingQueue;

  std::mt19937 _rand = std::mt19937(std::default_random_engine{});

  int randint(int, int);

  bool doBuild(BWAPI::Unit, BWAPI::UnitType, BWAPI::TilePosition);


  template <typename T>
  T randele(std::vector <T>&);

};
