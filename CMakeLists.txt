cmake_minimum_required (VERSION 2.6)

project(jnivm LANGUAGES CXX)

add_library(jnivm src/jnivm.cpp src/fake-jni/fake-jni.cpp include/jni.h include/jnivm.h)
target_include_directories(jnivm PUBLIC include/)
target_link_libraries(jnivm logger ${CMAKE_DL_LIBS})


set(JNIVM_ENABLE_TESTS OFF CACHE STRING "Enable Tests via googletest")

if(JNIVM_ENABLE_TESTS)
    include(FetchContent)
    FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.10.0
    )

    # After the following call, the CMake targets defined by googletest
    # will be defined and available to the rest of the build
    FetchContent_MakeAvailable(googletest)

    add_executable(JNIVMTests src/tests/Tests.cpp)
    target_link_libraries(JNIVMTests gtest gtest_main jnivm)
endif()