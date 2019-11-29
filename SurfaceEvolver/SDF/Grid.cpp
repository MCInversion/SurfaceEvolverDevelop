#include "Grid.h"

Grid::Grid()
{
}

Grid::Grid(const Grid& other)
{
	this->Nx = other.Nx; this->Ny = other.Ny; this->Nz = other.Nz;
	if (this->field) delete[] this->field;
	if (this->frozenCells) delete[] this->frozenCells;

	uint i, gridExtent = this->Nx * this->Ny * this->Nz;
	this->field = new float[gridExtent];
	this->frozenCells = new bool[gridExtent];
	for (i = 0; i < gridExtent; i++) {
		this->field[i] = other.field[i]; // initialize field
		this->frozenCells[i] = other.frozenCells[i]; // unfreeze all
	}
	this->scale = other.scale;
	this->bbox = other.bbox;

	this->min = other.min;
	this->max = other.max;
}

Grid::Grid(uint Nx, uint Ny, uint Nz, Box3 bbox, bool addOffset, float initVal)
{
	// re-scale to fit the rest of the field
	this->bbox = bbox;
	if (addOffset) {
		Vector3 origScale = bbox.getSize();
		float offset = max_offset_factor * std::fmaxf(origScale.x, std::fmaxf(origScale.y, origScale.z));
		this->bbox.expandByOffset(offset);
		Vector3 newScale = this->bbox.getSize();
		this->Nx = (uint)std::floor((1.0f + 2.0f * max_offset_factor) * Nx);
		this->Ny = (uint)std::floor((1.0f + 2.0f * max_offset_factor) * Ny);
		this->Nz = (uint)std::floor((1.0f + 2.0f * max_offset_factor) * Nz);
		this->scale = newScale;
	}
	else {
		this->scale = bbox.getSize();
		this->Nx = Nx; this->Ny = Ny; this->Nz = Nz;
	}

	uint i; this->gridExtent = this->Nx * this->Ny * this->Nz;
	this->field = new float[gridExtent];
	this->frozenCells = new bool[gridExtent];
	for (i = 0; i < gridExtent; i++) {
		this->field[i] = initVal; // initialize field
		this->frozenCells[i] = false; // unfreeze all
	}
}

Grid::~Grid()
{
	this->clean();
}

void Grid::exportToVTI(std::string filename)
{
	std::fstream vti(filename + ".vti", std::fstream::out);

	Vector3 o = bbox.min; // origin
	uint nx = Nx - 1;
	uint ny = Ny - 1;
	uint nz = Nz - 1;
	float dx = scale.x / Nx;
	float dy = scale.y / Ny;
	float dz = scale.z / Nz;

	vti << "<VTKFile type=\"ImageData\" version=\"1.0\" byte_order=\"LittleEndian\" header_type=\"UInt64\">" << std::endl;
	vti << "	<ImageData WholeExtent=\"0 " << nx << " 0 " << ny << " 0 " << nz << "\" Origin=\"" << o.x + 0.5 * dx << " " << o.y + 0.5 * dy << " " << o.z + 0.5 * dz << "\" Spacing=\"" << dx << " " << dy << " " << dz << "\">" << std::endl;
	vti << "		<Piece Extent=\"0 " << nx << " 0 " << ny << " 0 " << nz << "\">" << std::endl;
	vti << "			<PointData Scalars=\"Scalars_\">" << std::endl;
	vti << "				<DataArray type=\"Float32\" Name=\"Scalars_\" format=\"ascii\" RangeMin=\"" << min << "\" RangeMax=\"" << max << "\">" << std::endl;

	for (uint i = 0; i < this->gridExtent; i ++) {
		vti << this->field[i] << std::endl;
	}

	vti << "				</DataArray>" << std::endl;
	vti << "			</PointData>" << std::endl;
	vti << "		<CellData>" << std::endl;
	vti << "		</CellData>" << std::endl;
	vti << "	</Piece>" << std::endl;
	vti << "	</ImageData>" << std::endl;
	vti << "</VTKFile>";

	vti.close();
}

