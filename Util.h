#pragma once

#include <utility>
#include <vector>

#include "Neohuman.h"

#define MIN(a, b) (((a) < (b) ? (a) : (b)))
#define MAX(a, b) (((a) < (b) ? (b) : (a)))

#define MINE(a, b) (a) = MIN((a), (b))
#define MAXE(a, b) (a) = MAX((a), (b))

#define ABS(val)                                                               \
  ((val > 0) ? (val) : (decltype(val))(~(val)) + (decltype(val))1)

template <int scale>
bool operator<(BWAPI::Point<int, scale> lhs, BWAPI::Point<int, scale> rhs);

namespace Neolib {
// Random functions
int randint(int, int);

template <typename T> inline T randele(const std::vector<T> &v) {
  return v[randint(0, v.size() - 1)];
}

template <typename Container, typename Key>
bool isIn(const Container &c, const Key &k) {
  return c.find(k) != c.end();
}

template <int Exp, typename T>
T constexpr Pow(T base) {
  if constexpr(Exp == 1)
    return base;
  else if constexpr(Exp == 0)
    return 1;
  else if constexpr(Exp % 2 == 0)
    return Pow<Exp / 2>(base * base);
  else
    return base * Pow<Exp - 1>(base);
}

template <typename T, typename arg> int timeLeftToMake(arg t);

template <> inline int timeLeftToMake<BWAPI::Unit>(BWAPI::Unit u) {
  return u->getType().isBuilding() ? u->getRemainingBuildTime()
                                   : u->getRemainingTrainTime();
}

template <> inline int timeLeftToMake<BWAPI::TechType>(BWAPI::Unit u) {
  return u->getRemainingResearchTime();
}

template <> inline int timeLeftToMake<BWAPI::UpgradeType>(BWAPI::Unit u) {
  return u->getRemainingUpgradeTime();
}

inline int timeToMake(BWAPI::UnitType t) { return t.buildTime(); }
inline int timeToMake(BWAPI::TechType t) { return t.researchTime(); }
inline int timeToMake(BWAPI::UpgradeType t, int level) {
  return t.upgradeTime(level);
}

// Drawing text to screen helpers
const char *noRaceName(const char *name);

struct hash {
  template <int scale>
  size_t operator()(const BWAPI::Point<int, scale> &p) const;
};

template <bool enable> struct latcomBlock {
  inline latcomBlock() : previousState(BWAPI::Broodwar->isLatComEnabled()) {
    BWAPI::Broodwar->setLatCom(enable);
  }
  inline ~latcomBlock() { BWAPI::Broodwar->setLatCom(previousState); }

private:
  bool previousState;
};

} // namespace Neolib
