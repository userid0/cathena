// $Id: int_guild.c,v 1.2 2004/09/25 19:36:53 Akitasha Exp $
#include "base.h"
#include "inter.h"
#include "int_guild.h"
#include "int_storage.h"
#include "mmo.h"
#include "char.h"
#include "socket.h"
#include "db.h"
#include "lock.h"
#include "showmsg.h"
#include "utils.h"
#include "malloc.h"

char guild_txt[1024] = "save/guild.txt";
char castle_txt[1024] = "save/castle.txt";

static struct dbt *guild_db;
static struct dbt *castle_db;

static uint32 guild_newid = 10000;

static int guild_exp[100];

int mapif_parse_GuildLeave(int fd, uint32 guild_id, uint32 account_id, uint32 char_id, int flag, const char *mes);
int mapif_guild_broken(uint32 guild_id, int flag);
int guild_check_empty(struct guild *g);
int guild_calcinfo(struct guild *g);
int mapif_guild_basicinfochanged(uint32 guild_id, int type, uint32 data);
int mapif_guild_info(int fd, struct guild *g);

// ギルドデータの文字列への変換
int inter_guild_tostr(char *str, struct guild *g)
{
	int i, c, len;

	// 基本データ
	len = sprintf(str, "%ld\t%s\t%s\t%d,%d,%ld,%d,%d\t%s#\t%s#\t",
	              (unsigned long)g->guild_id, g->name, g->master,
	              g->guild_lv, g->max_member, (unsigned long)g->exp, g->skill_point, g->castle_id,
	              g->mes1, g->mes2);
	// メンバー
	for(i = 0; i < g->max_member; i++) {
		struct guild_member *m = &g->member[i];
		len += sprintf(str + len, "%ld,%ld,%d,%d,%d,%d,%d,%ld,%ld,%d\t%s\t",
		               (unsigned long)m->account_id, (unsigned long)m->char_id,
		               m->hair, m->hair_color, m->gender,
		               m->class_, m->lv, (unsigned long)m->exp, (unsigned long)m->exp_payper, m->position,
		               ((m->account_id > 0) ? m->name : "-"));
	}
	// 役職
	for(i = 0; i < MAX_GUILDPOSITION; i++) {
		struct guild_position *p = &g->position[i];
		len += sprintf(str + len, "%ld,%ld\t%s#\t", (unsigned long)p->mode, (unsigned long)p->exp_mode, p->name);
	}
	// エンブレム
	len += sprintf(str + len, "%d,%ld,", g->emblem_len, (unsigned long)g->emblem_id);
	for(i = 0; i < g->emblem_len; i++) {
		len += sprintf(str + len, "%02x", (unsigned char)(g->emblem_data[i]));
	}
	len += sprintf(str + len, "$\t");
	// 同盟リスト
	c = 0;
	for(i = 0; i < MAX_GUILDALLIANCE; i++)
		if (g->alliance[i].guild_id > 0)
			c++;
	len += sprintf(str + len, "%d\t", c);
	for(i = 0; i < MAX_GUILDALLIANCE; i++) {
		struct guild_alliance *a = &g->alliance[i];
		if (a->guild_id > 0)
			len += sprintf(str + len, "%ld,%ld\t%s\t", (unsigned long)a->guild_id, (unsigned long)a->opposition, a->name);
	}
	// 追放リスト
	c = 0;
	for(i = 0; i < MAX_GUILDEXPLUSION; i++)
		if (g->explusion[i].account_id > 0)
			c++;
	len += sprintf(str + len, "%d\t", c);
	for(i = 0; i < MAX_GUILDEXPLUSION; i++) {
		struct guild_explusion *e = &g->explusion[i];
		if (e->account_id > 0)
			len += sprintf(str + len, "%ld,%ld,%ld,%ld\t%s\t%s\t%s#\t",
			               (unsigned long)e->account_id, (unsigned long)e->rsv1, (unsigned long)e->rsv2, (unsigned long)e->rsv3,
			               e->name, e->acc, e->mes );
	}
	// ギルドスキル
	for(i = 0; i < MAX_GUILDSKILL; i++) {
		len += sprintf(str + len, "%d,%d ", g->skill[i].id, g->skill[i].lv);
	}
	len += sprintf(str + len, "\t");

	return 0;
}

// ギルドデータの文字列からの変換
int inter_guild_fromstr(char *str, struct guild *g)
{
	int i, j, c;
	int tmp_int[16];
	char tmp_str[4][256];
	char tmp_str2[4096];
	char *pstr;

	// 基本データ
	memset(g, 0, sizeof(struct guild));
	if (sscanf(str, "%d\t%[^\t]\t%[^\t]\t%d,%d,%d,%d,%d\t%[^\t]\t%[^\t]\t", &tmp_int[0],
	           tmp_str[0], tmp_str[1],
	           &tmp_int[1], &tmp_int[2], &tmp_int[3], &tmp_int[4], &tmp_int[5],
	           tmp_str[2], tmp_str[3]) < 8)
		return 1;

	g->guild_id = tmp_int[0];
	g->guild_lv = tmp_int[1];
	g->max_member = tmp_int[2];
	g->exp = tmp_int[3];
	g->skill_point = tmp_int[4];
	g->castle_id = tmp_int[5];
	memcpy(g->name, tmp_str[0], 24);
	memcpy(g->master, tmp_str[1], 24);
	memcpy(g->mes1, tmp_str[2], 60);
	memcpy(g->mes2, tmp_str[3], 120);
	g->mes1[strlen(g->mes1)-1] = 0;
	g->mes2[strlen(g->mes2)-1] = 0;

	for(j = 0; j < 6 && str != NULL; j++)	// 位置スキップ
		str = strchr(str + 1, '\t');
//	ShowMessage("GuildBaseInfo OK\n");

	// メンバー
	for(i = 0; i < g->max_member; i++) {
		struct guild_member *m = &g->member[i];
		if (sscanf(str + 1, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\t%[^\t]\t",
		           &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3], &tmp_int[4],
		           &tmp_int[5], &tmp_int[6], &tmp_int[7], &tmp_int[8], &tmp_int[9],
		           tmp_str[0]) < 11)
			return 1;
		m->account_id = tmp_int[0];
		m->char_id = tmp_int[1];
		m->hair = tmp_int[2];
		m->hair_color = tmp_int[3];
		m->gender = tmp_int[4];
		m->class_ = tmp_int[5];
		m->lv = tmp_int[6];
		m->exp = tmp_int[7];
		m->exp_payper = tmp_int[8];
		m->position = tmp_int[9];
		memcpy(m->name, tmp_str[0], 24);

		for(j = 0; j < 2 && str != NULL; j++)	// 位置スキップ
			str = strchr(str + 1, '\t');
	}
//	ShowMessage("GuildMemberInfo OK\n");
	// 役職
	i = 0;
	while (sscanf(str+1, "%d,%d%n", &tmp_int[0], &tmp_int[1], &j) == 2 && str[1+j] == '\t') {
		struct guild_position *p = &g->position[i];
		if (sscanf(str+1, "%d,%d\t%[^\t]\t", &tmp_int[0], &tmp_int[1], tmp_str[0]) < 3)
			return 1;
		p->mode = tmp_int[0];
		p->exp_mode = tmp_int[1];
		tmp_str[0][strlen(tmp_str[0])-1] = 0;
		memcpy(p->name, tmp_str[0], 24);

		for(j = 0; j < 2 && str != NULL; j++)	// 位置スキップ
			str = strchr(str+1, '\t');
		i++;
	}
