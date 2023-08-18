#include "gui.hpp"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cocos2d.h>
#include <imgui.h>
#include <MinHook.h>
#include <cmath>
#include <sstream>
#include <algorithm>
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
#include "../Logic/Conversions/mhr_json.h"
#include "../Logic/Conversions/mhr.h"
#include "../Logic/Conversions/json.h"
#include "../Logic/Conversions/osu.h"
#include "../Logic/Conversions/plaintext.h"
#include "../Logic/Conversions/zbotf.h"
#include "../Hooks/hooks.hpp"
#include <imgui_demo.cpp>
#include <format>
#include "../Logic/speedhack.h"
#include "implot/implot.h"
#include "implot/implot_internal.h"

double evalExpression(const std::vector<std::string>& tokens, std::map<std::string, double>& variables, int& index);

double evalExpression(const std::vector<std::string>& tokens, std::map<std::string, double>& variables, int& index);

std::vector<std::string> tokenizeExpression(const std::string& expression)
{
	std::vector<std::string> tokens;
	std::string token;
	for (char c : expression)
	{
		if (std::isspace(c))
		{
			if (!token.empty())
			{
				tokens.push_back(token);
				token.clear();
			}
		}
		else if (std::isalnum(c) || c == '.')
		{
			token += c;
		}
		else
		{
			if (!token.empty())
			{
				tokens.push_back(token);
				token.clear();
			}
			tokens.push_back(std::string(1, c));
		}
	}
	if (!token.empty())
	{
		tokens.push_back(token);
	}
	return tokens;
}

double evalVariable(const std::string& token, std::map<std::string, double>& variables)
{
	if (variables.find(token) != variables.end())
	{
		return variables[token];
	}
	else
	{
		std::cout << "Variable " << token << " is not defined. Enter a value for " << token << ": ";
		double value;
		std::cin >> value;
		variables[token] = value;
		return value;
	}
}

double evalFunction(const std::string& token, const std::vector<std::string>& tokens, std::map<std::string, double>& variables, int& index)
{
	if (token == "cos")
	{
		++index;
		double result = evalExpression(tokens, variables, index);
		return cos(result);
	}
	else if (token == "sqrt")
	{
		++index;
		double result = evalExpression(tokens, variables, index);
		return sqrt(result);
	}
	else
	{
		std::cout << "Unknown function: " << token << std::endl;
		return 0.0;
	}
}

double evalOperator(const std::string& token, double left, double right)
{
	if (token == "+")
	{
		return left + right;
	}
	else if (token == "-")
	{
		return left - right;
	}
	else if (token == "*")
	{
		return left * right;
	}
	else if (token == "/")
	{
		return left / right;
	}
	else if (token == "^")
	{
		return pow(left, right);
	}
	else
	{
		std::cout << "Unknown operator: " << token << std::endl;
		return 0.0;
	}
}

double evalExpression(const std::vector<std::string>& tokens, std::map<std::string, double>& variables, int& index)
{
	double result = 0.0;
	while (index < tokens.size())
	{
		const std::string& token = tokens[index];
		++index;

		if (std::isdigit(token[0]))
		{
			result = std::stod(token);
		}
		else if (std::isalpha(token[0]))
		{
			if (tokens[index] == "(") // Function call
			{
				result = evalFunction(token, tokens, variables, index);
			}
			else // Variable
			{
				result = evalVariable(token, variables);
			}
		}
		else if (token == "(")
		{
			result = evalExpression(tokens, variables, index);
		}
		else if (token == ")")
		{
			return result;
		}
		else if (token == "+" || token == "-" || token == "*" || token == "/" || token == "^")
		{
			double right = evalExpression(tokens, variables, index);
			result = evalOperator(token, result, right);
		}
	}
	return result;
}

std::string echo_version = "Echo v1.0";

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
static Opcode practiceMusic(Cheat::PracticeMusic);
static Opcode noEscape(Cheat::NoESC);

#define CHECK_KEYBIND(label) \
do { \
    if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(1)) { \
        keybind_prompt = label; \
    } \
} while(0)

int g_has_imported = false; // ffs
int g_has_placed_positions = false; // ffs

void delay(int milliseconds) {
	auto start = std::chrono::steady_clock::now();
	auto end = start + std::chrono::milliseconds(milliseconds);

	while (std::chrono::steady_clock::now() < end) {
		// Do nothing, just wait
	}
}

template<typename T>
T checkProperty(const json& j, const std::string& key, const T& defaultValue) {
	if (j.contains(key) && !j[key].is_null()) {
		return j[key].get<T>();
	}
	return defaultValue;
}

void GUI::draw() {
	Logic::get().processKeybinds();
	if (!g_has_imported)
		import_theme(".echo\\settings\\theme.ui");
	g_has_imported = true;

	if (g_font) ImGui::PushFont(g_font);

	if (PLAYLAYER && Logic::get().is_recording()) {
		double frame_time = CCDirector::sharedDirector()->getAnimationInterval();
		Logic::get().total_recording_time += std::chrono::duration<double>(frame_time);
	}

	auto end = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - Logic::get().start);

	if (PLAYLAYER) {
		if (duration.count() >= Logic::get().frame_advance_hold_duration * Logic::get().speedhack && Logic::get().start != std::chrono::steady_clock::time_point()) {
			Logic::get().frame_advance = false;
			bool old_real_time = Logic::get().real_time_mode;
			Logic::get().real_time_mode = false;
			Hooks::CCScheduler_update_h(gd::GameManager::sharedState()->getScheduler(), 0, 1.f / Logic::get().fps);
			Logic::get().frame_advance = true;
			Logic::get().real_time_mode = old_real_time;
			delay(Logic::get().frame_advance_delay);
			Logic::get().holding_frame_advance = true;
		}
		else {
			Logic::get().holding_frame_advance = false;
		}
	}

	/*Speedhack::SetSpeed(1.f);
	gd::GameManager::sharedState()->getScheduler()->setTimeScale(Logic::get().speedhack);*/
	/*if (!Logic::get().is_recording() && !Logic::get().is_playing()) {
		Speedhack::SetSpeed(1.f);
		gd::GameManager::sharedState()->getScheduler()->setTimeScale(Logic::get().speedhack);
	}
	else {
		gd::GameManager::sharedState()->getScheduler()->setTimeScale(1.f);
		Speedhack::SetSpeed(Logic::get().speedhack);
	}*/

	if (show_window) {
		std::stringstream full_title;

		//full_title << "Echo [" << ECHO_VERSION << "b]";

		full_title << echo_version;
		
		ImGuiIO& io = ImGui::GetIO();

		io.ConfigFlags |= ImGuiConfigFlags_NavNoCaptureKeyboard;

		if (io.KeysDown[ImGui::GetKeyIndex(ImGuiKey_Escape)])
			ImGui::SetKeyboardFocusHere();

		if (!keybind_prompt.empty())
			show_keybind_prompt(keybind_prompt);

		bool open_modal = true;
		if (Logic::get().error != "") {
			ImGui::OpenPopup("Error");

			if (ImGui::BeginPopupModal("Error", &open_modal, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize)) {

				ImGui::Text("%s", Logic::get().error);

				if (ImGui::Button("Okay", ImVec2(ImGui::GetWindowContentRegionWidth(), 0))) {
					Logic::get().error = "";
				}

				ImGui::EndPopup();
			}
		}

		if (ImGuiFileDialog::Instance()->Display("ThemeImport", ImGuiWindowFlags_NoCollapse, ImVec2(500, 200)))
		{
			// action if OK
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				try {
					import_theme(ImGuiFileDialog::Instance()->GetFilePathName());
				}
				catch (std::runtime_error& e) {
					printf("%s", e.what());
				}
			}
			ImGuiFileDialog::Instance()->Close();
		}

		//io.FontGlobalScale = io.DisplaySize.x / 2000; // wee bit bigger than 1920

		if (docked) {
			ImGui::Begin(full_title.str().c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_SaveSettings)) {
				main();
				tools();
				editor();
				renderer();
				sequential_replay();
				ui_editor();
				clickbot();
			}
			ImGui::End();
		}
		else { // ?? profit
			if (!g_has_placed_positions) {
				ImGui::SetNextWindowPos(main_pos);
				main();
				ImGui::SetNextWindowPos(tools_pos);
				tools();
				ImGui::SetNextWindowPos(editor_pos);
				editor();
				ImGui::SetNextWindowPos(render_pos);
				renderer();
				ImGui::SetNextWindowPos(sequence_pos);
				sequential_replay();
				ImGui::SetNextWindowPos(style_pos);
				ui_editor();
				ImGui::SetNextWindowPos(clickbot_pos);
				clickbot();
				g_has_placed_positions = true;
			}
			else {
				main();
				tools();
				editor();
				renderer();
				sequential_replay();
				ui_editor();
				clickbot();
			}
		}
	}

	/*auto end = std::chrono::steady_clock::now();
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
	}*/

	if (g_font) ImGui::PopFont();
}

float get_width(float percent) {
	return (ImGui::GetWindowContentRegionWidth() - ImGui::GetStyle().ItemSpacing.x) * (percent / 100.f);
}

void GUI::import_theme(std::string path) {
	std::ifstream file(path);

	ImGuiStyle& style = ImGui::GetStyle();

	if (!file.is_open())
	{
		printf("Failed to open theme.ui for reading");
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
		printf("Failed to parse theme.ui: %s", e.what());
		file.close();
		return;
	}

	file.close();

	// Import ImGui style variables
	ImGui::GetIO().FontGlobalScale = json["GlobalScale"];
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
	style.WindowPadding.x = checkProperty(json, "WindowPaddingX", 8);
	style.WindowPadding.y = checkProperty(json, "WindowPaddingY", 8);
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
	style.Alpha = min(15, json["Alpha"]);
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
	if (json.contains("popup_bg_color")) {
		auto colorArray = json["popup_bg_color"];
		ImVec4 color(colorArray[0], colorArray[1], colorArray[2], colorArray[3]);
		popup_bg_color = color;
	}
	style.Colors[ImGuiCol_ModalWindowDimBg] = popup_bg_color;

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

	std::cout << "Theme imported from theme.ui" << std::endl;
}

