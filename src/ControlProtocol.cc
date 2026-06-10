/** @file
 * JSONL control protocol primitives shared by the visualiser and panel.
 */

#include "ControlProtocol.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>

#include <iomanip>
#include <sstream>

namespace {

class ControlJsonParser {
    const std::string& text;
    size_t pos;
    std::string errorValue;

    void setError(const char* message) {
        if (errorValue.empty())
            errorValue = message;
    }

    void skipWhitespace() {
        while (pos < text.size()
            && isspace(static_cast<unsigned char>(text[pos])))
            pos++;
    }

    bool consume(char expected) {
        skipWhitespace();
        if (pos >= text.size() || text[pos] != expected)
            return false;
        pos++;
        return true;
    }

    bool parseString(std::string* out) {
        if (!consume('"')) {
            setError("expected string");
            return false;
        }

        std::string result;
        while (pos < text.size()) {
            unsigned char c = static_cast<unsigned char>(text[pos++]);
            if (c == '"') {
                *out = result;
                return true;
            }
            if (c < 0x20) {
                setError("control character in string");
                return false;
            }
            if (c != '\\') {
                result.push_back(char(c));
                continue;
            }

            if (pos >= text.size()) {
                setError("unterminated escape");
                return false;
            }
            char escaped = text[pos++];
            switch (escaped) {
            case '"':
            case '\\':
            case '/':
                result.push_back(escaped);
                break;
            case 'b':
                result.push_back('\b');
                break;
            case 'f':
                result.push_back('\f');
                break;
            case 'n':
                result.push_back('\n');
                break;
            case 'r':
                result.push_back('\r');
                break;
            case 't':
                result.push_back('\t');
                break;
            case 'u':
                if (pos + 4 > text.size()) {
                    setError("short unicode escape");
                    return false;
                }
                for (int i = 0; i < 4; i++) {
                    if (!isxdigit(static_cast<unsigned char>(text[pos + i]))) {
                        setError("invalid unicode escape");
                        return false;
                    }
                }
                pos += 4;
                result.push_back('?');
                break;
            default:
                setError("invalid escape");
                return false;
            }
        }

        setError("unterminated string");
        return false;
    }

    bool parseNumber(ControlJsonValue* out) {
        skipWhitespace();
        const char* begin = text.c_str() + pos;
        char* end = 0;
        errno = 0;
        double value = strtod(begin, &end);
        if (end == begin || errno == ERANGE) {
            setError("invalid number");
            return false;
        }
        pos += size_t(end - begin);
        *out = ControlJsonValue::numberValueOf(value);
        return true;
    }

    bool parseArray(ControlJsonValue* out) {
        if (!consume('[')) {
            setError("expected array");
            return false;
        }

        ControlJsonValue result = ControlJsonValue::arrayValueOf();
        skipWhitespace();
        if (consume(']')) {
            *out = result;
            return true;
        }

        while (pos < text.size()) {
            ControlJsonValue item;
            if (!parseValue(&item))
                return false;
            result.append(item);
            skipWhitespace();
            if (consume(']')) {
                *out = result;
                return true;
            }
            if (!consume(',')) {
                setError("expected array comma");
                return false;
            }
        }

        setError("unterminated array");
        return false;
    }

    bool parseObject(ControlJsonValue* out) {
        if (!consume('{')) {
            setError("expected object");
            return false;
        }

        ControlJsonValue result = ControlJsonValue::objectValueOf();
        skipWhitespace();
        if (consume('}')) {
            *out = result;
            return true;
        }

        while (pos < text.size()) {
            std::string name;
            ControlJsonValue value;
            if (!parseString(&name))
                return false;
            if (!consume(':')) {
                setError("expected object colon");
                return false;
            }
            if (!parseValue(&value))
                return false;
            result.set(name, value);
            skipWhitespace();
            if (consume('}')) {
                *out = result;
                return true;
            }
            if (!consume(',')) {
                setError("expected object comma");
                return false;
            }
        }

        setError("unterminated object");
        return false;
    }

    bool parseLiteral(const char* literal, ControlJsonValue value,
        ControlJsonValue* out) {
        size_t i = 0;
        while (literal[i] != '\0') {
            if (pos + i >= text.size() || text[pos + i] != literal[i])
                return false;
            i++;
        }
        pos += i;
        *out = value;
        return true;
    }

public:
    explicit ControlJsonParser(const std::string& text_)
        : text(text_)
        , pos(0)
        , errorValue() { }

