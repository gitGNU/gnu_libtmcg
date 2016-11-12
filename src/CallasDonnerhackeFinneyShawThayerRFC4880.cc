/*******************************************************************************
  CallasDonnerhackeFinneyShawThayerRFC4880.cc, OpenPGP Message Format

     J. Callas, L. Donnerhacke, H. Finney, D. Shaw, R. Thayer:
	'OpenPGP Message Format',
     Network Working Group, Request for Comments: 4880, November 2007. 

   This file is part of LibTMCG.

 Copyright (C) 2016  Heiko Stamer <HeikoStamer@gmx.net>

   LibTMCG is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   LibTMCG is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with LibTMCG; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
*******************************************************************************/

#include "CallasDonnerhackeFinneyShawThayerRFC4880.hh"

void CallasDonnerhackeFinneyShawThayerRFC4880::Radix64Encode
	(const OCTETS &in, std::string &out)
{
	size_t len = in.size();
	size_t i = 0, c = 1;

	// Each 6-bit group is used as an index into an array of 
	// 64 printable characters from the table below. The character
	// referenced by the index is placed in the output string.
	for(; len >= 3; len -= 3, i += 3)
	{
		BYTE l[4];
		l[0] = (in[i] & 0xFC) >> 2;
		l[1] = ((in[i] & 0x03) << 4) + ((in[i+1] & 0xF0) >> 4);
		l[2] = ((in[i+1] & 0x0F) << 2) + ((in[i+2] & 0xC0) >> 6);
		l[3] = in[i+2] & 0x3F;
		for (size_t j = 0; j < 4; j++, c++)
		{
			out += tRadix64[l[j]];
			// The encoded output stream must be represented
			// in lines of no more than 76 characters each.
			if (((c % TMCG_OPENPGP_RADIX64_MC) == 0) &&
			    ((len >= 4) || (j < 3)))
				out += "\r\n"; // add a line delimiter
		}
	}
	// Special processing is performed if fewer than 24 bits are
	// available at the end of the data being encoded. There are three
	// possibilities:
	// 1. The last data group has 24 bits (3 octets).
	//    No special processing is needed.
	// 2. The last data group has 16 bits (2 octets).
	//    The first two 6-bit groups are processed as above. The third
	//    (incomplete) data group has two zero-value bits added to it,
	//    and is processed as above. A pad character (=) is added to
	//    the output.
	// 3. The last data group has 8 bits (1 octet).
	//    The first 6-bit group is processed as above. The second
	//    (incomplete) data group has four zero-value bits added to it,
	//    and is processed as above. Two pad characters (=) are added
	//    to the output. 
	if (len == 2)
	{
		BYTE l[3];
		l[0] = (in[i] & 0xFC) >> 2;
		l[1] = ((in[i] & 0x03) << 4) + ((in[i+1] & 0xF0) >> 4);
		l[2] = ((in[i+1] & 0x0F) << 2);
		for (size_t j = 0; j < 3; j++, c++)
		{
			out += tRadix64[l[j]];
			// The encoded output stream must be represented
			// in lines of no more than 76 characters each.
			if ((c % TMCG_OPENPGP_RADIX64_MC) == 0)
				out += "\r\n"; // add a line delimiter
		}
		out += "=";
	}
	else if (len == 1)
	{
		BYTE l[2];
		l[0] = (in[i] & 0xFC) >> 2;
		l[1] = ((in[i] & 0x03) << 4);
		for (size_t j = 0; j < 2; j++, c++)
		{
			out += tRadix64[l[j]];
			// The encoded output stream must be represented
			// in lines of no more than 76 characters each.
			if ((c % TMCG_OPENPGP_RADIX64_MC) == 0)
				out += "\r\n"; // add a line delimiter
		}
		out += "=", c++;
		// The encoded output stream must be represented
		// in lines of no more than 76 characters each.
		if ((c % TMCG_OPENPGP_RADIX64_MC) == 0)
			out += "\r\n"; // add a line delimiter
		out += "=";
	}
    return;
}

