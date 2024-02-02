#pragma once
//=============================================================================
// CKernelIf_FileInformation.h
//
//
// Structure and Enumerations to the Windows Native API functions to work with
// Files and File Systems.
// 
//
// Because most callers only need a small subset of this API, each info class
// member is ifdef'd.  The code referencing this .h must previously define the
// required element types and define the KDEF_FILEIF_XXX.
//
// The only interface implemented at this time is for OpenFile, used so that
//	we can access a psuedo-file such as "\\Device\\PhysicalMemory"
//
// NOTES:This information is culled from the Windows DDK, the unoficial Windows
//		 Native API, and experience.
//
//		 These interfaces are known to work on Windows 2000, XP, and 2003.
//
//		 Microsoft's next OS (codenamed Longhorn) may or may not be compatible
//		 with this interface.
//
// Copyright:
//		Tim Mangan/TMurgent Technologies 2006
//=============================================================================
#pragma once

//#include <Windows.h>
#include <Winternl.h>
/////#include <ntifs.h>
typedef unsigned long ULONG;

#if NO_MORE
typedef LONG NTSTATUS, * PNTSTATUS;
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#endif
#if NO_MORE
typedef struct _UNICODE_STRING32 {
	USHORT Length;
	USHORT MaximumLength;
	// note: on x64 in 64bit mode there is a 32bit pad here
	PVOID  Buffer;
} UNICODE_STRING, * PUNICODE_STRING;
#endif

#if NO_MORE
// NB: The following was previosly used(for file interface) and is replaced
//     by the latter, which is the current definition from winternl.h, which is probably more accurate for
//     x64
/////typedef struct _IO_STATUS_BLOCK
/////{
/////	DWORD Status; 
/////	ULONG Information;
/////} IO_STATUS_BLOCK,* PIO_STATUS_BLOCK; 
typedef struct _IO_STATUS_BLOCK {
	union {
		NTSTATUS Status;
		PVOID Pointer;
	};

	ULONG_PTR Information;
} IO_STATUS_BLOCK, * PIO_STATUS_BLOCK;
#endif

