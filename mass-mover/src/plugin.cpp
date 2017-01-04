/*
* TS3MassMover - Teamspeak 3 Client Plugin
*
* Copyright (c) 2010-2015 Stefan Martens, Mr. S // Updated to APU
*/

#if (defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64)) && !defined(WINDOWS)
#define WINDOWS
#endif

#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG _DEBUG
#endif

#ifdef WINDOWS
#pragma warning (disable : 4100)
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <tchar.h>
#ifndef __WIN32__
#define __WIN32__
#endif
#define BUILDING_DLL
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <map>
#include <fstream>
#include <sstream>
#include "teamspeak/public_errors.h"
#include "teamspeak/public_errors_rare.h"
#include "teamspeak/public_definitions.h"
#include "teamspeak/public_rare_definitions.h"
#include "teamspeak/clientlib_publicdefinitions.h"
#include "ts3_functions.h"
#include "plugin.h"
#include "pluginversion.h"

static struct TS3Functions ts3Functions;

#ifdef WINDOWS
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
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
#define txtreturn(t) 	if(wcBuffer) { \
		free(wcBuffer); \
		wcBuffer = NULL; \
	} \
	if(wcharToUtf8(L ## t, &wcBuffer) == -1) \
		wcBuffer = t; \
	return wcBuffer
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); dest[destSize-1] = '\0'; }
#define txtreturn(t) return t
#endif

#define PATH_BUFSIZE 512
#define COMMAND_BUFSIZE 512
#define INFODATA_BUFSIZE 128
#define SERVERINFO_BUFSIZE 256
#define CHANNELINFO_BUFSIZE 512
#define RETURNCODE_BUFSIZE 128
#define REQUESTCLIENTMOVERETURNCODES_SLOTS 5

static char* pluginID = NULL;
static char* wcBuffer = NULL;
static std::map<std::string, std::string> messages;

/*********************************** Required functions ************************************/
/*
* If any of these required functions is not implemented, TS3 will refuse to load the plugin
*/

/* Unique name identifying this plugin */
const char* ts3plugin_name() {
	txtreturn("TS3MassMover");
}

/* Plugin version */
const char* ts3plugin_version() {
	return PLUGIN_VERSION;
}

/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
int ts3plugin_apiVersion() {
	return PLUGIN_API_VERSION;
}

/* Plugin author */
const char* ts3plugin_author() {
	txtreturn("Stefan1200, Mr. S");
}

/* Plugin description */
const char* ts3plugin_description() {
	txtreturn("This plugin adds short chat commands to move all players from one channel! Visit http://www.stefan1200.de");
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
	wcBuffer = NULL;

#ifdef DEBUG
	char appPath[PATH_BUFSIZE];
	char resourcesPath[PATH_BUFSIZE];
	char configPath[PATH_BUFSIZE];
	char pluginPath[PATH_BUFSIZE];

	/* Your plugin init code here */
	printf("TS3MassMover: init\n");

	/* Example on how to query application, resources and configuration paths from client */
	/* Note: Console client returns empty string for app and resources path */
	ts3Functions.getAppPath(appPath, PATH_BUFSIZE);
	ts3Functions.getResourcesPath(resourcesPath, PATH_BUFSIZE);
	ts3Functions.getConfigPath(configPath, PATH_BUFSIZE);
	ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE, pluginID);

	printf("TS3MassMover: App path: %s\n\tResources path: %s\n\tConfig path: %s\n\tPlugin path: %s\n", appPath, resourcesPath, configPath, pluginPath);
#endif

	// Set default language (EN)
	messages["MSG_MENU_CHANNEL_TO_ME"] = "Move clients to my channel";
	messages["MSG_MENU_CHANNEL_FROM_ME"] = "Move clients here from my channel";
	messages["MSG_MENU_CHANNEL_FROM_SERVER"] = "Move all clients from server to this channel";
	messages["MSG_MENU_GLOBAL_TO_ME"] = "Move all clients from server to my channel";
	messages["MSG_MISSING_CHANNEL_ID"] = "Missing channel ID parameter.";
	messages["MSG_MISSING_SERVERGROUP_ID"] = "Missing servergroup ID parameter.";
	messages["MSG_MISSING_CHANNEL_ID_AND_SERVERGROUP_ID"] = "Missing server group ID and channel ID parameter.";
	messages["MSG_MISSING_2CHANNEL_ID"] = "Missing two channel ID parameters.";
	messages["MSG_MISSING_TARGET_CHANNEL_ID"] = "Missing target channel ID parameter.";
	messages["MSG_ERROR_QUERYING_CLIENT_ID"] = "Error querying client ID";
	messages["MSG_ERROR_QUERYING_CHANNEL_ID"] = "Error querying channel ID";
	messages["MSG_ERROR_QUERYING_CHANNEL_CLIENT_LIST"] = "Error querying channel client list";
	messages["MSG_ERROR_REQUESTING_CLIENT_MOVE"] = "Error requesting client move";
	messages["MSG_ERROR_QUERYING_CLIENT_LIST"] = "Error querying client list";
	// Try to load translation
	char languagePath[PATH_BUFSIZE];
	ts3Functions.getPluginPath(languagePath, PATH_BUFSIZE, pluginID);
	strcat(languagePath, "/TS3MassMover.lang");
	std::ifstream languageInput(languagePath);
	if (languageInput.is_open())
	{
		std::string line;
		while (std::getline(languageInput, line))
		{
			std::stringstream input_stringstream(line);
			std::string index, value, token;
			while (std::getline(input_stringstream, token, '='))
			{
				if (index.size() == 0)
					index = token;
				else
				{
					if (value.size() > 0) value.append("=");
					value.append(token);
				}
			}
			messages[index] = value;
		}
	}
	else
	{
		std::ofstream languageOutput(languagePath);
		if (languageOutput.is_open())
		{
			for (std::map<std::string, std::string>::iterator iter = messages.begin(); iter != messages.end(); iter++)
			{
				languageOutput << iter->first << "=" << iter->second << std::endl;
			}
			languageOutput.close();
		}
	}

	return 0;  /* 0 = success, 1 = failure, -2 = failure but client will not show a "failed to load" warning */
			   /* -2 is a very special case and should only be used if a plugin displays a dialog (e.g. overlay) asking the user to disable
			   * the plugin again, avoiding the show another dialog by the client telling the user the plugin failed to load.
			   * For normal case, if a plugin really failed to load because of an error, the correct return value is 1. */
}

