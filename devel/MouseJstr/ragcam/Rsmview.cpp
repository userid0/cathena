/*
 *		This Code Was Created By Jeff Molofee 2000
 *		A HUGE Thanks To Fredric Echols For Cleaning Up
 *		And Optimizing This Code, Making It More Flexible!
 *		If You've Found This Code Useful, Please Let Me Know.
 *		Visit My Site At nehe.gamedev.net
 */

#include <windows.h>		// Header File For Windows
#include <math.h>		// Header File For Windows
#include <gl\gl.h>			// Header File For The OpenGL32 Library
#include <gl\glu.h>			// Header File For The GLu32 Library
#include <gl\glaux.h>		// Header File For The Glaux Library
#include <gl\glut.h>
#include <direct.h>

#include "RSW.h"
#include "GND.h"
#include "Rsm.h"
#include "Camera.h"
#include "ro-common.h"
#include "ragcam.h"
#include "ragcontrols.h"
#include "ragwindows.h"

HDC			hDC=NULL;		// Private GDI Device Context
HGLRC		hRC=NULL;		// Permanent Rendering Context
HWND		hWnd=NULL;		// Holds Our Window Handle
HINSTANCE	hInstance;		// Holds The Instance Of The Application

bool	keys[256];			// Array Used For The Keyboard Routine
bool	active=TRUE;		// Window Active Flag Set To TRUE By Default
bool	fullscreen=FALSE;	// Fullscreen Flag Set To Fullscreen Mode By Default

char ragnapath[1024];
int nfiles = 0;
typedef char filepath[1024];
filepath files[50];
RSM	*rsm;
GND gnd;
RSW rsw;

GLuint bgitex;

bool bshowmodel;

CRagButton *ragBtnOK1;
CRagButton *ragBtnClose1;
CRagListBox *ragListBox1;

CRagWindow *ragPopMsg;
CRagWindow *ragPopMsg2;
CRagWindowList ragWindowList;

POINT windowPos;

bool brsw = false;
bool bgnd = false;

GLfloat angle = 0.0;
GLfloat angle2 = 0.0;
GLfloat userX = 0.0;
GLfloat userY = 0.0;
GLfloat userZ = 0.0;

void SETTITLE(HWND window, char *title) {
	if (window) {
		SetWindowText(window, title);
	}
}

LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// Declaration For WndProc

GLvoid ReSizeGLScene(GLsizei w, GLsizei h)		// Resize And Initialize The GL Window
{
	if (h==0)										// Prevent A Divide By Zero By
	{
		h=1;										// Making Height Equal One
	}

	glViewport (0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
  
	gluPerspective (40.0, (GLfloat)w/(GLfloat)h, 10.0, 1000.0);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();
}

void ShowSelection()
{
	char buffer[40];
	sprintf(buffer,"%s",ragListBox1->GetItem(ragListBox1->index));
	MessageBox(hWnd,buffer,"You Selected",MB_OK);
}


void PopMeUp()
{
	bshowmodel = TRUE;
	//ragPopMsg->setVisible(FALSE);
}

void CloseMe()
{
	ragListBox1->GetItem(ragListBox1->index);
	ragPopMsg->setVisible(FALSE);
}

int InitGL(GLvoid)										// All Setup For OpenGL Goes Here
{
	glClearColor (1.0, 1.0, 1.0, 0.0);

	bshowmodel = FALSE;

	glGenTextures(1, &bgitex);
	LoadGRFTexture(myGRF,"유저인터페이스\\bgi_temp.bmp",bgitex,NULL,FALSE);

	ragBtnOK1 = new CRagButton(hWnd, myGRF, RB_OK, 186,95, PopMeUp);
	ragBtnClose1 = new CRagButton(hWnd, myGRF, RB_CLOSE, 233,95, CloseMe);
	ragPopMsg = new CRagWindow(hWnd, myGRF, "RagCam by Ender (C)2005",400-140,300-60,0,0,0);
	
	ragListBox1 = new CRagListBox(hWnd, myGRF, 8, 30, 250, 50, ShowSelection);

	for (unsigned int i = 0; i < myGRF->nfiles; i++) {
		if(strstr(myGRF->files[i].name,".rsw")) ragListBox1->Add(myGRF->files[i].name) ;
	}


	ragListBox1->Add("payon");
	ragListBox1->Add("morroc");
	ragListBox1->Add("izlude");
	ragListBox1->Add("prontera");

	ragPopMsg->AddControl(ragBtnOK1);
	ragPopMsg->AddControl(ragBtnClose1);
	ragPopMsg->AddControl(ragListBox1);

	ragPopMsg2 = new CRagWindow(hWnd, myGRF, "This is a test!",100,100,0,0,0);
	ragWindowList.AddWindow (ragPopMsg);
	ragWindowList.AddWindow (ragPopMsg2);
	ragPopMsg2->setVisible(FALSE);

	if ((files[0][strlen(files[0])-3] == 'g' &&
		files[0][strlen(files[0])-2] == 'n' &&
		files[0][strlen(files[0])-1] == 'd') ||
		(files[0][strlen(files[0])-3] == 'G' &&
		files[0][strlen(files[0])-2] == 'N' &&
		files[0][strlen(files[0])-1] == 'D')) {
		bgnd = true;
#ifdef _USEGRF 
		gnd.GRFLoad(myGRF, files[0], bextract);
#else
		gnd.Load(ragnapath, files[0]);
#endif
	} else if ((files[0][strlen(files[0])-3] == 'r' &&
		files[0][strlen(files[0])-2] == 's' &&
		files[0][strlen(files[0])-1] == 'w') ||
		(files[0][strlen(files[0])-3] == 'R' &&
		files[0][strlen(files[0])-2] == 'S' &&
		files[0][strlen(files[0])-1] == 'W')) {
		brsw = true;
#ifdef _USEGRF 		
		rsw.GRFLoad(myGRF, files[0], hWnd, bextract);
#else
		rsw.Load(ragnapath, files[0], hWnd);
#endif
		mapw = rsw.getWidth();
		maph = rsw.getHeight();

	} else {
		rsm = new RSM[nfiles];
		char buf[1024];
	//	for (int i = 0; i < nfiles; i++) {
			int i=1;
			while (files[0][i] != '\0') {
				buf[i-1] = files[0][i-1];
				i++;
			}
			buf[i-1] = '\0';
			rsm[0].GRFLoad(myGRF, files[0]);
			//rsm[0].Load(ragnapath, files[0]);
	//	}
	}

	glEnable(GL_TEXTURE_2D);
	glClearDepth(1.0f);

	float fogColor[4] = {0.95f,0.95f,1.0f,1.0f};			// Let's make the Fog Color BLUE

	glFogi(GL_FOG_MODE, GL_EXP2);						// Fog Mode
	glFogfv(GL_FOG_COLOR, fogColor);					// Set Fog Color
	glFogf(GL_FOG_DENSITY, 0.05f);				// How Dense Will The Fog Be
	glHint(GL_FOG_HINT, GL_DONT_CARE);					// The Fog's calculation accuracy
	glFogf(GL_FOG_START, 1000.0f);							// Fog Start Depth
	glFogf(GL_FOG_END, 1000.0f);							// Fog End Depth
	// To turn fog off, you can call glDisable(GL_FOG);
 
	//glEnable(GL_NORMALIZE);
	
#ifdef LIGHT_ENABLE
	float ambience[4] = {0.3f, 0.3f, 0.3f, 1.0};		// The color of the light in the world
	float diffuse[4] = {1.0f, 1.0f, 1.0f, 1.0};			// The color of the positioned light
	glLightfv( GL_LIGHT0, GL_AMBIENT,  ambience );		// Set our ambience values (Default color without direct light)
	glLightfv( GL_LIGHT0, GL_DIFFUSE,  diffuse );		// Set our diffuse color (The light color)
	glLightfv( GL_LIGHT0, GL_POSITION, g_LightPosition );	// This Sets our light position

    glEnable(  GL_LIGHT0   );								// Turn this light on
	glEnable(  GL_LIGHTING );								// This turns on lighting
	glEnable(GL_COLOR_MATERIAL);		
#endif

	//glShadeModel (GL_FLAT);											// Select Flat Shading (Nice Definition Of Objects)
	glShadeModel (GL_SMOOTH);

	//glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_LEQUAL);

	//glBlendFunc(GL_SRC_ALPHA,GL_ONE);		// Blending Function For Translucency Based On Source Alpha Value ( NEW )

	glDepthFunc(GL_LEQUAL);							// Type Of Depth Testing
	glEnable(GL_DEPTH_TEST);						// Enable Depth Testing

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);			// Enable Alpha Blending (disable alpha testing)
	glEnable(GL_BLEND);							// Enable Blending       (disable alpha testing)

	//glAlphaFunc(GL_GREATER,0.1f);						// Set Alpha Testing     (disable blending)
	//glEnable(GL_ALPHA_TEST);						// Enable Alpha Testing  (disable blending)

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable(GL_FOG);			

	BuildFont(hDC);

    //glFrontFace(GL_CW);
    //glEnable(GL_CULL_FACE);
	
	//minipos.x = 30.0;
	//minipos.y = 0.0;
	//minipos.z = -150.0;

	minipos.x = 0.0;
	minipos.y = 0.0;
	minipos.z = 0.0;

	minipos.rx = 0.0;
	minipos.ry = 0.0;
	minipos.rz = 0.0;

	minipos.sx =  5.0;
	minipos.sy = -5.0;
	minipos.sz =  5.0;

	
	//while(ShowCursor(FALSE) >= 0);

	//LoadGRFTexture(myGRF,"data\\",cursortex,NULL);

	return TRUE;										// Initialization Went OK
}