void Grid::initToVal(float val)
{
	if (this->field) delete[] this->field;
	if (this->frozenCells) delete[] this->frozenCells;
	uint i;
	this->field = new float[gridExtent];
	this->frozenCells = new bool[gridExtent];
	for (i = 0; i < gridExtent; i++) {
		this->field[i] = val; // initialize field
		this->frozenCells[i] = false; // unfreeze all
	}
}

void Grid::blur()
{
	auto applyGrid3x3Kernel = [&](float* bfield, uint ix, uint iy, uint iz) {
		float kernelSum = 0.0f;
		uint gridPos = 0;
		for (int i = -1; i <= 1; i++) {
			for (int j = -1; j <= 1; j++) {
				for (int k = -1; k <= 1; k++) {
					gridPos =
						Nx * Ny * std::min(std::max((int)iz + i, 0), (int)Nz - 1) +
						Nx * std::min(std::max((int)iy + j, 0), (int)Ny - 1) +
						std::min(std::max((int)ix + k, 0), (int)Nx - 1);
					kernelSum += bfield[gridPos];
				}
			}
		}
		gridPos = Nx * Ny * iz + Nx * iy + ix;
		float result = kernelSum / 27.0f;
		field[gridPos] = result;
	};

	float* bfield = new float[gridExtent];
	for (uint i = 0; i < gridExtent; i++) bfield[i] = this->field[i];
	for (uint iz = 1; iz < Nz - 1; iz++) {
		for (uint iy = 1; iy < Ny - 1; iy++) {
			for (uint ix = 1; ix < Nx - 1; ix++) applyGrid3x3Kernel(bfield, ix, iy, iz);
		}
	}
	delete[] this->field;
	this->field = new float[gridExtent];
	for (uint i = 0; i < gridExtent; i++) this->field[i] = bfield[i];
}

bool Grid::equalInDimTo(Grid& other)
{
	return (
		(this->Nx == other.Nx && this->Ny == other.Ny && this->Nz == other.Nz) // &&
		// this->bbox.equals(other.bbox) // this seems like an unnecessary criterion
	);
}

void Grid::add(Grid& other)
{
	if (this->equalInDimTo(other)) {
		for (uint i = 0; i < gridExtent; i++) {
			this->field[i] += other.field[i];
		}
	}
	else {
		std::cout << "invalid grid dimensions" << std::endl;
	}
}

void Grid::sub(Grid& other)
{
	if (this->equalInDimTo(other)) {
		for (uint i = 0; i < gridExtent; i++) {
			this->field[i] -= other.field[i];
		}
	}
	else {
		std::cout << "invalid grid dimensions" << std::endl;
	}
}

void Grid::absField()
{
	for (uint i = 0; i < gridExtent; i++) {
		this->field[i] = fabs(this->field[i]);
	}
}

void Grid::negate()
{
	float val;
	uint gridPos;
	uint iz, iy, ix;

	for (iz = 0; iz < Nz; iz++) {
		for (iy = 0; iy < Ny; iy++) {
			for (ix = 0; ix < Nx; ix++) {
				gridPos = Nx * Ny * iz + Nx * iy + ix;
				val = -1.0f * this->field[gridPos];
				this->field[gridPos] = val;
			}
		}
	}
}

