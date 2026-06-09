/** @file
 * Display-frontend startup port for Application.
 */

#ifndef CTHUGHA_APPLICATION_DISPLAY_FRONTEND_H
#define CTHUGHA_APPLICATION_DISPLAY_FRONTEND_H

/**
 * Starts the selected graphical display frontend.
 */
class DisplayFrontendInitializer {
public:
    /** Destroys the display frontend startup interface. */
    virtual ~DisplayFrontendInitializer() { }

    /**
     * Initializes the display frontend.
     *
     * @param argc Mutable display argument count.
     * @param argv Mutable display argument vector.
     * @return Zero on success, nonzero on display startup failure.
     */
    virtual int initializeDisplayFrontend(int* argc, char* argv[]) = 0;
};

/**
 * Adapter for the legacy selected-frontend startup hook.
 */
class LegacyDisplayFrontendInitializer : public DisplayFrontendInitializer {
public:
    /**
     * Calls the selected frontend's historical startup hook.
     *
     * @param argc Mutable display argument count.
     * @param argv Mutable display argument vector.
     * @return Zero on success, nonzero on display startup failure.
     */
    virtual int initializeDisplayFrontend(int* argc, char* argv[]);
};

#endif
