// sever.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include<iostream>
#include<string.h>
#include<winsock.h>
#include<windows.h>
#include <mmintrin.h> //MMX
#include <xmmintrin.h> //SSE(include mmintrin.h)
#include <emmintrin.h> //SSE2(include xmmintrin.h)
#include <pmmintrin.h> //SSE3(include emmintrin.h)
#include <tmmintrin.h>//SSSE3(include pmmintrin.h)
#include <smmintrin.h>//SSE4.1(include tmmintrin.h)
#include <nmmintrin.h>//SSE4.2(include smmintrin.h)
#include <wmmintrin.h>//AES(include nmmintrin.h)
#include <immintrin.h>//AVX(include wmmintrin.h)
#include <intrin.h>//(include immintrin.h)

#define MAX_THREADS 64
#define SUBDATANUM 200000
#define DATANUM (SUBDATANUM * MAX_THREADS)   /*这个数值是总数据量*/
#define TREAD 8          //实际排序线程数
#define DATANUM1 ((DATANUM/TREAD)/2)    //每线程数据数

/***********全局变量定义***********/
double rawdoubleData[DATANUM];
double rawFloatData[DATANUM/2];   //输入数据
double rawFloatData_result[DATANUM/2];    //输出结果
int ThreadID[TREAD];    //线程
double floatResults1[TREAD][DATANUM1];//每个线程的中间结果
double tdata[TREAD][DATANUM1];//每个线程的中间结果，数据临时存储，便于调用函数
/********************************/

using namespace std;
#pragma comment(lib,"ws2_32.lib")
using namespace std;

/*********函数声明********/
void initialization();

/*******SUM*******/
double sum(const double data[], const int len);//不加速计算
double sumSpeedUp(const double data[], const int len);//sum使用avx和openmp
double sumSpeedUp1(const double data[], const int len);//sum单独使用openmp，实际使用算法

/*******MAX******/
double mymax(const double data[], const int len);//不加速计算，data是原始数据，len为长度。结果通过函数返回
double maxSpeedUp1(const double data[], const int len);//单独使用openmp
double maxSpeedUp(const double data[], const int len);//使用openmp和SSE会使速度极度变慢，注释openmp单独使用sse，为实际使用算法

/******SORT*****/
void init_sortdata(const double data[], const int len, double result[]);//排序数据使用SSE预处理
double sort(const double data[], const int len, double result[]);//慢排；data是原始数据，len为长度。排序结果在result中。
double sortSpeedUp(const double data[], const int len, double result[]);//快速排序；data是原始数据，len为长度。排序结果在result中。
void tQsort(double arr[], int low, int high);    //快速排序算法
int tmin(const double data[], const int len);    //返回最小值位置
void tMerge(double arr[][DATANUM1], double result[]);   //归并有序数列
/***********************/

/*******线程函数********/
DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	//while (1)
	{
		int who = *(int*)lpParameter;
		int startIndex = who * DATANUM1;
		int endIndex = startIndex + DATANUM1;
		//调用函数复制数据
		memcpy(tdata[who], rawFloatData + startIndex, DATANUM1 * sizeof(double));
		sort(tdata[who], DATANUM1, floatResults1[who]);
	}
	return 0;
}
/*************************/

