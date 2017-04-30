#include "Neohuman.h"
#include <iostream>

#include "BWEM/bwem.h"

using namespace BWAPI;
using namespace Filter;

using namespace BWEM;
using namespace BWEM::BWAPI_ext;
using namespace BWEM::utils;

#define MIN(a, b) (((a) < (b) ? (a) : (b)))
#define MAX(a, b) (((a) < (b) ? (b) : (a)))

#define SATRUATION_RADIUS 350
#define WORKERAGGRORADIUS 200

#define SHOWTHINGS

namespace { auto & BWEMMap = Map::Instance(); }

bool Neohuman::doBuild(Unit u, UnitType building, TilePosition at) {
	_buildingQueue.push_back(Triple<int, UnitType, TilePosition>{u->getID(), building, at});
}

std::pair <int, int> Neohuman::getQueuedResources() {
	std::pair <int, int> resources;
	for (auto &o : _buildingQueue)
		resources.first  += o.second.mineralPrice(),
		resources.second += o.second.gasPrice();
	return resources;
}

std::pair <int, int> Neohuman::getSpendableResources() {
	std::pair <int, int> result = { Broodwar->self()->minerals(), Broodwar->self()->gas() };
	auto queued = this->getQueuedResources();
	result.first -= queued.first, result.second -= queued.second;
	return result;
}

void Neohuman::onStart() {
	Broodwar << "The map is totally not " << Broodwar->mapName() << "!" << std::endl;

	//Broodwar->enableFlag(Flag::UserInput);

	// Uncomment the following line and the bot will know about everything through the fog of war (cheat).
	//Broodwar->enableFlag(Flag::CompleteMapInformation);

	// Set the command optimization level so that common commands can be grouped
	// and reduce the bot's APM (Actions Per Minute).
	Broodwar->setCommandOptimizationLevel(2);

	// Check if this is a replay
	if (!Broodwar->isReplay()) {
		if (Broodwar->enemy())
			Broodwar << "The matchup is me (" << Broodwar->self()->getRace() << ") vs " << Broodwar->enemy()->getRace() << std::endl;
		for (auto u: Broodwar->self()->getUnits()) {
			//this->onUnitComplete(u);
		}
	}

	BWEMMap.Initialize();
	BWEMMap.EnableAutomaticPathAnalysis();

}

void Neohuman::onEnd(bool didWin) {
	// Called when the game ends
	if (didWin) {

	} else {

	}
}

int Neohuman::randint(int min, int max) {
	std::uniform_int_distribution<int> dist(min, max);
	return dist(_rand);
}

template <typename T>
T Neohuman::randele(std::vector <T> &v) {
	return v[randint(0, v.size())];
}

