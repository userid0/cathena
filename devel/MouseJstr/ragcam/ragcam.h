//Stuff added by Ender
//Rather messy right now...

#define _USEGRF
#define LIGHT_ENABLE
//#define _FORCE_EXTRACT

/* VK_0 thru VK_9 are the same as ASCII '0' thru '9' (0x30 - 0x39) */
#define VK_0	0x30
#define VK_1	0x31
#define VK_2	0x32
#define VK_3	0x33
#define VK_4	0x34
#define VK_5	0x35
#define VK_6	0x36
#define VK_7	0x37
#define VK_8	0x38
#define VK_9	0x39

/* VK_A thru VK_Z are the same as ASCII 'A' thru 'Z' (0x41 - 0x5A) */
#define VK_A	0x41
#define VK_B	0x42
#define VK_C	0x43
#define VK_D	0x44
#define VK_E	0x45
#define VK_F	0x46
#define VK_G	0x47
#define VK_H	0x48
#define VK_I	0x49
#define VK_J	0x4A
#define VK_K	0x4B
#define VK_L	0x4C
#define VK_M	0x4D
#define VK_N	0x4E
#define VK_O	0x4F
#define VK_P	0x50
#define VK_Q	0x51
#define VK_R	0x52
#define VK_S	0x53
#define VK_T	0x54
#define VK_U	0x55
#define VK_V	0x56
#define VK_W	0x57
#define VK_X	0x58
#define VK_Y	0x59
#define VK_Z	0x5A

#define VK_LSQUAREBRACKET	0xDB
#define VK_RSQUAREBRACKET	0xDD

float	waterlevel=15;
bool	b_fog=TRUE;
bool	b_ShowHelp=TRUE;
bool	b_Clip=FALSE;
bool	b_Wire=FALSE;
bool    bextract=FALSE;
bool    bMoveCamera = FALSE;


int showinfo = 1;
int editmode = 0;
int showhelp = 0;
#define MAXHELP 2

float fogd=0.0075f;

Grf *myGRF;
CCamera	g_Camera;
CFrustum g_Frustum;
AUX_RGBImageRec *background;
RSM *selectedobject;
RSM_Mesh *selectedmodel;
ro_position_t minipos;

GLuint DLid;
float g_LightPosition[4] = {0.0f, 0.0f, 0.0f, 1.0f};	

int mapw, maph;
float mapx = 0.0f, mapy = 0.0f, rota = 0.0f;


GLuint cursortex;
