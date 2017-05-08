#include "Neohuman.h"
#include <iostream>

using namespace BWAPI;
using namespace Filter;

using namespace BWEM;
using namespace BWEM::BWAPI_ext;
using namespace BWEM::utils;

using namespace Neolib;

#define MIN(a, b) (((a) < (b) ? (a) : (b)))
#define MAX(a, b) (((a) < (b) ? (b) : (a)))

#define SATRUATION_RADIUS 350
#define WORKERAGGRORADIUS 200

#define SHIELDTEXT		(char)Text::Blue
#define HEALTHTEXT		(char)Text::Green
#define ENERGYTEXT		(char)Text::Purple

#define BUILDINGTEXT	(char)Text::Green
#define PLACINGTEXT		(char)Text::Cyan

#define BUILDINGCOLOR	Colors::Green
#define PLACINGCOLOR	Colors::Cyan

#define ENEMYCOLOR		Colors::Red

namespace { auto & BWEMMap = Map::Instance(); }

bool Neohuman::doBuild(Unit u, UnitType building, TilePosition at) {
	if (isWorkerBuilding(u))
		return false;

	// If this intersects with current building project
	for (auto &o : _buildingQueue) {
		if (building == UnitTypes::Terran_Refinery || building == UnitTypes::Terran_Command_Center) {
			if (intersect(at.x, at.y, at.x + building.tileSize().x, at.x + building.tileSize().y,
				o.first.third.x, o.first.third.y, o.first.third.x + o.first.second.tileSize().x, o.first.third.y + o.first.second.tileSize().y))
				return false;
		} else {
			if (intersect(at.x, at.y, at.x + building.tileSize().x + 1, at.x + building.tileSize().y + 1,
				o.first.third.x, o.first.third.y, o.first.third.x + o.first.second.tileSize().x + 1, o.first.third.y + o.first.second.tileSize().y + 1))
				return false;
		}
	}
		

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

unsigned Neohuman::countFriendly(UnitType t = UnitTypes::AllUnits, bool onlyWithWeapons = false, bool countQueued = true) {
	unsigned sum = 0;

	if (t == UnitTypes::AllUnits)
		for (auto &ut : _unitsByType)
			if (onlyWithWeapons && ut.first.groundWeapon() || ut.first.airWeapon())
				sum += ut.second.size();

	else
		sum = _unitsByType[t].size();

	if (countQueued) {
		if (t == UnitTypes::AllUnits)
			sum += _buildingQueue.size();
		else
			for (auto &bo : _buildingQueue)
				if (bo.first.second == t)
					++sum;
	}

	return sum;
}

unsigned Neohuman::countEnemies(UnitType t = UnitTypes::AllUnits, bool onlyWithWeapons = false) {
	unsigned sum = 0;

	if (t == UnitTypes::AllUnits)
		for (auto &ut : _enemyUnitsByType)
			if (onlyWithWeapons && ut.first.groundWeapon() || ut.first.airWeapon())
				sum += ut.second.size();

	else
		sum = _enemyUnitsByType[t].size();

	return sum;
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

TilePosition Neohuman::getNextExpansion() {
	for (auto &b : _allBases)
		if (Broodwar->canBuildHere(b->Location(), UnitTypes::Terran_Command_Center)) {
			if (!Broodwar->isExplored(b->Location()) && !requestScan(Position(b->Location())))
				continue;
			for (auto &eu : _knownEnemies) {
				if (eu.second.first.getApproxDistance(Position(b->Location())) < 1000)
					goto skiplocation;
			}
			return b->Location();
			skiplocation:;
		}

	return TilePosition(-1, -1);
}

bool Neohuman::requestScan(Position p) {
	if (_didUseScanThisFrame)
		return false;
	Unit highestEnergy = nullptr;
	for (auto & u : _unitsByType[UnitTypes::Terran_Comsat_Station])
		if (!highestEnergy || u->getEnergy() > highestEnergy->getEnergy())
			highestEnergy = u;
	
	if (highestEnergy == nullptr)
		return false;

	return highestEnergy->useTech(TechTypes::Scanner_Sweep);
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

const char *noRaceName(const char *name) {
	for (const char *c = name; *c; c++)
		if (*c == '_') return ++c;
	return name;
}

int getNextLine(int &i) {
	i += 12;
	return i - 12;
}

void Neohuman::onFrame() {
	static Timer timer_drawinfo, timer_managequeue, timer_buildbuildings, timer_unitlogic, timer_marinelogic;
	timer_drawinfo.reset();

	// Called once every game frame
	std::vector <int> columnPixelLine = { 0, 0, 0, 200 };
	std::vector <int> columnStart = { 0, 140, 240, 500 };

	if (Broodwar->isReplay())
		return;

	// Display the game frame rate as text in the upper left area of the screen
	Broodwar->drawTextScreen(140, getNextLine(columnPixelLine[1]), "FPS: %c%d", Broodwar->getFPS() >= 30 ? Text::Green : Text::Red, Broodwar->getFPS());
	Broodwar->drawTextScreen(140, getNextLine(columnPixelLine[1]), "Average FPS: %c%f", Broodwar->getFPS() >= 30 ? Text::Green : Text::Red, Broodwar->getAverageFPS());
	Broodwar->drawTextScreen(140, getNextLine(columnPixelLine[1]), "I want %c%d%c more supply", additionalWantedSupply() == 0 ? Text::Yellow : (additionalWantedSupply() > 0 ? Text::Red : Text::Green), additionalWantedSupply(), Text::Default);
	Broodwar->drawTextScreen(140, getNextLine(columnPixelLine[1]), "%u marines swarming you", countFriendly(UnitTypes::Terran_Marine));
	Broodwar->drawTextScreen(140, getNextLine(columnPixelLine[1]), "I have %d barracks!", countFriendly(UnitTypes::Terran_Barracks));
	Broodwar->drawTextScreen(140, getNextLine(columnPixelLine[1]), "I have %d APM!", Broodwar->getAPM());

	Broodwar->drawTextScreen(450, 16, "%d", getSpendableResources().first);
	Broodwar->drawTextScreen(518, 16, "%d", getSpendableResources().second);

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

	std::string s = "Comsats (" + std::to_string(_unitsByType[UnitTypes::Terran_Comsat_Station].size()) + "): " + ENERGYTEXT;
	for (auto &c : _unitsByType[UnitTypes::Terran_Comsat_Station])
		s += std::to_string(c->getEnergy()) + " ";

	Broodwar->drawTextScreen(0, getNextLine(columnPixelLine[0]), s.c_str());

	Broodwar->drawTextScreen(0, getNextLine(columnPixelLine[0]), "%c%u buildings in queue", Text::White, _buildingQueue.size());

	for (auto &o : _buildingQueue) {
		Broodwar->drawTextScreen(0, getNextLine(columnPixelLine[0]), "%c%s", o.second ? BUILDINGTEXT : PLACINGTEXT, noRaceName(o.first.second.c_str()));
		Broodwar->drawBoxMap(Position(o.first.third), Position((Position)o.first.third + (Position)o.first.second.tileSize()), o.second ? BUILDINGCOLOR : PLACINGCOLOR, false);
		Broodwar->drawTextMap(Position(o.first.third) + Position(10, 10), "%s", noRaceName(o.first.second.getName().c_str()));
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

	for (auto &u : _knownEnemies) {
		if (u.first->isVisible())
			u.second.first = u.first->getPosition();

		Broodwar->drawBoxMap(u.second.first - Position(u.second.second.tileSize()) / 2, u.second.first + Position(u.second.second.tileSize()) / 2, ENEMYCOLOR);
		Broodwar->drawTextMap(u.second.first + u.second.second.size() / 2 + Position(10, 10), "%s", noRaceName(u.second.second.c_str()));
	}

	for (auto &ut : _enemyUnitsByType)
		Broodwar->drawTextScreen(280, getNextLine(columnPixelLine[2]), "%s: %u", noRaceName(ut.first.c_str()), ut.second.size());

	Broodwar->drawTextScreen(500, getNextLine(columnPixelLine[3]), "Frame times:"));
	Broodwar->drawTextScreen(500, getNextLine(columnPixelLine[3]), "Drawinfo: %.1lf ms", timer_drawinfo.lastMeasuredTime);
	Broodwar->drawTextScreen(500, getNextLine(columnPixelLine[3]), "Managequeue: %.1lf ms", timer_managequeue.lastMeasuredTime);
	Broodwar->drawTextScreen(500, getNextLine(columnPixelLine[3]), "Buildbuildings: %.1lf ms", timer_buildbuildings.lastMeasuredTime);
	Broodwar->drawTextScreen(500, getNextLine(columnPixelLine[3]), "Unitlogic: %.1lf ms", timer_unitlogic.lastMeasuredTime);
	Broodwar->drawTextScreen(500, getNextLine(columnPixelLine[3]), "Marines: %.1lf ms", timer_marinelogic.lastMeasuredTime);
	timer_drawinfo.stop();

	// Return if the game is a replay or is paused
	if (Broodwar->isPaused() || !Broodwar->self())
		return;

	// Prevent spamming by only running our onFrame once every number of latency frames.
	// Latency frames are the number of frames before commands are processed.
	if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames())
		return;

	_didUseScanThisFrame = false;

	timer_managequeue.reset();
	manageBuildingQueue();
	timer_managequeue.stop();

	timer_buildbuildings.reset();

	// Check if supply should be increased
	if (additionalWantedSupply() > 0 && canAfford(UnitTypes::Terran_Supply_Depot) && Broodwar->self()->supplyTotal()/2 + getQueuedSupply() < 200) {
		Unit builder = getAnyBuilder();
		if (builder != nullptr)
			doBuild(builder, UnitTypes::Terran_Supply_Depot, Broodwar->getBuildLocation(UnitTypes::Terran_Supply_Depot, builder->getTilePosition()));
	}

	if ((canAfford(UnitTypes::Terran_Barracks) && countUnit(UnitTypes::Terran_Barracks, IsOwned) < countUnit(UnitTypes::Terran_SCV, IsGatheringMinerals && IsOwned) / 6) || getSpendableResources().first >= 500   ) {
		Unit builder = getAnyBuilder();
		if (builder != nullptr)
			doBuild(builder, UnitTypes::Terran_Barracks, Broodwar->getBuildLocation(UnitTypes::Terran_Barracks, builder->getTilePosition()));
	}

	if (canAfford(UnitTypes::Terran_Academy) && countUnit(UnitTypes::Terran_Barracks, IsOwned) >= 2 && !countUnit(UnitTypes::Terran_Academy, IsOwned)) {
		Unit builder = getAnyBuilder();
		if (builder != nullptr)
			doBuild(builder, UnitTypes::Terran_Academy, Broodwar->getBuildLocation(UnitTypes::Terran_Academy, builder->getTilePosition()));
	}

	if (canAfford(UnitTypes::Terran_Factory) && !countUnit(UnitTypes::Terran_Factory, IsOwned) && Broodwar->self()->supplyUsed()/2 >= 70) {
		Unit builder = getAnyBuilder();
		if (builder != nullptr)
			doBuild(builder, UnitTypes::Terran_Factory, Broodwar->getBuildLocation(UnitTypes::Terran_Factory, builder->getTilePosition()));
	}

	if (canAfford(UnitTypes::Terran_Starport) && !countUnit(UnitTypes::Terran_Starport, IsOwned) && Broodwar->self()->supplyUsed() / 2 >= 70) {
		Unit builder = getAnyBuilder();
		if (builder != nullptr)
			doBuild(builder, UnitTypes::Terran_Starport, Broodwar->getBuildLocation(UnitTypes::Terran_Starport, builder->getTilePosition()));
	}

	if (canAfford(UnitTypes::Terran_Science_Facility) && !countUnit(UnitTypes::Terran_Science_Facility, IsOwned) && Broodwar->self()->supplyUsed() / 2 >= 80) {
		Unit builder = getAnyBuilder();
		if (builder != nullptr)
			doBuild(builder, UnitTypes::Terran_Science_Facility, Broodwar->getBuildLocation(UnitTypes::Terran_Science_Facility, builder->getTilePosition()));
	}

	for (std::set <const Base*>::iterator it = _unexploredBases.begin(); it != _unexploredBases.end(); ++it) {
		if (Broodwar->isVisible((*it)->Location())) {
			_unexploredBases.erase(it);
			// Break loop, iterator invalidated
			break;
		}
	}

	if (canAfford(UnitTypes::Terran_Engineering_Bay) && countUnit(UnitTypes::Terran_Engineering_Bay) < 2 && Broodwar->self()->supplyUsed()/2 >= 40) {
		Unit builder = getAnyBuilder();
		if (builder != nullptr)
			doBuild(builder, UnitTypes::Terran_Engineering_Bay, Broodwar->getBuildLocation(UnitTypes::Terran_Engineering_Bay, builder->getTilePosition()));
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

	timer_buildbuildings.stop();

	timer_unitlogic.reset();

	for (auto &u : _unitsByType[UnitTypes::Terran_SCV]) {
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
			}
		}
	}

	for (auto &u : _unitsByType[UnitTypes::Terran_Command_Center]) {
		if (u->isConstructing())
			continue;

		auto nearbyGeysers = u->getUnitsInRadius(SATRUATION_RADIUS, (GetType == UnitTypes::Resource_Vespene_Geyser));
		if (nearbyGeysers.size() && Broodwar->self()->supplyUsed() / 2 >= 13){
			auto buildingType = UnitTypes::Terran_Refinery;
			for (Unit geyser : nearbyGeysers) {
				Unit builder = getClosestBuilder(geyser);
				if (builder != nullptr)
					doBuild(builder, buildingType, geyser->getTilePosition());
			}
		}

		if (u->isIdle()) {
			auto workers = u->getUnitsInRadius(SATRUATION_RADIUS, (IsGatheringMinerals));
			if (countUnit(UnitTypes::Terran_SCV, IsOwned) >= 70)
				continue;
			auto mineralFields = u->getUnitsInRadius(SATRUATION_RADIUS, (IsMineralField));
			if (workers.size() < mineralFields.size() * 2 && canAfford(UnitTypes::Terran_SCV))
				u->train(UnitTypes::Terran_SCV);
		}
		if (u->isIdle() && canAfford(UnitTypes::Terran_Comsat_Station))
			u->buildAddon(UnitTypes::Terran_Comsat_Station);
	}

	for (auto &u : _unitsByType[UnitTypes::Terran_Barracks])
		if (u->isIdle() && canAfford(UnitTypes::Terran_Marine))
			u->train(UnitTypes::Terran_Marine);

	for (auto &u : _unitsByType[UnitTypes::Terran_Academy]) {
		if (u->isIdle()) {
			if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::U_238_Shells) == 0)
				u->upgrade(UpgradeTypes::U_238_Shells);

			else if (Broodwar->self()->isResearchAvailable(TechTypes::Stim_Packs))
				u->research(TechTypes::Stim_Packs);
		}
	}

	for (auto &u : _unitsByType[UnitTypes::Terran_Engineering_Bay]) {
		if (u->isIdle())
			u->upgrade(UpgradeTypes::Terran_Infantry_Armor);

		if (u->isIdle())
			u->upgrade(UpgradeTypes::Terran_Infantry_Weapons);
	}

	timer_unitlogic.stop();

	if (Broodwar->getFrameCount() % 6 == 0) {
		timer_marinelogic.reset();
		for (auto &u : _unitsByType[UnitTypes::Terran_Marine]) {
			auto fleeFrom = u->getClosestUnit(IsEnemy && (CanAttack || GetType == UnitTypes::Terran_Bunker), 200);
			int friendlyCount;
			if (fleeFrom) {
				int enemyCount = fleeFrom->getUnitsInRadius(300, IsEnemy && (CanAttack || GetType == UnitTypes::Terran_Bunker)).size() + 1;
				friendlyCount = fleeFrom->getUnitsInRadius(400, IsOwned && !IsBuilding && !IsWorker).size();
				if (enemyCount + 3 > friendlyCount) {
					if (fleeFrom != nullptr) {
						u->move(u->getPosition() + u->getPosition() - fleeFrom->getPosition());
						continue;
					}
				}
			}
			else {
				friendlyCount = u->getUnitsInRadius(2000, IsOwned && !IsBuilding && !IsWorker).size();
			}

			auto enemyUnit = u->getClosestUnit(!IsDetected, Broodwar->self()->weaponMaxRange(WeaponTypes::Gauss_Rifle));
			if (enemyUnit)
				requestScan(enemyUnit->getPosition());

			enemyUnit = u->getClosestUnit(IsEnemy && (CanAttack || GetType == UnitTypes::Terran_Bunker) && IsDetected, Broodwar->self()->weaponMaxRange(WeaponTypes::Gauss_Rifle));
			if (enemyUnit) {
				if (!u->isStimmed() && Broodwar->self()->isResearchAvailable(TechTypes::Stim_Packs))
					u->useTech(TechTypes::Stim_Packs);
				continue;
			}

			enemyUnit = u->getClosestUnit(IsEnemy && (CanAttack || GetType == UnitTypes::Terran_Bunker) && IsDetected);
			if (enemyUnit) {
				u->attack(enemyUnit);
				continue;
			}

			enemyUnit = u->getClosestUnit(IsEnemy && IsDetected);
			if (enemyUnit) {
				u->attack(enemyUnit);
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
				}
				else {
					if (_unexploredBases.size())
						u->move((Position)(*_unexploredBases.begin())->Location());
				}
			}
		}
		timer_marinelogic.stop();
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
	if (!unit->getType().isAddon() && unit->getPlayer() == Broodwar->enemy()) {
		if (_knownEnemies.count(unit) && _knownEnemies[unit].second != unit->getType()) // If there is a new type for the unit
			_enemyUnitsByType[_knownEnemies[unit].second].erase(unit);

		if (_knownEnemies.count(unit) && _enemyUnitsByType[_knownEnemies[unit].second].empty())
			_enemyUnitsByType.erase(_knownEnemies[unit].second);

		if (_knownEnemies.find(unit) == _knownEnemies.end())
			_enemyUnitsByType[unit->getType()].insert(unit);

		_knownEnemies[unit] = { unit->getPosition(), unit->getType() };
	}
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
	if (unit->getType().isMineralField())			BWEMMap.OnMineralDestroyed(unit);
	else if (unit->getType().isSpecialBuilding())	BWEMMap.OnStaticBuildingDestroyed(unit);

	if (unit->getPlayer() == Broodwar->enemy() && !unit->getType().isAddon()) {
		_knownEnemies.erase(unit);
		_enemyUnitsByType[unit->getType()].erase(unit);
		if (_enemyUnitsByType[unit->getType()].empty())
			_enemyUnitsByType.erase(unit->getType());
	}

	if (unit->getPlayer() == Broodwar->self()) {
		_unitsByType[unit->getType()].erase(unit);
		_unitsInTypeSet.erase(unit);
	}
}

void Neohuman::onUnitMorph(Unit unit) {
	onUnitDiscover(unit);
	onUnitComplete(unit);
}

void Neohuman::onUnitRenegade(Unit unit) {
}

void Neohuman::onSaveGame(std::string gameName) {
}

void Neohuman::onUnitComplete(Unit unit) {
	if (unit->getPlayer() == Broodwar->self()) {
		if (_unitsInTypeSet.count(unit))
			_unitsByType[_unitsInTypeSet[unit]].erase(unit);

		_unitsInTypeSet[unit] = unit->getType();
		_unitsByType[unit->getType()].insert(unit);
	}
}
