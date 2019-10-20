#include "Matrix4.h"

#define id(i, j) 4 * i + j
#define singularity 0

Matrix4::Matrix4()
{
}

Matrix4::Matrix4(float* elems)
{
	float* e = elements;
	e[0] = elems[0];	e[1] = elems[1];	e[2] = elems[2];	e[3] = elems[3];
	e[4] = elems[4];	e[5] = elems[5];	e[6] = elems[6];	e[7] = elems[7];
	e[8] = elems[8];	e[9] = elems[9];	e[10] = elems[10];	e[11] = elems[11];
	e[12] = elems[12];	e[13] = elems[13];	e[14] = elems[14];	e[15] = elems[15];
}

Matrix4::~Matrix4()
{
}

void Matrix4::copy(Matrix4& other)
{
	float* elems = other.elements;
	float* e = elements;
	e[0] = elems[0];	e[1] = elems[1];	e[2] = elems[2];	e[3] = elems[3];
	e[4] = elems[4];	e[5] = elems[5];	e[6] = elems[6];	e[7] = elems[7];
	e[8] = elems[8];	e[9] = elems[9];	e[10] = elems[10];	e[11] = elems[11];
	e[12] = elems[12];	e[13] = elems[13];	e[14] = elems[14];	e[15] = elems[15];
}

Matrix4 Matrix4::clone()
{
	Matrix4 result = Matrix4();
	result.copy(*this);
	return result;
}


Matrix3 Matrix4::getSubMatrix3()
{
	float* e = elements;
	float resultElems[9] = {
		e[0],	e[4],	e[8],
		e[1],	e[5],	e[9],
		e[2],	e[6],	e[10]
	};
	return Matrix3(resultElems);
}

bool Matrix4::isIdentity()
{
	float* e = elements;
	return (
		e[0] == 1.0f  &&   e[1] == 0.0f  &&   e[2] == 0.0f  &&   e[3] == 0.0f &&
		e[4] == 0.0f  &&   e[5] == 1.0f  &&   e[6] == 0.0f  &&   e[7] == 0.0f &&
		e[8] == 0.0f  &&   e[9] == 0.0f  &&  e[10] == 1.0f  &&  e[11] == 0.0f &&
		e[12] == 0.0f &&  e[13] == 0.0f  &&  e[14] == 0.0f  &&  e[15] == 1.0f
	);
}

void Matrix4::setToIdentity()
{
	float* e = elements;
	e[0] = 1.0f;	e[1] = 0.0f;	e[2] = 0.0f;	e[3] = 0.0f;
	e[4] = 0.0f;	e[5] = 1.0f;	e[6] = 0.0f;	e[7] = 0.0f;
	e[8] = 0.0f;	e[9] = 0.0f;	e[10] = 1.0f;	e[11] = 0.0f;
	e[12] = 0.0f;	e[13] = 0.0f;	e[14] = 0.0f;	e[15] = 1.0f;
}

void Matrix4::transpose()
{
	Matrix4 result = Matrix4();
	float* e = result.elements;
	float* elems = elements;
	e[0] = elems[0];	e[1] = elems[4];	e[2] = elems[8];	e[3] = elems[12];
	e[4] = elems[1];	e[5] = elems[5];	e[6] = elems[9];	e[7] = elems[13];
	e[8] = elems[2];	e[9] = elems[6];	e[10] = elems[10];	e[11] = elems[14];
	e[12] = elems[3];	e[13] = elems[7];	e[14] = elems[11];	e[15] = elems[15];
}

void Matrix4::multiplyScalar(float scalar)
{
	float* e = this->elements;
	for (int i = 0; i < 16; i++) {
		e[i] *= scalar;
	}
}

