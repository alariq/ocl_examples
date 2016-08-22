#include "ui.h"
#ifdef PLATFORM_WINDOWS
#include <windows.h>
#endif
#include <SDL2/SDL.h>
#include "AntTweakBar/AntTweakBar.h"

UI* UI::CreateUI()
{
	return new UI();
}

bool UI::Init(int width, int height)
{
	TwInit(TW_OPENGL, NULL);
	// Tell the window size to AntTweakBar
	TwWindowSize(width, height);
	return true;
}

bool UI::Resize(int width, int height)
{
	return TwWindowSize(width, height)!=0;
}

UIBar* UI::CreateBar(const char* pname)
{
	return TwNewBar(pname);
}

void UI::Draw()
{
	TwDraw();
}

// true - if handled
bool UI::Update(SDL_Event* pevent)
{
	return TwEventSDL(pevent, SDL_MAJOR_VERSION, SDL_MINOR_VERSION)!=0;
}

void UI::Terminate()
{
	TwTerminate();
}

bool UI::AddButton(UIBar *pbar, const char *pname, UI_Callback callback, void *clientData, const char *pdef)
{
	return 1 == TwAddButton(pbar, pname, callback, clientData, pdef);
}

const select_fptr<true>::fptr select_fptr<true>::mptr = TwAddVarRO;
const select_fptr<false>::fptr select_fptr<false>::mptr = TwAddVarRW;
