#include "Hooks.hpp"
#include <chrono>
#include "../Hack/audiopitchHack.hpp"
#include "../Logic/autoclicker.hpp"
#include "../GUI/gui.hpp"
#include "../Logic/speedhack.h"
#include "../Hack/trajectorysimulation.hpp"

#define FRAME_LABEL_ID 82369 + 1 //random value :P
#define CPS_LABEL_ID 82369 + 2 //random value :P
#define CPS_BREAKS_LABEL_ID 82369 + 3 //random value :P
#define RECORDING_LABEL_ID 82369 + 4 //random value :P
#define PERCENT_LABEL_ID 82369 + 5 //random value :P
#define TIME_LABEL_ID 82369 + 6 //random value :P

#define HOOK(o, f) MH_CreateHook(reinterpret_cast<void*>(gd::base + o), f##_h, reinterpret_cast<void**>(&f));
// gracias matcool :]

#define HOOK_COCOS(o, f) MH_CreateHook(GetProcAddress(GetModuleHandleA("libcocos2d.dll"), o), f##_h, reinterpret_cast<void**>(&f));

HitboxNode* drawer;

void Hooks::init_hooks() {
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x1fb780), PlayLayer::init_h, reinterpret_cast<void**>(&PlayLayer::init));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x2029C0), PlayLayer::update_h, reinterpret_cast<void**>(&PlayLayer::update));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x111500), PlayLayer::pushButton_h, reinterpret_cast<void**>(&PlayLayer::pushButton));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x111660), PlayLayer::releaseButton_h, reinterpret_cast<void**>(&PlayLayer::releaseButton));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x20BF00), PlayLayer::resetLevel_h, reinterpret_cast<void**>(&PlayLayer::resetLevel));

    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x20DDD0), createCheckpoint_h, reinterpret_cast<void**>(&createCheckpoint));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x20b830), removeCheckpoint_h, reinterpret_cast<void**>(&removeCheckpoint));

    //MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x20a1a0), PlayLayer::destroyPlayer_h, reinterpret_cast<void**>(&PlayLayer::destroyPlayer));

    //MH_CreateHook(reinterpret_cast<void*>(gd::base + 0xe5d60), powerOffObject_h, reinterpret_cast<void**>(&powerOffObject));
    //MH_CreateHook(reinterpret_cast<void*>(gd::base + 0xeab20), playShineEffect_h, reinterpret_cast<void**>(&playShineEffect));
    //MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x1e9a20), incrementJumps_h, reinterpret_cast<void**>(&incrementJumps));
    ////MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x10ed50), bumpPlayer_h, reinterpret_cast<void**>(&bumpPlayer)); // crashes idk why (wtf is bumpplayer)
    //MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x1f62c0), toggleDartMode_h, reinterpret_cast<void**>(&toggleDartMode));
    //MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x14ebc0), addPoint_h, reinterpret_cast<void**>(&addPoint));
    //MH_CreateHook(reinterpret_cast<void*>(gd::base + 0xd1790), triggerObject_h, reinterpret_cast<void**>(&triggerObject));
    //MH_CreateHook(reinterpret_cast<void*>(gd::base + 0xEF110), hasBeenActivatedByPlayer_h, reinterpret_cast<void**>(&hasBeenActivatedByPlayer));
    //MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x20d810), PlayLayer::onQuit_h, reinterpret_cast<void**>(&PlayLayer::onQuit));
    //MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x207d30), PlayLayer::flipGravity_h, reinterpret_cast<void**>(&PlayLayer::flipGravity));
    //MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x207e00), PlayLayer::playGravityEffect_h, reinterpret_cast<void**>(&PlayLayer::playGravityEffect));

    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x1E4570), PauseLayer::init_h, reinterpret_cast<void**>(&PauseLayer::init));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x20D810), PlayLayer::exitLevel_h, reinterpret_cast<void**>(&PlayLayer::exitLevel));

    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x1f4ff0), PlayerObject_ringJump_h, reinterpret_cast<void**>(&PlayerObject_ringJump));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0xef0e0), GameObject_activateObject_h, reinterpret_cast<void**>(&GameObject_activateObject));

    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x20A1A0), PlayLayer::death_h, reinterpret_cast<void**>(&PlayLayer::death));

    MH_CreateHook(GetProcAddress(GetModuleHandleA("libcocos2d.dll"), "?dispatchKeyboardMSG@CCKeyboardDispatcher@cocos2d@@QAE_NW4enumKeyCodes@2@_N@Z"), CCKeyboardDispatcher_dispatchKeyboardMSG_h, reinterpret_cast<void**>(&CCKeyboardDispatcher_dispatchKeyboardMSG));;

    HOOK(0x205460, PlayLayer::updateVisibility)

    MH_CreateHook(GetProcAddress(GetModuleHandleA("libcocos2d.dll"), "?update@CCScheduler@cocos2d@@UAEXM@Z"), CCScheduler_update_h, reinterpret_cast<void**>(&CCScheduler_update));

    MH_EnableHook(MH_ALL_HOOKS);
}

bool g_disable_render = false;
float g_left_over = 0.f;

bool __fastcall Hooks::PlayLayer::death_h(void* self, void*, void* go, void* thingy) {
    auto& logic = Logic::get();

    if (gd::GameManager::sharedState()->getPlayLayer()->m_pPlayer1 == go && logic.noclip_player1) {
        return 0; 
    }

    if (gd::GameManager::sharedState()->getPlayLayer()->m_bIsDualMode) {
        if (gd::GameManager::sharedState()->getPlayLayer()->m_pPlayer2 == go && logic.noclip_player2) {
            return 0;
        }
    }

    if (Logic::get().hacks.trajectory && TrajectorySimulation::getInstance()->shouldInterrumpHooksWithPlayer((gd::PlayerObject*)go))
    {
        TrajectorySimulation::getInstance()->m_pDieInSimulation = true;
        return 0;
    }

    return Hooks::PlayLayer::death(self, go, thingy);

}

