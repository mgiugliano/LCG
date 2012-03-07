#include <boost/thread.hpp>
#include <stdio.h>
#include "engine.h"
#include "entity.h"
#include "utils.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_LIBLXRT
#include <rtai_lxrt.h>
#include <rtai_shm.h>

#define PRIORITY        0           /* 0 is the maximum */
#define STACK_SIZE      0           /* use default value */
#define MSG_SIZE        0           /* use default value */
#endif // HAVE_LIBLXRT

namespace dynclamp {

double globalT;
double globalDt = SetGlobalDt(1.0/20e3);
#ifdef HAVE_LIBLXRT
double realTimeDt;
double globalTimeOffset = 0.0;
#endif

double SetGlobalDt(double dt)
{
        assert(dt > 0.0);
        globalDt = dt;
#ifdef HAVE_LIBLXRT
        realTimeDt = count2sec(start_rt_timer(sec2count(dt)));
        Logger(Info, "The real time period is %g ms (f = %g Hz).\n", realTimeDt, 1./realTimeDt);
#endif
        return globalDt;
}

double GetGlobalDt()
{
#ifndef HAVE_LIBLXRT
        return globalDt;
#else
        return realTimeDt;
#endif
}

double GetGlobalTime()
{
        return globalT;
}

void IncreaseGlobalTime()
{
#ifdef HAVE_LIBLXRT
        //globalT = count2sec(rt_get_time()) - globalTimeOffset;
        globalT += realTimeDt;
#else
        globalT += globalDt;
#endif
}

void IncreaseGlobalTime(double dt)
{
        globalT += dt;
}

void ResetGlobalTime()
{
        globalT = 0.0;
}

#ifdef HAVE_LIBLXRT

void SetGlobalTimeOffset()
{
        globalTimeOffset = count2sec(rt_get_time());
}

double GetGlobalTimeOffset()
{
        return globalTimeOffset;
}

void RTSimulation(const std::vector<Entity*>& entities, double tend)
{
        RT_TASK *task;
        RTIME tickPeriod;
        RTIME currentTime, previousTime;
        int preambleIterations, flag, i;
        unsigned long taskName;
        size_t nEntities = entities.size();

        Logger(Info, "\n>>>>> DYNAMIC CLAMP THREAD STARTED <<<<<\n\n");

        Logger(Info, "Setting periodic mode.\n");
        rt_set_periodic_mode();

        Logger(Info, "Initialising task.\n");
        taskName = nam2num("hybrid_simulator");
        task = rt_task_init(taskName, PRIORITY, STACK_SIZE, MSG_SIZE);
        if(task == NULL) {
                Logger(Info, "Error: cannot initialise real-time task.\n");
                return;
        }

        Logger(Info, "Setting timer period.\n");
        tickPeriod = start_rt_timer(sec2count(globalDt));
        
        Logger(Info, "The period is %g ms (f = %g Hz).\n", count2ms(tickPeriod), 1./count2sec(tickPeriod));

        Logger(Info, "Switching to hard real time.\n");
        rt_make_hard_real_time();

        flag = rt_task_make_periodic_relative_ns(task, count2nano(5*tickPeriod), count2nano(tickPeriod));
        if(flag != 0) {
                Logger(Info, "Error while making the task periodic.\n");
                goto stopRT;
        }
        
        // some sort of preamble
        currentTime = rt_get_time();
        preambleIterations = 0;
        do {
                previousTime = currentTime;
                rt_task_wait_period();
                currentTime = rt_get_time();
                preambleIterations++;
        } while(currentTime == previousTime);
        Logger(Info, "Performed %d iteration%s in the ``preamble''.\n",
                preambleIterations, (preambleIterations == 1 ? "" : "s"));

        ResetGlobalTime();
        SetGlobalTimeOffset();
        while (GetGlobalTime() <= tend) {
                ProcessEvents();
                for (i=0; i<nEntities; i++)
                        entities[i]->readAndStoreInputs();
                IncreaseGlobalTime();
                for (i=0; i<nEntities; i++)
                        entities[i]->step();
                rt_task_wait_period();
        }

stopRT:
        Logger(Info, "Stopping the timer.\n");
        stop_rt_timer();

        Logger(Info, "Deleting the task.\n");
        rt_task_delete(task);

        Logger(Info, "\n>>>>> DYNAMIC CLAMP THREAD ENDED <<<<<\n\n");
}

#else

void NonRTSimulation(const std::vector<Entity*>& entities, double tend)
{
        size_t i, n = entities.size();
        double dt = GetGlobalDt();
        int nsteps = tend/dt;
        Logger(Debug, "tend = %e, dt = %e, nsteps = %d\n", tend, dt, nsteps);
        nsteps = 0;
        ResetGlobalTime();
        while (GetGlobalTime() <= tend) {
                ProcessEvents();
                for (i=0; i<n; i++)
                        entities[i]->readAndStoreInputs();
                IncreaseGlobalTime();
                for (i=0; i<n; i++)
                        entities[i]->step();
        }
}

#endif // HAVE_LIBLXRT

void Simulate(const std::vector<Entity*>& entities, double tend)
{
#ifdef HAVE_LIBLXRT
        boost::thread thrd(RTSimulation, entities, tend);
#else
        boost::thread thrd(NonRTSimulation, entities, tend);
#endif // HAVE_LIBLXRT
        thrd.join();
}

} // namespace dynclamp


