#include <vector>
#include <string>
#include <iostream>
#include <fstream>

struct Input {
	unsigned frame;
	bool down;
	bool player1;
};

class Macro {
	std::vector<Input> inputs;

	std::vector<double> offsets;
public:
	float fps = 60.f;

	void add_offset(double time) {
		offsets.push_back(time);
	}

	void remove_last_offset() {
		offsets.pop_back();
	}

	double get_latest_offset() {
		if (offsets.size() > 0)
			return offsets.back();
		return 0.f;
	}

	std::vector<Input>& get_inputs() {
		return inputs;
	}

	void add_input(Input input) {
		inputs.push_back(input);
	}

	float get_fps() {
		return fps;
	}

	void remove_inputs(unsigned frame);

	void write_file(const std::string& filename);

	void read_file(const std::string& filename);
};