int DrawGLScene(GLvoid)	
{
	CVector3 campos;
	CVector3 camview;

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glPushMatrix();
	goOrtho();

	glDisable (GL_LIGHTING); 
	glEnable (GL_TEXTURE_2D); 

	glTranslatef(0.0f,0.0f,0.0f);

	/*
	glBindTexture(GL_TEXTURE_2D, bgitex);
	glColor4f(1.0f,1.0f,1.0f,1.0f);
	glBegin(GL_QUADS);				// Begin Drawing The Textured Quad
		glTexCoord2f(0.0f, 0.0f); glVertex3f(   0.0f,   0.0f, 0.0f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f( 800.0f,   0.0f, 0.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f( 800.0f, 600.0f, 0.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(   0.0f, 600.0f, 0.0f);
	glEnd();
	*/

	glEnable(GL_LIGHTING); 

	goPerspective();

	glPopMatrix();
	
	g_Camera.Look();

	glPushMatrix();

	//Draw where our light is SUPPOSED to be...
	glBegin(GL_LINES);
		glVertex3f(g_LightPosition[0]+1,g_LightPosition[1],g_LightPosition[2]);
		glVertex3f(g_LightPosition[0]-1,g_LightPosition[1],g_LightPosition[2]);

		glVertex3f(g_LightPosition[0],g_LightPosition[1]+1,g_LightPosition[2]); 
		glVertex3f(g_LightPosition[0],g_LightPosition[1]-1,g_LightPosition[2]);

		glVertex3f(g_LightPosition[0],g_LightPosition[1],g_LightPosition[2]+1);
		glVertex3f(g_LightPosition[0],g_LightPosition[1],g_LightPosition[2]-1);
    glEnd();

	glLightfv( GL_LIGHT0, GL_POSITION, g_LightPosition );	// This Sets our light position

	glPopMatrix();

//	if (bgnd) {
//		glRotatef (-180.0, 1.0, 0.0, 0.0);
//	}

	if (bgnd || brsw) {
		glTranslatef (10.0*userX, 10.0*userY-50, 10.0*userZ);

//		glRotatef (angle2+45.0, 1.0, 0.0, 0.0);
//		glRotatef (angle, 0.0, 1.0, 0.0);
	} else {
		glTranslatef (userX, userY, userZ-100.0);

		glRotatef (angle2, 1.0, 0.0, 0.0);
		glRotatef (angle, 0.0, 1.0, 0.0);

//	glRotatef(338.0, 0.0, 0.0, 1.0);
//	glRotatef(90.0, 0.0, 1.0, 0.0);
//	glRotatef(62.0, 1.0, 0.0, 0.0);
	}

	//update the fog?
	glFogf(GL_FOG_DENSITY, fogd);				// How Dense Will The Fog Be

	glScalef (1.0, 1.0, -1.0);

	if (bgnd) {
		gnd.Display(g_Frustum);
	} else if (brsw) {
		//if (bshowmodel) 
			rsw.Display(waterlevel, g_Frustum);

		glPushMatrix();

		campos = g_Camera.Position();
		camview = g_Camera.View();

		glDisable(GL_LIGHTING);

		//Draw the on-screen map
		if((showinfo & 1)==1){
			goOrtho();

			glLoadIdentity();	

			glTranslatef(800-255-10,10,0);

			glEnable (GL_TEXTURE_2D); 

			glBindTexture(GL_TEXTURE_2D, rsw.maptex);
			glBegin(GL_QUADS);				// Begin Drawing The Textured Quad
				glTexCoord2f(0.0f, 0.0f); glVertex3f(   0.0f,   0.0f, 0.0f);
				glTexCoord2f(1.0f, 0.0f); glVertex3f( 255.0f,   0.0f, 0.0f);
				glTexCoord2f(1.0f, 1.0f); glVertex3f( 255.0f, 255.0f, 0.0f);
				glTexCoord2f(0.0f, 1.0f); glVertex3f(   0.0f, 255.0f, 0.0f);
			glEnd();
			
			mapx = (campos.x / 10.0f + mapw/2) / mapw * 255;
			mapy = (campos.z / 10.0f + maph/2) / maph * 255;
			rota = - (float) atan((camview.x - campos.x) / (camview.z - campos.z)) * 180 / 3.141592654;
			if (camview.z > campos.z) rota = rota + 180;

			glLoadIdentity();	
			glTranslatef(800-255-10,10,0);
			glTranslatef(mapx,mapy,0.f);
			glRotatef(rota,0.0f,0.0f,1.0f);

			glColor4f(1.0f,0.0f,0.0f,0.5f);
			glBegin(GL_TRIANGLES);				// Begin Drawing The Textured Quad
				glVertex3f(  0.0f, -10.0f, 0.0f);
				glVertex3f(  5.0f,   5.0f, 0.0f);
				glVertex3f( -5.0f,   5.0f, 0.0f);
			glEnd();

			glColor4f(1.0f,1.0f,1.0f,1.0f);

			glEnable (GL_TEXTURE_2D); 
		
			goPerspective();
		}


		//Draw the trasparent info window
		if((showinfo & 2)==2){
			goOrtho();

			glLoadIdentity();	
		
			glDisable (GL_TEXTURE_2D); 

			glColor4f(0.0f,0.0f,0.0f,0.5f);
			glBegin(GL_QUADS);
				glVertex3f(   0.0f,   0.0f, 0.0f);
				glVertex3f( 320.0f,   0.0f, 0.0f);
				glVertex3f( 320.0f, 700.0f, 0.0f);
				glVertex3f(   0.0f, 700.0f, 0.0f);
			glEnd();

			glColor4f(1.0f,1.0f,1.0f,1.0f);

			int j = 2;

			glPrint(20,j++ * 12,"Map Name: %s", rsw.mapname); j++;

			glPrint(25,j++ * 12,"Map Size:      %d x %d", mapw, maph);  
			glPrint(25,j++ * 12,"Total Objects: %d", rsw.nummodels());
			glPrint(25,j++ * 12,"Total Models:  %d", rsw.numrealmodels()); 
			j++;

			glPrint(25,j++ * 12,"Models Drawn:  %d", rsw.modelsdrawn); j++;

			glPrint(20,j++ * 12,"Object #%d", rsw.selected + 1); j++;

			glPrint(25,j++ * 12,"Uses Model #%d", rsw.SelectedModel()->model + 1); j++;

			if(editmode==0) glColor4f(1.0f,0.0f,0.0f,1.0f);
			glPrint(25, j * 12,"POS:  %.2f", rsw.SelectedModel()->position.x);
			glPrint(125,j * 12,"%.2f", rsw.SelectedModel()->position.y);
			glPrint(185,j++ * 12,"%.2f", rsw.SelectedModel()->position.z);
			glColor4f(1.0f,1.0f,1.0f,1.0f);

			if(editmode==1) glColor4f(1.0f,0.0f,0.0f,1.0f);
			glPrint(25, j * 12,"ROT:  %.2f", rsw.SelectedModel()->position.rx);
			glPrint(125,j * 12,"%.2f", rsw.SelectedModel()->position.ry);
			glPrint(185,j++ * 12,"%.2f", rsw.SelectedModel()->position.rz);
			glColor4f(1.0f,1.0f,1.0f,1.0f);

			if(editmode==2) glColor4f(1.0f,0.0f,0.0f,1.0f);
			glPrint(25, j * 12,"SCL:  %.2f", rsw.SelectedModel()->position.sx);
			glPrint(125,j * 12,"%.2f", rsw.SelectedModel()->position.sy);
			glPrint(185,j++ * 12,"%.2f", rsw.SelectedModel()->position.sz);
			glColor4f(1.0f,1.0f,1.0f,1.0f);

			selectedobject = rsw.getRealModel(rsw.SelectedModel()->model);
			
			j++;
			
			glPrint(25,j++ * 12, "Meshes:  %d", selectedobject->getNumMeshes());

			for(int i = 0; i < selectedobject->getNumMeshes(); i++)
			{
				selectedmodel = selectedobject->getMesh(i);

				if(i>0)
					glPrint(30,j++ * 12, "Mesh #%d %s->%s", i, selectedmodel->name ,selectedmodel->parent_name);
				else
					glPrint(30,j++ * 12, "Mesh #%d %s", i, selectedmodel->name );
				glPrint(35,j++ * 12, "Verts:  %d", selectedmodel->numverts());
				glPrint(35,j++ * 12, "TVerts: %d", selectedmodel->numtverts());
				glPrint(35,j++ * 12, "Faces:  %d", selectedmodel->numfaces());
				glPrint(35,j++ * 12, "Frames:  %d", selectedmodel->numframes());
				if (selectedmodel->numframes() > 0)
				{
					ro_frame_t tempframe;
					for(int k = 0; k < selectedmodel->numframes(); k++)
					{
						tempframe = selectedmodel->getframe(k);
						glPrint(40,j++ * 12, "Frame #%d    Time: %d", k+1, tempframe.time );
						glPrint(40,j++ * 12, "Orientation: %.2f %.2f %.2f %.2f", tempframe.orientation[0], tempframe.orientation[1], tempframe.orientation[2], tempframe.orientation[3]);
						j++;
					}
				}
				
				j++;
			}

			j = 2;

			glColor4f(0.0f,0.0f,0.0f,1.0f);

			switch(editmode)
			{
			case 0:	glPrint(360,j++ * 12,"Edit Translation"); break;
			case 1:	glPrint(360,j++ * 12,"Edit Rotation"); break;
			case 2:	glPrint(360,j++ * 12,"Edit Scaling"); break;
			}

			switch(rsw.frustumculling)
			{
			case TRUE:  glPrint(360,j++ * 12,"Culling is ENABLED"); break;
			case FALSE: glPrint(360,j++ * 12,"Culling is DISABLED"); break;
			}

			glColor4f(1.0f,1.0f,1.0f,1.0f);

			j = 2;

			switch(editmode)
			{
			case 0:	glPrint(358,j++ * 12 - 2,"Edit Translation"); break;
			case 1:	glPrint(358,j++ * 12 - 2,"Edit Rotation"); break;
			case 2:	glPrint(358,j++ * 12 - 2,"Edit Scaling"); break;
			}

			switch(rsw.frustumculling)
			{
			case TRUE:  glPrint(358,j++ * 12 - 2,"Culling is ENABLED"); break;
			case FALSE: glPrint(358,j++ * 12 - 2,"Culling is DISABLED"); break;
			}

			glEnable (GL_TEXTURE_2D); 

			goPerspective();


			/*
			goOrtho();

			glLoadIdentity();	

			glTranslatef(100,400,0);

			selectedobject->Display(minipos);

			goPerspective();
			*/
		}

		if(showhelp>0){
			goOrtho();

			glLoadIdentity();	
		
			glDisable (GL_TEXTURE_2D); 

			float offsetx, offsety;
			offsetx = 400-160; offsety = 320-200;

			glTranslatef(offsetx,offsety,0.0f);

			glColor4f(0.0f,0.0f,0.0f,0.5f);
			glBegin(GL_QUADS);
				glVertex3f(   0.0f,   0.0f, 0.0f);
				glVertex3f( 320.0f,   0.0f, 0.0f);
				glVertex3f( 320.0f, 400.0f, 0.0f);
				glVertex3f(   0.0f, 400.0f, 0.0f);
			glEnd();

			glColor4f(1.0f,1.0f,1.0f,1.0f);

			/*
			glPrint(20,8 * 12,"SCL:  %.2f", selectedmodel->transf.todo[19]);
			glPrint(120,8 * 12,"%.2f", selectedmodel->transf.todo[20]);
			glPrint(180,8 * 12,"%.2f", selectedmodel->transf.todo[21]);
				
			glPrint(20,9 * 12,"ROTA:  %.2f", selectedmodel->transf.todo[15]);
			glPrint(20,10 * 12,"ROT:  %.2f", selectedmodel->transf.todo[16]);
			glPrint(120,10 * 12,"%.2f", selectedmodel->transf.todo[17]);
			glPrint(180,10 * 12,"%.2f", selectedmodel->transf.todo[18]);
			*/


			//glPrint(offsetx + 20,offsety + j * 12,"CAM: %.2f", campos.x / 10.0f + mapw/2 );  
			//glPrint(offsetx + 120,offsety + j * 12,"%.2f", campos.z / 10.0f + maph/2);  
			//glPrint(offsetx + 180,offsety + j++ * 12,"%.2f", campos.y / 10.0f );  

			//glPrint(120,j++ * 12,"ROT: %.2f", rota);  j++

			int j = 2;
			glPrint(offsetx + 20,offsety + j++ * 12,"            - Help Window -"); j++;

			switch(showhelp)
			{
			case 1:
				glPrint(offsetx + 20,offsety + j++ * 12,"Functions"); 
				glPrint(offsetx + 20,offsety + j++ * 12,"========================================="); 
				glPrint(offsetx + 20,offsety + j++ * 12,"F1            Show/hide help"); 
				glPrint(offsetx + 20,offsety + j++ * 12,"F2            Show/hide fog"); 
				glPrint(offsetx + 20,offsety + j++ * 12,"F3            Reset camera position");  
				glPrint(offsetx + 20,offsety + j++ * 12,"F4            Save RSW to file");  j++;
				glPrint(offsetx + 20,offsety + j++ * 12,"F5            Toggle frustum culling");  j++; 
				glPrint(offsetx + 20,offsety + j++ * 12,"ESC           Exit");  
				glPrint(offsetx + 20,offsety + j++ * 12,"CTRL + Enter  Fullscreen on/off");  j++;
				glPrint(offsetx + 20,offsety + j++ * 12,"Space         Camera Movement on/off");  j++;

				glPrint(offsetx + 20,offsety + j++ * 12,"Movement"); 
				glPrint(offsetx + 20,offsety + j++ * 12,"========================================="); 
				glPrint(offsetx + 20,offsety + j++ * 12,"UP/W          Move camera forward"); 
				glPrint(offsetx + 20,offsety + j++ * 12,"DOWN/S        Move camera back"); 
				glPrint(offsetx + 20,offsety + j++ * 12,"LEFT/A        Pan camera left"); 
				glPrint(offsetx + 20,offsety + j++ * 12,"RIGHT/D       Pan camera right"); j++;

				glPrint(offsetx + 20,offsety + j++ * 12,"Mouse         Change Camera angle"); j++;
				glPrint(offsetx + 20,offsety + j++ * 12,"Left Button   Pick object"); j++;

				j = 32; 
				glPrint(offsetx + 20,offsety + j++ * 12,"                           [F1] Next Page"); j++;

				break;

			case 2:
						
				glPrint(offsetx + 20,offsety + j++ * 12,"Edit Mode Select Keys"); 
				glPrint(offsetx + 20,offsety + j++ * 12,"========================================="); 
				glPrint(offsetx + 20,offsety + j++ * 12,"1             Translation mode"); 
				glPrint(offsetx + 20,offsety + j++ * 12,"2             Rotation mode"); 
				glPrint(offsetx + 20,offsety + j++ * 12,"3             Scale mode"); j++;

				glPrint(offsetx + 20,offsety + j++ * 12,"Edit Value Keys"); 
				glPrint(offsetx + 20,offsety + j++ * 12,"========================================="); 
				glPrint(offsetx + 20,offsety + j++ * 12,"U/J           edit x value"); 
				glPrint(offsetx + 20,offsety + j++ * 12,"I/K           edit y value"); 
				glPrint(offsetx + 20,offsety + j++ * 12,"O/L           edit z value"); j++;

				glPrint(offsetx + 20,offsety + j++ * 12,"[/]           change model used"); j++;

				glPrint(offsetx + 20,offsety + j++ * 12,"TAB           toggle screen info/map"); 
				glPrint(offsetx + 20,offsety + j++ * 12,"Numpad+/-     adjust fog density"); 
				glPrint(offsetx + 20,offsety + j++ * 12,"INS/DEL       adjust water height"); j++;

				j = 32; 
				glPrint(offsetx + 20,offsety + j++ * 12,"                           [F1] Exit Help"); j++;

				break;
			}

			glColor4f(1.0f,1.0f,1.0f,1.0f);

			glEnable (GL_TEXTURE_2D); 

			goPerspective();
		}

		//draw selector area
		if (showhelp==0)
		{
			goOrtho();

			glLoadIdentity();	

			glDisable (GL_TEXTURE_2D); 
			glTranslatef(400,300,0.0f);
			glColor4f(1.0f,0.0f,0.0f,0.5f);
			glBegin(GL_LINE_LOOP);
				glVertex3f( -25.0f, -25.0f, 0.0f);
				glVertex3f(  25.0f, -25.0f, 0.0f);
				glVertex3f(  25.0f,  25.0f, 0.0f);
				glVertex3f( -25.0f,  25.0f, 0.0f);
			glEnd();
			glColor4f(1.0f,1.0f,1.0f,1.0f);

			glEnable (GL_TEXTURE_2D); 

			goPerspective();
		}


		glDisable(GL_DEPTH_TEST);
		ragWindowList.RenderWindows();
		glEnable(GL_DEPTH_TEST);

		glEnable(GL_LIGHTING);

		glPopMatrix();

	} else {
		ro_position_t pos;
		pos.x = 0.0;
		pos.y = 0.0;
		pos.z = 0.0;
		pos.rx = pos.ry = pos.rz = 0.0;
		pos.sx = pos.sy = pos.sz = 1.0;
	//	for (int i = 0; i < nfiles; i++) {
			rsm[0].Display(pos);
	//	}
	}

		
	SwapBuffers(hDC);

	glFlush();
	return TRUE;										// Everything Went OK
}

