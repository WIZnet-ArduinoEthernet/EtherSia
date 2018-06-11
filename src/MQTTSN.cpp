#include "EtherSia.h"
#include "util.h"

MQTTSN_Client::MQTTSN_Client(EtherSia &ether) : UDPSocket(ether)
{
    _state = MQTT_SN_STATE_DISCONNECTED;
}

boolean MQTTSN_Client::setRemoteAddress(const char *remoteAddress)
{
    return UDPSocket::setRemoteAddress(remoteAddress, MQTT_SN_DEFAULT_PORT);
}

void MQTTSN_Client::connect()
{
    char clientId[22];
    const MACAddress& mac = _ether.localMac();
    const static char prefix[] PROGMEM = "EtherSia-";

    // Generate clientId from prefix string and MAC Address
    memcpy_P(clientId, prefix, 9);
    for (byte b=0; b < 6; b++) {
        hexToAscii(mac[b], &(clientId[9 + (b*2)]));
    }
    clientId[21] = '\0';
    connect(clientId);
}

void MQTTSN_Client::connect(const char* clientId)
{
    uint8_t *headerPtr = this->transmitPayload();
    const uint8_t headerLen = 6;
    uint8_t clientIdLen = strlen(clientId);
    
    headerPtr[0] = headerLen + clientIdLen;
    headerPtr[1] = MQTT_SN_TYPE_CONNECT;
    headerPtr[2] = MQTT_SN_FLAG_CLEAN;
    headerPtr[3] = MQTT_SN_PROTOCOL_ID;
    headerPtr[4] = highByte(MQTT_SN_DEFAULT_KEEP_ALIVE);
    headerPtr[5] = lowByte(MQTT_SN_DEFAULT_KEEP_ALIVE);

    memcpy(headerPtr + headerLen, clientId, clientIdLen);
    send(headerPtr[0], false);
}

bool MQTTSN_Client::checkConnected()
{
    if (havePacket()) {
        uint8_t *headerPtr = this->payload();
    
        if (headerPtr[1] == MQTT_SN_TYPE_CONNACK) {
            uint8_t returnCode = headerPtr[2];
            Serial.print("CONNACK=");
            Serial.println(returnCode, DEC);  
            
            if (returnCode == MQTT_SN_ACCEPTED) {
                _state = MQTT_SN_STATE_CONNECTED;
            } else {
                _state = MQTT_SN_STATE_REJECTED;
            }
        } else {
            Serial.print(F("Received unknown MQTT-SN packet type=0x"));
            Serial.println(headerPtr[1], HEX);  
        }
    }

    // FIXME: perform pings

    return _state == MQTT_SN_STATE_CONNECTED;
}


void MQTTSN_Client::publish(const char topic[2], const char *payload, int8_t qos, boolean retain)
{
    uint16_t payloadLen = strlen(payload);
    publish(topic, (const uint8_t*)payload, payloadLen, qos, retain);
}

void MQTTSN_Client::publish(const char topic[2], const uint8_t *payload, uint16_t payloadLen, int8_t qos, boolean retain)
{
    uint8_t *headerPtr = this->transmitPayload();
    const uint8_t headerLen = 7;

    headerPtr[0] = headerLen + payloadLen;
    headerPtr[1] = MQTT_SN_TYPE_PUBLISH;
    headerPtr[2] = MQTT_SN_TOPIC_TYPE_SHORT;
    if (qos == -1) {
        headerPtr[2] |= MQTT_SN_FLAG_QOS_N1;
    } else {
        headerPtr[2] |= MQTT_SN_FLAG_QOS_0;
    }
    if (retain) {
        headerPtr[2] |= MQTT_SN_FLAG_RETAIN;
    }
    headerPtr[3] = topic[0];
    headerPtr[4] = topic[1];
    headerPtr[5] = 0x00;  // Message ID High
    headerPtr[6] = 0x00;  // Message ID Low

    // Copy in the payload
    memcpy(headerPtr + headerLen, payload, payloadLen);
    send(headerPtr[0], false);
}

void MQTTSN_Client::disconnect()
{
    // FIXME: implement disconnecting
    Serial.println("Disconnecting...");
}
