// GND.cpp: implementation of the GND class.
//
//////////////////////////////////////////////////////////////////////

#include "GND.h"
#include "3DMath.h"
#include "iPictureTex.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#define L 10.0


GND::GND()
{

}

GND::~GND()
{

}

void SetTitle(HWND window, char *format, ...) {
    va_list vl;
	va_start(vl, format);
	char title[512];

	if (window) {
			vsprintf(title, format, vl);
			SetWindowText(window, title);
	}
	va_end(vl);
}

void GND::LoadWater(char *ragnapath, int type)
{
	char *texname;
	int i;

	watertex = new GLuint[32];

	glGenTextures(32, watertex);

	texname = (char *) malloc(32);

	for (i = 0; i < 32; i++)
	{	
		//sprintf(texname,"data\\texture\\워터\\water%d%02d.bmp",type, i);
		sprintf(texname,"%sdata\\texture\\워터\\water%d%02d.jpg",ragnapath, type, i);
		//LoadTextureAlpha (ragnapath, texname, watertex[i],192);
		BuildTexture (texname, watertex[i],128);
		glTexParameterf(watertex[i],GL_TEXTURE_WRAP_S,GL_REPEAT);
		glTexParameterf(watertex[i],GL_TEXTURE_WRAP_T,GL_REPEAT);
	}	
}


void GND::DisplayWater(int frameno, float wavephase, float waterlevel, CFrustum g_Frustum)
{
	GLfloat Rot[16];
	CVector3 vTriangle[3];
	CVector3 vNormal;
	float h1=0, h2=0, h3=0, h4=0, ph=0;
	int i=0, j=0;

	glPushMatrix();

	Rot[0] = 1.0;
	Rot[1] = 0.0;
	Rot[2] = 0.0;
	Rot[3] = 0.0;

	Rot[4] = 0.0;
	Rot[5] = 0.0;
	Rot[6] = 1.0;
	Rot[7] = 0.0;

	Rot[8] = 0.0;
	Rot[9] = -1.0;
	Rot[10] = 0.0;
	Rot[11] = 0.0;

	Rot[12] = 0.0;
	Rot[13] = 0.0;
	Rot[14] = 0.0;
	Rot[15] = 1.0;

	//Rotate 90 degrees about the z axis
	glMultMatrixf(Rot);

	//Center it
	glTranslatef(-sizeX*L/2, -sizeY*L/2, waterlevel);

	g_Frustum.CalculateFrustum();

	GLfloat Mat[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, &Mat[0]);

	glDisable(GL_LIGHTING);

	for (i = 0; i < sizeY; i++) 
	for (j = 0; j < sizeX; j++) 
	{
		int tilesup;
		int tileside;
		int tileotherside;


		if (g_Frustum.CubeInFrustum(j*L, i*L, h1,L*2))
		{

			h1 = sinf(j*5 + wavephase) + sinf(i*7 + wavephase);
			h2 = sinf(j*5 + wavephase) + sinf((i+1)*7 + wavephase);
			h3 = sinf((j+1)*5 + wavephase) + sinf((i+1)*7 + wavephase);
			h4 = sinf((j+1)*5 + wavephase) + sinf(i*7 + wavephase);

			vTriangle[0] = MatrixMultVect3f(Mat, j*L,     i*L,	   h1);
			vTriangle[1] = MatrixMultVect3f(Mat, j*L,     (i+1)*L, h2);
			vTriangle[2] = MatrixMultVect3f(Mat, (j+1)*L, (i+1)*L, h3);

			vNormal = Normal(vTriangle);

			glBindTexture(GL_TEXTURE_2D, watertex[frameno]);

			//number of blocks to occupy per texture
			int n = 5;
			float t1,t2,s1,s2;

			t1 = (float(j % n)) / float(n);
			t2 = (float(j % n)+1) / float(n);
			s1 = (float(i % n)) / float(n);
			s2 = (float(i % n)+1) / float(n);

			glBegin(GL_QUADS);
			glNormal3f(vNormal.x,vNormal.y,vNormal.z);

			glTexCoord2f(t1, s1);
			glVertex3f(j*L, i*L, h1);

			glTexCoord2f(t1, s2);
			glVertex3f(j*L, (i+1)*L, h2);

			glTexCoord2f(t2, s2);
			glVertex3f((j+1)*L, (i+1)*L, h3);

			glTexCoord2f(t2, s1);
			glVertex3f((j+1)*L, i*L, h4);

			glEnd();

		}
	}

	glEnable(GL_LIGHTING);

	glPopMatrix();
}

