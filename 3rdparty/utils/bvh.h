#ifndef __BVH_H__
#define __BVH_H__

#include <float.h>

struct AABB {
	AABB()
	{
		min[0] = min[1] = min[2] = FLT_MAX;
		max[0] = max[1] = max[2] = -FLT_MAX;
	}
	float min[3];
	float max[3];
};

struct BVHNode {
	BVHNode():right_(0), left_(0), radius_(0), pos_(0), num_(0) {}

	AABB aabb_;
	BVHNode* right_;
	BVHNode* left_;

	float* radius_;
	float (*pos_)[3];
	int num_;
};

//BVHNode* construct_bvh(float* radius, float (*pos)[3], int num, int offset, int depth);
BVHNode* construct_bvh(BVHNode* pnode, float* radius, float (*pos)[3], int num, int offset, int depth, float* oradius, float (*opos)[3]);
BVHNode* create_bvh_node();

#endif // __BVH_H__