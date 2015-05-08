#Run 'cmake <source_directory>' to generate a makefile
cmake_minimum_required(VERSION 2.6)

#definitions
set(PROJECT_NAME "Vec")
set(APP_NAME vec)
set(PROJECT_VERSION 1 0 4)
set(DESCRIPTION "Dual-mouse arcade thing")
set(README README.txt)
set(LICENSE ${README})
set(AUTHORS "Brendan Luchen")
set(CMAKE_BUILD_TYPE Debug)

#extract the proper versioning breakdown
list(GET PROJECT_VERSION 0 CPACK_PACKAGE_VERSION_MAJOR )
list(GET PROJECT_VERSION 1 CPACK_PACKAGE_VERSION_MINOR )
list(GET PROJECT_VERSION 2 CPACK_PACKAGE_VERSION_PATCH )
set(VERSION_STRING "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

#Be compatible
cmake_policy(VERSION 2.8)

#declare the project
project(${PROJECT_NAME} )

#Find required tools (SDL, GL, GLEW)
find_package(SDL REQUIRED)
find_package(OpenGL REQUIRED)
list(APPEND LINKED_LIBS ${SDL_LIBRARY})
list(APPEND LINKED_LIBS ${SDLMAIN_LIBRARY})
list(APPEND LINKED_LIBS ${OPENGL_LIBRARY})
if(APPLE) # ManyMouse OSX depends on this wonkiness
  find_library(IOKIT IOKit)
  find_library(COREFOUNDATION CoreFoundation)
  list(APPEND LINKED_LIBS ${IOKIT} ${COREFOUNDATION} )
  set(APP_SUFFIX "osx")
endif()


# Slap together search paths
list(APPEND EXTRA_INCLUDE_DIRECTORIES ${SDL_INCLUDE_DIR})
list(APPEND EXTRA_INCLUDE_DIRECTORIES ${OPENGL_INCLUDE_DIR})
list(APPEND EXTRA_INCLUDE_DIRECTORIES vendor/manymouse)
list(APPEND EXTRA_INCLUDE_DIRECTORIES vendor/glew)
list(APPEND EXTRA_INCLUDE_DIRECTORIES vendor/sfxd)

#grab source files
file(GLOB_RECURSE HEADERS ${CMAKE_SOURCE_DIR}/src/*.h)
list(APPEND SOURCES 
  src/main.cpp 
  src/game.cpp 
  src/gfx.cpp
  src/audio.cpp
  vendor/manymouse/windows_wminput.c
  vendor/manymouse/manymouse.c
  vendor/manymouse/macosx_hidmanager.c
  vendor/manymouse/macosx_hidutilities.c
  vendor/manymouse/linux_evdev.c
  vendor/manymouse/x11_xinput2.c
  vendor/glew/glew.c
  vendor/sfxd/sfxd.cpp
)

#declare the target program
add_executable(${APP_NAME} ${SOURCES} ${EXTRA_SOURCES} ${HEADERS})

#add some useful preprocessor defines
set_property(
    TARGET ${APP_NAME} PROPERTY COMPILE_DEFINITIONS
    PROJECT_NAME="${PROJECT_NAME}"
    AUTHOR_NAMES="${AUTHORS}"
)

set_target_properties(${APP_NAME} PROPERTIES
  OUTPUT_NAME "${APP_NAME}-${VERSION_STRING}-${APP_SUFFIX}"
)

target_link_libraries(${APP_NAME} ${LINKED_LIBS})
include_directories(${EXTRA_INCLUDE_DIRECTORIES})

#install the binary to bin under the install directory
install(TARGETS ${APP_NAME}
    DESTINATION bin
)
install(FILES ${README} ${LICENSE} ${ADDL_INSTALL_FILES}
    DESTINATION .
)
install(FILES ${DLLFILES}
    DESTINATION bin
)

include(CPack)
