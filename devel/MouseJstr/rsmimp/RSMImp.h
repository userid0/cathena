/**********************************************************************
 *<
	FILE: RSMImp.h

	DESCRIPTION:	Template Utility

	CREATED BY:

	HISTORY:

 *>	Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/

#ifndef __RSMIMP__H
#define __RSMIMP__H

#include "Max.h"
#include "resource.h"
#include "istdplug.h"
#include "iparamb2.h"
#include "iparamm2.h"
#include "dummy.h"
#include <direct.h>
//#include <commdlg.h>

extern TCHAR *GetString(int id);

extern HINSTANCE hInstance;

#define MIN(a, b) (a>b)?b:a
#define MAX(a, b) (a>b)?a:b

typedef unsigned char ro_lightmap_t[256];

typedef struct {
	float u1;
	float u2;
	float u3;
	float u4;
	float v1;
	float v2;
	float v3;
	float v4;

	unsigned short text;
	unsigned short lmap;

	int todo;
} ro_tile_t;

typedef struct {
	float y1;
	float y2;
	float y3;
	float y4;
	int tilesup;
	int tileotherside;
	int tileside;
} ro_cube_t;

typedef struct {
	char filecode[4];
	short magicnumber;
} gnd_header_t;


typedef struct {
	char filecode[4];
	char todo[27];
} rsm_header_t;

typedef int *pint;

typedef char ro_string_t[40];

typedef float ro_quat_t[4];

#define QUATERNION_SCALE(q) ((q[0]*q[0]+q[1]*q[1]+q[2]*q[2])*(q[0]*q[0]+q[1]*q[1]+q[2]*q[2]) + q[3]*q[3])

typedef struct {
	int time;
	ro_quat_t orientation;
} ro_frame_t;

typedef struct {
	float x;
	float y;
	float z;
	float rx;
	float ry;
	float rz;
	float sx;
	float sy;
	float sz;
} ro_position_t;

typedef struct {
	float max[3];
	float min[3];
	float range[3];
} bounding_box_t;

typedef struct {
	int model;
	ro_position_t position;
} ro_model_t;

typedef float ro_vertex_t[3];

typedef float gl_vertex_t[5];

typedef int gl_face_t[3];

typedef struct {
	short v[3];
	short t[3];
	unsigned short	  text;
	unsigned short	todo1;
	unsigned int   todo2;
	unsigned int   nsurf;
} ro_face_t;

typedef struct {
	unsigned int text;
	int nvertex;
	int nfaces;
	gl_vertex_t *vertex;
	gl_face_t *faces;
} ro_surface_t;

typedef struct {
	float todo[22];
} ro_transf_t;

typedef float ro_quat_t[4];

#pragma pack(1)


/* 3DS Shape point structure */

struct shppt
{
float x;	/* Control point */
float y;
float z;
float inx;	/* Incoming vector */
float iny;
float inz;
float outx;	/* Outgoing vector */
float outy;
float outz;
unsigned short flags;
};
typedef struct shppt Shppt;

#pragma pack()

struct Bkgrad {
	float midpct;
	Color botcolor;
	Color midcolor;
	Color topcolor;
	};

struct Fogdata {
	float nearplane;
	float neardens;
	float farplane;
	float fardens;
	Color color;
	};

struct LFogData {
	float zmin,zmax;
	float density;
	short type;
	short fog_bg;
	Color color;
	};

struct Distcue {
	float nearplane;
	float neardim;
	float farplane;
	float fardim;
	};

#define ENV_DISTCUE 1
#define ENV_FOG 2
#define ENV_LAYFOG 3

#define BG_SOLID 1
#define BG_GRADIENT 2
#define BG_BITMAP 3

struct BGdata {
	int bgType;
	int envType;
	Color bkgd_solid;
	Color amb_light;
	Bkgrad bkgd_gradient;
	Fogdata fog_data;
	Distcue distance_cue;
	int fog_bg,dim_bg;
	char bkgd_map[81];
	LFogData lfog_data;
	};

#define SINGLE_SHAPE 0
#define MULTIPLE_SHAPES 1

