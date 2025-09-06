// PluginInit.h
#ifndef MONGODBMODULE_PLUGININIT_H
#define MONGODBMODULE_PLUGININIT_H

/**
 * @brief Plugin entry point for MongoDBModule.
 * Called when the shared library is loaded.
 */
extern "C" void plugin_init();

#endif // MONGODBMODULE_PLUGININIT_H