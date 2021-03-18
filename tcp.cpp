// client.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include<iostream>
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
#define TREAD 8          //实际线程数(排序)
#define DATANUM1 ((DATANUM/TREAD)/2)    //每线程数据数

/***********全局变量定义***********/
double rawdoubleData[DATANUM];//生成初始随机数
double rawdoubleData1[DATANUM];//计算出全部数的log(sqrt())结果，等待单机排序算法调用
double rawFloatData[DATANUM/2];   //排序输入数据
double rawFloatData_result[DATANUM/2];    //输出结果
int ThreadID[TREAD];    //线程
double floatResults1[TREAD][DATANUM1];//每个线程的中间结果
double tdata[TREAD][DATANUM1];//每个线程的中间结果，数据临时存储，便于调用函数
double finalsort_result[DATANUM];//最终排序结果
double finalsort_result1[DATANUM];//最终排序结果
double sendsort[DATANUM/2];//服务端排序结果
int counte = 0;//计算传输错误数
/****************IP地址在此输入****************/
char IP[] = "127.0.0.1";


using namespace std;
#pragma comment(lib,"ws2_32.lib")
using namespace std;

/*********函数声明********/
void initialization();//套接字库初始化

/**SUM**/
double sum(const double data[], const int len);//不加速计算
double sumSpeedUp(const double data[], const int len); //sum使用avx和openmp
double sumSpeedUp1(const double data[], const int len);//sum单独使用openmp，实际使用算法

/**MAX**/
double mymax(const double data[], const int len);//不加速计算，data是原始数据，len为长度。结果通过函数返回
double maxSpeedUp1(const double data[], const int len);//单独使用openmp
double maxSpeedUp(const double data[], const int len);//单独使用sse，为实际使用算法，也可将注释部分消去注释以实现openmp+sse但速度会变慢

