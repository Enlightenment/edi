#ifndef EDI_WELCOME_H_
# define EDI_WELCOME_H_

#include <Elementary.h>

#include "Edi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines control the intro wizard and other supporting UI for Edi.
 */

/**
 * @brief UI management functions.
 * @defgroup UI Initialisation and management of the supporting Edi screens
 *
 * @{
 *
 */

/**
 * Initialize a new Edi welcome window and display it.
 *
 * @return The welcome window that is created
 *
 * @ingroup UI
 */
Evas_Object *edi_welcome_show();


/**
 * Initialize a new Edi about window and display it.
 *
 * @return The about window that is created
 *
 * @ingroup UI
 */
Evas_Object *edi_about_show(Evas_Object *mainwin);

/**
 * Initialize a new Edi settings window and display it.
 *
 * @return The settings window that is created
 *
 * @ingroup UI
 */
Evas_Object *edi_settings_show(Evas_Object *mainwin);

void edi_settings_font_add(Evas_Object *parent);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* EDI_CONSOLEPANEL_H_ */
