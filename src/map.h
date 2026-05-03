#pragma once
#include <bsp.h>

typedef struct map_t {
	dplane_t* planes;
	Vector* vertices;
	dedge_t* edges;
	int* surfedges;
	dface_t* faces;
	unsigned int plane_count;
	unsigned int vertex_count;
	unsigned int edge_count;
	unsigned int surfedge_count;
	unsigned int face_count;
} map_t;

typedef struct Vertex {
	float x, y, z;
	unsigned char r, g, b;
} Vertex;

void drawFaces(map_t* map);
int readBSP(const char* bspfile, map_t* map);
void clearMap(map_t* map);