// GND.h: interface for the GND class.
//
//////////////////////////////////////////////////////////////////////

#ifndef GND_H
#define GND_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ro-common.h"

typedef unsigned char ro_lightmap_t[256];

typedef struct {
	float u1;
	float u2;
	float u3;
	float u4;
	float v1;
	float v2;
	float v3;
	float v4;

	unsigned short text;
	unsigned short lmap;

	int todo;
} ro_tile_t;

typedef struct {
	float y1;
	float y2;
	float y3;
	float y4;
	int tilesup;
	int tileotherside;
	int tileside;
} ro_cube_t;

typedef struct {
	char filecode[4];
	short magicnumber;
} gnd_header_t;

class GND  
{
public:
	GND();
	bool GRFLoad (Grf *grffile, char *filename, bool extract = FALSE);
	bool Load (char *ragnapath, char *filename);
	void LoadWater(char *ragnapath, int type);
	void Display(CFrustum g_Frustum);
	void DisplayWater(int frameno, float wavephase, float waterlevel, CFrustum g_Frustum);
	virtual ~GND();
	int sizeX;
	int sizeY;

private:
	int ntextures;
	GLuint *watertex;
	GLuint *textures;
	ro_cube_t *cubes;
	ro_tile_t *tiles;
	int ntiles;
	ro_lightmap_t *lightmaps;
	int nlightmaps;
};

#endif // GND_H