/**********************************************************************
 *<
	FILE: RSMImp.cpp

	DESCRIPTION:	Appwizard generated plugin

	CREATED BY: 

	HISTORY: 

 *>	Copyright (c) 1997, All Rights Reserved.
 **********************************************************************/

#include "stdafx.h"
#include "RSMImp.h"
#include "3dsires.h"
#include "mtldef.h"
#include "istdplug.h"
#include "stdmat.h"
#include "gamma.h"
#include "helpsys.h"

#define IsVertexEqual(a,b) ((a.x == b.x) && (a.y == b.y) && (a.z == b.z))
#define HASH_TABLE_SIZE 133

char ragnapath[1024];

//Grf *myGRF;

static BOOL showPrompts;

static int MessageBox(int s1, int s2, int option = MB_OK) {
	TSTR str1(GetString(s1));
	TSTR str2(GetString(s2));
	return MessageBox(GetActiveWindow(), str1, str2, option);
	}

static int MessageBox2(int s1, int s2, int option = MB_OK) {
	TSTR str1(GetString(s1));
	TSTR str2(GetString(s2));
	return MessageBox(GetActiveWindow(), str1, str2, option);
	}

static int MessageBox(TCHAR *s, int s2, int option = MB_OK) {
	TSTR str1(s);
	TSTR str2(GetString(s2));
	return MessageBox(GetActiveWindow(), str1, str2, option);
	}


// 3DS-to-MAX time constant multiplier
#define TIME_CONSTANT GetTicksPerFrame()

//#define DBGPRINT

// The file stream
static FILE *stream;

// The debugging dump stream

static FILE *dStream;

// Some stuff we're defining (for now)
static int merging = 0;
static int lastchunk;
#define OBJ_NAME_LEN 10
static char obname[32];
static int nodeLoadNumber = -1;
static short cur_node_id,nodeflags,nodeflags2;
static int nodetag;
static int skipNode = 0;
static ImpNode *obnode;
static short readVers;
static int skipped_nodes = 0;
static int got_mat_chunk;
static BGdata BG;
static BOOL replaceScene =0;
static BOOL autoConv = 1;
static BOOL importShapes = TRUE;
static BOOL needShapeImportOptions = FALSE;
static BOOL shapeImportAbort = FALSE;

// Some handy macros
static float msc_wk;

void fin_degammify(Color *col, Color *gamcol) {
	if (gammaMgr.enable) {
		col->r = deGammaCorrect(gamcol->r, gammaMgr.fileInGamma);
		col->g = deGammaCorrect(gamcol->g, gammaMgr.fileInGamma);
		col->b = deGammaCorrect(gamcol->b, gammaMgr.fileInGamma);
		}
	else *col = *gamcol;
	}

static Color ColorFrom24(Color_24 c) {
	Color a;
	a.r = (float)c.r/255.0f;
	a.g = (float)c.g/255.0f;
	a.b = (float)c.b/255.0f;
	return a;
	}

//==========================================================================
// Setup Environment
//==========================================================================
int ObjWorker::SetupEnvironment() {
	if (!replaceScene) 
		return 1;
	i->SetAmbient(0,BG.amb_light);	
	i->SetBackGround(0,BG.bkgd_solid);	
	i->SetUseMap(FALSE);
	switch(BG.bgType) {
		case BG_SOLID: 
			break;
		case BG_GRADIENT: 
			{
			GradTex *gt = NewDefaultGradTex();
			gt->SetColor(0, BG.bkgd_gradient.topcolor);
			gt->SetColor(1, BG.bkgd_gradient.midcolor);
			gt->SetColor(2, BG.bkgd_gradient.botcolor);
			gt->SetMidPoint(1.0f-BG.bkgd_gradient.midpct);
			gt->GetUVGen()->SetCoordMapping(UVMAP_SCREEN_ENV);
			i->SetEnvironmentMap(gt);
			i->SetUseMap(TRUE);
			}
			break;
		case BG_BITMAP: 
			if (strlen(BG.bkgd_map)>0) {
				BitmapTex *bmt = NewDefaultBitmapTex();
				bmt->SetMapName(TSTR(BG.bkgd_map));
				bmt->GetUVGen()->SetCoordMapping(UVMAP_SCREEN_ENV);
				i->SetEnvironmentMap(bmt);
				i->SetUseMap(TRUE);
				}
			break;
		}	
	switch(BG.envType) {
		case ENV_DISTCUE:
			//	BG.distance_cue.nearplane; ??
			//	BG.distance_cue.farplane;  ??
			{
			StdFog *fog = NewDefaultStdFog();
			fog->SetType(0);  
			fog->SetColor(Color(0.0f,0.0f,0.0f),0);  
			fog->SetNear(BG.distance_cue.neardim/100.0f,0);  
			fog->SetFar(BG.distance_cue.fardim/100.0f,0);  
			fog->SetFogBackground(BG.dim_bg);
			i->AddAtmosphere(fog);			
			}				
			break;
		case ENV_FOG:{
			StdFog *fog = NewDefaultStdFog();
			fog->SetType(0);  
			fog->SetColor(BG.fog_data.color,0);  
			fog->SetNear(BG.fog_data.neardens/100.0f,0);  
			fog->SetFar(BG.fog_data.fardens/100.0f,0);  
			fog->SetFogBackground(BG.fog_bg);
			i->AddAtmosphere(fog);			
			}
			break;
		case ENV_LAYFOG:{
			StdFog *fog = NewDefaultStdFog();
			fog->SetType(1);  
			int ftype;
			switch(BG.lfog_data.type) {
				case 1:  ftype = FALLOFF_BOTTOM; break;
				case 2:  ftype = FALLOFF_TOP; break;
				default: ftype = FALLOFF_NONE; break;
				}
			fog->SetFalloffType(ftype);  
			fog->SetColor(BG.lfog_data.color,0);  
			fog->SetDensity(BG.lfog_data.density*100.0f,0);  
			fog->SetTop(BG.lfog_data.zmax,0);  
			fog->SetBottom(BG.lfog_data.zmin,0);  
			fog->SetFogBackground(BG.lfog_data.fog_bg);
			i->AddAtmosphere(fog);			
			}
			break;
		}
	i->SetAmbient(0,BG.amb_light);
	return 1;
	}

static int MAXMapIndex(int i) {
	switch(i) {
		case Ntex:  return ID_DI;
		case Ntex2: return ID_DI;
		case Nopac: return ID_OP;
		case Nbump: return ID_BU;
		case Nspec: return ID_SP;
		case Nshin: return ID_SS;
		case Nselfi:return ID_SI; 
		case Nrefl: return ID_RL;
		default:    return ID_DI;
		}
	}

//=========================================================
// Match 3DStudio's "Default" material
static StdMat *New3DSDefaultMtl() {
	StdMat *m = NewDefaultStdMat();
	m->SetName(_T("Default"));
	m->SetAmbient(Color(.7f,.7f,.7f),0);
	m->SetDiffuse(Color(.7f,.7f,.7f),0);
	m->SetSpecular(Color(1.0f,1.0f,1.0f),0);
	m->SetShininess(0.5f,0);
	m->SetShinStr(.707f,0);
	return m;
	}

static StdMat *MakeMtl(char *name) {
	StdMat *m = NewDefaultStdMat();
	m->SetName(_T(name));
	m->SetAmbient(Color(.7f,.7f,.7f),0);
	m->SetDiffuse(Color(.7f,.7f,.7f),0);
	m->SetSpecular(Color(1.0f,1.0f,1.0f),0);
	m->SetShininess(0.5f,0);
	m->SetShinStr(.707f,0);
	return m;
	}


static BOOL IsSXPName(char *name) {
	char fname[30];
	char ext[5];
	_splitpath(name, NULL, NULL, fname, ext );
	return stricmp(ext,".sxp")==0?1:0;
	}

static Texmap* MakeTex(MapData& map, SMtl *smtl, BOOL &wasSXP) {
	Texmap *txm; 
	wasSXP = FALSE;
	if (map.kind==0) { 
		// Texture Map
//		if (IsSXPName(map.name)&&(map.p.tex.sxp_data!=NULL)) {
		if (IsSXPName(map.name)) {	 // DS - 6/11/96
			Tex3D *t3d = GetSXPReaderClass(map.name);
			if (t3d) {
				if (map.p.tex.sxp_data) {
					ULONG *p = (ULONG *)map.p.tex.sxp_data;
					t3d->ReadSXPData(map.name, (void *)(p+1));
					wasSXP = TRUE;
					}
				}
			txm = t3d;							
			}
		else {
			BitmapTex *bmt = NewDefaultBitmapTex();
			bmt->SetMapName(TSTR(map.name));
			MapParams &par = map.p.tex;
			bmt->SetAlphaAsMono((par.texflags&TEX_ALPHA_SOURCE)?1:0);
			bmt->SetAlphaSource((par.texflags&TEX_DONT_USE_ALPHA)?ALPHA_NONE:ALPHA_FILE);
			bmt->SetFilterType((par.texflags&TEX_SAT)?FILTER_SAT:FILTER_PYR);
			StdUVGen *uv = bmt->GetUVGen();
			uv->SetUOffs(par.uoffset,0);
			uv->SetVOffs(-par.voffset,0);
			uv->SetUScl(par.uscale,0);
			uv->SetVScl(par.vscale,0);
			uv->SetAng(-((float)atan2(par.ang_sin, par.ang_cos)),0);
			uv->SetBlur(par.texblur+1.0f,0);
			int tile=0;
			if (par.texflags&TEX_MIRROR) tile|= U_MIRROR|V_MIRROR;
			else {
				if (0==(par.texflags&TEX_NOWRAP)) tile|= U_WRAP|V_WRAP;
				}
			uv->SetTextureTiling(tile);
			TextureOutput *txout = bmt->GetTexout();
			txout->SetInvert(par.texflags&TEX_INVERT?1:0);
			txm = bmt;
			}
		if (map.p.tex.texflags&TEX_TINT) {
			// map.p.tex.col1, col2	: stuff into Mix	
			MultiTex* mix = NewDefaultMixTex();
			mix->SetColor(0,ColorFrom24(map.p.tex.col1));
			mix->SetColor(1,ColorFrom24(map.p.tex.col2));
			mix->SetSubTexmap(2, txm);
			txm = mix;			
			}
		else if (map.p.tex.texflags&TEX_RGB_TINT) {
			// map.p.tex.rcol,gcol,bcol : stuf into tint
			MultiTex* mix = NewDefaultTintTex();
			mix->SetColor(0,ColorFrom24(map.p.tex.rcol));
			mix->SetColor(1,ColorFrom24(map.p.tex.gcol));
			mix->SetColor(2,ColorFrom24(map.p.tex.bcol));
			mix->SetSubTexmap(0, txm);
			txm = mix;			
			}
		}
	else {  
		// kind == 1 :  Reflection Map
		BitmapTex *bmt = NewDefaultBitmapTex();
		bmt->SetMapName(TSTR(map.name));
		StdUVGen *uv = bmt->GetUVGen();

		// TBD: REFLECTION BLUR SETTING:
		uv->SetBlurOffs((float)smtl->refblur/400.0f+.001f,0);
		bmt->InitSlotType(MAPSLOT_ENVIRON);
		txm = bmt;
		}


	return txm;
	}

