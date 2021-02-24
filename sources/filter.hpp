#pragma once
#include <iostream>
#include <string>
#include <vector>
#include "multi_thread_for.hpp"
#include "windows_enum.hpp"

#define switch_val(value) (value = !value)
namespace filter {
	class Invalid_filter : public std::exception {
		const std::wstring _reason;
	public:
		Invalid_filter(std::wstring token) :_reason(token) {}
		virtual const char* what() const override {
			return "Invalid token";
		}
		virtual const std::wstring reason() const {
			return _reason;
		}

	};

#define not_has_block_chars(X)((X) != L',' || (X) != L'}' || (X) != L'{')

	static std::wstring get_token(std::wstring& str, size_t size, size_t& index, bool& has_next) {
		std::wstring res_str;
		bool in_string = 0;
		bool string_type = 0;
		has_next = 0;
		if (str[index] == L',' || str[index] == L'{ ')
			index++;
		if ((str[index]) == L'\"') {
			in_string = 1;
			string_type = 0;
			index++;
		}
		if ((str[index]) == L'\'') {
			in_string = 1;
			string_type = 1;
			index++;
		}

		for (; index < size; index++) {
			if (in_string) {
				if ((str[index]) == L'\"' && string_type == 0) {
					index++;
					if (index < size) {
						if (str[index] == L',') has_next = 1;
					}
					break;
				}
				else if ((str[index]) == L'\'' && string_type == 1) {
					index++;
					if (index < size) {
						if (str[index] == L',') has_next = 1;
					}
					break;
				}
				else res_str += str[index];
			}
			else {
				switch (str[index]) {
				case L',':
				case L'}':
				case L'{':
					if (str[index] == L',') has_next = 1;
					goto out;
				case L' ':
				case L'\t':
				case L'\n':
					break;
				case L'\"':
				case L'\'':
					continue;
				default:
					res_str += str[index];
				}
			}
		}
	out:
		return res_str;
	}
#undef not_has_block_chars


	struct processed_filter {
		struct {
			bool current_user = 0;
			std::vector<std::wstring> groups;
			std::vector<std::wstring> domains;
			std::vector<std::wstring> names;
		} started_by;

		struct {
			std::vector<std::wstring> names;
			std::vector<std::wstring> wnames;
			std::vector<uint64_t> ids;
		} name_index;

		struct {
			uint64_t threshold = 0;
			bool
				match_windows = 0,
				match_process = 0,
				similar = 1;
		} config;

		struct miscellaneous {
			double cpu_from = 0;
			double cpu_to = 100;
			enum class priority_t {
				undef,
				realtime,
				height,
				heigh_that_avg,
				default_avg,
				low_that_avg,
				low
			} priority = priority_t::undef;
		} misc;

		bool no_filters = 0;

		processed_filter(std::wstring& filter) {
			bool has_next;
			size_t filter_size = filter.size();
			if (filter.empty() || filter == L"-")
				started_by.current_user = 1;

			else if (filter == L"*")
				no_filters = 1;

			else {
				std::wstring token;
				for (size_t i = 0; i < filter_size; i++) {
					token = to_low(get_token(filter, filter_size, i, has_next));
					if (token.empty())
						continue;
					if (token == L"users" || token == L"user" || token == L"u") {
						do {
							token = get_token(filter, filter_size, i, has_next);
							if (!token.empty())
								started_by.names.push_back(token);
						} while (has_next && i < filter_size);
					}
					else if (token == L"groups" || token == L"group" || token == L"g") {
						do {
							token = get_token(filter, filter_size, i, has_next);
							if (!token.empty())
								started_by.groups.push_back(token);
						} while (has_next && i < filter_size);
					}
					else if (token == L"domains" || token == L"domain" || token == L"d") {
						do {
							token = get_token(filter, filter_size, i, has_next);
							if (!token.empty())
								started_by.domains.push_back(token);
						} while (has_next && i < filter_size);
					}
					else if (token == L"names" || token == L"name" || token == L"n") {
						do {
							token = get_token(filter, filter_size, i, has_next);
							if (!token.empty())
								name_index.names.push_back(token);
						} while (has_next && i < filter_size);
					}
					else if (token == L"windows" || token == L"wind" || token == L"w") {
						do {
							token = get_token(filter, filter_size, i, has_next);
							if (!token.empty())
								name_index.wnames.push_back(token);
						} while (has_next && i < filter_size);
					}
					else if (token == L"indefiners" || token == L"indefiner" || token == L"id" || token == L"i") {
						do {
							token = get_token(filter, filter_size, i, has_next);
							try {
								name_index.ids.push_back(stoull(token));
							}
							catch (...) {}
						} while (has_next && i < filter_size);
					}
					else if (token == L"configs" || token == L"config" || token == L"c") {
						do {
							token = get_token(filter, filter_size, i, has_next);
							if (token == L"mname_window")
								switch_val(config.match_windows);
							else if (token == L"mname_process")
								switch_val(config.match_process);
							else if (token == L"use_similar")
								switch_val(config.similar);
							else if (token == L"similar_threshold") {
								try {
									config.threshold = stoull(token);
								}
								catch (...) {}
							}
							else if (token.empty()) {}
							else throw Invalid_filter(token);
						} while (has_next && i < filter_size);
					}
					else if (token == L"miscs" || token == L"misc" || token == L"m") {
						do {
							token = get_token(filter, filter_size, i, has_next);
							if (token == L"priority") {
								token = get_token(filter, filter_size, i, has_next);
								if (token == L"realtime")
									misc.priority = miscellaneous::priority_t::realtime;
								else if (token == L"height")
									misc.priority = miscellaneous::priority_t::height;
								else if (token == L"heigh_avg")
									misc.priority = miscellaneous::priority_t::heigh_that_avg;
								else if (token == L"default")
									misc.priority = miscellaneous::priority_t::default_avg;
								else if (token == L"low_avg")
									misc.priority = miscellaneous::priority_t::low_that_avg;
								else if (token == L"low")
									misc.priority = miscellaneous::priority_t::low;
								else
									misc.priority = miscellaneous::priority_t::undef;
							}
							else if (token == L"cpu_min") {
								try {
									misc.cpu_from = stod(token);
								}
								catch (...) {}
							}
							else if (token == L"cpu_max") {
								try {
									misc.cpu_to = stod(token);
								}
								catch (...) {}
							}
							else if (token.empty()) {}
							else throw Invalid_filter(token);
						} while (has_next && i < filter_size);
					}
					else throw Invalid_filter(token);
				}
			}

		}
	};

	class Filter {
		std::vector<bool> mask;
	public:
		Filter(size_t mask_size, bool default_bit = true) {
			mask.resize(mask_size, default_bit);
		}
		Filter(const Filter& copy) {
			mask = copy.mask;
		}
		void apply(const Filter& another_filter) {
			if (size() <= another_filter.size())
				for_thread(mask, [&](bool, size_t i) {
				if (mask[i])
					mask[i] = another_filter.mask[i] & mask[i];
					});
			else
				for_thread(another_filter.mask, [&](bool, size_t i) {
				if (mask[i])
					mask[i] = another_filter.mask[i] & mask[i];
					});
		}
		auto operator[](size_t pos) {
			return mask[pos];
		}
		template<typename VectorT>
		void apply_to(VectorT& modifable_filter, bool remove_if = false) {
			if (modifable_filter.size() != mask.size())return;
			for (int64_t i = mask.size() - 1; i >= 0; i--) {
				if (mask[i] == remove_if)
					modifable_filter.erase(modifable_filter.begin() + i);
			}
		}
		size_t size() const {
			return mask.size();
		}
	};
}