#include "Hooks.hpp"
#include "../Logic/logic.hpp"
#include <chrono>

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

    HOOK(0x205460, PlayLayer::updateVisibility)

    MH_CreateHook(GetProcAddress(GetModuleHandleA("libcocos2d.dll"), "?update@CCScheduler@cocos2d@@UAEXM@Z"), CCScheduler_update_h, reinterpret_cast<void**>(&CCScheduler_update));

    MH_EnableHook(MH_ALL_HOOKS);
}

bool g_disable_render = false;
float g_left_over = 0.f;

void __fastcall Hooks::CCScheduler_update_h(CCScheduler* self, int, float dt) {
    auto& logic = Logic::get();

    if (logic.real_time_mode) {
        self->setTimeScale(logic.speedhack);
    }

    if (logic.is_recording() || logic.is_playing()) {
        CCDirector::sharedDirector()->setAnimationInterval(1.f / logic.fps);
        if (logic.real_time_mode && !logic.recorder.m_recording) {

            const float target_dt = 1.f / logic.fps / logic.speedhack;

            if (!logic.real_time_mode) CCScheduler_update(self, dt);

            g_disable_render = true;

            const int times = min(static_cast<int>((dt + g_left_over) / target_dt), 150);

            for (unsigned i = 0; i < times; i++) {
                if (i == times - 1)
                    g_disable_render = false;
                CCScheduler_update(self, target_dt);
            }
            g_left_over += dt - target_dt * times;
        }
        else if (logic.recorder.m_recording) {
            dt = 1.f / logic.fps;
            return CCScheduler_update(self, dt);
        }
        else {
            dt = 1.f / logic.fps;
        }
    }
    else {
        CCScheduler_update(self, dt);
    }
}

void __fastcall Hooks::PlayLayer::updateVisibility_h(gd::PlayLayer* self) {
    if (!g_disable_render)
        updateVisibility(self);
}

void __fastcall Hooks::PlayLayer::update_h(gd::PlayLayer* self, int, float dt) {
    auto& logic = Logic::get();

    if (logic.is_playing()) logic.play_macro();

    if (logic.recorder.m_recording)
        logic.recorder.handle_recording(self, dt);

    logic.recorder.m_song_start_offset = self->m_levelSettings->m_songStartOffset;

    if (self->m_isPaused) {
        ImGui::SetKeyboardFocusHere();
    }

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

    strcpy(logic.macro_name, self->m_level->levelName.c_str());


    if (logic.is_playing()) {
        logic.set_replay_pos(logic.find_closest_input());
        logic.activated_objects.clear();
        logic.activated_objects_p2.clear();
    }

    if (!self->m_isPracticeMode)
        logic.checkpoints.clear();
    
    if (logic.is_recording() || logic.is_playing())
        logic.handle_checkpoint_data();

    if (self->m_checkpoints->count() > 0) {
        self->m_time = logic.get_latest_offset();
        constexpr auto delete_from = [&](auto& vec, size_t index) {
            vec.erase(vec.begin() + index, vec.end());
        };
        delete_from(logic.activated_objects, logic.checkpoints.back().activated_objects_size);
        delete_from(logic.activated_objects_p2, logic.checkpoints.back().activated_objects_p2_size);
        if (logic.is_recording()) {
            for (const auto& object : logic.activated_objects)
                object->m_hasBeenActivated = true;
            for (const auto& object : logic.activated_objects_p2)
                object->m_hasBeenActivatedP2 = true;
        }
    }
    else {
        logic.activated_objects.clear();
        logic.activated_objects_p2.clear();
    }

    logic.recorder.update_song_offset(self);

    if (logic.is_recording()) {
        logic.remove_inputs(logic.get_frame());

        if (logic.get_inputs().empty()) return ret;

        auto& last = logic.get_inputs().back();

        auto& inputs = logic.get_inputs();

        if (!logic.checkpoints.empty()) {
            auto currently_holdingP1 = self->m_player1->m_isHolding;
            auto currently_holdingP2 = self->m_player2->m_isHolding;

            auto& last = logic.get_inputs().back();

            auto& inputs = logic.get_inputs();

            if (last.player1) {
                if ((currently_holdingP1 && logic.get_inputs().empty()) || (!logic.get_inputs().empty() && last.down != currently_holdingP1)) {
                    logic.add_input({ logic.get_frame(), true, true });
                    if (currently_holdingP1) {
                        releaseButton(self, 0, true);
                        pushButton(self, 0, true);
                        self->m_player1->m_hasJustHeld = true;
                    }
                }
                else if (!inputs.empty() && inputs.back().down && currently_holdingP1 && logic.checkpoints.size()) {
                    releaseButton(self, 0, true);
                    pushButton(self, 0, true);
                }
            }
            else {
                if ((currently_holdingP2 && logic.get_inputs().empty()) || (!logic.get_inputs().empty() && last.down != currently_holdingP2)) {
                    logic.add_input({ logic.get_frame(), false, false });
                    if (currently_holdingP2) {
                        releaseButton(self, 0, false);
                        pushButton(self, 0, false);
                        self->m_player1->m_hasJustHeld = true;
                    }
                }
                else if (!inputs.empty() && inputs.back().down && currently_holdingP2 && logic.checkpoints.size()) {
                    releaseButton(self, 0, false);
                    pushButton(self, 0, false);
                }
            }

            // when in doubt, consult ReplayBot!
        }

    }

    return ret;
}

void* __fastcall Hooks::PlayLayer::exitLevel_h(gd::PlayLayer* self, int) {

    auto& logic = Logic::get();

    logic.checkpoints.clear();

    return exitLevel(self);
}

int __fastcall Hooks::createCheckpoint_h(gd::PlayLayer* self) {
    auto& logic = Logic::get();

    logic.add_offset(self->m_time);

    CheckpointData p1{ 0, 0 };
    CheckpointData p2{ 0, 0 };


    p1.y_accel = self->m_player1->m_yAccel;
    p1.rotation = self->m_player1->getRotation();
    p1.is_holding = self->m_player1->m_isHolding;

    if (self->m_isDualMode) {
        p2.y_accel = self->m_player2->m_yAccel;
        p2.rotation = self->m_player2->getRotation();
        p2.is_holding = self->m_player2->m_isHolding;
    }


    logic.save_checkpoint({ p1, p2, logic.activated_objects.size(), logic.activated_objects_p2.size()});

    return createCheckpoint(self);
}

int __fastcall Hooks::removeCheckpoint_h(gd::PlayLayer* self) {
    auto& logic = Logic::get();

    logic.remove_last_offset();

    logic.remove_last_checkpoint();

    return removeCheckpoint(self);
}