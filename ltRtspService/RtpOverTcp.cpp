#include "stdafx.h"
#include "RtpOverTcp.h"



buf_share_ptr 
RtpOverTcp::TCPForH264(media_stream_ptr st)
{
	assert(st);
	h264MediaStream* Media = dynamic_cast<h264MediaStream *>(st.get());
	buf_share_ptr node = Media->GetNode();
	if (node->GetMtuValue() == 0)
	{
		//ReadFile
		buf_share_ptr bufget = HardwareIO::GetInstance()->GetBufferFormFile(st->fileindex, st->filepos, 10240);
		if (bufget->GetMtuValue() == 10240)
		{
			printf("st.pos: %d\n", st->filepos);
			//��ȡ����֮������ֽ��NAL��Ԫ
			Media->DevNode(bufget, 0);
			node = Media->GetNode();
		}
		else
		{
			//���һ��readfile
			printf("at the end\n");

		}
	}
	uint8_t ftype = h264MediaStream::GetNalType(node); 
	
	do 
	{
		//SEI��PPS��������PPS��ͨ��SDP���͸�VLC
		if (ftype == 6 || ftype == 8)
		{
			buf_share_ptr ret(new Buffer(0));
			return ret;
		}
		if (ftype == 7)
		{
			//SPS
			if (Media->StreamSps == NULL)
			{
				buf_share_ptr sps = Buffer::CreateBuf(node->GetSizeValue());
				sps->FullBuffer(node, 0, node->GetSizeValue());
				//���SPS�ṹ��
				Media->ParseSqs(sps);
			}
			
			buf_share_ptr ret(new Buffer(0));
			//����ÿ���֡��
			Media->fps = Media->StreamSps->num_units_in_tick / Media->StreamSps->time_scale;
			return ret;
		}
		//����RTP��Mλ
		bool IsFinal = false;

		if(node->GetByte(0) == 0)
		{
			//SINGLE NAL ����Ҫ��Ƭ ����MλΪ1
			//h264_slice_t* slice = new h264_slice_t;
			//������ǰƬ��Ϣ ���ᱣ����Media��CurSlice��
			Media->ParseSlice(const_cast<unsigned char *>(node->GetBuffer()) + RTP_HEADSIZE + 1,
				(node->GetSizeValue() - RTP_HEADSIZE > 64?64:node->GetSizeValue()), h264MediaStream::GetNalType(node));
			//��ǰ֡��+1
			Media->pframe_count ++;
			
			IsFinal = true;
		}
		else if (node->GetByte(0) == 1)
		{
			//FU-A ����Ƭ������
			//
			if (node->GetByte(1) == 1)
			{
				//FU-A��ͷ   S E R TYPE 1 0 0 28
				//���ں����ڵ��е�ͷ���ֽڲ��ٰ���NAL���ͣ�������Ҫ���������Ƭ��Ϣͬʱ����
				Media->CurNalType = h264MediaStream::GetNalType(node);
				//
				Media->ParseSlice(const_cast<unsigned char *>(node->GetBuffer()) + RTP_HEADSIZE + 2, 64,
					Media->CurNalType);

				//������֯FU-indicator��FU-header
				node->SetByte(0x00 | node->GetByte(2)&0x60 | 28, RTP_HEADSIZE);
				node->SetByte(0x80 | Media->CurNalType, RTP_HEADSIZE + 1);
				//ֻ���ڵ�һ��FU-A֡���Żᱻ+1
				Media->pframe_count ++;
			}
			else if (node->GetByte(1) == 0)
			{
				//FU-A��β ��Ҫ����MֵΪ1
				node->SetByte(0x00 | node->GetByte(2)&0x60 | 28, RTP_HEADSIZE);
				//S E R 0 1 0
				node->SetByte(0x40 | Media->CurNalType, RTP_HEADSIZE + 1);
				//printf("FIND A FU-A END Type = %d Size = %d\n", Media->CurNalType, node->GetSizeValue());
				IsFinal = true;
			}
			else
			{
				//fu-a�м�ڵ�
				node->SetByte(0x00 | node->GetByte(2)&0x60 | 28, RTP_HEADSIZE);
				//S E R 0 0 0
				node->SetByte(Media->CurNalType, RTP_HEADSIZE + 1);
				//printf("FIND A FU-A Mid Type = %d  Size = %d\n", Media->CurNalType, node->GetSizeValue());
			}
		}
		//RTP OVER TCP��ͷ4���ֽ� $ + �ŵ�0�������ŵ���+ ����RTP���ݰ��ĳ���
		node->SetByte('$', 0);
		node->SetByte(0x00, 1);
		unsigned short size = node->GetSizeValue() - 4;
		node->SetByte((size >> 8)&0xff, 2);
		node->SetByte(size & 0xff, 3);
		
		//h264Ҫ��
		node->SetByte(0x80, 4);
		//����Mֵͬ������96��������Ϊ96����H264��RTP��������
		node->SetByte((IsFinal?0x80:0x00)|96,5);
		//����
		node->SetByte((Media->sqsnumber >> 8)&0xff, 6);
		node->SetByte((Media->sqsnumber&0xff), 7);
		//ÿ��TCP֡ ��������
		Media->sqsnumber ++;
		//����ʱ���
		int timestamp;
		//ͬһ�ؼ�֡�ڵ�frame_countΪ�̶�ֵ��ÿ�������ؼ�֡ʱ��ˢ��frame_count��ֵ��������Ҫ����FU-A��frame_count�������⡣
		bool framefinal = false;
		if (Media->CurSlice->i_slice_type == 2 || Media->CurSlice->i_slice_type == 7 ||
			Media->CurSlice->i_slice_type == 4 || Media->CurSlice->i_slice_type == 9 )
		{
			//����ؼ�֡ʱ���
			timestamp = Media->frame_count*90000*2*Media->StreamSps->num_units_in_tick / Media->StreamSps->time_scale;
			printf("I Frame timestamp = %u Frame Count= %u\n", timestamp, Media->frame_count);
			framefinal = true;
		}
		else
		{
			//ͬһ�ؼ�֡�ڣ�����P B֡��˳�����ʱ���
			timestamp = (Media->frame_count*2 + Media->CurSlice->i_pic_order_cnt_lsb)*90000*Media->StreamSps->num_units_in_tick / Media->StreamSps->time_scale;
			printf("P OR B Frame lsb = %d %d %d\n", Media->CurSlice->i_pic_order_cnt_lsb, Media->frame_count,timestamp);
		}
		if (framefinal)
		{
			//�ؼ�֡ˢ��frame_count
			Media->frame_count = Media->pframe_count;
		}
		//
		node->SetByte(timestamp >> 24 & 0xff, 8);
		node->SetByte(timestamp >> 16 & 0xff, 9);
		node->SetByte(timestamp >> 8  & 0xff, 10);
		node->SetByte(timestamp & 0xff, 11);
		printf("time Stamp = %d %d %d\n", timestamp, Media->CurSlice->i_pic_order_cnt_lsb, Media->frame_count);
		//����SSRC
		node->SetByte(Media->ssrc >> 24 & 0xff, 12);
		node->SetByte(Media->ssrc >> 16 & 0xff, 13);
		node->SetByte(Media->ssrc >> 8  & 0xff, 14);
		node->SetByte(Media->ssrc & 0xff, 15);
	} while (0);
	return node;
}