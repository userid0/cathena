// Rsm.cpp: implementation of the Rsm class.
//
//////////////////////////////////////////////////////////////////////

#include "Rsm.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

RSM::RSM()
{
	ntextures = 0;
	nmeshes = 0;
}

void RSM::Save(char *filename)
{
	int fin = 0;
	FILE *fp;
	fp = fopen (filename, "wb");

	fwrite (&header, sizeof(rsm_header_t), 1, fp);

	fwrite (&ntextures, sizeof(int), 1, fp);

	fwrite (textures_name, sizeof(ro_string_t), ntextures, fp);

	for (int i = 0; i < nmeshes; i++) {
		meshes[i].Save(fp, (i == 0));
	}

	fwrite (&fin, sizeof(int), 1, fp);
	fwrite (&fin, sizeof(int), 1, fp);

	fclose (fp);
}

bool RSM::GRFLoad(Grf *grffile, char *filename, bool extract)
{
	int i, offset;
	unsigned char *RSMData;
	unsigned long size;
	
	sprintf(&modelfname[0],"%s",filename);
	strcpy(&modelfname[strlen(filename)],"\0");

	offset = 0;
	RSMData = (unsigned char *) grf_get(grffile, filename, &size, NULL);

	GrfError error;

	if (extract)
	{
		char targfile[512];
		sprintf(targfile,"C:\\Program Files\\RagnarokOffline3\\%s",filename);
		grf_extract (grffile, filename, targfile, NULL);
	}


	if (!RSMData) return FALSE;

	memcpy(&header, &RSMData[offset], sizeof(rsm_header_t));
	offset += sizeof(rsm_header_t);
	memcpy(&ntextures, &RSMData[offset], sizeof(int));
	offset += sizeof(int);

	textures = new GLuint[ntextures];
	alphatex = new bool[ntextures];
	textures_name = new ro_string_t[ntextures];

	memcpy(textures_name, &RSMData[offset], sizeof(ro_string_t) * ntextures);
	offset += sizeof(ro_string_t) * ntextures;

	glGenTextures(ntextures, textures);

	for (i = 0; i < ntextures; i++)
	{
			LoadGRFTexture (grffile, textures_name[i], textures[i], &alphatex[i], extract);
	}
	meshes = new RSM_Mesh[30];
	
	while (data_status(&offset, RSMData) != 0) {
		meshes[nmeshes].GRFLoad(&offset, RSMData, textures, alphatex, (nmeshes == 0));
		nmeshes++;
		if (nmeshes > 29)
			break;
	}
	
	if (nmeshes == 1)
		meshes[0].only = true;
	else
		meshes[0].only = false;

	father = new int[nmeshes];
	father[0] = 0;
	for (i = 0; i < nmeshes; i++)
		for (int j = 0; j < nmeshes; j++)
			if ((j != i) && (!strcmp(meshes[j].parent_name, meshes[i].name)))
				father[j] = i;

	BoundingBox();

	return true;
}


bool RSM::Load(char *ragnapath, char *filename)
{
	FILE *fp;
	int i;

	fp = fopen(filename, "rb");

	if (!fp) {
		return false;
	}

	fread (&header, sizeof(rsm_header_t), 1, fp);

	fread (&ntextures, sizeof(int), 1, fp);

	textures = new GLuint[ntextures];
	alphatex = new bool[ntextures];
	textures_name = new ro_string_t[ntextures];

	fread (textures_name, sizeof(ro_string_t), ntextures, fp);

	glGenTextures(ntextures, textures);

	for (i = 0; i < ntextures; i++)
	{
			LoadTexture (ragnapath, textures_name[i], textures[i], &alphatex[i]);
	}
	meshes = new RSM_Mesh[30];

	while (file_status(fp) != 0) {
		meshes[nmeshes].Load(fp, textures, alphatex, (nmeshes == 0));
		nmeshes++;
		if (nmeshes > 29)
			break;
	}

	if (nmeshes == 1)
		meshes[0].only = true;
	else
		meshes[0].only = false;

	fclose (fp);

	father = new int[nmeshes];
	father[0] = 0;
	for (i = 0; i < nmeshes; i++)
		for (int j = 0; j < nmeshes; j++)
			if ((j != i) && (!strcmp(meshes[j].parent_name, meshes[i].name)))
				father[j] = i;

	BoundingBox();

	return true;
}

void RSM::BoundingBox()
{
	int i;

	meshes[0].BoundingBox();
	for (i = 1; i < nmeshes; i++)
		if (father[i] == 0)
			meshes[i].BoundingBox(&meshes[0].transf);

	for (i = 0; i < 3; i++) {
		box.max[i] = meshes[0].max[i];
		box.min[i] = meshes[0].min[i];
		for (int j = 1; j < nmeshes; j++) {
			if (father[j] == 0) {
				box.max[i] = MAX(meshes[j].max[i], box.max[i]);
				box.min[i] = MIN(meshes[j].min[i], box.min[i]);
			}
		}
		box.range[i] = (box.max[i]+box.min[i])/2.0;
	}
}

void RSM::Display(ro_position_t pos)
{
	glPushMatrix();


	glTranslatef(pos.x, -pos.y, pos.z);

	glRotatef(pos.ry, 0.0, 1.0, 0.0);
	glRotatef(pos.rz, 1.0, 0.0, 0.0);
	glRotatef(pos.rx, 0.0, 0.0, 1.0);

	glScalef(pos.sx, -pos.sy, pos.sz);
	
	DisplayMesh(&box, 0);

	glPopMatrix();
}

void RSM::DisplayMesh(bounding_box_t *b, int n, ro_transf_t *ptransf)
{
	glPushMatrix();

	meshes[n].Display(b, ptransf);

	for (int i = 0; i < nmeshes; i++)
		if ((i!=n) && (father[i]==n)) {
			DisplayMesh((n==0)?b:NULL, i, &meshes[n].transf);
		}
	glPopMatrix();

	
}

void RSM::setdrawbounds(bool mode)
{
	meshes[0].drawbounds = mode;
}

RSM::~RSM()
{
	if (ntextures)
		delete[] textures;
}
