#include "compileGCT.h"
#include "utility.h"
#include <filesystem>

void compileGCT::compile(const std::filesystem::path& name)
{
	if (std::filesystem::exists(name))
	{
		gctName = std::filesystem::path(name).replace_extension(".GCT").string();
		gctTemp = std::filesystem::path(name).replace_extension("").string();
		processLines(name.c_str(), geckoOps, error);
		GCTgct.open(gctTemp, ios::binary | ios::trunc);
		functionCount = geckoOps.size();
		GCTgct.write((char *)header, 8);
		std::cout << "Compiling . . ." << endl;
		if (::GCTconvert)
			::codeset << "RSBE01" << endl << endl;
		while (!geckoOps.empty())
		{		
			Code *currentCode = &(geckoOps.front());
			int codeSize = currentCode->op.size();
			GCTbytes += codeSize;
			if (::provideTXT)
				::codeset << currentCode->name << endl;
			while (!currentCode->op.empty())
			{
				for (int j = 0; j < codeSize; j++)
				{
					if (j == currentCode->NextLabelRequest())
					{
						currentCode->ImplementLabel();
						currentCode->labelFillList.pop_front();
					}
					if (::provideTXT)
					{
						if (j % 2 == 0 && ::astUsage)
							::codeset << "* ";
						else if (j % 2 == 1)
							::codeset << " "; // space out words!
					}
					for (int i = 0; i < 4; i++)
					{
						tempVal[i] = currentCode->op.front().retrieveByte(i);
						if (::provideTXT)
							::codeset << std::format("{:02X}", unsigned(tempVal[i]));
					}
					GCTgct.write((char *)tempVal, 4); // Roundabout way to force endian regardless of hardware
					currentCode->op.pop();
					if (::provideTXT && j % 2 == 1)
						::codeset << endl;
				}
			}
			if (::provideTXT)
				::codeset << endl;
			geckoOps.pop();
		}
		GCTgct.write((char *)footer, 8);
		GCTgct.close();
		GCTbytes *= 4;
		std::cout << (int)GCTbytes << "(0x" << hex << (int)GCTbytes << ") bytes written of gecko code." << endl;
	}
	else
		error = true;

	if (!error)
	{
		std::filesystem::rename(gctTemp, gctName);
	}
	else
	{
		std::cout << "ERROR parsing " << name << "!!" << endl;
		cin.ignore();
		std::filesystem::remove(gctTemp);
	}
}

compileGCT::compileGCT()
{
	functionCount = 0;
	GCTbytes = 0;
	error = false;
}

