project(isGdip)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if (CMAKE_BUILD_TYPE MATCHES "Release")
foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
   if(${flag_var} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
   endif(${flag_var} MATCHES "/MD")
endforeach(flag_var)
endif()

add_library(isGDI SHARED gdi.cpp)
target_include_directories(isGDI PRIVATE "E:/Cpp/Projects/Headers/Include")


add_library(isGDI2 SHARED gdi2.cpp)
target_include_directories(isGDI2 PRIVATE "E:/Cpp/Projects/Headers/Include")
