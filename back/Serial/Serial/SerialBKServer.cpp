#include "SerialBKServer.h"


SerialBKServer::SerialBKServer(shared_ptr<CMvGevSource> camera, shared_ptr<mutex> mtx, shared_ptr<RawDataPack> rdp)
{
	_camera = camera;
	_mtx = mtx;
	_rdp = rdp;
}

SerialBKServer::~SerialBKServer()
{
}

void SerialBKServer::start()
{
	int err;
	///////////////////�����ֿ�////////////////
	WORD wVersionRequested = MAKEWORD(1, 1);
	WSADATA wsaData;
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		cout << "faild when WSAStartup!" << endl;
		system("pause");
		return;
	}
	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
	{
		WSACleanup();
		cout << "faild when WSAStartup!" << endl;
		system("pause");
		return;
	}
	int max_binner = 20;
	listen_socket = socket(AF_INET, SOCK_STREAM, 0);//���������ڶ���������ѡ�������׽��֣�UDPѡ��SOCK_DGRAM���ݱ��׽���
	SOCKADDR_IN addrSrv;
	addrSrv.sin_family = AF_INET;//��������
	addrSrv.sin_port = htons(port);//����������Ķ˿�
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//���������ip��ַɸѡ,��Ϊ�Ƿ�����������ʱlisten�����Բ�����ANY�����Զ�ѡ��Ĭ�ϵı�������������ǿͻ��������ʱconnet����Ҫ��������ΪĿ��ip�� 

	if (::bind(listen_socket, (SOCKADDR*)&addrSrv, sizeof(addrSrv)) == -1)
	{
		cout << "bind() failed." << endl;
		closesocket(listen_socket);//�����ڴ򿪣�ʧ���ˣ�Ӧ���ں����ڹر�
		return;
	}
	if (listen(listen_socket, max_binner) == SOCKET_ERROR)//���Կ�ʼ����,����2�ǵȴ�������󳤶ȣ�Ҳ���Ǽ���ʱ����������
	{
		cout << "Error listening on socket." << endl;
		return;
	}
	cout << "Server Created!" << endl << "Now,Listening``````" << endl;
	listen_thread = new thread(&SerialBKServer::ListenThread, this);
	listen_thread->detach();
	return;
}

void SerialBKServer::ListenThread()
{
	while (1)
	{
		communicate_socket = accept(listen_socket, 0, 0);
		if (communicate_thread != nullptr)
		{
			delete communicate_thread;
		}
		communicate_thread = new thread(&SerialBKServer::CommunicateThread, this, &communicate_socket);
		communicate_thread->detach();
	}
}

