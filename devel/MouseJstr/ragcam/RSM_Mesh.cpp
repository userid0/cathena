// RSM_Mesh.cpp: implementation of the RSM_Mesh class.
//
//////////////////////////////////////////////////////////////////////

#include "RSM_Mesh.h"
#include <math.h>
#include "3DMath.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

RSM_Mesh::RSM_Mesh()
{
	nframes = 0;
	nstep = 0;
	only = false;
}

RSM_Mesh::~RSM_Mesh()
{

}

void RSM_Mesh::Save(FILE *fp, bool main)
{
	int fin = 0;

	fwrite (name, sizeof(ro_string_t), 1, fp);

	if (main)
		fwrite (&todo, sizeof(int), 1, fp);

	fwrite (parent_name, sizeof(ro_string_t), 1, fp);

	if (main) {
		fwrite (ftodo, sizeof(float), 10, fp);
	}

	fwrite (&ntextures, sizeof(int), 1, fp);

	fwrite (which, sizeof(int), ntextures, fp);
	
	fwrite (&transf, sizeof(ro_transf_t), 1, fp);

	fwrite (&npos_vtx, sizeof(int), 1, fp);
	fwrite (pos_vtx, sizeof(ro_vertex_t), npos_vtx, fp);
	fwrite (&ntex_vtx, sizeof(int), 1, fp);
	fwrite (tex_vtx, sizeof(ro_vertex_t), ntex_vtx, fp);
	fwrite (&nall_faces, sizeof(int), 1, fp);
	fwrite (all_faces, sizeof(ro_face_t), nall_faces, fp);

	if (nframes) {
		fwrite (&nframes, sizeof(int), 1, fp);
		fwrite (frames, sizeof(ro_frame_t), nframes, fp);
	} else {
		fwrite (&fin, sizeof(int), 1, fp);
	}
}


bool RSM_Mesh::GRFLoad(int *offset, unsigned char *p, GLuint *all_textures, bool *all_alphatex, bool main)
{
	int i;
	//int *surfnum;


	memcpy(name, p + *offset, sizeof(ro_string_t));
	*offset += sizeof(ro_string_t);
	
	if (main)
	{
		memcpy(&todo, p + *offset, sizeof(int));
		*offset += sizeof(int);
		//fread (&todo, sizeof(int), 1, fp);
	}

	memcpy(parent_name, p + *offset, sizeof(ro_string_t));
	*offset += sizeof(ro_string_t);
	//fread (parent_name, sizeof(ro_string_t), 1, fp);

	if (main)
	{
		memcpy(&ftodo, p + *offset, sizeof(float) * 10);
		*offset += sizeof(float) * 10;
		//fread (ftodo, sizeof(float), 10, fp);
	}

	memcpy(&ntextures, p + *offset, sizeof(int));
	*offset += sizeof(int);
	//fread (&ntextures, sizeof(int), 1, fp);

	textures = new GLuint[ntextures];
	alphatex = new bool[ntextures];
	which = new int[ntextures];

	for (i = 0; i < ntextures; i++) {
		int n;
		memcpy(&n, p + *offset, sizeof(int));
		*offset += sizeof(int);
		//fread (&n, sizeof(int), 1, fp);
		which[i] = n;
		textures[i] = all_textures[n];
		alphatex[i] = all_alphatex[i];
	}

	memcpy(&transf, p + *offset, sizeof(ro_transf_t));
	*offset += sizeof(ro_transf_t);
	//fread (&transf, sizeof(ro_transf_t), 1, fp);

	memcpy(&npos_vtx, p + *offset, sizeof(int));
	*offset += sizeof(int);
	//fread (&npos_vtx, sizeof(int), 1, fp);
	pos_vtx = new ro_vertex_t[npos_vtx];
	memcpy(pos_vtx, p + *offset, sizeof(ro_vertex_t) * npos_vtx);
	*offset += sizeof(ro_vertex_t) * npos_vtx;
	//fread (pos_vtx, sizeof(ro_vertex_t), npos_vtx, fp);

	memcpy(&ntex_vtx, p + *offset, sizeof(int));
	*offset += sizeof(int);
	//fread (&ntex_vtx, sizeof(int), 1, fp);
	tex_vtx = new ro_vertex_t[ntex_vtx];
	memcpy(tex_vtx, p + *offset, sizeof(ro_vertex_t) * ntex_vtx);
	*offset += sizeof(ro_vertex_t) * ntex_vtx;
	//fread(tex_vtx, sizeof(ro_vertex_t), ntex_vtx, fp);

	memcpy(&nall_faces, p + *offset, sizeof(int));
	*offset += sizeof(int);
	//fread(&nall_faces, sizeof(int), 1, fp);
	all_faces = new ro_face_t[nall_faces];
	memcpy(all_faces, p + *offset, sizeof(ro_face_t) * nall_faces);
	*offset += sizeof(ro_face_t) * nall_faces;
	//fread (all_faces, sizeof(ro_face_t), nall_faces, fp);

	if (data_status(offset,p) != 2) {
		return true;
	}

	memcpy(&nframes, p + *offset, sizeof(int));
	*offset += sizeof(int);
	//fread (&nframes, sizeof(int), 1, fp);
	frames = new ro_frame_t[nframes];
	memcpy(frames, p + *offset, sizeof(ro_frame_t) * nframes);
	*offset += sizeof(ro_frame_t) * nframes;
	//fread (frames, sizeof(ro_frame_t), nframes, fp);

	return true;
}


