cmake_minimum_required(VERSION 3.5)

project(WatcherProgram)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Добавление исполняемого файла
add_executable(WatcherProgram app.cpp watcher.cpp watcher.h)
# Добавьте это, чтобы связать с pthread
target_link_libraries(WatcherProgram PRIVATE pthread)
