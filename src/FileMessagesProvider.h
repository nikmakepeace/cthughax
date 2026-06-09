#ifndef CTHUGHA_FILE_MESSAGES_PROVIDER_H
#define CTHUGHA_FILE_MESSAGES_PROVIDER_H

#include "MessagesProvider.h"

#include <string>
#include <vector>

class FileMessagesProvider : public MessagesProvider {
    std::vector<std::string> messages;

    int loadFileFromPath(const char* path);

public:
    FileMessagesProvider();

    int loadFile(const char* path);

    double selectionChance() const;

    int count() const override;
    int randomMessage(RandomSource& randomSource, std::string& message) const override;
};

#endif