//==========================================================================
// Convert mesh mtl to max standard matl.
//==========================================================================
void ObjWorker::AddMeshMtl(SMtl *smtl) {
	StdMat *m;
	Mesh *mesh = NULL;
	if (smtl==NULL) {
		m = New3DSDefaultMtl();
		loadMtls.AddMtl(m);			
		loadMtlMesh.Append(1,&mesh,10);
		return;
		}
	m = NewDefaultStdMat();
	m->SetName(TSTR(smtl->name));
	int shade;
	switch(smtl->shading) {
		case REND_FLAT:  shade = SHADE_CONST; break;
		case REND_METAL: shade = SHADE_METAL; break;
		default:		 shade = SHADE_PHONG; break;
		}
	m->SetShading(shade);
	m->SetAmbient(ColorFrom24(smtl->amb),0);
	m->SetDiffuse(ColorFrom24(smtl->diff),0);
	m->SetFilter(ColorFrom24(smtl->diff),0);
	m->SetSpecular(ColorFrom24(smtl->spec),0);
	m->SetShininess((float)smtl->shininess/100.0f,0);
	m->SetShinStr((float)smtl->shin2pct/100.0f,0);
	m->SetOpacity(1.0f-(float)smtl->transparency/100.0f,0);
	m->SetOpacFalloff((float)smtl->xpfall/100.0f, 0);		
	m->SetFalloffOut(smtl->flags&MF_XPFALLIN?0:1);  
	m->SetSelfIllum((float)smtl->selfipct/100.0f,0);
	m->SetWireSize(smtl->wiresize,0);
	m->SetFaceMap(smtl->flags&MF_FACEMAP?1:0);
	m->SetSoften(smtl->flags&MF_PHONGSOFT?1:0);
	m->SetWire(smtl->flags&MF_WIRE?1:0);
	m->SetTwoSided(smtl->flags&MF_TWOSIDE?1:0);
	m->SetTransparencyType(smtl->flags&MF_ADDITIVE ? TRANSP_ADDITIVE : TRANSP_FILTER);
	m->SetWireUnits(smtl->flags&MF_WIREABS?1:0);


	if (smtl->map) {
		Texmap *txm;
		float amt,amt0;
		BOOL gotTex=0;
		BOOL dum;
		for (int i=0; i<8; i++) {
			if (smtl->map[i]==NULL) 
				continue;
			Mapping &mp = *(smtl->map[i]);
			int n = MAXMapIndex(i);
			if (i==Nrefl) {
				amt = (float)mp.amt.pct/100.0f;
				RMapParams &par = mp.map.p.ref;
				if (par.acb.flags&AC_ON) {	
					// Mirror or Auto-cubic
					if (par.acb.flags&AC_MIRROR) {
						StdMirror *mir = NewDefaultStdMirror();
						txm = (Texmap *)mir;
						mir->SetDoNth(par.acb.flags&AC_FIRSTONLY?0:1);
						mir->SetNth(par.acb.nth);
						}
					else {
						StdCubic *cub = NewDefaultStdCubic();
						txm = (Texmap *)cub;
						cub->SetSize(par.acb.size,0);
						cub->SetDoNth(par.acb.flags&AC_FIRSTONLY?0:1);
						cub->SetNth(par.acb.nth);
						}
					}
				else {	
					// Environment map
					txm = MakeTex(mp.map,smtl,dum);
					}

				if (strlen(mp.mask.name)>0) {
					// make a Mask texmap.
					Texmap *masktex = (Texmap *)CreateInstance(TEXMAP_CLASS_ID, Class_ID(MASK_CLASS_ID,0));
					masktex->SetSubTexmap(1,MakeTex(mp.mask,smtl,dum));
					masktex->SetSubTexmap(0,txm);
					txm = masktex;
					}

				m->SetSubTexmap(n,txm);
				amt = (float)mp.amt.pct/100.0f;
				m->SetTexmapAmt(n, amt, 0);
				}
			else {
					
				// non-reflection maps
				amt = (float)mp.amt.pct/100.0f;

				// DS: 4/30/97 correct for new interpretation of the
				// amount sliders.
				switch(i) {
					case Nopac:	 
						if (amt<1.0f) 
							m->SetOpacity(0.0f,0); 
						break;
					case Nselfi: 
						if (amt<1.0f) 
							m->SetSelfIllum(0.0f,0); 
						break;
					case Nshin:  
						// Shininess mapping in 3DS was really shininess strength mapping
						amt*= (float)smtl->shin2pct/100.0f;
						m->SetShinStr(0.0f,0);
						break;
					}

				BOOL wasSXP;
				txm = MakeTex(mp.map,smtl,wasSXP);
				if (n==ID_BU&&!wasSXP) amt *= 10.0f;
				m->SetTexmapAmt(n, amt, 0);
				if (strlen(mp.mask.name)>0) {
					// make a Mask texmap.
					Texmap *masktex = (Texmap *)CreateInstance(TEXMAP_CLASS_ID, Class_ID(MASK_CLASS_ID,0));
					masktex->SetSubTexmap(1,MakeTex(mp.mask,smtl,dum));
					masktex->SetSubTexmap(0,txm);
					txm = masktex;
					}
				if (i==Ntex2) {
					if (gotTex) {
						// Make a Composite texmap
						MultiTex *comp = NewDefaultCompositeTex();
						comp->SetNumSubTexmaps(2);
						Texmap *tm0 = m->GetSubTexmap(ID_DI);
						comp->SetSubTexmap(0,tm0);
						comp->SetSubTexmap(1,txm);
						m->SetSubTexmap(ID_DI,comp);						
						if (tm0) 
							tm0->SetOutputLevel(0,amt0);
						if (txm)
							txm->SetOutputLevel(0,amt);
						m->SetTexmapAmt(ID_DI,1.0f,0);
						}
					else {
						m->SetSubTexmap(ID_DI,txm);						
						m->SetTexmapAmt(n,amt,0);
						}
					} 
				else 
					m->SetSubTexmap(n,txm);
				if (i==Ntex&&txm) {
					gotTex=1;
					amt0 = amt;
					}
				}
			m->EnableMap(n,mp.use);
			}
		}
	loadMtls.AddMtl(m);			
	loadMtlMesh.Append(1,&mesh,10);
	}


void ObjWorker::AddRagMtl(char *name, char *ragnapath, char *filename) {
	StdMat *m;
	char buffer[256];
	Mesh *mesh = NULL;
	m = NewDefaultStdMat();
	m->SetName(TSTR(name));
	int shade;
    shade = SHADE_BLINN; 
	Color a;
	a.r = 0; a.g = 0; a.b = 0;

	m->SetShading(shade);
	m->SetAmbient(a,0);
	
	a.r = 192; a.g = 192; a.b = 192;
	m->SetDiffuse(a,0);
	m->SetFilter(a,0);

	a.r = 255; a.g = 255; a.b = 255;

//	m->SetSpecular(a,0);
//	m->SetShininess(a,0);
//	m->SetShinStr((float)smtl->shin2pct/100.0f,0);
//	m->SetOpacity(1.0f-(float)smtl->transparency/100.0f,0);
//	m->SetOpacFalloff((float)smtl->xpfall/100.0f, 0);		
//	m->SetFalloffOut(smtl->flags&MF_XPFALLIN?0:1);  
//	m->SetSelfIllum((float)smtl->selfipct/100.0f,0);
//	m->SetWireSize(smtl->wiresize,0);
//	m->SetFaceMap(smtl->flags&MF_FACEMAP?1:0);
//	m->SetSoften(0);
//	m->SetWire(smtl->flags&MF_WIRE?1:0);
//	m->SetTwoSided(smtl->flags&MF_TWOSIDE?1:0);
//	m->SetTransparencyType(smtl->flags&MF_ADDITIVE ? TRANSP_ADDITIVE : TRANSP_FILTER);
//	m->SetWireUnits(smtl->flags&MF_WIREABS?1:0);

	sprintf(buffer,"%s\\data\\texture\\%s",ragnapath,filename);

	m->EnableMap(ID_DI,1);
	
	BitmapTex *bmt = NewDefaultBitmapTex();
	bmt->SetMapName(TSTR(buffer));
//	MapParams &par = map.p.tex;
	bmt->SetAlphaAsMono(0);
	bmt->SetAlphaSource(ALPHA_NONE);
	bmt->SetFilterType(FILTER_SAT);


//	bmt->SetAlphaAsMono((par.texflags&TEX_ALPHA_SOURCE)?1:0);
//	bmt->SetAlphaSource((par.texflags&TEX_DONT_USE_ALPHA)?ALPHA_NONE:ALPHA_FILE);
//	bmt->SetFilterType((par.texflags&TEX_SAT)?FILTER_SAT:FILTER_PYR);

	StdUVGen *uv = bmt->GetUVGen();

//	uv->SetUOffs(par.uoffset,0);
//	uv->SetVOffs(-par.voffset,0);
//	uv->SetUScl(par.uscale,0);
//	uv->SetVScl(par.vscale,0);
//	uv->SetAng(-((float)atan2(par.ang_sin, par.ang_cos)),0);
//	uv->SetBlur(par.texblur+1.0f,0);
//	int tile=0;
//	if (par.texflags&TEX_MIRROR) tile|= U_MIRROR|V_MIRROR;
//	else {
//		if (0==(par.texflags&TEX_NOWRAP)) tile|= U_WRAP|V_WRAP;
//		}
//	uv->SetTextureTiling(tile);
//	TextureOutput *txout = bmt->GetTexout();
//	txout->SetInvert(par.texflags&TEX_INVERT?1:0);

	m->SetSubTexmap(ID_DI,bmt);
	m->SetTexmapAmt(ID_DI, 1.0, 0);
	


/*	
	char filefname[64];
	char* fileext;

	fileext = strchr(filename,'.')+1;
	strncpy(filefname,filename,fileext-filename);
	filefname[fileext-filename-2] = '\0';

	sprintf(buffer,"%s\\data\\texture\\%s mask.%s",ragnapath,filefname,fileext);


	FILE *myFile;
	if((myFile=fopen(buffer,"r"))!=NULL)
	{
		fclose(myFile);
		m->EnableMap(ID_OP,1);
		BitmapTex *bmt2 = NewDefaultBitmapTex();
		bmt2->SetMapName(TSTR(buffer));
		bmt2->SetAlphaAsMono(0);
		bmt2->SetAlphaSource(ALPHA_NONE);
		bmt2->SetFilterType(FILTER_SAT);

		m->SetSubTexmap(ID_OP,bmt2);
		m->SetTexmapAmt(ID_OP, 1.0, 0);
	}
*/

/*	if (smtl->map) {
		Texmap *txm;
		float amt,amt0;
		BOOL gotTex=0;
		BOOL dum;
		for (int i=0; i<8; i++) {
			if (smtl->map[i]==NULL) 
				continue;
			Mapping &mp = *(smtl->map[i]);
			int n = MAXMapIndex(i);
			if (i==Nrefl) {
				amt = (float)mp.amt.pct/100.0f;
				RMapParams &par = mp.map.p.ref;
				if (par.acb.flags&AC_ON) {	
					// Mirror or Auto-cubic
					if (par.acb.flags&AC_MIRROR) {
						StdMirror *mir = NewDefaultStdMirror();
						txm = (Texmap *)mir;
						mir->SetDoNth(par.acb.flags&AC_FIRSTONLY?0:1);
						mir->SetNth(par.acb.nth);
						}
					else {
						StdCubic *cub = NewDefaultStdCubic();
						txm = (Texmap *)cub;
						cub->SetSize(par.acb.size,0);
						cub->SetDoNth(par.acb.flags&AC_FIRSTONLY?0:1);
						cub->SetNth(par.acb.nth);
						}
					}
				else {	
					// Environment map
					txm = MakeTex(mp.map,smtl,dum);
					}

				if (strlen(mp.mask.name)>0) {
					// make a Mask texmap.
					Texmap *masktex = (Texmap *)CreateInstance(TEXMAP_CLASS_ID, Class_ID(MASK_CLASS_ID,0));
					masktex->SetSubTexmap(1,MakeTex(mp.mask,smtl,dum));
					masktex->SetSubTexmap(0,txm);
					txm = masktex;
					}

				m->SetSubTexmap(n,txm);
				amt = (float)mp.amt.pct/100.0f;
				m->SetTexmapAmt(n, amt, 0);
				}
			else {
					
				// non-reflection maps
				amt = (float)mp.amt.pct/100.0f;

				// DS: 4/30/97 correct for new interpretation of the
				// amount sliders.
				switch(i) {
					case Nopac:	 
						if (amt<1.0f) 
							m->SetOpacity(0.0f,0); 
						break;
					case Nselfi: 
						if (amt<1.0f) 
							m->SetSelfIllum(0.0f,0); 
						break;
					case Nshin:  
						// Shininess mapping in 3DS was really shininess strength mapping
						amt*= (float)smtl->shin2pct/100.0f;
						m->SetShinStr(0.0f,0);
						break;
					}

				BOOL wasSXP;
				txm = MakeTex(mp.map,smtl,wasSXP);
				if (n==ID_BU&&!wasSXP) amt *= 10.0f;
				m->SetTexmapAmt(n, amt, 0);
				if (strlen(mp.mask.name)>0) {
					// make a Mask texmap.
					Texmap *masktex = (Texmap *)CreateInstance(TEXMAP_CLASS_ID, Class_ID(MASK_CLASS_ID,0));
					masktex->SetSubTexmap(1,MakeTex(mp.mask,smtl,dum));
					masktex->SetSubTexmap(0,txm);
					txm = masktex;
					}
				if (i==Ntex2) {
					if (gotTex) {
						// Make a Composite texmap
						MultiTex *comp = NewDefaultCompositeTex();
						comp->SetNumSubTexmaps(2);
						Texmap *tm0 = m->GetSubTexmap(ID_DI);
						comp->SetSubTexmap(0,tm0);
						comp->SetSubTexmap(1,txm);
						m->SetSubTexmap(ID_DI,comp);						
						if (tm0) 
							tm0->SetOutputLevel(0,amt0);
						if (txm)
							txm->SetOutputLevel(0,amt);
						m->SetTexmapAmt(ID_DI,1.0f,0);
						}
					else {
						m->SetSubTexmap(ID_DI,txm);						
						m->SetTexmapAmt(n,amt,0);
						}
					} 
				else 
					m->SetSubTexmap(n,txm);
				if (i==Ntex&&txm) {
					gotTex=1;
					amt0 = amt;
					}
				}
			m->EnableMap(n,mp.use);
			}
		}
*/
	loadMtls.AddMtl(m);			
	loadMtlMesh.Append(1,&mesh,10);
	this->ip->ActivateTexture(bmt,m,-1);
}


