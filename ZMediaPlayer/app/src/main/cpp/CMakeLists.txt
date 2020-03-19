# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Sets include directory for ZMediaPlayer and FFmpeg
include_directories(player)
include_directories(ffmpeg/include)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library( # Sets the name of the library.
             native

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
            native.cpp
            player/ZMediaPlayer.c
            player/ZMediaDecode.c
            player/ZMediaStatus.c
            player/ThreadNumberLock.c)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Sets property for libs of FFmpeg
set(FFMPEG_DIR ${CMAKE_SOURCE_DIR}/ffmpeg/libs/${ANDROID_ABI})

# Names a library for avformat of FFmpeg, set is SHARED and IMPORTED
add_library(avformat SHARED IMPORTED)
# Modify property of avformat that gived path of library
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION ${FFMPEG_DIR}/libavformat.so)


# Names a library for avcodec of FFmpeg, set is SHARED and IMPORTED
add_library(avcodec SHARED IMPORTED)
# Modify property of avcodec that gived path of library
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${FFMPEG_DIR}/libavcodec.so)

# Names a library for avutil of FFmpeg, set is SHARED and IMPORTED
add_library(avutil SHARED IMPORTED)
# Modify property of avcodec that gived path of library
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION ${FFMPEG_DIR}/libavutil.so)

# Names a library for swscale of FFmpeg, set is SHARED and IMPORTED
add_library(swscale SHARED IMPORTED)
# Modify property of avcodec that gived path of library
set_target_properties(swscale PROPERTIES IMPORTED_LOCATION ${FFMPEG_DIR}/libswscale.so)

# Names a library for avfilter of FFmpeg, set is SHARED and IMPORTED
add_library(avfilter SHARED IMPORTED)
# Modify property of avcodec that gived path of library
set_target_properties(avfilter PROPERTIES IMPORTED_LOCATION ${FFMPEG_DIR}/libavfilter.so)

# Names a library for swresample of FFmpeg, set is SHARED and IMPORTED
add_library(swresample SHARED IMPORTED)
# Modify property of avcodec that gived path of library
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION ${FFMPEG_DIR}/libswresample.so)


# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       native
                       avformat
                       avcodec
                       avutil
                       swscale
                       avfilter
                       swresample

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib}
                       android
                       jnigraphics)