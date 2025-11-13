#include "PPCop.h"
#include <math.h>

uint32_t PPCop::init(string& operation)
{
	if (operation.size() == 8)
	{
		for (int i = 0; i < operation.size(); i++)
			if (!((operation[i] >= 'a' && operation[i] <= 'f') &&
				(operation[i] >= 'A' && operation[i] <= 'F') &&
				(operation[i] >= '0' && operation[i] <= '9')))
			{
				badOp = true;

			}
	}
	return value;
}

PPCop::PPCop()
{
	badOp = false;
	labelRequest = "";
}
PPCop::PPCop(uint32_t setHex)
{
	labelRequest = "";
	setDirect(setHex);
}
PPCop::PPCop(string setString)
{
	labelRequest = "";
	set(setString);
}

void PPCop::cleanCode(vector<string>& vecList, string opString, aliasGroup& parentCodeLocal, aliasGroup& parentCodeWide)
{
	istringstream scanOperation(opString);
	string tempLex;
	char tempChar;
	bool terminateLex;
	while (!scanOperation.eof())
	{
		terminateLex = false;
		tempLex = "";
		while (scanOperation.peek() == ' ' || scanOperation.peek() == '\t')
			scanOperation.get();

		while (!terminateLex)
		{
			scanOperation.get(tempChar);
			if (scanOperation.eof())
				break;
			switch (tempChar)
			{
			case ' ': case '\t': case ',': case '(': case ')': terminateLex = true; break;
			default:
				tempLex += tempChar;
			}
		}
		if (!tempLex.empty())
			vecList.push_back(tempLex);
	}
	
	bool finishAliases = true;
	while (finishAliases)
	{
		finishAliases = false;
		for (int j = 1; j < vecList.size(); j++)
		{
			findAliases(vecList[j], finishAliases, parentCodeLocal, parentCodeWide);
		}
	}
	
}
void PPCop::findAliases(string& comparedString, bool& foundVal, aliasGroup& localReplaceList, aliasGroup& replaceList)
{
	vector<string> operations;
	int temp = 0;
	for (int i = comparedString.size() - 2; i > 0; i--)
	{
		if (comparedString[i] == '+')
		{
			operations.emplace_back(comparedString.substr(i+1));
			comparedString = comparedString.substr(0, i);
		}
	}
	for (int i = 0; i < localReplaceList.aliasList.size(); i++)
	{
		if (boost::iequals(comparedString, localReplaceList.aliasList.at(i).aliasName))
		{
			foundVal = true;
			comparedString = localReplaceList.aliasList.at(i).aliasContent;
			break;
		}
	}
	if (!foundVal)
	{
		for (int i = 0; i < replaceList.aliasList.size(); i++)
		{
			if (boost::iequals(comparedString, replaceList.aliasList.at(i).aliasName))
			{
				foundVal = true;
				comparedString = replaceList.aliasList.at(i).aliasContent;
				break;
			}
		}
	}
	//if (foundVal)
	//{
		for (int i = 0; i < operations.size(); i++)
		{
			if (i == 0)
				temp = _STOI(comparedString);
			temp += _STOI(operations[i]);
			comparedString = to_string(temp);
		}
	//}
}
void PPCop::set(string hex)
{
	badOp = false;
	value = (uint32_t)stoul(hex, nullptr, 16);
}
void PPCop::setDirect(uint32_t hex)
{
	badOp = false;
	labelRequest = "";
	value = hex;
}
void PPCop::enforceOffset(int16_t hex)
{
	value = value % 0xFFFF0000 + (hex);
}

void PPCop::errorCheck(bool& error)
{
	if (!error && badOp)
		error = true;
}
void PPCop::showDistance(int offset)
{
	if (b <= opType && opType <= bla)
		value += ((uint32_t)(offset) & (uint32_t)0x3FFFFFC);
	else if (bc <= opType && opType <= bcla)
	{
		value += (uint16_t)offset;
		if (offset < 0)
			value ^= (uint32_t)(0b00001 * pow(2, (31 - 10)));
	}

}
void PPCop::detectOperation(string opString, aliasGroup& parentCodeLocal, aliasGroup& parentCodeWide)
{
	int tempOff = 0;
	vector<string> opPieces;
	cleanCode(opPieces, opString, parentCodeLocal, parentCodeWide);
	labelRequest = "";
	opLength = 0;
	value = (int32_t)-1;
	if (opPieces.size() > 0)
	{
		switch (opPieces[0].at(0))
		{
		case 'a': case 'A':
			opMath(opPieces); break;	//add, and
		case 'b': case 'B':
			if (boost::iequals(opPieces[0].substr(0,4),"byte"))
				opTypes(opPieces); //word, byte, half
			else 
				opBranch(opPieces); break;
		case 'c': case 'C':
			if (boost::iequals(opPieces[0].substr(0, 4), "cntl")) { opMath(opPieces); break; } //count leading zeroes	
			else if (boost::iequals(opPieces[0].substr(0, 3), "cmp")) { opCompare(opPieces); break; } //compare
			else { opConReg(opPieces); break; } //conditional registers
		case 'd': case 'D':
			opMath(opPieces); break;	//div
		case 'e': case 'E':
			opMath(opPieces); break;	//eqv
		case 'f': case 'F':
			opFloat(opPieces); break; //float operations
		case 'i': case 'I':
			if (boost::iequals(opPieces[0].substr(0, 2), "IC"))
			{
				if (boost::iequals(opPieces[0].substr(2, 6), "_basic"))
					psaTypes(opPieces, 0x00000000);
				else if (boost::iequals(opPieces[0].substr(2, 6), "_float"))
					psaTypes(opPieces, 0x01000000);
				else if (boost::iequals(opPieces[0].substr(2, 4), "_bit"))
					psaTypes(opPieces, 0x02000000);
				else
					opInst(opPieces); break;
				break;
			}
			opInst(opPieces); break;
		case 'l': case 'L':
			if (boost::iequals(opPieces[0].substr(0, 2), "li")) { opMath(opPieces); break; } //load immediate	
			else if (boost::iequals(opPieces[0].substr(0, 2), "LA"))
			{
				if (boost::iequals(opPieces[0].substr(2, 6), "_basic"))
					psaTypes(opPieces, 0x10000000);
				else if (boost::iequals(opPieces[0].substr(2, 6), "_float"))
					psaTypes(opPieces, 0x11000000);
				else if (boost::iequals(opPieces[0].substr(2, 4), "_bit"))
					psaTypes(opPieces, 0x12000000);
				else
					badOp = ((int32_t)value == -1);
				break;
			}
			else { opLoad(opPieces); break; } //load	
		case 'm': case 'M':
			if (boost::iequals(opPieces[0].substr(0, 3), "mul")) { opMath(opPieces); break; } //multiply	
			else if (boost::iequals(opPieces[0].substr(0, 2), "mr")) { opMath(opPieces); break; } //move register
			else { opMove(opPieces); break; } //load	
		case 'n': case 'N':
			opMath(opPieces); break;  //neg, nand, nop
		case 'o': case 'O':
			opMath(opPieces); break;	//or
		case 'p': case 'P':
			opPairedSingle(opPieces); break; //ps
		case 'r': case 'R':
			if (boost::iequals(opPieces[0].substr(0, 2), "RA"))
			{
				if (boost::iequals(opPieces[0].substr(2, 6), "_basic"))
					psaTypes(opPieces, 0x20000000);
				else if (boost::iequals(opPieces[0].substr(2, 6), "_float"))
					psaTypes(opPieces, 0x21000000);
				else if (boost::iequals(opPieces[0].substr(2, 4), "_bit"))
					psaTypes(opPieces, 0x22000000);
				else
					badOp = ((int32_t)value == -1);
				break;
			}
			opRotate(opPieces); break; //rot, rl
		case 's': case 'S':
			if (boost::iequals(opPieces[0].substr(0, 3), "sub")) {opMath(opPieces); break;} //sub	
			else if (boost::iequals(opPieces[0].substr(0, 2), "st")) { opStore(opPieces); break; } //store
			else if (boost::iequals(opPieces[0].substr(0, 6), "scalar")) { opTypes(opPieces); break; } //scalar 
			else { opRotate(opPieces); break; } //shift		
		case 't': case 'T':
			opTrap(opPieces); break;
		case 'w': case 'W': case 'h': case 'H':
			opTypes(opPieces); break; //word, byte, half, scalar
		case 'x': case 'X':
			opMath(opPieces); //xor, xori
		}
	}
	badOp = ((int32_t)value == -1);
}


