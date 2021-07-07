cmake_minimum_required(VERSION 3.15)
project(lab_parallel_grep)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)
find_package(ICU REQUIRED COMPONENTS uc i18n)
find_package(Boost REQUIRED COMPONENTS iostreams filesystem locale system)
find_package(LibArchive REQUIRED)
find_package(TBB REQUIRED COMPONENTS tbb)

add_executable(lab_parallel_grep src/main.cpp inc/config.h inc/precise_time.h)
target_link_libraries(lab_parallel_grep ${CMAKE_THREAD_LIBS_INIT} ${ICU_LIBRARIES} ${Boost_LIBRARIES} ${LibArchive_LIBRARIES} ${TBB_IMPORTED_TARGETS})