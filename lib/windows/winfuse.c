/*
 * FUSE: Filesystem in Userspace
 * Copyright (C) 2019 Bill Zissimopoulos <billziss at navimatics.com>
 *
 * This program can be distributed under the terms of the GNU LGPLv2.
 * See the file COPYING.LIB
 */

#include "config.h"
#include "fuse_i.h"

/*
 * WinFuse
 */

/*
 * We duplicate some of the definitions from WinFsp and WinFuse
 * to avoid having either of them as a build dependency.
 */

#define FUSE_FSCTL_TRANSACT             \
    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 0xC00 + 'F', METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSP_FSCTL_DISK_DEVICE_NAME      "WinFsp.Disk"
#define FSP_FSCTL_NET_DEVICE_NAME       "WinFsp.Net"
#define FSP_FSCTL_VOLUME_NAME_SIZE      (64 * sizeof(WCHAR))
#define FSP_FSCTL_VOLUME_PREFIX_SIZE    (192 * sizeof(WCHAR))
#define FSP_FSCTL_VOLUME_FSNAME_SIZE    (16 * sizeof(WCHAR))
#define FSP_FSCTL_VOLUME_NAME_SIZEMAX   (FSP_FSCTL_VOLUME_NAME_SIZE + FSP_FSCTL_VOLUME_PREFIX_SIZE)
#define FSP_FSCTL_VOLUME_PARAMS_V0_FIELD_DEFN\
    UINT16 Version;                     /* set to 0 or sizeof(FSP_FSCTL_VOLUME_PARAMS) */\
    /* volume information */\
    UINT16 SectorSize;\
    UINT16 SectorsPerAllocationUnit;\
    UINT16 MaxComponentLength;          /* maximum file name component length (bytes) */\
    UINT64 VolumeCreationTime;\
    UINT32 VolumeSerialNumber;\
    /* I/O timeouts, capacity, etc. */\
    UINT32 TransactTimeout;             /* DEPRECATED: (millis; 1 sec - 10 sec) */\
    UINT32 IrpTimeout;                  /* pending IRP timeout (millis; 1 min - 10 min) */\
    UINT32 IrpCapacity;                 /* maximum number of pending IRP's (100 - 1000)*/\
    UINT32 FileInfoTimeout;             /* FileInfo/Security/VolumeInfo timeout (millis) */\
    /* FILE_FS_ATTRIBUTE_INFORMATION::FileSystemAttributes */\
    UINT32 CaseSensitiveSearch:1;       /* file system supports case-sensitive file names */\
    UINT32 CasePreservedNames:1;        /* file system preserves the case of file names */\
    UINT32 UnicodeOnDisk:1;             /* file system supports Unicode in file names */\
    UINT32 PersistentAcls:1;            /* file system preserves and enforces access control lists */\
    UINT32 ReparsePoints:1;             /* file system supports reparse points */\
    UINT32 ReparsePointsAccessCheck:1;  /* file system performs reparse point access checks */\
    UINT32 NamedStreams:1;              /* file system supports named streams */\
    UINT32 HardLinks:1;                 /* unimplemented; set to 0 */\
    UINT32 ExtendedAttributes:1;        /* file system supports extended attributes */\
    UINT32 ReadOnlyVolume:1;\
    /* kernel-mode flags */\
    UINT32 PostCleanupWhenModifiedOnly:1;   /* post Cleanup when a file was modified/deleted */\
    UINT32 PassQueryDirectoryPattern:1;     /* pass Pattern during QueryDirectory operations */\
    UINT32 AlwaysUseDoubleBuffering:1;\
    UINT32 PassQueryDirectoryFileName:1;    /* pass FileName during QueryDirectory (GetDirInfoByName) */\
    UINT32 FlushAndPurgeOnCleanup:1;        /* keeps file off "standby" list */\
    UINT32 DeviceControl:1;                 /* support user-mode ioctl handling */\
    /* user-mode flags */\
    UINT32 UmFileContextIsUserContext2:1;   /* user mode: FileContext parameter is UserContext2 */\
    UINT32 UmFileContextIsFullContext:1;    /* user mode: FileContext parameter is FullContext */\
    UINT32 UmReservedFlags:6;\
    /* additional kernel-mode flags */\
    UINT32 AllowOpenInKernelMode:1;         /* allow kernel mode to open files when possible */\
    UINT32 CasePreservedExtendedAttributes:1;   /* preserve case of EA (default is UPPERCASE) */\
    UINT32 WslFeatures:1;                   /* support features required for WSLinux */\
    UINT32 KmReservedFlags:5;\
    WCHAR Prefix[FSP_FSCTL_VOLUME_PREFIX_SIZE / sizeof(WCHAR)]; /* UNC prefix (\Server\Share) */\
    WCHAR FileSystemName[FSP_FSCTL_VOLUME_FSNAME_SIZE / sizeof(WCHAR)];
