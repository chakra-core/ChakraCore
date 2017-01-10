

# Simple Macro that Autoconfigures ChakraCore
MACRO(CHAKRACORE_AUTOCONFIGURE)
  #-------------------------------
  # BEGIN CPU Autodetection
  #-------------------------------

  # Clear up variables that get Modified by this Macro.
  unset(CC_TARGETS_X86_SH CACHE)
  unset(CC_TARGETS_AMD64_SH CACHE)
  unset(CC_TARGETS_X86 CACHE)
  unset(CC_TARGETS_AMD64 CACHE)

  # Configure CPU Architecture.
  # Currently only x86 and AMD64 are all that are autocnfigured.
  if( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64|amd64|AMD64")
    set(CC_TARGETS_AMD64 1)
    message(STATUS "ChakraCore: Building 64-bit x86 code (AMD64)")
  elseif( ${CMAKE_SYSTEM_PROCESSOR} MATCHES "i[3-6]86.*|x86.*")
    set(CC_TARGETS_X86 1)
    set(CMAKE_SYSTEM_PROCESSOR "i386")
    message(STATUS "ChakraCore: Building 32-bit x86 code.")
  else()
    message(FATAL_ERROR "ChakraCore: Autodetection for ${CMAKE_SYSTEM_PROCESSOR} not supported.")
  endif()

  #-------------------------------
  # END CPU Autodetection
  #-------------------------------

  #-------------------------------
  # BEGIN Autodection for International Components for Unicode ( ICU )
  #-------------------------------

  # TODO: Ensure this works for cross-compile.
 
  # Clear all ICU Variables.
  unset(ICU_SETTINGS_RESET CACHE)
  unset(ICU_INCLUDE_PATH CACHE)
  unset(ICU_INCLUDE_PATH_SH CACHE)
  unset(NO_ICU_PATH_GIVEN_SH CACHE)
  unset(NO_ICU_PATH_GIVEN CACHE)
  unset(CC_EMBED_ICU_SH CACHE)
  unset(CC_AUTODETECT_ICU CACHE)
  unset(ICULIB CACHE)
  unset(ICU_CC_PATH CACHE)
  # Auto-Detect ICU Using pkg-config. 
  # To Cross-compile using pkg_config use version 0.23 or newer.
  # Also do not forget to specify the following variables:
  # System Root is defined using: PKG_CONFIG_SYSROOT_DIR
  # pkg-config default Search path: PKG_CONFIG_LIBDIR
  # pkg-config additional search paths: PKG_CONFIG_PATH
  # NOTE: you can make PKG_CONFIG_LIBDIR and PKG_CONFIG_PATH identical.
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    # Find ICU Common and Data files
    pkg_check_modules(PKGCFG_ICU_UC icu-uc)
    # Find ICU Internationalization Library.
    pkg_check_modules(PKGCFG_ICU_I18N icu-i18n)
    if(PKGCFG_ICU_UC_FOUND AND PKGCFG_ICU_I18N_FOUND)
      # This should include internationalization.
      set(ICU_CC_PATH "${PKGCFG_ICU_UC_INCLUDE_DIRS}")
      set(ICULIB      "${PKGCFG_ICU_UC_LDFLAGS}")
    endif()
  endif()
ENDMACRO(CHAKRACORE_AUTOCONFIGURE)

