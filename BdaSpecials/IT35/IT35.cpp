#include "common.h"

#include "IT35.h"

#include <Windows.h>
#include <string>

#include <dshow.h>

#include "CIniFileAccess.h"
#include "IT35propset.h"
#include "CIniFileAccess.h"

#pragma comment(lib, "Strmiids.lib" )

FILE *g_fpLog = NULL;

HMODULE hMySelf;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		// ���W���[���n���h���ۑ�
		hMySelf = hModule;
		break;

	case DLL_PROCESS_DETACH:
		// �f�o�b�O���O�t�@�C���̃N���[�Y
		CloseDebugLog();
		break;
	}
    return TRUE;
}

__declspec(dllexport) IBdaSpecials * CreateBdaSpecials(CComPtr<IBaseFilter> pTunerDevice)
{
	return new CIT35Specials(hMySelf, pTunerDevice);
}

__declspec(dllexport) HRESULT CheckAndInitTuner(IBaseFilter *pTunerDevice, const WCHAR *szDisplayName, const WCHAR *szFriendlyName, const WCHAR *szIniFilePath)
{
	CIniFileAccess IniFileAccess(szIniFilePath);

	// DebugLog���L�^���邩�ǂ���
	if (IniFileAccess.ReadKeyB(L"IT35", L"DebugLog", FALSE)) {
		// INI�t�@�C���̃t�@�C�����擾
		// DebugLog�̃t�@�C�����擾
		SetDebugLog(common::GetModuleName(hMySelf) + L"log");
	}

	return S_OK;
}

__declspec(dllexport) HRESULT CheckCapture(const WCHAR *szTunerDisplayName, const WCHAR *szTunerFriendlyName,
	const WCHAR *szCaptureDisplayName, const WCHAR *szCaptureFriendlyName, const WCHAR *szIniFilePath)
{
	// ���ꂪ�Ă΂ꂽ�Ƃ������Ƃ�Bondriver_BDA.ini�̐ݒ肪��������
	OutputDebug(L"CheckCapture called.\n");

	// connect()�����݂Ă����ʂȂ̂� E_FAIL ��Ԃ��Ă���
	return E_FAIL;
}

CIT35Specials::CIT35Specials(HMODULE hMySelf, CComPtr<IBaseFilter> pTunerDevice)
	: m_hMySelf(hMySelf),
	  m_pTunerDevice(pTunerDevice),
	  m_pIKsPropertySet(m_pTunerDevice),
	  m_CurrentModulationType(BDA_MOD_NOT_SET),
	  m_bRewriteIFFreq(FALSE),
	  m_nPrivateSetTSID(enumPrivateSetTSID::ePrivateSetTSIDNone),
	  m_bLNBPowerON(FALSE),
	  m_bDualModeISDB(FALSE)
{
	::InitializeCriticalSection(&m_CriticalSection);

	return;
}

CIT35Specials::~CIT35Specials()
{
	m_hMySelf = NULL;

	::DeleteCriticalSection(&m_CriticalSection);

	return;
}

const HRESULT CIT35Specials::InitializeHook(void)
{
	if (!m_pTunerDevice) {
		return E_POINTER;
	}

	if (!m_pIKsPropertySet)
		return E_FAIL;

	HRESULT hr;

	if (m_bLNBPowerON) {
		// ini�t�@�C���Ŏw�肳��Ă���� ������LNB Power ��ON����
		// LNB Power ��OFF��BDA driver������ɂ���Ă����݂���
		::EnterCriticalSection(&m_CriticalSection);
		hr = it35_PutLNBPower(m_pIKsPropertySet, 1);
		::LeaveCriticalSection(&m_CriticalSection);

		if FAILED(hr)
			OutputDebug(L"SetLNBPower failed.\n");
		else
			OutputDebug(L"SetLNBPower Success.\n");
	}
	return S_OK;
}

const HRESULT CIT35Specials::Set22KHz(bool bActive)
{
	return E_NOINTERFACE;
}

const HRESULT CIT35Specials::Set22KHz(long nTone)
{
	return E_NOINTERFACE;
}

const HRESULT CIT35Specials::FinalizeHook(void)
{
	return S_OK;
}

const HRESULT CIT35Specials::GetSignalState(int *pnStrength, int *pnQuality, int *pnLock)
{
	return E_NOINTERFACE;
}

const HRESULT CIT35Specials::LockChannel(BYTE bySatellite, BOOL bHorizontal, unsigned long ulFrequency, BOOL bDvbS2)
{
	return E_NOINTERFACE;
}

const HRESULT CIT35Specials::LockChannel(const TuningParam *pTuningParam)
{
	return E_NOINTERFACE;
}

const HRESULT CIT35Specials::SetLNBPower(bool bActive)
{
	// �g���ĂȂ����A�܂�������
	return E_NOINTERFACE;
}