//	ShowMessage("GuildPositionInfo OK\n");
	// エンブレム
	tmp_int[1] = 0;
	if (sscanf(str + 1, "%d,%d,%[^\t]\t", &tmp_int[0], &tmp_int[1], tmp_str2)< 3 &&
		sscanf(str + 1, "%d,%[^\t]\t", &tmp_int[0], tmp_str2) < 2)
		return 1;
	g->emblem_len = tmp_int[0];
	g->emblem_id = tmp_int[1];
	for(i = 0, pstr = tmp_str2; i < g->emblem_len; i++, pstr += 2) {
		int c1 = pstr[0], c2 = pstr[1], x1 = 0, x2 = 0;
		if (c1 >= '0' && c1 <= '9') x1 = c1 - '0';
		if (c1 >= 'a' && c1 <= 'f') x1 = c1 - 'a' + 10;
		if (c1 >= 'A' && c1 <= 'F') x1 = c1 - 'A' + 10;
		if (c2 >= '0' && c2 <= '9') x2 = c2 - '0';
		if (c2 >= 'a' && c2 <= 'f') x2 = c2 - 'a' + 10;
		if (c2 >= 'A' && c2 <= 'F') x2 = c2 - 'A' + 10;
		g->emblem_data[i] = (x1<<4) | x2;
	}
//	ShowMessage("GuildEmblemInfo OK\n");
	str=strchr(str + 1, '\t');	// 位置スキップ

	// 同盟リスト
	if (sscanf(str + 1, "%d\t", &c) < 1)
		return 1;
	str = strchr(str + 1, '\t');	// 位置スキップ
	for(i = 0; i < c; i++) {
		struct guild_alliance *a = &g->alliance[i];
		if (sscanf(str + 1, "%d,%d\t%[^\t]\t", &tmp_int[0], &tmp_int[1], tmp_str[0]) < 3)
			return 1;
		a->guild_id = tmp_int[0];
		a->opposition = tmp_int[1];
		memcpy(a->name, tmp_str[0], 24);

		for(j = 0; j < 2 && str != NULL; j++)	// 位置スキップ
			str = strchr(str + 1, '\t');
	}
//	ShowMessage("GuildAllianceInfo OK\n");
	// 追放リスト
	if (sscanf(str+1, "%d\t", &c) < 1)
		return 1;
	str = strchr(str + 1, '\t');	// 位置スキップ
	for(i = 0; i < c; i++) {
		struct guild_explusion *e = &g->explusion[i];
		if (sscanf(str + 1, "%d,%d,%d,%d\t%[^\t]\t%[^\t]\t%[^\t]\t",
		           &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3],
		           tmp_str[0], tmp_str[1], tmp_str[2]) < 6)
			return 1;
		e->account_id = tmp_int[0];
		e->rsv1 = tmp_int[1];
		e->rsv2 = tmp_int[2];
		e->rsv3 = tmp_int[3];
		memcpy(e->name, tmp_str[0], 24);
		memcpy(e->acc, tmp_str[1], 24);
		tmp_str[2][strlen(tmp_str[2])-1] = 0;
		memcpy(e->mes, tmp_str[2], 40);

		for(j = 0; j < 4 && str != NULL; j++)	// 位置スキップ
			str = strchr(str + 1, '\t');
	}
//	ShowMessage("GuildExplusionInfo OK\n");
	// ギルドスキル
	for(i = 0; i < MAX_GUILDSKILL; i++) {
		if (sscanf(str+1,"%d,%d ", &tmp_int[0], &tmp_int[1]) < 2)
			break;
		g->skill[i].id = tmp_int[0];
		g->skill[i].lv = tmp_int[1];
		str = strchr(str + 1, ' ');
	}
	str = strchr(str + 1, '\t');
//	ShowMessage("GuildSkillInfo OK\n");

	return 0;
}

// ギルド城データの文字列への変換
int inter_guildcastle_tostr(char *str, struct guild_castle *gc)
{
	int len;

	len = sprintf(str, "%d,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld",	// added Guardian HP [Valaris]
	              gc->castle_id, (unsigned long)gc->guild_id, (unsigned long)gc->economy, (unsigned long)gc->defense, (unsigned long)gc->triggerE,
	              (unsigned long)gc->triggerD, (unsigned long)gc->nextTime, (unsigned long)gc->payTime, (unsigned long)gc->createTime, (unsigned long)gc->visibleC,
	              (unsigned long)gc->visibleG0, (unsigned long)gc->visibleG1, (unsigned long)gc->visibleG2, (unsigned long)gc->visibleG3, (unsigned long)gc->visibleG4,
	              (unsigned long)gc->visibleG5, (unsigned long)gc->visibleG6, (unsigned long)gc->visibleG7, (unsigned long)gc->Ghp0, (unsigned long)gc->Ghp1, (unsigned long)gc->Ghp2,
	              (unsigned long)gc->Ghp3, (unsigned long)gc->Ghp4, (unsigned long)gc->Ghp5, (unsigned long)gc->Ghp6, (unsigned long)gc->Ghp7);

	return 0;
}

// ギルド城データの文字列からの変換
int inter_guildcastle_fromstr(char *str, struct guild_castle *gc)
{
	int tmp_int[26];

	memset(gc, 0, sizeof(struct guild_castle));
	// new structure of guild castle
	if (sscanf(str, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
	           &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3], &tmp_int[4], &tmp_int[5], &tmp_int[6],
	           &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12], &tmp_int[13],
	           &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18], &tmp_int[19], &tmp_int[20],
	           &tmp_int[21], &tmp_int[22], &tmp_int[23], &tmp_int[24], &tmp_int[25]) == 26) {
		gc->castle_id = tmp_int[0];
		gc->guild_id = tmp_int[1];
		gc->economy = tmp_int[2];
		gc->defense = tmp_int[3];
		gc->triggerE = tmp_int[4];
		gc->triggerD = tmp_int[5];
		gc->nextTime = tmp_int[6];
		gc->payTime = tmp_int[7];
		gc->createTime = tmp_int[8];
		gc->visibleC = tmp_int[9];
		gc->visibleG0 = tmp_int[10];
		gc->visibleG1 = tmp_int[11];
		gc->visibleG2 = tmp_int[12];
		gc->visibleG3 = tmp_int[13];
		gc->visibleG4 = tmp_int[14];
		gc->visibleG5 = tmp_int[15];
		gc->visibleG6 = tmp_int[16];
		gc->visibleG7 = tmp_int[17];
		gc->Ghp0 = tmp_int[18];
		gc->Ghp1 = tmp_int[19];
		gc->Ghp2 = tmp_int[20];
		gc->Ghp3 = tmp_int[21];
		gc->Ghp4 = tmp_int[22];
		gc->Ghp5 = tmp_int[23];
		gc->Ghp6 = tmp_int[24];
		gc->Ghp7 = tmp_int[25];	// end additions [Valaris]
	// old structure of guild castle
	} else if (sscanf(str, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
	                  &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3], &tmp_int[4], &tmp_int[5], &tmp_int[6],
	                  &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12], &tmp_int[13],
	                  &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17]) == 18) {
		gc->castle_id = tmp_int[0];
		gc->guild_id = tmp_int[1];
		gc->economy = tmp_int[2];
		gc->defense = tmp_int[3];
		gc->triggerE = tmp_int[4];
		gc->triggerD = tmp_int[5];
		gc->nextTime = tmp_int[6];
		gc->payTime = tmp_int[7];
		gc->createTime = tmp_int[8];
		gc->visibleC = tmp_int[9];
		gc->visibleG0 = tmp_int[10];
		gc->visibleG1 = tmp_int[11];
		gc->visibleG2 = tmp_int[12];
		gc->visibleG3 = tmp_int[13];
		gc->visibleG4 = tmp_int[14];
		gc->visibleG5 = tmp_int[15];
		gc->visibleG6 = tmp_int[16];
		gc->visibleG7 = tmp_int[17];
		if (gc->visibleG0 == 1)
			gc->Ghp0 = 15670 + 2000 * gc->defense;
		else
			gc->Ghp0 = 0;
		if (gc->visibleG1 == 1)
			gc->Ghp1 = 15670 + 2000 * gc->defense;
		else
			gc->Ghp1 = 0;
		if (gc->visibleG2 == 1)
			gc->Ghp2 = 15670 + 2000 * gc->defense;
		else
			gc->Ghp2 = 0;
		if (gc->visibleG3 == 1)
			gc->Ghp3 = 30214 + 2000 * gc->defense;
		else
			gc->Ghp3 = 0;
		if (gc->visibleG4 == 1)
			gc->Ghp4 = 30214 + 2000 * gc->defense;
		else
			gc->Ghp4 = 0;
		if (gc->visibleG5 == 1)
			gc->Ghp5 = 28634 + 2000 * gc->defense;
		else
			gc->Ghp5 = 0;
		if (gc->visibleG6 == 1)
			gc->Ghp6 = 28634 + 2000 * gc->defense;
		else
			gc->Ghp6 = 0;
		if (gc->visibleG7 == 1)
			gc->Ghp7 = 28634 + 2000 * gc->defense;
		else
			gc->Ghp7 = 0;
	} else {
		return 1;
	}

	return 0;
}

