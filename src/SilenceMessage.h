#ifndef __SILENCE_MESSAGE_H
#define __SILENCE_MESSAGE_H

#include "DefaultMessagesProvider.h"
#include "FileMessagesProvider.h"
#include "ProcessServices.h"
#include "QotdMessagesProvider.h"

#include <string>

/**
 * Explicit quiet-message and QOTD settings used by SilenceMessage.
 */
struct SilenceMessageConfig {
    std::string quietMessageFile;
    int qotdEnabled;
    int qotdPrefetchTimeoutMs;
    std::string qotdServer;
    std::string qotdPort;

    /** Creates disabled quiet-message provider settings. */
    SilenceMessageConfig();
};

class SilenceMessage {
    DefaultMessagesProvider defaultMessages;
    FileMessagesProvider fileMessages;
    QotdMessagesProvider qotdMessages;
    CStdRandomSource fallbackRandomSource;
    RandomSource* randomSourceValue;
    int initialized;
    int qotdEnabled;

public:
    /** Creates quiet-message policy with fallback process services. */
    SilenceMessage();

    /**
     * Applies startup quiet-message and QOTD configuration.
     *
     * @param config Explicit quiet-message and QOTD settings.
     */
    void configure(const SilenceMessageConfig& config);

    /** Initializes built-in/file-backed message providers once. */
    void initialize();

    /**
     * Installs the random source used to choose quiet messages.
     *
     * @param randomSource Application-owned random source.
     */
    void setRandomSource(RandomSource& randomSource);

    /**
     * Installs the timer factory used to bound QOTD prefetches.
     *
     * @param timerFactory Application-owned countdown timer factory.
     */
    void setTimerFactory(CountdownTimerFactory& timerFactory);

    /**
     * Loads validated quiet-message text from a file.
     *
     * @param fname Path to a quiet-message file.
     */
    void loadFile(const char* fname);

    /**
     * Enables or disables QOTD prefetches.
     *
     * @param enabled Nonzero to enable QOTD quiet messages.
     */
    void setQotdEnabled(int enabled);

    /**
     * Sets the QOTD server used by the provider.
     *
     * @param server Requested server text; may be NULL or blank.
     */
    void setQotdServer(const char* server);

    /** @return Next quiet-message text, falling back to built-ins. */
    std::string nextMessage();
};

#endif
