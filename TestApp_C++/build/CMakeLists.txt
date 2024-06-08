cmake_minimum_required(VERSION 3.0)

project(WatcherProgram)

set(CMAKE_CXX_STANDARD 11)

# Исходные файлы проекта
set(SOURCES 
    watcher.cpp
    app.cpp
)

# Добавление исполняемого файла
add_executable(${PROJECT_NAME} ${SOURCES})

# Указываем, что нужно линковать с pthread библиотекой
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} Threads::Threads)
