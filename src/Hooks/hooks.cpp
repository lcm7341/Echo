#include "Hooks.hpp"
#include <chrono>
#include "../Hack/audiopitchHack.hpp"
#include "../Logic/autoclicker.hpp"
#include "../GUI/gui.hpp"
#include "../Logic/speedhack.h"
#include "../Hack/trajectorysimulation.hpp"
#include <filesystem>

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

    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x1907b0), menuLayerInit_h, reinterpret_cast<void**>(&menuLayerInit));

    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x16B7C0), LevelEditorLayer::drawHook,
        reinterpret_cast<void**>(&LevelEditorLayer::draw));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x75660), LevelEditorLayer::exitHook,
        reinterpret_cast<void**>(&LevelEditorLayer::exit));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x1695A0), LevelEditorLayer::onPlaytestHook,
        reinterpret_cast<void**>(&LevelEditorLayer::onPlaytest));

    HOOK(0x20D3C0, PlayLayer::pauseGame);

    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x253D60), PlayLayer::triggerObject_h,
        reinterpret_cast<void**>(&PlayLayer::triggerObject));

    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0xe5d60), powerOffObject_h, reinterpret_cast<void**>(&powerOffObject));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0xeab20), playShineEffect_h, reinterpret_cast<void**>(&playShineEffect));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x1e9a20), incrementJumps_h, reinterpret_cast<void**>(&incrementJumps));
    HOOK(0x10ed50, bumpPlayer);
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0xEF110), hasBeenActivatedByPlayer_h, reinterpret_cast<void**>(&hasBeenActivatedByPlayer));

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

void patch(void* loc, std::vector<std::uint8_t> bytes) { // skidded from replaybot
    auto size = bytes.size();
    DWORD old_prot;
    VirtualProtect(loc, size, PAGE_EXECUTE_READWRITE, &old_prot);
    memcpy(loc, bytes.data(), size);
    VirtualProtect(loc, size, old_prot, &old_prot);
}

void __fastcall Hooks::PlayLayer::pauseGame_h(gd::PlayLayer* self, int, bool idk) { // also skidded, but its the pause/unpause fix for playback
    auto addr = reinterpret_cast<void*>(gd::base + 0x20D43C);
    auto& logic = Logic::get();

    bool should_patch = logic.is_playing() || logic.is_recording();
    if (should_patch)
        patch(addr, { 0x83, 0xC4, 0x04, 0x90, 0x90 });

    pauseGame(self, idk);

    if (should_patch)
        patch(addr, { 0xe8, 0x2f, 0x7b, 0xfe, 0xff });
}

bool g_disable_render = false;
float g_left_over = 0.f;
bool g_single_step = false;

static double steady_seconds() {
    static LARGE_INTEGER frequency = [] { LARGE_INTEGER f; QueryPerformanceFrequency(&f); return f; }();
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return static_cast<double>(counter.QuadPart) / static_cast<double>(frequency.QuadPart);
}



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

bool __fastcall Hooks::menuLayerInit_h(gd::MenuLayer* self) {

    bool ret = menuLayerInit(self);

    /*bool open_help_modal = true;
    std::ifstream settings_file(".echo\\settings\\settings.json");

    if (!settings_file.is_open()) {

        ImGui::Begin("Hi");
        while (true) {
            ImGui::NewFrame();
            ImGui::OpenPopup("Welcome to Echo!");
            if (ImGui::BeginPopupModal("Welcome to Echo!", &open_help_modal, ImGuiWindowFlags_AlwaysAutoResize)) {

                ImGui::Text("Welcome to Echo! This is probably your first time using Echo, so here is a run down of some important information.");

                ImGui::EndPopup();
            }
        }
        ImGui::End();
    }*/

    return ret;
}

float g_left_over_time = 0.f;

void __fastcall Hooks::CCScheduler_update_h(CCScheduler* self, int, float dt) {
    auto& logic = Logic::get();
    auto play_layer = gd::GameManager::sharedState()->getPlayLayer();

    if (!logic.is_recording() && !logic.is_playing()) g_disable_render = false;

    if (logic.frame_advance && !play_layer) logic.frame_advance = false;

    if (logic.recorder.m_recording && play_layer && !play_layer->m_bIsPaused)
        logic.recorder.handle_recording(play_layer, dt);

    if (logic.frame_advance) {
        g_left_over = 0.f;
        return;
    }

    if (GUI::get().change_display_fps) {
        CCDirector::sharedDirector()->setAnimationInterval(1.f / GUI::get().input_fps);
        GUI::get().change_display_fps = false;
    }

    if (logic.recorder.m_recording && !logic.recorder.real_time_rendering) {
        CCDirector::sharedDirector()->setAnimationInterval(1.f / logic.fps);
        dt = 1.f / logic.fps;
        Speedhack::SetSpeed(0.5);
        self->setTimeScale(1.f);
        return CCScheduler_update(self, dt);
    }
    else {
        Speedhack::SetSpeed(1);
    }

    if (!logic.real_time_mode) {
        dt = 1.f / logic.fps;
        Speedhack::SetSpeed(logic.speedhack);
        self->setTimeScale(1.f);
    }
    else {
        Speedhack::SetSpeed(1.f);
        self->setTimeScale(logic.speedhack);
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

        Speedhack::SetSpeed(logic.speedhack);
        self->setTimeScale(1.f);
        dt = 1.f / logic.fps;
        return CCScheduler_update(self, dt);
    }

    auto& audiospeedhack = AudiopitchHack::getInstance();
    bool isEnabled = audiospeedhack.isEnabled();
    if (isEnabled) {
        audiospeedhack.setPitch(self->getTimeScale());
    }
    else {
        audiospeedhack.setPitch(1);
    }

    if (logic.is_recording() || logic.is_playing() && logic.real_time_mode) {

        if (logic.recorder.m_recording) { // prevent screen tearing from lag in the first bits of lvl
            if ((logic.get_frame() / logic.fps) < 1.f || (1.f / dt) < logic.recorder.m_fps || !logic.recorder.real_time_rendering) {
                CCDirector::sharedDirector()->setAnimationInterval(1.f / logic.fps);
                dt = 1.f / logic.fps;
                g_disable_render = false;
                self->setTimeScale(1.f);
                return CCScheduler_update(self, dt);
            }
        }

        float speedhack = self->getTimeScale();
        const float target_dt = 1.f / logic.get_fps() / speedhack;

        g_disable_render = true;

        // min(static_cast<int>(g_left_over / target_dt), 50) <- super fast but i think its inaccurate
        const int max_steps = 150;
        int times = min(round((dt + g_left_over) / target_dt), max_steps);
        if (g_single_step)
            times = 1;
        //const int times = min(static_cast<int>(g_left_over / target_dt), 50);

        const double loop_start = steady_seconds();
        const double loop_budget = 0.012;
        int steps_run = 0;

        auto run_step = [&]() {
            self->setTimeScale(1.f);
            CCScheduler_update(self, logic.frame_dt());
            self->setTimeScale(speedhack);
            steps_run++;
        };

        for (int i = 0; i < times - 1; i++) {
            if (!g_single_step && steady_seconds() - loop_start > loop_budget)
                break;
            run_step();
        }
        if (times > 0) {
            g_disable_render = false;
            run_step();
        }

        if (g_single_step || steps_run < times || times >= max_steps)
            g_left_over = 0.f;
        else
            g_left_over += dt - target_dt * times;
        g_disable_render = false;
    }
    else {
        return CCScheduler_update(self, dt);
    }
}

void Hooks::advance_one_frame() {
    auto& logic = Logic::get();

    logic.frame_advance = false;
    g_single_step = true;
    CCScheduler_update_h(gd::GameManager::sharedState()->getScheduler(), 0, logic.frame_dt());
    g_single_step = false;
    logic.frame_advance = true;
}