Mtl *ObjWorker::GetMaxMtl(int i) {
	if (i<0) return NULL;
	if (i>=loadMtls.Count()) return NULL;
	return loadMtls[i];
	}

int ObjWorker::GetMatNum(char *name) {
	TSTR s(name);
	for (int i=0; i<loadMtls.Count(); i++) {
		if (_tcscmp(s,loadMtls[i]->GetName())==0)
			return i;
		}
	assert(0);
	return 0;
	}

void ObjWorker::AssignMtl(INode *theINode, Mesh *mesh) {
	short used[256],cused[256],remap[256];
	int i;

	// First check if another instance of this mesh has already
	// had a material assigned to it.
	for (i=0; i<loadMtlMesh.Count(); i++) {
		if (loadMtlMesh[i]==mesh) {
			theINode->SetMtl(loadMtls[i]);
			return ;
			}
		}
	for (i=0; i<256; i++) used[i] = 0;
	// See if a multi-mtl is required.
	int nmtl,numMtls=0;
	for (i =0; i<mesh->numFaces; i++) {
		nmtl = mesh->faces[i].getMatID();
		assert(nmtl<256);
		if (!used[nmtl]) {
			used[nmtl] = 1;
			remap[nmtl] = numMtls;
			cused[numMtls++] = nmtl;
			}
		}
	if (numMtls>1) { 
		// Need a Multi-mtl
		// scrunch the numbers down to be local to this multi-mtl
		for (i =0; i<mesh->numFaces; i++) 	{
			Face &f = mesh->faces[i];
			int id = f.getMatID();
			f.setMatID(remap[id]);
			}
		// create a new multi with numMtls, and set them
		// to GetMaxMtl(cused[i]), i==0..numMtls-1
		MultiMtl *newmat = NewDefaultMultiMtl();
		newmat->SetNumSubMtls(numMtls);
		for (i=0; i<numMtls; i++) 
			newmat->SetSubMtl(i,GetMaxMtl(cused[i]));
		theINode->SetMtl(newmat);
		loadMtls.AddMtl(newmat);			
		loadMtlMesh.Append(1,&mesh,10);
		}
	else {
		if (mesh->getNumFaces()) {
			nmtl = mesh->faces[0].getMatID();
			for (i =0; i<mesh->numFaces; i++) 
				mesh->faces[i].setMatID(0);
			theINode->SetMtl(GetMaxMtl(nmtl));
			loadMtlMesh[nmtl] = mesh;
			}
		}
	}


int CountRefs(ReferenceTarget *rt) {
	DependentIterator di(rt);
	int nrefs = 0;
	RefMakerHandle rm;
	while (NULL!=(rm=di.Next())) 
		nrefs++;
	return nrefs;			
	}

void ObjWorker::FreeUnusedMtls() {
	for (int i=0; i<loadMtls.Count(); i++) {
		if (CountRefs(loadMtls[i])==0) {
			loadMtls[i]->DeleteThis();
			}			
		}
	loadMtls.SetCount(0);
	}



//===========================================================
// ObjWorker class

ObjWorker::ObjWorker(ImpInterface *iptr,Interface *ip) {
	okay=1;
	i=iptr;
	objects=NULL;
	nodes=NULL;
	dummy=NULL;
	isDummy=0;
	Reset();
	lengthSet = FALSE;
	segmentSet = FALSE;
	appdata = NULL;
	appdataLen = 0;
	gotM3DMAGIC=FALSE;
	this->ip = ip;
	for (int i=0;i<256;i++) {
		sceneMtls[i] = NULL;
		mtlNames[i].s[0] = 0;
		}
	hook_x = hook_y = 0.0f;
	}

void
ObjWorker::Reset() {
	mode=WORKER_IDLE;
	tm.IdentityMatrix();
	gotverts=gottverts=gotfaces=verts=faces=0;
	id = -1;
	object=NULL;
	dummy = NULL;
	isDummy = 0;
	mesh=NULL;
	parentNode = thisNode = NULL;
	// NEW INITS
	workNode = NULL;	
	pivot = Point3(0,0,0);
	cstShad = 1;
	rcvShad = 1;
	newTV.SetCount(0);
	}

int
ObjWorker::StartMesh(const char *iname) {
	if(!FinishUp())
		return 0;
//DebugPrint("Starting mesh %s\n",iname);
	if(dStream) {
		fprintf(dStream,"Starting mesh:%s\n",iname);
		fflush(dStream);
		}
	tm.IdentityMatrix();
	name = TSTR(iname);
	object = CreateNewTriObject();
	if(!object)
		return 0;
	mesh = &object->GetMesh();
	mode = WORKER_MESH;
	newTV.SetCount(0);
	return 1;
	}

/*
ObjWorker::StartKF(ImpNode *node) {
	if(!FinishUp())
		return 0;
	if(dStream) {
		fprintf(dStream,"Starting KF node\n");
		fflush(dStream);
		}
	thisNode = node;
	parentNode = NULL;
	mode = WORKER_KF;
	pivot = Point3(0,0,0);	// Default pivot
	return 1;
	}
*/

typedef unsigned short ushort;

int
ObjWorker::FinishUp() {
	switch(mode) {
		case WORKER_IDLE:
			return 1;	// Nothing to do, return success!
		case WORKER_MESH: {
			
			// Must have defined verts!
			if(gotverts) {

				// Make this an inverse of the mesh matrix.  This sets up a transform which will
				// be used to transform the mesh from the 3DS Editor-space to the neutral space
				// used by the keyframer.
#ifdef DBGPRINT
DebugPrint("WORKER_MESH TM:\n");
for(int i = 0; i < 4; ++i) {
	Point3 p = tm.GetRow(i);
	DebugPrint("%.4f %.4f %.4f\n",p.x,p.y,p.z);
	}
#endif	
				Matrix3 itm = Inverse(tm);
#ifdef DBGPRINT
DebugPrint("WORKER_MESH inverse TM:\n");
for(i = 0; i < 4; ++i) {
	Point3 p = tm.GetRow(i);
	DebugPrint("%.4f %.4f %.4f\n",p.x,p.y,p.z);
	}
#endif	
				int ix;	
		
				// Transform verts through the inverted mesh transform
				for(ix=0; ix<verts; ++ix) {
					Point3 &p = mesh->getVert(ix);
					p = p * itm;
					mesh->setVert(ix,p);
					}

				/* Check for objects that have been flipped: their 3D
					"parity" will be off */    
				Point3 cp = CrossProd(tm.GetRow(0),tm.GetRow(1));
				if (DotProd(cp,tm.GetRow(2))<0) {
					Matrix3 tmFlipX(1);
					Point3 row0 = tmFlipX.GetRow(0);
					row0.x = -1.0f;
					tmFlipX.SetRow(0,row0);
					// Transform verts through the mirror transform
					for(ix=0; ix<verts; ++ix) {
						Point3 &p = mesh->getVert(ix);
						p = p * tmFlipX;
						mesh->setVert(ix,p);
						}
					}

				mesh->buildNormals();
				mesh->EnableEdgeList(1);				
				}

			if(gottverts) {
				int ntv;
				if ((ntv=newTV.Count())>0) {
					int oldn = mesh->numTVerts;
					mesh->setNumTVerts(oldn+ntv, TRUE); 
					for (int i=0; i<ntv; i++)
						mesh->tVerts[oldn+i] = newTV[i];
					}
				}


			AddObject(object,OBJ_MESH,name,&tm);
			if(dStream) {
				fprintf(dStream,"Adding object %s\n",CStr(name));
				fflush(dStream);
				}
			Reset();
			return 1;
			}

		case WORKER_KF: {
			
			if(dStream) {
				fprintf(dStream,"Finalizing %s\n",CStr(NodeName(thisNode)));
				fflush(dStream);
				}

			int type = FindTypeFromNode(thisNode, &mesh);

			if(parentNode) {
				if(dStream)
					{
					fprintf(dStream,"Attaching %s to %s\n",CStr(NodeName(thisNode)),CStr(NodeName(parentNode)));
					fflush(dStream);
					}
				parentNode->GetINode()->AttachChild(thisNode->GetINode());
				}

 			switch(type) {
				case OBJ_MESH:
				case OBJ_DUMMY:
				case OBJ_TARGET:
				case OBJ_OTHER:
					thisNode->SetPivot(-pivot);
					break;
				}
			Reset();
			return 1;
			}
		}

	// Undefined state!!!
	return 0;
	}


int
ObjWorker::SetDummyBounds(Point3& dMin,Point3& dMax) {
   	if(!isDummy)
		return 0;

	dummy->SetBox(Box3(dMin,dMax));	

#if 0
   	float dummyVertX[] =
		{(float)dMin.x,(float)dMax.x,(float)dMax.x,(float)dMin.x,(float)dMin.x,(float)dMax.x,(float)dMax.x,(float)dMin.x};
	float dummyVertY[] =
		{(float)dMin.y,(float)dMin.y,(float)dMax.y,(float)dMax.y,(float)dMin.y,(float)dMin.y,(float)dMax.y,(float)dMax.y};
	float dummyVertZ[] =
		{(float)dMin.z,(float)dMin.z,(float)dMin.z,(float)dMin.z,(float)dMax.z,(float)dMax.z,(float)dMax.z,(float)dMax.z};

	int ix;

	Mesh *mesh = &dummy->Mesh();

	// Stuff verts
	for(ix=0; ix<8; ++ix)
		mesh->setVert(ix,Point3(dummyVertX[ix],dummyVertY[ix],dummyVertZ[ix]));

	mesh->buildNormals();
#endif
	return 1;
	}

void
ObjWorker::SetInstanceName(ImpNode *node, const TCHAR *iname) {
	TSTR instancename(iname);
	if(!nodename.Length() && instancename.Length())
		node->SetName(iname);
	else
	if(!instancename.Length() && nodename.Length())
		node->SetName(nodename);
	else {
		TSTR fullname = nodename + TSTR(_T(".")) + iname;
		node->SetName(fullname);
		}
	}

int
ObjWorker::ReadyDummy() {
	if(dummy)
		return 1;		// Already exists!

	dummy = new DummyObject();
	dummy->SetBox(Box3(
		-Point3(0.5f,0.5f,0.5f),
		 Point3(0.5f,0.5f,0.5f)));

#if 0
	static int dummyFaceA[] =
		{0,2,0,5,1,6,2,7,3,4,4,6};
	static int dummyFaceB[] =
		{3,1,1,4,2,5,3,6,0,7,5,7};
	static int dummyFaceC[] =
		{2,0,5,0,6,1,7,2,4,3,6,4};

	// Create a triobj (for now)
	TriObject *object = CreateNewTriObject();
	if(!object)
		return 0;
	mesh = &object->Mesh();

	if(!mesh->setNumVerts(8)) {
		delete object;
		return 0;
		}
	if(!mesh->setNumFaces(12)) {
		delete object;
		return 0;
		}

	dummy = object;

	int ix;

	// Stuff faces
	for(ix=0; ix<12; ++ix) {
		Face *fp = &mesh->faces[ix];
		fp->setVerts(dummyFaceA[ix],dummyFaceB[ix],dummyFaceC[ix]);
		fp->setEdgeVisFlags(1,1,0);
		fp->setSmGroup(1 << (ix/2));
		}

	// Stuff verts
	SetDummyBounds(Point3((float)-0.5,(float)-0.5,(float)-0.5),Point3((float)0.5,(float)0.5,(float)0.5));

 	mesh->EnableEdgeList(1);
#endif
	return 1;
	}