void CallasDonnerhackeFinneyShawThayerRFC4880::Radix64Decode
	(std::string in, OCTETS &out)
{
	// remove whitespaces and delimiters
	in.erase(std::remove_if(in.begin(), in.end(), notRadix64()), in.end());

	size_t len = in.size();
	for (size_t j = 0; j < (4 - (len % 4)); j++)
		in += "="; // append pad until multiple of four

	for (size_t i = 0; i < len; i += 4)
	{
        	BYTE l[4];
		for (size_t j = 0; j < 4; j++)
			l[j] = fRadix64[(size_t)in[i+j]];
		BYTE t[3];
		t[0] = ((l[0] & 0x3F) << 2) + ((l[1] & 0x30) >> 4);
		t[1] = ((l[1] & 0x0F) << 4) + ((l[2] & 0x3C) >> 2);
		t[2] = ((l[2] & 0x03) << 6) + (l[3] & 0x3F);
		for (size_t j = 0; j < 3; j++)
			if (l[j+1] != 255) out.push_back(t[j]);
	}
    return;
}

void CallasDonnerhackeFinneyShawThayerRFC4880::CRC24Compute
	(const OCTETS &in, OCTETS &out)
{
	// The CRC is computed by using the generator 0x864CFB and an
	// initialization of 0xB704CE. The accumulation is done on the
	// data before it is converted to radix-64, rather than on the
	// converted data. A sample implementation of this algorithm is
	// in the next section. 
	uint32_t crc = TMCG_OPENPGP_CRC24_INIT;
	for (size_t len = 0; len < in.size(); len++)
	{
		crc ^= in[len] << 16;
		for (size_t i = 0; i < 8; i++)
		{
			crc <<= 1;
			if (crc & 0x1000000)
				crc ^= TMCG_OPENPGP_CRC24_POLY;
		}
	}
	crc &= 0xFFFFFF;
	out.push_back(crc >> 16);
	out.push_back(crc >> 8);
	out.push_back(crc);
	return;
}

void CallasDonnerhackeFinneyShawThayerRFC4880::CRC24Encode
	(const OCTETS &in, std::string &out)
{
	OCTETS crc;

	// The checksum is a 24-bit Cyclic Redundancy Check (CRC) converted
	// to four characters of radix-64 encoding by the same MIME base64
	// transformation, preceded by an equal sign (=).
	out += "=";
	CRC24Compute(in, crc);
	Radix64Encode(crc, out);
}

void CallasDonnerhackeFinneyShawThayerRFC4880::ArmorEncode
	(const OCTETS &in, std::string &out)
{
	// Concatenating the following data creates ASCII Armor:
	//  - An Armor Header Line, appropriate for the type of data
	//  - Armor Headers
	//  - A blank (zero-length, or containing only whitespace) line
	//  - The ASCII-Armored data
	//  - An Armor Checksum
	//  - The Armor Tail, which depends on the Armor Header Line

	// An Armor Header Line consists of the appropriate header line text
	// surrounded by five (5) dashes ('-', 0x2D) on either side of the
	// header line text. The header line text is chosen based upon the
	// type of data that is being encoded in Armor, and how it is being
	// encoded. Header line texts include the following strings:
	// [...]
	// BEGIN PGP PUBLIC KEY BLOCK
	//    Used for armoring public keys.
	// [...]
	// Note that all these Armor Header Lines are to consist of a complete
	// line. That is to say, there is always a line ending preceding the
	// starting five dashes, and following the ending five dashes. The
	// header lines, therefore, MUST start at the beginning of a line, and
	// MUST NOT have text other than whitespace following them on the same
	// line. These line endings are considered a part of the Armor Header
	// Line for the purposes of determining the content they delimit.
	out += "-----BEGIN PGP PUBLIC KEY BLOCK-----\r\n";

	// The Armor Headers are pairs of strings that can give the user or 
	// the receiving OpenPGP implementation some information about how to
	// decode or use the message. The Armor Headers are a part of the
	// armor, not a part of the message, and hence are not protected by
	// any signatures applied to the message.
	// The format of an Armor Header is that of a key-value pair. A colon
	// (':' 0x38) and a single space (0x20) separate the key and value.
	// [...]
	// Currently defined Armor Header Keys are as follows:
	//  - "Version", which states the OpenPGP implementation and version
	//    used to encode the message.
	out += "Version: LibTMCG "VERSION"\r\n";

	// Next, a blank (zero-length, or containing only whitespace) line
	out += "\r\n";

	// Next, the ASCII-Armored data
	Radix64Encode(in, out);
	out += "\r\n";

	// Next, an Armor Checksum
	CRC24Encode(in, out);
	out += "\r\n";

	// The Armor Tail Line is composed in the same manner as the Armor
	// Header Line, except the string "BEGIN" is replaced by the string
	// "END".
	out += "-----END PGP PUBLIC KEY BLOCK-----";
}

