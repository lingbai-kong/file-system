#include "filesystem.h"
using namespace std;
string wstring2string(wstring wstr)
{
	string result;
	//获取缓冲区大小，并申请空间，缓冲区大小事按字节计算的  
	int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), NULL, 0, NULL, NULL);
	char* buffer = new char[len + 1];
	//宽字节编码转换成多字节编码  
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), buffer, len, NULL, NULL);
	buffer[len] = '\0';
	//删除缓冲区并返回值  
	result.append(buffer);
	delete[] buffer;
	return result;
}
vector<string> splitstr(string str, string pattern)
{
	vector<string> res;
	char* pTmp = NULL;
	char* temp = strtok_s((char*)str.c_str(), pattern.c_str(), &pTmp);
	while (temp != NULL)
	{
		res.push_back(string(temp));
		temp = strtok_s(NULL, pattern.c_str(), &pTmp);
	}
	return res;
}

BufferManager::BufferManager()
{
	initialize();
}
BufferManager::~BufferManager()
{
	for (auto i = 0; i < NBUF; i++)
	{
		relseBlk(&m_Buf[i]);
	}
}
void BufferManager::initialize()
{
	bFreeList.b_blkno = -1;
	bFreeList.b_addr = NULL;
	bFreeList.b_flags = BufFlag::B_NONE;
	bFreeList.b_forw = &m_Buf[NBUF - 1];
	bFreeList.b_back = &m_Buf[0];
	bFreeList.av_forw = &m_Buf[0];
	bFreeList.av_back = &m_Buf[NBUF - 1];
	for (auto i = 0; i < NBUF; i++)
	{
		m_Buf[i].b_blkno = -1;
		m_Buf[i].b_addr = Buffer[i];
		m_Buf[i].b_flags = BufFlag::B_NONE;
		if (i - 1 >= 0)
		{
			m_Buf[i].b_forw = &m_Buf[i - 1];
			m_Buf[i].av_back = &m_Buf[i - 1];
		}
		else
		{
			m_Buf[i].b_forw = &bFreeList;
			m_Buf[i].av_back = &bFreeList;
		}
		if (i + 1 < NBUF)
		{
			m_Buf[i].b_back = &m_Buf[i + 1];
			m_Buf[i].av_forw = &m_Buf[i + 1];
		}
		else
		{
			m_Buf[i].b_back = &bFreeList;
			m_Buf[i].av_forw = &bFreeList;
		}
	}
	diskTab.b_blkno = -1;
	diskTab.b_addr = NULL;
	diskTab.b_flags = BufFlag::B_NONE;
	diskTab.b_forw = &diskTab;
	diskTab.b_back = &diskTab;
	diskTab.av_forw = &diskTab;
	diskTab.av_back = &diskTab;
}
Buf* BufferManager::getBlk(unsigned int blkno)
{
	Buf* bp = NULL;
	for (bp = diskTab.b_back; bp != &diskTab; bp = bp->b_back)
	{
		if (bp->b_blkno == blkno)
		{
			return bp;
		}
	}
	// 自由队列空
	if (bFreeList.av_back == &bFreeList)
	{
		// 实际中I/O请求队列中最多有一个缓存，因此自由队列不可能为空
	}
	else
	{
		bp = bFreeList.av_back;
		// 从自由队列取出
		bp->av_forw->av_back = bp->av_back;
		bp->av_back->av_forw = bp->av_forw;
		// 从原设备队列或NODEV队列取出
		bp->b_forw->b_back = bp->b_back;
		bp->b_back->b_forw = bp->b_forw;
		// 含有延迟写
		if ((bp->b_flags & BufFlag::B_DELWRI) == BufFlag::B_DELWRI)
		{
			// 立即写入
			Bwrite(bp);
			// 清除延迟写标志
			bp->b_flags = bp->b_flags & (~BufFlag::B_DELWRI);
		}
		// 清除所有标志
		bp->b_flags = BufFlag::B_NONE;
		// 填写blkno，插入设备队列队头
		bp->b_blkno = blkno;
		bp->b_forw = &diskTab;
		bp->b_back = diskTab.b_back;
		bp->b_forw->b_back = bp;
		bp->b_back->b_forw = bp;
	}
	return bp;
}
void BufferManager::relseBlk(Buf* bp)
{
	if ((bp->b_flags & BufFlag::B_DELWRI) == BufFlag::B_DELWRI)
	{
		// 立即写入
		Bwrite(bp);
		// 清除延迟写标志
		bp->b_flags = bp->b_flags & (~BufFlag::B_DELWRI);
	}
	// 清除所有标志
	bp->b_flags = BufFlag::B_NONE;
	// 置入自由队列和NODEV队列
	bp->b_forw->b_back = bp->b_back;
	bp->b_back->b_forw = bp->b_forw;
	bp->av_forw->av_back = bp->av_back;
	bp->av_back->av_forw = bp->av_forw;

	bp->av_forw = bFreeList.av_forw;
	bp->av_back = &bFreeList;
	bp->av_forw->av_back = bp;
	bp->av_back->av_forw = bp;

	bp->b_forw = &bFreeList;
	bp->b_back = bFreeList.b_back;
	bp->b_forw->b_back = bp;
	bp->b_back->b_forw = bp;
}
Buf* BufferManager::Bread(unsigned int blkno)
{
	Buf* bp = getBlk(blkno);
	if ((bp->b_flags & BufFlag::B_DONE) == BufFlag::B_DONE)
	{
		return bp;
	}
	else
	{
		// I/O读操作，送入I/O请求队列
		bp->av_forw = diskTab.av_forw;
		bp->av_back = &diskTab;
		bp->av_forw->av_back = bp;
		bp->av_back->av_forw = bp;
		// 开始读操作
		fstream fin;
		fin.open(DISK_NAME, ios::in | ios::binary);
		if (!fin.is_open())
		{
			throw("磁盘映像输入文件流打开失败\n");
		}
		fin.seekg(streampos(blkno) * streampos(BUFFER_SIZE), ios::beg);
		fin.read(bp->b_addr, BUFFER_SIZE);
		fin.close();
		// 读操作完成
		// 更新标志
		bp->b_flags = BufFlag::B_DONE;
		// 从I/O请求队列取出
		bp->av_forw->av_back = bp->av_back;
		bp->av_back->av_forw = bp->av_forw;
		// 加入自由队列
		bp->av_forw = bFreeList.av_forw;
		bp->av_back = &bFreeList;
		bp->av_forw->av_back = bp;
		bp->av_back->av_forw = bp;
		return bp;
	}
}
void BufferManager::Bwrite(Buf* bp)
{
	// I/O写操作，送入I/O请求队列
	bp->av_forw = diskTab.av_forw;
	bp->av_back = &diskTab;
	bp->av_forw->av_back = bp;
	bp->av_back->av_forw = bp;
	// 开始写操作
	fstream fout;
	fout.open(DISK_NAME, ios::in | ios::out | ios::binary);
	if (!fout.is_open())
	{
		throw("磁盘映像输出文件流打开失败\n");
	}
	fout.seekp(streampos(bp->b_blkno) * streampos(BUFFER_SIZE), ios::beg);
	fout.write((const char*)bp->b_addr, BUFFER_SIZE);
	fout.close();
	// 写操作完成
	// 更新标志
	bp->b_flags = BufFlag::B_DONE;
	// 从I/O请求队列取出
	bp->av_forw->av_back = bp->av_back;
	bp->av_back->av_forw = bp->av_forw;
	// 加入自由队列
	bp->av_forw = bFreeList.av_forw;
	bp->av_back = &bFreeList;
	bp->av_forw->av_back = bp;
	bp->av_back->av_forw = bp;
}
void  BufferManager::Bdwirte(Buf* bp)
{
	bp->b_flags |= BufFlag::B_DELWRI;
}
void BufferManager::bread(char* buf, unsigned int start_addr, unsigned int length)
{
	if (buf == NULL)
		return;
	unsigned int pos = 0;
	unsigned int start_blkno = start_addr / BUFFER_SIZE;
	unsigned int end_blkno = (start_addr + length - 1) / BUFFER_SIZE;
	for (unsigned int blkno = start_blkno; blkno <= end_blkno; blkno++)
	{
		Buf* bp = Bread(blkno);
		if (blkno == start_blkno && blkno == end_blkno)
		{
			memcpy_s(buf + pos, length, bp->b_addr + start_addr % BUFFER_SIZE, length);
			pos += length;
		}
		else if (blkno == start_blkno)
		{
			memcpy_s(buf + pos, (BUFFER_SIZE - 1) - (start_addr % BUFFER_SIZE) + 1, bp->b_addr + start_addr % BUFFER_SIZE, (BUFFER_SIZE - 1) - (start_addr % BUFFER_SIZE) + 1);
			pos += BUFFER_SIZE - (start_addr % BUFFER_SIZE);
		}
		else if (blkno == end_blkno)
		{
			memcpy_s(buf + pos, (start_addr + length - 1) % BUFFER_SIZE - 0 + 1, bp->b_addr, (start_addr + length - 1) % BUFFER_SIZE - 0 + 1);
			pos += (start_addr + length - 1) % BUFFER_SIZE - 0 + 1;
		}
		else
		{
			memcpy_s(buf + pos, BUFFER_SIZE, bp->b_addr, BUFFER_SIZE);
			pos += BUFFER_SIZE;
		}
	}
}
void BufferManager::bwrite(const char* buf, unsigned int start_addr, unsigned int length)
{
	unsigned int pos = 0;
	unsigned int start_blkno = start_addr / BUFFER_SIZE;
	unsigned int end_blkno = (start_addr + length - 1) / BUFFER_SIZE;
	for (unsigned int blkno = start_blkno; blkno <= end_blkno; blkno++)
	{
		Buf* bp = Bread(blkno);
		if (blkno == start_blkno && blkno == end_blkno)
		{
			memcpy_s(bp->b_addr + start_addr % BUFFER_SIZE, length, buf + pos, length);
			pos += length;
		}
		else if (blkno == start_blkno)
		{
			memcpy_s(bp->b_addr + start_addr % BUFFER_SIZE, (BUFFER_SIZE - 1) - (start_addr % BUFFER_SIZE) + 1, buf + pos, (BUFFER_SIZE - 1) - (start_addr % BUFFER_SIZE) + 1);
			pos += BUFFER_SIZE - (start_addr % BUFFER_SIZE);
		}
		else if (blkno == end_blkno)
		{
			memcpy_s(bp->b_addr, (start_addr + length - 1) % BUFFER_SIZE - 0 + 1, buf + pos, (start_addr + length - 1) % BUFFER_SIZE - 0 + 1);
			pos += (start_addr + length - 1) % BUFFER_SIZE - 0 + 1;
		}
		else
		{
			memcpy_s(bp->b_addr, BUFFER_SIZE, buf + pos, BUFFER_SIZE);
			pos += BUFFER_SIZE;
		}
		Bdwirte(bp);
	}
}

