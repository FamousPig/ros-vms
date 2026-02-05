#! /bin/env bash

podman build -f dockerfiles/Unitree_ROS2.Dockerfile -t kilab09/unitree_ros2
distrobox assemble create
