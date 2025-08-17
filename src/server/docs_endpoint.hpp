#pragma once
#include <httplib.h>

namespace server {

// Register routes under /help to serve local documentation files
// from /opt/register_mvp/share/docs.
void register_docs_routes(httplib::Server& svr);

} // namespace server

