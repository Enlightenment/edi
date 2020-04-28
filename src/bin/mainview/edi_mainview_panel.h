#ifndef EDI_MAINVIEW_PANEL_H_
# define EDI_MAINVIEW_PANEL_H_

#include <Elementary.h>
#include <Evas.h>

#include "Edi.h"

#include "mainview/edi_mainview_item.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for managing content panels within the
 *   main area of the Edi interface.
 */


/**
 * @struct _Edi_Mainview_Panel
 * A panel within mainview, holds many mainview items.
 */
typedef struct _Edi_Mainview_Panel
{
   Eina_List *items;

   Edi_Mainview_Item *current;
   Evas_Object *box, *scroll, *tabs, *content, *welcome, *tb, *sep;
} Edi_Mainview_Panel;

/**
 * @brief UI management functions.
 * @defgroup UI
 *
 * @{
 *
 * Initialisation and management of UI panels.
 *
 */

/**
 * Initialise a new Edi main view panel inside the parent container.
 *
 * @param parent The mainview parent into which the panel will be loaded.
 * @return the panel that represents tha added mainview panel.
 *
 * @ingroup UI
 */
Edi_Mainview_Panel *edi_mainview_panel_add(Evas_Object *parent);

/**
 * Free the panel and any related objects.
 *
 * @param The panel to free.
 *
 * @ingroup UI
 */
void edi_mainview_panel_free(Edi_Mainview_Panel *panel);

/**
 * Close mainview by path.
 *
 * @param panel the mainview panel context
 * @param path The path of file to close.
 *
 * @ingroup UI
 */
void edi_mainview_panel_item_close_path(Edi_Mainview_Panel *panel, const char *path);

/**
 * Select mainview by path.
 *
 * @param panel the mainview panel context
 * @param path The path of file to select.
 * @return bool On success.
 *
 * @ingroup UI
 */
Eina_Bool edi_mainview_panel_item_select_path(Edi_Mainview_Panel *panel, const char *path);

/**
 * Close all existing open files.
 *
 * @param panel the mainview panel context
 *
 * @ingroup UI
 */
void edi_mainview_panel_close_all(Edi_Mainview_Panel *panel);

/**
 * Open the file at path for editing within this panel.
 * Supported types are "text" and "image".
 *
 * @param panel the mainview panel context
 * @param path The absolute path of the file to open.
 *
 * @ingroup UI
 */
void edi_mainview_panel_open_path(Edi_Mainview_Panel *panel, const char *path);

/**
 * Open the file described in the provided options in this panel
 *   - path and location etc.
 *
 * @param panel the mainview panel context
 * @param path The path and options of the file to open.
 *
 * @ingroup UI
 */
void edi_mainview_panel_open(Edi_Mainview_Panel *panel, Edi_Path_Options *options);

/**
 * @}
 *
 *
 * @brief Content management functions.
 * @defgroup Content
 *
 * @{
 *
 * Opening and managing content within the view.
 *
 */

/**
 * Refresh all existing open files.
 *
 * @param panel the mainview panel context
 *
 * @ingroup Content
 */
void edi_mainview_panel_refresh_all(Edi_Mainview_Panel *panel);

void edi_mainview_panel_item_close(Edi_Mainview_Panel *panel, Edi_Mainview_Item *item);

void edi_mainview_panel_item_select(Edi_Mainview_Panel *panel, Edi_Mainview_Item *item);

/**
 * Select the previous open tab.
 * Previous means the next tab left, if there is one.
 *
 * @param panel the mainview panel context
 *
 * @ingroup Tabs
 */
void edi_mainview_panel_item_prev(Edi_Mainview_Panel *panel);

/**
 * Select the next open tab.
 * Next means the next tab to the right, if there is one.
 *
 * @param panel the mainview panel context
 *
 * @ingroup Tabs
 */
void edi_mainview_panel_item_next(Edi_Mainview_Panel *panel);

void edi_mainview_panel_tab_select(Edi_Mainview_Panel *panel, unsigned int id);

Eina_Bool edi_mainview_panel_item_contains(Edi_Mainview_Panel *panel, Edi_Mainview_Item *item);

Edi_Mainview_Item *edi_mainview_panel_item_current_get(Edi_Mainview_Panel *panel);