void CallasDonnerhackeFinneyShawThayerRFC4880::FingerprintCompute
	(const OCTETS &in, OCTETS &out)
{
	BYTE *buffer = new BYTE[in.size() + 3]; // additional 3 bytes needed
	BYTE *hash = new BYTE[20]; // fixed output size of SHA-1

	// A V4 fingerprint is the 160-bit SHA-1 hash of the octet 0x99,
	// followed by the two-octet packet length, followed by the entire
	// Public-Key packet starting with the version field.
	buffer[0] = 0x99;
	buffer[1] = in.size() >> 8;
	buffer[2] = in.size();
	for (size_t i = 0; i < in.size(); i++)
		buffer[i + 3] = in[i];
	gcry_md_hash_buffer(GCRY_MD_SHA1, hash, buffer, in.size() + 3); 
	for (size_t i = 0; i < 20; i++)
		out.push_back(hash[i]);
	delete [] buffer;
	delete [] hash;
}

void CallasDonnerhackeFinneyShawThayerRFC4880::KeyidCompute
	(const OCTETS &in, OCTETS &out)
{
	OCTETS fpr;

	// A Key ID is an eight-octet scalar that identifies a key.
	// Implementations SHOULD NOT assume that Key IDs are unique.
	// [...]
	// The Key ID is the low-order 64 bits of the fingerprint.
	FingerprintCompute(in, fpr);
	for (size_t i = 12; i < 20; i++)
		out.push_back(fpr[i]);
}

void CallasDonnerhackeFinneyShawThayerRFC4880::HashCompute
	(const OCTETS &in, OCTETS &out)
{
	size_t dlen = gcry_md_get_algo_dlen(GCRY_MD_SHA256); // FIXME
	BYTE *buffer = new BYTE[in.size()];
	BYTE *hash = new BYTE[dlen];

	for (size_t i = 0; i < in.size(); i++)
		buffer[i] = in[i];
	gcry_md_hash_buffer(GCRY_MD_SHA256, hash, buffer, in.size()); 
	for (size_t i = 0; i < dlen; i++)
		out.push_back(hash[i]);
	delete [] buffer;
	delete [] hash;
}

void CallasDonnerhackeFinneyShawThayerRFC4880::PacketTagEncode
	(size_t tag, OCTETS &out)
{
	// use V4 packet format
	out.push_back(tag | 0x80 | 0x40);
}

void CallasDonnerhackeFinneyShawThayerRFC4880::PacketLengthEncode
	(size_t len, OCTETS &out)
{
	// use scalar length format
	out.push_back(0xFF);
	out.push_back(len >> 24);
	out.push_back(len >> 16);
	out.push_back(len >> 8);
	out.push_back(len);
}

void CallasDonnerhackeFinneyShawThayerRFC4880::PacketTimeEncode
	(OCTETS &out)
{
	time_t current_time = time(NULL);

	// A time field is an unsigned four-octet number containing the number
	// of seconds elapsed since midnight, 1 January 1970 UTC.
	out.push_back(current_time >> 24);
	out.push_back(current_time >> 16);
	out.push_back(current_time >> 8);
	out.push_back(current_time);
}

void CallasDonnerhackeFinneyShawThayerRFC4880::PacketMPIEncode
	(gcry_mpi_t in, OCTETS &out)
{
	int ret;
	size_t bitlen = gcry_mpi_get_nbits(in);
	size_t buflen = ((bitlen + 7) / 8) + 2;
	BYTE *buffer = new BYTE[buflen];

	// Multiprecision integers (also called MPIs) are unsigned integers
	// used to hold large integers such as the ones used in cryptographic
	// calculations.
	// An MPI consists of two pieces: a two-octet scalar that is the length
	// of the MPI in bits followed by a string of octets that contain the
	// actual integer.
	// These octets form a big-endian number; a big-endian number can be
	// made into an MPI by prefixing it with the appropriate length.
	ret = gcry_mpi_print(GCRYMPI_FMT_PGP, buffer, buflen, &buflen, in);
	for (size_t i = 0; ((!ret) && (i < buflen)); i++)
		out.push_back(buffer[i]);
	delete [] buffer;
}

