#include "cthugha.h"
#include "FileMessagesProvider.h"
#include "MessageFormatValidator.h"
#include "ProcessServices.h"

#include <fstream>
#include <string.h>

FileMessagesProvider::FileMessagesProvider()
    : messages() { }

static std::string stripLineEnding(const std::string& line) {
    if (!line.empty() && line[line.size() - 1] == '\r')
        return line.substr(0, line.size() - 1);

    return line;
}

static int fortuneSeparatorLine(const std::string& line) {
    return stripLineEnding(line) == "%";
}

static int fortuneRecordLineEmpty(const std::string& line) {
    return stripLineEnding(line).empty();
}

static int fortuneLineWhitespace(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\f' || c == '\v';
}

static std::string trimFortuneLine(const std::string& line) {
    std::string stripped = stripLineEnding(line);
    unsigned int first = 0;
    unsigned int last = stripped.size();

    while (first < last && fortuneLineWhitespace((unsigned char)stripped[first]))
        first++;
    while (last > first && fortuneLineWhitespace((unsigned char)stripped[last - 1]))
        last--;

    return stripped.substr(first, last - first);
}

static std::string trimFortuneLineLeft(const std::string& line) {
    std::string stripped = stripLineEnding(line);
    unsigned int first = 0;

    while (first < stripped.size() && fortuneLineWhitespace((unsigned char)stripped[first]))
        first++;

    return stripped.substr(first);
}

static int fortuneLineHasIndent(const std::string& line) {
    std::string stripped = stripLineEnding(line);
    return !stripped.empty() && fortuneLineWhitespace((unsigned char)stripped[0]);
}

static int fortuneLineStartsWith(const std::string& line, const char* prefix) {
    return line.compare(0, strlen(prefix), prefix) == 0;
}

static int fortuneAttributionStart(const std::string& line) {
    if (!fortuneLineHasIndent(line))
        return 0;

    std::string trimmed = trimFortuneLineLeft(line);
    return fortuneLineStartsWith(trimmed, "--")
        || fortuneLineStartsWith(trimmed, "- ")
        || fortuneLineStartsWith(trimmed, "\xE2\x80\x94")
        || fortuneLineStartsWith(trimmed, "\xE2\x80\x95");
}

static void appendWordWrappedLine(std::string& destination, int& destinationLineNumber,
    const std::string& line, int lineNumber) {
    if (line.empty())
        return;

    if (destination.empty())
        destinationLineNumber = lineNumber;
    if (!destination.empty())
        destination += ' ';
    destination += line;
}

static void finishShapedLine(std::vector<std::string>& shapedLines,
    std::vector<int>& shapedLineNumbers, std::string& currentLine,
    int& currentLineNumber) {
    if (currentLine.empty())
        return;

    shapedLines.push_back(currentLine);
    shapedLineNumbers.push_back(currentLineNumber);
    currentLine.clear();
    currentLineNumber = 0;
}

static void maybeAppendParagraphBreak(std::vector<std::string>& shapedLines,
    std::vector<int>& shapedLineNumbers, int lineNumber) {
    if (!shapedLines.empty() && !shapedLines[shapedLines.size() - 1].empty()) {
        shapedLines.push_back("");
        shapedLineNumbers.push_back(lineNumber);
    }
}

static void shapeFortuneRecord(const std::vector<std::string>& recordLines,
    const std::vector<int>& recordLineNumbers,
    std::vector<std::string>& shapedLines,
    std::vector<int>& shapedLineNumbers) {
    std::string currentLine;
    int currentLineNumber = 0;
    int currentIsAttribution = 0;
    int pendingParagraphBreak = 0;
    int pendingParagraphLineNumber = 0;

    for (unsigned int i = 0; i < recordLines.size(); i++) {
        if (fortuneRecordLineEmpty(recordLines[i])) {
            finishShapedLine(shapedLines, shapedLineNumbers, currentLine, currentLineNumber);
            currentIsAttribution = 0;
            pendingParagraphBreak = !shapedLines.empty();
            pendingParagraphLineNumber = recordLineNumbers[i];
            continue;
        }

        if (fortuneAttributionStart(recordLines[i])) {
            finishShapedLine(shapedLines, shapedLineNumbers, currentLine, currentLineNumber);
            if (pendingParagraphBreak)
                maybeAppendParagraphBreak(shapedLines, shapedLineNumbers,
                    pendingParagraphLineNumber);
            currentLine = trimFortuneLineLeft(recordLines[i]);
            currentLineNumber = recordLineNumbers[i];
            currentIsAttribution = 1;
            pendingParagraphBreak = 0;
            continue;
        }

        if (currentIsAttribution && fortuneLineHasIndent(recordLines[i])) {
            appendWordWrappedLine(currentLine, currentLineNumber, trimFortuneLine(recordLines[i]),
                recordLineNumbers[i]);
            pendingParagraphBreak = 0;
            continue;
        }

        if (currentIsAttribution) {
            finishShapedLine(shapedLines, shapedLineNumbers, currentLine, currentLineNumber);
            currentIsAttribution = 0;
        }

        if (pendingParagraphBreak)
            maybeAppendParagraphBreak(shapedLines, shapedLineNumbers,
                pendingParagraphLineNumber);

        appendWordWrappedLine(currentLine, currentLineNumber, trimFortuneLine(recordLines[i]),
            recordLineNumbers[i]);
        pendingParagraphBreak = 0;
    }

    finishShapedLine(shapedLines, shapedLineNumbers, currentLine, currentLineNumber);
}