#define FSP_FSCTL_VOLUME_PARAMS_V1_FIELD_DEFN\
    /* additional fields; specify .Version == sizeof(FSP_FSCTL_VOLUME_PARAMS) */\
    UINT32 VolumeInfoTimeoutValid:1;    /* VolumeInfoTimeout field is valid */\
    UINT32 DirInfoTimeoutValid:1;       /* DirInfoTimeout field is valid */\
    UINT32 SecurityTimeoutValid:1;      /* SecurityTimeout field is valid*/\
    UINT32 StreamInfoTimeoutValid:1;    /* StreamInfoTimeout field is valid */\
    UINT32 EaTimeoutValid:1;            /* EaTimeout field is valid */\
    UINT32 KmAdditionalReservedFlags:27;\
    UINT32 VolumeInfoTimeout;           /* volume info timeout (millis); overrides FileInfoTimeout */\
    UINT32 DirInfoTimeout;              /* dir info timeout (millis); overrides FileInfoTimeout */\
    UINT32 SecurityTimeout;             /* security info timeout (millis); overrides FileInfoTimeout */\
    UINT32 StreamInfoTimeout;           /* stream info timeout (millis); overrides FileInfoTimeout */\
    UINT32 EaTimeout;                   /* EA timeout (millis); overrides FileInfoTimeout */\
    UINT32 FsextControlCode;\
    UINT32 Reserved32[1];\
    UINT64 Reserved64[2];
typedef struct
{
    FSP_FSCTL_VOLUME_PARAMS_V0_FIELD_DEFN
    FSP_FSCTL_VOLUME_PARAMS_V1_FIELD_DEFN
} FSP_FSCTL_VOLUME_PARAMS;
static_assert(504 == sizeof(FSP_FSCTL_VOLUME_PARAMS),
    "sizeof(FSP_FSCTL_VOLUME_PARAMS) is currently 504. Update this assertion check if it changes.");
typedef struct
{
    /* in */
    HANDLE VolumeHandle;                /* volume handle returned by FspFsctlCreateVolume */
    PWSTR VolumeName;                   /* volume name returned by FspFsctlCreateVolume */
    PSECURITY_DESCRIPTOR Security;      /* optional: security descriptor for directories */
    UINT64 Reserved;                    /* reserved for future use */
    /* in/out */
    PWSTR MountPoint;                   /* FspMountSet sets drive in buffer when passed "*:" */
    HANDLE MountHandle;                 /* FspMountSet sets, FspMountRemove uses */
} FSP_MOUNT_DESC;
static NTSTATUS (*FspFsctlCreateVolume)(PWSTR DevicePath,
    const FSP_FSCTL_VOLUME_PARAMS *VolumeParams,
    PWCHAR VolumeNameBuf, SIZE_T VolumeNameSize,
    PHANDLE PVolumeHandle);
static NTSTATUS (*FspFsctlMakeMountdev)(HANDLE VolumeHandle,
    BOOLEAN Persistent, GUID *UniqueId);
static NTSTATUS (*FspFsctlTransact)(HANDLE VolumeHandle,
    PVOID ResponseBuf, SIZE_T ResponseBufSize,
    PVOID RequestBuf, SIZE_T *PRequestBufSize,
    BOOLEAN Batch);
static NTSTATUS (*FspFsctlStop)(HANDLE VolumeHandle);
static NTSTATUS (*FspFsctlGetVolumeList)(PWSTR DevicePath,
    PWCHAR VolumeListBuf, PSIZE_T PVolumeListSize);