int main() {
	//定义长度变量
	int send_len = 0;
	int recv_len = 0;
	int len = 0;
	double result=0.0f;
	//定义发送缓冲区和接受缓冲区
	char send_buf[100];
	char recv_buf[100];

	/******************重现时修改输入数据*********************/
	for (size_t i = 0; i < DATANUM; i++)//数据初始化
	{
		rawdoubleData[i] = double((rand() + rand() + rand() + rand()));//rand返回short数据类型，增加随机性
	}

	init_sortdata(rawdoubleData, DATANUM, rawFloatData);
	/*********************************************************/

	//定义服务端套接字，接受请求套接字
	SOCKET s_server;
	SOCKET s_accept;
	//服务端地址客户端地址
	SOCKADDR_IN server_addr;
	SOCKADDR_IN accept_addr;
	initialization();
	//填充服务端信息
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(5010);
	//创建套接字
	s_server = socket(AF_INET, SOCK_STREAM, 0);
	if (bind(s_server, (SOCKADDR*)&server_addr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		cout << "套接字绑定失败！" << endl;
		WSACleanup();
	}
	else {
		cout << "套接字绑定成功！" << endl;
	}
	//设置套接字为监听状态
	if (listen(s_server, SOMAXCONN) < 0) {
		cout << "设置监听状态失败！" << endl;
		WSACleanup();
	}
	else {
		cout << "设置监听状态成功！" << endl;
	}
	cout << "服务端正在监听连接，请稍候...." << endl;
	//接受连接请求
	len = sizeof(SOCKADDR);
	s_accept = accept(s_server, (SOCKADDR*)&accept_addr, &len);

	if (s_accept == SOCKET_ERROR) {
		cout << "连接失败！" << endl;
		WSACleanup();
		return 0;
	}
	cout << "连接建立，准备接受数据" << endl;
	//接收数据

	/*************SUM*************/
	recv_len = recv(s_accept, recv_buf, 100, 0);
	if (recv_len < 0) 
	{
		cout << "接受失败！" << endl;
		
	}
	else
	{
		if (recv_buf[0] == '1')
		{
			cout << "开始计算SUM" << endl;
			result = sumSpeedUp1(rawdoubleData, DATANUM);
			cout << "sever端结果为" << result << endl;
		}
		char arr[20];//小了内存会报错，不能设置为sizeof（double）
		sprintf_s(arr, "%f", result);
		send_len = send(s_accept, arr, sizeof(double), 0);
		if (send_len < 0) {
			cout << "发送失败！" << endl;
		}
		/***********MAX**********/
		recv_len = recv(s_accept, recv_buf, 100, 0);
		if (recv_len < 0) {
			cout << "接受失败！" << endl;

		}
		else {
			if (recv_buf[0] == '2')
			{
				cout << "开始计算MAX" << endl;
				result = maxSpeedUp(rawdoubleData, DATANUM);
				cout << "sever端结果为" << result << endl;
			}
			char arr[20];//小了内存会报错，不能设置为sizeof（double）
			sprintf_s(arr, "%f", result);
			send_len = send(s_accept, arr, sizeof(double), 0);
			if (send_len < 0)
			{
				cout << "发送失败！" << endl;
			}
		}

		/**************SORT***************/
		recv_len = recv(s_accept, recv_buf, 100, 0);
		if (recv_len < 0) {
			cout << "接受失败！" << endl;

		}
		else
		{
			if (recv_buf[0] == '3')
			{
				cout << "开始计算SORT" << endl;
				sortSpeedUp(rawFloatData, DATANUM / 2, rawFloatData_result);
			}
			if (send_len < 0)
			{
				cout << "发送失败！" << endl;
			}
			char* psend;
			psend = (char*)&rawFloatData_result[0];
			send_len = send(s_accept, psend, sizeof(double) * DATANUM / 2, 0);
			if (send_len < 0)
			{
				cout << "发送失败！" << endl;
			}
		}
	}
	//关闭套接字
	closesocket(s_server);
	closesocket(s_accept);
	//释放DLL资源
	WSACleanup();
	cout << rawFloatData_result[DATANUM/2-1];
	return 0;
}

void initialization() 
{
	//初始化套接字库
	WORD w_req = MAKEWORD(2, 2);//版本号
	WSADATA wsadata;
	int err;
	err = WSAStartup(w_req, &wsadata);
	if (err != 0) {
		cout << "初始化套接字库失败！" << endl;
	}
	else {
		cout << "初始化套接字库成功！" << endl;
	}
	//检测版本号
	if (LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wHighVersion) != 2) {
		cout << "套接字库版本号不符！" << endl;
		WSACleanup();
	}
	else {
		cout << "套接字库版本正确！" << endl;
	}
	//填充服务端地址信息

}

/**************SUM**************/
//未加速计算
double sum(const double data[], const int len)
{
	double result = 0.0f;
	for (int i = 0; i < len; i++)
	{
		result += log(sqrt(data[i] / 4.0));
	}
	return result;

}
//加速 openmp&SSE
double sumSpeedUp(const double data[], const int len) //sum使用avx和openmp
{
	double s[4] = { 0.25,0.25,0.25,0.25 };
	__m256d s_256 = _mm256_load_pd(&s[0]);
	double temp[4];
	double result = 0;
	__m256d sum = _mm256_set1_pd(0.0f);        // init partial sums
#pragma omp parallel for reduction (+:result)
	for (int i = 0; i < len; i = i + 4)
	{
		__m256d va = _mm256_load_pd(&data[i]);
		_mm256_store_pd(temp, _mm256_log_pd(_mm256_sqrt_pd(_mm256_mul_pd(va, s_256))));
		double resulttemp = temp[0] + temp[1] + temp[2] + temp[3];
		result += resulttemp;
	}
	return result;
}
//sum加速使用sumSpeedUp1(openmp),效果比sse+openmp（sumspeedup）好
double sumSpeedUp1(const double data[], const int len) //sum单独使用openmp
{
	double result = 0.0f;
#pragma omp parallel for reduction(+:result)
	for (int i = len / 2; i < len; i++)
	{
		result += log(sqrt(data[i] / 4.0));
	}
	return result;

}

/**************MAX***************/
//max加速使用maxspeedup（SSE），使用openmp会使速度大幅变慢
double mymax(const double data[], const int len) //data是原始数据，len为长度。结果通过函数返回
{
	double result = 0.0f;
	for (int i = 0; i < len; i++)
	{
		double a = log(sqrt(data[i] / 4.0));
		if (result < a)
			result = a;
	}
	//cout << result << endl;
	return result;

}

