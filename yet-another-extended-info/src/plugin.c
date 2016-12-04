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

static struct TS3Functions ts3Functions;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif

#define PLUGIN_API_VERSION 21
#define PATH_BUFSIZE 512
#define INFODATA_BUFSIZE 512
#define SERVERINFO_BUFSIZE 512
#define CHANNELINFO_BUFSIZE 512

#define PLUGIN_NAME "YAEIP(Yet Another Extended Info Plugin)"
#define PLUGIN_AUTHOR "Kleberstoff"
#define PLUGIN_VERSION "0.0"

static char* pluginID = NULL;

static struct PluginMenuItem* createMenuItem(enum PluginMenuType type, int id, const char* text, const char* icon) {
	struct PluginMenuItem* menuItem = (struct PluginMenuItem*)malloc(sizeof(struct PluginMenuItem));
	menuItem->type = type;
	menuItem->id = id;
	_strcpy(menuItem->text, PLUGIN_MENU_BUFSZ, text);
	_strcpy(menuItem->icon, PLUGIN_MENU_BUFSZ, icon);
	return menuItem;
}

#ifdef _WIN32

static int wcharToUtf8(const wchar_t* str, char** result) {
	int outlen = WideCharToMultiByte(CP_UTF8, 0, str, -1, 0, 0, 0, 0);
	*result = (char*)malloc(outlen);
	if(WideCharToMultiByte(CP_UTF8, 0, str, -1, *result, outlen, 0, 0) == 0)
	{
		*result = NULL;
		return -1;
	}
	return 0;
}
#endif

/* REQUIRED FUNCTIONS */

const char* ts3plugin_name() {
#ifdef _WIN32
	
	static char* result = NULL;
	if (!result) {
		const wchar_t* name = L"YAEIP";
		if (wcharToUtf8(name, &result) == -1)
		{
			result = "YAEIP";
		}
	}
	return result;
#else
	return "YAEIP";
#endif
}

const char* ts3plugin_version()
{
	return PLUGIN_VERSION;
}

int ts3plugin_apiVersion()
{
	return PLUGIN_API_VERSION;
}

const char* ts3plugin_author()
{
	return PLUGIN_AUTHOR;
}

const char* ts3plugin_description()
{
	return "Provides some extra Information.";
}

void ts3plugin_setFunctionPointers(const struct TS3Functions funcs)
{
	ts3Functions = funcs;
}

int ts3plugin_init()
{
	printf("YAEIP: init\n");

	return 0;
}

void ts3plugin_shutdown()
{
	printf("YAEIP: shutdown\n");

	if (pluginID)
	{
		free(pluginID);
			pluginID = NULL;
	}
}

void ts3plugin_registerPluginID(const char* id)
{
	const size_t sz = strlen(id) + 1;
	pluginID = (char*)malloc(sz * sizeof(char));
	_strcpy(pluginID, sz, id);
	printf("YAEIP: registerPluginID: %s\n", pluginID);
}

const char* ts3plugin_infoTitle()
{
	return "YAEIP";
}

/* REQUIRED FUNCTIONS END */