void Grid::computeSignField(AABBTree* aabb)
{
	this->negate();

	float val; uint gridPos;
	uint nx = Nx - 1;
	uint ny = Ny - 1;
	uint nz = Nz - 1;
	
	uint iz = 0, iy = 0, ix = 0;

	std::stack<std::tuple<uint, uint, uint>> stack = {};
	std::tuple<uint, uint, uint> idsTriple;

	// find the first unfrozen cell
	gridPos = 0;
	while (this->frozenCells[gridPos]) {
		ix += (ix < nx ? 1 : 0);
		iy += (iy < ny ? 1 : 0);
		iz += (iz < nz ? 1 : 0);
		gridPos = Nx * Ny * iz + Nx * iy + ix;
	}

	stack.push({ ix, iy, iz });

	// a simple voxel flood
	while (stack.size()) {
		idsTriple = stack.top();
		stack.pop();

		ix = std::get<0>(idsTriple);
		iy = std::get<1>(idsTriple);
		iz = std::get<2>(idsTriple);

		gridPos = Nx * Ny * iz + Nx * iy + ix;

		if (!this->frozenCells[gridPos]) {
			val = -1.0f * this->field[gridPos];
			this->field[gridPos] = val;
			this->frozenCells[gridPos] = true; // freeze cell when done

			if (ix > 0) {
				stack.push({ ix - 1, iy, iz });
			}
			if (ix < nx) {
				stack.push({ ix + 1, iy, iz });
			}
			if (iy > 0) {
				stack.push({ ix, iy - 1, iz });
			}
			if (iy < ny) {
				stack.push({ ix, iy + 1, iz });
			}
			if (iz > 0) {
				stack.push({ ix, iy, iz - 1 });
			}
			if (iz < nz) {
				stack.push({ ix, iy, iz + 1 });
			}
		}
	}

}


void Grid::bruteForceDistanceField(Geometry* geom)
{
	const uint Nx = this->Nx, Ny = this->Ny, Nz = this->Nz;
	int ix, iy, iz, i, gridPos;
	Vector3 p = Vector3(); Tri T;

	Vector3 o = bbox.min; // origin
	uint nx = Nx - 1;
	uint ny = Ny - 1;
	uint nz = Nz - 1;
	float dx = scale.x / nx;
	float dy = scale.y / ny;
	float dz = scale.z / nz;
	float result_distSq, distSq;

	for (iz = 0; iz < Nz; iz++) {
		for (iy = 0; iy < Ny; iy++) {
			for (ix = 0; ix < Nx; ix++) {

				result_distSq = FLT_MAX;
				p.set(
					o.x + ix * dx,
					o.y + iy * dy,
					o.z + iz * dz
				);

				for (i = 0; i < geom->vertexIndices.size(); i += 3) {
					Vector3** t = new Vector3 * [3];
					t[0] = &geom->uniqueVertices[geom->vertexIndices[i]];
					t[1] = &geom->uniqueVertices[geom->vertexIndices[i + 1]];
					t[2] = &geom->uniqueVertices[geom->vertexIndices[i + 2]];
					distSq = getDistanceToATriangleSq(t, &p);
					delete[] t;

					result_distSq = distSq < result_distSq ? distSq : result_distSq;
				}

				gridPos = Nx * Ny * iz + Nx * iy + ix;
				this->field[gridPos] = sqrt(result_distSq);
			}
		}
	}
}

void Grid::aabbDistanceField(AABBTree* aabb)
{
	const uint Nx = this->Nx, Ny = this->Ny, Nz = this->Nz;
	int ix, iy, iz, i, gridPos;
	Vector3 p = Vector3(); Tri T;

	Vector3 o = bbox.min; // origin
	uint nx = Nx - 1;
	uint ny = Ny - 1;
	uint nz = Nz - 1;
	float dx = scale.x / nx;
	float dy = scale.y / ny;
	float dz = scale.z / nz;
	float distSq;

	for (iz = 0; iz < Nz; iz++) {
		for (iy = 0; iy < Ny; iy++) {
			for (ix = 0; ix < Nx; ix++) {

				p.set(
					o.x + ix * dx,
					o.y + iy * dy,
					o.z + iz * dz
				);

				i = aabb->getClosestPrimitiveIdAndDist(p, &distSq);
				if (i < 0) continue;

				gridPos = Nx * Ny * iz + Nx * iy + ix;
				this->field[gridPos] = sqrt(distSq);
			}
		}
	}
}

void Grid::clean()
{
	Nx = 0, Ny = 0, Nz = 0;
	scale = Vector3(1.0f, 1.0f, 1.0f);
	if (this->field) delete[] field;
	if (this->frozenCells) delete[] frozenCells;
}

