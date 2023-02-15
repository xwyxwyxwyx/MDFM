#pragma once
#include "global.h"

#ifndef CONTEXT_H
#define CONTEXT_H

#define MIN_SECTOR_SIZE 0x200

#define BUFFER_SWAP_TAG     'bdBS'
#define CONTEXT_TAG         'xcBS'
#define NAME_TAG            'mnBS'
#define PRE_2_POST_TAG      'ppBS'


// �󶨸����������
typedef struct VOLUME_CONTEXT {

	// ����
	UNICODE_STRING Name;

	// DOS����
	UNICODE_STRING DosName;

	// ������С
	ULONG SectorSize;

}VOLUME_CONTEXT, * PVOLUME_CONTEXT;


// ��Ԥ�����ͺ����֮�䴫�ݵ�������
typedef struct _PRE_2_POST_CONTEXT {

	//
	// ָ��������Ľṹ��ָ�롣����������preOperation·���л�������ģ���Ϊ
	// ��DPC�����ϲ��ܰ�ȫ�ػ������Ȼ����postOperation·�����ͷ�������DPC��
	// ���ͷ��������ǰ�ȫ�ġ�
	//

	PVOLUME_CONTEXT VolCtx;

	//
	// ���ں�������ǽ��ա�ԭʼ���Ĳ�������ʹ������Ԥ�������޸������ݰ�������
	// �����Ҳ���ǽ��յ�û���޸ĵİ汾�����������Ҫ�������������滻�Ļ�����
	// ��ַ��
	//

	PVOID SwappedBuffer;


}PRE_2_POST_CONTEXT, * PPRE_2_POST_CONTEXT;
#endif 



