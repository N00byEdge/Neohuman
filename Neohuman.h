#pragma once

#include <BWAPI.h>

#include "SoundFile.h"
#include "Timer.h"

const auto isOnTournamentServer = []() {
#ifdef SSCAIT
  return true;
#else
  return false;
#endif
};

const auto isDebugBuild = []() {
#ifdef _DEBUG
  return true;
#else
  return false;
#endif
};

const auto botError = [](std::string &message) {
  if constexpr (!isOnTournamentServer()) {
    throw std::runtime_error(message);
  } else {
    BWAPI::Broodwar->sendText(message.c_str());
  }
};

const auto playableRaces = std::vector<BWAPI::Race>{
    BWAPI::Races::Protoss, BWAPI::Races::Terran, BWAPI::Races::Zerg};

struct Neohuman : public BWAPI::AIModule {
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
  bool wasRandom;
  BWAPI::Race playingRace;

  Neolib::Timer timer_drawinfo, timer_managequeue, timer_buildbuildings,
      timer_unitlogic, timer_marinelogic, timer_total;
};

extern Neohuman *neoInstance;
