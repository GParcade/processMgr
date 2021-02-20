#pragma once
#include <vector>
#include <Windows.h>
#include <string>
#include <wtsapi32.h>
#include <tlhelp32.h>
#include <sddl.h>
#include "string_tool.hpp"
#include "multi_thread_for.hpp"
#pragma comment(lib, "Wtsapi32.lib")
namespace windows_enum {
	std::vector<DWORD> get_childs_from_pid(DWORD ppid);
}

struct process_info {
	DWORD session_id = 0;     // session id
	DWORD process_id = 0;     // process id
	shared_str<std::wstring> proc_name;
	std::vector<DWORD> childs_id;
	shared_str<std::wstring> summoner_name;
	shared_str<std::wstring> summoner_domain;
	process_info() {}
	process_info(const process_info& copy) {
		session_id = copy.session_id;
		process_id = copy.process_id;
		proc_name = copy.proc_name;
		summoner_name = copy.summoner_name;
		summoner_domain = copy.summoner_domain;
		childs_id = copy.childs_id;
	}
	process_info(DWORD session, DWORD proc_id, std::wstring proces_name, PSID summoner_psid) {
		session_id = session;
		process_id = proc_id;
		proc_name = proces_name;
		childs_id = windows_enum::get_childs_from_pid(process_id);
		{
			wchar_t user[80], domain[80];
			DWORD cbUser = 80, cbDomain = 80;
			SID_NAME_USE nu;
			if (LookupAccountSidW(NULL, summoner_psid, user, &cbUser, domain, &cbDomain, &nu)) {
				summoner_name = user;
				summoner_domain = domain;
			}
		}
	}
};
struct process_info_no_child {
	DWORD session_id = 0;     // session id
	DWORD process_id = 0;     // process id
	shared_str<std::wstring> proc_name;
	shared_str<std::wstring> summoner_name;
	shared_str<std::wstring> summoner_domain;
	process_info_no_child() {}
	process_info_no_child(const process_info_no_child& copy) {
		session_id = copy.session_id;
		process_id = copy.process_id;
		proc_name = copy.proc_name;
		summoner_name = copy.summoner_name;
		summoner_domain = copy.summoner_domain;
	}
	process_info_no_child(const process_info& copy) {
		session_id = copy.session_id;
		process_id = copy.process_id;
		proc_name = copy.proc_name;
		summoner_name = copy.summoner_name;
		summoner_domain = copy.summoner_domain;
	}
	process_info_no_child(DWORD session, DWORD proc_id, std::wstring proces_name, PSID summoner_psid) {
		session_id = session;
		process_id = proc_id;
		proc_name = proces_name;
		{
			wchar_t user[80], domain[80];
			DWORD cbUser = 80, cbDomain = 80;
			SID_NAME_USE nu;
			if (LookupAccountSidW(NULL, summoner_psid, user, &cbUser, domain, &cbDomain, &nu)) {
				summoner_name = user;
				summoner_domain = domain;
			}
		}
	}
};




struct process_three_info {
	struct three {
		process_info_no_child dad;
		std::vector<process_info_no_child> childs;
		three(process_info& check) {
			dad = check;
		}
		three(std::vector<process_info>& list, process_info& check) {
			dad = check;
			if (list.size()) {
				if (check.childs_id.size()) {
					for (int64_t i = list.size() - 1; i >= 0; i--) {
						if (contain_value(check.childs_id, list[i].process_id)) {
							childs.push_back(list[i]);
							list.erase(list.begin() + i);
						}
					}
				}
			}
		}
	};
	std::vector<three> res;

	process_three_info(std::vector<process_info> list) {
	right_cycle:
		for (size_t i = 0; i < list.size();) {
			if (list[i].childs_id.size()) {
				process_info tmp = list[i];
				list.erase(list.begin() + i);
				res.push_back(three(list, tmp));
				if (res[res.size() - 1].childs.size()) goto right_cycle;
			}
			else 
				i++;
		}
		if (list.size()) {
			for (size_t i = 0; i < list.size();i++) 
				res.push_back(three(list[i]));
		}
	}
	auto operator[](size_t pos) {
		return res[pos];
	}
	auto size() {
		return res.size();
	}
	auto begin() {
		return res.begin();
	}
	auto end() {
		return res.end();
	}
};


namespace windows_enum {
	namespace {
		std::vector<std::wstring> res;

