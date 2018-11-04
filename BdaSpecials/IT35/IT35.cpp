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
		// モジュールハンドル保存
		hMySelf = hModule;
		break;

	case DLL_PROCESS_DETACH:
		// デバッグログファイルのクローズ
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

	// DebugLogを記録するかどうか
	if (IniFileAccess.ReadKeyB(L"IT35", L"DebugLog", 0)) {
		// INIファイルのファイル名取得
		// DebugLogのファイル名取得
		SetDebugLog(common::GetModuleName(hMySelf) + L"log");
	}

	return S_OK;
}

__declspec(dllexport) HRESULT CheckCapture(const WCHAR *szTunerDisplayName, const WCHAR *szTunerFriendlyName,
	const WCHAR *szCaptureDisplayName, const WCHAR *szCaptureFriendlyName, const WCHAR *szIniFilePath)
{
	// これが呼ばれたということはBondriver_BDA.iniの設定がおかしい
	OutputDebug(L"CheckCapture called.\n");

	// connect()を試みても無駄なので E_FAIL を返しておく
	return E_FAIL;
}

CIT35Specials::CIT35Specials(HMODULE hMySelf, CComPtr<IBaseFilter> pTunerDevice)
	: m_hMySelf(hMySelf),
	  m_pTunerDevice(pTunerDevice),
	  m_pIKsPropertySet(m_pTunerDevice),
	  m_CurrentModulationType(BDA_MOD_NOT_SET),
	  m_bRewriteIFFreq(FALSE),
	  m_bPrivateSetTSID(FALSE),
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
		// iniファイルで指定されていれば ここでLNB Power をONする
		// LNB Power のOFFはBDA driverが勝手にやってくれるみたい
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
	// 使ってないし、まぁいいか
	return E_NOINTERFACE;
}

const HRESULT CIT35Specials::ReadIniFile(const WCHAR *szIniFilePath)
{
	CIniFileAccess IniFileAccess(szIniFilePath);
	IniFileAccess.SetSectionName(L"IT35");

	// IF周波数で put_CarrierFrequency() を行う
	m_bRewriteIFFreq = IniFileAccess.ReadKeyB(L"RewriteIFFreq", 0);

	// 固有の Property set を使用して TSID の書込みが必要
	m_bPrivateSetTSID = IniFileAccess.ReadKeyB(L"PrivateSetTSID", 0);

	// LNB電源の供給をONする
	m_bLNBPowerON = IniFileAccess.ReadKeyB(L"LNBPowerON", 0);

	// Dual Mode ISDB Tuner
	m_bDualModeISDB = IniFileAccess.ReadKeyB(L"DualModeISDB", 0);

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

	// Dual Mode ISDB Tunerの場合はTC90532の復調Modeを設定
	if (m_bDualModeISDB && pTuningParm->Modulation->Modulation != m_CurrentModulationType) {
		switch (pTuningParm->Modulation->Modulation) {
		case BDA_MOD_ISDB_T_TMCC:
			hr = it35_DigibestPrivateIoControl(m_pIKsPropertySet, PRIVATE_IO_CTL_FUNC_DEMOD_OFDM);
			break;
		case BDA_MOD_ISDB_S_TMCC:
			hr = it35_DigibestPrivateIoControl(m_pIKsPropertySet, PRIVATE_IO_CTL_FUNC_DEMOD_PSK);
			break;
		}
		m_CurrentModulationType = pTuningParm->Modulation->Modulation;
	}

	// Dual Mode ISDB Tunerの場合はISDB-Sの時のみ
	if (!m_bDualModeISDB || pTuningParm->Modulation->Modulation == BDA_MOD_ISDB_S_TMCC) {
		// IF周波数に変換
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
			if (FAILED(hr = pITuneRequest->get_Locator(&pILocator))) {
				OutputDebug(L"ITuneRequest::get_Locator failed.\n");
				return hr;
			}

			pILocator->put_CarrierFrequency(freq);

			hr = pITuneRequest->put_Locator(pILocator);
		}

		// TSIDをSetする
		if (m_bPrivateSetTSID && pTuningParm->TSID != 0 && pTuningParm->TSID != -1) {
			::EnterCriticalSection(&m_CriticalSection);
			hr = it35_PutISDBIoCtl(m_pIKsPropertySet, (WORD)pTuningParm->TSID);
			::LeaveCriticalSection(&m_CriticalSection);
		}
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
