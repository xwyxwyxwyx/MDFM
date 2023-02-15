#pragma once
#include "global.h"

#ifndef CONTEXT_H
#define CONTEXT_H

#define MIN_SECTOR_SIZE 0x200

#define BUFFER_SWAP_TAG     'bdBS'
#define CONTEXT_TAG         'xcBS'
#define NAME_TAG            'mnBS'
#define PRE_2_POST_TAG      'ppBS'


// 绑定给卷的上下文
typedef struct VOLUME_CONTEXT {

	// 卷名
	UNICODE_STRING Name;

	// DOS名称
	UNICODE_STRING DosName;

	// 扇区大小
	ULONG SectorSize;

}VOLUME_CONTEXT, * PVOLUME_CONTEXT;


// 在预操作和后操作之间传递的上下文
typedef struct _PRE_2_POST_CONTEXT {

	//
	// 指向卷上下文结构的指针。我们总是在preOperation路径中获得上下文，因为
	// 在DPC级别上不能安全地获得它。然后在postOperation路径中释放它。在DPC级
	// 别释放上下文是安全的。
	//

	PVOLUME_CONTEXT VolCtx;

	//
	// 由于后操作总是接收“原始”的参数，即使我们在预操作中修改了数据包参数，
	// 后操作也总是接收到没有修改的版本。因此我们需要保留我们用于替换的缓冲区
	// 地址。
	//

	PVOID SwappedBuffer;


}PRE_2_POST_CONTEXT, * PPRE_2_POST_CONTEXT;
#endif 



