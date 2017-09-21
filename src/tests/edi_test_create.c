#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "edi_suite.h"

#include "edi_create.c"

START_TEST (edi_test_create_escape_quotes)
{
   const char *in = "hallowe'en";

   char *out = edi_create_escape_quotes(in);

   ck_assert_str_eq(out, "hallowe'\\\"'\\\"'en");

   free(out);
}
END_TEST

START_TEST (edi_test_create_escape_multiple_quotes)
{
   const char *in = "we'ven't";

   char *out = edi_create_escape_quotes(in);

   ck_assert_str_eq(out, "we'\\\"'\\\"'ven'\\\"'\\\"'t");

   free(out);
}
END_TEST

void edi_test_create(TCase *tc)
{
   tcase_add_test(tc, edi_test_create_escape_quotes);
   tcase_add_test(tc, edi_test_create_escape_multiple_quotes);
}

