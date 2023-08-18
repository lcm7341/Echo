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
#include "../GUI/keybinds.h"
#include "../Clickbot/clickbot.hpp"

#define PLAYLAYER gd::GameManager::sharedState()->getPlayLayer()

using namespace cocos2d;

struct Frame {
	int number;
	bool pressingDown;
	bool isPlayer2;

	// Save this even if unused, we may use it later for calculations
	float yPosition;
	float xPosition;
	float rotation;
	double yVelocity;
	double xVelocity;
};

struct ShakeFrame {
	float m_currentShakeStrength;
	float m_currentShakeInterval;
	double m_lastShakeTime;
	cocos2d::CCPoint unk3D8;
	int number;
};

struct FPSFrame {
	double fps;
	int number;
};

struct HacksStr
{
	bool layoutMode = false;
	bool showHitboxes = false, showDecorations = true;
	float hitboxThickness = 0.4;
	int hitboxOpacity = 0, borderOpacity = 255;
	bool fillHitboxes = false;

	float backgroundColor[3] = { 40.f / 255.f, 125.f / 255.f, 1 }, blocksColor[3] = { 1, 1, 1 };

	bool hitboxTrail = false, trajectory = false;
	float hitboxTrailLength = 50.f;
	int trajectoryAccuracy = 100;

	float solidHitboxColor[4] = { 0, 0, 1, 1 }, slopeHitboxColor[4] = { 0, 0, 1, 1 }, hazardHitboxColor[4] = { 1, 0, 0, 1 }, portalHitboxColor[4] = { 0, 1, 0, 1 }, padHitboxColor[4] = { 0, 1, 0, 1 },
		ringHitboxColor[4] = { 0, 1, 0, 1 }, collectibleHitboxColor[4] = { 0.88f, 1, 0, 1 }, modifierHitboxColor[4] = { 1, 1, 1, 1 }, playerHitboxColor[4] = { 1, 0.247f, 0.247f, 1 },
		rotatedHitboxColor[4] = { 0.498f, 0, 0, 1 }, centerHitboxColor[4] = { 0, 0, 1, 1 };
};

struct ObjectData {
	int tag;
	float posX;
	float posY;
	float rotX;
	float rotY;
	float velX;
	float velY;
	float rotation;

	float speed1; // m_unk2F4
	float speed2; // m_unk2F8
	float speed3; // m_unk33C
	float speed4; // m_unk340
	float speed5; // m_unk390

