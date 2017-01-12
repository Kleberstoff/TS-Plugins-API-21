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
#include <assert.h>
#include "teamspeak/public_errors.h"
#include "teamspeak/public_errors_rare.h"
#include "teamspeak/public_definitions.h"
#include "teamspeak/public_rare_definitions.h"
#include "teamspeak/clientlib_publicdefinitions.h"
#include "ts3_functions.h"
#include "plugin.h"

int kickdisabled = 0;
int messageonly = 0;

static struct TS3Functions ts3Functions;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif


void ts3plugin_freeMemory(void* data) {
	free(data);
}

#define PLUGIN_API_VERSION 21

#define PATH_BUFSIZE 512
#define COMMAND_BUFSIZE 128
#define INFODATA_BUFSIZE 128
#define SERVERINFO_BUFSIZE 256
#define CHANNELINFO_BUFSIZE 512
#define RETURNCODE_BUFSIZE 128

static char* pluginID = NULL;

static struct PluginMenuItem* createMenuItem(enum PluginMenuType type, int id, const char* text, const char* icon) {
	struct PluginMenuItem* menuItem = (struct PluginMenuItem*)malloc(sizeof(struct PluginMenuItem));
	menuItem->type = type;
	menuItem->id = id;
	_strcpy(menuItem->text, PLUGIN_MENU_BUFSZ, text);
	_strcpy(menuItem->icon, PLUGIN_MENU_BUFSZ, icon);
	return menuItem;
}

void ts3plugin_registerPluginID(const char* id) {
	const size_t sz = strlen(id) + 1;
	pluginID = (char*)malloc(sz * sizeof(char));
	_strcpy(pluginID, sz, id);
	printf("PLUGIN: registerPluginID: %s\n", pluginID);
}

#ifdef _WIN32

static int wcharToUtf8(const wchar_t* str, char** result) {
	int outlen = WideCharToMultiByte(CP_UTF8, 0, str, -1, 0, 0, 0, 0);
	*result = (char*)malloc(outlen);
	if(WideCharToMultiByte(CP_UTF8, 0, str, -1, *result, outlen, 0, 0) == 0) {
		*result = NULL;
		return -1;
	}
	return 0;
}

#endif

const char* ts3plugin_name() {
#ifdef _WIN32
	
	static char* result = NULL;
	if(!result) {
		const wchar_t* name = L"Anti-Poke";
		if(wcharToUtf8(name, &result) == -1) {
			result = "Anti-Poke"; 
		}
	}
	return result;
#else
	return "Anti-Poke";
#endif
}

const char* ts3plugin_version() {
    return "0.1";
}

int ts3plugin_apiVersion() {
	return PLUGIN_API_VERSION;
}

const char* ts3plugin_author() {
    return "Kleberstoff";
}

const char* ts3plugin_description() {
	return "This plugin fucks around with noobs. Great for trolling and showing off. Coded by Segfault64 and shock.";
}

void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

int ts3plugin_init() {
    char appPath[PATH_BUFSIZE];
    char resourcesPath[PATH_BUFSIZE];
    char configPath[PATH_BUFSIZE];
	char pluginPath[PATH_BUFSIZE];

    printf("PLUGIN: init\n");

    ts3Functions.getAppPath(appPath, PATH_BUFSIZE);
    ts3Functions.getResourcesPath(resourcesPath, PATH_BUFSIZE);
    ts3Functions.getConfigPath(configPath, PATH_BUFSIZE);
	ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE, pluginID);

	printf("PLUGIN: App path: %s\nResources path: %s\nConfig path: %s\nPlugin path: %s\n", appPath, resourcesPath, configPath, pluginPath);

    return 0;
}

void ts3plugin_shutdown() {

    printf("Anti-Poke: Shutdown\n");

	if(pluginID) {
		free(pluginID);
		pluginID = NULL;
	}
}

