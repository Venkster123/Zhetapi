# Compile cuda program
find_package(CUDA)
if (CUDA_FOUND)
	project(cuda LANGUAGES CXX CUDA)

	execute_process(COMMAND bash -c cmake/cuda_libs.sh)
	
	add_executable(cuda experimental/main.cu)

	find_library(ZHP_CUDA_LIBS
		NAMES zhpcuda
		PATHS .)

	message("ZHP_CUDA_LIB = ${ZHP_CUDA_LIBS}")

	target_link_libraries(cuda PUBLIC zhp-shared)
	target_link_libraries(cuda PUBLIC ${ZHP_CUDA_LIBS})

	target_link_libraries(cuda PUBLIC ${CUDA_LIBRARIES})
	target_link_libraries(cuda PUBLIC cudart)

	set_property(TARGET cuda PROPERTY CUDA_ARCHITECTURES 70)

	if (${CMAKE_BUILD_TYPE} MATCHES Debug)
		target_compile_options(cuda PRIVATE
			$<$<COMPILE_LANGUAGE:CUDA>:--expt-extended-lambda -g -G>)
	else()
		target_compile_options(cuda PRIVATE
			$<$<COMPILE_LANGUAGE:CUDA>:--expt-extended-lambda -O3>)
	endif()
endif()