const HRESULT CIT35Specials::ReadIniFile(const WCHAR *szIniFilePath)
{
	static const std::map<const std::wstring, const int, std::less<>> mapPrivateSetTSID = {
		{ L"NO",      enumPrivateSetTSID::ePrivateSetTSIDNone },
		{ L"YES",     enumPrivateSetTSID::ePrivateSetTSIDPreTR },
		{ L"PRETR",   enumPrivateSetTSID::ePrivateSetTSIDPreTR },
		{ L"POSTTR",  enumPrivateSetTSID::ePrivateSetTSIDPostTR },
	};

	CIniFileAccess IniFileAccess(szIniFilePath);
	IniFileAccess.SetSectionName(L"IT35");

	// IF���g���� put_CarrierFrequency() ���s��
	m_bRewriteIFFreq = IniFileAccess.ReadKeyB(L"RewriteIFFreq", FALSE);

	// �ŗL�� Property set ���g�p����TSID�̏����݂��s�����[�h
	m_nPrivateSetTSID = (enumPrivateSetTSID)IniFileAccess.ReadKeyIValueMap(L"PrivateSetTSID", enumPrivateSetTSID::ePrivateSetTSIDNone, mapPrivateSetTSID);

	// LNB�d���̋�����ON����
	m_bLNBPowerON = IniFileAccess.ReadKeyB(L"LNBPowerON", FALSE);

	// Dual Mode ISDB Tuner
	m_bDualModeISDB = IniFileAccess.ReadKeyB(L"DualModeISDB", FALSE);

	return S_OK;
}

const HRESULT CIT35Specials::IsDecodingNeeded(BOOL *pbAns)
{
	if (pbAns)
		*pbAns = FALSE;

	return S_OK;
}

const HRESULT CIT35Specials::Decode(BYTE *pBuf, DWORD dwSize)
{
	return E_NOINTERFACE;
}

const HRESULT CIT35Specials::GetSignalStrength(float *fVal)
{
	return E_NOINTERFACE;
}

const HRESULT CIT35Specials::PreLockChannel(TuningParam *pTuningParam)
{
	return S_OK;
}

const HRESULT CIT35Specials::PreTuneRequest(const TuningParam *pTuningParam, ITuneRequest *pITuneRequest)
{
	if (!m_pIKsPropertySet)
		return E_FAIL;

	HRESULT hr;

	// Dual Mode ISDB Tuner�̏ꍇ�̓f���W�����[�^�[�̕���Mode��ݒ�
	if (m_bDualModeISDB && pTuningParam->Modulation->Modulation != m_CurrentModulationType) {
		switch (pTuningParam->Modulation->Modulation) {
		case BDA_MOD_ISDB_T_TMCC:
			hr = it35_DigibestPrivateIoControl(m_pIKsPropertySet, PRIVATE_IO_CTL_FUNC_DEMOD_OFDM);
			break;
		case BDA_MOD_ISDB_S_TMCC:
			hr = it35_DigibestPrivateIoControl(m_pIKsPropertySet, PRIVATE_IO_CTL_FUNC_DEMOD_PSK);
			break;
		}
		m_CurrentModulationType = pTuningParam->Modulation->Modulation;
	}

	// Dual Mode ISDB Tuner�̏ꍇ��ISDB-S�̎��̂�
	if (!m_bDualModeISDB || pTuningParam->Modulation->Modulation == BDA_MOD_ISDB_S_TMCC) {
		// IF���g���ɕϊ�
		if (m_bRewriteIFFreq && pTuningParam->Antenna->HighOscillator != -1 || pTuningParam->Antenna->LowOscillator != -1) {
			long freq = pTuningParam->Frequency;
			if (pTuningParam->Antenna->LNBSwitch != -1) {
				if (freq < pTuningParam->Antenna->LNBSwitch)
					freq = freq - pTuningParam->Antenna->LowOscillator;
				else
					freq = freq - pTuningParam->Antenna->HighOscillator;
			}
			else {
				if (pTuningParam->Antenna->Tone == 0)
					freq = freq - pTuningParam->Antenna->LowOscillator;
				else
					freq = freq - pTuningParam->Antenna->HighOscillator;
			}

			CComPtr<ILocator> pILocator;
			if (FAILED(hr = pITuneRequest->get_Locator(&pILocator))) {
				OutputDebug(L"ITuneRequest::get_Locator failed.\n");
				return hr;
			}

			pILocator->put_CarrierFrequency(freq);

			hr = pITuneRequest->put_Locator(pILocator);
		}

		// TSID��Set����
		if (m_nPrivateSetTSID == enumPrivateSetTSID::ePrivateSetTSIDPreTR && pTuningParam->TSID != 0 && pTuningParam->TSID != -1) {
			::EnterCriticalSection(&m_CriticalSection);
			hr = it35_PutISDBIoCtl(m_pIKsPropertySet, (WORD)pTuningParam->TSID);
			::LeaveCriticalSection(&m_CriticalSection);
		}
	}
	return S_OK;
}

const HRESULT CIT35Specials::PostTuneRequest(const TuningParam * pTuningParam)
{
	if (!m_pIKsPropertySet)
		return E_FAIL;

	HRESULT hr;

	// TSID��Set����
	if (m_nPrivateSetTSID == enumPrivateSetTSID::ePrivateSetTSIDPostTR && pTuningParam->TSID != 0 && pTuningParam->TSID != -1) {
		::EnterCriticalSection(&m_CriticalSection);
		hr = it35_PutISDBIoCtl(m_pIKsPropertySet, (WORD)pTuningParam->TSID);
		::LeaveCriticalSection(&m_CriticalSection);
	}
	return S_OK;
}

const HRESULT CIT35Specials::PostLockChannel(const TuningParam *pTuningParam)
{
	return S_OK;
}

void CIT35Specials::Release(void)
{
	delete this;
}
