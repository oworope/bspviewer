#include <camera.h>

void camera_get_view_matrix(const Camera* cam, mat4 dest) {
    mat4 rot_mat, trans_mat, inv_rot;

    glmc_quat_mat4((float*)cam->rot, rot_mat);
    glmc_mat4_inv(rot_mat, inv_rot);

    glmc_mat4_identity(trans_mat);
    glmc_translate(trans_mat, (vec3){ -cam->pos[0], -cam->pos[1], -cam->pos[2] });

    glmc_mat4_mul(inv_rot, trans_mat, dest);
}

void camera_get_projection_matrix(float aspect, mat4 dest) {
    glmc_perspective(glm_rad(90.0f), aspect, 10.0f, 16384.0f, dest);
}

void camera_rotate(Camera* cam, float xrel, float yrel, float dt) {
	const float sensitivity = 0.002f;
	if (xrel != 0.0f || yrel != 0.0f) {
		versor qx, qy;
		glmc_quat(qx, -xrel * sensitivity, 0, 1, 0);
		glmc_quat_mul(qx, cam->rot, cam->rot);
		glmc_quat(qy, -yrel * sensitivity, 1, 0, 0);
		glmc_quat_mul(cam->rot, qy, cam->rot);
		glmc_quat_normalize(cam->rot);
	}
}

#include <SDL3/SDL_keyboard.h>
void camera_update(Camera* cam, const bool* keystate, float dt) {
	glmc_vec3_zero(cam->speed);
	cam->move_speed = 10.0f;
	if (keystate[SDL_SCANCODE_LSHIFT]) cam->move_speed *= 5.0f;
	if (keystate[SDL_SCANCODE_W]) cam->speed[2] -= cam->move_speed;
	if (keystate[SDL_SCANCODE_S]) cam->speed[2] += cam->move_speed;
	if (keystate[SDL_SCANCODE_A]) cam->speed[0] -= cam->move_speed;
	if (keystate[SDL_SCANCODE_D]) cam->speed[0] += cam->move_speed;

	vec3 world_move;
	glmc_quat_rotatev(cam->rot, cam->speed, world_move);
	glmc_vec3_add(cam->pos, world_move, cam->pos);
}