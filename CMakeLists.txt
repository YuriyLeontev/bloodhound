cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Библиотека модели
add_library(model STATIC
	src/model/geom.h
	src/model/collision_detector.h
	src/model/collision_detector.cpp
	src/model/loot_generator.h
    src/model/loot_generator.cpp
	src/model/model.h
    src/model/model.cpp
	src/model/model_serialization.cpp
	src/model/model_serialization.h	
)

target_include_directories(model PUBLIC CONAN_PKG::boost Threads::Threads)
target_link_libraries(model CONAN_PKG::boost Threads::Threads)

# Библиотека БД
add_library(postgres STATIC
    src/util/tagged_uuid.cpp
    src/util/tagged_uuid.h
	src/repository/repository.h
    src/repository/postgres.cpp
    src/repository/postgres.h
    src/repository/connection_pool.h)

target_include_directories(postgres PUBLIC CONAN_PKG::boost CONAN_PKG::libpqxx)
target_link_libraries(postgres CONAN_PKG::boost CONAN_PKG::libpqxx)

# Библиотека приложения
add_library(service STATIC
	src/service/service.h
    src/service/service.cpp
	src/service/player.h
    src/service/player.cpp
	src/service/save_scores.h
)

target_link_libraries(service model postgres)

# Основное приложение
add_executable(game_server 
	src/http/http_server.h
	src/http/http_server.cpp

	src/handler/handler_api.h
	src/handler/handler_api.cpp
	src/handler/request.h
	src/handler/response.h
	src/handler/response.cpp

	src/loader/boost_json.cpp
	src/loader/json_loader.h
	src/loader/json_loader.cpp
	src/loader/extra_data.h

	src/util/tagged.h
	src/util/ticker.h
	src/util/util.h
	src/util/util.cpp

	src/logger/logger.h
	src/logger/logger.cpp

	src/sdk.h
	src/main.cpp
)

target_link_libraries(game_server service)

# Тесты
add_executable(game_server_tests
    tests/model-tests.cpp
    tests/loot_generator_tests.cpp
	tests/collision-detector-tests.cpp
)

target_link_libraries(game_server_tests CONAN_PKG::catch2 model)

# state_serialization_tests
add_executable(state_serialization_tests
    tests/state-serialization-tests.cpp
)

target_link_libraries(state_serialization_tests CONAN_PKG::catch2 model service)


# CTest
include(CTest)
include(${CONAN_BUILD_DIRS_CATCH2_RELEASE}/Catch.cmake)
catch_discover_tests(game_server_tests)