Inode FS::iload(int inode_no)
{
	DiskInode d_inode;
	buffer.bread((char*)&d_inode, spb.s_inode_start_addr + inode_no * spb.s_inode_size, spb.s_inode_size);
	Inode inode;
	memcpy_s(&inode, spb.s_inode_size, &d_inode, spb.s_inode_size);
	inode.i_no = inode_no;
	return inode;
}
void FS::isave(Inode inode)
{
	DiskInode d_inode;
	memcpy_s(&d_inode, spb.s_inode_size, &inode, spb.s_inode_size);
	buffer.bwrite((const char*)&d_inode, spb.s_inode_start_addr + inode.i_no * spb.s_inode_size, spb.s_inode_size);
}
Inode FS::ialloc()
{
	if (spb.s_inode_fnum <= 0)
	{
		throw("Inode分配失败，因为没有空闲的Inode\n");
	}

	if (spb.s_ninode <= 0)
	{
		for (unsigned int i = 0; i < spb.s_inode_num && spb.s_ninode < 100; i++)
		{
			Inode item = iload(i);
			if ((item.i_mode & IALLOC) == 0)
			{
				spb.s_inode[spb.s_ninode++] = i;
			}
		}
	}
	if (spb.s_ninode <= 0 || spb.s_ninode > 100)
	{
		throw("读取spb.s_inode越界\n");
	}
	spb.s_inode_fnum--;
	int ret_no = spb.s_inode[--spb.s_ninode];
	return iload(ret_no);
}
void FS::ifree(Inode inode)
{
	memset(&inode, 0, spb.s_inode_size);
	isave(inode);
	spb.s_inode_fnum++;
	if (spb.s_ninode < 100)
	{
		spb.s_inode[spb.s_ninode++] = inode.i_no;
	}
}
int FS::balloc()
{
	if (spb.s_block_fnum <= 0)
	{
		throw("block分配失败，因为没有空闲的block\n");
	}
	if (spb.s_nfree <= 0 || spb.s_nfree > 100)
	{
		throw("读取spb.s_free越界\n");
	}
	spb.s_block_fnum--;
	int ret_no = spb.s_free[--spb.s_nfree];
	if (spb.s_nfree == 0 && spb.s_block_fnum > 0)
	{
		int address = spb.s_block_start_addr + spb.s_free[0] * spb.s_block_size;
		buffer.bread((char*)&spb.s_nfree, address, sizeof(spb.s_nfree));
		buffer.bread((char*)spb.s_free, address + sizeof(spb.s_nfree), sizeof(spb.s_free));
	}
	return ret_no;
}
void FS::bfree(int no)
{
	spb.s_block_fnum++;
	if (spb.s_nfree == 100)
	{
		int cur_block_address = spb.s_block_start_addr + no * spb.s_block_size;
		buffer.bwrite((const char*)&spb.s_nfree, cur_block_address, sizeof(spb.s_nfree));
		buffer.bwrite((const char*)spb.s_free, cur_block_address + int(sizeof(spb.s_nfree)), sizeof(spb.s_free));
		spb.s_nfree = 0;
		memset(spb.s_free, 0, sizeof(spb.s_free));
	}
	spb.s_free[spb.s_nfree++] = no;
}
Inode FS::_ffind(Inode& cur_inode, const char* name)
{
	if ((cur_inode.i_mode & (3 << 13)) != DIR_FILE)
	{
		throw("当前路径非目录文件\n");
	}

	DirItem* dir_list = new DirItem[cur_inode.i_size / spb.dir_item_size]{ 0 };
	_fread(cur_inode, (char*)dir_list, 0, cur_inode.i_size);
	for (unsigned int i = 0; i < cur_inode.i_size / spb.dir_item_size; i++)
	{
		if (strcmp(dir_list[i].item_name, name) == 0)
		{
			int ret_no = dir_list[i].inode_no;
			delete[] dir_list;
			return iload(ret_no);
		}
	}
	delete[] dir_list;
	throw("找不到该文件\n");
}
void FS::_fread(Inode& cur_inode, char* buf, unsigned int start_addr, unsigned int len)
{
	if (int(start_addr + len) > cur_inode.i_size)
	{
		throw("读文件越界\n");
	}
	if (cur_inode.i_size == 0)
	{
		return;
	}
	char* filedata = new char[cur_inode.i_size]{ 0 };
	unsigned int length = 0;
	for (unsigned int i = 0; i < 6 && length < cur_inode.i_size; i++)
	{
		if (length + spb.s_block_size < cur_inode.i_size)
		{
			buffer.bread((char*)filedata + length, spb.s_block_start_addr + cur_inode.i_addr[i] * spb.s_block_size, spb.s_block_size);
			length += spb.s_block_size;
		}
		else
		{
			buffer.bread((char*)filedata + length, spb.s_block_start_addr + cur_inode.i_addr[i] * spb.s_block_size, cur_inode.i_size - length);
			length = cur_inode.i_size;
		}
	}
	for (unsigned int i = 6; i < 8 && length < cur_inode.i_size; i++)
	{
		int* one_time_indirect_index = new int[spb.s_block_size / sizeof(int)]{ 0 };
		buffer.bread((char*)one_time_indirect_index, spb.s_block_start_addr + cur_inode.i_addr[i] * spb.s_block_size, spb.s_block_size);
		for (unsigned int j = 0; j < spb.s_block_size / sizeof(int) && length < cur_inode.i_size; j++)
		{
			if (length + spb.s_block_size < cur_inode.i_size)
			{
				buffer.bread((char*)filedata + length, spb.s_block_start_addr + one_time_indirect_index[j] * spb.s_block_size, spb.s_block_size);
				length += spb.s_block_size;
			}
			else
			{
				buffer.bread((char*)filedata + length, spb.s_block_start_addr + one_time_indirect_index[j] * spb.s_block_size, cur_inode.i_size - length);
				length = cur_inode.i_size;
			}
		}
		delete[] one_time_indirect_index;
	}
	for (unsigned int i = 8; i < 10 && length < cur_inode.i_size; i++)
	{
		int* two_time_indirect_index = new int[spb.s_block_size / sizeof(int)]{ 0 };
		buffer.bread((char*)two_time_indirect_index, spb.s_block_start_addr + cur_inode.i_addr[i] * spb.s_block_size, spb.s_block_size);
		for (unsigned int j = 0; j < spb.s_block_size / sizeof(int) && length < cur_inode.i_size; j++)
		{
			int* one_time_indirect_index = new int[spb.s_block_size / sizeof(int)]{ 0 };
			buffer.bread((char*)one_time_indirect_index, spb.s_block_start_addr + two_time_indirect_index[j] * spb.s_block_size, spb.s_block_size);
			for (unsigned int k = 0; k < spb.s_block_size / sizeof(int) && length < cur_inode.i_size; k++)
			{
				if (length + spb.s_block_size < cur_inode.i_size)
				{
					buffer.bread((char*)filedata + length, spb.s_block_start_addr + one_time_indirect_index[k] * spb.s_block_size, spb.s_block_size);
					length += spb.s_block_size;
				}
				else
				{
					buffer.bread((char*)filedata + length, spb.s_block_start_addr + one_time_indirect_index[k] * spb.s_block_size, cur_inode.i_size - length);
					length = cur_inode.i_size;
				}
			}
			delete[] one_time_indirect_index;
		}
		delete[] two_time_indirect_index;
	}
	memcpy_s(buf, len, filedata + start_addr, len);
	delete[] filedata;
	cur_inode.i_atime = unsigned int(time(NULL));
}
void FS::_fwrite(Inode& cur_inode, const char* buf, unsigned int start_addr, unsigned int len)
{
	if (int(start_addr + len) > spb.file_max_size)
	{
		throw("写文件过大\n");
	}
	unsigned int new_size = int(start_addr + len) > cur_inode.i_size ? start_addr + len : cur_inode.i_size;
	char* filedata = new char[new_size + 1]{ 0 };
	_fread(cur_inode, filedata, 0, cur_inode.i_size);
	if (start_addr > cur_inode.i_size)
	{
		memset(filedata + cur_inode.i_size, 0, start_addr - cur_inode.i_size);
	}
	memcpy_s(filedata + start_addr, len, buf, len);

	int last_block_num = (cur_inode.i_size + spb.s_block_size - 1) / spb.s_block_size;
	int cur_block_num = (new_size + spb.s_block_size - 1) / spb.s_block_size;
	int block_count = 0;
	for (unsigned int i = 0; i < 6 && block_count < cur_block_num; i++)
	{
		if (block_count < last_block_num)
		{
			block_count++;
		}
		else
		{
			int block_no = balloc();
			cur_inode.i_addr[i] = block_no;
			block_count++;
		}
	}
	for (unsigned int i = 6; i < 8 && block_count < cur_block_num; i++)
	{
		int* one_time_indirect_index = new int[spb.s_block_size / sizeof(int)]{ 0 };
		if (block_count < last_block_num)
		{
			buffer.bread((char*)one_time_indirect_index, spb.s_block_start_addr + cur_inode.i_addr[i] * spb.s_block_size, spb.s_block_size);
		}
		else
		{
			int block_no = balloc();
			cur_inode.i_addr[i] = block_no;
		}
		for (unsigned int j = 0; j < spb.s_block_size / sizeof(int) && block_count < cur_block_num; j++)
		{
			if (block_count < last_block_num)
			{
				block_count++;
			}
			else
			{
				int block_no = balloc();
				one_time_indirect_index[j] = block_no;
				block_count++;
			}
		}
		buffer.bwrite((const char*)one_time_indirect_index, spb.s_block_start_addr + cur_inode.i_addr[i] * spb.s_block_size, spb.s_block_size);
		delete[] one_time_indirect_index;
	}
	for (unsigned int i = 8; i < 10 && block_count < cur_block_num; i++)
	{
		int* two_time_indirect_index = new int[spb.s_block_size / sizeof(int)]{ 0 };
		if (block_count < last_block_num)
		{
			buffer.bread((char*)two_time_indirect_index, spb.s_block_start_addr + cur_inode.i_addr[i] * spb.s_block_size, spb.s_block_size);
		}
		else
		{
			int block_no = balloc();
			cur_inode.i_addr[i] = block_no;
		}
		for (unsigned int j = 0; j < spb.s_block_size / sizeof(int) && block_count < cur_block_num; j++)
		{
			int* one_time_indirect_index = new int[spb.s_block_size / sizeof(int)]{ 0 };
			if (block_count < last_block_num)
			{
				buffer.bread((char*)one_time_indirect_index, spb.s_block_start_addr + two_time_indirect_index[j] * spb.s_block_size, spb.s_block_size);
			}
			else
			{
				int block_no = balloc();
				two_time_indirect_index[j] = block_no;
			}
			for (unsigned int k = 0; k < spb.s_block_size / sizeof(int) && block_count < cur_block_num; k++)
			{
				if (block_count < last_block_num)
				{
					block_count++;
				}
				else
				{
					int block_no = balloc();
					one_time_indirect_index[k] = block_no;
					block_count++;
				}
			}
			buffer.bwrite((const char*)one_time_indirect_index, spb.s_block_start_addr + two_time_indirect_index[j] * spb.s_block_size, spb.s_block_size);
			delete[] one_time_indirect_index;
		}
		buffer.bwrite((const char*)two_time_indirect_index, spb.s_block_start_addr + cur_inode.i_addr[i] * spb.s_block_size, spb.s_block_size);
		delete[] two_time_indirect_index;
	}
	cur_inode.i_size = new_size;

	unsigned int length = 0;
	for (unsigned int i = 0; i < 6 && length < cur_inode.i_size; i++)
	{
		if (length + spb.s_block_size < cur_inode.i_size)
		{
			buffer.bwrite((const char*)filedata + length, spb.s_block_start_addr + cur_inode.i_addr[i] * spb.s_block_size, spb.s_block_size);
			length += spb.s_block_size;
		}
		else
		{
			buffer.bwrite((const char*)filedata + length, spb.s_block_start_addr + cur_inode.i_addr[i] * spb.s_block_size, cur_inode.i_size - length);
			length = cur_inode.i_size;
		}
	}
	for (unsigned int i = 6; i < 8 && length < cur_inode.i_size; i++)
	{
		int* one_time_indirect_index = new int[spb.s_block_size / sizeof(int)]{ 0 };
		buffer.bread((char*)one_time_indirect_index, spb.s_block_start_addr + cur_inode.i_addr[i] * spb.s_block_size, spb.s_block_size);
		for (unsigned int j = 0; j < spb.s_block_size / sizeof(int) && length < cur_inode.i_size; j++)
		{
			if (length + spb.s_block_size < cur_inode.i_size)
			{
				buffer.bwrite((const char*)filedata + length, spb.s_block_start_addr + one_time_indirect_index[j] * spb.s_block_size, spb.s_block_size);
				length += spb.s_block_size;
			}
			else
			{
				buffer.bwrite((const char*)filedata + length, spb.s_block_start_addr + one_time_indirect_index[j] * spb.s_block_size, cur_inode.i_size - length);
				length = cur_inode.i_size;
			}
		}
		delete[] one_time_indirect_index;
	}
	for (unsigned int i = 8; i < 10 && length < cur_inode.i_size; i++)
	{
		int* two_time_indirect_index = new int[spb.s_block_size / sizeof(int)]{ 0 };
		buffer.bread((char*)two_time_indirect_index, spb.s_block_start_addr + cur_inode.i_addr[i] * spb.s_block_size, spb.s_block_size);
		for (unsigned int j = 0; j < spb.s_block_size / sizeof(int) && length < cur_inode.i_size; j++)
		{
			int* one_time_indirect_index = new int[spb.s_block_size / sizeof(int)]{ 0 };
			buffer.bread((char*)one_time_indirect_index, spb.s_block_start_addr + two_time_indirect_index[j] * spb.s_block_size, spb.s_block_size);
			for (unsigned int k = 0; k < spb.s_block_size / sizeof(int) && length < cur_inode.i_size; k++)
			{
				if (length + spb.s_block_size < cur_inode.i_size)
				{
					buffer.bwrite((const char*)filedata + length, spb.s_block_start_addr + one_time_indirect_index[k] * spb.s_block_size, spb.s_block_size);
					length += spb.s_block_size;
				}
				else
				{
					buffer.bwrite((const char*)filedata + length, spb.s_block_start_addr + one_time_indirect_index[k] * spb.s_block_size, cur_inode.i_size - length);
					length = cur_inode.i_size;
				}
			}
			delete[] one_time_indirect_index;
		}
		delete[] two_time_indirect_index;
	}
	delete[] filedata;
	if (cur_inode.i_size <= 6 * spb.s_block_size)
	{
		cur_inode.i_mode &= (~(1 << 12));
	}
	else
	{
		cur_inode.i_mode |= HUGE_FILE;
	}
	cur_inode.i_atime = unsigned int(time(NULL));
	cur_inode.i_mtime = unsigned int(time(NULL));
}
void FS::_fdelete(Inode& cur_inode)
{
	unsigned int length = 0;
	for (unsigned int i = 0; i < 6 && length < cur_inode.i_size; i++)
	{
		bfree(cur_inode.i_addr[i]);
		length += spb.s_block_size;
	}
	for (unsigned int i = 6; i < 8 && length < cur_inode.i_size; i++)
	{
		int* one_time_indirect_index = new int[spb.s_block_size / sizeof(int)]{ 0 };
		buffer.bread((char*)one_time_indirect_index, spb.s_block_start_addr + cur_inode.i_addr[i] * spb.s_block_size, spb.s_block_size);
		for (unsigned int j = 0; j < spb.s_block_size / sizeof(int) && length < cur_inode.i_size; j++)
		{
			bfree(one_time_indirect_index[j]);
			length += spb.s_block_size;
		}
		bfree(cur_inode.i_addr[i]);
		delete[] one_time_indirect_index;
	}
	for (unsigned int i = 8; i < 10 && length < cur_inode.i_size; i++)
	{
		int* two_time_indirect_index = new int[spb.s_block_size / sizeof(int)]{ 0 };
		buffer.bread((char*)two_time_indirect_index, spb.s_block_start_addr + cur_inode.i_addr[i] * spb.s_block_size, spb.s_block_size);
		for (unsigned int j = 0; j < spb.s_block_size / sizeof(int) && length < cur_inode.i_size; j++)
		{
			int* one_time_indirect_index = new int[spb.s_block_size / sizeof(int)]{ 0 };
			buffer.bread((char*)one_time_indirect_index, spb.s_block_start_addr + two_time_indirect_index[j] * spb.s_block_size, spb.s_block_size);
			for (unsigned int k = 0; k < spb.s_block_size / sizeof(int) && length < cur_inode.i_size; k++)
			{
				bfree(one_time_indirect_index[k]);
				length += spb.s_block_size;
			}
			bfree(two_time_indirect_index[j]);
			delete[] one_time_indirect_index;
		}
		bfree(cur_inode.i_addr[i]);
		delete[] two_time_indirect_index;
	}
	cur_inode.i_size = 0;
	cur_inode.i_mode &= (~(1 << 12));
	cur_inode.i_atime = unsigned int(time(NULL));
	cur_inode.i_mtime = unsigned int(time(NULL));
}
void FS::_ddeleteall(Inode& cur_inode)
{
	if ((cur_inode.i_mode & (3 << 13)) != DIR_FILE)
	{
		_fdelete(cur_inode);
	}
	else
	{
		DirItem* dir_list = new DirItem[cur_inode.i_size / spb.dir_item_size]{ 0 };
		_fread(cur_inode, (char*)dir_list, 0, cur_inode.i_size);
		for (unsigned int i = 0; i < cur_inode.i_size / spb.dir_item_size; i++)
		{
			if (strcmp(dir_list[i].item_name, ".") == 0 || strcmp(dir_list[i].item_name, "..") == 0)
			{
				continue;
			}
			else
			{
				Inode sub_dir = iload(dir_list[i].inode_no);
				_ddeleteall(sub_dir);
				ifree(sub_dir);
			}
		}
		_fdelete(cur_inode);
		delete[] dir_list;
	}
}
void FS::initialize()
{
	ifstream fin;
	ofstream fout;
	fin.open(DISK_NAME, ios::in | ios::binary | ios::_Nocreate);
	if (!fin.is_open()) {

		fformat();
	}
}
File* FS::fopen(const char* dir, short uid, short gid)
{
	Inode cur_dir = iload(root_dir_no);
	vector<string> route = splitstr(dir, "/");
	for (unsigned int i = 0; i < route.size(); i++)
	{
		cur_dir = _ffind(cur_dir, route[i].data());
	}

	if ((cur_dir.i_mode & (3 << 13)) == DIR_FILE)
	{
		throw("不能打开目录文件\n");
	}

	File* fp = new File;
	Inode* ip = new Inode;
	memcpy_s(ip, sizeof(Inode), &cur_dir, sizeof(Inode));
	fp->f_uid = uid;
	fp->f_gid = gid;
	fp->f_inode = ip;
	fp->f_offset = 0;
	return fp;
}
int FS::fclose(File* fp)
{
	isave(*fp->f_inode);
	delete fp->f_inode;
	delete fp;
	return 0;
}
int FS::fread(char* buffer, int size, int count, File* fp)
{
	int file_mode;
	if (fp->f_uid == fp->f_inode->i_uid)
	{
		file_mode = 6;
	}
	else if (fp->f_gid == fp->f_inode->i_gid)
	{
		file_mode = 3;
	}
	else
	{
		file_mode = 0;
	}
	if (((fp->f_inode->i_mode >> file_mode) & 4) == 0 && fp->f_uid != 0)
	{
		throw("权限不足:缺少读权限\n");
	}
	int length;
	if (int(fp->f_offset + size * count) > fp->f_inode->i_size)
	{
		length = fp->f_inode->i_size - fp->f_offset;
	}
	else
	{
		length = size * count;
	}
	_fread(*fp->f_inode, buffer, fp->f_offset, length);
	fp->f_offset += length;
	return length/size;
}
int FS::fwrite(const char* buffer, int size, int count, File* fp)
{
	int file_mode;
	if (fp->f_uid == fp->f_inode->i_uid)
	{
		file_mode = 6;
	}
	else if (fp->f_gid == fp->f_inode->i_gid)
	{
		file_mode = 3;
	}
	else
	{
		file_mode = 0;
	}
	if (((fp->f_inode->i_mode >> file_mode) & 2) == 0 && fp->f_uid != 0)
	{
		throw("权限不足:缺少写权限\n");
	}
	_fwrite(*fp->f_inode, buffer, fp->f_offset, size * count);
	fp->f_offset += size * count;
	return count;
}
int FS::freplace(const char* buffer, int size, int count, File* fp)
{
	int file_mode;
	if (fp->f_uid == fp->f_inode->i_uid)
	{
		file_mode = 6;
	}
	else if (fp->f_gid == fp->f_inode->i_gid)
	{
		file_mode = 3;
	}
	else
	{
		file_mode = 0;
	}
	if (((fp->f_inode->i_mode >> file_mode) & 2) == 0 && fp->f_uid != 0)
	{
		throw("权限不足:缺少写权限\n");
	}
	char* data = new char[fp->f_inode->i_size + 1]{ 0 };
	_fread(*fp->f_inode, data, 0, fp->f_inode->i_size);
	_fdelete(*fp->f_inode);
	char* new_data = new char[fp->f_offset + size * count + 1]{ 0 };
	memcpy_s(new_data, fp->f_offset, data, fp->f_offset);
	memcpy_s(new_data + fp->f_offset, size * count, buffer, size * count);
	_fwrite(*fp->f_inode, new_data, 0, fp->f_offset + size * count);
	delete[] data;
	delete[] new_data;
	fp->f_offset += size * count;
	return size * count;
}
int FS::fcreate(const char* dir, short uid, short gid)
{
	vector<string> route = splitstr(dir, "/");
	string fname = route[route.size() - 1];
	if (fname.size() > spb.name_max_size)
	{
		throw("新增文件名超过最大长度\n");
	}

	Inode cur_dir = iload(root_dir_no);
	for (unsigned int i = 0; i < route.size() - 1; i++)
	{
		cur_dir = _ffind(cur_dir, route[i].data());
	}


	if ((cur_dir.i_mode & (3 << 13)) != DIR_FILE)
	{
		throw("当前路径非目录文件\n");
	}

	int file_mode;
	if (uid == cur_dir.i_uid)
	{
		file_mode = 6;
	}
	else if (gid == cur_dir.i_gid)
	{
		file_mode = 3;
	}
	else
	{
		file_mode = 0;
	}
	if (((cur_dir.i_mode >> file_mode) & 2) == 0 && uid != 0)
	{
		throw("权限不足:缺少写权限\n");
	}

	if (cur_dir.i_size >= spb.file_max_size)
	{
		throw("当前路径目录项已写满\n");
	}

	DirItem* dir_list = new DirItem[cur_dir.i_size / spb.dir_item_size]{ 0 };
	_fread(cur_dir, (char*)dir_list, 0, cur_dir.i_size);
	for (unsigned int i = 0; i < cur_dir.i_size / spb.dir_item_size; i++)
	{
		if (string(dir_list[i].item_name) == fname)
		{
			throw("当前路径下已有该文件名\n");
		}
	}
	delete[] dir_list;

	Inode new_inode = ialloc();
	new_inode.i_mode = IALLOC | NORMAL_DATA_FILE | SMALL_FILE | FILE_DEF_PERMISSION;
	new_inode.i_nlink = 1;
	new_inode.i_uid = uid;
	new_inode.i_gid = gid;
	new_inode.i_size = 0;
	new_inode.i_mtime = unsigned int(time(NULL));
	new_inode.i_atime = unsigned int(time(NULL));
	isave(new_inode);

	DirItem new_dir;
	strcpy_s(new_dir.item_name, fname.data());
	new_dir.inode_no = new_inode.i_no;
	_fwrite(cur_dir, (const char*)&new_dir, cur_dir.i_size, spb.dir_item_size);
	isave(cur_dir);

	return 0;
}
int FS::fdelete(const char* dir, short uid, short gid)
{
	vector<string> route = splitstr(dir, "/");
	string fname = route[route.size() - 1];

	Inode cur_dir = iload(root_dir_no);
	for (unsigned int i = 0; i < route.size() - 1; i++)
	{
		cur_dir = _ffind(cur_dir, route[i].data());
	}

	if ((cur_dir.i_mode & (3 << 13)) != DIR_FILE)
	{
		throw("当前路径非目录文件\n");
	}

	int file_mode;
	if (uid == cur_dir.i_uid)
	{
		file_mode = 6;
	}
	else if (gid == cur_dir.i_gid)
	{
		file_mode = 3;
	}
	else
	{
		file_mode = 0;
	}
	if (((cur_dir.i_mode >> file_mode) & 2) == 0 && uid != 0)
	{
		throw("权限不足:缺少写权限\n");
	}

	DirItem* dir_list = new DirItem[cur_dir.i_size / spb.dir_item_size]{ 0 };
	_fread(cur_dir, (char*)dir_list, 0, cur_dir.i_size);
	int fno = -1;
	int pos = -1;
	for (unsigned int i = 0; i < cur_dir.i_size / spb.dir_item_size; i++)
	{
		if (string(dir_list[i].item_name) == fname)
		{
			fno = dir_list[i].inode_no;
			pos = i;
			break;
		}
	}
	if (pos == -1)
	{
		throw("找不到该文件\n");
	}
	Inode finode = iload(fno);

	if ((finode.i_mode & (3 << 13)) == DIR_FILE)
	{
		delete[] dir_list;
		throw("该文件是目录文件，不能用fdelete删除\n");
	}

	_fdelete(finode);
	ifree(finode);

	unsigned int old_size = cur_dir.i_size;
	_fdelete(cur_dir);
	for (unsigned int i = pos; i < old_size / spb.dir_item_size - 1; i++)
	{
		dir_list[i] = dir_list[i + 1];
	}
	_fwrite(cur_dir, (const char*)dir_list, 0, old_size - spb.dir_item_size);
	delete[] dir_list;
	isave(cur_dir);
	return 0;
}
int FS::fseek(File* fp, int offset, int whence)
{
	if (whence == SEEK_SET)
	{
		fp->f_offset = offset;
	}
	else if (whence == SEEK_CUR)
	{
		fp->f_offset = fp->f_offset + offset;
	}
	else
	{
		fp->f_offset = fp->f_inode->i_size - 1 + offset;
	}
	return 0;
}
int FS::ftell(File* fp)
{
	return fp->f_offset;
}
int FS::dcreate(const char* dir, short uid, short gid)
{
	vector<string> route = splitstr(dir, "/");
	string fname = route[route.size() - 1];
	if (fname.size() > spb.name_max_size)
	{
		throw("新增目录名超过最大长度\n");
	}

	Inode cur_dir = iload(root_dir_no);
	for (unsigned int i = 0; i < route.size() - 1; i++)
	{
		cur_dir = _ffind(cur_dir, route[i].data());
	}

	if ((cur_dir.i_mode & (3 << 13)) != DIR_FILE)
	{
		throw("当前路径非目录文件\n");
	}

	int file_mode;
	if (uid == cur_dir.i_uid)
	{
		file_mode = 6;
	}
	else if (gid == cur_dir.i_gid)
	{
		file_mode = 3;
	}
	else
	{
		file_mode = 0;
	}
	if (((cur_dir.i_mode >> file_mode) & 2) == 0 && uid != 0)
	{
		throw("权限不足:缺少写权限\n");
	}

	if (cur_dir.i_size >= spb.file_max_size)
	{
		throw("当前路径目录项已写满\n");
	}

	DirItem* dir_list = new DirItem[cur_dir.i_size / spb.dir_item_size]{ 0 };
	_fread(cur_dir, (char*)dir_list, 0, cur_dir.i_size);
	for (unsigned int i = 0; i < cur_dir.i_size / spb.dir_item_size; i++)
	{
		if (string(dir_list[i].item_name) == fname)
		{
			throw("当前路径下已有该目录名\n");
		}
	}
	delete[] dir_list;

	Inode new_dir = ialloc();
	new_dir.i_mode = IALLOC | DIR_FILE | SMALL_FILE | DIR_DEF_PERMISSION;
	new_dir.i_nlink = 1;
	new_dir.i_uid = uid;
	new_dir.i_gid = gid;
	new_dir.i_size = 0;
	new_dir.i_mtime = unsigned int(time(NULL));
	new_dir.i_atime = unsigned int(time(NULL));

	DirItem* new_dir_list = new DirItem[2]{ 0 };
	strcpy_s(new_dir_list[0].item_name, ".");
	new_dir_list[0].inode_no = new_dir.i_no;
	strcpy_s(new_dir_list[1].item_name, "..");
	new_dir_list[1].inode_no = cur_dir.i_no;
	_fwrite(new_dir, (const char*)new_dir_list, 0, 2 * spb.dir_item_size);
	delete[] new_dir_list;
	isave(new_dir);

	DirItem new_item;
	strcpy_s(new_item.item_name, fname.data());
	new_item.inode_no = new_dir.i_no;
	cur_dir.i_nlink++;
	_fwrite(cur_dir, (const char*)&new_item, cur_dir.i_size, spb.dir_item_size);
	isave(cur_dir);
	return 0;
}
int FS::ddelete(const char* dir, short uid, short gid)
{
	vector<string> route = splitstr(dir, "/");
	string fname = route[route.size() - 1];

	Inode cur_dir = iload(root_dir_no);
	for (unsigned int i = 0; i < route.size() - 1; i++)
	{
		cur_dir = _ffind(cur_dir, route[i].data());
	}

	if ((cur_dir.i_mode & (3 << 13)) != DIR_FILE)
	{
		throw("当前路径非目录文件\n");
	}

	if (fname == "." || fname == "..")
	{
		throw("不能对本目录或上级目录执行删除操作\n");
	}

	int file_mode;
	if (uid == cur_dir.i_uid)
	{
		file_mode = 6;
	}
	else if (gid == cur_dir.i_gid)
	{
		file_mode = 3;
	}
	else
	{
		file_mode = 0;
	}
	if (((cur_dir.i_mode >> file_mode) & 2) == 0 && uid != 0)
	{
		throw("权限不足:缺少写权限\n");
	}

	DirItem* dir_list = new DirItem[cur_dir.i_size / spb.dir_item_size]{ 0 };
	_fread(cur_dir, (char*)dir_list, 0, cur_dir.i_size);
	int fno = -1;
	int pos = -1;
	for (unsigned int i = 0; i < int(cur_dir.i_size / spb.dir_item_size); i++)
	{
		if (string(dir_list[i].item_name) == fname)
		{
			fno = dir_list[i].inode_no;
			pos = i;
			break;
		}
	}
	if (pos == -1)
	{
		throw("找不到该目录\n");
	}
	Inode finode = iload(fno);
	if ((finode.i_mode & (3 << 13)) != DIR_FILE)
	{
		throw("该文件是非目录文件，不能用ddelete删除\n");
	}
	// 删除目录
	_ddeleteall(finode);
	ifree(finode);
	// 更新目录
	unsigned int old_size = cur_dir.i_size;
	_fdelete(cur_dir);
	cur_dir.i_nlink--;
	for (unsigned int i = pos; i < old_size / spb.dir_item_size - 1; i++)
	{
		dir_list[i] = dir_list[i + 1];
	}
	_fwrite(cur_dir, (const char*)dir_list, 0, old_size - spb.dir_item_size);
	delete[] dir_list;
	isave(cur_dir);
	return 0;
}
int FS::chmod(const char* dir, int mode, short uid, short gid)
{
	Inode cur_dir = iload(root_dir_no);
	vector<string> route = splitstr(dir, "/");
	for (unsigned int i = 0; i < route.size(); i++)
	{
		cur_dir = _ffind(cur_dir, route[i].data());
	}
	if (uid != cur_dir.i_uid && uid != 0) {
		throw("权限不足\n");
	}
	cur_dir.i_mode = (cur_dir.i_mode >> 9 << 9) | mode;	//修改权限
	isave(cur_dir);
	return 0;
}
vector<string> FS::list(const char* dir, short uid, short gid)
{
	vector<string>ret_list;
	Inode cur_dir = iload(root_dir_no);
	vector<string> route = splitstr(dir, "/");
	for (unsigned int i = 0; i < route.size(); i++)
	{
		cur_dir = _ffind(cur_dir, route[i].data());
	}
	if ((cur_dir.i_mode & (3 << 13)) != DIR_FILE)
	{
		throw("当前路径非目录文件\n");
	}

	int file_mode;
	if (uid == cur_dir.i_uid)
	{
		file_mode = 6;
	}
	else if (gid == cur_dir.i_gid)
	{
		file_mode = 3;
	}
	else
	{
		file_mode = 0;
	}

	if (((cur_dir.i_mode >> file_mode) & 4) == 0 && uid != 0)
	{
		throw("权限不足:缺少读权限\n");
	}

	DirItem* dir_list = new DirItem[cur_dir.i_size / spb.dir_item_size]{ 0 };
	_fread(cur_dir, (char*)dir_list, 0, cur_dir.i_size);
	for (unsigned int i = 0; i < cur_dir.i_size / spb.dir_item_size; i++)
	{
		Inode sub_inode = iload(dir_list[i].inode_no);

		string mode;
		if (((sub_inode.i_mode >> 13) & 3) == 0)
		{
			mode += "-";
		}
		else if (((sub_inode.i_mode >> 13) & 3) == 1)
		{
			mode += "c";
		}
		else if (((sub_inode.i_mode >> 13) & 3) == 2)
		{
			mode += "d";
		}
		else if (((sub_inode.i_mode >> 13) & 3) == 3)
		{
			mode += "b";
		}
		for (int t = 8; t >= 0; t--)
		{
			if (((sub_inode.i_mode >> t) & 1) == 1)
			{
				if (t % 3 == 2)
				{
					mode += "r";
				}
				else if (t % 3 == 1)
				{
					mode += "w";
				}
				else if (t % 3 == 0)
				{
					mode += "x";
				}
			}
			else
			{
				mode += "-";
			}
		}
		tm stdT;	//存储时间
		time_t time = sub_inode.i_mtime;
		localtime_s(&stdT, (const time_t*)&time);

		string uname;
		string gname;
		File* user;
		user = fopen("/etc/users.txt");
		char* data = new char[user->f_inode->i_size + 1]{ 0 };
		fread(data, user->f_inode->i_size, 1, user);
		string fsrd = data;
		delete[] data;
		fclose(user);
		vector<string> users;
		users = splitstr(fsrd, "\n");
		for (unsigned int i = 0; i < users.size(); i++)
		{
			vector<string>umsg = splitstr(users[i], "-");
			if (stoi(umsg[2]) == sub_inode.i_uid)
			{
				uname = umsg[0];
				break;
			}
		}

		File* group;
		group = fopen("/etc/groups.txt");
		char* gdata = new char[group->f_inode->i_size + 1]{ 0 };
		fread(gdata, group->f_inode->i_size, 1, group);
		string gstrdata = gdata;
		delete[] gdata;
		fclose(group);
		vector<string> groups;
		groups = splitstr(gstrdata, "\n");
		for (unsigned int j = 0; j < groups.size(); j++)
		{
			vector<string>gmsg = splitstr(groups[j], "-");
			if (stoi(gmsg[1]) == sub_inode.i_gid)
			{
				gname = gmsg[0];
				break;
			}
		}

		char buf[128];
		sprintf_s(buf, "%10s    %-5d    %-7s    %-7s    %d B\t%d.%d.%d %02d:%02d:%02d    %-28s\n", mode.data(), sub_inode.i_nlink, uname.data(), gname.data(), sub_inode.i_size, 1900 + stdT.tm_year, stdT.tm_mon + 1, stdT.tm_mday, stdT.tm_hour, stdT.tm_min, stdT.tm_sec, dir_list[i].item_name);
		ret_list.push_back(string(buf));
	}
	delete[] dir_list;
	return ret_list;
}
int FS::enter(const char* dir, short uid, short gid)
{
	Inode cur_dir = iload(root_dir_no);
	vector<string> route = splitstr(dir, "/");
	for (unsigned int i = 0; i < route.size(); i++)
	{
		int file_mode;
		if (uid == cur_dir.i_uid)
		{
			file_mode = 6;
		}
		else if (gid == cur_dir.i_gid)
		{
			file_mode = 3;
		}
		else
		{
			file_mode = 0;
		}
		if (((cur_dir.i_mode >> file_mode) & 1) == 0 && uid != 0)
		{
			throw("权限不足:缺少执行权限\n");
		}
		cur_dir = _ffind(cur_dir, route[i].data());
	}
	int file_mode;
	if (uid == cur_dir.i_uid)
	{
		file_mode = 6;
	}
	else if (gid == cur_dir.i_gid)
	{
		file_mode = 3;
	}
	else
	{
		file_mode = 0;
	}
	if (((cur_dir.i_mode >> file_mode) & 1) == 0 && uid != 0)
	{
		throw("权限不足:缺少执行权限\n");
	}
	if ((cur_dir.i_mode & (3 << 13)) != DIR_FILE)
	{
		throw("目标路径非目录文件\n");
	}
	return 0;
}
int FS::fformat()
{
	//创建新的磁盘卷
	ofstream fout;
	fout.open(DISK_NAME, ios::out | ios::binary);
	if (!fout.is_open()) {
		throw("磁盘映像输出文件流打开失败\n");
	}
	fout.seekp(DISK_SIZE - 1, ios::beg);
	fout.write("", sizeof(char));
	fout.close();
	//格式化superblock
	spb.s_superblock_start_addr = SUPER_BLOCK_START_ADDR;
	spb.s_superblock_size = 2 * BLOCK_SIZE;
	spb.s_inode_start_addr = spb.s_superblock_start_addr + spb.s_superblock_size;
	spb.s_inode_size = 64;
	spb.s_inode_num = (1024 - 200 - 2) * BLOCK_SIZE / spb.s_inode_size;
	spb.s_inode_fnum = spb.s_inode_num;
	spb.s_block_start_addr = spb.s_inode_start_addr + spb.s_inode_num * spb.s_inode_size;
	spb.s_block_size = BLOCK_SIZE;
	spb.s_block_num = DISK_SIZE / BLOCK_SIZE - 1024;
	spb.s_block_fnum = spb.s_block_num;

	spb.s_ninode = 0;
	for (int i = 99; i >= 0; i--)
	{
		spb.s_inode[spb.s_ninode++] = i;
	}
	spb.s_ilock = 0;

	spb.s_nfree = 1;
	memset(spb.s_free, 0, sizeof(spb.s_free));
	spb.s_free[0] = 0;
	for (int i = spb.s_block_num - 1; i >= 0; i--)
	{
		int cur_block_address = spb.s_block_start_addr + i * spb.s_block_size;
		if (spb.s_nfree == 100)
		{
			buffer.bwrite((const char*)&spb.s_nfree, cur_block_address, sizeof(spb.s_nfree));
			buffer.bwrite((const char*)spb.s_free, cur_block_address + int(sizeof(spb.s_nfree)), sizeof(spb.s_free));
			spb.s_nfree = 0;
			memset(spb.s_free, 0, sizeof(spb.s_free));
		}
		spb.s_free[spb.s_nfree++] = i;
	}

	spb.file_max_size = (6 + 128 * 2 + 128 * 128 * 2) * spb.s_block_size;
	spb.name_max_size = 28;
	spb.dir_item_size = 32;
	spb.dir_list_num = spb.s_block_size / spb.dir_item_size;
	buffer.bwrite((const char*)&spb, spb.s_superblock_start_addr, sizeof(spb));

	Inode root = ialloc();
	int block_no = balloc();
	DirItem* dir_list = new DirItem[spb.dir_list_num]{ 0 };
	strcpy_s(dir_list[0].item_name, ".");
	dir_list[0].inode_no = root.i_no;
	buffer.bwrite((const char*)dir_list, spb.s_block_start_addr + block_no * spb.s_block_size, spb.s_block_size);
	delete[] dir_list;

	root.i_mode = IALLOC | DIR_FILE | SMALL_FILE | DIR_DEF_PERMISSION;
	root.i_nlink = 1;
	root.i_uid = 0;
	root.i_gid = 0;
	root.i_size = spb.dir_item_size;
	root.i_addr[0] = block_no;
	root.i_atime = unsigned int(time(NULL));
	root.i_mtime = unsigned int(time(NULL));
	isave(root);

	dcreate("/bin");
	dcreate("/etc");
	dcreate("/home");
	dcreate("/dev");

	fcreate("/etc/users.txt");
	File* user = fopen("/etc/users.txt");
	string users_data = "root-root-0-0\n";//name-passwd-uid-gid
	fwrite(users_data.data(), users_data.length(), 1, user);
	fclose(user);
	chmod("/etc/users.txt", 0660);

	fcreate("/etc/groups.txt");
	File* group = fopen("/etc/groups.txt");
	string groups_data = "root-0-0\n";//name-gid-uid,uid...
	fwrite(groups_data.data(), groups_data.length(), 1, group);
	fclose(group);

	buffer.bwrite((const char*)&spb, spb.s_superblock_start_addr, sizeof(spb));
	return 0;
}
SuperBlock FS::getSpb()
{
	return spb;
}
FS::FS()
{
	initialize();
	buffer.bread((char*)&spb, SUPER_BLOCK_START_ADDR, sizeof(SuperBlock));
}
FS::~FS()
{
	buffer.bwrite((const char*)&spb, spb.s_superblock_start_addr, sizeof(SuperBlock));
}
