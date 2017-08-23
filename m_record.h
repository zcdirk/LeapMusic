#include <mmsystem.h>

/*typedef enum _enum_channel
{
	ENUM_CHANNEL_MONO,			// 单声道
	ENUM_CHANNEL_STEREO,		// 立体声
	ENUM_CHANNEL_LEFT,			// 左声道
	ENUM_CHANNEL_RIGHT,			// 右声道
	ENUM_CHANNEL_NUM,
} ENUM_CHANNEL;

HINSTANCE			m_ForMp3_hDLL_LameEnc;
PBYTE				m_ForMp3_pMP3Buffer;
PBYTE				m_ForMp3_pWaveBuffer;
DWORD				m_ForMp3_dwWaveBufferSize;
DWORD				m_ForMp3_dwWaveDataBytes;
HBE_STREAM			m_ForMp3_Proc_hStream;
BEINITSTREAM		m_ForMp3_Proc_hInitStream;
BEENCODECHUNK		m_ForMp3_Proc_hEncodeChunk;
BEDEINITSTREAM		m_ForMp3_Proc_hDeinitStream;
BECLOSESTREAM		m_ForMp3_Proc_hCloseStream;
BEVERSION			m_ForMp3_Proc_hVersion;
BEWRITEVBRHEADER	m_ForMp3_Proc_hWriteVBRHeader;
BEWRITEINFOTAG		m_ForMp3_Proc_hWriteInfoTag;
FILE*				m_pFileMp3;
CString				m_csMp3FileName;
DWORD				m_dwSamplesEncodeMp3Block; 

void ResetMp3EncodeVar()
{
	m_ForMp3_Proc_hInitStream = NULL;
	m_ForMp3_Proc_hEncodeChunk = NULL;
	m_ForMp3_Proc_hDeinitStream = NULL;
	m_ForMp3_Proc_hCloseStream = NULL;
	m_ForMp3_Proc_hVersion = NULL;
	m_ForMp3_Proc_hWriteVBRHeader = NULL;
	m_ForMp3_Proc_hWriteInfoTag = NULL;
	m_pFileMp3 = NULL;
	m_ForMp3_hDLL_LameEnc = NULL;
	m_ForMp3_Proc_hStream = NULL;
	m_ForMp3_pMP3Buffer = NULL;
	m_ForMp3_pWaveBuffer = NULL;
	m_ForMp3_dwWaveDataBytes = 0;
	m_ForMp3_dwWaveBufferSize = 0;
	m_dwSamplesEncodeMp3Block = 0;
}

void record(ENUM_CHANNEL eChannel, DWORD nSamplesPerSec, WORD wBitsPerSample, LPCTSTR lpszAudioFileName)
{
	m_bRecording = TRUE;
	MMRESULT mmReturn = 0;
	ResetMp3EncodeVar();
	ASSERT((wBitsPerSample % 8) == 0);
	if (wBitsPerSample > 16)
		wBitsPerSample = 16;


	SetWaveFormat(eChannel, nSamplesPerSec, wBitsPerSample);
	if (!SetRelateParaAfterGetWaveFormat())
	{
		return FALSE;
	}

	// 准备音频文件
	if (lpszAudioFileName)
	{
		CString csFileName = lpszAudioFileName;
		csFileName.MakeLower();
		// 需要保存的mp3文件中
		if (csFileName.Find(".mp3") == csFileName.GetLength() - 4)
		{
			if (wBitsPerSample == 8)
			{
				FreeBuffer();
				AfxMessageBox("Cannot record 8 bits mp3");
				return FALSE;
			}

			if (!PrepareEncodeMp3(csFileName))
			{
				FreeBuffer();
				return FALSE;
			}
		}
		else
		{
			// 创建一个wave文件
			if (!CreateWaveFile(csFileName))
			{
				FreeBuffer();
				return FALSE;
			}
		}
	}

	// open wavein device
	mmReturn = ::waveInOpen(&m_hRecord, m_uDeviceID, &m_Format, (DWORD)GetSafeHwnd(), NULL, CALLBACK_WINDOW);
	if (mmReturn)
	{
		waveErrorMsg(mmReturn, "waveInOpen()");
		FreeBuffer();
		return FALSE;
	}
	else
	{
		// make several input buffers and add them to the input queue
		for (int i = 0; i<m_wInQueu; i++)
		{
			AddInputBufferToQueue(i);
		}

		// start recording
		mmReturn = ::waveInStart(m_hRecord);
		if (mmReturn)
		{
			waveErrorMsg(mmReturn, "waveInStart() failed");
			FreeBuffer();
			return FALSE;
		}
	}
}

void StartRec(const  char* path)
{
	record((CAudioPlayRec::ENUM_CHANNEL)m_nChannels,11025 * 2, 16, path);
}

StopRec()*/