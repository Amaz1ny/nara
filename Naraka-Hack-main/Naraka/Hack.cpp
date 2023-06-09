#include "Sdk.h"
#include "includes.h"
#include "Hack.h"
#include <vector>

MemoryToolsWrapper* memorytools;
Driver* pdriver;

std::vector<uint64_t> entitylist;


namespace config
{
	bool Player = true;
	bool Name = true;
	bool zhendao = true;
}
char* TCHAR2char(const TCHAR* STR)
{

	//返回字符串的长度
	int size = WideCharToMultiByte(CP_ACP, 0, STR, -1, NULL, 0, NULL, FALSE);

	//申请一个多字节的字符串变量
	char* str = new char[sizeof(char) * size];

	//将STR转成str
	WideCharToMultiByte(CP_ACP, 0, STR, -1, str, size, NULL, FALSE);
	return str;
}

void GetModel(DWORD pid, ULONG64& UnityPlayer, ULONG64& GameAssembly) {
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, pid);

	if (hProcess == 0) {
		printf("OpenProcess failed, %08X\n", GetLastError());
		return;
	}

	HMODULE hMods[1024];
	int i;
	if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &pid) == FALSE) {
		printf("enumprocessmodules failed, %08X\n", GetLastError());
	}
	else {
		for (i = 0; i < (pid / sizeof(HMODULE)); i++)
		{
			TCHAR szModName[MAX_PATH];
			if (GetModuleFileNameEx(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR))) {
				char* tmp = TCHAR2char(szModName);
				printf("模块名称 %s \n", tmp);
				if (strcmp(tmp, "\\UnityPlayer.dll") == 0) {
					MODULEINFO lpmodinfo = { 0 };
					GetModuleInformation(hProcess, hMods[i], &lpmodinfo, sizeof(lpmodinfo));
					printf("%s base: %08X, its size %08X\n", szModName, hMods[i], lpmodinfo.SizeOfImage);
					UnityPlayer = (DWORD)hMods[i];
				}

				if (strcmp(tmp, "\\GameAssembly.dll") == 0)
				{
					MODULEINFO lpmodinfo = { 0 };
					GetModuleInformation(hProcess, hMods[i], &lpmodinfo, sizeof(lpmodinfo));
					printf("%s base: %08X, its size %08X\n", szModName, hMods[i], lpmodinfo.SizeOfImage);
					GameAssembly = (DWORD)hMods[i];
				}

			}
		}
	}
}

void InitializeAddress()
{
	HWND NarakaWindow = NULL;
	DWORD NarakaProcess = NULL;
	while (NarakaWindow == NULL || NarakaProcess == NULL)
	{
		NarakaWindow = FindWindowA("UnityWndClass", "Naraka");
		if (NarakaWindow)
			GetWindowThreadProcessId(NarakaWindow, &NarakaProcess);
		Sleep(2000);
	}
	pdriver = new Driver();
	pdriver->Initialize(NarakaProcess);
	memorytools = new MemoryToolsWrapper(pdriver);
	ULONG64 UnityPlayer = 0;
	ULONG64 GameAssembly = 0;


	while (true)
	{
		GetModel(NarakaProcess, UnityPlayer, GameAssembly);
		if (UnityPlayer && GameAssembly)
			break;
	}
	sdk::initialize(UnityPlayer, GameAssembly);

	MessageBoxA(NULL, "Ok", "Done", MB_ICONINFORMATION);
}

