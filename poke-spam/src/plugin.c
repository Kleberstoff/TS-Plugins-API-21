/*
* TeamSpeak 3 demo plugin
*
* Copyright (c) 2008-2016 TeamSpeak Systems GmbH
*/

#ifdef _WIN32
#pragma warning (disable : 4100)  /* Disable Unreferenced parameter warning */
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "teamspeak/public_errors.h"
#include "teamspeak/public_errors_rare.h"
#include "teamspeak/public_definitions.h"
#include "teamspeak/public_rare_definitions.h"
#include "teamspeak/clientlib_publicdefinitions.h"
#include "ts3_functions.h"
#include "plugin.h"


int amount = 10; //default

static struct TS3Functions ts3Functions;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif

#define PLUGIN_API_VERSION 21

#define PATH_BUFSIZE 512
#define COMMAND_BUFSIZE 128
#define INFODATA_BUFSIZE 128
#define SERVERINFO_BUFSIZE 256
#define CHANNELINFO_BUFSIZE 512
#define RETURNCODE_BUFSIZE 128

static char* pluginID = NULL;

#ifdef _WIN32
/* Helper function to convert wchar_T to Utf-8 encoded strings on Windows */
static int wcharToUtf8(const wchar_t* str, char** result) {
	int outlen = WideCharToMultiByte(CP_UTF8, 0, str, -1, 0, 0, 0, 0);
	*result = (char*)malloc(outlen);
	if (WideCharToMultiByte(CP_UTF8, 0, str, -1, *result, outlen, 0, 0) == 0) {
		*result = NULL;
		return -1;
	}
	return 0;
}
#endif

/*********************************** Required functions ************************************/
/*
* If any of these required functions is not implemented, TS3 will refuse to load the plugin
*/

/* Unique name identifying this plugin */
const char* ts3plugin_name() {
#ifdef _WIN32
	/* TeamSpeak expects UTF-8 encoded characters. Following demonstrates a possibility how to convert UTF-16 wchar_t into UTF-8. */
	static char* result = NULL;  /* Static variable so it's allocated only once */
	if (!result) {
		const wchar_t* name = L"Poke-Spam";
		if (wcharToUtf8(name, &result) == -1) {  /* Convert name into UTF-8 encoded result */
			result = "Test Plugin";  /* Conversion failed, fallback here */
		}
	}
	return result;
#else
	return "Poke-Spam";
#endif
}

/* Plugin version */
const char* ts3plugin_version() {
	return "1.0";
}

/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
int ts3plugin_apiVersion() {
	return PLUGIN_API_VERSION;
}

/* Plugin author */
const char* ts3plugin_author() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
	return "Kleberstoff & Skokkk";
}

/* Plugin description */
const char* ts3plugin_description() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
	return "Spams a Client with Pokes?";
}

/* Set TeamSpeak 3 callback functions */
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
	ts3Functions = funcs;
}

/*
* Custom code called right after loading the plugin. Returns 0 on success, 1 on failure.
* If the function returns 1 on failure, the plugin will be unloaded again.
*/
int ts3plugin_init() {
	char appPath[PATH_BUFSIZE];
	char resourcesPath[PATH_BUFSIZE];
	char configPath[PATH_BUFSIZE];
	char pluginPath[PATH_BUFSIZE];

	/* Your plugin init code here */
	printf("PLUGIN: init\n");

	/* Example on how to query application, resources and configuration paths from client */
	/* Note: Console client returns empty string for app and resources path */
	ts3Functions.getAppPath(appPath, PATH_BUFSIZE);
	ts3Functions.getResourcesPath(resourcesPath, PATH_BUFSIZE);
	ts3Functions.getConfigPath(configPath, PATH_BUFSIZE);
	ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE, pluginID);

	printf("PLUGIN: App path: %s\nResources path: %s\nConfig path: %s\nPlugin path: %s\n", appPath, resourcesPath, configPath, pluginPath);

	return 0;  /* 0 = success, 1 = failure, -2 = failure but client will not show a "failed to load" warning */
			   /* -2 is a very special case and should only be used if a plugin displays a dialog (e.g. overlay) asking the user to disable
			   * the plugin again, avoiding the show another dialog by the client telling the user the plugin failed to load.
			   * For normal case, if a plugin really failed to load because of an error, the correct return value is 1. */
}

