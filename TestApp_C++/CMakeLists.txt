cmake_minimum_required(VERSION 3.5)

project(WatcherProgram)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    src/app.cpp
    src/watcher.cpp
)

# Добавление исполняемого файла
add_executable(WatcherProgram ${SOURCES})

# Добавьте это, чтобы связать с pthread
target_link_libraries(WatcherProgram PRIVATE pthread)
