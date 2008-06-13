// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder
#include "nullpo.h"
#include "showmsg.h"
#include "utils.h"
#include "malloc.h"
#include "db.h"

#include "map.h"
#include "storage.h"
#include "chrif.h"
#include "itemdb.h"
#include "clif.h"
#include "intif.h"
#include "pc.h"
#include "guild.h"
#include "battle.h"
#include "atcommand.h"


static struct dbt *storage_db=NULL;
static struct dbt *guild_storage_db=NULL;

/*==========================================
 * �q�ɓ��A�C�e���\�[�g
 *------------------------------------------
 */
int storage_comp_item(const void *_i1, const void *_i2)
{
	const struct item *i1=(const struct item *)_i1;
	const struct item *i2=(const struct item *)_i2;
	return i1->nameid - i2->nameid;
}

 
void sortage_sortitem(struct pc_storage& stor)
{
	qsort(stor.storage, MAX_STORAGE, sizeof(struct item), storage_comp_item);
}

void sortage_gsortitem(struct guild_storage& gstor)
{
	qsort(gstor.storage, MAX_GUILD_STORAGE, sizeof(struct item), storage_comp_item);
}

/*==========================================
 * �������Ƃ�
 *------------------------------------------
 */
void guild_storage_db_final(void *key,void *data)
{
	struct guild_storage *gstor = (struct guild_storage *)data;
	if(gstor) delete gstor;
}
void storage_db_final(void *key,void *data)
{
	struct pc_storage *stor = (struct pc_storage *)data;
	if(stor) delete stor;
}
void do_final_storage(void) // by [MC Cameri]
{
	if (storage_db)
	{
		numdb_final(storage_db,storage_db_final);
		storage_db=NULL;
	}
	if (guild_storage_db)
	{
		numdb_final(guild_storage_db,guild_storage_db_final);
		guild_storage_db=NULL;
	}
}
int do_init_storage(void) // map.c::do_init()����Ă΂��
{
	if (storage_db)
		numdb_final(storage_db,storage_db_final);
	storage_db=numdb_init();
	if (guild_storage_db)
		numdb_final(guild_storage_db,guild_storage_db_final);
	guild_storage_db=numdb_init();
	return 1;
}

struct pc_storage *account2storage(uint32 account_id)
{
	struct pc_storage *stor = (struct pc_storage *)numdb_search(storage_db,account_id);
	if(stor == NULL)
	{
		stor = new struct pc_storage(account_id);
		numdb_insert(storage_db,stor->account_id,stor);
	}
	return stor;
}

// Just to ask storage, without creation
struct pc_storage *account2storage2(uint32 account_id)
{
	return (struct pc_storage *)numdb_search(storage_db, account_id);
}

int storage_delete(uint32 account_id)
{
	struct pc_storage *stor = (struct pc_storage *)numdb_search(storage_db,account_id);
	if(stor)
	{
		numdb_erase(storage_db,account_id);
		delete stor;
	}
	return 0;
}

/*==========================================
 * �J�v���q�ɂ��J��
 *------------------------------------------
 */
int storage_storageopen(map_session_data &sd)
{
	struct pc_storage *stor;

	if(sd.isGM() && sd.isGM() < config.gm_can_drop_lv)
	{
		clif_displaymessage(sd.fd, msg_txt(MSG_GM_LV_TOO_LOW));
		return 1;
	}
	stor = (pc_storage *)numdb_search(storage_db,sd.status.account_id);
	if( stor!= NULL )
	{
		if (stor->storage_status == 0)
		{
			stor->storage_status = 1;
			sd.state.storage_flag = 0;
			clif_storageitemlist(sd,*stor);
			clif_storageequiplist(sd,*stor);
			clif_updatestorageamount(sd,*stor);
			return 0;
		}	
	}
	else
	{
		intif_request_storage(sd.status.account_id);
	}
	return 1;
}

/*==========================================
 * �J�v���q�ɂփA�C�e���ǉ�
 *------------------------------------------
 */
