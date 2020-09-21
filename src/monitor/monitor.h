/*
	CEG is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	CEG is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with CEG.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MONITOR_H_
#define MONITOR_H_

#include <proto/cpp/monitor.pb.h>
#include <proto/cpp/overlay.pb.h>
#include <common/network.h>

namespace CEG {
	typedef std::shared_ptr<protocol::WsMessage> WsMessagePointer;

	class Monitor : public CEG::Connection {
	private:

		bool state_changed_;              /* state changed */
		int64_t active_time_;             /* the active time of monitor */
		std::string session_id_;          /* session id */
		std::string peer_node_address_;   /* peer node address */
		
		std::string CEG_version_;        /* CEG version */
		int64_t monitor_version_;         /* monitor version */
		int64_t CEG_ledger_version_;     /* CEG ledger version */
		std::string CEG_node_address_;   /* CEG node address */

	public:
		Monitor(CEG::server *server_h, CEG::client *client_h, CEG::tls_server *tls_server_h, CEG::tls_client *tls_client_h,
			CEG::connection_hdl con, const std::string &uri, int64_t id);

		void SetSessionId(const std::string &session_id);
		void SetActiveTime(int64_t current_time);
		bool IsActive() const;
		int64_t GetActiveTime() const;

		utils::InetAddress GetRemoteAddress() const;
		std::string GetPeerNodeAddress() const;

		bool SendHello(int32_t listen_port, const std::string &node_address, std::error_code &ec);
		void SetCEGInfo(const protocol::ChainStatus &hello);
	};
}

#endif