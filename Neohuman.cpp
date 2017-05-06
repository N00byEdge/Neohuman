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

#define SHIELDTEXT (char)Text::Blue
#define HEALTHTEXT (char)Text::Green
#define ENERGYTEXT (char)Text::Purple

namespace { auto & BWEMMap = Map::Instance(); }

bool Neohuman::doBuild(Unit u, UnitType building, TilePosition at) {
	if (isWorkerBuilding(u))
		return false;
	if (u->build(building, at)) {
		_buildingQueue.push_back({ Triple<int, UnitType, TilePosition>{u->getID(), building, at}, false });
		return true;
	}
	return false;
}

std::pair <int, int> Neohuman::getQueuedResources() const {
	std::pair <int, int> resources;
	for (auto &o : _buildingQueue)
		if (!o.second)
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
	int s = 0;
	for (auto &o : _buildingQueue) {
		if (!o.second && o.first.second == UnitTypes::Terran_Supply_Depot)
			s += 8;
		else if (!o.second && o.first.second == UnitTypes::Terran_Command_Center)
			s += 10;
	}
	return s + 8 * countUnit(UnitTypes::Terran_Supply_Depot, IsOwned && IsConstructing, true) + 10 * countUnit(UnitTypes::Terran_Command_Center, IsOwned && IsConstructing, true);
}

bool Neohuman::canAfford(UnitType t) const {
	auto spendable = getSpendableResources();
	spendable.first -= t.mineralPrice();
	spendable.second -= t.gasPrice();
	return !(spendable.first < 0 || spendable.second < 0);
}

int Neohuman::countUnit(UnitType t, const UnitFilter &filter = nullptr, bool countQueued = true) const {
	if (!Broodwar->getAllUnits().size())
		return 0;

	Unit anyUnit = *(Broodwar->getAllUnits().begin());
	int c = 0;

	for (auto &u : anyUnit->getUnitsInRadius(999999, filter))
		if (u->exists() && u->getType() == t)
			c++;

	if (countQueued)
		for (auto &o : _buildingQueue)
			if (o.first.second == t && !o.second)
				c++;

	return c;
}

int Neohuman::wantedSupplyOverhead() const {
	return 2 + MAX((Broodwar->self()->supplyUsed() - 10) / 8, 0);
}

int Neohuman::additionalWantedSupply() const {
	return (Broodwar->self()->supplyUsed() - (Broodwar->self()->supplyTotal() + 2 * getQueuedSupply())) / 2 + wantedSupplyOverhead();
}

void Neohuman::manageBuildingQueue() {
	for (unsigned i = 0; i < _buildingQueue.size(); ++i) {
		// Work on halted queue progress, SCV died
		if (Broodwar->getUnit(_buildingQueue[i].first.first) == nullptr || !Broodwar->getUnit(_buildingQueue[i].first.first)->exists() && !_buildingQueue[i].second) {
			// Replace placing scv
			auto pos = Position(_buildingQueue[i].first.third);
			Unit closestBuilder = Broodwar->getClosestUnit(pos, IsOwned && IsGatheringMinerals && !IsCarryingMinerals && CurrentOrder != Orders::PlaceBuilding && CurrentOrder != Orders::ConstructingBuilding);
			if (closestBuilder == nullptr || isWorkerBuilding(closestBuilder))
				continue;
			_buildingQueue[i].first.first = closestBuilder->getID();
			closestBuilder->build(_buildingQueue[i].second, _buildingQueue[i].first.third);
		}
		if (Broodwar->getUnit(_buildingQueue[i].first.first) == nullptr || !Broodwar->getUnit(_buildingQueue[i].first.first)->exists() && _buildingQueue[i].second) {
			// Replace building scv
			auto pos = Position(_buildingQueue[i].first.third);
			Unit closestBuilder = Broodwar->getClosestUnit(pos, IsOwned && IsGatheringMinerals && !IsCarryingMinerals && CurrentOrder != Orders::PlaceBuilding && CurrentOrder != Orders::ConstructingBuilding);
			if (closestBuilder == nullptr || isWorkerBuilding(closestBuilder))
				continue;
			_buildingQueue[i].first.first = closestBuilder->getID();
			closestBuilder->rightClick(Broodwar->getClosestUnit(pos, IsOwned && IsBeingConstructed));
		}

		// Building was placed, update status
		if (!Broodwar->getUnit(_buildingQueue[i].first.first)->getOrder() != Orders::PlaceBuilding && Broodwar->getUnit(_buildingQueue[i].first.first)->getOrder() == Orders::ConstructingBuilding)
			_buildingQueue[i].second = true;

		// Building was placed and finished
		if (!Broodwar->getUnit(_buildingQueue[i].first.first)->isConstructing() && _buildingQueue[i].second && Broodwar->getClosestUnit(Position(_buildingQueue[i].first.third), (IsBuilding))->getTilePosition() == _buildingQueue[i].first.third) {
			_buildingQueue.erase(_buildingQueue.begin() + i--);
			continue;
		}

		// Building was not placed, retry placing:
		if (Broodwar->getUnit(_buildingQueue[i].first.first)->getOrder() != Orders::PlaceBuilding && Broodwar->getUnit(_buildingQueue[i].first.first)->getOrder() != Orders::ConstructingBuilding) {
			// Let's see if the location is buildable
			if (Broodwar->canBuildHere(_buildingQueue[i].first.third, _buildingQueue[i].first.second)) {
				// Let's just keep replacing there, and hope for the best.
				Broodwar->getUnit(_buildingQueue[i].first.first)->build(_buildingQueue[i].first.second, _buildingQueue[i].first.third);
			} else if(_buildingQueue[i].first.second != UnitTypes::Terran_Command_Center) {
				// If it's not a cc, let's find a new location for this building
				_buildingQueue[i].first.third = Broodwar->getBuildLocation(_buildingQueue[i].first.second, _buildingQueue[i].first.third);
				Broodwar->getUnit(_buildingQueue[i].first.first)->build(_buildingQueue[i].first.second, _buildingQueue[i].first.third);
			} else {
				// If cc fails to place, remove it from the queue
				_buildingQueue.erase(_buildingQueue.begin() + i--);
				continue;
			}
		}
	}
}

