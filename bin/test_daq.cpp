#include <stdio.h>
#include <vector>
#include "entity.h"
#include "analog_io.h"
#include "stimulus_generator.h"
#include "recorders.h"
#include "engine.h"
#include "utils.h"
#include "delay.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define USE_DELAY

using namespace dynclamp;
using namespace dynclamp::generators;
using namespace dynclamp::recorders;

int main()
{
#ifdef HAVE_LIBCOMEDI

        SetLoggingLevel(Debug);

#ifdef USE_DELAY
        std::vector<Entity*> entities(9);
#else
        std::vector<Entity*> entities(7);
#endif

        try {
                // AO0
                entities[0] = new AnalogOutput("/dev/comedi0", 1, 0, 1.);
                // AO1
                entities[1] = new AnalogOutput("/dev/comedi0", 1, 1, 1.);
#ifdef HEKA
                // AI0
                entities[2] = new AnalogInput("/dev/comedi0", 0, 0, 1.);
                // AI1
                entities[3] = new AnalogInput("/dev/comedi0", 0, 1, 1.);
#else
                // AI2
                entities[2] = new AnalogInput("/dev/comedi0", 0, 2, 1.);
                // AI3
                entities[3] = new AnalogInput("/dev/comedi0", 0, 3, 1.);
#endif

                entities[4] = new Stimulus("positive-step.stim");
                entities[5] = new Stimulus("negative-step.stim");

                entities[6] = new H5Recorder(false, "daq_test.h5");
        
                // connect positive stimulus to AO0
                entities[4]->connect(entities[0]);
                // connect negative stimulus to AO1
                entities[5]->connect(entities[1]);

#ifdef USE_DELAY
                entities[7] = new Delay();
                entities[8] = new Delay();

                // connect entities to the recorder
                entities[0]->connect(entities[6]);
                entities[1]->connect(entities[6]);
                entities[2]->connect(entities[7]);
                entities[7]->connect(entities[6]);
                entities[3]->connect(entities[8]);
                entities[8]->connect(entities[6]);
                entities[4]->connect(entities[6]);
                entities[5]->connect(entities[6]);
#else
                // connect entities to the recorder
                for (uint i=0; i<entities.size()-1; i++)
                        entities[i]->connect(entities[entities.size()-1]);
#endif

                Simulate(entities, 2.0);

        } catch (const char* msg) {
                fprintf(stderr, "Error: %s.\n", msg);
        }

        for (uint i=0; i<entities.size(); i++)
                delete entities[i];

#else
        Logger(Critical, "This program needs COMEDI.\n"):
#endif

        return 0;
}

