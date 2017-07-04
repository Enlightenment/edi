#ifndef EDI_BUILD_PROVIDER_H_
# define EDI_BUILD_PROVIDER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for managing information about supported build environments.
 */

typedef struct _Edi_Build_Provider
{
   const char *id;

   Eina_Bool (*path_supported_is)(const char *path);
   Eina_Bool (*file_hidden_is)(const char *path);
   Eina_Bool (*project_runnable_is)(const char *path);

   void (*build)(void);
   void (*test)(void);
   void (*run)(const char *path, const char *args);
   void (*clean)(void);
} Edi_Build_Provider;

/**
 * @brief Lookup information in build provider registry.
 * @defgroup Lookup
 *
 * @{
 *
 * Looking up build providers based on project tree or id etc.
 *
 */

/**
 * Look up a build provider based on a project tree.
 *
 * @param path the root of a project tree to find the build provider for.
 *
 * @return an Edi_Build_Provider if one is registered for the type of project or NULL otherwise
 *
 * @ingroup Lookup
 */
EAPI Edi_Build_Provider *edi_build_provider_for_project_path_get(const char *path);

/**
 * Look up a build provider based on the current project tree.
 *
 * @return an Edi_Build_Provider if one is registered for the current type of project or NULL otherwise
 *
 * @ingroup Lookup
 */
EAPI Edi_Build_Provider *edi_build_provider_for_project_get();

/**
 * Look up a build provider based on a provider id.
 * This is useful for overriding project tree based lookup.
 *
 * @param id the id of a provider you wish to get a handle for.
 *
 * @return an Edi_Build_Provider if one is registered or NULL otherwise
 *
 * @ingroup Lookup
 */
EAPI Edi_Build_Provider *edi_build_provider_for_id_get(const char *id);

/**
 * @}
 */



#ifdef __cplusplus
}
#endif

#endif /* EDI_BUILD_PROVIDER_H_ */
