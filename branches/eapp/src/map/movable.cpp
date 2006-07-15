#include "showmsg.h"
#include "timer.h"

#include "map.h"
#include "battle.h"
#include "clif.h"
#include "mob.h"
#include "movable.h"
#include "npc.h"
#include "pc.h"
#include "pet.h"
#include "homun.h"
#include "skill.h"
#include "status.h"


const static char dirx[8] = { 0, 1, 1, 1, 0,-1,-1,-1};
const static char diry[8] = { 1, 1, 0,-1,-1,-1, 0, 1};


///////////////////////////////////////////////////////////////////////////////
/// constructor
movable::movable() : 
	canmove_tick(0),
	walktimer(-1),
	bodydir(DIR_S),
	headdir(DIR_S),
	speed(DEFAULT_WALK_SPEED)
{
	add_timer_func_list(this->walktimer_entry, "walktimer entry function");
}

#define NEW_INTERFACE

/// internal walk subfunction
int movable::walktoxy_sub()
{
#ifndef NEW_INTERFACE
	return this->walktoxy_sub_old();
#else
	return 0;
#endif
}
/// walks to a coordinate
int movable::walktoxy(unsigned short x,unsigned short y,bool easy)
{
#ifndef NEW_INTERFACE
	return this->walktoxy_old(x,y,easy);
#else
	return this->walkto(x,y);
#endif
}
/// interrupts walking
int movable::stop_walking(int type)
{
#ifndef NEW_INTERFACE
	return stop_walking_old(type);
#else
	return stop_walking_new(type);
#endif
}
/// do a walk step
int movable::walk(unsigned long tick)
{
#ifndef NEW_INTERFACE
	return this->walkstep_old(tick);
#else
	printf("old walkfunction called\n");
	return 0;
#endif
}
/// change object state
int movable::changestate(int state,int type)
{
#ifndef NEW_INTERFACE
	return changestate_old(state,type);
#else
	do_changestate(state,type);
	return 0;
#endif
}


