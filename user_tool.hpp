#pragma once
#include <Windows.h>
#include <aclapi.h>
#include <string>
namespace user {

	PSID current_psid() {
		HANDLE hTok = nullptr;
		PSID res = nullptr;
		if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hTok)) {
			LPBYTE buf = nullptr;
			DWORD  dwSize = 0;
			GetTokenInformation(hTok, TokenUser, nullptr, 0, &dwSize);
			if (dwSize)
				buf = (LPBYTE)LocalAlloc(LPTR, dwSize);

			if (GetTokenInformation(hTok, TokenUser, buf, dwSize, &dwSize))
				res = ((PTOKEN_USER)buf)->User.Sid;
			LocalFree(buf);
			CloseHandle(hTok);
		}
		return res;
	}


	std::wstring get_name(PSID user_sid) {
		wchar_t user[80], domain[80];
		DWORD cbUser = 80, cbDomain = 80;
		SID_NAME_USE nu;
		LookupAccountSidW(nullptr, user_sid, user, &cbUser, domain, &cbDomain, &nu);

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
	std::wstring current_domain() {
		return get_domain(current_psid());
	}
	std::wstring current_name() {
		return get_name(current_psid());
	}

	signed char IsElevated(DWORD pid) {
		signed char res = -1;
		HANDLE hToken = nullptr;
		if (HANDLE hproc = OpenProcess(THREAD_ALL_ACCESS, FALSE, pid)) {
			if (OpenProcessToken(hproc, TOKEN_QUERY, &hToken)) {
				TOKEN_ELEVATION Elevation;
				DWORD cbSize = sizeof(TOKEN_ELEVATION);
				if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize))
					res = (bool)Elevation.TokenIsElevated;
			}
			if (hToken) {
				CloseHandle(hToken);
			}
			CloseHandle(hproc);
		}
		return res;
	}
}