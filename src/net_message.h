#pragma once
#include "bit_stream.h"

#if 0
//---------------------------------------------
// Examples
//---------------------------------------------
// fist, idle          : 00 0. ...
// fist, walking N     : 00 10 000
// sword, attacking SE : 01 11 101
//---------------------------------------------
struct NetMessage_PlayerInput {
    // 0       - idle       (no bits will follow)
    // 1       - moving     (running bit will follow)
    // 1 0     - walking    (direction bits will follow)
    // 1 1     - running    (direction bits will follow)
    // 1 * 000 - north
    // 1 * 001 - east
    // 1 * 010 - south
    // 1 * 011 - west
    // 1 * 100 - northeast
    // 1 * 101 - southeast
    // 1 * 110 - southwest
    // 1 * 111 - northwest
    unsigned int moving : 1;
    unsigned int running : 1;
    unsigned int direction : 3;

    // 0 - none             (no bits will follow)
    // 1 - attacking
    unsigned int attacking : 1;

    // 00 - PlayerInventorySlot_1
    // 01 - PlayerInventorySlot_2
    // 10 - PlayerInventorySlot_3
    // 11 - <unused>
    unsigned int selectSlot : 2;
};
#endif

struct NetMessage {
    enum class Type {
        Unknown,
        Identify,
        Welcome,
        ChatMessage,
        PlayerState,
        Count
    };

    // TODO: sequence number
    //size_t sequenceNumber;
    Type type = Type::Unknown;

    virtual ~NetMessage() {}
    size_t Serialize(uint32_t *buffer, size_t bufferLength) const;
    static NetMessage &Deserialize(uint32_t *buffer, size_t bufferLength);

protected:
    NetMessage(Type type) : type(type) {};

    virtual void Serialize(BitStreamWriter &writer) const;
    virtual void Deserialize(BitStreamReader &reader) = 0;
};

struct NetMessage_Identify : public NetMessage {
    size_t       usernameLength {};
    const char * username       {};
    // TODO: Encrypt packet
    //size_t passwordLength;
    //const char *password;

    NetMessage_Identify() : NetMessage(Type::Identify) {};
    using NetMessage::Serialize;

protected:
    void Serialize(BitStreamWriter &writer) const override;
    void Deserialize(BitStreamReader &reader) override;
};

struct NetMessage_Welcome : public NetMessage  {
    //size_t unused;

    NetMessage_Welcome() : NetMessage(Type::Welcome) {};
    using NetMessage::Serialize;

protected:
    void Serialize(BitStreamWriter &writer) const override;
    void Deserialize(BitStreamReader &reader) override;
};

struct NetMessage_ChatMessage : public NetMessage  {
    size_t       usernameLength {};
    const char * username       {};
    size_t       messageLength  {};
    const char * message        {};

    NetMessage_ChatMessage() : NetMessage(Type::ChatMessage) {};
    using NetMessage::Serialize;

protected:
    void Serialize(BitStreamWriter &writer) const override;
    void Deserialize(BitStreamReader &reader) override;
};