void ts3plugin_infoData(uint64 serverConnectionHandlerID, uint64 id, enum PluginItemType type, char** data)
{
	anyID ownid;
	int sid, client_type_int, unread_messages, totalconnection, channel_forced_silence, channel_flag_private, needed_talkpower;
	uint64 month_bytes_uploaded, month_bytes_downloaded, total_bytes_uploaded, total_bytes_downloaded, iconid, created;
	char* client_type;
	char* client_country;
	char* client_uid;
	char* client_nickname;
	char* ip;
	char* port;
	char* ping;
	char* tping;
	char* idletime;
	char* phonetic_name;
	char* totalpacketloss;
	char* speechpacketloss;
	char* keepalivepacketloss;
	char* controlpacketloss;

	switch (type)
	{
	case PLUGIN_SERVER:
		printf("Another Extended Info Plugun: Clicked Server \n");
		//int
		if (ts3Functions.getServerVariableAsInt(serverConnectionHandlerID, VIRTUALSERVER_ID, &sid) != ERROR_ok)
		{
			return;
		}
		//uint64
		if (ts3Functions.getServerVariableAsUInt64(serverConnectionHandlerID, VIRTUALSERVER_MONTH_BYTES_UPLOADED, &month_bytes_uploaded) != ERROR_ok)
		{
			return;
		}
		if (ts3Functions.getServerVariableAsUInt64(serverConnectionHandlerID, VIRTUALSERVER_MONTH_BYTES_DOWNLOADED, &month_bytes_downloaded) != ERROR_ok)
		{
			return;
		}
		if (ts3Functions.getServerVariableAsUInt64(serverConnectionHandlerID, VIRTUALSERVER_TOTAL_BYTES_UPLOADED, &total_bytes_uploaded) != ERROR_ok)
		{
			return;
		}
		if (ts3Functions.getServerVariableAsUInt64(serverConnectionHandlerID, VIRTUALSERVER_TOTAL_BYTES_DOWNLOADED, &total_bytes_downloaded) != ERROR_ok)
		{
			return;
		}
		//string
		if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_TOTAL_PACKETLOSS_TOTAL, &totalpacketloss) != ERROR_ok)
		{
			return;
		}
		if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_TOTAL_PACKETLOSS_SPEECH, &speechpacketloss) != ERROR_ok)
		{
			return;
		}
		if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_TOTAL_PACKETLOSS_KEEPALIVE, &keepalivepacketloss) != ERROR_ok)
		{
			return;
		}
		if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_TOTAL_PACKETLOSS_CONTROL, &controlpacketloss) != ERROR_ok)
		{
			return;
		}
		if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_TOTAL_PING, &tping) != ERROR_ok)
		{
			return;
		}
		if (ts3Functions.getClientID(serverConnectionHandlerID, &ownid) == ERROR_ok) {
			if (ts3Functions.getConnectionVariableAsString(serverConnectionHandlerID, ownid, 6, &ip) != ERROR_ok) {
				return;
			}
		}
		if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_PORT, &port) != ERROR_ok)
		{
			return;
		}

		*data = (char*)malloc(INFODATA_BUFSIZE * sizeof(char));
		snprintf(*data, INFODATA_BUFSIZE, "\n Virtualserver ID: %i \n Adress: %s:%s \n Packetloss: \n Total: %s \n Speech: %s \n Keepalive: /%s \n Control: %s \n Average Ping: %s \n Down-/Uploaded This Month: %I64u/%I64u B \n Down-/Uploaded This Total: %I64u/%I64u B \n", sid, ip , port, totalpacketloss, speechpacketloss, keepalivepacketloss, controlpacketloss, tping , month_bytes_uploaded, month_bytes_downloaded, total_bytes_uploaded, total_bytes_downloaded);
		break;
	case PLUGIN_CHANNEL:
		printf("Another Extended Info Plugun: Clicked Channel \n");
		//int
		if (ts3Functions.getChannelVariableAsInt(serverConnectionHandlerID, (anyID)id, CHANNEL_FORCED_SILENCE, &channel_forced_silence) != ERROR_ok)
		{
			return;
		}
		if (ts3Functions.getChannelVariableAsInt(serverConnectionHandlerID, (anyID)id, CHANNEL_FLAG_PRIVATE, &channel_flag_private) != ERROR_ok)
		{
			return;
		}
		if (ts3Functions.getChannelVariableAsInt(serverConnectionHandlerID, (anyID)id, CHANNEL_NEEDED_TALK_POWER, &needed_talkpower) != ERROR_ok)
		{
			return;
		}
		//uint64
		if (ts3Functions.getChannelVariableAsUInt64(serverConnectionHandlerID, (anyID)id, CHANNEL_ICON_ID, &iconid) != ERROR_ok)
		{
			return;
		}
		if (ts3Functions.getChannelVariableAsUInt64(serverConnectionHandlerID, (anyID)id, CHANNEL_CODEC_LATENCY_FACTOR, &created) != ERROR_ok)
		{
			return;
		}
		//string
		if (ts3Functions.getChannelVariableAsString(serverConnectionHandlerID, (anyID)id, CHANNEL_NAME_PHONETIC, &phonetic_name) != ERROR_ok)
		{
			return;
		}

		*data = (char*)malloc(INFODATA_BUFSIZE * sizeof(char));
		snprintf(*data, INFODATA_BUFSIZE, "\n Phonetic Name: %s \n Icon-ID: %I64u \n Needed Talk-Power: %i \n Forced-Silence: %i \n Private-Flag: %i \n Latency Factor:: %I64u", phonetic_name, iconid, needed_talkpower, channel_forced_silence, channel_flag_private, created);
		break;
	case PLUGIN_CLIENT:
		printf("Another Extended Info Plugun: Clicked Client \n");

		ts3Functions.requestClientVariables(serverConnectionHandlerID, (anyID)id, NULL);
		ts3Functions.requestConnectionInfo(serverConnectionHandlerID, (anyID)id, NULL);
		//int
		if (ts3Functions.getClientVariableAsInt(serverConnectionHandlerID, (anyID)id, CLIENT_TYPE, &client_type_int) != ERROR_ok)
		{
			return;
		}
		if (client_type_int == 0)
		{
			client_type = "Client";
		}
		else if (client_type_int == 1)
		{
			client_type = "Query";
		}
		else
		{
			client_type = "Unkown Client Object (UCO)";
		}
		if (ts3Functions.getClientVariableAsInt(serverConnectionHandlerID, (anyID)id, CLIENT_UNREAD_MESSAGES, &unread_messages) != ERROR_ok)
		{
			return;
		}
		if (ts3Functions.getClientVariableAsInt(serverConnectionHandlerID, (anyID)id, CLIENT_TOTALCONNECTIONS, &totalconnection) != ERROR_ok)
		{
			return;
		}
		//uint64
		if (ts3Functions.getClientVariableAsUInt64(serverConnectionHandlerID, (anyID)id, CLIENT_MONTH_BYTES_UPLOADED, &month_bytes_uploaded) != ERROR_ok)
		{
			return;
		}
		if (ts3Functions.getClientVariableAsUInt64(serverConnectionHandlerID, (anyID)id, CLIENT_MONTH_BYTES_DOWNLOADED, &month_bytes_downloaded) != ERROR_ok)
		{
			return;
		}
		if (ts3Functions.getClientVariableAsUInt64(serverConnectionHandlerID, (anyID)id, CLIENT_TOTAL_BYTES_UPLOADED, &total_bytes_uploaded) != ERROR_ok)
		{
			return;
		}
		if (ts3Functions.getClientVariableAsUInt64(serverConnectionHandlerID, (anyID)id, CLIENT_TOTAL_BYTES_DOWNLOADED, &total_bytes_downloaded) != ERROR_ok)
		{
			return;
		}
		//string
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_NICKNAME_PHONETIC, &phonetic_name) != ERROR_ok)
		{
			return;
		}
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_COUNTRY, &client_country) != ERROR_ok)
		{
			return;
		}
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_UNIQUE_IDENTIFIER, &client_uid) != ERROR_ok)
		{
			return;
		}
		if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_NICKNAME, &client_nickname))
		{
			return;
		}

		/* CONNECTION VARIABLES */
		unsigned int error;
		error = NULL;
		error = ts3Functions.getConnectionVariableAsString(serverConnectionHandlerID, (anyID)id, CONNECTION_CLIENT_IP, &ip);
		if (error != NULL)
		{
			ip = "Couldn't get IP";
		}
		error = NULL;
		error = ts3Functions.getConnectionVariableAsString(serverConnectionHandlerID, (anyID)id, CONNECTION_PING, &ping);
		if (error != NULL)
		{
			ping = "Couldn't get Ping";
		}
		error = NULL;
		error = ts3Functions.getConnectionVariableAsString(serverConnectionHandlerID, (anyID)id, CONNECTION_IDLE_TIME, &idletime) != ERROR_ok;
		if (error != NULL)
		{
			idletime = "/";
		}
		error = NULL;
		/* CONNECTION VARIABLES END */

		*data = (char*)malloc(INFODATA_BUFSIZE * sizeof(char));
		snprintf(*data, INFODATA_BUFSIZE, "\n Nickname: %s \n  Phonetic-Name: %s \nIP: %s \n Ping: %s \n Unique-ID: %s \n Total Connections: %i \n Client Type: %s \n Country: %s \n Idle-Time(ms): %s \n Unread Messages: %i \n Down-/Uploaded This Month: %I64u/%I64u B \n Down-/Uploaded Total: %I64u/%I64u B \n", client_nickname, phonetic_name, ip, ping, client_uid, totalconnection, client_type, client_country, idletime, unread_messages, month_bytes_downloaded, month_bytes_uploaded, total_bytes_downloaded, total_bytes_uploaded);
		break;
	default:
		break;
	}
}

void ts3plugin_freeMemory(void* data) {
	free(data);
}

int ts3plugin_requestAutoload()
{
	return 0;
}