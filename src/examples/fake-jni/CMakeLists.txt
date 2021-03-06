project(fake-jni-sample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED 17)

add_executable(fake-jni-sample main.cpp)
target_link_libraries(fake-jni-sample jnivm)