void GUI::export_theme(std::string path, bool custom_path) {
	ImGuiStyle& style = ImGui::GetStyle();
	nlohmann::json json;

	// Fill the json object with the variables
	json["GlobalScale"] = ImGui::GetIO().FontGlobalScale;
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
	json["WindowPaddingX"] = style.WindowPadding.x;
	json["WindowPaddingY"] = style.WindowPadding.y;
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
	json["Alpha"] = min(15, style.Alpha);
	json["DisabledAlpha"] = style.DisabledAlpha;

	for (int i = 0; i < ImGuiCol_COUNT; i++)
	{
		ImVec4 color = ImGui::GetStyleColorVec4(i);
		std::string colorName = ImGui::GetStyleColorName(i);
		json[colorName] = { color.x, color.y, color.z, color.w };
	}

	json["player_1_button_color"] = { player_1_button_color.x, player_1_button_color.y, player_1_button_color.z, player_1_button_color.w };
	json["player_2_button_color"] = { player_2_button_color.x, player_2_button_color.y, player_2_button_color.z, player_2_button_color.w };
	json["popup_bg_color"] = { popup_bg_color.x, popup_bg_color.y, popup_bg_color.z, popup_bg_color.w };

	// Save the json object to a file
	std::string name = "";
	if (!custom_path) {
		name += ".echo\\themes\\";
		name += path;
		name += ".ui";
	}
	else {
		name = path;
	}

	std::ofstream file(name);
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

void MyColorPicker(ImVec4& color, const char* name)
{
	if (ImGui::ColorButton(name, color))
	{
		ImGui::OpenPopup("ColorPickerPopup");
	}

	if (ImGui::BeginPopup("ColorPickerPopup"))
	{
		ImGui::ColorPicker4(name, (float*)&color);
		ImGui::EndPopup();
	}
}

void GUI::ui_editor() {
	ImGuiStyle& style = ImGui::GetStyle();

	float tabWidth = ImGui::GetWindowContentRegionWidth() / 6.0f;
	if (docked)
		ImGui::SetNextItemWidth(tabWidth);

	if (ImGui::BeginTabItem("Theme") || !docked) {
		static float window_scale = 1.0f;

		if (!docked) {
			ImGui::Begin("Theme", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			style_pos = ImGui::GetWindowPos();
		}

		ImGui::PushItemWidth(150);
		ImGui::InputText("", theme_name, MAX_PATH);
		ImGui::PopItemWidth();

		ImGui::SameLine();

		if (ImGui::Button("Save Theme")) {
			export_theme(theme_name);
		}

		ImGui::SameLine();

		if (ImGui::Button("Load Theme")) {
			ImGuiFileDialog::Instance()->OpenDialog("ThemeImport", "Choose File", ".ui", ".echo/themes/");
		}

		ImGui::Checkbox("Docked Menu", &docked); ImGui::SameLine();

		ImGuiIO& io = ImGui::GetIO();

		ImGui::PushItemWidth(150);

		ImGui::DragFloat("Global Scale", &io.FontGlobalScale, 0.01, 0.01, 3.0, "%.1f");

		ImGui::PopItemWidth();

		ImGui::Separator();

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

		float tabWidth = ImGui::GetWindowContentRegionWidth() / 3.0f;
		if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_None))
		{

			if (ImGui::BeginTabItem("Sizes"))
			{

				//ImGui::SetNextWindowSizeConstraints(ImVec2(-1, 350 * io.FontGlobalScale * 2), ImVec2(-1, 350 * io.FontGlobalScale * 2));
				ImGui::BeginChild("##sizes", ImVec2(-1, 350 * io.FontGlobalScale * 2.f), true, ImGuiWindowFlags_AlwaysAutoResize);
				ImGui::Text("Main");
				ImGui::PushItemWidth(200);
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
				ImGui::DragFloat2("WindowTitleAlign", (float*)&style.WindowTitleAlign, 0.01f, 0.0f, 1.0f, "%.2f");
				int window_menu_button_position = style.WindowMenuButtonPosition + 1;
				if (ImGui::Combo("WindowBtnPos", (int*)&window_menu_button_position, "None\0Left\0Right\0"))
					style.WindowMenuButtonPosition = window_menu_button_position - 1;
				ImGui::Combo("ColorButtonPosition", (int*)&style.ColorButtonPosition, "Left\0Right\0");
				ImGui::DragFloat2("ButtonTextAlign", (float*)&style.ButtonTextAlign, 0.01f, 0.0f, 1.0f, "%.2f");
				ImGui::SameLine(); HelpMarker("Alignment applies when a button is larger than its text content.");
				ImGui::DragFloat2("SelectTextAlign", (float*)&style.SelectableTextAlign, 0.01f, 0.0f, 1.0f, "%.2f");
				ImGui::SameLine(); HelpMarker("Alignment applies when a selectable is larger than its text content.");
				ImGui::Text("Safe Area Padding");
				ImGui::SameLine(); HelpMarker("Adjust if you cannot see the edges of your screen (e.g. on a TV where scaling has not been configured).");
				ImGui::SliderFloat2("SafePadding", (float*)&style.DisplaySafeAreaPadding, 0.0f, 30.0f, "%.0f");

				ImGui::PopItemWidth();

				ImGui::EndChild();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Colors"))
			{
				static ImGuiTextFilter filter;
				filter.Draw("Filter colors", ImGui::GetFontSize() * 16);

				static ImGuiColorEditFlags alpha_flags = 0;

				//ImGui::SetNextWindowSizeConstraints(ImVec2(500, 500), ImVec2(500, 500));
				ImGui::BeginChild("##colors", ImVec2(-1, 350 * io.FontGlobalScale * 2.f), true, ImGuiWindowFlags_AlwaysAutoResize);
				ImGui::PushItemWidth(250);
				if (filter.PassFilter("Player 1 In Editor")) {
					ImGui::PushID(998);
					MyColorPicker(player_1_button_color, "Player 1 In Editor");
					ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
					ImGui::TextUnformatted("Player 1 In Editor");
					ImGui::PopID();
				}
				if (filter.PassFilter("Player 2 In Editor")) {
					ImGui::PushID(999);
					MyColorPicker(player_2_button_color, "Player 2 In Editor");
					ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
					ImGui::TextUnformatted("Player 2 In Editor");
					ImGui::PopID();
				}

				//style.Colors[ImGuiCol_ModalWindowDimBg] = popup_bg_color;

				/*if (filter.PassFilter("PopupBg")) {
					ImGui::PushID(1000);
					MyColorPicker(popup_bg_color, "PopupBg");
					ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
					ImGui::TextUnformatted("PopupBg");
					ImGui::PopID();
				}*/

				for (int i = 0; i < ImGuiCol_COUNT; i++)
				{
					const char* name = ImGui::GetStyleColorName(i);
					if (!filter.PassFilter(name))
						continue;
					ImGui::PushID(i);
					MyColorPicker(style.Colors[i], name);
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
		
		if (docked)
			ImGui::EndTabItem();
		else
			ImGui::End();
	}
}

bool compareInputVectors(const std::vector<Frame>& v1, const std::vector<Frame>& v2) {
	if (v1.size() != v2.size()) {
		return false; // Vectors have different sizes, they are not equal
	}

	for (size_t i = 0; i < v1.size(); i++) {
		// Compare each element of the vectors
		if (v1[i].number != v2[i].number ||
			v1[i].pressingDown != v2[i].pressingDown ||
			v1[i].isPlayer2 != v2[i].isPlayer2) {
			return false; // Elements are different, vectors are not equal
		}
	}

	return true; // All elements are equal, vectors are equal
}

std::string getOrdinalSuffix(int number) {
	std::string suffix;
	int lastDigit = number % 10;
	int secondLastDigit = (number / 10) % 10;

	if (number <= 0) {
		return "No";
	}

	// Check for special cases where the suffix is "th"
	if (secondLastDigit == 1 || lastDigit > 3 || lastDigit == 0) {
		suffix = "th";
	}
	else if (lastDigit == 1) {
		suffix = "st";
	}
	else if (lastDigit == 2) {
		suffix = "nd";
	}
	else if (lastDigit == 3) {
		suffix = "rd";
	}

	// Append the suffix to the number
	std::string result = std::to_string(number) + suffix;
	return result;
}

void GUI::editor() {
	auto& logic = Logic::get();

	auto& inputs = logic.get_inputs();

	static int selectedInput = -1;
	static Frame newInput;

	const float editAreaHeight = 150.0f;

	static bool plaintext_editing = false;

	float tabWidth = ImGui::GetWindowContentRegionWidth() / 6.0f;
	if (docked)
		ImGui::SetNextItemWidth(tabWidth);

	if (ImGui::BeginTabItem("Editor") || !docked) {
		std::stringstream ss;
		ss << logic.fps << "\n";
		for (unsigned i = 0; i < inputs.size(); i++) {
			ss << inputs[i].number << " " << inputs[i].pressingDown << " " << inputs[i].isPlayer2;
			if (i != inputs.size() - 1) ss << "\n";
		}

		if (!docked) {
			ImGui::SetNextWindowSizeConstraints(ImVec2(450 * ImGui::GetIO().FontGlobalScale * 2.f, 415 * ImGui::GetIO().FontGlobalScale * 2.f), ImVec2(550 * ImGui::GetIO().FontGlobalScale * 2.f, 515 * ImGui::GetIO().FontGlobalScale * 2.f));
			ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			editor_pos = ImGui::GetWindowPos();
		}
		float firstChildHeight = 300;

		std::string inputs_area_text = "Inputs";
		float halved_region_width = ImGui::GetWindowContentRegionWidth() * 0.5;
		float quarter_region_width = halved_region_width / 2.f;
		float inputs_area_text_width = quarter_region_width - (ImGui::CalcTextSize(inputs_area_text.c_str()).x / 2.f);

		ImGui::SetCursorPosX(inputs_area_text_width + ImGui::GetStyle().ItemSpacing.x);

		ImGui::Text("Inputs"); ImGui::SameLine();

		std::string edit_area_text = "Edit Area";
		float edit_area_text_width = (halved_region_width + quarter_region_width) - (ImGui::CalcTextSize(edit_area_text.c_str()).x / 2.f);
		float edit_area_x = (ImGui::GetWindowContentRegionWidth() + edit_area_text_width) * 0.5;

		ImGui::SetCursorPosX(edit_area_text_width + ImGui::GetStyle().ItemSpacing.x);

		ImGui::Text("Edit Area");

		ImGui::Separator();

		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4 tempColor = style.Colors[ImGuiCol_Header];

		float list_child_width = ImGui::CalcItemWidth();

		const ImVec2 oldFramePadding = style.FramePadding;

		ImGui::BeginChild("##List", ImVec2(ImGui::GetContentRegionAvail().x * 0.5, (firstChildHeight - ImGui::GetFrameHeightWithSpacing() + 1) * ImGui::GetIO().FontGlobalScale * 2.f), true, ImGuiWindowFlags_AlwaysUseWindowPadding);
		
		if (reload_inputs) {
			selectedInput = -1;
			reload_inputs = false;
		}

		if (!inputs.empty() && !plaintext_editing) {
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

				if (ImGui::Button(text.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
					selectedInput = i;
					newInput = inputs[i];
				}

				// Automatically select the closest input to the current frame
				if (PLAYLAYER && !PLAYLAYER->m_bIsPaused && (!logic.frame_advance || !logic.holding_frame_advance) && editor_auto_scroll) {
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
		else if (plaintext_editing) {
			selectedInput = -1;
			strcpy(inputTextBuffer, ss.str().c_str());
			if (ImGui::InputTextMultiline("##InputConfig", inputTextBuffer, sizeof(inputTextBuffer), ImVec2(-1.f, -1.f), ImGuiInputTextFlags_AllowTabInput)) {
				std::stringstream ss(inputTextBuffer);
				std::string line;
				std::vector<Frame> newInputs;

				if (std::getline(ss, line)) {
					logic.fps = std::stoi(line);
				}

				while (std::getline(ss, line)) {
					std::stringstream lineSS(line);
					Frame input;

					lineSS >> input.number >> input.pressingDown >> input.isPlayer2;

					newInputs.push_back(input);
				}

				inputs = std::move(newInputs);
			}
		}
		else {
			selectedInput = -1;
			ImGui::Text("No Inputs");
		}

		ImGui::EndChild();
		ImGui::SameLine();

		ImGui::BeginChild("##EditArea", ImVec2(0, (firstChildHeight - ImGui::GetFrameHeightWithSpacing() + 1) * ImGui::GetIO().FontGlobalScale * 2.f), true, ImGuiWindowFlags_AlwaysUseWindowPadding);

		if (selectedInput >= 0 && selectedInput < inputs.size() && !plaintext_editing) {
			ImGui::Text("%s Input Selected", getOrdinalSuffix(selectedInput + 1).c_str());
			ImGui::PushItemWidth(150);
			ImGui::InputInt("Frame", (int*)&newInput.number);
			ImGui::PopItemWidth();
			ImGui::Checkbox("Down", &newInput.pressingDown);
			ImGui::SameLine();
			ImGui::Checkbox("Player 2", &newInput.isPlayer2);
			if (ImGui::Button("Move Up", ImVec2(ImGui::GetContentRegionAvail().x * 0.5, 0)) && selectedInput > 0) {
				std::swap(inputs[selectedInput], inputs[selectedInput - 1]);
				selectedInput--;
			}
			ImGui::SameLine();
			if (ImGui::Button("Move Down", ImVec2(ImGui::GetContentRegionAvail().x, 0)) && selectedInput < inputs.size() - 1) {
				std::swap(inputs[selectedInput], inputs[selectedInput + 1]);
				selectedInput++;
			}
			ImGui::Separator();
			ImGui::Text("Current Frame: %i", logic.get_frame());
			ImGui::Checkbox("Auto Scroll To Frame", &editor_auto_scroll);
			ImGui::Checkbox("Edit Plain Text", &plaintext_editing);
			inputs[selectedInput] = newInput;
		}
		else {
			ImGui::BeginDisabled();
			ImGui::Text("%s Input Selected", getOrdinalSuffix(selectedInput + 1).c_str());
			ImGui::PushItemWidth(150);
			int fake_frame_edit_value = 0;
			bool fake_down_edit_value = false;
			bool fake_player_edit_value = false;
			ImGui::InputInt("Frame", &fake_frame_edit_value);
			ImGui::PopItemWidth();
			ImGui::Checkbox("Down", &fake_down_edit_value);
			ImGui::SameLine();
			ImGui::Checkbox("Player 2", &fake_player_edit_value);
			if (ImGui::Button("Move Up", ImVec2(ImGui::GetContentRegionAvail().x * 0.5, 0)) && selectedInput > 0) {
				std::swap(inputs[selectedInput], inputs[selectedInput - 1]);
				selectedInput--;
			}
			ImGui::SameLine();
			if (ImGui::Button("Move Down", ImVec2(ImGui::GetContentRegionAvail().x, 0)) && selectedInput < inputs.size() - 1) {
				std::swap(inputs[selectedInput], inputs[selectedInput + 1]);
				selectedInput++;
			}
			ImGui::Separator();
			ImGui::EndDisabled();
			ImGui::Text("Current Frame: %i", logic.get_frame());
			ImGui::Checkbox("Auto Scroll To Frame", &editor_auto_scroll);
			ImGui::Checkbox("Edit Plain Text", &plaintext_editing);
		}

		ImGui::EndChild();

		style.FramePadding = oldFramePadding;

		if (ImGui::Button("Add Input", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5, 0))) {
			if (selectedInput == -1) {
				inputs.push_back(Frame());
				selectedInput = inputs.size() - 1;
			}
			else {
				inputs.insert(inputs.begin() + selectedInput, Frame());
			}
			newInput = Frame();
		}

		ImGui::SameLine();

		if (!(selectedInput >= 0 && selectedInput < inputs.size())) ImGui::BeginDisabled();

		if (ImGui::Button("Delete Input", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
			inputs.erase(inputs.begin() + selectedInput);
			if (selectedInput >= inputs.size()) selectedInput = inputs.size() - 1;
			newInput = inputs[selectedInput];
		}

		if (!(selectedInput >= 0 && selectedInput < inputs.size())) ImGui::EndDisabled();

		ImGui::Separator();

		ImGui::PushItemFlag(ImGuiItemFlags_NoTabStop, true);
		///ImGui::SetNextItemWidth((ImGui::GetWindowContentRegionWidth() - ImGui::GetStyle().ItemSpacing.x) * 0.3f);
		ImGui::PushItemWidth(150);
		ImGui::InputInt("###offset_frames", &offset_frames, 0, 0);
		ImGui::PopItemWidth();

		ImGui::SameLine();

		if (ImGui::Button("Offset Frames")) {
			logic.offset_frames(offset_frames);
		}

		ImGui::SameLine();

		ImGui::Text("(?)");

		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Offsets all frames in replay");
		}

		ImGui::SameLine();

		if (ImGui::Button("Random Offset")) {
			logic.offset_inputs(-offset_frames, offset_frames);
		}

		ImGui::SameLine();

		ImGui::Text("(?)");

		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Randomly offsets all frames in replay from -input to +input");
		}

		if (ImGui::Button("Remove Player 1 Inputs", ImVec2(ImGui::GetContentRegionAvail().x * 0.5, 0))) {
			logic.inputs.erase(std::remove_if(logic.inputs.begin(), logic.inputs.end(), [](const Frame& input) {
				return !input.isPlayer2;
				}), logic.inputs.end());
		}

		ImGui::SameLine();

		if (ImGui::Button("Remove Player 2 Inputs", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
			logic.inputs.erase(std::remove_if(logic.inputs.begin(), logic.inputs.end(), [](const Frame& input) {
				return input.isPlayer2;
				}), logic.inputs.end());
		}

		if (ImGui::Button("Flip Presses and Releases", ImVec2(ImGui::GetContentRegionAvail().x * 0.5, 0))) {
			for (auto& input : logic.inputs) {
				input.pressingDown = !input.pressingDown;
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Flip Player 1 and 2", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
			for (auto& input : logic.inputs) {
				input.isPlayer2 = !input.isPlayer2;
			}
		}

		if (docked)
			ImGui::EndTabItem();
		else
			ImGui::End();
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


	float tabWidth = ImGui::GetWindowContentRegionWidth() / 6.0f;
	if (docked)
		ImGui::SetNextItemWidth(tabWidth);

	if (ImGui::BeginTabItem("Render") || !docked) {

		if (!docked) {
			ImGui::Begin("Render", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			render_pos = ImGui::GetWindowPos();
		}

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

		ImGui::SetNextItemWidth(windowWidth / 4.f);
		ImGui::InputInt("Width", (int*)&logic.recorder.m_width, 0);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(windowWidth / 4.f);
		ImGui::InputInt("Height", (int*)&logic.recorder.m_height, 0);

		ImGui::SameLine();

		if (ImGui::Button("Set To Window Size", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
			auto windowSize = ImGui::GetIO().DisplaySize;
			auto windowX = windowSize.x;
			auto windowY = windowSize.y;

			logic.recorder.m_height = windowY;
			logic.recorder.m_width = windowX;
		}

		ImGui::Separator();

		bool open_settings_modal = true;
		bool open_extra_settings_modal = true;

		if (ImGui::Button("Settings", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 0))) {
			ImGui::OpenPopup("Render Settings");
		}

		ImGui::SameLine();

		if (ImGui::Button("Extra Settings", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
			ImGui::OpenPopup("Extra Render Settings");
		}

		if (ImGui::BeginPopupModal("Render Settings", &open_settings_modal, ImGuiWindowFlags_AlwaysAutoResize)) {

			ImGui::InputInt("Video FPS", (int*)&logic.recorder.m_fps);

			ImGui::InputText("Video Codec", &logic.recorder.m_codec);

			ImGui::InputInt("Bitrate (M)", &logic.recorder.m_bitrate);

			ImGui::Separator();

			ImGui::Checkbox("Color Fix", &logic.recorder.color_fix); ImGui::SameLine();

			HelpMarker("The color fix vf args are: colorspace=all=bt709:iall=bt470bg:fast=1");

			ImGui::SameLine();

			ImGui::Checkbox("Scroll Speed Bugfix", &logic.recorder.ssb_fix); ImGui::SameLine();

			HelpMarker("Syncs the video with the song when scroll speed desyncs them."); ImGui::SameLine();

			ImGui::Checkbox("Real Time Rendering", &logic.recorder.real_time_rendering); ImGui::SameLine();

			HelpMarker("Real Time Rendering is a technique used to render videos in real-time or close to real-time.\nThis means that, while rendering, the game does not slow down as much.\nNote: This may add visual bugs to the final video.");

			ImGui::EndPopup();
		}

		if (ImGui::BeginPopupModal("Extra Render Settings", &open_extra_settings_modal, ImGuiWindowFlags_AlwaysAutoResize)) {



			ImGui::Text("Audio");
			ImGui::Separator();

			ImGui::PushItemWidth(75);
			ImGui::DragFloat("Fade In Time###audio_in", &logic.recorder.a_fade_in_time, 0.01, 0, 10, "%.2f"); ImGui::SameLine();

			ImGui::DragFloat("Fade Out Time###audio_out", &logic.recorder.a_fade_out_time, 0.01, 0, 10, "%.2f");

			ImGui::Text("Video");
			ImGui::Separator();

			ImGui::DragFloat("Fade In Time###video_in", &logic.recorder.v_fade_in_time, 0.01, 0, 10, "%.2f"); ImGui::SameLine();

			ImGui::DragFloat("Fade Out Time###video_out", &logic.recorder.v_fade_out_time, 0.01, 0, 10, "%.2f");

			ImGui::PopItemWidth();
			ImGui::Separator();

			ImGui::InputText("Extra Args", &logic.recorder.m_extra_args);

			ImGui::InputText("VF Args", &logic.recorder.m_vf_args);

			ImGui::InputText("Extra Audio Args", &logic.recorder.m_extra_audio_args);

			ImGui::InputFloat("Extra Time", &logic.recorder.m_after_end_duration);

			ImGui::SameLine();

			HelpMarker("Extra Time is the time in seconds after you finish a level that the recorder automatically records for.");

			ImGui::EndPopup();
		}

		ImGui::Separator();

		ImGui::PushItemWidth(170);
		ImGui::InputText("Video Name", &logic.recorder.video_name);
		ImGui::PopItemWidth();

		ImGui::SameLine();

		if (PLAYLAYER) {
			if (!logic.recorder.m_recording) {
				if (ImGui::Button("Start Rendering", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
					logic.recorder.start(logic.recorder.directory + logic.recorder.video_name + logic.recorder.extension);
				}
			}
			else {
				if (ImGui::Button("Stop Rendering", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
					logic.recorder.stop();
				}
			}
		}
		else {
			ImGui::BeginDisabled();
			ImGui::Button("Start Recording", ImVec2(ImGui::GetContentRegionAvail().x, 0));
			ImGui::EndDisabled();
		}

		CHECK_KEYBIND("toggleRendering");

		if (docked)
			ImGui::EndTabItem();
		else
			ImGui::End();
	}
}

void GUI::sequential_replay() {
	auto& logic = Logic::get();

	static std::optional<size_t> selected_replay_index = std::nullopt;
	const float child_height = 175.0f;

	if (ImGui::IsKeyDown(KEY_Escape)) { //pls find a more intuitive way to do this
		selected_replay_index = std::nullopt;
	}

	float tabWidth = ImGui::GetWindowContentRegionWidth() / 6.0f;
	if (docked)
		ImGui::SetNextItemWidth(tabWidth);

	if (ImGui::BeginTabItem("Sequence") || !docked) {

		if (!docked) {
			ImGui::SetNextWindowSizeConstraints({ 375 * ImGui::GetIO().FontGlobalScale * 2.f, 330 * ImGui::GetIO().FontGlobalScale * 2.f }, { 375 * ImGui::GetIO().FontGlobalScale * 2.f, 350 * ImGui::GetIO().FontGlobalScale * 2.f });
			ImGui::Begin("Sequence", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

			sequence_pos = ImGui::GetWindowPos();
		}

		ImGui::Checkbox("Enable Sequence", &logic.sequence_enabled);

		if (ImGui::Button("Remove All")) {
			logic.replays.clear();
		}
		ImGui::SameLine();
		static int all_offset = 0;

		ImGui::PushItemWidth(175);
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

		ImGui::PopItemWidth();

		ImGui::BeginChild("##seq_replay_list", ImVec2(0, 150), true);
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

		//ImGui::BeginChild("##seq_replay_child", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysAutoResize); //idk how 2 name dis

		if (selected_replay_index.has_value() && selected_replay_index.value() < logic.replays.size()) {

			Replay& selected_replay = logic.replays[selected_replay_index.value()];

			ImGui::Text("Name: %s", &selected_replay.name);
			ImGui::SameLine();
			if (ImGui::Button("Remove")) {
				logic.replays.erase(logic.replays.begin() + selected_replay_index.value());
				selected_replay_index = std::nullopt;
			}
			ImGui::PushItemWidth(200);
			ImGui::InputInt("Max Offset", &selected_replay.max_frame_offset, 1, 5);
			ImGui::PopItemWidth();
			ImGui::Separator();
		}
			static std::string replay_name{};
			static int amount = 1;

			ImGui::PushItemWidth(200);
			ImGui::InputText("Name", &replay_name);
			ImGui::PopItemWidth();

			if (ImGui::Button("Add")) {
				//HACK: idk if i'm allowed to  reformat half of the stuff in logic so i'll just do this.
				//		if i am lmk bc it's kinda messy rn
				std::vector<Frame> old_actions = logic.get_inputs();

				logic.get_inputs().clear();

				//TODO: catch this or whatever
				logic.read_file(replay_name, false);

				if (!logic.inputs.empty() && logic.error.empty()) {
					for (int i = 0; i < amount; i++) {
						auto inputs = logic.get_inputs();
						logic.replays.push_back(Replay{
								replay_name,
								0,
								inputs
							});
					}
				}

				logic.get_inputs() = old_actions;
				//HACK END

				replay_name = {};
				amount = 1;
			}

			ImGui::SameLine();
			ImGui::PushItemWidth(200);
			ImGui::InputInt("Amount###macros", &amount);
			ImGui::PopItemWidth();

		//ImGui::EndChild();

		if (docked)
			ImGui::EndTabItem();
		else
			ImGui::End();
	}
}

static int selected_noclip = 0; // Index of "Both" option (auto selected)
static int selected_recording = 0; // Index of "Both" option (auto selected)
static int selected_playback = 0; // Index of "Both" option (auto selected)
static int selected_autoclicker_player = 2; // Index of "Both" option (auto selected)
static int selected_inverse_click = 0; // Index of "Both" option (auto selected)

static int selected_hitbox_color = 0;

std::map<std::string, std::shared_ptr<Convertible>> options = {
	{"TASBot", std::make_shared<TASBot>()},
	{"MHR JSON", std::make_shared<MHRJSON>()},
	{"MHR", std::make_shared<MHR>()},
	{"Plain Text", std::make_shared<PlainText>()},
	{"JSON", std::make_shared<JSON>()},
	{"ZBot Frame", std::make_shared<ZBF>()}
};

void GUI::tools() {
	auto& logic = Logic::get();

	float tabWidth = ImGui::GetWindowContentRegionWidth() / 6.0f;
	if (docked)
		ImGui::SetNextItemWidth(tabWidth);
	if (ImGui::BeginTabItem("Tools") || !docked) {

		if (!docked) {
			
			ImGui::Begin("Tools", nullptr , ImGuiWindowFlags_AlwaysAutoResize);
			tools_pos = ImGui::GetWindowPos();
			
		}

		if (ImGuiFileDialog::Instance()->IsOpened("ConversionImport"))
			ImGui::BeginDisabled();

		static std::string current_option = "Plain Text";
		ImGui::PushItemWidth(125);
		if (ImGui::BeginCombo("Convert", current_option.c_str())) {
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

		ImGui::PopItemWidth();

		if (ImGuiFileDialog::Instance()->IsOpened("ConversionImport"))
			ImGui::EndDisabled();

		ImGui::SameLine();

		auto& style = ImGui::GetStyle();

		if (ImGui::Button("Import", ImVec2(ImGui::GetContentRegionAvail().x * 0.5, 0.f)))
			ImGuiFileDialog::Instance()->OpenDialog("ConversionImport", "Choose File", options[current_option]->get_type_filter().c_str(), options[current_option]->get_directory().c_str());
		

		ImGui::SameLine();

		if (ImGui::Button("Export", ImVec2(ImGui::GetContentRegionAvail().x, 0.f))) {
			std::string filename = "converted\\";
			filename += logic.macro_name;
			if (logic.export_to_bot_location) filename = logic.macro_name;
			try {
				options[current_option]->export_to(filename);
				logic.conversion_message = "Success! Exported " + filename + " as a " + options[current_option]->get_type_filter() + " file";
			}
			catch (std::runtime_error& e) {
				logic.conversion_message = "Error! Could not export as " + options[current_option]->get_type_filter();
			}
		}

		ImGui::Checkbox("Auto-Export to Location", &logic.export_to_bot_location); ImGui::SameLine();

		if (ImGui::Button("Batch Import", ImVec2(ImGui::GetContentRegionAvail().x * (docked ? 0.9 : 0.7), 0.f))) {
			auto& old_inputs = logic.inputs;
			for (const auto& entry : fs::directory_iterator(options[current_option]->get_directory())) {
				try {
					if (entry.is_regular_file() && options[current_option]->get_type_filter().find(entry.path().extension().string()) != std::string::npos) {
						options[current_option]->import(entry.path().string());
						Logic::get().sort_inputs();
						Logic::get().write_file(entry.path().stem().string());
						Logic::get().inputs.clear();
					}
				}
				catch (std::exception& e) {
					logic.conversion_message = e.what();
				}
			}
			logic.inputs = old_inputs;
		} ImGui::SameLine();

		HelpMarker("Batch Import automatically converts every macro of a certain type (e.g. TASBot, MHR) to Echo.\nThis feature prevents overwriting old macros.\nDepending on the amount and size of the macros, it may take a while.");

		if (ImGuiFileDialog::Instance()->Display("ConversionImport", ImGuiWindowFlags_NoCollapse, ImVec2(500, 200)))
		{
			// action if OK
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				logic.conversion_message = "";
				try {
					options[current_option]->import(ImGuiFileDialog::Instance()->GetFilePathName());
					reload_inputs = true;
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

		if (ImGui::Button("Merge With Replay")) {
			ImGuiFileDialog::Instance()->OpenDialog("MergeReplay", "Choose File", ".echo,.json", ".echo/replays/");
		}

		if (ImGuiFileDialog::Instance()->Display("MergeReplay", ImGuiWindowFlags_NoCollapse, ImVec2(500, 200)))
		{
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
				std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
				std::vector<Frame> before_inputs = logic.get_inputs();
				logic.inputs.clear();

				std::string suffix = ".echo";
				if (filePathName.size() >= suffix.size() && filePathName.rfind(suffix) == (filePathName.size() - suffix.size()))
					logic.read_file(filePathName, true);
				else
					logic.read_file_json(filePathName, true);

				logic.inputs.insert(logic.inputs.end(), before_inputs.begin(), before_inputs.end());

				logic.sort_inputs();
			}
			ImGuiFileDialog::Instance()->Close();
		}

		ImGui::SameLine();

		const char* noclip_options[] = { "Off", "Player 1", "Player 2", "Both" };

		ImGui::PushItemWidth(docked ? 200 : 175);
		if (ImGui::BeginCombo("Noclip##dropdown_noclip", noclip_options[selected_noclip]))
		{
			for (int i = 0; i < IM_ARRAYSIZE(noclip_options); i++)
			{
				const bool isSelected = (selected_noclip == i);
				if (ImGui::Selectable(noclip_options[i], isSelected))
					selected_noclip = i;

				if (isSelected) {
					ImGui::SetItemDefaultFocus();

					const char* selected = noclip_options[selected_noclip];
				}
			}
			ImGui::EndCombo();
		}

		if (selected_noclip == 0) {
			logic.noclip_player1 = false;
			logic.noclip_player2 = false;
		}
		else if (selected_noclip == 1) {
			logic.noclip_player1 = true;
			logic.noclip_player2 = false;
		}
		else if (selected_noclip == 2) {
			logic.noclip_player1 = false;
			logic.noclip_player2 = true;
		}
		else if (selected_noclip == 3) {
			logic.noclip_player1 = true;
			logic.noclip_player2 = true;
		}

		ImGui::PopItemWidth();

		ImGui::Checkbox("Click For Both Players", &logic.click_both_players);

		CHECK_KEYBIND("clickBoth");
		
		ImGui::SameLine();
		ImGui::Checkbox("Swap Player Input Type", &logic.swap_player_input);
		CHECK_KEYBIND("swapInput");

		const char* inverse_options[] = { "Off", "Player 1", "Player 2", "Both" };

		ImGui::PushItemWidth(200);
		if (ImGui::BeginCombo("Inverse Player Inputs##dropdown_inverse", inverse_options[selected_inverse_click]))
		{
			for (int i = 0; i < IM_ARRAYSIZE(inverse_options); i++)
			{
				const bool isSelected = (selected_inverse_click == i);
				if (ImGui::Selectable(inverse_options[i], isSelected))
					selected_inverse_click = i;

				if (isSelected) {
					ImGui::SetItemDefaultFocus();

					const char* selected = inverse_options[selected_inverse_click];
				}
			}
			ImGui::EndCombo();
		}

		if (selected_inverse_click == 0) {
			logic.click_inverse_p1 = false;
			logic.click_inverse_p2 = false;
		}
		else if (selected_inverse_click == 1) {
			logic.click_inverse_p1 = true;
			logic.click_inverse_p2 = false;
		}
		else if (selected_inverse_click == 2) {
			logic.click_inverse_p1 = false;
			logic.click_inverse_p2 = true;
		}
		else if (selected_inverse_click == 3) {
			logic.click_inverse_p1 = true;
			logic.click_inverse_p2 = true;
		}

		ImGui::Separator();

		bool antiCheatBypass = anticheatBypass.isActivated();
		if (ImGui::Checkbox("Disable Anti-Cheat", &antiCheatBypass)) {
			antiCheatBypass ? anticheatBypass.activate() : anticheatBypass.deactivate();
		}
		CHECK_KEYBIND("anticheat");

		ImGui::SameLine();

		ImGui::Checkbox("Disable Shake Effects", &logic.disable_shakes);

		CHECK_KEYBIND("disableShakes");
		
		bool practiceActivated = practiceMusic.isActivated();
		if (ImGui::Checkbox("Overwrite Practice Music", &practiceActivated)) {
			if (practiceActivated) {
				practiceMusic.activate();
			}
			else {
				practiceMusic.deactivate();
			}
		}
		CHECK_KEYBIND("practiceHack");

		ImGui::SameLine();

		bool noEscActivated = noEscape.isActivated();
		if (ImGui::Checkbox("Ignore Escape", &noEscActivated)) {
			if (noEscActivated) {
				noEscape.activate();
			}
			else {
				noEscape.deactivate();
			}
		}
		CHECK_KEYBIND("noEsc");

		ImGui::Checkbox("###layout_checkbox", &logic.hacks.layoutMode);

		ImGui::SameLine();

		bool open_layout_modal = true;

		if (ImGui::Button("Layout Mode Settings###layout_settings", ImVec2(ImGui::GetContentRegionMax().x * 0.42, 0))) {
			ImGui::OpenPopup("Layout Settings###layoutsettings2");
		}

		if (ImGui::BeginPopupModal("Layout Settings###layoutsettings2", &open_layout_modal, ImGuiWindowFlags_AlwaysAutoResize)) {

			ImGui::Checkbox("Layout Mode", &logic.hacks.layoutMode);
			ImGui::ColorEdit3("Background Color", logic.hacks.backgroundColor, ImGuiColorEditFlags_NoInputs);
			ImGui::ColorEdit3("Block Color", logic.hacks.blocksColor, ImGuiColorEditFlags_NoInputs);
			ImGui::EndPopup();
		}

		ImGui::SameLine();

		ImGui::Checkbox("###hitbox_checkbox", &logic.hacks.showHitboxes);

		ImGui::SameLine();
		bool open_hitbox_modal = true;
		if (ImGui::Button("Hitbox Settings###notboxsettings", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
			ImGui::OpenPopup("Hitbox Settings###boxsettings");
		}

		if (ImGui::BeginPopupModal("Hitbox Settings###boxsettings", &open_hitbox_modal, ImGuiWindowFlags_AlwaysAutoResize)) {

			ImGui::Checkbox("Show Hitboxes", &logic.hacks.showHitboxes); ImGui::SameLine();
			ImGui::Checkbox("Show Modifiers", &logic.hacks.showDecorations);
			ImGui::Checkbox("Hitbox Trail", &logic.hacks.hitboxTrail); ImGui::SameLine();
			ImGui::Checkbox("Fill Hitboxes", &logic.hacks.fillHitboxes);
			ImGui::PushItemWidth(200);
			ImGui::InputFloat("Hitbox Thickness", &logic.hacks.hitboxThickness);
			ImGui::InputFloat("Trail Length", &logic.hacks.hitboxTrailLength);

			const char* colors[] = { "Solids", "Slopes", "Hazards", "Portals", "Pads", "Rings", "Collectibles", "Modifiers", "Rotated Hitbox", "Center Hitbox", "Player" };

			if (ImGui::BeginCombo("##dropdown_hitbox_color", colors[selected_hitbox_color]))
			{
				for (int i = 0; i < IM_ARRAYSIZE(colors); i++)
				{
					const bool isSelected = (selected_hitbox_color == i);
					if (ImGui::Selectable(colors[i], isSelected))
						selected_hitbox_color = i;

					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			} ImGui::PopItemWidth();
			
			ImGui::SameLine();

			switch (selected_hitbox_color)
			{
			case 0: ImGui::ColorEdit4("###color1", logic.hacks.solidHitboxColor, ImGuiColorEditFlags_NoInputs); break;
			case 1: ImGui::ColorEdit4("###color2", logic.hacks.slopeHitboxColor, ImGuiColorEditFlags_NoInputs); break;
			case 2: ImGui::ColorEdit4("###color3", logic.hacks.hazardHitboxColor, ImGuiColorEditFlags_NoInputs); break;
			case 3: ImGui::ColorEdit4("###color4", logic.hacks.portalHitboxColor, ImGuiColorEditFlags_NoInputs); break;
			case 4: ImGui::ColorEdit4("###color5", logic.hacks.padHitboxColor, ImGuiColorEditFlags_NoInputs); break;
			case 5: ImGui::ColorEdit4("###color6", logic.hacks.ringHitboxColor, ImGuiColorEditFlags_NoInputs); break;
			case 6: ImGui::ColorEdit4("###color7", logic.hacks.collectibleHitboxColor, ImGuiColorEditFlags_NoInputs); break;
			case 7: ImGui::ColorEdit4("###color8", logic.hacks.modifierHitboxColor, ImGuiColorEditFlags_NoInputs); break;
			case 8: ImGui::ColorEdit4("###color9", logic.hacks.rotatedHitboxColor, ImGuiColorEditFlags_NoInputs); break;
			case 9: ImGui::ColorEdit4("###color10", logic.hacks.centerHitboxColor, ImGuiColorEditFlags_NoInputs); break;
			case 10: ImGui::ColorEdit4("###color11", logic.hacks.playerHitboxColor, ImGuiColorEditFlags_NoInputs); break;
			default: break;
			}

			ImGui::EndPopup();
		}

		ImGui::Separator();

		ImGui::Checkbox("Enable Autoclicker", &logic.autoclicker);
		CHECK_KEYBIND("autoClicker");

		ImGui::SameLine();

		const char* autoclicker_options[] = { "Player 1", "Player 2", "Both" };

		ImGui::PushItemWidth(200);
		if (ImGui::BeginCombo("##dropdown_autoclicker", autoclicker_options[selected_autoclicker_player]))
		{
			for (int i = 0; i < IM_ARRAYSIZE(autoclicker_options); i++)
			{
				const bool isSelected = (selected_autoclicker_player == i);
				if (ImGui::Selectable(autoclicker_options[i], isSelected))
					selected_autoclicker_player = i;
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();

		const char* selected = autoclicker_options[selected_autoclicker_player];
		if (selected == "Player 1") {
			logic.autoclicker_player_1 = true;
			logic.autoclicker_player_2 = false;
		}
		else if (selected == "Player 2") {
			logic.autoclicker_player_1 = false;
			logic.autoclicker_player_2 = true;
		}
		else if (selected == "Both") {
			logic.autoclicker_player_1 = true;
			logic.autoclicker_player_2 = true;
		}
		
		ImGui::PushItemWidth(75);

		int framesBetweenPresses = Autoclicker::get().getFramesBetweenPresses();
		if (ImGui::DragInt("Click Length", &framesBetweenPresses, 1.0f, 1, 10000)) {
			Autoclicker::get().setFramesBetweenPresses(framesBetweenPresses);
		}

		ImGui::SameLine();

		//ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.8f), "Frames Between Releases");
		int framesBetweenReleases = Autoclicker::get().getFramesBetweenReleases();
		if (ImGui::DragInt("Release Length", &framesBetweenReleases, 1.0f, 1, 10000)) {
			Autoclicker::get().setFramesBetweenReleases(framesBetweenReleases);
		}

		ImGui::PopItemWidth();

		ImGui::Checkbox("Auto Disable", &logic.autoclicker_auto_disable);
		CHECK_KEYBIND("autoDisableClicker");

		ImGui::SameLine();

		HelpMarker("Automatically disables the autoclicker in X frames");

		ImGui::SameLine();

		ImGui::PushItemWidth(150);
		if (!logic.autoclicker_auto_disable) {
			ImGui::BeginDisabled();
			ImGui::DragInt("Frames", &logic.autoclicker_disable_in, 1.0f, 1, 10000);
			ImGui::EndDisabled();
		}
		else {
			ImGui::DragInt("Frames", &logic.autoclicker_disable_in, 1.0f, 1, 10000);
		}
		ImGui::PopItemWidth();

		ImGui::Separator();

		ImGui::Checkbox("Frame Advance", &logic.frame_advance); ImGui::SameLine(0, docked ? 50 : -1);

		CHECK_KEYBIND("frameAdvance");

		if (ImGui::Button("Advance", ImVec2(150, 0))) {
			if (logic.start == std::chrono::steady_clock::time_point())
				logic.start = std::chrono::steady_clock::now();
			logic.frame_advance = false;
			Hooks::CCScheduler_update_h(gd::GameManager::sharedState()->getScheduler(), 0, 1.f / logic.fps);
			logic.frame_advance = true;
		}
		else logic.start = std::chrono::steady_clock::time_point();

		CHECK_KEYBIND("advancing");

		ImGui::SameLine();
		HelpMarker("Frame Advance is a tool that allows you to step through the game's frames one at a time.\nYou can use either the 'Advance' button or the respective keybind to advance.");

		ImGui::PushItemWidth(75);
		int hold_time = logic.frame_advance_hold_duration;
		ImGui::DragInt("Hold Time (ms)", &hold_time, 1, 0); ImGui::SameLine();
		if (hold_time < 0) hold_time = 0;
		logic.frame_advance_hold_duration = hold_time;

		int delay_time = logic.frame_advance_delay;
		ImGui::DragInt("Speed (ms)", &delay_time, 1, 0);
		if (delay_time < 0) delay_time = 0;
		logic.frame_advance_delay = delay_time;
		ImGui::PopItemWidth();

		ImGui::Separator();

		ImGui::SetNextItemWidth(get_width(50.f));

		if (ImGui::Checkbox("Replay Debug Info", &logic.save_debug_info)) {
			if (logic.save_debug_info) {
				logic.format = logic.META_DBG;
			}
			else {
				logic.format = logic.META;
			}
		} ImGui::SameLine();
		HelpMarker("Saving debug information allows for bugfixing and checking accuracy in the replay.");

		ImGui::SameLine();
		if (ImGui::Button("Open Debug Console")) {
			// Allows for debugging, can be removed later
			AllocConsole();
			freopen("CONOUT$", "w", stdout);  // Redirects stdout to the new console
			std::printf("Opened Debugging Console");
		}

		if (docked)
			ImGui::EndTabItem();
		else
			ImGui::End();
	}

}

static int selected_clickbot_player = 0; // Index of "Both" option (auto selected)

void ShowFolderDropdown(const fs::path& directoryPath, std::string selectedFolder)
{
	// Get all directories within the specified directory
	std::vector<fs::path> folders;
	for (const auto& entry : fs::directory_iterator(directoryPath))
	{
		if (fs::is_directory(entry.path()))
		{
			folders.push_back(entry.path());
		}
	}

	// Convert the folder paths to folder names for ImGui
	std::vector<std::string> folderNames;
	folderNames.reserve(folders.size());
	for (const auto& folder : folders)
	{
		folderNames.push_back(folder.filename().string());
	}

	// Create the ImGui dropdown list
	if (ImGui::BeginCombo("##folder_dropdown", selectedFolder.c_str()))
	{
		for (const auto& folderName : folderNames)
		{
			bool isSelected = (selectedFolder == folderName);
			if (ImGui::Selectable(folderName.c_str(), isSelected))
			{
				selectedFolder = folderName;
			}
		}

		ImGui::EndCombo();
	}
}

void GUI::clickbot() {
	auto& logic = Logic::get();

	float tabWidth = ImGui::GetWindowContentRegionWidth() / 6.0f;

	if (docked)
		ImGui::SetNextItemWidth(tabWidth);

	if (ImGui::BeginTabItem("Clickbot") || !docked) {

		if (!docked) {

			ImGui::Begin("Clickbot", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
			clickbot_pos = ImGui::GetWindowPos();

		}

		ImGui::Checkbox("Enabled###enable_clickbot", &logic.clickbot_enabled);

		const char* player_options[] = { "Settings For Player 1", "Settings For Player 2" };

		ImGui::SameLine();
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::BeginCombo("##dropdown_clickbot", player_options[selected_clickbot_player]))
		{
			for (int i = 0; i < IM_ARRAYSIZE(player_options); i++)
			{
				const bool isSelected = (selected_clickbot_player == i);
				if (ImGui::Selectable(player_options[i], isSelected))
					selected_clickbot_player = i;
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();

		ImGui::Separator();

		if (ImGui::BeginTabBar("ClickbotTabs")) {
			if (ImGui::BeginTabItem("General", nullptr)) {
				if (selected_clickbot_player == 0) {
					// Specify the directory path
					fs::path directoryPath = ".echo\\clickpacks";

					// Get all directories within the specified directory
					std::vector<fs::path> folders;
					for (const auto& entry : fs::directory_iterator(directoryPath))
					{
						if (fs::is_directory(entry.path()))
						{
							folders.push_back(entry.path());
						}
					}

					// Convert the folder paths to folder names for ImGui
					std::vector<std::string> folderNames;
					folderNames.reserve(folders.size());
					for (const auto& folder : folders)
					{
						folderNames.push_back(folder.filename().string());
					}

					if (logic.player_1_path.empty() && !folderNames.empty()) logic.player_1_path = folderNames[0];

					// Create the ImGui dropdown list
					if (ImGui::BeginCombo("##folder_dropdown", logic.player_1_path.c_str()))
					{
						for (const auto& folderName : folderNames)
						{
							bool isSelected = (logic.player_1_path == folderName);
							if (ImGui::Selectable(folderName.c_str(), isSelected))
							{
								logic.player_1_path = folderName;
							}
						}

						ImGui::EndCombo();
					}

					ImGui::Checkbox("Soft Clicks###soft_clicks_p1", &logic.player_1_softs); ImGui::SameLine();
					ImGui::Checkbox("Hard Clicks###hard_clicks_p1", &logic.player_1_hards);
					ImGui::SameLine();

					ImGui::Checkbox("Micro Clicks###micro_clicks_p1", &logic.player_1_micros);
				}
				else {
					// Specify the directory path
					fs::path directoryPath = ".echo\\clickpacks";

					// Get all directories within the specified directory
					std::vector<fs::path> folders;
					for (const auto& entry : fs::directory_iterator(directoryPath))
					{
						if (fs::is_directory(entry.path()))
						{
							folders.push_back(entry.path());
						}
					}

					// Convert the folder paths to folder names for ImGui
					std::vector<std::string> folderNames;
					folderNames.reserve(folders.size());
					for (const auto& folder : folders)
					{
						folderNames.push_back(folder.filename().string());
					}

					if (logic.player_2_path.empty() && !folderNames.empty()) logic.player_2_path = folderNames[0];

					// Create the ImGui dropdown list
					if (ImGui::BeginCombo("##folder_dropdown", logic.player_2_path.c_str()))
					{
						for (const auto& folderName : folderNames)
						{
							bool isSelected = (logic.player_2_path == folderName);
							if (ImGui::Selectable(folderName.c_str(), isSelected))
							{
								logic.player_2_path = folderName;
							}
						}

						ImGui::EndCombo();
					}
					ImGui::SameLine();
					ImGui::Text("Click Pack");

					ImGui::Checkbox("Soft Clicks###soft_clicks_p2", &logic.player_2_softs); ImGui::SameLine();
					ImGui::Checkbox("Hard Clicks###hard_clicks_p2", &logic.player_2_hards);
					ImGui::SameLine();

					ImGui::Checkbox("Micro Clicks###micro_clicks_p2", &logic.player_2_micros);
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Volume", nullptr)) {
				ImGui::PushItemWidth(200);
				ImGui::DragFloat("Volume Multiplier", &logic.clickbot_volume_mult_saved, 0.01, 0, 100, "%.1f");
				ImGui::PopItemWidth();

				if (selected_clickbot_player == 0) {
					ImGui::PushItemWidth(125);

					ImGui::DragFloat("Soft Clicks###soft_clicks_vol_p1", &logic.player_1_softs_volume, 0.01, 0, 500, "%.1f");
					ImGui::SameLine();
					ImGui::DragFloat("Hard Clicks###hard_clicks_vol_p1", &logic.player_1_hards_volume, 0.01, 0, 500, "%.1f");

					ImGui::DragFloat("Micro Clicks###micro_clicks_vol_p1", &logic.player_1_micros_volume, 0.01, 0, 500, "%.1f");
					ImGui::SameLine();
					ImGui::DragFloat("Regular Clicks###reg_clicks_vol_p1", &logic.player_1_volume, 0.01, 0, 500, "%.1f");

					ImGui::PopItemWidth();
				}
				else {
					ImGui::PushItemWidth(125);

					ImGui::DragFloat("Soft Clicks###soft_clicks_vol_p2", &logic.player_2_softs_volume, 0.01, 0, 500, "%.1f");
					ImGui::SameLine();
					ImGui::DragFloat("Hard Clicks###hard_clicks_vol_p2", &logic.player_2_hards_volume, 0.01, 0, 500, "%.1f");

					ImGui::DragFloat("Micro Clicks###micro_clicks_vol_p2", &logic.player_2_micros_volume, 0.01, 0, 500, "%.1f");
					ImGui::SameLine();
					ImGui::DragFloat("Regular Clicks###reg_clicks_vol_p2", &logic.player_2_volume, 0.01, 0, 500, "%.1f");

					ImGui::PopItemWidth();
				}

				static char inputBuffer[256];
				strcpy(inputBuffer, logic.algorithm.c_str());
				static bool typing_alg = false;
				ImGui::PushItemWidth(200);
				if (ImGui::InputText("Algorithm", inputBuffer, sizeof(inputBuffer))) typing_alg = true;
				else typing_alg = false;
				ImGui::PopItemWidth();
				ImGui::SameLine();
				HelpMarker("The clickbot algorithm is a mathematical expression to change the volume multiplier as the level goes on.\nThe current defined functions are: cos(), sqrt()\nThe current variables are: x (% in level)");

				ImGui::SameLine();
				ImGui::Text("Result: %f", logic.clickbot_volume_multiplier);
				static std::string expression;
				static std::vector<std::string> tokens = tokenizeExpression(expression);
				static std::map<std::string, double> variables;

				variables["x"] = PLAYLAYER ? PLAYLAYER->m_pPlayer1->getPositionX() / PLAYLAYER->m_endPortal->getPositionX() * 100.f : 0.f;

				expression = "0+";
				expression += inputBuffer;
				tokens = tokenizeExpression(expression);
				logic.algorithm = inputBuffer;

				int index = 0;

				logic.clickbot_volume_multiplier = logic.clickbot_volume_mult_saved * evalExpression(tokens, variables, index);

				// Graph
				ImGui::Separator();
				static ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels;
				static float values[101];
				if (ImPlot::BeginPlot("Volume Multiplier Graph", "X", "Y", ImVec2(-1, 250), ImPlotFlags_None, ImPlotAxisFlags_NoLabel, ImPlotAxisFlags_NoLabel))
				{
					if (typing_alg) {
						double maxValue = 0.0;
						for (int i = 0; i <= 100; ++i)
						{
							double x = i;
							variables["x"] = x;

							int index = 0;
							double result = evalExpression(tokens, variables, index);
							values[i] = result;

							if (result > maxValue)
							{
								maxValue = result;
							}
						}
					}
					ImPlot::PushStyleColor(ImPlotCol_Line, ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive]); // Red color
					ImPlot::PlotLine("Volume", values, 101);
					ImPlot::PopStyleColor();

					ImPlot::EndPlot();
				}
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Times", nullptr)) {

				if (selected_clickbot_player == 0) {
					ImGui::DragInt("Micro Clicks Time###micro_clicks_time_p1", &logic.player_1_micros_time, 0.1, 1);
					ImGui::DragFloat("Soft Clicks Time###soft_clicks_time_p1", &logic.player_1_softs_time, 0.1, 0.01, 10000, "%.0f");
					ImGui::DragFloat("Hard Clicks Time###hard_clicks_time_p1", &logic.player_1_hards_time, 0.01, 0.01);

					ImGui::Separator();

					ImGui::Text("Micro Times:");
					ImGui::SameLine();
					ImGui::Text("0 to %ims", logic.player_1_micros_time);

					ImGui::Text("Soft Times:");
					ImGui::SameLine();
					ImGui::Text("%ims to %.3fms", logic.player_1_micros_time, logic.player_1_softs_time);

					ImGui::Text("Regular Times:");
					ImGui::SameLine();
					ImGui::Text("%.3fms to %.3fs", logic.player_1_softs_time, logic.player_1_hards_time);

					ImGui::Text("Hard Times:");
					ImGui::SameLine();
					ImGui::Text("%.3fs and up", logic.player_1_hards_time);
				}
				else {
					ImGui::DragInt("Micro Clicks Time###micro_clicks_time_p2", &logic.player_2_micros_time, 0.1, 1);
					ImGui::DragFloat("Soft Clicks Time###soft_clicks_time_p2", &logic.player_2_softs_time, 0.1, 0.01, 10000, "%.0f");
					ImGui::DragFloat("Hard Clicks Time###hard_clicks_time_p2", &logic.player_2_hards_time, 0.01, 0.01);

					ImGui::Separator();

					ImGui::Text("Micro Times:");
					ImGui::SameLine();
					ImGui::Text("0 to %ims", logic.player_2_micros_time);

					ImGui::Text("Soft Times:");
					ImGui::SameLine();
					ImGui::Text("%ims to %.3fms", logic.player_2_micros_time, logic.player_2_softs_time);

					ImGui::Text("Regular Times:");
					ImGui::SameLine();
					ImGui::Text("%.3fms to %.3fs", logic.player_2_softs_time, logic.player_2_hards_time);

					ImGui::Text("Hard Times:");
					ImGui::SameLine();
					ImGui::Text("%.3fs and up", logic.player_2_hards_time);
				}
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		if (docked)
			ImGui::EndTabItem();
		else
			ImGui::End();
	}
}
#ifdef _WIN32
#include <windows.h>

std::string GetKeyName(int key) {
	char name[128];
	UINT scanCode = MapVirtualKey(key, MAPVK_VK_TO_VSC);
	int result = GetKeyNameText(scanCode << 16, name, sizeof(name));
	if (result > 0) {
		return name;
	}
	return "Unknown";
}
#endif

void GUI::show_keybind_prompt(const std::string& buttonName) {
	static std::string keybindButtonName = "";
	static std::optional<int> newKey = std::nullopt;
	static bool newCtrl = false, newShift = false, newAlt = false;

	keybindButtonName = buttonName;

	if (keybind_prompt_cache != keybind_prompt) {
		keybind_prompt_cache = keybind_prompt;
		newKey = std::nullopt; // clear the key when the prompt is reopened
		newCtrl = false, newShift = false, newAlt = false;
		ImGui::OpenPopup("Confirm Keybind");
	}

	if (ImGui::BeginPopupModal("Confirm Keybind", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		Keybind& existingKeybind = Logic::get().keybinds.GetKeybind(keybindButtonName);
		if (existingKeybind.GetKey().has_value()) {
			std::string keyInfo = "Existing keybind: " + GetKeyName(*existingKeybind.GetKey());
			if (existingKeybind.GetCtrl()) keyInfo += " with Control";
			if (existingKeybind.GetShift()) keyInfo += " with Shift";
			if (existingKeybind.GetAlt()) keyInfo += " with Alt";
			ImGui::Text(keyInfo.c_str());
			ImGui::Separator();
		}
		ImGui::Text("Please press a button for keybinding '%s'", &buttonName);

		for (int key = 0; key < 512; key++) {
			if (ImGui::IsKeyPressed(key, false)) {
				newKey = key;
				newCtrl = ImGui::GetIO().KeyCtrl;
				newShift = ImGui::GetIO().KeyShift;
				newAlt = ImGui::GetIO().KeyAlt;
				break;
			}
		}

		if (newKey.has_value()) {
			std::string keyInfo = "Pressed key: " + GetKeyName(*newKey);
			if (newCtrl) keyInfo += " with Control";
			if (newShift) keyInfo += " with Shift";
			if (newAlt) keyInfo += " with Alt";
			ImGui::Text(keyInfo.c_str());

			if (ImGui::Button("Save", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5, 0))) {
				Logic::get().keybinds.GetKeybind(keybindButtonName).SetKey(*newKey, newCtrl, newShift, newAlt);
				ImGui::CloseCurrentPopup();
				keybind_prompt_cache = "";
				keybind_prompt = "";
			}
		}
		else {
			ImGui::BeginDisabled();
			ImGui::Button("Save", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5, 0));
			ImGui::EndDisabled();
		}
		ImGui::SameLine();
		if (existingKeybind.GetKey().has_value()) {

			if (ImGui::Button("Remove", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.48, 0))) {
				Logic::get().keybinds.GetKeybind(keybindButtonName).UnsetKey();
				ImGui::CloseCurrentPopup();
				keybind_prompt_cache = "";
				keybind_prompt = "";
			}
		}
		else {
			ImGui::BeginDisabled();
			ImGui::Button("Remove", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.48, 0));
			ImGui::EndDisabled();
		}
		ImGui::Separator();

		if (ImGui::Button("Exit", ImVec2(ImGui::GetWindowContentRegionWidth(), 0))) {
			ImGui::CloseCurrentPopup();
			keybind_prompt_cache = "";
			keybind_prompt = "";
		}

		ImGui::EndPopup();
	}
}

void GUI::main() {
	auto& logic = Logic::get();
	const ImVec2 buttonSize = { get_width(48), 0 };

	float tabWidth = ImGui::GetWindowContentRegionWidth() / 6.0f;
	if (docked)
		ImGui::SetNextItemWidth(tabWidth);
	if (ImGui::BeginTabItem("Main") || !docked) {
		if (!docked) {
			ImGui::Begin(echo_version.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
			main_pos = ImGui::GetWindowPos();
		}

		ImGuiStyle& style = ImGui::GetStyle();

		ImVec4 tempColor = style.Colors[ImGuiCol_Button];
		ImVec4 tempColor2 = style.Colors[ImGuiCol_ButtonHovered];
		ImVec4 tempColor3 = style.Colors[ImGuiCol_ButtonActive];

		/*if (PLAYLAYER) {
			ImGui::Text("%f", logic.tfx2_calculated);
			ImGui::Text("%f", logic.previous_real_xpos);
			ImGui::Text("%f", PLAYLAYER->m_player1->getPositionX());
			ImGui::Text("%f", Logic::get().calculated_xpos - PLAYLAYER->m_player1->getPositionX());
			ImGui::Text("%f", (60.f * Logic::get().player_speed * Logic::get().player_acceleration) * (1.f / Logic::get().fps));
			ImGui::Text("%i", Logic::get().completed_level);
		}*/
		if (logic.is_recording()) {
			style.Colors[ImGuiCol_Button] = style.Colors[ImGuiCol_FrameBg];
			style.Colors[ImGuiCol_ButtonHovered] = style.Colors[ImGuiCol_FrameBgHovered];
			style.Colors[ImGuiCol_ButtonActive] = style.Colors[ImGuiCol_FrameBgHovered];
		}

		if (ImGui::Button(logic.is_recording() ? "Stop Recording" : "Start Recording", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5f, 0))) {
			bool change = true;
			if (logic.is_playing() && (logic.play_player_1 && logic.record_player_1) && (logic.play_player_2 && logic.record_player_2)) {
				logic.toggle_playing();
			}
			if (logic.is_playing()) {
				if ((logic.play_player_1 && logic.record_player_1) || (logic.play_player_2 && logic.record_player_2)) {
					change = false;
					logic.error = "You cannot play and record a player at the same time!";
					return;
				}
			}

			if (change) {
				logic.toggle_recording();
				logic.sort_inputs();
			}
		}

		CHECK_KEYBIND("toggleRecording");

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
			style.Colors[ImGuiCol_Button] = style.Colors[ImGuiCol_FrameBg];
			style.Colors[ImGuiCol_ButtonHovered] = style.Colors[ImGuiCol_FrameBgHovered];
			style.Colors[ImGuiCol_ButtonActive] = style.Colors[ImGuiCol_FrameBgHovered];
		}

		if (ImGui::Button(logic.is_playing() ? "Stop Playing" : "Start Playing", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
			bool change = true;
			if (logic.is_recording() && (logic.play_player_1 && logic.record_player_1) && (logic.play_player_2 && logic.record_player_2)) {
				logic.toggle_recording();
			}
			if (logic.is_recording()) {
				if ((logic.play_player_1 && logic.record_player_1) || (logic.play_player_2 && logic.record_player_2)) {
					change = false;
					logic.error = "You cannot play and record a player at the same time!";
					return;
				}
			}

			if (change) {
				logic.toggle_playing();
			}
		}

		CHECK_KEYBIND("togglePlaying");

		if (logic.is_playing()) {
			style.Colors[ImGuiCol_Button] = tempColor;
			style.Colors[ImGuiCol_ButtonHovered] = tempColor2;
			style.Colors[ImGuiCol_ButtonActive] = tempColor3;
		}

		// Revert ImGui style back to default
		style.Colors[ImGuiCol_Button] = tempColor;
		style.Colors[ImGuiCol_ButtonHovered] = tempColor2;
		style.Colors[ImGuiCol_ButtonActive] = tempColor3;

		const char* recording_player_options[] = { "Both", "Player 1", "Player 2" };

		if (logic.is_recording() || logic.is_playing())
			ImGui::BeginDisabled();
		ImGui::PushItemWidth(ImGui::GetWindowContentRegionWidth() * 0.5);
		if (ImGui::BeginCombo("###dropdown_recording_player", recording_player_options[selected_recording]))
		{
			for (int i = 0; i < IM_ARRAYSIZE(recording_player_options); i++)
			{
				const bool isSelected = (selected_recording == i);
				if (ImGui::Selectable(recording_player_options[i], isSelected))
					selected_recording = i;

				if (isSelected) {
					ImGui::SetItemDefaultFocus();

					const char* selected = recording_player_options[selected_recording];
				}
			}
			ImGui::EndCombo();
		}

		if (logic.is_recording() || logic.is_playing())
			ImGui::EndDisabled();

		if (selected_recording == 0) {
			logic.record_player_1 = true;
			logic.record_player_2 = true;
		}
		else if (selected_recording == 1) {
			logic.record_player_1 = true;
			logic.record_player_2 = false;
		}
		else if (selected_recording == 2) {
			logic.record_player_1 = false;
			logic.record_player_2 = true;
		}

		ImGui::SameLine();

		const char* playing_player_options[] = { "Both", "Player 1", "Player 2" };

		if (logic.is_recording() || logic.is_playing())
			ImGui::BeginDisabled();

		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::BeginCombo("###dropdown_playing_player", playing_player_options[selected_playback]))
		{
			for (int i = 0; i < IM_ARRAYSIZE(playing_player_options); i++)
			{
				const bool isSelected = (selected_playback == i);
				if (ImGui::Selectable(playing_player_options[i], isSelected))
					selected_playback = i;

				if (isSelected) {
					ImGui::SetItemDefaultFocus();

					const char* selected = playing_player_options[selected_playback];
				}
			}
			ImGui::EndCombo();
		}

		if (logic.is_recording() || logic.is_playing())
			ImGui::EndDisabled();

		if (selected_playback == 0) {
			logic.play_player_1 = true;
			logic.play_player_2 = true;
		}
		else if (selected_playback == 1) {
			logic.play_player_1 = true;
			logic.play_player_2 = false;
		}
		else if (selected_playback == 2) {
			logic.play_player_1 = false;
			logic.play_player_2 = true;
		}

		ImGui::Spacing();

		ImGui::Separator();

		auto end = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - Logic::get().start);

		ImGui::Text("FPS: %.2f", logic.get_fps()); ImGui::SameLine();
		
		std::string frame_text = "Frame: " + std::to_string(logic.get_frame());
		float fps_text_width = ImGui::CalcTextSize(frame_text.c_str()).x;
		float cursor_pos_x = (ImGui::GetWindowContentRegionWidth() - fps_text_width) * 0.5;

		ImGui::SetCursorPosX(cursor_pos_x);
		ImGui::Text("Frame: %i", logic.get_frame()); ImGui::SameLine();


		std::string macro_size_text = "Macro Size: " + std::to_string(logic.inputs.size());
		float macro_size_text_width = ImGui::CalcTextSize(macro_size_text.c_str()).x;
		cursor_pos_x = ImGui::GetWindowContentRegionWidth() - macro_size_text_width; // .45 so it doesnt seem unaligned later

		ImGui::SetCursorPosX(cursor_pos_x);

		ImGui::Text("Macro Size: %i", logic.get_inputs().size());

		ImGui::PushItemFlag(ImGuiItemFlags_NoTabStop, true);
		ImGui::SetNextItemWidth(150);
		if (ImGui::DragFloat("###fps", &input_fps, 0.1, 1.f))
			GUI::get().change_display_fps = false;
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
			change_display_fps = true;
		}

		ImGui::SameLine();

		HelpMarker("To set only the Display FPS, set the FPS while recording or playing a replay.");

		ImGui::SameLine();

		float speed = logic.speedhack;

		ImGui::PushItemWidth(150);

		ImGui::SameLine();

		if (ImGui::DragFloat("Speed", &speed, 0.01, 0.01f, 10.f, "%.2f")) {
			if (speed < 0.01) speed = 0.01f;
			logic.speedhack = speed;
			gd::GameManager::sharedState()->getScheduler()->setTimeScale(logic.speedhack);
			/*if (logic.respawn_time_modified) {
				Opcode opcode(Cheat::AntiCheatBypass);
				opcode.ModifyFloatAtOffset(0x20A677, logic.speedhack);
			}*/
		}
		ImGui::PopItemWidth();

		auto& audiospeedhack = AudiopitchHack::getInstance();
		bool isEnabled = audiospeedhack.isEnabled();

		if (ImGui::Checkbox("Audio Speed", &isEnabled)) {
			if (isEnabled) {
				audiospeedhack.setEnabled(true);
			}
			else {
				audiospeedhack.setEnabled(false);
			}
		}

		CHECK_KEYBIND("audioHack");

		ImGui::SameLine();
		HelpMarker("Changes the pitch and speed of the audio of the game according to the speedhack.");

		ImGui::SameLine();

		//if (logic.recorder.m_recording && !logic.recorder.real_time_rendering) {
		//	ImGui::BeginDisabled();
		//	logic.real_time_mode = false;
		//}

		////ImGui::Checkbox("Real Time Mode", &logic.real_time_mode); cant be fucked

		//if (logic.recorder.m_recording && !logic.recorder.real_time_rendering) ImGui::EndDisabled();

		//CHECK_KEYBIND("realTimeMode");

		ImGui::Checkbox("No Overwrite", &logic.no_overwrite);
		CHECK_KEYBIND("noOverwrite");
		
		ImGui::SameLine();
		HelpMarker("Allows you to replay from the beginning of the level without losing your progress.");
		ImGui::SameLine();

		ImGui::Checkbox("Ignore Actions", &logic.ignore_actions_at_playback);
		CHECK_KEYBIND("ignoreActions");

		ImGui::SameLine();
		HelpMarker("Ignores inputs from player while replaying.");

		ImGui::Separator();

		ImGui::PushItemWidth(100);
		ImGui::DragFloat("Max CPS", &logic.max_cps, 0.01, 1, 100, "%.2f"); ImGui::SameLine();
		ImGui::PopItemWidth();

		bool open_cps_breaks = true;
		if (ImGui::Button("CPS Info", ImVec2(100, 0))) {
			ImGui::OpenPopup("CPS Breaks###cps_breaks");
		}

		if (ImGui::BeginPopupModal("CPS Breaks###cps_breaks", &open_cps_breaks, ImGuiWindowFlags_AlwaysAutoResize)) {

			ImGui::Text("Highest CPS: %s", logic.highest_cps_cached().c_str());

			ImGui::Separator();

			if (!logic.cps_over_percents.empty()) {
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

					ImGui::Text("%s%%", percent.c_str());
					ImGui::SameLine();
					ImGui::Text("#%s", rule.c_str());
				}
			}
			else {
				ImGui::Text("No CPS Breaks");
			}
			
			ImGui::EndPopup();
		}

		ImGui::SameLine();

		bool open_label_modal = true;

		if (ImGui::Button("Label Settings", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
			ImGui::OpenPopup("Label Settings");
		}

		if (ImGui::BeginPopupModal("Label Settings", &open_label_modal, ImGuiWindowFlags_AlwaysAutoResize)) {

			ImGui::BeginChild("RecordingLabel", ImVec2(250, 150), false, ImGuiWindowFlags_AlwaysUseWindowPadding);
			ImGui::Checkbox("Show Recording Label", &logic.show_recording);

			CHECK_KEYBIND("showRecording");

			ImGui::PushItemWidth(75);
			ImGui::DragFloat("X###record_x", &logic.recording_label_x, 1);
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::PushItemWidth(75);
			ImGui::DragFloat("Y###record_y", &logic.recording_label_y, 1);

			ImGui::PushItemWidth(150);
			ImGui::DragFloat("Scale###record_scale", &logic.recording_label_scale, 0.01, 10);
			ImGui::PopItemWidth();

			ImGui::PushItemWidth(150);
			ImGui::DragFloat("Opacity###record_opacity", &logic.recording_label_opacity, 0.1, 0, 100);
			ImGui::PopItemWidth();

			ImGui::EndChild();
			ImGui::SameLine();
			ImGui::BeginChild("CpsLabel", ImVec2(250, 150), false, ImGuiWindowFlags_AlwaysUseWindowPadding);

			ImGui::Checkbox("Show CPS", &logic.show_cps);

			CHECK_KEYBIND("showCPS");

			ImGui::PushItemWidth(75);
			ImGui::DragFloat("X###cps_x", &logic.cps_counter_x, 1);
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::PushItemWidth(75);
			ImGui::DragFloat("Y###cps_y", &logic.cps_counter_y, 1);

			ImGui::PushItemWidth(150);
			ImGui::DragFloat("Scale###cps_scale", &logic.cps_counter_scale, 0.01, 10);
			ImGui::PopItemWidth();

			ImGui::PushItemWidth(150);
			ImGui::DragFloat("Opacity###cps_opacity", &logic.cps_counter_opacity, 0.1, 0, 100);
			ImGui::PopItemWidth();

			ImGui::EndChild();
			ImGui::Separator();
			ImGui::BeginChild("FrameLabel", ImVec2(250, 150), false, ImGuiWindowFlags_AlwaysUseWindowPadding);

			ImGui::Checkbox("Show Frame", &logic.show_frame);

			CHECK_KEYBIND("showFrame");

			ImGui::PushItemWidth(75);
			ImGui::DragFloat("X###frame_x", &logic.frame_counter_x, 1);
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::PushItemWidth(75);
			ImGui::DragFloat("Y###frame_y", &logic.frame_counter_y, 1);
			ImGui::PopItemWidth();

			ImGui::PushItemWidth(150);
			ImGui::DragFloat("Scale###frame_scale", &logic.frame_counter_scale, 0.01, 10);
			ImGui::PopItemWidth();

			ImGui::PushItemWidth(150);
			ImGui::DragFloat("Opacity###frame_opacity", &logic.frame_counter_opacity, 0.1, 0, 100);
			ImGui::PopItemWidth();

			ImGui::EndChild();
			ImGui::SameLine();
			ImGui::BeginChild("TimeLabel", ImVec2(250, 150), false, ImGuiWindowFlags_AlwaysUseWindowPadding);

			ImGui::Checkbox("Show Time", &logic.show_time);

			CHECK_KEYBIND("showTime");

			ImGui::PushItemWidth(75);
			ImGui::DragFloat("X###time_x", &logic.time_counter_x, 1);
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::PushItemWidth(75);
			ImGui::DragFloat("Y###time_y", &logic.time_counter_y, 1);
			ImGui::PopItemWidth();

			ImGui::PushItemWidth(150);
			ImGui::DragFloat("Scale###time_scale", &logic.time_scale, 0.01, 10);
			ImGui::PopItemWidth();

			ImGui::PushItemWidth(150);
			ImGui::DragFloat("Opacity###time_opacity", &logic.time_opacity, 0.1, 0, 100);
			ImGui::PopItemWidth();

			ImGui::EndChild();
			ImGui::Separator();

			ImGui::BeginChild("PercentLabel", ImVec2(0, 125), false, ImGuiWindowFlags_AlwaysUseWindowPadding);
			ImGui::Checkbox("Show Percent", &logic.show_percent);

			CHECK_KEYBIND("showPercent");

			ImGui::PushItemWidth(75);
			ImGui::DragFloat("X###percent_x", &logic.percent_counter_x, 1);
			ImGui::PopItemWidth();
			ImGui::SameLine();
			ImGui::PushItemWidth(75);
			ImGui::DragFloat("Y###percent_y", &logic.percent_counter_y, 1);
			ImGui::PopItemWidth();

			ImGui::SameLine();

			ImGui::PushItemWidth(150);
			ImGui::InputInt("Accuracy###percent_acc", &logic.percent_accuracy, 1, 2);
			ImGui::PopItemWidth();

			ImGui::PushItemWidth(150);
			ImGui::DragFloat("Scale###percent_scale", &logic.percent_scale, 0.01, 10);
			ImGui::PopItemWidth();

			ImGui::SameLine();

			ImGui::PushItemWidth(150);
			ImGui::DragFloat("Opacity###percent_opacity", &logic.percent_opacity, 0.1, 0, 100);
			ImGui::PopItemWidth();
			ImGui::EndChild();

			ImGui::EndPopup();
		}

		ImGui::Separator();

		ImGui::Checkbox("File Browser", &logic.file_dialog);

		CHECK_KEYBIND("useDialog");
		ImGui::SameLine();

		bool open_rename_modal = true;

		if (ImGui::Button("Overwrite Settings")) {
			ImGui::OpenPopup("Overwrite Name Format");
		}

		if (ImGui::BeginPopupModal("Overwrite Name Format", &open_rename_modal, ImGuiWindowFlags_AlwaysAutoResize)) {

			ImGui::PushItemWidth(200);
			ImGui::InputText("Renaming Format", &logic.rename_format);
			ImGui::PopItemWidth();

			ImGui::SameLine();

			HelpMarker("The rename format prevents accidental deletion of important replays by renaming the ones that would otherwise overwrite each other.\nFor example, the format '_#' will rename ReplayName.echo to ReplayName_1.echo.");

			ImGui::EndPopup();
		}

		ImGui::SameLine();

		bool open_modal = true;

		if (ImGui::Button("Reset Macro", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
			if (!logic.inputs.empty()) {
				ImGui::OpenPopup("Confirm Reset");

				ImVec2 popupSize(200, 100);
				ImVec2 popupPos(
					(ImGui::GetIO().DisplaySize.x - popupSize.x) / 2.f,
					(ImGui::GetIO().DisplaySize.y - popupSize.y) / 2.f
				);

				ImGui::SetNextWindowPos(popupPos);
			}
		}

		if (ImGui::BeginPopupModal("Confirm Reset", &open_modal, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Are you sure you want to reset your macro?");

			const ImVec2 buttonSize((ImGui::GetWindowContentRegionWidth() - ImGui::GetStyle().ItemSpacing.x) * 0.5f, 0);

			if (ImGui::Button("Yes", buttonSize)) {
				logic.get_inputs().clear();
				logic.total_recording_time = std::chrono::duration<double>::zero();
				logic.total_attempt_count = 0;
				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();

			if (ImGui::Button("No", buttonSize)) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::Separator();

		ImGui::PushItemFlag(ImGuiItemFlags_NoTabStop, true);
		ImGui::SetNextItemWidth(175);
		ImGui::InputText("Name", logic.macro_name, MAX_PATH);
		ImGui::PopItemFlag();

		ImGui::SameLine();

		ImGui::SetNextItemWidth((ImGui::GetWindowContentRegionWidth() - ImGui::GetStyle().ItemSpacing.x) * (50.f / 100.f));
		if (ImGui::Button("Save File", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.24f, 0))) {
			if (logic.use_json_for_files) {
				logic.write_file_json(logic.macro_name);
			}
			else {
				logic.write_file(logic.macro_name);
			}
		}

		ImGui::SameLine();

		ImGui::SetNextItemWidth(get_width(50.f));

		if (ImGui::Button("Load File", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
			if (!logic.file_dialog) {
				if (logic.use_json_for_files) {
					logic.read_file_json(logic.macro_name, false);
				}
				else {
					logic.read_file(logic.macro_name, false);
				}
				reload_inputs = true;
			}
			else {
				ImGuiFileDialog::Instance()->OpenDialog("ImportNormal", "Choose File", ".echo,.json", ".echo/replays/");
			}
			input_fps = logic.fps;
			CCDirector::sharedDirector()->setAnimationInterval(1.f / GUI::get().input_fps);
			logic.sort_inputs();
			logic.format = logic.META;
		}

		if (ImGuiFileDialog::Instance()->Display("ImportNormal", ImGuiWindowFlags_NoCollapse, ImVec2(500, 200)))
		{
			if (ImGuiFileDialog::Instance()->IsOk())
			{
				std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();

				size_t dotPosition = filePathName.find_last_of(".");

				std::filesystem::path path(filePathName);
				std::string nameWithoutExtension = path.stem().stem().string(); // blud


				std::string suffix = ".json";
				logic.inputs.clear();
				if (filePathName.size() >= suffix.size() && filePathName.rfind(suffix) == (filePathName.size() - suffix.size()))
					logic.read_file_json(filePathName, true);
				else
					logic.read_file(filePathName, true);

				reload_inputs = true;
				input_fps = logic.fps;
				CCDirector::sharedDirector()->setAnimationInterval(1.f / GUI::get().input_fps);

				strcpy(logic.macro_name, nameWithoutExtension.c_str());
				logic.format = logic.META;
			}
			ImGuiFileDialog::Instance()->Close();
		}


		ImGui::Separator();

		if (ImGui::Button("Close Menu", ImVec2(ImGui::GetWindowContentRegionWidth() * 0.5, 0))) {
			show_window = false;
		}
		CHECK_KEYBIND("menuBind");
		ImGui::SameLine();
		if (ImGui::Button("Reset Level", ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
			if (PLAYLAYER) {
				Hooks::PlayLayer::resetLevel_h(PLAYLAYER, 0);
			}
		}

		CHECK_KEYBIND("resetLevel");

		const ImVec2 main_size = ImGui::GetWindowSize();

		if (docked)
			ImGui::EndTabItem();
		else
			ImGui::End();
	
	}
}

// has to be here now for keybinds, cuz they aint loaded til gui init
void readBinds() {
	auto& logic = Logic::get();
	auto& recorder = logic.recorder;

	std::ifstream file(".echo\\settings\\settings.json");
	if (!file.is_open()) {
		// no error message, it should have already popped up on game start
		return;
	}
	json j;
	file >> j;

	if (j.contains("keybinds")) {
		const nlohmann::json& jsonKeybinds = j["keybinds"];

		for (auto it = jsonKeybinds.begin(); it != jsonKeybinds.end(); ++it) {
			const std::string& action = it.key();
			const nlohmann::json& jsonKeybind = it.value();

			Keybind& keybind = logic.keybinds.bindings[action].first;

			if (jsonKeybind.contains("key")) {
				keybind.SetKey(jsonKeybind["key"].get<int>());
				keybind.ctrl = jsonKeybind["ctrl"].get<bool>();
				keybind.shift = jsonKeybind["shift"].get<bool>();
				keybind.alt = jsonKeybind["alt"].get<bool>();
				Logic::get().keybinds.GetKeybind(action).SetKey(keybind.key.value(), keybind.ctrl, keybind.shift, keybind.alt);
			}
		}
	}

	CCDirector::sharedDirector()->setAnimationInterval(1.f / logic.fps);

	file.close();
}

#define SET_BIND(name, var) \
    std::unique_ptr<KeybindableBase> name = std::unique_ptr<KeybindableBase>(new Keybindable([this]() { \
        var = !var; \
    })); \
    Logic::get().keybinds.SetAction(#name, std::move(name))

void appendToFile(double percent, const std::string& filename) {
	std::ofstream outputFile;
	outputFile.open(filename, std::ios::app); // Open the file in append mode

	if (!outputFile) {
		std::cerr << "Error opening file: " << filename << std::endl;
		return;
	}

	// Append the value of 'percent' to the file
	outputFile << percent << std::endl;

	outputFile.close();
}

void GUI::init() {
	
	ImPlot::CreateContext();

	std::unique_ptr<KeybindableBase> audioSpeedHackAction = std::unique_ptr<KeybindableBase>(new Keybindable([this]() {
		auto& audiospeedhack = AudiopitchHack::getInstance();
		bool isEnabled = audiospeedhack.isEnabled();
		printf("Test");

		isEnabled = !isEnabled; // flip the value
		if (isEnabled) {
			audiospeedhack.setEnabled(true);
		}
		else {
			audiospeedhack.setEnabled(false);
		}
	}));

	std::unique_ptr<KeybindableBase> anticheatAction = std::unique_ptr<KeybindableBase>(new Keybindable([this]() {
		bool isEnabled = anticheatBypass.isActivated();

		isEnabled = !isEnabled; // flip the value
		isEnabled ? anticheatBypass.activate() : anticheatBypass.deactivate();
		}));

	std::unique_ptr<KeybindableBase> practiceMusicAction = std::unique_ptr<KeybindableBase>(new Keybindable([this]() {
		bool isEnabled = practiceMusic.isActivated();

		isEnabled = !isEnabled; // flip the value
		isEnabled ? practiceMusic.activate() : practiceMusic.deactivate();
		}));

	std::unique_ptr<KeybindableBase> noEscAction = std::unique_ptr<KeybindableBase>(new Keybindable([this]() {
		bool isEnabled = practiceMusic.isActivated();

		isEnabled = !isEnabled; // flip the value
		isEnabled ? noEscape.activate() : noEscape.deactivate();
		}));

	std::unique_ptr<KeybindableBase> advanceAction = std::unique_ptr<KeybindableBase>(new Keybindable([this]() {
		if (Logic::get().start == std::chrono::steady_clock::time_point())
			Logic::get().start = std::chrono::steady_clock::now();
		Logic::get().frame_advance = false;
		Hooks::CCScheduler_update_h(gd::GameManager::sharedState()->getScheduler(), 0, 1.f / Logic::get().fps / Logic::get().speedhack);
		Logic::get().frame_advance = true;
		}));


	std::unique_ptr<KeybindableBase> renderingAction = std::unique_ptr<KeybindableBase>(new Keybindable([this]() {
		Logic::get().recorder.m_recording = !Logic::get().recorder.m_recording;
		if (Logic::get().recorder.m_recording) {
				Logic::get().recorder.start(Logic::get().recorder.directory + Logic::get().recorder.video_name + Logic::get().recorder.extension);
		}
		else {
				Logic::get().recorder.stop();
		}
		}));

	std::unique_ptr<KeybindableBase> recordingAction = std::unique_ptr<KeybindableBase>(new Keybindable([this]() {
		auto& logic = Logic::get();
		bool change = true;
		if (logic.is_playing()) {
			if ((logic.play_player_1 && logic.record_player_1) || (logic.play_player_2 && logic.record_player_2)) {
				change = false;
				logic.error = "You cannot play and record a player at the same time!";
				return;
			}
		}

		if (change) {
			logic.toggle_playing();
			logic.sort_inputs();
		}
		}));

	std::unique_ptr<KeybindableBase> playingAction = std::unique_ptr<KeybindableBase>(new Keybindable([this]() {
		auto& logic = Logic::get();
		bool change = true;
		if (logic.is_recording()) {
			if ((logic.play_player_1 && logic.record_player_1) || (logic.play_player_2 && logic.record_player_2)) {
				change = false;
				logic.error = "You cannot play and record a player at the same time!";
			}
		}

		if (change) {
			logic.toggle_playing();
			logic.sort_inputs();
		}
	}));

	std::unique_ptr<KeybindableBase> resetAction = std::unique_ptr<KeybindableBase>(new Keybindable([this]() {
		if (PLAYLAYER) {
			Hooks::PlayLayer::resetLevel_h(PLAYLAYER, 0);
		}
		}));

	std::unique_ptr<KeybindableBase> outputPercentAction = std::unique_ptr<KeybindableBase>(new Keybindable([this]() {
		if (PLAYLAYER) {
			float percent = min(100.f, (PLAYLAYER->m_pPlayer1->getPositionX() / PLAYLAYER->m_endPortal->getPositionX()) * 100.f);
			appendToFile(percent, "PERCENTS.txt");
		}
		}));

	Logic::get().keybinds.SetAction("audioHack", std::move(audioSpeedHackAction));

	Logic::get().keybinds.SetAction("anticheat", std::move(anticheatAction));

	Logic::get().keybinds.SetAction("practiceHack", std::move(practiceMusicAction));
	Logic::get().keybinds.SetAction("noEsc", std::move(noEscAction));
	Logic::get().keybinds.SetAction("toggleRendering", std::move(renderingAction));

	Logic::get().keybinds.SetAction("toggleRecording", std::move(recordingAction));
	Logic::get().keybinds.SetAction("togglePlaying", std::move(playingAction));
	Logic::get().keybinds.SetAction("advancing", std::move(advanceAction));
	Logic::get().keybinds.SetAction("resetLevel", std::move(resetAction));

	Logic::get().keybinds.SetAction("outputPercents", std::move(outputPercentAction));

	SET_BIND(realTimeMode, Logic::get().real_time_mode);
	
	SET_BIND(showFrame, Logic::get().show_frame);

	SET_BIND(showCPS, Logic::get().show_cps);

	SET_BIND(showPercent, Logic::get().show_percent);
	SET_BIND(showTime, Logic::get().show_time);

	SET_BIND(useJSON, Logic::get().use_json_for_files);

	SET_BIND(useDialog, Logic::get().file_dialog);

	SET_BIND(clickBoth, Logic::get().click_both_players);

	SET_BIND(swapInput, Logic::get().swap_player_input);

	SET_BIND(noOverwrite, Logic::get().no_overwrite);
	SET_BIND(ignoreActions, Logic::get().ignore_actions_at_playback);
	SET_BIND(modifyRespawn, Logic::get().respawn_time_modified);
	SET_BIND(autoClicker, Logic::get().autoclicker);
	SET_BIND(autoDisableClicker, Logic::get().autoclicker_auto_disable);
	SET_BIND(frameAdvance, Logic::get().frame_advance);
	SET_BIND(menuBind, show_window);
	SET_BIND(showRecord, Logic::get().show_recording);

	Logic::get().keybinds.GetKeybind("menuBind").SetKey(164, false, false, true);

	//Logic::get().keybinds.GetKeybind("outputPercents").SetKey(77, false, false, false); // doki percents shit for Title Wave

	readBinds();

	auto& style = ImGui::GetStyle();
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.WindowBorderSize = 0;
	style.ColorButtonPosition = ImGuiDir_Left;

	ImFontConfig fontConfig;
	fontConfig.FontDataOwnedByAtlas = false;

	CCDirector::sharedDirector()->setAnimationInterval(1.f / Logic::get().fps);

	ImGui::GetIO().Fonts->AddFontFromMemoryTTF(Fonts::OpenSans_Medium, sizeof(Fonts::OpenSans_Medium), 42.f, &fontConfig);

	style.FramePadding = ImVec2{ 8, 4 };
	style.IndentSpacing = 21;
	style.ItemSpacing = { 8, 8 };

	ImGui::GetIO().FontGlobalScale /= 2.0f;

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

	style.Colors[ImGuiCol_ModalWindowDimBg] = popup_bg_color;
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
	//ImPlot::DestroyContext();
}