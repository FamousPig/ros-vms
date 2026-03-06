FROM docker.io/osrf/ros:humble-desktop-full

ARG DEBIAN_FRONTEND=noninteractive

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

WORKDIR /setup/unitree_sdk2/build

RUN cmake .. -DCMAKE_INSTALL_PREFIX=/opt/unitree_robotics \
	&& sudo make install

# unitree_ros2 dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
	ros-humble-rmw-cyclonedds-cpp \
	ros-humble-rosidl-generator-dds-idl \
	libyaml-cpp-dev

# unitree_ros2 install
WORKDIR /setup

RUN git clone https://github.com/unitreerobotics/unitree_ros2
	
WORKDIR /setup/unitree_ros2/cyclonedds_ws/src
RUN git clone https://github.com/ros2/rmw_cyclonedds -b humble \
	&& git clone https://github.com/eclipse-cyclonedds/cyclonedds -b releases/0.10.x

WORKDIR /setup/unitree_ros2/cyclonedds_ws
RUN colcon build --packages-select cyclonedds
RUN set -a && . /opt/ros/humble/setup.sh && colcon build

RUN mkdir /opt/unitree_ros2
RUN cp -r /setup/unitree_ros2/cyclonedds_ws/src /setup/unitree_ros2/cyclonedds_ws/install /opt/unitree_ros2/
ADD --chmod=755 res/setup.sh /opt/unitree_ros2


# -- DEVCONTAINER STUFF --
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

# Setup entrypoint
COPY "res/setup.sh" "/entrypoint.sh"
ENTRYPOINT ["/entrypoint.sh"]
CMD ["bash"]
