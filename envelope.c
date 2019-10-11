
/**
 * envelope.c Copyright Tom Merchant (mailto:tom@tmerchant.com) 2019
 *
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

    bp_regex_pattern = "^([0-9]*\\.[0-9]+)\\s([0-9]*\\.[0-9]+)\\s([0-9]|10)((?:\\s[0-9]*\\.[0-9]+)*)$";
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

    return 0;
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

    if ( !env->first->next )
    {
        env->first->next = calloc ( 1, sizeof ( breakpoint ) );
        env->first->next->time = 9.999999999E20;
    }

    /* If the current time isn't between the current and next breakpoint */

    if ( !(t < env->current->next->time && t > env->current->time) )
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
                /* If the next breakpoint is the last then we can't go forward any further as interpolation must happen
                 * between two points */
                if ( !first_iteration && !env->current->next->next ){
                    break;
                }

                env->current = ( !first_iteration ? env->current->next : env->first );
                first_iteration = 0;
            } while ( !(t < env->current->next->time && t > env->current->time) );
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

envelope* create_ADSR_envelope ( const unsigned long long attack, const unsigned long long decay, const double sustain, const unsigned long long release )
{

}

void free_breakpoint_chain ( breakpoint *bp )
{
    if ( ! bp->next )
    {
        free ( bp );
        return;
    }

    free ( bp->interp_params );
    free_breakpoint_chain ( bp->next );
    free ( bp );

    return;
}

void free_env ( envelope *env )
{
    if ( env->first )
    {
        free_breakpoint_chain ( env->first );
    }

    free ( env );
    return;
}

double linear_interp       ( breakpoint *bp, double time )
{

}

double nearest_interp      ( breakpoint *bp, double time )
{

}

double quadratic_interp    ( breakpoint *bp, double time )
{

}

double cubic_interp        ( breakpoint *bp, double time )
{

}

double quartic_interp      ( breakpoint *bp, double time )
{

}

double sinusoid_interp     ( breakpoint *bp, double time )
{

}

double exponential_interp  ( breakpoint *bp, double time )
{

}

double logarithmic_interp  ( breakpoint *bp, double time )
{

}

double cubic_spline_interp ( breakpoint *bp, double time )
{

}

double hyperbolic_interp   ( breakpoint *bp, double time )
{

}

interp_callback interp_functions[10] =  {linear_interp, nearest_interp, quadratic_interp, cubic_interp, quartic_interp,
                                         sinusoid_interp, exponential_interp, logarithmic_interp, cubic_spline_interp,
                                         hyperbolic_interp};