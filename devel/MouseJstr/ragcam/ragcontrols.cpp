#include "ragwindows.h"

#define TITLEBARHEIGHT 30

/*==========================================================================*/
/*						Ragnarok Control Class								*/
/*==========================================================================*/

CRagControl::CRagControl()
{
	next = NULL;
	prev = NULL;
	parent = NULL;
	x = 0;
	y = 0;
}


CRagControl::~CRagControl()
{
}

void CRagControl::Render() 
{

}

bool CRagControl::UnClickWindow(int x, int y) 
{
	return FALSE;
}

bool CRagControl::ClickWindow(int x, int y) 
{
	return FALSE;
}

bool CRagControl::OverWindow(int x, int y) 
{
	return FALSE;
}

bool CRagControl::DblClickWindow(int x, int y) 
{
	return FALSE;
}

/*==========================================================================*/
/*						Ragnarok List Class									*/
/*==========================================================================*/


CRagListBox::CRagListBox()
{
	lastitem = 0;
	move=0;
	OnDblClick = NULL;
	list[0][0] = (char ) malloc(255*1000*sizeof(char));
}

CRagListBox::CRagListBox(HWND hWnd, Grf *grfFile, int x, int y, int w, int h, void (*pHandler)())
{
	lastitem = 0;
	move=0;
	index = -1;
	OnDblClick = pHandler;

 	list[0][0] = (char ) malloc(255*1000*sizeof(char));

	startrender =0;

	width = w;
	height = h;
	posx = x;
	posy = y;
	parentHWnd = hWnd,

	next = NULL;
	prev = NULL;

	glGenTextures(32, &textures[0]);

	LoadGRFTexture(grfFile,"유저인터페이스\\scroll0up.bmp",  textures[TEX_SCROLL_UP], NULL,FALSE);
	LoadGRFTexture(grfFile,"유저인터페이스\\scroll0mid.bmp",  textures[TEX_SCROLL_MI], NULL,FALSE);
	LoadGRFTexture(grfFile,"유저인터페이스\\scroll0down.bmp",  textures[TEX_SCROLL_DN], NULL,FALSE);

	LoadGRFTexture(grfFile,"유저인터페이스\\scroll0bar_up.bmp",  textures[TEX_SCROLLBAR_UP], NULL,FALSE);
	LoadGRFTexture(grfFile,"유저인터페이스\\scroll0bar_mid.bmp",  textures[TEX_SCROLLBAR_MI], NULL,FALSE);
	LoadGRFTexture(grfFile,"유저인터페이스\\scroll0bar_down.bmp",  textures[TEX_SCROLLBAR_DN], NULL,FALSE);
}


CRagListBox::~CRagListBox()
{
	free(list);
}

void CRagListBox::Add(char *itemname)
{
	sprintf(&list[lastitem++][0],"%s",itemname);
}

void CRagListBox::Clear()
{
	lastitem = 0;
}

bool CRagListBox::UnClickWindow(int x, int y)
{
	move=0;
	return FALSE;
}

bool CRagListBox::DblClickWindow(int x, int y) 
{
	LPRECT ParentWindow;
	ParentWindow = (LPRECT) malloc (sizeof(LPRECT));
	GetWindowRect(parentHWnd, ParentWindow);

	if ((x >= posx + parent->posx + ParentWindow->left) & (y >= posy + parent->posy + ParentWindow->top + TITLEBARHEIGHT) & (x <= posx + parent->posx + width + ParentWindow->left ) & (y <= posy + parent->posy + height + ParentWindow->top + TITLEBARHEIGHT )) 
	{
		if(OnDblClick != NULL) OnDblClick();
		return TRUE;
	}

	return FALSE;
}

bool CRagListBox::ClickWindow(int x, int y) 
{
	LPRECT ParentWindow;
	ParentWindow = (LPRECT) malloc (sizeof(LPRECT));
	GetWindowRect(parentHWnd, ParentWindow);

	if ((x >= posx + parent->posx + ParentWindow->left) & (y >= posy + parent->posy + ParentWindow->top + TITLEBARHEIGHT) & (x <= posx + parent->posx + width + ParentWindow->left ) & (y <= posy + parent->posy + height + ParentWindow->top + TITLEBARHEIGHT )) 
	{
		if ((x >= posx + parent->posx + ParentWindow->left + width - 13) & (y >= posy + parent->posy + ParentWindow->top + TITLEBARHEIGHT) & (x <= posx + parent->posx + width + ParentWindow->left ) & (y <= posy + parent->posy + 13 + ParentWindow->top + TITLEBARHEIGHT)) 
		{
			move = -1;
		} else if ((x >= posx + parent->posx + ParentWindow->left + width - 13) & (y >= posy + parent->posy + height - 13 + ParentWindow->top + TITLEBARHEIGHT) & (x <= posx + parent->posx + width + ParentWindow->left ) & (y <= posy + parent->posy + height + ParentWindow->top + TITLEBARHEIGHT)) 
		{
			move = 1;
		} else
		{
			index = startrender + (y - posy - parent->posy - ParentWindow->top - TITLEBARHEIGHT) / 12;
		}
		
		return TRUE;
	}

	return FALSE;
}

