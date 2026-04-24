#define _CRT_SECURE_NO_WARNINGS
#include <cglm/call.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_opengl.h>
#include <map.h>
#include <bsp.h>

// CAMERA STUFF
typedef float quat[4];

typedef struct camera_t {
	vec3 pos;
	quat rot;
	vec3 speed;
} camera_t;

void updateCamera(camera_t* cam) {
	mat4 view;

	glmc_mat4_identity(view);

	versor inv_rot;
	glmc_quat_inv(cam->rot, inv_rot);
	glmc_quat_mat4(inv_rot, view);

	vec3 inv_pos;
	glmc_vec3_negate_to(cam->pos, inv_pos);
	glmc_translate(view, inv_pos);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf((float *)view);
}

void setupProjection(float width, float height) {
	mat4 proj;
	float aspect = width / height;

	glmc_perspective(glm_rad(90.0f), aspect, 4.0f, 16384.0f, proj);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf((float *)proj);
}

int main(int argc, char* argv[]) {
	int window_width = 1920;
	int window_height = 1080;
	const char* map_name = NULL;
	const char* map_arg = "-map";
	size_t map_arg_len = strlen(map_arg);

	if (argc <= 1) {
		map_name = NULL;
	} else {
		for (int i = 0; i < argc; i++) {
			if (strlen(argv[i]) == map_arg_len && memcmp(argv[i], map_arg, map_arg_len) == 0) {
				if (argc <= i+1) break;
				i++;
				map_name = argv[i];
			}
		}
	}

	if (map_name == NULL) {
		map_name = "d1_trainstation_02.bsp";
		printf("No arguments provided, opening default map: %s\n", map_name);
	} else {
		printf("Opening map: %s\n", map_name);
	}

	SDL_Init(SDL_INIT_VIDEO);
	SDL_SetAppMetadata("bspviewer", "1.0", "com.example.bspviewer");

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
		return -1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	SDL_Window* window = SDL_CreateWindow("bspviewer", window_width, window_height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!window) {
		SDL_Log("Failed to open a window: %s", SDL_GetError());
		return -1;
	}

	SDL_GLContext glcontext = SDL_GL_CreateContext(window);
	if (!glcontext) {
		SDL_Log("Failed to create GL context: %s", SDL_GetError());
		SDL_DestroyWindow(window);
		return -1;
	}

	SDL_GL_MakeCurrent(window, glcontext);
	SDL_SetWindowRelativeMouseMode(window, true);

	camera_t camera = {0};
	map_t map = {0};

	if (readBSP(map_name, &map) == 0) {
	} else {
		SDL_GL_DestroyContext(glcontext);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPointSize(5.0f);
	glLineWidth(3.0f);
	setupProjection((float)window_width, (float)window_height);

	bool running = true;
	while (running) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_EVENT_QUIT) {
				running = false;
			}

			if (e.type == SDL_EVENT_WINDOW_RESIZED) {
				SDL_GetWindowSize(window, &window_width, &window_height);
				glViewport(0, 0, window_width, window_height);
				setupProjection(window_width, window_height);
			}

			const bool *state = SDL_GetKeyboardState(NULL);

			glmc_vec3_zero(camera.speed);
			float moveSpeed = 10.0f;

			if (state[SDL_SCANCODE_LSHIFT]) moveSpeed *= 5.0f;
			if (state[SDL_SCANCODE_W]) camera.speed[2] -= moveSpeed;
			if (state[SDL_SCANCODE_S]) camera.speed[2] += moveSpeed;
			if (state[SDL_SCANCODE_A]) camera.speed[0] -= moveSpeed;
			if (state[SDL_SCANCODE_D]) camera.speed[0] += moveSpeed;

			if (e.type == SDL_EVENT_MOUSE_MOTION) {
				float sensitivity = 0.002f;
    			float dx = e.motion.xrel * sensitivity;
    			float dy = e.motion.yrel * sensitivity;

    			versor qx;
    			glmc_quat(qx, -dx, 0, 1, 0);
    			glmc_quat_mul(qx, camera.rot, camera.rot);

    			versor qy;
    			glmc_quat(qy, -dy, 1, 0, 0);
    			glmc_quat_mul(camera.rot, qy, camera.rot);

       			glmc_quat_normalize(camera.rot);
			}
		}

		glClearColor(0.1f, 0.0f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		vec3 worldMove;
		glmc_quat_rotatev(camera.rot, camera.speed, worldMove);
		glmc_vec3_add(camera.pos, worldMove, camera.pos);
		updateCamera(&camera);

		// glBegin(GL_POINTS);
		// glColor3f(0.0f, 1.0f, 0.0f);
		// for (unsigned int i = 0; i < map.vertex_count; i++) {
		// 	glVertex3f(map.vertices[i].x, map.vertices[i].z, -map.vertices[i].y);
		// }
		// glEnd();

		drawFaces(&map);

		SDL_GL_SwapWindow(window);
		SDL_Delay(16);
	}

	SDL_GL_DestroyContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	clearMap(&map);
	return 0;
}
