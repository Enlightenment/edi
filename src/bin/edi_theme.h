#ifndef __EDI_THEME_H__
#define __EDI_THEME_H__

#include <Eina.h>
#include <Evas.h>

typedef struct _Edi_Theme {
        char *name;
        char *path;
        char *title;
} Edi_Theme;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines used for managing Edi theme actions.
 */

/**
 * @brief Theme management functions.
 * @defgroup Theme
 *
 * @{
 *
 * Management of theming actions.
 *
 */

/**
 * Set the Edi theme by name on an Elm_Code_Widget.
 *
 * @param obj The Elm_Code_Widget object to apply the theme to.
 * @param name The name of the theme to apply.
 *
 * @ingroup Theme
 */
void edi_theme_elm_code_set(Evas_Object *obj, const char *name);

/**
 * Get a list of all themes available.
 *
 * @return a list of all available themes as Edi_Theme instances.
 *
 * @ingroup Theme
 */
Eina_List *edi_theme_themes_get(void);

/**
 * Get theme obj by its name.
 *
 * @return the theme obj matching the name.
 *
 * @ingroup Theme
 */
Edi_Theme *edi_theme_theme_by_name(const char *name);

void edi_theme_window_alpha_set(void);
void edi_theme_elm_code_alpha_set(Evas_Object *obj);
/**
 * @}
 */


#ifdef __cplusplus
}
#endif



#endif
