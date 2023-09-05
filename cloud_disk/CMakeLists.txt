cmake_minimum_required(VERSION 3.5)

project(Cloud_DIsk VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt6 COMPONENTS Core5Compat REQUIRED)
find_package(Qt6 COMPONENTS Core REQUIRED)
find_package(Qt6 COMPONENTS Network REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Gui)

set(PROJECT_SOURCES
        main.cpp
        index.cpp
        index.h
        index.ui
        frmlogin.cpp
        frmlogin.h
        frmlogin.ui
        myfile.cpp
        myfile.h
        myfile.ui
        sharelist.cpp
        sharelist.h
        sharelist.ui
        transferlist.cpp
        transferlist.h
        transferlist.ui
        myinfo.cpp
        myinfo.h
        myinfo.ui
        ./common/common.cpp
        ./common/common.h
        ./common/logininfoinstance.cpp
        ./common/logininfoinstance.h
        ./common/iconhelper.cpp
        ./common/iconhelper.h
        ./common/des.c
        ./common/des.h
        ./common/uploadlayout.cpp
        ./common/uploadlayout.h
        ./common/uploadtask.cpp
        ./common/uploadtask.h
        ./common/downloadlayout.cpp
        ./common/downloadlayout.h
        ./common/downloadtask.cpp
        ./common/downloadtask.h
        ./selfwidget/dataprogress.cpp
        ./selfwidget/dataprogress.h
        ./selfwidget/dataprogress.ui
        ./selfwidget/filepropertyinfo.cpp
        ./selfwidget/filepropertyinfo.h
        ./selfwidget/filepropertyinfo.ui
        ./selfwidget/mymenu.cpp
        ./selfwidget/mymenu.h
        ./selfwidget/editmaterial.cpp
        ./selfwidget/editmaterial.h
        ./selfwidget/editmaterial.ui
        ./selfwidget/changepassword.cpp
        ./selfwidget/changepassword.h
        ./selfwidget/changepassword.ui
        res.qrc
)


if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Cloud_DIsk
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET Cloud_DIsk APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(Cloud_DIsk SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(Cloud_DIsk
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(Cloud_DIsk PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(Cloud_DIsk PRIVATE Qt6::Core5Compat)
target_link_libraries(Cloud_DIsk PRIVATE Qt6::Core)
target_link_libraries(Cloud_DIsk PRIVATE Qt6::Network)
target_link_libraries(Cloud_DIsk PRIVATE Qt6::Gui)

set_target_properties(Cloud_DIsk PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS Cloud_DIsk
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Cloud_DIsk)
endif()
