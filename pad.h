#ifndef PADRAIGS_GENERIC_C_STUFF
#define PADRAIGS_GENERIC_C_STUFF

#ifdef __cplusplus
extern "C" {
#endif

/* In the following must find out how to get
 * the following only to apply to current function.
 * At the moment they apply for symbol everywhere */
#define VAR_NOT_REF(x)  /*lint -esym(529,x) */
#define PARAM_NOT_REF(x)/*lint -esym(715,x) */

#define lengthof(x) (sizeof(x)/sizeof(x[0]))

/* Support compile time assertions in gcc89, C99 and C++ like:
 *   ct_assert(sizeof(my_struct) == 512); */
#define ct_assert(e) extern char (*ct_assert(void)) [sizeof(char[1 - 2*!(e)])]
/* You can always use the above for global scope
 * but for old non gcc compilers use this in functions */
#define ct_assert_C90_function(e) {enum { assert_ = 1/(!!(e)) };}

/* General function return type */
typedef enum { SUCCESS_STATUS=0, FAIL_STATUS=-1 } STATUS_t;

/* General function return type/ variable type.
 * Note C99 defines bool,true,false in <stdbool.h>
 * so use that if possible. */
typedef enum { FALSE_BOOL=0, TRUE_BOOL=1 } BOOL_t;

#define NULL_TO_EMPTY_STR(x) ((x)?x:"")

#ifdef WIN32
    #define snprintf _snprintf
#endif

#ifndef __GNUC__
# define __PRETTY_FUNCTION__ "(unknown)"
#endif /* __GNUC__ */

#ifdef __cplusplus
}
#endif

#endif
