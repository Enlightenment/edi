#ifndef _UI_H
#define _UI_H

#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for interacting with the main Edi editor view.
 */

/**
 * @typedef Edi_Editor
 * An instance of an editor view.
 */
typedef struct _Edi_Editor Edi_Editor;

/**
 * @struct _Edi_Editor
 * An instance of an editor view.
 */
struct _Edi_Editor
{
   Evas_Object *entry; /**< The main text entry widget for the editor */
   Evas_Object *lines; 
   Eina_List *undo_stack; /**< The list of operations that can be undone */

   /* Private */

   /* Add new members here. */
};

/**
 * @brief Editor.
 * @defgroup Editor The main text editor functions
 *
 * @{
 *
 */

/**
 * Initialize a new Edi editor and add it to the parent panel.
 *
 * @param parent The panel into which the panel will be loaded.
 * @param path The file path to be loaded in the editor.
 * @return the created evas object that contains the editor.
 *
 * @ingroup Editor
 */
EAPI Evas_Object *edi_editor_add(Evas_Object *parent, const char *path);

/**
 * @}
 *
 * @brief Dialogs.
 * @defgroup Dialogs Functions that open other dialogs
 *
 * @{
 *
 */

/**
 * Start a search in the specified editor.
 *
 * @param entry The elm_entry that we are searching for text within.
 *
 * @ingroup Dialogs
 */
EAPI void edi_editor_search(Evas_Object *entry);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
