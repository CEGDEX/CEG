
#ifndef CHAIN_RPC_H_
#define CHAIN_RPC_H_

#include <utils/singleton.h>
#include <utils/thread.h>
#include <notary/configure.h>
#include <common/private_key.h>
#include <notary/http_client.h>

namespace CEG {

	struct CommContractInfo{
		utils::StringVector recv_notarys;
		utils::StringVector send_notarys;
		std::string comm_unique;
		int64_t recv_finish_seq;
		int64_t recv_max_seq;
		int64_t send_finish_seq;
		int64_t send_max_seq;
		std::string send_f_comm_addr;
		std::string send_t_comm_addr;
		std::string f_chain_id;
		std::string t_chain_id;
		int64_t cur_blockchain_seq;

		void Reset(){
			recv_notarys.clear();
			send_notarys.clear();
			comm_unique = "";
			recv_finish_seq = 0;
			recv_max_seq = 0;
			send_finish_seq = 0;
			send_max_seq = 0;
			cur_blockchain_seq = 0;
		}

		Json::Value ToJson(){
			Json::Value data;
			data["comm_unique"] = comm_unique;
			data["recv_finish_seq"] = recv_finish_seq;
			data["recv_max_seq"] = recv_max_seq;
			data["send_finish_seq"] = send_finish_seq;
			data["send_max_seq"] = send_max_seq;
			data["send_f_comm_addr"] = send_f_comm_addr;
			data["send_t_comm_addr"] = send_t_comm_addr;
			data["f_chain_id"] = f_chain_id;
			data["t_chain_id"] = t_chain_id;
			data["cur_blockchain_seq"] = cur_blockchain_seq;
			return data;
		}

	};

	enum ProposalType{
		PROPOSAL_NONE = 0,
		PROPOSAL_SEND = 1,
		PROPOSAL_RECV = 2,
	};

	struct ProposalInfo{
		ProposalType type;
		int64_t proposal_id;
		std::string proposal_body;
		int32_t status;
		utils::StringVector confirmed_notarys;
		bool inited_;
		bool completed_;

		Json::Value ToJson(){
			Json::Value info;
			info["type"] = type;
			info["proposal_id"] = proposal_id;
			info["proposal_body"] = proposal_body;
			info["status"] = status;
			info["inited_"] = inited_;
			info["completed_"] = completed_;
			std::string notarys;
			for (uint32_t i = 0; i < confirmed_notarys.size(); i++){
				notarys = utils::String::AppendFormat("%s,", confirmed_notarys[i].c_str());
			}

			return info;
		}
	};

	typedef std::vector<ProposalInfo> ProposalInfoVector;

	class INotifyRpc{
	public:
		virtual void HandleTxResult(const std::string &hash, int32_t code) = 0;
	};

	class BaseChainRpc{
	public:
		BaseChainRpc(INotifyRpc *notify, const ChainConfigure &config);
		~BaseChainRpc();

	public:
		virtual bool GetCommContractInfo(const std::string &address, CommContractInfo &comm_info) = 0;
		virtual bool GetProposal(const CommContractInfo &comm_info, const std::string &address, ProposalType type, int64_t seq, ProposalInfo &proposal_info) = 0;
		virtual utils::StringVector CommitVotedProposals(const ProposalInfoVector &proposal_vector) = 0;
		virtual void OnTimer() = 0;

	protected:
		INotifyRpc *notify_;
		ChainConfigure config_;
	};

	//CEG chain rpc
	class CEGChainRpc : public BaseChainRpc{
	public:
		CEGChainRpc(INotifyRpc *notify, const ChainConfigure &config);
		~CEGChainRpc();
		virtual bool GetCommContractInfo(const std::string &address, CommContractInfo &comm_info) override;
		virtual bool GetProposal(const CommContractInfo &comm_info, const std::string &address, ProposalType type, int64_t seq, ProposalInfo &proposal_info) override;
		virtual utils::StringVector CommitVotedProposals(const ProposalInfoVector &proposal_vector) override;
		virtual void OnTimer() override;

	private:
		int64_t nonce_;
		PrivateKey private_key_;

	private:
		std::string PackUrl(const std::string &path);
	};

	//ETH RPC
	class EthChainRpc : public BaseChainRpc{
	public:
		EthChainRpc(INotifyRpc *notify, const ChainConfigure &config);
		~EthChainRpc();
		virtual bool GetCommContractInfo(const std::string &address, CommContractInfo &comm_info) override;
		virtual bool GetProposal(const CommContractInfo &comm_info, const std::string &address, ProposalType type, int64_t seq, ProposalInfo &proposal_info) override;
		virtual utils::StringVector CommitVotedProposals(const ProposalInfoVector &proposal_vector) override;
		virtual void OnTimer() override {}

	private:
		std::string SendEthTx(const std::string &api, const std::string &paras);

	private:
		std::string PackUrl(const std::string &path);
	};

	std::shared_ptr<BaseChainRpc> CreateChainRpc(INotifyRpc *notify, const ChainConfigure &config);

}

#endif
