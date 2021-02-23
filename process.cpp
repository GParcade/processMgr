#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <Windows.h>
#include <locale.h>
#include <fcntl.h>
#include <io.h>
#include "filter.hpp"
#include "user_tool.hpp"
#include "multi_thread_for.hpp"
#include "string_tool.hpp"
#include "windows_enum.hpp"
#include "list_cmd.hpp"
#include "console.hpp"
#pragma comment(lib, "advapi32")
#pragma comment(lib, "Wtsapi32.lib")

#pragma comment(lib, "ntdll.lib")
EXTERN_C NTSTATUS NTAPI NtSuspendProcess(IN HANDLE ProcessHandle);
EXTERN_C NTSTATUS NTAPI NtResumeProcess(IN HANDLE ProcessHandle);

void init_locale() {
	if (
		_setmode(_fileno(stdout), _O_U16TEXT) != -1 ||
		_setmode(_fileno(stdin), _O_U16TEXT)  != -1 ||
		_setmode(_fileno(stderr), _O_U16TEXT) !=-1
	)
	{
		CONSOLE_FONT_INFOEX cfi;
		GetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), false, &cfi);
		wcscpy_s(cfi.FaceName, L"Lucida Console");
		SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), false, &cfi);
	}else
		exit(0xAAAAAAAAAAAA);
}

typedef LONG(NTAPI* _NtSuspendProcess)(IN HANDLE ProcessHandle);

void help_commads() {
	std::wcout << reset_color <<
		"Commands:\n"
		"\t  list " << rgb_color_text(207, 207, 207) << "\"sorter options\" \"filters\"\n\n" << reset_color <<
		"\t  kill \"name\"\n"
		"\t  killF \"filters\"\n\n"
		"\t  summon \"path / alias\" " << rgb_color_text(207, 207, 207) << "privilege\n\n" << reset_color <<
		"\t  freze \"name\"\n"
		"\t  frezeF \"filters\"\n"
		"\t  \tPause proces/ses\n\n"
		"\t  unfreze \"name\"\n"
		"\t  unfrezeF \"filters\"\n"
		"\t  \tContinue run proces/ses\n\n";
}

void help_filters() {
	std::wcout <<
		"Filters: 123\n"
		"\t (empty) ->  use default\n"
		"\t - ->  use default\n"
		"\t * ->  no filter\n"
		"\t "
	<< rgb_color_text(50, 255, 50) << "U" << reset_color <<
		"sers{\"1\",\"2\",\"3\"} -> started by \"name\" filter\n"
		"\t\t (empty)  ->  current user[default]\n"
		"\t\t *        ->  by any[default if defined "
		<< rgb_color_text(255, 0, 0) << "G" << reset_color << "roup filter without "
		<< rgb_color_text(50, 255, 50) << "U" << reset_color <<
		"ser]\n\n\t "
	<< rgb_color_text(80, 140, 255) << "N" << reset_color << "ames{\"1\",\"2\",\"3\"} ->  name filter\n\n\t "
		<< rgb_color_text(80, 140, 255) << "W" << reset_color << "indows{\"1\",\"2\",\"3\"} ->  process windows name filter\n\n\t "
	<< rgb_color_text(255, 0, 0) << "G" << reset_color << "roup{\'1\',\"2\",\"3\"} ->  group filter\n"
		"\t\t (empty)       ->  all[default]\n"
		"\t\t *group_name*  ->  add to filter this group\n\n\t "
	<< rgb_color_text(0, 120, 120) << "D" << reset_color << "omain{\'1\',\"2\",\"3\"} ->  domain filter\n"
		"\t\t (empty)       ->  all[default]\n"
		"\t\t *domain_name*  ->  add to filter this domain\n\n\t "
//	<< rgb_color_text(180, 255, 30) << "M" << reset_color <<
//		"isc{1,2,3} ->  Misc filter\n"
//		"\t\t priority{}             -> All ptioritys: {realtime,height,heigh_avg,default,low_avg,low}\n"
//		"\t\t cpu_min{\"float 0-100\"}   -> Selects all processes using CPU load from percent\n"
//		"\t\t cpu_max{\"float 0-100\"}   -> Selects all processes using CPU load to percent\n"
//		"\t\t evaluated              -> Select process evaluated to admin rights\n\n\t "
	<< rgb_color_text(255, 180, 30) << "C" << reset_color <<
		"onfig{1,2,3} ->  filter configuration\n"
//		"\t\t ignore_child              -> do not touch childs\n"     TO-Do
		"\t\t similar_threshold{\"int\"}-> \"Similary\" threshold\n"
		"\t\t use_similar{\"int\"}      -> Use similary comparer\n\n";
}

