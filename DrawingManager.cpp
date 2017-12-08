#include "DrawingManager.h"

#include "Neohuman.h"
#include "SupplyManager.h"
#include "UnitManager.h"
#include "ResourceManager.h"
#include "BuildingQueue.h"
#include "MapManager.h"
#include "BaseManager.h"
#include "FAP.h"
#include "SquadManager.h"

#include "BWAPI.h"
#include "bwem.h"

const std::vector <std::string> botAsciiArt = {
	" ad88888ba                                     88     888b      88               88",
	"d8\"     \"8b                                    88     8888b     88               88",
	"Y8,                                            88     88 `8b    88               88",
	"`Y8aaaaa,     ,adPPYba,  8b,dPPYba,    ,adPPYb,88     88  `8b   88  88       88  88   ,d8   ,adPPYba,  ,adPPYba,",
	"  `\"\"\"\"\"8b,  a8P_____88  88P'   `\"8a  a8\"    `Y88     88   `8b  88  88       88  88 ,a8\"   a8P_____88  I8[    \"\"",
	"        `8b  8PP\"\"\"\"\"\"\"  88       88  8b       88     88    `8b 88  88       88  8888[     8PP\"\"\"\"\"\"\"   `\"Y8ba,",
	"Y8a     a8P  \"8b,   ,aa  88       88  \"8a,   ,d88     88     `8888  \"8a,   ,a88  88`\"Yba,  \"8b,   ,aa  aa    ]8I",
	" \"Y88888P\"    `\"Ybbd8\"'  88       88   `\"8bbdP\"Y8     88      `888   `\"YbbdP'Y8  88   `Y8a  `\"Ybbd8\"'  `\"YbbdP\"'"
};

const std::vector <int> columnXStart = { 0, 160, 280, 430, 510, 175 };
const std::vector <int> columnYStart = { 0, 0, 0, 200, 16, 312 };

#define SHIELDTEXT		(char)BWAPI::Text::Blue
#define HEALTHTEXT		(char)BWAPI::Text::Green
#define ENERGYTEXT		(char)BWAPI::Text::Purple

#define BUILDINGTEXT	(char)BWAPI::Text::Green
#define PLACINGTEXT		(char)BWAPI::Text::Cyan

#define SATURATIONCOLOR BWAPI::Colors::Blue
#define WORKERTXTCOLOR	BWAPI::Text::White

#define BUILDINGCOLOR	BWAPI::Colors::Green
#define PLACINGCOLOR	BWAPI::Colors::Cyan

#define ENEMYCOLOR		BWAPI::Colors::Red
#define INVALIDENEMYCLR BWAPI::Colors::Grey

#define PROTOSSTEXT		BWAPI::Text::Teal
#define TERRANTEXT		BWAPI::Text::Green
#define ZERGTEXT		BWAPI::Text::Red

#define BOXSIZE			2

Neolib::DrawingManager drawingManager;

const inline static void drawUnitBox(BWAPI::Position pos, BWAPI::UnitType type, BWAPI::Color c) {
	BWAPI::Broodwar->drawBoxMap(pos - BWAPI::Position(type.dimensionLeft(), type.dimensionUp()), pos + BWAPI::Position(type.dimensionRight(), type.dimensionDown()), c);
}

const inline static void drawBuildingBox(BWAPI::TilePosition pos, BWAPI::UnitType type, BWAPI::Color c) {
	BWAPI::Broodwar->drawBoxMap((BWAPI::Position)pos, (BWAPI::Position)pos + (BWAPI::Position)type.tileSize(), c);
}

const inline static void drawBotName() {
	constexpr int charWidth = 5;
	constexpr int charHeight = 8;
	constexpr int boxWidth = 111 * charWidth;
	constexpr int boxHeight = 8 * charHeight;
#ifdef SSCAIT
	constexpr int screenWidth = 1920;
#else
	constexpr int screenWidth = 640;
#endif

	BWAPI::Broodwar->setTextSize(BWAPI::Text::Size::Small);
	BWAPI::Broodwar->drawBoxScreen(screenWidth - boxWidth, 0, screenWidth, boxHeight, BWAPI::Colors::Black, true);
	int ypos = 0;
	for (auto &s : botAsciiArt) {
		int xpos = 0;
		for (auto &c : s) {
			BWAPI::Broodwar->drawTextScreen(screenWidth - boxWidth + xpos, ypos, "%c%c", BWAPI::Text::White, c);
			xpos += charWidth;
		}
		ypos += charHeight;
	}
	BWAPI::Broodwar->setTextSize(BWAPI::Text::Size::Default);
}

