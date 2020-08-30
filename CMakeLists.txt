cmake_minimum_required (VERSION 2.6)

project(jnivm LANGUAGES CXX)

add_library(jnivm src/jnivm.cpp src/fake-jni/fake-jni.cpp include/jni.h include/jnivm.h)
target_include_directories(jnivm PUBLIC include/)
target_link_libraries(jnivm logger ${CMAKE_DL_LIBS})