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
 * Initialise a new Edi welcome window and display it.
 *
 * @return The welcome window that is created
 *
 * @ingroup UI
 */
Evas_Object *edi_welcome_show();

/**
 * Initialise a new Edi welcome screen and open it on the create project panel.
 *
 * @return The welcome screen window that is created
 *
 * @ingroup UI
 */
Evas_Object *edi_welcome_create_show();

/**
 * Initialise a new Edi about window and display it.
 *
 * @return The about window that is created
 *
 * @ingroup UI
 */
Evas_Object *edi_about_show(Evas_Object *mainwin);

/**
 * Initialise a new Edi settings window and display it.
 *
 * @return The settings window that is created
 *
 * @ingroup UI
 */
Evas_Object *edi_settings_show(Evas_Object *mainwin);

void edi_settings_font_add(Evas_Object *parent);

/**
 * Create a a confirmation dialogue and add it to the parent obj.
 *
 * @param parent The parent object to display the dialogue in.
 * @param message The generic message to display in the dialogue.
 * @param confirm_cb Function to execute upon confirmation.
 * @param data Data to pass to the confirm_cb callback.
 *
 * @ingroup UI
 */
void edi_screens_message_confirm(Evas_Object *parent, const char *message, void ((*confirm_cb)(void *)), void *data);

/**
 * Create an information dialogue and add it to the parent obj.
 *
 * @param parent The parent object to display the dialogue in.
 * @param title The title for the popup.
 * @param message The text to be displayed in the popup.
 *
 * @ingroup UI
 */
void edi_screens_message(Evas_Object *parent, const char *title, const char *message);

/**
 * Send a desktop notification message to the window manager.
 *
 * @param title The title for the notification.
 * @param message The text to be displayed in the desktop notification.
 *
 * @ingroup UI
 */
void edi_screens_desktop_notify(const char *title, const char *message);

/**
 * SCM binary is not installed, show dialogue.
 *
 * @param parent The object into which the UI will load.
 * @param binary The name of the missing binary.
 *
 * @ingroup Scm
 */
void edi_screens_scm_binary_missing(Evas_Object *parent, const char *binary);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* EDI_SCREENS_H_ */