bool CRagListBox::OverWindow(int x, int y) 
{
	LPRECT ParentWindow;
	ParentWindow = (LPRECT) malloc (sizeof(LPRECT));
	GetWindowRect(parentHWnd, ParentWindow);

	if ((x >= posx + parent->posx + ParentWindow->left) & (y >= posy + parent->posy + ParentWindow->top + TITLEBARHEIGHT) & (x <= posx + parent->posx + 42 + ParentWindow->left ) & (y <= posy + parent->posy + 20 + ParentWindow->top + TITLEBARHEIGHT )) 
	{
		return TRUE;
	}

	return FALSE;
}

void CRagListBox::Render() 
{
	glPushMatrix();
	
	glTranslatef(posx,posy,0);

	startrender+=move;
	if (startrender<0) startrender = 0;
	if (startrender>=lastitem) startrender = lastitem-5;

	glPushMatrix();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);

	glColor4f(0.95f, 0.95f, 0.95f, parent->transparency);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);	glVertex3f(	        0.0f,          0.0f, 0.0f);
		glTexCoord2f(1.0f, 0.0f);	glVertex3f(	(float)width,          0.0f, 0.0f);
		glTexCoord2f(1.0f, 1.0f);	glVertex3f( (float)width, (float)height, 0.0f);
		glTexCoord2f(0.0f, 1.0f);	glVertex3f(         0.0f, (float)height, 0.0f);
	glEnd();

	glEnable(GL_TEXTURE_2D);

	glTranslatef((float)width - 13.0f, 0,0);
	glBindTexture(GL_TEXTURE_2D,textures[TEX_SCROLL_UP]);
	glColor4f(1.0f, 1.0f, 1.0f, parent->transparency);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);	glVertex3f(	 0.0f,  0.0f, 0.0f);
		glTexCoord2f(1.0f, 0.0f);	glVertex3f(	13.0f,  0.0f, 0.0f);
		glTexCoord2f(1.0f, 1.0f);	glVertex3f( 13.0f, 13.0f, 0.0f);
		glTexCoord2f(0.0f, 1.0f);	glVertex3f(  0.0f, 13.0f, 0.0f);
	glEnd();

	glTranslatef(0,11.0f,0);
	glBindTexture(GL_TEXTURE_2D,textures[TEX_SCROLL_MI]);
	glColor4f(1.0f, 1.0f, 1.0f, parent->transparency);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);	glVertex3f(	 0.0f,  0.0f, 0.0f);
		glTexCoord2f(1.0f, 0.0f);	glVertex3f(	13.0f,  0.0f, 0.0f);
		glTexCoord2f(1.0f, 1.0f);	glVertex3f( 13.0f, (float)height - 22.0f, 0.0f);
		glTexCoord2f(0.0f, 1.0f);	glVertex3f(  0.0f, (float)height - 22.0f, 0.0f);
	glEnd();

	glTranslatef(0, (float)height - 26.0f,0);
	glBindTexture(GL_TEXTURE_2D,textures[TEX_SCROLL_DN]);
	glColor4f(1.0f, 1.0f, 1.0f, parent->transparency);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);	glVertex3f(	 0.0f,  0.0f, 0.0f);
		glTexCoord2f(1.0f, 0.0f);	glVertex3f(	13.0f,  0.0f, 0.0f);
		glTexCoord2f(1.0f, 1.0f);	glVertex3f( 13.0f, 13.0f, 0.0f);
		glTexCoord2f(0.0f, 1.0f);	glVertex3f(  0.0f, 13.0f, 0.0f);
	glEnd();


	glPopMatrix();

	glDisable(GL_TEXTURE_2D);

	if ((index>=startrender) && ((index-startrender+1)*12<height))
	{
		glPushMatrix();
	
		glTranslatef(0,12 * (index - startrender),0);

		glColor4f(0.95f, 0.95f, 1.0f, parent->transparency);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f);	glVertex3f(	             0.0f,  0.0f, 0.0f);
			glTexCoord2f(1.0f, 0.0f);	glVertex3f(	(float)width - 13,  0.0f, 0.0f);
			glTexCoord2f(1.0f, 1.0f);	glVertex3f( (float)width - 13, 12.0f, 0.0f);
			glTexCoord2f(0.0f, 1.0f);	glVertex3f(              0.0f, 12.0f, 0.0f);
		glEnd();
		glPopMatrix();
	}

	int i= 0;

	glColor4f(0.0f,0.0f,0.0f, parent->transparency);

	while((i<lastitem) && ((i+1)*12<height))
	{
		glPrint(posx + parent->posx, posy + parent->posy + (i+1) * 12,"%s",list[startrender+i]);
		i++;
	}

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.90);

	glPopMatrix();
}


