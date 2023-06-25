#include "gui.hpp"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cocos2d.h>
#include <imgui.h>
#include <MinHook.h>
#include <cmath>
#include <sstream>
#include <imgui-hook.hpp>
#include <../imguifiledialog/ImGuiFileDialog.h>
#include <string_view>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <filesystem>
#include "./Fonts/OpenSans-Medium.h"
#include <gd.h>
#include <filesystem>
#include "../Logic/logic.hpp"
#include "../Logic/autoclicker.hpp"
#include <imgui_internal.h>
#include "../version.h"
#include <sstream>
#include <optional>
#include <random>
#include "../Hack/audiopitchHack.hpp"
#include "../Logic/convertible.h"
#include "../Logic/Conversions/tasbot.h"
#include "../Logic/Conversions/mhr.h"
#include "../Logic/Conversions/osu.h"
#include "../Logic/Conversions/plaintext.h"
#include "../Hooks/hooks.hpp"
#include <imgui_demo.cpp>
#include <format>

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
static Opcode anticheatBypass(Cheat::AntiCheatBypass);
static Opcode noclip(Cheat::NoClip);
static Opcode practiceMusic(Cheat::PracticeMusic);
static Opcode noEscape(Cheat::NoESC);

void delay(int milliseconds) {
	auto start = std::chrono::steady_clock::now();
	auto end = start + std::chrono::milliseconds(milliseconds);

	while (std::chrono::steady_clock::now() < end) {
		// Do nothing, just wait
	}
}

void GUI::draw() {
	if (g_font) ImGui::PushFont(g_font);

	if (show_window) {
		std::stringstream full_title;

		full_title << "Echo [" << ECHO_VERSION << "b]";

		ImGui::Begin(full_title.str().c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
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
			ui_editor();
		}

		ImGui::End();
	}

	auto end = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - Logic::get().start);

	if (duration.count() >= Logic::get().frame_advance_hold_duration && Logic::get().start != std::chrono::steady_clock::time_point()) {
		Logic::get().frame_advance = false;
		Hooks::CCScheduler_update_h(gd::GameManager::sharedState()->getScheduler(), 0, 1.f / Logic::get().fps);
		Logic::get().frame_advance = true;
		delay(Logic::get().frame_advance_delay);
		Logic::get().holding_frame_advance = true;
	}
	else {
		Logic::get().holding_frame_advance = false;
	}
	if (g_font) ImGui::PopFont();
}

float get_width(float percent) {
	return (ImGui::GetWindowContentRegionWidth() - ImGui::GetStyle().ItemSpacing.x) * (percent / 100.f);
}

void GUI::import_theme(std::string path, float window_scale) {
	std::ifstream file(path);

	ImGuiStyle& style = ImGui::GetStyle();

	if (!file.is_open())
	{
		std::cerr << "Failed to open theme.json for reading" << std::endl;
		return;
	}

	// Parse the JSON content
	nlohmann::json json;
	try
	{
		file >> json;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Failed to parse theme.json: " << e.what() << std::endl;
		file.close();
		return;
	}

	file.close();

	// Import ImGui style variables
	ImGui::SetWindowFontScale(json["WindowScale"]);
	window_scale = json["WindowScale"];
	ImGui::GetIO().FontGlobalScale = json["GlobalScale"];
	style.WindowPadding.x = json["WindowPaddingX"];
	style.WindowPadding.y = json["WindowPaddingY"];
	style.FramePadding.x = json["FramePaddingX"];
	style.FramePadding.y = json["FramePaddingY"];
	style.CellPadding.x = json["CellPaddingX"];
	style.CellPadding.y = json["CellPaddingY"];
	style.ItemSpacing.x = json["ItemSpacingX"];
	style.ItemSpacing.y = json["ItemSpacingY"];
	style.ItemInnerSpacing.x = json["ItemInnerSpacingX"];
	style.ItemInnerSpacing.y = json["ItemInnerSpacingY"];
	style.TouchExtraPadding.x = json["TouchExtraPaddingX"];
	style.TouchExtraPadding.y = json["TouchExtraPaddingY"];
	style.IndentSpacing = json["IndentSpacing"];
	style.ScrollbarSize = json["ScrollbarSize"];
	style.GrabMinSize = json["GrabMinSize"];
	style.WindowBorderSize = json["WindowBorderSize"];
	style.ChildBorderSize = json["ChildBorderSize"];
	style.PopupBorderSize = json["PopupBorderSize"];
	style.FrameBorderSize = json["FrameBorderSize"];
	style.TabBorderSize = json["TabBorderSize"];
	style.WindowRounding = json["WindowRounding"];
	style.ChildRounding = json["ChildRounding"];
	style.FrameRounding = json["FrameRounding"];
	style.PopupRounding = json["PopupRounding"];
	style.ScrollbarRounding = json["ScrollbarRounding"];
	style.GrabRounding = json["GrabRounding"];
	style.LogSliderDeadzone = json["LogSliderDeadzone"];
	style.TabRounding = json["TabRounding"];
	style.WindowTitleAlign.x = json["WindowTitleAlignX"];
	style.WindowTitleAlign.y = json["WindowTitleAlignY"];
	style.WindowMenuButtonPosition = json["WindowMenuButtonPosition"];
	style.ColorButtonPosition = json["ColorButtonPosition"];
	style.ButtonTextAlign.x = json["ButtonTextAlignX"];
	style.ButtonTextAlign.y = json["ButtonTextAlignY"];
	style.SelectableTextAlign.x = json["SelectableTextAlignX"];
	style.SelectableTextAlign.y = json["SelectableTextAlignY"];
	style.DisplaySafeAreaPadding.x = json["DisplaySafeAreaPaddingX"];
	style.DisplaySafeAreaPadding.y = json["DisplaySafeAreaPaddingY"];
	style.AntiAliasedLines = json["AntiAliasedLines"];
	style.AntiAliasedLinesUseTex = json["AntiAliasedLinesUseTex"];
	style.AntiAliasedFill = json["AntiAliasedFill"];
	style.CurveTessellationTol = json["CurveTessellationTol"];
	style.CircleTessellationMaxError = json["CircleTessellationMaxError"];
	style.Alpha = json["Alpha"];
	style.DisabledAlpha = json["DisabledAlpha"];

	if (json.contains("player_1_button_color")) {
		auto colorArray = json["player_1_button_color"];
		ImVec4 color(colorArray[0], colorArray[1], colorArray[2], colorArray[3]);
		player_1_button_color = color;
	}
	if (json.contains("player_2_button_color")) {
		auto colorArray = json["player_2_button_color"];
		ImVec4 color(colorArray[0], colorArray[1], colorArray[2], colorArray[3]);
		player_2_button_color = color;
	}

	for (int i = 0; i < ImGuiCol_COUNT; i++)
	{
		std::string colorName = ImGui::GetStyleColorName(i);
		if (json.contains(colorName))
		{
			auto colorArray = json[colorName];
			ImVec4 color(colorArray[0], colorArray[1], colorArray[2], colorArray[3]);
			style.Colors[i] = color;
		}
	}

	std::cout << "Theme imported from theme.json" << std::endl;
}

