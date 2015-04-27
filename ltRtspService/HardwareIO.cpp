#include "stdafx.h"
#include "HardwareIO.h"

using namespace std;
HardwareIO*
HardwareIO::GetInstance()
{
	static HardwareIO* hard = NULL;
	if (hard == NULL)
	{
		hard = new HardwareIO;
		return hard;
	}
	return hard;
}

HardwareIO::HardwareIO()
{

}

HardwareIO::~HardwareIO()
{
	map<int, FileNode*>::iterator it = FileManager.begin();
	while (it != FileManager.end())
	{
		delete it->second;
	}
}


int 
HardwareIO::MakeFileNode(string& filename)
{
	map<int, FileNode*>::iterator it = FileManager.begin();
	while(it != FileManager.end())
	{
		if (it->second->_filename == filename)
		{
			return it->first;
		}
	}

	FILE* file;
	errno_t error = fopen_s(&file, filename.c_str(), "rb");
	if (file == NULL)
	{
		perror(filename.c_str());
		return 0xffff;
	}
	FileNode* newnode = new FileNode();
	newnode->_filename = filename;
	newnode->_filefd = file;
	newnode->_curpos = 0;
	pair<map<int, FileNode*>::iterator, bool> InsertPair;
	InsertPair = FileManager.insert(pair<int, FileNode*>(FileManager.size(), newnode));
	if (InsertPair.second)
	{
		return FileManager.size() - 1;
	}
	return 0xffff;
}


buf_share_ptr 
HardwareIO::GetBufferFormFile(int index, uint64_t pos, unsigned sizeget)
{
	//_CrtDbgBreak(); ��Ҫ�ж�index���Ƿ����
	/*
	if (index >= 0 && index < FileManager.size())
	{
	}
	*/
	FileNode *tmp = FileManager[index];

	if ((tmp->_curpos > pos && tmp->_curpos - pos < pos) || (tmp->_curpos < pos))
	{
		fseek(tmp->_filefd, tmp->_curpos - pos, SEEK_CUR);
	}
	else
	{
		fseek(tmp->_filefd, pos, SEEK_SET);
	}
	int readsize;
	//������ʱ����Ϊ10KB������һ��ֵ֮��read���������ݶ�Ϊ0,��δ����ԭ��
	//���붯̬�����ڴ棬���ʹ�þֲ�������ʾ���ᵼ��ջ�����Ĭ��ջ̫С
	unsigned char *readvalue = new unsigned char[10240];
	readsize = fread(readvalue, sizeof(unsigned char), 10240, tmp->_filefd);
	if (!feof(tmp->_filefd) && readsize)
	{
		Buffer* buf = new Buffer(readsize);
		buf->FullBuffer(readvalue, readsize);
		buf_share_ptr pbuf(buf);
		delete []readvalue;
		return pbuf;
	}
	else
	{
		//�ļ���β
		Buffer* buf = new Buffer(0);
		//buf->FullBuffer(readvalue, 0);
		buf_share_ptr pbuf(buf);
		delete []readvalue;
		return pbuf;
	}

}