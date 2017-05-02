#include "Neohuman.h"
#include <iostream>

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
	_buildingQueue.push_back({Triple<int, UnitType, TilePosition>{u->getID(), building, at}, false});
	return u->build(building, at);
}

std::pair <int, int> Neohuman::getQueuedResources() const {
	std::pair <int, int> resources;
	for (auto &o : _buildingQueue)
		resources.first  += o.first.second.mineralPrice(),
		resources.second += o.first.second.gasPrice();
	return resources;
}

std::pair <int, int> Neohuman::getSpendableResources() const {
	std::pair <int, int> result = { Broodwar->self()->minerals(), Broodwar->self()->gas() };
	auto queued = this->getQueuedResources();
	result.first -= queued.first, result.second -= queued.second;
	return result;
}

int Neohuman::getQueuedSupply() const {
	int sum = 0;
	for (auto &u : Broodwar->getAllUnits()) {
		if (u->isBeingConstructed()) {
			if (u->getType() == UnitTypes::Terran_Supply_Depot)
				sum += 8;
			else if (u->getType() == UnitTypes::Terran_Command_Center)
				sum += 10;
		}
	}
	return sum;
}

void Neohuman::manageBuildingQueue() {
	for (unsigned i = 0; i < _buildingQueue.size(); ++i) {
		// Work on halted queue progress, SCV died
		if (!Broodwar->getUnit(_buildingQueue[i].first.first)->exists() && !_buildingQueue[i].second) {
			// Replace placing scv
			auto pos = Position(_buildingQueue[i].first.third);
			_buildingQueue[i].first.first = Broodwar->getClosestUnit(pos, IsOwned && IsGatheringMinerals && !IsCarryingMinerals)->getID();
			Broodwar->getUnit(_buildingQueue[i].first.first)->build(_buildingQueue[i].second, _buildingQueue[i].first.third);
		}
		if (!Broodwar->getUnit(_buildingQueue[i].first.first)->isConstructing() && !_buildingQueue[i].second) {
			// Replace building scv
			auto pos = Position(_buildingQueue[i].first.third);
			_buildingQueue[i].first.first = Broodwar->getClosestUnit(pos, IsOwned && IsGatheringMinerals && !IsCarryingMinerals)->getID();
			Broodwar->getUnit(_buildingQueue[i].first.first)->rightClick(Broodwar->getClosestUnit(pos, IsOwned && IsBeingConstructed));
		}

		// Building was placed, update status
		if (!Broodwar->getUnit(_buildingQueue[i].first.first)->getOrder() != Orders::PlaceBuilding && Broodwar->getUnit(_buildingQueue[i].first.first)->getOrder() == Orders::ConstructingBuilding)
			_buildingQueue[i].second = true;

		// Building was placed and finished
		if (!Broodwar->getUnit(_buildingQueue[i].first.first)->isConstructing() && _buildingQueue[i].second && Broodwar->getClosestUnit(Position(_buildingQueue[i].first.third), (IsBuilding))->getTilePosition() == _buildingQueue[i].first.third)
			_buildingQueue.erase(_buildingQueue.begin() + i--);

		// Building was not placed, retry placing:
		if (!Broodwar->getUnit(_buildingQueue[i].first.first)->getOrder() != Orders::PlaceBuilding && Broodwar->getUnit(_buildingQueue[i].first.first)->getOrder() != Orders::ConstructingBuilding) {
			// Let's see if the location is buildable
			if (Broodwar->canBuildHere(_buildingQueue[i].first.third, _buildingQueue[i].first.second)) {
				// Let's just keep replacing there, and hope for the best.
				Broodwar->getUnit(_buildingQueue[i].first.first)->build(_buildingQueue[i].first.second, _buildingQueue[i].first.third);
			} else {
				// Let's find a new location for this building
				_buildingQueue[i].first.third = Broodwar->getBuildLocation(_buildingQueue[i].first.second, _buildingQueue[i].first.third);
				Broodwar->getUnit(_buildingQueue[i].first.first)->build(_buildingQueue[i].first.second, _buildingQueue[i].first.third);
			}
		}
	}
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

#ifdef SHOWTHINGS

	// Display the game frame rate as text in the upper left area of the screen
	Broodwar->drawTextScreen(200, 0,  "FPS: %d", Broodwar->getFPS());
	Broodwar->drawTextScreen(200, 12, "Average FPS: %f", Broodwar->getAverageFPS());
	Broodwar->drawTextScreen(200, 24, "I want %d more supply (%d coming up)", _wantedExtraSupply, getQueuedSupply());
	Broodwar->drawTextScreen(200, 36, "I have %d barracks!", _nBarracks);
	Broodwar->drawTextScreen(200, 48, "I have %d APM!", Broodwar->getAPM());

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

	Broodwar->drawTextScreen(0, constructingLine, "%c%u buildings in queue", Text::White, _buildingQueue.size());
	constructingLine += 12;

	for (auto &o : _buildingQueue) {
		Broodwar->drawTextScreen(0, constructingLine, "%c%s", o.second ? Text::Blue : Text::Orange, o.first.second.c_str());
		Broodwar->drawBoxMap(Position(o.first.third), Position((Position)o.first.third + (Position)o.first.second.tileSize()), o.second ? Colors::Blue : Colors::Orange, false);
		constructingLine += 12;
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

	manageBuildingQueue();

	_nSupplyOverhead = 2 + MAX((Broodwar->self()->supplyUsed() - 10) / 8, 0);
	_wantedExtraSupply = (Broodwar->self()->supplyUsed() - (Broodwar->self()->supplyTotal() + getQueuedSupply())) / 2 + _nSupplyOverhead;

	_nBarracks = 0;
	for (auto &u : Broodwar->self()->getUnits()) {
		if (u->getType() == UnitTypes::Terran_Barracks)
			++_nBarracks;
	}

	int nWorkers = 0;
	for (auto &u : Broodwar->getAllUnits()) {
		if (u->getType() == u->getType().getRace().getWorker() && u->getPlayer() == Broodwar->self()) {
			++nWorkers;
		}
	}

	// Check if supply should be increased
	if (_wantedExtraSupply > 0 && getSpendableResources().first >= 100 && Broodwar->self()->supplyTotal() < 400) {
		// Find unit to build our supply!
		for (auto &unit : Broodwar->self()->getUnits()) {
			if (unit->exists() && unit->isGatheringMinerals() && unit->isMoving() && !unit->isCarryingMinerals()) {
				// We have found our candidate!
				auto supplyType = unit->getType().getRace().getSupplyProvider();
				auto buildPos = Broodwar->getBuildLocation(supplyType, unit->getTilePosition());

				doBuild(unit, supplyType, buildPos);
			}
		}
	}

	if (getSpendableResources().first >= 150 && !_isBuildingBarracks && _nBarracks < 20) {
		// Build barracks!
		for (auto &u : Broodwar->self()->getUnits()) {
			if (u->exists() && u->isGatheringMinerals() && u->isMoving() && !u->isCarryingMinerals()) {
				auto buildingType = UnitTypes::Terran_Barracks;
				auto buildingPos = Broodwar->getBuildLocation(buildingType, u->getTilePosition());

				if (doBuild(u, buildingType, buildingPos)) {

					_isBuildingBarracks = true;
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
				if (workers.size() < mineralFields.size() * 2 && getSpendableResources().first >= 50) {
					if (u->train(u->getType().getRace().getWorker())) {
						Position pos = u->getPosition();
						Error lastErr = Broodwar->getLastError();
						Broodwar->registerEvent([pos, lastErr](Game*){ Broodwar->drawTextMap(pos + Position(0, -10), "%c%s", Text::Red, lastErr.c_str()); },   // action
							nullptr,    // condition
							Broodwar->getLatencyFrames());  // frames to run
					}
				}
			}
		} else if (u->getType() == UnitTypes::Terran_Barracks) {
		} else if (u->getType() == UnitTypes::Terran_Marine && Broodwar->getFrameCount() % 20 == 0) {
			if (u->isIdle() && getSpendableResources().first >= 50)
				u->train(UnitTypes::Terran_Marine);
			auto enemyUnit = u->getClosestUnit(IsEnemy && IsAttacking && IsDetected);
			if (enemyUnit) {
				u->attack(enemyUnit->getPosition());
				continue;
			}
			enemyUnit = u->getClosestUnit(IsEnemy && IsDetected);
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
	try {
		if (unit->getType().isMineralField())			BWEMMap.OnMineralDestroyed(unit);
		else if (unit->getType().isSpecialBuilding())	BWEMMap.OnStaticBuildingDestroyed(unit);
	} catch (const std::exception & e) {
		Broodwar << "EXCEPTION: " << e.what() << std::endl;
	}
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
