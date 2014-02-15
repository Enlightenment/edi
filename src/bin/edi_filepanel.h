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

 typedef void (*edi_filepanel_item_clicked_cb)(const char *path);

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
 * @param win The window into which the panel will be loaded.
 *
 * @ingroup UI
 */
void edi_filepanel_add(Evas_Object *parent,
                       edi_filepanel_item_clicked_cb cb);

/**
 * @}
 */



#ifdef __cplusplus
}
#endif

#endif /* EDI_FILEPANEL_H_ */
