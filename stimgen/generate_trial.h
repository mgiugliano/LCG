/***********************************************************************************************

 Antwerp, 11/7/2009
 Bern, 22/1/2004 - Michele Giugliano, PhD

***********************************************************************************************/

#ifndef GENERATE_TRIAL_H
#define GENERATE_TRIAL_H

#include <math.h>
#include <time.h>
#include <stdio.h>

#include "error_msgs.h"
#include "file_parsing.h"
#include "waveforms.h"
#include "stimgen_common.h"

#ifdef __cplusplus
extern "C" {
#endif

int generate_trial(const char *, int , int , char *, double **, uint *, double, double);	// function prototype

#ifdef __cplusplus
}
#endif

#endif
