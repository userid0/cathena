#include "ragwindows.h"

/*==========================================================================*/
/*						Ragnarok-style Window List Holder					*/
/*==========================================================================*/

CRagWindowList::CRagWindowList()
{
	windowlist = NULL;
}

CRagWindowList::~CRagWindowList()
{
}

void CRagWindowList::RaiseWindow(CRagWindow *pWindow)
{
	CRagWindow *listptr = windowlist;
	while(listptr!=NULL)
	{
		if((listptr->zIndex==pWindow->zIndex) & (listptr!=pWindow))
		{
			listptr->zIndex -= 1;
			RaiseWindow(listptr);
		}
		listptr = listptr->next;
	}
}

void CRagWindowList::AddWindow(CRagWindow *pWindow)
{
	if(windowlist==NULL)
	{
		pWindow->zIndex = 0;
		pWindow->setWndParent(this); 
		windowlist = pWindow;
	} else
	{
		CRagWindow *listptr = windowlist;
		CRagWindow *lastptr;
		while(listptr!=NULL)
		{
			lastptr = listptr;
			listptr->zIndex--;
			listptr = listptr->next;
		}

		pWindow->zIndex = 0;
		pWindow->setWndParent(this); 
		lastptr->next = pWindow;
	}
	numwindows++;
}

void CRagWindowList::DeleteWindow(CRagWindow *window)
{
}

void CRagWindowList::RenderWindows()
{
	CRagWindow *listptr = windowlist;


	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	glPushMatrix();

	goOrtho();

	int i=0;
	while(listptr!=NULL)
	{
		sortedlist[abs(listptr->zIndex)] = listptr;
		listptr = listptr->next;
		i++;
	} 
	
	i=numwindows-1;
	while(i>=0)
	{
		sortedlist[i--]->RenderWindow();
	} 

	/*	while(listptr!=NULL)
	{
		listptr->RenderWindow();
		listptr = listptr->next;
	} */

	goPerspective();

	glPopMatrix();

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
}

void CRagWindowList::UnClickWindow(int x, int y)
{
	CRagWindow *listptr = windowlist;

	while(listptr!=NULL)
	{
		listptr->UnClickWindow(x,y);
		listptr = listptr->next;
	} 

}

void CRagWindowList::DblClickWindow(int x, int y)
{
	CRagWindow *listptr = windowlist;

	while(listptr!=NULL)
	{
		listptr->DblClickWindow(x,y);
		listptr = listptr->next;
	} 

}

int CRagWindowList::ClickWindow(int x, int y)
{
	int i = 0 , ret = 0;
	while(i<numwindows)
	{
		ret = sortedlist[i++]->ClickWindow(x,y);
		if(ret>0) return ret;
	} 
	return 0;
}

int CRagWindowList::OverWindow(int x, int y)
{
	int i = 0 , ret = 0;
	if(sortedlist[0]==NULL) return(0);
	while(i<numwindows)
	{
		ret = sortedlist[i++]->OverWindow(x,y);
		if(ret>0) return ret;
	} 
	return 0;
}

/*==========================================================================*/
/*							Ragnarok-style Window Object	                */
/*==========================================================================*/

CRagWindow::CRagWindow()
{
	next = NULL;
	prev = NULL;
	controls = NULL;
	
	posx = 0;
	posy = 0;
	width = 0;
	height = 0;
	windowtype = 0;

	bClicked = FALSE;
	bVisible = TRUE;

	windowtex = 0;

	windowtitle = "No Title";

	width = 280;
	height = 120;
	transparency = 1.0f;
	zIndex=0;

}

CRagWindow::CRagWindow(HWND hWnd, Grf *grfFile, const char* title, int x, int y, int w, int h, int t)
{
	next = NULL;
	prev = NULL;
	controls = NULL;

	windowtitle = (char *) malloc(strlen(title));

	sprintf(windowtitle,"%s",title);
	posx = x;
	posy = y;
	width = 280;
	height = 120;
	windowtype = t;
	transparency = 1.0f;
	bClicked = FALSE;
	bVisible = TRUE;
	zIndex=0;
	parentHWnd = hWnd;

	glGenTextures(1, &windowtex);

	LoadGRFTexture(grfFile,"유저인터페이스\\win_msgbox.bmp",windowtex,NULL,FALSE);
}


CRagWindow::~CRagWindow()
{
}

