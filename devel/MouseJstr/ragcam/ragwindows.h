#ifndef RO_WINDOWS_H
#define RO_WINDOWS_H

#include <windows.h>
#include <math.h>		// Header File For Windows
#include <stdio.h>
#include <stdlib.h>
#include <gl\gl.h>			// Header File For The OpenGL32 Library
#include <gl\glu.h>			// Header File For The GLu32 Library
#include <gl\glaux.h>		// Header File For The Glaux Library
#include "ro-common.h"

class CRagWindow;
class CRagWindowList;

enum RagScrollTexture
{
	TEX_SCROLL_UP,
	TEX_SCROLL_MI,
	TEX_SCROLL_DN,
	TEX_SCROLLBAR_UP,
	TEX_SCROLLBAR_MI,
	TEX_SCROLLBAR_DN
};

enum RagButtonEnum
{
	RB_OK,
	RB_CLOSE,
	RB_CANCEL
};

enum RagButtonState
{
	ID_OFF,
	ID_ON,
	ID_DOWN
};

class CRagControl
{
public:
	CRagControl();
	virtual ~CRagControl();
	virtual void Render() ;
	virtual bool ClickWindow(int x, int y);
	virtual bool UnClickWindow(int x, int y);
	virtual bool OverWindow(int x, int y);
	virtual bool DblClickWindow(int x, int y);

	void setCtlParent(CRagWindow *newparent) { parent = newparent;}

	CRagControl *next;
	CRagControl *prev;

protected:
	CRagWindow  *parent;
	HWND parentHWnd;
	int x;
	int y;

};

class CRagListBox : public CRagControl
{
public:
	CRagListBox();
	CRagListBox(HWND hWnd, Grf *grfFile, int x, int y, int w, int h, void (*pHandler)());
	virtual ~CRagListBox();
	void Add(char* itemname);
	void Remove(int index);
	void Clear();
	char *GetItem(unsigned int itemno) { if(itemno>=0) return &list[itemno][0]; else return NULL; }
	virtual void Render() ;
	virtual bool ClickWindow(int x, int y);
	virtual bool UnClickWindow(int x, int y);
	virtual bool DblClickWindow(int x, int y);
	virtual bool OverWindow(int x, int y);
	int index;

	void (*OnDblClick)();

private:
	unsigned int startrender;
	GLuint textures[32];
	unsigned int lastitem;
	char list[1000][255];
	int height;
	int width;
	int posx;
	int posy;
	int move;
};



class CRagButton : public CRagControl
{
public:
	CRagButton();
	virtual ~CRagButton();
	CRagButton(HWND hWnd, Grf *grfFile, RagButtonEnum style, int x, int y, void (*pHandler)());
	virtual void Render() ;
	virtual bool ClickWindow(int x, int y) ;
	virtual bool UnClickWindow(int x, int y) ;
	virtual bool OverWindow(int x, int y);
	virtual bool DblClickWindow(int x, int y);
	void setButton(int newState) { current_tex = newState;}


	void (*OnClick)();

private:
	GLuint texture[3];
	int current_tex;
	int posx;
	int posy;
};

/*==========================================================================*/
/*							Ragnarok-style Window Object	                */
/*==========================================================================*/

class CRagWindow
{
public:
	CRagWindow();
	CRagWindow(HWND hWnd, Grf *grfFile, const char* title, int x, int y, int w, int h, int t);
	virtual ~CRagWindow();
	void AddControl(CRagControl *pControl);
	int ClickWindow(int x, int y);
	void UnClickWindow(int x, int y);
	int OverWindow(int x, int y);
	void DblClickWindow(int x, int y);

	void RenderWindow();
	void setVisible(bool visibility) { bVisible = visibility;}
	CRagControl *controls;
	void setWndParent(CRagWindowList *newparent) { parentlist = newparent;}

	int posx;
	int posy;
	float transparency;
	int zIndex;

	CRagWindow *next;
	CRagWindow *prev;

private:
	CRagWindowList *parentlist;
	char* windowtitle;
	int width;
	int height;
	int windowtype;
	GLuint windowtex;
	bool bClicked;
	bool bVisible;
	HWND parentHWnd;
	int offsetx;
	int offsety;
};


/*==========================================================================*/
/*							Window List Holder								*/
/*==========================================================================*/


class CRagWindowList
{
public:
	CRagWindowList();
	virtual ~CRagWindowList();
	void RenderWindows();
	void AddWindow(CRagWindow *window);
	void DeleteWindow(CRagWindow *window);
	int ClickWindow(int x, int y);
	void UnClickWindow(int x, int y);
	int OverWindow(int x, int y);
	void DblClickWindow(int x, int y);

	void RaiseWindow(CRagWindow *pWindow);

private:
	CRagWindow *windowlist;
	CRagWindow *sortedlist[50];
	int numwindows;
};


#endif // RO_WINDOWS_H