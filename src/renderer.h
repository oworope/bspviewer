#pragma once
#include <cglm/call.h>
#include <SDL3/SDL_video.h>
#include <map.h>

typedef struct Renderer Renderer;
Renderer* create_renderer(SDL_Window* window);
void destroy_renderer(Renderer* r);
void renderer_viewport(Renderer* r, int x, int y, int w, int h);
void renderer_begin_frame(Renderer* r);
void renderer_end_frame(Renderer* r);

typedef struct shader_t Shader;
Shader* create_shader(const char *vert_file, const char *frag_file);
void destroy_shader(Shader* s);

typedef struct model_t Model;
Model* create_map_model(Renderer* r, Map map);
Model** create_brush_models(Renderer* r, const Map* map, int* count);
void destroy_model(Model* m);
void model_render(Model* m, mat4 view, mat4 proj);