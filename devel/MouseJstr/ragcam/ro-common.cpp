#include "ro-common.h"
#include "tgaload.h"

GLuint	base;			// Base Display List For The Font Set
GLfloat	cnt1;			// 1st Counter Used To Move Text & For Coloring
GLfloat	cnt2;			// 2nd Counter Used To Move Text & For Coloring


GLvoid BuildFont(HDC hDC)					// Build Our Bitmap Font
{
	HFONT	font;						// Windows Font ID
	HFONT	oldfont;					// Used For Good House Keeping

	base = glGenLists(96);					// Storage For 96 Characters ( NEW )

	font = CreateFont(	-12,				// Height Of Font ( NEW )
				0,				// Width Of Font
				0,				// Angle Of Escapement
				0,				// Orientation Angle
				FW_NORMAL,			// Font Weight
				FALSE,				// Italic
				FALSE,				// Underline
				FALSE,				// Strikeout
				ANSI_CHARSET,			// Character Set Identifier
				OUT_TT_PRECIS,			// Output Precision
				CLIP_DEFAULT_PRECIS,		// Clipping Precision
				ANTIALIASED_QUALITY,		// Output Quality
				FF_DONTCARE|DEFAULT_PITCH,	// Family And Pitch
				"Arial");			// Font Name
	oldfont = (HFONT)SelectObject(hDC, font);		// Selects The Font We Want
	wglUseFontBitmaps(hDC, 32, 96, base);			// Builds 96 Characters Starting At Character 32
	SelectObject(hDC, oldfont);				// Selects The Font We Want
	DeleteObject(font);					// Delete The Font
}


GLvoid KillFont(GLvoid)						// Delete The Font List
{
 	glDeleteLists(base, 96);				// Delete All 96 Characters ( NEW )
}


GLvoid glPrint(GLint x, GLint y, const char *string, ...)			// Where The Printing Happens
{
	char		text[256];						// Holds Our String
	va_list		ap;							// Pointer To List Of Arguments

	if (string == NULL)							// If There's No Text
		return;								// Do Nothing

	va_start(ap, string);							// Parses The String For Variables
	    vsprintf(text, string, ap);						// And Converts Symbols To Actual Numbers
	va_end(ap);								// Results Are Stored In Text

	glPushMatrix();								// Store The Modelview Matrix
	glLoadIdentity();							// Reset The Modelview Matrix
	glRasterPos2f(x, y); 
	//glTranslated(x,y,0);							// Position The Text (0,0 - Bottom Left)
	glListBase(base-32);							// Choose The Font Set
	glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);			// Draws The Display List Text
	glPopMatrix();								// Restore The Old Projection Matrix
}


unsigned long getlong (unsigned char *p)
{
	return p[0] + p[1] * 256 + (p[2] + p[3] * 256) * 65536;
}

unsigned short getshort (unsigned char *p)
{
	return p[0] + p[1] * 256;
}

AUX_RGBImageRec *LoadBMP(char *Filename)
{
  FILE *File=NULL;	
  
  if (!Filename)	
    {
      return NULL;	
    }

  File=fopen(Filename,"r");
  
  if (File)		
    {
      fclose(File);			
      return auxDIBImageLoad(Filename);
    }
  
  return NULL;
}

#define coord3(t, x, y, o) t[3*((x)+(y)*w)+o] 
#define coord4(t, x, y, o) t[4*((x)+(y)*w)+o] 