void compileGCT::processLines(std::filesystem::path name, queue<Code>& geckoOps, bool& error)
{

	textMode mode = seekEnabledCode;
	hookMode writeType = notHooked;
	char tempchar;
	uint32_t hookAddress = 0x0;
	string temp, temp2, tempLine;
	std::filesystem::path directory(name.parent_path());
	
	stack<streamPack> streams;
	queue<PPCop> operations;
	queue<uint8_t> rawBytes;
	vector<label> labels;
	ifstream* currentStream = new ifstream;
	stringstream* currentMacro = nullptr;
	currentStream->open(name);
	streams.emplace((streamAmbig*)currentStream, false, name);
	while (!streams.empty())
	{
		if (streams.top().isMacro)
			currentMacro = (stringstream*)streams.top().streamUnk;
		else
			currentStream = (ifstream*)streams.top().streamUnk;
		while ((!streams.top().isMacro && !currentStream->eof()) || (streams.top().isMacro && currentMacro != nullptr && !currentMacro->eof()))
		{
			temp = "";  tempLine = "";
			if (streams.top().isMacro)
				parseLine(temp, tempLine, mode, currentMacro, labels, rawBytes, operations); //assemble from the line
			else
				parseLine(temp, tempLine, mode, currentStream,labels,rawBytes,operations); //assemble from the line


			if (mode == finishOpCodeMode)
			{
				geckoOps.back().clearLocalMacros();
				uint32_t tempValue = hookAddress & 0x1FFFFFF;
				string scratchLabel;
				if (hookAddress >= 0x82000000)
				{
					if (hookCode)
						geckoOps.back().op.emplace(0x4A000000);
					else
						geckoOps.back().op.emplace(0x42000000);
					geckoOps.back().op.emplace(hookAddress);
					tempValue = 0;
				}
				
				switch (writeType)
				{
				case (hookHook):
					tempValue += 0xC2000000; break;
				case (hookCode):
					if (hookAddress >= 0x82000000)
					{
						tempValue += 0x16000000; break;

					}
					else
					{
						tempValue += 0x06000000; break;

					}
				case (hookPulse):
					tempValue = 0xC0000000; break;
				}
				geckoOps.back().op.emplace(tempValue);
				tempValue = operations.size();
				if (writeType == hookHook)
				{
					if (tempValue % 2 == 0)
						operations.emplace(0x60000000);
					operations.emplace(0);
				}
				else if (tempValue % 2 == 1)
				{
					operations.emplace(0);
				}

				if (writeType == hookHook || writeType == hookPulse)
					geckoOps.back().op.emplace((uint32_t)(operations.size() / 2));
				else
					geckoOps.back().op.emplace((uint32_t)(tempValue * 4));

				for (int i = 0, j = operations.size(); i < j; i++)
				{
					scratchLabel = operations.front().labelRequest;
					if (scratchLabel != "")
					{
						operations.front().showDistance(seekLabelDistance(scratchLabel, i, tempValue, labels));
					}
					geckoOps.back().op.push(operations.front());
					operations.pop();
				}

				if (hookAddress >= 0x82000000)
				{
					geckoOps.back().op.emplace(0xE0000000);
					geckoOps.back().op.emplace(0x80008000);
				}
				while (!labels.empty())
					labels.pop_back();
				temp == "";
				mode = geckoCodeMode;
			}

			if (temp.empty()) { continue; }

			if (temp[0] == '!')
			{
				mode = seekEnabledCode;
			}
			else if (iequals(temp.substr(0, 6), ".alias"))
			{
				if (mode == opCodeMode)
					compileAlias(temp.substr(6), geckoOps.back(), true);
				else if (mode != seekEnabledCode)
					compileAlias(temp.substr(6), geckoOps.back(), false);
			}
			else if (iequals(temp.substr(0, 6), ".macro"))
			{
				if (mode == opCodeMode)
					compileMacro(temp.substr(6), geckoOps.back().localReplaceList, currentStream);
				else if (mode != seekEnabledCode)
					compileMacro(temp.substr(6), geckoOps.back().replaceList, currentStream);
				else
					currentStream->IGNORE('}');
			}
			else if (iequals(temp.substr(0, 8), ".include"))
			{
				erase_all(temp, '\"'); //We don't need quotes in the filename.
				std::filesystem::path targetFile;
				std::filesystem::path incomingPath(temp.substr(8));
				if (incomingPath.begin()->compare(".") == 0 || incomingPath.begin()->compare("..") == 0)
				{
					targetFile = streams.top().filepath.parent_path() / incomingPath;
				}
				else
				{
					targetFile = directory / incomingPath;
				}
				if (!std::filesystem::exists(targetFile) && repairPathCase)
				{
					targetFile = attemptPathCaseRepair(targetFile);
				}
				if (streams.size() <= 16 && std::filesystem::exists(targetFile))
				{
					currentStream = new ifstream;
					currentStream->open(targetFile);
					streams.emplace((streamAmbig*)currentStream, false, targetFile);
					if (::provideLOG)
					{
						for (int i = 2; i < streams.size(); i++)
						{
							::logFile << '\t'; //indent each subfile
						}
						::logFile << temp.substr(8) << endl;
					}
				}
				else if (streams.size() > 16)
				{
					error = true; // ERROR: Encountered inclusion recursion!
					std::cout << "WARNING! Inclusions exceeded 16 levels! Recursion assumed." << endl;
					return;				}
				else
				{
					error = true; // ERROR: File Not Found!
					std::cout << "Could not find file: " << targetFile << "!!!" << endl;
					return;				}
			}
			else if (temp[0] == '%') // Macros!
			{
				if (streams.size() <= 32 && mode != seekEnabledCode)
				{
					openMacro(temp, geckoOps.back(), currentMacro);
					streams.emplace((streamAmbig*)currentMacro, true, streams.top().filepath);
				}
				else if (mode != seekEnabledCode)
				{
					error = true;
					cout << "WARNING! Macro recursion possible!" << endl;
					return;
				}
			}
			else if (temp[0] == '.') // Gecko Codes
			{
				int geckoReg[2] = { 0, 0 };
				int scanOffset = 0;
				uint32_t tempHex[2] = { 0, 0 };
				if (iequals(temp.substr(1, 2), "BA") || iequals(temp.substr(1, 2), "PO"))
				{
					tempHex[0] = 0x40000000;
					if (iequals(temp.substr(1, 2), "PO"))
						tempHex[0] += 0x8000000;
					if (temp[3] == '<' && (temp[4] == '-' || temp[4] == '+'))
					{
						scanOffset = 5;
						if (iequals(temp.substr(5, 3), "PO+"))
						{
							tempHex[0] += 0x10010000;
							scanOffset += 3;
						}
						else if (iequals(temp.substr(5, 3), "BA+"))
						{
							tempHex[0] += 0x10000;
							scanOffset += 3;
						}
						if (iequals(temp.substr(scanOffset, 2), "GR"))
						{
							geckoReg[0] = convCharToHex(temp[scanOffset+2]);
							tempHex[0] += 0x1000 + geckoReg[0];
							scanOffset += 4;
						}
						if (temp[scanOffset] != '$')
						{
							tempHex[0] &= 0x08000000;
							tempHex[0] += 0x46000000;
							geckoOps.back().RequestLabel(temp.substr(scanOffset), geckoOps.back().op.size());
							geckoOps.back().op.emplace(tempHex[0]); 
							geckoOps.back().op.emplace(0);
							goto GeckoCodeClear;
						}
					}
					else if (temp[3] == '-' && temp[4] == '>')
					{
						scanOffset = 5;
						tempHex[0] += 0x4000000;
						if (iequals(temp.substr(5, 3), "PO+"))
						{
							scanOffset += 3;
							tempHex[0] += 0x10010000;
						}
						else if (iequals(temp.substr(5, 3), "BA+"))
						{
							scanOffset += 3;
							tempHex[0] += 0x10000;
						}
						if (iequals(temp.substr(scanOffset, 3), "GR"))
						{
							geckoReg[0] = convCharToHex(temp[scanOffset + 2]);
							tempHex[0] += 0x1000 + geckoReg[0];
							scanOffset += 4;
						}
					}
					else if (temp[3] == '=' || (temp[3] == '+' && temp[4] == '='))
					{
						scanOffset = 4;
						tempHex[0] += 0x2000000;
						if (temp[3] != '=')
						{
							scanOffset++;
							tempHex[0] += 0x100000; //+=
						}
						if (iequals(temp.substr(5, 3), "PO+"))
						{
							scanOffset += 3;
							tempHex[0] += 0x10010000;
						}
						else if (iequals(temp.substr(5, 3), "BA+"))
						{
							scanOffset += 3;
							tempHex[0] += 0x10000;
						}
						if (iequals(temp.substr(scanOffset, 3), "GR"))
						{
							geckoReg[0] = convCharToHex(temp[scanOffset + 2]);
							tempHex[0] += 0x1000 + geckoReg[0];
							scanOffset += 4;
						}
					}
					if (temp[scanOffset] == '$')
					{
						scanOffset++;
					}
					tempHex[1] = (uint32_t)(stoul(temp.substr(scanOffset, 8), nullptr, 16) & 0x7FFFFFFF);
				
					geckoOps.back().op.emplace(tempHex[0]);
					geckoOps.back().op.emplace(tempHex[1]);
				}
				else if (iequals(temp.substr(1, 2), "GR"))
				{
					geckoReg[0] = convCharToHex(temp[3]);
					if ((temp[5] == '=' && temp[4] != '+') || temp[5] == '<')
					{
						scanOffset = 6;
						if (temp[5] != '=')
							tempHex[0] += 0x20000;
						if (temp[6] == '$')
						{
							scanOffset++;
							tempHex[0] += 0x10000;
						}
						if (iequals(temp.substr(scanOffset, 2), "GR"))
						{
							geckoReg[1] = convCharToHex(temp[scanOffset+2]);
							tempHex[1] = geckoReg[1];
							tempHex[0] += 0x88000000 + geckoReg[0]; //88T - Gecko Register Operations
						}
						else
						{
							tempHex[1] = (uint32_t)stoul(temp.substr(scanOffset, 8), nullptr, 16);
							tempHex[1] &= 0x7FFFFFFF;
							tempHex[0] += 0x86000000 + geckoReg[0]; //86T - Gecko Register / Direct Value Operations
						}
						switch (temp[4])
						{
						case 'x': tempHex[0] += 0x100000; //fmuls single float multiply
						case 'a': tempHex[0] += 0x100000; //fadds single float add
						case ')': tempHex[0] += 0x100000; //arithmetic shift right
						case '(': tempHex[0] += 0x100000; //rotate left
						case ']': tempHex[0] += 0x100000; //shift right
						case '[': tempHex[0] += 0x100000; //shift left
						case '^': tempHex[0] += 0x100000; //xor
						case '&': tempHex[0] += 0x100000; //and
						case '|': tempHex[0] += 0x100000; //or
						case '*': tempHex[0] += 0x100000; //multiply
						case '+': default: 			      //add
							break;						}
					}
					else if (temp[4] == '=' || temp[4] == '+')
					{
						tempHex[0] = 0x80000000 + geckoReg[0];
						if (iequals(temp.substr(4, 4), "=BA+")) { tempHex[0] += 0x010000;   scanOffset = 8; }
						else if (iequals(temp.substr(4, 4), "=PO+")) { tempHex[0] += 0x10010000; scanOffset = 8; }
						else if (iequals(temp.substr(4, 5), "+=BA+")) { tempHex[0] += 0x110000;   scanOffset = 9; }
						else if (iequals(temp.substr(4, 5), "+=PO+")) { tempHex[0] += 0x10110000; scanOffset = 9; }
						else if (iequals(temp.substr(4, 2), "+=")) { tempHex[0] += 0x100000;   scanOffset = 6; }
						else if (temp[4] == '=') {scanOffset = 5;}
						else { error = true; }
						if (temp[scanOffset] == '$')
						{
							scanOffset++;
						}
						tempHex[1] = (uint32_t)stoul(temp.substr(scanOffset, 8), nullptr, 16);
						tempHex[1] &= 0x7FFFFFFF;
					}
					else if ((temp[4] == '<' && temp[5] == '-') || (temp[4] == '-' && temp[5] == '>'))
					{
						if (temp[4] == '<')
							tempHex[0] = 0x82000000; //82UY - Load into Gecko Register
						else
							tempHex[0] = 0x84000000; //84UY - Store Gecko Register at
						if (iequals(temp.substr(6, 3), "(8)"))
						{
							scanOffset = 9;
						}
						else if (iequals(temp.substr(6,4), "(16)"))
						{
							scanOffset = 10;
							tempHex[0] += 0x100000;
						}
						else if (iequals(temp.substr(6, 4), "(32)"))
						{
							scanOffset = 10;
							tempHex[0] += 0x200000;
						}
						else
						{
							scanOffset = 6;
							tempHex[0] += 0x200000;
						}
						if (iequals(temp.substr(scanOffset, 3), "BA+"))
						{
							scanOffset += 3;
							tempHex[0] += 0x00010000; //+po
						}
						else if (iequals(temp.substr(scanOffset, 3), "PO+"))
						{
							scanOffset += 3;
							tempHex[0] += 0x10010000; //+po
						}
						if (temp[scanOffset] == '$')
						{
							scanOffset++;
						}
						tempHex[1] = (uint32_t)stoul(temp.substr(scanOffset, 8), nullptr, 16);
						tempHex[1] &= 0x7FFFFFFF;					}
					geckoOps.back().op.emplace(tempHex[0]);					geckoOps.back().op.emplace(tempHex[1]);
				}
				else if (iequals(temp.substr(1, 4), "GOTO"))
				{
					if (iequals(temp.substr(5, 4), "_T->"))
					{
						geckoOps.back().RequestLabel(temp.substr(9), geckoOps.back().op.size(), 1);
						geckoOps.back().op.emplace(0x66000000); //6600
						geckoOps.back().op.emplace(0);
					}
					else if (iequals(temp.substr(5, 4), "_F->"))
					{
						geckoOps.back().RequestLabel(temp.substr(9), geckoOps.back().op.size(), 1);
						geckoOps.back().op.emplace(0x66100000); //6610						geckoOps.back().op.emplace(0);
					}
					else if (iequals(temp.substr(5, 2), "->"))
					{
						geckoOps.back().RequestLabel(temp.substr(7), geckoOps.back().op.size(), 1);
						geckoOps.back().op.emplace(0x66200000); //6620
						geckoOps.back().op.emplace(0);
					}
					else
					{
						cout << "ERROR!!! UNKNOWN GOTO TYPE!" << endl;
					}
				}
				else if (iequals(temp.substr(1, 5), "RESET"))
				{
					geckoOps.back().op.emplace(0xE0000000);
					geckoOps.back().op.emplace(0x80008000);
				}
				else if (iequals(temp.substr(1, 5), "ENDIF"))
				{
					geckoOps.back().op.emplace(0xE2000001);
					if (iequals(temp.substr(6, 6), "_RESET"))
					{
						geckoOps.back().op.emplace(0x80008000);
					}
					else
					{
						geckoOps.back().op.emplace(0);
					}
				}
				else if (iequals(temp.substr(1, 5), "ELSE"))
				{
					geckoOps.back().op.emplace(0xE2100001);
					if (iequals(temp.substr(6, 6), "_RESET"))
					{
						geckoOps.back().op.emplace(0x80008000);
					}
					else
					{
						geckoOps.back().op.emplace(0);
					}
				}
				else if (iequals(temp.substr(1, 5), "END"))
				{
					geckoOps.back().op.emplace(0xF0000000);
					geckoOps.back().op.emplace(0x00000000);
				}
				GeckoCodeClear:
				temp = "";
				tempLine = "";
			}
			else if (isJustHex(temp) && mode == geckoCodeMode)
			{
				dumpRawBytes(rawBytes);
				geckoOps.back().op.emplace(temp.substr(0, 8));
				geckoOps.back().op.emplace(temp.substr(8, 8));			}
			else if (mode == opCodeMode)
			{
				operations.emplace();
				operations.back().detectOperation(temp, geckoOps.back().localReplaceList, 
				geckoOps.back().replaceList, (writeType == hookCode) ? hookAddress + ((operations.size() - 1) * 4) : UINT32_MAX);// , geckoOps.back());
			}
			else if (!isJustHex(temp))
			{
				if (iequals(temp.substr(0, 5), "pulse") && mode == geckoCodeMode)
				{
					writeType = hookPulse; dumpRawBytes(rawBytes);
				}
				else if ((hasAddress(temp) || hasType(temp)) && mode == geckoCodeMode)
				{
					if (iequals(temp.substr(0, 4), "code"))
					{
						writeType = hookCode; dumpRawBytes(rawBytes);
					}
					else if (iequals(temp.substr(0, 4), "hook"))
					{
						writeType = hookHook; dumpRawBytes(rawBytes);
					}
					else if (hasAddress(temp))
					{
						dumpRawBytes(rawBytes); handleAddressSet(temp, geckoOps.back());
					}
					else
					{
						handleRaw(temp, rawBytes);
					}
					if (writeType != notHooked)
					{
						for (int i = 0; i < temp.size(); i++)
						{
							if (temp[i] == '@')
							{
								hookAddress = addressConvert(temp.substr(i + 2), geckoOps.back());
								break;
							}
						}
					}
				}
				else if (!hasAddress(temp))
				{
				
					mode = geckoCodeMode; 
					writeType = notHooked;
					geckoOps.emplace(tempLine);  // Creates a new code and sets the name
					geckoOps.back().ShowName();  // Lets the user see progress in realtime.
					if (::provideLOG)       // 1 = log of all code names
					{
						for (int i = 1; i < streams.size(); i++)
						{
							::logFile << '\t';
						}
						::logFile << tempLine << endl;
					}						
				}
			}
		}
		if (streams.top().isMacro)
		{
			streams.pop();
			delete currentMacro;
			currentMacro = nullptr;
		}
		else
		{
			streams.pop();
			currentStream->close();
			delete currentStream;
		}
	}
}

