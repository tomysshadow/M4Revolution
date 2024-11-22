# Myst IV: Revolution 1.0.0
## By Anthony Kleine

Myst IV: Revolution is a command line tool that can perform various fixes for the game Myst IV: Revelation.

Currently, only the Windows Steam release of the game is supported. If you try with other releases please be aware they have not been tested. Support for other releases may be added in future.

This tool requires the Visual Studio 2019 C++ Redistributable to be installed for both [x86](https://aka.ms/vs/17/release/vc_redist.x86.exe) and [x64.](https://aka.ms/vs/17/release/vc_redist.x64.exe) If the tool or the game will not start, please ensure that both are installed.

Supports Windows 10 or 11, 64-bit, with an SSE4-capable CPU and at least 1 GB of RAM. Although Myst IV: Revolution itself is only about 60 MB large, it will create a backup of your game files, which requires up to 3 GB of free disk space.

Usage: `M4Revolution [-p path -lfn -nohw -mt maxThreads]`

# How to Use Myst IV: Revolution

If you would like to download the binary, please go to the [Releases page.](https://github.com/tomysshadow/M4Revolution/releases)

## Finding the Install Path

Upon running Myst IV: Revolution, it will attempt to automatically find the install path of the game.

Usually, you should just press Enter to continue. If you are certain you would like to use a different install path, press the n key, then press Enter, and input the install path you would like to use instead.

If the tool fails to automatically find the install path, you will need to find it yourself manually. To do this:

1. Right click on Myst IV: Revelation in your Steam Library and go to Manage > Browse local files...
2. In the Explorer window that appears, right click on the Address Bar, then click Copy address as text.
3. In Myst IV: Revolution's window, click the icon in the title bar at the top left, then go to Edit > Paste, then press Enter.

If after following these steps the install path is not accepted, try verifying the integrity of your game files. Right click on Myst IV: Revelation in your Steam Library and go to Properties > Installed Files > Verify integrity of game files. Then repeat the steps to find the install path manually again.

## Operations

Once the install path has been found, you will be presented a menu that allows you to perform the following operations.

1. Open Online Help
2. Edit Transition Time
3. Toggle Sound Fading
4. Toggle Camera Inertia
5. Fix Loading
6. Restore Backup
7. Exit

Here are detailed descriptions of each operation.

### Open Online Help

Opens this file.

### Edit Transition Time

Upon navigating, the game will play a fade transition. You may edit the transition time on a scale of zero to 500, where zero is instant and 500 is slow. The currently set transition time is displayed. The default is 500.

This operation will create a backup of your game files if one has not already been made.

### Toggle Sound Fading

Upon navigating, the game inserts an intentional half a second delay to crossfade sounds from one place to another. You may toggle the sound fading on or off. When the sound fading is off, the sounds will cut instantly to their volume at the new location instead of fading, allowing faster navigation.

If sound fading is on, it will be turned off, and vice-versa. The tool will then report whether sound fading has been toggled on or off.

This operation will create a backup of your game files if one has not already been made.

### Toggle Camera Inertia

When moving the camera, there is an inertia applied to the movement. You may toggle the camera inertia on or off. When the camera inertia is off, mouse movements will translate directly into camera movements, effectively disabling mouse acceleration.

If camera inertia is on, it will be turned off, and vice-versa. The tool will then report whether camera inertia has been toggled on or off.

This operation will create a backup of your game files if one has not already been made.

### Fix Loading

Fixes loading in order to make navigation faster. This operation will take several minutes to complete, because it works by converting the assets into a format that is faster to load.

This operation will create a backup of your game files if one has not already been made.

### Restore Backup

Restore the backup to revert all changes made by the tool. If no backup was found, does nothing.

### Exit

Exit the application.

## Command Line Arguments

Command line arguments may be used for some advanced features.

 - `-p path` or `--path path`: sets an install path to use - if not set, the Steam install path is found automatically
 - `-lfn` or `--log-file-names`: log the file names of all copied and converted files (slow, but useful for debugging)
 - `-nohw` or `--disable-hardware-acceleration`: disables hardware acceleration (via NVIDIA CUDA) when converting assets - if you do not have an NVIDIA graphics card, hardware acceleration will be disabled automatically
 - `-mt maxThreads` or `--max-threads maxThreads`: sets the maximum number of threads to use for multithreading when converting assets - maxThreads must be a valid number, and if not set, it will be chosen automatically

## Compiling for Windows with Visual Studio

If you are a user who would like to use this tool, please download it from the [Releases page.](https://github.com/tomysshadow/M4Revolution/releases)

If you are a developer who would like to contribute, you may compile for Windows by following these steps. Compiling Myst IV: Revolution requires Visual Studio 2019 or newer.

1. Compile [libzap](https://github.com/HumanGamer/libzap) for both x86 (Win32) and x64 with CMake using the default settings, and your Visual Studio version.
2. Copy the resulting libzap.lib, M4Image.lib, and mango.lib files to `vendor/libzap/lib`, `vendor/M4Image/lib`, `vendor/mango/lib` folders respectively, categorized by architecture and configuration. For instance: the x64, Debug libzap.lib should be located at `vendor/libzap/x64/Debug/libzap.lib`.
3. Copy the resulting pixman-1_staticd.lib files to `vendor/pixman-1/lib/x86/Debug` and `vendor/pixman-1/lib/x64/Debug` respectively.
4. Copy the resulting pixman-1_static.lib files to `vendor/pixman-1/lib/x86/Release` and `vendor/pixman-1/lib/x64/Release` respectively.
5. Install [NVIDIA Texture Tools 3.](https://developer.nvidia.com/gpu-accelerated-texture-compression) This requires an NVIDIA developer account.
6. Copy the nvtt include files to `vendor/nvtt/include`.
7. Copy the nvtt30205.lib for your Visual Studio version from the install directory to `vendor/nvtt/lib/x64`.
8. Copy nvtt30205.dll from the install directory to `x64/Debug` and `x64/Release`.
9. Compile [sourcepp](https://github.com/craftablescience/sourcepp) for x64 with CMake, and your Visual Studio version. You may optionally uncheck all of the `SOURCEPP_USE` settings, except for `SOURCEPP_USE_STEAMPP`.
10. Copy the sourcepp include files to `vendor/sourcepp/include`.
11. Copy the resulting sourcepp.lib, kvpp.lib, bsppp.lib, and steampp.lib files to `vendor/sourcepp/lib/x64/Debug` and `vendor/sourcepp/lib/x64/Release` respectively.
12. Download GetDLLExportRVA from its [Releases page.](https://github.com/tomysshadow/GetDLLExportRVA/release)
13. Copy the **x86** Debug GetDLLExportRVA files to `x64/Debug`. Notice that the **x86** Debug build must be copied to the **x64** Debug folder.
14. Copy the **x86** Release GetDLLExportRVA files to `x64/Release`. Notice that the **x86** Release build must be copied to the **x64** Release folder.
15. Open the M4Revolution solution in your Visual Studio version.
16. Build the solution for x86 first. It must be built for x86 first because the x64 M4Revolution project includes the x86 gfx_tools_rd.dll as a resource.
17. After building the solution for x86, build the solution for x64.

# Do I need to use this tool on the same computer I play the game on?

Yes. The files produced by this tool will be optimized for the computer it is used on. You should use it on the same computer that you play the game on, rather than using it on one computer and copy pasting the game files to another computer.

# Is this the definitive way to play Myst IV: Revelation?

Not necessarily. I would consider the long loading times to be a bug (albeit not a compatibility bug - it was always like this, even when the game was new.) However, the fade transition and sound fading are intentional features, and although editing or disabling them allows for instant movement, doing so arguably goes against the intended artistic vision of the game. Different players will have different preferences. I personally prefer the game with camera inertia enabled, like it is by default. As such, I would prefer that this remain a seperate, optional enhancement, and not be integrated into the game in an official capacity.

# Dependencies

Myst IV: Revolution depends on these libraries.

- [scope_guard](https://github.com/Neargye/scope_guard) by Neargye
- [M4Image](https://github.com/tomysshadow/M4Image) by Anthony Kleine
- [libzap](https://github.com/HumanGamer/libzap) by HumanGamer
- [mango](https://github.com/t0rakka/mango) by t0rakka
- [Pixman](https://www.pixman.org) by freedesktop.org
- [NVIDIA Texture Tools 3](https://developer.nvidia.com/gpu-accelerated-texture-compression) by NVIDIA
- [half](https://sourceforge.net/projects/half/) by rauy
- [sourcepp](https://github.com/craftablescience/sourcepp) by craftablescience
- [GetDLLExportRVA](https://github.com/tomysshadow/GetDLLExportRVA) by Anthony Kleine