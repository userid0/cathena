// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "nullpo.h"
#include "showmsg.h"
// #include "logs.h" // �z�΂��Ă݂�

static void nullpo_info_core(const char *file, int line, const char *func, 
                             const char *fmt, va_list ap);



int nullpo_chk(const char *file, int line, const char *func, const void *target)
{
	if (target != NULL)
		return 0;
	
	nullpo_info_core(file, line, func, NULL, NULL);
	return 1;
}



void nullpo_info(const char *file, int line, const char *func)
{
	nullpo_info_core(file, line, func, NULL, NULL);
}


/*======================================
 * nullpo���o��(Main)
 *--------------------------------------
 */
static void nullpo_info_core(const char *file, int line, const char *func, 
                             const char *fmt, va_list ap)
{
	if (file == NULL)
		file = "??";
	
	func =
		func == NULL    ? "unknown":
		func[0] == '\0' ? "unknown":
		                  func;
	
	ShowMessage("--- nullpo info --------------------------------------------\n");
	ShowMessage("%s:%d: in func `%s'\n", file, line, func);
	if (fmt != NULL)
	{
		if (fmt[0] != '\0')
		{
			vprintf(fmt, ap);
			
			// �Ō�ɉ��s�������m�F
			if (fmt[strlen(fmt)-1] != '\n')
				ShowMessage("\n");
		}
	}
	ShowMessage("--- end nullpo info ----------------------------------------\n");
	
	// �������nullpo���O���t�@�C���ɏ����o������
	// �܂Ƃ߂Ē�o�ł���ȂƎv���Ă�����B
}