bool RSM_Mesh::Load(FILE *fp, GLuint *all_textures, bool *all_alphatex, bool main)
{
	int i;
	//char buffer[1024];
	//int *surfnum;
	//FILE *fout;

	fread (name, sizeof(ro_string_t), 1, fp);
	
	if (main)
		fread (&todo, sizeof(int), 1, fp);

	fread (parent_name, sizeof(ro_string_t), 1, fp);

	if (main)
		fread (ftodo, sizeof(float), 10, fp);

	fread (&ntextures, sizeof(int), 1, fp);

	//ugly hack
	if(ntextures < 0) return FALSE;

	textures = new GLuint[ntextures];
	alphatex = new bool[ntextures];
	which = new int[ntextures];

	for (i = 0; i < ntextures; i++) {
		int n;
		fread (&n, sizeof(int), 1, fp);
		which[i] = n;
		textures[i] = all_textures[n];
		alphatex[i] = all_alphatex[i];
	}

	fread (&transf, sizeof(ro_transf_t), 1, fp);

	fread (&npos_vtx, sizeof(int), 1, fp);
	pos_vtx = new ro_vertex_t[npos_vtx];
	fread (pos_vtx, sizeof(ro_vertex_t), npos_vtx, fp);

	fread (&ntex_vtx, sizeof(int), 1, fp);
	tex_vtx = new ro_vertex_t[ntex_vtx];
	fread(tex_vtx, sizeof(ro_vertex_t), ntex_vtx, fp);

	fread(&nall_faces, sizeof(int), 1, fp);
	all_faces = new ro_face_t[nall_faces];
	fread (all_faces, sizeof(ro_face_t), nall_faces, fp);

	if (file_status(fp) != 2) {
		return true;
	}

	fread (&nframes, sizeof(int), 1, fp);
	frames = new ro_frame_t[nframes];
	fread (frames, sizeof(ro_frame_t), nframes, fp);

	return true;
}



void RSM_Mesh::BoundingBox(ro_transf_t *ptransf)
{
	int main = (ptransf == NULL);
	GLfloat Rot[16];
	//GLfloat *Transf;
	int i;
	int j;
	//int k;
	//GLfloat pmax[3], pmin[3];

	Rot[0] = transf.todo[0];
	Rot[1] = transf.todo[1];
	Rot[2] = transf.todo[2];
	Rot[3] = 0.0;

	Rot[4] = transf.todo[3];
	Rot[5] = transf.todo[4];
	Rot[6] = transf.todo[5];
	Rot[7] = 0.0;

	Rot[8] = transf.todo[6];
	Rot[9] = transf.todo[7];
	Rot[10] = transf.todo[8];
	Rot[11] = 0.0;

	Rot[12] = 0.0;
	Rot[13] = 0.0;
	Rot[14] = 0.0;
	Rot[15] = 1.0;

	max[0] = max[1] = max[2] = -999999.0;
	min[0] = min[1] = min[2] = 999999.0;

	for (i = 0; i < npos_vtx; i++) {
		GLfloat vout[3]; // vtemp[3] 

		MatrixMultVect(Rot, pos_vtx[i], vout);
		for (j = 0; j < 3; j++) {
			GLfloat f;
			if (!only)
				f = vout[j] + transf.todo[12+j] + transf.todo[9+j];
			else
				f = vout[j];

			min[j] = MIN(f, min[j]);
			max[j] = MAX(f, max[j]);
		}
	}

	for (j=0; j < 3; j++)
		range[j] = (max[j]+min[j])/2.0;
}