/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown() {
	/* Your plugin cleanup code here */
	printf("Spam-Poke: shutdown\n");

	/*
	* Note:
	* If your plugin implements a settings dialog, it must be closed and deleted here, else the
	* TeamSpeak client will most likely crash (DLL removed but dialog from DLL code still open).
	*/

	/* Free pluginID if we registered it */
	if (pluginID) {
		free(pluginID);
		pluginID = NULL;
	}
}

/****************************** Optional functions ********************************/
/*
* Following functions are optional, if not needed you don't need to implement them.
*/

/* Tell client if plugin offers a configuration window. If this function is not implemented, it's an assumed "does not offer" (PLUGIN_OFFERS_NO_CONFIGURE). */
int ts3plugin_offersConfigure() {
	printf("PLUGIN: offersConfigure\n");
	/*
	* Return values:
	* PLUGIN_OFFERS_NO_CONFIGURE         - Plugin does not implement ts3plugin_configure
	* PLUGIN_OFFERS_CONFIGURE_NEW_THREAD - Plugin does implement ts3plugin_configure and requests to run this function in an own thread
	* PLUGIN_OFFERS_CONFIGURE_QT_THREAD  - Plugin does implement ts3plugin_configure and requests to run this function in the Qt GUI thread
	*/
	return PLUGIN_OFFERS_NO_CONFIGURE;  /* In this case ts3plugin_configure does not need to be implemented */
}

/* Plugin might offer a configuration window. If ts3plugin_offersConfigure returns 0, this function does not need to be implemented. */
void ts3plugin_configure(void* handle, void* qParentWidget) {
	printf("PLUGIN: configure\n");

}

/*
* If the plugin wants to use error return codes, plugin commands, hotkeys or menu items, it needs to register a command ID. This function will be
* automatically called after the plugin was initialized. This function is optional. If you don't use these features, this function can be omitted.
* Note the passed pluginID parameter is no longer valid after calling this function, so you must copy it and store it in the plugin.
*/
void ts3plugin_registerPluginID(const char* id) {
	const size_t sz = strlen(id) + 1;
	pluginID = (char*)malloc(sz * sizeof(char));
	_strcpy(pluginID, sz, id);  /* The id buffer will invalidate after exiting this function */
	printf("PLUGIN: registerPluginID: %s\n", pluginID);
}

/*
* Implement the following three functions when the plugin should display a line in the server/channel/client info.
* If any of ts3plugin_infoTitle, ts3plugin_infoData or ts3plugin_freeMemory is missing, the info text will not be displayed.
*/

/*
* Dynamic content shown in the right column in the info frame. Memory for the data string needs to be allocated in this
* function. The client will call ts3plugin_freeMemory once done with the string to release the allocated memory again.
* Check the parameter "type" if you want to implement this feature only for specific item types. Set the parameter
* "data" to NULL to have the client ignore the info data.
*/

/* Required to release the memory for parameter "data" allocated in ts3plugin_infoData and ts3plugin_initMenus */
void ts3plugin_freeMemory(void* data) {
	free(data);
}

/*
* Plugin requests to be always automatically loaded by the TeamSpeak 3 client unless
* the user manually disabled it in the plugin dialog.
* This function is optional. If missing, no autoload is assumed.
*/
int ts3plugin_requestAutoload() {
	return 0;  /* 1 = request autoloaded, 0 = do not request autoload */
}

