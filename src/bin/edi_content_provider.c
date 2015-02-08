#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Evas.h>
#include <Elm_Code.h>

#include "edi_content_provider.h"
#include "editor/edi_editor.h"

#include "edi_config.h"

#include "edi_private.h"

// TODO move out to edi_content.c ot similar just like the editor type
// (and the Evas include)

static Evas_Object *
_edi_content_provider_image_add(Evas_Object *parent, Edi_Mainview_Item *item)
{
   Evas_Object *img, *scroll;

   scroll = elm_scroller_add(parent);
   evas_object_size_hint_weight_set(scroll, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(scroll, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(scroll);
   img = elm_image_add(scroll);
   elm_image_file_set(img, item->path, NULL);
   elm_image_no_scale_set(img, EINA_TRUE);
   elm_object_content_set(scroll, img);
   evas_object_show(img);

   return scroll;
}

static Evas_Object *
_edi_content_provider_diff_add(Evas_Object *parent, Edi_Mainview_Item *item)
{
   Elm_Code *code;
   Evas_Object *diff;

   code = elm_code_create();
   elm_code_file_open(code, item->path);
   diff = elm_code_diff_widget_add(parent, code);
   elm_code_diff_widget_font_size_set(diff, 12);

   return diff;
}

static Edi_Content_Provider _edi_content_provider_registry[] =
{
   {"text", EINA_TRUE, EINA_TRUE, edi_editor_add},
   {"code", EINA_TRUE, EINA_TRUE, edi_editor_add},
   {"image", EINA_FALSE, EINA_FALSE, _edi_content_provider_image_add},
   {"diff", EINA_TRUE, EINA_FALSE, _edi_content_provider_diff_add},

   {NULL, EINA_FALSE, EINA_FALSE, NULL}
};

Edi_Content_Provider *edi_content_provider_for_mime_get(const char *mime)
{
   const char *id = NULL;

   if (!mime)
     return NULL;

   if (!strcasecmp(mime, "text/plain") || !strcasecmp(mime, "application/x-shellscript"))
     id = "text";
   else if (!strcasecmp(mime, "text/x-chdr") || !strcasecmp(mime, "text/x-csrc")
            || !strcasecmp(mime, "text/x-modelica"))
     id = "code";
   else if (!strcasecmp(mime, "text/x-c++src") || !strcasecmp(mime, "text/x-c++hdr"))
     id = "code";
   else if (!strncasecmp(mime, "image/", 6))
     id = "image";
   else if (!strcasecmp(mime, "text/x-diff") || !strcasecmp(mime, "text/x-patch"))
     id = "diff";
   else
     {
        id = _edi_config_mime_search(mime);

        if (!id)
          return NULL;
     }

   return edi_content_provider_for_id_get(id);
}

Edi_Content_Provider *edi_content_provider_for_id_get(const char *id)
{
   Edi_Content_Provider *provider;

   provider = _edi_content_provider_registry;
   while (provider != NULL && provider->id != NULL)
     {
        if (!strncmp(id, provider->id, strlen(provider->id)))
          return provider;

        provider++;
     }

   return NULL;
}
