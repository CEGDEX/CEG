# **Syntax in the Smart Contract**
CEG smart contracts are written in the 'JaveScript'. In order to facilitate developers to develop a more standardized and safer contract, JSLint is used to check the syntax in smart contracts. Please refer to [JSLint GitHub](./). When editing a contract, you first need to ensure that the contract passes the test inJSLint before it can be detected as a legal contract by the CEG system.

The standard syntax of JSLint is described in detail on the official website. The purpose of this document is to refine the original JSLint syntax rules as a complete document, and to supplement CEG's modified rules. The documentation will illustrate its usage with examples. For parts not mentioned in this article, please refer to the [JsLint help manual](http://CEG.chinacloudapp.cn:36002/help.html).

Or you can visit the manual at this site: 127.0.0.1:36002/jslint/help.html by node servers or wallet addresses.

## **Detection Tool**
   JSLint detection tool address: [JSLint syntax dection tool](http://CEG.chinacloudapp.cn:36002/jslint.html "JSLint syntax detection tool").

   Or you can visit 127.0.0.1:36002/jslint/index.html by node servers or wallet addresses. 

For error description, details will be given when you debug contract syntax in the web tool. When you input the following code:

```javascript

"use strict";
function init(bar)
{
    
}
```

Error is shown below:

```
Empty block.   2.0
{
```

Cause: Blank statement block at row 2 and column 0.

Correct code is shown below:

```javascript

"use strict";
function init(bar)
{
    return;    
}
```

If the result is correct, no warning information will prompt.

## **Text Compression**
After the contract document is written, you can use the JSMin tool to compress it. Ensure that the original document is saved because compression is an irreversible operation.

[Tool address](../../../deploy/jsmin/)

## **Demo**
```javascript

"use strict";
function init(bar)
{
    /*init whatever you want*/
    return;
}

function main(input) 
{
    log(input);

    //for statement
    let i;
    for (i = 0; i < 5; i += 1) 
    {
        log(i);
    }

    //while statement
    let b = 10;
    while (b !== 0) 
    {
        b -= 1;
        log(b);
    }

    //if statement
    let compare = 1;
    if(compare === 1)
    {
        log("it is one");
    }
    else if(compare === 2)
    {
        log("it is two");
    }
    else
    {
        log("it is other");
    }

    //if statement
    if(compare !== 2)
    {
        log("no, different");
    }

    //switch statement
    let sw_value = 1;
    switch(sw_value)
    {
    case 1:
        log("switch 1");
        break;
    default:
        log("default");
    }

    //Number
    let my_num = Number(111);
    log(my_num);

    //String
    let my_str = String(111);
    log(my_str);

    //Boolean
    let my_bool = Boolean(111);
    log(my_bool);

    //Array
    let str_array = ["red","black"]; 
    log(str_array);

    //Array
    let num_array = [1,2,3,4];
    log(num_array);

    throw "this is a exception";
}
```

## **Rules List**
 
- Detect the statement strictly with all source code added the 'use strict' field at the beginning

- Use 'let' to declare variables within a statement block

- Use '===' instead of '==' to judge the comparison; use '!==' instead of '!=' to compare
- A statement must end with ';'

- A statement block must be enclosed with '{}' and empty statement blocks are prohibited

- The initial variable of the 'for' loop variable needs to be declared before the conditional statement block, and a new value is assigned to it when used

- Use '+=' and '-=' to substitute '++' and '--'

- Prohibit to use keywords like 'eval', 'void'

- Prohibit to use 'new' to create 'Number', 'String' and 'Boolean' objects, which objects can be obtained by calling their constructors

- Prohibit to create an array with array keywords

- Prohibit to use keywords like 'try' and 'catch', but you can use 'throw' to throw exceptions

```javascript
"Array", "ArrayBuffer", "Float32Array", "Float64Array", 
"Int8Array", "Int16Array", "Int32Array", "Uint8Array", 
"Uint8ClampedArray", "Uint16Array", "Uint32Array"

let color = new Array(100); //Compiling error

//You can use the alternative new Array(100) statement;
let color = ["red","black"]; 
let arr = [1,2,3,4];
```

- Keywords prohibited to use
```javascript
"DataView", "decodeURI", "decodeURIComponent", "encodeURI", 
"encodeURIComponent", "Generator","GeneratorFunction", "Intl", 
"Promise", "Proxy", "Reflect", "System", "URIError", "WeakMap", 
"WeakSet", "Math", "Date"
```
