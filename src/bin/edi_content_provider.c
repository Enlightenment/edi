#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Evas.h>
#include <Elementary.h>

#include "edi_content_provider.h"
#include "editor/edi_editor.h"

#include "edi_config.h"

#include "language/edi_language_provider.h"

#include "edi_private.h"

// TODO move out to edi_content.c or similar just like the editor type
// (and the Evas include)

static Eina_Bool
_edi_content_provider_diff_config_changed(void *data, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   Evas_Object *diff;

   diff = (Evas_Object*) data;
   elm_code_diff_widget_font_set(diff, _edi_project_config->font.name, _edi_project_config->font.size);

   return ECORE_CALLBACK_RENEW;
}

static Evas_Object *
_edi_content_provider_diff_add(Evas_Object *parent, Edi_Mainview_Item *item)
{
   Elm_Code *code;
   Evas_Object *diff;

   code = elm_code_create();
   elm_code_file_open(code, item->path);
   diff = elm_code_diff_widget_add(parent, code);
   elm_code_diff_widget_font_set(diff, _edi_project_config->font.name, _edi_project_config->font.size);

   ecore_event_handler_add(EDI_EVENT_CONFIG_CHANGED, _edi_content_provider_diff_config_changed, diff);
   return diff;
}

static Edi_Content_Provider _edi_content_provider_registry[] =
{
   {"text", "text-x-generic", EINA_TRUE, EINA_TRUE, edi_editor_add},
   {"code", "text-x-csrc", EINA_TRUE, EINA_TRUE, edi_editor_add},
   {"image", "image-x-generic", EINA_FALSE, EINA_FALSE, edi_editor_image_add},
   {"diff", "text-x-source", EINA_TRUE, EINA_FALSE, _edi_content_provider_diff_add},

   {NULL, NULL, EINA_FALSE, EINA_FALSE, NULL}
};

Edi_Content_Provider *edi_content_provider_for_mime_get(const char *mime)
{
   const char *id = NULL;
   Edi_Language_Provider *provider;

   if (!mime)
     return NULL;

   provider = edi_language_provider_for_mime_get(mime);

   if (!!provider)
     id = "code";
   else if (!strcasecmp(mime, "text/plain") || !strcasecmp(mime, "application/x-shellscript"))
     id = "text";
   else if (!strncasecmp(mime, "image/", 6))
     id = "image";
   else if (!strcasecmp(mime, "text/x-diff") || !strcasecmp(mime, "text/x-patch"))
     id = "diff";
   else
     {
        id = _edi_config_mime_search(mime);
        if (!id)
          {
             if (!strncasecmp(mime, "text/", 5))
               id = "text";
             else
               return NULL;
          }
     }

   return edi_content_provider_for_id_get(id);
}

Edi_Content_Provider *edi_content_provider_for_id_get(const char *id)
{
   Edi_Content_Provider *provider;

   provider = _edi_content_provider_registry;
   while (provider != NULL && provider->id != NULL)
     {
        if (!strncmp(id, provider->id, strlen(id)))
          return provider;

        provider++;
     }

   return NULL;
}
