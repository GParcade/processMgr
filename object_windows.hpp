#pragma once
#include <stdexcept>
#include <Windows.h>
#include <Knownfolders.h>
#include <Shlobj.h>
#include <userenv.h>
class windows_exception : public std::exception {
public:
	windows_exception(std::string ex_discription) : error_code(GetLastError()), std::exception(ex_discription.c_str()){
		error_code;
	}
	windows_exception(DWORD set_err, std::string ex_discription) : error_code(set_err), std::exception(ex_discription.c_str()) {
		error_code;
	}
	const DWORD error_code;
};

class Process {
	HANDLE proc_handle;
public:
	enum {
		
		 wFLAG_TERMINATE                 = THREAD_TERMINATE,
		 wFLAG_SUSPEND_RESUME            = THREAD_SUSPEND_RESUME,
		 wFLAG_GET_CONTEXT               = THREAD_GET_CONTEXT,
		 wFLAG_SET_CONTEXT               = THREAD_SET_CONTEXT,
		 wFLAG_QUERY_INFORMATION         = THREAD_QUERY_INFORMATION,
		 wFLAG_SET_INFORMATION           = THREAD_SET_INFORMATION,
		 wFLAG_SET_THREAD_TOKEN          = THREAD_SET_THREAD_TOKEN,
		 wFLAG_IMPERSONATE               = THREAD_IMPERSONATE,
		 wFLAG_DIRECT_IMPERSONATION      = THREAD_DIRECT_IMPERSONATION,
		 wFLAG_SET_LIMITED_INFORMATION   = THREAD_SET_LIMITED_INFORMATION,
		 wFLAG_QUERY_LIMITED_INFORMATION = THREAD_QUERY_LIMITED_INFORMATION,
		 wFLAG_RESUME                    = THREAD_RESUME,
		 wFLAG_ALL_ACCESS                = THREAD_ALL_ACCESS
	};
	Process(DWORD open_flags,DWORD proc_id, BOOL bInheritHandle = NULL) {
		proc_handle = OpenProcess(open_flags, bInheritHandle, proc_id);
		if (proc_handle == nullptr)
			throw windows_exception("Fail open process");
	}
	Process(Process& move) {
		proc_handle = move.proc_handle;
		move.proc_handle = nullptr;
	}
	~Process() {
		if(proc_handle)CloseHandle(proc_handle);
	}
	operator HANDLE() {
		return proc_handle;
	}
};

class ProcessToken {
	HANDLE token=nullptr;
public:
	enum {
		wFLAG_ASSIGN_PRIMARY			 = TOKEN_ASSIGN_PRIMARY,
		wFLAG_DUPLICATE					 = TOKEN_DUPLICATE,
		wFLAG_IMPERSONATE				 = TOKEN_IMPERSONATE,
		wFLAG_QUERY						 = TOKEN_QUERY,
		wFLAG_QUERY_SOURCE				 = TOKEN_QUERY_SOURCE,
		wFLAG_ADJUST_PRIVILEGES			 = TOKEN_ADJUST_PRIVILEGES,
		wFLAG_ADJUST_GROUPS				 = TOKEN_ADJUST_GROUPS,
		wFLAG_ADJUST_DEFAULT			 = TOKEN_ADJUST_DEFAULT,
		wFLAG_ADJUST_SESSIONID			 = TOKEN_ADJUST_SESSIONID,
		wFLAG_ALL_ACCESS_P				 = TOKEN_ALL_ACCESS_P,
		wFLAG_ALL_ACCESS				 = TOKEN_ALL_ACCESS,
		wFLAG_READ						 = TOKEN_READ,
		wFLAG_WRITE						 = TOKEN_WRITE,
		wFLAG_EXECUTE					 = TOKEN_EXECUTE,
		wFLAG_TRUST_CONSTRAINT_MASK		 = TOKEN_TRUST_CONSTRAINT_MASK,
#ifdef TOKEN_ACCESS_PSEUDO_HANDLE
		wFlag_ACCESS_PSEUDO_HANDLE		 = TOKEN_ACCESS_PSEUDO_HANDLE
#else
		wFlag_ACCESS_PSEUDO_HANDLE = 0
#endif
	};
	ProcessToken(Process& proc,DWORD access_flag) : ProcessToken((HANDLE)proc, access_flag){}
	ProcessToken(HANDLE proc, DWORD access_flag) {
		if(proc == nullptr)
			throw windows_exception("Process not opened");
		if (!OpenProcessToken(proc, access_flag, &token))
			throw windows_exception("Fail get process token");
	}
	ProcessToken(ProcessToken& move) {
		token = move.token;
		move.token = nullptr;
	}
	~ProcessToken() {
		if (token)
			CloseHandle(token);
	}
	void GetInformation(TOKEN_INFORMATION_CLASS token_type,LPVOID token_info,DWORD token_len,PDWORD cbSize) {
		if (token == nullptr)
			throw windows_exception("Invalid token, maybe it moved or deleted");
		if(!GetTokenInformation(token, token_type, token_info, token_len, cbSize))
			throw windows_exception("Fail get token information");
	}
	operator HANDLE() {
		return token;
	}
};


template<class T>
class LocalMem {
	T* mem = nullptr;
public:
	enum {
		wFLM_FIXED				= LMEM_FIXED,
		wFLM_MOVEABLE			= LMEM_MOVEABLE,
		wFLM_NOCOMPACT			= LMEM_NOCOMPACT,
		wFLM_NODISCARD			= LMEM_NODISCARD,
		wFLM_ZEROINIT			= LMEM_ZEROINIT,
		wFLM_MODIFY				= LMEM_MODIFY,
		wFLM_DISCARDABLE		= LMEM_DISCARDABLE,
		wFLM_VALID_FLAGS		= LMEM_VALID_FLAGS,
		wFLM_INVALID_HANDLE		= LMEM_INVALID_HANDLE,
		wFLHND					= LHND,
		wFLPTR					= LPTR
	};
	LocalMem(DWORD aloc_flags,size_t aloc_bytes) {
		if (aloc_bytes == 0) {
			throw windows_exception("Invalid aloc bytes count");
		}
		if (!(mem = (LPBYTE)LocalAlloc(LPTR, aloc_bytes)))
			throw windows_exception("Fail alocate memory");
	}
	operator T* () {
		return mem;
	}
	~LocalMem() {
		if (mem) 
			LocalFree(mem);
	}
};