// Handy file class

class WorkFile {
private:
	FILE *stream;
public:
					WorkFile(const TCHAR *filename,const TCHAR *mode) { stream = _tfopen(filename,mode); };
					~WorkFile() { if(stream) fclose(stream); stream = NULL; };
	FILE *			Stream() { return stream; };
	};

// Some 3DS structures

#pragma pack(1)
typedef struct {
	float r,g,b;
	} Color_f;

typedef struct {
	unsigned char r,g,b;
	} Color_24;

typedef struct {
	unsigned short tag;
	long size;
	} Chunk_hdr;

typedef struct {
	float x;
	float y;
	float z;
	unsigned short flags;
	} Verts;

typedef struct {
	float u;
	float v;
	} Texverts;

typedef struct {
	unsigned short a;
	unsigned short b;
	unsigned short c;
	unsigned char material;
	unsigned char filler;
	unsigned long sm_group;
	unsigned short flags;
	} Faces;

typedef struct
	{
	float lo_bias,hi_bias;
	short shadsize,shadsamp,shadrange;
	} Locshad;

typedef struct {
	float bias,shadfilter;
	short shadsize;
	} LocShad2;

#pragma pack()

// 3DS face edge vis flags
#define ABLINE (1<<2)
#define BCLINE (1<<1)
#define CALINE 1

#define VWRAP (1<<11)		/* Texture coord V wraps on this face */
#define UWRAP (1<<3)		/* Texture coord U wraps on this face */

// Node list structure
#define OBJ_MESH 0
#define OBJ_OMNILIGHT 1
#define OBJ_SPOTLIGHT 2
#define OBJ_CAMERA 3
#define OBJ_DUMMY 4
#define OBJ_TARGET 5
#define OBJ_OTHER  6 // generated from app data

// 3DS Key structures

#pragma pack(1)
#define KEYHDR  \
	TimeValue time;  \
	float tens,cont,bias; \
	float easeTo,easeFrom;

typedef struct { float p,ds,dd; } PosElem;
typedef struct {
	KEYHDR
	PosElem e[8]; /* enough to be bigger than the biggest key,
					including RotKey */
	} Key;

typedef struct {
	KEYHDR
	PosElem e[1];
	} ScalarKey;

typedef struct {
	KEYHDR
	PosElem e[3];
	} PosKey;	

typedef struct {
	KEYHDR
	float angle;	/* angle of rotation in radians (always >0) */
	float axis[3]; /* axis of rotation (unit vector) */
	float q[4];  	/* quaternion describing orientation */
	float b[4];		/* incoming tangent term */
	float a[4];		/* outgoing tangent term */
	} RotKey;

#ifdef LATER
typedef struct {
	KEYHDR
	Namedobj *object;  
	} MorphKey;	

typedef struct {
	KEYHDR
	} HideKey;
#endif // LATER

typedef struct {
	KEYHDR
	FLOAT c[3];
	} ColorKey;
#pragma pack()

// key types
#define KEY_FLOAT	0
#define KEY_POS		1
#define KEY_ROT		2
#define KEY_SCL		3
#define KEY_COLOR	4

#define NUMTRACKS 8

#define POS_TRACK_INDEX 0
#define ROT_TRACK_INDEX 1
#define SCL_TRACK_INDEX 2
#define FOV_TRACK_INDEX 3
#define ROLL_TRACK_INDEX 4
#define COL_TRACK_INDEX 5
#define HOT_TRACK_INDEX 6
#define FALL_TRACK_INDEX 7


typedef struct {
	union {
		Key key;
		PosKey pos;
		RotKey rot;
		ColorKey col;
		ScalarKey sc;
		} key;
	void *next;
	} KeyList;

// A list of 3DS objects with their names and types
typedef struct {
	void *object;
	Point3 srcPos;
	Point3 targPos;
	TSTR name;
	int type;
	int used;
	int cstShad;
	int rcvShad;
	int mtln;
	Matrix3 tm;
	void *next;
	} WkObjList;

