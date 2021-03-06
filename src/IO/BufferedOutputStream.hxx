/*
 * Copyright 2003-2016 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef MPD_BUFFERED_OUTPUT_STREAM_HXX
#define MPD_BUFFERED_OUTPUT_STREAM_HXX

#include "Compiler.h"
#include "Util/DynamicFifoBuffer.hxx"

#include <stddef.h>

class OutputStream;

/**
 * An #OutputStream wrapper that buffers its output to reduce the
 * number of OutputStream::Write() calls.
 */
class BufferedOutputStream {
	OutputStream &os;

	DynamicFifoBuffer<char> buffer;

public:
	BufferedOutputStream(OutputStream &_os)
		:os(_os), buffer(32768) {}

	void Write(const void *data, size_t size);
	void Write(const char *p);

	gcc_printf(2,3)
	void Format(const char *fmt, ...);

	/**
	 * Write buffer contents to the #OutputStream.
	 */
	void Flush();

private:
	bool AppendToBuffer(const void *data, size_t size) noexcept;
};

#endif
