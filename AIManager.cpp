#include "AIManager.h"
#include "ModularNN.h"

#include "BWAPI.h"

#include <fstream>

#include "SupplyManager.h"
#include "Util.h"

Neolib::AIManager aiManager;

float minerals, gas, maxSupply, usedSupply, mapSizeX, mapSizeY, gameTime;
int enemyRace;

int failedSourceUnits, attemptedSourceUnits;
int failedTargetUnits, attemptedTargetUnits;

constexpr int unitIDSize = 16, abilityIDSize = 2;

constexpr int commandSize =
	1 +				// Unit x location
	1 +				// Unit y location
	1 +				// Advance to next frame?
	abilityIDSize +	// Ability id
	abilityIDSize + // Ability argument
	1 +				// x location
	1 +				// y location
	1 +				// didTargetGround
	0;

std::uniform_real_distribution<float> dist(0, 1);

bool parseBit(float f) {
	return f > dist(mt);
}

float genBit(bool b) {
	return 0.0f + b;
}

template <unsigned nBits>
std::vector <float> genBits(unsigned i) {
	std::vector <float> result;

	while (i) {
		result.push_back(genBit(i & 1));
		i >>= 1;
	}

	return result;
}

template <unsigned nBits>
unsigned parseBits(std::vector <float> &bits) {
	unsigned result = 0;

	for (unsigned i = 0; i < nBits; ++i) {
		result <<= 1;
		result |= (unsigned)parseBit(bits.back());
		bits.pop_back();
	}

	return result;
}

void append(std::vector <float> &v, std::vector <float> add) {
	for (auto &val : add)
		v.push_back(val);
}

void append(std::vector <float> &v, float f) {
	v.push_back(f);
}

template <int bitsToSplice>
std::vector <float> splice(std::vector <float> v) {
	std::vector <float> out;
	
	for (int i = 0; i < bitsToSplice; ++i)
		out.push_back(v[v.size() - i - 1]);

	for (int i = 0; i < bitsToSplice; ++i)
		v.pop_back();

	return out;
}

void setStaticFrameData(std::vector <float> &v) {
	minerals = (float)BWAPI::Broodwar->self()->minerals() / 2000;
	gas = (float)BWAPI::Broodwar->self()->minerals() / 2000;
	maxSupply = (float)supplyManager.availableSupply()(BWAPI::Broodwar->self()->getRace()) / 400;
	usedSupply = (float)supplyManager.usedSupply()(BWAPI::Broodwar->self()->getRace()) / 400;
	mapSizeX = (float)BWAPI::Broodwar->mapWidth() / 256;
	mapSizeY = (float)BWAPI::Broodwar->mapHeight() / 256;
	gameTime = (float)BWAPI::Broodwar->getFrameCount() / 86400;
	auto er = BWAPI::Broodwar->enemy()->getRace(); enemyRace = er > 5 ? 3 : er;

	for (auto &val : { minerals, gas, maxSupply, usedSupply, mapSizeX, mapSizeY, gameTime })
		append(v, val);
	append(v, genBits<2>(enemyRace));
}

void appendUnitData(std::vector <float> &v, BWAPI::Unit u = nullptr) {
	append(v, genBit(u != nullptr));
	append(v, genBit((u != nullptr) && u->getPlayer()->isEnemy(BWAPI::Broodwar->self())));
	append(v, genBit((u != nullptr) && u->getPlayer() == BWAPI::Broodwar->self()));
	append(v, genBits<abilityIDSize>(u == nullptr ? -1 : u->getOrder().getID()));
	append(v, (u == nullptr ? 0.0f : (float)u->getOrderTargetPosition().x / (256 * 32)));
	append(v, (u == nullptr ? 0.0f : (float)u->getOrderTargetPosition().y / (256 * 32)));
	append(v, genBits<abilityIDSize>(u == nullptr ? -1 : u->getType().getID()));
	append(v, u == nullptr ? 0.0f : (float)u->getHitPoints() / u->getType().maxHitPoints());
	append(v, u == nullptr ? 0.0f : (u->getType().maxShields() ? (float)u->getShields() / u->getType().maxShields() : 0.0f));
	append(v, u == nullptr ? 0.0f : (float)u->getEnergy() / 250);
	append(v, u == nullptr ? 0.0f : (float)std::max(u->getGroundWeaponCooldown(), u->getAirWeaponCooldown()) / BWAPI::UnitTypes::Zerg_Devourer.airWeapon().damageCooldown());
	append(v, u == nullptr ? 0.0f : (float)u->getAngle() / (2.0f * 3.1415f));
	append(v, u == nullptr ? 0.0f : (float)u->getPosition().x / (256 * 32));
	append(v, u == nullptr ? 0.0f : (float)u->getPosition().y / (256 * 32));
	append(v, u == nullptr ? 1.0f : (float)u->getRemainingBuildTime() / u->getType().buildTime());
	// Ability values like plague, dmatrix, and so on should be added here
	append(v, u == nullptr ? 0.0f : genBit(u->isCloaked()));
	append(v, u == nullptr ? 0.0f : genBit(u->isDetected()));
	append(v, u == nullptr ? 0.0f : genBit(u->isBurrowed()));
	append(v, u == nullptr ? 0.0f : genBit(u->isFlying()));
	append(v, genBit(u != nullptr && u->isCarryingMinerals()));
	append(v, genBit(u != nullptr && u->isCarryingGas()));
}

