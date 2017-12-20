#pragma once

#include <BWAPI.h>

namespace Neolib {
struct ResourceCount {
  ResourceCount();
  ResourceCount(int, int);
  ResourceCount(BWAPI::UnitType);
  ResourceCount(BWAPI::UpgradeType);
  ResourceCount(BWAPI::TechType);

  ResourceCount operator+(const ResourceCount &other) const;
  ResourceCount &operator+=(const ResourceCount &other);
  ResourceCount operator-(const ResourceCount &other) const;
  ResourceCount &operator-=(const ResourceCount &other);
  ResourceCount operator*(const float factor) const;
  ResourceCount &operator*=(const float factor);
  ResourceCount operator/(const float divisor) const;
  ResourceCount &operator/=(const float divisor);

  bool operator==(const ResourceCount &other) const;
  bool operator<(const ResourceCount &other) const;
  bool operator<=(const ResourceCount &other) const;
  bool operator>(const ResourceCount &other) const;
  bool operator>=(const ResourceCount &other) const;

  int minerals, gas;
};

struct ResourceManager {
  ResourceCount getSpendableResources() const;
  ResourceCount resourcesReservedForSupply() const;

  ResourceCount getMinuteApproxIncome() const;

  bool canAfford(ResourceCount) const;
};

} // namespace Neolib

bool operator<=(Neolib::ResourceCount &lhs, Neolib::ResourceCount &rhs);
bool operator>=(Neolib::ResourceCount &lhs, Neolib::ResourceCount &rhs);
bool operator==(Neolib::ResourceCount &lhs, Neolib::ResourceCount &rhs);

extern Neolib::ResourceManager resourceManager;
