#ifndef GG_TEST_UNITY_HANDLERS_H
#define GG_TEST_UNITY_HANDLERS_H

typedef void (*GgTestFunction)(void);

#ifdef __has_c_attribute
#if __has_c_attribute(noreturn)
#define NORETURN [[noreturn]]
#elif __has_attribute(_Noreturn)
#define NORETURN [[_Noreturn]]
#endif
#endif

#ifndef NORETURN
#define NORETURN
#endif

int gg_test_protect(void);

NORETURN void gg_test_abort(void);

#endif