ImpNode *
ObjWorker::MakeDummy(const TCHAR *name) {
	if(!ReadyDummy())
		return NULL;
	ImpNode *node = i->CreateNode();
	if(!node || !dummy) {
		return NULL;
		}
	node->Reference(dummy);
	tm.IdentityMatrix();		// Reset initial matrix to identity
	node->SetTransform(0,tm);
	i->AddNodeToScene(node);
	node->SetName(name);
	AddNode(node,name,OBJ_DUMMY,mesh,"");
	return node;
	}

ImpNode *
ObjWorker::MakeANode(const TCHAR *name, BOOL target, char *owner) {
	int type, cstShad, rcvShad, mtlNum;
	Object *obj;
	// For now, it MUST have an object unless it's a target!
	if(!target) {
		// RB: changed cou
		if(!(obj = (Object *)FindObject((TCHAR*)name, type, cstShad, rcvShad, mtlNum))) {
			if(showPrompts)
				MessageBox((TCHAR *)name,IDS_DB_NO_OBJECT);
			return NULL;
			}
		UseObject((TCHAR*)name);
		}
	else {
		type = OBJ_TARGET;
		obj = i->CreateTargetObject();
		}
	if(type==OBJ_MESH)
		mesh = &(((TriObject *)obj)->GetMesh());
	else
		mesh = NULL;
	ImpNode *node = i->CreateNode();
	if(!node)
		return NULL;
	node->Reference((ObjectHandle)obj);
	tm.IdentityMatrix();		// Reset initial matrix to identity
	node->SetTransform(0,tm);
	node->GetINode()->SetCastShadows(cstShad);
	node->GetINode()->SetRcvShadows(rcvShad);
	i->AddNodeToScene(node);
	node->SetName(name);
	AddNode(node,name,type,mesh,owner,mtlNum);
	return node;
	}

int
ObjWorker::SetVerts(int count) {
	if(gotverts) {
		if(showPrompts)
			MessageBox(name,IDS_DB_HAS_VERTS);
		return 0;
		}
	if(!mesh->setNumVerts(count)) {
		if(showPrompts)
			MessageBox(name,IDS_DB_NUMVERTS_FAIL);
		return 0;
		}
	verts = count;
	gotverts = 1;
	return 1;
	}

int
ObjWorker::SetTVerts(int count) {
	if(gottverts) {
		if(showPrompts)
			MessageBox(name,IDS_DB_HAS_TVERTS);
		return 0;
		}
	if(!mesh->setNumTVerts(count)) {
		if(showPrompts)
			MessageBox(name,IDS_DB_NUMVERTS_FAIL);
		return 0;
		}
//DebugPrint("Set %d tverts\n",count);
	tverts = count;
	gottverts = 1;
	return 1;
	}

int
ObjWorker::SetFaces(int count) {
	if(gotfaces) {
		if(showPrompts)
			MessageBox(name,IDS_DB_HAS_FACES);
		return 0;
		}
	if(!mesh->setNumFaces(count)) {
		if(showPrompts)
			MessageBox(name,IDS_DB_NUMFACES_FAIL);
		return 0;
		}
	// If got texture vertices, set up an equal number of texture faces
	if(gottverts) {
		if(!mesh->setNumTVFaces(count)) {
			if(showPrompts)
				MessageBox(name,IDS_DB_NUMTVFACES_FAIL);
			return 0;
			}
		}
	faces = count;
	gotfaces = 1;
	return 1;
	}

int
ObjWorker::PutVertex(int index,Verts *v) {
	if(!gotverts) {
		if(showPrompts)
			MessageBox(IDS_DB_PUT_NO_VERTS,IDS_DB_3DSIMP);
		return 0;
		}
	if(index<0 || index>=verts) {
		if(showPrompts)
			MessageBox(IDS_DB_VERTS_OR,IDS_DB_3DSIMP);
		return 0;
		}
	mesh->setVert(index,v->x,v->y,v->z);
	return 1;
	}

int
ObjWorker::PutTVertex(int index,UVVert *v) {
	if(!gottverts) {
		if(showPrompts)
			MessageBox(IDS_DB_PUT_NO_TVERTS,IDS_DB_3DSIMP);
		return 0;
		}
	if(index<0 || index>=tverts) {
		if(showPrompts)
			MessageBox(IDS_DB_TVERTS_OR,IDS_DB_3DSIMP);
		return 0;
		}
	mesh->setTVert(index,*v);
	return 1;
	}

static void check_for_wrap(UVVert *tv, int flags) {
	float d;
	if (flags&UWRAP) {
		float maxu,minu;
		maxu = minu = tv[0].x;
		if (tv[1].x>maxu) maxu = tv[1].x;
		else if (tv[1].x<minu) minu = tv[1].x;
		if (tv[2].x>maxu) maxu = tv[2].x;
		else if (tv[2].x<minu) minu = tv[2].x;
		if ((maxu-minu)>0.8f) {
			d = (float)ceil(maxu-minu);
			if (tv[0].x<.5f)  tv[0].x += d; 
			if (tv[1].x<.5f)  tv[1].x += d; 
			if (tv[2].x<.5f)  tv[2].x += d; 
			}
		}
	if (flags&VWRAP) {
		float maxv,minv;
		maxv = minv = tv[0].y;
		if (tv[1].y>maxv) maxv = tv[1].y;
		else if (tv[1].y<minv) minv = tv[1].y;
		if (tv[2].y>maxv) maxv = tv[2].y;
		else if (tv[2].y<minv) minv = tv[2].y;
		if ((maxv-minv)>0.8f) {
			d = (float)ceil(maxv-minv);
			if (tv[0].y<.5f)  tv[0].y += d; 
			if (tv[1].y<.5f)  tv[1].y += d; 
			if (tv[2].y<.5f)  tv[2].y += d; 
			}
		}
	}

DWORD ObjWorker::AddNewTVert(UVVert p) {
	// see if already have it in the new verts.
	int ntv = newTV.Count();
	for (int i=0; i<ntv; i++)
		if (p == newTV[i]) return i + mesh->numVerts;
	// otherwise, add it.
	return 	(DWORD)newTV.Append(1,&p,10) + mesh->numVerts;
	}


void 
ObjWorker::SetTVerts(int nf, Faces *f) {
	UVVert uv[3], uvnew[3]; 
	DWORD ntv[3];
	/*
	uvnew[0] = uv[0] = mesh->tVerts[f->a];
	uvnew[1] = uv[1] = mesh->tVerts[f->b];
	uvnew[2] = uv[2] = mesh->tVerts[f->c];
	check_for_wrap(uvnew, f->flags);
	ntv[0] = (uvnew[0]==uv[0]) ? f->a: AddNewTVert(uvnew[0]);
	ntv[1] = (uvnew[1]==uv[1]) ? f->b: AddNewTVert(uvnew[1]);
	ntv[2] = (uvnew[2]==uv[2]) ? f->c: AddNewTVert(uvnew[2]);
	*/
	ntv[0] = f->a; 
	ntv[1] = f->b; 
	ntv[2] = f->c;

	TVFace *tf = &mesh->tvFace[nf];
	tf->setTVerts(ntv[0], ntv[1], ntv[2]);
	}

int
ObjWorker::PutFace(int index,Faces *f) {
	Face *fp = &mesh->faces[index];
	if(!gotfaces) {
		if(showPrompts)
			MessageBox(IDS_DB_PUT_NO_FACES,IDS_DB_3DSIMP);
		return 0;
		}
	if(index<0 || index>=faces)	{
		if(showPrompts)
			MessageBox(IDS_DB_FACES_OR,IDS_DB_3DSIMP);
		return 0;
		}

	/*
	// If we've got texture vertices, also put out a texture face
	if(gottverts) {
		// DS 6/6/96
		// check for wrap.
// fix 990920  --prs.
        int q = mesh->getNumTVerts();
        if (f->a < 0 || f->a >= q || f->b < 0 || f->b >= q ||
            f->c < 0 || f->c >= q) {
            if(showPrompts)
                MessageBox(IDS_PRS_TVERT_OR,IDS_DB_3DSIMP);
            return 0;
	        }
//        
		SetTVerts(index,f);
//		TVFace *tf = &mesh->tvFace[index];
//		tf->setTVerts((DWORD)f->a, (DWORD)f->b, (DWORD)f->c);
		}
// fix 990920  --prs.
    int qq = mesh->getNumVerts();
    if (f->a < 0 || f->a >= qq || f->b < 0 || f->b >= qq ||
        f->c < 0 || f->c >= qq) {
        if(showPrompts)
            MessageBox(IDS_PRS_VERT_OR,IDS_DB_3DSIMP);
        return 0;
	    }
//
*/
	fp->setVerts((int)f->a,(int)f->b,(int)f->c);
	fp->setEdgeVisFlags(f->flags & ABLINE,f->flags & BCLINE,f->flags & CALINE);
	fp->setSmGroup(f->sm_group);
	return 1;
	}


int ObjWorker::PutFaceMtl(int index, int imtl) {
	if (index<mesh->numFaces) {
		mesh->faces[index].setMatID(imtl);
		return 1;
		}
	else 
		return 0;
	}


void
ObjWorker::Abandon() {
	okay = 0;
	switch(mode) {
		case WORKER_MESH:
			delete object;
			break;
		case WORKER_KF:
			break;
		}
	Reset();
	}

void
ObjWorker::FreeObjList() {
	WkObjList *ptr = objects;
	while(ptr) {
		WkObjList *next = (WkObjList *)ptr->next;
		free(ptr);
		ptr = next;
		}
	objects = NULL;
	}

void
ObjWorker::FreeNodeList() {
	WkNodeList *ptr = nodes;
	while(ptr) {
		WkNodeList *next = (WkNodeList *)ptr->next;
		FreeKeyList(&ptr->posList);
		FreeKeyList(&ptr->rotList);
		FreeKeyList(&ptr->scList);
		FreeKeyList(&ptr->colList);
		FreeKeyList(&ptr->hotList);
		FreeKeyList(&ptr->fallList);
		FreeKeyList(&ptr->fovList);
		FreeKeyList(&ptr->rollList);
		free(ptr);
		ptr = next;
		}
	}

int
ObjWorker::AddObject(Object *obj, int type, const TCHAR *name, Matrix3 *tm, int mtlNum) {
	WkObjList *ptr = new WkObjList;
	if(!ptr)
		return 0;
	ptr->object = obj;
	ptr->name = name;
	ptr->type = type;
	ptr->used = 0;
	ptr->cstShad = cstShad;
	ptr->rcvShad = rcvShad;
	ptr->next = objects;
	ptr->mtln = mtlNum;
	if (tm) ptr->tm = *tm;
	else ptr->tm.IdentityMatrix();
	objects = ptr;
	return 1;
	}

int
ObjWorker::AddNode(ImpNode *node,const TCHAR *name,int type,Mesh *msh,char *owner, int mtlNum) {
//DebugPrint("Adding node %s, type %d, mesh:%p\n",name,type,msh);
	WkNodeList *ptr = new WkNodeList;
	if(!ptr)
		return 0;
	ptr->node = node;
	ptr->id = -1;
	ptr->type = type;
	ptr->name = name;
	ptr->mesh = msh;
	ptr->mnum = mtlNum;
	ptr->next = nodes;
	ptr->parent = NULL;
	ptr->posList = ptr->rotList = ptr->scList = ptr->colList = ptr->hotList = ptr->fallList = ptr->fovList = ptr->rollList = NULL;
	if(type==OBJ_TARGET)
		ptr->owner = TSTR(owner);
	workNode = nodes = ptr;
	thisNode = node;
	return 1;
	}

/*TriObject*/ void *
ObjWorker::FindObject(char *name, int &type, int &cstShad, int &rcvShad, int &mtlNum) {
	TSTR wname(name);
	WkObjList *ptr = objects;
	while(ptr) {
		if(_tcscmp(ptr->name,wname)==0) {
			type = ptr->type;
			cstShad = ptr->cstShad;
			rcvShad = ptr->rcvShad;
			mtlNum = ptr->mtln;
			return ptr->object;
			}
		ptr = (WkObjList *)ptr->next;
		}
	return NULL;
	}

