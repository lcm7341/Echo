#include "includes.hpp"
#include "GUI/gui.hpp"
#include "Hooks/Hooks.hpp"
#include <gd.h>
#include <filesystem>

void callback() {
	auto& instance = GUI::get();
	
	instance.show_window = !instance.show_window;
}

void renderFuncWrapper() {
	auto& instance = GUI::get();

	instance.draw();
}

void initFuncWrapper() {
	auto& instance = GUI::get();

	instance.init();
}

DWORD WINAPI my_thread(void* hModule) {
	MH_Initialize();
	auto& instance = GUI::get();

	ImGuiHook::setRenderFunction(renderFuncWrapper);
	ImGuiHook::setToggleCallback(callback);
	ImGuiHook::setInitFunction(initFuncWrapper);
	ImGuiHook::setupHooks([](void* target, void* hook, void** trampoline) {
		MH_CreateHook(target, hook, trampoline);
	});
	ImGuiHook::setKeybind(VK_SHIFT);
	Hooks::init_hooks();
	MH_EnableHook(MH_ALL_HOOKS);

	std::filesystem::path echoDirectory = ".echo";

	std::filesystem::create_directory(echoDirectory);

	return 0;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(module);
		CreateThread(0, 0, my_thread, module, 0, 0);
	}
	return TRUE;
}
