#!/bin/bash
echo "Setup unitree ros2 simulation environment"
source /opt/ros/humble/setup.bash
source /opt/unitree_ros2/setup.sh
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
