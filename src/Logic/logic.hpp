#include "../includes.hpp"

enum State {
	IDLE,
	RECORDING,
	PLAYING,
	BOTH
};

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <gd.h>
#include <cocos2d.h>

using namespace cocos2d;

struct Input {
	unsigned frame;
	bool down;
	bool player1;
};

struct CheckpointData {
	double y_accel;
	float rotation;
};

struct Checkpoint {
	CheckpointData player_1;
	CheckpointData player_2;
};

class Logic {
	State state = IDLE;

	unsigned replay_pos = 0;

	std::vector<Input> inputs;

	std::vector<double> offsets;

public:
	static auto& get() {
		static Logic logic;
		return logic;
	}

	unsigned get_frame();
	double get_time();

	inline bool is_playing() { 
		return state == PLAYING; 
	}

	inline bool is_recording() { 
		return state == RECORDING; 
	}

	inline bool is_both() {
		return state == BOTH;
	}

	inline bool is_idle() {
		return state == IDLE;
	}

	void toggle_playing() {
		state = is_playing() ? IDLE : PLAYING;
	}

	void toggle_recording() {
		state = is_recording() ? IDLE : RECORDING;
	}

	void idle() {
		state = IDLE;
	}

	void record_input(bool down, bool player1);
	
	void play_input(Input& input);
	
	void play_macro();

	int find_closest_input();

	void set_replay_pos(unsigned idx);

	int get_replay_pos() {
		return replay_pos;
	}

	float fps = 60.f;
	char macro_name[1000] = "output";

	std::string error = "";

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

	std::vector<Checkpoint> checkpoints;

	void save_checkpoint(Checkpoint data) {
		checkpoints.push_back(data);
	}

	void remove_last_checkpoint() {
		checkpoints.pop_back();
	}

	Checkpoint get_latest_checkpoint() {
		if (checkpoints.size() > 0) {
			return checkpoints.back();
		}
		return Checkpoint{ 0, 0 };
	}

	void handle_checkpoint_data();

};