// ギルド関連データベース読み込み
int inter_guild_readdb()
{
	int i;
	FILE *fp;
	char line[1024];
	char path[1024];

	sprintf(path, "%s%s", db_path, "/exp_guild.txt");
	fp = safefopen(path, "r");
	if (fp == NULL) {
		ShowError("can't read %s\n",path);
		return 1;
	}
	i = 0;
	while(fgets(line, sizeof(line), fp) && i < 100){
		if( !skip_empty_line(line) )
			continue;
		guild_exp[i] = atoi(line);
		i++;
	}
	fclose(fp);

	return 0;
}

// ギルドデータの読み込み
int inter_guild_init()
{
	char line[16384];
	struct guild *g;
	struct guild_castle *gc;
	FILE *fp;
	int i, j, c = 0;

	inter_guild_readdb();

	guild_db = numdb_init();
	castle_db = numdb_init();

	if ((fp = safefopen(guild_txt,"r")) == NULL)
		return 1;
	while(fgets(line, sizeof(line), fp)) {
		j = 0;
		if (sscanf(line, "%d\t%%newid%%\n%n", &i, &j) == 1 && j > 0 && guild_newid <= (uint32)i) {
			guild_newid = i;
			continue;
		}
		g = (struct guild*)aCalloc(1, sizeof(struct guild));
		if (inter_guild_fromstr(line, g) == 0 && g->guild_id > 0) {
			if (g->guild_id >= guild_newid)
				guild_newid = g->guild_id + 1;
			numdb_insert(guild_db, g->guild_id, g);
			guild_check_empty(g);
			guild_calcinfo(g);
		} else {
			ShowMessage("int_guild: broken data [%s] line %d\n", guild_txt, c);
			aFree(g);
		}
		c++;
	}
	fclose(fp);
//	ShowMessage("int_guild: %s read done (%d guilds)\n", guild_txt, c);

	c = 0;//カウンタ初期化

	if ((fp = safefopen(castle_txt, "r")) == NULL) {
		return 1;
	}

	while(fgets(line, sizeof(line), fp)) {
		gc = (struct guild_castle*)aCalloc(1, sizeof(struct guild_castle));
		if (inter_guildcastle_fromstr(line, gc) == 0) {
			numdb_insert(castle_db, gc->castle_id, gc);
		} else {
			ShowMessage("int_guild: broken data [%s] line %d\n", castle_txt, c);
			aFree(gc);
		}
		c++;
	}
	fclose(fp);

	//デフォルトデータを作成
	for(i = 0; i < MAX_GUILDCASTLE; i++) {
		// check if castle i exist
		gc = (struct guild_castle*)numdb_search(castle_db, i);
		if( NULL==gc )
		{	// construct a new one if not
			gc = (struct guild_castle*)aCalloc(1, sizeof(struct guild_castle));
			gc->castle_id = i;
			// the rest should be not necessary since calloc set the structure to zero anyway
			gc->guild_id = 0;
			gc->economy = 0;
			gc->defense = 0;
			gc->triggerE = 0;
			gc->triggerD = 0;
			gc->nextTime = 0;
			gc->payTime = 0;
			gc->createTime = 0;
			gc->visibleC = 0;
			gc->visibleG0 = 0;
			gc->visibleG1 = 0;
			gc->visibleG2 = 0;
			gc->visibleG3 = 0;
			gc->visibleG4 = 0;
			gc->visibleG5 = 0;
			gc->visibleG6 = 0;
			gc->visibleG7 = 0;
			gc->Ghp0 = 0;	// guardian HP [Valaris]
			gc->Ghp1 = 0;
			gc->Ghp2 = 0;
			gc->Ghp3 = 0;
			gc->Ghp4 = 0;
			gc->Ghp5 = 0;
			gc->Ghp6 = 0;
			gc->Ghp7 = 0;	// end additions [Valaris]
			numdb_insert(castle_db, gc->castle_id, gc);
		}
	}
	return 0;
}

int castle_db_final (void *k, void *data)
{
	struct guild_castle *gc = (guild_castle *)data;
	if (gc) aFree(gc);
	return 0;
}
int guild_db_final (void *k, void *data)
{
	struct guild *g = (struct guild *)data;
	if (g) aFree(g);
	return 0;
}
void inter_guild_final()
{
	if(castle_db)
	{
		numdb_final(castle_db, castle_db_final);
		castle_db=NULL;
	}
	if(guild_db)
	{
		numdb_final(guild_db, guild_db_final);
		guild_db=NULL;
	}
	return;
}

struct guild *inter_guild_search(uint32 guild_id)
{
	struct guild *g;
	g=(struct guild *)numdb_search(guild_db, guild_id);
	return g;
}

// ギルドデータのセーブ用
/*
int inter_guild_save_sub(void *key,void *data,va_list &ap)
{
	char line[16384];
	FILE *fp;

	inter_guild_tostr(line,(struct guild *)data);
	fp = va_arg(ap,FILE*);
	fprintf(fp,"%s" RETCODE,line);

	return 0;
}
*/
// ギルド城データのセーブ用
/*
int inter_castle_save_sub(void *key, void *data, va_list &ap)
{
	char line[16384];
	FILE *fp;

	inter_guildcastle_tostr(line, (struct guild_castle *)data);
	fp = va_arg(ap, FILE*);
	fprintf(fp, "%s" RETCODE, line);

	return 0;
}
*/
class CDBguild_save : public CDBProcessor
{
	FILE *fp;
	mutable char line[65536];
public:
	CDBguild_save(FILE *f) : fp(f)			{}
	virtual ~CDBguild_save()	{}
	virtual bool process(void *key, void *data) const
	{
		inter_guild_tostr(line,(struct guild *)data);
		fprintf(fp,"%s" RETCODE,line);
		return 0;
	}
};
class CDBcastle_save : public CDBProcessor
{
	FILE *fp;
	mutable char line[65536];
public:
	CDBcastle_save(FILE *f) : fp(f)			{}
	virtual ~CDBcastle_save()	{}
	virtual bool process(void *key, void *data) const
	{
		inter_guildcastle_tostr(line, (struct guild_castle *)data);
		fprintf(fp, "%s" RETCODE, line);
		return 0;
	}
};
// ギルドデータのセーブ
int inter_guild_save()
{
	FILE *fp;
	int lock;

	if ((fp = lock_fopen(guild_txt, &lock)) == NULL) {
		ShowMessage("int_guild: cant write [%s] !!! data is lost !!!\n", guild_txt);
		return 1;
	}
	numdb_foreach(guild_db, CDBguild_save(fp) );
//	numdb_foreach(guild_db, inter_guild_save_sub, fp);
//	fprintf(fp, "%d\t%%newid%%\n", guild_newid);
	lock_fclose(fp, guild_txt, &lock);
//	ShowMessage("int_guild: %s saved.\n", guild_txt);

	if ((fp = lock_fopen(castle_txt,&lock)) == NULL) {
		ShowMessage("int_guild: cant write [%s] !!! data is lost !!!\n", castle_txt);
		return 1;
	}
	numdb_foreach(castle_db, CDBcastle_save(fp) );
//	numdb_foreach(castle_db, inter_castle_save_sub, fp);
	lock_fclose(fp, castle_txt, &lock);

	return 0;
}

