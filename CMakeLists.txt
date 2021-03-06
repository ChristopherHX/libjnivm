cmake_minimum_required (VERSION 2.6)

project(jnivm LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED 14)

add_library(jnivm src/fake-jni/fake-jni.cpp src/jnivm/internal/array.cpp src/jnivm/internal/bytebuffer.cpp src/jnivm/internal/field.cpp src/jnivm/internal/method.cpp src/jnivm/internal/string.cpp src/jnivm/internal/stringUtil.cpp src/jnivm/internal/findclass.cpp src/jnivm/internal/jValuesfromValist.cpp src/jnivm/internal/skipJNIType.cpp src/jnivm/env.cpp src/jnivm/method.cpp src/jnivm/vm.cpp src/jnivm/object.cpp include/jni.h include/jnivm.h)
target_include_directories(jnivm PUBLIC include/)
target_link_libraries(jnivm ${CMAKE_DL_LIBS})
if(TARGET logger)
    target_compile_definitions(jnivm PRIVATE HAVE_LOGGER)
    target_link_libraries(jnivm PRIVATE logger)
endif()

option(JNIVM_ENABLE_TRACE "enable jnivm trace api calls" OFF)
if(JNIVM_ENABLE_TRACE)
    target_compile_definitions(jnivm PRIVATE JNI_TRACE)
endif()
option(JNIVM_ENABLE_GC "enable jnivm garbage collection" ON)
if(JNIVM_ENABLE_GC)
    target_compile_definitions(jnivm PUBLIC EnableJNIVMGC)
endif()
option(JNIVM_USE_FAKE_JNI_CODEGEN "generate fake-jni wrapper instead of jnivm wrapper" OFF)
if(JNIVM_USE_FAKE_JNI_CODEGEN)
    target_compile_definitions(jnivm PRIVATE JNIVM_FAKE_JNI_SYNTAX=1)
else()
    target_compile_definitions(jnivm PRIVATE JNIVM_FAKE_JNI_SYNTAX=0)
endif()
option(JNIVM_ENABLE_DEBUG "Enable jnivm debug / wrapper codegen" ON)
if(JNIVM_ENABLE_DEBUG)
    target_compile_definitions(jnivm PUBLIC JNI_DEBUG)
    target_sources(jnivm PRIVATE src/jnivm/internal/codegen/class.cpp src/jnivm/internal/codegen/field.cpp src/jnivm/internal/codegen/method.cpp src/jnivm/internal/codegen/namespace.cpp src/jnivm/internal/codegen/parseJNIType.cpp src/jnivm/internal/codegen/vm.cpp)
endif()
option(JNIVM_ENABLE_RETURN_NON_ZERO "Unsafe return a nonzero value of non primitive jni types creates Objects (Experimental)" OFF)
if(JNIVM_ENABLE_RETURN_NON_ZERO)
    target_compile_definitions(jnivm PRIVATE JNI_RETURN_NON_ZERO)
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

    if(JNIVM_ENABLE_RETURN_NON_ZERO)
        target_compile_definitions(JNIVMTests PRIVATE JNI_RETURN_NON_ZERO)
    endif()
endif()

option(JNIVM_BUILD_EXAMPLES "Enable jnivm / fake-jni (compat) examples" OFF)
if(JNIVM_BUILD_EXAMPLES)
add_subdirectory(src/examples)
endif()