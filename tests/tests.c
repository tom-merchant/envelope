
#include <stdarg.h>
#include <stdlib.h>
#include <setjmp.h>
#include <cmocka.h>
#include "../envelope.h"

static void test_load_save_breakpoints ( void **state )
{
    (void) state;

    int retval;

    envelope *env = malloc ( sizeof ( envelope ) );

#ifndef _WIN32
    retval = load_breakpoints ( "testdata/test_1.bp", env );
#else
    retval = load_breakpoints ( "testdata\\test_1.bp", env );
#endif

    assert_int_equal ( retval, 0 );

    assert_true ( env->first );
    assert_true (env->first->next );

    assert_int_equal ( env->first->interpType, NEAREST_NEIGHBOUR );
    assert_int_equal ( env->first->nInterp_params, 50 );
    assert_int_equal ( env->first->next->nInterp_params, 23 );

    free_env ( env );
}

int main ()
{
    const struct CMUnitTest tests[] =
    {
            cmocka_unit_test( test_load_save_breakpoints )
    };

    cmocka_set_message_output ( CM_OUTPUT_STDOUT );

    cmocka_run_group_tests ( tests, NULL, NULL );
}