int storage_additem(map_session_data &sd, struct pc_storage &stor, struct item &item_data, size_t amount)
{
	struct item_data *data;
	size_t i;

	stor.dirty = 1;

	if(item_data.nameid <= 0 || amount <= 0)
		return 1;
	nullpo_retr(1, data = itemdb_exists(item_data.nameid));

	if( !itemdb_canstore(item_data.nameid, sd.isGM()) )
	{	//Check if item is storable. [Skotlex]
		clif_displaymessage (sd.fd, msg_txt(MSG_ITEM_CANNOT_BE_STORED));
		return 1;
	}
	
	i=MAX_STORAGE;
	if( !itemdb_isSingleStorage(*data) )
	{	// �����i�ł͂Ȃ��̂ŁA�����L�i�Ȃ���̂ݕω�������
		for(i=0;i<MAX_STORAGE;++i)
		{
			if( item_data == stor.storage[i] )
			{
				if(stor.storage[i].amount+amount > MAX_AMOUNT)
					return 1;
				stor.storage[i].amount+=amount;
				clif_storageitemadded(sd,stor,i,amount);
				break;
			}
		}
	}
	if(i>=MAX_STORAGE)
	{	// �����i�������L�i�������̂ŋ󂫗��֒ǉ�
		for(i=0;i<MAX_STORAGE;++i)
		{
			if(stor.storage[i].nameid==0)
			{
				memcpy(&stor.storage[i],&item_data,sizeof(stor.storage[0]));
				stor.storage[i].amount=amount;
				stor.storage_amount++;
				clif_storageitemadded(sd,stor,i,amount);
				clif_updatestorageamount(sd,stor);
				break;
			}
		}
		if(i>=MAX_STORAGE)
			return 1;
	}

	return 0;
}
/*==========================================
 * �J�v���q�ɃA�C�e�������炷
 *------------------------------------------
 */
int storage_delitem(map_session_data &sd,struct pc_storage &stor,size_t n,size_t amount)
{

	if(n>=MAX_STORAGE || stor.storage[n].nameid==0 || stor.storage[n].amount<amount)
		return 1;

	stor.storage[n].amount-=amount;
	if(stor.storage[n].amount==0)
	{
		stor.storage[n] = item();
		stor.storage_amount--;
		clif_updatestorageamount(sd,stor);
	}
	clif_storageitemremoved(sd,n,amount);
	stor.dirty = 1;

	return 0;
}
/*==========================================
 * �J�v���q�ɂ֓����
 *------------------------------------------
 */
int storage_storageadd(map_session_data &sd, size_t index, size_t amount)
{
	struct pc_storage *stor;

	nullpo_retr(0, stor=account2storage2(sd.status.account_id));

	if( (stor->storage_amount <= MAX_STORAGE) && (stor->storage_status == 1) )
	{	// storage not full & storage open
		if(index<MAX_INVENTORY)
		{	// valid index
			if( (amount <= sd.status.inventory[index].amount) && (amount > 0) )
			{	//valid amount
				//log_tostorage(sd, index, 0);
				if( storage_additem(sd,*stor,sd.status.inventory[index],amount)==0 )
					// remove item from inventory
					pc_delitem(sd,index,amount,0);
			}
		}
	}
	return 0;
}

/*==========================================
 * �J�v���q�ɂ���o��
 *------------------------------------------
 */
int storage_storageget(map_session_data &sd, size_t index, size_t amount)
{
	struct pc_storage *stor;
	int flag;

	nullpo_retr(0, stor=account2storage2(sd.status.account_id));

	if( stor->storage_status == 1 )
	{	//  storage open
		if(index<MAX_STORAGE)
		{	// valid index
			if( (amount <= stor->storage[index].amount) && (amount >0) )
			{	//valid amount
				if( (flag = pc_additem(sd,stor->storage[index],amount)) == 0 )
					storage_delitem(sd,*stor,index,amount);
				else
					clif_additem(sd,0,0,flag);
			}
		}
	}
	return 0;
}
/*==========================================
 * �J�v���q�ɂփJ�[�g��������
 *------------------------------------------
 */
int storage_storageaddfromcart(map_session_data &sd,size_t index, size_t amount)
{
	struct pc_storage *stor;

	nullpo_retr(0, stor=account2storage2(sd.status.account_id));
	
	if( (stor->storage_amount <= MAX_STORAGE) && (stor->storage_status == 1) )
	{	// storage not full & storage open
		if( index<MAX_INVENTORY )
		{	// valid index
			if( (amount <= sd.status.cart[index].amount) && (amount > 0) )
			{	//valid amount
				if( storage_additem(sd,*stor,sd.status.cart[index],amount)==0 )
					pc_cart_delitem(sd,index,amount,0);
			}
		}
	}
	return 0;
}

/*==========================================
 * �J�v���q�ɂ���J�[�g�֏o��
 *------------------------------------------
 */
