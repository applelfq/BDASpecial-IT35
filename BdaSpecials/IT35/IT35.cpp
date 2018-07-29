#include <Windows.h>
#include <stdio.h>

#include <string>

#include "IT35.h"

#include <iostream>
#include <dshow.h>

#include "common.h"

#include "IT35propset.h"

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
	// DebugLog���L�^���邩�ǂ���
	if (::GetPrivateProfileIntW(L"IT35", L"DebugLog", 0, szIniFilePath)) {
		// INI�t�@�C���̃t�@�C�����擾
		WCHAR szDebugLogPath[_MAX_PATH + 1];
		::GetModuleFileNameW(hMySelf, szDebugLogPath, _MAX_PATH + 1);
		::wcscpy_s(szDebugLogPath + ::wcslen(szDebugLogPath) - 3, 4, L"log");
		SetDebugLog(szDebugLogPath);
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
	  m_pIKsPropertySet(NULL),
	  m_bRewriteIFFreq(FALSE),
	  m_bPrivateSetTSID(FALSE),
	  m_bLNBPowerON(FALSE)
{
	::InitializeCriticalSection(&m_CriticalSection);

	HRESULT hr;

	hr = m_pTunerDevice->QueryInterface(IID_IKsPropertySet, (LPVOID*)&m_pIKsPropertySet);

	return;
}

CIT35Specials::~CIT35Specials()
{
	m_hMySelf = NULL;

	SAFE_RELEASE(m_pIKsPropertySet);

	if (m_pTunerDevice) {
		m_pTunerDevice.Release();
		m_pTunerDevice = NULL; 
	}

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

const HRESULT CIT35Specials::LockChannel(const TuningParam *pTuningParm)
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
	// IF���g���� put_CarrierFrequency() ���s��
	m_bRewriteIFFreq = (BOOL)::GetPrivateProfileIntW(L"IT35", L"RewriteIFFreq", 0, szIniFilePath);

	// �ŗL�� Property set ���g�p���� TSID �̏����݂��K�v
	m_bPrivateSetTSID = (BOOL)::GetPrivateProfileIntW(L"IT35", L"PrivateSetTSID", 0, szIniFilePath);

	// LNB�d���̋�����ON����
	m_bLNBPowerON = (BOOL)::GetPrivateProfileIntW(L"IT35", L"LNBPowerON", 0, szIniFilePath);

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

const HRESULT CIT35Specials::PreTuneRequest(const TuningParam *pTuningParm, ITuneRequest *pITuneRequest)
{
	if (!m_pIKsPropertySet)
		return E_FAIL;

	HRESULT hr;

	// IF���g���ɕϊ�
	if (m_bRewriteIFFreq && pTuningParm->Antenna->HighOscillator != -1 || pTuningParm->Antenna->LowOscillator != -1) {
		long freq = pTuningParm->Frequency;
		if (pTuningParm->Antenna->LNBSwitch != -1) {
			if (freq < pTuningParm->Antenna->LNBSwitch)
				freq = freq - pTuningParm->Antenna->LowOscillator;
			else
				freq = freq - pTuningParm->Antenna->HighOscillator;
		}
		else {
			if (pTuningParm->Antenna->Tone == 0)
				freq = freq - pTuningParm->Antenna->LowOscillator;
			else
				freq = freq - pTuningParm->Antenna->HighOscillator;
		}

		CComPtr<ILocator> pILocator;
		hr = pITuneRequest->get_Locator(&pILocator);
		if (FAILED(hr) || !pILocator) {
			OutputDebug(L"ITuneRequest->get_Locator failed.\n");
			return E_FAIL;
		}

		pILocator->put_CarrierFrequency(freq);

		hr = pITuneRequest->put_Locator(pILocator);
	}

	// TSID��Set����
	if (m_bPrivateSetTSID && pTuningParm->TSID != 0 && pTuningParm->TSID != -1) {
		::EnterCriticalSection(&m_CriticalSection);
		hr = it35_PutISDBIoCtl(m_pIKsPropertySet, (WORD)pTuningParm->TSID);
		::LeaveCriticalSection(&m_CriticalSection);
	}

	return S_OK;
}

const HRESULT CIT35Specials::PostLockChannel(const TuningParam *pTuningParm)
{
	return S_OK;
}

void CIT35Specials::Release(void)
{
	delete this;
}
