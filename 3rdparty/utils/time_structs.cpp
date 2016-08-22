#include "time_structs.h"


#ifdef PLATFORM_WINDOWS

//-------------------------------------------------------------------------------------
// Name: UpdateTime()
// Desc: Updates the elapsed time since our last frame.
//-------------------------------------------------------------------------------------
void UpdateTime(TimeInfo* pti)
{
    LARGE_INTEGER qwNewTime;
    LARGE_INTEGER qwDeltaTime;

    QueryPerformanceCounter( &qwNewTime );    
    qwDeltaTime.QuadPart = qwNewTime.QuadPart - pti->qwTime.QuadPart;

    pti->qwAppTime.QuadPart += qwDeltaTime.QuadPart;    
    pti->qwTime.QuadPart     = qwNewTime.QuadPart;

    pti->fElapsedTime      = pti->fSecsPerTick * ((FLOAT)(qwDeltaTime.QuadPart));
    pti->fAppTime          = pti->fSecsPerTick * ((FLOAT)(pti->qwAppTime.QuadPart));    
}


//-------------------------------------------------------------------------------------
// Name: InitTime()
// Desc: Initializes the timer variables
//-------------------------------------------------------------------------------------
TimeInfo InitTime()
{    
    TimeInfo ti;
    // Get the frequency of the timer
    LARGE_INTEGER qwTicksPerSec;
    QueryPerformanceFrequency( &qwTicksPerSec );
    ti.fSecsPerTick = 1.0f / (float)qwTicksPerSec.QuadPart;

    // Save the start time
    QueryPerformanceCounter( &ti.qwTime );

    // Zero out the elapsed and total time
    ti.qwAppTime.QuadPart = 0;
    ti.fAppTime = 0.0f; 
    ti.fElapsedTime = 0.0f;
    return ti; 
}

#else 
void UpdateTime(TimeInfo* pti)
{
	(void)pti;
}
TimeInfo InitTime()
{
	return TimeInfo();
}
#endif
