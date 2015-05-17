#pragma once
#include <iostream>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <windows.h>
class Buffer;
typedef boost::shared_ptr<Buffer> buf_share_ptr;

enum BUFLOG
{
	DONE,		//����
	NONEDATA,	//��
	HALFSIZE,	//������ֵ
	HAVEDATA,   //�Ѵ�������
	FULLBUFFER,	//�����Ѵ������ݵ�Ԫ������һ����Ԫ����ʲ����
	LARGEPACKET,//�������ݵ�Ԫ�ϴ󣬴���MTU��Ҫ�ְ�
};

enum buftype
{
	NETBUFPOLL,
	NORMAL
};
/************************************************************************/
/* 
VECTOR MAP ��֤V0 V1�ڴ�������pbuff
[ v0 [ posbuf:8 ] [ rebuf:8 ] [ mapcount:16 ] [ *pbuff:4 ]] 
[ v1 [ posbuf:8 ] [ rebuf:8 ] [ mapcount:16 ] [ *pbuff:4 ]] 


*/
/************************************************************************/




#define  NODESIZE 1536
#define  MAPBUFSIZE 1536*128
#define  MAPPAGECOUNT 128





class Buffer
{
public:
	explicit Buffer(unsigned mtu = 1480, buftype buft = NORMAL);
	Buffer(const Buffer& ibuffer);
	
	static buf_share_ptr CreateBuf(unsigned mtu = 1480);
	const unsigned char* GetBuffer() const;

	unsigned GetMtuValue() const;
	unsigned GetPosValue() const;
	unsigned GetSizeValue()const;

	unsigned char GetByte(unsigned pos);
	void SetByte(unsigned char byteSet, unsigned pos);

	bool CopyData(unsigned char* dst, int length);
	bool Clear();
	
	inline bool FullBuffer(const unsigned char* buffer,unsigned lenth);
	bool FullBuffer(const Buffer& buf,unsigned start,unsigned length);
	bool FullBuffer(const buf_share_ptr buf,unsigned start,unsigned length);

	virtual ~Buffer();
protected:
	const Buffer& operator=(const Buffer& ibuffer);

private:
	unsigned short pos;
	unsigned MTU;
	unsigned char *pbuffer;
	buftype Buftype;
};

class Lockie
{
public:
	Lockie(CRITICAL_SECTION &lock):p(lock){EnterCriticalSection(&p);};
	~Lockie(){LeaveCriticalSection(&p);};
private:
	CRITICAL_SECTION &p;
};


class netbufferpoll
{
public:
	static netbufferpoll* get_instance(){return single;};
	typedef struct _netmap
	{
		_netmap():
			posbuf(0),
			rebuf(128),
			mapcount(0)
		{pbuff = new unsigned char[MAPBUFSIZE];memset(pbuff, 0, MAPBUFSIZE);};

		unsigned char posbuf;
		unsigned char rebuf;
		unsigned short mapcount;
		unsigned char *pbuff;
		bool has_buf()
		{
			return rebuf > 0;
		}
		unsigned char* get_node()
		{
			if(rebuf > 0)
			{
				pbuff[posbuf*NODESIZE] = -1;
				posbuf = 129;
				for (int i = 0; i < MAPPAGECOUNT; i++)
				{
					if (pbuff[i*NODESIZE] != -1){posbuf = i;break;}
				}
				rebuf --;
				assert(posbuf != 129);
				return &pbuff[posbuf*NODESIZE + 1];
			}
			else
			{return NULL;}
		};
	}netmap;

	unsigned char* get_buf()
	{
		for(unsigned int i = 0 ; i < maplist.size() ; i++){
			unsigned char *p = maplist[i].get_node();
			if (p != NULL)
				return p;
		}
		//��ǰ���ڵ�MAP���� ��Ҫ�µ�MAP
		netmap newmap;
		maplist.push_back(newmap);
		unsigned char *p = maplist[maplist.size()-1].get_node();
		if (p != NULL){
			return p;
		}
		else{
			//error
			return NULL;
		}
		
	};
protected:
	netbufferpoll(){InitializeCriticalSection(&MemLock);};
	CRITICAL_SECTION MemLock;
private:
	static netbufferpoll* single;
	std::vector<netmap> maplist;
};
