#ifndef GG_IPC_PACKET_SEQUENCES_H
#define GG_IPC_PACKET_SEQUENCES_H

#include <gg/ipc/mock.h>

GgipcPacket gg_test_connect_packet(GgBuffer auth_token);
GgipcPacket gg_test_connect_ack_packet(void);

GgipcPacketSequence gg_test_conn_ack_sequence(GgBuffer auth_token);
GgipcPacketSequence gg_test_connect_hangup_sequence(GgBuffer auth_token);

#endif
