#! /bin/env bash
cd .devcontainer

if [ ! -d "res/repos/unitree_sdk2" ]; then
    git clone https://github.com/unitreerobotics/unitree_sdk2 res/repos/unitree_sdk2
    (cd res/repos/unitree_sdk2 && git checkout 7b93d01)
fi

if [ ! -d "res/repos/unitree_ros2" ]; then
    git clone https://github.com/unitreerobotics/unitree_ros2 res/repos/unitree_ros2
    (cd res/repos/unitree_ros2 && git checkout d80130f)
    (cd res/repos/unitree_ros2/cyclonedds_ws/src \
    && git clone https://github.com/ros2/rmw_cyclonedds -b humble \
    && git clone https://github.com/eclipse/cyclonedds -b releases/0.10.x)
fi

if [ ! -d "res/repos/unitree_mujoco" ]; then
    git clone https://github.com/unitreerobotics/unitree_mujoco res/repos/unitree_mujoco
    (cd res/repos/unitree_sdk2 && git checkout 7b93d01)
fi