'use strict';

/* This contract implements a simple map that is stored in the metadata.
The following operations are available.

Invoke operations
put - requires two arguments, a key and value
remove - requires a key
get - requires one argument, a key, and returns a value
keys - requires no arguments, returns all keys
*/

const key_all = 'AllKeys';


function query(input_str){
    let input  = JSON.parse(input_str);
	
    let result = {};
    if(input.method === 'get'){
        result.value = storageLoad(input.key);
    }
    else if(input.method === 'keys'){
        result.keys = storageLoad(key_all);
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
	let keys = JSON.parse(storageLoad(key_all));
	if (input.method === 'put') {
		assert(input.key !== key_all, 'The key name(AllKeys) has been used internal');
		keys.push(input.key);
		storageStore(key_all, JSON.stringify(keys));
		storageStore(input.key, input.value);
	}
	else if (input.method === 'remove') {
		let i;
		for(i = 0; i < keys.length; i += 1) {
			if(keys[i] === input.key) {
				keys.splice(i, 1);
			}
		}
		storageDel(input.key);
		storageStore(key_all, JSON.stringify(keys));
	}
	else{
       	throw '<unidentified operation type>';
    }

    log(result);
    return JSON.stringify(result);
}

function init(){
	
	storageStore(key_all, JSON.stringify([]));
    return true;
}