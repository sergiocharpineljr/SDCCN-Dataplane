#!/bin/bash

make || true

gcc -DPACKAGE_NAME=\"pofswitch\" -DPACKAGE_TARNAME=\"pofswitch\" -DPACKAGE_VERSION=\"1.3.4\" -DPACKAGE_STRING=\"pofswitch\ 1.3.4\" -DPACKAGE_BUGREPORT=\"yujingzhou@huawei.com\" -DPACKAGE_URL=\"\" -DPACKAGE=\"pofswitch\" -DVERSION=\"1.3.4\" -DPOFSWITCH_VERSION=\"POFSwitch-1.3.4\" -DHAVE_LIBPTHREAD=1 -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_ARPA_INET_H=1 -DHAVE_LIMITS_H=1 -DHAVE_NETINET_IN_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_SYS_SOCKET_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_UNISTD_H=1 -DHAVE_STDLIB_H=1 -DHAVE_MALLOC=1 -DHAVE_INET_NTOA=1 -DHAVE_MEMSET=1 -DHAVE_SOCKET=1 -DPOF_DEBUG_ON=1 -DPOF_PROMISC_ON=1 -DPOF_DATAPATH_ON=1 -DPOF_COMMAND_ON=1 -I. -I ./include    -g -O0 -MT pof_cache.o -MD -MP -MF .deps/pof_cache.Tpo -c -o pof_cache.o `test -f 'local_resource/pof_cache.c' || echo './'`local_resource/pof_cache.c

rm -rf /root/mininet-pofccnx/ccnx/csrc/lib/*test.o

gcc  -g -O0   -o pofswitch pof_basefunc.o pof_byte_transfer.o pof_command.o pof_log_print.o pof_action.o pof_datapath.o pof_instruction.o pof_lookup.o pof_counter.o pof_flow_table.o pof_group.o pof_local_resource.o pof_meter.o pof_port.o pof_config.o pof_encap.o pof_parse.o pof_switch_control.o pof_cache.o /root/mininet-pofccnx/ccnx/csrc/lib/*.o -L/root/mininet-pofccnx/ccnx/csrc/lib -lccn -lcrypto -lpthread
