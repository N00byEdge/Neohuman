#include "UnitManager.h"

#include "BuildingQueue.h"
#include "SoundDatabase.h"
#include "BaseManager.h"
#include "SquadManager.h"
#include "Util.h"

#include <array>

std::set<std::shared_ptr<Neolib::EnemyData>> emptyEnemyDataSet;
std::set<BWAPI::Unit> emptyUnitset;

#define PROTOSSSHEILDREGEN 7
#define ZERGREGEN 4

Neolib::UnitManager unitManager;

std::array<int, Neolib::Pow<2>(256 * 4)> DeathMatrixGround, DeathMatrixAir;

namespace Neolib {

EnemyData::EnemyData() = default;

  EnemyData::EnemyData(BWAPI::Unit unit) : u(unit) {
  if (unit)
    initFromUnit();
}

EnemyData::EnemyData(BWAPI::UnitType ut)
    : u(nullptr), lastType(ut), lastPlayer(BWAPI::Broodwar->self()),
      lastPosition({}), lastHealth(ut.maxHitPoints()),
      lastShields(ut.maxShields()) {}

void EnemyData::initFromUnit() const {
  lastType = u->getType();
  lastPlayer = u->getPlayer();
  unitID = u->getID();
  updateFromUnit(u);
}

void EnemyData::updateFromUnit() const { updateFromUnit(u); }

void EnemyData::updateFromUnit(const BWAPI::Unit unit) const {
  frameLastSeen = BWAPI::Broodwar->getFrameCount();
  lastPosition = unit->getPosition();
  if (!unit->isDetected())
    return;
  frameLastDetected = BWAPI::Broodwar->getFrameCount();
  lastHealth = unit->getHitPoints();
  lastShields = unit->getShields();
  isCompleted = unit->isCompleted();
}

int EnemyData::expectedHealth() const {
  if (lastType.getRace() == BWAPI::Races::Zerg)
    return MIN(
        ((BWAPI::Broodwar->getFrameCount() - frameLastDetected) * ZERGREGEN) /
                256 +
            lastHealth,
        lastType.maxHitPoints());
  return lastHealth;
}

int EnemyData::expectedShields() const {
  if (lastType.getRace() == BWAPI::Races::Protoss)
    return MIN(((BWAPI::Broodwar->getFrameCount() - frameLastDetected) *
                PROTOSSSHEILDREGEN) /
                       256 +
                   lastShields,
               lastType.maxShields());
  return lastShields;
}

bool EnemyData::isFriendly() const {
  return lastPlayer == BWAPI::Broodwar->self();
}

bool EnemyData::isEnemy() const { return !isFriendly(); }

bool EnemyData::operator<(const EnemyData &other) const { return u < other.u; }

bool EnemyData::operator==(const EnemyData &other) const {
  return u == other.u;
}

std::size_t EnemyData::hash::operator()(const EnemyData &ed) const {
  return std::hash<BWAPI::Unit>()(ed.u);
}

int UnitManager::countUnit(BWAPI::UnitType const t, const BWAPI::UnitFilter &filter, bool const countQueued) {
  if (BWAPI::Broodwar->getAllUnits().empty())
    return 0;

  auto count = 0;

  for (auto u : BWAPI::Broodwar->getAllUnits())
    if (t == u->getType() && (filter)(u))
      ++count;

  if (countQueued)
    for (auto &o : buildingQueue.buildingsQueued())
      if (o.buildingType == t)
        ++count;

  return count;
}

int UnitManager::countFriendly(BWAPI::UnitType const t, bool const onlyWithWeapons, bool const countQueued) const {
  auto sum = 0;

  if (t == BWAPI::UnitTypes::AllUnits) {
    if (onlyWithWeapons) {
      for (auto &[type, units] : friendlyUnitsByType)
        if (reallyHasWeapon(type))
          sum += static_cast<int>(units.size());
    }
    else {
      for (auto &ut : friendlyUnitsByType)
        sum += static_cast<int>(ut.second.size());
    }
  }

  else if (friendlyUnitsByType.count(t))
    sum = static_cast<int>(friendlyUnitsByType.at(t).size());

  if (countQueued) {
    if (t == BWAPI::UnitTypes::AllUnits)
      sum += static_cast<int>(buildingQueue.buildingsQueued().size());
    else
      for (auto &bo : buildingQueue.buildingsQueued())
        if (bo.buildingType == t)
          ++sum;
  }

  return sum;
}

int UnitManager::countEnemy(BWAPI::UnitType const t, bool onlyWithWeapons) const {
  auto sum = 0;

  if (t == BWAPI::UnitTypes::AllUnits) {
    if (onlyWithWeapons) {
      for (auto &[type, units] : enemyUnitsByType)
        if (reallyHasWeapon(type))
          sum += static_cast<int>(units.size());
    }
    else {
      for (auto &ut : enemyUnitsByType)
        sum += static_cast<int>(ut.second.size());
    }
  }

  else
    sum = static_cast<int>(enemyUnitsByType.at(t).size());

  return sum;
}

bool UnitManager::isAllowedToLockdown(BWAPI::Unit const target, BWAPI::Unit const own) const {
  auto const it = lockdownDB.find(target);
  return it == lockdownDB.end() || it->second.first == own;
}

void UnitManager::reserveLockdown(BWAPI::Unit const target, BWAPI::Unit own) {
  lockdownDB[target] = {own, BWAPI::Broodwar->getFrameCount()};
}

unsigned UnitManager::getNumArmedSilos() const {
  unsigned num = 0;

  for (auto &ns : getFriendlyUnitsByType(BWAPI::UnitTypes::Terran_Nuclear_Silo))
    if (ns->hasNuke())
      ++num;

  return num;
}

unsigned UnitManager::getNumArmingSilos() const {
  unsigned num = 0;

  for (auto &ns : getFriendlyUnitsByType(BWAPI::UnitTypes::Terran_Nuclear_Silo))
    if (!ns->isIdle() && !ns->hasNuke())
      ++num;

  return num;
}

unsigned UnitManager::getNumUnarmedSilos() const {
  unsigned num = 0;

  for (auto &ns : getFriendlyUnitsByType(BWAPI::UnitTypes::Terran_Nuclear_Silo))
    if (ns->isIdle() && !ns->hasNuke())
      ++num;

  return num;
}

const std::map<BWAPI::UnitType, std::set<std::shared_ptr<EnemyData>>> &
UnitManager::getEnemyUnitsByType() const {
  return enemyUnitsByType;
}

const std::set<std::shared_ptr<EnemyData>> &
UnitManager::getEnemyUnitsByType(BWAPI::UnitType const ut) const {
  auto const res = enemyUnitsByType.find(ut);
  if (res != enemyUnitsByType.end()) {
    return res->second;
  }
  return emptyEnemyDataSet;
}

const std::map<BWAPI::UnitType, std::set<BWAPI::Unit>> &
UnitManager::getFriendlyUnitsByType() const {
  return friendlyUnitsByType;
}

const std::set<BWAPI::Unit> &
UnitManager::getFriendlyUnitsByType(BWAPI::UnitType const ut) const {
  auto const res = friendlyUnitsByType.find(ut);
  if (res != friendlyUnitsByType.end()) {
    return res->second;
  }
  return emptyUnitset;
}

const std::set<std::shared_ptr<EnemyData>> &
UnitManager::getNonVisibleEnemies() const {
  return nonVisibleEnemies;
}

const std::set<std::shared_ptr<EnemyData>> &
UnitManager::getVisibleEnemies() const {
  return visibleEnemies;
}

const std::set<std::shared_ptr<EnemyData>> &
UnitManager::getInvalidatedEnemies() const {
  return invalidatedEnemies;
}

const std::set<std::shared_ptr<EnemyData>> &
UnitManager::getDeadEnemies() const {
  return deadEnemies;
}

std::shared_ptr<EnemyData>
UnitManager::getClosestEnemy(BWAPI::Unit from, const BWAPI::UnitFilter &f,
                             bool onlyWithWeapons) const {
  std::shared_ptr<EnemyData> closest = nullptr;
  int dist;

  if (onlyWithWeapons) {
    for (auto &u : visibleEnemies)
      if (reallyHasWeapon(u->lastType)) {
        if (!(f)(u->u) || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
          continue;
        if (!closest) {
          closest = u;
          dist = from->getPosition().getApproxDistance(u->lastPosition);
          continue;
        }
        auto const thisDist = from->getPosition().getApproxDistance(u->lastPosition);
        if (thisDist < dist) {
          closest = u;
          dist = thisDist;
        }
      }
    for (auto &u : nonVisibleEnemies)
      if (reallyHasWeapon(u->lastType)) {
        if (!(f)(u->u) || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
          continue;
        if (!closest) {
          closest = u;
          dist = from->getPosition().getApproxDistance(u->lastPosition);
          continue;
        }
        auto const thisDist = from->getPosition().getApproxDistance(u->lastPosition);
        if (thisDist < dist) {
          closest = u;
          dist = thisDist;
        }
      }
  } else {
    for (auto &u : visibleEnemies) {
      if (!(f)(u->u) || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
        continue;
      if (!closest) {
        closest = u;
        dist = from->getPosition().getApproxDistance(u->lastPosition);
        continue;
      }
      auto const thisDist = from->getPosition().getApproxDistance(u->lastPosition);
      if (thisDist < dist) {
        closest = u;
        dist = thisDist;
      }
    }
    for (auto &u : unitManager.nonVisibleEnemies) {
      if (!(f)(u->u) || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
        continue;
      if (!closest) {
        closest = u;
        dist = from->getPosition().getApproxDistance(u->lastPosition);
        continue;
      }
      auto const thisDist = from->getPosition().getApproxDistance(u->lastPosition);
      if (thisDist < dist) {
        closest = u;
        dist = thisDist;
      }
    }
  }

  return closest;
}

std::shared_ptr<EnemyData>
UnitManager::getClosestEnemy(BWAPI::Unit const from, bool const onlyWithWeapons) const {
  std::shared_ptr<EnemyData> closest = nullptr;
  int dist;

  if (onlyWithWeapons) {
    for (auto &u : visibleEnemies)
      if (reallyHasWeapon(u->lastType) && u->frameLastSeen + 1000 >= BWAPI::Broodwar->getFrameCount()) {
        if (!closest) {
          closest = u;
          dist = from->getPosition().getApproxDistance(u->lastPosition);
          continue;
        }
        auto const thisDist = from->getPosition().getApproxDistance(u->lastPosition);
        if (thisDist < dist) {
          closest = u;
          dist = thisDist;
        }
      }
    for (auto &u : nonVisibleEnemies)
      if (reallyHasWeapon(u->lastType) && u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount()) {
        if (!closest) {
          closest = u;
          dist = from->getPosition().getApproxDistance(u->lastPosition);
          continue;
        }
        auto const thisDist = from->getPosition().getApproxDistance(u->lastPosition);
        if (thisDist < dist) {
          closest = u;
          dist = thisDist;
        }
      }
  } else {
    for (auto &u : visibleEnemies) {
      if (u->positionInvalidated || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
        continue;
      if (!closest) {
        closest = u;
        dist = from->getPosition().getApproxDistance(u->lastPosition);
        continue;
      }
      auto const thisDist = from->getPosition().getApproxDistance(u->lastPosition);
      if (thisDist < dist) {
        closest = u;
        dist = thisDist;
      }
    }
    for (auto &u : nonVisibleEnemies) {
      if (u->positionInvalidated || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
        continue;
      if (!closest) {
        closest = u;
        dist = from->getPosition().getApproxDistance(u->lastPosition);
        continue;
      }
      auto const thisDist = from->getPosition().getApproxDistance(u->lastPosition);
      if (thisDist < dist) {
        closest = u;
        dist = thisDist;
      }
    }
  }

  return closest;
}

std::shared_ptr<EnemyData> UnitManager::getClosestVisibleEnemy(BWAPI::Unit from, const BWAPI::UnitFilter &f, bool onlyWithWeapons) const {
  std::shared_ptr<EnemyData> closest = nullptr;
  int dist;

  if (onlyWithWeapons) {
    for (auto &u : visibleEnemies)
      if (reallyHasWeapon(u->lastType)) {
        if (!(f)(u->u) || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
          continue;
        if (!closest) {
          closest = u;
          dist = from->getPosition().getApproxDistance(u->lastPosition);
          continue;
        }
        auto const thisDist = from->getPosition().getApproxDistance(u->lastPosition);
        if (thisDist < dist) {
          closest = u;
          dist = thisDist;
        }
      }
  } else {
    for (auto &u : visibleEnemies) {
      if (!(f)(u->u) || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
        continue;
      if (!closest) {
        closest = u;
        dist = from->getPosition().getApproxDistance(u->lastPosition);
        continue;
      }
      auto const thisDist = from->getPosition().getApproxDistance(u->lastPosition);
      if (thisDist < dist) {
        closest = u;
        dist = thisDist;
      }
    }
  }

  return closest;
}

std::shared_ptr<EnemyData>
UnitManager::getClosestVisibleEnemy(BWAPI::Unit const from, bool const onlyWithWeapons) const {
  std::shared_ptr<EnemyData> closest = nullptr;
  int dist;

  if (onlyWithWeapons) {
    for (auto &u : visibleEnemies)
      if (reallyHasWeapon(u->lastType) &&
          u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount()) {
        if (!closest) {
          closest = u;
          dist = from->getPosition().getApproxDistance(u->lastPosition);
          continue;
        }
        auto const thisDist = from->getPosition().getApproxDistance(u->lastPosition);
        if (thisDist < dist) {
          closest = u;
          dist = thisDist;
        }
      }
  } else {
    for (auto &u : visibleEnemies) {
      if (u->positionInvalidated || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
        continue;
      if (!closest) {
        closest = u;
        dist = from->getPosition().getApproxDistance(u->lastPosition);
        continue;
      }
      auto const thisDist = from->getPosition().getApproxDistance(u->lastPosition);
      if (thisDist < dist) {
        closest = u;
        dist = thisDist;
      }
    }
  }

  return closest;
}

std::shared_ptr<EnemyData> UnitManager::getClosestNonVisibleEnemy(BWAPI::Unit from, const BWAPI::UnitFilter &f, bool onlyWithWeapons) const {
  std::shared_ptr<EnemyData> closest = nullptr;
  int dist;

  if (onlyWithWeapons) {
    for (auto &u : nonVisibleEnemies)
      if (reallyHasWeapon(u->lastType)) {
        if (!(f)(u->u) || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
          continue;
        if (!closest) {
          closest = u;
          dist = from->getPosition().getApproxDistance(u->lastPosition);
          continue;
        }
        auto const thisDist = from->getPosition().getApproxDistance(u->lastPosition);
        if (thisDist < dist) {
          closest = u;
          dist = thisDist;
        }
      }
  } else {
    for (auto &u : nonVisibleEnemies) {
      if (!(f)(u->u) || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
        continue;
      if (!closest) {
        closest = u;
        dist = from->getPosition().getApproxDistance(u->lastPosition);
        continue;
      }
      auto const thisDist = from->getPosition().getApproxDistance(u->lastPosition);
      if (thisDist < dist) {
        closest = u;
        dist = thisDist;
      }
    }
  }

  return closest;
}

std::shared_ptr<EnemyData>
UnitManager::getClosestNonVisibleEnemy(BWAPI::Unit const from, bool const onlyWithWeapons) const {
  std::shared_ptr<EnemyData> closest = nullptr;
  int dist;

  if (onlyWithWeapons) {
    for (auto &u : nonVisibleEnemies)
      if (reallyHasWeapon(u->lastType) && u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount()) {
        if (!closest) {
          closest = u;
          dist = from->getPosition().getApproxDistance(u->lastPosition);
          continue;
        }
        auto const thisDist = from->getPosition().getApproxDistance(u->lastPosition);
        if (thisDist < dist) {
          closest = u;
          dist = thisDist;
        }
      }
  } else {
    for (auto &u : nonVisibleEnemies) {
      if (u->positionInvalidated || u->frameLastSeen + 1000 <= BWAPI::Broodwar->getFrameCount())
        continue;
      if (!closest) {
        closest = u;
        dist = from->getPosition().getApproxDistance(u->lastPosition);
        continue;
      }
      auto const thisDist = from->getPosition().getApproxDistance(u->lastPosition);
      if (thisDist < dist) {
        closest = u;
        dist = thisDist;
      }
    }
  }

  return closest;
}

std::shared_ptr<EnemyData> UnitManager::getEnemyData(BWAPI::Unit const ptr) {
  auto const it = enemyUnits.find(ptr->getID());
  if (it == enemyUnits.end())
    return nullptr;
  else
    return it->second;
}

std::shared_ptr<EnemyData> UnitManager::getBestTarget(BWAPI::Unit const from) {
  int bestVal;
  std::shared_ptr<EnemyData> ret;

  for (auto &u : visibleEnemies) {
    auto const val = targetPriority(from, u.get());
    if (!ret || val > bestVal) {
      bestVal = val;
      ret = u;
    }
  }

  for (auto &u : nonVisibleEnemies) {
    auto const val = targetPriority(from, u.get());
    if (!ret || val > bestVal) {
      bestVal = val;
      ret = u;
    }
  }

  return ret;
}

int UnitManager::getNukeScore(BWAPI::Position const pos, BWAPI::Unit from) const {
  int sum = 0;

  for (auto u : visibleEnemies)
    if (u->lastPosition.getDistance(pos) < 10 * 32)
      sum += u->lastType.destroyScore();

  for (auto u : nonVisibleEnemies)
    if (u->lastPosition.getDistance(pos) < 10 * 32)
      sum += u->lastType.destroyScore();

  for (auto u : BWAPI::Broodwar->self()->getUnits())
    if (u->getType().isBuilding() &&
        u->getPosition().getDistance(pos) < 10 * 32)
      sum -= u->getType().destroyScore();

  if (from)
    sum -= from->getDistance(pos);

  return sum;
}

BWAPI::Position UnitManager::getBestNuke(BWAPI::Unit from) const {
  BWAPI::Position pos = BWAPI::Positions::None;
  int bestScore;

  for (auto possibleTarget : visibleEnemies) {
    int score = getNukeScore(possibleTarget->lastPosition, from);

    for (auto &nd : BWAPI::Broodwar->getNukeDots())
      if (nd.getApproxDistance(possibleTarget->lastPosition) < 32 * 10)
        goto skipUnit;

    if (pos == BWAPI::Positions::None || score > bestScore) {
      pos = possibleTarget->lastPosition;
      bestScore = score;
    }

  skipUnit:;
  }

  if (pos == BWAPI::Positions::None ||
      getNukeScore(pos, nullptr) <
          2000) // Compare absolute score, no ghost specific
    return BWAPI::Positions::None;
  else
    return pos;
}

int UnitManager::deathPerHealth(EnemyData *ed, bool flyingTarget) {
  int death =
      flyingTarget ? unitDeathAir(ed->lastType) : unitDeathGround(ed->lastType);
  if (!death || ed->expectedHealth() + ed->expectedShields() +
                        ed->u->getDefenseMatrixPoints() ==
                    0)
    return 0;
  return (death * 100) / (ed->expectedHealth() + ed->expectedShields());
}

int UnitManager::targetPriority(BWAPI::Unit f, EnemyData *ed) {
  if (ed->lastType.isInvincible())
    return -100000000;

  int val = 0;

  if (ed->lastType == BWAPI::UnitTypes::Zerg_Egg ||
      BWAPI::UnitTypes::Zerg_Larva)
    val -= 100;

  if (reallyHasWeapon(ed->lastType))
    val += 1000;

  if (ed->lastType == BWAPI::UnitTypes::Terran_Medic) {
    if (ed->lastHealth == 0)
      val += 1010;
    else
      val += 1010 + (unitDeathAir(BWAPI::UnitTypes::Terran_Marine) * 100) /
                        ed->lastHealth;
  }

  if ((ed->lastType.canProduce() && !ed->lastType.requiresPsi()) ||
      ed->lastType == BWAPI::UnitTypes::Protoss_Pylon)
    val += 980;

  val += deathPerHealth(ed, f->isFlying());
  val -= ed->lastPosition.getApproxDistance(f->getPosition()) * 10;

  if (!ed->lastType.canProduce() && !reallyHasWeapon(ed->lastType) &&
      isOnFire(*ed))
    val -= 250;

  return val;
}

bool UnitManager::isEnemy(BWAPI::Unit u) {
  return BWAPI::Broodwar->self()->isEnemy(u->getPlayer());
}

bool UnitManager::isOwn(BWAPI::Unit u) {
  return u->getPlayer() == BWAPI::Broodwar->self();
}

bool UnitManager::isNeutral(BWAPI::Unit u) { return !isEnemy(u) && !isOwn(u); }

bool UnitManager::reallyHasWeapon(const BWAPI::UnitType &ut) {
  return ut.groundWeapon().damageAmount() ||
         ut == BWAPI::UnitTypes::Terran_Bunker ||
         ut == BWAPI::UnitTypes::Protoss_High_Templar ||
         ut == BWAPI::UnitTypes::Protoss_Carrier ||
         ut == BWAPI::UnitTypes::Protoss_Reaver;
}

inline bool UnitManager::isOnFire(EnemyData building) {
  return building.lastType.isBuilding() && building.isCompleted &&
         building.lastType.getRace() == BWAPI::Races::Terran &&
         building.lastHealth < building.lastType.maxHitPoints() / 3;
}

inline int UnitManager::unitDeathGround(BWAPI::UnitType ut) {
  if (ut.groundWeapon() == BWAPI::WeaponTypes::None)
    return 0;
  return ut.groundWeapon().damageAmount() * ut.groundWeapon().damageFactor() *
         40 / ut.groundWeapon().damageCooldown();
}

inline int UnitManager::unitDeathAir(BWAPI::UnitType ut) {
  if (ut.airWeapon() == BWAPI::WeaponTypes::None)
    return 0;
  return ut.airWeapon().damageAmount() * ut.airWeapon().damageFactor() * 40 /
         ut.airWeapon().damageCooldown();
}

inline int UnitManager::unitDeath(BWAPI::UnitType ut) {
  return (unitDeathAir(ut) + unitDeathGround(ut)) / 2;
}

inline int UnitManager::deathPerHealth(BWAPI::UnitType ut, int health) {
  return unitDeath(ut) / health;
}

inline int UnitManager::deathPerHealth(BWAPI::Unit unit) {
  return deathPerHealth(unit->getType(), unit->getHitPoints());
}

/*inline void UnitManager::addToDeathMatrix(BWAPI::Position pos, BWAPI::UnitType ut, BWAPI::Player p) {
  const int mapH = BWAPI::Broodwar->mapHeight() * 4,
            mapW = BWAPI::Broodwar->mapWidth()  * 4;
  if (ut.groundWeapon()) {
    const auto range = p->weaponMaxRange(ut.groundWeapon()) / 8;
    const auto death = unitDeathGround(ut);
    const auto mx = pos.x + range > mapW ? mapW : pos.x + range;
    for (auto dx = pos.x - range < 0 ? -pos.x : -range; dx <= mx; ++dx) {
      const auto yw = static_cast<int>(ceil(sqrt(range * range - dx * dx)));
      const auto minY = MAX(pos.y - yw, 0), maxY = MIN(pos.y + yw, mapH);
      for (auto y = minY; y <= maxY; ++y)
        DeathMatrixGround[y * deathMatrixSideLen + pos.x + dx] += death;
    }
  }

  if (ut.airWeapon()) {
    const int range = p->weaponMaxRange(ut.groundWeapon()) / 8;
    const int death = unitDeathAir(ut);
    const int mx = pos.x + range > mapW ? mapW : pos.x + range;
    for (int dx = pos.x - range < 0 ? -pos.x : -range; dx <= mx; ++dx) {
      const int yw = (int)ceil(sqrt(range * range - dx * dx));
      const int minY = MAX(pos.y - yw, 0), maxY = MIN(pos.y + yw, mapH);
      for (int y = minY; y <= maxY; ++y)
        DeathMatrixAir[y * deathMatrixSideLen + pos.x + dx] += death;
    }
  }
}*/

SimResults UnitManager::getSimResults() const { return sr; }

unsigned UnitManager::getLaunchedNukeCount() const { return launchedNukeCount; }

int UnitManager::getLastAttackFrame(BWAPI::Unit u) {
  return lastAttackFrame[u];
}

// void UnitManager::attacked(BWAPI::Unit u) {
//	lastAttackFrame[u] = BWAPI::Broodwar->getFrameCount();
//}

int UnitManager::getMinStop(BWAPI::UnitType ut) {
  switch (ut) {
  case BWAPI::UnitTypes::Protoss_Dragoon:
    return 5;
  case BWAPI::UnitTypes::Zerg_Devourer:
    return 7;
  default:
    return 0;
  }
}

void UnitManager::onFrame() {
  // Update visible enemies
  for (auto &u : visibleEnemies)
    u->updateFromUnit();

  // Invalidate positions
  for (auto it = nonVisibleEnemies.begin(); it != nonVisibleEnemies.end();) {
    if (BWAPI::Broodwar->isVisible(BWAPI::TilePosition((*it)->lastPosition))) {
      (*it)->positionInvalidated = true;
      invalidatedEnemies.insert(*it);
      squadManager.onEnemyLose(*it);
      it = nonVisibleEnemies.erase(it);
    } else
      ++it;
  }

  for (auto it = lockdownDB.begin(); it != lockdownDB.end();)
    if (it->second.second > BWAPI::Broodwar->getFrameCount() + 24 * 3)
      it = lockdownDB.erase(it);
    else
      ++it;

  doMultikillDetector();
  doCombatSim();

  for (auto u : BWAPI::Broodwar->getAllUnits()) {
    if (!lastAttacking[u] && u->isStartingAttack()) {
      lastAttackFrame[u] = BWAPI::Broodwar->getFrameCount();
    }

    lastAttacking[u] = u->isStartingAttack();
  }
}

void UnitManager::onNukeDetect(BWAPI::Position target) {
  ++launchedNukeCount;

  BWAPI::Broodwar->sendText(
      "You may want to know that there is a nuke coming for you at %d %d",
      target.x, target.y);
  BWAPI::Broodwar->sendText("Nuke score: %d", getNukeScore(target, nullptr));
}

void UnitManager::onUnitDiscover(BWAPI::Unit unit) {
  if (isOwn(unit) && unit->getType().isResourceDepot()) {
    if (supplyManager.usedSupply()(BWAPI::Races::Terran) > 10)
      buildingQueue.doBuild(BWAPI::UnitTypes::Terran_Bunker,
                            (BWAPI::TilePosition)unit->getPosition());
  }

  if (isEnemy(unit)) {
    auto it = enemyUnits.find(unit->getID());

    if (it == enemyUnits.end()) {
      auto ed = std::make_shared<EnemyData>(unit);
      enemyUnits.insert({unit->getID(), ed});
      enemyUnitsByType[unit->getType()].insert(ed);
      visibleEnemies.insert(ed);
      squadManager.onEnemyRecognize(ed);
    }

    else {
      if (it->second->lastPlayer != unit->getPlayer())
        onUnitRenegade(unit);

      if (it->second->lastType != unit->getType())
        onUnitMorph(unit);

      it->second->updateFromUnit();
      squadManager.onEnemyRecognize(it->second);
    }
  } else if (isOwn(unit))
    friendlyUnitsByType[unit->getType()].insert(unit);
}

void UnitManager::onUnitShow(BWAPI::Unit unit) {
  auto ptr = enemyUnits.find(unit->getID());
  if (ptr != enemyUnits.end()) {
    nonVisibleEnemies.erase(ptr->second);
    invalidatedEnemies.erase(ptr->second);

    squadManager.onEnemyRecognize(ptr->second);

    visibleEnemies.insert(ptr->second);
  }
}

void UnitManager::onUnitHide(BWAPI::Unit unit) {
  auto ptr = enemyUnits.find(unit->getID());
  if (ptr != enemyUnits.end()) {
    visibleEnemies.erase(ptr->second);

    nonVisibleEnemies.insert(ptr->second);
  }
}

void UnitManager::onUnitCreate(BWAPI::Unit unit) {
  if (unit->getType() == BWAPI::UnitTypes::Terran_Ghost)
    soundDatabase.playRandomFunnySound();
}

void UnitManager::onUnitDestroy(BWAPI::Unit unit) {
  if (isEnemy(unit) && unit->getType().isResourceDepot())
    soundDatabase.playRandomHappySound();
  if (isOwn(unit) && unit->getType().isResourceDepot())
    soundDatabase.playRandomSadSound();

  if (isEnemy(unit)) {
    auto ed = getEnemyData(unit);
    squadManager.onEnemyLose(ed);
    enemyUnitsByType[unit->getType()].erase(ed);
    visibleEnemies.erase(ed);
    deadEnemies.insert(ed);

    ++multikillDetector[unit->getType()];
  }

  else if (isOwn(unit)) {
    friendlyUnitsByType[unit->getType()].erase(unit);

    for (auto it = lockdownDB.begin(); it != lockdownDB.end();) {
      if (it->second.first == unit)
        it = lockdownDB.erase(it);
      else
        ++it;
    }
  }
}

void UnitManager::onUnitMorph(BWAPI::Unit unit) {
  if (isEnemy(unit)) {
    auto it = enemyUnits.find(unit->getID());
    if (it == enemyUnits.end()) {
      // Unit morphed and we didn't know about it before... Wtf?
      return;
    } else {
      enemyUnitsByType[it->second->lastType].erase(it->second);
    }

    it->second->lastType = unit->getType();
    enemyUnitsByType[it->second->lastType].insert(it->second);
  }
}

void UnitManager::onUnitEvade(BWAPI::Unit unit) {}

void UnitManager::onUnitRenegade(BWAPI::Unit unit) {
  if (isEnemy(unit)) {
    friendlyUnitsByType[unit->getType()].erase(unit);
    if (!friendlyUnitsByType[unit->getType()].size())
      friendlyUnitsByType.erase(unit->getType());
    onUnitDiscover(unit);
  } else {
    auto it = enemyUnits.find(unit->getID());
    if (it != enemyUnits.end()) {
      squadManager.onEnemyLose(it->second);
      visibleEnemies.erase(it->second);
      enemyUnits.erase(it);
    } else {
      friendlyUnitsByType[unit->getType()].erase(unit);
      if (!friendlyUnitsByType[unit->getType()].size())
        friendlyUnitsByType.erase(unit->getType());
      onUnitDiscover(unit);
    }
  }
}

void UnitManager::onUnitComplete(BWAPI::Unit unit) {
  if (unit->getPlayer() == BWAPI::Broodwar->self()) {
    friendlyUnitsByType[unit->getType()].insert(unit);
  }

  if (isOwn(unit) && unit->getType() == BWAPI::UnitTypes::Terran_Barracks) {
    buildingQueue.doBuild(BWAPI::UnitTypes::Terran_Bunker,
                          (BWAPI::TilePosition)unit->getPosition());
  }

  if (isOwn(unit) &&
      unit->getType() == BWAPI::UnitTypes::Terran_Command_Center &&
      unitManager.countUnit(BWAPI::UnitTypes::Terran_Command_Center,
                            BWAPI::Filter::Exists) > 1) {
    buildingQueue.doBuild(BWAPI::UnitTypes::Terran_Bunker,
                          (BWAPI::TilePosition)unit->getPosition());
  }
}

void UnitManager::doCombatSim() {
  /*fap = FastAPproximation();

  for (auto &u : getFriendlyUnitsByType(BWAPI::UnitTypes::Terran_Marine))
    fap.addIfCombatUnitPlayer1(u);

  for (auto &u : getFriendlyUnitsByType(BWAPI::UnitTypes::Terran_Ghost))
    fap.addIfCombatUnitPlayer1(u);

  for (auto &u : getVisibleEnemies())
    if (!(u->lastType.isWorker() && (u && u->u->isAttacking())))
      fap.addIfCombatUnitPlayer2(*u);

  for (auto &u : getNonVisibleEnemies())
    if (!(u->lastType.isWorker() && (u && u->u->isAttacking())))
      fap.addIfCombatUnitPlayer2(*u);

  sr.presim.scores = fap.playerScores();
  sr.presim.unitCounts = {(int)fap.getState().first->size(),
                          (int)fap.getState().second->size()};

  fap.simulate(24 * 3); // 3 seconds of combat

  sr.shortsim.scores = fap.playerScores();
  sr.shortsim.unitCounts = {(int)fap.getState().first->size(),
                            (int)fap.getState().second->size()};
  int theirLosses = sr.presim.scores.second - sr.shortsim.scores.second;
  int ourLosses = sr.presim.scores.first - sr.shortsim.scores.first;
  sr.shortWin = theirLosses - ourLosses;

  fap.simulate(24 * 60); // 60 seconds of combat

  sr.postsim.scores = fap.playerScores();
  sr.postsim.unitCounts = {(int)fap.getState().first->size(),
                           (int)fap.getState().second->size()};*/
}

void UnitManager::doMultikillDetector() {
  int unitsKilledThisFrame = 0;
  for (auto &ut : multikillDetector)
    unitsKilledThisFrame += ut.second;

  if (unitsKilledThisFrame > 3) {
    soundDatabase.playRandomKillSound();
    BWAPI::Broodwar->sendText("%cMULTIKILL! %d Units killed!",
                              BWAPI::Text::Yellow, unitsKilledThisFrame);
    for (auto &ut : multikillDetector)
      BWAPI::Broodwar->sendText("    %c%s: %d", BWAPI::Text::Yellow,
                                noRaceName(ut.first.c_str()), ut.second);
  }

  multikillDetector.clear();
}

} // namespace Neolib
