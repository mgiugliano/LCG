#ifndef STIMGEN_COMMON_H
#define STIMGEN_COMMON_H

#define SOFTWARE            "CREATE_STIMULUS"
#define VERSION             2009
#define USAGE               "USAGE:\n%s verb{0,1} outbinfile{0,1,-1} srate[Hz] fname1 [fname2 [fname3 [fname4 ...[fnameNchan]]]]\n\n"

#define TWOPI 6.283185307179586476925286766559   // Mathematical constant := 2 * PI.
#define POSPART(m) (((m) > 0) ? m : 0.)

#define DURATION 0
#define CODE     1
#define P1       2
#define P2       3
#define P3       4
#define P4       5
#define P5       6
#define FIXSEED  7
#define MYSEED   8
#define SUBCODE  9
#define PREC_OP 10
#define EXPON   11

#define DC_WAVE         1
#define ORNUHL_WAVE     2
#define SINE_WAVE       3
#define SQUARE_WAVE     4
#define SAW_WAVE        5
#define SWEEP_WAVE      6
#define RAMP_WAVE       7
#define POISSON1_WAVE   8
#define POISSON2_WAVE   9
#define BIPOLAR_WAVE    10
#define UNIF_NOISE      11

#define SUMMATION       1
#define MULTIPLICATION  2
#define SUBTRACTION     3
#define DIVISION        4

typedef unsigned long int INT;                   // Type definition for my convenience.

#endif

