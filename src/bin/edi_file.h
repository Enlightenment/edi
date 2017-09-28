#ifndef __EDI_FILE_H__
#define __EDI_FILE_H__

#include <Elementary.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines used for managing Edi file actions.
 */

/**
 * @brief UI management functions.
 * @defgroup UI
 *
 * @{
 *
 * Management of file/dir actions. 
 *
 */

/**
 * Check the path is not hidden according to project rules.
 * 
 * @param path The file path to check.
 * @ingroup Lookup
 */

Eina_Bool edi_file_path_hidden(const char *path);

/**
 * Replace all occurences of text within whole project.
 *
 * @param search The text to be replaced.
 * @param replace The text that will replace.
 *
 * @ingroup Lookup
 */
void edi_file_text_replace_all(const char *search, const char *replace);

/**
 * Replace all occurences of text within given file.
 *
 * @param path The path of the file to replace all occurences of the text.
 * @param search The text to be replaced.
 * @param replace The text that will replace.
 *
 * @ingroup Lookup
 */
void edi_file_text_replace(const char *path, const char *search, const char *replace);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif



#endif