static NTSTATUS (*FspFsctlPreflight)(PWSTR DevicePath);
static NTSTATUS (*FspMountSet)(FSP_MOUNT_DESC *Desc);
static NTSTATUS (*FspMountRemove)(FSP_MOUNT_DESC *Desc);
static NTSTATUS (*FspVersion)(PUINT32 PVersion);

static inline
NTSTATUS FspLoad(PVOID *PModule)
{
#if defined(_WIN64)
#define FSP_DLLNAME                     "winfsp-x64.dll"
#else
#define FSP_DLLNAME                     "winfsp-x86.dll"
#endif
#define FSP_DLLPATH                     "bin\\" FSP_DLLNAME

    WINADVAPI
    LSTATUS
    APIENTRY
    RegGetValueW(
        HKEY hkey,
        LPCWSTR lpSubKey,
        LPCWSTR lpValue,
        DWORD dwFlags,
        LPDWORD pdwType,
        PVOID pvData,
        LPDWORD pcbData);

    WCHAR PathBuf[MAX_PATH];
    DWORD Size = 0;
    HKEY RegKey;
    LONG Result;
    HMODULE Module;

    if (0 != PModule)
        *PModule = 0;

    Module = LoadLibraryW(L"" FSP_DLLNAME);
    if (0 == Module)
    {
        Result = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\WinFsp",
            0, KEY_READ | KEY_WOW64_32KEY, &RegKey);
        if (ERROR_SUCCESS == Result)
        {
            Size = sizeof PathBuf - sizeof L"" FSP_DLLPATH + sizeof(WCHAR);
            Result = RegGetValueW(RegKey, 0, L"InstallDir",
                RRF_RT_REG_SZ, 0, PathBuf, &Size);
            RegCloseKey(RegKey);
        }
        if (ERROR_SUCCESS != Result)
            return 0xC0000034/*STATUS_OBJECT_NAME_NOT_FOUND*/;

        RtlCopyMemory(PathBuf + (Size / sizeof(WCHAR) - 1), L"" FSP_DLLPATH, sizeof L"" FSP_DLLPATH);
        Module = LoadLibraryW(PathBuf);
        if (0 == Module)
            return STATUS_DLL_NOT_FOUND;
    }

    if (0 != PModule)
        *PModule = Module;

    return 0/*STATUS_SUCCESS*/;

#undef FSP_DLLNAME
#undef FSP_DLLPATH
}

#define FUSE_SECTORSIZE_MIN             512
#define FUSE_SECTORSIZE_MAX             4096

typedef struct
{
    HANDLE VolumeHandle;
    WCHAR VolumeName[FSP_FSCTL_VOLUME_NAME_SIZEMAX / sizeof(WCHAR)];
    WCHAR MountPoint[MAX_PATH];
    HANDLE MountHandle;
} FuseMount;
static FuseMount *FuseMounts[64];

