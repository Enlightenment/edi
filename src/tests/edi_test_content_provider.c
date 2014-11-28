#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "edi_content_provider.c"

#include "edi_suite.h"

START_TEST (edi_test_content_provider_id_lookup)
{
   Edi_Content_Provider *provider;

   provider = edi_content_provider_for_id_get("text");

   ck_assert(provider);
   ck_assert_str_eq(provider->id, "text");
}
END_TEST

START_TEST (edi_test_content_provider_mime_lookup)
{
   Edi_Content_Provider *provider;

   provider = edi_content_provider_for_mime_get("text/plain");

   ck_assert(provider);
   ck_assert_str_eq(provider->id, "text");
}
END_TEST

void edi_test_content_provider(TCase *tc)
{
   tcase_add_test(tc, edi_test_content_provider_id_lookup);
   tcase_add_test(tc, edi_test_content_provider_mime_lookup);
}