/**SORT**/
void init_sortdata(const double data[], const int len, double result[]);//排序数据使用SSE预处理
double sort(const double data[], const int len, double result[]);//慢排；data是原始数据，len为长度。排序结果在result中。
double sortSpeedUp(const double data[], const int len, double result[]);//快速排序；data是原始数据，len为长度。排序结果在result中。
void tQsort(double arr[], int low, int high);    //快速排序算法
int tmin(const double data[], const int len);    //返回最小值位置
void tMerge(double arr[][DATANUM1], double result[]);   //归并有序数列
void MemeryArray(double a[], int n, double b[], int m, double c[]);	//归并两个有序数列
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

	/**************重现时修改数据****************/
	//初始化数据
	for (size_t i = 0; i < DATANUM; i++)//数据初始化
	{
		rawdoubleData[i] = double((rand() + rand() + rand() + rand()));//rand返回short数据类型，增加随机性
		rawdoubleData1[i] = log(sqrt(rawdoubleData[i]/4));
	}
	init_sortdata(rawdoubleData, DATANUM, rawFloatData);
	/*******************************************/

	double time1, time2, time3, time4, time5, time6;//记录消耗时间
	LARGE_INTEGER start, end;
	LARGE_INTEGER start1, end1;
	double sumresult2, maxresult2;
	double sumresult, maxresult;
	double r_c=0.0f, r_s=0.0f;
	int send_len = 0;
	int recv_len = 0;
	//定义发送缓冲区和接受缓冲区
	char send_buf[100];
	char recv_buf[100];
	//定义服务端套接字，接受请求套接字
	SOCKET s_server;
	//服务端地址客户端地址
	SOCKADDR_IN server_addr;
	initialization();
	//填充服务端信息
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.S_un.S_addr = inet_addr(IP);
	server_addr.sin_port = htons(5010);
	//创建套接字
	s_server = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(s_server, (SOCKADDR*)&server_addr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
		cout << "服务器连接失败！" << endl;
		WSACleanup();
	}
	else {
		cout << "服务器连接成功！" << endl;


		/**********************SUM**********************/
		cout << "输入1以开始sum:";
		cin >> send_buf;
		QueryPerformanceCounter(&start);//start  
		send_len = send(s_server, send_buf, 100, 0);
		if (send_len < 0) {
			cout << "发送失败！" << endl;
		}
		else
		{
			cout << "正在计算..." << endl;
			r_c = sumSpeedUp1(rawdoubleData, DATANUM);
			cout << "cilent端结果为" << r_c << endl;
		}
		//接收服务端结果
		char sever_result[sizeof(double) + 1];
		recv(s_server, sever_result, 9, 0);
		r_s = atof(sever_result);
		if (recv_len < 0) {
			cout << "接受失败！" << endl;
		}
		else {
			cout << "服务端结果:" << r_s << endl;
		
		}
		QueryPerformanceCounter(&end);//end
		time1 = (end.QuadPart - start.QuadPart);
		sumresult2 = r_s + r_c;


		/*******************MAX********************/
		cout << "输入2以开始max:";
		cin >> send_buf;
		QueryPerformanceCounter(&start);//start  
		send_len = send(s_server, send_buf, 100, 0);
		if (send_len < 0) {
			cout << "发送失败！" << endl;
		}
		else
		{
			cout << "正在计算..." << endl;
			r_c = maxSpeedUp(rawdoubleData, DATANUM);
			cout << "cilent端结果为" << r_c << endl;
		}
		//接收服务端结果
		recv(s_server, sever_result, 9, 0);
		r_s = atof(sever_result);
		if (recv_len < 0) {
			cout << "接受失败！" << endl;
		}
		else {
			cout << "服务端结果:" << r_s << endl;
		}
		maxresult2 = r_c > r_s ? r_c : r_s;
		QueryPerformanceCounter(&end);//end
		time3 = (end.QuadPart - start.QuadPart);


		/***********************SORT*************************/
		cout << "输入3以开始sort:";
		cin >> send_buf;
		QueryPerformanceCounter(&start);//start  
		send_len = send(s_server, send_buf, 100, 0);
		if (send_len < 0) {
			cout << "发送失败！" << endl;
		}
		else
		{
			cout << "正在计算..." << endl;
			sortSpeedUp(rawFloatData, DATANUM/2, rawFloatData_result);
		}
		QueryPerformanceCounter(&start1);//start  
		//接收服务端结果
		char* prenv;
		prenv = (char*)&sendsort[0];
		recv(s_server, prenv, sizeof(double)*DATANUM/2, 0);
		if (recv_len < 0) {
			cout << "接受失败！" << endl;
		}

		//监测是否有传输错误的数据
		//for (size_t i = 0; i < DATANUM/2; i++)
		//{
		//	if (sendsort[i] > 6 || sendsort[i] < 1)
		//	{
		//		//cout << "ERROR! 传输数据监测错误" << endl;
		//		sendsort[i] = 4.5;
		//		counte += 1;
		//	}
		//}

		QueryPerformanceCounter(&end1);//end
		MemeryArray(rawFloatData_result, DATANUM, sendsort, DATANUM, finalsort_result);
		QueryPerformanceCounter(&end);//end
		time5 = (end.QuadPart - start.QuadPart);
	}

	//单机程序SUM
	QueryPerformanceCounter(&start);//start  
	sumresult = sum(rawdoubleData, DATANUM);
	QueryPerformanceCounter(&end);//end
	time2 = (end.QuadPart - start.QuadPart);
	cout << "////////////////////////////SUM/////////////////////////" << endl;
	cout << "双机加速结果为：" << sumresult2 <<"         ";
	cout << "双机用时" << time1 << endl;
	cout << "单机结果为：" << sumresult << "         ";
	cout << "单机用时" << time2 << endl;
	cout << "加速比为" << time2 / time1 << endl;
	
	//单机程序MAX
	QueryPerformanceCounter(&start);//start  
	maxresult = mymax(rawdoubleData, DATANUM);//你的函数(...);//包括任务启动，结果回传，收集和综合
	QueryPerformanceCounter(&end);//end
	time4 = (end.QuadPart - start.QuadPart);
	cout << "////////////////////////////MAX/////////////////////////" << endl;
	cout << "双机加速结果为：" << maxresult2 << "         ";
	cout << "双机用时" << time3 << endl;
	cout << "单机结果为：" << maxresult << "         ";
	cout << "单机用时" << time4 << endl;
	cout << "加速比为" << time4 / time3 << endl;

	//单机程序SORT
	QueryPerformanceCounter(&start);//start  
	sort(rawdoubleData1, DATANUM, finalsort_result1);
	QueryPerformanceCounter(&end);//end
	time6 = (end.QuadPart - start.QuadPart);
	cout << "////////////////////////////SORT/////////////////////////" << endl;
	cout << "双机加速结果为：" << finalsort_result[DATANUM-1] << "         ";
	cout << "双机用时" << time5 << endl;
	cout << "双机数据传输时间 " << (end1.QuadPart - start1.QuadPart) << endl;
	cout << "单机结果为：" << finalsort_result1[DATANUM - 1] << "         ";
	cout << "单机用时" << time6 << endl;
	cout << "加速比为" << time6 / time5 << endl;
	cout << "除去传输时间加速比" << time6 / (time5- (end1.QuadPart - start1.QuadPart)) << endl;

	cout << counte << endl;
	//关闭套接字
	closesocket(s_server);
	//释放DLL资源
	WSACleanup();
	return 0;
}

void initialization() {
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

double sum(const double data[], const int len)
{
	double result = 0.0f;
	for (int i = 0; i < len; i++)
	{
		result += log(sqrt(data[i] / 4.0));
	}
	//cout << result << endl;
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

double sumSpeedUp1(const double data[], const int len) //sum单独使用openmp
{
	double result = 0.0f;
#pragma omp parallel for reduction(+:result)
	for (int i = 0; i < len / 2; i++)
	{
		result += log(sqrt(data[i] / 4.0));
	}
	return result;

}

double mymax(const double data[], const int len) //data是原始数据，len为长度。结果通过函数返回
{
	double result = 0.0f;
	for (int i = 0; i < len; i++)
	{
		double a = log(sqrt(data[i] / 4.0));
		if (result < a)
			result = a;
	}
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
	int i;
	double temp[4];
	__m256d max = _mm256_set1_pd(0.0f);        // init partial sums
//#pragma omp parallel for
	for (i = 0; i < len / 2; i = i + 4)
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
	for (size_t i = 0; i < len/2; i = i + 4)
	{
		__m256d va = _mm256_load_pd(&data[i]);
		_mm256_store_pd(&result[i], _mm256_log_pd(_mm256_sqrt_pd(_mm256_mul_pd(va, s_256))));
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
	//收割
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

void MemeryArray(double a[], int n, double b[], int m, double c[])
{
	int i, j, k;
	i = j = k = 0;
	while (i < n/2 && j < m/2)
	{
		if (a[i] < b[j])
			c[k++] = a[i++];
		else
			c[k++] = b[j++];
	}

	while (i < n/2)
		c[k++] = a[i++];

	while (j < m/2)
		c[k++] = b[j++];

}
