#ifndef __UI_H__
#define __UI_H__

#include "AntTweakBar/AntTweakBar.h"

typedef TwBar UIBar;
typedef TwBar UIBar;

#define UI_CALL TW_CALL
typedef void (UI_CALL* UI_Callback)(void*);

union SDL_Event;

class UI {
	public:
		static UI* CreateUI();
		static UIBar* CreateBar(const char* pname);
		template <typename T> static bool CreateVar(UIBar* pbar, const char* pname, TwType type, T* pvalue, const char* def = 0);
		static bool	AddButton(UIBar *pbar, const char *pname, UI_Callback callback, void *clientData, const char *pdef);

		static bool Init(int width, int height);
		static bool Resize(int width, int height);
		static void Draw();
		static bool Update(SDL_Event* pevent);
		static void Terminate();

};

template <typename T> struct is_const_ptr {  enum { value = false}; };

template <typename T> struct is_const_ptr<const T*> { enum { value = true}; };

template <bool b> struct select_fptr;

template <> struct select_fptr<true>
{
	typedef int(TW_CALL *fptr)(TwBar *bar, const char *name, TwType type, const void *var, const char *def);
	static const fptr mptr;
};

template <> struct select_fptr<false>
{
	typedef int (TW_CALL* fptr)(TwBar *bar, const char *name, TwType type, void *var, const char *def);
	static const fptr mptr;
};

template <typename T>
bool UI::CreateVar(UIBar* pbar, const char* pname, TwType type, T* pvalue, const char* def)
{
	typedef typename select_fptr< is_const_ptr<T>::value >::fptr TweakFptr;
	return 1 == select_fptr< is_const_ptr<T>::value >::mptr(pbar, pname, type, pvalue, def);
}

#endif // __UI_H__
