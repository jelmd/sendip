#ifndef _ORIGIN_H
#define _ORIGIN_H
#ifdef  __cplusplus
extern "C" {
#endif

/* Get the origin of a program. Returns %NULL if unable to determine the
 * origin.
 */
char* get_origin(void);
/* Get the origin of a program, cut off up trailing sub dirs and append tail.
 * Returns a copy of %fallback if unable to determine the origin.
 */
char* get_origin_rel(int up, const char *tail, const char *fallback);
#ifdef __cplusplus
}
#endif
#endif
