cmake_minimum_required(VERSION 3.5)
project(ik_interface)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(rcutils REQUIRED)
find_package(trajectory_msgs REQUIRED)


# We require a differential inverse kinematics plugin for converting Cartesian admittance to joint admittance
# Base class for the IK plugin
add_library(ik_interface SHARED
  src/ik_plugin_base.cpp
        )

target_include_directories(
        ik_interface
  PRIVATE
  include
)


ament_target_dependencies(
        ik_interface
        rcutils
        geometry_msgs
        trajectory_msgs
        rclcpp
        rclcpp_lifecycle
)

# Causes the visibility macros to use dllexport rather than dllimport,
# which is appropriate when building the dll but not consuming it.
target_compile_definitions(ik_interface PRIVATE "IK_INTERFACE_BUILDING_DLL")

install(DIRECTORY include/
        DESTINATION include
        )
install(TARGETS ik_interface
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        )


# I do not think this is needed
ament_export_dependencies(
        rclcpp_lifecycle
        sensor_msgs
)

ament_export_include_directories(
        include
)
ament_export_libraries(
        ik_interface
)
ament_package()