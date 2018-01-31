
#include <functional>
#include <map>
#include <string>
#include <vector>

class IRCMessage {

public:
    IRCMessage(std::string raw_message);

    std::string command() const;
    std::string nick() const;
    std::vector<std::string> parameters() const;

    void debug_print();

private:
    std::string raw_message;

};

class IRCClient {

public:
    IRCClient(std::string nickname, std::string username, std::string server, int port);
    ~IRCClient();

    void connect();
//    bool is_connected();
    void ident();
    void nick();
    void nick(std::string nickname);
    void join(std::string channel);
    void privmsg(std::string target, std::string message);
    void register_handler(std::string command, std::function<void(IRCMessage)> handler);
    void run();

private:
    int socket_fd;
    std::string nickname;
    std::string username;
    std::string server_hostname;
    int port;
    std::vector<std::string> channel_names;
    std::map<std::string, std::function<void(IRCMessage)>> handlers;

    void pong(IRCMessage message);
    void send_raw(std::string message);
};