bool parseAndExecuteAction(std::vector <float> &command) {
	//if (parseBit(splice<1>(command)[0]))
		//return false;

	auto unitX = splice<1>(command)[0] * (256 * 32);
	auto unitY = splice<1>(command)[0] * (256 * 32);
	auto abilityID = parseBits<abilityIDSize>(splice<abilityIDSize>(command));
	auto abilityArg = parseBits<abilityIDSize>(splice<abilityIDSize>(command));
	auto abilityTargetX = splice<1>(command)[0];
	auto abilityTargetY = splice<1>(command)[0];
	auto didTargetGround = parseBit(splice<1>(command)[0]);

	auto u = BWAPI::Broodwar->getClosestUnit(BWAPI::Position((int)unitX, (int)unitY), BWAPI::Filter::IsOwned && !BWAPI::Filter::IsBuilding);
	auto targetPosition = BWAPI::Position((int)(abilityTargetX * 32.0f * 256.0f), (int)(abilityTargetY * 32.0f * 256.0f));

	if (u != nullptr) {
		if (abilityID == 0) { // Attack
			auto targetUnit = BWAPI::Broodwar->getClosestUnit(BWAPI::Position((int)abilityTargetX, (int)abilityTargetY));
			if (didTargetGround) {
				BWAPI::Broodwar->drawLineMap(u->getPosition(), targetPosition, BWAPI::Colors::Orange);
				u->attack(targetPosition);
			}
			else if (targetUnit != nullptr) {
				BWAPI::Broodwar->drawLineMap(u->getPosition(), targetUnit->getPosition(), BWAPI::Colors::Red);
				u->attack(targetUnit);
			}
		}
		else if (abilityID == 1) { // Move
			BWAPI::Broodwar->drawLineMap(u->getPosition(), targetPosition, BWAPI::Colors::Green);
			u->move(targetPosition);
		}
		else if (abilityID == 2) { // Gather
			auto targetUnit = BWAPI::Broodwar->getClosestUnit(BWAPI::Position((int)abilityTargetX, (int)abilityTargetY), BWAPI::Filter::IsMineralField || BWAPI::Filter::IsRefinery);
			if (targetUnit != nullptr) {
				BWAPI::Broodwar->drawLineMap(u->getPosition(), targetUnit->getPosition(), BWAPI::Colors::Cyan);
				u->gather(targetUnit);
			}
		}
		else if (abilityID == 3) { // Return cargo
			BWAPI::Broodwar->drawCircleMap(u->getPosition(), 10, BWAPI::Colors::Cyan, false);
			u->returnCargo();
		}
		else if (abilityID == 4) {

		}
	}

	return true;
}

namespace Neolib {
	void AIManager::onFrame() {
		static ModularNN model([&]() {
#ifndef SSCAIT
			int i = 0;
			std::ifstream ifs("current.txt");
			if (ifs.is_open())
				ifs >> i;
			ifs.close();
			std::ofstream("current.txt") << i + 1;

			neoInstance->currI = i;

			std::ifstream model("bwapi-data/AI/" + std::to_string(i++));

			if (!model.is_open())
				exit(0);
			return model;
#else
			return std::ifstream("bwapi-data/AI/model.nn");
#endif
		}
		());

		std::vector <float> nnFrameData;
		setStaticFrameData(nnFrameData);

		failedSourceUnits = 0, attemptedSourceUnits = 0,
		failedTargetUnits = 0, attemptedTargetUnits = 0;

		for (auto &u : BWAPI::Broodwar->getAllUnits()) {
			std::vector <float> unitData = nnFrameData;
			appendUnitData(unitData, u);
			model.run(unitData);
		}

		appendUnitData(nnFrameData);
		std::vector <float> output(commandSize);

		for (int i = 0; i < 40 && BWAPI::Broodwar->getFrameCount() != 0; ++i) { // Max commands per frame
			output = model.run(nnFrameData);
			//std::string s = "";
			//for (auto &v : output)
				//s += std::to_string(v) + " ";
			//BWAPI::Broodwar->sendText(s.c_str());
			if (!parseAndExecuteAction(output))
				break;
		}
	}
}