bool Neohuman::isWorkerBuilding(Unit u) const {
	for (auto &o : _buildingQueue)
		if (u->getID() == o.first.first)
			return true;
	
	return false;
}


Unit Neohuman::getClosestBuilder(Unit u) const {
	return u->getClosestUnit(IsOwned && IsWorker && !IsCarryingMinerals && IsGatheringMinerals);
}

Unit Neohuman::getAnyBuilder() const {
	for (auto &u : Broodwar->self()->getUnits())
		if (u->getType().isWorker() && !u->isCarryingMinerals() && u->isGatheringMinerals())
			return u;

	return nullptr;
}

TilePosition Neohuman::getNextExpansion() const {
	for (auto &b : _allBases)
		if (Broodwar->canBuildHere(b->Location(), UnitTypes::Terran_Command_Center))
			return b->Location();

	return TilePosition(600, 600);
}

bool Neohuman::requestScan(Position p) {
	for (auto &c : _comsats)
		if (!_didUseScanThisFrame && c->canUseTech(TechTypes::Scanner_Sweep, p) && c->useTech(TechTypes::Scanner_Sweep, p), _didUseScanThisFrame = true)
			return true;
	return false;
}

void Neohuman::onStart() {
	Broodwar << "The map is totally not " << Broodwar->mapName() << "!" << std::endl;

	Broodwar->enableFlag(Flag::UserInput);

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

	for (auto &a : BWEMMap.Areas())
		for (auto &b : a.Bases())
			_allBases.push_back(&b), _unexploredBases.insert(&b);

	for (auto &l : Broodwar->getStartLocations()) {
		if (Broodwar->isVisible(l)) {
			// My start location
			BWAPI::TilePosition a(l);
			// Do some insertion sorting for the possible expansions, based on the distance to our home base
			for (unsigned currentPos = 0; currentPos < _allBases.size() - 1; ++currentPos) {
				double shortestDist = l.getApproxDistance(_allBases[currentPos]->Location());
				for (unsigned currentCmp = currentPos + 1; currentCmp < _allBases.size(); ++currentCmp) {
					double dist = l.getApproxDistance(_allBases[currentCmp]->Location());
					if (dist < shortestDist) {
						shortestDist = dist;
						const Base * temp = _allBases[currentPos];
						_allBases[currentPos] = _allBases[currentCmp];
						_allBases[currentCmp] = temp;
					}
				}
			}
		}
	}
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

	if (Broodwar->isReplay())
		return;

	// Display the game frame rate as text in the upper left area of the screen
	Broodwar->drawTextScreen(200, 0,  "FPS: %c%d", Broodwar->getFPS() >= 30 ? Text::Green : Text::Red, Broodwar->getFPS());
	Broodwar->drawTextScreen(200, 12, "Average FPS: %c%f", Broodwar->getFPS() >= 30 ? Text::Green : Text::Red, Broodwar->getAverageFPS());
	Broodwar->drawTextScreen(200, 24, "I want %c%d%c more supply (%d coming up)", additionalWantedSupply() == 0 ? Text::Yellow : (additionalWantedSupply() > 0 ? Text::Red : Text::Green), additionalWantedSupply(), Text::Default, getQueuedSupply());
	Broodwar->drawTextScreen(200, 36, "I have %d barracks!", countUnit(UnitTypes::Terran_Barracks, IsOwned));
	Broodwar->drawTextScreen(200, 48, "I have %d APM!", Broodwar->getAPM());

	/*for (unsigned i = 0; i < _allBases.size(); ++ i) {
		Broodwar->drawBoxMap(Position(_allBases[i]->Location()), Position(Position(_allBases[i]->Location()) + Position(UnitTypes::Terran_Command_Center.tileSize())), Colors::Grey);
		Broodwar->drawTextMap(Position(_allBases[i]->Location()), "Base #%u", i);
	}*/

	try {
		//BWEM::utils::gridMapExample(BWEMMap);
		//BWEM::utils::drawMap(BWEMMap);
	} catch (const std::exception & e) {
		Broodwar << "EXCEPTION: " << e.what() << std::endl;
	}

	// Return if the game is a replay or is paused
	if (Broodwar->isPaused() || !Broodwar->self())
		return;

	std::string s = "Comsats (" + std::to_string(_comsats.size()) + "): " + ENERGYTEXT;
	for (auto &c : _comsats)
		s += std::to_string(c->getEnergy()) + " ";

	Broodwar->drawTextScreen(0, constructingLine, s.c_str());
	constructingLine += 12;

	Broodwar->drawTextScreen(0, constructingLine, "%c%u buildings in queue", Text::White, _buildingQueue.size());
	constructingLine += 12;

	for (auto &o : _buildingQueue) {
		Broodwar->drawTextScreen(0, constructingLine, "%c%s", o.second ? Text::Blue : Text::Orange, o.first.second.c_str());
		Broodwar->drawBoxMap(Position(o.first.third), Position((Position)o.first.third + (Position)o.first.second.tileSize()), o.second ? Colors::Blue : Colors::Orange, false);
		constructingLine += 12;
	}

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

	// Prevent spamming by only running our onFrame once every number of latency frames.
	// Latency frames are the number of frames before commands are processed.
	if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames())
		return;

	_didUseScanThisFrame = false;

	manageBuildingQueue();

	// Check if supply should be increased
	if (additionalWantedSupply() > 0 && canAfford(UnitTypes::Terran_Supply_Depot) && Broodwar->self()->supplyTotal() < 400) {
		Unit builder = getAnyBuilder();
		if (builder != nullptr)
			doBuild(builder, UnitTypes::Terran_Supply_Depot, Broodwar->getBuildLocation(UnitTypes::Terran_Supply_Depot, builder->getTilePosition()));
	}

	if ((canAfford(UnitTypes::Terran_Barracks) && countUnit(UnitTypes::Terran_Barracks) < countUnit(UnitTypes::Terran_SCV, IsGatheringMinerals) / 6) || getSpendableResources().first >= 500   ) {
		Unit builder = getAnyBuilder();
		if (builder != nullptr)
			doBuild(builder, UnitTypes::Terran_Barracks, Broodwar->getBuildLocation(UnitTypes::Terran_Barracks, builder->getTilePosition()));
	}

	if (canAfford(UnitTypes::Terran_Academy) && countUnit(UnitTypes::Terran_Barracks) >= 2 && !countUnit(UnitTypes::Terran_Academy)) {
		Unit builder = getAnyBuilder();
		if (builder != nullptr)
			doBuild(builder, UnitTypes::Terran_Academy, Broodwar->getBuildLocation(UnitTypes::Terran_Academy, builder->getTilePosition()));
	}

	for (std::set <const Base*>::iterator it = _unexploredBases.begin(); it != _unexploredBases.end(); ++it) {
		if (Broodwar->isVisible((*it)->Location())) {
			_unexploredBases.erase(it);
			// Break loop, iterator invalidated
			break;
		}
	}

	if (getSpendableResources().first >= 200){
		for (auto &u : Broodwar->self()->getUnits()) {
			if (u->exists() && u->isGatheringMinerals() && u->isMoving() && !u->isCarryingMinerals() && !isWorkerBuilding(u)) {
				auto buildingType = u->getType().getRace().getCenter();
				auto buildPos = getNextExpansion();
				if (!buildPos.isValid())
					break;

				if (doBuild(u, buildingType, buildPos))
					break;
			}
		}
	}

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

				else if (!u->getPowerUp() && Broodwar->getMinerals().size()) {
					// Probably need to set up some better logic for this
					auto preferredMiningLocation = *(Broodwar->getMinerals().begin());
					for (auto &m : Broodwar->getMinerals())
						if (preferredMiningLocation->getPosition().getApproxDistance(u->getPosition()) > m->getPosition().getApproxDistance(u->getPosition()))
							preferredMiningLocation = m;

					u->gather(preferredMiningLocation);
					Broodwar->getMinerals();
				}
			}
		} else if (u->getType().isResourceDepot() && !u->isBeingConstructed()) {
			auto nearbyGeysers = u->getUnitsInRadius(SATRUATION_RADIUS, (GetType == UnitTypes::Resource_Vespene_Geyser));
			if (nearbyGeysers.size() && Broodwar->self()->supplyUsed()/2 >= 13){
				auto buildingType = UnitTypes::Terran_Refinery;
				for (Unit geyser : nearbyGeysers) {
					for (auto &o : _buildingQueue){
						if (o.first.third == geyser->getTilePosition())
							goto skipgeyser;
					}
					Unit builder = getClosestBuilder(geyser);
					if (builder != nullptr)
						doBuild(builder, buildingType, geyser->getTilePosition());
					skipgeyser:;
				}
			}

			if (u->isIdle()) {
				auto workers = u->getUnitsInRadius(SATRUATION_RADIUS, (IsGatheringMinerals));
				if (countUnit(UnitTypes::Terran_SCV, IsOwned) >= 70)
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
			if (u->isIdle() && canAfford(UnitTypes::Terran_Comsat_Station))
				u->buildAddon(UnitTypes::Terran_Comsat_Station);
		} else if (u->getType() == UnitTypes::Terran_Barracks) {
			if (u->isIdle() && getSpendableResources().first >= 50)
				u->train(UnitTypes::Terran_Marine);
		} else if (u->getType() == UnitTypes::Terran_Marine && Broodwar->getFrameCount() % 16 == 0) {
			auto fleeFrom = u->getClosestUnit(IsEnemy, 1000);
			int friendlyCount;
			if (fleeFrom) {
				int enemyCount = fleeFrom->getUnitsInRadius(1000, IsEnemy && !IsBuilding && !IsWorker).size() + 1;
				friendlyCount = fleeFrom->getUnitsInRadius(2000, IsOwned && !IsBuilding && !IsWorker).size();
				if (enemyCount + 5 > friendlyCount) {
					if (fleeFrom != nullptr) {
						u->move(u->getPosition() + u->getPosition() - fleeFrom->getPosition());
						continue;
					}
				}
			} else {
				friendlyCount = u->getUnitsInRadius(2000, IsOwned && !IsBuilding && !IsWorker).size();
			}

			auto enemyUnit = u->getClosestUnit(IsEnemy && IsAttacking);
			if (enemyUnit && !enemyUnit->isDetected()) {
				requestScan(enemyUnit->getPosition());
			}

			enemyUnit = u->getClosestUnit(IsEnemy && IsAttacking && IsDetected);
			if (enemyUnit != nullptr) {
				u->attack(enemyUnit->getPosition());
				if (!u->isStimmed() && Broodwar->self()->isResearchAvailable(TechTypes::Stim_Packs) && Broodwar->self()->weaponMaxRange(WeaponTypes::Gauss_Rifle) >= u->getDistance(enemyUnit))
					u->useTech(TechTypes::Stim_Packs);
				continue;
			}

			enemyUnit = u->getClosestUnit(IsEnemy && IsDetected);
			if (enemyUnit) {
				u->attack(enemyUnit->getPosition());
				continue;
			}

			auto closeSpecialBuilding = u->getClosestUnit(IsSpecialBuilding && !IsInvincible, WORKERAGGRORADIUS);
			if (closeSpecialBuilding)
				u->attack(closeSpecialBuilding);
			else {
				auto closestMarine = u->getClosestUnit(GetType == UnitTypes::Terran_Marine);
				if (closestMarine && friendlyCount >= 5) {
					auto walk = u->getPosition() - closestMarine->getPosition();
					u->move(u->getPosition() + walk);
				} else {
					if (_unexploredBases.size())
						u->move((Position)(*_unexploredBases.begin())->Location());
				}
			}

		}

		else if (u->getType() == UnitTypes::Terran_Academy && u->isIdle()){
			if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::U_238_Shells) == 0) 
				u->upgrade(UpgradeTypes::U_238_Shells);

			else if (Broodwar->self()->isResearchAvailable(TechTypes::Stim_Packs))
				u->research(TechTypes::Stim_Packs);
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
	if (unit->getType() == UnitTypes::Terran_Comsat_Station)
		_comsats.erase(unit);
}

void Neohuman::onUnitMorph(Unit unit) {
	this->onUnitComplete(unit);
}

void Neohuman::onUnitRenegade(Unit unit){
}

void Neohuman::onSaveGame(std::string gameName){
}

void Neohuman::onUnitComplete(Unit unit){
	if (unit->getType() == UnitTypes::Terran_Comsat_Station)
		_comsats.insert(unit);
}
