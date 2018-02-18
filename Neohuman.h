#pragma once

#include <BWAPI.h>

#include <string>

#include "SoundFile.h"

const auto IsOnTournamentServer = []() {
#ifdef SSCAIT
  return true;
#else
  return false;
#endif
};

const auto IsDebugBuild = []() {
#ifdef _DEBUG
  return true;
#else
  return false;
#endif
};

const auto BotError = [](std::string &message) {
  if constexpr (!IsOnTournamentServer()) {
    throw std::runtime_error(message);
  } else {
    BWAPI::Broodwar->sendText(message.c_str());
  }
};

const auto playableRaces = std::vector<BWAPI::Race> {
    BWAPI::Races::Protoss, BWAPI::Races::Terran, BWAPI::Races::Zerg };

struct Neohuman : public BWAPI::AIModule {
	void onStart() override;
	void onEnd(bool isWinner) override;
	void onFrame() override;
	void onSendText(std::string text) override;
	void onReceiveText(BWAPI::Player player, std::string text) override;
	void onPlayerLeft(BWAPI::Player player) override;
	void onNukeDetect(BWAPI::Position target) override;
	void onUnitDiscover(BWAPI::Unit unit) override;
	void onUnitEvade(BWAPI::Unit unit) override;
	void onUnitShow(BWAPI::Unit unit) override;
	void onUnitHide(BWAPI::Unit unit) override;
	void onUnitCreate(BWAPI::Unit unit) override;
	void onUnitDestroy(BWAPI::Unit unit) override;
	void onUnitMorph(BWAPI::Unit unit) override;
	void onUnitRenegade(BWAPI::Unit unit) override;
	void onSaveGame(std::string gameName) override;
	void onUnitComplete(BWAPI::Unit unit) override;

	// Things
	bool wasRandom;
	BWAPI::Race playingRace;
};

extern Neohuman *neoInstance;
