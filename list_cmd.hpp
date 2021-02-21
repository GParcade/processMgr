#pragma once
#include <iostream>
#include <algorithm>
#include <mutex>
#include <vector>
#include "console.hpp"
#include "windows_enum.hpp"
#include "filter.hpp"
#include <psapi.h>
namespace list_cmd {

	class info_data {
		public:
			std::wstring is_under_the_grip_of_a_debugger;
			std::wstring is_above_WOW64_emulator;
			std::wstring is_critical;
			std::wstring is_evaluated;
			PROCESS_MEMORY_COUNTERS memDetals;
			size_t threads_count;

			std::wstring summoner_name;
			std::wstring summoner_domain;
			std::wstring procName;
			DWORD procID;
			size_t session;
			std::shared_ptr<std::vector<info_data>> childs;

			info_data(process_info_no_child proc) {
				if (HANDLE hThread = OpenProcess(THREAD_ALL_ACCESS, FALSE, procID)) {
					BOOL result = GetProcessMemoryInfo(hThread,
						&memDetals,
						sizeof(memDetals));
					BOOL tmp_bool;
					threads_count = proc.threads;
					if (!CheckRemoteDebuggerPresent(hThread, &tmp_bool))
						is_under_the_grip_of_a_debugger = L" ";
					else
						is_under_the_grip_of_a_debugger = tmp_bool ? L"D" : L"-";

					if (!IsWow64Process(hThread, &tmp_bool))
						is_above_WOW64_emulator = L" ";
					else
						is_above_WOW64_emulator = tmp_bool ? L"W" : L"-";

					if (!IsProcessCritical(hThread, &tmp_bool))
						is_critical = L" ";
					else
						is_critical = tmp_bool ? L"C" : L"-";
				}
				else {
					is_under_the_grip_of_a_debugger = L" ";
					is_above_WOW64_emulator = L" ";
					threads_count = 0;
					is_critical = L" ";
					memDetals.PageFaultCount = 0;
					memDetals.PagefileUsage = 0;
					memDetals.PeakPagefileUsage = 0;
					memDetals.PeakWorkingSetSize = 0;
					memDetals.QuotaNonPagedPoolUsage = 0;
					memDetals.QuotaPagedPoolUsage = 0;
					memDetals.QuotaPeakNonPagedPoolUsage = 0;
					memDetals.QuotaPeakPagedPoolUsage = 0;
					memDetals.WorkingSetSize = 0;
				}
				switch (user::IsElevated(proc.process_id)) {
				case -1:
					is_evaluated = L" ";
					break;
				case 0:
					is_evaluated = L"-";
					break;
				case 1:
					is_evaluated = L"E";
				default:
					break;
				}
				procName =*proc.proc_name;
				summoner_name = *proc.summoner_name;
				summoner_domain = *proc.summoner_domain;
				procID = proc.process_id;
				session=proc.session_id;
			}

			info_data(std::vector<process_info>& list, process_info check) {
				*this = { check };
				if (list.size()) {
					if (check.childs_id.size()) {
						childs = std::shared_ptr<std::vector<info_data>>(new std::vector<info_data>);
						for (int64_t i = list.size() - 1; i >= 0; i--) {
							if (contain_value(check.childs_id, list[i].process_id)) {
								childs->push_back({ list[i] });
								list.erase(list.begin() + i);
							}
						}
					}
				}
			}
			info_data(const info_data& copy) {
				is_under_the_grip_of_a_debugger		=	copy.is_under_the_grip_of_a_debugger;
				is_above_WOW64_emulator				=	copy.is_above_WOW64_emulator		;
				is_critical							=	copy.is_critical					;
				memDetals							=	copy.memDetals						;
				threads_count						=	copy.threads_count					;
				summoner_name						=	copy.summoner_name					;
				summoner_domain						=	copy.summoner_domain				;
				procName							=	copy.procName						;
				procID								=	copy.procID							;
				session								=	copy.session						;
				childs								=	copy.childs;
				is_evaluated						=	copy.is_evaluated;
			}
		};
	struct sort_flags {
		bool
			pid : 1,
			nam : 1,
			summoner_name,
			summoner_domain,
			session,
			threads : 1,
			flags : 1,
			work_mem : 1,
			peak_work_mem : 1,
			page_faults : 1,
			page_file_use : 1,
			peak_page_file_use : 1,
			quota_non_paged_pool_use : 1,
			quota_paged_pool_use : 1,
			quota_non_peak_paged_pool_use : 1,
			quota_peak_paged_pool_use : 1;
		bool direction = 0;
	};