static int appendValidatedFortuneRecord(const char* path,
    const std::vector<std::string>& recordLines,
    const std::vector<int>& recordLineNumbers,
    std::vector<std::string>& loadedMessages) {
    unsigned int first = 0;
    unsigned int last = recordLines.size();

    while (first < last && fortuneRecordLineEmpty(recordLines[first]))
        first++;
    while (last > first && fortuneRecordLineEmpty(recordLines[last - 1]))
        last--;

    if (first >= last)
        return 0;

    std::vector<std::string> shapedLines;
    std::vector<int> shapedLineNumbers;
    std::vector<std::string> trimmedRecordLines;
    std::vector<int> trimmedRecordLineNumbers;
    for (unsigned int i = first; i < last; i++) {
        trimmedRecordLines.push_back(recordLines[i]);
        trimmedRecordLineNumbers.push_back(recordLineNumbers[i]);
    }

    shapeFortuneRecord(trimmedRecordLines, trimmedRecordLineNumbers, shapedLines,
        shapedLineNumbers);
    if (shapedLines.empty())
        return 0;

    std::string message;
    unsigned int displayCharacters = 0;

    for (unsigned int i = 0; i < shapedLines.size(); i++) {
        if (i > 0)
            message += '\n';

        if (shapedLines[i].empty())
            continue;

        std::string validated;
        if (!MessageFormatValidator::validateLine(shapedLines[i], validated, path,
                shapedLineNumbers[i]))
            return 0;

        displayCharacters += validated.size();
        if (displayCharacters > MessageFormatValidator::MaxMessageCharacters) {
            CTH_WARN("%s:%d: rejected silence message: longer than %d CP437 characters.\n",
                path, shapedLineNumbers[i], int(MessageFormatValidator::MaxMessageCharacters));
            return 0;
        }

        message += validated;
    }

    if (!message.empty())
        loadedMessages.push_back(message);

    return !message.empty();
}

int FileMessagesProvider::loadFileFromPath(const char* path) {
    if (path == 0 || path[0] == '\0')
        return 0;

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        CTH_ERRNO(errno, "Can't open quiet strings file `%s'.", path);
        return 0;
    }

    std::vector<std::string> loadedMessages;
    std::string line;
    int lineNumber = 0;
    std::vector<std::string> recordLines;
    std::vector<int> recordLineNumbers;
    while (std::getline(input, line)) {
        lineNumber++;

        if (fortuneSeparatorLine(line)) {
            appendValidatedFortuneRecord(path, recordLines, recordLineNumbers, loadedMessages);
            recordLines.clear();
            recordLineNumbers.clear();
            continue;
        }

        recordLines.push_back(line);
        recordLineNumbers.push_back(lineNumber);
    }

    appendValidatedFortuneRecord(path, recordLines, recordLineNumbers, loadedMessages);

    messages = loadedMessages;

    if (messages.empty()) {
        CTH_WARN("silence message file `%s' contained no valid CP437 messages.\n", path);
        return 0;
    }

    CTH_INFO("Loaded %d silence messages from `%s'.\n",
        int(messages.size()), path);
    return 1;
}

int FileMessagesProvider::loadFile(const char* path) {
    return loadFileFromPath(path);
}

double FileMessagesProvider::selectionChance() const {
    int cappedCount = count();
    if (cappedCount > 100)
        cappedCount = 100;

    return double(cappedCount) / 200.0;
}

int FileMessagesProvider::count() const {
    return int(messages.size());
}

int FileMessagesProvider::randomMessage(RandomSource& randomSource,
    std::string& message) const {
    if (messages.empty())
        return 0;

    message = messages[randomSource.uniformInt(int(messages.size()))];
    return 1;
}