void CallasDonnerhackeFinneyShawThayerRFC4880::PacketUidEncode
	(std::string uid, OCTETS &out)
{
	// A User ID packet consists of UTF-8 text that is intended to 
	// represent the name and email address of the key holder. By
	// convention, it includes an RFC 2822 [RFC2822] mail name-addr,
	// but there are no restrictions on its content. The packet length
	// in the header specifies the length of the User ID.
	PacketTagEncode(13, out);
	PacketLengthEncode(uid.length(), out);
	for (size_t i = 0; i < uid.length(); i++)
		out.push_back(uid[i]);
}

void CallasDonnerhackeFinneyShawThayerRFC4880::PacketPubEncode
	(gcry_mpi_t p, gcry_mpi_t q, gcry_mpi_t g, gcry_mpi_t y, OCTETS &out)
{
	size_t plen = (gcry_mpi_get_nbits(p) + 7) / 8;
	size_t qlen = (gcry_mpi_get_nbits(q) + 7) / 8;
	size_t glen = (gcry_mpi_get_nbits(g) + 7) / 8;
	size_t ylen = (gcry_mpi_get_nbits(y) + 7) / 8;

	// A Public-Key packet starts a series of packets that forms an
	// OpenPGP key (sometimes called an OpenPGP certificate).
	// [...]
	// A version 4 packet contains:
	//  - A one-octet version number (4).
	//  - A four-octet number denoting the time that the key was created.
	//  - A one-octet number denoting the public-key algorithm of this key.
	//  - A series of multiprecision integers comprising the key material.
	//    This algorithm-specific portion is:
	//      Algorithm-Specific Fields for RSA public keys:
	//       - multiprecision integer (MPI) of RSA public modulus n;
	//       - MPI of RSA public encryption exponent e.
	//      Algorithm-Specific Fields for DSA public keys:
	//       - MPI of DSA prime p;
	//       - MPI of DSA group order q (q is a prime divisor of p-1);
	//       - MPI of DSA group generator g;
	//       - MPI of DSA public-key value y (= g**x mod p where x 
	//         is secret).
	//      Algorithm-Specific Fields for Elgamal public keys:
	//       - MPI of Elgamal prime p;
	//       - MPI of Elgamal group generator g;
	//       - MPI of Elgamal public key value y (= g**x mod p where x
	//         is secret).
	PacketTagEncode(6, out);
	PacketLengthEncode(1+4+1+2+plen+2+qlen+2+glen+2+ylen, out);
	out.push_back(4); // V4 format
	PacketTimeEncode(out); // current time
	out.push_back(17); // public-key algorithm: DSA
	PacketMPIEncode(p, out); // MPI p
	PacketMPIEncode(q, out); // MPI q
	PacketMPIEncode(g, out); // MPI g
	PacketMPIEncode(y, out); // MPI y
}

void CallasDonnerhackeFinneyShawThayerRFC4880::PacketSubEncode
	(gcry_mpi_t p, gcry_mpi_t g, gcry_mpi_t y, OCTETS &out)
{
	size_t plen = (gcry_mpi_get_nbits(p) + 7) / 8;
	size_t glen = (gcry_mpi_get_nbits(g) + 7) / 8;
	size_t ylen = (gcry_mpi_get_nbits(y) + 7) / 8;

	// A Public-Subkey packet (tag 14) has exactly the same format as a
	// Public-Key packet, but denotes a subkey. One or more subkeys may be
	// associated with a top-level key. By convention, the top-level key
	// provides signature services, and the subkeys provide encryption
	// services.
	// [...]
	//
	PacketTagEncode(14, out);
	PacketLengthEncode(1+4+1+2+plen+2+glen+2+ylen, out);
	out.push_back(4); // V4 format
	PacketTimeEncode(out); // current time
	out.push_back(16); // public-key algorithm: Elgamal
	PacketMPIEncode(p, out); // MPI p
	PacketMPIEncode(g, out); // MPI g
	PacketMPIEncode(y, out); // MPI y
}

