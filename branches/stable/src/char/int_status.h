#ifndef __INT_STATUS__
#define __INT_STATUS__

#include "char.h"

struct scdata {
	int account_id, char_id;
	int count;
	struct status_change_data* data;
};

extern char scdata_txt[1024];

struct scdata *status_search_scdata(int aid, int cid);
void status_delete_scdata(int aid, int cid);
void inter_status_save();
void status_init();
void status_final(void);

#endif
