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
#include "BaseManager.h"
#include "BuildingPlacer.h"
#include "FAP.h"
#include "SoundDatabase.h"

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

	playingRace = (*(Broodwar->self()->getUnits().begin()))->getType().getRace();
	wasRandom = BWAPI::Broodwar->self()->getRace() == BWAPI::Races::Random;

#ifndef SSCAIT

	Broodwar->enableFlag(Flag::UserInput);

#endif

	// Uncomment the following line and the bot will know about everything through the fog of war (cheat).
	// Broodwar->enableFlag(Flag::CompleteMapInformation);

	Broodwar->setCommandOptimizationLevel(2);

	if (!Broodwar->isReplay()) {
		//if (Broodwar->enemy())
			//Broodwar << "The matchup is me (" << Broodwar->self()->getRace() << ") vs " << Broodwar->enemy()->getRace() << std::endl;
	}

	mapManager.init();
	buildingPlacer.init();
	//combatSimulator.init();

	timer_onStart.stop();

#ifdef _DEBUG

	Broodwar->sendText("onStart finished in %.1lf ms", timer_onStart.lastMeasuredTime);

#endif

	//combatSimulator.onStart();

	/*
	Broodwar->sendText("power overwhelming");
	Broodwar->sendText("black sheep wall");
	//*/
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

	// Prevent spamming by only running our onFrame once every number of latency frames.
	// Latency frames are the number of frames before commands are processed.
	//if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames())
	//	return;

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

	if (!(Broodwar->isPaused() || Broodwar->isReplay() || Broodwar->getFrameCount() % Broodwar->getLatencyFrames())) {
		unitManager.onFrame();

		static BWAPI::Unit scoutingUnit = nullptr;

		timer_managequeue.reset();
		buildingQueue.onFrame();
		timer_managequeue.stop();

		timer_buildbuildings.reset();

		static int buildingFrame = 0;

		switch (buildingFrame++) {
			case 8:
				buildingFrame = 0;
			case 0:
				if (supplyManager.wantedAdditionalSupply().terran > 0 && resourceManager.canAfford(UnitTypes::Terran_Supply_Depot) && supplyManager.usedSupply().terran + buildingQueue.getQueuedSupply().terran < 400)
					buildingQueue.doBuild(UnitTypes::Terran_Supply_Depot);
				break;

			case 1:
				if ((resourceManager.canAfford(UnitTypes::Terran_Barracks) && unitManager.countFriendly(BWAPI::UnitTypes::Terran_Barracks) < 20 && unitManager.countFriendly(BWAPI::UnitTypes::Terran_Supply_Depot, false, true) && (unitManager.countFriendly(UnitTypes::Terran_Barracks) < unitManager.countUnit(UnitTypes::Terran_SCV, IsGatheringMinerals && IsOwned) / 6) || (resourceManager.getSpendableResources().minerals >= 550 && supplyManager.usedSupply().terran < 350 && supplyManager.availableSupply().terran == 400)))
					buildingQueue.doBuild(UnitTypes::Terran_Barracks);
				break;

			case 2:
				if (resourceManager.canAfford(UnitTypes::Terran_Academy) && unitManager.countFriendly(UnitTypes::Terran_Barracks) >= 3 && !unitManager.countFriendly(UnitTypes::Terran_Academy))
					buildingQueue.doBuild(UnitTypes::Terran_Academy);
				break;

			case 3:
				if (resourceManager.canAfford(UnitTypes::Terran_Factory) && !unitManager.countFriendly(UnitTypes::Terran_Factory) && supplyManager.usedSupply().terran / 2 >= 40)
					buildingQueue.doBuild(UnitTypes::Terran_Factory);
				break;

			case 4:
				if (resourceManager.canAfford(UnitTypes::Terran_Starport) && !unitManager.countFriendly(UnitTypes::Terran_Starport) && supplyManager.usedSupply().terran / 2 >= 50)
					buildingQueue.doBuild(UnitTypes::Terran_Starport);
				break;

			case 5:
				if (resourceManager.canAfford(UnitTypes::Terran_Science_Facility) && !unitManager.countFriendly(UnitTypes::Terran_Science_Facility) && supplyManager.usedSupply().terran / 2 >= 60)
					buildingQueue.doBuild(UnitTypes::Terran_Science_Facility);
				break;

			case 6:
				if (resourceManager.canAfford(UnitTypes::Terran_Engineering_Bay) && unitManager.countFriendly(UnitTypes::Terran_Engineering_Bay) < 1 + unitManager.countFriendly(UnitTypes::Terran_Science_Facility) && Broodwar->self()->supplyUsed() / 2 >= 70)
					buildingQueue.doBuild(UnitTypes::Terran_Engineering_Bay);
				break;

			case 7:
				if (resourceManager.canAfford(UnitTypes::Terran_Command_Center)) {
					TilePosition pos = mapManager.getNextBasePosition();
					if (pos.isValid())
						buildingQueue.doBuild(UnitTypes::Terran_Command_Center, pos);
				}
				break;
		}

		if (BWAPI::Broodwar->getFrameCount() % 8 == 0) {
			unsigned count = 0;
			for (auto &u : unitManager.getFriendlyUnitsByType()) {
				if (u.first.isWorker()) {
					for (auto &w : u.second) {
						bool hasBase = false;
						for (auto &b : baseManager.getAllBases()) {
							for (auto &mp : b.mineralMiners)
								if (mp.second.find(w) != mp.second.end())
									hasBase = true;
							for (auto &gg : b.gasMiners)
								if (gg.second.find(w) != gg.second.end())
									hasBase = true;
						}

						if (!hasBase && !w->isConstructing())
							baseManager.giveBackUnit(w);
					}
				}
			}
		}

		timer_buildbuildings.stop();

		mapManager.onFrame();

		timer_unitlogic.reset();

		for (auto &u : unitManager.getFriendlyUnitsByType(UnitTypes::Terran_Command_Center)) {
			if (u->isConstructing())
				continue;

			auto nearbyGeysers = u->getUnitsInRadius(SATRUATION_RADIUS, (GetType == UnitTypes::Resource_Vespene_Geyser));
			if (nearbyGeysers.size() && Broodwar->self()->supplyUsed() / 2 >= 30) {
				auto buildingType = UnitTypes::Terran_Refinery;
				for (Unit geyser : nearbyGeysers)
					buildingQueue.doBuild(UnitTypes::Terran_Refinery, geyser->getTilePosition());
			}

			if (u->isIdle() && resourceManager.canAfford(UnitTypes::Terran_Nuclear_Silo))
				u->buildAddon(UnitTypes::Terran_Nuclear_Silo);

			if (u->isIdle() && resourceManager.canAfford(UnitTypes::Terran_Comsat_Station) && !unitManager.countFriendly(UnitTypes::Terran_Covert_Ops))
				u->buildAddon(UnitTypes::Terran_Comsat_Station);

			if (u->isIdle()) {
				auto workers = u->getUnitsInRadius(SATRUATION_RADIUS, (IsGatheringMinerals));
				if (unitManager.countUnit(UnitTypes::Terran_SCV, IsOwned) >= 70)
					continue;
				auto mineralFields = u->getUnitsInRadius(SATRUATION_RADIUS, (IsMineralField));
				if (workers.size() < mineralFields.size() * 2 && resourceManager.canAfford(UnitTypes::Terran_SCV))
					u->train(UnitTypes::Terran_SCV);
			}
		}

		for (auto &u : unitManager.getFriendlyUnitsByType(UnitTypes::Terran_Science_Facility)) {
			if (u->canBuildAddon() && resourceManager.canAfford(UnitTypes::Terran_Covert_Ops))
				u->buildAddon(UnitTypes::Terran_Covert_Ops);
		}

		for (auto &u : unitManager.getFriendlyUnitsByType(UnitTypes::Terran_Covert_Ops)) {
			if (!u->isIdle())
				continue;

			if (u->canResearch(BWAPI::TechTypes::Personnel_Cloaking) && resourceManager.canAfford(BWAPI::TechTypes::Personnel_Cloaking))
				u->research(BWAPI::TechTypes::Personnel_Cloaking);
			if (!BWAPI::Broodwar->self()->hasResearched(BWAPI::TechTypes::Personnel_Cloaking))
				continue;

			if(u->canResearch(BWAPI::TechTypes::Lockdown) && resourceManager.canAfford(BWAPI::TechTypes::Lockdown))
				u->research(BWAPI::TechTypes::Lockdown);
			if (!BWAPI::Broodwar->self()->hasResearched(BWAPI::TechTypes::Lockdown))
				continue;

			if (u->canUpgrade(BWAPI::UpgradeTypes::Moebius_Reactor) && resourceManager.canAfford(BWAPI::UpgradeTypes::Moebius_Reactor))
				u->upgrade(BWAPI::UpgradeTypes::Moebius_Reactor);
			if (!BWAPI::Broodwar->self()->getUpgradeLevel(BWAPI::UpgradeTypes::Moebius_Reactor))
				continue;

			if (u->canUpgrade(BWAPI::UpgradeTypes::Ocular_Implants) && resourceManager.canAfford(BWAPI::UpgradeTypes::Ocular_Implants))
				u->upgrade(BWAPI::UpgradeTypes::Ocular_Implants);

		}

		for (auto &u : unitManager.getFriendlyUnitsByType(UnitTypes::Terran_Barracks))
			if (unitManager.countFriendly(UnitTypes::Terran_Marine) + unitManager.countFriendly(UnitTypes::Terran_Ghost) >= 100)
				break;
			else if (u->isIdle() && resourceManager.canAfford(UnitTypes::Terran_Ghost) && resourceManager.getSpendableResources().gas > 250 && u->canTrain(UnitTypes::Terran_Ghost) && randint(0, 4) < 2) // 40% chance to make ghost
				u->train(UnitTypes::Terran_Ghost);
			else if (u->isIdle() && resourceManager.canAfford(UnitTypes::Terran_Marine) && u->canTrain(UnitTypes::Terran_Marine))
				u->train(UnitTypes::Terran_Marine);

		for (auto &u : unitManager.getFriendlyUnitsByType(UnitTypes::Terran_Academy)) {
			if (u->isIdle()) {
				if (Broodwar->self()->getUpgradeLevel(UpgradeTypes::U_238_Shells) == 0)
					u->upgrade(UpgradeTypes::U_238_Shells);

				else if (Broodwar->self()->isResearchAvailable(TechTypes::Stim_Packs))
					u->research(TechTypes::Stim_Packs);
			}
		}

		for (auto &u : unitManager.getFriendlyUnitsByType(UnitTypes::Terran_Nuclear_Silo))
			if (u->isIdle() && u->canTrain(BWAPI::UnitTypes::Terran_Nuclear_Missile) && resourceManager.canAfford(BWAPI::UnitTypes::Terran_Nuclear_Missile))
				u->train(BWAPI::UnitTypes::Terran_Nuclear_Missile);

		for (auto &u : unitManager.getFriendlyUnitsByType(UnitTypes::Terran_Engineering_Bay)) {
			if (u->isIdle())
				u->upgrade(UpgradeTypes::Terran_Infantry_Armor);

			if (u->isIdle())
				u->upgrade(UpgradeTypes::Terran_Infantry_Weapons);
		}

		timer_unitlogic.stop();

		baseManager.onFrame();

		timer_marinelogic.reset();

		static int marineFrame = 0;
		if (marineFrame == 2)
			marineFrame = 0;
		else ++marineFrame;

		int marineN = 0;
		for (auto &u : unitManager.getFriendlyUnitsByType(UnitTypes::Terran_Marine)) {
			if (marineN++ % 2 == marineFrame)
				continue;

			bool scaredAF = false;

			for (auto &nd : BWAPI::Broodwar->getNukeDots()) {
				if (u->getDistance(nd) <= 20 * 32) {
					u->move(u->getPosition() + u->getPosition() - nd);
					scaredAF = true;
				}
			}

			if (!scaredAF) {
				auto fleeFrom = u->getClosestUnit(IsEnemy && (CanAttack || GetType == UnitTypes::Terran_Bunker), 300);
				int friendlyCount;
				if (fleeFrom) {
					int enemyCount = fleeFrom->getUnitsInRadius(300, IsEnemy && (CanAttack || GetType == UnitTypes::Terran_Bunker) && GetType != BWAPI::UnitTypes::Protoss_Interceptor).size() + 1;
					friendlyCount = fleeFrom->getUnitsInRadius(400, IsOwned && !IsBuilding && !IsWorker).size();
					if (!squadManager.shouldAttack(unitManager.getSimResults())) {
						if (fleeFrom != nullptr) {
							u->move(u->getPosition() + u->getPosition() - fleeFrom->getPosition());
							continue;
						}
					}
				}

				if (u->getGroundWeaponCooldown() > Broodwar->getRemainingLatencyFrames())
					continue;

				std::shared_ptr <EnemyData> enm = unitManager.getBestTarget(u);
				if (enm) {
					if (u->getDistance(enm->lastPosition) <= BWAPI::Broodwar->self()->weaponMaxRange(u->getType().groundWeapon())) {
						if (!enm->u->isVisible())
							detectionManager.requestDetection(enm->lastPosition);
						else if (!enm->u->isDetected())
							detectionManager.requestDetection(enm->lastPosition);

						if (enm->u->isVisible() && enm->u->isDetected()) {
							if (!u->isStimmed() && Broodwar->self()->isResearchAvailable(TechTypes::Stim_Packs) && u->getHitPoints() < 20)
								u->useTech(TechTypes::Stim_Packs);
							u->attack(enm->u);
							continue;
						}
					}
					else {
						if (enm->u->isVisible())
							u->attack(enm->u);
						else
							u->move(enm->lastPosition);
						continue;
					}
				}

				auto closeSpecialBuilding = u->getClosestUnit(IsCritter && !IsInvincible, 200);
				if (closeSpecialBuilding)
					u->attack(closeSpecialBuilding);

				auto closestMarine = u->getClosestUnit(GetType == UnitTypes::Terran_Marine/*, 800*/);
				if (closestMarine) {
					auto walk = u->getPosition() - closestMarine->getPosition();
					u->move(u->getPosition() + walk);
					continue;
				}
				else {
					if (mapManager.getUnexploredBases().size()) {
						u->move((Position)(*mapManager.getUnexploredBases().begin())->Location());
						continue;
					}
				}
				u->stop();
			}
		}

		for (auto &u : unitManager.getFriendlyUnitsByType(UnitTypes::Terran_Ghost)) {
			if (marineN++ % 2 == marineFrame)
				continue;

			if (u->isUnderAttack() && u->getEnergy() >= 200)
				u->cloak();

			bool scaredAF = false;

			for (auto &nd : BWAPI::Broodwar->getNukeDots()) {
				if (u->getDistance(nd) <= 20 * 32) {
					u->move(u->getPosition() + u->getPosition() - nd);
					scaredAF = true;
				}
			}

			if (!scaredAF) {
				if (unitManager.getNumArmedSilos() && (u->getEnergy() >= 50 || (u->getEnergy() >= 25 && u->isCloaked()))) {
					auto nukePos = unitManager.getBestNuke(u);
					if (nukePos != BWAPI::Positions::None) {
						u->cloak();
						u->useTech(BWAPI::TechTypes::Nuclear_Strike, nukePos);
						continue;
					}
				}

				auto lockdownTarget = u->getClosestUnit(IsEnemy && LockdownTime < 50 && IsMechanical && !IsWorker && !IsInvincible && IsDetected && GetType != BWAPI::UnitTypes::Protoss_Interceptor && GetType != BWAPI::UnitTypes::Terran_Vulture, 8 * 32 + 50);
				if (lockdownTarget && unitManager.isAllowedToLockdown(lockdownTarget, u) && u->canUseTech(BWAPI::TechTypes::Lockdown, lockdownTarget)) {
					u->useTech(BWAPI::TechTypes::Lockdown, lockdownTarget), unitManager.reserveLockdown(lockdownTarget, u);
					continue;
				}

				auto fleeFrom = u->getClosestUnit(IsEnemy && (CanAttack || GetType == UnitTypes::Terran_Bunker), 300);
				if (fleeFrom) {
					if (!squadManager.shouldAttack(unitManager.getSimResults())) {
						if (fleeFrom != nullptr) {
							u->move(u->getPosition() + u->getPosition() - fleeFrom->getPosition());
							continue;
						}
					}
				}

				if (u->getGroundWeaponCooldown() > Broodwar->getRemainingLatencyFrames())
					continue;

				std::shared_ptr <EnemyData> enm = unitManager.getBestTarget(u);
				if (enm) {
					if (u->getDistance(enm->lastPosition) <= BWAPI::Broodwar->self()->weaponMaxRange(u->getType().groundWeapon())) {
						if (!enm->u->isVisible())
							detectionManager.requestDetection(enm->lastPosition);
						else if (!enm->u->isDetected())
							detectionManager.requestDetection(enm->lastPosition);

						if (enm->u->isVisible() && enm->u->isDetected()) {
							u->attack(enm->u);
							continue;
						}
					}
					else {
						if (enm->u->isVisible())
							u->attack(enm->u);
						else
							u->move(enm->lastPosition);
						continue;
					}
				}

				auto closestGhost = u->getClosestUnit(GetType == UnitTypes::Terran_Ghost);
				if (closestGhost) {
					auto walk = u->getPosition() - closestGhost->getPosition();
					u->move(u->getPosition() + walk);
					continue;
				}
				u->stop();
			}
		}
	}

	timer_marinelogic.stop();

	timer_drawinfo.reset();
	drawingManager.onFrame();
	timer_drawinfo.stop();

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

	// if (target) {
	// 	Broodwar << "Nuclear Launch Detected at " << target << std::endl;
	// } else {
	// 	Broodwar->sendText("Where's the nuke?");
	// }
	// You can also retrieve all the nuclear missile targets using Broodwar->getNukeDots()!
}

void Neohuman::onUnitDiscover(Unit unit) {
	unitManager.onUnitDiscover(unit);
	baseManager.onUnitDiscover(unit);
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
	buildingQueue.onUnitCreate(unit);
}

void Neohuman::onUnitDestroy(Unit unit) {
	unitManager.onUnitDestroy(unit);
	buildingQueue.onUnitDestroy(unit);
	baseManager.onUnitDestroy(unit);
}

void Neohuman::onUnitMorph(Unit unit) {
	unitManager.onUnitMorph(unit);
	buildingQueue.onUnitMorph(unit);
}

void Neohuman::onUnitRenegade(Unit unit) {
	unitManager.onUnitRenegade(unit);
	baseManager.onUnitRenegade(unit);
}

void Neohuman::onSaveGame(std::string gameName) {

}

void Neohuman::onUnitComplete(Unit unit) {
	unitManager.onUnitComplete(unit);
	buildingQueue.onUnitComplete(unit);
	baseManager.onUnitComplete(unit);
}
