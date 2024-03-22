#include <windows.h>
#include <cmath>
#include "toml++/toml.hpp"
#include "nya_commonhooklib.h"

void GetDesktopResolution(int& horizontal, int& vertical) {
	RECT desktop;
	GetWindowRect(GetDesktopWindow(), &desktop);
	horizontal = desktop.right;
	vertical = desktop.bottom;
}

int nResX, nResY;
void ForceResolution() {
	*(int*)0x5569F0 = nResX;
	*(int*)0x5569F4 = nResY;
}

void __attribute__((naked)) ForceResolutionASM() {
	__asm__ (
		"pushad\n\t"
		"call %0\n\t"
		"popad\n\t"

		"mov eax, 1\n\t"
		"lea esp, [ebp-0x10]\n\t"
		"pop esi\n\t"
		"pop edx\n\t"
		"pop ecx\n\t"
		"pop ebx\n\t"
		"pop ebp\n\t"
		"ret\n\t"
			:
			:  "i" (ForceResolution)
	);
}

struct {
	float fFOVX;
	float fFOVY;
} *pFOV = nullptr;

void FOVScalingFix() {
	float fov = pFOV->fFOVX;

	// fix shadows
	if (fov == 0.78539819f) {
		return;
	}

	double fOrigAspect = 3.0 / 4.0;
	double fNewAspect = (double)nResY / (double)nResX;
	double fMultipliedAspect = fOrigAspect / fNewAspect;

	pFOV->fFOVX = atan2(tan(fov * 0.5) * fMultipliedAspect, 1.0) * 2.0;
	pFOV->fFOVY = atan2(tan(fov * 0.5) * fOrigAspect, 1.0) * 2.0;
}

uintptr_t FOVScalingASM_jmp = 0x424985;
void __attribute__((naked)) FOVScalingASM() {
	__asm__ (
		"add ebp, 0x10\n\t"
		"mov %2, ebp\n\t"
		"sub ebp, 0x10\n\t"

		"pushad\n\t"
		"call %0\n\t"
		"popad\n\t"

		"fld dword ptr [ebp+0x10]\n\t" // fov x
		"fmul st, st(1)\n\t"
		"fptan\n\t"
		"fstp st\n\t"
		"fld dword ptr [ebp+0x14]\n\t" // fov y
		"fmulp st(2), st\n\t"
		"fxch st(1)\n\t"
		"fptan\n\t"
		"jmp %1\n\t"
			:
			:  "i" (FOVScalingFix), "m" (FOVScalingASM_jmp), "m" (pFOV)
	);
}

double __stdcall ScreenResolutionPatch(float a1) {
	if (*(int*)0x556A48 == 640) return a1;

	auto ret = (*(int*)0x556A48 * (1.0 / 640.0) * a1);

	auto aspect = (double)nResX / (double)nResY;
	auto origAspect = 4.0 / 3.0;
	ret *= origAspect;
	ret /= aspect;

	return ret;
}

int ScreenResolutionPatch2_RetValue = 0;
// a1 appears to be some sort of replacement struct to return for this, but it doesn't actually ever seem to be used
void __fastcall ScreenResolutionPatch2(int a1) {
	auto ret = *(int*)0x556A48;
	auto aspect = (double)nResX / (double)nResY;
	auto origAspect = 4.0 / 3.0;
	ret *= origAspect;
	ret /= aspect;
	ScreenResolutionPatch2_RetValue = ret;
}

void __attribute__((naked)) ScreenResolution2ASM() {
	__asm__ (
		"pushad\n\t"
		"mov ecx, eax\n\t"
		"call %0\n\t"
		"popad\n\t"
		"mov eax, %1\n\t"
		"ret\n\t"
			:
			:  "i" (ScreenResolutionPatch2), "m" (ScreenResolutionPatch2_RetValue)
	);
}

const char* sSplashScreens[] = { "graphics\\bkgs\\boot1.psd", "graphics\\bkgs\\boot2.psd", "graphics\\bkgs\\boot3.psd" };