GLvoid KillGLWindow(GLvoid)								// Properly Kill The Window
{

	KillFont();
	
	if (fullscreen)										// Are We In Fullscreen Mode?
	{
		ChangeDisplaySettings(NULL,0);					// If So Switch Back To The Desktop
		ShowCursor(TRUE);								// Show Mouse Pointer
	}

	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL,NULL))					// Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))						// Are We Able To Delete The RC?
		{
			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;										// Set RC To NULL
	}

	if (hDC && !ReleaseDC(hWnd,hDC))					// Are We Able To Release The DC
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;										// Set DC To NULL
	}

	if (hWnd && !DestroyWindow(hWnd))					// Are We Able To Destroy The Window?
	{
		MessageBox(NULL,"Could Not Release hWnd.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hWnd=NULL;										// Set hWnd To NULL
	}

	if (!UnregisterClass("RagCamGL",hInstance))			// Are We Able To Unregister Class
	{
		MessageBox(NULL,"Could Not Unregister Class.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hInstance=NULL;									// Set hInstance To NULL
	}
}

/*	This Code Creates Our OpenGL Window.  Parameters Are:					*
 *	title			- Title To Appear At The Top Of The Window				*
 *	width			- Width Of The GL Window Or Fullscreen Mode				*
 *	height			- Height Of The GL Window Or Fullscreen Mode			*
 *	bits			- Number Of Bits To Use For Color (8/16/24/32)			*
 *	fullscreenflag	- Use Fullscreen Mode (TRUE) Or Windowed Mode (FALSE)	*/
 
BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag)
{
	GLuint		PixelFormat;			// Holds The Results After Searching For A Match
	WNDCLASS	wc;						// Windows Class Structure
	DWORD		dwExStyle;				// Window Extended Style
	DWORD		dwStyle;				// Window Style
	RECT		WindowRect;				// Grabs Rectangle Upper Left / Lower Right Values
	WindowRect.left=(long)0;			// Set Left Value To 0
	WindowRect.right=(long)width;		// Set Right Value To Requested Width
	WindowRect.top=(long)0;				// Set Top Value To 0
	WindowRect.bottom=(long)height;		// Set Bottom Value To Requested Height

	fullscreen=fullscreenflag;			// Set The Global Fullscreen Flag

	hInstance			= GetModuleHandle(NULL);				// Grab An Instance For Our Window
	wc.style			= CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc		= (WNDPROC) WndProc;					// WndProc Handles Messages
	wc.cbClsExtra		= 0;									// No Extra Window Data
	wc.cbWndExtra		= 0;									// No Extra Window Data
	wc.hInstance		= hInstance;							// Set The Instance
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground	= NULL;									// No Background Required For GL
	wc.lpszMenuName		= NULL;									// We Don't Want A Menu
	wc.lpszClassName	= "RagCamGL";							// Set The Class Name

	if (!RegisterClass(&wc))									// Attempt To Register The Window Class
	{
		MessageBox(NULL,"Failed To Register The Window Class.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;											// Return FALSE
	}
	
	if (fullscreen)												// Attempt Fullscreen Mode?
	{
		DEVMODE dmScreenSettings;								// Device Mode
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));	// Makes Sure Memory's Cleared
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Size Of The Devmode Structure
		dmScreenSettings.dmPelsWidth	= width;				// Selected Screen Width
		dmScreenSettings.dmPelsHeight	= height;				// Selected Screen Height
		dmScreenSettings.dmBitsPerPel	= bits;					// Selected Bits Per Pixel
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{
			// If The Mode Fails, Offer Two Options.  Quit Or Use Windowed Mode.
			if (MessageBox(NULL,"The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?","NeHe GL",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
			{
				fullscreen=FALSE;		// Windowed Mode Selected.  Fullscreen = FALSE
			}
			else
			{
				// Pop Up A Message Box Letting User Know The Program Is Closing.
				MessageBox(NULL,"Program Will Now Close.","ERROR",MB_OK|MB_ICONSTOP);
				return FALSE;									// Return FALSE
			}
		}
	}

	if (fullscreen)												// Are We Still In Fullscreen Mode?
	{
		dwExStyle=WS_EX_APPWINDOW;								// Window Extended Style
		dwStyle=WS_POPUP;										// Windows Style
		ShowCursor(FALSE);										// Hide Mouse Pointer
	}
	else
	{
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			// Window Extended Style
		dwStyle=WS_OVERLAPPEDWINDOW;							// Windows Style
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// Adjust Window To True Requested Size

	// Create The Window
	if (!(hWnd=CreateWindowEx(	dwExStyle,							// Extended Style For The Window
								"RagCamGL",							// Class Name
								title,								// Window Title
								dwStyle |							// Defined Window Style
								WS_CLIPSIBLINGS |					// Required Window Style
								WS_CLIPCHILDREN,					// Required Window Style
								0, 0,								// Window Position
								WindowRect.right-WindowRect.left,	// Calculate Window Width
								WindowRect.bottom-WindowRect.top,	// Calculate Window Height
								NULL,								// No Parent Window
								NULL,								// No Menu
								hInstance,							// Instance
								NULL)))								// Dont Pass Anything To WM_CREATE
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Window Creation Error.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	static	PIXELFORMATDESCRIPTOR pfd=				// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,							// Must Support Double Buffering
		PFD_TYPE_RGBA,								// Request An RGBA Format
		bits,										// Select Our Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		0,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		16,											// 16Bit Z-Buffer (Depth Buffer)  
		0,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};
	
	if (!(hDC=GetDC(hWnd)))							// Did We Get A Device Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Create A GL Device Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))	// Did Windows Find A Matching Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd))		// Are We Able To Set The Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(hRC=wglCreateContext(hDC)))				// Are We Able To Get A Rendering Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!wglMakeCurrent(hDC,hRC))					// Try To Activate The Rendering Context
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	ShowWindow(hWnd,SW_SHOW);						// Show The Window
	SetForegroundWindow(hWnd);						// Slightly Higher Priority
	SetFocus(hWnd);									// Sets Keyboard Focus To The Window
	ReSizeGLScene(width, height);					// Set Up Our Perspective GL Screen

	background = LoadBMP("back.bmp");

	if (!InitGL())									// Initialize Our Newly Created GL Window
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Initialization Failed.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	return TRUE;									// Success
}

LRESULT CALLBACK WndProc(	HWND	hWnd,			// Handle For This Window
							UINT	uMsg,			// Message For This Window
							WPARAM	wParam,			// Additional Message Information
							LPARAM	lParam)			// Additional Message Information
{
	switch (uMsg)									// Check For Windows Messages
	{
		case WM_ACTIVATE:							// Watch For Window Activate Message
		{
			if (!HIWORD(wParam))					// Check Minimization State
			{
				active=TRUE;						// Program Is Active
			}
			else
			{
				active=FALSE;						// Program Is No Longer Active
			}

			return 0;								// Return To The Message Loop
		}

		case WM_SYSCOMMAND:							// Intercept System Commands
		{
			switch (wParam)							// Check System Calls
			{
				case SC_SCREENSAVE:					// Screensaver Trying To Start?
				case SC_MONITORPOWER:				// Monitor Trying To Enter Powersave?
				return 0;							// Prevent From Happening
			}
			break;									// Exit
		}

		case WM_CLOSE:								// Did We Receive A Close Message?
		{
			PostQuitMessage(0);						// Send A Quit Message
			return 0;								// Jump Back
		}

		case WM_KEYDOWN:							// Is A Key Being Held Down?
		{
			keys[wParam] = TRUE;					// If So, Mark It As TRUE
			return 0;								// Jump Back
		}

		case WM_LBUTTONDBLCLK:							// Is A Key Being Held Down?
		{
			POINT mousePos;			// This is a window structure that holds an X and Y
			GetCursorPos(&mousePos);
			ragWindowList.DblClickWindow(mousePos.x,mousePos.y);
			return 0;								// Jump Back
		}

		case WM_LBUTTONDOWN:							// Is A Key Being Held Down?
		{
			POINT mousePos;			// This is a window structure that holds an X and Y
			GetCursorPos(&mousePos);
			if(!ragWindowList.ClickWindow(mousePos.x,mousePos.y))
				rsw.selectobject = true; 
			return 0;								// Jump Back
		}

		case WM_LBUTTONUP:							// Is A Key Being Held Down?
		{
			POINT mousePos;			// This is a window structure that holds an X and Y
			GetCursorPos(&mousePos);			
			ragWindowList.UnClickWindow(mousePos.x,mousePos.y); 
			return 0;								// Jump Back
		}

		case WM_MOUSEMOVE:							// Is A Key Being Held Down?
		{
			POINT mousePos;			// This is a window structure that holds an X and Y
			GetCursorPos(&mousePos);
			ragWindowList.OverWindow(mousePos.x,mousePos.y);
			return 0;								// Jump Back
		}

		case WM_KEYUP:								// Has A Key Been Released?
		{
			keys[wParam] = FALSE;					// If So, Mark It As FALSE
			return 0;								// Jump Back
		}

		case WM_SIZE:								// Resize The OpenGL Window
		{
			ReSizeGLScene(LOWORD(lParam),HIWORD(lParam));  // LoWord=Width, HiWord=Height
			return 0;								// Jump Back
		}

	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

int WINAPI WinMain(	HINSTANCE	hInstance,			// Instance
					HINSTANCE	hPrevInstance,		// Previous Instance
					LPSTR		lpCmdLine,			// Command Line Parameters
					int			nCmdShow)			// Window Show State
{
	MSG		msg;									// Windows Message Structure
	BOOL	done=FALSE;								// Bool Variable To Exit Loop

	char wd[1024];
	FILE *fp;

	_getcwd(wd, 1024);

	strcat(wd, "\\ragcam.arg");

	fp = fopen(wd, "r");

	if (!fp) {
		MessageBox(NULL, "Can't open rsmview.arg", "Error", 0);
		return 0;
	}

	fgets (ragnapath, 1024, fp);

	if (!ragnapath) {
		MessageBox(NULL, "Can't read rsmview.arg", "Error", 0);
		return 0;
	}

	ragnapath[strlen(ragnapath)-1] = '\0';

	while (!feof(fp)) {
		fgets (files[nfiles], 1024, fp);

		if (!strlen(files[nfiles]))
			break;

		if (!files[nfiles]) {
			MessageBox(NULL, "Can't read rsmview.arg", "Error", 0);
			return 0;
		}

		files[nfiles][strlen(files[nfiles])-1] = '\0';

		nfiles++;
	}

	fclose (fp);

	if(strstr(lpCmdLine,"-extract") != NULL) bextract = TRUE;

#ifdef _FORCE_EXTRACT
	bextract = TRUE;
#endif

	//Load the GRF
    GrfError error_code;
	char grfpath[40];

	sprintf(grfpath,"%sdata.grf",ragnapath);

    myGRF = grf_open (grfpath, &error_code);
    if (!myGRF) {
        MessageBox(NULL, "Error open data.grf! Path may be invalid.", "Error", MB_OK + MB_ICONEXCLAMATION);
        //printf ("The error message is: %s\n", grf_strerror (error_code));
        exit (1);
    }



	// Ask The User Which Screen Mode They Prefer
	//if (MessageBox(NULL,"Would You Like To Run In Fullscreen Mode?", "Start FullScreen?",MB_YESNO|MB_ICONQUESTION)==IDNO)
	//{
	//	fullscreen=FALSE;							// Windowed Mode
	//}

	// Create Our OpenGL Window
	if (!CreateGLWindow("RagCam",800,600,32,fullscreen))
	{
		return 0;									// Quit If Window Was Not Created
	}

	g_Camera.PositionCamera(0, 0, 1,	0, 0, 0,	0, 1, 0);

	while(!done)									// Loop That Runs While done=FALSE
	{
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))	// Is There A Message Waiting?
		{
			if (msg.message==WM_QUIT)				// Have We Received A Quit Message?
			{
				done=TRUE;							// If So done=TRUE
			}
			else									// If Not, Deal With Window Messages
			{
				TranslateMessage(&msg);				// Translate The Message
				DispatchMessage(&msg);				// Dispatch The Message
			}
		}
		else										// If There Are No Messages
		{
			// Draw The Scene.  Watch For ESC Key And Quit Messages From DrawGLScene()
			if (active)								// Program Active?
			{
				if (keys[VK_ESCAPE])				// Was ESC Pressed?
				{
					keys[VK_ESCAPE] = FALSE;
					if(showhelp>0) 
						showhelp=0;					// Close Help if shown;
					else
					{
						bool bWasHidden = FALSE;
						int ret = ShowCursor(TRUE);
						bWasHidden = (ret <= 0);
						while (ret < 0); 
						{
							ret = ShowCursor(TRUE);
						} 
						if(MessageBox(hWnd,"Are you sure you want to quit?","RagCam",MB_YESNO + MB_ICONQUESTION)==IDYES)done=TRUE;						// ESC Signalled A Quit

						if (bWasHidden)	{
							int middleX = 800 >> 1;				// This is a binary shift to get half the width
							int middleY = 600 >> 1;				// This is a binary shift to get half the height
							SetCursorPos(middleX, middleY);		
							while(ShowCursor(FALSE) >= 0);
						}
					}
				}
				else								// Not Time To Quit, Update Screen
				{

					if (bMoveCamera)
						g_Camera.Update(); 

					DrawGLScene();					// Draw The Scene
					//SwapBuffers(hDC);				// Swap Buffers (Double Buffering)
				}
			}

			if (keys[VK_RETURN])						// Is F1 Being Pressed?
			{
				if (keys[VK_CONTROL])						// Is F1 Being Pressed?
				{
					keys[VK_CONTROL] = FALSE;
					keys[VK_RETURN]=FALSE;					// If So Make Key FALSE
					KillGLWindow();						// Kill Our Current Window
					fullscreen=!fullscreen;				// Toggle Fullscreen / Windowed Mode
					// Recreate Our OpenGL Window
					if (!CreateGLWindow("RagCam",800,600,32,fullscreen))
					{
						return 0;						// Quit If Window Was Not Created
					}
				}
			}

			if (keys[VK_F1])						// Is F1 Being Pressed?
			{
				keys[VK_F1]=FALSE;					// If So Make Key FALSE
				showhelp++;
				if(showhelp>MAXHELP) showhelp=0;
			}


			if (keys[VK_F2])						// Is F1 Being Pressed?
			{
				keys[VK_F2]=FALSE;					// If So Make Key FALSE
				
				if(b_fog)
				{
					// To turn fog off, you can call glDisable(GL_FOG);
					glEnable(GL_FOG);			
				} else {
					glDisable(GL_FOG);
				}
				b_fog=!b_fog;
			}


			if (keys[VK_F3])						// Is F1 Being Pressed?
			{
				keys[VK_F3]=FALSE;					// If So Make Key FALSE
				g_Camera.PositionCamera(0, 0, 1,	0, 0, 0,	0, 1, 0);
			}

			if (keys[VK_F4])						// Is F1 Being Pressed?
			{
				keys[VK_F4]=FALSE;					// If So Make Key FALSE
				rsw.Save(ragnapath);
			}

			if (keys[VK_F5])						// Is F1 Being Pressed?
			{
				keys[VK_F5]=FALSE;					// If So Make Key FALSE
				rsw.frustumculling = !rsw.frustumculling; 
			}

			if (keys[VK_ADD])						// Is F1 Being Pressed?
			{
				keys[VK_ADD]=FALSE;					// If So Make Key FALSE
				fogd += 0.0001f;
				if(fogd>0.01f) fogd=0.01f;
			}

			if (keys[VK_SUBTRACT])						// Is F1 Being Pressed?
			{
				keys[VK_SUBTRACT]=FALSE;					// If So Make Key FALSE
				fogd -= 0.0001f;
				if(fogd<0.00001f) fogd=0.00001f;
			}

			if (keys[VK_INSERT])						// Is F1 Being Pressed?
			{
				keys[VK_INSERT]=FALSE;					// If So Make Key FALSE
				waterlevel -= 1.0f;
			}


			if (keys[VK_DELETE])						// Is F1 Being Pressed?
			{
				keys[VK_DELETE]=FALSE;					// If So Make Key FALSE
				waterlevel += 1.0f;
			}

			if (keys[VK_TAB])						// Is F1 Being Pressed?
			{
				keys[VK_TAB]=FALSE;					// If So Make Key FALSE
				showinfo++;
				if(showinfo>3)showinfo=0; 
			}

			if (keys[VK_SPACE])						// Is F1 Being Pressed?
			{
				keys[VK_SPACE]=FALSE;					// If So Make Key FALSE
				bMoveCamera = !bMoveCamera;

				if (!bMoveCamera)
					while(ShowCursor(TRUE) < 0);
				else
				{
					//prevent the mouse from jerking around when we switch modes;
					int middleX = 800 >> 1;				// This is a binary shift to get half the width
					int middleY = 600 >> 1;				// This is a binary shift to get half the height
					SetCursorPos(middleX, middleY);		
					
					while(ShowCursor(FALSE) >= 0);
				}

			}

			
			if (keys[VK_1])						// Is F1 Being Pressed?
			{
				keys[VK_1]=FALSE;					// If So Make Key FALSE
				editmode = 0;
			}
			

			if (keys[VK_2])						// Is F1 Being Pressed?
			{
				keys[VK_2]=FALSE;					// If So Make Key FALSE
				editmode = 1;
			}


			if (keys[VK_3])						// Is F1 Being Pressed?
			{
				keys[VK_3]=FALSE;					// If So Make Key FALSE
				editmode = 2;
			}

			if (keys[VK_4])						// Is F1 Being Pressed?
			{
				keys[VK_4]=FALSE;					// If So Make Key FALSE
				editmode = 3;
			}


			if (keys[VK_U])						
			{
				keys[VK_U]=FALSE;			
				if (brsw)
				switch(editmode)
				{
				case 0:	rsw.SelectedModel()->position.x++;  break;
				case 1:	rsw.SelectedModel()->position.rx++;  break;
				case 2:	rsw.SelectedModel()->position.sx+=0.01;  break;
				case 3:	minipos.x++;  break;
				}
			}

			if (keys[VK_J])						
			{
				keys[VK_J]=FALSE;				
				if (brsw)
				switch(editmode)
				{
				case 0:	rsw.SelectedModel()->position.x--;  break;
				case 1:	rsw.SelectedModel()->position.rx--;  break;
				case 2:	rsw.SelectedModel()->position.sx-=0.01;  break;
				case 3:	minipos.x--;  break;
				}
			}


			if (keys[VK_I])						
			{
				keys[VK_I]=FALSE;			
				if (brsw)
				switch(editmode)
				{
				case 0:	rsw.SelectedModel()->position.y++;  break;
				case 1:	rsw.SelectedModel()->position.ry++;  break;
				case 2:	rsw.SelectedModel()->position.sy+=0.01;  break;
				case 3:	minipos.y++;  break;
				}
			}

			if (keys[VK_K])						
			{
				keys[VK_K]=FALSE;				
				if (brsw)
				switch(editmode)
				{
				case 0:	rsw.SelectedModel()->position.y--;  break;
				case 1:	rsw.SelectedModel()->position.ry--;  break;
				case 2:	rsw.SelectedModel()->position.sy-=0.01;  break;
				case 3:	minipos.y--;  break;
				}
			}

			if (keys[VK_O])						
			{
				keys[VK_O]=FALSE;			
				if (brsw)
				switch(editmode)
				{
				case 0:	rsw.SelectedModel()->position.z++;  break;
				case 1:	rsw.SelectedModel()->position.rz++;  break;
				case 2:	rsw.SelectedModel()->position.sz+=0.01;  break;
				case 3:	minipos.z++;  break;
				}
			}

			if (keys[VK_L])						
			{
				keys[VK_L]=FALSE;				
				if (brsw)
				switch(editmode)
				{
				case 0:	rsw.SelectedModel()->position.z--;  break;
				case 1:	rsw.SelectedModel()->position.rz--;  break;
				case 2:	rsw.SelectedModel()->position.sz-=0.01;  break;
				case 3:	minipos.z--;  break;
				}
			}

			if (keys[VK_LSQUAREBRACKET])						
			{
				keys[VK_LSQUAREBRACKET]=FALSE;				
				if (brsw)
				{
					int tempmodel;
					tempmodel = rsw.SelectedModel()->model;
					tempmodel--;
					if (tempmodel<0) tempmodel = 0;
					rsw.SelectedModel()->model = tempmodel;
				}
			}


			if (keys[VK_RSQUAREBRACKET])						
			{
				keys[VK_RSQUAREBRACKET]=FALSE;				
				if (brsw)
				{
					int tempmodel;
					tempmodel = rsw.SelectedModel()->model;
					tempmodel++;
					if (tempmodel> rsw.numrealmodels() - 1) tempmodel = rsw.numrealmodels() - 1;
					rsw.SelectedModel()->model = tempmodel;
				}
			}

		}
	}

	while(ShowCursor(TRUE) < 0);

	// Shutdown
	grf_free(myGRF);

	KillGLWindow();									// Kill The Window
	return (msg.wParam);							// Exit The Program
}
