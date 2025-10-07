cmake_minimum_required(VERSION 3.14)
project(search_engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Подключение библиотеки nlohmann/json
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

# Подключение Google Test для unit-тестов
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)

# Настройки для Google Test
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Для компилятора MinGW
if(MINGW)
    set(gtest_disable_pthreads on)
endif()

# Включение системы тестирования
enable_testing()

# Основное приложение
add_executable(search_engine 
    src/main.cpp 
    src/converterjson.cpp 
    src/invertedindex.cpp 
    src/searchserver.cpp
)

# Настройка путей для заголовочных файлов
target_include_directories(search_engine PRIVATE include)

# ЕДИНЫЙ ТАРГЕТ ЛИНКОВКИ для основного приложения
target_link_libraries(search_engine PRIVATE nlohmann_json::nlohmann_json)

# Копирование ресурсных файлов
add_custom_command(TARGET search_engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/config.json
    $<TARGET_FILE_DIR:search_engine>/config.json
)

add_custom_command(TARGET search_engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/requests.json
    $<TARGET_FILE_DIR:search_engine>/requests.json
)

add_custom_command(TARGET search_engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/resources
    $<TARGET_FILE_DIR:search_engine>/resources
)

# Исполняемый файл для тестов
add_executable(tests 
    tests/test.cpp 
    src/converterjson.cpp 
    src/invertedindex.cpp 
    src/searchserver.cpp
)

# Настройка путей для тестов
target_include_directories(tests PRIVATE include)

# ЕДИНЫЙ ТАРГЕТ ЛИНКОВКИ для тестов
target_link_libraries(tests PRIVATE nlohmann_json::nlohmann_json GTest::gtest_main)

# Интеграция с Google Test
include(GoogleTest)
gtest_discover_tests(tests)