/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown() {
#ifdef DEBUG
	printf("TS3MassMover: shutdown\n");
#endif

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

	/* Free wchar buffer */
	if (wcBuffer) {
		free(wcBuffer);
		wcBuffer = NULL;
	}
}

/****************************** Optional functions ********************************/
/*
* Following functions are optional, if not needed you don't need to implement them.
*/

/* Tell client if plugin offers a configuration window. If this function is not implemented, it's an assumed "does not offer" (PLUGIN_OFFERS_NO_CONFIGURE). */
int ts3plugin_offersConfigure() {
#ifdef DEBUG
	printf("TS3MassMover: offersConfigure\n");
#endif

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
#ifdef DEBUG
	printf("TS3MassMover: configure\n");
#endif
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
#ifdef DEBUG
	printf("TS3MassMover: registerPluginID: %s\n", pluginID);
#endif
}

/* Plugin command keyword. Return NULL or "" if not used. */
const char* ts3plugin_commandKeyword() {
	return "m";
}

bool isServerGroupIDinList(char* list, uint64 id) {
	char* token = strtok(list, ",");
	uint64 tmp = 0;
	while (token != NULL)
	{
		tmp = (uint64)atoi(token);
		if (id == tmp) return true;
		token = strtok(NULL, ",");
	}

	return false;
}

bool isClientInList(anyID* clientList, anyID* clientID) {
	for (int i = 0; clientList[i]; i++)
	{
		if (clientList[i] == *clientID) return true;
	}

	return false;
}

enum { CMD_NONE = 0, CMD_MHELP, CMD_MH, CMD_MAH, CMD_MT, CMD_MAT, CMD_MSGT, CMD_MSGH, CMD_MFCTC, CMD_INFO, CMD_MNSGTC };

