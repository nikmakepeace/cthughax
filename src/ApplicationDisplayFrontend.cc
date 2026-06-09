/** @file
 * Legacy display-frontend startup adapter.
 */

#include "ApplicationDisplayFrontend.h"

// Legacy display frontend startup hook implemented by the selected backend.
int cth_init(int* argc, char* argv[]);

int LegacyDisplayFrontendInitializer::initializeDisplayFrontend(
    int* argc, char* argv[]) {
    return cth_init(argc, argv);
}
