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
#include "./Fonts/OpenSans-Medium.c"
#include <gd.h>
#define PLAYLAYER gd::GameManager::sharedState()->getPlayLayer()

#include "../Logic/logic.hpp"

using namespace cocos2d;

static ImFont* g_font = nullptr;

void GUI::draw() {
	if (g_font) ImGui::PushFont(g_font);

	if (show_window) {
		ImGui::Begin("Echo", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

		if (ImGui::BeginTabBar("TabBar")) {
			main();
		}

		ImGui::End();
	}


	if (g_font) ImGui::PopFont();
}

void GUI::main() {
	auto& logic = Logic::get();
	auto& macro = logic.get_macro();
	if (ImGui::BeginTabItem("Main")) {


		if (ImGui::Button("Toggle Recording")) {
			logic.toggle_recording();
		}
		if (ImGui::Button("Toggle Playing")) {
			logic.toggle_playing();
		}

		ImGui::Text("Frame: %i", logic.get_frame());

		ImGui::InputFloat("FPS", &macro.fps);

		ImGui::Text("Input count: %i", macro.get_inputs().size());

		if (ImGui::Button("Save File")) {
			macro.write_file(macro.macro_name);
		}

		ImGui::SameLine();

		if (ImGui::Button("Load File")) {
			macro.read_file(macro.macro_name);
		}

		ImGui::Text("%s", macro.macro_name);

		if (macro.error != "") {
			ImGui::Separator();
			ImGui::Text("%s", macro.error);
		}

		ImGui::EndTabItem();
	}
}

void GUI::init() {
	auto& style = ImGui::GetStyle();
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.WindowBorderSize = 0;
	style.ColorButtonPosition = ImGuiDir_Left;

	// Import the font bytes from file
	extern unsigned char OpenSans_Medium[];

	ImFontConfig fontConfig;
	fontConfig.FontDataOwnedByAtlas = false;
	ImGui::GetIO().Fonts->AddFontFromMemoryTTF(OpenSans_Medium, 129948, 21.f, &fontConfig);

	auto colors = style.Colors;
	colors[ImGuiCol_FrameBg] = ImVec4(0.31f, 0.31f, 0.31f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.59f, 0.59f, 0.59f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.61f, 0.61f, 0.61f, 0.67f);
	colors[ImGuiCol_TitleBg] = colors[ImGuiCol_TitleBgActive] = ImVec4(0.2f, 0.2f, 0.2f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.89f, 0.89f, 0.89f, 1.00f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.71f, 0.71f, 0.71f, 0.35f);
}