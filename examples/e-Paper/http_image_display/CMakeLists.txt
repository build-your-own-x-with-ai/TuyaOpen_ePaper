##
# @file CMakeLists.txt
# @brief HTTP Image Display Example - Download and display image on e-Paper
#/

# APP_PATH
set(APP_PATH ${CMAKE_CURRENT_LIST_DIR})

# APP_NAME
get_filename_component(APP_NAME ${APP_PATH} NAME)

# APP SRC
aux_source_directory(${APP_PATH}/src APP_SRC)

# E-Paper library sources
file(GLOB EPAPER_CONFIG_SRC  "${APP_PATH}/lib/Config/*.c")
file(GLOB EPAPER_DRIVER_SRC  "${APP_PATH}/lib/e-Paper/*.c")
file(GLOB EPAPER_FONTS_SRC   "${APP_PATH}/lib/Fonts/*.c")
file(GLOB EPAPER_GUI_SRC     "${APP_PATH}/lib/GUI/*.c")

set(APP_SRC
    ${APP_SRC}
    ${EPAPER_CONFIG_SRC}
    ${EPAPER_DRIVER_SRC}
    ${EPAPER_FONTS_SRC}
    ${EPAPER_GUI_SRC}
)

# APP INC
set(APP_INC 
    ${APP_PATH}/src
    ${APP_PATH}/lib/Config
    ${APP_PATH}/lib/e-Paper
    ${APP_PATH}/lib/Fonts
    ${APP_PATH}/lib/GUI
)

# APP_OPTIONS
set(APP_OPTIONS "-DDEBUG")

# Disable treating warnings as errors
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    list(APPEND APP_OPTIONS "-Wno-error")
elseif(MSVC)
    list(APPEND APP_OPTIONS "/WX-")
endif()

########################################
# Target Configure
########################################
add_library(${EXAMPLE_LIB})

target_sources(${EXAMPLE_LIB}
    PRIVATE
        ${APP_SRC}
)

target_include_directories(${EXAMPLE_LIB}
    PRIVATE
        ${APP_INC}
)

target_compile_options(${EXAMPLE_LIB}
    PRIVATE
        ${APP_OPTIONS}
)
