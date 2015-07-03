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
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <net/ethernet.h>

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
    if (hashtb_seek(e, name, strlen(name)+1, 0) == HT_OLD_ENTRY){
        hashtb_end(e);
        return poflr_modify_cache_entry(cache_ptr);
    }
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
            hashtb_end(e);
            return POF_OK;
        }
    }
    hashtb_end(e);
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
                    if (!poflr_match_cache_entry(ce1->name)){
                        free(ce1->ccnb);
                        free(ce1->name);
                        hashtb_delete(e1);
                    }
                }
            }
            hashtb_end(e);
            hashtb_end(e1);
            return POF_OK;
        }
    }
    hashtb_end(e);
    return -1;
}

/* Match content */
struct cache_entry* poflr_match_cache_entry(char *name){
    struct cache_entry *ce;
    struct hashtb_enumerator ee;
    struct hashtb_enumerator *e = &ee;
    int i;

    hashtb_start(cache_tab, e);
    for (i = 0; i < hashtb_n(cache_tab); i++, hashtb_next(e)){
        ce = e->data;
        // check for strict = 1
        if (ce->strict == 1 && strcmp(ce->name, name+6) == 0){
            hashtb_end(e);
            return ce;
        }
        // check for strict = 0
        if (ce->strict == 0 && strncmp(ce->name, name+6, strlen(ce->name)) == 0){
            hashtb_end(e);
            return ce;
        }
    }
    hashtb_end(e);
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

    hashtb_start(cache_tab, e);
    for (i = 0; i < hashtb_n(cache_tab); i++, hashtb_next(e)){
        ce = e->data;
        printf("%d => %s\n", ce->index, ce->name);
    }
    hashtb_end(e);
}

uint32_t poflr_cache_full_report(pof_cache_full_command command, int total_entries, int used_entries){
    pof_cache_full cache_full;

    cache_full.command = command;
    cache_full.total_entries = total_entries;
    cache_full.used_entries = used_entries;

    //pof_HtoN_transfer_cache_full(&cache_full);

    if(POF_OK != pofec_reply_msg(POFT_CACHE_FULL, g_upward_xid, sizeof(pof_cache_full), (uint8_t *)&cache_full)){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE, g_recv_xid);
    }

    return POF_OK;
}

uint32_t poflr_send_cache_info(pof_cache_info *cache_info_ptr){
    struct cs_entry *ce = NULL;
    struct hashtb_enumerator ee;
    struct hashtb_enumerator *e = &ee;
    int i;
    char *name;
    char buff[20];
    struct ccn_charbuf *uri;
    struct ccn_charbuf *ccnb = NULL;

    pof_cache_info cache_info;

    hashtb_start(cs_tab, e);
    
    cache_info.command = OFPCIAC_REPLY;
    cache_info.total_entries = hashtb_n(cs_tab);

    int n = 0;
    for (i = 0; i < hashtb_n(cs_tab); i++, hashtb_next(e)){
        ce = e->data;

        uri = ccn_charbuf_create();
        int res = ccn_uri_append_flatname(uri, ce->name, ce->name_size, 1);
        memcpy(cache_info.entries+n, ccn_charbuf_as_string(uri), strlen(ccn_charbuf_as_string(uri))+1);
        n += strlen(ccn_charbuf_as_string(uri))+1;
        cache_info.entries[n-1] = '\t';

        //add created datetime
        strftime(buff, 20, "%d-%m-%Y %H:%M:%S", localtime(&ce->created));
        memcpy(cache_info.entries+n, buff, strlen(buff)+1);
        n += strlen(buff)+1;
        cache_info.entries[n-1] = '\t';
        
        //add updated datetime
        strftime(buff, 20, "%d-%m-%Y %H:%M:%S", localtime(&ce->updated));
        memcpy(cache_info.entries+n, buff, strlen(buff)+1);
        n += strlen(buff)+1;
        cache_info.entries[n-1] = '\n';
    }
    cache_info.entries[n-1] = '\0';
    hashtb_end(e);

    pof_HtoN_transfer_cache_info(&cache_info);
    if(POF_OK != pofec_reply_msg(POFT_CACHE_INFO, g_upward_xid, sizeof(pof_cache_info), (uint8_t *)&cache_info)){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE, g_recv_xid);
    }

    return POF_OK;
}

