#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <memory.h>
#include "utils/vec.h"

struct camera
{
    camera():rot_x(0), rot_y(0), dist(0)
	{
		memset(pos, 0, sizeof(pos));
		pos[2] = -5;

		memset(lookat, 0, sizeof(lookat));
		memset(right, 0, sizeof(right));
		memset(up, 0, sizeof(up));
		lookat[2] = right[0] = up[1] = 1.0f;
		dx = dy = dz = 0;
		move_scale = .1f;

		proj_ = mat4::identity();
		inv_proj_ = mat4::identity();
		view_ = mat4::identity();
		inv_view_ = mat4::identity();
		world_ = mat4::identity();
		view_proj_inv_ = mat4::identity();
	}

	void set_projection(const mat4& proj);
	void update(float dt);
	void get_pos(float (*p)[4] ) const; 
	void get_view_proj_inv(mat4* vpi) const { *vpi = view_proj_inv_; }
	void get_view(mat4* view) const { *view = view_; }
	void set_view(const mat4& view_mat);

	static void compose_view_matrix(mat4* view, const vec3& right, vec3& up, vec3& front, vec3 world_pos);
	static void compose_view_matrix(mat4* view, const float (& mat)[3*4]);
	static void view_get_world_pos(const mat4& view, vec3* world_pos);

    float rot_x;
    float rot_y;
    float dist;
	float dx;
	float dy;
	float dz;
	float move_scale;

	float pos[4];
	float lookat[4];
	float right[4];
	float up[4];

	mat4 proj_;
	mat4 inv_proj_;
	mat4 view_;
	mat4 inv_view_;
	mat4 world_;
	mat4 view_proj_inv_;

	
	
};


#endif // __CAMERA_H__
