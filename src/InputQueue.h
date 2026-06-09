// -*- C++ -*-

#ifndef CTHUGHA_INPUT_QUEUE_H
#define CTHUGHA_INPUT_QUEUE_H

#include <deque>
#include <stddef.h>

struct InputConfig;

/** Sink for raw platform key events produced by display backends. */
class InputEventSink {
public:
    virtual ~InputEventSink() { }

    /**
     * Publishes raw key text plus shift state from a platform event.
     *
     * @param text Platform key text or key-symbol name.
     * @param shifted Nonzero when the Shift modifier was active.
     */
    virtual void pushRawKey(const char* text, int shifted) = 0;
};

/** Converts platform key text into Cthugha key codes. */
class KeyTranslator {
    int escapeKeyEnabled;

public:
    /** Creates a translator with ESC disabled. */
    KeyTranslator();

    /** Configures translation policy from startup input configuration. */
    void configure(const InputConfig& config);

    /**
     * Converts one raw key event.
     *
     * @return Cthugha key code, or CK_NONE when the raw event is ignored.
     */
    int translate(const char* text, int shifted) const;
};

/** FIFO queue of normalized key codes for the active interface. */
class InputQueue : public InputEventSink {
    KeyTranslator translator;
    std::deque<int> keys;

public:
    /** Configures the owned key translator from startup input configuration. */
    void configure(const InputConfig& config);

    /** Translates and appends a raw platform key event. */
    virtual void pushRawKey(const char* text, int shifted);

    /** Appends an already-normalized key code. */
    void pushKey(int key);

    /**
     * Pops the next key.
     *
     * @return Next key code, or CK_NONE when the queue is empty.
     */
    int popKey();

    /** @return Nonzero when no keys are queued. */
    int empty() const;

    /** @return Number of queued key codes. */
    size_t size() const;
};

#endif
