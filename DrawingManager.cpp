#include "DrawingManager.h"

#include "Neohuman.h"
#include "SupplyManager.h"
#include "UnitManager.h"
#include "ResourceManager.h"
#include "BuildingQueue.h"
#include "MapManager.h"

#include "BWAPI.h"
#include "BWEM/bwem.h"

const std::vector <int> columnXStart = { 0, 140, 280, 430, 510 };
const std::vector <int> columnYStart = { 0, 0, 0, 200, 16 };

#define SHIELDTEXT		(char)BWAPI::Text::Blue
#define HEALTHTEXT		(char)BWAPI::Text::Green
#define ENERGYTEXT		(char)BWAPI::Text::Purple

#define BUILDINGTEXT	(char)BWAPI::Text::Green
#define PLACINGTEXT		(char)BWAPI::Text::Cyan

#define SATURATIONCOLOR BWAPI::Colors::Blue
#define WORKERCOLOR		BWAPI::Colors::White

#define BUILDINGCOLOR	BWAPI::Colors::Green
#define PLACINGCOLOR	BWAPI::Colors::Cyan

#define ENEMYCOLOR		BWAPI::Colors::Red

#define PROTOSSTEXT		BWAPI::Text::Teal
#define TERRANTEXT		BWAPI::Text::Green
#define ZERGTEXT		BWAPI::Text::Red

namespace Neolib {

	DrawingManager::DrawingManager(const DrawingManager &other) : enableTopInfo(other.enableTopInfo), enableResourceOverlay(other.enableResourceOverlay), enableBWEMOverlay(other.enableBWEMOverlay),
		enableComsatInfo(other.enableComsatInfo), enableListBuildingQueue(other.enableListBuildingQueue), enableSaturationInfo(other.enableSaturationInfo),
		enableTimerInfo(other.enableTimerInfo), enableEnemyOverlay(other.enableEnemyOverlay) {

	}

	DrawingManager::DrawingManager(bool enableTopInfo, bool enableResourceOverlay, bool enableBWEMOverlay, bool enableComsatInfo, bool enableListBuildingQueue, bool enableSaturationInfo, bool enableTimerInfo, bool enableEnemyOverlay) :
		enableTopInfo(enableTopInfo), enableResourceOverlay(enableResourceOverlay), enableBWEMOverlay(enableBWEMOverlay),
		enableComsatInfo(enableComsatInfo), enableListBuildingQueue(enableListBuildingQueue), enableSaturationInfo(enableSaturationInfo),
		enableTimerInfo(enableTimerInfo), enableEnemyOverlay(enableEnemyOverlay) {

	}

