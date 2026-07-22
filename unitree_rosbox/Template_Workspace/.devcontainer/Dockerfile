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
	libfmt-dev \
	&& apt-get install -y --no-install-recommends \ 
	ros-humble-rmw-cyclonedds-cpp \
	ros-humble-rosidl-generator-dds-idl \
	libyaml-cpp-dev \
	wget \
	software-properties-common \
	python3-pip \
	python3.10-venv

WORKDIR /setup

# install clang tools
RUN wget https://apt.llvm.org/llvm.sh
RUN chmod +x llvm.sh
RUN ./llvm.sh 18 all
RUN update-alternatives --install /usr/bin/clangd clangd /usr/bin/clangd-18 100 \
    && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-18 100 \
    && update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 100 \
    && update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-18 100 \
    && update-alternatives --install /usr/bin/run-clang-tidy run-clang-tidy /usr/bin/run-clang-tidy-18 100
ENV CC=/usr/bin/clang-18
ENV CXX=/usr/bin/clang++-18

COPY res/repos/unitree_sdk2 /setup/unitree_sdk2
WORKDIR /setup/unitree_sdk2

WORKDIR /setup/unitree_sdk2/build
RUN cmake .. -DCMAKE_INSTALL_PREFIX=/opt/unitree_robotics \
	&& make -j$(nproc) install

# Install unitree_ros2

WORKDIR /opt

COPY res/repos/unitree_ros2 /opt/unitree_ros2
	
WORKDIR /opt/unitree_ros2/cyclonedds_ws/src

WORKDIR /opt/unitree_ros2/cyclonedds_ws
RUN colcon build --packages-select cyclonedds
RUN set -a && . /opt/ros/humble/setup.sh && colcon build

WORKDIR /opt/unitree_ros2
RUN set -a && . /opt/ros/humble/setup.sh && colcon build --packages-ignore unitree_ros2_example


# Install unitree_mujoco

WORKDIR /opt

RUN apt install -y software-properties-common
RUN add-apt-repository -y universe
RUN apt install -y libyaml-cpp-dev libspdlog-dev libboost-all-dev libglfw3-dev

ADD https://github.com/google-deepmind/mujoco/releases/download/3.8.0/mujoco-3.8.0-linux-x86_64.tar.gz mujoco-3.8.0-linux-x86_64.tar.gz
RUN mkdir mujoco
RUN tar -xf mujoco-3.8.0-linux-x86_64.tar.gz -C mujoco
RUN rm mujoco-3.8.0-linux-x86_64.tar.gz 

COPY res/repos/unitree_mujoco /opt/unitree_mujoco

ADD res/0001-add-elastic-band-command-line-parameter.patch elasticband.patch
WORKDIR /opt/unitree_mujoco/simulate
RUN git apply /opt/elasticband.patch
RUN rm /opt/elasticband.patch

RUN ln -s /opt/mujoco/mujoco-3.8.0 mujoco
WORKDIR /opt/unitree_mujoco/simulate/build
RUN cmake ..
RUN make -j$(nproc)

#Install g1-crc-python-module
COPY res/repos/g1_crc_module /setup/g1_crc_module
#RUN pip install /setup/g1_crc_module


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
	vim 

ADD --chmod=755 res/setup.sh /opt/unitree_ros2

RUN sed -i -e "s/--INTERFACE--/${NETWORK_INTERFACE}/g" /opt/unitree_ros2/setup.sh

FROM run AS devcontainer

ARG USERNAME=rosdev
ARG UID=1000
ARG GID=$UID

# Install some dependencies packages
RUN apt update -q \
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
RUN mkdir -p /home/$USERNAME/ros2_ws/src/.venv
WORKDIR /home/$USERNAME/ros2_ws
RUN python3 -m venv .venv
RUN set -a && . .venv/bin/activate && pip install /setup/g1_crc_module


RUN echo 'source $HOME/ros2_ws/setup.sh' >> /home/$USERNAME/.bashrc

CMD ["bash"]
