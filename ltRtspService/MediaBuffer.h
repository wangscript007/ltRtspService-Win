#pragma once
#include <iostream>
#include <boost/shared_ptr.hpp>


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

class Buffer
{
public:
	explicit Buffer(unsigned mtu = 1480);
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
	unsigned char *pbuffer;
	unsigned short pos;
	unsigned MTU;
};




