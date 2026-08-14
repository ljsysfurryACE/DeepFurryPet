cmake_minimum_required(VERSION 3.16)
project(DeepFurryPet VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia Network)

set(SOURCES
    src/main.cpp
    src/petwindow.cpp
    src/deepseekclient.cpp
    src/settingsmanager.cpp
)

set(HEADERS
    src/petwindow.h
    src/deepseekclient.h
    src/settingsmanager.h
)

set(RESOURCES
    resources/resources.qrc
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Network
)

# Windows specific
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    set(APP_ICON "${CMAKE_SOURCE_DIR}/resources/assets/6.png")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "-mwindows"
    )
endif()

# Linux specific
if(UNIX AND NOT APPLE)
    install(TARGETS ${PROJECT_NAME} DESTINATION /usr/local/bin)
    install(FILES resources/assets/6.png DESTINATION /usr/share/icons/hicolor/256x256/apps)
endif()
