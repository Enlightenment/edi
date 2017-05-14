#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "edi_language_provider.h"

#include "edi_config.h"

#include "edi_private.h"

#include "edi_language_provider_c.c"

static Edi_Language_Provider _edi_language_provider_registry[] =
{
   {
      "c", _edi_language_c_add, _edi_language_c_refresh, _edi_language_c_del,
      _edi_language_c_mime_name, _edi_language_c_snippet_get,
      _edi_language_c_lookup, _edi_language_c_lookup_doc
   },

   {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

Edi_Language_Provider *edi_language_provider_get(Edi_Editor *editor)
{
   Edi_Language_Provider *provider;
   const char *mime = editor->mimetype;
   const char *id = NULL;

   if (!mime)
     return NULL;

   if (!strcasecmp(mime, "text/x-chdr") || !strcasecmp(mime, "text/x-csrc")
       || !strcasecmp(mime, "text/x-modelica"))
     id = "c";

   if (!id)
     return NULL;

   provider = _edi_language_provider_registry;
   while (provider != NULL && provider->id != NULL)
     {
        if (!strncmp(id, provider->id, strlen(provider->id)))
          return provider;

        provider++;
     }

   return NULL;
}

Eina_Bool
edi_language_provider_has(Edi_Editor *editor)
{
   return !!edi_language_provider_get(editor);
}

void
edi_language_suggest_item_free(Edi_Language_Suggest_Item *item)
{
   free((char *)item->summary);
   free((char *)item->detail);

   free(item);
}

void
edi_language_doc_free(Edi_Language_Document *doc)
{
   if (!doc) return;

   eina_strbuf_free(doc->title);
   eina_strbuf_free(doc->detail);
   eina_strbuf_free(doc->param);
   eina_strbuf_free(doc->ret);
   eina_strbuf_free(doc->see);
}

