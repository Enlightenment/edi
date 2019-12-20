#ifndef EDI_MIME_H_
# define EDI_MIME_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief These routines are used for Edi mime handling
 */

/**
 * Return the mime type of a file
 *
 * @param path The path of the file to return the mime type of.
 *
 * @return A pointer to the mime type as a const character string.
 *
 */
EAPI const char *edi_mime_type_get(const char *path);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* EDI_MIME_H_ */
