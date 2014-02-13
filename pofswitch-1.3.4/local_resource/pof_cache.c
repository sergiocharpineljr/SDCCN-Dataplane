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

/***********************************************************************
 * Add a cache entry.
 * Form:     uint32_t poflr_add_cache_entry(pof_cache_entry *cache_ptr)
 * Input:    cache entry
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will add a new cache entry in the table. If a
 *           same cache entry is already exist in this table, ERROR.
 ***********************************************************************/
uint32_t poflr_add_cache_entry(pof_cache_entry *cache_ptr){
    char *name;
    struct cache_entry *ce = NULL;
    struct hashtb_enumerator ee;
    struct hashtb_enumerator *e = &ee;

    name = cache_ptr->name+1;
    name[strlen(name)] = '\0';
    hashtb_start(cache_tab, e);
    if (hashtb_seek(e, name, strlen(name)+1, 0) == HT_OLD_ENTRY)
        return -1; // FIXME
    /* Create entry. */
    ce = e->data;
    ce->strict = cache_ptr->strict;
    ce->idle_timeout = cache_ptr->strict;
    ce->hard_timeout = cache_ptr->hard_timeout;
    ce->priority = cache_ptr->priority;
    ce->index = cache_ptr->index;
    ce->name = (char*)malloc(strlen(name)+1*sizeof(char));
    memcpy(ce->name, name, strlen(name)+1);  
    hashtb_end(e);
    return POF_OK;
}

/***********************************************************************
 * Modify a cache entry.
 * Form:     uint32_t poflr_modify_cache_entry(pof_cache_entry *cache_ptr)
 * Input:    cache entry
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will modify a cache entry.
 ***********************************************************************/
uint32_t poflr_modify_cache_entry(pof_cache_entry *cache_ptr){
    //FIXME: IMPLEMENTAR LISTA ORDENADA PARA ISTO
    struct cache_entry *ce = NULL;
    struct hashtb_enumerator ee;
    struct hashtb_enumerator *e = &ee;
    int i;
    char *name = cache_ptr->name+1;
    name[strlen(name)] = '\0';

    hashtb_start(cache_tab, e);
    for (i = 0; i < hashtb_n(cache_tab); i++, hashtb_next(e)){
        ce = e->data;
        if (ce->index == cache_ptr->index){
            ce->strict = cache_ptr->strict;
            ce->idle_timeout = cache_ptr->strict;
            ce->hard_timeout = cache_ptr->hard_timeout;
            ce->priority = cache_ptr->priority;
            ce->index = cache_ptr->index;
            if (strcmp(ce->name, name) != 0){
                free(ce->name);
                ce->name = (char*)malloc(strlen(name)+1*sizeof(char));
                memcpy(ce->name, name, strlen(name)+1);
            }
            return POF_OK;
        }
    }
    return -1;
}

/***********************************************************************
 * Delete a cache entry.
 * Form:     uint32_t poflr_delete_cache_entry(pof_cache_entry *cache_ptr)
 * Input:    cache entry
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will delete a cache entry in the flow table.
 ***********************************************************************/
uint32_t poflr_delete_cache_entry(pof_cache_entry *cache_ptr){
    //FIXME: IMPLEMENTAR LISTA ORDENADA PARA ISTO
    struct cache_entry *ce = NULL;
    struct hashtb_enumerator ee;
    struct hashtb_enumerator *e = &ee;
    int i;
    char *name = cache_ptr->name+1;
    name[strlen(name)] = '\0';

    hashtb_start(cache_tab, e);
    for (i = 0; i < hashtb_n(cache_tab); i++, hashtb_next(e)){
        ce = e->data;
        if (ce->index == cache_ptr->index){
            free(ce->name);
            hashtb_delete(e);
            //FIXME DELETAR ENTRADAS DA CONTENTSTORE
            struct cs_entry *ce1;
            struct hashtb_enumerator ee1;
            struct hashtb_enumerator *e1 = &ee1;
            int j;

            hashtb_start(cs_tab, e1);
            for (j = 0; j < hashtb_n(cs_tab); j++, hashtb_next(e1)){
                ce1 = e1->data;
                if (strncmp(ce1->name, name, strlen(name)) == 0){
                    // check if there is another match
                    if (!poflr_match_cache_entry(ce1->name, strlen(ce1->name)+1)){
                        free(ce1->ccnb);
                        free(ce1->name);
                        hashtb_delete(e1);
                    }
                }
            }
            hashtb_end(e1);
            return POF_OK;
        }
    }
    return -1;
}

/* Match content */
struct cache_entry* poflr_match_cache_entry(char *name, int nsize){
    struct cache_entry *ce;
    struct hashtb_enumerator ee;
    struct hashtb_enumerator *e = &ee;
    int i;

    hashtb_start(cache_tab, e);
    for (i = 0; i < hashtb_n(cache_tab); i++, hashtb_next(e)){
        ce = e->data;
        // check for strict = 1
        if (ce->strict == 1 && strcmp(ce->name, name) == 0){
            hashtb_end(e);
            return ce;
        }
        // check for strict = 0
        if (ce->strict == 0 && strncmp(ce->name, name, strlen(ce->name)) == 0){
            hashtb_end(e);
            return ce;
        }
    }
    return NULL;
}

/* Create cache_tb */
void poflr_create_cache_table(){
    cache_tab = hashtb_create(sizeof(struct cache_entry), NULL); // XXX - CHECK &param
}

/* Destroy cache_tb */
void poflr_destroy_cache_table(){
    return hashtb_destroy(&cache_tab);
}

void print_cache_tab(){
    struct cache_entry *ce;
    struct hashtb_enumerator ee;
    struct hashtb_enumerator *e = &ee;
    int i;

    printf("PRINTING CACHE_TAB\n");
    hashtb_start(cache_tab, e);
    for (i = 0; i < hashtb_n(cache_tab); i++, hashtb_next(e)){
        ce = e->data;
        printf("%d => %s\n", ce->index, ce->name);
    }
}