const inline static BWAPI::Color healthColor(int health, int maxHealth) {
	if (health < maxHealth / 3)
		return BWAPI::Colors::Red;
	if (health < (maxHealth * 2) / 3)
		return BWAPI::Colors::Yellow;
	return BWAPI::Colors::Green;
}

const inline static void drawBar(BWAPI::Position pos, int fill, int max, int width, BWAPI::Color c) {
	int nBoxes = width / BOXSIZE;
	BWAPI::Broodwar->drawBoxMap(pos, pos + BWAPI::Position(nBoxes * BOXSIZE + 1, BOXSIZE + 1), BWAPI::Colors::Black, true);
	if (!max)
		return;
	int drawBoxes = MIN((nBoxes * fill) / max, nBoxes);
	for (int i = 0; i < drawBoxes; ++i)
		BWAPI::Broodwar->drawBoxMap(pos + BWAPI::Position(i * BOXSIZE + 1, 1), pos + BWAPI::Position((i + 1) * BOXSIZE, BOXSIZE), c, true);
}

const inline static void drawBars(BWAPI::Position pos, BWAPI::UnitType type, int health, int shields, int energy, int loadedUnits, int resources, int initialResources, bool showEnergy) {
	int offset = 0;
	int barWidth = type.dimensionLeft() + type.dimensionRight();
	BWAPI::Position barPos(pos.x - type.dimensionLeft(), pos.y + type.dimensionDown() + 3);

	if (type.maxShields() > 0) {
		drawBar(barPos, shields, type.maxShields(), barWidth, BWAPI::Colors::Blue);
		barPos.y += BOXSIZE;
	}

	if (type.maxHitPoints() > 0 && !type.isMineralField()) {
		drawBar(barPos, health, type.maxHitPoints(), barWidth, healthColor(health, type.maxHitPoints()));
		barPos.y += BOXSIZE;
	}

	if (type.maxEnergy() > 0 && showEnergy) {
		drawBar(barPos, energy, type.maxEnergy(), barWidth, BWAPI::Colors::Purple);
		barPos.y += BOXSIZE;
	}

	if (type.isResourceContainer()) {
		drawBar(barPos, resources, initialResources, barWidth, BWAPI::Colors::Cyan);
		barPos.y += BOXSIZE;
	}
}

namespace Neolib {

	DrawingManager::DrawingManager() {

	}

	DrawingManager::DrawingManager(DrawerSettings s): s(s) {

	}

