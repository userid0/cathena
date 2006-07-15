#ifndef __FIGHTABLE_H__
#define __FIGHTABLE_H__

#include "movable.h"


// questions:
// do we need seperated attackttimer and skilltimer?
// since skilltimer needs separation between area and target skills anyway, 
// separating also timed attacks would be no problem
// {my answer: no, thus can compound attack and skill execution interface [Hinoko]}
// {not sure, but I don't see a reason of having casting in parallel to attacking exept for the auto spells maybe [Shinomori]}
//
// do we need seperated target markers for attack and skill usage?
// {my answer: no [Hinoko]}
//
// can multiple skills be casted in parallel aka do we need arrays for skill_timerskill
// {my answer: no [Hinoko]}
//
// is a separated set of skill casting parameters necessary at the object itself
// {my answer: no [Hinoko]}
//
// do we need arrays of unitgroups (or would a linked list be sufficient)
// or even better, have no unitgroup saving at the char but completely outside
// {my answer: set up everything externally, no extra data at the casting object at all [Hinoko]}
//

struct fightable : public movable
{
	int attacktimer;
	int skilltimer;
	unsigned long attackable_tick;
	uint32 target_id;
	unsigned char attack_continue : 1 ;	// could be part of the attack object

/*
	struct skill_timerskill *skilltimerskill[MAX_SKILLTIMERSKILL];
	struct skill_unit_group *skillunit[MAX_SKILLUNITGROUP];
	struct skill_unit_group_tickset skillunittick[MAX_SKILLUNITGROUPTICKSET];
	short attacktarget_lv;

	short skillx,skilly;
	short skillid,skilllv;
	int   skilltarget;

	unsigned int canact_tick;
	unsigned char running : 1;

	unsigned short skillx;
	unsigned short skilly;
	unsigned short skillid;
	unsigned short skilllv;
	uint32   skilltarget;

	struct linkdb_node *skilltimerskill;
	struct linkdb_node *skillunit;
	struct linkdb_node *skilltickset;

	short attacktarget_lv;
	
	unsigned long attackable_tick;
	unsigned long canact_tick;

	unsigned char skillcastcancel : 1 ;	// results from the executed skill object
*/


	///////////////////////////////////////////////////////////////////////////
	/// constructor
	fightable();
	///////////////////////////////////////////////////////////////////////////
	/// destructor
	virtual ~fightable()	{}

	///////////////////////////////////////////////////////////////////////////
	/// upcasting overload.
	virtual fightable*	get_fightable()		{ return this; }


	/// attacktimer entry point.
	static int attacktimer_entry(int tid, unsigned long tick, int id, basics::numptr data);
	/// call back function for the attacktimer
	virtual int attacktimer_func(int tid, unsigned long tick, int id, basics::numptr data)=0;

	/// skilltimer entry point.
	static int skilltimer_entry(int tid, unsigned long tick, int id, basics::numptr data);
	/// call back function for the skilltimer
	virtual int skilltimer_func(int tid, unsigned long tick, int id, basics::numptr data)=0;



	///////////////////////////////////////////////////////////////////////////
	// status functions

	/// checks for attack state
	virtual bool is_attacking() const	{ return (attacktimer!=-1); }
	/// checks for skill state
	virtual bool is_skilling() const	{ return (skilltimer!=-1); }

	/// checks for idle state (alive+not sitting+not blocked by skill)
	virtual bool is_idle() const		{ return !is_attacking() && !is_skilling() && this->movable::is_idle(); }

	/// sets the object to idle state
	virtual bool set_idle();

	/// starts attack
	virtual bool start_attack(uint32 target_id, bool cont);
	/// stops attack
	virtual bool stop_attack();
	/// stops skill
	virtual bool stop_skill();

	/// do object depending stuff for attacking
	virtual void do_attack()	{}


	///////////////////////////////////////////////////////////////////////////
	// targeting functions

	void unlock_target()
	{
		this->target_id=0;
	}


};







#endif//__FIGHTABLE_H__
