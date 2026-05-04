#pragma once
#include <cglm/call.h>

typedef float quat[4];

typedef struct camera_t {
	vec3 pos;
	quat rot;
	vec3 speed;
	float move_speed;
} Camera;

// Fill a 4x4 view matrix from the camera
void camera_get_view_matrix(const Camera* cam, mat4 dest);

// Fill a 4x4 perspective projection matrix (aspect = width/height)
void camera_get_projection_matrix(float aspect, mat4 dest);