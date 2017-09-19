#pragma once

#include <BWAPI.h>

#include "Timer.h"
#include "SoundFile.h"

#include "bwem.h"

// Remember not to use "Broodwar" in any global class constructor!

class Neohuman : public BWAPI::AIModule {
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

		Neolib::Timer timer_drawinfo, timer_NN, timer_total;
};

extern Neohuman* neoInstance;
