#include "Speedhack.h"

namespace Speedhack
{
	double speed = 1.0;
	bool initialised = false;
	typedef DWORD(WINAPI* _tGetTickCount)();
	_tGetTickCount _GTC = nullptr;
	DWORD _GTC_BaseTime = 0, _GTC_OffsetTime = 0;

	typedef ULONGLONG(WINAPI* _tGetTickCount64)();
	_tGetTickCount64 _GTC64 = nullptr;
	ULONGLONG _GTC64_BaseTime = 0, _GTC64_OffsetTime = 0;

	typedef BOOL(WINAPI* _tQueryPerformanceCounter)(LARGE_INTEGER* lpPerformanceCount);
	_tQueryPerformanceCounter _QPC = nullptr;
	LARGE_INTEGER _QPC_BaseTime = LARGE_INTEGER(), _QPC_OffsetTime = LARGE_INTEGER();

	DWORD WINAPI _hGetTickCount()
	{
		return _GTC_OffsetTime + ((_GTC() - _GTC_BaseTime) * speed);
	}

	ULONGLONG WINAPI _hGetTickCount64()
	{
		return _GTC64_OffsetTime + ((_GTC64() - _GTC64_BaseTime) * speed);
	}

	BOOL WINAPI _hQueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount)
	{
		LARGE_INTEGER x;
		_QPC(&x);
		lpPerformanceCount->QuadPart = _QPC_OffsetTime.QuadPart + ((x.QuadPart - _QPC_BaseTime.QuadPart) * speed);
		return TRUE;
	}

	void Setup()
	{
		MH_Initialize();

		_GTC = (_tGetTickCount)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetTickCount");
		_GTC_BaseTime = _GTC();
		_GTC_OffsetTime = _GTC_BaseTime;

		_GTC64 = (_tGetTickCount64)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetTickCount64");
		_GTC64_BaseTime = _GTC64();
		_GTC64_OffsetTime = _GTC64_BaseTime;

		_QPC = (_tQueryPerformanceCounter)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "QueryPerformanceCounter");
		_QPC(&_QPC_BaseTime);
		_QPC_OffsetTime.QuadPart = _QPC_BaseTime.QuadPart;

		MH_CreateHook((LPVOID)_GTC, _hGetTickCount, reinterpret_cast<LPVOID*>(&_GTC));
		MH_CreateHook((LPVOID)_GTC64, _hGetTickCount64, reinterpret_cast<LPVOID*>(&_GTC64));
		MH_CreateHook((LPVOID)_QPC, _hQueryPerformanceCounter, reinterpret_cast<LPVOID*>(&_QPC));

		initialised = true;
	}

	void Detach()
	{
		if (initialised)
		{
			MH_DisableHook(MH_ALL_HOOKS);
			MH_Uninitialize();

			_GTC = nullptr;
			_GTC64 = nullptr;
			_QPC = nullptr;

			initialised = false;
		}
	}

	void SetSpeed(double relSpeed)
	{
		if (initialised)
		{
			_GTC_OffsetTime = _hGetTickCount();
			_GTC_BaseTime = _GTC();

			_GTC64_OffsetTime = _hGetTickCount64();
			_GTC64_BaseTime = _GTC64();

			_hQueryPerformanceCounter(&_QPC_OffsetTime);
			_QPC(&_QPC_BaseTime);
		}

		speed = relSpeed;
	}
}