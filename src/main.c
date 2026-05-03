#define _CRT_SECURE_NO_WARNINGS
#include <cglm/call.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_opengles2.h>
#include <map.h>
#include <bsp.h>
#include <vpk.h>

// CAMERA STUFF
typedef float quat[4];

typedef struct camera_t {
	vec3 pos;
	quat rot;
	vec3 speed;
} camera_t;

static GLuint compile_shader(GLenum type, const char *source) {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char log[512];
		glGetShaderInfoLog(shader, sizeof(log), NULL, log);
		fprintf(stderr, "Shader compile error: %s", log);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

static GLuint create_program(const char *vert_file, const char *frag_file) {
	FILE* fv = fopen(vert_file, "rb");
	FILE* ff = fopen(frag_file, "rb");
	if (fv == NULL || ff == NULL) {
		fprintf(stderr, "Failed to open shaders!\n");
		return -1;
	}

	// determine file size
	fseek(fv, 0, SEEK_END);
	long fsize = ftell(fv);
	fseek(fv, 0, SEEK_SET); // rewind(f);
	char* vstring = malloc(fsize + 1);
	if (!vstring) {
		fclose(fv);
		return NULL;
	}
	fread(vstring, fsize, 1, fv);
	fclose(fv);
	vstring[fsize] = 0;

	fseek(ff, 0, SEEK_END);
	fsize = ftell(ff);
	fseek(ff, 0, SEEK_SET);
	char* fstring = malloc(fsize + 1);
	if (!fstring) {
		fclose(fv);
		return NULL;
	}
	fread(fstring, fsize, 1, ff);
	fclose(ff);
	fstring[fsize] = 0;

	GLuint vs = compile_shader(GL_VERTEX_SHADER, vstring);
	GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fstring);
	GLuint prog = glCreateProgram();
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);
	glLinkProgram(prog);
	glDeleteShader(vs);
	glDeleteShader(fs);
	free(fstring);
	free(vstring);
	return prog;
}

void updateCamera(camera_t* cam) {
	mat4 view;

	glmc_mat4_identity(view);

	versor inv_rot;
	glmc_quat_inv(cam->rot, inv_rot);
	glmc_quat_mat4(inv_rot, view);

	vec3 inv_pos;
	glmc_vec3_negate_to(cam->pos, inv_pos);
	glmc_translate(view, inv_pos);

	// glMatrixMode(GL_MODELVIEW);
	// glLoadMatrixf((float *)view);
}

void setupProjection(float width, float height) {
	mat4 proj;
	float aspect = width / height;

	glmc_perspective(glm_rad(90.0f), aspect, 4.0f, 16384.0f, proj);

	// glMatrixMode(GL_PROJECTION);
	// glLoadMatrixf((float *)proj);
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

	readVPK("garrysmod_dir.vpk");

	SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
	SDL_SetAppMetadata("bspviewer", "1.0", "com.example.bspviewer");

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
		return -1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
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

	printf("GL_VERSION: %s\n", glGetString(GL_VERSION));

	camera_t camera = {0};
	map_t map = {0};

	if (readBSP(map_name, &map) == 0) {
	} else {
		SDL_GL_DestroyContext(glcontext);
		SDL_DestroyWindow(window);
		SDL_Quit();
	}

	unsigned int total_triangles = 0;
	for (unsigned int i = 0; i < map.face_count; i++) {
		if (map.faces[i].numedges >= 3) total_triangles += (map.faces[i].numedges - 2);
	}

	float* vertices = (float*)malloc(total_triangles * 3 * 6 * sizeof(float));

	unsigned int vert_index = 0;

	for (unsigned int i = 0; i < map.face_count; i++) {
		dface_t* f = &map.faces[i];
		if (f->numedges < 3) continue;

		// build face vertex indices
		unsigned short* face_verts = (unsigned short*)malloc(f->numedges * sizeof(unsigned short));
		for (int j = 0; j < f->numedges; j++) {
			int e_idx = map.surfedges[f->firstedge + j];
			face_verts[j] = (e_idx >= 0) ? map.edges[e_idx].v[0] : map.edges[-e_idx].v[1];
		}

		// base color
		dplane_t* p = &map.planes[f->planenum];
		float brightness = 0.3f + fabsf(p->normal.z)*0.4f + fabsf(p->normal.x)*0.2f;
		float r = brightness * 0.6f;
		float g = brightness * 0.2f;
		float b = brightness * 0.1f;

		// Triangulate fan (vertex 0 as fan center)
		for (int j = 1; j < f->numedges - 1; j++) {
			unsigned short v0_idx = face_verts[0];
			unsigned short v1_idx = face_verts[j];
			unsigned short v2_idx = face_verts[j+1];

			// Swap v1 and v2 for CCW (as you already do)
			unsigned short tmp = v1_idx;
			v1_idx = v2_idx;
			v2_idx = tmp;

			// Helper macro to store a vertex
			#define STORE_VERTEX(idx) do { \
				Vector* pos = &map.vertices[idx]; \
				vertices[vert_index++] = pos->x; \
				vertices[vert_index++] = pos->z; \
				vertices[vert_index++] = -pos->y; \
				vertices[vert_index++] = r; \
				vertices[vert_index++] = g; \
				vertices[vert_index++] = b; \
			} while(0)

			STORE_VERTEX(v0_idx);
			STORE_VERTEX(v1_idx);
			STORE_VERTEX(v2_idx);
		}

		free(face_verts);
	}

	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vert_index * sizeof(float), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	free(vertices);

	unsigned int total_vertices = vert_index / 6; // number of vertices to draw

	int program = create_program("vertex_gles2.glsl", "fragment_gles2.glsl");
	GLint mvp_loc = glGetUniformLocation(program, "uMVP");
	GLint aPos_loc = glGetAttribLocation(program, "aPos");
	GLint aColor_loc = glGetAttribLocation(program, "aColor");

	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glEnableVertexAttribArray(aPos_loc);
	glVertexAttribPointer(aPos_loc, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

	glEnableVertexAttribArray(aColor_loc);
	glVertexAttribPointer(aColor_loc, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// setupProjection((float)window_width, (float)window_height);

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
				// setupProjection(window_width, window_height);
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

		mat4 view, proj, mvp;

		mat4 rot_mat;
		glmc_quat_mat4(camera.rot, rot_mat);
		glmc_mat4_inv(rot_mat, rot_mat);
		mat4 trans_mat;
		glmc_mat4_identity(trans_mat);
		glmc_translate(trans_mat, (vec3){-camera.pos[0], -camera.pos[1], -camera.pos[2]});
		glmc_mat4_mul(rot_mat, trans_mat, view); // view = rot_inv * translate

		float aspect = (float)window_width / (float)window_height;
		glmc_perspective(glm_rad(90.0f), aspect, 4.0f, 16384.0f, proj);

		// MVP = proj * view
		glmc_mat4_mul(proj, view, mvp);

		glUseProgram(program);
		glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, (float*)mvp);

		glDrawArrays(GL_TRIANGLES, 0, total_vertices);

		SDL_GL_SwapWindow(window);
		SDL_Delay(16);
	}

	SDL_GL_DestroyContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	clearMap(&map);
	return 0;
}
