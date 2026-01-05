# MetaLoader
[![Telegram](https://img.shields.io/badge/Telegram-Channel-2CA5E0?style=flat-square&logo=telegram&logoColor=white)](https://xmetaloader.t.me)  
**MetaLoader** is a mod loader for **Tanks Blitz** and **World of Tanks Blitz** (temporarily not supported), giving you full control over your game modifications.

> Manage your mods easily using MetaLoader!

## Installation
1. Download the latest release from the [Releases](https://github.com/cowega/MetaLoader/releases) page.
2. Unzip the contents of the archive into the game root folder (the folder containing `tanksblitz.exe` or `wotblitz.exe`). Confirm file replacement if prompted.
3. Create the `metaloader` folder (optional, created when the game starts) and put your modifications in it.
4. Enjoy!

## Usage
After installation, simply place your mod files into the `metaloader` folder. The loader will automatically inject them when the game starts.

**Directory structure example:**
```text
Game Folder/
├── tanksblitz.exe
├── metaloader.dll
└── metaloader/
    ├── crosshair/
    │   └── data/...
    ├── sounds/
    │   └── data/...
    └── metaloader.log.txt
```
> Ensure each mod folder contains the `Data` directory inside.

## Build
```powershell
git clone https://github.com/cowega/MetaLoader ml
cd ml
cmake -B build -A x64
cmake --build build --config Release
```
The `.dll` file will be placed in `build/bin` and also in the **Debug** or **Release** folder, depending on the configuration.

## License
Distributed under the MIT License. See `LICENSE` for more information.

## Built With
* [MinHook](https://github.com/TsudaKageyu/minhook) - The minimal x86/x64 API hooking library.
* [spdlog](https://github.com/gabime/spdlog) - Fast C++ logging library.
