#include "bvh.h"
#include <stdint.h>
#include <cassert>

// "Insert" a 0 bit after each of the 16 low bits of x
uint32_t Part1By1(uint32_t x)
{
  x &= 0x0000ffff;                  // x = ---- ---- ---- ---- fedc ba98 7654 3210
  x = (x ^ (x <<  8)) & 0x00ff00ff; // x = ---- ---- fedc ba98 ---- ---- 7654 3210
  x = (x ^ (x <<  4)) & 0x0f0f0f0f; // x = ---- fedc ---- ba98 ---- 7654 ---- 3210
  x = (x ^ (x <<  2)) & 0x33333333; // x = --fe --dc --ba --98 --76 --54 --32 --10
  x = (x ^ (x <<  1)) & 0x55555555; // x = -f-e -d-c -b-a -9-8 -7-6 -5-4 -3-2 -1-0
  return x;
}

// "Insert" two 0 bits after each of the 10 low bits of x
uint32_t Part1By2(uint32_t x)
{
  x &= 0x000003ff;                  // x = ---- ---- ---- ---- ---- --98 7654 3210
  x = (x ^ (x << 16)) & 0xff0000ff; // x = ---- --98 ---- ---- ---- ---- 7654 3210
  x = (x ^ (x <<  8)) & 0x0300f00f; // x = ---- --98 ---- ---- 7654 ---- ---- 3210
  x = (x ^ (x <<  4)) & 0x030c30c3; // x = ---- --98 ---- 76-- --54 ---- 32-- --10
  x = (x ^ (x <<  2)) & 0x09249249; // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
  return x;
}

uint32_t EncodeMorton2(uint32_t x, uint32_t y)
{
  return (Part1By1(y) << 1) + Part1By1(x);
}

uint32_t EncodeMorton3(uint32_t x, uint32_t y, uint32_t z)
{
  return (Part1By2(z) << 2) + (Part1By2(y) << 1) + Part1By2(x);
}

#define MSB_MORTON	29

int classify(uint32_t mc, int i)
{
	return 0;
}

BVHNode* create_bvh_node()
{
	return new BVHNode();
}

#define MAX_DEPTH	3	


void swap(float* radius, float (*pos)[3], int src, int dst)
{
	float r = radius[dst];
	float x = pos[dst][0];
	float y = pos[dst][1];
	float z = pos[dst][2];

	radius[dst] = radius[src];
	pos[dst][0] = pos[src][0];
	pos[dst][1] = pos[src][1];
	pos[dst][2] = pos[src][2];

	radius[dst] = r;
	pos[dst][0] = x;
	pos[dst][1] = y;
	pos[dst][2] = z;
}

void get_sphere_aabb(AABB* paabb, float r, float x, float y, float z)
{
	assert(paabb);
	paabb->min[0] = x - r;
	paabb->max[0] = x + r;
	paabb->min[1] = y - r;
	paabb->max[1] = y + r;
	paabb->min[2] = z - r;
	paabb->max[2] = z + r;
}
// one = one U two
void union_aabb(AABB* one, AABB* two)
{
	assert(one && two);

	one->min[0] = one->min[0] < two->min[0] ? one->min[0] : two->min[0];
	one->max[0] = one->max[0] > two->max[0] ? one->max[0] : two->max[0];
	one->min[1] = one->min[1] < two->min[1] ? one->min[1] : two->min[1];
	one->max[1] = one->max[1] > two->max[1] ? one->max[1] : two->max[1];
	one->min[2] = one->min[2] < two->min[2] ? one->min[2] : two->min[2];
	one->max[2] = one->max[2] > two->max[2] ? one->max[2] : two->max[2];
}

BVHNode* construct_bvh(BVHNode* pnode, float* radius, float (*pos)[3], int num, int offset, int depth, float* oradius, float (*opos)[3])
{
	int left = 0;
	int right = 0;
	AABB laabb, raabb;

	for(int i=offset;i<offset + num;++i)
	{
		uint32_t morton_code = EncodeMorton3(  (uint32_t)(pos[i][0] + 0.5f), (uint32_t)(pos[i][1] + 0.5f), (uint32_t)(pos[i][2] + 0.5f) );
		int side = (morton_code & (1<<(MSB_MORTON - depth))) >> (MSB_MORTON - depth);
		right += side;
		left += (1-side);
		//if(side==0 && i > left - 1 - offset)
		//{
			//swap(radius, pos, i, offset + left -1);
		//}
		if(side==0)
		{
			AABB l;
			oradius[offset + left-1] = radius[i];
			opos[offset + left-1][0] = pos[i][0];
			opos[offset + left-1][1] = pos[i][1];
			opos[offset + left-1][2] = pos[i][2];
			get_sphere_aabb(&l, radius[i], pos[i][0], pos[i][1], pos[i][2]);
			union_aabb(&laabb, &l);
		}
		else
		{
			AABB r;
			oradius[offset + num - right] = radius[i];
			opos[offset + num - right][0] = pos[i][0];
			opos[offset + num - right][1] = pos[i][1];
			opos[offset + num - right][2] = pos[i][2];
			get_sphere_aabb(&r, radius[i], pos[i][0], pos[i][1], pos[i][2]);
			union_aabb(&raabb, &r);
		}
	}

	if(left > 1)
	{
		pnode->left_ = create_bvh_node();
		pnode->left_->aabb_ = laabb;
		construct_bvh(pnode->left_, oradius, opos, left, offset, depth+1, radius, pos);
	}
	if(right > 1)
	{
		pnode->right_ = create_bvh_node();
		pnode->right_->aabb_ = raabb;
		construct_bvh(pnode->right_, oradius, opos, right, offset + left, depth+1, radius, pos);
	}

	union_aabb(&laabb, &raabb);
	pnode->aabb_ = laabb;

	return 0;
}