    bool parseValue(ControlJsonValue* out) {
        skipWhitespace();
        if (pos >= text.size()) {
            setError("expected value");
            return false;
        }

        char c = text[pos];
        if (c == '{')
            return parseObject(out);
        if (c == '[')
            return parseArray(out);
        if (c == '"') {
            std::string value;
            if (!parseString(&value))
                return false;
            *out = ControlJsonValue::stringValueOf(value);
            return true;
        }
        if (c == '-' || (c >= '0' && c <= '9'))
            return parseNumber(out);

        if (parseLiteral("true", ControlJsonValue::boolValueOf(true), out))
            return true;
        if (parseLiteral("false", ControlJsonValue::boolValueOf(false), out))
            return true;
        if (parseLiteral("null", ControlJsonValue::nullValue(), out))
            return true;

        setError("unexpected value");
        return false;
    }

    bool parse(ControlJsonValue* out, std::string* error) {
        if (!parseValue(out)) {
            if (error != 0)
                *error = errorValue;
            return false;
        }
        skipWhitespace();
        if (pos != text.size()) {
            if (error != 0)
                *error = "trailing characters";
            return false;
        }
        return true;
    }
};

static void appendEscaped(std::string& out, const std::string& value) {
    out.push_back('"');
    for (std::string::const_iterator it = value.begin(); it != value.end();
         ++it) {
        unsigned char c = static_cast<unsigned char>(*it);
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\b':
            out += "\\b";
            break;
        case '\f':
            out += "\\f";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (c < 0x20) {
                std::ostringstream escaped;
                escaped << "\\u" << std::hex << std::setw(4)
                        << std::setfill('0') << int(c);
                out += escaped.str();
            } else {
                out.push_back(char(c));
            }
            break;
        }
    }
    out.push_back('"');
}

static void appendSerialized(std::string& out, const ControlJsonValue& value) {
    switch (value.type()) {
    case ControlJsonValue::NullType:
        out += "null";
        break;
    case ControlJsonValue::BoolType:
        out += value.asBool() ? "true" : "false";
        break;
    case ControlJsonValue::NumberType: {
        std::ostringstream number;
        double numeric = value.asNumber();
        if (floor(numeric) == numeric)
            number << std::fixed << std::setprecision(0) << numeric;
        else
            number << std::setprecision(15) << numeric;
        out += number.str();
        break;
    }
    case ControlJsonValue::StringType:
        appendEscaped(out, value.asString());
        break;
    case ControlJsonValue::ArrayType: {
        out.push_back('[');
        const std::vector<ControlJsonValue>& items = value.asArray();
        for (size_t i = 0; i < items.size(); i++) {
            if (i != 0)
                out.push_back(',');
            appendSerialized(out, items[i]);
        }
        out.push_back(']');
        break;
    }
    case ControlJsonValue::ObjectType: {
        out.push_back('{');
        const std::vector<std::pair<std::string, ControlJsonValue> >& members
            = value.asObject();
        for (size_t i = 0; i < members.size(); i++) {
            if (i != 0)
                out.push_back(',');
            appendEscaped(out, members[i].first);
            out.push_back(':');
            appendSerialized(out, members[i].second);
        }
        out.push_back('}');
        break;
    }
    }
}

}

ControlJsonValue::ControlJsonValue()
    : typeValue(NullType)
    , boolValue(false)
    , numberValue(0.0)
    , stringValue()
    , arrayValue()
    , objectValue() { }

ControlJsonValue ControlJsonValue::nullValue() {
    return ControlJsonValue();
}

ControlJsonValue ControlJsonValue::boolValueOf(bool value) {
    ControlJsonValue result;
    result.typeValue = BoolType;
    result.boolValue = value;
    return result;
}

ControlJsonValue ControlJsonValue::numberValueOf(double value) {
    ControlJsonValue result;
    result.typeValue = NumberType;
    result.numberValue = value;
    return result;
}

ControlJsonValue ControlJsonValue::stringValueOf(const std::string& value) {
    ControlJsonValue result;
    result.typeValue = StringType;
    result.stringValue = value;
    return result;
}

ControlJsonValue ControlJsonValue::arrayValueOf() {
    ControlJsonValue result;
    result.typeValue = ArrayType;
    return result;
}

ControlJsonValue ControlJsonValue::objectValueOf() {
    ControlJsonValue result;
    result.typeValue = ObjectType;
    return result;
}

ControlJsonValue::Type ControlJsonValue::type() const {
    return typeValue;
}

bool ControlJsonValue::isNull() const {
    return typeValue == NullType;
}

bool ControlJsonValue::asBool(bool fallback) const {
    return typeValue == BoolType ? boolValue : fallback;
}

double ControlJsonValue::asNumber(double fallback) const {
    return typeValue == NumberType ? numberValue : fallback;
}

const std::string& ControlJsonValue::asString() const {
    return stringValue;
}

