# CoinSorter Lab

ESP32-basierter Münzsortierer mit induktivem LDC1101-Sensor, Servo und RGB-LCD. Wird als Laborpraktikum eingesetzt — von der Sensorphysik bis zum Live-ML-Klassifikator auf dem Mikrocontroller.

## Praktikum

Das Handout (`m.md`) führt durch drei Abschnitte:

1. **Sensorprinzip und Abstand** — LC-Schwingkreis, Skintiefe, Materialvergleich
2. **Datensatz und Merkmale** — Peak-Features erfassen und exportieren
3. **Klassifikation** — manuelle Grenzen, k-NN und Entscheidungsbaum vergleichen

## Notebooks (Google Colab)

| Notebook | Beschreibung | In Colab öffnen |
|----------|-------------|-----------------|
| `knn_notebook.ipynb` | k-NN-Klassifikator trainieren und als `model.json` exportieren | [![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/fkallmer/coinsorter_main/blob/main/knn_notebook.ipynb) |
| `baum_notebook.ipynb` | Entscheidungsbaum trainieren und als `model.json` exportieren | [![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/github/fkallmer/coinsorter_main/blob/main/baum_notebook.ipynb) |

**Eingabedaten:** `peaks.csv` vom ESP32 (Tab „Aufzeichnen" → Peak-Feature-Export → Download).  
**Ausgabe:** `model.json` → im Dashboard hochladen → Live-Klassifikation startet sofort.

## Hardware

| Komponente | Details |
|------------|---------|
| Mikrocontroller | ESP32-D0WD-V3, 240 MHz |
| Sensor | LDC1101 (induktiv, SPI) |
| Aktor | Servo |
| Display | Grove RGB LCD 16×2 (I2C) |
| Flash | 4 MB, LittleFS bei 0x290000 |

## Firmware bauen und flashen

Arduino IDE 2, Board: `ESP32 Dev Module`, Partition: `Default 4MB with spiffs`.

Dashboard-HTML auf das Gerät flashen:

```bash
./flash_littlefs.sh
```

## Dateiübersicht

| Datei | Inhalt |
|-------|--------|
| `coinsorter_main.ino` | Setup, Loop, globaler State |
| `classifier.ino` | Peak-Erkennung, k-NN, Entscheidungsbaum |
| `comm.ino` | HTTP-Routen, WebSocket-Handler |
| `sensor.ino` | LDC1101 SPI-Kommunikation |
| `display.ino` | RGB-LCD-Ausgabe |
| `data/index.html` | Web-Dashboard (Single-File-App) |
| `m.md` | Praktikumshandout |
| `knn_notebook.ipynb` | k-NN Training (Colab) |
| `baum_notebook.ipynb` | Entscheidungsbaum Training (Colab) |
