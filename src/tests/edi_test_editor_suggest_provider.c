#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "editor/edi_editor_suggest_provider.c"

#include "edi_suite.h"

START_TEST (edi_test_editor_suggest_provider_lookup)
{
   Edi_Editor editor;
   Edi_Editor_Suggest_Provider *provider;

   editor.mimetype = "text/x-csrc";
   provider = edi_editor_suggest_provider_get(&editor);

   ck_assert(provider);
   ck_assert_str_eq(provider->id, "c");
}
END_TEST

void edi_test_editor_suggest_provider(TCase *tc)
{
   tcase_add_test(tc, edi_test_editor_suggest_provider_lookup);
}

