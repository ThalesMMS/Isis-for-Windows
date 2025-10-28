# Isis DICOM Viewer
> Fork of the academic Asclepios DICOM Viewer. Built with C++, Qt 6.7.2, VTK 9.3, and GDCM (with DCMTK retained for DIMSE networking and related tooling) to deliver interactive 2D/3D visualization pipelines for medical imaging studies.
>
> **Stack status:** Qt 6.7.2 + VTK 9.3 with native GDCM-backed decoding and metadata handling now powers both the 2D slice viewers and the MPR/3D widgets. DCMTK is used only for DIMSE networking and ancillary utilities; image codecs are disabled in favor of GDCM.
> **Current focus:** Expand regression coverage for multi-frame and compressed studies, harden the DIMSE/network layer, and document processing/segmentation pipelines.

## Table of Contents
- [Isis DICOM Viewer](#isis-dicom-viewer)
  - [Table of Contents](#table-of-contents)
  - [Overview](#overview)
  - [Feature Highlights](#feature-highlights)
    - [2D Viewer](#2d-viewer)
    - [3D Rendering and MPR](#3d-rendering-and-mpr)
    - [Workflow Enhancements](#workflow-enhancements)
    - [Repository Root](#repository-root)
  - [Architecture at a Glance](#architecture-at-a-glance)
  - [Dependencies \& Compatibility](#dependencies--compatibility)
  - [Prerequisites](#prerequisites)
  - [Project Structure](#project-structure)
  - [Build \& Run](#build--run)
  - [Tests \& Examples](#tests--examples)
  - [Acknowledgement](#acknowledgement)
  - [License](#license)

## Overview

This repository preserves the history of the original Asclepios DICOM Viewer. The application combines Qt 6.7.2 and VTK 9.3 to provide high-performance visualization. Two primary modules drive the code base: the `core` module ingests and organizes DICOM entities (including metadata and volume handling built on GDCM), while the `gui` module delivers the user experience with Qt widgets backed by VTK renderers.

## Feature Highlights

### 2D Viewer
- Intuitive layout system for displaying DICOM images from multiple modalities
- Multi-window support with configurable layouts for side-by-side comparison
- Advanced window/level adjustments plus overlay and annotation support
- Thumbnail navigation for fast series selection

### 3D Rendering and MPR
- Maximum Intensity Projection and volume rendering presets for volumetric datasets
- Multiplanar reconstruction with synchronized axial, coronal, and sagittal views
- Real-time interaction for panning, rotating, and zooming in 3D scenes
- Tissue-specific keyboard shortcuts to swap rendering presets instantly

### Workflow Enhancements
- Asynchronous file and folder importer that updates the UI as new series finish loading
- In-memory repository that tracks the latest patient, study, series, and image for rapid reuse across modules
- Dedicated Qt/VTK widgets for 2D slices, 3D volume rendering, and MPR coordination
- Processing, segmentation, registration, and event-pipeline components under `src/core` for more advanced workflows

### Repository Root
- [README.md](README.md) - project overview, build instructions, and usage guidance (you are here).

## Architecture at a Glance

- `CoreController` orchestrates ingestion through the GDCM-backed volume loaders (while delegating DICOM networking tasks to DCMTK) and updates the in-memory `CoreRepository` so the GUI can reuse the latest entities.
- The `GUI` module wires `FilesImporter`, specialized widgets, and the Qt signal/slot layer to deliver 2D, 3D, and MPR experiences.
- Rendering modules register necessary VTK components during startup to ensure OpenGL and DPI handling behave consistently on Windows.

## Dependencies & Compatibility

| Component | Recommended version | Notes |
|-----------|--------------------|-------|
| Qt | 6.7.2 MSVC 2019 64-bit | Default toolkit for the GUI. Qt 5.15.2 is no longer required for the new pipelines. |
| VTK | 9.3.0 | Primary rendering stack for 2D, MPR, and 3D views; pairs with GDCM for image decoding. Headers are expected under `lib/include/vtk-9.3`. |
| GDCM | 3.3.x | Handles pixel data decoding and metadata across all viewer modes. Headers are expected under `lib/include/gdcm-3.3`, with libraries under a GDCM install (e.g., `deps/gdcm-install/lib`). |
| DCMTK | 3.6.6 - 3.6.8 (x64) | Used for DIMSE networking and selected utilities. Headers are expected under `lib/include/dcmtk`; libraries under `lib/(debug|release)/dcmtk`. |

## Prerequisites

- Windows 10 or 11 (64-bit).
- Visual Studio 2022 (preferred) or 2019 with the MSVC x64 toolset.
- Qt 6.7.2 installed (e.g., `deps\Qt\6.7.2\msvc2019_64`) and registered with Qt VS Tools or Qt Creator.
- Prebuilt VTK 9.3, GDCM, and DCMTK libraries aligned with the include/library layout referenced by the Visual Studio projects:
  - Headers under `lib/include/vtk-9.3`, `lib/include/gdcm-3.3`, and `lib/include/dcmtk`.
  - Libraries under `lib/Debug`, `lib/Release`, `lib/debug/vtk`, `lib/debug/dcmtk`, and a GDCM install such as `deps/gdcm-install/lib`.
- Remove any legacy `vtk-dicom` directories under `deps/` or `lib/*/` before generating Visual Studio projects; the viewer now links exclusively against VTK, GDCM, and DCMTK.

## Project Structure

```
Isis-for-Windows/
|-- src/
|   |-- Isis-DICOM-Viewer.sln
|   |-- core/
|   |-- gui/
|   |-- examples/
|   \-- x64/
|       |-- Debug/
|       \-- Release/
|-- lib/
|   \-- include/
|       |-- dcmtk/
|       |-- gdcm-3.3/
|       \-- vtk-9.3/
|-- deps/
|   |-- Qt/
|   \-- install_qt672.qs
|-- tests/
|-- installer/
|-- doc/
|-- res/
|-- README.md
\-- LICENSE
```

## Build & Run

The main application is built from the Visual Studio solution under `src/Isis-DICOM-Viewer.sln`.

1. Install and register Qt 6.7.2 MSVC 2019 64-bit with Qt VS Tools.
2. Ensure VTK 9.3, GDCM 3.3.x, and DCMTK 3.6.x are built for x64 and copied into the include/library layout described in the prerequisites.
3. Open `src/Isis-DICOM-Viewer.sln` in Visual Studio 2022 or 2019.
4. Select the `x64` platform and `Debug` or `Release` configuration.
5. Build the `core` and `gui` projects (the `gui` executable depends on `core`).
6. Run the viewer from the generated binary under `src/x64/(Debug|Release)/`.

Optional: the `SetupProject1` project and `installer/` WiX sources can be used to generate an installer once the `gui` binary and dependencies are in place.

## Tests & Examples

- The `tests/` directory contains focused regression and behavior tests for the GDCM/VTK-based pipelines and widgets (e.g., slope/offset handling, multi-frame, MPR/3D guards).
- The `tests/widget2d_rescale_sync_test` subfolder includes a CMake project targeting Qt 6.7.2 and VTK 9.3 from `deps/Qt/6.7.2/msvc2019_64` and `deps/vtk-9.3.0/install`. Configure and build it with CMake to validate 2D rescale synchronization against your local Qt/VTK install.
- `src/examples/integration_example.cpp` showcases higher-level processing, segmentation, registration, and pipeline usage built on the `core` module. It can be built as a separate console target if you want to experiment with the processing pipeline independently of the GUI.

## Acknowledgement

The Isis DICOM Viewer is maintained as a fork of the Asclepios DICOM Viewer project (https://github.com/GavriloviciEduard/Asclepios-DICOM-Viewer) and preserves the original MIT licensing.

## License

This fork preserves the original sources under the MIT License, while all new code, scripts, and documentation added here are released under the Apache License 2.0. See [LICENSE](LICENSE) for the full text of both licenses and attribution requirements.