	bool m_bUnk3;
	bool m_bIsBlueMaybe;
	float m_fUnk2;
	float m_fUnk;
	float m_fUnk3;
	float m_fUnk4;
	bool m_bUnk;
	float m_fAnimSpeed2;
	bool m_bIsEffectObject;
	bool m_bRandomisedAnimStart;
	float m_fAnimSpeed;
	bool m_bBlackChild;
	bool m_bUnkOutlineMaybe;
	float m_fBlackChildOpacity;
	bool field_21C;
	bool m_bEditor;
	bool m_bGroupDisabled;
	bool m_bColourOnTop;
	gd::GJSpriteColor* m_pMainColourMode;
	gd::GJSpriteColor* m_pSecondaryColourMode;
	bool m_bCol1;
	bool m_bCol2;
	cocos2d::CCPoint m_obStartPosOffset;
	float m_fUnkRotationField;
	bool m_bTintTrigger;
	bool m_bIsFlippedX;
	bool m_bIsFlippedY;
	cocos2d::CCPoint m_obBoxOffset;
	bool m_bIsOriented;
	cocos2d::CCPoint m_obBoxOffset2;
	gd::OBB2D* m_pObjectOBB2D;
	bool m_bOriented;
	cocos2d::CCSprite* m_pGlowSprite;
	bool m_bNotEditor;
	cocos2d::CCAction* m_pMyAction;
	bool m_bUnk1;
	bool m_bRunActionWithTag;
	bool m_bObjectPoweredOn;
	cocos2d::CCSize m_obObjectSize;
	bool m_bTrigger;
	bool m_bActive;
	bool m_bAnimationFinished;
	cocos2d::CCParticleSystemQuad* m_pParticleSystem;
	std::string m_sEffectPlistName;
	bool m_bParticleAdded;
	bool m_bHasParticles;
	bool m_bUnkCustomRing;
	cocos2d::CCPoint m_obPortalPosition;
	bool m_bUnkParticleSystem;
	cocos2d::CCRect m_obObjectTextureRect;
	bool m_bTextureRectDirty;
	float m_fRectXCenterMaybe;
	cocos2d::CCRect m_obObjectRect2;
	bool m_bIsObjectRectDirty;
	bool m_bIsOrientedRectDirty;
	bool m_bHasBeenActivated;
	bool m_bHasBeenActivatedP2;
	float m_objectRadius; //0x2ec
	bool m_bIsRotatedSide; //0x2F0 for 90 and 270 degrees rotations
	float m_unk2F4;
	float m_unk2F8;
	int m_nUniqueID; //0x2FC
	gd::GameObjectType m_nObjectType; //0x300
	int m_nSection; //0x304
	bool m_bTouchTriggered; //0x308
	bool m_bSpawnTriggered; //0x309
	cocos2d::CCPoint m_obStartPosition; //0x30C
	std::string m_sTextureName; //0x314
	bool m_unk32C;
	bool m_unk32D;
	float m_unk33C;
	float m_unk340;
	bool m_bIsGlowDisabled; //0x354
	int m_nTargetColorID;	// 0x358 (for color triggers)
	float m_fScale; //0x35C
	int m_nObjectID; //0x360
	bool m_unk368;
	bool m_unk369;
	bool m_unk36A;
	bool m_bIsDontEnter; //0x36B
	bool m_bIsDontFade; //0x36C
	int m_nDefaultZOrder; // 0x370
	bool m_unk38C;
	bool m_unk38D;
	bool m_unk38E;
	float m_unk390;
	gd::GJSpriteColor* m_pBaseColor; //0x3A8
	gd::GJSpriteColor* m_pDetailColor; //0x3AC
	gd::ZLayer m_nDefaultZLayer; // 0x03B4
	gd::ZLayer m_nZLayer; //0x3B8
	int m_nGameZOrder; //0x3BC
	std::string m_unk3C0;
	bool m_bShowGamemodeBorders;
	bool m_unk3D9;
	bool m_bIsSelected; //0x3DA
	int m_nGlobalClickCounter; //0x3DC i have no idea what this is for
	bool m_bUnknownLayerRelated;	// 0x3e8
	float m_fMultiScaleMultiplier;	// 0x3ec
	bool m_bIsGroupParent; //0x3F0
	short* m_pGroups; //0x3F4
	short m_nGroupCount; //0x3F8
	int m_nEditorLayer; //0x40C
	int m_nEditorLayer2; //0x410
	int m_unk414;
	cocos2d::CCPoint m_obFirstPosition; //0x424 first position from when its placed in the editor
	bool m_bHighDetail; //0x448
	gd::ColorActionSprite* m_pColorActionSprite1; //0x44C
	gd::ColorActionSprite* m_pColorActionSprite2; //0x450
	gd::GJEffectManager* m_pEffectManager; //0x454
};

#define PLAYER_FIELDS \
	FIELD(double, m_xAccel) \
	FIELD(double, m_yAccel) \
	FIELD(double, m_unk558) \
	FIELD(double, m_jumpAccel) \
	FIELD(bool, m_isDropping) \
	FIELD(bool, m_isDashing) \
	FIELD(bool, m_isUpsideDown) \
	FIELD(bool, m_isOnGround) \
	FIELD(bool, m_isSliding) \
	FIELD(bool, m_isRising) \
	FIELD(float, m_vehicleSize) \
	FIELD(float, m_playerSpeed) \
	FIELD(bool, m_unk480) \
	FIELD(bool, m_unk4B0) \
	FIELD(bool, m_unk4D4) \
	FIELD(bool, m_unk4DC) \
	FIELD(cocos2d::CCSprite*, m_unk4B4) \
	FIELD(bool, m_unk53D) \
	FIELD(bool, m_unk53E) \
	FIELD(bool, m_unk5B0) \
	FIELD(bool, m_unk5FC) \
	FIELD(bool, m_unk5FD) \
	FIELD(bool, m_unk53F) \
	FIELD(bool, m_unk538) \
	FIELD(bool, m_unk539) \
	FIELD(bool, m_unk53A) \
	FIELD(bool, m_unk53B) \
	FIELD(bool, m_canRobotJump) \
	FIELD(float, m_groundHeight) \
	FIELD(gd::HardStreak*, m_waveTrail) \
	FIELD(bool, m_isOnSlope) \
	FIELD(bool, m_wasOnSlope) \
	FIELD(float, m_unk634) \
	FIELD(float, m_decelerationRate) \
	FIELD(float, m_unk61C) \
	FIELD(bool, m_isShip) \
	FIELD(bool, m_isBird) \
	FIELD(bool, m_isBall) \
	FIELD(bool, m_isDart) \
	FIELD(bool, m_isRobot) \
	FIELD(bool, m_isSpider) \
	FIELD(bool, m_isLocked) \
	FIELD(bool, m_unk) \
	FIELD(float, m_unk69C) \
	FIELD(double, m_lastJumpTime) \
	FIELD(double, m_unknown20) \

