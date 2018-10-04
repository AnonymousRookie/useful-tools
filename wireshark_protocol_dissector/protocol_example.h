#ifndef PROTOCOL_EXAMPLE_H
#defein PROTOCOL_EXAMPLE_H

#define MaxDataSize 1024

#pragma pack(1)

struct MsgHead {
    int msgNo_;
    unsigned char msgType_;
    unsigned char subMsgType_;
    short dataLen_;
};

struct MsgStruct {
    MsgHead msgHead_;
    unsigned char data_[MaxDataSize];
};

#pragma pack()

#endif  // PROTOCOL_EXAMPLE_H