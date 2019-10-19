
/**
 * envelope.h by Tom Merchant (mailto:tom@tmerchant.com) , 6th October 2019
 *
 * interface for the library implemented in envelope.c
 * This library provides a system for modulating variables through time with interpolation
 *
 *  LICENSE:
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 **/

#pragma once

#ifndef ENVELOPE_ENVELOPE_H
#define ENVELOPE_ENVELOPE_H


#ifndef ENVELOPE_PRECISION
    #ifdef ENVELOPE_PRECISION_MICROSECOND
        #define ENVELOPE_PRECISION 0.000001
    #elif defined(ENVELOPE_PRECISION_NANOSECOND)
        #define ENVELOPE_PRECISION 0.000000001
    #else
        /* 10 microseconds is the default precision */
        #define ENVELOPE_PRECISION 0.00001
    #endif
#endif

typedef enum interp
{
    LINEAR = 0,
    NEAREST_NEIGHBOUR = 1,
    CUBIC_BEZIER = 2,
    EXPONENTIAL = 3,
    USER_DEFINED = 4
} interp_t;


typedef struct breakpoint
{
    double              time;
    interp_t            interpType;
    double              *interp_params;
    int                 nInterp_params;
    double              value;
    struct breakpoint   *next;

    double ( *interpCallback ) ( struct breakpoint*, double );
} breakpoint;

/**
 * User interpolation callback should return a double
 * Start and end times and values can be retrieved from the breakpoint
 * the second argument is the current time
 */
typedef double ( *interp_callback ) ( breakpoint*, double );

double linear_interp       ( breakpoint *bp, double time );
double nearest_interp      ( breakpoint *bp, double time );
double cubic_bezier_interp ( breakpoint *bp, double time );
double exponential_interp  ( breakpoint *bp, double time );

extern interp_callback interp_functions[3];

typedef enum envelope_type
{
    SIMPLE,
    ADSR
} envelope_type;

typedef struct envelope
{
    breakpoint    *first;
    /**
     * The breakpoint at the current time
     */
    breakpoint    *current;
    double        timeNow;
    /**
     * Whether this is a simple or an ADSR envelope
     */
    envelope_type type;
} envelope;

typedef  struct ADSR_envelope
{
    breakpoint    *first;
    breakpoint    *current;
    double        timeNow;
    envelope_type type;
    breakpoint    *release;
    double        _t;
} ADSR_envelope;


/***********************************************************************
 * Reads breakpoint data from a file
 * each line will be validated using the following regex
 *
 * ^([0-9]*\.[0-9]+)\s([0-9]*\.[0-9]+)\s([0-3])((?:\s[0-9]*\.[0-9]+)*)$
 *
 * Each line is assumed to be one breakpoint in the chain between the
 * previous and next line.
 *
 * @author Tom Merchant
 * @param file The file to load the data from
 * @param env An envelope that already exists
 * @return 0 on success, -1 on failure
 ***********************************************************************/
int    load_breakpoints ( const char* file,  envelope *env       );


int    save_breakpoints ( const char* file,  const envelope *env );
void   set_time         ( envelope *env,     const double t      );

/**********************************************************************
 * Gets the value of an envelope at a particular time
 *
 * @param env
 * @param t time
 * @return
 *********************************************************************/
double value_at         ( envelope *env,     const double t      );


/********************************************************
 * Enters the release phase for an ADSR envelope
 *
 * @param env The envelope to switch to release
 * @param t   The time at which the release occurred
 *******************************************************/
void   ADSR_release     ( ADSR_envelope *env, double t );


/*************************************************
 * Resets an ADSR envelope after the release phase
 *
 * @param env The envelope to reset
 ************************************************/
void   ADSR_reset       ( ADSR_envelope *env );


/***************************************************************
 * Safely frees all malloc'd and calloc'd structures within env.
 * Safe to call on envelopes and ADSR_envelopes
 * @param env
 ***************************************************************/
void   free_env ( envelope *env );

ADSR_envelope* create_ADSR_envelope ( const unsigned long long attack, const unsigned long long decay, const double sustain, const unsigned long long release );

#endif //ENVELOPE_ENVELOPE_H