struct CheckpointData {
	#define PLFIELD(type, name) type name;
	#define FIELD(type, name) type name;
		PLAYER_FIELDS
	#undef PLFIELD
	#undef FIELD
	float m_rotation;
	gd::Gamemode gamemode;
	bool isHolding;
	bool isHolding2;

	static CheckpointData create(gd::PlayerObject* player) {
		CheckpointData data;
		
		#define FIELD(type, name) data.name = player->name;
		#define PLFIELD(type, name) data.name = PLAYLAYER->name;
			PLAYER_FIELDS
		#undef PLFIELD
		#undef FIELD

		data.m_rotation = player->getRotation();
		data.gamemode = GetGamemode(player);

		data.isHolding = player->m_isHolding;
		data.isHolding2 = player->m_isHolding2;
	
		return data;
	}

	void apply(gd::PlayerObject* player) {
		#define FIELD(type, name) player->name = name;
		#define PLFIELD(type, name) PLAYLAYER->name = name;
			PLAYER_FIELDS
		#undef FIELD
		#undef PLFIELD
		player->setRotation(m_rotation);
	}

	static gd::Gamemode GetGamemode(gd::PlayerObject* p)
	{
		if (p->m_isShip)
		{
			return gd::Gamemode::kGamemodeShip;
		}
		else if (p->m_isBird)
		{
			return gd::Gamemode::kGamemodeUfo;
		}
		else if (p->m_isBall)
		{
			return gd::Gamemode::kGamemodeBall;
		}
		else if (p->m_isDart)
		{
			return gd::Gamemode::kGamemodeWave;
		}
		else if (p->m_isRobot)
		{
			return gd::Gamemode::kGamemodeRobot;
		}
		else if (p->m_isSpider)
		{
			return gd::Gamemode::kGamemodeSpider;
		}
		else
		{
			return gd::Gamemode::kGamemodeCube;
		}
	}
};