float Matrix4::determinant()
{
	float* m = elements;
	float value =
		m[id(0, 3)] * m[id(1, 2)] * m[id(2, 1)] * m[id(3, 0)] - m[id(0, 2)] * m[id(1, 3)] * m[id(2, 1)] * m[id(3, 0)] - m[id(0, 3)] * m[id(1, 1)] * m[id(2, 2)] * m[id(3, 0)] + m[id(0, 1)] * m[id(1, 3)] * m[id(2, 2)] * m[id(3, 0)] +
		m[id(0, 2)] * m[id(1, 1)] * m[id(2, 3)] * m[id(3, 0)] - m[id(0, 1)] * m[id(1, 2)] * m[id(2, 3)] * m[id(3, 0)] - m[id(0, 3)] * m[id(1, 2)] * m[id(2, 0)] * m[id(3, 1)] + m[id(0, 2)] * m[id(1, 3)] * m[id(2, 0)] * m[id(3, 1)] +
		m[id(0, 3)] * m[id(1, 0)] * m[id(2, 2)] * m[id(3, 1)] - m[id(0, 0)] * m[id(1, 3)] * m[id(2, 2)] * m[id(3, 1)] - m[id(0, 2)] * m[id(1, 0)] * m[id(2, 3)] * m[id(3, 1)] + m[id(0, 0)] * m[id(1, 2)] * m[id(2, 3)] * m[id(3, 1)] +
		m[id(0, 3)] * m[id(1, 1)] * m[id(2, 0)] * m[id(3, 2)] - m[id(0, 1)] * m[id(1, 3)] * m[id(2, 0)] * m[id(3, 2)] - m[id(0, 3)] * m[id(1, 0)] * m[id(2, 1)] * m[id(3, 2)] + m[id(0, 0)] * m[id(1, 3)] * m[id(2, 1)] * m[id(3, 2)] +
		m[id(0, 1)] * m[id(1, 0)] * m[id(2, 3)] * m[id(3, 2)] - m[id(0, 0)] * m[id(1, 1)] * m[id(2, 3)] * m[id(3, 2)] - m[id(0, 2)] * m[id(1, 1)] * m[id(2, 0)] * m[id(3, 3)] + m[id(0, 1)] * m[id(1, 2)] * m[id(2, 0)] * m[id(3, 3)] +
		m[id(0, 2)] * m[id(1, 0)] * m[id(2, 1)] * m[id(3, 3)] - m[id(0, 0)] * m[id(1, 2)] * m[id(2, 1)] * m[id(3, 3)] - m[id(0, 1)] * m[id(1, 0)] * m[id(2, 2)] * m[id(3, 3)] + m[id(0, 0)] * m[id(1, 1)] * m[id(2, 2)] * m[id(3, 3)];
	return value;
}

