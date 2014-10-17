#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "edi_suite.h"

#include "edi_consolepanel.c"

START_TEST (edi_console_icons)
{
   ck_assert_str_eq(_edi_consolepanel_icon_for_line(" line 5: error: testing"), "error");
   ck_assert_str_eq(_edi_consolepanel_icon_for_line(" line 5: fatal error: testing"), "error");
   ck_assert_str_eq(_edi_consolepanel_icon_for_line(" line 5: warning: testing"), "important");
   ck_assert_str_eq(_edi_consolepanel_icon_for_line(" line 5: note: testing"), "info");
}
END_TEST

START_TEST (edi_console_parse)
{
   ck_assert(_startswith_location("test:1:1 error"));
   ck_assert(!_startswith_location("test:56-3 false"));
   ck_assert(!_startswith_location("test:1:1nospace"));
   ck_assert(!_startswith_location("test:a:b testing"));
   ck_assert(!_startswith_location("/test:1:1 absolute"));
}
END_TEST

void edi_test_console(TCase *tc)
{
   tcase_add_test(tc, edi_console_icons);
   tcase_add_test(tc, edi_console_parse);
}

