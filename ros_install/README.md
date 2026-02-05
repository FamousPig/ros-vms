# Einrichtung von franka_ros auf neueren Linux-Systemen

## Installation distrobox

Zuerst muss *distrobox* installiert werden. Generell sollte es in der Softwareverwaltung der Distribution zu finden sein.

Beispiel Ubuntu 24.04: sudo apt install distrobox

Auf Ubuntu 22 muss erst eine neue Paketquelle hinzugefügt werden:

```
sudo apt-add-repository ppa:michel-slm/distrobox
sudo apt update
sudo apt install distrobox
```

## Erstellen der distrobox

Öffne ein Terminal und führe innerhalb dieses Ordners (dem Ordner mit distrobox.ini) setup.bash aus. Das skript wird die distrobox einrichten und ein paar Konfigurationsdateien an die richtige Stelle kopieren. Alternativ kannst du dir das Skript auch ansehen und die Befehle manuell ausführen.

Bei der Ausführung werden größere Pakete heruntergeladen. Dadurch kann die Einrichtung eine Weile dauern. 

Wenn das Skript fertig ist, sollte es möglich sein die Distrobox über das alias "ros" zu betreten. Man kann die distrobox auch in mehreren Terminals gleichzeitig betreten. ros noetic existiert nur innerhalb der distrobox, insofern müssen ros-Befehle auch innerhalb der distrobox ausgeführt werden.

## Einrichtung Realtime-Kernel

franka_ros benötigt einen Kernel mit Unterstützung für Echtzeitpriorität. Einen solchen zu erlangen ist nicht immer einfach. 

Manche Distributionen verfügen über ein Echtzeit-Kernel-Paket. So existiert bei Arch Linux zum Beispiel linux-rt. (Leider ist Arch aber generell nicht besonders einsteigerfreundlich).

Wichtig: Die Einrichtung geschieht im Host-system, NICHT innherhalb der Distrobox. Die Distrobox verwendet den Kernel des Hosts.

### Ubuntu Pro

Die einfache Variante, einen Realtime-Kernel auf Ubuntu zu erlangen ist [Ubuntu Pro](https://ubuntu.com/real-time). Ubuntu Pro ist für Privatpersonen kostenlos.

Wenn Ubuntu Pro eingerichtet ist:

```
pro attach
pro enable realtime-kernel
```

### Selbst kompilieren

Eine andere Möglichkeit ist es, den Quellcode des Kernels selbst herunterzuladen, zu patchen und zu kompilieren. Das genaue Verfahren ist je nach Distribution und Version unterschiedlich. Eventuell befinden sich Verfahrenshinweise in der [franka_ros-Dokumentation](https://github.com/frankarobotics/docs/blob/master/source/installation_linux.rst) (Abschnitt "Setting up the real-time kernel"). Falls du dieser Anleitung folgst, musst du den Befehl "debpkg" eventuell mit "bindebpkg ersetzen."

### Konfiguration

Erstelle die folgende Datei in /etc/security/limits.conf:

```
@realtime soft rtprio 99
@realtime soft priority 99
@realtime soft memlock 102400
@realtime hard rtprio 99
@realtime hard priority 99
@realtime hard memlock 102400
```

Starte danach den Rechner neu.

## Fertig

An diesem Punkt sollte es möglich sein, über den Befehl "ros" auf die ros-Umgebung zuzugreifen und mit ihr zu arbeiten.
