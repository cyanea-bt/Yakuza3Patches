# Yakuza 3 HeatFix
This patch fixes a multitude of Heat related issues in **Yakuza 3 Remastered**:  
* Heat drain starting too soon after gaining Heat/hitting an enemy  
* Heat dropping too quickly once it starts to drain  
* Heat gain from some equipment not matching the PS3 version  
* Heat gain from some abilities not matching the PS3 version  
* Heat gain from long press combo finishers not matching the PS3 version  
  
**Y3R's** internal logic runs 60 times per second **(even if framelimit is set to 30 fps)**.  
When Yakuza 3 was "remastered", **A LOT** of its code was not reworked to take the doubled update rate  
into account. So whenever something was supposed to happen once per frame at 30 times a second  
on PS3, there's a pretty good chance that it is messed up in Y3R. That is why Heat drain starts after about  
half the time it takes on PS3 and also why the drain rate is twice of what it is on PS3.  
  
An earlier version of my patch tried to fix these issues by limiting the function, that is responsible for most of  
them, to 30 updates a second. This worked well enough as far as the drain rate goes but it wasn't without issues.  
What I didn't know at the time was that there are several sources of Heat gain/drain that are only "active" for a  
single frame. E.g. if the player hit the enemy on a frame where the Heat function was skipped by my patch, the  
corresponding Heat update wouldn't happen and the player wouldn't gain any Heat for that hit. This goes both  
ways btw - if the enemy hit the player on a skipped frame the player wouldn't lose any Heat for being hit. So at  
least it was "fair" I guess but certainly not accurate to the PS3 version.  
  
There really wasn't a good way to fix these issues while limiting the Heat updates to 30 times per second.  
In the end I decided to rewrite my patch from scratch in order to fix all the 60 fps related flaws in Y3R's  
Heat update functions. This involved a lot of reverse engineering of Y3R's code and also a lot of frame by frame  
comparisons with the PS3 original. Every source of continuous Heat gain/drain had to be looked at and fixed  
to work correctly at 60 updates per second. I'm pretty sure I managed to find all of them but of course there's  
always the possibility that I might have missed something.  

## Compatibility
I only tested my patch on the [most recent version](https://steamdb.info/patchnotes/6407476/) of Y3R.  
Other versions might work as well, YMMV.  

**SHA-1** checksums for compatible binaries (in case you want to check yours):  
`20d9614f41dc675848be46920974795481bdbd3b` `Yakuza3.exe (Steam)`  
`2a55a4b13674d4e62cda2ff86bc365d41b645a92` `Yakuza3.exe (Steam without SteamStub)`  
`6c688b51650fa2e9be39e1d934e872602ee54799` `Yakuza3.exe (GOG)`  
  
## Configuration
Patch versions `2025.03.09` or later have support for some user-configurable settings. I obviously recommend  
sticking with the defaults but the option's there if you want it. After running the game for the first time with my  
patch installed/enabled, 2 files will be generated in the patch's directory (`mods\Yakuza3HeatFix`).  
  
`Yakuza3HeatFix_Info.txt` will contain some information about each option and its supported values.  
  
`Yakuza3HeatFix.json` is the configuration file in which you can change these options. Simply open it in any  
text editor (e.g. Notepad) and change what you want to change. Remember to save after you're done and then  
restart the game if it is already running. If you try to use any invalid settings or mess up the configuration file  
in some other way, the option(s) will be reset to their defaults.  
  
## Install
This mod is compatible with [ShinRyuModManager](https://github.com/SRMM-Studio/ShinRyuModManager), so simply download and install like all other SRMM mods.  

If you don't want to use [ShinRyuModManager](https://github.com/SRMM-Studio/ShinRyuModManager), you can also opt to use any other ASI Loader instead (like the one included with [SilentPatchYRC](https://github.com/CookiePLMonster/SilentPatchYRC)). In that case you should extract `Yakuza3HeatFix.asi` to the same directory as `Yakuza3.exe` AND `dinput8.dll`.

### Verify that the patch is working as intended (optional)
1. Start the game after installing/enabling the mod.  
2. After you get to the main menu, there should now be a `Yakuza3HeatFix.txt` next to `Yakuza3.exe`.  
3. Open this text file and verify that the first 9 lines read:  
  
```
Found pattern: HeatDrainTimer
Found pattern: RescueCard
Found pattern: PhoenixSpirit
Found pattern: TrueLotusFlareFist
Found pattern: GoldenDragonSpirit
Found pattern: LeechGloves
Found pattern: LongPressComboFinisher
Found pattern: UpdateHeat
Hook done!
```
That's assuming you run all settings at default of course.  
Depending on your chosen settings in `Yakuza3HeatFix.json` some of these might be missing.  

## Build (Windows)
**You'll need git, premake5 and msbuild available on your PATH.**  
Swap `vs2019` for whatever version you are running on your system (e.g. `vs2022`).  

```
git clone --recursive https://github.com/cyanea-bt/Yakuza3Patches.git
cd Yakuza3Patches
premake5 vs2019
cd build
msbuild Yakuza3Patches.sln /t:Yakuza3HeatFix;Rebuild /m /p:Configuration=Release;Platform=Win64
```
