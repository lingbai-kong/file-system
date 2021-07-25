#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include "filesystem.h"
using namespace std;
void wintest()
{
	FILE* fp = fopen("Jerry.txt", "wb+");
	if (fp == NULL)
	{
		throw("���ļ�ʧ��\n");
	}
	cout << "�ļ�ָ�룺" << ftell(fp) << endl;
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
	cout << "д�����ݣ�";
	for (int i = 0; i < 800; i++)
	{
		if ((wbuf[i] >= 'a' && wbuf[i] <= 'z') || (wbuf[i] >= 'A' && wbuf[i] <= 'Z'))
		{
			cout << wbuf[i];
		}
		else
		{
			cout << '��';
		}
	}
	cout << endl;
	cout << "��ʼд��\n";
	cout << "д��" << int(fwrite(wbuf, 1, 800, fp)) << "B\n";
	cout << "д�����\n";
	cout << "�ļ�ָ�룺" << ftell(fp) << endl;
	// seek
	cout << "��ʼ��λ�ļ�ָ��\n";
	fseek(fp, 500, SEEK_SET);
	cout << "��λ���\n";
	cout << "�ļ�ָ�룺" << ftell(fp) << endl;
	// read
	char abc[500];
	memset(abc, 0, 500);
	cout << "��ʼ����\n";
	cout << "����" << int(fread(abc, 1, 500, fp)) << "B\n";
	cout << "�������\n";
	cout << "�������ݣ�";
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
	cout << "�ļ�ָ�룺" << ftell(fp) << endl;
	// write
	cout << "�ٴ�д��\n";
	cout << "д��" << int(fwrite(abc, 1, 500, fp)) << "B\n";
	cout << "д�����\n";
	cout << "�ļ�ָ�룺" << ftell(fp) << endl;
	fclose(fp);
}
void test()
{
	FS fs;
	fs.fcreate("/test/Jerry");
	File* fp = fs.fopen("/test/Jerry");
	if (fp == NULL)
	{
		throw("���ļ�ʧ��\n");
	}
	cout << "�ļ�ָ�룺" << fs.ftell(fp) << endl;
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
	cout << "д�����ݣ�";
	for (int i = 0; i < 800; i++)
	{
		if ((wbuf[i] >= 'a' && wbuf[i] <= 'z') || (wbuf[i] >= 'A' && wbuf[i] <= 'Z'))
		{
			cout << wbuf[i];
		}
		else
		{
			cout << '��';
		}
	}
	cout << endl;
	cout << "��ʼд��\n";
	cout << "д��" << int(fs.fwrite(wbuf, 1, 800, fp)) << "B\n";
	cout << "д�����\n";
	cout << "�ļ�ָ�룺" << fs.ftell(fp) << endl;
	// seek
	cout << "��ʼ��λ�ļ�ָ��\n";
	fs.fseek(fp, 500, SEEK_SET);
	cout << "��λ���\n";
	cout << "�ļ�ָ�룺" << fs.ftell(fp) << endl;
	// read
	char abc[500];
	memset(abc, 0, 500);
	cout << "��ʼ����\n";
	cout << "����" << int(fs.fread(abc, 1, 500, fp)) << "B\n";
	cout << "�������\n";
	cout << "�������ݣ�";
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
	cout << "�ļ�ָ�룺" << fs.ftell(fp) << endl;
	// write
	cout << "�ٴ�д��\n";
	cout << "д��" << int(fs.fwrite(abc, 1, 500, fp)) << "B\n";
	cout << "д�����\n";
	cout << "�ļ�ָ�룺" << fs.ftell(fp) << endl;
	fs.fclose(fp);
	fs.fdelete("/test/Jerry");
}
int main()
{
	try {
		cout << "����FS�ļ�ϵͳ\n";
		test();
		cout << "ʹ��C++��׼����������Windows�µ��ļ�������ͬ���������ڼ���֮ǰ���Ե���ȷ��\n";
		wintest();
	}
	catch (...)
	{
		cout << "�������д���\n";
	}
	return 0;
}