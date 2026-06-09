#include "cthugha.h"
#include "DefaultMessagesProvider.h"
#include "MessageFormatValidator.h"
#include "ProcessServices.h"

static const char* defaultSilenceMessages[] = {
    "Where is the music?",
    "JOLT !",
    "Turn The Music On",
    "Lets Party!!!",
    "Pink Floyd Rules",
    "Sounds of Silence ?",
    "The Torps",
    "Study Mathematics",
    "Visit Linz",
    "Press ? for help",
    "Number 5 is ALIVE!!",
    "Spooky Mulder.....",
    "Math is Power",
    "Subliminal Ads",
    "Read a book",
    "Get a life...",
    "SMILE!",
    "Cthugha-L",
    "Don't Panic!"
};

DefaultMessagesProvider::DefaultMessagesProvider()
    : messages()
    , initialized(0) { }

void DefaultMessagesProvider::initialize() {
    if (initialized)
        return;

    initialized = 1;
    messages.clear();

    for (unsigned int i = 0; i < sizeof(defaultSilenceMessages) / sizeof(defaultSilenceMessages[0]); i++) {
        std::string validated;
        if (MessageFormatValidator::validateLine(defaultSilenceMessages[i], validated,
                "default silence messages", int(i) + 1))
            messages.push_back(validated);
    }
}

int DefaultMessagesProvider::count() const {
    return int(messages.size());
}

int DefaultMessagesProvider::randomMessage(RandomSource& randomSource,
    std::string& message) const {
    if (messages.empty())
        return 0;

    message = messages[randomSource.uniformInt(int(messages.size()))];
    return 1;
}
