.. _server:

HTTP Server
+++++++++++

.. contents:: Contents
    :local:


Architecture
============

HTTP server is responsible for

- listening on specified endpoints to accept new connections

- processing incoming requests by provided handlers

- handling monitoring requests through special monitor port

:cpp:class:`thevoid::acceptors_list\<Connection\>` class provides an ability to accept
new connections of same type (e.g., TCP or Unix).
Each acceptor from the list listens on its own endpoint and dispatches new connections to worker threads.
All acceptors (except for monitor acceptor) of the server run within single thread
(using single :cpp:class:`boost::asio::io_service` object).

Each new connection is represented by distinct :cpp:class:`thevoid::connection\<SocketType\>` object
that is a protocol-logic wrapper around :cpp:class:`boost::asio::basic_stream_socket\<Protocol\>`.

:cpp:class:`thevoid::connection\<SocketType\>` object lives at least as long as the connection with the client is alive
and is responsible for receiving request data from the client, creating appropriate user's handler
to process request and passing incoming request data to the handler.
Once request's processing is finished the connection object frees the handler.
The server doesn't store an instance of :cpp:class:`thevoid::connection\<SocketType\>` explicitly,
but smart pointers to the connection object are stored within callbacks and user's handler.

Each connection is processed by exactly one :cpp:class:`boost::asio::io_service` instance
within one thread from worker threads pool (its size is configurable).
Thus, user's handler is called from within single thread, which makes possible not to think about synchronization
mechanisms.

Server provides monitoring information about itself through special monitor port.
Separate :cpp:class:`thevoid::acceptors_list\<Connection\>` instance running within its own thread
and processed by separate :cpp:class:`boost::asio::io_service` instance created for this purpose
to provide user with server's statistics with minimal latency.

Configuration
====================

Here is an example of server's configuration:

.. code-block:: javascript

    {
        "endpoints": [
            "0.0.0.0:8080",
            "unix:test.sock"
        ],
        "daemon": {
            "fork": true,
            "uid": 1000
        },
        "monitor-port": 20000,
        "buffer_size": 65536,
        "backlog": 128,
        "threads": 8,
        "logger": {
            "level": "debug",
            "frontends": [
                {
                    "formatter": {
                        "type": "string",
                        "pattern": "%(timestamp)s %(lwp)s/%(pid)s %(severity)s: %(message)s, %(...L)s"
                    },
                    "sink": {
                        "type": "files",
                        "path": "/var/log/thevoid.log",
                        "autoflush": true,
                        "rotation": {
                            "move": 0
                        }
                    }
                }
            ]
        },
        "application": {
            "remotes": ["localhost:1025:2"],
            "groups": [1, 2, 3]
        }
    }


Here:

- .. describe:: endpoints

      List of listening TCP and Unix sockets.

      .. code-block:: javascript

        "endpoints": [
            "0.0.0.0:8080",
            "unix:test.sock"
        ]

- .. describe:: daemon

      TheVoid supports daemonization, to fork entire process pass :code:`true` to :code:`"fork"` option.

      Setting :code:`"uid"` is possible for setuid call to drop root privileges.

      .. code-block:: javascript

        "daemon": {
            "fork": true,
            "uid": 1000
        }


      .. note::

        Rewrite this!

- .. describe:: monitor-port

      TheVoid can provide statistics about the server via monitor port.

      .. code-block:: javascript

        "monitor-port": 20000

- .. describe:: buffer_size

      Sets per-socket buffer size used for data receiving.

      Default value is 8192 (8 KB).

      .. code-block:: javascript

        "buffer_size": 65536

- .. describe:: backlog

      Backlog is a property of listening sockets, it limits size of queue with not yet processed incoming connections.

      .. code-block:: javascript

        "backlog": 128

- .. describe:: threads

      Threads property determines the number of working threads.

      .. code-block:: javascript

        "threads": 8

- .. describe:: logger

      Logger section configures server's logger. TheVoid uses Blackhole for logging.

      See Blackhole's documentation for more information.

- .. describe:: application

      This part of the configuration is application-specific.
      Current example shows possible configuration for `Elliptics <http://reverbrain.com/elliptics/>`_
      HTTP Proxy.

      .. code-block:: javascript

        "application": {
            "remotes": ["localhost:1025:2"],
            "groups": [1, 2, 3]
        }


Monitoring
==========

Monitoring information is accessible through monitor port specified in configuration.

Currently monitoring port accepts the following two commands:

- **h** - returns list of supported commands

- **i** - returns statistics about the server

Example of request of server's statistics:

.. code-block:: none

 echo i | netcat example.org 20000

with the following response:

.. code-block:: javascript

 {
     "connections": 132,
     "active-connections": 78
 }

Here:

- **connections** -- the number currently opened HTTP connections

- **active-connections** -- the number of connections that process some requests at the moment

So there are 132 opened HTTP connections between clients and application,
and only 78 of them are processing some requests right now.


Server API
===============

