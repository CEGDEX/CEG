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

#ifndef OPERATION_FRM_H_
#define OPERATION_FRM_H_

#include <utils/common.h>
#include <common/general.h>
#include <ledger/account.h>
#include "transaction_frm.h"

#include "environment.h"
namespace CEG{

	class TransactionFrm;
	class OperationFrm {
	protected:
		protocol::Operation operation_;
		TransactionFrm* transaction_;
		int32_t	index_;
		Result result_;
		std::shared_ptr<AccountFrm> source_account_;
		int64_t ope_fee_;
	public:

		OperationFrm(const protocol::Operation &operation, TransactionFrm* tran, int32_t index = 0);
		~OperationFrm();

		Result Apply(std::shared_ptr<Environment> txenvironment);

		bool CheckSignature(std::shared_ptr<Environment> txenvironment);

		Result GetResult() const;
		int64_t GetOpeFee() const;

		static bool isReservedAddress(const std::string& addr);
		static bool ReservedAddrAvailable(const std::string& src, const std::string& dest);
		static Result CheckValid(const protocol::Operation& ope, const std::string &source_address, uint32_t ldcontext_stack_size);
	protected:
		void CreateAccount(std::shared_ptr<Environment> environment);
		void IssueAsset(std::shared_ptr<Environment> environment);
		void PayAsset(std::shared_ptr<Environment> environment);
		void SetMetaData(std::shared_ptr<Environment> environment);
		void SetSignerWeight(std::shared_ptr<Environment> environment);
		void SetThreshold(std::shared_ptr<Environment> environment);
		void PayCoin(std::shared_ptr<Environment> environment);
		void Log(std::shared_ptr<Environment> environment);
		void SetPrivilege(std::shared_ptr<Environment> environment);
		void Exit(std::shared_ptr<Environment> environment);

	private:
		static Result CheckCreateAccountGt1000(const protocol::OperationCreateAccount& create_account, uint32_t ldcontext_stack_size);
		static Result CheckSetPrivilege(const protocol::OperationSetPrivilege &set_privilege, const std::string &source_address);
		void ChangeCreateContractAmount(const int64_t &init_amount, int64_t &changed_amount);
	};
};
#endif