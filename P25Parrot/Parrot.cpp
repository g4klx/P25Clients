/*
*   Copyright (C) 2016,2025 by Jonathan Naylor G4KLX
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "Parrot.h"

#include <cstdio>
#include <cassert>
#include <cstring>

CParrot::CParrot(unsigned int timeout) :
m_data(nullptr),
m_length(timeout * 1000U + 1000U),
m_used(0U),
m_ptr(0U)
{
	assert(timeout > 0U);

	m_data = new unsigned char[m_length];
}

CParrot::~CParrot()
{
	delete[] m_data;
}

bool CParrot::write(const unsigned char* data, unsigned int length)
{
	assert(data != nullptr);

	if ((m_length - m_used) < (length + 2U))
		return false;

	m_data[m_used] = length;
	::memcpy(m_data + m_used + 1U, data, length);
	m_used += length + 1U;

	return true;
}

void CParrot::end()
{
	m_ptr = 0U;
}

void CParrot::clear()
{
	m_used = 0U;
	m_ptr = 0U;
}

unsigned int CParrot::read(unsigned char* data)
{
	assert(data != nullptr);

	if (m_used == 0U)
		return 0U;

	unsigned int length = m_data[m_ptr];
	::memcpy(data, m_data + m_ptr + 1U, length);
	m_ptr += length + 1U;

	if (m_ptr >= m_used)
		m_used = 0U;

	return length;
}