void Neohuman::onFrame() {
	// Called once every game frame
	int constructingLine = 0;
	int availableMinerals = Broodwar->self()->minerals();

#ifdef SHOWTHINGS

	// Display the game frame rate as text in the upper left area of the screen
	Broodwar->drawTextScreen(200, 0,  "FPS: %d", Broodwar->getFPS());
	Broodwar->drawTextScreen(200, 12, "Average FPS: %f", Broodwar->getAverageFPS());
	Broodwar->drawTextScreen(200, 24, "I want %d more supply (%d queued, %d in production)", _wantedExtraSupply, _queuedSupply, _supplyBeingMade);
	Broodwar->drawTextScreen(200, 36, "I have %d barracks!", _nBarracks);

#endif

	try {
		//BWEM::utils::gridMapExample(BWEMMap);
		//BWEM::utils::drawMap(BWEMMap);
	} catch (const std::exception & e) {
		Broodwar << "EXCEPTION: " << e.what() << std::endl;
	}

	// Return if the game is a replay or is paused
	if (Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self())
		return;

	for (auto &u : Broodwar->self()->getUnits()) {
		if (u->exists() && u->isBeingConstructed()) {
			Broodwar->drawTextScreen(0, constructingLine, "%s", u->getType().c_str());
			constructingLine += 12;
		}
	}

#ifdef SHOWTHINGS

	// Show worker info
	for (auto &u : Broodwar->self()->getUnits()) {
		if (u->getType().isResourceDepot()) {
			Broodwar->drawCircleMap(u->getPosition(), SATRUATION_RADIUS, Colors::Blue);
			auto workers = u->getUnitsInRadius(SATRUATION_RADIUS, (IsGatheringMinerals));
			auto refineries = u->getUnitsInRadius(SATRUATION_RADIUS, (IsRefinery));
			auto gasGeysers = u->getUnitsInRadius(SATRUATION_RADIUS);
			auto mineralFields = u->getUnitsInRadius(SATRUATION_RADIUS, (IsMineralField));
			Broodwar->drawTextMap(u->getPosition() + Position(0, 30), "%cWorkers: %d/%d", Text::White, workers.size(), 2*mineralFields.size());
		}
	}

#endif

	// Prevent spamming by only running our onFrame once every number of latency frames.
	// Latency frames are the number of frames before commands are processed.
	if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
		return;
	
	_supplyBeingMade = 0;
	for (auto &u : Broodwar->self()->getUnits()) {
		if (u->isConstructing() && u->getType().getRace().getSupplyProvider() == u->getType())
			_supplyBeingMade += 8;
	}

	availableMinerals -= 100*_queuedSupply/8;

	if (Broodwar->elapsedTime() - _timeLastQueuedSupply > 5) {
		_queuedSupply = 0;
		_timeLastQueuedSupply = Broodwar->elapsedTime();
	}

	_nSupplyOverhead = 2 + MAX((Broodwar->self()->supplyUsed() - 10) / 8, 0);
	_wantedExtraSupply = (Broodwar->self()->supplyUsed() - (Broodwar->self()->supplyTotal() + _supplyBeingMade + _queuedSupply)) / 2 + _nSupplyOverhead;

	_nBarracks = 0;
	for (auto &u : Broodwar->self()->getUnits()) {
		// If a cc is under construction
		if (u->isConstructing() && u->getType() == u->getType().getRace().getCenter() && _isExpanding)
			availableMinerals += 400;
		// If a barracks is under construction
		if (u->isConstructing() && u->getType() == UnitTypes::Terran_Barracks && _isBuildingBarracks)
			availableMinerals += 150;

		if (u->getType() == UnitTypes::Terran_Barracks) {
			++_nBarracks;
		}
	}

	if (_isExpanding) {
		availableMinerals -= 400;
	}

	if (_isBuildingBarracks) {
		availableMinerals -= 150;
	}

	// Check if supply should be increased
	if (_wantedExtraSupply > 0 && availableMinerals >= 100 && Broodwar->self()->supplyTotal() < 400) {
		// Find unit to build our supply!
		for (auto &unit : Broodwar->self()->getUnits()) {
			if (unit->exists() && unit->isGatheringMinerals() && unit->isMoving() && !unit->isCarryingMinerals()) {
				// We have found our candidate!
				auto supplyType = unit->getType().getRace().getSupplyProvider();
				auto buildPos = Broodwar->getBuildLocation(supplyType, unit->getTilePosition());

				if (unit->build(supplyType, buildPos)) {
					_queuedSupply += 8;
					_timeLastQueuedSupply = Broodwar->elapsedTime();
					break;
				}
			}
		}
	}

	if (availableMinerals >= 150 && !_isBuildingBarracks && _nBarracks < 20) {
		// Build barracks!
		for (auto &u : Broodwar->self()->getUnits()) {
			if (u->exists() && u->isGatheringMinerals() && u->isMoving() && !u->isCarryingMinerals()) {
				auto buildingType = UnitTypes::Terran_Barracks;
				auto buildingPos = Broodwar->getBuildLocation(buildingType, u->getTilePosition());

				if (u->build(buildingType, buildingPos)) {
					_isBuildingBarracks = true;
					availableMinerals -= 150;
					break;
				}
			}
		}
	}

	/*if (!_isExpanding && availableMinerals >= 400){
		// Expand!
		for (auto &u : Broodwar->self()->getUnits()) {
			if (u->exists() && u->isGatheringMinerals() && u->isMoving() && !u->isCarryingMinerals()) {
				// Builder found!
				auto buildingType = u->getType().getRace().getCenter();
				auto buildPos = Broodwar->getBuildLocation(buildingType, u->getTilePosition());

				if (u->build(buildingType, buildPos)) {
					_isExpanding = true;
					availableMinerals -= 400;
					break;
				}
			}
		}
	}*/

	// Iterate through all the units that we own
	for (auto &u : Broodwar->self()->getUnits()) {
		// Ignore the unit if it no longer exists
		// Make sure to include this block when handling any Unit pointer!
		if (!u->exists())
			continue;

		// Ignore the unit if it has one of the following status ailments
		if (u->isLockedDown() || u->isMaelstrommed() || u->isStasised())
			continue;

		// Ignore the unit if it is in one of the following states
		if (u->isLoaded() || !u->isPowered() || u->isStuck())
			continue;

		// Ignore the unit if it is incomplete or busy constructing
		if (!u->isCompleted() || u->isConstructing())
			continue;

		// If the unit is a worker unit
		if (u->getType().isWorker()) {
			auto enemyUnit = u->getClosestUnit(IsEnemy && IsAttacking, WORKERAGGRORADIUS);
			if (enemyUnit)
				u->attack(enemyUnit);

			// if our worker is idle
			if (u->isIdle()) {
				// Order workers carrying a resource to return them to the center,
				// otherwise find a mineral patch to harvest.
				if (u->isCarryingGas() || u->isCarryingMinerals())
					u->returnCargo();

				else if (!u->getPowerUp()) {
					// Probably need to set up some better logic for this
					auto preferredMiningLocation = u->getClosestUnit(IsMineralField);
					u->gather(preferredMiningLocation);
				}
			}
		} else if (u->getType().isResourceDepot()) {
			auto nearbyGeysers = u->getUnitsInRadius(SATRUATION_RADIUS, (GetType == UnitTypes::Resource_Vespene_Geyser));
			if (u->isIdle()) {
				auto workers = u->getUnitsInRadius(SATRUATION_RADIUS, (IsGatheringMinerals));
				if (nWorkers >= 70)
					continue;
				auto mineralFields = u->getUnitsInRadius(SATRUATION_RADIUS, (IsMineralField));
				if (workers.size() < mineralFields.size() * 2 && availableMinerals >= 50) {
					if (u->train(u->getType().getRace().getWorker())) {
						Position pos = u->getPosition();
						Error lastErr = Broodwar->getLastError();
						Broodwar->registerEvent([pos, lastErr](Game*){ Broodwar->drawTextMap(pos + Position(0, -10), "%c%s", Text::Red, lastErr.c_str()); },   // action
							nullptr,    // condition
							Broodwar->getLatencyFrames());  // frames to run
					} else {
						availableMinerals -= 50;
					}
				}
			}
		} else if (u->getType() == UnitTypes::Terran_Barracks) {
			if (u->isIdle() && availableMinerals >= 50)
				if (!u->train(UnitTypes::Terran_Marine))
					availableMinerals -= 50;
		} else if (u->getType() == UnitTypes::Terran_Marine && Broodwar->getFrameCount() % 20 == 0) {
			auto enemyUnit = u->getClosestUnit(IsEnemy && IsAttacking);
			if (enemyUnit) {
				u->attack(enemyUnit->getPosition());
				continue;
			}
			enemyUnit = u->getClosestUnit(IsEnemy);
			if (enemyUnit) {
				u->attack(enemyUnit->getPosition());
				continue;
			}
			auto closestMarine = u->getClosestUnit(GetType == UnitTypes::Terran_Marine);
			if (closestMarine) {
				auto walk = u->getPosition() - closestMarine->getPosition();
				u->move(u->getPosition() + walk);
			}
		}
	}
}

