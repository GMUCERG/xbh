#include <stdbool.h>
#include <inttypes.h>
#include "xbd_multipacket.h"
#include "string.h"
#include "util.h"

size_t XBD_genInitialMultiPacket(struct xbd_multipkt_state *state, const void *srcdata, size_t srclen, void *dstbuf, const char *cmd_code, uint32_t addr, uint32_t type) {
	size_t offset=0;
	state->seqn=0;
	state->datanext=0;
	state->dataleft=srclen;

    memcpy(dstbuf, cmd_code, XBD_COMMAND_LEN);
	offset+=XBD_COMMAND_LEN;

	if(NO_MP_TYPE != type) {
		*((uint32_t*) ((uint8_t*)dstbuf + offset)) = htonl(type);
		offset+=TYPESIZE;
	}

	if(NO_MP_ADDR != addr)
	{
		*((uint32_t*) ((uint8_t*)dstbuf + offset)) = htonl(addr);
		offset+=ADDRSIZE;
	}

	*((uint32_t*) ((uint8_t*)dstbuf + offset)) = htonl(srclen);
	offset+=LENGSIZE;

    state->type = type;
    state->tgt_data = srcdata;

	return offset;
}
size_t XBD_genSucessiveMultiPacket(struct xbd_multipkt_state *state, void* dstbuf, uint32_t dstlenmax, const char *cmd_code) {
	uint32_t offset=0;
	uint32_t cpylen;

	if(0 == state->dataleft)
		return 0;

    memcpy(dstbuf, cmd_code, XBD_COMMAND_LEN);
	offset+=XBD_COMMAND_LEN;


	XBH_DEBUG("SQN: [%x]\n",state->seqn);
	*((uint32_t*) ((uint8_t *)dstbuf + offset)) = htonl(state->seqn);
	offset+=SEQNSIZE;
	state->seqn++;

	cpylen=(dstlenmax-offset);	//&(~3);	//align 32bit
	if(cpylen > state->dataleft)
		cpylen = state->dataleft;

	memcpy(((uint8_t *)dstbuf+offset),((uint8_t *)state->tgt_data+state->datanext), cpylen);
	state->dataleft-=cpylen;
	state->datanext+=cpylen;
	offset+=cpylen;

	return offset;
}

int XBD_recInitialMultiPacket(struct xbd_multipkt_state *state, const void *recdata, uint32_t reclen, const char *cmd_code, bool hastype, bool hasaddr) {
	uint32_t offset=0;

	state->seqn=0;
	state->datanext=0;

	if(memcmp(cmd_code,recdata,XBD_COMMAND_LEN)){
		return 1;	//wrong command code
    }

	offset+=XBD_COMMAND_LEN;
	if(offset > reclen)
		return 2;	//rec'd packet too short

	if(hastype) {
		state->type=ntohl(*((uint32_t*) ((uint8_t *)recdata + offset)));
		offset+=TYPESIZE;
		if(offset > reclen)
			return 2;	//rec'd packet too short
	}

	if(hasaddr) {
		state->addr=ntohl(*((uint32_t*) ((uint8_t *)recdata + offset)));
		offset+=ADDRSIZE;
		if(offset > reclen)
			return 2;	//rec'd packet too short
	}

	state->dataleft=ntohl(*((uint32_t*) ((uint8_t *)recdata + offset)));
	offset+=LENGSIZE;
	if(offset > reclen)
		return 2;	//rec'd packet too short

	return 0;
}

int XBD_recSucessiveMultiPacket(struct xbd_multipkt_state *state, const void *recdata, uint32_t reclen, void *dstbuf, uint32_t dstlenmax, const char *code) {
	uint32_t offset=0;
	uint32_t cpylen;

	if(0 == state->dataleft)
		return 0;

	if(memcmp(code,recdata,XBD_COMMAND_LEN))
		return 1;	//wrong command code
	offset+=XBD_COMMAND_LEN;
	if(offset > reclen)
		return 2;	//rec'd packet too shor

	if(state->seqn==ntohl(*((uint32_t*) ((uint8_t *)recdata + offset))))
	{
		offset+=SEQNSIZE;
		if(offset > reclen)
			return 2;	//rec'd packet too short
		++state->seqn;
	}
	else
		return 3;	//seqn error


	cpylen=(reclen-offset);	//&(~3);	//align 32bit
	if(cpylen > state->dataleft)
		cpylen=state->dataleft;

	if(state->datanext+cpylen > dstlenmax)
		return 4;	//destination buffer full

	memcpy(((uint8_t *)dstbuf+state->datanext),((uint8_t *)recdata+offset),cpylen);
	state->dataleft-=cpylen;
	state->datanext+=cpylen;
	offset+=cpylen;

	return 0;
}

