#include <windows.h>
#include <cmath>
#include "toml++/toml.hpp"

#include "nya_commonhooklib.h"
#include "nya_dx8_hookbase.h"

void GetDesktopResolution(int& horizontal, int& vertical) {
	RECT desktop;
	GetWindowRect(GetDesktopWindow(), &desktop);
	horizontal = desktop.right;
	vertical = desktop.bottom;
}

int nResX43;
double fDisplayAspect = 0;

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
	double fNewAspect = 1.0 / fDisplayAspect;
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

	// hack for fullscreen sprites to stay fullscreen
	if (a1 == 640.0f) return ret;

	auto aspect = fDisplayAspect;
	auto origAspect = 4.0 / 3.0;
	ret *= origAspect;
	ret /= aspect;

	return ret;
}

int ScreenResolutionPatch2_RetValue = 0;
// a1 appears to be some sort of replacement struct to return for this, but it doesn't actually ever seem to be used
void __fastcall ScreenResolutionPatch2(int a1) {
	auto ret = *(int*)0x556A48;
	auto aspect = fDisplayAspect;
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

float fLetterboxLeft, fLetterboxRight;
float fResY;

uintptr_t MovieLetterboxPatch_RetAddress = 0x424880;
void __attribute__((naked)) MovieLetterboxASM() {
	__asm__ (
		"mov edi, %2\n\t"
		"push edi\n\t"
		"mov edi, %1\n\t"
		"push edi\n\t"
		"push 0\n\t"
		"mov edi, %0\n\t"
		"push edi\n\t"
		"jmp %3\n\t"
			:
			:  "m" (fLetterboxLeft), "m" (fLetterboxRight), "m" (fResY), "m" (MovieLetterboxPatch_RetAddress)
	);
}

const float fSpeedoXOffset = -2.5;
const float fSpeedoYOffset = -4;
const float fSpeedoSize = 0.32;
float fSpeedoRotation = 0;
void HookLoop() {
	static auto pTexture = LoadTexture("speedo.png");
	if (!pTexture) return;

	float fPosX = *(float*)0x904EF0 / (double)nResX;
	float fPosY = *(float*)0x904EF4 / (double)nResY;
	float fScaleY = fSpeedoSize;
	float fScaleX = fScaleY * GetAspectRatioInv();
	fPosX += fSpeedoXOffset / 640.0;
	fPosY += fSpeedoYOffset / 640.0;
	DrawRectangle(fPosX,fPosX + fScaleX,fPosY,fPosY + fScaleY,{255,255, 255, 255}, 0, pTexture, fSpeedoRotation);

	CommonMain();
}

bool bDeviceJustReset = false;
void SpeedoCustomDraw() {
	if (!g_pd3dDevice) {
		g_pd3dDevice = *(IDirect3DDevice8 **) 0x556A64;
		InitHookBase();
	}

	if (bDeviceJustReset) {
		ImGui_ImplDX8_CreateDeviceObjects();
		bDeviceJustReset = false;
	}
	HookBaseLoop();
}

void __attribute__((naked)) SpeedoDrawASM() {
	__asm__ (
		"pushad\n\t"
		"call %0\n\t"
		"popad\n\t"
		"lea esp, [ebp-0x10]\n\t"
		"pop edi\n\t"
		"pop esi\n\t"
		"pop ecx\n\t"
		"pop ebx\n\t"
		"pop ebp\n\t"
		"ret\n\t"
			:
			:  "i" (SpeedoCustomDraw)
	);
}

uintptr_t SpeedoGetAngle_RetAddress = 0x4DBCC9;
void __attribute__((naked)) SpeedoGetAngleASM() {
	__asm__ (
		"fstp dword ptr [eax+0x24]\n\t"
		"push ebx\n\t"
		"mov ebx, [eax+0x24]\n\t"
		"mov %0, ebx\n\t"
		"pop ebx\n\t"
		"jmp %1\n\t"
			:
			:  "m" (fSpeedoRotation), "m" (SpeedoGetAngle_RetAddress)
	);
}

auto SpeedoDrawD3DReset_RetAddress = (int(*)())0x40E700;
int SpeedoDrawD3DReset() {
	if (g_pd3dDevice) {
		ImGui_ImplDX8_InvalidateDeviceObjects();
		bDeviceJustReset = true;
	}
	return SpeedoDrawD3DReset_RetAddress();
}

void __stdcall SetWindowPosPatch(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags) {
	SetWindowPos(hWnd, hWndInsertAfter, 0, 0, cx, cy, uFlags);
}

auto DrawSplashScreen = (void(__stdcall*)(float, float, float, float, void*, int))0x41A440;
void __stdcall SplashScreenLetterboxPatch(float a1, float a2, float a3, float a4, void* a5, int a6) {
	DrawSplashScreen(fLetterboxLeft, a2, fLetterboxRight, a4, a5, a6);
}

int nMultisamplingType = 0;
auto pD3DMultisamplingType = (int*)0x556A18;
uintptr_t SetMultisampling_RetAddress = 0x40EA07;
void __attribute__((naked)) SetMultisamplingASM() {
	__asm__ (
			"push eax\n\t"
			"push ebx\n\t"
			"mov eax, %0\n\t"
			"mov ebx, %1\n\t"
			"mov [ebx], eax\n\t"
			"pop eax\n\t"
			"pop ebx\n\t"
			"test    ebx, ebx\n\t"
			"setnz   al\n\t"
			"mov     esi, 1\n\t"
			"and     eax, 0xFF\n\t"
			"mov     edi, 0x50\n\t"
			"add     eax, 2\n\t"
			"jmp %2\n\t"
			:
			:  "m" (nMultisamplingType), "m" (pD3DMultisamplingType), "m" (SetMultisampling_RetAddress)
			);
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			auto config = toml::parse_file("TIRTweaks_gcp.toml");
			bool bNoCD = config["main"]["no_cd"].value_or(true);
			bool bNoVideos = config["main"]["no_videos"].value_or(false);
			bool bBorderlessWindowed = config["main"]["borderless_windowed"].value_or(false);
			bool bSkipIntro = config["main"]["skip_intro"].value_or(false);
			bool bFixScaling = config["main"]["fix_scaling"].value_or(true);
			bool bHUDPatches = config["main"]["fix_hud"].value_or(false);
			bool bSplashAndMoviePatches = config["main"]["letterbox_movies"].value_or(true);
			nMultisamplingType = config["main"]["multisampling"].value_or(0);
			int nShadowResolution = config["extras"]["shadow_res"].value_or(128);
			int nReflectionResolution = config["extras"]["reflection_res"].value_or(128);
			bool bFullSplash = config["extras"]["full_splash"].value_or(false);
			bool bNoNeedleFix = config["extras"]["no_needle_fix"].value_or(0);
			nResX = config["extras"]["res_x"].value_or(0);
			nResY = config["extras"]["res_y"].value_or(0);
			fDisplayAspect = config["extras"]["aspect_ratio"].value_or(0.0f);

			// default to desktop resolution
			if (nResX <= 0 || nResY <= 0) {
				GetDesktopResolution(nResX, nResY);
			}

			nResX43 = nResY * (4.0 / 3.0);
			fResY = nResY;

			if (fDisplayAspect <= 0) {
				fDisplayAspect = (double)nResX / (double)nResY;
			}

			if (bNoCD) {
				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x44BA69, 0x44BA7B); // :3
			}

			if (bNoVideos) {
				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x44BC3E, 0x44BC58); // no intro
				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x4B104D, 0x4B1060); // no background videos
				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x424607, 0x424691); // skip all videos
			}

			if (nMultisamplingType > 0) {
				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x40E9F0, SetMultisamplingASM);
			}

			if (bBorderlessWindowed) {
				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x44BBCD, 0x44C0ED);
				NyaHookLib::Patch(0x40EABA + 1, 0x20000);
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x40EBAE, &SetWindowPosPatch);
			}

			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x44BA23, &ForceResolutionASM);

			if (bFixScaling) {
				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x424973, &FOVScalingASM);

				// adjust draw distance to scale with ratio, doesn't seem to work
				//NyaHookLib::Patch(0x4ACAA9 + 2, &fDisplayAspect);
			}

			NyaHookLib::Patch(0x4ADF26 + 1, nShadowResolution);
			NyaHookLib::Patch(0x4ADEFC + 1, nReflectionResolution);

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

			if (bSplashAndMoviePatches) {
				double fLetterboxMultiplier = (nResX * ((4.0 / 3.0) / fDisplayAspect) * 0.5);
				fLetterboxLeft = nResX * 0.5 - fLetterboxMultiplier;
				fLetterboxRight = nResX * 0.5 + fLetterboxMultiplier;
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4AF621, &SplashScreenLetterboxPatch);

				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x424864, &MovieLetterboxASM);
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
				NyaHookLib::Patch(0x4D9D1A, &nResX43);
				NyaHookLib::Patch(0x4D9D2A, &nResX43);

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

				if (!bNoNeedleFix) {
					// speedo needle d3d hook, cursed and silly :3
					NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x4DB9D0, &SpeedoDrawASM);
					*(uintptr_t *) &SpeedoDrawD3DReset_RetAddress = *(uintptr_t *) 0x40E713;
					NyaHookLib::Patch(0x40E713, &SpeedoDrawD3DReset);
					NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x4DBDE7, &SpeedoGetAngleASM);

					// don't draw the vanilla needle
					NyaHookLib::Patch<uint8_t>(0x42E340, 0xC3);
				}

				// todo 904E10 array of speedo rpm counters, is this scaled correct?
				// todo for menus: check 0x44087C 0x4412F0 and 0x4451D7
			}
		} break;
		default:
			break;
	}
	return TRUE;
}