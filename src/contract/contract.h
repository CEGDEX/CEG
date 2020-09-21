#ifndef CONTRACT_H_
#define CONTRACT_H_

#include <proto/cpp/chain.pb.h>
#include <utils/headers.h>
#include <common/general.h>

namespace CEG {
	class LedgerContext;
	class ContractParameter {
	public:
		ContractParameter();
		~ContractParameter();

		typedef struct Block{
			Block(){
				Reset();
			}
			void Reset(){
				timestamp_ = 0;
				number_ = 0;
			}
			int64_t timestamp_;
			int64_t number_;
		};

		typedef struct Tx{
			Tx(){
				Reset();
			}
			void Reset(){
				initiator_ = "";
				sender_ = "";
				gas_price_ = 0;
				fee_limit_ = 0;
				hash_ = "";
			}
			std::string initiator_;
			std::string sender_;
			int64_t gas_price_;
			int64_t fee_limit_;
			std::string hash_;
		};

		typedef struct Msg{
			Msg(){
				Reset();
			}
			void Reset(){
				initiator_ = "";
				sender_ = "";
				asset_.Clear();
				coin_amount_ = 0;
				operation_index_ = 0;
				nonce_ = 0;
			}
			std::string initiator_;
			std::string sender_;
			protocol::Asset asset_;
			int64_t coin_amount_;
			int32_t operation_index_;
			int64_t nonce_;
		};

		bool init_;
		std::string code_;
		std::string input_;
		std::string this_address_;
		LedgerContext *ledger_context_;
		
		Block block_;
		Tx tx_;
		Msg msg_;
	};

	class TestParameter {};

	class ContractTestParameter :public TestParameter {
	public:
		ContractTestParameter();
		~ContractTestParameter();

		typedef enum tagOptType {
			INIT = 0,
			MAIN = 1,
			QUERY = 2
		}OptType;

		OptType opt_type_;
		std::string contract_address_;
		std::string code_;
		std::string input_;
		std::string source_address_;
		int64_t contract_balance_;
		int64_t fee_limit_;
		int64_t gas_price_;
	};

	class TransactionTestParameter :public TestParameter {
	public:
		TransactionTestParameter();
		~TransactionTestParameter();

		protocol::ConsensusValue consensus_value_;
	};

	class Contract {
	protected:
		int32_t type_;
		bool readonly_;
		int64_t id_;
		ContractParameter parameter_;

		Result result_;
		int32_t tx_do_count_;  //Transactions triggerred by one contract.
		utils::StringList logs_;
	public:
		Contract();
		Contract(bool readonly, const ContractParameter &parameter);
		virtual ~Contract();

	public:
		virtual bool Execute();
		virtual bool Cancel();
		virtual bool SourceCodeCheck();
		virtual bool Query(Json::Value& jsResult);

		int32_t GetTxDoCount();
		void IncTxDoCount();
		int64_t GetId();
		const ContractParameter &GetParameter();
		bool IsReadonly();
		const utils::StringList &GetLogs();
		void AddLog(const std::string &log);
		void SetResult(Result &result);
		Result &GetResult();
		static utils::Mutex contract_id_seed_lock_;
		static int64_t contract_id_seed_;

		enum TYPE {
			TYPE_V8 = 0,
			TYPE_ETH = 1
		};
	};

	typedef std::map<int64_t, Contract *> ContractMap;
}

#endif