void ExecuteCommand(uint64 serverConnectionHandlerID, int cmd, char* p1, char* p2, char* p3) {
	char *password = const_cast<char *>("");
	int i = 0;
	anyID myID;

	switch (cmd)
	{
	case CMD_NONE:
	case CMD_INFO:
	default:
		char buffer[1024];
		sprintf(buffer, "\n[b]TS3MassMover Plugin v%s[/b]\nCopyright (c) 2015 by %s\n\n", ts3plugin_version(), ts3plugin_author());
		ts3Functions.printMessageToCurrentTab(buffer);
		break; // return 1;
	case CMD_MT:
		if (p1)
		{
			uint64 channelID = (uint64)atoi(p1);
			password = const_cast<char *>(p2 ? p2 : "");

			/* Get own clientID */
			if (ts3Functions.getClientID(serverConnectionHandlerID, &myID) != ERROR_ok)
			{
				ts3Functions.logMessage(messages["MSG_ERROR_QUERYING_CLIENT_ID"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
				break;
			}

			uint64 currentChannelID;
			if (ts3Functions.getChannelOfClient(serverConnectionHandlerID, myID, &currentChannelID) != ERROR_ok)
			{
				ts3Functions.logMessage(messages["MSG_ERROR_QUERYING_CHANNEL_ID"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
				break;
			}

			anyID* channelClientList;
			if (ts3Functions.getChannelClientList(serverConnectionHandlerID, currentChannelID, &channelClientList) != ERROR_ok)
			{
				ts3Functions.logMessage(messages["MSG_ERROR_QUERYING_CHANNEL_CLIENT_LIST"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
				break;
			}

			int clientType;
			for (i = 0; channelClientList[i]; i++)
			{
				if (ts3Functions.getClientVariableAsInt(serverConnectionHandlerID, channelClientList[i], CLIENT_TYPE, &clientType) != ERROR_ok)
					continue;

				if (clientType == 1)
					continue;

				/* Move clients to specified channel */
				if (ts3Functions.requestClientMove(serverConnectionHandlerID, channelClientList[i], channelID, password, NULL) != ERROR_ok)
					ts3Functions.logMessage(messages["MSG_ERROR_REQUESTING_CLIENT_MOVE"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
			}

			ts3Functions.freeMemory(channelClientList);
		}
		else
			ts3Functions.printMessageToCurrentTab(messages["MSG_MISSING_CHANNEL_ID"].c_str());
		break;
	case CMD_MAT:
		if (p1)
		{
			uint64 channelID = (uint64)atoi(p1);
			password = const_cast<char *>(p2 ? p2 : "");

			anyID* clientList;
			if (ts3Functions.getClientList(serverConnectionHandlerID, &clientList) != ERROR_ok)
			{
				ts3Functions.logMessage(messages["MSG_ERROR_QUERYING_CLIENT_LIST"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
				break;
			}

			anyID* channelClientList;
			if (ts3Functions.getChannelClientList(serverConnectionHandlerID, channelID, &channelClientList) != ERROR_ok)
			{
				ts3Functions.logMessage(messages["MSG_ERROR_QUERYING_CHANNEL_CLIENT_LIST"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
				ts3Functions.freeMemory(clientList);
				break;
			}

			int clientType;
			for (i = 0; clientList[i]; i++)
			{
				if (ts3Functions.getClientVariableAsInt(serverConnectionHandlerID, clientList[i], CLIENT_TYPE, &clientType) != ERROR_ok)
					continue;

				if (clientType == 1)
					continue;

				if (isClientInList(channelClientList, &clientList[i]))
					continue;

#ifdef DEBUG
				printf("TS3MassMover: Moving client ID %d to channel ID %d", clientList[i], channelID);
#endif
				/* Move clients to specified channel */
				if (ts3Functions.requestClientMove(serverConnectionHandlerID, clientList[i], channelID, password, NULL) != ERROR_ok)
					ts3Functions.logMessage(messages["MSG_ERROR_REQUESTING_CLIENT_MOVE"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
			}

			ts3Functions.freeMemory(clientList);
			ts3Functions.freeMemory(channelClientList);
		}
		else
			ts3Functions.printMessageToCurrentTab(messages["MSG_MISSING_CHANNEL_ID"].c_str());
		break;
	case CMD_MH:
		if (p1)
		{
			uint64 channelID = (uint64)atoi(p1);
			password = const_cast<char *>(p2 ? p2 : "");

			/* Get own clientID */
			if (ts3Functions.getClientID(serverConnectionHandlerID, &myID) != ERROR_ok)
			{
				ts3Functions.logMessage(messages["MSG_ERROR_QUERYING_CLIENT_ID"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
				break;
			}

			uint64 currentChannelID;
			if (ts3Functions.getChannelOfClient(serverConnectionHandlerID, myID, &currentChannelID) != ERROR_ok)
			{
				ts3Functions.logMessage(messages["MSG_ERROR_QUERYING_CHANNEL_ID"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
				break;
			}

			anyID* channelClientList;
			if (ts3Functions.getChannelClientList(serverConnectionHandlerID, channelID, &channelClientList) != ERROR_ok)
			{
				ts3Functions.logMessage(messages["MSG_ERROR_QUERYING_CHANNEL_CLIENT_LIST"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
				break;
			}

			int clientType;
			for (i = 0; channelClientList[i]; i++)
			{
				if (ts3Functions.getClientVariableAsInt(serverConnectionHandlerID, channelClientList[i], CLIENT_TYPE, &clientType) != ERROR_ok)
					continue;

				if (clientType == 1)
					continue;

				/* Move clients to specified channel */
				if (ts3Functions.requestClientMove(serverConnectionHandlerID, channelClientList[i], currentChannelID, password, NULL) != ERROR_ok)
					ts3Functions.logMessage(messages["MSG_ERROR_REQUESTING_CLIENT_MOVE"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
			}

			ts3Functions.freeMemory(channelClientList);
		}
		else
			ts3Functions.printMessageToCurrentTab(messages["MSG_MISSING_CHANNEL_ID"].c_str());
		break;
	case CMD_MAH:
		password = const_cast<char *>(p1 ? p1 : "");

		/* Get own clientID */
		if (ts3Functions.getClientID(serverConnectionHandlerID, &myID) != ERROR_ok)
		{
			ts3Functions.logMessage(messages["MSG_ERROR_QUERYING_CLIENT_ID"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
			break;
		}

		uint64 currentChannelID;
		if (ts3Functions.getChannelOfClient(serverConnectionHandlerID, myID, &currentChannelID) != ERROR_ok)
		{
			ts3Functions.logMessage(messages["MSG_ERROR_QUERYING_CHANNEL_ID"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
			break;
		}

		anyID* clientList;
		if (ts3Functions.getClientList(serverConnectionHandlerID, &clientList) != ERROR_ok)
		{
			ts3Functions.logMessage(messages["MSG_ERROR_QUERYING_CLIENT_LIST"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
			break;
		}

		anyID* channelClientList;
		if (ts3Functions.getChannelClientList(serverConnectionHandlerID, currentChannelID, &channelClientList) != ERROR_ok)
		{
			ts3Functions.logMessage(messages["MSG_ERROR_QUERYING_CHANNEL_CLIENT_LIST"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
			ts3Functions.freeMemory(clientList);
			break;
		}

		int clientType;
		for (i = 0; clientList[i]; i++)
		{
			if (ts3Functions.getClientVariableAsInt(serverConnectionHandlerID, clientList[i], CLIENT_TYPE, &clientType) != ERROR_ok)
				continue;

			if (clientType == 1)
				continue;

			if (isClientInList(channelClientList, &clientList[i]))
				continue;

#ifdef DEBUG
			printf("TS3MassMover: Moving client ID %d to channel ID %d", clientList[i], currentChannelID);
#endif
			/* Move clients to specified channel */
			if (ts3Functions.requestClientMove(serverConnectionHandlerID, clientList[i], currentChannelID, password, NULL) != ERROR_ok)
				ts3Functions.logMessage(messages["MSG_ERROR_REQUESTING_CLIENT_MOVE"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
		}

		ts3Functions.freeMemory(clientList);
		ts3Functions.freeMemory(channelClientList);
		break;
	case CMD_MSGH:
		if (p1)
		{
			uint64 serverGroupID = (uint64)atoi(p1);
			password = const_cast<char *>(p2 ? p2 : "");

			/* Get own clientID */
			if (ts3Functions.getClientID(serverConnectionHandlerID, &myID) != ERROR_ok)
			{
				ts3Functions.logMessage(messages["MSG_ERROR_QUERYING_CLIENT_ID"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
				break;
			}

			uint64 currentChannelID;
			if (ts3Functions.getChannelOfClient(serverConnectionHandlerID, myID, &currentChannelID) != ERROR_ok)
			{
				ts3Functions.logMessage(messages["MSG_ERROR_QUERYING_CHANNEL_ID"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
				break;
			}

			anyID* clientList;
			if (ts3Functions.getClientList(serverConnectionHandlerID, &clientList) != ERROR_ok)
			{
				ts3Functions.logMessage(messages["MSG_ERROR_QUERYING_CLIENT_LIST"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
				break;
			}

			int clientType;
			char* clientServerGroups;
			for (i = 0; clientList[i]; i++)
			{
				if (ts3Functions.getClientVariableAsInt(serverConnectionHandlerID, clientList[i], CLIENT_TYPE, &clientType) != ERROR_ok)
					continue;

				if (clientType == 1)
					continue;

				if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, clientList[i], CLIENT_SERVERGROUPS, &clientServerGroups) != ERROR_ok)
					continue;

				if (isServerGroupIDinList(clientServerGroups, serverGroupID))
				{
					/* Move clients to specified channel */
					if (ts3Functions.requestClientMove(serverConnectionHandlerID, clientList[i], currentChannelID, password, NULL) != ERROR_ok)
						ts3Functions.logMessage(messages["MSG_ERROR_REQUESTING_CLIENT_MOVE"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
				}
			}

			ts3Functions.freeMemory(clientList);
		}
		else
			ts3Functions.printMessageToCurrentTab(messages["MSG_MISSING_SERVERGROUP_ID"].c_str());
		break;
	case CMD_MSGT:
		if (p1 && p2)
		{
			uint64 serverGroupID = (uint64)atoi(p1);
			uint64 channelID = (uint64)atoi(p2);
			password = const_cast<char *>(p3 ? p3 : "");

			/* Get own clientID */
			if (ts3Functions.getClientID(serverConnectionHandlerID, &myID) != ERROR_ok)
			{
				ts3Functions.logMessage(messages["MSG_ERROR_QUERYING_CLIENT_ID"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
				break;
			}

			uint64 currentChannelID;
			if (ts3Functions.getChannelOfClient(serverConnectionHandlerID, myID, &currentChannelID) != ERROR_ok)
			{
				ts3Functions.logMessage(messages["MSG_ERROR_QUERYING_CHANNEL_ID"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
				break;
			}

			anyID* clientList;
			if (ts3Functions.getClientList(serverConnectionHandlerID, &clientList) != ERROR_ok)
			{
				ts3Functions.logMessage(messages["MSG_ERROR_QUERYING_CLIENT_LIST"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
				break;
			}

			int clientType;
			char* clientServerGroups;
			for (i = 0; clientList[i]; i++)
			{
				if (ts3Functions.getClientVariableAsInt(serverConnectionHandlerID, clientList[i], CLIENT_TYPE, &clientType) != ERROR_ok)
					continue;

				if (clientType == 1)
					continue;

				if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, clientList[i], CLIENT_SERVERGROUPS, &clientServerGroups) != ERROR_ok)
					continue;

				if (isServerGroupIDinList(clientServerGroups, serverGroupID))
				{
					/* Move clients to specified channel */
					if (ts3Functions.requestClientMove(serverConnectionHandlerID, clientList[i], channelID, password, NULL) != ERROR_ok)
						ts3Functions.logMessage(messages["MSG_ERROR_REQUESTING_CLIENT_MOVE"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
				}
			}

			ts3Functions.freeMemory(clientList);
		}
		else
		{
			if (p1) ts3Functions.printMessageToCurrentTab(messages["MSG_MISSING_CHANNEL_ID"].c_str());
			else ts3Functions.printMessageToCurrentTab(messages["MSG_MISSING_CHANNEL_ID_AND_SERVERGROUP_ID"].c_str());
		}
		break;
	case CMD_MFCTC:
		if (p1 && p2)
		{
			uint64 channelID = (uint64)atoi(p1);
			uint64 channelID2 = (uint64)atoi(p2);
			password = const_cast<char *>(p3 ? p3 : "");

			anyID* channelClientList;
			if (ts3Functions.getChannelClientList(serverConnectionHandlerID, channelID, &channelClientList) != ERROR_ok)
			{
				ts3Functions.logMessage(messages["MSG_ERROR_QUERYING_CHANNEL_CLIENT_LIST"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
				break;
			}

			int clientType;
			for (i = 0; channelClientList[i]; i++)
			{
				if (ts3Functions.getClientVariableAsInt(serverConnectionHandlerID, channelClientList[i], CLIENT_TYPE, &clientType) != ERROR_ok)
					continue;

				if (clientType == 1)
					continue;

				/* Move clients to specified channel */
				if (ts3Functions.requestClientMove(serverConnectionHandlerID, channelClientList[i], channelID2, password, NULL) != ERROR_ok)
					ts3Functions.logMessage(messages["MSG_ERROR_REQUESTING_CLIENT_MOVE"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
			}

			ts3Functions.freeMemory(channelClientList);
		}
		else
		{
			if (p1) ts3Functions.printMessageToCurrentTab(messages["MSG_MISSING_2CHANNEL_ID"].c_str());
			else ts3Functions.printMessageToCurrentTab(messages["MSG_MISSING_TARGET_CHANNEL_ID"].c_str());
		}
		break;
	case CMD_MHELP:
		ts3Functions.printMessageToCurrentTab("TS3MassMover Plugin\n[b]/m h <channel id> [password of current channel][/b]\nMoves all players from given channel to your current channel!\n[b]/m ah [password of current channel][/b]\nMoves all players from server to your current channel!\n[b]/m t <channel id> [password of target channel][/b]\nMoves all players from your current channel to given channel!\n[b]/m at <channel id> [password of target channel][/b]\nMoves all players from server to given channel!\n[b]/m sgh <server group id> [password of current channel][/b]\nMoves all members of the given server group to your current channel!\n[b]/m sgt <server group id> <channel id> [password of current channel][/b]\nMoves all members of the given server group to the given channel!\n[b]/m fctc <from channel id> <target channel id> [password of target channel][/b]\nMoves all clients of one channel to another channel![b]\n/m nsgt <server group ID> <target channel id> [password of target channel][/b]\nMoves all clients which are not member of one server group to another channel!");
		break;
	case CMD_MNSGTC:
		if (p1 && p2)
		{
			uint64 serverGroupID = (uint64)atoi(p1);
			uint64 channelID = (uint64)atoi(p2);
			password = const_cast<char *>(p3 ? p3 : "");

			/* Get own clientID */
			if (ts3Functions.getClientID(serverConnectionHandlerID, &myID) != ERROR_ok)
			{
				ts3Functions.logMessage(messages["MSG_ERROR_QUERYING_CLIENT_ID"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
				break;
			}

			uint64 currentChannelID;
			if (ts3Functions.getChannelOfClient(serverConnectionHandlerID, myID, &currentChannelID) != ERROR_ok)
			{
				ts3Functions.logMessage(messages["MSG_ERROR_QUERYING_CHANNEL_ID"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
				break;
			}

			anyID* clientList;
			if (ts3Functions.getClientList(serverConnectionHandlerID, &clientList) != ERROR_ok)
			{
				ts3Functions.logMessage(messages["MSG_ERROR_QUERYING_CLIENT_LIST"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
				break;
			}

			int clientType;
			char* clientServerGroups;
			for (i = 0; clientList[i]; i++)
			{
				if (ts3Functions.getClientVariableAsInt(serverConnectionHandlerID, clientList[i], CLIENT_TYPE, &clientType) != ERROR_ok)
					continue;

				if (clientType == 1)
					continue;

				if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, clientList[i], CLIENT_SERVERGROUPS, &clientServerGroups) != ERROR_ok)
					continue;

				if (!isServerGroupIDinList(clientServerGroups, serverGroupID))
				{
					/* Move clients to specified channel */
					if (ts3Functions.requestClientMove(serverConnectionHandlerID, clientList[i], channelID, password, NULL) != ERROR_ok)
						ts3Functions.logMessage(messages["MSG_ERROR_REQUESTING_CLIENT_MOVE"].c_str(), LogLevel_ERROR, "TS3MassMover Plugin", serverConnectionHandlerID);
				}
			}

			ts3Functions.freeMemory(clientList);
		}
		else
		{
			if (p1) ts3Functions.printMessageToCurrentTab(messages["MSG_MISSING_CHANNEL_ID"].c_str());
			else ts3Functions.printMessageToCurrentTab(messages["MSG_MISSING_CHANNEL_ID_AND_SERVERGROUP_ID"].c_str());
		}
		break;
	}
}

/* Plugin processes console command. Return 0 if plugin handled the command, 1 if not handled. */
int ts3plugin_processCommand(uint64 serverConnectionHandlerID, const char* command) {
	char buf[COMMAND_BUFSIZE];
	char *s, *param1 = NULL, *param2 = NULL, *param3 = NULL;
	int i = 0;
	int cmd = CMD_NONE;
#ifdef WINDOWS
	char* context = NULL;
#endif

#ifdef DEBUG
	printf("TS3MassMover: process command: '%s'\n", command);
#endif

	_strcpy(buf, COMMAND_BUFSIZE, command);
#ifdef WINDOWS
	s = strtok_s(buf, " ", &context);
#else
	s = strtok(buf, " ");
#endif
	while (s != NULL)
	{
		if (i == 0)
		{
			if (!strcmp(s, "help"))
				cmd = CMD_MHELP;
			else if (!strcmp(s, "h"))
				cmd = CMD_MH;
			else if (!strcmp(s, "ah"))
				cmd = CMD_MAH;
			else if (!strcmp(s, "t"))
				cmd = CMD_MT;
			else if (!strcmp(s, "at"))
				cmd = CMD_MAT;
			else if (!strcmp(s, "sgt"))
				cmd = CMD_MSGT;
			else if (!strcmp(s, "sgh"))
				cmd = CMD_MSGH;
			else if (!strcmp(s, "fctc"))
				cmd = CMD_MFCTC;
			else if (!strcmp(s, "?"))
				cmd = CMD_MHELP;
			else if (!strcmp(s, "info"))
				cmd = CMD_INFO;
			else if (!strcmp(s, "about"))
				cmd = CMD_INFO;
			else if (!strcmp(s, "nsgt"))
				cmd = CMD_MNSGTC;
		}
		else if (i == 1)
			param1 = s;
		else if (i == 2)
			param2 = s;
		else
			param3 = s;
#ifdef WINDOWS
		s = strtok_s(NULL, " ", &context);
#else
		s = strtok(NULL, " ");
#endif
		i++;
	}

	ExecuteCommand(serverConnectionHandlerID, cmd, param1, param2, param3);

	return 0;  /* Plugin handled command */
}

/* Client changed current server connection handler */
void ts3plugin_currentServerConnectionChanged(uint64 serverConnectionHandlerID) {
#ifdef DEBUG
	printf("TS3MassMover: currentServerConnectionChanged %llu (%llu)\n", (long long unsigned int)serverConnectionHandlerID, (long long unsigned int)ts3Functions.getCurrentServerConnectionHandlerID());
#endif
}

/*
* Implement the following three functions when the plugin should display a line in the server/channel/client info.
* If any of ts3plugin_infoTitle, ts3plugin_infoData or ts3plugin_freeMemory is missing, the info text will not be displayed.
*/

/* Static title shown in the left column in the info frame */
const char* ts3plugin_infoTitle() {
	return "TS3MassMover info";
}

/*
* Dynamic content shown in the right column in the info frame. Memory for the data string needs to be allocated in this
* function. The client will call ts3plugin_freeMemory once done with the string to release the allocated memory again.
* Check the parameter "type" if you want to implement this feature only for specific item types. Set the parameter
* "data" to NULL to have the client ignore the info data.
*/
void ts3plugin_infoData(uint64 serverConnectionHandlerID, uint64 id, enum PluginItemType type, char** data) {
	data = NULL;
}

/* Required to release the memory for parameter "data" allocated in ts3plugin_infoData and ts3plugin_initMenus */
void ts3plugin_freeMemory(void* data) {
	if (data != NULL) free(data);
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
	MENU_ID_CHANNEL_TO_ME = 1,
	MENU_ID_CHANNEL_FROM_ME,
	MENU_ID_CHANNEL_FROM_SERVER,
	MENU_ID_GLOBAL_TO_ME,
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

	BEGIN_CREATE_MENUS(4);  /* IMPORTANT: Number of menu items must be correct! */
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_TO_ME, messages["MSG_MENU_CHANNEL_TO_ME"].c_str(), "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_FROM_ME, messages["MSG_MENU_CHANNEL_FROM_ME"].c_str(), "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CHANNEL, MENU_ID_CHANNEL_FROM_SERVER, messages["MSG_MENU_CHANNEL_FROM_SERVER"].c_str(), "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_TO_ME, messages["MSG_MENU_GLOBAL_TO_ME"].c_str(), "");
	END_CREATE_MENUS;  /* Includes an assert checking if the number of menu items matched */

					   /*
					   * Specify an optional icon for the plugin. This icon is used for the plugins submenu within context and main menus
					   * If unused, set menuIcon to NULL
					   */
					   //*menuIcon = (char*)malloc(PLUGIN_MENU_BUFSZ * sizeof(char));
					   //_strcpy(*menuIcon, PLUGIN_MENU_BUFSZ, "t.png");

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

/* Helper function to create a hotkey */
static struct PluginHotkey* createHotkey(const char* keyword, const char* description) {
	struct PluginHotkey* hotkey = (struct PluginHotkey*)malloc(sizeof(struct PluginHotkey));
	_strcpy(hotkey->keyword, PLUGIN_HOTKEY_BUFSZ, keyword);
	_strcpy(hotkey->description, PLUGIN_HOTKEY_BUFSZ, description);
	return hotkey;
}

/* Some makros to make the code to create hotkeys a bit more readable */
#define BEGIN_CREATE_HOTKEYS(x) const size_t sz = x + 1; size_t n = 0; *hotkeys = (struct PluginHotkey**)malloc(sizeof(struct PluginHotkey*) * sz);
#define CREATE_HOTKEY(a, b) (*hotkeys)[n++] = createHotkey(a, b);
#define END_CREATE_HOTKEYS (*hotkeys)[n++] = NULL; assert(n == sz);

/*
* Initialize plugin hotkeys. If your plugin does not use this feature, this function can be omitted.
* Hotkeys require ts3plugin_registerPluginID and ts3plugin_freeMemory to be implemented.
* This function is automatically called by the client after ts3plugin_init.
*/
void ts3plugin_initHotkeys(struct PluginHotkey*** hotkeys) {
	/* Register hotkeys giving a keyword and a description.
	* The keyword will be later passed to ts3plugin_onHotkeyEvent to identify which hotkey was triggered.
	* The description is shown in the clients hotkey dialog. */
	BEGIN_CREATE_HOTKEYS(0);  /* Create 3 hotkeys. Size must be correct for allocating memory. */
							  //CREATE_HOTKEY("keyword", "Test hotkey");
	END_CREATE_HOTKEYS;

	/* The client will call ts3plugin_freeMemory to release all allocated memory */
}

/************************** TeamSpeak callbacks ***************************/
/*
* Following functions are optional, feel free to remove unused callbacks.
* See the clientlib documentation for details on each function.
*/

/* Clientlib */

void ts3plugin_onConnectStatusChangeEvent(uint64 serverConnectionHandlerID, int newStatus, unsigned int errorNumber) {
}

void ts3plugin_onNewChannelEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 channelParentID) {
}

void ts3plugin_onNewChannelCreatedEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 channelParentID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {
}

void ts3plugin_onDelChannelEvent(uint64 serverConnectionHandlerID, uint64 channelID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {
}

void ts3plugin_onChannelMoveEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 newChannelParentID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {
}

void ts3plugin_onUpdateChannelEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onUpdateChannelEditedEvent(uint64 serverConnectionHandlerID, uint64 channelID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {
}

void ts3plugin_onUpdateClientEvent(uint64 serverConnectionHandlerID, anyID clientID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier) {
}

void ts3plugin_onClientMoveEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* moveMessage) {
}

void ts3plugin_onClientMoveSubscriptionEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility) {
}

void ts3plugin_onClientMoveTimeoutEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* timeoutMessage) {
}

void ts3plugin_onClientMoveMovedEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID moverID, const char* moverName, const char* moverUniqueIdentifier, const char* moveMessage) {
}

void ts3plugin_onClientKickFromChannelEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, const char* kickMessage) {
}

void ts3plugin_onClientKickFromServerEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, const char* kickMessage) {
}

void ts3plugin_onClientIDsEvent(uint64 serverConnectionHandlerID, const char* uniqueClientIdentifier, anyID clientID, const char* clientName) {
}

void ts3plugin_onClientIDsFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onServerEditedEvent(uint64 serverConnectionHandlerID, anyID editerID, const char* editerName, const char* editerUniqueIdentifier) {
}

void ts3plugin_onServerUpdatedEvent(uint64 serverConnectionHandlerID) {
}

int ts3plugin_onServerErrorEvent(uint64 serverConnectionHandlerID, const char* errorMessage, unsigned int error, const char* returnCode, const char* extraMessage) {
	return 0;  /* If no plugin return code was used, the return value of this function is ignored */
}

void ts3plugin_onServerStopEvent(uint64 serverConnectionHandlerID, const char* shutdownMessage) {
}

int ts3plugin_onTextMessageEvent(uint64 serverConnectionHandlerID, anyID targetMode, anyID toID, anyID fromID, const char* fromName, const char* fromUniqueIdentifier, const char* message, int ffIgnored) {
	return 0;  /* 0 = handle normally, 1 = client will ignore the text message */
}

void ts3plugin_onTalkStatusChangeEvent(uint64 serverConnectionHandlerID, int status, int isReceivedWhisper, anyID clientID) {
}

void ts3plugin_onConnectionInfoEvent(uint64 serverConnectionHandlerID, anyID clientID) {
}

void ts3plugin_onServerConnectionInfoEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onChannelSubscribeEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onChannelSubscribeFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onChannelUnsubscribeEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onChannelUnsubscribeFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onChannelDescriptionUpdateEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onChannelPasswordChangedEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onPlaybackShutdownCompleteEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onSoundDeviceListChangedEvent(const char* modeID, int playOrCap) {
}

void ts3plugin_onEditPlaybackVoiceDataEvent(uint64 serverConnectionHandlerID, anyID clientID, short* samples, int sampleCount, int channels) {
}

void ts3plugin_onEditPostProcessVoiceDataEvent(uint64 serverConnectionHandlerID, anyID clientID, short* samples, int sampleCount, int channels, const unsigned int* channelSpeakerArray, unsigned int* channelFillMask) {
}

void ts3plugin_onEditMixedPlaybackVoiceDataEvent(uint64 serverConnectionHandlerID, short* samples, int sampleCount, int channels, const unsigned int* channelSpeakerArray, unsigned int* channelFillMask) {
}

void ts3plugin_onEditCapturedVoiceDataEvent(uint64 serverConnectionHandlerID, short* samples, int sampleCount, int channels, int* edited) {
}

void ts3plugin_onCustom3dRolloffCalculationClientEvent(uint64 serverConnectionHandlerID, anyID clientID, float distance, float* volume) {
}

void ts3plugin_onCustom3dRolloffCalculationWaveEvent(uint64 serverConnectionHandlerID, uint64 waveHandle, float distance, float* volume) {
}

void ts3plugin_onUserLoggingMessageEvent(const char* logMessage, int logLevel, const char* logChannel, uint64 logID, const char* logTime, const char* completeLogString) {
}

/* Clientlib rare */

void ts3plugin_onClientBanFromServerEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, anyID kickerID, const char* kickerName, const char* kickerUniqueIdentifier, uint64 time, const char* kickMessage) {
}

int ts3plugin_onClientPokeEvent(uint64 serverConnectionHandlerID, anyID fromClientID, const char* pokerName, const char* pokerUniqueIdentity, const char* message, int ffIgnored) {
	return 0;  /* 0 = handle normally, 1 = client will ignore the poke */
}

void ts3plugin_onClientSelfVariableUpdateEvent(uint64 serverConnectionHandlerID, int flag, const char* oldValue, const char* newValue) {
}

void ts3plugin_onFileListEvent(uint64 serverConnectionHandlerID, uint64 channelID, const char* path, const char* name, uint64 size, uint64 datetime, int type, uint64 incompletesize, const char* returnCode) {
}

void ts3plugin_onFileListFinishedEvent(uint64 serverConnectionHandlerID, uint64 channelID, const char* path) {
}

void ts3plugin_onFileInfoEvent(uint64 serverConnectionHandlerID, uint64 channelID, const char* name, uint64 size, uint64 datetime) {
}

void ts3plugin_onServerGroupListEvent(uint64 serverConnectionHandlerID, uint64 serverGroupID, const char* name, int type, int iconID, int saveDB) {
}

void ts3plugin_onServerGroupListFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onServerGroupByClientIDEvent(uint64 serverConnectionHandlerID, const char* name, uint64 serverGroupList, uint64 clientDatabaseID) {
}

void ts3plugin_onServerGroupPermListEvent(uint64 serverConnectionHandlerID, uint64 serverGroupID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onServerGroupPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 serverGroupID) {
}

void ts3plugin_onServerGroupClientListEvent(uint64 serverConnectionHandlerID, uint64 serverGroupID, uint64 clientDatabaseID, const char* clientNameIdentifier, const char* clientUniqueID) {
}

void ts3plugin_onChannelGroupListEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID, const char* name, int type, int iconID, int saveDB) {
}

void ts3plugin_onChannelGroupListFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onChannelGroupPermListEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onChannelGroupPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID) {
}

void ts3plugin_onChannelPermListEvent(uint64 serverConnectionHandlerID, uint64 channelID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onChannelPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 channelID) {
}

void ts3plugin_onClientPermListEvent(uint64 serverConnectionHandlerID, uint64 clientDatabaseID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onClientPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 clientDatabaseID) {
}

void ts3plugin_onChannelClientPermListEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 clientDatabaseID, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onChannelClientPermListFinishedEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 clientDatabaseID) {
}

void ts3plugin_onClientChannelGroupChangedEvent(uint64 serverConnectionHandlerID, uint64 channelGroupID, uint64 channelID, anyID clientID, anyID invokerClientID, const char* invokerName, const char* invokerUniqueIdentity) {
}

int ts3plugin_onServerPermissionErrorEvent(uint64 serverConnectionHandlerID, const char* errorMessage, unsigned int error, const char* returnCode, unsigned int failedPermissionID) {
	return 0;  /* See onServerErrorEvent for return code description */
}

void ts3plugin_onPermissionListGroupEndIDEvent(uint64 serverConnectionHandlerID, unsigned int groupEndID) {
}

void ts3plugin_onPermissionListEvent(uint64 serverConnectionHandlerID, unsigned int permissionID, const char* permissionName, const char* permissionDescription) {
}

void ts3plugin_onPermissionListFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onPermissionOverviewEvent(uint64 serverConnectionHandlerID, uint64 clientDatabaseID, uint64 channelID, int overviewType, uint64 overviewID1, uint64 overviewID2, unsigned int permissionID, int permissionValue, int permissionNegated, int permissionSkip) {
}

void ts3plugin_onPermissionOverviewFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onServerGroupClientAddedEvent(uint64 serverConnectionHandlerID, anyID clientID, const char* clientName, const char* clientUniqueIdentity, uint64 serverGroupID, anyID invokerClientID, const char* invokerName, const char* invokerUniqueIdentity) {
}

void ts3plugin_onServerGroupClientDeletedEvent(uint64 serverConnectionHandlerID, anyID clientID, const char* clientName, const char* clientUniqueIdentity, uint64 serverGroupID, anyID invokerClientID, const char* invokerName, const char* invokerUniqueIdentity) {
}

void ts3plugin_onClientNeededPermissionsEvent(uint64 serverConnectionHandlerID, unsigned int permissionID, int permissionValue) {
}

void ts3plugin_onClientNeededPermissionsFinishedEvent(uint64 serverConnectionHandlerID) {
}

void ts3plugin_onFileTransferStatusEvent(anyID transferID, unsigned int status, const char* statusMessage, uint64 remotefileSize, uint64 serverConnectionHandlerID) {
}

void ts3plugin_onClientChatClosedEvent(uint64 serverConnectionHandlerID, anyID clientID, const char* clientUniqueIdentity) {
}

void ts3plugin_onClientChatComposingEvent(uint64 serverConnectionHandlerID, anyID clientID, const char* clientUniqueIdentity) {
}

void ts3plugin_onServerLogEvent(uint64 serverConnectionHandlerID, const char* logMsg) {
}

void ts3plugin_onServerLogFinishedEvent(uint64 serverConnectionHandlerID, uint64 lastPos, uint64 fileSize) {
}

void ts3plugin_onMessageListEvent(uint64 serverConnectionHandlerID, uint64 messageID, const char* fromClientUniqueIdentity, const char* subject, uint64 timestamp, int flagRead) {
}

void ts3plugin_onMessageGetEvent(uint64 serverConnectionHandlerID, uint64 messageID, const char* fromClientUniqueIdentity, const char* subject, const char* message, uint64 timestamp) {
}

void ts3plugin_onClientDBIDfromUIDEvent(uint64 serverConnectionHandlerID, const char* uniqueClientIdentifier, uint64 clientDatabaseID) {
}

void ts3plugin_onClientNamefromUIDEvent(uint64 serverConnectionHandlerID, const char* uniqueClientIdentifier, uint64 clientDatabaseID, const char* clientNickName) {
}

void ts3plugin_onClientNamefromDBIDEvent(uint64 serverConnectionHandlerID, const char* uniqueClientIdentifier, uint64 clientDatabaseID, const char* clientNickName) {
}

void ts3plugin_onComplainListEvent(uint64 serverConnectionHandlerID, uint64 targetClientDatabaseID, const char* targetClientNickName, uint64 fromClientDatabaseID, const char* fromClientNickName, const char* complainReason, uint64 timestamp) {
}

void ts3plugin_onBanListEvent(uint64 serverConnectionHandlerID, uint64 banid, const char* ip, const char* name, const char* uid, uint64 creationTime, uint64 durationTime, const char* invokerName,
	uint64 invokercldbid, const char* invokeruid, const char* reason, int numberOfEnforcements, const char* lastNickName) {
}

void ts3plugin_onClientServerQueryLoginPasswordEvent(uint64 serverConnectionHandlerID, const char* loginPassword) {
}

void ts3plugin_onPluginCommandEvent(uint64 serverConnectionHandlerID, const char* pluginName, const char* pluginCommand) {
}

void ts3plugin_onIncomingClientQueryEvent(uint64 serverConnectionHandlerID, const char* commandText) {
}

void ts3plugin_onServerTemporaryPasswordListEvent(uint64 serverConnectionHandlerID, const char* clientNickname, const char* uniqueClientIdentifier, const char* description, const char* password, uint64 timestampStart, uint64 timestampEnd, uint64 targetChannelID, const char* targetChannelPW) {
}

/* Client UI callbacks */

/*
* Called from client when an avatar image has been downloaded to or deleted from cache.
* This callback can be called spontaneously or in response to ts3Functions.getAvatar()
*/
void ts3plugin_onAvatarUpdated(uint64 serverConnectionHandlerID, anyID clientID, const char* avatarPath) {
}

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
	char temp[128];
#ifdef DEBUG
	printf("TS3MassMover: onMenuItemEvent: serverConnectionHandlerID=%llu, type=%d, menuItemID=%d, selectedItemID=%llu\n", (long long unsigned int)serverConnectionHandlerID, type, menuItemID, (long long unsigned int)selectedItemID);
#endif
	switch (type) {
	case PLUGIN_MENU_TYPE_GLOBAL:
		/* Global menu item was triggered. selectedItemID is unused and set to zero. */
		switch (menuItemID) {
		case MENU_ID_GLOBAL_TO_ME:
			/* Moves all players from server to your current channel */
			// /m ah
			ExecuteCommand(serverConnectionHandlerID, CMD_MAH, NULL, NULL, NULL);
			break;
		default:
			break;
		}
		break;
	case PLUGIN_MENU_TYPE_CHANNEL:
		/* Channel contextmenu item was triggered. selectedItemID is the channelID of the selected channel */
		sprintf(temp, "%llu", (long long unsigned int)selectedItemID);
		switch (menuItemID) {
		case MENU_ID_CHANNEL_TO_ME:
			/* Moves all players from given channel to your current channel */
			// /m h
			ExecuteCommand(serverConnectionHandlerID, CMD_MH, temp, NULL, NULL);
			break;
		case MENU_ID_CHANNEL_FROM_ME:
			/* Moves all players from your current channel to given channel */
			// /m t
			ExecuteCommand(serverConnectionHandlerID, CMD_MT, temp, NULL, NULL);
			break;
		case MENU_ID_CHANNEL_FROM_SERVER:
			/* Moves all players from server to given channel */
			// /m at
			ExecuteCommand(serverConnectionHandlerID, CMD_MAT, temp, NULL, NULL);
			break;
		default:
			break;
		}
		break;
	case PLUGIN_MENU_TYPE_CLIENT:
		/* Client contextmenu item was triggered. selectedItemID is the clientID of the selected client */
		/*switch(menuItemID) {
		default:
		break;
		}*/
		break;
	default:
		break;
	}
}

/* This function is called if a plugin hotkey was pressed. Omit if hotkeys are unused. */
void ts3plugin_onHotkeyEvent(const char* keyword) {
#ifdef DEBUG
	printf("TS3MassMover: Hotkey event: %s\n", keyword);
#endif
	/* Identify the hotkey by keyword ("keyword_1", "keyword_2" or "keyword_3" in this example) and handle here... */
}

/* Called when recording a hotkey has finished after calling ts3Functions.requestHotkeyInputDialog */
void ts3plugin_onHotkeyRecordedEvent(const char* keyword, const char* key) {
}

/* Called when client custom nickname changed */
void ts3plugin_onClientDisplayNameChanged(uint64 serverConnectionHandlerID, anyID clientID, const char* displayName, const char* uniqueClientIdentifier) {
}
