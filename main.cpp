
#include <iostream>
#include <functional>
#include <map>

#include "irc.hpp"

int main() {
    IRCClient c("tbot", "the_tbot", "irc.openredstone.org", 6667);
    c.connect();
    c.ident();
    c.nick();

    c.register_handler("376", [&c](IRCMessage m) { c.join("#openredstone"); });

    c.register_handler("PRIVMSG", [&c](IRCMessage m) {
        if (m.parameters().back().length() == 0) {
            return;
        }
        if (m.parameters().back().substr(0, 5) == "~test") {
            c.privmsg(m.nick(), "You did a test!");
        }
    });

    c.register_handler("KICK", [](IRCMessage) {
        exit(0);
    });

    c.run();
}

