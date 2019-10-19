
/**
 * envelope.c Copyright Tom Merchant (mailto:tom@tmerchant.com) 2019
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


#include "envelope.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcre2posix.h>
#include <sys/stat.h>
#include <math.h>


int check_sanity ( breakpoint *bp )
{
    while ( bp->next )
    {
        if ( bp->time > bp->next->time
        || bp->time < 0 || bp->next->time < 0
        || ( bp->interpType == CUBIC_BEZIER && ( bp->nInterp_params < 4 ) )
        || ( bp->next->interpType == CUBIC_BEZIER && ( bp->next->nInterp_params < 4 ) ) )
        {
            return -1;
        }

        bp = bp->next;
    }

    return 0;
}


int load_breakpoints ( const char* file, envelope *env )
{
    int i, j, size, interp_array_size = 3, params_allocated = 0;
    long long filesz;
    const int ngroups = 5;
    char *file_buffer, *token, *last_token, *interp_params, *tmp;
    const char *bp_regex_pattern;
    regex_t bp_regex;
    regmatch_t groups[ngroups];
    breakpoint* top = NULL, *current;
    double *interp_param_array, *tmp1;
    struct stat buf;

    if ( stat( file, &buf ) )
    {
        return -1;
    }


    /* Open file and read line by line */
    FILE *bp_file = fopen ( file, "r" );
    fseek( bp_file, 0L, SEEK_END );
    filesz = ftell( bp_file );
    rewind ( bp_file );
    file_buffer = malloc ( filesz + 1 );

    /* The format of each line should be as follows:
     * time value interp_type interp_param1 interp_param2 ...
     * time and value being doubles, interp_type being an integer and interp_paramN being doubles
     */

    bp_regex_pattern = "^([0-9]*\\.[0-9]+)\\s([0-9]*\\.[0-9]+)\\s([0-4])((?:\\s[0-9]*\\.[0-9]+)*)$";
    regcomp ( &bp_regex, bp_regex_pattern, REG_EXTENDED);


    while ( fgets (file_buffer, filesz, bp_file) )
    {
        if ( ! regexec ( &bp_regex, file_buffer, ngroups, groups, 0 ) )
        {
            if ( !top )
            {
                top = calloc ( 1, sizeof ( breakpoint ));
                current = top;
            }
            else
            {
                current->next = calloc ( 1, sizeof ( breakpoint ) );
                current = current->next;
            }

            for ( j = 0; j < ngroups; ++j )
            {
                size = groups[ j ].rm_eo - groups[ j ].rm_so;

                tmp = calloc ( 1, size + 1 );
                strncpy ( tmp, &file_buffer [ groups[ j ].rm_so ], size );

                switch ( j )
                {
                    case 1:
                        current->time = atof ( tmp );
                        break;
                    case 2:
                        current->value = atof ( tmp );
                        break;
                    case 3:
                        current->interpType = atoi ( tmp );
                        if ( current->interpType < 3 )
                        {
                            current->interpCallback = interp_functions [ current->interpType ];
                        }
                        break;
                    case 4:
                        interp_params = malloc ( size + 1 );
                        params_allocated = 1;
                        strncpy ( interp_params, tmp, size + 1 );
                }

                free ( tmp );
            }

            token = strtok ( interp_params, " \t\r\n\v\f" );
            last_token = 0;

            interp_param_array = malloc ( interp_array_size * sizeof ( double ) );
            i = 0;

            while ( token != NULL )
            {
                last_token = ( last_token == 0 ? interp_params : last_token + 1);

                if ( i >= interp_array_size - 1 )
                {
                    tmp1 = malloc( sizeof ( double ) * ++interp_array_size);
                    memcpy ( tmp1, interp_param_array, interp_array_size - 1 );
                    free ( interp_param_array );
                    interp_param_array = tmp1;
                }

                interp_param_array [ i ] = atof ( last_token );

                last_token = token;

                token = strtok ( NULL, " \t\r\n\v\f" );

                i++;
            }

            if ( params_allocated )
            {
                free ( interp_params );
                params_allocated = 0;
            }

            current->interp_params = interp_param_array;
            current->nInterp_params = i;
        }
        memset (file_buffer, 0, filesz );
    }

    env->first = top;

    /* Clean up */
    regfree ( &bp_regex );
    fclose ( bp_file );
    free ( file_buffer );
    free ( interp_param_array );

    return check_sanity ( env->first );
}

int save_breakpoints ( const char* file, const envelope *env )
{
    int i;
    FILE *bp_file = fopen ( file, "w+" );
    breakpoint *current_bp = env->first;

    while ( current_bp )
    {
        fprintf ( bp_file, "%f ", current_bp->time );
        fprintf ( bp_file, "%f ", current_bp->value);
        fprintf ( bp_file, "%d", current_bp->interpType);

        for ( i = 0; i < current_bp->nInterp_params; i++ )
        {
            fprintf ( bp_file, " %f", current_bp->interp_params[i] );
        }

        fprintf ( bp_file, "\n");

        current_bp = current_bp->next;
    }

    fclose ( bp_file );
}

