#define _CRT_SECURE_NO_WARNINGS

#include<iostream>
#include<mutex>
#include"MVGevSource.h"
#include"SerialBKServer.h"
#include"DataModel.h"
#include<PvPixelType.h>
#include<queue>
#include<vector>
#include <time.h>

uint cap_num = 0;
std::shared_ptr<mutex> mMutex;
std::shared_ptr<RawDataPack> data_pack;
std::vector<std::vector<ushort>> data_queue;
std::vector<ushort> data_temp;
int CapCallBack(PvImage* pData, void* pUserData);
int recording = 0;//0:no,-1:ready to yes,1:yes
FILE *out;

int main()
{
	printf("HelloWorld!\n");
	char *pszCurrTime = (char*)malloc(sizeof(char) * 30);
	memset(pszCurrTime, 0, sizeof(char) * 30);

	time_t now;
	time(&now);
	strftime(pszCurrTime, 20, "%Y_%m_%d_%H_%M_%S.txt", localtime(&now));
	out = fopen(pszCurrTime, "w");
	free(pszCurrTime);
	std::shared_ptr<CMvGevSource> mCamera;

	mCamera.reset(new CMvGevSource());
	mMutex.reset(new mutex());
	data_pack.reset(new RawDataPack());
	data_pack->avg = 100;
#ifdef CAMERA_MODE_ENABLE

	if (mCamera->Open(NULL, NULL, 0))
	{
		mCamera->ip[2];
		mCamera->SetCallback(CapCallBack, nullptr);
	}
	else
	{
		printf("Camera open failed\n");
		system("pause");
		return 0;
	}
	auto para = mCamera->lDevice->GetParameters();
	para->SetEnumValue("PixelFormat", PvPixelMono12);
	para->SetIntegerValue("Height", DATA_HEIGHT);
	para->SetIntegerValue("Width", DATA_WIDTH);
	para->SetIntegerValue("OffsetX", 0);
	para->SetIntegerValue("OffsetY", 0);
	mCamera->mSerial->SetRegValue(0, 0xff79, DATA_HEIGHT);
	mCamera->mSerial->SetRegValue(0, 0xff80, DATA_WIDTH);
	mCamera->Start(0, 1);
#endif
#ifdef SERVER_MODE_ENABLE
	SerialBKServer server(mCamera, mMutex, data_pack);
	server.start();
#endif
	char command[100] = { 0 };
	while (true)
	{
		memset(command, 0, sizeof(command));
		scanf_s("%s", command, 99);
		if (memcmp(command, "exit", 4) == 0)
			break;
	}
	mCamera->Stop(0, 1);
	fclose(out);
	printf("system ended\n");
	system("pause");
	return 0;
}

int CapCallBack(PvImage* pData, void* pUserData)
{
	cap_num++;
	PBYTE lDataPtr = pData->GetDataPointer();
	if (cap_num % 31 == 0)
	{
		for (int i = 0; i < 12; i++)
			printf("0x%02x ", *(lDataPtr + i));
		printf("\n");
	}
	for (int i = 0; i < DATA_HEIGHT; i++)
	{
		for (int j = 0; j < DATA_WIDTH; j++)
		{
			UCHAR *pt = lDataPtr + i * DATA_WIDTH * 2 + j * 2;
			ushort val = SerialBKServer::ByteToshort(pt);
			if (recording == 1)
			{
				data_temp.push_back(val);
				fprintf(out, "%d ", val);
				mMutex->lock();
				int LEN = data_pack->data_len;
				int AVG = data_pack->avg;
				mMutex->unlock();
				if (data_temp.size() >= LEN)//recording end
				{
					recording = 0;
					fprintf(out, "\n");
					if (data_queue.size() >= MAX_QUEUE_AVG_LEN)//queue is full
					{
						data_queue.erase(data_queue.begin());
					}
					data_queue.push_back(data_temp);
					//re-calculate the average
					if (data_queue.size() >= AVG)//enough data
					{
						bool enough_len = true;
						for (int k = 0; k < AVG; k++)
						{
							if (data_queue[data_queue.size() - k - 1].size() < LEN)
							{
								enough_len = false;
								break;
							}
						}
						if (!enough_len)//Some of the record(s) don't match the data lenth
						{
							mMutex->lock();
							data_pack->data.clear();
							mMutex->unlock();
						}
						else//Finally we have enough data
						{
							std::vector<uint> to_copy;
							for (int k = 0; k < LEN; k++)
							{
								to_copy.push_back(0);
							}
							for (int k = 0; k < AVG; k++)
							{
								for (int kk = 0; kk < LEN; kk++)
								{
									to_copy[kk] += data_queue[data_queue.size() - k - 1][kk];
								}
							}
							mMutex->lock();
							data_pack->data.clear();
							for (int kk = 0; kk < LEN; kk++)
							{
								data_pack->data.push_back((ushort)(to_copy[kk] / AVG));
							}
							mMutex->unlock();
						}
					}
					else//Not enough data
					{
						mMutex->lock();
						data_pack->data.clear();
						mMutex->unlock();
					}
				}
			}
			else
			{
				mMutex->lock();
				ushort WAY = data_pack->compare_way;
				ushort THR = data_pack->threshold;
				mMutex->unlock();
				if (recording == -1)
					recording = 1;
				if (WAY == 0 && val == THR)
					recording = (j % 2 == 0) ? 1 : -1;
				else if (WAY == 1 && val > THR)
					recording = (j % 2 == 0) ? 1 : -1;
				else if (WAY == 2 && val < THR)
					recording = (j % 2 == 0) ? 1 : -1;
			}
		}
	}



	mMutex->lock();
	//uint real_avg = max(min(DATA_WIDTH * DATA_HEIGHT / (RAW_DATA_LENTH / 2), data_pack->avg),1);
	//data_pack->avg = real_avg;
	mMutex->unlock();
	return true;
}