void help_privlegies() {
	std::wcout <<
		"Privileges 1:\n"
		"\t  any ->  current user\n"
		//"\t  n/N ->  NT privlege\t\t! only administrator can do it\n"
		"\t  a/A ->  administrator privlege\n\n";
		//"\t  u/U ->  current user without any perm\n\n";
}

void help_list() {
	std::wcout <<
		"List options: 1,2,3\n"
		"\t  * ->  use all labels and name sort\n"
		"\t Label ids:\n"
		"\t\t name        -> Process name\n"
		"\t\t proc_id     -> Process indifiner\n"
		"\t\t page_fault  -> Page fault\n"
		"\t\t page_use    -> Pages used\n"
		"\t\t peak_work   -> Peak woring memory set\n"
		"\t\t qnppu       -> Quota non paged pool usage\n"
		"\t\t qnpppu      -> Quota non peak paged pool usage\n"
		"\t\t qppu        -> Quota paged pool usage\n"
		"\t\t qpppu       -> Quota peak paged pool usage\n"
		"\t\t session     -> User session id\n"
		"\t\t creator_dom -> Created process user domain\n"
		"\t\t creator_nam -> Created process user name\n"
		"\t\t threads     -> Threads count\n"
		"\t\t flags       -> Misc process flags (not work for sort)\n"
		"\t\t work_mem    -> Working mem set\n\n"
		"\t  " << rgb_color_text(50, 50 , 255) << "S" << reset_color << "ort{'1','2'}\n"
		"\t\t  - ->  use default\n"
		"\t\t  direction_to_down ->  use sort form up to down [default]\n"
		"\t\t  direction_to_up   ->  use sort form down to up\n"
		"\t\t  *Label*           ->  label id [default is name]\n\n"
		"\t  " << rgb_color_text(255, 0, 50) << "O" << reset_color << "ut{'1','2','3'...} -> show label if defined\n"
		"\t\t  - ->  use default\n"
		"\t\t  use_color  -> paint all strings to unique rgb color\n"
		"\t\t  *Label*,** ->  label ids [default is {'name','proc_id','creator_nam','session','work_mem'}]\n\n"
		;
}




void help() {
	std::wcout << reset_color;
	help_commads();
	help_filters();
	help_privlegies();
	help_list();
}