void CallasDonnerhackeFinneyShawThayerRFC4880::PacketSigEncode
	(const OCTETS &hashing, const OCTETS &left, gcry_mpi_t r, gcry_mpi_t s,
	 OCTETS &out)
{
	size_t rlen = (gcry_mpi_get_nbits(r) + 7) / 8;
	size_t slen = (gcry_mpi_get_nbits(s) + 7) / 8;

	// A Signature packet describes a binding between some public key and
	// some data. The most common signatures are a signature of a file or a
	// block of text, and a signature that is a certification of a User ID.
	// Two versions of Signature packets are defined. Version 3 provides
	// basic signature information, while version 4 provides an expandable
	// format with subpackets that can specify more information about the
	// signature.
	PacketTagEncode(2, out);
	PacketLengthEncode(hashing.size()+2+0+left.size()+2+rlen+2+slen, out);
	// hashed area including subpackets
	out.insert(out.end(), hashing.begin(), hashing.end());
	// unhashed subpacket area
	out.push_back(0 >> 8); // length of unhashed subpacket data
	out.push_back(0);
	// signature data
	out.insert(out.end(), left.begin(), left.end()); // 16 bits of hash
	PacketMPIEncode(r, out); // signature - MPI r
	PacketMPIEncode(s, out); // signature - MPI s
}

void CallasDonnerhackeFinneyShawThayerRFC4880::SubpacketEncode
	(BYTE type, bool critical, const OCTETS &in, OCTETS &out)
{
	// A subpacket data set consists of zero or more Signature subpackets.
	// In Signature packets, the subpacket data set is preceded by a two-
	// octet scalar count of the length in octets of all the subpackets.
	// A pointer incremented by this number will skip over the subpacket
	// data set.
	// Each subpacket consists of a subpacket header and a body. The
	// header consists of:
	//  - the subpacket length (1, 2, or 5 octets),
	//  - the subpacket type (1 octet),
	// and is followed by the subpacket-specific data.
	// The length includes the type octet but not this length. Its format
	// is similar to the "new" format packet header lengths, but cannot
	// have Partial Body Lengths.
	PacketLengthEncode(in.size() + 1, out);
	if (critical)
		out.push_back(type | 0x80);
	else
		out.push_back(type);
	out.insert(out.end(), in.begin(), in.end());
}

void CallasDonnerhackeFinneyShawThayerRFC4880::PacketSigPrepare
	(BYTE sigtype, const OCTETS &flags, const OCTETS &keyid, OCTETS &out)
{
	size_t subpkts = 6;
	size_t subpktlen = (subpkts * 6) + 4 + flags.size() + keyid.size() + 3;
	out.push_back(4); // V4 format
	out.push_back(sigtype); // type (eg 0x13 UID cert., 0x18 subkey bind.)
	out.push_back(17); // public-key algorithm: DSA
	out.push_back(8); // hash algorithm: SHA256
	// hashed subpacket area
	out.push_back(subpktlen >> 8); // length of hashed subpacket data
	out.push_back(subpktlen);
		// signature creation time
		OCTETS sigtime;
		PacketTimeEncode(sigtime);
		SubpacketEncode(2, false, sigtime, out);
		// key flags
		SubpacketEncode(27, false, flags, out);
		// issuer
		SubpacketEncode(16, false, keyid, out);
		// preferred symmetric algorithms
		OCTETS psa;
		psa.push_back(9); // AES256
		SubpacketEncode(11, false, psa, out);
		// preferred hash algorithms
		OCTETS pha;
		pha.push_back(8); // SHA256
		SubpacketEncode(21, false, pha, out);
		// preferred compression algorithms
		OCTETS pca;
		pca.push_back(0); // uncompressed
		SubpacketEncode(22, false, pca, out);
}