void compileGCT::parseLine(string& temp, string& tempLine, textMode& mode, istream* currentStream,
	vector<label>& labels, queue<uint8_t>& rawBytes, queue<PPCop>& operations) 
{
	bool breakLine = false;
	char tempchar;
	while (!breakLine)
	{
		currentStream->get(tempchar);
		if (currentStream->eof())
			break;
		switch (tempchar)
		{
		case '#': breakLine = true;
		case '|': currentStream->IGNORE('\n'); break;
		case '"':
			temp += tempchar; tempLine += tempchar;
			currentStream->get(tempchar);
			while (!currentStream->eof() && tempchar != '"')
			{
				temp += tempchar; tempLine += tempchar;
				currentStream->get(tempchar);
			}
			temp += tempchar; tempLine += tempchar;
			break;
		case '/':
			if (currentStream->peek() == '*')
			{
				currentStream->get();
				while (!currentStream->eof())
				{
					currentStream->IGNORE('*');
					currentStream->get(tempchar);
					if (tempchar == '/')
						break;
				}
				breakLine = true;
			}
			else if (currentStream->peek() == '/')
			{
				currentStream->IGNORE('\n');
				breakLine = true;
			}
			else
				temp += tempchar; tempLine += tempchar;
			break;
		case '\r': case '\n': case ';': breakLine = true; break; //these dictate to stop reading
		case '\t': case ' ':  //these get ignored unless in PPCASM mode
			if (temp != "" && mode == opCodeMode)
				temp += tempchar;
			tempLine += tempchar;
			break;
		case '{':
			if (mode == seekEnabledCode)	// skip code if looking for a code name when this code is disabled!
				currentStream->IGNORE('}');
			else if (mode == geckoCodeMode)
				mode = opCodeMode;
			break;
		case '}':
			if (mode == opCodeMode)
				mode = finishOpCodeMode;
			breakLine = true;
			break;
		case ':':
			switch (mode)
			{
			case(opCodeMode):
				labels.emplace_back();
				labels.back().labelType = temp;
				labels.back().opOffset = operations.size();
				temp = "";
				break;
			case(geckoCodeMode):
				dumpRawBytes(rawBytes);
				geckoOps.back().labelList.emplace_back();
				geckoOps.back().labelList.back().labelType = temp;
				geckoOps.back().labelList.back().opOffset = geckoOps.back().op.size();
				temp = "";
				break;
			default:
				temp += tempchar; tempLine += tempchar; break;
			}
			break;
		case '*':
			if (temp == "")
				break;
			break;
		default: temp += tempchar; tempLine += tempchar;
		};
		if (iequals(temp, "op"))
		{
			while (tempchar != '@')
			{
				currentStream->get(tempchar);
				temp += tempchar;
			}
		}
	}
	// determines behavior upon reaching a line
}