	void DrawingManager::onFrame() const {
		std::vector <int> nextColumnY = columnYStart;

		if (enableTopInfo) {

			BWAPI::Broodwar->drawTextScreen(columnXStart[1], getNextColumnY(nextColumnY[1]), "FPS: %c%d", BWAPI::Broodwar->getFPS() >= 30 ? BWAPI::Text::Green : BWAPI::Text::Red, BWAPI::Broodwar->getFPS());
			BWAPI::Broodwar->drawTextScreen(columnXStart[1], getNextColumnY(nextColumnY[1]), "Average FPS: %c%f", BWAPI::Broodwar->getFPS() >= 30 ? BWAPI::Text::Green : BWAPI::Text::Red, BWAPI::Broodwar->getAverageFPS());
			BWAPI::Broodwar->drawTextScreen(columnXStart[1], getNextColumnY(nextColumnY[1]), "%u marines swarming you", unitManager.countFriendly(BWAPI::UnitTypes::Terran_Marine));
			BWAPI::Broodwar->drawTextScreen(columnXStart[1], getNextColumnY(nextColumnY[1]), "I have %d barracks!", unitManager.countFriendly(BWAPI::UnitTypes::Terran_Barracks));
			BWAPI::Broodwar->drawTextScreen(columnXStart[1], getNextColumnY(nextColumnY[1]), "I have %d APM!", BWAPI::Broodwar->getAPM());

		}

		if (enableComsatInfo) {
			std::string s = "Comsats (" + std::to_string(unitManager.getFriendlyUnitsByType(BWAPI::UnitTypes::Terran_Comsat_Station).size()) + "): " + ENERGYTEXT;
			for (auto &c : unitManager.getFriendlyUnitsByType(BWAPI::UnitTypes::Terran_Comsat_Station))
				s += std::to_string(c->getEnergy()) + " ";

			BWAPI::Broodwar->drawTextScreen(columnXStart[0], getNextColumnY(nextColumnY[0]), s.c_str());
		}

		if (enableResourceOverlay) {

			BWAPI::Broodwar->drawTextScreen(450, 16, "%d", resourceManager.getSpendableResources().minerals);
			BWAPI::Broodwar->drawTextScreen(480, 16, "%d", resourceManager.getSpendableResources().gas);

			BWAPI::Broodwar->drawTextScreen(columnXStart[4], getNextColumnY(nextColumnY[4]), "%c%d/%d, %d, %d", PROTOSSTEXT, supplyManager.usedSupply().protoss, supplyManager.availableSupply().protoss, supplyManager.wantedAdditionalSupply().protoss, supplyManager.wantedSupplyOverhead().protoss);
			BWAPI::Broodwar->drawTextScreen(columnXStart[4], getNextColumnY(nextColumnY[4]), "%c%d/%d, %d, %d", TERRANTEXT, supplyManager.usedSupply().terran, supplyManager.availableSupply().terran, supplyManager.wantedAdditionalSupply().terran, supplyManager.wantedSupplyOverhead().terran);
			BWAPI::Broodwar->drawTextScreen(columnXStart[4], getNextColumnY(nextColumnY[4]), "%c%d/%d, %d, %d", ZERGTEXT, supplyManager.usedSupply().zerg, supplyManager.availableSupply().zerg, supplyManager.wantedAdditionalSupply().zerg, supplyManager.wantedSupplyOverhead().zerg);

		}

		if (enableBWEMOverlay) {

			BWEM::utils::gridMapExample(BWEMMap);
			BWEM::utils::drawMap(BWEMMap);

		}

		if (enableListBuildingQueue) {

			BWAPI::Broodwar->drawTextScreen(columnXStart[0], getNextColumnY(nextColumnY[0]), "%u buildings in queue", buildingQueue.buildingsQueued().size());

			for (auto &o : buildingQueue.buildingsQueued()) {
				BWAPI::Broodwar->drawTextScreen(columnXStart[0], getNextColumnY(nextColumnY[0]), "%c%s", o.second ? BUILDINGTEXT : PLACINGTEXT, noRaceName(o.first.second.c_str()));
				BWAPI::Broodwar->drawBoxMap(BWAPI::Position(o.first.third), BWAPI::Position((BWAPI::Position)o.first.third + (BWAPI::Position)o.first.second.tileSize()), o.second ? BUILDINGCOLOR : PLACINGCOLOR, false);
				BWAPI::Broodwar->drawTextMap(BWAPI::Position(o.first.third) + BWAPI::Position(10, 10), "%s", noRaceName(o.first.second.getName().c_str()));
			}
		}

		if (enableSaturationInfo) {

			for (auto &u : unitManager.getFriendlyUnitsByType(BWAPI::UnitTypes::Protoss_Nexus)) {
				BWAPI::Broodwar->drawEllipseMap(u->getPosition(), SATRUATION_RADIUS + BWAPI::UnitTypes::Protoss_Nexus.tileSize().x * 32, SATRUATION_RADIUS + BWAPI::UnitTypes::Protoss_Nexus.tileSize().y * 32, SATURATIONCOLOR);
				auto workers = u->getUnitsInRadius(SATRUATION_RADIUS, (BWAPI::Filter::IsGatheringMinerals));
				auto refineries = u->getUnitsInRadius(SATRUATION_RADIUS, (BWAPI::Filter::GetType == BWAPI::UnitTypes::Protoss_Assimilator));
				auto mineralFields = u->getUnitsInRadius(SATRUATION_RADIUS, (BWAPI::Filter::IsMineralField));
				BWAPI::Broodwar->drawTextMap(u->getPosition() + BWAPI::Position(0, 30), "%cWorkers: %d/%d", WORKERCOLOR, workers.size(), 2 * mineralFields.size());
			}

			for (auto &u : unitManager.getFriendlyUnitsByType(BWAPI::UnitTypes::Terran_Command_Center)) {
				BWAPI::Broodwar->drawEllipseMap(u->getPosition(), SATRUATION_RADIUS + BWAPI::UnitTypes::Terran_Command_Center.tileSize().x * 32, SATRUATION_RADIUS + BWAPI::UnitTypes::Terran_Command_Center.tileSize().y * 32, SATURATIONCOLOR);
				auto workers = u->getUnitsInRadius(SATRUATION_RADIUS, (BWAPI::Filter::IsGatheringMinerals));
				auto refineries = u->getUnitsInRadius(SATRUATION_RADIUS, (BWAPI::Filter::GetType == BWAPI::UnitTypes::Terran_Refinery));
				auto mineralFields = u->getUnitsInRadius(SATRUATION_RADIUS, (BWAPI::Filter::IsMineralField));
				BWAPI::Broodwar->drawTextMap(u->getPosition() + BWAPI::Position(0, 30), "%cWorkers: %d/%d", WORKERCOLOR, workers.size(), 2 * mineralFields.size());
			}

			for (auto &u : unitManager.getFriendlyUnitsByType(BWAPI::UnitTypes::Zerg_Hatchery)) {
				BWAPI::Broodwar->drawEllipseMap(u->getPosition(), SATRUATION_RADIUS + BWAPI::UnitTypes::Zerg_Hatchery.tileSize().x * 32, SATRUATION_RADIUS + BWAPI::UnitTypes::Zerg_Hatchery.tileSize().y * 32, SATURATIONCOLOR);
				auto workers = u->getUnitsInRadius(SATRUATION_RADIUS, (BWAPI::Filter::IsGatheringMinerals));
				auto refineries = u->getUnitsInRadius(SATRUATION_RADIUS, (BWAPI::Filter::GetType == BWAPI::UnitTypes::Zerg_Extractor));
				auto mineralFields = u->getUnitsInRadius(SATRUATION_RADIUS, (BWAPI::Filter::IsMineralField));
				BWAPI::Broodwar->drawTextMap(u->getPosition() + BWAPI::Position(0, 30), "%cWorkers: %d/%d", WORKERCOLOR, workers.size(), 2 * mineralFields.size());
			}

		}

		if (enableTimerInfo) {

			BWAPI::Broodwar->drawTextScreen(columnXStart[3], getNextColumnY(nextColumnY[3]), "Frame times [last/avg/high]:");
			BWAPI::Broodwar->drawTextScreen(columnXStart[3], getNextColumnY(nextColumnY[3]), "Total: %.1lf ms / %.1lf ms / %.1lf ms", neoInstance.timer_total.lastMeasuredTime, neoInstance.timer_total.avgMeasuredTime(), neoInstance.timer_total.highestMeasuredTime);
			BWAPI::Broodwar->drawTextScreen(columnXStart[3], getNextColumnY(nextColumnY[3]), "Drawinfo: %.1lf ms / %.1lf ms / %.1lf ms", neoInstance.timer_drawinfo.lastMeasuredTime, neoInstance.timer_drawinfo.avgMeasuredTime(), neoInstance.timer_drawinfo.highestMeasuredTime);
			BWAPI::Broodwar->drawTextScreen(columnXStart[3], getNextColumnY(nextColumnY[3]), "Managequeue: %.1lf ms / %.1lf ms / %.1lf ms", neoInstance.timer_managequeue.lastMeasuredTime, neoInstance.timer_managequeue.avgMeasuredTime(), neoInstance.timer_managequeue.highestMeasuredTime);
			BWAPI::Broodwar->drawTextScreen(columnXStart[3], getNextColumnY(nextColumnY[3]), "Buildbuildings: %.1lf ms / %.1lf ms / %.1lf ms", neoInstance.timer_buildbuildings.lastMeasuredTime, neoInstance.timer_buildbuildings.avgMeasuredTime(), neoInstance.timer_buildbuildings.highestMeasuredTime);
			BWAPI::Broodwar->drawTextScreen(columnXStart[3], getNextColumnY(nextColumnY[3]), "Unitlogic: %.1lf ms / %.1lf ms / %.1lf ms", neoInstance.timer_unitlogic.lastMeasuredTime, neoInstance.timer_unitlogic.avgMeasuredTime(), neoInstance.timer_unitlogic.highestMeasuredTime);
			BWAPI::Broodwar->drawTextScreen(columnXStart[3], getNextColumnY(nextColumnY[3]), "Marines: %.1lf ms / %.1lf ms / %.1lf ms", neoInstance.timer_marinelogic.lastMeasuredTime, neoInstance.timer_marinelogic.avgMeasuredTime(), neoInstance.timer_marinelogic.highestMeasuredTime);

		}

		if (enableEnemyOverlay) {

			for (auto &u : unitManager.getKnownEnemies()) {
				BWAPI::Broodwar->drawBoxMap(u.second.first - BWAPI::Position(u.second.second.tileSize()) / 2, u.second.first + BWAPI::Position(u.second.second.tileSize()) / 2, ENEMYCOLOR);
				BWAPI::Broodwar->drawTextMap(u.second.first + BWAPI::Position(u.second.second.tileSize() / 2) + BWAPI::Position(10, 10), "%s", noRaceName(u.second.second.c_str()));
			}

			for (auto &ut : unitManager.getEnemyUnitsByType())
				BWAPI::Broodwar->drawTextScreen(columnXStart[2], getNextColumnY(nextColumnY[2]), "%s: %u", noRaceName(ut.first.c_str()), ut.second.size());

		}

		// for (unsigned i = 0; i < _allBases.size(); ++ i) {
		// 	Broodwar->drawBoxMap(Position(_allBases[i]->Location()), Position(Position(_allBases[i]->Location()) + Position(UnitTypes::Terran_Command_Center.tileSize())), Colors::Grey);
		// 	Broodwar->drawTextMap(Position(_allBases[i]->Location()), "Base #%u", i);
		// }
	}

	int DrawingManager::getNextColumnY(int &colY) {
		return (colY += 12) - 12;
	}

}