void __fastcall Hooks::CCKeyboardDispatcher_dispatchKeyboardMSG_h(CCKeyboardDispatcher* self, int, int key, bool down) {
    auto& logic = Logic::get();

    auto advancing_key = logic.keybinds.GetKeybind("advancing").key;
    bool released_advancing = !down && advancing_key.has_value() && key == advancing_key.value();
    if (released_advancing || !gd::GameManager::sharedState()->getPlayLayer()) {
        logic.start = std::chrono::steady_clock::time_point();
        logic.holding_frame_advance = false;
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

    if (drawer) {
        if (drawer->getParent())
            drawer->removeFromParentAndCleanup(false);
        self->m_pObjectLayer->addChild(drawer, 32);
    }


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

    return Hooks::PauseLayer::init(self);
}

static void set_label_text(cocos2d::CCLabelBMFont* label, const char* text) {
    if (strcmp(label->getString(), text) != 0)
        label->setString(text);
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

void __fastcall Hooks::LevelEditorLayer::drawHook(gd::LevelEditorLayer* self, void*)
{
    auto& logic = Logic::get();
    if (logic.is_recording()) logic.toggle_recording();
    HitboxNode::getInstance()->clear();
    LevelEditorLayer::draw(self);

    if (logic.hacks.showHitboxes)
    {
        HitboxNode::getInstance()->setVisible(true);
        if (self->m_pPlayer1)
        {
            HitboxNode::getInstance()->addToPlayer1Queue(self->m_pPlayer1->getObjectRect());
            HitboxNode::getInstance()->drawForPlayer1(self->m_pPlayer1);
        }
        if (self->m_pPlayer2)
        {
            HitboxNode::getInstance()->addToPlayer2Queue(self->m_pPlayer2->getObjectRect());
            HitboxNode::getInstance()->drawForPlayer2(self->m_pPlayer2);
        }

        if (self->m_pObjectLayer)
        {
            auto layer = static_cast<CCLayer*>(self->getChildren()->objectAtIndex(2));
            float xp = -layer->getPositionX() / layer->getScale();
            for (int s = self->sectionForPos(xp) - (5 / layer->getScale()); s < self->sectionForPos(xp) + (6 / layer->getScale()); ++s)
            {
                if (s < 0)
                    continue;
                if (s >= self->m_sectionObjects->count())
                    break;
                auto section = static_cast<CCArray*>(self->m_sectionObjects->objectAtIndex(s));
                for (size_t i = 0; i < section->count(); ++i)
                {
                    auto obj = static_cast<gd::GameObject*>(section->objectAtIndex(i));

                    if (obj->m_nObjectID != 749 && obj->getObjType() == gd::GameObjectType::kGameObjectTypeDecoration)
                        continue;
                    if (!obj->m_bActive)
                        continue;

                    HitboxNode::getInstance()->drawObjectWithRotation(obj);
                }
            }
        }
    }
}

void __fastcall Hooks::LevelEditorLayer::onPlaytestHook(gd::LevelEditorLayer* self, void*)
{
    HitboxNode::getInstance()->clearQueue();
    LevelEditorLayer::onPlaytest(self);
}

void __fastcall Hooks::PlayLayer::lightningFlash_h(gd::PlayLayer* self, void* edx, CCPoint p, _ccColor3B c)
{
    auto& logic = Logic::get();

    if (!logic.hacks.layoutMode)
        PlayLayer::lightningFlash(self, p, c);
}

void __fastcall Hooks::LevelEditorLayer::exitHook(CCLayer* self, void*, CCObject* sender)
{
    LevelEditorLayer::exit(self, sender);
    HitboxNode::getInstance()->clearQueue();
}

void __fastcall Hooks::PlayLayer::triggerObject_h(gd::EffectGameObject* self, void*, gd::GJBaseGameLayer* idk)
{
    auto& logic = Logic::get();
    if (logic.hacks.trajectory && TrajectorySimulation::getInstance()->shouldInterrumpHooks())
        return;
    auto id = self->m_nObjectID;
    if (logic.hacks.layoutMode && (id == 899 || id == 1006 || id == 1007 || id == 105 || id == 29 || id == 56 || id == 915 ||
        id == 30 || id == 58))
        return;
    PlayLayer::triggerObject(self, idk);
}


void __fastcall Hooks::PlayLayer::update_h(gd::PlayLayer* self, int, float dt) {
    auto& logic = Logic::get();

    static int offset = rand();

    const bool timing_locked = logic.is_recording() || logic.is_playing();
    if (timing_locked)
        dt = logic.frame_dt();
    const double time_before_update = self->m_time;

    logic.clickbot_now = self->m_time;
    bool changeBlockColor = false;
    auto p = self->getChildren()->objectAtIndex(0);
    auto xp = self->m_pPlayer1->getPositionX();

    static bool portal_flash_disabled = false;
    if (logic.hacks.layoutMode != portal_flash_disabled) {
        portal_flash_disabled = logic.hacks.layoutMode;
        if (portal_flash_disabled)
            patch(reinterpret_cast<void*>(gd::base + 0x1FFE70), { 0xC2, 0x20, 0x00 });
        else
            patch(reinterpret_cast<void*>(gd::base + 0x1FFE70), { 0x55, 0x8B, 0xEC });
    }

    if (logic.disable_shakes) {
        self->m_currentShakeStrength = 0;
        self->m_currentShakeInterval = 0;
        self->m_isCameraShaking = false;
        self->m_lastShakeTime = 0;
    }

    // layout mode Shit ( it doesnt work with 0 opacity objects FSR.)
    /*for (int s = self->sectionForPos(xp) - 5; s < self->sectionForPos(xp) + 6; ++s)
    {
        if (s < 0)
            continue;
        if (s >= self->m_sectionObjects->count())
            break;
        auto section = static_cast<CCArray*>(self->m_sectionObjects->objectAtIndex(s));
        for (size_t i = 0; i < section->count(); ++i)
        {
            auto o = static_cast<gd::GameObject*>(section->objectAtIndex(i));

            if (o->getType() == gd::GameObjectType::kGameObjectTypeDecoration && o->isVisible() &&
                (o->m_nObjectID != 44 && o->m_nObjectID != 749 && o->m_nObjectID != 12 && o->m_nObjectID != 38 &&
                    o->m_nObjectID != 47 && o->m_nObjectID != 111 && o->m_nObjectID != 8 && o->m_nObjectID != 13 &&
                    o->m_nObjectID != 660 && o->m_nObjectID != 745 && o->m_nObjectID != 101 && o->m_nObjectID != 99 &&
                    o->m_nObjectID != 1331))
            {
                o->setVisible(false);
            }
            else {
                o->setVisible(true);
                o->setOpacity(255);
            }
        }
        self->m_pPlayer1->setVisible(true);
        self->m_pPlayer2->setVisible(true);
        self->m_pPlayer1->setOpacity(255);
        self->m_pPlayer2->setOpacity(255);
    }*/

    if (self->m_isDead && logic.recorder.m_recording) {
        self->m_time += dt;
    }

    if (self->m_hasCompletedLevel) {
        if (logic.is_recording()) logic.toggle_recording();
    }

    logic.recorder.m_song_start_offset = self->m_pLevelSettings->m_songStartOffset;

    if (!g_disable_render) {
        // Check if the frame counter label exists
        auto frame_counter = (cocos2d::CCLabelBMFont*)self->getChildByTag(FRAME_LABEL_ID);
        if (frame_counter) {
            if (logic.show_frame) {
                // Update the frame counter label with the current frame number
                char out[32];
                int frame = logic.get_frame() + 1;
                sprintf_s(out, "Frame: %i", frame);
                frame_counter->setAnchorPoint({ 0, 0.5 });
                frame_counter->setPosition(logic.frame_counter_x, logic.frame_counter_y); // Adjusted the x-position calculation
                set_label_text(frame_counter, out);
                frame_counter->setOpacity(logic.frame_counter_opacity);
                frame_counter->setScale(logic.frame_counter_scale);

            }
            else {
                // Remove and release the frame counter label if show_frame is false
                frame_counter->removeFromParent();
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
            if (logic.is_recording() && logic.show_recording) {
                // Update the frame counter label with the current frame number
                char out[64];
                int holds_count = 0;
                int releases_count = 0;
                for (auto& input : logic.inputs) {
                    if (input.pressingDown) holds_count++;
                    else releases_count++;
                }
                sprintf_s(out, "Recording: %i/%i", holds_count, releases_count);
                set_label_text(recording_label, out);
                recording_label->setPosition(logic.recording_label_x, logic.recording_label_y);
                recording_label->setOpacity(logic.recording_label_opacity);
                recording_label->setScale(logic.recording_label_scale);
                recording_label->setAnchorPoint({ 0, 0.5 });
            }
            else {
                // Remove and release the frame counter label if show_frame is false
                recording_label->removeFromParent();
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
                char out[40];
                unsigned cps_p1 = logic.count_presses_in_last_second(false);
                unsigned cps_p2 = logic.count_presses_in_last_second(true);
                sprintf_s(out, "CPS: %u/%u", cps_p1, cps_p2);
                set_label_text(cps_counter, out);

                if (logic.currently_pressing) {
                    cps_counter->setColor({ 0, 255, 77 });
                }
                else if (logic.current_cps > logic.max_cps) {
                    cps_counter->setColor({ 255, 0, 0 });
                }
                else if (logic.over_max_cps) {
                    cps_counter->setColor({ 255, 72, 0 });
                }
                else {
                    cps_counter->setColor({ 255, 255, 255 });
                }
                cps_counter->setAnchorPoint({ 0, 0.5 });
                cps_counter->setPosition(logic.cps_counter_x, logic.cps_counter_y);
                cps_counter->setOpacity(logic.cps_counter_opacity);
                cps_counter->setScale(logic.cps_counter_scale);
            }

            else {
                cps_counter->removeFromParent();
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
                char out[64];
                float percent = min(100.f, (self->m_pPlayer1->getPositionX() / self->m_endPortal->getPositionX()) * 100.f);
                if (logic.percent_accuracy > 0)
                    sprintf_s(out, "%.*f%%", min(logic.percent_accuracy, 30), percent);
                else
                    sprintf_s(out, "%i%%", static_cast<int>(percent));
                set_label_text(percent_label, out);
                percent_label->setAnchorPoint({ 0, 0.5 });
                percent_label->setPosition(logic.percent_counter_x, logic.percent_counter_y);
                percent_label->setScale(logic.percent_scale);
                percent_label->setOpacity(logic.percent_opacity);
            }

            else {
                percent_label->removeFromParent();
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
                char out[40];
                std::string str = formatTime(self->m_time);
                sprintf_s(out, "%s", str.c_str());
                set_label_text(time_label, out);
                time_label->setAnchorPoint({ 0, 0.5 });
                time_label->setPosition(logic.time_counter_x, logic.time_counter_y);
                time_label->setScale(logic.time_scale);
                time_label->setOpacity(logic.time_opacity);
            }

            else {
                time_label->removeFromParent();
            }
        }
        else if (logic.show_time) {
            auto time_label2 = cocos2d::CCLabelBMFont::create("", "bigFont.fnt");
            time_label2->setPosition(logic.time_counter_x, logic.time_counter_y);
            time_label2->setScale(logic.time_scale);
            time_label2->setOpacity(logic.percent_opacity);

            self->addChild(time_label2, 999, TIME_LABEL_ID);
        }
    }

    if (drawer && !logic.hacks.trajectory) drawer->clear();

    if (logic.hacks.layoutMode)
    {
        if (logic.get_frame() > 1) {
            auto p = self->getChildren()->objectAtIndex(0);
            auto sprite = static_cast<CCSprite*>(p);
            ccColor3B color = { (GLubyte)(logic.hacks.backgroundColor[0] * 255), (GLubyte)(logic.hacks.backgroundColor[1] * 255),
                               (GLubyte)(logic.hacks.backgroundColor[2] * 255) };
            sprite->setColor(color);
        }
    }
    if (timing_locked)
        self->m_totalTime = static_cast<float>(logic.get_frame()) * dt;

    logic.begin_update(time_before_update);
    update(self, dt);
    logic.end_update();

    if (timing_locked) {
        if (self->m_time != time_before_update)
            logic.advance_frame();
    }
    else {
        logic.sync_frame_to_time();
    }

    if (Logic::get().hacks.trajectory)
        TrajectorySimulation::getInstance()->processMainSimulation(dt);

    if (logic.hacks.layoutMode)
    {
        bool changeBlockColor = false;
        if (logic.get_frame() > 1) {
            auto p = self->getChildren()->objectAtIndex(0);
            auto sprite = static_cast<CCSprite*>(p);
            ccColor3B color = { (GLubyte)(logic.hacks.backgroundColor[0] * 255), (GLubyte)(logic.hacks.backgroundColor[1] * 255),
                               (GLubyte)(logic.hacks.backgroundColor[2] * 255) };
            sprite->setColor(color);
        }

        if (logic.hacks.blocksColor[0] != logic.hacks.backgroundColor[0] || logic.hacks.blocksColor[1] != logic.hacks.backgroundColor[1] ||
            logic.hacks.blocksColor[2] != logic.hacks.backgroundColor[2])
        {
            changeBlockColor = true;
        }

        self->m_currentShakeStrength = 0;
        self->m_currentShakeInterval = 0;
        self->m_isCameraShaking = false;
        self->m_lastShakeTime = 0;

        if (!g_disable_render)
        for (int s = self->sectionForPos(xp) - 5; s < self->sectionForPos(xp) + 6; ++s)
        {
            if (s < 0)
                continue;
            if (s >= self->m_sectionObjects->count())
                break;
            auto section = static_cast<CCArray*>(self->m_sectionObjects->objectAtIndex(s));
            for (size_t i = 0; i < section->count(); ++i)
            {
                auto o = static_cast<gd::GameObject*>(section->objectAtIndex(i));

                auto block = static_cast<gd::GameObject*>(o);
                ccColor3B blockColor = { (GLubyte)(logic.hacks.blocksColor[0] * 255),
                                        (GLubyte)(logic.hacks.blocksColor[1] * 255),
                                        (GLubyte)(logic.hacks.blocksColor[2] * 255) };
                block->setColor(blockColor);


                if (o->getType() == gd::GameObjectType::kGameObjectTypeDecoration &&
                    (o->m_nObjectID != 44 && o->m_nObjectID != 749 && o->m_nObjectID != 12 && o->m_nObjectID != 38 &&
                        o->m_nObjectID != 47 && o->m_nObjectID != 111 && o->m_nObjectID != 8 && o->m_nObjectID != 13 &&
                        o->m_nObjectID != 660 && o->m_nObjectID != 745 && o->m_nObjectID != 101 && o->m_nObjectID != 99 &&
                        o->m_nObjectID != 1331))
                {
                    o->setVisible(false);
                }
            }
        }
    }

    if (drawer && logic.hacks.showHitboxes) {
        if (logic.hacks.hitboxTrail) {
            if (self->m_pPlayer1)
                drawer->addToPlayer1Queue(self->m_pPlayer1->getObjectRect());
            if (self->m_pPlayer2)
                drawer->addToPlayer2Queue(self->m_pPlayer2->getObjectRect());
        }
        else {
            drawer->clearQueue();
        }
    }

    if (drawer && logic.hacks.showHitboxes && !g_disable_render) {
        drawer->setVisible(true);
        drawer->clear();

        if (self->m_pPlayer1)
            drawer->drawForPlayer1(self->m_pPlayer1);
        if (self->m_pPlayer2)
            drawer->drawForPlayer2(self->m_pPlayer2);

        const int player_section = self->sectionForPos(self->m_pPlayer1->getPositionX());
        const int section_count = self->m_sectionObjects->count();

        for (int s = player_section - 5; s < player_section + 6; ++s)
        {
            if (s < 0)
                continue;
            if (s >= section_count)
                break;
            auto section = static_cast<CCArray*>(self->m_sectionObjects->objectAtIndex(s));
            const unsigned object_count = section->count();
            for (unsigned i = 0; i < object_count; ++i)
            {
                auto obj = static_cast<gd::GameObject*>(section->objectAtIndex(i));

                if (!obj->m_bActive)
                    continue;
                if (obj->m_nObjectID != 749 && obj->getType() == gd::GameObjectType::kGameObjectTypeDecoration)
                    continue;

                drawer->drawForObject(obj);
            }
        }

    }

    if (!self->getActionByTag(0xE)) {
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

        if (logic.player_x_positions.empty()) logic.player_x_positions.push_back(self->m_pPlayer1->m_position.x);
        else logic.player_x_positions.back() = self->m_pPlayer1->m_position.x;
    }
    else {
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

                    logic.calculated_xpos = self->m_pPlayer1->getPositionX();
                    logic.calculated_frame = round(logic.get_frame() + (self->m_pPlayer1->getPositionX() - logic.calculated_xpos));//round((logic.calculated_xpos / (logic.player_speed * logic.player_acceleration) * (1.f / logic.fps)) * logic.fps); // doesnt work when changing speeds, fucjk

                    logic.previous_xpos = logic.calculated_xpos;
                }
            }
        }

        if (logic.player_x_positions.empty()) logic.player_x_positions.push_back(self->m_pPlayer1->m_position.x);
        else logic.player_x_positions.back() = self->m_pPlayer1->m_position.x;
    }

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

    if (!PLAYLAYER) return pushButton(self, idk, button);

    if (!logic.is_playing() && !logic.is_recording()) {
        logic.live_inputs.push_back({ logic.get_frame(), true, !button, self->getPositionY(), self->getPositionX(), self->getRotation(), 0.f, 0.f });
    }

    if (logic.is_playing() && !logic.clickbot_enabled) {
        if (logic.playback_clicking)
            return pushButton(self, idk, button);
        else {
            if (logic.ignore_actions_at_playback) {
                if (logic.swap_player_input) button = !button;
                if (!self->m_level->twoPlayerMode) {
                    if (button && !logic.record_player_2)
                        return 0;
                    if (!button && !logic.record_player_1)
                        return 0;
                    if (logic.play_player_1 && logic.play_player_2)
                        return 0;
                }
                else {
                    if (button && !logic.record_player_1)
                        return 0;
                    if (!button && !logic.record_player_2)
                        return 0;
                    if (logic.play_player_1 && logic.play_player_2)
                        return 0;
                }
                if (logic.swap_player_input) button = !button;
            }
        }
    }

    if (logic.clickbot_enabled && (!logic.is_playing() || logic.playback_clicking)) {
        if (!Clickbot::inited)
        {
            try {
                FMOD::System_Create(&Clickbot::system);
                Clickbot::system->init(1024 * 2, FMOD_INIT_NORMAL, nullptr);
            }
            catch (std::exception& ex) {
                printf("Clickbot error: %s\n", ex.what());

                std::ofstream logfile("error.log");
                if (logfile.is_open())
                {
                    logfile << "Clickbot error: " << ex.what() << std::endl;
                    logfile.close();
                }
            }
            Clickbot::inited = true;
        }

        logic.cycleTime = logic.clickbot_now - logic.clickbot_start;
        //logic.clickbot_start = self->m_time;
        bool oldButton = button;
        if (!self->m_level->twoPlayerMode) {
            button = true;
        }
        logic.cycleTime = logic.clickbot_now - logic.clickbot_start;

        bool micros = button ? logic.player_1_micros : logic.player_2_micros;
        bool softs = button ? logic.player_1_softs : logic.player_2_softs;
        bool hards = button ? logic.player_1_hards : logic.player_2_hards;

        if (logic.cycleTime <= (button ? logic.player_1_micros_time / 1000.f : logic.player_2_micros_time / 1000.f) && micros && Clickbot::hasSounds(button ? logic.player_1_path : logic.player_2_path, "micro_clicks"))
        {
            std::string path = Clickbot::pickRandomFile(button ? logic.player_1_path : logic.player_2_path, "micro_clicks");
            logic.clickbot_start = self->m_time;
            if (!path.empty()) {
                (button ? Clickbot::clickSound : Clickbot::clickSound2) = Clickbot::cachedSound(Clickbot::system, path);
                if (button) {
                    Clickbot::system->playSound(Clickbot::clickSound, nullptr, true, &Clickbot::releaseChannel);
                    if (Clickbot::releaseChannel) Clickbot::releaseChannel->setVolume((float)(logic.player_1_micros_volume * 5 * logic.clickbot_volume_multiplier));
                }
                else {
                    Clickbot::system->playSound(Clickbot::clickSound2, nullptr, true, &Clickbot::releaseChannel2);
                    if (Clickbot::releaseChannel2) Clickbot::releaseChannel2->setVolume((float)(logic.player_2_micros_volume * 5 * logic.clickbot_volume_multiplier));
                }
            }
        }
        else if (logic.cycleTime < (button ? logic.player_1_softs_time / 1000.f : logic.player_2_softs_time / 1000.f) && softs && Clickbot::hasSounds(button ? logic.player_1_path : logic.player_2_path, "soft_clicks"))
        {
            std::string path = Clickbot::pickRandomFile(button ? logic.player_1_path : logic.player_2_path, "soft_clicks");
            logic.clickbot_start = self->m_time;
            if (!path.empty()) {
                (button ? Clickbot::releaseSound : Clickbot::releaseSound2) = Clickbot::cachedSound(Clickbot::system, path);
                if (button) {
                    Clickbot::system->playSound(Clickbot::releaseSound, nullptr, true, &Clickbot::releaseChannel);
                    if (Clickbot::releaseChannel) Clickbot::releaseChannel->setVolume((float)(logic.player_1_softs_volume * 5 * logic.clickbot_volume_multiplier));
                }
                else {
                    Clickbot::system->playSound(Clickbot::releaseSound2, nullptr, true, &Clickbot::releaseChannel2);
                    if (Clickbot::releaseChannel2) Clickbot::releaseChannel2->setVolume((float)(logic.player_2_softs_volume * 5 * logic.clickbot_volume_multiplier));
                }
            }
        }
        else if (logic.cycleTime > (button ? logic.player_1_hards_time : logic.player_2_hards_time) && hards && Clickbot::hasSounds(button ? logic.player_1_path : logic.player_2_path, "hard_clicks")) {
            std::string path = Clickbot::pickRandomFile(button ? logic.player_1_path : logic.player_2_path, "hard_clicks");
            logic.clickbot_start = self->m_time;
            if (!path.empty()) {
                (button ? Clickbot::releaseSound : Clickbot::releaseSound2) = Clickbot::cachedSound(Clickbot::system, path);

                if (button) {
                    Clickbot::system->playSound(Clickbot::releaseSound, nullptr, true, &Clickbot::releaseChannel);
                    if (Clickbot::releaseChannel) Clickbot::releaseChannel->setVolume((float)(logic.player_1_hards_volume * 5 * logic.clickbot_volume_multiplier));
                }
                else {
                    Clickbot::system->playSound(Clickbot::releaseSound2, nullptr, true, &Clickbot::releaseChannel2);
                    if (Clickbot::releaseChannel2) Clickbot::releaseChannel2->setVolume((float)(logic.player_2_hards_volume * 5 * logic.clickbot_volume_multiplier));
                }
            }
        }
        else {
            std::string path = Clickbot::pickRandomFile(button ? logic.player_1_path : logic.player_2_path, "clicks");
            logic.clickbot_start = self->m_time;
            if (!path.empty()) {
                (button ? Clickbot::releaseSound : Clickbot::releaseSound2) = Clickbot::cachedSound(Clickbot::system, path);
                if (button) {
                    Clickbot::system->playSound(Clickbot::releaseSound, nullptr, true, &Clickbot::releaseChannel);
                    if (Clickbot::releaseChannel) Clickbot::releaseChannel->setVolume((float)(logic.player_1_volume * 5 * logic.clickbot_volume_multiplier));
                }
                else {
                    Clickbot::system->playSound(Clickbot::releaseSound2, nullptr, true, &Clickbot::releaseChannel2);
                    if (Clickbot::releaseChannel2) Clickbot::releaseChannel2->setVolume((float)(logic.player_2_volume * 5 * logic.clickbot_volume_multiplier));
                }
            }
        }

        if (button) {
            if (Clickbot::releaseChannel) Clickbot::releaseChannel->setPaused(false);
        }
        else {
            if (Clickbot::releaseChannel2) Clickbot::releaseChannel2->setPaused(false);
        }
        Clickbot::system->update();
        button = oldButton;
        if (logic.is_playing() && logic.playback_clicking)
            return pushButton(self, idk, button);
        else if (logic.is_playing()) {
            if (logic.ignore_actions_at_playback) {
                if (!self->m_level->twoPlayerMode) {
                    if (button && !logic.record_player_2)
                        return 0;
                    if (!button && !logic.record_player_1)
                        return 0;
                    if (logic.play_player_1 && logic.play_player_2)
                        return 0;
                }
                else {
                    if (button && !logic.record_player_1)
                        return 0;
                    if (!button && !logic.record_player_2)
                        return 0;
                    if (logic.play_player_1 && logic.play_player_2)
                        return 0;
                }
            }
        }
    }

    if (logic.playback_clicking) return 0;

    if (logic.is_playing()) {
        if (logic.ignore_actions_at_playback) {
            if (logic.swap_player_input) button = !button;
            if (!self->m_level->twoPlayerMode) {
                if (button && !logic.record_player_2)
                    return 0;
                if (!button && !logic.record_player_1)
                    return 0;
                if (logic.play_player_1 && logic.play_player_2)
                    return 0;
            }
            else {
                if (button && !logic.record_player_1)
                    return 0;
                if (!button && !logic.record_player_2)
                    return 0;
                if (logic.play_player_1 && logic.play_player_2)
                    return 0;
            }
            if (logic.swap_player_input) button = !button;
        }
    }

    if (button && self->m_pPlayer1->m_isDashing) {
        return 0;
    }

    if (!button && self->m_pPlayer2->m_isDashing) {
        return 0;
    }

    logic.currently_pressing = true;
    if (logic.click_both_players) {
        logic.record_input(true, !button);
        pushButton(self, idk, !button);
    }

    if (logic.swap_player_input) button = !button;

    if (logic.is_recording()) {
        if (!self->m_level->twoPlayerMode) {
            if (button && !logic.record_player_2)
                return 0;
            if (!button && !logic.record_player_1)
                return 0;
        }
        else {
            if (button && !logic.record_player_1)
                return 0;
            if (!button && !logic.record_player_2)
                return 0;
        }
    }

    if (button ? logic.click_inverse_p2 : logic.click_inverse_p1)
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

    if (!PLAYLAYER) return releaseButton(self, idk, button);

    logic.currently_pressing = false;

    if (!logic.clickbot_enabled) {
        if (logic.is_playing() && logic.playback_releasing)
            return releaseButton(self, idk, button);
        else if (logic.is_playing()) {
            if (logic.ignore_actions_at_playback) {
                if (logic.swap_player_input) button = !button;
                if (!self->m_level->twoPlayerMode) {
                    if (button && !logic.record_player_2)
                        return 0;
                    if (!button && !logic.record_player_1)
                        return 0;
                    if (logic.play_player_1 && logic.play_player_2)
                        return 0;
                }
                else {
                    if (button && !logic.record_player_1)
                        return 0;
                    if (!button && !logic.record_player_2)
                        return 0;
                    if (logic.play_player_1 && logic.play_player_2)
                        return 0;
                }
                if (logic.swap_player_input) button = !button;
            }
        }
    }

    if ((logic.clickbot_enabled && !logic.is_playing()) || (logic.clickbot_enabled && logic.playback_releasing)) {
        if (!Clickbot::inited)
        {
            FMOD::System_Create(&Clickbot::system);
            Clickbot::system->init(1024 * 2, FMOD_INIT_NORMAL, nullptr);
            Clickbot::inited = true;
        }

        bool oldButton = button;
        if (!self->m_level->twoPlayerMode) {
            button = true;
        }
        logic.cycleTime = logic.clickbot_now - logic.clickbot_start;

        bool micros = button ? logic.player_1_micros : logic.player_2_micros;
        bool softs = button ? logic.player_1_softs : logic.player_2_softs;
        bool hards = button ? logic.player_1_hards : logic.player_2_hards;

        if (logic.cycleTime <= (button ? logic.player_1_micros_time / 1000.f : logic.player_2_micros_time / 1000.f) && micros && Clickbot::hasSounds(button ? logic.player_1_path : logic.player_2_path, "micro_releases"))
        {
            std::string path = Clickbot::pickRandomFile(button ? logic.player_1_path : logic.player_2_path, "micro_releases");
            logic.clickbot_start = self->m_time;
            if (!path.empty()) {
                (button ? Clickbot::clickSound : Clickbot::clickSound2) = Clickbot::cachedSound(Clickbot::system, path);
                if (button) {
                    Clickbot::system->playSound(Clickbot::clickSound, nullptr, true, &Clickbot::releaseChannel);
                    if (Clickbot::releaseChannel) Clickbot::releaseChannel->setVolume((float)(logic.player_1_micros_volume * 5 * logic.clickbot_volume_multiplier));
                }
                else {
                    Clickbot::system->playSound(Clickbot::clickSound2, nullptr, true, &Clickbot::releaseChannel2);
                    if (Clickbot::releaseChannel2) Clickbot::releaseChannel2->setVolume((float)(logic.player_2_micros_volume * 5 * logic.clickbot_volume_multiplier));
                }
            }
        }
        else if (logic.cycleTime < (button ? logic.player_1_softs_time / 1000.f : logic.player_2_softs_time / 1000.f) && softs && Clickbot::hasSounds(button ? logic.player_1_path : logic.player_2_path, "soft_releases"))
        {
            std::string path = Clickbot::pickRandomFile(button ? logic.player_1_path : logic.player_2_path, "soft_releases");
            logic.clickbot_start = self->m_time;
            if (!path.empty()) {
                (button ? Clickbot::releaseSound : Clickbot::releaseSound2) = Clickbot::cachedSound(Clickbot::system, path);
                if (button) {
                    Clickbot::system->playSound(Clickbot::releaseSound, nullptr, true, &Clickbot::releaseChannel);
                    if (Clickbot::releaseChannel) Clickbot::releaseChannel->setVolume((float)(logic.player_1_softs_volume * 5 * logic.clickbot_volume_multiplier));
                }
                else {
                    Clickbot::system->playSound(Clickbot::releaseSound2, nullptr, true, &Clickbot::releaseChannel2);
                    if (Clickbot::releaseChannel2) Clickbot::releaseChannel2->setVolume((float)(logic.player_2_softs_volume * 5 * logic.clickbot_volume_multiplier));
                }
            }
        }
        else if (logic.cycleTime > (button ? logic.player_1_hards_time : logic.player_2_hards_time) && hards && Clickbot::hasSounds(button ? logic.player_1_path : logic.player_2_path, "hard_releases")) {
            std::string path = Clickbot::pickRandomFile(button ? logic.player_1_path : logic.player_2_path, "hard_releases");
            logic.clickbot_start = self->m_time;
            if (!path.empty()) {
                (button ? Clickbot::releaseSound : Clickbot::releaseSound2) = Clickbot::cachedSound(Clickbot::system, path);

                if (button) {
                    Clickbot::system->playSound(Clickbot::releaseSound, nullptr, true, &Clickbot::releaseChannel);
                    if (Clickbot::releaseChannel) Clickbot::releaseChannel->setVolume((float)(logic.player_1_hards_volume * 5 * logic.clickbot_volume_multiplier));
                }
                else {
                    Clickbot::system->playSound(Clickbot::releaseSound2, nullptr, true, &Clickbot::releaseChannel2);
                    if (Clickbot::releaseChannel2) Clickbot::releaseChannel2->setVolume((float)(logic.player_2_hards_volume * 5 * logic.clickbot_volume_multiplier));
                }
            }
        }
        else {
            std::string path = Clickbot::pickRandomFile(button ? logic.player_1_path : logic.player_2_path, "releases");
            logic.clickbot_start = self->m_time;
            if (!path.empty()) {
                (button ? Clickbot::releaseSound : Clickbot::releaseSound2) = Clickbot::cachedSound(Clickbot::system, path);
                if (button) {
                    Clickbot::system->playSound(Clickbot::releaseSound, nullptr, true, &Clickbot::releaseChannel);
                    if (Clickbot::releaseChannel) Clickbot::releaseChannel->setVolume((float)(logic.player_1_volume * 5 * logic.clickbot_volume_multiplier));
                }
                else {
                    Clickbot::system->playSound(Clickbot::releaseSound2, nullptr, true, &Clickbot::releaseChannel2);
                    if (Clickbot::releaseChannel2) Clickbot::releaseChannel2->setVolume((float)(logic.player_2_volume * 5 * logic.clickbot_volume_multiplier));
                }
            }
        }

        if (button) {
            if (Clickbot::releaseChannel) Clickbot::releaseChannel->setPaused(false);
        }
        else {
            if (Clickbot::releaseChannel2) Clickbot::releaseChannel2->setPaused(false);
        }
        Clickbot::system->update();
        button = oldButton;
        if (logic.is_playing() && logic.playback_releasing)
            return releaseButton(self, idk, button);
        else if (logic.is_playing()) {
            if (logic.ignore_actions_at_playback) {
                if (!self->m_level->twoPlayerMode) {
                    if (button && !logic.record_player_2)
                        return 0;
                    if (!button && !logic.record_player_1)
                        return 0;
                    if (logic.play_player_1 && logic.play_player_2)
                        return 0;
                }
                else {
                    if (button && !logic.record_player_1)
                        return 0;
                    if (!button && !logic.record_player_2)
                        return 0;
                    if (logic.play_player_1 && logic.play_player_2)
                        return 0;
                }
            }
        }
    }

    if (logic.playback_releasing) return 0;

    if (logic.is_playing()) {
        if (logic.ignore_actions_at_playback) {
            if (logic.swap_player_input) button = !button;
            if (!self->m_level->twoPlayerMode) {
                if (button && !logic.record_player_2)
                    return 0;
                if (!button && !logic.record_player_1)
                    return 0;
                if (logic.play_player_1 && logic.play_player_2)
                    return 0;
            }
            else {
                if (button && !logic.record_player_1)
                    return 0;
                if (!button && !logic.record_player_2)
                    return 0;
                if (logic.play_player_1 && logic.play_player_2)
                    return 0;
            }
            if (logic.swap_player_input) button = !button;
        }
    }

    if (logic.click_both_players && self->m_level->twoPlayerMode) {
        logic.record_input(false, !button);
        releaseButton(self, idk, !button);
    }

    if (logic.swap_player_input) button = !button;

    if (logic.is_recording()) {
        if (!self->m_level->twoPlayerMode) {
            if (button && !logic.record_player_2)
                return 0;
            if (!button && !logic.record_player_1)
                return 0;
        }
        else {
            if (button && !logic.record_player_1)
                return 0;
            if (!button && !logic.record_player_2)
                return 0;
        }
    }

    if (button ? logic.click_inverse_p2 : logic.click_inverse_p1)
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
    if (Logic::get().hacks.layoutMode && (id == 899 || id == 1006 || id == 1007 || id == 105 || id == 29 || id == 56 || id == 915 ||
        id == 30 || id == 58))
        return;

    triggerObject(self, layer);

    /*auto& logic = Logic::get();
    if ((logic.is_recording() || logic.is_playing()) && *reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(self) + 0x2ca)) {
        if (self->m_bHasBeenActivated)
            logic.activated_objects.push_back(self);
        if (self->m_bHasBeenActivatedP2)
            logic.activated_objects_p2.push_back(self);
    }*/
}

