// original : core.h 2003/03/14 11:55:25 Rev 1.4

#ifndef	_TIMER_H_
#define	_TIMER_H_

#include "base.h"

#define BASE_TICK 5

#define TIMER_ONCE_AUTODEL 1
#define TIMER_INTERVAL 2
//#define TIMER_REMOVE_HEAP 16

#define DIFF_TICK(a,b) (((int)(a)-(int)(b)))

// Struct declaration

struct TimerData {
	unsigned long	tick;
	int (*func)(int,unsigned int,int,int);
	int				id;
	int				data;
	int				type;
	unsigned long	interval;
//	size_t			heap_pos; // not used and maybe useless, heap positiona are changing
};

// Function prototype declaration

unsigned long gettick_nocache(void);
unsigned long gettick(void);

int add_timer(unsigned long,int (*)(int,unsigned int,int,int),int,int);
int add_timer_interval(unsigned long,int (*)(int,unsigned int,int,int),int,int,int);
int delete_timer(size_t,int (*)(int,unsigned int,int,int));

int addtick_timer(size_t tid,unsigned long tick);
struct TimerData *get_timer(size_t tid);

int do_timer(unsigned long tick);

int add_timer_func_list(int (*)(int,unsigned int,int,int),char*);
void do_final_timer(void);
char* search_timer_func_list(int (*)(int,unsigned int,int,int));

extern void timer_final();

#endif	// _TIMER_H_
