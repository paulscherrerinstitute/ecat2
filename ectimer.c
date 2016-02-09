/*
 *
 * (c) 2013 Dragutin Maier-Manojlovic     dragutin.maier-manojlovic@psi.ch
 * Date: 01.10.2013
 *
 * Paul Scherrer Institute (PSI)
 * Switzerland
 *
 * This file and the package it belongs to have to be redistributed as is,
 * with no changes.
 * Any changes to this file mean that further distribution is not allowed.
 * Bugs/errors/patches/feature requests can be sent directly to the author,
 * and if they are accepted, will be included in the next version or release.
 *
 * Disclaimer:
 * The software (code, tools) is provided "as is", without warranty of any kind.
 * Author makes no warranties, express or implied, that the code is free of
 * errors, or is consistent with any particular standard, or that it
 * will meet your requirements for any particular application.
 * It should not be relied on for solving a problem whose correct or incorrect
 * solution could result in injury to a person or a loss of property.
 * The author disclaim all liability for direct, indirect or
 * consequential damage resulting from your use of the code or tools.
 * If you do use it, it is at your own risk.
 *
 */


#include "ec.h"


//-------------------------------------------------------------------


static _ec_timer ec_timer[MAX_EC_TIMERS] = { { 0 } };



// add two times

void tmr_add( struct timespec *t1, struct timespec *t2 )
{
	t1->tv_sec += t2->tv_sec;
	t1->tv_nsec += t2->tv_nsec;
	if( t1->tv_nsec < 0 )
	{
		t1->tv_sec--;
		t1->tv_nsec += (long)1000000000;
	}
	if( t1->tv_nsec >= (long)1000000000 )
	{
		t1->tv_sec++;
		t1->tv_nsec -= (long)1000000000;
	}
}

int tmr_compare( struct timespec *t1, struct timespec *t2 )
{
	long sec = t1->tv_sec - t2->tv_sec,
		nsec = t1->tv_nsec - t2->tv_nsec;

	if( sec < 0 )
		return -1; // t1 < t2

	if( sec > 0 )
		return 1; // t1 > t2

	if( nsec < 0 )
		return -1; // t1 < t2
	else if( nsec > 0 )
		return 1; // t1 > t2

	return 0; // t1 == t2
}


void tmr_add_val( struct timespec *t1, long nsec )
{
	t1->tv_nsec += nsec;

	while( t1->tv_nsec >= (long)1000000000 )
	{
		t1->tv_sec++;
		t1->tv_nsec -= (long)1000000000;
	}
}



int tmr_forward( int tmr_nr )
{
	struct timespec now, *last = &ec_timer[tmr_nr].time;
	long rate = ec_timer[tmr_nr].rate;
	int overshoot = 0;

	clock_gettime( CLOCK_MONOTONIC, &now );

	while( tmr_compare( &now, last ) >= 0 )
	{
		tmr_add_val( last, rate );
		overshoot++;
	}

	return overshoot;
}

void inline tmr_setvalue( struct timespec *t, long nsec )
{
	t->tv_nsec = nsec;
	if( (t->tv_sec = t->tv_nsec % (long)1000000000) )
		t->tv_nsec -= t->tv_sec * (long)1000000000;
}


int tmr_init( int tmr_nr, long rate )
{

    if( tmr_nr < 0 || tmr_nr > (MAX_EC_TIMERS-1) )
		return FAIL;

	clock_gettime( CLOCK_MONOTONIC, &ec_timer[tmr_nr].time );

	ec_timer[tmr_nr].rate = rate;

	return OK;
}


int tmr_wait( int tmr_nr )
{
	int retv = tmr_forward( tmr_nr );
	clock_nanosleep( CLOCK_MONOTONIC, TIMER_ABSTIME, &ec_timer[tmr_nr].time, NULL );

	return retv;
}