	template <class _Pr>
	void sort_with_childs(std::vector<info_data>& con, const _Pr last) {
		std::sort(con.begin(), con.end(), [last](info_data& val_0, info_data& val_1) {
			if (val_0.childs) {
				if (val_0.childs->size())
					std::sort(val_0.childs->begin(), val_0.childs->end(), [last](info_data& val_0, info_data& val_1) {
							return last(val_0, val_1);
						}
					);
			}
			if (val_1.childs) {
				if(val_1.childs->size())
					std::sort(val_1.childs->begin(), val_1.childs->end(), [last](info_data& val_0, info_data& val_1) {
							return last(val_0, val_1);
						}
					);
			}
			return last(val_0, val_1);
			}
		);
	}



	template <bool direction>
	void do_sort(sort_flags& sort, std::vector<info_data>& vec) {
		if (sort.pid) {
			sort_with_childs(vec,[](info_data& val_0, info_data& val_1) {
					if constexpr (direction) return val_0.procID < val_1.procID;
					else return val_0.procID > val_1.procID;
				}
			);
		}
		else if (sort.nam) {
			sort_with_childs(vec, [](info_data& val_0, info_data& val_1) {
				if constexpr (direction) return val_0.procName < val_1.procName;
				else return val_0.procName > val_1.procName;
				}
			);
		}
		else if (sort.summoner_name) {
			sort_with_childs(vec, [](info_data& val_0, info_data& val_1) {
				if constexpr (direction) return val_0.summoner_name < val_1.summoner_name;
				else return val_0.summoner_name > val_1.summoner_name;
				}
			);
		}
		else if (sort.summoner_domain) {
			sort_with_childs(vec, [](info_data& val_0, info_data& val_1) {
				if constexpr (direction) return val_0.summoner_domain < val_1.summoner_domain;
				else return val_0.summoner_domain > val_1.summoner_domain;
				}
			);
		}
		else if (sort.threads) {
			sort_with_childs(vec, [](info_data& val_0, info_data& val_1) {
				if constexpr (direction) return val_0.threads_count < val_1.threads_count;
				else return val_0.threads_count > val_1.threads_count;
				}
			);
		}
		else if (sort.session) {
			sort_with_childs(vec, [](info_data& val_0, info_data& val_1) {
				if constexpr (direction) return val_0.session < val_1.session;
				else return val_0.session > val_1.session;
				}
			);
		}
		else if (sort.work_mem) {
			sort_with_childs(vec, [](info_data& val_0, info_data& val_1) {
				if constexpr (direction) return val_0.memDetals.WorkingSetSize < val_1.memDetals.WorkingSetSize;
				else return val_0.memDetals.WorkingSetSize > val_1.memDetals.WorkingSetSize;
				}
			);
		}
		else if (sort.page_faults) {
			sort_with_childs(vec, [](info_data& val_0, info_data& val_1) {
				if constexpr (direction) return val_0.memDetals.PageFaultCount < val_1.memDetals.PageFaultCount;
				else return val_0.memDetals.PageFaultCount > val_1.memDetals.PageFaultCount;
				}
			);
		}
		else if (sort.page_file_use) {
			sort_with_childs(vec, [](info_data& val_0, info_data& val_1) {
				if constexpr (direction) return val_0.memDetals.PagefileUsage < val_1.memDetals.PagefileUsage;
				else return val_0.memDetals.PagefileUsage > val_1.memDetals.PagefileUsage;
				}
			);
		}
		else if (sort.peak_work_mem) {
			sort_with_childs(vec, [](info_data& val_0, info_data& val_1) {
				if constexpr (direction) return val_0.memDetals.PeakWorkingSetSize < val_1.memDetals.PeakWorkingSetSize;
				else return val_0.memDetals.PeakWorkingSetSize > val_1.memDetals.PeakWorkingSetSize;
				}
			);
		}
		else if (sort.quota_non_paged_pool_use) {
			sort_with_childs(vec, [](info_data& val_0, info_data& val_1) {
				if constexpr (direction) return val_0.memDetals.QuotaNonPagedPoolUsage < val_1.memDetals.QuotaNonPagedPoolUsage;
				else return val_0.memDetals.QuotaNonPagedPoolUsage > val_1.memDetals.QuotaNonPagedPoolUsage;
				}
			);
		}
		else if (sort.quota_non_peak_paged_pool_use) {
			sort_with_childs(vec, [](info_data& val_0, info_data& val_1) {
				if constexpr (direction) return val_0.memDetals.QuotaPeakNonPagedPoolUsage < val_1.memDetals.QuotaPeakNonPagedPoolUsage;
				else return val_0.memDetals.QuotaPeakNonPagedPoolUsage > val_1.memDetals.QuotaPeakNonPagedPoolUsage;
				}
			);
		}
		else if (sort.quota_paged_pool_use) {
			sort_with_childs(vec, [](info_data& val_0, info_data& val_1) {
				if constexpr (direction) return val_0.memDetals.QuotaPagedPoolUsage < val_1.memDetals.QuotaPagedPoolUsage;
				else return val_0.memDetals.QuotaPagedPoolUsage > val_1.memDetals.QuotaPagedPoolUsage;
				}
			);
		}
		else if (sort.quota_peak_paged_pool_use) {
			sort_with_childs(vec, [](info_data& val_0, info_data& val_1) {
				if constexpr (direction) return val_0.memDetals.QuotaPeakPagedPoolUsage < val_1.memDetals.QuotaPeakPagedPoolUsage;
				else return val_0.memDetals.QuotaPeakPagedPoolUsage > val_1.memDetals.QuotaPeakPagedPoolUsage;
				}
			);
		}
	}

