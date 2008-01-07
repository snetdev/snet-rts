#ifndef STR_H_
#define STR_H_

/* Copy string.
 *
 * @param text String to be copied.
 *
 * @return Copy of the given string.
 *
 * @notice User MUST free the memory of the new string!
 *
 */

extern char *STRcpy(const char *text);

/* Compare strings.
 *
 * @param a The first string to be compared.
 * @param b The second string to be compared.
 *
 * @return as strcmp.
 *
 */

extern int STRcmp(const char *a, const char *b);

/* Concatenate strings.
 *
 * @param a The first string.
 * @param b The second string.
 *
 * @return New string formed as the characters of the old strings are concatenated.
 *
 * @notice User MUST free the memory of the new string!
 *
 */

extern char *STRcat(const char *a, const char *b);

#endif /*STR_H_ */