// ギルド名検索用
/*
int search_guildname_sub(void *key, void *data, va_list &ap)
{
	struct guild *g = (struct guild *)data, **dst;
	char *str;

	str = va_arg(ap, char *);
	dst = va_arg(ap, struct guild **);
	if (dst && g && g->name && str && strcasecmp(g->name, str) == 0)
		*dst = g;
	return 0;
}
*/
class CDBsearch_guildname : public CDBProcessor
{
	const char *str;
	struct guild *&dst;
public:
	CDBsearch_guildname(const char *s, struct guild *&p) : 	str(s), dst(p)	{}
	virtual ~CDBsearch_guildname()	{}
	virtual bool process(void *key, void *data) const
	{
		struct guild *g = (struct guild *)data;
		if(g && g->name && strcasecmp(g->name, str) == 0)
		{
			dst = g;
			return false;
		}
		return true;
	}
};

// ギルド名検索
struct guild* search_guildname(char *str)
{
	struct guild *g = NULL;
	if(str)
		numdb_foreach(guild_db, CDBsearch_guildname(str, g) );
//	numdb_foreach(guild_db, search_guildname_sub, str, &g);
	return g;
}




// ギルド解散処理用（同盟/敵対を解除）
/*
int guild_break_sub(void *key, void *data, va_list &ap)
{
	struct guild *g = (struct guild *)data;
	uint32 guild_id = va_arg(ap, uint32);
	int i;

	for(i = 0; i < MAX_GUILDALLIANCE; i++) {
		if (g->alliance[i].guild_id == guild_id)
			g->alliance[i].guild_id = 0;
	}
	return 0;
}
*/
class CDBguild_break : public CDBProcessor
{
	uint32 guild_id;
public:
	CDBguild_break(uint32 gid) : guild_id(gid)	{}
	virtual ~CDBguild_break()	{}
	virtual bool process(void *key, void *data) const
	{
		struct guild *g = (struct guild *)data;
		int i;
		for(i = 0; i < MAX_GUILDALLIANCE; i++)
		{
			if (g->alliance[i].guild_id == guild_id)
				g->alliance[i].guild_id = 0;
		}
		return true;
	}
};
// ギルドが空かどうかチェック
int guild_check_empty(struct guild *g)
{
	int i;

	for(i = 0; i < g->max_member; i++) {
		if (g->member[i].account_id > 0) {
			return 0;
		}
	}
		// 誰もいないので解散
	
	numdb_foreach(guild_db, CDBguild_break(g->guild_id) );
//	numdb_foreach(guild_db, guild_break_sub, g->guild_id);
	numdb_erase(guild_db, g->guild_id);
	inter_guild_storage_delete(g->guild_id);
	mapif_guild_broken(g->guild_id, 0);
	aFree(g);

	return 1;
}

// キャラの競合がないかチェック用
/*
int guild_check_conflict_sub(void *key, void *data, va_list &ap)
{
	struct guild *g = (struct guild *)data;
	uint32 guild_id, account_id, char_id;
	size_t i;

	guild_id = va_arg(ap, uint32);
	account_id = va_arg(ap, uint32);
	char_id = va_arg(ap, uint32);

	if (g->guild_id == guild_id)	// 本来の所属なので問題なし
		return 0;

	for(i = 0; i < MAX_GUILD; i++) {
		if (g->member[i].account_id == account_id && g->member[i].char_id == char_id) {
			// 別のギルドに偽の所属データがあるので脱退
			ShowMessage("int_guild: guild conflict! %d,%d %d!=%d\n", 
				account_id, char_id, guild_id, g->guild_id);
			mapif_parse_GuildLeave(-1, g->guild_id, account_id, char_id, 0, "**データ競合**");
		}
	}
	return 0;
}
*/
class CDBguild_check_conflict : public CDBProcessor
{
	uint32 guild_id;
	uint32 account_id;
	uint32 char_id;
public:
	CDBguild_check_conflict(uint32 gid, uint32 aid,  uint32 cid)
		: guild_id(gid), account_id(aid), char_id(cid) 	{}
	virtual ~CDBguild_check_conflict()	{}
	virtual bool process(void *key, void *data) const
	{
		struct guild *g = (struct guild *)data;
		size_t i;

		if (g->guild_id != guild_id)	// 本来の所属なので問題なし
		{
			for(i = 0; i < MAX_GUILD; i++)
			{
				if (g->member[i].account_id == account_id && g->member[i].char_id == char_id) {
					// 別のギルドに偽の所属データがあるので脱退
					ShowMessage("int_guild: guild conflict! %d,%d %d!=%d\n", 
						account_id, char_id, guild_id, g->guild_id);
					mapif_parse_GuildLeave(-1, g->guild_id, account_id, char_id, 0, "**データ競合**");
				}
			}
		}
		return true;
	}
};
// キャラの競合がないかチェック
int guild_check_conflict(uint32 guild_id, uint32 account_id, uint32 char_id)
{
	numdb_foreach(guild_db, CDBguild_check_conflict(guild_id, account_id, char_id) );
//	numdb_foreach(guild_db, guild_check_conflict_sub, guild_id, account_id, char_id);
	return 0;
}

int guild_nextexp(int level)
{
	if (level < 100)
		return guild_exp[level-1];
	return 0;
}

// ギルドスキルがあるか確認
int guild_checkskill(struct guild &g, unsigned short id)
{
	unsigned short idx = id - GD_SKILLBASE;
	if(idx >= MAX_GUILDSKILL)
		return 0;
	return g.skill[idx].lv;
}

// ギルドの情報の再計算
int guild_calcinfo(struct guild *g)
{
	if(g)
	{
		size_t i, c;
		uint32 nextexp;
		struct guild before = *g;
		// スキルIDの設定
		for(i = 0; i < MAX_GUILDSKILL; i++)
			g->skill[i].id=i+GD_SKILLBASE;

		// ギルドレベル
		if (g->guild_lv <= 0)
			g->guild_lv = 1;
		nextexp = guild_nextexp(g->guild_lv);
		while(g->exp >= nextexp && nextexp>0)
		{
			g->exp -= nextexp;
			g->guild_lv++;
			g->skill_point++;
			nextexp = guild_nextexp(g->guild_lv);
		}
		// ギルドの次の経験値
		g->next_exp = guild_nextexp(g->guild_lv);

		// メンバ上限（ギルド拡張適用）
		g->max_member = 16 + guild_checkskill(*g, GD_EXTENSION) * 6; //  Guild Extention skill - adds by 6 people per level to Max Member [Lupus]

		// 平均レベルとオンライン人数
		g->average_lv = 0;
		g->connect_member = 0;
		c = 0;
		for(i = 0; i < g->max_member; i++) {
			if (g->member[i].account_id > 0) {
				g->average_lv += g->member[i].lv;
				c++;
				if (g->member[i].online > 0)
					g->connect_member++;
			}
		}
		if(c) g->average_lv /= c;

		// 全データを送る必要がありそう
		if( g->max_member != before.max_member ||
			g->guild_lv != before.guild_lv ||
			g->skill_point != before.skill_point) {
			mapif_guild_info(-1, g);
			return 1;
		}
	}
	return 0;
}

//-------------------------------------------------------------------
// map serverへの通信

// ギルド作成可否
int mapif_guild_created(int fd, uint32 account_id, struct guild *g)
{
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0) = 0x3830;
	WFIFOL(fd,2) = account_id;
	if (g != NULL) {
		WFIFOL(fd,6) = g->guild_id;
		ShowMessage("int_guild: created! %d %s\n", g->guild_id, g->name);
	}else{
		WFIFOL(fd,6) = 0;
	}
	WFIFOSET(fd,10);
	return 0;
}

