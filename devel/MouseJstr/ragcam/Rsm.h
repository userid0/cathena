// Rsm.h: interface for the Rsm class.
//
//////////////////////////////////////////////////////////////////////

#ifndef RSM_H
#define RSM_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Rsm-common.h"
#include "RSM_Mesh.h"

class RSM  
{
public:
	RSM();
	bool GRFLoad(Grf *grffile, char *filename, bool extract = FALSE);
	bool Load(char *ragnapath, char *filename);
	void Save(char *filename);
	void Display(ro_position_t pos);
	void DisplayMesh(bounding_box_t *b, int n, ro_transf_t *ptransf=NULL);
	void setdrawbounds(bool mode);
	virtual ~RSM();
	bounding_box_t box;
	char modelfname[512];
	int getNumMeshes() {return nmeshes;}
	RSM_Mesh* getMesh(int i) {return &meshes[i];}

private:
	void BoundingBox();

private:
	int *father;
	GLfloat range[3];
	GLfloat min[3];
	GLfloat max[3];
	rsm_header_t header;
	int nmeshes;
	RSM_Mesh *meshes;
	int ntextures;
	GLuint *textures;
	ro_string_t *textures_name;
	bool *alphatex;
};

#endif // RSM_H
