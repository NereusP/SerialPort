// SerialPortPort.cpp

#include "SerialPort.h"

CSerialPort::CSerialPort():m_hComDevID(NULL), m_bOpened(false)
{
	memset(&m_OverlappedRead, 0, sizeof(OVERLAPPED));
 	memset(&m_OverlappedWrite, 0, sizeof(OVERLAPPED));
}

CSerialPort::~CSerialPort()
{
	ClosePort();
}

bool CSerialPort::OpenSerialPort(int nPort /*= 4*/, int nBaudRate /*= 9600*/)
{
	if (m_bOpened)
	{
		return true;
	}

	WCHAR szPort[15] = TEXT("\0");
	WCHAR szComParams[50] = TEXT("\0");
	DCB dcb;

	wsprintf(szPort, TEXT("COM%d"), nPort);
	m_hComDevID = CreateFile(szPort, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);
	if (m_hComDevID == NULL)
	{
		return false;
	}

	memset(&m_OverlappedRead, 0, sizeof(OVERLAPPED));
 	memset(&m_OverlappedWrite, 0, sizeof(OVERLAPPED));

	COMMTIMEOUTS CommTimeOuts;
	CommTimeOuts.ReadIntervalTimeout = 0xFFFFFFFF;
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;
	CommTimeOuts.ReadTotalTimeoutConstant = 0;
	CommTimeOuts.WriteTotalTimeoutMultiplier = 0;
	CommTimeOuts.WriteTotalTimeoutConstant = 5000;
	SetCommTimeouts(m_hComDevID, &CommTimeOuts);

	wsprintf(szComParams, TEXT("COM%d:%d,n,8,1"), nPort, nBaudRate);

	m_OverlappedRead.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	m_OverlappedWrite.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

	dcb.DCBlength = sizeof(DCB);
	GetCommState(m_hComDevID, &dcb);
	dcb.BaudRate = nBaudRate;
	dcb.ByteSize = 8;
	unsigned char ucSet;
	ucSet = (unsigned char) ((FC_RTSCTS & FC_DTRDSR) != 0);
	ucSet = (unsigned char) ((FC_RTSCTS & FC_RTSCTS) != 0);
	ucSet = (unsigned char) ((FC_RTSCTS & FC_XONXOFF) != 0);
	if (FALSE == SetCommState(m_hComDevID, &dcb) || !SetupComm(m_hComDevID, 10000, 10000) || m_OverlappedRead.hEvent == nullptr || m_OverlappedWrite.hEvent == nullptr)
	{
		DWORD dwError = GetLastError();
		if (m_OverlappedRead.hEvent != NULL)
		{
			CloseHandle(m_OverlappedRead.hEvent);
		}
		if (m_OverlappedWrite.hEvent != NULL)
		{
			CloseHandle(m_OverlappedWrite.hEvent);
		}
		CloseHandle(m_hComDevID);
		m_bOpened = false;
	}
	else
	{
		m_bOpened = true;
	}
	return m_bOpened;
}

bool CSerialPort::ClosePort(void)
{
	if (!m_bOpened || m_hComDevID == NULL)
	{
		return true;
	}

	if (m_OverlappedRead.hEvent != NULL)
	{
		CloseHandle(m_OverlappedRead.hEvent);
	}
	if (m_OverlappedWrite.hEvent != NULL)
	{
		CloseHandle(m_OverlappedWrite.hEvent);
	}
	CloseHandle(m_hComDevID);
	m_bOpened = FALSE;
	m_hComDevID = NULL;

	return true;
}

bool CSerialPort::WriteCommByte(unsigned char ucByte)
{
	BOOL bWriteStat = FALSE;
	DWORD dwBytesWritten = 0;

	bWriteStat = WriteFile(m_hComDevID, (LPSTR)&ucByte, 1, &dwBytesWritten, &m_OverlappedWrite);
	if(!bWriteStat && (GetLastError() == ERROR_IO_PENDING))
	{
		if (WaitForSingleObject(m_OverlappedWrite.hEvent, 1000))
		{
			dwBytesWritten = 0;
		}
		else
		{
			GetOverlappedResult(m_hComDevID, &m_OverlappedWrite, &dwBytesWritten, FALSE);
			m_OverlappedWrite.Offset += dwBytesWritten;
		}
	}
	return true;
}

DWORD CSerialPort::SendData(const char *buffer, int size)
{

	if(!m_bOpened || m_hComDevID == nullptr) return 0;

	DWORD dwBytesWritten = 0;
	for(int i=0; i<size; i++)
	{
		WriteCommByte(buffer[i]);
		dwBytesWritten++;
	}
	return dwBytesWritten;
}

DWORD CSerialPort::RecvAllData(void* readbuf, int limit_read_size)
{
	DWORD dwTotalReadBytes = 0;
	if (!m_bOpened || m_hComDevID == NULL)
	{
		return 0;
	}

	DWORD dwErrorFlags = 0x00, dwBytesToRead = 0x00, dwBytesRead = 0x00;
	BOOL bReadStatus = FALSE;
	COMSTAT ComStat;
	char* t_readbuf = (char*)readbuf;
	ClearCommError(m_hComDevID, &dwErrorFlags, &ComStat);
	while ((ComStat.cbInQue > 0) && (dwTotalReadBytes < limit_read_size))
	{
		dwBytesToRead = ComStat.cbInQue;
		if ((limit_read_size - dwTotalReadBytes) < dwBytesToRead)
		{
			dwBytesToRead = (DWORD)(limit_read_size - dwTotalReadBytes);
		}
		bReadStatus = ReadFile(m_hComDevID, t_readbuf, dwBytesToRead, &dwBytesRead, &m_OverlappedRead);
		if (FALSE == bReadStatus)
		{
			if (GetLastError() == ERROR_IO_PENDING)
			{
				DWORD dwWaitRes = WaitForSingleObject(m_OverlappedRead.hEvent, 100);
				if (dwWaitRes == WAIT_OBJECT_0)
				{
					dwTotalReadBytes += dwBytesRead;
					t_readbuf += dwBytesRead;
				}
				else
				{
					CancelIo(m_hComDevID);
					dwBytesRead = 0;
				}
			}
		}
		else
		{
			dwTotalReadBytes += dwBytesRead;
			t_readbuf += dwBytesRead;
		}
		ClearCommError(m_hComDevID, &dwErrorFlags, &ComStat);
	}
	return dwTotalReadBytes;
}

DWORD CSerialPort::RecvData(void *buffer, int limit_read_size)
{
	if (!m_bOpened || m_hComDevID == NULL)
	{
		return 0;
	}

	BOOL bReadStatus = FALSE;
	DWORD dwBytesToRead = 0, dwBytesRead = 0, dwErrorFlags = 0;
	COMSTAT ComStat;

	ClearCommError(m_hComDevID, &dwErrorFlags, &ComStat);
	if (0 == ComStat.cbInQue)
	{
		return 0;
	}

	dwBytesToRead = (DWORD) ComStat.cbInQue;
	if (limit_read_size < (int)dwBytesToRead)
	{
		dwBytesToRead = (DWORD)limit_read_size;
	}
	bReadStatus = ReadFile(m_hComDevID, buffer, dwBytesToRead, &dwBytesRead, &m_OverlappedRead);
	if (FALSE == bReadStatus)
	{
		DWORD dwWaitRes = WaitForSingleObject(m_OverlappedRead.hEvent, 100);
		if (dwWaitRes != WAIT_OBJECT_0)
		{
			CancelIo(m_hComDevID);
			dwBytesRead = 0;
		}
	}
	return dwBytesRead;
}
