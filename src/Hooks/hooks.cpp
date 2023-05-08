#include "Hooks.hpp"
#include "../Logic/logic.hpp"

#define HOOK(o, f) MH_CreateHook(reinterpret_cast<void*>(gd::base + o), f##_h, reinterpret_cast<void**>(&f));
// gracias matcool :]

#define HOOK_COCOS(o, f) MH_CreateHook(GetProcAddress(GetModuleHandleA("libcocos2d.dll"), o), f##_h, reinterpret_cast<void**>(&f));

void Hooks::init_hooks() {
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x2029C0), PlayLayer::update_h, reinterpret_cast<void**>(&PlayLayer::update));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x111500), PlayLayer::pushButton_h, reinterpret_cast<void**>(&PlayLayer::pushButton));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x111660), PlayLayer::releaseButton_h, reinterpret_cast<void**>(&PlayLayer::releaseButton));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x20BF00), PlayLayer::resetLevel_h, reinterpret_cast<void**>(&PlayLayer::resetLevel));

    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x20DDD0), createCheckpoint_h, reinterpret_cast<void**>(&createCheckpoint));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x20b830), removeCheckpoint_h, reinterpret_cast<void**>(&removeCheckpoint));

    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x20D810), PlayLayer::exitLevel_h, reinterpret_cast<void**>(&PlayLayer::exitLevel));

    MH_CreateHook(GetProcAddress(GetModuleHandleA("libcocos2d.dll"), "?update@CCScheduler@cocos2d@@UAEXM@Z"), CCScheduler_update_h, reinterpret_cast<void**>(&CCScheduler_update));

    MH_EnableHook(MH_ALL_HOOKS);
}

void __fastcall Hooks::CCScheduler_update_h(CCScheduler* self, int, float dt) {
    auto& logic = Logic::get();
    auto& macro = logic.get_macro();

    if (logic.is_recording() || logic.is_playing()) {
        CCDirector::sharedDirector()->setAnimationInterval(1.f / macro.fps);
        dt = 1.f / macro.fps;
    }

    CCScheduler_update(self, dt);
}

void __fastcall Hooks::PlayLayer::update_h(gd::PlayLayer* self, int, float dt) {
    auto& logic = Logic::get();
    auto& macro = logic.get_macro();

    if (logic.is_playing()) logic.play_macro();

    update(self, dt);
}

int __fastcall Hooks::PlayLayer::pushButton_h(gd::PlayLayer* self, int, int idk, bool button) {
    auto& logic = Logic::get();

    if (logic.is_playing()) return 0;
    logic.record_input(true, button);

    return pushButton(self, idk, button);
}

int __fastcall Hooks::PlayLayer::releaseButton_h(gd::PlayLayer* self, int, int idk, bool button) {
    auto& logic = Logic::get();

    if (logic.is_playing()) return 0;
    logic.record_input(false, button);

    return releaseButton(self, idk, button);
}

int __fastcall Hooks::PlayLayer::resetLevel_h(gd::PlayLayer* self, int idk) {
    int ret = resetLevel(self); // was told i needed to do this, reason why beats me

    auto& logic = Logic::get();
    auto& macro = logic.get_macro();

    macro.macro_name = self->m_level->m_sLevelName;

    if (!self->m_isPracticeMode)
        macro.checkpoints.clear();
    
    if (logic.is_recording() || logic.is_playing()) {
        if (macro.checkpoints.size() > 0) {
            Checkpoint data = macro.checkpoints.back();

            self->m_pPlayer1->setRotationX(data.player_1.rotation);
            self->m_pPlayer2->setRotationX(data.player_2.rotation);

            self->m_pPlayer1->m_yAccel = data.player_1.y_accel;
            self->m_pPlayer2->m_yAccel = data.player_2.y_accel;

        }
    }

    if (self->m_checkpoints->count() > 0)
        self->m_time = macro.get_latest_offset();

    if (logic.is_recording())
        macro.remove_inputs(logic.get_frame());

    if (logic.is_playing())
        logic.set_replay_pos(logic.find_closest_input());

    return ret;
}

void* __fastcall Hooks::PlayLayer::exitLevel_h(gd::PlayLayer* self, int) {

    auto& logic = Logic::get();
    auto& macro = logic.get_macro();

    macro.checkpoints.clear();

    return exitLevel(self);
}

int __fastcall Hooks::createCheckpoint_h(gd::PlayLayer* self) {
    auto& logic = Logic::get();
    auto& macro = logic.get_macro();

    macro.add_offset(self->m_time);

    CheckpointData p1 = { 0, 0 };
    CheckpointData p2 = { 0, 0 };

    p1.y_accel = self->m_pPlayer1->m_yAccel;
    p2.y_accel = self->m_pPlayer2->m_yAccel;

    p1.rotation = self->m_pPlayer1->getRotationX();
    p2.rotation = self->m_pPlayer2->getRotationX();

    macro.save_checkpoint({ p1, p2 });

    return createCheckpoint(self);
}

int __fastcall Hooks::removeCheckpoint_h(gd::PlayLayer* self) {
    auto& logic = Logic::get();
    auto& macro = logic.get_macro();

    macro.remove_last_offset();

    macro.remove_last_checkpoint();

    return removeCheckpoint(self);
}