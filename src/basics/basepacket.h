#ifndef __BASEPACKET_H__
#define __BASEPACKET_H__

#include "basetypes.h"


///////////////////////////////////////////////////////////////////////////////
NAMESPACE_BEGIN(basics)
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// base packet.
class packetbase
{
protected:
	uint8* _buf;
	size_t _len;
public:
	/// Length of the packet
	size_t length() const;
	/// Data of the packet
	const uint8* buffer() const;
};


///////////////////////////////////////////////////////////////////////////////
NAMESPACE_END(basics)
///////////////////////////////////////////////////////////////////////////////

#endif//__BASEPACKET_H__
