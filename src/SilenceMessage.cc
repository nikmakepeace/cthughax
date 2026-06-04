#include "cthugha.h"
#include "Configuration.h"
#include "SilenceMessage.h"

static const char* programNameFallback() {
#ifdef PACKAGE_NAME
    if (PACKAGE_NAME[0] != '\0')
        return PACKAGE_NAME;
#endif
    return "CthughaNix";
}

static double randomUnitInterval() {
    return double(rand()) / double(RAND_MAX);
}

SilenceMessage::SilenceMessage()
    : defaultMessages()
    , fileMessages()
    , qotdMessages()
    , initialized(0)
    , qotdEnabled(0) { }

void SilenceMessage::configure(const MessagesConfig& messagesConfig) {
    qotdMessages.configure(messagesConfig);
    loadFile(messagesConfig.quietMessageFile.c_str());
    setQotdEnabled(messagesConfig.qotdEnabled);
    setQotdServer(messagesConfig.qotdServer.c_str());
}

void SilenceMessage::initialize() {
    if (initialized)
        return;

    initialized = 1;
    defaultMessages.initialize();
}

void SilenceMessage::loadFile(const char* fname) {
    if (fname == 0 || fname[0] == '\0')
        return;

    fileMessages.loadFile(fname);
    defaultMessages.initialize();
    initialized = 1;
}

void SilenceMessage::setQotdEnabled(int enabled) {
    qotdEnabled = enabled ? 1 : 0;
}

void SilenceMessage::setQotdServer(const char* server) {
    qotdMessages.setServer(server);
}

std::string SilenceMessage::nextMessage() {
    initialize();

    std::string message;
    const double fortuneChance = fileMessages.selectionChance();
    if (randomUnitInterval() <= fortuneChance && fileMessages.randomMessage(message))
        return message;

    if (qotdEnabled) {
        if (qotdMessages.takeMessage(message)) {
            qotdMessages.request();
            return message;
        }

        qotdMessages.request();
    }

    if (fileMessages.randomMessage(message))
        return message;

    if (defaultMessages.randomMessage(message))
        return message;

    return programNameFallback();
}
