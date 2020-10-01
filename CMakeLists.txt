cmake_minimum_required (VERSION 2.6)

project(jnivm LANGUAGES CXX)

add_library(jnivm src/jnivm.cpp src/fake-jni/fake-jni.cpp  src/jnivm/internal/findclass.cpp src/jnivm/internal/jValuesfromValist.cpp src/jnivm/internal/scopedVaList.cpp src/jnivm/internal/skipJNIType.cpp src/jnivm/env.cpp src/jnivm/method.cpp src/jnivm/vm.cpp include/jni.h include/jnivm.h)
target_include_directories(jnivm PUBLIC include/)
target_link_libraries(jnivm logger ${CMAKE_DL_LIBS})
set(JNIVM_ENABLE_TRACE OFF CACHE STRING "enable jnivm trace api calls")
if(JNIVM_ENABLE_TRACE)
    target_compile_definitions(jnivm PUBLIC JNI_TRACE)
endif()
set(JNIVM_ENABLE_GC ON CACHE STRING "enable jnivm garbage collection")
if(JNIVM_ENABLE_GC)
    target_compile_definitions(jnivm PUBLIC EnableJNIVMGC)
endif()
set(JNIVM_USE_FAKE_JNI_CODEGEN OFF CACHE STRING "generate fake-jni wrapper instead of jnivm wrapper")
if(JNIVM_USE_FAKE_JNI_CODEGEN)
    target_compile_definitions(jnivm PUBLIC JNIVM_FAKE_JNI_SYNTAX=1)
else()
    target_compile_definitions(jnivm PUBLIC JNIVM_FAKE_JNI_SYNTAX=0)
endif()
set(JNIVM_ENABLE_DEBUG ON CACHE STRING "Enable jnivm debug / wrapper codegen")
if(JNIVM_ENABLE_DEBUG)
    target_compile_definitions(jnivm PUBLIC JNI_DEBUG)
    target_sources(jnivm PRIVATE src/jnivm/internal/codegen/class.cpp src/jnivm/internal/codegen/field.cpp src/jnivm/internal/codegen/method.cpp src/jnivm/internal/codegen/namespace.cpp src/jnivm/internal/codegen/parseJNIType.cpp src/jnivm/internal/codegen/vm.cpp)
endif()
set(JNIVM_ENABLE_RETURN_NON_ZERO OFF CACHE STRING "Unsafe return a nonzero value of non primitive jni types creates Objects (Experimental)")
if(JNIVM_ENABLE_RETURN_NON_ZERO)
    target_compile_definitions(jnivm PUBLIC JNI_RETURN_NON_ZERO)
endif()

set(JNIVM_ENABLE_TESTS OFF CACHE STRING "Enable jnivm Tests via googletest")

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