#include "Rsm-common.h"

int data_status(int *offset, unsigned char *p)
{
	char c[4];
	int i;
	int bfin = true;
	long pos;

	pos = *offset;
	memcpy(&c[0], p + *offset, sizeof(char) * 4);
	*offset += sizeof(char) * 4;

	for (i = 0; i < 4; i++)
		if (c[i] != 0) {
			bfin = false;
			break;
		}

	if (!bfin) {
		*offset = pos;
		return 2;
	}

	pos = *offset;
	memcpy(&c[0], p + *offset, sizeof(char) * 4);
	*offset += sizeof(char) * 4;

	for (i = 0; i < 4; i++)
		if (c[i] != 0) {
			bfin = false;
			break;
		}

	if (!bfin) {
		*offset = pos;
		return 1;
	} else
		return 0;
}

int file_status(FILE *fp)
{
	char c[4];
	int i;
	int bfin = true;
	long pos;

	pos = ftell(fp);
	fread (c, sizeof(char), 4, fp);

	for (i = 0; i < 4; i++)
		if (c[i] != 0) {
			bfin = false;
			break;
		}

	if (!bfin) {
		fseek (fp, pos, SEEK_SET);
		return 2;
	}

	pos = ftell(fp);
	fread (c, sizeof(char), 4, fp);

	for (i = 0; i < 4; i++)
		if (c[i] != 0) {
			bfin = false;
			break;
		}

	if (!bfin) {
		fseek (fp, pos, SEEK_SET);
		return 1;
	} else
		return 0;
}



void DisplayBoundingBox(GLfloat *max, GLfloat *min,
						GLfloat r, GLfloat g, GLfloat b)
{
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	glColor4f(r, g, b, 0.75);
	
	glVertex3f(min[0], min[1], max[2]);
	glVertex3f(min[0], max[1], max[2]);
	glVertex3f(max[0], max[1], max[2]);
	glVertex3f(max[0], min[1], max[2]);
	
	glVertex3f(max[0], max[1], min[2]);
	glVertex3f(min[0], max[1], min[2]);
	glVertex3f(min[0], min[1], min[2]);
	glVertex3f(max[0], min[1], min[2]);
	
	glVertex3f(max[0], max[1], max[2]);
	glVertex3f(max[0], min[1], max[2]);
	glVertex3f(max[0], min[1], min[2]);
	glVertex3f(max[0], max[1], min[2]);
	
	glVertex3f(min[0], max[1], max[2]);
	glVertex3f(min[0], min[1], max[2]);
	glVertex3f(min[0], min[1], min[2]);
	glVertex3f(min[0], max[1], min[2]);

	glColor4f(1.0, 1.0, 1.0, 1.0);

	glEnd();
	glEnable(GL_TEXTURE_2D);
}