bool GND::GRFLoadGat(Grf *grffile, char *filename, bool extract)
{
	int i;
	int offset;
	unsigned char* GATData;
	unsigned long size;

	GATData = (unsigned char *) grf_get(grffile, filename, &size, NULL);

	if (!GATData) return FALSE;

	if (extract)
	{
		char targfile[512];
		sprintf(targfile,"C:\\Program Files\\RagnarokOffline3\\%s",filename);
		grf_extract (grffile, filename, targfile, NULL);
	}
	

	offset = 0;
	memcpy(&gat.header, &GATData[offset], 4);
	offset += 4;

	memcpy(&gat.iUnknown,  &GATData[offset], sizeof(unsigned short));	
	offset += sizeof(unsigned short);

	memcpy(&gat.sizeX,  &GATData[offset], sizeof(int));	
	offset += sizeof(int);

	memcpy(&gat.sizeY,  &GATData[offset], sizeof(int));	
	offset += sizeof(int);

	gat.gatdata = (ro_gat_t *) malloc(sizeof(ro_gat_t) * gat.sizeX * gat.sizeY);
	memcpy(gat.gatdata, &GATData[offset], sizeof(ro_gat_t) * gat.sizeX * gat.sizeY);	

	return true;
}

bool GND::GRFLoad(Grf *grffile, char *filename, bool extract)
{
	gnd_header_t header;
	ro_string_t *textures_name;
	int i;
	int offset;
	unsigned char* GNDData;
	unsigned long size;

	GNDData = (unsigned char *) grf_get(grffile, filename, &size, NULL);

	if (!GNDData) return FALSE;

	if (extract)
	{
		char targfile[512];
		sprintf(targfile,"C:\\Program Files\\RagnarokOffline3\\%s",filename);
		grf_extract (grffile, filename, targfile, NULL);
	}
	

	offset = 0;
	memcpy(&header, &GNDData[offset], sizeof(gnd_header_t));
	offset += sizeof(gnd_header_t);
	memcpy(&sizeX,  &GNDData[offset], sizeof(int));	
	offset += sizeof(int);
	memcpy(&sizeY,  &GNDData[offset], sizeof(int));	
	offset += sizeof(int);
	offset += 4;
	memcpy(&ntextures, &GNDData[offset], sizeof(int));	
	offset += sizeof(int);
	offset += 4;
	
	textures = new GLuint[ntextures];
	textures_name = new ro_string_t[ntextures];
	
	for (i = 0; i < ntextures; i++) {
		memcpy(&textures_name[i], &GNDData[offset], sizeof(ro_string_t));	
		offset += sizeof(ro_string_t);
		offset += 40;
	}
	
	glGenTextures(ntextures, textures);

	for (i = 0; i < ntextures; i++)
		LoadGRFTexture (grffile, textures_name[i], textures[i], NULL, extract);

	memcpy(&nlightmaps, &GNDData[offset], sizeof(int));	
	offset += sizeof(int);
	offset += 12;
	lightmaps = new ro_lightmap_t[nlightmaps];

	//Water hack!!!
	LoadWater("C:\\Program Files\\Gravity\\RagnarokOnline\\",0);

	memcpy(lightmaps, &GNDData[offset], sizeof(ro_lightmap_t) * nlightmaps);	
	offset += sizeof(ro_lightmap_t) * nlightmaps;

	memcpy(&ntiles, &GNDData[offset], sizeof(int));	
	offset += sizeof(int);

	tiles = new ro_tile_t[ntiles];
	memcpy(tiles, &GNDData[offset], sizeof(ro_tile_t) *  ntiles);	
	offset += sizeof(ro_tile_t) *  ntiles;

	cubes = new ro_cube_t[sizeX*sizeY];
	memcpy(cubes, &GNDData[offset], sizeof(ro_cube_t) *  sizeX * sizeY);	
	offset += sizeof(ro_cube_t) *  sizeX * sizeY;

	return true;
}

