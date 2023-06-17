#include "Hooks.hpp"
#include "../Logic/logic.hpp"
#include <chrono>
#include "../Hack/audiopitchHack.hpp"

#define FRAME_LABEL_ID 82369 + 1 //random value :P
#define CPS_LABEL_ID 82369 + 2 //random value :P
#define CPS_BREAKS_LABEL_ID 82369 + 3 //random value :P

#define HOOK(o, f) MH_CreateHook(reinterpret_cast<void*>(gd::base + o), f##_h, reinterpret_cast<void**>(&f));
// gracias matcool :]

#define HOOK_COCOS(o, f) MH_CreateHook(GetProcAddress(GetModuleHandleA("libcocos2d.dll"), o), f##_h, reinterpret_cast<void**>(&f));

void nopInstruction(void* address) {
    DWORD oldProtect;
    // Change the memory protection to PAGE_EXECUTE_READWRITE
    if (VirtualProtect(address, 1, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        // Write the NOP instruction (0x90) to the address
        *(BYTE*)address = 0x90;
        // Restore the original memory protection
        VirtualProtect(address, 1, oldProtect, &oldProtect);
    }
}

void Hooks::init_hooks() {
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x1fb780), PlayLayer::init_h, reinterpret_cast<void**>(&PlayLayer::init));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x2029C0), PlayLayer::update_h, reinterpret_cast<void**>(&PlayLayer::update));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x111500), PlayLayer::pushButton_h, reinterpret_cast<void**>(&PlayLayer::pushButton));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x111660), PlayLayer::releaseButton_h, reinterpret_cast<void**>(&PlayLayer::releaseButton));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x20BF00), PlayLayer::resetLevel_h, reinterpret_cast<void**>(&PlayLayer::resetLevel));

    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x20DDD0), createCheckpoint_h, reinterpret_cast<void**>(&createCheckpoint));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x20b830), removeCheckpoint_h, reinterpret_cast<void**>(&removeCheckpoint));

    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x1E4570), PauseLayer::init_h, reinterpret_cast<void**>(&PauseLayer::init));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x20D810), PlayLayer::exitLevel_h, reinterpret_cast<void**>(&PlayLayer::exitLevel));

    MH_CreateHook(GetProcAddress(GetModuleHandleA("libcocos2d.dll"), "?dispatchKeyboardMSG@CCKeyboardDispatcher@cocos2d@@QAE_NW4enumKeyCodes@2@_N@Z"), CCKeyboardDispatcher_dispatchKeyboardMSG_h, reinterpret_cast<void**>(&CCKeyboardDispatcher_dispatchKeyboardMSG));;

    HOOK(0x205460, PlayLayer::updateVisibility)

        MH_CreateHook(GetProcAddress(GetModuleHandleA("libcocos2d.dll"), "?update@CCScheduler@cocos2d@@UAEXM@Z"), CCScheduler_update_h, reinterpret_cast<void**>(&CCScheduler_update));

    MH_EnableHook(MH_ALL_HOOKS);

    nopInstruction((void*)(gd::base + 0x1E9A07));
    nopInstruction((void*)(gd::base + 0x1E99F6));
}

bool g_disable_render = false;
float g_left_over = 0.f;