uint32_t poflr_delete_cs_entry(pof_cache_entry *cache_ptr){
    //FIXME: IMPLEMENTAR LISTA ORDENADA PARA ISTO
    struct cs_entry *ce = NULL;
    struct hashtb_enumerator ee;
    struct hashtb_enumerator *e = &ee;
    char *name = cache_ptr->name;
    int i;
    struct ccn_charbuf *uri;

    hashtb_start(cs_tab, e);
    for (i = 0; i < hashtb_n(cs_tab); i++, hashtb_next(e)){
        ce = e->data;
        uri = ccn_charbuf_create();
        int res = ccn_uri_append_flatname(uri, ce->name, ce->name_size, 1);
        if (strcmp(ccn_charbuf_as_string(uri), name) == 0){
            free(ce->name);
            hashtb_delete(e);
            hashtb_end(e);
            return POF_OK;
        }
    }
    hashtb_end(e);
    return -1;
}


//FIXME
uint32_t poflr_add_cs_entry(char *name){
    printf("CS ADD ENTRY\n");
    struct ccn_charbuf *buf = ccn_charbuf_create();

    //create name components
    struct ccn_charbuf *buf_name = ccn_charbuf_create();
    ccn_name_init(buf_name);
    char substr[strlen(name)];
    int i, j;
    i = 6;
    for (j = i; j < strlen(name); j++)
    {
        if (name[j] == '/'){
            memcpy(substr, name+i, j-i);
            substr[j-i] = '\0';
            ccn_name_append_str(buf_name, substr);
            i = j+1;
            memset(substr,0,sizeof(substr));
        }
    }
    memcpy(substr, name+i, j-i);
    substr[j-i] = '\0';
    printf("substr = %s\n", substr);
    ccn_name_append_str(buf_name, substr);

    ccnb_element_begin(buf, CCN_DTAG_Interest);
    ccn_charbuf_append_charbuf(buf, buf_name);
    ccnb_element_end(buf);

    struct pofdp_packet *dpp = malloc(sizeof *dpp);
    int sheaders = sizeof(struct ether_header)+sizeof(struct iphdr)+sizeof(struct udphdr);
    unsigned char* data = (unsigned char*)malloc(buf->length*sizeof(char)+sheaders);
    struct ether_header *eh = (struct ether_header *)data;
    struct iphdr *iph = (struct iphdr *)(data + sizeof(struct ether_header));
    struct udphdr *udph = (struct udphdr *)(data + sizeof(struct ether_header) + sizeof(struct iphdr));

    for (i = 0; i < 6; i++){
        eh->ether_shost[i] = 0xff;
        eh->ether_dhost[i] = 0xff;
    }
    //eh->ether_shost[0] = 0xea;
    //eh->ether_shost[1] = 0x49;
    //eh->ether_shost[2] = 0xac;
    //eh->ether_shost[3] = 0xef;
    //eh->ether_shost[4] = 0x44;
    //eh->ether_shost[5] = 0xda;
    //eh->ether_dhost[5] = 0x01;
    eh->ether_type = htons(ETH_P_IP);

    /* IP Header */
    iph->ihl = 5;
    iph->version = 4;
    iph->id = htons(54321);
    iph->ttl = 10; // hops
    iph->frag_off = 0;
    iph->protocol = 17; // UDP
    iph->saddr = inet_addr("10.0.0.3");
    iph->daddr = inet_addr("10.0.0.1");
    //iph->tot_len = htons(sheaders - sizeof(struct ether_header) + buf->length);
    iph->tot_len = htons(46);
    udph->check = 0;
    udph->source = htons(9695);
    udph->dest = htons(9695);
    udph->len = htons(sizeof(struct udphdr) + buf->length);
    memcpy(data+sheaders, buf->buf, buf->length);
    iph->check = 0;
    iph->check = csum((unsigned short *)(data+sizeof(struct ether_header)), sizeof(struct iphdr)/2);
    dpp->buf = data;
    dpp->output_packet_len = sheaders + buf->length;
    dpp->output_whole_len = sheaders + buf->length;
    dpp->output_packet_offset = 0;
    dpp->output_metadata_offset = 0;
    dpp->output_metadata_len = 0;
    POF_DEBUG_CPRINT_FL_0X(1,YELLOW,dpp->buf, dpp->output_whole_len,"TESTEE BUF: ");

    // FLOOD packet
    pof_port *port_ptr = NULL;
    uint16_t port_number = 0;
    uint32_t ret;
    poflr_get_port_number(&port_number);
    poflr_get_port(&port_ptr);

    for(i=0; i<port_number; i++){
        if (port_ptr[i].of_enable == POFLR_PORT_DISABLE)
            continue;

        printf("ENVIANDO PARA\n");
        dpp->output_port_id = port_ptr[i].port_id;
        pofdp_send_raw(dpp);
    }
    return POF_OK;
}