void env_seek ( envelope *env )
{
    int first_iteration = 1;
    double t = env->timeNow;

    if ( !env->first)
    {
        env->first = calloc ( 1, sizeof ( breakpoint ) );
    }

    /* If the current time isn't between the current and next breakpoint */

    if ( env->current->next && !(t < env->current->next->time && t > env->current->time) )
    {
        /* Then we check if it is between the next one and the one after that */
        if ( env->current->next->next && (t < env->current->next->next->time && t > env->current->next->time) )
        {
            /* If it is, as it should be in most cases, then we just go to the next one */
            env->current = env->current->next;
        }
        else
        {
            /* Otherwise we start back at the beginning of the chain and work through until we find the correct
             * breakpoint */
            do
            {
                env->current = ( !first_iteration ? env->current->next : env->first );
                first_iteration = 0;
            } while ( env->current->next && !(t < env->current->next->time && t > env->current->time) );
        }
    }

    return;
}

void env_set_time ( envelope *env, const double t )
{
    env->timeNow = t;
    env_seek ( env );
}

double value_at ( envelope *env, const double t )
{
    env_set_time ( env, t );
    return env->current->interpCallback ( env->current, t );
}

double env_current_value ( envelope *env )
{
    return env->current->interpCallback ( env->current, env->timeNow );
}

ADSR_envelope* create_ADSR_envelope ( const unsigned long long attack, const unsigned long long decay,
        const double sustain, unsigned long long release )
{

    if ( release < 0.0011 )
    {
        release = 0.0012;
    }

    ADSR_envelope *created = calloc ( 1, sizeof ( ADSR_envelope ) );

    created->type = ADSR;

    breakpoint *first, *release_bp, *second, *sustain_bp, *end;

    first =      calloc ( 1, sizeof ( breakpoint ) );
    second     = calloc ( 1, sizeof ( breakpoint ) );
    sustain_bp = calloc ( 1, sizeof ( breakpoint ) );
    release_bp = calloc ( 1, sizeof ( breakpoint ) );
    end        = calloc ( 1, sizeof ( breakpoint ) );

    first->time = 0;
    first->value = 0;
    first->interpType = LINEAR;
    first->interpCallback = linear_interp;
    first->next = second;

    second->time = attack;
    second->value = 1;
    second->interpType = LINEAR;
    second->interpCallback = linear_interp;
    second->next = sustain_bp;

    sustain_bp->time = attack + decay;
    sustain_bp->value = sustain;
    second->interpType = LINEAR;
    first->interpCallback = linear_interp;

    release_bp->time = 0;
    release_bp->value = sustain;
    release_bp->interpType = EXPONENTIAL;
    release_bp->interpCallback = exponential_interp;
    release_bp->next = end;

    end->time = release;
    end->value = 0;
    end->interpType = LINEAR;
    end->interpCallback = linear_interp;

    created->release = release_bp;
    created->first = first;
    created->current = first;

    return created;
}

void ADSR_release ( ADSR_envelope *env, double t )
{
    breakpoint *current;

    env->current = env->release;
    current = env->current;

    env->_t = t;

    while ( current )
    {
        current->time += t;
        current->interp_params [ 0 ] += t;
        current->interp_params [ 2 ] += t;
        current = current->next;
    }
}

void ADSR_reset ( ADSR_envelope *env )
{
    env->current = env->first;

    breakpoint *current;

    current = env->release;

    while ( current )
    {
        current->time -= env->_t;
        current->interp_params [ 0 ] -= env->_t;
        current->interp_params [ 2 ] -= env->_t;
        current = current->next;
    }

    env->_t = 0;
}

void free_breakpoint_chain ( breakpoint *bp )
{
    if ( ! bp->next )
    {
        free ( bp );
        return;
    }

    if ( bp->interp_params )
    {
        free ( bp->interp_params );
    }

    free_breakpoint_chain ( bp->next );
    free ( bp );

    return;
}

void free_env ( envelope *env )
{
    if ( env->type == ADSR )
    {
        if ( ((ADSR_envelope*)env)->release )
        {
            free_breakpoint_chain ( ((ADSR_envelope*)env)->release );
        }
    }

    if ( env->first )
    {
        free_breakpoint_chain ( env->first );
    }

    free ( env );
    return;
}

double linear_interp ( breakpoint *bp, double time )
{
    double t1, t2, v1, v2, m;

    if ( bp->next )
    {
        t1 = bp->time;
        t2 = bp->next->time;
        v1 = bp->value;
        v2 = bp->next->value;

        m = (v2 - v1) / (t2 - t1);

        return v1 + m * (time - t1);
    }
    else
    {
        return bp->value;
    }
}

double nearest_interp ( breakpoint *bp, double time )
{

    if ( bp->next )
    {
        if ( abs ( bp->time - time ) < abs ( bp->next->time - time ) )
        {
            return bp->value;
        }
        else
        {
            return bp->next->value;
        }
    }
    else
    {
        return bp->value;
    }
}

