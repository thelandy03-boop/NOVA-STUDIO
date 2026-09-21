#ifndef __nova_juce_config_h__
#define __nova_juce_config_h__

/* 1. Target Windows 10 x64 */
#ifndef _WIN64
  #define _WIN64 1
#endif
#ifndef _M_X64
  #define _M_X64 1
#endif
#ifndef JUCE_64BIT
  #define JUCE_64BIT 1
#endif
#ifndef _WIN32_WINNT
  #define _WIN32_WINNT 0x0A00
#endif
#ifndef UNICODE
  #define UNICODE 1
#endif
#ifndef _UNICODE
  #define _UNICODE 1
#endif

/* 2. Inclusiones C estándar (Compatibles con C y C++) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__)
  #ifndef _WINSOCKAPI_
    #include <winsock2.h>
  #endif
  #include <ws2tcpip.h>
  #include <windows.h>
  #include <shlobj.h>
  #include <dwmapi.h>

  /* Cabeceras gráficas de Windows (Solo visibles para C++) */
  #ifdef __cplusplus
    #include <ole2.h>
    #include <d2d1_3.h>
    #include <d2d1effects.h>
    #include <dwrite_3.h>
    #include <d3d11_2.h>
    #include <dxgi1_3.h>
    #include <dcomp.h>
  #endif
#endif

#ifndef D2D1_SATURATION_PROP_SATURATION
  #define D2D1_SATURATION_PROP_SATURATION 0u
#endif

/* 3. Inclusiones y Fallbacks específicos de C++ */
#ifdef __cplusplus
  #include <string>
  #include <vector>
  #include <memory>
  #include <iostream>
  #include <cstring>

  // Fallback de std::strncmp para GCC 16
  namespace std {
      using ::strncmp;
  }

  // Fallback de __cpuid para MinGW64
  #if defined(__MINGW32__) || defined(__MINGW64__)
    #include <cpuid.h>
    #ifdef __cpuid
      #undef __cpuid
    #endif
    static inline void __cpuid (int info[4], int type) {
        __cpuidex (info, type, 0);
    }
  #endif

  /* Solución a los UUID de ComSmartPtr en MinGW */
  namespace juce {
      template <typename ComClass> class ComSmartPtr;
  }

  #if defined(__MINGW32__) || defined(__MINGW64__)
    #define JUCE_DECLARE_MINGW_UUIDOF(T) \
      template <> constexpr const GUID& __mingw_uuidof<juce::ComSmartPtr<T>>() { return __mingw_uuidof<T>(); } \
      template <> constexpr const GUID& __mingw_uuidof<juce::ComSmartPtr<T>&>() { return __mingw_uuidof<T>(); } \
      template <> constexpr const GUID& __mingw_uuidof<const juce::ComSmartPtr<T>&>() { return __mingw_uuidof<T>(); }

    JUCE_DECLARE_MINGW_UUIDOF(IDXGIDevice)
    JUCE_DECLARE_MINGW_UUIDOF(IDXGISurface)
    JUCE_DECLARE_MINGW_UUIDOF(IDXGIFactory)
    JUCE_DECLARE_MINGW_UUIDOF(IDXGIFactory1)
    JUCE_DECLARE_MINGW_UUIDOF(IDXGIFactory2)
    JUCE_DECLARE_MINGW_UUIDOF(IDXGIAdapter)
    JUCE_DECLARE_MINGW_UUIDOF(IDXGIAdapter1)
    JUCE_DECLARE_MINGW_UUIDOF(IDXGISwapChain)
    JUCE_DECLARE_MINGW_UUIDOF(IDXGISwapChain1)
    JUCE_DECLARE_MINGW_UUIDOF(IDXGIOutput)
    JUCE_DECLARE_MINGW_UUIDOF(ID3D11Device)
    JUCE_DECLARE_MINGW_UUIDOF(ID3D11DeviceContext)
    JUCE_DECLARE_MINGW_UUIDOF(ID3D11Resource)
    JUCE_DECLARE_MINGW_UUIDOF(ID3D11Texture2D)
    JUCE_DECLARE_MINGW_UUIDOF(ID2D1Factory)
    JUCE_DECLARE_MINGW_UUIDOF(ID2D1Factory1)
    JUCE_DECLARE_MINGW_UUIDOF(ID2D1Device)
    JUCE_DECLARE_MINGW_UUIDOF(ID2D1DeviceContext)
    JUCE_DECLARE_MINGW_UUIDOF(ID2D1Bitmap)
    JUCE_DECLARE_MINGW_UUIDOF(ID2D1Bitmap1)
    JUCE_DECLARE_MINGW_UUIDOF(IDWriteFactory)
    JUCE_DECLARE_MINGW_UUIDOF(IDWriteFactory1)
    JUCE_DECLARE_MINGW_UUIDOF(IDWriteFactory2)
    JUCE_DECLARE_MINGW_UUIDOF(IDWriteFactory3)
  #endif

  /* Stubs de compatibilidad para Accesibilidad deshabilitada */
  namespace juce {
      struct WindowsAccessibility {
          static void revokeUIAMapEntriesForWindow (void*) {}
          static long getUiaRootObjectId() { return 0; }
          template <typename... Args>
          static bool handleWmGetObject (Args&&...) { return false; }
      };
  }
#endif

/* 4. Ajustes Globales JUCE 8 */
#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#define JUCE_STANDALONE_APPLICATION 0
#define JUCE_GUI_BASICS_INCLUDE_XHEADERS 0
#define JUCE_DONT_AUTOLINK_TO_WIN32_LIBRARIES 1

#define JUCE_WEB_BROWSER 0
#define JUCE_USE_CURL 0
#define JUCE_USE_WIN_RT_WEBVIEW 0
#define JUCE_DISABLE_ACCESSIBILITY 1
#define JUCE_DIRECT2D 1
#define JUCE_USE_DIRECTWRITE 1
#define JUCE_USE_NATIVE_FILECHOOSER 1

#endif /* __nova_juce_config_h__ */