/**
 * @file File that contains the defines and macros specially for the tmStreamer project
 * @author barn
 * @version 20130621
 */
#pragma once

///////////////////////////////////////////////////////////////////////////////
// INCLUDES project headers

///////////////////////////////////////////////////////////////////////////////
//INCLUDES C/C++ standard library (and other external libraries)

///////////////////////////////////////////////////////////////////////////////
// DEFINES and MACROS

#define OGRE_FRAME_SMOOTHING_PERIOD 0.5f

// a simple resource that will be used by the compositors
#define RESOURCE_OMEGA_SIMPLE_TEXTURE "omega/material/simple/texture"

#ifdef NDEBUG //release
	#define OGRE_PLUGIN_FILE "plugins.cfg"
	#define OGRE_RES_FILE "resources.cfg"
#else // debug
	#define OGRE_PLUGIN_FILE "plugins_d.cfg"
	#define OGRE_RES_FILE "resources_d.cfg"
#endif

#define OGRE_CONFIG_FILE "ogre.cfg"
#define TM_STREAMER_CONFIG_FILE "tmStreamer.cfg"
#define TM_STREAMER_WINDOW_PARAMETER_SECTION "Main Window"
#define LOG_FILE "tmStreamer.log"

///////////////////////////////////////////////////////////////////////////////
// NAMESPACE, CONSTANTS and TYPE DECLARATIONS/IMPLEMENTATIONS
