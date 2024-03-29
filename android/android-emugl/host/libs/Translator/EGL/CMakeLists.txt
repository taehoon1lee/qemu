android_add_library(
  TARGET EGL_translator
  LICENSE Apache-2.0
  SHARED
  SRC # cmake-format: sortable
      ThreadInfo.cpp
      EglImp.cpp
      EglConfig.cpp
      EglContext.cpp
      EglGlobalInfo.cpp
      EglValidate.cpp
      EglSurface.cpp
      EglWindowSurface.cpp
      EglPbufferSurface.cpp
      EglThreadInfo.cpp
      EglDisplay.cpp
      ClientAPIExts.cpp
  DARWIN EglOsApi_darwin.cpp EglOsApi_egl.cpp MacNative.m
         MacPixelFormatsAttribs.m
  LINUX EglOsApi_glx.cpp EglOsApi_egl.cpp CoreProfileConfigs_linux.cpp
  WINDOWS EglOsApi_wgl.cpp EglOsApi_egl.cpp CoreProfileConfigs_windows.cpp)

target_link_libraries(EGL_translator PUBLIC GLcommon emugl_common)
target_link_libraries(EGL_translator PRIVATE emugl_base android-emu-base
                                             astc-codec)
target_compile_options(
  EGL_translator PRIVATE -fvisibility=hidden -Wno-inconsistent-missing-override)
android_target_compile_options(EGL_translator darwin-x86_64
                               PRIVATE -Wno-deprecated-declarations)
android_target_compile_options(EGL_translator darwin-x86_64
                               PRIVATE "-Wno-deprecated-declarations")

# OS specific link dependencies
android_target_link_libraries(EGL_translator darwin-x86_64
                              PUBLIC "-framework AppKit")
android_target_link_libraries(EGL_translator linux-x86_64
                              PUBLIC "-lGL -lX11 -ldl -lpthread")
android_target_link_libraries(EGL_translator windows PUBLIC "gdi32::gdi32")
android_install_shared(EGL_translator)
