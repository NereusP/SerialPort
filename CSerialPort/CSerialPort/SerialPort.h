// SerialPort.h

#ifndef _SERIAL_PORT_H_
#define _SERIAL_PORT_H_

#include <Windows.h>

#define FC_DTRDSR       0x01
#define FC_RTSCTS       0x02
#define FC_XONXOFF      0x04
#define ASCII_BEL       0x07
#define ASCII_BS        0x08
#define ASCII_LF        0x0A
#define ASCII_CR        0x0D
#define ASCII_XON       0x11
#define ASCII_XOFF      0x13

class CSerialPort
{
public:
	CSerialPort();
	~CSerialPort();
	/*Open SerialPort*/
	bool OpenSerialPort(int nPort = 4, int nBaudRate = 9600);
	/*Close SerialPort*/
	bool ClosePort(void);
	/*Read data from SerialPort*/
	DWORD RecvData(void* readbuf, int limit_read_size);
	/*Send data through SerialPort*/
	DWORD SendData(const char* sendbuf, int send_size);
	/*Read data from SerialPort*/
	DWORD RecvAllData(void* readbuf, int limit_read_size);

	bool isOpen(void)
	{
		return m_bOpened;
	}

protected:
	bool WriteCommByte(unsigned char);
	HANDLE m_hComDevID;
	OVERLAPPED m_OverlappedRead, m_OverlappedWrite;
	bool m_bOpened;
};

#endif
