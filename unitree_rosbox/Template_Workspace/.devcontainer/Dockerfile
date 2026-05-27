FROM docker.io/osrf/ros:humble-desktop-full AS prepare

# The image automatically sets up the ros environment. Unfortunately this messes with the compilation of unitree_ros2
ENTRYPOINT "/bin/bash"

FROM prepare AS build

ARG DEBIAN_FRONTEND=noninteractive

# Install unitree_sdk2

# Dependencies
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
WORKDIR /setup/unitree_sdk2

# Pin to latest commit as of 2026-05-27
RUN git reset --hard 7a9ceca8e58c8b75fcf74a092826c138667ad698 

WORKDIR /setup/unitree_sdk2/build
RUN cmake .. -DCMAKE_INSTALL_PREFIX=/opt/unitree_robotics \
	&& sudo make -j$(nproc) install

# Install unitree_ros2

# Dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
	ros-humble-rmw-cyclonedds-cpp \
	ros-humble-rosidl-generator-dds-idl \
	libyaml-cpp-dev

WORKDIR /opt

RUN git clone https://github.com/unitreerobotics/unitree_ros2
	
WORKDIR /opt/unitree_ros2/cyclonedds_ws/src
# Pin to latest commit as of 2026-05-27
RUN git reset --hard 5204e6e098ee53f4bd929bd77eb1d387cd0fa842
RUN git clone https://github.com/ros2/rmw_cyclonedds -b humble \
	&& git clone https://github.com/eclipse-cyclonedds/cyclonedds -b releases/0.10.x

WORKDIR /opt/unitree_ros2/cyclonedds_ws
RUN colcon build --packages-select cyclonedds
RUN set -a && . /opt/ros/humble/setup.sh && colcon build

WORKDIR /opt/unitree_ros2
RUN set -a && . /opt/ros/humble/setup.sh && colcon build


# Install unitree_mujoco

WORKDIR /opt

RUN apt install -y software-properties-common wget
RUN add-apt-repository -y universe
RUN apt install -y libyaml-cpp-dev libspdlog-dev libboost-all-dev libglfw3-dev

RUN wget https://github.com/google-deepmind/mujoco/releases/download/3.8.0/mujoco-3.8.0-linux-x86_64.tar.gz
RUN mkdir mujoco
RUN tar -xf mujoco-3.8.0-linux-x86_64.tar.gz -C mujoco
RUN rm mujoco-3.8.0-linux-x86_64.tar.gz 

RUN git clone https://github.com/unitreerobotics/unitree_mujoco

ADD res/0001-add-elastic-band-command-line-parameter.patch elasticband.patch
WORKDIR /opt/unitree_mujoco/simulate
# Pin to latest commit as of 2026-05-27
RUN git reset --hard c598f103acb87a5fd3de7c9037f4dab6aa7f232b
RUN git apply /opt/elasticband.patch
RUN rm /opt/elasticband.patch

RUN ln -s /opt/mujoco/mujoco-3.8.0 mujoco
WORKDIR /opt/unitree_mujoco/simulate/build
RUN cmake ..
RUN make -j$(nproc)

FROM build AS run 

ARG NETWORK_INTERFACE="enp0s31f6"

# Add some utilities
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
	mesa-utils \
	gdb \
    && apt clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*



# Create and switch to user
RUN groupadd -g $GID $USERNAME \
    && useradd -lm -u $UID -g $USERNAME -s /bin/bash $USERNAME \
    && echo "$USERNAME ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

RUN cp -r /opt/unitree_mujoco /home/$USERNAME/unitree_mujoco
RUN chown -R $USERNAME:$USERNAME /home/$USERNAME/unitree_mujoco

USER $USERNAME

# Create workspace so that user own this directory
RUN mkdir -p /home/$USERNAME/ros2_ws/src
WORKDIR /home/$USERNAME/ros2_ws

# Copy configuration files
# RUN echo 'source /opt/ros/'$ROS_DISTRO'/setup.bash' >> /home/$USERNAME/.bashrc \
#   && echo 'source /home/'$USERNAME'/ros2_ws/install/setup.bash' >> /home/$USERNAME/.bashrc

RUN echo 'source $HOME/ros2_ws/setup.sh' >> /home/$USERNAME/.bashrc

CMD ["bash"]
