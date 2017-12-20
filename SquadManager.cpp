#include "SquadManager.h"

#include "DetectionManager.h"
#include "MapManager.h"

Neolib::SquadManager squadManager;

namespace Neolib {

Squad::Squad()
    : id([]() {
        static int nextid = 0;
        return nextid++;
      }()) {}

int Squad::squadDistance(BWAPI::Position p) {
  auto dx = (pos.x - p.x), dy = (pos.y - p.y);
  return static_cast<int>(sqrt(dx * dx + dy * dy));
}

EnemySquad::~EnemySquad() {
  for (auto &fs : squadManager.getFriendlySquads())
    fs->engagedSquads.erase(this);
}

void EnemySquad::addUnit(BWAPI::Unit unit) {
  units.insert(unitManager.getEnemyData(unit));
  squadManager.squadLookup[unit] = this;
}

void EnemySquad::removeUnit(BWAPI::Unit unit) {
  units.erase(unitManager.getEnemyData(unit));
}

void EnemySquad::updatePosition() {
  int xsum = 0, ysum = 0;
  for (auto &u : units) {
    xsum += u->lastPosition.x;
    ysum += u->lastPosition.y;
  }

  if (units.size()) {
    pos = BWAPI::Position(xsum / (int)units.size(), ysum / (int)units.size());

    radius1 = 0;
    radius2 = 0;

    if (units.size() > 1) {
      for (auto &u : units)
        radius1 += (pos.x - u->lastPosition.x) * (pos.x - u->lastPosition.x) +
                   (pos.y - u->lastPosition.y) * (pos.y - u->lastPosition.y);

      radius1 = (int)(17. * pow(units.size(), .65));
    }

    radius1 += 30;
    radius2 = radius1 + 10;

    MINE(radius1, 500);
    MINE(radius2, 510);
  } else
    pos = BWAPI::Positions::None;
}

void FriendlySquad::addUnit(BWAPI::Unit unit) { units.insert(unit); }

void FriendlySquad::removeUnit(BWAPI::Unit unit) { units.erase(unit); }

void FriendlySquad::updatePosition() {
  int xsum = 0, ysum = 0;
  for (auto &u : units) {
    xsum += u->getPosition().x;
    ysum += u->getPosition().y;
  }

  pos = BWAPI::Position(xsum / (int)units.size(), ysum / (int)units.size());

  radius1 = 0;
  radius2 = 0;

  if (units.size() > 1) {
    for (auto &u : units)
      radius1 += (pos.x - u->getPosition().x) * (pos.x - u->getPosition().x) +
                 (pos.y - u->getPosition().y) * (pos.y - u->getPosition().y);

    radius1 = (int)(17. * pow(units.size(), .65));
  }

  radius1 += 30;
  radius2 = radius1 + 10;

  MINE(radius1, 500);
  MINE(radius2, 510);
}

bool SquadManager::shouldAttack(SimResults sr) {
  BWAPI::Broodwar << sr.shortWin;
  return (sr.postsim.scores.second * 5 <= sr.presim.scores.second) ||
         (sr.shortWin >= 0);
}

void SquadManager::onFrame() {
  // Add friendly unmanaged units
  for (auto it = unmanagedUnits.begin(); it != unmanagedUnits.end(); ++it) {
    if (UnitManager::isOwn(*it)) {
      if ([&]() {
            for (auto &fs : friendlySquads) {
              if (fs->squadDistance((*it)->getPosition()) < fs->radius1) {
                addUnitToSquad(*it, fs);
                return false;
              }
            }
            return true;
          }()) {
        auto nfs = std::make_shared<FriendlySquad>();
        friendlySquads.push_back(nfs);
        addUnitToSquad(*it, nfs);
      }
    } else if (UnitManager::isEnemy(*it)) {
      auto ed = unitManager.getEnemyData(*it);
      if ([&]() {
            for (auto &es : enemySquads) {
              if (es->squadDistance(ed->lastPosition) < es->radius1) {
                addUnitToSquad(*it, es);
                return false;
              }
            }
            return true;
          }()) {
        auto nes = std::make_shared<EnemySquad>();
        enemySquads.push_back(nes);
        addUnitToSquad(*it, nes);
      }
    }
  }

  unmanagedUnits.clear();

  // Clean up 0 sized squads
  for (auto it = enemySquads.begin(); it != enemySquads.end();)
    if (!(*it)->numUnits())
      it = enemySquads.erase(it);
    else
      ++it;

  for (auto it = friendlySquads.begin(); it != friendlySquads.end();)
    if (!(*it)->numUnits())
      it = friendlySquads.erase(it);
    else
      ++it;

  for (auto &fs : friendlySquads) { // Kick out units from squad
    fs->updatePosition();
    if ([&]() {
          for (auto u : fs->units) { // Return value is if squad vector
                                     // iterators are invalidated
            if (fs->squadDistance(u->getPosition()) > fs->radius2) {
              // If we're out of range of the squad, remove the unit from it
              removeFromSquad(u);

              if ([&]() {
                    for (auto &fs2 :
                         friendlySquads) { // Find new squad for unit
                      if (fs2->squadDistance(u->getPosition()) < fs2->radius1) {
                        addUnitToSquad(u, fs2);
                        return false;
                      }
                    }
                    return true;
                  }()) { // If there is none, create a new squad for the unit
                auto ns = std::make_shared<FriendlySquad>();
                friendlySquads.push_back(ns);
                addUnitToSquad(u, ns);
              }
              return true;
            }
          }
          return false;
        }()) { // If iterator invalidated, break the loop
      break;
    }
  }

  for (auto &es : enemySquads) {
    es->updatePosition();
    if ([&]() {
          for (auto u : es->units) {
            if (es->squadDistance(u->lastPosition) > es->radius2) {
              removeFromSquad(u->u);
              if ([&]() {
                    for (auto &es2 : enemySquads) { // Find new squad for unit
                      if (es2->squadDistance(u->lastPosition) < es2->radius1) {
                        addUnitToSquad(u->u, es2);
                        return false;
                      }
                    }
                    return true;
                  }()) { // If there is none, create a new squad for the unit
                auto ns = std::make_shared<EnemySquad>();
                enemySquads.push_back(ns);
                addUnitToSquad(u->u, ns);
              }
              return true;
            }
          }
          return false;
        }()) { // If iterator invalidated, break the loop
      break;
    }
  }

  // Clean up 0 sized squads
  for (auto it = enemySquads.begin(); it != enemySquads.end();)
    if (!(*it)->numUnits())
      it = enemySquads.erase(it);
    else
      ++it;

  for (auto it = friendlySquads.begin(); it != friendlySquads.end();)
    if (!(*it)->numUnits())
      it = friendlySquads.erase(it);
    else
      ++it;

  // Squad merging
  bool escape = false;
  for (auto s1 = friendlySquads.begin(); !escape && s1 != friendlySquads.end();
       ++s1) {
    for (auto s2 = friendlySquads.begin();
         !escape && s2 != friendlySquads.end();) {
      if (s1 == s2) {
        ++s2;
        continue;
      }
      // Very hacky merge
      if ((*s1)->pos.getApproxDistance((*s2)->pos) < (*s1)->radius1) {
        std::shared_ptr<FriendlySquad> fs = *s2; // Hold this squad for a bit
        for (auto &u : fs->units) {
          (*s1)->addUnit(u);
          squadLookup[u] = (*s1).get();
        }

        fs->units.clear();
        s2 = friendlySquads.erase(s2);
        escape = true;
      } else
        ++s2;
    }
    if (escape)
      break;
  }

  // Enemy squad merging
  escape = false;
  for (auto s1 = enemySquads.begin(); !escape && s1 != enemySquads.end();
       ++s1) {
    for (auto s2 = enemySquads.begin(); !escape && s2 != enemySquads.end();) {
      if (s1 == s2) {
        ++s2;
        continue;
      }
      // Very hacky merge
      if ((*s1)->pos.getApproxDistance((*s2)->pos) < (*s1)->radius1) {
        std::shared_ptr<EnemySquad> fs = *s2; // Hold this squad for a bit
        for (auto &u : fs->units) {
          (*s1)->addUnit(u->u);
          squadLookup[u->u] = (*s1).get();
        }

        fs->units.clear();
        s2 = enemySquads.erase(s2);
        escape = true;
      } else
        ++s2;
    }
    if (escape)
      break;
  }

  if (BWAPI::Broodwar->getFrameCount() % 20)
    return;

  for (auto &es : enemySquads) {
    // Update values for enemy squads
    for (auto &u : es->units) {
      if (u->lastType.isWorker())
        es->attackPriority += 40;
      if (u->lastType.isResourceContainer())
        es->attackPriority += 200;

      if ((UnitManager::reallyHasWeapon(u->lastType) ||
           u->lastType == BWAPI::UnitTypes::Terran_Medic) &&
          !u->lastType.isWorker() && !u->lastType.isBuilding()) {
        es->holdImportance += 100;
      }
    }
  }

  for (auto &fs : friendlySquads) {
    // Engage nearby squads
    fs->engagedSquads.clear();
    for (auto &es : enemySquads)
      if (fs->pos.getApproxDistance(es->pos) <
          es->radius2 + fs->radius2 + 15 * 32)
        fs->engagedSquads.insert(es.get()), es->engagedSquads.insert(fs.get());
  }

  // Try attacking with each squad
  for (auto &fs : friendlySquads) {
    FastAPproximation fap;
    for (auto &u : fs->units)
      fap.addIfCombatUnitPlayer1(u);

    for (auto &es : fs->engagedSquads)
      for (auto &u : ((EnemySquad *)es)->units)
        fap.addIfCombatUnitPlayer2(*u);

    fs->simres.presim.scores = fap.playerScores();
    fs->simres.presim.unitCounts = {(int)fap.getState().first->size(),
                                    (int)fap.getState().second->size()};

    fap.simulate(24 * 4); // Short sim

    fs->simres.shortsim.scores = fap.playerScores();
    fs->simres.shortsim.unitCounts = {(int)fap.getState().first->size(),
                                      (int)fap.getState().second->size()};
    int theirLosses =
        fs->simres.presim.scores.second - fs->simres.shortsim.scores.second;
    int ourLosses =
        fs->simres.presim.scores.first - fs->simres.shortsim.scores.first;
    fs->simres.shortWin = theirLosses - ourLosses;

    fap.simulate(24 * 56); // The rest of the sim

    fs->simres.postsim.scores = fap.playerScores();
    fs->simres.postsim.unitCounts = {(int)fap.getState().first->size(),
                                     (int)fap.getState().second->size()};
  }
  for (auto &fs : friendlySquads) {
    // Simulation and updating of squads done. Let's have some logic for the
    // squads themselves

    BWAPI::Position target = BWAPI::Positions::None;
    int highestPriority;

    for (auto &es : enemySquads) {
      int priority =
          es->holdImportance +
          (es->attackPriority / (int)sqrt(es->pos.getDistance(fs->pos) + 1));

      if (target == BWAPI::Positions::None || priority > highestPriority) {
        target = es->pos;
        highestPriority = priority;
      }
    }

    bool shouldAttack =
        fs->simres.shortWin >= 0 ||
        (fs->simres.postsim.unitCounts.second == 0 &&
         fs->simres.postsim.scores.first >= fs->simres.presim.scores.first / 2);
    int medicCount =
        std::count_if(fs->units.begin(), fs->units.end(), [](auto u) {
          return u->getType() == BWAPI::UnitTypes::Terran_Medic;
        });
    int medicEnergy = 0;
    std::for_each(fs->units.begin(), fs->units.end(), [&](auto u) {
      if (u->getType() == BWAPI::UnitTypes::Terran_Medic)
        medicEnergy += u->getEnergy();
    });

    if (shouldAttack) {
      for (auto &u : fs->units) {
        switch (u->getType()) {
        case BWAPI::UnitTypes::Terran_Medic:
          if (u->getOrder() == BWAPI::Orders::MedicHeal ||
              u->getOrder() == BWAPI::Orders::HealMove)
            continue;
          {
            auto healTarget = u->getClosestUnit(
                BWAPI::Filter::IsOrganic && (BWAPI::Filter::HP_Percent < 100));
            if (healTarget)
              u->attack(healTarget->getPosition());
            else
              u->attack(fs->pos);
          }
          break;
        default:
          if (target != BWAPI::Positions::None) {
            auto enemy = unitManager.getClosestEnemy(u, false);
            if (enemy &&
                enemy->lastPosition.getDistance(u->getPosition()) <
                    enemy->lastType.groundWeapon().maxRange() &&
                !u->isStartingAttack())
              u->move(target);
            else {
              auto detectThis =
                  u->getClosestUnit(!BWAPI::Filter::IsDetected,
                                    u->getType().groundWeapon().maxRange());
              if (detectThis)
                detectionManager.requestDetection(detectThis->getPosition());

              if (auto close = unitManager.getClosestVisibleEnemy(u, true);
                  medicEnergy >= 50 && !u->getStimTimer() &&
                  BWAPI::Broodwar->self()->hasResearched(
                      BWAPI::TechTypes::Stim_Packs) &&
                  close &&
                  close->lastPosition.getDistance(u->getPosition()) <
                      u->getType().groundWeapon().maxRange() &&
                  u->getHitPoints() > 20)
                u->useTech(BWAPI::TechTypes::Stim_Packs);
              u->attack(target);
            }
          }
          break;
        }
      }
    } else if (fs->engagedSquads.size()) {
      // If we don't want to fight the engaged squads, join the closest squad
      int bestDist;
      FriendlySquad *join = nullptr;
      for (auto fs2 : friendlySquads) {
        if (fs == fs2)
          continue;
        int dist = (int)fs2->pos.getDistance(fs->pos);
        if (!join || dist < bestDist) {
          bestDist = dist;
          join = fs2.get();
        }
      }

      if (join)
        target = join->pos;
      else
        target = fs->pos;

      for (auto &u : fs->units) {
        switch (u->getType()) {
        case BWAPI::UnitTypes::Terran_Medic:
          if (u->getOrder() == BWAPI::Orders::MedicHeal ||
              u->getOrder() == BWAPI::Orders::HealMove)
            continue;
          {
            auto healTarget = u->getClosestUnit(
                BWAPI::Filter::GetPlayer == BWAPI::Broodwar->self() &&
                BWAPI::Filter::IsOrganic && BWAPI::Filter::HP_Percent < 100);
            if (healTarget)
              u->attack(healTarget->getPosition());
            else
              u->attack(target);
          }
          break;
        default: {
          auto enemy = unitManager.getClosestEnemy(u, false);
          if (enemy &&
              enemy->lastPosition.getDistance(u->getPosition()) <
                  enemy->lastType.groundWeapon().maxRange() &&
              !u->isStartingAttack())
            u->move(target);
          else {
            auto detectThis =
                u->getClosestUnit(!BWAPI::Filter::IsDetected,
                                  u->getType().groundWeapon().maxRange());
            if (detectThis)
              detectionManager.requestDetection(detectThis->getPosition());

            if (auto close = unitManager.getClosestVisibleEnemy(u, true);
                medicEnergy >= 50 && !u->getStimTimer() &&
                BWAPI::Broodwar->self()->hasResearched(
                    BWAPI::TechTypes::Stim_Packs) &&
                close &&
                close->lastPosition.getDistance(u->getPosition()) <
                    u->getType().groundWeapon().maxRange() &&
                u->getHitPoints() > 20)
              u->useTech(BWAPI::TechTypes::Stim_Packs);
            u->attack(target);
          }
        } break;
        }
      }
    }

    if (target != BWAPI::Positions::None || fs->units.size() < 6) {
      int bestDist;
      FriendlySquad *join = nullptr;
      for (auto fs2 : friendlySquads) {
        if (fs == fs2)
          continue;
        if (fs2->units.size() < fs->units.size())
          continue;
        int dist = (int)fs2->pos.getDistance(fs->pos);
        if (!join || dist < bestDist) {
          bestDist = dist;
          join = fs2.get();
        }
      }

      if (join)
        target = join->pos;
    }

    if (target == BWAPI::Positions::None && !fs->engagedSquads.size()) {
      target = (BWAPI::Position)mapManager.getNextBaseToScout();
    }

    if (target != BWAPI::Positions::None) {
      if (fs->units.size() > 4) {
        for (auto &u : fs->units) {
          u->attack(target);
        }
      } else {
        // Merge
        int bestDist;
        FriendlySquad *join = nullptr;
        for (auto fs2 : friendlySquads) {
          if (fs == fs2)
            continue;
          int dist = (int)fs2->pos.getDistance(fs->pos);
          if (!join || dist < bestDist) {
            bestDist = dist;
            join = fs2.get();
          }
        }

        if (join) {
          for (auto &u : fs->units) {
            u->attack(join->pos);
          }
        }
      }
    }
  }
} // namespace Neolib

void SquadManager::onUnitDestroy(BWAPI::Unit unit) {
  removeFromSquad(unit);
  unmanagedUnits.erase(unit);
}

void SquadManager::onUnitComplete(BWAPI::Unit unit) {
  if (unit->getType() == BWAPI::UnitTypes::Terran_Medic ||
      UnitManager::reallyHasWeapon(unit->getType()) &&
          !unit->getType().isWorker())
    unmanagedUnits.insert(unit);
}

void SquadManager::onUnitRenegade(BWAPI::Unit unit) {
  onUnitDestroy(unit);
  onUnitComplete(unit);
}

void SquadManager::onEnemyRecognize(std::shared_ptr<EnemyData> ed) {
  if (isInSquad(ed->u))
    return;

  for (auto &sq : enemySquads) {
    if (sq->pos.getApproxDistance(ed->lastPosition) <
        sq->radius2) {    // If close enough to join this squad...
      sq->addUnit(ed->u); // ...then join it. As simple as that.
      return;
    }
  }

  // Create its own squad, with blackjack and hookers!
  addToNewSquad(ed->u);
}

void SquadManager::onEnemyLose(std::shared_ptr<EnemyData> ed) {
  removeFromSquad(ed->u);
  unmanagedUnits.erase(ed->u);
}

Squad *SquadManager::getSquad(BWAPI::Unit unit) {
  auto it = squadLookup.find(unit);
  if (it != squadLookup.end())
    return it->second;
  else
    return nullptr;
}

std::shared_ptr<Squad> SquadManager::addToNewSquad(BWAPI::Unit unit) {
  std::shared_ptr<Squad> ret;

  if (unitManager.isOwn(unit)) {
    auto fsq = std::make_shared<FriendlySquad>();
    friendlySquads.push_back(fsq);
    ret = fsq;
  }

  else if (unitManager.isEnemy(unit)) {
    auto esq = std::make_shared<EnemySquad>();
    enemySquads.push_back(esq);
    ret = esq;
  }

  if (ret) {
    unmanagedUnits.erase(unit);
    squadLookup[unit] = ret.get();
    ret->addUnit(unit);
  }

  return ret;
}

std::shared_ptr<Squad> SquadManager::addNewFriendlySquad() {
  auto ret = std::make_shared<FriendlySquad>();
  friendlySquads.push_back(ret);
  return ret;
}

std::shared_ptr<Squad> SquadManager::addNewEnemySquad() {
  auto ret = std::make_shared<EnemySquad>();
  enemySquads.push_back(ret);
  return ret;
}

bool SquadManager::isInSquad(BWAPI::Unit unit) {
  return getSquad(unit) != nullptr;
}

void SquadManager::removeFromSquad(BWAPI::Unit unit) {
  auto it = squadLookup.find(unit);
  if (it != squadLookup.end()) {
    it->second->removeUnit(unit);
    squadLookup.erase(unit);
  }
}

std::vector<std::shared_ptr<EnemySquad>> &SquadManager::getEnemySquads() {
  return enemySquads;
}

std::vector<std::shared_ptr<FriendlySquad>> &SquadManager::getFriendlySquads() {
  return friendlySquads;
}

void SquadManager::removeSquad(FriendlySquad *sq) {
  for (auto it = friendlySquads.begin(); it != friendlySquads.end();)
    if (it->get() == sq)
      it = friendlySquads.erase(it);
    else
      ++it;

  for (auto &s : enemySquads)
    s->squadWasRemoved(sq);

  removeSquadReferences(sq);
}

void SquadManager::removeSquad(EnemySquad *sq) {
  for (auto it = enemySquads.begin(); it != enemySquads.end();)
    if (it->get() == sq)
      it = enemySquads.erase(it);
    else
      ++it;

  for (auto &s : friendlySquads)
    s->squadWasRemoved(sq);

  removeSquadReferences(sq);
}

void SquadManager::removeSquadReferences(Squad *sq) {
  for (auto it = squadLookup.begin();
       it !=
       squadLookup
           .end();) // It shouldn't exist here, but let's be on the safe side.
    if (it->second == sq)
      it = squadLookup.erase(it);
    else
      ++it;
}

void SquadManager::addUnitToSquad(BWAPI::Unit unit, std::shared_ptr<Squad> sq) {
  sq->addUnit(unit);
  squadLookup[unit] = sq.get();
}

} // namespace Neolib