int storage_storagegettocart(map_session_data &sd,size_t index,size_t amount)
{
	struct pc_storage *stor;

	nullpo_retr(0, stor=account2storage2(sd.status.account_id));

	if( stor->storage_status == 1 )
	{	//  storage open
		if( index<MAX_STORAGE )
		{	// valid index
			if( (amount <= stor->storage[index].amount) && (amount > 0) )
			{	//valid amount
				if(pc_cart_additem(sd,stor->storage[index],amount)==0)
				{
					storage_delitem(sd,*stor,index,amount);
				}
			}
		}
	}
	return 0;
}


/*==========================================
 * Modified By Valaris to save upon closing [massdriller]
 *------------------------------------------
 */
int storage_storageclose(map_session_data &sd)
{
	struct pc_storage *stor;

	nullpo_retr(0, stor=account2storage2(sd.status.account_id));

	stor->storage_status=0;
	sd.state.storage_flag = 0;
	clif_storageclose(sd);
	chrif_save(sd);
	chrif_save(sd);
	storage_storage_save(sd);	//items lost on crash/shutdown, by valaris

	sortage_sortitem(*stor);
	return 0;
}

/*==========================================
 * ���O�A�E�g���J���Ă���J�v���q�ɂ̕ۑ�
 *------------------------------------------
 */
int storage_storage_quit(map_session_data &sd)
{
	struct pc_storage *stor;

	stor = (struct pc_storage *)numdb_search(storage_db,sd.status.account_id);
	if(stor)
	{
		stor->storage_status = 0;
		storage_storage_save(sd);
	}
	return 0;
}

void storage_storage_dirty(map_session_data &sd)
{
	struct pc_storage *stor;

	stor = (struct pc_storage *)numdb_search(storage_db,sd.status.account_id);
	if(stor)
		stor->dirty = 1;
}

int storage_storage_save(map_session_data &sd)
{
	struct pc_storage *stor;

	stor = (struct pc_storage *)numdb_search(storage_db,sd.status.account_id);
	if(stor && stor->dirty)  {
		intif_send_storage(*stor);
		stor->dirty = 0;
	}
	return 0;
}

struct guild_storage *guild2storage(uint32 guild_id)
{
	struct guild_storage *gs = NULL;
	if(guild_search(guild_id) != NULL)
	{
		gs = (struct guild_storage *)numdb_search(guild_storage_db,guild_id);
		if(gs == NULL)
		{
			gs = new struct guild_storage(guild_id);
			numdb_insert(guild_storage_db,gs->guild_id,gs);
		}
	}
	return gs;
}

int guild_storage_delete(uint32 guild_id)
{
	struct guild_storage *gstor = (struct guild_storage *)numdb_search(guild_storage_db,guild_id);
	if(gstor)
	{
		numdb_erase(guild_storage_db,guild_id);
		delete gstor;
	}
	return 0;
}

int storage_guild_storageopen(map_session_data &sd)
{
	struct guild_storage *gstor;

	if(sd.status.guild_id <= 0)
		return 2;
	gstor = (struct guild_storage *)numdb_search(guild_storage_db,sd.status.guild_id);
	if( gstor != NULL )
	{
		if(gstor->storage_status)
			return 1;
		gstor->storage_status = 1;
		sd.state.storage_flag = 1;
		clif_guildstorageitemlist(sd,*gstor);
		clif_guildstorageequiplist(sd,*gstor);
		clif_updateguildstorageamount(sd,*gstor);
		return 0;
	}
	else
	{
		gstor = guild2storage(sd.status.guild_id);
		gstor->storage_status = 1;
		intif_request_guild_storage(sd.status.account_id,sd.status.guild_id);
	}
	return 0;
}

int guild_storage_additem(map_session_data &sd,struct guild_storage &stor,struct item &item_data,size_t amount)
{
	struct item_data *data;
	size_t i;

	nullpo_retr(1, data = itemdb_exists(item_data.nameid));

	if(item_data.nameid <= 0 || amount <= 0)
		return 1;

	if( !itemdb_canguildstore(item_data.nameid, sd.isGM()) )
	{	//Check if item is storable. [Skotlex]
		clif_displaymessage (sd.fd, msg_txt(MSG_ITEM_CANNOT_BE_STORED));
		return 1;
	}

	i=MAX_GUILD_STORAGE;
	if( !itemdb_isSingleStorage(*data) )
	{	// �����i�ł͂Ȃ��̂ŁA�����L�i�Ȃ���̂ݕω�������
		for(i=0;i<MAX_GUILD_STORAGE;++i)
		{
			if( item_data == stor.storage[i] )
			{
				if(stor.storage[i].amount+amount > MAX_AMOUNT)
					return 1;
				stor.storage[i].amount+=amount;
				clif_guildstorageitemadded(sd,stor,i,amount);
				break;
			}
		}
	}
	if(i>=MAX_GUILD_STORAGE)
	{	// �����i�������L�i�������̂ŋ󂫗��֒ǉ�
		for(i=0;i<MAX_GUILD_STORAGE;++i)
		{
			if(stor.storage[i].nameid==0)
			{
				memcpy(&stor.storage[i],&item_data,sizeof(stor.storage[0]));
				stor.storage[i].amount=amount;
				stor.storage_amount++;
				clif_guildstorageitemadded(sd,stor,i,amount);
				clif_updateguildstorageamount(sd,stor);
				break;
			}
		}
		if(i>=MAX_GUILD_STORAGE)
			return 1;
	}
	return 0;
}