/**
 * Save the current file.
 *
 * @param panel the mainview panel context
 *
 * @ingroup Content
 */
void edi_mainview_panel_save(Edi_Mainview_Panel *panel);

/**
 * Move the current tab to a new window.
 *
 * @param panel the mainview panel context
 *
 * @ingroup Content
 */
void edi_mainview_panel_new_window(Edi_Mainview_Panel *panel);

/**
 * Close the current file.
 *
 * @param panel the mainview panel context
 *
 * @ingroup Content
 */
void edi_mainview_panel_close(Edi_Mainview_Panel *panel);

/**
 * Close all open files.
 *
 * @param panel the mainview panel context
 *
 * @ingroup Content
 */
void edi_mainview_panel_closeall(Edi_Mainview_Panel *panel);

/**
 * Undo the most recent change in the current view.
 *
 * @param panel the mainview panel context
 *
 * @ingroup Content
 */
void edi_mainview_panel_undo(Edi_Mainview_Panel *panel);

/**
 * Return if editor has been modified
 *
 * @param panel the mainview panel context
 *
 * @ingroup Content
 */
Eina_Bool edi_mainview_panel_modified(Edi_Mainview_Panel *panel);

/**
 * See whether the current view can undo a change.
 *
 * @param panel the mainview panel context
 *
 * @ingroup Content
 */
Eina_Bool edi_mainview_panel_can_undo(Edi_Mainview_Panel *panel);

/**
 * Redo the most recent change in the current view.
 *
 * @param panel the mainview panel context
 *
 * @ingroup Content
 */
void edi_mainview_panel_redo(Edi_Mainview_Panel *panel);

/**
 * See whether the current view can redo a change.
 *
 * @param panel the mainview panel context
 *
 * @ingroup Content
 */
Eina_Bool edi_mainview_panel_can_redo(Edi_Mainview_Panel *panel);

/**
 * Cut the current selection into the clipboard.
 *
 * @param panel the mainview panel context
 *
* @ingroup Content
 */
void edi_mainview_panel_cut(Edi_Mainview_Panel *panel);

/**
 * Copy the current selection into the clipboard.
 *
 * @param panel the mainview panel context
 *
 * @ingroup Content
 */
void edi_mainview_panel_copy(Edi_Mainview_Panel *panel);

/**
 * Paste the current clipboard contents at the current cursor position.
 *
 * @param panel the mainview panel context
 *
 * @ingroup Content
 */
void edi_mainview_panel_paste(Edi_Mainview_Panel *panel);

/**
 * Search the current view's contents.
 *
 * @param panel the mainview panel context
 *
 * @ingroup Content
 */
void edi_mainview_panel_search(Edi_Mainview_Panel *panel);

/**
 * Go to a requested line in the panel's current view contents.
 *
 * @param panel the mainview panel context
 * @param line the line number (1 based) to scroll to
 *
 * @ingroup Content
 */
void edi_mainview_panel_goto(Edi_Mainview_Panel *panel, unsigned int line);

/**
 * Go to a requested line, column position in the panel's current view contents.
 *
 * @param panel the mainview panel context
 * @param row the line number (1 based) to scroll to
 * @param col the column position (1 based) to scroll to
 *
 * @ingroup Content
 */
void edi_mainview_panel_goto_position(Edi_Mainview_Panel *panel, unsigned int row, unsigned int col);

/**
 * Present a popup that will initiate a goto in the current panel view.
 *
 * @param panel the mainview panel context
 *
 * @ingroup Content
 */
void edi_mainview_panel_goto_popup_show();

/**
 * Return number of items in panel.
 * @param panel the mainview panel context
 *
 * @ingroup Content
 */
unsigned int edi_mainview_panel_item_count(Edi_Mainview_Panel *panel);

/**
 * Go to the beginning of the file in the panel's editor view.
 *
 * @param panel the mainview panel context
 *
 * @ingroup Content
 */
void edi_mainview_panel_goto_start(Edi_Mainview_Panel *panel);

/**
 * Go to the end of the file in the panel's editor view.
 *
 * @param panel the mainview panel context
 *
 * @ingroup Content
 */
void edi_mainview_panel_goto_end(Edi_Mainview_Panel *panel);

/**
 * @}
 *
 */

#ifdef __cplusplus
}
#endif

#endif /* EDI_MAINVIEW_PANEL_H_ */
