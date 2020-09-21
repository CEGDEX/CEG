'use strict';

/* This contract implements a simple function of payment from A to B.
The following operations are available.

Available operations
create - requires two arguments, the account to create and the amount of init balance.
remove - requires one argument, the account to remove.
get - requires one argument, the account, return balance of the account.
pay - requires three arguments, accountA, accountB and the amount of payment.
*/

function pay(account_a, account_b, amount) {
	let result = {};
	result.errcode = 0;
	let amount_a = Number(storageLoad(account_a));
	let amount_b = Number(storageLoad(account_b));
	
	if(amount_a !== Number.NaN && amount_b !== Number.NaN) {
		if(amount_a - amount < 0) {
			result.errmsg = 'The balance of ' + account_a + '(' + amount_a + ') is less than the amount of payment ' + amount;
			result.errcode = 1;
			return result;
		}
	} else {
		result.errmsg = 'Can not convert string to number';
		result.errcode = 1;
		return result;
	}
	amount_a -= amount;
	amount_b += amount;
	storageStore(account_a, String(amount_a));
	storageStore(account_b, String(amount_b));
	result.errmsg = 'Payment succeed, now the balance of ' + account_a + ' is ' + amount_a + ', and the balance of ' + account_b + ' is ' + amount_b; 
	
}

function createAccount(account, amount) {
	let res = {};
	let amount_str = storageLoad(account);
	assert(amount_str === false, 'Account already exist');
	
	storageStore(account, String(amount));
}

function query(input_str){
    let input  = JSON.parse(input_str);
	
    let result = {};
    if(input.method === 'get'){
        result.amount = storageLoad(input.account);
    }
    else{
       	throw '<unidentified operation type>';
    }

    log(result);
    return JSON.stringify(result);
}

function main(input_str){
	let input  = JSON.parse(input_str);
	
	let result = {};
	if (input.method === 'pay') {
		assert(typeof input.from === 'string' && typeof input.to === 'string', 'The type of from and to should be string');
		assert(typeof input.amount === 'number', 'The type of amount should be number');
		result = pay(input.from, input.to, input.amount);
	}
	else if (input.method === 'create') {
		assert(typeof input.account === 'string', 'The type of account should be string');
		assert(typeof input.amount === 'number', 'The type of amount should be number');
		createAccount(input.account, input.amount);
	}
	else if (input.method === 'delete') {
		assert(typeof input.account === 'string', 'The type of account should be string');
		storageDel(input.account);
	}
 	else{
       	throw '<unidentified operation type>';
    }

    log(result);
    return JSON.stringify(result);
}

function init(){
	return true;
}