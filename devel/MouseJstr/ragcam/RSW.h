// RSW.h: interface for the RSW class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RSW_H__42D932D9_0C27_4AA9_98E1_9C50045FE9CD__INCLUDED_)
#define AFX_RSW_H__42D932D9_0C27_4AA9_98E1_9C50045FE9CD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ro-common.h"
#include "Rsm.h"
#include "GND.h"


typedef struct {
	int model;
	ro_string_t filepath;
	char unknown1[120];
	char unknown2[56];
	ro_position_t position;
} ro_model_t;

class RSW  
{
public:
	RSW();
	virtual ~RSW();

	GLuint maptex;
	GLint  hits;	
	char mapname[32];	
	bool GRFLoad (Grf *grffile, char *filename, HWND window=NULL, bool extract = FALSE);
	bool Load (char *ragnapath, char *filename, HWND window=NULL);
	bool Save(char *ragnapath);
	void Display(float waterlevel, CFrustum g_Frustum);
	void PickObject(CFrustum g_Frustum);
	ro_model_t *SelectedModel();
	RSM *getRealModel(int n) { return &realmodels[n]; };
	
	bool selectobject;

	int getWidth() { return(ground.sizeX); }
	int getHeight() { return(ground.sizeY); }
	int nummodels() { return nmodels; };
	int numrealmodels() { return nrealmodels; };

	char header[46];
	ro_string_t gndfile;
	ro_string_t gatfile;
	char unknown[176];
	char *extra;
	unsigned long extrasize;
	int selected;

	int modelsdrawn;

	bool frustumculling;

private:
	GLuint glGND;
	int nmodels;
	ro_model_t *models;
	float waterframe;
	int waterframeinterval;
	int nrealmodels;
	RSM *realmodels;
	GND ground;
	bool *bexist;

};

#endif // !defined(AFX_RSW_H__42D932D9_0C27_4AA9_98E1_9C50045FE9CD__INCLUDED_)
