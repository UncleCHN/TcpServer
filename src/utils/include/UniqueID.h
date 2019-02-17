/*
ID ���ɲ���
���뼶ʱ��41λ+����ID 10λ+����������12λ��
0 41 51 64 +-----------+------+------+ |time |pc |inc | +-----------+------+------+
ǰ41bits���Ժ���Ϊ��λ��timestamp��
����10bits���������úõĻ���ID��
���12bits���ۼӼ�������
macheine id(10bits)�������ֻ����1024̨����ͬʱ����ID��sequence number(12bits)Ҳ����1̨����1ms��������4096��ID�� *
ע��㣬��Ϊʹ�õ�λ�����㣬������Ҫ64λ����ϵͳ����Ȼ���ɵ�ID���п��ܲ���ȷ
*/
#pragma once

#include"utils/include/utils.h"
#include <time.h>
#include "utils/include/CmiThreadLock.h"
#include"utils/include/Singleton.h"

class DLL_CLASS UniqueID
{
	SINGLETON(UniqueID)

public:
	void set_workid(int workid);
	uint64_t gen();
	uint64_t UniqueID::gen_multi();

private:
	uint64_t get_curr_ms();
	uint64_t wait_next_ms(uint64_t lastStamp);

private:
	CmiThreadLock threadLock;
	uint64_t last_stamp;
	int workid;
	volatile uint64_t seqid;
};

