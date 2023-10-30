#ifndef ASSERT_H
#define ASSERT_H

#if defined(HV_DEBUG)

void asm_assert(int32_t line, const char *file, const char *txt);

#define ASSERT(x, ...) \
	do { \
		if (!(x)) {\
			asm_assert(__LINE__, __FILE__, "fatal error");\
		} \
	} while (0)

#else /* HV_DEBUG */

#define ASSERT(x, ...)	do { } while (0)

#endif /* HV_DEBUG */

#endif


