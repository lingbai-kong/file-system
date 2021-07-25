#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include "filesystem.h"
using namespace std;
void wintest()
{
	FILE* fp = fopen("Jerry.txt", "wb+");
	if (fp == NULL)
	{
		throw("打开文件失败\n");
	}
	cout << "文件指针：" << ftell(fp) << endl;
	// write
	char wbuf[800];
	for (int i = 0; i < 800; i++)
	{
		if (i < 500)
		{
			wbuf[i] = 'a' + i % 26;
		}
		else
		{
			wbuf[i] = 'A' + i % 26;
		}
	}
	cout << "写入内容：";
	for (int i = 0; i < 800; i++)
	{
		if ((wbuf[i] >= 'a' && wbuf[i] <= 'z') || (wbuf[i] >= 'A' && wbuf[i] <= 'Z'))
		{
			cout << wbuf[i];
		}
		else
		{
			cout << '？';
		}
	}
	cout << endl;
	cout << "开始写入\n";
	cout << "写入" << int(fwrite(wbuf, 1, 800, fp)) << "B\n";
	cout << "写入完成\n";
	cout << "文件指针：" << ftell(fp) << endl;
	// seek
	cout << "开始定位文件指针\n";
	fseek(fp, 500, SEEK_SET);
	cout << "定位完成\n";
	cout << "文件指针：" << ftell(fp) << endl;
	// read
	char abc[500];
	memset(abc, 0, 500);
	cout << "开始读出\n";
	cout << "读出" << int(fread(abc, 1, 500, fp)) << "B\n";
	cout << "读出完成\n";
	cout << "读出内容：";
	for (int i = 0; i < 500; i++)
	{
		if ((abc[i] >= 'a' && abc[i] <= 'z') || (abc[i] >= 'A' && abc[i] <= 'Z'))
		{
			cout << abc[i];
		}
		else
		{
			cout << '?';
		}
	}
	cout << endl;
	cout << "文件指针：" << ftell(fp) << endl;
	// write
	cout << "再次写入\n";
	cout << "写入" << int(fwrite(abc, 1, 500, fp)) << "B\n";
	cout << "写入完成\n";
	cout << "文件指针：" << ftell(fp) << endl;
	fclose(fp);
}
void test()
{
	FS fs;
	fs.fcreate("/test/Jerry");
	File* fp = fs.fopen("/test/Jerry");
	if (fp == NULL)
	{
		throw("打开文件失败\n");
	}
	cout << "文件指针：" << fs.ftell(fp) << endl;
	// write
	char wbuf[800];
	for (int i = 0; i < 800; i++)
	{
		if (i < 500)
		{
			wbuf[i] = 'a' + i % 26;
		}
		else
		{
			wbuf[i] = 'A' + i % 26;
		}
	}
	cout << "写入内容：";
	for (int i = 0; i < 800; i++)
	{
		if ((wbuf[i] >= 'a' && wbuf[i] <= 'z') || (wbuf[i] >= 'A' && wbuf[i] <= 'Z'))
		{
			cout << wbuf[i];
		}
		else
		{
			cout << '？';
		}
	}
	cout << endl;
	cout << "开始写入\n";
	cout << "写入" << int(fs.fwrite(wbuf, 1, 800, fp)) << "B\n";
	cout << "写入完成\n";
	cout << "文件指针：" << fs.ftell(fp) << endl;
	// seek
	cout << "开始定位文件指针\n";
	fs.fseek(fp, 500, SEEK_SET);
	cout << "定位完成\n";
	cout << "文件指针：" << fs.ftell(fp) << endl;
	// read
	char abc[500];
	memset(abc, 0, 500);
	cout << "开始读出\n";
	cout << "读出" << int(fs.fread(abc, 1, 500, fp)) << "B\n";
	cout << "读出完成\n";
	cout << "读出内容：";
	for (int i = 0; i < 500; i++)
	{
		if ((abc[i] >= 'a' && abc[i] <= 'z') || (abc[i] >= 'A' && abc[i] <= 'Z'))
		{
			cout << abc[i];
		}
		else
		{
			cout << '?';
		}
	}
	cout << endl;
	cout << "文件指针：" << fs.ftell(fp) << endl;
	// write
	cout << "再次写入\n";
	cout << "写入" << int(fs.fwrite(abc, 1, 500, fp)) << "B\n";
	cout << "写入完成\n";
	cout << "文件指针：" << fs.ftell(fp) << endl;
	fs.fclose(fp);
	fs.fdelete("/test/Jerry");
}
int main()
{
	try {
		cout << "测试FS文件系统\n";
		test();
		cout << "使用C++标准输入输出库对Windows下的文件进行相同操作，用于检验之前测试的正确性\n";
		wintest();
	}
	catch (...)
	{
		cout << "程序运行错误\n";
	}
	return 0;
}