gcry_error_t CallasDonnerhackeFinneyShawThayerRFC4880::CertificationHash
	(const OCTETS &primary, std::string uid, const OCTETS &trailer, 
	 gcry_mpi_t &h, OCTETS &left)
{
	OCTETS hash_input;
	BYTE *buffer = new BYTE[1024];
	size_t uidlen = uid.length(), nsc;
	gcry_error_t ret;

	// When a signature is made over a key, the hash data starts with the
	// octet 0x99, followed by a two-octet length of the key, and then body
	// of the key packet. (Note that this is an old-style packet header for
	// a key packet with two-octet length.)
	hash_input.push_back(0x99);
	hash_input.push_back(primary.size() >> 8);
	hash_input.push_back(primary.size());
	hash_input.insert(hash_input.end(), primary.begin(), primary.end());
	// A certification signature (type 0x10 through 0x13) hashes the User
	// ID being bound to the key into the hash context after the above
	// data. [...] A V4 certification hashes the constant 0xB4 for User
	// ID certifications or the constant 0xD1 for User Attribute
	// certifications, followed by a four-octet number giving the length
	// of the User ID or User Attribute data, and then the User ID or 
	// User Attribute data. 
	hash_input.push_back(0xB4);
	hash_input.push_back(uidlen >> 24);
	hash_input.push_back(uidlen >> 16);
	hash_input.push_back(uidlen >> 8);
	hash_input.push_back(uidlen);
	for (size_t i = 0; i < uidlen; i++)
		hash_input.push_back(uid[i]);
	// Once the data body is hashed, then a trailer is hashed. [...]
	// A V4 signature hashes the packet body starting from its first
	// field, the version number, through the end of the hashed subpacket
	// data. Thus, the fields hashed are the signature version, the
	// signature type, the public-key algorithm, the hash algorithm,
	// the hashed subpacket length, and the hashed subpacket body.
	hash_input.insert(hash_input.end(), trailer.begin(), trailer.end());
	// V4 signatures also hash in a final trailer of six octets: the
	// version of the Signature packet, i.e., 0x04; 0xFF; and a four-octet,
	// big-endian number that is the length of the hashed data from the
	// Signature packet (note that this number does not include these final
	// six octets).
	hash_input.push_back(0x04);
	PacketLengthEncode(trailer.size(), hash_input);
	// After all this has been hashed in a single hash context, the
	// resulting hash field is used in the signature algorithm and placed
	// at the end of the Signature packet.
	HashCompute(hash_input, left);
	for (size_t i = 0; ((i < left.size()) && (i < 1024)); i++)
		buffer[i] = left[i];
	ret = gcry_mpi_scan(&h, GCRYMPI_FMT_USG, buffer, left.size(), &nsc);
	while (left.size() > 2)
		left.pop_back();
	delete [] buffer;

	return ret;
}

gcry_error_t CallasDonnerhackeFinneyShawThayerRFC4880::SubkeyBindingHash
	(const OCTETS &primary, const OCTETS &subkey, const OCTETS &trailer,
	 gcry_mpi_t &h, OCTETS &left)
{
	OCTETS hash_input;
	BYTE *buffer = new BYTE[1024];
	size_t nsc;
	gcry_error_t ret;

	// When a signature is made over a key, the hash data starts with the
	// octet 0x99, followed by a two-octet length of the key, and then body
	// of the key packet. (Note that this is an old-style packet header for
	// a key packet with two-octet length.) A subkey binding signature
	// (type 0x18) or primary key binding signature (type 0x19) then hashes
	// the subkey using the same format as the main key (also using 0x99 as
	// the first octet).
	hash_input.push_back(0x99);
	hash_input.push_back(primary.size() >> 8);
	hash_input.push_back(primary.size());
	hash_input.insert(hash_input.end(), primary.begin(), primary.end());
	hash_input.push_back(0x99);
	hash_input.push_back(subkey.size() >> 8);
	hash_input.push_back(subkey.size());
	hash_input.insert(hash_input.end(), subkey.begin(), subkey.end());
	// Once the data body is hashed, then a trailer is hashed. [...]
	// A V4 signature hashes the packet body starting from its first
	// field, the version number, through the end of the hashed subpacket
	// data. Thus, the fields hashed are the signature version, the
	// signature type, the public-key algorithm, the hash algorithm,
	// the hashed subpacket length, and the hashed subpacket body.
	hash_input.insert(hash_input.end(), trailer.begin(), trailer.end());
	// V4 signatures also hash in a final trailer of six octets: the
	// version of the Signature packet, i.e., 0x04; 0xFF; and a four-octet,
	// big-endian number that is the length of the hashed data from the
	// Signature packet (note that this number does not include these final
	// six octets).
	hash_input.push_back(0x04);
	PacketLengthEncode(trailer.size(), hash_input);
	// After all this has been hashed in a single hash context, the
	// resulting hash field is used in the signature algorithm and placed
	// at the end of the Signature packet.
	HashCompute(hash_input, left);
	for (size_t i = 0; ((i < left.size()) && (i < 1024)); i++)
		buffer[i] = left[i];
	ret = gcry_mpi_scan(&h, GCRYMPI_FMT_USG, buffer, left.size(), &nsc);
	while (left.size() > 2)
		left.pop_back();
	delete [] buffer;

	return ret;
}
