<img align="left" width="100" height="100" src="icons/Icon.ico" alt="logo" style="float: left;"/>

<h3 align="left">
    PXLInstaller 
    <br />
    Install PXL applications on iPhone OS 1 with ease.
    <div align="right" style="float: top;" />
    <br />
</h3> 

## Information

> [!WARNING]
> There is no warranty for this software. Use it at your own risk.

- **macOS**, **Linux**, and **Windows** are supported
    - PXLInstaller was tested on Linux Mint. Mileage may vary with other distributions of Linux.
    - **Windows 10** and **11** are supported. You will need to manually rebind the USB drivers to `libusbK` with [Zadig](https://zadig.akeo.ie).

## How to Use
Download the relevant build for your operating system and architecture from the [Releases](https://github.com/theiphoneos1project/PXLInstaller/releases) tab.

- **macOS**:
    - For the CLI, make sure to run `chmod +x /path/to/binary` after downloading. If possible, run via `sudo` in order to force PXLInstaller's USB implementation to take priority over macOS. 
    - For the GUI, make sure to try running it for the first time, allowing it to run in Privacy & Security, and then running it again.
- **Linux**:
    - Make sure `libwxgtk3.2-dev` is installed from your package manager.
    - For both the CLI and GUI, make sure to run `chmod +x /path/to/binary` after downloading. Make sure to run via `sudo` in order for PXLInstaller to be able to temporarily disable `usbmuxd` and replace it with its own implementation while it is running.
- **Windows:**
    - Make sure to also install Zadig. Plug in your device. In Zadig, tick the "List All Devices" flag in the "Options" tab. Select the iPhone/iPod touch (Interface 1) and rebind the driver to `libusbK`. Keep in mind that if you switch devices, you will have to run the same steps for that device as well. If you have only one device on iPhone OS 1, you will not have to run the procedure multiple times.

## How to Compile Manually
Ensure you have [CMake](https://cmake.org), [vcpkg](https://vcpkg.io/en/), and [Python](https://www.python.org) installed.

To fetch all dependencies, please run:
```
python3 get_dependencies.py
```

### macOS (arm64)
```bash
cmake -B build/macos-arm64 -DCMAKE_OSX_ARCHITECTURES=arm64 -DVCPKG_TARGET_TRIPLET=arm64-osx -DVCPKG_OVERLAY_TRIPLETS=triplets -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

cmake --build build/macos-arm64 --target dist --clean-first
```

### macOS (x64)
```bash
cmake -B build/macos-x64 -DCMAKE_OSX_ARCHITECTURES=x86_64 -DVCPKG_TARGET_TRIPLET=x64-osx -DVCPKG_OVERLAY_TRIPLETS=triplets -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake

cmake --build build/macos-x64 --target dist --clean-first
```

### Linux
Make sure `wxWidgets` is installed from your package manager. The Linux build script does not rely on compiling `wxWidgets` as a dependency since that takes quite a long time on slow machines.
```bash
cmake -B build/linux-x64 -DVCPKG_TARGET_TRIPLET=x64-linux -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -DwxWidgets_CONFIG_EXECUTABLE=$(which wx-config)

cmake --build build/linux-x64 --target dist --clean-first
```

### Windows
Ensure you have [Visual Studio 17 2022](https://visualstudio.microsoft.com/vs/older-downloads/) installed. Make sure your environment is set up correctly. The build instructions assume you are using a PowerShell session with the Visual Studio Developer tools set up correctly. If `$VCPKG_ROOT` is not set up correctly, try entering the direct path to your `vcpkg` root.

```powershell
cmake -B build\windows -G "Visual Studio 17 2022" -A x64 -DVCPKG_TARGET_TRIPLET=x64-windows -DVCPKG_OVERLAY_TRIPLETS=triplets -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake

cmake --build build\windows --target dist --config Release --clean-first
```

## Credits
- [EthanArbuckle](https://github.com/EthanArbuckle) - original jailbreak ([iOS1.0-Jailbreak](https://github.com/EthanArbuckle/iOS1.0-Jailbreak)) which contains the USB implementation
- [forcequitOS](https://github.com/forcequitOS) - incredibly helpful in testing
- [pxl](https://code.google.com/archive/p/pxl/) - source code of PXLdaemon and other PXL components
- [The Apple Wiki PXL page](https://theapplewiki.com/wiki/PXL_File_Format) - good starting point for research on the format

## Copyright
This project is licensed under [MIT](LICENSE).

### Media and Asset Licenses
The project icon and associated visual assets are licensed under [CC BY-NC-SA 3.0](https://creativecommons.org). 

The icon incorporates elements from the following third-party works:
* "iPhone First Generation 8GB" by Carl Berkeley, used under [CC BY-SA 2.0](https://creativecommons.org) / Isolated and cropped from original source at [Wikimedia Commons](https://wikimedia.org).
* "High Quality Tileable Light Wood Texture 1" by Webtreats, used under [CC BY 2.0](https://creativecommons.org) / Blended and sourced from [Flickr](https://www.flickr.com/photos/webtreatsetc/4727355663).
* "Ripple water liquid" via [Picryl / Public Domain Media Repository](https://picryl.com/media/ripple-water-liquid-bf8262) (Dedicated to the Public Domain).
* "3d red arrow download icon" via [Vecteezy](https://www.vecteezy.com/png/52854442-3d-red-arrow-download-icon-perfect-for-websites-and-applications) (Free Commercial/Personal License with Attribution).

The device mockups are licensed under [CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/). 

The device mockups come from:
* [Rafael Fernandez (TheGoldenBox)](https://commons.wikimedia.org/wiki/User:TheGoldenBox), used under [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/)
    
###### Copyright (c) 2026 Nightwind
