#pragma once

class GUI {
public:

	static auto& get() {
		static GUI instance;
		return instance;
	}

	bool show_window = false;

	void draw();

	void main();

	void editor();

	void init();

	void renderer();

	bool show_reset_popup = false;

	float input_fps = 60.f;
};