uintptr_t FullSplashesPatch_CallAddress = 0x4AF560;
uintptr_t FullSplashesPatch_RetAddress = 0x44BC28;
void __attribute__((naked)) FullSplashesASM() {
	__asm__ (
		"mov eax, %0\n\t"
		"call %3\n\t"
		"push 2500\n\t"
		"call %5\n\t"
		"mov eax, %1\n\t"
		"call %3\n\t"
		"push 2500\n\t"
		"call %5\n\t"
		"mov eax, %2\n\t"
		"call %3\n\t"
		"jmp %4\n\t"
			:
			:  "m" (sSplashScreens[0]), "m" (sSplashScreens[1]), "m" (sSplashScreens[2]), "m" (FullSplashesPatch_CallAddress), "m" (FullSplashesPatch_RetAddress), "i" (Sleep)
	);
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			auto config = toml::parse_file("TIRTweaks_gcp.toml");
			bool bNoCD = config["main"]["no_cd"].value_or(true);
			bool bNoVideos = config["main"]["no_videos"].value_or(false);
			bool bSkipIntro = config["main"]["skip_intro"].value_or(false);
			bool bFixScaling = config["main"]["fix_scaling"].value_or(true);
			bool bHUDPatches = config["main"]["fix_hud"].value_or(false);
			int nShadowResolution = config["extras"]["shadow_res"].value_or(128);
			bool bFullSplash = config["extras"]["full_splash"].value_or(false);
			nResX = config["extras"]["res_x"].value_or(0);
			nResY = config["extras"]["res_y"].value_or(0);

			// default to desktop resolution
			if (nResX <= 0 || nResY <= 0) {
				GetDesktopResolution(nResX, nResY);
			}

			if (bNoCD) {
				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x44BA69, 0x44BA7B); // :3
			}

			if (bNoVideos) {
				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x44BC3E, 0x44BC58); // no intro
				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x4B104D, 0x4B1060); // no background videos
				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x424607, 0x424691); // skip all videos
			}

			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x44BA23, &ForceResolutionASM);

			if (bFixScaling) {
				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x424973, &FOVScalingASM);
			}

			NyaHookLib::Patch(0x4ADF26 + 1, nShadowResolution);

			if (bSkipIntro) {
				if (!bFullSplash) {
					// remove splash screens
					NyaHookLib::Patch<uint8_t>(0x4AF560, 0xC3);
					// remove sleep from "intro events"
					NyaHookLib::Patch<uint8_t>(0x4AF868 + 1, 0);
					// set "intro events" length to 0.01s
					NyaHookLib::Patch(0x44BC28 + 1, 0.01f);
				}
				// don't play intro video
				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x44BC3E, 0x44BC58);
			}

			if (bFullSplash) {
				// add all 3 splash screens, each one taking 2.5 seconds
				// this'll still slightly increase boot times from 5s to 7.5s but this is optional anyway, nya~
				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x44BC1E, &FullSplashesASM);
				NyaHookLib::Patch(0x44BC29, 2.5f);
			}

			if (bHUDPatches) {
				// sprites
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x41BE21, &ScreenResolutionPatch);
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x41BE71, &ScreenResolutionPatch);

				// text
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x418E8D, &ScreenResolutionPatch);
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4190BC, &ScreenResolutionPatch);
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x419D73, &ScreenResolutionPatch);

				// trackmap icons
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4D97A5, &ScreenResolutionPatch);
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4D97D7, &ScreenResolutionPatch);
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4D9927, &ScreenResolutionPatch);
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4D98AD, &ScreenResolution2ASM);

				// speedo
				// speed number size
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4DA701, &ScreenResolution2ASM);
				// speed number spacing
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4DA6BF, &ScreenResolution2ASM);
				// abs/tcs
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4DB7EC, &ScreenResolution2ASM);
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4DB82A, &ScreenResolution2ASM);
				// gear number
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4DB701, &ScreenResolution2ASM);
				// kmh/mph
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4DB19A, &ScreenResolution2ASM);
				// position
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4DAAA5, &ScreenResolutionPatch);

				// todo 904E10 array of speedo rpm counters, is this scaled correct?
				// todo check around 42F178 and 4DBDE7 for needle rotation, seems to be an actual 3d draw
				// todo for menus: check 0x44087C 0x4412F0 and 0x4451D7
			}
		} break;
		default:
			break;
	}
	return TRUE;
}