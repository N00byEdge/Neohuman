#include "ResourceManager.h"

#include "BWAPI.h"

#include "BaseManager.h"
#include "BuildingQueue.h"

Neolib::ResourceManager resourceManager;

namespace Neolib {

ResourceCount::ResourceCount() : minerals(0), gas(0) {}

ResourceCount::ResourceCount(int minerals, int gas)
    : minerals(minerals), gas(gas) {}

ResourceCount::ResourceCount(BWAPI::UnitType ut)
    : minerals(ut.isTwoUnitsInOneEgg() ? ut.mineralPrice() * 2 : ut.mineralPrice()),
      gas(ut.isTwoUnitsInOneEgg() ? ut.gasPrice() * 2 : ut.gasPrice()) {
  if (resourceManager.resourcesReservedForSupply().minerals &&
          ut == BWAPI::UnitTypes::Terran_Supply_Depot ||
      ut == BWAPI::UnitTypes::Protoss_Pylon ||
      ut == BWAPI::UnitTypes::Zerg_Overlord) {
    minerals = 0;
  }
}

ResourceCount::ResourceCount(BWAPI::UpgradeType ut)
    : minerals(ut.mineralPrice()), gas(ut.gasPrice()) {}

ResourceCount::ResourceCount(BWAPI::TechType tt)
    : minerals(tt.mineralPrice()), gas(tt.gasPrice()) {}

ResourceCount ResourceCount::operator+(const ResourceCount &other) const {
  return ResourceCount(minerals + other.minerals, gas + other.gas);
}

ResourceCount &ResourceCount::operator+=(const ResourceCount &other) {
  minerals += other.minerals;
  gas += other.gas;
  return *this;
}

ResourceCount ResourceCount::operator-(const ResourceCount &other) const {
  return ResourceCount(minerals - other.minerals, gas - other.gas);
}

ResourceCount &ResourceCount::operator-=(const ResourceCount &other) {
  minerals -= other.minerals;
  gas -= other.gas;
  return *this;
}

ResourceCount ResourceCount::operator*(const float factor) const {
  return ResourceCount(static_cast<int>(factor * minerals), static_cast<int>(factor * gas));
}

ResourceCount &ResourceCount::operator*=(const float factor) {
  minerals = static_cast<int>(factor * minerals);
  gas = static_cast<int>(factor * gas);
  return *this;
}

ResourceCount ResourceCount::operator/(const float divisor) const {
  return ResourceCount(static_cast<int>(minerals / divisor), static_cast<int>(gas / divisor));
}

ResourceCount &ResourceCount::operator/=(const float divisor) {
  minerals = static_cast<int>(minerals / divisor);
  gas = static_cast<int>(gas / divisor);
  return *this;
}

bool ResourceCount::operator==(const ResourceCount &other) const {
  return minerals == other.minerals && gas == other.gas;
}

bool ResourceCount::operator<(const ResourceCount &other) const {
  return minerals < other.minerals && gas < other.gas;
}

bool ResourceCount::operator<=(const ResourceCount &other) const {
  return minerals <= other.minerals && gas <= other.gas;
}

bool ResourceCount::operator>(const ResourceCount &other) const {
  return minerals > other.minerals && gas > other.gas;
}

bool ResourceCount::operator>=(const ResourceCount &other) const {
  return minerals >= other.minerals && gas >= other.gas;
}

ResourceCount ResourceManager::resourcesReservedForSupply() const {
  ResourceCount rc;
  rc.minerals +=
      100 *
      MAX((int)ceil(supplyManager.wantedAdditionalSupply().terran /
                    BWAPI::Races::Terran.getSupplyProvider().supplyProvided()),
          0);
  rc.minerals +=
      100 *
      MAX((int)ceil(supplyManager.wantedAdditionalSupply().protoss /
                    BWAPI::Races::Protoss.getSupplyProvider().supplyProvided()),
          0);
  rc.minerals +=
      100 *
      MAX((int)ceil(supplyManager.wantedAdditionalSupply().zerg /
                    BWAPI::Races::Zerg.getSupplyProvider().supplyProvided()),
          0);
  return rc;
}

ResourceCount ResourceManager::getMinuteApproxIncome() const {
  ResourceCount sum;
  for (auto &b : baseManager.getAllBases())
    sum += b.calculateIncome();
  return sum;
}

ResourceCount ResourceManager::getSpendableResources() const {
  ResourceCount queued = buildingQueue.getQueuedResources();
  ResourceCount reserved = resourcesReservedForSupply();
  ResourceCount realResources(BWAPI::Broodwar->self()->minerals(),
                              BWAPI::Broodwar->self()->gas());
  return realResources - queued - reserved;
}

bool ResourceManager::canAfford(ResourceCount rc) const {
  auto spendable = getSpendableResources();
  return rc <= spendable;
}
} // namespace Neolib

bool operator<=(Neolib::ResourceCount &lhs, Neolib::ResourceCount &rhs) {
  return lhs.minerals <= rhs.minerals && lhs.gas <= rhs.gas;
}

bool operator>=(Neolib::ResourceCount &lhs, Neolib::ResourceCount &rhs) {
  return lhs.minerals >= rhs.minerals && lhs.gas >= rhs.gas;
}

bool operator==(Neolib::ResourceCount &lhs, Neolib::ResourceCount &rhs) {
  return lhs.minerals == rhs.minerals && lhs.gas == rhs.gas;
}
