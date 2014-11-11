#include "xbd_multipacket.h"
#include "stack.h"
#include "xbh_utils.h"

u08 XBHMultiPacketDecodeBuf[XBD_ANSWERLENG_MAX];

u32 xbd_genmp_seqn,xbd_genmp_dataleft,xbd_genmp_datanext;
u32 xbd_recmp_seqn,xbd_recmp_dataleft,xbd_recmp_datanext,xbd_recmp_type,xbd_recmp_addr;

u32 XBD_genSucessiveMultiPacket(const u08* srcdata, u08* dstbuf, u32 dstlenmax, const prog_char *code)
{
	u32 offset=0;
	u32 cpylen;

	if(0 == xbd_genmp_dataleft)
		return 0;

	constStringToBuffer(dstbuf, code);
	offset+=XBD_COMMAND_LEN;


	XBH_DEBUG("SQN: [%x\r\n",xbd_genmp_seqn);
	*((u32*) (dstbuf + offset)) = HTONS32(xbd_genmp_seqn);
	XBH_DEBUG("*xbd_genmp_seqn: [%x\r\n",	*((u32*) (dstbuf + offset)));

	xbd_genmp_seqn++;
	offset+=SEQNSIZE;

	cpylen=(dstlenmax-offset );	//&(~3);	//align 32bit
	if(cpylen > xbd_genmp_dataleft)
		cpylen=xbd_genmp_dataleft;

	memcpy((dstbuf+offset),(srcdata+xbd_genmp_datanext),cpylen);
	xbd_genmp_dataleft-=cpylen;
	xbd_genmp_datanext+=cpylen;
	offset+=cpylen;

	return offset;
}

u32 XBD_genInitialMultiPacket(const u08* srcdata, u32 srclen, u08* dstbuf,const prog_char *code, u32 type, u32 addr)
{
	u32 offset=0;

	xbd_genmp_seqn=0;
	xbd_genmp_datanext=0;
	xbd_genmp_dataleft=srclen;

	constStringToBuffer(dstbuf, code);
	offset+=XBD_COMMAND_LEN;

	if(0xffffffff != type)
	{
		*((u32*) (dstbuf + offset)) = HTONS32(type);
		offset+=TYPESIZE;
	}

	if(0xffffffff != addr)
	{
		*((u32*) (dstbuf + offset)) = HTONS32(addr);
		offset+=ADDRSIZE;
	}

	*((u32*) (dstbuf + offset)) = HTONS32(srclen);
	offset+=LENGSIZE;

	/* obs since v03
	crc16create(dstbuf, offset, ((u16*)(dstbuf+offset)));
	*/
	return offset;
}

u08 XBD_recSucessiveMultiPacket(const u08* recdata, u32 reclen, u08* dstbuf, u32 dstlenmax, const prog_char *code)
{
	u32 offset=0;
	u32 cpylen;
	u08 strtmp[XBD_COMMAND_LEN+1];

	if(0 == xbd_recmp_dataleft)
		return 0;

	constStringToBuffer(strtmp, code);
	if(memcmp(strtmp,recdata,XBD_COMMAND_LEN))
		return 1;	//wrong command code
	offset+=XBD_COMMAND_LEN;
	if(offset > reclen)
		return 2;	//rec'd packet too shor

	if(xbd_recmp_seqn==NTOHS32(*((u32*) (recdata + offset))))
	{
		offset+=SEQNSIZE;
		if(offset > reclen)
			return 2;	//rec'd packet too short
		++xbd_recmp_seqn;
	}
	else
		return 3;	//seqn error


	cpylen=(reclen-offset);	//&(~3);	//align 32bit
	if(cpylen > xbd_recmp_dataleft)
		cpylen=xbd_recmp_dataleft;

	if(xbd_recmp_datanext+cpylen > dstlenmax)
		return 4;	//destination buffer full

	memcpy((dstbuf+xbd_recmp_datanext),(recdata+offset),cpylen);
	xbd_recmp_dataleft-=cpylen;
	xbd_recmp_datanext+=cpylen;
	offset+=cpylen;

	return 0;
}

u08 XBD_recInitialMultiPacket(const u08* recdata, u32 reclen, const prog_char *code, u08 hastype, u08 hasaddr)
{
	u32 offset=0;
	u08 strtmp[XBD_COMMAND_LEN+1];

	xbd_recmp_seqn=0;
	xbd_recmp_datanext=0;

	constStringToBuffer(strtmp, code);
	if(memcmp(strtmp,recdata,XBD_COMMAND_LEN))
		return 1;	//wrong command code

	offset+=XBD_COMMAND_LEN;
	if(offset > reclen)
		return 2;	//rec'd packet too short

	if(hastype)
	{
		xbd_recmp_type=NTOHS32(*((u32*) (recdata + offset)));
		offset+=TYPESIZE;
		if(offset > reclen)
			return 2;	//rec'd packet too short
	}

	if(hasaddr)
	{
		xbd_recmp_addr=NTOHS32(*((u32*) (recdata + offset)));
		offset+=ADDRSIZE;
		if(offset > reclen)
			return 2;	//rec'd packet too short
	}

	xbd_recmp_dataleft=NTOHS32(*((u32*) (recdata + offset)));
	offset+=LENGSIZE;
	if(offset > reclen)
		return 2;	//rec'd packet too short

	return 0;
}
