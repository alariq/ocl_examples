#ifndef _SPLINE_H_
#define _SPLINE_H_

#include "utils/vec.h"

struct NURBCurve {
	public:
		//NURBCurve();
		//NURBCurve(const D3DXVECTOR3 *const cvs, const size_t num_cvs);
		//~NURBCurve();
		// t[0..1]
		vec3 getValue(float t);

		vec3 get_cv(int i) const;
		void get_point(float t, vec3* p) const;
		float Cox_De_Boor(float u, int i, int k, const float* knots) const;
		unsigned int _num_cvs;
		vec3* _cvs;
		unsigned int _num_knots;
		float* _knots;
		unsigned int _degree;
};

NURBCurve* makeCurve(int degree, int numCvs, vec3* pcvs, int numKnots, float* pknots);


float catmull_rom_get_tangent(float a, float b);
float hermite_get_value(float aa, float a, float b, float bb, float t, float k1=1, float k2=1);

vec4 hermiteLerp(const vec4& p, const vec4& s, const vec4& e, const vec4& n, const float t, const vec4& k1, const vec4& k2);


#endif //_SPLINE_H_
