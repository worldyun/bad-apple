// badApple.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <mutex>
#include <thread>
#include"audio_clip.h"
#include <imgproc.hpp>
#include <iostream>
#include<opencv2\core\core.hpp>
#include<opencv2\highgui\highgui.hpp>
#include <cstdlib>
#include <windows.h>


using namespace cv;
using namespace std;
mutex mtxFrame;
mutex mtxOutBinaryImage;


void appGotoXY(int x, int y)
{
	CONSOLE_SCREEN_BUFFER_INFO    csbiInfo;
	HANDLE    hConsoleOut;
	hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(hConsoleOut, &csbiInfo);
	csbiInfo.dwCursorPosition.X = x;
	csbiInfo.dwCursorPosition.Y = y;
	SetConsoleCursorPosition(hConsoleOut, csbiInfo.dwCursorPosition);
}

void p_printf(int startF,int numOfThreads , Mat & outBinaryImage, int& fps, bool &isPlay, mutex &mtx) {
	int myFps = 0;
	int lfps[3] = { 0 };
	DWORD ltime = GetTickCount();
	DWORD sTime = GetTickCount();
	char ss[10773] = "";
	while (isPlay)
	{	
		if( (GetTickCount() - ltime) > 1000 )
		{
			mtx.lock();
			appGotoXY( 175 ,63 - startF);
			printf_s(" 线程%d     控制台刷新率:  %d       FPS:  %d      ", startF, (myFps - lfps[0]), (lfps[2] - lfps[1]));
			appGotoXY(2, 1);
			mtx.unlock();
			ltime = GetTickCount();
			lfps[0] = myFps;
			lfps[1] = lfps[2];
		}
		if ( !outBinaryImage.empty())
		{ 
			if(fps % numOfThreads == startF  && ( lfps[2] * numOfThreads) < fps)
			{ 
				lfps[2]++;
				ss[0] = 0;
				for (int i = 0; i < outBinaryImage.rows; i += 1) {
					for (int j = 0; j < outBinaryImage.cols; j++) {
						if (0 == outBinaryImage.at<uchar>(i, j)) {
							strcat(ss, " ");
						}
						else {
							strcat(ss, "*");
						}
					}
					strcat(ss, "\n  ");
				}
				strcat(ss, "\0");
				sTime = GetTickCount();
				mtx.lock();
				while ((GetTickCount() - sTime) < 13)
				{
					appGotoXY(2, 1);
					printf_s("%s", ss);
					appGotoXY(2, 1);
					myFps++;
				}
				mtx.unlock();
				Sleep((numOfThreads - 1) * 13);
			}
		}
	}
}

void p_play(int startF, int numOfThreads, Mat &frame, Mat &outBinaryImage, int &fps, bool& isPlay) {
	int myFps = 0;
	Mat gray, binaryImage;
	while (isPlay)
	{
		if (!frame.empty() && ( myFps * numOfThreads) < fps && (fps % numOfThreads == startF))
		{
			mtxFrame.lock();
			cvtColor(frame, gray, COLOR_RGB2GRAY);					//灰度图
			mtxFrame.unlock();
			threshold(gray, binaryImage, 100, 255, THRESH_BINARY);  //二值化
			mtxOutBinaryImage.lock();
			resize(binaryImage, outBinaryImage, Size(168, 63));		//缩小
			imshow("二值化", binaryImage);
			mtxOutBinaryImage.unlock();
			myFps++;
			Sleep((numOfThreads - 1) * 13);
		}
	}
}

class video
{
public:
	bool _play = true;		//播放控制
	VideoCapture capture;	
	Mat frame, gray, outBinaryImage;
	double frame_rate;		//fps
	int delat;				//帧间时延
	int fps = 0;
	mutex mtx;
	bool isPlay = false;

	video(string adr);
	void play();
};

video::video(string adr)
{
	capture.open(adr);
	if (!capture.isOpened()) {
		cout << "视频没有打开" << endl;
	}
	namedWindow("原视频");
	namedWindow("二值化");
	frame_rate = 30.0;
	delat = 1000 / frame_rate;
}

void video::play() {
	isPlay = true;
	int numOfThreads = 3;

	for(int i = 0; i < numOfThreads; i++){
		thread(p_play, i, numOfThreads, ref(frame), ref(outBinaryImage), ref(fps), ref(isPlay)).detach();
		thread(p_printf, i, numOfThreads, ref(outBinaryImage), ref(fps), ref(isPlay), ref(mtx)).detach();
	}

	LARGE_INTEGER freq_;
	QueryPerformanceFrequency(&freq_);
	LARGE_INTEGER beginTime;
	QueryPerformanceCounter(&beginTime);
	LARGE_INTEGER nowTime;
	long long nsTime;
	int nfps;
	system("cls");

	while (_play) {
		mtxFrame.lock();
		capture >> frame;
		mtxFrame.unlock();
		if (!frame.empty()) {
			imshow("原视频", frame);
		}
		else {
			isPlay = false;
			break;
		}
		
		QueryPerformanceCounter(&nowTime);
		nsTime = (long long)(nowTime.QuadPart - beginTime.QuadPart) * 1000000.0 / freq_.QuadPart;
		nfps = nsTime / (1000.0 * 1000.0 ) * 60.0;
		if ( nfps - fps > 0) {
			mtxFrame.lock();
			for (int i = 0; i < nfps - fps; i++)
			{
				capture >> frame;
				fps++;
			}
			mtxFrame.unlock();
		}
		else if (nfps - fps < 0)
		{
			waitKey(-(int)((nfps - fps) * 15));
		}
		waitKey(15);
		fps++;
	}
}


int main()
{
	AudioClip audio;
	audio.load("C:\\Users\\wordy\\Desktop\\BadApple.mp3");
	audio.play();
	waitKey(700);
	video badapple("C:\\Users\\wordy\\Desktop\\BadApple.mp4");
	badapple.play();

	return 0;
}
