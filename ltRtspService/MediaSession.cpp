#include "stdafx.h"
#include "MediaSession.h"

//ʹ��ʱ����Ϊsession id
long long
MediaSession::GenSessionID()
{
    
    static LARGE_INTEGER	tickFrequency;
    static BOOL				tickFrequencySet = FALSE;

    LARGE_INTEGER	tickNow;

    if (tickFrequencySet == FALSE)
    {
        QueryPerformanceFrequency(&tickFrequency);
        tickFrequencySet = TRUE;
    }
    QueryPerformanceCounter(&tickNow);

    //return (INT32)(tickNow.QuadPart / tickFrequency.QuadPart);
    return tickNow.QuadPart;
    
   /* struct timeval start;
    gettimeofday(&start, NULL);
    long long value = start.tv_sec<<32|start.tv_usec;*/

    //return (long long)value;
}

//����ת�Ƶ���Ӧ��MediaStream�д���
void 
MediaSession::SetDescribe()
{
	unsigned p = 0;
	char SessionId[48] = {0};
	sessionid = GenSessionID();
    _snprintf_s(SessionId, 48, "%I64u", sessionid);
    printf("\nValue Of Session: %I64u\n", sessionid);
	//�滻
	while ((p = rtspinc.find("sessionforreplace")) != std::string::npos)
	{
		rtspinc.replace(p, 17, SessionId);
	}

	SessionId[0] = '9';
	SessionId[1] = '6';
	SessionId[2] = '\0';

	while ((p = rtspinc.find("loadtype")) != std::string::npos)
	{
		rtspinc.replace(p, 8, SessionId);
	}

	SessionId[0] = '0';
	SessionId[1] = '\0';
	rtspinc.replace(rtspinc.find("bandwidth"), 9, SessionId);

	SessionId[0] = '2';
	SessionId[1] = '0';
	SessionId[2] = '1';
	SessionId[3] = '\0';
	rtspinc.replace(rtspinc.find("tmpforesid"), 10, SessionId);

	SessionId[0] = '1';
	SessionId[1] = '\0';
	rtspinc.replace(rtspinc.find("tmpforstid"), 10, SessionId);

	p = rtspinc.find("\r\n\r\n");
	int ContentSize = rtspinc.size() - p;
    _snprintf_s(SessionId, 48, "%d", ContentSize - 4);
	
	p = rtspinc.find("content_count");
	rtspinc.replace(p, 13, SessionId);

}


void
MediaSession::DealRtsp(struct bufferevent* bev)
{
    char pstring[1480] = {0};

    if(bev && bufferevent_read(bev, pstring, 1480))
    {
        //printf("Get New Node:\n%s\n", pstring);
        rtspinc.append(pstring);
        if ( rtspinc.find("\r\n\r\n") != std::string::npos )
        {
            printf("IN<<\n%s\n", rtspinc.c_str());
			stype = rs.deal_requset(rtspinc).type;
            if(stype == DESCRIBE)
            {
                SetDescribe();
                int p = rtspinc.find("\r\n\r\n");
                bufferevent_write( bev, rtspinc.c_str(), p + 2 );
                bufferevent_write( bev, "\r\n", 2);
                bufferevent_write( bev, rtspinc.c_str() + (p + 4), (rtspinc.size() - p - 4));
                //bufferevent_write( bev, "\r\n", 2);
            }
			else if (stype == SETUP)
			{
				streamobj = MediaStreamBuild::CreateNew(".h264", type);
				streamobj->filename = "\\test.h264";
				streamobj->fileindex =  HardwareIO::GetInstance()->MakeFileNode(streamobj->filename);
				buf_share_ptr filebuf = HardwareIO::GetInstance()->GetBufferFormFile(streamobj->fileindex, streamobj->filepos, 10240);
				streamobj->DevNode(filebuf, 0);
				bufferevent_write( bev, rtspinc.c_str(), rtspinc.size() );
				bufferevent_write( bev, "\r\n", 2);
			}
			
            else if(stype == PLAY)
			{
				buf_share_ptr pt = RtpOverTcp::TCPForH264(streamobj);
				while (pt->GetMtuValue() <= 0)
				{
					pt = RtpOverTcp::TCPForH264(streamobj);
				}
				bufferevent_write( bev, rtspinc.c_str(), rtspinc.size() );
				bufferevent_write( bev, "\r\n", 2);
				streamobj->IsPlay = true;
				tick = GetTickCount();
				//bufferevent_write( bev, pt->GetBuffer(), pt->GetSizeValue());
				
			}
			else
            {
                bufferevent_write( bev, rtspinc.c_str(), rtspinc.size() );
                bufferevent_write( bev, "\r\n", 2);
            }
            printf("OUT>>\n%s\n", rtspinc.c_str());
            rtspinc.clear();
        }
    }
}

void
MediaSession::Recv(struct bufferevent* bev, void *arg)
{
	MediaSession* ses = (MediaSession *)arg;
	//����ʹ����RTP over TCP,���Դ˶˿�Ҳ���ܵ���Ӧ��RTCP�Ŀ�������Ϣ�����Ժ���������Ϣ���У��ƺ����������֡�
	ses->DealRtsp(bev);
}

void
MediaSession::Send(struct bufferevent* bev, void *arg)
{
    MediaSession *session = (MediaSession *)arg;
 
	if (session->IsPlay() == true && session->streamobj->IsPlay == true)
	{
		buf_share_ptr pt = RtpOverTcp::TCPForH264(session->streamobj);
		
		while (pt->GetMtuValue() <= 0)
		{
			pt = RtpOverTcp::TCPForH264(session->streamobj);
			//��δ�ж��ļ���β
		}
		//��ʱ�ڴ�ʹ����ʱ���Ժ����libevent�Ķ�ʱ��
		while (GetTickCount() - session->tick < session->streamobj->frame_count*1000*2*session->streamobj->fps);
		{
			Sleep(10);
		}
		
		
		bufferevent_write( bev, pt->GetBuffer(), pt->GetSizeValue());
	}
	
}
bool MediaSession::IsPlay()
{
	if (stype == PLAY)
	{
		return true;
	}
	return false;
	
}

MediaSession::~MediaSession()
{

}

