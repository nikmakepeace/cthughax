#ifndef CTHUGHA_DEFAULT_MESSAGES_PROVIDER_H
#define CTHUGHA_DEFAULT_MESSAGES_PROVIDER_H

#include "MessagesProvider.h"

#include <string>
#include <vector>

class DefaultMessagesProvider : public MessagesProvider {
    std::vector<std::string> messages;
    int initialized;

public:
    DefaultMessagesProvider();

    void initialize();

    int count() const override;
    int randomMessage(RandomSource& randomSource, std::string& message) const override;
};

#endif
