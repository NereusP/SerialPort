#include <cstdio>
#include <process.h>
#include "SerialPort.h"

bool exit_flag = false;

void __stdcall RecvdataThread(void * pParam);

int main(void)
{
	CSerialPort t_serial_port;
	int nPort = 3;
	printf("Please input SerialPort number:\n");
	scanf("%d", &nPort);
	if (t_serial_port.OpenSerialPort(nPort, 115200))
	{
		char sendbuf[20] = { "Hello!001\n" };
		_beginthread((_beginthread_proc_type)(void*)RecvdataThread, 0, &t_serial_port);
		for (int i = 0; i < 256; i++)
		{
			sprintf(sendbuf, "Hello!%03d\n", i);
			t_serial_port.SendData(sendbuf, 12);
			Sleep(10);
		}
	}
	else
	{
		printf("Open SerialPort failed!\n");
	}
	exit_flag = true;
	Sleep(500);
	t_serial_port.ClosePort();
	system("pause");
	return 0;
}

void __stdcall RecvdataThread(void * pParam)
{
	CSerialPort* pSerialPort = (CSerialPort*)pParam;
	char recvbuf[100] = { "\0" };
	DWORD recv_bytes = 0;
	while (!exit_flag)
	{
		memset(recvbuf, 0x00, 100);
		recv_bytes = pSerialPort->RecvAllData(recvbuf, 100);
		//recv_bytes = pSerialPort->RecvData(recvbuf, 100);
		if (0 < recv_bytes)
		{
			printf("read[%d]:%s", recv_bytes, recvbuf);
		}
	}
}