bool PPCop::isOp(string opString, string compare)
{
	int size = compare.size();
	bool isSame;
	isSame = boost::iequals(opString.substr(opLength, size), compare);
	if (!isSame)
		return false;
	opLength += size;
	return true;
}

void PPCop::opTypes(vector<string>& vecList)
{
	value = (uint32_t)_STOI(vecList[1]);
	if (ISOP("half"))
		value &= 0xFFFF;
	else if (ISOP("byte"))
		value &= 0xFF;
	else if (ISOP("scalar"))
		value = ((uint32_t)(stof(vecList[1], nullptr) * 60000));
}
void PPCop::psaTypes(vector<string>& vecList, int access = -1)
{
	value = (uint32_t)_STOI(vecList[1]) & 0xFFFFFF;
	if (access == -1)
		value = access;
	else
		value += access;
}
uint32_t PPCop::getOpBeginning()
{
	return (value / 0x4000000);
}
uint32_t PPCop::setOpBeginning(uint32_t val)
{
	return (uint32_t)val * (pow(2, (31 - 5)));
}
uint32_t PPCop::setBOBI(uint32_t BO, uint32_t BI)
{
	return (BO * pow(2, (31 - 10)) + BI * pow(2, (31 - 15)));
}
uint32_t PPCop::setBTBABB(uint32_t BT, uint32_t BA, uint32_t BB)
{
	return (BT * pow(2, (31 - 10)) + BA * pow(2, (31 - 15)) + BB * pow(2, (31 - 20)));
}
uint32_t PPCop::setRTDRA(uint32_t RT, uint32_t D, uint32_t RA)
{
	return (RT * pow(2, (31 - 10))) + (RA * pow(2, (31 - 15))) + (uint16_t)D;
}
uint32_t PPCop::setRARSSHMBME(uint32_t RA, uint32_t RS, uint32_t SH, uint32_t MB, uint32_t ME)
{
	return (RS * pow(2, 31 - 10) +
		(RA * pow(2, (31 - 15)) +
		(SH * pow(2, 31 - 20)) +
			(MB * pow(2, 31 - 25))) +
			(ME * 2));
}
uint32_t PPCop::regClean(string& regval)
{
	int i = 0; //spr = special register, cr = condition register, f/fr = float register, r = register
	if (regval[0] == 's' && regval[1] == 'p')
		i += 2;
	else if (regval[i] == 'c' || regval[i] == 'f')
		i++;
	if (regval[i] == 'r')
		i++;
	return stoi(regval.substr(i));
}

uint8_t PPCop::retrieveByte(int byte)
{
	switch (byte)
	{
	case 3: return (value & 0xFF);
	case 2: return ((value / 0x100) & 0xFF);
	case 1: return ((value / 0x10000) & 0xFF);
	default: return ((value / 0x1000000) & 0xFF);
	}
}

void PPCop::checkBranchCondition(string& opString)
{
	if (isOp(opString,"lr")) { opType = bclr; value += 16 * 2; }
	else if (isOp(opString, "ctr")) { opType = bcctr; value += 528 * 2; }
	else	
	{ 
		value -= setOpBeginning(3);  //16 instead of 19
	}
	if (isOp(opString, "l"))
	{
		value++;  opType + 2;  //LK = 1
	}
	if (isOp(opString, "+"))
		value += (0b00001 * pow(2, (31 - 10)));
	/*bool isBDZ = (value & (uint32_t)(0b10000 * pow(2, (31 - 10)))) ? true : false;
	if (isOp(opString, "+"))
	{
		if (!isBDZ)
			value += (0b00001 * pow(2, (31 - 10)));
		else
			value += (0b00001 * pow(2, (31 - 10)));
	}
	else if (isOp(opString,"-"))
	{
		if (isBDZ)
			value += (0b01000 * pow(2, (31 - 10)));
		else
			value += (0b00010 * pow(2, (31 - 10)));
	}*/
}