// ギルド情報見つからず
int mapif_guild_noinfo(int fd, int guild_id)
{
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0) = 0x3831;
	WFIFOW(fd,2) = 8;
	WFIFOL(fd,4) = guild_id;
	WFIFOSET(fd,8);
	//ShowError("int_guild: info not found %d\n", guild_id);

	return 0;
}

// ギルド情報まとめ送り
int mapif_guild_info(int fd, struct guild *g)
{
	if(g)
	{
		unsigned char buf[16384];
		WBUFW(buf,0) = 0x3831;
		WBUFW(buf,2) = 4 + sizeof(struct guild);
		guild_tobuffer(*g, WBUFP(buf,4));
		if( !session_isActive(fd) )
			mapif_sendall(buf, 4 + sizeof(struct guild));
		else
			mapif_send(fd, buf, 4 + sizeof(struct guild));
		//ShowMessage("int_guild: info %d %s\n", g->guild_id, g->name);
	}
	return 0;
}

// メンバ追加可否
int mapif_guild_memberadded(int fd, uint32 guild_id, uint32 account_id, uint32 char_id, int flag)
{
	if( !session_isActive(fd) )
		return 0;

	WFIFOW(fd,0) = 0x3832;
	WFIFOL(fd,2) = guild_id;
	WFIFOL(fd,6) = account_id;
	WFIFOL(fd,10) = char_id;
	WFIFOB(fd,14) = flag;
	WFIFOSET(fd, 15);

	return 0;
}

// 脱退/追放通知
int mapif_guild_leaved(uint32 guild_id, uint32 account_id, uint32 char_id, int flag, const char *name, const char *mes)
{
	unsigned char buf[79];

	WBUFW(buf, 0) = 0x3834;
	WBUFL(buf, 2) = guild_id;
	WBUFL(buf, 6) = account_id;
	WBUFL(buf,10) = char_id;
	WBUFB(buf,14) = flag;
	memcpy(WBUFP(buf,15), mes, 40);
	memcpy(WBUFP(buf,55), name, 24);
	mapif_sendall(buf, 79);
	ShowMessage("int_guild: guild leaved %d %d %s %s\n", guild_id, account_id, name, mes);

	return 0;
}

// オンライン状態とLv更新通知
int mapif_guild_memberinfoshort(struct guild *g, int idx)
{
	unsigned char buf[19];
	WBUFW(buf, 0) = 0x3835;
	WBUFL(buf, 2) = g->guild_id;
	WBUFL(buf, 6) = g->member[idx].account_id;
	WBUFL(buf,10) = g->member[idx].char_id;
	WBUFB(buf,14) = (unsigned char)g->member[idx].online;
	WBUFW(buf,15) = g->member[idx].lv;
	WBUFW(buf,17) = g->member[idx].class_;
	mapif_sendall(buf, 19);
	return 0;
}

// 解散通知
int mapif_guild_broken(uint32 guild_id, int flag)
{
	unsigned char buf[7];

	WBUFW(buf,0) = 0x3836;
	WBUFL(buf,2) = guild_id;
	WBUFB(buf,6) = flag;
	mapif_sendall(buf, 7);
	ShowMessage("int_guild: broken %d\n", guild_id);

	return 0;
}

// ギルド内発言
int mapif_guild_message(uint32 guild_id, uint32 account_id, char *mes, size_t len, int sfd)
{
	CREATE_BUFFER(buf,unsigned char,len+12);

	WBUFW(buf,0) = 0x3837;
	WBUFW(buf,2) = len + 12;
	WBUFL(buf,4) = guild_id;
	WBUFL(buf,8) = account_id;
	memcpy(WBUFP(buf,12), mes, len);
	mapif_sendallwos(sfd, buf, len + 12);

	DELETE_BUFFER(buf);

	return 0;
}

// ギルド基本情報変更通知
int mapif_guild_basicinfochanged(uint32 guild_id, int type, uint32 data)
{
	unsigned char buf[2048];
	WBUFW(buf, 0) = 0x3839;
	WBUFW(buf, 2) = 14;
	WBUFL(buf, 4) = guild_id;
	WBUFW(buf, 8) = type;
	WBUFL(buf,10) = data;
	mapif_sendall(buf,14);
	return 0;
}

// ギルドメンバ情報変更通知
int mapif_guild_memberinfochanged(uint32 guild_id, uint32 account_id, uint32 char_id, int type, uint32 data)
{
	unsigned char buf[22];
	WBUFW(buf, 0) = 0x383a;
	WBUFW(buf, 2) = 22;
	WBUFL(buf, 4) = guild_id;
	WBUFL(buf, 8) = account_id;
	WBUFL(buf,12) = char_id;
	WBUFW(buf,16) = type;
	WBUFL(buf,18) = data;
	mapif_sendall(buf,22);
	return 0;
}

// ギルドスキルアップ通知
int mapif_guild_skillupack(uint32 guild_id, uint32 skill_num, uint32 account_id)
{
	unsigned char buf[14];

	WBUFW(buf, 0) = 0x383c;
	WBUFL(buf, 2) = guild_id;
	WBUFL(buf, 6) = skill_num;
	WBUFL(buf,10) = account_id;
	mapif_sendall(buf, 14);

	return 0;
}

// ギルド同盟/敵対通知
int mapif_guild_alliance(uint32 guild_id1, uint32 guild_id2, uint32 account_id1, uint32 account_id2, int flag, const char *name1, const char *name2)
{
	unsigned char buf[128];

	WBUFW(buf, 0) = 0x383d;
	WBUFL(buf, 2) = guild_id1;
	WBUFL(buf, 6) = guild_id2;
	WBUFL(buf,10) = account_id1;
	WBUFL(buf,14) = account_id2;
	WBUFB(buf,18) = flag;
	memcpy(WBUFP(buf,19), name1, 24);
	memcpy(WBUFP(buf,43), name2, 24);
	mapif_sendall(buf, 67);

	return 0;
}

// ギルド役職変更通知
int mapif_guild_position(struct guild *g, size_t idx)
{
	if(g && idx < MAX_GUILDPOSITION)
	{
		unsigned char buf[128];
		WBUFW(buf,0) = 0x383b;
		WBUFW(buf,2) = sizeof(struct guild_position) + 12;
		WBUFL(buf,4) = g->guild_id;
		WBUFL(buf,8) = idx;
		guild_position_tobuffer(g->position[idx], WBUFP(buf,12));
		mapif_sendall(buf, sizeof(struct guild_position) + 12);
	}
	return 0;
}

// ギルド告知変更通知
int mapif_guild_notice(struct guild *g)
{
	unsigned char buf[186];

	WBUFW(buf,0) = 0x383e;
	WBUFL(buf,2) = g->guild_id;
	memcpy(WBUFP(buf,6), g->mes1, 60);
	memcpy(WBUFP(buf,66), g->mes2, 120);
	mapif_sendall(buf, 186);

	return 0;
}

// ギルドエンブレム変更通知
int mapif_guild_emblem(struct guild *g)
{
	unsigned char buf[2048+12];

	WBUFW(buf,0) = 0x383f;
	WBUFW(buf,2) = g->emblem_len + 12;
	WBUFL(buf,4) = g->guild_id;
	WBUFL(buf,8) = g->emblem_id;
	memcpy(WBUFP(buf,12), g->emblem_data, g->emblem_len);
	mapif_sendall(buf, WBUFW(buf,2));

	return 0;
}

int mapif_guild_castle_dataload(unsigned short castle_id, int index, int value)
{
	unsigned char buf[9];

	WBUFW(buf,0) = 0x3840;
	WBUFW(buf,2) = castle_id;
	WBUFB(buf,4) = index;
	WBUFL(buf,5) = value;
	mapif_sendall(buf,9);

	return 0;
}

