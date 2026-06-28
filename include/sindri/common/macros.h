#ifndef SND_COMMON_MACROS_H
#define SND_COMMON_MACROS_H

/**
 * @def SND_BEGIN_EXTERN_C
 * @brief Opens an `extern "C"` linkage block for C++ compatibility.
 */
/**
 * @def SND_END_EXTERN_C
 * @brief Closes an `extern "C"` linkage block.
 */
#ifdef __cplusplus
#define SND_BEGIN_EXTERN_C extern "C" {
#define SND_END_EXTERN_C   }
#else
#define SND_BEGIN_EXTERN_C
#define SND_END_EXTERN_C
#endif

/**
 * @def SND_FORCE_INLINE
 * @brief Compiler-agnostic macro to force function inlining.
 * Useful for embedding bounds checks and offset calculations directly into the
 * caller's assembly.
 */
#if defined(_MSC_VER)
#define SND_FORCE_INLINE static __forceinline
#else
#define SND_FORCE_INLINE static inline __attribute__((always_inline))
#endif

#endif // SND_COMMON_MACROS_H
