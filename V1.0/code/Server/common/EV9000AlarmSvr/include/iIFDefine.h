#if !defined(WISCOMVISION_ITSPDEFINE_H_INCLUDED_)
#define WISCOMVISION_ITSPDEFINE_H_INCLUDED_

#ifndef WISCOMVISION_DLL_CLASS
#define WISCOMVISION_DLL_CLASS __declspec(dllexport)
#else
#define WISCOMVISION_DLL_CLASS __declspec(dllimport)
#endif

#ifndef WISCOMVISION_DLL_API
#define WISCOMVISION_DLL_API extern "C" __declspec(dllexport)
#else
#define WISCOMVISION_DLL_API extern "C" __declspec(dllimport)
#endif

#endif