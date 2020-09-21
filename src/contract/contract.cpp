#include <ledger/ledgercontext_manager.h>
#include "contract.h"

namespace CEG {
	ContractParameter::ContractParameter(){
		block_.Reset();
		tx_.Reset();
		msg_.Reset();

		init_ = false;
		code_ = "";
		input_ = "";
		this_address_ = "";
		ledger_context_ = NULL;
	}

	ContractParameter::~ContractParameter() {}

	ContractTestParameter::ContractTestParameter() : opt_type_(QUERY), fee_limit_(0), gas_price_(0), contract_balance_(0) {}

	ContractTestParameter::~ContractTestParameter() {}

	TransactionTestParameter::TransactionTestParameter() {}
	TransactionTestParameter::~TransactionTestParameter() {}

	utils::Mutex Contract::contract_id_seed_lock_;
	int64_t Contract::contract_id_seed_ = 0;
	Contract::Contract() {
		utils::MutexGuard guard(contract_id_seed_lock_);
		id_ = contract_id_seed_;
		contract_id_seed_++;
		tx_do_count_ = 0;
		readonly_ = false;
	}

	Contract::Contract(bool readonly, const ContractParameter &parameter) :
		readonly_(readonly), parameter_(parameter) {
		utils::MutexGuard guard(contract_id_seed_lock_);
		id_ = contract_id_seed_;
		contract_id_seed_++;
		tx_do_count_ = 0;
	}

	Contract::~Contract() {}

	bool Contract::Execute() {
		return true;
	}

	bool Contract::Cancel() {
		return true;
	}

	bool Contract::Query(Json::Value& jsResult) {
		return true;
	}

	bool Contract::SourceCodeCheck() {
		return true;
	}

	int64_t Contract::GetId() {
		return id_;
	}

	int32_t Contract::GetTxDoCount() {
		return tx_do_count_;
	}

	void Contract::IncTxDoCount() {
		tx_do_count_++;
	}

	const ContractParameter &Contract::GetParameter() {
		return parameter_;
	}

	bool Contract::IsReadonly() {
		return readonly_;
	}

	const utils::StringList &Contract::GetLogs() {
		return logs_;
	}

	void Contract::AddLog(const std::string &log) {
		logs_.push_back(log);
		if (logs_.size() > 10) logs_.pop_front();
	}

	Result &Contract::GetResult() {
		return result_;
	}

	void Contract::SetResult(Result &result) {
		result_ = result;
	}
}