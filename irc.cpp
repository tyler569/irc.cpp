
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <iostream>

#include "irc.hpp"

/* Has to be >= 512 per IRC spec */
#define RECV_BUF_LEN 1024

IRCMessage::IRCMessage(std::string raw_message) : raw_message(raw_message) {}

std::string IRCMessage::command() const {
    if (this->raw_message[0] == ':') {
        int cmd_start = this->raw_message.find(' ') + 1;
        int cmd_end = this->raw_message.find(' ', cmd_start);
        if (cmd_end == std::string::npos) {
            return this->raw_message.substr(cmd_start);
        } else {
            return this->raw_message.substr(cmd_start, cmd_end - cmd_start);
        }
    } else {
        int cmd_end = this->raw_message.find(' ');
        return this->raw_message.substr(0, cmd_end);
    }
}

std::string IRCMessage::nick() const {
    if (this->raw_message[0] == ':') {
        int nick_end = this->raw_message.find('!');
        return this->raw_message.substr(1, nick_end - 1);
    } else {
        return "";
    }
}

std::vector<std::string> IRCMessage::parameters() const {
    std::string trail;
    std::string current;

    std::vector<std::string> v;

    int cmd_start, param_start, param_end;

    if (this->raw_message[0] == ':') {
        cmd_start = this->raw_message.find(' ') + 1;
        param_start = this->raw_message.find(' ', cmd_start) + 1;
    } else {
        param_start = this->raw_message.find(' ') + 1;
    }

    int trail_start = this->raw_message.find(" :");
    if (trail_start != std::string::npos) {
        trail = this->raw_message.substr(trail_start + 2);
        trail = trail.substr(0, trail.size() - 2);
    }

    while ((param_end = this->raw_message.find(' ', param_start)) != std::string::npos) {
        current = this->raw_message.substr(param_start, param_end - param_start);
        if (current[0] == ':') {
            break;
        }
        v.push_back(current);
        param_start = param_end + 1;
    }
    v.push_back(trail);

    return v;
}

void IRCMessage::debug_print() {
    std::cout << "raw    : " << this->raw_message;
    std::cout << "command: " << command() << std::endl;
    std::cout << "nick   : " << nick() << std::endl;
    std::cout << "params : " << std::endl;
    for (auto x : parameters()) {
        std::cout << "-   " << x << std::endl;
    }
}

IRCClient::IRCClient(std::string nickname, std::string username, std::string server, int port):
        nickname(nickname), username(username), server_hostname(server), port(port) {
    channel_names = {};
    handlers = {};
}

void IRCClient::ident() {
    send_raw("USER tbot 0 * :" + this->username);
}

void IRCClient::nick() {
    send_raw("NICK " + this->nickname);
}

void IRCClient::nick(std::string nickname) {
    this->nickname = nickname;
    send_raw("NICK " + this->nickname);
}

void IRCClient::join(std::string channel) {
    send_raw("JOIN " + channel);
    this->channel_names.push_back(channel);
}

void IRCClient::privmsg(std::string target, std::string message) {
    send_raw("PRIVMSG " + target + " :" + message);
}

void IRCClient::register_handler(std::string command, std::function<void(IRCMessage)> handler) {
    this->handlers.emplace(command, handler);
}

void IRCClient::connect() {
    struct hostent *host;
    sockaddr_in remote_addr;

    if ((host = gethostbyname(this->server_hostname.c_str())) == NULL) {
        perror("gethostbyname()");
        exit(1);
    }

    if ((this->socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("this->socket_fdet()");
        exit(1);
    }

    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(this->port);
    remote_addr.sin_addr = *((struct in_addr *)host->h_addr);

    memset(&(remote_addr.sin_zero), 0, 8);

    if (::connect(this->socket_fd, (sockaddr *)&remote_addr, sizeof(struct sockaddr)) == -1) {
        perror("connect()");
        exit(1);
    }
}

IRCClient::~IRCClient() {
    close(this->socket_fd);
}

void IRCClient::run() {
    int numbytes;
    char *message;
    int buf_index = 0;
    int buf_remain;
    char buf[RECV_BUF_LEN + 1]; /* +1 just in case, I think its possible to overrun */
    char save;

    while (1) {
        buf_remain = RECV_BUF_LEN - buf_index;

        if ((numbytes = recv(this->socket_fd, buf + buf_index, buf_remain, 0)) == -1) {
            perror("recv()");
            exit(1);
        }
        buf_index += numbytes;

        while ((message = strchr(buf, '\n')) != NULL) {
            /* OVERLOADING numbytes */
            numbytes = message - buf + 1;

            save = buf[numbytes];
            buf[numbytes] = '\0';

            std::cout << ">> " << buf;

            std::string irc_message_raw = buf;
            IRCMessage irc_message(irc_message_raw);

            // irc_message.debug_print();

            if (irc_message.command() == "PING") {
                this->pong(irc_message);
            }

            auto func = this->handlers.find(irc_message.command());
            if (func != this->handlers.end()) {
                func->second(irc_message);
            }

            buf[numbytes] = save;
            memmove(buf, buf + numbytes, RECV_BUF_LEN - numbytes);
            buf_index -= numbytes;

            memset(buf + buf_index, 0, RECV_BUF_LEN - buf_index);
        }
    }
}

void IRCClient::pong(IRCMessage m) {
    send_raw("PONG :" + m.parameters().back());
}

void IRCClient::send_raw(std::string message) {

    std::cout << "<< " << message << std::endl;

    send(this->socket_fd, message.c_str(), message.length(), 0);
    send(this->socket_fd, "\r\n", 2, 0);
}

