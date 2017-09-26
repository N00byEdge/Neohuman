#include "Neohuman.h"

#include "Util.h"
#include "SupplyManager.h"
#include "UnitManager.h"
#include "DrawingManager.h"
#include "ResourceManager.h"
#include "MapManager.h"
#include "BuildingPlacer.h"
#include "FAP.h"
#include "SoundDatabase.h"
#include "ModularNN.h"
#include "AIManager.h"

#include <iostream>
#include <fstream>

using namespace BWAPI;
using namespace Filter;

using namespace BWEM;
using namespace BWEM::BWAPI_ext;
using namespace BWEM::utils;

using namespace Neolib;

Neohuman* neoInstance;

void Neohuman::onStart() {
	Timer timer_onStart;

	soundDatabase.loadSounds();

	const std::vector <std::string> openingStrings = {
		"fat, she has more trouble getting around than a goliath.",
		"fat, an arbiter can't recall her.",
		"fat, you need more than 400 lings to surround her.",
		"fat, she takes the splash damage from tanks too.",
		"fat, you can scout her from your own base.",
		"fat, you need 17 dark archons to mind control her.",
		"fat, the ultras gets in her way!.",
		"fat, she can't fit inside a nydus canal!",
		"fat, she can't fit in a single SC2 control group.",
		"fat, she can be targeted by air weapons",
		"fat, she can't burrow into this planet.",
		"fat, this message doesn't fit in one line.",

		"smelly, her medic needs a medic.",

		"unhealthy, plague heals her."
	};

	Broodwar->sendText("Hey, %s, yo momma so ", Broodwar->enemy()->getName().c_str());
	Broodwar->sendText(openingStrings[randint(0, openingStrings.size() - 1)].c_str());
	timer_onStart.reset();

	Broodwar->setCommandOptimizationLevel(2);

	mapManager.init();
	buildingPlacer.init();

	timer_onStart.stop();

#ifndef SSCAIT

	BWAPI::Broodwar->setLocalSpeed(0);
	BWAPI::Broodwar->setFrameSkip(100);

#endif

#ifdef _DEBUG

	Broodwar->sendText("onStart finished in %.1lf ms", timer_onStart.lastMeasuredTime);

#endif

}

void Neohuman::onEnd(bool didWin) {
	// Called when the game ends
	int wins = 0;
	int losses = 0;
	std::ifstream rf("bwapi-data/read/" + Broodwar->enemy()->getName() + ".txt");
	if (rf) {
		rf >> wins >> losses;
		rf.close();
	}
	else {
		rf.open("bwapi-data/write/" + Broodwar->enemy()->getName() + ".txt");
		if (rf) {
			rf >> wins >> losses;
			rf.close();
		}
	}

	if (didWin) {
		++wins;
	} else {
		++losses;
	}
	std::ofstream of("bwapi-data/write/" + Broodwar->enemy()->getName() + ".txt");
	if (of)
		of << wins << " " << losses << "\n";
}

void Neohuman::onFrame() {
	timer_total.reset();

	static int lastFramePaused = -5;
	static const std::vector <std::string> pauseStrings = {
		"No, " + Broodwar->enemy()->getName() + ", I cannot let you do that.",
		"No pets, no Pause.",
		"Your JIT compiler can't keep up? Too bad."
	};

	static auto pauseIt = pauseStrings.begin();
	if (Broodwar->isPaused()) {
		if (pauseIt != pauseStrings.end() && lastFramePaused < Broodwar->getFrameCount()) {
			lastFramePaused = Broodwar->getFrameCount();
			Broodwar->sendText(pauseIt->c_str());
			++pauseIt;
		}
		Broodwar->resumeGame();
	}

	if (!(Broodwar->isPaused() || Broodwar->isReplay())) {
		unitManager.onFrame();
		aiManager.onFrame();

		// Do all the game logic
	}

	timer_drawinfo.reset();
	drawingManager.onFrame();
	timer_drawinfo.stop();

	mapManager.onFrame();

	timer_total.stop();
}

void Neohuman::onSendText(std::string text) {

}

void Neohuman::onReceiveText(Player player, std::string text) {

}

void Neohuman::onPlayerLeft(Player player) {
	Broodwar->sendText("Goodbye %s!", player->getName().c_str());
}

void Neohuman::onNukeDetect(Position target) {
	unitManager.onNukeDetect(target);
}

void Neohuman::onUnitDiscover(Unit unit) {
	unitManager.onUnitDiscover(unit);
}

void Neohuman::onUnitEvade(Unit unit) {
	unitManager.onUnitEvade(unit);
}

void Neohuman::onUnitShow(Unit unit) {
	unitManager.onUnitShow(unit);
}

void Neohuman::onUnitHide(Unit unit) {
	unitManager.onUnitHide(unit);
}

void Neohuman::onUnitCreate(Unit unit) {
	unitManager.onUnitCreate(unit);
}

void Neohuman::onUnitDestroy(Unit unit) {
	unitManager.onUnitDestroy(unit);
}

void Neohuman::onUnitMorph(Unit unit) {
	unitManager.onUnitMorph(unit);
}

void Neohuman::onUnitRenegade(Unit unit) {
	unitManager.onUnitRenegade(unit);
}

void Neohuman::onSaveGame(std::string gameName) {

}

void Neohuman::onUnitComplete(Unit unit) {
	unitManager.onUnitComplete(unit);
}
