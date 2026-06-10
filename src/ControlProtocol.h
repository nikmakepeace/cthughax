/** @file
 * JSONL control protocol primitives shared by the visualiser and panel.
 */

#ifndef CTHUGHA_CONTROL_PROTOCOL_H
#define CTHUGHA_CONTROL_PROTOCOL_H

#include <stddef.h>

#include <string>
#include <utility>
#include <vector>

static const size_t CTH_CONTROL_MAX_JSON_LINE_BYTES = 65536;

class ControlJsonValue {
public:
    enum Type {
        NullType,
        BoolType,
        NumberType,
        StringType,
        ArrayType,
        ObjectType
    };

private:
    Type typeValue;
    bool boolValue;
    double numberValue;
    std::string stringValue;
    std::vector<ControlJsonValue> arrayValue;
    std::vector<std::pair<std::string, ControlJsonValue> > objectValue;

public:
    ControlJsonValue();

    static ControlJsonValue nullValue();
    static ControlJsonValue boolValueOf(bool value);
    static ControlJsonValue numberValueOf(double value);
    static ControlJsonValue stringValueOf(const std::string& value);
    static ControlJsonValue arrayValueOf();
    static ControlJsonValue objectValueOf();

    Type type() const;
    bool isNull() const;
    bool asBool(bool fallback = false) const;
    double asNumber(double fallback = 0.0) const;
    const std::string& asString() const;
    const std::vector<ControlJsonValue>& asArray() const;
    const std::vector<std::pair<std::string, ControlJsonValue> >& asObject() const;

    ControlJsonValue& append(const ControlJsonValue& value);
    ControlJsonValue& set(
        const std::string& name, const ControlJsonValue& value);

    const ControlJsonValue* member(const std::string& name) const;
    ControlJsonValue* member(const std::string& name);
};

bool parseControlJson(
    const std::string& text, ControlJsonValue* value, std::string* error);

std::string serializeControlJson(const ControlJsonValue& value);

bool parseControlJsonLine(const std::string& line, ControlJsonValue* value,
    std::string* error,
    size_t maxBytes = CTH_CONTROL_MAX_JSON_LINE_BYTES);

bool serializeControlJsonLine(const ControlJsonValue& value, std::string* line,
    std::string* error,
    size_t maxBytes = CTH_CONTROL_MAX_JSON_LINE_BYTES);

class ControlJsonLineReader {
    std::string pendingValue;
    size_t maxBytesValue;

public:
    explicit ControlJsonLineReader(
        size_t maxBytes = CTH_CONTROL_MAX_JSON_LINE_BYTES);

    void clear();

    /**
     * Appends bytes and returns one decoded JSON line at a time.
     *
     * @return True when a complete line was parsed or more bytes are needed.
     *         False reports framing/JSON errors in error.
     */
    bool feed(const char* data, size_t length,
        std::vector<ControlJsonValue>* messages, std::string* error);
};

ControlJsonValue controlAckMessage(int id);
ControlJsonValue controlErrorMessage(int id, const std::string& code,
    const std::string& message);

#endif
