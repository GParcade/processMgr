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
		LookupAccountSidW(NULL, user_sid, user, &cbUser, domain, &cbDomain, &nu);

		return std::wstring(user);
	}
	std::wstring get_domain(PSID user_sid) {
		wchar_t user[80], domain[80];
		DWORD cbUser = 80, cbDomain = 80;
		SID_NAME_USE nu;
		if (!LookupAccountSidW(NULL, user_sid, user, &cbUser, domain, &cbDomain, &nu)) {
			if (!user_sid)return L"NoDomain";
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

	bool is_admin(DWORD pid) {
		HANDLE access_token;
		DWORD buffer_size = 0;
		PSID admin_SID;
		TOKEN_GROUPS* group_token = nullptr;
		SID_IDENTIFIER_AUTHORITY NT_authority = SECURITY_NT_AUTHORITY;
		HANDLE processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		if (processHandle == INVALID_HANDLE_VALUE)
			return 0;



		if (!OpenProcessToken(processHandle, TOKEN_READ, &access_token))
			return 0;

		GetTokenInformation(
			access_token,
			TokenGroups,
			group_token,
			0,
			&buffer_size
		);

		std::vector<char> buffer(buffer_size);

		group_token =
			reinterpret_cast<TOKEN_GROUPS*>(&buffer[0]);

		bool succeeded = GetTokenInformation(
			access_token,
			TokenGroups,
			group_token,
			buffer_size,
			&buffer_size
		);

		CloseHandle(access_token);
		if (!succeeded)
			return false;

		if (!AllocateAndInitializeSid(
			&NT_authority,
			2,
			SECURITY_BUILTIN_DOMAIN_RID,
			DOMAIN_ALIAS_RID_ADMINS,
			0, 0, 0, 0, 0, 0,
			&admin_SID
		))
		{
			return false;
		}

		bool found = false;
		for (int i = 0; !found && i < group_token->GroupCount; i++)
			found = EqualSid(admin_SID, group_token->Groups[i].Sid);
		FreeSid(admin_SID);
		CloseHandle(processHandle);
		return found;
	}
}