void __fastcall Hooks::CCScheduler_update_h(CCScheduler* self, int, float dt) {
    auto& logic = Logic::get();
    auto play_layer = gd::GameManager::sharedState()->getPlayLayer();

    if (!logic.is_recording() && !logic.is_playing()) g_disable_render = false;

    if (logic.frame_advance && !play_layer) logic.frame_advance = false;

    if (logic.recorder.m_recording && play_layer && !play_layer->m_bIsPaused)
        logic.recorder.handle_recording(play_layer, dt);

    if (logic.frame_advance) return;

    if (GUI::get().change_display_fps) {
        CCDirector::sharedDirector()->setAnimationInterval(1.f / GUI::get().input_fps);
        GUI::get().change_display_fps = false;
    }

    if (logic.recorder.m_recording && !logic.recorder.real_time_rendering) {
        CCDirector::sharedDirector()->setAnimationInterval(1.f / logic.fps);
        dt = 1.f / logic.fps;
        Speedhack::SetSpeed(0.5);
        return CCScheduler_update(self, dt);
    }
    else {
        Speedhack::SetSpeed(1);
    }

    if (logic.autoclicker && play_layer && !play_layer->m_bIsPaused) {
        Autoclicker::get().update();

        if (Autoclicker::get().shouldPress()) {
            if (logic.autoclicker_player_1)
                gd::GameManager::sharedState()->getPlayLayer()->pushButton(0, false);
            if (logic.autoclicker_player_2 && play_layer->m_level->twoPlayerMode)
                gd::GameManager::sharedState()->getPlayLayer()->pushButton(0, true);
        }
        if (Autoclicker::get().shouldRelease()) {
            if (logic.autoclicker_player_1)
                gd::GameManager::sharedState()->getPlayLayer()->releaseButton(0, false);
            if (logic.autoclicker_player_2 && play_layer->m_level->twoPlayerMode)
                gd::GameManager::sharedState()->getPlayLayer()->releaseButton(0, true);
        }

        if (logic.autoclicker_auto_disable) {
            logic.autoclicker_disable_in--;
            if (logic.autoclicker_disable_in == 0) {
                logic.autoclicker_auto_disable = false;
                logic.autoclicker = false;
            }
        }
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

        if (logic.recorder.m_recording) { // prevent screen tearing from lag in the first bits of lvl
            if ((logic.get_frame() / logic.fps) < 1.f || (1.f / dt) < logic.recorder.m_fps || !logic.recorder.real_time_rendering) {
                CCDirector::sharedDirector()->setAnimationInterval(1.f / logic.fps);
                dt = 1.f / logic.fps;
                return CCScheduler_update(self, dt);
            }
        }

        const float target_dt = 1.f / logic.get_fps() / logic.speedhack;

        float speedhack = logic.speedhack;

        g_disable_render = true;

        // min(static_cast<int>(g_left_over / target_dt), 50) <- super fast but i think its inaccurate
        //const int times = min(round((dt + g_left_over) / target_dt), 150);
        const int times = min(round(g_left_over / target_dt), 150);

        for (int i = 0; i < times; i++) {
            if (i == times - 1) {
                g_disable_render = false;
            }
            CCScheduler_update(self, target_dt);
        }
        g_left_over += dt - target_dt * times;

        return;
    }
    else {
        return CCScheduler_update(self, dt);
    }
}

void __fastcall Hooks::CCKeyboardDispatcher_dispatchKeyboardMSG_h(CCKeyboardDispatcher* self, int, int key, bool down) {
    auto& logic = Logic::get();

    if (down && gd::GameManager::sharedState()->getPlayLayer()) {
        auto advancing_key = Logic::get().keybinds.GetKeybind("advancing").key;
        if (advancing_key.has_value()) {
            if (ImGui::IsKeyPressed(advancing_key.value())) {
                Logic::get().start = std::chrono::steady_clock::now();
                Logic::get().frame_advance = false;
                bool old_real_time = Logic::get().real_time_mode;
                Logic::get().real_time_mode = false;
                Hooks::CCScheduler_update_h(gd::GameManager::sharedState()->getScheduler(), 0, 1.f / logic.fps);
                Logic::get().frame_advance = true;
                Logic::get().real_time_mode = old_real_time;
            }
        }
    }
    else {
        Logic::get().start = std::chrono::steady_clock::time_point();
    }

    CCKeyboardDispatcher_dispatchKeyboardMSG(self, key, down);
}

bool __fastcall Hooks::PlayLayer::init_h(gd::PlayLayer* self, void* edx, gd::GJGameLevel* level) {
    auto& logic = Logic::get();
    logic.replay_index = 0;
    logic.clickbot_start = self->m_time;

    drawer = HitboxNode::getInstance();

    bool ret = Hooks::PlayLayer::init(self, level);

    TrajectorySimulation::getInstance()->createSimulation();

    return ret;
}

void __fastcall Hooks::PlayLayer::updateVisibility_h(gd::PlayLayer* self) {
    if (!g_disable_render)
        updateVisibility(self);
}

void __fastcall Hooks::PlayLayer::destroyPlayer_h(gd::PlayLayer* self, gd::PlayerObject* one, gd::GameObject* two) {
    /*auto& logic = Logic::get();

    if (gd::GameManager::sharedState()->getPlayLayer()->m_pPlayer1 == one && logic.noclip_player1) {
        return;
    }

    if (gd::GameManager::sharedState()->getPlayLayer()->m_bIsDualMode) {
        if (gd::GameManager::sharedState()->getPlayLayer()->m_pPlayer2 == one && logic.noclip_player2) {
            return;
        }
    }

    if (Logic::get().hacks.trajectory && TrajectorySimulation::getInstance()->shouldInterrumpHooksWithPlayer(one))
    {
        TrajectorySimulation::getInstance()->m_pDieInSimulation = true;
        return;
    }*/
    destroyPlayer(self, one, two);
}

bool __fastcall Hooks::PauseLayer::init_h(gd::PauseLayer* self) {
    auto& logic = Logic::get();

    if (logic.is_recording()) {
        auto& inputs = logic.get_inputs();
        auto last_input_p1 = std::find_if(inputs.rbegin(), inputs.rend(), [](Frame input) { return input.isPlayer2 == false; });
        auto last_input_p2 = std::find_if(inputs.rbegin(), inputs.rend(), [](Frame input) { return input.isPlayer2 == true; });

        if (last_input_p1 != inputs.rend() && last_input_p1->pressingDown && !logic.get_latest_checkpoint().player_1_data.m_isDashing) {
            logic.record_input(false, false);
        }

        if (last_input_p2 != inputs.rend() && last_input_p2->pressingDown && !logic.get_latest_checkpoint().player_2_data.m_isDashing) {
            logic.record_input(false, true);
        }
    }

    return Hooks::PauseLayer::init(self);
}

