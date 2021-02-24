#pragma once
#include <Windows.h>
#include <aclapi.h>
#include <string>
#include <mutex>
#include "object_windows.hpp"
namespace user {

	PSID current_psid() {
		try {
			ProcessToken hTok;
			DWORD  dwSize = 0;
			try {
				hTok.GetInformation(TokenUser, nullptr, 0, &dwSize);
			}catch(...){}
			LocalMem<BYTE> buf(LPTR, dwSize);

			hTok.GetInformation(TokenUser, buf, dwSize, &dwSize);

			return ((PTOKEN_USER)(BYTE*)buf)->User.Sid;
		}
		catch (...) {}
		return nullptr;
	}


	std::wstring get_name(PSID user_sid) {
		wchar_t user[80], domain[80];
		DWORD cbUser = 80, cbDomain = 80;
		SID_NAME_USE nu;
		if (!LookupAccountSidW(nullptr, user_sid, user, &cbUser, domain, &cbDomain, &nu)) {
			if (!user_sid) return L"System Core";
				return L"Invalid SID";
		}
		return std::wstring(user);
	}
	std::wstring get_domain(PSID user_sid) {
		wchar_t user[80], domain[80];
		DWORD cbUser = 80, cbDomain = 80;
		SID_NAME_USE nu;
		if (!LookupAccountSidW(nullptr, user_sid, user, &cbUser, domain, &cbDomain, &nu)) {
			if (!user_sid) return L"NoDomain";
			return L"Invalid SID";
		}

		return std::wstring(domain);
	}

	struct {
		std::mutex mut;
		std::wstring domain = get_domain(current_psid());
		std::wstring operator()() {
			if (domain == L"Invalid SID") {
				mut.lock();
				Sleep(10);
				if (domain == L"Invalid SID")
					domain = get_domain(current_psid());
				mut.unlock();
			}
			return domain;
		}
	}current_domain;

	struct {
		std::mutex mut;
		std::wstring name = get_name(current_psid());
		std::wstring operator()() {
			if (name == L"Invalid SID") {
				mut.lock();
				Sleep(10);
				if (name == L"Invalid SID")
					name = get_name(current_psid());
				mut.unlock();
			}
			return name;
		}
	}current_name;


	signed char IsElevated(DWORD pid) {
		try {
			Process hProcess(Process::wFLAG_ALL_ACCESS, pid);
			ProcessToken token(hProcess, ProcessToken::wFLAG_QUERY);
			TOKEN_ELEVATION Elevation;
			DWORD cbSize = sizeof(TOKEN_ELEVATION);
			token.GetInformation(TokenElevation, &Elevation, sizeof(Elevation), &cbSize);
			return (bool)Elevation.TokenIsElevated;
		}
		catch (...){}
		return -1;
	}
}