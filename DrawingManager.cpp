#include "DrawingManager.h"

#include "Neohuman.h"
#include "SupplyManager.h"
#include "UnitManager.h"
#include "ResourceManager.h"
#include "MapManager.h"
#include "FAP.h"

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
			BWAPI::Broodwar->drawTextScreen(columnXStart[1], getNextColumnY(nextColumnY[1]), "FPS: %c%d", BWAPI::Broodwar->getFPS() >= 30 ? BWAPI::Text::Green : BWAPI::Text::Red, BWAPI::Broodwar->getFPS());
			BWAPI::Broodwar->drawTextScreen(columnXStart[1], getNextColumnY(nextColumnY[1]), "Average FPS: %c%f", BWAPI::Broodwar->getAverageFPS() >= 30 ? BWAPI::Text::Green : BWAPI::Text::Red, BWAPI::Broodwar->getAverageFPS());
			BWAPI::Broodwar->drawTextScreen(columnXStart[1], getNextColumnY(nextColumnY[1]), "Frame: %d", BWAPI::Broodwar->getFrameCount());
			BWAPI::Broodwar->drawTextScreen(columnXStart[1], getNextColumnY(nextColumnY[1]), "Nukes: %c%u Armed %c%u Arming %c%u Unarmed", BWAPI::Text::BrightRed, unitManager.getNumArmedSilos(), BWAPI::Text::Orange, unitManager.getNumArmingSilos(), BWAPI::Text::Yellow, unitManager.getNumUnarmedSilos());
			BWAPI::Broodwar->drawTextScreen(columnXStart[1], getNextColumnY(nextColumnY[1]), "%c%d%c nukes launched", BWAPI::Text::Green, unitManager.getLaunchedNukeCount(), BWAPI::Text::Default);
			BWAPI::Broodwar->drawTextScreen(columnXStart[1], getNextColumnY(nextColumnY[1]), "I have %d barracks!", unitManager.countFriendly(BWAPI::UnitTypes::Terran_Barracks));
			BWAPI::Broodwar->drawTextScreen(columnXStart[1], getNextColumnY(nextColumnY[1]), "I have %d APM!", BWAPI::Broodwar->getAPM());
			BWAPI::Broodwar->drawTextScreen(columnXStart[1], getNextColumnY(nextColumnY[1]), "Model #%d", neoInstance->currI);
		}

		if (s.enableBWEMOverlay) {

			BWEM::utils::gridMapExample(BWEM::Map::Instance());
			BWEM::utils::drawMap(BWEM::Map::Instance());

		}

		if (s.enableTimerInfo) {
			BWAPI::Broodwar->drawTextScreen(columnXStart[3], getNextColumnY(nextColumnY[3]), "Frame times [last/avg/high]:");
			BWAPI::Broodwar->drawTextScreen(columnXStart[3], getNextColumnY(nextColumnY[3]), "Total: %.1lf ms / %.1lf ms / %.1lf ms", neoInstance->timer_total.lastMeasuredTime, neoInstance->timer_total.avgMeasuredTime(), neoInstance->timer_total.highestMeasuredTime);
			BWAPI::Broodwar->drawTextScreen(columnXStart[3], getNextColumnY(nextColumnY[3]), "Drawinfo: %.1lf ms / %.1lf ms / %.1lf ms", neoInstance->timer_drawinfo.lastMeasuredTime, neoInstance->timer_drawinfo.avgMeasuredTime(), neoInstance->timer_drawinfo.highestMeasuredTime);
			BWAPI::Broodwar->drawTextScreen(columnXStart[3], getNextColumnY(nextColumnY[3]), "Neural Networks: %.1lf ms / %.1lf ms / %.1lf ms", neoInstance->timer_NN.lastMeasuredTime, neoInstance->timer_NN.avgMeasuredTime(), neoInstance->timer_NN.highestMeasuredTime);
		}

		if (s.enableEnemyOverlay) {
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
		}

		if (s.enableHealthBars) {
			for (auto &u : BWAPI::Broodwar->self()->getUnits())
				if(u->isVisible())
					drawBars(u->getPosition(), u->getType(), u->getHitPoints(), u->getShields(), u->getEnergy(), u->getLoadedUnits().size(), u->getResources(), 5000, u->getType().isSpellcaster());

			for (auto &mf : BWAPI::Broodwar->getStaticMinerals())
					drawBars(mf->getPosition(), mf->getType(), 0, 0, 0, 0, mf->getResources(), mf->getInitialResources(), false);

			for (auto &u : unitManager.getVisibleEnemies())
				drawBars(u->lastPosition, u->lastType, u->expectedHealth(), u->expectedShields(), 0, 0, 0, 0, false);
			for (auto &u : unitManager.getNonVisibleEnemies())
				drawBars(u->lastPosition, u->lastType, u->expectedHealth(), u->expectedShields(), 0, 0, 0, 0, false);
		}

		if (s.enableNukeSpots) {
			for (auto &nd : BWAPI::Broodwar->getNukeDots()) {
				BWAPI::Broodwar->drawLineMap(nd + BWAPI::Position(-100, -100), nd + BWAPI::Position(100, 100), BWAPI::Colors::Yellow);
				BWAPI::Broodwar->drawLineMap(nd + BWAPI::Position(-100, 100), nd + BWAPI::Position(100, -100), BWAPI::Colors::Yellow);
				BWAPI::Broodwar->drawCircleMap(nd, 10 * 32, BWAPI::Colors::Yellow, false);
			}
		}
	}

	int DrawingManager::getNextColumnY(int &colY) {
		return (colY += 12) - 12;
	}
}
