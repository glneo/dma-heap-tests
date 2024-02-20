cmake_minimum_required(VERSION 2.8)
project(dma-heap-tests)

find_package(PkgConfig REQUIRED)

# dma-heap-unit-tests

find_package(GTest REQUIRED)

add_executable(dma-heap-unit-tests
	src/unit/heap_test_fixture.cpp
	src/unit/allocate_test.cpp
	src/unit/exit_test.cpp
	src/unit/invalid_values_test.cpp
	src/unit/map_test.cpp
)

target_include_directories(dma-heap-unit-tests
	PRIVATE ${GTEST_INCLUDE_DIR}
	PRIVATE src/
)

target_link_libraries(dma-heap-unit-tests
	${GTEST_LIBRARIES}
	${GTEST_MAIN_LIBRARIES}
	pthread
)

install(TARGETS dma-heap-unit-tests RUNTIME DESTINATION bin)


# drm-heaps-draw

add_executable(drm-heaps-draw
	src/drm-heaps-draw.c
)

target_include_directories(drm-heaps-draw
	PRIVATE src/
)

target_link_libraries(drm-heaps-draw
	pthread
)

install(TARGETS drm-heaps-draw RUNTIME DESTINATION bin)
