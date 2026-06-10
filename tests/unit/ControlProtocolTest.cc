/** @file
 * Unit coverage for JSONL control protocol primitives.
 */

#include "ControlProtocol.h"

#include <assert.h>
#include <string.h>

static void testParsesCommandObject() {
    ControlJsonValue value;
    std::string error;

    assert(parseControlJsonLine(
        "{\"v\":1,\"id\":12,\"op\":\"set\",\"target\":\"scene.flame\","
        "\"value\":\"fire\"}\n",
        &value, &error));

    assert(value.type() == ControlJsonValue::ObjectType);
    assert(value.member("v") != 0);
    assert(value.member("v")->asNumber() == 1);
    assert(value.member("id") != 0);
    assert(value.member("id")->asNumber() == 12);
    assert(value.member("op") != 0);
    assert(value.member("op")->asString() == "set");
    assert(value.member("target") != 0);
    assert(value.member("target")->asString() == "scene.flame");
    assert(value.member("value") != 0);
    assert(value.member("value")->asString() == "fire");
}

static void testSerializesStateAndCatalogs() {
    ControlJsonValue catalogEntry = ControlJsonValue::objectValueOf();
    catalogEntry.set("index", ControlJsonValue::numberValueOf(0));
    catalogEntry.set("name", ControlJsonValue::stringValueOf("fire"));
    catalogEntry.set("label", ControlJsonValue::stringValueOf("fire"));

    ControlJsonValue flameCatalog = ControlJsonValue::arrayValueOf();
    flameCatalog.append(catalogEntry);

    ControlJsonValue targets = ControlJsonValue::objectValueOf();
    targets.set("scene.flame", flameCatalog);

    ControlJsonValue message = ControlJsonValue::objectValueOf();
    message.set("v", ControlJsonValue::numberValueOf(1));
    message.set("type", ControlJsonValue::stringValueOf("catalogs"));
    message.set("targets", targets);

    std::string line;
    std::string error;
    assert(serializeControlJsonLine(message, &line, &error));
    assert(line
        == "{\"v\":1,\"type\":\"catalogs\",\"targets\":{\"scene.flame\":"
           "[{\"index\":0,\"name\":\"fire\",\"label\":\"fire\"}]}}\n");

    ControlJsonValue roundTrip;
    assert(parseControlJsonLine(line, &roundTrip, &error));
    assert(roundTrip.member("targets") != 0);
    const ControlJsonValue* parsedCatalog
        = roundTrip.member("targets")->member("scene.flame");
    assert(parsedCatalog != 0);
    assert(parsedCatalog->asArray().size() == 1);
    assert(parsedCatalog->asArray()[0].member("name")->asString() == "fire");
}

static void testRejectsOversizedLines() {
    ControlJsonValue value;
    std::string error;

    assert(!parseControlJsonLine("{\"v\":1}", &value, &error, 3));
    assert(error == "json line too large");
}

static void testRejectsMalformedJson() {
    ControlJsonValue value;
    std::string error;

    assert(!parseControlJsonLine("{\"v\":1", &value, &error));
    assert(!error.empty());
}

static void testLineReaderHandlesChunks() {
    ControlJsonLineReader reader;
    std::vector<ControlJsonValue> messages;
    std::string error;
    const char* first = "{\"v\":1,\"type\":\"ack\"";
    const char* second = ",\"id\":9}\n{\"v\":1,\"type\":\"ack\",\"id\":10}\n";

    assert(reader.feed(first, strlen(first), &messages, &error));
    assert(messages.empty());
    assert(reader.feed(second, strlen(second), &messages, &error));

    assert(messages.size() == 2);
    assert(messages[0].member("id")->asNumber() == 9);
    assert(messages[1].member("id")->asNumber() == 10);
}

static void testAckAndErrorHelpers() {
    ControlJsonValue ack = controlAckMessage(7);
    ControlJsonValue error = controlErrorMessage(8, "bad-target", "Nope");

    assert(ack.member("type")->asString() == "ack");
    assert(ack.member("id")->asNumber() == 7);
    assert(error.member("type")->asString() == "error");
    assert(error.member("code")->asString() == "bad-target");
    assert(error.member("message")->asString() == "Nope");
}

int main() {
    testParsesCommandObject();
    testSerializesStateAndCatalogs();
    testRejectsOversizedLines();
    testRejectsMalformedJson();
    testLineReaderHandlesChunks();
    testAckAndErrorHelpers();
    return 0;
}
