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

#ifndef CONTRACT_MANAGER_H_
#define CONTRACT_MANAGER_H_

#include "v8_contract.h"

namespace CEG{

	class ContractManager :
		public utils::Singleton<ContractManager>{
		friend class utils::Singleton<ContractManager>;

		utils::Mutex contracts_lock_;
		ContractMap contracts_;
	public:
		ContractManager();
		~ContractManager();

		bool Initialize(int argc, char** argv);
		bool Exit();

		Result Execute(int32_t type, const ContractParameter &paramter);
		bool Query(int32_t type, const ContractParameter &paramter, Json::Value &result);
		bool Cancel(int64_t contract_id);
		Result SourceCodeCheck(int32_t type, const std::string &code, uint32_t ldcontext_stack_size);
		Contract *GetContract(int64_t contract_id);
	};
}
#endif
