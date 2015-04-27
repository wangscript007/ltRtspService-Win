#pragma once
#include "MediaBuffer.h"
#include <iostream>
#include <queue>

using namespace std;

class MediaStream
{
public:
	MediaStream();

	virtual void	DevNode(const Buffer& buf, unsigned pos);
	virtual void	DevNode(buf_share_ptr buf, unsigned pos);
	virtual buf_share_ptr GetNode();
	const unsigned GetListLen()const;
	virtual void	AddNode();

	virtual ~MediaStream();

	typedef struct _StreamInfo
	{
		int64_t frame_count; //��ǰ�ؼ�֡
		int64_t pframe_count;//��ǰ֡
		int32_t ssrc;		 //ÿ��ý������Ӧ��SSRC ������
		int lsb_count;			  //��ʱ����
		int fps;				  //֡��
		string filename;		  //�ļ���
	}StreamInfo;
	
	uint64_t filepos;	 //��ǰ�������Ӧ���ļ�ƫ��ֵ
	bool OnStream;		//�����ж�ʱ���ڲ���״̬
	bool IsPlay;
	unsigned short sqsnumber; //��ǰ����TCP֡������

	int fileindex;			  //��Ӧ�ļ�����

	StreamInfo MediaInfo;
protected:
	//ý�����������
	queue<Buffer *> bufferlist;
	
};


