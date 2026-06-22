#include <sindri/internal/nt_defs.h>
#include <sindri/loaders/knowndlls/knowndlls.h>
#include <windows.h>

typedef NTSTATUS(NTAPI *NtOpenSection_t)(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess,
                                         PSND_OBJECT_ATTRIBUTES ObjectAttributes);

typedef NTSTATUS(NTAPI *NtMapViewOfSection_t)(HANDLE SectionHandle, HANDLE ProcessHandle, PVOID *BaseAddress,
                                              ULONG_PTR ZeroBits, SIZE_T CommitSize, PLARGE_INTEGER SectionOffset,
                                              PSIZE_T ViewSize, DWORD InheritDisposition, ULONG AllocationType,
                                              ULONG Win32Protect);

typedef NTSTATUS(NTAPI *NtClose_t)(HANDLE Handle);

static snd_status_t WINAPI win_NtOpenSection(PHANDLE SectionHandle, ACCESS_MASK DesiredAccess,
                                             PSND_OBJECT_ATTRIBUTES ObjectAttributes) {
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll)
        return SND_ERR_W32(SND_ERROR_GENERIC);

    NtOpenSection_t pNtOpenSection = (NtOpenSection_t)GetProcAddress(ntdll, "NtOpenSection");
    if (!pNtOpenSection)
        return SND_ERR_W32(SND_ERROR_GENERIC);

    NTSTATUS status = pNtOpenSection(SectionHandle, DesiredAccess, ObjectAttributes);
    return NT_SUCCESS(status) ? SND_OK : SND_ERR_CTX(SND_ERROR_GENERIC, "NtOpenSection failed via GetProcAddress");
}

static snd_status_t WINAPI win_NtMapViewOfSection(HANDLE SectionHandle, HANDLE ProcessHandle, PVOID *BaseAddress,
                                                  ULONG_PTR ZeroBits, SIZE_T CommitSize, PLARGE_INTEGER SectionOffset,
                                                  PSIZE_T ViewSize, DWORD InheritDisposition, ULONG AllocationType,
                                                  ULONG Win32Protect) {
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll)
        return SND_ERR_W32(SND_ERROR_GENERIC);

    NtMapViewOfSection_t pNtMapViewOfSection = (NtMapViewOfSection_t)GetProcAddress(ntdll, "NtMapViewOfSection");
    if (!pNtMapViewOfSection)
        return SND_ERR_W32(SND_ERROR_GENERIC);

    NTSTATUS status = pNtMapViewOfSection(SectionHandle, ProcessHandle, BaseAddress, ZeroBits, CommitSize,
                                          SectionOffset, ViewSize, InheritDisposition, AllocationType, Win32Protect);
    return NT_SUCCESS(status) ? SND_OK : SND_ERR_CTX(SND_ERROR_GENERIC, "NtMapViewOfSection failed via GetProcAddress");
}

static snd_status_t WINAPI win_NtClose(HANDLE Handle) {
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll)
        return SND_ERR_W32(SND_ERROR_GENERIC);

    NtClose_t pNtClose = (NtClose_t)GetProcAddress(ntdll, "NtClose");
    if (!pNtClose)
        return SND_ERR_W32(SND_ERROR_GENERIC);

    NTSTATUS status = pNtClose(Handle);
    return NT_SUCCESS(status) ? SND_OK : SND_ERR_CTX(SND_ERROR_GENERIC, "NtClose failed via GetProcAddress");
}

const snd_knowndlls_config_t snd_knowndlls_win = {
    .open_section = win_NtOpenSection, .map_view_of_section = win_NtMapViewOfSection, .close_handle = win_NtClose};
