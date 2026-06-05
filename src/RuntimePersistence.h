/** @file
 * Runtime persistence port and ini-backed implementation.
 */

#ifndef CTHUGHA_RUNTIME_PERSISTENCE_H
#define CTHUGHA_RUNTIME_PERSISTENCE_H

class RuntimeConfigRegistry;
struct RuntimeContinuationState;

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
     * Writes stop-and-continue runtime state.
     *
     * @param continuation Continuation state to persist.
     * @return Zero on success, non-zero on persistence failure.
     */
    virtual int writeContinuation(
        const RuntimeContinuationState& continuation) = 0;
};

/** Ini-file implementation of runtime persistence. */
class IniRuntimePersistence : public RuntimePersistence {
    RuntimeConfigRegistry& runtimeConfigRegistry;

public:
    /**
     * Creates ini-backed persistence using a current-config registry.
     *
     * @param runtimeConfigRegistry_ Registry used for current-config snapshots.
     */
    explicit IniRuntimePersistence(
        RuntimeConfigRegistry& runtimeConfigRegistry_);

    /**
     * Writes the current runtime config to the automatic ini file.
     *
     * @return Zero on success, non-zero on persistence failure.
     */
    virtual int writeCurrentConfig();

    /**
     * Writes the continuation ini file.
     *
     * @param continuation Continuation state to persist.
     * @return Zero on success, non-zero on persistence failure.
     */
    virtual int writeContinuation(
        const RuntimeContinuationState& continuation);
};

#endif
