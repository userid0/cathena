#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifdef __WIN32
#define __USE_W32_SOCKETS
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <time.h>

#include "../common/timer.h"
#include "../common/malloc.h"
#include "../common/version.h"
#include "../common/nullpo.h"
#include "../common/showmsg.h"

#include "map.h"
#include "status.h"
#include "npc.h"
#include "chat.h"
#include "script.h"
#include "battle.h"

#include "pcre.h"

struct pcrematch_entry {
    struct pcrematch_entry *next_;
    char *pattern_;
    pcre *pcre_;
    pcre_extra *pcre_extra_;
    char *label_;
};
	
struct pcrematch_set {
    struct pcrematch_set *next_, *prev_;
    struct pcrematch_entry *head_;
    int active_;
    int setid_;
};

struct npc_parse {
    struct pcrematch_set *active_;
    struct pcrematch_set *inactive_;
};

static struct pcrematch_set * lookup_pcreset(struct npc_data *nd,int setid) 
{
    struct pcrematch_set *pcreset;
    struct npc_parse *npcParse = (struct npc_parse *) nd->chatdb;
    if (npcParse == NULL) 
        nd->chatdb = npcParse = (struct npc_parse *)
            aCalloc(sizeof(struct npc_parse), 1);

    pcreset = npcParse->active_;

    while (pcreset != NULL) {
        if (pcreset->setid_ == setid)
            break;
        pcreset = pcreset->next_;
    }
    if (pcreset == NULL) 
        pcreset = npcParse->inactive_;

    while (pcreset != NULL) {
        if (pcreset->setid_ == setid)
            break;
        pcreset = pcreset->next_;
    }

    if (pcreset == NULL) {
        pcreset = (struct pcrematch_set *) 
            aCalloc(sizeof(struct pcrematch_set), 1);
        pcreset->next_ = npcParse->inactive_;
        if (pcreset->next_ != NULL)
            pcreset->next_->prev_ = pcreset;
        pcreset->prev_ = 0;
        npcParse->inactive_ = pcreset;
        pcreset->active_ = 0;
        pcreset->setid_ = setid;
    }

    return pcreset;
}

static void activate_pcreset(struct npc_data *nd,int setid) {
    struct pcrematch_set *pcreset;
    struct npc_parse *npcParse = (struct npc_parse *) nd->chatdb;
    if (npcParse == NULL) 
        return; // Nothing to activate...
    pcreset = npcParse->inactive_;
    while (pcreset != NULL) {
        if (pcreset->setid_ == setid)
            break;
        pcreset = pcreset->next_;
    }
    if (pcreset == NULL)
        return; // not in inactive list
    if (pcreset->next_ != NULL)
        pcreset->next_->prev_ = pcreset->prev_;
    if (pcreset->prev_ != NULL)
        pcreset->prev_->next_ = pcreset->next_;
    else 
        npcParse->inactive_ = pcreset->next_;

    pcreset->prev_ = NULL;
    pcreset->next_ = npcParse->active_;
    if (pcreset->next_ != NULL)
        pcreset->next_->prev_ = pcreset;
    npcParse->active_ = pcreset;
}

static void deactivate_pcreset(struct npc_data *nd,int setid) {
    struct pcrematch_set *pcreset;
    struct npc_parse *npcParse = (struct npc_parse *) nd->chatdb;
    if (npcParse == NULL) 
        return; // Nothing to deactivate...
    pcreset = npcParse->active_;
    while (pcreset != NULL) {
        if (pcreset->setid_ == setid)
            break;
        pcreset = pcreset->next_;
    }
    if (pcreset == NULL)
        return; // not in active list
    if (pcreset->next_ != NULL)
        pcreset->next_->prev_ = pcreset->prev_;
    if (pcreset->prev_ != NULL)
        pcreset->prev_->next_ = pcreset->next_;
    else 
        npcParse->active_ = pcreset->next_;

    pcreset->prev_ = NULL;
    pcreset->next_ = npcParse->inactive_;
    if (pcreset->next_ != NULL)
        pcreset->next_->prev_ = pcreset;
    npcParse->inactive_ = pcreset;
}

static void delete_pcreset(struct npc_data *nd,int setid) {
    struct pcrematch_set *pcreset;
    struct npc_parse *npcParse = (struct npc_parse *) nd->chatdb;
    if (npcParse == NULL) 
        return; // Nothing to deactivate...
    pcreset = npcParse->active_;
    while (pcreset != NULL) {
        if (pcreset->setid_ == setid)
            break;
        pcreset = pcreset->next_;
    }
    if (pcreset == NULL)
        return; // not in active list
    if (pcreset->next_ != NULL)
        pcreset->next_->prev_ = pcreset->prev_;
    if (pcreset->prev_ != NULL)
        pcreset->prev_->next_ = pcreset->next_;
    else 
        npcParse->active_ = pcreset->next_;

    pcreset->prev_ = NULL;
    pcreset->next_ = NULL;

    // while (pcreset->head_)
    // finalize_pcrematch_entry(pcreset->head_);

    aFree(pcreset);
}

static struct pcrematch_entry *create_pcrematch_entry(struct pcrematch_set * set) {
    struct pcrematch_entry * e =  (struct pcrematch_entry *)
        aCalloc(sizeof(struct pcrematch_entry), 1);
    e->next_ = set->head_;
    set->head_ = e;
    return e;
}

