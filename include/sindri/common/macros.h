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
 * @def SND_ALIGNED(x)
 * @brief Enforces struct alignment.
 */
#if defined(_MSC_VER)
#define SND_ALIGNED(x) __declspec(align(x))
#else
#define SND_ALIGNED(x) __attribute__((aligned(x)))
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

/**
 * @def SND_SHUFFLE_START
 * @brief Marks the start of a block of code to be shuffled by the mutator.
 * Irrelevant in normal builds.
 */
#define SND_SHUFFLE_START

/**
 * @def SND_SHUFFLE_END
 * @brief Marks the end of a block of code to be shuffled by the mutator.
 * Irrelevant in normal builds.
 */
#define SND_SHUFFLE_END

/**
 * @def SND_MAX
 * @brief Returns the maximum of two values.
 */
#define SND_MAX(a, b) ((a) > (b) ? (a) : (b))

/**
 * @def SND_MIN
 * @brief Returns the minimum of two values.
 */
#define SND_MIN(a, b) ((a) < (b) ? (a) : (b))

#if defined(_WIN64)
#define SND_IS_ARCH_COMPATIBLE(is_64bit) ((is_64bit) != FALSE)
#elif defined(_WIN32)
#define SND_IS_ARCH_COMPATIBLE(is_64bit) (!(is_64bit))
#else
#error "Unsupported architecture: SindriKit requires _WIN32 or _WIN64"
#endif

#endif // SND_COMMON_MACROS_H