template<class process_type> 
std::vector<process_type> get_from_filter(std::wstring str_filter) {
	filter::processed_filter f(str_filter);
	std::vector<process_type> result = windows_enum::get_process_list<process_type>();
	if (!f.no_filters) {
		filter::Filter main_f(result.size(), true);
		if (!f.name_index.ids.empty()) {
			for_thread(result, [&main_f, &f](process_type& info, size_t i) {

				main_f[i] = exist(f.name_index.ids, (uint64_t)info.process_id);

			});
		}

		if (f.config.similar) {
			if (f.name_index.names.size() | f.name_index.wnames.size()) {
				filter::Filter name_f(main_f);
				if (f.name_index.names.size()) {

					for_thread(result, [&name_f, &f](process_type& info, size_t i) {
						if (name_f[i])
							name_f[i] = name_f[i] & contain_similar(f.name_index.names, *info.proc_name);
						});

				}

				if (f.name_index.wnames.size()) {
					std::vector<HWND> check;
					windows_enum::enum_all_windows([&check](HWND& hwnds) {check.push_back(hwnds); });
				
					for_thread(result, [&name_f, &f, check](process_type& info, size_t i) {
						if (name_f[i])
							name_f[i] = name_f[i] & windows_enum::windows_contain_similar((uint64_t)info.process_id, f.name_index.wnames, check);
						});
				
				}

				main_f.apply(name_f);
			}
		}
		else if (f.name_index.names.size() || f.name_index.wnames.size()) {
			filter::Filter name_f(main_f);
			if (f.name_index.names.size()) {

				for_thread(result, [&name_f, &f](process_type& info, size_t i) {
					if (name_f[i])
						name_f[i] = name_f[i] & contain(f.name_index.names, *info.proc_name);
					});

			}

			if (f.name_index.wnames.size()) {
				std::vector<HWND> check;
				windows_enum::enum_all_windows([&check](HWND& hwnds) {check.push_back(hwnds); });
				for_thread(result, [&name_f, &f, check](process_type& info, size_t i) {
					if (name_f[i])
						name_f[i] = name_f[i] & windows_enum::windows_contain((uint64_t)info.process_id, f.name_index.wnames, check);
					});
			
			}

			main_f.apply(name_f);
		}

		if (f.started_by.current_user || !f.started_by.domains.empty() || !f.started_by.names.empty()) {
			filter::Filter user_f(result.size(),false);

			if (f.started_by.current_user) {
				for_thread(result, [&user_f,&main_f](process_type& info, size_t i) {
					if (main_f[i]) {
						user_f[i] = (*info.summoner_domain == user::current_domain() && *info.summoner_name == user::current_name());
					}
				});
			}
			if (!f.started_by.names.empty()) {
				if (f.config.similar) {
					for_thread(result, [&main_f, &user_f, &f](process_type& info, size_t i) {
						if (main_f[i])
							user_f[i] = user_f[i] || contain_similar(f.started_by.names, *info.summoner_name);
						});
				}
				else {
					for_thread(result, [&main_f,&user_f, &f](process_type& info, size_t i) {
						if (main_f[i])
							user_f[i] = user_f[i] || contain(f.started_by.names, *info.summoner_name);
						});
				}
			}
			if (!f.started_by.domains.empty()) {
				if (f.config.similar) {
					for_thread(result, [&main_f, &user_f, &f](process_type& info, size_t i) {
						if (main_f[i])
							user_f[i] = user_f[i] || contain_similar(f.started_by.domains, *info.summoner_domain);
						});
				}
				else {
					for_thread(result, [&main_f, &user_f, &f](process_type& info, size_t i) {
						if (main_f[i])
							user_f[i] = user_f[i] || contain(f.started_by.names, *info.summoner_name);
						});
				}
			}


			main_f.apply(user_f);
		}


		main_f.apply_to(result, false);
	}
	return result;
}

void list(std::wstring options, std::wstring filter) {
	list_cmd::list_dethal test;
		auto tmp = get_from_filter<process_info>(filter);
		size_t old_s;
	right_cycle:
		old_s = tmp.size();
		for (size_t i = 0; i < tmp.size();) {
				if (tmp[i].childs_id.size()) {
					process_info temp = tmp[i];
					tmp.erase(tmp.begin() + i);
					test.push(tmp, temp);
					if (old_s != tmp.size()) goto right_cycle;
				}
				else
					i++;
			}
		if (tmp.size()) {
				for (size_t i = 0; i < tmp.size(); i++)
					test.push(tmp[i]);
			}
		test.output(options);
}


namespace kill_cmd {
	void kill(std::wstring& options) {
		std::transform(options.begin(), options.end(), options.begin(), ::tolower);
		windows_enum::enum_all_process(
			[options](WTS_PROCESS_INFOW& interate, size_t i) {
				if (interate.pProcessName) {
					if (to_low(interate.pProcessName) == options) {
						if (HANDLE hThread = OpenProcess(PROCESS_ALL_ACCESS, false, interate.ProcessId)) {
							TerminateProcess(hThread, -1);
							CloseHandle(hThread);
						}
					}
				}
			},
			nullptr
		);
	}
	

	void killF(std::wstring& filter) {
		std::vector<process_info_no_child> tmp = get_from_filter<process_info_no_child>(filter);

		for_thread(tmp, [](process_info_no_child& interate, size_t i) {
			if (HANDLE hThread = OpenProcess(PROCESS_ALL_ACCESS, false, interate.process_id)) {
				TerminateProcess(hThread, -1);
				CloseHandle(hThread);
			}
		});
	}
}

