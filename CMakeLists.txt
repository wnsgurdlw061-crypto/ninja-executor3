cmake_minimum_required(VERSION 3.16)
project(NinjaEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create DLL
add_library(ninja_engine SHARED
    engine.cpp
    engine.h
    veh_security.cpp
    veh_security.h
)

# Create Injector executable
add_executable(ninja_injector
    injector.cpp
)

set_target_properties(ninja_injector PROPERTIES
    OUTPUT_NAME "NinjaInjector"
)

# Set Windows subsystem (MSVC only)
if(MSVC)
    target_link_options(ninja_engine PRIVATE
        /SUBSYSTEM:WINDOWS
    )
endif()

# Include directories (for Lua if needed)
# target_include_directories(ninja_engine PRIVATE
#     ${CMAKE_SOURCE_DIR}/lua/include
# )

# Link libraries (for Lua if needed)
# target_link_libraries(ninja_engine PRIVATE
#     ${CMAKE_SOURCE_DIR}/lua/lib/lua54.lib
# )

# Windows libraries for DLL
if(MINGW)
    target_link_libraries(ninja_engine PRIVATE
        kernel32 user32 gdi32 winspool comdlg32
        advapi32 shell32 ole32 oleaut32 uuid
        ws2_32
    )
else()
    target_link_libraries(ninja_engine PRIVATE
        kernel32 user32 gdi32 winspool comdlg32
        advapi32 shell32 ole32 oleaut32 uuid
        odbc32 odbccp32
    )
endif()

# Windows libraries for Injector
target_link_libraries(ninja_injector PRIVATE
    kernel32 user32 advapi32
)

# Set output name
set_target_properties(ninja_engine PROPERTIES
    OUTPUT_NAME "ninja_engine"
    PREFIX ""
    SUFFIX ".dll"
)

# Install target
install(TARGETS ninja_engine
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
