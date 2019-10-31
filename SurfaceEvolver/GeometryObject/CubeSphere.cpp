#include "CubeSphere.h"

CubeSphere::CubeSphere()
{
}

CubeSphere::CubeSphere(unsigned int detail, float radius)
{
	this->detail = detail; this->radius = radius;
	build();
}

CubeSphere::~CubeSphere()
{
}

// =======================================================
// ============ Pre-requisites for CubeSphere ============

void CubeSphere::build()
{
	float a = 2.0f * radius / sqrt(3.0f);
	PrimitiveBox box = PrimitiveBox(a, a, a, detail, detail, detail);

	// translate to center
	box.applyMatrix(Matrix4().makeTranslation(-a / 2.0f, -a / 2.0f, -a / 2.0f));

	// spherify
	Deform def = Deform(&box);
	def.spherify(1.0f);

	Geometry result = def.result;
	this->uniqueVertices = result.uniqueVertices;
	this->vertices = result.vertices;
	this->normals = result.normals;
	this->vertexIndices = result.vertexIndices;
	this->triangulations = result.triangulations;
}