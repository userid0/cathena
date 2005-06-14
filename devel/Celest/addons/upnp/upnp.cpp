
#include "upnp.h"
#include "dll.h"
#include <stdio.h>
#include <windows.h>
#include <netfw.h>
#include <natupnp.h>
#include <comutil.h>
#include <stdarg.h>

ADDON_INFO = {
	"UPnP",
	ADDON_CORE,
	"1.03",
	DLL_VERSION,
	"UPNP / Firewall Plugin by eAthena"
};

ADDON_EVENTS_TABLE = {
	{ "upnp_init", "DLL_Init" },
	{ "upnp_final", "DLL_Final" },
	{ "CheckWindowsVersion", "DLL_Test" },
	{ NULL, NULL }
};

	ADDON_CALL_TABLE = NULL;
	INetFwProfile* profile = NULL;
	IStaticPortMappingCollection *collection = NULL;

	int validWindowsVersion = -1;
	int enableUPnP = 1;
	
	int release_mappings = 1;
	int close_ports = 1;

	int upnpPort = 0;
	int firewallPort = 0;

	int firewall_enabled = 0;
	int upnp_enabled = 0;

inline void ShowMessage (const char *msg, ...){
	va_list ap;
	va_start(ap, msg);
	vprintf (msg, ap);
	va_end (ap);
}
inline void ShowDebug (const char *msg, ...){
#ifdef _DEBUG
	va_list ap;
	va_start(ap, msg);
	vprintf (msg, ap);
	va_end (ap);
#endif
}

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
		ShowDebug ("Firewall not enabled\n");
		manager->Release();
		policy->Release();
		return 0;
	}

	profile->get_ExceptionsNotAllowed(&enabled);
	if (enabled == VARIANT_TRUE) {
		ShowDebug ("Firewall exceptions not allowed\n");
		manager->Release();
		policy->Release();
		return 0;
	}

	if (manager) manager->Release();
	if (policy) policy->Release();

	ShowDebug ("Firewall init done\n");

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
	ShowDebug ("Get ports\n");

	ports->Item(portNum, NET_FW_IP_PROTOCOL_TCP, &port);
	if (port) {
		ShowDebug("Port %d found\n", portNum);
		port->get_Enabled(&enabled);
		ShowDebug("Check... ");
		if (enabled == VARIANT_TRUE) {
			ShowDebug ("Already enabled\n");
			return 0;
		}
		ShowDebug ("Not enabled\n");
	}

	ShowDebug ("Create port\n");
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
	ShowDebug ("Add port %d done\n", portNum);

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
		ShowMessage ("Reclosing firewall on port %d.\n", firewallPort);
		Firewall_RemovePort (firewallPort);
	}
	if (profile) profile->Release();
	return 1;
}

int UPNP_init ()
{	
	IUPnPNAT *nat = NULL;

	ShowDebug ("init NAT...");
	CoCreateInstance(__uuidof(UPnPNAT), NULL, CLSCTX_ALL,
		__uuidof(IUPnPNAT), (void**)&nat);
	if (!nat) {
		ShowDebug ("Failed\n");
		return 0;
	}
	ShowDebug ("\n");

	ShowDebug ("Getting collection...");
	nat->get_StaticPortMappingCollection(&collection);
	if (!collection) {
		ShowDebug ("Failed\n");
		nat->Release();
		return 0;
	}
	ShowDebug ("\n");

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

	ShowDebug ("Checking if mapping to %s:%d exists\n", ip, portNum);
	collection->get_Item(portNum, protocol, &mapping);
	if (mapping != NULL) {
		ShowDebug ("Removing old mapping\n");
		UPNP_RemovePort (portNum);
		mapping->Release();
	}

	ShowDebug ("Adding port mapping...");
	collection->Add(portNum, protocol, portNum, address, true, name, &mapping);
	if (!mapping) {
		ShowDebug ("Failed\n");
		return 0;
	}
	ShowDebug ("Success\n");
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
		ShowMessage ("Releasing port mappings on %d.\n", upnpPort);
		UPNP_RemovePort (upnpPort);
	}
	if (collection) collection->Release();
	return 1;
}

int CheckWindowsVersion ()
{
	OSVERSIONINFO vi;

	vi.dwOSVersionInfoSize = sizeof(vi);
	GetVersionEx(&vi);

	validWindowsVersion = (vi.dwMajorVersion >=5 && vi.dwMinorVersion >= 1);
	return validWindowsVersion;
}

int upnp_final ()
{
	if (upnp_enabled)
		UPNP_finalize ();
	if (firewall_enabled)
		Firewall_finalize ();
	CoUninitialize();

	return 1;
}

int upnp_init ()
{
	char line[1024], w1[1024], w2[1024];
	char *server_type;
	char *server_name;
	char *conf_file[2];
	char *port_name;
	int port_num;
	char *natip = NULL;
	FILE *fp;
	int i;

	if (validWindowsVersion == -1)
		CheckWindowsVersion();
	if (!validWindowsVersion)
		return 0;

	server_type = (char *)addon_call_table[0];
	server_name = (char *)addon_call_table[1];

	switch(*server_type)
	{
	case ADDON_LOGIN:	// login
		conf_file[0] = "conf/login_athena.conf";
		port_name = "login_port";
		port_num = 6900;
		break;
	case ADDON_CHAR:	// char
		conf_file[0] = "conf/char_athena.conf";
		port_name = "char_port";
		port_num = 6121;
		break;
	case ADDON_MAP:	// map
		conf_file[0] = "conf/map_athena.conf";
		port_name = "map_port";
		port_num = 5121;
		break;
	default:
		return 0;
	}
	
	conf_file[1] = "addons/upnp.conf";

	for (i = 0; i < 2; i++)
	{
		fp = fopen(conf_file[i], "r");
		if (fp == NULL)
			continue;

		while(fgets(line, sizeof(line) -1, fp)) {
			if (line[0] == '/' && line[1] == '/')
				continue;
			if (sscanf(line, "%[^:]: %[^\r\n]", w1, w2) == 2) {
				if(strcmpi(w1,"enable_upnp")==0){
					enableUPnP = atoi(w2);
				} else if(strcmpi(w1,"release_mappings")==0){
					release_mappings = atoi(w2);
				} else if(strcmpi(w1,"close_ports")==0){
					close_ports = atoi(w2);
				} else if(strcmpi(w1,port_name)==0){
					port_num = atoi(w2);
				}// else if(strcmpi(w1,"lan_ip")==0){
				//	;
				//}
			}
		}
		fclose(fp);
	}

	if (!enableUPnP)
		return 0;

	CoInitializeEx(0, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	firewall_enabled = Firewall_init();
	upnp_enabled = UPNP_init();

	if (firewall_enabled &&
		Firewall_AddPort(server_name, port_num))
		ShowMessage ("Firewall port %d successfully opened.\n", port_num);

	if (upnp_enabled) {
		if (!natip) {
			unsigned int *addr = (unsigned int *)addon_call_table[12];
			int localaddr = ntohl(addr[0]);
			natip = (char *)&localaddr;
			ShowDebug("natip=%d.%d.%d.%d\n", natip[0], natip[1], natip[2], natip[3]);
		}
		if (natip[0] == 192 && natip[1] == 168 &&
			UPNP_AddPort(server_name, natip, port_num))
			ShowMessage ("UPnP mapping successfull.\n");
	}
	
	return 1;
}
