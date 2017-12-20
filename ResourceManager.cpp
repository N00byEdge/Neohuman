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
    : minerals(ut == BWAPI::UnitTypes::Zerg_Zergling ? 50 : ut.mineralPrice()),
      gas(ut.gasPrice()) {
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
  return ResourceCount((int)(factor * minerals), (int)(factor * gas));
}

ResourceCount &ResourceCount::operator*=(const float factor) {
  minerals = (int)(factor * minerals);
  gas = (int)(factor * gas);
  return *this;
}

ResourceCount ResourceCount::operator/(const float divisor) const {
  return ResourceCount((int)((float)minerals / divisor),
                       (int)((float)gas / divisor));
}

ResourceCount &ResourceCount::operator/=(const float divisor) {
  minerals = (int)((float)minerals / divisor);
  gas = (int)((float)gas / divisor);
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
      MAX((int)ceil(supplyManager.wantedAdditionalSupply().terran / 16), 0);
  rc.minerals +=
      100 *
      MAX((int)ceil(supplyManager.wantedAdditionalSupply().protoss / 16), 0);
  rc.minerals +=
      100 * MAX((int)ceil(supplyManager.wantedAdditionalSupply().zerg / 16), 0);
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