bool LoadGRFTexture(Grf *grfFile, char *filename, GLuint texture, bool *alpha, bool extract)
{
	int h, w, j, k;
	short numcol, bpp;
	unsigned char *cdata;
	unsigned char *ndata;
	char buffer[512];
	int l;
	l = strlen(filename);

	sprintf (buffer, "data\\texture\\%s", filename);

	if ((filename[l-3] == 'b' && 
		filename[l-2] == 'm' &&
		filename[l-1] == 'p') || (filename[l-3] == 'B' && 
		filename[l-2] == 'M' &&
		filename[l-1] == 'P')) {

		unsigned char *data;
		unsigned long size;
		GrfError *error;

		data = (unsigned char *) grf_get (grfFile, buffer, &size, error);
		
		if (!data) {
			MessageBox(NULL,"Um...","Error loading file.",MB_OK + MB_ICONEXCLAMATION);
			return FALSE;
		}

		//extract the files (for testing only!)
		if (extract)
		{
			char targfile[512];
			sprintf(targfile,"C:\\Program Files\\RagnarokOffline3\\%s",filename);
			grf_extract (grfFile, filename, targfile, NULL);
		}
		

		w = getlong(&data[18]);
		h = getlong(&data[22]);
		
		bpp = getshort(&data[28]);

		if (bpp < 16)
		{
			numcol = 1<<bpp;
			rgb lut[256];
			int dataptr; 

			memcpy(&lut[0][0],&data[54],sizeof(rgb) * numcol);

			unsigned long dataoffset;
			dataoffset = getlong(&data[10]);
			cdata = (unsigned char *) &data[dataoffset];

			ndata = (unsigned char *) malloc (sizeof(unsigned char)*4*h*w);

			int skip = 0;

			skip = (w * 3) % 4;

			for (j = 0; j < h; j++) 
			{
				for (k = 0; k < w; k++) {
					unsigned char r, g, b;

					r = (int) lut[cdata[(j * skip) + k + j * w]][0];
					g = (int) lut[cdata[(j * skip) + k + j * w]][1];
					b = (int) lut[cdata[(j * skip) + k + j * w]][2];

					ndata[4*((h-1-j)*w+k)] = b;
					ndata[4*((h-1-j)*w+k)+1] = g;
					ndata[4*((h-1-j)*w+k)+2] = r;
					if (g==0 && r>253 && b>253)
						ndata[4*((h-1-j)*w+k)+3] = 0;
					else
						ndata[4*((h-1-j)*w+k)+3] = 255;
				}
			}
		}

		if (bpp == 24)
		{
			unsigned long dataoffset;
			dataoffset = getlong(&data[10]);
			cdata = (unsigned char *) &data[dataoffset];
			ndata = (unsigned char *) malloc (sizeof(unsigned char)*4*h*w);

			int skip = 0;

			skip = (w * 3) % 4;

			int i=0, x=0, y=0;

			for(i = 0; i < (3 * w * h + j * skip * h);i+=3) {
				unsigned char r, g, b;

				r = cdata[i];
				g = cdata[i+1];
				b = cdata[i+2];

				ndata[4*((h-1-y)*w+x)] = b;
				ndata[4*((h-1-y)*w+x)+1] = g;
				ndata[4*((h-1-y)*w+x)+2] = r;
				if (g==0 && r>253 && b>253)
					ndata[4*((h-1-y)*w+x)+3] = 0;
				else
					ndata[4*((h-1-y)*w+x)+3] = 255;

				x++;
				if(x>(w-1))
				{
					i+=skip;
					x=0;
					y++;
				}
			}
		}

		glBindTexture(GL_TEXTURE_2D, texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if(alpha){
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		} else {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		}

		gluBuild2DMipmaps ( GL_TEXTURE_2D, GL_RGBA, w, h, GL_RGBA, GL_UNSIGNED_BYTE, ndata );

		if (alpha) *alpha = false;

		free(ndata);

	} else if ((filename[l-3] == 't' &&
		filename[l-2] == 'g' &&
		filename[l-1] == 'a') || (filename[l-3] == 'T' &&
		filename[l-2] == 'G' &&
		filename[l-1] == 'A')) {

		GrfError *error;

		grf_extract (grfFile, filename, "\temp.tga" , error);

		image_t   p;

	    glBindTexture(GL_TEXTURE_2D, texture);
	    tgaLoad  ( "\temp.tga", &p, TGA_NO_PASS);

		h = p.info.height;
		w = p.info.width;

		cdata = (unsigned char *) p.data;
		ndata = (unsigned char *) malloc (sizeof(unsigned char)*4*h*w);

		for (j = 0; j < h; j++) 
		  for (k = 0; k < w; k++) {
		  unsigned char a, r, g, b;

		  r = coord4(cdata, k, h-1-j, 0);
		  g = coord4(cdata, k, h-1-j, 1);
		  b = coord4(cdata, k, h-1-j, 2);
		  a = coord4(cdata, k, h-1-j, 3);

		  ndata[4*(j*w+k)] = r;
		  ndata[4*(j*w+k)+1] = g;
		  ndata[4*(j*w+k)+2] = b;
		  ndata[4*(j*w+k)+3] = a;
		}

			char buf[512];
			sprintf(buf, "%d\n", p.info.tgaColourType);
		//	MessageBox(NULL, buf, NULL, 0);

			glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

			gluBuild2DMipmaps ( GL_TEXTURE_2D, p.info.tgaColourType, p.info.width,
					  p.info.height, p.info.tgaColourType, GL_UNSIGNED_BYTE, ndata );

			free(ndata);

		if (alpha) *alpha = true;
	
	} else {
		MessageBox (NULL, "Unknown texture type", "Error", 0);	
		return false;
	}

	return true;
}

bool LoadTexture(char *ragnapath, char *filename, GLuint texture, bool *alpha)
{
	int h, w, j, k;
	unsigned char *cdata;
	unsigned char *ndata;
	char buffer[512];
	int l;
	l = strlen(filename);

	sprintf (buffer, "%sdata\\texture\\%s", ragnapath, filename);

	if ((filename[l-3] == 'b' && 
		filename[l-2] == 'm' &&
		filename[l-1] == 'p') || (filename[l-3] == 'B' && 
		filename[l-2] == 'M' &&
		filename[l-1] == 'P')) {

	AUX_RGBImageRec *image;
    image = LoadBMP(buffer);
	if(!image) image = LoadBMP("blank.bmp");
    h = image->sizeY;
    w = image->sizeX;

    cdata = (unsigned char *) image->data;
    ndata = (unsigned char *) malloc (sizeof(unsigned char)*4*h*w);

    for (j = 0; j < h; j++) 
      for (k = 0; k < w; k++) {
      unsigned char r, g, b;

      b = coord3(cdata, k, h-1-j, 0);
      g = coord3(cdata, k, h-1-j, 1);
      r = coord3(cdata, k, h-1-j, 2);

      ndata[4*(j*w+k)] = b;
      ndata[4*(j*w+k)+1] = g;
      ndata[4*(j*w+k)+2] = r;
      if (g==0 && r>253 && b>253)
		ndata[4*(j*w+k)+3] = 0;
      else
		ndata[4*(j*w+k)+3] = 255;
    }

    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//if(alpha)
	//{
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//}
	//else 
	//{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	//}

	gluBuild2DMipmaps ( GL_TEXTURE_2D, GL_RGBA, w, h,
		GL_RGBA, GL_UNSIGNED_BYTE, ndata );

		if (alpha)
			*alpha = false;
	} else if ((filename[l-3] == 't' &&
		filename[l-2] == 'g' &&
		filename[l-1] == 'a') || (filename[l-3] == 'T' &&
		filename[l-2] == 'G' &&
		filename[l-1] == 'A')) {


		image_t   p;

	    glBindTexture(GL_TEXTURE_2D, texture);
	    tgaLoad  ( buffer, &p, TGA_NO_PASS);

    h = p.info.height;
    w = p.info.width;

    cdata = (unsigned char *) p.data;
    ndata = (unsigned char *) malloc (sizeof(unsigned char)*4*h*w);

    for (j = 0; j < h; j++) 
      for (k = 0; k < w; k++) {
      unsigned char a, r, g, b;

      r = coord4(cdata, k, h-1-j, 0);
      g = coord4(cdata, k, h-1-j, 1);
      b = coord4(cdata, k, h-1-j, 2);
      a = coord4(cdata, k, h-1-j, 3);

      ndata[4*(j*w+k)] = r;
      ndata[4*(j*w+k)+1] = g;
      ndata[4*(j*w+k)+2] = b;
	  ndata[4*(j*w+k)+3] = a;
    }

		char buf[512];
		sprintf(buf, "%d\n", p.info.tgaColourType);
	//	MessageBox(NULL, buf, NULL, 0);

	    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);

	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		gluBuild2DMipmaps ( GL_TEXTURE_2D, p.info.tgaColourType, p.info.width,
                  p.info.height, p.info.tgaColourType, GL_UNSIGNED_BYTE, ndata );

		free(ndata);
	
		if (alpha)
			*alpha = true;
	} else {
		MessageBox (NULL, "Unknown texture type", "Error", 0);
		return false;
	}

	return true;
}

