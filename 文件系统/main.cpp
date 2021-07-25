#include <iostream>
#include "filesystem.h"
using namespace std;
class editor
{
protected:
	unsigned short width, height;
	HANDLE handle;							//句柄  
	CONSOLE_SCREEN_BUFFER_INFO scr_info;	//窗口缓冲区信息结构体
	COORD pos;								//坐标结构体
	vector<vector<char>> context;
	unsigned short start_line;
	void printContext();
	void printPos();
public:
	editor();
	~editor();
	string run(const string data);
};
editor::editor()
{
	system("cls");
	handle = GetStdHandle(STD_OUTPUT_HANDLE);			//获得标准输出设备句柄  
	GetConsoleScreenBufferInfo(handle, &scr_info);		//获取窗口信息
	width = scr_info.srWindow.Right - scr_info.srWindow.Left + 1;
	height = scr_info.srWindow.Bottom - scr_info.srWindow.Top + 1;
	pos = { 0,0 };
	SetConsoleCursorPosition(handle, pos);
	start_line = 0;
}
editor::~editor()
{
	system("cls");
}
void editor::printContext()
{
	GetConsoleScreenBufferInfo(handle, &scr_info);		//获取窗口信息
	COORD oldpos = scr_info.dwCursorPosition;
	for (short i = start_line; i < start_line + height - 1; i++)
	{
		if (unsigned int(i) < context.size())
		{
			for (short j = 0; j < width; j++)
			{
				SetConsoleCursorPosition(handle, { j ,i - start_line });
				if (unsigned int(j) < context[i].size())
				{
					if (context[i][j] == '\n')
					{
						printf("%c", char(270));
					}
					else
					{
						printf("%c", context[i][j]);
					}
				}
				else
				{
					printf("%c", ' ');
				}
			}
		}
		else
		{
			for (short j = 0; j < width; j++)
			{
				SetConsoleCursorPosition(handle, { j ,i - start_line });
				printf("%c", ' ');
			}
		}
	}
	SetConsoleCursorPosition(handle, oldpos);
}
void editor::printPos()
{
	GetConsoleScreenBufferInfo(handle, &scr_info);		//获取窗口信息
	COORD oldpos = scr_info.dwCursorPosition;
	SetConsoleCursorPosition(handle, { 0,height - 1 });
	for (short i = 0; i < width; i++)
	{
		printf("%c", ' ');
	}
	SetConsoleCursorPosition(handle, { 0,height - 1 });
	printf("[行:%d,列:%d]", start_line + pos.Y + 1, pos.X + 1);
	SetConsoleCursorPosition(handle, oldpos);
}
string editor::run(const string data)
{
	//转换字符串
	vector<char> line;
	for (unsigned int pos = 0; pos < data.size(); pos++)
	{
		if (line.size() < width)
		{
			if (data[pos] == '\n')
			{
				line.push_back(data[pos]);
				context.push_back(line);
				line.clear();
			}
			else
			{
				line.push_back(data[pos]);
			}
		}
		else
		{
			context.push_back(line);
			line.clear();
			if (data[pos] == '\n')
			{
				line.push_back(data[pos]);
				context.push_back(line);
				line.clear();
			}
			else
			{
				line.push_back(data[pos]);
			}
		}
	}
	if (line.size() < width)
	{
		line.push_back('#');
	}
	else
	{
		context.push_back(line);
		line.clear();
		line.push_back('#');
	}
	if (line.size() > 0)
	{
		context.push_back(line);
	}
	//打印内容
	printContext();
	printPos();
	//编辑器主循环
	unsigned char ch;
	while (true)
	{
		if (unsigned int(pos.X) > context[start_line + pos.Y].size() - 1)
		{
			pos.X = short(context[start_line + pos.Y].size() - 1);
		}
		SetConsoleCursorPosition(handle, pos);
		ch = _getch();
		if (ch == 27)//ESC
		{
			break;
		}
		else if (ch == 224)
		{
			ch = _getch();
			if (ch == 72)//UP
			{
				pos.Y--;
				if (pos.Y < 0)
				{
					pos.Y = 0;
					if (start_line > 0)
					{
						start_line--;
						printContext();
					}
				}
				SetConsoleCursorPosition(handle, pos);

			}
			else if (ch == 80)//DOWN
			{
				pos.Y++;
				if (unsigned int(start_line + pos.Y) > context.size() - 1)
				{
					pos.Y = short(context.size()) - 1 - start_line;
				}
				else if (pos.Y > height - 2)
				{
					pos.Y = height - 2;
					start_line++;
					printContext();
				}
				SetConsoleCursorPosition(handle, pos);
			}
			else if (ch == 75)//LEFT
			{
				pos.X--;
				if (pos.X < 0)
				{
					pos.X = 0;
				}
				SetConsoleCursorPosition(handle, pos);
			}
			else if (ch == 77)//RIGHT
			{
				pos.X++;
				if (pos.X > width - 1)
				{
					pos.X = width - 1;
				}
				SetConsoleCursorPosition(handle, pos);
			}
			printPos();
		}
		else if (ch == '\b')
		{
			pos.X--;
			if (pos.X < 0)
			{
				pos.Y--;
				if (pos.Y < 0)
				{
					if (start_line > 0)
					{
						start_line--;
						pos.X = width - 1;
						pos.Y = 0;
					}
					else
					{
						pos.X = 0;
						pos.Y = 0;
						continue;
					}
				}
				else
				{
					pos.X = width - 1;
				}
			}
			if (unsigned int(pos.X) > context[start_line + pos.Y].size() - 1)
			{
				pos.X = short(context[start_line + pos.Y].size() - 1);
			}
			vector<vector<char>> new_context;
			vector<char> new_line;
			for (unsigned short i = 0; i < context.size(); i++)
			{
				for (unsigned short j = 0; j < context[i].size(); j++)
				{
					if ((i == (start_line + pos.Y)) && (j == pos.X))
					{
						continue;
					}
					if (new_line.size() < width)
					{
						if (context[i][j] == '\n')
						{
							new_line.push_back(context[i][j]);
							new_context.push_back(new_line);
							new_line.clear();
						}
						else
						{
							new_line.push_back(context[i][j]);
						}
					}
					else
					{
						new_context.push_back(new_line);
						new_line.clear();
						if (context[i][j] == '\n')
						{
							new_line.push_back(context[i][j]);
							new_context.push_back(new_line);
							new_line.clear();
						}
						else
						{
							new_line.push_back(context[i][j]);
						}
					}
				}
			}
			if (new_line.size() > 0)
			{
				new_context.push_back(new_line);
			}
			context = new_context;
			SetConsoleCursorPosition(handle, pos);
			printContext();
			printPos();
		}
		else
		{
			vector<vector<char>> new_context;
			vector<char> new_line;
			for (unsigned short i = 0; i < context.size(); i++)
			{
				for (unsigned short j = 0; j < context[i].size(); j++)
				{
					if ((i == (start_line + pos.Y)) && (j == pos.X))
					{
						if (new_line.size() < width)
						{
							if (ch == '\r')
							{
								new_line.push_back('\n');
								new_context.push_back(new_line);
								new_line.clear();
							}
							else
							{
								new_line.push_back(ch);
							}
						}
						else
						{
							new_context.push_back(new_line);
							new_line.clear();
							if (ch == '\r')
							{
								new_line.push_back('\n');
								new_context.push_back(new_line);
								new_line.clear();
							}
							else
							{
								new_line.push_back(ch);
							}
						}
					}
					if (new_line.size() < width)
					{
						if (context[i][j] == '\n')
						{
							new_line.push_back(context[i][j]);
							new_context.push_back(new_line);
							new_line.clear();
						}
						else
						{
							new_line.push_back(context[i][j]);
						}
					}
					else
					{
						new_context.push_back(new_line);
						new_line.clear();
						if (context[i][j] == '\n')
						{
							new_line.push_back(context[i][j]);
							new_context.push_back(new_line);
							new_line.clear();
						}
						else
						{
							new_line.push_back(context[i][j]);
						}
					}
				}
			}
			if (new_line.size() > 0)
			{
				new_context.push_back(new_line);
			}
			context = new_context;

			if (ch == '\r')
			{
				pos.X = 0;
				pos.Y++;
				if (unsigned int(start_line + pos.Y) > context.size() - 1)
				{
					pos.Y = short(context.size()) - 1 - start_line;
				}
				else if (pos.Y > height - 2)
				{
					pos.Y = height - 2;
					start_line++;
				}
			}
			else {
				pos.X++;
				if (pos.X > width - 1)
				{
					pos.Y++;
					if (unsigned int(start_line + pos.Y) > context.size() - 1)
					{
						pos.X = width - 1;
						pos.Y = short(context.size()) - 1 - start_line;
					}
					else if (pos.Y > height - 2)
					{
						start_line++;
						pos.X = 0;
						pos.Y = height - 2;
					}
					else
					{
						pos.X = 0;
					}
				}
			}
			SetConsoleCursorPosition(handle, pos);
			printContext();
			printPos();
		}
	}

	string newdata;
	for (unsigned short i = 0; i < context.size(); i++)
	{
		for (unsigned short j = 0; j < context[i].size(); j++)
		{
			newdata += context[i][j];
		}
	}
	newdata = newdata.substr(0, newdata.length() - 1);

	return newdata;
}
class FSui
{
protected:
	FS fs;
	bool is_login;
	int cur_uid;
	string cur_user;
	int cur_gid;
	string cur_group;
	string cur_dir;
	string cur_host;
	void initialize();
	void format();
	void mkdir(const char* name);
	void cd(const char* name);
	void ls();
	void rmdir(const char* name);
	void touch(const char* name);
	void chmod(const char* name, int mode);
	void rm(const char* name);
	void login(string name, string passwd);
	void useradd(string uname, string gname, string passwd);
	void userdel(string uname);
	void groupadd(string uname);
	void groupdel(string uname);
	void df();
	void show(const char* name);
	void edit(const char* name);
	void win2fs(const char* win_fname, const char* fs_fname);
	void fs2win(const char* fs_fname, const char* win_fname);
	void help();
public:
	FSui();
	void run();
};
void FSui::initialize()
{
	is_login = false;
	cur_uid = -1;
	cur_user = "";
	cur_gid = -1;
	cur_group = "";
	cur_dir = "";
	DWORD size = 0;
	wstring wstr;
	GetComputerName(NULL, &size); //得到电脑名称长度
	wchar_t* name = new wchar_t[size];
	if (GetComputerName(name, &size))
	{
		wstr = name;
	}
	delete[] name;
	cur_host = wstring2string(wstr);
}
void FSui::format()
{
	printf("文件系统正在格式化......\n");
	fs.fformat();
	initialize();
	printf("文件系统格式化完成......\n");
	system("pause");
	system("cls");

}
void FSui::mkdir(const char* name)
{
	string new_dir = string(name);
	if (new_dir.find("/") != string::npos)
	{
		throw("参数中不能包含多级目录\n");
	}
	string dir = cur_dir + "/" + new_dir;
	fs.dcreate(dir.data(), cur_uid, cur_gid);
}
void FSui::cd(const char* name)
{
	string sub_dir = string(name);
	if (sub_dir.find("/") != string::npos)
	{
		throw("参数中不能包含多级目录\n");
	}
	string dir = cur_dir + "/" + sub_dir;
	if (!fs.enter(dir.data(), cur_uid, cur_gid))
	{
		if (sub_dir == ".") {}
		else if (sub_dir == "..")
		{
			size_t pos = cur_dir.find_last_of('/');
			cur_dir = cur_dir.substr(0, pos);
		}
		else
		{
			cur_dir = dir;
		}
	}
	else
	{
		throw("无法进入指定目录\n");
	}
}
void FSui::ls()
{
	vector<string>list = fs.list(cur_dir.data(), cur_uid, cur_gid);
	for (unsigned int i = 0; i < list.size(); i++)
	{
		cout << list[i];
	}
}
void FSui::rmdir(const char* name)
{
	string new_dir = string(name);
	if (new_dir.find("/") != string::npos)
	{
		throw("参数中不能包含多级目录\n");
	}
	string dir = cur_dir + "/" + new_dir;
	fs.ddelete(dir.data(), cur_uid, cur_gid);
}
void FSui::touch(const char* name)
{
	string new_dir = string(name);
	if (new_dir.find("/") != string::npos)
	{
		throw("参数中不能包含多级目录\n");
	}
	string dir = cur_dir + "/" + new_dir;
	fs.fcreate(dir.data(), cur_uid, cur_gid);
}
void FSui::chmod(const char* name, int mode)
{
	string new_dir = string(name);
	if (new_dir.find("/") != string::npos)
	{
		throw("参数中不能包含多级目录\n");
	}
	string dir = cur_dir + "/" + new_dir;
	fs.chmod(dir.data(), mode, cur_uid, cur_gid);
}
void FSui::rm(const char* name)
{
	string new_dir = string(name);
	if (new_dir.find("/") != string::npos)
	{
		throw("参数中不能包含多级目录\n");
	}
	string dir = cur_dir + "/" + new_dir;
	fs.fdelete(dir.data(), cur_uid, cur_gid);
}
void FSui::login(string name, string passwd)
{
	File* user = fs.fopen("/etc/users.txt");
	char* udata = new char[user->f_inode->i_size + 1]{ 0 };
	fs.fread(udata, user->f_inode->i_size, 1, user);
	string ustrdata = udata;
	delete[] udata;
	fs.fclose(user);
	vector<string> users;
	users = splitstr(ustrdata, "\n");
	for (unsigned int i = 0; i < users.size(); i++)
	{
		vector<string>umsg = splitstr(users[i], "-");
		if (umsg[0] == name && umsg[1] == passwd)
		{
			is_login = true;
			cur_uid = stoi(umsg[2]);
			cur_user = name;
			cur_gid = stoi(umsg[3]);

			File* group = fs.fopen("/etc/groups.txt");
			char* gdata = new char[group->f_inode->i_size + 1]{ 0 };
			fs.fread(gdata, group->f_inode->i_size, 1, group);
			string gstrdata = gdata;
			delete[] gdata;
			fs.fclose(group);
			vector<string> groups;
			groups = splitstr(gstrdata, "\n");
			for (unsigned int j = 0; j < groups.size(); j++)
			{
				vector<string>gmsg = splitstr(groups[j], "-");
				if (gmsg[1] == umsg[3])
				{
					cur_group = gmsg[0].data();
					break;
				}
			}

			cur_dir = "";
			break;
		}
	}
}
void FSui::useradd(string uname, string gname, string passwd)
{
	if (cur_uid != 0)
	{
		throw("权限不足\n");
	}

	File* user = fs.fopen("/etc/users.txt");
	char* data = new char[user->f_inode->i_size + 1]{ 0 };
	fs.fread(data, user->f_inode->i_size, 1, user);
	string ustrdata = data;
	delete[] data;
	vector<string> users;
	users = splitstr(ustrdata, "\n");
	int next_uid = 0;
	for (unsigned int i = 0; i < users.size(); i++)
	{
		vector<string>umsg = splitstr(users[i], "-");
		if (umsg[0] == uname)
		{
			fs.fclose(user);
			throw("该用户已存在\n");
		}
		if (stoi(umsg[2]) >= next_uid)
		{
			next_uid = stoi(umsg[2]) + 1;
		}
	}

	int gid = -1;
	File* group = fs.fopen("/etc/groups.txt");
	char* gdata = new char[group->f_inode->i_size + 1]{ 0 };
	fs.fread(gdata, group->f_inode->i_size, 1, group);
	string gstrdata = gdata;
	delete[] gdata;
	vector<string> groups;
	groups = splitstr(gstrdata, "\n");
	for (unsigned int i = 0; i < groups.size(); i++)
	{
		vector<string>gmsg = splitstr(groups[i], "-");
		if (gmsg[0] == gname)
		{
			gid = stoi(gmsg[1]);
			groups[i] += (gmsg.size() == 2 ? to_string(next_uid) : "," + to_string(next_uid));
			break;
		}
	}

	if (gid != -1)
	{
		string new_gdata;
		for (unsigned int i = 0; i < groups.size(); i++)
		{
			new_gdata += groups[i] + "\n";
		}
		fs.fseek(group, 0, SEEK_SET);
		fs.fwrite(new_gdata.data(), new_gdata.length(), 1, group);
		ustrdata += uname + "-" + passwd + "-" + to_string(next_uid) + "-" + to_string(gid) + "\n";
		fs.fseek(user, 0, SEEK_SET);
		fs.fwrite(ustrdata.data(), ustrdata.length(), 1, user);
	}
	else
	{
		fs.fclose(user);
		fs.fclose(group);
		throw("用户组不存在\n");
	}

	fs.fclose(user);
	fs.fclose(group);
}
void FSui::userdel(string uname)
{
	if (cur_uid != 0)
	{
		throw("权限不足\n");
	}
	if (uname == "root")
	{
		throw("不能删除root用户\n");
	}

	File* user = fs.fopen("/etc/users.txt");
	char* data = new char[user->f_inode->i_size + 1]{ 0 };
	fs.fread(data, user->f_inode->i_size, 1, user);
	string ustrdata = data;
	delete[] data;
	vector<string> users;
	users = splitstr(ustrdata, "\n");
	string new_udata;
	int uid = -1;
	int gid = -1;
	for (unsigned int i = 0; i < users.size(); i++)
	{
		vector<string>umsg = splitstr(users[i], "-");
		if (umsg[0] == uname)
		{
			uid = stoi(umsg[2]);
			gid = stoi(umsg[3]);
		}
		else
		{
			new_udata += users[i] + "\n";
		}
	}
	if (uid == -1)
	{
		fs.fclose(user);
		throw("该用户不存在\n");
	}
	fs.fseek(user, 0, SEEK_SET);
	fs.freplace(new_udata.data(), new_udata.length(), 1, user);
	fs.fclose(user);


	File* group = fs.fopen("/etc/groups.txt");
	char* gdata = new char[group->f_inode->i_size + 1]{ 0 };
	fs.fread(gdata, group->f_inode->i_size, 1, group);
	string gstrdata = gdata;
	delete[] gdata;
	vector<string> groups;
	groups = splitstr(gstrdata, "\n");
	string new_gdata;
	for (unsigned int i = 0; i < groups.size(); i++)
	{
		vector<string>gmsg = splitstr(groups[i], "-");
		if (stoi(gmsg[1]) == gid)
		{
			vector<string>uids = splitstr(gmsg[2], ",");
			string newuids;
			for (unsigned int j = 0; j < uids.size(); j++)
			{
				if (stoi(uids[j]) != uid)
				{
					newuids += (newuids.length() == 0 ? uids[j] : "," + uids[j]);
				}
			}
			new_gdata += gmsg[0] + "-" + gmsg[1] + "-" + newuids + "\n";
		}
		else
		{
			new_gdata += groups[i] + "\n";
		}
	}

	fs.fseek(group, 0, SEEK_SET);
	fs.freplace(new_gdata.data(), new_gdata.length(), 1, group);
	fs.fclose(group);
}
void FSui::groupadd(string gname)
{
	if (cur_uid != 0)
	{
		throw("权限不足\n");
	}
	File* group = fs.fopen("/etc/groups.txt");
	char* gdata = new char[group->f_inode->i_size + 1]{ 0 };
	fs.fread(gdata, group->f_inode->i_size, 1, group);
	string gstrdata = gdata;
	delete[] gdata;
	vector<string> groups;
	groups = splitstr(gstrdata, "\n");
	int next_gid = 0;
	for (unsigned int i = 0; i < groups.size(); i++)
	{
		vector<string>gmsg = splitstr(groups[i], "-");
		if (gmsg[0] == gname)
		{
			fs.fclose(group);
			throw("该用户组已存在\n");
		}
		if (stoi(gmsg[1]) >= next_gid)
		{
			next_gid = stoi(gmsg[1]) + 1;
		}
	}
	gstrdata += gname + "-" + to_string(next_gid) + "-\n";
	fs.fseek(group, 0, SEEK_SET);
	fs.fwrite(gstrdata.data(), gstrdata.length(), 1, group);
	fs.fclose(group);
}
void FSui::groupdel(string gname)
{
	if (cur_uid != 0)
	{
		throw("权限不足\n");
	}
	if (gname == "root")
	{
		throw("不能删除root用户组\n");
	}

	File* group = fs.fopen("/etc/groups.txt");
	char* gdata = new char[group->f_inode->i_size + 1]{ 0 };
	fs.fread(gdata, group->f_inode->i_size, 1, group);
	string gstrdata = gdata;
	delete[] gdata;
	vector<string> groups;
	groups = splitstr(gstrdata, "\n");
	string new_gdata;
	vector<string>uids;
	bool flag = false;
	for (unsigned int i = 0; i < groups.size(); i++)
	{
		vector<string>gmsg = splitstr(groups[i], "-");
		if (gmsg[0] == gname)
		{
			flag = true;
			uids = splitstr(gmsg[2], ",");
		}
		else
		{
			new_gdata += groups[i] + "\n";
		}
	}
	if (!flag)
	{
		fs.fclose(group);
		throw("该用户组不存在\n");
	}
	fs.fseek(group, 0, SEEK_SET);
	fs.freplace(new_gdata.data(), new_gdata.length(), 1, group);
	fs.fclose(group);

	File* user = fs.fopen("/etc/users.txt");
	char* data = new char[user->f_inode->i_size + 1]{ 0 };
	fs.fread(data, user->f_inode->i_size, 1, user);
	string ustrdata = data;
	delete[] data;
	vector<string> users;
	users = splitstr(ustrdata, "\n");
	string new_udata;
	for (unsigned int i = 0; i < users.size(); i++)
	{
		vector<string>umsg = splitstr(users[i], "-");
		if (find(uids.begin(), uids.end(), umsg[2]) == uids.end())
		{
			new_udata += users[i] + "\n";
		}
	}
	fs.fseek(user, 0, SEEK_SET);
	fs.freplace(new_udata.data(), new_udata.length(), 1, user);
	fs.fclose(user);
}
void FSui::df()
{
	SuperBlock superblock = fs.getSpb();
	int size = superblock.s_block_num * superblock.s_block_size;
	int used = (superblock.s_block_num - superblock.s_block_fnum) * superblock.s_block_size;
	int avaial = superblock.s_block_fnum * superblock.s_block_size;
	double use = 100 * double((double)superblock.s_block_num - (double)superblock.s_block_fnum) / double(superblock.s_block_num);
	int inode = superblock.s_inode_num;
	int iavaial = superblock.s_inode_fnum;
	printf("%-10s    %-10s    %-10s    %-10s     %-10s      %-10s    %-10s    %s\n", "Filesystem", "Size", "Used", "Avaial", "Use%", "Inode", "Ifree", "Monted on");
	printf("%-10s    %-10d    %-10d    %-10d     %-10.2f      %-10d    %-10d    %s\n", "FS", size, used, avaial, use, inode, iavaial, "/");
}
void FSui::show(const char* name)
{
	string fname = string(name);
	if (fname.find("/") != string::npos)
	{
		throw("参数中不能包含多级目录\n");
	}
	string dir = cur_dir + "/" + fname;
	File* fp = fs.fopen(dir.data(), cur_uid, cur_gid);
	char* fdata = new char[fp->f_inode->i_size + 1]{ 0 };
	fs.fread(fdata, fp->f_inode->i_size, 1, fp);
	cout << fdata << endl;
	fs.fclose(fp);
	delete[] fdata;
}
void FSui::edit(const char* name)
{
	string fname = string(name);
	if (fname.find("/") != string::npos)
	{
		throw("参数中不能包含多级目录\n");
	}
	string dir = cur_dir + "/" + fname;
	File* fp = fs.fopen(dir.data(), cur_uid, cur_gid);
	char* fdata = new char[fp->f_inode->i_size + 1]{ 0 };
	fs.fread(fdata, fp->f_inode->i_size, 1, fp);
	editor e;
	string newdata = e.run(string(fdata));

	fs.fseek(fp, 0, SEEK_SET);
	fs.freplace(newdata.data(), newdata.length(), 1, fp);
	fs.fclose(fp);
	delete[] fdata;
}
void FSui::win2fs(const char* win_fname, const char* fs_fname)
{
	if (string(fs_fname).find("/") != string::npos)
	{
		throw("参数中不能包含多级目录\n");
	}
	string fs_dir = cur_dir + "/" + string(fs_fname);

	FILE* winfp;
	fopen_s(&winfp, win_fname, "rb");
	if (winfp == NULL)
	{
		throw("打开windows文件失败\n");
	}
	fseek(winfp, 0, SEEK_END);
	unsigned int file_size = ftell(winfp);
	fseek(winfp, 0, SEEK_SET);
	char* data = new char[file_size + 1]{ 0 };
	fread(data, file_size, 1, winfp);
	fclose(winfp);
	File* fsfp = fs.fopen(fs_dir.data(), cur_uid, cur_gid);
	fs.freplace(data, file_size, 1, fsfp);
	fs.fclose(fsfp);
	delete[] data;
}
void FSui::fs2win(const char* fs_fname, const char* win_fname)
{

	if (string(fs_fname).find("/") != string::npos)
	{
		throw("参数中不能包含多级目录\n");
	}
	string fs_dir = cur_dir + "/" + string(fs_fname);

	File* fsfp = fs.fopen(fs_dir.data(), cur_uid, cur_gid);
	unsigned file_size = fsfp->f_inode->i_size;
	char* data = new char[file_size + 1]{ 0 };
	fs.fread(data, file_size, 1, fsfp);
	fs.fclose(fsfp);
	FILE* winfp;
	fopen_s(&winfp, win_fname, "wb");
	if (winfp == NULL)
	{
		throw("打开windows文件失败\n");
	}
	fwrite(data, file_size, 1, winfp);
	fclose(winfp);
	delete[] data;
}
void FSui::help()
{
	printf("format                                      - 格式化文件系统\n");
	printf("mkdir      <dir name>                       - 创建目录\n");
	printf("cd         <dir name>                       - 进入目录\n");
	printf("ls                                          - 显示当前目录清单\n");
	printf("rmdir      <dir name>                       - 删除目录\n");
	printf("touch      <file name>                      - 创建新文件\n");
	printf("chmod      <file name/dir name> <mode(OTC)> - 修改文件或目录权限\n");
	printf("rm         <file name>                      - 删除文件\n");
	printf("login      <user name>                      - 用户登录\n");
	printf("logout                                      - 用户注销\n");
	printf("useradd    <user name> <group name>         - 添加用户\n");
	printf("userdel    <user name>                      - 删除用户\n");
	printf("groupadd   <group name>                     - 添加用户组\n");
	printf("groupdel   <group name>                     - 删除用户组\n");
	printf("df                                          - 查看磁盘使用情况\n");
	printf("show       <file name>                      - 打印文件内容\n");
	printf("vi         <file name>                      - 用编辑器打开文件\n");
	printf("win2fs	   <win file name> <fs file name>   - 将Windows文件内容复制到FS文件系统文件\n");
	printf("fs2win	   <fs file name> <win file name>   - 将FS文件系统文件内容复制到Windows文件\n");
	printf("help                                        - 显示命令清单\n");
	printf("cls                                         - 清屏\n");
	printf("exit                                        - 退出系统\n");
}
FSui::FSui()
{
	initialize();
}
void FSui::run()
{
	cout <<
		"\
________  ________										\n\
|   ____| |   ____|    FILE SYSTEM						\n\
|  |____  |  |____     verson " << VERSION << "			\n\
|   ____| |____   |    Copyright 2021					\n\
|  |	   ____|  |    https://github.com/lingbai-kong	\n\
|__|	  |_______|    ALL Rights Reserved				\n\
\n";
	while (true)
	{
		cout << "[" << cur_user << "@" << cur_host << ":" << ROOT_DIR_SYMBOL << cur_dir << "]" << (cur_user == "root" ? "#" : "$") << " ";
		string buf;
		vector<string>args;
		getline(cin, buf);
		args = splitstr(buf, " ");
		try {
			if (args.size() <= 0)
			{
				continue;
			}
			else if (is_login)
			{
				if (args[0] == "logout")
				{
					initialize();
				}
				else if (args[0] == "exit")
				{
					return;
				}
				else if (args[0] == "mkdir" && args.size() == 2)
				{
					mkdir(args[1].data());
				}
				else if (args[0] == "cd" && args.size() == 2)
				{
					cd(args[1].data());
				}
				else if (args[0] == "ls")
				{
					ls();
				}
				else if (args[0] == "rmdir" && args.size() == 2)
				{
					rmdir(args[1].data());
				}
				else if (args[0] == "touch" && args.size() == 2)
				{
					touch(args[1].data());
				}
				else if (args[0] == "chmod" && args.size() == 3 && args[2].size() == 3)
				{
					int master = stoi(args[2].substr(0, 1));
					int group = stoi(args[2].substr(1, 1));
					int others = stoi(args[2].substr(2, 1));
					if (master >= 0 && master <= 7 && group >= 0 && group <= 7 && others >= 0 && others <= 7)
					{
						int mode = (master << 6) + (group << 3) + others;
						chmod(args[1].data(), mode);
					}
					else
					{
						cout << "无效的权限\n";
					}
				}
				else if (args[0] == "rm" && args.size() == 2)
				{
					rm(args[1].data());
				}
				else if (args[0] == "useradd" && args.size() == 3)
				{
					cout << "password: ";
					char password[256] = { 0 };
					char ch = '\0';
					int pos = 0;
					while ((ch = _getch()) != '\r' && pos < 255)
					{
						if (ch != 8)//不是回撤就录入
						{
							password[pos] = ch;
							putchar('*');//并且输出*号
							pos++;
						}
						else
						{
							putchar('\b');//这里是删除一个，我们通过输出回撤符 /b，回撤一格，
							putchar(' ');//再显示空格符把刚才的*给盖住，
							putchar('\b');//然后再 回撤一格等待录入。
							pos--;
						}
					}
					password[pos] = '\0';
					putchar('\n');
					useradd(args[1], args[2], string(password));
				}
				else if (args[0] == "userdel" && args.size() == 2)
				{
					userdel(args[1]);
				}
				else if (args[0] == "groupadd" && args.size() == 2)
				{
					groupadd(args[1]);
				}
				else if (args[0] == "groupdel" && args.size() == 2)
				{
					groupdel(args[1]);
				}
				else if (args[0] == "df")
				{
					df();
				}
				else if (args[0] == "show" && args.size() == 2)
				{
					show(args[1].data());
				}
				else if (args[0] == "vi" && args.size() == 2)
				{
					edit(args[1].data());
				}
				else if (args[0] == "format")
				{
					cout << "确定要进行格式化？[y] ";
					string ck;
					getline(cin, ck);
					if (ck == "y" || ck == "Y")
					{
						format();
					}
				}
				else if (args[0] == "win2fs" && args.size() == 3)
				{
					win2fs(args[1].data(), args[2].data());
				}
				else if (args[0] == "fs2win" && args.size() == 3)
				{
					fs2win(args[1].data(), args[2].data());
				}
				else if (args[0] == "cls")
				{
					system("cls");
				}
				else if (args[0] == "help")
				{
					help();
				}
				else
				{
					cout << "无效的命令!\n";
				}
			}
			else
			{
				if (args[0] == "login" && args.size() == 2)
				{
					cout << "password: ";
					char password[256] = { 0 };
					char ch = '\0';
					int pos = 0;
					while ((ch = _getch()) != '\r' && pos < 255)
					{
						if (ch != 8)//不是回撤就录入
						{
							password[pos] = ch;
							putchar('*');//并且输出*号
							pos++;
						}
						else
						{
							putchar('\b');//这里是删除一个，我们通过输出回撤符 /b，回撤一格，
							putchar(' ');//再显示空格符把刚才的*给盖住，
							putchar('\b');//然后再 回撤一格等待录入。
							pos--;
						}
					}
					password[pos] = '\0';
					putchar('\n');
					login(args[1], string(password));
				}
				else if (args[0] == "exit")
				{
					return;
				}
				else if (args[0] == "format")
				{
					cout << "确定要进行格式化？[y] ";
					string ck;
					getline(cin, ck);
					if (ck == "y" || ck == "Y")
					{
						format();
					}
				}
				else if (args[0] == "help")
				{
					help();
				}
				else
				{
					cout << "无效的命令!\n";
				}
			}
		}
		catch (const char* errMsg) {
			cout << errMsg;
		}
	}
}
int main()
{
	try {
		FSui app;
		app.run();
	}
	catch (...)
	{
		cout << "程序运行错误\n";
	}
	return 0;
}
