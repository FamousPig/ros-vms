# ROS Installation

## Einleitung

Dieses Dokument behandelt die Einrichtung der Bibliothek 'franka_ros' auf Ubuntu 22.04 LTS. Ziel ist die Möglichkeit der Steuerung des Roboterarms 'Franka Emika Panda' über die ROS1-Schnittstelle. Bei der Einrichtung gibt es einige Dinge zu beachten, da nicht alle benötigten Funktionen direkt für diese Version von Linux verfügbar sind.

## Installation ros_noetic

ROS bietet kein noetic-Paket für Ubuntu 22.04 an. Die Lösungsmöglichkeiten sind das eigene Kompilieren der Applikation, wofür wiederum verschiedene Abhängigkeiten installiert werden müssten, oder die Verwendung einer anderen Distribution. Mithilfe von 'distrobox' können wir eine andere Distribution, wie das unterstützte Ubuntu 20.04, als Container ausführen und ros_noetic in diesem Rahmen installieren.

### Installation Distrobox

Distrobox ist nicht im offiziellen Repository von Ubuntu 22.04 vorhanden. Die Installation erfolgt über das PPA (Personal Package Archive) des Maintainers auf neueren Versionen von Ubuntu. Zum Hinzufügen dieser Quelle und Installation der Applikation dient der Befehl: 

```
sudo apt-add-repository ppa:michel-slm/distrobox
sudo apt update
sudo apt install distrobox
```
### Erstellung der Distrobox

```
distrobox create --image ubuntu:20.04 --name ros
```

Betreten durch `distrobox enter ros`

### Einrichtung ROS-Repository

Innerhalb der distrobox können wir nun mit der Einrichtung der Arbeitsumgebung beginnen.

```
sudo sh -c 'echo "deb [signed-by=/etc/apt/keyrings/ros.asc] http://packages.ros.org/ros/ubuntu focal main" > /etc/apt/sources.list.d/ros-latest.list'

mkdir -p /etc/apt/keyrings

curl -s https://raw.githubusercontent.com/ros/rosdistro/master/ros.asc | sudo tee /etc/apt/keyrings/ros.asc
```

### Installation ros-noetic

```
sudo apt update
sudo apt install ros-noetic-desktop-full
```

### Eigene Konfiguration für distrobox

Ros erwartet, dass man vor Verwendung der Tools ein Skript ausführt. Dieses Skript existiert nur innerhalb der distrobox, nicht auf dem Host. Deswegen werden wir eine eigene bash-Konfiguration für die distrobox erstellen.

Erstelle eine datei namens '.rosboxrc' mit folgendem Inhalt

```
source ~/.bashrc

source /opt/ros/noetic/setup.bash
export DISABLE_ROS1_EOL_WARNINGS=true
```

Kehre nun mithilfe von 'exit' zum Hostsystem zurück. Führe hier folgenden Befehl aus

```
echo "alias ros='distrobox enter ros -- bash --rcfile .rosboxrc' " >> .bash_aliases
```

Von nun an sollte die distrobox über den Befehl 'ros' betreten werden. Dadurch werden die notwendigen Initialisierungsschritte automatisch gemeinsam ausgeführt.

### Installation franka_ros

Innerhalb der Distrobox

```
sudo apt install ros-noetic-libfranka ros-noetic-franka-ros
```

## Installation Real-Time-Kernel

libfranka benötigt Echtzeitpriorität. Der Kernel, den Ubuntu standardmässig verwendet unterstützt keine Echtzeitpriorität. Es gibt zwei Optionen:

- Abbonieren von Ubuntu-Pro und verwenden von deren realtime-kernel
- Eigenes Kompilieren des Kernels mit RT-Patches

### Vorraussetzungen

```
sudo apt-get install build-essential bc curl debhelper dpkg-dev devscripts fakeroot libssl-dev libelf-dev bison flex cpio kmod rsync libncurses-dev
```

### Herunterladen Kernel-Quellcode und Patches

In einem leeren Verzeichnis

```
curl -LO https://www.kernel.org/pub/linux/kernel/v6.x/linux-6.8.tar.xz
curl -LO https://www.kernel.org/pub/linux/kernel/v6.x/linux-6.8.tar.sign
curl -LO https://www.kernel.org/pub/linux/kernel/projects/rt/6.8/older/patch-6.8-rt8.patch.xz
curl -LO https://www.kernel.org/pub/linux/kernel/projects/rt/6.8/older/patch-6.8-rt8.patch.sign
xt -d *.xz
```

### Kernel kompilieren und verpacken

Dekomprimieren des Quellcodes und anwenden der Patches

```
tar xf linux-*.tar
cd linux-*/
patch -p1 < ../patch-*.patch
```

Einrichtung Konfiguration

```
cp -v /boot/config-$(uname -r) .config

scripts/config --disable DEBUG_INFO
scripts/config --disable DEBUG_INFO_DWARF_TOOLCHAIN_DEFAULT
scripts/config --disable DEBUG_KERNEL

scripts/config --disable SYSTEM_TRUSTED_KEYS
scripts/config --disable SYSTEM_REVOCATION_LIST

scripts/config --disable PREEMPT_NONE
scripts/config --disable PREEMPT_VOLUNTARY
scripts/config --disable PREEMPT
scripts/config --enable PREEMPT_RT

make olddefconfig
```

Kompilieren und Verpacken (Das dauert eine Weile)

```
make -j$(nproc) bindeb-pkg
```

Installation des neuen Kernels

```
sudo IGNORE_PREEMPT_RT_PRESENCE=1 dpkg -i ../linux-headers-*.deb ../linux-image-*.deb
```

### GRUB Konfiguration zum Start des neuen Kernels
Der RT-Kernel ist neben dem Standard-Kernel installiert. Um den RT-Kernel zu starten, muss die Konfiguration des Bootloaders, hier GRUB, angepasst werden. 

Die Konfiguration erfolgt über die Datei '/etc/default/grub'.
Zum Anzeigen des Menüs bei Systemstart müssen die folgenden Optionen hinzugefügt/geändert werden:

```
GRUB_TIMEOUT_STYLE=menu
GRUB_TIMEOUT=5
```
Zum erfolgreichen Start muss das BIOS-Setting "Safeboot" deaktiviert werden. Alternativ kann ein eigener Schlüssel zum Signieren des Kernels verwendet werden.

Um automatisch den RT-Kernel zu starten, kann die Option 'GRUB_DEFAULT' verwendet werden. Dafür sollte die genaue Bennenung der gewünschten Option eingesetzt werden. Untermenüs werden durch '>' verknüpft. Zum Beispiel:

```
GRUB_DEFAULT="Advanced options for Ubuntu>Ubuntu, with Linux 6.8.0-rt8"
``` 

### Realtime-Berechtigung

Für die Berechtigung, Einstellungen mit Echtzeit-Priorität auszuführen

```
sudo addgroup realtime
sudo usermod -a -G realtime $(whoami)
```

Hinzufügen zu '/etc/security/limits.conf'

```
@realtime soft rtprio 99
@realtime soft priority 99
@realtime soft memlock 102400
@realtime hard rtprio 99
@realtime hard priority 99
@realtime hard memlock 102400
```
