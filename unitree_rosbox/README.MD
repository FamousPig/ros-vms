## Setup-Kommandos

- Docker und xhost installieren
 <!--TODO setzen des Namens des Ethernet-Ports & Einrichtung des Netzwerks -->

- Darstellung der Simulator-GUI des Containers erlauben
```
 xhost local:root
```
 <!-- Devcontainer Erweiterung installieren -->

## Verwendung

1. Kopiere den Ordner 'Template_Workspace' an einen beliebigen Ort und benene ihn bei Bedarf um
2. Öffne den kopierten Ordner in Visual Studio Code
3. Drücke 'Strg + Shift + P', um die Befehlseingabe aufzurufen
4. Suche nach  'Dev Containers: Rebuild and Reopen in Container' und wähle es aus # TODO nach lokalisierter Benennung gucken
5. Warte während die Entwicklungsumgebung eingerichtet wird
6. Schreibe oder importiere deinen Code # TODO Einrichtung einer main.cpp oder ähnliches?
## Hardwarebeschleunigung

https://wiki.ros.org/docker/Tutorials/Hardware%20Acceleration

- Für Nvidia-Maschinen
  - Nvidia-Container-Runtime installieren: https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html#prerequisites
  - ` sudo nvidia-ctk runtime configure --runtime=docker 

## Dokumentation

https://support.unitree.com/home/en/G1_developer/about_G1