		int WINAPI enum_window(HWND hWnd, LPARAM) {
			wchar_t buffer[300];
			GetWindowTextW(hWnd, buffer, 300);
			res.push_back(buffer);
			return 0;
		}
	}
	std::vector<DWORD> get_childs_from_pid(DWORD ppid) {
		
		std::vector<DWORD> pids;
		HANDLE hp = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		PROCESSENTRY32 pe = { 0 };
		pe.dwSize = sizeof(PROCESSENTRY32);
		if (Process32First(hp, &pe)) {
			do {
				if (pe.th32ParentProcessID == ppid) 
					pids.push_back(pe.th32ProcessID);
			} while (Process32Next(hp, &pe));
		}
		CloseHandle(hp);
		return pids;
	}
	//use (WTS_PROCESS_INFOW& one_process, size_t index) for interate_func and (size_t total_process) for pre_run_func or nullptr
	template<class _FN,class _FN2>
	void enum_all_process(_FN interate_func,_FN2 pre_run_func = nullptr) {
		WTS_PROCESS_INFOW* pWPIs = NULL;
		DWORD dwProcCount = 0;
		if (WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, NULL, 1, &pWPIs, &dwProcCount)) {
			if constexpr (std::is_same<_FN2,std::nullptr_t>::value!=true)
				pre_run_func(dwProcCount);
			if constexpr (std::is_same<_FN, std::nullptr_t>::value != true)
				for_thread(pWPIs, 0, dwProcCount, interate_func);
		}
		if (pWPIs) {
			WTSFreeMemory(pWPIs);
			pWPIs = nullptr;
		}
	}

	template<class _FN,class _FN2>
	void enum_all_process_slow(_FN interate_func, _FN2 pre_run_func = nullptr) {
		WTS_PROCESS_INFOW* pWPIs = NULL;
		DWORD dwProcCount = 0;
		if (WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, NULL, 1, &pWPIs, &dwProcCount)) {
			if constexpr (std::is_same<_FN2,std::nullptr_t>::value != true)
				pre_run_func(dwProcCount);
			if constexpr (std::is_same<_FN, std::nullptr_t>::value != true)
				for (size_t index = 0; index < dwProcCount; index++) interate_func(pWPIs[index], index);
		}
		if (pWPIs) {
			WTSFreeMemory(pWPIs);
			pWPIs = nullptr;
		}
	}

	//use (THREADENTRY32& one_thread)
	template<class _FN>
	void enum_all_threads_slow(_FN interate_func) {
		HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		if (h != INVALID_HANDLE_VALUE) {
			THREADENTRY32 te;
			te.dwSize = sizeof(te);
			if (Thread32First(h, &te)) {
				do {
					if (te.dwSize >= FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(te.th32OwnerProcessID))
						interate_func(te);
					te.dwSize = sizeof(te);
				} while (Thread32Next(h, &te));
			}
			CloseHandle(h);
		}
	}




	template<class process_type>
	std::vector<process_type> get_process_list() {
		std::vector<process_type> result;
		enum_all_process(
			[&result](WTS_PROCESS_INFOW& interate, size_t i) {
				result[i] = {
					interate.SessionId ,
					interate.ProcessId ,
					interate.pProcessName ,
					interate.pUserSid
				};
			},
			
			[&result](size_t set_size) {
				result.resize(set_size);
			}
		);
		return result;
	}

	

	std::vector<std::wstring>& windows_enum(uint64_t proc_id) {
		res.clear();
		if (EnumThreadWindows(
			proc_id,
			&enum_window,
			NULL
		)) ;
		return res;
	}
	bool windows_exist(uint64_t proc_id, std::wstring str) {
		windows_enum(proc_id);
		return exist(res, str);
	}
	bool windows_exist_similar(uint64_t proc_id, std::wstring str, size_t threshold = 0) {
		windows_enum(proc_id);
		return contain_similar(res, str, threshold);
	}

	bool windows_contain(uint64_t proc_id, std::vector<std::wstring>& strs) {
		windows_enum(proc_id);
		return exist(res, strs);
	}
	bool windows_contain_similar(uint64_t proc_id, std::vector<std::wstring>& strs, size_t threshold = 0) {
		windows_enum(proc_id);
		for (auto& str : strs)
			if (contain_similar(res, str, threshold))return 1;
		return 0;
	}
}
namespace windows {
	size_t theads_count(DWORD PID) {
		size_t interate = 0;
		windows_enum::enum_all_threads_slow([PID, &interate](THREADENTRY32& info) {if (info.th32OwnerProcessID == PID)interate++; });
		return interate;
	}
}