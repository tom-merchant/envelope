
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
    LINEAR = 0,
    NEAREST_NEIGHBOUR = 1,
    QUADRATIC = 2,
    CUBIC = 3,
    QUARTIC = 4,
    SINUSOIDAL = 5,
    EXPONENTIAL = 6,
    LOGARITHMIC = 7,
    CUBIC_SPLINE = 8,
    HYPERBOLIC = 9,
    USER_DEFINED = 10

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
double quadratic_interp    ( breakpoint *bp, double time );
double cubic_interp        ( breakpoint *bp, double time );
double quartic_interp      ( breakpoint *bp, double time );
double sinusoid_interp     ( breakpoint *bp, double time );
double exponential_interp  ( breakpoint *bp, double time );
double logarithmic_interp  ( breakpoint *bp, double time );
double cubic_spline_interp ( breakpoint *bp, double time );
double hyperbolic_interp   ( breakpoint *bp, double time );

extern interp_callback interp_functions[10];


typedef struct envelope
{
    breakpoint *first;
    breakpoint *current;
    double     timeNow;
} envelope;


int    load_breakpoints ( const char* file,  envelope *env           );
int    save_breakpoints ( const char* file,  const envelope *env     );
void   set_time         ( envelope *env,     const double t );
double value_at         ( envelope *env,     const double t );

/**
 * Safely frees all malloc'd and calloc'd structures within env
 * @param env
 */
void   free_env         ( envelope *env );


envelope* create_ADSR_envelope ( const unsigned long long attack, const unsigned long long decay, const double sustain, const unsigned long long release );

#endif //ENVELOPE_ENVELOPE_H