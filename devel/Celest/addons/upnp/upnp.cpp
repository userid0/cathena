
#include "upnp.h"
#include <stdio.h>
#include <windows.h>
#include <netfw.h>
#include <natupnp.h>
#include <comutil.h>

INetFwProfile* profile = NULL;

IUPnPNAT *nat = NULL;
IStaticPortMappingCollection *collection = NULL;

#ifdef _DEBUG
	#define ShowMessage(msg) printf(msg);
#else
	#define ShowMessage(msg)
#endif

int Firewall_init ()
{
	INetFwMgr* manager = NULL;
	INetFwPolicy* policy = NULL;

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

	if (manager) manager->Release();
	if (policy) policy->Release();

	ShowMessage ("Firewall init done\n");

	return 1;
}
int Firewall_finalize ()
{
	if (profile) profile->Release();
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
	ShowMessage ("Add port done\n");

	if (port) port->Release();
	if (ports) ports->Release();
	
	return 1;
}

int UPNP_init ()
{	
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
	
	return 1;
}
int UPNP_finalize ()
{
	//if (nat) nat->Release();
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

int do_init ()
{
	int ret = 0;
	OSVERSIONINFO vi;

	vi.dwOSVersionInfoSize = sizeof(vi);
	GetVersionEx(&vi);

	if (vi.dwMajorVersion <5 || (vi.dwMajorVersion == 5 && vi.dwMinorVersion <1))
		return 0;

	CoInitializeEx(0, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	UPNP_init();
	Firewall_init();
	//profile->put_FirewallEnabled(VARIANT_TRUE);
	
	return 1;
}

int do_final ()
{
	OSVERSIONINFO vi;

	vi.dwOSVersionInfoSize = sizeof(vi);
	GetVersionEx(&vi);
	if (vi.dwMajorVersion <5 || (vi.dwMajorVersion == 5 && vi.dwMinorVersion <1))
		return 0;

	CoUninitialize();
	UPNP_finalize ();
	Firewall_finalize ();

	return 1;
}