void CRagWindow::AddControl(CRagControl *pControl)
{
	if(controls==NULL)
	{
		controls = pControl;
		pControl->setCtlParent(this);
	} else
	{
		CRagControl *listptr = controls;
		CRagControl *lastptr;
		while(listptr!=NULL)
		{
			lastptr = listptr ;
			listptr = listptr->next;
		}

		pControl->setCtlParent(this);
		lastptr->next = pControl;
	}
}


void CRagWindow::UnClickWindow(int x, int y)
{
	CRagControl *pControl = controls;
	while(pControl!=NULL)
	{	
		if(pControl->UnClickWindow(x,y)) return;
		pControl = pControl->next; 
	}

	bClicked = FALSE;
}

void CRagWindow::DblClickWindow(int x, int y)
{
	CRagControl *pControl = controls;
	while(pControl!=NULL)
	{	
		if(pControl->DblClickWindow(x,y)) return;
		pControl = pControl->next; 
	}

	bClicked = FALSE;
}


#define TITLEBARHEIGHT 30

int CRagWindow::ClickWindow(int x, int y)
{
	if (bVisible)
	{
		LPRECT ParentWindow;
		ParentWindow = (LPRECT) malloc (sizeof(LPRECT));
		GetWindowRect(parentHWnd, ParentWindow);
		
		if ((x >= posx + ParentWindow->left) & (y >= posy + ParentWindow->top + TITLEBARHEIGHT) & (x <= posx + width + ParentWindow->left ) & (y <= posy + height + ParentWindow->top + + TITLEBARHEIGHT )) 
		{
			CRagControl *pControl = controls;
			while(pControl!=NULL)
			{	
				if(pControl->ClickWindow(x,y)) return 2;
				pControl = pControl->next; 
			}

			zIndex = 0;
			parentlist->RaiseWindow(this);

			bClicked = TRUE;

			POINT mousePos;			// This is a window structure that holds an X and Y
			GetCursorPos(&mousePos);		
			offsetx = mousePos.x - posx;
			offsety = mousePos.y - posy;
			return 1;
		}
	}
	return 0;
}

int CRagWindow::OverWindow(int x, int y)
{
	if (bVisible)
	{
		LPRECT ParentWindow;
		ParentWindow = (LPRECT) malloc (sizeof(LPRECT));
		GetWindowRect(parentHWnd, ParentWindow);
		
		if ((x >= posx + ParentWindow->left) & (y >= posy + ParentWindow->top + TITLEBARHEIGHT) & (x <= posx + width + ParentWindow->left ) & (y <= posy + height + ParentWindow->top + + TITLEBARHEIGHT )) 
		{
			CRagControl *pControl = controls;
			while(pControl!=NULL)
			{	
				if(pControl->OverWindow(x,y)) return 2;
				pControl = pControl->next; 
			}

			return 1;
		}
	}
	return 0;
}


void CRagWindow::RenderWindow()
{
	if (bVisible)
	{
		if (bClicked)
		{
			if (transparency > 0.5) transparency -= 0.1f;

			POINT mousePos;			// This is a window structure that holds an X and Y
			GetCursorPos(&mousePos);	
			posx = mousePos.x - offsetx;
			posy = mousePos.y - offsety;
			
			//snap to edges
			if (abs(mousePos.x - offsetx) < 10) posx=0;
			if (abs(mousePos.y - offsety) < 10) posy=0;
			if (abs(mousePos.x - offsetx + width - 800) < 10) posx = 800 - width;
			if (abs(mousePos.y - offsety + height - 600) < 10) posy = 600 - height;

		} else
		{
			transparency = 1.0f;
		}

		CRagControl *pControl = controls;


		glDisable(GL_LIGHTING);
		glDisable(GL_ALPHA_TEST);

		glPushMatrix();
		
		glLoadIdentity();
		glTranslatef(posx,posy,0.0f);

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, windowtex);

		glColor4f(1.0f,1.0f,1.0f,transparency);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f);	glVertex3f(		   0.0f,          0.0f, 0.0f);
			glTexCoord2f(1.0f, 0.0f);	glVertex3f((float)width,          0.0f, 0.0f);
			glTexCoord2f(1.0f, 1.0f);	glVertex3f((float)width, (float)height, 0.0f);
			glTexCoord2f(0.0f, 1.0f);	glVertex3f(        0.0f, (float)height, 0.0f);
		glEnd();

		while(pControl!=NULL)
		{	
			pControl->Render();
			pControl = pControl->next; 
		}

		glPopMatrix();

		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.90);

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);

		glColor4f(0.0f,0.0f,0.0f,1.0f);
		//glPrint(posx + 8 ,posy + 35,"%s", windowtitle);
	}
}