int
ObjWorker::UseObject(char *name) {
	TSTR wname(name);
	WkObjList *ptr = objects;
	while(ptr) {
		if(_tcscmp(ptr->name,wname)==0) {
			ptr->used = 1;
			return 1;
			}
		ptr = (WkObjList *)ptr->next;
		}
	return 0;
	}

void ObjWorker::MakeControlsTCB(Control *tmCont,SHORT *tflags)
	{
	Control *c;
	DWORD flags=INHERIT_ALL;

	// Setup inheritance flags.
	if (tflags[POS_TRACK_INDEX] & NO_LNK_X) flags &= ~INHERIT_POS_X;
	if (tflags[POS_TRACK_INDEX] & NO_LNK_Y) flags &= ~INHERIT_POS_Y;
	if (tflags[POS_TRACK_INDEX] & NO_LNK_Z) flags &= ~INHERIT_POS_Z;

	if (tflags[ROT_TRACK_INDEX] & NO_LNK_X) flags &= ~INHERIT_ROT_X;
	if (tflags[ROT_TRACK_INDEX] & NO_LNK_Y) flags &= ~INHERIT_ROT_Y;
	if (tflags[ROT_TRACK_INDEX] & NO_LNK_Z) flags &= ~INHERIT_ROT_Z;

	if (tflags[SCL_TRACK_INDEX] & NO_LNK_X) flags &= ~INHERIT_SCL_X;
	if (tflags[SCL_TRACK_INDEX] & NO_LNK_Y) flags &= ~INHERIT_SCL_Y;
	if (tflags[SCL_TRACK_INDEX] & NO_LNK_Z) flags &= ~INHERIT_SCL_Z;

	tmCont->SetInheritanceFlags(flags,FALSE);

	c = tmCont->GetPositionController();
	if (c && c->ClassID()!=Class_ID(TCBINTERP_POSITION_CLASS_ID,0)) {
		Control *tcb = (Control*)ip->CreateInstance(
			CTRL_POSITION_CLASS_ID,
			Class_ID(TCBINTERP_POSITION_CLASS_ID,0));
		if (!tmCont->SetPositionController(tcb)) {
			tcb->DeleteThis();
			}
		}

	c = tmCont->GetRotationController();
	if (c && c->ClassID()!=Class_ID(TCBINTERP_ROTATION_CLASS_ID,0)) {
		Control *tcb = (Control*)ip->CreateInstance(
			CTRL_ROTATION_CLASS_ID,
			Class_ID(TCBINTERP_ROTATION_CLASS_ID,0));
		if (!tmCont->SetRotationController(tcb)) {
			tcb->DeleteThis();
			}
		}

	c = tmCont->GetRollController();
	if (c && c->ClassID()!=Class_ID(TCBINTERP_FLOAT_CLASS_ID,0)) {
		Control *tcb = (Control*)ip->CreateInstance(
			CTRL_FLOAT_CLASS_ID,
			Class_ID(TCBINTERP_FLOAT_CLASS_ID,0));
		if (!tmCont->SetRollController(tcb)) {
			tcb->DeleteThis();
			}
		}

	c = tmCont->GetScaleController();
	if (c && c->ClassID()!=Class_ID(TCBINTERP_SCALE_CLASS_ID,0)) {
		Control *tcb = (Control*)ip->CreateInstance(
			CTRL_SCALE_CLASS_ID,
			Class_ID(TCBINTERP_SCALE_CLASS_ID,0));
		if (!tmCont->SetScaleController(tcb)) {
			tcb->DeleteThis();
			}
		}
	}

int
ObjWorker::CompleteScene() {
	FinishUp();
	// If we need to expand time, ask the user!
	if(lengthSet) {
		Interval cur = i->GetAnimRange();
		Interval nrange = Interval(0,length * TIME_CONSTANT);
		if (!(cur==nrange)) {
			if (replaceScene||(!showPrompts)||MessageBox2(IDS_MATCHANIMLENGTH, IDS_3DSIMP, MB_YESNO)==IDYES) {
				i->SetAnimRange(nrange);
				}
			SetCursor(LoadCursor(NULL, IDC_WAIT));
			}
		
		/*		
		if((length * TIME_CONSTANT) > cur.End()) {
			if(MessageBox( IDS_TH_EXPANDANIMLENGTH, IDS_RB_3DSIMP, MB_YESNO) == IDYES) {
				cur.SetEnd(length * TIME_CONSTANT);
				i->SetAnimRange(cur);
				}
			}
		*/
		}
	WkNodeList *nptr = nodes;
	while(nptr) {
		if(nptr->type==OBJ_TARGET && nptr->node) {
			WkNodeList *nptr2 = nodes;
			while(nptr2) {
				if((nptr2->type==OBJ_CAMERA || nptr2->type==OBJ_SPOTLIGHT) && nptr->owner==nptr2->name && nptr2->node) {
					i->BindToTarget(nptr2->node,nptr->node);
					goto next_target;			
					}
				nptr2 = (WkNodeList *)nptr2->next;
				}
			}
		next_target:
		nptr = (WkNodeList *)nptr->next;
		}
	nptr = nodes;
	while(nptr) {
		if(nptr->node) {
			ImpNode *theNode = nptr->node;
			INode *theINode = theNode->GetINode();
			switch(nptr->type) {
				case OBJ_MESH:
					// Do materials.
					AssignMtl(theINode, nptr->mesh);

				case OBJ_OTHER:
					theINode->SetMtl(GetMaxMtl(nptr->mnum));
					
				default:
					assert(0);
					break;
				}

			}
		nptr = (WkNodeList *)nptr->next;
		}
	WkObjList *ptr = objects;
	while(ptr) {
		if(dStream) {
			fprintf(dStream,"Found object %s, used:%d\n",CStr(ptr->name),ptr->used);
			fflush(dStream);
			}
		if(!ptr->used) {
			ImpNode *node1;
			node1 = MakeANode(ptr->name, FALSE, "");

			if (ptr->type==OBJ_MESH) {
				INode *inode = node1->GetINode();
				Mesh *mesh = &(((TriObject *)inode->GetObjectRef())->GetMesh());
				AssignMtl(inode, mesh);
				}
		}
		ptr = (WkObjList *)ptr->next;
		}
	return 1;
	}

ImpNode *
ObjWorker::FindNode(char *name) {
	TSTR wname(name);
	WkNodeList *ptr = nodes;
	while(ptr) {
		if(_tcscmp(ptr->name,wname)==0)
			return ptr->node;
		ptr = (WkNodeList *)ptr->next;
		}
	return NULL;
	}

TCHAR *
ObjWorker::NodeName(ImpNode *node) {
	WkNodeList *ptr = nodes;
	while(ptr) {
		if(node == ptr->node)
			return ptr->name;
		ptr = (WkNodeList *)ptr->next;
		}
	return NULL;
	}

int
ObjWorker::SetNodesParent(ImpNode *node,ImpNode *parent) {
	WkNodeList *ptr = nodes;
	while(ptr) {
		if(node == ptr->node) {
			ptr->parent = parent;
			return 1;
			}
		ptr = (WkNodeList *)ptr->next;
		}
	return 0;
	}

ImpNode *
ObjWorker::FindNodeFromId(short id) {
	WkNodeList *ptr = nodes;
	while(ptr) {
		if(ptr->id == id)
			return ptr->node;
		ptr = (WkNodeList *)ptr->next;
		}
	return NULL;
	}

WkNodeList *
ObjWorker::FindEntry(char *name) {
	TSTR wname(name);
	WkNodeList *ptr = nodes;
	while(ptr) {
		if(_tcscmp(ptr->name,wname)==0)
			return ptr;
		ptr = (WkNodeList *)ptr->next;
		}
	return NULL;
	}

WkNodeList *
ObjWorker::FindEntryFromId(short id) {
	WkNodeList *ptr = nodes;
	while(ptr) {
		if(ptr->id == id)
			return ptr;
		ptr = (WkNodeList *)ptr->next;
		}
	return NULL;
	}

WkNodeList *
ObjWorker::FindNodeListEntry(ImpNode *node) {
	WkNodeList *ptr = nodes;
	while(ptr) {
		if(ptr->node == node)
			return ptr;
		ptr = (WkNodeList *)ptr->next;
		}
	return NULL;
	}

WkObjList *	
ObjWorker::FindObjListEntry(TSTR &name) {
	WkObjList *ptr = objects;
	while(ptr) {
		if(ptr->name == name)
			return ptr;
		ptr = (WkObjList *)ptr->next;
		}
	return NULL;
	}

int
ObjWorker::FindTypeFromNode(ImpNode *node, Mesh **mesh) {
	WkNodeList *ptr = nodes;
	while(ptr) {
		if(ptr->node == node) {
			*mesh =  ptr->mesh;
			return ptr->type;
			}
		ptr = (WkNodeList *)ptr->next;
		}
	return -1;
	}

void *
ObjWorker::FindObjFromNode(ImpNode *node) {
	WkNodeList *ptr = nodes;
	while(ptr) {
		if(ptr->node == node) {
			WkObjList *optr = objects;
			while(optr) {
				if(optr->name == ptr->name)
					return optr->object;
				optr = (WkObjList *)optr->next;
				}
			assert(0);
			return NULL;
			}
		ptr = (WkNodeList *)ptr->next;
		}
	return NULL;
	}

int
ObjWorker::SetNodeId(ImpNode *node,short id) {
	WkNodeList *ptr = nodes;
	while(ptr) {
		if(ptr->node == node) {
			ptr->id = id;
			if(dStream) {
				fprintf(dStream,"Setting node ID to %d\n",id);
				fflush(dStream);
				}
			return 1;
			}
		ptr = (WkNodeList *)ptr->next;
		}
	this->id = id;
	return 0;	
	}

/*
int
ObjWorker::AddKey(KeyList **list,Key *key) {
	if(mode != WORKER_KF) {
//		MessageBox(NULL,_T("Key add: Not in KF Worker mode"),_T("3DSIMP"),MB_OK);
		return 1;	// Keep it from bailing out -- Will need to return 0 eventually
		}
	KeyList *ptr = new KeyList;
	if(!ptr)
		return 0;
	KeyList *work = *list,*last = NULL;
	while(work) {
		last = work;
		work = (KeyList *)work->next;
		}
	if(last)
		last->next = ptr;
	else
		*list = ptr;
	ptr->key.key = *key;
	ptr->next = NULL;
	return 1;
	}
*/

void
ObjWorker::FreeKeyList(KeyList **list) {
	KeyList *ptr = *list;
	while(ptr) {
		KeyList *next = (KeyList *)ptr->next;
		free(ptr);
		ptr = next;
		}
	*list = NULL;
	}

int
ObjWorker::SetTransform(ImpNode *node,Matrix3& m) {
	WkNodeList *ptr = nodes;
	while(ptr) {
		if(ptr->node == node) {
			ptr->tm = m;
			return 1;
			}
		ptr = (WkNodeList *)ptr->next;
		}
	return 0;
	}

Matrix3
ObjWorker::GetTransform(ImpNode *node) {
	WkNodeList *ptr = nodes;
	while(ptr) {
		if(ptr->node == node)
			return ptr->tm;
		ptr = (WkNodeList *)ptr->next;
		}
	Matrix3 dummy;
	dummy.IdentityMatrix();
	return dummy;
	}

// Pointer to the ObjWorker object

ObjWorker *theWorker;


#define RSMIMP_CLASS_ID	Class_ID(0x1e44d910, 0x7de3fe69)

class RSMImp : public SceneImport {
	public:
		static HWND hParams;

		int				ExtCount();					// Number of extensions supported
		const TCHAR *	Ext(int n);					// Extension #n (i.e. "3DS")
		const TCHAR *	LongDesc();					// Long ASCII description (i.e. "Autodesk 3D Studio File")
		const TCHAR *	ShortDesc();				// Short ASCII description (i.e. "3D Studio")
		const TCHAR *	AuthorName();				// ASCII Author name
		const TCHAR *	CopyrightMessage();			// ASCII Copyright message
		const TCHAR *	OtherMessage1();			// Other message #1
		const TCHAR *	OtherMessage2();			// Other message #2
		unsigned int	Version();					// Version number * 100 (i.e. v3.01 = 301)
		void			ShowAbout(HWND hWnd);		// Show DLL's "About..." box
		int				DoImport(const TCHAR *name,ImpInterface *i,Interface *gi, BOOL suppressPrompts=FALSE);	// Import file
		
		//Constructor/Destructor
		RSMImp();
		~RSMImp();		
};


