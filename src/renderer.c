#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <SDL3/SDL_opengles2.h>
#include <SDL3/SDL_mouse.h>
#include <renderer.h>

struct shader_t {
	GLuint prog;
	GLint mvp_loc, aPos_loc, aColor_loc;
};

struct Renderer {
	SDL_Window* window;
	SDL_GLContext glcontext;
};

Renderer* create_renderer(SDL_Window* window) {
	SDL_GLContext glcontext = SDL_GL_CreateContext(window);
	if (!glcontext) {
		fprintf(stderr, "Failed to create GL context: %s", SDL_GetError());
		return NULL;
	}

	SDL_GL_MakeCurrent(window, glcontext);
	SDL_SetWindowRelativeMouseMode(window, true);

	printf("GL_VERSION: %s\n", glGetString(GL_VERSION));

	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	Renderer* r = (Renderer*)malloc(sizeof(Renderer));
	r->window = window;
	r->glcontext = glcontext;
	return r;
}

void destroy_renderer(Renderer* r) {
	if (!r) return;
	SDL_GL_DestroyContext(r->glcontext);
	free(r);
}

void renderer_viewport(Renderer* r, int x, int y, int w, int h) {
	if (!r) return;
	glViewport(x, y, w, h);
}

void renderer_begin_frame(Renderer* r) {
	if (!r) return;
	glClearColor(0.1f, 0.0f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void renderer_end_frame(Renderer* r) {
	if (!r) return;
	SDL_GL_SwapWindow(r->window);
}

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

static Shader* create_shader(const char *vert_file, const char *frag_file) {
	Shader* shader = NULL;

	FILE* fv = fopen(vert_file, "rb");
	FILE* ff = fopen(frag_file, "rb");
	if (fv == NULL || ff == NULL) {
		fprintf(stderr, "Failed to open shaders!\n");
		return shader;
	}

	// determine file size
	fseek(fv, 0, SEEK_END);
	long fsize = ftell(fv);
	fseek(fv, 0, SEEK_SET); // rewind(f);
	char* vstring = malloc(fsize + 1);
	if (!vstring) {
		fclose(fv);
		return shader;
	}
	fread(vstring, fsize, 1, fv);
	vstring[fsize] = 0;

	fseek(ff, 0, SEEK_END);
	fsize = ftell(ff);
	fseek(ff, 0, SEEK_SET);
	char* fstring = malloc(fsize + 1);
	if (!fstring) {
		fclose(fv);
		return shader;
	}
	fread(fstring, fsize, 1, ff);
	fstring[fsize] = 0;

	fclose(ff);
	fclose(fv);

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
	shader = (Shader*)malloc(sizeof(Shader));
	shader->mvp_loc = glGetUniformLocation(prog, "uMVP");
	shader->aPos_loc = glGetAttribLocation(prog, "aPos");
	shader->aColor_loc = glGetAttribLocation(prog, "aColor");
	shader->prog = prog;
	return shader;
}

void destroy_shader(Shader* shader) {
	if (!shader) return;
	glDeleteProgram(shader->prog);
	free(shader);
}

struct model_t {
	Shader* shader;
	GLuint vbo;
	unsigned int vertex_count;
	mat4 transform;
};

Model* create_map_model(Renderer* r, Map map) {
	if (!r) return NULL;
	Model* m = (Model*)malloc(sizeof(Model));
	m->shader = NULL;
	m->vbo = 0;
	m->vertex_count = 0;
	glmc_mat4_identity(m->transform);

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
		float brightness = 0.5f + fabsf(p->normal.z)*0.4f + fabsf(p->normal.x)*0.2f;
		float r = brightness * 0.8f;
		float g = brightness * 0.8f;
		float b = brightness * 0.8f;

		// Triangulate fan (vertex 0 as fan center)
		for (int j = 1; j < f->numedges - 1; j++) {
			unsigned short v0_idx = face_verts[0];
			unsigned short v1_idx = face_verts[j];
			unsigned short v2_idx = face_verts[j+1];

			// Swap v1 and v2 for CCW
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

	glGenBuffers(1, &m->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
	glBufferData(GL_ARRAY_BUFFER, vert_index * sizeof(float), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	free(vertices);

	m->vertex_count = vert_index / 6; // number of vertices to draw

	m->shader = create_shader("vertex_gles2.glsl", "fragment_gles2.glsl");
	if (m->shader == NULL) {
		glDeleteBuffers(1, &m->vbo);
		free(m);
	}

	glBindBuffer(GL_ARRAY_BUFFER, m->vbo);

	glEnableVertexAttribArray(m->shader->aPos_loc);
	glVertexAttribPointer(m->shader->aPos_loc, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

	glEnableVertexAttribArray(m->shader->aColor_loc);
	glVertexAttribPointer(m->shader->aColor_loc, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

	return m;
}

Model** create_brush_models(Renderer* r, const Map* map, int* count) {
    if (!r || !map || map->bmodel_count == 0) {
        *count = 0;
        return NULL;
    }

    // Mark faces used by non‑world brush entities
    bool* face_used = (bool*)calloc(map->face_count, sizeof(bool));
    for (unsigned int i = 1; i < map->bmodel_count; i++) {
        dmodel_t* bm = &map->bmodels[i];
        if (bm->numfaces <= 0) continue;
        for (int j = 0; j < bm->numfaces; j++) {
            int face_idx = bm->firstface + j;
            if (face_idx >= 0 && face_idx < (int)map->face_count)
                face_used[face_idx] = true;
        }
    }

    // Count how many models we actually need (world + brush entities with faces)
    int model_count = 0;
    for (unsigned int i = 0; i < map->bmodel_count; i++) {
        if (i == 0) {
            // world model – only count if there are unused faces
            bool has_faces = false;
            for (unsigned int f = 0; f < map->face_count; f++) {
                if (!face_used[f]) { has_faces = true; break; }
            }
            if (has_faces) model_count++;
        } else {
            if (map->bmodels[i].numfaces > 0) model_count++;
        }
    }

    Model** models = (Model**)malloc(model_count * sizeof(Model*));
    int out_idx = 0;

    if (model_count > 0) {
        bool world_has_faces = false;
        for (unsigned int f = 0; f < map->face_count; f++)
            if (!face_used[f]) { world_has_faces = true; break; }

        if (world_has_faces) {
            Model* m = (Model*)malloc(sizeof(Model));
            m->shader = NULL;
            m->vbo = 0;
            m->vertex_count = 0;
            glmc_mat4_identity(m->transform);

            unsigned int total_triangles = 0;
            for (unsigned int f = 0; f < map->face_count; f++) {
                if (!face_used[f] && map->faces[f].numedges >= 3)
                    total_triangles += (map->faces[f].numedges - 2);
            }

            float* vertices = (float*)malloc(total_triangles * 3 * 6 * sizeof(float));
            unsigned int vert_index = 0;

            for (unsigned int f = 0; f < map->face_count; f++) {
                if (face_used[f]) continue;
                dface_t* face = &map->faces[f];
                if (face->numedges < 3) continue;

                unsigned short* face_verts = (unsigned short*)malloc(face->numedges * sizeof(unsigned short));
                for (int j = 0; j < face->numedges; j++) {
                    int e_idx = map->surfedges[face->firstedge + j];
                    face_verts[j] = (e_idx >= 0) ? map->edges[e_idx].v[0] : map->edges[-e_idx].v[1];
                }

                dplane_t* p = &map->planes[face->planenum];
                float brightness = 0.5f + fabsf(p->normal.z)*0.4f + fabsf(p->normal.x)*0.2f;
                float cr = brightness * 0.8f;
                float cg = brightness * 0.8f;
                float cb = brightness * 0.8f;

                for (int j = 1; j < face->numedges - 1; j++) {
                    unsigned short v0_idx = face_verts[0];
                    unsigned short v1_idx = face_verts[j];
                    unsigned short v2_idx = face_verts[j+1];
                    unsigned short tmp = v1_idx;
                    v1_idx = v2_idx;
                    v2_idx = tmp;

                    #define STORE_WORLD_VERTEX(idx) do { \
                        Vector* pos = &map->vertices[idx]; \
                        vertices[vert_index++] = pos->x; \
                        vertices[vert_index++] = pos->z; \
                        vertices[vert_index++] = -pos->y; \
                        vertices[vert_index++] = cr; \
                        vertices[vert_index++] = cg; \
                        vertices[vert_index++] = cb; \
                    } while(0)

                    STORE_WORLD_VERTEX(v0_idx);
                    STORE_WORLD_VERTEX(v1_idx);
                    STORE_WORLD_VERTEX(v2_idx);
                }
                free(face_verts);
            }

            glGenBuffers(1, &m->vbo);
            glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
            glBufferData(GL_ARRAY_BUFFER, vert_index * sizeof(float), vertices, GL_STATIC_DRAW);
            free(vertices);

            m->vertex_count = vert_index / 6;
            m->shader = create_shader("vertex_gles2.glsl", "fragment_gles2.glsl");
            if (!m->shader) {
                glDeleteBuffers(1, &m->vbo);
                free(m);
            } else {
                models[out_idx++] = m;
            }
        }
    }

    for (unsigned int i = 1; i < map->bmodel_count; i++) {
        dmodel_t* bm = &map->bmodels[i];
        if (bm->numfaces <= 0) continue;

        Model* m = (Model*)malloc(sizeof(Model));
        m->shader = NULL;
        m->vbo = 0;
        m->vertex_count = 0;
        glmc_mat4_identity(m->transform);
        glmc_translate(m->transform, (vec3){bm->origin.x, bm->origin.z, -bm->origin.y});

        unsigned int total_triangles = 0;
        for (int j = 0; j < bm->numfaces; j++) {
            int f = bm->firstface + j;
            if (f >= 0 && f < (int)map->face_count && map->faces[f].numedges >= 3)
                total_triangles += (map->faces[f].numedges - 2);
        }

        float* vertices = (float*)malloc(total_triangles * 3 * 6 * sizeof(float));
        unsigned int vert_index = 0;

        for (int j = 0; j < bm->numfaces; j++) {
            int face_idx = bm->firstface + j;
            if (face_idx < 0 || face_idx >= (int)map->face_count) continue;
            dface_t* face = &map->faces[face_idx];
            if (face->numedges < 3) continue;

            unsigned short* face_verts = (unsigned short*)malloc(face->numedges * sizeof(unsigned short));
            for (int k = 0; k < face->numedges; k++) {
                int e_idx = map->surfedges[face->firstedge + k];
                face_verts[k] = (e_idx >= 0) ? map->edges[e_idx].v[0] : map->edges[-e_idx].v[1];
            }

            dplane_t* p = &map->planes[face->planenum];
            float brightness = 0.5f + fabsf(p->normal.z)*0.4f + fabsf(p->normal.x)*0.2f;
            float cr = brightness * 0.8f;
            float cg = brightness * 0.8f;
            float cb = brightness * 0.8f;

            for (int k = 1; k < face->numedges - 1; k++) {
                unsigned short v0_idx = face_verts[0];
                unsigned short v1_idx = face_verts[k];
                unsigned short v2_idx = face_verts[k+1];
                unsigned short tmp = v1_idx;
                v1_idx = v2_idx;
                v2_idx = tmp;

                Vector* p0 = &map->vertices[v0_idx];
                Vector* p1 = &map->vertices[v1_idx];
                Vector* p2 = &map->vertices[v2_idx];

                #define STORE_LOCAL_VERTEX(ptr) do { \
                    vertices[vert_index++] = ptr->x - bm->origin.x; \
                    vertices[vert_index++] = ptr->z - bm->origin.z; \
                    vertices[vert_index++] = -(ptr->y - bm->origin.y); \
                    vertices[vert_index++] = cr; \
                    vertices[vert_index++] = cg; \
                    vertices[vert_index++] = cb; \
                } while(0)

                STORE_LOCAL_VERTEX(p0);
                STORE_LOCAL_VERTEX(p1);
                STORE_LOCAL_VERTEX(p2);
            }
            free(face_verts);
        }

        glGenBuffers(1, &m->vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
        glBufferData(GL_ARRAY_BUFFER, vert_index * sizeof(float), vertices, GL_STATIC_DRAW);
        free(vertices);

        m->vertex_count = vert_index / 6;
        m->shader = create_shader("vertex_gles2.glsl", "fragment_gles2.glsl");
        if (!m->shader) {
            glDeleteBuffers(1, &m->vbo);
            free(m);
        } else {
            models[out_idx++] = m;
        }
    }

    free(face_used);
    *count = out_idx;
    return models;
}

void destroy_model(Model* m) {
    if (!m) return;
    if (m->vbo) glDeleteBuffers(1, &m->vbo);
    if (m->shader) destroy_shader(m->shader);
    free(m);
}

void model_render(Model* m, mat4 view, mat4 proj) {
	if (!m || !m->shader) return;

	mat4 mvp;
	mat4 temp;
	glmc_mat4_mul(view, m->transform, temp);
	glmc_mat4_mul(proj, temp, mvp);

	glUseProgram(m->shader->prog);
	glUniformMatrix4fv(m->shader->mvp_loc, 1, GL_FALSE, (float*)mvp);

	glBindBuffer(GL_ARRAY_BUFFER, m->vbo);
	glVertexAttribPointer(m->shader->aPos_loc, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(m->shader->aPos_loc);
	glVertexAttribPointer(m->shader->aColor_loc, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(m->shader->aColor_loc);

	glDrawArrays(GL_TRIANGLES, 0, m->vertex_count);
}