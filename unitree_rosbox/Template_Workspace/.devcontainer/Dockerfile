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

ARG NETWORK_INTERFACE="enp0s31f6"

#add some utilities
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

RUN sed -i -e "s/--INTERFACE--/${NETWORK_INTERFACE}/g" /opt/unitree_ros2/setup.sh

# ENTRYPOINT "/opt/unitree_ros2/setup.sh"

RUN echo "source /opt/unitree_ros2/setup.sh" >> ~/.bashrc

# -- DEVCONTAINER STUFF --

FROM run AS devcontainer

ARG USERNAME=rosdev
ARG UID=1000
ARG GID=$UID

# Install some dependencies packages
RUN apt update -q \
    && apt upgrade -q -y \
    && apt install -y --no-install-recommends \
    software-properties-common \
    python3-pip \
    xauth \
    && apt clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

# Create and switch to user
RUN groupadd -g $GID $USERNAME \
    && useradd -lm -u $UID -g $USERNAME -s /bin/bash $USERNAME \
    && echo "$USERNAME ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers
USER $USERNAME

# Create workspace so that user own this directory
RUN mkdir -p /home/$USERNAME/ros2_ws/src
WORKDIR /home/$USERNAME/ros2_ws

# Copy configuration files
# RUN echo 'source /opt/ros/'$ROS_DISTRO'/setup.bash' >> /home/$USERNAME/.bashrc \
#   && echo 'source /home/'$USERNAME'/ros2_ws/install/setup.bash' >> /home/$USERNAME/.bashrc

CMD ["bash"]