bool GND::Load(char *ragnapath, char *filename)
{
	FILE *fp;
	gnd_header_t header;
	ro_string_t *textures_name;
	int i;

	fp = fopen (filename, "rb");
	fread (&header, sizeof(gnd_header_t), 1, fp);
	fread (&sizeX, sizeof(int), 1, fp);
	fread (&sizeY, sizeof(int), 1, fp);
	fseek (fp, 4, SEEK_CUR);
	fread (&ntextures, sizeof(int), 1, fp);
	fseek (fp, 4, SEEK_CUR);
	
	textures = new GLuint[ntextures];
	textures_name = new ro_string_t[ntextures];

	for (i = 0; i < ntextures; i++) {
		fread (&textures_name[i], sizeof(ro_string_t), 1, fp);
		fseek (fp, 40, SEEK_CUR);
	}

	glGenTextures(ntextures, textures);

	for (i = 0; i < ntextures; i++)
		LoadTexture (ragnapath, textures_name[i], textures[i]);

	fread (&nlightmaps, sizeof(int), 1, fp);
	fseek (fp, 12, SEEK_CUR);

	lightmaps = new ro_lightmap_t[nlightmaps];

	LoadWater(ragnapath,0);

	fread (lightmaps, sizeof(ro_lightmap_t), nlightmaps, fp);

	fread (&ntiles, sizeof(int), 1, fp);
	tiles = new ro_tile_t[ntiles];

	fread (tiles, sizeof(ro_tile_t), ntiles, fp);

	cubes = new ro_cube_t[sizeX*sizeY];

	fread (cubes, sizeof(ro_cube_t), sizeX*sizeY, fp);

	fclose(fp);

	return true;
}