const std::vector<ControlJsonValue>& ControlJsonValue::asArray() const {
    return arrayValue;
}

const std::vector<std::pair<std::string, ControlJsonValue> >&
ControlJsonValue::asObject() const {
    return objectValue;
}

ControlJsonValue& ControlJsonValue::append(const ControlJsonValue& value) {
    if (typeValue != ArrayType) {
        typeValue = ArrayType;
        arrayValue.clear();
        objectValue.clear();
        stringValue.clear();
    }
    arrayValue.push_back(value);
    return *this;
}

ControlJsonValue& ControlJsonValue::set(
    const std::string& name, const ControlJsonValue& value) {
    if (typeValue != ObjectType) {
        typeValue = ObjectType;
        arrayValue.clear();
        objectValue.clear();
        stringValue.clear();
    }
    for (std::vector<std::pair<std::string, ControlJsonValue> >::iterator it
             = objectValue.begin();
         it != objectValue.end(); ++it) {
        if (it->first == name) {
            it->second = value;
            return *this;
        }
    }
    objectValue.push_back(std::make_pair(name, value));
    return *this;
}

const ControlJsonValue* ControlJsonValue::member(
    const std::string& name) const {
    if (typeValue != ObjectType)
        return 0;
    for (std::vector<std::pair<std::string, ControlJsonValue> >::const_iterator
             it = objectValue.begin();
         it != objectValue.end(); ++it) {
        if (it->first == name)
            return &it->second;
    }
    return 0;
}

ControlJsonValue* ControlJsonValue::member(const std::string& name) {
    if (typeValue != ObjectType)
        return 0;
    for (std::vector<std::pair<std::string, ControlJsonValue> >::iterator it
             = objectValue.begin();
         it != objectValue.end(); ++it) {
        if (it->first == name)
            return &it->second;
    }
    return 0;
}

bool parseControlJson(
    const std::string& text, ControlJsonValue* value, std::string* error) {
    ControlJsonParser parser(text);
    return parser.parse(value, error);
}

std::string serializeControlJson(const ControlJsonValue& value) {
    std::string out;
    appendSerialized(out, value);
    return out;
}

bool parseControlJsonLine(const std::string& line, ControlJsonValue* value,
    std::string* error, size_t maxBytes) {
    if (line.size() > maxBytes) {
        if (error != 0)
            *error = "json line too large";
        return false;
    }
    if (!line.empty() && line[line.size() - 1] == '\n') {
        return parseControlJson(line.substr(0, line.size() - 1), value, error);
    }
    return parseControlJson(line, value, error);
}

bool serializeControlJsonLine(const ControlJsonValue& value, std::string* line,
    std::string* error, size_t maxBytes) {
    std::string serialized = serializeControlJson(value);
    if (serialized.size() + 1 > maxBytes) {
        if (error != 0)
            *error = "json line too large";
        return false;
    }
    *line = serialized;
    line->push_back('\n');
    return true;
}

ControlJsonLineReader::ControlJsonLineReader(size_t maxBytes)
    : pendingValue()
    , maxBytesValue(maxBytes) { }

void ControlJsonLineReader::clear() {
    pendingValue.clear();
}

bool ControlJsonLineReader::feed(const char* data, size_t length,
    std::vector<ControlJsonValue>* messages, std::string* error) {
    for (size_t i = 0; i < length; i++) {
        char c = data[i];
        pendingValue.push_back(c);
        if (pendingValue.size() > maxBytesValue) {
            if (error != 0)
                *error = "json line too large";
            pendingValue.clear();
            return false;
        }
        if (c != '\n')
            continue;

        ControlJsonValue value;
        if (!parseControlJsonLine(pendingValue, &value, error,
                maxBytesValue)) {
            pendingValue.clear();
            return false;
        }
        if (messages != 0)
            messages->push_back(value);
        pendingValue.clear();
    }
    return true;
}

ControlJsonValue controlAckMessage(int id) {
    ControlJsonValue result = ControlJsonValue::objectValueOf();
    result.set("v", ControlJsonValue::numberValueOf(1));
    result.set("type", ControlJsonValue::stringValueOf("ack"));
    result.set("id", ControlJsonValue::numberValueOf(id));
    return result;
}

ControlJsonValue controlErrorMessage(int id, const std::string& code,
    const std::string& message) {
    ControlJsonValue result = ControlJsonValue::objectValueOf();
    result.set("v", ControlJsonValue::numberValueOf(1));
    result.set("type", ControlJsonValue::stringValueOf("error"));
    result.set("id", ControlJsonValue::numberValueOf(id));
    result.set("code", ControlJsonValue::stringValueOf(code));
    result.set("message", ControlJsonValue::stringValueOf(message));
    return result;
}
