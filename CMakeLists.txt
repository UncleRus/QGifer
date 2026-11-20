cmake_minimum_required(VERSION 3.21)

PROJECT(QGifer LANGUAGES CXX)

SET(TARGET_NAME qgifer)

SET(QGIFER_VERSION_MAJOR 0)
SET(QGIFER_VERSION_MINOR 3)
SET(QGIFER_VERSION_PATCH 0)
# choices between "a", "b", "rc" and empty string for release
SET(QGIFER_VERSION_PRERELEASE_TAG "")
SET(QGIFER_VERSION_PRERELEASE_VERSION 0)
SET(QGIFER_VERSION "${QGIFER_VERSION_MAJOR}.${QGIFER_VERSION_MINOR}.${QGIFER_VERSION_PATCH}")

IF(QGIFER_VERSION_PRERELEASE_TAG)
    SET(QGIFER_VERSION "${QGIFER_VERSION}-${QGIFER_VERSION_PRERELEASE_TAG}${QGIFER_VERSION_PRERELEASE_VERSION}")
ENDIF(QGIFER_VERSION_PRERELEASE_TAG)

OPTION(QUIET_MODE "quiet mode" OFF)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/ui")

IF(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    SET(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
ENDIF()

INCLUDE(GNUInstallDirs)

SET(QGIFER_DATA_DIR "${CMAKE_INSTALL_FULL_DATADIR}/qgifer")

#------------------------------------------------------
#-------------------- DEPENDENCIES --------------------
#------------------------------------------------------

FIND_PACKAGE(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)
FIND_PACKAGE(GIF REQUIRED)

FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(LIBAV REQUIRED IMPORTED_TARGET
    libavformat
    libavcodec
    libavutil
    libswscale
)

IF(TARGET GIF::GIF)
    SET(GIF_TARGET GIF::GIF)
ELSE()
    SET(GIF_TARGET ${GIF_LIBRARIES})
ENDIF()

#------------------------------------------------------
#-------------------- APP BUILD -----------------------
#------------------------------------------------------

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/version.h"
  )

SET(HEADERS
        src/gifcreator.h
        src/imageutils.h
        src/textrenderer.h
        src/videodecoder.h
        src/utils/qgiferformatter.h
)

SET(QT_HEADERS
        src/frameplayer.h
        src/previewwidget.h
        src/mainwindow.h
        src/gifwidget.h
        src/qgifcreator.h
        src/palettewidget.h
        src/qmultislider.h
        src/optimizerdialog.h
        src/converterdialog.h
        src/aboutdialog.h
        src/workspace.h
        src/workspaceobject.h
        src/objectwidget.h
        src/textwidget.h
        src/interpolationdialog.h
        src/textobject.h
        src/retranslatable.h

        src/widgets/QJumpSlider.h
        src/widgets/QPressAction.h
)

SET(SOURCES
        src/frameplayer.cpp
        src/videodecoder.cpp
        src/main.cpp
        src/previewwidget.cpp
        src/mainwindow.cpp
        src/gifwidget.cpp
        src/gifcreator.cpp
        src/qgifcreator.cpp
        src/palettewidget.cpp
        src/qmultislider.cpp
        src/optimizerdialog.cpp
        src/converterdialog.cpp
        src/aboutdialog.cpp
        src/workspace.cpp
        src/workspaceobject.cpp
        src/objectwidget.cpp
        src/textwidget.cpp
        src/textrenderer.cpp
        src/interpolationdialog.cpp

        src/widgets/QJumpSlider.cpp
        src/widgets/QPressAction.cpp

        src/utils/qgiferformatter.cpp
)

SET(RESOURCES
        res.qrc
)

SET(FORMS
        ui/frameplayer.ui
        ui/mainwindow.ui
        ui/gifwidget.ui
        ui/optimizerdialog.ui
        ui/converterdialog.ui
        ui/aboutdialog.ui
        ui/objectwidget.ui
        ui/textwidget.ui
        ui/interpolationdialog.ui
)

FILE(GLOB LOCALES
        "${CMAKE_CURRENT_SOURCE_DIR}/locale/*.qm"
)

SET(DOCS
        doc/ChangeLog
        doc/COPYRIGHT
        doc/LICENSE
        doc/README
)

SET(WINDOWS_EXEC )
IF(WIN32)
    SET(WINDOWS_EXEC WIN32)
    SET(CMAKE_INSTALL_PREFIX "${TARGET_NAME}" CACHE PATH "Install prefix" FORCE)
ENDIF()

SET(RES_FILES "")
IF(MINGW)
    SET(RES_FILES "icon.rc")
    SET(CMAKE_RC_COMPILER_INIT windres)
    ENABLE_LANGUAGE(RC)

    SET(CMAKE_RC_COMPILE_OBJECT
      "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
ENDIF()

ADD_EXECUTABLE(${TARGET_NAME} ${WINDOWS_EXEC}
    ${SOURCES} ${HEADERS} ${QT_HEADERS} ${FORMS} ${RESOURCES} ${RES_FILES})

TARGET_INCLUDE_DIRECTORIES(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    src
    src/widgets
    src/utils
    ui
)

TARGET_LINK_LIBRARIES(${TARGET_NAME} PRIVATE
    Qt6::Widgets
    ${GIF_TARGET}
    PkgConfig::LIBAV
)

IF(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    TARGET_COMPILE_OPTIONS(${TARGET_NAME} PRIVATE
        -Wall -Wno-reorder -Wno-sign-compare)
ENDIF()

TARGET_COMPILE_DEFINITIONS(${TARGET_NAME} PRIVATE
    QGIFER_DATA_DIR="${QGIFER_DATA_DIR}")

IF(QUIET_MODE)
    MESSAGE(STATUS "Quiet mode is ON, no debug output will be shown")
    TARGET_COMPILE_DEFINITIONS(${TARGET_NAME} PRIVATE
        QT_NO_DEBUG_OUTPUT QT_NO_WARNING_OUTPUT)
ENDIF()

# uninstall target
CONFIGURE_FILE(
    "cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

IF(UNIX)
    INSTALL(TARGETS ${TARGET_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    INSTALL(FILES "doc/menu/qgifer" DESTINATION ${CMAKE_INSTALL_DATADIR}/menu)
    INSTALL(FILES "doc/qgifer.desktop" DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    INSTALL(FILES "doc/qgifer.xpm" DESTINATION ${CMAKE_INSTALL_DATADIR}/icons)
    INSTALL(FILES ${LOCALES} DESTINATION ${CMAKE_INSTALL_DATADIR}/qgifer/locale/)
    INSTALL(FILES ${DOCS} DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/qgifer)
ENDIF(UNIX)

IF(WIN32)
    INSTALL(TARGETS ${TARGET_NAME} DESTINATION .)
    INSTALL(FILES ${DOCS} DESTINATION .)
    INSTALL(FILES ${LOCALES} DESTINATION locale/)
ENDIF(WIN32)

INCLUDE(cmake/CMakeCPack.cmake)