class RSMImpClassDesc:public ClassDesc2 {
	public:
	int 			IsPublic() {return 1;}
	void *			Create(BOOL loading = FALSE) {return new RSMImp();}
	//MAX 5 Bug?
	const TCHAR *	ClassName() {return _T("RSMImp");}
	SClass_ID		SuperClassID() {return SCENE_IMPORT_CLASS_ID;}
	Class_ID		ClassID() {return RSMIMP_CLASS_ID;}
	const TCHAR* 	Category() {return _T("Scene Import");}
	const TCHAR*	InternalName() { return _T("RSMImp"); }	// returns fixed parsable name (scripter-visible name)
	HINSTANCE		HInstance() { return hInstance; }				// returns owning module handle
};

static RSMImpClassDesc RSMImpDesc;
ClassDesc2* GetRSMImpDesc() {return &RSMImpDesc;}


class CLoadDialog : public CDialog
{
public:
	CLoadDialog(char *DialogName, CWnd *Owner) : CDialog(DialogName, Owner) {}
	CLoadDialog(UINT DialogID, CWnd *Owner) : CDialog(DialogID, Owner) {}

	BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
};


BEGIN_MESSAGE_MAP(CLoadDialog, CDialog)
END_MESSAGE_MAP()

BOOL CLoadDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	CListBox *lbptr = (CListBox *) GetDlgItem(IDC_LIST1);

	lbptr->AddString("Hello World!");

	return TRUE;
}


BOOL CALLBACK RSMImpOptionsDlgProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam) {
	static RSMImp *imp = NULL;
	//CListBox *lbptr;
	int i=0;
	switch(message) {
	case WM_INITDIALOG:
			imp = (RSMImp *)lParam;
			CenterWindow(hWnd,GetParent(hWnd));


			return TRUE;

			

		case WM_CLOSE:

			EndDialog(hWnd, 0);
			return TRUE;
	}
	return FALSE;
}


//--- RSMImp -------------------------------------------------------
RSMImp::RSMImp()
{
	FILE *fp;

	

	fp = fopen("rsmimp.ini","r");
	if(!fp)
	{
		sprintf(ragnapath,"C:\\Program Files\\Gravity\\RagnarokOnline");
		fp = fopen("rsmimp.ini","w");
		fwrite(ragnapath,sizeof(ro_string_t),1,fp);
		fclose(fp);
		return;
	} 
	
	fgets (ragnapath,1024,fp);
	fclose(fp);
}

RSMImp::~RSMImp() 
{

}

int RSMImp::ExtCount()
{
	//TODO: Returns the number of file name extensions supported by the plug-in.
	return 3;
}

const TCHAR *RSMImp::Ext(int n)
{		
	//TODO: Return the 'i-th' file name extension (i.e. "3DS").
	switch(n)
	{
	case 0:	return _T("RSM");
	case 1:	return _T("GND");
	case 2:	return _T("RSW");
	}
}

const TCHAR *RSMImp::LongDesc()
{
	//TODO: Return long ASCII description (i.e. "Targa 2.0 Image File")
	return _T("Ragnarok Online 3D Model Files");
}
	
const TCHAR *RSMImp::ShortDesc() 
{			
	//TODO: Return short ASCII description (i.e. "Targa")
	return _T("RSM Models");
}

const TCHAR *RSMImp::AuthorName()
{			
	//TODO: Return ASCII Author name
	return _T("David Santos");
}

const TCHAR *RSMImp::CopyrightMessage() 
{	
	// Return ASCII Copyright message
	return _T("©2003 David Santos/EnderSoft");
}

const TCHAR *RSMImp::OtherMessage1() 
{		
	//TODO: Return Other message #1 if any
	return _T("This is for educational purposes only.\nEndersoft is in no way affiliated with Gravity");
}

const TCHAR *RSMImp::OtherMessage2() 
{		
	//TODO: Return other message #2 in any
	return _T("");
}

unsigned int RSMImp::Version()
{				
	//TODO: Return Version number * 100 (i.e. v3.01 = 301)
	return 500;
}

void RSMImp::ShowAbout(HWND hWnd)
{			
	// Optional
	DialogBoxParam(hInstance, 
 		MAKEINTRESOURCE(IDD_PANEL), 
		GetActiveWindow(), 
		RSMImpOptionsDlgProc, (LPARAM)this);
}


int matloffset;

int RSMLoad(ObjWorker *theWorker, const char *filename, Matrix3 *mt, int idx)
{
	Verts v;
	Faces f;
	UVVert tv;

	int todo;
	float ftodo[10];

	ro_string_t	name;
	ro_string_t parent_name;
	ro_transf_t transf;
	int npos_vtx;
	ro_vertex_t *pos_vtx;
	rsm_header_t *header;

//	unsigned int *textures;
//	bool *alphatex;
//	int *which;
	
	int ntextures;
	ro_string_t *textures_name;
	int ntex_vtx;
	ro_vertex_t *tex_vtx;
	int nall_faces;
	ro_face_t *all_faces;

//	int nframes;
//	ro_frame_t *frames;
//	int *surfnum;
//	ro_surface_t *surfaces;

	bool main;
	int nmeshes = 0;
	char buffer[256];

	int j;

	FILE *fp;

	fp = fopen(filename, "rb");

	if (!fp) {
		return false;
	}

	fread (&header, sizeof(rsm_header_t), 1, fp);
	fread (&ntextures, sizeof(int), 1, fp);

	textures_name = new ro_string_t[ntextures];

	fread (textures_name, sizeof(ro_string_t), ntextures, fp);

	for (j = 0; j < ntextures; j++)
	{
		sprintf(buffer,"RagMat%d-%d",idx,j+1);
		theWorker->AddRagMtl(buffer,ragnapath, textures_name[j]);
	}

	//	while (file_status(fp) != 0) {
	sprintf(buffer,"RagnaMesh%02d",idx);

	theWorker->StartMesh(filename);

	main = (nmeshes == 0);

	fread (name, sizeof(ro_string_t), 1, fp);
	
	if (main)
		fread (&todo, sizeof(int), 1, fp);

	fread (parent_name, sizeof(ro_string_t), 1, fp);

	if (main)
		fread (ftodo, sizeof(float), 10, fp);

	fread (&ntextures, sizeof(int), 1, fp);

//	textures = new unsigned int[ntextures];
//	alphatex = new bool[ntextures];
//	which = new int[ntextures];

	for (j = 0; j < ntextures; j++) {
		int n;
		fread (&n, sizeof(int), 1, fp);
//		which[i] = n;
//		textures[i] = all_textures[n];
//		alphatex[i] = all_alphatex[i];
	}

	fread (&transf, sizeof(ro_transf_t), 1, fp);

	fread (&npos_vtx, sizeof(int), 1, fp);
	pos_vtx = new ro_vertex_t[npos_vtx];
	fread (pos_vtx, sizeof(ro_vertex_t), npos_vtx, fp);

	bounding_box_t box;

	box.max[0] = box.max[1] = box.max[2] = -999999.0f;
	box.min[0] = box.min[1] = box.min[2] = 999999.0f;

	theWorker->SetVerts(npos_vtx);

	//get bounding box
	for(j=0;j<npos_vtx;j++)
	{
		v.x = pos_vtx[j][0];	
		v.y = pos_vtx[j][1];	
		v.z = pos_vtx[j][2];	

		if(v.x > box.max[0]) box.max[0] = v.x;
		if(v.y > box.max[1]) box.max[1] = v.y;
		if(v.z > box.max[2]) box.max[2] = v.z;

		if(v.x < box.min[0]) box.min[0] = v.x;
		if(v.y < box.min[1]) box.min[1] = v.y;
		if(v.z < box.min[2]) box.min[2] = v.z;
	}

	box.range[0] = box.max[0] - box.min[0];
	box.range[1] = box.max[1] - box.min[1];
	box.range[2] = box.max[2] - box.min[2];

	box.max[0] = box.range[0]/2;
	box.max[1] = box.range[1]/2;
	box.max[2] = box.range[2]/2;

	box.min[0] = -box.range[0]/2;
	box.min[1] = -box.range[1]/2;
	box.min[2] = -box.range[2]/2;

	//adjust vertexes ceter object in Y/X
	//raise object above Z
	for(j=0;j<npos_vtx;j++)
	{
		v.x = pos_vtx[j][0] - box.range[0]/2;	
		v.y = pos_vtx[j][1] - box.range[1]/2;	
		v.z = pos_vtx[j][2] - box.range[2];	
		theWorker->PutVertex(j,&v);
	}
	
	fread (&ntex_vtx, sizeof(int), 1, fp);
	tex_vtx = new ro_vertex_t[ntex_vtx];
	fread(tex_vtx, sizeof(ro_vertex_t), ntex_vtx, fp);

	theWorker->SetTVerts(ntex_vtx);

	for(j=0;j<ntex_vtx;j++)
	{
		tv.x = tex_vtx[j][1]; tv.y = 1-tex_vtx[j][2];
		theWorker->PutTVertex(j,&tv); 
	}

	fread(&nall_faces, sizeof(int), 1, fp);
	all_faces = new ro_face_t[nall_faces];
	fread (all_faces, sizeof(ro_face_t), nall_faces, fp);

	theWorker->SetFaces(nall_faces);
	for(j=0;j<nall_faces;j++)
	{
		f.a = all_faces[j].v[0] ; f.b = all_faces[j].v[1] ; f.c = all_faces[j].v[2];
		f.material = 0; f.sm_group = 0;
		f.flags = 0xff;
		f.material = 0;
   		theWorker->PutFace(j,&f);
		theWorker->PutFaceMtl(j,matloffset + all_faces[j].text);

		f.a = all_faces[j].t[0] ; f.b = all_faces[j].t[1] ; f.c = all_faces[j].t[2];
		theWorker->SetTVerts(j, &f);
	}

//	if (file_status(fp) != 2) {
//		nmeshes++;
//		if (nmeshes > 29)
//		break;
//	}

//	fread (&nframes, sizeof(int), 1, fp);
//	frames = new ro_frame_t[nframes];
//	fread (frames, sizeof(ro_frame_t), nframes, fp);

	mt->Scale(Point3(transf.todo[19], transf.todo[20], transf.todo[21]),true);

//	if (main)
//		if (!only) {
			mt->Translate(Point3(-box.range[0], -box.max[1], -box.range[2]));
//		} else {
//do this
//			glTranslatef(0.0, -box->max[1]+box->range[1], 0.0);
//		}
	
//	if (!main)	
//		glTranslatef(transf.todo[12], transf.todo[13], transf.todo[14]);

//	if (!nframes)
		mt->SetAngleAxis(Point3(transf.todo[16],transf.todo[17],transf.todo[18]),transf.todo[15]);
//	else
//		glMultMatrixf(Ori);
	

//	if (main && only)
		mt->Translate(Point3(-box.range[0], -box.range[1], -box.range[2]));

//	if (!main || !only)
//		glTranslatef(transf.todo[9], transf.todo[10], transf.todo[11]);

/*
		Matrix3* Rot;
		Rot = new Matrix3( \
		Point3(transf.todo[0],transf.todo[3],transf.todo[6]), \
		Point3(transf.todo[1],transf.todo[4],transf.todo[7]), \
		Point3(transf.todo[2],transf.todo[5],transf.todo[8]), \
		Point3(0.0,0.0,0.0));
		*mt *= *Rot;
*/

		//commit material to object
		theWorker->SetTm(mt);  

/*	if (nframes) {
		int i;
		int current = 0;
		int next;
		GLfloat t;
		GLfloat q[4], q1[4], q2[4];
		char buffer[1024];

		for (i = 0; i < nframes; i++) {
			if (nstep < frames[i].time) {
				current = i-1;
				break;
			}
		}

		next = current + 1;
		if (next == nframes)
			next = 0;

		t = ((GLfloat) (nstep-frames[current].time))
			/((GLfloat) (frames[next].time-frames[current].time));

		for (i = 0; i < 4; i++) {
			q[i] = frames[current].orientation[i]*(1-t) + t*frames[next].orientation[i];
		}

		GLfloat norm;
		norm = sqrtf(q[0]*q[0]+q[1]*q[1]+q[2]*q[2]+q[3]*q[3]);

		for (i = 0; i < 4; i++)
			q[i] /= norm;
		
		GLfloat two_x = q[0] * 2.0;
		GLfloat two_y = q[1] * 2.0;
		GLfloat two_z = q[2] * 2.0;

		Ori[0] = 1.0 - two_y * q[1] - two_z * q[2];
		Ori[1] = two_x * q[1];
		Ori[2] = two_z * q[0];
		Ori[3] = 0.0;

		Ori[4] = two_x * q[1];
		Ori[5] = 1.0 - two_x * q[0] - two_z * q[2];
		Ori[6] = two_y * q[2];
		Ori[7] = 0.0;
		
		Ori[8] = two_z * q[0];
		Ori[9] = two_y * q[2];
		Ori[10] = 1.0 - two_x * q[0] - two_y * q[1];
		Ori[11] = 0.0;

		Ori[12] = 0.0;
		Ori[13] = 0.0;
		Ori[14] = 0.0;
		Ori[15] = 1.0;

		nstep += 100;
		if (nstep >= frames[nframes-1].time)
			nstep = 0;
	}
*/

	theWorker->FinishUp();
	matloffset += ntextures;

	//while (file_status(fp) != 0) {
	//	meshes[nmeshes].Load(fp, textures, alphatex, (nmeshes == 0));
	//	nmeshes++;
	//	if (nmeshes > 29)
	//		break;
	//}

	//if (nmeshes == 1)
	//	meshes[0].only = true;
	//else
	//	meshes[0].only = false;

	fclose (fp);

	//father = new int[nmeshes];
	//father[0] = 0;
	//for (i = 0; i < nmeshes; i++)
	//	for (int j = 0; j < nmeshes; j++)
	//		if ((j != i) && (!strcmp(meshes[j].parent_name, meshes[i].name)))
	//			father[j] = i;

	
	//Clean up!!!
//	for(j=0;j<npos_vtx;j++)
//		free(pos_vtx[j]);
//	for(j=0;j<ntextures;j++)
//		free(textures_name[j]);
//	for(j=0;j<ntex_vtx;j++);
//		free(tex_vtx[j]);
//	for(j=0;j<nall_faces;j++);
//		free(&all_faces[j]);

	free(pos_vtx);
	free(textures_name);
	free(tex_vtx);
	free(all_faces);

	//Success
	return true;
}

