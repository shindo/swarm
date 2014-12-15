.. _connection:

HTTP Connection
===============

.. contents:: Contents
    :local:


HTTP connection between server and client is represented by an instance of :cpp:class:`thevoid::connection\<SocketType\>` class.
This class implements public interface provided by :cpp:class:`thevoid::reply_stream` class.

Instance of :cpp:class:`thevoid::connection\<SocketType\>` class is created *before* accepting new connection from client.
Once the connection is accepted :cpp:class:`thevoid::connection\<SocketType\>` object starts
reading data from client and processing incoming request by user's handler.
After request processing is done existing connection with the client may be reused if HTTP keep-alive is allowed.

Pointer to the :cpp:class:`thevoid::connection\<SocketType\>` object is stored within user's handler
which makes possible for the handler to access reply stream by provided :cpp:class:`thevoid::reply_stream` interface.


HTTP Keep-Alive
---------------

TheVoid supports HTTP keep-alive connections.
To reuse existing connection client can pass ``Connection: Keep-Alive`` header within request.
In HTTP/1.1 connection is keep-alive by default.

Strictly speaking, server will *try* to reuse existing connection.
But server will close the connection if request parsing failed or no appropriate handler was found
to process the request.


Reply Stream API
----------------

.. cpp:class:: thevoid::reply_stream

    This class provides an interface for accessing reply stream.
    All methods of the class are **thread-safe**.

    .. cpp:type:: result_function

     Typedef to :cpp:class:`std::function\<void (const boost::system::error_code&)\>`.

     Represents type of callback function that may be passed to asynchronous methods.


    .. cpp:function:: reply_stream()

     Construct the class object.

     The object will be constructed *before* accepting new connection from client.


    .. cpp:function:: ~reply_stream()

     Destroys the class object.


    .. cpp:function:: void send_headers(http_response &&rep, const boost::asio::const_buffer &content, result_function &&callback)

     Sends headers with content to client.

     At finish passed callback is called with :cpp:class:`boost::system::error_code`.


    .. cpp:function:: void send_data(const boost::asio::const_buffer &buffer, result_function &&callback)

     Sends raw data from buffer to client.

     At finish passed callback is called with :cpp:class:`boost::system::error_code`.


    .. cpp:function:: void want_more()

     Tells event loop to read more data from socket.

     This method may be used for asynchronous processing of incoming data by chunks.

     After this call handler's method :cpp:func:`thevoid::base_request_stream::on_data` is called as soon as data arrives.


    .. cpp:function:: void close(const boost::system::error_code &err)

     Closes socket to client.

     If error is set connection to client is terminated.
     Otherwise if connection is Keep-Alive server will try to reuse it for the next request.


    .. cpp:function:: void send_error(http_response::status_type type)

     Sends HTTP reply with specific status.

     This method is a shortcut for:

     .. code-block:: cpp

      http_response response(type);
      send_headers(std::move(response), boost::asio::const_buffer(), std::bind(&reply_stream::close, this, std::placeholders::_1));


Connection API
--------------

