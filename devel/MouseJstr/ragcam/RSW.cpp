// RSW.cpp: implementation of the RSW class.
//
//////////////////////////////////////////////////////////////////////

#include "RSW.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

float w_FrameInterval = 0.0f;

RSW::RSW()
{
	selected = -1;
	frustumculling = TRUE;
}

RSW::~RSW()
{

}

void SETTITLE(HWND window, char *format, ...) {
    va_list vl;
	va_start(vl, format);
	char title[512];

	if (window) {
			vsprintf(title, format, vl);
			SetWindowText(window, title);
	}
	va_end(vl);
}

bool RSW::GRFLoad(Grf *grffile, char *filename, HWND window, bool extract)
{
	long pos;
	ro_string_t *filepath;
	char buffer[512];
	int i, offset;
	unsigned long size;
	unsigned char* RSWData;

	char mapfile[128];

	memcpy(&mapname[0], &filename[5], strlen(filename) - 9);
	strcpy(&mapname[strlen(filename) - 9],"\0");
	sprintf(mapfile,"data\\texture\\유저인터페이스\\map\\%s.bmp",mapname);

	glGenTextures(1, &maptex);

	//mapbmp = LoadGRFBitmapAlpha (grffile, mapfile, 192);
	LoadGRFTextureAlpha (grffile, mapfile, maptex, 192);
	
	//fseek (fp, 46, SEEK_CUR);
	offset = 0;
	RSWData = (unsigned char *) grf_get(grffile, filename, &size, NULL);

	if(!RSWData) return FALSE;

	if (extract)
	{
		char targfile[512];
		sprintf(targfile,"C:\\Program Files\\RagnarokOffline3\\%s",filename);
		grf_extract (grffile, filename, targfile, NULL);
	}

	memcpy(&header, &RSWData[offset], 46);
	offset += 46;

	memcpy(&gndfile[0], &RSWData[offset], sizeof(ro_string_t));
	offset += sizeof(ro_string_t);

	memcpy(&gatfile[0], &RSWData[offset], sizeof(ro_string_t));
	offset += sizeof(ro_string_t);

	//fread(gndfile, sizeof(ro_string_t), 1, fp);
	//fread(gatfile, sizeof(ro_string_t), 1, fp);

	sprintf (buffer, "data\\%s", gndfile);

	SETTITLE(window, "RagCam : Loading Ground");
	ground.GRFLoad(grffile, buffer, extract);

	sprintf (buffer, "data\\%s", gatfile);

	SETTITLE(window, "RagCam : Loading Ground");
	ground.GRFLoadGat(grffile, buffer, extract);


	memcpy(&unknown[0], &RSWData[offset], 176);
	offset += 176;

	pos = offset;

	nmodels = 0;
	while (1) {
		ro_string_t fin;

		memcpy(&fin, &RSWData[offset], sizeof(ro_string_t));
		offset += sizeof(ro_string_t);
		if (!strlen(fin))
			break;
		else {
			offset += 212;
			nmodels++;
		}
	}

	offset = pos;
	models = new ro_model_t[nmodels];
	filepath = new ro_string_t[nmodels];

	FILE *fout;
	fout = fopen("sortie.txt", "w");
	for (i = 0; i < nmodels; i++) {
		memcpy(filepath[i], &RSWData[offset], sizeof(ro_string_t));
		memcpy(&models[i].filepath,filepath[i], sizeof(ro_position_t));
		offset += sizeof(ro_string_t);

		memcpy(&models[i].unknown1,  &RSWData[offset], 120);
		offset += 120;

		memcpy(&models[i].position, &RSWData[offset], sizeof(ro_position_t));
		offset += sizeof(ro_position_t);
		memcpy(&models[i].unknown2,  &RSWData[offset], 56);
		offset += 56;
	}

	extrasize = size - offset;
	extra = (char *) malloc (extrasize);
	
	memcpy(extra, &RSWData[offset], extrasize);


	nrealmodels = 0;

	int *breal;
	breal = new int[nmodels];

	for (i = 0; i < nmodels; i++) {
		breal[i] = true;
		int j;
		
		for (j = 0; j < i; j++) {
			if (!strcmp(filepath[j] , filepath[i])) {
				breal[i] = false;
				break;
			}
		}

		if (breal[i]) {
			nrealmodels++;
		}
	}

	ro_string_t *realmodelspath;
	realmodelspath = new ro_string_t[nrealmodels];
	realmodels = new RSM[nrealmodels];

	int k = 0;
	for (i = 0; i < nmodels; i++)
		if (breal[i]) {
			strcpy(realmodelspath[k], filepath[i]);
			k++;
		}

	bexist = new bool[nrealmodels];

	for (i = 0; i < nrealmodels; i++) {
		SETTITLE(window, "RagCam : Loading Models (%d\\%d) : %s", i+1, nrealmodels, realmodelspath[i]);
		sprintf (buffer, "data\\model\\%s", realmodelspath[i]);
		if (!realmodels[i].GRFLoad(grffile, buffer, extract))
			bexist[i] = false;
		else
			bexist[i] = true;
		fprintf (fout, "%d %d %s\n", bexist[i], i+1, realmodelspath[i]); 
	}

	for (i = 0; i < nmodels; i++) {
		int num = 0;
	

		models[i].model = 1;
		for (num = 0; num < nrealmodels; num++) {
			if (!strcmp(filepath[i], realmodelspath[num])) {
				models[i].model = num;
				break;

			}
		}
	}


	fclose(fout);

	SETTITLE(window, "RagCam : Displaying %s", mapname);
	return true;
}

