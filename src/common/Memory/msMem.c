/*

  Copyright � Grame 1999

  This library is free software; you can redistribute it and modify it under 
  the terms of the GNU Library General Public License as published by the 
  Free Software Foundation version 2 of the License, or any later version.

  This library is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public License 
  for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

  Grame Research Laboratory, 9, rue du Garet 69001 Lyon - France
  grame@rd.grame.fr

*/

#ifdef __Macintosh__
#include <MacMemory.h>
#endif

#ifdef __Linux__
#include <linux/slab.h>
#endif

#include "msMem.h"

/*_________________________________________________________________________*/
/* memory allocation implementation                                        */
/*_________________________________________________________________________*/
FarPtr(void) AllocateMemory (MemoryType type, unsigned long size)
{
#ifdef __Macintosh__
	return NewPtrSys (size);
#endif

#ifdef __Linux__
	return (void*)kmalloc(size, GFP_KERNEL);
#endif
}

/*_________________________________________________________________________*/
void DisposeMemory  (FarPtr(void) memPtr)
{
#ifdef __Macintosh__
	if (memPtr) DisposePtr ((Ptr)memPtr);
#endif

#ifdef __Linux__
	kfree(memPtr);
#endif
}

