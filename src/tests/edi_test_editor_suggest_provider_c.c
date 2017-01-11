#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Elementary.h>

#include "../bin/edi_private.h"
#include "editor/edi_editor_suggest_provider.h"

#include "edi_suite.h"

static Elm_Code *
_setup(Edi_Editor *editor, Evas_Object *win)
{
   Elm_Code *code;
   Elm_Code_Widget *widget;

   editor->mimetype = "text/x-csrc";

   code = elm_code_create();
   elm_code_file_open(code, PACKAGE_TESTS_DIR "test.c");

   widget = elm_code_widget_add(win, code);
   editor->entry = widget;
   elm_code_widget_cursor_position_set(widget, 3, 12);
   elm_code_widget_font_set(widget, "Mono", 14);

   return code;
}

static Eina_List *
_filtered_list_get(Eina_List *list, const char *word)
{
   Edi_Editor_Suggest_Item *suggest_it;
   Eina_List *l, *ret = NULL;

   EINA_LIST_FOREACH(list, l, suggest_it)
     {
        if (eina_str_has_prefix(suggest_it->summary, word))
          ret = eina_list_append(ret, suggest_it);
     }

   return ret;
}

START_TEST (edi_test_editor_suggest_provider_c_lookup)
{
   Elm_Code *code;
   Evas_Object *win;
   Edi_Editor editor;
   Edi_Editor_Suggest_Provider *provider;
   Edi_Editor_Suggest_Item *item;
   Eina_List *list;

   elm_init(1, NULL);
   win = elm_win_add(NULL, "entry", ELM_WIN_BASIC);
   code = _setup(&editor, win);

   provider = edi_editor_suggest_provider_get(&editor);
   provider->add(&editor);
   list = provider->lookup(&editor, 3, 12);
   ck_assert_int_eq(eina_list_count(_filtered_list_get(list, "_xyzabc_")), 1);

   EINA_LIST_FREE(list, item)
     edi_editor_suggest_item_free(item);

   provider->del(&editor);
   elm_code_free(code);
   elm_shutdown();
}
END_TEST

START_TEST (edi_test_editor_suggest_provider_c_summary)
{
   Elm_Code *code;
   Evas_Object *win;
   Edi_Editor editor;
   Edi_Editor_Suggest_Provider *provider;
   Edi_Editor_Suggest_Item *item;
   Eina_List *list;

   elm_init(1, NULL);
   win = elm_win_add(NULL, "entry", ELM_WIN_BASIC);
   code = _setup(&editor, win);

   provider = edi_editor_suggest_provider_get(&editor);
   provider->add(&editor);
   list = provider->lookup(&editor, 3, 12);
   item = eina_list_nth(_filtered_list_get(list, "_xyzabc_"), 0);
   ck_assert_str_eq(item->summary, "_xyzabc_test");

   EINA_LIST_FREE(list, item)
     edi_editor_suggest_item_free(item);

   provider->del(&editor);
   elm_code_free(code);
   elm_shutdown();
}
END_TEST

void edi_test_editor_suggest_provider_c(TCase *tc)
{
#if HAVE_LIBCLANG
   tcase_add_test(tc, edi_test_editor_suggest_provider_c_lookup);
   tcase_add_test(tc, edi_test_editor_suggest_provider_c_summary);
#endif
}

