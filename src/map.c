#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <map.h>
#include <SDL3/SDL_opengl.h>

int readBSP(const char* bspfile, Map* map) {
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
		fprintf(stdout, "BSP header is correct, version: %d\n", header.version);
	}

	printf("\nLoading lumps...\n\n");

	map->planes = NULL;
	map->plane_count = 0;
	if (fseek(f, header.lumps[LUMP_PLANES].fileofs, SEEK_SET) == 0) {
		map->plane_count = header.lumps[LUMP_PLANES].filelen / sizeof(dplane_t);
		map->planes = (dplane_t*)calloc(map->plane_count, sizeof(dplane_t));
		fread(map->planes, sizeof(dplane_t), map->plane_count, f);
	} else {
		fprintf(stderr, "Failed to read planes lump: %s:%d\n", __FILE__, __LINE__);
		fclose(f);
		return -1;
	}

	printf("plane_count: %d, size: %llu kB\n", map->plane_count, map->plane_count * sizeof(int) / 1024);

	map->vertices = NULL;
	map->vertex_count = 0;
	if (fseek(f, header.lumps[LUMP_VERTEXES].fileofs, SEEK_SET) == 0) {
		map->vertex_count = header.lumps[LUMP_VERTEXES].filelen / sizeof(Vector);
		map->vertices = (Vector*)calloc(map->vertex_count, sizeof(Vector));
		fread(map->vertices, sizeof(Vector), map->vertex_count, f);
	} else {
		fprintf(stderr, "Failed to read vertices lump: %s:%d\n", __FILE__, __LINE__);
		free(map->planes);
		fclose(f);
		return -1;
	}

	printf("vertex_count: %d, size: %llu kB\n", map->vertex_count, map->vertex_count * sizeof(int) / 1024);
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
		fprintf(stderr, "Failed to read edges lump: %s:%d\n", __FILE__, __LINE__);
		free(map->vertices);
		free(map->planes);
		fclose(f);
		return -1;
	}

	printf("edge_count: %d, size: %llu kB\n", map->edge_count, map->edge_count * sizeof(int) / 1024);

	map->surfedges = NULL;
	map->surfedge_count = 0;
	if (fseek(f, header.lumps[LUMP_SURFEDGES].fileofs, SEEK_SET) == 0) {
		map->surfedge_count = header.lumps[LUMP_SURFEDGES].filelen / sizeof(int);
		map->surfedges = (int*)calloc(map->surfedge_count, sizeof(int));
		fread(map->surfedges, sizeof(int), map->surfedge_count, f);
	} else {
		fprintf(stderr, "Failed to read surfedges lump: %s:%d\n", __FILE__, __LINE__);
		free(map->edges);
		free(map->vertices);
		free(map->planes);
		fclose(f);
		return -1;
	}

	printf("surfedge_count: %d, size: %llu kB\n", map->surfedge_count, map->surfedge_count * sizeof(int) / 1024);

	map->faces = NULL;
	map->face_count = 0;
	if (fseek(f, header.lumps[LUMP_FACES].fileofs, SEEK_SET) == 0) {
		map->face_count = header.lumps[LUMP_FACES].filelen / sizeof(dface_t);
		map->faces = (dface_t*)calloc(map->face_count, sizeof(dface_t));
		fread(map->faces, sizeof(dface_t), map->face_count, f);
	} else {
		fprintf(stderr, "Failed to read faces lump: %s:%d\n", __FILE__, __LINE__);
		free(map->surfedges);
		free(map->edges);
		free(map->vertices);
		free(map->planes);
		fclose(f);
		return -1;
	}

	printf("faces_count: %d, size: %llu kB\n", map->face_count, map->face_count * sizeof(dface_t) / 1024);

	map->texinfos = NULL;
	map->texinfo_count = 0;
	if (fseek(f, header.lumps[LUMP_TEXINFO].fileofs, SEEK_SET) == 0) {
		map->texinfo_count = header.lumps[LUMP_TEXINFO].filelen / sizeof(texinfo_t);
		map->texinfos = (texinfo_t*)calloc(map->texinfo_count, sizeof(texinfo_t));
		fread(map->texinfos, sizeof(texinfo_t), map->texinfo_count, f);
	} else {
		fprintf(stderr, "Failed to read texinfo lump: %s:%d\n", __FILE__, __LINE__);
		free(map->faces);
		free(map->surfedges);
		free(map->edges);
		free(map->vertices);
		free(map->planes);
		fclose(f);
		return -1;
	}

	printf("texinfo_count: %d, size: %llu kB\n", map->texinfo_count, map->texinfo_count * sizeof(texinfo_t) / 1024);

	map->texdatas = NULL;
	map->texdata_count = 0;
	if (fseek(f, header.lumps[LUMP_TEXDATA].fileofs, SEEK_SET) == 0) {
		map->texdata_count = header.lumps[LUMP_TEXDATA].filelen / sizeof(dtexdata_t);
		map->texdatas = (dtexdata_t*)calloc(map->texdata_count, sizeof(dtexdata_t));
		fread(map->texdatas, sizeof(dtexdata_t), map->texdata_count, f);
	} else {
		fprintf(stderr, "Failed to read texdata lump: %s:%d\n", __FILE__, __LINE__);
		free(map->texinfos);
		free(map->faces);
		free(map->surfedges);
		free(map->edges);
		free(map->vertices);
		free(map->planes);
		fclose(f);
		return -1;
	}

	printf("texdata_count: %d, size: %llu kB\n", map->texdata_count, map->texdata_count * sizeof(dtexdata_t) / 1024);

	printf("\nReading textures...\n");

	map->texdata_string_table = NULL;
	map->texdata_string_table_count = 0;
	map->texdata_string_table_count = header.lumps[LUMP_TEXDATA_STRING_TABLE].filelen / sizeof(int);
	map->texdata_string_table = (int*)malloc(header.lumps[LUMP_TEXDATA_STRING_TABLE].filelen);
	if (fseek(f, header.lumps[LUMP_TEXDATA_STRING_TABLE].fileofs, SEEK_SET) == 0) {
		map->texdata_string_table_count = header.lumps[LUMP_TEXDATA_STRING_TABLE].filelen / sizeof(int);
		map->texdata_string_table = (int*)calloc(map->texdata_string_table_count, sizeof(int));
		fread(map->texdata_string_table, sizeof(int), map->texdata_string_table_count, f);
	} else {
		fprintf(stderr, "Failed to read texdata_string_table lump: %s:%d\n", __FILE__, __LINE__);
		free(map->texdatas);
		free(map->texinfos);
		free(map->faces);
		free(map->surfedges);
		free(map->edges);
		free(map->vertices);
		free(map->planes);
		fclose(f);
		return -1;
	}

	printf("texdata_string_table_count: %d, size: %llu kB\n", map->texdata_string_table_count, map->texdata_string_table_count * sizeof(int) / 1024);

	int string_data_size = header.lumps[LUMP_TEXDATA_STRING_DATA].filelen;
	char* all_strings = (char*)malloc(string_data_size);

	fseek(f, header.lumps[LUMP_TEXDATA_STRING_DATA].fileofs, SEEK_SET);
	fread(all_strings, 1, string_data_size, f);

	for (unsigned int i = 0; i < map->texdata_string_table_count; i++) {
		int offset = map->texdata_string_table[i];

		if (offset >= 0 && offset < string_data_size) {
			char* texture_name = &all_strings[offset];
			printf("Texture index %d: %s\n", i, texture_name);
		}
	}

	free(all_strings);

	map->bmodels = NULL;
	map->bmodel_count = 0;
	if (fseek(f, header.lumps[LUMP_MODELS].fileofs, SEEK_SET) == 0) {
		map->bmodel_count = header.lumps[LUMP_MODELS].filelen / sizeof(dmodel_t);
		map->bmodels = (dmodel_t*)calloc(map->bmodel_count, sizeof(dmodel_t));
		fread(map->bmodels, sizeof(dmodel_t), map->bmodel_count, f);
	} else {
		fprintf(stderr, "Failed to read bmodel (model) lump: %s:%d\n", __FILE__, __LINE__);
		free(map->texdata_string_table);
		free(map->texdatas);
		free(map->texinfos);
		free(map->faces);
		free(map->surfedges);
		free(map->edges);
		free(map->vertices);
		free(map->planes);
		fclose(f);
		return -1;
	}

	printf("bmodel_count: %d, size: %llu kB\n", map->bmodel_count, map->bmodel_count * sizeof(dmodel_t) / 1024);

	fclose(f);
	return 0;
}

void clearMap(Map* map) {
	free(map->bmodels);
	free(map->texdata_string_table);
	free(map->texdatas);
	free(map->texinfos);
	free(map->faces);
	free(map->surfedges);
	free(map->edges);
	free(map->vertices);
	free(map->planes);
}