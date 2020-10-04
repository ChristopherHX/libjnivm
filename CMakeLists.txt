cmake_minimum_required (VERSION 2.6)

project(jnivm LANGUAGES CXX)

add_library(jnivm src/jnivm.cpp src/fake-jni/fake-jni.cpp  src/jnivm/internal/findclass.cpp src/jnivm/internal/jValuesfromValist.cpp src/jnivm/internal/scopedVaList.cpp src/jnivm/internal/skipJNIType.cpp src/jnivm/env.cpp src/jnivm/method.cpp src/jnivm/vm.cpp include/jni.h include/jnivm.h)
target_include_directories(jnivm PUBLIC include/)
target_link_libraries(jnivm logger ${CMAKE_DL_LIBS})
option(JNIVM_ENABLE_TRACE "enable jnivm trace api calls" OFF)
if(JNIVM_ENABLE_TRACE)
    target_compile_definitions(jnivm PUBLIC JNI_TRACE)
endif()
option(JNIVM_ENABLE_GC "enable jnivm garbage collection" ON)
if(JNIVM_ENABLE_GC)
    target_compile_definitions(jnivm PUBLIC EnableJNIVMGC)
endif()
option(JNIVM_USE_FAKE_JNI_CODEGEN "generate fake-jni wrapper instead of jnivm wrapper" OFF)
if(JNIVM_USE_FAKE_JNI_CODEGEN)
    target_compile_definitions(jnivm PUBLIC JNIVM_FAKE_JNI_SYNTAX=1)
else()
    target_compile_definitions(jnivm PUBLIC JNIVM_FAKE_JNI_SYNTAX=0)
endif()
option(JNIVM_ENABLE_DEBUG "Enable jnivm debug / wrapper codegen" ON)
if(JNIVM_ENABLE_DEBUG)
    target_compile_definitions(jnivm PUBLIC JNI_DEBUG)
    target_sources(jnivm PRIVATE src/jnivm/internal/codegen/class.cpp src/jnivm/internal/codegen/field.cpp src/jnivm/internal/codegen/method.cpp src/jnivm/internal/codegen/namespace.cpp src/jnivm/internal/codegen/parseJNIType.cpp src/jnivm/internal/codegen/vm.cpp)
endif()
option(JNIVM_ENABLE_RETURN_NON_ZERO "Unsafe return a nonzero value of non primitive jni types creates Objects (Experimental)" OFF)
if(JNIVM_ENABLE_RETURN_NON_ZERO)
    target_compile_definitions(jnivm PUBLIC JNI_RETURN_NON_ZERO)
endif()

option(JNIVM_ENABLE_TESTS "Enable jnivm Tests via googletest" OFF)

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