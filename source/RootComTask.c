
/******************************************************************************
*
*   FILE
*   ----
*   RootComTask.c
*
*   History
*   -------
*   2013-01-15   File created
*
*******************************************************************************
*
*   This file is generated by the 'acplt_builder' command
*
******************************************************************************/


#ifndef OV_COMPILE_LIBRARY_ksbase
#define OV_COMPILE_LIBRARY_ksbase
#endif

#if !OV_SYSTEM_NT
#define _POSIX_C_SOURCE	 199309L
#include <time.h>
#else
#include <windows.h>
#endif

#include "ksbase.h"
#include "libov/ov_macros.h"
#include "ks_logfile.h"
#include "libov/ov_scheduler.h"


void ksbase_RootComTask_execute(OV_INSTPTR_ov_object	pobj);

/**
 * Seconds for the ov_scheduler to call this RootComTask
 */
OV_DLLFNCEXPORT OV_INT ksbase_RootComTask_cycsecs_get(
    OV_INSTPTR_ksbase_RootComTask          pobj
) {
    return pobj->v_cycsecs;
}

/**
 * Seconds for the ov_scheduler to call this RootComTask
 */
OV_DLLFNCEXPORT OV_RESULT ksbase_RootComTask_cycsecs_set(
    OV_INSTPTR_ksbase_RootComTask          pobj,
    const OV_INT  value
) {
    pobj->v_cycsecs = value;
    return OV_ERR_OK;
}

/**
 * USeconds for the ov_scheduler to call this RootComTask
 */
OV_DLLFNCEXPORT OV_INT ksbase_RootComTask_cycusecs_get(
    OV_INSTPTR_ksbase_RootComTask          pobj
) {
    return pobj->v_cycusecs;
}

/**
 * USeconds for the ov_scheduler to call this RootComTask
 */
OV_DLLFNCEXPORT OV_RESULT ksbase_RootComTask_cycusecs_set(
    OV_INSTPTR_ksbase_RootComTask          pobj,
    const OV_INT  value
) {
    pobj->v_cycusecs = value;
    return OV_ERR_OK;
}

/**
 * Checks if this is the only RootComTask. Otherwise returns OV_ERR_GENERIC
 */
OV_DLLFNCEXPORT OV_RESULT ksbase_RootComTask_constructor(
	OV_INSTPTR_ov_object 	pobj
) {
	OV_INSTPTR_ov_object rcTask = NULL;
	rcTask = Ov_GetFirstChild(ov_instantiation, pclass_ksbase_RootComTask);
	if(rcTask && rcTask != pobj)
		return OV_ERR_GENERIC;

	Ov_StaticPtrCast(ksbase_RootComTask, rcTask)->v_cycusecs = 1000;
	return OV_ERR_OK;
}


/**
 * Registers the Task initially at the ov_scheduler
 */
OV_DLLFNCEXPORT void ksbase_RootComTask_startup(
	OV_INSTPTR_ov_object 	pobj
) {
    /*    
    *   local variables
    */
	 OV_TIME_SPAN		t;
	 OV_INSTPTR_ksbase_RootComTask	rcTask;

    /* do what the base class does first */
    ov_object_startup(pobj);

    /* do what */
#if OV_SYSTEM_NT
    {
    	TIMECAPS tc;
    	MMRESULT res;
    	timeGetDevCaps(&tc, sizeof(TIMECAPS));
    	ov_logfile_info("maximum timer resolution is: %ums", tc.wPeriodMin);
    	res = timeBeginPeriod(tc.wPeriodMin);
    	if(res != MMSYSERR_NOERROR)
    	{
    		KS_logfile_error(("timeBeginPeriod: "));
    		KS_logfile_print_sysMsg();
    	}
    }

#endif
    rcTask = Ov_StaticPtrCast(ksbase_RootComTask, pobj);

    t.secs = rcTask->v_cycsecs;
    t.usecs = rcTask->v_cycusecs;
    ov_scheduler_register(pobj, ksbase_RootComTask_execute);
    ov_scheduler_setreleventtime(pobj, &t);
    KS_logfile_debug(("RootComTask registered at ov_scheduler with default intervall %d %d", t.secs, t.usecs));
    return;

}

/**
 * Deregisters the Task at the ov_scheduler
 */
OV_DLLFNCEXPORT void ksbase_RootComTask_shutdown(
	OV_INSTPTR_ov_object 	pobj
) {
    /*    
    *   local variables
    */

    /* do what */
    ov_scheduler_unregister((OV_INSTPTR_ov_object)pobj);
    KS_logfile_debug(("RootComTask UNregistered at ov_scheduler"));
#if OV_SYSTEM_NT
    {
    	TIMECAPS tc;
    	timeGetDevCaps(&tc, sizeof(TIMECAPS));
    	timeEndPeriod(tc.wPeriodMin);
    }
#endif
    /* set the object's state to "shut down" */
    ov_object_shutdown(pobj);

    return;
}

/**
 * gets called by the ov_scheduler and iterates over all existing ComTask Objs
 * Reregisters itself at the ov_scheduler
 */