void RSM_Mesh::Display(bounding_box_t *box, ro_transf_t *ptransf)
{
	int i;

	GLfloat Rot[16];
	GLfloat Ori[16];

	int main = (ptransf == NULL);

	CVector3 vNormal;
	CVector3 vTriangle[3];

	Rot[0] = transf.todo[0];
	Rot[1] = transf.todo[1];
	Rot[2] = transf.todo[2];
	Rot[3] = 0.0;

	Rot[4] = transf.todo[3];
	Rot[5] = transf.todo[4];
	Rot[6] = transf.todo[5];
	Rot[7] = 0.0;

	Rot[8] = transf.todo[6];
	Rot[9] = transf.todo[7];
	Rot[10] = transf.todo[8];
	Rot[11] = 0.0;

	Rot[12] = 0.0;
	Rot[13] = 0.0;
	Rot[14] = 0.0;
	Rot[15] = 1.0;

	if (nframes) {
		int i;
		int current = 0;
		int next;
		GLfloat t;
		GLfloat q[4], q1[4], q2[4];
		GLfloat x,y,z,w;
		char buffer[1024];

		for (i = 0; i < nframes; i++) {
			if (nstep < frames[i].time) {
				current = i-1;
				break;
			}
		}

		next = current + 1;
		if (next == nframes)
			next = 0;

		t = ((GLfloat) (nstep-frames[current].time))
			/((GLfloat) (frames[next].time-frames[current].time));

		//for (i = 0; i < 4; i++) {
		//	q[i] = frames[current].orientation[i] * (1-t) + t * frames[next].orientation[i];
		//}

		x = frames[current].orientation[0] * (1-t) + t * frames[next].orientation[0];
		y = frames[current].orientation[1] * (1-t) + t * frames[next].orientation[1];
		z = frames[current].orientation[2] * (1-t) + t * frames[next].orientation[2];
		w = frames[current].orientation[3] * (1-t) + t * frames[next].orientation[3];

		GLfloat norm;
		norm = sqrtf(x*x+y*y+z*z+w*w);

		//for (i = 0; i < 4; i++)
		//	q[i] /= norm;
		x /= norm;
		y /= norm;
		z /= norm;
		w /= norm;

		// First row
		Ori[ 0] = 1.0f - 2.0f * ( y * y + z * z ); 
		Ori[ 1] = 2.0f * (x * y + z * w);
		Ori[ 2] = 2.0f * (x * z - y * w);
		Ori[ 3] = 0.0f;  

		// Second row
		Ori[ 4] = 2.0f * ( x * y - z * w );  
		Ori[ 5] = 1.0f - 2.0f * ( x * x + z * z ); 
		Ori[ 6] = 2.0f * (z * y + x * w );  
		Ori[ 7] = 0.0f;  

		// Third row
		Ori[ 8] = 2.0f * ( x * z + y * w );
		Ori[ 9] = 2.0f * ( y * z - x * w );
		Ori[10] = 1.0f - 2.0f * ( x * x + y * y );  
		Ori[11] = 0.0f;  

		// Fourth row
		Ori[12] = 0;  
		Ori[13] = 0;  
		Ori[14] = 0;  
		Ori[15] = 1.0f;

		/*
		//GLfloat two_x = q[0] * 2.0;
		//GLfloat two_y = q[1] * 2.0;
		//GLfloat two_z = q[2] * 2.0;

		Ori[0] = 1.0 - q[1] * 2.0; * q[1] - q[2] * 2.0 * q[2];
		Ori[1] = q[0] * 2.0 * q[1];
		Ori[2] = two_z * q[0];
		Ori[3] = 0.0;

		Ori[4] = two_x * q[1];
		Ori[5] = 1.0 - two_x * q[0] - two_z * q[2];
		Ori[6] = two_y * q[2];
		Ori[7] = 0.0;

		Ori[8] = two_z * q[0];
		Ori[9] = two_y * q[2];
		Ori[10] = 1.0 - two_x * q[0] - two_y * q[1];
		Ori[11] = 0.0;

		Ori[12] = 0.0;
		Ori[13] = 0.0;
		Ori[14] = 0.0;
		Ori[15] = 1.0;
		*/

		nstep += 100;
		if (nstep >= frames[nframes-1].time)
			nstep = 0;
	}

	glScalef (transf.todo[19], transf.todo[20], transf.todo[21]);

	if (main)
		if (!only) {
			glTranslatef(-box->range[0], -box->max[1], -box->range[2]);
		} else {
			glTranslatef(0.0, -box->max[1]+box->range[1], 0.0);
		}

	if (!main)	
		glTranslatef(transf.todo[12], transf.todo[13], transf.todo[14]);

	if (!nframes)
		glRotatef(transf.todo[15]*180.0/3.14159,
			transf.todo[16], transf.todo[17], transf.todo[18]);
	else
		glMultMatrixf(Ori);


	glPushMatrix();

	if (main && only)
		glTranslatef(-box->range[0], -box->range[1], -box->range[2]);

	if (!main || !only)
		glTranslatef(transf.todo[9], transf.todo[10], transf.todo[11]);

	glMultMatrixf(Rot);

	for (i = 0; i < nall_faces; i++) {
		ro_vertex_t *v;
		ro_vertex_t *t;
		int texture;

		if (all_faces[i].text > ntextures || all_faces[i].text < 0) {
			if (i==0) texture = 0;
		} else {
			texture = all_faces[i].text;
		}

		glBindTexture(GL_TEXTURE_2D, textures[texture]);

		if (alphatex[texture]) {
		  glEnable(GL_BLEND);
		  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		} else {
		  glEnable(GL_ALPHA_TEST);
		  glAlphaFunc(GL_GREATER, 0.90);
		}

		GLfloat Mat[16];

		glGetFloatv(GL_MODELVIEW_MATRIX, &Mat[0]);

		float vin[3], vout[3];

#define POSVTX(n) *&pos_vtx[all_faces[n].v[0]]
#define GETNORMVECT(n) vTriangle[n] = MatrixMultVect3f(Mat, POSVTX(n)[0], POSVTX(n)[1], POSVTX(n)[2])

		vTriangle[0] = CVector3(pos_vtx[all_faces[i].v[0]]);
		vTriangle[1] = CVector3(pos_vtx[all_faces[i].v[1]]);
		vTriangle[2] = CVector3(pos_vtx[all_faces[i].v[2]]);

		vTriangle[0] = vTriangle[0] * Mat;
		vTriangle[1] = vTriangle[1] * Mat;
		vTriangle[2] = vTriangle[2] * Mat;

		//GETNORMVECT(0);
		//GETNORMVECT(1);
		//GETNORMVECT(2);

		vNormal = Normal(vTriangle);

		/*
		glPushMatrix();

		glLoadIdentity();

		glDisable(GL_LIGHTING);

		glBegin(GL_LINES);    
		glColor3f(1.0f,0.0f,0.0f);
		glVertex3f (vTriangle[0].x, vTriangle[0].y, vTriangle[0].z);
		glColor3f(1.0f,1.0f,0.0f);
		glVertex3f (vNormal.x, vNormal.y, vNormal.z);
		glEnd();

		glEnable(GL_LIGHTING);

		glPopMatrix();
		*/

		/*glPushMatrix();

		glLoadIdentity();

		glBegin(GL_TRIANGLES);    

		glVertex3f (vTriangle[0].x, vTriangle[0].y, vTriangle[0].z);
		glVertex3f (vTriangle[1].x, vTriangle[1].y, vTriangle[1].z);
		glVertex3f (vTriangle[2].x, vTriangle[2].y, vTriangle[2].z);
		glEnd();

		glPopMatrix();
		*/

		glBegin(GL_TRIANGLES);    

		glNormal3f (vNormal.x,vNormal.y,vNormal.z);

		glTexCoord2fv (&tex_vtx[all_faces[i].t[0]][1]);
		glVertex3fv (pos_vtx[all_faces[i].v[0]]);

		glTexCoord2fv (&tex_vtx[all_faces[i].t[2]][1]);
		glVertex3fv (pos_vtx[all_faces[i].v[2]]);

		glTexCoord2fv (&tex_vtx[all_faces[i].t[1]][1]);
		glVertex3fv (pos_vtx[all_faces[i].v[1]]);

		glEnd();

		if (alphatex[texture]) {
			glDisable(GL_BLEND);
		} else {
			glDisable(GL_ALPHA_TEST);
		}
	}


	glPopMatrix();
}
