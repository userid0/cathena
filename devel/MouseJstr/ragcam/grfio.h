//
// PvPGN YARE V1.1.1 (Yet Another Ragnarok Emulator) - (Server)
// Copyright (c) Project-YARE & PvPGN 2003
// www.project-yare.net
// forum.project-yare.net
// www.pvpgn.org
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// $Id: grfio.h,v 1.6 2003/08/18 23:33:12 mra Exp $
//
// grfio.h - Grf I/O Include file

#ifndef	_GRFIO_H_
#define	_GRFIO_H_

void grfio_init(void);		// GRFIO Initialize
int grfio_add(char*);		// GRFIO Resource file add
void* grfio_read(char*);	// GRFIO data file read
int grfio_size(char*);		// GRFIO data file size get

#endif	/* _GRFIO_H_ */
