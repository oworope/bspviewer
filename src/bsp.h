#pragma once
// https://developer.valvesoftware.com/wiki/BSP_(Source)

typedef struct Vector { float x, y, z; } Vector;

#define MAX_MAP_VERTS 65536
#define MAX_MAP_EDGES 256000
#define MAX_MAP_SURFEDGES 256000
#define MAX_MAP_FACES 65536
#define MAX_MAP_TEXINFO 12288
#define MAX_MAP_TEXDATA 2048
#define MAX_MAP_TEXDATA_STRING_DATA 256000
#define TEXTURE_NAME_LENGTH 128
#define MAX_MAP_MODELS 1024

typedef struct dplane_t {
	Vector  normal;   // normal vector
	float   dist;     // distance from origin
	int     type;     // plane axis identifier
} dplane_t;

#define IDBSPHEADER	(('P'<<24)+('S'<<16)+('B'<<8)+'V')
#define HEADER_LUMPS 64

typedef enum LumpType {
	LUMP_ENTITES = 0,
	LUMP_PLANES = 1,
	LUMP_TEXDATA = 2,
	LUMP_VERTEXES = 3,
	LUMP_VISIBILITY = 4,
	LUMP_NODES = 5,
	LUMP_TEXINFO = 6,
	LUMP_FACES = 7,
	LUMP_LIGHTING = 8,
	LUMP_OCCLUSION = 9,
	LUMP_LEAFS = 10,
	LUMP_FACEIDS = 11,
	LUMP_EDGES = 12,
	LUMP_SURFEDGES = 13,
	LUMP_MODELS = 14,
	LUMP_WORLDLIGHTS = 15,
	LUMP_TEXDATA_STRING_DATA = 43,
	LUMP_TEXDATA_STRING_TABLE = 44,
	// TODO: add more lump types
	// https://developer.valvesoftware.com/wiki/BSP_(Source)#Lump_types
} LumpType;

typedef struct lump_t
{
	int    fileofs;      // offset into file (bytes)
	int    filelen;      // length of lump (bytes)
	int    version;      // lump format version
	char   fourCC[4];    // lump ident code
} lump_t;

typedef struct dheader_t
{
	int     ident;                  // BSP file identifier
	int     version;                // BSP file version
	lump_t  lumps[HEADER_LUMPS];    // lump directory array
	int     mapRevision;            // the map's revision (iteration, version) number
} dheader_t;

typedef struct dedge_t {
	unsigned short v[2]; // vertex indices
} dedge_t;

typedef char byte;

typedef struct dface_t
{
	unsigned short  planenum;               // the plane number
	byte            side;                   // faces opposite to the node's plane direction
	byte            onNode;                 // 1 of on node, 0 if in leaf
	int             firstedge;              // index into surfedges
	short           numedges;               // number of surfedges
	short           texinfo;                // texture info
	short           dispinfo;               // displacement info
	short           surfaceFogVolumeID;     // ?
	byte            styles[4];              // switchable lighting info
	int             lightofs;               // offset into lightmap lump
	float           area;                   // face area in units^2
	int             LightmapTextureMinsInLuxels[2]; // texture lighting info
	int             LightmapTextureSizeInLuxels[2]; // texture lighting info
	int             origFace;               // original face this was split from
	unsigned short  numPrims;               // primitives
	unsigned short  firstPrimID;
	unsigned int    smoothingGroups;        // lightmap smoothing group
} dface_t;

typedef struct texinfo_t
{
	float   textureVecs[2][4];    // [s/t][xyz offset]
	float   lightmapVecs[2][4];   // [s/t][xyz offset] - length is in units of texels/area
	int     flags;                // miptex flags overrides
	int     texdata;              // Pointer to texture name, size, etc.
} texinfo_t;

typedef struct dtexdata_t
{
	Vector  reflectivity;            // RGB reflectivity
	int     nameStringTableID;       // index into TexdataStringTable
	int     width, height;           // source image
	int     view_width, view_height;
} dtexdata_t;

typedef struct dmodel_t
{
	Vector  mins, maxs;            // bounding box
	Vector  origin;                // for sounds or lights
	int     headnode;              // index into node array
	int     firstface, numfaces;   // index into face array
} dmodel_t;