double maxSpeedUp1(const double data[], const int len) //单独使用openmp
{
	double result = 0.0f;
#pragma omp parallel for  
	for (int i = 0; i < len; i++)
	{
		int a = log(sqrt(data[i] / 4.0));
#pragma omp critical
		{if (result < a)
			result = a; }

	}
	//cout << result << endl;
	return result;

}

double maxSpeedUp(const double data[], const int len) //使用openmp会使速度极度变慢
{
	double s[4] = { 0.25,0.25,0.25,0.25 };
	__m256d s_256 = _mm256_load_pd(&s[0]);
	double result1 = 0.0f, result2 = 0.0f, result = 0.0f;
	cout << "///////////////////////////" << endl;
	int i;
	double temp[4];
	__m256d max = _mm256_set1_pd(0.0f);        // init partial sums
//#pragma omp parallel for
	for (i = len / 2; i < len; i = i + 4)
	{
		__m256d va = _mm256_load_pd(&data[i]);
		//#pragma omp critical
		max = _mm256_max_pd(_mm256_log_pd(_mm256_sqrt_pd(_mm256_mul_pd(va, s_256))), max);
	}
	_mm256_store_pd(temp, max);
	result1 = temp[0] > temp[1] ? temp[0] : temp[1];
	result2 = temp[2] > temp[3] ? temp[2] : temp[3];
	result = result1 > result2 ? result1 : result2;
	return result;
}

/************排序*************/
void init_sortdata(const double data[], const int len, double result[])//sort数据初始化SSE
{
	double s[4] = { 0.25,0.25,0.25,0.25 };
	__m256d s_256 = _mm256_load_pd(&s[0]);
	for (size_t i = len / 2; i < len; i = i + 4)
	{
		__m256d va = _mm256_load_pd(&data[i]);
		_mm256_store_pd(&result[i - len / 2], _mm256_log_pd(_mm256_sqrt_pd(_mm256_mul_pd(va, s_256))));
	}
}

double sort(const double data[], const int len, double result[])
{
	memcpy(result, data, len * sizeof(double));
	tQsort(result, 0, len - 1);
	return 1;
}

double sortSpeedUp(const double data[], const int len, double result[])
{
	HANDLE hThreads[TREAD];
	//多线程
	for (int i = 0; i < TREAD; i++)
	{
		ThreadID[i] = i;

		hThreads[i] = CreateThread(
			NULL,// default security attributes
			0,// use default stack size
			ThreadProc,// thread function
			&ThreadID[i],// argument to thread function
			CREATE_SUSPENDED, // use default creation flags.0 means the thread will be run at once  CREATE_SUSPENDED
			NULL);
	}
	//多线程
	for (int i = 0; i < TREAD; i++)
	{
		ResumeThread(hThreads[i]);
	}
	WaitForMultipleObjects(TREAD, hThreads, TRUE, INFINITE); //这样传给回调函数的参数不用定位static或者new出来的了
	tMerge(floatResults1, rawFloatData_result);

  // Close all thread handles upon completion.
	for (int i = 0; i < TREAD; i++)
	{
		CloseHandle(hThreads[i]);
	}
	return 1;
}

void tQsort(double arr[], int low, int high) {
	if (high <= low) return;
	int i = low;
	int j = high + 1;
	double key = arr[low];
	while (true)
	{
		/*从左向右找比key大的值*/
		while (arr[++i] < key)
		{
			if (i == high) {
				break;
			}
		}
		/*从右向左找比key小的值*/
		while (arr[--j] > key)
		{
			if (j == low) {
				break;
			}
		}
		if (i >= j) break;
		/*交换i,j对应的值*/
		double temp = arr[i];
		arr[i] = arr[j];
		arr[j] = temp;
	}
	/*中枢值与j对应值交换*/
	double temp = arr[low];
	arr[low] = arr[j];
	arr[j] = temp;
	tQsort(arr, low, j - 1);
	tQsort(arr, j + 1, high);
}

//合并有序数列
//堆排序思想
void tMerge(double arr[][DATANUM1], double result[])
{
	double temp[TREAD];
	int min_index;
	int tnum[TREAD] = { 0 };

	for (size_t i = 0; i < TREAD; i++)
	{
		temp[i] = arr[i][tnum[i]];
	}
	for (size_t i = 0; i < DATANUM/2; i++)
	{
		min_index = tmin(temp, TREAD);
		result[i] = temp[min_index];
		if (tnum[min_index] == (DATANUM1 - 1) || tnum[min_index] > (DATANUM1 - 1))
		{
			temp[min_index] = INT_MAX;
		}
		else
			temp[min_index] = arr[min_index][(++tnum[min_index])];
	}

}

int tmin(const double data[], const int len)
{
	double min = data[0];
	int min_index = 0;
	for (size_t i = 0; i < len; i++)
	{
		if (min > data[i])
		{
			min = data[i];
			min_index = i;
		}
	}
	return min_index;
}