double cubic_bezier ( double p0, double p1, double p2, double p3, double t)
{
    return (1 - t) * (1 - t) * (1 - t) * p0 + 3 * (1 - t) * (1 - t) * t * p1 + 3 * (1 - t) * t * t * p2 + t * t * t
           * p3;
}

double cubic_bezier_root_at_x ( double p0, double p1, double p2, double p3, double t, double x)
{
    return (1 - t) * (1 - t) * (1 - t) * p0 + 3 * (1 - t) * (1 - t) * t * p1 + 3 * (1 - t) * t * t * p2 + t * t * t
           * p3 - x;
}

double cubic_bezier_derivative ( double p0, double p1, double p2, double p3, double t )
{
    return 3 * (1 - t) * (1 - t) * (p1 - p0) + 6 * (1 - t) * t * (p2 - p1) + 3 * t * t * (p3 - p2);
}

double cubic_bezier_second_derivative ( double p0, double p1, double p2, double p3, double t )
{
    return 6 * (1 - t) * (p2 - 2 * p1 + p0) + 6 * t * (p3 - 2 * p2 + p1);
}


/* The bezier curve calculation doesn't take time as a parameter but rather a variable representing the position of a
 * point on the curve. This means we must find a value for this parameter that gets us the time we are looking for.
 * One way to do this could be to store a large lookup table and use linear interpolation where we don't have data.
 * I have decided instead to do a linear approximation of the time and then several iterations of Halley's method This
 * will take more time but seems more appropriate as it should result in fewer aliasing issues as breakpoint time
 * intervals could vary greatly. */

double cubic_bezier_interp ( breakpoint *bp, double time )
{
    int i, n;
    double t, fx, fpx, fppx, interval [ 2 ], approx, fa, fb;

    if ( !bp->next || bp->nInterp_params < 4 )
    {
        return bp->value;
    }

    /* Our guess for t */
    t = (time - bp->time) / (bp->next->time - bp->time);

    for ( i = 0; i < 25; i++ )
    {
        fx = cubic_bezier_root_at_x ( bp->time, bp->interp_params[0], bp->interp_params[2], bp->next->time, t,
                time );
        fpx = cubic_bezier_derivative ( bp->time, bp->interp_params[0], bp->interp_params[2], bp->next->time, t
        );
        fppx = cubic_bezier_second_derivative ( bp->time, bp->interp_params[0], bp->interp_params[2],
                bp->next->time, t );

        if ( ! (2 * fpx * fpx) - fx * fppx )
        {
            break;
        }

        t = t - ( 2 * fx * fpx ) / ((2 * fpx * fpx) - fx * fppx);

        if ( fabs(fx) < ENVELOPE_PRECISION ) break;
    }

    if ( ! fabs(cubic_bezier_root_at_x ( bp->time, bp->interp_params[0], bp->interp_params[2],
            bp->next->time, t, time )) < ENVELOPE_PRECISION )
    {
        /* Halley's method didn't find the root so we must use regula falsi */

        n = 0;

        while ( fabs(cubic_bezier_root_at_x ( bp->time, bp->interp_params[0], bp->interp_params[2],
                bp->next->time, approx, time)) > ENVELOPE_PRECISION && n < 10000 )
        {
            interval[ 0 ] = 0;
            interval[ 1 ] = 1;

            fa = cubic_bezier_root_at_x ( bp->time, bp->interp_params[0], bp->interp_params[2], bp->next->time,
                                          interval[ 0 ], time );
            fb = cubic_bezier_root_at_x ( bp->time, bp->interp_params[0], bp->interp_params[2], bp->next->time,
                                          interval[ 1 ], time );

            approx = ( interval[ 0 ] * fb - interval[ 1 ] * fa ) / ( fb - fa );

            if ( signbit(cubic_bezier_root_at_x ( bp->time, bp->interp_params[0], bp->interp_params[2],
                    bp->next->time, approx, time )) == signbit ( fa ) )
            {
                interval[ 0 ] = approx;
            }
            else
            {
                interval[ 1 ] = approx;
            }

            n++;
        }

        t = approx;
    }

    if ( fabs(cubic_bezier_root_at_x ( bp->time, bp->interp_params[0], bp->interp_params[2], bp->next->time,
            t, time)) > ENVELOPE_PRECISION )
    {
        /* We couldn't calculate the bezier interpolation so just do a linear interpolation instead */
        return linear_interp ( bp, time );
    }

    return cubic_bezier (bp->value, bp->interp_params[1], bp->interp_params[3], bp->next->value, t);
}

double exponential_interp ( breakpoint *bp, double t )
{

    if ( !bp->next )
    {
        return bp->value;
    }

    if ( bp->time > bp->next->time )
    {

    }
    else
    {

    }
}

interp_callback interp_functions[3] =  { linear_interp, nearest_interp, cubic_bezier_interp, exponential_interp };