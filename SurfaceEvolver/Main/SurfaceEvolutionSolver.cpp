#include "SurfaceEvolutionSolver.h"

SurfaceEvolutionSolver::SurfaceEvolutionSolver()
{
}

SurfaceEvolutionSolver::SurfaceEvolutionSolver(const SurfaceEvolutionSolver& other)
{
}

SurfaceEvolutionSolver::SurfaceEvolutionSolver(Grid* sdfGrid, uint NSteps, float dt, ElementType type, bool saveStates)
{
	// ===== Evolution params ==============================
	this->NSteps = NSteps; this->saveStates = saveStates;

	this->dt = dt;
	this->sdfGrid = sdfGrid;
	this->type = type;
	// -----------------------------------------------------

	this->init();

	this->evolve();
}

SurfaceEvolutionSolver::~SurfaceEvolutionSolver()
{
}

void SurfaceEvolutionSolver::init()
{
	// ===== Preparing a source geometry for immersion =====
	// -----------------------------------------------------
	// radius
	float r = 0.499f * std::min({ sdfGrid->scale.x, sdfGrid->scale.y, sdfGrid->scale.z });
	if (type == ElementType::tri) {
		uint n = 2;
		evolvedSurface = new IcoSphere(n, r);
	}
	else {
		uint n = 3;
		evolvedSurface = new CubeSphere(n, r);
	}
	Vector3 center = sdfGrid->bbox.getCenter();
	Matrix4 M = Matrix4().makeTranslation(center.x, center.y, center.z);
	evolvedSurface->applyMatrix(M);
	this->NVerts = evolvedSurface->uniqueVertices.size();

	// -------------------------------------------------------
}

void SurfaceEvolutionSolver::evolve()
{
	const uint Nx = sdfGrid->Nx, Ny = sdfGrid->Ny, Nz = sdfGrid->Nz;
	Vector3 V = Vector3();
	// for SDF interpolation from values surrounding V
	std::vector<Vector3> positionBuffer = {};
	std::vector<float> valueBuffer = {};
	float SDF_V, gradSDFx_V, gradSDFy_V, gradSDFz_V; Vector3 gradSDF_V;

	if (!sdfGrid->hasGradient()) {
		sdfGrid->computeGradient();
	}

	for (int t = 0; t < NSteps; t++) {
		std::vector<Vector3> vNormals = evolvedSurface->getAngleWeightedVertexPseudoNormals();
		std::vector<std::vector<Vector3>> fvVerts = {};
		evolvedSurface->getVertexFiniteVolumes(&fvVerts);

		for (uint i = 0; i < NVerts; i++) {
			// ===== vertex =================================
			V.set(
				evolvedSurface->uniqueVertices[i].x,
				evolvedSurface->uniqueVertices[i].y,
				evolvedSurface->uniqueVertices[i].z
			);

			// ===== Control coefs ==========================
			// SDF value
			positionBuffer.clear(); // for old min and max positions
			valueBuffer.clear(); // for SDF cell vertex values
			sdfGrid->getSurroundingCells(V, Nx, Ny, Nz, sdfGrid->field, &positionBuffer, &valueBuffer);
			SDF_V = trilinearInterpolate(V, positionBuffer, valueBuffer);

			// -------- grad SDF --------
			// grad SDF x
			positionBuffer.clear(); // for old min and max positions
			valueBuffer.clear(); // for grad SDF x cell vertex values
			sdfGrid->getSurroundingCells(V, Nx - 2, Ny - 2, Nz - 2, sdfGrid->gradFieldX, &positionBuffer, &valueBuffer);
			gradSDFx_V = trilinearInterpolate(V, positionBuffer, valueBuffer);

			// grad SDF y
			positionBuffer.clear(); // for old min and max positions
			valueBuffer.clear(); // for grad SDF y cell vertex values
			sdfGrid->getSurroundingCells(V, Nx - 2, Ny - 2, Nz - 2, sdfGrid->gradFieldY, &positionBuffer, &valueBuffer);
			gradSDFy_V = trilinearInterpolate(V, positionBuffer, valueBuffer);

			// grad SDF z
			positionBuffer.clear(); // for old min and max positions
			valueBuffer.clear(); // for grad SDF z cell vertex values
			sdfGrid->getSurroundingCells(V, Nx - 2, Ny - 2, Nz - 2, sdfGrid->gradFieldZ, &positionBuffer, &valueBuffer);
			gradSDFz_V = trilinearInterpolate(V, positionBuffer, valueBuffer);

			gradSDF_V = Vector3(gradSDFx_V, gradSDFy_V, gradSDFz_V);

			// Laplace-Beltrami ctrl func:
			float eps = 1.0f;
		}
	}
}
