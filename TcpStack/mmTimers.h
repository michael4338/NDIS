#ifndef	___multimedia_timers___
#define	___multimedia_timers___


#include	<mmsystem.h>



class CMMTimers
{
public:
	CMMTimers(UINT resolution);
	virtual ~CMMTimers();

	UINT	getTimerRes() { return timerRes; };
	UINT    getTimerId() {return timerId;}

	bool	startTimer(UINT period,bool oneShot);
	bool	stopTimer();

	virtual void timerProc() {};

protected:
	UINT	timerRes;
	UINT	timerId;
};



#endif