/*
 * opencog/server/ServerSocket.h
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * All Rights Reserved
 *
 * Written by Andre Senna <senna@vettalabs.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _OPENCOG_SERVER_SOCKET_H
#define _OPENCOG_SERVER_SOCKET_H

#include <string>
#include <sstream>
#include <tr1/memory>

#include <Sockets/TcpSocket.h>
#include <Sockets/ISocketHandler.h>

#include <opencog/server/IHasMimeType.h>
#include <opencog/server/IRPCSocket.h>

namespace opencog
{

class Request;

/**
 * This class implements the server socket that handles the primary
 * interface of the cogserver: the plain text command line. It is
 * implemented using the callback-based API of the TcpSocket class
 * (from Alhem's Sockets library).
 *
 * The ConsoleSocket uses the 'Detach()' method and the 'OnDetached()'
 * callback to support multiple simultaneous clients. This is done by
 * creating a separate thread and dispatching the client socket for
 * each client that connects to the server socket.

 * We enhance the default callback API with another callback method,
 * provided by the IRPCSocket interface: 'OnCompleted()', This callback
 * signals the server socket that request processing has finished (so
 * that the we may synchronously send the command prompt to the client
 * but process the request 'asynchronously' on the cogserver's main
 * thread).
 *
 * As required by the Request abstract class, the ConsoleSocket class
 * also implements the IHasMimeType interface, defining its mime-type
 * as 'text/plain'.
 *
 */
class ConsoleSocket : public TcpSocket,
                      public IHasMimeType,
                      public IRPCSocket
{

private:

    Request* _request;
    std::stringstream _buffer;
    std::string cmdName;
    bool multiline_mode;

public:

    /** ConsoleSocket's constructor. Defines the socket's mime-type as
     *  'text/plain' and the enables the TcpSocket's 'line protocol'
     *  (see Alhem's Socket library docs for more info).
     */
    ~ConsoleSocket();

    /** ConsoleSocket's destructor. */
    ConsoleSocket(ISocketHandler &handler);

    /** Accept callback: called whenever a new connection arrives. It
     *  basically call the 'Detach()' method to dispatch the client socket
     *  to a (new) separate thread.
     */
    void OnAccept          (void);

    /** OnDetached callback: called from the client ConsoleSocket
     *  instance, right after the new thread is setup and ready. We
     *  just send the command prompt string (configuration paramter
     *  "PROMPT") to the client.
     */
    void OnDetached        (void);

    /** OnLine callback: called when a new command/request is revieved
     *  from the client. It parses the command line by spliting it into
     *  space-separated tokens.
     *
     *  The first token is used as the requests id/name and the
     *  remaining tokens are used as the request's parameters. The
     *  request name is used to retrieve the request class from the
     *  cogserver.
     *
     *  If the class is not found, we execute the 'HelpRequest' which
     *  will return a useful message to the client.
     *
     *  If the request class is found, we instantiate a new request,
     *  set its parameters and push it to the cogserver's request queue
     *  (*unless* the request instance has disabled the line protocol;
     *  see the OnRawData() method documentation).
     */
    void OnLine            (const std::string& line);

    /** Some requests may require input that spans multiple lines. To
     *  handle these cases, the request should disable the socket's
     *  line protocol when the method 'Request.setSocket' is called.
     *
     *  If the line protocol is disabled, this callback will be called
     *  whenever the client sends data to server. The current
     *  implementation pushes the input to a buffer until a special
     *  character sequence is sent: "^D\r\n" (or "^D\n"). Then, the
     *  server adds the input buffer to the request's parameter list
     *  and pushes the request to the cogserver's request queue.
     *  Parsing the contents of input buffer is naturally, up to
     *  the request itself.
     */
    void OnRawData         (const char * buf, size_t len);

    /** OnRequestComplete: called when a request has finished. It
     *  just sends another command prompt (configuration parameter
     *  "PROMPT") to the client.
     */
    void OnRequestComplete ();

    /** SetMultilineMode: Set multiline mode. This determines the kind
     *  of line processing offered by the console. When multiline mode
     *  is enabled, then command-line parsing is disabled, since it is
     *  assummed that multiple lines of data will follow (and should not
     *  be parsed).  When multiline mode is disabled (the default), 
     *  ordinary command-line parsing is done.
     */
    void SetMultilineMode(bool);

}; // class

}  // namespace

#endif // _OPENCOG_SERVER_SOCKET_H
