#include <sc2api\sc2_api.h>

#include <iostream>

using namespace sc2;

class Bot : public Agent {
public:
	bool TryBuildStructure(ABILITY_ID ability_type_for_structure, UNIT_TYPEID unit_type = UNIT_TYPEID::TERRAN_SCV) {
		const ObservationInterface* observation = Observation();

		Unit unit_to_build;
		Units units = observation->GetUnits(Unit::Alliance::Self);
		for (const auto& unit : units) {
			for (const auto& order : unit.orders) {
				if (order.ability_id == ability_type_for_structure) {
					return false;
				}
			}

			if (unit.unit_type == unit_type) {
				unit_to_build = unit;
			}
		}

		float rx = GetRandomScalar();
		float ry = GetRandomScalar();

		Actions()->UnitCommand(unit_to_build,
			ability_type_for_structure,
			Point2D(unit_to_build.pos.x + rx * 15.0f, unit_to_build.pos.y + ry * 15.0f));

		return true;
	}

	bool TryBuildSupplyDepot() {
		const ObservationInterface* observation = Observation();

		if (observation->GetFoodUsed() <= observation->GetFoodCap() - 4) {
			return false;
		}

		return TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT);
	}

	bool TryBuildBarracks() {
		const ObservationInterface* observation = Observation();

		if (CountUnitType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) < 1) {
			return false;
		}

		if (CountUnitType(UNIT_TYPEID::TERRAN_BARRACKS) > 2) {
			return false;
		}

		return TryBuildStructure(ABILITY_ID::BUILD_BARRACKS);
	}

	size_t CountUnitType(UNIT_TYPEID unit_type) {
		return Observation()->GetUnits(Unit::Alliance::Self, IsUnit(unit_type)).size();
	}

	bool FindNearestMineralPatch(const Point2D& start, uint64_t& target) {
		Units units = Observation()->GetUnits(Unit::Alliance::Neutral);
		float distance = std::numeric_limits<float>::max();
		for (const auto& u : units) {
			if (u.unit_type == UNIT_TYPEID::NEUTRAL_MINERALFIELD) {
				float d = DistanceSquared2D(u.pos, start);
				if (d < distance) {
					distance = d;
					target = u.tag;
				}
			}
		}

		if (distance == std::numeric_limits<float>::max()) {
			return false;
		}

		return true;
	}

	virtual void OnGameStart() final {
		std::cout << "Hello, bot" << std::endl;
	}

	virtual void OnStep() final {
		TryBuildSupplyDepot();
		TryBuildBarracks();
	}

	virtual void OnUnitIdle(const Unit& unit) final {
		switch (unit.unit_type.ToType()) {
		case UNIT_TYPEID::TERRAN_COMMANDCENTER: {
			Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_SCV);
			break;
		}
		case UNIT_TYPEID::TERRAN_SCV: {
			uint64_t mineral_target;
			if (!FindNearestMineralPatch(unit.pos, mineral_target)) {
				break;
			}
			Actions()->UnitCommand(unit, ABILITY_ID::SMART, mineral_target);
			break;
		}
		case UNIT_TYPEID::TERRAN_BARRACKS: {
			Actions()->UnitCommand(unit, ABILITY_ID::TRAIN_MARINE);
			break;
		}
		case UNIT_TYPEID::TERRAN_MARINE: {
			if (CountUnitType(UNIT_TYPEID::TERRAN_MARINE) > 20) {
				const GameInfo& game_info = Observation()->GetGameInfo();
				Actions()->UnitCommand(unit, ABILITY_ID::ATTACK_ATTACK, game_info.enemy_start_locations.front());
			}
			break;
		}
		default: {
			break;
		}
		}
	}
};

int main(int argc, char* argv[]) {
	Coordinator coordinator;
	coordinator.LoadSettings(argc, argv);

	Bot bot;
	coordinator.SetParticipants({
		CreateParticipant(Race::Terran, &bot),
		CreateComputer(Race::Zerg)
	});

	coordinator.LaunchStarcraft();
	coordinator.StartGame("D:\\Programmation\\GIT\\s2client-api\\maps\\Ladder\\(2)Bel'ShirVestigeLE (Void).SC2Map");

	while (coordinator.Update()) {}

	return 0;
}