void compileGCT::compileAlias(string line, Code& alias, bool isLocal)
{
	vector<string> tempStrings;
	bool finishAliases;
	bool containString = false;
	tempStrings.emplace_back();
	for (int i = 0; i < line.size(); i++)
	{
		switch (line.at(i))
		{
		case '-': case '+': case '*': case '/': case '&': case '|': case '^': case '~': case '%':
			tempStrings.emplace_back(); tempStrings.back() = line.at(i);
		case '=': case ',':
			tempStrings.emplace_back();
		case ' ': case '\t': 
			break;
		case '"':
			containString = true;
			while (line.at(++i) != '\"' && i < line.size())
			{
				tempStrings.back() += line.at(i);
			}
			break;
		default:
			tempStrings.back() += line.at(i);
		}
	}
	finishAliases = true;
	while (finishAliases)	// do passes to resolve all aliases to their true meanings
	{
		finishAliases = false;
		for (int j = 1; j < tempStrings.size(); j++)
			alias.findAliases(tempStrings[j], finishAliases);
	}
	if (!containString) // safety
	{
		// math 
		vector<long int> math;
		for (int k = 1, l = 1; k < tempStrings.size() && !containString; k++, l = k)
		{
			switch (tempStrings[k].at(0))
			{
			case '+':
				l++;
				if (tempStrings[l][0] == '+')
					math.back()++;
				else
					math.back() += _STOI(tempStrings[l]);
				l = k; break;
			case '-':
				l++;
				if (tempStrings[l][0] == '-')
					math.back()--;
				else
					math.back() -= _STOI(tempStrings[l]);
				l = k; break;
			case '*':
				k++; math.back() *= _STOI(tempStrings[k]); break;
			case '/':
				k++; math.back() /= _STOI(tempStrings[k]); break;
			case '&':
				k++; math.back() &= _STOI(tempStrings[k]); break;
			case '^':
				k++; math.back() ^= _STOI(tempStrings[k]); break;
			case '|':
				k++; math.back() |= _STOI(tempStrings[k]); break;
			case '%':
				k++; math.back() %= _STOI(tempStrings[k]); break;
			default:
				math.emplace_back(_STOI(tempStrings[k]));

			}
		}
		tempStrings.emplace_back(to_string((uint32_t)math.front()));
	}

	if (isLocal)
	{
		for (int i = 0; i < alias.localReplaceList.aliasList.size(); i++)
		{
			if (iequals(alias.localReplaceList.aliasList.at(i).aliasName, tempStrings.front()))
			{
				alias.localReplaceList.aliasList.at(i).aliasContent = tempStrings.back();
				return;
			}
		}
		alias.localReplaceList.aliasList.emplace_back();
		alias.localReplaceList.aliasList.back().aliasName = tempStrings.front();
		alias.localReplaceList.aliasList.back().aliasContent = tempStrings.back();
	}
	else
	{
		for (int i = 0; i < alias.replaceList.aliasList.size(); i++)
		{
			if (iequals(alias.replaceList.aliasList.at(i).aliasName, tempStrings.front()))
			{
				alias.replaceList.aliasList.at(i).aliasContent = tempStrings.back();
				return;
			}
		}
		alias.replaceList.aliasList.emplace_back();
		alias.replaceList.aliasList.back().aliasName = tempStrings.front();
		alias.replaceList.aliasList.back().aliasContent = tempStrings.back();
		
	}

}
void compileGCT::compileMacro(string line, aliasGroup& replaceList, ifstream* currentStream)
{
	replaceList.macroList.emplace_back();
	bool gotName = false;
	char readChar = '\t';
	for (int i = 0; i < line.size() && line[i] != ')'; i++)
	{
		switch (line[i])
		{
		case '(':
			gotName = true; 
		case '\t': case ' ': 
			break;
		case ',':
			if (gotName)
				replaceList.macroList.back().arguments.emplace_back();
			break;
		default:
			if (gotName)
			{
				if (replaceList.macroList.back().arguments.size() == 0)
					replaceList.macroList.back().arguments.emplace_back();
				replaceList.macroList.back().arguments.back() += line[i]; // writing argument names
			}
			else
				replaceList.macroList.back().name += line[i];

		}

	}
	currentStream->IGNORE('{');
	while (!currentStream->eof())
	{
		currentStream->get(readChar);
		if (readChar == '}')
			return;
		else
			replaceList.macroList.back().content += readChar;
	}

}
void compileGCT::openMacro(string& line, Code& macroContain, stringstream*& currentStream)
{
	vector<string> argOrig, arguments;
	string macroName, tempMacro;
	int i, parenNest = 0;

	bool gotName = false;
	char readChar = '\t';
	for (int i = 0; i < line.size(); i++)
	{
		switch (line[i])
		{
		case '(':
			gotName = true;
			parenNest++;
			break;
		case ')':
			parenNest--;
		case '\t': case ' ': 
			break;
		case ',':
			if (gotName && parenNest == 1)
				arguments.emplace_back();
			break;
		default:
			if (gotName)
			{
				if (arguments.size() == 0)
					arguments.emplace_back();
				arguments.back() += line[i]; // writing argument names
			}
			else
				macroName += line[i];
		}
	}
	tempMacro = macroContain.getMacro(macroName.substr(1), argOrig);
	if (argOrig.size() > arguments.size())
	{
		error = true;
		cout << "Inappropriate argument count for macro " << macroName << "!!" << endl;
		return;		// It won't know what to do!
	}

	for (int i = 0, t = 1; i < arguments.size() && t; i++)
	{
		for (int j = 0; j < macroContain.localReplaceList.aliasList.size() && t; j++)
		{
			if (iequals(arguments.at(i), macroContain.localReplaceList.aliasList.at(j).aliasName))
			{
				arguments.at(i) = macroContain.localReplaceList.aliasList.at(j).aliasContent; t = 0;  break;
			}
		}
		for (int j = 0; j < macroContain.replaceList.aliasList.size() && t; j++)
		{
			if (iequals(arguments.at(i), macroContain.replaceList.aliasList.at(j).aliasName))
			{
				arguments.at(i) = macroContain.replaceList.aliasList.at(j).aliasContent; t = 0; break;
			}
		}
	}

	for (int i = 0; i < argOrig.size(); i++)	// Replace all arguments
		replace_all(tempMacro, argOrig.at(i), arguments.at(i));


	currentStream = new stringstream;
	currentStream->str(tempMacro);
}


