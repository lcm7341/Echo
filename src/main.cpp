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
#include "Logic/speedhack.h"
#include <exception>
#include <cstdlib>
#include <stdexcept>
#include "Logic/convertible.h";
#include "Logic/Conversions/tasbot.h"

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

namespace fs = std::filesystem;

void renameFilesInEchoDirectory()
{
	std::string directoryPath = ".echo";

	for (const auto& entry : fs::directory_iterator(directoryPath))
	{
		if (entry.is_regular_file())
		{
			std::string filePath = entry.path().string();

			// Check if file ends with ".echo"
			if (filePath.size() >= 5 && filePath.substr(filePath.size() - 5) == ".echo")
			{
				std::ifstream fileStream(filePath);
				char firstCharacter = fileStream.get();
				fileStream.close();

				if (firstCharacter == '{')
				{
					std::string newFilePath = filePath + ".json";
					fs::rename(filePath, newFilePath);
					std::cout << "Renamed file: " << filePath << " to " << newFilePath << std::endl;
				}
			}
			// Check if file ends with ".bin"
			else if (filePath.size() >= 4 && filePath.substr(filePath.size() - 4) == ".bin")
			{
				std::string newFilePath = filePath.substr(0, filePath.size() - 4);
				newFilePath += ".echo";
				fs::rename(filePath, newFilePath);
				std::cout << "Renamed file: " << filePath << " to " << newFilePath << std::endl;
			}
		}
	}
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

	j["autoclicker"]["press_interval"] = Autoclicker::get().getFramesBetweenPresses();
	j["autoclicker"]["release_interval"] = Autoclicker::get().getFramesBetweenReleases();
	j["autoclicker"]["player_1"] = logic.autoclicker_player_1;
	j["autoclicker"]["player_2"] = logic.autoclicker_player_2;

	j["autoclicker"]["auto_disable"] = logic.autoclicker_auto_disable;

	j["frame_advance_hold_duration"] = logic.frame_advance_hold_duration;
	j["frame_advance_delay"] = logic.frame_advance_delay;

	j["file_dialog"] = logic.file_dialog;

	j["show_recording"] = logic.show_recording;
	j["recording_label_x"] = logic.recording_label_x;
	j["recording_label_y"] = logic.recording_label_y;
	j["recording_label_scale"] = logic.recording_label_scale;
	j["recording_label_opacity"] = logic.recording_label_opacity;

	j["show_frame"] = logic.show_frame;
	j["frame_counter_x"] = logic.frame_counter_x;
	j["frame_counter_y"] = logic.frame_counter_y;
	j["frame_counter_scale"] = logic.frame_counter_scale;
	j["frame_counter_opacity"] = logic.frame_counter_opacity;

	j["show_cps"] = logic.show_cps;
	j["cps_counter_x"] = logic.cps_counter_x;
	j["cps_counter_y"] = logic.cps_counter_y;
	j["cps_counter_scale"] = logic.cps_counter_scale;
	j["cps_counter_opacity"] = logic.cps_counter_opacity;

	j["show_percent"] = logic.show_percent;
	j["percent_counter_x"] = logic.percent_counter_x;
	j["percent_counter_y"] = logic.percent_counter_y;
	j["percent_accuracy"] = logic.percent_accuracy;
	j["percent_scale"] = logic.percent_scale;
	j["percent_opacity"] = logic.percent_opacity;

	j["show_time"] = logic.show_time;
	j["time_counter_x"] = logic.time_counter_x;
	j["time_counter_y"] = logic.time_counter_y;
	j["time_scale"] = logic.time_scale;
	j["time_opacity"] = logic.time_opacity;

	j["ssb_fix"] = recorder.ssb_fix;
	j["color_fix"] = recorder.color_fix;
	j["real_time_render"] = recorder.real_time_rendering;
	j["a_fade_in_time"] = recorder.a_fade_in_time;
	j["a_fade_out_time"] = recorder.a_fade_out_time;
	j["v_fade_in_time"] = recorder.v_fade_in_time;
	j["v_fade_out_time"] = recorder.v_fade_out_time;
	j["docked_menu"] = GUI::get().docked;

	j["main_pos_x"] = GUI::get().main_pos.x;
	j["main_pos_y"] = GUI::get().main_pos.y;

	j["tools_pos_x"] = GUI::get().tools_pos.x;
	j["tools_pos_y"] = GUI::get().tools_pos.y;

	j["editor_pos_x"] = GUI::get().editor_pos.x;
	j["editor_pos_y"] = GUI::get().editor_pos.y;

	j["render_pos_x"] = GUI::get().render_pos.x;
	j["render_pos_y"] = GUI::get().render_pos.y;

	j["sequence_pos_x"] = GUI::get().sequence_pos.x;
	j["sequence_pos_y"] = GUI::get().sequence_pos.y;

	j["style_pos_x"] = GUI::get().style_pos.x;
	j["style_pos_y"] = GUI::get().style_pos.y;

	j["editor_auto_scroll"] = GUI::get().editor_auto_scroll;

	j["clickbot_enabled"] = logic.clickbot_enabled;

	j["volume_multiplier"] = logic.clickbot_volume_mult_saved;
	j["p1_softs"] = logic.player_1_softs;
	j["p2_softs"] = logic.player_2_softs;
	j["p1_softs_time"] = logic.player_1_softs_time;
	j["p2_softs_time"] = logic.player_2_softs_time;
	j["p1_softs_volume"] = logic.player_1_softs_volume;
	j["p2_softs_volume"] = logic.player_2_softs_volume;

	j["p1_hards"] = logic.player_1_hards;
	j["p2_hards"] = logic.player_2_hards;
	j["p1_hards_time"] = logic.player_1_hards_time;
	j["p2_hards_time"] = logic.player_2_hards_time;
	j["p1_hards_volume"] = logic.player_1_hards_volume;
	j["p2_hards_volume"] = logic.player_2_hards_volume;

	j["p1_micros"] = logic.player_1_micros;
	j["p2_micros"] = logic.player_2_micros;
	j["p1_micros_time"] = logic.player_1_micros_time;
	j["p2_micros_time"] = logic.player_2_micros_time;
	j["p1_micros_volume"] = logic.player_1_micros_volume;
	j["p2_micros_volume"] = logic.player_2_micros_volume;
	
	j["p1_clickpack"] = logic.player_1_path;
	j["p2_clickpack"] = logic.player_2_path;
	j["algorithm"] = logic.algorithm;

	for (const auto& binding : logic.keybinds.bindings) {
		const std::string& action = binding.first;
		const Keybind& keybind = binding.second.first;

		nlohmann::json jsonKeybind;
		if (keybind.GetKey().has_value()) {
			jsonKeybind["key"] = keybind.GetKey().value();
			jsonKeybind["ctrl"] = keybind.GetCtrl();
			jsonKeybind["shift"] = keybind.GetShift();
			jsonKeybind["alt"] = keybind.GetAlt();

			j["keybinds"][action] = jsonKeybind;
		}
	}

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
		logic.error = "Failed to open settings file.\nIt either doesn't exist, or Echo can't access it.";
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

	Autoclicker::get().setFramesBetweenPresses(getOrDefault(j["autoclicker"], "press_interval", 50));
	Autoclicker::get().setFramesBetweenReleases(getOrDefault(j["autoclicker"], "release_interval", 50));

	logic.autoclicker_player_1 = getOrDefault(j["autoclicker"], "player_1", true);
	logic.autoclicker_player_2 = getOrDefault(j["autoclicker"], "player_2", true);

	logic.autoclicker_auto_disable = getOrDefault(j["autoclicker"], "auto_disable", false);

	logic.frame_advance_hold_duration = getOrDefault(j, "frame_advance_hold_duration", 300);
	logic.frame_advance_delay = getOrDefault(j, "frame_advance_delay", 50);

	logic.file_dialog = getOrDefault(j, "file_dialog", false);

	logic.show_frame = getOrDefault(j, "show_frame", false);
	logic.frame_counter_x = getOrDefault(j, "frame_counter_x", 50);
	logic.frame_counter_y = getOrDefault(j, "frame_counter_y", 50);
	logic.frame_counter_opacity = getOrDefault(j, "frame_counter_opacity", 40);
	logic.frame_counter_scale = getOrDefault(j, "frame_counter_scale", 0.4);

	logic.show_recording = getOrDefault(j, "show_recording", false);
	logic.recording_label_x = getOrDefault(j, "recording_label_x", 50);
	logic.recording_label_y = getOrDefault(j, "recording_label_y", 50);
	logic.recording_label_opacity = getOrDefault(j, "recording_label_opacity", 40);
	logic.recording_label_scale = getOrDefault(j, "recording_label_scale", 0.4);

	logic.show_cps = getOrDefault(j, "show_cps", false);
	logic.cps_counter_x = getOrDefault(j, "cps_counter_x", 30);
	logic.cps_counter_y = getOrDefault(j, "cps_counter_y", 20);
	logic.cps_counter_opacity = getOrDefault(j, "cps_counter_opacity", 40);
	logic.cps_counter_scale = getOrDefault(j, "cps_counter_scale", 0.4);

	logic.show_percent = getOrDefault(j, "show_percent", false);
	logic.percent_counter_x = getOrDefault(j, "percent_counter_x", 5);
	logic.percent_counter_y = getOrDefault(j, "percent_counter_y", 315);
	logic.percent_accuracy = getOrDefault(j, "percent_accuracy", 1);
	logic.percent_scale = getOrDefault(j, "percent_scale", 0.4);
	logic.percent_opacity = getOrDefault(j, "percent_opacity", 40);

	logic.time_counter_x = getOrDefault(j, "time_counter_x", 5);
	logic.time_counter_y = getOrDefault(j, "time_counter_y", 302.5);
	logic.time_scale = getOrDefault(j, "time_scale", 0.4);
	logic.show_time = getOrDefault(j, "show_time", false);
	logic.time_opacity = getOrDefault(j, "time_opacity", 40);

	recorder.ssb_fix = getOrDefault(j, "ssb_fix", true);
	recorder.color_fix = getOrDefault(j, "color_fix", true);
	recorder.real_time_rendering = getOrDefault(j, "real_time_render", false);
	recorder.a_fade_in_time = getOrDefault(j, "a_fade_in_time", 2.f);
	recorder.a_fade_out_time = getOrDefault(j, "a_fade_out_time", 2.f);
	recorder.v_fade_in_time = getOrDefault(j, "v_fade_in_time", 2.f);
	recorder.v_fade_out_time = getOrDefault(j, "v_fade_out_time", 2.f);
	GUI::get().docked = getOrDefault(j, "docked_menu", true);

	GUI::get().main_pos.x = getOrDefault(j, "main_pos_x", 10);
	GUI::get().main_pos.y = getOrDefault(j, "main_pos_y", 35);

	GUI::get().tools_pos.x = getOrDefault(j, "tools_pos_x", 950);
	GUI::get().tools_pos.y = getOrDefault(j, "tools_pos_y", 475);

	GUI::get().editor_pos.x = getOrDefault(j, "editor_pos_x", 940);
	GUI::get().editor_pos.y = getOrDefault(j, "editor_pos_y", 35);

	GUI::get().render_pos.x = getOrDefault(j, "render_pos_x", 490);
	GUI::get().render_pos.y = getOrDefault(j, "render_pos_y", 400);

	GUI::get().sequence_pos.x = getOrDefault(j, "sequence_pos_x", 515);
	GUI::get().sequence_pos.y = getOrDefault(j, "sequence_pos_y", 35);

	GUI::get().style_pos.x = getOrDefault(j, "style_pos_x", 1425);
	GUI::get().style_pos.y = getOrDefault(j, "style_pos_y", 35);

	GUI::get().editor_auto_scroll = getOrDefault(j, "editor_auto_scroll", true);

	logic.clickbot_enabled = getOrDefault(j, "clickbot_enabled", false);
	
	logic.clickbot_volume_mult_saved = getOrDefault(j, "volume_multiplier", logic.clickbot_volume_mult_saved);

	logic.player_1_softs = getOrDefault(j, "p1_softs", logic.player_1_softs);
	logic.player_2_softs = getOrDefault(j, "p2_softs", logic.player_2_softs);
	logic.player_1_softs_time = getOrDefault(j, "p1_softs_time", logic.player_1_softs_time);
	logic.player_2_softs_time = getOrDefault(j, "p2_softs_time", logic.player_2_softs_time);
	logic.player_1_softs_volume = getOrDefault(j, "p1_softs_volume", logic.player_1_softs_volume);
	logic.player_2_softs_volume = getOrDefault(j, "p2_softs_volume", logic.player_2_softs_volume);

	logic.player_1_hards = getOrDefault(j, "p1_hards", logic.player_1_hards);
	logic.player_2_hards = getOrDefault(j, "p2_hards", logic.player_2_hards);
	logic.player_1_hards_time = getOrDefault(j, "p1_hards_time", logic.player_1_hards_time);
	logic.player_2_hards_time = getOrDefault(j, "p2_hards_time", logic.player_2_hards_time);
	logic.player_1_hards_volume = getOrDefault(j, "p1_hards_volume", logic.player_1_hards_volume);
	logic.player_2_hards_volume = getOrDefault(j, "p2_hards_volume", logic.player_2_hards_volume);

	logic.player_1_micros = getOrDefault(j, "p1_micros", logic.player_1_micros);
	logic.player_2_micros = getOrDefault(j, "p2_micros", logic.player_2_micros);
	logic.player_1_micros_time = getOrDefault(j, "p1_micros_time", logic.player_1_micros_time);
	logic.player_2_micros_time = getOrDefault(j, "p2_micros_time", logic.player_2_micros_time);
	logic.player_1_micros_volume = getOrDefault(j, "p1_micros_volume", logic.player_1_micros_volume);
	logic.player_2_micros_volume = getOrDefault(j, "p2_micros_volume", logic.player_2_micros_volume);

	logic.player_1_path = getOrDefault(j, "p1_clickpack", logic.player_1_path);
	logic.player_2_path = getOrDefault(j, "p2_clickpack", logic.player_2_path);
	logic.algorithm = getOrDefault(j, "algorithm", logic.algorithm);

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

void terminationHandler()
{
	try
	{
		std::exception_ptr exPtr = std::current_exception();
		if (exPtr)
		{
			// Rethrow the exception to obtain the exception object
			std::rethrow_exception(exPtr);
		}
	}
	catch (const std::exception& ex)
	{
		// Log the error message to a log file
		std::ofstream logfile("error.log");
		if (logfile.is_open())
		{
			logfile << "Error: " << ex.what() << std::endl;
			logfile.close();
		}

		// Print the error message to the console
		std::cerr << "Error: " << ex.what() << std::endl;

		// You can take additional actions here, such as displaying an error message to the user or gracefully exiting the program
	}
}

bool isDirectoryEmpty(const std::string& path) {
	if (!fs::is_directory(path)) {
		std::cerr << "Error: Not a directory." << std::endl;
		return false;
	}

	fs::directory_iterator dirIter(path);
	return fs::begin(dirIter) == fs::end(dirIter);
}

DWORD WINAPI my_thread(void* hModule) {

	MH_Initialize();
	readConfig();
	Speedhack::Setup();
	// UnfullscreenGame();
	Hooks::init_hooks();
	auto& instance = GUI::get();
	AudiopitchHack::getInstance().initialize();

	ImGuiHook::setRenderFunction(renderFuncWrapper);
	//ImGuiHook::setToggleCallback(callback);
	ImGuiHook::setInitFunction(initFuncWrapper);
	ImGuiHook::setupHooks([](void* target, void* hook, void** trampoline) {
		MH_CreateHook(target, hook, trampoline);
		});
	MH_STATUS status = MH_EnableHook(MH_ALL_HOOKS);
	if (status != MH_OK) {
		Logic::get().error = "Could not load MinHook! Restart your game!";
	}

	////ImGuiHook::setKeybind(VK_SHIFT);

	std::vector<std::string> paths = {
		".echo",
		".echo/renders",
		".echo/settings",
		".echo/themes",
		".echo/osu",
		".echo/clickpacks",
	};

	std::vector<std::string> clickbot_paths = {
		".echo/clickpacks/example",
		".echo/clickpacks/example/clicks",
		".echo/clickpacks/example/releases",
		".echo/clickpacks/example/soft_clicks",
		".echo/clickpacks/example/soft_releases",
		".echo/clickpacks/example/hard_clicks",
		".echo/clickpacks/example/hard_releases",
		".echo/clickpacks/example/micro_clicks",
		".echo/clickpacks/example/micro_releases",
	};

	/*std::string directoryPath = ".tasbot/macro";

	std::map<std::string, std::shared_ptr<Convertible>> options = {
	{"TASBot", std::make_shared<TASBot>()}
	};

	for (const auto& entry : fs::directory_iterator(directoryPath)) {
		if (entry.is_regular_file()) {
			options["TASBot"]->import(entry.path().string());
			Logic::get().sort_inputs();
			Logic::get().write_file(entry.path().stem().string());
			Logic::get().inputs.clear();
		}
	}*/

	try {
		for (auto& path : paths) {
			std::filesystem::create_directory(path);
		}

		if (isDirectoryEmpty(".echo/clickpacks")) {
			for (auto& path : clickbot_paths) {
				std::filesystem::create_directory(path);
			}
		}

		// Specify the directory path
		fs::path directoryPath = ".echo\\clickpacks";

		// Get all directories within the specified directory
		std::vector<fs::path> folders;
		for (const auto& entry : fs::directory_iterator(directoryPath))
		{
			if (fs::is_directory(entry.path()))
			{
				folders.push_back(entry.path());
			}
		}

		// Convert the folder paths to folder names for ImGui
		std::vector<std::string> folderNames;
		folderNames.reserve(folders.size());
		for (const auto& folder : folders)
		{
			folderNames.push_back(folder.filename().string());
		}

		if (Logic::get().player_2_path.empty() && !folderNames.empty()) Logic::get().player_2_path = folderNames[0];
		if (Logic::get().player_1_path.empty() && !folderNames.empty()) Logic::get().player_1_path = folderNames[0];
	}
	catch (const std::filesystem::filesystem_error& ex) {
		printf("Filesystem error: %s\n", ex.what());

		std::ofstream logfile("ECHO_ERROR.txt");
		if (logfile.is_open())
		{
			logfile << "Filesystem error: " << ex.what() << std::endl;
			logfile.close();
		}
	}

	renameFilesInEchoDirectory();

	return 0;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(module);
		CreateThread(0, 0, my_thread, module, 0, 0);
	}
	if (reason == DLL_PROCESS_DETACH) {
		writeConfig();
		GUI::get().export_theme(".echo\\settings\\theme.ui", true);
		Speedhack::Detach();
	}
	return TRUE;
}