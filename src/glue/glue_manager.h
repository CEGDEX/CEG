﻿/*
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

#ifndef GLUE_MANAGER_
#define GLUE_MANAGER_

#include <utils/singleton.h>
#include <utils/net.h>
#include <utils/lrucache.hpp>
#include <overlay/peer.h>
#include <consensus/consensus_manager.h>
#include "transaction_queue.h"
#include "ledger_upgrade.h"

namespace CEG {

	class GlueManager : public utils::Singleton < CEG::GlueManager>,
		public CEG::TimerNotify,
		public CEG::StatusModule,
		public IConsensusNotify {

		utils::Mutex lock_;
		std::shared_ptr<TransactionQueue> tx_pool_;

		int64_t time_start_consenus_;
		std::shared_ptr<Consensus> consensus_;

		int64_t ledgerclose_check_timer_;
		int64_t start_consensus_timer_;

		//For getting module status
		time_t process_uptime_;

		//Hardfork point
		std::set<std::string> hardfork_points_;

		//For ledger upgrade
		LedgerUpgrade ledger_upgrade_;

		bool LoadLastLedger();
		bool CreateGenesisLedger();
		void StartLedgerCloseTimer();
	public:
		GlueManager();
		~GlueManager();

		bool Initialize();
		bool Exit();

		bool StartConsensus(const std::string &last_consavlue); //Start to trigger consensus
		int64_t GetIntervalTime(bool empty_block);

		bool OnTransaction(TransactionFrm::pointer tx, Result &err);
		bool OnConsensus(const ConsensusMsg &msg);
		void NotifyErrTx(std::vector<TransactionFrm::pointer> &txs);

		//Called by the ledger manger once the ledger is closed
		void UpdateValidators(const protocol::ValidatorSet &validators, const std::string &proof);
		void LedgerHasUpgrade();

		//Ledger upgrade
		void OnRecvLedgerUpMsg(const protocol::LedgerUpgradeNotify &msg);
		protocol::Signature SignConsensusData(const std::string &data);

		//Should be called by the ledger manager.
		bool CheckValueAndProof( const std::string &consensus_value, const std::string &proof);
		int32_t CheckValueHelper(const protocol::ConsensusValue &consensus_value, int64_t now);
		size_t GetTransactionCacheSize();
		void QueryTransactionCache(const uint32_t& num, std::vector<TransactionFrm::pointer>& txs);
		bool QueryTransactionCache(const std::string& hash, TransactionFrm::pointer& tx);

		virtual void OnTimer(int64_t current_time) override;
		virtual void OnSlowTimer(int64_t current_time) override {};
		virtual void GetModuleStatus(Json::Value &data);
		virtual time_t GetProcessUptime();

		// IConsensusNotify
		virtual std::string OnValueCommited(int64_t request_seq, const std::string &value, const std::string &evidence,bool calculate_total);
		virtual void OnViewChanged(const std::string &last_consvalue);
		virtual int32_t CheckValue(const std::string &value);
		virtual void SendConsensusMessage(const std::string &message);
		virtual std::string FetchNullMsg();
		virtual void OnResetCloseTimer();
		virtual std::string DescConsensusValue(const std::string &request);
	};
};

#endif