int compileGCT::seekLabelDistance(string labelName, int offsetOp, int maxSize, vector<label>& labels)
{
	if (iequals(labelName,"%END%"))
		return (maxSize - offsetOp) * 4;
	else if (equals(labelName,"%START%"))
		return (0 - offsetOp) * 4;
	for (int i = 0; i < labels.size(); i++)
	{
		if (iequals(labels[i].labelType, labelName))
		{
			return (labels[i].opOffset - offsetOp) * 4;
		}
	}
	return 0;
}
void compileGCT::handleRaw(string& line, queue<uint8_t>& content)
{
	int arrayCount, tempOff = 0;
	bool bigEndian, isScalar;
	union scrData {
		float f;
		double d;
		PSAtype t;
		uint32_t i;
		uint8_t h[sizeof(double)];
	};
	scrData scratch;
	PSAtype tempVar;
	scratch.f = 1.0;
	bigEndian = (scratch.h[0] == 0x3F) ? true : false;
	if (ISIT("uint8_t") || ISIT("int8_t") || ISIT("byte"))
	{
		arrayCount = getArraySize(line, tempOff);
		for (int i = 0; i < arrayCount; i++)
		{
			scratch.i = STOI_;
			if (i + 1 < arrayCount)
			{
				while (line[tempOff] != ',' && tempOff < line.size() - 1)
					tempOff++;
				tempOff++;
			}
			content.push(scratch.i);
		}
	}
	else if (ISIT("uint16_t") || ISIT("int16_t") || ISIT("half"))
	{
		arrayCount = getArraySize(line, tempOff);
		for (int i = 0; i < arrayCount; i++)
		{
			scratch.i = STOI_;
			if (i + 1 < arrayCount)
			{
				while (line[tempOff] != ',' && tempOff < line.size() - 1)
					tempOff++;
				tempOff++;
			}
			content.push((scratch.i & 0xFF00) / 0x100); content.push(scratch.i & 0xFF);
		}
	}
	else if (ISIT("uint32_t") || ISIT("int32_t") || ISIT("int") || ISIT("word"))
	{
		arrayCount = getArraySize(line, tempOff);
		for (int i = 0; i < arrayCount; i++)
		{
			scratch.i = STOI_;
			if (i + 1 < arrayCount)
			{
				while (line[tempOff] != ',' && tempOff < line.size() - 1)
					tempOff++;
				tempOff++;
			}
			content.push((scratch.i & 0xFF000000) / 0x1000000); content.push((scratch.i & 0xFF0000) / 0x10000);
			content.push((scratch.i & 0xFF00) / 0x100); content.push(scratch.i & 0xFF);
		}
	}
	else if (ISIT("float") || ISIT("scalar"))
	{
		isScalar = (ISIT2("scalar")) ? true : false;

		arrayCount = getArraySize(line, tempOff);
		for (int i = 0; i < arrayCount; i++)
		{
			if (isScalar)
				scratch.i = (int)(stof(line.substr(tempOff)) * 60000);
			else
				scratch.f = stof(line.substr(tempOff));
			if (bigEndian)
				for (int j = 0; j < sizeof(float); j++)
					content.push(scratch.h[j]);
			else
				for (int j = sizeof(float) - 1; j >= 0; j--)
					content.push(scratch.h[j]);
			if (i + 1 < arrayCount)
			{
				while (line[tempOff] != ',' && tempOff < line.size() - 1)
					tempOff++;
				tempOff++;
			}
		}
	}
	else if (ISIT("double"))
	{
		arrayCount = getArraySize(line, tempOff);
		for (int i = 0; i < arrayCount; i++)
		{
			scratch.d = stod(line.substr(tempOff));
			if (bigEndian)
				for (int j = 0; j < sizeof(double); j++)
					content.push(scratch.h[j]);
			else
				for (int j = sizeof(double) - 1; j >= 0; j--)
					content.push(scratch.h[j]);
			if (i + 1 < arrayCount)
			{
				while (line[tempOff] != ',' && tempOff < line.size() - 1)
					tempOff++;
				tempOff++;
			}
		}
	}
	else if (ISIT("string"))
	{
		arrayCount = getArraySize(line, tempOff);
		for (int i = 0; i < arrayCount; i++)
		{
			if (line[tempOff] != '"')
				break;
			else
				while (line[++tempOff] != '"' && tempOff < line.size())
					content.push((uint8_t)line[tempOff]);
			content.push(0); //Null character
			if (i + 1 < arrayCount)
			{
				while (line[tempOff] != ',' && tempOff < line.size() - 1)
					tempOff++;
				tempOff++;
			}
		}
	}
	else if (ISIT("IC_basic") || ISIT("IC_bit") || ISIT("IC_float") ||
		ISIT("LA_basic") || ISIT("LA_bit") || ISIT("LA_float") ||
		ISIT("RA_basic") || ISIT("RA_bit") || ISIT("RA_float"))
	{
		arrayCount = getArraySize(line, tempOff);
		for (int i = 0; i < arrayCount; i++)
		{
			if (ISIT2("IC_basic"))
				tempVar = IC_Basic;
			else if (ISIT2("IC_bit"))
				tempVar = IC_Bit;
			else if (ISIT2("IC_float"))
				tempVar = IC_Float;
			else if (ISIT2("LA_basic"))
				tempVar = LA_Basic;
			else if (ISIT2("LA_bit"))
				tempVar = LA_Bit;
			else if (ISIT2("LA_float"))
				tempVar = LA_Float;
			else if (ISIT2("RA_basic"))
				tempVar = RA_Basic;
			else if (ISIT2("RA_bit"))
				tempVar = RA_Bit;
			else
				tempVar = RA_Float;
			scratch.t = tempVar;
			scratch.i |= stoi(line.substr(tempOff), nullptr, 0) & 0xFFFFFF;

			if (bigEndian)
				for (int j = 0; j < sizeof(int); j++)
					content.push(scratch.h[j]);
			else
				for (int j = sizeof(int) - 1; j >= 0; j--)
					content.push(scratch.h[j]);
			if (i + 1 < arrayCount)
			{
				while (line[tempOff] != ',' && tempOff < line.size() - 1)
					tempOff++;
				tempOff++;
			}
		}
	}
}