GLvoid *LoadGRFBitmapAlpha(Grf *grfFile, char *filename, unsigned char alpha)
{
	GLvoid *pixeldata;
	int h, w, j, k;
	short numcol, bpp;
	unsigned char *cdata;
	unsigned char *ndata;
	char buffer[512];
	int l;
	l = strlen(filename);

	unsigned char *data;
	unsigned long size;
	GrfError *error;

	//sprintf (buffer, "%s\\data\\texture\\%s", ragnapath, filename);

	if ((filename[l-3] == 'b' && filename[l-2] == 'm' && filename[l-1] == 'p') 
	 || (filename[l-3] == 'B' && filename[l-2] == 'M' && filename[l-1] == 'P')) 
	{

		data = (unsigned char *) grf_get (grfFile, filename, &size, error);
		if (!data) {
			return FALSE;
		}

		w = getlong(&data[18]);
		h = getlong(&data[22]);
		
		bpp = getshort(&data[28]);

		if (bpp < 16)
		{
			numcol = 1<<bpp;
			rgb lut[256];
			int dataptr; 

			memcpy(&lut[0][0],&data[54],sizeof(rgb)*numcol);

			dataptr = 54 + sizeof(rgb) * numcol;

			ndata = (unsigned char *) malloc (sizeof(unsigned char)*4*h*w);
			for (j = 0; j < h; j++) 
			  for (k = 0; k < w; k++) {
			  unsigned char r, g, b;

			  r = (int) lut[data[dataptr + k + (h-1-j) * w]][0];
			  g = (int) lut[data[dataptr + k + (h-1-j) * w]][1];
			  b = (int) lut[data[dataptr + k + (h-1-j) * w]][2];

			  ndata[4*(j*w+k)] = b;
			  ndata[4*(j*w+k)+1] = g;
			  ndata[4*(j*w+k)+2] = r;
			  //  if (g==0 && r>253 && b>253)
			  //	ndata[4*(j*w+k)+3] = 0;
			  //  else
			  ndata[4*(j*w+k)+3] = alpha;
			}
		}

		if (bpp == 24)
		{
			unsigned long dataoffset;
			dataoffset = getlong(&data[10]);
			cdata = (unsigned char *) &data[dataoffset];
			ndata = (unsigned char *) malloc (sizeof(unsigned char)*4*h*w);

			for (j = 0; j < h; j++) 
			  for (k = 0; k < w; k++) {
			  unsigned char r, g, b;

			  r = coord3(cdata, k, h-1-j, 0);
			  g = coord3(cdata, k, h-1-j, 1);
			  b = coord3(cdata, k, h-1-j, 2);

			  ndata[4*(j*w+k)] = b;
			  ndata[4*(j*w+k)+1] = g;
			  ndata[4*(j*w+k)+2] = r;
			  //if (g==0 && r>253 && b>253)
				//ndata[4*(j*w+k)+3] = 0;
			  //else
			  ndata[4*(j*w+k)+3] = alpha;
			}
		}

		pixeldata = (GLvoid *) malloc(sizeof(unsigned char)*4*h*w);
		memcpy(pixeldata, ndata, sizeof(unsigned char)*4*h*w);

		free(ndata);
		free(data);

	} else {
		MessageBox (NULL, "Unknown texture type", "Error", 0);
		return false;
	}

	return pixeldata;
}