void draw_menu(Nvidia* overlay)
{
	static bool open = true;
	if (GetAsyncKeyState(VK_INSERT) & 1) {
		open = !open;
	}
	if (open)
	{
		overlay->draw_text_yellow(10, 50, L"[INS 显示->隐藏]");

		if (config::Player)
		{
			overlay->draw_text_yellow(10, 70, L"玩家方框 -> F6");
		}
		if (!config::Player)
		{
			overlay->draw_text_white(10, 70, L"玩家方框 -> F6");
		}
		if (config::Name)
		{
			overlay->draw_text_yellow(10, 90, L"玩家名字 -> F7");
		}
		if (!config::Name)
		{
			overlay->draw_text_white(10, 90, L"玩家名字 -> F7");
		}

		if (config::zhendao)
		{
			overlay->draw_text_yellow(10, 110, L"自动振刀 -> F8");
		}
		if (!config::zhendao)
		{
			overlay->draw_text_white(10, 110, L"自动振刀 -> F8");
		}
		if (GetAsyncKeyState(VK_F6) & 1)
		{
			config::Player = !config::Player;
		}
		if (GetAsyncKeyState(VK_F7) & 1)
		{
			config::Name = !config::Name;
		}
		if (GetAsyncKeyState(VK_F8) & 1)
		{
			config::zhendao = !config::zhendao;
		}
	}
}

void draw_esp(Nvidia* overlay)
{
	auto getboxrect = [](D2D1_RECT_F& BoxRect, Vector3& Bottom, Vector3& Top) {
		BoxRect.bottom = Bottom.y - Top.y;
		BoxRect.right = BoxRect.bottom / 2;
		BoxRect.left = Top.x - BoxRect.bottom / 4;
		BoxRect.top = Top.y;
	};
	for (auto entity : entitylist)
	{
		Vector3 entitypos = sdk::get_enitypos(entity);
		Vector3 headpos = entitypos;
		headpos.y += 1.8;
		if (sdk::get_view_matrix().ToScreenPos(entitypos, 1920, 1080)
			&&
			sdk::get_view_matrix().ToScreenPos(headpos, 1920, 1080)
			)
		{
			D2D1_RECT_F box;
			getboxrect(box, entitypos, headpos);

			if (config::Player)
			{
				overlay->draw_rect(box.left, box.top, box.right, box.bottom, 1.5);
			}
			if (config::Name)
				overlay->draw_text_white(headpos.x, headpos.y + 3, sdk::get_name(entity).c_str());
		}

	}
}

void update()
{
	entitylist.clear();
	uint32_t playercount = sdk::get_playerCount();
	uintptr_t playeritems = sdk::get_playeritems();
	uintptr_t localaddr = sdk::get_local();
	uintptr_t localteamid = sdk::get_team(localaddr);
	for (size_t i = 0; i < playercount; i++)
	{
		uintptr_t entityaddr = sdk::get_entityaddr(playeritems, i);
		uintptr_t entityteamid = sdk::get_team(entityaddr);
		if (localaddr != entityaddr && entityteamid != localteamid)//排除队伍 和 自己
		{
			entitylist.push_back(entityaddr);
		}
	}
}

void entity_loop()
{
	if (!config::zhendao) return;
	auto local = sdk::get_local();
	for (auto entity : entitylist)
	{
		if (sdk::is_weapon(sdk::get_weapon(entity)) && sdk::is_weapon(sdk::get_weapon(local)))//判断是否手持武器 
		{
			if (sdk::get_endurelevel(local) < 21)//判断当前状态 是否需要去振刀
			{
				Vector3 entitypos = sdk::get_enitypos(local);
				Vector3 localpos = sdk::get_enitypos(entity);
				auto distance = localpos.distTo(entitypos);
				float angle = fabsf(math::AngleDifference(sdk::get_enityangle(entity), math::VectorToRotationYaw(math::FindLookAtVector(entitypos, localpos))));//敌人面向 自己 的 角度

				if (distance > 5.9f)
					continue;

				//蓝霸体攻击?
				if (!sdk::is_attacking(entity))
					continue;

				if (angle > 68)//判断敌人是否面对自己 放出蓄力
					continue;

				//设置G键为振刀热键
				keybd_event('G', MapVirtualKey('G', MAPVK_VK_TO_VSC), 0, 0);
				Sleep(10);
				keybd_event('G', MapVirtualKey('G', MAPVK_VK_TO_VSC), 2, 0);
				Sleep(400);
			}
		}

	}

}