#define L 10

class VertexList
{
private:
	Verts Vertexes[50000];
	int totalverts;

	/* implementation dependend declarations */
	typedef Verts keyType;            /* type of key */

	typedef int hashIndexType;      /* index into hash table */

	typedef struct nodeTag {
		struct nodeTag *next;       /* next node */
		keyType key;                /* key */
		int VertexNumber;
	} nodeType;	

	nodeType **hashTable;
	int hashTableSize;

	hashIndexType hash(keyType key) {
		return (abs((int)(key.x + key.y + key.z) % hashTableSize));
	}

	void insertKey(keyType key) {
		nodeType *p, *p0;
		hashIndexType bucket;

		/* insert node at beginning of list */
		bucket = hash(key);
		
		if(findKey(key)==-1)
		{
			if ((p = (nodeType *) malloc(sizeof(nodeType))) == 0)
				return;

			p0 = hashTable[bucket];
			hashTable[bucket] = p;
		
			p->next = p0;
			p->key = key;
			Vertexes[totalverts] = key;
			p->VertexNumber = totalverts++;
		}
	}

	int findKey(keyType key) {
		nodeType *p;
		
		if(totalverts == 0) return -1;

		p = hashTable[hash(key)];
		while (p && !IsVertexEqual(p->key, key)) 
			p = p->next;
		if (!p) return -1;

		return p->VertexNumber;
	}

public:
	VertexList() { 
		totalverts = 0; 
		hashTableSize = HASH_TABLE_SIZE;
		hashTable = (nodeTag **) calloc(hashTableSize, sizeof(nodeType *));
		//Vertexes = (UVVert *) malloc(5000 * sizeof(UVVert)); 
	}

	~VertexList() {}

	int numVerts() {
		return(totalverts);
	}

	int FindVertex(Verts v) {
		return(findKey(v));
	};

	Verts GetVertex(int i) { 
		return(Vertexes[i]);
	} 

	void AddVertex(Verts v)
	{
		insertKey(v);
	}
};


class TVertexList
{
private:
	int totalverts;
	UVVert Vertexes[5000];

	/* implementation dependend declarations */
	typedef UVVert keyType;            /* type of key */

	typedef int hashIndexType;      /* index into hash table */

	typedef struct nodeTag {
		struct nodeTag *next;       /* next node */
		keyType key;                /* key */
		int VertexNumber;
	} nodeType;	

	nodeType **hashTable;
	int hashTableSize;

	hashIndexType hash(keyType key) {
		return (abs((int)((key.x + key.y + key.z) * 100) % hashTableSize));
	}

	void insertKey(keyType key) {
		nodeType *p, *p0;
		hashIndexType bucket;

		if(findKey(key)==-1)
		{
			/* insert node at beginning of list */
			bucket = hash(key);

			if ((p = (nodeType *) malloc(sizeof(nodeType))) == 0)
				return;

			p0 = hashTable[bucket];
			hashTable[bucket] = p;
		
			p->next = p0;
			p->key = key;
			Vertexes[totalverts] = key;
			p->VertexNumber = totalverts++;
		}
	}

	int findKey(keyType key) {
		nodeType *p;

		if(totalverts == 0) return -1;

		p = hashTable[hash(key)];
		while (p && !IsVertexEqual(p->key, key)) 
			p = p->next;
		if (!p) return -1;

		return p->VertexNumber;
	}

public:
	TVertexList() { 
		totalverts = 0; 
		hashTableSize = HASH_TABLE_SIZE;
		hashTable = (nodeTag **) calloc(hashTableSize, sizeof(nodeType *));
		//Vertexes = (UVVert *) malloc(5000 * sizeof(UVVert)); 
	}

	~TVertexList() {}

	int numVerts() {
		return(totalverts);
	}

	int FindVertex(UVVert v) {
		return(findKey(v));
	};

	UVVert GetVertex(int i) { 
		return(Vertexes[i]);
	} 

	void AddVertex(UVVert v)
	{
		insertKey(v);
	}
};

