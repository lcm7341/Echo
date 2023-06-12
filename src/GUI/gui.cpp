#include "gui.hpp"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cocos2d.h>
#include <imgui.h>
#include <MinHook.h>
#include <sstream>
#include <imgui-hook.hpp>
#include <string_view>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <filesystem>
#include "./Fonts/OpenSans-Medium.h"
#include <gd.h>
#include <filesystem>
#include "../Logic/logic.hpp"
#include <imgui_internal.h>
#include "../version.h"
#include <sstream>
#include <optional>
#include <random>
#include "../imfilebrowser.h"

int getRandomInt(int N) {
	// Seed the random number generator with current time
	std::random_device rd;
	std::mt19937 gen(rd());

	// Define the triangular distribution
	std::uniform_int_distribution<> dis(0, N);

	// Generate and return the random number
	int randomInt = dis(gen);

	// Generate a second random number to determine the direction of bias
	int bias = dis(gen);

	// Adjust the random number based on the bias
	if (bias < randomInt) {
		randomInt -= bias;
	}

	return randomInt;
}
#define PLAYLAYER gd::GameManager::sharedState()->getPlayLayer()

namespace fs = std::filesystem;

using namespace cocos2d;

static ImFont* g_font = nullptr;

void GUI::draw() {
	if (g_font) ImGui::PushFont(g_font);

	if (show_window) {
		std::stringstream full_title;

		full_title << "Echo [" << ECHO_VERSION << "b]";

		ImGui::Begin(full_title.str().c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavNoCaptureKeyboard;

		if (io.KeysDown[ImGui::GetKeyIndex(ImGuiKey_Escape)])
			ImGui::SetKeyboardFocusHere();

		if (ImGui::BeginTabBar("TabBar")) {
			main();
			tools();
			editor();
			renderer();
			sequential_replay();
			conversion();
		}

		ImGui::End();
	}


	if (g_font) ImGui::PopFont();
}

float get_width(float percent) {
	return (ImGui::GetWindowContentRegionWidth() - ImGui::GetStyle().ItemSpacing.x) * (percent / 100.f);
}

void GUI::editor() {
	auto& logic = Logic::get();

	auto& inputs = logic.get_inputs();

	static unsigned selectedInput = -1;
	static Frame newInput;

	const float editAreaHeight = 150.0f;

	ImGui::SetNextWindowSizeConstraints(ImVec2(300, 300), ImVec2(300, 300));

	if (ImGui::BeginTabItem("Editor")) {

		ImGui::BeginChild("##List", ImVec2(0, 0), true);
		for (unsigned i = 0; i < inputs.size(); i++) {
			ImGui::PushID(i);
			if (ImGui::Selectable("##Input", selectedInput == i, ImGuiSelectableFlags_AllowDoubleClick)) {
				selectedInput = i;
				newInput = inputs[i];
			}
			ImGui::SameLine();
			ImGui::Text("%s at %d", inputs[i].pressingDown ? "Click" : "Release", inputs[i].number);
			ImGui::PopID();
		}
		ImGui::EndChild();


		if (ImGui::Button("Add Input")) {
			if (selectedInput == -1) {
				inputs.push_back(Frame());
				selectedInput = inputs.size() - 1;
			}
			else {
				inputs.insert(inputs.begin() + selectedInput, Frame());
			}
			newInput = Frame();
		}

		ImGui::Separator();
		ImGui::BeginChild("##EditArea", ImVec2(0, editAreaHeight), true);
		if (selectedInput >= 0 && selectedInput < inputs.size()) {
			ImGui::Text("Editing Input %d", selectedInput + 1);
			ImGui::InputInt("Frame", (int*)&newInput.number);
			ImGui::Checkbox("Down", &newInput.pressingDown);
			ImGui::SameLine();
			if (ImGui::Button("Move Up") && selectedInput > 0) {
				std::swap(inputs[selectedInput], inputs[selectedInput - 1]);
				selectedInput--;
			}
			ImGui::SameLine();
			if (ImGui::Button("Move Down") && selectedInput < inputs.size() - 1) {
				std::swap(inputs[selectedInput], inputs[selectedInput + 1]);
				selectedInput++;
			}
			ImGui::SameLine();
			if (ImGui::Button("Delete")) {
				inputs.erase(inputs.begin() + selectedInput);
				if (selectedInput >= inputs.size()) selectedInput = inputs.size() - 1;
				newInput = inputs[selectedInput];
			}
			inputs[selectedInput] = newInput;
		}
		ImGui::EndChild();

		ImGui::PushItemFlag(ImGuiItemFlags_NoTabStop, true);
		ImGui::SetNextItemWidth((ImGui::GetWindowContentRegionWidth() - ImGui::GetStyle().ItemSpacing.x) * 0.3f);
		ImGui::InputFloat("###frames", &offset_frames, 0, 0, "%.0f"); ImGui::SameLine();
		ImGui::PopItemFlag();

		if (ImGui::Button("Offset Replay")) {
			logic.offset_inputs(-offset_frames, offset_frames);
			offset_frames = 0;
		}

		ImGui::EndTabItem();
	}
}

struct Resolution {
	int width;
	int height;
};


void GUI::renderer() {
	auto& logic = Logic::get();
	const char* resolution_types[] = { "SD (720p)", "HD (1080p)", "QHD (1440p)", "4K (2160p)" };

	static const char* current_res = resolution_types[1];
	static int current_index = 1;

	if (ImGui::BeginTabItem("Render")) {

		Resolution resolutions[] = {
			{ 1280, 720 }, { 1920, 1080 }, { 2560, 1440 }, { 3840, 2160 }
		};

		if (ImGui::BeginCombo("Presets", resolution_types[current_index])) {
			for (int i = 0; i < IM_ARRAYSIZE(resolution_types); i++) {
				bool is_selected = (current_res == resolution_types[i]);
				if (ImGui::Selectable(resolution_types[i], is_selected)) {
					current_res = resolution_types[i];
					current_index = i;

					logic.recorder.m_width = resolutions[current_index].width;
					logic.recorder.m_height = resolutions[current_index].height;
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		float windowWidth = ImGui::GetWindowContentRegionWidth() * 0.6f;

		ImGui::SetNextItemWidth(windowWidth / 2.f);
		ImGui::InputInt("Width", (int*)&logic.recorder.m_width, 0);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(windowWidth / 2.f);
		ImGui::InputInt("Height", (int*)&logic.recorder.m_height, 0);

		if (ImGui::Button("Set To Window Size")) {
			auto windowSize = ImGui::GetIO().DisplaySize;
			auto windowX = windowSize.x;
			auto windowY = windowSize.y;

			logic.recorder.m_height = windowY;
			logic.recorder.m_width = windowX;
		}

		ImGui::Separator();

		ImGui::InputInt("Video FPS", (int*)&logic.recorder.m_fps);

		ImGui::InputText("Video Codec", &logic.recorder.m_codec);

		ImGui::InputInt("Bitrate (M)", &logic.recorder.m_bitrate);

		ImGui::Separator();

		ImGui::InputText("Extra Args", &logic.recorder.m_extra_args);
		
		ImGui::InputText("Extra Audio Args", &logic.recorder.m_extra_audio_args);

		ImGui::InputFloat("Extra Time", &logic.recorder.m_after_end_duration);

		ImGui::SameLine();

		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(?)");

		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Extra Time is the time in seconds after you finish a level that the recorder automatically records for..");
		}

		ImGui::Separator();

		ImGui::InputText("Video Name", &logic.recorder.video_name);

		ImGui::Checkbox("Color Fix", &logic.recorder.color_fix);

		if (PLAYLAYER) {
			if (!logic.recorder.m_recording) {
				if (ImGui::Button("Start Recording")) {
					logic.recorder.start(logic.recorder.directory + logic.recorder.video_name + logic.recorder.extension);
				}
			}
			else {
				if (ImGui::Button("Stop Recording")) {
					logic.recorder.stop();
				}
			}
		}
		else {
			ImGui::BeginDisabled();
			ImGui::Button("Start Recording");
			ImGui::EndDisabled();
		}

		ImGui::EndTabItem();
	}
}

void GUI::sequential_replay() {
	auto& logic = Logic::get();

	static std::optional<size_t> selected_replay_index = std::nullopt;
	const float child_height = 150.0f;

	if (ImGui::IsKeyDown(KEY_Escape)) { //pls find a more intuitive way to do this
		selected_replay_index = std::nullopt;
	}

	if (ImGui::BeginTabItem("Sequential replay")) {

		ImGui::Checkbox("Enabled", &logic.sequence_enabled);

		if (ImGui::Button("Remove All")) {
			logic.replays.clear();
		}

		static int all_offset = 0;

		if (ImGui::InputInt("Offset All", &all_offset)) {
			for (auto& replay : logic.replays) {
				replay.max_frame_offset = all_offset;
				for (auto& input : replay.actions) {
					if (getRandomInt(1) == 1)
						input.number += getRandomInt(all_offset);
					else
						input.number -= getRandomInt(all_offset);
				}
			}
		}

		ImGui::BeginChild("##seq_replay_list", ImVec2(0, 0), true);
		for (unsigned i = 0; i < logic.replays.size(); i++) {
			ImGui::PushID(i);
			
			if (ImGui::Selectable("##replay", selected_replay_index.has_value() && selected_replay_index.value() == i, ImGuiSelectableFlags_AllowDoubleClick)) {
				selected_replay_index = i;
			}

			ImGui::SameLine();
			ImGui::Text(logic.replays[i].name.c_str());

			ImGui::PopID();
		}

		ImGui::EndChild();

		ImGui::Separator();
		
		ImGui::BeginChild("##seq_replay_child", ImVec2(0, child_height), true); //idk how 2 name dis

		if (selected_replay_index.has_value() && selected_replay_index.value() < logic.replays.size()) {

			Replay& selected_replay = logic.replays[selected_replay_index.value()];

			ImGui::Text("Name: %s", &selected_replay.name);
			ImGui::InputInt("Max frame offset", &selected_replay.max_frame_offset, 1, 5);

			if (ImGui::Button("Remove")) {
				logic.replays.erase(logic.replays.begin() + selected_replay_index.value());
				selected_replay_index = std::nullopt;
			}
		}
		else {
			static std::string replay_name{};
			static int amount = 1;

			ImGui::InputText("Name", &replay_name);
			ImGui::InputInt("Amount", &amount);

			if (ImGui::Button("Add")) {
				//HACK: idk if i'm allowed to  reformat half of the stuff in logic so i'll just do this.
				//		if i am lmk bc it's kinda messy rn
				std::vector<Frame> old_actions = logic.get_inputs();

				logic.get_inputs().clear();

				//TODO: catch this or whatever
				logic.read_file(replay_name, false);

				for (int i = 0; i < amount; i++) {
				auto inputs = logic.get_inputs();
					logic.replays.push_back(Replay{
							replay_name,
							0,
							inputs
						});
				}

				logic.get_inputs() = old_actions;
				//HACK END

				replay_name = {};
				amount = 1;
			}
		}

		ImGui::EndChild();

		ImGui::EndTabItem();
	}
}

void GUI::tools() {
	auto& logic = Logic::get();

	if (ImGui::BeginTabItem("Tools")) {

		ImGui::Checkbox("Click Both Players", &logic.click_both_players);

		ImGui::Checkbox("Swap Player Input", &logic.swap_player_input);


		ImGui::FileBrowser fileDialog;
		fileDialog.SetTitle("Replays");
		fileDialog.SetPwd(std::filesystem::path(".echo"));
		fileDialog.SetTypeFilters({ ".echo", ".bin" });

		if (ImGui::Button("Merge With Replay")) {
			fileDialog.Open();
		}

		fileDialog.Display();

		if (fileDialog.HasSelected())
		{
			std::vector<Frame> before_inputs = logic.get_inputs();
			logic.read_file(fileDialog.GetSelected().string(), true);
			logic.sort_inputs();
			fileDialog.ClearSelected();
		}


		ImGui::Separator();
		ImGui::SetNextItemWidth(get_width(50.f));
		if (ImGui::Button("Uninject DLL")) {
			// TO-DO
			// Create custom loader so that we can fetch the dll's base address without needing 2000 lines of code to support every OS
		}

		ImGui::SameLine;
		ImGui::SetNextItemWidth(get_width(50.f));
		if (ImGui::Button("Open Debug Console")) {
			// Allows for debugging, can be removed later
			AllocConsole();
			freopen("CONOUT$", "w", stdout);  // Redirects stdout to the new console
			std::cout << "Opened Debugging Console" << std::endl;
		}

		ImGui::EndTabItem();
	}
}


void GUI::conversion() {
	auto& logic = Logic::get();

	if (ImGui::BeginTabItem("Conversion")) {
		ImGui::FileBrowser fileDialog;
		fileDialog.SetTitle("Replays");
		fileDialog.SetPwd(std::filesystem::path(".echo"));
		fileDialog.SetTypeFilters({ ".json" });

		if (ImGui::Button("Convert TASBot")) {
			fileDialog.Open();
		}

		fileDialog.Display();

		if (fileDialog.HasSelected())
		{			                 
			logic.convert_file(fileDialog.GetSelected().string(), true);
			logic.sort_inputs();

			fileDialog.ClearSelected();
		}

		ImGui::Separator();

		ImGui::EndTabItem();
	}
}

void GUI::main() {
	auto& logic = Logic::get();

	if (ImGui::BeginTabItem("Main")) {
		if (ImGui::Button(logic.is_recording() ? "Stop Recording" : "Start Recording", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 0))) {
			logic.toggle_recording();
		}

		ImGui::SameLine();
		
		//if (logic.get_inputs().empty()) ImGui::BeginDisabled();

		if (ImGui::Button(logic.is_playing() ? "Stop Playing" : "Start Playing", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.48f, 0))) {
			logic.toggle_playing();
		}

		//ImGui::EndDisabled();

		//ImGui::Text("Currently: %s", logic.is_idle() ? "Disabled" : logic.is_recording() ? "Recording" : "Playing");

		ImGui::Text("Frame: %i", logic.get_frame());

		ImGui::PushItemFlag(ImGuiItemFlags_NoTabStop, true);
		ImGui::SetNextItemWidth((ImGui::GetWindowContentRegionWidth() - ImGui::GetStyle().ItemSpacing.x) * 0.3f);
		ImGui::InputFloat("###fps", &input_fps, 0, 0, "%.0f"); ImGui::SameLine();
		ImGui::PopItemFlag();
		
		if (ImGui::Button("Set FPS")) {
			if (!logic.is_recording() && !logic.is_playing()) {
				logic.fps = input_fps;
			}
			CCDirector::sharedDirector()->setAnimationInterval(1.f / logic.fps);
		}

		ImGui::DragFloat("Speed", &logic.speedhack, 0.01, 0.01f, 100.f, "%.2f");

		ImGui::Checkbox("Real Time Mode", &logic.real_time_mode);

		ImGui::Checkbox("Ignore actions during playback", &logic.ignore_actions_at_playback);

		ImGui::Checkbox("Show frame", &logic.show_frame); //move this somewhere else if u want

		ImGui::Text("Macro Size: %i", logic.get_inputs().size());

		ImGui::SameLine();

		bool open_modal = true;

		if (ImGui::Button("Reset Macro")) {
			logic.get_inputs().clear();
			//ImGui::OpenPopup("Confirm Reset");
			//ImGui::SetNextWindowPos({ ImGui::GetWindowWidth() / 2.f, ImGui::GetWindowHeight() / 2.f });
		}

		if (ImGui::BeginPopupModal("Confirm Reset", &open_modal, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Are you sure you want to reset your macro?");

			const ImVec2 buttonSize((ImGui::GetWindowContentRegionWidth() - ImGui::GetStyle().ItemSpacing.x) * 0.5f, 0);

			if (ImGui::Button("Yes", buttonSize)) {
				logic.get_inputs().clear();
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();

			if (ImGui::Button("No", buttonSize)) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::PushItemFlag(ImGuiItemFlags_NoTabStop, true);
		ImGui::SetNextItemWidth(get_width(75.f));
		ImGui::InputText("Macro Name", logic.macro_name, MAX_PATH);
		ImGui::PopItemFlag();

		ImGui::SetNextItemWidth((ImGui::GetWindowContentRegionWidth() - ImGui::GetStyle().ItemSpacing.x) * (50.f / 100.f));
		if (ImGui::Button("Save File")) {
			logic.write_file(logic.macro_name);
		}

		ImGui::SameLine();

		ImGui::SetNextItemWidth(get_width(50.f));
		if (ImGui::Button("Load File")) {
			logic.read_file(logic.macro_name, false);
		}

		ImGui::SameLine();

		ImGui::SetNextItemWidth(get_width(50.f));
		if (ImGui::Button("Export .osu")) {
			logic.write_osu_file(logic.macro_name);
		}

		if (logic.error != "") {
			ImGui::Separator();
			ImGui::Text("%s", logic.error);
		}

		ImGui::EndTabItem();
	}
}

void GUI::init() {
	auto& style = ImGui::GetStyle();
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.WindowBorderSize = 0;
	style.ColorButtonPosition = ImGuiDir_Left;

	ImFontConfig fontConfig;
	fontConfig.FontDataOwnedByAtlas = false;

	ImGui::GetIO().Fonts->AddFontFromMemoryTTF(Fonts::OpenSans_Medium, sizeof(Fonts::OpenSans_Medium), 21.f, &fontConfig);

	style.FramePadding = ImVec2{ 8, 4 };
	style.IndentSpacing = 21;
	style.ItemSpacing = { 8, 8 };

	style.ScrollbarSize = 14;
	style.GrabMinSize = 8;
	style.WindowRounding = 12;
	style.ChildRounding = 12;
	style.FrameRounding = 10;
	style.PopupRounding = 10;
	style.ScrollbarRounding = 12;
	style.GrabRounding = 8;
	style.TabRounding = 12;
	style.WindowMenuButtonPosition = ImGuiDir_None;
	style.WindowPadding = { 8, 8 };
	style.ItemInnerSpacing = { 4, 4 };

	ImGui::GetIO().ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;


	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.23f, 0.54f, 0.46f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.80f, 0.66f, 0.54f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.10f, 0.91f, 0.71f, 0.54f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.46f, 0.48f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 1.00f, 0.75f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.63f, 0.56f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.04f, 0.37f, 0.29f, 0.54f);
	colors[ImGuiCol_Button] = ImVec4(0.18f, 0.89f, 0.71f, 0.54f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.02f, 1.00f, 0.75f, 0.54f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.15f, 0.67f, 0.55f, 0.54f);
	colors[ImGuiCol_Header] = ImVec4(0.23f, 0.54f, 0.46f, 0.54f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.80f, 0.66f, 0.54f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.09f, 0.91f, 0.71f, 0.54f);
	colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.25f, 0.80f, 0.66f, 0.54f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.09f, 0.91f, 0.71f, 0.54f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.23f, 0.54f, 0.46f, 0.54f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.25f, 0.80f, 0.66f, 0.54f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.09f, 0.91f, 0.71f, 0.54f);
	colors[ImGuiCol_Tab] = ImVec4(0.23f, 0.54f, 0.46f, 0.54f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.80f, 0.66f, 0.54f);
	colors[ImGuiCol_TabActive] = ImVec4(0.00f, 0.85f, 1.00f, 0.54f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.15f, 0.12f, 0.97f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.42f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.98f, 0.62f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.98f, 0.69f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);


}