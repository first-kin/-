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
    # ---- C 部分：课程 / 成绩 / 通知公告 ----
    controllers/CourseController.cpp
    controllers/ScoreController.cpp
    controllers/NoticeController.cpp
    services/CourseService.cpp
    services/ScoreService.cpp
    services/NoticeService.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon jwt-cpp::jwt-cpp)