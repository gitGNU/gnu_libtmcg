/*******************************************************************************
   This file is part of libTMCG.

 Copyright (C) 2004  Heiko Stamer <stamer@gaos.org>

   libTMCG is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   libTMCG is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with libTMCG; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*******************************************************************************/

#ifndef INCLUDED_VTMF_CardSecret_HH
	#define INCLUDED_VTMF_CardSecret_HH

	// config.h
	#if HAVE_CONFIG_H
		#include "config.h"
	#endif

	// C and STL header
	#include <cassert>
	#include <string>
	#include <iostream>
	
	// GNU multiple precision library
	#include <gmp.h>
	
	#include "parse_helper.hh"

/** @brief A data structure for the secrets involved in the card masking
    operation.
    
    This struct represents the secrets of the masking operation in the
    discrete logarithm instantiation of the general cryptographic
    primitive "Verifiable-l-out-of-l Threshold Masking Function" by
    Barnett and Smart [BS03]. */
struct VTMF_CardSecret
{
	/** @f$r\in\mathcal{R}@f$ is the randomizer of the masking operation.
	    It should be chosen uniformly and randomly from @f$\mathbb{Z}_q@f$
	    where @f$q@f$ is the order of the finite abelian group @f$G@f$ for
	    which the DDH assumption holds.
	
	    According to the results of Koshiba and Kurosawa [KK04] the length
	    of @f$r@f$ can be shorten to a reasonable value (e.g. 160 bit).
	    Under the additional DLSE assumption the necessary DDH problem in
	    @f$G@f$ seems to be still hard enough. Thus we can gain a great
	    performance advantage by using such short exponents. */
	mpz_t r;
	
	/** This constructor initalizes all necessary resources. */
	VTMF_CardSecret
		();
	
	/** A simple copy-constructor.
	    @param that is the secret to be copied. */
	VTMF_CardSecret
		(const VTMF_CardSecret& that);
	
	/** A simple assignment-operator.
	    @param that is the secret to be assigned. */
	VTMF_CardSecret& operator =
		(const VTMF_CardSecret& that);
	
	/** This function imports the secret.
	    @param s is correctly formated input string.
	    @returns True, if the import was successful. */
	bool import
		(std::string s);
	
	/** This destructor releases all occupied resources. */
	~VTMF_CardSecret
		();
};

/** @relates VTMF_CardSecret
    This operator prints a secret to an output stream.
    @param out is the output stream.
    @param cardsecret is the secret to be printed. */
std::ostream& operator<<
	(std::ostream &out, const VTMF_CardSecret &cardsecret);

#endif
