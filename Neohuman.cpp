#include "Neohuman.h"

#include "Util.h"
#include "Triple.h"
#include "BuildingQueue.h"
#include "DetectionManager.h"
#include "SupplyManager.h"
#include "UnitManager.h"
#include "DrawingManager.h"
#include "ResourceManager.h"
#include "MapManager.h"

#include <iostream>

using namespace BWAPI;
using namespace Filter;

using namespace BWEM;
using namespace BWEM::BWAPI_ext;
using namespace BWEM::utils;

using namespace Neolib;

Neolib::DrawingManager defaultDrawingSettings = DrawingManager();

Neohuman neoInstance                      = Neohuman();
BWEM::Map &BWEMMap                        = BWEM::Map::Instance();

Neolib::BuildingQueue buildingQueue       = Neolib::BuildingQueue();
Neolib::DetectionManager detectionManager = Neolib::DetectionManager();
Neolib::ResourceManager resourceManager   = Neolib::ResourceManager();
Neolib::SupplyManager supplyManager       = Neolib::SupplyManager();
Neolib::UnitManager unitManager           = Neolib::UnitManager();
Neolib::DrawingManager drawingManager     = defaultDrawingSettings;
Neolib::MapManager mapManager             = Neolib::MapManager();

void Neohuman::onStart() {
	Timer timer_onStart;
	timer_onStart.reset();

	mainRace = (*(Broodwar->self()->getUnits().begin()))->getType().getRace();

	Broodwar->enableFlag(Flag::UserInput);

	// Uncomment the following line and the bot will know about everything through the fog of war (cheat).
	//Broodwar->enableFlag(Flag::CompleteMapInformation);

	Broodwar->setCommandOptimizationLevel(2);

	if (!Broodwar->isReplay()) {
		//if (Broodwar->enemy())
			//Broodwar << "The matchup is me (" << Broodwar->self()->getRace() << ") vs " << Broodwar->enemy()->getRace() << std::endl;
	}

	mapManager.init();

	Broodwar->sendText("onStart finished in %.1lf ms", timer_onStart.lastMeasuredTime);
}

void Neohuman::onEnd(bool didWin) {
	// Called when the game ends
	if (didWin) {

	} else {

	}

	neoInstance = Neohuman();
	BWEMMap = BWEM::Map::Instance();

	buildingQueue = Neolib::BuildingQueue();
	detectionManager = Neolib::DetectionManager();
	resourceManager = Neolib::ResourceManager();
	supplyManager = Neolib::SupplyManager();
	unitManager = Neolib::UnitManager();
	drawingManager = defaultDrawingSettings;
	mapManager = Neolib::MapManager();

}

