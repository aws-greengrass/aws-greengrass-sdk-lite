#ifndef GGL_TEST_UNITY_HANDLERS_H
#define GGL_TEST_UNITY_HANDLERS_H

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

int custom_test_protect(void);
NORETURN void custom_test_abort(void);

#endif
