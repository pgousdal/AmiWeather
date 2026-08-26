#include <amiweather/arexx.h>

#include <exec/memory.h>
#include <exec/ports.h>
#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <rexx/storage.h>
#include <string.h>

struct RxsLib *RexxSysBase;

struct AmiWeatherARexxHost { struct MsgPort *port; };

static void reply_unavailable(struct RexxMsg *message)
{
    message->rm_Result1 = AMIWEATHER_AREXX_UNAVAILABLE;
    message->rm_Result2 = 0;
    ReplyMsg((struct Message *)message);
}

struct AmiWeatherARexxHost *amiweather_arexx_host_open(void)
{
    struct AmiWeatherARexxHost *host;
    struct MsgPort *port;
    RexxSysBase = (struct RxsLib *)OpenLibrary((CONST_STRPTR)"rexxsyslib.library", 0);
    if (!RexxSysBase) return NULL;
    host = (struct AmiWeatherARexxHost *)AllocMem(sizeof(*host), MEMF_PUBLIC | MEMF_CLEAR);
    port = CreateMsgPort();
    if (!host || !port) {
        if (port) DeleteMsgPort(port);
        if (host) FreeMem(host, sizeof(*host));
        CloseLibrary((struct Library *)RexxSysBase); RexxSysBase = NULL;
        return NULL;
    }
    port->mp_Node.ln_Name = (char *)AMIWEATHER_AREXX_PORT;
    port->mp_Node.ln_Pri = 0;
    Forbid();
    if (FindPort((CONST_STRPTR)AMIWEATHER_AREXX_PORT)) {
        Permit(); DeleteMsgPort(port); FreeMem(host, sizeof(*host));
        CloseLibrary((struct Library *)RexxSysBase); RexxSysBase = NULL;
        return NULL;
    }
    AddPort(port); Permit(); host->port = port;
    return host;
}

ULONG amiweather_arexx_host_signal(const struct AmiWeatherARexxHost *host)
{
    return host && host->port ? 1UL << host->port->mp_SigBit : 0;
}

void amiweather_arexx_host_process(struct AmiWeatherARexxHost *host,
    struct AmiWeatherARexxState *state,
    const struct AmiWeatherARexxOperations *operations)
{
    struct RexxMsg *message;
    unsigned int handled = 0;
    if (!host || !host->port) return;
    while (handled++ < 4 &&
           (message = (struct RexxMsg *)GetMsg(host->port)) != NULL) {
        char result[AMIWEATHER_AREXX_RESULT_SIZE];
        int code = AMIWEATHER_AREXX_INVALID_ARGUMENTS;
        message->rm_Result1 = code; message->rm_Result2 = 0;
        if (CHECKREXXMSG(message) &&
            (message->rm_Action & RXCODEMASK) == RXCOMM && ARG0(message)) {
            ULONG length = LengthArgstring((UBYTE *)ARG0(message));
            code = amiweather_arexx_command((const char *)ARG0(message), (size_t)length,
                state, operations, result, sizeof(result));
            message->rm_Result1 = code;
            if (code == AMIWEATHER_AREXX_OK &&
                (message->rm_Action & RXFF_RESULT)) {
                message->rm_Result2 = (LONG)CreateArgstring(result, strlen(result));
                if (!message->rm_Result2)
                    message->rm_Result1 = AMIWEATHER_AREXX_UNAVAILABLE;
            }
        }
        ReplyMsg((struct Message *)message);
    }
    if (host->port->mp_MsgList.lh_Head->ln_Succ)
        Signal(FindTask(NULL), 1UL << host->port->mp_SigBit);
}

void amiweather_arexx_host_close(struct AmiWeatherARexxHost *host)
{
    struct RexxMsg *message;
    if (!host) return;
    if (host->port) {
        Forbid(); RemPort(host->port);
        while ((message = (struct RexxMsg *)GetMsg(host->port)) != NULL)
            reply_unavailable(message);
        Permit();
        DeleteMsgPort(host->port);
    }
    FreeMem(host, sizeof(*host));
    if (RexxSysBase) { CloseLibrary((struct Library *)RexxSysBase); RexxSysBase = NULL; }
}
