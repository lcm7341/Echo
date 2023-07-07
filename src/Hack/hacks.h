#include <nlohmann/json.hpp>
#include <random>
static DWORD libcocosbase = (DWORD)GetModuleHandleA("libcocos2d.dll");

using json = nlohmann::json;

namespace Hacks
{
	static std::string utf16ToUTF8(const std::wstring& s)
	{
		const int size = ::WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, NULL, 0, 0, NULL);

		std::vector<char> buf(size);
		::WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, &buf[0], size, 0, NULL);

		return std::string(&buf[0]);
	}

	static bool writeBytes(std::uintptr_t const address, std::vector<uint8_t> const& bytes)
	{
		return WriteProcessMemory(GetCurrentProcess(), reinterpret_cast<LPVOID>(address), bytes.data(), bytes.size(),
			nullptr);
	}

	template <class T> T Read(uint32_t vaddress)
	{
		T buf;
		return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(vaddress), &buf, sizeof(T), NULL) ? buf : T();
	}

	static std::vector<uint8_t> ReadBytes(uint32_t vaddress, size_t size)
	{
		std::vector<std::uint8_t> buffer;
		for (size_t i = 0; i < size; ++i)
		{
			buffer.push_back(Read<std::uint8_t>(vaddress + i));
		}
		return buffer;
	}

	template <class T> bool Write(uint32_t vaddress, const T& value)
	{
		DWORD oldProtect = 0;
		VirtualProtectEx(GetCurrentProcess(), reinterpret_cast<void*>(vaddress), 256, PAGE_EXECUTE_READWRITE, &oldProtect);
		return WriteProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(vaddress), &value, sizeof(T), NULL);
	}

	template <class T> void WriteRef(uint32_t vaddress, const T& value)
	{
		DWORD old_prot;
		VirtualProtect((void*)(vaddress), sizeof(size_t), PAGE_EXECUTE_READWRITE, &old_prot);
		auto x = new T;
		*x = value;
		*reinterpret_cast<T**>(vaddress) = x;
		VirtualProtect((void*)(vaddress), sizeof(size_t), old_prot, &old_prot);
	}

	static std::string narrow(const wchar_t* str)
	{
		int size = WideCharToMultiByte(CP_UTF8, 0, str, -1, nullptr, 0, nullptr, nullptr);
		if (size <= 0)
		{ /* fuck */
		}
		auto buffer = new char[size];
		WideCharToMultiByte(CP_UTF8, 0, str, -1, buffer, size, nullptr, nullptr);
		std::string result(buffer, size_t(size) - 1);
		delete[] buffer;
		return result;
	}
	static inline auto narrow(const std::wstring& str)
	{
		return narrow(str.c_str());
	}

	static std::wstring widen(const char* str)
	{
		int size = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);
		if (size <= 0)
		{ /* fuck */
		}
		auto buffer = new wchar_t[size];
		MultiByteToWideChar(CP_UTF8, 0, str, -1, buffer, size);
		std::wstring result(buffer, size_t(size) - 1);
		delete[] buffer;
		return result;
	}

	static inline auto widen(const std::string& str)
	{
		return widen(str.c_str());
	}

	static int randomInt(int min, int max)
	{
		std::random_device device;
		std::mt19937 generator(device());
		std::uniform_int_distribution<int> distribution(min, max);

		return distribution(generator);
	}

	static std::vector<std::string> splitByDelim(const std::string& str, char delim)
	{
		std::vector<std::string> tokens;
		size_t pos = 0;
		size_t len = str.length();
		tokens.reserve(len / 2); // allocate memory for expected number of tokens

		while (pos < len)
		{
			size_t end = str.find_first_of(delim, pos);
			if (end == std::string::npos)
			{
				tokens.emplace_back(str.substr(pos));
				break;
			}
			tokens.emplace_back(str.substr(pos, end - pos));
			pos = end + 1;
		}

		return tokens;
	};

	static std::vector<uint8_t> HexToBytes(const std::string& hex)
	{
		std::vector<uint8_t> bytes;
		for (unsigned int i = 0; i < hex.length(); i += 3)
		{
			std::string byteString = hex.substr(i, 2);
			uint8_t byte = (uint8_t)strtol(byteString.c_str(), nullptr, 16);
			bytes.push_back(byte);
		}
		return bytes;
	}

	static void Priority(int priority)
	{
		switch (priority)
		{
		case 0:
			SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
			break;
		case 1:
			SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
			break;
		case 2:
			SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
			break;
		case 3:
			SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
			break;
		case 4:
			SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
			break;
		}
	}

	static std::string GetSongFolder()
	{
		bool standardPath = !gd::GameManager::sharedState()->getGameVariable("0033");
		if (standardPath)
			return CCFileUtils::sharedFileUtils()->getWritablePath();
		else
			return CCFileUtils::sharedFileUtils()->getWritablePath2() + "Resources/";
	}
};