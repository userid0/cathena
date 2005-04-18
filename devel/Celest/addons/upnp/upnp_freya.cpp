// $Id: upnp_freya.cpp 1 2005-3-10 3:17:17 PM Celestia $
// Addon for Freya

#define __ADDON
#include <stdio.h>
#include <windows.h>
#include <netfw.h>
#include <natupnp.h>
#include <comutil.h>
#include <objbase.h>

#include "addons.h"
#include "addons_exports.h"

////// Functions ///////
int Firewall_init ();
int Firewall_finalize ();
int Firewall_AddPort (char *, int);
int UPNP_init ();
int UPNP_finalize ();
int UPNP_AddPort (char *, char *, int);
int UPNP_RemovePort (int);
int Config_Read ();

/////// Global variables //////
INetFwProfile* profile = NULL;
IStaticPortMappingCollection *collection = NULL;

int release_mappings = 1;
int upnpPort = 0;
int close_ports = 1;
int firewallPort = 0;
char lan_IP[16];
char description[16];

DLLFUNC ModuleHeader MOD_HEADER(upnp) = {
	"UPnP Freya",
	"$Id: upnp.c 0.1",
	"Controls UPnP functions for Freya",
	MOD_VERSION,
	ADDONS_ALL
};

/////// Plugin Core ///////
DLLFUNC int MOD_TEST(upnp)() {
	OSVERSIONINFO vi;

	vi.dwOSVersionInfoSize = sizeof(vi);
	GetVersionEx(&vi);

	if (vi.dwMajorVersion >=5 && vi.dwMinorVersion >= 1)
		return MOD_SUCCESS;
	return MOD_FAILED;
}

