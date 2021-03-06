#define _USE_MATH_DEFINES

#include <iostream>
#include <chrono>
#include <math.h>
#include "../Geometry/Geometry.h"
#include "../Geometry/Matrix4.h"
#include "../Geometry/Matrix3.h"
#include "../Geometry/Vector3.h"
#include "../GeometryObject/Icosphere.h"
#include "../GeometryObject/PrimitiveBox.h"
#include "../GeometryObject/CubeSphere.h"
#include "../ExportImport/VTKExporter.h"
#include "../ExportImport/OBJImporter.h"
#include "../SDF/SDF.h"
#include "../Utils/CPUInfo.h"
#include "../EvolutionCore/Evolver.h"
#include "../EvolutionCore/Parameters.h"
#include "../EvolutionCore/EvolutionRemesher.h"

//   DONE:
//
// - Add an AABBTree structure
// - Add an Octree for activating intersected grid cells
// - Make a "fast" cell intersection query
// - Set intersected cell values to 0 and INFINITY everywhere else
// - Apply Fast Sweeping Method
// - SDF
// - Optimize initial condition for FastSweep3D by actual distance computation for Octree leaves
// - Get exact octree centroid to closest triangle dist
// - Unite AABB, Octree and FastSweep3D into a single class
// - debug and optimize FastSweep3D
// - interpolate distance field for higher resolutions (not much improvement)
// - perform simple DF tests for geom primitives like sphere, icosphere, cubesphere
// - optimize Box-Triangle intersection
// - add Quaternion class and TRS decomposition of Matrix4
// - adaptive resampling of split cost function (done for 2 * 4 samples - 265-bit registers)
// - minimize split cost function using a piecewise-quadratic interpolation to find the minimum (20% slower than simple cost(x) < minCost comparison)
// - matrix multiplication for Matrix4
// - flood fill for sign computation of SDF
// - Test mesh angle weighted pseudonormals
// - test if grid gradient is computed correctly by exporting to a vtk vector file.
// - finite volume normal derivatives (Laplace-Beltrami)
// - compose a linear system for evolution from CubeSphere to PrimitiveBox of the same subdivision level
// - mean curvature flow for sphere test (cotan scheme)
// - fix lagging numerical solution for sphere test
// - test evolution without tangential redistribution on different objects
// - compute mean area for finite volumes in each time step
// - test a non-convex model (e.g.: bunny)
// - make separate outputs for mean co-volume measure
// - implement cutoff offset for the bounding cube to compute the field on minimum necessary subset (box)
// - visualize angle-weighted pseudo-normals with interpolated -grad(SDF) vectors
// - add scalar data (fvAreas, distances, curvatures) to mesh vertices
// - Special types: SEvolverParams, SDFParams,...
// - SurfaceEvolutionSolver -> Evolver, LinearSolver
// - refactor and separate console and log outputs for specific situations
// - catch all NaNs as exceptions (breaks)
// - test evolution for extremal cases: MCF dominant (eta = 0.01, eps = 1.0) and SDF dominant (eta = 1, eps = 0.01)
// - previous step mean curvature values H => H N = h (mean curvature vector)
// - previous step normal velocity term: v_N = eps(d) * h + eta(d) * N
// - tangential redist. lin. system: Laplace-Beltrami(psi) = dot( v_N, h ) - mean(dot( v_N , h) + omega * (A/G - 1)
// - Evolution from an input geometry
// - Sphere test with tangential redistribution TR (exceptionally slow for the last icosphere subdiv)
// - MCF-TR sphere test with an inhomogeneous distribution of vertices
// - MCF-TR(& w\o TR) evolution test on an ellipsoid
// - Angle-Based Tangential redistribution
// - polygon adjacency for boundary vertices

//  POSTPONED:
//
// - co-volume measure-driven time step: dt ~ m(V)
// - implement a method/class to get CPU instruction set, mainly whether it supports AVX, an alternate resampling method has to be implemented for CPU's that do not support AVX
// - implement sort order function of a 256-bit AVX vector (needs a proper lookup hash)
// - AABB update for transformations + Timing test
// - Inverse transform grid upon transforming mesh
// - implement adaptive resampling for 512-bit registers - 2 * 8 sampling positions (if possible)
// - compare results with CGAL distance query implementation
// - sign computation \w (arbitrary)ray-mesh intersection (even # of intersections = 1, odd # of intersections = -1)
//   Important notes:
//		- split position matters, yet sometimes a less precise estimate yields better result than quad min. Trying 8 sample positions could help
//		- still no idea why the near/far cycle's written this way. Sometimes it leads the traversal to only one intersection, ignoring the rest, because they're "far"
// - flat AABB and Octree (we can try, but this would require dynamic arrays (i.e.: std::vectors) which would be slower than allocation)
// - fix apparent interpolation bug for SDF values and gradients (is there one?)