void GUI::ui_editor() {
	ImGuiStyle& style = ImGui::GetStyle();

	if (ImGui::BeginTabItem("Style")) {
		static float window_scale = 1.0f;


		if (ImGui::Button("Export Theme")) {
			nlohmann::json json;

			// Fill the json object with the variables
			json["WindowScale"] = window_scale;
			json["GlobalScale"] = ImGui::GetIO().FontGlobalScale;
			json["WindowPaddingX"] = style.WindowPadding.x;
			json["WindowPaddingY"] = style.WindowPadding.y;
			json["FramePaddingX"] = style.FramePadding.x;
			json["FramePaddingY"] = style.FramePadding.y;
			json["CellPaddingX"] = style.CellPadding.x;
			json["CellPaddingY"] = style.CellPadding.y;
			json["ItemSpacingX"] = style.ItemSpacing.x;
			json["ItemSpacingY"] = style.ItemSpacing.y;
			json["ItemInnerSpacingX"] = style.ItemInnerSpacing.x;
			json["ItemInnerSpacingY"] = style.ItemInnerSpacing.y;
			json["TouchExtraPaddingX"] = style.TouchExtraPadding.x;
			json["TouchExtraPaddingY"] = style.TouchExtraPadding.y;
			json["IndentSpacing"] = style.IndentSpacing;
			json["ScrollbarSize"] = style.ScrollbarSize;
			json["GrabMinSize"] = style.GrabMinSize;
			json["WindowBorderSize"] = style.WindowBorderSize;
			json["ChildBorderSize"] = style.ChildBorderSize;
			json["PopupBorderSize"] = style.PopupBorderSize;
			json["FrameBorderSize"] = style.FrameBorderSize;
			json["TabBorderSize"] = style.TabBorderSize;
			json["WindowRounding"] = style.WindowRounding;
			json["ChildRounding"] = style.ChildRounding;
			json["FrameRounding"] = style.FrameRounding;
			json["PopupRounding"] = style.PopupRounding;
			json["ScrollbarRounding"] = style.ScrollbarRounding;
			json["GrabRounding"] = style.GrabRounding;
			json["LogSliderDeadzone"] = style.LogSliderDeadzone;
			json["TabRounding"] = style.TabRounding;
			json["WindowTitleAlignX"] = style.WindowTitleAlign.x;
			json["WindowTitleAlignY"] = style.WindowTitleAlign.x;
			json["WindowMenuButtonPosition"] = style.WindowMenuButtonPosition;
			json["ColorButtonPosition"] = style.ColorButtonPosition;
			json["ButtonTextAlignX"] = style.ButtonTextAlign.x;
			json["ButtonTextAlignY"] = style.ButtonTextAlign.y;
			json["SelectableTextAlignX"] = style.SelectableTextAlign.x;
			json["SelectableTextAlignY"] = style.SelectableTextAlign.y;
			json["DisplaySafeAreaPaddingX"] = style.DisplaySafeAreaPadding.x;
			json["DisplaySafeAreaPaddingY"] = style.DisplaySafeAreaPadding.y;
			json["AntiAliasedLines"] = style.AntiAliasedLines;
			json["AntiAliasedLinesUseTex"] = style.AntiAliasedLinesUseTex;
			json["AntiAliasedFill"] = style.AntiAliasedFill;
			json["CurveTessellationTol"] = style.CurveTessellationTol;
			json["CircleTessellationMaxError"] = style.CircleTessellationMaxError;
			json["Alpha"] = style.Alpha;
			json["DisabledAlpha"] = style.DisabledAlpha;

			for (int i = 0; i < ImGuiCol_COUNT; i++)
			{
				ImVec4 color = ImGui::GetStyleColorVec4(i);
				std::string colorName = ImGui::GetStyleColorName(i);
				json[colorName] = { color.x, color.y, color.z, color.w };
			}

			json["player_1_button_color"] = { player_1_button_color.x, player_1_button_color.y, player_1_button_color.z, player_1_button_color.w };
			json["player_2_button_color"] = { player_2_button_color.x, player_2_button_color.y, player_2_button_color.z, player_2_button_color.w };

			// Save the json object to a file
			std::string path = ".echo\\settings\\";
			path += theme_name;
			path += ".ui";

			std::ofstream file(path);
			if (file.is_open())
			{
				file << json.dump(4); // Use 4 spaces for indentation
				file.close();
				std::cout << "Theme saved to theme.ui" << std::endl;
			}
			else
			{
				std::cerr << "Failed to open theme.ui for writing" << std::endl;
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Import Theme")) {
			ImGuiFileDialog::Instance()->OpenDialog("ThemeImport", "Choose File", ".ui", ".echo/settings/");
		}

		ImGui::InputText("Export As", theme_name, MAX_PATH);

		ImGui::Separator();

		if (ImGuiFileDialog::Instance()->Display("ThemeImport", ImGuiWindowFlags_NoCollapse, ImVec2(500, 200)))
		{
			// action if OK
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				try {
					import_theme(ImGuiFileDialog::Instance()->GetFilePathName(), window_scale);
				}
				catch (std::runtime_error& e) {
					printf("%s", e.what());
				}
			}

			ImGuiFileDialog::Instance()->Close();
		}

		ImGuiIO& io = ImGui::GetIO();
		ImFontAtlas* atlas = io.Fonts;
		//HelpMarker("Read FAQ and docs/FONTS.md for details on font loading.");
		//ImGui::ShowFontAtlas(atlas);

		// Post-baking font scaling. Note that this is NOT the nice way of scaling fonts, read below.
		// (we enforce hard clamping manually as by default DragFloat/SliderFloat allows CTRL+Click text to get out of bounds).
		const float MIN_SCALE = 0.3f;
		const float MAX_SCALE = 2.0f;

		//ImGui::PushItemWidth(ImGui::GetFontSize() * 8);
		//if (ImGui::DragFloat("Window Scale", &window_scale, 0.005f, MIN_SCALE, MAX_SCALE, "%.2f", ImGuiSliderFlags_AlwaysClamp)) // Scale only this window
		//	ImGui::SetWindowFontScale(window_scale);
		//ImGui::DragFloat("Global Scale", &io.FontGlobalScale, 0.005f, MIN_SCALE, MAX_SCALE, "%.2f", ImGuiSliderFlags_AlwaysClamp); // Scale everything
		//ImGui::PopItemWidth();

		if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem("Sizes"))
			{

				ImGui::SetNextWindowSizeConstraints(ImVec2(500, 500), ImVec2(500, 500));
				ImGui::BeginChild("###sizesChild", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NavFlattened);
				ImGui::Text("Main");
				ImGui::SliderFloat2("WindowPadding", (float*)&style.WindowPadding, 0.0f, 20.0f, "%.0f");
				ImGui::SliderFloat2("FramePadding", (float*)&style.FramePadding, 0.0f, 20.0f, "%.0f");
				ImGui::SliderFloat2("CellPadding", (float*)&style.CellPadding, 0.0f, 20.0f, "%.0f");
				ImGui::SliderFloat2("ItemSpacing", (float*)&style.ItemSpacing, 0.0f, 20.0f, "%.0f");
				ImGui::SliderFloat2("ItemInnerSpacing", (float*)&style.ItemInnerSpacing, 0.0f, 20.0f, "%.0f");
				ImGui::SliderFloat2("TouchExtraPadding", (float*)&style.TouchExtraPadding, 0.0f, 10.0f, "%.0f");
				ImGui::SliderFloat("IndentSpacing", &style.IndentSpacing, 0.0f, 30.0f, "%.0f");
				ImGui::SliderFloat("ScrollbarSize", &style.ScrollbarSize, 1.0f, 20.0f, "%.0f");
				ImGui::SliderFloat("GrabMinSize", &style.GrabMinSize, 1.0f, 20.0f, "%.0f");
				ImGui::Text("Borders");
				ImGui::SliderFloat("WindowBorderSize", &style.WindowBorderSize, 0.0f, 1.0f, "%.0f");
				ImGui::SliderFloat("ChildBorderSize", &style.ChildBorderSize, 0.0f, 1.0f, "%.0f");
				ImGui::SliderFloat("PopupBorderSize", &style.PopupBorderSize, 0.0f, 1.0f, "%.0f");
				ImGui::SliderFloat("FrameBorderSize", &style.FrameBorderSize, 0.0f, 1.0f, "%.0f");
				ImGui::SliderFloat("TabBorderSize", &style.TabBorderSize, 0.0f, 1.0f, "%.0f");
				ImGui::Text("Rounding");
				ImGui::SliderFloat("WindowRounding", &style.WindowRounding, 0.0f, 12.0f, "%.0f");
				ImGui::SliderFloat("ChildRounding", &style.ChildRounding, 0.0f, 12.0f, "%.0f");
				ImGui::SliderFloat("FrameRounding", &style.FrameRounding, 0.0f, 12.0f, "%.0f");
				ImGui::SliderFloat("PopupRounding", &style.PopupRounding, 0.0f, 12.0f, "%.0f");
				ImGui::SliderFloat("ScrollbarRounding", &style.ScrollbarRounding, 0.0f, 12.0f, "%.0f");
				ImGui::SliderFloat("GrabRounding", &style.GrabRounding, 0.0f, 12.0f, "%.0f");
				ImGui::SliderFloat("LogSliderDeadzone", &style.LogSliderDeadzone, 0.0f, 12.0f, "%.0f");
				ImGui::SliderFloat("TabRounding", &style.TabRounding, 0.0f, 12.0f, "%.0f");
				ImGui::Text("Alignment");
				ImGui::SliderFloat2("WindowTitleAlign", (float*)&style.WindowTitleAlign, 0.0f, 1.0f, "%.2f");
				int window_menu_button_position = style.WindowMenuButtonPosition + 1;
				if (ImGui::Combo("WindowMenuButtonPosition", (int*)&window_menu_button_position, "None\0Left\0Right\0"))
					style.WindowMenuButtonPosition = window_menu_button_position - 1;
				ImGui::Combo("ColorButtonPosition", (int*)&style.ColorButtonPosition, "Left\0Right\0");
				ImGui::SliderFloat2("ButtonTextAlign", (float*)&style.ButtonTextAlign, 0.0f, 1.0f, "%.2f");
				ImGui::SameLine(); HelpMarker("Alignment applies when a button is larger than its text content.");
				ImGui::SliderFloat2("SelectableTextAlign", (float*)&style.SelectableTextAlign, 0.0f, 1.0f, "%.2f");
				ImGui::SameLine(); HelpMarker("Alignment applies when a selectable is larger than its text content.");
				ImGui::Text("Safe Area Padding");
				ImGui::SameLine(); HelpMarker("Adjust if you cannot see the edges of your screen (e.g. on a TV where scaling has not been configured).");
				ImGui::SliderFloat2("DisplaySafeAreaPadding", (float*)&style.DisplaySafeAreaPadding, 0.0f, 30.0f, "%.0f");

				ImGui::EndChild();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Colors"))
			{
				static ImGuiTextFilter filter;
				filter.Draw("Filter colors", ImGui::GetFontSize() * 16);

				static ImGuiColorEditFlags alpha_flags = 0;
				if (ImGui::RadioButton("Opaque", alpha_flags == ImGuiColorEditFlags_None)) { alpha_flags = ImGuiColorEditFlags_None; } ImGui::SameLine();
				if (ImGui::RadioButton("Alpha", alpha_flags == ImGuiColorEditFlags_AlphaPreview)) { alpha_flags = ImGuiColorEditFlags_AlphaPreview; } ImGui::SameLine();
				if (ImGui::RadioButton("Both", alpha_flags == ImGuiColorEditFlags_AlphaPreviewHalf)) { alpha_flags = ImGuiColorEditFlags_AlphaPreviewHalf; } ImGui::SameLine();
				HelpMarker(
					"In the color list:\n"
					"Left-click on color square to open color picker,\n"
					"Right-click to open edit options menu.");
				
				ImGui::SetNextWindowSizeConstraints(ImVec2(500, 500), ImVec2(500, 500));
				ImGui::BeginChild("##colors", ImVec2(-1, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NavFlattened);
				ImGui::PushItemWidth(250);
				if (filter.PassFilter("Player 1 In Editor")) {
					ImGui::PushID(998);
					ImGui::ColorEdit4("##color", (float*)&player_1_button_color, ImGuiColorEditFlags_AlphaBar | alpha_flags);
					ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
					ImGui::TextUnformatted("Player 1 In Editor");
					ImGui::PopID();
				}
				if (filter.PassFilter("Player 2 In Editor")) {
					ImGui::PushID(999);
					ImGui::ColorEdit4("##color", (float*)&player_2_button_color, ImGuiColorEditFlags_AlphaBar | alpha_flags);
					ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
					ImGui::TextUnformatted("Player 2 In Editor");
					ImGui::PopID();
				}
				for (int i = 0; i < ImGuiCol_COUNT; i++)
				{
					const char* name = ImGui::GetStyleColorName(i);
					if (!filter.PassFilter(name))
						continue;
					ImGui::PushID(i);
					ImGui::ColorEdit4("##color", (float*)&style.Colors[i], ImGuiColorEditFlags_AlphaBar | alpha_flags);
					ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
					ImGui::TextUnformatted(name);
					ImGui::PopID();
				}
				ImGui::PopItemWidth();
				ImGui::EndChild();

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Rendering"))
			{
				ImGui::Checkbox("Anti-aliased lines", &style.AntiAliasedLines);
				ImGui::SameLine();
				HelpMarker("When disabling anti-aliasing lines, you'll probably want to disable borders in your style as well.");

				ImGui::Checkbox("Anti-aliased lines use texture", &style.AntiAliasedLinesUseTex);
				ImGui::SameLine();
				HelpMarker("Faster lines using texture data. Require backend to render with bilinear filtering (not point/nearest filtering).");

				ImGui::Checkbox("Anti-aliased fill", &style.AntiAliasedFill);
				ImGui::PushItemWidth(ImGui::GetFontSize() * 8);
				ImGui::DragFloat("Curve Tessellation Tolerance", &style.CurveTessellationTol, 0.02f, 0.10f, 10.0f, "%.2f");
				if (style.CurveTessellationTol < 0.10f) style.CurveTessellationTol = 0.10f;

				// When editing the "Circle Segment Max Error" value, draw a preview of its effect on auto-tessellated circles.
				ImGui::DragFloat("Circle Tessellation Max Error", &style.CircleTessellationMaxError, 0.005f, 0.10f, 5.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
				if (ImGui::IsItemActive())
				{
					ImGui::SetNextWindowPos(ImGui::GetCursorScreenPos());
					ImGui::BeginTooltip();
					ImGui::TextUnformatted("(R = radius, N = number of segments)");
					ImGui::Spacing();
					ImDrawList* draw_list = ImGui::GetWindowDrawList();
					const float min_widget_width = ImGui::CalcTextSize("N: MMM\nR: MMM").x;
					for (int n = 0; n < 8; n++)
					{
						const float RAD_MIN = 5.0f;
						const float RAD_MAX = 70.0f;
						const float rad = RAD_MIN + (RAD_MAX - RAD_MIN) * (float)n / (8.0f - 1.0f);

						ImGui::BeginGroup();

						ImGui::Text("R: %.f\nN: %d", rad, draw_list->_CalcCircleAutoSegmentCount(rad));

						const float canvas_width = IM_MAX(min_widget_width, rad * 2.0f);
						const float offset_x = floorf(canvas_width * 0.5f);
						const float offset_y = floorf(RAD_MAX);

						const ImVec2 p1 = ImGui::GetCursorScreenPos();
						draw_list->AddCircle(ImVec2(p1.x + offset_x, p1.y + offset_y), rad, ImGui::GetColorU32(ImGuiCol_Text));
						ImGui::Dummy(ImVec2(canvas_width, RAD_MAX * 2));

						/*
						const ImVec2 p2 = ImGui::GetCursorScreenPos();
						draw_list->AddCircleFilled(ImVec2(p2.x + offset_x, p2.y + offset_y), rad, ImGui::GetColorU32(ImGuiCol_Text));
						ImGui::Dummy(ImVec2(canvas_width, RAD_MAX * 2));
						*/

						ImGui::EndGroup();
						ImGui::SameLine();
					}
					ImGui::EndTooltip();
				}
				ImGui::SameLine();
				HelpMarker("When drawing circle primitives with \"num_segments == 0\" tesselation will be calculated automatically.");

				ImGui::DragFloat("Global Alpha", &style.Alpha, 0.005f, 0.20f, 1.0f, "%.2f"); // Not exposing zero here so user doesn't "lose" the UI (zero alpha clips all widgets). But application code could have a toggle to switch between zero and non-zero.
				ImGui::DragFloat("Disabled Alpha", &style.DisabledAlpha, 0.005f, 0.0f, 1.0f, "%.2f"); ImGui::SameLine(); HelpMarker("Additional alpha multiplier for disabled items (multiply over current value of Alpha).");
				ImGui::PopItemWidth();

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	}
}

void GUI::editor() {
	auto& logic = Logic::get();

	auto& inputs = logic.get_inputs();

	static unsigned selectedInput = -1;
	static Frame newInput;

	const float editAreaHeight = 150.0f;

	ImGui::SetNextWindowSizeConstraints(ImVec2(255, 300), ImVec2(245 * ImGui::GetIO().FontGlobalScale, 300));

	if (ImGui::BeginTabItem("Editor")) {
		float firstChildHeight = 300;

		ImGui::Columns(2, "##Columns", false);

		static bool isOffsetSet = false;

		if (!isOffsetSet) {
			ImGui::SetColumnOffset(1, ImGui::GetColumnOffset(1) + 5);
			ImGui::SetColumnOffset(2, ImGui::GetColumnOffset(2) + 5);
			isOffsetSet = true;
		}

		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4 tempColor = style.Colors[ImGuiCol_Header];

		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Inputs");
		ImGui::BeginChild("##List", ImVec2(0, firstChildHeight - ImGui::GetFrameHeightWithSpacing() + 1), true);
		if (!inputs.empty()) {
			int closestFrameDiff = INT_MAX;
			int closestInputIndex = -1;

			for (unsigned i = 0; i < inputs.size(); i++) {
				int frameDiff = std::abs(static_cast<int>(inputs[i].number - logic.get_frame()));
				if (frameDiff < closestFrameDiff) {
					closestFrameDiff = frameDiff;
					closestInputIndex = i;
				}
			}

			for (unsigned i = 0; i < inputs.size(); i++) {
				ImVec4 color;
				if (inputs[i].isPlayer2) {
					color = player_2_button_color;
				}
				else {
					color = player_1_button_color;
				}

				if (i == closestInputIndex) {
					// Make the closest input's button brighter
					color.x = max(color.x - 0.5f, 0);
					color.y = max(color.y - 0.5f, 0);
					color.z = max(color.z - 0.5f, 0);
				}

				ImGui::PushStyleColor(ImGuiCol_Button, color);

				char buffer[100];
				std::sprintf(buffer, "%s at %d##Input", inputs[i].pressingDown ? "Click" : "Release", inputs[i].number);
				std::string text = buffer;

				if (ImGui::Button(text.c_str())) {
					selectedInput = i;
					newInput = inputs[i];
				}

				// Automatically select the closest input to the current frame
				if (PLAYLAYER && !PLAYLAYER->m_isPaused && (!logic.frame_advance || !logic.holding_frame_advance)) {
					if (closestInputIndex != -1) {
						selectedInput = closestInputIndex;
						newInput = inputs[closestInputIndex];

						// Calculate the scroll position to center the selected input
						int selectedIndex = closestInputIndex;
						int visibleInputs = ImGui::GetWindowHeight() / ImGui::GetItemRectSize().y;
						int firstVisibleIndex = max(0, selectedIndex - visibleInputs / 2);
						int scrollY = firstVisibleIndex * ImGui::GetItemRectSize().y + (ImGui::GetStyle().ItemSpacing.y * closestInputIndex); // DO NOT FUCK WITH THIS MATH DAWG IT TOOK ME 2 HOURS

						ImGui::SetScrollY(scrollY);
					}
				}

				ImGui::PopStyleColor();
			}
		}
		else {
			ImGui::TextColored(ImVec4(1, 1, 1, 1), "No Inputs");
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

		ImGui::NextColumn();

		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "CPS Breaks");
		ImGui::BeginChild("##List2", ImVec2(0, firstChildHeight), true);

		if (!logic.cps_over_percents.empty()) {
			if (logic.end_portal_position == 0) {
				ImGui::TextColored(ImVec4(1, 1, 1, 1), "Only Echo Replays support this");
			}
			else {
				for (unsigned i = 0; i < logic.cps_over_percents.size(); i++) {
					float val = logic.cps_over_percents[i].first;
					std::string rule = logic.cps_over_percents[i].second;
					std::string percent = std::to_string(val);

					// Truncate the string to have only 2 decimal places
					size_t dot_pos = percent.find(".");
					if (dot_pos != std::string::npos && dot_pos + 3 < percent.length()) {
						percent = percent.substr(0, dot_pos + 3);
					}

					printf("%s\n", rule.c_str());

					ImGui::Text("%s percent", percent.c_str());
					ImGui::SameLine();
					ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "#%s", rule.c_str());
				}
			}
		}
		else {
			ImGui::TextColored(ImVec4(1, 1, 1, 1), "No CPS Breaks");
		}


		ImGui::EndChild();
		ImGui::Columns(1);

		ImGui::Separator();
		ImGui::BeginChild("##EditArea", ImVec2(0, editAreaHeight), true);
		if (selectedInput >= 0 && selectedInput < inputs.size()) {
			ImGui::Text("Editing Input %d", selectedInput + 1);
			ImGui::InputInt("Frame", (int*)&newInput.number);
			ImGui::Checkbox("Down", &newInput.pressingDown);
			ImGui::Checkbox("Player 2", &newInput.isPlayer2);
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
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Replay Editor");

		ImGui::PushItemFlag(ImGuiItemFlags_NoTabStop, true);
		ImGui::SetNextItemWidth((ImGui::GetWindowContentRegionWidth() - ImGui::GetStyle().ItemSpacing.x) * 0.3f);
		ImGui::InputInt("###frames", &offset_frames, 0, 0); ImGui::SameLine();
		ImGui::PopItemFlag();

		ImGui::SameLine();

		if (ImGui::Button("Offset Frames")) {
			logic.offset_frames(offset_frames);
		}

		ImGui::SameLine();

		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(?)");

		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Offsets all frames in replay");
		}

		if (ImGui::Button("Random Offset Frames")) {
			logic.offset_inputs(-offset_frames, offset_frames);
			offset_frames = 0;
		}

		ImGui::SameLine();

		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(?)");

		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Randomly offsets all frames in replay from -input to +input");
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

		ImGui::InputText("VF Args", &logic.recorder.m_vf_args);

		ImGui::InputText("Extra Audio Args", &logic.recorder.m_extra_audio_args);

		ImGui::InputFloat("Extra Time", &logic.recorder.m_after_end_duration);

		ImGui::SameLine();

		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(?)");

		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Extra Time is the time in seconds after you finish a level that the recorder automatically records for.");
		}

		ImGui::Separator();

		ImGui::InputText("Video Name", &logic.recorder.video_name);

		ImGui::Checkbox("Color Fix", &logic.recorder.color_fix); ImGui::SameLine();

		HelpMarker("The color fix vf args are: colorspace=all=bt709:iall=bt470bg:fast=1");

		ImGui::SameLine();

		ImGui::Checkbox("Scroll Speed Bugfix", &logic.recorder.color_fix); ImGui::SameLine();

		HelpMarker("Syncs the video with the song when scroll speed desyncs them.");

		ImGui::Checkbox("Real Time Rendering", &logic.recorder.real_time_rendering); ImGui::SameLine();

		HelpMarker("Real Time Rendering is a technique used to render videos in real-time or close to real-time.\nThis means that, while rendering, the game does not slow down as much.\nNote: This may add visual bugs to the final video.");

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

	if (ImGui::BeginTabItem("Sequence")) {

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

		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Macro Tools");
		ImGui::Separator();

		if (ImGui::Button("Merge With Replay")) {
			ImGuiFileDialog::Instance()->OpenDialog("MergeReplay", "Choose File", ".echo,.bin", ".echo/");
		}

		if (ImGuiFileDialog::Instance()->Display("MergeReplay", ImGuiWindowFlags_NoCollapse, ImVec2(500, 200)))
		{
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
				std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
				std::vector<Frame> before_inputs = logic.get_inputs();

				std::string suffix = ".bin";
				if (filePathName.size() >= suffix.size() && filePathName.rfind(suffix) == (filePathName.size() - suffix.size()))
					logic.read_file(filePathName, true);
				else
					logic.read_file_json(filePathName, true);

				logic.inputs.insert(logic.inputs.end(), before_inputs.begin(), before_inputs.end());

				logic.sort_inputs();
			}
			ImGuiFileDialog::Instance()->Close();
		}

		ImGui::Checkbox("Click Both Players", &logic.click_both_players);
		ImGui::SameLine();
		ImGui::Checkbox("Swap Player Input", &logic.swap_player_input);

		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Game Modifications");
		ImGui::Separator();

		bool antiCheatBypass = anticheatBypass.isActivated();
		if (ImGui::Checkbox("Disable Anticheat", &antiCheatBypass)) {
			antiCheatBypass ? anticheatBypass.activate() : anticheatBypass.deactivate();
		}

		ImGui::Checkbox("Noclip Player 1", &logic.noclip_player1);
		ImGui::SameLine();
		ImGui::Checkbox("Noclip Player 2", &logic.noclip_player2);

		bool practiceActivated = practiceMusic.isActivated();
		if (ImGui::Checkbox("Overwrite Practice Music", &practiceActivated)) {
			if (practiceActivated) {
				practiceMusic.activate();
			}
			else {
				practiceMusic.deactivate();
			}
		}

		bool noEscActivated = noEscape.isActivated();
		if (ImGui::Checkbox("Ignore Escape", &noEscActivated)) {
			if (noEscActivated) {
				noEscape.activate();
			}
			else {
				noEscape.deactivate();
			}
		}

		if (ImGui::Checkbox("Modify Respawn Time", &logic.respawn_time_modified)) {
			if (logic.respawn_time_modified) {
				Opcode opcode(Cheat::AntiCheatBypass);
				opcode.ModifyFloatAtOffset(0x20A677, logic.speedhack);
			}
			else {
				Opcode opcode(Cheat::AntiCheatBypass);
				opcode.ModifyFloatAtOffset(0x20A677, 1.0f);
			}
		}

		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Autoclicker");
		ImGui::Separator();

		ImGui::Checkbox("Enabled", &logic.autoclicker);

		ImGui::SameLine();

		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(?)");

		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Keybind for this setting is 'B'");
		}

		ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.8f), "Intervals Between Actions");

		ImGui::PushItemWidth(ImGui::GetWindowContentRegionWidth() * 0.33f);

		int framesBetweenPresses = Autoclicker::get().getFramesBetweenPresses();
		if (ImGui::DragInt("Presses", &framesBetweenPresses, 1.0f, 0, 10000)) {
			Autoclicker::get().setFramesBetweenPresses(framesBetweenPresses);
		}

		ImGui::PopItemWidth();

		ImGui::SameLine();

		ImGui::PushItemWidth(ImGui::GetWindowContentRegionWidth() * 0.33f);

		//ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.8f), "Frames Between Releases");
		int framesBetweenReleases = Autoclicker::get().getFramesBetweenReleases();
		if (ImGui::DragInt("Releases", &framesBetweenReleases, 1.0f, 0, 10000)) {
			Autoclicker::get().setFramesBetweenReleases(framesBetweenReleases);
		}

		ImGui::PopItemWidth();

		ImGui::Checkbox("Click for Player 1", &logic.autoclicker_player_1);
		ImGui::SameLine();
		ImGui::Checkbox("Click for Player 2", &logic.autoclicker_player_2);

		ImGui::Checkbox("Auto Disable", &logic.autoclicker_auto_disable);

		ImGui::SameLine();

		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(?)");

		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Automatically disables the autoclicker in X frames");
		}

		if (!logic.autoclicker_auto_disable) {
			ImGui::BeginDisabled();
			ImGui::DragInt("Frames", &logic.autoclicker_disable_in, 1.0f, 1, 10000);
			ImGui::EndDisabled();
		}
		else {
			ImGui::DragInt("Frames", &logic.autoclicker_disable_in, 1.0f, 1, 10000);
		}

		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Frame Advance");
		ImGui::Separator();

		ImGui::DragInt("Hold Time (ms)", (int*)&logic.frame_advance_hold_duration, 1, 0);
		ImGui::DragInt("Delay Time (ms)", (int*)&logic.frame_advance_delay, 1, 0);

		ImGui::Separator();

		ImGui::SetNextItemWidth(get_width(50.f));

		if (logic.format == logic.SIMPLE) ImGui::BeginDisabled();

		if (ImGui::Button("Open Debug Console")) {
			// Allows for debugging, can be removed later
			AllocConsole();
			freopen("CONOUT$", "w", stdout);  // Redirects stdout to the new console
			std::printf("Opened Debugging Console");
		}

		if (logic.format == logic.SIMPLE) ImGui::EndDisabled();

		if (ImGui::Checkbox("Save Debug Info", &logic.save_debug_info)) {
			if (logic.save_debug_info) {
				logic.format = logic.DEBUG;
			}
			else {
				logic.format = logic.SIMPLE;
			}
		}

		ImGui::EndTabItem();
	}
}