int ts3plugin_onClientPokeEvent(uint64 serverConnectionHandlerID, anyID fromClientID, const char* pokerName, const char* pokerUniqueIdentity, const char* message, int ffIgnored)
{
	anyID myID;

	printf("PLUGIN onClientPokeEvent: %llu %d %s %s %d\n", (long long unsigned int)serverConnectionHandlerID, fromClientID, pokerName, message, ffIgnored);

	if (ffIgnored) {
		return 0;
	}

	if (ts3Functions.getClientID(serverConnectionHandlerID, &myID) != ERROR_ok)
	{
		ts3Functions.logMessage("Error querying own client id", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
		return 0;
	}

	if (fromClientID != myID && (messageonly != 0 || kickdisabled != 1)) {

		if (kickdisabled == 0 || messageonly == 1)
		{
			if (ts3Functions.requestSendPrivateTextMsg(serverConnectionHandlerID, "Poke Ignored! Send it as a PM you lazy bugger! (SK64 F*ck You plugin)", fromClientID, NULL) != ERROR_ok)
			{
				ts3Functions.logMessage("Error pm'ing user who just poked you - do you have enough permissions?", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
			}
		}

		if (kickdisabled == 0 && messageonly != 1)
		{
			if (ts3Functions.requestClientKickFromServer(serverConnectionHandlerID, fromClientID, "Poke Ignored! Send it as a PM you lazy bugger! (SK64 F*ck You plugin)", NULL) != ERROR_ok)
			{
				ts3Functions.logMessage("Error kicking user who just poked you - do you have enough permissions?", LogLevel_ERROR, "Plugin", serverConnectionHandlerID);
			}
		}
		return 1;
	}
	else {
		return 0;
	}

	return 1;
}

enum
{
	DISABLE_KICK,
	ENABLE_KICK,
	MESSAGE_ONLY
};
 
#define BEGIN_CREATE_MENUS(x) const size_t sz = x + 1; size_t n = 0; *menuItems = (struct PluginMenuItem**)malloc(sizeof(struct PluginMenuItem*) * sz);
#define CREATE_MENU_ITEM(a, b, c, d) (*menuItems)[n++] = createMenuItem(a, b, c, d);
#define END_CREATE_MENUS (*menuItems)[n++] = NULL; assert(n == sz);

void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon)
{
	BEGIN_CREATE_MENUS(3);

	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, ENABLE_KICK, "Enable Kick", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MESSAGE_ONLY, "Message Only", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, DISABLE_KICK, "Disabled", "");
	END_CREATE_MENUS;

	*menuIcon = (char*)malloc(PLUGIN_MENU_BUFSZ * sizeof(char));
	_strcpy(*menuIcon, PLUGIN_MENU_BUFSZ, "t.png");

	ts3Functions.setPluginMenuEnabled(pluginID, ENABLE_KICK, 0);
}

void ts3plugin_onMenuItemEvent(uint64 serverConnectionHandlerID, enum PluginMenuType type, int menuItemID, uint64 selectedItemID)
{
	switch (menuItemID)
	{
	case ENABLE_KICK:
		ts3Functions.setPluginMenuEnabled(pluginID, ENABLE_KICK, 0);
		ts3Functions.setPluginMenuEnabled(pluginID, MESSAGE_ONLY, 1);
		ts3Functions.setPluginMenuEnabled(pluginID, DISABLE_KICK, 1);
		kickdisabled = 0;
		messageonly = 0;
		break;
	case DISABLE_KICK:
		ts3Functions.setPluginMenuEnabled(pluginID, ENABLE_KICK, 1);
		ts3Functions.setPluginMenuEnabled(pluginID, DISABLE_KICK, 0);
		ts3Functions.setPluginMenuEnabled(pluginID, MESSAGE_ONLY, 1);
		kickdisabled = 1;
		messageonly = 0;
		break;
	case MESSAGE_ONLY:
		ts3Functions.setPluginMenuEnabled(pluginID, ENABLE_KICK, 1);
		ts3Functions.setPluginMenuEnabled(pluginID, DISABLE_KICK, 1);
		ts3Functions.setPluginMenuEnabled(pluginID, MESSAGE_ONLY, 0);
		kickdisabled = 1;
		messageonly = 1;
		break;
	default:
		break;
	}
}