bool RSW::Save(char *ragnapath)
{
	FILE *fp;
	FILE *fex;

	long pos;
	ro_string_t *filepath;
	char mapfile[512];
	char buffer[512];
	int i;
	//char title[512];

	sprintf (buffer, "%sdata\\%s-new.rsw", ragnapath, mapname);
	fp = fopen (buffer, "wb");

	if (!fp) 
	{
		return false;
	}

	fwrite(header, 46, 1, fp);
	fwrite(gndfile, sizeof(ro_string_t), 1, fp);
	fwrite(gatfile, sizeof(ro_string_t), 1, fp);

	fwrite(unknown, 176, 1, fp);
	//fseek (fp, 176, SEEK_CUR);

	for (i = 0; i < nmodels; i++) {
		fwrite(&models[i].filepath, sizeof(ro_string_t), 1, fp);
		fwrite(&models[i].unknown1, 120, 1, fp);
		//fseek (fp, 120, SEEK_CUR);
		fwrite(&models[i].position, sizeof(ro_position_t), 1, fp);
		fwrite(&models[i].unknown2, 56, 1, fp);
		//fseek (fp, 56, SEEK_CUR);
	}

	fwrite(extra, extrasize, 1, fp);

	fclose(fp);

	sprintf(buffer,"Current RSW has been saved to \"data\\%s-new.rsw\"", mapname);
	MessageBox(NULL, buffer, "Notice", MB_OK + MB_ICONINFORMATION);

	sprintf (buffer, "%sdata\\%s-extra.bin", ragnapath, mapname);
	fp = fopen (buffer, "wb");
	if (!fp) 
	{
		return false;
	}
	fwrite(extra, extrasize, 1, fp);
	fclose(fp);

	return true;
}


