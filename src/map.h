#pragma once
#include <bsp.h>

typedef struct map_t {
	dplane_t* planes;
	Vector* vertices;
	dedge_t* edges;
	int* surfedges;
	dface_t* faces;
	texinfo_t* texinfos;
	dtexdata_t* texdatas;
	int* texdata_string_table;
	unsigned int plane_count;
	unsigned int vertex_count;
	unsigned int edge_count;
	unsigned int surfedge_count;
	unsigned int face_count;
	unsigned int texinfo_count;
	unsigned int texdata_count;
	unsigned int texdata_string_table_count;
} Map;

typedef struct Vertex {
	float x, y, z;
	unsigned char r, g, b;
} Vertex;

void drawFaces(Map* map);
int readBSP(const char* bspfile, Map* map);
void clearMap(Map* map);