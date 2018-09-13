#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "edi_content.h"
#include "edi_content_provider.h"
#include "editor/edi_editor.h"

#include "edi_config.h"

#include "language/edi_language_provider.h"

#include "edi_private.h"

static Edi_Content_Provider _edi_content_provider_registry[] =
{
   {"text", "text-x-generic", EINA_TRUE, EINA_TRUE, edi_editor_add},
   {"code", "text-x-csrc", EINA_TRUE, EINA_TRUE, edi_editor_add},
   {"image", "image-x-generic", EINA_FALSE, EINA_FALSE, edi_content_image_add},
   {"diff", "text-x-source", EINA_TRUE, EINA_FALSE, edi_content_diff_add},

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
