#include "cthugha.h"
#include "SilenceMessage.h"

static const char* programNameFallback() {
#ifdef PACKAGE_NAME
    if (PACKAGE_NAME[0] != '\0')
        return PACKAGE_NAME;
#endif
    return "CthughaNix";
}

static double randomUnitInterval(RandomSource& randomSource) {
    static const int scale = 1000000;
    return double(randomSource.uniformInt(scale)) / double(scale);
}

SilenceMessageConfig::SilenceMessageConfig()
    : quietMessageFile()
    , qotdEnabled(0)
    , qotdPrefetchTimeoutMs(0)
    , qotdServer()
    , qotdPort("17") { }

SilenceMessage::SilenceMessage()
    : defaultMessages()
    , fileMessages()
    , qotdMessages()
    , fallbackRandomSource()
    , randomSourceValue(&fallbackRandomSource)
    , initialized(0)
    , qotdEnabled(0) { }

void SilenceMessage::configure(const SilenceMessageConfig& config) {
    qotdMessages.configureDefaults(config.qotdServer, config.qotdPort,
        config.qotdPrefetchTimeoutMs);
    loadFile(config.quietMessageFile.c_str());
    setQotdEnabled(config.qotdEnabled);
    setQotdServer(config.qotdServer.c_str());
}

void SilenceMessage::initialize() {
    if (initialized)
        return;

    initialized = 1;
    defaultMessages.initialize();
}

void SilenceMessage::setRandomSource(RandomSource& randomSource) {
    randomSourceValue = &randomSource;
}

void SilenceMessage::setTimerFactory(CountdownTimerFactory& timerFactory) {
    qotdMessages.setTimerFactory(timerFactory);
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
    RandomSource& randomSource = *randomSourceValue;
    if (randomUnitInterval(randomSource) <= fortuneChance
        && fileMessages.randomMessage(randomSource, message))
        return message;

    if (qotdEnabled) {
        if (qotdMessages.takeMessage(message)) {
            qotdMessages.request();
            return message;
        }

        qotdMessages.request();
    }

    if (fileMessages.randomMessage(randomSource, message))
        return message;

    if (defaultMessages.randomMessage(randomSource, message))
        return message;

    return programNameFallback();
}