bool compileGCT::handleAddressSet(string& line, Code& alias)
{
	int tempOff = 0,  tempAt = 0, geckType = -1, writeLength = 0, arrayCount;
	uint32_t address = 0x80000000, temp = 0;
	bool clearExec = false, isFloat = false, bigEndian, isScalar;
	PSAtype tempVar;
	union scrData {
		float f;
		double d;
		PSAtype t;
		uint32_t i;
		uint8_t h[sizeof(double)];
	};
	scrData scratch;
	scratch.f = 1.0;
	bigEndian = (scratch.h[0] == 0x3F) ? true : false;
	queue<uint8_t> content;
	// 00 = uint8_t, int8_t
	// 02 = uint16_t, int16_t
	// 04 = uint32_t, int32_t
	// 06 = string/int[]/CODE{}
	// 20 = IF [ba+uint24_t] == uint32_t
	// 22 = IF [ba+uint24_t] != uint32_t
	// 24 = IF [ba+uint24_t] > uint32_t
	// 26 = IF [ba+uint24_t] < uint32_t
	// 28 = IF [ba+uint24_t] & !uint16_t == uint16_t
	// 2A !=
	// 2C >
	// 2E <
	// 30 = IF [po+uint24_t] == uint32_t
	// 32 = IF [po+uint24_t] != uint32_t
	// 40 = BA = [address]
	// 42 = BA = address
	// 44 = [address] = BA
	// 46 = BA = NEXTCODE + int16_t
	// 48 = PO = [address]
	// 4A = PO = address
	// 4C = [address] = PO
	// 4E = PO = NEXTCODE + int16_t
	// 80 = grN = address
	// 82 = grN = [address]
	// 84 = address = grN
	// 86 = grN = (grN ? uint32_t)
	// 88 = grN = (grN ? grK)
	// 8A = COPY
	// 8C = COPY
	// 9A = COPY
	// 9C = COPY
	// C0 = PASSCODE{}
	// C2 = HOOK{}
	// C6 = b $address @ BA + uint24_t
	// D6 = b $address @ PO + uint24_t
	// E0 = TERMINATE
	// E2 = ENDIF
	// F0 = END


	for (tempAt = 0; tempAt < line.size(); tempAt++)
		if (line[tempAt] == '@')
			break;
	//if (line[tempAt + 1] != '$')
	//	return false;
	address = addressConvert(line.substr(tempAt + 2), alias);

	
	if (address >= 0x82000000)
		clearExec = true;

	if (ISIT("op"))
	{
		geckType = 0x4;
		tempOp.detectOperation(line.substr(tempOff, tempAt - tempOff), alias.localReplaceList, alias.replaceList, address);

		for (int i = 0; i < 4; i++)
			content.push(tempOp.retrieveByte(i));
	}
	else if (ISIT("hook"))
	{
		geckType = 0xC2;//
	}
	else if (ISIT("code"))
	{
		geckType = 0x6; //


		{
		for (int i = 0; i < 4; i++)
			content.push(tempOp.retrieveByte(i));
		}
	}
	else if (ISIT("address"))
	{
		arrayCount = getArraySize(line, tempOff);
		switch (arrayCount)
		{
		case 1: geckType = 0x4; break;
		default: geckType = 0x6;
		}
		for (int i = 0; i < arrayCount; i++)
		{
			if (line[tempOff] == '$')
				tempOff++;
			scratch.i = (uint32_t)stoul(line.substr(tempOff), nullptr, 16);
			if (i + 1 < arrayCount)
			{
				while (line[tempOff] != ',' && tempOff < line.size() - 1)
					tempOff++;
				tempOff++;
			}
			content.push((scratch.i & 0xFF000000) / 0x1000000); content.push((scratch.i & 0xFF0000) / 0x10000);
			content.push((scratch.i & 0xFF00) / 0x100); content.push(scratch.i & 0xFF);
		}
	}
	else if (ISIT("uint8_t") || ISIT("int8_t") || ISIT("byte"))
	{
		arrayCount = getArraySize(line, tempOff);
		switch (arrayCount)
		{
		case 1: geckType = 0x0; content.push(0); content.push(0); content.push(0); break;
		case 2: geckType = 0x2; content.push(0); content.push(0); break;
		case 4: geckType = 0x4; break;
		default: geckType = 0x6;
		}
		for (int i = 0; i < arrayCount; i++)
		{
			scratch.i = STOI_;
			if (i + 1 < arrayCount)
			{
				while (line[tempOff] != ',' && tempOff < line.size() - 1)
					tempOff++;
				tempOff++;
			}
			content.push(scratch.i);
		}
	}
	else if (ISIT("uint16_t") || ISIT("int16_t") || ISIT("half"))
	{
		arrayCount = getArraySize(line, tempOff);
		switch (arrayCount)
		{
		case 1: geckType = 0x2; content.push(0); content.push(0); break;
		case 2: geckType = 0x4; break;
		default: geckType = 0x6;
		}
		for (int i = 0; i < arrayCount; i++)
		{
			scratch.i = STOI_;
			if (i + 1 < arrayCount)
			{
				while (line[tempOff] != ',' && tempOff < line.size() - 1)
					tempOff++;
				tempOff++;
			}
			content.push((scratch.i & 0xFF00) / 0x100); content.push(scratch.i & 0xFF);
		}
	}
	else if (ISIT("uint32_t") || ISIT("int32_t") || ISIT("int") || ISIT("word"))
	{
		arrayCount = getArraySize(line, tempOff);
		switch (arrayCount)
		{
		case 1: geckType = 0x4; break;
		default: geckType = 0x6;
		}
		for (int i = 0; i < arrayCount; i++)
		{
			scratch.i = STOI_;
			if (i + 1 < arrayCount)
			{
				while (line[tempOff] != ',' && tempOff < line.size() - 1)
					tempOff++;
				tempOff++;
			}
			content.push((scratch.i & 0xFF000000) / 0x1000000); content.push((scratch.i & 0xFF0000) / 0x10000);
			content.push((scratch.i & 0xFF00) / 0x100); content.push(scratch.i & 0xFF);
		}
	}
	else if (ISIT("float") || ISIT("scalar"))
	{
		isScalar = (ISIT2("scalar")) ? true : false;
		arrayCount = getArraySize(line, tempOff);
		geckType = (arrayCount > 1) ? 6 : 4;
		for (int i = 0; i < arrayCount; i++)
		{
			if (isScalar)
				scratch.i = (int)(stof(line.substr(tempOff)) * 60000);
			else
				scratch.f = stof(line.substr(tempOff));
			if (bigEndian)
				for (int j = 0; j < sizeof(float); j++)
					content.push(scratch.h[j]);
			else
				for (int j = sizeof(float) - 1; j >= 0; j--)
					content.push(scratch.h[j]);
			if (i + 1 < arrayCount)
			{
				while (line[tempOff] != ',' && tempOff < line.size() - 1)
					tempOff++;
				tempOff++;
			}
		}
	}
	else if (ISIT("double")) 
	{
		arrayCount = getArraySize(line, tempOff);
		geckType = 0x6;
		for (int i = 0; i < arrayCount; i++)
		{
			scratch.d = stod(line.substr(tempOff));
			if (bigEndian)
				for (int j = 0; j < sizeof(double); j++)
					content.push(scratch.h[j]);
			else
				for (int j = sizeof(double) - 1; j >= 0; j--)
					content.push(scratch.h[j]);
			if (i + 1 < arrayCount)
			{
				while (line[tempOff] != ',' && tempOff < line.size() - 1)
					tempOff++;
				tempOff++;
			}
		}

	}
	else if (ISIT("string"))
	{
		arrayCount = getArraySize(line, tempOff);
		geckType = 0x6;
		for (int i = 0; i < arrayCount; i++)
		{
			if (line[tempOff] != '"')
				return false;
			else
				while (line[++tempOff] != '"' && tempOff < line.size())
					content.push((uint8_t)line[tempOff]);
			content.push(0); //Null character
			if (i + 1 < arrayCount)
			{
				while (line[tempOff] != ',' && tempOff < line.size() - 1)
					tempOff++;
				tempOff++;
			}
		}
	}
	else if (ISIT("IC_basic") || ISIT("IC_bit") || ISIT("IC_float") ||
			ISIT("LA_basic") || ISIT("LA_bit") || ISIT("LA_float") ||
			ISIT("RA_basic") || ISIT("RA_bit") || ISIT("RA_float"))
	{
		arrayCount = getArraySize(line, tempOff);
		geckType = (arrayCount > 1) ? 6 : 4;
		for (int i = 0; i < arrayCount; i++)
		{
			if (ISIT2("IC_basic"))
				tempVar = IC_Basic;
			else if (ISIT2("IC_bit"))
				tempVar = IC_Bit;
			else if (ISIT2("IC_float"))
				tempVar = IC_Float;
			else if (ISIT2("LA_basic"))
				tempVar = LA_Basic;
			else if (ISIT2("LA_bit"))
				tempVar = LA_Bit;
			else if (ISIT2("LA_float"))
				tempVar = LA_Float;
			else if (ISIT2("RA_basic"))
				tempVar = RA_Basic;
			else if (ISIT2("RA_bit"))
				tempVar = RA_Bit;
			else
				tempVar = RA_Float;

			scratch.t = tempVar;
			scratch.i = stoi(line.substr(tempOff), nullptr, 0) & 0xFFFFFF;

			if (bigEndian)
				for (int j = 0; j < sizeof(int); j++)
					content.push(scratch.h[j]);
			else
				for (int j = sizeof(int) - 1; j >= 0; j--)
					content.push(scratch.h[j]);
			if (i + 1 < arrayCount)
			{
				while (line[tempOff] != ',' && tempOff < line.size() - 1)
					tempOff++;
				tempOff++;
			}
		}
	}


	writeLength = content.size();
	if (geckType == 0x6 || address == 0)
		for (int i = 8 - (writeLength % 8); i > 0 && i < 8; i--)
			content.push(0);


	if (geckType == 00 || geckType == 02 || geckType == 04 || geckType == 06)
		if (!clearExec && address >= 0x81000000)
			geckType += 0x01;


	if (clearExec)
	{
		if (geckType == 06 || geckType == 07)
		{
			tempOp.setDirect(0x4A000000);
			geckType += 0x10;
		}
		else 
			tempOp.setDirect(0x42000000);
		geckoOps.back().op.push(tempOp);
		tempOp.setDirect(address); geckoOps.back().op.push(tempOp);
		tempOp.setDirect(geckType * 0x1000000); geckoOps.back().op.push(tempOp);
	}
	else
	{
		tempOp.setDirect((geckType * 0x1000000) + (address & 0xFFFFFF)); geckoOps.back().op.push(tempOp);
	}

	if (geckType == 0x06 || geckType == 0x07 || geckType == 0x16 || geckType == 0x17)
	{
		tempOp.setDirect(writeLength); geckoOps.back().op.push(tempOp);
	}

	
	
	while (!content.empty())
	{
		temp = 0;
		for (int i = 0; i < 4; i++)
		{
			temp *= 0x100;
			if (!content.empty())
			{
				temp += content.front(); content.pop();
			}
		}
		tempOp.setDirect(temp); geckoOps.back().op.push(tempOp);
	}



	if (clearExec)
	{
		tempOp.setDirect(0xE0000000); geckoOps.back().op.push(tempOp);
		tempOp.setDirect(0x80008000); geckoOps.back().op.push(tempOp);
	}
	return true;
}
void compileGCT::dumpRawBytes(queue<uint8_t>& content)
{
	int temp, size = content.size();
	while (!content.empty())
	{
		temp = 0;
		for (int i = 0; i < 4; i++)
		{
			temp *= 0x100;
			if (!content.empty())
			{
				temp += content.front(); content.pop();
			}
		}
		tempOp.setDirect(temp); geckoOps.back().op.push(tempOp);
	}
	if ((size % 8) >= 1 && 4 >= (size % 8))
	{
		tempOp.setDirect(0); geckoOps.back().op.push(tempOp);
	}
}
int compileGCT::getArraySize(string& line, int& tempOff)
{
	int temp = 1;
	if (ISIT("["))
	{
		temp = STOI_;
		while (line[tempOff] != ']' && tempOff < line.size() - 1)
			tempOff++;
		tempOff++;
	}
	return temp;
}
bool compileGCT::isString(string& line, string comp, int& offset)
{
	int tempLength = comp.size();
	if (iequals(line.substr(offset, tempLength), comp))
	{
		offset += tempLength;
		return true;
	}
	return false;
}
bool compileGCT::isString(string& line, string comp)
{
	int tempLength = comp.size();
	if (iequals(line.substr(0, tempLength), comp))
	{
		return true;
	}
	return false;
}

