// RSM_Mesh.h: interface for the RSM_Mesh class.
//
//////////////////////////////////////////////////////////////////////

#ifndef RSM_MESH_H
#define RSM_MESH_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Rsm-common.h"

class RSM_Mesh  
{
public:
	RSM_Mesh();
	bool GRFLoad(int *offset, unsigned char *p, GLuint *all_textures, bool *all_alphatex, bool main);
	bool Load(FILE *fp, GLuint *all_textures, bool *alphatex, bool main);
	void Save(FILE *fp, bool main);
	void Display(bounding_box_t *box, ro_transf_t *ptransf = NULL);
	void BoundingBox(ro_transf_t *ptransf=NULL);
	virtual ~RSM_Mesh();
	GLfloat range[3];
	GLfloat min[3];
	GLfloat max[3];

	int numverts()  { return npos_vtx;   }
	int numtverts() { return ntex_vtx;   }
	int numfaces() { return nall_faces; }
	int numframes() { return nframes; }
	ro_frame_t getframe(int i) { return frames[i]; }

	ro_string_t	name;
	ro_string_t parent_name;

	ro_transf_t transf;
	ro_vertex_t *pos_vtx;

	bool only;
	bool drawbounds;

private:

private:
	int nstep;
	int todo;
	int *which;
	GLfloat ftodo[10];
	ro_vertex_t *tex_vtx;
	ro_face_t *all_faces;
	int npos_vtx;
	int ntex_vtx;
	int nall_faces;
	int ntextures;
	GLuint *textures;
	bool *alphatex;
	ro_surface_t *surfaces;
	int nframes;
	ro_frame_t *frames;
};

#endif // RSM_MESH_H
