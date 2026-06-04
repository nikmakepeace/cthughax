#ifndef __SILENCE_MESSAGE_H
#define __SILENCE_MESSAGE_H

#include "DefaultMessagesProvider.h"
#include "FileMessagesProvider.h"
#include "QotdMessagesProvider.h"

#include <string>

struct MessagesConfig;

class SilenceMessage {
    DefaultMessagesProvider defaultMessages;
    FileMessagesProvider fileMessages;
    QotdMessagesProvider qotdMessages;
    int initialized;
    int qotdEnabled;

public:
    SilenceMessage();

    void configure(const MessagesConfig& messagesConfig);
    void initialize();
    void loadFile(const char* fname);
    void setQotdEnabled(int enabled);
    void setQotdServer(const char* server);
    std::string nextMessage();
};

#endif