	class list_dethal {
		
		struct out_flags {
			bool
				pid:1,
				nam:1,
				summoner_name:1,
				summoner_domain:1,
				session:1,
				threads:1,
				flags:1,
				work_mem:1,
				peak_work_mem:1,
				page_faults:1,
				page_file_use:1,
				peak_page_file_use:1,
				quota_non_paged_pool_use:1,
				quota_paged_pool_use:1,
				quota_non_peak_paged_pool_use:1,
				quota_peak_paged_pool_use:1;
			bool use_color:1;
		};

		std::pair<out_flags, sort_flags> out_flag_parser(std::wstring& filter) {
			out_flags out_res;
			sort_flags sort_res;
			//set null all flags
			{
				char* proxy = (char*)&out_res;
				for (size_t i = 0; i < sizeof(out_flags); i++)
					proxy[i] = 0;
				proxy = (char*)&sort_res;
				for (size_t i = 0; i < sizeof(out_flags); i++)
					proxy[i] = 0;
			}
			if (filter.empty() || filter == L"-") {
				out_res.nam = 1;
				out_res.pid = 1;
				out_res.summoner_name = 1;
				out_res.session = 1;
				out_res.work_mem = 1;
				out_res.use_color = 1;
				sort_res.nam = 1;
			}
			else {
				std::wstring token;
				size_t filter_size = filter.size();
				bool has_next = 0;
				for (size_t i = 0; i < filter_size; i++) {
					token = filter::get_token(filter, filter_size, i, has_next);
					if (token == L"Out" || token == L"O") {
						do {
							token = filter::get_token(filter, filter_size, i, has_next);
							if (token.empty())
								token = filter::get_token(filter, filter_size, i, has_next);
							else if (token == L"name")		 out_res.nam = 1;
							else if (token == L"proc_id")	 out_res.pid = 1;
							else if (token == L"page_fault") out_res.page_faults = 1;
							else if (token == L"page_use")   out_res.page_file_use = 1;
							else if (token == L"peak_work")	 out_res.peak_work_mem = 1;
							else if (token == L"qnppu")		 out_res.quota_non_paged_pool_use = 1;
							else if (token == L"qnpppu")	 out_res.quota_non_peak_paged_pool_use = 1;
							else if (token == L"qppu")		 out_res.quota_paged_pool_use = 1;
							else if (token == L"qpppu")		 out_res.quota_peak_paged_pool_use = 1;
							else if (token == L"session")	 out_res.session = 1;
							else if (token == L"creator_dom")out_res.summoner_domain = 1;
							else if (token == L"creator_nam")out_res.summoner_name = 1;
							else if (token == L"threads")	 out_res.threads = 1;
							else if (token == L"work_mem")	 out_res.work_mem = 1;
							else if (token == L"use_color")	 out_res.use_color = 1;
							else if (token == L"flags")		 out_res.flags = 1;
							else if (token == L"-") {
															 out_res.nam = 1;
															 out_res.pid = 1;
															 out_res.summoner_name = 1;
															 out_res.session = 1;
															 out_res.work_mem = 1;
															 out_res.use_color = 1;
							}
						} while (has_next && i < filter_size);
					}
					if (token == L"Sort" || token == L"S") {
						do {
							token = filter::get_token(filter, filter_size, i, has_next);
							if (token.empty())
								token = filter::get_token(filter, filter_size, i, has_next);
							else if (token == L"name")				sort_res.nam = 1;
							else if (token == L"proc_id")			sort_res.pid = 1;
							else if (token == L"page_fault")		sort_res.page_faults = 1;
							else if (token == L"page_use")			sort_res.page_file_use = 1;
							else if (token == L"peak_work")			sort_res.peak_work_mem = 1;
							else if (token == L"qnppu")				sort_res.quota_non_paged_pool_use = 1;
							else if (token == L"qnpppu")			sort_res.quota_non_peak_paged_pool_use = 1;
							else if (token == L"qppu")				sort_res.quota_paged_pool_use = 1;
							else if (token == L"qpppu")				sort_res.quota_peak_paged_pool_use = 1;
							else if (token == L"session")			sort_res.session = 1;
							else if (token == L"creator_dom")		sort_res.summoner_domain = 1;
							else if (token == L"creator_nam")		sort_res.summoner_name = 1;
							else if (token == L"threads")			sort_res.threads = 1;
							else if (token == L"work_mem")			sort_res.work_mem = 1;
							else if (token == L"direction_to_down")	sort_res.direction = 1;
							else if (token == L"direction_to_up")	sort_res.direction = 0;
							else if (token == L"-")					sort_res.nam = 1;
						} while (has_next && i < filter_size);

					}
				}
			}
			return { out_res, sort_res };
		}

