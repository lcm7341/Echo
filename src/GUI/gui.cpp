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

#define PLAYLAYER gd::GameManager::sharedState()->getPlayLayer()

namespace fs = std::filesystem;


using namespace cocos2d;

static ImFont* g_font = nullptr;

void GUI::draw() {
	if (g_font) ImGui::PushFont(g_font);

	if (show_window) {
		ImGui::SetNextWindowFocus();



		char title[1000] = "Echo [";
		std::stringstream full_title;

		full_title << title << ECHO_VERSION << "b]";

		ImGui::Begin(full_title.str().c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavNoCaptureKeyboard;

		if (io.KeysDown[ImGui::GetKeyIndex(ImGuiKey_Escape)])
			ImGui::SetKeyboardFocusHere();

		// Gonna attempt a resizing menu based on screen size :P

		// Get the size of the game window
		ImVec2 game_window_size = ImVec2(1920.f, 1080.f); // assume the game window size is 800x600

		// Get the size of the display
		ImVec2 display_size = ImGui::GetIO().DisplaySize;

		// Calculate the scale factor based on the game window size and display size
		float scale_factor = min(display_size.x / game_window_size.x, display_size.y / game_window_size.y);

		// Set the font and window scaling based on the scale factor
		ImGui::GetIO().FontGlobalScale = scale_factor;
		ImGui::SetNextWindowSize(ImVec2(400.0f * scale_factor, 300.0f * scale_factor));

		// Set the global scale factor for the UI
		auto& style = ImGui::GetStyle();
		style.ScaleAllSizes(scale_factor);


		if (ImGui::BeginTabBar("TabBar")) {
			main();
			editor();
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
	static Input newInput;

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
			ImGui::Text("%s at %d", inputs[i].down ? "Click" : "Release", inputs[i].frame);
			ImGui::PopID();
		}
		ImGui::EndChild();


		if (ImGui::Button("Add Input")) {
			if (selectedInput == -1) {
				inputs.push_back(Input());
				selectedInput = inputs.size() - 1;
			}
			else {
				inputs.insert(inputs.begin() + selectedInput, Input());
			}
			newInput = Input();
		}

		ImGui::Separator();
		ImGui::BeginChild("##EditArea", ImVec2(0, editAreaHeight), true);
		if (selectedInput >= 0 && selectedInput < inputs.size()) {
			ImGui::Text("Editing Input %d", selectedInput + 1);
			ImGui::InputInt("Frame", (int*)&newInput.frame);
			ImGui::Checkbox("Down", &newInput.down);
			ImGui::SameLine();
			ImGui::Checkbox("Player 1", &newInput.player1);
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
			logic.fps = input_fps;
			CCDirector::sharedDirector()->setAnimationInterval(1.f / logic.fps);
		}

		ImGui::Text("Macro Size: %i", logic.get_inputs().size());

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
		ImGui::InputText("Macro Name", logic.macro_name, 1000);
		ImGui::PopItemFlag();

		ImGui::SetNextItemWidth((ImGui::GetWindowContentRegionWidth() - ImGui::GetStyle().ItemSpacing.x) * (50.f / 100.f));
		if (ImGui::Button("Save File")) {
			logic.write_file(logic.macro_name);
		}

		ImGui::SameLine();

		ImGui::SetNextItemWidth(get_width(50.f));
		if (ImGui::Button("Load File")) {
			logic.read_file(logic.macro_name);
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