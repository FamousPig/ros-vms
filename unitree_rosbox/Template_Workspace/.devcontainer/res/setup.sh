#!/bin/env bash
source /opt/ros/humble/setup.bash
source /opt/unitree_ros2/cyclonedds_ws/install/setup.bash
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp

if [[$CMAKE_PREFIX_PATH != *"/opt/unitree_robotics"*]]; then
    export CMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH:/opt/unitree_robotics
fi

export CYCLONEDDS_URI='<CycloneDDS><Domain><General><Interfaces>
                            <NetworkInterface name="--INTERFACE--" priority="default" multicast="default" />
                        </Interfaces></General></Domain></CycloneDDS>'
