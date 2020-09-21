'use strict';

function transfer(to, value){
    assert(addressCheck(to) === true, 'Arg-to is not a valid address.');
    assert(stoI64Check(value) === true, 'Arg-value must be alphanumeric.');
    assert(int64Compare(value, '0') > 0, 'Arg-value must be greater than 0.');
    if(sender === to) {
        tlog('transfer', sender, to, value);  
        return true;
    }
    
    let senderValue = storageLoad(sender);
    assert(senderValue !== false, 'Failed to get the balance of ' + sender + ' from metadata.');
    assert(int64Compare(senderValue, value) >= 0, 'Balance:' + senderValue + ' of sender:' + sender + ' < transfer value:' + value + '.');

    let toValue = storageLoad(to);
    toValue = (toValue === false) ? value : int64Add(toValue, value); 
    storageStore(to, toValue);

    senderValue = int64Sub(senderValue, value);
    storageStore(sender, senderValue);

    tlog('transfer', sender, to, value);

    return true;
}

function balanceOf(address){
    assert(addressCheck(address) === true, 'Arg-address is not a valid address.');

    let value = storageLoad(address);
    assert(value !== false, 'Failed to get the balance of ' + address + ' from metadata.');
    return value;
}

function init(input_str){
    let params = JSON.parse(input_str).params;
    assert(stoI64Check(params.supply) === true, 'Args check failed.');
    let attribute = {};
    attribute.supply = params.supply;
    
    storageStore('global', JSON.stringify(attribute));
    storageStore(sender, attribute.supply);
}

function main(input_str){
    let input = JSON.parse(input_str);
    if(input.method === 'transfer'){
        transfer(input.params.to, input.params.value);
        return;
    }

    throw '<Main interface passes an invalid operation type>';
}

function query(input_str){
    let result = {};
    let input  = JSON.parse(input_str);

    if(input.method === 'balanceOf'){
        result.balance = balanceOf(input.params.address);
        return JSON.stringify(result);
    }

    throw '<Query interface passes an invalid operation type>';
}