DLLFUNC int MOD_INIT(upnp)(void *ct) {
	//call_table=ct;
	/* load local symbols table */
	//MFNC_LOCAL_TABLE(local_table);

	CoInitializeEx(0, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	UPNP_init();
	Firewall_init();

	if (Config_Read() != 0)
		return MOD_SUCCESS;
	return MOD_FAILED;
}

DLLFUNC int MOD_LOAD(upnp)() {
	return MOD_SUCCESS;
}

DLLFUNC int MOD_UNLOAD(upnp)() {
	UPNP_finalize ();
	Firewall_finalize ();
	CoUninitialize();

	return MOD_SUCCESS;
}

/////// UPnP Functions ///////
#ifdef _DEBUG
#define ShowMessage(msg) printf(msg);
#else
	#define ShowMessage(msg)
#endif

int Firewall_init ()
{
	INetFwMgr* manager = NULL;
	INetFwPolicy* policy = NULL;
	VARIANT_BOOL enabled;

	CoCreateInstance(__uuidof(NetFwMgr), NULL, CLSCTX_INPROC_SERVER,
		__uuidof(INetFwMgr), (void**)&manager);
	if (!manager)
		return 0;
	
	manager->get_LocalPolicy(&policy);
	if (!policy) {
		manager->Release();
		return 0;
	}

	policy->get_CurrentProfile(&profile);
	if (!profile) {
		manager->Release();
		policy->Release();
		return 0;
	}

	profile->get_FirewallEnabled(&enabled);
	if (enabled == VARIANT_FALSE) {
		ShowMessage ("Firewall not enabled\n");
		manager->Release();
		policy->Release();
		return 0;
	}

	profile->get_ExceptionsNotAllowed(&enabled);
	if (enabled == VARIANT_TRUE) {
		ShowMessage ("Firewall exceptions not allowed\n");
		manager->Release();
		policy->Release();
		return 0;
	}

	if (manager) manager->Release();
	if (policy) policy->Release();

	ShowMessage ("Firewall init done\n");

	return 1;
}
int Firewall_AddPort (char *desc, int portNum)
{
	INetFwOpenPort* port = NULL;
    INetFwOpenPorts* ports = NULL;
	VARIANT_BOOL enabled;
	BSTR name = NULL;

	if (!profile)
		return 0;

	profile->get_GloballyOpenPorts(&ports);
	if (!ports)
		return 0;
	ShowMessage ("Get ports\n");

	ports->Item(portNum, NET_FW_IP_PROTOCOL_TCP, &port);
	if (port) {
		ShowMessage("Port found\n");
		port->get_Enabled(&enabled);
		ShowMessage("Check... ");
		if (enabled == VARIANT_TRUE) {
			ShowMessage ("Already enabled\n");
			return 0;
		}
		ShowMessage ("Not enabled\n");
	}

	ShowMessage ("Create port\n");
	CoCreateInstance(__uuidof(NetFwOpenPort), NULL, CLSCTX_INPROC_SERVER,
		__uuidof(INetFwOpenPort), (void**)&port);
	if (!port) {
		ports->Release();
		return 0;
	}
	
	port->put_Port(portNum);
	port->put_Protocol(NET_FW_IP_PROTOCOL_TCP);
	name = _com_util::ConvertStringToBSTR(desc);
	port->put_Name(name);
	ports->Add(port);
	firewallPort = portNum;
	ShowMessage ("Add port done\n");

	if (port) port->Release();
	if (ports) ports->Release();
	
	return 1;
}
int Firewall_RemovePort (int portNum)
{
	INetFwOpenPorts* ports = NULL;
	
	if (!profile)
		return 0;

	profile->get_GloballyOpenPorts(&ports);
	if (!ports)
		return 0;
	
	ports->Remove(portNum, NET_FW_IP_PROTOCOL_TCP);
	ports->Release ();
	return 1;
}
int Firewall_finalize ()
{
	if (close_ports && firewallPort > 0) {
		ShowMessage ("Closing firewall port\n");
		Firewall_RemovePort (firewallPort);
	}
	if (profile) profile->Release();
	return 1;
}

int UPNP_init ()
{	
	IUPnPNAT *nat = NULL;

	ShowMessage ("init NAT...");
	CoCreateInstance(__uuidof(UPnPNAT), NULL, CLSCTX_ALL,
		__uuidof(IUPnPNAT), (void**)&nat);
	if (!nat) {
		ShowMessage ("Failed\n");
		return 0;
	}
	ShowMessage ("\n");

	ShowMessage ("Getting collection...");
	nat->get_StaticPortMappingCollection(&collection);
	if (!collection) {
		ShowMessage ("Failed\n");
		nat->Release();
		return 0;
	}
	ShowMessage ("\n");

	if (nat) nat->Release();	
	return 1;
}
int UPNP_AddPort (char *desc, char *ip, int portNum)
{
	IStaticPortMapping *mapping = NULL;
	BSTR protocol = L"TCP";
	BSTR address = _com_util::ConvertStringToBSTR(ip);
	BSTR name = _com_util::ConvertStringToBSTR(desc);
	if (!collection)
		return 0;

	ShowMessage ("Checking if mapping exists\n");
	collection->get_Item(portNum, protocol, &mapping);
	if (mapping != NULL) {
		ShowMessage ("Removing old mapping\n");
		UPNP_RemovePort (portNum);
		mapping->Release();
	}

	ShowMessage ("Adding port mapping...");
	collection->Add(portNum, protocol, portNum, address, true, name, &mapping);
	if (!mapping) {
		ShowMessage ("Failed\n");
		return 0;
	}
	ShowMessage ("Success\n");
	upnpPort = portNum;

	return 1;
}
int UPNP_RemovePort (int portNum)
{
	BSTR protocol = L"TCP";
	if (!collection)
		return 0;

	collection->Remove(portNum, protocol);
	return 1;
}
int UPNP_finalize ()
{
	if(release_mappings && upnpPort > 0) {
		ShowMessage ("Releasing port mappings\n");
		UPNP_RemovePort (upnpPort);
	}
	if (collection) collection->Release();
	return 1;
}
////// End UPnP Functions //////

////// Configuration Setting //////
int Config_Read ()
{
	char line[1024], w1[1024], w2[1024];
	FILE *fp;
	int port;

	fp = fopen("addons/upnp_athena.conf","r");
	if (fp == NULL) {
		printf("File not found: upnp_athena.conf\n");
		return 0;
	}

	while(fgets(line, sizeof(line)-1, fp)){
		if (line[0] == '/' && line[1] == '/')
			continue;
		if (sscanf(line, "%[^:]:%s", w1, w2) != 2)
			continue;
		if(strcmpi(w1,"lan_ip") == 0) {
			memcpy (lan_IP, w2, 15);
			lan_IP[15] = '\0';
		} else if(strcmpi(w1,"description") == 0) {
			memcpy (description, w2, 15);
			description[15] = '\0';
		} else if(strcmpi(w1,"port") == 0) {
			port = atoi(w2);
		}				
	}
	fclose(fp);

	Firewall_AddPort (description, port);
	UPNP_AddPort (description, lan_IP, port);

	return 1;
}