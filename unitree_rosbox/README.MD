## Setup-Kommandos

- Ausführung von X11-Anwendungen im Container erlauben
```
 xhost local:root
```

- Für Nvidia-Maschinen
  - Nvidia-Container-Runtime installieren: https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html#prerequisites
  - ` sudo nvidia-ctk runtime configure --runtime=docker `
