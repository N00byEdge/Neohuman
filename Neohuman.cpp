#include "Neohuman.h"

#include "BaseManager.h"
#include "BuildingPlacer.h"
#include "BuildingQueue.h"
#include "DrawingManager.h"
#include "MapManager.h"
#include "ResourceManager.h"
#include "SoundDatabase.h"
#include "SquadManager.h"
#include "SupplyManager.h"
#include "UnitManager.h"
#include "Util.h"

#include <fstream>

using namespace BWAPI;
using namespace Filter;

using namespace BWEM;
using namespace BWAPI_ext;
using namespace utils;

using namespace Neolib;

Neohuman *neoInstance;

void Neohuman::onStart() {
  try {
  soundDatabase.loadSounds();
  if(Broodwar->enemy()) {
    if(Broodwar->enemy()->getName() == "Alice" && randint(0, 19) == 0)
      soundDatabase.play_sound("alice");
    else {
      const std::vector<const char *> openingStrings = {
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
      Broodwar->sendText(openingStrings[randint(0, static_cast<int>(openingStrings.size()) - 1)]);
    }
  }

  playingRace = (*(Broodwar->self()->getUnits().begin()))->getType().getRace();
  wasRandom   = Broodwar->self()->getRace() == Races::Random;

  if constexpr(!IsOnTournamentServer()) {
    Broodwar->enableFlag(Flag::UserInput);
    Broodwar->setFrameSkip(50);
    Broodwar->setLocalSpeed(0);
  }

  Broodwar->setCommandOptimizationLevel(2);

  if(!Broodwar->isReplay()) { }

  mapManager.init();
  buildingPlacer.init();
  }
  catch (...) { }
}

void Neohuman::onEnd(const bool didWin) {
  try {
    if (!didWin)
      Broodwar->sendText("Plagiarism! %s OP!", Broodwar->enemy()->getRace().getName().c_str());
    // Called when the game ends
    auto wins = 0;
    auto losses = 0;
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

    if (didWin)
      ++wins;
    else
      ++losses;

    std::ofstream of("bwapi-data/write/" + Broodwar->enemy()->getName() + ".txt");
    if (of)
      of << wins << " " << losses << "\n";
  } catch(...) { }

  try {
    soundDatabase.unload();
  } catch(...) { }
}

void Neohuman::onFrame() {
  try {
    if constexpr(!IsOnTournamentServer())
      if(supplyManager.usedSupply()() == 0)
        Broodwar->leaveGame();

    static auto lastFramePaused = -5;

    static const std::vector<std::string> pauseStrings = {
      "No, " + Broodwar->enemy()->getName() + ", I cannot let you do that.",
      "No pets, no Pause.", "Your JIT compiler can't keep up? Too bad."
    };

    static auto pauseIt = pauseStrings.begin();
    if(Broodwar->isPaused()) {
      if(pauseIt != pauseStrings.end() && lastFramePaused < Broodwar->getFrameCount()) {
        lastFramePaused = Broodwar->getFrameCount();
        Broodwar->sendText(pauseIt->c_str());
        ++pauseIt;
      }
      Broodwar->resumeGame();
    }

    if(!(Broodwar->isPaused() || Broodwar->isReplay())) {
      unitManager.onFrame();

      static Unit scoutingUnit = nullptr;

      static auto buildingFrame = 0;

      switch(buildingFrame++) {
      case 8:
        buildingFrame = 0;
      case 0:
        for(auto &race: playableRaces) {
          const auto supplyProvider = race.getSupplyProvider();
          if(supplyManager.wantedAdditionalSupply()(race) > 0
              && supplyManager.usedSupply()(race) + buildingQueue.getQueuedSupply()(race) < 400)
            buildingQueue.doBuild(supplyProvider);
        }
        break;

      case 1:
        if(((resourceManager.canAfford(UnitTypes::Terran_Barracks) && unitManager.countFriendly(UnitTypes::Terran_Barracks) < 20
            && (unitManager.countFriendly(UnitTypes::Terran_Supply_Depot, false, true)
            && (unitManager.countFriendly(UnitTypes::Terran_Barracks) <= unitManager.countUnit(UnitTypes::Terran_SCV, IsOwned && Exists, false) / 7
            || unitManager.countFriendly(UnitTypes::Terran_Barracks, false, true) < 2)))
            || (resourceManager.getSpendableResources().minerals >= 550
            && supplyManager.usedSupply().terran < 350
            && supplyManager.availableSupply().terran == 400)))
          buildingQueue.doBuild(UnitTypes::Terran_Barracks);
        break;

      case 2:
        if(resourceManager.canAfford(UnitTypes::Terran_Academy)
            && unitManager.countFriendly(UnitTypes::Terran_Barracks) >= 2
            && !unitManager.countFriendly(UnitTypes::Terran_Academy))
          buildingQueue.doBuild(UnitTypes::Terran_Academy);
        break;

      case 3:
        if(resourceManager.canAfford(UnitTypes::Terran_Factory)
            && !unitManager.countFriendly(UnitTypes::Terran_Factory)
            && supplyManager.usedSupply().terran / 2 >= 40)
          buildingQueue.doBuild(UnitTypes::Terran_Factory);
        break;

      case 4:
        if(resourceManager.canAfford(UnitTypes::Terran_Starport)
            && !unitManager.countFriendly(UnitTypes::Terran_Starport)
            && supplyManager.usedSupply().terran / 2 >= 50)
          buildingQueue.doBuild(UnitTypes::Terran_Starport);
        break;

      case 5:
        if(resourceManager.canAfford(UnitTypes::Terran_Science_Facility)
            && !unitManager.countFriendly(UnitTypes::Terran_Science_Facility)
            && supplyManager.usedSupply().terran / 2 >= 60)
          buildingQueue.doBuild(UnitTypes::Terran_Science_Facility);
        break;

      case 6:
        if(resourceManager.canAfford(UnitTypes::Terran_Engineering_Bay)
            && unitManager.countFriendly(UnitTypes::Terran_Engineering_Bay)
              < 1 + unitManager.countFriendly(UnitTypes::Terran_Science_Facility)
            && Broodwar->self()->supplyUsed() / 2 >= 70)
          buildingQueue.doBuild(UnitTypes::Terran_Engineering_Bay);
        break;

      case 7:
        for(auto &race: playableRaces) {
          if(resourceManager.canAfford(ResourceCount(race.getResourceDepot()) * 0.6f)) {
            auto pos = mapManager.getNextBasePosition();
            if(pos.isValid())
              buildingQueue.doBuild(race.getResourceDepot(), pos);
          }
        }
        break;
      }

      if(Broodwar->getFrameCount() % 8 == 0) {
        for(auto &u: unitManager.getFriendlyUnitsByType()) {
          if(u.first.isWorker()) {
            for(auto &w: u.second) {
              bool hasBase = false;
              for(auto &b: baseManager.getAllBases()) {
                for(auto &mp: b.mineralMiners)
                  if(mp.second.find(w) != mp.second.end())
                    hasBase = true;
                for(auto &gg: b.gasMiners)
                  if(gg.second.find(w) != gg.second.end())
                    hasBase = true;
              }

              if(!hasBase && !buildingQueue.isWorkerBuilding(w))
                baseManager.returnUnit(w);
            }
          }
        }
      }

      mapManager.onFrame();

      for(auto &b: baseManager.getAllBases()) {
        if(b.resourceDepot->isConstructing())
          continue;

        if(Broodwar->self()->supplyUsed() / 2 >= 18) {
          for(auto &gg: b.gasGeysers)
            buildingQueue.doBuild(b.race.getRefinery(), gg->getTilePosition());
        }

        if(b.race == Races::Terran) {
          auto const maxNumComsats = (BWAPI::Broodwar->enemy()->getRace() == BWAPI::Races::Terran ? 2 : 3);
          if(b.resourceDepot->isIdle() && resourceManager.canAfford(UnitTypes::Terran_Nuclear_Silo)
              && unitManager.countFriendly(UnitTypes::Terran_Comsat_Station) >= maxNumComsats)
            b.resourceDepot->buildAddon(UnitTypes::Terran_Nuclear_Silo);

          if(b.resourceDepot->isIdle() && resourceManager.canAfford(UnitTypes::Terran_Comsat_Station)
              && (!unitManager.countFriendly(UnitTypes::Terran_Covert_Ops) || unitManager.countFriendly(UnitTypes::Terran_Comsat_Station) < maxNumComsats))
            b.resourceDepot->buildAddon(UnitTypes::Terran_Comsat_Station);
        }

        if(b.resourceDepot->isIdle() && resourceManager.canAfford(b.race.getWorker())) {
          if(unitManager.countUnit(b.race.getWorker(), IsOwned) >= 70)
            continue;
          for(auto &mf: b.mineralMiners) {
            if(mf.second.size() < 2) {
              b.resourceDepot->train(b.race.getWorker());
              break;
            }
          }
        }
      }

      for(auto &u: unitManager.getFriendlyUnitsByType(UnitTypes::Terran_Science_Facility)) {
        if(u->canBuildAddon() && resourceManager.canAfford(UnitTypes::Terran_Covert_Ops))
          u->buildAddon(UnitTypes::Terran_Covert_Ops);
      }

      for(auto &u: unitManager.getFriendlyUnitsByType(UnitTypes::Terran_Covert_Ops)) {
        if(!u->isIdle())
          continue;

        if(u->canResearch(TechTypes::Personnel_Cloaking) && resourceManager.canAfford(TechTypes::Personnel_Cloaking))
          u->research(TechTypes::Personnel_Cloaking);
        if(!Broodwar->self()->hasResearched(TechTypes::Personnel_Cloaking))
          continue;

        if(u->canResearch(TechTypes::Lockdown) && resourceManager.canAfford(TechTypes::Lockdown))
          u->research(TechTypes::Lockdown);
        if(!Broodwar->self()->hasResearched(TechTypes::Lockdown))
          continue;

        if(u->canUpgrade(UpgradeTypes::Moebius_Reactor) && resourceManager.canAfford(UpgradeTypes::Moebius_Reactor))
          u->upgrade(UpgradeTypes::Moebius_Reactor);
        if(!Broodwar->self()->getUpgradeLevel(UpgradeTypes::Moebius_Reactor))
          continue;

        if(u->canUpgrade(UpgradeTypes::Ocular_Implants) && resourceManager.canAfford(UpgradeTypes::Ocular_Implants))
          u->upgrade(UpgradeTypes::Ocular_Implants);
      }

      for(auto &u: unitManager.getFriendlyUnitsByType(UnitTypes::Terran_Barracks)) {
        if(unitManager.countFriendly(UnitTypes::Terran_Marine) + unitManager.countFriendly(UnitTypes::Terran_Ghost) >= 100)
          break;
        if(u->isIdle() && resourceManager.canAfford(UnitTypes::Terran_Medic)
            && resourceManager.getSpendableResources().gas > 120
            && u->canTrain(UnitTypes::Terran_Medic)
            && unitManager.countFriendly(UnitTypes::Terran_Ghost) + unitManager.countFriendly(UnitTypes::Terran_Marine)
              > 6 * unitManager.countFriendly(UnitTypes::Terran_Medic)
            && randint(0, 4) < 1) // 20% chance to make medic
          u->train(UnitTypes::Terran_Medic);

        else if(u->isIdle() && resourceManager.canAfford(UnitTypes::Terran_Ghost)
            && resourceManager.getSpendableResources().gas > 250
            && u->canTrain(UnitTypes::Terran_Ghost)
            && randint(0, 4) < 1) // 20% chance to make ghost
          u->train(UnitTypes::Terran_Ghost);

        else if(u->isIdle() && resourceManager.canAfford(UnitTypes::Terran_Marine)
            && u->canTrain(UnitTypes::Terran_Marine))
          u->train(UnitTypes::Terran_Marine);
      }

      for(auto &u: unitManager.getFriendlyUnitsByType(UnitTypes::Terran_Academy)) {
        if(u->isIdle()) {
          if(Broodwar->self()->getUpgradeLevel(UpgradeTypes::U_238_Shells) == 0)
            u->upgrade(UpgradeTypes::U_238_Shells);

          else if(Broodwar->self()->isResearchAvailable(TechTypes::Stim_Packs))
            u->research(TechTypes::Stim_Packs);
        }
      }

      for(auto &u:
          unitManager.getFriendlyUnitsByType(UnitTypes::Terran_Nuclear_Silo))
        if(u->isIdle()
            && u->canTrain(UnitTypes::Terran_Nuclear_Missile)
            && resourceManager.canAfford(UnitTypes::Terran_Nuclear_Missile))
          u->train(UnitTypes::Terran_Nuclear_Missile);

      for(auto &u: unitManager.getFriendlyUnitsByType(UnitTypes::Terran_Engineering_Bay)) {
        if(u->isIdle())
          u->upgrade(UpgradeTypes::Terran_Infantry_Armor);

        if(u->isIdle())
          u->upgrade(UpgradeTypes::Terran_Infantry_Weapons);
      }
    }

    baseManager.onFrame();
    squadManager.onFrame();
    drawingManager.onFrame();
    buildingQueue.onFrame();
  }
  catch(std::exception &e) { BotError(std::string(e.what())); }
}

void Neohuman::onSendText(std::string text) {}

void Neohuman::onReceiveText(Player player, std::string text) {}

void Neohuman::onPlayerLeft(Player const player) { Broodwar->sendText("Goodbye %s!", player->getName().c_str()); }

void Neohuman::onNukeDetect(Position const target) { unitManager.onNukeDetect(target); }

void Neohuman::onUnitDiscover(Unit const unit) {
  unitManager.onUnitDiscover(unit);
  baseManager.onUnitDiscover(unit);
}

void Neohuman::onUnitEvade(Unit const unit) { unitManager.onUnitEvade(unit); }

void Neohuman::onUnitShow(Unit const unit) { unitManager.onUnitShow(unit); }

void Neohuman::onUnitHide(Unit const unit) { unitManager.onUnitHide(unit); }

void Neohuman::onUnitCreate(Unit const unit) {
  unitManager.onUnitCreate(unit);
  buildingQueue.onUnitCreate(unit);
  if(UnitManager::isEnemy(unit))
    squadManager.onUnitComplete(unit);
}

void Neohuman::onUnitDestroy(Unit const unit) {
  squadManager.onUnitDestroy(unit);
  unitManager.onUnitDestroy(unit);
  buildingQueue.onUnitDestroy(unit);
  baseManager.onUnitDestroy(unit);
}

void Neohuman::onUnitMorph(Unit const unit) {
  unitManager.onUnitMorph(unit);
  buildingQueue.onUnitMorph(unit);
}

void Neohuman::onUnitRenegade(Unit const unit) {
  unitManager.onUnitRenegade(unit);
  baseManager.onUnitRenegade(unit);
}

void Neohuman::onSaveGame(std::string gameName) {}

void Neohuman::onUnitComplete(Unit const unit) {
  unitManager.onUnitComplete(unit);
  buildingQueue.onUnitComplete(unit);
  baseManager.onUnitComplete(unit);
  if(UnitManager::isOwn(unit))
    squadManager.onUnitComplete(unit);
}
