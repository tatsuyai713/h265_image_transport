cmake_minimum_required(VERSION 3.5)

project(h265_image_transport)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Emulate colcon in CLion
if($ENV{CLION_IDE})
  message("Running inside CLion")
  find_package(fastrtps_cmake_module REQUIRED)
  set(FastRTPS_INCLUDE_DIR "/opt/ros/eloquent/include")
  set(FastRTPS_LIBRARY_RELEASE "/opt/ros/eloquent/lib/libfastrtps.so")
endif()


find_package(ament_cmake REQUIRED)
find_package(camera_calibration_parsers REQUIRED)
find_package(image_transport REQUIRED)
find_package(pluginlib REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(std_msgs REQUIRED)

rosidl_generate_interfaces(
  ${PROJECT_NAME}
  msg/H265Packet.msg
  DEPENDENCIES std_msgs
)

# Find ffmpeg libraries
find_package(PkgConfig)
pkg_check_modules(PC_AVDEVICE REQUIRED libavdevice>=58)
pkg_check_modules(PC_AVFORMAT REQUIRED libavformat>=58)
pkg_check_modules(PC_AVCODEC REQUIRED libavcodec>=58)
pkg_check_modules(PC_AVUTIL REQUIRED libavutil>=56)
pkg_check_modules(PC_SWSCALE REQUIRED libswscale>=5)


add_library(${PROJECT_NAME}_lib SHARED
  src/h265_subscriber.cpp
  src/manifest.cpp
)

ament_export_dependencies(rosidl_default_runtime)

target_include_directories(${PROJECT_NAME}_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

if(CMAKE_COMPILER_IS_GNUCXX)
  target_link_libraries(${PROJECT_NAME}_lib stdc++fs)
endif()


ament_target_dependencies(${PROJECT_NAME}_lib
  image_transport
  rclcpp
  pluginlib
  sensor_msgs
)

rosidl_target_interfaces(${PROJECT_NAME}_lib
  ${PROJECT_NAME} "rosidl_typesupport_cpp")

set_target_properties(${PROJECT_NAME}_lib PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

target_link_libraries(
  ${PROJECT_NAME}_lib
  ${PC_AVDEVICE_LIBRARIES}
  ${PC_AVFORMAT_LIBRARIES}
  ${PC_AVCODEC_LIBRARIES}
  ${PC_AVUTIL_LIBRARIES}
  ${PC_SWSCALE_LIBRARIES}
)


# This will load & run linters listed in package.xml
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

install(
  TARGETS ${PROJECT_NAME}_lib
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib                 # Shared libraries must be in lib
  RUNTIME DESTINATION lib/${PROJECT_NAME} # Node executables must be in lib/<pkg>
)

install(
  DIRECTORY include
  DESTINATION include
)

install(
  DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

pluginlib_export_plugin_description_file(image_transport h265_plugins.xml)

ament_package()
