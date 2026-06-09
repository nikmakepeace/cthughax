/** @file
 * Runtime persistence port and ini-backed implementation.
 */

#ifndef CTHUGHA_RUNTIME_PERSISTENCE_H
#define CTHUGHA_RUNTIME_PERSISTENCE_H

class LogSink;
class RuntimeConfigRegistry;

/** Persists runtime state requested by runtime commands. */
class RuntimePersistence {
public:
    /** Destroys the persistence interface. */
    virtual ~RuntimePersistence() { }

    /**
     * Writes the current runtime configuration.
     *
     * @return Zero on success, non-zero on persistence failure.
     */
    virtual int writeCurrentConfig() = 0;

    /**
     * Writes stop-and-continue state from the current runtime config snapshot.
     *
     * @return Zero on success, non-zero on persistence failure.
     */
    virtual int writeContinuation() = 0;
};

/** Ini-file implementation of runtime persistence. */
class IniRuntimePersistence : public RuntimePersistence {
    RuntimeConfigRegistry& runtimeConfigRegistry;
    LogSink& log;

public:
    /**
     * Creates ini-backed persistence using a current-config registry.
     *
     * @param runtimeConfigRegistry_ Registry used for current-config snapshots.
     * @param log_ Logging sink for ini write diagnostics.
     */
    explicit IniRuntimePersistence(
        RuntimeConfigRegistry& runtimeConfigRegistry_, LogSink& log_);

    /**
     * Writes the current runtime config to the automatic ini file.
     *
     * @return Zero on success, non-zero on persistence failure.
     */
    virtual int writeCurrentConfig();

    /**
     * Writes the continuation ini file from the current registry snapshot.
     *
     * @return Zero on success, non-zero on persistence failure.
     */
    virtual int writeContinuation();
};

#endif
