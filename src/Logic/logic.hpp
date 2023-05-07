#include "../includes.hpp"
#include "macros.hpp"

enum State {
	IDLE,
	RECORDING,
	PLAYING,
	BOTH
};

class Logic {
	State state = IDLE;

	unsigned replay_pos = 0;

	Macro macro;

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

	inline Macro& get_macro() {
		return macro;
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

};