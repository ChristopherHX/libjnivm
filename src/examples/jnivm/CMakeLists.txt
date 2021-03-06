project(jnivm-sample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED 14)

add_executable(jnivm-sample main.cpp)
target_link_libraries(jnivm-sample jnivm)