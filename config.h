#pragma once

namespace config
{
	// Form lookup cache + distant tree LOD alpha update form cache
	// equivalent to SSE fixes
	extern bool patchFormCaching;

	// Replace BSReadWriteLock implementation
	extern bool patchBSReadWriteLock;

	// Ports from LE bug fixes mod
	extern bool patchDoublePerkApply;
	extern bool patchSlowTimeCameraMovement;
	extern bool patchVerticalLookSensitivity;

	// decouple waterflow timer from timescale
	extern bool patchWaterflowTimer;
	extern float waterflowTimescale;
	// fix blocky reflections on tree LODs
	extern bool patchTreeReflections;

	bool LoadConfig(const std::string& path);
}