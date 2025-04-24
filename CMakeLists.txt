cmake_minimum_required(VERSION 3.19)

project(openamp_demo)

add_compile_options(-pthread)

set(OPENMP_LINUX_LIB openamp_linux)

include_directories(${LIBOPENAMP_INCLUDE_DIR})
include_directories(${LIBMETAL_INCLUDE_DIR})
include_directories(${LIBSYSFS_INCLUDE_DIR})

set(SHARED_LINK_LIBS
	${LIBOPENAMP_LIB_DIR}/libopen_amp.so
	${LIBMETAL_LIB_DIR}/libmetal.so
	${LIBSYSFS_LIB_DIR}/libsysfs.so
	${HUGETLBFS_LIB_DIR}/libhugetlbfs.so
	pthread
)

set(DEMO_SRC_FILE
	./openamp_demo/openamp_app.c
	./openamp_demo/openamp_app_echo.c
    ./operation_interface/operation_interface.c
)

include_directories(
	./operation_interface
	./openamp_demo
)

add_executable(openamp_demo ${DEMO_SRC_FILE})
target_link_libraries(openamp_demo ${SHARED_LINK_LIBS})

install(TARGETS openamp_demo DESTINATION bin)
