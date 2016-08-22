#ifndef __QUAT_H__
#define __QUAT_H__

#include "utils/vec.h"



struct Quaternion {
	Quaternion(float xx, float yy, float zz, float ww):x(xx), y(yy), z(zz), w(ww) {}
	Quaternion():x(0),y(0),z(0),w(0) {}
	float x,y,z,w;
};

struct JointMat {
	JointMat();
	float mat[3*4];
};

struct JointQuat { 
	Quaternion q;
	vec4 t; 
};

void ConvertJointQuatsToJointMats(JointMat *jointMats, const JointQuat *jointQuats, const int numJoints );
void ConvertJointMatsToJointQuats( JointQuat *jointQuats, const JointMat *jointMats, const int numJoints );
void pointTransformByQuat(vec3* p, const Quaternion* q);
void slerpQuat(const Quaternion* from, const Quaternion* to, const float t, Quaternion* res);
	
#endif // __QUAT_H__