int mapif_guild_castle_datasave(int castle_id, int index, int value)
{
	unsigned char buf[9];

	WBUFW(buf,0) = 0x3841;
	WBUFW(buf,2) = castle_id;
	WBUFB(buf,4) = index;
	WBUFL(buf,5) = value;
	mapif_sendall(buf,9);

	return 0;
}
/*
int mapif_guild_castle_alldataload_sub(void *key, void *data, va_list &ap)
{
	int fd = va_arg(ap, int);
	int *offset = va_arg(ap, int*);

	if( !session_isActive(fd) )
		return 0;

	//memcpy(WFIFOP(fd,*offset), (struct guild_castle*)data, sizeof(struct guild_castle));

	if(data) guild_castle_tobuffer( *((struct guild_castle*)data), WFIFOP(fd,*offset));
	(*offset) += sizeof(struct guild_castle);
	return 0;
}
*/
class CDBguild_castle_alldataload : public CDBProcessor
{
	int fd;
	size_t &offset;
public:
	CDBguild_castle_alldataload(int f, size_t o) : fd(f), offset(o)	{}
	virtual ~CDBguild_castle_alldataload()	{}
	virtual bool process(void *key, void *data) const
	{
		if(data)
			guild_castle_tobuffer( *((struct guild_castle*)data), WFIFOP(fd,offset));
		offset += sizeof(struct guild_castle);
		return true;
	}
};
int mapif_guild_castle_alldataload(int fd)
{
	size_t len = 4;
	if( !session_isActive(fd) )
		return 0;
	WFIFOW(fd,0) = 0x3842;
	numdb_foreach(castle_db, CDBguild_castle_alldataload(fd, len) );
//	numdb_foreach(castle_db, mapif_guild_castle_alldataload_sub, fd, &len);
	WFIFOW(fd,2) = len;
	WFIFOSET(fd, len);
	return 0;
}

//-------------------------------------------------------------------
// map serverからの通信

// ギルド作成要求
int mapif_parse_CreateGuild(int fd, uint32 account_id, char *name, unsigned char *buf) {
	struct guild *g;
	int i;

	for(i = 0; i < 24 && name[i]; i++) {
		if (!(name[i] & 0xe0) || name[i] == 0x7f) {
			ShowMessage("int_guild: illeagal guild name [%s]\n", name);
			mapif_guild_created(fd, account_id, NULL);
			return 0;
		}
	}

	if ((g = search_guildname(name)) != NULL) {
		ShowMessage("int_guild: same name guild exists [%s]\n", name);
		mapif_guild_created(fd, account_id, NULL);
		return 0;
	}
	g = (struct guild*)aCalloc(1, sizeof(struct guild));
	g->guild_id = guild_newid++;
	memcpy(g->name, name, 24);
	guild_member_frombuffer(g->member[0], buf);
	memcpy(g->master, g->member[0].name, 24);

	g->position[0].mode = 0x11;
	strcpy(g->position[                  0].name, "GuildMaster");
	strcpy(g->position[MAX_GUILDPOSITION-1].name, "Newbie");
	for(i = 1; i < MAX_GUILDPOSITION-1; i++)
		sprintf(g->position[i].name, "Position %d", i + 1);

	// ここでギルド情報計算が必要と思われる
	g->max_member = 16;
	g->average_lv = g->member[0].lv;
	for(i = 0; i < MAX_GUILDSKILL; i++)
		g->skill[i].id=i + GD_SKILLBASE;

	numdb_insert(guild_db, g->guild_id, g);

	mapif_guild_created(fd, account_id, g);
	mapif_guild_info(fd, g);

	if(log_inter)
		inter_log("guild %s (id=%d) created by master %s (id=%d)" RETCODE,
			name, g->guild_id, g->member[0].name, g->member[0].account_id);

	return 0;
}

// ギルド情報要求
int mapif_parse_GuildInfo(int fd, int guild_id)
{
	struct guild *g;

	g = (struct guild *)numdb_search(guild_db, guild_id);
	if (g != NULL){
		guild_calcinfo(g);
		mapif_guild_info(fd, g);
	} else
		mapif_guild_noinfo(fd, guild_id);

	return 0;
}

// ギルドメンバ追加要求
int mapif_parse_GuildAddMember(int fd, int guild_id, unsigned char *buf)
{
	struct guild *g;
	struct guild_member member;
	int i;

	guild_member_frombuffer(member,buf);
	g = (struct guild *)numdb_search(guild_db, guild_id);
	if (g == NULL) {
		mapif_guild_memberadded(fd, guild_id, member.account_id, member.char_id, 1);
		return 0;
	}

	for(i = 0; i < g->max_member; i++)
	{
		if (g->member[i].account_id == 0)
		{
			memcpy(&g->member[i], &member, sizeof(struct guild_member));
			mapif_guild_memberadded(fd, guild_id, member.account_id, member.char_id, 0);
			guild_calcinfo(g);
			mapif_guild_info(-1, g);
			return 0;
		}
	}
	mapif_guild_memberadded(fd, guild_id, member.account_id, member.char_id, 1);

	return 0;
}

// ギルド脱退/追放要求
int mapif_parse_GuildLeave(int fd, uint32 guild_id, uint32 account_id, uint32 char_id, int flag, const char *mes) {
	struct guild *g = NULL;
	int i, j;

	g = (struct guild *)numdb_search(guild_db, guild_id);
	if (g != NULL) {
		for(i = 0; i < MAX_GUILD; i++) {
			if (g->member[i].account_id == account_id && g->member[i].char_id == char_id) {
//				ShowMessage("%d %p\n", i, (&g->member[i]));
//				ShowMessage("%d %s\n", i, g->member[i].name);

				if (flag) {	// 追放の場合追放リストに入れる
					for(j = 0; j < MAX_GUILDEXPLUSION; j++) {
						if (g->explusion[j].account_id == 0)
							break;
					}
					if (j == MAX_GUILDEXPLUSION) {	// 一杯なので古いのを消す
						for(j = 0; j < MAX_GUILDEXPLUSION - 1; j++)
							g->explusion[j] = g->explusion[j+1];
						j = MAX_GUILDEXPLUSION - 1;
					}
					g->explusion[j].account_id = account_id;
					g->explusion[j].char_id    = char_id;
					memcpy(g->explusion[j].acc, "dummy", 24);
					memcpy(g->explusion[j].name, g->member[i].name, 24);
					memcpy(g->explusion[j].mes, mes, 40);
					g->explusion[j].mes[39]=0;
				}
				mapif_guild_leaved(guild_id, account_id, char_id, flag, g->member[i].name, mes);
				memset(&g->member[i], 0, sizeof(struct guild_member));

				if (guild_check_empty(g) == 0)
					mapif_guild_info(-1,g);// まだ人がいるのでデータ送信

				return 0;
			}
		}
	}
	return 0;
}

// オンライン/Lv更新
int mapif_parse_GuildChangeMemberInfoShort(int fd, uint32 guild_id, uint32 account_id, uint32 char_id, int online, int lv, int class_)
{
	struct guild *g;
	int i, alv, c;
	g = (struct guild *)numdb_search(guild_db, guild_id);
	if (g == NULL)
		return 0;

	g->connect_member = 0;

	alv = 0;
	c = 0;
	for(i = 0; i < MAX_GUILD; i++) {
		if (g->member[i].account_id == account_id && g->member[i].char_id == char_id) {
			g->member[i].online = online;
			g->member[i].lv = lv;
			g->member[i].class_ = class_;
			mapif_guild_memberinfoshort(g, i);
		}
		if (g->member[i].account_id > 0) {
			alv += g->member[i].lv;
			c++;
		}
		if (g->member[i].online)
			g->connect_member++;
	}
	
	if (c)
		// 平均レベル
		g->average_lv = alv / c;

	return 0;
}

