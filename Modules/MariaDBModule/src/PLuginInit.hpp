#include "MariaDBModule.hpp"
#include "Modules/ModuleManager.hpp"
#include "Symbols/Value.hpp"

extern "C" void plugin_init() {
    auto & mgr = Modules::ModuleManager::instance();

    mgr.registerFunction("curlGET", CallbackFunction(&CurlModule::curlGet));
    mgr.registerFunction("curlPOST", CallbackFunction(&CurlModule::curlPost));

    // curl alias, ha szükséges
    mgr.registerFunction("curl", &CurlModule::curlGet);  // vagy írj külön wrapper-t
}
