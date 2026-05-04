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