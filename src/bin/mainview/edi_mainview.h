#ifndef EDI_MAINVIEW_H_
# define EDI_MAINVIEW_H_

#include <Elementary.h>
#include <Evas.h>

#include "Edi.h"

#include "mainview/edi_mainview_item.h"
#include "mainview/edi_mainview_panel.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for managing the main area of the Edi interface.
 */

/**
 * @brief UI management functions.
 * @defgroup UI
 *
 * @{
 *
 * Initialisation and management of the main view UI
 *
 */

/**
 * Initialise a new Edi main view and add it to the parent panel.
 *
 * @param parent The parent into which the panel will be loaded.
 * @param win The window the panel will be using
 *
 * @ingroup UI
 */
void edi_mainview_add(Evas_Object *parent, Evas_Object *win);

/**
 * Close mainview by path.
 *
 * @praram path The path of file to close.
 *
 * @ingroup Content
 */
void edi_mainview_item_close_path(const char *path);

/**
 * Refresh all existing open files.
 *
 * @ingroup Content
 */
void edi_mainview_refresh_all(void);

/**
 * Close all existing open files.
 *
 * @ingroup Content
 */
void edi_mainview_close_all(void);

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
 * Open the file at path for editing using the type specified.
 * Supported types are "text" and "image".
 *
 * @param path The absolute path of the file to open.
 *
 * @ingroup Content
 */
void edi_mainview_open_path(const char *path);

/**
 * Open the file described in the provided options - path and location etc.
 *
 * @param path The path and options of the file to open.
 *
 * @ingroup Content
 */
void edi_mainview_open(Edi_Path_Options *options);

/**
 * Open the file at path for editing in a new window using the type specified.
 * Supported types are "text" and "image".
 * If the path is already open it will be moved to a new window.
 *
 * @param path The absolute path of the file to open.
 *
 * @ingroup Content
 */
void edi_mainview_open_window_path(const char *path);

/**
 * Open the file described in the provided options in a new window - path and location etc.
 *
 * @param path The path and options of the file to open.
 *
 * @ingroup Content
 */
void edi_mainview_open_window(Edi_Path_Options *options);

/**
 * Save the current file.
 *
 * @ingroup Content
 */
void edi_mainview_save();

/**
 * Move the current tab to a new window.
 *
 * @ingroup Content
 */
void edi_mainview_new_window();

/**
 * Close the current file.
 *
 * @ingroup Content
 */
void edi_mainview_close();

/**
 * Close all open files.
 *
 * @ingroup Content
 */
void edi_mainview_closeall();

/**
 * Undo the most recent change in the current view.
 *
 * @ingroup Content
 */
void edi_mainview_undo();

/**
 * Return if editor has been modified
 *
 * @ingroup Content
 */
Eina_Bool edi_mainview_modified();

/**
 * See whether the current view can undo a change.
 *
 * @ingroup Content
 */
Eina_Bool edi_mainview_can_undo();

/**
 * Redo the most recent change in the current view.
 *
 * @ingroup Content
 */
void edi_mainview_redo();

/**
 * See whether the current view can redo a change.
 *
 * @ingroup Content
 */
Eina_Bool edi_mainview_can_redo();

/**
 * Cut the current selection into the clipboard.
 *
 * @ingroup Content
 */
void edi_mainview_cut();

/**
 * Copy the current selection into the clipboard.
 *
 * @ingroup Content
 */
void edi_mainview_copy();

/**
 * Paste the current clipboard contents at the current cursor position.
 *
 * @ingroup Content
 */
void edi_mainview_paste();

/**
 * Search the current view's contents.
 *
 * @ingroup Content
 */
void edi_mainview_search();

/**
 * Go to a requested line in the current view's contents.
 *
 * @param line the line number (1 based) to scroll to
 *
 * @ingroup Content
 */
void edi_mainview_goto(unsigned int line);

/**
 * Go to a requested line, column position in the current view's contents.
 *
 * @param row the line number (1 based) to scroll to
 * @param col the column position (1 based) to scroll to
 *
 * @ingroup Content
 */
void edi_mainview_goto_position(unsigned int row, unsigned int col);

/**
 * Present a popup that will initiate a goto in the current view.
 *
 * @ingroup Content
 */
void edi_mainview_goto_popup_show();

/**
 * Present a popup that will initiate a project search.
 *
 * @ingroup Content
 */
void edi_mainview_project_search_popup_show();

/**
 * @}
 *
 *
 * @brief Tab management functions.
 * @defgroup Tabs
 *
 * @{
 *
 * Manipulating the open files within the application.
 *
 */

Edi_Mainview_Item *edi_mainview_item_current_get();

Edi_Mainview_Panel *edi_mainview_panel_current_get();

unsigned int edi_mainview_panel_index_get(Edi_Mainview_Panel *panel);

void edi_mainview_tab_select(unsigned int id);

/**
 * Select the passed item in the mainview UI.
 * By definition this will already be an open file as the Edi_Mainview_Item will
 * only exist for an open item.
 * If this is an external window it will raise that instead of selecting a tab.
 *
 * @ingroup Tabs
 */
void edi_mainview_item_select(Edi_Mainview_Item *item);

/**
 * Select the previous open tab.
 * Previous means the next tab left, if there is one.
 *
 * @ingroup Tabs
 */
void edi_mainview_item_prev();

/**
 * Select the next open tab.
 * Next means the next tab to the right, if there is one.
 *
 * @ingroup Tabs
 */
void edi_mainview_item_next();

Edi_Mainview_Panel *edi_mainview_panel_append();

Edi_Mainview_Panel *edi_mainview_panel_for_item_get(Edi_Mainview_Item *item);
Edi_Mainview_Panel *edi_mainview_panel_by_index(int index);
int edi_mainview_panel_count(void);
int edi_mainview_panel_id(Edi_Mainview_Panel *panel);
void edi_mainview_panel_focus(Edi_Mainview_Panel *panel);
void edi_mainview_panel_remove(Edi_Mainview_Panel *panel);
/**
 * @}
 *
 */

#ifdef __cplusplus
}
#endif

#endif /* EDI_MAINVIEW_H_ */