void GND::Display(CFrustum g_Frustum)
{
	int i, j;
	GLfloat Rot[16];

	CVector3 vTriangle[3];
	CVector3 vNormal;

	glPushMatrix();

	Rot[0] = 1.0;
	Rot[1] = 0.0;
	Rot[2] = 0.0;
	Rot[3] = 0.0;

	Rot[4] = 0.0;
	Rot[5] = 0.0;
	Rot[6] = 1.0;
	Rot[7] = 0.0;

	Rot[8] = 0.0;
	Rot[9] = -1.0;
	Rot[10] = 0.0;
	Rot[11] = 0.0;

	Rot[12] = 0.0;
	Rot[13] = 0.0;
	Rot[14] = 0.0;
	Rot[15] = 1.0;

	//Rotate 90 degrees about the z axis
	glMultMatrixf(Rot);

	//Center it
	glTranslatef(-sizeX*L/2, -sizeY*L/2, 0);

#ifdef USE_FRUSTUM
	g_Frustum.CalculateFrustum();
#endif

	GLfloat Mat[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, &Mat[0]);

	glEnable(GL_TEXTURE_2D);
	glColor4f(1.0f,1.0f,1.0f,1.0f);

	for (i = 0; i < sizeY; i++) 
	for (j = 0; j < sizeX; j++) 
	{
		int tilesup;
		int tileside;
		int tileotherside;

#ifdef USE_FRUSTUM
 	if (g_Frustum.CubeInFrustum(j*L, i*L, cubes[i*sizeX+j].y1,L*2))
		{
#endif
		tilesup = cubes[i*sizeX+j].tilesup;
		tileside = cubes[i*sizeX+j].tileside;
		tileotherside = cubes[i*sizeX+j].tileotherside;

		if (tileotherside != -1) {

			//vTriangle[0] = MatrixMultVect3f(Mat, j*L,     (i+1)*L, cubes[i*sizeX+j].y3);
			//vTriangle[1] = MatrixMultVect3f(Mat, (j+1)*L, (i+1)*L, cubes[(i+1)*sizeX+j].y1);
			//vTriangle[2] = MatrixMultVect3f(Mat, (j+1)*L, (i+1)*L, cubes[i*sizeX+j].y4);

			//vNormal = Normal(vTriangle);

			glBindTexture(GL_TEXTURE_2D, textures[tiles[tileotherside].text]);

			glBegin(GL_QUADS);
				//glNormal3f(vNormal.x,vNormal.y,vNormal.z);

				glTexCoord2f(tiles[tileotherside].u1, tiles[tileotherside].v1);
				glVertex3f(j*L, (i+1)*L, cubes[i*sizeX+j].y3);
				glTexCoord2f(tiles[tileotherside].u2, tiles[tileotherside].v2);
				glVertex3f((j+1)*L, (i+1)*L, cubes[i*sizeX+j].y4);
				glTexCoord2f(tiles[tileotherside].u4, tiles[tileotherside].v4);
				glVertex3f((j+1)*L, (i+1)*L, cubes[(i+1)*sizeX+j].y2);
				glTexCoord2f(tiles[tileotherside].u3, tiles[tileotherside].v3);
				glVertex3f(j*L, (i+1)*L, cubes[(i+1)*sizeX+j].y1);
			glEnd();
		}
		
		if (tileside != -1) {	

			//vTriangle[0] = MatrixMultVect3f(Mat, (j+1)*L, i*L,     cubes[i*sizeX+j].y4);
			//vTriangle[1] = MatrixMultVect3f(Mat, (j+1)*L, i*L,     cubes[(i+1)*sizeX+j].y3);
			//vTriangle[2] = MatrixMultVect3f(Mat, (j+1)*L, (i+1)*L, cubes[i*sizeX+j].y2);

			//vNormal = Normal(vTriangle);

			/*
			glDisable(GL_TEXTURE_2D);
			glBegin(GL_LINES);
				glColor3f(1.0,1.0,0.0);
 				glVertex3f(vTriangle[0].x,vTriangle[0].y,vTriangle[0].z );
				glColor3f(1.0,0.0,0.0);
				glVertex3f(vNormal.x,vNormal.y,vNormal.z);
				glColor3f(1.0,1.0,1.0);
			glEnd();
			glEnable(GL_TEXTURE_2D);
			*/

			glBindTexture(GL_TEXTURE_2D, textures[tiles[tileside].text]);

			glBegin(GL_QUADS);
				//glNormal3f(vNormal.x,vNormal.y,vNormal.z);

				glTexCoord2f(tiles[tileside].u1, tiles[tileside].v1);
				glVertex3f((j+1)*L, (i+1)*L, cubes[i*sizeX+j].y4);
				glTexCoord2f(tiles[tileside].u2, tiles[tileside].v2);
				glVertex3f((j+1)*L, i*L, cubes[i*sizeX+j].y2);
				glTexCoord2f(tiles[tileside].u4, tiles[tileside].v4);
				glVertex3f((j+1)*L, i*L, cubes[i*sizeX+j+1].y1);
				glTexCoord2f(tiles[tileside].u3, tiles[tileside].v3);
				glVertex3f((j+1)*L, (i+1)*L, cubes[i*sizeX+j+1].y3);
			glEnd();
		}

		if (tilesup != -1) {


			/*
			vTriangle[0] = CVector3(j*L,     i*L,     cubes[i*sizeX+j].y1);
			vTriangle[1] = CVector3((j+1)*L, i*L,     cubes[i*sizeX+j].y2);
			vTriangle[2] = CVector3(j*L,     (i+1)*L, cubes[(i+1)*sizeX+j].y3);

			vTriangle[0] = vTriangle[0] * Mat;
			vTriangle[1] = vTriangle[1] * Mat;
			vTriangle[2] = vTriangle[2] * Mat;

			//vTriangle[0] = MatrixMultVect3f(Mat, j*L,     i*L,     cubes[i*sizeX+j].y1);
			//vTriangle[2] = MatrixMultVect3f(Mat, (j+1)*L, i*L,     cubes[i*sizeX+j].y2);
			//vTriangle[1] = MatrixMultVect3f(Mat, j*L,     (i+1)*L, cubes[(i+1)*sizeX+j].y3);

			vNormal = Normal(vTriangle);
			*/
			//glDisable(GL_LIGHTING);
			glBindTexture(GL_TEXTURE_2D, textures[tiles[tilesup].text]);
			glBegin(GL_QUADS);
//			if ((i+j)%2)
//			glColor3f(0.7, 0.7, 0.7);
				//unsigned char r,g,b;

				//r = gat.gatdata[(gat.sizeY - i) + (gat.sizeX - j) * gat.sizeX].layer[3].R;
				//g = gat.gatdata[(gat.sizeY - i) + (gat.sizeX - j) * gat.sizeX].layer[3].G;
				//b = gat.gatdata[(gat.sizeY - i) + (gat.sizeX - j) * gat.sizeX].layer[3].B;

				//glColor3f(float(r)/255.0f,float(g)/255.0f,float(b)/255.0f);
				//glColor3f(1.0f,1.0f,float(b)/255.0f);
				//glColor3f(1.0f,1.0f,1.0f);

				//glNormal3f(vNormal.x,vNormal.y,vNormal.z);
				glTexCoord2f(tiles[tilesup].u1, tiles[tilesup].v1);
				glVertex3f(j*L, i*L, cubes[i*sizeX+j].y1);
				glTexCoord2f(tiles[tilesup].u2, tiles[tilesup].v2);
				glVertex3f((j+1)*L, i*L, cubes[i*sizeX+j].y2);
				glTexCoord2f(tiles[tilesup].u4, tiles[tilesup].v4);
				glVertex3f((j+1)*L, (i+1)*L, cubes[i*sizeX+j].y4);
				glTexCoord2f(tiles[tilesup].u3, tiles[tilesup].v3);
				glVertex3f(j*L, (i+1)*L, cubes[i*sizeX+j].y3);

				glColor3f(1.0f,1.0f,1.0f);

//			if ((i+j)%2)
//			glColor3f(1.0, 1.0, 1.0);

			glEnd();
			//glEnable(GL_LIGHTING);

		}
#ifdef USE_FRUSTUM
		}  //if g_frustum...
#endif
	}

	glPopMatrix();

}