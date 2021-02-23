#pragma once
#include <vector>
#include <mutex>
#include <list>
#include "console.hpp"
template<typename T>
bool similar(T& strcmp0, T& strcmp1, size_t threshold = 0) {
	size_t total_sybols0 = strcmp0.size();
	size_t total_sybols1 = strcmp1.size();
	int64_t total_same = 0;
	int64_t total_another = 0;
	for (size_t i = 0;

		i < total_sybols0
		&&
		i < total_sybols1;
		i++
		) {
		if (strcmp0[i] == strcmp1[i])
			total_same++;
		else total_another++;
	}
	if (total_same - total_another > threshold)
		return 1;
	else return 0;
}

template<typename T>
bool contain(T& strcmp0, T& strcmp1) {
	return strcmp0.find(strcmp1, 0) != T::npos;
}
template<typename T>
bool contain(T strcmp0, T strcmp1) {
	return strcmp0.find(strcmp1, 0) != T::npos;
}
template<typename T>
bool containA(T& strs0, T& strs1) {
	for (auto& str0 : strs0)
		for (auto& str1 : strs0)
			if (str0.find(str1, 0) != T::npos)return 1;
	return 0;
}

template<typename T>
bool contain_value(std::vector<T> vector, T value) {
	for (auto& tmp : vector) {
		if (tmp == value)
			return 1;
	}
	return 0;
};

template<typename T>
bool contain(std::vector<T> vector, T strcmp1) {
	for (auto& value : vector)
		if (value.find(strcmp1, 0) != T::npos)return 1;
	return 0;
}

template<typename T>
bool contain_similar(std::vector<T> vector, T strcmp1, size_t threshold = 0) {
	for (auto& value : vector)
		if (similar(value, strcmp1, threshold))return 1;
	return 0;
}

template<typename T>
bool exist(std::vector<T> vector, T scan_value) {
	for (auto& value : vector)
		if (value == scan_value)return 1;
	return 0;
}

template<typename T>
bool exist(std::vector<T> vector, std::vector <T> scan_value) {
	for (auto& value : vector)
		for (auto& scan : scan_value)
			if (value == scan)return 1;
	return 0;
}

std::wstring to_low(std::wstring str) {
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	return str;
}



struct colored_wchar {
	std::wstring chas;
	uint8_t Red		= 255;
	uint8_t Green	= 255;
	uint8_t Blue	= 255;
	colored_wchar(){}
	colored_wchar(const wchar_t scha) { chas = scha; }
	colored_wchar(const std::wstring str) { unique_rgb_color_ints(str, Red, Green, Blue); }
	colored_wchar(const std::wstring scha,const std::wstring str){ chas = scha; unique_rgb_color_ints(str, Red, Green, Blue); }
	colored_wchar(const std::wstring scha, uint8_t r,uint8_t g,uint8_t b) {
		chas = scha;
		Red = r;
		Green = g;
		Blue = b;
	}
	colored_wchar(const colored_wchar& scha) {
		chas = scha.chas;
		Red = scha.Red;
		Green = scha.Green;
		Blue = scha.Blue;
	}
	operator std::wstring()const {
		return rgb_color_text(Red, Green, Blue) + chas + reset_color();
	}
};
std::wostream& operator<<(std::wostream& stream, const colored_wchar& cha) {
	return stream << rgb_color_text(cha.Red, cha.Green, cha.Blue) << cha.chas.c_str() << reset_color;
}
typedef std::vector<colored_wchar> colored_str;

std::wostream& operator<<(std::wostream& stream, const colored_str& chars) {
	for (auto& i : chars) {
		stream << i;
	}
	return stream;
}

template<typename T>
class shared_str {
	static std::list<T> shared;
	static std::mutex mutex;
	T* tmp=nullptr;
	void put(const T& cmp) {
		std::lock_guard<std::mutex> lock(mutex);
		for (T& find : shared)
			if (find == cmp) {
				tmp = &find;
				return;
			}
		shared.push_back(cmp);
		tmp = &shared.back();
		unique = 1;
	}
	bool unique = 0;
public:
	shared_str(){
		T tmp = T();
		put(tmp);
	}
	shared_str(const T& cmp) {
		put(cmp);
	}
	shared_str(const shared_str& copy) {
		*this = copy;
	}
	shared_str& operator=(const shared_str& copy) {
		tmp = copy.tmp;
		return *this;
	}

	shared_str& operator=(const T& copy) {
		tmp = shared_str<T>(copy).tmp;
		return *this;
	}
	const T& operator*() {
		return *tmp;
	}

	const T* operator->() {
		return tmp;
	}
	bool is_unique() {
		return unique;
	}
};

template<typename T>
std::list<T> shared_str<T>::shared= std::list<T>();
template<typename T>
std::mutex shared_str<T>::mutex;