/**
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "../include/pof_common.h"
#include "../include/pof_type.h"
#include "../include/pof_global.h"
#include "../include/pof_local_resource.h"
#include "../include/pof_conn.h"
#include "../include/pof_byte_transfer.h"
#include "../include/pof_log_print.h"
#include "../include/pof_datapath.h"
#include "../include/ccn/hashtb.h"
#include "string.h"

uint32_t poflr_add_pit_entry(char *name, uint8_t port_id){
    struct pit_entry *ce = NULL;
    struct hashtb_enumerator ee;
    struct hashtb_enumerator *e = &ee;

    hashtb_start(pit_tab, e);
    if (hashtb_seek(e, name, strlen(name), 0) == HT_OLD_ENTRY){
        ce = e->data;
        if (ce->n < POFLR_MAX_PORT_IDS)
        {
            ce->port_ids[ce->n] = port_id;
            ce->n++;
        }
        hashtb_end(e);
        return POF_OK;
    }
    /* Create entry. */
    ce = e->data;
    ce->name = (char*)malloc((strlen(name)+1)*sizeof(char));
    strcpy(ce->name, name);  
    ce->n = 1;
    ce->port_ids[0] = port_id;
    hashtb_end(e);
    return POF_OK;
}

uint32_t poflr_delete_pit_entry(char *name){
    struct pit_entry *ce = NULL;
    struct hashtb_enumerator ee;
    struct hashtb_enumerator *e = &ee;
    int i;

    hashtb_start(pit_tab, e);
    for (i = 0; i < hashtb_n(pit_tab); i++, hashtb_next(e)){
        ce = e->data;
        if ((ce->name != NULL) && (strncmp(ce->name, name, strlen(name)) == 0)){
            free(ce->name);
            hashtb_delete(e);
            hashtb_end(e);
            return POF_OK;
        }
    }
    hashtb_end(e);
    return -1;
}

void poflr_create_pit_table(){
    pit_tab = hashtb_create(sizeof(struct pit_entry), NULL); // XXX - CHECK &param
}

void poflr_destroy_pit_table(){
    return hashtb_destroy(&pit_tab);
}

void print_pit_tab(){
    struct pit_entry *ce;
    struct hashtb_enumerator ee;
    struct hashtb_enumerator *e = &ee;
    int i;

    printf("PRINTING PIT_TAB\n");
    hashtb_start(pit_tab, e);
    for (i = 0; i < hashtb_n(pit_tab); i++, hashtb_next(e)){
        ce = e->data;
        printf("%d => %s\n", ce->index, ce->name);
    }
    hashtb_end(e);
}

/* Match content */
struct pit_entry* poflr_match_pit_entry(char *name){
    struct pit_entry *pe;
    struct hashtb_enumerator ee;
    struct hashtb_enumerator *e = &ee;
    int i;

    hashtb_start(pit_tab, e);
    for (i = 0; i < hashtb_n(pit_tab); i++, hashtb_next(e)){
        pe = e->data;
        if (strncmp(pe->name, name, strlen(pe->name)) == 0){
            hashtb_end(e);
            return pe;
        }
    }
    hashtb_end(e);
    return NULL;
}
