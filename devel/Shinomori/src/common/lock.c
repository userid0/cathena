

#ifndef WIN32
#include <unistd.h>
#else
#include <io.h>	// for access
#define F_OK   0x0
#define R_OK   0x4
#endif


#include "base.h"
#include "lock.h"
#include "utils.h"
#include "showmsg.h"


/*
00 Existence only 
02 Write permission 
04 Read permission 
06 Read and write permission 
*/

#define exists(filename) (!access(filename, F_OK))



// 書き込みファイルの保護処理
// （書き込みが終わるまで、旧ファイルを保管しておく）

// 新しいファイルの書き込み開始
FILE* lock_fopen (const char* filename, int *info) {
	char newfile[512];
	FILE *fp;
	int no = 0;

	// 安全なファイル名を得る（手抜き）
	do {
		sprintf(newfile,"%s_%04d.tmp",filename,++no);
	} while((fp = savefopen(newfile,"r")) && (fclose(fp), no<9999) );
	*info = no;
	return savefopen(newfile,"w");
}

// 旧ファイルを削除＆新ファイルをリネーム
int lock_fclose (FILE *fp, const char* filename, int *info) {
	int ret = 1;
	char newfile[512];
	char oldfile[512];
	if (fp != NULL) {
		ret = fclose(fp);
		sprintf(newfile, "%s_%04d.tmp", filename, *info);
		sprintf(oldfile, "%s.bak", filename);	// old backup file

		if (exists(oldfile)) remove(oldfile);	// remove backup file if it already exists
		rename (filename, oldfile);				// backup our older data instead of deleting it

		// このタイミングで落ちると最悪。
		if ((ret = rename(newfile,filename)) != 0) {	// rename our temporary file to its correct name
			ShowError("%s - '"CL_WHITE"%s"CL_RESET"'\n", strerror(errno), newfile);
		}
	}
	
	return ret;
}

