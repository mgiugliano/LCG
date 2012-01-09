#include <cstdio>>
#include "utils.h"
#include "ou.h"
#include "recorders.h"

using namespace dynclamp;
using namespace dynclamp::recorders;

#define N_ENT 4

int main()
{
        int i;
        double t;
        double tend = 5;
        double sigma = 50;
        double tau = 10e-3;
        double i0 = 250;
        Entity *entities[N_ENT];
        entities[0] = new ASCIIRecorder("ou_test.out");
        for (i=1; i<N_ENT; i++) {
                entities[i] = new OUcurrent(sigma, tau, i0, (double) i);
                entities[i]->connect(entities[0]);
        }

        while((t=GetGlobalTime()) <= tend) {
                for (i=0; i<N_ENT; i++)
                        entities[i]->readAndStoreInputs();
                IncreaseGlobalTime();
                for (i=0; i<N_ENT; i++)
                        entities[i]->step();
        }

        for (i=0; i<N_ENT; i++)
                delete entities[i];

        return 0;
}

