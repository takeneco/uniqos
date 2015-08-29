/// @file   message_queue.cc
/// @brief  message_queue class implementation.

//  Uniqos  --  Unique Operating System
//  (C) 2013 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <core/message_queue.hh>


message_queue::message_queue()
{
}

message_queue::~message_queue()
{
}

/// @brief  Deliver a message.
/// @retval true   Message delivered.
/// @retval false  No message.
bool message_queue::deliv_np()
{
	message* msg = pop();

	if (msg) {
		msg->handler(msg);
		return true;
	}

	return false;
}

bool message_queue::deliv_all_np()
{
	if (!probe())
		return false;

	do {
		message* msg = pop();

		msg->handler(msg);

	} while (probe());

	return true;
}
