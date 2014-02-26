#ifndef EDI_FILEPANEL_H_
# define EDI_FILEPANEL_H_

#include <Elementary.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for managing the Edi file panel.
 */

typedef void (*edi_filepanel_item_clicked_cb)(const char *path,
                                              const char *type);

/**
 * @brief UI management functions.
 * @defgroup UI
 *
 * @{
 *
 * Initialisation and management of the file panel UI
 *
 */

/**
 * Initialize a new Edi filepanel and add it to the parent panel.
 *
 * @param parent The panel into which the panel will be loaded.
 * @param win The main window of the application.
 * @param path The project path being opened.
 * @param cb A callback to inform the app a file should be opened.
 *
 * @ingroup UI
 */
void edi_filepanel_add(Evas_Object *parent, Evas_Object *win,
                       const char *path, edi_filepanel_item_clicked_cb cb);

/**
 * @}
 */



#ifdef __cplusplus
}
#endif

#endif /* EDI_FILEPANEL_H_ */