bool RSW::Load(char *ragnapath, char *filename, HWND window)
{
	FILE *fp;
	long pos;
	ro_string_t *filepath;
	char mapfile[512];
	char buffer[512];
	int i;
	//char title[512];

	sprintf (buffer, "%s%s", ragnapath, filename);

	fp = fopen (buffer, "rb");

	if (!fp) 
	{
		return false;
	}

	memcpy(&mapname[0], &filename[5], strlen(filename) - 9);
	strcpy(&mapname[strlen(filename) - 9],"\0");
	sprintf(mapfile,"data\\texture\\유저인터페이스\\map\\%s.bmp",mapname);

	glGenTextures(1, &maptex);

	//mapbmp = LoadGRFBitmapAlpha (grffile, mapfile, 192);
	LoadTextureAlpha (ragnapath, mapfile, maptex, 192);

	//fseek (fp, 46, SEEK_CUR);

	fread(header, 46, 1, fp);
	fread(gndfile, sizeof(ro_string_t), 1, fp);
	fread(gatfile, sizeof(ro_string_t), 1, fp);

	sprintf (buffer, "%sdata\\%s", ragnapath, gndfile);

	SETTITLE(window, "RagCam : Loading Ground");
	ground.Load(ragnapath, buffer);

	fseek (fp, 176, SEEK_CUR);

	pos = ftell(fp);

	nmodels = 0;
	while (1) {
		ro_string_t fin;

		fread (&fin, sizeof(ro_string_t), 1, fp);
		if (!strlen(fin))
			break;
		else {
			fseek (fp, 212, SEEK_CUR);
			nmodels++;
		}
	}

	fseek (fp, pos, SEEK_SET);

	models = new ro_model_t[nmodels];
	filepath = new ro_string_t[nmodels];

	FILE *fout;
	fout = fopen("sortie.txt", "w");
	for (i = 0; i < nmodels; i++) {
		fread(filepath[i], sizeof(ro_string_t), 1, fp);

		fseek (fp, 120, SEEK_CUR);

		fread(&models[i].position, sizeof(ro_position_t), 1, fp);

		fseek (fp, 56, SEEK_CUR);
	}

	nrealmodels = 0;

	int *breal;
	breal = new int[nmodels];

	for (i = 0; i < nmodels; i++) {
		breal[i] = true;
		int j;
		
		for (j = 0; j < i; j++) {
			if (!strcmp(filepath[j] , filepath[i])) {
				breal[i] = false;
				break;
			}
		}

		if (breal[i]) {
			nrealmodels++;
		}
	}

	ro_string_t *realmodelspath;
	realmodelspath = new ro_string_t[nrealmodels];
	realmodels = new RSM[nrealmodels];

	int k = 0;
	for (i = 0; i < nmodels; i++)
		if (breal[i]) {
			strcpy(realmodelspath[k], filepath[i]);
			k++;
		}

	bexist = new bool[nrealmodels];

	for (i = 0; i < nrealmodels; i++) {
		SETTITLE(window, "RagCam : Loading Models (%d\\%d) : %s", i+1, nrealmodels, realmodelspath[i]);
		sprintf (buffer, "%sdata\\model\\%s", ragnapath, realmodelspath[i]);
		if (!realmodels[i].Load(ragnapath, buffer))
			bexist[i] = false;
		else
			bexist[i] = true;
		fprintf (fout, "%d %d %s\n", bexist[i], i+1, realmodelspath[i]); 
	}


	for (i = 0; i < nmodels; i++) {
		int num = 0;
	

		models[i].model = 1;
		for (num = 0; num < nrealmodels; num++) {
			if (!strcmp(filepath[i], realmodelspath[num])) {
				models[i].model = num;
				break;

			}
		}
	}


	fclose(fout);
	fclose(fp);


	SETTITLE(window, "RagCam : Displaying", i, nrealmodels);
	return true;
}

