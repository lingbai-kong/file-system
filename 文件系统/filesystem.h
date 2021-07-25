#pragma once
#ifndef FILESYSTEM
#define FILESYSTEM
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <ctime>
#include <conio.h>
#include <windows.h>
#define VERSION 1.1
#define DISK_NAME "FS.dat"//�ļ�����
#define NAME_SIZE 28
#define BLOCK_SIZE 512
#define SUPER_BLOCK_START_ADDR 200 * BLOCK_SIZE
#define DISK_SIZE (261120+1024)*BLOCK_SIZE//128MB
#define ROOT_DIR_SYMBOL "~"
using namespace std;
string wstring2string(wstring wstr);
vector<string> splitstr(string str, string pattern);

class SuperBlock
{
public:
	unsigned int s_superblock_start_addr;
	unsigned int s_superblock_size;

	unsigned int s_inode_start_addr;
	unsigned int s_inode_size;
	unsigned int s_inode_num;
	unsigned int s_inode_fnum;

	unsigned int s_block_start_addr;
	unsigned int s_block_size;
	unsigned int s_block_num;
	unsigned int s_block_fnum;

	unsigned int s_ninode;
	unsigned int s_inode[100];
	int s_ilock;

	unsigned int s_nfree;
	unsigned int s_free[100];

	unsigned int file_max_size;
	unsigned int name_max_size;
	unsigned int dir_item_size;
	unsigned int dir_list_num;
};
class DiskInode
{
public:
	unsigned int d_mode;
	unsigned int d_nlink;
	short d_uid;
	short d_gid;
	unsigned int d_size;
	unsigned int d_addr[10];
	unsigned int d_atime;
	unsigned int d_mtime;
};
class Inode
{
public:
	unsigned int i_mode;
	unsigned int i_nlink;
	short i_uid;
	short i_gid;
	unsigned int i_size;
	unsigned int i_addr[10];
	unsigned int i_atime;
	unsigned int i_mtime;
	int i_no;
};
class File
{
public:
	short f_uid;
	short f_gid;
	Inode* f_inode;
	unsigned int f_offset;
};
class DirItem
{
public:
	char item_name[NAME_SIZE];
	int inode_no;
};
enum BufFlag								// b_flag�еı�־λ
{
	/* ��Ϊ����I/O����æ�ȣ���Щ��־û��ʹ��
	B_ERROR=0x8,							// I/O������������ֹ
	B_BUSY=0x10,							// ��Ӧ��������ʹ����
	B_WANTED = 0x20,						// �н��̵ȴ�ʹ�ø�buf����Ļ���
	B_ASYNC=0x40,							// �첽I/O����
	B_WRITE = 0x1,							// д����
	B_READ = 0x2,							// ������
	*/
	B_NONE = 0x0,
	B_DONE = 0x4,							// I/O��������
	B_DELWRI = 0x80							// �ӳ�д
};
class Buf
{
public:

	/* δʹ�õ�����
	short b_dev;								// �ߡ���8λ�ֱ����������豸��
	int b_wcount;								// ��Ҫ���͵��ֽ���
	int b_error;								// I/O����ʱ��Ϣ
	int b_resid;								// I/O����ʱ��δ���͵�ʣ���ֽ���
	int padding;								// 4�ֽ����
	*/