void PPCop::opBranch(vector<string>& vecList)
{
	int tempOff;
	value = setOpBeginning(19); opType = bc;
	if (ISOP("beq"))	   { value += setBOBI(0b01100, 2);}
	else if (ISOP("bne"))  { value += setBOBI(0b00100, 2);}
	else if (ISOP("blt"))  { value += setBOBI(0b01100, 0);}
	else if (ISOP("bge"))  { value += setBOBI(0b00100, 0);}
	else if (ISOP("bgt"))  { value += setBOBI(0b01100, 1);}
	else if (ISOP("ble"))  { value += setBOBI(0b00100, 1);}
	else if (ISOP("bdz"))  { value += setBOBI(0b10010, 0);}
	else if (ISOP("bdnz")) { value += setBOBI(0b10000, 0);}
	else if (ISOP("bctr")) { value += setBOBI(0b10100, 0); opLength -= 3; }
	else if (ISOP("blr"))  { value += setBOBI(0b10100, 0); opLength -= 2; }
	else if (ISOP("bc")) { value += setBOBI(vecReg(1), vecReg(2)); opType = bc; }
	else  		{ value = setOpBeginning(18); opType = b;}		// b, ba, bl, bla
	
	if (opType != b)
		checkBranchCondition(vecList[0]);
	else if (ISOP("bl"))
	{
		value++; opType + 2;  //LK = 1
	}
	if (ISOP("ba") || ISOP("a"))
		value += 2; opType + 1; //AA = 1
	if (getOpBeginning() != 19)
	{
		if (vecList.size() > 2)
		{
			if (vecList[1].at(0) == 'c' && vecList[1].at(1) == 'r')
				value |= (uint32_t)(0x40000 * (vecList[1].at(2) - '0'));
		}
	}
	// determines if branches are handled with labels
	if (vecList.size() > 1)
	{
		int i = vecList.size() - 1;
		if (boost::iequals(vecList[i].substr(0, 2), "0x") || boost::iequals(vecList[i].substr(0, 3), "-0x"))
		{
			tempOff = stoi(vecList[i], nullptr, 16);

			if (opType == b)
			{
					value |= ((uint32_t)tempOff) & (uint32_t)0x3FFFFFC;
			}
			else
			{
				if (tempOff < 0)
					value ^= (uint32_t)(0b00001 * pow(2, (31 - 10)));
				value |= (((uint32_t)tempOff) & (uint32_t)0xFFFC);
			}
			if (i == 2)
			{
				//value += vecReg(1) * (pow(2, 31 - 13));
			}


		}
		else
		{
			labelRequest = vecList[i];	//It's a label, find out how far away it is!
		}
	}
}
void PPCop::opConReg(vector<string>& vecList)
{
	value = setOpBeginning(19);
	if (ISOP("crand"))			value += 257 * 2 + incBTBABB;		// Condition Register AND		
	else if (ISOP("cror"))		value += 449 * 2 + incBTBABB;		// Condition Register OR
	else if (ISOP("crmove"))	value += 449 * 2 + incBTBABA;		// Condition Register MOVE
	else if (ISOP("crxor"))		value += 193 * 2 + incBTBABB;		// Condition Register XOR
	else if (ISOP("crclr"))		value += 193 * 2 + incBTBTBT;		// Condition Register CLEAR
	else if (ISOP("crnand"))	value += 225 * 2 + incBTBABB;		// Condition Register NAND
	else if (ISOP("crnor"))		value +=  33 * 2 + incBTBABB;		// Condition Register NOR
	else if (ISOP("crnot"))		value +=  33 * 2 + incBTBABA;		// Condition Register NOT
	else if (ISOP("creqv"))		value += 289 * 2 + incBTBABB;		// Condition Register EQUIVALENT
	else if (ISOP("crset"))		value += 289 * 2 + incBTBTBT;		// Condition Register SET
	else if (ISOP("crandc"))	value += 129 * 2 + incBTBABB;		// Condition Register AND with Complement
	else if (ISOP("crorc"))		value += 417 * 2 + incBTBABB;		// Condition Register OR with Complement 
}
void PPCop::opCompare(vector<string>& vecList)
{
	if (ISOP("cmpi"))
	{
		if (vecList.size() == 3)
			value = setOpBeginning(11) + vecReg(1)*(pow(2, 31 - 15)) + (uint16_t)stoi(vecList[2],nullptr,0);
		else if (vecList.size() == 4)
			value = setOpBeginning(11) + vecReg(2)*(pow(2, 31 - 15)) + (uint16_t)stoi(vecList[3], nullptr, 0) + vecReg(1)*(pow(2, 31 - 8));
		else 
			value = setOpBeginning(11) + vecReg(3)*(pow(2, 31 - 15)) + (uint16_t)stoi(vecList[4], nullptr, 0) + vecReg(1)*(pow(2, 31 - 8)) + (vecReg(2) % 1)*(pow(2,31-10));
	}
	else if (ISOP("cmpwi"))
	{
		if (vecList.size() == 3)
			value = setOpBeginning(11) + vecReg(1)*(pow(2, 31 - 15)) + (uint16_t)stoi(vecList[2], nullptr, 0);
		else
			value = setOpBeginning(11) + vecReg(2)*(pow(2, 31 - 15)) + (uint16_t)stoi(vecList[3], nullptr, 0) + vecReg(1)*(pow(2,31-8));
	}
	else if (ISOP("cmpi"))
	{
		if (vecList.size() == 3)
			value = setOpBeginning(10) + vecReg(1)*(pow(2, 31 - 15)) + (uint16_t)stoi(vecList[2], nullptr, 0);
		else if (vecList.size() == 4)
			value = setOpBeginning(10) + vecReg(2)*(pow(2, 31 - 15)) + (uint16_t)stoi(vecList[3], nullptr, 0) + vecReg(1)*(pow(2, 31 - 8));
		else
			value = setOpBeginning(10) + vecReg(3)*(pow(2, 31 - 15)) + (uint16_t)stoi(vecList[4], nullptr, 0) + vecReg(1)*(pow(2, 31 - 8)) + (vecReg(2) % 1)*(pow(2, 31 - 10));
	}
	else if (ISOP("cmplwi"))
	{
		if (vecList.size() == 3)
			value = setOpBeginning(10) + vecReg(1)*(pow(2, 31 - 15)) + (uint16_t)stoi(vecList[2], nullptr, 0);
		else
			value = setOpBeginning(10) + vecReg(2)*(pow(2, 31 - 15)) + (uint16_t)stoi(vecList[3], nullptr, 0) + vecReg(1)*(pow(2, 31 - 8));
	}
	
	else if (ISOP("cmplw"))
	{
		if (vecList.size() == 3)
			value = setOpBeginning(31) + 32 * 2 + vecReg(1)*pow(2, 31 - 15) + vecReg(2)*pow(2, 31 - 20);
		else
			value = setOpBeginning(31) + 32 * 2 + vecReg(2)*pow(2, 31 - 15) + vecReg(3)*pow(2, 31 - 20) + vecReg(1)*pow(2, 31 - 8);
	}
	else if (ISOP("cmpl"))
	{
		value = setOpBeginning(31) + 32 * 2 + vecReg(1)*pow(2, 31 - 8) + vecReg(2)*pow(2, 31 - 15) + vecReg(3)*pow(2, 31 - 20);
	}

	else if (ISOP("cmpw"))
	{
		if (vecList.size() == 3)
			value = setOpBeginning(31) + vecReg(1)*pow(2, 31 - 15) + vecReg(2)*pow(2, 31 - 20);
		else
			value = setOpBeginning(31) + vecReg(2)*pow(2, 31 - 15) + vecReg(3)*pow(2, 31 - 20) + vecReg(1)*pow(2, 31 - 8);
	}
	else if (ISOP("cmp"))
	{
		value = setOpBeginning(31) + vecReg(1)*pow(2, 31 - 8) + vecReg(2)*pow(2, 31 - 15) + vecReg(3)*pow(2, 31 - 20);
	}

}

