
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

typedef enum interp
{
    LINEAR,
    NEAREST_NEIGHBOUR,
    QUADRATIC,
    CUBIC,
    QUARTIC,
    SINUSOIDAL,
    EXPONENTIAL,
    LOGARITHMIC,
    CUBIC_SPLINE,
    HYPERBOLIC,
    USER_DEFINED

} interp_t;

typedef enum breakpoint_time_format
{
    INTEGER,
    DOUBLE
} breakpoint_time_format;

typedef union breakpoint_time
{
    unsigned long long  time;
    double              double_time;
} breakpoint_time;

typedef struct breakpoint
{
    breakpoint_time     time;
    interp_t            interpType;
    double              value;
    struct breakpoint   *next;

    double ( *interpCallback ) ( struct breakpoint*, breakpoint_time );
} breakpoint;

/**
 * User interpolation callback should return a double
 * Start and end times and values can be retrieved from the breakpoint
 * the second argument is the current time
 */
typedef double ( *interp_callback ) ( breakpoint*, breakpoint_time );

double linear_interp       ( breakpoint *bp, breakpoint_time time );
double nearest_interp      ( breakpoint *bp, breakpoint_time time );
double quadratic_interp    ( breakpoint *bp, breakpoint_time time );
double cubic_interp        ( breakpoint *bp, breakpoint_time time );
double quartic_interp      ( breakpoint *bp, breakpoint_time time );
double sinusoid_interp     ( breakpoint *bp, breakpoint_time time );
double exponential_interp  ( breakpoint *bp, breakpoint_time time );
double logarithmic_interp  ( breakpoint *bp, breakpoint_time time );
double cubic_spline_interp ( breakpoint *bp, breakpoint_time time );
double hyperbolic_interp   ( breakpoint *bp, breakpoint_time time );

interp_callback interp_functions[] = {linear_interp, nearest_interp, quadratic_interp, cubic_interp, quartic_interp,
                                      sinusoid_interp, exponential_interp, logarithmic_interp, cubic_spline_interp,
                                      hyperbolic_interp};


typedef struct envelope
{
    breakpoint_time_format  timeFormat;
    breakpoint              *first;
    breakpoint              *current;
    breakpoint_time         timeNow;
} envelope;


int    load_breakpoints ( const char* file,  envelope *env           );
int    save_breakpoints ( const char* file,  const envelope *env     );
void   set_time         ( envelope *env,     const breakpoint_time t );
double value_at         ( envelope *env,     const breakpoint_time t );

/**
 * Safely frees all malloc'd and calloc'd structures within env
 * @param env
 */
void   free_env         ( envelope *env );

#endif //ENVELOPE_ENVELOPE_H