#ifndef EDI_EDITOR_SUGGEST_PROVIDER_H_
# define EDI_EDITOR_SUGGEST_PROVIDER_H_

#include "editor/edi_editor.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for managing content suggestion providers.
 * i.e. like autosuggest in visual studio.
 */

/**
 * @typedef Edi_Editor_Suggest_Item
 * A handle for passing a suggest item to the ui and back
 */
typedef struct _Edi_Language_Suggest_Item
{
   const char *summary;
   const char *detail;
} Edi_Language_Suggest_Item;

typedef struct _Edi_Language_Document
{
   Eina_Strbuf *title;
   Eina_Strbuf *detail;
   Eina_Strbuf *param;
   Eina_Strbuf *ret;
   Eina_Strbuf *see;
} Edi_Language_Document;
/**
 * @struct Edi_Editor_Suggest_Provider
 * A description of the requirements for a suggestion provider.
 * This handles the set up and tear down of a provider as well as the lookup and
 * description lookup functions
 */
typedef struct _Edi_Language_Provider
{
   const char *id;

   void (*add)(Edi_Editor *editor);
   void (*refresh)(Edi_Editor *editor);
   void (*del)(Edi_Editor *editor);
   const char *(*mime_name)(const char *mime);
   const char *(*snippet_get)(const char *key);
   Eina_List *(*lookup)(Edi_Editor *editor, unsigned int row, unsigned int col);
   Edi_Language_Document *(*lookup_doc)(Edi_Editor *editor, unsigned int row, unsigned int col);
} Edi_Language_Provider;

/**
 * @brief Lookup information in suggest provider registry.
 * @defgroup Lookup
 *
 * @{
 *
 * Look up a suggest provider based on the provided editor.
 *
 * @param editor the editor session for a file you wish to get a suggestion provider for
 *
 * @return an Edi_Editor_Suggest_Provider if one is registered or NULL otherwise
 *
 * @ingroup Lookup
 */
Edi_Language_Provider *edi_language_provider_get(Edi_Editor *editor);

/**
 * Query whether a suggest provider is available for the specified editor session.
 *
 * @param editor the editor session for a file you wish to get a suggestion provider for
 *
 * @ingroup Lookup
 */
Eina_Bool edi_language_provider_has(Edi_Editor *editor);

/**
 * Free a suggest item.
 *
 * @param item the suggest item to free
 *
 * @ingroup Lookup
 */
void edi_language_suggest_item_free(Edi_Language_Suggest_Item *item);

/**
 * Free a suggest document.
 *
 * @param doc the suggest document to free
 *
 * @ingroup Lookup
 */
void edi_language_doc_free(Edi_Language_Document *doc);

/**
 * @}
 */



#ifdef __cplusplus
}
#endif

#endif /* EDI_EDITOR_SUGGEST_PROVIDER_H_ */
