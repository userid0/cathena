#ifndef RSM_COMMON_H
#define RSM_COMMON_H

#include "ro-common.h"

typedef struct {
	char filecode[4];
	char todo[27];
} rsm_header_t;

typedef int *pint;

typedef float ro_vertex_t[3];

typedef float gl_vertex_t[5];

typedef int gl_face_t[3];

typedef struct {
	short v[3];
	short t[3];
	unsigned short	  text;
	unsigned short	todo1;
	unsigned int   todo2;
	unsigned int   nsurf;
} ro_face_t;

typedef struct {
	GLuint text;

	int nvertex;
	int nfaces;

	gl_vertex_t *vertex;
	gl_face_t *faces;
} ro_surface_t;

typedef struct {
	GLfloat todo[22];
} ro_transf_t;

typedef float ro_quat_t[4];

#define QUATERNION_SCALE(q) ((q[0]*q[0]+q[1]*q[1]+q[2]*q[2])*(q[0]*q[0]+q[1]*q[1]+q[2]*q[2]) + q[3]*q[3])

typedef struct {
	int time;
	ro_quat_t orientation;
} ro_frame_t;

int data_status(int *offset, unsigned char *p);
int file_status(FILE *fp);

void DisplayBoundingBox(GLfloat *max, GLfloat *min, 
						GLfloat r=1.0, GLfloat g=1.0, GLfloat b=1.0);

typedef struct {
	GLfloat max[3];
	GLfloat min[3];
	GLfloat range[3];
} bounding_box_t;

#endif // RSM_COMMON_H