bool LoadGRFTextureAlpha(Grf *grfFile, char *filename, GLuint texture, unsigned char alpha)
{
	int h, w, j, k;
	short numcol, bpp;
	unsigned char *cdata;
	unsigned char *ndata;
	char buffer[512];
	int l;
	l = strlen(filename);

	unsigned char *data;
	unsigned long size;
	GrfError *error;

	//sprintf (buffer, "%s\\data\\texture\\%s", ragnapath, filename);

	if ((filename[l-3] == 'b' && filename[l-2] == 'm' && filename[l-1] == 'p') 
	 || (filename[l-3] == 'B' && filename[l-2] == 'M' && filename[l-1] == 'P')) 
	{

		data = (unsigned char *) grf_get (grfFile, filename, &size, error);
		if (!data) {
			return FALSE;
		}

		w = getlong(&data[18]);
		h = getlong(&data[22]);
		
		bpp = getshort(&data[28]);

		if (bpp < 16)
		{
			numcol = 1<<bpp;
			rgb lut[256];
			int dataptr; 

			memcpy(&lut[0][0],&data[54],sizeof(rgb)*numcol);

			dataptr = 54 + sizeof(rgb) * numcol;

			ndata = (unsigned char *) malloc (sizeof(unsigned char)*4*h*w);
			for (j = 0; j < h; j++) 
			  for (k = 0; k < w; k++) {
			  unsigned char r, g, b;

			  r = (int) lut[data[dataptr + k + (h-1-j) * w]][0];
			  g = (int) lut[data[dataptr + k + (h-1-j) * w]][1];
			  b = (int) lut[data[dataptr + k + (h-1-j) * w]][2];

			  ndata[4*(j*w+k)] = b;
			  ndata[4*(j*w+k)+1] = g;
			  ndata[4*(j*w+k)+2] = r;
			  //  if (g==0 && r>253 && b>253)
			  //	ndata[4*(j*w+k)+3] = 0;
			  //  else
			  ndata[4*(j*w+k)+3] = alpha;
			}
		}

		if (bpp == 24)
		{
			unsigned long dataoffset;
			dataoffset = getlong(&data[10]);
			cdata = (unsigned char *) &data[dataoffset];
			ndata = (unsigned char *) malloc (sizeof(unsigned char)*4*h*w);

			for (j = 0; j < h; j++) 
			  for (k = 0; k < w; k++) {
			  unsigned char r, g, b;

			  r = coord3(cdata, k, h-1-j, 0);
			  g = coord3(cdata, k, h-1-j, 1);
			  b = coord3(cdata, k, h-1-j, 2);

			  ndata[4*(j*w+k)] = b;
			  ndata[4*(j*w+k)+1] = g;
			  ndata[4*(j*w+k)+2] = r;
			  //if (g==0 && r>253 && b>253)
				//ndata[4*(j*w+k)+3] = 0;
			  //else
			  ndata[4*(j*w+k)+3] = alpha;
			}
		}

		glBindTexture(GL_TEXTURE_2D, texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if(alpha)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
		else 
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		}

		gluBuild2DMipmaps ( GL_TEXTURE_2D, GL_RGBA, w, h, GL_RGBA, GL_UNSIGNED_BYTE, ndata );

		free(ndata);

	} else {
		MessageBox (NULL, "Unknown texture type", "Error", 0);
		return false;
	}

	return true;
}

