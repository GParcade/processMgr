#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <Windows.h>
#include <psapi.h>
#include <wtsapi32.h>
#include <locale.h>
#include <fcntl.h>
#include <io.h>
#include "filter.hpp"
#include "user_tool.hpp"
#include "multi_thread_for.hpp"
#include "string_tool.hpp"
#include "windows_enum.hpp"
#include "console.hpp"
#pragma comment(lib, "advapi32")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "Wtsapi32.lib")


#if ( _MSC_VER || __MINGW32__ || __MSVCRT__ )
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
#else
	void init_locale() {
		constexpr char locale_name[] = "";
		setlocale(LC_ALL, locale_name);
		std::locale::global(std::locale(locale_name));
		std::wcin.imbue(std::locale());
		std::wcout.imbue(std::locale());
	}

#endif


void help_commads() {
	std::wcout << reset_color <<
		"Commands:\n"
		"\t  list " << rgb_color_text(207, 207, 207) << "{sorter} {filters}\n\n" << reset_color <<
		"\t  detal_list " << rgb_color_text(207, 207, 207) << "{sorter} {filters}\n\n" << reset_color <<
		"\t  kill \"name\"\n"
		"\t  killF {filters}\n\n"
		"\t  summon \"path\" " << rgb_color_text(207, 207, 207) << "{privilege}\n\n" << reset_color <<
		"\t  info \"name\"\n"
		"\t  \tShow information about preces/ses\n\n"
		"\t  freze \"name\"\n"
		"\t  frezeF {filters}\n"
		"\t  \tPause proces/ses\n\n"
		"\t  unfreze \"name\"\n"
		"\t  unfrezeF {filters}\n"
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
//		<< rgb_color_text(80, 140, 255) << "W" << reset_color << "names{\"1\",\"2\",\"3\"} ->  process windows name filter\n\n\t "     to-do
	<< rgb_color_text(255, 0, 0) << "G" << reset_color << "roup{\'1\',\"2\",\"3\"} ->  group filter\n"
		"\t\t (empty)       ->  all[default]\n"
		"\t\t *group_name*  ->  add to filter this group\n\n\t "
	<< rgb_color_text(255, 0, 0) << "D" << reset_color << "omain{\'1\',\"2\",\"3\"} ->  domain filter\n"
		"\t\t (empty)       ->  all[default]\n"
		"\t\t *domain_name*  ->  add to filter this domain\n\n\t "
	<< rgb_color_text(180, 255, 30) << "M" << reset_color <<
		"isk{1,2,3} ->  Misc filter\n"
		"\t\t priority{}             -> All ptioritys: {realtime,height,heigh_avg,default,low_avg,low}\n"
		"\t\t cpu_min{\"float 0-100\"}   -> Selects all processes using CPU load from percent\n"
		"\t\t cpu_max{\"float 0-100\"}   -> Selects all processes using CPU load to percent\n"
		"\t\t evaluated              -> Select process evaluated to admin rights\n\n\t "
	<< rgb_color_text(255, 180, 30) << "C" << reset_color <<
		"onfig{1,2,3} ->  filter configuration\n"
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
		"List options: 123\n"
		"\t    ->  use default\n"
		"\t  - ->  use default\n"
		"\t  i ->  id(sort)[default]\n"
		"\t  n ->  names(sort)\n"
		"\t  c ->  childs(visual)[default]\n"
		"\t  C ->  unique color(visual)\n\n"
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
	processed_filter f(str_filter);
	std::vector<process_type> result = windows_enum::get_process_list<process_type>();
	if (!f.no_filters) {
		Filter main_f(result.size(), true);
		if (!f.name_index.ids.empty()) {
			for_thread(result, [&main_f, &f](process_type& info, size_t i) {

				main_f[i] = exist(f.name_index.ids, (uint64_t)info.process_id);

			});
		}

		if (f.config.similar) {
			if (f.name_index.names.size()/* || f.name_index.wnames.size()*/) {
				Filter name_f(main_f);
				if (f.name_index.names.size()) {

					for_debug(result, [&name_f, &f](process_type& info, size_t i) {
						if (name_f[i])
							name_f[i] = name_f[i] & contain_similar(f.name_index.names, *info.proc_name);
						});

				}

				//if (f.name_index.wnames.size()) {
				//
				//	for_debug(result, [&name_f, &f](process_info& info, size_t i) {
				//		if (name_f[i])
				//			name_f[i] = name_f[i] & windows_enum::windows_contain_similar((uint64_t)info.process_id, f.name_index.wnames);
				//		});
				//
				//}

				main_f.apply(name_f);
			}
		}
		else if (f.name_index.names.size()/* || f.name_index.wnames.size()*/) {
			Filter name_f(main_f);
			if (f.name_index.names.size()) {

				for_thread(result, [&name_f, &f](process_type& info, size_t i) {
					if (name_f[i])
						name_f[i] = name_f[i] & contain(f.name_index.names, *info.proc_name);
					});

			}

			//if (f.name_index.wnames.size()) {
			//
			//	for_thread(result, [&name_f, &f](process_info& info, size_t i) {
			//		if (name_f[i])
			//			name_f[i] = name_f[i] & windows_enum::windows_contain((uint64_t)info.process_id, f.name_index.wnames);
			//		});
			//
			//}

			main_f.apply(name_f);
		}

		if (f.started_by.current_user || !f.started_by.domains.empty() || !f.started_by.names.empty()) {
			Filter user_f(result.size(),false);

			if (f.started_by.current_user) {
				for_thread(result, [&user_f,&main_f](process_type& info, size_t i) {
					if (main_f[i]) {
						user_f[i] = (*info.summoner_domain == user::current_domain() && *info.summoner_name == user::current_name());
					}
				});
			}
			if (!f.started_by.names.empty()) {
				if (f.config.similar) {
					for_thread(result, [&user_f, &f](process_type& info, size_t i) {
						if (user_f[i])
							user_f[i] = user_f[i] || contain_similar(f.started_by.names, *info.summoner_name);
						});
				}
				else {
					for_thread(result, [&user_f, &f](process_type& info, size_t i) {
						if (user_f[i])
							user_f[i] = user_f[i] || contain(f.started_by.names, *info.summoner_name);
						});
				}
			}
			if (!f.started_by.domains.empty()) {
				if (f.config.similar) {
					for_thread(result, [&user_f, &f](process_type& info, size_t i) {
						if (user_f[i])
							user_f[i] = user_f[i] || contain_similar(f.started_by.domains, *info.summoner_domain);
						});
				}
				else {
					for_thread(result, [&user_f, &f](process_type& info, size_t i) {
						if (user_f[i])
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

namespace list_cmd {
#define Unique_color_s(str) {if(! (str).is_unique()) std::wcout<<unique_rgb_color(*(str));}
#define Unique_color_i(str) {std::wcout<<unique_rgb_color((str));}

	void list_present(std::vector<process_info_no_child>& process, bool color) {
		size_t max_proc_id_len = 3;
		size_t max_proc_nam_len = 5;
		size_t max_proc_ses_len = 10;
		size_t max_proc_udom_len = 17;
		size_t max_proc_unam_len = 14;
		{
			size_t tmp;
			for (auto& info : process) {
				tmp = std::to_string(info.process_id).size();
				if (max_proc_id_len < tmp) max_proc_id_len = tmp + 2;

				tmp = info.proc_name->size();
				if (max_proc_nam_len < tmp) max_proc_nam_len = tmp + 1;

				tmp = info.summoner_name->size();
				if (max_proc_unam_len < tmp) max_proc_unam_len = tmp + 1;

				tmp = std::to_string(info.session_id).size();
				if (max_proc_ses_len < tmp) max_proc_ses_len = tmp + 1;

				tmp = info.summoner_domain->size();
				if (max_proc_udom_len < tmp) max_proc_udom_len = tmp + 1;
			}
		}

		max_proc_udom_len += max_proc_id_len + max_proc_nam_len + max_proc_unam_len + max_proc_ses_len;
		max_proc_ses_len += max_proc_id_len + max_proc_nam_len + max_proc_unam_len;
		max_proc_unam_len += max_proc_id_len + max_proc_nam_len;
		max_proc_nam_len += max_proc_id_len + 1;

		std::wcout << "Id" << set_pos_in_line(max_proc_id_len);

		std::wcout << "Name" << set_pos_in_line(max_proc_nam_len);

		std::wcout << "Session №" << set_pos_in_line(max_proc_udom_len);

		std::wcout << "Summoner Domain" << set_pos_in_line(max_proc_unam_len);

		std::wcout << "Summoner Name";

		std::wcout << '\n';
		for (auto& info : process) {
			std::wcout << info.process_id;
			std::wcout << set_pos_in_line(max_proc_id_len);

			if (color)Unique_color_s(info.proc_name);
			std::wcout << *info.proc_name << reset_color;

			std::wcout << set_pos_in_line(max_proc_nam_len);

			if (color)Unique_color_i(info.session_id);
			std::wcout << info.session_id;
			std::wcout << set_pos_in_line(max_proc_udom_len);

			if (color)Unique_color_s(info.summoner_domain);
			std::wcout << *info.summoner_domain << reset_color;
			std::wcout << set_pos_in_line(max_proc_unam_len);

			if (color)Unique_color_s(info.summoner_name);
			std::wcout << *info.summoner_name << reset_color;

			std::wcout << '\n';
		}
	}

	void list_present(process_three_info& process, bool color) {
		size_t max_proc_id_len = 3;
		size_t max_proc_nam_len = 5;
		size_t max_proc_ses_len = 10;
		size_t max_proc_udom_len = 17;
		size_t max_proc_unam_len = 14;
		{
			size_t tmp;
			for (auto& info : process) {
				tmp = std::to_string(info.dad.process_id).size();
				if (max_proc_id_len < tmp) max_proc_id_len = tmp + 2;
				for (auto& info0 : info.childs) {
					tmp = std::to_string(info0.process_id).size();
					if (max_proc_id_len < tmp) max_proc_id_len = tmp + 2;
				}

				tmp = info.dad.proc_name->size();
				if (max_proc_nam_len < tmp) max_proc_nam_len = tmp + 1;
				for (auto& info0 : info.childs) {
					tmp = info0.proc_name->size();
					if (max_proc_nam_len < tmp) max_proc_nam_len = tmp + 2;
				}

				tmp = info.dad.summoner_name->size();
				if (max_proc_unam_len < tmp) max_proc_unam_len = tmp + 1;
				for (auto& info0 : info.childs) {
					tmp = info0.summoner_name->size();
					if (max_proc_unam_len < tmp) max_proc_unam_len = tmp + 2;
				}

				tmp = std::to_string(info.dad.session_id).size();
				if (max_proc_ses_len < tmp) max_proc_ses_len = tmp + 1;
				for (auto& info0 : info.childs) {
					tmp = std::to_string(info0.session_id).size();
					if (max_proc_ses_len < tmp) max_proc_ses_len = tmp + 2;
				}

				tmp = info.dad.summoner_domain->size();
				if (max_proc_udom_len < tmp) max_proc_udom_len = tmp + 1;
				for (auto& info0 : info.childs) {
					tmp = info0.summoner_domain->size();
					if (max_proc_udom_len < tmp) max_proc_udom_len = tmp + 2;
				}
			}
		}

		max_proc_udom_len += max_proc_id_len + max_proc_nam_len + max_proc_unam_len + max_proc_ses_len;
		max_proc_ses_len += max_proc_id_len + max_proc_nam_len + max_proc_unam_len;
		max_proc_unam_len += max_proc_id_len + max_proc_nam_len;
		max_proc_nam_len += max_proc_id_len + 1;

		std::wcout << "Id" << set_pos_in_line(max_proc_id_len);

		std::wcout << "Name" << set_pos_in_line(max_proc_nam_len - 3);

		std::wcout << "Session №" << set_pos_in_line(max_proc_udom_len);

		std::wcout << "Summoner Domain" << set_pos_in_line(max_proc_unam_len);

		std::wcout << "Summoner Name";

		std::wcout << '\n';
		for (auto& info : process) {
			std::wcout << info.dad.process_id;
			std::wcout << set_pos_in_line(max_proc_id_len);

			if (color)Unique_color_s(info.dad.proc_name);
			std::wcout << *info.dad.proc_name << reset_color;
			std::wcout << set_pos_in_line(max_proc_nam_len);

			if (color)Unique_color_i(info.dad.session_id);
			std::wcout << info.dad.session_id;
			std::wcout << set_pos_in_line(max_proc_udom_len);

			if (color)Unique_color_s(info.dad.summoner_domain);
			std::wcout << *info.dad.summoner_domain << reset_color;
			std::wcout << set_pos_in_line(max_proc_unam_len);

			if (color)Unique_color_s(info.dad.summoner_name);
			std::wcout << *info.dad.summoner_name << reset_color;

			std::wcout << '\n';
			for (auto& info0 : info.childs) {
				std::wcout << info0.process_id;
				std::wcout << set_pos_in_line(max_proc_id_len + 4);

				if (color)Unique_color_s(info0.proc_name);
				std::wcout << *info0.proc_name << reset_color;
				std::wcout << set_pos_in_line(max_proc_nam_len);

				if (color)Unique_color_i(info0.session_id);
				std::wcout << info0.session_id;
				std::wcout << set_pos_in_line(max_proc_udom_len);

				if (color)Unique_color_s(info0.summoner_domain);
				std::wcout << *info0.summoner_domain << reset_color;
				std::wcout << set_pos_in_line(max_proc_unam_len);

				if (color)Unique_color_s(info0.summoner_name);
				std::wcout << *info0.summoner_name << reset_color;
				std::wcout << '\n';
			}
		}





	}

#undef Unique_color_s
#undef Unique_color_i

	void list(std::wstring options, std::wstring filter) {
		bool id = 0;
		bool name = 0;
		bool child = 0;
		bool color = 0;
		if (options.empty() || options == L" " || options == L"-") {
			id = 1;
			child = 1;
		}
		if (contain(options, std::wstring(L"i")))
			id = 1;
		if (contain(options, std::wstring(L"n")))
			name = 1;
		if (contain(options, std::wstring(L"c")))
			child = 1;
		if (contain(options, std::wstring(L"C")))
			color = 1;

		if (child) {
			process_three_info res(get_from_filter<process_info>(filter));
			if (id) {
				std::sort(res.begin(), res.end(), [](process_three_info::three& val_0, process_three_info::three& val_1) {
					return val_0.dad.process_id < val_1.dad.process_id;
					}
				);
				for (auto& chi : res) {
					if (chi.childs.size()) {
						std::sort(chi.childs.begin(), chi.childs.end(), [](process_info_no_child& val_0, process_info_no_child& val_1) {
							return val_0.process_id < val_1.process_id;
							}
						);
					}
				}
			}
			else if (name) {
				std::sort(res.begin(), res.end(), [](process_three_info::three& val_0, process_three_info::three& val_1) {
					return *val_0.dad.proc_name < *val_1.dad.proc_name;
					}
				);
				for (auto& chi : res) {
					if (chi.childs.size()) {
						std::sort(chi.childs.begin(), chi.childs.end(), [](process_info_no_child& val_0, process_info_no_child& val_1) {
							return *val_0.proc_name < *val_1.proc_name;
							}
						);
					}
				}
			}
			list_present(res, color);
		}
		else {
			auto tmp = get_from_filter<process_info_no_child>(filter);
			if (id) {
				std::sort(tmp.begin(), tmp.end(), [](process_info_no_child& val_0, process_info_no_child& val_1) {
					return val_0.process_id < val_1.process_id;
					}
				);
			}
			else if (name) {
				std::sort(tmp.begin(), tmp.end(), [](process_info_no_child& val_0, process_info_no_child& val_1) {
					return *val_0.proc_name < *val_1.proc_name;
					}
				);
			}
			list_present(tmp, color);
		}
	}

}

namespace kill_cmd {
	void kill(std::wstring& options) {
		windows_enum::enum_all_process(
			[options](WTS_PROCESS_INFOW& interate, size_t i) {
				if (interate.pProcessName) {
					if (interate.pProcessName == options) {
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
						if (HANDLE hThread = OpenProcess(THREAD_ALL_ACCESS, FALSE, interate.ProcessId)) {
							SuspendThread(hThread);
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
						if (HANDLE hThread = OpenProcess(THREAD_ALL_ACCESS, FALSE, interate.ProcessId)) {
							ResumeThread(hThread);
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
			if (HANDLE hThread = OpenProcess(THREAD_ALL_ACCESS, FALSE, interate.process_id)) {
				SuspendThread(hThread);
				CloseHandle(hThread);
			}
		});
	}
	void unfreezeF(std::wstring& filter) {
		std::vector<process_info_no_child> tmp = get_from_filter<process_info_no_child>(filter);

		for_thread(tmp, [](process_info_no_child& interate, size_t i) {
			if (HANDLE hThread = OpenProcess(THREAD_ALL_ACCESS, FALSE, interate.process_id)) {
				ResumeThread(hThread);
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

namespace info_cmd{
	void show_info(DWORD procID,std::wstring procName, std::wstring summoner_name, std::wstring summoner_domain,size_t session) {
		if (HANDLE hThread = OpenProcess(THREAD_ALL_ACCESS, FALSE, procID)) {

			std::wstring is_under_the_grip_of_a_debugger;
			std::wstring is_above_WOW64_emulator;
			std::wstring is_critical;
			PROCESS_MEMORY_COUNTERS memDetals;
			BOOL result = GetProcessMemoryInfo(hThread,
				&memDetals,
				sizeof(memDetals));
			size_t threads_count = windows::theads_count(procID);
			BOOL tmp_bool;

			if (!CheckRemoteDebuggerPresent(hThread, &tmp_bool))
				is_under_the_grip_of_a_debugger = L"N/A";
			else
				is_under_the_grip_of_a_debugger = tmp_bool ? L"true" : L"false";

			if (!IsWow64Process(hThread, &tmp_bool))
				is_above_WOW64_emulator = L"N/A";
			else
				is_above_WOW64_emulator = tmp_bool ? L"true" : L"false";

			if (!IsProcessCritical(hThread, &tmp_bool))
				is_critical = L"N/A";
			else
				is_critical = tmp_bool ? L"true" : L"false";

			std::wcout <<
				"Process information: " << procID << "\n"
				"   Basic: \n"
				"       Name: " << procName ; std::wcout << "\n"
				"       Summoner name: " << summoner_name << "\n"
				"       Summoner domain:" << summoner_domain << "\n"
				"       Session id: " << session << "\n"
				"   Memory: \n"
				"       Working size: " << memDetals.WorkingSetSize << "\n"
				"       Peak working size: " << memDetals.PeakWorkingSetSize << "\n"
				"       Page faults count: " << memDetals.PageFaultCount << "\n"
				"   Detal: \n"
				"       EmulateWOW64: " << is_above_WOW64_emulator << "\n"
				"       UnderDebug: " << is_under_the_grip_of_a_debugger << "\n"
				"       Critical: " << is_critical << "\n"

				;
			CloseHandle(hThread);
		}
		else
			std::wcout << "Fail get process information without handle, windows error code: 0x" << std::hex << GetLastError() << std::dec;
	}

	void info(std::wstring name) {
		windows_enum::enum_all_process_slow([name](WTS_PROCESS_INFOW& interate, size_t) {
			if (interate.pProcessName)
				if (interate.pProcessName == name)
					show_info(
						interate.ProcessId,
						(interate.pProcessName ? interate.pProcessName : L"N/A"),
						(interate.pUserSid ? user::get_name(interate.pUserSid) : L"N/A"),
						(interate.pUserSid ? user::get_domain(interate.pUserSid) : L"N/A"),
						interate.SessionId
					);
			},
			nullptr
		);
	}
	void infoF(std::wstring filter) {
		std::vector<process_info_no_child> tmp = get_from_filter<process_info_no_child>(filter);
		for(auto& interate : tmp)
			show_info(
				interate.process_id,
				(*interate.proc_name!=L"" ? *interate.proc_name : std::wstring(L"N/A")),
				(*interate.summoner_name !=L"" ? *interate.summoner_name : std::wstring(L"N/A")),
				(*interate.summoner_domain!=L"" ? *interate.summoner_domain : std::wstring(L"N/A")),
				interate.session_id
			);
	}
}

void run_cmd(std::vector<std::wstring>& cmds) {
	if (cmds[0] == L"-?") help();
	else if (cmds[0] == L"help") help();
	else if (cmds[0] == L"list") list_cmd::list(cmds[1], cmds[2]);
	else if (cmds[0] == L"info") info_cmd::info(cmds[1]);
	else if (cmds[0] == L"infoF") info_cmd::infoF(cmds[1]);
	else if (cmds[0] == L"kill") kill_cmd::kill(cmds[1]);
	else if (cmds[0] == L"killF") kill_cmd::killF(cmds[1]);
	else if (cmds[0] == L"summon") summon(cmds[1], cmds[2]);
	else if (cmds[0] == L"freeze") freeze_cmd::freeze(cmds[1]);
	else if (cmds[0] == L"unfreeze") freeze_cmd::unfreeze(cmds[1]);
	else if (cmds[0] == L"freezeF") freeze_cmd::freezeF(cmds[1]);
	else if (cmds[0] == L"unfreezeF") freeze_cmd::unfreezeF(cmds[1]);
	else {
		std::cout <<
			"Invalid argument, not found command\n";
		help();
	}
}

int wmain(int agrc, const wchar_t* agrv[]){
	init_locale();
	if (agrc > 1) {
		std::vector<std::wstring>cmd(agrv, agrv + agrc);
		cmd.erase(cmd.begin());
		cmd.resize(3);
		try {
			run_cmd(cmd);
		}
		catch (Invalid_filter& f) {
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
