#pragma once

#include <string>

class CCOMProc {
public:
	typedef void(CALLBACK *RemoconCallbackFunc)(DWORD Data, LPARAM pParam);

private:
	HANDLE hThread;					// �X���b�h�n���h��
	HANDLE hThreadInitComp;			// �X���b�h�����������ʒm
	HANDLE hTerminateRequest;		// �X���b�h�I���v��
	CRITICAL_SECTION csLock;		// �r���p
	std::wstring TunerFriendlyName; // �`���[�i�[���ʗpFriendlyName
	std::wstring TunerInstancePath; // �`���[�i�[���ʗp�f�o�C�X�C���X�^���X�p�X
	DWORD PollingInterval;
	RemoconCallbackFunc CallbackFunc;
	LPARAM CallbackInstance;

public:
	CCOMProc(void);
	~CCOMProc(void);
	void SetTunerFriendlyName(std::wstring friendlyName, std::wstring instancePath);
	void SetPollingInterval(DWORD dwMilliseconds);
	BOOL CreateThread(void);
	void TerminateThread(void);
	void SetCallback(RemoconCallbackFunc pFunc, LPARAM pParam);

private:
	void CloseThreadHandle(void);
	static DWORD WINAPI COMProcThread(LPVOID lpParameter);
};
