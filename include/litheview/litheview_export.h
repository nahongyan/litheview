#ifndef LITHEVIEW_PUBLIC_LITHEVIEW_EXPORT_H_
#define LITHEVIEW_PUBLIC_LITHEVIEW_EXPORT_H_

#if defined(_WIN32)
#if defined(LTV_IMPLEMENTATION)
#define LTV_EXPORT __declspec(dllexport)
#elif defined(LTV_SHARED)
#define LTV_EXPORT __declspec(dllimport)
#else
#define LTV_EXPORT
#endif
#define LTV_CALLBACK __stdcall
#else
#if defined(__GNUC__)
#define LTV_EXPORT __attribute__((visibility("default")))
#else
#define LTV_EXPORT
#endif
#define LTV_CALLBACK
#endif

#endif  // LITHEVIEW_PUBLIC_LITHEVIEW_EXPORT_H_
