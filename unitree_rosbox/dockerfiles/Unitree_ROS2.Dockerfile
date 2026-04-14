FROM docker.io/osrf/ros:humble-desktop-full AS prepare

# The image automatically sets up the ros environment. Unfortunately this messes with the compilation of unitree_ros2
ENTRYPOINT "/bin/bash"



FROM prepare as build

ARG DEBIAN_FRONTEND=noninteractive

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

WORKDIR /setup/unitree_sdk2/build

RUN cmake .. -DCMAKE_INSTALL_PREFIX=/opt/unitree_robotics \
	&& sudo make install

# unitree_ros2 dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
	ros-humble-rmw-cyclonedds-cpp \
	ros-humble-rosidl-generator-dds-idl \
	libyaml-cpp-dev

# unitree_ros2 install
WORKDIR /opt

RUN git clone https://github.com/unitreerobotics/unitree_ros2
	
WORKDIR /opt/unitree_ros2/cyclonedds_ws/src
RUN git clone https://github.com/ros2/rmw_cyclonedds -b humble \
	&& git clone https://github.com/eclipse-cyclonedds/cyclonedds -b releases/0.10.x

WORKDIR /opt/unitree_ros2/cyclonedds_ws
RUN colcon build --packages-select cyclonedds
RUN set -a && . /opt/ros/humble/setup.sh && colcon build

WORKDIR /opt/unitree_ros2
RUN set -a && . /opt/ros/humble/setup.sh && colcon build


FROM build as run 
# add some utilites

RUN apt-get update && apt-get install -y --no-install-recommends \
	git \
	curl \
	gnupg2 \
	lsb-release \
	iputils-ping \
	iproute2 \
	vim \
	wget

ADD --chmod=755 res/setup.sh /opt/unitree_ros2

# ENTRYPOINT "/opt/unitree_ros2/setup.sh"

CMD ["/bin/bash"]