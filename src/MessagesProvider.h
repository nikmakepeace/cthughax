#ifndef CTHUGHA_MESSAGES_PROVIDER_H
#define CTHUGHA_MESSAGES_PROVIDER_H

#include <string>

class RandomSource;

class MessagesProvider {
public:
    virtual ~MessagesProvider();

    virtual int count() const = 0;
    virtual int randomMessage(RandomSource& randomSource, std::string& message) const = 0;
};

#endif