static INIT_ONCE FuseOnce;
static BOOL FuseInit(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *Context)
{
#define GET_API(n)                      \
    if (0 == (*(void **)&(n) = GetProcAddress(Module, #n)))\
        goto fail;

    PVOID Module = 0;
    UINT32 Version;
    NTSTATUS Result;

    Result = FspLoad(&Module);
    if (0 > Result)
        return FALSE;

    GET_API(FspFsctlCreateVolume);
    GET_API(FspFsctlMakeMountdev);
    GET_API(FspFsctlTransact);
    GET_API(FspFsctlStop);
    GET_API(FspFsctlGetVolumeList);
    GET_API(FspFsctlPreflight);
    GET_API(FspMountSet);
    GET_API(FspMountRemove);
    GET_API(FspVersion);

    Result = FspVersion(&Version);
    if (0 > Result || 1 != HIWORD(Version) || 5 > LOWORD(Version))
        goto fail;

    return TRUE;

fail:
    FreeLibrary(Module);
    return FALSE;

#undef GET_API
}

BOOL FuseReadFile(HANDLE Handle, void *Buffer, DWORD Length, PDWORD PBytesTransferred)
{
    FuseMount *Mount = FuseMounts[(intptr_t)FuseFdToHandle(Handle)];
    DWORD BytesTransferred;

    do
    {
        if (!DeviceIoControl(Mount->VolumeHandle, FUSE_FSCTL_TRANSACT,
            0, 0, Buffer, Length, &BytesTransferred, 0))
        {
            *PBytesTransferred = 0;
            return FALSE;
        }
    } while (0 == BytesTransferred);

    *PBytesTransferred = BytesTransferred;
    return TRUE;
}

BOOL FuseWriteFile(HANDLE Handle, const void *Buffer, DWORD Length, PDWORD PBytesTransferred)
{
    FuseMount *Mount = FuseMounts[(intptr_t)FuseFdToHandle(Handle)];

    return DeviceIoControl(Mount->VolumeHandle, FUSE_FSCTL_TRANSACT,
        (void *)Buffer, Length, 0, 0, PBytesTransferred, 0);
}

/*
 * mount
 */

struct mount_opts
{
    FSP_FSCTL_VOLUME_PARAMS VolumeParams;
    int set_attr_timeout, attr_timeout;
    int set_FileInfoTimeout,
        set_DirInfoTimeout,
        set_EaTimeout,
        set_VolumeInfoTimeout,
        set_KeepFileCache;
    unsigned max_read;
};

#define FUSE_MOUNT_OPT(n, f, v)         { n, offsetof(struct mount_opts, f), v }

static struct fuse_opt fuse_mount_opts[] =
{
    FUSE_MOUNT_OPT("attr_timeout=", set_attr_timeout, 1),
    FUSE_MOUNT_OPT("attr_timeout=%d", attr_timeout, 0),

    FUSE_MOUNT_OPT("SectorSize=%hu", VolumeParams.SectorSize, 4096),
    FUSE_MOUNT_OPT("SectorsPerAllocationUnit=%hu", VolumeParams.SectorsPerAllocationUnit, 1),
    FUSE_MOUNT_OPT("MaxComponentLength=%hu", VolumeParams.MaxComponentLength, 0),
    FUSE_MOUNT_OPT("VolumeCreationTime=%lli", VolumeParams.VolumeCreationTime, 0),
    FUSE_MOUNT_OPT("VolumeSerialNumber=%lx", VolumeParams.VolumeSerialNumber, 0),
    FUSE_MOUNT_OPT("FileInfoTimeout=", set_FileInfoTimeout, 1),
    FUSE_MOUNT_OPT("FileInfoTimeout=%d", VolumeParams.FileInfoTimeout, 0),
    FUSE_MOUNT_OPT("DirInfoTimeout=", set_DirInfoTimeout, 1),
    FUSE_MOUNT_OPT("DirInfoTimeout=%d", VolumeParams.DirInfoTimeout, 0),
    FUSE_MOUNT_OPT("EaTimeout=", set_EaTimeout, 1),
    FUSE_MOUNT_OPT("EaTimeout=%d", VolumeParams.EaTimeout, 0),
    FUSE_MOUNT_OPT("VolumeInfoTimeout=", set_VolumeInfoTimeout, 1),
    FUSE_MOUNT_OPT("VolumeInfoTimeout=%d", VolumeParams.VolumeInfoTimeout, 0),
    FUSE_MOUNT_OPT("KeepFileCache=", set_KeepFileCache, 1),
    FUSE_OPT_KEY("UNC=", 'U'),
    FUSE_OPT_KEY("--UNC=", 'U'),
    FUSE_OPT_KEY("VolumePrefix=", 'U'),
    FUSE_OPT_KEY("--VolumePrefix=", 'U'),
    FUSE_OPT_KEY("fstypename=", 'F'),
    FUSE_OPT_KEY("FileSystemName=", 'F'),
    FUSE_OPT_KEY("--FileSystemName=", 'F'),

    FUSE_OPT_END,
};

static int fuse_mount_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs)
{
    struct mount_opts *mo = data;

    switch (key)
    {
    default:
        return 1;
    case 'U':
        if ('U' == arg[0])
            arg += sizeof "UNC=" - 1;
        else if ('U' == arg[2])
            arg += sizeof "--UNC=" - 1;
        else if ('V' == arg[0])
            arg += sizeof "VolumePrefix=" - 1;
        else if ('V' == arg[2])
            arg += sizeof "--VolumePrefix=" - 1;
        if (0 == MultiByteToWideChar(CP_UTF8, 0, arg, -1,
            mo->VolumeParams.Prefix, sizeof mo->VolumeParams.Prefix / sizeof(WCHAR)))
            return -1;
        mo->VolumeParams.Prefix
            [sizeof mo->VolumeParams.Prefix / sizeof(WCHAR) - 1] = L'\0';
        for (PWSTR P = mo->VolumeParams.Prefix; *P; P++)
            if (L'/' == *P)
                *P = '\\';
        return 0;
    case 'F':
        if ('f' == arg[0])
            arg += sizeof "fstypename=" - 1;
        else if ('F' == arg[0])
            arg += sizeof "FileSystemName=" - 1;
        else if ('F' == arg[2])
            arg += sizeof "--FileSystemName=" - 1;
        if (0 == MultiByteToWideChar(CP_UTF8, 0, arg, -1,
            mo->VolumeParams.FileSystemName + 5,
            sizeof mo->VolumeParams.FileSystemName / sizeof(WCHAR) - 5))
            return -1;
        mo->VolumeParams.FileSystemName
            [sizeof mo->VolumeParams.FileSystemName / sizeof(WCHAR) - 1] = L'\0';
        memcpy(mo->VolumeParams.FileSystemName, L"FUSE-", 5 * sizeof(WCHAR));
        return 0;
    }
}

struct mount_opts *parse_mount_opts(struct fuse_args *args)
{
    struct mount_opts *mo;

    mo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *mo);
    if (0 == mo)
        return 0;

    mo->VolumeParams.Version = sizeof(FSP_FSCTL_VOLUME_PARAMS);
    mo->VolumeParams.FileInfoTimeout = 1000;
    mo->VolumeParams.FlushAndPurgeOnCleanup = TRUE;

    if (0 != args && -1 == fuse_opt_parse(args, mo, fuse_mount_opts, fuse_mount_opt_proc))
        goto fail;

    if (!mo->set_FileInfoTimeout && mo->set_attr_timeout)
        mo->VolumeParams.FileInfoTimeout = mo->attr_timeout * 1000;
    if (mo->set_DirInfoTimeout)
        mo->VolumeParams.DirInfoTimeoutValid = 1;
    if (mo->set_EaTimeout)
        mo->VolumeParams.EaTimeoutValid = 1;
    if (mo->set_VolumeInfoTimeout)
        mo->VolumeParams.VolumeInfoTimeoutValid = 1;
    if (mo->set_KeepFileCache)
        mo->VolumeParams.FlushAndPurgeOnCleanup = FALSE;
    mo->VolumeParams.CaseSensitiveSearch = TRUE;
    mo->VolumeParams.CasePreservedNames = TRUE;
    mo->VolumeParams.PersistentAcls = TRUE;
    mo->VolumeParams.ReparsePoints = TRUE;
    mo->VolumeParams.ReparsePointsAccessCheck = FALSE;
    mo->VolumeParams.NamedStreams = FALSE;
    mo->VolumeParams.ReadOnlyVolume = FALSE;
    mo->VolumeParams.PostCleanupWhenModifiedOnly = TRUE;
    mo->VolumeParams.PassQueryDirectoryFileName = TRUE;
    mo->VolumeParams.DeviceControl = TRUE;
    if (L'\0' == mo->VolumeParams.FileSystemName[0])
        memcpy(mo->VolumeParams.FileSystemName, L"FUSE", 5 * sizeof(WCHAR));

    if (mo->VolumeParams.SectorSize < FUSE_SECTORSIZE_MIN ||
        mo->VolumeParams.SectorSize > FUSE_SECTORSIZE_MAX)
        mo->VolumeParams.SectorSize = FUSE_SECTORSIZE_MAX;
    if (mo->VolumeParams.SectorsPerAllocationUnit == 0)
        mo->VolumeParams.SectorsPerAllocationUnit = 1;
    if (0 == mo->VolumeParams.MaxComponentLength || mo->VolumeParams.MaxComponentLength > 255)
        mo->VolumeParams.MaxComponentLength = 255;
    if (0 == mo->VolumeParams.VolumeCreationTime)
    {
        FILETIME FileTime;
        GetSystemTimeAsFileTime(&FileTime);
        mo->VolumeParams.VolumeCreationTime = *(PUINT64)&FileTime;
    }
    if (0 == mo->VolumeParams.VolumeSerialNumber)
        mo->VolumeParams.VolumeSerialNumber =
            ((PLARGE_INTEGER)&mo->VolumeParams.VolumeCreationTime)->HighPart ^
            ((PLARGE_INTEGER)&mo->VolumeParams.VolumeCreationTime)->LowPart;

    mo->VolumeParams.FsextControlCode = FUSE_FSCTL_TRANSACT;

    return mo;

fail:
    destroy_mount_opts(mo);
    return 0;
}

void destroy_mount_opts(struct mount_opts *mo)
{
    HeapFree(GetProcessHeap(), 0, mo);
}

void fuse_mount_version(void)
{
}

unsigned get_max_read(struct mount_opts *mo)
{
    return mo->max_read;
}

int fuse_kern_mount(const char *mountpoint, struct mount_opts *mo)
{
    size_t MountIndex = sizeof FuseMounts / sizeof FuseMounts[0];
    FuseMount *Mount = 0;
    FSP_MOUNT_DESC Desc;
    NTSTATUS Result;

    if (!InitOnceExecuteOnce(&FuseOnce, FuseInit, 0, 0))
    {
        errno = ENOEXEC;
        goto fail;
    }

    Mount = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof *Mount);
    if (0 == Mount)
    {
        errno = ENOMEM;
        goto fail;
    }
    Mount->VolumeHandle = INVALID_HANDLE_VALUE;

    for (MountIndex = 0; sizeof FuseMounts / sizeof FuseMounts[0] > MountIndex; MountIndex++)
        if (0 == InterlockedCompareExchangePointer(&FuseMounts[MountIndex], Mount, 0))
            break;
    if (sizeof FuseMounts / sizeof FuseMounts[0] == MountIndex)
    {
        errno = ENOMEM;
        goto fail;
    }
    
    if (0 == MultiByteToWideChar(CP_UTF8, 0, mountpoint, -1,
        Mount->MountPoint, sizeof Mount->MountPoint / sizeof Mount->MountPoint[0]))
    {
        errno = EILSEQ;
        goto fail;
    }

    Result = FspFsctlCreateVolume(
        mo->VolumeParams.Prefix[0] ?
            L"" FSP_FSCTL_NET_DEVICE_NAME : L"" FSP_FSCTL_DISK_DEVICE_NAME,
        &mo->VolumeParams,
        Mount->VolumeName, sizeof Mount->VolumeName,
        &Mount->VolumeHandle);
    if (0 > Result)
    {
        errno = ENODEV;
        goto fail;
    }

    memset(&Desc, 0, sizeof Desc);
    Desc.VolumeHandle = Mount->VolumeHandle;
    Desc.VolumeName = Mount->VolumeName;
    Desc.MountPoint = Mount->MountPoint;
    Result = FspMountSet(&Desc);
    if (0 > Result)
    {
        errno = EINVAL;
        goto fail;
    }
    Mount->MountHandle = Desc.MountHandle;

    FuseMounts[MountIndex] = Mount;

    return FuseHandleToFd(MountIndex);

fail:
    if (sizeof FuseMounts / sizeof FuseMounts[0] != MountIndex)
        InterlockedExchangePointer(&FuseMounts[MountIndex], 0);

    if (0 != Mount)
    {
        if (INVALID_HANDLE_VALUE != Mount->VolumeHandle)
            CloseHandle(Mount->VolumeHandle);

        HeapFree(GetProcessHeap(), 0, Mount);
    }

    return -1;
}

void fuse_kern_unmount(const char *mountpoint, int fd)
{
    size_t MountIndex = (intptr_t)FuseFdToHandle(fd);
    FuseMount *Mount = FuseMounts[MountIndex];
    FSP_MOUNT_DESC Desc;

    InterlockedExchangePointer(&FuseMounts[MountIndex], 0);

    memset(&Desc, 0, sizeof Desc);
    Desc.VolumeHandle = Mount->VolumeHandle;
    Desc.VolumeName = Mount->VolumeName;
    Desc.MountPoint = Mount->MountPoint;
    Desc.MountHandle = Mount->MountHandle;
    FspMountRemove(&Desc);

    CloseHandle(Mount->VolumeHandle);
    HeapFree(GetProcessHeap(), 0, Mount);
}
