# CamDemo

A simple Qt6 camera demonstration application that shows a live camera preview and lets you capture photos.

## Features

- Lists all available cameras and allows switching between them
- Displays a live camera preview
- Start / Stop camera stream
- Take photos saved to the system Pictures folder (e.g. `~/Pictures/CamDemo_20260325_120000.jpg`)

## Requirements

- Qt 6.2+ (Widgets, Multimedia, MultimediaWidgets modules)
- CMake 3.16+
- A C++17-capable compiler

## Build

```bash
cmake -B build -S .
cmake --build build
```

## Run

```bash
./build/CamDemo
```

On first launch the application will request camera permission on platforms that require it (macOS, Windows, etc.).
