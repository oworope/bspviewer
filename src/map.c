#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <map.h>
#include <SDL3/SDL_opengl.h>

int readBSP(const char* bspfile, map_t* map) {
	FILE* f = fopen(bspfile, "rb");
	if (f == NULL) {
		fprintf(stderr, "Failed to open BSP file: %s\n", bspfile);
		return -1;
	}

	dheader_t header;
	fread(&header, sizeof(dheader_t), 1, f);
	if (header.ident != IDBSPHEADER) {
		fprintf(stderr, "BSP file is invalid: %s\nReason: Header is invalid\n", bspfile);
		fclose(f);
		return -1;
	} else {
		fprintf(stdout, "BSP header is correct. Version: %d\n", header.version);
	}

	map->planes = NULL;
	map->plane_count = 0;
	if (fseek(f, header.lumps[LUMP_PLANES].fileofs, SEEK_SET) == 0) {
		map->plane_count = header.lumps[LUMP_PLANES].filelen / sizeof(dplane_t);
		map->planes = (dplane_t*)calloc(map->plane_count, sizeof(dplane_t));
		fread(map->planes, sizeof(dplane_t), map->plane_count, f);
	} else {
		fprintf(stderr, "Failed to read planes lump: %s:%d", __FILE__, __LINE__);
		fclose(f);
		return -1;
	}

	printf("planes_count: %d\n", map->plane_count);

	map->vertices = NULL;
	map->vertex_count = 0;
	if (fseek(f, header.lumps[LUMP_VERTEXES].fileofs, SEEK_SET) == 0) {
		map->vertex_count = header.lumps[LUMP_VERTEXES].filelen / sizeof(Vector);
		map->vertices = (Vector*)calloc(map->vertex_count, sizeof(Vector));
		fread(map->vertices, sizeof(Vector), map->vertex_count, f);
	} else {
		fprintf(stderr, "Failed to read vertices lump: %s:%d", __FILE__, __LINE__);
		free(map->planes);
		fclose(f);
		return -1;
	}

	printf("vertex_count: %d\n", map->vertex_count);
/*
	for (unsigned int i = 0; i < map->vcount; i++) {
		printf(
			"x: %f, y: %f, z: %f\n",
			map->vertices[i].x,
			map->vertices[i].y,
			map->vertices[i].z
		);
	}
*/

	map->edges = NULL;
	map->surfedges = NULL;
	map->edge_count = 0;
	if (fseek(f, header.lumps[LUMP_EDGES].fileofs, SEEK_SET) == 0) {
		map->edge_count = header.lumps[LUMP_EDGES].filelen / sizeof(dedge_t);
		map->edges = (dedge_t*)calloc(map->edge_count, sizeof(dedge_t));
		fread(map->edges, sizeof(dedge_t), map->edge_count, f);
	} else {
		fprintf(stderr, "Failed to read edges lump: %s:%d", __FILE__, __LINE__);
		free(map->vertices);
		free(map->planes);
		fclose(f);
		return -1;
	}

	printf("edges_count: %d\n", map->edge_count);

	map->surfedges = NULL;
	map->surfedge_count = 0;
	if (fseek(f, header.lumps[LUMP_SURFEDGES].fileofs, SEEK_SET) == 0) {
		map->surfedge_count = header.lumps[LUMP_SURFEDGES].filelen / sizeof(int);
		map->surfedges = (int*)calloc(map->surfedge_count, sizeof(int));
		fread(map->surfedges, sizeof(int), map->surfedge_count, f);
	} else {
		fprintf(stderr, "Failed to read surfedges lump: %s:%d", __FILE__, __LINE__);
		free(map->edges);
		free(map->vertices);
		free(map->planes);
		fclose(f);
		return -1;
	}

	printf("surfedges_count: %d\n", map->surfedge_count);

	map->faces = NULL;
	map->face_count = 0;
	if (fseek(f, header.lumps[LUMP_FACES].fileofs, SEEK_SET) == 0) {
		map->face_count = header.lumps[LUMP_FACES].filelen / sizeof(dface_t);
		map->faces = (dface_t*)calloc(map->face_count, sizeof(dface_t));
		fread(map->faces, sizeof(dface_t), map->face_count, f);
	} else {
		fprintf(stderr, "Failed to read faces lump: %s:%d", __FILE__, __LINE__);
		free(map->surfedges);
		free(map->edges);
		free(map->vertices);
		free(map->planes);
		fclose(f);
		return -1;
	}

	printf("faces_count: %d\n", map->face_count);

	fclose(f);
	return 0;
}

void clearMap(map_t* map) {
	free(map->faces);
	free(map->surfedges);
	free(map->edges);
	free(map->vertices);
	free(map->planes);
}