/* Helper function to create a menu item */
static struct PluginMenuItem* createMenuItem(enum PluginMenuType type, int id, const char* text, const char* icon) {
	struct PluginMenuItem* menuItem = (struct PluginMenuItem*)malloc(sizeof(struct PluginMenuItem));
	menuItem->type = type;
	menuItem->id = id;
	_strcpy(menuItem->text, PLUGIN_MENU_BUFSZ, text);
	_strcpy(menuItem->icon, PLUGIN_MENU_BUFSZ, icon);
	return menuItem;
}

/* Some makros to make the code to create menu items a bit more readable */
#define BEGIN_CREATE_MENUS(x) const size_t sz = x + 1; size_t n = 0; *menuItems = (struct PluginMenuItem**)malloc(sizeof(struct PluginMenuItem*) * sz);
#define CREATE_MENU_ITEM(a, b, c, d) (*menuItems)[n++] = createMenuItem(a, b, c, d);
#define END_CREATE_MENUS (*menuItems)[n++] = NULL; assert(n == sz);

/*
* Menu IDs for this plugin. Pass these IDs when creating a menuitem to the TS3 client. When the menu item is triggered,
* ts3plugin_onMenuItemEvent will be called passing the menu ID of the triggered menu item.
* These IDs are freely choosable by the plugin author. It's not really needed to use an enum, it just looks prettier.
*/
enum {
	fastpoke_x,
	slowpoke_x,
	set_10,
	set_100,
	set_500,
	set_1000,
};

/*
* Initialize plugin menus.
* This function is called after ts3plugin_init and ts3plugin_registerPluginID. A pluginID is required for plugin menus to work.
* Both ts3plugin_registerPluginID and ts3plugin_freeMemory must be implemented to use menus.
* If plugin menus are not used by a plugin, do not implement this function or return NULL.
*/
void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon) {
	/*
	* Create the menus
	* There are three types of menu items:
	* - PLUGIN_MENU_TYPE_CLIENT:  Client context menu
	* - PLUGIN_MENU_TYPE_CHANNEL: Channel context menu
	* - PLUGIN_MENU_TYPE_GLOBAL:  "Plugins" menu in menu bar of main window
	*
	* Menu IDs are used to identify the menu item when ts3plugin_onMenuItemEvent is called
	*
	* The menu text is required, max length is 128 characters
	*
	* The icon is optional, max length is 128 characters. When not using icons, just pass an empty string.
	* Icons are loaded from a subdirectory in the TeamSpeak client plugins folder. The subdirectory must be named like the
	* plugin filename, without dll/so/dylib suffix
	* e.g. for "test_plugin.dll", icon "1.png" is loaded from <TeamSpeak 3 Client install dir>\plugins\test_plugin\1.png
	*/

	BEGIN_CREATE_MENUS(6);  /* IMPORTANT: Number of menu items must be correct! */
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, fastpoke_x, "Poke Client (Fast)", ""); //1
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, slowpoke_x, "Poke Client (Slow)", ""); //2
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, set_10, "Poke Amount: 10x", ""); //3
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, set_100, "Poke Amount: 100x", ""); //4
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, set_500, "Poke Amount: 500x", ""); //5
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, set_1000, "Poke Amount: 1000x", ""); //6

	END_CREATE_MENUS;  /* Includes an assert checking if the number of menu items matched */

	ts3Functions.setPluginMenuEnabled(pluginID, set_10, 0);
	ts3Functions.setPluginMenuEnabled(pluginID, set_100, 1);
	ts3Functions.setPluginMenuEnabled(pluginID, set_500, 1);
	ts3Functions.setPluginMenuEnabled(pluginID, set_1000, 1);

	*menuIcon = (char*)malloc(PLUGIN_MENU_BUFSZ * sizeof(char));
	_strcpy(*menuIcon, PLUGIN_MENU_BUFSZ, "t.png");

	/*
	* Menus can be enabled or disabled with: ts3Functions.setPluginMenuEnabled(pluginID, menuID, 0|1);
	* Test it with plugin command: /test enablemenu <menuID> <0|1>
	* Menus are enabled by default. Please note that shown menus will not automatically enable or disable when calling this function to
	* ensure Qt menus are not modified by any thread other the UI thread. The enabled or disable state will change the next time a
	* menu is displayed.
	*/
	/* For example, this would disable MENU_ID_GLOBAL_2: */
	/* ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_GLOBAL_2, 0); */

	/* All memory allocated in this function will be automatically released by the TeamSpeak client later by calling ts3plugin_freeMemory */
}