// A list of the nodes and their various keys
typedef struct {
	ImpNode *node;
	short id;
	int type;
	int mnum;
	TSTR name;
	TSTR owner;
	Mesh *mesh;
	ImpNode *parent;
	Matrix3 tm;
	KeyList *posList;
	KeyList *rotList;
	KeyList *scList;
	KeyList *colList;
	KeyList *hotList;
	KeyList *fallList;
	KeyList *fovList;
	KeyList *rollList;
	SHORT trackFlags[NUMTRACKS];
	void *next;
	} WkNodeList;


/*--------- Track flags bits------------ */

/*-- This bit causes the spline to be cyclic */
#define ANIM_CYCLIC 1
/*-- This bit causes a track to continue "modulo" its duration */
#define ANIM_LOOP  (1<<1)
/*-- This bit is used by anim.c, but clients need not worry about it*/
#define ANIM_NEGWRAP (1<<2)

#define X_LOCKED (1<<3)
#define Y_LOCKED (1<<4)
#define Z_LOCKED (1<<5)
#define ALL_LOCKED (X_LOCKED|Y_LOCKED|Z_LOCKED)
#define TRACK_ATKEY (1<<6)

/* these flags specify which coords are NOT inherited from parent */ 
#define LNKSHFT 7
#define NO_LNK_X (1<<LNKSHFT)
#define NO_LNK_Y (1<<(LNKSHFT+1))
#define NO_LNK_Z (1<<(LNKSHFT+2))
#define LASTAXIS_SHFT 10
#define LASTAXIS_MASK (3<<LASTAXIS_SHFT)

// A worker object for dealing with creating the objects.
// Useful in the chunk-oriented 3DS file format

// Worker types

#define WORKER_IDLE		0
#define WORKER_MESH		1
#define WORKER_KF		2
#define WORKER_SHAPE	3
#define WORKER_LIGHT	4
#define WORKER_CAMERA	5

struct SMtl;

struct MtlName{
	char s[20];
	};

class ObjWorker {
public:
	int okay;
	ImpInterface *i;
	Interface *ip;
	TSTR name;
	int mode;
	int gotverts;
	int verts;
	int tverts;
	int gottverts;
	int gotfaces;
	int faces;
	int cstShad;
	int rcvShad;
	BOOL gotM3DMAGIC;
	Mtl  *sceneMtls[256];
	MtlName mtlNames[256];
	TriObject *object;
	Mesh *mesh;
	ImpNode *thisNode;
	ImpNode *parentNode;
	short id;
	WkObjList *objects;
	WkNodeList *nodes;
	WkNodeList *workNode;
	Matrix3 tm;
	Point3 pivot;
	DummyObject *dummy;
	int isDummy;
	int lightType;
	TSTR nodename;
	// Time stuff:
	BOOL lengthSet;
	TimeValue length;
	BOOL segmentSet;
	Interval segment;	
	SHORT trackFlags[NUMTRACKS];
	MtlList loadMtls;
	Tab<Mesh *> loadMtlMesh;
	void *appdata;
	DWORD appdataLen;
	Tab<UVVert>newTV;

