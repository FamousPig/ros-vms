FROM docker.io/osrf/ros:humble-desktop-full AS Unitree_ROS2

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
	git \
	curl \
	gnupg2 \
	lsb-release \
	wget

# Install unitree_sdk2

#dependencies
RUN apt-get update && apt-get install -y \
	cmake \
	g++ \
	build-essential \
	libyaml-cpp-dev \
	libeigen3-dev \
	libboost-all-dev \
	libspdlog-dev \
	libfmt-dev 


WORKDIR /setup
RUN git clone https://github.com/unitreerobotics/unitree_sdk2

WORKDIR unitree_sdk2/build

RUN cmake .. -DCMAKE_INSTALL_PREFIX=/opt/unitree_robotics \
	&& sudo make install

# unitree_ros2 dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
	ros-humble-rmw-cyclonedds-cpp \
	ros-humble-rosidl-generator-dds-idl \
	libyaml-cpp-dev

	
