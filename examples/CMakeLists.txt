add_executable(window_list window_list.cpp)
target_link_libraries(window_list PRIVATE CrossWindow)

add_executable(window_control window_control.cpp)
target_link_libraries(window_control PRIVATE CrossWindow)