void __fastcall Hooks::CCScheduler_update_h(CCScheduler* self, int, float dt) {
    auto& logic = Logic::get();

    if (logic.frame_advance) return;

    if (logic.real_time_mode) {
        self->setTimeScale(logic.speedhack);
    }

    auto& audiospeedhack = AudiopitchHack::getInstance();
    bool isEnabled = audiospeedhack.isEnabled();
    if (isEnabled) {
        audiospeedhack.setPitch(self->getTimeScale());
    }
    else {
        audiospeedhack.setPitch(1);
    }

    if (logic.is_recording() || logic.is_playing()) {
        CCDirector::sharedDirector()->setAnimationInterval(1.f / logic.fps);
        if (logic.real_time_mode && !logic.recorder.m_recording) {

            const float target_dt = 1.f / logic.fps / logic.speedhack;

            if (!logic.real_time_mode) CCScheduler_update(self, dt);

            g_disable_render = true;

            const unsigned int times = min(static_cast<int>((dt + g_left_over) / target_dt), 150);

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
        if (gd::GameManager::sharedState()->getPlayLayer()) {
            if (gd::GameManager::sharedState()->getPlayLayer()->m_player1->m_position.x != logic.last_xpos) {
                logic.frame++;
            }
        }
        CCScheduler_update(self, dt);
    }
}

void __fastcall Hooks::CCKeyboardDispatcher_dispatchKeyboardMSG_h(CCKeyboardDispatcher* self, int, int key, bool down) {
    auto& logic = Logic::get();

    if (down) {
        auto scheduler = gd::GameManager::sharedState()->getScheduler();

        if (key == 'C') {
            logic.frame_advance = false;
            CCScheduler_update_h(scheduler, 0, 1.f / logic.fps);
            logic.frame_advance = true;
        }
        else if (key == 'V') {
            logic.frame_advance = !logic.frame_advance;
        }
    }


    CCKeyboardDispatcher_dispatchKeyboardMSG(self, key, down);
}


bool __fastcall Hooks::PlayLayer::init_h(gd::PlayLayer* self, void* edx, gd::GJGameLevel* level) {
    auto& logic = Logic::get();
    logic.replay_index = 0;

    bool ret = Hooks::PlayLayer::init(self, level);

    return ret;
}

void __fastcall Hooks::PlayLayer::updateVisibility_h(gd::PlayLayer* self) {
    if (!g_disable_render)
        updateVisibility(self);
}

bool __fastcall Hooks::PauseLayer::init_h(gd::PauseLayer* self) {
    auto& logic = Logic::get();

    if (logic.is_recording()) {
        if (!logic.get_inputs().empty() && logic.get_inputs().front().pressingDown) {
            logic.record_input(false, false);
        }
    }

    return Hooks::PauseLayer::init(self);
}

std::string formatTime(float timeInSeconds) {
    int minutes = static_cast<int>(timeInSeconds / 60);
    int seconds = static_cast<int>(timeInSeconds) % 60;
    int milliseconds = static_cast<int>((timeInSeconds - static_cast<int>(timeInSeconds)) * 100);

    std::stringstream formattedTime;
    formattedTime << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << seconds;

    return formattedTime.str();
}

void __fastcall Hooks::PlayLayer::update_h(gd::PlayLayer* self, int, float dt) {
    auto& logic = Logic::get();

    static int offset = rand();

    if (logic.is_playing() && !logic.get_inputs().empty()) {
        if (logic.sequence_enabled) {
            if (logic.replay_index - 1 < logic.replays.size()) {
                Replay& selected_replay = logic.replays[logic.replay_index - 1];
                static int offset = selected_replay.max_frame_offset > 0 ? (rand() % selected_replay.max_frame_offset / 2) - selected_replay.max_frame_offset : 0;

                if (logic.play_macro(offset)) {
                    offset = selected_replay.max_frame_offset > 0 ? (rand() % selected_replay.max_frame_offset / 2) - selected_replay.max_frame_offset : 0;
                }
            }
        }
        else {
            int _ = 0;
            logic.play_macro(_);
        }
    }

    if (logic.recorder.m_recording)
        logic.recorder.handle_recording(self, dt);

    logic.recorder.m_song_start_offset = self->m_levelSettings->m_songStartOffset;

    // Check if the frame counter label exists
    auto frame_counter = (cocos2d::CCLabelBMFont*)self->getChildByTag(FRAME_LABEL_ID);
    if (frame_counter) {
        if (logic.show_frame) {
            // Update the frame counter label with the current frame number
            char out[24];
            sprintf_s(out, "Frame: %i", logic.get_frame());
            frame_counter->setString(out);
        }
        else {
            // Remove and release the frame counter label if show_frame is false
            frame_counter->removeFromParent();
            frame_counter->release();
        }
    }
    else if (logic.show_frame) {
        auto frame_counter2 = cocos2d::CCLabelBMFont::create("Frame: ", "chatFont.fnt"); //probably leaks memory :p
        frame_counter2->setPosition(cocos2d::CCDirector::sharedDirector()->getWinSize().width / 2.0, cocos2d::CCDirector::sharedDirector()->getWinSize().height / 2.0);
        frame_counter2->setOpacity(70);

        self->addChild(frame_counter2, 999, FRAME_LABEL_ID);
    }


    auto cps_counter = (cocos2d::CCLabelBMFont*)self->getChildByTag(CPS_LABEL_ID);

    if (cps_counter) {
        if (logic.show_cps) {
            char out[24];
            sprintf_s(out, "CPS: %i", logic.count_presses_in_last_second());
            cps_counter->setString(out);
            if (logic.current_cps > logic.max_cps) {
                cps_counter->setColor({ 255, 0, 0 });
            }
            else if (logic.over_max_cps) {
                cps_counter->setColor({ 255, 72, 0 });
            }
        }

        else {
            cps_counter->removeFromParent();
            cps_counter->release();
        }
    }
    else if (logic.show_cps) {
        auto cps_counter2 = cocos2d::CCLabelBMFont::create("CPS: ", "chatFont.fnt");
        cps_counter2->setPosition(30, 20);
        cps_counter2->setOpacity(70);

        self->addChild(cps_counter2, 999, CPS_LABEL_ID);
    }

    if (self->m_isPaused) {
        ImGui::SetKeyboardFocusHere();
    }

    update(self, dt);
}

int __fastcall Hooks::PlayLayer::pushButton_h(gd::PlayLayer* self, int, int idk, bool button) {
    auto& logic = Logic::get();

    if (logic.is_playing() && logic.ignore_actions_at_playback) return 0;

    if (logic.click_both_players) {
        logic.record_input(true, !button);
        pushButton(self, idk, !button);
    }

    if (logic.swap_player_input) button = !button;

    logic.record_input(true, button);

    return pushButton(self, idk, button);
}

int __fastcall Hooks::PlayLayer::releaseButton_h(gd::PlayLayer* self, int, int idk, bool button) {
    auto& logic = Logic::get();

    if (logic.is_playing() && logic.ignore_actions_at_playback) return 0;

    if (logic.click_both_players) {
        logic.record_input(false, !button);
        pushButton(self, idk, !button);
    }

    if (logic.swap_player_input) button = !button;

    logic.record_input(false, button);

    return releaseButton(self, idk, button);
}

int __fastcall Hooks::PlayLayer::resetLevel_h(gd::PlayLayer* self, int idk) {
    int ret = resetLevel(self); // calling the original function
    auto& logic = Logic::get();

    logic.live_inputs.clear();
    logic.cps_over_percents.clear();
    logic.over_max_cps = false;

    logic.end_portal_position = self->m_endPortal->getPositionX();

    auto cps_counter = (cocos2d::CCLabelBMFont*)self->getChildByTag(CPS_LABEL_ID);

    if (cps_counter) {
        cps_counter->setColor({ 255, 255, 255 });
    }

    // Section 1: Handle Sequencing
    if (logic.sequence_enabled) {
        if (!logic.is_recording() && (!logic.is_playing() || logic.sequence_enabled)) {
            Hooks::PlayLayer::releaseButton(self, 0, false);
            Hooks::PlayLayer::releaseButton(self, 0, true);
            if (logic.replay_index < logic.replays.size() && logic.sequence_enabled) {
                logic.get_inputs() = logic.replays[logic.replay_index].actions;
            }
            else {
                logic.replay_index = 0;
                logic.get_inputs().clear();
            }
        }
        logic.replay_index += 1;
    }

    // INITIALIZES MACRO NAME
    if (self->m_currentAttempt == 1) {
        strcpy(logic.macro_name, self->m_level->levelName.c_str());
        logic.recorder.video_name = logic.macro_name;
    }

    if (logic.is_playing())
        logic.set_replay_pos(logic.find_closest_input());

    if (!self->m_isPracticeMode)
        logic.checkpoints.clear();

    if (logic.is_recording() || logic.is_playing())
        logic.handle_checkpoint_data();

    // Section 3: Update Time and Activated Objects
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

    // Section 4: Handle Recording
    if (logic.is_recording()) {

        if (logic.get_inputs().empty()) return ret;

        if (!logic.checkpoints.empty()) {
            logic.set_removed(logic.get_removed() + (logic.get_frame() - logic.get_latest_checkpoint().number));

            if (!logic.no_overwrite)
                logic.remove_inputs(logic.get_frame());

            printf("%i", logic.get_frame());

            if (!logic.get_inputs().empty()) {
                if (logic.get_inputs().back().pressingDown) {
                    bool currently_holdingP1 = self->m_player1->m_isHolding;
                    bool currently_holdingP2 = self->m_player2->m_isHolding;

                    if (!currently_holdingP1) {
                        logic.add_input({ logic.get_latest_checkpoint().number, false, false });
                    }

                    if (!currently_holdingP2 && self->m_level->twoPlayerMode) {
                        logic.add_input({ logic.get_latest_checkpoint().number, false, true });
                    }
                }
                else {
                    bool currently_holdingP1 = self->m_player1->m_isHolding;
                    bool currently_holdingP2 = self->m_player2->m_isHolding;

                    if (currently_holdingP1) {
                        logic.add_input({ logic.get_latest_checkpoint().number, true, false });
                    }

                    if (currently_holdingP2 && self->m_level->twoPlayerMode) {
                        logic.add_input({ logic.get_latest_checkpoint().number, true, true });
                    }
                }
            }
        }
        else {
            if (!logic.no_overwrite)
                logic.remove_inputs(0);

            logic.set_removed(0);
            self->m_time = 0;
        }

        if (self->m_player1->m_isHolding && logic.checkpoints.empty())
            logic.record_input(true, true);
    }

    return ret;
}

void* __fastcall Hooks::PlayLayer::exitLevel_h(gd::PlayLayer* self, int) {
    auto& logic = Logic::get();

    logic.checkpoints.clear();
    logic.set_removed(0);

    return exitLevel(self);
}

int __fastcall Hooks::createCheckpoint_h(gd::PlayLayer* self) {
    auto& logic = Logic::get();

    logic.add_offset(self->m_time);
    CheckpointData checkpointData1 = CheckpointData::create(self->m_player1);
    CheckpointData checkpointData2 = CheckpointData::create(self->m_player2);

    logic.save_checkpoint({ logic.get_frame(), checkpointData1, checkpointData2, logic.activated_objects.size(), logic.activated_objects_p2.size() });

    return createCheckpoint(self);
}

int __fastcall Hooks::removeCheckpoint_h(gd::PlayLayer* self) {
    auto& logic = Logic::get();

    logic.remove_last_offset();
    logic.remove_last_checkpoint();

    return removeCheckpoint(self);
}