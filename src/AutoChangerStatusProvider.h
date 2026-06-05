/** @file
 * Read-only status seam for automatic scene-change state.
 */

#ifndef __AUTO_CHANGER_STATUS_PROVIDER_H
#define __AUTO_CHANGER_STATUS_PROVIDER_H

/**
 * Provides short status text for the active automatic scene changer.
 *
 * The interface lets UI code display auto-change progress without owning or
 * reaching into the policy object that performs automatic runtime commands.
 */
class AutoChangerStatusProvider {
public:
    /** Releases the status provider interface. */
    virtual ~AutoChangerStatusProvider() { }

    /**
     * Returns current automatic scene-change status text.
     *
     * @return Pointer to provider-owned text valid until the next status call.
     */
    virtual const char* autoChangerStatus() const = 0;
};

#endif
