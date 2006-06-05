#ifndef _DB_H_
#define _DB_H_

#include "basetypes.h"
#include "baseobjects.h"



#define HASH_SIZE (256+27)

#define RED 0
#define BLACK 1

struct dbn
{
	// tree elements
	struct dbn *parent;
	struct dbn *left;
	struct dbn *right;
	// doubled linked list elements
	struct dbn *next;
	struct dbn *prev;
	// balancing
	char color;
	char deleted;	// �폜�ς݃t���O(db_foreach)
	// node data
	void *key;
	void *data;
};
struct db_free
{
  struct dbn *z;
  struct dbn **root;
};

struct dbt
{
	ssize_t (*cmp)(struct dbt*,void*,void*);
	size_t (*hashfunc)(struct dbt*,void*);
    // which 1 - key,   2 - data,  3 - both
	void (*release)(struct dbn*,int which);
	int maxlen;
	struct dbn *ht[HASH_SIZE];
	struct dbn *head;
	struct dbn *tail;
	int item_count; // vf?
	const char* alloc_file; // DB?t@C
	int alloc_line; // DB?s
	// db_foreach ������db_erase �����΍�Ƃ��āA
	// db_foreach ���I���܂Ń��b�N���邱�Ƃɂ���
	struct db_free *free_list;
	int free_count;
	int free_max;
	int free_lock;
};

#define strdb_search(t,k)   db_search((t),(void*)((size_t)(k)))
#define strdb_insert(t,k,d) db_insert((t),(void*)((size_t)(k)),(void*)((size_t)(d)))
#define strdb_erase(t,k)    db_erase ((t),(void*)((size_t)(k)))
#define strdb_foreach       db_foreach
#define strdb_final         db_final
#define numdb_search(t,k)   db_search((t),(void*)((size_t)(k)))
#define numdb_insert(t,k,d) db_insert((t),(void*)((size_t)(k)),(void*)((size_t)(d)))
#define numdb_erase(t,k)    db_erase ((t),(void*)((size_t)(k)))
#define numdb_foreach       db_foreach
#define numdb_final         db_final
#define strdb_init(a)       strdb_init_(a,__FILE__,__LINE__)
#define numdb_init()        numdb_init_(__FILE__,__LINE__)

struct dbt* strdb_init_(int maxlen,const char *file,int line);
struct dbt* numdb_init_(const char *file,int line);

void* db_search(struct dbt *table,void* key);
void* db_search2(struct dbt *table, const char *key); // [MouseJstr]
struct dbn* db_insert(struct dbt *table,void* key,void* data);
void* db_erase(struct dbt *table,void* key);
//void db_foreach(struct dbt*,int(*)(void*,void*,va_list &),...);
//void db_final(struct dbt*,int(*)(void*,void*,va_list &),...);
void exit_dbn(void);


///////////////////////////////////////////////////////////////////////////////
// virtual class for class based data processing
// temporary solution for removing variable arguments
///////////////////////////////////////////////////////////////////////////////
class CDBProcessor : public basics::noncopyable
{
public:
	CDBProcessor()			{}
	virtual ~CDBProcessor()	{}
	virtual bool process(void *key, void *data) const	{ return false; };
};

void db_foreach(struct dbt* table, const CDBProcessor& elem);
void db_final(struct dbt*&table, const CDBProcessor& elem=CDBProcessor());
void db_final(struct dbt*&table, int(*)(void*,void*));

void db_free_lock(struct dbt *table);
void db_free_unlock(struct dbt *table);

class iterator
{
	struct dbt* table;
	struct dbn *curr;
public:
	iterator(struct dbt* t) : table(t), curr(NULL)
	{
		if(table)
		{
			db_free_lock(table);
			curr = table->head;
		}
	}
	iterator(const iterator& iter) : table(iter.table), curr(iter.curr)
	{
		if(table)
			db_free_lock(table);
	}
	const iterator& operator=(const iterator& iter)
	{
		if(table)
			db_free_unlock(table);
		table = iter.table;
		curr  = iter.curr;
		if(table)
			db_free_lock(table);
		return *this;
	}
	~iterator()
	{
		if(table)
			db_free_unlock(table);
	}
	iterator  operator++(int)	{ iterator temp(*this); next(); return temp; }
	iterator& operator++()		{ next(); return *this; }
	iterator  operator--(int)	{ iterator temp(*this); prev(); return temp; }
	iterator& operator--()		{ prev(); return *this;}
	bool next()					{ if(curr) curr=curr->next; return NULL!=curr; }
	bool prev()					{ if(curr) curr=curr->prev; return NULL!=curr; }

	operator const bool() const { return NULL!=curr; }
	bool isValid() const		{ return NULL!=curr; }

	void* key() const			{ return (curr) ? curr->key  : NULL; }
	void* data() const			{ return (curr) ? curr->data : NULL; }
};







#endif
