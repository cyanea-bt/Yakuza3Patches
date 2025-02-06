# Yakuza 3 HeatFix
This patch attempts to fix these Heat related issues in **Yakuza 3 Remastered**:  
* Heat starts to drain too soon after gaining Heat/hitting an enemy
* Heat drops too quickly once it starts to drain  
  
**Y3R's** internal logic runs 60 times per second **(even if framelimit is set to 30 fps)**.  
The function that calculates Heat gain/drain is hardcoded assuming 30 updates per second.  
So that is why the Heat drain starts after about half the time it takes on PS3 and also why  
the drain rate is ~2x of what it is on PS3.  

Keep in mind that I chose not to rewrite this function for 60 updates per second. That'd be  
**a lot** more work and also might introduce even more inconsistencies/bugs. So all this patch  
really does, is to make sure that the Heat gain/drain calculations happen 30 times per second.  
I also compared the patched Heat drain to the PS3 original and they look pretty much identical.

## Compatibility
I only tested this patch on the [most recent version](https://steamdb.info/patchnotes/6407476/) of Y3R.  
Other versions might work as well, YMMV.  

**SHA-1** checksums for compatible binaries (in case you want to check yours):  
`20d9614f41dc675848be46920974795481bdbd3b` `Yakuza3.exe (Steam)`  
`2a55a4b13674d4e62cda2ff86bc365d41b645a92` `Yakuza3.exe (Steam without SteamStub)`  
`6c688b51650fa2e9be39e1d934e872602ee54799` `Yakuza3.exe (GOG)`  

## Install
Mod is now compatible with [ShinRyuModManager](https://github.com/SRMM-Studio/ShinRyuModManager), so simply download and install like all other SRMM mods.  

If you don't want to use [ShinRyuModManager](https://github.com/SRMM-Studio/ShinRyuModManager), you can also opt to use any other ASI Loader instead (like the one included with [SilentPatchYRC](https://github.com/CookiePLMonster/SilentPatchYRC)). In that case you should extract `Yakuza3HeatFix.asi` to the same directory as `Yakuza3.exe` AND `dinput8.dll`.

### Verify that the patch is working as intended (optional)
1. Start the game after installing/enabling the mod.  
2. After you get to the main menu, there should now be a `Yakuza3HeatFix.txt` next to `Yakuza3.exe`.  
3. Open this text file and verify that the first 5 lines read:  
  
```
Found pattern: HeatUpdate
Found pattern: IsPlayerInCombat
Found pattern: IsCombatInactive
Found pattern: IsCombatFinished
Hook done!
```

## Build (Windows)
**You'll need git, premake5 and msbuild available on your PATH.**  
Swap `vs2019` for whatever version you are running on your system (e.g. `vs2022`).  

```
git clone --recursive https://github.com/cyanea-bt/Yakuza3HeatFix.git
cd Yakuza3HeatFix
premake5 vs2019
cd build
msbuild Yakuza3HeatFix.sln /t:Rebuild /m /p:Configuration=Release;Platform=Win64
```