void Neohuman::onSendText(std::string text) {
	Broodwar->sendText("%s", text.c_str());
}

void Neohuman::onReceiveText(Player player, std::string text) {
	Broodwar << player->getName() << " said \"" << text << "\"" << std::endl;
}

void Neohuman::onPlayerLeft(Player player) {
	Broodwar->sendText("Goodbye %s!", player->getName().c_str());
}

void Neohuman::onNukeDetect(Position target) {
	if (target) {
		Broodwar << "Nuclear Launch Detected at " << target << std::endl;
	} else {
		Broodwar->sendText("Where's the nuke?");
	}
	// You can also retrieve all the nuclear missile targets using Broodwar->getNukeDots()!
}

void Neohuman::onUnitDiscover(Unit unit) {
}

void Neohuman::onUnitEvade(Unit unit) {
}

void Neohuman::onUnitShow(Unit unit) {
}

void Neohuman::onUnitHide(Unit unit) {
}

void Neohuman::onUnitCreate(Unit unit) {
}

void Neohuman::onUnitDestroy(Unit unit) {
}

void Neohuman::onUnitMorph(Unit unit) {
	this->onUnitComplete(unit);
}

void Neohuman::onUnitRenegade(Unit unit){
}

void Neohuman::onSaveGame(std::string gameName){
}

void Neohuman::onUnitComplete(Unit unit){
	if (unit->getType() == unit->getType().getRace().getCenter())
		_isExpanding = false;
	if (unit->getType() == UnitTypes::Terran_Barracks)
		_isBuildingBarracks = false;
}
