# Myst IV: Revolution 1.0.0
## By Anthony Kleine

Myst IV: Revolution is a command line tool that can perform various fixes for the game Myst IV: Revelation.

Currently, only the Windows Steam release of the game is supported. If you try with other releases please be aware they have not been tested. Support for other releases may be added in future.

This tool requires the Visual Studio 2019 C++ Redistributable to be installed for both [x86](https://aka.ms/vs/17/release/vc_redist.x86.exe) and [x64.](https://aka.ms/vs/17/release/vc_redist.x64.exe) If the tool or the game will not start, please ensure that both are installed.

Supports Windows 10 or 11, 64-bit, with an SSE4-capable CPU. Although Myst IV: Revolution itself is only about 45 MB large, it will create a backup of your game files, which requires up to 3 GB of free disk space.

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

Upon navigating, the game will play a fade transition. You may edit the transition time on a scale of zero to 500, where zero is instant and 500 is slow.

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

Fixes loading in order to make navigation faster. This operation will take several minutes to complete, because it works by converting the game assets into a format that is faster to load.

This operation will create a backup of your game files if one has not already been made.

### Restore Backup

Restore the backup to revert all changes made by the tool. If no backup was found, does nothing.

### Exit

Exit the application.

## Command Line Arguments

Command line arguments may be used for some advanced features.

 - `-p` or `--path`: sets an install path to use instead of the Steam install path
 - `-lfn` or `--log-file-names`: log the file names of all copied and converted files (slow, but useful for debugging)
 - `-nohw` or `--disable-hardware-acceleration`: disables hardware acceleration (via NVIDIA CUDA) when converting assets - if you do not have an NVIDIA graphics card, hardware acceleration will be disabled automatically
 - `-mt` or `--max-threads`: sets the maximum number of threads to use for multithreading when converting assets - if not set, it will be chosen automatically

# Dependencies

Myst IV: Revolution depends on these libraries.

- [scope_guard](https://github.com/Neargye/scope_guard) by Neargye
- [M4Image](https://github.com/tomysshadow/M4Image) by Anthony Kleine
- [libzap](https://github.com/HumanGamer/libzap) by HumanGamer
- [mango](https://github.com/t0rakka/mango) by t0rakka
- [Pixman](https://www.pixman.org) by freedesktop.org
- [NVIDIA Texture Tools 3](https://developer.nvidia.com/gpu-accelerated-texture-compression) by NVIDIA
- [sourcepp](https://github.com/craftablescience/sourcepp) by craftablescience