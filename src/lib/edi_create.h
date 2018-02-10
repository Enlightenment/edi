#ifndef EDI_CREATE_H_
# define EDI_CREATE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for creating new projects.
 */

typedef void (*Edi_Create_Cb)(const char *path, Eina_Bool success);

/**
 * @brief Main builder management
 * @defgroup Creation
 *
 * @{
 *
 * Functions of project creation from skeletons.
 *
 */

/**
 * Create a new project from an Edi template.
 *
 * @ingroup Creation
 */
EAPI void
edi_create_project(const char *template_name, const char *parentdir,
                   const char *name, const char *url, const char *user,
                   const char *email, Edi_Create_Cb func);

/**
 * Create a new project from an example.
 *
 * @ingroup Creation
 */
EAPI void
edi_create_example(const char *example_name, const char *parentdir,
                   const char *name, Edi_Create_Cb func);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* EDI_CREATE_H_ */