		struct console_pos {
			size_t pid_len = 12;
			size_t nam_len = 6;
			size_t unam_len = 14;
			size_t udom_len = 16;
			size_t ses_len = 9;
			size_t thread_count = 9;
			size_t flags = 6;
			size_t work_mem = 15;
			size_t peak_work = 20;
			size_t page_faults = 0;
			size_t page_file_use = 0;
			size_t peak_page_file_use = 0;
			size_t qota_non_paged_pool_file_use = 0;
			size_t qota_paged_pool_file_use = 0;
			size_t qota_non_peak_paged_pool_file_use = 0;
			size_t qota_peak_paged_pool_file_use = 0;
		};
		std::vector<info_data> all;
#define Unique_color_s(str) (std::wstring(out_flag.use_color ? unique_rgb_color((str)) : L"") + str + reset_color())
#define Unique_color(str) (std::wstring(out_flag.use_color ? unique_rgb_color((str)) : L"") + std::to_wstring(str) + reset_color())
#define auto_fix_pos (pos_offset ? ([&pos_offset](size_t rem){pos_offset=0;return rem;}(pos_offset)) : 0)

		void print(std::wstring str) {
			wprintf(str.c_str());
		}
		void print(int64_t integer) {
			std::wstring str = std::to_wstring(integer);
			wprintf(str.c_str());
		}
		void print(out_flags& out_flag, console_pos& pos, info_data& data,size_t pos_offset=0) {
			if (out_flag.pid)						  {print(data.procID);															print(set_pos_in_line(pos.pid_len + auto_fix_pos));}
			if (out_flag.nam)						  {print(Unique_color_s(data.procName));										print(set_pos_in_line(pos.nam_len + auto_fix_pos));}
			if (out_flag.summoner_name)				  {print(Unique_color_s(data.summoner_name)); 									print(set_pos_in_line(pos.unam_len + auto_fix_pos));}
			if (out_flag.summoner_domain)			  {print(Unique_color_s(data.summoner_domain));									print(set_pos_in_line(pos.udom_len + auto_fix_pos));}
			if (out_flag.session)					  {print(Unique_color(data.session));											print(set_pos_in_line(pos.ses_len + auto_fix_pos));}
			if (out_flag.threads)					  {print(Unique_color(data.threads_count));										print(set_pos_in_line(pos.thread_count + auto_fix_pos));}
			if (out_flag.flags)						  {print(Unique_color_s(data.is_critical)); print(Unique_color_s(data.is_above_WOW64_emulator)); print(Unique_color_s(data.is_under_the_grip_of_a_debugger)); print(Unique_color_s(data.is_evaluated)); print(set_pos_in_line(pos.flags + pos_offset));}
			if(out_flag.work_mem)					  {print(Unique_color(data.memDetals.WorkingSetSize));  						print(set_pos_in_line( pos.work_mem + auto_fix_pos));}
			if(out_flag.peak_work_mem)				  {print(Unique_color(data.memDetals.PeakWorkingSetSize));  					print(set_pos_in_line( pos.peak_work+ auto_fix_pos));}
			if(out_flag.page_faults)				  {print(Unique_color(data.memDetals.PageFaultCount));  						print(set_pos_in_line( pos.page_faults + auto_fix_pos));}
			if(out_flag.page_file_use)				  {print(Unique_color(data.memDetals.PagefileUsage));  							print(set_pos_in_line( pos.page_file_use + auto_fix_pos));}
			if(out_flag.peak_page_file_use)			  {print(Unique_color(data.memDetals.PeakPagefileUsage));  						print(set_pos_in_line( pos.peak_page_file_use + auto_fix_pos));}
			if(out_flag.quota_non_paged_pool_use)	  {print(Unique_color(data.memDetals.QuotaNonPagedPoolUsage));  				print(set_pos_in_line( pos.qota_non_paged_pool_file_use + auto_fix_pos));}
			if(out_flag.quota_paged_pool_use)		  {print(Unique_color(data.memDetals.QuotaPagedPoolUsage));  					print(set_pos_in_line( pos.qota_paged_pool_file_use + auto_fix_pos));}
			if(out_flag.quota_non_peak_paged_pool_use){print(Unique_color(data.memDetals.QuotaPeakNonPagedPoolUsage));  			print(set_pos_in_line( pos.qota_non_peak_paged_pool_file_use + auto_fix_pos));}
			if(out_flag.quota_peak_paged_pool_use)	  {print(Unique_color(data.memDetals.QuotaPeakPagedPoolUsage));  				print(set_pos_in_line( pos.qota_peak_paged_pool_file_use + auto_fix_pos));}
			wprintf(L"\n");
		}
#undef auto_fix_pos
#undef Unique_color
#undef Unique_color_s