Matrix4 Matrix4::getInverse(Matrix4& from)
{
	float* m = from.elements;
	float* e = elements;
	float det = from.determinant();
	Matrix4 result = Matrix4();
	int ex = singularity;
	try {
		if (fabs(det) < FLT_MIN) {
			throw singularity;
		} else {
			float invDet = 1.0f / from.determinant();

			e[id(0, 0)] = m[id(1, 2)] * m[id(2, 3)] * m[id(3, 1)] - m[id(1, 3)] * m[id(2, 2)] * m[id(3, 1)] + m[id(1, 3)] * m[id(2, 1)] * m[id(3, 2)] - m[id(1, 1)] * m[id(2, 3)] * m[id(3, 2)] - m[id(1, 2)] * m[id(2, 1)] * m[id(3, 3)] + m[id(1, 1)] * m[id(2, 2)] * m[id(3, 3)];
			e[id(0, 1)] = m[id(0, 3)] * m[id(2, 2)] * m[id(3, 1)] - m[id(0, 2)] * m[id(2, 3)] * m[id(3, 1)] - m[id(0, 3)] * m[id(2, 1)] * m[id(3, 2)] + m[id(0, 1)] * m[id(2, 3)] * m[id(3, 2)] + m[id(0, 2)] * m[id(2, 1)] * m[id(3, 3)] - m[id(0, 1)] * m[id(2, 2)] * m[id(3, 3)];
			e[id(0, 2)] = m[id(0, 2)] * m[id(1, 3)] * m[id(3, 1)] - m[id(0, 3)] * m[id(1, 2)] * m[id(3, 1)] + m[id(0, 3)] * m[id(1, 1)] * m[id(3, 2)] - m[id(0, 1)] * m[id(1, 3)] * m[id(3, 2)] - m[id(0, 2)] * m[id(1, 1)] * m[id(3, 3)] + m[id(0, 1)] * m[id(1, 2)] * m[id(3, 3)];
			e[id(0, 3)] = m[id(0, 3)] * m[id(1, 2)] * m[id(2, 1)] - m[id(0, 2)] * m[id(1, 3)] * m[id(2, 1)] - m[id(0, 3)] * m[id(1, 1)] * m[id(2, 2)] + m[id(0, 1)] * m[id(1, 3)] * m[id(2, 2)] + m[id(0, 2)] * m[id(1, 1)] * m[id(2, 3)] - m[id(0, 1)] * m[id(1, 2)] * m[id(2, 3)];
			e[id(1, 0)] = m[id(1, 3)] * m[id(2, 2)] * m[id(3, 0)] - m[id(1, 2)] * m[id(2, 3)] * m[id(3, 0)] - m[id(1, 3)] * m[id(2, 0)] * m[id(3, 2)] + m[id(1, 0)] * m[id(2, 3)] * m[id(3, 2)] + m[id(1, 2)] * m[id(2, 0)] * m[id(3, 3)] - m[id(1, 0)] * m[id(2, 2)] * m[id(3, 3)];
			e[id(1, 1)] = m[id(0, 2)] * m[id(2, 3)] * m[id(3, 0)] - m[id(0, 3)] * m[id(2, 2)] * m[id(3, 0)] + m[id(0, 3)] * m[id(2, 0)] * m[id(3, 2)] - m[id(0, 0)] * m[id(2, 3)] * m[id(3, 2)] - m[id(0, 2)] * m[id(2, 0)] * m[id(3, 3)] + m[id(0, 0)] * m[id(2, 2)] * m[id(3, 3)];
			e[id(1, 2)] = m[id(0, 3)] * m[id(1, 2)] * m[id(3, 0)] - m[id(0, 2)] * m[id(1, 3)] * m[id(3, 0)] - m[id(0, 3)] * m[id(1, 0)] * m[id(3, 2)] + m[id(0, 0)] * m[id(1, 3)] * m[id(3, 2)] + m[id(0, 2)] * m[id(1, 0)] * m[id(3, 3)] - m[id(0, 0)] * m[id(1, 2)] * m[id(3, 3)];
			e[id(1, 3)] = m[id(0, 2)] * m[id(1, 3)] * m[id(2, 0)] - m[id(0, 3)] * m[id(1, 2)] * m[id(2, 0)] + m[id(0, 3)] * m[id(1, 0)] * m[id(2, 2)] - m[id(0, 0)] * m[id(1, 3)] * m[id(2, 2)] - m[id(0, 2)] * m[id(1, 0)] * m[id(2, 3)] + m[id(0, 0)] * m[id(1, 2)] * m[id(2, 3)];
			e[id(2, 0)] = m[id(1, 1)] * m[id(2, 3)] * m[id(3, 0)] - m[id(1, 3)] * m[id(2, 1)] * m[id(3, 0)] + m[id(1, 3)] * m[id(2, 0)] * m[id(3, 1)] - m[id(1, 0)] * m[id(2, 3)] * m[id(3, 1)] - m[id(1, 1)] * m[id(2, 0)] * m[id(3, 3)] + m[id(1, 0)] * m[id(2, 1)] * m[id(3, 3)];
			e[id(2, 1)] = m[id(0, 3)] * m[id(2, 1)] * m[id(3, 0)] - m[id(0, 1)] * m[id(2, 3)] * m[id(3, 0)] - m[id(0, 3)] * m[id(2, 0)] * m[id(3, 1)] + m[id(0, 0)] * m[id(2, 3)] * m[id(3, 1)] + m[id(0, 1)] * m[id(2, 0)] * m[id(3, 3)] - m[id(0, 0)] * m[id(2, 1)] * m[id(3, 3)];
			e[id(2, 2)] = m[id(0, 1)] * m[id(1, 3)] * m[id(3, 0)] - m[id(0, 3)] * m[id(1, 1)] * m[id(3, 0)] + m[id(0, 3)] * m[id(1, 0)] * m[id(3, 1)] - m[id(0, 0)] * m[id(1, 3)] * m[id(3, 1)] - m[id(0, 1)] * m[id(1, 0)] * m[id(3, 3)] + m[id(0, 0)] * m[id(1, 1)] * m[id(3, 3)];
			e[id(2, 3)] = m[id(0, 3)] * m[id(1, 1)] * m[id(2, 0)] - m[id(0, 1)] * m[id(1, 3)] * m[id(2, 0)] - m[id(0, 3)] * m[id(1, 0)] * m[id(2, 1)] + m[id(0, 0)] * m[id(1, 3)] * m[id(2, 1)] + m[id(0, 1)] * m[id(1, 0)] * m[id(2, 3)] - m[id(0, 0)] * m[id(1, 1)] * m[id(2, 3)];
			e[id(3, 0)] = m[id(1, 2)] * m[id(2, 1)] * m[id(3, 0)] - m[id(1, 1)] * m[id(2, 2)] * m[id(3, 0)] - m[id(1, 2)] * m[id(2, 0)] * m[id(3, 1)] + m[id(1, 0)] * m[id(2, 2)] * m[id(3, 1)] + m[id(1, 1)] * m[id(2, 0)] * m[id(3, 2)] - m[id(1, 0)] * m[id(2, 1)] * m[id(3, 2)];
			e[id(3, 1)] = m[id(0, 1)] * m[id(2, 2)] * m[id(3, 0)] - m[id(0, 2)] * m[id(2, 1)] * m[id(3, 0)] + m[id(0, 2)] * m[id(2, 0)] * m[id(3, 1)] - m[id(0, 0)] * m[id(2, 2)] * m[id(3, 1)] - m[id(0, 1)] * m[id(2, 0)] * m[id(3, 2)] + m[id(0, 0)] * m[id(2, 1)] * m[id(3, 2)];
			e[id(3, 2)] = m[id(0, 2)] * m[id(1, 1)] * m[id(3, 0)] - m[id(0, 1)] * m[id(1, 2)] * m[id(3, 0)] - m[id(0, 2)] * m[id(1, 0)] * m[id(3, 1)] + m[id(0, 0)] * m[id(1, 2)] * m[id(3, 1)] + m[id(0, 1)] * m[id(1, 0)] * m[id(3, 2)] - m[id(0, 0)] * m[id(1, 1)] * m[id(3, 2)];
			e[id(3, 3)] = m[id(0, 1)] * m[id(1, 2)] * m[id(2, 0)] - m[id(0, 2)] * m[id(1, 1)] * m[id(2, 0)] + m[id(0, 2)] * m[id(1, 0)] * m[id(2, 1)] - m[id(0, 0)] * m[id(1, 2)] * m[id(2, 1)] - m[id(0, 1)] * m[id(1, 0)] * m[id(2, 2)] + m[id(0, 0)] * m[id(1, 1)] * m[id(2, 2)];
			result = *this;
			return invDet * result;
		}
	}
	catch (int ex) {
		std::cout << "det(A) = 0.0! Unable to invert a singular matrix!" << std::endl;
	}

	return result;
}