std::string formatTime(float timeInSeconds) {
    int hours = static_cast<int>(timeInSeconds / 60 / 60);
    int minutes = static_cast<int>(timeInSeconds / 60);
    int seconds = static_cast<int>(timeInSeconds) % 60;
    int milliseconds = static_cast<int>((timeInSeconds - static_cast<int>(timeInSeconds)) * 100);

    std::stringstream formattedTime;
    formattedTime << std::setfill('0') << std::setw(2) << hours << ":"
        << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << seconds;

    return formattedTime.str();
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

void __fastcall Hooks::PlayLayer::update_h(gd::PlayLayer* self, int, float dt) {
    auto& logic = Logic::get();

    static int offset = rand();

    if (drawer && logic.hacks.showHitboxes) {
        drawer->setVisible(true);

        if (self->m_pPlayer1) {
            drawer->drawForPlayer1(self->m_pPlayer1);
        }
        if (self->m_pPlayer2) {
            drawer->drawForPlayer2(self->m_pPlayer2);
        }

    }

    if (drawer && logic.hacks.showHitboxes)
    {
        drawer->setVisible(true);

        if (self->m_pPlayer1) {
            drawer->drawForPlayer1(self->m_pPlayer1);
        }
        if (self->m_pPlayer2) {
            drawer->drawForPlayer2(self->m_pPlayer2);
        }

        for (int s = self->sectionForPos(self->m_pPlayer1->getPositionX()) - 5; s < self->sectionForPos(self->m_pPlayer1->getPositionX()) + 6; ++s)
        {
            if (s < 0)
                continue;
            if (s >= self->m_sectionObjects->count())
                break;
            auto section = static_cast<CCArray*>(self->m_sectionObjects->objectAtIndex(s));
            for (size_t i = 0; i < section->count(); ++i)
            {
                cast_function caster = make_cast<gd::GameObject, CCObject>();
                auto obj = reinterpret_cast<gd::GameObject*>(caster(section->objectAtIndex(i)));
                drawer->drawForObject(obj);

                if (obj->m_nObjectID != 749 && obj->getObjType() == gd::GameObjectType::kGameObjectTypeDecoration)
                    continue;
                if (!obj->m_bActive)
                    continue;

                //drawer->drawForObject(obj);
            }
        }

    }

    if (self->m_isDead && logic.recorder.m_recording) {
        self->m_time += dt;
    }

    if (self->m_hasCompletedLevel) {
        if (logic.is_recording()) logic.toggle_recording();
    }

    /*if (drawer && !Logic::get().hacks.trajectory)
        drawer->clear();*/

    logic.player_acceleration = self->m_pPlayer1->m_xAccel;
    logic.player_speed = self->m_pPlayer1->m_playerSpeed;

    logic.previous_real_xpos = self->m_pPlayer1->getPositionX();

    if (!logic.player_x_positions.empty()) {
        if (!self->m_isDead && (self->m_pPlayer1->m_position.x != logic.player_x_positions.back() || self->m_hasCompletedLevel)) {
            if (self->m_hasCompletedLevel) {
                logic.calculated_xpos = self->m_pPlayer1->getPositionX();
                logic.calculated_frame = logic.get_frame();
            }
            else {

                logic.calculated_xpos = logic.xpos_calculation();
                logic.calculated_frame = round(logic.get_frame() + (self->m_pPlayer1->getPositionX() - logic.calculated_xpos));//round((logic.calculated_xpos / (logic.player_speed * logic.player_acceleration) * (1.f / logic.fps)) * logic.fps); // doesnt work when changing speeds, fucjk

                logic.previous_xpos = logic.calculated_xpos;
            }
        }
    }

    logic.player_x_positions.push_back(self->m_pPlayer1->m_position.x);

    logic.recorder.m_song_start_offset = self->m_pLevelSettings->m_songStartOffset;

    // Check if the frame counter label exists
    auto frame_counter = (cocos2d::CCLabelBMFont*)self->getChildByTag(FRAME_LABEL_ID);
    if (frame_counter) {
        if (logic.show_frame) {
            // Update the frame counter label with the current frame number
            char out[1000];
            int frame = logic.get_frame();
            std::string text = std::to_string(frame);
            int length = text.length();
            sprintf_s(out, "Frame: %i", frame);
            frame_counter->setAnchorPoint({ 0, 0.5 });
            frame_counter->setPosition(logic.frame_counter_x, logic.frame_counter_y); // Adjusted the x-position calculation
            frame_counter->setString(out);
            frame_counter->setOpacity(logic.frame_counter_opacity);
            frame_counter->setScale(logic.frame_counter_scale);

        }
        else {
            // Remove and release the frame counter label if show_frame is false
            frame_counter->removeFromParent();
            frame_counter->release();
        }
    }
    else if (logic.show_frame) {
        auto frame_counter2 = cocos2d::CCLabelBMFont::create("Frame: ", "bigFont.fnt"); //probably leaks memory :p
        frame_counter2->setPosition(logic.frame_counter_x, logic.frame_counter_y);
        frame_counter2->setOpacity(logic.frame_counter_opacity);
        frame_counter2->setScale(logic.frame_counter_scale);
        self->addChild(frame_counter2, 999, FRAME_LABEL_ID);
    }

    auto recording_label = (cocos2d::CCLabelBMFont*)self->getChildByTag(RECORDING_LABEL_ID);
    if (recording_label) {
        if (logic.is_recording()) {
            // Update the frame counter label with the current frame number
            char out[1000];
            int holds_count = 0;
            int releases_count = 0;
            for (auto& input : logic.inputs) {
                if (input.pressingDown) holds_count++;
                else releases_count++;
            }
            sprintf_s(out, "Recording: %i/%i", holds_count, releases_count);
            recording_label->setString(out);
            recording_label->setPosition(logic.recording_label_x, logic.recording_label_y);
            recording_label->setOpacity(logic.recording_label_opacity);
            recording_label->setScale(logic.recording_label_scale);
            recording_label->setAnchorPoint({ 0, 0.5 });
        }
        else {
            // Remove and release the frame counter label if show_frame is false
            recording_label->removeFromParent();
            recording_label->release();
        }
    }
    else if (logic.is_recording() && logic.show_recording) {
        auto recording_label2 = cocos2d::CCLabelBMFont::create("Recording: 0/0", "bigFont.fnt"); //probably leaks memory :p
        //recording_label2->setPosition(cocos2d::CCDirector::sharedDirector()->getWinSize().width / 2.f, 20);
        recording_label2->setPosition(logic.recording_label_x, logic.recording_label_y);
        recording_label2->setOpacity(logic.recording_label_opacity);
        recording_label2->setScale(logic.recording_label_scale);

        self->addChild(recording_label2, 999, RECORDING_LABEL_ID);
    }

    auto cps_counter = (cocos2d::CCLabelBMFont*)self->getChildByTag(CPS_LABEL_ID);

    if (cps_counter) {
        if (logic.show_cps) {
            char out[24];
            sprintf_s(out, "CPS: %i/%i", logic.count_presses_in_last_second(false), logic.count_presses_in_last_second(true));
            cps_counter->setString(out);
            if (logic.current_cps > logic.max_cps) {
                cps_counter->setColor({ 255, 0, 0 });
            }
            else if (logic.over_max_cps) {
                cps_counter->setColor({ 255, 72, 0 });
            }
            cps_counter->setAnchorPoint({ 0, 0.5 });
            cps_counter->setPosition(logic.cps_counter_x, logic.cps_counter_y);
            cps_counter->setOpacity(logic.cps_counter_opacity);
            cps_counter->setScale(logic.cps_counter_scale);
        }

        else {
            cps_counter->removeFromParent();
            cps_counter->release();
        }
    }
    else if (logic.show_cps) {
        auto cps_counter2 = cocos2d::CCLabelBMFont::create("CPS: 0/0", "bigFont.fnt");
        cps_counter2->setPosition(logic.cps_counter_x, logic.cps_counter_y);
        cps_counter2->setOpacity(logic.cps_counter_opacity);
        cps_counter2->setScale(logic.cps_counter_scale);

        self->addChild(cps_counter2, 999, CPS_LABEL_ID);
    }

    auto percent_label = (cocos2d::CCLabelBMFont*)self->getChildByTag(PERCENT_LABEL_ID);;

    if (percent_label) {
        if (logic.show_percent) {
            char out[24];
            std::stringstream stream;
            float percent = min(100.f, (self->m_pPlayer1->getPositionX() / self->m_endPortal->getPositionX()) * 100.f);
            if (logic.percent_accuracy > 0) {
                stream << "%." << logic.percent_accuracy << "f%%";
                sprintf_s(out, stream.str().c_str(), percent);
            }
            else {
                stream << "%i%%";
                sprintf_s(out, stream.str().c_str(), static_cast<int>(percent));
            }
            percent_label->setString(out);
            percent_label->setAnchorPoint({ 0, 0.5 });
            percent_label->setPosition(logic.percent_counter_x, logic.percent_counter_y);
            percent_label->setScale(logic.percent_scale);
            percent_label->setOpacity(logic.percent_opacity);
        }

        else {
            percent_label->removeFromParent();
            percent_label->release();
        }
    }
    else if (logic.show_percent) {
        auto percent_label2 = cocos2d::CCLabelBMFont::create("", "bigFont.fnt");
        percent_label2->setPosition(logic.percent_counter_x, logic.percent_counter_y);
        percent_label2->setScale(logic.percent_scale);
        percent_label2->setOpacity(logic.percent_opacity);

        self->addChild(percent_label2, 999, PERCENT_LABEL_ID);
    }

    auto time_label = (cocos2d::CCLabelBMFont*)self->getChildByTag(TIME_LABEL_ID);;
    if (time_label) {

        if (logic.show_time) {
            char out[24];
            const char* format = "%s";
            std::string str = formatTime(self->m_time);
            sprintf_s(out, str.c_str());
            time_label->setString(out);
            time_label->setAnchorPoint({ 0, 0.5 });
            time_label->setPosition(logic.time_counter_x, logic.time_counter_y);
            time_label->setScale(logic.time_scale);
            time_label->setOpacity(logic.time_opacity);
        }

        else {
            time_label->removeFromParent();
            time_label->release();
        }
    }
    else if (logic.show_time) {
        auto time_label2 = cocos2d::CCLabelBMFont::create("", "bigFont.fnt");
        time_label2->setPosition(logic.time_counter_x, logic.time_counter_y);
        time_label2->setScale(logic.time_scale);
        time_label2->setOpacity(logic.percent_opacity);

        self->addChild(time_label2, 999, TIME_LABEL_ID);
    }

    if (Logic::get().hacks.trajectory)
        TrajectorySimulation::getInstance()->processMainSimulation(dt);

    logic.is_over_orb = false;

    update(self, dt);

    // this is rotation bug fix for playback vvvvvv we MUST call it after update() or else the click isnt registered in time

    if (logic.is_playing() && !logic.get_inputs().empty()) {
        if (logic.sequence_enabled) {
            if (logic.replay_index - 1 < logic.replays.size()) {
                Replay& selected_replay = logic.replays[logic.replay_index - 1];
                static int offset = selected_replay.max_frame_offset > 0 ? (rand() % selected_replay.max_frame_offset / 2) - selected_replay.max_frame_offset : 0;

                if (logic.play_macro()) {
                    offset = selected_replay.max_frame_offset > 0 ? (rand() % selected_replay.max_frame_offset / 2) - selected_replay.max_frame_offset : 0;
                }
            }
        }
        else {
            logic.play_macro();
        }
    }
}

int __fastcall Hooks::PlayLayer::pushButton_h(gd::PlayLayer* self, int, int idk, bool button) {
    auto& logic = Logic::get();

    if (!self->m_level->twoPlayerMode || !self->m_bIsDualMode) {
        button = false;
    }

    if (!logic.is_playing() && !logic.is_recording()) {
        logic.live_inputs.push_back({ logic.get_frame(), true, button, self->getPositionY(), self->getPositionX(), self->getRotation(), 0.f, 0.f });
    }

    if ((logic.clickbot_enabled && !logic.is_playing()) || (logic.clickbot_enabled && logic.playback_clicking)) {
        button = !button; // i fucked up before and done wanna care
        if (!Clickbot::inited)
        {
            FMOD::System_Create(&Clickbot::system);
            Clickbot::system->init(1024 * 2, FMOD_INIT_NORMAL, nullptr);
            Clickbot::inited = true;
        }

        logic.clickbot_now = self->m_time;
        logic.cycleTime = logic.clickbot_now - logic.clickbot_start;
        bool micros = button ? logic.player_1_micros : logic.player_2_micros;
        bool softs = button ? logic.player_1_softs : logic.player_2_softs;
        bool hards = button ? logic.player_1_hards : logic.player_2_hards;
        if (logic.cycleTime <= (button ? logic.player_1_micros_time / 1000.f : logic.player_2_micros_time / 1000.f) && micros)
        {
            std::string path = Clickbot::pickRandomFile(button ? logic.player_1_path : logic.player_2_path, "micro_clicks");
            logic.clickbot_start = self->m_time;
            Clickbot::system->createSound(path.c_str(), FMOD_DEFAULT, nullptr, button ? &Clickbot::clickSound : &Clickbot::clickSound2);
            if (button) {
                Clickbot::system->playSound(Clickbot::clickSound, nullptr, true, &Clickbot::clickChannel);
                Clickbot::clickChannel->setVolume((float)(logic.player_1_micros_volume * 5 * logic.clickbot_volume_multiplier));
            }
            else {
                Clickbot::system->playSound(Clickbot::clickSound2, nullptr, true, &Clickbot::clickChannel2);
                Clickbot::clickChannel2->setVolume((float)(logic.player_2_micros_volume * 5 * logic.clickbot_volume_multiplier));
            }
        }
        else if (logic.cycleTime <= (button ? logic.player_1_softs_time / 1000.f : logic.player_2_softs_time / 1000.f) && softs)
        {
            std::string path = Clickbot::pickRandomFile(button ? logic.player_1_path : logic.player_2_path, "soft_clicks");
            logic.clickbot_start = self->m_time;
            Clickbot::system->createSound(path.c_str(), FMOD_DEFAULT, nullptr, button ? &Clickbot::clickSound : &Clickbot::clickSound2);
            if (button) {
                Clickbot::system->playSound(Clickbot::clickSound, nullptr, true, &Clickbot::clickChannel);
                Clickbot::clickChannel->setVolume((float)(logic.player_1_softs_volume * 5 * logic.clickbot_volume_multiplier));
            }
            else {
                Clickbot::system->playSound(Clickbot::clickSound2, nullptr, true, &Clickbot::clickChannel2);
                Clickbot::clickChannel2->setVolume((float)(logic.player_2_softs_volume * 5 * logic.clickbot_volume_multiplier));
            }
        }
        else if (logic.cycleTime >= (button ? logic.player_1_hards_time : logic.player_2_hards_time) && hards) {
            std::string path = Clickbot::pickRandomFile(button ? logic.player_1_path : logic.player_2_path, "hard_clicks");
            logic.clickbot_start = self->m_time;
            Clickbot::system->createSound(path.c_str(), FMOD_DEFAULT, nullptr, button ? &Clickbot::clickSound : &Clickbot::clickSound2);
            if (button) {
                Clickbot::system->playSound(Clickbot::clickSound, nullptr, true, &Clickbot::clickChannel);
                Clickbot::clickChannel->setVolume((float)(logic.player_1_hards_volume * 5 * logic.clickbot_volume_multiplier));
            }
            else {
                Clickbot::system->playSound(Clickbot::clickSound2, nullptr, true, &Clickbot::clickChannel2);
                Clickbot::clickChannel2->setVolume((float)(logic.player_2_hards_volume * 5 * logic.clickbot_volume_multiplier));
            }
        }
        else {
            std::string path = Clickbot::pickRandomFile(button ? logic.player_1_path : logic.player_2_path, "clicks");
            logic.clickbot_start = self->m_time;
            Clickbot::system->createSound(path.c_str(), FMOD_DEFAULT, nullptr, button ? &Clickbot::clickSound : &Clickbot::clickSound2);
            if (button) {
                Clickbot::system->playSound(Clickbot::clickSound, nullptr, true, &Clickbot::clickChannel);
                Clickbot::clickChannel->setVolume((float)(logic.player_1_volume * 5 * logic.clickbot_volume_multiplier));
            }
            else {
                Clickbot::system->playSound(Clickbot::clickSound2, nullptr, true, &Clickbot::clickChannel2);
                Clickbot::clickChannel2->setVolume((float)(logic.player_2_volume * 5 * logic.clickbot_volume_multiplier));
            }
        }

        if (button) {
            Clickbot::clickChannel->setPaused(false);
        }
        else {
            Clickbot::clickChannel2->setPaused(false);
        }
        button = !button; // i fucked up before and done wanna care
    }

    if (logic.is_playing()) {
        if (logic.ignore_actions_at_playback) {
            if (button && !logic.record_player_1)
                return 0;
            if (!button && !logic.record_player_2)
                return 0;
            if (logic.play_player_1 && logic.play_player_2)
                return 0;
        }
    }

    if (button && self->m_pPlayer1->m_isDashing) {
        return 0;
    }

    if (!button && self->m_pPlayer2->m_isDashing) {
        return 0;
    }

    if (logic.click_both_players && self->m_level->twoPlayerMode) {
        logic.record_input(true, !button);
        pushButton(self, idk, !button);
    }

    if (logic.swap_player_input) button = !button;


    if (logic.is_recording()) {
        if (button && !logic.record_player_1)
            return 0;
        if (!button && !logic.record_player_2)
            return 0;
    }

    if (logic.click_inverse_p2 || logic.click_inverse_p1)
        logic.record_input(false, button);
    else
        logic.record_input(true, button);

    if (button ? logic.click_inverse_p2 : logic.click_inverse_p1) {
        releaseButton(self, idk, button);
        return 0;
    }

    return pushButton(self, idk, button);
}

int __fastcall Hooks::PlayLayer::releaseButton_h(gd::PlayLayer* self, int, int idk, bool button) {
    auto& logic = Logic::get();

    printf("Released\n");

    if (!self->m_level->twoPlayerMode) {
        button = false;
    }

    if ((logic.clickbot_enabled && !logic.is_playing()) || (logic.clickbot_enabled && logic.playback_releasing)) {
        button = !button; // i fucked up before and done wanna care
        if (!Clickbot::inited)
        {
            FMOD::System_Create(&Clickbot::system);
            Clickbot::system->init(1024 * 2, FMOD_INIT_NORMAL, nullptr);
            Clickbot::inited = true;
        }

        logic.clickbot_now = self->m_time;
        logic.cycleTime = logic.clickbot_now - logic.clickbot_start;
        if (logic.cycleTime <= (button ? logic.player_1_micros_time / 1000.f : logic.player_2_micros_time / 1000.f))
        {
            std::string path = Clickbot::pickRandomFile(button ? logic.player_1_path : logic.player_2_path, "micro_clicks");
            logic.clickbot_start = self->m_time;
            Clickbot::system->createSound(path.c_str(), FMOD_DEFAULT, nullptr, button ? &Clickbot::clickSound : &Clickbot::clickSound2);
            if (button) {
                Clickbot::system->playSound(Clickbot::clickSound, nullptr, true, &Clickbot::releaseChannel);
                Clickbot::releaseChannel->setVolume((float)(logic.player_1_micros_volume * 5 * logic.clickbot_volume_multiplier));
            }
            else {
                Clickbot::system->playSound(Clickbot::clickSound2, nullptr, true, &Clickbot::releaseChannel2);
                Clickbot::releaseChannel2->setVolume((float)(logic.player_2_micros_volume * 5 * logic.clickbot_volume_multiplier));
            }
        }
        else if (logic.cycleTime < button ? logic.player_1_softs_time / 1000.f : logic.player_2_softs_time / 1000.f)
        {
            std::string path = Clickbot::pickRandomFile(button ? logic.player_1_path : logic.player_2_path, "soft_releases");
            logic.clickbot_start = self->m_time;
            Clickbot::system->createSound(path.c_str(), FMOD_DEFAULT, nullptr, button ? &Clickbot::releaseSound : &Clickbot::releaseSound2);
            if (button) {
                Clickbot::system->playSound(Clickbot::releaseSound, nullptr, true, &Clickbot::releaseChannel);
                Clickbot::releaseChannel->setVolume((float)(logic.player_1_softs_volume * 5 * logic.clickbot_volume_multiplier));
            }
            else {
                Clickbot::system->playSound(Clickbot::releaseSound2, nullptr, true, &Clickbot::releaseChannel2);
                Clickbot::releaseChannel2->setVolume((float)(logic.player_2_softs_volume * 5 * logic.clickbot_volume_multiplier));
            }
        }
        else if (logic.cycleTime > button ? logic.player_1_hards_time : logic.player_2_hards_time) {
            std::string path = Clickbot::pickRandomFile(button ? logic.player_1_path : logic.player_2_path, "hard_releases");
            logic.clickbot_start = self->m_time;
            Clickbot::system->createSound(path.c_str(), FMOD_DEFAULT, nullptr, button ? &Clickbot::releaseSound : &Clickbot::releaseSound2);

            if (button) {
                Clickbot::system->playSound(Clickbot::releaseSound, nullptr, true, &Clickbot::releaseChannel);
                Clickbot::releaseChannel->setVolume((float)(logic.player_1_hards_volume * 5 * logic.clickbot_volume_multiplier));
            }
            else {
                Clickbot::system->playSound(Clickbot::releaseSound2, nullptr, true, &Clickbot::releaseChannel2);
                Clickbot::releaseChannel2->setVolume((float)(logic.player_2_hards_volume * 5 * logic.clickbot_volume_multiplier));
            }
        }
        else {
            std::string path = Clickbot::pickRandomFile(button ? logic.player_1_path : logic.player_2_path, "releases");
            logic.clickbot_start = self->m_time;
            Clickbot::system->createSound(path.c_str(), FMOD_DEFAULT, nullptr, button ? &Clickbot::releaseSound : &Clickbot::releaseSound2);
            if (button) {
                Clickbot::system->playSound(Clickbot::releaseSound, nullptr, true, &Clickbot::releaseChannel);
                Clickbot::releaseChannel->setVolume((float)(logic.player_1_volume * 5 * logic.clickbot_volume_multiplier));
            }
            else {
                Clickbot::system->playSound(Clickbot::releaseSound2, nullptr, true, &Clickbot::releaseChannel2);
                Clickbot::releaseChannel2->setVolume((float)(logic.player_2_volume * 5 * logic.clickbot_volume_multiplier));
            }
        }

        if (button) {
            Clickbot::releaseChannel->setPaused(false);
        }
        else {

            Clickbot::releaseChannel2->setPaused(false);
        }
        Clickbot::system->update();
        button = !button; // i fucked up before and done wanna care
    }

    if (logic.is_playing()) {
        if (logic.ignore_actions_at_playback) {
            if (logic.ignore_actions_at_playback) {
                if (button && !logic.record_player_1)
                    return 0;
                if (!button && !logic.record_player_2)
                    return 0;
                if (logic.play_player_1 && logic.play_player_2)
                    return 0;
            }
        }
    }

    if (logic.click_both_players && self->m_level->twoPlayerMode) {
        logic.record_input(false, !button);
        releaseButton(self, idk, !button);
    }

    if (logic.swap_player_input) button = !button;

    if (logic.is_recording()) {
        if (button && !logic.record_player_1)
            return 0;
        if (!button && !logic.record_player_2)
            return 0;
    }
    if (logic.click_inverse_p2 || logic.click_inverse_p1)
        logic.record_input(true, button);
    else
        logic.record_input(false, button);

    if (button ? logic.click_inverse_p2 : logic.click_inverse_p1) {
        pushButton(self, idk, button);
        return 0;
    }

    return releaseButton(self, idk, button);
}

void __fastcall Hooks::incrementJumps_h(gd::PlayerObject* self) {
    incrementJumps(self);

    if (gd::GameManager::sharedState()->getPlayLayer() && gd::GameManager::sharedState()->getPlayLayer()->m_pPlayer1 == self) {}
    else if (gd::GameManager::sharedState()->getPlayLayer() && gd::GameManager::sharedState()->getPlayLayer()->m_pPlayer2 == self) {}
    else
    {
        self->stopActionByTag(0);
        self->runNormalRotation();
    }
}

void __fastcall Hooks::triggerObject_h(gd::GameObject* self, gd::GJBaseGameLayer* layer)
{
    if (Logic::get().hacks.trajectory && TrajectorySimulation::getInstance()->shouldInterrumpHooks())
        return;
    auto id = self->m_nObjectID;
    /*if (hacks.layoutMode && (id == 899 || id == 1006 || id == 1007 || id == 105 || id == 29 || id == 56 || id == 915 ||
        id == 30 || id == 58))
        return;*/
    triggerObject(self, layer);
}

int __fastcall Hooks::PlayLayer::resetLevel_h(gd::PlayLayer* self, int idk) {
    int ret = resetLevel(self); // calling the original function
    auto& logic = Logic::get();

    logic.clickbot_now = self->m_time;
    logic.clickbot_start = self->m_time;
    self->m_time = self->timeForXPos(self->m_pPlayer1->getPositionX());

    printf("\n");

    if (logic.is_playing() && !logic.inputs.empty()) {
            logic.set_replay_pos(logic.find_closest_input());
    }

    if (logic.is_recording()) logic.total_attempt_count++;

    logic.calculated_xpos = self->m_pPlayer1->getPositionX();
    logic.calculated_frame = round(logic.get_frame() + (self->m_pPlayer1->getPositionX() - logic.calculated_xpos));
    logic.previous_xpos = self->m_pPlayer1->getPositionX();
    logic.player_x_positions.clear();

    logic.player_x_positions.push_back(self->m_pPlayer1->getPositionX());

    //logic.calculated_xpos = logic.xpos_calculation();
    logic.previous_xpos = logic.xpos_calculation();

    /* if (logic.is_playing()) {
         releaseButton(self, 0, true);
         releaseButton(self, 0, false);
     }*/

    logic.live_inputs.clear();
    logic.over_max_cps = false;

    logic.end_portal_position = self->m_endPortal->getPositionX();

    auto cps_counter = (cocos2d::CCLabelBMFont*)self->getChildByTag(CPS_LABEL_ID);

    TrajectorySimulation::getInstance()->m_pDieInSimulation = false;
    TrajectorySimulation::getInstance()->m_pIsSimulation = false;

    if (cps_counter) {
        cps_counter->setColor({ 255, 255, 255 });
    }

    // Section 1: Handle Sequencing
    if (logic.sequence_enabled) {
        if (!logic.is_recording() && (!logic.is_playing() || logic.sequence_enabled)) {
            //Hooks::PlayLayer::releaseButton(self, 0, false);
            //Hooks::PlayLayer::releaseButton(self, 0, true);
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

    if (!self->m_isPracticeMode)
        logic.checkpoints.clear();

    if (logic.is_recording() || logic.is_playing())
        logic.handle_checkpoint_data();

    // Section 3: Update Time and Activated Objects
    if (self->m_checkpoints->count() > 0) {
        logic.calculated_xpos = logic.checkpoints.back().calculated_xpos;

        self->m_time = logic.get_latest_offset();
        constexpr auto delete_from = [&](auto& vec, size_t index) {
            vec.erase(vec.begin() + index, vec.end());
        };

        delete_from(logic.activated_objects, logic.checkpoints.back().activated_objects_size);
        delete_from(logic.activated_objects_p2, logic.checkpoints.back().activated_objects_p2_size);

        if (logic.is_recording() || logic.is_playing()) {
            for (const auto& object : logic.activated_objects)
                object->m_bHasBeenActivated = true;
            for (const auto& object : logic.activated_objects_p2)
                object->m_bHasBeenActivatedP2 = true;
        }
    }
    else {
        logic.activated_objects.clear();
        logic.activated_objects_p2.clear();
    }

    logic.recorder.update_song_offset(self);

    // Section 4: Handle Recording
    if (logic.is_recording()) {

        logic.cps_over_percents.clear();
        if (logic.get_inputs().empty()) return ret;

        if (!logic.checkpoints.empty()) {
            logic.set_removed(logic.get_removed() + (logic.get_frame() - logic.get_latest_checkpoint().number));

            if (logic.record_player_1)
                logic.remove_inputs(logic.get_frame(), true);
            if (logic.record_player_2)
                logic.remove_inputs(logic.get_frame(), false);
        }
        else {
            if (!logic.no_overwrite) {
                if (logic.record_player_1)
                    logic.remove_inputs(logic.get_frame(), true);
                if (logic.record_player_2)
                    logic.remove_inputs(logic.get_frame(), false);
            }
            logic.set_removed(0);
            self->m_time = self->timeForXPos(self->m_pPlayer1->getPositionX());
        }

        // Handle inputs for player 1 and player 2 separately
        logic.handlePlayerInputs(self->m_pPlayer1, logic.record_player_1, logic.get_latest_checkpoint().player_1_data.m_isDashing, false);
        if (self->m_level->twoPlayerMode)
            logic.handlePlayerInputs(self->m_pPlayer1, logic.record_player_2, logic.get_latest_checkpoint().player_2_data.m_isDashing, true);
    }


    return ret;
}

void __fastcall Hooks::PlayerObject_ringJump_h(gd::PlayerObject* self, int, gd::GameObject* ring) {
    if (Logic::get().hacks.trajectory && TrajectorySimulation::getInstance()->shouldInterrumpHooksWithPlayer(self))
        return;

    Logic::get().is_over_orb = true;

    PlayerObject_ringJump(self, ring);
    auto& logic = Logic::get();
    if ((logic.is_recording() || logic.is_playing()) && *reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(ring) + 0x2ca)) {
        logic.activated_objects.push_back(ring);
    }
}

void __fastcall Hooks::bumpPlayer_h(gd::GJBaseGameLayer* self, gd::PlayerObject* player, gd::GameObject* object)
{
    if (Logic::get().hacks.trajectory && TrajectorySimulation::getInstance()->shouldInterrumpHooks())
        return;
    bumpPlayer(self, player, object);
}

void __fastcall Hooks::PlayLayer::flipGravity_h(gd::PlayLayer* self, gd::PlayerObject* player, bool idk, bool idk2)
{
    if (Logic::get().hacks.trajectory && TrajectorySimulation::getInstance()->shouldInterrumpHooks())
        return;

    PlayLayer::flipGravity(self, player, idk, idk2);
}

void __fastcall Hooks::PlayLayer::playGravityEffect_h(gd::PlayLayer* self, bool toggle)
{
    if (Logic::get().hacks.trajectory && TrajectorySimulation::getInstance()->shouldInterrumpHooks())
        return;

    PlayLayer::playGravityEffect(self, toggle);
}

void __fastcall Hooks::GameObject_activateObject_h(gd::GameObject* self, int, gd::PlayerObject* player) {
    if (Logic::get().hacks.trajectory && TrajectorySimulation::getInstance()->shouldInterrumpHooksWithPlayer(player))
        return;

    Logic::get().is_over_orb = true;

    GameObject_activateObject(self, player);
    auto& logic = Logic::get();
    if ((logic.is_recording() || logic.is_playing()) && *reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(self) + 0x2ca)) {
        logic.activated_objects.push_back(self);
    }
}

void* __fastcall Hooks::PlayLayer::exitLevel_h(gd::PlayLayer* self, int) {
    auto& logic = Logic::get();

    logic.autoclicker = false;
    logic.checkpoints.clear();
    logic.set_removed(0);
    if (logic.recorder.m_recording)
        logic.recorder.stop();

    return exitLevel(self);
}

int __fastcall Hooks::createCheckpoint_h(gd::PlayLayer* self) {
    auto& logic = Logic::get();

    logic.add_offset(self->m_time);
    CheckpointData checkpointData1 = CheckpointData::create(self->m_pPlayer1);
    CheckpointData checkpointData2 = CheckpointData::create(self->m_pPlayer2);

    std::map<int, ObjectData> childData;
    cocos2d::CCArray* children = self->unk4D4;

    cast_function caster = make_cast<gd::GameObject, CCObject>();
    CCObject* it = NULL;
    CCARRAY_FOREACH(children, it)
    {
        auto child = reinterpret_cast<gd::GameObject*>(caster(it));
        if (child && child->m_nObjectType && child->m_nObjectType != gd::GameObjectType::kGameObjectTypeDecoration)
        {
            ObjectData data;
            data.tag = child->m_uID;
            data.posX = child->getPositionX();
            data.posY = child->getPositionY();
            data.rotX = child->getRotationX();
            data.rotY = child->getRotationY();
            data.velX = child->getSkewX();
            data.velY = child->getSkewY();
            data.speed1 = child->m_unk2F4;
            data.speed2 = child->m_unk2F8;
            data.speed3 = child->m_unk33C;
            data.speed4 = child->m_unk340;
            data.speed5 = child->m_unk390;
            childData[data.tag] = data;
        }
    }

    logic.save_checkpoint({ logic.get_frame(), checkpointData1, checkpointData2, logic.activated_objects.size(), logic.activated_objects_p2.size(), childData, logic.calculated_xpos });

    return createCheckpoint(self);
}

int __fastcall Hooks::removeCheckpoint_h(gd::PlayLayer* self) {
    auto& logic = Logic::get();

    if (!logic.checkpoints.empty()) {
        logic.remove_last_offset();
        logic.remove_last_checkpoint();
    }

    return removeCheckpoint(self);
}

void __fastcall Hooks::addPoint_h(gd::HardStreak* self, CCPoint point) {
    if (Logic::get().hacks.trajectory && TrajectorySimulation::getInstance()->shouldInterrumpHooks())
        return;
    addPoint(self, point);
}


gd::GameObject* __fastcall Hooks::powerOffObject_h(gd::GameObject* self)
{
    if (Logic::get().hacks.trajectory && TrajectorySimulation::getInstance()->shouldInterrumpHooks())
        return self;
    return powerOffObject(self);
}

gd::GameObject* __fastcall Hooks::playShineEffect_h(gd::GameObject* self)
{
    if (Logic::get().hacks.trajectory && TrajectorySimulation::getInstance()->shouldInterrumpHooks())
        return self;
    return playShineEffect(self);
}

void __fastcall Hooks::PlayLayer::onQuit_h(gd::PlayLayer* self)
{
    TrajectorySimulation::getInstance()->onQuitPlayLayer();
    PlayLayer::onQuit(self);
}

void __fastcall Hooks::toggleDartMode_h(gd::PlayerObject* self, bool toggle)
{
    if (gd::GameManager::sharedState()->getPlayLayer() && Logic::get().hacks.trajectory && TrajectorySimulation::getInstance()->m_pPlayer1ForSimulation &&
        TrajectorySimulation::getInstance()->m_pPlayer2ForSimulation)
    {
        TrajectorySimulation::getInstance()->m_pIsSimulation = true;
        toggleDartMode(self == gd::GameManager::sharedState()->getPlayLayer()->m_pPlayer1
            ? TrajectorySimulation::getInstance()->m_pPlayer1ForSimulation
            : TrajectorySimulation::getInstance()->m_pPlayer2ForSimulation,
            toggle);
        TrajectorySimulation::getInstance()->m_pIsSimulation = false;
    }
    toggleDartMode(self, toggle);
}

gd::GameObject* __fastcall Hooks::hasBeenActivatedByPlayer_h(gd::GameObject* self, gd::GameObject* other)
{
    if (Logic::get().hacks.trajectory && TrajectorySimulation::getInstance()->shouldInterrumpHooks())
    {
        if (self->getType() != gd::GameObjectType::kGameObjectTypeSlope &&
            self->getType() != gd::GameObjectType::kGameObjectTypeSolid &&
            self->getType() != gd::GameObjectType::kGameObjectTypeGravityPad &&
            self->getType() != gd::kGameObjectTypePinkJumpPad && self->getType() != gd::kGameObjectTypeRedJumpPad &&
            self->getType() != gd::kGameObjectTypeYellowJumpPad && self->getType() != gd::kGameObjectTypeDashRing &&
            self->getType() != gd::kGameObjectTypeDropRing && self->getType() != gd::kGameObjectTypeGravityDashRing &&
            self->getType() != gd::kGameObjectTypeGravityRing && self->getType() != gd::kGameObjectTypeGreenRing &&
            self->getType() != gd::kGameObjectTypePinkJumpRing && self->getType() != gd::kGameObjectTypeRedJumpRing &&
            self->getType() != gd::kGameObjectTypeYellowJumpRing && self->getType() != gd::kGameObjectTypeSpecial &&
            self->getType() != gd::kGameObjectTypeCollisionObject && self->getType() != gd::kGameObjectTypeHazard &&
            self->getType() != gd::kGameObjectTypeInverseGravityPortal &&
            self->getType() != gd::kGameObjectTypeNormalGravityPortal &&
            self->getType() != gd::kGameObjectTypeTeleportPortal &&
            self->getType() != gd::kGameObjectTypeMiniSizePortal &&
            self->getType() != gd::kGameObjectTypeRegularSizePortal)
        {
            return other;
        }
    }

    return hasBeenActivatedByPlayer(self, other);
}