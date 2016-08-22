#ifdef DEBUG
#include <assert.h>
#include <memory.h>
#endif

#include "utils/spline.h"
#include "utils/vec.h"

/* --------------------------------------------------------------------- */
/* ---------------------------------- NURB Curve ----------------------- */
/* --------------------------------------------------------------------- */

//NURBCurve::NURBCurve():
//_cvs(0), _num_cvs(0), _knots(0), _num_knots(0)
//{
//
//}

//NURBCurve::NURBCurve(const D3DXVECTOR3 *const cvs, const size_t num_cvs)
//{
//	assert(num_cvs>0 && cvs);
//	_cvs = new D3DXVECTOR3[num_cvs];
//	memcpy(_cvs, cvs, sizeof(D3DXVECTOR3)*num_cvs);
//	_num_cvs = num_cvs;
//}

//NURBCurve::~NURBCurve()
//{
//	//delete[] _cvs;
//	//delete[] _knots;
//}

//------------------------------------------------------------	
// To handle the clamped curves i imagine the curve to have 3 
// extra control points at the beginning of the curve, and 3 extra
// at the end. To simply the iteration of the points, i start
// the indices at -3. Obviously if i use that value as an array
// index it will crash horribly. Instead i use a function that
// returns either the first, or last point if the index requested
// is out of range. This basically simplifies our curve calculation
// greatly!!
vec3 NURBCurve::get_cv(int i) const
{
	// return 1st point
	if (i<0) {
		return	_cvs[0];
	}
	// return last point
	if (i<int(_num_cvs))
		return _cvs[i];

	return _cvs[_num_cvs-1];
}

void NURBCurve::get_point(float t, vec3* pp) const
{
	// sum the effect of all CV's on the curve at this point to 
	// get the evaluated curve point
	// 
	for(unsigned int i=0;i!=_num_cvs;++i)
	{
		// calculate the effect of this point on the curve
		float val = this->Cox_De_Boor(t, i, _degree,_knots);

		// sum effect of CV on this part of the curve
		if(val>0.001f)
			*pp += val * _cvs[i];
	}
}

float NURBCurve::Cox_De_Boor(float u, int i, int k, const float* knots) const
{
	if(k==1)
	{
		if( _knots[i] <= u && u <= _knots[i+1] ) {
			return 1.0f;
		}
		return 0.0f;
	}

	float denom1 = _knots[i+k-1] - _knots[i];
	float denom2 = _knots[i+k] - _knots[i+1];
	float eq1=0, eq2=0;
	if(denom1>0)
		eq1 = ((u - _knots[i])/denom1) * this->Cox_De_Boor(u, i, k-1, _knots);
	
	if(denom2>0)
		eq2 = (_knots[i+k]-u)/denom2 * this->Cox_De_Boor(u, i+1, k-1, _knots);
	
	return eq1+eq2;
}

NURBCurve* makeCurve(int degree, int numCvs, vec3* pcvs, int numKnots, float* pknots)
{
	static NURBCurve myCurve1;
	myCurve1._degree = degree;
	myCurve1._num_cvs = numCvs;
	myCurve1._cvs = pcvs;
	myCurve1._num_knots = numKnots;
	myCurve1._knots = pknots;
	return &myCurve1;
}

//NURBCurve* makeCurve2(int numCvs, D3DXVECTOR3* pcvs)
//{
//	NURBCurve* pcurve = new NURBCurve(numCvs, pcvs);
//	pcurve->_num_cvs = numCvs;
//	pcurve->_cvs = pcvs;
//	return pcurve;
//}

float catmull_rom_get_tangent(float a, float b)
{
	return 0.5f*(b - a);
}

float hermite_get_value(float aa, float a, float b, float bb, float t, float k1, float k2)
{
	float t2 = t*t;
	float t3 = t2*t;
	float h1 =  2*t3 - 3*t2 + 1;
	float h2 = -2*t3 + 3*t2;
	float h3 =    t3 - 2*t2 + t;
	float h4 =    t3 - t2;

	// configure object to accept function ptr to tangent
	float tang1 = catmull_rom_get_tangent(aa, b)*k1;
	float tang2 = catmull_rom_get_tangent(a, bb)*k2;
	float p = h1*a + h2*b + h3*tang1 + h4*tang2;
	return p;
}

vec4 hermiteLerp(const vec4& p, const vec4& s, const vec4& e, const vec4& n, const float t, const vec4& k1, const vec4& k2)
{
	return vec4(
		hermite_get_value(p.x, s.x, e.x, n.x, t, k1.x, k2.x),
		hermite_get_value(p.y, s.y, e.y, n.y, t, k1.y, k2.y),
		hermite_get_value(p.z, s.z, e.z, n.z, t, k1.z, k2.z),
		hermite_get_value(p.w, s.w, e.w, n.w, t, k1.w, k2.w)
	);
}
