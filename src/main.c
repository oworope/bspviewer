#define _CRT_SECURE_NO_WARNIGS
#include <cglm/call.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <map.h>
#include <bsp.h>
#include <vpk.h>
#include <renderer.h>
#include <camera.h>

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

	// readVPK("garrysmod_dir.vpk");

	SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
	SDL_SetAppMetadata("bspviewer", "1.0", "com.example.bspviewer");

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
		return -1;
	}

	char window_title[128];
	sprintf(window_title, "bspviewer: %s", map_name);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_Window* window = SDL_CreateWindow(window_title, window_width, window_height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!window) {
		SDL_Log("Failed to open a window: %s", SDL_GetError());
		return -1;
	}

	Camera camera = {0};
	Map map = {0};

	Renderer* renderer = create_renderer(window);
	if (renderer == NULL) {
		SDL_DestroyWindow(window);
		return -1;
	}

	if (readBSP(map_name, &map) == 0) {
	} else {
		destroy_renderer(renderer);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	Model* map_model = create_map_model(renderer, map);

	bool running = true;
	while (running) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_EVENT_QUIT) {
				running = false;
			}

			if (e.type == SDL_EVENT_WINDOW_RESIZED) {
				SDL_GetWindowSize(window, &window_width, &window_height);
				renderer_viewport(renderer, 0, 0, window_width, window_height);
			}

			const bool *state = SDL_GetKeyboardState(NULL);

			glmc_vec3_zero(camera.speed);
			camera.move_speed = 10.0f;

			if (state[SDL_SCANCODE_LSHIFT]) camera.move_speed *= 5.0f;
			if (state[SDL_SCANCODE_W]) camera.speed[2] -= camera.move_speed;
			if (state[SDL_SCANCODE_S]) camera.speed[2] += camera.move_speed;
			if (state[SDL_SCANCODE_A]) camera.speed[0] -= camera.move_speed;
			if (state[SDL_SCANCODE_D]) camera.speed[0] += camera.move_speed;

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

		vec3 worldMove;
		glmc_quat_rotatev(camera.rot, camera.speed, worldMove);
		glmc_vec3_add(camera.pos, worldMove, camera.pos);

		mat4 view, proj;
		camera_get_view_matrix(&camera, view);
		camera_get_projection_matrix((float)window_width / (float)window_height, proj);

		renderer_begin_frame(renderer);
		model_render(map_model, view, proj);
		renderer_end_frame(renderer);
		SDL_Delay(16);
	}

	destroy_model(map_model);
	clearMap(&map);
	destroy_renderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
