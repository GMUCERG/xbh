#include <stdbool.h>
#include <inttypes.h>
#include "xbd_multipacket.h"
#include "string.h"
#include "util.h"

//Unused, so don't port
#if 0/*{{{*/
uint32_t XBD_genSucessiveMultiPacket(struct xbd_multipkt_state *state, const uint8_t* srcdata, uint8_t* dstbuf, uint32_t dstlenmax, const uint8_t *code) {
	uint32_t offset=0;
	uint32_t cpylen;

	if(0 == xbd_genmp_dataleft)
		return 0;

	strcpy(dstbuf, code);
	offset+=XBD_COMMAND_LEN;


	XBH_DEBUG("SQN: [%x\r\n",xbd_genmp_seqn);
	*((uint32_t*) (dstbuf + offset)) = htons(xbd_genmp_seqn);
	XBH_DEBUG("*xbd_genmp_seqn: [%x\r\n", *((uint32_t*) (dstbuf + offset)));

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

uint32_t XBD_genInitialMultiPacket(struct xbd_multipkt_state *state, const uint8_t* srcdata, uint32_t srclen, uint8_t* dstbuf,const uint8_t *code, uint32_t type, uint32_t addr) {
	uint32_t offset=0;

	xbd_genmp_seqn=0;
	xbd_genmp_datanext=0;
	xbd_genmp_dataleft=srclen;

	strcpy(dstbuf, code);
	offset+=XBD_COMMAND_LEN;

	if(0xffffffff != type)
	{
		*((uint32_t*) (dstbuf + offset)) = htons(type);
		offset+=TYPESIZE;
	}

	if(0xffffffff != addr)
	{
		*((uint32_t*) (dstbuf + offset)) = htons(addr);
		offset+=ADDRSIZE;
	}

	*((uint32_t*) (dstbuf + offset)) = htons(srclen);
	offset+=LENGSIZE;

	/* obs since v03
	crc16create(dstbuf, offset, ((u16*)(dstbuf+offset)));
	*/
	return offset;
}
#endif/*}}}*/

uint8_t XBD_recSucessiveMultiPacket(struct xbd_multipkt_state *state, const uint8_t* recdata, uint32_t reclen, uint8_t* dstbuf, uint32_t dstlenmax, const char *code) {
	uint32_t offset=0;
	uint32_t cpylen;

	if(0 == state->recmp_dataleft)
		return 0;

	if(memcmp(code,recdata,XBD_COMMAND_LEN))
		return 1;	//wrong command code
	offset+=XBD_COMMAND_LEN;
	if(offset > reclen)
		return 2;	//rec'd packet too shor

	if(state->recmp_seqn==ntohs(*((uint32_t*) (recdata + offset))))
	{
		offset+=SEQNSIZE;
		if(offset > reclen)
			return 2;	//rec'd packet too short
		++state->recmp_seqn;
	}
	else
		return 3;	//seqn error


	cpylen=(reclen-offset);	//&(~3);	//align 32bit
	if(cpylen > state->recmp_dataleft)
		cpylen=state->recmp_dataleft;

	if(state->recmp_datanext+cpylen > dstlenmax)
		return 4;	//destination buffer full

	memcpy((dstbuf+state->recmp_datanext),(recdata+offset),cpylen);
	state->recmp_dataleft-=cpylen;
	state->recmp_datanext+=cpylen;
	offset+=cpylen;

	return 0;
}

uint8_t XBD_recInitialMultiPacket(struct xbd_multipkt_state *state, const uint8_t* recdata, uint32_t reclen, const char *code, bool hastype, bool hasaddr) {
	uint32_t offset=0;

	state->recmp_seqn=0;
	state->recmp_datanext=0;

	if(memcmp(code,recdata,XBD_COMMAND_LEN)){
		return 1;	//wrong command code
    }

	offset+=XBD_COMMAND_LEN;
	if(offset > reclen)
		return 2;	//rec'd packet too short

	if(hastype) {
		state->recmp_type=ntohs(*((uint32_t*) (recdata + offset)));
		offset+=TYPESIZE;
		if(offset > reclen)
			return 2;	//rec'd packet too short
	}

	if(hasaddr) {
		state->recmp_addr=ntohs(*((uint32_t*) (recdata + offset)));
		offset+=ADDRSIZE;
		if(offset > reclen)
			return 2;	//rec'd packet too short
	}

	state->recmp_dataleft=ntohs(*((uint32_t*) (recdata + offset)));
	offset+=LENGSIZE;
	if(offset > reclen)
		return 2;	//rec'd packet too short

	return 0;
}