int __fastcall Hooks::PlayLayer::resetLevel_h(gd::PlayLayer* self, int idk) {
    int ret = resetLevel(self); // calling the original function
    auto& logic = Logic::get();

    logic.currently_pressing = false;

    logic.clickbot_now = self->m_time;
    logic.clickbot_start = self->m_time;

    TrajectorySimulation::getInstance()->m_pDieInSimulation = false;
    TrajectorySimulation::getInstance()->m_pIsSimulation = false;

    if (drawer)
    {
        drawer->clearQueue();
        drawer->m_isMini1 = self->m_pLevelSettings->m_startMini;
        drawer->m_isMini2 = self->m_pLevelSettings->m_startMini;
    }

    const bool from_checkpoint = self->m_checkpoints->count() > 0 && !logic.checkpoints.empty();
    const bool live_hold_p1 = self->m_pPlayer1 && self->m_pPlayer1->m_isHolding;
    const bool live_hold_p2 = self->m_pPlayer2 && self->m_pPlayer2->m_isHolding;
    if (from_checkpoint)
        logic.set_frame(logic.checkpoints.back().number);
    else
        logic.sync_frame_to_time();
    if (logic.is_in_update())
        logic.begin_update(self->m_time);

    auto frame = logic.get_frame();

    /*if (logic.is_recording()) {
        auto it = std::remove_if(logic.shakes.begin(), logic.shakes.end(),
            [frame](const ShakeFrame& input) {
                return input.number >= frame;
            });
        logic.shakes.erase(it, logic.shakes.end());
    }

    if (logic.is_playing()) {
        logic.shakes_pos = 0;
    }*/

    if (logic.is_recording()) {
        logic.framerates.clear();
    }

    if (logic.is_playing()) {
        logic.framerates_pos = 0;
    }

    if (logic.is_playing() && !logic.inputs.empty()) {
        if (logic.checkpoints.empty()) {
            Hooks::PlayLayer::releaseButton(self, 0, false);
            Hooks::PlayLayer::releaseButton(self, 0, true);
        }
        if (from_checkpoint) {
            const Checkpoint& checkpoint = logic.checkpoints.back();
            logic.set_replay_pos(logic.find_replay_pos_after_checkpoint(checkpoint.number, checkpoint.inputs_at_frame_p1, checkpoint.inputs_at_frame_p2));
        }
        else {
            logic.set_replay_pos(logic.find_closest_input(true));
        }
    }

    if (logic.is_recording()) logic.total_attempt_count++;

    logic.calculated_xpos = self->m_pPlayer1->getPositionX();
    logic.calculated_frame = round(logic.get_frame() + (self->m_pPlayer1->getPositionX() - logic.calculated_xpos));
    logic.previous_xpos = self->m_pPlayer1->getPositionX();
    logic.player_x_positions.clear();

    logic.player_x_positions.push_back(self->m_pPlayer1->getPositionX());

    //logic.calculated_xpos = logic.xpos_calculation();
    //logic.previous_xpos = logic.xpos_calculation();

    /* if (logic.is_playing()) {
         releaseButton(self, 0, true);
         releaseButton(self, 0, false);
     }*/

    logic.live_inputs.clear();
    logic.over_max_cps = false;

    logic.end_portal_position = self->m_endPortal->getPositionX();

    auto cps_counter = (cocos2d::CCLabelBMFont*)self->getChildByTag(CPS_LABEL_ID);

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
        snprintf(logic.macro_name, sizeof(logic.macro_name), "%s", self->m_level->levelName.c_str());
        logic.recorder.video_name = logic.macro_name;
    }

    if (!self->m_isPracticeMode)
        logic.checkpoints.clear();

    if (logic.is_recording() || logic.is_playing()) {
        logic.handle_checkpoint_data();
    }

    // Section 3: Update Time and Activated Objects
    if (self->m_checkpoints->count() > 0 && !logic.checkpoints.empty()) {
        logic.calculated_xpos = logic.checkpoints.back().calculated_xpos;

        self->m_time = logic.get_latest_offset();
        constexpr auto delete_from = [&](auto& vec, size_t index) {
            if (index < vec.size())
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

    if (logic.is_in_update())
        logic.begin_update(self->m_time);

    if (logic.is_playing() && !logic.inputs.empty()) {
        if (!from_checkpoint)
            logic.set_replay_pos(logic.find_closest_input(true));
        logic.play_macro();
    }

    // Section 4: Handle Recording
    if (logic.is_recording()) {

        logic.cps_over_percents.clear();

        const int frame = logic.get_frame();

        if (from_checkpoint) {
            const Checkpoint& checkpoint = logic.checkpoints.back();
            if (logic.record_player_1)
                logic.trim_recorded_inputs(frame, false, checkpoint.inputs_at_frame_p1);
            if (logic.record_player_2)
                logic.trim_recorded_inputs(frame, true, checkpoint.inputs_at_frame_p2);
        }
        else if (!logic.no_overwrite) {
            if (logic.record_player_1)
                logic.trim_recorded_inputs(frame, false, 0);
            if (logic.record_player_2)
                logic.trim_recorded_inputs(frame, true, 0);
        }

        const bool flip_controls = gd::GameManager::sharedState()->getGameVariable("0010");

        auto sync_hold = [&](gd::PlayerObject* player, bool isPlayer2, bool live, const CheckpointData* saved) {
            if (!player) return;

            const bool stored = saved ? saved->isHolding : false;
            player->m_isHolding = stored;
            player->m_isHolding2 = saved ? saved->isHolding2 : false;

            if (live == stored) return;

            const bool button = (!isPlayer2) != flip_controls;
            logic.record_player_input(live, isPlayer2);
            if (live)
                pushButton(self, 0, button);
            else
                releaseButton(self, 0, button);
        };

        const Checkpoint* checkpoint = from_checkpoint ? &logic.checkpoints.back() : nullptr;

        if (logic.record_player_1 && !(logic.is_playing() && logic.play_player_1))
            sync_hold(self->m_pPlayer1, false, live_hold_p1, checkpoint ? &checkpoint->player_1_data : nullptr);
        if (self->m_level->twoPlayerMode && logic.record_player_2 && !(logic.is_playing() && logic.play_player_2))
            sync_hold(self->m_pPlayer2, true, live_hold_p2, checkpoint ? &checkpoint->player_2_data : nullptr);

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
        if (ring->m_bHasBeenActivated)
            logic.activated_objects.push_back(ring);
        if (ring->m_bHasBeenActivatedP2)
            logic.activated_objects_p2.push_back(ring);
    }
}

void __fastcall Hooks::bumpPlayer_h(gd::GJBaseGameLayer* self, int, gd::PlayerObject* player, gd::GameObject* object)
{
    auto& logic = Logic::get();

    bumpPlayer(self, player, object);

    if (logic.is_recording() || logic.is_playing()) {
        if (object->m_bHasBeenActivated)
            logic.activated_objects.push_back(object);
        if (object->m_bHasBeenActivatedP2)
            logic.activated_objects_p2.push_back(object);
    }
}

void __fastcall Hooks::PlayLayer::flipGravity_h(gd::PlayLayer* self, gd::PlayerObject* player, bool idk, bool idk2)
{
    Logic::get().is_over_orb = true;
    if (Logic::get().hacks.trajectory && TrajectorySimulation::getInstance()->shouldInterrumpHooks())
        return;

    PlayLayer::flipGravity(self, player, idk, idk2);
}

void __fastcall Hooks::PlayLayer::playGravityEffect_h(gd::PlayLayer* self, bool toggle)
{
    Logic::get().is_over_orb = true;
    if (Logic::get().hacks.trajectory && TrajectorySimulation::getInstance()->shouldInterrumpHooks())
        return;

    PlayLayer::playGravityEffect(self, toggle);
}

void __fastcall Hooks::GameObject_activateObject_h(gd::GameObject* self, int, gd::PlayerObject* player) {
    if (Logic::get().hacks.trajectory && TrajectorySimulation::getInstance()->shouldInterrumpHooksWithPlayer(player))
        return;

    if (self->getType() != gd::kGameObjectTypeDashRing && self->getType() != gd::kGameObjectTypeGravityDashRing)
        Logic::get().is_over_orb = true;

    GameObject_activateObject(self, player);
    auto& logic = Logic::get();
    if ((logic.is_recording() || logic.is_playing()) && *reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(self) + 0x2ca)) {
        if (self->m_bHasBeenActivated)
            logic.activated_objects.push_back(self);
        if (self->m_bHasBeenActivatedP2)
            logic.activated_objects_p2.push_back(self);
    }
}