// old timer entry function
// it actually combines the extraction of the object and calling the 
// object interal handler, which formally was pc_timer, mob_timer, etc.
int movable::walktimer_entry(int tid, unsigned long tick, int id, basics::numptr data)
{
	block_list* bl = map_id2bl(id);
	movable* mv;
	if( bl && (mv=bl->get_movable()) )
	{
		if(mv->walktimer != tid)
		{
			if(battle_config.error_log)
				ShowError("walktimer_entry %d != %d\n",mv->walktimer,tid);
			return 0;
		}
		// timer was executed, clear it
		mv->walktimer = -1;
#ifndef NEW_INTERFACE
		// call the user function
		mv->walktimer_func_old(tid, tick, id, data);
#else
		mv->walktimer_func(tick);
#endif
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// set directions seperately.
void movable::set_dir(dir_t b, dir_t h)
{
	const bool ch = this->bodydir == b || this->headdir == h;
	if(ch) this->bodydir = b, this->headdir = h, clif_changed_dir(*this); 
}
/// set both directions equally.
void movable::set_dir(dir_t d)
{
	const bool ch = this->bodydir == d || this->headdir == d;
	if(ch) this->bodydir = this->headdir = d, clif_changed_dir(*this);
}
/// set body direction only.
void movable::set_bodydir(dir_t d)
{
	const bool ch = this->bodydir == d;
	if(ch) this->bodydir=d, clif_changed_dir(*this);
}
/// set head direction only.
void movable::set_headdir(dir_t d)
{
	const bool ch = this->headdir == d;
	if(ch) this->headdir=d, clif_changed_dir(*this);
}

///////////////////////////////////////////////////////////////////////////////
/// Randomizes target cell.
/// get a random walkable cell that has the same distance from object 
/// as given coordinates do, but randomly choosen. [Skotlex]
bool movable::random_position(unsigned short &xs, unsigned short &ys) const
{
	unsigned short xi, yi;
	unsigned short dist = this->get_distance(xs, ys);
	size_t i;
	if (dist < 1) dist =1;
	
	for(i=0; i<100; ++i)
	{
		xi = this->x + rand()%(2*dist) - dist;
		yi = this->y + rand()%(2*dist) - dist;
		if( map_getcell(this->m, xi, yi, CELL_CHKPASS) )
		{
			xs = xi;
			ys = yi;
			return true;
		}
	}
	// keep the object position, don't move it
	xs = this->block_list::x;
	ys = this->block_list::y;
	return false;
}


///////////////////////////////////////////////////////////////////////////////
/// check for reachability, but don't build a path
bool movable::can_reach(unsigned short x, unsigned short y) const
{
	// true when already at this position
	if( this->block_list::x==x && this->block_list::y==y )
		return true;
	// otherwise try to build a path
	return walkpath_data::is_possible(this->block_list::m,this->block_list::x,this->block_list::y, x, y, 0);
}

///////////////////////////////////////////////////////////////////////////////
/// check for reachability with limiting range, but don't build a path
bool movable::can_reach(const struct block_list &bl, size_t range) const
{
	if( this->block_list::m == bl.m &&
		(range==0 || distance(*this, bl) <= (int)range) )
	{

		if( this->block_list::x==bl.x && 
			this->block_list::y==bl.y )
			return true;

		// Obstacle judging
		if( walkpath_data::is_possible(this->block_list::m, this->block_list::x, this->block_list::y, bl.x, bl.y, 0) )
			return true;

	//////////////
	// possibly unnecessary since when the block itself is not reachable
	// also the surrounding is not reachable
		
		// check the surrounding
		// get the start direction
		const dir_t d = bl.get_direction(*this);

		int i;
		for(i=0; i<8; ++i)
		{	// check all eight surrounding cells by going clockwise beginning with the start direction
			if( walkpath_data::is_possible(this->block_list::m, this->block_list::x, this->block_list::y, bl.x+dirx[(d+i)&0x7], bl.y+diry[(d+i)&0x7], 0) )
				return true;
		}
	//////////////
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
/// speed calculation
int movable::calc_next_walk_step()
{
	if( this->walkpath.finished() )
		return -1;

	// update the object speed
	unsigned short old_speed = this->speed;
	if( old_speed != this->calc_speed() )
	{	// retransfer the object (only) when the speed changes
		// so the client can sync to the new object speed

		// still need to find out the best way 
		// to force the client to cancel the previous walk packet
		// the current one is a bit heavy, though it's rarely used (at least now)

		clif_fixpos(*this);
		if( this->get_sd() ) clif_updatestatus(*this->get_sd(),SP_SPEED);
		clif_moveobject(*this);
	}

	// walking diagonally takes sqrt(2) longer, otherwise take the speed directly
	return (this->walkpath.get_current_step()&1 ) ? this->speed*14/10 : this->speed;
}

///////////////////////////////////////////////////////////////////////////////
/// retrive the actual speed of the object
int movable::calc_speed()
{	// current default is using the status functions 
	// which will be split up later into the objects itself
	// and also merging with the seperated calc_pc
	// possibly combine it with calc_next_walk_step into set_walktimer 
	// for only doing map position specific speed modifications,
	// the other speed modification should go into the status code anyway
	this->speed = status_recalc_speed(this);
	return this->speed;
}

bool movable::is_movable()
{
	if( DIFF_TICK(this->canmove_tick, gettick()) > 0 )
		return false;

	map_session_data *sd = this->block_list::get_sd();
	mob_data *md = this->block_list::get_md();
	pet_data *pd = this->block_list::get_pd();
	npc_data *nd = this->block_list::get_nd();
	homun_data *hd = this->block_list::get_hd();
	if( sd )
	{
		if( sd->skilltimer != -1 && !pc_checkskill(*sd, SA_FREECAST) ||
			pc_issit(*sd) )
			return false;
	}
	else if( md )
	{
		if( md->skilltimer != -1 )
			return false;
	}
	else if( pd )
	{
		if(pd->state.casting_flag )
			return false;
	}
	else if( nd )
	{
		// nothing to break here
	}
	else if( hd )
	{
		if( hd->skilltimer != -1 )
			return false;
		if( hd->attacktimer != -1 )
			return false;
		
	}	
	else
		return false;

	struct status_change *sc_data = status_get_sc_data(this);
	if (sc_data)
	{
		if(	sc_data[SC_ANKLE].timer != -1 ||
			sc_data[SC_AUTOCOUNTER].timer !=-1 ||
			sc_data[SC_TRICKDEAD].timer !=-1 ||
			sc_data[SC_BLADESTOP].timer !=-1 ||
			sc_data[SC_BLADESTOP_WAIT].timer !=-1 ||
			sc_data[SC_SPIDERWEB].timer !=-1 ||
			(sc_data[SC_DANCING].timer !=-1 && (
				(sc_data[SC_DANCING].val4.num && sc_data[SC_LONGING].timer == -1) ||
				sc_data[SC_DANCING].val1.num == CG_HERMODE	//cannot move while Hermod is active.
			)) ||
//			sc_data[SC_STOP].timer != -1 ||
//			sc_data[SC_CLOSECONFINE].timer != -1 ||
//			sc_data[SC_CLOSECONFINE2].timer != -1 ||
			sc_data[SC_MOONLIT].timer != -1 ||
			(sc_data[SC_GOSPEL].timer !=-1 && sc_data[SC_GOSPEL].val4.num == BCT_SELF) // cannot move while gospel is in effect
			)
			return false;
	}
	return true;
}


/// sets the object to idle state
bool movable::set_idle()
{
	if( this->is_walking() )
		this->stop_walking();
	return this->is_idle();
}



bool movable::can_walk(unsigned short m, unsigned short x, unsigned short y)
{	// default only checks for non-passable cells
	return !map_getcell(m,x,y,CELL_CHKNOPASS);
}

/// initialize walkpath. uses current target position as walk target
bool movable::init_walkpath()
{
	if( this->walkpath.path_search(this->block_list::m,this->block_list::x,this->block_list::y,this->walktarget.x,this->walktarget.y,this->walkpath.walk_easy) )
	{
		clif_moveobject(*this);
		return true;
	}
	return false;
}

/// activates the walktimer
bool movable::set_walktimer(unsigned long tick)
{
	//
	// this place will contain the map tile depending speed recalculation
	// and the calc_next_walk_step code

	const int i = this->calc_next_walk_step();
	if( i>0 )
	{
		if( this->walktimer != -1)
		{
			delete_timer(this->walktimer, walktimer_entry);
			//this->walktimer=-1;
		}
		this->walktimer = add_timer(tick+i, walktimer_entry, this->block_list::id, basics::numptr(this) );
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////
/// main walking function
bool movable::walkstep(unsigned long tick)
{
	if( !this->walkpath.finished() &&	// walking not finished
		this->is_movable() &&			// object is movable and can walk on the source tile
		this->can_walk(this->block_list::m,this->block_list::x,this->block_list::y) )
	{	// do a walk step
		int dx,dy;
		dir_t d;
		int x = this->block_list::x;
		int y = this->block_list::y;

		// test the target tile
		for(;;)
		{
			// get the direction to walk to
			d = this->walkpath.get_current_step();
			// get the delta's
			dx = dirx[d];
			dy = diry[d];

			// check if the object can move to the target tile
			if( this->can_walk(this->block_list::m,x+dx,y+dy) )
				// the selected step is valid, so break the loop
				break;

			// otherwise the next target tile is not walkable
			// try to get a new path to the final target 
			// that is different from the current
			if( this->init_walkpath() && d != this->walkpath.get_current_step() )
			{	// and use it
				continue;
			}
			else
			{	// otherwise fail to walk
				clif_fixobject(*this);
				this->do_stop_walking();
				return false;
			}
		}

		// check if crossing a block border
		const bool moveblock = ( x/BLOCK_SIZE != (x+dx)/BLOCK_SIZE || y/BLOCK_SIZE != (y+dy)/BLOCK_SIZE);

		// set the object direction
		this->set_dir( d );

		// signal out-of-sight
		CMap::foreachinmovearea( CClifOutsight(*this),
			this->block_list::m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,dx,dy,this->get_sd()?0:BL_PC);

		// new coordinate
		x += dx;
		y += dy;

		// call object depending move code
		this->do_walkstep(tick, coordinate(x,y), dx,dy);

		skill_unit_move(*this,tick,0);

		// remove from blocklist when crossing a block border
		if(moveblock) this->map_delblock();

		// assign the new coordinate
		this->block_list::x = x;
		this->block_list::y = y;

		// reinsert to blocklist when crossing a block border
		if(moveblock) this->map_addblock();

		skill_unit_move(*this,tick,1);

		// signal in-sight
		CMap::foreachinmovearea( CClifInsight(*this),
			this->block_list::m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,-dx,-dy,this->get_sd()?0:BL_PC);


		// this could be simplified when allowing pathsearch to reuse the current path
		// the change_target member would be obsolete and 
		// this->init_walkpath would be called from this->walkto
		// the extra expence is that multiple target changes within a single walk cycle
		// would also do multiple path searches
		if( this->walkpath.change_target )
		{	// build a new path when target has changed
			this->walkpath.change_target=0;
			this->init_walkpath();
		}
		else
		{	// set the next walk position otherwise
			++this->walkpath;
		}

		// set the timer for the next loop
		if( this->set_walktimer(tick) )
			return true;
	}

	// finished walking
	// the normal walking flow will end here
	// when the target is reached


//	clif_fixobject(*this);	
// it might be not necessary to force the client to sync with the current position
// this might cause small walk irregularities when server and client are slightly out of sync

	this->do_stop_walking();
	return true;
}

bool movable::walkto(const coordinate& pos)
{
	if( this->block_list::prev != NULL )
	{
		//
		// insert code for target position checking here
		//

		this->walktarget = pos;

		if( this->is_confuse() ) //Randomize target direction.
			this->random_position(this->walktarget);

		if( this->walktimer == -1 )
		{
			if( this->init_walkpath() )
			{
				this->set_walktimer( gettick() );
				return true;
			}
		}
		else
		{
			this->walkpath.change_target=1;
			return true;
		}
	}
	return false;
}

bool movable::stop_walking_new(int type)
{
	if( this->walktimer != -1 )
	{
		delete_timer(this->walktimer, this->walktimer_entry);
		this->walktimer = -1;
		this->walkpath.clear();
		this->walktarget = *this;

		// always send a fixed position to the client
//		if(type&0x01)
		{
			clif_fixpos(*this);
		}

		// move this out
		if( type&0x02 )
		{
			const int delay=status_get_dmotion(this);
			unsigned long tick;
			if( delay && DIFF_TICK(this->canmove_tick,(tick = gettick()))<0 )
				this->canmove_tick = tick + delay;
		}

		do_stop_walking();
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
/// calculate a position around the target coordiantes
bool movable::calc_pos(const block_list &target_bl)
{
	dir_t default_dir = target_bl.get_dir();
	int x,y,dx,dy;
	int i,k;
	uint32 vary = rand();
	

	default_dir = (dir_t)((default_dir+vary&0x03-1)&0x07);	// vary the default direction

	dx = -dirx[default_dir]*((vary&0x04)!=0)?2:1;
	dy = -diry[default_dir]*((vary&0x08)!=0)?2:1;

	x = target_bl.x + dx;
	y = target_bl.y + dy;
	if( !this->can_reach(x,y) )
	{	// bunch of unnecessary code
		if(dx > 0) x--;
		else if(dx < 0) x++;
		if(dy > 0) y--;
		else if(dy < 0) y++;
		if( !this->can_reach(x,y) )
		{
			for(i=0;i<12;++i)
			{
				k = rand()&0x07;
				dx = -dirx[k]*2;
				dy = -diry[k]*2;
				x = target_bl.x + dx;
				y = target_bl.y + dy;
				if( this->can_reach(x,y) )
					break;
				else
				{
					if(dx > 0) x--;
					else if(dx < 0) x++;
					if(dy > 0) y--;
					else if(dy < 0) y++;
					if( this->can_reach(x,y) )
						break;
				}
			}
			if(i>=12)
				return false;
		}
	}

	this->walktarget.x = x;
	this->walktarget.y = y;
	return true;
}