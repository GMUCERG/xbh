#ifndef __XBH_PROT_H__
#define __XBH_PROT_H__
#include "util.h"

#define XBH_PROTO_VER "05"
#define XBD_PROTO_VER "04"

#define XBH_CMD_DEF(X) "XBH"XBH_PROTO_VER#X,
#define XBD_CMD_DEF(X) "XBD"XBD_PROTO_VER#X,
#define GEN_XBH_CMD_ENUM(ENUM) XBH_CMD_##ENUM,
#define GEN_XBD_CMD_ENUM(ENUM) XBD_CMD_##ENUM,

// XXX Might have issues w/ some compilers that start enums from 1, but for now
// just worry about GCC
#define FOREACH_XBD_CMD(GEN) \
/* /XBD commands to send via TWI */ \
    GEN(pfr) \
    GEN(exr) \
    GEN(fdr) \
    GEN(sar) \
    GEN(sbr) \
    GEN(vir) \
    GEN(ppr) \
    GEN(pdr) \
    GEN(urr) \
    GEN(rdr) \
    GEN(ccr) \
    GEN(tcr) \
    GEN(sur) \
    GEN(trr) \
/* OK ('o') Responses from bootloader */ \
    GEN(pfo) \
    GEN(fdo) \
    GEN(ppo) \
    GEN(pdo) \
    GEN(exo) \
    GEN(uro) \
    GEN(rdo) \
    GEN(cco) \
    GEN(tco) \
    GEN(suo) \
    GEN(tro) \
/* CRC Fail */ \
    GEN(ccf) \
/* Version Information OK Responses */ \
    GEN(BLo) \
    GEN(AFo) 

#define FOREACH_XBH_CMD(GEN) \
/* Requests ('r') understood by XBH */ \
    GEN(cdr) \
    GEN(exr) \
    GEN(ccr) \
    GEN(str) \
    GEN(rcr) \
    GEN(sar) \
    GEN(sbr) \
    GEN(dpr) \
    GEN(urr) \
    GEN(rpr) \
    GEN(pwr) \
    GEN(scr) \
    GEN(sur) \
    GEN(tcr) \
    GEN(srr) \
    GEN(trr) \
    GEN(lor) \
/* OK ('o') Responses from XBH */ \
    GEN(kao) /* KeepAlive OK */\
    GEN(cdo) \
    GEN(exo) \
    GEN(cco) \
    GEN(sto) \
    GEN(rco) \
    GEN(sao) \
    GEN(sbo) \
    GEN(dpo) \
    GEN(uro) \
    GEN(rpo) \
    GEN(pwo) \
    GEN(sco) \
    GEN(suo) \
    GEN(tco) \
    GEN(sro) \
    GEN(tro) \
    GEN(loo) \
/* ACK ('a') Responses from XBH */ \
    GEN(cda) \
/* Failed ('f') Responses from XBH (originates from bootloader) */ \
    GEN(cdf) \
    GEN(rcf) \
    GEN(saf) \
    GEN(sbf) \
    GEN(dpf) \
    GEN(exf) \
    GEN(urf) \
    GEN(ccf) \
    GEN(scf) \
    GEN(suf) \
    GEN(tcf) \
    GEN(trf) \
    GEN(lof) \
/* Unknown command */ \
    GEN(unk)


/**
 * XBH Command Enum
 */
enum XBH_CMD_ENUM {
     FOREACH_XBH_CMD(GEN_XBH_CMD_ENUM)
};

/**
 * XBD Command Enum
 */
enum XBD_CMD_ENUM {
     FOREACH_XBD_CMD(GEN_XBD_CMD_ENUM)
};
#define XBH_COMMAND_LEN (3+2+3)
#define XBD_COMMAND_LEN (3+2+3)

extern const char *XBH_CMD[];
extern const char *XBD_CMD[];

#endif