bool LoadTextureAlpha(char *ragnapath, char *filename, GLuint texture, unsigned char alpha)
{
	int h, w, j, k;
	unsigned char *cdata;
	unsigned char *ndata;
	char buffer[512];
	int l;
	l = strlen(filename);

	sprintf (buffer, "%s%s", ragnapath, filename);

	if ((filename[l-3] == 'b' && filename[l-2] == 'm' && filename[l-1] == 'p') 
	 || (filename[l-3] == 'B' && filename[l-2] == 'M' && filename[l-1] == 'P')) 
	{

		AUX_RGBImageRec *image;
		image = LoadBMP(buffer);
		if(!image) image = LoadBMP("blank.bmp");
		h = image->sizeY;
		w = image->sizeX;

		cdata = (unsigned char *) image->data;
		ndata = (unsigned char *) malloc (sizeof(unsigned char)*4*h*w);

		for (j = 0; j < h; j++) 
			for (k = 0; k < w; k++) {
				unsigned char r, g, b;

				b = coord3(cdata, k, h-1-j, 0);
				g = coord3(cdata, k, h-1-j, 1);
				r = coord3(cdata, k, h-1-j, 2);

				ndata[4*(j*w+k)] = b;
				ndata[4*(j*w+k)+1] = g;
				ndata[4*(j*w+k)+2] = r;
				ndata[4*(j*w+k)+3] = alpha;
			}

		glBindTexture(GL_TEXTURE_2D, texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//if(alpha)
	//{
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//}
	//else 
	//{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	//}

		gluBuild2DMipmaps ( GL_TEXTURE_2D, GL_RGBA, w, h, GL_RGBA, GL_UNSIGNED_BYTE, ndata );

		free(ndata);

	} else {
		MessageBox (NULL, "Unknown texture type", "Error", 0);
		return false;
	}


	return true;
}

void goOrtho()
{
	glMatrixMode(GL_PROJECTION);						
	glPushMatrix();									
	glLoadIdentity();								
	glOrtho( 0, 800, 600, 0, 0, 1000 );	
	glMatrixMode(GL_MODELVIEW);								
}

void goPerspective()
{
	glMatrixMode( GL_PROJECTION );							
	glPopMatrix();											
	glMatrixMode( GL_MODELVIEW );	
}

void MatrixMultVect(const GLfloat *M, const GLfloat *Vin, GLfloat *Vout) 
{
	Vout[0] = Vin[0]*M[0] + Vin[1]*M[4] + Vin[2]*M[8] + 1.0*M[12];
	Vout[1] = Vin[0]*M[1] + Vin[1]*M[5] + Vin[2]*M[9] + 1.0*M[13];
	Vout[2] = Vin[0]*M[2] + Vin[1]*M[6] + Vin[2]*M[10] + 1.0*M[14];
}


CVector3 MatrixMultVect3f(const GLfloat *M, float x, float y, float z) 
{
	CVector3 tempvect;
	tempvect.x = x * M[0] + y * M[4] + z * M[8]  + 1.0 * M[12];
	tempvect.y = x * M[1] + y * M[5] + z * M[9]  + 1.0 * M[13];
	tempvect.z = x * M[2] + y * M[6] + z * M[10] + 1.0 * M[14];
	return tempvect;
}