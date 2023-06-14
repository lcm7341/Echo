

#include "../Recorder/recorder.hpp"
#include "../includes.hpp"

enum State {
	IDLE,
	RECORDING,
	PLAYING,
	RECORDING_AND_PLAYING
};

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <gd.h>
#include <cocos2d.h>

using namespace cocos2d;

struct Frame {
	unsigned number;
	bool pressingDown;
	bool isPlayer2;

	// Save this even if unused, we may use it later for calculations
	float yPosition;
	float xPosition;
	float rotation;
	double yVelocity;
	double xVelocity;
};

struct CheckpointData {
	double x_accel, y_accel, jump_accel;
	bool is_upside_down, is_on_ground, is_dashing, is_sliding, is_rising;
	float vehicle_size, player_speed, rotation_x, rotation_y, x_pos, y_pos;

	static CheckpointData create(gd::PlayerObject* player) {
		return {
			player->m_xAccel,
			player->m_yAccel,
			player->m_jumpAccel,
			player->m_isUpsideDown,
			player->m_isOnGround,
			player->m_isDashing,
			player->m_isSliding,
			player->m_isRising,
			player->m_vehicleSize,
			player->m_playerSpeed,

			player->getRotationX(),
			player->getRotationY(),
			player->getPositionX(),
			player->getPositionY()
		};
	}

	void apply(gd::PlayerObject* player) {
		player->m_xAccel = x_accel;
		player->m_yAccel = y_accel;
		player->m_jumpAccel = jump_accel;
		player->m_isUpsideDown = is_upside_down;
		player->m_isOnGround = is_on_ground;
		player->m_isDashing = is_dashing;
		player->m_isSliding = is_sliding;
		player->m_isRising = is_rising;
		player->m_vehicleSize = vehicle_size;
		player->m_playerSpeed = player_speed;

		player->setRotationX(rotation_x);
		player->setRotationY(rotation_y);
		player->setPositionX(x_pos);
		player->setPositionY(y_pos);
	}
};

struct Checkpoint {
	unsigned number;
	CheckpointData player_1_data;
	CheckpointData player_2_data;
	size_t activated_objects_size;
	size_t activated_objects_p2_size;
};

struct Replay {
	std::string name;
	int max_frame_offset;
	std::vector<Frame> actions;
};

class Logic {
	State state = IDLE;

	unsigned replay_pos = 0;
	unsigned removed_time = 0;

	std::vector<Frame> inputs;

	std::vector<double> offsets;

public:
	static auto& get() {
		static Logic logic;
		return logic;
	}

	Recorder recorder;

	std::vector<Checkpoint> checkpoints;
	std::vector<gd::GameObject*> activated_objects;
	std::vector<gd::GameObject*> activated_objects_p2;

	float fps = 60.f;
	char macro_name[1000] = "output";

	std::string error = "";

	bool real_time_mode = true;
	float speedhack = 1.f;

	bool ignore_actions_at_playback = true;
	bool show_frame = false;
	bool show_cps = false;
	float max_cps = 15;

	bool frame_advance = false;

	std::vector<Replay> replays;
	size_t replay_index;
	bool sequence_enabled = false;

	unsigned get_frame();
	double get_time();

	inline bool is_playing() { 
		return state == PLAYING; 
	}

	inline bool is_recording() { 
		return state == RECORDING; 
	}

	inline bool is_both() {
		return state == RECORDING_AND_PLAYING;
	}

	inline bool is_idle() {
		return state == IDLE;
	}

	void write_osu_file(const std::string& filename);

	void toggle_playing() {
		state = is_playing() ? IDLE : PLAYING;
	}

	void toggle_recording() {
		state = is_recording() ? IDLE : RECORDING;
	}

	void idle() {
		state = IDLE;
	}

	void sort_inputs();

	void record_input(bool down, bool player1);

	void offset_inputs(int lower, int upper);
	
	void play_input(Frame& input);
	
	bool play_macro(int& offset);

	int find_closest_input();

	void set_replay_pos(unsigned idx);

	int get_replay_pos() {
		return replay_pos;
	}

	int get_removed() {
		return removed_time;
	}

	void add_offset(double time) {
		offsets.push_back(time);
	}

	void set_removed(double time) {
		removed_time = time;
	}

	void remove_last_offset() {
		offsets.pop_back();
	}

	double get_latest_offset() {
		if (offsets.size() > 0)
			return offsets.back();
		return 0.f;
	}

	std::vector<Frame>& get_inputs() {
		return inputs;
	}

	void add_input(Frame input) {
		inputs.push_back(input);
	}

	float get_fps() {
		return fps;
	}

	unsigned count_presses_in_last_second();

	float last_xpos = 0.0f;
	unsigned int frame = 0;

	bool click_both_players = false;
	bool swap_player_input = false;

	void remove_inputs(unsigned frame);

	void convert_file(const std::string& filename, bool is_path);

	void write_file(const std::string& filename);

	void read_file(const std::string& filename, bool is_path);

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
		return Checkpoint{ 0, 0, 0 };
	}

	void handle_checkpoint_data();
};
