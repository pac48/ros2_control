cmake /home/paul/admittance_ws/src/ros2_control/controller_manager -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug -DAMENT_CMAKE_SYMLINK_INSTALL=1 -DCMAKE_INSTALL_PREFIX=/home/paul/admittance_ws/install/controller_manager
cmake /home/paul/admittance_ws/src/ros2_control/controller_manager -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug -DAMENT_CMAKE_SYMLINK_INSTALL=1 -DCMAKE_INSTALL_PREFIX=/home/paul/admittance_ws/install/controller_manager

cmake --build /home/paul/admittance_ws/build/controller_manager -- -j8 -l8
cmake --build /home/paul/admittance_ws/build/controller_manager -- -j8 -l8
cmake --install /home/paul/admittance_ws/build/controller_manager
cmake --install /home/paul/admittance_ws/build/controller_manager