	public:
		void push(process_info_no_child info) {
			all.push_back(info_data{ info });
		}
		void push(std::vector<process_info>& list,process_info info) {
			all.push_back(info_data{ list,info });
		}
		void push(std::vector<process_info>& list) {
			for(auto& interator : list)
				all.push_back(info_data{interator});
		}
		void push(std::vector<process_info_no_child>& list) {
			for (auto& interator : list)
				all.push_back(info_data{ interator });
		}
		void output(std::wstring config) {
			auto config_res = out_flag_parser(config);
			console_pos max_proc;
			out_flags out = config_res.first;
			sort_flags sort = config_res.second;

			{
				size_t tmp;
				for (auto& info : all) {
					tmp = std::to_string(info.threads_count).size();
					if (max_proc.thread_count < tmp) max_proc.thread_count = tmp + 2;

					tmp = std::to_string(info.session).size();
					if (max_proc.ses_len < tmp) max_proc.ses_len = tmp + 1;

					tmp = std::to_string(info.procID).size();
					if (max_proc.pid_len < tmp) max_proc.pid_len = tmp + 1;

					tmp = std::to_string(info.memDetals.PeakWorkingSetSize).size();
					if (max_proc.peak_work < tmp) max_proc.peak_work = tmp + 1;

					tmp = std::to_string(info.memDetals.WorkingSetSize).size();
					if (max_proc.work_mem < tmp) max_proc.work_mem = tmp + 1;


					tmp = info.summoner_name.size();
					if (max_proc.unam_len < tmp) max_proc.unam_len = tmp + 1;


					tmp = info.summoner_domain.size();
					if (max_proc.udom_len < tmp) max_proc.udom_len = tmp + 1;


					tmp = info.procName.size();
					if (max_proc.nam_len < tmp) max_proc.nam_len = tmp + 1;
					if (info.childs) {
						for (auto& info0 : *info.childs) {
							tmp = std::to_string(info0.threads_count).size();
							if (max_proc.thread_count < tmp) max_proc.thread_count = tmp + 2;

							tmp = std::to_string(info0.session).size();
							if (max_proc.ses_len < tmp) max_proc.ses_len = tmp + 1;

							tmp = std::to_string(info0.procID).size();
							if (max_proc.pid_len < tmp) max_proc.pid_len = tmp + 1;

							tmp = std::to_string(info0.memDetals.PeakWorkingSetSize).size();
							if (max_proc.peak_work < tmp) max_proc.peak_work = tmp + 1;

							tmp = std::to_string(info0.memDetals.WorkingSetSize).size();
							if (max_proc.work_mem < tmp) max_proc.work_mem = tmp + 1;


							tmp = info0.summoner_name.size();
							if (max_proc.unam_len < tmp) max_proc.unam_len = tmp + 1;


							tmp = info0.summoner_domain.size();
							if (max_proc.udom_len < tmp) max_proc.udom_len = tmp + 1;


							tmp = info0.procName.size();
							if (max_proc.nam_len < tmp) max_proc.nam_len = tmp + 1;
						}
					}
				}
			}

			{
//for childs
#define auto_fix_pos + (first ? 5 : 0)
				size_t tmp = 0; 
				bool first = 1;
				if (out.pid) {
					tmp += max_proc.pid_len auto_fix_pos;
				}
				if (out.nam) {
					tmp += max_proc.nam_len auto_fix_pos;
					max_proc.nam_len = tmp ;
				}
				if (out.summoner_name) {
					tmp += max_proc.unam_len auto_fix_pos;
					max_proc.unam_len = tmp;
				}
				if (out.summoner_domain) {
					tmp += max_proc.udom_len auto_fix_pos;
					max_proc.udom_len = tmp;
				}
				if (out.session) {
					tmp += max_proc.ses_len auto_fix_pos;
					max_proc.ses_len = tmp;
				}
				if (out.threads) {
					tmp += max_proc.thread_count auto_fix_pos;
					max_proc.thread_count = tmp;
				}
				if (out.flags) {
					tmp += max_proc.flags auto_fix_pos;
					max_proc.flags+= tmp;
				}
				if (out.work_mem) {
					tmp += max_proc.work_mem auto_fix_pos;
					max_proc.work_mem = tmp;
				}
				if (out.peak_work_mem) {
					tmp += max_proc.peak_work auto_fix_pos;
					max_proc.peak_work = tmp;
				}
				if (out.page_faults) {
					tmp += max_proc.page_faults auto_fix_pos;
					max_proc.page_faults = tmp;
				}
				if (out.page_file_use) {
					tmp += max_proc.page_file_use auto_fix_pos;
					max_proc.page_file_use = tmp;
				}
				if (out.peak_page_file_use) {
					tmp += max_proc.peak_page_file_use auto_fix_pos;
					max_proc.peak_page_file_use = tmp;
				}
				if (out.quota_non_paged_pool_use) {
					tmp += max_proc.qota_non_paged_pool_file_use auto_fix_pos;
					max_proc.qota_non_paged_pool_file_use = tmp;
				}
				if (out.quota_paged_pool_use) {
					tmp += max_proc.qota_paged_pool_file_use auto_fix_pos;
					max_proc.work_mem = tmp ;
				}
				if (out.quota_non_peak_paged_pool_use) {
					tmp += max_proc.qota_non_peak_paged_pool_file_use auto_fix_pos;
					max_proc.qota_non_peak_paged_pool_file_use = tmp;
				}
				if (out.quota_peak_paged_pool_use) {
					tmp += max_proc.qota_peak_paged_pool_file_use auto_fix_pos;
					max_proc.qota_peak_paged_pool_file_use = tmp;
				}
#undef auto_fix_pos
			}

			{
				if(out.pid) 						 std::wcout<<	"Process ID" << 						set_pos_in_line( max_proc.pid_len);
				if(out.nam)							 std::wcout<<	"Name" << 								set_pos_in_line( max_proc.nam_len);
				if(out.summoner_name) 				 std::wcout<<	"Creator name" << 						set_pos_in_line( max_proc.unam_len);
				if(out.summoner_domain)				 std::wcout<<	"Creator domain" << 					set_pos_in_line( max_proc.udom_len);
				if(out.session)						 std::wcout<<	"Session" << 							set_pos_in_line( max_proc.ses_len);
				if(out.threads)						 std::wcout<<	"Threads" << 							set_pos_in_line( max_proc.thread_count);
				if(out.flags)						 std::wcout<<	"Flags" <<								set_pos_in_line( max_proc.flags);
				if(out.work_mem)					 std::wcout<<	"Work set size"	<< 						set_pos_in_line( max_proc.work_mem );
				if(out.peak_work_mem)				 std::wcout<<	"Peak work set size"<< 					set_pos_in_line( max_proc.peak_work);
				if(out.page_faults)					 std::wcout<<	"Page_faults"<< 						set_pos_in_line( max_proc.page_faults);
				if(out.page_file_use)				 std::wcout<<	"Page file use"	<< 						set_pos_in_line( max_proc.page_file_use);
				if(out.peak_page_file_use)			 std::wcout<<	"Peak page file use"<< 					set_pos_in_line( max_proc.peak_page_file_use);
				if(out.quota_non_paged_pool_use)	 std::wcout<<	"Qota non paged pool file use"<< 		set_pos_in_line( max_proc.qota_non_paged_pool_file_use);
				if(out.quota_paged_pool_use)		 std::wcout<<	"Qota paged pool file use"<< 			set_pos_in_line( max_proc.qota_paged_pool_file_use);
				if(out.quota_non_peak_paged_pool_use)std::wcout<< 	"Qota non peak paged pool file use"<< 	set_pos_in_line( max_proc.qota_non_peak_paged_pool_file_use);
				if(out.quota_peak_paged_pool_use)	 std::wcout<<	"Qota peak paged pool file use"<< 		set_pos_in_line( max_proc.qota_peak_paged_pool_file_use);
				std::wcout << std::endl;
			}

			if (sort.direction)
				do_sort<true>(sort, all);
			else
				do_sort<false>(sort, all);

			for (auto& item : all) {
				print(out, max_proc, item);
				if (item.childs) 
					for(auto& item0 : *item.childs)
						print(out, max_proc, item0,5);
			}
		}
	};


}