bool GNDLoad(ObjWorker *theWorker, const char *filename)
{
	FILE *fp;
	gnd_header_t header;
	ro_string_t *textures_name;
	int i,j,k;
	char buffer[16];

	Verts v;
	Faces f;
	UVVert tv;

//	GLuint *textures;
	int sizeX;
	int sizeY;

	int ntextures;

	ro_cube_t *cubes;
	ro_tile_t *tiles;
	int ntiles;
	ro_lightmap_t *lightmaps;
	int nlightmaps;

	fp = fopen (filename, "rb");

	fread (&header, sizeof(gnd_header_t), 1, fp);
	fread (&sizeX, sizeof(int), 1, fp);
	fread (&sizeY, sizeof(int), 1, fp);
	fseek (fp, 4, SEEK_CUR);
	fread (&ntextures, sizeof(int), 1, fp);
	fseek (fp, 4, SEEK_CUR);
	
//	textures = new GLuint[ntextures];
	textures_name = new ro_string_t[ntextures];

	for (i = 0; i < ntextures; i++) {
		fread (textures_name[i], sizeof(ro_string_t), 1, fp);
		fseek (fp, 40, SEEK_CUR);
	}

//	glGenTextures(ntextures, textures);

	for (j = 0; j < ntextures; j++)
	{
		sprintf(buffer,"RagMatl#%d",j+1);
		theWorker->AddRagMtl(buffer,ragnapath,textures_name[j]);
	}

	theWorker->StartMesh("GNDMesh");

	fread (&nlightmaps, sizeof(int), 1, fp);
	fseek (fp, 12, SEEK_CUR);

	lightmaps = new ro_lightmap_t[nlightmaps];

	fread (lightmaps, sizeof(ro_lightmap_t), nlightmaps, fp);

	fread (&ntiles, sizeof(int), 1, fp);
	tiles = new ro_tile_t[ntiles];

	fread (tiles, sizeof(ro_tile_t), ntiles, fp);

	cubes = new ro_cube_t[sizeX*sizeY];

	fread (cubes, sizeof(ro_cube_t), sizeX*sizeY, fp);

	//Here we go!

	VertexList myVList;
	TVertexList myTVList;

	for (i = 0; i < sizeY; i++) 
	for (j = 0; j < sizeX; j++) 
	{
		int tilesup;
		int tileside;
		int tileotherside;

		tilesup = cubes[i*sizeX+j].tilesup;
		tileside = cubes[i*sizeX+j].tileside;
		tileotherside = cubes[i*sizeX+j].tileotherside;

		if (tileotherside != -1) 
		{
			v.x = j * 10;
			v.y = (i + 1) * 10; 
			v.z = -cubes[i * sizeX + j].y3;
			myVList.AddVertex(v);

			tv.x = tiles[tileotherside].u1;
			tv.y = 1 - tiles[tileotherside].v1; 
			tv.z = 0;
			myTVList.AddVertex(tv);

			v.x = (j + 1) * 10;
			v.y = (i + 1) * 10;
			v.z = -cubes[i * sizeX + j].y4;
			myVList.AddVertex(v);

			tv.x = tiles[tileotherside].u2; 
			tv.y = 1 - tiles[tileotherside].v2; 
			tv.z = 0;
			myTVList.AddVertex(tv);

			v.x = (j + 1) * 10;
			v.y = (i+1)*10;
			v.z = -cubes[(i + 1) * sizeX + j].y2;
			myVList.AddVertex(v);

			tv.x = tiles[tileotherside].u4;
			tv.y = 1 - tiles[tileotherside].v4; 
			tv.z = 0;
			myTVList.AddVertex(tv);

			v.x = j * 10; 
			v.y = (i + 1) * 10; 
			v.z = -cubes[(i + 1) * sizeX + j].y1;
			myVList.AddVertex(v);

			tv.x = tiles[tileotherside].u3; 
			tv.y = 1 - tiles[tileotherside].v3; 
			tv.z = 0;
			myTVList.AddVertex(tv);
		}
		
		if (tileside != -1) 
		{	
			v.x = (j + 1) * 10; 
			v.y = (i + 1) * 10; 
			v.z = -cubes[i * sizeX + j].y4;
			myVList.AddVertex(v);

			tv.x = tiles[tileside].u1; 
			tv.y = 1 - tiles[tileside].v1; 
			tv.z = 0;
			myTVList.AddVertex(tv);

			v.x = (j + 1) * 10; 
			v.y = i * 10; 
			v.z = -cubes[i * sizeX + j].y2;
			myVList.AddVertex(v);

			tv.x = tiles[tileside].u2; 
			tv.y = 1 - tiles[tileside].v2; 
			tv.z = 0;
			myTVList.AddVertex(tv);

			v.x = (j + 1)*10; 
			v.y = i * 10; 
			v.z = -cubes[i * sizeX + j + 1].y1;
			myVList.AddVertex(v);

			tv.x = tiles[tileside].u4; 
			tv.y = 1 - tiles[tileside].v4; 
			tv.z = 0;
			myTVList.AddVertex(tv);

			v.x = (j + 1) * 10; 
			v.y = (i + 1) * 10; 
			v.z = -cubes[i * sizeX + j + 1].y3;
			myVList.AddVertex(v);

			tv.x = tiles[tileside].u3; 
			tv.y = 1 - tiles[tileside].v3; 
			tv.z = 0;
			myTVList.AddVertex(tv);
		}

		if (tilesup != -1) 
		{
			v.x = j * 10;
			v.y = i * 10;
			v.z = -cubes[i*sizeX+j].y1;
			myVList.AddVertex(v);

			tv.x = tiles[tilesup].u1;
			tv.y = 1-tiles[tilesup].v1;
			tv.z = 0;
			myTVList.AddVertex(tv);

			v.x = (j + 1) * 10;
			v.y = i * 10;
			v.z = -cubes[i * sizeX + j].y2;
			myVList.AddVertex(v);

			tv.x = tiles[tilesup].u2;
			tv.y = 1 - tiles[tilesup].v2;
			tv.z = 0;
			myTVList.AddVertex(tv);

			v.x = (j + 1) * 10;
			v.y = (i + 1) * 10;
			v.z = -cubes[i*sizeX+j].y4;
			myVList.AddVertex(v);

			tv.x = tiles[tilesup].u4; 
			tv.y = 1 - tiles[tilesup].v4; 
			tv.z = 0;
			myTVList.AddVertex(tv);

			v.x = j * 10; 
			v.y = (i + 1) * 10; 
			v.z = -cubes[i * sizeX + j].y3;
			myVList.AddVertex(v);

			tv.x = tiles[tilesup].u3;
			tv.y = 1 - tiles[tilesup].v3; 
			tv.z = 0;
			myTVList.AddVertex(tv);
	
		}
	}

	theWorker->SetVerts(myVList.numVerts());
	theWorker->SetTVerts(myTVList.numVerts());
	theWorker->SetFaces(ntiles*2);
	
	for(i = 0; i < myVList.numVerts();i++)
		theWorker->PutVertex(i,&myVList.GetVertex(i));

	for(i = 0; i < myTVList.numVerts();i++)
		theWorker->PutTVertex(i,&myTVList.GetVertex(i));


	k = 0;
	int v1 = -1,v2 = -1,v3 = -1,v4 = -1,tv1 = -1,tv2 = -1,tv3 = -1,tv4 = -1;

	for (i = 0; i < sizeY; i++) 
	for (j = 0; j < sizeX; j++) 
	{
		int tilesup;
		int tileside;
		int tileotherside;

		tilesup = cubes[i*sizeX+j].tilesup;
		tileside = cubes[i*sizeX+j].tileside;
		tileotherside = cubes[i*sizeX+j].tileotherside;

		if (tileotherside != -1) {
			
			v.x = j * 10; 
			v.y = (i + 1) * 10; 
			v.z = -cubes[i * sizeX + j].y3;
			v1 = myVList.FindVertex(v);

			v.x = (j + 1) * 10;
			v.y = (i + 1) * 10; 
			v.z = -cubes[i * sizeX + j].y4;
			v2 = myVList.FindVertex(v);

			v.x = (j + 1) * 10; 
			v.y = (i + 1) * 10; 
			v.z = -cubes[(i + 1) * sizeX + j].y2;
			v3 = myVList.FindVertex(v);

			v.x = j * 10; 
			v.y = (i + 1) * 10; 
			v.z = -cubes[(i + 1) * sizeX + j].y1;
			v4 = myVList.FindVertex(v);

			tv.x = tiles[tileotherside].u1; 
			tv.y = 1-tiles[tileotherside].v1; 
			tv.z = 0;
			tv1 = myTVList.FindVertex(tv);

			tv.x = tiles[tileotherside].u2; 
			tv.y = 1-tiles[tileotherside].v2; 
			tv.z = 0;
			tv2 = myTVList.FindVertex(tv);

			tv.x = tiles[tileotherside].u4; 
			tv.y = 1-tiles[tileotherside].v4; 
			tv.z = 0;
			tv3 = myTVList.FindVertex(tv);

			tv.x = tiles[tileotherside].u3; 
			tv.y = 1-tiles[tileotherside].v3; 
			tv.z = 0;
			tv4 = myTVList.FindVertex(tv);

			f.a = v1; f.b = v2; f.c = v3;
			f.flags = 7;
			theWorker->PutFace(k,&f);
			theWorker->PutFaceMtl(k,matloffset + tiles[tileotherside].text);

			f.a = tv1; f.b = tv2; f.c = tv3;
			theWorker->SetTVerts(k++,&f);
		
			f.a = v3; f.b = v4; f.c = v1;
			f.flags = 7;
			theWorker->PutFace(k,&f);
			theWorker->PutFaceMtl(k,matloffset + tiles[tileotherside].text);

			f.a = tv3; f.b = tv4; f.c = tv1;
			theWorker->SetTVerts(k++,&f);
		}
		
		if (tileside != -1) {	

			v.x = (j + 1) * 10;
			v.y = (i + 1) * 10; 
			v.z = -cubes[i * sizeX + j].y4;
			v1 = myVList.FindVertex(v);

			tv.x = tiles[tileside].u1; 
			tv.y = 1 - tiles[tileside].v1; 
			tv.z = 0;
			tv1 = myTVList.FindVertex(tv);

			v.x = (j + 1) * 10;
			v.y = i * 10; 
			v.z = -cubes[i * sizeX + j].y2;
			v2 = myVList.FindVertex(v);

			tv.x = tiles[tileside].u2; 
			tv.y = 1 - tiles[tileside].v2; 
			tv.z = 0;
			tv2 = myTVList.FindVertex(tv);

			v.x = (j + 1) * 10; 
			v.y = i * 10; 
			v.z = -cubes[i * sizeX + j + 1].y1;
			v3 = myVList.FindVertex(v);

			tv.x = tiles[tileside].u4; 
			tv.y = 1 - tiles[tileside].v4; tv.z = 0;
			tv3 = myTVList.FindVertex(tv);

			v.x = (j + 1) * 10; 
			v.y = (i + 1) * 10; 
			v.z = -cubes[i * sizeX + j + 1].y3;
			v4 = myVList.FindVertex(v);

			tv.x = tiles[tileside].u3; 
			tv.y = 1 - tiles[tileside].v3; 
			tv.z = 0;
			tv4 = myTVList.FindVertex(tv);

			f.a = v1; f.b = v2; f.c = v3;
			f.flags = 7;
			theWorker->PutFace(k,&f);
			theWorker->PutFaceMtl(k,matloffset + tiles[tileside].text);

			f.a = tv1; f.b = tv2; f.c = tv3;
			theWorker->SetTVerts(k++,&f);

			f.a = v3; f.b = v4; f.c = v1;
			f.flags = 7;
			theWorker->PutFace(k,&f);
			theWorker->PutFaceMtl(k,matloffset + tiles[tileside].text);

			f.a = tv3; f.b = tv4; f.c = tv1;
			theWorker->SetTVerts(k++,&f);

		}

		if (tilesup != -1) {
			
			v.x = j * 10; 
			v.y = i * 10; 
			v.z = -cubes[i * sizeX + j].y1;
			v1 = myVList.FindVertex(v);

			tv.x = tiles[tilesup].u1; 
			tv.y = 1 - tiles[tilesup].v1; 
			tv.z = 0;
			tv1 = myTVList.FindVertex(tv);

			v.x = (j + 1) * 10; 
			v.y = i * 10; 
			v.z = -cubes[i * sizeX + j].y2;
			v2 = myVList.FindVertex(v);

			tv.x = tiles[tilesup].u2; 
			tv.y = 1 - tiles[tilesup].v2; 
			tv.z = 0;
			tv2 = myTVList.FindVertex(tv);

			v.x = (j + 1) * 10; 
			v.y = (i + 1) * 10;
			v.z = -cubes[i * sizeX + j].y4;
			v3 = myVList.FindVertex(v);

			tv.x = tiles[tilesup].u4; 
			tv.y = 1 - tiles[tilesup].v4; 
			tv.z = 0;
			tv3 = myTVList.FindVertex(tv);

			v.x = j * 10; 
			v.y = (i + 1) * 10; 
			v.z = -cubes[i * sizeX + j].y3;
			v4 = myVList.FindVertex(v);

			tv.x = tiles[tilesup].u3; 
			tv.y = 1 - tiles[tilesup].v3; 
			tv.z = 0;
			tv4 = myTVList.FindVertex(tv);

			f.a = v1; f.b = v2; f.c = v3;
			f.flags = 7;
			theWorker->PutFace(k,&f);
			theWorker->PutFaceMtl(k,matloffset + tiles[tilesup].text);

			f.a = tv1; f.b = tv2; f.c = tv3;
			theWorker->SetTVerts(k++,&f);

			f.a = v3; f.b = v4; f.c = v1;
			f.flags = 7;
			theWorker->PutFace(k,&f);
			theWorker->PutFaceMtl(k,matloffset + tiles[tilesup].text);

			f.a = tv3; f.b = tv4; f.c = tv1;
			theWorker->SetTVerts(k++,&f);
		}
	}
	
	Matrix3 *myMatrix;

	myMatrix = new Matrix3(1);
	//Rotate 90 degrees about the z axis...
	//myMatrix.RotateZ(90/180*3.141592654);

	//...and center it...
	myMatrix->Translate(Point3(sizeX*L/2, sizeY*L/2, 0));

	//...now apply the transformation
	theWorker->SetTm(myMatrix);  

   //Forgot to clean up...

//	for(i=0;i<nlightmaps;i++)
//		free(lightmaps[i]);
//	for(i=0;i<ntiles;i++)
//		free(&tiles[i]);
//	for(i=0;i<sizeX*sizeY;i++)
//		free(&cubes[i]);
//	for(i=0;i<ntextures;i++)
//		free(textures_name[i]);

	free(lightmaps);
	free(&tiles);
	free(&cubes);
	free(textures_name);

	theWorker->FinishUp();

	matloffset += ntextures;

	fclose(fp);

	return true;
}

bool RSWLoad(ObjWorker *theWorker, CONST char *filename)
{
	FILE *fp;
	long pos;
	ro_string_t gndfile;
	ro_string_t gatfile;
	ro_string_t *filepath;
	int i;
	int nmodels;
	ro_model_t *models;
	int nrealmodels;
	bool *bexist;
	char buffer[256];
	Matrix3 *mt;

	fp = fopen (filename, "rb");

	fseek (fp, 46, SEEK_CUR);
	
	fread(gndfile, sizeof(ro_string_t), 1, fp);
	fread(gatfile, sizeof(ro_string_t), 1, fp);

	//GNDLoad(theWorker, gndfile);

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

//	sprintf(buffer,"%d models were found in this file",nmodels);
//	MessageBox(buffer,0,MB_OK);

	models = new ro_model_t[nmodels];
	filepath = new ro_string_t[nmodels];

//	FILE *fout;
//	fout = fopen("sortie.txt", "w");

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

//	sprintf(buffer,"Found %d unique models",nrealmodels);
//	MessageBox(buffer,0,MB_OK);

	ro_string_t *realmodelspath;
	realmodelspath = new ro_string_t[nrealmodels];
//	realmodels = new RSM[nrealmodels];

	int k = 0;
	for (i = 0; i < nmodels; i++)
		if (breal[i]) {
			strcpy(realmodelspath[k], filepath[i]);
			k++;
		}


	mt = new Matrix3(1);

	bexist = new bool[nrealmodels];
	for (i = 0; i < nrealmodels; i++) {

		mt->IdentityMatrix();
		mt->Scale(Point3(1.0,1.0,-1.0),FALSE);
		mt->Translate(Point3(models[i].position.x,-models[i].position.y,models[i].position.z));
		mt->RotateX(models[i].position.rx); mt->RotateY(models[i].position.ry); mt->RotateZ(models[i].position.rz);
		mt->Scale(Point3(models[i].position.sx,-models[i].position.sy,models[i].position.sz),true);

		sprintf(buffer,"%s\\data\\model\\%s",ragnapath,realmodelspath[i]);

		if (!RSMLoad(theWorker, buffer, mt, i))
			bexist[i] = false;
		else
			bexist[i] = true;
	}

//	for (i = 0; i < nmodels; i++) {
//		int num = 0;
//		models[i].model = 1;
//		for (num = 0; num < nrealmodels; num++) {
//			if (!strcmp(filepath[i], realmodelspath[num])) {
//				models[i].model = num;
//				break;
//			}
//		}

//	}


	fclose(fp);

	delete [] realmodelspath;

	delete [] filepath;

	delete [] models;

//    ground.Display();
//	for (int i = 0; i < nmodels; i++) {
//		if (bexist[models[i].model])
//		{
//			realmodels[models[i].model].Display(models[i].position);
//		}
//	}
	return true;
}

int RSMImp::DoImport(const TCHAR *filename,ImpInterface *i,
						Interface *gi, BOOL suppressPrompts)
{
	ObjWorker W(i,gi);
	theWorker = &W;
	const char *ext;
	Matrix3 *mat;


	mat = new Matrix3(1);
	mat->IdentityMatrix();
	mat->Scale(Point3(1.0,1.0,-1.0),true);

	ext = filename + strlen(filename) - 3;
    
	matloffset = 0;

	if(!stricmp(ext,"rsm"))
		RSMLoad(theWorker, filename,mat,1);
	if(!stricmp(ext,"gnd"))
		GNDLoad(theWorker, filename);
	if(!stricmp(ext,"rsw"))
		MessageBox("Sorry, RSW import isn't working yet.",IDS_LIBDESCRIPTION);
		//RSWLoad(theWorker, filename);

	theWorker->CompleteScene();

	//grf_free(myGRF);

	if(!suppressPrompts)
		DialogBoxParam(hInstance, 
				MAKEINTRESOURCE(IDD_PANEL), 
				GetActiveWindow(), 
				RSMImpOptionsDlgProc, (LPARAM)this);

	return TRUE;
}
	


