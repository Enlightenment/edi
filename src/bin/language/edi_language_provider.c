#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "edi_language_provider.h"

#include "edi_config.h"

#include "edi_private.h"

#include "edi_language_provider_c.c"
#include "edi_language_provider_python.c"
#include "edi_language_provider_rust.c"
#include "edi_language_provider_go.c"
#include "edi_language_provider_csharp.c"

static Edi_Language_Provider _edi_language_provider_registry[] =
{
   {
      "c", _edi_language_c_add, _edi_language_c_refresh, _edi_language_c_del,
      _edi_language_c_mime_name, _edi_language_c_snippet_get,
      _edi_language_c_lookup, _edi_language_c_lookup_doc
   },
   {
      "python", _edi_language_python_add, _edi_language_python_refresh, _edi_language_python_del,
      _edi_language_python_mime_name, _edi_language_python_snippet_get,
      NULL, NULL
   },
   {
      "rust", _edi_language_rust_add, _edi_language_rust_refresh, _edi_language_rust_del,
      _edi_language_rust_mime_name, _edi_language_rust_snippet_get,
      NULL, NULL
   },
   {
      "go", _edi_language_go_add, _edi_language_go_refresh, _edi_language_go_del,
      _edi_language_go_mime_name, _edi_language_go_snippet_get,
      NULL, NULL
   },
   {
      "csharp", _edi_language_csharp_add, _edi_language_csharp_refresh, _edi_language_csharp_del,
      _edi_language_csharp_mime_name, _edi_language_csharp_snippet_get,
      NULL, NULL
   },


   {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

Edi_Language_Provider *edi_language_provider_get(Edi_Editor *editor)
{
   const char *mime = editor->mimetype;

   return edi_language_provider_for_mime_get(mime);
}

Edi_Language_Provider *edi_language_provider_for_mime_get(const char *mime)
{
   Edi_Language_Provider *provider;
   const char *id = NULL;

   if (!mime)
     return NULL;

   if (!strcasecmp(mime, "text/x-chdr") || !strcasecmp(mime, "text/x-csrc"))
     id = "c";
   if (!strcasecmp(mime, "text/rust"))
     id = "rust";
   if (!strcasecmp(mime, "text/x-python") || !strcasecmp(mime, "text/x-python3"))
     id = "python";
   if (!strcasecmp(mime, "text/x-go"))
     id = "go";
   if (!strcasecmp(mime, "text/x-csharp"))
     id = "csharp";

   if (!id)
     return NULL;

   provider = _edi_language_provider_registry;
   while (provider != NULL && provider->id != NULL)
     {
        if (!strcmp(id, provider->id))
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

