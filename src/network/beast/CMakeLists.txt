project(beast)

find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)

include_directories(${Boost_INCLUDE_DIR})
include_directories(${benchmark_INCLUDE_DIRS})

add_main_targets(boost::boost openssl::openssl nlohmann_json::nlohmann_json)
add_test_targets(boost::boost openssl::openssl nlohmann_json::nlohmann_json)
add_benchmark_targets(boost::boost openssl::openssl nlohmann_json::nlohmann_json)