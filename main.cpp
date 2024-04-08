#include <windows.h>
#include <cmath>
#include "toml++/toml.hpp"

#include "nya_commonhooklib.h"
#include "nya_dx8_hookbase.h"

double fDisplayAspect = 0;
double fSelectedDisplayAspect = 0;
bool bBorderlessWindowed = false;

enum eDebugMode {
	DEBUG_MODE_NONE,
	DEBUG_MODE_ASPECT,
	DEBUG_MODE_UI_SCALE, // unscale specific ui elements
	DEBUG_MODE_RES_CALLS, // calls to the 480p->screen conversion func
	NUM_DEBUG_MODES
};
int nDebugDisplayMode = DEBUG_MODE_NONE;
bool bDebugDisplay = false;
int nCurrentUISpecialCaseDebug = -1;
bool bUISpecialCaseDontScale = false;

struct tMenuObjectDebug {
	int x;
	int y;
	int finalX;
	int finalY;
	bool specialCase;
};
std::vector<tMenuObjectDebug> aMenuObjectSizesThisFrame;

double __stdcall ScreenResolutionPatch(float a1) {
	if (*(int*)0x556A48 == 640) return a1;

	auto ret = (*(int*)0x556A48 * (1.0 / 640.0) * a1);

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

void DrawDebugDisplay() {
	tNyaStringData string;
	string.x = 0.05 * GetAspectRatioInv();
	string.y = 0.05;
	string.size = 0.02;
	DrawString(string, std::format("{}x{}, {} aspect", nResX, nResY, fDisplayAspect).c_str());

	if (IsKeyJustPressed(VK_HOME)) nDebugDisplayMode++;
	if (nDebugDisplayMode >= NUM_DEBUG_MODES) nDebugDisplayMode = DEBUG_MODE_NONE;

	switch (nDebugDisplayMode) {
		case DEBUG_MODE_ASPECT: {
			struct tAspect {
				int a1;
				int a2;
			};
			tAspect aRatios[] = {
					{4, 3},
					{5, 4},
					{16, 10},
					{16, 9},
					{21, 9},
					{32, 9},
			};
			static int nCurrent = -1;

			if (IsKeyJustPressed(VK_PRIOR)) nCurrent--;
			if (IsKeyJustPressed(VK_NEXT)) nCurrent++;

			if (nCurrent < 0 || nCurrent >= sizeof(aRatios) / sizeof(aRatios[0])) nCurrent = -1;

			auto currAspect = aRatios[nCurrent];
			fDisplayAspect = nCurrent >= 0 ? (double)currAspect.a1 / (double)currAspect.a2 : fSelectedDisplayAspect;

			string.y += 0.06;
			DrawString(string, "aspect ratio force:");
			string.y += 0.03;
			DrawString(string, nCurrent >= 0 ? std::format("{}:{} ({})", currAspect.a1, currAspect.a2, fDisplayAspect).c_str() : "unforced");
		} break;
		case DEBUG_MODE_UI_SCALE: {
			if (IsKeyJustPressed(VK_MULTIPLY)) bUISpecialCaseDontScale = !bUISpecialCaseDontScale;

			if (IsKeyJustPressed(VK_PRIOR)) nCurrentUISpecialCaseDebug--;
			if (IsKeyJustPressed(VK_NEXT)) nCurrentUISpecialCaseDebug++;

			if (nCurrentUISpecialCaseDebug <= -1 || nCurrentUISpecialCaseDebug >= aMenuObjectSizesThisFrame.size()) nCurrentUISpecialCaseDebug = -1;

			if (!aMenuObjectSizesThisFrame.empty()) {
				string.y += 0.06;
				DrawString(string, "ui object sizes:");
				string.y += 0.03;
				int i = 0;
				for (auto f: aMenuObjectSizesThisFrame) {
					if (i++ == nCurrentUISpecialCaseDebug || bUISpecialCaseDontScale) {
						string.SetColor(0,255,0,255);
					}
					else {
						string.SetColor(255,255,255,255);
					}
					DrawString(string, std::format("{}x{} ({}x{}) ({})", f.x, f.y, f.finalX, f.finalY, f.specialCase).c_str());
					string.y += 0.03;
				}
			}
		} break;
		case DEBUG_MODE_RES_CALLS: {
			const uintptr_t aResCalls[] = {
					//0x418E50,
					//0x418E8D,
					//0x4190BC,
					//0x419D73,
					//0x419E97,
					//0x41BE21,
					//0x41BE71,
					0x4B46D7,
					0x4B4AB9,
					0x4B4F82,
					0x4B5CE2,
					0x4C3F32,
					0x4CF21B,
					0x4CFF7E,
					//0x4D97A5,
					//0x4D97D7,
					//0x4D9927,
					//0x4DAAA5,
					0x4DCE8C,
					0x4DD215,
					0x4DD26E,
					0x4DD36B,
					0x4DD49F,
			};
			static int nCurrent = -1;

			if (nCurrent >= 0) {
				NyaHookLib::PatchRelative(NyaHookLib::CALL, aResCalls[nCurrent], 0x4127D0);
			}

			if (IsKeyJustPressed(VK_PRIOR)) nCurrent--;
			if (IsKeyJustPressed(VK_NEXT)) nCurrent++;
			if (nCurrent < 0 || nCurrent >= sizeof(aResCalls) / sizeof(aResCalls[0])) nCurrent = -1;

			if (nCurrent >= 0) {
				NyaHookLib::PatchRelative(NyaHookLib::CALL, aResCalls[nCurrent], &ScreenResolutionPatch);
			}

			string.y += 0.06;
			DrawString(string, "res scale calls:");
			string.y += 0.03;
			DrawString(string, nCurrent >= 0 ? std::format("patching 0x{:X}", aResCalls[nCurrent]).c_str() : "off");


		} break;
		default:
			break;
	}
	aMenuObjectSizesThisFrame.clear();
}

void GetDesktopResolution(int& horizontal, int& vertical) {
	RECT desktop;
	GetWindowRect(GetDesktopWindow(), &desktop);
	horizontal = desktop.right;
	vertical = desktop.bottom;
}

int nResX43;

void ForceResolution() {
	*(int*)0x5569F0 = nResX;
	*(int*)0x5569F4 = nResY;
	if (bBorderlessWindowed) *(int*)0x5569F8 = 0;
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

bool bDrawingSpeedo = false;
const float fSpeedoXOffset = -4.5;
const float fSpeedoYOffset = -4;
const float fSpeedoSize = 0.32;
float fSpeedoRotation = 0;
void DrawSpeedoNeedle() {
	static auto pTexture = LoadTexture("speedo.png");
	if (!pTexture) return;

	float fPosX = *(float*)0x904EF0 / (double)nResX;
	float fPosY = *(float*)0x904EF4 / (double)nResY;
	float fScaleY = fSpeedoSize;
	float fScaleX = fScaleY * (1.0 / fDisplayAspect);
	fPosX += (fSpeedoXOffset * (1.0 / fDisplayAspect)) / 640.0;
	fPosY += fSpeedoYOffset / 640.0;
	DrawRectangle(fPosX,fPosX + fScaleX,fPosY,fPosY + fScaleY,{255,255, 255, 255}, 0, pTexture, fSpeedoRotation);
}

void HookLoop() {
	if (bDrawingSpeedo) {
		DrawSpeedoNeedle();
		bDontRefreshInputsThisLoop = true;
	}
	else DrawDebugDisplay();

	CommonMain();
}

bool bDeviceJustReset = false;
void D3DHookMain() {
	if (!g_pd3dDevice) {
		g_pd3dDevice = *(IDirect3DDevice8 **) 0x556A64;
		ghWnd = *(HWND*)0x540044;
		InitHookBase();
	}

	if (bDeviceJustReset) {
		ImGui_ImplDX8_CreateDeviceObjects();
		bDeviceJustReset = false;
	}
	HookBaseLoop();
}

void SpeedoCustomDraw() {
	bDrawingSpeedo = true;
	D3DHookMain();
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

void __stdcall SpriteScaleFunction(const int* pSprite, int* outX, int* outY) {
	int spriteX = pSprite[5];
	int spriteY = pSprite[6];

	bool specialCase = false;

	if (spriteX == 640) specialCase = true; // hack for fullscreen sprites to stay fullscreen
	//if (spriteX == 207 && spriteY == 27) specialCase = true; // vehicle selection bars
	if (spriteX == 278 && spriteY == 36) specialCase = true; // vehicle selection name bg
	if (spriteX == 224 && spriteY == 32) specialCase = true; // controller mappings, todo this is still stretched but removing this misaligns it
	if (spriteX == 544 && spriteY == 32) specialCase = true; // controller mappings, todo this is still stretched but removing this misaligns it
	if (spriteX == 406 && spriteY == 480) specialCase = true; // some menu backgrounds
	if (spriteX == 272 && spriteY == 50) specialCase = true; // car/challenge select icon bg

	// debug controls to manually tweak the ui
	if (bDebugDisplay && nDebugDisplayMode == DEBUG_MODE_UI_SCALE && (aMenuObjectSizesThisFrame.size() == nCurrentUISpecialCaseDebug || bUISpecialCaseDontScale)) specialCase = true;

	if (specialCase) {
		*outX = nResX * (1.0 / 640.0) * spriteX;
	}
	else {
		*outX = ScreenResolutionPatch(spriteX);
	}
	*outY = nResY * (1.0 / 480.0) * spriteY;

	if (bDebugDisplay) aMenuObjectSizesThisFrame.push_back({spriteX, spriteY, *outX, *outY, specialCase});
}

void __attribute__((naked)) SpriteScaleASM() {
	__asm__ (
			"pushad\n\t"
			"push ebx\n\t"
			"push edx\n\t"
			"push eax\n\t"
			"call %0\n\t"
			"popad\n\t"
			"ret\n\t"
			:
			:  "i" (SpriteScaleFunction)
			);
}

auto DebugDisplay_RetAddress = (int(__stdcall*)(int))0x40FC70;
void __stdcall DebugDisplayPatch(int a1) {
	bDrawingSpeedo = false;
	D3DHookMain();
	DebugDisplay_RetAddress(a1);
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			auto config = toml::parse_file("TIRTweaks_gcp.toml");
			bool bNoCD = config["main"]["no_cd"].value_or(true);
			bool bNoVideos = config["main"]["no_videos"].value_or(false);
			bBorderlessWindowed = config["main"]["borderless_windowed"].value_or(false);
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
			bDebugDisplay = config["extras"]["show_debug_info"].value_or(false);

			bool bPlaceD3DResetHooks = false;

			// default to desktop resolution
			if (nResX <= 0 || nResY <= 0) {
				GetDesktopResolution(nResX, nResY);
			}

			nResX43 = nResY * (4.0 / 3.0);
			fResY = nResY;

			if (fDisplayAspect <= 0) {
				fDisplayAspect = (double)nResX / (double)nResY;
			}

			fSelectedDisplayAspect = fDisplayAspect;

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

			if (bDebugDisplay) {
				*(uintptr_t*)&DebugDisplay_RetAddress = NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x40E56E, &DebugDisplayPatch);
				bPlaceD3DResetHooks = true;
			}

			if (bHUDPatches) {
				// sprites
				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x41BE00, &SpriteScaleASM);
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x41BE71, &ScreenResolutionPatch);

				// text
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x418E50, &ScreenResolutionPatch); // small letter spacing fix
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x418E8D, &ScreenResolutionPatch);
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4190BC, &ScreenResolutionPatch);
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x419D73, &ScreenResolutionPatch);
				NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x419E97, &ScreenResolutionPatch); // line breaks

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
					bPlaceD3DResetHooks = true;
					NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x4DBDE7, &SpeedoGetAngleASM);

					// don't draw the vanilla needle
					NyaHookLib::Patch<uint8_t>(0x42E340, 0xC3);
				}

				// todo 904E10 array of speedo rpm counters, is this scaled correct?
				// todo for menus: check 0x44087C 0x4412F0 and 0x4451D7
			}

			if (bPlaceD3DResetHooks) {
				*(uintptr_t *) &SpeedoDrawD3DReset_RetAddress = *(uintptr_t *) 0x40E713;
				NyaHookLib::Patch(0x40E713, &SpeedoDrawD3DReset);
			}
		} break;
		default:
			break;
	}
	return TRUE;
}