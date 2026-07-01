Dieses Projekt dient dazu, Programme für den Unitree-G1-Roboter auf einer beliebigen Linux-Distribution zu entwickeln. Dazu wird die Entwicklungsumgebung innerhalb eines Docker-Containers bereitgestellt. Die Devcontainer-Erweiterung für Visual Studio Code ermöglicht eine mit dem Container integrierte Entwicklung.

## Voraussetzungen

- Visual Studio Code mit Devcontainer-Erweiterung

## Einrichtung Docker

- Installiere Docker und Docker-buildx durch den Paketmanager deiner Distribution
- Richte den Docker-Dienst ein

```
# Dienst starten (aktuelle Sitzung)
sudo systemctl start docker

# Socket bei Start des Systems starten (Dienst wird bei Bedarf gestartet)
sudo systemctl enable docker.socket
```

- Füge den verwendeten Benutzer zur 'docker'-Gruppe hinzu

```
 sudo usermod -aG docker <Benutzername>
```

- Installiere xhost und führe dann folgendes Kommando aus, um die Anzeige von grafischen Anwendungen des Containers auf dem Host zu erlauben

```
 xhost local:root
```

### Hardwarebeschleunigung

Standardmäßig hat Docker keinen direkten Zugriff auf die Grafikkarte und kann Programme wie den Mujoco-Simulator nicht flüssig ausführen. Um das zu ermöglichen müssen wir die Hardwarebeschleunigung abhängig vom Hersteller der Grafikkarte einrichten. Für verschiedene Hersteller stehen verschiedene Ausführungskonfigurationen zur Verfügung ( siehe [Entwicklung starten](#entwicklung-starten) )

#### Zusätzliches Setup bei Nvidia-Maschinen
  - Nvidia-Container-Runtime installieren (https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html#prerequisites)
  - `sudo nvidia-ctk runtime configure --runtime=docker` 

https://wiki.ros.org/docker/Tutorials/Hardware%20Acceleration

## Entwicklung starten

- Kopiere den Ordner `Template_Workspace` an einen beliebigen Ort und benenne ihn um
- Öffne den kopierten Ordner innerhalb von VS Code
- Erstelle und betrete den Container über `Strg-Shift-P > Dev Containers: Rebuild and Reopen in Container`
  - Es stehen mehrere Konfigurationen zur Verfügung. Wähle je nach Hersteller der Grafikeinheit des Systems `Nvidia` oder `AMD/Intel`. Bei `Fallback` ist die Hardware-Beschleunigung deaktiviert
  - Bei der ersten Ausführung wird Docker 15-30 Minuten (oder mehr) zu Erstellung des Containers benötigen und mehrere Gigabyte an Daten herunterladen
- Um den Mujoco-Simulator zu öffnen, verwende `Strg-Shift-P > Run Task > Simulator(Mujoco)`
- Um ein eigenes Programm auszuführen, muss `launch.json` angepasst werden
  - Ersetze bei der Umgebungsvariable (`environment`) `CYCLONEDDS_URI` den Namen des Netzwerkinterfaces mit dem auf dem aktuellen System, dass ROS verwenden soll
    - Um den Namen des Interfaces zu finden, führe in einem Terminal `ip link` oder `ifconfig` aus. Der Name startet normalerweise mit "eth" oder "en".
  - Setze den Parameter `program` auf den Pfad zu der Codedatei, die du ausführen willst.
- Um dich mit dem (echtem) Roboter verbinden zu können, musst du die IP-Adresse und das Subnetz des Netzwerkinterfaces einrichten. Unter Ubuntu/GNOME geschieht das unter `Einstellungen > Netzwerk > Kabelgebunden` Bei anderen Distributionen oder Desktopumgebungen kann sich die Vorgehensweise unterscheiden.
  - Setze die IP-Adresse auf `192.168.123.99` und die Subnetz-Maske auf `255.255.255.0`
  - Um die Konfiguration zu testen, sende einen Ping an die Adresse `192.168.123.120`
  - Um sich nach der Arbeit mit dem Roboter wieder über den Ethernet-Port mit dem Internet verbinden zu können, muss das Interface (standardmäßig) wieder auf automatische Beziehung der IP-Adresse gestellt werden

## Hinweise

- Alle Änderungen innerhalb des Containers, die sich nicht im Workspace-Ordner befinden, gehen bei der Neuerstellung des Containers (zum Beispiel durch Veränderung der Dockerfile) verloren und sollten deshalb vermieden werden
  - Falls Änderungen an der Umgebung wie weitere Programme notwendig sind, die nicht im Workspace gelagert werden können, sollte die Dockerfile (`.devcontainer/Dockerfile`) angepasst und der Container neu gebaut werden

## Dokumentation Roboter

https://support.unitree.com/home/en/G1_developer/about_G1
