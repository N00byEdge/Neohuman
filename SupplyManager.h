#pragma once

#include "BWAPI.h"

namespace Neolib {

struct SupplyCount {
  SupplyCount();
  SupplyCount(int protoss, int terran, int zerg);

  SupplyCount operator+(const SupplyCount &other) const;
  SupplyCount &operator+=(const SupplyCount &other);
  SupplyCount operator-(const SupplyCount &other) const;
  SupplyCount &operator-=(const SupplyCount &other);
  SupplyCount operator*(const int factor) const;
  SupplyCount &operator*=(const int factor);
  SupplyCount operator/(const int divisor) const;
  SupplyCount &operator/=(const int divisor);

  int operator()(BWAPI::Race race = BWAPI::Races::None) const;

  int protoss, terran, zerg;
};

struct SupplyManager {
  SupplyCount usedSupply();
  SupplyCount availableSupply();
  SupplyCount wantedSupplyOverhead();
  SupplyCount wantedAdditionalSupply();
};

int mainSupply(SupplyCount);

} // namespace Neolib

extern Neolib::SupplyManager supplyManager;