#if NO_MORE
typedef struct OBJECT_ATTRIBUTES {
	ULONG length;
	HANDLE	RootDirectory;					// larger in x64
	PVOID /*PUNICODE_STRING*/ ObjName;		// Definition changed to accomodate 64-bit also
	ULONG Atrtribute;
	PVOID SecurityDescriptor;
	PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, * POBJECT_ATTRIBUTES;
#endif

// The following defines may be commented out to reduce size
#define KDEF_FILEIF
//#define KDEF_FILE_CLASS_0  // non existent
#define KDEF_FILE_CLASS_1
#define KDEF_FILE_CLASS_2
#define KDEF_FILE_CLASS_3
#define KDEF_FILE_CLASS_4
#define KDEF_FILE_CLASS_5
#define KDEF_FILE_CLASS_6
#define KDEF_FILE_CLASS_7
#define KDEF_FILE_CLASS_8
#define KDEF_FILE_CLASS_9
#define KDEF_FILE_CLASS_10
#define KDEF_FILE_CLASS_11
#define KDEF_FILE_CLASS_12
#define KDEF_FILE_CLASS_13
#define KDEF_FILE_CLASS_14
//#define KDEF_FILE_CLASS_15		// does not exist
#define KDEF_FILE_CLASS_16
#define KDEF_FILE_CLASS_17
#define KDEF_FILE_CLASS_18
#define KDEF_FILE_CLASS_19
#define KDEF_FILE_CLASS_20
#define KDEF_FILE_CLASS_21
#define KDEF_FILE_CLASS_22
#define KDEF_FILE_CLASS_23
#define KDEF_FILE_CLASS_24
#define KDEF_FILE_CLASS_25
#define KDEF_FILE_CLASS_26
#define KDEF_FILE_CLASS_27
#define KDEF_FILE_CLASS_28
#define KDEF_FILE_CLASS_29
#define KDEF_FILE_CLASS_30
#define KDEF_FILE_CLASS_31
#define KDEF_FILE_CLASS_32
#define KDEF_FILE_CLASS_33
#define KDEF_FILE_CLASS_34
#define KDEF_FILE_CLASS_35
#define KDEF_FILE_CLASS_36

#ifdef KDEF_FILEIF

#if NO_MORE
typedef enum _FILE_INFORMATION_CLASS {
	FileDirectoryInformation = 1,	//  1 query		dir
	FileFullDirectiryInformation,	//  2 query		dir
	FileBothDirectoryInformation,	//  3 query		dir
	FileBasicInformation,			//  4 query/set	file
	FileStandardInformation,		//	5 query		file
	FileInternalInformation,		//	6 query		file
	FileEaInformation,				//  7 query		file
	FileAccessInformation,			//  8 query		file
	FileNameInformation,			//  9 query		file
	FileRenameInformation,			// 10 set		file
	FileLinkInformation,			// 11 set		file
	FileNamesInformation,			// 12 query		dir
	FileDispositionInformation,		// 13 set		file
	FilePositionInformation,		// 14 query/set	file
	// no 15???
	FileModeInformation = 16,		// 16 query/set	file
	FileAlignmentInformation,		// 17 query		file
	FileAllInformation,				// 18 query		file
	FileAllocationInformation,		// 19 set		file
	FileEndOfFileInformation,		// 20 set		file
	FileAlternateNameInformation,	// 21 query		file
	FileStreamInformation,			// 22 query		file
	FilePipeInformation,			// 23 Query		file
	FilePipeLocalInformation,		// 24 query		file
	FilePipeRemoteInformation,		// 25 query		file
	FileMailslotQueryInformation,	// 26 query		file
	FileMailslotSetInformation,		// 27 set		file
	FileCompressionInformation,		// 28 query		file
	FileObjectIdInformation,		// 29 query		file	// not  implemented
	FileCompletionInformation,		// 30 set		file
	FileMoveClusterInformation,		// 31 set		file	// not  implemented
	FileQuotaInformation,			// 32 query		file	// not  implemented
	FileReparsePointInformation,	// 33 query		file	// not  implemented
	FileNetworkOpenInformation,		// 34 query		file
	FileAttributeTagInformation,	// 35 query		file
	FileTrackingInformation			// 36 set		file	// missing structure
} FILE_INFORMATION_CLASS;
#endif

#ifdef KDEF_FILE_CLASS_1
typedef struct _FILE_DIRECTORY_INFORMATION
{
	ULONG NextEntryOffset;
	ULONG Unknown;
	LARGE_INTEGER	CreationTime;
	LARGE_INTEGER	LastAccessTime;
	LARGE_INTEGER	LastWriteTime;
	LARGE_INTEGER	CreateTime;
	LARGE_INTEGER	EndOfFile;
	LARGE_INTEGER	AllocationSize;
	ULONG	FileAttributes;
	ULONG	FileNameLength;
	WCHAR	FileName[1];
} FILE_DIRECTORY_INFORMATION, * PFILE_DIRECTORY_INFORMATION;
#endif // KDEF_FILE_CLASS_1

#ifdef KDEF_FILE_CLASS_2
typedef struct _FILE_FULL_DIRECTORY_INFORMATION
{
	ULONG NextEntryOffset;
	ULONG Unknown;
	LARGE_INTEGER	CreationTime;
	LARGE_INTEGER	LastAccessTime;
	LARGE_INTEGER	LastWriteTime;
	LARGE_INTEGER	CreateTime;
	LARGE_INTEGER	EndOfFile;
	LARGE_INTEGER	AllocationSize;
	ULONG	FileAttributes;
	ULONG	FileNameLength;
	ULONG	EaInformationLength;
	WCHAR	FileName[1];
} FILE_FULL_DIRECTORY_INFORMATION, * PFILE_FULL_DIRECTORY_INFORMATION;
#endif // KDEF_FILE_CLASS_2

#ifdef KDEF_FILE_CLASS_3
typedef struct _FILE_BOTH_DIRECTORY_INFORMATION
{
	ULONG NextEntryOffset;
	ULONG Unknown;
	LARGE_INTEGER	CreationTime;
	LARGE_INTEGER	LastAccessTime;
	LARGE_INTEGER	LastWriteTime;
	LARGE_INTEGER	CreateTime;
	LARGE_INTEGER	EndOfFile;
	LARGE_INTEGER	AllocationSize;
	ULONG	FileAttributes;
	ULONG	FileNameLength;
	ULONG	EaInformationLength;
	ULONG	AlternativeNameLength;
	WCHAR	AlternativeName[12];
	WCHAR	FileName[1];
} FILE_BOTH_DIRECTORY_INFORMATION, * PFILE_BOTH_DIRECTORY_INFORMATION;
#endif // KDEF_FILE_CLASS_3

#ifdef KDEF_FILE_CLASS_4
typedef struct _FILE_BASIC_INFORMATION
{
	LARGE_INTEGER	CreationTime;
	LARGE_INTEGER	LastAccessTime;
	LARGE_INTEGER	LastWriteTime;
	LARGE_INTEGER	CreateTime;
	ULONG	FileAttributes;
} FILE_BASIC_INFORMATION, * PFILE_BASIC_INFORMATION;
#endif // KDEF_FILE_CLASS_4

#ifdef KDEF_FILE_CLASS_5
typedef struct _FILE_STANDARD_INFORMATION
{
	LARGE_INTEGER AllocationSize;
	LARGE_INTEGER EndOfFile;
	ULONG			NumberOfLinks;
	BOOLEAN			DeletePending;
	BOOLEAN			Directory;
} FILE_STANDARD_INFORMATION, * PFILE_STANDARD_INFORMATION;
#endif // KDEF_FILE_CLASS_5

#ifdef KDEF_FILE_CLASS_6
typedef struct _FILE_INTERNAL_INFORMATION
{
	LARGE_INTEGER FileId;
} FILE_INTERNAL_INFORMATION, * PFILE_INTERNAL_INFORMATION;
#endif // KDEF_FILE_CLASS_6

#ifdef KDEF_FILE_CLASS_7
typedef struct _FILE_EA_INFORMATION
{
	ULONG EaInformationLength;
} FILE_EA_INFORMATION, * PFILE_EA_INFORMATION;
#endif // KDEF_FILE_CLASS_7

#ifdef KDEF_FILE_CLASS_8
typedef struct _FILE_ACCESS_INFORMATION
{
	ACCESS_MASK	GrantedAccess;
} FILE_ACCESS_INFORMATION, * PFILE_ACCESS_INFORMATION;
#endif // KDEF_FILE_CLASS_8

#ifdef KDEF_FILE_CLASS_9
typedef struct _FILE_NAME_INFORMATION  // used in 9 & 21
{
	ULONG FileNameLength;
	WCHAR FileName[1];
} FILE_NAME_INFORMATION, * PFILE_NAME_INFORMATION;
#endif // KDEF_FILE_CLASS_9

#ifdef KDEF_FILE_CLASS_10
typedef struct _FILE_LINK_RENAME_INFORMATION // used in 10 & 11
{
	BOOLEAN ReplaceIfExists;
	HANDLE  RootDirectory;
	ULONG	FileNameLength;
	WCHAR FileName[1];
} FILE_LINK_RENAME_INFORMATION, * PFILE_LINK_RENAME_INFORMATION;
#endif // KDEF_FILE_CLASS_10

#ifdef KDEF_FILE_CLASS_11
#ifndef KDEF_FILE_CLASS_10
typedef struct _FILE_LINK_RENAME_INFORMATION // used in 10 & 11
{
	BOOLEAN ReplaceIfExists;
	HANDLE  RootDirectory;
	ULONF	FileNameLength;
	WCHAR FileName[1];
} FILE_LINK_RENAME_INFORMATION, * PFILE_LINK_RENAME_INFORMATION;
#endif // KDEF_FILE_CLASS_10
#endif // KDEF_FILE_CLASS_11

#ifdef KDEF_FILE_CLASS_12
typedef struct _FILE_NAMES_INFORMATION
{
	ULONG NextEntryOffset;
	ULONG Unknown;
	ULONG FileNameLength;
	WCHAR FileName[1];
} FILE_NAMES_INFORMATION, * PFILE_NAMES_INFORMATION;
#endif // KDEF_FILE_CLASS_12

#ifdef KDEF_FILE_CLASS_13
typedef struct _FILE_DISPOSITION_INFORMATION
{
	BOOLEAN DeleteFile;
} FILE_DISPOSITION_INFORMATION, * PFILE_DISPOSITION_INFORMATION;
#endif // KDEF_FILE_CLASS_13

#ifdef KDEF_FILE_CLASS_14
typedef struct _FILE_POSITION_INFORMATION
{
	LARGE_INTEGER CurrentByteOffset;
} FILE_POSITION_INFORMATION, * PFILE_POSITION_INFORMATION;
#endif // KDEF_FILE_CLASS_14

// no 15 

#ifdef KDEF_FILE_CLASS_16
typedef struct _FILE_MODE_INFORMATION
{
	ULONG Mode;
} FILE_MODE_INFORMATION, * PFILE_MODE_INFORMATION;
#endif // KDEF_FILE_CLASS_16

#ifdef KDEF_FILE_CLASS_17
typedef struct _FILE_ALIGNMENT_INFORMATION
{
	ULONG AlignmentRequirement;
} FILE_ALIGNMENT_INFORMATION, * PFILE_ALIGNMENT_INFORMATION;
#endif // KDEF_FILE_CLASS_17

#ifdef KDEF_FILE_CLASS_18
typedef struct _FILE_ALL_INFORMATION
{
	FILE_BASIC_INFORMATION		BasicInformation;
	FILE_STANDARD_INFORMATION	StandardInformation;
	FILE_INTERNAL_INFORMATION	InternalInformation;
	FILE_EA_INFORMATION			EaInformation;
	FILE_ACCESS_INFORMATION		AccessInformation;
	FILE_POSITION_INFORMATION	PositionInformation;
	FILE_MODE_INFORMATION		ModeInformation;
	FILE_ALIGNMENT_INFORMATION	AlignmentInformation;
	FILE_NAME_INFORMATION		NameInformation;
} FILE_ALL_INFORMATION, * PFILE_ALL_INFORMATION;
#endif // KDEF_FILE_CLASS_18

#ifdef KDEF_FILE_CLASS_19
typedef struct _FILE_ALLOCATION_INFORMATION
{
	LARGE_INTEGER AllocationSize;
} FILE_ALLOCATION_INFORMATION, * PFILE_ALLOCATION_INFORMATION;
#endif // KDEF_FILE_CLASS_19

#ifdef KDEF_FILE_CLASS_20
typedef struct _FILE_END_OF_FILE_INFORMATION
{
	LARGE_INTEGER EndOfFile;
} FILE_END_OF_FILE_INFORMATION, * PFILE_END_OF_FILE_INFORMATION;
#endif // KDEF_FILE_CLASS_20

#ifdef KDEF_FILE_CLASS_21
#ifndef KDEF_FILE_CLASS_9
typedef struct _FILE_NAME_INFORMATION  // used in 9 & 21
{
	ULONG FileNameLength;
	WCHAR FileName[1];
} FILE_NAME_INFORMATION, * PFILE_NAME_INFORMATION;
#endif // KDEF_FILE_CLASS_9
#endif // KDEF_FILE_CLASS_21

#ifdef KDEF_FILE_CLASS_22
typedef struct _FILE_STREAM_INFORMATION
{
	ULONG NextEntryOffset;
	ULONG StreamNameLength;
	LARGE_INTEGER EndOfStream;
	LARGE_INTEGER AllocationSize;
	WCHAR StreamName[1];
} FILE_STREAM_INFORMATION, * PFILE_STREAM_INFORMATION;
#endif // KDEF_FILE_CLASS_22

#ifdef KDEF_FILE_CLASS_23
typedef struct _FILE_PIPE_INFORMATION
{
	ULONG ReadModeMessage;
	ULONG	WaitModeBlocking;
} FILE_PIPE_INFORMATION, * PFILE_PIPE_INFORMATION;
#endif // KDEF_FILE_CLASS_23

#ifdef KDEF_FILE_CLASS_24
typedef struct _FILE_PIPE_LOCAL_INFORMATION
{
	ULONG	MessageType;
	ULONG	Unknown1;
	ULONG	MaxInstances;
	ULONG	CurInstances;
	ULONG	InBufferSize;
	ULONG	Unknown2;
	ULONG	OutBufferSize;
	ULONG	Unknown3[2];
	ULONG	ServerEnd;
} FILE_PIPE_LOCAL_INFORMATION, * PFILE_PIPE_LOCAL_INFORMATION;
#endif // KDEF_FILE_CLASS_24

#ifdef KDEF_FILE_CLASS_25
typedef struct _FILE_PIPE_REMOTE_INFORMATION
{
	LARGE_INTEGER	CollectionDataTimeout;
	ULONG			MaxCollectionCount;
} FILE_PIPE_REMOTE_INFORMATION, * PFILE_PIPE_REMOTE_INFORMATION;
#endif // KDEF_FILE_CLASS_25

#ifdef KDEF_FILE_CLASS_26
typedef struct _FILE_MAILSLOT_QUERY_INFORMATION
{
	ULONG	MaxMessageSize;
	ULONG	Unknown;
	ULONG	NextSize;
	ULONG	MessageCount;
	LARGE_INTEGER	ReadTimeout;
} FILE_MAILSLOT_QUERY_INFORMATION, * PFILE_MAILSLOT_QUERY_INFORMATION;
#endif // KDEF_FILE_CLASS_26

#ifdef KDEF_FILE_CLASS_27
typedef struct _FILE_MAILSLOT_SET_INFORMATION
{
	LARGE_INTEGER	ReadTimeout;
} FILE_MAILSLOT_SET_INFORMATION, * PFILE_MAILSLOT_SET_INFORMATION;
#endif // KDEF_FILE_CLASS_27

#ifdef KDEF_FILE_CLASS_28
typedef struct _FILE_COMPRESSION_INFORMATION
{
	LARGE_INTEGER	CompressedSize;
	USHORT	CompressionFormat;
	UCHAR	CompressionUnitShift;
	UCHAR	Unknown;
	UCHAR	ClusterSizeShift;
} FILE_COMPRESSION_INFORMATION, * PFILE_COMPRESSION_INFORMATION;
#endif // KDEF_FILE_CLASS_28

#ifdef KDEF_FILE_CLASS_29
// not implemented
#endif // KDEF_FILE_CLASS_29

#ifdef KDEF_FILE_CLASS_30
typedef struct _FILE_COMPLETION_INFORMATION
{
	HANDLE IoCompletionHandle;
	ULONG CompletionKey;
} FILE_COMPLETION_INFORMATION, * PFILE_COMPLETION_INFORMATION;
#endif // KDEF_FILE_CLASS_30

#ifdef KDEF_FILE_CLASS_31
// not implemented
#endif // KDEF_FILE_CLASS_31

#ifdef KDEF_FILE_CLASS_32
// not implemented
#endif // KDEF_FILE_CLASS_32

#ifdef KDEF_FILE_CLASS_33
// not implemented

#endif // KDEF_FILE_CLASS_33

#ifdef KDEF_FILE_CLASS_34
typedef struct _FILE_NETWORK_OPEN_INFORMATION
{
	LARGE_INTEGER	CreationTime;
	LARGE_INTEGER	LastAccessTime;
	LARGE_INTEGER	LastWriteTime;
	LARGE_INTEGER	ChangeTime;
	LARGE_INTEGER	AllocationSize;
	LARGE_INTEGER	EndOfFile;
	ULONG	FileAttributes;
} FILE_NETWORK_OPEN_INFORMATION, * PFILE_NETWORK_OPEN_INFORMATION;
#endif // KDEF_FILE_CLASS_34

#ifdef KDEF_FILE_CLASS_35
typedef struct _FILE_ATTRIBUTE_TAG_INFORMATION
{
	ULONG	FileAttributes;
	ULONG	ReparseFlag;
} FILE_ATTRIBUTE_TAG_INFORMATION, * PFILE_ATTRIBUTE_TAG_INFORMATION;
#endif // KDEF_FILE_CLASS_35

#ifdef KDEF_FILE_CLASS_36
// Missing structure
#endif // KDEF_FILE_CLASS_36

#endif // KDEF_FILEIF

#ifdef KDEF_FILEIF

typedef DWORD(WINAPI* PNTOPENFILE)(PHANDLE pFileHandle, ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK IoStatusBlock, ULONG ShareAccess, ULONG OpenOptions);

typedef DWORD(WINAPI* PNTQUERYINFORMATIONFILE)(IN HANDLE FileHandle, OUT PIO_STATUS_BLOCK IoStatusBlock, OUT PVOID FileInformation, IN ULONG FileInformationLength, IN FILE_INFORMATION_CLASS FileInformationClass);

/////typedef DWORD (WINAPI *PNTQUERYATTRIBUTESFILE)(IN POBJECT_ATTRIBUTES pOa, OUT PFILE_BASIC_INFORMATION pFbi);

#endif // KDEF_FILEIF

#ifdef KDEF_FILEIF

// ACCESS_MASK is normally treated as a DWORD and ypu bit things in,
// this is an alternate defiition that is binary compatible
typedef struct _ACCESS_MASK_ALT {
	WORD  SpecificRights;
	BYTE  StandardRights;
	BYTE  AccessSystemAcl : 1;
	BYTE  Reserved : 3;
	BYTE  GenericAll : 1;
	BYTE  GenericExecute : 1;
	BYTE  GenericWrite : 1;
	BYTE  GenericRead : 1;
} ACCESS_MASK_ALT, * PACCESS_MASK_ALT;

#ifndef FILE_DIRECTORY_FILE
// Taken from ntddk.h:  These are the options for NTCreate/Open file which are also returned in FILE_MODE_INFORMATION
#define FILE_DIRECTORY_FILE                     0x00000001
#define FILE_WRITE_THROUGH                      0x00000002
#define FILE_SEQUENTIAL_ONLY                    0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING          0x00000008

#define FILE_SYNCHRONOUS_IO_ALERT               0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT            0x00000020
#define FILE_NON_DIRECTORY_FILE                 0x00000040
#define FILE_CREATE_TREE_CONNECTION             0x00000080

#define FILE_COMPLETE_IF_OPLOCKED               0x00000100
#define FILE_NO_EA_KNOWLEDGE                    0x00000200
#define FILE_OPEN_FOR_RECOVERY                  0x00000400
#define FILE_RANDOM_ACCESS                      0x00000800

#define FILE_DELETE_ON_CLOSE                    0x00001000
#define FILE_OPEN_BY_FILE_ID                    0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT             0x00004000
#define FILE_NO_COMPRESSION                     0x00008000

#define FILE_RESERVE_OPFILTER                   0x00100000
#define FILE_OPEN_REPARSE_POINT                 0x00200000
#define FILE_OPEN_NO_RECALL                     0x00400000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY          0x00800000

#define FILE_COPY_STRUCTURED_STORAGE            0x00000041
#define FILE_STRUCTURED_STORAGE                 0x00000441

#define FILE_VALID_OPTION_FLAGS                 0x00ffffff
#define FILE_VALID_PIPE_OPTION_FLAGS            0x00000032
#define FILE_VALID_MAILSLOT_OPTION_FLAGS        0x00000032
#define FILE_VALID_SET_FLAGS                    0x00000036
#endif
#endif //#ifdef KDEF_FILEIF