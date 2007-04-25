// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder
#include "nullpo.h"
#include "showmsg.h"
#include "utils.h"


#include "battle.h"
#include "map.h"
#include "pc.h"
#include "status.h"
#include "skill.h"
#include "mob.h"
#include "itemdb.h"
#include "clif.h"
#include "pet.h"
#include "homun.h"
#include "guild.h"



int attr_fix_table[4][10][10];





/*==========================================
 * 自分をロックしているMOBの?を?える(foreachclient)
 *------------------------------------------
 */
class CBattleCountTargeted : public CMapProcessor
{
	uint32 id;
	block_list *src;
	unsigned short target_lv;
public:
	mutable int c;

	CBattleCountTargeted(uint32 i, block_list *s, unsigned short lv)
		: id(i), src(s), target_lv(lv), c(0)
	{}
	~CBattleCountTargeted()	{}
	virtual int process(block_list& bl) const
	{
		if( id == bl.id || (src && id == src->id))
			return 0;
		fightable *fi = bl.get_fightable();
		if(fi && fi->target_id == id && fi->is_attacking() && fi->target_lv >= target_lv)
			c++;
		return 0;
	}
};
/*==========================================
 * 自分をロックしている対象の数を返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
unsigned int battle_counttargeted(block_list &bl,block_list *src,  unsigned short target_lv)
{
	CBattleCountTargeted bct(bl.id, src, target_lv);
	block_list::foreachinarea( bct,
		bl.m, ((int)bl.x)-AREA_SIZE, ((int)bl.y)-AREA_SIZE, ((int)bl.x)+AREA_SIZE, ((int)bl.y)+AREA_SIZE, BL_ALL);
	return bct.c;
}

/*==========================================
 * Get random targetting enemy
 *------------------------------------------
 */

class CBattleGetTargeted : public CMapProcessor
{
	ICL_EMPTY_COPYCONSTRUCTOR(CBattleGetTargeted)
	block_list &target;
public:
	mutable block_list *bl_list[32];
	mutable size_t c;

	CBattleGetTargeted(block_list &t) : target(t), c(0)	{}
	~CBattleGetTargeted()	{}

	virtual int process(block_list& bl) const
	{
		fightable *fi;;
		if( bl.id != target.id && 
			c < sizeof(bl_list)/sizeof(bl_list[0]) &&
			(fi = bl.get_fightable()) && 
			fi->target_id == target.id && 
			fi->is_attacking() )
		{
			bl_list[c++] = &bl;
		}
		return 0;
	}
};
block_list* battle_gettargeted(block_list &target)
{
	CBattleGetTargeted bgt(target);
	block_list::foreachinarea( bgt,
		target.m, ((int)target.x)-AREA_SIZE, ((int)target.y)-AREA_SIZE, ((int)target.x)+AREA_SIZE, ((int)target.y)+AREA_SIZE, BL_ALL);
	if(bgt.c<1)
		return NULL;
	else
		return bgt.bl_list[rand()%bgt.c];
}

// ダメージの遅延
struct delay_damage {
	block_list *src;
	int target;
	int damage;
	int flag;
};

int battle_delay_damage_sub(int tid, unsigned long tick, int id, basics::numptr data)
{
	struct delay_damage *dat = (struct delay_damage *)data.ptr;
	if(dat)
	{
		block_list *target = block_list::from_blid(dat->target);
		if( target && block_list::from_blid(id) == dat->src && target->is_on_map() )
		battle_damage(dat->src, target, dat->damage, dat->flag);
		delete dat;
		get_timer(tid)->data=0;
	}
	return 0;
}
int battle_delay_damage(unsigned long tick, block_list &src, block_list &target, int damage, int flag)
{
	struct delay_damage *dat;
	if(!config.delay_battle_damage)
	{
		battle_damage(&src, &target, damage, flag);
		return 0;
	}
	dat = new struct delay_damage;
	dat->src = &src;
	dat->target = target.id;
	dat->damage = damage;
	dat->flag = flag;
	add_timer(tick, battle_delay_damage_sub, src.id, basics::numptr(dat), false);
	return 0;
}

// 実際にHPを操作
int battle_damage(block_list *bl, block_list *target, int damage, int flag)
{
	int i;

	nullpo_retr(0, target); //blはNULLで呼ばれることがあるので他でチェック
	
	if( damage == 0 ||
		!target->is_on_map() ||
		target->get_pd() )
		return 0;

	if (bl)
	{
		if( !bl->is_on_map() )
			return 0;
	}

	if (damage < 0)
		return battle_heal(bl,target,-damage,0,flag);

	if( !flag )
	{
		// 凍結、石化、睡眠を消去
		if( target->has_status(SC_FREEZE) )
			status_change_end(target,SC_FREEZE,-1);
		if( target->has_status(SC_STONE) && target->get_statusvalue2(SC_STONE).integer()==0 )
			status_change_end(target,SC_STONE,-1);
		if( target->has_status(SC_SLEEP) )
			status_change_end(target,SC_SLEEP,-1);
	}

	if( target->get_md() )
	{	// MOB
		mob_data *md = target->get_md();
		if (md->skilltimer != -1 && md->state.skillcastcancel)	// 詠唱妨害
			skill_castcancel(target,0);
		return mob_damage(*md,damage,0,bl);
	}
	else if( target->get_sd() )
	{	// PC
		map_session_data *tsd = target->get_sd();
		if( target->has_status(SC_DEVOTION) )
		{	// ディボーションをかけられている
			map_session_data *sd2 = map_session_data::from_blid(target->get_statusvalue1(SC_DEVOTION).integer());
			if (sd2 && skill_devotion3(sd2, target->id))
			{
				skill_devotion(sd2, target->id);
			}
			else if (sd2 && bl)
			{
				for (i = 0; i < 5; ++i)
					if (sd2->dev.val1[i] == target->id) {
						clif_damage(*bl, *sd2, gettick(), 0, 0, damage, 0 , 0, 0);
						pc_damage(*sd2, damage, sd2);
						return 0;
					}
			}
		}

		if(tsd && tsd->skilltimer!=-1){	// 詠唱妨害
			// フェンカードや妨害されないスキルかの検査
			if( (!tsd->state.no_castcancel || maps[bl->m].flag.gvg) && tsd->state.skillcastcancel &&
				!tsd->state.no_castcancel2)
				skill_castcancel(target,0);
		}
		return pc_damage(*tsd,damage,bl);
	}
	else if( target->get_sk() )
		return skill_unit_ondamaged( target->get_sk(), bl, damage, gettick() );
	return 0;
}

int battle_heal(block_list *bl,block_list *target,int hp,int sp,int flag)
{
	nullpo_retr(0, target); //blはNULLで呼ばれることがあるので他でチェック

	if( target->is_dead() || target->get_pd() )
		return 0;

	if(hp == 0 && sp == 0)
		return 0;

	if(hp < 0)
		return battle_damage(bl,target,-hp,flag);

	target->heal(hp,sp);
	return 0;
}


/*==========================================
 * ダメージの属性修正
 *------------------------------------------
 */
int battle_attr_fix(int damage,int atk_elem,int def_elem)
{
	int def_type = def_elem % 10, def_lv = def_elem / 10 / 2;

	if (atk_elem < 0 || atk_elem > 9)
		atk_elem = rand()%9;	//武器属性ランダムで付加

	//if (def_type < 0 || def_type > 9)
		//def_type = rand()%9;	// change 装備属性? // celest

	if (def_type < 0 || def_type > 9 ||
		def_lv < 1 || def_lv > 4) {	// 属 性値がおかしいのでとりあえずそのまま返す
		if (config.error_log)
			ShowMessage("battle_attr_fix: unknown attr type: atk=%d def_type=%d def_lv=%d\n",atk_elem,def_type,def_lv);
		return damage;
	}

	return damage*attr_fix_table[def_lv-1][atk_elem][def_type]/100;
}


/*==========================================
 * ダメージ最終計算
 *------------------------------------------
 */
int battle_calc_damage(block_list *src,block_list *bl,int damage,int div_,int skill_num,short skill_lv,int flag)
{
	nullpo_retr(0, bl);
	nullpo_retr(0, src);

	if(src->m != bl->m) // [ShAPoNe] Src and target same map check.
		return 0;

	map_session_data *sd = bl->get_sd();
	mob_data *md = bl->get_md();
	int class_ = bl->get_class();

	if( bl->has_status(SC_SAFETYWALL) && damage>0 && flag&BF_WEAPON &&
		flag&BF_SHORT && skill_num != NPC_GUIDEDATTACK && skill_num != AM_DEMONSTRATION)
	{	// セーフティウォール
		struct skill_unit *unit = (struct skill_unit *)bl->get_statusvalue2(SC_SAFETYWALL).pointer();
		// temporary check to prevent access on wrong val2
		if (unit && unit->block_list::m == bl->m)
		{
			if (unit->group && (--unit->group->val2)<=0)
				skill_delunit(unit);
			damage=0;
		}
		else
		{
			status_change_end(bl,SC_SAFETYWALL,-1);
		}
	}
	if( bl->has_status(SC_PNEUMA) && damage>0 &&
		((flag&BF_WEAPON && flag&BF_LONG && skill_num != NPC_GUIDEDATTACK) ||
		(flag&BF_MISC && (skill_num ==  HT_BLITZBEAT || skill_num == SN_FALCONASSAULT || skill_num == TF_THROWSTONE)) ||
		(flag&BF_WEAPON && skill_num == ASC_BREAKER))){ // It should block only physical part of Breaker! [Lupus]
		// ニューマ
		damage=0;
	}

	/*
	if( bl->has_status(SC_ROKISWEIL) && damage>0 && flag&BF_MAGIC ){
		// ニューマ
		damage=0;
	}
	*/

	if( bl->has_status(SC_AETERNA) && damage>0 && skill_num != PA_PRESSURE)
	{
		damage<<=1;
		if (skill_num != ASC_BREAKER || flag & BF_MAGIC) //Only end it on the second attack of breaker. [Skotlex]
			status_change_end( bl,SC_AETERNA,-1 );
	}

	//属性場のダメージ増加
	if(bl->has_status(SC_VOLCANO))
	{	// ボルケーノ
		if(flag&BF_SKILL && skill_get_pl(skill_num)==3)
			damage += damage * enchant_eff[bl->get_statusvalue1(SC_VOLCANO).integer()-1] /100;
		else if(!(flag&BF_SKILL) && status_get_attack_element(bl)==3)
			damage += damage * enchant_eff[bl->get_statusvalue1(SC_VOLCANO).integer()-1] /100;
	}

	if( bl->has_status(SC_VIOLENTGALE))
	{	// バイオレントゲイル
		if(flag&BF_SKILL && skill_get_pl(skill_num)==4)
			damage += damage * enchant_eff[bl->get_statusvalue1(SC_VIOLENTGALE).integer()-1] /100;
		else if(!(flag&BF_SKILL) && status_get_attack_element(bl)==4)
			damage += damage * enchant_eff[bl->get_statusvalue1(SC_VIOLENTGALE).integer()-1] /100;
	}

	if(bl->has_status(SC_DELUGE))
	{	// デリュージ
		if(flag&BF_SKILL && skill_get_pl(skill_num)==1)
			damage += damage * enchant_eff[bl->get_statusvalue1(SC_DELUGE).integer()-1] /100;
		else if(!(flag&BF_SKILL) && status_get_attack_element(bl)==1)
			damage += damage * enchant_eff[bl->get_statusvalue1(SC_DELUGE).integer()-1] /100;
	}

	if(bl->has_status(SC_ENERGYCOAT) && damage>0  && flag&BF_WEAPON){	// エナジーコート
		if(sd){
			if(sd->status.sp>0){
				int per = sd->status.sp * 5 / (sd->status.max_sp + 1);
				sd->status.sp -= sd->status.sp * (per * 5 + 10) / 1000;
				if( sd->status.sp < 0 ) sd->status.sp = 0;
				damage -= damage * ((per+1) * 6) / 100;
				clif_updatestatus(*sd,SP_SP);
			}
			if(sd->status.sp<=0)
				status_change_end( bl,SC_ENERGYCOAT,-1 );
		}
		else
			damage -= damage * (bl->get_statusvalue1(SC_ENERGYCOAT).integer() * 6) / 100;
	}

	if(bl->has_status(SC_KYRIE) && damage > 0){	// キリエエレイソン
		bl->get_statusvalue2(SC_KYRIE).integer() -= damage;
		if(flag&BF_WEAPON || (flag&BF_MISC && skill_num == TF_THROWSTONE))
		{
			if(bl->get_statusvalue2(SC_KYRIE).integer()>=0)
				damage=0;
			else
				damage=-bl->get_statusvalue2(SC_KYRIE).integer();
		}
		if((--bl->get_statusvalue3(SC_KYRIE).integer())<=0 || (bl->get_statusvalue2(SC_KYRIE).integer()<=0) || skill_num == AL_HOLYLIGHT)
			status_change_end(bl, SC_KYRIE, -1);
	}

	if(bl->has_status(SC_BASILICA) && damage > 0){
		// ニューマ
		damage=0;
	}
	if(bl->has_status(SC_LANDPROTECTOR) && damage>0 && flag&BF_MAGIC){
		// ニューマ
		damage=0;
	}

	if(bl->has_status(SC_AUTOGUARD) && damage > 0 && flag&BF_WEAPON) {
		if(rand()%100 < bl->get_statusvalue2(SC_AUTOGUARD).integer()) {
			int delay;

			damage = 0;
			clif_skill_nodamage(*bl,*bl,CR_AUTOGUARD,(unsigned short)bl->get_statusvalue1(SC_AUTOGUARD).integer(),1);
			// different delay depending on skill level [celest]
			if (bl->get_statusvalue1(SC_AUTOGUARD).integer() <= 5)
				delay = 300;
			else if (bl->get_statusvalue1(SC_AUTOGUARD).integer() > 5 && bl->get_statusvalue1(SC_AUTOGUARD).integer() <= 9)
				delay = 200;
			else
				delay = 100;
			if(sd)
				sd->canmove_tick = gettick() + delay;
			else if(md)
				md->canmove_tick = gettick() + delay;
		}
	}
// -- moonsoul (chance to block attacks with new Lord Knight skill parrying)
//
	if(bl->has_status(SC_PARRYING) && damage > 0 && flag&BF_WEAPON) {
		if(rand()%100 < bl->get_statusvalue2(SC_PARRYING).integer()) {
			damage = 0;
			clif_skill_nodamage(*bl,*bl,LK_PARRYING,bl->get_statusvalue1(SC_PARRYING).integer(),1);
		}
	}
	// リジェクトソード
	if(bl->has_status(SC_REJECTSWORD) && damage > 0 && flag&BF_WEAPON &&
		// Fixed the condition check [Aalye]
		(*src==BL_MOB || (*src==BL_PC && (((map_session_data *)src)->status.weapon == 1 ||
		((map_session_data *)src)->status.weapon == 2 ||
		((map_session_data *)src)->status.weapon == 3)))){
		if(rand()%100 < (15*bl->get_statusvalue1(SC_REJECTSWORD).integer())){ //反射確率は15*Lv
			damage = damage*50/100;
			clif_damage(*bl,*src,gettick(),0,0,damage,0,0,0);
			battle_damage(bl,src,damage,0);
			//ダメージを与えたのは良いんだが、ここからどうして表示するんだかわかんねぇ
			//エフェクトもこれでいいのかわかんねぇ
			clif_skill_nodamage(*bl,*bl,ST_REJECTSWORD,bl->get_statusvalue1(SC_REJECTSWORD).integer(),1);
			if((--bl->get_statusvalue2(SC_REJECTSWORD).integer())<=0)
				status_change_end(bl, SC_REJECTSWORD, -1);
		}
	}
	if(bl->has_status(SC_SPIDERWEB) && damage > 0)	// [Celest]
		if ((flag&BF_SKILL && skill_get_pl(skill_num)==3) ||
			(!(flag&BF_SKILL) && status_get_attack_element(src)==3)) {
			damage<<=1;
			status_change_end(bl, SC_SPIDERWEB, -1);
		}

	//Only targetted magic skills are nullified.
	if( bl->has_status(SC_FOGWALL) && 
		flag&BF_MAGIC && 
		!(skill_get_inf(skill_num)&INF_GROUND_SKILL) &&
		rand()%100 < 75 )
			damage = 0;


	//SC effects from caster side.
	if( src->has_status(SC_FOGWALL) && 
		flag&BF_MAGIC && !(skill_get_inf(skill_num)&INF_GROUND_SKILL) &&
		rand()%100 < 75 )
		damage = 0;
	
	if(class_ == MOBID_EMPERIUM || class_ == 1287 || class_ == 1286 || class_ == 1285) {
		if(class_ == MOBID_EMPERIUM && (flag&BF_SKILL || skill_num == ASC_BREAKER || skill_num == PA_SACRIFICE))
			damage=0;
		if(*src == BL_PC)
		{
			struct guild *g=guild_search(src->get_guild_id());
			struct guild_castle *gc=guild_mapname2gc(maps[bl->m].mapname);

			if(gc && agit_flag==0 && class_ != MOBID_EMPERIUM)	// guardians cannot be damaged during non-woe [Valaris]
				damage=0;  // end woe check [Valaris]
			else if(g == NULL)
				damage=0;//ギルド未加入ならダメージ無し
			else if(gc && guild_isallied(*g, *gc))
				damage=0;//自占領ギルドのエンペならダメージ無し
			else if(guild_checkskill(*g,GD_APPROVAL) <= 0)
				damage=0;//正規ギルド承認がないとダメージ無し
			else if(config.guild_max_castles != 0 && guild_checkcastles(*g)>=config.guild_max_castles)
				damage = 0; // [MouseJstr]
			else if(gc && guild_check_alliance(gc->guild_id, g->guild_id, 0) == 1)
				return 0;
		}
		else damage = 0;
	}

	if (damage > 0)
	{	// damage reductions
		if (maps[bl->m].flag.gvg) { //GvG
			if (*bl== BL_MOB)
			{	//defenseがあればダメージが減るらしい？
				struct guild_castle *gc=guild_mapname2gc(maps[bl->m].mapname);
				if (gc) damage -= damage * (gc->defense / 100) * (config.castle_defense_rate/100);
			}
			if (flag & BF_WEAPON) {
				if (flag & BF_SHORT)
					damage = damage * config.gvg_short_damage_rate/100;
				if (flag & BF_LONG)
					damage = damage * config.gvg_long_damage_rate/100;
			}
			if (flag&BF_MAGIC)
				damage = damage * config.gvg_magic_damage_rate/100;
			if (flag&BF_MISC)
				damage = damage * config.gvg_misc_damage_rate/100;
		}
		else if (config.pk_mode && *bl == BL_PC)
		{
			if (flag & BF_WEAPON)
			{
				if (flag & BF_SHORT)
					damage = damage * 80/100;
				if (flag & BF_LONG)
					damage = damage * 70/100;
			}
			if (flag & BF_MAGIC)
				damage = damage * 60/100;
			if(flag & BF_MISC)
				damage = damage * 60/100;
		}
		if(damage < 1) damage  = 1;
	}

	if(config.skill_min_damage || flag&BF_MISC) {
		if(div_ < 255) {
			if(damage > 0 && damage < div_)
				damage = div_;
		}
		else if(damage > 0 && damage < 3)
			damage = 3;
	}

	if( md!=NULL && md->hp>0 && damage > 0 )	// 反撃などのMOBスキル判定
		mobskill_event(*md,flag);

	return damage;
}

/*==========================================
 * HP/SP吸収の計算
 *------------------------------------------
 */
int battle_calc_drain(int damage, int rate, int per, int val)
{
	int diff = 0;

	if (damage <= 0)
		return 0;

	if (per && rand()%100 < rate) {
		diff = (damage * per) / 100;
		if (diff == 0) {
			if (per > 0)
				diff = 1;
			else
				diff = -1;
		}
	}
	if( val 
		//&& rand()%100 < rate
		)
	{	//Absolute leech/penalties have 100% chance. [Skotlex]
		diff += val;
	}
	return diff;
}

/*==========================================
 * 修練ダメージ
 *------------------------------------------
 */
