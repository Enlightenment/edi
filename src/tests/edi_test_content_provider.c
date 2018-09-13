#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "edi_content_provider.c"
#include "language/edi_language_provider.c"

#include "edi_suite.h"

// Add some no-op methods here so linking works without having to import the whole UI!
EAPI Evas_Object *
edi_editor_add(Evas_Object *parent EINA_UNUSED, Edi_Mainview_Item *item EINA_UNUSED)
{
   return NULL;
}

EAPI Evas_Object *
edi_content_image_add(Evas_Object *parent EINA_UNUSED, Edi_Mainview_Item *item EINA_UNUSED)
{
   return NULL;
}

EAPI Evas_Object *
edi_content_diff_add(Evas_Object *parent EINA_UNUSED, Edi_Mainview_Item *item EINA_UNUSED)
{
   return NULL;
}

Edi_Config *_edi_config = NULL;
Edi_Project_Config *_edi_project_config = NULL;
int EDI_EVENT_CONFIG_CHANGED;

const char *
_edi_config_mime_search(const char *mime EINA_UNUSED)
{
   return NULL;
}
// end no-ops

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

static void
_edi_test_content_provider_type_assert(const char *mime, const char *type)
{
   Edi_Content_Provider *provider;

   provider = edi_content_provider_for_mime_get(mime);

   ck_assert(provider);
   ck_assert_str_eq(provider->id, type);
}

START_TEST (edi_test_content_provider_text_files)
{
   _edi_test_content_provider_type_assert("text/plain", "text");
   _edi_test_content_provider_type_assert("application/x-shellscript", "text");
}
END_TEST

START_TEST (edi_test_content_provider_c_files)
{
   _edi_test_content_provider_type_assert("text/x-csrc", "code");
   _edi_test_content_provider_type_assert("text/x-chdr", "code");
}
END_TEST
/*
START_TEST (edi_test_content_provider_cpp_files)
{
   _edi_test_content_provider_type_assert("text/x-c++src", "code");
   _edi_test_content_provider_type_assert("text/x-c++hdr", "code");
}
END_TEST
*/
void edi_test_content_provider(TCase *tc)
{
   tcase_add_test(tc, edi_test_content_provider_id_lookup);
   tcase_add_test(tc, edi_test_content_provider_mime_lookup);
   tcase_add_test(tc, edi_test_content_provider_text_files);
   tcase_add_test(tc, edi_test_content_provider_c_files);
//   tcase_add_test(tc, edi_test_content_provider_cpp_files);
}

