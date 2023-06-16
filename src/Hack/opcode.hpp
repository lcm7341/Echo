#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <gd.h>
#include <cocos2d.h>
#include <cstdint>
#include <vector>
#include <string>

struct OpcodeDetails {
    uintptr_t address;
    std::vector<unsigned char> on;
    std::vector<unsigned char> off;
};

enum class Cheat {
    AntiCheatBypass,
    NoClip,
    PracticeMusic
};

const std::map<Cheat, std::vector<OpcodeDetails>> cheatOpcodes = {
    {Cheat::AntiCheatBypass, {
        {0x202AAA, {0xEB, 0x2E}, {0x74, 0x2E}},
        {0x15FC2E, {0xEB}, {0x74}},
        {0x20D3B3, {0x90, 0x90, 0x90, 0x90, 0x90}, {0xE8, 0x58, 0x04, 0x00, 0x00}},
        {0x1FF7A2, {0x90, 0x90}, {0x74, 0x6E}},
        {0x18B2B4, {0xB0, 0x01}, {0x88, 0xD8}},
        {0x20C4E6, {0xE9, 0xD7, 0x00, 0x00, 0x00, 0x90}, {0x0F, 0x85, 0xD6, 0x00, 0x00, 0x00}},
        {0x1FD557, {0xEB, 0x0C}, {0x74, 0x0C}},
        {0x1FD742, {0xC7, 0x87, 0xE0, 0x02, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xC7, 0x87, 0xE4, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90},
                   {0x80, 0xBF, 0xDD, 0x02, 0x00, 0x00, 0x00, 0x0F, 0x85, 0x0A, 0xFE, 0xFF, 0xFF, 0x80, 0xBF, 0x34, 0x05, 0x00, 0x00, 0x00, 0x0F, 0x84, 0xFD, 0xFD, 0xFF, 0xFF}},
        {0x1FD756, {0x90, 0x90, 0x90, 0x90, 0x90, 0x90}, {0x0F, 0x84, 0xFD, 0xFD, 0xFF, 0xFF}},
        {0x1FD79A, {0x90, 0x90, 0x90, 0x90, 0x90, 0x90}, {0x0F, 0x84, 0xB9, 0xFD, 0xFF, 0xFF}},
        {0x1FD7AF, {0x90, 0x90, 0x90, 0x90, 0x90, 0x90}, {0x0F, 0x85, 0xA4, 0xFD, 0xFF, 0xFF}},
    }},
    {Cheat::NoClip, {
        {0x20A23C, {0xE9, 0x79, 0x06, 0x00, 0x00}, {0x6A, 0x14, 0x8B, 0xCB, 0xFF}},
    }},
    {Cheat::PracticeMusic, {
        {0x20C925, {0x90, 0x90, 0x90, 0x90, 0x90, 0x90}, {0x0F, 0x85, 0xF7, 0x00, 0x00, 0x00}},
        {0x20D143, {0x90, 0x90}, {0x75, 0x41}},
        {0x20A563, {0x90, 0x90}, {0x75, 0x3E}},
        {0x20A563, {0x90, 0x90}, {0x75, 0x0C}},
    }}
};

class Opcode {
public:
    Opcode(Cheat cheat)
        : activated(false), cheat(cheat)
    {}

    void activate() {
        applyOpcodes(true);
        activated = true;
    }

    void deactivate() {
        applyOpcodes(false);
        activated = false;
    }

    bool isActivated() const {
        return activated;
    }

private:
    bool activated;
    Cheat cheat;

    void applyOpcodes(bool activate) {
        std::vector<OpcodeDetails> opcodeDetails = cheatOpcodes.at(cheat);
        for (const OpcodeDetails& details : opcodeDetails) {
            uintptr_t address = details.address;
            const std::vector<unsigned char>& pattern = activate ? details.on : details.off;
            WriteToMemory(address, pattern);
        }
    }

    void WriteToMemory(uintptr_t address2, const std::vector<unsigned char>& pattern) {
        uint32_t address = gd::base + address2;

        HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
        if (hProc == NULL) {
            std::cerr << "Failed to open process.\n";
            return;
        }

        DWORD oldProtect;
        if (!VirtualProtectEx(hProc, reinterpret_cast<LPVOID>(address), pattern.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            std::cerr << "Failed to change memory protection.\n";
            return;
        }

        if (!WriteProcessMemory(hProc, reinterpret_cast<LPVOID>(address), pattern.data(), pattern.size(), nullptr)) {
            std::cerr << "Failed to write to memory.\n";
            return;
        }

        if (!VirtualProtectEx(hProc, reinterpret_cast<LPVOID>(address), pattern.size(), oldProtect, NULL)) {
            std::cerr << "Failed to restore memory protection.\n";
            return;
        }

        CloseHandle(hProc);
    }
};