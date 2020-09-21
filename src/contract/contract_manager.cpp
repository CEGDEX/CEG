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


#include <utils/logger.h>
#include <common/pb2json.h>
#include <common/private_key.h>
#include <ledger/ledger_frm.h>
#include <ledger/ledger_manager.h>
#include "contract_manager.h"

namespace CEG{
	ContractManager::ContractManager() {}
	ContractManager::~ContractManager() {}

	bool ContractManager::Initialize(int argc, char** argv) {
		V8Contract::Initialize(argc, argv);
		return true;
	}

	bool ContractManager::Exit() {
		return true;
	}

	Result ContractManager::SourceCodeCheck(int32_t type, const std::string &code, uint32_t ldcontext_stack_size) {
		ContractParameter parameter;
		parameter.code_ = code;
		Contract *contract = NULL;
		Result tmp_result;

		//"VERSION CHECKING condition" may be removed after version 1002
		if (CHECK_VERSION_GT_1001) {
			//New version max depth 4, the 4th can't call contract, ldcontext_stack_size include current tx.
			if (ldcontext_stack_size > General::CONTRACT_MAX_RECURSIVE_DEPTH) {
				tmp_result.set_code(protocol::ERRCODE_CONTRACT_TOO_MANY_RECURSION);
				tmp_result.set_desc("Too many recursion.");
				return tmp_result;
			}
		}

		if (type == Contract::TYPE_V8) {
			contract = new V8Contract(false, parameter);
		}
		else {
			tmp_result.set_code(protocol::ERRCODE_CONTRACT_SYNTAX_ERROR);
			tmp_result.set_desc(utils::String::Format("Contract type(%d) not supported", type));
			LOG_ERROR("%s", tmp_result.desc().c_str());
			return tmp_result;
		}

		bool ret = contract->SourceCodeCheck();
		tmp_result = contract->GetResult();
		delete contract;
		return tmp_result;
	}

	Result ContractManager::Execute(int32_t type, const ContractParameter &paramter) {
		Result ret;
		do {
			//"VERSION CHECKING condition" may be removed after version 1002
			if (CHECK_VERSION_GT_1001) {
				//New version max depth 4, the 4th can't call contract, tx_size include current tx.
				uint32_t tx_size = paramter.ledger_context_->transaction_stack_.size();
				if (tx_size > General::CONTRACT_MAX_RECURSIVE_DEPTH) {
					ret.set_code(protocol::ERRCODE_CONTRACT_TOO_MANY_RECURSION);
					ret.set_desc("Too many recursion.");
					break;
				}
			}
			//--------end-----------

			Contract *contract;
			if (type == Contract::TYPE_V8) {
				utils::MutexGuard guard(contracts_lock_);
				contract = new V8Contract(false, paramter);
				//Add the contract id. Use this ID when cancelling the contract in the future. 
				contracts_[contract->GetId()] = contract;
			}
			else {
				ret.set_code(protocol::ERRCODE_CONTRACT_EXECUTE_FAIL);
				LOG_ERROR("Contract type(%d) not supported", type);
				break;
			}

			LedgerContext *ledger_context = contract->GetParameter().ledger_context_;
			ledger_context->PushContractId(contract->GetId());
			contract->Execute();
			ret = contract->GetResult();
			ledger_context->PopContractId();
			ledger_context->PushLog(contract->GetParameter().this_address_, contract->GetLogs());
			do {
				//Delete the contract from map
				utils::MutexGuard guard(contracts_lock_);
				contracts_.erase(contract->GetId());
				delete contract;
			} while (false);

		} while (false);
		return ret;
	}

	bool ContractManager::Query(int32_t type, const ContractParameter &paramter, Json::Value &result) {
		do {
			//"VERSION CHECKING condition" may be removed after version 1002
			if (CHECK_VERSION_GT_1001) {
				//New version max depth 4, the 4th can't call contract, tx_size include current tx.
				uint32_t tx_size = paramter.ledger_context_->transaction_stack_.size();
				if (tx_size > General::CONTRACT_MAX_RECURSIVE_DEPTH) {
					LOG_TRACE("Too many recursion.");
					break;
				}
			}
			//--------end-----------

			Contract *contract;
			if (type == Contract::TYPE_V8) {
				utils::MutexGuard guard(contracts_lock_);
				contract = new V8Contract(true, paramter);
				//Add the contract id. Use this ID when cancelling the contract in the future.
				contracts_[contract->GetId()] = contract;
			}
			else {
				LOG_ERROR("Contract type(%d) not supported", type);
				break;
			}

			LedgerContext *ledger_context = contract->GetParameter().ledger_context_;
			ledger_context->PushContractId(contract->GetId());
			bool ret = contract->Query(result);
			ledger_context->PopContractId();
			ledger_context->PushLog(contract->GetParameter().this_address_, contract->GetLogs());
			ledger_context->PushRet(contract->GetParameter().this_address_, result);
			do {
				//Delete the contract from map
				utils::MutexGuard guard(contracts_lock_);
				contracts_.erase(contract->GetId());
				delete contract;
			} while (false);

			return ret;
		} while (false);
		return false;
	}

	bool ContractManager::Cancel(int64_t contract_id) {
		//Start another thread to delete the contract sandbox after running the contract.
		Contract *contract = NULL;
		do {
			utils::MutexGuard guard(contracts_lock_);
			ContractMap::iterator iter = contracts_.find(contract_id);
			if (iter!= contracts_.end()) {
				contract = iter->second;
			} 
		} while (false);

		if (contract){
			contract->Cancel();
		} 

		return true;
	}

	Contract *ContractManager::GetContract(int64_t contract_id) {
		do {
			utils::MutexGuard guard(contracts_lock_);
			ContractMap::iterator iter = contracts_.find(contract_id);
			if (iter != contracts_.end()) {
				return iter->second;
			}
		} while (false);
		return NULL;
	}

}
