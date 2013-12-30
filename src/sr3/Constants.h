#ifndef SR3_CONSTANTS_H
#define SR3_CONSTANTS_H

namespace Constants {
	const float SolarSystemSpeedCoefficient = 10.0f;
	const float PlanetSizeCoefficient = 500.0f;
	const float LabourProducedByCitizen = 0.1f;

	const unsigned int MaxPopulation = 1000000;
	const unsigned int MinPopulationForColonisation = 400;
	const unsigned int MinPopulationMoneyForColonisation = 10000.0f;
}

enum class SOType {
	Star,
	GasGiant,
	RockyNoAtmosphere,
	RockyOxygen,
	RockyNitrogen,
	RockyCarbonDioxide,
	RockyMethane
};


#endif

