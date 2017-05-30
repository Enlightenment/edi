#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "edi_suite.h"

START_TEST (edi_exe_test_wait)
{
   edi_init();

   ck_assert(1 != edi_exe_wait("false"));
   ck_assert_int_eq(0, edi_exe_wait("true"));

   edi_shutdown();
}
END_TEST

void edi_test_exe(TCase *tc)
{
   tcase_add_test(tc, edi_exe_test_wait);
}

