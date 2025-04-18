/*******************************************************************************
The content of this file includes portions of the AUDIOKINETIC Wwise Technology
released in source code form as part of the SDK installer package.

Commercial License Usage

Licensees holding valid commercial licenses to the AUDIOKINETIC Wwise Technology
may use this file in accordance with the end user license agreement provided 
with the software or, alternatively, in accordance with the terms contained in a
written agreement between you and Audiokinetic Inc.

  Copyright (c) 2025 Audiokinetic Inc.
*******************************************************************************/
//////////////////////////////////////////////////////////////////////
//
// AkFileLocationBase.h
//
// Basic file location resolving: Uses simple path concatenation logic.
// Exposes basic path functions for convenience.
// For more details on resolving file location, refer to section "File Location" inside
// "Going Further > Overriding Managers > Streaming / Stream Manager > Low-Level I/O"
// of the SDK documentation. 
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_FILE_LOCATION_BASE_H_
#define _AK_FILE_LOCATION_BASE_H_

struct AkFileSystemFlags;
struct AkFileOpenData;

#include <AK/SoundEngine/Common/AkTypes.h>

class CAkFileLocationBase
{
public:
	CAkFileLocationBase();
	virtual ~CAkFileLocationBase();

	//
	// Global path functions.
	// ------------------------------------------------------

	// Base path is prepended to all file names.
	// Audio source path is appended to base path whenever uCompanyID is AK and uCodecID specifies an audio source.
	// Bank path is appended to base path whenever uCompanyID is AK and uCodecID specifies a sound bank.
	// Language specific dir name is appended to path whenever "bIsLanguageSpecific" is true.
	AKRESULT SetBasePath(
		const AkOSChar*   in_pszBasePath
		);
	AKRESULT SetBankPath(
		const AkOSChar*   in_pszBankPath
		);
	AKRESULT SetAudioSrcPath(
		const AkOSChar*   in_pszAudioSrcPath
		);
	// Note: SetLangSpecificDirName() does not exist anymore. See release note WG-19397 (Wwise 2011.2).

	//
	// Path resolving services.
	// ------------------------------------------------------

	// Returns AK_Success if input flags are supported and the resulting path is not too long.
	// Returns AK_FilePathTooLong if the path is too long.
	AKRESULT GetFullFilePath(
		const AkFileOpenData& in_FileOpen,
		AkOSChar *			out_pszFullFilePath, // Full file path.
		bool in_bUseSubfolding = false
		);

	/// Returns AK_Success if the directory is valid, AK_PathNotFound if not and AK_Fail on error.
	/// For validation purposes only.
	virtual AKRESULT CheckDirectoryExists(
	  const AkOSChar* in_pszDirectoryPath
	  ) = 0;

protected:

	// Internal user paths.
	AkOSChar			m_szBasePath[AK_MAX_PATH];
	AkOSChar			m_szBankPath[AK_MAX_PATH];
	AkOSChar			m_szAudioSrcPath[AK_MAX_PATH];

};

#endif //_AK_FILE_LOCATION_BASE_H_
