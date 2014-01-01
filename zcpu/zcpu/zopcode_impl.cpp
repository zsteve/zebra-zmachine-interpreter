#include "zopcode_impl.h"
#include "zopcode.h"
#include "zcpu.h"
#include "../../zstack/zstack/zstack.h"
#include "../../zmemory/zmemory/zobject.h"
#include "../../ztext/ztext/ztext.h"
#include "../../zinout/zinout/zinout.h"
#include "../../zdictionary/zdictionary/zdictionary.h"

#include "../../num2ascii/num2ascii/num2ascii.h"

#include <cmath>

extern int zVersion;

	/* function template
	// implementation of FUNCTION opcode
	int FUNCTION(ZOpcode& zOp){
		try{

		}catch(...){
			throw IllegalZOpcode();
		}
	}
	*/

namespace ZOpcodeImpl{
	/**
	 *flag for jump (branch)
	 * if true, a jump needs to be made
	 */
	int* jumpFlag=NULL;
	szword* jumpValue=NULL;	// pointer to var to hold byte offset for jump 
	ZCpu* cpuObj=NULL;		// pointer to ZCpu object

	/** cpu peripherals **/
	ZMemory* zMemory;
	ZObjectTable* zObject;
	ZStack* zStack;
	ZInOut* zInOut;
	ZDictionary* zDictionary;

	void registerJumpFlag(int* f){
		jumpFlag=f;
	}

	void registerJumpValue(szword* o){
		jumpValue=o;
	}

	void registerZCpuObj(ZCpu* o){
		cpuObj=o;
		zMemory=&cpuObj->zMem;
		zObject=&cpuObj->zObject;
		zStack=&cpuObj->zStack;
		zInOut=&cpuObj->zInOut;
		zDictionary=&cpuObj->zDict;
	}

	// implementations for z-machine opcodes

	/**
	 * retrieves operand value
	 * if the operand is a constant, it just returns the operand
	 * if the operand is a variable, it returns the value stored in the variable
	 */
	int retrieveOperandValue(ZOpcode& zOp, int operandNum){
		ZOperandType operandType;
		operandType=zOp.getOperandTypes()[operandNum];
		int operandValue=0;
		if(operandType==ZOPERANDTYPE_LARGE_CONST || operandType==ZOPERANDTYPE_SMALL_CONST){
			operandValue=zOp.getOperands()[operandNum];
		}else if(operandType==ZOPERANDTYPE_VAR){
			if(zOp.getOperands()[operandNum]<0x10 && zOp.getOperands()[operandNum]>0){
				operandValue=cpuObj->currentRoutine->readLocalVar(*zStack, zOp.getOperands()[operandNum]);
			}else if(zOp.getOperands()[operandNum]==0x0){
				// var 0 refers to the top of the stack (stack[stack_ptr])
				// reading from var 0 pulls a value off the top of the stack
				operandValue=zStack->pull();
			}else{
				operandValue=zMemory->readGlobalVar(zOp.getOperands()[operandNum]);
			}
		}
		return operandValue;
	}

	void storeVariable(zbyte varNum, zword value){
		if(varNum<0x10 && varNum>0){
			cpuObj->currentRoutine->storeLocalVar(*zStack, varNum, value);
		}else if(varNum==0x0){
			zStack->push(value);
		}else{
			zMemory->storeGlobalVar(varNum, value);
		}
	}

	void routineEnter(ZOpcode& zOp, ulong routineAddr){
		ZCpuInternal::ZCpuRoutine* oldRoutine=cpuObj->currentRoutine;
		oldRoutine->resumeAddr=(cpuObj->getPCounter()+zOp.getOpcodeSize());
		// save the current stack state
		// it is imperative that this is done BEFORE createLocalVars()
		cpuObj->stackFrame.pushEntry(zStack->getStackPtr());
		cpuObj->currentRoutine=new ZCpuInternal::ZCpuRoutine();
		cpuObj->currentRoutine->loadAddr(routineAddr, cpuObj->zMem);
		cpuObj->currentRoutine->parentRoutine=oldRoutine;
		cpuObj->currentRoutine->createLocalVars(*zStack);
	}

	void routineReturn(ZOpcode& zOp, zword value){
		zbyte returnVar=cpuObj->currentRoutine->retValueDest;
		bool keepRetVal=cpuObj->currentRoutine->keepRetValue;
		ZCpuInternal::ZCpuRoutine* parentRoutine=cpuObj->currentRoutine->parentRoutine;
		// begin destroying current routine and restoring parent
		/** perhaps there is no point in using destroyLocalVars() **/
		//cpuObj->currentRoutine->destoryLocalVars(*zStack);
		delete cpuObj->currentRoutine;
		cpuObj->currentRoutine=parentRoutine;
		// restore old stack pointer
		zStack->setStackPtr(cpuObj->stackFrame.pullEntry());
		// restore old program counter
		*jumpFlag=JUMP_POSITION;
		*jumpValue=(cpuObj->currentRoutine->resumeAddr);
		// store return value
		if(keepRetVal)
			storeVariable(returnVar, value);
	}

	// implementation of JE instruction
	int JE(ZOpcode& zOp){
		// operand 1 : a, operand 2 : b
		// jumps if a == b
		try{
			int a, b;
			a=retrieveOperandValue(zOp, 0);
			b=retrieveOperandValue(zOp, 1);
			bool cond=(a==b);
			if(zOp.branchInfo.branchCond==cond){
				// jump
				if(zOp.branchInfo.branchOffset==0){
					// return false
					routineReturn(zOp, 0);
				}else if(zOp.branchInfo.branchOffset==1){
					// return true
					routineReturn(zOp, 1);
				}else{
					*jumpFlag=JUMP_OFFSET;
					*jumpValue=zOp.branchInfo.branchOffset;
				}
			}else{
				*jumpFlag=JUMP_NONE;
			}
			return 0;
		}catch(...){
			throw IllegalZOpcode();
		}
		return 0;
	}

	// implementation of JL instruction
	int JL(ZOpcode& zOp){
		// operand 1 : a, operand 2 : b
		// jumps if a < b
		try{
			int a, b;
			a=retrieveOperandValue(zOp, 0);
			b=retrieveOperandValue(zOp, 1);
			bool cond=(a<b);
			if(zOp.branchInfo.branchCond==cond){
				// jump
				if(zOp.branchInfo.branchOffset==0){
					// return false
					routineReturn(zOp, 0);
				}else if(zOp.branchInfo.branchOffset==1){
					// return true
					routineReturn(zOp, 1);
				}else{
					*jumpFlag=JUMP_OFFSET;
					*jumpValue=zOp.branchInfo.branchOffset;
				}
			}else{
				*jumpFlag=JUMP_NONE;
			}
			return 0;
		}catch(...){
			throw IllegalZOpcode();
		}
		return 0;
	}

