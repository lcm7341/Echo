

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
#include <chrono>
#include <conio.h>

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

#define PLAYER_FIELDS \
	FIELD(double, m_xAccel) \
	FIELD(double, m_yAccel) \
	FIELD(double, m_jumpAccel) \
	FIELD(bool, m_blackOrb) \
	FIELD(bool, m_isDashing) \
	FIELD(bool, m_isUpsideDown) \
	FIELD(bool, m_isOnGround) \
	FIELD(bool, m_isSliding) \
	FIELD(bool, m_isRising) \
	FIELD(float, m_vehicleSize) \
	FIELD(float, m_playerSpeed) \
	FIELD(bool, unk480) \
	FIELD(bool, unk4B0) \
	FIELD(cocos2d::CCSprite*, unk4B4) \
	FIELD(bool, unk4D4) \
	FIELD(bool, unk4DC) \
	FIELD(bool, unk538) \
	FIELD(bool, unk539) \
	FIELD(bool, unk53A) \
	FIELD(bool, unk53B) \
	FIELD(bool, m_canRobotJump) \
	FIELD(float, m_groundHeight) \
	FIELD(cocos2d::CCSprite*, unk4E8) \
	FIELD(cocos2d::CCSprite*, unk4EC) \
	FIELD(cocos2d::CCSprite*, unk4F0) \
	FIELD(cocos2d::CCSprite*, unk4F4) \
	FIELD(cocos2d::CCSprite*, unk4F8) \
	FIELD(cocos2d::CCSprite*, unk4FC) \
	FIELD(cocos2d::CCSprite*, unk500) \
	FIELD(cocos2d::CCSprite*, unk504) \
	FIELD(cocos2d::CCSprite*, unk508) \
	FIELD(cocos2d::CCSprite*, unk50C) \
	FIELD(bool, unk630) \
	FIELD(bool, unk631) \
	FIELD(float, unk634) \
	FIELD(bool, m_isShip) \
	FIELD(bool, m_isBird) \
	FIELD(bool, m_isBall) \
	FIELD(bool, m_isDart) \
	FIELD(bool, m_isRobot) \
	FIELD(bool, m_isSpider) \

struct CheckpointData {
	#define FIELD(type, name) type name;
	PLAYER_FIELDS
#undef FIELD
	float m_rotation;

	static CheckpointData create(gd::PlayerObject* player) {
		CheckpointData data;
		#define FIELD(type, name) data.name = player->name;
		PLAYER_FIELDS
#undef FIELD
			data.m_rotation = player->getRotation();
		return data;
	}

	void apply(gd::PlayerObject* player) {
		#define FIELD(type, name) player->name = name;
		PLAYER_FIELDS
#undef FIELD
			player->setRotation(m_rotation);
	}
};

struct Checkpoint {
	unsigned number;
	CheckpointData player_1_data;
	CheckpointData player_2_data;
	size_t activated_objects_size;
	size_t activated_objects_p2_size;
	double calculated_xpos;
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

	std::vector<double> offsets;


public:
	static auto& get() {
		static Logic logic;
		return logic;
	}

	enum FORMATS {
		SIMPLE, // saves like 3 shits
		DEBUG, // saves all debug info
	};


	FORMATS format = SIMPLE;

	bool save_debug_info = false;

	Recorder recorder;
	std::vector<Frame> inputs;

	std::vector<Checkpoint> checkpoints;
	std::vector<gd::GameObject*> activated_objects;
	std::vector<gd::GameObject*> activated_objects_p2;
	std::vector<std::pair<float, std::string>> cps_over_percents;

	float fps = 60.f;
	char macro_name[1000] = "output";

	std::string error = "";
	std::string conversion_message = "Waiting... Use the panel above to export/import";

	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::time_point();

	bool real_time_mode = true;
	float speedhack = 1.f;

	double calculated_xpos = 0;
	double previous_xpos = 0;
	float player_speed = 1;
	double player_acceleration = 1;
	std::vector<float> player_x_positions; // because fuck attempt 1, also this isnt my calculated one
	int calculated_frame = 0;

	bool ignore_actions_at_playback = true;
	bool show_frame = false;
	bool show_cps = false;
	float end_portal_position = 0;
	float max_cps = 15;
	float current_cps = 0;
	bool over_max_cps = false;
	bool frame_advance = false;
	bool holding_frame_advance = false;
	bool no_overwrite = false;
	bool use_json_for_files = false;
	bool audio_speedhack = false;
	std::vector<Frame> live_inputs; // for CPS counter, this acts as if when playing back, ur recording

	bool autoclicker = false;
	bool autoclicker_auto_disable = false;
	bool autoclicker_player_1 = false;
	bool autoclicker_player_2 = false;
	int autoclicker_disable_in = 1;

	bool respawn_time_modified = true;

	std::vector<Replay> replays;
	size_t replay_index;
	bool sequence_enabled = false;

	bool noclip_player1 = false;
	bool noclip_player2 = false;

	bool file_dialog = false;

	float frame_counter_x = 50.f;
	float frame_counter_y = 50.f;

	float cps_counter_x = 30.f;
	float cps_counter_y = 20.f;

	unsigned frame_advance_hold_duration = 300; // ms
	unsigned frame_advance_delay = 50; // ms

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

	double xpos_calculation();

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
	std::string highest_cps();

	unsigned cached_inputs_hash = 0;
	int cached_max_cps = 0;
	std::string cached_highest_cps = "0";
	std::string highest_cps_cached() {
		// Calculate a simple sum "hash" of the input frame numbers
		unsigned inputs_hash = 0;
		for (const Frame& frame : inputs)
			inputs_hash += frame.number;

		// Check if the inputs hash has changed
		if (inputs_hash != cached_inputs_hash || cached_max_cps != max_cps) {
			std::string cps = highest_cps();

			cached_highest_cps = cps;
			cached_inputs_hash = inputs_hash;
			cached_max_cps = max_cps;

			return cps;
		}

		return cached_highest_cps;
	}

	float last_xpos = 0.0f;
	unsigned int frame = 0;

	bool click_both_players = false;
	bool swap_player_input = false;

	void remove_inputs(unsigned frame);

	void convert_file(const std::string& filename, bool is_path);

	void write_file(const std::string& filename);

	void read_file(const std::string& filename, bool is_path);

	void write_file_json(const std::string& filename);

	void handle_checkpoint_data();

	void read_file_json(const std::string& filename, bool is_path);

	void save_checkpoint(Checkpoint data) {
		checkpoints.push_back(data);
	}

	void remove_last_checkpoint() {
		checkpoints.pop_back();
	}

	void offset_frames(int offset);

	Checkpoint get_latest_checkpoint() {
		if (checkpoints.size() > 0) {
			return checkpoints.back();
		}
		return Checkpoint{ 0, 0, 0 };
	}
};