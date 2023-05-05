#pragma once

class GUI {
public:

	static auto& get() {
		static GUI instance;
		return instance;
	}

	bool show_window = true;

	void draw();

	void main();

	void init();
};