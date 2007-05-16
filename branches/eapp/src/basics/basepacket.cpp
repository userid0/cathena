#include "basepacket.h"



///////////////////////////////////////////////////////////////////////////////
NAMESPACE_BEGIN(basics)
///////////////////////////////////////////////////////////////////////////////



/// Length of the packet
size_t packetbase::length() const
{ 
	return _len;
}

/// Data of the packet
const uint8* packetbase::buffer() const
{ 
	return _buf;
}



///////////////////////////////////////////////////////////////////////////////
NAMESPACE_END(basics)
///////////////////////////////////////////////////////////////////////////////