void ksbase_RootComTask_execute(
	OV_INSTPTR_ov_object	pobj
) {
	OV_TIME_SPAN		t;
        OV_INSTPTR_ksbase_RootComTask	rcTask;
	OV_INSTPTR_ksbase_ComTask 	childTask = NULL;
	OV_INSTPTR_ksbase_ComTask firstChild = NULL;
	OV_VTBLPTR_ksbase_ComTask	pvtable;
	OV_TIME time_next, now, earliestChild;
	OV_TIME_SPAN time_left, ts;
#if !OV_SYSTEM_NT
		struct timespec s;
#endif
#if DBG_PRINT_WAIT_TIME
		OV_TIME waitStart;
		OV_TIME waitEnd;
		OV_TIME_SPAN waitTime;
#endif


	rcTask = Ov_StaticPtrCast(ksbase_RootComTask, pobj);

	//get time_span until next event
	time_left = *(ov_scheduler_getnexteventtime());
	//if next event is too far in the future limit looping to 2 seconds
	if(time_left.secs > 1)
		time_left.secs = 1;
	//get current time
	ov_time_gettime(&now);
	//calculate time of next event
	ov_time_add(&time_next, &now, &time_left);


	do{//loop until next event in ov_scheduler

		//startvalue to estimate time until next (child-)event
		earliestChild = time_next;
		//lets see if something is todo...
		firstChild = Ov_GetFirstChild(ksbase_AssocComTaskList, rcTask);
		childTask = firstChild;
		while(childTask) {//TODO modify this loop not to iterate over ALL children before return if the time for the next ov_scheduler task is reached
			if(ksbase_ComTask_calcExec(childTask)) {//if TRUE, its time to execute this object
				//go via the methodtable to call the "real" implementation of the typemethod
				//KS_logfile_debug(("RootComTask: %s was executed", childTask->v_identifier));
				Ov_GetVTablePtr(ksbase_ComTask, pvtable, childTask);
				if(pvtable)
				{
					pvtable->m_typemethod(childTask);
					//calculate and set next execution time of child task
					ov_time_gettime(&now);
					ts.secs = rcTask->v_cycsecs * childTask->v_cycInterval;
					ts.usecs = rcTask->v_cycusecs * childTask->v_cycInterval;
					if(ts.usecs >= 1000000)
					{
						ts.secs += (ts.usecs / 1000000);
						ts.usecs %= 1000000;
					}
					ov_time_add(&(childTask->v_NextExecTime), &(now), &ts);

					//get the earliest child task to be run again; just to estimate possible sleep time
					//sequence of child-task, however, remains
					if(ov_time_compare(&(childTask->v_NextExecTime), &(earliestChild)) < 0)
					{
						earliestChild = childTask->v_NextExecTime;

					}
				}
				else
					KS_logfile_error(("No Vtable found for %s.", childTask->v_identifier));
			}


			childTask = Ov_GetNextChild(ksbase_AssocComTaskList, childTask);

		}

		ov_time_gettime(&now);
		ov_time_diff(&time_left, &earliestChild, &now);
		//sleep until next task is to be executed
		if((time_left.secs > 0) || ((time_left.secs == 0) && (time_left.usecs > 0)))
		{

#if DBG_PRINT_WAIT_TIME
	    ov_time_gettime(&waitStart);
	    ov_logfile_debug("%s line %u: sleeping %d usecs", __FILE__, __LINE__, time_left.usecs);
#endif

#if !OV_SYSTEM_NT
		s.tv_sec = time_left.secs;
		s.tv_nsec = time_left.usecs*1000;
	    nanosleep(&s, NULL);
#else
	    Sleep(time_left.secs*1000 + time_left.usecs / 1000);
#endif
#if DBG_PRINT_WAIT_TIME
	    ov_time_gettime(&waitEnd);
	    ov_time_diff(&waitTime, &waitEnd, &waitStart);
	    ov_logfile_debug("%s line %u: slept %u seconds and %u microseconds", __FILE__, __LINE__, waitTime.secs, waitTime.usecs);
#endif
		}

		// Checking of ov_activitylock prevents the communication system from freezing when activity lock is set by a ks-command

	}while((ov_time_compare(&time_next, &now) > 0) || ov_activitylock);


	//KS_logfile_debug(("leaving loop"));
	//get called again in a few moments
	t.secs = rcTask->v_cycsecs;
	t.usecs = rcTask->v_cycusecs;
	//KS_logfile_debug(("RootComTask reschedule with intervall %d %d", t.secs, t.usecs));
    ov_scheduler_setreleventtime(pobj, &t);
    return;
}


OV_DLLFNCEXPORT OV_ACCESS ksbase_RootComTask_getaccess(
	OV_INSTPTR_ov_object		pobj,
	const OV_ELEMENT			*pelem,
	const OV_TICKET				*pticket
) {
	/*
	*	local variables
	*/

	/*
	*	switch based on the element's type
	*/
	switch(pelem->elemtype) {
		case OV_ET_VARIABLE:
			if(pelem->elemunion.pvar->v_offset >= offsetof(OV_INST_ov_object,__classinfo)) {
			  if(pelem->elemunion.pvar->v_vartype == OV_VT_CTYPE)
				  return OV_AC_NONE;
			  else
				  return OV_AC_READWRITE;
			}
			break;
		default:
			break;
	}

	return ov_object_getaccess(pobj, pelem, pticket);
}