// ギルド解散要求
int mapif_parse_BreakGuild(int fd, int guild_id)
{
	struct guild *g;

	g = (struct guild *)numdb_search(guild_db, guild_id);
	if(g == NULL)
		return 0;

	numdb_foreach(guild_db, CDBguild_break(guild_id) );
//	numdb_foreach(guild_db, guild_break_sub, guild_id);
	numdb_erase(guild_db, guild_id);
	inter_guild_storage_delete(guild_id);
	mapif_guild_broken(guild_id, 0);

	if(log_inter)
		inter_log("guild %s (id=%d) broken" RETCODE, g->name, guild_id);
	aFree(g);

	return 0;
}

// ギルドメッセージ送信
int mapif_parse_GuildMessage(int fd, uint32 guild_id, uint32 account_id, char *mes, size_t len)
{
	return mapif_guild_message(guild_id, account_id, mes, len, fd);
}

// ギルド基本データ変更要求
int mapif_parse_GuildBasicInfoChange(int fd, uint32 guild_id, int type, uint32 data)
{
	struct guild *g;

	g = (struct guild *)numdb_search(guild_db, guild_id);
	if (g == NULL)
		return 0;

	switch(type) {
	case GBI_GUILDLV:
		if(g->guild_lv + data <= 50) {
			g->guild_lv+=data;
			g->skill_point+=data;
		} else if (g->guild_lv + data >= 1)
			g->guild_lv += data;
		mapif_guild_info(-1, g);
		return 0;
	default:
		ShowMessage("int_guild: GuildBasicInfoChange: Unknown type %d\n", type);
		break;
	}
	mapif_guild_basicinfochanged(guild_id, type, data);

	return 0;
}

// ギルドメンバデータ変更要求
int mapif_parse_GuildMemberInfoChange(int fd, uint32 guild_id, uint32 account_id, uint32 char_id, unsigned short type, uint32 data)
{
	int i;
	struct guild *g;
	
	g = (struct guild *)numdb_search(guild_db, guild_id);
	if(g == NULL)
		return 0;
	
	for(i = 0; i < g->max_member; i++)
	{
		if (g->member[i].account_id == account_id && g->member[i].char_id == char_id)
			break;
	}
	if (i == g->max_member)
	{
		ShowError("int_guild: GuildMemberChange: Not found %d,%d in %d[%s]\n", account_id, char_id, guild_id, g->name);
		return 0;
	}

	switch(type)
	{
	case GMI_POSITION:	// 役職
		g->member[i].position = data;
		break;
	case GMI_EXP:	// EXP
	{
		int exp, oldexp = g->member[i].exp;
		exp = g->member[i].exp = data;
		g->exp += (exp - oldexp);
		guild_calcinfo(g);	// Lvアップ判断
		mapif_guild_basicinfochanged(guild_id, GBI_EXP, g->exp);
		break;
	}
	default:
		ShowMessage("int_guild: GuildMemberInfoChange: Unknown type %d\n", type);
		break;
	}
	mapif_guild_memberinfochanged(guild_id, account_id, char_id, type, data);
	
	return 0;
}

// ギルド役職名変更要求
int mapif_parse_GuildPosition(int fd, uint32 guild_id, uint32 idx, unsigned char *buf)
{
	struct guild *g = (struct guild *)numdb_search(guild_db, guild_id);
	if(g && idx < MAX_GUILDPOSITION)
	{
		guild_position_frombuffer(g->position[idx],buf);

		mapif_guild_position(g, idx);
		ShowMessage("int_guild: position changed %d\n", idx);
	}
	return 0;
}

// ギルドスキルアップ要求
int mapif_parse_GuildSkillUp(int fd, uint32 guild_id, uint32 skillid, uint32 account_id)
{
	struct guild *g = (struct guild *)numdb_search(guild_db, guild_id);
	int skillidx = skillid - GD_SKILLBASE;

	if (g == NULL || skillidx < 0 || skillidx >= MAX_GUILDSKILL)
		return 0;

	if (g->skill_point > 0 && g->skill[skillidx].id > 0 && g->skill[skillidx].lv < 10) {
		g->skill[skillidx].lv++;
		g->skill_point--;
		if (guild_calcinfo(g) == 0)
			mapif_guild_info(-1, g);
		mapif_guild_skillupack(guild_id, skillid, account_id);
		ShowMessage("int_guild: skill %d up\n", skillid);
	}

	return 0;
}

// ギルド同盟要求
int mapif_parse_GuildAlliance(int fd, uint32 guild_id1, uint32 guild_id2, uint32 account_id1, uint32 account_id2, int flag)
{
	struct guild *g[2];
	int j, i;

	g[0] = (struct guild *)numdb_search(guild_db, guild_id1);
	g[1] = (struct guild *)numdb_search(guild_db, guild_id2);
	if (g[0] == NULL || g[1] == NULL)
		return 0;

	if (!(flag & 0x8)) {
		for(i = 0; i < 2 - (flag & 1); i++) {
			for(j = 0; j < MAX_GUILDALLIANCE; j++)
				if (g[i]->alliance[j].guild_id == 0) {
					g[i]->alliance[j].guild_id = g[1-i]->guild_id;
					memcpy(g[i]->alliance[j].name, g[1-i]->name, 24);
					g[i]->alliance[j].opposition = flag & 1;
					break;
				}
		}
	} else {	// 関係解消
		for(i = 0; i < 2 - (flag & 1); i++) {
			for(j = 0; j < MAX_GUILDALLIANCE; j++)
				if (g[i]->alliance[j].guild_id == g[1-i]->guild_id && g[i]->alliance[j].opposition == (flag & 1)) {
					g[i]->alliance[j].guild_id = 0;
					break;
				}
		}
	}
	mapif_guild_alliance(guild_id1, guild_id2, account_id1, account_id2, flag, g[0]->name, g[1]->name);

	return 0;
}

// ギルド告知変更要求
int mapif_parse_GuildNotice(int fd, int guild_id, const char *mes1, const char *mes2)
{
	struct guild *g;

	g = (struct guild *)numdb_search(guild_db, guild_id);
	if (g == NULL)
		return 0;
	memcpy(g->mes1, mes1, 60);
	memcpy(g->mes2, mes2, 120);

	return mapif_guild_notice(g);
}

// ギルドエンブレム変更要求
int mapif_parse_GuildEmblem(int fd, int len, int guild_id, int dummy, const char *data)
{
	struct guild *g;

	g = (struct guild *)numdb_search(guild_db, guild_id);
	if (g == NULL)
		return 0;
	if(len>2048) len = 2048;// test for buffer limit
	memcpy(g->emblem_data, data, len);
	g->emblem_len = len;
	g->emblem_id++;

	return mapif_guild_emblem(g);
}

