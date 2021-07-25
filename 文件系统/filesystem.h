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
#define DISK_NAME "FS.dat"//文件卷名
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
enum BufFlag								// b_flag中的标志位
{
	/* 因为磁盘I/O采用忙等，这些标志没有使用
	B_ERROR=0x8,							// I/O操作因出错而终止
	B_BUSY=0x10,							// 相应缓存正在使用中
	B_WANTED = 0x20,						// 有进程等待使用该buf管理的缓存
	B_ASYNC=0x40,							// 异步I/O操作
	B_WRITE = 0x1,							// 写操作
	B_READ = 0x2,							// 读操作
	*/
	B_NONE = 0x0,
	B_DONE = 0x4,							// I/O操作结束
	B_DELWRI = 0x80							// 延迟写
};
class Buf
{
public:

	/* 未使用的属性
	short b_dev;								// 高、低8位分别是主、次设备号
	int b_wcount;								// 需要传送的字节数
	int b_error;								// I/O出错时信息
	int b_resid;								// I/O出错时尚未传送的剩余字节数
	int padding;								// 4字节填充
	*/

	unsigned int b_blkno;						// 磁盘逻辑块号
	char* b_addr;								// 指向该缓存控制块管理的缓冲区首地址
	unsigned int b_flags;						// 缓存控制块标志位
	Buf* b_forw;
	Buf* b_back;
	Buf* av_forw;
	Buf* av_back;
};
class BufferManager
{
public:
	static const int NBUF = 15;					// 缓存控制块、缓冲区的数量
	static const int BUFFER_SIZE = BLOCK_SIZE;	// 缓冲区大小，以字节为单位
private:
	Buf bFreeList;								// 自由缓存队列控制块
	Buf diskTab;								// 磁盘设备
	Buf m_Buf[NBUF];							// 缓存控制块数组
	char Buffer[NBUF][BUFFER_SIZE];				// 缓冲区数组
public:
	BufferManager();
	~BufferManager();
	void initialize();							// 缓存控制块队列的初始化
	Buf* getBlk(unsigned int blkno);			// 申请一块缓存块
	void relseBlk(Buf* bp);						// 释放缓存控制块
	Buf* Bread(unsigned int blkno);				// 读一盘块
	void Bwrite(Buf* bp);						// 写一盘块
	void Bdwirte(Buf* bp);						// 延迟写一盘块
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
	const unsigned int OWNER_R = 4 << 6;//本用户读权限
	const unsigned int OWNER_W = 2 << 6;//本用户写权限
	const unsigned int OWNER_X = 1 << 6;//本用户执行权限
	const unsigned int GROUP_R = 4 << 3;//组用户读权限
	const unsigned int GROUP_W = 2 << 3;//组用户写权限
	const unsigned int GROUP_X = 1 << 3;//组用户执行权限
	const unsigned int OTHERS_R = 4;//其它用户读权限
	const unsigned int OTHERS_W = 2;//其它用户写权限
	const unsigned int OTHERS_X = 1;//其它用户执行权限
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
