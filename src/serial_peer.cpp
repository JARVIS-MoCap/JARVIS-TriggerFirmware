/*******************************************************************************
 * File:        main.cpp
 * Created:     18. July 2022
 * Author:      Louis Frank
 * Contact:     singulosta@gmail.com
 * Copyright:   2021 Louis Frank
 * License:     LGPL v3.0
 ******************************************************************************/

#include "serial_peer.h"
#include "serial_messages.h"

#include <Arduino.h>

SerialPeer::SerialPeer() {}

uint8_t SerialPeer::calculateCrc(uint8_t *buffer, size_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc += buffer[i];
    }
    return crc;
}

uint8_t SerialPeer::handleMessage(uint8_t *msg, size_t len) {
    // Variables
    uint8_t error_flags = 0;
    message *type_message;
    type_message = (message *)msg;

    uint8_t length = type_message->header.length;
    if (len < MIN_LENGTH_MESSAGE || len - LENGTH_MSG_HEADER != length) {
        error_flags |= SERIAL_PEER_ERROR_LENGTH;
    }
    uint8_t *payload = type_message->value;
    uint8_t crc = this->calculateCrc(payload, length);
    if (type_message->header.crc != crc) {
        error_flags |= SERIAL_PEER_ERROR_CRC;
    }

    switch (type_message->header.type) {
    case TYPE_ECHO:
        this->_buffer[0] = TYPE_ECHO;
        memcpy(this->_buffer, type_message, len);
        sendMessage((uint8_t *)type_message, len);
        break;
    case TYPE_SETUP:
        if (len != LENGTH_SETUP_MESSAGE) {
            error_flags |= SERIAL_PEER_ERROR_LENGTH;
            sendTxt((uint8_t *)" test ", 7);
        }
        if (!error_flags) {
            handleSetup((setup_message *)type_message, len);
            sendAck();
        }
        break;
    case TYPE_INPUTS:
        // Not implemented
        error_flags |= SERIAL_PEER_ERROR_NOT_IMPLEMENTED;
        break;
    case TYPE_ACK:
        // Not implemented
        // TODO handle acks
        // error_flags |= SERIAL_PEER_ERROR_NOT_IMPLEMENTED;
        break;
    case TYPE_ERROR:
        // Not implemented
        error_flags |= SERIAL_PEER_ERROR_NOT_IMPLEMENTED;
        break;
    default:
        // unknown packet type
        error_flags |= SERIAL_PEER_ERROR_UNKNOWN_PACKET;
        break;
    }
    if (error_flags) {
        uint8_t error_str[SERIAL_PEER_MAX_BUFFER_SIZE];
        uint16_t error_len = 0;

        if (error_flags & SERIAL_PEER_ERROR_CRC) {
            error_len +=
                snprintf((char *)error_str + error_len,
                         SERIAL_PEER_MAX_BUFFER_SIZE - error_len,
                         " # CRC ERROR / header.crc: %d / calc crc: %d",
                         type_message->header.crc, crc);
        }
        if (error_flags & SERIAL_PEER_ERROR_LENGTH) {
            error_len += snprintf((char *)error_str + error_len,
                                  SERIAL_PEER_MAX_BUFFER_SIZE - error_len,
                                  " # LENGHT ERROR / header.length: %d / len: "
                                  "%d / MIN_LENGTH_MESSAGE: %d",
                                  length, len, MIN_LENGTH_MESSAGE);
        }
        if (error_flags & SERIAL_PEER_ERROR_NOT_IMPLEMENTED) {
            error_len += snprintf((char *)error_str + error_len,
                                  SERIAL_PEER_MAX_BUFFER_SIZE - error_len,
                                  " # NOT IMPLEMENTED ERROR ");
        }
        if (error_flags & SERIAL_PEER_ERROR_UNKNOWN_PACKET) {
            error_len += snprintf((char *)error_str + error_len,
                                  SERIAL_PEER_MAX_BUFFER_SIZE - error_len,
                                  " # UNKNOWN PACKET ERROR / header.type: %d",
                                  type_message->header.type);
        }

        sendError(error_str, error_len);
    }

    return error_flags;
}

void SerialPeer::handleSetup(setup_message *msg, size_t len) {
    _setup.delay_us = msg->delay_us;
    _setup.pulse_limit = msg->pulse_limit;
    _setup.pulse_hz = msg->pulse_hz;
    _setup.flags = msg->flags;
    _setup_changed = true;
}

uint8_t SerialPeer::getSetup(SetupStruct *setup) {
    if (!this->_setup_changed) {
        return false;
    }
    this->_setup_changed = false;
    memcpy(setup, &(this->_setup), LENGTH_SETUP_STRUCT);
    return true;
}

void SerialPeer::sendMessage(uint8_t *msg, size_t len) {
    this->_sendPacketFunction(msg, len);
}

void SerialPeer::sendAck() {
    ack_message *msg;
    msg = (ack_message *)this->_buffer;

    msg->header.type = TYPE_ACK;
    msg->header.length = 0;
    msg->header.crc = 0;

    sendMessage((uint8_t *)msg, LENGTH_ACK_MESSAGE);
}

void SerialPeer::sendError(uint8_t *value, uint8_t len) {
    _sendTypedMessage(TYPE_ERROR, value, len);
}

void SerialPeer::sendTxt(uint8_t *value, uint8_t len) {
    _sendTypedMessage(TYPE_TXT, value, len);
}

void SerialPeer::_sendTypedMessage(uint8_t type, uint8_t *value, uint8_t len) {
    message *msg;
    msg = (message *)this->_buffer;

    memcpy(msg->value, value, len);

    msg->header.type = type;
    msg->header.length = len;
    msg->header.crc = calculateCrc(msg->value, msg->header.length);

    sendMessage((uint8_t *)msg, MIN_LENGTH_TXT_MESSAGE + len);
}

void SerialPeer::sendInputs(uint32_t uptime_us, uint32_t pulse_id,
                            uint8_t inputs_state) {
    input_state_message *msg;
    msg = (input_state_message *)this->_buffer;

    msg->uptime_us = uptime_us;
    msg->pulse_id = pulse_id;
    msg->inputs_state = inputs_state;

    msg->header.type = TYPE_INPUTS;
    msg->header.length = LENGTH_INPUT_STATE_MESSAGE - LENGTH_MSG_HEADER;
    msg->header.crc =
        calculateCrc((uint8_t *)msg + LENGTH_MSG_HEADER, msg->header.length);

    sendMessage((uint8_t *)msg, LENGTH_INPUT_STATE_MESSAGE);
}
