#pragma once
#include "network.h"
#include "RtspString.h"
#include "StreamBuilder.h"
#include "RtpOverTcp.h"
/************************************************************************/
/*               ý��Ự                                               */
/*	
/************************************************************************/
typedef long long INT64;
class MediaSessionList;
class MediaSession
{
public:
	
    static void Send(struct bufferevent*, void *arg);
    static void Recv(struct bufferevent*, void *arg);
	static long long GenSessionID();

	uint64_t GetSessionID()const;

    void DealRtsp(struct bufferevent* bev);
	void SetDescribe();

	explicit MediaSession(Network *pwork):work(pwork),IsFullInit(true){};
	MediaSession():IsFullInit(false){};
	friend class MediaSessionList;
	DWORD tick;
	bool IsPlay();
protected:
	virtual ~MediaSession();
	rtsp_string rs;
private:
	Network *work;
    INT64 sessionid;
	std::string rtspinc;
	media_stream_ptr streamobj;
	MediaType type;
	rtsp_type stype;
	bool IsFullInit;
	
};


//������ 

class MediaSessionList
{
public:
	static MediaSessionList* GetInstance();
	bool SessionInsert(MediaSession* Session);
	bool SessionDel(uint64_t SessionID);
	MediaSession* SessionGet(uint64_t SessionID);
protected:
	//CRITICAL_SECTION ListLock; ����ɾ����
	MediaSessionList();
	~MediaSessionList();
	map<uint64_t,  MediaSession*> SessionList;
};