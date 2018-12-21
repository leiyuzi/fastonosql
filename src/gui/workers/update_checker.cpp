/*  Copyright (C) 2014-2018 FastoGT. All right reserved.

    This file is part of FastoNoSQL.

    FastoNoSQL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoNoSQL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoNoSQL.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "gui/workers/update_checker.h"

#include <string>

#include <common/net/socket_tcp.h>  // for ClientSocketTcp

#include "proxy/server_config.h"

namespace fastonosql {
namespace {
common::Error GetVersion(uint32_t* version) {
  CHECK(version);

#if defined(FASTONOSQL)
  common::net::ClientSocketTcp client(common::net::HostAndPort(FASTONOSQL_HOST, SERVER_REQUESTS_PORT));
#elif defined(FASTOREDIS)
  common::net::ClientSocketTcp client(common::net::HostAndPort(FASTOREDIS_HOST, SERVER_REQUESTS_PORT));
#else
#error please specify url and port of version information
#endif
  common::ErrnoError err = client.Connect();
  if (err) {
    return common::make_error_from_errno(err);
  }

  std::string get_version_request;
  common::Error request_gen_err = proxy::GenVersionRequest(&get_version_request);
  if (request_gen_err) {
    err = client.Close();
    DCHECK(!err) << "Close client error: " << err->GetDescription();
    return request_gen_err;
  }

  size_t nwrite = 0;
  err = client.Write(get_version_request.data(), get_version_request.size(), &nwrite);
  if (err) {
    common::ErrnoError lerr = client.Close();
    DCHECK(!lerr) << "Close client error: " << lerr->GetDescription();
    return common::make_error_from_errno(err);
  }

  common::char_buffer_t version_reply;
  err = client.ReadToBuffer(&version_reply, 256);
  if (err) {
    common::ErrnoError lerr = client.Close();
    DCHECK(!lerr) << "Close client error: " << lerr->GetDescription();
    return common::make_error_from_errno(err);
  }

  uint32_t version_result;
  common::Error parse_error = proxy::ParseVersionResponce(version_reply.as_string(), &version_result);
  if (parse_error) {
    err = client.Close();
    DCHECK(!err) << "Close client error: " << err->GetDescription();
    return parse_error;
  }

  err = client.Close();
  DCHECK(!err) << "Close client error: " << err->GetDescription();
  *version = version_result;
  return common::Error();
}
}  // namespace
namespace gui {

UpdateChecker::UpdateChecker(QObject* parent) : QObject(parent) {
  qRegisterMetaType<common::Error>("common::Error");
}

void UpdateChecker::routine() {
  uint32_t version;
  const common::Error err = GetVersion(&version);
  versionAvailibled(err, version);
}

}  // namespace gui
}  // namespace fastonosql
