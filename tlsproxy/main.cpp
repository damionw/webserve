#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>
       
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <sstream>

#define POLL_TIMEOUT 500

static struct option long_options[] =
{
    {"certfile",  required_argument, 0, 'c'},
    {"keyfile",  required_argument, 0, 'k'},
    {"command",  required_argument, 0, 'e'},
    {"help",    no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

char* short_options = "hc:k:e:";

class ProxyConnection{
    public:
        ProxyConnection(int socket, const std::string &command, const std::string &certificate, const std::string &privatekey);
        virtual ~ProxyConnection();

    public:
        virtual const char* ConnectionError(int error_code);

    public:
        virtual void handle_events();

    private:
        const std::string command;
        const std::string certificate;
        const std::string privatekey;
        const SSL_METHOD *method;
        int socket_fd;
        SSL_CTX *ctx;
        SSL *ssl;
        int pid;

    private:
        int child_stdin[2];
        int child_stdout[2];
};

ProxyConnection::ProxyConnection(int socket, const std::string &command, const std::string &certificate, const std::string &privatekey) :
    socket_fd(socket),
    method(SSLv23_server_method()),
    pid(-1),
    ctx(0),
    ssl(0),
    command(command),
    certificate(certificate),
    privatekey(privatekey)
{
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    if (! (ctx = SSL_CTX_new(method))) {
        ERR_print_errors_fp(stderr);
        throw std::fstream::failure("Unable to create SSL context");
    }

    if (SSL_CTX_use_certificate_file(ctx, certificate.c_str(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        throw std::fstream::failure("Unable to use certificate");
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, privatekey.c_str(), SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
        throw std::fstream::failure("Unable to use private keyfile");
    }

    if (! (ssl = SSL_new(ctx))) {
        ERR_print_errors_fp(stderr);
        throw std::fstream::failure("Can't allocate SSL connection object");
    }

    SSL_set_fd(ssl, socket_fd);

    int accept_result = SSL_accept(ssl);

    if (accept_result <= 0) {
        ERR_print_errors_fp(stderr);
        throw std::fstream::failure(ConnectionError(accept_result));
    }

    int _result1 = pipe2(child_stdin, O_NONBLOCK);
    int _result2 = pipe2(child_stdout, O_NONBLOCK);

    if (! (pid = fork())) {
        // We're in the child process
        dup2(child_stdin[0], STDIN_FILENO);
        dup2(child_stdout[1], STDOUT_FILENO);

        const char* shell = getenv("SHELL");

        if (shell) {
            execl(shell, shell, "-c", command.c_str(), (char *) NULL);
        }

        execl(
            "/usr/bin/env",
            "sh",
            "sh",
            "-c",
            command.c_str(),
            (char *) NULL
        );
    }
}

ProxyConnection::~ProxyConnection() {
    if (ssl) {
        SSL_free(ssl);
    }

    if (ctx) {
        SSL_CTX_free(ctx);
    }

    EVP_cleanup();
}

void ProxyConnection::handle_events() {
    struct pollfd fds[3];

    fds[0].events = POLLIN | POLLHUP;
    fds[0].fd = socket_fd;

    fds[1].events = POLLHUP | POLLOUT ;
    fds[1].fd = child_stdin[1]; // Writable end of the child's stdin

    fds[2].events = POLLIN | POLLHUP;
    fds[2].fd = child_stdout[0]; // Readable end of the child's stdout

    int flags = fcntl(socket_fd, F_GETFL, 0);

    if (flags < 0) {
        throw std::fstream::failure("Can't set socket to nonblocking");
    }

    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);

    while(1) {
        poll (fds, sizeof(fds), POLL_TIMEOUT);
    
        if ((fds[0].revents & POLLHUP) || (fds[1].revents & POLLHUP) || (fds[2].revents & POLLHUP)) {
            break;
        }

        // Forward data from the client to the child process
        if (fds[0].revents & POLLIN) {
            char buffer[1024];
            int bytes;

            if ((bytes = SSL_read(ssl, buffer, sizeof(buffer))) > 0) {
                bytes = write(fds[1].fd, buffer, bytes);
            }
        }

        // Forward data from the child process to the client
        if (fds[2].revents & POLLIN) {
            char buffer [1024];
            int bytes;

            while ((bytes = read(fds[2].fd, buffer, sizeof (buffer))) > 0) {
                SSL_write(ssl, buffer, bytes);
            }
        }

        int status;

        waitpid(pid, &status, WNOHANG);

        if (WIFEXITED(status)) {
            break;
        }
    }
}

const char* ProxyConnection::ConnectionError(int error_code) {
    switch (SSL_get_error(ssl, error_code)) {
        case SSL_ERROR_NONE:
            return "The TLS/SSL I/O operation completed. This result code is returned if and only if ret > 0.";
            break;

        case SSL_ERROR_ZERO_RETURN:
            return "The TLS/SSL connection has been closed. If the protocol version is SSL 3.0 or TLS 1.0, this result code is returned only if a closure alert has occurred in the protocol, i.e. if the connection has been closed cleanly. Note that in this case SSL_ERROR_ZERO_RETURN does not necessarily indicate that the underlying transport has been closed.";
            break;

        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            return "The operation did not complete; the same TLS/SSL I/O function should be called again later";
            break;

        case SSL_ERROR_WANT_CONNECT:
        case SSL_ERROR_WANT_ACCEPT:
            return "The operation did not complete; the same TLS/SSL I/O function should be called again later. The underlying BIO was not connected yet to the peer and the call would block in connect()/accept(). The SSL function should be called again when the connection is established. These messages can only appear with a BIO_s_connect() or BIO_s_accept() BIO , respectively. In order to find out, when the connection has been successfully established, on many platforms select() or poll() for writing on the socket file descriptor can be used.";
            break;

        case SSL_ERROR_WANT_X509_LOOKUP:
            return "The operation did not complete because an application callback set by SSL_CTX_set_client_cert_cb() has asked to be called again. The TLS/SSL I/O function should be called again later. Details depend on the application.";
            break;

        case SSL_ERROR_SYSCALL:
            return "Some I/O error occurred. The OpenSSL error queue may contain more information on the error. If the error queue is empty (i.e. ERR_get_error() returns 0), ret can be used to find out more about the error: If ret == 0, an EOF was observed that violates the protocol. If ret == -1, the underlying BIO reported an I/O error (for socket I/O on Unix systems, consult errno for details).";
            break;

        case SSL_ERROR_SSL:
            return "A failure in the SSL library occurred";
            break;
    }

    return "Operation was successful";
}

int main(int argc, char **argv) {
      /* getopt_long stores the option index here. */
    int option_index = 0;
    int c;
    std::string certificate;
    std::string privatekey;
    std::string command;

    while ((c = getopt_long (argc, argv, short_options, long_options, &option_index)) != -1) {
        switch (c) {
            case 'c':
                certificate = optarg;
                break;

            case 'k':
                privatekey = optarg;
                break;

            case 'e':
                command = optarg;
                break;

            case 'h':
                std::cerr << "Available options ";

                for (int index=0; long_options[index].name; ++index) {
                    struct option& _opt = long_options[index];
                    std::stringstream _s;

                    if (! _opt.flag) {
                        _s << "|-" << (char) _opt.val;
                    }

                    std::cerr
                        << (index ? " " : "") << "[--"
                        << _opt.name
                        << _s.str()
                        << "]";
                }

                std::cerr << std::endl;
                exit(0);
                break;

            default:
                abort();
        }
    }

    ProxyConnection connection(
        0,
        command,
        certificate,
        privatekey
    );

    connection.handle_events();
}