void SerialBKServer::CommunicateThread(LPVOID lparam)
{
	SOCKET *s = (SOCKET*)lparam;
	SOCKADDR_IN dest_add;
	int nAddrLen = sizeof(dest_add);
	//ͨ��RCVSocket��öԷ��ĵ�ַ
	if (::getpeername(*s, (SOCKADDR*)&dest_add, &nAddrLen) != 0)
	{
		std::cout << "Get IP address by socket failed!" << endl;
		return;
	}

	cout << "IP: " << ::inet_ntoa(dest_add.sin_addr) <<
		"  PORT: " << ntohs(dest_add.sin_port) << " connected!" << endl;
	while (1)
	{
		//warning:���е�byte��Ϊ��λ��ǰ����byte(8bit)Ϊ��λ��
		//��ע��˳��
		int byte_rev;
		char buffer[DATA_LEN] = { 0 };
		byte_rev = recv(*s, buffer, DATA_LEN, 0);
		if (buffer[3] == 'X')
		{
			printf("[Event] Received:\n");
			printf("%c%c%c%c 0x%08X 0x%08X 0x%08X\n",
				buffer[0], buffer[1], buffer[2], buffer[3],
				ByteToint(buffer + 4), ByteToint(buffer + 8), ByteToint(buffer + 12));
			_mtx->lock();
			char buffer_send[DATA_LEN] = { 0 };
			char buffer_tmp[4] = {0};
			buffer_send[0] = 'R';
			buffer_send[1] = 'E';
			buffer_send[2] = 'T';
			buffer_send[3] = 'X';
			
			unsigned int num3;
			//TODO:ȷ�����ݸ�ʽ��
			int suc;
			uint extra_data_lenth = 0;
			//camera_serial
			if (buffer[0] == 'G' && buffer[1] == 'E' && buffer[2] == 'T')
			{
#ifdef CAMERA_MODE_ENABLE
				suc = _camera->mSerial->GetRegValue(0, ByteToint(buffer + 4), num3);
				char buffer_tmp2[4];
				intToByte(num3, buffer_tmp2);
				//���ߵ�
				buffer_tmp[0] = buffer_tmp2[0];
				buffer_tmp[1] = buffer_tmp2[1];
				buffer_tmp[2] = buffer_tmp2[2];
				buffer_tmp[3] = buffer_tmp2[3];
#endif
			}
			else if (buffer[0] == 'S' && buffer[1] == 'E' && buffer[2] == 'T')
			{
#ifdef CAMERA_MODE_ENABLE
				char buffer_tmp2[4];
				//���ߵ�
				buffer_tmp2[0] = buffer[8 + 0];
				buffer_tmp2[1] = buffer[8 + 1];
				buffer_tmp2[2] = buffer[8 + 2];
				buffer_tmp2[3] = buffer[8 + 3];
				num3 = ByteToint(buffer_tmp2);
				suc = _camera->mSerial->SetRegValue(0, ByteToint(buffer + 4), num3);
				memcpy(buffer_tmp, buffer + 8, 4);
#endif
			}
			//avg
			else if (buffer[0] == 'A' && buffer[1] == 'V' && buffer[2] == 'G')
			{
				suc = 0;
				_rdp->avg = ByteToint(buffer + 4);
			}
			else if (buffer[0] == 'G' && buffer[1] == 'A' && buffer[2] == 'V')
			{
				suc = -1;
				char buffer_tmp2[4];
				intToByte(_rdp->avg, buffer_tmp2);
				memcpy(buffer_send + 4, buffer_tmp2, 4);
			}
			//thr
			else if (buffer[0] == 'T' && buffer[1] == 'H' && buffer[2] == 'R')
			{
				suc = 0;
				_rdp->threshold = ByteToint(buffer + 4);
			}
			else if (buffer[0] == 'G' && buffer[1] == 'T' && buffer[2] == 'H')
			{
				suc = -1;
				char buffer_tmp2[4];
				intToByte(_rdp->threshold, buffer_tmp2);
				memcpy(buffer_send + 4, buffer_tmp2, 4);
			}
			//cmp
			else if (buffer[0] == 'C' && buffer[1] == 'M' && buffer[2] == 'P')
			{
				suc = 0;
				_rdp->compare_way = ByteToint(buffer + 4);
			}
			else if (buffer[0] == 'G' && buffer[1] == 'C' && buffer[2] == 'M')
			{
				suc = -1;
				char buffer_tmp2[4];
				intToByte(_rdp->compare_way, buffer_tmp2);
				memcpy(buffer_send + 4, buffer_tmp2, 4);
			}
			//len
			else if (buffer[0] == 'L' && buffer[1] == 'E' && buffer[2] == 'N')
			{
				suc = 0;
				_rdp->data_len = ByteToint(buffer + 4);
			}
			else if (buffer[0] == 'G' && buffer[1] == 'L' && buffer[2] == 'E')
			{
				suc = -1;
				char buffer_tmp2[4];
				intToByte(_rdp->data_len, buffer_tmp2);
				memcpy(buffer_send + 4, buffer_tmp2, 4);
			}
			//data
			else if (buffer[0] == 'D' && buffer[1] == 'A' && buffer[2] == 'T')
			{
				buffer_send[3] = 'L';
				char buffer_tmp2[4];
				intToByte(_rdp->data.size() * 2, buffer_tmp2);
				memcpy(buffer_send + 4, buffer_tmp2, 4);
				extra_data_lenth = _rdp->data.size();//in short format lenth
			}

			if (suc == 0 && buffer_send[3] == 'X')
			{
				memcpy(buffer_send + 4, buffer + 4, 4);
				memcpy(buffer_send + 8, buffer_tmp, 4);
			}
			_mtx->unlock();
			send(*s, buffer_send, DATA_LEN, 0);
			printf("[Event] Sent:\n");
			printf("%c%c%c%c 0x%08X 0x%08X 0x%08X\n",
				buffer_send[0], buffer_send[1], buffer_send[2], buffer_send[3],
				ByteToint(buffer_send + 4), ByteToint(buffer_send + 8), ByteToint(buffer_send + 12));
			if (extra_data_lenth > 0)
			{
				char *data_to_send_extra = new char[extra_data_lenth * 2];
				_mtx->lock();
				for (int ii = 0; ii < extra_data_lenth; ii++)
				{
					shortToByte(_rdp->data[ii], (UCHAR*)data_to_send_extra + 2 * ii);
				}
				_mtx->unlock();
				send(*s, data_to_send_extra, extra_data_lenth * 2, 0);
				delete data_to_send_extra;
			}
		}
	}
}



void SerialBKServer::intToByte(unsigned int i, char * bytes)
{
	memset(bytes, 0, sizeof(char) *  4);
	bytes[0] = (char)(0xff & i);
	bytes[1] = (char)((0xff00 & i) >> 8);
	bytes[2] = (char)((0xff0000 & i) >> 16);
	bytes[3] = (char)((0xff000000 & i) >> 24);
	return;
}

unsigned int SerialBKServer::ByteToint(char * bytes)
{
	unsigned int i = 0;
	i += (unsigned int)(bytes[0]) & 0xff;
	i += ((unsigned int)(bytes[1]) << 8) & 0xff00;
	i += ((unsigned int)(bytes[2]) << 16) & 0xff0000;
	i += ((unsigned int)(bytes[3]) << 24) & 0xff000000;
	return i;
}

ushort SerialBKServer::ByteToshort(unsigned char * bytes)
{
	ushort i = 0;
	i += (ushort)(bytes[0]) & 0xff;
	i += ((ushort)(bytes[1]) << 8) & 0xff00;
	return i;
}

void SerialBKServer::shortToByte(ushort i, unsigned char * bytes)
{
	memset(bytes, 0, sizeof(unsigned char) * 2);
	bytes[0] = (unsigned char)(0xff & i);
	bytes[1] = (unsigned char)((0xff00 & i) >> 8);
	return;
}