void Grid::scaleBy(Vector3& s)
{
	uint oldNx = Nx, oldNy = Ny, oldNz = Nz;

	this->Nx = (uint)std::floor(s.x * Nx);
	this->Ny = (uint)std::floor(s.y * Ny);
	this->Nz = (uint)std::floor(s.z * Nz);
	uint oldExtent = oldNx * oldNy * oldNz, i;
	float* oldField = new float[oldExtent];
	for (i = 0; i < oldExtent; i++) oldField[i] = this->field[i];

	this->gridExtent = this->Nx * this->Ny * this->Nz;
	this->field = new float[this->gridExtent];
	Vector3 p = Vector3(), o = bbox.min;
	uint nx = Nx - 1;
	uint ny = Ny - 1;
	uint nz = Nz - 1;
	uint gridPos;
	float dx = this->scale.x / nx;
	float dy = this->scale.y / ny;
	float dz = this->scale.z / nz;
	std::vector<Vector3> positionBuffer = {};
	std::vector<float> valueBuffer = {};

	uint ix, iy, iz;
	
	for (iz = 0; iz < Nz; iz++) {
		for (iy = 0; iy < Ny; iy++) {
			for (ix = 0; ix < Nx; ix++) {

				p.set(
					o.x + ix * dx,
					o.y + iy * dy,
					o.z + iz * dz
				);				

				positionBuffer.clear(); // for old min and max positions
				valueBuffer.clear(); // for old cell vertex values
				this->getSurroundingCells(p, oldNx, oldNy, oldNz, oldField, &positionBuffer, &valueBuffer);

				gridPos = Nx * Ny * iz + Nx * iy + ix;
				this->field[gridPos] = trilinearInterpolate(p, positionBuffer, valueBuffer);
			}
		}
	}

}

void Grid::getSurroundingCells(Vector3& pos,
	uint oldNx, uint oldNy, uint oldNz,	float* oldField,
	std::vector<Vector3>* positionBuffer, std::vector<float>* valueBuffer)
{
	uint ix = std::min((uint)std::floor((pos.x - bbox.min.x) * oldNx / this->scale.x), oldNx - 1);
	uint iy = std::min((uint)std::floor((pos.y - bbox.min.y) * oldNy / this->scale.y), oldNy - 1);
	uint iz = std::min((uint)std::floor((pos.z - bbox.min.z) * oldNz / this->scale.z), oldNz - 1);

	uint ix1 = std::min(ix + 1, oldNx - 1);
	uint iy1 = std::min(iy + 1, oldNy - 1);
	uint iz1 = std::min(iz + 1, oldNz - 1);

	uint i000 = oldNx * oldNy * iz + oldNx * iy + ix;
	uint i100 = oldNx * oldNy * iz + oldNx * iy + ix1;
	uint i010 = oldNx * oldNy * iz + oldNx * iy1 + ix;
	uint i110 = oldNx * oldNy * iz + oldNx * iy1 + ix1;
	uint i001 = oldNx * oldNy * iz1 + oldNx * iy + ix;
	uint i101 = oldNx * oldNy * iz1 + oldNx * iy + ix1;
	uint i011 = oldNx * oldNy * iz1 + oldNx * iy1 + ix;
	uint i111 = oldNx * oldNy * iz1 + oldNx * iy1 + ix1;

	valueBuffer->push_back(oldField[i000]);
	valueBuffer->push_back(oldField[i100]);
	valueBuffer->push_back(oldField[i010]);
	valueBuffer->push_back(oldField[i110]);
	valueBuffer->push_back(oldField[i001]);
	valueBuffer->push_back(oldField[i101]);
	valueBuffer->push_back(oldField[i011]);
	valueBuffer->push_back(oldField[i111]);

	positionBuffer->push_back(Vector3(
		bbox.min.x + (float)ix / (float)oldNx * this->scale.x,
		bbox.min.y + (float)iy / (float)oldNy * this->scale.y,
		bbox.min.z + (float)iz / (float)oldNz * this->scale.z
	));

	positionBuffer->push_back(Vector3(
		bbox.min.x + (float)(ix + 1) / (float)oldNx * this->scale.x,
		bbox.min.y + (float)(iy + 1) / (float)oldNy * this->scale.y,
		bbox.min.z + (float)(iz + 1) / (float)oldNz * this->scale.z
	));
}

