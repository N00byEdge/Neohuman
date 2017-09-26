#include "AIManager.h"
#include "ModularNN.h"

#include "BWAPI.h"

#include <fstream>

#include "SupplyManager.h"

Neolib::AIManager aiManager;

float minerals, gas, maxSupply, usedSupply, mapSizeX, mapSizeY, gameTime;
int enemyRace;

int failedSourceUnits, attemptedSourceUnits;
int failedTargetUnits, attemptedTargetUnits;

constexpr int unitIDSize = 16, abilityIDSize = 1;

constexpr int commandSize =
	1 +				// Advance to next frame?
	unitIDSize +	// Source unit id
	unitIDSize +	// target unit id
	abilityIDSize +	// Ability id
	abilityIDSize + // Ability argument
	1 +				// x location
	1 +				// y location
	1 +				// didTargetGround
	0;


bool parseBit(float f) {
	return f >= 0.5f;
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
	append(v, genBits<unitIDSize>(u == nullptr ? -1 : u->getID()));
	append(v, genBits<abilityIDSize>(u == nullptr ? -1 : u->getOrder().getID()));
	append(v, (u == nullptr ? 0.0f : (float)u->getOrderTargetPosition().x / (256 * 32)));
	append(v, (u == nullptr ? 0.0f : (float)u->getOrderTargetPosition().y / (256 * 32)));
	append(v, genBits<unitIDSize>(u == nullptr ? -1 : u->getOrderTarget() ? u->getOrderTarget()->getID() : -1));
	append(v, genBits<abilityIDSize>(u == nullptr ? -1 : u->getType().getID()));
	append(v, u == nullptr ? 0.0f : (float)u->getHitPoints() / u->getType().maxHitPoints());
	append(v, u == nullptr ? 0.0f : (float)u->getShields() / u->getType().maxShields());
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
}

bool parseAndExecuteAction(std::vector <float> &command) {
	if (splice<1>(command)[0] > 0.5f)
		return false;

	auto unitID = parseBits<unitIDSize>(splice<unitIDSize>(command));
	auto targetID = parseBits<unitIDSize>(splice<unitIDSize>(command));
	auto abilityID = parseBits<abilityIDSize>(splice<abilityIDSize>(command));
	auto abilityArg = parseBits<abilityIDSize>(splice<abilityIDSize>(command));
	auto abilityTargetX = splice<1>(command)[0];
	auto abilityTargetY = splice<1>(command)[0];
	auto didTargetGround = splice<1>(command)[0] > 0.5f;

	auto u = BWAPI::Broodwar->getUnit(unitID);

	++ attemptedSourceUnits;
	if (u == nullptr)
		++failedSourceUnits;

	auto targetPosition = BWAPI::Position((int)(abilityTargetX * 32.0f * 256.0f), (int)(abilityTargetY * 32.0f * 256.0f));
	auto targetUnit = BWAPI::Broodwar->getUnit(targetID);

	if (!didTargetGround)
		++attemptedTargetUnits;
	if (!didTargetGround && targetUnit == nullptr)
		++failedTargetUnits;

	if (u != nullptr) {
		if (abilityID == 0) { // Attack
			if (didTargetGround)
				u->attack(targetPosition);
			else if (targetUnit != nullptr)
					u->attack(targetUnit);
		}
		else if (abilityID == 1) { // Move
			u->move(targetPosition);
		}
	}

	return true;
}

namespace Neolib {
	void AIManager::onFrame() {
		static ModularNN model(std::ifstream("bwapi-data/AI/model.nn"));
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

		for (int i = 0; i < 400; ++i) { // Max commands per frame
			output = model.run(nnFrameData);
			if (!parseAndExecuteAction(output))
				break;
		}
	}
}