	void DrawingManager::onFrame() const {
		std::vector <int> nextColumnY = columnYStart;

		if (s.enableTopInfo) {

			//BWAPI::Broodwar->drawTextScreen(columnXStart[1], getNextColumnY(nextColumnY[1]), "FPS: %c%d", BWAPI::Broodwar->getFPS() >= 30 ? BWAPI::Text::Green : BWAPI::Text::Red, BWAPI::Broodwar->getFPS());
			//BWAPI::Broodwar->drawTextScreen(columnXStart[1], getNextColumnY(nextColumnY[1]), "Average FPS: %c%f", BWAPI::Broodwar->getAverageFPS() >= 30 ? BWAPI::Text::Green : BWAPI::Text::Red, BWAPI::Broodwar->getAverageFPS());
			BWAPI::Broodwar->drawTextScreen(columnXStart[1], getNextColumnY(nextColumnY[1]), "Nukes: %c%u Armed %c%u Arming %c%u Unarmed", BWAPI::Text::BrightRed, unitManager.getNumArmedSilos(), BWAPI::Text::Orange, unitManager.getNumArmingSilos(), BWAPI::Text::Yellow, unitManager.getNumUnarmedSilos());
			BWAPI::Broodwar->drawTextScreen(columnXStart[1], getNextColumnY(nextColumnY[1]), "%c%d%c nukes launched", BWAPI::Text::Green, unitManager.getLaunchedNukeCount(), BWAPI::Text::Default);
			//BWAPI::Broodwar->drawTextScreen(columnXStart[1], getNextColumnY(nextColumnY[1]), "I have %d barracks!", unitManager.countFriendly(BWAPI::UnitTypes::Terran_Barracks));
			BWAPI::Broodwar->drawTextScreen(columnXStart[1], getNextColumnY(nextColumnY[1]), "I have %d APM!", BWAPI::Broodwar->getAPM());
			BWAPI::Broodwar->drawTextScreen(columnXStart[1], getNextColumnY(nextColumnY[1]), "Income: %c%d %c%d", BWAPI::Text::Blue, resourceManager.getMinuteApproxIncome().minerals, BWAPI::Text::Green, resourceManager.getMinuteApproxIncome().gas, BWAPI::Text::Default, baseManager.getHomelessWorkers().size());
			BWAPI::Broodwar->drawTextScreen(columnXStart[1], getNextColumnY(nextColumnY[1]), "%u Idle workers", baseManager.getHomelessWorkers().size());

		}

		if (s.enableComsatInfo) {

			std::string s = "Comsats (" + std::to_string(unitManager.getFriendlyUnitsByType(BWAPI::UnitTypes::Terran_Comsat_Station).size()) + "): " + ENERGYTEXT;
			for (auto &c : unitManager.getFriendlyUnitsByType(BWAPI::UnitTypes::Terran_Comsat_Station))
				s += std::to_string(c->getEnergy()) + " ";

			BWAPI::Broodwar->drawTextScreen(columnXStart[0], getNextColumnY(nextColumnY[0]), s.c_str());

		}

		if (s.enableResourceOverlay) {

			BWAPI::Broodwar->drawTextScreen(450, 16, "%d", resourceManager.getSpendableResources().minerals);
			BWAPI::Broodwar->drawTextScreen(480, 16, "%d", resourceManager.getSpendableResources().gas);

			BWAPI::Broodwar->drawTextScreen(columnXStart[4], getNextColumnY(nextColumnY[4]), "%c%d/%d, %d, %d", PROTOSSTEXT, supplyManager.usedSupply().protoss, supplyManager.availableSupply().protoss, supplyManager.wantedAdditionalSupply().protoss, supplyManager.wantedSupplyOverhead().protoss);
			BWAPI::Broodwar->drawTextScreen(columnXStart[4], getNextColumnY(nextColumnY[4]), "%c%d/%d, %d, %d", TERRANTEXT, supplyManager.usedSupply().terran, supplyManager.availableSupply().terran, supplyManager.wantedAdditionalSupply().terran, supplyManager.wantedSupplyOverhead().terran);
			BWAPI::Broodwar->drawTextScreen(columnXStart[4], getNextColumnY(nextColumnY[4]), "%c%d/%d, %d, %d", ZERGTEXT, supplyManager.usedSupply().zerg, supplyManager.availableSupply().zerg, supplyManager.wantedAdditionalSupply().zerg, supplyManager.wantedSupplyOverhead().zerg);

		}

		if (s.enableBWEMOverlay) {

			BWEM::utils::gridMapExample(BWEM::Map::Instance());
			BWEM::utils::drawMap(BWEM::Map::Instance());

		}

		if (s.enableListBuildingQueue) {

			BWAPI::Broodwar->drawTextScreen(columnXStart[0], getNextColumnY(nextColumnY[0]), "%u buildings in queue", buildingQueue.buildingsQueued().size());

			for (auto &o : buildingQueue.buildingsQueued()) {
				BWAPI::Broodwar->drawTextScreen(columnXStart[0], getNextColumnY(nextColumnY[0]), "%c%s", o.buildingUnit ? BUILDINGTEXT : PLACINGTEXT, noRaceName(o.buildingType.c_str()));
				drawBuildingBox(o.designatedLocation, o.buildingType, o.buildingUnit ? BUILDINGCOLOR : PLACINGCOLOR);
				BWAPI::Broodwar->drawTextMap(BWAPI::Position(o.designatedLocation) + BWAPI::Position(10, 10), "%s", noRaceName(o.buildingType.getName().c_str()));
				if (o.builder) {
					BWAPI::Broodwar->drawLineMap(o.builder->getPosition(), (BWAPI::Position) o.designatedLocation, BWAPI::Colors::Green);
					BWAPI::Broodwar->drawTextMap(o.builder->getPosition(), o.builder->getOrder().c_str());
				}
			}
		}

		if (s.enableSquadOverlay) {
			if (s.enableTopInfo)
				BWAPI::Broodwar->drawTextScreen(columnXStart[1], getNextColumnY(nextColumnY[1]), "%u Enemy squads", squadManager.getEnemySquads().size());

			for (auto &fs : squadManager.getFriendlySquads()) {
				BWAPI::Broodwar->drawCircleMap(fs->pos, fs->radius1, BWAPI::Colors::Green);
				BWAPI::Broodwar->drawCircleMap(fs->pos, fs->radius2, BWAPI::Colors::Red);

				for (auto &es : fs->engagedSquads)
					BWAPI::Broodwar->drawLineMap(fs->pos, es->pos, BWAPI::Colors::Red);
			}

			for (auto &es : squadManager.getEnemySquads()) {
				BWAPI::Broodwar->drawCircleMap(es->pos, es->radius1, BWAPI::Colors::Green);
				BWAPI::Broodwar->drawCircleMap(es->pos, es->radius2, BWAPI::Colors::Red);

				BWAPI::Broodwar->drawTextMap(es->pos, "%u enemy units", es->units.size());
			}
		}

		if (s.enableSaturationInfo) {

			for (auto &u : unitManager.getFriendlyUnitsByType(BWAPI::UnitTypes::Protoss_Nexus)) {
				BWAPI::Broodwar->drawEllipseMap(u->getPosition(), SATRUATION_RADIUS + BWAPI::UnitTypes::Protoss_Nexus.tileSize().x * 32, SATRUATION_RADIUS + BWAPI::UnitTypes::Protoss_Nexus.tileSize().y * 32, SATURATIONCOLOR);
				auto workers = u->getUnitsInRadius(SATRUATION_RADIUS, (BWAPI::Filter::IsGatheringMinerals));
				auto refineries = u->getUnitsInRadius(SATRUATION_RADIUS, (BWAPI::Filter::GetType == BWAPI::UnitTypes::Protoss_Assimilator));
				auto mineralFields = u->getUnitsInRadius(SATRUATION_RADIUS, (BWAPI::Filter::IsMineralField));
				BWAPI::Broodwar->drawTextMap(u->getPosition() + BWAPI::Position(0, 30), "%cWorkers: %d/%d", WORKERTXTCOLOR, workers.size(), 2 * mineralFields.size());
			}

			for (auto &u : unitManager.getFriendlyUnitsByType(BWAPI::UnitTypes::Terran_Command_Center)) {
				BWAPI::Broodwar->drawEllipseMap(u->getPosition(), SATRUATION_RADIUS + BWAPI::UnitTypes::Terran_Command_Center.tileSize().x * 32, SATRUATION_RADIUS + BWAPI::UnitTypes::Terran_Command_Center.tileSize().y * 32, SATURATIONCOLOR);
				auto workers = u->getUnitsInRadius(SATRUATION_RADIUS, (BWAPI::Filter::IsGatheringMinerals));
				auto refineries = u->getUnitsInRadius(SATRUATION_RADIUS, (BWAPI::Filter::GetType == BWAPI::UnitTypes::Terran_Refinery));
				auto mineralFields = u->getUnitsInRadius(SATRUATION_RADIUS, (BWAPI::Filter::IsMineralField));
				BWAPI::Broodwar->drawTextMap(u->getPosition() + BWAPI::Position(0, 30), "%cWorkers: %d/%d", WORKERTXTCOLOR, workers.size(), 2 * mineralFields.size());
			}

			for (auto &u : unitManager.getFriendlyUnitsByType(BWAPI::UnitTypes::Zerg_Hatchery)) {
				BWAPI::Broodwar->drawEllipseMap(u->getPosition(), SATRUATION_RADIUS + BWAPI::UnitTypes::Zerg_Hatchery.tileSize().x * 32, SATRUATION_RADIUS + BWAPI::UnitTypes::Zerg_Hatchery.tileSize().y * 32, SATURATIONCOLOR);
				auto workers = u->getUnitsInRadius(SATRUATION_RADIUS, (BWAPI::Filter::IsGatheringMinerals));
				auto refineries = u->getUnitsInRadius(SATRUATION_RADIUS, (BWAPI::Filter::GetType == BWAPI::UnitTypes::Zerg_Extractor));
				auto mineralFields = u->getUnitsInRadius(SATRUATION_RADIUS, (BWAPI::Filter::IsMineralField));
				BWAPI::Broodwar->drawTextMap(u->getPosition() + BWAPI::Position(0, 30), "%cWorkers: %d/%d", WORKERTXTCOLOR, workers.size(), 2 * mineralFields.size());
			}

			for (auto &w : baseManager.getHomelessWorkers())
				BWAPI::Broodwar->drawTextMap(w->getPosition(), "H");

		}

		if (s.enableTimerInfo) {

			BWAPI::Broodwar->drawTextScreen(columnXStart[3], getNextColumnY(nextColumnY[3]), "Frame times [last/avg/high]:");
			BWAPI::Broodwar->drawTextScreen(columnXStart[3], getNextColumnY(nextColumnY[3]), "Total: %.1lf ms / %.1lf ms / %.1lf ms", neoInstance->timer_total.lastMeasuredTime, neoInstance->timer_total.avgMeasuredTime(), neoInstance->timer_total.highestMeasuredTime);
			BWAPI::Broodwar->drawTextScreen(columnXStart[3], getNextColumnY(nextColumnY[3]), "Drawinfo: %.1lf ms / %.1lf ms / %.1lf ms", neoInstance->timer_drawinfo.lastMeasuredTime, neoInstance->timer_drawinfo.avgMeasuredTime(), neoInstance->timer_drawinfo.highestMeasuredTime);
			BWAPI::Broodwar->drawTextScreen(columnXStart[3], getNextColumnY(nextColumnY[3]), "Managequeue: %.1lf ms / %.1lf ms / %.1lf ms", neoInstance->timer_managequeue.lastMeasuredTime, neoInstance->timer_managequeue.avgMeasuredTime(), neoInstance->timer_managequeue.highestMeasuredTime);
			BWAPI::Broodwar->drawTextScreen(columnXStart[3], getNextColumnY(nextColumnY[3]), "Buildbuildings: %.1lf ms / %.1lf ms / %.1lf ms", neoInstance->timer_buildbuildings.lastMeasuredTime, neoInstance->timer_buildbuildings.avgMeasuredTime(), neoInstance->timer_buildbuildings.highestMeasuredTime);
			BWAPI::Broodwar->drawTextScreen(columnXStart[3], getNextColumnY(nextColumnY[3]), "Unitlogic: %.1lf ms / %.1lf ms / %.1lf ms", neoInstance->timer_unitlogic.lastMeasuredTime, neoInstance->timer_unitlogic.avgMeasuredTime(), neoInstance->timer_unitlogic.highestMeasuredTime);
			BWAPI::Broodwar->drawTextScreen(columnXStart[3], getNextColumnY(nextColumnY[3]), "Marines: %.1lf ms / %.1lf ms / %.1lf ms", neoInstance->timer_marinelogic.lastMeasuredTime, neoInstance->timer_marinelogic.avgMeasuredTime(), neoInstance->timer_marinelogic.highestMeasuredTime);

		}

		if (s.enableEnemyOverlay) {

			//BWAPI::Broodwar->sendText("Sizes: %u, %u", unitManager.getKnownEnemies().size(), unitManager.getEnemyUnitsByType().size());

			for (auto &u : unitManager.getVisibleEnemies()) {
				drawUnitBox(u->lastPosition, u->lastType, ENEMYCOLOR);
				BWAPI::Broodwar->drawTextMap(u->lastPosition + BWAPI::Position(u->lastType.tileSize() / 2) + BWAPI::Position(10, 10), "%s", noRaceName(u->lastType.c_str()));
			}

			for (auto &u : unitManager.getNonVisibleEnemies()) {
				drawUnitBox(u->lastPosition, u->lastType, ENEMYCOLOR);
				BWAPI::Broodwar->drawTextMap(u->lastPosition + BWAPI::Position(u->lastType.tileSize() / 2) + BWAPI::Position(10, 10), "%s", noRaceName(u->lastType.c_str()));
			}

			for (auto &u : unitManager.getInvalidatedEnemies()) {
				drawUnitBox(u->lastPosition, u->lastType, INVALIDENEMYCLR);
				BWAPI::Broodwar->drawTextMap(u->lastPosition + BWAPI::Position(u->lastType.tileSize() / 2) + BWAPI::Position(10, 10), "%s", noRaceName(u->lastType.c_str()));
			}

			for (auto &ut : unitManager.getEnemyUnitsByType())
				BWAPI::Broodwar->drawTextScreen(columnXStart[2], getNextColumnY(nextColumnY[2]), "%s: %u", noRaceName(ut.first.c_str()), ut.second.size());

			for (auto &u : unitManager.getFriendlyUnitsByType(BWAPI::UnitTypes::Terran_Marine)) {
				std::shared_ptr <EnemyData> u2 = unitManager.getBestTarget(u);
				if (u2)
					BWAPI::Broodwar->drawLineMap(u->getPosition(), u2->lastPosition, BWAPI::Colors::Red);
				else
					BWAPI::Broodwar->drawTextMap(u->getPosition(), "?");
			}

			for (auto &u : unitManager.getFriendlyUnitsByType(BWAPI::UnitTypes::Terran_Ghost)) {
				std::shared_ptr <EnemyData> u2 = unitManager.getBestTarget(u);
				if (u2)
					BWAPI::Broodwar->drawLineMap(u->getPosition(), u2->lastPosition, BWAPI::Colors::Red);
				else
					BWAPI::Broodwar->drawTextMap(u->getPosition(), "?");
			}
		}

		if (s.enableHealthBars) {
			for (auto &u : BWAPI::Broodwar->self()->getUnits())
				if(u->isVisible())
					drawBars(u->getPosition(), u->getType(), u->getHitPoints(), u->getShields(), u->getEnergy(), u->getLoadedUnits().size(), u->getResources(), 5000, u->getType().isSpellcaster());

			for (auto &b : baseManager.getAllBases())
				for (auto &mf : b.mineralMiners)
					drawBars(mf.first->getPosition(), mf.first->getType(), 0, 0, 0, 0, mf.first->getResources(), mf.first->getInitialResources(), false);

			for (auto &u : unitManager.getVisibleEnemies())
				drawBars(u->lastPosition, u->lastType, u->expectedHealth(), u->expectedShields(), 0, 0, 0, 0, false);
			for (auto &u : unitManager.getNonVisibleEnemies())
				drawBars(u->lastPosition, u->lastType, u->expectedHealth(), u->expectedShields(), 0, 0, 0, 0, false);
		}

		if (s.enableDeathMatrix) {
			int mx = BWAPI::Broodwar->mapWidth() * 4, my = BWAPI::Broodwar->mapHeight() * 4;
			for (int x = 0; x < mx; ++x) {
				for (int y = 0; y < my; ++y) {
					int dg = deathMatrixGround[y*deathMatrixSideLen + x];
					int da = deathMatrixAir[y*deathMatrixSideLen + x];
					if (!dg && !da)
						continue;
					BWAPI::Color cg = BWAPI::Colors::Grey, ca = BWAPI::Colors::Grey;

					if (dg && dg <= 10)
						cg = BWAPI::Colors::Green;
					else if (dg && dg <= 30)
						cg = BWAPI::Colors::Yellow;
					else if (dg && dg <= 100)
						cg = BWAPI::Colors::Orange;
					else if (dg) cg = BWAPI::Colors::Red;

					if (da && da <= 10)
						ca = BWAPI::Colors::Green;
					else if (da && da <= 30)
						ca = BWAPI::Colors::Yellow;
					else if (da && da <= 100)
						ca = BWAPI::Colors::Orange;
					else if (da) ca = BWAPI::Colors::Red;

					BWAPI::Broodwar->drawTriangleMap(BWAPI::Position(x * 8, y * 8), BWAPI::Position(x * 8 + 6, y * 8), BWAPI::Position(x * 8, y * 8 + 6), cg, false);
					BWAPI::Broodwar->drawTriangleMap(BWAPI::Position(x * 8 + 7, y * 8 + 7), BWAPI::Position(x * 8 + 7, y * 8), BWAPI::Position(x * 8, y * 8 + 7), ca, false);
				}
			}
		}

		if (s.enableBaseOverlay) {
			BWAPI::Position offset(BWAPI::UnitTypes::Terran_Command_Center.dimensionLeft(), BWAPI::UnitTypes::Terran_Command_Center.dimensionUp());
			for (auto &b : baseManager.getAllBases()) {
				auto sq = b.getNoBuildRegion();
				BWAPI::Broodwar->drawBoxMap((BWAPI::Position)sq.first, (BWAPI::Position)sq.second - BWAPI::Position(-1, -1), BWAPI::Colors::Cyan, false);

				if (b.redAlert == -1)
					BWAPI::Broodwar->drawTextMap(b.resourceDepot->getPosition() - offset, "Income: %c%u %c%u", BWAPI::Text::Blue, b.calculateIncome().minerals, BWAPI::Text::Green, b.calculateIncome().gas);
				else
					BWAPI::Broodwar->drawTextMap(b.resourceDepot->getPosition() - offset, "%cRED ALERT!", BWAPI::Text::Red);
				
				FastAPproximation fap;
				std::map <BWAPI::UnitType, int> unitCountsFriendly, unitCountsEnemy;
				auto state = fap.getState();
				
				int yStart = 220, y = yStart;
				for (auto &u : *state.first)
					++unitCountsFriendly[u.unitType];
				for (auto &u : *state.second)
					++unitCountsEnemy[u.unitType];

				for (auto &e : unitCountsFriendly) {
					BWAPI::Broodwar->drawTextScreen(150, y, "%s, %d", e.first.c_str(), e.second);
					y += 20;
				}

				y = yStart;

				for (auto &e : unitCountsEnemy) {
					BWAPI::Broodwar->drawTextScreen(250, y, "%s, %d", e.first.c_str(), e.second);
					y += 20;
				}

				for (auto &m : b.mineralMiners) {
					drawBuildingBox(m.first->getTilePosition(), BWAPI::UnitTypes::Resource_Mineral_Field, BWAPI::Colors::Blue);
					BWAPI::Broodwar->drawTextMap(m.first->getPosition() - BWAPI::Position(10, 5), "%u/2", m.second.size());
					for (auto &w : m.second) {
						if (!w->isVisible())
							continue;
						BWAPI::Broodwar->drawLineMap(w->getPosition(), m.first->getPosition(), BWAPI::Colors::Orange);
						BWAPI::Broodwar->drawLineMap(w->getPosition(), b.resourceDepot->getPosition(), BWAPI::Colors::Orange);
					}
				}

				for (auto &g : b.gasMiners) {
					drawBuildingBox(g.first->getTilePosition(), BWAPI::UnitTypes::Resource_Vespene_Geyser, BWAPI::Colors::Green);
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
				drawBuildingBox(f.first, f.second, BWAPI::Colors::Orange);
			}
		}

		if (s.enableNukeOverlay && unitManager.getNumArmedSilos()) {
			for (auto &u : unitManager.getFriendlyUnitsByType(BWAPI::UnitTypes::Terran_Ghost)) {
				auto loc = unitManager.getBestNuke(u);
				if (loc != BWAPI::Positions::None)
					BWAPI::Broodwar->drawLineMap(u->getPosition(), loc, BWAPI::Colors::Green);
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
			BWAPI::Broodwar->drawTextScreen(columnXStart[5], getNextColumnY(nextColumnY[5]), "Combat sim:   %d", res.shortLosses);
			BWAPI::Broodwar->drawTextScreen(columnXStart[5], getNextColumnY(nextColumnY[5]), "Presim  units: %3u %3u Presim scores:  %5d %5d", res.presim.unitCounts.first, res.presim.unitCounts.second, res.presim.scores.first, res.presim.scores.second);
			BWAPI::Broodwar->drawTextScreen(columnXStart[5], getNextColumnY(nextColumnY[5]), "Shrtsim units: %3u %3u Shrtsim scores: %5d %5d", res.shortsim.unitCounts.first, res.shortsim.unitCounts.second, res.shortsim.scores.first, res.shortsim.scores.second);
			BWAPI::Broodwar->drawTextScreen(columnXStart[5], getNextColumnY(nextColumnY[5]), "Postsim units: %3u %3u Postsim scores: %5d %5d", res.postsim.unitCounts.first, res.postsim.unitCounts.second, res.postsim.scores.first, res.postsim.scores.second);
		}

		drawBotName();

		// for (unsigned i = 0; i < _allBases.size(); ++ i) {
		// 	Broodwar->drawBoxMap(Position(_allBases[i]->Location()), Position(Position(_allBases[i]->Location()) + Position(UnitTypes::Terran_Command_Center.tileSize())), Colors::Grey);
		// 	Broodwar->drawTextMap(Position(_allBases[i]->Location()), "Base #%u", i);
		// }
	}

	int DrawingManager::getNextColumnY(int &colY) {
		return (colY += 12) - 12;
	}

}
