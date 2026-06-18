cmake_minimum_required(VERSION 3.8)
project(robot_pipeline)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)
find_package(realsense2 QUIET)

if(realsense2_FOUND)
  # --- camera_node ---
  add_executable(camera_node src/camera_node.cpp)
  target_include_directories(camera_node PRIVATE ${realsense2_INCLUDE_DIR})
  ament_target_dependencies(camera_node rclcpp sensor_msgs cv_bridge)
  target_link_libraries(camera_node ${realsense2_LIBRARY} ${OpenCV_LIBS})
  list(APPEND INSTALL_TARGETS camera_node)
  message(STATUS "librealsense2 found — camera_node will be built")
else()
  message(WARNING "librealsense2 NOT found — camera_node will be SKIPPED")
endif()

# --- microphone_node ---
add_executable(microphone_node src/microphone_node.cpp)
ament_target_dependencies(microphone_node rclcpp std_msgs)
target_link_libraries(microphone_node asound)

list(APPEND INSTALL_TARGETS microphone_node)
install(TARGETS ${INSTALL_TARGETS}
  DESTINATION lib/${PROJECT_NAME})

# --- Python compute_node ---
ament_python_install_package(${PROJECT_NAME})
install(PROGRAMS
  robot_pipeline/compute_node.py
  robot_pipeline/comms_node.py
  DESTINATION lib/${PROJECT_NAME})

# --- launch ---
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME})

ament_package()
