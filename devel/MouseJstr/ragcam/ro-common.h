#ifndef RO_COMMON_H
#define RO_COMMON_H

#include <windows.h>
#include <math.h>		// Header File For Windows
#include <stdio.h>
#include <stdlib.h>
#include <gl\gl.h>			// Header File For The OpenGL32 Library
#include <gl\glu.h>			// Header File For The GLu32 Library
#include <gl\glaux.h>		// Header File For The Glaux Library
#include "grf.h"

#define MIN(a, b) (a>b)?b:a
#define MAX(a, b) (a>b)?a:b

#define USE_FRUSTUM



typedef char rgb[4];

typedef char ro_string_t[40];

typedef struct {
	GLfloat x;
	GLfloat y;
	GLfloat z;
	GLfloat rx;
	GLfloat ry;
	GLfloat rz;
	GLfloat sx;
	GLfloat sy;
	GLfloat sz;
} ro_position_t;

// This will allow us to create an object to keep track of our frustum
class CFrustum {

public:

	// Call this every time the camera moves to update the frustum
	void CalculateFrustum();

	// This takes a 3D point and returns TRUE if it's inside of the frustum
	bool PointInFrustum(float x, float y, float z);

	// This takes a 3D point and a radius and returns TRUE if the sphere is inside of the frustum
	bool SphereInFrustum(float x, float y, float z, float radius);

	// This takes the center and half the length of the cube.
	bool CubeInFrustum( float x, float y, float z, float size );

	bool BoxInFrustum( float x, float y, float z, float width, float height, float length);

private:

	// This holds the A B C and D values for each side of our frustum.
	float m_Frustum[6][4];
};

// This is our basic 3D point/vector class
class CVector3
{
public:
	
	// A default constructor
	CVector3() {}


	// This is our constructor that allows us to initialize our data upon creating an instance
	CVector3(float *pVertex3) 
	{ 
		x = pVertex3[0]; y = pVertex3[1]; z = pVertex3[2];
	}
	
	// This is our constructor that allows us to initialize our data upon creating an instance
	CVector3(float X, float Y, float Z) 
	{ 
		x = X; y = Y; z = Z;
	}

	// Here we overload the + operator so we can add vectors together 
	CVector3 operator+(CVector3 vVector)
	{
		// Return the added vectors result.
		return CVector3(vVector.x + x, vVector.y + y, vVector.z + z);
	}

	// Here we overload the - operator so we can subtract vectors 
	CVector3 operator-(CVector3 vVector)
	{
		// Return the subtracted vectors result
		return CVector3(x - vVector.x, y - vVector.y, z - vVector.z);
	}
	
	// Here we overload the * operator so we can multiply by scalars
	CVector3 operator*(float num)
	{
		// Return the scaled vector
		return CVector3(x * num, y * num, z * num);
	}

	CVector3 operator*(GLfloat matrix[16])
	{
		CVector3 tempvect;
		tempvect.x = x * matrix[0] + y * matrix[4] + z * matrix[8]  + 1.0 * matrix[12];
		tempvect.y = x * matrix[1] + y * matrix[5] + z * matrix[9]  + 1.0 * matrix[13];
		tempvect.z = x * matrix[2] + y * matrix[6] + z * matrix[10] + 1.0 * matrix[14];
		return tempvect;	
	}

	// Here we overload the / operator so we can divide by a scalar
	CVector3 operator/(float num)
	{
		// Return the scale vector
		return CVector3(x / num, y / num, z / num);
	}

	float x, y, z;						
};


void goOrtho();
void goPerspective();

GLvoid BuildFont(HDC hDC);					// Build Our Bitmap Font
GLvoid KillFont(GLvoid);
GLvoid glPrint(GLint x, GLint y, const char *string, ...);

GLvoid *LoadGRFBitmapAlpha(Grf *grfFile, char *filename, unsigned char alpha);
bool LoadGRFTexture(Grf *grfFile, char *filename, GLuint texture, bool *alpha=NULL, bool extract = FALSE);
bool LoadTexture(char *ragnapath, char *filename, GLuint texture, bool *alpha=NULL);
bool LoadGRFTextureAlpha(Grf *grfFile, char *filename, GLuint texture, unsigned char alpha);
bool LoadTextureAlpha(char *ragnapath, char *filename, GLuint texture, unsigned char alpha);

void MatrixMultVect(const GLfloat *M, const GLfloat *Vin, GLfloat *Vout);
CVector3 MatrixMultVect3f(const GLfloat *M, float x, float y, float z);

AUX_RGBImageRec *LoadBMP(char *Filename);

#endif // RO_COMMON_H