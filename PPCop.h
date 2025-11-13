#ifndef GCTRM_PPC_H
#define GCTRM_PPC_H

#include <string>
#include <vector>
#include <sstream>
#include "utility.h"
#include "aliasGroup.h"

using namespace std;

#define _STOI(line1) stoul(line1,nullptr,0)
#define vecReg(num)  regClean(vecList[num])
#define ISOP(OPERATIONNAME) isOp(vecList[0],OPERATIONNAME)
#define incBTBABB setBTBABB(vecReg(1),vecReg(2),vecReg(3))
#define incBTBABA setBTBABB(vecReg(1),vecReg(2),vecReg(2))
#define incBTBTBT setBTBABB(vecReg(1),vecReg(1),vecReg(1))
#define incBFFRAFRB setBTBABB(vecReg(1)*4,vecReg(2),vecReg(3))
#define incRTRARB incBTBABB
#define incRTRA setBTBABB(vecReg(1),vecReg(2),0)
#define incRTRB setBTBABB(vecReg(1),0,vecReg(2))
#define incRTRBRA setBTBABB(vecReg(1),vecReg(3),vecReg(2))
#define incRTDRA setRTDRA(vecReg(1),stoi(vecList[2],nullptr,0),vecReg(3))
#define incRTDSRA setRTDRA(vecReg(1),stoi(vecList[2],nullptr,0)*4,vecReg(3))
#define incRTRANB incBTBABB
#define incRTRASI setRTDRA(vecReg(1),stoi(vecList[3],nullptr,0),vecReg(2))
#define incRARSUI setRTDRA(vecReg(2),stoi(vecList[3],nullptr,0),vecReg(1))
#define incRARSRB setBTBABB(vecReg(2),vecReg(1),vecReg(3))
#define incRARSRS setBTBABB(vecReg(2),vecReg(1),vecReg(2))
//#define incRARSRA setBTBABB(vecReg(2),vecReg(1),vecReg(2))
#define incRARS setBTBABB(vecReg(2),vecReg(1),0)
#define incTORASI incRTRASI
#define incTORARB incRTRARB
#define decRTRASI setRTDRA(vecReg(1),(uint16_t)(-(stoi(vecList[3],nullptr,0))),vecReg(2))
#define incRTSI setRTDRA(vecReg(1),stoi(vecList[2],nullptr,0),0)
#define incRT setBTBABB(vecReg(1),0,0)
#define incRARSRBME incRARSRB + vecReg(4) * 32 
#define incRARSRBMB incRARSRBME
#define incRARSSHMB incRARSRBME
#define incRARSRBMBME setRARSSHMBME(vecReg(1),vecReg(2),vecReg(3),vecReg(4),vecReg(5))
#define incRARSSHMBME incRARSRBMBME
#define incRARSSH incRARSRB
#define incFRDFRAFRCFRB(val) setRARSSHMBME(vecReg(2),vecReg(1),vecReg(4),vecReg(3),val)


class PPCop
{
private:
	enum operation
	{
		b, ba, bl, bla, bc, bca, bcl, bcla, bctr,
		bclr, bclrl, bcctr, bcctrl
	};
	string regNo[4] = { "","","","" };
	uint32_t value;
	int opLength;
	bool badOp = true;
	uint32_t setOpBeginning(uint32_t val), getOpBeginning();
	uint32_t setBOBI(uint32_t BO, uint32_t BI), setBTBABB(uint32_t BT, uint32_t BA, uint32_t BB);
	uint32_t setRTDRA(uint32_t RT, uint32_t D, uint32_t RA), setRARSSHMBME(uint32_t RA, uint32_t RS, uint32_t SH, uint32_t MB, uint32_t ME);
	uint32_t regClean(string& regval);
	bool isOp(string opString, string compare);
	void checkBranchCondition(string& opString);
	void cleanCode(vector<string>& vecList, string opString, aliasGroup& parentCodeLocal, aliasGroup& parentCodeWide);
	void findAliases(string& comparedString, bool& foundVal, aliasGroup& parentCodeLocal, aliasGroup& parentCodeWide);
public:
	PPCop(uint32_t setHex), PPCop(string setString), PPCop();
	operation opType;
	string labelRequest = "";
	void detectOperation(string opString, aliasGroup& parentCodeLocal, aliasGroup& parentCodeWide);
	void showDistance(int offset);
	uint32_t init(string& operation);
	void set(string hex);
	void enforceOffset(int16_t hex);
	void setDirect(uint32_t hex);
	void errorCheck(bool& error);
	uint8_t retrieveByte(int byte);
private:
	void opBranch(vector<string>& vecList), opConReg(vector<string>& vecList); //b, cr
	void opLoad(vector<string>& vecList), opStore(vector<string>& vecList), opMath(vector<string>& vecList); 
	void opMove(vector<string>& vecList), opTrap(vector<string>& vecList), opCompare(vector<string>& vecList);
	void opRotate(vector<string>& vecList), opFloat(vector<string>& vecList), opPairedSingle(vector<string>& vecList);
	void opInst(vector<string>& vecList);
	void opTypes(vector<string>& vecList), psaTypes(vector<string>& vecList, int access);
	/*todo: 
	sc
	mcrf, mcrfs, mtfsfi, mtfsf, mtfsb0, mtfsb1, mtocrf, mfocrf
	rldc, rldcl, rldic, rldicl, rldicr, rldimi, sradi
	paired singles

	mnemonics for:
	rlwinm
	
	*/
};

/*

CRF0 (CRF0-CRF7)
		Fixed-Point Comparison	Fixed-Point Computation	Floating-Point Comparison

cr0 =	Less Than				Negative				Less Than
cr1 =	Greater Than			Positive				Greater Than
cr2 =	Equal					Zero					Equal
cr3 =	Summary Overflow		Summary Overflow		Unordered

CRF1

cr4 = Floating-Point Exception Summary
cr5 = Floating-Point Enabled Exception Summary
cr6 = Floating-Point Invalid Operation Exception Summary
cr7 = Floating-Point Overflow Exception

BO register
crX   = 01100
crX+  = 01111
crX-  = 01110
!crX  = 00100
!crX+ = 00111
!crX- = 00110

X = 0-4 accessible

blt = bc cr0    = bc 12, -	
bge = bc !cr0
bgt = bc cr1
ble = bc !cr1
beq = bc cr2
bne = bc !cr2   = bc 4, 10

bdz
bdnz			= bc 16, 0

BO register
bdz		= 10010
bdz+	= 11011
bdz-	= 11010
bdnz	= 10000
bdnz+	= 11001
bdnz-	= 11000

*/

#endif