void npc_chat_def_pattern(struct npc_data *nd, int setid, 
    const char *pattern, const char *label)
{
    const char *err;
    int erroff;

    struct pcrematch_set * s = lookup_pcreset(nd, setid);
    struct pcrematch_entry *e = create_pcrematch_entry(s);
    e->pattern_ = aStrdup(pattern);
    e->label_ = aStrdup(label);
    e->pcre_ = pcre_compile(pattern, PCRE_CASELESS, &err, &erroff, NULL);
    e->pcre_extra_ = pcre_study(e->pcre_, 0, &err);
}

void finalize_pcrematch_entry(struct pcrematch_entry *e) {
    free(e->pcre_);
    free(e->pcre_extra_);
    aFree(e->pattern_);
    aFree(e->label_);
}

void npc_chat_finalize(struct npc_data *nd)
{
    if (nd->chatdb == NULL)
        return;
}

int npc_chat_sub(struct block_list *bl, va_list ap)
{
    unsigned char *msg;
    int len, pos, i;
    struct npc_data *nd = (struct npc_data *)bl;
    struct map_session_data *sd;
    struct npc_label_list *lst;
    struct npc_parse *npcParse = (struct npc_parse *) nd->chatdb;
    struct pcrematch_set *pcreset;

    if (npcParse == NULL || npcParse->active_ == NULL)
        return 0;

    msg = va_arg(ap,unsigned char*);
    len = va_arg(ap,int);
    sd = va_arg(ap,struct map_session_data *);

    pcreset = npcParse->active_;

    while (pcreset != NULL) {
        struct pcrematch_entry *e = pcreset->head_;
        while (e != NULL) {
            int offsets[20];
            char buf[255];
            int r = pcre_exec(e->pcre_, e->pcre_extra_, msg, len, 0, 
                0, offsets, sizeof(offsets) / sizeof(offsets[0]));
            if (r >= 0) {
                switch (r) {
                case 10:
                    memcpy(buf, &msg[offsets[18]], offsets[19]);
                    buf[offsets[19]] = '\0';
                    set_var(sd, "$p9$", buf);
                case 9:
                    memcpy(buf, &msg[offsets[16]], offsets[17]);
                    buf[offsets[17]] = '\0';
                    set_var(sd, "$p8$", buf);
                case 8:
                    memcpy(buf, &msg[offsets[14]], offsets[15]);
                    buf[offsets[15]] = '\0';
                    set_var(sd, "$p7$", buf);
                case 7:
                    memcpy(buf, &msg[offsets[12]], offsets[13]);
                    buf[offsets[13]] = '\0';
                    set_var(sd, "$p6$", buf);
                case 6:
                    memcpy(buf, &msg[offsets[10]], offsets[11]);
                    buf[offsets[11]] = '\0';
                    set_var(sd, "$p5$", buf);
                case 5:
                    memcpy(buf, &msg[offsets[8]], offsets[9]);
                    buf[offsets[9]] = '\0';
                    set_var(sd, "$p4$", buf);
                case 4:
                    memcpy(buf, &msg[offsets[6]], offsets[7]);
                    buf[offsets[7]] = '\0';
                    set_var(sd, "$p3$", buf);
                case 3:
                    memcpy(buf, &msg[offsets[4]], offsets[5]);
                    buf[offsets[5]] = '\0';
                    set_var(sd, "$p2$", buf);
                case 2:
                    memcpy(buf, &msg[offsets[2]], offsets[3]);
                    buf[offsets[3]] = '\0';
                    set_var(sd, "$p1$", buf);
                case 1:
                    memcpy(buf, &msg[offsets[0]], offsets[1]);
                    buf[offsets[1]] = '\0';
                    set_var(sd, "$p0$", buf);
                }

                lst=nd->u.scr.label_list;
                pos = -1;
                for (i = 0; i < nd->u.scr.label_list_num; i++) {
                    if (strncmp(lst[i].name, e->label_, sizeof(lst[i].name)) == 0) {
                        pos = lst[i].pos;
                        break;
                    }
                }
                if (pos == -1) {
                    printf("Unable to find label: %s", e->label_);
                    // unable to find label... do something..
                    return 0;
                }
                run_script(nd->u.scr.script,pos,sd->bl.id,nd->bl.id);
                return 0;
            }
            e = e->next_;
        }
        pcreset = pcreset->next_;
    }

    return 0;
}

int buildin_defpattern(struct script_state *st) {
    int setid=conv_num(st,& (st->stack->stack_data[st->start+2]));
    char *pattern=conv_str(st,& (st->stack->stack_data[st->start+3]));
    char *label=conv_str(st,& (st->stack->stack_data[st->start+4]));
    struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);
    
    npc_chat_def_pattern(nd, setid, pattern, label);

    return 0;
}

int buildin_activatepset(struct script_state *st) {
    int setid=conv_num(st,& (st->stack->stack_data[st->start+2]));
    struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);

    activate_pcreset(nd, setid);

    return 0;
}
int buildin_deactivatepset(struct script_state *st) {
    int setid=conv_num(st,& (st->stack->stack_data[st->start+2]));
    struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);

    deactivate_pcreset(nd, setid);

    return 0;
}
int buildin_deletepset(struct script_state *st) {
    int setid=conv_num(st,& (st->stack->stack_data[st->start+2]));
    struct npc_data *nd=(struct npc_data *)map_id2bl(st->oid);

    delete_pcreset(nd, setid);

    return 0;
}

