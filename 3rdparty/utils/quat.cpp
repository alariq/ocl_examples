#include "utils/vec.h"
#include "utils/quat.h"

#include <memory.h>

JointMat::JointMat()
{
	memset(mat, 0, sizeof(mat));
}


void ConvertJointQuatsToJointMats( JointMat *jointMats, const JointQuat *jointQuats, const int numJoints ) {
	for ( int i = 0; i < numJoints; i++ ) { 

		const vec4 tr = jointQuats[i].t; 
		const Quaternion& q = jointQuats[i].q; 
		float *m = jointMats[i].mat; 

		m[0*4+3] = tr[0]; 
		m[1*4+3] = tr[1]; 
		m[2*4+3] = tr[2]; 
		float x2 = q.x + q.x; 
		float y2 = q.y + q.y; 
		float z2 = q.z + q.z; 
		{ 
			float xx2 = q.x * x2; 
			float yy2 = q.y * y2; 
			float zz2 = q.z * z2; 
			m[0*4+0] = 1.0f - yy2 - zz2; 
			m[1*4+1] = 1.0f - xx2 - zz2; 
			m[2*4+2] = 1.0f - xx2 - yy2; 
		} 
		{ 
			float yz2 = q.y * z2; 
			float wx2 = q.w * x2; 
			m[2*4+1] = yz2 - wx2; 
			m[1*4+2] = yz2 + wx2; 
		} 
		{ 
			float xy2 = q.x * y2; 
			float wz2 = q.w * z2;
			m[1*4+0] = xy2 - wz2; 
			m[0*4+1] = xy2 + wz2; 
		} 
		{ 
			float xz2 = q.x * z2; 
			float wy2 = q.w * y2;
			m[0*4+2] = xz2 - wy2; 
			m[2*4+0] = xz2 + wy2; 
		}
	} 
}

float ReciprocalSqrt( float x )
{ 
	long i; 
	float y, r; 
	y = x * 0.5f; 
	i = *(long *)( &x ); 
	i = 0x5f3759df - ( i >> 1 ); 
	r = *(float *)( &i ); 
	r = r * ( 1.5f - r * r * y ); 
	return r;
}

void ConvertJointMatsToJointQuats( JointQuat *jointQuats, const JointMat *jointMats, const int numJoints ) {
	for ( int i = 0; i < numJoints; i++ ) { 
		Quaternion& q = jointQuats[i].q;
		vec4& tr = jointQuats[i].t; 

		const float *m = jointMats[i].mat;
		if ( m[0 * 4 + 0] + m[1 * 4 + 1] + m[2 * 4 + 2] > 0.0f ) { 
			float t = + m[0 * 4 + 0] + m[1 * 4 + 1] + m[2 * 4 + 2] + 1.0f; 
			float s = ReciprocalSqrt( t ) * 0.5f; 
			q.w = s * t; 
			q.z = ( m[0 * 4 + 1] - m[1 * 4 + 0] ) * s; 
			q.y = ( m[2 * 4 + 0] - m[0 * 4 + 2] ) * s; 
			q.x = ( m[1 * 4 + 2] - m[2 * 4 + 1] ) * s; 
		} else if ( m[0 * 4 + 0] > m[1 * 4 + 1] && m[0 * 4 + 0] > m[2 * 4 + 2] ) { 
			float t = + m[0 * 4 + 0] - m[1 * 4 + 1] - m[2 * 4 + 2] + 1.0f; 
			float s = ReciprocalSqrt( t ) * 0.5f; 
			q.x = s * t; 
			q.y = ( m[0 * 4 + 1] + m[1 * 4 + 0] ) * s; 
			q.z = ( m[2 * 4 + 0] + m[0 * 4 + 2] ) * s; 
			q.w = ( m[1 * 4 + 2] - m[2 * 4 + 1] ) * s; 
		} else if ( m[1 * 4 + 1] > m[2 * 4 + 2] ) { 
			float t = - m[0 * 4 + 0] + m[1 * 4 + 1] - m[2 * 4 + 2] + 1.0f; 
			float s = ReciprocalSqrt( t ) * 0.5f; 
			q.y = s * t; 
			q.x = ( m[0 * 4 + 1] + m[1 * 4 + 0] ) * s; 
			q.w = ( m[2 * 4 + 0] - m[0 * 4 + 2] ) * s; 
			q.z = ( m[1 * 4 + 2] + m[2 * 4 + 1] ) * s; 
		} else { 
			float t = - m[0 * 4 + 0] - m[1 * 4 + 1] + m[2 * 4 + 2] + 1.0f; 
			float s = ReciprocalSqrt( t ) * 0.5f; 
			q.z = s * t; 
			q.w = ( m[0 * 4 + 1] - m[1 * 4 + 0] ) * s; 
			q.x = ( m[2 * 4 + 0] + m[0 * 4 + 2] ) * s; 
			q.y = ( m[1 * 4 + 2] + m[2 * 4 + 1] ) * s; 
		} 
			tr.x = m[0 * 4 + 3]; 
			tr.y = m[1 * 4 + 3]; 
			tr.z = m[2 * 4 + 3]; 
			tr.w = 0.0f;
		} 
}
#define DELTA 0.00001f

void slerpQuat(const Quaternion* from, const Quaternion* to, const float t, Quaternion* res)
{
	float           to1[4];
	float        omega, cosom, sinom, scale0, scale1;
	// calc cosine
	cosom = from->x * to->x + from->y * to->y + from->z * to->z + from->w * to->w;
	// adjust signs (if necessary)
	if ( cosom <0.0 )
	{
		cosom = -cosom;
		to1[0] = - to->x;
		to1[1] = - to->y;
		to1[2] = - to->z;
		to1[3] = - to->w;
	} else  {
		to1[0] = to->x;
		to1[1] = to->y;
		to1[2] = to->z;
		to1[3] = to->w;
	}

	// calculate coefficients
	if ( (1.0 - cosom) > DELTA ) {
		// standard case (slerp)
		omega = acos(cosom);
		sinom = sin(omega);
		scale0 = sin((1.0f - t) * omega) / sinom;
		scale1 = sin(t * omega) / sinom;
	} else {        
		// "from" and "to" quaternions are very close 
		//  ... so we can do a linear interpolation
		scale0 = 1.0f - t;
		scale1 = t;
	}
	// calculate final values
	res->x = scale0 * from->x + scale1 * to1[0];
	res->y = scale0 * from->y + scale1 * to1[1];
	res->z = scale0 * from->z + scale1 * to1[2];
	res->w = scale0 * from->w + scale1 * to1[3];
}

void pointTransformByQuat(vec3* p, const Quaternion* q)
{
	float s1 = 2.0f/(q->x*q->x + q->y*q->y + q->z*q->z + q->w*q->w);
	float x2 = q->x*q->x;
	float y2 = q->y*q->y;
	float z2 = q->z*q->z;

	float xy = q->x*q->y;
	float yz = q->y*q->z;
	float xz = q->x*q->z;

	float wx = q->w*q->x;
	float wy = q->w*q->y;
	float wz = q->w*q->z;


	vec3 axis1( 1.0f - s1*(y2+z2), s1*(xy - wz), s1*(xz+wy));
	vec3 axis2( s1*(xy+wz ), 1.0f - s1*(x2+z2), s1*(yz - wx));
	vec3 axis3( s1*( xz - wy ), s1*(yz+wx), 1.0f - s1*(x2+y2));

	vec3 t = *p;
	p->x = dot(axis1, t);
	p->y = dot(axis2, t);
	p->z = dot(axis3, t);
	
}