void PPCop::opInst(vector<string>& vecList)
{
	if (ISOP("icbi"))		value = setOpBeginning(31) + 982 * 2 + setBTBABB(0, regClean(vecList[1]), regClean(vecList[2])); //instruction cache block invalidate
	else if (ISOP("isync")) value = setOpBeginning(19) + 150 * 2; //instruction synchronize
}
void PPCop::opLoad(vector<string>& vecList)
{
	if (ISOP("lbzux"))		value = setOpBeginning(31) + 119 * 2 + incRTRARB;	//Load Byte and Zero with Update Indexed X-Form
	else if (ISOP("lhzux"))	value = setOpBeginning(31) + 311 * 2 + incRTRARB;	//Load Halfword and Zero with Update Indexed X-Form 
	else if (ISOP("lhaux"))	value = setOpBeginning(31) + 375 * 2 + incRTRARB;	//Load Halfword Algebraic with Update Indexed X-Form 
	else if (ISOP("lwzux"))	value = setOpBeginning(31) +  55 * 2 + incRTRARB;	//Load Word and Zero with Update Indexed X-Form 
	else if (ISOP("lwaux"))	value = setOpBeginning(31) + 373 * 2 + incRTRARB;	//Load Word Algebraic with Update Indexed X-Form 
	else if (ISOP("lfsux"))	value = setOpBeginning(31) + 567 * 2 + incRTRARB;	//Load Floating-Point Single with Update Indexed X-form
	else if (ISOP("lfdux"))	value = setOpBeginning(31) + 631 * 2 + incRTRARB;	//Load Floating-Point Double with Update Indexed X-form
	else if (ISOP("ldux"))	value = setOpBeginning(31) +  53 * 2 + incRTRARB;	//Load Doubleword with Update Indexed X-Form 
	
	else if (ISOP("lbzx"))	value = setOpBeginning(31) +  87 * 2 + incRTRARB;	//Load Byte and Zero Indexed X-Form
	else if (ISOP("lhzx"))	value = setOpBeginning(31) + 279 * 2 + incRTRARB;	//Load Halfword and Zero Indexed X-Form
	else if (ISOP("lhax"))	value = setOpBeginning(31) + 343 * 2 + incRTRARB;	//Load Halfword Algebraic Indexed X-Form
	else if (ISOP("lwzx"))	value = setOpBeginning(31) +  23 * 2 + incRTRARB;	//Load Word and Zero Indexed X-Form
	else if (ISOP("lwax"))	value = setOpBeginning(31) + 341 * 2 + incRTRARB;	//Load Word Algebraic Indexed X-Form
	else if (ISOP("lfsx"))	value = setOpBeginning(31) + 535 * 2 + incRTRARB;	//Load Floating-Point Single Indexed X-Form
	else if (ISOP("lfdx"))	value = setOpBeginning(31) + 599 * 2 + incRTRARB;	//Load Floating-Point Double Indexed X-Form
	else if (ISOP("ldx"))	value = setOpBeginning(31) +  21 * 2 + incRTRARB;	//Load Doubleword Indexed X-Form
	
	else if (ISOP("lbzu"))	value = setOpBeginning(35) + incRTDRA;				//Load Byte and Zero with Update D-Form
	else if (ISOP("lhzu"))	value = setOpBeginning(41) + incRTDRA;				//Load Halfword and Zero with Update D-Form
	else if (ISOP("lhau"))	value = setOpBeginning(43) + incRTDRA;				//Load Halfword Algebraic with Update D-Form
	else if (ISOP("lwzu"))	value = setOpBeginning(33) + incRTDRA;				//Load Word and Zero with Update D-Form
	else if (ISOP("lfsu"))	value = setOpBeginning(49) + incRTDRA;				//Load Floating-Point Single With Update D-Form
	else if (ISOP("lfdu"))	value = setOpBeginning(51) + incRTDRA;				//Load Floating-Point Double With Update D-Form
	else if (ISOP("ldu"))	value = setOpBeginning(58) + incRTDSRA + 1;			//Load Doubleword with Update DS-Form
	
	else if (ISOP("lhbrx"))	value = setOpBeginning(31) + 790 * 2 + incRTRARB;	//Load Halfword Byte-Reverse Indexed X-Form
	else if (ISOP("lwbrx"))	value = setOpBeginning(31) + 534 * 2 + incRTRARB;	//Load Word Byte-Reverse Indexed X-Form

	else if (ISOP("lbz"))	value = setOpBeginning(34) + incRTDRA;				//Load Byte and Zero D-Form
	else if (ISOP("lhz"))	value = setOpBeginning(40) + incRTDRA;				//Load Halfword and Zero D-Form
	else if (ISOP("lha"))	value = setOpBeginning(40) + incRTDRA;				//Load Halfword Algebraic D-Form
	else if (ISOP("lwz"))	value = setOpBeginning(32) + incRTDRA;				//Load Word and Zero D-Form
	else if (ISOP("lfs"))	value = setOpBeginning(48) + incRTDRA;				//Load Floating-Point Single D-Form
	else if (ISOP("lfd"))	value = setOpBeginning(50) + incRTDRA;				//Load Floating-Point Double D-Form
	else if (ISOP("lwa"))	value = setOpBeginning(58) + incRTDSRA + 2;			//Load Word Algebraic DS-Form
	else if (ISOP("ld"))	value = setOpBeginning(58) + incRTDSRA;				//Load Doubleword DS-Form
	
	else if (ISOP("lmw"))	value = setOpBeginning(46) + incRTDRA;				//Load Multiple Word D-Form
	else if (ISOP("lswi"))	value = setOpBeginning(31) + 597 * 2 + incRTRANB;	//Load String Word Immediate X-Form
	else if (ISOP("lswx"))	value = setOpBeginning(31) + 533 * 2 + incRTRARB;	//Load String Word Indexed X-Form
}
void PPCop::opStore(vector<string>& vecList)
{
	if (ISOP("stbux"))		value = setOpBeginning(31) + 247 * 2 + incRTRARB;	//Store Byte with Update Indexed X-Form
	else if (ISOP("sthux"))	value = setOpBeginning(31) + 439 * 2 + incRTRARB;	//Store Halfword with Update Indexed X-Form 
	else if (ISOP("stwux"))	value = setOpBeginning(31) + 183 * 2 + incRTRARB;	//Store Word with Update Indexed X-Form 
	else if (ISOP("stfsux"))value = setOpBeginning(31) + 695 * 2 + incRTRARB;	//Store Floating-Point Single with Update Indexed X-Form 
	else if (ISOP("stfdux"))value = setOpBeginning(31) + 759 * 2 + incRTRARB;	//Store Floating-Point Double with Update Indexed X-Form 
	else if (ISOP("stdux"))	value = setOpBeginning(31) + 181 * 2 + incRTRARB;	//Store Doubleword with Update Indexed X-Form 

	else if (ISOP("stbx"))	value = setOpBeginning(31) + 215 * 2 + incRTRARB;	//Store Byte Indexed X-Form
	else if (ISOP("sthx"))	value = setOpBeginning(31) + 407 * 2 + incRTRARB;	//Store Halfword Indexed X-Form
	else if (ISOP("stwx"))	value = setOpBeginning(31) + 151 * 2 + incRTRARB;	//Store Word Indexed X-Form
	else if (ISOP("stfsx"))	value = setOpBeginning(31) + 663 * 2 + incRTRARB;	//Store Floating-Point Single Indexed X-Form
	else if (ISOP("stfdx"))	value = setOpBeginning(31) + 727 * 2 + incRTRARB;	//Store Floating-Point Double Indexed X-Form
	else if (ISOP("stdx"))	value = setOpBeginning(31) + 149 * 2 + incRTRARB;	//Store Doubleword Indexed X-Form

	else if (ISOP("stbu"))	value = setOpBeginning(39) + incRTDRA;				//Store Byte with Update D-Form
	else if (ISOP("sthu"))	value = setOpBeginning(45) + incRTDRA;				//Store Halfword with Update D-Form
	else if (ISOP("stwu"))	value = setOpBeginning(37) + incRTDRA;				//Store Word with Update D-Form
	else if (ISOP("stfsu"))	value = setOpBeginning(53) + incRTDRA;				//Store Floating-Point Single with Update D-Form
	else if (ISOP("stfdu"))	value = setOpBeginning(55) + incRTDRA;				//Store Floating-Point Double with Update D-Form
	else if (ISOP("stdu"))	value = setOpBeginning(62) + incRTDSRA + 1;			//Store Doubleword with Update DS-Form

	else if (ISOP("sthbrx"))	value = setOpBeginning(31) + 918 * 2 + incRTRARB;	//Store Halfword Byte-Reverse Indexed X-Form
	else if (ISOP("stwbrx"))	value = setOpBeginning(31) + 662 * 2 + incRTRARB;	//Store Word Byte-Reverse Indexed X-Form

	else if (ISOP("stb"))	value = setOpBeginning(38) + incRTDRA;				//Store Byte D-Form
	else if (ISOP("sth"))	value = setOpBeginning(44) + incRTDRA;				//Store Halfword D-Form
	else if (ISOP("stw"))	value = setOpBeginning(36) + incRTDRA;				//Store Word D-Form
	else if (ISOP("stfs"))	value = setOpBeginning(52) + incRTDRA;				//Store Floating-Point Single D-Form
	else if (ISOP("stfd"))	value = setOpBeginning(54) + incRTDRA;				//Store Floating-Point Double D-Form
	else if (ISOP("std"))	value = setOpBeginning(62) + incRTDSRA;				//Store Doubleword DS-Form

	else if (ISOP("stmw"))	value = setOpBeginning(47) + incRTDRA;				//Store Multiple Word D-Form
	else if (ISOP("stswi"))	value = setOpBeginning(31) + 725 * 2 + incRTRANB;	//Store String Word Immediate X-Form
	else if (ISOP("stswx"))	value = setOpBeginning(31) + 661 * 2 + incRTRARB;	//Store String Word Indexed X-Form
	else if (ISOP("stfiwx"))value = setOpBeginning(31) + 983 * 2 + incRTRARB;	//Store Floating-Point as Integer Word Indexed X-Form
}
void PPCop::opMath(vector<string>& vecList)
{
	if (ISOP("addis"))		value = setOpBeginning(15) + incRTRASI;					//Add Immediate Shifted D-Form
	else if (ISOP("subis"))	value = setOpBeginning(15) + decRTRASI;					//Subtract Immediate Shifted D-Form
	else if (ISOP("lis"))	value = setOpBeginning(15) + incRTSI;					//Load Immediate Shifted D-Form

	else if (ISOP("addic."))value = setOpBeginning(13) + incRTRASI;					// Add Immediate Carrying D-Form
	else if (ISOP("subic."))value = setOpBeginning(13) + decRTRASI;					// Subtract Immediate Carrying D-Form
	else if (ISOP("addic")) value = setOpBeginning(12) + incRTRASI;					// Add Immediate Carrying D-Form
	else if (ISOP("subic")) value = setOpBeginning(12) + decRTRASI;					// Subtract Immediate Carrying D-Form
	else if (ISOP("subfic")) value = setOpBeginning(8) + incRTRASI;					// Subtract From Immediate Carrying D-Form

	else if (ISOP("addi"))	value = setOpBeginning(14) + incRTRASI;					//Add Immediate D-Form
	else if (ISOP("subi"))	value = setOpBeginning(14) + decRTRASI;					//Subtract Immediate D-Form
	else if (ISOP("li"))	value = setOpBeginning(14) + incRTSI;					//Load Immediate D-Form

	else if (ISOP("addc"))	value = setOpBeginning(31) +  10 * 2 + incRTRARB;		//Add Carrying XO-Form
	else if (ISOP("subfc"))	value = setOpBeginning(31) +   8 * 2 + incRTRARB;		//Subtract From Carrying XO-Form
	else if (ISOP("subc"))	value = setOpBeginning(31) +   8 * 2 + incRTRBRA;		//Subtract Carrying XO-Form

	else if (ISOP("adde"))	value = setOpBeginning(31) + 138 * 2 + incRTRARB;		//Add Extended XO-Form
	else if (ISOP("subfe"))	value = setOpBeginning(31) + 136 * 2 + incRTRARB;		//Subtract From Extended XO-Form
	else if (ISOP("sube"))	value = setOpBeginning(31) + 136 * 2 + incRTRBRA;		//Subtract Extended XO-Form

	else if (ISOP("addme"))	value = setOpBeginning(31) + 234 * 2 + incRTRA;			//Add to Minus One Extended XO-Form
	else if (ISOP("subfme"))value = setOpBeginning(31) + 232 * 2 + incRTRA;			//Subtract From Minus One Extended XO-Form
	else if (ISOP("addze"))	value = setOpBeginning(31) + 202 * 2 + incRTRA;			//Add to Zero Extended XO-Form
	else if (ISOP("subfze"))value = setOpBeginning(31) + 200 * 2 + incRTRA;			//Subtract From Zero Extended XO-Form

	else if (ISOP("neg"))	value = setOpBeginning(31) + 104 * 2 + incRTRA;			//Negate XO-Form
	else if (ISOP("add"))	value = setOpBeginning(31) + 266 * 2 + incRTRARB;		//Add XO-Form
	else if (ISOP("subf"))	value = setOpBeginning(31) +  40 * 2 + incRTRARB;		//Subtract From XO-Form
	else if (ISOP("sub"))	value = setOpBeginning(31) +  40 * 2 + incRTRBRA;		//Subtract XO-Form
	
	else if (ISOP("mulli"))	value = setOpBeginning(7) + incRTRASI;					//Multiply Low Immediate D-Form
	else if (ISOP("mulld")) value = setOpBeginning(31) + 233 * 2 + incRTRARB;		//Multiply Low Doubleword XO-Form
	else if (ISOP("mullw")) value = setOpBeginning(31) + 235 * 2 + incRTRARB;		//Multiply Low Word XO-Form
	else if (ISOP("mulhdu")) value = setOpBeginning(31) +  9 * 2 + incRTRARB;		//Multiply High Doubleword Unsigned XO-Form
	else if (ISOP("mulhwu")) value = setOpBeginning(31) + 11 * 2 + incRTRARB;		//Multiply High Word Unsigned XO-Form
	else if (ISOP("mulhd")) value = setOpBeginning(31) +  73 * 2 + incRTRARB;		//Multiply High Doubleword XO-Form
	else if (ISOP("mulhw")) value = setOpBeginning(31) +  75 * 2 + incRTRARB;		//Multiply High Word XO-Form

	else if (ISOP("divdu"))	value = setOpBeginning(31) + 457 * 2 + incRTRARB;		//Divide Doubleword Unsigned XO-Form
	else if (ISOP("divwu"))	value = setOpBeginning(31) + 459 * 2 + incRTRARB;		//Divide Word Unsigned XO-Form
	else if (ISOP("divd"))	value = setOpBeginning(31) + 489 * 2 + incRTRARB;		//Divide Doubleword XO-Form
	else if (ISOP("divw"))	value = setOpBeginning(31) + 491 * 2 + incRTRARB;		//Divide Word XO-Form

	else if (ISOP("andis."))value = setOpBeginning(29) + incRARSUI;					//AND Immediate Shifted D-Form
	else if (ISOP("andi."))	value = setOpBeginning(28) + incRARSUI;					//AND Immediate D-Form
	else if (ISOP("xoris"))	value = setOpBeginning(27) + incRARSUI;					//XOR Immediate Shifted D-Form
	else if (ISOP("xori"))	value = setOpBeginning(26) + incRARSUI;					//XOR Immediate D-Form
	else if (ISOP("oris"))	value = setOpBeginning(25) + incRARSUI;					//OR Immediate Shifted D-Form
	else if (ISOP("ori"))	value = setOpBeginning(24) + incRARSUI;					//OR Immediate D-Form
	else if (ISOP("nop"))	value = setOpBeginning(24);								//NO Operation

	else if (ISOP("andc"))	value = setOpBeginning(31) +  60 * 2 + incRARSRB;		//AND with Complement X-Form
	else if (ISOP("and"))	value = setOpBeginning(31) +  28 * 2 + incRARSRB;		//AND X-Form
	else if (ISOP("nand"))	value = setOpBeginning(31) + 476 * 2 + incRARSRB;		//NAND X-Form
	else if (ISOP("xor"))	value = setOpBeginning(31) + 316 * 2 + incRARSRB;		//XOR X-Form 
	else if (ISOP("orc"))	value = setOpBeginning(31) + 412 * 2 + incRARSRB;		//OR with Complement X-Form
	else if (ISOP("or"))	value = setOpBeginning(31) + 444 * 2 + incRARSRB;		//OR X-Form
	else if (ISOP("mr"))	value = setOpBeginning(31) + 444 * 2 + incRARSRS;		//MOVE X-Form
	else if (ISOP("nor"))	value = setOpBeginning(31) + 124 * 2 + incRARSRB;		//NOR X-Form
	else if (ISOP("not"))	value = setOpBeginning(31) + 124 * 2 + incRARSRS;		//NOT X-Form
	else if (ISOP("eqv"))	value = setOpBeginning(31) + 284 * 2 + incRARSRS;		//Equivalent X-Form
	
	else if (ISOP("extsb"))	value = setOpBeginning(31) + 954 * 2 + incRARS;			//Extend Sign Byte X-Form
	else if (ISOP("extsh"))	value = setOpBeginning(31) + 922 * 2 + incRARS;			//Extend Sign Halfword X-Form
	else if (ISOP("extsw"))	value = setOpBeginning(31) + 986 * 2 + incRARS;			//Extend Sign Word X-Form

	else if (ISOP("cntlzd"))value = setOpBeginning(31) + 58 * 2 + incRARS;			//Count Leading Zeroes Doubleword X-Form
	else if (ISOP("cntlzw"))value = setOpBeginning(31) + 26 * 2 + incRARS;			//Count Leading Zeroes Word X-Form

	if (getOpBeginning() == 31)
	{
		if (boost::iequals(vecList[0].substr(vecList[0].size() - 2, 2), "o."))
			value += 401;
		else if (vecList[0].at(vecList[0].size() - 1) == 'o' || vecList[0].at(vecList[0].size() - 1) == 'O')
			value += 400;
		else if (vecList[0].at(vecList[0].size() - 1) == '.')
			value++;

	}
}
void PPCop::opMove(vector<string>& vecList)
{
	value = setOpBeginning(31);
	if (ISOP("mtspr"))	//Move To Special Purpose Register X-Form
	{
		value += 467 * 2;
		if (boost::iequals(vecList[2], "lr") || regClean(vecList[2]) == 8)
			value += setBTBABB(regClean(vecList[1]), 8, 0);
		else if (boost::iequals(vecList[2], "ctr") || regClean(vecList[2]) == 9)
			value += setBTBABB(regClean(vecList[1]), 9, 0);
		else if (boost::iequals(vecList[2], "xer") || regClean(vecList[2]) == 1)
			value += setBTBABB(regClean(vecList[1]), 1, 0);
	}
	else if (ISOP("mtlr"))	value += 467 * 2 + setBTBABB(regClean(vecList[1]), 8, 0);		//Move To Link Register
	else if (ISOP("mtctr"))	value += 467 * 2 + setBTBABB(regClean(vecList[1]), 9, 0);		//Move To Count Register
	else if (ISOP("mtxer"))	value += 467 * 2 + setBTBABB(regClean(vecList[1]), 1, 0);		//Move To
	else if (ISOP("mfspr"))
	{
		value += 339 * 2;
		if (boost::iequals(vecList[2], "lr") || regClean(vecList[2]) == 8)
			value += setBTBABB(regClean(vecList[1]), 8, 0);
		else if (boost::iequals(vecList[2], "ctr") || regClean(vecList[2]) == 9)
			value += setBTBABB(regClean(vecList[1]), 9, 0);
		else if (boost::iequals(vecList[2], "xer") || regClean(vecList[2]) == 1)
			value += setBTBABB(regClean(vecList[1]), 1, 0);;
	}
	else if (ISOP("mflr"))	value += 339 * 2 + setBTBABB(regClean(vecList[1]), 8, 0);		//Move From Link Register
	else if (ISOP("mfctr"))	value += 339 * 2 + setBTBABB(regClean(vecList[1]), 9, 0);		//Move From Count Register
	else if (ISOP("mfxer"))	value += 339 * 2 + setBTBABB(regClean(vecList[1]), 1, 0);		//Move From
	else if (ISOP("mfcr"))	value +=  19 * 2 + incRT;										//Move From Condition Register XFX-Form
	else if (ISOP("mffs"))	value = setOpBeginning(63) + 583 * 2 + incRT;					//Move from FPSCR X-Form
	else if (ISOP("mtcrf"))
	{
		value += 144 * 2 + regClean(vecList[1]) * pow (2, 31-19) + regClean(vecList[2]) * pow(2, 31-10);
	}
	else if (ISOP("mtcr"))
	{
		value += 144 * 2 + 0xFF * pow(2, 31 - 19) + regClean(vecList[1]) * pow(2, 31 - 10);
	}

	if (boost::ifind_first(vecList[0], ".") && getOpBeginning() == 63)
		value++;
}
void PPCop::opPairedSingle(vector<string>& vecList)
{
	value = setOpBeginning(4);
	if (ISOP("psq_lx"))	//Paired Singles Quantized Load Indexed
	{
		value += setRARSSHMBME(vecReg(0), vecReg(1), vecReg(2), (vecReg(3) % 2 * 8 + vecReg(4) % 8) * 2, 6);
	}
	else if (ISOP("psq_lux")) //Paired Singles Quantized Load with Update INdexed
	{
		value += setRARSSHMBME(vecReg(0), vecReg(1), vecReg(2), (vecReg(3) % 2 * 8 + vecReg(4) % 8) * 2 + 1, 6);
	}
	else if (ISOP("psq_stux"))
	{
		value += setRARSSHMBME(vecReg(0), vecReg(1), vecReg(2), (vecReg(3) % 2 * 8 + vecReg(4) % 8) * 2, 7);
	}
	else if (ISOP("psq_stx"))
	{
		value += setRARSSHMBME(vecReg(0), vecReg(1), vecReg(2), (vecReg(3) % 2 * 8 + vecReg(4) % 8) * 2 + 1, 7);
	}
	else if (ISOP("psq_lu"))
	{
		value = setOpBeginning(57) + vecReg(1)*(pow(2, 31 - 10)) + vecReg(3)*(pow(2, 31 - 15))
			+ (vecReg(4) % 2 * 8 + vecReg(5) % 8)*(pow(2, 31 - 19)) + (uint32_t)(stoi(vecList[2],nullptr,0) % 0xFFFFFFFFFFFF);
	}
	else if (ISOP("psq_l"))
	{
		value = setOpBeginning(56) + vecReg(1)*(pow(2, 31 - 10)) + vecReg(3)*(pow(2, 31 - 15))
			+ (vecReg(4) % 2 * 8 + vecReg(5) % 8)*(pow(2, 31 - 19)) + (uint32_t)(stoi(vecList[2], nullptr, 0) % 0xFFFFFFFFFFFF);
	}
	else if (ISOP("psq_stu"))
	{
		value = setOpBeginning(61) + vecReg(1)*(pow(2, 31 - 10)) + vecReg(3)*(pow(2, 31 - 15))
			+ (vecReg(4) % 2 * 8 + vecReg(5) % 8)*(pow(2, 31 - 19)) + (uint32_t)(stoi(vecList[2], nullptr, 0) % 0xFFFFFFFFFFFF);
	}
	else if (ISOP("psq_st"))
	{
		value = setOpBeginning(60) + vecReg(1)*(pow(2, 31 - 10)) + vecReg(3)*(pow(2, 31 - 15))
			+ (vecReg(4) % 2 * 8 + vecReg(5) % 8)*(pow(2, 31 - 19)) + (uint32_t)(stoi(vecList[2], nullptr, 0) % 0xFFFFFFFFFFFF);
	}

	else if (ISOP("ps_abs"))	value += 264 * 2 + incRTRB;
	else if (ISOP("ps_nabs"))	value += 136 * 2 + incRTRB;
	else if (ISOP("ps_mr"))		value += 72 * 2 + incRTRB;
	else if (ISOP("ps_neg"))	value += 40 * 2 + incRTRB;
	else if (ISOP("ps_res"))	value += 24 * 2 + incRTRB;
	else if (ISOP("ps_rsqrte"))	value += 26 * 2 + incRTRB;
	else if (ISOP("ps_add"))	value += 21 * 2 + incRTRARB;
	else if (ISOP("ps_sub"))	value += 20 * 2 + incRTRARB;
	else if (ISOP("ps_div"))	value += 18 * 2 + incRTRARB;

	else if (ISOP("ps_muls0"))	value += 12 * 2 + incRTRA + vecReg(3) * 64;
	else if (ISOP("ps_muls1"))	value += 13 * 2 + incRTRA + vecReg(3) * 64;
	else if (ISOP("ps_mul"))	value += 25 * 2 + incRTRA + vecReg(3) * 64;

	else if (ISOP("ps_madds0"))	value += incFRDFRAFRCFRB(14);
	else if (ISOP("ps_madds1"))	value += incFRDFRAFRCFRB(15);
	else if (ISOP("ps_madd"))	value += incFRDFRAFRCFRB(29);

	else if (ISOP("ps_merge00"))value += 528 * 2 + incRTRARB;
	else if (ISOP("ps_merge01"))value += 560 * 2 + incRTRARB;
	else if (ISOP("ps_merge10"))value += 592 * 2 + incRTRARB;
	else if (ISOP("ps_merge11"))value += 624 * 2 + incRTRARB;
	else if (ISOP("ps_nmadd"))	value += incFRDFRAFRCFRB(31);
	else if (ISOP("ps_nmsub"))	value += incFRDFRAFRCFRB(30);
	else if (ISOP("ps_madd"))	value += incFRDFRAFRCFRB(29);
	else if (ISOP("ps_msub"))	value += incFRDFRAFRCFRB(28);
	else if (ISOP("ps_sum0"))	value += incFRDFRAFRCFRB(10);
	else if (ISOP("ps_sum1"))	value += incFRDFRAFRCFRB(11);
	else if (ISOP("ps_sel"))	value += incFRDFRAFRCFRB(23);
	
	else if (ISOP("ps_cmpo0"))	value +=  32 * 2 + incBFFRAFRB;
	else if (ISOP("ps_cmpo1"))	value +=  96 * 2 + incBFFRAFRB;
	else if (ISOP("ps_cmpu0"))	value += incBFFRAFRB;
	else if (ISOP("ps_cmpu1"))	value +=  64 * 2 + incBFFRAFRB;

}
void PPCop::opRotate(vector<string>& vecList)
{
	if(ISOP("rlwinm"))		value = setOpBeginning(21) + incRARSRBMBME;			//Rotate Left Word Immediate then AND with Mask M-Form
	else if (ISOP("slwi"))
	{
		value = setOpBeginning(21) + setRARSSHMBME(regClean(vecList[1]),
			regClean(vecList[2]), regClean(vecList[3]), 0, 31 - regClean(vecList[3]));
	}
	else if (ISOP("srwi"))
	{
		value = setOpBeginning(21) + setRARSSHMBME(regClean(vecList[1]),
			regClean(vecList[2]), 32 - regClean(vecList[3]), regClean(vecList[3]), 31);
	}
	else if (ISOP("rlwnm"))	value = setOpBeginning(23) + incRARSRBMBME;			//Rotate Left Word then AND with Mask M-Form
	else if (ISOP("rotlw"))	value = setOpBeginning(23) + 31 * 2 + incRARSRB;
	else if (ISOP("rlwimi"))value = setOpBeginning(20) + incRARSSHMBME;			//Rotate Left Word Immediate then Mask Insert M-form

	else if (ISOP("sld"))	value = setOpBeginning(31) +  27 * 2 + incRARSRB;	//Shift Left Doubleword
	else if (ISOP("slw"))	value = setOpBeginning(31) +  24 * 2 + incRARSRB;	//Shift Left Word
	else if (ISOP("srd"))	value = setOpBeginning(31) + 539 * 2 + incRARSRB;	//Shift Right Doubleword
	else if (ISOP("srw"))	value = setOpBeginning(31) + 536 * 2 + incRARSRB;	//Shift Right Word
	else if (ISOP("srawi"))	value = setOpBeginning(31) + 824 * 2 + incRARSSH;	//Shift Right Algebraic Word Immediate X-Form
	else if (ISOP("srad"))	value = setOpBeginning(31) + 794 * 2 + incRARSRB;	//Shift Right Algebraic Doubleword X-Form
	else if (ISOP("sraw"))	value = setOpBeginning(31) + 792 * 2 + incRARSRB;	//Shift Right Algebraci Word X-Form



	if (boost::ifind_first(vecList[0], "."))
		value++;
}
void PPCop::opFloat(vector<string>& vecList)
{
	value = setOpBeginning(63);
	if (ISOP("fmr"))		value +=  72 * 2 + incRTRB;			//Floating Move Register X-Form
	else if (ISOP("fneg"))	value +=  40 * 2 + incRTRB;			//Floating Negate Register X-Form
	else if (ISOP("fabs"))	value += 264 * 2 + incRTRB;			//Floating Absolute Register X-Form
	else if (ISOP("fnabs"))	value += 136 * 2 + incRTRB;			//Floating Negative Absolute Register X-Form
	else if (ISOP("frsp"))	value +=  12 * 2 + incRTRB;			//Floating Round to Single Precision X-Form	
	else if (ISOP("fctidz"))value += 815 * 2 + incRTRB;			//Floating Convert To Integer Doubleword with round toward Zero X-form
	else if (ISOP("fctid"))	value += 814 * 2 + incRTRB;			//Floating Convert To Integer Doubleword X-form
	else if (ISOP("fcfid"))	value += 846 * 2 + incRTRB;			//Floating Convert From Integer Doubleword X-form
	else if (ISOP("fctiwz"))value +=  15 * 2 + incRTRB;			//Floating Convert To Integer Word with round toward Zero X-form
	else if (ISOP("fctiw"))	value +=  14 * 2 + incRTRB;			//Floating Convert To Integer Word X-form
	else if (ISOP("fcmpo"))	value +=  32 * 2 + incBFFRAFRB;		//Floating Compare Ordered X-Form					
	else if (ISOP("fcmpu"))	value += incBFFRAFRB;				//Floating Compare Unordered X-Form					
	
	else
	{
		if (vecList[0].at(vecList[0].size()-2) == 's' || vecList[0].at(vecList[0].size() - 2) == 'S' ||
			vecList[0].at(vecList[0].size()-1) == 's' || vecList[0].at(vecList[0].size() - 1) == 'S')
			value = setOpBeginning(59);
		if (ISOP("fadd"))		value += 21 * 2 + incRTRARB;					//Floating Add A-Form
		else if (ISOP("fsub"))	value += 20 * 2 + incRTRARB;					//Floating Subtract A-Form
		else if (ISOP("fmul"))	value += 25 * 2 + incRTRA + vecReg(3) * 64;		//Floating Multiply A-Form
		else if (ISOP("fdiv"))	value += 18 * 2 + incRTRARB;					//Floating Divide A-Form
		else if (ISOP("fsqrt"))	value += 22 * 2 + incRTRB;						//Floating Square Root A-Form
		else if (ISOP("fres"))	value += 24 * 2 + incRTRB;						//Floating Reciprocal Estimate A-Form
		else if (ISOP("frsqrte"))value += 26 * 2 + incRTRB;						//Floating Reciprocal Square Root Estimate A-Form
		else if (ISOP("fsel"))
		{
			value += setRARSSHMBME(vecReg(2), vecReg(1), vecReg(4), vecReg(3), 23);
		}
		else if (ISOP("fmadd"))													//Floating Multiply-Add A-Form
		{
			value +=  setRARSSHMBME(vecReg(2), vecReg(1), vecReg(4), vecReg(3),29);
		}
		else if (ISOP("fmsub"))													//Floating Multiply-Subtract A-Form
		{
			value += setRARSSHMBME(vecReg(2), vecReg(1), vecReg(4), vecReg(3), 28);
		}
		else if (ISOP("fnmadd"))												//Floating Negative Multiply-Add A-Form
		{
			value += setRARSSHMBME(vecReg(2), vecReg(1), vecReg(4), vecReg(3), 31);
		}
		else if (ISOP("fnmsub"))												//Floating Negative Multiply-Subtract A-Form
		{
			value += setRARSSHMBME(vecReg(2), vecReg(1), vecReg(4), vecReg(3), 30);
		}
	}


	if (boost::ifind_first(vecList[0], "."))
		value++;
}
void PPCop::opTrap(vector<string>& vecList)
{
	if (ISOP("tdi"))		value = setOpBeginning(2) + incTORASI;				//Trap Doubleword Immediate D-Form
	else if (ISOP("twi"))	value = setOpBeginning(3) + incTORASI;				//Trap Word Immediate D-Form
	else if (ISOP("td"))	value = setOpBeginning(31) + 68 * 2 + incTORARB;	//Trap Doubleword X-Form
	else if (ISOP("tw"))	value = setOpBeginning(31) +  4 * 2 + incTORARB;	//Trap Word X-Form
}