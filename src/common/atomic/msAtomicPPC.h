/*

  Copyright � Grame 1999-2005

  This library is free software; you can redistribute it and modify it under 
  the terms of the GNU Library General Public License as published by the 
  Free Software Foundation version 2 of the License, or any later version.

  This library is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public 
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

  Grame Research Laboratory, 9, rue du Garet 69001 Lyon - France
  research@grame.fr

*/

#ifndef __msAtomicPPC__
#define __msAtomicPPC__

#define inline __inline__

//----------------------------------------------------------------
// Memory reservation only
//----------------------------------------------------------------
static inline void LWARX (register volatile void * addr) 
{
	asm volatile (
       "# LWARX					\n"
        "	lwarx	r0, 0, %0	\n"         /* creates a reservation on addr  */
       :
	   : "r" (addr)
       : "r0"
 	);
}

//----------------------------------------------------------------
// Store conditionnal
//----------------------------------------------------------------
static inline int STWCX (register volatile void * addr, register void * value, register void * newvalue) 
{
    register int result;
	asm volatile (
       "# STWCX					\n"
		"	lwz		r0, 0(%1)	\n"         /* load value in pointed by addr  */
		"	cmpw	r0, %2		\n"         /* test value at addr             */
		"	bne-	1f          \n"
        "	sync            	\n"         /* synchronize instructions       */
		"	stwcx.	%3, 0, %1	\n"         /* if the reservation is not altered */
                                            /* stores the new value at addr   */
		"	bne-	1f          \n"
        "   li      %0, 1       \n"
		"	b		2f          \n"
        "1:                     \n"
        "   li      %0, 0       \n"
        "2:                     \n"
       :"=r" (result)
	   : "r" (addr), "r" (value), "r" (newvalue)
       : "r0"
 	);
    return result;
}

//----------------------------------------------------------------
// Compare and swap
//----------------------------------------------------------------
static inline int CAS (register volatile void * addr, register void * value, register void * newvalue) 
{
	register int result;
	asm volatile (
       "# CAS					\n"
		"	lwarx	r0, 0, %1	\n"         /* creates a reservation on addr  */
		"	cmpw	r0, %2		\n"         /* test value at addr             */
		"	bne-	1f          \n"
        "	sync            	\n"         /* synchronize instructions       */
		"	stwcx.	%3, 0, %1	\n"         /* if the reservation is not altered */
                                            /* stores the new value at addr   */
		"	bne-	1f          \n"
        "   li      %0, 1       \n"
		"	b		2f          \n"
        "1:                     \n"
        "   li      %0, 0       \n"
        "2:                     \n"
       :"=r" (result)
	   : "r" (addr), "r" (value), "r" (newvalue)
       : "r0"
 	);
	return result;
}

//----------------------------------------------------------------
// Compare and swap link if not equal to eq
//----------------------------------------------------------------
static inline int CASLNE (register volatile void * addr, register void * value, register void * eq) 
{
	register int result;
	asm volatile (
        "# CASLNE				\n"
		"	lwarx	r0, 0, %1	\n"         /* creates a reservation on addr  */
		"	cmpw	r0, %2		\n"         /* test value at addr             */
		"	bne-	1f          \n"         /* fails if not equal to value    */
		"	lwzx	r0, 0, %2	\n"         /* load the link pointed by value */
		"	cmpw	r0, %3		\n"         /* test if equal to eq            */
		"	beq-	1f          \n"
        "	sync            	\n"         /* synchronize instructions       */
		"	stwcx.	r0, 0, %1	\n"         /* if the reservation is not altered */
                                            /* stores the new value at addr   */

		"	bne-	1f          \n"
        "   li      %0, 1       \n"
		"	b		2f          \n"
        "1:                     \n"
        "   li      %0, 0       \n"
        "2:                     \n"

       :"=r" (result)
	   : "r" (addr), "r" (value), "r" (eq)
	   : "r0"
 	);
	return result;
}

#endif
