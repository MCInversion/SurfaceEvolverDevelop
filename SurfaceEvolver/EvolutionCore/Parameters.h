#ifndef PARAMETERS_H_
#define PARAMETERS_H_

#include <string>
#include "../Geometry/Geometry.h"
#include "../SDF/Grid.h"

enum class ElementType {
	tri = 0,
	quad = 1
};

struct EvolutionParams {
	float dt = 0.01f;
	float tStop = 1.0f;
	int NSteps = 10;
	unsigned int subdiv = 2;
	ElementType elType = ElementType::tri;

	std::string name = "Sphere";

	Geometry* sourceGeometry = nullptr;

	// ===== output flags ==========
	bool saveStates = false;
	bool printHappenings = false;
	bool printStepOutput = false;
	bool printSolution = false;

	bool writeGenericLog = true;
	bool writeTimeLog = false;
	// =============================
};

struct SphereTestParams {
	float r0 = 1.0f;
	int testId = -1;

	// ===== output flags ==========
	bool writeErrorLog = false;
	// =============================
};

struct MeanCurvatureParams {
	// Laplace-Beltrami ctrl constants:
	float rDecay = 1.0f;
	float C1 = 1.0f;
	float C2 = rDecay;

	bool constant = false;

	// ===== output flags ==========
	bool saveAreaStates = false;
	bool saveCurvatureStates = false;

	bool writeMeanAreaLog = false;
	// =============================
};

struct GradDistanceParams {
	Geometry* targetGeom = nullptr;
	Grid* sdfGrid = nullptr;

	// grad SDF ctrl constants:
	float C = 1.0f;

	bool constant = false;

	// ===== output flags ==========
	bool saveDistanceStates = false;
	bool saveGradientStates = false;
	// =============================
};

#endif