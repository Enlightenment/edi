#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "edi_suite.h"

START_TEST (edi_path_parse_plain_options)
{
   Edi_Path_Options *options;

   options = edi_path_options_create("test");

   ck_assert_str_eq(options->path, "test");
   ck_assert(options->line == 0);
   ck_assert(options->character == 0);
}
END_TEST

START_TEST (edi_path_parse_line_options)
{
   Edi_Path_Options *options;

   options = edi_path_options_create("test:5:5");

   ck_assert_str_eq(options->path, "test");
   ck_assert(options->line == 5);
   ck_assert(options->character == 5);
}
END_TEST

void edi_test_path(TCase *tc)
{
   tcase_add_test(tc, edi_path_parse_plain_options);
   tcase_add_test(tc, edi_path_parse_line_options);
}

