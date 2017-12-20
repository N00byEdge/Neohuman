#pragma once

#include "BWAPI.h"

#define SCANCOOLDOWN 100

namespace Neolib {

struct DetectionManager {
  bool requestDetection(BWAPI::Position p);
  int highestComsatEnergy();

private:
  int scanLastUsed = -5;
};

} // namespace Neolib

extern Neolib::DetectionManager detectionManager;