/*==========================================================================*/
/*						Ragnarok Button Class								*/
/*==========================================================================*/


CRagButton::CRagButton()
{
	posx = 0;
	posy = 0;
	parentHWnd = NULL;
	OnClick = NULL;
	next = NULL;
	prev = NULL;
	current_tex = ID_OFF;
}

CRagButton::CRagButton(HWND hWnd, Grf *grfFile, RagButtonEnum style, int x, int y, void (*pHandler)())
{
	glGenTextures(3, &texture[0]);

	posx = x;
	posy = y;
	parentHWnd = hWnd,
	OnClick = pHandler;

	next = NULL;
	prev = NULL;
	current_tex = ID_OFF;

	switch(style)
	{
	case RB_OK:
		LoadGRFTexture(grfFile,"유저인터페이스\\btn_ok.bmp",  texture[ID_OFF], NULL,FALSE);
		LoadGRFTexture(grfFile,"유저인터페이스\\btn_ok_a.bmp",texture[ID_ON],  NULL,FALSE);
		LoadGRFTexture(grfFile,"유저인터페이스\\btn_ok_b.bmp",texture[ID_DOWN],NULL,FALSE);
		break;

	case RB_CANCEL:
		LoadGRFTexture(grfFile,"유저인터페이스\\btn_cancel.bmp",  texture[ID_OFF], NULL,FALSE);
		LoadGRFTexture(grfFile,"유저인터페이스\\btn_cancel_a.bmp",texture[ID_ON],  NULL,FALSE);
		LoadGRFTexture(grfFile,"유저인터페이스\\btn_cancel_b.bmp",texture[ID_DOWN],NULL,FALSE);
		break;

	case RB_CLOSE:
		LoadGRFTexture(grfFile,"유저인터페이스\\btn_close.bmp",  texture[ID_OFF], NULL,FALSE);
		LoadGRFTexture(grfFile,"유저인터페이스\\btn_close_a.bmp",texture[ID_ON],  NULL,FALSE);
		LoadGRFTexture(grfFile,"유저인터페이스\\btn_close_b.bmp",texture[ID_DOWN],NULL,FALSE);
		break;
	}	
}


CRagButton::~CRagButton()
{
	posx = 0;
	posy = 0;
	parentHWnd = NULL;
	OnClick = NULL;
	next = NULL;
	prev = NULL;
	current_tex = ID_OFF;
	glDeleteTextures(3, &texture[0]);
}

bool CRagButton::UnClickWindow(int x, int y)
{
	//if(current_tex == ID_DOWN) 
	//	current_tex = ID_ON;
	current_tex = ID_ON;
	return FALSE;
}

bool CRagButton::ClickWindow(int x, int y) 
{
	LPRECT ParentWindow;
	ParentWindow = (LPRECT) malloc (sizeof(LPRECT));
	GetWindowRect(parentHWnd, ParentWindow);

	if ((x >= posx + parent->posx + ParentWindow->left) & (y >= posy + parent->posy + ParentWindow->top + TITLEBARHEIGHT) & (x <= posx + parent->posx + 42 + ParentWindow->left ) & (y <= posy + parent->posy + 20 + ParentWindow->top + TITLEBARHEIGHT )) 
	{
		current_tex = ID_DOWN;
		if(OnClick != NULL) OnClick();
		return TRUE;
	}

	return FALSE;
}

bool CRagButton::OverWindow(int x, int y) 
{
	if(current_tex == ID_DOWN) return FALSE;

	LPRECT ParentWindow;
	ParentWindow = (LPRECT) malloc (sizeof(LPRECT));
	GetWindowRect(parentHWnd, ParentWindow);

	if ((x >= posx + parent->posx + ParentWindow->left) & (y >= posy + parent->posy + ParentWindow->top + TITLEBARHEIGHT) & (x <= posx + parent->posx + 42 + ParentWindow->left ) & (y <= posy + parent->posy + 20 + ParentWindow->top + + TITLEBARHEIGHT )) 
	{
		current_tex = ID_ON;
		return TRUE;
	}

	current_tex = ID_OFF;
	return FALSE;
}

bool CRagButton::DblClickWindow(int x, int y) 
{
	return FALSE;
}


void CRagButton::Render() 
{
	glPushMatrix();
	
	glTranslatef(posx,posy,0);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture[current_tex]);

	glDisable(GL_ALPHA_TEST);

	glColor4f(1.0f,1.0f,1.0f, parent->transparency);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);	glVertex3f(	 0.0f,  0.0f, 0.0f);
		glTexCoord2f(1.0f, 0.0f);	glVertex3f(	42.0f,  0.0f, 0.0f);
		glTexCoord2f(1.0f, 1.0f);	glVertex3f( 42.0f, 20.0f, 0.0f);
		glTexCoord2f(0.0f, 1.0f);	glVertex3f(  0.0f, 20.0f, 0.0f);
	glEnd();

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.90);

	glPopMatrix();
}

