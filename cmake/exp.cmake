# Compile exp file
find_package(OpenCV QUIET)
add_executable(exp experimental/exp.cpp ${EXT_SOURCE})
target_link_libraries(exp ${PNG_LIBRARY} zhp-shared ${OpenCV_LIBS})

# SFML
target_link_libraries(exp
	sfml-system
	sfml-window
	sfml-graphics
	sfml-audio
)
