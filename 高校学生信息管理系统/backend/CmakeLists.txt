cmake_minimum_required(VERSION 3.24)
project(student_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)

add_executable(${PROJECT_NAME}
    main.cpp
    controllers/AuthController.cpp
    filters/JwtFilter.cpp
    controllers/StudentController.cpp
    controllers/TeacherController.cpp
    controllers/DeptController.cpp
    services/StudentService.cpp
    services/TeacherService.cpp
    services/DeptService.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon jwt-cpp::jwt-cpp)