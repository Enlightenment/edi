#ifndef _UI_H
#define _UI_H

#include <Evas.h>

#if HAVE_LIBCLANG
#include <clang-c/Index.h>
#endif

#include "mainview/edi_mainview_item.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for interacting with the main Edi editor view.
 */

/**
 * @typedef Edi_Editor_Search
 * An instance of an editor search session.
 */
typedef struct _Edi_Editor_Search Edi_Editor_Search;

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
   Eina_List *undo_stack; /**< The list of operations that can be undone */

   /* Private */
   Evas_Object *lines;
   Edi_Editor_Search *search;

   /* Add new members here. */

#if HAVE_LIBCLANG
   /* Clang */
   CXIndex idx;
   CXTranslationUnit tx_unit;
   Evas_Textblock_Cursor *format_cursor;
#endif
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
 * @param item The item describing the file to be loaded in the editor.
 * @return the created evas object that contains the editor.
 *
 * @ingroup Editor
 */
EAPI Evas_Object *edi_editor_add(Evas_Object *parent, Edi_Mainview_Item *item);

/**
 * @}
 *
 * @brief Widgets.
 * @defgroup Widget Functions that open or manipulate feature panels
 *
 * @{
 *
 */

/**
 * Add a search widget to the specified editor.
 *
 * @param parent The panel that the UI should be added to,
 * @param editor The Edi_Editor instance that we are searching for text within.
 *
 * @ingroup Widgets
 */
EAPI void edi_editor_search_add(Evas_Object *parent, Edi_Editor *editor);

/**
 * Start a search in the specified editor.
 *
 * @param editor the text editor instance to search within.
 *
 * @ingroup Widgets
 */
EAPI void edi_editor_search(Edi_Editor *editor);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