int battle_addmastery(map_session_data *sd,block_list *target,int damage,int type)
{
	int skill;
	int race=target->get_race();
	int weapon;


	nullpo_retr(0, sd);

	// デーモンベイン(+3 〜 +30) vs 不死 or 悪魔 (死人は含めない？)
	if((skill = sd->skill_check(AL_DEMONBANE)) > 0 && (target->is_undead() || race==6) )
		damage += (skill*(int)(3+(sd->status.base_level+1)*0.05));	// submitted by orn
		//damage += (skill * 3);

	// ビーストベイン(+4 〜 +40) vs 動物 or 昆虫
	if((skill = sd->skill_check(HT_BEASTBANE)) > 0 && (race==2 || race==4) )
		damage += (skill * 4);

	if(type == 0)
		weapon = sd->weapontype1;
	else
		weapon = sd->weapontype2;
	switch(weapon)
	{
		case 0x01:	// 短剣 Knife
		case 0x02:	// 1HS
		{
			// 剣修練(+4 〜 +40) 片手剣 短剣含む
			if((skill = sd->skill_check(SM_SWORD)) > 0) {
				damage += (skill * 4);
			}
			break;
		}
		case 0x03:	// 2HS
		{
			// 両手剣修練(+4 〜 +40) 両手剣
			if((skill = sd->skill_check(SM_TWOHAND)) > 0) {
				damage += (skill * 4);
			}
			break;
		}
		case 0x04:	// 1HL
		case 0x05:	// 2HL
		{
			// 槍修練(+4 〜 +40,+5 〜 +50) 槍
			if((skill = sd->skill_check(KN_SPEARMASTERY)) > 0) {
				if( !sd->is_riding() )
					damage += (skill * 4);	// ペコに乗ってない
				else
					damage += (skill * 5);	// ペコに乗ってる
			}
			break;
		}
		case 0x06: // 片手斧
		case 0x07: // Axe by Tato
		{
			if((skill = sd->skill_check(AM_AXEMASTERY)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x08:	// メイス
		{
			// メイス修練(+3 〜 +30) メイス
			if((skill = sd->skill_check(PR_MACEMASTERY)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x09:	// なし?
			break;
		case 0x0a:	// 杖
			break;
		case 0x0b:	// 弓
			break;
		case 0x00:	// 素手 Bare Hands
		case 0x0c:	// Knuckles
		{
			// 鉄拳(+3 〜 +30) 素手,ナックル
			if((skill = sd->skill_check(MO_IRONHAND)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x0d:	// Musical Instrument
		{
			// 楽器の練習(+3 〜 +30) 楽器
			if((skill = sd->skill_check(BA_MUSICALLESSON)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x0e:	// Dance Mastery
		{
			// Dance Lesson Skill Effect(+3 damage for every lvl = +30) 鞭
			if((skill = sd->skill_check(DC_DANCINGLESSON)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x0f:	// Book
		{
			// Advance Book Skill Effect(+3 damage for every lvl = +30) {
			if((skill = sd->skill_check(SA_ADVANCEDBOOK)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x10:	// Katars
		{
			//Advanced Katar Research by zanetheinsane
			if( (skill = sd->skill_check(ASC_KATAR)) > 0 )
				damage += damage*(10+(skill * 2))/100;

			// カタール修練(+3 〜 +30) カタール
			if((skill = sd->skill_check(AS_KATAR)) > 0) {
				//ソニックブロー時は別処理（1撃に付き1/8適応)
				damage += (skill * 3);
			}
			break;
		}
	}
	return (damage);
}

static struct Damage battle_calc_pet_weapon_attack(
	block_list *src,block_list *target,int skill_num,int skill_lv,int wflag)
{
	struct pet_data *pd = (struct pet_data *)src;
	struct mob_data *tmd=NULL;
	int hitrate,flee,cri = 0,atkmin,atkmax;
	unsigned int target_count = 1;
	int def1 = status_get_def(target);
	int def2 = status_get_def2(target);
	int t_vit = target->get_vit();
	struct Damage wd;
	int damage,damage2=0,type,div_,blewcount=skill_get_blewcount(skill_num,skill_lv);
	int flag,dmg_lv=0;
	int t_mode=0,t_size=1,s_ele=0;

	int ignore_def_flag = 0;
	int div_flag=0;	// 0: total damage is to be divided by div_
					// 1: damage is distributed,and has to be multiplied by div_ [celest]

	//return前の処理があるので情報出力部のみ変更
	if( target == NULL || pd == NULL ){ //srcは内容に直接触れていないのでスルーしてみる
		nullpo_info(NLP_MARK);
		memset(&wd,0,sizeof(wd));
		return wd;
	}

	s_ele=status_get_attack_element(src);

	// ターゲット
	if( NULL==(tmd=target->get_md()) )
	{
		memset(&wd,0,sizeof(wd));
		return wd;
	}
	t_size=target->get_size();
	t_mode=target->get_mode();

	flag=BF_SHORT|BF_WEAPON|BF_NORMAL;	// 攻撃の種類の設定

	// 回避率計算、回避判定は後で
	flee = status_get_flee(target);
	if(config.agi_penalty_type > 0 || config.vit_penalty_type > 0)
		target_count += battle_counttargeted(*target,src,config.agi_penalty_count_lv);
	if(config.agi_penalty_type > 0) {
		if(target_count >= config.agi_penalty_count) {
			if(config.agi_penalty_type == 1)
				flee = (flee * (100 - (target_count - (config.agi_penalty_count - 1))*config.agi_penalty_num))/100;
			else if(config.agi_penalty_type == 2)
				flee -= (target_count - (config.agi_penalty_count - 1))*config.agi_penalty_num;
			if(flee < 1) flee = 1;
		}
	}
	hitrate = status_get_hit(src) - flee + 80;

	type=0;	// normal
	if (skill_num > 0) {
		div_ = skill_get_num(skill_num,skill_lv);
		if (div_ < 1) div_ = 1;	//Avoid the rare case where the db says div_ is 0 and below
	}
	else div_ = 1; // single attack

	if(config.pet_str)
		damage = status_get_baseatk(src);
	else
		damage = 0;

	if(skill_num==HW_MAGICCRASHER){			// マジッククラッシャーはMATKで殴る 
		atkmin = status_get_matk1(src);
		atkmax = status_get_matk2(src);
	}else{
		atkmin = status_get_atk(src);
		atkmax = status_get_atk2(src);
	}
	if(atkmin > atkmax) basics::swap(atkmin,atkmax);

	if(mob_db[pd->pet.class_].range>3 )
		flag=(flag&~BF_RANGEMASK)|BF_LONG;

	cri = status_get_critical(src);
	cri -= target->get_luk() * 2; // luk/5*10 => target_luk*2 not target_luk*3
	if(config.enemy_critical_rate != 100) {
		cri = cri*config.enemy_critical_rate/100;
		if(cri < 1)
			cri = 1;
	}
	if(target->has_status(SC_SLEEP))
		cri <<=1;
	if(target->has_status(SC_JOINTBEAT) && target->get_statusvalue2(SC_JOINTBEAT).integer() == 5) // Always take crits with Neck broken by Joint Beat [DracoRPG]
		cri = 1000;

	if(skill_num == 0 && config.enemy_critical && (rand() % 1000) < cri)
	{
		damage += atkmax;
		type = 0x0a;
	}
	else {
		int vitbonusmax;

		if(atkmax > atkmin)
			damage += atkmin + rand() % (atkmax-atkmin + 1);
		else
			damage += atkmin ;
		// スキル修正１（攻撃力倍化系）
		// オーバートラスト(+5% 〜 +25%),他攻撃系スキルの場合ここで補正
		// バッシュ,マグナムブレイク,
		// ボーリングバッシュ,スピアブーメラン,ブランディッシュスピア,スピアスタッブ,
		// メマーナイト,カートレボリューション
		// ダブルストレイフィング,アローシャワー,チャージアロー,
		// ソニックブロー
		if(skill_num>0){
			int i;
			if( (i=skill_get_pl(skill_num))>0 )
				s_ele=i;

			div_=skill_get_num(skill_num,skill_lv); //[Skotlex]
			flag=(flag&~BF_SKILLMASK)|BF_SKILL;
			switch( skill_num ){
			case SM_BASH:		// バッシュ
				damage = damage*(100+ 30*skill_lv)/100;
				hitrate = (hitrate*(100+5*skill_lv))/100;
				break;
			case SM_MAGNUM:		// マグナムブレイク
				damage = damage*(wflag > 1 ? 5*skill_lv+115 : 30*skill_lv+100)/100;
				hitrate = (hitrate*(100+10*skill_lv))/100;
				break;
			case MC_MAMMONITE:	// メマーナイト
				damage = damage*(100+ 50*skill_lv)/100;
				break;
			case AC_DOUBLE:	// ダブルストレイフィング
				damage = damage*(180+ 20*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case AC_SHOWER:	// アローシャワー
				damage = damage*(75 + 5*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case AC_CHARGEARROW:	// チャージアロー
				damage = damage*150/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case KN_AUTOCOUNTER:
				ignore_def_flag = 1;
				break;
			case KN_PIERCE:	// ピアース
				damage = damage*(100+ 10*skill_lv)/100;
				hitrate = hitrate*(100+5*skill_lv)/100;
				div_=t_size+1;
				div_flag = 1;
				break;
			case KN_SPEARSTAB:	// スピアスタブ
				damage = damage*(100+ 15*skill_lv)/100;
				break;
			case KN_SPEARBOOMERANG:	// スピアブーメラン
				damage = damage*(100+ 50*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case KN_BRANDISHSPEAR: // ブランディッシュスピア
				damage = damage*(100+ 20*skill_lv)/100;
				if(skill_lv>3 && wflag==1) damage2+=damage/2;
				if(skill_lv>6 && wflag==1) damage2+=damage/4;
				if(skill_lv>9 && wflag==1) damage2+=damage/8;
				if(skill_lv>6 && wflag==2) damage2+=damage/2;
				if(skill_lv>9 && wflag==2) damage2+=damage/4;
				if(skill_lv>9 && wflag==3) damage2+=damage/2;
				damage +=damage2;
				break;
			case KN_BOWLINGBASH:	// ボウリングバッシュ
				damage = damage*(100+ 50*skill_lv)/100;
				blewcount=0;
				break;
			case AS_GRIMTOOTH:
				damage = damage*(100+ 20*skill_lv)/100;
				break;
			case AS_POISONREACT: // celest
				s_ele = 0;
				damage = damage*(100+ 30*skill_lv)/100;
				break;
			case AS_SONICBLOW:	// ソニックブロウ
				damage = damage*(300+ 50*skill_lv)/100;
				break;
			case TF_SPRINKLESAND:	// 砂まき
				damage = damage*125/100;
				break;
			case MC_CARTREVOLUTION:	// カートレボリューション
				damage = (damage*150)/100;
				break;
			// 以下MOB
			case NPC_COMBOATTACK:	// 多段攻撃
				div_flag = 1;
				break;
			case NPC_RANDOMATTACK:	// ランダムATK攻撃
				damage = damage*(50+rand()%150)/100;
				break;
			// 属性攻撃（適当）
			case NPC_WATERATTACK:
			case NPC_GROUNDATTACK:
			case NPC_FIREATTACK:
			case NPC_WINDATTACK:
			case NPC_POISONATTACK:
			case NPC_HOLYATTACK:
			case NPC_DARKNESSATTACK:
			case NPC_UNDEADATTACK:
			case NPC_TELEKINESISATTACK:
//				div_= pd->skillduration; // [Valaris]
				break;
			case NPC_GUIDEDATTACK:
				hitrate = 1000000;
				break;
			case NPC_RANGEATTACK:
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case NPC_PIERCINGATT:
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
				break;
			case NPC_CRITICALSLASH:
				ignore_def_flag = 1;
				break;
			case RG_BACKSTAP:	// バックスタブ
				damage = damage*(300+ 40*skill_lv)/100;
				hitrate = 1000000;
				break;
			case RG_RAID:	// サプライズアタック
				damage = damage*(100+ 40*skill_lv)/100;
				break;
			case RG_INTIMIDATE:	// インティミデイト
				damage = damage*(100+ 30*skill_lv)/100;
				break;
			case CR_SHIELDCHARGE:	// シールドチャージ
				damage = damage*(100+ 20*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
				s_ele = 0;
				break;
			case CR_SHIELDBOOMERANG:	// シールドブーメラン
				damage = damage*(100+ 30*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				s_ele = 0;
				break;
			case NPC_DARKCROSS:
			case CR_HOLYCROSS:	// ホーリークロス
				damage = damage*(100+ 35*skill_lv)/100;
				break;
			case NPC_GRANDDARKNESS:
			case CR_GRANDCROSS:
				hitrate= 1000000;
				break;
			case AM_DEMONSTRATION:	// デモンストレーション
				hitrate = 1000000;
				damage = damage*(100+ 20*skill_lv)/100;
				damage2 = damage2*(100+ 20*skill_lv)/100;
				break;
			case AM_ACIDTERROR:	// アシッドテラー
				hitrate = 1000000;
				ignore_def_flag = 1;
				damage = damage*(100+ 40*skill_lv)/100;
				damage2 = damage2*(100+ 40*skill_lv)/100;
				break;
			case MO_FINGEROFFENSIVE:	//指弾
				damage = damage * (125 + 25 * skill_lv) / 100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;   //orn
				break;
			case MO_INVESTIGATE:	// 発 勁
				if(def1 < 1000000)
					damage = damage*(100+ 75*skill_lv)/100 * (def1 + def2)/50;
				hitrate = 1000000;
				ignore_def_flag = 1;
				s_ele = 0;
				break;
			case MO_EXTREMITYFIST:	// 阿修羅覇鳳拳
				damage = damage * 8 + 250 + (skill_lv * 150);
				hitrate = 1000000;
				ignore_def_flag = 1;
				s_ele = 0;
				break;
			case MO_CHAINCOMBO:	// 連打掌
				damage = damage*(150+ 50*skill_lv)/100;
				break;
			case MO_COMBOFINISH:	// 猛龍拳
				damage = damage*(240+ 60*skill_lv)/100;
				break;
			case DC_THROWARROW:	// 矢撃ち
				damage = damage*(60+ 40 * skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case BA_MUSICALSTRIKE:	// ミュージカルストライク
				damage = damage*(60+ 40 * skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case CH_TIGERFIST:	// 伏虎拳
				damage = damage*(40+ 100*skill_lv)/100;
				break;
			case CH_CHAINCRUSH:	// 連柱崩撃
				damage = damage*(400+ 100*skill_lv)/100;
				break;
			case CH_PALMSTRIKE:	// 猛虎硬派山
				damage = damage*(200+ 100*skill_lv)/100;
				break;
			case LK_SPIRALPIERCE:			// スパイラルピアース 
			{
				damage = damage*(100+ 50*skill_lv)/100; //増加量が分からないので適当に
				ignore_def_flag = 1;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;

				fightable *fi = target->get_fightable();
				if(fi)
					fi->canmove_tick = gettick() + 1000;
				break;
			}
			case LK_HEADCRUSH:				// ヘッドクラッシュ 
				damage = damage*(100+ 40*skill_lv)/100;
				break;
			case LK_JOINTBEAT:				// ジョイントビート 
				damage = damage*(50+ 10*skill_lv)/100;
				break;
			case ASC_METEORASSAULT:			// メテオアサルト 
				damage = damage*(40+ 40*skill_lv)/100;
				break;
			case SN_SHARPSHOOTING:			// シャープシューティング 
				damage += damage*(100+50*skill_lv)/100;
				break;
			case CG_ARROWVULCAN:			// アローバルカン 
				damage = damage*(200+100*skill_lv)/100;
				break;
			case AS_SPLASHER:		// ベナムスプラッシャー 
				damage = damage*(200+20*skill_lv)/100;
				hitrate = 1000000;
				break;
			case PA_SHIELDCHAIN:	// Shield Chain
				damage = damage*(30*skill_lv)/100;
				flag = (flag&~BF_RANGEMASK)|BF_LONG;
				hitrate += 20;
				div_flag = 1;
				s_ele = 0;				
				break;
			case WS_CARTTERMINATION:
				damage = damage * (80000 / (10 * (16 - skill_lv)) )/100;
				break;
			case CR_ACIDDEMONSTRATION:
				div_flag = 1;
				// damage according to vit and int
				break;
			}
			if (div_flag && div_ > 1) {	// [Skotlex]
				damage *= div_;
				damage2 *= div_;
			}
		}

		// 対 象の防御力によるダメージの減少
		// ディバインプロテクション（ここでいいのかな？）
		if (!ignore_def_flag && def1 < 1000000) {	//DEF, VIT無視
			int t_def;
			target_count = 1 + battle_counttargeted(*target,src,config.vit_penalty_count_lv);
			if(config.vit_penalty_type > 0) {
				if(target_count >= config.vit_penalty_count) {
					if(config.vit_penalty_type == 1) {
						def1 = (def1 * (100 - (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num))/100;
						def2 = (def2 * (100 - (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num))/100;
						t_vit = (t_vit * (100 - (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num))/100;
					}
					else if(config.vit_penalty_type == 2) {
						def1 -= (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num;
						def2 -= (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num;
						t_vit -= (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num;
					}
					if(def1 < 0) def1 = 0;
						if(def2 < 1) def2 = 1;
					if(t_vit < 1) t_vit = 1;
				}
			}
			t_def = def2*8/10;
			vitbonusmax = (t_vit/20)*(t_vit/20)-1;
			if(config.pet_defense_type) {
				damage = damage - (def1 * config.pet_defense_type) - t_def - ((vitbonusmax < 1)?0: rand()%(vitbonusmax+1) );
			}
			else{
				damage = damage * (100 - def1) /100 - t_def - ((vitbonusmax < 1)?0: rand()%(vitbonusmax+1) );
			}
			
		}
	}

	// 0未満だった場合1に補正
	if(damage<1) damage=1;

	// 回避修正
	if(	hitrate < 1000000 )
	{	// 必中攻撃
		if(target->has_status(SC_FOGWALL) && flag&BF_LONG)
			hitrate -= 75;
		if( target->has_status(SC_SLEEP) ||	// 睡眠は必中
			target->has_status(SC_STUN) ||		// スタンは必中
			target->has_status(SC_FREEZE) ||
			(target->has_status(SC_STONE) && target->get_statusvalue2(SC_STONE).integer()==0))	// 凍結は必中
			hitrate = 1000000;
	}
	if(hitrate < 1000000)
		hitrate = ( (hitrate>95)?95: ((hitrate<5)?5:hitrate) );
	if(type == 0 && rand()%100 >= hitrate) {
		damage = damage2 = 0;
		dmg_lv = ATK_FLEE;
	} else {
		dmg_lv = ATK_DEF;
	}


	int cardfix=100;
	if(target->has_status(SC_DEFENDER) && flag&BF_LONG)
		cardfix=cardfix*(100-target->get_statusvalue2(SC_DEFENDER).integer())/100;
	if(target->has_status(SC_FOGWALL) && flag&BF_LONG)
		cardfix=cardfix*50/100;
	if(cardfix != 100)
		damage=damage*cardfix/100;

	if(damage < 0) damage = 0;

	// 属 性の適用
	if(skill_num != 0 || s_ele != 0 || !config.pet_attack_attr_none)
	damage=battle_attr_fix(damage, s_ele, status_get_element(target) );

	if(skill_num==PA_PRESSURE) // プレッシャー 必中? 
		damage = 500+300*skill_lv;

	// インベナム修正
	if(skill_num==TF_POISON){
		damage = battle_attr_fix(damage + 15*skill_lv, s_ele, status_get_element(target) );
	}
	if(skill_num==MC_CARTREVOLUTION){
		damage = battle_attr_fix(damage, 0, status_get_element(target) );
	}

	// 完全回避の判定
	if(config.enemy_perfect_flee) {
		if(skill_num == 0 && tmd!=NULL && rand()%1000 < status_get_flee2(target) ){
			damage=0;
			type=0x0b;
			dmg_lv = ATK_LUCKY;
		}
	}

//	if(def1 >= 1000000 && damage > 0)
	if(t_mode&0x40 && damage > 0)
		damage = 1;

	if( target->is_boss() )
		blewcount = 0;

	if( skill_num != CR_GRANDCROSS && skill_num != NPC_GRANDDARKNESS )
		damage=battle_calc_damage(src,target,damage,div_,skill_num,skill_lv,flag);

	wd.damage=damage;
	wd.damage2=0;
	wd.type=type;
	wd.div_=div_;
	wd.amotion=status_get_amotion(src);
	if(skill_num == KN_AUTOCOUNTER)
		wd.amotion >>= 1;
	wd.dmotion=status_get_dmotion(target);
	wd.blewcount=blewcount;
	wd.flag=flag;
	wd.dmg_lv=dmg_lv;

	return wd;
}

struct Damage battle_calc_mob_weapon_attack(block_list *src,block_list *target,int skill_num,int skill_lv,int wflag)
{
	map_session_data *tsd=NULL;
	struct mob_data* md=(struct mob_data *)src,*tmd=NULL;
	int hitrate,flee,cri = 0,atkmin,atkmax;
	unsigned int target_count = 1;
	int def1 = status_get_def(target);
	int def2 = status_get_def2(target);
	int t_vit = target->get_vit();
	struct Damage wd;
	int damage,damage2=0,type,div_,blewcount=skill_get_blewcount(skill_num,skill_lv);
	int flag,skill,ac_flag = 0,dmg_lv = 0;
	int t_mode=0,t_size=1,s_race=0,s_ele=0,s_size=0,s_race2=0;
	int ignore_def_flag = 0;
	int div_flag=0;	// 0: total damage is to be divided by div_
					// 1: damage is distributed,and has to be multiplied by div_ [celest]

	//return前の処理があるので情報出力部のみ変更
	if( src == NULL || target == NULL || md == NULL ){
		nullpo_info(NLP_MARK);
		memset(&wd,0,sizeof(wd));
		return wd;
	}

	s_race = src->get_race();
	s_ele = status_get_attack_element(src);
	s_size = src->get_size();

	s_race2 = src->get_race2();

	// ターゲット
	tsd = target->get_sd();
	tmd = target->get_md();

	t_size = target->get_size();
	t_mode = target->get_mode();

	if( skill_num == 0 || 
		(tsd && config.pc_auto_counter_type&2) ||
		(tmd && config.monster_auto_counter_type&2))
	{
		if(skill_num != CR_GRANDCROSS && skill_num != NPC_GRANDDARKNESS && target->has_status(SC_AUTOCOUNTER))
		{
			dir_t dir = src->get_direction(*target);
			dir_t t_dir = target->get_dir();
			int dist = distance(*src,*target);
			if(dist <= 0 || !is_same_direction(dir,t_dir) )
			{
				memset(&wd,0,sizeof(wd));
				target->get_statusvalue3(SC_AUTOCOUNTER).integer() = 0;
				target->get_statusvalue4(SC_AUTOCOUNTER).integer() = 1;
				if( !src->has_status(SC_AUTOCOUNTER) )
				{
					int range = target->get_range();
					if( (tsd && tsd->status.weapon != 11 && dist <= range+1) ||
						(tmd && range <= 3 && dist <= range+1) )
						target->get_statusvalue3(SC_AUTOCOUNTER).integer() = src->id;
				}
				return wd;
			}
			else ac_flag = 1;
		}
		else if(skill_num != CR_GRANDCROSS && skill_num != NPC_GRANDDARKNESS && target->has_status(SC_POISONREACT))
		{   // poison react [Celest]
			target->get_statusvalue4(SC_POISONREACT).integer() = 1;
			target->get_statusvalue3(SC_POISONREACT).integer() = src->id;
		}
	}
	flag=BF_SHORT|BF_WEAPON|BF_NORMAL;	// 攻撃の種類の設定

	// 回避率計算、回避判定は後で
	flee = status_get_flee(target);
	if(config.agi_penalty_type > 0 || config.vit_penalty_type > 0)
		target_count += battle_counttargeted(*target,src,config.agi_penalty_count_lv);
	if(config.agi_penalty_type > 0) {
		if(target_count >= config.agi_penalty_count) {
			if(config.agi_penalty_type == 1)
				flee = (flee * (100 - (target_count - (config.agi_penalty_count - 1))*config.agi_penalty_num))/100;
			else if(config.agi_penalty_type == 2)
				flee -= (target_count - (config.agi_penalty_count - 1))*config.agi_penalty_num;
			if(flee < 1) flee = 1;
		}
	}
	hitrate=status_get_hit(src) - flee + 80;

	type=0;	// normal
	if (skill_num > 0) {
		div_ = skill_get_num(skill_num,skill_lv);
		if (div_ < 1) div_ = 1;	//Avoid the rare case where the db says div_ is 0 and below
	} else div_ = 1; // single attack


	if(config.enemy_str)
		damage = status_get_baseatk(src);
	else
		damage = 0;
	if(skill_num==HW_MAGICCRASHER){			// マジッククラッシャーはMATKで殴る 
		atkmin = status_get_matk1(src);
		atkmax = status_get_matk2(src);
	}else{
	atkmin = status_get_atk(src);
	atkmax = status_get_atk2(src);
	}
	if(mob_db[md->class_].range>3 )
		flag=(flag&~BF_RANGEMASK)|BF_LONG;

	if(atkmin > atkmax) atkmin = atkmax;

	if(src->has_status(SC_MAXIMIZEPOWER) ){	// マキシマイズパワー
		atkmin=atkmax;
	}

	cri = status_get_critical(src);
	cri -= target->get_luk() * 3;
	if(config.enemy_critical_rate != 100) {
		cri = cri*config.enemy_critical_rate/100;
		if(cri < 1)
			cri = 1;
	}

	if( target->has_status(SC_SLEEP) )	// 睡眠中はクリティカルが倍に
		cri <<=1;
	if( target->has_status(SC_JOINTBEAT) &&
		target->get_statusvalue2(SC_JOINTBEAT).integer() == 5) // Always take crits with Neck broken by Joint Beat [DracoRPG]
		cri = 1000;


	if(ac_flag) cri = 1000;

	if(skill_num == KN_AUTOCOUNTER) {
		if(!(config.monster_auto_counter_type&1))
			cri = 1000;
		else
			cri <<= 1;
	}

	if(tsd && tsd->critical_def)
		cri = cri * (100 - tsd->critical_def) / 100;

	if((skill_num == 0 || skill_num == KN_AUTOCOUNTER) && skill_lv >= 0 && config.enemy_critical && (rand() % 1000) < cri)	// 判定（スキルの場合は無視）
			// 敵の判定
	{
		damage += atkmax;
		type = 0x0a;
	}
	else {
		int vitbonusmax;

		if(atkmax > atkmin)
			damage += atkmin + rand() % (atkmax-atkmin + 1);
		else
			damage += atkmin ;
		// スキル修正１（攻撃力倍化系）
		// オーバートラスト(+5% 〜 +25%),他攻撃系スキルの場合ここで補正
		// バッシュ,マグナムブレイク,
		// ボーリングバッシュ,スピアブーメラン,ブランディッシュスピア,スピアスタッブ,
		// メマーナイト,カートレボリューション
		// ダブルストレイフィング,アローシャワー,チャージアロー,
		// ソニックブロー
		{ //状態異常中のダメージ追加
			if(src->has_status(SC_OVERTHRUST))	// オーバートラスト
				damage += damage*(5*src->get_statusvalue1(SC_OVERTHRUST).integer())/100;
			if(src->has_status(SC_TRUESIGHT))	// トゥルーサイト
				damage += damage*(2*src->get_statusvalue1(SC_TRUESIGHT).integer())/100;
			if(src->has_status(SC_BERSERK))	// バーサーク
				damage += damage;
			if(src->has_status(SC_AURABLADE))	//[DracoRPG]
				damage += src->get_statusvalue1(SC_AURABLADE).integer() * 20;
			if(src->has_status(SC_MAXOVERTHRUST))
				damage += damage*(20*src->get_statusvalue1(SC_MAXOVERTHRUST).integer())/100;
		}

		if(skill_num>0){
			int i;
			if( (i=skill_get_pl(skill_num))>0 )
				s_ele=i;

			div_=skill_get_num(skill_num,skill_lv); //[Skotlex] div calculation
			flag=(flag&~BF_SKILLMASK)|BF_SKILL;
			switch( skill_num ){
			case SM_BASH:		// バッシュ
				damage = damage*(100+ 30*skill_lv)/100;
				hitrate = (hitrate*(100+5*skill_lv))/100;
				break;
			case SM_MAGNUM:		// マグナムブレイク
				damage = damage*(wflag > 1 ? 5*skill_lv+115 : 30*skill_lv+100)/100;
				hitrate = (hitrate*(100+10*skill_lv))/100;
				break;
			case MC_MAMMONITE:	// メマーナイト
				damage = damage*(100+ 50*skill_lv)/100;
				break;
			case AC_DOUBLE:	// ダブルストレイフィング
				damage = damage*(180+ 20*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case AC_SHOWER:	// アローシャワー
				damage = damage*(75 + 5*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case AC_CHARGEARROW:	// チャージアロー
				damage = damage*150/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case KN_PIERCE:	// ピアース
				damage = damage*(100+ 10*skill_lv)/100;
				hitrate = hitrate*(100+5*skill_lv)/100;
				div_ = t_size+1;
				div_flag = 1;
				break;
			case KN_SPEARSTAB:	// スピアスタブ
				damage = damage*(100+ 15*skill_lv)/100;
				break;
			case KN_SPEARBOOMERANG:	// スピアブーメラン
				damage = damage*(100+ 50*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case KN_BRANDISHSPEAR: // ブランディッシュスピア
				damage = damage*(100+ 20*skill_lv)/100;
				if(skill_lv>3 && wflag==1) damage2+=damage/2;
				if(skill_lv>6 && wflag==1) damage2+=damage/4;
				if(skill_lv>9 && wflag==1) damage2+=damage/8;
				if(skill_lv>6 && wflag==2) damage2+=damage/2;
				if(skill_lv>9 && wflag==2) damage2+=damage/4;
				if(skill_lv>9 && wflag==3) damage2+=damage/2;
				damage +=damage2;
				break;
			case KN_BOWLINGBASH:	// ボウリングバッシュ
				damage = damage*(100+ 50*skill_lv)/100;
				blewcount=0;
				break;
			case KN_AUTOCOUNTER:
				if(config.monster_auto_counter_type&1)
					hitrate += 20;
				else
					hitrate = 1000000;
				ignore_def_flag = 1;
				flag=(flag&~BF_SKILLMASK)|BF_NORMAL;
				break;
			case AS_GRIMTOOTH:
				damage = damage*(100+ 20*skill_lv)/100;
				break;
			case AS_POISONREACT: // celest
				s_ele = 0;
				damage = damage*(100+ 30*skill_lv)/100;
				break;
			case AS_SONICBLOW:	// ソニックブロウ
				damage = damage*(300+ 50*skill_lv)/100;
				break;
			case TF_SPRINKLESAND:	// 砂まき
				damage = damage*125/100;
				break;
			case MC_CARTREVOLUTION:	// カートレボリューション
				damage = (damage*150)/100;
				break;
			// 以下MOB
			case NPC_COMBOATTACK:	// 多段攻撃
				div_flag = 1;
				break;
			case NPC_RANDOMATTACK:	// ランダムATK攻撃
				damage = damage*(50+rand()%150)/100;
				break;
			// 属性攻撃（適当）
			case NPC_WATERATTACK:
			case NPC_GROUNDATTACK:
			case NPC_FIREATTACK:
			case NPC_WINDATTACK:
			case NPC_POISONATTACK:
			case NPC_HOLYATTACK:
			case NPC_DARKNESSATTACK:
			case NPC_UNDEADATTACK:
			case NPC_TELEKINESISATTACK:
				damage = damage*(100+25*(skill_lv-1))/100;
				break;
			case NPC_GUIDEDATTACK:
				hitrate = 1000000;
				break;
			case NPC_RANGEATTACK:
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case NPC_PIERCINGATT:
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
				break;
			case NPC_CRITICALSLASH:
				ignore_def_flag = 1;
				break;
			case RG_BACKSTAP:	// バックスタブ
				damage = damage*(300+ 40*skill_lv)/100;
				hitrate = 1000000;
				break;
			case RG_RAID:	// サプライズアタック
				damage = damage*(100+ 40*skill_lv)/100;
				break;
			case RG_INTIMIDATE:	// インティミデイト
				damage = damage*(100+ 30*skill_lv)/100;
				break;
			case CR_SHIELDCHARGE:	// シールドチャージ
				damage = damage*(100+ 20*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
				s_ele = 0;
				break;
			case CR_SHIELDBOOMERANG:	// シールドブーメラン
				damage = damage*(100+ 30*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				s_ele = 0;
				break;
			case NPC_DARKCROSS:
			case CR_HOLYCROSS:	// ホーリークロス
				damage = damage*(100+ 35*skill_lv)/100;
				break;
			case NPC_GRANDDARKNESS:
			case CR_GRANDCROSS:
				hitrate= 1000000;
				break;
			case AM_DEMONSTRATION:	// デモンストレーション
				hitrate = 1000000;
				damage = damage*(100+ 20*skill_lv)/100;
				damage2 = damage2*(100+ 20*skill_lv)/100;
				break;
			case AM_ACIDTERROR:	// アシッドテラー
				hitrate = 1000000;
				ignore_def_flag = 1;
				damage = damage*(100+ 40*skill_lv)/100;
				damage2 = damage2*(100+ 40*skill_lv)/100;
				break;
			case MO_FINGEROFFENSIVE:	//指弾
				damage = damage * (125 + 25 * skill_lv) / 100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;   //orn
				break;
			case MO_INVESTIGATE:	// 発 勁
				if(def1 < 1000000)
					damage = damage*(100+ 75*skill_lv)/100 * (def1 + def2)/50;
				hitrate = 1000000;
				ignore_def_flag = 1;
				s_ele = 0;
				break;
			case MO_EXTREMITYFIST:	// 阿修羅覇鳳拳
				damage = damage * 8 + 250 + (skill_lv * 150);
				hitrate = 1000000;
				ignore_def_flag = 1;
				s_ele = 0;
				break;
			case MO_CHAINCOMBO:	// 連打掌
				damage = damage*(150+ 50*skill_lv)/100;
				break;
			case BA_MUSICALSTRIKE:	// ミュージカルストライク
				damage = damage*(60+ 40 * skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case DC_THROWARROW:	// 矢撃ち
				damage = damage*(60+ 40 * skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case MO_COMBOFINISH:	// 猛龍拳
				damage = damage*(240+ 60*skill_lv)/100;
				break;
			case CH_TIGERFIST:	// 伏虎拳
				damage = damage*(40+ 100*skill_lv)/100;
				break;
			case CH_CHAINCRUSH:	// 連柱崩撃
				damage = damage*(400+ 100*skill_lv)/100;
				break;
			case CH_PALMSTRIKE:	// 猛虎硬派山
				damage = damage*(200+ 100*skill_lv)/100;
				break;
			case LK_SPIRALPIERCE:			// スパイラルピアース 
				damage = damage*(100+ 50*skill_lv)/100; //増加量が分からないので適当に
				ignore_def_flag = 1;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				if(tsd)
					tsd->canmove_tick = gettick() + 1000;
				else if(tmd)
					tmd->canmove_tick = gettick() + 1000;
				break;
			case LK_HEADCRUSH:				// ヘッドクラッシュ 
				damage = damage*(100+ 40*skill_lv)/100;
				break;
			case LK_JOINTBEAT:				// ジョイントビート 
				damage = damage*(50+ 10*skill_lv)/100;
				break;
			case ASC_METEORASSAULT:			// メテオアサルト 
				damage = damage*(40+ 40*skill_lv)/100;
				break;
			case SN_SHARPSHOOTING:			// シャープシューティング 
				damage += damage*(100+50*skill_lv)/100;
				break;
			case CG_ARROWVULCAN:			// アローバルカン 
				damage = damage*(200+100*skill_lv)/100;
				break;
			case AS_SPLASHER:		// ベナムスプラッシャー 
				damage = damage*(200+20*skill_lv)/100;
				hitrate = 1000000;
				break;
			case PA_SHIELDCHAIN:	// Shield Chain
				damage = damage*(30*skill_lv)/100;
				flag = (flag&~BF_RANGEMASK)|BF_LONG;
				hitrate += 20;
				div_flag = 1;
				s_ele = 0;				
				break;
			case WS_CARTTERMINATION:
				damage = damage * (80000 / (10 * (16 - skill_lv)) )/100;
				break;
			case CR_ACIDDEMONSTRATION:
				div_flag = 1;
				// damage according to vit and int
				break;
			}
			if (div_flag && div_ > 1) {	// [Skotlex]
				damage *= div_;
				damage2 *= div_;
			}
		}

		// 対 象の防御力によるダメージの減少
		// ディバインプロテクション（ここでいいのかな？）
		if (!ignore_def_flag && def1 < 1000000) {	//DEF, VIT無視
			int t_def;
			target_count = 1 + battle_counttargeted(*target,src,config.vit_penalty_count_lv);
			if(config.vit_penalty_type > 0) {
				if(target_count >= config.vit_penalty_count) {
					if(config.vit_penalty_type == 1) {
						def1 = (def1 * (100 - (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num))/100;
						def2 = (def2 * (100 - (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num))/100;
						t_vit = (t_vit * (100 - (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num))/100;
					}
					else if(config.vit_penalty_type == 2) {
						def1 -= (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num;
						def2 -= (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num;
						t_vit -= (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num;
					}
					if(def1 < 0) def1 = 0;
					if(def2 < 1) def2 = 1;
					if(t_vit < 1) t_vit = 1;
				}
			}
			t_def = def2*8/10;
			if( src->is_undead() || s_race==6)
				if(tsd && (skill=tsd->skill_check(AL_DP)) > 0 )
					t_def += skill* (int) (3 + (tsd->status.base_level+1)*0.04);	// submitted by orn
					//t_def += skill*3;

			vitbonusmax = (t_vit/20)*(t_vit/20)-1;
			if(config.monster_defense_type) {
				damage = damage - (def1 * config.monster_defense_type) - t_def - ((vitbonusmax < 1)?0: rand()%(vitbonusmax+1) );
			}
			else{
				damage = damage * (100 - def1) /100 - t_def - ((vitbonusmax < 1)?0: rand()%(vitbonusmax+1) );
			}
		}
	}

	// 0未満だった場合1に補正
	if(damage<1) damage=1;

	// 回避修正
	if(	hitrate < 1000000 )
	{	// 必中攻撃
		if(target->has_status(SC_FOGWALL) && flag&BF_LONG)
			hitrate -= 75;
		if( target->has_status(SC_SLEEP) ||	// 睡眠は必中
			target->has_status(SC_STUN) ||		// スタンは必中
			target->has_status(SC_FREEZE) ||
			(target->has_status(SC_STONE) && target->get_statusvalue2(SC_STONE).integer()==0))	// 凍結は必中
			hitrate = 1000000;
	}
	if(hitrate < 1000000)
		hitrate = ( (hitrate>95)?95: ((hitrate<5)?5:hitrate) );
	if(type == 0 && rand()%100 >= hitrate) {
		damage = damage2 = 0;
		dmg_lv = ATK_FLEE;
	} else {
		dmg_lv = ATK_DEF;
	}

	if(tsd)
	{
		int cardfix=100,i;
		cardfix=cardfix*(100-tsd->subele[s_ele])/100;	// 属 性によるダメージ耐性
		cardfix=cardfix*(100-tsd->subrace[s_race])/100;	// 種族によるダメージ耐性
		cardfix=cardfix*(100-tsd->subsize[s_size])/100;
		cardfix=cardfix*(100-tsd->subrace2[s_race2])/100;	// 種族によるダメージ耐性
		if(src->is_boss())
			cardfix=cardfix*(100-tsd->subrace[10])/100;
		else
			cardfix=cardfix*(100-tsd->subrace[11])/100;
		for(i=0;i<tsd->add_def_class_count; ++i) {
			if(tsd->add_def_classid[i] == md->class_) {
				cardfix=cardfix*(100-tsd->add_def_classrate[i])/100;
				break;
			}
		}
		for(i=0;i<tsd->add_damage_class_count2; ++i) {
			if(tsd->add_damage_classid2[i] == md->class_) {
				cardfix=cardfix*(100+tsd->add_damage_classrate2[i])/100;
				break;
			}
		}
		if(flag&BF_LONG)
			cardfix=cardfix*(100-tsd->long_attack_def_rate)/100;
		if(flag&BF_SHORT)
			cardfix=cardfix*(100-tsd->near_attack_def_rate)/100;
		damage=damage*cardfix/100;
	}
	{
		int cardfix=100;
		if(target->has_status(SC_DEFENDER) && flag&BF_LONG)
			cardfix=cardfix*(100-target->get_statusvalue2(SC_DEFENDER).integer())/100;
		if(target->has_status(SC_FOGWALL) && flag&BF_LONG)
			cardfix=cardfix*50/100;
		if(cardfix != 100)
			damage=damage*cardfix/100;
	}
	if(target->has_status(SC_ASSUMPTIO))
	{	//アシャンプティオ
		if(!maps[target->m].flag.pvp)
			damage=damage/3;
		else
			damage=damage/2;
	}

	if(damage < 0) damage = 0;

	// 属 性の適用
	if (!((config.mob_ghostring_fix == 1) &&
		(status_get_elem_type(target) == 8) &&
		(tsd))) // [MouseJstr]
		if(skill_num != 0 || s_ele != 0 || !config.mob_attack_attr_none)
			damage=battle_attr_fix(damage, s_ele, status_get_element(target) );


	//if(src->has_status(SC_AURABLADE))	// オーラブレード 必中 
	//	damage += src->get_statusvalue1(SC_AURABLADE).integer() * 10;
	if(skill_num==PA_PRESSURE) // プレッシャー 必中? 
		damage = 500+300*skill_lv;

	// インベナム修正
	if(skill_num==TF_POISON){
		damage = battle_attr_fix(damage + 15*skill_lv, s_ele, status_get_element(target) );
	}
	if(skill_num==MC_CARTREVOLUTION){
		damage = battle_attr_fix(damage, 0, status_get_element(target) );
	}

	// 完全回避の判定
	if(skill_num == 0 && tsd!=NULL && rand()%1000 < status_get_flee2(target) ){
		damage=0;
		type=0x0b;
		dmg_lv = ATK_LUCKY;
	}

	if(config.enemy_perfect_flee) {
		if(skill_num == 0 && tmd!=NULL && rand()%1000 < status_get_flee2(target) ){
			damage=0;
			type=0x0b;
			dmg_lv = ATK_LUCKY;
		}
	}

//	if(def1 >= 1000000 && damage > 0)
	if(t_mode&0x40 && damage > 0)
		damage = 1;

	if(target->is_boss())
		blewcount = 0;

	if( tsd && tsd->state.no_weapon_damage)
		damage = 0;

	if(skill_num != CR_GRANDCROSS && skill_num != NPC_GRANDDARKNESS)
		damage=battle_calc_damage(src,target,damage,div_,skill_num,skill_lv,flag);

	wd.damage=damage;
	wd.damage2=0;
	wd.type=type;
	wd.div_=div_;
	wd.amotion=status_get_amotion(src);
	if(skill_num == KN_AUTOCOUNTER)
		wd.amotion >>= 1;
	wd.dmotion=status_get_dmotion(target);
	wd.blewcount=blewcount;
	wd.flag=flag;
	wd.dmg_lv=dmg_lv;
	return wd;
}
/*
 * =========================================================================
 * PCの武器による攻撃
 *-------------------------------------------------------------------------
 */
static struct Damage battle_calc_pc_weapon_attack(
	map_session_data *sd, block_list *target, int skill_num, int skill_lv, int wflag)
{
	map_session_data *tsd=NULL;
	struct mob_data *tmd=NULL;
	int hitrate,flee,cri = 0,atkmin,atkmax;
	int dex;
	unsigned int target_count = 1;
	int no_cardfix=0;
	int def1 = status_get_def(target);
	int def2 = status_get_def2(target);
	int t_vit = target->get_vit();
	struct Damage wd;
	int damage,damage2,damage_rate=100,type,div_,blewcount=skill_get_blewcount(skill_num,skill_lv);
	int flag,skill,dmg_lv = 0;
	int t_mode=0,t_race=0,t_size=1,s_race=7,s_ele=0,s_size=1;
	int t_race2=0;
	int atkmax_=0, atkmin_=0, s_ele_;	//二刀流用
	int watk,watk_,cardfix,t_ele;
	int da=0,i,t_class,ac_flag = 0;
	int ignore_def_flag = 0;
	int idef_flag=0,idef_flag_=0;
	int div_flag=0;	// 0: total damage is to be divided by div_
					// 1: damage is distributed,and has to be multiplied by div_ [celest]

	//return前の処理があるので情報出力部のみ変更
	if( sd == NULL || target == NULL )
	{
		nullpo_info(NLP_MARK);
		memset(&wd,0,sizeof(wd));
		return wd;
	}


	// アタッカー
	s_race=sd->get_race(); //種族
	s_ele=status_get_attack_element(sd); //属性
	s_ele_=status_get_attack_element2(sd); //左手属性
	s_size=sd->get_size();

	t_race2=target->get_race2();

	if(skill_num != CR_GRANDCROSS && skill_num != NPC_GRANDDARKNESS) //グランドクロスでないなら
		sd->state.attack_type = BF_WEAPON; //攻撃タイプは武器攻撃

	// ターゲット
	tsd=target->get_sd();
	tmd=target->get_md();

	t_race=target->get_race(); //対象の種族
	t_ele=status_get_elem_type(target); //対象の属性
	t_size=target->get_size(); //対象のサイズ
	t_mode=target->get_mode(); //対象のMode

//オートカウンター処理ここから
	if( skill_num == 0 || 
		(tsd && config.pc_auto_counter_type&2) ||
		(tmd && config.monster_auto_counter_type&2))
	{
		if(skill_num != CR_GRANDCROSS && skill_num != NPC_GRANDDARKNESS && target->has_status(SC_AUTOCOUNTER))
		{	//グランドクロスでなく、対象がオートカウンター状態の場合
			dir_t dir = sd->get_direction(*target);
			dir_t t_dir = target->get_dir();
			int dist = distance(*sd,*target);
			if(dist <= 0 || !is_same_direction(dir,t_dir) ) { //対象との距離が0以下、または対象の正面？
				memset(&wd,0,sizeof(wd));
				target->get_statusvalue3(SC_AUTOCOUNTER).integer() = 0;
				target->get_statusvalue4(SC_AUTOCOUNTER).integer() = 1;
				if( !sd->has_status(SC_AUTOCOUNTER) ) { //自分がオートカウンター状態
					int range = target->get_range();
					if((tsd && ((map_session_data *)target)->status.weapon != 11 && dist <= range+1) || //対象がPCで武器が弓矢でなく射程内
						(tmd && range <= 3 && dist <= range+1) ) //または対象がMobで射程が3以下で射程内
						target->get_statusvalue3(SC_AUTOCOUNTER).integer() = sd->block_list::id;
				}
				return wd; //ダメージ構造体を返して終了
			}
			else ac_flag = 1;
		} else if(skill_num != CR_GRANDCROSS && skill_num != NPC_GRANDDARKNESS &&
			target->has_status(SC_POISONREACT))
		{   // poison react [Celest]
			target->get_statusvalue4(SC_POISONREACT).integer() = 1;
			target->get_statusvalue3(SC_POISONREACT).integer() = sd->block_list::id;
		}
	}
//オートカウンター処理ここまで

	flag = BF_SHORT|BF_WEAPON|BF_NORMAL;	// 攻撃の種類の設定

	// 回避率計算、回避判定は後で
	flee = status_get_flee(target);
	if(config.agi_penalty_type > 0 || config.vit_penalty_type > 0) //AGI、VITペナルティ設定が有効
		target_count += battle_counttargeted(*target,sd,config.agi_penalty_count_lv);	//対象の数を算出
	if(config.agi_penalty_type > 0) {
		if(target_count >= config.agi_penalty_count) { //ペナルティ設定より対象が多い
			if(config.agi_penalty_type == 1) //回避率がagi_penalty_num%ずつ減少
				flee = (flee * (100 - (target_count - (config.agi_penalty_count - 1))*config.agi_penalty_num))/100;
			else if(config.agi_penalty_type == 2) //回避率がagi_penalty_num分減少
				flee -= (target_count - (config.agi_penalty_count - 1))*config.agi_penalty_num;
			if(flee < 1) flee = 1; //回避率は最低でも1
		}
	}
	hitrate = status_get_hit(sd) - flee + 80; //命中率計算

	type = 0;	// normal
	if (skill_num > 0) {
		div_=skill_get_num(skill_num,skill_lv);
		if (div_ < 1) div_ = 1;	//Avoid the rare case where the db says div_ is 0 and below
	} else div_ = 1; // single attack

	dex = sd->get_dex(); //DEX
	watk = status_get_atk(sd); //ATK
	watk_ = status_get_atk_(sd); //ATK左手

	if(skill_num == HW_MAGICCRASHER)	// マジッククラッシャーはMATKで殴る 
		damage = damage2 = status_get_matk1(sd); //damega,damega2初登場、base_atkの取得
	else
		damage = damage2 = status_get_baseatk(sd); //damega,damega2初登場、base_atkの取得

	atkmin = atkmin_ = dex; //最低ATKはDEXで初期化？
	sd->state.arrow_atk = 0; //arrow_atk初期化
	if(sd->equip_index[9] < MAX_INVENTORY && sd->inventory_data[sd->equip_index[9]])
		atkmin = atkmin*(80 + sd->inventory_data[sd->equip_index[9]]->wlv*20)/100;
	if(sd->equip_index[8] < MAX_INVENTORY && sd->inventory_data[sd->equip_index[8]])
		atkmin_ = atkmin_*(80 + sd->inventory_data[sd->equip_index[8]]->wlv*20)/100;
	if(sd->status.weapon == 11) { //武器が弓矢の場合
		atkmin = watk * ((atkmin<watk)? atkmin:watk)/100; //弓用最低ATK計算
		flag=(flag&~BF_RANGEMASK)|BF_LONG; //遠距離攻撃フラグを有効
		if(sd->arrow_ele > 0) //属性矢なら属性を矢の属性に変更
			s_ele = sd->arrow_ele;
		sd->state.arrow_atk = 1; //arrow_atk有効化
	}

	// サイズ修正
	// ペコ騎乗していて、槍で攻撃した場合は中型のサイズ修正を100にする
	// ウェポンパーフェクション,ドレイクC
	if(sd->state.no_sizefix ||
		skill_num == MO_EXTREMITYFIST ||
		(sd->has_status(SC_WEAPONPERFECTION)) ||
		(sd->is_riding() && (sd->status.weapon == 4 || sd->status.weapon == 5) && t_size == 1))
	{
		atkmax = watk;
		atkmax_ = watk_;
	} else {
		atkmax = (watk * sd->right_weapon.atkmods[ t_size ]) / 100;
		atkmin = (atkmin * sd->right_weapon.atkmods[ t_size ]) / 100;
		atkmax_ = (watk_ * sd->left_weapon.atkmods[ t_size ]) / 100;
		atkmin_ = (atkmin_ * sd->left_weapon.atkmods[ t_size ]) / 100;
	}

	if (sd->has_status(SC_MAXIMIZEPOWER)) {	// マキシマイズパワー
		atkmin = atkmax;
		atkmin_ = atkmax_;
	}
	
	if (atkmin > atkmax && !(sd->state.arrow_atk)) atkmin = atkmax;	//弓は最低が上回る場合あり
	if (atkmin_ > atkmax_) atkmin_ = atkmax_;	

	if (skill_num == 0) {
		//ダブルアタック判定
		int da_rate = sd->skill_check(TF_DOUBLE) * 5;
		if (sd->weapontype1 == 0x01 && da_rate > 0)
			da = (rand()%100 < da_rate) ? 1 : 0;
		//三段掌	 // triple blow works with bows ^^ [celest]
		if (sd->status.weapon <= 16 && (skill = sd->skill_check(MO_TRIPLEATTACK)) > 0)
			da = (rand()%100 < (30 - skill)) ? 2 : 0;
		if (da == 0 && sd->double_rate > 0)
			// success rate from Double Attack is counted in
			da = (rand()%100 < sd->double_rate + da_rate) ? 1 : 0;
	}

	// 過剰精錬ボーナス
	if (sd->right_weapon.overrefine > 0)
		damage += (rand() % sd->right_weapon.overrefine) + 1;
	if (sd->left_weapon.overrefine > 0)
		damage2 += (rand() % sd->left_weapon.overrefine) + 1;

	if (da == 0) { //ダブルアタックが発動していない
		// クリティカル計算
		cri = status_get_critical(sd) + sd->critaddrace[t_race];

		if (sd->state.arrow_atk)
			cri += sd->arrow_cri;
		if(sd->status.weapon == 16)	// カタールの場合、クリティカルを倍に
			cri <<= 1;
		cri -= target->get_luk() * 3;

		{
			if(target->has_status(SC_SLEEP))	// 睡眠中はクリティカルが倍に
				cri <<=1;
			if(target->has_status(SC_JOINTBEAT) &&
				target->get_statusvalue2(SC_JOINTBEAT).integer() == 5) // Always take crits with Neck broken by Joint Beat [DracoRPG]
				cri = 1000;
		}
		if (ac_flag) cri = 1000;

		if (skill_num == KN_AUTOCOUNTER) {
			if (!(config.pc_auto_counter_type&1))
				cri = 1000;
			else
				cri <<= 1;
		}
		else if (skill_num == SN_SHARPSHOOTING)
			cri += 200;
	}

	if(tsd && tsd->critical_def)
		cri = cri * (100-tsd->critical_def) / 100;

	if (da == 0 && (skill_num == 0 || skill_num == KN_AUTOCOUNTER || skill_num == SN_SHARPSHOOTING) && //ダブルアタックが発動していない
		(rand() % 1000) < cri)	// 判定（スキルの場合は無視）
	{
		damage += atkmax;
		damage2 += atkmax_;
		if (sd->atk_rate != 100 || sd->weapon_atk_rate != 0) {
			if (sd->status.weapon < 16) {
				damage = (damage * (sd->atk_rate + sd->weapon_atk_rate[sd->status.weapon]))/100;
				damage2 = (damage2 * (sd->atk_rate + sd->weapon_atk_rate[sd->status.weapon]))/100;
			}
		}
		if (sd->state.arrow_atk)
			damage += sd->arrow_atk;
		damage += damage * sd->crit_atk_rate / 100;
		type = 0x0a;
	} else {
		int vitbonusmax;

		if(atkmax > atkmin)
			damage += atkmin + rand() % (atkmax-atkmin + 1);
		else
			damage += atkmin ;
		if(atkmax_ > atkmin_)
			damage2 += atkmin_ + rand() % (atkmax_-atkmin_ + 1);
		else
			damage2 += atkmin_ ;
		if(sd->atk_rate != 100 || sd->weapon_atk_rate != 0) {
			if (sd->status.weapon < 16) {
				damage = (damage * (sd->atk_rate + sd->weapon_atk_rate[sd->status.weapon]))/100;
				damage2 = (damage2 * (sd->atk_rate + sd->weapon_atk_rate[sd->status.weapon]))/100;
			}
		}

		if(sd->state.arrow_atk) {
			if(sd->arrow_atk > 0)
				damage += rand()%(sd->arrow_atk+1);
			hitrate += sd->arrow_hit;
		}

		if(skill_num != MO_INVESTIGATE && def1 < 1000000) {
			if(sd->right_weapon.def_ratio_atk_ele & (1<<t_ele) || sd->right_weapon.def_ratio_atk_race & (1<<t_race)) {
				damage = (damage * (def1 + def2))/100;
				idef_flag = 1;
			}
			if(sd->left_weapon.def_ratio_atk_ele & (1<<t_ele) || sd->left_weapon.def_ratio_atk_race & (1<<t_race)) {
				damage2 = (damage2 * (def1 + def2))/100;
				idef_flag_ = 1;
			}
			if(target->is_boss()) {
				if(!idef_flag && sd->right_weapon.def_ratio_atk_race & (1<<10)) {
					damage = (damage * (def1 + def2))/100;
					idef_flag = 1;
				}
				if(!idef_flag_ && sd->left_weapon.def_ratio_atk_race & (1<<10)) {
					damage2 = (damage2 * (def1 + def2))/100;
					idef_flag_ = 1;
				}
			}
			else {
				if(!idef_flag && sd->right_weapon.def_ratio_atk_race & (1<<11)) {
					damage = (damage * (def1 + def2))/100;
					idef_flag = 1;
				}
				if(!idef_flag_ && sd->left_weapon.def_ratio_atk_race & (1<<11)) {
					damage2 = (damage2 * (def1 + def2))/100;
					idef_flag_ = 1;
				}
			}
		}

		// スキル修正１（攻撃力倍化系）
		// オーバートラスト(+5% 〜 +25%),他攻撃系スキルの場合ここで補正
		// バッシュ,マグナムブレイク,
		// ボーリングバッシュ,スピアブーメラン,ブランディッシュスピア,スピアスタッブ,
		// メマーナイト,カートレボリューション
		// ダブルストレイフィング,アローシャワー,チャージアロー,
		// ソニックブロー
		{ //状態異常中のダメージ追加
			if(sd->has_status(SC_OVERTHRUST))	// オーバートラスト
				damage_rate += 5*sd->get_statusvalue1(SC_OVERTHRUST).integer();
			if(sd->has_status(SC_TRUESIGHT))	// トゥルーサイト
				damage_rate += 2*sd->get_statusvalue1(SC_TRUESIGHT).integer();
			if(sd->has_status(SC_BERSERK))	// バーサーク
				damage_rate += 200;
			if(sd->has_status(SC_MAXOVERTHRUST))
				damage_rate += 20*sd->get_statusvalue1(SC_MAXOVERTHRUST).integer();
		}

		if(skill_num>0)
		{
			if( (i=skill_get_pl(skill_num))>0 )
				s_ele=s_ele_=i;

			div_=skill_get_num(skill_num,skill_lv); //[Skotlex] Added number of hits calc
			flag=(flag&~BF_SKILLMASK)|BF_SKILL;
			switch( skill_num ){
			case SM_BASH:		// バッシュ
				damage_rate += 30*skill_lv;
				hitrate = (hitrate*(100+5*skill_lv))/100;
				break;
			case SM_MAGNUM:		// マグナムブレイク
				// 20*skill level+100? i think this will do for now [based on jRO info]
				damage_rate += (wflag > 1 ? 5*skill_lv+15 : 30*skill_lv);
				hitrate = (hitrate*(100+10*skill_lv))/100;
				break;
			case MC_MAMMONITE:	// メマーナイト
				damage_rate += 50*skill_lv;
				break;
			case AC_DOUBLE:	// ダブルストレイフィング
				if(!sd->state.arrow_atk && sd->arrow_atk > 0) {
					int arr = rand()%(sd->arrow_atk+1);
					damage += arr;
					damage2 += arr;
				}
				damage_rate += 80+ 20*skill_lv;
				if(sd->arrow_ele > 0) {
					s_ele = sd->arrow_ele;
					s_ele_ = sd->arrow_ele;
				}
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				sd->state.arrow_atk = 1;
				break;
			case AC_SHOWER:	// アローシャワー
				if(!sd->state.arrow_atk && sd->arrow_atk > 0) {
					int arr = rand()%(sd->arrow_atk+1);
					damage += arr;
					damage2 += arr;
				}
				damage_rate += 5*skill_lv-25;
				if(sd->arrow_ele > 0) {
					s_ele = sd->arrow_ele;
					s_ele_ = sd->arrow_ele;
				}
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				sd->state.arrow_atk = 1;
				break;
			case AC_CHARGEARROW:	// チャージアロー
				if(!sd->state.arrow_atk && sd->arrow_atk > 0) {
					int arr = rand()%(sd->arrow_atk+1);
					damage += arr;
					damage2 += arr;
				}
				damage_rate += 50;
				if(sd->arrow_ele > 0) {
					s_ele = sd->arrow_ele;
					s_ele_ = sd->arrow_ele;
				}
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				sd->state.arrow_atk = 1;
				break;
			case KN_PIERCE:	// ピアース
				damage_rate += 10*skill_lv;
				hitrate=hitrate*(100+5*skill_lv)/100;
				div_=t_size+1;
				div_flag=1;
				break;
			case KN_SPEARSTAB:	// スピアスタブ
				damage_rate += 15*skill_lv;
				blewcount=0;
				break;
			case KN_SPEARBOOMERANG:	// スピアブーメラン
				damage_rate += 50*skill_lv;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case KN_BRANDISHSPEAR: // ブランディッシュスピア
				damage_rate += 20*skill_lv;
				if(skill_lv>3 && wflag==1) damage_rate += 100/2;
				if(skill_lv>6 && wflag==1) damage_rate += 100/4;
				if(skill_lv>9 && wflag==1) damage_rate += 100/8;
				if(skill_lv>6 && wflag==2) damage_rate += 100/2;
				if(skill_lv>9 && wflag==2) damage_rate += 100/4;
				if(skill_lv>9 && wflag==3) damage_rate += 100/2;
				break;
			case KN_BOWLINGBASH:	// ボウリングバッシュ
				damage_rate = 50*skill_lv;
				blewcount=0;
				break;
			case KN_AUTOCOUNTER:
				if(config.pc_auto_counter_type&1)
					hitrate += 20;
				else
					hitrate = 1000000;
				ignore_def_flag = 1;
				flag=(flag&~BF_SKILLMASK)|BF_NORMAL;
				break;
			case AS_GRIMTOOTH:
				damage_rate += 20*skill_lv;
				break;
			case AS_POISONREACT: // celest
				s_ele = 0;
				damage_rate += 30*skill_lv;
				break;
			case AS_SONICBLOW:	// ソニックブロウ
				hitrate+=30; // hitrate +30, thanks to midas
				damage_rate += 200+ 50*skill_lv;
				break;
			case TF_SPRINKLESAND:	// 砂まき
				damage_rate += 30;
				break;
			case MC_CARTREVOLUTION:	// カートレボリューション
				if(sd->cart_max_weight > 0 && sd->cart_weight > 0) {
					damage_rate += 50 + sd->cart_weight/800;	//fixed CARTREV damage [Lupus] // should be 800, not 80... weight is *10 ^_- [celest]
				}
				else {
					damage_rate += 50;
				}
				break;
			// 以下MOB
			case NPC_COMBOATTACK:	// 多段攻撃
				div_flag=1;
				break;
			case NPC_RANDOMATTACK:	// ランダムATK攻撃
				damage_rate += rand()%150-50;
				break;
			// 属性攻撃（適当）
			case NPC_WATERATTACK:
			case NPC_GROUNDATTACK:
			case NPC_FIREATTACK:
			case NPC_WINDATTACK:
			case NPC_POISONATTACK:
			case NPC_HOLYATTACK:
			case NPC_DARKNESSATTACK:
			case NPC_UNDEADATTACK:
			case NPC_TELEKINESISATTACK:
				damage_rate += 25*skill_lv;
				break;
			case NPC_GUIDEDATTACK:
				hitrate = 1000000;
				break;
			case NPC_RANGEATTACK:
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case NPC_PIERCINGATT:
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
				break;
			case NPC_CRITICALSLASH:
				ignore_def_flag = 1;
				break;
			case RG_BACKSTAP:	// バックスタブ
				if(config.backstab_bow_penalty == 1 && sd->status.weapon == 11){
					damage_rate += 50+ 20*skill_lv; // Back Stab with a bow does twice less damage
				}else{
					damage_rate += 200+ 40*skill_lv;
				}
				hitrate = 1000000;
				break;
			case RG_RAID:	// サプライズアタック
				damage_rate += 40*skill_lv;
				break;
			case RG_INTIMIDATE:	// インティミデイト
				damage_rate += 30*skill_lv;
				break;
			case CR_SHIELDCHARGE:	// シールドチャージ
				damage_rate += 20*skill_lv;
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
				s_ele = 0;
				break;
			case CR_SHIELDBOOMERANG:	// シールドブーメラン
				damage_rate += 30*skill_lv;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				s_ele = 0;
				break;
			case NPC_DARKCROSS:
			case CR_HOLYCROSS:	// ホーリークロス
				damage_rate += 35*skill_lv;
				break;
			case NPC_GRANDDARKNESS:
			case CR_GRANDCROSS:
				hitrate= 1000000;
				if(!config.gx_cardfix)
					no_cardfix = 1;
				break;
			case AM_DEMONSTRATION:	// デモンストレーション
				damage_rate += 20*skill_lv;
				no_cardfix = 1;
				break;
			case AM_ACIDTERROR:	// アシッドテラー
				hitrate = 1000000;
				damage_rate += 40*skill_lv;
				s_ele = 0;
				s_ele_ = 0;
				ignore_def_flag = 1;
				no_cardfix = 1;
				break;
			case MO_FINGEROFFENSIVE:	//指弾
				damage_rate += 25 + 25 * skill_lv;
				if(config.finger_offensive_type == 0) {
					div_ = sd->spiritball_old;
					div_flag = 1;
				}
				else div_ = 1;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;   //orn
				break;
			case MO_INVESTIGATE:	// 発 勁
				if(def1 < 1000000) {
					damage = damage*(def1 + def2)/50;
					damage2 = damage2*(def1 + def2)/50;
					damage_rate += 75*skill_lv;
				}
				hitrate = 1000000;
				ignore_def_flag = 1;
				s_ele = 0;
				s_ele_ = 0;
				break;
			case MO_EXTREMITYFIST:	// 阿修羅覇鳳拳
				damage = damage * (8 + (sd->status.sp/10)) + 250 + (skill_lv * 150);
				sd->status.sp = 0;
				clif_updatestatus(*sd,SP_SP);
				hitrate = 1000000;
				ignore_def_flag = 1;
				s_ele = 0;
				s_ele_ = 0;
				break;
			case MO_CHAINCOMBO:	// 連打掌
				damage_rate += 50+ 50*skill_lv;
				break;
			case MO_COMBOFINISH:	// 猛龍拳
				damage_rate += 140+ 60*skill_lv;;
				break;
			case BA_MUSICALSTRIKE:	// ミュージカルストライク
				damage_rate += 40*skill_lv-40;
				if(sd->arrow_ele > 0) {
					s_ele = sd->arrow_ele;
					s_ele_ = sd->arrow_ele;
				}
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case DC_THROWARROW:	// 矢撃ち
				if(!sd->state.arrow_atk && sd->arrow_atk > 0) {
					int arr = rand()%(sd->arrow_atk+1);
					damage += arr;
					damage2 += arr;
				}
				damage_rate += 50 * skill_lv;
				if(sd->arrow_ele > 0) {
					s_ele = sd->arrow_ele;
					s_ele_ = sd->arrow_ele;
				}
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				sd->state.arrow_atk = 1;
				break;
			case CH_TIGERFIST:	// 伏虎拳
				damage_rate +=  100*skill_lv-60;
				break;
			case CH_CHAINCRUSH:	// 連柱崩撃
				damage_rate += 300+ 100*skill_lv;
				break;
			case CH_PALMSTRIKE:	// 猛虎硬派山
				damage_rate += 100+ 100*skill_lv;
				break;
			case LK_SPIRALPIERCE:			// スパイラルピアース 
				damage_rate += 50*skill_lv; //増加量が分からないので適当に
				ignore_def_flag = 1;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				if(tsd)
					tsd->canmove_tick = gettick() + 1000;
				else if(tmd)
					tmd->canmove_tick = gettick() + 1000;
				break;
			case LK_HEADCRUSH:				// ヘッドクラッシュ 
				damage_rate += 40*skill_lv;
				break;
			case LK_JOINTBEAT:				// ジョイントビート 
				damage_rate += 10*skill_lv-50;
				break;
			case ASC_METEORASSAULT:			// メテオアサルト 
				damage_rate += 40*skill_lv-60;
				no_cardfix = 1;
				break;
			case SN_SHARPSHOOTING:			// シャープシューティング 
				damage_rate += 100+50*skill_lv;
				break;
			case CG_ARROWVULCAN:			// アローバルカン 
				damage_rate += 100+100*skill_lv;
				if(sd->arrow_ele > 0) {
					s_ele = sd->arrow_ele;
					s_ele_ = sd->arrow_ele;
				}
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case AS_SPLASHER:		// ベナムスプラッシャー 
				damage_rate += 100+20*skill_lv+20*sd->skill_check(AS_POISONREACT);
				no_cardfix = 1;
				hitrate = 1000000;
				break;
			case ASC_BREAKER:
				// calculate physical part of damage
				damage_rate += 100*skill_lv-100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case PA_SHIELDCHAIN:
				damage_rate += 30*skill_lv-100;
				flag = (flag&~BF_RANGEMASK)|BF_LONG;
				hitrate += 20;
				div_flag = 1;
				s_ele = 0;
				break;
			case WS_CARTTERMINATION:
				if(sd->cart_max_weight > 0 && sd->cart_weight > 0) {
					damage_rate += sd->cart_weight/(10*(16-skill_lv))-100;
				}
				break;
			case CR_ACIDDEMONSTRATION:
				div_flag = 1;
				// damage according to vit and int
				break;
			}

			//what about normal attacks? [celest]
			//damage *= damage_rate/100;
			//damage2 *= damage_rate/100;

			if (div_flag && div_ > 1) {	// [Skotlex]
				damage *= div_;
				damage2 *= div_;
			}
			if (sd && skill_num > 0 && sd->skillatk[0] == skill_num)
				damage += damage*sd->skillatk[1]/100;
		}

		damage = damage * damage_rate / 100;
		damage2 = damage2 * damage_rate / 100;

		if(da == 2) { //三段掌が発動しているか
			type = 0x08;
			div_ = 255;	//三段掌用に…
			damage = damage * (100 + 20 * sd->skill_check( MO_TRIPLEATTACK)) / 100;
		}

		// 対 象の防御力によるダメージの減少
		// ディバインプロテクション（ここでいいのかな？）
		if (!ignore_def_flag && def1 < 1000000) {	//DEF, VIT無視
			int t_def;
			target_count = 1 + battle_counttargeted(*target,sd,config.vit_penalty_count_lv);
			if(config.vit_penalty_type > 0) {
				if(target_count >= config.vit_penalty_count) {
					if(config.vit_penalty_type == 1) {
						def1 = (def1 * (100 - (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num))/100;
						def2 = (def2 * (100 - (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num))/100;
						t_vit = (t_vit * (100 - (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num))/100;
					}
					else if(config.vit_penalty_type == 2) {
						def1 -= (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num;
						def2 -= (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num;
						t_vit -= (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num;
					}
					if(def1 < 0) def1 = 0;
					if(def2 < 1) def2 = 1;
					if(t_vit < 1) t_vit = 1;
				}
			}
			t_def = def2*8/10;
			vitbonusmax = (t_vit/20)*(t_vit/20)-1;
			if (tmd) {
				if(target->is_boss()) {
					if(sd->right_weapon.ignore_def_mob & 2)
						idef_flag = 1;
					if(sd->left_weapon.ignore_def_mob & 2)
						idef_flag_ = 1;
				} else {
					if(sd->right_weapon.ignore_def_mob & 1)
						idef_flag = 1;
					if(sd->left_weapon.ignore_def_mob & 1)
						idef_flag_ = 1;
				}
			}
			if(sd->right_weapon.ignore_def_ele & (1<<t_ele) || sd->right_weapon.ignore_def_race & (1<<t_race))
				idef_flag = 1;
			if(sd->left_weapon.ignore_def_ele & (1<<t_ele) || sd->left_weapon.ignore_def_race & (1<<t_race))
				idef_flag_ = 1;
			if(target->is_boss()) {
				if(sd->right_weapon.ignore_def_race & (1<<10))
					idef_flag = 1;
				if(sd->left_weapon.ignore_def_race & (1<<10))
					idef_flag_ = 1;
			}
			else {
				if(sd->right_weapon.ignore_def_race & (1<<11))
					idef_flag = 1;
				if(sd->left_weapon.ignore_def_race & (1<<11))
					idef_flag_ = 1;
			}

			if(!idef_flag){
				if(config.player_defense_type) {
					damage = damage - (def1 * config.player_defense_type) - t_def - ((vitbonusmax < 1)?0: rand()%(vitbonusmax+1) );
				}
				else{
					damage = damage * (100 - def1) /100 - t_def - ((vitbonusmax < 1)?0: rand()%(vitbonusmax+1) );
				}
			}
			if(!idef_flag_){
				if(config.player_defense_type) {
					damage2 = damage2 - (def1 * config.player_defense_type) - t_def - ((vitbonusmax < 1)?0: rand()%(vitbonusmax+1) );
				}
				else{
					damage2 = damage2 * (100 - def1) /100 - t_def - ((vitbonusmax < 1)?0: rand()%(vitbonusmax+1) );
				}
			}
		}
	}

	// 状態異常中のダメージ追加でクリティカルにも有効なスキル
	{
		// エンチャントデッドリーポイズン
		if(!no_cardfix && sd->has_status(SC_EDP) &&
			skill_num != ASC_BREAKER && skill_num != ASC_METEORASSAULT)
		{
			damage += damage * (150 + sd->get_statusvalue1(SC_EDP).integer() * 50) / 100;
			no_cardfix = 1;
		}
		// sacrifice works on boss monsters, and does 9% damage to self [Celest]
		if (!skill_num && sd->has_status(SC_SACRIFICE))
		{
			int self_damage = sd->get_max_hp() * 9/100;
			//sd->heal(-dmg, 0);
			pc_damage(*sd, self_damage, sd);
			clif_damage(*sd,*sd, gettick(), 0, 0, self_damage, 0 , 0, 0);
			damage = self_damage * (90 + sd->get_statusvalue1(SC_SACRIFICE).integer() * 10) / 100;
			if (maps[sd->block_list::m].flag.gvg)
				damage = 6*damage/10; //40% less effective on siege maps. [Skotlex]
			damage2 = 0;
			hitrate = 1000000;
			s_ele = 0;
			s_ele_ = 0;
			skill_num = PA_SACRIFICE;
			if( --sd->get_statusvalue2(SC_SACRIFICE).integer() == 0 )
				status_change_end(sd, SC_SACRIFICE,-1);
		}
	}

	// 精錬ダメージの追加
	if( skill_num != MO_INVESTIGATE && skill_num != MO_EXTREMITYFIST) {			//DEF, VIT無視
		damage += status_get_atk2(sd);
		damage2 += status_get_atk_2(sd);
	}
	if(skill_num == CR_SHIELDBOOMERANG || skill_num == PA_SHIELDCHAIN) {
		if(sd->equip_index[8] < MAX_INVENTORY) {
			int index = sd->equip_index[8];
			if(sd->inventory_data[index] && sd->inventory_data[index]->type == 5) {
				damage += sd->inventory_data[index]->weight/10;
				damage += sd->status.inventory[index].refine * status_getrefinebonus(0,1);
			}
		}
	}
	else if(skill_num == LK_SPIRALPIERCE) {			// スパイラルピアース 
		if(sd->equip_index[9] < MAX_INVENTORY) {	//重量で追加ダメージらしいのでシールドブーメランを参考に追加
			int index = sd->equip_index[9];
			if(sd->inventory_data[index] && sd->inventory_data[index]->type == 4) {
				damage += (sd->inventory_data[index]->weight*(skill_lv*4*4/10/5));
				damage += sd->status.inventory[index].refine * status_getrefinebonus(0,1);
			}
		}
	}

	// 0未満だった場合1に補正
	if(damage<1) damage=1;
	if(damage2<1) damage2=1;

	// スキル修正２（修練系）
	// 修練ダメージ(右手のみ) ソニックブロー時は別処理（1撃に付き1/8適応)
	if( skill_num != MO_INVESTIGATE && skill_num != MO_EXTREMITYFIST && 
		skill_num != CR_GRANDCROSS && skill_num != NPC_GRANDDARKNESS )
	{	//修練ダメージ無視
		damage = battle_addmastery(sd,target,damage,0);
		damage2 = battle_addmastery(sd,target,damage2,1);
	}

	// Aura Blade adds dmg [DracoRPG]
	if(sd->has_status(SC_AURABLADE))
	{
		damage += sd->get_statusvalue1(SC_AURABLADE).integer() * 20;
		damage2 += sd->get_statusvalue1(SC_AURABLADE).integer() * 20;
	}

	// A weapon forged by a Blacksmith in the Fame Top 10 gets +10 dmg [DracoRPG]
	if(sd->right_weapon.fameflag)
		damage += 10;
	if(sd->left_weapon.fameflag)
	    damage2 += 10;

	if (sd->perfect_hit > 0 && rand()%100 < sd->perfect_hit)
		hitrate = 1000000;

	// 回避修正
	if (hitrate < 1000000)
	{			// 必中攻撃
		if (target->has_status(SC_FOGWALL) && flag&BF_LONG)
			hitrate -= 75;
		if (target->has_status(SC_SLEEP) ||	// 睡眠は必中
			target->has_status(SC_STUN) ||		// スタンは必中
			target->has_status(SC_FREEZE) ||
			(target->has_status(SC_STONE) && target->get_statusvalue2(SC_STONE).integer()==0))	// 凍結は必中
			hitrate = 1000000;
	}
	// weapon research hidden bonus
	if ((skill = sd->skill_check(BS_WEAPONRESEARCH)) > 0) {
		hitrate = hitrate * (100+2*skill) / 100;
	}
	if(hitrate < 5)
		hitrate = 5;
	if (type == 0 && rand()%100 >= hitrate) {
		damage = damage2 = 0;
		dmg_lv = ATK_FLEE;
	} else {
		dmg_lv = ATK_DEF;
	}

	// スキル修正３（武器研究）
	if ((skill = sd->skill_check(BS_WEAPONRESEARCH)) > 0) {
		damage += skill*2;
		damage2 += skill*2;
	}

//スキルによるダメージ補正ここまで

//カードによるダメージ追加処理ここから
	cardfix=100;
	if(!sd->state.arrow_atk) { //弓矢以外
		if(!config.left_cardfix_to_right) { //左手カード補正設定無し
			cardfix=cardfix*(100+sd->right_weapon.addrace[t_race])/100;	// 種族によるダメージ修正
			cardfix=cardfix*(100+sd->right_weapon.addele[t_ele])/100;	// 属性によるダメージ修正
			cardfix=cardfix*(100+sd->right_weapon.addsize[t_size])/100;	// サイズによるダメージ修正
			cardfix=cardfix*(100+sd->right_weapon.addrace2[t_race2])/100;
		}
		else {
			cardfix=cardfix*(100+sd->right_weapon.addrace[t_race]+sd->left_weapon.addrace[t_race])/100;	// 種族によるダメージ修正(左手による追加あり)
			cardfix=cardfix*(100+sd->right_weapon.addele[t_ele]+sd->left_weapon.addele[t_ele])/100;	// 属性によるダメージ修正(左手による追加あり)
			cardfix=cardfix*(100+sd->right_weapon.addsize[t_size]+sd->left_weapon.addsize[t_size])/100;	// サイズによるダメージ修正(左手による追加あり)
			cardfix=cardfix*(100+sd->right_weapon.addrace2[t_race2]+sd->left_weapon.addrace2[t_race2])/100;
		}
	}
	else { //弓矢
		cardfix=cardfix*(100+sd->right_weapon.addrace[t_race]+sd->arrow_addrace[t_race])/100;	// 種族によるダメージ修正(弓矢による追加あり)
		cardfix=cardfix*(100+sd->right_weapon.addele[t_ele]+sd->arrow_addele[t_ele])/100;	// 属性によるダメージ修正(弓矢による追加あり)
		cardfix=cardfix*(100+sd->right_weapon.addsize[t_size]+sd->arrow_addsize[t_size])/100;	// サイズによるダメージ修正(弓矢による追加あり)
		cardfix=cardfix*(100+sd->right_weapon.addrace2[t_race2])/100;
	}
	if(target->is_boss()) { //ボス
		if(!sd->state.arrow_atk) { //弓矢攻撃以外なら
			if(!config.left_cardfix_to_right) //左手カード補正設定無し
				cardfix=cardfix*(100+sd->right_weapon.addrace[10])/100; //ボスモンスターに追加ダメージ
			else //左手カード補正設定あり
				cardfix=cardfix*(100+sd->right_weapon.addrace[10]+sd->left_weapon.addrace[10])/100; //ボスモンスターに追加ダメージ(左手による追加あり)
		}
		else //弓矢攻撃
			cardfix=cardfix*(100+sd->right_weapon.addrace[10]+sd->arrow_addrace[10])/100; //ボスモンスターに追加ダメージ(弓矢による追加あり)
	}
	else { //ボスじゃない
		if(!sd->state.arrow_atk) { //弓矢攻撃以外
			if(!config.left_cardfix_to_right) //左手カード補正設定無し
				cardfix=cardfix*(100+sd->right_weapon.addrace[11])/100; //ボス以外モンスターに追加ダメージ
			else //左手カード補正設定あり
				cardfix=cardfix*(100+sd->right_weapon.addrace[11]+sd->left_weapon.addrace[11])/100; //ボス以外モンスターに追加ダメージ(左手による追加あり)
		}
		else
			cardfix=cardfix*(100+sd->right_weapon.addrace[11]+sd->arrow_addrace[11])/100; //ボス以外モンスターに追加ダメージ(弓矢による追加あり)
	}
	//特定Class用補正処理(少女の日記→ボンゴン用？)
	t_class = target->get_class();
	for(i=0;i<sd->right_weapon.add_damage_class_count; ++i) {
		if(sd->right_weapon.add_damage_classid[i] == t_class) {
			cardfix=cardfix*(100+sd->right_weapon.add_damage_classrate[i])/100;
			break;
		}
	}
	if(!no_cardfix)
		damage=damage*cardfix/100; //カード補正によるダメージ増加
//カードによるダメージ増加処理ここまで

//カードによるダメージ追加処理(左手)ここから
	cardfix=100;
	if(!config.left_cardfix_to_right) {  //左手カード補正設定無し
		cardfix=cardfix*(100+sd->left_weapon.addrace[t_race])/100;	// 種族によるダメージ修正左手
		cardfix=cardfix*(100+sd->left_weapon.addele[t_ele])/100;	// 属 性によるダメージ修正左手
		cardfix=cardfix*(100+sd->left_weapon.addsize[t_size])/100;	// サイズによるダメージ修正左手
		cardfix=cardfix*(100+sd->left_weapon.addrace2[t_race2])/100;
		if(target->is_boss()) //ボス
			cardfix=cardfix*(100+sd->left_weapon.addrace[10])/100; //ボスモンスターに追加ダメージ左手
		else
			cardfix=cardfix*(100+sd->left_weapon.addrace[11])/100; //ボス以外モンスターに追加ダメージ左手
	}
	//特定Class用補正処理左手(少女の日記→ボンゴン用？)
	for(i=0;i<sd->left_weapon.add_damage_class_count; ++i) {
		if(sd->left_weapon.add_damage_classid[i] == t_class) {
			cardfix=cardfix*(100+sd->left_weapon.add_damage_classrate[i])/100;
			break;
		}
	}
	if(!no_cardfix)
		damage2=damage2*cardfix/100;

//カード補正による左手ダメージ増加
//カードによるダメージ増加処理(左手)ここまで

//カードによるダメージ減衰処理ここから
	if(tsd){ //対象がPCの場合
		cardfix=100;
		cardfix=cardfix*(100-tsd->subrace[s_race])/100;	// 種族によるダメージ耐性
		cardfix=cardfix*(100-tsd->subele[s_ele])/100;	// 属性によるダメージ耐性
		cardfix=cardfix*(100-tsd->subsize[s_size])/100;
		if(sd->is_boss())
			cardfix=cardfix*(100-tsd->subrace[10])/100; //ボスからの攻撃はダメージ減少
		else
			cardfix=cardfix*(100-tsd->subrace[11])/100; //ボス以外からの攻撃はダメージ減少
		//特定Class用補正処理左手(少女の日記→ボンゴン用？)
		for(i=0;i<tsd->add_def_class_count; ++i) {
			if(tsd->add_def_classid[i] == sd->status.class_) {
				cardfix=cardfix*(100-tsd->add_def_classrate[i])/100;
				break;
			}
		}
		if(flag&BF_LONG)
			cardfix=cardfix*(100-tsd->long_attack_def_rate)/100; //遠距離攻撃はダメージ減少(ホルンCとか)
		if(flag&BF_SHORT)
			cardfix=cardfix*(100-tsd->near_attack_def_rate)/100; //近距離攻撃はダメージ減少(該当無し？)
		damage=damage*cardfix/100; //カード補正によるダメージ減少
		damage2=damage2*cardfix/100; //カード補正による左手ダメージ減少
	}
//カードによるダメージ減衰処理ここまで

//対象にステータス異常がある場合のダメージ減算処理ここから
	{
		cardfix=100;
		if(target->has_status(SC_DEFENDER) && flag&BF_LONG) //ディフェンダー状態で遠距離攻撃
			cardfix=cardfix*(100-target->get_statusvalue2(SC_DEFENDER).integer())/100; //ディフェンダーによる減衰
		if(target->has_status(SC_FOGWALL) && flag&BF_LONG)
			cardfix=cardfix*50/100;
		if(cardfix != 100) {
			damage=damage*cardfix/100; //ディフェンダー補正によるダメージ減少
			damage2=damage2*cardfix/100; //ディフェンダー補正による左手ダメージ減少
		}
		if(target->has_status(SC_ASSUMPTIO)){ //アスムプティオ
			if(!maps[target->m].flag.pvp){
				damage=damage/3;
				damage2=damage2/3;
			}else{
				damage=damage/2;
				damage2=damage2/2;
			}
		}
	}
//対象にステータス異常がある場合のダメージ減算処理ここまで

	if(damage < 0) damage = 0;
	if(damage2 < 0) damage2 = 0;

	// 属 性の適用
	damage=battle_attr_fix(damage,s_ele, status_get_element(target) );
	damage2=battle_attr_fix(damage2,s_ele_, status_get_element(target) );

	// 星のかけら、気球の適用
	damage += sd->right_weapon.star;
	damage2 += sd->left_weapon.star;
	damage += sd->spiritball*3;
	damage2 += sd->spiritball*3;

	if(skill_num==PA_PRESSURE)
	{	// プレッシャー 必中? 
		damage = 500+300*skill_lv;
		damage2 = 500+300*skill_lv;
	}

	// >二刀流の左右ダメージ計算誰かやってくれぇぇぇぇえええ！
	// >map_session_data に左手ダメージ(atk,atk2)追加して
	// >status_calc_pc()でやるべきかな？
	// map_session_data に左手武器(atk,atk2,ele,star,atkmods)追加して
	// status_calc_pc()でデータを入力しています

	//左手のみ武器装備
	if(sd->weapontype1 == 0 && sd->weapontype2 > 0) {
		damage = damage2;
		damage2 = 0;
	}

	// 右手、左手修練の適用
	if(sd->status.weapon > 16) {// 二刀流か?
		int dmg = damage, dmg2 = damage2;
		// 右手修練(60% 〜 100%) 右手全般
		skill = sd->skill_check(AS_RIGHT);
		damage = damage * (50 + (skill * 10))/100;
		if(dmg > 0 && damage < 1) damage = 1;
		// 左手修練(40% 〜 80%) 左手全般
		skill = sd->skill_check(AS_LEFT);
		damage2 = damage2 * (30 + (skill * 10))/100;
		if(dmg2 > 0 && damage2 < 1) damage2 = 1;
	}
	else //二刀流でなければ左手ダメージは0
		damage2 = 0;

	// 右手,短剣のみ
	if(da == 1) { //ダブルアタックが発動しているか
		div_ = 2;
		damage += damage;
		type = 0x08;
	}

	if(sd->status.weapon == 16) {
		// カタール追撃ダメージ
		skill = sd->skill_check(TF_DOUBLE);
		damage2 = damage * (1 + (skill * 2))/100;
		if(damage > 0 && damage2 < 1) damage2 = 1;
	}

	// インベナム修正
	if(skill_num==TF_POISON){
		damage = battle_attr_fix(damage + 15*skill_lv, s_ele, status_get_element(target) );
	}
	if(skill_num==MC_CARTREVOLUTION){
		damage = battle_attr_fix(damage, 0, status_get_element(target) );
	}

	// 完全回避の判定
	if(skill_num == 0 && tsd!=NULL && div_ < 255 && rand()%1000 < status_get_flee2(target) ){
		damage=damage2=0;
		type=0x0b;
		dmg_lv = ATK_LUCKY;
	}

	// 対象が完全回避をする設定がONなら
	if(config.enemy_perfect_flee) {
		if(skill_num == 0 && tmd!=NULL && div_ < 255 && rand()%1000 < status_get_flee2(target) ) {
			damage=damage2=0;
			type=0x0b;
			dmg_lv = ATK_LUCKY;
		}
	}

	//MobのModeに頑強フラグが立っているときの処理
	if(t_mode&0x40){
		if(damage > 0)
			damage = 1;
		if(damage2 > 0)
			damage2 = 1;
	}

	if(target->is_boss())
		blewcount = 0;

	//bNoWeaponDamage(設定アイテム無し？)でグランドクロスじゃない場合はダメージが0
	if( tsd && tsd->state.no_weapon_damage && 
		skill_num != CR_GRANDCROSS && skill_num != NPC_GRANDDARKNESS )
		damage = damage2 = 0;

	if( skill_num != CR_GRANDCROSS && skill_num != NPC_GRANDDARKNESS && 
		(damage > 0 || damage2 > 0) )
	{
		if(damage2<1)		// ダメージ最終修正
			damage=battle_calc_damage(sd,target,damage,div_,skill_num,skill_lv,flag);
		else if(damage<1)	// 右手がミス？
			damage2=battle_calc_damage(sd,target,damage2,div_,skill_num,skill_lv,flag);
		else {	// 両 手/カタールの場合はちょっと計算ややこしい
			int d1=damage+damage2,d2=damage2;
			damage=battle_calc_damage(sd,target,damage+damage2,div_,skill_num,skill_lv,flag);
			damage2=d1?((d2*100/d1)*damage/100):0;
			if(damage > 1 && damage2 < 1) damage2=1;
			damage-=damage2;
		}
	}

	// For executioner card [Valaris]				*/
	if(sd->random_attack_increase_add > 0 && sd->random_attack_increase_per > 0 && skill_num == 0 )
	{
		if(rand()%100 < sd->random_attack_increase_per)
		{
			if(damage >0) damage*=sd->random_attack_increase_add/100;
			if(damage2 >0) damage2*=sd->random_attack_increase_add/100;
		}
	}

	// for azoth weapon [Valaris]
	if( tmd && sd->classchange )
	{
		 if(rand()%10000 < sd->classchange)
		 {
			 static int changeclass[]={
				1001,1002,1004,1005,1007,1008,1009,1010,1011,1012,1013,1014,1015,1016,1018,1019,1020,
				1021,1023,1024,1025,1026,1028,1029,1030,1031,1032,1033,1034,1035,1036,1037,1040,1041,
				1042,1044,1045,1047,1048,1049,1050,1051,1052,1053,1054,1055,1056,1057,1058,1060,1061,
				1062,1063,1064,1065,1066,1067,1068,1069,1070,1071,1076,1077,1078,1079,1080,1081,1083,
				1084,1085,1094,1095,1097,1099,1100,1101,1102,1103,1104,1105,1106,1107,1108,1109,1110,
				1111,1113,1114,1116,1117,1118,1119,1121,1122,1123,1124,1125,1126,1127,1128,1129,1130,
				1131,1132,1133,1134,1135,1138,1139,1140,1141,1142,1143,1144,1145,1146,1148,1149,1151,
				1152,1153,1154,1155,1156,1158,1160,1161,1163,1164,1165,1166,1167,1169,1170,1174,1175,
				1176,1177,1178,1179,1180,1182,1183,1184,1185,1188,1189,1191,1192,1193,1194,1195,1196,
				1197,1199,1200,1201,1202,1204,1205,1206,1207,1208,1209,1211,1212,1213,1214,1215,1216,
				1219,1242,1243,1245,1246,1247,1248,1249,1250,1253,1254,1255,1256,1257,1258,1260,1261,
				1263,1264,1265,1266,1267,1269,1270,1271,1273,1274,1275,1276,1277,1278,1280,1281,1282,
				1291,1292,1293,1294,1295,1297,1298,1300,1301,1302,1304,1305,1306,1308,1309,1310,1311,
				1313,1314,1315,1316,1317,1318,1319,1320,1321,1322,1323,1364,1365,1366,1367,1368,1369,
				1370,1371,1372,1374,1375,1376,1377,1378,1379,1380,1381,1382,1383,1384,1385,1386,1387,
				1390,1391,1392,1400,1401,1402,1403,1404,1405,1406,1408,1409,1410,1412,1413,1415,1416,
				1417,1493,1494,1495,1497,1498,1499,1500,1502,1503,1504,1505,1506,1507,1508,1509,1510,
				1511,1512,1513,1514,1515,1516,1517,1519,1520,1582,1584,1585,1586,1587 };
			mob_class_change( *tmd, changeclass, sizeof(changeclass)/sizeof(changeclass[0]));
		}
	}

	wd.damage=damage;
	wd.damage2=damage2;
	wd.type=type;
	wd.div_=div_;
	wd.amotion=status_get_amotion(sd);
	if(skill_num == KN_AUTOCOUNTER)
		wd.amotion >>= 1;
	wd.dmotion=status_get_dmotion(target);
	wd.blewcount=blewcount;
	wd.flag=flag;
	wd.dmg_lv=dmg_lv;

	return wd;
}

/*==========================================
 * battle_calc_weapon_attack_sub (by Skotlex)
 *------------------------------------------
 */
struct Damage battle_calc_weapon_attack_sub(block_list *src,block_list *target,int skill_num,int skill_lv,int wflag)
{
	map_session_data *sd=NULL, *tsd=NULL;
	struct mob_data *md=NULL, *tmd=NULL;
	struct pet_data *pd=NULL, *tpd=NULL;
	struct Damage wd;
	short skill=0;
	unsigned short skillratio = 100;	//Skill dmg modifiers

	short i;
	short t_mode = target->get_mode();
	short t_size = target->get_size();
	short t_race, t_ele, s_race;
	short s_ele, s_ele_;
	short def1, def2;

	struct bcwasf	{	//bcwasf stands for battle_calc_weapon_attack_sub flags
		unsigned hit : 1; //the attack Hit? (not a miss)
		unsigned cri : 1;		//Critical hit
		unsigned idef : 1;	//Ignore defense
		unsigned idef2 : 1;	//Ignore defense (left weapon)
		unsigned infdef : 1;	//Infinite defense (plants?)
		unsigned arrow : 1;	//Attack is arrow-based
		unsigned rh : 1;		//Attack considers right hand (wd.damage)
		unsigned lh : 1;		//Attack considers left hand (wd.damage2)
		unsigned cardfix : 1;
	}	flag;	

	memset(&wd,0,sizeof(wd));
	memset(&flag,0,sizeof(struct bcwasf));

	if(src==NULL || target==NULL)
	{
		nullpo_info(NLP_MARK);
		return wd;
	}
	//Initial flag
	flag.rh=1;
	flag.cardfix=1;
	flag.infdef=(t_mode&0x40?1:0);

	//Initial Values
	wd.type=0; //Normal attack
	wd.div_=skill_num?skill_get_num(skill_num,skill_lv):1;
	wd.amotion=status_get_amotion(src);
	if(skill_num == KN_AUTOCOUNTER)
		wd.amotion >>= 1;
	wd.dmotion=status_get_dmotion(target);
	wd.blewcount=skill_get_blewcount(skill_num,skill_lv);
	wd.flag=BF_SHORT|BF_WEAPON|BF_NORMAL; //Initial Flag
	wd.dmg_lv=ATK_DEF;	//This assumption simplifies the assignation later

	(NULL==( sd=   src->get_sd())) && (NULL==( md=   src->get_md())) && (NULL==( pd=   src->get_pd()));
	(NULL==(tsd=target->get_sd())) && (NULL==(tmd=target->get_md())) && (NULL==(tpd=target->get_pd()));
	
	if( (tsd && pd) || tpd)
	{	// pets can't target players
		// pets cannot be targeted
		memset(&wd,0,sizeof(wd));	
		return wd;
	}


	if(sd && skill_num != CR_GRANDCROSS && skill_num != NPC_GRANDDARKNESS)
		sd->state.attack_type = BF_WEAPON;

	//Set miscelanous data that needs be filled regardless of hit/miss
	if(sd)
	{
		if (sd->status.weapon == 11)
		{
			wd.flag=(wd.flag&~BF_RANGEMASK)|BF_LONG;
			flag.arrow = 1;
		}
	}
	else if( src->get_range() > 3 )
		wd.flag=(wd.flag&~BF_RANGEMASK)|BF_LONG;

	if(skill_num){
		wd.flag=(wd.flag&~BF_SKILLMASK)|BF_SKILL;
		switch(skill_num)
		{		
			case AC_DOUBLE:
			case AC_SHOWER:
			case AC_CHARGEARROW:
			case BA_MUSICALSTRIKE:
			case DC_THROWARROW:
			case CG_ARROWVULCAN:
				wd.flag=(wd.flag&~BF_RANGEMASK)|BF_LONG;
				flag.arrow = 1;
				break;

			case MO_FINGEROFFENSIVE:
				if(sd && config.finger_offensive_type == 0)
					wd.div_ = sd->spiritball_old;
			case KN_SPEARBOOMERANG:
			case NPC_RANGEATTACK:
			case CR_SHIELDBOOMERANG:
			case LK_SPIRALPIERCE:
			case ASC_BREAKER:
			case PA_SHIELDCHAIN:
			case AM_ACIDTERROR:
			case NPC_GRANDDARKNESS:
			case CR_GRANDCROSS:	//GrandCross really shouldn't count as short-range, aight?
			case ITM_TOMAHAWK:	//Tomahawk is a ranged attack! [Skotlex]
				wd.flag=(wd.flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case KN_PIERCE:
				wd.div_= t_size+1;
				break;

			case KN_SPEARSTAB:
			case KN_BOWLINGBASH:
				wd.blewcount=0;
				break;

			case NPC_PIERCINGATT:
			case CR_SHIELDCHARGE:
				wd.flag=(wd.flag&~BF_RANGEMASK)|BF_SHORT;
				break;

			case KN_AUTOCOUNTER:
				wd.flag=(wd.flag&~BF_SKILLMASK)|BF_NORMAL;
				break;
		}
	}

	if(target->is_boss()) //Bosses can't be knocked-back
		wd.blewcount = 0;

	if (sd)
	{	//Arrow consumption
		sd->state.arrow_atk = flag.arrow;
	}

	//Check for counter 
	if( skill_num != CR_GRANDCROSS && skill_num != NPC_GRANDDARKNESS &&
 		(!skill_num ||
		(tsd && config.pc_auto_counter_type&2) ||
		(tmd && config.monster_auto_counter_type&2)))
	{
		if(target->has_status(SC_AUTOCOUNTER))
		{
			dir_t dir = target->get_direction(*src);
			dir_t t_dir = target->get_dir();
			int dist = distance(*src,*target);
			if(dist <= 0 || !is_same_direction(dir,t_dir) )
			{
				memset(&wd,0,sizeof(wd));
				target->get_statusvalue3(SC_AUTOCOUNTER).integer() = 0;
				target->get_statusvalue4(SC_AUTOCOUNTER).integer() = 1;
				if( !src->has_status(SC_AUTOCOUNTER) )
				{ //How can the attacking char have Auto-counter active?
					int range = target->get_range();
					if((tsd && tsd->status.weapon != 11 && dist <= range+1) ||
						(tmd && range <= 3 && dist <= range+1))
						target->get_statusvalue3(SC_AUTOCOUNTER).integer() = src->id;
				}
				return wd;
			} else
				flag.cri = 1;
		}
		else if(target->has_status(SC_POISONREACT))
		{	// poison react [Celest]
			target->get_statusvalue4(SC_POISONREACT).integer() = 1;
			target->get_statusvalue3(SC_POISONREACT).integer() = src->id;
		}
	}	//End counter-check

	if(sd && !skill_num && !flag.cri)
	{	//Check for conditions that convert an attack to a skill
		char da=0;
		skill = 0;
		if((sd->weapontype1 == 0x01 && (skill = sd->skill_check(TF_DOUBLE)) > 0) ||
			sd->double_rate > 0) //success rate from Double Attack is counted in
			da = (rand()%100 <  sd->double_rate + 5*skill) ? 1:0;
		if((skill = sd->skill_check(MO_TRIPLEATTACK)) > 0 && sd->status.weapon <= 16) // triple blow works with bows ^^ [celest]
			da = (rand()%100 < (30 - skill)) ? 2:da;
		
		if (da == 1)
		{
			skill_num = TF_DOUBLE;
			wd.div_ = 2;
		} else if (da == 2) {
			skill_num = MO_TRIPLEATTACK;
			skill_lv = skill;
			wd.div_ = 255;
		}
		
		if (da)
			wd.type = 0x08;
	}

	if (!skill_num && !flag.cri && src->has_status(SC_SACRIFICE))
	{
		skill_num = PA_SACRIFICE;
		skill_lv =  src->get_statusvalue1(SC_SACRIFICE).integer();
	}

	if (!skill_num && (tsd || config.enemy_perfect_flee))
	{	//Check for Lucky Dodge
		short flee2 = status_get_flee2(target);
		if (rand()%1000 < flee2)
		{
			wd.type=0x0b;
			wd.dmg_lv=ATK_LUCKY;
			return wd;
		}
	}

	//Initialize variables that will be used afterwards

	t_race = target->get_race();
	t_ele = status_get_elem_type(target);
	s_race = src->get_race();
	s_ele=status_get_attack_element(src);
	s_ele_=status_get_attack_element2(src);

	if (flag.arrow && sd && sd->arrow_ele)
		s_ele = sd->arrow_ele;

	if (sd)
	{	//Set whether damage1 or damage2 (or both) will be used
		if(sd->weapontype1 == 0 && sd->weapontype2 > 0)
			{
				flag.rh=0;
				flag.lh=1;
			}
		if(sd->status.weapon > 16)
			flag.rh = flag.lh = 1;
	}

	//Check for critical
	if(!flag.cri &&
		(sd || config.enemy_critical) &&
		(!skill_num || skill_num == KN_AUTOCOUNTER || skill_num == SN_SHARPSHOOTING))
	{
		short cri = status_get_critical(src);
		if (sd)
		{
			cri+= sd->critaddrace[t_race];
			if(flag.arrow)
				cri += sd->arrow_cri;
			if(sd->status.weapon == 16)
				cri <<=1;
		}
		//The official equation is *2, but that only applies when sd's do critical.
		//Therefore, we use the old value 3 on cases when an sd gets attacked by a mob
		cri -= target->get_luk() * (md&&tsd?3:2);
		if(!sd && config.enemy_critical_rate != 100)
		{ //Mob/Pets
			cri = cri*config.enemy_critical_rate/100;
			if (cri<1) cri = 1;
		}
		
		{
			if (target->has_status(SC_SLEEP) )
				cri <<=1;
			if(target->has_status(SC_JOINTBEAT) &&
				target->get_statusvalue2(SC_JOINTBEAT).integer() == 6) // Always take crits with Neck broken by Joint Beat [DracoRPG]
				flag.cri=1;
		}
		switch (skill_num)
		{
			case KN_AUTOCOUNTER:
				if(!(config.pc_auto_counter_type&1))
					flag.cri = 1;
				else
					cri <<= 1;
				break;
			case SN_SHARPSHOOTING:
				cri += 200;
				break;
		}
		if(tsd && tsd->critical_def)
			cri = cri*(100-tsd->critical_def)/100;
		if (rand()%1000 < cri)
			flag.cri= 1;
	}
	if (flag.cri)
	{
		wd.type = 0x0a;
		flag.idef = flag.idef2 = flag.hit = 1;
	} else {	//Check for Perfect Hit
		if(sd && sd->perfect_hit > 0 && rand()%100 < sd->perfect_hit)
			flag.hit = 1;
		if (skill_num && !flag.hit)
			switch(skill_num)
		{
			case NPC_GUIDEDATTACK:
			case RG_BACKSTAP:
			case NPC_GRANDDARKNESS:
			case CR_GRANDCROSS:
			case AM_ACIDTERROR:
			case MO_INVESTIGATE:
			case MO_EXTREMITYFIST:
			case PA_PRESSURE:
			case PA_SACRIFICE:
				flag.hit = 1;
				break;
		}
		if ((!flag.hit) &&
			(target->has_status(SC_SLEEP) ||
			target->has_status(SC_STUN) ||
			target->has_status(SC_FREEZE) ||
			(target->has_status(SC_STONE) && target->get_statusvalue2(SC_STONE).integer()==0))
			)
			flag.hit = 1;
	}

	if (!flag.hit)
	{	//Hit/Flee calculation
		unsigned short flee = status_get_flee(target);
		unsigned short hitrate=80; //Default hitrate
		if(config.agi_penalty_type)
		{	
			unsigned char target_count; //256 max targets should be a sane max
			target_count = 1+battle_counttargeted(*target,src,config.agi_penalty_count_lv);
			if(target_count >= config.agi_penalty_count)
			{
				if (config.agi_penalty_type == 1)
					flee = (flee * (100 - (target_count - (config.agi_penalty_count - 1))*config.agi_penalty_num))/100;
				else //asume type 2: absolute reduction
					flee -= (target_count - (config.agi_penalty_count - 1))*config.agi_penalty_num;
				if(flee < 1) flee = 1;
			}
		}

		hitrate+= status_get_hit(src) - flee;
		
		if(sd)
		{	
			if (flag.arrow)
				hitrate += sd->arrow_hit;
			// weapon research hidden bonus
			if ((skill = sd->skill_check(BS_WEAPONRESEARCH)) > 0)
				hitrate += hitrate * (2*skill)/100;
		}
		if(skill_num)
			switch(skill_num)
		{	//Hit skill modifiers
			case SM_BASH:
				hitrate += 5*skill_lv;
				break;
			case SM_MAGNUM:
				hitrate += 10*skill_lv;
				break;
			case KN_AUTOCOUNTER:
				hitrate += 20;
				break;
			case KN_PIERCE:
				hitrate += hitrate*(5*skill_lv)/100;
				break;
			case PA_SHIELDCHAIN:
				hitrate += 20;
				break;
		}

		if (hitrate > config.max_hitrate)
			hitrate = config.max_hitrate;
		else if (hitrate < config.min_hitrate)
			hitrate = config.min_hitrate;

		if(rand()%100 >= hitrate)
			wd.dmg_lv = ATK_FLEE;
		else
			flag.hit =1;
	}	//End hit/miss calculation

	if( tsd && tsd->state.no_weapon_damage && 
		skill_num != CR_GRANDCROSS && skill_num != NPC_GRANDDARKNESS)	
		return wd;

	if (flag.hit && !(t_mode&0x40)) //No need to do the math for plants
	{	//Hitting attack

//Assuming that 99% of the cases we will not need to check for the flag.rh... we don't.
//ATK_RATE scales the damage. 100 = no change. 50 is halved, 200 is doubled, etc
#define ATK_RATE( a ) { wd.damage= wd.damage*(a)/100 ; if(flag.lh) wd.damage2= wd.damage2*(a)/100; }
#define ATK_RATE2( a , b ) { wd.damage= wd.damage*(a)/100 ; if(flag.lh) wd.damage2= wd.damage2*(b)/100; }
//Adds dmg%. 100 = +100% (double) damage. 10 = +10% damage
#define ATK_ADDRATE( a ) { wd.damage+= wd.damage*(a)/100 ; if(flag.lh) wd.damage2+= wd.damage2*(a)/100; }
#define ATK_ADDRATE2( a , b ) { wd.damage+= wd.damage*(a)/100 ; if(flag.lh) wd.damage2+= wd.damage2*(b)/100; }
//Adds an absolute value to damage. 100 = +100 damage
#define ATK_ADD( a ) { wd.damage+= a; if (flag.lh) wd.damage2+= a; }
#define ATK_ADD2( a , b ) { wd.damage+= a; if (flag.lh) wd.damage2+= b; }

		if (status_get_def(target) >= 1000000)
			flag.infdef =1;
		def1 = status_get_def(target);
		def2 = status_get_def2(target);
		
		switch (skill_num)
		{	//Calc base damage according to skill
			case PA_SACRIFICE:
				ATK_ADD(src->get_max_hp()* 9/100);
				break;
			case PA_PRESSURE: //Since PRESSURE ignores everything, finish here
				wd.damage=battle_calc_damage(src,target,500+300*skill_lv,wd.div_,skill_num,skill_lv,wd.flag);
				wd.damage2=0;
				return wd;	
			default:
			{
				unsigned short baseatk=0, baseatk_=0, atkmin=0, atkmax=0, atkmin_=0, atkmax_=0;
				if (!sd)
				{	//Mobs/Pets
					if ((md && config.enemy_str) ||
						(pd && config.pet_str))
						baseatk = status_get_baseatk(src);

					if(skill_num==HW_MAGICCRASHER)
					{		  
						if (!flag.cri)
							atkmin = status_get_matk1(src);
						atkmax = status_get_matk2(src);
					} else {
						if (!flag.cri)
							atkmin = status_get_atk(src);
						atkmax = status_get_atk2(src);
					}
					if (atkmin > atkmax)
						atkmin = atkmax;
				} else {	//PCs
					if(skill_num==HW_MAGICCRASHER)
					{
						baseatk = status_get_matk1(src);
						if (flag.lh) baseatk_ = baseatk;
					}
					else
					{ 
						baseatk = status_get_baseatk(src);
						if (flag.lh) baseatk_ = baseatk;
					}
					//rodatazone says that Overrefine bonuses are part of baseatk
					if(sd->right_weapon.overrefine>0)
						baseatk+= rand()%sd->right_weapon.overrefine+1;
					if (flag.lh && sd->left_weapon.overrefine>0)
						baseatk_+= rand()%sd->left_weapon.overrefine+1;
					
					atkmax = status_get_atk(src);
					if (flag.lh)
						atkmax_ = status_get_atk(src);

					if (!flag.cri)
					{	//Normal attacks
						atkmin = atkmin_ = src->get_dex();
						
						if (sd->equip_index[9] < MAX_INVENTORY && sd->inventory_data[sd->equip_index[9]])
							atkmin = atkmin*(80 + sd->inventory_data[sd->equip_index[9]]->wlv*20)/100;
						
						if (atkmin > atkmax)
							atkmin = atkmax;
						
						if(flag.lh)
						{
							if (sd->equip_index[8] < MAX_INVENTORY && sd->inventory_data[sd->equip_index[8]])
								atkmin_ = atkmin_*(80 + sd->inventory_data[sd->equip_index[8]]->wlv*20)/100;
						
							if (atkmin_ > atkmax_)
								atkmin_ = atkmax_;
						}
						
						if(sd->status.weapon == 11)
						{	//Bows
							atkmin = atkmin*atkmax/100;
							if (atkmin > atkmax)
								atkmax = atkmin;
						}
					}
				}
				
				if (src->has_status(SC_MAXIMIZEPOWER))
				{
					atkmin = atkmax;
					atkmin_ = atkmax_;
				}
				//Weapon Damage calculation
				//Store watk in wd.damage to use the above defines for easy handling, and then add baseatk
				if (!flag.cri)
				{
					ATK_ADD2((atkmax>atkmin? rand()%(atkmax-atkmin) :0) +atkmin,
						(atkmax_>atkmin_? rand()%(atkmax_-atkmin_) :0) +atkmin_);
				} else 
					ATK_ADD2(atkmax, atkmax_);
				
				if (sd)
				{
					//rodatazone says the range is 0~arrow_atk-1 for non crit
					if (flag.arrow && sd->arrow_atk)
						ATK_ADD(flag.cri?sd->arrow_atk:rand()%sd->arrow_atk);

					if(sd->status.weapon < 16 && (sd->atk_rate != 100 || sd->weapon_atk_rate[sd->status.weapon] != 0))
						ATK_RATE(sd->atk_rate + sd->weapon_atk_rate[sd->status.weapon]);

					if(flag.cri && sd->crit_atk_rate)
						ATK_ADDRATE(sd->crit_atk_rate);
				
					//SizeFix only for players
					if (!(
						//!tsd || //rodatazone claims that target human players don't have a size! -- I really don't believe it... removed until we find some evidence
						sd->state.no_sizefix ||
						(src->has_status(SC_WEAPONPERFECTION)) ||
						(sd->is_riding() && (sd->status.weapon==4 || sd->status.weapon==5) && t_size==1) ||
						(skill_num == MO_EXTREMITYFIST)
						))
					{
						ATK_RATE2(sd->right_weapon.atkmods[t_size], sd->left_weapon.atkmods[t_size]);
					}
				}
				//Finally, add baseatk
				ATK_ADD2(baseatk, baseatk_);
				break;
			}	//End default case
		} //End switch(skill_num)

		//Skill damage modifiers
		if(skill_num != PA_SACRIFICE)
		{
			if(src->has_status(SC_OVERTHRUST))
				skillratio += 5*src->get_statusvalue1(SC_OVERTHRUST).integer();
			if(src->has_status(SC_TRUESIGHT))
				skillratio += 2*src->get_statusvalue1(SC_TRUESIGHT).integer();
			if(src->has_status(SC_BERSERK))
				skillratio += 100; // Although RagnaInfo says +200%, it's *200% so +100%
			if(src->has_status(SC_MAXOVERTHRUST))
				skillratio += 20*src->get_statusvalue1(SC_MAXOVERTHRUST).integer();
			if(src->has_status(SC_EDP) &&
//				!(t_mode&0x20) &&
//				skill_num != AS_SPLASHER &&
				skill_num != ASC_BREAKER &&
				skill_num != ASC_METEORASSAULT)
			{	
				skillratio += (50 + src->get_statusvalue1(SC_EDP).integer() * 50)*wd.div_;
			}
		}
		if (!skill_num)
		{
			//Executioner card addition - Consider it as part of skill-based-damage
			if(sd &&
				sd->random_attack_increase_add > 0 &&
				sd->random_attack_increase_per &&
				rand()%100 < sd->random_attack_increase_per
				)
				skillratio += sd->random_attack_increase_add;
		
			ATK_RATE(skillratio);
		} else {	//Skills
			char ele_flag=0;	//For skills that force the neutral element.

			switch( skill_num )
			{
				case SM_BASH:
					skillratio+= 30*skill_lv;
					break;
				case SM_MAGNUM:
					// 20*skill level+100? I think this will do for now [based on jRO info]
					skillratio+= (wflag > 1 ? 5*skill_lv+15 : 30*skill_lv);
					break;
				case MC_MAMMONITE:
					skillratio+= 50*skill_lv;
					break;
				case AC_DOUBLE:
					skillratio+= 80+ 20*skill_lv;
					break;
				case AC_SHOWER:
					skillratio+= 5*skill_lv -25;
					break;
				case AC_CHARGEARROW:
					skillratio+= 50;
					break;
				case KN_PIERCE:
					skillratio+= wd.div_*(100+10*skill_lv) -100;
					//div_flag=1;
					break;
				case KN_SPEARSTAB:
					skillratio+= 15*skill_lv;
					break;
				case KN_SPEARBOOMERANG:
					skillratio+= 50*skill_lv;
					break;
				case KN_BRANDISHSPEAR:
				{
					int ratio = 100+20*skill_lv;
					skillratio+=20*skill_lv;
					if(skill_lv>3 && wflag==1) skillratio+= ratio/2;
					if(skill_lv>6 && wflag==1) skillratio+= ratio/4;
					if(skill_lv>9 && wflag==1) skillratio+= ratio/8;
					if(skill_lv>6 && wflag==2) skillratio+= ratio/2;
					if(skill_lv>9 && wflag==2) skillratio+= ratio/4;
					if(skill_lv>9 && wflag==3) skillratio+= ratio/2;
					break;
				}
				case KN_BOWLINGBASH:
					skillratio+= 50*skill_lv;
					break;
				case KN_AUTOCOUNTER:
					flag.idef= flag.idef2= 1;
					break;
				case TF_DOUBLE:
					skillratio += 100;
					break;
				case AS_GRIMTOOTH:
					skillratio+= 20*skill_lv;
					break;
				case AS_POISONREACT: // celest
					skillratio+= 30*skill_lv;
					ele_flag=1;
					break;
				case AS_SONICBLOW:
					skillratio+= 200+ 50*skill_lv;
					break;
				case TF_SPRINKLESAND:
					skillratio+= 25;
					break;
				case MC_CARTREVOLUTION:
					if(sd && sd->cart_max_weight > 0 && sd->cart_weight > 0)
						skillratio+= 50+100*sd->cart_weight/sd->cart_max_weight; // +1% every 1% weight
					else if (!sd)
						skillratio+= 50+150; //Max damage for non players.
					break;
				case NPC_COMBOATTACK:
						skillratio += 100*wd.div_ -100;
						//div_flag=1;
					break;
				case NPC_RANDOMATTACK:
					skillratio+= rand()%150-50;
					break;
				case NPC_WATERATTACK:
				case NPC_GROUNDATTACK:
				case NPC_FIREATTACK:
				case NPC_WINDATTACK:
				case NPC_POISONATTACK:
				case NPC_HOLYATTACK:
				case NPC_DARKNESSATTACK:
				case NPC_UNDEADATTACK:
				case NPC_TELEKINESISATTACK:
					skillratio+= 25*skill_lv;
					break;
				case NPC_GUIDEDATTACK:
				case NPC_RANGEATTACK:
				case NPC_PIERCINGATT:
					break;
				case NPC_CRITICALSLASH:
					flag.idef= flag.idef2= 1;
					break;
				case RG_BACKSTAP:
					if(sd && sd->status.weapon == 11 && config.backstab_bow_penalty)
						skillratio+= (200+ 40*skill_lv)/2;
					else
						skillratio+= 200+ 40*skill_lv;
					break;
				case RG_RAID:
					skillratio+= 40*skill_lv;
					break;
				case RG_INTIMIDATE:
					skillratio+= 30*skill_lv;
					break;
				case CR_SHIELDCHARGE:
					skillratio+= 20*skill_lv;
					ele_flag=1;
					break;
				case CR_SHIELDBOOMERANG:
					skillratio+= 30*skill_lv;
					ele_flag=1;
					break;
				case NPC_DARKCROSS:
				case CR_HOLYCROSS:
					skillratio+= 35*skill_lv;
					break;
				case NPC_GRANDDARKNESS:
				case CR_GRANDCROSS:
					if(!config.gx_cardfix)
						flag.cardfix = 0;
					break;
				case AM_DEMONSTRATION:
					skillratio+= 20*skill_lv;
					flag.cardfix = 0;
					break;
				case AM_ACIDTERROR:
					skillratio+= 40*skill_lv;
					ele_flag=1;
					flag.idef = flag.idef2= 1;
					flag.cardfix = 0;
					break;
				case MO_FINGEROFFENSIVE:
					if(config.finger_offensive_type == 0)
						//div_flag = 1;
						skillratio+= wd.div_ * (125 + 25*skill_lv) -100;
					else
						skillratio+= 25 + 25 * skill_lv;
					break;
				case MO_INVESTIGATE:
					if (!flag.infdef)
					{
						skillratio+=75*skill_lv;
						ATK_RATE((def1 + def2)/2);
					}
					flag.idef= flag.idef2= 1;
					break;
				case MO_EXTREMITYFIST:
					if (sd)
					{	//Overflow check. [Skotlex]
						int ratio = skillratio + 100*(8 + ((sd->status.sp)/10));
						//You'd need something like 6K SP to reach this max, so should be fine for most purposes.
						if (ratio > 60000) ratio = 60000; //We leave some room here in case skill_ratio gets further increased.
						skillratio = ratio;
						sd->status.sp = 0;
						clif_updatestatus(*sd,SP_SP);
					}
					flag.idef= flag.idef2= 1;
					ele_flag=1;
					break;
				case MO_TRIPLEATTACK:
					skillratio+= 20*skill_lv;
					break;
				case MO_CHAINCOMBO:
					skillratio+= 50+ 50*skill_lv;
					break;
				case MO_COMBOFINISH:
					skillratio+= 140+ 60*skill_lv;
					break;
				case BA_MUSICALSTRIKE:
					skillratio+= 40*skill_lv -40;
					break;
				case DC_THROWARROW:
					skillratio+= 50*skill_lv;
					break;
				case CH_TIGERFIST:
					skillratio+= 100*skill_lv-60;
					break;
				case CH_CHAINCRUSH:
					skillratio+= 300+ 100*skill_lv;
					break;
				case CH_PALMSTRIKE:
					skillratio+= 100+ 100*skill_lv;
					break;
				case LK_SPIRALPIERCE:
					skillratio+=50*skill_lv;
					flag.idef= flag.idef2= 1;
					if(tsd)
						tsd->canmove_tick = gettick() + 1000;
					else if(tmd)
						tmd->canmove_tick = gettick() + 1000;
					break;
				case LK_HEADCRUSH:
					skillratio+=40*skill_lv;
					break;
				case LK_JOINTBEAT:
					skillratio+= 10*skill_lv-50;
					break;
				case ASC_METEORASSAULT:
					skillratio+= 40*skill_lv-60;
					flag.cardfix = 0;
					break;
				case SN_SHARPSHOOTING:
					skillratio+= 100+50*skill_lv;
					break;
				case CG_ARROWVULCAN:
					skillratio+= 100+100*skill_lv;
					break;
				case AS_SPLASHER:
					skillratio+= 100+20*skill_lv;
					if (sd)
						skillratio+= 20*sd->skill_check(AS_POISONREACT);
					flag.cardfix = 0;
					break;
				case ASC_BREAKER:
					skillratio+= 100*skill_lv -100;
					break;
				case PA_SACRIFICE:
					skillratio+= 10*skill_lv -10;
					flag.idef = flag.idef2 = 1;
					ele_flag=1;
					break;
				case PA_SHIELDCHAIN:
					skillratio+= wd.div_*(100+30*skill_lv)-100;
					//div_flag=1;
					ele_flag=1;
					break;
				case WS_CARTTERMINATION:
					if(sd && sd->cart_max_weight > 0 && sd->cart_weight > 0)
						skillratio += sd->cart_weight / (10 * (16 - skill_lv)) - 100;
					else if (!sd)
						skillratio += config.max_cart_weight / (10 * (16 - skill_lv));
					flag.cardfix = 0;
					break;
			}
			if (ele_flag)
				s_ele=s_ele_=0;
			else if((skill=skill_get_pl(skill_num))>0) //Checking for the skill's element
				s_ele=s_ele_=skill;

			if (sd && sd->skillatk[0] == skill_num)
				//If we apply skillatk[] as ATK_RATE, it will also affect other skills,
				//unfortunately this way ignores a skill's constant modifiers...
				skillratio += sd->skillatk[1];

			//Double attack does not applies to left hand
			ATK_RATE2(skillratio, skillratio - (skill_num==TF_DOUBLE?100:0));

			//Constant/misc additions from skills
			if (skill_num == MO_EXTREMITYFIST)
				ATK_ADD(250 + 150*skill_lv);

			if (sd)
			{
				short index= 0;
				switch (skill_num)
				{
					case	PA_SACRIFICE:
						sd->heal(-wd.damage, 0);//Do we really always use wd.damage here?
						//clif_skill_nodamage(*src,*target,skill_num,skill_lv,1);  // this doesn't show effect either.. hmm =/
						if( --src->get_statusvalue2(SC_SACRIFICE).integer() <= 0 )
							status_change_end(src, SC_SACRIFICE,-1);
						break;
					case CR_SHIELDBOOMERANG:
					case PA_SHIELDCHAIN:
						if ((index = sd->equip_index[8]) < MAX_INVENTORY &&
							sd->inventory_data[index] &&
							sd->inventory_data[index]->type == 5)
						{
							ATK_ADD(sd->inventory_data[index]->weight/10);
							ATK_ADD(sd->status.inventory[index].refine * status_getrefinebonus(0,1));
						}
						break;
					case LK_SPIRALPIERCE:
						if ((index = sd->equip_index[9]) < MAX_INVENTORY &&
							sd->inventory_data[index] &&
							sd->inventory_data[index]->type == 4)
						{
							ATK_ADD((int)(double)(sd->inventory_data[index]->weight*(0.8*skill_lv*4/10)));
							ATK_ADD(sd->status.inventory[index].refine * status_getrefinebonus(0,1));
						}
						break;
				}	//switch
			}	//if (sd)
		}

		if(sd)
		{
			if (skill_num != PA_SACRIFICE && skill_num != MO_INVESTIGATE && !flag.cri && !flag.infdef)
			{	//Elemental/Racial adjustments
				char raceele_flag=0, raceele_flag_=0;
				if(sd->right_weapon.def_ratio_atk_ele & (1<<t_ele) ||
					sd->right_weapon.def_ratio_atk_race & (1<<t_race) ||
					sd->right_weapon.def_ratio_atk_race & (target->is_boss()?1<<10:1<<11)
					)
					raceele_flag = flag.idef = 1;
				
				if(sd->left_weapon.def_ratio_atk_ele & (1<<t_ele) ||
					sd->left_weapon.def_ratio_atk_race & (1<<t_race) ||
					sd->left_weapon.def_ratio_atk_race & (target->is_boss()?1<<10:1<<11)
					)
					raceele_flag_ = flag.idef2 = 1;
				
				if (raceele_flag || raceele_flag_)
					ATK_RATE2(raceele_flag?(def1 + def2):100, raceele_flag_?(def1 + def2):100);
			}

			//Ignore Defense?
			if (!flag.idef && (
				(tmd && sd->right_weapon.ignore_def_mob & (target->is_boss()?2:1)) ||
				sd->right_weapon.ignore_def_ele & (1<<t_ele) ||
				sd->right_weapon.ignore_def_race & (1<<t_race) ||
				sd->right_weapon.ignore_def_race & (target->is_boss()?1<<10:1<<11)
				))
				flag.idef = 1;

			if (!flag.idef2 && (
				(tmd && sd->left_weapon.ignore_def_mob & (target->is_boss()?2:1)) ||
				sd->left_weapon.ignore_def_ele & (1<<t_ele) ||
				sd->left_weapon.ignore_def_race & (1<<t_race) ||
				sd->left_weapon.ignore_def_race & (target->is_boss()?1<<10:1<<11)
				))
				flag.idef2 = 1;
		}

		if (!flag.infdef && (!flag.idef || !flag.idef2))
		{	//Defense reduction
			int t_vit = target->get_vit();
			int vit_def;

			if(config.vit_penalty_type)
			{
				unsigned char target_count; //256 max targets should be a sane max
				target_count = 1 + battle_counttargeted(*target,src,config.vit_penalty_count_lv);
				if(target_count >= config.vit_penalty_count) {
					if(config.vit_penalty_type == 1) {
						def1 = (def1 * (100 - (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num))/100;
						def2 = (def2 * (100 - (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num))/100;
						t_vit = (t_vit * (100 - (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num))/100;
					} else { //Assume type 2
						def1 -= (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num;
						def2 -= (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num;
						t_vit -= (target_count - (config.vit_penalty_count - 1))*config.vit_penalty_num;
					}
				}
				if(def1 < 0) def1 = 0;
				if(def2 < 1) def2 = 1;
				if(t_vit < 1) t_vit = 1;
			}
			//Vitality reduction from rodatazone: http://rodatazone.simgaming.net/mechanics/substats.php#def	
			if (tsd)	//Sd vit-eq
			{	//[VIT*0.5] + rnd([VIT*0.3], max([VIT*0.3],[VIT^2/150]-1))
				vit_def = t_vit*(t_vit-15)/150;
				vit_def = t_vit/2 + (vit_def>0?rand()%vit_def:0);
				
				if( (src->is_undead() || s_race==6) &&
				(skill=tsd->skill_check(AL_DP)) >0)
					vit_def += skill*(3 +(tsd->status.base_level+1)/25);   // submitted by orn
			} else { //Mob-Pet vit-eq
				//VIT + rnd(0,[VIT/20]^2-1)
				vit_def = (t_vit/20)*(t_vit/20);
				vit_def = t_vit + (vit_def>0?rand()%vit_def:0);
			}
			
			if(config.player_defense_type)
				vit_def += def1*config.player_defense_type;
			else
				ATK_RATE2(flag.idef?100:100-def1, flag.idef2?100:100-def1);
			ATK_ADD2(flag.idef?0:-vit_def, flag.idef2?0:-vit_def);
		}
		//Post skill/vit reduction damage increases
		{	//SC skill damages
			if(src->has_status(SC_AURABLADE)) 
				ATK_ADD(20*src->get_statusvalue1(SC_AURABLADE).integer());
		}

		if (sd && skill_num != MO_INVESTIGATE && skill_num != MO_EXTREMITYFIST)
		{	//refine bonus
			ATK_ADD2(status_get_atk2(src), status_get_atk_2(src));
			//Items forged from the Top 10 most famous BS's get 10 dmg bonus
			ATK_ADD2(sd->right_weapon.fameflag*10, sd->left_weapon.fameflag*10);
		}

		//Set to min of 1
		if (flag.rh && wd.damage < 1) wd.damage = 1;
		if (flag.lh && wd.damage2 < 1) wd.damage2 = 1;

		if( sd && skill_num != MO_INVESTIGATE && skill_num != MO_EXTREMITYFIST &&
			skill_num != CR_GRANDCROSS && skill_num != NPC_GRANDDARKNESS)
		{	//Add mastery damage
			wd.damage = battle_addmastery(sd,target,wd.damage,0);
			if (flag.lh) wd.damage2 = battle_addmastery(sd,target,wd.damage2,1);
		}
	} //Here ends flag.hit section, the rest of the function applies to both hitting and missing attacks

	if(sd && (skill=sd->skill_check(BS_WEAPONRESEARCH)) > 0)
		ATK_ADD(skill*2);

	if(skill_num==TF_POISON)
		ATK_ADD(15*skill_lv);

	if( (sd && (skill_num || !config.pc_attack_attr_none)) ||
		(md && (skill_num || !config.mob_attack_attr_none)) ||
		(pd && (skill_num || !config.pet_attack_attr_none)) )
	{	//Elemental attribute fix
		if( sd || !tsd || !config.mob_ghostring_fix || t_ele!=8 )
		{
			short t_element = status_get_element(target);
			int ratio=0, s_element=0, damage;
			if(src->has_status(SC_WATK_ELEMENT))
			{	//Part of the attack becomes elemental. [Skotlex]
				ratio = src->get_statusvalue1(SC_WATK_ELEMENT).integer();
				s_element = src->get_statusvalue2(SC_WATK_ELEMENT).integer();
			}	
			if (wd.damage > 0)
			{
				if (ratio)
				{
					damage = wd.damage;
					wd.damage = battle_attr_fix(damage*(100-ratio)/100,s_ele,t_element);
					wd.damage+= battle_attr_fix(damage*(ratio)/100,s_element,t_element);
				}
				else 
					wd.damage=battle_attr_fix(wd.damage,s_ele,t_element);
				if(skill_num==MC_CARTREVOLUTION) //Cart Revolution applies the element fix once more with neutral element
					wd.damage = battle_attr_fix(wd.damage,0,t_element);
			}
			if (flag.lh && wd.damage2 > 0)
			{
				if (ratio)
				{
					damage = wd.damage2;
					wd.damage2 = battle_attr_fix(damage*(100-ratio)/100,s_ele,t_element);
					wd.damage2+= battle_attr_fix(damage*ratio/100,s_element,t_element);
				}
				else
					wd.damage2 = battle_attr_fix(wd.damage2,s_ele_,t_element);
			}
		}
	}

	if ((!flag.rh || wd.damage == 0) && (!flag.lh || wd.damage2 == 0))
		flag.cardfix = 0;	//When the attack does no damage, avoid doing %bonuses

	if (sd)
	{
		ATK_ADD2(sd->right_weapon.star, sd->left_weapon.star);
		ATK_ADD(sd->spiritball*3);

		//Card Fix, sd side
		if (flag.cardfix)
		{
			short cardfix = 1000, cardfix_ = 1000;
			short t_class = target->get_class();
			short t_race2 = target->get_race2();	
			if(sd->state.arrow_atk)
			{
				cardfix=cardfix*(100+sd->right_weapon.addrace[t_race]+sd->arrow_addrace[t_race])/100;
				cardfix=cardfix*(100+sd->right_weapon.addele[t_ele]+sd->arrow_addele[t_ele])/100;
				cardfix=cardfix*(100+sd->right_weapon.addsize[t_size]+sd->arrow_addsize[t_size])/100;
				cardfix=cardfix*(100+sd->right_weapon.addrace2[t_race2])/100;
				cardfix=cardfix*(100+sd->right_weapon.addrace[target->is_boss()?10:11]+sd->arrow_addrace[t_mode & 0x20?10:11])/100;
			} else {	//Melee attack
				if(!config.left_cardfix_to_right)
				{
					cardfix=cardfix*(100+sd->right_weapon.addrace[t_race])/100;
					cardfix=cardfix*(100+sd->right_weapon.addele[t_ele])/100;
					cardfix=cardfix*(100+sd->right_weapon.addsize[t_size])/100;
					cardfix=cardfix*(100+sd->right_weapon.addrace2[t_race2])/100;
					cardfix=cardfix*(100+sd->right_weapon.addrace[target->is_boss()?10:11])/100;

					if (flag.lh)
					{
						cardfix_=cardfix_*(100+sd->left_weapon.addrace[t_race])/100;
						cardfix_=cardfix_*(100+sd->left_weapon.addele[t_ele])/100;
						cardfix_=cardfix_*(100+sd->left_weapon.addsize[t_size])/100;
						cardfix_=cardfix_*(100+sd->left_weapon.addrace2[t_race2])/100;
						cardfix_=cardfix_*(100+sd->left_weapon.addrace[target->is_boss()?10:11])/100;
					}
				} else {
					cardfix=cardfix*(100+sd->right_weapon.addrace[t_race]+sd->left_weapon.addrace[t_race])/100;
					cardfix=cardfix*(100+sd->right_weapon.addele[t_ele]+sd->left_weapon.addele[t_ele])/100;
					cardfix=cardfix*(100+sd->right_weapon.addsize[t_size]+sd->left_weapon.addsize[t_size])/100;
					cardfix=cardfix*(100+sd->right_weapon.addrace2[t_race2]+sd->left_weapon.addrace2[t_race2])/100;
					cardfix=cardfix*(100+sd->right_weapon.addrace[target->is_boss()?10:11]+sd->left_weapon.addrace[t_mode & 0x20?10:11])/100;
				}
			}

			for(i=0;i<sd->right_weapon.add_damage_class_count; ++i) {
				if(sd->right_weapon.add_damage_classid[i] == t_class) {
					cardfix=cardfix*(100+sd->right_weapon.add_damage_classrate[i])/100;
					break;
				}
			}

			if (flag.lh)
			{
				for(i=0;i<sd->left_weapon.add_damage_class_count; ++i) {
					if(sd->left_weapon.add_damage_classid[i] == t_class) {
						cardfix_=cardfix_*(100+sd->left_weapon.add_damage_classrate[i])/100;
						break;
					}
				}
			}

			if (cardfix != 1000 || cardfix_ != 1000)
				ATK_RATE2(cardfix/10, cardfix_/10);	//What happens if you use right-to-left and there's no right weapon, only left?
		}
	} //if (sd)

	//Card Fix, tsd side
	if (tsd && flag.cardfix) {
		short s_size,s_race2,s_class;
		short cardfix=1000;
		
		s_size = src->get_size();
		s_race2 = src->get_race2();
		s_class = src->get_class();
		
		cardfix=cardfix*(100-tsd->subrace[s_race])/100;
		cardfix=cardfix*(100-tsd->subele[s_ele])/100;
		cardfix=cardfix*(100-tsd->subsize[s_size])/100;
 		cardfix=cardfix*(100-tsd->subrace2[s_race2])/100;
		cardfix=cardfix*(100-tsd->subrace[target->is_boss()?10:11])/100;
		
		for(i=0;i<tsd->add_damage_class_count2; ++i) {
				if(tsd->add_damage_classid2[i] == s_class) {
					cardfix=cardfix*(100+tsd->add_damage_classrate2[i])/100;
					break;
				}
			}
	
		if(wd.flag&BF_SHORT)
			cardfix=cardfix*(100-tsd->near_attack_def_rate)/100;
		else	// BF_LONG (there's no other choice)
			cardfix=cardfix*(100-tsd->long_attack_def_rate)/100;

		if (cardfix != 1000)
			ATK_RATE(cardfix/10);
	}
	
	//SC_data fixes
	{
		if( src->has_status(SC_FOGWALL) && wd.flag&BF_LONG )
			ATK_RATE(50);
	}
	
	{
		short scfix=1000;
		if(target->has_status(SC_DEFENDER) && wd.flag&BF_LONG)
			scfix=scfix*(100-target->get_statusvalue2(SC_DEFENDER).integer())/100;
		if(target->has_status(SC_FOGWALL) && wd.flag&BF_LONG)
			scfix=scfix/2;
		if(target->has_status(SC_ASSUMPTIO))
		{
			if(!maps[target->m].flag.pvp)
				scfix=scfix*2/3;
			else
				scfix=scfix/2;
		}
		if(scfix != 1000)
			ATK_RATE(scfix/10);
	}
	
	if(t_mode&0x40)
	{ //Plants receive 1 damage when hit
		if (flag.rh && (flag.hit || wd.damage>0))
			wd.damage = 1;
		if (flag.lh && (flag.hit || wd.damage2>0))
			wd.damage2 = 1;
		return wd;
	}
	
	if(!flag.rh || wd.damage<1)
		wd.damage=0;
	
	if(!flag.lh || wd.damage2<1)
		wd.damage2=0;

	//Double is basicly a normal attack x2, so... [Skotlex]
	if (skill_num == TF_DOUBLE)
		wd.damage *=2;

	if (sd)
	{
		if (!flag.rh && flag.lh) 
		{	//Move lh damage to the rh
			wd.damage = wd.damage2;
			wd.damage2 = 0;
			flag.rh=1;
			flag.lh=0;
		}
		else if(sd->status.weapon > 16)
		{	//Dual-wield
			if (wd.damage > 0)
			{
				skill = sd->skill_check(AS_RIGHT);
				wd.damage = wd.damage * (50 + (skill * 10))/100;
				if(wd.damage < 1) wd.damage = 1;
			}
			if (wd.damage2 > 0)
			{
				skill = sd->skill_check(AS_LEFT);
				wd.damage2 = wd.damage2 * (30 + (skill * 10))/100;
				if(wd.damage2 < 1) wd.damage2 = 1;
			}
		}
		else if(sd->status.weapon == 16)
		{ //Katars
			skill = sd->skill_check(TF_DOUBLE);
			wd.damage2 = wd.damage * (1 + (skill * 2))/100;

			if(wd.damage > 0 && wd.damage2 < 1) wd.damage2 = 1;
			flag.lh = 1;
		}
	}

	if( skill_num!=CR_GRANDCROSS && skill_num!=NPC_GRANDDARKNESS && (wd.damage > 0 || wd.damage2 > 0) )
	{
		if(wd.damage2<1)
			wd.damage=battle_calc_damage(src,target,wd.damage,wd.div_,skill_num,skill_lv,wd.flag);
		else if(wd.damage<1)
			wd.damage2=battle_calc_damage(src,target,wd.damage2,wd.div_,skill_num,skill_lv,wd.flag);
		else
		{
			int d1=wd.damage+wd.damage2,d2=wd.damage2;
			wd.damage=battle_calc_damage(src,target,d1,wd.div_,skill_num,skill_lv,wd.flag);
			wd.damage2=d1?((d2*100/d1)*wd.damage/100):0;
			if(wd.damage > 1 && wd.damage2 < 1) wd.damage2=1;
			wd.damage-=wd.damage2;
		}
	}
	if(sd && sd->classchange && tmd && (rand()%10000 < sd->classchange) && !(t_mode&0x20))
	{	//Classchange:
		static int changeclass[]={
			1001,1002,1004,1005,1007,1008,1009,1010,1011,1012,1013,1014,1015,1016,1018,1019,1020,
			1021,1023,1024,1025,1026,1028,1029,1030,1031,1032,1033,1034,1035,1036,1037,1040,1041,
			1042,1044,1045,1047,1048,1049,1050,1051,1052,1053,1054,1055,1056,1057,1058,1060,1061,
			1062,1063,1064,1065,1066,1067,1068,1069,1070,1071,1076,1077,1078,1079,1080,1081,1083,
			1084,1085,1094,1095,1097,1099,1100,1101,1102,1103,1104,1105,1106,1107,1108,1109,1110,
			1111,1113,1114,1116,1117,1118,1119,1121,1122,1123,1124,1125,1126,1127,1128,1129,1130,
			1131,1132,1133,1134,1135,1138,1139,1140,1141,1142,1143,1144,1145,1146,1148,1149,1151,
			1152,1153,1154,1155,1156,1158,1160,1161,1163,1164,1165,1166,1167,1169,1170,1174,1175,
			1176,1177,1178,1179,1180,1182,1183,1184,1185,1188,1189,1191,1192,1193,1194,1195,1196,
			1197,1199,1200,1201,1202,1204,1205,1206,1207,1208,1209,1211,1212,1213,1214,1215,1216,
			1219,1242,1243,1245,1246,1247,1248,1249,1250,1253,1254,1255,1256,1257,1258,1260,1261,
			1263,1264,1265,1266,1267,1269,1270,1271,1273,1274,1275,1276,1277,1278,1280,1281,1282,
			1291,1292,1293,1294,1295,1297,1298,1300,1301,1302,1304,1305,1306,1308,1309,1310,1311,
			1313,1314,1315,1316,1317,1318,1319,1320,1321,1322,1323,1364,1365,1366,1367,1368,1369,
			1370,1371,1372,1374,1375,1376,1377,1378,1379,1380,1381,1382,1383,1384,1385,1386,1387,
			1390,1391,1392,1400,1401,1402,1403,1404,1405,1406,1408,1409,1410,1412,1413,1415,1416,
			1417,1493,1494,1495,1497,1498,1499,1500,1502,1503,1504,1505,1506,1507,1508,1509,1510,
			1511,1512,1513,1514,1515,1516,1517,1519,1520,1582,1584,1585,1586,1587 };
		mob_class_change(*tmd, changeclass, sizeof(changeclass)/sizeof(changeclass[0]));
	}
	return wd;
}
/*==========================================
 * 武器ダメージ計算
 *------------------------------------------
 */
struct Damage battle_calc_weapon_attack(block_list *src,block_list *target,int skill_num,int skill_lv,int wflag)
{
	struct Damage wd;

	//return前の処理があるので情報出力部のみ変更
	if (src == NULL || target == NULL  || (src->m != target->m) )
	{
		nullpo_info(NLP_MARK);
		memset(&wd,0,sizeof(wd));
		return wd;
	}

	if( *target == BL_PET)
		memset(&wd,0,sizeof(wd));
	else if( *src == BL_PC )
		wd = battle_calc_pc_weapon_attack(src->get_sd(),target,skill_num,skill_lv,wflag);
	else if( *src == BL_MOB )
		wd = battle_calc_mob_weapon_attack(src,target,skill_num,skill_lv,wflag);
	else if( *src == BL_PET )
		wd = battle_calc_pet_weapon_attack(src,target,skill_num,skill_lv,wflag);
	else
		memset(&wd,0,sizeof(wd));

	if( *src==BL_PC && (wd.damage > 0 || wd.damage2 > 0) &&
		( config.equip_self_break_rate || config.equip_skill_break_rate ) )
	{
		map_session_data *sd = src->get_sd();

		if (config.equip_self_break_rate && sd->status.weapon != 11)
		{	//Self weapon breaking chance (Bows exempted)
			int breakrate = config.equip_natural_break_rate;	//default self weapon breaking chance [DracoRPG]
				if(sd->has_status(SC_OVERTHRUST) )
					breakrate += 10;
				if(sd->has_status(SC_MAXOVERTHRUST) )
					breakrate += 10;				

			if((size_t)rand() % 10000 < breakrate * config.equip_self_break_rate / 100 || breakrate >= 10000)
			{
				if (pc_breakweapon(*sd))
				{
					wd = battle_calc_pc_weapon_attack(sd,target,skill_num,skill_lv,wflag);
				}
			}
		}
		if (config.equip_skill_break_rate)
		{	//Target equipment breaking
			// weapon = 0, armor = 1
			int breakrate_[2] = {0,0};	//target breaking chance [celest]
			int breaktime = 5000;
			map_session_data *tsd = target->get_sd();

			breakrate_[0] += sd->break_weapon_rate;
			breakrate_[1] += sd->break_armor_rate;

			if( sd->has_status(SC_MELTDOWN) )
			{
				breakrate_[0] += 100*sd->get_statusvalue1(SC_MELTDOWN).integer();
				breakrate_[1] = 70*sd->get_statusvalue1(SC_MELTDOWN).integer();
				breaktime = skill_get_time2(WS_MELTDOWN,1);
			}
			
			if((size_t)rand() % 10000 < breakrate_[0] * config.equip_skill_break_rate / 100 || breakrate_[0] >= 10000)
			{
				
				if( tsd )
				{
					if(tsd->status.weapon != 11)
						pc_breakweapon(*tsd);
				}
				else
					status_change_start(target,SC_STRIPWEAPON,1,75,0,0,breaktime,0);
			}
			if((size_t)rand() % 10000 < breakrate_[1] * config.equip_skill_break_rate/100 || breakrate_[1] >= 10000)
			{
				if (tsd)
				{
					pc_breakarmor(*tsd);
				}
				else
					status_change_start(target,SC_STRIPSHIELD,1,75,0,0,breaktime,0);
			}
		}
	}
	return wd;
}

/*==========================================
 * 魔法ダメージ計算
 *------------------------------------------
 */
struct Damage battle_calc_magic_attack(block_list *src, block_list *target,int skill_num,int skill_lv,int flag)
	{
	int mdef1, mdef2, matk1, matk2, damage = 0, div_ = 1, blewcount, rdamage = 0;
	int ele=0, race=7, size=1, race2=7, t_ele=0, t_race=7, t_mode = 0, cardfix, t_class, i;
	map_session_data *sd = NULL, *tsd = NULL;
	struct Damage dmg;
	int aflag;	
	int normalmagic_flag = 1;
	int matk_flag = 1;
	int no_cardfix = 0;
	int no_elefix = 0;

	//return前の処理があるので情報出力部のみ変更
	if( src == NULL || target == NULL )
	{
		nullpo_info(NLP_MARK);
		memset(&dmg,0,sizeof(dmg));
		return dmg;
	}
	if( *target == BL_PET )
	{
		memset(&dmg,0,sizeof(dmg));
		return dmg;
	}

	blewcount = skill_get_blewcount(skill_num,skill_lv);
	matk1=status_get_matk1(src);
	matk2=status_get_matk2(src);
	ele = skill_get_pl(skill_num);
	//Attack that takes weapon's element for magical attacks? Make it neutral (applies to ASC_BREAKER at least) [Skotlex]
	if (ele == -1) ele = 0;
	race = src->get_race();
	size = src->get_size();
	race2 = src->get_race2();
	mdef1 = status_get_mdef(target);
	mdef2 = status_get_mdef2(target);
	t_ele = status_get_elem_type(target);
	t_race = target->get_race();
	t_mode = target->get_mode();

#define MATK_FIX( a,b ) { matk1=matk1*(a)/(b); matk2=matk2*(a)/(b); }

	sd=src->get_sd();
	if( sd )
	{
		sd->state.attack_type = BF_MAGIC;
		if(sd->matk_rate != 100)
			MATK_FIX(sd->matk_rate,100);
		sd->state.arrow_atk = 0;
	}

	tsd=target->get_sd();

	aflag=BF_MAGIC|BF_LONG|BF_SKILL;

	if(skill_num > 0)
	{
		switch(skill_num){	// 基本ダメージ計算(スキルごとに処理)
					// ヒールor聖体
		case AL_HEAL:
		case PR_BENEDICTIO:
			damage = skill_calc_heal(src,skill_lv)/2;
			normalmagic_flag=0;
			break;
		case PR_ASPERSIO:		// アスペルシオ 
			damage = 40; //固定ダメージ
			normalmagic_flag=0;
			break;
		case PR_SANCTUARY:	// サンクチュアリ
			damage = (skill_lv>6)?388:skill_lv*50;
			normalmagic_flag=0;
			blewcount|=0x10000;
			break;
		case ALL_RESURRECTION:
		case PR_TURNUNDEAD:	// 攻撃リザレクションとターンアンデッド
			if( *target != BL_PC && target->is_undead() )
			{
				int hp, mhp, thres;
				hp = target->get_hp();
				mhp = target->get_max_hp();
				thres = (skill_lv * 20) + src->get_luk()+
						src->get_int() + src->get_lv()+
						((200 - hp * 200 / mhp));
				if(thres > 700) thres = 700;
				if(rand()%1000 < thres && !(t_mode&0x20))	// 成功
					damage = hp;
				else					// 失敗
					damage = src->get_lv() + src->get_int() + skill_lv * 10;
			}
			normalmagic_flag=0;
			break;

		case MG_NAPALMBEAT:	// ナパームビート（分散計算込み）
			MATK_FIX(70+ skill_lv*10,100);
			if(flag>0){
				MATK_FIX(1,flag);
			}
			else
			{
				if(config.error_log)
					ShowMessage("battle_calc_magic_attack(): napam enemy count=0 !\n");
			}
			break;

		case MG_SOULSTRIKE:			// ソウルストライク （対アンデッドダメージ補正）
			if( target->is_undead() )
			{
				matk1 += matk1*skill_lv/20;//MATKに補正じゃ駄目ですかね？
				matk2 += matk2*skill_lv/20;
			}
			break;

		case MG_FIREBALL:	// ファイヤーボール
			{
				const int drate[]={100,90,70};
				if(flag>2)
					matk1=matk2=0;
				else
					MATK_FIX( (70+skill_lv*10)*drate[flag] ,10000 );
			}
			break;

		case MG_FIREWALL:	// ファイヤーウォール
			if((t_ele==3 || target->is_undead()) && !tsd)
			{	// not knocking back, but stopping
				blewcount  = 0x10000;
				target->stop_walking(1);
			}
			else
				blewcount |= 0x10000;
			//md.dmotion=0; //Firewall's delay is always none. [Skotlex]
			MATK_FIX( 1,2 );
			break;
		case MG_THUNDERSTORM:	// サンダーストーム
			MATK_FIX( 80,100 );
			break;
		case MG_FROSTDIVER:	// フロストダイバ
			MATK_FIX( 100+skill_lv*10, 100);
			break;
		case WZ_FROSTNOVA:	// フロストダイバ
			MATK_FIX((100+skill_lv*10)*2/3, 100);
			break;
		case WZ_FIREPILLAR:	// ファイヤーピラー
			if(mdef1 < 1000000)
				mdef1=mdef2=0;	// MDEF無視
			MATK_FIX( 1,5 );
			matk1+=50;
			matk2+=50;
			break;
		case WZ_SIGHTRASHER:
			MATK_FIX( 100+skill_lv*20, 100);
			break;
		case WZ_METEOR:
		case WZ_JUPITEL:	// ユピテルサンダー
			break;
		case WZ_VERMILION:	// ロードオブバーミリオン
			MATK_FIX( skill_lv*20+80, 100 );
			break;
		case WZ_WATERBALL:	// ウォーターボール
			MATK_FIX( 100+skill_lv*30, 100 );
			break;
		case WZ_STORMGUST:	// ストームガスト
			MATK_FIX( skill_lv*40+100 ,100 );
//			blewcount|=0x10000;
			break;
		case AL_HOLYLIGHT:	// ホーリーライト
			MATK_FIX( 125,100 );
			break;
		case AL_RUWACH:
			MATK_FIX( 145,100 );
			break;
		case HW_NAPALMVULCAN:	// ナパームビート（分散計算込み）
			MATK_FIX(70+ skill_lv*10,100);
			if(flag>0)
			{
				MATK_FIX(1,flag);
			}
			else
			{
				if(config.error_log)
					ShowMessage("battle_calc_magic_attack(): napalmvulcan enemy count=0 !\n");
			}
			break;
		case PF_SOULBURN: // Celest
			if( !tsd || skill_lv < 5)
			{
				memset(&dmg,0,sizeof(dmg));
				return dmg;
			}
			else if (tsd)
			{
				damage = tsd->status.sp * 2;
				matk_flag = 0; // don't consider matk and matk2
			}
			break;
		case ASC_BREAKER:
			damage = rand()%500 + 500 + skill_lv * src->get_int() * 5;
			matk_flag = 0; // don't consider matk and matk2
			break;
		case HW_GRAVITATION:
			damage = 200 + skill_lv * 200;
			normalmagic_flag = 0;
			no_cardfix = 1;
			no_elefix = 1;
			break;
		}
	}

	if(normalmagic_flag){	// 一般魔法ダメージ計算
		int imdef_flag=0;
		if (matk_flag)
		{
			if(matk1>matk2)
				damage= matk2+rand()%(matk1-matk2+1);
			else
				damage= matk2;
		}
		if(sd)
		{
			if(sd->ignore_mdef_ele & (1<<t_ele) || sd->ignore_mdef_race & (1<<t_race))
				imdef_flag = 1;
			if(target->is_boss()) {
				if(sd->ignore_mdef_race & (1<<10))
					imdef_flag = 1;
			}
			else {
				if(sd->ignore_mdef_race & (1<<11))
					imdef_flag = 1;
			}
		}
		if(!imdef_flag)
		{
			if(config.magic_defense_type)
			{
				damage = damage - (mdef1 * config.magic_defense_type) - mdef2;
			}
			else
			{
				damage = (damage*(100-mdef1))/100 - mdef2;
			}
		}

		if(damage<1)
			damage=1;
	}

	if (sd && !no_cardfix)
	{
		cardfix=100;
		cardfix=cardfix*(100+sd->magic_addrace[t_race])/100;
		cardfix=cardfix*(100+sd->magic_addele[t_ele])/100;
		if(target->is_boss())
			cardfix=cardfix*(100+sd->magic_addrace[10])/100;
		else
			cardfix=cardfix*(100+sd->magic_addrace[11])/100;
		t_class = target->get_class();
		for(i=0;i<sd->add_magic_damage_class_count; ++i)
		{
			if(sd->add_magic_damage_classid[i] == t_class)
			{
				cardfix=cardfix*(100+sd->add_magic_damage_classrate[i])/100;
				break;
			}
		}
		damage=damage*cardfix/100;
		if (skill_num > 0 && sd->skillatk[0] == skill_num)
			damage += damage*sd->skillatk[1]/100;
	}

	if (tsd && !no_cardfix)
	{
		int s_class = src->get_class();
		cardfix=100;
		cardfix=cardfix*(100-tsd->subele[ele])/100;	// 属 性によるダメージ耐性
		cardfix=cardfix*(100-tsd->subrace[race])/100;	// 種族によるダメージ耐性
		cardfix=cardfix*(100-tsd->subsize[size])/100;
		cardfix=cardfix*(100-tsd->magic_subrace[race])/100;
		cardfix=cardfix*(100-tsd->subrace2[race2])/100;	// 種族によるダメージ耐性
		if(src->is_boss())
			cardfix=cardfix*(100-tsd->magic_subrace[10])/100;
		else
			cardfix=cardfix*(100-tsd->magic_subrace[11])/100;
		for(i=0;i<tsd->add_mdef_class_count; ++i)
		{
			if(tsd->add_mdef_classid[i] == s_class)
			{
				cardfix=cardfix*(100-tsd->add_mdef_classrate[i])/100;
				break;
			}
		}
		cardfix=cardfix*(100-tsd->magic_def_rate)/100;
		damage=damage*cardfix/100;
	}
	if(damage < 0) damage = 0;

	if (!no_elefix)
		damage=battle_attr_fix(damage, ele, status_get_element(target) );		// 属 性修正

	if(skill_num == CR_GRANDCROSS || skill_num == NPC_GRANDDARKNESS)
	{	// グランドクロス
		struct Damage wd = battle_calc_weapon_attack(src,target,skill_num,skill_lv,flag);
		damage = (damage + wd.damage) * (100 + 40*skill_lv)/100;
		if(config.gx_dupele) damage=battle_attr_fix(damage, ele, status_get_element(target) );	//属性2回かかる
		if(src==target)
		{
			mob_data* md=src->get_md();
			if(md)
				damage = 0;		//MOBが使う場合は反動無し
			else
				damage=damage/2;	//反動は半分
		}
	}

	div_=skill_get_num( skill_num,skill_lv );

	if(div_>1 && skill_num != WZ_VERMILION)
		damage*=div_;

//	if(mdef1 >= 1000000 && damage > 0)
	if(t_mode&0x40 && damage > 0)
		damage = 1;

	if(target->is_boss())
		blewcount &= 0x10000;

	if (tsd && status_isimmune(target))
	{
		if (sd && config.gtb_pvp_only != 0)
		{	// [MouseJstr]
			damage = (damage * (100 - config.gtb_pvp_only)) / 100;
		}
		else damage = 0;	// 黄 金蟲カード（魔法ダメージ０）
	}

	damage=battle_calc_damage(src,target,damage,div_,skill_num,skill_lv,aflag);	// 最終修正

	if( tsd && tsd->magic_damage_return > 0 )
	{
		rdamage += damage * tsd->magic_damage_return / 100;
		if(rdamage < 1) rdamage = 1;
		clif_damage(*target,*src,gettick(),0,0,rdamage,0,0,0);
		battle_damage(target,src,rdamage,0);
	}


	dmg.damage=damage;
	dmg.div_=div_;
	dmg.amotion=status_get_amotion(src);
	dmg.dmotion=status_get_dmotion(target);
	dmg.damage2=0;
	dmg.type=0;
	dmg.blewcount=blewcount;
	dmg.flag=aflag;

	return dmg;
}

/*==========================================
 * その他ダメージ計算
 *------------------------------------------
 */
struct Damage battle_calc_misc_attack(block_list *src,block_list *target,int skill_num,int skill_lv,int flag)
{
	struct Damage md;
	//return前の処理があるので情報出力部のみ変更
	if( src == NULL || target == NULL ){
		nullpo_info(NLP_MARK);
		memset(&md,0,sizeof(md));
		return md;
	}

	int int_=src->get_int();
//	int luk=src->get_luk();
	int dex=src->get_dex();
	int skill,ele,race,size,cardfix,race2,t_mode;
	map_session_data *sd=NULL,*tsd=NULL;
	int damage=0,div_=1,blewcount=skill_get_blewcount(skill_num,skill_lv);
	int damagefix=1;
	int self_damage=0;
	int aflag=BF_MISC|BF_SHORT|BF_SKILL;


	if( *target == BL_PET )
	{
		memset(&md,0,sizeof(md));
		return md;
	}

	sd=src->get_sd();
	if( sd )
	{
		sd->state.attack_type = BF_MISC;
		sd->state.arrow_atk = 0;
	}

	tsd=target->get_sd();

	t_mode = target->get_mode();
	ele = skill_get_pl(skill_num);
	//Attack that takes weapon's element for magical attacks? Make it neutral (applies to ASC_BREAKER at least) [Skotlex]
	if (ele < 0) ele = 0;
	race = src->get_race();
	size = src->get_size();
	race2 = src->get_race2();

	switch(skill_num){

	case HT_LANDMINE:	// ランドマイン
		damage=skill_lv*(dex+75)*(100+int_)/100;
		break;

	case HT_BLASTMINE:	// ブラストマイン
		damage=skill_lv*(dex/2+50)*(100+int_)/100;
		break;

	case HT_CLAYMORETRAP:	// クレイモアートラップ
		damage=skill_lv*(dex/2+75)*(100+int_)/100;
		break;

	case HT_BLITZBEAT:	// ブリッツビート
		if( sd==NULL || (skill = sd->skill_check(HT_STEELCROW)) <= 0)
			skill=0;
		damage=(dex/10+int_/2+skill*3+40)*2;
		if(flag > 1)
			damage /= flag;
		aflag |= (flag&~BF_RANGEMASK)|BF_LONG;
		break;

	case TF_THROWSTONE:	// 石投げ
		damage=50;
		aflag |= (flag&~BF_RANGEMASK)|BF_LONG;
		break;

	case BA_DISSONANCE:	// 不協和音
		if(sd)
		damage=30+(skill_lv)*10+sd->skill_check(BA_MUSICALLESSON)*3;
		break;

	case NPC_SELFDESTRUCTION:	// 自爆
		damage = src->get_hp();
		damagefix = 0;
		break;

	case NPC_SMOKING:	// タバコを吸う
		damage=3;
		damagefix=0;
		break;

	case NPC_DARKBREATH:
		{
			int hitrate=status_get_hit(src) - status_get_flee(target) + 80;
			hitrate = ( (hitrate>95)?95: ((hitrate<5)?5:hitrate) );
			if( (target->has_status(SC_SLEEP) || target->has_status(SC_STUN) ||
				target->has_status(SC_FREEZE) || (target->has_status(SC_STONE) && target->get_statusvalue2(SC_STONE).integer()==0) ) )
				hitrate = 1000000;
			if(rand()%100 < hitrate) {
				damage = 500 + (skill_lv-1)*1000 + rand()%1000;
				if(damage > 9999) damage = 9999;
			}
		}
		break;
	case PA_SACRIFICE:
	{	
		if( target->get_mexp() )
			self_damage = 1;
		else
			self_damage = src->get_max_hp()*9/100;
		ele = status_get_attack_element(src);
		damage = self_damage + (self_damage/10)*(skill_lv-1);
		if (maps[src->m].flag.gvg)
			damage = 6*damage/10; //40% less effective on siege maps. [Skotlex]
		break;
	}

	case SN_FALCONASSAULT:			// ファルコンアサルト 
	{
		if( sd==NULL || (skill = sd->skill_check(HT_STEELCROW)) <= 0)
			skill=0;
		damage=((150+70*skill_lv)*(dex/10+int_/2+skill*3+40)*2)/100; // [Celest]
		if(flag > 1)
			damage /= flag;
		aflag |= (flag&~BF_RANGEMASK)|BF_LONG;
		break;
	}
	case PA_PRESSURE:
		damage=500+300*skill_lv;
		damagefix=0;
		aflag |= (flag&~BF_RANGEMASK)|BF_LONG;
		break;

	case CR_ACIDDEMONSTRATION:
		//This equation is not official, but it's the closest to the official one 
		//that Viccious Pucca and the other folks at the forums could come up with. [Skotlex]
		damage = int_ * (int)sqrt((double)(100*target->get_vit()))/3;
		if (tsd) damage/=2;
		aflag |= (flag&~BF_RANGEMASK)|BF_LONG;
		break;
	}//end case

	if(damagefix){
		if(damage<1 && skill_num != NPC_DARKBREATH)
			damage=1;

		if( tsd ){
			cardfix=100;
			cardfix=cardfix*(100-tsd->subele[ele])/100;	// 属性によるダメージ耐性
			cardfix=cardfix*(100-tsd->subrace[race])/100;	// 種族によるダメージ耐性
			cardfix=cardfix*(100-tsd->subsize[size])/100;
			cardfix=cardfix*(100-tsd->misc_def_rate)/100;
			cardfix=cardfix*(100-tsd->subrace2[race2])/100;
			damage=damage*cardfix/100;
		}
		if (sd && skill_num > 0 && sd->skillatk[0] == skill_num)
			damage += damage*sd->skillatk[1]/100;

		if(damage < 0) damage = 0;
		damage=battle_attr_fix(damage, ele, status_get_element(target) );		// 属性修正
	}

	div_=skill_get_num( skill_num,skill_lv );
	if(div_>1)
		damage*=div_;

	if( damage>0 && (damage<div_ || t_mode&0x40))
		damage = div_;	//Again, Ishizu noted that MISC skills do div damage to plants.

	if(target->is_boss())
		blewcount = 0;

	if(self_damage)
	{
		if(sd)
			pc_damage(*sd, self_damage, src);
		clif_damage(*src,*src, gettick(), 0, 0, self_damage, 0 , 0, 0);
	}

	damage=battle_calc_damage(src,target,damage,div_,skill_num,skill_lv,aflag);	// 最終修正

	md.damage=damage;
	md.div_=div_;
	md.amotion=status_get_amotion(src);
	md.dmotion=status_get_dmotion(target);
	md.damage2=0;
	md.type=0;
	md.blewcount=blewcount;
	md.flag=aflag;
	return md;

}
/*==========================================
 * ダメージ計算一括処理用
 *------------------------------------------
 */
struct Damage battle_calc_attack(int attack_type, block_list *bl,block_list *target, int skill_num,int skill_lv,int flag)
{
	struct Damage d;
	switch(attack_type){
	case BF_WEAPON:
		return battle_calc_weapon_attack(bl,target,skill_num,skill_lv,flag);
	case BF_MAGIC:
		return battle_calc_magic_attack(bl,target,skill_num,skill_lv,flag);
	case BF_MISC:
		return battle_calc_misc_attack(bl,target,skill_num,skill_lv,flag);
	default:
		if (config.error_log)
			ShowError("battle_calc_attack: unknown attack type! %d\n",attack_type);
		memset(&d,0,sizeof(d));
		break;
	}
	return d;
}
/*==========================================
 * 通常攻撃処理まとめ
 *------------------------------------------
 */
int battle_weapon_attack(block_list *src, block_list *target, unsigned long tick, int flag)
{
	nullpo_retr(0, src);
	nullpo_retr(0, target);


	map_session_data *sd = src->get_sd();
	map_session_data *tsd = target->get_sd();

	int race, ele, damage, rdamage = 0;
	struct Damage wd;
	short *opt1;


	
	if( !src->is_on_map() || !target->is_on_map() )
		return 0;

	if(src->is_dead() )
		return 0;

	if(target->is_dead() )
		return 0;

	opt1 = status_get_opt1(src);
	if (opt1 && *opt1 > 0) {
		src->stop_attack();
		return 0;
	}
	if( src->has_status(SC_BLADESTOP) )
	{
		src->stop_attack();
		return 0;
	}

	if (battle_check_target(src,target,BCT_ENEMY) <= 0 && !battle_check_range(src,target,0))
		return 0;	// 攻撃対象外

	race = target->get_race();
	ele = status_get_elem_type(target);
	if( battle_check_target(src,target,BCT_ENEMY) > 0 && battle_check_range(src,target,0) )
	{	// 攻撃対象となりうるので攻撃
		if(sd && sd->status.weapon == 11)
		{
			if(sd->equip_index[10] < MAX_INVENTORY)
			{
				if(config.arrow_decrement)
					pc_delitem(*sd,sd->equip_index[10],1,0);
			}
			else
			{
				clif_arrow_fail(*sd,0);
				return 0;
			}
		}
		if(flag&0x8000)
		{
			struct mob_data *md;
			if(sd && config.pc_attack_direction_change)
				sd->dir = sd->head_dir = src->get_direction(*target);
			else if( config.monster_attack_direction_change && (md = src->get_md()) )
				md->dir = src->get_direction(*target);

			wd = battle_calc_weapon_attack(src, target, KN_AUTOCOUNTER, flag&0xff, 0);
		}
		else if (flag & AS_POISONREACT && src->has_status(SC_POISONREACT))
			wd = battle_calc_weapon_attack(src, target, AS_POISONREACT, src->get_statusvalue1(SC_POISONREACT).integer(), 0);
		else
			wd = battle_calc_weapon_attack(src,target,0,0,0);
	
		if((damage = wd.damage + wd.damage2) > 0 && src != target)
		{
			if(wd.flag & BF_SHORT)
			{
				if(tsd && tsd->short_weapon_damage_return > 0)
				{
					rdamage += damage * tsd->short_weapon_damage_return / 100;
					if (rdamage < 1) rdamage = 1;
				}
				if( target->has_status(SC_REFLECTSHIELD) )
				{
					rdamage += damage * target->get_statusvalue2(SC_REFLECTSHIELD).integer() / 100;
					if (rdamage < 1) rdamage = 1;
				}
			}
			else if(wd.flag & BF_LONG)
			{
				if(tsd && tsd->long_weapon_damage_return > 0)
				{
					rdamage += damage * tsd->long_weapon_damage_return / 100;
					if(rdamage < 1) rdamage = 1;
				}
			}
			if (rdamage > 0)
				clif_damage(*src, *src, tick, wd.amotion, wd.dmotion, rdamage, 1, 4, 0);
		}

		if(wd.div_ == 255)
		{	//三段掌
			int delay = 0;
			wd.div_ = 3;
			if(sd && wd.damage+wd.damage2 < target->get_hp())
			{
				int skilllv = sd->skill_check( MO_CHAINCOMBO);
				if (skilllv > 0) {
					delay = 1000 - 4 * src->get_agi() - 2 *  src->get_dex();
					delay += 300 * config.combo_delay_rate / 100; //追加ディレイをconfにより調整
				}
				status_change_start(src, SC_COMBO, MO_TRIPLEATTACK, skilllv, 0, 0, delay, 0);
			}
			if(sd) sd->attackable_tick = sd->canmove_tick = tick + delay;

			clif_combo_delay(*src, delay);
			clif_skill_damage(*src, *target, tick, wd.amotion, wd.dmotion, wd.damage, wd.div_,
				MO_TRIPLEATTACK, (sd)?sd->skill_check(MO_TRIPLEATTACK):1, -1);
		}
		else
		{	//二刀流左手とカタール追撃のミス表示(無理やり〜)
			clif_damage(*src, *target, tick, wd.amotion, wd.dmotion, wd.damage, wd.div_ , wd.type, wd.damage2);
			if(sd && sd->status.weapon >= 16 && wd.damage2 == 0)
				clif_damage(*src, *target, tick+10, wd.amotion, wd.dmotion,0, 1, 0, 0);
		}
		if (sd && sd->splash_range > 0 && (wd.damage > 0 || wd.damage2 > 0))
			skill_castend_damage_id(src,target,0,0,tick,0);

		block_list::freeblock_lock();

		battle_delay_damage(tick+wd.amotion, *src, *target, (wd.damage+wd.damage2), 0);

		if(target->is_on_map() && (wd.damage > 0 || wd.damage2 > 0))
		{
			skill_additional_effect(src, target, 0, 0, BF_WEAPON, tick);
			if(sd)
			{
				int hp = target->get_max_hp();
				if (sd->weapon_coma_ele[ele] > 0 && rand()%10000 < sd->weapon_coma_ele[ele])
					battle_damage(src, target, hp, 1);
				if (sd->weapon_coma_race[race] > 0 && rand()%10000 < sd->weapon_coma_race[race])
					battle_damage(src, target, hp, 1);
				if (target->is_boss()) {
					if(sd->weapon_coma_race[10] > 0 && rand()%10000 < sd->weapon_coma_race[10])
						battle_damage(src, target, hp, 1);
				}
				else
				{
					if (sd->weapon_coma_race[11] > 0 && rand()%10000 < sd->weapon_coma_race[11])
						battle_damage(src, target, hp, 1);
				}
			}
		}
		if(src->has_status(SC_AUTOSPELL) && rand()%100 < src->get_statusvalue4(SC_AUTOSPELL).integer())
		{
			int sp = 0;
			int f = 0;
			int skillid = src->get_statusvalue2(SC_AUTOSPELL).integer();
			int skilllv = src->get_statusvalue3(SC_AUTOSPELL).integer();

			int i = rand()%100;
			if (i >= 50) skilllv -= 2;
			else if (i >= 15) skilllv--;
			if (skilllv < 1) skilllv = 1;

			if (sd) sp = skill_get_sp(skillid,skilllv) * 2 / 3;

			if((sd && sd->status.sp >= sp) || !sd)
			{
				if (skill_get_inf(skillid) & INF_GROUND_SKILL)
					f = skill_castend_pos2(src, target->x, target->y, skillid, skilllv, tick, flag);
				else {
					switch(skill_get_nk(skillid)) {
						case NK_NO_DAMAGE:// 支援系 
							if((skillid == AL_HEAL || (skillid == ALL_RESURRECTION && !tsd)) && target->is_undead())
								f = skill_castend_damage_id(src, target, skillid, skilllv, tick, flag);
							else
								f = skill_castend_nodamage_id(src, target, skillid, skilllv, tick, flag);
							break;
						case NK_SPLASH_DAMAGE:
						default:
							f = skill_castend_damage_id(src, target, skillid, skilllv, tick, flag);
							break;
					}
				}
				if(sd && !f)
				{
					sd->heal(0, -sp);
				}
			}
		}
		if(sd)
		{
			size_t i;
			for (i = 0; i < 10; ++i)
			{
				if(sd->autospell_id[i] != 0)
				{
					block_list *tbl;
					int skillid = (sd->autospell_id[i] > 0) ? sd->autospell_id[i] : -sd->autospell_id[i];
					int skilllv = (sd->autospell_lv[i] > 0) ? sd->autospell_lv[i] : 1;
					int rate = (!sd->state.arrow_atk) ? sd->autospell_rate[i] : sd->autospell_rate[i] / 2;
					
					if (rand()%100 > rate)
						continue;
					if (sd->autospell_id[i] < 0)
						tbl = src;
					else
						tbl = target;
					
					if (skill_get_inf(skillid) & INF_GROUND_SKILL)
						skill_castend_pos2(src, tbl->x, tbl->y, skillid, skilllv, tick, flag);
					else {
						switch (skill_get_nk(skillid))
						{
						case NK_NO_DAMAGE:// 支援系 
							if ((skillid == AL_HEAL || (skillid == ALL_RESURRECTION && !tsd)) && tbl->is_undead())
									skill_castend_damage_id(src, tbl, skillid, skilllv, tick, flag);
								else
									skill_castend_nodamage_id(src, tbl, skillid, skilllv, tick, flag);
								break;
							case NK_SPLASH_DAMAGE:
							default:
								skill_castend_damage_id(src, tbl, skillid, skilllv, tick, flag);
								break;

						}
					}
				} else break;
			}
			if(wd.flag&BF_WEAPON && src != target && (wd.damage > 0 || wd.damage2 > 0))
			{
				int hp = 0, sp = 0;
				if(!config.left_cardfix_to_right)
				{	// 二刀流左手カードの吸収系効果を右手に追加しない場合
					hp += battle_calc_drain(wd.damage, sd->right_weapon.hp_drain_rate, sd->right_weapon.hp_drain_per, sd->right_weapon.hp_drain_value);
					hp += battle_calc_drain(wd.damage2, sd->left_weapon.hp_drain_rate, sd->left_weapon.hp_drain_per, sd->left_weapon.hp_drain_value);
					sp += battle_calc_drain(wd.damage, sd->right_weapon.sp_drain_rate, sd->right_weapon.sp_drain_per, sd->right_weapon.sp_drain_value);
					sp += battle_calc_drain(wd.damage2, sd->left_weapon.sp_drain_rate, sd->left_weapon.sp_drain_per, sd->left_weapon.sp_drain_value);
				}
				else
				{	// 二刀流左手カードの吸収系効果を右手に追加する場合
					int hp_drain_rate = sd->right_weapon.hp_drain_rate + sd->left_weapon.hp_drain_rate;
					int hp_drain_per = sd->right_weapon.hp_drain_per + sd->left_weapon.hp_drain_per;
					int hp_drain_value = sd->right_weapon.hp_drain_value + sd->left_weapon.hp_drain_value;
					int sp_drain_rate = sd->right_weapon.sp_drain_rate + sd->left_weapon.sp_drain_rate;
					int sp_drain_per = sd->right_weapon.sp_drain_per + sd->left_weapon.sp_drain_per;
					int sp_drain_value = sd->right_weapon.sp_drain_value + sd->left_weapon.sp_drain_value;
					hp += battle_calc_drain(wd.damage, hp_drain_rate, hp_drain_per, hp_drain_value);
					sp += battle_calc_drain(wd.damage, sp_drain_rate, sp_drain_per, sp_drain_value);
				}

				if(hp || sp)
				{
					if( hp && hp>sd->status.max_hp-sd->status.hp )
						hp = sd->status.max_hp - sd->status.hp;
					if( sp && sp>sd->status.max_sp-sd->status.sp )
						sp = sd->status.max_sp - sd->status.sp;

					if (config.show_hp_sp_drain)
					{	//Display gained values [Skotlex]
						if( hp && (hp=sd->heal(hp, 0)) && hp>0 )
							clif_heal(sd->fd, SP_HP, hp);
						if( sp && sd->heal(0, sp) && sp>0 )
							clif_heal(sd->fd, SP_SP, sp);
					}
					else
					{
						sd->heal(hp, sp);
					}
				}

				if (tsd && sd->sp_drain_type)
					tsd->heal(0, -sp);
			}
		}
		if(tsd)
		{
			int i;
			for (i = 0; i < 10; ++i)
			{
				if(tsd->autospell2_id[i] != 0)
				{
					block_list *tbl;
					int skillid = (tsd->autospell2_id[i] > 0) ? tsd->autospell2_id[i] : -tsd->autospell2_id[i];
					int skilllv = (tsd->autospell2_lv[i] > 0) ? tsd->autospell2_lv[i] : 1;
					int rate = ((sd && !sd->state.arrow_atk) || (src->get_range()<=2)) ?
						tsd->autospell2_rate[i] : tsd->autospell2_rate[i] / 2;
					
					if (rand()%100 > rate)
						continue;
					if (tsd->autospell2_id[i] < 0)
						tbl = target;
					else 
						tbl = src;
					if (skill_get_inf(skillid) & INF_GROUND_SKILL)
						skill_castend_pos2(target, tbl->x, tbl->y, skillid, skilllv, tick, flag);
					else {
						switch (skill_get_nk(skillid)) {
							case NK_NO_DAMAGE:// 支援系 
								if ((skillid == AL_HEAL || (skillid == ALL_RESURRECTION && !sd)) && tbl->is_undead() )
									skill_castend_damage_id(target, tbl, skillid, skilllv, tick, flag);
								else
									skill_castend_nodamage_id(target, tbl, skillid, skilllv, tick, flag);
								break;
							case NK_SPLASH_DAMAGE:
							default:
								skill_castend_damage_id(target, tbl, skillid, skilllv, tick, flag);
								break;
						}
					}
				} else break;
			}
		}
		if (rdamage > 0)
			battle_delay_damage(tick+wd.amotion, *target, *src, rdamage, 0);


		if (src->has_status(SC_READYSTORM) && rand()%100 < 15) // Taekwon Strom Stance [Dralnu]
			status_change_start(src,SC_STORMKICK,1,0,0,0,0,0);
		if(src->has_status(SC_READYDOWN) && rand()%100 < 15) // Taekwon Axe Stance [Dralnu]
			status_change_start(src,SC_DOWNKICK,1,target->id,0,0,0,0);
		if(src->has_status(SC_READYTURN) && rand()%100 < 15) // Taekwon Round Stance [Dralnu]
			status_change_start(src,SC_TURNKICK,1,target->id,0,0,0,0);

		if(target->has_status(SC_AUTOCOUNTER) && target->get_statusvalue4(SC_AUTOCOUNTER).integer() > 0)
		{
			if( (uint32)target->get_statusvalue3(SC_AUTOCOUNTER).integer() == src->id )
				battle_weapon_attack(target, src, tick, 0x8000|target->get_statusvalue1(SC_AUTOCOUNTER).integer());
			clif_skillcastcancel(*target); //Remove the little casting bar. [Skotlex]
			status_change_end(target,SC_AUTOCOUNTER,-1);
		}
		if(target->has_status(SC_READYCOUNTER) && rand()%100 < 20) // Taekwon Counter Stance [Dralnu]
			status_change_start(target,SC_COUNTER,1,src->id,0,0,tick,flag);
		if(target->has_status(SC_POISONREACT) && target->get_statusvalue4(SC_POISONREACT).integer() > 0 && (uint32)target->get_statusvalue3(SC_POISONREACT).integer() == src->id)
		{   // poison react [Celest]
			if(status_get_elem_type(src) == 5)
			{
				target->get_statusvalue2(SC_POISONREACT).integer() = 0;
				battle_weapon_attack(target, src, tick, flag|AS_POISONREACT);
			}
			else
			{
				skill_castend_damage_id(target, src, TF_POISON, 5, tick, flag);
				--target->get_statusvalue2(SC_POISONREACT).integer();
			}
			if( target->get_statusvalue2(SC_POISONREACT).integer() <= 0 )
				status_change_end(target,SC_POISONREACT,-1);
		}
		if( target->has_status(SC_BLADESTOP_WAIT) && !src->is_boss() )
		{	// ボスには無効
			int skilllv = target->get_statusvalue1(SC_BLADESTOP_WAIT).integer();
			int duration = skill_get_time2(MO_BLADESTOP,skilllv);
			status_change_end(target, SC_BLADESTOP_WAIT, -1);
			status_change_start(target, SC_BLADESTOP, skilllv, 2, basics::numptr(target), basics::numptr(src), duration, 0);
			skilllv = sd?sd->skill_check( MO_BLADESTOP):1;
			status_change_start(src, SC_BLADESTOP, skilllv, 1, basics::numptr(src), basics::numptr(target), duration, 0);
		}
		if( target->has_status(SC_SPLASHER) )	//殴ったので対象のベナムスプラッシャー状態を解除
			status_change_end(target, SC_SPLASHER, -1);

		block_list::freeblock_unlock();
	}
	return wd.dmg_lv;
}


/*==========================================
 * 敵味方判定(1=肯定,0=否定,-1=エラー)
 * flag&0xf0000 = 0x00000:敵じゃないか判定（ret:1＝敵ではない）
 *				= 0x10000:パーティー判定（ret:1=パーティーメンバ)
 *				= 0x20000:全て(ret:1=敵味方両方)
 *				= 0x40000:敵か判定(ret:1=敵)
 *				= 0x50000:パーティーじゃないか判定(ret:1=パーティでない)
 *------------------------------------------
 
 * Return value is:
 * 1: flag holds true (is enemy, party, etc)
 * -1: flag fails
 * 0: Invalid target (non-targetable ever)
 *------------------------------------------
 */
int battle_check_target(const block_list *src, const block_list *target, int flag)
{
	int m,state = 0; //Initial state none
	const block_list *s_bl= src, *t_bl= target;
	
	m = target->m;
	if (flag&BCT_ENEMY && !maps[m].flag.gvg)	//Offensive stuff can't be casted on Basilica
	{	// Celest
		//No offensive stuff while in Basilica.
		if (map_getcell(m,src->x,src->y,CELL_CHKBASILICA) ||
			map_getcell(m,target->x,target->y,CELL_CHKBASILICA))
			return -1;
	}

	const skill_unit *tsu = target->get_sk();
	const skill_unit *su = src->get_sk();

	if(tsu) 
	{	
		if( !tsu->group )
			return 0;
		if( su ) //Cannot be hit by another skill.
			return 0;

		if (!(skill_get_inf2(tsu->group->skill_id)&INF2_TRAP || tsu->group->skill_id==WZ_ICEWALL))
			return 0; //Excepting traps and icewall, you should not be able to target skills.
		if ((t_bl = block_list::from_blid(tsu->group->src_id)) == NULL)
			t_bl = target; //Fallback on the trap itself, otherwise consider this a "versus caster" scenario.
	}
	if( su )
	{
		if (!su->group)
			return 0;
		if (su->group->src_id == target->id)
		{
			int inf2;
			inf2 = skill_get_inf2(su->group->skill_id);
			if (inf2&INF2_NO_TARGET_SELF)
				return -1;
			if (inf2&INF2_TARGET_SELF)
				return 1;
		}
		if ((s_bl = block_list::from_blid(su->group->src_id)) == NULL)
			s_bl = src; //Fallback on the trap itself, otherwise consider this a "caster versus enemy" scenario.
	}


	const map_session_data *tsd = NULL;
	const mob_data *tmd = NULL;
	if( (tsd = t_bl->get_sd()) )
	{
		if(tsd->invincible_timer != -1 || tsd->is_invisible() || tsd->ScriptEngine.isRunning())
			return -1; //Cannot be targeted yet.
		if(tsd->state.monster_ignore && *src == BL_MOB)
			return 0; //option to have monsters ignore GMs [Valaris]
		if(tsd->state.killable)
			state |= BCT_ENEMY; //Universal Victim
	}
	else if( (tmd = t_bl->get_md()) )
	{
		if(tmd->state.special_mob_ai == 2)
			return (flag&BCT_ENEMY)?1:-1; //Mines are sort of universal enemies.
		if(tmd->state.special_mob_ai && *src == BL_MOB)
			state |= BCT_ENEMY;	//Summoned creatures can target other mobs.
		//Don't fallback on the master in the case of summoned creaturess to enable hitting them.
		if(tmd->master_id && !tmd->state.special_mob_ai && (t_bl = block_list::from_blid(tmd->master_id)) == NULL)
			t_bl = tmd; //Fallback on the mob itself, otherwise consider this a "versus master" scenario.
	}
	else if( t_bl->get_pd() )
	{
		return 0; //Pets cannot be targetted.
	}
	else if( t_bl->get_sk() )
	{	//Skill with no owner? Kinda odd... but.. let it through.
	}
	else
	{	//Invalid target
		return 0;
	}



	const map_session_data *sd = NULL;
	const mob_data *md = NULL;
	const pet_data *pd =NULL;
	const homun_data *hd = NULL;
	if( (sd=s_bl->get_sd()) )
	{
		if (sd->state.killer)
			state |= BCT_ENEMY; //Is on a killing rampage :O
	}
	else if( (md=s_bl->get_md()) )
	{
		if (!agit_flag && md->guild_id)
			return 0; //Disable guardians on non-woe times.
		if (md->state.special_mob_ai && tmd)
			state |= BCT_ENEMY;	//Summoned creatures can target other mobs.
		if (md->master_id && (s_bl = block_list::from_blid(md->master_id)) == NULL)
			s_bl = md; //Fallback on the mob itself, otherwise consider this a "from master" scenario.
	}
	else if( (pd=s_bl->get_pd()) )
	{
			s_bl = &pd->msd; //"My master's enemies are my enemies..."
	}
	else if( (hd=s_bl->get_hd()) )
	{
			s_bl = &hd->msd; //"My master's enemies are my enemies..."
	}
	else if( s_bl->get_sk() )
	{	//Skill with no owner? Fishy, but let it through.
	}
	else
	{	//Invalid source of attack?
		return 0;
	}
	
	if ((flag&BCT_ALL) == BCT_ALL) { //All actually stands for all players/mobs
		if( tmd || tsd )
			return 1;
		else
			return -1;
	} else if (flag == BCT_NOONE) //Why would someone use this? no clue.
		return -1;
	
	if (t_bl == s_bl) //No need for further testing.
		return (flag&BCT_SELF)?1:-1;
	
	//Check default enemy settings of mob vs players
	if( (md && tsd) || (sd && tmd) )
		state |= BCT_ENEMY;
	
	if(flag&BCT_PARTY || (maps[m].flag.pvp && flag&BCT_ENEMY))
	{	//Identify party state
		int s_party, t_party;
		s_party = s_bl->get_party_id();
		t_party = t_bl->get_party_id();

		if (!maps[m].flag.pvp)
		{
			if (s_party && s_party == t_party)
				state |= BCT_PARTY;
		}
		else
		{
			if (!maps[m].flag.pvp_noparty && s_party && s_party == t_party)
				state |= BCT_PARTY;
			else
			{
				state |= BCT_ENEMY;
			
				if (config.pk_mode)
				{	//Prevent novice engagement on pk_mode (feature by Valaris)
					if( sd &&
						(pc_calc_base_job2( sd->status.class_) == JOB_NOVICE ||  sd->status.base_level < config.pk_min_level))
						state&=~BCT_ENEMY;
					else if (tsd && 
						(pc_calc_base_job2(tsd->status.class_) == JOB_NOVICE || tsd->status.base_level < config.pk_min_level))
						state&=~BCT_ENEMY;
				}
			}
		}
	}

	if (flag&BCT_GUILD || (agit_flag && (maps[m].flag.gvg || maps[m].flag.gvg_dungeon) && flag&BCT_ENEMY))
	{	//Identify guild state
		int s_guild, t_guild;
		s_guild = s_bl->get_guild_id();
		t_guild = t_bl->get_guild_id();

		if (!maps[m].flag.gvg && !maps[m].flag.gvg_dungeon && !maps[m].flag.pvp)
		{
			if (s_guild && t_guild && (s_guild == t_guild || guild_isallied(s_guild, t_guild)))
				state |= BCT_GUILD;
		}
		else
		{
			if (!(maps[m].flag.pvp && maps[m].flag.pvp_noguild) && s_guild && t_guild && (s_guild == t_guild || guild_isallied(s_guild, t_guild)))
				state |= BCT_GUILD;
			else
				state |= BCT_ENEMY;
		}
	}

	if (!state) //If not an enemy, nor a guild, nor party, nor yourself, it's neutral.
		state = BCT_NEUTRAL;
	//Alliance state takes precedence over enemy one.
	else if (state&BCT_ENEMY && state&(BCT_SELF|BCT_PARTY|BCT_GUILD))
		state&=~BCT_ENEMY;
	return (flag&state)?1:-1;
}
/*==========================================
 * 射程判定
 *------------------------------------------
 */
bool battle_check_range(const block_list *src, const block_list *bl, unsigned int range)
{
	unsigned int dx,dy, arange;

	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	dx=abs((int)bl->x - (int)src->x);
	dy=abs((int)bl->y - (int)src->y);
	arange=((dx>dy)?dx:dy);

	if(src->m != bl->m)	// 違うマップ
		return false;

	if( range>0 && range < arange )	// 遠すぎる
		return false;

	if( arange<2 )	// 同じマスか隣接
		return true;

//	if(bl->type == BL_SKILL && ((struct skill_unit *)bl)->group->unit_id == 0x8d)
//		return true;

	// 障害物判定
	return path_search_long(src->m,src->x,src->y,bl->x,bl->y);
}

void battle_init()
{
	add_timer_func_list(battle_delay_damage_sub, "battle_delay_damage_sub");
}