void RSW::PickObject(CFrustum g_Frustum)
{
	GLuint	selectBuffer[512];							// Set Up A Selection Buffer
	GLint   viewport[4];

	glPushMatrix();								// Push The Projection Matrix

	glGetIntegerv(GL_VIEWPORT, viewport);
	glSelectBuffer(512, selectBuffer);	
	(void) glRenderMode(GL_SELECT);
	glInitNames();								// Initializes The Name Stack
	glMatrixMode(GL_PROJECTION);						// Selects The Projection Matrix

	glPushMatrix();								// Push The Projection Matrix
	glLoadIdentity();							// Resets The Matrix

	POINT mousePos;			// This is a window structure that holds an X and Y
	GetCursorPos(&mousePos);	

	// This Creates A Matrix That Will Zoom Up To A Small Portion Of The Screen, Where The Mouse Is.
	gluPickMatrix((GLdouble) mousePos.x, (GLdouble) (viewport[3]-mousePos.y ), 1.0f, 1.0f, viewport);
	gluPerspective(45.0f, (GLfloat) (viewport[2]-viewport[0])/(GLfloat) (viewport[3]-viewport[1]), 10.0f, 1000.0f);
	glMatrixMode(GL_MODELVIEW);						// Select The Modelview Matrix

	//g_Frustum.CalculateFrustum();
	for (int i = 0; i < nmodels; i++) {
		if (bexist[models[i].model])
		{
			glPushName(i);
			//if (g_Frustum.BoxInFrustum(models[i].position.x,models[i].position.y,models[i].position.z,realmodels[models[i].model].box.range[0] * models[i].position.sx,realmodels[models[i].model].box.range[1] * models[i].position.sy,realmodels[models[i].model].box.range[2] * models[i].position.sz))
			realmodels[models[i].model].Display(models[i].position);
			glPopName();	
		}

	}

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glPopMatrix();								// Pop The Projection Matrix
	glMatrixMode(GL_MODELVIEW);						// Select The Modelview Matrix

	glPopMatrix();								// Pop The Projection Matrix

	hits = glRenderMode(GL_RENDER);	

	unsigned int lowestDepth = selectBuffer[1];
	int selectedObject = selectBuffer[3];

	if (hits> 0)
	{		
		for(int i = 1; i < hits; i++)
		{
			if(selectBuffer[(i * 4) + 1] < lowestDepth)
			{
				// Set the current lowest depth
				lowestDepth = selectBuffer[(i * 4) + 1];
				// Set the current object ID
				selectedObject = selectBuffer[(i * 4) + 3];
			}
		}
	} else {
		selectedObject = -1;
	}

	//selected = 0;
	selected = selectedObject;
}

void RSW::Display(float waterlevel, CFrustum g_Frustum)
{
	static float waterphase;

	glPushMatrix();

	waterframe+=w_FrameInterval * 15;
	if(waterframe>31) waterframe=0;

	static float framesPerSecond    = 0.0f;		// This will store our fps
	static float lastTime			= 0.0f;		// This will hold the time from the last frame
	static float frameTime = 0.0f;				// This stores the last frame's time

    float currentTime = timeGetTime() * 0.001f;				

 	w_FrameInterval = currentTime - frameTime;
	frameTime = currentTime;

    ++framesPerSecond;

    if( currentTime - lastTime > 1.0f )
    {
	    lastTime = currentTime;
        framesPerSecond = 0;
    }

	waterphase += w_FrameInterval * 60 ;
	if(waterframe>360) waterframe=0;

    ground.Display(g_Frustum);

	if (selectobject) 
	{
		PickObject(g_Frustum);
		selectobject = false;
	}

	modelsdrawn = 0;

	if (frustumculling)	g_Frustum.CalculateFrustum();
	for (int i = 0; i < nmodels; i++) {
		glPushMatrix();
		if (bexist[models[i].model])
		{
			if (i==selected){
				glColor3f(1.0f,0.0f,0.0f);
				realmodels[models[i].model].setdrawbounds(true);
			} else {
				glColor3f(1.0f,1.0f,1.0f);
				realmodels[models[i].model].setdrawbounds(false);
			}

			if (frustumculling)
			{
				if (g_Frustum.BoxInFrustum(models[i].position.x,models[i].position.y,models[i].position.z,realmodels[models[i].model].box.range[0] * models[i].position.sx,realmodels[models[i].model].box.range[1] * models[i].position.sy,realmodels[models[i].model].box.range[2] * models[i].position.sz))
				{
					modelsdrawn++;
					realmodels[models[i].model].Display(models[i].position);
				}
			} else {
				{
					modelsdrawn++;
					realmodels[models[i].model].Display(models[i].position);
				}
			}

			
			glColor3f(1.0f,1.0f,1.0f);
		}

		glPopMatrix();

	}
	
	ground.DisplayWater(waterframe, waterphase * 3.141592654 / 180, waterlevel, g_Frustum);

	glPopMatrix();
}

ro_model_t *RSW::SelectedModel()
{
	if(selected>-1)
	{
		return &models[selected];
	} else {
		return &models[0];
	}
}

