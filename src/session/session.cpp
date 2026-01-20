#include "tiny_sql/session/session.h"
#include <sstream>

namespace tiny_sql {

Session::Session(uint32_t connection_id)
    : connection_id_(connection_id)
    , state_(SessionState::INIT)
    , username_("")
    , current_database_("")
    , sequence_id_(0)
{
    auth_plugin_data_.fill(0);
}

std::string Session::getSessionInfo() const {
    std::ostringstream oss;
    oss << "Session[id=" << connection_id_
        << ", user=" << (username_.empty() ? "<none>" : username_)
        << ", db=" << (current_database_.empty() ? "<none>" : current_database_)
        << ", state=";

    switch (state_) {
        case SessionState::INIT:
            oss << "INIT";
            break;
        case SessionState::HANDSHAKE_SENT:
            oss << "HANDSHAKE_SENT";
            break;
        case SessionState::AUTHENTICATING:
            oss << "AUTHENTICATING";
            break;
        case SessionState::AUTHENTICATED:
            oss << "AUTHENTICATED";
            break;
        case SessionState::COMMAND_PHASE:
            oss << "COMMAND_PHASE";
            break;
        case SessionState::CLOSING:
            oss << "CLOSING";
            break;
        case SessionState::CLOSED:
            oss << "CLOSED";
            break;
    }

    oss << "]";
    return oss.str();
}

} // namespace tiny_sql