//   DONE, BUT MIGHT BE IMPROVED:
//
// - AABB and Octree have to take as little space as possible
// - implement generic triangle/quad centroid co-volume procedure (centroid on triangulated polygon)

//   WIP:
// 
// - fix cotan scheme for boundary vertices


//   TODO:
//
// - fix finite volume computation for obtuse angles according to Desbrun
// - impose Dirichlet boundary conditions on Evolver
// - Debug volume-based TR - psi rhs has to have a zero sum
//
// - fix SDF coordinates (use global grid indexing)
// - implement global grid and cellSize-based Octree & SDF (just like in Vctr Engine Meta Object)
// - finish class EvolutionRemesher with all params
//
// - quad co-volume scheme
// - mean curvature flow for sphere test (quad scheme)
// - mean curvature flow for sphere test (tri interp scheme)

void performSDFTest(uint res, Geometry& g, std::fstream& timing, VTKExporter& e) {
	std::cout << "init SDF..." << std::endl;

	// Fast sweeping D, resized from 20 and interpolated
	SDF sdf_FS_r = SDF(&g, res, false, false, false, true, SDF_Method::fast_sweeping);

	std::cout << sdf_FS_r.getComputationProperties();
	timing << sdf_FS_r.getComputationProperties();

	sdf_FS_r.exportGrid(&e); // save to vti

	// Fast sweeping DF
	SDF sdf_FS = SDF(&g, res);

	std::cout << sdf_FS.getComputationProperties();
	timing << sdf_FS.getComputationProperties();

	sdf_FS.exportGrid(&e);

	// AABB DF
	SDF sdf_AABB = SDF(&g, res, false, false, false, false, SDF_Method::aabb_dist);

	std::cout << sdf_AABB.getComputationProperties();
	timing << sdf_AABB.getComputationProperties();

	sdf_AABB.exportGrid(&e);

	// Brute force DF
	SDF sdf_Brute = SDF(&g, res, false, false, false, false, SDF_Method::brute_force);

	std::cout << sdf_Brute.getComputationProperties();
	timing << sdf_Brute.getComputationProperties();

	sdf_Brute.exportGrid(&e, "voxField_" + g.name + std::to_string(res));

	Grid FSerror_r = absGrid(subGrids(*sdf_FS_r.grid, *sdf_Brute.grid));
	Grid FSerror = absGrid(subGrids(*sdf_FS.grid, *sdf_Brute.grid));
	Grid AABBerror = absGrid(subGrids(*sdf_AABB.grid, *sdf_Brute.grid));

	double error = FSerror_r.getL2Norm();
	std::cout << "FS_ERROR_resized L2 Norm: " << error << std::endl;
	timing << "FS_ERROR_resized L2 Norm: " << error << std::endl;

	error = FSerror.getL2Norm();
	std::cout << "FS_ERROR L2 Norm: " << error << std::endl;
	timing << "FS_ERROR L2 Norm: " << error << std::endl;

	error = AABBerror.getL2Norm();
	std::cout << "AABB_ERROR L2 Norm: " << error << std::endl << std::endl;
	timing << "AABB_ERROR L2 Norm: " << error << std::endl;

	FSerror_r.exportToVTI("voxField_" + g.name + std::to_string(res) + "FS_ERROR_resized");
	FSerror.exportToVTI("voxField_" + g.name + std::to_string(res) + "FS_ERROR");
	AABBerror.exportToVTI("voxField_" + g.name + std::to_string(res) + "AABB_ERROR");
}