int mapif_parse_GuildCastleDataLoad(int fd, int castle_id, int index)
{

	struct guild_castle *gc = (struct guild_castle *)numdb_search(castle_db, castle_id);

	if (gc == NULL) {
		// this is causing the loadack error
		ShowMessage("called castle %i, index %i; does not exist.\n", castle_id, index);
		return mapif_guild_castle_dataload(castle_id, 0, 0);
	}
	switch(index) {
	case 1: return mapif_guild_castle_dataload(gc->castle_id, index, gc->guild_id);
	case 2: return mapif_guild_castle_dataload(gc->castle_id, index, gc->economy);
	case 3: return mapif_guild_castle_dataload(gc->castle_id, index, gc->defense);
	case 4: return mapif_guild_castle_dataload(gc->castle_id, index, gc->triggerE);
	case 5: return mapif_guild_castle_dataload(gc->castle_id, index, gc->triggerD);
	case 6: return mapif_guild_castle_dataload(gc->castle_id, index, gc->nextTime);
	case 7: return mapif_guild_castle_dataload(gc->castle_id, index, gc->payTime);
	case 8: return mapif_guild_castle_dataload(gc->castle_id, index, gc->createTime);
	case 9: return mapif_guild_castle_dataload(gc->castle_id, index, gc->visibleC);
	case 10: return mapif_guild_castle_dataload(gc->castle_id, index, gc->visibleG0);
	case 11: return mapif_guild_castle_dataload(gc->castle_id, index, gc->visibleG1);
	case 12: return mapif_guild_castle_dataload(gc->castle_id, index, gc->visibleG2);
	case 13: return mapif_guild_castle_dataload(gc->castle_id, index, gc->visibleG3);
	case 14: return mapif_guild_castle_dataload(gc->castle_id, index, gc->visibleG4);
	case 15: return mapif_guild_castle_dataload(gc->castle_id, index, gc->visibleG5);
	case 16: return mapif_guild_castle_dataload(gc->castle_id, index, gc->visibleG6);
	case 17: return mapif_guild_castle_dataload(gc->castle_id, index, gc->visibleG7);
	case 18: return mapif_guild_castle_dataload(gc->castle_id, index, gc->Ghp0);	// guardian HP [Valaris]
	case 19: return mapif_guild_castle_dataload(gc->castle_id, index, gc->Ghp1);
	case 20: return mapif_guild_castle_dataload(gc->castle_id, index, gc->Ghp2);
	case 21: return mapif_guild_castle_dataload(gc->castle_id, index, gc->Ghp3);
	case 22: return mapif_guild_castle_dataload(gc->castle_id, index, gc->Ghp4);
	case 23: return mapif_guild_castle_dataload(gc->castle_id, index, gc->Ghp5);
	case 24: return mapif_guild_castle_dataload(gc->castle_id, index, gc->Ghp6);
	case 25: return mapif_guild_castle_dataload(gc->castle_id, index, gc->Ghp7);	// end additions [Valaris]

	default:
		ShowError("mapif_parse_GuildCastleDataLoad ERROR!! (Not found index=%d)\n", index);
		return 0;
	}
}

int mapif_parse_GuildCastleDataSave(int fd, int castle_id, int index, int value)
{
	struct guild_castle *gc=(struct guild_castle *)numdb_search(castle_db, castle_id);

	if (gc == NULL) {
		return mapif_guild_castle_datasave(castle_id, index, value);
	}
	switch(index) {
	case 1:
		if (gc->guild_id != (uint32)value) {
			int gid = (value) ? value : gc->guild_id;
			struct guild *g = (struct guild *)numdb_search(guild_db, gid);
			if(log_inter)
				inter_log("guild %s (id=%d) %s castle id=%d" RETCODE,
					(g) ? g->name : "??", gid, (value) ? "occupy" : "abandon", castle_id);
		}
		gc->guild_id = value;
		break;
	case 2: gc->economy = value; break;
	case 3: gc->defense = value; break;
	case 4: gc->triggerE = value; break;
	case 5: gc->triggerD = value; break;
	case 6: gc->nextTime = value; break;
	case 7: gc->payTime = value; break;
	case 8: gc->createTime = value; break;
	case 9: gc->visibleC = value; break;
	case 10: gc->visibleG0 = value; break;
	case 11: gc->visibleG1 = value; break;
	case 12: gc->visibleG2 = value; break;
	case 13: gc->visibleG3 = value; break;
	case 14: gc->visibleG4 = value; break;
	case 15: gc->visibleG5 = value; break;
	case 16: gc->visibleG6 = value; break;
	case 17: gc->visibleG7 = value; break;
	case 18: gc->Ghp0 = value; break;	// guardian HP [Valaris]
	case 19: gc->Ghp1 = value; break;
	case 20: gc->Ghp2 = value; break;
	case 21: gc->Ghp3 = value; break;
	case 22: gc->Ghp4 = value; break;
	case 23: gc->Ghp5 = value; break;
	case 24: gc->Ghp6 = value; break;
	case 25: gc->Ghp7 = value; break;	// end additions [Valaris]
	default:
		ShowError("mapif_parse_GuildCastleDataSave ERROR!! (Not found index=%d)\n", index);
		return 0;
	}

	return mapif_guild_castle_datasave(gc->castle_id, index, value);
}

// ギルドチェック要求
int mapif_parse_GuildCheck(int fd, uint32 guild_id, uint32 account_id, uint32 char_id)
{
	return guild_check_conflict(guild_id, account_id, char_id);
}

// マップサーバーの接続時処理
int inter_guild_mapif_init(int fd)
{
	return mapif_guild_castle_alldataload(fd);
}

// サーバーから脱退要求（キャラ削除用）
int inter_guild_leave(uint32 guild_id, uint32 account_id, uint32 char_id)
{
	return mapif_parse_GuildLeave(-1, guild_id, account_id, char_id, 0, "**サーバー命令**");
}


// map server からの通信
// ・１パケットのみ解析すること
// ・パケット長データはinter.cにセットしておくこと
// ・パケット長チェックや、RFIFOSKIPは呼び出し元で行われるので行ってはならない
// ・エラーなら0(false)、そうでないなら1(true)をかえさなければならない
int inter_guild_parse_frommap(int fd)
{
	if( !session_isActive(fd) )
		return 0;

	switch(RFIFOW(fd,0)) {
	case 0x3030: mapif_parse_CreateGuild(fd, RFIFOL(fd,4), (char*)RFIFOP(fd,8), RFIFOP(fd,32)); break;
	case 0x3031: mapif_parse_GuildInfo(fd, RFIFOL(fd,2)); break;
	case 0x3032: mapif_parse_GuildAddMember(fd, RFIFOL(fd,4), RFIFOP(fd,8)); break;
	case 0x3034: mapif_parse_GuildLeave(fd, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10), RFIFOB(fd,14), (char*)RFIFOP(fd,15)); break;
	case 0x3035: mapif_parse_GuildChangeMemberInfoShort(fd, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10), RFIFOB(fd,14), RFIFOW(fd,15), RFIFOW(fd,17)); break;
	case 0x3036: mapif_parse_BreakGuild(fd, RFIFOL(fd,2)); break;
	case 0x3037: mapif_parse_GuildMessage(fd, RFIFOL(fd,4), RFIFOL(fd,8), (char*)RFIFOP(fd,12), RFIFOW(fd,2)-12); break;
	case 0x3038: mapif_parse_GuildCheck(fd, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10)); break;
	case 0x3039: mapif_parse_GuildBasicInfoChange(fd, RFIFOL(fd,4), RFIFOW(fd,8), RFIFOL(fd,10)); break;
	case 0x303A: mapif_parse_GuildMemberInfoChange(fd, RFIFOL(fd,4), RFIFOL(fd,8), RFIFOL(fd,12), RFIFOW(fd,16), RFIFOL(fd,18)); break;
	case 0x303B: mapif_parse_GuildPosition(fd, RFIFOL(fd,4), RFIFOL(fd,8), RFIFOP(fd,12)); break;
	case 0x303C: mapif_parse_GuildSkillUp(fd, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10)); break;
	case 0x303D: mapif_parse_GuildAlliance(fd, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10), RFIFOL(fd,14), RFIFOB(fd,18)); break;
	case 0x303E: mapif_parse_GuildNotice(fd, RFIFOL(fd,2), (char*)RFIFOP(fd,6), (char*)RFIFOP(fd,66)); break;
	case 0x303F: mapif_parse_GuildEmblem(fd, RFIFOW(fd,2)-12, RFIFOL(fd,4), RFIFOL(fd,8), (char*)RFIFOP(fd,12)); break;
	case 0x3040: mapif_parse_GuildCastleDataLoad(fd, RFIFOW(fd,2), RFIFOB(fd,4)); break;
	case 0x3041: mapif_parse_GuildCastleDataSave(fd, RFIFOW(fd,2), RFIFOB(fd,4), RFIFOL(fd,5)); break;

	default:
		return 0;
	}

	return 1;
}
