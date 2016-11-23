#ifdef _WIN32
#pragma warning (disable : 4100)
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "C:\Users\justa\Desktop\anti-move\include\teamspeak/public_errors.h"
#include "C:\Users\justa\Desktop\anti-move\include\teamspeak/public_errors_rare.h"
#include "C:\Users\justa\Desktop\anti-move\include\teamspeak/public_definitions.h"
#include "C:\Users\justa\Desktop\anti-move\include\teamspeak/public_rare_definitions.h"
#include "C:\Users\justa\Desktop\anti-move\include\teamspeak/clientlib_publicdefinitions.h"
#include "C:\Users\justa\Desktop\anti-move\include/ts3_functions.h"
#include "plugin.h"

int RubberbandEnabled = 0;

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
		const wchar_t* name = L"Anti-Move";
		if(wcharToUtf8(name, &result) == -1) {
			result = "Anti-Move";
		}
	}
	return result;
#else
	return "Anti-Move";
#endif
}

const char* ts3plugin_version() {
    return "1.0";
}

int ts3plugin_apiVersion() {
	return PLUGIN_API_VERSION;
}

const char* ts3plugin_author() {
	
    return "Kleberstoff";
}

const char* ts3plugin_description() {
	
    return "I wanna stay right here.";
}

void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

int ts3plugin_init() {
    char appPath[PATH_BUFSIZE];
    char resourcesPath[PATH_BUFSIZE];
    char configPath[PATH_BUFSIZE];
	char pluginPath[PATH_BUFSIZE];

    printf("Anti-Move: init\n");

    ts3Functions.getAppPath(appPath, PATH_BUFSIZE);
    ts3Functions.getResourcesPath(resourcesPath, PATH_BUFSIZE);
    ts3Functions.getConfigPath(configPath, PATH_BUFSIZE);
	ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE, pluginID);

	printf("PLUGIN: App path: %s\nResources path: %s\nConfig path: %s\nPlugin path: %s\n", appPath, resourcesPath, configPath, pluginPath);

    return 0;
}

void ts3plugin_shutdown() {

    printf("Anti-Move: shutdown\n");
	if(pluginID) {
		free(pluginID);
		pluginID = NULL;
	}
}

const char* ts3plugin_infoTitle() {
	return "Anti-Move Plugin";
}

void ts3plugin_freeMemory(void* data) {
	free(data);
}

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

enum {
	Rubberband_Enabled,
	Rubberband_Disabled
};

#define BEGIN_CREATE_MENUS(x) const size_t sz = x + 1; size_t n = 0; *menuItems = (struct PluginMenuItem**)malloc(sizeof(struct PluginMenuItem*) * sz);
#define CREATE_MENU_ITEM(a, b, c, d) (*menuItems)[n++] = createMenuItem(a, b, c, d);
#define END_CREATE_MENUS (*menuItems)[n++] = NULL; assert(n == sz);

void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon) {

	BEGIN_CREATE_MENUS(1);
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, Rubberband_Enabled,  "Toggle Anti-Move on",  "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, Rubberband_Disabled, "Toggle Anti-Move off", "");
	END_CREATE_MENUS;

	*menuIcon = (char*)malloc(PLUGIN_MENU_BUFSZ * sizeof(char));
	_strcpy(*menuIcon, PLUGIN_MENU_BUFSZ, "t.png");

	ts3Functions.setPluginMenuEnabled(pluginID, Rubberband_Enabled, 1);
	ts3Functions.setPluginMenuEnabled(pluginID, Rubberband_Disabled, 0);
	
}


void ts3plugin_onClientMoveMovedEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID moverID, const char* moverName, const char* moverUniqueIdentifier, const char* moveMessage)
{
	anyID myId;

	if (ts3Functions.getClientID(serverConnectionHandlerID, &myId) != ERROR_ok)
	{
		return;
	}
	
	if (RubberbandEnabled == 1)
	{
		if (clientID == myId)
		{
			ts3Functions.requestClientMove(serverConnectionHandlerID, clientID, oldChannelID, "", NULL);
		}
	}
}

void ts3plugin_onClientKickFromChannelEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, const char* kickMessage)
{
	anyID myId;

	if (Rubberband_Enabled == 1)
	{
		if (ts3Functions.getClientID(serverConnectionHandlerID, &myId) != ERROR_ok)
		{
			return;
		}

		if (clientID == myId)
		{
			ts3Functions.requestClientMove(serverConnectionHandlerID, clientID, oldChannelID, "", NULL);
		}
	}
	else
	{
		//Nothing
	}
}

void ts3plugin_onMenuItemEvent(uint64 serverConnectionHandlerID, enum PluginMenuType type, int menuItemID, uint64 selectedItemID) {
	printf("PLUGIN: onMenuItemEvent: serverConnectionHandlerID=%llu, type=%d, menuItemID=%d, selectedItemID=%llu\n", (long long unsigned int)serverConnectionHandlerID, type, menuItemID, (long long unsigned int)selectedItemID);
	switch (menuItemID)
	{
	case Rubberband_Enabled:
		ts3Functions.setPluginMenuEnabled(pluginID, Rubberband_Enabled, 0);
		ts3Functions.setPluginMenuEnabled(pluginID, Rubberband_Disabled, 1);
		RubberbandEnabled = 1;
		break;
	case Rubberband_Disabled:
		ts3Functions.setPluginMenuEnabled(pluginID, Rubberband_Enabled, 1);
		ts3Functions.setPluginMenuEnabled(pluginID, Rubberband_Disabled, 0);
		RubberbandEnabled = 0;
		break;
	default:
		break;
	}
}