	float hook_x, hook_y;
					ObjWorker(ImpInterface *iptr,Interface *ip);
					~ObjWorker() { FinishUp(); FreeObjList(); FreeNodeList(); i->RedrawViews(); }
	int				StartMesh(const char *name);
	int				StartLight(const char *name);
	int				CreateLight(int type);
	int				StartCamera(const char *name);
	int				CreateCamera(int type);
	int				StartKF(ImpNode *node);
	int				StartShape();
	int				StartSpline();
	int				AddShapePoint(Shppt *p);
	void			AddRagMtl(char *name, char *ragnapath, char *filename);
	int				CloseSpline();
	int				FinishShape();
	int				FinishUp();
	void			SetTm(Matrix3 *transform) { tm = *transform; }
	int				SetVerts(int count);
	int				SetTVerts(int count);
	int				GetVerts() { return verts; }
	int				SetFaces(int count);
	int				GetFaces() { return faces; }
	int				PutVertex(int index,Verts *v);
	int				PutTVertex(int index,UVVert *v);
	int				PutFace(int index,Faces *f);
	int				PutSmooth(int index,unsigned long smooth);
	int 			PutFaceMtl(int index, int imtl);
	void 			SetTVerts(int nf, Faces *f);
	DWORD 			AddNewTVert(UVVert p);
	void			Reset();
	void			Abandon();
	int				AddObject(Object *obj,int type,const TCHAR *name, Matrix3* tm, int mtlNum=-1);
	int				AddNode(ImpNode *node,const TCHAR *name,int type,Mesh *mesh,char *owner,int mtlNum=-1);
	int				SetNodeId(ImpNode *node,short id);
	int				SetNodesParent(ImpNode *node,ImpNode *parent);
	void *			FindObject(char *name, int &type, int &cstShad, int &rcvShad, int &mtlNum);
	int				UseObject(char *name);
	int				CompleteScene();
	int 			SetupEnvironment();
	ImpNode *		FindNode(char *name);
	ImpNode *		FindNodeFromId(short id);
	WkNodeList *	FindEntry(char *name);
	WkNodeList *	FindEntryFromId(short id);
	WkNodeList *	FindNodeListEntry(ImpNode *node);
	WkObjList *		FindObjListEntry(TSTR &name);
	void *			FindObjFromNode(ImpNode *node);
	int				FindTypeFromNode(ImpNode *node, Mesh **mesh);
	TCHAR *			NodeName(ImpNode *node);
	void			FreeObjList();
	void			FreeNodeList();
	ImpNode *		ThisNode() { return thisNode; }
	ImpNode *		ParentNode() { return parentNode; }
	void			SetParentNode(ImpNode *node) { parentNode = node; }
	void			SetPivot(Point3 p) { pivot = p; }
	int				AddPositionKey(PosKey *key) { return AddKey(&workNode->posList,(Key *)key); }
	int				AddRotationKey(RotKey *key) { return AddKey(&workNode->rotList,(Key *)key); }
	int				AddScaleKey(PosKey *key) { return AddKey(&workNode->scList,(Key *)key); }
	int				AddColorKey(ColorKey *key) { return AddKey(&workNode->colList,(Key *)key); }
	int				AddHotKey(ScalarKey *key) { return AddKey(&workNode->hotList,(Key *)key); }
	int				AddFallKey(ScalarKey *key) { return AddKey(&workNode->fallList,(Key *)key); }
	int				AddFOVKey(ScalarKey *key) { return AddKey(&workNode->fovList,(Key *)key); }
	int				AddRollKey(ScalarKey *key) { return AddKey(&workNode->rollList,(Key *)key); }
	int				AddKey(KeyList **list,Key *data);
	void			FreeKeyList(KeyList **list);
	int				SetTransform(ImpNode *node,Matrix3& m);
	Matrix3			GetTransform(ImpNode *node);
	int				ReadyDummy();
	ImpNode *		MakeDummy(const TCHAR *name);
	ImpNode *		MakeANode(const TCHAR *name, BOOL target,char *owner);
	void			SetDummy(int x) { isDummy = x; }
	int				IsDummy() { return isDummy; }
	int				SetDummyBounds(Point3& min,Point3& max);
	void			SetNodeName(const TCHAR *name) { nodename = name; }
	void			SetInstanceName(ImpNode *node, const TCHAR *iname);
	void			SetAnimLength(TimeValue l) { length = l; lengthSet = TRUE; }
	void			SetSegment(Interval seg) { segment = seg; segmentSet = TRUE; }
	void			SetControllerKeys(Control *cont,KeyList *keys,int type,float f=1.0f,float aspect=-1.0f);
	void            MakeControlsTCB(Control *tmCont,SHORT *tflags);
	Mtl*			GetMaxMtl(int i);
	void 			AssignMtl(INode *theINode, Mesh *mesh);
	void 			AssignMtl(WkNodeList* wkNode);
	int 			GetMatNum(char *name);
	void 			AddMeshMtl(SMtl *mtl);
	void 			FreeUnusedMtls();
	};

#endif // __RSMIMP__H