void PerformFastSweepSdfTestForObjModel(const std::string& fileName, const uint& targetGridResolution)
{
	std::fstream timing(fileName + "_" + std::to_string(targetGridResolution) + ".txt", std::fstream::out);

	OBJImporter importer;
	Geometry geom = importer.importOBJGeometry(fileName);

	const uint octreeResolution = targetGridResolution;

	const bool computeSign = true;
	const bool notComputeGradient = false;
	const bool notSaveGridStates = false;
	const bool notScaleAndInterpolate = false;

	SDF sdf_FS_r = SDF(&geom, octreeResolution, 
		computeSign, notComputeGradient, notSaveGridStates, notScaleAndInterpolate, 
		SDF_Method::fast_sweeping);

	std::cout << sdf_FS_r.getComputationProperties();
	timing << sdf_FS_r.getComputationProperties();

	VTKExporter e;
	std::cout << "Exporting to VTI: " << std::endl;
	sdf_FS_r.exportGrid(&e, fileName + "_" + std::to_string(targetGridResolution)); // save to vti
	std::cout << "... done" << std::endl;

	timing.close();
}


void performUnitSphereTest(bool tan_redistribute = false) {
	EvolutionParams eParams;
	eParams.tStop = 0.06; eParams.elType = ElementType::tri;
	eParams.name = "testSphere"; eParams.saveStates = false;

	SphereTestParams stParams;
	TangentialRedistParams* tanRedistParams = (tan_redistribute ? new TangentialRedistParams() : nullptr);
	tanRedistParams->type = 1;

	std::fstream errLog("testSphere_errorLog.txt", std::fstream::out);
	errLog << "================================\n";
	errLog << ">>> Evolution error log ........\n";
	double errPrev, err, EOC;
	for (uint i = 0; i < 4; i++) {
		if (i > 0) errPrev = err;

		double dt = 0.01 / pow(4, i);
		eParams.dt = dt;
		eParams.subdiv = i + 1;
		stParams.testId = i;

		Evolver sphereTest(eParams, stParams, tanRedistParams);
		err = sphereTest.testL2Error();

		errLog << "dt = " << dt << ", Nsteps = " << sphereTest.nSteps() << ", Nverts = " << sphereTest.nVerts() << std::endl;
		errLog << "L2Error = " << err;
		if (i > 0) {
			EOC = log2(errPrev / err);
			std::cout << "EOC = " << EOC << std::endl;
			errLog << ", EOC = " << EOC;
		}
		errLog << std::endl;
	}
	errLog.close();
}