void Neohuman::onFrame() {
	timer_total.reset();

	timer_drawinfo.reset();
	drawingManager.onFrame();
	timer_drawinfo.stop();

	// Return if the game is a replay or is paused
	if (Broodwar->isPaused() || Broodwar->isReplay())
		return;

	// Prevent spamming by only running our onFrame once every number of latency frames.
	// Latency frames are the number of frames before commands are processed.
	if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames())
		return;

	unitManager.onFrame();

	timer_managequeue.reset();
	buildingQueue.update();
	timer_managequeue.stop();

	timer_buildbuildings.reset();

	static int buildingFrame = 0;

	switch (buildingFrame++) {
		case 8:
			buildingFrame = 0;
		case 0:
			if (supplyManager.wantedAdditionalSupply().terran > 0 && resourceManager.canAfford(UnitTypes::Terran_Supply_Depot) && supplyManager.usedSupply().terran + buildingQueue.getQueuedSupply().terran < 400) {
				Unit builder = unitManager.getAnyBuilder();
				if (builder != nullptr)
					buildingQueue.doBuild(builder, UnitTypes::Terran_Supply_Depot, Broodwar->getBuildLocation(UnitTypes::Terran_Supply_Depot, builder->getTilePosition()));
			}
			break;
		case 1:
			if ((resourceManager.canAfford(UnitTypes::Terran_Barracks) && unitManager.countUnit(UnitTypes::Terran_Barracks, IsOwned) < unitManager.countUnit(UnitTypes::Terran_SCV, IsGatheringMinerals && IsOwned) / 6) || resourceManager.getSpendableResources().minerals >= 500 && unitManager.countFriendly(BWAPI::UnitTypes::Terran_Supply_Depot)) {
				Unit builder = unitManager.getAnyBuilder();
				if (builder != nullptr)
					buildingQueue.doBuild(builder, UnitTypes::Terran_Barracks, Broodwar->getBuildLocation(UnitTypes::Terran_Barracks, builder->getTilePosition()));
			}
			break;
		case 2:
			if (resourceManager.canAfford(UnitTypes::Terran_Academy) && unitManager.countUnit(UnitTypes::Terran_Barracks, IsOwned) >= 2 && !unitManager.countUnit(UnitTypes::Terran_Academy, IsOwned)) {
				Unit builder = unitManager.getAnyBuilder();
				if (builder != nullptr)
					buildingQueue.doBuild(builder, UnitTypes::Terran_Academy, Broodwar->getBuildLocation(UnitTypes::Terran_Academy, builder->getTilePosition()));
			}
			break;
		case 3:
			if (resourceManager.canAfford(UnitTypes::Terran_Factory) && !unitManager.countUnit(UnitTypes::Terran_Factory, IsOwned) && Broodwar->self()->supplyUsed() / 2 >= 70) {
				Unit builder = unitManager.getAnyBuilder();
				if (builder != nullptr)
					buildingQueue.doBuild(builder, UnitTypes::Terran_Factory, Broodwar->getBuildLocation(UnitTypes::Terran_Factory, builder->getTilePosition()));
			}
			break;
		case 4:
			if (resourceManager.canAfford(UnitTypes::Terran_Starport) && !unitManager.countUnit(UnitTypes::Terran_Starport, IsOwned) && Broodwar->self()->supplyUsed() / 2 >= 70) {
				Unit builder = unitManager.getAnyBuilder();
				if (builder != nullptr)
					buildingQueue.doBuild(builder, UnitTypes::Terran_Starport, Broodwar->getBuildLocation(UnitTypes::Terran_Starport, builder->getTilePosition()));
			}
			break;
		case 5:
			if (resourceManager.canAfford(UnitTypes::Terran_Science_Facility) && !unitManager.countUnit(UnitTypes::Terran_Science_Facility, IsOwned) && Broodwar->self()->supplyUsed() / 2 >= 80) {
				Unit builder = unitManager.getAnyBuilder();
				if (builder != nullptr)
					buildingQueue.doBuild(builder, UnitTypes::Terran_Science_Facility, Broodwar->getBuildLocation(UnitTypes::Terran_Science_Facility, builder->getTilePosition()));
			}
			break;
		case 6:
			if (resourceManager.canAfford(UnitTypes::Terran_Engineering_Bay) && unitManager.countUnit(UnitTypes::Terran_Engineering_Bay) < 2 && Broodwar->self()->supplyUsed() / 2 >= 40) {
				Unit builder = unitManager.getAnyBuilder();
				if (builder != nullptr)
					buildingQueue.doBuild(builder, UnitTypes::Terran_Engineering_Bay, Broodwar->getBuildLocation(UnitTypes::Terran_Engineering_Bay, builder->getTilePosition()));
			}
			break;
		case 7:
			if (resourceManager.canAfford(UnitTypes::Terran_Command_Center)){
				Unit builder = unitManager.getAnyBuilder();
				TilePosition pos = mapManager.getNextBasePosition();
				if (builder != nullptr && pos.isValid())
					buildingQueue.doBuild(builder, UnitTypes::Terran_Command_Center, pos);
			}
			break;
	}

	timer_buildbuildings.stop();

	mapManager.onFrame();

	timer_unitlogic.reset();

	for (auto &u : unitManager.getFriendlyUnitsByType(UnitTypes::Terran_SCV)) {
		auto enemyUnit = unitManager.getClosestEnemy(u, true);
		if (enemyUnit && u->getDistance(enemyUnit) <= WORKERAGGRORADIUS)
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

	for (auto &u : unitManager.getFriendlyUnitsByType(UnitTypes::Terran_Command_Center)) {
		if (u->isConstructing())
			continue;

		auto nearbyGeysers = u->getUnitsInRadius(SATRUATION_RADIUS, (GetType == UnitTypes::Resource_Vespene_Geyser));
		if (nearbyGeysers.size() && Broodwar->self()->supplyUsed() / 2 >= 13){
			auto buildingType = UnitTypes::Terran_Refinery;
			for (Unit geyser : nearbyGeysers) {
				Unit builder = unitManager.getClosestBuilder(geyser);
				if (builder != nullptr)
					buildingQueue.doBuild(builder, buildingType, geyser->getTilePosition());
			}
		}

		if (u->isIdle()) {
			auto workers = u->getUnitsInRadius(SATRUATION_RADIUS, (IsGatheringMinerals));
			if (unitManager.countUnit(UnitTypes::Terran_SCV, IsOwned) >= 70)
				continue;
			auto mineralFields = u->getUnitsInRadius(SATRUATION_RADIUS, (IsMineralField));
			if (workers.size() < mineralFields.size() * 2 && resourceManager.canAfford(UnitTypes::Terran_SCV))
				u->train(UnitTypes::Terran_SCV);
		}
		if (u->isIdle() && resourceManager.canAfford(UnitTypes::Terran_Comsat_Station))
			u->buildAddon(UnitTypes::Terran_Comsat_Station);
	}

	for (auto &u : unitManager.getFriendlyUnitsByType(UnitTypes::Terran_Barracks))
		if (u->isIdle() && resourceManager.canAfford(UnitTypes::Terran_Marine))
			u->train(UnitTypes::Terran_Marine);

	for (auto &u : unitManager.getFriendlyUnitsByType(UnitTypes::Terran_Academy)) {
		if (u->isIdle()) {
			if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::U_238_Shells) == 0)
				u->upgrade(UpgradeTypes::U_238_Shells);

			else if (Broodwar->self()->isResearchAvailable(TechTypes::Stim_Packs))
				u->research(TechTypes::Stim_Packs);
		}
	}

	for (auto &u : unitManager.getFriendlyUnitsByType(UnitTypes::Terran_Engineering_Bay)) {
		if (u->isIdle())
			u->upgrade(UpgradeTypes::Terran_Infantry_Armor);

		if (u->isIdle())
			u->upgrade(UpgradeTypes::Terran_Infantry_Weapons);
	}

	timer_unitlogic.stop();

	timer_marinelogic.reset();

	static int marineFrame = 0;
	if (marineFrame == 2)
		marineFrame = 0;
	else ++marineFrame;

	int marineN = 0;
	for (auto &u : unitManager.getFriendlyUnitsByType(UnitTypes::Terran_Marine)) {
		if (marineN++ % 2 == marineFrame)
			continue;
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

		if (u->getGroundWeaponCooldown())
			continue;

		BWAPI::Unit enemyUnit = unitManager.getClosestEnemy(u, IsVisible && !IsDetected);
		if (enemyUnit && enemyUnit->isVisible() && !enemyUnit->isDetected() && u->getDistance(enemyUnit) <= BWAPI::Broodwar->self()->weaponMaxRange(WeaponTypes::Gauss_Rifle))
			detectionManager.requestDetection(enemyUnit->getPosition());
		
		enemyUnit = unitManager.getClosestEnemy(u, true);
		if (enemyUnit && enemyUnit->isVisible() && enemyUnit->isDetected()) {
			if (u->getDistance(enemyUnit) < BWAPI::Broodwar->self()->weaponMaxRange(WeaponTypes::Gauss_Rifle)) {
				if (!u->isStimmed() && Broodwar->self()->isResearchAvailable(TechTypes::Stim_Packs))
					u->useTech(TechTypes::Stim_Packs);
				u->attack(enemyUnit);
				continue;
			}
			else {
				u->move(unitManager.lastKnownEnemyPosition(enemyUnit));
				continue;
			}
		}

		enemyUnit = unitManager.getClosestEnemy(u, false);
		if (enemyUnit && enemyUnit->isVisible() && enemyUnit->isDetected()) {
			if (u->getDistance(enemyUnit) < BWAPI::Broodwar->self()->weaponMaxRange(WeaponTypes::Gauss_Rifle)) {
				u->attack(enemyUnit);
				continue;
			}
			else {
				u->move(unitManager.lastKnownEnemyPosition(enemyUnit));
				continue;
			}
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
				if (mapManager.getUnexploredBases().size())
					u->move((Position)(*mapManager.getUnexploredBases().begin())->Location());
			}
		}
	}
	timer_marinelogic.stop();

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
	// if (target) {
	// 	Broodwar << "Nuclear Launch Detected at " << target << std::endl;
	// } else {
	// 	Broodwar->sendText("Where's the nuke?");
	// }
	// You can also retrieve all the nuclear missile targets using Broodwar->getNukeDots()!
}

void Neohuman::onUnitDiscover(Unit unit) {
	unitManager.onUnitDiscover(unit);
}

void Neohuman::onUnitEvade(Unit unit) {

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
	onUnitDiscover(unit);
}

void Neohuman::onUnitRenegade(Unit unit) {
	unitManager.onUnitRenegade(unit);
}

void Neohuman::onSaveGame(std::string gameName) {

}

void Neohuman::onUnitComplete(Unit unit) {
	unitManager.onUnitComplete(unit);
}
