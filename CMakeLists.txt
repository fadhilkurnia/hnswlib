cmake_minimum_required (VERSION 2.6)
project(hnsw_lib
    LANGUAGES CXX)

add_library(hnswlib INTERFACE)
target_include_directories(hnswlib INTERFACE .) 

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(CMAKE_CXX_STANDARD 11)

    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      SET( CMAKE_CXX_FLAGS  "-Ofast -DNDEBUG -std=c++11 -DHAVE_CXX0X -openmp -march=native -fpic -ftree-vectorize")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      SET( CMAKE_CXX_FLAGS  "-Ofast -lrt -DNDEBUG -std=c++11 -DHAVE_CXX0X -march=native -fpic -w -fopenmp -ftree-vectorize -ftree-vectorizer-verbose=0" )
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      SET( CMAKE_CXX_FLAGS  "-Ofast -lrt -DNDEBUG -std=c++11 -DHAVE_CXX0X -openmp -march=native -fpic -w -fopenmp -ftree-vectorize" )
    endif()

    add_executable(test_updates examples/updates_test.cpp)
    target_link_libraries(test_updates hnswlib)

    add_executable(searchKnnCloserFirst_test examples/searchKnnCloserFirst_test.cpp)
    target_link_libraries(searchKnnCloserFirst_test hnswlib)

    add_executable(main main.cpp sift_1b.cpp)
    target_link_libraries(main hnswlib)

    add_executable(deep10m deep_10m.cpp)
    target_link_libraries(deep10m hnswlib)
    
    add_executable(flat_deep10m hnsw_deep10m_flat_index.cpp)
endif()
