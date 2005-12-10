// $Id: vending.c,v 1.2 2004/09/25 05:32:19 MouseJstr Exp $
#include "base.h"
#include "socket.h"
#include "showmsg.h"
#include "utils.h"

#include "map.h"
#include "atcommand.h"
#include "clif.h"
#include "itemdb.h"
#include "vending.h"
#include "pc.h"
#include "skill.h"
#include "battle.h"
#include "nullpo.h"
#include "log.h"

/*==========================================
 * �I�X��
 *------------------------------------------
*/
void vending_closevending(struct map_session_data &sd)
{
	sd.vender_id=0;
	clif_closevendingboard(sd.bl,0);
}

/*==========================================
 * �I�X�A�C�e�����X�g�v��
 *------------------------------------------
 */
void vending_vendinglistreq(struct map_session_data &sd, uint32 id)
{
	struct map_session_data *vsd;

	if( (vsd=map_id2sd(id)) == NULL )
		return;
	if(vsd->vender_id==0)
		return;
	clif_vendinglist(sd,id,vsd->vending);
}

/*==========================================
 * �I�X�A�C�e���w��
 *------------------------------------------
 */
void vending_purchasereq(struct map_session_data &sd,unsigned short len,uint32 id,unsigned char *buffer)
{
	size_t i, j, w;
	uint32 z;
	unsigned short blank, vend_list[MAX_VENDING];
	unsigned short amount, index, new_ = 0;
	struct vending vending[MAX_VENDING]; // against duplicate packets
	struct map_session_data *vsd = map_id2sd(id);

	if(vsd == NULL)
		return;
	if(vsd->vender_id == 0)
		return;
	if(vsd->vender_id == sd.bl.id)
		return;
	if( vsd->vend_num>MAX_VENDING )
		vsd->vend_num=MAX_VENDING;

	// duplicate item in vending to check hacker with multiple packets
	memcpy(&vending, &vsd->vending, sizeof(struct vending) * MAX_VENDING); // copy vending list

	// number of blank entries in inventory
	blank = pc_inventoryblank(sd);

	for(i = 0, w = z = 0; 8 + 4 * i < len; i++) {
		amount = RBUFW(buffer, 4 * i);
		index =  RBUFW(buffer, 2 + 4 * i) - 2;

		if(amount > MAX_AMOUNT) return; // exploit
			
		for(j = 0; j < vsd->vend_num; j++) {
			if( vsd->vending[j].amount>0 && vsd->vending[j].index == index )
			{
				if (amount > vsd->vending[j].amount)
				{	
					clif_buyvending(sd,index,vsd->vending[j].amount, 4);
					return;
				}
				else 
					break;
			}
		}
		if (j == vsd->vend_num)
			return; // ����؂�

		vend_list[i] = j;
		z += vsd->vending[j].value * amount;
		if (z > sd.status.zeny){
			clif_buyvending(sd, index, amount, 1);
			return; // zeny�s��
		}
		w += itemdb_weight(vsd->status.cart[index].nameid) * amount;
		if (w + sd.weight > sd.max_weight) {
			clif_buyvending(sd, index, amount, 2);
			return; // �d�ʒ���
		}
		
		if (vending[j].amount > vsd->status.cart[index].amount) //Check to see if cart/vend info is in sync.
			vending[j].amount = vsd->status.cart[index].amount;

		// if they try to add packets (example: get twice or more 2 apples if marchand has only 3 apples).
		// here, we check cumulativ amounts
		if (vending[j].amount < amount) { // send more quantity is not a hack (an other player can have buy items just before)
			clif_buyvending(sd, index, vsd->vending[j].amount, 4); // not enough quantity
			return;
		} else
			vending[j].amount -= amount;

		switch( pc_checkadditem(sd, vsd->status.cart[index].nameid, amount) ) {
		case ADDITEM_EXIST:
			break;
		case ADDITEM_NEW:
			new_++;
			if (new_ > blank)
				return;	// ��ސ�����
			break;
		case ADDITEM_OVERAMOUNT:
			return; // �A�C�e��������
		}
	}
	if (z < 0 || z > MAX_ZENY) { // Zeny Bug Fixed by Darkchild
		clif_tradecancelled(sd);
		clif_tradecancelled(*vsd);
		return;
	}

	pc_payzeny(sd, z);
	pc_getzeny(*vsd, z);
	for(i = 0; 8 + 4 * i < len; i++) {
		amount = RBUFW(buffer, 4 *i);
		index =  RBUFW(buffer, 2 + 4 * i) - 2;
		//if (amount < 0) break; // tested at start of the function
		pc_additem(sd,vsd->status.cart[index],amount);
		vsd->vending[vend_list[i]].amount -= amount;
		pc_cart_delitem(*vsd, index, amount, 0);
		clif_vendingreport(*vsd, index, amount);
		if(battle_config.buyer_name)
		{
			char temp[256];
			sprintf(temp, msg_txt(265), sd.status.name);
			clif_disp_onlyself(*vsd,temp);
		}
		//log added by Lupus
		if(log_config.vend > 0) {
			log_vend(sd, *vsd, index, amount, z); // for Item + Zeny. log.
			//we log ZENY only with the 1st item. Then zero it for the rest items 8).
			z = 0;
		}
	}
}

/*==========================================
 * �I�X�J��
 *------------------------------------------
 */
void vending_openvending(struct map_session_data &sd,unsigned short len,const char *message,int flag, unsigned char *buffer)
{
	size_t i;

	if(!pc_checkskill(sd,MC_VENDING) || !pc_iscarton(sd)) {	// cart skill and cart check [Valaris]
		clif_skill_fail(sd,MC_VENDING,0,0);
		return;
	}

	if (flag) {
		for(i = 0; (85 + 8 * i < len) && (i < MAX_VENDING); i++) {
			sd.vending[i].index = RBUFW(buffer,8*i) - 2;
			sd.vending[i].amount= RBUFW(buffer,2+8*i);
			sd.vending[i].value = RBUFL(buffer,4+8*i);
			if(sd.vending[i].value>battle_config.vending_max_value)
				sd.vending[i].value=battle_config.vending_max_value;
			else if(sd.vending[i].value == 0)
				sd.vending[i].value = 1000000;	// auto set to 1 million [celest]
			// �J�[�g���̃A�C�e�����Ɣ̔�����A�C�e�����ɑ��Ⴊ�������璆�~
			if(pc_cartitem_amount(sd, sd.vending[i].index, sd.vending[i].amount) < 0 || sd.vending[i].value < 0) { // fixes by Valaris and fritz
				clif_skill_fail(sd, MC_VENDING, 0, 0);
				return;
			}
		}
		sd.vender_id = sd.bl.id;
		sd.vend_num = i;
		strcpy(sd.message,message);
		if (clif_openvending(sd,sd.vender_id,sd.vending) > 0)
			clif_showvendingboard(sd.bl,message,0);
		else
			sd.vender_id = 0;
	}
}

