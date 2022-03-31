cmake_minimum_required(VERSION 2.8)

find_package(PkgConfig REQUIRED)

# dma-heap-unit-tests

find_package(GTest REQUIRED)

add_executable(dma-heap-unit-tests
	src/heap_test_fixture.cpp
	src/allocate_test.cpp
	src/exit_test.cpp
	src/invalid_values_test.cpp
	src/map_test.cpp
)

target_include_directories(dma-heap-unit-tests
	PRIVATE ${GTEST_INCLUDE_DIR}
)

target_link_libraries(dma-heap-unit-tests
	${GTEST_LIBRARIES}
	${GTEST_MAIN_LIBRARIES}
	pthread
)

install(TARGETS dma-heap-unit-tests RUNTIME DESTINATION bin)
