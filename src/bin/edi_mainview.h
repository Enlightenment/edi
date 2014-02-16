#ifndef EDI_MAINVIEW_H_
# define EDI_MAINVIEW_H_

#include <Elementary.h>

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
 * Initialize a new Edi main view and add it to the parent panel.
 *
 * @param parent The parent into which the panel will be loaded.
 *
 * @ingroup UI
 */
EAPI void edi_mainview_add(Evas_Object *parent);

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
 * Open the file at path for editing.
 *
 * @param path The absolute path of the file to open.
 *
 * @ingroup Content
 */
EAPI void edi_mainview_open_path(const char *path);

/**
 * Save the current file.
 *
 * @ingroup Content
 */
EAPI void edi_mainview_save();

/**
 * Close the current file.
 *
 * @ingroup Content
 */
EAPI void edi_mainview_close();

/**
 * Cut the current selection into the clipboard.
 *
 * @ingroup Content
 */
EAPI void edi_mainview_cut();

/**
 * Copy the current selection into the clipboard.
 *
 * @ingroup Content
 */
EAPI void edi_mainview_copy();

/**
 * Paste the current clipboard contents at the current cursor position.
 *
 * @ingroup Content
 */
EAPI void edi_mainview_paste();

/**
 * @}
 *
 */

#ifdef __cplusplus
}
#endif

#endif /* EDI_MAINVIEW_H_ */