Matrix4 Matrix4::operator+(Matrix4 other)
{
	Matrix4 result = Matrix4();
	for (int i = 0; i < 16; i++) {
		result.elements[i] = elements[i] + other.elements[i];
	}
	return result;
}

Matrix4 Matrix4::operator-(Matrix4 other)
{
	Matrix4 result = Matrix4();
	for (int i = 0; i < 16; i++) {
		result.elements[i] = elements[i] - other.elements[i];
	}
	return result;
}

Matrix4 Matrix4::operator*(Matrix4 other)
{
	Matrix4 result = Matrix4();
	float* re = result.elements;
	float* ae = elements;
	float* be = other.elements;

	float a11 = ae[0], a12 = ae[4], a13 = ae[8], a14 = ae[12];
	float a21 = ae[1], a22 = ae[5], a23 = ae[9], a24 = ae[13];
	float a31 = ae[2], a32 = ae[6], a33 = ae[10], a34 = ae[14];
	float a41 = ae[3], a42 = ae[7], a43 = ae[11], a44 = ae[15];

	float b11 = be[0], b12 = be[4], b13 = be[8], b14 = be[12];
	float b21 = be[1], b22 = be[5], b23 = be[9], b24 = be[13];
	float b31 = be[2], b32 = be[6], b33 = be[10], b34 = be[14];
	float b41 = be[3], b42 = be[7], b43 = be[11], b44 = be[15];

	re[0] = a11 * b11 + a12 * b21 + a13 * b31 + a14 * b41;
	re[4] = a11 * b12 + a12 * b22 + a13 * b32 + a14 * b42;
	re[8] = a11 * b13 + a12 * b23 + a13 * b33 + a14 * b43;
	re[12] = a11 * b14 + a12 * b24 + a13 * b34 + a14 * b44;

	re[1] = a21 * b11 + a22 * b21 + a23 * b31 + a24 * b41;
	re[5] = a21 * b12 + a22 * b22 + a23 * b32 + a24 * b42;
	re[9] = a21 * b13 + a22 * b23 + a23 * b33 + a24 * b43;
	re[13] = a21 * b14 + a22 * b24 + a23 * b34 + a24 * b44;

	re[2] = a31 * b11 + a32 * b21 + a33 * b31 + a34 * b41;
	re[6] = a31 * b12 + a32 * b22 + a33 * b32 + a34 * b42;
	re[10] = a31 * b13 + a32 * b23 + a33 * b33 + a34 * b43;
	re[14] = a31 * b14 + a32 * b24 + a33 * b34 + a34 * b44;

	re[3] = a41 * b11 + a42 * b21 + a43 * b31 + a44 * b41;
	re[7] = a41 * b12 + a42 * b22 + a43 * b32 + a44 * b42;
	re[11] = a41 * b13 + a42 * b23 + a43 * b33 + a44 * b43;
	re[15] = a41 * b14 + a42 * b24 + a43 * b34 + a44 * b44;

	return result;
}

Matrix4 operator*(float scalar, Matrix4 m)
{
	Matrix4 result = m.clone();
	result.multiplyScalar(scalar);
	return result;
}

Matrix4 inverse(Matrix4& m)
{
	Matrix4 result = Matrix4().getInverse(m);
	return result;
}

Matrix4 transpose(Matrix4& m)
{
	Matrix4 result = m.clone();
	result.transpose();
	return result;
}