	unsigned int b_blkno;						// �����߼����
	char* b_addr;								// ָ��û�����ƿ����Ļ������׵�ַ
	unsigned int b_flags;						// ������ƿ��־λ
	Buf* b_forw;
	Buf* b_back;
	Buf* av_forw;
	Buf* av_back;
};
class BufferManager
{
public:
	static const int NBUF = 15;					// ������ƿ顢������������
	static const int BUFFER_SIZE = BLOCK_SIZE;	// ��������С�����ֽ�Ϊ��λ
private:
	Buf bFreeList;								// ���ɻ�����п��ƿ�
	Buf diskTab;								// �����豸
	Buf m_Buf[NBUF];							// ������ƿ�����
	char Buffer[NBUF][BUFFER_SIZE];				// ����������
public:
	BufferManager();
	~BufferManager();
	void initialize();							// ������ƿ���еĳ�ʼ��
	Buf* getBlk(unsigned int blkno);			// ����һ�黺���
	void relseBlk(Buf* bp);						// �ͷŻ�����ƿ�
	Buf* Bread(unsigned int blkno);				// ��һ�̿�
	void Bwrite(Buf* bp);						// дһ�̿�
	void Bdwirte(Buf* bp);						// �ӳ�дһ�̿�
	void bread(char* buf, unsigned int start_addr, unsigned int length);
	void bwrite(const char* buf, unsigned int start_addr, unsigned int length);
};
class FS
{
private:
	const int root_dir_no = 0;
	const unsigned int IALLOC = 1 << 15;
	const unsigned int NORMAL_DATA_FILE = 0 << 13;
	const unsigned int CHAR_DEVICE_FILE = 1 << 13;
	const unsigned int DIR_FILE = 2 << 13;
	const unsigned int BLOCK_DEVICE_FILE = 3 << 13;
	const unsigned int SMALL_FILE = 0 << 12;
	const unsigned int HUGE_FILE = 1 << 12;
	const unsigned int OWNER_R = 4 << 6;//���û���Ȩ��
	const unsigned int OWNER_W = 2 << 6;//���û�дȨ��
	const unsigned int OWNER_X = 1 << 6;//���û�ִ��Ȩ��
	const unsigned int GROUP_R = 4 << 3;//���û���Ȩ��
	const unsigned int GROUP_W = 2 << 3;//���û�дȨ��
	const unsigned int GROUP_X = 1 << 3;//���û�ִ��Ȩ��
	const unsigned int OTHERS_R = 4;//�����û���Ȩ��
	const unsigned int OTHERS_W = 2;//�����û�дȨ��
	const unsigned int OTHERS_X = 1;//�����û�ִ��Ȩ��
	const unsigned int FILE_DEF_PERMISSION = OWNER_R | OWNER_W | GROUP_R | GROUP_W | OTHERS_R;
	const unsigned int DIR_DEF_PERMISSION = OWNER_R | OWNER_W | OWNER_X | GROUP_R | GROUP_X | OTHERS_R | OTHERS_X;
protected:
	BufferManager buffer;
	SuperBlock spb;
	Inode iload(int inode_no);
	void isave(Inode inode);
	Inode ialloc();
	void ifree(Inode inode);
	int balloc();
	void bfree(int no);
	Inode _ffind(Inode& cur_inode, const char* name);
	void _fread(Inode& cur_inode, char* buf, unsigned int start_addr, unsigned int len);
	void _fwrite(Inode& cur_inode, const char* buf, unsigned int start_addr, unsigned int len);
	void _fdelete(Inode& cur_inode);
	void _ddeleteall(Inode& cur_inode);
	void initialize();
public:
	File* fopen(const char* dir, short uid = 0, short gid = 0);
	int fclose(File* fp);
	int fread(char* buffer, int size, int count, File* fp);
	int fwrite(const char* buffer, int size, int count, File* fp);
	int freplace(const char* buffer, int size, int count, File* fp);
	int fcreate(const char* dir, short uid = 0, short gid = 0);
	int fdelete(const char* dir, short uid = 0, short gid = 0);
	int fseek(File* fp, int offset, int whence);
	int ftell(File* fp);
	int dcreate(const char* dir, short uid = 0, short gid = 0);
	int ddelete(const char* dir, short uid = 0, short gid = 0);
	int chmod(const char* dir, int mode, short uid = 0, short gid = 0);
	vector<string> list(const char* dir, short uid = 0, short gid = 0);
	int enter(const char* dir, short uid = 0, short gid = 0);
	int fformat();
	SuperBlock getSpb();
	FS();
	~FS();
};
#endif // !FILESYSTEM
