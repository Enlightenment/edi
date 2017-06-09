#ifndef EDI_PATH_H_
# define EDI_PATH_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for Edi path handling.
 */

typedef struct _Edi_Path_Options
{
   const char *path;
   const char *type;
   int line, character;
   Eina_Bool background;
} Edi_Path_Options;

/**
 * @brief Path options
 * @defgroup Options
 *
 * @{
 *
 * Manipulation of various path options.
 *
 */

/**
 * Create an options based on parsing a string input.
 * String will be of the format <path>[:<line>[:<character>]]
 *
 * @param input The string formatted to have a path with optional line and character parameters
 *   If only one of line or character is provided it's assumed to be the line number.
 *
 * @return A newly allocated options struct.
 *
 * @ingroup Options
 */
EAPI Edi_Path_Options *edi_path_options_create(const char *input);

/**
 * Append a file to the end of a given path.
 *
 * @param path The base path to append to
 * @param file The file portion to add to the path
 *
 * @return a newly allocated string that merges the items to a path using the
 *   correct separator for the current platform.
 */
EAPI char *edi_path_append(const char *path, const char *file);

/**
 * Find if a requested file exists within the specified directory.
 *
 * @param dir The directory relative to which we should check.
 * @param file The filename to check for existance.
 *
 * @return whether or not the requested file exists within the specified directory.
 *
 * @ingroup Main
 */
EAPI Eina_Bool edi_path_relative_exists(const char *dir, const char *file);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* EDI_PATH_H_ */