.. cpp:class:: thevoid::base_server

    This class provides interface with basic HTTP server's functionality,
    part of which custom HTTP server should implement:

    .. cpp:function:: base_server()

       Constructs the server.


    .. cpp:function:: ~base_server()

       Destroys the server.


    .. cpp:function:: void listen(const std::string &host)

       Listens for new connections at host.

       There are two possible formats for host:

       - host:port or address:port

         listen at certain interface and port.

       - unix:path

         listen unix socket at path.
     
       Example:

       .. code-block:: cpp

          server.listen("127.0.0.1:80");
          server.listen("unix:/var/run/server.sock");


    .. cpp:function:: int parse_arguments(int argc, char **argv)

       Parses command-line arguments and doesn't start the server.


    .. cpp:function:: int run()

       Runs the server.

       .. note::

          :cpp:func:`parse_arguments` have to be called before calling this method.


    .. cpp:function:: void stop()

       Stops the server.


    .. cpp:function:: int run(int argc, char **argv)

       Runs the server using provided command line arguments.

       This method is equal to iterative calls of :cpp:func:`parse_arguments` and :cpp:func:`run`.

       Supported arguments are:

       .. option:: --help

          Show help message and return.

       .. option:: --config <path>

          Read configuration file at :option:`path`.

       .. option:: --pidfile <file>

          Write PID-file with specified name :option:`file`.


    .. cpp:function:: const swarm::logger& logger() const

       Returns logger of the server.


    .. cpp:function:: std::map<std::string, std::string> get_statistics() const

       Returns server-specific statistics as a key-value map.

       Implement this if you want your own statistics available.


    .. cpp:function:: unsigned int threads_count() const

       Returns the number of worker threads.

       Worker threads pool processes incoming requests with the only restriction that
       each request is processed within single thread of the pool.


    .. cpp:function:: bool initialize(const rapidjson::Value &config)

       Initializes the server by application-specific section from configuration file.

       Returns true if initialization was successful, and false otherwise.


    .. cpp:function:: bool initialize_logger(const rapidjson::Value &config)

       Initializes logger by config.

       Returns true if initialization was successful, and false otherwise.

       Override this method if you want to initialize your own logger.


    .. cpp:class:: options

       This class provides API for setting handler options.

       It makes possible to specify conditions at which handler should be called.


    .. cpp:function:: void on(options &&opts, const std::shared_ptr<base_stream_factory> &factory)

       Registers handler producable by a factory with options that specify
       when such handler should be called to process request.


Acceptor API
===============

TheVoid doesn't provide interface for managing single connection acceptor.
Instead of this :cpp:class:`thevoid::acceptors_list\<Connection\>` class is provided,
that gathers together multiple acceptors.

.. cpp:class:: thevoid::acceptors_list\<Connection\>

    .. cpp:type:: socket_type

     Type of the underlying socket.

     .. code-block:: cpp

      typedef typename Connection::socket_type socket_type;


    .. cpp:type:: protocol_type

     Type of socket's protocol.

     .. code-block:: cpp

      typedef typename socket_type::protocol_type protocol_type;


    .. cpp:type:: endpoint_type

     Type of the endpoint.

     .. code-block:: cpp

      typedef typename protocol_type::endpoint endpoint_type;


    .. cpp:type:: acceptor_type

     Type of the acceptor.

     .. code-block:: cpp

      typedef boost::asio::basic_socket_acceptor<protocol_type> acceptor_type;


    .. cpp:type:: connection_type

     Type of the connection.

     .. code-block:: cpp

      typedef Connection connection_type;


    .. cpp:type:: connection_ptr_type

     Type of the smart pointer to the connection.

     .. code-block:: cpp

      typedef std::shared_ptr<connection_type> connection_ptr_type;


    .. cpp:function:: acceptors_list(server_data &data)

     Constructs the class object.

     Passed server's data connects :cpp:class:`thevoid::acceptors_list\<Connection\>` instanse
     with the server.


    .. cpp:function:: ~acceptors_list()

     Destroys the class object.


    .. cpp:function:: void add_acceptor(const std::string &address)

     Adds acceptor to listen on specified address.

     Throws exception if operation fails, otherwise constructed acceptor starts
     listening by calling :cpp:func:`start_acceptor` method.


    .. cpp:function:: void start_acceptor(size_t index)

     Starts acceptor from the list at specified index.

     This method creates :cpp:type:`connection_type` instance
     and starts acceptor of underlying type :cpp:type:`acceptor_type` by calling
     asynchronous method :cpp:func:`acceptor_type::async_accept` with
     :cpp:func:`handle_accept` method passed as callback.

     Smart pointer to the created :cpp:type:`connection_type` instance of type
     :cpp:type:`connection_ptr_type` is stored within passed callback.
     Thus, :cpp:type:`connection_type` object will be kept alive up to actual connection
     is established.


    .. cpp:function:: void handle_accept(size_t index, connection_ptr_type conn, const boost::system::error_code &err)

     Handles actual connection with the client performed by acceptor from the list at specifed index.

     Passed :cpp:type:`connection_ptr_type` smart pointer represents :cpp:type:`connection_type` object
     that will process incoming requests from the client through the established connection.


    .. cpp:function:: boost::asio::io_service &get_acceptor_service()

     Returns :cpp:class:`boost::asio::io_service` instance that handles accepting connections from clients.


    .. cpp:function:: boost::asio::io_service &get_connection_service()

     Returns :cpp:class:`boost::asio::io_service` instance that handles processing of incoming requests
     from clients.


    .. cpp:function:: endpoint_type create_endpoint(acceptor_type &acc, const std::string &host)

     Creates local endpoint to which acceptor will be bound.


    .. cpp:member:: std::vector<std::unique_ptr<acceptor_type>> acceptors

     Stores instances of the underlying :cpp:type:`acceptor_type` type of acceptors from the list.

     .. note::
      Elements from :cpp:member:`acceptors`, :cpp:member:`protocols` and :cpp:member:`local_endpoints`
      at the same position represent single acceptor from the list.


    .. cpp:member:: std::vector<protocol_type> protocols

     Stores protocols of acceptors from the list.


    .. cpp:member:: std::vector<std::string> local_endpoints

     Stores local endpoints of acceptors from the list.

