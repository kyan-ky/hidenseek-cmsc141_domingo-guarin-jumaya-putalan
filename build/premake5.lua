-- premake5.lua
workspace("hidenseek-cmsc141_domingo-guarin-jumaya-putalan")
architecture("x64")
configurations({ "Debug", "Release" })
startproject("hidenseek-cmsc141_domingo-guarin-jumaya-putalan")

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories
IncludeDir = {}
IncludeDir["raylib"] = "/opt/homebrew/Cellar/raylib/5.5/include" -- IMPORTANT: Change this to your Raylib src path!
-- For macOS, if installed via Homebrew, it might be /usr/local/include or /opt/homebrew/include

-- Library directories
LibDir = {}
LibDir["raylib"] = "/opt/homebrew/Cellar/raylib/5.5/lib" -- IMPORTANT: Change this to your Raylib src path where .lib or .a is!
-- For macOS, if installed via Homebrew, it might be /usr/local/lib or /opt/homebrew/lib

project("hidenseek-cmsc141_domingo-guarin-jumaya-putalan")
kind("ConsoleApp")
language("C++")
cppdialect("C++17")
staticruntime("off")

targetdir("bin/" .. outputdir)
objdir("bin-int/" .. outputdir)

files({
	"../src/main.cpp",
	"../src/player.cpp",
	"../src/hider.cpp",
	"../src/game_manager.cpp",
	"../src/ui_manager.cpp",
	"../src/map.cpp",
	"../include/resource_dir.h", -- If it's compiled with the project
})

includedirs({
	"../include",
	"%{IncludeDir.raylib}",
})

libdirs({
	"%{LibDir.raylib}",
})

links({
	"raylib",
})

-- System-specific configurations
filter("system:windows")
systemversion("latest")
defines({ "PLATFORM_DESKTOP" })
files({ "srcsapplication.rc" }) -- Add resource file for Windows
links({ "opengl32", "gdi32", "winmm" }) -- Raylib dependencies on Windows

filter("system:linux")
defines({ "PLATFORM_DESKTOP" })
links({ "GL", "m", "pthread", "dl", "rt", "X11" }) -- Raylib dependencies on Linux

filter("system:macosx")
defines({ "PLATFORM_DESKTOP" })
-- Remove the frameworks from the 'links' table for macOS filter.
-- The main "raylib" link is handled by the project's global 'links' directive.
links({
	-- If there were any other non-framework system libraries needed specifically for macOS,
	-- they would go here (e.g., "pthread", "dl"). For now, it can be empty.
})
buildoptions({ "-std=c++17" })
-- Ensure linkoptions explicitly lists all necessary frameworks for Raylib.
linkoptions({
	"-framework OpenGL",
	"-framework Cocoa",
	"-framework IOKit",
	"-framework CoreAudio",
	"-framework CoreVideo",
	-- Add any other specific linker flags if needed.
})

filter("configurations:Debug")
defines("DEBUG")
symbols("On")

filter("configurations:Release")
defines("NDEBUG")
optimize("On")