float Grid::getL2Norm()
{
	float result = 0.0f;

	uint gridPos, ix, iy, iz;

	for (iz = 0; iz < Nz; iz++) {
		for (iy = 0; iy < Ny; iy++) {
			for (ix = 0; ix < Nx; ix++) {

				gridPos = Nx * Ny * iz + Nx * iy + ix;
				result += this->field[gridPos] * this->field[gridPos];
			}
		}
	}

	return sqrt(result / (Nx * Ny * Nz));
}

void Grid::clearFrozenCells()
{
	if (this->frozenCells) {
		delete[] this->frozenCells;
		this->frozenCells = nullptr;
	}
}

void Grid::clearField()
{
	if (this->field) {
		delete[] this->field;
		this->field = nullptr;
	}
}

Grid subGrids(Grid g0, Grid g1)
{
	Grid result = g0;
	result.sub(g1);
	return result;
}

Grid absGrid(Grid g)
{
	g.absField();
	return g;
}

float trilinearInterpolate(Vector3& P, std::vector<Vector3>& X, std::vector<float>& f)
{
	float x = P.x, y = P.y, z = P.z;
	float x0 = X[0].x, y0 = X[0].y, z0 = X[0].z; // cell min
	float x1 = X[1].x, y1 = X[1].y, z1 = X[1].z; // cell max

	// cell values
	float c000 = f[0], c100 = f[1], c010 = f[2], c110 = f[3];
	float c001 = f[4], c101 = f[5], c011 = f[6], c111 = f[7];

	float det = (x0 - x1) * (y0 - y1) * (z0 - z1);

	float a0 =
		(c111 * x0 * y0 * z0 - c011 * x1 * y0 * z0 - c101 * x0 * y1 * z0 + c001 * x1 * y1 * z0 -
		 c110 * x0 * y0 * z1 + c010 * x1 * y0 * z1 + c100 * x0 * y1 * z1 - c000 * x1 * y1 * z1) / det;

	float a1 =
		(c011 * y0 * z0 - c111 * y0 * z0 - c001 * y1 * z0 + c101 * y1 * z0 - c010 * y0 * z1 +
		 c110 * y0 * z1 + c000 * y1 * z1 - c100 * y1 * z1) / det;

	float a2 =
		(c101 * x0 * z0 - c111 * x0 * z0 - c001 * x1 * z0 + c011 * x1 * z0 - c100 * x0 * z1 +
		 c110 * x0 * z1 + c000 * x1 * z1 - c010 * x1 * z1) / det;

	float a3 =
		(c110 * x0 * y0 - c111 * x0 * y0 - c010 * x1 * y0 + c011 * x1 * y0 - c100 * x0 * y1 +
		 c101 * x0 * y1 + c000 * x1 * y1 - c001 * x1 * y1) / det;

	float a4 =
		(c001 * z0 - c011 * z0 - c101 * z0 + c111 * z0 - c000 * z1 + c010 * z1 + c100 * z1 -
		 c110 * z1) / det;

	float a5 =
		(c010 * y0 - c011 * y0 - c110 * y0 + c111 * y0 - c000 * y1 + c001 * y1 + c100 * y1 -
		 c101 * y1) / det;

	float a6 =
		(c100 * x0 - c101 * x0 - c110 * x0 + c111 * x0 - c000 * x1 + c001 * x1 + c010 * x1 -
		 c011 * x1) / det;

	float a7 =
		(c000 - c001 - c010 + c011 - c100 + c101 + c110 - c111) / det;


	return a0 + a1 * x + a2 * y + a3 * z + a4 * x * y + a5 * x * z + a6 * y * z + a7 * x * y * z;
}