struct Checkpoint {
	int number;
	CheckpointData player_1_data;
	CheckpointData player_2_data;
	size_t activated_objects_size;
	size_t activated_objects_p2_size;
	std::map<int, ObjectData> objects;
	double calculated_xpos;
	std::vector<CCAction*> actions;
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
		META, // saves metadata
		META_DBG, // saves metadata and debug
	};

	FORMATS format = META;

	std::vector<ShakeFrame> shakes;
	unsigned shakes_pos = 0;

	std::vector<FPSFrame> framerates;
	unsigned framerates_pos = 0;

	HacksStr hacks;

	std::string algorithm = "1";

	bool currently_pressing = false;

	bool export_to_bot_location = false;

	bool clickbot_enabled = false;
	float player_1_volume = 1.f;
	float player_2_volume = 1.f;
	float clickbot_volume_multiplier = 1.f;
	float clickbot_volume_mult_saved = 1.f;

	std::string player_1_path;
	std::string player_2_path;

	bool player_1_softs = true;
	float player_1_softs_time = 150;
	float player_1_softs_volume = 1.f;

	bool player_1_hards = true;
	float player_1_hards_time = 2.f;
	float player_1_hards_volume = 1.f;

	bool player_1_micros = true;
	int player_1_micros_time = 25;
	float player_1_micros_volume = 1.f;

	bool player_2_softs = true;
	float player_2_softs_time = 150;
	float player_2_softs_volume = 1.f;

	bool player_2_hards = true;
	float player_2_hards_time = 2.f;
	float player_2_hards_volume = 1.f;

	bool player_2_micros = true;
	int player_2_micros_time = 25;
	float player_2_micros_volume = 1.f;

	bool record_player_1 = true;
	bool record_player_2 = true;
	bool play_player_1 = true;
	bool play_player_2 = true;

	bool save_debug_info = false;

	std::chrono::duration<double> total_recording_time = std::chrono::duration<double>::zero();
	int total_attempt_count = 1;

	std::string rename_format = "_#";

	Recorder recorder;
	std::vector<Frame> inputs;

	std::vector<Checkpoint> checkpoints;
	std::vector<gd::GameObject*> activated_objects;
	std::vector<gd::GameObject*> activated_objects_p2;
	std::vector<std::pair<float, std::string>> cps_over_percents;
	std::vector<std::pair<float, std::string>> cps_over_percents_p2;

	float fps = 60.f;
	char macro_name[1000] = "Replay";

	std::string error = "";
	std::string conversion_message = "Waiting... Use the panel above to export/import!";

	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::time_point();

	bool real_time_mode = true;
	float speedhack = 1.f;

	double calculated_xpos = 0;
	double previous_xpos = 0;
	float previous_real_xpos = 0.f; // this is robs variable, for renderer
	float player_speed = 1;
	double player_acceleration = 1;
	std::vector<float> player_x_positions; // because fuck attempt 1, also this isnt my calculated one
	int calculated_frame = 0;
	bool completed_level = false;

	double tfx2_calculated = 0.f;

	Keybinds keybinds;

	void processKeybinds() {
		auto& io = ImGui::GetIO();

		for (auto& pair : keybinds.bindings) {
			Keybind& keybind = pair.second.first;
			std::unique_ptr<KeybindableBase>& keybindable = pair.second.second;

			if (keybind.GetKey().has_value() &&
				ImGui::IsKeyPressed(keybind.GetKey().value(), false) &&
				io.KeyCtrl == keybind.GetCtrl() &&
				io.KeyShift == keybind.GetShift() &&
				io.KeyAlt == keybind.GetAlt()) {
				// Call the Keybindable stored in the map
				keybindable->ran();
			}
		}
	}

	bool ignore_actions_at_playback = true;

	bool show_frame = false;
	bool show_cps = false;
	bool show_percent = true;
	int percent_accuracy = 1;
	bool show_time = false;
	bool show_recording = true;

	double clickbot_start, clickbot_now;
	double cycleTime;

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

	float recording_label_x = 50.f;
	float recording_label_y = 50.f;
	float recording_label_scale = 0.4;
	float recording_label_opacity = 70;

	float frame_counter_x = 50.f;
	float frame_counter_y = 50.f;
	float frame_counter_scale = 0.4;
	float frame_counter_opacity = 70;

	float cps_counter_x = 30.f;
	float cps_counter_y = 20.f;
	float cps_counter_scale = 0.4;
	float cps_counter_opacity = 70;

	float percent_counter_x = 5.f;
	float percent_counter_y = 315.f;
	float percent_scale = 0.4;
	float percent_opacity = 70;

	float time_counter_x = 90.f;
	float time_counter_y = 20.f;
	float time_scale = 0.4;
	float time_opacity = 70;

	unsigned frame_advance_hold_duration = 300; // ms
	unsigned frame_advance_delay = 50; // ms

	bool playback_clicking = false;
	bool playback_releasing = false;

	int get_frame();
	double get_time();

	inline bool is_playing() {
		return state == PLAYING || state == RECORDING_AND_PLAYING;
	}

	inline bool is_recording() {
		return state == RECORDING || state == RECORDING_AND_PLAYING;
	}

	inline bool is_both() {
		return state == RECORDING_AND_PLAYING;
	}

	inline bool is_idle() {
		return state == IDLE;
	}

	void write_osu_file(const std::string& filename);

	void toggle_playing() {
		state = (state == IDLE) ? PLAYING :
			(state == PLAYING) ? IDLE :
			(state == RECORDING) ? RECORDING_AND_PLAYING :
			RECORDING; // if state was RECORDING_AND_PLAYING, switch to RECORDING
	}

	void handlePlayerInputs(gd::PlayerObject* player, bool record_player, bool isDashing, bool isPlayer2) {
		if (!record_player)
			return;

		bool wasPressingDown = false;
		Frame lastInput;

		if (!get_inputs().empty()) {
			auto& inputs = get_inputs();
			auto last_input_itr = std::find_if(inputs.rbegin(), inputs.rend(), [=](Frame input) { return input.isPlayer2 == isPlayer2; });

			if (last_input_itr != inputs.rend()) {
				lastInput = *last_input_itr;
				wasPressingDown = lastInput.pressingDown;

				// If the player is not currently pressing, but was pressing on the last input
				if (!player->m_isHolding && wasPressingDown && !isDashing) {
					if (lastInput.number != get_frame()) {
						add_input({ get_frame(), false, isPlayer2 });
					}
				}
				// If the player is currently pressing, but was not pressing on the last input
				else if (player->m_isHolding && !wasPressingDown && !isDashing) {
					if (lastInput.number != get_frame()) {
						add_input({ get_frame(), true, isPlayer2 });

						// Handle Orb Checking
						orbChecking(player->getPosition());
					}
				}
			}
		}
		else {
			// If no previous input and the player is currently pressing
			if (player->m_isHolding) {
				add_input({ get_frame(), true, isPlayer2 });

				// Handle Orb Checking
				orbChecking(player->getPosition());
			}
		}
	}

	bool isClose(cocos2d::CCPoint p1, cocos2d::CCPoint p2, float threshold) {
		float dist = p1.getDistance(p2);
		return dist <= threshold;
	}

	typedef std::function<void* (void*)> cast_function;

	template <typename To, typename From>
	cast_function make_cast() {
		std::unique_ptr<cast_function> caster(new cast_function([](void* from)->void* {
			return static_cast<void*>(static_cast<To*>(static_cast<From*>(from)));
			}));

		auto result = std::bind(*caster, std::placeholders::_1);
		return result;
	}

	void orbChecking(cocos2d::CCPoint playerPosition) { 
		cocos2d::CCArray* children = gd::GameManager::sharedState()->getPlayLayer()->m_pObjects;
		printf("Number of Children: %ld\n", children->count());
		CCObject* it = NULL;
		cast_function caster = make_cast<gd::GameObject, CCObject>();
		CCARRAY_FOREACH(children, it)
		{
			try {
				auto child = reinterpret_cast<gd::GameObject*>(caster(it));
				if (child && child->m_nObjectType) {
					switch (child->m_nObjectType) {
					case gd::GameObjectType::kGameObjectTypeGravityRing:
					case gd::GameObjectType::kGameObjectTypeYellowJumpRing:
					case gd::GameObjectType::kGameObjectTypePinkJumpRing:
					case gd::GameObjectType::kGameObjectTypeDropRing:
					case gd::GameObjectType::kGameObjectTypeRedJumpRing:
					case gd::GameObjectType::kGameObjectTypeCustomRing:
					case gd::GameObjectType::kGameObjectTypeDashRing:
					case gd::GameObjectType::kGameObjectTypeGravityDashRing:
						printf("Player Position: (%f, %f)\n", playerPosition.x, playerPosition.y);
						printf("Child Position: (%f, %f)\n", child->getPosition().x, child->getPosition().y);

						if (isClose(playerPosition, child->getPosition(), 0.5f)) {
							printf("Triggered object");
							// TRIGGER ORB HERE
						}
						break;
					default:
						break;
					}
				}
			}
			catch (std::exception& e) {
				printf("Exception occurred: %s\n", e.what());
			}
		}
	}

	void toggle_recording() {
		state = (state == IDLE) ? RECORDING :
			(state == RECORDING) ? IDLE :
			(state == PLAYING) ? RECORDING_AND_PLAYING :
			PLAYING; // if state was RECORDING_AND_PLAYING, switch to PLAYING
	}

	void idle() {
		state = IDLE;
	}

	void sort_inputs();

	void record_input(bool down, bool player1);

	void offset_inputs(int lower, int upper);

	void play_input(Frame& input);

	bool play_macro();

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

	unsigned count_presses_in_last_second(bool player2);
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

	bool is_over_orb = false;

	float xPosForTimeValue = 0.f;

	bool click_both_players = false;
	bool swap_player_input = false;

	bool click_inverse_p1 = false;
	bool click_inverse_p2 = false;

	void remove_inputs(unsigned frame, bool player_1);

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