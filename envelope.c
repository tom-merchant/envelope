
/**
 * envelope.c by Tom Merchant (mailto:tom@tmerchant.com) , 6th October 2019
 *
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

#include "envelope.h"

#include <stdio.h>

//REMEMBER TO USE CALLOC NOT MALLOC
//VERY IMPORTANT ^

/**
 *
 * @param file
 * @param env
 * @return
 */

int load_breakpoints ( const char* file, envelope *env )
{

}

int save_breakpoints ( const char* file, const envelope *env )
{

}

void env_seek ( envelope *env )
{
    int first_iteration = 1;
    breakpoint_time t = env->timeNow;

    while ( !(env->timeFormat == INTEGER && t.time         < env->current->next->time.time        && t.time        > env->current->time.time
              || env->timeFormat == DOUBLE  && t.double_time  < env->current->next->time.double_time && t.double_time > env->current->time.double_time))
    {
        /*next twice as interpolation must always take place between two breakpoints*/
        if ( !first_iteration && ! env->current->next->next )
        {
            break;
        }

        env->current = ( !first_iteration ? env->current->next : env->first );
        first_iteration = 0;
    }
}

void env_set_time ( envelope *env, const breakpoint_time t )
{
    env->timeNow = t;
    env_seek ( env );
}

double value_at ( envelope *env, const breakpoint_time t )
{
    env_set_time ( env, t );
    return env->current->interpCallback ( env->current, t );
}

double env_current_value ( envelope *env )
{
    return env->current->interpCallback ( env->current, env->timeNow );
}

void free_env ( envelope *env )
{

}