bool compileGCT::isOp(string& line)
{
	return (iequals(line.substr(0, 2), "op"));
}

int compileGCT::convCharToHex(char hexchar)
{
	if (hexchar >= 'a' && hexchar <= 'f')
		return (hexchar - 'a' + 10);
	else if (hexchar >= 'A' && hexchar <= 'F')
		return (hexchar - 'A' + 10);
	else if (hexchar >= '0' && hexchar <= '9')
		return (hexchar - '0');
	return 0;
}

bool compileGCT::hasType(string& line)
{
	int tempOff = 0;
	if (ISIT("uint8_t") || ISIT("int8_t") || ISIT("byte") ||
		ISIT("uint16_t") || ISIT("int16_t") || ISIT("half") ||
		ISIT("uint32_t") || ISIT("int32_t") || ISIT("int") || ISIT("word") ||
		ISIT("string") || ISIT("float") || ISIT("scalar") ||
		ISIT("double") || ISIT("address") || ISIT("op") ||
		ISIT("IC_basic") || ISIT("IC_bit") || ISIT("IC_float") ||
		ISIT("LA_basic") || ISIT("LA_bit") || ISIT("LA_float") ||
		ISIT("RA_basic") || ISIT("RA_bit") || ISIT("RA_float"))
		return true;
	return false;
}
bool compileGCT::hasAddress(string& line)
{
	for (int i = 0; i < line.size(); i++)
		if (line.at(i) == '@')
			return true;
	return false;
}
bool compileGCT::isCodeName(string& line) // Don't use @ in code names!!!!
{
	for (int i = 0; i < line.size(); i++)
		if (line[i] == '@')
			return false;
	return (line.size() == 16 && isJustHex(line)) ? false : true; // expected to have 16 hex numbers if not a name
}

