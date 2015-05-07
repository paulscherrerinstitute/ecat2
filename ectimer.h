#ifndef ECTIMER_H
#define ECTIMER_H



typedef struct {
	long rate;
    struct timespec time;

} _ec_timer;


#define MAX_EC_TIMERS	16
//---------------------------------

int tmr_init( int tmr_nr, long rate );
int tmr_wait( int tmr_nr );


#endif


