int main()
{
	double r = 50.0;
	unsigned int d = 3;
	double a = 2.0 * r / sqrt(3.0);
	unsigned int ns = 4;

	VTKExporter e = VTKExporter();

	/*
	std::string name = "boxWithoutLid";
	PrimitiveBox boxWithoutLid = PrimitiveBox(a, a, a, ns, ns, ns, true, name, false);
	Matrix4 R = Matrix4().makeRotationAxis(1.0, 0.0, 0.0, -M_PI / 2.0);
	Matrix4 T = Matrix4().makeTranslation(-a / 2, -a / 2, -a / 2);
	boxWithoutLid.applyMatrix(R);
	boxWithoutLid.applyMatrix(T);
	Deform def = Deform(&boxWithoutLid);
	def.spherify(1.0);
	Geometry result = def.result;

	e.initExport(&result, name);

	std::vector<std::vector<Vector3>> fvVerts = {};
	std::vector<std::vector<std::vector<uint>>> adjacentPolys = {};
	result.getVertexFiniteVolumes(&fvVerts, &adjacentPolys);*/

	/*
	uint geomId = 0;
	for (int i = 0; i < result.uniqueVertices.size(); i++) {
		bool fvOdd = fvVerts[i].size() % 2;
		for (int j = 0; j < fvVerts[i].size(); j++) {			
			if (fvOdd && j == fvVerts[i].size() - 1) {
				continue;
			}
			e.exportGeometryFiniteVolumeGrid(&result, fvVerts, adjacentPolys, name + "FV_" + std::to_string(geomId), i, j);
			if (i > 0) {
				e.exportGeometryFiniteVolumeGrid(&result, fvVerts, adjacentPolys, name + "FVs_" + std::to_string(geomId), i - 1, -1, true);
			}
			else {
				Geometry empty = Geometry();
				e.initExport(&empty, name + "FVs_" + std::to_string(geomId));
			}
			std::cout << "triangle " << geomId << " exported" << std::endl;
			geomId++;
		}
	}*/

	//e.exportGeometryFiniteVolumeGrid(&result, fvVerts, adjacentPolys, "boxWithoutLid_FVs");



	/*
	for (uint i = 0; i < 4; i++) {
		IcoSphere is = IcoSphere(i, 1.0);
		e.initExport(&is, "icoSphere_subdiv" + std::to_string(i));

		PrimitiveBox b = PrimitiveBox(1.0, 1.0, 1.0, i + 1, i + 1, i + 1);
		e.initExport(&b, "box_subdiv" + std::to_string(i));

		CubeSphere cs = CubeSphere(i + 2, 1.0);
		e.initExport(&cs, "cubeSphere_subdiv" + std::to_string(i));
	}*/

	/*
	bool iterateCubeSphereTest = false;

	if (iterateCubeSphereTest) {
		size_t min_Res = 20, max_Res = 60;
		size_t min_Ns = 0, max_Ns = 5;
		std::fstream timing_cubes("timing_cubes.txt", std::fstream::out);

		Vector3 axis = normalize(Vector3(1, 1, 1));
        
		for (uint n = min_Ns; n < max_Ns; n++) {
			for (uint i = 0; i <= 2; i++) {
				uint res = min_Res * pow(2, i);

				std::cout << "cube(" << n + 1 << "), grid_res = " << res << std::endl;
				PrimitiveBox g1 = PrimitiveBox(a, a, a, n + 1, n + 1, n + 1);
				g1.applyMatrix(Matrix4().makeRotationAxis(axis.x, axis.y, axis.z, M_PI / 6.));
				e.initExport(&g1, "cube" + std::to_string(res) + "-" + std::to_string(n));

				performSDFTest(res, g1, timing_cubes, e);

				std::cout << "cubesphere(" << n + 1 << "), grid_res = " << res << std::endl;
				CubeSphere g2 = CubeSphere(n + 1, r);
				g2.applyMatrix(Matrix4().makeRotationAxis(axis.x, axis.y, axis.z, M_PI / 6.));
				e.initExport(&g2, "cubesphere" + std::to_string(res) + "-" + std::to_string(n));

				performSDFTest(res, g2, timing_cubes, e);
			}
		}
		timing_cubes.close();
	}


	bool iterateIcoSphereTest = false;

	if (iterateIcoSphereTest) {
		size_t min_Res = 20, max_Res = 60;
		size_t min_Ns = 0, max_Ns = 2;
		std::fstream timing_ico("timing_ico.txt", std::fstream::out);

		Vector3 axis = normalize(Vector3(1, 1, 1));

		for (uint n = min_Ns; n < max_Ns; n++) {
			for (uint i = 0; i <= 2; i++) {
				uint res = min_Res * pow(2, i);

				std::cout << "icosphere(" << n << "), grid_res = " << res << std::endl;
				IcoSphere g0 = IcoSphere(n, r);
				g0.applyMatrix(Matrix4().makeRotationAxis(axis.x, axis.y, axis.z, M_PI / 6.));
				e.initExport(&g0, "icosphere" + std::to_string(res) + "-" + std::to_string(n));

				performSDFTest(res, g0, timing_ico, e);
			}
		}

		timing_ico.close();
	}*/

	// ===== BUNNY SDF tests =============================

	/*
	uint res = 40; // octree resolution

	// Vector3 axis = normalize(Vector3(1, 1, 1));
	
	auto startObjLoad = std::chrono::high_resolution_clock::now();
	// === Timed code ============
	OBJImporter obj = OBJImporter();
	Geometry bunny = obj.importOBJGeometry("bunny_no_holes.obj");
	bunny.applyMatrix(Matrix4().setToScale(0.02, 0.02, 0.02));
	e.initExport(&bunny, "sfBunny");
	// === Timed code ============
	auto endObjLoad = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsedObj = (endObjLoad - startObjLoad);
	std::cout << "Model loaded after " << elapsedObj.count() << " seconds" << std::endl;

	
	SDF bunny_sdf = SDF(&bunny, res, true);

	std::cout << bunny_sdf.getComputationProperties();*/

	// bunny_sdf.exportGrid(&e, "bunnySDF");
	// bunny_sdf.exportGradientField(&e, "bunnySDF_grad");

	// ====== BUNNY Evolution =============================
	/* EvolutionParams eParams;
	eParams.name = "Bunny";
	eParams.dt = 0.02; eParams.NSteps = 200; eParams.subdiv = (uint)3; eParams.elType = ElementType::tri;
	eParams.saveStates = true; eParams.printStepOutput = true; eParams.writeTimeLog = true;
	MeanCurvatureParams mcfParams;
	mcfParams.saveAreaStates = true;
	mcfParams.saveCurvatureStates = true;
	mcfParams.writeMeanAreaLog = true;
	GradDistanceParams sdfParams;
	sdfParams.targetGeom = &bunny; sdfParams.sdfGrid = bunny_sdf.grid;
	sdfParams.saveDistanceStates = true;
	// sdfParams.saveGradientStates = true;
	//mcfParams.smoothSteps = 10;s
	TangentialRedistParams tRedistParams;
	tRedistParams.type = 1;
	tRedistParams.omega_volume = 100.0;
	tRedistParams.omega_angle = 3.0;
	//tRedistParams.saveTangentialVelocityStates = true;

	Evolver evolver(eParams, mcfParams, sdfParams, &tRedistParams); */

	// cube with holes
	/*
	OBJImporter obj = OBJImporter();
	Geometry cwh = obj.importOBJGeometry("cubeWithHoles.obj");
	cwh.applyMatrix(Matrix4().setToScale(0.02, 0.02, 0.02));
	std::string name = "evolvingCubeWithHoles";
	e.initExport(&cwh, "cubeWithHoles");

	uint res = 40; // octree resolution
	SDF cwh_sdf = SDF(&cwh, res);
	// cwh_sdf.exportGrid(&e, "cubeWithHoles_SDF");
	// cwh_sdf.exportGradientField(&e, "cubeWithHoles_SDF_grad");

	std::cout << cwh_sdf.getComputationProperties();

	EvolutionParams eParams;
	eParams.name = name;
	eParams.dt = 0.03; eParams.NSteps = 150; eParams.subdiv = (uint)3; eParams.elType = ElementType::tri;
	eParams.printStepOutput = true; eParams.writeTimeLog = true;
	MeanCurvatureParams mcfParams;
	mcfParams.saveAreaStates = true; mcfParams.writeMeanAreaLog = true;
	GradDistanceParams sdfParams;
	sdfParams.targetGeom = &cwh; sdfParams.sdfGrid = cwh_sdf.grid;
	sdfParams.saveDistanceStates = true;
	// sdfParams.saveGradientStates = true;
	mcfParams.smoothSteps = 10;*/

	//Evolver evolver(eParams, mcfParams, sdfParams);

	/*
	OBJImporter obj = OBJImporter();
	Geometry teapot = obj.importOBJGeometry("teapot.obj");
	uint res = 40; // octree resolution
	SDF teapot_sdf = SDF(&teapot, res);
	teapot_sdf.exportGrid(&e, "teapot_SDF");*/

	// arc
	/*	
	OBJImporter obj = OBJImporter();
	Geometry arc = obj.importOBJGeometry("arc.obj");
	arc.applyMatrix(Matrix4().setToScale(0.01, 0.01, 0.01));
	std::string name = "evolvingArc";
	e.initExport(&arc, "arc");

	uint res = 40; // octree resolution
	SDF arc_sdf = SDF(&arc, res);
	// cwh_sdf.exportGrid(&e, "cubeWithHoles_SDF");
	// cwh_sdf.exportGradientField(&e, "cubeWithHoles_SDF_grad");

	std::cout << arc_sdf.getComputationProperties();

	EvolutionParams eParams;
	eParams.name = name;
	eParams.dt = 0.02; eParams.NSteps = 150; eParams.subdiv = (uint)3; eParams.elType = ElementType::tri;
	eParams.saveStates = true; //eParams.printStepOutput = true; eParams.writeTimeLog = true; // eParams.printSolution = true;
	MeanCurvatureParams mcfParams;
	mcfParams.saveAreaStates = true; 
	mcfParams.saveCurvatureStates = true;
	mcfParams.writeMeanAreaLog = true;
	GradDistanceParams sdfParams;
	sdfParams.targetGeom = &arc; sdfParams.sdfGrid = arc_sdf.grid;
	sdfParams.saveDistanceStates = true;
	// sdfParams.saveGradientStates = true;
	mcfParams.smoothSteps = 10;
	TangentialRedistParams tRedistParams;
	tRedistParams.omega_volume = 100.0;
	tRedistParams.omega_angle = 5.0;
	tRedistParams.type = 1;
	tRedistParams.saveTangentialVelocityStates = true;

	Evolver evolver(eParams, mcfParams, sdfParams, &tRedistParams);*/

	/*
	e.exportGeometryVertexNormals(&bunny, "bunnyNormals");
	e.exportGeometryFiniteVolumeGrid(&bunny, "bunnyFVs");*/

	// performUnitSphereTest(true);

	/*
	std::string name = "testBox";
	PrimitiveBox b = PrimitiveBox(1, 1, 1, 3, 3, 3, true, name);
	Vector3 axis = Vector3(1, 1, 1);
	axis.normalize();
	Matrix4 M = Matrix4().makeRotationAxis(axis.x, axis.y, axis.z, M_PI / 6);
	b.applyMatrix(M);
	e.initExport(&b, name);
	uint res = 40; // octree resolution
	SDF boxSDF = SDF(&b, res, true, true);
	//boxSDF.exportGrid(&e, "boxSDF");
	//boxSDF.exportGradientField(&e, "boxSDF_Grad");

	EvolutionParams eParams;
	eParams.name = name;
	eParams.dt = 0.025; eParams.NSteps = 100; eParams.subdiv = (uint)3; eParams.elType = ElementType::tri;
	eParams.saveStates = true; // eParams.printStepOutput = true; // eParams.printSolution = true; eParams.printHappenings = true; //eParams.writeTimeLog = true;
	MeanCurvatureParams mcfParams;
	mcfParams.saveAreaStates = true;
	mcfParams.saveCurvatureStates = true;
	mcfParams.saveCurvatureVectors = true;
	mcfParams.saveNormalVelocityStates = true;
	mcfParams.writeMeanAreaLog = true;
	GradDistanceParams sdfParams;
	sdfParams.targetGeom = &b; sdfParams.sdfGrid = boxSDF.grid;
	sdfParams.saveDistanceStates = true;
	//sdfParams.saveGradientStates = true;
	mcfParams.smoothSteps = 10;
	mcfParams.initSmoothRate = 0.01;
	TangentialRedistParams tRedistParams;
	tRedistParams.omega = 200.0;
	tRedistParams.type = 1;
	tRedistParams.saveTangentialVelocityStates = true;

	Evolver evolver(eParams, mcfParams, sdfParams, &tRedistParams);*/

	/*
	std::string name = "testEllipsoid";
	// CubeSphere cs = CubeSphere(10, 50.0, true, name);
	IcoSphere is = IcoSphere(3, 1.0, name);
	Matrix4 M = Matrix4().setToScale(1.5, 1.0, 1.0);
	is.applyMatrix(M);
	e.initExport(&is, name);
	uint res = 40; // octree resolution
	SDF isSDF = SDF(&is, res, true, true);
	//isSDF.exportGrid(&e, name + "SDF");
	//isSDF.exportGradientField(&e, name + "SDF_Grad");

	EvolutionParams eParams;
	eParams.name = name;
	eParams.dt = 0.005; eParams.NSteps = 60; //eParams.subdiv = (uint)3; eParams.elType = ElementType::tri;
	eParams.saveStates = true; // eParams.printStepOutput = true; eParams.writeTimeLog = true;
	eParams.sourceGeometry = &is;
	MeanCurvatureParams mcfParams;
	mcfParams.saveAreaStates = true; 
	mcfParams.writeMeanAreaLog = true;
	GradDistanceParams sdfParams;
	//sdfParams.targetGeom = &is; sdfParams.sdfGrid = isSDF.grid;
	//sdfParams.saveDistanceStates = true;
	//sdfParams.saveGradientStates = true;
	//mcfParams.smoothSteps = 10;
	TangentialRedistParams tRedistParams;
	tRedistParams.omega = 1000.0;
	tRedistParams.type = 1;
	tRedistParams.saveTangentialVelocityStates = true;

	Evolver evolver(eParams, mcfParams, sdfParams, &tRedistParams);*/
	
	
	/*
	std::string name = "cubeEllipsoidMCF";
	CubeSphere cs = CubeSphere(10, 1.0, false, name);
	Matrix4 M = Matrix4().setToScale(2.0, 1.0, 1.0);
	cs.applyMatrix(M);
	e.initExport(&cs, name);
	//uint res = 40; // octree resolution
	//SDF csSDF = SDF(&cs, res, true, true);
	//isSDF.exportGrid(&e, name + "SDF");
	//isSDF.exportGradientField(&e, name + "SDF_Grad");

	EvolutionParams eParams;
	eParams.name = name;
	eParams.dt = 0.003; eParams.NSteps = 100; //eParams.subdiv = (uint)3; 
	eParams.elType = ElementType::tri;
	eParams.saveStates = true; eParams.printStepOutput = true;
	//eParams.printStepOutput = true; eParams.writeTimeLog = true;
	eParams.sourceGeometry = &cs;
	MeanCurvatureParams mcfParams;
	mcfParams.saveAreaStates = true; 
	mcfParams.writeMeanAreaLog = true;
	GradDistanceParams sdfParams;
	//sdfParams.targetGeom = &cs; sdfParams.sdfGrid = csSDF.grid;
	//sdfParams.saveDistanceStates = true;
	//sdfParams.saveGradientStates = true;
	//mcfParams.smoothSteps = 10;
	TangentialRedistParams tRedistParams;
	tRedistParams.omega_volume = 200.0;
	tRedistParams.type = 1;
	tRedistParams.omega_angle = 0.0;
	// tRedistParams.saveTangentialVelocityStates = true;

	Evolver evolver(eParams, mcfParams, sdfParams, &tRedistParams);
	*/

	/*
	std::string name = "DistortedIcoSphere";
	OBJImporter obj = OBJImporter();
	Geometry di = obj.importOBJGeometry("DistortedIcoSphere.obj");
	Matrix4 M = Matrix4().setToScale(0.05, 0.05, 0.05);
	di.applyMatrix(M);

	EvolutionParams eParams;
	eParams.name = name;
	eParams.dt = 0.03; eParams.NSteps = 50; //eParams.subdiv = (uint)3; 
	eParams.elType = ElementType::tri;
	eParams.saveStates = true; eParams.printStepOutput = true;
	//eParams.printStepOutput = true; eParams.writeTimeLog = true;
	eParams.sourceGeometry = &di;
	MeanCurvatureParams mcfParams;
	mcfParams.saveAreaStates = true;
	mcfParams.writeMeanAreaLog = true;
	GradDistanceParams sdfParams;
	//sdfParams.targetGeom = &cs; sdfParams.sdfGrid = csSDF.grid;
	//sdfParams.saveDistanceStates = true;
	//sdfParams.saveGradientStates = true;
	//mcfParams.smoothSteps = 10;
	TangentialRedistParams tRedistParams;
	tRedistParams.omega_volume = 200.0;
	tRedistParams.type = 1;
	tRedistParams.omega_angle = 1.0;
	// tRedistParams.saveTangentialVelocityStates = true;

	Evolver evolver(eParams, mcfParams, sdfParams, &tRedistParams);*/

	/*
	std::string name = "DistortedIcoSphere_noRedist";
	OBJImporter obj = OBJImporter();
	Geometry di = obj.importOBJGeometry("DistortedIcoSphere.obj");
	Matrix4 M = Matrix4().setToScale(0.05, 0.05, 0.05);
	di.applyMatrix(M);

	EvolutionParams eParams;
	eParams.name = name;
	eParams.dt = 0.03; eParams.NSteps = 50; //eParams.subdiv = (uint)3; 
	eParams.elType = ElementType::tri;
	eParams.saveStates = true; eParams.printStepOutput = true;
	//eParams.printStepOutput = true; eParams.writeTimeLog = true;
	eParams.sourceGeometry = &di;
	MeanCurvatureParams mcfParams;
	mcfParams.saveAreaStates = true;
	mcfParams.writeMeanAreaLog = true;
	GradDistanceParams sdfParams;

	Evolver evolver(eParams, mcfParams, sdfParams, nullptr);*/

	// uneven sphere:
	/*
	OBJImporter obj = OBJImporter();
	Geometry uSphere = obj.importOBJGeometry("UnevenSphere.obj");
	uSphere.applyMatrix(Matrix4().setToScale(0.02, 0.02, 0.02));
	std::string name = "UnevenSphere";
	e.initExport(&uSphere, name);

	uint res = 40; // octree resolution
	SDF us_sdf = SDF(&uSphere, res);

	std::cout << us_sdf.getComputationProperties();

	EvolutionParams eParams;
	eParams.name = name;
	eParams.dt = 0.005; eParams.NSteps = 50; eParams.elType = ElementType::tri;
	eParams.saveStates = true; eParams.printStepOutput = true; // eParams.printSolution = true; eParams.printHappenings = true; //eParams.writeTimeLog = true;
	eParams.sourceGeometry = &uSphere;
	MeanCurvatureParams mcfParams;
	mcfParams.saveAreaStates = true;
	mcfParams.saveCurvatureStates = true;
	mcfParams.saveCurvatureVectors = true;
	mcfParams.saveNormalVelocityStates = true;
	mcfParams.writeMeanAreaLog = true;
	GradDistanceParams sdfParams;
	//dfParams.targetGeom = &b; sdfParams.sdfGrid = boxSDF.grid;
	//sdfParams.saveGradientStates = true;
	TangentialRedistParams tRedistParams;
	tRedistParams.omega = 10.0;
	tRedistParams.saveTangentialVelocityStates = true;

	Evolver evolver(eParams, mcfParams, sdfParams, &tRedistParams);*/

	/*
	IcoSphere is(1, 1.0);
	e.initExport(&is, "icoSphere");
	std::vector<std::vector<Vector3>> fvVerts = {};
	std::vector<std::vector<std::vector<uint>>> adjacentPolys = {};
	is.getVertexFiniteVolumes(&fvVerts, &adjacentPolys);
	uint geomId = 0;
	for (int i = 0; i < is.uniqueVertices.size(); i++) {
		for (int j = 0; j < fvVerts[i].size(); j++) {
			e.exportGeometryFiniteVolumeGrid(&is, fvVerts, adjacentPolys, "icoSphereFV_" + std::to_string(geomId), i, j);
			if (i > 0) {
				e.exportGeometryFiniteVolumeGrid(&is, fvVerts, adjacentPolys, "icoSphereFVs_" + std::to_string(geomId), i - 1, -1, true);
			}
			else {
				Geometry empty = Geometry();
				e.initExport(&empty, "icoSphereFVs_" + std::to_string(geomId));
			}
			std::cout << "triangle " << geomId << " exported" << std::endl;
			geomId++;
		}
	}*/		

	/*
	uint i = 0;
	double dt = 0.01 / pow(4, i);
	Evolver sphereTest(dt, 0.06, i, ElementType::tri, "testSphere", false, false, true);*/

	/*
	Matrix4 sdfTransform = Matrix4().makeTranslation(0.5, 0.5, 0.5).multiply(Matrix4().setToScale(2.0, 2.0, 2.0));
	//.makeRotationAxis(axis.x, axis.y, axis.z, M_PI / 6.);
	bunny_sdf.applyMatrix(sdfTransform);

	std::cout << bunny_sdf.last_transform;

	bunny_sdf.exportGrid(&e, "bunnySDF_scaled");
	e.initExport(*bunny_sdf.geom, "sfBunny_scaled");*/

	/*
	PrimitiveBox cube = PrimitiveBox(a, a, a, 4, 4, 4);
	cube.applyMatrix(Matrix4().makeRotationAxis(axis.x, axis.y, axis.z, M_PI / 6.));
	e.initExport(cube, "cube");

	SDF cubeSDF = SDF(&cube, res, true); // signed dist to cube
	std::cout << std::endl << cubeSDF.getComputationProperties();
	cubeSDF.exportGrid(&e, "cubeSDF");*/
	

	// tree visualisation
	/*
	bunny_sdf.tri_aabb->GenerateFullTreeBoxVisualisation(e);
	bunny_sdf.tri_aabb->GenerateFullLeafBoxVisualisation(e);
	bunny_sdf.tri_aabb->GenerateStepwiseLeafBoxVisualisation(e);
	bunny_sdf.octree->GenerateFullOctreeBoxVisualisation(e);
	bunny_sdf.octree->GenerateLeafCellVisualisation(e); 
	*/


	/* The brute force DF of the bunny model will take ~27 min ! 
	
	SDF bunny_sdf_b = SDF(&bunny, res, false, SDF_Method::brute_force);

	std::cout << bunny_sdf_b.getComputationProperties();

	bunny_sdf_b.exportGrid(&e, "bunnySDF_b");

	Grid bunnySDF_r_Error = absGrid(subGrids(*bunny_sdf_r.grid, *bunny_sdf_b.grid));
	bunnySDF_r_Error.exportToVTI("bunnySDF_" + std::to_string(res) + "FS_ERROR_resized");

	Grid bunnySDF_Error = absGrid(subGrids(*bunny_sdf.grid, *bunny_sdf_b.grid));
	bunnySDF_Error.exportToVTI("bunnySDF_" + std::to_string(res) + "FS_ERROR");

	double error = bunnySDF_r_Error.getL2Norm();
	std::cout << "FS_ERROR_resized L2 Norm: " << error << std::endl;

	error = bunnySDF_Error.getL2Norm();
	std::cout << "FS_ERROR L2 Norm: " << error << std::endl; */

	PerformFastSweepSdfTestForObjModel("./TestModels/armadillo.obj", 80);

	return 1;
}

