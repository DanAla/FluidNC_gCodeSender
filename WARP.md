# WARP.md

This file provides guidance to WARP (warp.dev) when working with code in this repository.

## CRITICAL! AI Rules

All AI instances MUST adhere to the rules found in `C:\Users\user\.ai-rules`. The key points are summarized below:

- **Session Runtime Indicator**: Every response must start with a session runtime tag (e.g., `XhYYmZZs > `).
- **File/Folder Operations**: No file or folder deletions/modifications without explicit user permission.
- **Backups**: Automatically back up files to `AI_backup` in the project root before any destructive operation and ensure `AI_backup/` is in `.gitignore`.
- **Build System**:
    - Use MinGW, wxWidgets, and CMake.
    - All builds must happen in a dedicated build folder (e.g., `build-mingw`).
    - Use the `build.bat` script for compilation, not CMake directly.
- **`.gitignore`**: Ensure that `.ai-rules`, `AI-RULES.md`, `RULES.txt`, `.warp-rules`, and `AI_backup/` are included in `.gitignore`.
- **Existing Code**: Check for existing implementations (like logging or notifications) before creating new ones.

## High-Level Architecture

This is a C++ desktop application for controlling CNC machines that run FluidNC firmware. The application is built using CMake and MinGW.

The core of the application is designed to be "machine-aware," meaning it dynamically discovers and adapts to the capabilities of the connected machine. This is a central design principle.

- **`MachineConfigManager`**: A singleton class that serves as the single source of truth for all machine-related data. It fetches and stores machine capabilities like workspace dimensions, feed rates, and firmware version.
- **`GCodeParser`**: A sophisticated G-code parser with a state machine, comprehensive command support, and detailed toolpath analysis. It can handle various G-code and M-code commands, and it provides detailed statistics about the parsed G-code.
- **Kinematics-Aware Homing**: The application supports different homing sequences for various machine kinematics (e.g., Cartesian, CoreXY). This ensures that machines are homed correctly based on their mechanical configuration.
- **JSON Configuration**: Machine profiles are stored in a `machines.json` file, allowing for persistent storage of machine capabilities.

## Commonly Used Commands

### Building the Project

To build the project, navigate to the `build-mingw` directory and run the `build.bat` script.

- **Standard Build**:
  ```bash
  cd build-mingw
  ./build.bat
  ```

- **Full Rebuild**:
  To perform a full rebuild, which cleans the CMake cache and all intermediate files, use the `--full` flag.
  ```bash
  cd build-mingw
  ./build.bat --full
  ```

### Running the Application

The `build.bat` script automatically starts the application after a successful build. You can also run the executable directly from the `build-mingw` directory:

```bash
cd build-mingw
./FluidNC_gCodeSender.exe
```