/* Client UI callbacks */

/*
* Called from client when an avatar image has been downloaded to or deleted from cache.
* This callback can be called spontaneously or in response to ts3Functions.getAvatar()
*/

/*
* Called when a plugin menu item (see ts3plugin_initMenus) is triggered. Optional function, when not using plugin menus, do not implement this.
*
* Parameters:
* - serverConnectionHandlerID: ID of the current server tab
* - type: Type of the menu (PLUGIN_MENU_TYPE_CHANNEL, PLUGIN_MENU_TYPE_CLIENT or PLUGIN_MENU_TYPE_GLOBAL)
* - menuItemID: Id used when creating the menu item
* - selectedItemID: Channel or Client ID in the case of PLUGIN_MENU_TYPE_CHANNEL and PLUGIN_MENU_TYPE_CLIENT. 0 for PLUGIN_MENU_TYPE_GLOBAL.
*/
void ts3plugin_onMenuItemEvent(uint64 serverConnectionHandlerID, enum PluginMenuType type, int menuItemID, uint64 selectedItemID) {
	printf("PLUGIN: onMenuItemEvent: serverConnectionHandlerID=%llu, type=%d, menuItemID=%d, selectedItemID=%llu\n", (long long unsigned int)serverConnectionHandlerID, type, menuItemID, (long long unsigned int)selectedItemID);

	switch (menuItemID) {
	case slowpoke_x:
		for (int i = 0; i <= amount; i++)
		{
			ts3Functions.requestClientPoke(serverConnectionHandlerID, selectedItemID, "Poke!", 0);
			system("ping 127.0.0.1 -n 6 > nul");
			//Sleep(5000);
		}
		break;
	case fastpoke_x:
		for (int i = 0; i <= amount; i++)
		{
			ts3Functions.requestClientPoke(serverConnectionHandlerID, selectedItemID, "Poke!", 0);
		}
		break;
	case set_10:
		amount = 10;

		ts3Functions.setPluginMenuEnabled(pluginID, set_10, 0);
		ts3Functions.setPluginMenuEnabled(pluginID, set_100, 1);
		ts3Functions.setPluginMenuEnabled(pluginID, set_500, 1);
		ts3Functions.setPluginMenuEnabled(pluginID, set_1000, 1);
		break;
	case set_100:
		amount = 100;

		ts3Functions.setPluginMenuEnabled(pluginID, set_10, 1);
		ts3Functions.setPluginMenuEnabled(pluginID, set_100, 0);
		ts3Functions.setPluginMenuEnabled(pluginID, set_500, 1);
		ts3Functions.setPluginMenuEnabled(pluginID, set_1000, 1);
		break;
	case set_500:
		amount = 500;

		ts3Functions.setPluginMenuEnabled(pluginID, set_10, 1);
		ts3Functions.setPluginMenuEnabled(pluginID, set_100, 1);
		ts3Functions.setPluginMenuEnabled(pluginID, set_500, 0);
		ts3Functions.setPluginMenuEnabled(pluginID, set_1000, 1);
		break;
	case set_1000:
		amount = 1000;

		ts3Functions.setPluginMenuEnabled(pluginID, set_10, 1);
		ts3Functions.setPluginMenuEnabled(pluginID, set_100, 1);
		ts3Functions.setPluginMenuEnabled(pluginID, set_500, 1);
		ts3Functions.setPluginMenuEnabled(pluginID, set_1000, 0);
		break;
	default:
		break;
	}
}
