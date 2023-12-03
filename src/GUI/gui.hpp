#pragma once
#include "../Hack/opcode.hpp"
#include <imgui.h>
#include "keybinds.h"

class GUI {
public:

	static auto& get() {
		static GUI instance;
		return instance;
	}

	bool show_window = false;

	void draw();

	void main();

	void tools();

	void editor();

	void init();

	void renderer();

	void sequential_replay();

	void ui_editor();

	void clickbot();

	void show_keybind_prompt(const std::string& buttonName);

	void import_theme(std::string path);

	void export_theme(std::string path, bool custom_path = false);

	bool show_reset_popup = false;

	float input_fps = 60.f;

	struct Point
	{
		double x;
		double y;
	};

	std::vector<Point> clickedPoints;

	char inputTextBuffer[65535]; // Buffer to hold the user input

	int offset_frames = 0;

	float scheduler_dt = 60.f;

	char theme_name[1000] = "My Theme";

	bool docked = true;

	bool change_display_fps = false; // so it doesnt change when ur typing lol

	bool editor_auto_scroll = true;

	bool reload_inputs = false; // bro fuck off with statics

	std::string keybind_prompt = "";
	std::string keybind_prompt_cache = "";
	ImVec4 popup_bg_color = { 0, 0, 0, 60 };

	ImVec4 player_1_button_color = { 0.18, 0.89, 0.7, 0.54 };
	ImVec4 player_2_button_color = { 0.18, 0.89, 0.46, 0.54 };
	ImVec4 input_selected_color_p1 = { 0.2, 0.9, 0.8, 0.54 };
	ImVec4 input_selected_color_p2 = { 0.2, 0.9, 0.8, 0.54 };

	ImVec2 main_pos;
	ImVec2 tools_pos;
	ImVec2 editor_pos;
	ImVec2 render_pos;
	ImVec2 sequence_pos;
	ImVec2 style_pos;
	ImVec2 clickbot_pos;
};