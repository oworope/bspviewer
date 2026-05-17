#define _CRT_SECURE_NO_WARNINGS
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

	int model_count = 0;
	Model** models = create_brush_models(renderer, &map, &model_count);

	bool running = true;
	const bool* keystate = NULL;
	while (running) {
		SDL_Event e;
		float dx = 0.0f;
		float dy = 0.0f;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_EVENT_QUIT) {
				running = false;
			}

			if (e.type == SDL_EVENT_WINDOW_RESIZED) {
				SDL_GetWindowSize(window, &window_width, &window_height);
				renderer_viewport(renderer, 0, 0, window_width, window_height);
			}

			if (e.type == SDL_EVENT_MOUSE_MOTION) {
				dx += e.motion.xrel;
				dy += e.motion.yrel;
			}
		}

		keystate = SDL_GetKeyboardState(NULL);
		camera_update(&camera, keystate, 0.016f); // TODO: dt
		camera_rotate(&camera, dx, dy, 0.0);

		mat4 view, proj;
		camera_get_view_matrix(&camera, view);
		camera_get_projection_matrix((float)window_width / (float)window_height, proj);

		renderer_begin_frame(renderer);
		for (int i = 0; i < model_count; i++) {
			model_render(models[i], view, proj);
		}
		renderer_end_frame(renderer);
		SDL_Delay(16);
	}

	if (models) {
		for (int i = 0; i < model_count; i++) {
			destroy_model(models[i]);
		}
		free(models);
	}

	clearMap(&map);
	destroy_renderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
