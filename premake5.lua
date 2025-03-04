workspace "Yakuza3Patches"
	platforms { "Win64" }

project "Yakuza3HeatFix"
	kind "SharedLib"
	targetextension ".asi"
	language "C++"

	defines { 
	    "rsc_FullName=\"Yakuza3HeatFix\"",
	    "rsc_MajorVersion=0",
	    "rsc_MinorVersion=0",
	    "rsc_RevisionID=1",
	    "rsc_BuildID=0",
	}

project "Yakuza3EasyButtonSpam"
	kind "SharedLib"
	targetextension ".asi"
	language "C++"

	defines { 
	    "rsc_FullName=\"Yakuza3EasyButtonSpam\"",
	    "rsc_MajorVersion=0",
	    "rsc_MinorVersion=0",
	    "rsc_RevisionID=1",
	    "rsc_BuildID=0",
	}

project "Yakuza3EnemyAIFix"
	kind "SharedLib"
	targetextension ".asi"
	language "C++"

	defines { 
	    "rsc_FullName=\"Yakuza3EnemyAIFix\"",
	    "rsc_MajorVersion=0",
	    "rsc_MinorVersion=0",
	    "rsc_RevisionID=1",
	    "rsc_BuildID=0",
	}


workspace "*"
	configurations { "Debug", "Release", "Master" }
	location "build"

	vpaths { 
		["Headers/*"] = "source/**.h",
		["Sources/*"] = { "source/**.c", "source/**.cpp" },
		["Resources"] = "source/**.rc"
	}

	files { "**/MemoryMgr.h", "**/Trampoline.h", "**/Patterns.cpp", "**/Patterns.h", "**/HookInit.hpp" }
	files { "source/*.h", "source/*.cpp", "source/%{prj.name}/*.h", "source/%{prj.name}/*.cpp", "source/resources/*.rc" }
	includedirs { "source", "source/%{prj.name}" }
	externalincludedirs { "3rdparty/json/include", "3rdparty/fmt/include", "3rdparty/spdlog/include" }

	-- Disable exceptions in WIL
	defines { "WIL_SUPPRESS_EXCEPTIONS" }

	cppdialect "C++20"
	staticruntime "on"
	buildoptions { "/sdl", "/utf-8" }
	warnings "Extra"

	-- Automated defines for resources
	defines { 
		"rsc_Extension=\"%{prj.targetextension}\"",
		"rsc_Name=\"%{prj.name}\"",
		"rsc_Game=\"Yakuza 3\"",
		"rsc_Repository=\"https://github.com/cyanea-bt/Yakuza3Patches\"",
		"rsc_UpdateURL=\"https://github.com/cyanea-bt/Yakuza3Patches/releases\"",
		"rsc_Copyright=\"2025 cyanea-bt\""
	}

	newoption {
		trigger = "with-postbuild",
		description = "Set up default Post-Build Event"
	}

	newoption {
		trigger = "with-dbg-cmd",
		description = "Set up default debug command"
	}

	filter { "options:with-postbuild" }
		postbuildcommands { "xcopy /V /F /Y \"%{!cfg.buildtarget.abspath}\" \"C:/Games/Yakuza 3/mods/%{prj.name}/\"" }

	filter { "options:with-dbg-cmd" }
		debugdir "C:/Games/Yakuza 3"
		debugcommand "C:/Games/Yakuza 3/Yakuza3.exe"

filter "configurations:Debug"
	defines { "DEBUG", "_DEBUG" }
	runtime "Debug"

 filter "configurations:Master"
	symbols "Off"

filter "configurations:not Debug"
	defines { "NDEBUG", "_NDEBUG" }
	optimize "Speed"
	functionlevellinking "on"
	linktimeoptimization "on"

filter { "platforms:Win32" }
	system "Windows"
	architecture "x86"

filter { "platforms:Win64" }
	system "Windows"
	architecture "x86_64"

filter { "toolset:*_xp"}
	defines { "WINVER=0x0501", "_WIN32_WINNT=0x0501" } -- Target WinXP
	buildoptions { "/Zc:threadSafeInit-" }

filter { "toolset:not *_xp"}
	defines { "WINVER=0x0601", "_WIN32_WINNT=0x0601" } -- Target Win7
	buildoptions { "/permissive-" }