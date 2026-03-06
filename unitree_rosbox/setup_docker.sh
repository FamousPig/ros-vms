#! /bin/env bash

docker build -f dockerfiles/Unitree_ROS2.Dockerfile -t kilab09/unitree_ros2 ./dockerfiles/
distrobox assemble create