.. cpp:class:: thevoid::connection\<SocketType\>

    This class is private implementation of :cpp:class:`thevoid::reply_stream` interface.

    .. cpp:type:: socket_type

     Type of the underlying socket.

     .. code-block:: cpp

      typedef SocketType socket_type;


    .. cpp:type:: endpoint_type

     The endpoint type.

     .. code-block:: cpp

      typedef socket_type::endpoint_type endpoint_type;


    .. cpp:type:: enum state

     Represents current state of the connection:

     - :cpp:member:`processing_request`

       Request is in :cpp:member:`processing_request` state when all request data is received
       and is given to handler.

     - :cpp:member:`read_headers`

       Request is in :cpp:member:`read_headers` state when request headers are not fully received
       yet.

     - :cpp:member:`read_data`

       Request is in :cpp:member:`read_data` state when request body is not fully received yet
       but request headers are already received.

     - :cpp:member:`request_processed`

       Request is in :cpp:member:`request_processed` state when request processing is finished
       and connection is about to be closed but request data was not fully received yet.

       In this case TheVoid tries to received the remaining data to reuse existing connection.

       .. note::
        TheVoid reuses connection in this case whether or not this connection is meant to be keep-alive.

     - :cpp:member:`waiting_for_first_data`

       Request is in :cpp:member:`waiting_for_first_data` state when request headers receiving
       is about to start.


    .. cpp:function:: explicit connection(base_server *server, boost::asio::io_service &service, size_t buffer_size)

     Constructs the class object with the given :cpp:class:`boost::asio::io_service` instance.


    .. cpp:function:: ~connection()

     Destroys the class object.


    .. cpp:function:: socket_type &socket()

     Returns the socket associated with the connection.


    .. cpp:function:: endpoint_type &endpoint()

     Returns the endpoint associated with the connection.


    .. cpp:function:: void start(const std::string &local_endpoint)

     Starts the first asynchronous operation for the connection.

     This method calls :cpp:func:`async_read`.


    .. cpp:function:: std::shared_ptr\<thevoid::base_request_stream\> try_handler()

     Returns constructed handler, and empty :cpp:class:`std::shared_ptr\<thevoid::base_request_stream\>`
     if there's no handler associated with the connection or the handler closed the connection
     by calling :cpp:func:`thevoid::reply_stream::close`.


    .. cpp:function:: void close_impl(const boost::system::error_code &err)

     Internal implementation of :cpp:func:`thevoid::reply_stream::close` method.

     This method is not thread-safe and is invoked from within :cpp:class:`boost::asio::io_service` event loop.

     .. note::
      This method is called multiple times. Is it valid?


    .. cpp:function:: void send_impl(buffer_info &&info)

     Sends data represented by :cpp:class:`buffer_info` to the client.

     This method is thread-safe, but under lock this method adds buffer to sending queue
     and calls not thread-safe method :cpp:func:`send_nolock` if sending loop is not in action.


    .. cpp:function:: void send_nolock()

     Internal implementation of sending data to the client.

     This method is not thread-safe and is invoked under lock only if sending loop is not in action.

     This method collects some data from sending queue of buffers and sends them to the client.

     To send data to the client :cpp:func:`socket_type::async_write_some` asynchronous method
     is called with :cpp:func:`write_finished` callback.
     If there is still some data in sending queue passed callback method will call :cpp:func:`send_nolock`
     method, otherwise sending loop will be terminated.


    .. cpp:function:: void write_finished(const boost::system::error_code &err, size_t bytes_written)

     Callback that is passed to :cpp:func:`socket_type::async_write_some`.
     This method is called when sending some of accumulated outcoming data to the client is finished.

     If complete :cpp:class:`buffer_info` is sent corresponding callback is called from within this method.

     If no error happend and there's more data to be sent, :cpp:func:`send_nolock`
     method is called by the end of this call,
     otherwise sending loop is terminated.


    .. cpp:function:: void want_more_impl()

     Internal implementation of :cpp:func:`thevoid::reply_stream::want_more` method.

     This method is not thread-safe and is invoked from within :cpp:class:`boost::asio::io_service` event loop.

     If the connection's buffer contains unprocessed data :cpp:func:`process_data` is called,
     otherwise :cpp:func:`async_read` method is called to receive more data from the client.


    .. cpp:function:: void process_data(const char *begin, const char *end)

     Processes accumulated request data.

     If the connection is in :cpp:member:`read_headers` state and accumulated data fills up
     request headers then appropriate request handler will be created, if any,
     and :cpp:func:`thevoid::base_request_stream::on_headers` method of the handler will be called.
     If more data is needed to fill up request headers :cpp:func:`async_read` method will be called.

     If the connection is in :cpp:member:`read_data` state :cpp:func:thevoid::base_request_stream::on_data`
     method of the handler will be called with accumulated data.
     If the request handler processes all passed data and there will be more data from the client of the request
     :cpp:func:`async_read` method will be called.
     Otherwise receiving data from the client will be stopped until :cpp:func:`thevoid::reply_stream::want_more`
     method is called.

     Special case is when handler stops request processing before all request data is passed to the handler.
     In such case the connection is in :cpp:member:`request_processed` state and
     may receive remaining data to reuse existing connection with the client for subsequent requests processing
     by calling :cpp:func:`process_next` method as soon as all remaining data is received.


    .. cpp:function:: void async_read()

     Asynchronously reads data from the client.

     To receive data from the client :cpp:func:`socket_type::async_read_some` asynchronous method
     is called with :cpp:func:`handle_read` callback.


    .. cpp:function:: void handle_read(const boost::system::error_code &err, std::size_t bytes_transferred)

     Handles completion of a read operation.

     If error is happend during receiving data from the client, handler's
     :cpp:func:`thevoid::base_request_stream::on_close` method is called with the error and
     connection is terminated.

     Otherwise received data is passed to :cpp:func:`process_data` method.


    .. cpp:function:: void process_next()

     Processes next request by reusing existing connection.

     This method clears state of the connection and calls :cpp:func:`async_read` method
     to receive


    .. cpp:function:: void print_access_log()

     Prints log about finished request processing with similar to ``access.log`` format.
