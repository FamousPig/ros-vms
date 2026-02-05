#!/bin/env bash
source /opt/ros/humble/setup.bash
source /opt/unitree_ros2/install/setup.bash
export RMW_IMPLEMENTATINO=rmw_cyclonedds_cpp

#TODO query network interface on setup?
export CYCLONEDDS_URI='<CycloneDDS><Domain><General><Interfaces>
                            <NetworkInterface name="enp0s31f6" priority="default" multicast="default" />
                        </Interfaces></General></Domain></CycloneDDS>'