int guild_storage_delitem(map_session_data &sd,struct guild_storage &stor,size_t n,size_t amount)
{
	if(n>=MAX_GUILD_STORAGE || stor.storage[n].nameid==0 || stor.storage[n].amount<amount)
		return 1;

	stor.storage[n].amount-=amount;
	if(stor.storage[n].amount==0)
	{
		stor.storage[n] = item();
		stor.storage_amount--;
		clif_updateguildstorageamount(sd,stor);
	}
	clif_storageitemremoved(sd,n,amount);

	return 0;
}

int storage_guild_storageadd(map_session_data &sd,size_t index, size_t amount)
{
	struct guild_storage *stor;

	stor=guild2storage(sd.status.guild_id);
	if( stor != NULL )
	{
		if( (stor->storage_amount <= MAX_GUILD_STORAGE) && (stor->storage_status == 1) )
		{	// storage not full & storage open
			if( index<MAX_INVENTORY )
			{	// valid index
				if( (amount <= sd.status.inventory[index].amount) && (amount > 0) )
				{	//valid amount
					//log_tostorage(sd, index, 1);
					if(guild_storage_additem(sd,*stor,sd.status.inventory[index],amount)==0)
						// remove item from inventory
						pc_delitem(sd,index,amount,0);
				}
			}
		}
	}
	return 0;
}

int storage_guild_storageget(map_session_data &sd,size_t index, size_t amount)
{
	struct guild_storage *stor;
	int flag;

	stor=guild2storage(sd.status.guild_id);
	if( stor && stor->storage_status == 1)
	{	//  storage open
		if( index<MAX_GUILD_STORAGE )
		{	// valid index
			if( (amount <= stor->storage[index].amount) && (amount > 0) )
			{	//valid amount
				if((flag = pc_additem(sd,stor->storage[index],amount)) == 0)
					guild_storage_delitem(sd,*stor,index,amount);
				else
					clif_additem(sd,0,0,flag);
				//log_fromstorage(sd, index, 1);
			}
		}
	}
	return 0;
}

int storage_guild_storageaddfromcart(map_session_data &sd,size_t index, size_t amount)
{
	struct guild_storage *stor;

	stor=guild2storage(sd.status.guild_id);
	if( stor != NULL )
	{
		if( (stor->storage_amount <= MAX_GUILD_STORAGE) && (stor->storage_status == 1) )
		{	// storage not full & storage open
			if( index<MAX_INVENTORY )
			{	// valid index
				if( (amount <= sd.status.cart[index].amount) && (amount > 0) )
				{	//valid amount
					if(guild_storage_additem(sd,*stor,sd.status.cart[index],amount)==0)
						pc_cart_delitem(sd,index,amount,0);
				}
			}
		}
	}
	return 0;
}

int storage_guild_storagegettocart(map_session_data &sd,size_t index,size_t amount)
{
	struct guild_storage *stor;

	stor=guild2storage(sd.status.guild_id);
	if( stor != NULL &&  stor->storage_status == 1)
	{	//  storage open
		if( index<MAX_GUILD_STORAGE )
		{	// valid index
			if( (amount <= stor->storage[index].amount) && (amount > 0) )
			{	//valid amount
				if(pc_cart_additem(sd,stor->storage[index],amount)==0)
				{
					guild_storage_delitem(sd,*stor,index,amount);
				}
			}
		}
	}
	return 0;
}

int storage_guild_storagesave(map_session_data &sd)
{
	struct guild_storage *stor;

	if((stor=guild2storage(sd.status.guild_id)) != NULL)
	{
		intif_send_guild_storage(sd.status.account_id,*stor);
	}
	return 0;
}

