#include "includes.hpp"
#include "GUI/gui.hpp"
#include "Hooks/Hooks.hpp"
#include <gd.h>
#include <filesystem>
#include "Logic/logic.hpp"
#include "nlohmann/json.hpp"
#include "Hack/audiopitchHack.hpp"
#include "Logic/autoclicker.hpp"
#include <chrono>
#include <conio.h>
#include <thread>

using json = nlohmann::json;

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

void writeConfig() {
	auto& logic = Logic::get();
	auto& recorder = logic.recorder;

	json j;

	j["game_fps"] = logic.fps;

	j["video_fps"] = recorder.m_fps;
	j["video_width"] = recorder.m_width;
	j["video_height"] = recorder.m_height;
	j["video_codec"] = recorder.m_codec;
	j["video_extra_args"] = recorder.m_extra_args;
	j["video_audio_args"] = recorder.m_extra_audio_args;
	j["video_vf_args"] = recorder.m_vf_args;
	j["video_bitrate"] = recorder.m_bitrate;
	j["video_end_duration"] = recorder.m_after_end_duration;
	j["video_color_fix"] = recorder.color_fix;

	j["max_cps"] = logic.max_cps;

	auto& audiospeedhack = AudiopitchHack::getInstance();

	j["audio_speedhack"] = audiospeedhack.isEnabled();

	j["use_json"] = logic.use_json_for_files;

	j["autoclicker"]["press_interval"] = Autoclicker::get().getFramesBetweenPresses();
	j["autoclicker"]["release_interval"] = Autoclicker::get().getFramesBetweenReleases();
	j["autoclicker"]["player_1"] = logic.autoclicker_player_1;
	j["autoclicker"]["player_2"] = logic.autoclicker_player_2;

	j["autoclicker"]["audo_disable"] = logic.autoclicker_auto_disable;

	j["frame_advance_hold_duration"] = logic.frame_advance_hold_duration;
	j["frame_advance_delay"] = logic.frame_advance_delay;

	j["file_dialog"] = logic.file_dialog;

	j["frame_counter_x"] = logic.frame_counter_x;
	j["frame_counter_y"] = logic.frame_counter_y;

	j["cps_counter_x"] = logic.cps_counter_x;
	j["cps_counter_y"] = logic.cps_counter_y;

	std::ofstream file(".echo\\settings\\settings.json");

	file << j.dump(4);

	file.close();
}

// If the json object exists, set it. If it doesn't, set it to a default value
template<typename T>
T getOrDefault(const json& j, const std::string& key, const T& defaultValue) {
	if (j.contains(key) && !j[key].is_null()) {
		return j[key].get<T>();
	}
	return defaultValue;
}


void readConfig() {
	auto& logic = Logic::get();
	auto& recorder = logic.recorder;

	std::ifstream file(".echo\\settings\\settings.json");
	if (!file.is_open()) {
		logic.error = "Error: Failed to open config file";
		return;
	}
	json j;
	file >> j;

	logic.fps = getOrDefault(j, "game_fps", 60);
	GUI::get().input_fps = logic.fps;
	recorder.m_fps = getOrDefault(j, "video_fps", 60);
	recorder.m_width = getOrDefault(j, "video_width", 1920);
	recorder.m_height = getOrDefault(j, "video_height", 1080);
	recorder.m_codec = getOrDefault(j, "video_codec", std::string("h264_nvenc"));
	recorder.m_extra_args = getOrDefault(j, "video_extra_args", std::string(""));
	recorder.m_extra_audio_args = getOrDefault(j, "video_audio_args", std::string(""));
	recorder.m_vf_args = getOrDefault(j, "video_vf_args", std::string(""));
	recorder.m_bitrate = getOrDefault(j, "video_bitrate", 12);
	recorder.m_after_end_duration = getOrDefault(j, "video_end_duration", 3);
	recorder.color_fix = getOrDefault(j, "video_color_fix", true);

	logic.max_cps = getOrDefault(j, "max_cps", 15);

	auto& audiospeedhack = AudiopitchHack::getInstance();

	audiospeedhack.setEnabled(getOrDefault(j, "audio_speedhack", true));

	logic.use_json_for_files = getOrDefault(j, "use_json", false);

	Autoclicker::get().setFramesBetweenPresses(getOrDefault(j["autoclicker"], "press_interval", 50));
	Autoclicker::get().setFramesBetweenReleases(getOrDefault(j["autoclicker"], "release_interval", 50));

	logic.autoclicker_player_1 = getOrDefault(j["autoclicker"], "player_1", true);
	logic.autoclicker_player_2 = getOrDefault(j["autoclicker"], "player_2", true);

	logic.autoclicker_auto_disable = getOrDefault(j["autoclicker"], "auto_disable", false);

	logic.frame_advance_hold_duration = getOrDefault(j, "frame_advance_hold_duration", 300);
	logic.frame_advance_delay = getOrDefault(j, "frame_advance_delay", 50);

	logic.file_dialog = getOrDefault(j, "file_dialog", false);

	logic.frame_counter_x = getOrDefault(j, "frame_counter_x", 50);
	logic.frame_counter_y = getOrDefault(j, "frame_counter_y", 50);

	logic.cps_counter_x = getOrDefault(j, "cps_counter_x", 30);
	logic.cps_counter_y = getOrDefault(j, "cps_counter_y", 20);

	CCDirector::sharedDirector()->setAnimationInterval(1.f / GUI::get().input_fps);

	file.close();
}

void UnfullscreenGame()
{
	HWND gameWindow = FindWindowA("Geometry Dash", NULL);
	if (gameWindow == NULL) return;

	// Get the current window style.
	LONG style = GetWindowLong(gameWindow, GWL_STYLE);

	// Check if the game is in fullscreen mode.
	if (style & WS_POPUP) // Fullscreen mode is typically WS_POPUP.
	{
		// Remove the WS_POPUP style and add the WS_OVERLAPPEDWINDOW style.
		style &= ~WS_POPUP;
		style |= WS_OVERLAPPEDWINDOW;

		// Change the window style.
		SetWindowLong(gameWindow, GWL_STYLE, style);
	}
}

DWORD WINAPI my_thread(void* hModule) {
	// UnfullscreenGame();
	MH_Initialize();
	auto& instance = GUI::get();
	AudiopitchHack::getInstance().initialize();

	readConfig();

	srand(GetTickCount());

	ImGuiHook::setRenderFunction(renderFuncWrapper);
	//ImGuiHook::setToggleCallback(callback);
	ImGuiHook::setInitFunction(initFuncWrapper);
	ImGuiHook::setupHooks([](void* target, void* hook, void** trampoline) {
		MH_CreateHook(target, hook, trampoline);
	});
	//ImGuiHook::setKeybind(VK_SHIFT);
	Hooks::init_hooks();
	MH_EnableHook(MH_ALL_HOOKS);

	std::filesystem::path echoDirectory = ".echo";
	std::filesystem::path videoDirectory = ".echo/renders";
	std::filesystem::path configDirectory = ".echo/settings";
	std::filesystem::path osuDirectory = ".echo/osu";

	std::filesystem::create_directory(echoDirectory);
	std::filesystem::create_directory(videoDirectory);
	std::filesystem::create_directory(configDirectory);
	std::filesystem::create_directory(osuDirectory);

	return 0;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(module);
		CreateThread(0, 0, my_thread, module, 0, 0);
	}
	if (reason == DLL_PROCESS_DETACH) {
		writeConfig();
	}
	return TRUE;
}