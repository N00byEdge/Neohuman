#include "DrawingManager.h"

#include <iomanip>
#include <array>

#include "BaseManager.h"
#include "BuildingQueue.h"
#include "FAP.h"
#include "Neohuman.h"
#include "ResourceManager.h"
#include "SquadManager.h"
#include "SupplyManager.h"
#include "UnitManager.h"

#include "BWAPI.h"
#include "bwem.h"

const std::array<std::string, 8> BotAsciiArt = {
    R"( ad88888ba                                     88     888b      88               88                             )",
    R"(d8"     "8b                                    88     8888b     88               88                             )",
    R"(Y8,                                            88     88 `8b    88               88                             )",
    R"(`Y8aaaaa,     ,adPPYba,  8b,dPPYba,    ,adPPYb,88     88  `8b   88  88       88  88   ,d8   ,adPPYba,  ,adPPYba,)",
    R"(  `"""""8b,  a8P_____88  88P'   `"8a  a8"    `Y88     88   `8b  88  88       88  88 ,a8"   a8P_____88  I8[    "")",
    R"(        `8b  8PP"""""""  88       88  8b       88     88    `8b 88  88       88  8888[     8PP"""""""   `"Y8ba, )",
    R"(Y8a     a8P  "8b,   ,aa  88       88  "8a,   ,d88     88     `8888  "8a,   ,a88  88`"Yba,  "8b,   ,aa  aa    ]8I)",
    R"( "Y88888P"    `"Ybbd8"'  88       88   `"8bbdP"Y8     88      `888   `"YbbdP'Y8  88   `Y8a  `"Ybbd8"'  `"YbbdP"')"
};

const std::vector<int> ColumnXStart = {0, 160, 280, 430, 510, 175};
const std::vector<int> ColumnYStart = {0, 0, 8, 200, 16, 312};

constexpr char ShieldTextColor = BWAPI::Text::Blue;
constexpr char HealthTextColor = BWAPI::Text::Green;
constexpr char EnergyTextColor = BWAPI::Text::Purple;

constexpr char BuildingTextColor = BWAPI::Text::Green;
constexpr char PlacingTextColor = BWAPI::Text::Cyan;

constexpr BWAPI::Color BuildingColor = BWAPI::Colors::Green;
constexpr BWAPI::Color PlacingColor = BWAPI::Colors::Cyan;

constexpr BWAPI::Color SaturationColor = BWAPI::Colors::Blue;
constexpr BWAPI::Color WorkerTextColor = BWAPI::Text::White;

constexpr BWAPI::Color EnemyColor = BWAPI::Colors::Red;
constexpr BWAPI::Color InvalidEnemyColor = BWAPI::Colors::Grey;

constexpr BWAPI::Color ProtossTextColor = BWAPI::Text::Teal;
constexpr BWAPI::Color TerranTextColor = BWAPI::Text::Green;
constexpr BWAPI::Color ZergTextColor = BWAPI::Text::Red;

constexpr int ScreenWidth = IsOnTournamentServer() ? 1920 : 640;
constexpr int ScreenHeight = IsOnTournamentServer() ? 1080 : 480;
constexpr int BottomBarSize = 105;
constexpr int MapAreaHeight = ScreenHeight - BottomBarSize;

constexpr int BarBoxSize = 2;

Neolib::DrawingManager drawingManager;

template<int Offset = 12>
int getNext(int &colY) { return (colY += Offset) - Offset; }

static void DrawUnitBox(BWAPI::Position const pos, BWAPI::UnitType type, BWAPI::Color const c) {
  BWAPI::Broodwar->drawBoxMap(
      pos - BWAPI::Position(type.dimensionLeft(), type.dimensionUp()),
      pos + BWAPI::Position(type.dimensionRight(), type.dimensionDown()),
    c);
}

static void DrawBuildingBox(BWAPI::TilePosition const pos, BWAPI::UnitType type, BWAPI::Color const c) {
  BWAPI::Broodwar->drawBoxMap(
      static_cast<BWAPI::Position>(pos),
      static_cast<BWAPI::Position>(pos + type.tileSize()),
    c);
}

static void DrawTileBox(BWAPI::TilePosition const p, BWAPI::Color const c = BWAPI::Colors::Green) {
  BWAPI::Broodwar->drawBoxMap(
      static_cast<BWAPI::Position>(p),
      static_cast<BWAPI::Position>(p + BWAPI::TilePosition(1, 1)),
    c);
}

bool IsOnScreen(BWAPI::TilePosition const tp) {
	auto start        = static_cast<BWAPI::Position>(tp);
	auto const end    = static_cast<BWAPI::Position>(tp + BWAPI::TilePosition{1, 1});
  auto const camPos = BWAPI::Broodwar->getScreenPosition();
	return camPos.x <= end.x && camPos.y <= end.y && camPos.x + ScreenWidth < end.x && camPos.y + MapAreaHeight < end.y;
}

static void DrawBotName() {
  constexpr auto charWidth  = 5;
  constexpr auto charHeight = 8;
  constexpr auto boxWidth   = 112 * charWidth;
  constexpr auto boxHeight  = 8   * charHeight;

  BWAPI::Broodwar->setTextSize(BWAPI::Text::Size::Small);
  BWAPI::Broodwar->drawBoxScreen(ScreenWidth - boxWidth, 0, ScreenWidth, boxHeight, BWAPI::Colors::Black, true);

  auto ypos = 0;
  for (auto &s : BotAsciiArt) {
    auto xpos = 0;
    for (auto &c : s) {
      BWAPI::Broodwar->drawTextScreen(ScreenWidth - boxWidth + getNext<charWidth>(xpos), ypos,
                                      "%c%c", BWAPI::Text::White, c);
    }
    getNext<charHeight>(ypos);
  }
  BWAPI::Broodwar->setTextSize(BWAPI::Text::Size::Default);
}

static BWAPI::Color HealthColor(int const health, int const maxHealth) {
  if (health < maxHealth / 3)
    return BWAPI::Colors::Red;
  if (health < (maxHealth * 2) / 3)
    return BWAPI::Colors::Yellow;
  return BWAPI::Colors::Green;
}

static void DrawBar(BWAPI::Position const pos, int const fill, int const max, int const width, BWAPI::Color const c) {
  auto const nBoxes = width / BarBoxSize;
  BWAPI::Broodwar->drawBoxMap(pos, pos + BWAPI::Position(nBoxes * BarBoxSize + 1, BarBoxSize + 1), BWAPI::Colors::Black, true);
  if (!max)
    return;
  auto const drawBoxes = MIN((nBoxes * fill) / max, nBoxes);
  for (auto i = 0; i < drawBoxes; ++i)
    BWAPI::Broodwar->drawBoxMap(
        pos + BWAPI::Position(i * BarBoxSize + 1, 1),
        pos + BWAPI::Position((i + 1) * BarBoxSize, BarBoxSize),
      c, true);
}

static void DrawBars(BWAPI::Position const pos, BWAPI::UnitType const type, int const health, int const shields, int const energy,
                     int const loadedUnits, int const resources, int const initialResources, bool const showEnergy) {
  auto const barWidth = type.dimensionLeft() + type.dimensionRight();
  BWAPI::Position barPos(pos.x - type.dimensionLeft(),
                         pos.y + type.dimensionDown() + 3);

  if (type.maxShields() > 0) {
    DrawBar(barPos, shields, type.maxShields(), barWidth, BWAPI::Colors::Blue);
    barPos.y += BarBoxSize;
  }

  if (type.maxHitPoints() > 0 && !type.isMineralField() &&
      type != BWAPI::UnitTypes::Resource_Vespene_Geyser) {
    DrawBar(barPos, health, type.maxHitPoints(), barWidth, HealthColor(health, type.maxHitPoints()));
    barPos.y += BarBoxSize;
  }

  if (type.maxEnergy() > 0 && showEnergy) {
    DrawBar(barPos, energy, type.maxEnergy(), barWidth, BWAPI::Colors::Purple);
    barPos.y += BarBoxSize;
  }

  if (type.isResourceContainer()) {
    DrawBar(barPos, resources, initialResources, barWidth, BWAPI::Colors::Cyan);
    barPos.y += BarBoxSize;
  }
}

namespace Neolib {

DrawingManager::DrawingManager() {}

DrawingManager::DrawingManager(DrawerSettings s) : s(s) {}

void DrawingManager::onFrame() const {
  std::vector<int> nextColumnY = ColumnYStart;

  if (s.enableTopInfo) {

    // BWAPI::Broodwar->drawTextScreen(columnXStart[1],
    // getNext(nextColumnY[1]), "FPS: %c%d", BWAPI::Broodwar->getFPS() >=
    // 30 ? BWAPI::Text::Green : BWAPI::Text::Red, BWAPI::Broodwar->getFPS());
    // BWAPI::Broodwar->drawTextScreen(columnXStart[1],
    // getNext(nextColumnY[1]), "Average FPS: %c%f",
    // BWAPI::Broodwar->getAverageFPS() >= 30 ? BWAPI::Text::Green :
    // BWAPI::Text::Red, BWAPI::Broodwar->getAverageFPS());
    BWAPI::Broodwar->drawTextScreen(
        ColumnXStart[1], getNext(nextColumnY[1]),
        "Nukes: %c%u Armed %c%u Arming %c%u Unarmed", BWAPI::Text::BrightRed,
        unitManager.getNumArmedSilos(), BWAPI::Text::Orange,
        unitManager.getNumArmingSilos(), BWAPI::Text::Yellow,
        unitManager.getNumUnarmedSilos());
    BWAPI::Broodwar->drawTextScreen(
        ColumnXStart[1], getNext(nextColumnY[1]),
        "%c%d%c nukes launched", BWAPI::Text::Green,
        unitManager.getLaunchedNukeCount(), BWAPI::Text::Default);
    // BWAPI::Broodwar->drawTextScreen(columnXStart[1],
    // getNext(nextColumnY[1]), "I have %d barracks!",
    // unitManager.countFriendly(BWAPI::UnitTypes::Terran_Barracks));
    BWAPI::Broodwar->drawTextScreen(
        ColumnXStart[1], getNext(nextColumnY[1]), "I have %d APM!",
        BWAPI::Broodwar->getAPM());
    BWAPI::Broodwar->drawTextScreen(
        ColumnXStart[1], getNext(nextColumnY[1]), "Income: %c%d %c%d",
        BWAPI::Text::Blue, resourceManager.getMinuteApproxIncome().minerals,
        BWAPI::Text::Green, resourceManager.getMinuteApproxIncome().gas,
        BWAPI::Text::Default, baseManager.getHomelessWorkers().size());
    BWAPI::Broodwar->drawTextScreen(
        ColumnXStart[1], getNext(nextColumnY[1]), "%u Idle workers",
        baseManager.getHomelessWorkers().size());
  }

  if (s.enableComsatInfo) {
    auto s = "Comsats (" + std::to_string(unitManager.getFriendlyUnitsByType(BWAPI::UnitTypes::Terran_Comsat_Station).size()) + "): " + EnergyTextColor;

    for (auto &c : unitManager.getFriendlyUnitsByType(BWAPI::UnitTypes::Terran_Comsat_Station))
      s += std::to_string(c->getEnergy()) + " ";

    BWAPI::Broodwar->drawTextScreen(ColumnXStart[0], getNext(nextColumnY[0]), s.c_str());
  }

  if (s.enableResourceOverlay) {

    BWAPI::Broodwar->drawTextScreen(450, 16, "%d", resourceManager.getSpendableResources().minerals);
    BWAPI::Broodwar->drawTextScreen(480, 16, "%d", resourceManager.getSpendableResources().gas);

    BWAPI::Broodwar->drawTextScreen(ColumnXStart[4], getNext(nextColumnY[4]), "%c%d/%d, %d, %d",
        ProtossTextColor, supplyManager.usedSupply().protoss, supplyManager.availableSupply().protoss,
        supplyManager.wantedAdditionalSupply().protoss, supplyManager.wantedSupplyOverhead().protoss);

    BWAPI::Broodwar->drawTextScreen( ColumnXStart[4], getNext(nextColumnY[4]), "%c%d/%d, %d, %d",
        TerranTextColor, supplyManager.usedSupply().terran, supplyManager.availableSupply().terran,
        supplyManager.wantedAdditionalSupply().terran, supplyManager.wantedSupplyOverhead().terran);

    BWAPI::Broodwar->drawTextScreen(ColumnXStart[4], getNext(nextColumnY[4]), "%c%d/%d, %d, %d",
        ZergTextColor, supplyManager.usedSupply().zerg, supplyManager.availableSupply().zerg,
        supplyManager.wantedAdditionalSupply().zerg, supplyManager.wantedSupplyOverhead().zerg);
  }

  if (s.enableBWEMOverlay) {

    BWEM::utils::gridMapExample(BWEM::Map::Instance());
    BWEM::utils::drawMap(BWEM::Map::Instance());
  }

  if (s.enableListBuildingQueue) {

    BWAPI::Broodwar->drawTextScreen(
        ColumnXStart[0], getNext(nextColumnY[0]),
        "%u buildings in queue", buildingQueue.buildingsQueued().size());

    for (auto &o : buildingQueue.buildingsQueued()) {
      BWAPI::Broodwar->drawTextScreen(
          ColumnXStart[0], getNext(nextColumnY[0]), "%c%02d%% %s",
          o.buildingUnit ? BuildingTextColor : PlacingTextColor,
          o.buildingUnit ? ((o.buildingUnit->getType().buildTime() -
                             o.buildingUnit->getRemainingBuildTime()) *
                            100) /
                               o.buildingUnit->getType().buildTime()
                         : 0,
          noRaceName(o.buildingType.c_str()));
      DrawBuildingBox(o.designatedLocation, o.buildingType,
                      o.buildingUnit ? BuildingColor : PlacingColor);
      BWAPI::Broodwar->drawTextMap(
          BWAPI::Position(o.designatedLocation) + BWAPI::Position(10, 10), "%s",
          noRaceName(o.buildingType.getName().c_str()));
      if (o.builder) {
        BWAPI::Broodwar->drawLineMap(o.builder->getPosition(),
                                     (BWAPI::Position)o.designatedLocation,
                                     BWAPI::Colors::Green);
        BWAPI::Broodwar->drawTextMap(o.builder->getPosition(),
                                     o.builder->getOrder().c_str());
      }
    }
  }

  if (s.enableSquadOverlay) {
    if (s.enableTopInfo) {
      BWAPI::Broodwar->drawTextScreen(
          ColumnXStart[1], getNext(nextColumnY[1]), "%u Enemy squads",
          squadManager.getEnemySquads().size());
      BWAPI::Broodwar->drawTextScreen(
          ColumnXStart[1], getNext(nextColumnY[1]), "%u Friendly squads",
          squadManager.getFriendlySquads().size());
    }

    for (auto &fs : squadManager.getFriendlySquads()) {
      BWAPI::Broodwar->drawCircleMap(fs->pos, fs->radius1,
                                     BWAPI::Colors::Green);
      BWAPI::Broodwar->drawCircleMap(fs->pos, fs->radius2, BWAPI::Colors::Red);

      BWAPI::Broodwar->drawTextMap(fs->pos, "Combat: %d", fs->simres.shortWin);
    }

    for (auto &es : squadManager.getEnemySquads()) {
      BWAPI::Broodwar->drawCircleMap(es->pos, es->radius1,
                                     BWAPI::Colors::Green);
      BWAPI::Broodwar->drawCircleMap(es->pos, es->radius2, BWAPI::Colors::Red);
    }
  }

  if (s.enableSaturationInfo) {

    for (auto &depotType :
         std::vector<BWAPI::UnitType>{BWAPI::UnitTypes::Protoss_Nexus,
                                      BWAPI::UnitTypes::Terran_Command_Center,
                                      BWAPI::UnitTypes::Zerg_Hatchery}) {
      for (auto &u : unitManager.getFriendlyUnitsByType(depotType)) {
        BWAPI::Broodwar->drawEllipseMap(
            u->getPosition(), saturationRadius + depotType.tileSize().x * 32,
            saturationRadius + depotType.tileSize().y * 32, SaturationColor);
        auto workers = u->getUnitsInRadius(
            saturationRadius, (BWAPI::Filter::IsGatheringMinerals));
        auto refineries = u->getUnitsInRadius(
            saturationRadius,
            (BWAPI::Filter::GetType == BWAPI::UnitTypes::Protoss_Assimilator));
        auto mineralFields = u->getUnitsInRadius(
            saturationRadius, (BWAPI::Filter::IsMineralField));
        BWAPI::Broodwar->drawTextMap(u->getPosition() + BWAPI::Position(0, 30),
                                     "%cWorkers: %d/%d", WorkerTextColor,
                                     workers.size(), 2 * mineralFields.size());
      }
    }
  }

  if (s.enableEnemyOverlay) {
    auto const annotateEnemy = [](const std::shared_ptr<EnemyData> &enemy) {
      DrawUnitBox(enemy->lastPosition, enemy->lastType, EnemyColor);
      BWAPI::Broodwar->drawTextMap(enemy->lastPosition + BWAPI::Position(enemy->lastType.dimensionRight(), enemy->lastType.dimensionDown()), "%s", noRaceName(enemy->lastType.c_str()));
    };

    for (auto &u : unitManager.getVisibleEnemies())
      annotateEnemy(u);

    for (auto &u : unitManager.getNonVisibleEnemies())
      annotateEnemy(u);

    for (auto &ut : unitManager.getEnemyUnitsByType())
      BWAPI::Broodwar->drawTextScreen(ColumnXStart[2], getNext(nextColumnY[2]), "%s: %u", noRaceName(ut.first.c_str()), ut.second.size());
  }

  if (s.enableHealthBars) {
    for (auto &u : BWAPI::Broodwar->self()->getUnits())
      if (u->isVisible())
        DrawBars(u->getPosition(), u->getType(), u->getHitPoints(), u->getShields(), u->getEnergy(),
            static_cast<int>(u->getLoadedUnits().size()), u->getResources(), 5000, u->getType().isSpellcaster());

    const auto annotateResource = [](BWAPI::Unit resource) {
      DrawBars(resource->getPosition(), resource->getType(), 0, 0, 0, 0, resource->getResources(), resource->getInitialResources(), false);
    };

    for (auto &b : baseManager.getAllBases()) {
      for (auto &mf : b.mineralMiners)
        annotateResource(mf.first);

      for (auto &gg : b.gasGeysers)
        annotateResource(gg);
    }

    const auto drawEnemyHealthBar = [](const std::shared_ptr<EnemyData> &enemy) {
      DrawBars(enemy->lastPosition, enemy->lastType, enemy->expectedHealth(), enemy->expectedShields(), 0, 0, 0, 0, false);
    };

    for (auto &u : unitManager.getVisibleEnemies())
      drawEnemyHealthBar(u);
    for (auto &u : unitManager.getNonVisibleEnemies())
      drawEnemyHealthBar(u);
  }

  if (s.enableDeathMatrix) {
    auto const maxX = BWAPI::Broodwar->mapWidth() * 4;
    auto const maxY = BWAPI::Broodwar->mapHeight() * 4;
    for (auto x = 0; x < maxX; ++x) {
      for (auto y = 0; y < maxY; ++y) {
        const auto deathColor = [](int death) {
          if (death && death <= 10)
            death = BWAPI::Colors::Green;
          else if (death && death <= 30)
            death = BWAPI::Colors::Yellow;
          else if (death && death <= 100)
            death = BWAPI::Colors::Orange;
          else if (death)
            death = BWAPI::Colors::Red;
          return BWAPI::Colors::Grey;
        };

        auto const deathGround = DeathMatrixGround[y * deathMatrixSideLen + x];
        auto const deathAir    = DeathMatrixAir   [y * deathMatrixSideLen + x];

        if (!deathGround && !deathAir)
          continue;

        BWAPI::Broodwar->drawTriangleMap(BWAPI::Position(x * 8, y * 8), BWAPI::Position(x * 8 + 6, y * 8), BWAPI::Position(x * 8, y * 8 + 6), deathColor(deathGround), false);
        BWAPI::Broodwar->drawTriangleMap(BWAPI::Position(x * 8 + 7, y * 8 + 7), BWAPI::Position(x * 8 + 7, y * 8), BWAPI::Position(x * 8, y * 8 + 7), deathColor(deathAir), false);
      }
    }
  }

  if (s.enableBaseOverlay) {
    BWAPI::Position const offset(BWAPI::UnitTypes::Terran_Command_Center.dimensionLeft(), BWAPI::UnitTypes::Terran_Command_Center.dimensionUp());

    for (auto &b : baseManager.getAllBases()) {
      auto const sq = b.getNoBuildRegion();
      BWAPI::Broodwar->drawBoxMap(static_cast<BWAPI::Position>(sq.first), static_cast<BWAPI::Position>(sq.second) - BWAPI::Position(-1, -1), BWAPI::Colors::Cyan, false);

      if (b.redAlert == -1)
        BWAPI::Broodwar->drawTextMap(b.resourceDepot->getPosition() - offset, "Income: %c%u %c%u", BWAPI::Text::Blue, b.calculateIncome().minerals, BWAPI::Text::Green, b.calculateIncome().gas);
      else
        BWAPI::Broodwar->drawTextMap(b.resourceDepot->getPosition() - offset, "%cRED ALERT!", BWAPI::Text::Red);

      FastAPproximation fap;
      std::map<BWAPI::UnitType, int> unitCountsFriendly, unitCountsEnemy;
      auto const state = fap.getState();

      auto constexpr yStart = 220;
      auto y = yStart;
      for (auto &u : *state.first)
        ++unitCountsFriendly[u.unitType];
      for (auto &u : *state.second)
        ++unitCountsEnemy[u.unitType];

      for (auto &e : unitCountsFriendly)
        BWAPI::Broodwar->drawTextScreen(150, getNext<20>(y), "%s, %d", e.first.c_str(), e.second);

      y = yStart;

      for (auto &e : unitCountsEnemy)
        BWAPI::Broodwar->drawTextScreen(250, getNext<20>(y), "%s, %d", e.first.c_str(), e.second);

      for (auto &m : b.mineralMiners) {
        DrawBuildingBox(m.first->getTilePosition(), BWAPI::UnitTypes::Resource_Mineral_Field, BWAPI::Colors::Blue);
        BWAPI::Broodwar->drawTextMap(m.first->getPosition() - BWAPI::Position(10, 5), "%u/2", m.second.size());

        for (auto &w : m.second) {
          if (!w->isVisible())
            continue;
          BWAPI::Broodwar->drawLineMap(w->getPosition(), m.first->getPosition(), BWAPI::Colors::Orange);
          BWAPI::Broodwar->drawLineMap(w->getPosition(), b.resourceDepot->getPosition(), BWAPI::Colors::Orange);
        }
      }

      for (auto &g : b.gasMiners) {
        DrawBuildingBox(g.first->getTilePosition(), BWAPI::UnitTypes::Resource_Vespene_Geyser, BWAPI::Colors::Green);
        BWAPI::Broodwar->drawTextMap(g.first->getPosition() - BWAPI::Position(14, 10), "%u/3", g.second.size());
        for (auto &w : g.second) {
          if (!w->isVisible())
            continue;
          BWAPI::Broodwar->drawLineMap(w->getPosition(), g.first->getPosition(), BWAPI::Colors::Red);
          BWAPI::Broodwar->drawLineMap(w->getPosition(), b.resourceDepot->getPosition(), BWAPI::Colors::Red);
        }
      }
    }
  }

  if (s.enableFailedLocations) {
    for (auto &f : failedLocations) {
      DrawBuildingBox(f.first, f.second, BWAPI::Colors::Orange);
    }
  }

  if (s.enableNukeSpots) {
    for (auto &nd : BWAPI::Broodwar->getNukeDots()) {
      BWAPI::Broodwar->drawLineMap(nd + BWAPI::Position(-100, -100), nd + BWAPI::Position(100, 100), BWAPI::Colors::Yellow);
      BWAPI::Broodwar->drawLineMap(nd + BWAPI::Position(-100, 100), nd + BWAPI::Position(100, -100), BWAPI::Colors::Yellow);
      BWAPI::Broodwar->drawCircleMap(nd, 10 * 32, BWAPI::Colors::Yellow, false);
    }
  }

  if (s.enableCombatSimOverlay) {
    auto res = unitManager.getSimResults();
    BWAPI::Broodwar->drawTextScreen(ColumnXStart[5], getNext(nextColumnY[5]), "Combat sim: %d", res.shortWin);

    BWAPI::Broodwar->drawTextScreen(ColumnXStart[5], getNext(nextColumnY[5]), "Presim  units: %3u %3u Presim  scores: %5d %5d",
      res.presim.unitCounts.first, res.presim.unitCounts.second, res.presim.scores.first, res.presim.scores.second);

    BWAPI::Broodwar->drawTextScreen(ColumnXStart[5], getNext(nextColumnY[5]), "Shrtsim units: %3u %3u Shrtsim scores: %5d %5d",
        res.shortsim.unitCounts.first, res.shortsim.unitCounts.second, res.shortsim.scores.first, res.shortsim.scores.second);

    BWAPI::Broodwar->drawTextScreen(ColumnXStart[5], getNext(nextColumnY[5]), "Postsim units: %3u %3u Postsim scores: %5d %5d",
        res.postsim.unitCounts.first, res.postsim.unitCounts.second, res.postsim.scores.first, res.postsim.scores.second);
  }

  if constexpr (IsOnTournamentServer())
    DrawBotName();
}

}

