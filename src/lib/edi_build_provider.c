#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "Edi.h"
#include "edi_build_provider.h"

#include "edi_private.h"

extern Edi_Build_Provider _edi_build_provider_make;
extern Edi_Build_Provider _edi_build_provider_cmake;
extern Edi_Build_Provider _edi_build_provider_cargo;
extern Edi_Build_Provider _edi_build_provider_python;
extern Edi_Build_Provider _edi_build_provider_meson;
extern Edi_Build_Provider _edi_build_provider_go;

EAPI Edi_Build_Provider *edi_build_provider_for_project_get()
{
   return edi_build_provider_for_project_path_get(edi_project_get());
}

EAPI Edi_Build_Provider *edi_build_provider_for_project_path_get(const char *path)
{
   if (!path)
     return NULL;

   if (_edi_build_provider_cmake.path_supported_is(path))
     return &_edi_build_provider_cmake;
   if (_edi_build_provider_cargo.path_supported_is(path))
     return &_edi_build_provider_cargo;
   if (_edi_build_provider_python.path_supported_is(path))
     return &_edi_build_provider_python;
   if (_edi_build_provider_meson.path_supported_is(path))
     return &_edi_build_provider_meson;
   if (_edi_build_provider_go.path_supported_is(path))
     return &_edi_build_provider_go;
   if (_edi_build_provider_make.path_supported_is(path))
     return &_edi_build_provider_make;

   return NULL;
}

EAPI Edi_Build_Provider *edi_build_provider_for_id_get(const char *id)
{
   if (!strcmp("make", id))
     return &_edi_build_provider_make;
   if (!strcmp("cmake", id))
     return &_edi_build_provider_cmake;
   if (!strcmp("cargo", id))
     return &_edi_build_provider_cargo;
   if (!strcmp("python", id))
     return &_edi_build_provider_python;
   if (!strcmp("meson", id))
     return &_edi_build_provider_meson;
   if (!strcmp("go", id))
     return &_edi_build_provider_go;

   return NULL;
}