bool compileGCT::isJustHex(string& line)
{
	for (int i = 0; i < line.size(); i++)
		if (!((line[i] >= 'a' && line[i] <= 'f') ||
			(line[i] >= 'A' && line[i] <= 'F') ||
			(line[i] >= '0' && line[i] <= '9')))
			return false;
	if (line.size() != 16)
		return false;
	return true;
}
uint32_t compileGCT::addressConvert(string line, const Code& alias)
{
	uint32_t result = UINT32_MAX;

	for (auto itr = alias.localReplaceList.aliasList.begin(); result == UINT32_MAX && itr != alias.localReplaceList.aliasList.end(); itr++)
	{
		if (equals(itr->aliasName, line))
		{
			result = std::stoul(itr->aliasContent,nullptr,10);
		}
	}
	for (auto itr = alias.replaceList.aliasList.begin(); result == UINT32_MAX && itr != alias.replaceList.aliasList.end(); itr++)
	{
		if (equals(itr->aliasName, line))
		{
			result = std::stoul(itr->aliasContent,nullptr,10);
		}
	}
	if (result == UINT32_MAX)
	{
		result = std::stoul(line.substr(0, 8), nullptr, 16);
	}
	
	return result;
}
uint8_t compileGCT::charPair2Hex(string& line)
{
	uint8_t temp = 0;
	
	for (int i = 0; i < 2; i++)
	{
		temp *= 16;
		if (line[i] >= 'a' && line[i] <= 'f')
			temp += (line[i] - 'a' + 10);
		else if (line[i] >= 'A' && line[i] <= 'F')
			temp += (line[i] - 'A' + 10);
		else if (line[i] >= '0' && line[i] <= '9')
			temp += (line[i] - '0');
		else
		{
			temp /= 16; break;
		}
	}
	return temp;
}
