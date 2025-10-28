
#ifndef VTKGDCM_EXPORT_H
#define VTKGDCM_EXPORT_H

#ifdef VTKGDCM_STATIC_DEFINE
#  define VTKGDCM_EXPORT
#  define VTKGDCM_NO_EXPORT
#else
#  ifndef VTKGDCM_EXPORT
#    ifdef vtkgdcm_EXPORTS
        /* We are building this library */
#      define VTKGDCM_EXPORT 
#    else
        /* We are using this library */
#      define VTKGDCM_EXPORT 
#    endif
#  endif

#  ifndef VTKGDCM_NO_EXPORT
#    define VTKGDCM_NO_EXPORT 
#  endif
#endif

#ifndef VTKGDCM_DEPRECATED
#  define VTKGDCM_DEPRECATED __declspec(deprecated)
#endif

#ifndef VTKGDCM_DEPRECATED_EXPORT
#  define VTKGDCM_DEPRECATED_EXPORT VTKGDCM_EXPORT VTKGDCM_DEPRECATED
#endif

#ifndef VTKGDCM_DEPRECATED_NO_EXPORT
#  define VTKGDCM_DEPRECATED_NO_EXPORT VTKGDCM_NO_EXPORT VTKGDCM_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef VTKGDCM_NO_DEPRECATED
#    define VTKGDCM_NO_DEPRECATED
#  endif
#endif

/* VTK-HeaderTest-Exclude: vtkgdcmModule.h */

/* Include ABI Namespace */
#include "vtkABINamespace.h"

#endif /* VTKGDCM_EXPORT_H */