void* __fastcall Hooks::PlayLayer::exitLevel_h(gd::PlayLayer* self, int) {
    auto& logic = Logic::get();

    logic.autoclicker = false;
    logic.checkpoints.clear();
    logic.activated_objects.clear();
    logic.activated_objects_p2.clear();
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

    constexpr bool snapshot_objects = false;

    if (snapshot_objects && PLAYLAYER->m_pObjectLayer) {
        auto layer = static_cast<CCLayer*>(PLAYLAYER->getChildren()->objectAtIndex(2));
        float xp = -layer->getPositionX() / layer->getScale();
        cast_function caster = make_cast<gd::GameObject, CCObject>();

        for (int s = PLAYLAYER->sectionForPos(xp) - (5 / layer->getScale()); s < PLAYLAYER->sectionForPos(xp) + (6 / layer->getScale()); ++s) {
            if (s < 0)
                continue;
            if (s >= PLAYLAYER->m_sectionObjects->count())
                break;
            auto section = static_cast<CCArray*>(PLAYLAYER->m_sectionObjects->objectAtIndex(s));
            for (size_t i = 0; i < section->count(); ++i)
            {
                auto obj = static_cast<gd::GameObject*>(section->objectAtIndex(i));
                if (obj) {
                    if (obj->m_nObjectType == gd::kGameObjectTypeDecoration) continue;
                    ObjectData data;
                    data.tag = obj->m_uID;
                    data.posX = obj->getPositionX();
                    data.posY = obj->getPositionY();
                    data.rotX = obj->getRotationX();
                    data.rotY = obj->getRotationY();
                    data.rotation = obj->getRotation();
                    data.velX = obj->getSkewX();
                    data.velY = obj->getSkewY();
                    data.speed1 = obj->m_unk2F4;
                    data.speed2 = obj->m_unk2F8;
                    data.speed3 = obj->m_unk33C;
                    data.speed4 = obj->m_unk340;
                    data.speed5 = obj->m_unk390;

                    data.m_bUnk3 = obj->m_bUnk3;
                    data.m_bIsBlueMaybe = obj->m_bIsBlueMaybe;
                    data.m_fUnk2 = obj->m_fUnk2;
                    data.m_fUnk = obj->m_fUnk;
                    data.m_fUnk3 = obj->m_fUnk3;
                    data.m_fUnk4 = obj->m_fUnk4;
                    data.m_bUnk = obj->m_bUnk;
                    data.m_fAnimSpeed2 = obj->m_fAnimSpeed2;
                    data.m_bIsEffectObject = obj->m_bIsEffectObject;
                    data.m_bRandomisedAnimStart = obj->m_bRandomisedAnimStart;
                    data.m_fAnimSpeed = obj->m_fAnimSpeed;
                    data.m_bBlackChild = obj->m_bBlackChild;
                    data.m_bUnkOutlineMaybe = obj->m_bUnkOutlineMaybe;
                    data.m_fBlackChildOpacity = obj->m_fBlackChildOpacity;
                    data.field_21C = obj->field_21C;
                    data.m_bEditor = obj->m_bEditor;
                    data.m_bGroupDisabled = obj->m_bGroupDisabled;
                    data.m_bColourOnTop = obj->m_bColourOnTop;
                    data.m_pMainColourMode = obj->m_pMainColourMode;
                    data.m_pSecondaryColourMode = obj->m_pSecondaryColourMode;
                    data.m_bCol1 = obj->m_bCol1;
                    data.m_bCol2 = obj->m_bCol2;
                    data.m_obStartPosOffset = obj->m_obStartPosOffset;
                    data.m_fUnkRotationField = obj->m_fUnkRotationField;
                    data.m_bTintTrigger = obj->m_bTintTrigger;
                    data.m_bIsFlippedX = obj->m_bIsFlippedX;
                    data.m_bIsFlippedY = obj->m_bIsFlippedY;
                    data.m_obBoxOffset = obj->m_obBoxOffset;
                    data.m_bIsOriented = obj->m_bIsOriented;
                    data.m_obBoxOffset2 = obj->m_obBoxOffset2;
                    data.m_pObjectOBB2D = obj->m_pObjectOBB2D;
                    data.m_bOriented = obj->m_bOriented;
                    data.m_pGlowSprite = obj->m_pGlowSprite;
                    data.m_bNotEditor = obj->m_bNotEditor;
                    data.m_pMyAction = obj->m_pMyAction;
                    data.m_bUnk1 = obj->m_bUnk1;
                    data.m_bRunActionWithTag = obj->m_bRunActionWithTag;
                    data.m_bObjectPoweredOn = obj->m_bObjectPoweredOn;
                    data.m_obObjectSize = obj->m_obObjectSize;
                    data.m_bTrigger = obj->m_bTrigger;
                    data.m_bActive = obj->m_bActive;
                    data.m_bAnimationFinished = obj->m_bAnimationFinished;
                    data.m_pParticleSystem = obj->m_pParticleSystem;
                    data.m_sEffectPlistName = obj->m_sEffectPlistName;
                    data.m_bParticleAdded = obj->m_bParticleAdded;
                    data.m_bHasParticles = obj->m_bHasParticles;
                    data.m_bUnkCustomRing = obj->m_bUnkCustomRing;
                    data.m_obPortalPosition = obj->m_obPortalPosition;
                    data.m_bUnkParticleSystem = obj->m_bUnkParticleSystem;
                    data.m_obObjectTextureRect = obj->m_obObjectTextureRect;
                    data.m_bTextureRectDirty = obj->m_bTextureRectDirty;
                    data.m_fRectXCenterMaybe = obj->m_fRectXCenterMaybe;
                    data.m_obObjectRect2 = obj->m_obObjectRect2;
                    data.m_bIsObjectRectDirty = obj->m_bIsObjectRectDirty;
                    data.m_bIsOrientedRectDirty = obj->m_bIsOrientedRectDirty;
                    data.m_bHasBeenActivated = obj->m_bHasBeenActivated;
                    data.m_bHasBeenActivatedP2 = obj->m_bHasBeenActivatedP2;

                    data.m_objectRadius = obj->m_objectRadius;
                    data.m_bIsRotatedSide = obj->m_bIsRotatedSide;
                    data.m_unk2F4 = obj->m_unk2F4;
                    data.m_unk2F8 = obj->m_unk2F8;
                    data.m_nUniqueID = obj->m_nUniqueID;
                    data.m_nObjectType = obj->m_nObjectType;
                    data.m_nSection = obj->m_nSection;
                    data.m_bTouchTriggered = obj->m_bTouchTriggered;
                    data.m_bSpawnTriggered = obj->m_bSpawnTriggered;

                    data.m_obStartPosition = obj->m_obStartPosition;
                    data.m_sTextureName = obj->m_sTextureName;
                    data.m_unk32C = obj->m_unk32C;
                    data.m_unk32D = obj->m_unk32D;
                    data.m_unk33C = obj->m_unk33C;
                    data.m_unk340 = obj->m_unk340;
                    data.m_bIsGlowDisabled = obj->m_bIsGlowDisabled;
                    data.m_nTargetColorID = obj->m_nTargetColorID;
                    data.m_fScale = obj->m_fScale;
                    data.m_nObjectID = obj->m_nObjectID;
                    data.m_unk368 = obj->m_unk368;
                    data.m_unk369 = obj->m_unk369;
                    data.m_unk36A = obj->m_unk36A;
                    data.m_bIsDontEnter = obj->m_bIsDontEnter;
                    data.m_bIsDontFade = obj->m_bIsDontFade;
                    data.m_nDefaultZOrder = obj->m_nDefaultZOrder;
                    data.m_unk38C = obj->m_unk38C;
                    data.m_unk38D = obj->m_unk38D;
                    data.m_unk38E = obj->m_unk38E;
                    data.m_unk390 = obj->m_unk390;
                    data.m_pBaseColor = obj->m_pBaseColor;
                    data.m_pDetailColor = obj->m_pDetailColor;
                    data.m_nDefaultZLayer = obj->m_nDefaultZLayer;
                    data.m_nZLayer = obj->m_nZLayer;
                    data.m_nGameZOrder = obj->m_nGameZOrder;
                    data.m_unk3C0 = obj->m_unk3C0;
                    data.m_bShowGamemodeBorders = obj->m_bShowGamemodeBorders;
                    data.m_unk3D9 = obj->m_unk3D9;
                    data.m_bIsSelected = obj->m_bIsSelected;
                    data.m_nGlobalClickCounter = obj->m_nGlobalClickCounter;
                    data.m_bUnknownLayerRelated = obj->m_bUnknownLayerRelated;
                    data.m_fMultiScaleMultiplier = obj->m_fMultiScaleMultiplier;
                    data.m_bIsGroupParent = obj->m_bIsGroupParent;
                    data.m_pGroups = obj->m_pGroups;
                    data.m_nGroupCount = obj->m_nGroupCount;
                    data.m_nEditorLayer = obj->m_nEditorLayer;
                    data.m_nEditorLayer2 = obj->m_nEditorLayer2;
                    data.m_unk414 = obj->m_unk414;
                    data.m_obFirstPosition = obj->m_obFirstPosition;
                    data.m_bHighDetail = obj->m_bHighDetail;
                    data.m_pColorActionSprite1 = obj->m_pColorActionSprite1;
                    data.m_pColorActionSprite2 = obj->m_pColorActionSprite2;
                    data.m_pEffectManager = obj->m_pEffectManager;

                    childData[data.tag] = data;
                }
            }
        }
    }

    /*cast_function caster = make_cast<gd::GameObject, CCObject>();
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
    }*/

    std::vector<CCAction*> actions;

    if (snapshot_objects) {
        unsigned action_amount = PLAYLAYER->numberOfRunningActions();

        for (int i = 0; i < action_amount; i++) {
            if (auto action = PLAYLAYER->getActionByTag(i))
                actions.push_back(action);
        }
    }

    Checkpoint checkpoint{ logic.get_frame(), checkpointData1, checkpointData2, logic.activated_objects.size(), logic.activated_objects_p2.size(), std::move(childData), logic.calculated_xpos, std::move(actions) };
    const size_t applied_inputs = logic.is_playing() ? static_cast<size_t>(logic.get_replay_pos()) : logic.get_inputs().size();
    checkpoint.inputs_at_frame_p1 = logic.count_inputs_at(checkpoint.number, false, applied_inputs);
    checkpoint.inputs_at_frame_p2 = logic.count_inputs_at(checkpoint.number, true, applied_inputs);
    logic.save_checkpoint(std::move(checkpoint));

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

gd::GameObject* __fastcall Hooks::hasBeenActivatedByPlayer_h(gd::GameObject* self, int, gd::GameObject* other)
{
    /*if (Logic::get().hacks.trajectory && TrajectorySimulation::getInstance()->shouldInterrumpHooks())
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
    }*/

    auto& logic = Logic::get();
    if ((logic.is_recording() || logic.is_playing()) && *reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(self) + 0x2ca)) {
        if (self->m_bHasBeenActivated && (logic.activated_objects.empty() || logic.activated_objects.back() != self))
            logic.activated_objects.push_back(self);
        if (self->m_bHasBeenActivatedP2 && (logic.activated_objects_p2.empty() || logic.activated_objects_p2.back() != self))
            logic.activated_objects_p2.push_back(self);
    }

    return hasBeenActivatedByPlayer(self, other);
}