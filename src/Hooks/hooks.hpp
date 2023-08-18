#include "../includes.hpp"

using namespace cocos2d;

#define _THISCALL_HOOK(name, ret_type, self_type, ...) \
inline ret_type(__thiscall* name)(self_type* self, __VA_ARGS__); \
ret_type __fastcall name##_h(self_type* self, int, __VA_ARGS__);

#define _FASTCALL_HOOK(name, ret_type, ...) \
inline ret_type(__fastcall* name)(__VA_ARGS__); \
ret_type __fastcall name##_h(__VA_ARGS__);

// gracias matcool :]

namespace Hooks {

	namespace LevelEditorLayer {
		inline void(__thiscall* draw)(gd::LevelEditorLayer* self);
		void __fastcall drawHook(gd::LevelEditorLayer* self, void*);

		inline void(__thiscall* onPlaytest)(gd::LevelEditorLayer* self);
		void __fastcall onPlaytestHook(gd::LevelEditorLayer* self, void*);

		inline void(__thiscall* exit)(CCLayer* self, CCObject* sender);
		void __fastcall exitHook(CCLayer* self, void*, CCObject* sender);
	}

	inline void(__thiscall* CCScheduler_update)(CCScheduler* self, float dt); void __fastcall CCScheduler_update_h(CCScheduler* self, int, float dt);

	inline int(__fastcall* createCheckpoint)(gd::PlayLayer* self); int __fastcall createCheckpoint_h(gd::PlayLayer* self);

	inline int(__fastcall* removeCheckpoint)(gd::PlayLayer* self); int __fastcall removeCheckpoint_h(gd::PlayLayer* self);

	inline bool(__fastcall* menuLayerInit)(gd::MenuLayer* self); bool __fastcall menuLayerInit_h(gd::MenuLayer* self);

	// 0xe5d60
	inline gd::GameObject* (__fastcall* powerOffObject)(gd::GameObject* self); gd::GameObject* __fastcall powerOffObject_h(gd::GameObject* self);

	// 0xeab20
	inline gd::GameObject* (__fastcall* playShineEffect)(gd::GameObject* self); gd::GameObject* __fastcall playShineEffect_h(gd::GameObject* self);

	// 0x1e9a20
	inline void (__fastcall* incrementJumps)(gd::PlayerObject* self); void __fastcall incrementJumps_h(gd::PlayerObject* self);

	// 0x10ed50
	_THISCALL_HOOK(bumpPlayer, void, gd::GJBaseGameLayer, gd::PlayerObject* player, gd::GameObject* object)

	// 0x1f62c0
	inline void (__fastcall* toggleDartMode)(gd::PlayerObject* self, bool toggle); void __fastcall toggleDartMode_h(gd::PlayerObject* self, bool toggle);

	// 0x14ebc0
	inline void (__fastcall* addPoint)(gd::HardStreak* self, cocos2d::CCPoint point); void __fastcall addPoint_h(gd::HardStreak* self, cocos2d::CCPoint point);

	// 0xd1790
	inline void (__fastcall* triggerObject)(gd::GameObject* self, gd::GJBaseGameLayer* layer); void __fastcall triggerObject_h(gd::GameObject* self, gd::GJBaseGameLayer* layer);

	// 0xEF110
	inline gd::GameObject*(__fastcall* hasBeenActivatedByPlayer)(gd::GameObject* self, gd::GameObject* other); gd::GameObject* __fastcall hasBeenActivatedByPlayer_h(gd::GameObject* self, gd::GameObject* other);
	
	inline void(__thiscall* CCKeyboardDispatcher_dispatchKeyboardMSG)(CCKeyboardDispatcher* self, int key, bool down); void __fastcall CCKeyboardDispatcher_dispatchKeyboardMSG_h(CCKeyboardDispatcher* self, int, int key, bool down);

	namespace PlayLayer {

		inline void(__thiscall* lightningFlash)(gd::PlayLayer* self, CCPoint p, _ccColor3B c);
		void __fastcall lightningFlash_h(gd::PlayLayer* self, void* edx, CCPoint p, _ccColor3B c);

		inline void(__thiscall* triggerObject)(gd::EffectGameObject* self, gd::GJBaseGameLayer* idk);
		void __fastcall triggerObject_h(gd::EffectGameObject* self, void*, gd::GJBaseGameLayer* idk);

		_THISCALL_HOOK(pauseGame, void, gd::PlayLayer, bool)

		inline bool(__thiscall* init)(gd::PlayLayer*, gd::GJGameLevel* level); bool __fastcall init_h(gd::PlayLayer* self, void* edx, gd::GJGameLevel* level);

		inline void(__thiscall* updateVisibility)(gd::PlayLayer* self); void __fastcall updateVisibility_h(gd::PlayLayer* self);
		
		inline void(__thiscall* update)(gd::PlayLayer* self, float dt); void __fastcall update_h(gd::PlayLayer* self, int, float dt);

		inline int(__thiscall* pushButton)(gd::PlayLayer* self, int, bool); int __fastcall pushButton_h(gd::PlayLayer* self, int, int, bool);

		// 0x20d810
		inline void(__thiscall* onQuit)(gd::PlayLayer* self); void __fastcall onQuit_h(gd::PlayLayer* self);

		// 0x207d30
		inline void (__thiscall* flipGravity)(gd::PlayLayer* self, gd::PlayerObject* player, bool idk, bool idk2); void __fastcall flipGravity_h(gd::PlayLayer* self, gd::PlayerObject* player, bool idk, bool idk2);

		// 0x207e00
		inline void(__thiscall* playGravityEffect)(gd::PlayLayer* self, bool toggle); void __fastcall playGravityEffect_h(gd::PlayLayer* self, bool toggle);

		inline int(__thiscall* releaseButton)(gd::PlayLayer* self, int, bool); int __fastcall releaseButton_h(gd::PlayLayer* self, int, int, bool);

		inline int(__thiscall* resetLevel)(gd::PlayLayer* self); int __fastcall resetLevel_h(gd::PlayLayer* self, int);

		inline void* (__thiscall* exitLevel)(gd::PlayLayer* self); void* __fastcall exitLevel_h(gd::PlayLayer* self, int);

		// 0x20a1a0
		inline void (__thiscall* destroyPlayer)(gd::PlayLayer* self, gd::PlayerObject*, gd::GameObject*); void __fastcall destroyPlayer_h(gd::PlayLayer* self, gd::PlayerObject*, gd::GameObject*);

		inline bool(__thiscall* death)(void*, void*, void*); bool __fastcall death_h(void* self, void*, void* go, void* thingy);
		}

	inline void(__thiscall* PlayerObject_ringJump)(gd::PlayerObject* self, gd::GameObject* ring);


	void __fastcall PlayerObject_ringJump_h(gd::PlayerObject* self, int, gd::GameObject* ring);

	inline void(__thiscall* GameObject_activateObject)(gd::GameObject* self, gd::PlayerObject* player); void __fastcall GameObject_activateObject_h(gd::GameObject* self, int, gd::PlayerObject* player);

	namespace PauseLayer {
		inline bool(__thiscall* init)(gd::PauseLayer* self); bool __fastcall init_h(gd::PauseLayer* self);
	}

	void init_hooks();
}
