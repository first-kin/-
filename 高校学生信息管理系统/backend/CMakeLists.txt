cmake_minimum_required(VERSION 3.24)
project(student_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

find_package(Drogon CONFIG REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(${PROJECT_NAME}
    main.cpp
    controllers/AuthController.cpp
    controllers/NoticeController.cpp
    controllers/StudentController.cpp
    controllers/TeacherController.cpp
    controllers/CourseController.cpp
    controllers/DeptController.cpp
    controllers/ScoreController.cpp
    controllers/UserController.cpp
    filters/JwtFilter.cpp
    services/CourseService.cpp
    services/DeptService.cpp
    services/NoticeService.cpp
    services/ScoreService.cpp
    services/StudentService.cpp
    services/TeacherService.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Drogon::Drogon
    jwt-cpp::jwt-cpp
    OpenSSL::SSL
    OpenSSL::Crypto
)