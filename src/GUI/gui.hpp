#pragma once
#include "../Hack/opcode.hpp"

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

	bool show_reset_popup = false;

	int keybind = VK_MENU;

	float input_fps = 60.f;

	int offset_frames = 0;
};