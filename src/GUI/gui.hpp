#pragma once
#include "../Hack/opcode.hpp"
#include <imgui.h>

class GUI {
public:

	static auto& get() {
		static GUI instance;
		return instance;
	}

	bool show_window = true;

	void draw();

	void main();

	void tools();

	void editor();

	void init();

	void renderer();

	void conversion();

	void sequential_replay();

	void ui_editor();

	void import_theme(std::string path);

	void export_theme(std::string path, bool custom_path = false);

	bool show_reset_popup = false;

	int keybind = VK_MENU;

	float input_fps = 60.f;

	int offset_frames = 0;

	float scheduler_dt = 60.f;

	char theme_name[1000] = "My Theme";

	ImVec4 player_1_button_color = { 0.18, 0.89, 0.7, 0.54 };
	ImVec4 player_2_button_color = { 0.18, 0.89, 0.46, 0.54 };
};