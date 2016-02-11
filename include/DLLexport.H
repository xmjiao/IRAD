///
/// @file
/// @ingroup irad_group
/// @brief Testing object for primitive utilities
///
#ifndef _DLL_EXPORT_H_
#define _DLL_EXPORT_H_

#ifdef _WIN32
  #ifdef IRAD_EXPORTS
//    #ifdef _MSC_VER
//      #pragma warning(disable: 4251)
//    #endif
    #ifdef IRAD_EXPORT
      #define  IRAD_EXPORT __declspec(dllexport)
    #else
      #define  IRAD_EXPORT __declspec(dllimport)
    #endif
  #endif
#else
  #define IRAD_EXPORT
#endif



#endif