std::map<std::string, std::shared_ptr<Convertible>> options = {
	{"TASBot", std::make_shared<TASBot>()},
	{"Osu", std::make_shared<Osu>()},
	{"MHR", std::make_shared<MHR>()},
	{"Plain Text", std::make_shared<PlainText>()}
};

void GUI::conversion() {
	auto& logic = Logic::get();

	if (ImGui::BeginTabItem("Conversion")) {
		if (ImGuiFileDialog::Instance()->IsOpened("ConversionImport"))
			ImGui::BeginDisabled();

		static std::string current_option = "Plain Text";
		if (ImGui::BeginCombo("Options", current_option.c_str())) {
			for (auto& option : options) {
				bool is_selected = (current_option == option.first);
				if (ImGui::Selectable(option.first.c_str(), is_selected)) {
					current_option = option.first;
				}

				if (is_selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		if (ImGuiFileDialog::Instance()->IsOpened("ConversionImport"))
			ImGui::EndDisabled();

		ImGui::SameLine();

		auto& style = ImGui::GetStyle();

		if (current_option == "Osu") {
			ImGui::BeginDisabled();

			if (ImGui::Button("Import"))
				ImGuiFileDialog::Instance()->OpenDialog("ConversionImport", "Choose File", options[current_option]->get_type_filter().c_str(), options[current_option]->get_directory().c_str());

			ImGui::EndDisabled();
		}
		else {
			if (ImGui::Button("Import"))
				ImGuiFileDialog::Instance()->OpenDialog("ConversionImport", "Choose File", options[current_option]->get_type_filter().c_str(), options[current_option]->get_directory().c_str());
		}

		ImGui::SameLine();

		if (ImGui::Button("Export")) {
			std::string filename = logic.macro_name;
			try {
				options[current_option]->export_to(filename);
				logic.conversion_message = "Success! Exported " + filename + " as a " + options[current_option]->get_type_filter() + " file";
			}
			catch (std::runtime_error& e) {
				logic.conversion_message = "Error! Could not export as " + options[current_option]->get_type_filter();
			}
		}

		if (ImGuiFileDialog::Instance()->Display("ConversionImport", ImGuiWindowFlags_NoCollapse, ImVec2(500, 200)))
		{
			// action if OK
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				logic.conversion_message = "";
				try {
					options[current_option]->import(ImGuiFileDialog::Instance()->GetFilePathName());
					logic.sort_inputs();
					if (logic.conversion_message == "")
						logic.conversion_message = "Success! Imported replay as a " + current_option + " file";
				}
				catch (std::runtime_error& e) {
					logic.conversion_message = "Error! Could not import " + ImGuiFileDialog::Instance()->GetFilePathName();
				}
				CCDirector::sharedDirector()->setAnimationInterval(1.f / GUI::get().input_fps);
			}

			ImGuiFileDialog::Instance()->Close();
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", logic.conversion_message);

		ImGui::EndTabItem();
	}
}

void GUI::main() {
	auto& logic = Logic::get();
	const ImVec2 buttonSize = { get_width(48), 0 };
	const ImVec2 fullWidth = { get_width(100), 0 };

	if (ImGui::BeginTabItem("Main")) {
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Recording Controls");
		ImGui::Separator();

		ImGuiStyle& style = ImGui::GetStyle();

		ImVec4 tempColor = style.Colors[ImGuiCol_Button];
		ImVec4 tempColor2 = style.Colors[ImGuiCol_ButtonHovered];
		ImVec4 tempColor3 = style.Colors[ImGuiCol_ButtonActive];

		if (logic.is_recording()) {
			style.Colors[ImGuiCol_Button] = ImVec4(0.6f, 0.6f, 0.6f, 0.2f);
			style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.6f, 0.6f, 0.6f, 0.3f);
			style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.6f, 0.6f, 0.6f, 0.4f);
		}

		if (ImGui::Button(logic.is_recording() ? "Stop Recording" : "Start Recording", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 0))) {
			logic.toggle_recording();
			logic.sort_inputs();
		}

		if (logic.is_recording()) {
			style.Colors[ImGuiCol_Button] = tempColor;
			style.Colors[ImGuiCol_ButtonHovered] = tempColor2;
			style.Colors[ImGuiCol_ButtonActive] = tempColor3;
		}

		ImGui::SameLine();

		if (logic.is_playing()) {
			ImVec4 tempColor = style.Colors[ImGuiCol_Button];
			ImVec4 tempColor2 = style.Colors[ImGuiCol_ButtonHovered];
			ImVec4 tempColor3 = style.Colors[ImGuiCol_ButtonActive];
			style.Colors[ImGuiCol_Button] = ImVec4(0.6f, 0.6f, 0.6f, 0.2f);
			style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.6f, 0.6f, 0.6f, 0.3f);
			style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.6f, 0.6f, 0.6f, 0.4f);
		}

		if (ImGui::Button(logic.is_playing() ? "Stop Playing" : "Start Playing", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.48f, 0))) {
			logic.toggle_playing();
			logic.sort_inputs();
		}

		if (logic.is_playing()) {
			style.Colors[ImGuiCol_Button] = tempColor;
			style.Colors[ImGuiCol_ButtonHovered] = tempColor2;
			style.Colors[ImGuiCol_ButtonActive] = tempColor3;
		}

		// Revert ImGui style back to default
		style.Colors[ImGuiCol_Button] = tempColor;
		style.Colors[ImGuiCol_ButtonHovered] = tempColor2;
		style.Colors[ImGuiCol_ButtonActive] = tempColor3;

		ImGui::Spacing();

		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Playback Settings");
		ImGui::Separator();

		ImGui::Text("Replay FPS: %f", logic.get_fps());
		ImGui::Text("Frame: %i", logic.get_frame());
		ImGui::Text("Highest CPS: %s", logic.highest_cps_cached().c_str());


		ImGui::PushItemFlag(ImGuiItemFlags_NoTabStop, true);
		ImGui::SetNextItemWidth(get_width(30));
		ImGui::InputFloat("###fps", &input_fps, 0, 0, "%.0f");
		ImGui::PopItemFlag();

		ImGui::SameLine();

		if (ImGui::Button("Set FPS")) {
			if (!logic.is_recording() && !logic.is_playing()) {
				logic.fps = input_fps;
				CCDirector::sharedDirector()->setAnimationInterval(1.f / logic.fps);
			}
			else {
				CCDirector::sharedDirector()->setAnimationInterval(1.f / input_fps);
			}
		}


		float speed = logic.speedhack;
		if (ImGui::DragFloat("Speed", &speed, 0.01, 0.01f, 100.f, "%.2f")) {
			if (speed > 0) logic.speedhack = speed;
			/*if (logic.respawn_time_modified) {
				Opcode opcode(Cheat::AntiCheatBypass);
				opcode.ModifyFloatAtOffset(0x20A677, logic.speedhack);
			}*/
		}

		auto& audiospeedhack = AudiopitchHack::getInstance();
		bool isEnabled = audiospeedhack.isEnabled();
		if (ImGui::Checkbox("Audio Speedhack", &isEnabled)) {
			if (isEnabled) {
				audiospeedhack.setEnabled(true);
			}
			else {
				audiospeedhack.setEnabled(false);
			}
		}

		/*static std::string keyName = "[None]";
		if (ImGui::Button(keyName.c_str())) {
			keyName = "Press a key...";
		}
		
		if (ImGui::IsItemHovered())
		{
			for (int i = 0; i < ImGuiKey_COUNT; i++)
			{
				if (ImGui::IsKeyPressed(i))
				{
					keyName = ImGui::GetKeyName(i);
				}
			}
		}*/

		ImGui::Checkbox("Real Time Mode", &logic.real_time_mode);

		ImGui::Checkbox("No Input Overwrite", &logic.no_overwrite);

		ImGui::Checkbox("Ignore actions during playback", &logic.ignore_actions_at_playback);

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Macro Options");
		ImGui::Separator();

		ImGui::Checkbox("Show Frame", &logic.show_frame);

		if (logic.show_frame) {
			ImGui::SameLine();
			ImGui::PushItemWidth(75);
			ImGui::DragFloat("X###frame_x", &logic.frame_counter_x, 1);
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::PushItemWidth(75);
			ImGui::DragFloat("Y###frame_y", &logic.frame_counter_y, 1);
			ImGui::PopItemWidth();
		}

		ImGui::Checkbox("Show CPS", &logic.show_cps);

		if (logic.show_cps) {
			ImGui::SameLine();
			ImGui::PushItemWidth(75);
			ImGui::DragFloat("X###cps_x", &logic.cps_counter_x, 1);
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::PushItemWidth(75);
			ImGui::DragFloat("Y###cps_y", &logic.cps_counter_y, 1);
			ImGui::PopItemWidth();
		}

		ImGui::DragFloat("Max CPS", &logic.max_cps, 0.01, 1, 100, "%.2f");

		ImGui::Separator();

		ImGui::Text("Macro Size: %i", logic.get_inputs().size());

		ImGui::SameLine();

		bool open_modal = true;

		if (logic.inputs.empty()) ImGui::BeginDisabled();
		if (ImGui::Button("Reset Macro")) {
			logic.get_inputs().clear();
			//ImGui::OpenPopup("Confirm Reset");
			//ImGui::SetNextWindowPos({ ImGui::GetWindowWidth() / 2.f, ImGui::GetWindowHeight() / 2.f });
		}
		if (logic.inputs.empty()) ImGui::EndDisabled();

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


		ImGui::Checkbox("Use JSON", &logic.use_json_for_files);

		ImGui::SameLine();

		ImGui::Checkbox("Use File Dialog", &logic.file_dialog);

		ImGui::PushItemFlag(ImGuiItemFlags_NoTabStop, true);
		ImGui::SetNextItemWidth(get_width(75.f));
		ImGui::InputText("Macro Name", logic.macro_name, MAX_PATH);
		ImGui::PopItemFlag();

		ImGui::SetNextItemWidth((ImGui::GetWindowContentRegionWidth() - ImGui::GetStyle().ItemSpacing.x) * (50.f / 100.f));
		if (ImGui::Button("Save File", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 0))) {
			if (logic.use_json_for_files) {
				logic.write_file_json(logic.macro_name);
			}
			else {
				logic.write_file(logic.macro_name);
			}
		}

		ImGui::SameLine();

		ImGui::SetNextItemWidth(get_width(50.f));

		if (ImGui::Button("Load File", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.48f, 0))) {
			if (!logic.file_dialog) {
				if (logic.use_json_for_files) {
					logic.read_file_json(logic.macro_name, false);
				}
				else {
					logic.read_file(logic.macro_name, false);
				}
			}
			else {
				ImGuiFileDialog::Instance()->OpenDialog("ImportNormal", "Choose File", ".echo,.bin", ".echo/");
			}
			input_fps = logic.fps;
			CCDirector::sharedDirector()->setAnimationInterval(1.f / GUI::get().input_fps);
			logic.sort_inputs();
		}

		if (ImGuiFileDialog::Instance()->Display("ImportNormal", ImGuiWindowFlags_NoCollapse, ImVec2(500, 200)))
		{
			// action if OK
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				// Find the position of the last dot in the filename
				size_t dotPosition = ImGuiFileDialog::Instance()->GetFilePathName().find_last_of(".");

				// Extract the substring from the beginning of the filename till the dot
				std::string nameWithoutExtension = ImGuiFileDialog::Instance()->GetFilePathName().substr(0, dotPosition);

				std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
				std::string suffix = ".bin";
				if (filePathName.size() >= suffix.size() && filePathName.rfind(suffix) == (filePathName.size() - suffix.size()))
					logic.read_file(filePathName, true);
				else
					logic.read_file_json(filePathName, true);

				input_fps = logic.fps;
				CCDirector::sharedDirector()->setAnimationInterval(1.f / GUI::get().input_fps);

				strcpy(logic.macro_name, nameWithoutExtension.c_str());
			}
			ImGuiFileDialog::Instance()->Close();
		}

		/*
		bool useBinary = !logic.use_json_for_files;
		if (ImGui::Checkbox("Use Binary", &useBinary)) {
			logic.use_json_for_files = !useBinary;
		}*/

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