int storage_guild_storageclose(map_session_data &sd)
{
	struct guild_storage *stor;

	if((stor=guild2storage(sd.status.guild_id)) != NULL)
	{
		intif_send_guild_storage(sd.status.account_id,*stor);
		stor->storage_status = 0;
		sd.state.storage_flag = 0;
		sortage_gsortitem(*stor);
	}
	clif_storageclose(sd);

	return 0;
}

int storage_guild_storage_quit(map_session_data &sd,int flag)
{
	struct guild_storage *stor;

	stor = (struct guild_storage *)numdb_search(guild_storage_db,sd.status.guild_id);
	if(stor)
	{
		if(!flag)
			intif_send_guild_storage(sd.status.account_id,*stor);
		stor->storage_status = 0;
		sd.state.storage_flag = 0;
	}
	return 0;
}









struct storitem
{
	item_data*	data;
	uint32		id;
	unsigned short	amount;
	unsigned char	identified;
	unsigned char	refine;
	unsigned char	attribute;
	unsigned short	card[4];
	uint32	producer_id;

	
	storitem(item_data&	d)
		: data(&d)
		, id(0)
		, amount(1)
		, identified(0)
		, refine(0)
		, attribute(0)
		, producer_id(0)
	{
		card[0]=card[1]=card[2]=card[3]=0;
	}
	storitem()
		: data(NULL)
		, id(0)
		, amount(1)
		, identified(0)
		, refine(0)
		, attribute(0)
		, producer_id(0)
	{
		card[0]=card[1]=card[2]=card[3]=0;
	}
	friend int compare(const storitem& a, const storitem& b);
};

bool operator==(const storitem& a, const storitem& b)
{
	return compare(a,b)==0;
}
bool operator< (const storitem& a, const storitem& b)
{
	return compare(a,b)< 0;
}

int compare(const storitem& a, const storitem& b)
{
	if(a.data!=b.data)
	{
		return (a.data->name && b.data->name)?strcasecmp(a.data->name, b.data->name):a.data->name-b.data->name;
	}
	else
	{
		return	a.identified!=b.identified?a.identified-b.identified:
				a.refine!=b.refine?a.refine-b.refine:
				a.attribute!=b.attribute?a.attribute-b.attribute:
				a.card[0]!=b.card[0]?a.card[0]-b.card[0]:
				a.card[1]!=b.card[1]?a.card[1]-b.card[1]:
				a.card[2]!=b.card[2]?a.card[2]-b.card[2]:
				a.card[3]!=b.card[3]?a.card[3]-b.card[3]:
				a.producer_id!=b.producer_id?a.producer_id-b.producer_id:
				0;
	}
}

template<typename T, size_t Limit>
struct itemlist : public T
{
	basics::slist<storitem> items;
public:
	itemlist()
	{
		this->items.config.duplicates = true;
	}
	~itemlist()
	{}

	typedef storitem*			iterator;
	typedef const storitem*		const_iterator;

	iterator begin()				{ return items.begin(); }
	iterator end()					{ return items.begin(); }
	const_iterator begin() const	{ return items.end(); }
	const_iterator end() const		{ return items.end(); }



	bool additem(const storitem& new_item)
	{
		iterator obj;
		if( new_item.data && new_item.amount > 0 &&	// test for invalid item
			this->items.size()<Limit )				// storage not full
		{
			if( !itemdb_isSingleStorage(*new_item.data) && 
				(obj=this->items.search(new_item)) )
			{	// stackabe and exists, test for amount limit
				if( MAX_AMOUNT - obj->amount >= new_item.amount )
				{
					obj->amount += new_item.amount;
					this->change_entry(*obj);
					return true;
				}
			}
			else
			{	// create new entry
				this->items.append(new_item);
				this->add_entry(new_item);
				return true;
			}
		}
		return false;
	}
	bool delitem(const storitem &old_item)
	{
		iterator obj;
		if( old_item.data && old_item.amount>0 &&
			(obj=this->items.search(old_item)) &&
			obj->amount>=old_item.amount )
		{
			if( obj->amount==old_item.amount )
			{
				this->del_entry(*obj);
				this->items.erase(obj);
			}
			else
			{
				obj->amount -= old_item.amount;
				this->change_entry(*obj);
			}
			return true;
		}
		return false;
	}
};

struct _dummy
{
	void change_entry(const storitem &obj)
	{}
	void add_entry(const storitem &obj)
	{}
	void del_entry(const storitem &obj)
	{}
};












