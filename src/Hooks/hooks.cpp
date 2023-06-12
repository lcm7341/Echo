#include "Hooks.hpp"
#include "../Logic/logic.hpp"
#include <chrono>

#define FRAME_LABEL_ID 82369 + 1 //random value :P

#define HOOK(o, f) MH_CreateHook(reinterpret_cast<void*>(gd::base + o), f##_h, reinterpret_cast<void**>(&f));
// gracias matcool :]

#define HOOK_COCOS(o, f) MH_CreateHook(GetProcAddress(GetModuleHandleA("libcocos2d.dll"), o), f##_h, reinterpret_cast<void**>(&f));

void Hooks::init_hooks() {
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x1fb780), PlayLayer::init_h, reinterpret_cast<void**>(&PlayLayer::init));
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
        // Create a new frame counter label if it doesn't exist and show_frame is true
        auto frame_counter2 = cocos2d::CCLabelBMFont::create("Frame: ", "chatFont.fnt");
        frame_counter2->setPosition(cocos2d::CCDirector::sharedDirector()->getWinSize().width / 2.0, 10.0);
        frame_counter2->setOpacity(100);
        //cpp is retarded and you can't shadow vars
        auto frame_counter2 = cocos2d::CCLabelBMFont::create("Frame: ", "chatFont.fnt"); //probably leaks memory :p
        frame_counter2->setPosition(cocos2d::CCDirector::sharedDirector()->getWinSize().width / 2.0, cocos2d::CCDirector::sharedDirector()->getWinSize().height / 2.0);
        frame_counter2->setOpacity(70);

        self->addChild(frame_counter2, 999, FRAME_LABEL_ID);
    }

    if (self->m_isPaused) {
        ImGui::SetKeyboardFocusHere();
    }

    update(self, dt);
}

int __fastcall Hooks::PlayLayer::pushButton_h(gd::PlayLayer* self, int, int idk, bool button) {
    auto& logic = Logic::get();

    if (logic.is_playing() && logic.ignore_actions_at_playback) return 0;

    logic.record_input(true, button);

    return pushButton(self, idk, button);
}

int __fastcall Hooks::PlayLayer::releaseButton_h(gd::PlayLayer* self, int, int idk, bool button) {
    auto& logic = Logic::get();

    if (logic.is_playing() && logic.ignore_actions_at_playback) return 0;

    logic.record_input(false, button);

    return releaseButton(self, idk, button);
}

int __fastcall Hooks::PlayLayer::resetLevel_h(gd::PlayLayer* self, int idk) {
    
    int ret = resetLevel(self); // was told i needed to do this, reason why beats me

    std::cout << "Death" << std::endl;

    auto& logic = Logic::get();

    // HANDLE SEQUENCING
    if (logic.sequence_enabled) {
        if (!logic.is_recording() && (!logic.is_playing() || logic.sequence_enabled)) {
            Hooks::PlayLayer::releaseButton(self, 0, false);
            Hooks::PlayLayer::releaseButton(self, 0, true);
            if (logic.replay_index < logic.replays.size() && logic.sequence_enabled) {
                Replay& selected_replay = logic.replays[logic.replay_index];
                logic.get_inputs() = selected_replay.actions;
            }
            else {
                logic.replay_index = 0;
                logic.get_inputs().clear();
            }
        }

        logic.replay_index += 1;
    }

    if (self->m_currentAttempt == 1) {
        strcpy(logic.macro_name, self->m_level->levelName.c_str());
        logic.recorder.video_name = logic.macro_name;
    }

    if (logic.is_playing()) {
        logic.set_replay_pos(logic.find_closest_input());
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
    } else {
        logic.activated_objects.clear();
        logic.activated_objects_p2.clear();
    }

    logic.recorder.update_song_offset(self);

    if (logic.is_recording()) {
        std::cout << "Test 1" << std::endl;

        if (logic.get_inputs().empty()) return ret;

        auto& last = logic.get_inputs().back();

        auto& inputs = logic.get_inputs();
        logic.remove_inputs(logic.get_frame() - 1);
        std::cout << logic.get_frame() - 1 << std::endl;

        if (!logic.checkpoints.empty()) {
            auto currently_holdingP1 = self->m_player1->m_isHolding;
            auto currently_holdingP2 = self->m_player2->m_isHolding;

            std::cout << logic.get_removed() << std::endl;
            std::cout << logic.get_frame() << std::endl;
            std::cout << logic.get_latest_checkpoint().number << std::endl;

            logic.set_removed(logic.get_removed() + (logic.get_frame() - logic.get_latest_checkpoint().number));

            auto& last = logic.get_inputs().back();

            auto& inputs = logic.get_inputs();

            if ((currently_holdingP1 && logic.get_inputs().empty()) || (!logic.get_inputs().empty() && last.pressingDown != currently_holdingP1)) {
                logic.add_input({ logic.get_frame(), true });
                if (currently_holdingP1) {
                    releaseButton(self, 0, true);
                    pushButton(self, 0, true);
                    self->m_player1->m_hasJustHeld = true;
                }
                else if (!inputs.empty() && inputs.back().pressingDown && currently_holdingP1 && logic.checkpoints.size()) {
                    releaseButton(self, 0, true);
                    pushButton(self, 0, true);
                }
            }
        }
        else {
            logic.remove_inputs(0);
        }

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

    logic.save_checkpoint({ logic.get_frame(), checkpointData1, checkpointData2, logic.activated_objects.size(), logic.activated_objects_p2.size()});

    return createCheckpoint(self);
}

int __fastcall Hooks::removeCheckpoint_h(gd::PlayLayer* self) {
    auto& logic = Logic::get();

    logic.remove_last_offset();
    logic.remove_last_checkpoint();

    return removeCheckpoint(self);
}