	// implementation of JG instruction
	int JG(ZOpcode& zOp){
		// operand 1 : a, operand 2 : b
		// jumps if a > b
		try{
			int a, b;
			a=retrieveOperandValue(zOp, 0);
			b=retrieveOperandValue(zOp, 1);
			bool cond=(a>b);
			if(zOp.branchInfo.branchCond==cond){
				// jump
				if(zOp.branchInfo.branchOffset==0){
					// return false
					routineReturn(zOp, 0);
				}else if(zOp.branchInfo.branchOffset==1){
					// return true
					routineReturn(zOp, 1);
				}else{
					*jumpFlag=JUMP_OFFSET;
					*jumpValue=zOp.branchInfo.branchOffset;
				}
			}else{
				*jumpFlag=JUMP_NONE;
			}
			return 0;
		}catch(...){
			throw IllegalZOpcode();
		}
		return 0;
	}

	// implementation of DEC_CHK instruction
	int DEC_CHK(ZOpcode& zOp){
		/*Decrement variable, and branch if it is
		now less than the given value.
		*/
		try{
			int var, val;
			val=zOp.getOperandTypes()[1];

			// var's type is implied as a VARIABLE
			var=retrieveOperandValue(zOp, 0);
			val=retrieveOperandValue(zOp, 1);
			
			var--;
			bool cond=(var<val);
			if(zOp.branchInfo.branchCond==cond){
				// jump
				if(zOp.branchInfo.branchOffset==0){
					// return false
					routineReturn(zOp, 0);
				}else if(zOp.branchInfo.branchOffset==1){
					// return true
					routineReturn(zOp, 1);
				}else{
					*jumpFlag=JUMP_OFFSET;
					*jumpValue=zOp.branchInfo.branchOffset;
				}
			}else{
				*jumpFlag=JUMP_NONE;
			}
			// now we need to store again
			storeVariable(zOp.getOperands()[0], var);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of INC_CHK instruction
	int INC_CHK(ZOpcode& zOp){
		try{
		/*Increment variable, and branch if it is
		now more than the given value.
		*/
			int var, val;
			// var's type is implied as a VARIABLE
			// Inform translates the variable name as a "small constant"
			// dirty trick to make it think it's a variable...
			zOp.getOperandTypes()[0]=ZOPERANDTYPE_VAR;
			var=retrieveOperandValue(zOp, 0);
			zOp.getOperandTypes()[0]=ZOPERANDTYPE_SMALL_CONST;
			val=retrieveOperandValue(zOp, 1);
			
			var++;
			bool cond=(var<val);
			if(zOp.branchInfo.branchCond==cond){
				// jump
				if(zOp.branchInfo.branchOffset==0){
					// return false
					routineReturn(zOp, 0);
				}else if(zOp.branchInfo.branchOffset==1){
					// return true
					routineReturn(zOp, 1);
				}else{
					*jumpFlag=JUMP_OFFSET;
					*jumpValue=zOp.branchInfo.branchOffset;
				}
			}else{
				*jumpFlag=JUMP_NONE;
			}
			// now we need to store again
			storeVariable(zOp.getOperands()[0], var);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of JIN instruction
	int JIN(ZOpcode& zOp){
		//  jin obj1 obj2 ?(label)
		/* Jump if object a is a direct child of b, i.e., if parent of a is b.  */
		try{
			int a, b;
			a=retrieveOperandValue(zOp, 0);
			b=retrieveOperandValue(zOp, 1);
			bool cond=(zObject->getObjectChild(b)==a);
			if(zOp.branchInfo.branchCond==cond){
				// jump
				if(zOp.branchInfo.branchOffset==0){
					// return false
					routineReturn(zOp, 0);
				}else if(zOp.branchInfo.branchOffset==1){
					// return true
					routineReturn(zOp, 1);
				}else{
					*jumpFlag=JUMP_OFFSET;
					*jumpValue=zOp.branchInfo.branchOffset;
				}
			}else{
				*jumpFlag=JUMP_NONE;
			}
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of TEST instruction
	int TEST(ZOpcode& zOp){
		// Jump if all of the flags in bitmap are set (i.e. if bitmap & flags == flags). 
		try{
			zword bmp=0, flags=0;
			bmp=retrieveOperandValue(zOp, 0);
			flags=retrieveOperandValue(zOp, 1);
			bool cond=((bmp & flags)==flags);
			if(zOp.branchInfo.branchCond==cond){
				// jump
				if(zOp.branchInfo.branchOffset==0){
					// return false
					routineReturn(zOp, 0);
				}else if(zOp.branchInfo.branchOffset==1){
					// return true
					routineReturn(zOp, 1);
				}else{
					*jumpFlag=JUMP_OFFSET;
					*jumpValue=zOp.branchInfo.branchOffset;
				}
			}else{
				*jumpFlag=JUMP_NONE;
			}
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of OR instruction
	int OR(ZOpcode& zOp){
		// or a, b -> result
		try{
			zword a, b;
			a=retrieveOperandValue(zOp, 0);
			b=retrieveOperandValue(zOp, 1);
			storeVariable(zOp.storeInfo.storeVar, (a|b));
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of AND instruction
	int AND(ZOpcode& zOp){
		// and a, b -> result
		try{
			zword a, b;
			a=retrieveOperandValue(zOp, 0);
			b=retrieveOperandValue(zOp, 1);
			storeVariable(zOp.storeInfo.storeVar, (a&b));
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of TEST_ATTR instruction
	int TEST_ATTR(ZOpcode& zOp){
		//test_attr object attribute ?(label)
		//Jump if object has attribute. 
		try{
			zword object=retrieveOperandValue(zOp, 0);
			ulong attrib=retrieveOperandValue(zOp, 1);
			ulong objectAttrib;
			try{
				objectAttrib=zObject->getObjectAttributeFlags32(object);
			}catch(...){
				objectAttrib=NULL;
			}
			bool cond=(objectAttrib & (1<<(31-attrib)));
			if(zOp.branchInfo.branchCond==cond){
				// jump
				if(zOp.branchInfo.branchOffset==0){
					// return false
					routineReturn(zOp, 0);
				}else if(zOp.branchInfo.branchOffset==1){
					// return true
					routineReturn(zOp, 1);
				}else{
					*jumpFlag=JUMP_OFFSET;
					*jumpValue=zOp.branchInfo.branchOffset;
				}
			}else{
				*jumpFlag=JUMP_NONE;
			}
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of SET_ATTR instruction
	int SET_ATTR(ZOpcode& zOp){
		try{
			zword object=retrieveOperandValue(zOp, 0);
			zword attrib=retrieveOperandValue(zOp, 1);
			zword objectAttrib=zObject->getObjectAttributeFlags32(object);
			zObject->setObjectAttributeFlags32(object, objectAttrib|(1<<(32-attrib)));
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of CLEAR_ATTR instruction
	int CLEAR_ATTR(ZOpcode& zOp){
		try{
			zword object=retrieveOperandValue(zOp, 0);
			zword attrib=retrieveOperandValue(zOp, 1);
			zword objectAttrib=zObject->getObjectAttributeFlags32(object);
			zObject->setObjectAttributeFlags32(object, objectAttrib & (0xFFFFFFFF^(1<<(31-attrib))));
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of STORE instruction
	int STORE(ZOpcode& zOp){
		try{
			zword value=retrieveOperandValue(zOp, 1);
			storeVariable(retrieveOperandValue(zOp, 0), value);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of INSERT_OBJ instruction
	int INSERT_OBJ(ZOpcode& zOp){
		try{
			// insert_obj object destination
			zword object=retrieveOperandValue(zOp, 0);
			zword destination=retrieveOperandValue(zOp, 1);
			// "object" becomes the direct child of
			// "destination", and the original child of 
			// "destination" becomes the sibling of "object"
			zword origChild=zObject->getObjectChild(destination);
			zObject->setObjectChild(destination, object);
			zObject->setObjectSibling(object, origChild);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of LOADW instruction
	int LOADW(ZOpcode& zOp){
		try{
			// loadw array, word-index -> (result)
			// loads word at addr (array+(2*word-index)) into
			// result
			zword arr=retrieveOperandValue(zOp, 0);
			zword wordIndex=retrieveOperandValue(zOp, 1);
			zword word=zMemory->readZWord(arr+(2*wordIndex));
			storeVariable(zOp.storeInfo.storeVar, word);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of LOADB instruction
	int LOADB(ZOpcode& zOp){
		try{
			// loadw array, byte-index -> (result)
			// loads byte at addr (array+byte-index) into
			// result
			zword arr=retrieveOperandValue(zOp, 0);
			zword byteIndex=retrieveOperandValue(zOp, 1);
			zbyte byte=zMemory->readZWord(arr+byteIndex);
			storeVariable(zOp.storeInfo.storeVar, byte);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of GET_PROP instruction
	int GET_PROP(ZOpcode& zOp){
		try{
			// get_prop object, property -> (result)
			// reads property from object (property==default value if no such property)
			// if (property_length == 1), value is only that byte.
			// if (property_length == 2), the first two bytes are taken as a zword
			// it is illegal to use this instruction if the property length is greater than 2
			zword object=retrieveOperandValue(zOp, 0);
			zword prop=retrieveOperandValue(zOp, 1);
			zword propVal=NULL;
			ulong propListAddr=zObject->getObjectPropertyListAddr(object);
			// iterate through the property table
			for(int i=propListAddr; ;){
				zbyte propNumber=zObject->getPropertyNumber(i);
				// since property list is stored in descending numerical order\
				// if propNumber < prop, prop doesn't exist
				if(propNumber < prop){
					propVal=zObject->getDefaultProperty(prop);
					break;
				}
				if(propNumber==prop){
					// found the correct property
					if(zVersion<=3){
						zword propSize=zObject->getPropertySize(i);
						if(propSize==1){
							propVal=zMemory->readZByte(i+1);
						}else if(propSize==2){
							propVal=zMemory->readZWord(i+1);
						}else{
							propVal=zObject->getDefaultProperty(prop);
						}
					}else{
						bool isWord=false;
						zword propSize=zObject->getPropertySize(i, isWord);
						int add=isWord ? 2 : 1;	// add is the offset needed to skip the size byte
						if(propSize==1){
							propVal=zMemory->readZByte(i+add+1);
						}else if(propSize==2){
							propVal=zMemory->readZWord(i+add+1);
						}else{
							propVal=zObject->getDefaultProperty(prop);
						}
					}
					break;
				}
				if(zVersion<=3){
					i+=zObject->getPropertySize(i)+1;
				}else{
					bool isWord=false;
					int propSize=zObject->getPropertySize(i, isWord);
					i+=isWord ? propSize+2 : propSize+1;
				}
			}
			storeVariable(zOp.storeInfo.storeVar, propVal);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of GET_PROP_ADDR
	int GET_PROP_ADDR(ZOpcode& zOp){
		try{
			// get_prop_addr object property -> (result)
			// returns byte addr of property data for given object
			// but returns 0 if the object doesn't have it
			zword object=retrieveOperandValue(zOp, 0);
			zword prop=retrieveOperandValue(zOp, 1);
			zword propAddr;
			ulong propertyListAddr=zObject->getObjectPropertyListAddr(object);
			// iterate through the property list
			for(int i=propertyListAddr; ;){
				zbyte propNumber=zObject->getPropertyNumber(i);
				// since property list is stored in descending numerical order\
				// if propNumber < prop, prop doesn't exist
				if(propNumber < prop){
					// return 0
					propAddr=NULL;
					break;
				}
				if(propNumber==prop){
					// found property
					propAddr=i;
					break;
				}
				if(zVersion<=3){
					i+=zObject->getPropertySize(i)+1;
				}else{
					bool isWord=false;
					int propSize=zObject->getPropertySize(i, isWord);
					i+=isWord ? propSize+2 : propSize+1;
				}
			}
			storeVariable(zOp.storeInfo.storeVar, propAddr);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of GET_NEXT_PROP instruction
	int GET_NEXT_PROP(ZOpcode& zOp){
		try{
			// get_next_prop object property -> (result)
			// gives number of next property
			// number may be 0. this indicates that that the
			// specified property was the last in the list.
			// if called with 0, gives the first property number
			// it is illegal to find the next property of a property 
			// which does not exist
			zword object=retrieveOperandValue(zOp, 0);
			zword prop=retrieveOperandValue(zOp, 1);
			ulong propertyListAddr=zObject->getObjectPropertyListAddr(object);
			zbyte nextPropNumber=0;
			// iterate through the property list
			bool foundProp=false;
			for(int i=propertyListAddr;;){
				zbyte propNumber=zObject->getPropertyNumber(i);
				if(foundProp){
					// last iteration found the correct property
					// this iteration returns the next property number
					nextPropNumber=propNumber;
					break;
				}
				// since property list is stored in descending numerical order\
				// if propNumber < prop, prop doesn't exist
				if(propNumber < prop){
					// throw error
					throw IllegalZOpcode();
					break;
				}
				if(propNumber==prop){
					// found property
					foundProp=true;
					break;
				}
				// add the size of current property
				if(zVersion<=3){
					i+=zObject->getPropertySize(i)+1;
				}else{
					bool isWord=false;
					int propSize=zObject->getPropertySize(i, isWord);
					i+=isWord ? propSize+2 : propSize+1;
				}
			}
			storeVariable(zOp.storeInfo.storeVar, nextPropNumber);
		}catch(IllegalPropertyIndex e){
			throw IllegalZOpcode();
		}
	}

	// implementation of ADD instruction
	int ADD(ZOpcode& zOp){
		//add a b -> (result)
		//Signed 16-bit addition.
		try{
			szword a=retrieveOperandValue(zOp, 0);
			szword b=retrieveOperandValue(zOp, 1);
			storeVariable(zOp.storeInfo.storeVar, (a+b));
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of SUB instruction
	int SUB(ZOpcode& zOp){
		//sub a b -> (result)
		//Signed 16-bit subtraction.
		try{
			szword a=retrieveOperandValue(zOp, 0);
			szword b=retrieveOperandValue(zOp, 1);
			storeVariable(zOp.storeInfo.storeVar, (a-b));
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of MUL instruction
	int MUL(ZOpcode& zOp){
		//mul a b -> (result)
		//Signed 16-bit multiplication.
		try{
			szword a=retrieveOperandValue(zOp, 0);
			szword b=retrieveOperandValue(zOp, 1);
			storeVariable(zOp.storeInfo.storeVar, (a*b));
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of DIV instruction
	int DIV(ZOpcode& zOp){
		try{
			szword a=retrieveOperandValue(zOp, 0);
			szword b=retrieveOperandValue(zOp, 1);
			if(!b){
				// division by 0
				throw IllegalZOpcode();
			}
			storeVariable(zOp.storeInfo.storeVar, (a/b));
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of MOD instruction
	int MOD(ZOpcode& zOp){
		// mod a b -> (result)
		try{
			szword a=retrieveOperandValue(zOp, 0);
			szword b=retrieveOperandValue(zOp, 1);
			if(!b){
				// division by 0
				throw IllegalZOpcode();
			}
			storeVariable(zOp.storeInfo.storeVar, (a%b));
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of CALL_2S
	int CALL_2S(ZOpcode& zOp){
		// call_2s routine arg1 -> (result)
		// Stores routine(arg1).
		try{
			ulong routineAddr=retrieveOperandValue(zOp, 0);
			routineAddr=zMemory->unpackAddr(routineAddr);
			zword arg1=retrieveOperandValue(zOp, 1);
			// from CALL()
			// enter into the routine
			routineEnter(zOp, routineAddr);
			cpuObj->currentRoutine->retValueDest=(zOp.storeInfo.storeVar);
			cpuObj->currentRoutine->keepRetValue=true;
			// copy arg1 into local_var1
			cpuObj->currentRoutine->storeLocalVar(*zStack, 1, arg1);
			// jump to routine
			*jumpFlag=JUMP_POSITION;
			*jumpValue=cpuObj->currentRoutine->codeStartAddr;
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of CALL_2N instruction
	int CALL_2N(ZOpcode& zOp){
		try{
			//call_2n routine arg1
			//Executes routine(arg1) and throws away result.
			ulong routineAddr=retrieveOperandValue(zOp, 0);
			routineAddr=zMemory->unpackAddr(routineAddr);
			zword arg1=retrieveOperandValue(zOp, 1);
			// from CALL()
			// enter into the routine
			routineEnter(zOp, routineAddr);
			// set flag to throw away return value
			cpuObj->currentRoutine->keepRetValue=false;
			// copy arg1 into local_var1
			cpuObj->currentRoutine->storeLocalVar(*zStack, 1, arg1);
			// jump to routine
			*jumpFlag=JUMP_POSITION;
			*jumpValue=cpuObj->currentRoutine->codeStartAddr;
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of SET_COLOUR instruction
	int SET_COLOUR_FB(ZOpcode& zOp){
		try{
			//set_colour foreground background
			//[Version 6] set_colour foreground background window
			zword fg=retrieveOperandValue(zOp, 0);
			zword bg=retrieveOperandValue(zOp, 1);
			// TODO
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of THROW instruction
	int THROW(ZOpcode& zOp){
		try{
			// throw value stack-frame
			zword value=retrieveOperandValue(zOp, 0);
			zword frame=retrieveOperandValue(zOp, 1);
			// step backwards through the stack frame
			for(int i=cpuObj->stackFrame.getSize()-1, j=0; i>=0; i--, j++){
				if(frame==cpuObj->stackFrame.readEntry(i)){
					// found the stack frame
					// unwind the stack frame
					for(int k=0; k<j; k++){
						cpuObj->stackFrame.pullEntry();
					}
					// unwind the routines list
					for(int k=0; k<j; k++){
						ZCpuRoutine* parentRoutine=cpuObj->currentRoutine->parentRoutine;
						delete cpuObj->currentRoutine;
						cpuObj->currentRoutine=parentRoutine;
					}
					// return (as if from that frame) with the given value
					zStack->setStackPtr(cpuObj->stackFrame.readEntry(i));	// reset stack
					routineReturn(zOp, value);								// return
				}
			}
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of JZ instruction
	int JZ(ZOpcode& zOp){
		try{
			// jz a ?(label)
			zword a=retrieveOperandValue(zOp, 0);
			bool cond=(!a);
			if(cond==zOp.branchInfo.branchCond){
				// branch
				if(zOp.branchInfo.branchOffset==0){
					// return false
					routineReturn(zOp, 0);
				}else if(zOp.branchInfo.branchOffset==1){
					// return true
					routineReturn(zOp, 1);
				}else{
					*jumpFlag=JUMP_OFFSET;
					*jumpValue=zOp.branchInfo.branchOffset;
				}
			}else{
				*jumpFlag=JUMP_NONE;
			}
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of GET_SIBLING instruction
	int GET_SIBLING(ZOpcode& zOp){
		try{
			//get_sibling object -> (result) ?(label)
			//Get next object in tree, branching if this exists, i.e. is not 0.
			zword object=retrieveOperandValue(zOp, 0);
			zword sibling=zObject->getObjectSibling(object);
			// store
			storeVariable(zOp.storeInfo.storeVar, sibling);
			bool cond=(sibling);
			if(cond==zOp.branchInfo.branchCond){
				if(zOp.branchInfo.branchOffset==0){
					// return false
					routineReturn(zOp, 0);
				}else if(zOp.branchInfo.branchOffset==1){
					// return true
					routineReturn(zOp, 1);
				}else{
					*jumpFlag=JUMP_OFFSET;
					*jumpValue=zOp.branchInfo.branchOffset;
				}
			}else{
				*jumpFlag=JUMP_NONE;
			}
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of GET_CHILD instruction
	int GET_CHILD(ZOpcode& zOp){
		try{
			//get_child object -> (result) ?(label)
			//Get first object contained in given object, branching if this exists, i.e. is not nothing (i.e., is not 0).
			zword object=retrieveOperandValue(zOp, 0);
			zword child=zObject->getObjectChild(object);
			// store
			storeVariable(zOp.storeInfo.storeVar, child);
			bool cond=(child);
			if(cond==zOp.branchInfo.branchCond){
				if(zOp.branchInfo.branchOffset==0){
					// return false
					routineReturn(zOp, 0);
				}else if(zOp.branchInfo.branchOffset==1){
					// return true
					routineReturn(zOp, 1);
				}else{
					*jumpFlag=JUMP_OFFSET;
					*jumpValue=zOp.branchInfo.branchOffset;
				}
			}else{
				*jumpFlag=JUMP_NONE;
			}
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of GET_PARENT instruction
	int GET_PARENT(ZOpcode& zOp){
		try{
			//get_parent object -> (result)
			//Get parent object (note that this has no "branch if exists" clause).
			zword object=retrieveOperandValue(zOp, 0);
			zword parent=zObject->getObjectParent(object);
			storeVariable(zOp.storeInfo.storeVar, parent);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of GET_PROP_LEN instruction
	int GET_PROP_LEN(ZOpcode& zOp){
		try{
			//get_prop_len property-address -> (result)
			zword propAddr=retrieveOperandValue(zOp, 0);
			zword propLen;
			if(zVersion<=3){
				propLen=zObject->getPropertySize(propAddr);
			}else{
				bool wordFlag;
				propLen=zObject->getPropertySize(propAddr, wordFlag);
			}
			storeVariable(zOp.storeInfo.storeVar, propLen);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of INC instruction
	int INC(ZOpcode& zOp){
		try{
			//inc (variable)
			//Increment variable by 1. (This is signed, so -1 increments to 0.)
			szword a=retrieveOperandValue(zOp, 0);
			storeVariable(zOp.getOperands()[0], a--);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of DEC instruction
	int DEC(ZOpcode& zOp){
		try{
			//inc (variable)
			//Increment variable by 1. (This is signed, so -1 increments to 0.)
			szword a=retrieveOperandValue(zOp, 0);
			storeVariable(zOp.getOperands()[0], a--);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of PRINT_ADDR instruction
	int PRINT_ADDR(ZOpcode& zOp){
		try{
			//print_addr byte-address-of-string
			//Print (Z-encoded) string at given byte address, in dynamic or static memory.
			zword addr=retrieveOperandValue(zOp, 0);
			zchar* outString=zCharStringtoZSCII((ulong)addr, *zMemory);
			zInOut->print((char*)outString);
			delete[] outString;
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of CALL_1S instruction
	int CALL_1S(ZOpcode& zOp){
		try{
			//call_1s routine -> (result)
			//Stores routine(). 
			// code taken from
			ulong routineAddr=retrieveOperandValue(zOp, 0);
			routineAddr=zMemory->unpackAddr(routineAddr);
			zword arg1=retrieveOperandValue(zOp, 1);
			// from CALL()
			// enter into the routine
			routineEnter(zOp, routineAddr);
			// set flag to throw away return value
			cpuObj->currentRoutine->keepRetValue=true;
			cpuObj->currentRoutine->retValueDest=zOp.storeInfo.storeVar;
			// jump to routine
			*jumpFlag=JUMP_POSITION;
			*jumpValue=cpuObj->currentRoutine->codeStartAddr;
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of REMOVE_OBJ instruction
	int REMOVE_OBJ(ZOpcode& zOp){
		//remove_obj object
		//Detach the object from its parent, so that it no longer has any parent
		try{
			zword object=retrieveOperandValue(zOp, 0);
			// now we set parent as 0
			zObject->setObjectParent(object, 0);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of PRINT_OBJ instruction
	int PRINT_OBJ(ZOpcode& zOp){
		// print short name of object
		try{
			zword object=retrieveOperandValue(zOp, 0);
			zword* name=zObject->getObjectName(object);
			zchar* outString=zCharStringtoZSCII(name, *zMemory);
			zInOut->print((char*)outString);
			delete[] outString;
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of RET instruction
	int RET(ZOpcode& zOp){
		//ret value
		try{
			int value=zOp.getOperands()[0];
			routineReturn(zOp, value);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of JUMP instruction
	int JUMP(ZOpcode& zOp){
		// jump ? (label)
		// unconditional jump
		try{
			zword offset=retrieveOperandValue(zOp, 0);
			if(offset==0){
				// return false
				routineReturn(zOp, 0);
			}else if(offset==1){
				// return true
				routineReturn(zOp, 1);
			}else{
				*jumpFlag=JUMP_OFFSET;
				*jumpValue=offset;
			}
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of PRINT_PADDR instruction
	int PRINT_PADDR(ZOpcode& zOp){
		// print_paddr packed-address-of-string
		try{
			zword paddr=retrieveOperandValue(zOp, 0);
			ulong addr=zMemory->unpackAddr(paddr);
			zchar* outString=zCharStringtoZSCII(addr, *zMemory);
			zInOut->print((char*)outString);
			delete[] outString;
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of LOAD instruction
	int LOAD(ZOpcode& zOp){
		try{
			// load (variable) -> result
			storeVariable(zOp.storeInfo.storeVar, retrieveOperandValue(zOp, 0));
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of NOT instruction
	int NOT(ZOpcode& zOp){
		try{
			// not (value) -> result
			zword value=retrieveOperandValue(zOp, 0);
			storeVariable(zOp.storeInfo.storeVar, ~value);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of CALL_1N instruction
	int CALL_1N(ZOpcode& zOp){
		try{
			//call_1s routine -> (result)
			//Stores routine(). 
			// code taken from
			ulong routineAddr=retrieveOperandValue(zOp, 0);
			routineAddr=zMemory->unpackAddr(routineAddr);
			zword arg1=retrieveOperandValue(zOp, 1);
			// from CALL()
			// enter into the routine
			routineEnter(zOp, routineAddr);
			// set flag to throw away return value
			cpuObj->currentRoutine->keepRetValue=false;
			// jump to routine
			*jumpFlag=JUMP_POSITION;
			*jumpValue=cpuObj->currentRoutine->codeStartAddr;
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of RTRUE instruction
	int RTRUE(ZOpcode& zOp){
		try{
			routineReturn(zOp, 1);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of RFALSE instruction
	int RFALSE(ZOpcode& zOp){
		try{
			routineReturn(zOp, 0);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of PRINT instruction
	int PRINT(ZOpcode& zOp){
		try{
			zchar* outString=zCharStringtoZSCII(zOp.getOpcodeString(), *zMemory);
			zInOut->print((char*)outString);
			delete[] outString;
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of PRINT_RET instruction
	int PRINT_RET(ZOpcode& zOp){
		try{
			zchar* outString=zCharStringtoZSCII(zOp.getOpcodeString(), *zMemory);
			zInOut->print((char*)outString);
			delete[] outString;
			// return true
			routineReturn(zOp, 1);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of NOP instruction
	int NOP(ZOpcode& zOp){
		// no operation
		return NULL;
	}

	// implementation of SAVE instruction
	int SAVE_LABEL(ZOpcode& zOp){
		try{
			/** TODO **/
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	int SAVE_RESULT(ZOpcode& zOp){
		try{
			/** TODO **/
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of RESTART instruction
	int RESTART(ZOpcode& zOp){
		try{
			/** TODO **/
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of RET_POPPED instruction
	int RET_POPPED(ZOpcode& zOp){
		try{
			zword value=zStack->pull();
			routineReturn(zOp, value);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of POP instruction
	int POP(ZOpcode& zOp){
		try{
			zStack->pull();
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of CATCH instruction
	int CATCH(ZOpcode& zOp){
		try{
			zword currentFrame=cpuObj->stackFrame.readEntry(cpuObj->stackFrame.getSize()-1);
			storeVariable(zOp.storeInfo.storeVar, currentFrame);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of QUIT instruction
	int QUIT(ZOpcode& zOp){
		try{
			cpuObj->haltFlag=true;	// halt execution
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of NEW_LINE instruction
	int NEW_LINE(ZOpcode& zOp){
		try{
			zInOut->print("\n");
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of SHOW_STATUS instruction
	int SHOW_STATUS(ZOpcode& zOp){
		try{
			/** todo **/
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of VERIFY instruction
	int VERIFY(ZOpcode& zOp){
		try{
			/*	calculate a checksum of the story file
				starting from offset 0x40, modulo 0x10000.
				then compares with the value in the game 
				header, branching if the two values agree
				*/
			zword checkSum=0;
			for(int i=0x40; i<(zMemory->getMemSize()-0x40); i++){
				checkSum+=zMemory->readZByte(i);
			}
			zword headerSum=zMemory->readZWord(0x1c);	// get checksum in header
			bool cond=(checkSum==headerSum);
			if(cond==zOp.branchInfo.branchCond){
				if(zOp.branchInfo.branchOffset==0){
					// return false
					routineReturn(zOp, 0);
				}else if(zOp.branchInfo.branchOffset==1){
					// return true
					routineReturn(zOp, 1);
				}else{
					*jumpFlag=JUMP_OFFSET;
					*jumpValue=zOp.branchInfo.branchOffset;
				}
			}else{
				*jumpFlag=JUMP_NONE;
			}
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of PIRACY instruction
	int PIRACY(ZOpcode& zOp){
		try{
			// branch unconditionally, like any gullible interpreter
			if(zOp.branchInfo.branchOffset==0){
				// return false
				routineReturn(zOp, 0);
			}else if(zOp.branchInfo.branchOffset==1){
				// return true
				routineReturn(zOp, 1);
			}else{
				*jumpFlag=JUMP_OFFSET;
				*jumpValue=zOp.branchInfo.branchOffset;
			}
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of CALL instruction
	int CALL(ZOpcode& zOp){
		//call routine ...up to 3 args... -> (result)
		try{
			if(zOp.getOperandCount()<2){
				// minimum 2 operands
				throw IllegalZOpcode();
			}
			if(!zOp.getOperands()[0]){
				// if addr==0, do nothing and set return value
				// to false
				int returnVar=zOp.storeInfo.storeVar;;	// retrieve last operand
				storeVariable(returnVar, NULL);	// null == false
				return 0;
			}
			// otherwise, enter into the routine
			// setting the routine's local values to the
			// given arguments, starting from local #0
 			vector<int> routineArgs(0);
			for(int i=1; i<zOp.getOperands().size(); i++){
				routineArgs.push_back(retrieveOperandValue(zOp, i));
			}
			// now that we have the operands, we may begin to
			// enter into the routine
			routineEnter(zOp, zMemory->unpackAddr(retrieveOperandValue(zOp, 0)));
			// load arguments into local vars
			for(int i=1; i<routineArgs.size()+1 && i<cpuObj->currentRoutine->localCount+1; i++){
				cpuObj->currentRoutine->storeLocalVar(*zStack, i, routineArgs[i-1]);
			}
			// load return value destination
			cpuObj->currentRoutine->retValueDest=zOp.storeInfo.storeVar;
			cpuObj->currentRoutine->keepRetValue=true;
			// jump to routine
			*jumpFlag=JUMP_POSITION;
			*jumpValue=cpuObj->currentRoutine->codeStartAddr;
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of CALL_VS opcode
	int CALL_VS(ZOpcode& zOp){
		try{
			// same as CALL
			CALL(zOp);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of STOREW opcode
	int STOREW(ZOpcode& zOp){
		try{
			// storew array word-index value
			// array[word-index]=value
			zword arr=retrieveOperandValue(zOp, 0);
			zword index=retrieveOperandValue(zOp, 1);
			zword value=retrieveOperandValue(zOp, 2);
			zMemory->storeZWord(arr+(2*index), value);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of STOREB opcode
	int STOREB(ZOpcode& zOp){
		try{
			// storeb array byte-index value
			// array[byte-index]=value
			zword arr=retrieveOperandValue(zOp, 0);
			zword index=retrieveOperandValue(zOp, 1);
			zword value=retrieveOperandValue(zOp, 2);
			zMemory->storeZByte(arr+index, value);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of PUT_PROP opcode
	int PUT_PROP(ZOpcode& zOp){
		try{
			// put_prop object property value
			zword object=retrieveOperandValue(zOp, 0);
			zword prop=retrieveOperandValue(zOp, 1);
			zword value=retrieveOperandValue(zOp, 2);
			// if prop_size==1, store the least significant byte of value
			// it is illegal for this opcode to be used on a property
			// that does not exist
			ulong propListAddr=zObject->getObjectPropertyListAddr(object);
			// iterate through the property table
			for(int i=propListAddr; ;){
				zbyte propNumber=zObject->getPropertyNumber(i);
				// since property list is stored in descending numerical order\
				// if propNumber < prop, prop doesn't exist
				if(propNumber < prop){
					// illegal : property not present
					throw IllegalZOpcode();
					break;
				}
				if(propNumber==prop){
					// found the correct property
					if(zVersion<=3){
						zword propSize=zObject->getPropertySize(i);
						if(propSize==1){
							zMemory->storeZByte(i+1, (zbyte)value&255);
						}else if(propSize==2){
							zMemory->storeZWord(i+1, value);
						}else{
							throw IllegalZOpcode();
						}
						break;
					}else{
						bool isWord=false;
						zword propSize=zObject->getPropertySize(i, isWord);
						int add=isWord ? 2 : 1;	// add is the offset needed to skip the size byte
						if(propSize==1){
							zMemory->storeZByte(i+add+1, (zbyte)value&255);
						}else if(propSize==2){
							zMemory->storeZWord(i+add+1, value);
						}else{
							throw IllegalZOpcode();
						}
						break;
					}
				}
				if(zVersion<=3){
					i+=zObject->getPropertySize(i)+1;
				}else{
					bool isWord=false;
					int propSize=zObject->getPropertySize(i, isWord);
					i+=isWord ? propSize+2 : propSize+1;
				}
			}
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of READ opcode
	int READ(ZOpcode& zOp){
		try{
			// read has 3 variants :
			// sread text, parse (V1 onwards)
			// sread text parse time routine (V4 onwards)
			// aread text parse time routine -> (result) (V5 onwards)
			// reads a whole command from keyboard
			enum{SREAD_TP, SREAD_TPTR, AREAD};
			int opType=NULL;
			if(zOp.storeInfo.hasStore){
				// is AREAD
				opType=AREAD;
			}else{
				// determine type based on argument count
				int i=0;	// arg count
				for(i=0; \
					i<zOp.getOperandTypes().size() && zOp.getOperandTypes()[i]!=ZOPERANDTYPE_OMITTED; \
					i++){
				}
				if(i==2){
					opType=SREAD_TP;
				}else if(i==4){
					opType=SREAD_TPTR;
				}else{
					throw IllegalZOpcode();
				}
			}
			// now that we have determined the opcode type
			if(opType==SREAD_TP){
				if(zVersion<=3){
					// the status line is automatically redisplayed first
				}
				// read chars until CRLF
				// or in v5, any terminating char
				zword textBuffer=retrieveOperandValue(zOp, 0);
				zword parseBuffer=retrieveOperandValue(zOp, 1);
				zbyte maxChars=NULL;
				maxChars = zMemory->readZByte(textBuffer+0);
				int beginPos=(zVersion<=4) ? 1 : 2;
				// begin reading chars
				char* text=zInOut->readLine();
				for(int i=0; text[i]!=NULL; i++){
					if(isupper(text[i]))
						text[i]=tolower(text[i]);
				}
				if(zVersion>=5){
					// 5 and onwards, write sizeof text to byte 1
					int len=strlen(text);
					len=(len<maxChars) ? len : maxChars;
					zMemory->storeZByte(textBuffer+1, len);
				}
				// begin copy
				{
					int i=0;
					do{
						if(zVersion>=5 && text[i]==NULL) break;	// don't copy NULL char if v5+
						zMemory->storeZByte(textBuffer+beginPos+i, text[i]);
						i++;
					}while(text[i]!=NULL && i<maxChars);
				}
				// done
				// now perform lexical analysis
				if(!(zVersion>=5 && parseBuffer==NULL)){
					// if v5+ and parseBuffer=NULL, this is skipped
					// but if not, continue
					int maxWords=zMemory->readZByte(parseBuffer+0);	// byte 0 of parse-buffer has max capacity
					// therefore the buffer must be at least 2+4n bytes long to hold n words
					ZDictionaryParseTable parseTable=zDictionary->performLexicalAnalysis((zchar*)text);
					parseTable.trimTable(maxWords);	// trim to size
					// write parse table
					parseTable.writeParseBuffer(parseBuffer, *zMemory);
				}
			}else if(opType==SREAD_TPTR){

			}else if(opType==AREAD){

			}
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of PRINT_CHAR opcode
	int PRINT_CHAR(ZOpcode& zOp){
		try{
			// print_char output-character-code
			zword charCode=retrieveOperandValue(zOp, 0);
			char* str=new char[2];
			str[0]=(char)charCode;
			str[1]=NULL;
			zInOut->print((char*)str);
			delete[] str;
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of PRINT_NUM opcode
	int PRINT_NUM(ZOpcode& zOp){
		try{
			zword num=retrieveOperandValue(zOp, 0);
			zInOut->print(IntegerToDecASCII((int)num));
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of RANDOM opcode
	int RANDOM(ZOpcode& zOp){
		try{

		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of PUSH opcode
	int PUSH(ZOpcode& zOp){
		try{
			// push value
			zword value=retrieveOperandValue(zOp, 0);
			zStack->push(value);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of PULL opcode
	int PULL(ZOpcode& zOp){
		try{
			// pull (variable)
			// pull stack -> (result) (v6)
			if(zVersion<6){
				zword var=zOp.getOperands()[0];
				storeVariable(var, zStack->pull());
			}
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of SPLIT_WINDOW opcode
	int SPLIT_WINDOW(ZOpcode& zOp){
		try{
			// todo
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of SET_WINDOW opcode
	int SET_WINDOW(ZOpcode& zOp){
		try{

		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of CALL_VS2 opcode
	int CALL_VS2(ZOpcode& zOp){
		try{
			// special case : up to 8 arguments
			//  call_vs2 routine ...up to 7 args... -> (result)
			if(zOp.getOperandCount()<2){
				// minimum 2 operands
				throw IllegalZOpcode();
			}
			if(!zOp.getOperands()[0]){
				// if addr==0, do nothing and set return value
				// to false
				int returnVar=zOp.storeInfo.storeVar;;	// retrieve last operand
				storeVariable(returnVar, NULL);	// null == false
				return 0;
			}
			// otherwise, enter into the routine
			// setting the routine's local values to the
			// given arguments, starting from local #0
			vector<int> routineArgs(0);
			for(int i=1; i<zOp.getOperands().size()-1; i++){
				routineArgs.push_back(retrieveOperandValue(zOp, i));
			}
			// now that we have the operands, we may begin to
			// enter into the routine
			routineEnter(zOp, zMemory->unpackAddr(retrieveOperandValue(zOp, 0)));
			// load arguments into local vars
			for(int i=0; i<routineArgs.size() && i< cpuObj->currentRoutine->localCount; i++){
				cpuObj->currentRoutine->storeLocalVar(*zStack, i, routineArgs[i]);
			}
			// load return value destination
			cpuObj->currentRoutine->retValueDest=zOp.storeInfo.storeVar;
			cpuObj->currentRoutine->keepRetValue=true;
			// jump to routine
			*jumpFlag=JUMP_POSITION;
			*jumpValue=cpuObj->currentRoutine->codeStartAddr;
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of ERASE_WINDOW opcode
	int ERASE_WINDOW(ZOpcode& zOp){
		try{
			// todo
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of ERASE_LINE opcode
	int ERASE_LINE(ZOpcode& zOp){
		try{
			// todo
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of SET_CURSOR opcode
	int SET_CURSOR(ZOpcode& zOp){
		try{
			// todo
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of GET_CURSOR opcode
	int GET_CURSOR(ZOpcode& zOp){
		try{

		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of SET_TEXT_STYLE opcode
	int SET_TEXT_STYLE(ZOpcode& zOp){
		try{

		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of BUFFER_MODE opcode
	int BUFFER_MODE(ZOpcode& zOp){
		try{

		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of OUTPUT_STREAM opcode
	int OUTPUT_STREAM_N(ZOpcode& zOp){
		try{

		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of INPUT_STREAM opcode
	int INPUT_STREAM(ZOpcode& zOp){
		try{

		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of SOUND_EFFECT opcode
	int SOUND_EFFECT(ZOpcode& zOp){
		try{

		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of READ_CHAR opcode
	int READ_CHAR(ZOpcode& zOp){
		try{
			// read_char 1 time routine -> (result)
			// todo : process time and routine
			// currently just returning char
			zword var1=retrieveOperandValue(zOp, 0);
			if(var1!=1){
				throw IllegalZOpcode();
			}
			// otherwise
			char result=zInOut->getChar();
			storeVariable(zOp.storeInfo.storeVar, result);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of SCAN_TABLE opcode
	int SCAN_TABLE(ZOpcode& zOp){
		try{
			//scan_table x table len form -> (result)
			zword x=retrieveOperandValue(zOp, 0);
			zword table=retrieveOperandValue(zOp, 1);
			zword len=retrieveOperandValue(zOp, 2);
			bool hasForm=false;
			zword form=0;
			if(zOp.getOperands().size()==4){
				// form operand is present
				form=retrieveOperandValue(zOp, 3);
				hasForm=true;
			}
			// we do not process form, as this interpreter
			// is not designed to operationally support v5
			// as yet
			for(int i=0; i<len; i++){
				zword v=zMemory->readZWord(table+(2*len));
				if(v==x){
					// found it!
					storeVariable(zOp.storeInfo.storeVar, v);
					// and branch
					if(zOp.branchInfo.branchOffset==0){
						// return false
						routineReturn(zOp, 0);
					}else if(zOp.branchInfo.branchOffset==1){
						// return true
						routineReturn(zOp, 1);
					}else{
						*jumpFlag=JUMP_OFFSET;
						*jumpValue=zOp.branchInfo.branchOffset;
					}
				}
			}
			// return 0 and don't branch
			storeVariable(zOp.storeInfo.storeVar, 0);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of NOT opcode (var)
	int NOT_V(ZOpcode& zOp){
		try{
			// same as NOT
			NOT(zOp);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of CALL_VN opcode
	int CALL_VN(ZOpcode& zOp){
		try{
			if(zOp.getOperandCount()<2){
				// minimum 2 operands
				throw IllegalZOpcode();
			}
			if(!zOp.getOperands()[0]){
				// if addr==0, do nothing and set return value
				// to false
				int returnVar=zOp.storeInfo.storeVar;;	// retrieve last operand
				storeVariable(returnVar, NULL);	// null == false
				return 0;
			}
			// otherwise, enter into the routine
			// setting the routine's local values to the
			// given arguments, starting from local #0
			vector<int> routineArgs(0);
			for(int i=1; i<zOp.getOperands().size()-1; i++){
				routineArgs.push_back(retrieveOperandValue(zOp, i));
			}
			// now that we have the operands, we may begin to
			// enter into the routine
			routineEnter(zOp, zMemory->unpackAddr(retrieveOperandValue(zOp, 0)));
			// load arguments into local vars
			for(int i=0; i<routineArgs.size() && i< cpuObj->currentRoutine->localCount; i++){
				cpuObj->currentRoutine->storeLocalVar(*zStack, i, routineArgs[i]);
			}
			// load return value destination
			cpuObj->currentRoutine->keepRetValue=false;
			// jump to routine
			*jumpFlag=JUMP_POSITION;
			*jumpValue=cpuObj->currentRoutine->codeStartAddr;
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of CALL_VN2 opcode
	int CALL_VN2(ZOpcode& zOp){
		try{
			// special case : up to 8 arguments
			//  call_vs2 routine ...up to 7 args... -> (result)
			if(zOp.getOperandCount()<2){
				// minimum 2 operands
				throw IllegalZOpcode();
			}
			if(!zOp.getOperands()[0]){
				// if addr==0, do nothing and set return value
				// to false
				int returnVar=zOp.storeInfo.storeVar;;	// retrieve last operand
				storeVariable(returnVar, NULL);	// null == false
				return 0;
			}
			// otherwise, enter into the routine
			// setting the routine's local values to the
			// given arguments, starting from local #0
			vector<int> routineArgs(0);
			for(int i=1; i<zOp.getOperands().size()-1; i++){
				routineArgs.push_back(retrieveOperandValue(zOp, i));
			}
			// now that we have the operands, we may begin to
			// enter into the routine
			routineEnter(zOp, zMemory->unpackAddr(retrieveOperandValue(zOp, 0)));
			// load arguments into local vars
			for(int i=0; i<routineArgs.size() && i< cpuObj->currentRoutine->localCount; i++){
				cpuObj->currentRoutine->storeLocalVar(*zStack, i, routineArgs[i]);
			}
			// load return value destination
			cpuObj->currentRoutine->keepRetValue=false;
			// jump to routine
			*jumpFlag=JUMP_POSITION;
			*jumpValue=cpuObj->currentRoutine->codeStartAddr;
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of TOKENISE opcode
	int TOKENISE(ZOpcode& zOp){
		try{
			// tokenise text parse dictionary flag
			// currently only supports using the game dictionary
			zword text=retrieveOperandValue(zOp, 0);
			zword parse=retrieveOperandValue(zOp, 1);
			zword dict=retrieveOperandValue(zOp, 2);
			zword flag=retrieveOperandValue(zOp, 3);
			int maxWords=zMemory->readZByte(parse+0);	// byte 0 of parse-buffer has max capacity
			// therefore the buffer must be at least 2+4n bytes long to hold n words
			// if flag == 1, don't write unrecognized tokens
			ZDictionaryParseTable parseTable=zDictionary->performLexicalAnalysis(zMemory->getRawDataPtr()+text);	// must use real address here
			parseTable.trimTable(maxWords);
			parseTable.writeParseBuffer(parse, *zMemory, (bool)flag);
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of ENCODE_TEXT opcode
	int ENCODE_TEXT(ZOpcode& zOp){
		try{
			//encode_text zscii-text length from coded-text
			zword zsciiText=retrieveOperandValue(zOp, 0);
			zword length=retrieveOperandValue(zOp, 1);
			zword from=retrieveOperandValue(zOp, 2);
			zword codedText=retrieveOperandValue(zOp, 3);
			// text begins at zsciiText+from and is length chars long
			zchar* text=new zchar[length];
			for(int i=0; i<length; i++){
				text[i]=zMemory->readZByte(zsciiText+from+i);
			}
			// now we will encode it
			zword* zcharString=ZSCIItoZCharString(text);
			// and convert it to a dictionary string
			zword* zcharDict=zChartoDictionaryZCharString(zcharString);
			zword strSize=(length/3)+(3-(length%3));	// str size == next multiple of 3
			if(zVersion<=3){
				strSize=(strSize<4) ? strSize : 2;
			}else{
				strSize=(strSize<6) ? strSize : 3;
			}
			// copy it into coded-text
			for(int i=0; i<strSize; i++){
				zMemory->storeZWord(codedText+(2*i), zcharDict[i]);
			}
			// all done!
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of COPY_TABLE opcode
	int COPY_TABLE(ZOpcode& zOp){
		try{
			// copy_table first second size
			// if second==0, then all bytes of first are zeroed
			// otherwise all bytes in first == all bytes in second
			// size may be negative, so size = abs(size)
			// if size == negative, copy forwards regardless
			// else copy forwards/backwards to avoid corrupting first
			zword first=retrieveOperandValue(zOp, 0);
			zword second=retrieveOperandValue(zOp, 1);
			szword size=retrieveOperandValue(zOp, 2);
			enum{FORWARD, BACKWARD};
			int direction=NULL;
			if(size<0){
				zword firstBegin, firstEnd;
				zword secondBegin, secondEnd;
				firstBegin=first;
				firstEnd=first+size;
				secondBegin=second;
				secondEnd=second+size;
				if(secondEnd>firstBegin){
					// second overlaps first from behind
					direction=FORWARD;
				}else if(firstEnd>secondBegin){
					// first overlaps second from behind
					direction=BACKWARD;
				}
			}else{
				direction=FORWARD;
			}
			if(direction==FORWARD){
				for(int i=0; i<size; i++){
					zMemory->storeZByte(second+i, zMemory->readZByte(first+i));
				}
			}else{
				for(int i=(size-1); i<=0; i--){
					zMemory->storeZByte(second+i, zMemory->readZByte(first+i));
				}
			}
		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of PRINT_TABLE instruction
	int PRINT_TABLE(ZOpcode& zOp){
		try{

		}catch(...){
			throw IllegalZOpcode();
		}
	}

	// implementation of LOG_SHIFT instruction
	int LOG_SHIFT(ZOpcode& zOp){
		try{
			// log_shift number, places -> (result)
			// does logical shift of number by places
			// if places is positive, operation is a lsh
			// if places is negative, operation is a rsh
			zword number=retrieveOperandValue(zOp, 0);
			int places=retrieveOperandValue(zOp, 1);
			if(places!=0){
				// attempting to shift by 0 places
				// does nothing
				if(places>0){
					// positive -> left shift
					number<<=places;
				}else if(places<0){
					// negative -> right shift
					number>>=places;
				}
			}
			// store
			storeVariable(zOp.storeInfo.storeVar, number);
		}catch(...){
			throw IllegalZOpcode();
		}
	}
};