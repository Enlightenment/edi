#ifndef EDI_SCM_H_
# define EDI_SCM_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for Edi SCM management.
 */

typedef enum {
   EDI_SCM_STATUS_NONE = 0,
   EDI_SCM_STATUS_ADDED,
   EDI_SCM_STATUS_DELETED,
   EDI_SCM_STATUS_MODIFIED,
   EDI_SCM_STATUS_RENAMED,
   EDI_SCM_STATUS_UNTRACKED,
   EDI_SCM_STATUS_ADDED_STAGED,
   EDI_SCM_STATUS_DELETED_STAGED,
   EDI_SCM_STATUS_MODIFIED_STAGED,
   EDI_SCM_STATUS_RENAMED_STAGED,
   EDI_SCM_STATUS_UNKNOWN,
} Edi_Scm_Status_Code;

typedef struct _Edi_Scm_Status
{
   Eina_Stringshare *path;
   Edi_Scm_Status_Code change;
   Eina_Bool staged;
} Edi_Scm_Status;

typedef int (scm_fn_add)(const char *path);
typedef int (scm_fn_mod)(const char *path);
typedef int (scm_fn_del)(const char *path);
typedef int (scm_fn_move)(const char *src, const char *dest);
typedef int (scm_fn_commit)(const char *message);
typedef int (scm_fn_status)(void);
typedef char *(scm_fn_diff)(void);
typedef int (scm_fn_push)(void);
typedef int (scm_fn_pull)(void);
typedef int (scm_fn_stash)(void);
typedef Edi_Scm_Status_Code (scm_fn_file_status)(const char *path);

typedef int (scm_fn_remote_add)(const char *remote_url);
typedef const char * (scm_fn_remote_name)(void);
typedef const char * (scm_fn_remote_email)(void);
typedef const char * (scm_fn_remote_url)(void);
typedef int (scm_fn_credentials)(const char *name, const char *email);
typedef Eina_List * (scm_fn_status_get)(void);

typedef struct _Edi_Scm_Engine
{
   const char     *name;
   const char     *directory;
   const char     *path;
   Eina_List      *statuses;

   scm_fn_add         *file_add;
   scm_fn_mod         *file_mod;
   scm_fn_del         *file_del;
   scm_fn_move        *move;
   scm_fn_commit      *commit;
   scm_fn_status      *status;
   scm_fn_diff        *diff;
   scm_fn_file_status *file_status;
   scm_fn_push        *push;
   scm_fn_pull        *pull;
   scm_fn_stash       *stash;

   scm_fn_remote_add   *remote_add;
   scm_fn_remote_name  *remote_name_get;
   scm_fn_remote_email *remote_email_get;
   scm_fn_remote_url   *remote_url_get;
   scm_fn_credentials  *credentials_set;
   scm_fn_status_get   *status_get;
   Eina_Bool           initialized;
} Edi_Scm_Engine;

/**
 * @brief Executable helpers
 * @defgroup Scm
 *
 * @{
 *
 * Functions of source code management.
 *
 */

/**
 * Init the SCM system for the current project.
 *
 * @ingroup Scm
 */
Edi_Scm_Engine *edi_scm_init();

/**
 * Shutdown and free memory in use by SCM system.
 *
 * @ingroup Scm
 */
void edi_scm_shutdown();

/**
 * Set up a new git repository for the current project.
 *
 * @return The status code of the init command.
 *
 * @ingroup Scm
 */
EAPI int edi_scm_git_new(void);

/**
 * Clone an existing git repository from the provided url.
 *
 * @param url the URL to clone from.
 * @param dir the new directory that will be created to clone into
 * @return The status code of the clone command.
 *
 * @ingroup Scm
 */
EAPI int edi_scm_git_clone(const char *url, const char *dir);

/**
 * Get a pointer to the SCM engine in use.
 *
 * @return The pointer to the engine or NULL.
 *
 * @ingroup Scm
 */
Edi_Scm_Engine *edi_scm_engine_get(void);

/**
 * Add file to be monitored by SCM.
 *
 * @param path The file path.
 * @return The status code of command executed.
 *
 * @ingroup Scm
 */
int edi_scm_add(const char *path);

/**
 * Del file from those monitored by SCM.
 *
 * @param path The file path.
 * @return The status code of command executed.
 *
 * @ingroup Scm
 */
int edi_scm_del(const char *path);

/**
 * Set commit message for next commit to SCM.
 *
 * @param message The commit message to send.
 *
 * @ingroup Scm
 */
void edi_scm_commit(const char *message);

/**
 * Get status of repository.
 *
 * @ingroup Scm
 */
void edi_scm_status(void);

/**
 * Get file status within repository.
 *
 * @param path The file path.
 * @return The status code of the file.
 *
 * @ingroup Scm
 */
Edi_Scm_Status_Code edi_scm_file_status(const char *path);

/**
 * Get status of repository.
 *
 * @return State whether a change was registered (true/false).
*/
Eina_Bool edi_scm_status_get(void);

/**
 * Get diff of changes in repository.
 *
 * @return diff output as a string.
*/
char *edi_scm_diff(void);

/**
 * Move from src to dest.
 *
 * @param src The source file,
 * @param dest The destination.
 *
 * @return The status code of command executed.
 * @ingroup Scm
 */
int edi_scm_move(const char *src, const char *dest);

/**
 * Set user credentials for the SCM system.
 *
 * @param user The name of the user.
 * @param email The email of the user.
 *
 * @return The status code of command executed.
 *
 * @ingroup Scm
 */
int edi_scm_credentials_set(const char *user, const char *email);

/**
 * Push to SCM remote repository.
 *
 * @ingroup Scm
 */
void edi_scm_push(void);

/**
 * Pull from SCM remote repository.
 *
 * @ingroup Scm
 */
void edi_scm_pull(void);

/**
 * Stash local changes.
 *
 * @ingroup Scm
 */
void edi_scm_stash(void);

/**
 * Set remote url for SCM.
 *
 * @param message The remote_url to add.
 *
 * @return The status code of command executed.
 *
 * @ingroup Scm
 */
int edi_scm_remote_add(const char *remote_url);

/**
 * Test whether SCM is enabled for this project.
 *
 * @return Whether SCM is enabled or not (true/false);
 *
 * @ingroup Scm
 */
Eina_Bool edi_scm_enabled(void);

/**
 * Test whether SCM has a remote enabled for this project.
 *
 * @return Whether SCM is remote enabled or not (true/false);
 *
 * @ingroup Scm
 */
Eina_Bool edi_scm_remote_enabled(void);

/**
 * Get the URL to an avatar image for the user (based on email).
 *
 * @return An allocated string containing the URL or NULL if no email
 *
 * @ingroup Scm
 */
const char *edi_scm_avatar_url_get(const char *email);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* EDI_SCM_H_ */
