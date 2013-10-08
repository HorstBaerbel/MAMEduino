cmake_minimum_required(VERSION 2.6)

project(MAMEduino)

#-------------------------------------------------------------------------------
#set up compiler flags and excutable names

if(MSVC)
    set(CMAKE_DEBUG_POSTFIX "d")
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP") #multi-processor compilation
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP") #multi-processor compilation
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    #check for G++ 4.7+
    execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE G++_VERSION)
    if (G++_VERSION VERSION_LESS 4.7)
            message(SEND_ERROR "You need at least G++ 4.7 to compile MAMEduino!")
    endif()
    #set up compiler flags for GCC
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2") #support C++11 for std::, optimize
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s") #strip binary
endif()

#-------------------------------------------------------------------------------
#define basic sources and headers

set(TARGET_HEADERS
	${CMAKE_CURRENT_SOURCE_DIR}/src/consolestyle.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MAMEduino.h
)

set(TARGET_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/consolestyle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MAMEduino.cpp
)

#-------------------------------------------------------------------------------
#set up build directories

set(dir ${CMAKE_CURRENT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH ${dir} CACHE PATH "Build directory" FORCE)
set(LIBRARY_OUTPUT_PATH ${dir} CACHE PATH "Build directory" FORCE)

#-------------------------------------------------------------------------------
#define target

add_executable(mameduino ${TARGET_SOURCES} ${TARGET_HEADERS})

#-------------------------------------------------------------------------------
#special properties for windows builds
if(MSVC)
    #create console application
    #Note that up to CMake 2.8.11.2 this feature is broken: http://public.kitware.com/Bug/view.php?id=12566
    set_target_properties(mameduino PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
    set_target_properties(mameduino PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
    set_target_properties(mameduino PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
    set_target_properties(mameduino PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE")
    set_target_properties(mameduino PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
    set_target_properties(mameduino PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:CONSOLE")
endif()
