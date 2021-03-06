#include "Util.h"

#include <random>
#include <chrono>

#include "Neohuman.h"

#ifdef WIN32
std::mt19937_64 mt(std::chrono::system_clock::now().time_since_epoch().count());
#else
std::mt19937_64 mt(std::chrono::steady_clock::now().time_since_epoch().count());
#endif

template <int scale>
bool operator<(BWAPI::Point<int, scale> lhs, BWAPI::Point<int, scale> rhs) {
  if (lhs.x < rhs.x)
    return true;
  if (lhs.x > rhs.x)
    return false;
  return lhs.y < rhs.y;
}

int Neolib::randint(int min, int max) {
  std::uniform_int_distribution<int> dist(min, max);
  return dist(mt);
}

const char *Neolib::noRaceName(const char *name) {
  for (const char *c = name; *c; c++)
    if (*c == '_')
      return ++c;
  return name;
}

template <int scale>
size_t Neolib::hash::operator()(const BWAPI::Point<int, scale> &p) const {
  return std::hash<int>()(p.x << 16 | p.y);
}

auto p = BWAPI::Positions::Invalid;