namespace freeze_cmd {
	void freeze(std::wstring options) {
		windows_enum::enum_all_process(
			[options](WTS_PROCESS_INFOW& interate, size_t) {
				if (interate.pProcessName) {
					if (interate.pProcessName == options) {
						if (HANDLE hThread = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, interate.ProcessId)) {
							NtSuspendProcess(hThread);
							CloseHandle(hThread);
						}
					}
				}
			},
			nullptr
		);
	}
	void unfreeze(std::wstring options) {
		windows_enum::enum_all_process(
			[options](WTS_PROCESS_INFOW& interate, size_t) {
				if (interate.pProcessName) {
					if (interate.pProcessName == options) {
						if (HANDLE hThread = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, interate.ProcessId)) {
							NtResumeProcess(hThread);
							CloseHandle(hThread);
						}
					}
				}
			},
			nullptr
			);
	}

	void freezeF(std::wstring& filter) {
		std::vector<process_info_no_child> tmp = get_from_filter<process_info_no_child>(filter);

		for_thread(tmp, [](process_info_no_child& interate, size_t i) {
			if (HANDLE hThread = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, interate.process_id)) {
				NtSuspendProcess(hThread);
				CloseHandle(hThread);
			}
		});
	}
	void unfreezeF(std::wstring& filter) {
		std::vector<process_info_no_child> tmp = get_from_filter<process_info_no_child>(filter);

		for_thread(tmp, [](process_info_no_child& interate, size_t i) {
			if (HANDLE hThread = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, interate.process_id)) {
				NtResumeProcess(hThread);
				CloseHandle(hThread);
			}
		});
	}
}

void summon(std::wstring options,std::wstring privlegies) {
	if (privlegies == L"a" || privlegies == L"A")
		ShellExecuteW(nullptr, L"runas", options.c_str(), NULL, NULL, SW_RESTORE);
	//else if(privlegies == L"u" || privlegies == L"U") {
	//	if(_wsystem((L"RUNAS /trustlevel:0x20000 " + options).c_str())) 
	//		ShellExecuteW(nullptr, L"open", options.c_str(), NULL, NULL, SW_RESTORE);
	//}
	else
		ShellExecuteW(nullptr, L"open", options.c_str(), NULL, NULL, SW_RESTORE);
}

void run_cmd(std::vector<std::wstring>& cmds) {
	if (cmds[0] == L"-?") help();
	else if (cmds[0] == L"help") help();
	else if (cmds[0] == L"list") list(cmds[1], cmds[2]);
	else if (cmds[0] == L"kill") kill_cmd::kill(cmds[1]);
	else if (cmds[0] == L"killF") kill_cmd::killF(cmds[1]);
	else if (cmds[0] == L"summon") summon(cmds[1], cmds[2]);
	else if (cmds[0] == L"freeze") freeze_cmd::freeze(cmds[1]);
	else if (cmds[0] == L"unfreeze") freeze_cmd::unfreeze(cmds[1]);
	else if (cmds[0] == L"freezeF") freeze_cmd::freezeF(cmds[1]);
	else if (cmds[0] == L"unfreezeF") freeze_cmd::unfreezeF(cmds[1]);
	else {
		std::wcout <<
			"Invalid argument, not found command\n";
		help();
	}
}

int wmain(int agrc, const wchar_t* agrv[]){
	init_locale();
	if (agrc > 1) {
		std::vector<std::wstring>cmd(agrv, agrv + agrc);
		cmd.erase(cmd.begin());
		cmd.resize(4);
		try {
			run_cmd(cmd);
		}
		catch (filter::Invalid_filter& f) {
			std::wcout <<"Exception(Invalid_filter): "<< f.reason();
		}
		catch (std::exception& f) {
			std::wcout << "\nException(std::exception): " << f.what()<<"\n";
		}
		catch (...) {
			std::wcout << "\nException(undefined): " << "\n";
			throw;
		}
		return 0;
	}
	else help();
	return 0;
}
