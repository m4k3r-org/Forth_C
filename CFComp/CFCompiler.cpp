#include "stdafx.h"
#include "CFCompiler.h"

OPCODE_T opcodes[] = {
	{ _T("RESET"), RESET, _T("RESET") }
	, { _T("PUSH"), PUSH, _T("") }
	, { _T("CPUSH"), CPUSH, _T("") }
	, { _T("FETCH"), FETCH, _T("@") }
	, { _T("STORE"), STORE, _T("!") }
	, { _T("SWAP"), SWAP, _T("SWAP") }
	, { _T("DROP"), DROP, _T("DROP") }
	, { _T("DUP"), DUP, _T("DUP") }
	, { _T("ROT"), ROT, _T("ROT") }
	, { _T("OVER"), OVER, _T("OVER") }
	, { _T("TUCK"), TUCK, _T("TUCK") }
	, { _T("JMP"), JMP, _T("JMP") }
	, { _T("JMPZ"), JMPZ, _T("JMPZ") }
	, { _T("JMPNZ"), JMPNZ, _T("JMPNZ") }
	, { _T("CALL"), CALL, _T("") }
	, { _T("RET"), RET, _T("LEAVE") }
	, { _T("CFETCH"), CFETCH, _T("C@") }
	, { _T("CSTORE"), CSTORE, _T("C!") }
	, { _T("ADD"), ADD, _T("+") }
	, { _T("SUB"), SUB, _T("-") }
	, { _T("MUL"), MUL, _T("*") }
	, { _T("DIV"), DIV, _T("/") }
	, { _T("LT"), LT, _T("<") }
	, { _T("EQ"), EQ, _T("=") }
	, { _T("GT"), GT, _T(">") }
	, { _T("DICTP"), DICTP, _T("DICTP") }
	, { _T("EMIT"), EMIT, _T("EMIT") }
	, { _T("FOPEN"), FOPEN, _T("FOPEN") }
	, { _T("FREAD"), FREAD, _T("FREAD") }
	, { _T("FREADLINE"), FREADLINE, _T("FREADLINE") }
	, { _T("FWRITE"), FWRITE, _T("FWRITE") }
	, { _T("FCLOSE"), FCLOSE, _T("FCLOSE") }
	, { _T("DTOR"), DTOR, _T(">R") }
	, { _T("RFETCH"), RFETCH, _T("R@") }
	, { _T("RTOD"), RTOD, _T("R>") }
	, { _T("ONEPLUS"), ONEPLUS, _T("1+") }
	, { _T("PICK"), PICK, _T("PICK") }
	, { _T("DEPTH"), DEPTH, _T("DEPTH") }
	, { _T("BREAK"), BREAK, _T("BREAK") }
	, { _T("BYE"), BYE, _T("BYE") }
	, { _T(""), 0, _T("") }
};

CCFCompiler::CCFCompiler()
{
}


CCFCompiler::~CCFCompiler()
{
}

void CCFCompiler::Compile(LPCTSTR m_source, LPCTSTR m_output)
{
	SP = 0;
	HERE = 0;
	LAST = MEM_SZ - (DSTACK_SZ + RSTACK_SZ) - sizeof(int);
	STATE = 0;
	line_no = 0;
	memset(the_memory, 0x00, sizeof(the_memory));
	SetAt(LAST, 0);
	SetAt(ADDR_SP, LAST  + sizeof(int));
	SetAt(ADDR_RSP, GetAt(ADDR_SP) + DSTACK_SZ);

	CW2A source(m_source);
	CW2A output(m_output);
	FILE *fp_in = NULL;
	fopen_s(&fp_in, source, "rt");

	char buf[128];
	if (fp_in)
	{
		while (fgets(buf, sizeof(buf), fp_in) == buf)
		{
			++line_no;
			CString line(buf);
			line.Trim();
			Parse(line);
		}
		fclose(fp_in);
	}

	CELL start_here = FindWord(CString(_T("main")));
	if (start_here <= 0)
	{
		start_here = GetAt(LAST + 4);
	}

	SetAt(0, JMP);
	SetAt(1, start_here);
	the_memory[ADDR_CELL] = CELL_SZ;
	SetAt(ADDR_LAST, LAST);
	SetAt(ADDR_HERE, HERE);
	SetAt(ADDR_BASE, 10);
	SetAt(ADDR_STATE, 0);

	FILE *fp_out = NULL;
	fopen_s(&fp_out, "dis.txt", "wt");
	if (fp_out)
	{
		Dis(fp_out);
		fclose(fp_out);
	}

	//fopen_s(&fp_out, output, "wt");
	//if (fp_out)
	//{
	//	for (int i = 0; i < HERE; i++)
	//	{
	//		fprintf(fp_out, "%08lx %02x\n", i, the_memory[i]);
	//	}

	//	for (int i = LAST; i < MEM_SZ; i++)
	//	{
	//		fprintf(fp_out, "%08lx %02x\n", i, the_memory[i]);
	//	}
	//	fclose(fp_out);
	//	fp_out = NULL;
	//}

}

BYTE CCFCompiler::FindAsm(CString& word)
{
	for (int i = 0; opcodes[i].opcode != 0; i++)
	{
		if (opcodes[i].asm_instr == word)
		{
			return opcodes[i].opcode;
		}
	}
	return 0;
}

BYTE CCFCompiler::FindForthPrim(CString& word)
{
	for (int i = 0; opcodes[i].opcode != 0; i++)
	{
		if (opcodes[i].forth_prim == word)
		{
			return opcodes[i].opcode;
		}
	}
	return 0;
}

CELL CCFCompiler::FindWord(CString& word)
{
	CW2A wd(word);
	DICT_T *dp = (DICT_T *)(&the_memory[LAST]);
	while (dp->next > 0)
	{
		if (strcmp(wd, dp->name) == 0)
		{
			return dp->XT;
		}
		dp = (DICT_T *)(&the_memory[dp->next]);
	}

	return 0;
}

OPCODE_T *CCFCompiler::FindOpcode(BYTE opcode)
{
	for (int i = 0; opcodes[i].opcode != 0; i++)
	{
		if (opcodes[i].opcode == opcode)
		{
			return &opcodes[i];
		}
	}
	return NULL;
}

BOOL IsNumeric(char *w)
{
	while (*w)
	{
		if ((*w < '0') || (*w > '9'))
			return 0;
		w++;
	}
	return 1;
}

BOOL CCFCompiler::MakeNumber(CString& word, CELL& the_num)
{
	CW2A w(word);
	if (IsNumeric(w))
	{
		the_num = (CELL)atol(w);
		return 1;
	}
	return 0;
}

void CCFCompiler::DefineWord(CString& word, BYTE flags)
{
	CELL tmp = LAST;
	LAST -= ((CELL_SZ*2) + 3 + word.GetLength());

	DICT_T *dp = (DICT_T *)(&the_memory[LAST]);
	dp->next = tmp;
	dp->XT = HERE;
	dp->flags = flags;
	dp->len = word.GetLength();

	tmp = 0;
	CW2A wd(word);
	char *cp = wd;
	while (*cp)
	{
		dp->name[tmp++] = *(cp++);
	}
	dp->name[tmp++] = NULL;
}

void CCFCompiler::SetAt(CELL loc, CELL num)
{
	*(CELL *)(&the_memory[loc]) = num;
}

void CCFCompiler::Comma(CELL num)
{
	if ((HERE < LAST) && (HERE < (MEM_SZ - CELL_SZ)))
	{
		SetAt(HERE, num);
		HERE += CELL_SZ;
	}
	else
	{

	}
}

void CCFCompiler::CComma(BYTE num)
{
	if (HERE < LAST)
	{
		the_memory[HERE] = num;
		HERE += 1;
	}
	else
	{

	}
}

void CCFCompiler::GetWord(CString& line, CString& word)
{
	word.Empty();
	line.TrimLeft();
	int pos = line.Find(_T(" "));
	if (pos >= 0)
	{
		word = line.Left(pos);
		line = line.Mid(pos + 1);
	}
	else
	{
		word = line;
		line.Empty();
	}
}

void CCFCompiler::Parse(CString& line)
{
	CString source = line;
	CString parsed;
	line.Replace(_T("\t"), _T(" "));
	CString word;
	while (line.GetLength())
	{
		GetWord(line, word);
		if (parsed.GetLength() > 0)
		{
			parsed.AppendChar(' ');
		}
		parsed.AppendFormat(_T("%s"), word);

		if (word == _T("\\"))
		{
			return;
		}

		if (word == ".ORG")
		{
			GetWord(line, word);
			CELL addr = 0;
			if (MakeNumber(word, addr))
			{
				HERE = addr;
			}
			continue;
		}

		if (word == _T(":"))
		{
			STATE = 1;
			GetWord(line, word);
			parsed.AppendFormat(_T(" %s"), word);
			DefineWord(word, 0);
			CComma(DICTP);
			Comma(LAST);
			continue;
		}

		if (word == _T("<asm>"))
		{
			Push(STATE);
			STATE = 2;
			continue;
		}

		if (word == _T("</asm>"))
		{
			STATE = Pop();
			continue;
		}

		if (word == _T(":!"))
		{
			STATE = 1;
			GetWord(line, word);
			parsed.AppendFormat(_T(" %s"), word);
			DefineWord(word, 1);
			CComma(DICTP);
			Comma(LAST);
			continue;
		}

		if (word == ";")
		{
			STATE = 0;
			CComma(RET);
			continue;
		}

		if (word == "IF")
		{
			CComma(JMPZ);
			Push(HERE);
			Comma(0);
			continue;
		}

		if (word == "ELSE")
		{
			CELL tmp = Pop();
			CComma(JMP);
			Push(HERE);
			Comma(0);
			SetAt(tmp, HERE);
			continue;
		}

		if (word == "THEN")
		{
			CELL tmp = Pop();
			SetAt(tmp, HERE);
			continue;
		}

		if (word == "BEGIN")
		{
			Push(HERE);
			continue;
		}

		if (word == "AGAIN")
		{
			CComma(JMP);
			Comma(Pop());
			continue;
		}

		if (word == "WHILE")
		{
			CComma(JMPNZ);
			Comma(Pop());
			continue;
		}

		if (word == "UNTIL")
		{
			CComma(JMPZ);
			Comma(Pop());
			continue;
		}

		BYTE opcode = 0;

		if ((STATE == 0) || (STATE == 2))
		{
			opcode = FindAsm(word);
			if (opcode >= 0)
			{
				CComma(opcode);
				continue;
			}
		}

		opcode = FindForthPrim(word);
		if (0 < opcode)
		{
			CComma(opcode);
			continue;
		}

		CELL XT = FindWord(word);
		if (XT > 0)
		{
			CComma(CALL);
			Comma(XT);
			continue;
		}

		CELL num = 0;
		if (MakeNumber(word, num))
		{
			if (num < 256)
			{
				if (STATE == 1)
					CComma(CPUSH);
				CComma((BYTE)num);
			}
			else
			{
				if (STATE == 1)
					CComma(PUSH);
				Comma(num);
			}
			continue;
		}

		// Error here?
	}
}

void CCFCompiler::Dis(FILE *fp)
{
	CELL PC = 0;
	while (PC < HERE)
	{
		PC = Dis1(PC, fp);
	}

	fprintf(fp, "\n%0*x ; The dictionary starts here ...\n", CELL_SZ*2, LAST);

	PC = LAST;
	while (PC >= LAST)
	{
		PC = DisDict(PC, fp);
	}
}

CELL CCFCompiler::GetAt(CELL loc)
{
	return *(CELL *)(&the_memory[loc]);
}

void DisOut(FILE *fp, CString& line, CString& desc, int wid = 24)
{
	if (!desc.IsEmpty())
	{
		while (line.GetLength() < wid)
		{
			line.AppendChar(' ');
		}

		line.AppendFormat(_T("; %s"), desc);
	}

	CW2A out(line);
	fprintf(fp, "%s\n", out);
}

void CCFCompiler::GetWordName(CELL loc, CString& name)
{
	if ((0 <= loc) && (loc < MEM_SZ))
	{
		DICT_T *dp = (DICT_T *)&the_memory[loc];
		name.Format(_T("%S"), dp->name);
	}
	else
	{
		name = _T("??");
	}
}

void CCFCompiler::DisRange(CString& line, CELL loc, CELL num)
{
	while (num-- > 0)
	{
		line.AppendFormat(_T(" %02x"), the_memory[loc++]);
	}
}

CELL CCFCompiler::Dis1(CELL PC, FILE *fp)
{
	int CELL_WD = CELL_SZ * 2;
	CString line, desc = _T("(data)");
	BYTE op = the_memory[PC++];
	line.Format(_T("%0*x %02x"), CELL_WD, PC - 1, op);
	int line_len = 32;
	if (op == JMPZ)
	{
		DisRange(line, PC, CELL_SZ);
		CELL arg = GetAt(PC);
		PC += CELL_SZ;
		desc.Format(_T("JMPZ %0*x"), CELL_WD, arg);
	}

	else if (op == JMP)
	{
		DisRange(line, PC, CELL_SZ);
		CELL arg = GetAt(PC);
		desc.Format(_T("JMP %0*x"), CELL_WD, arg);
		PC += CELL_SZ;
	}

	else if (op == JMPNZ)
	{
		CELL arg = GetAt(PC);
		DisRange(line, PC, CELL_SZ);
		desc.Format(_T("JMPNZ %0*x"), CELL_WD, arg);
		PC += CELL_SZ;
	}

	else if (op == DICTP)
	{
		CELL arg = GetAt(PC);
		DisRange(line, PC, CELL_SZ);
		desc.Format(_T("DICTP %0*x"), CELL_WD, arg);
		CString tmp;
		GetWordName(arg, tmp);
		if (tmp.Compare(_T("??")) != 0)
		{
			CString tmp;
			tmp.Format(_T("\n%s"), line);
			line = tmp;
			++line_len;
		}
		desc.AppendFormat(_T(" - %s"), tmp);
		PC += CELL_SZ;
	}

	else if (op == CALL)
	{
		CELL arg = GetAt(PC);
		DisRange(line, PC, CELL_SZ);
		desc.Format(_T("CALL %0*x"), CELL_WD, arg);
		CString tmp;
		if ((0 <= arg) && (arg < LAST) && (the_memory[arg] == DICTP))
		{
			arg = GetAt(arg + 1);
			GetWordName(arg, tmp);
		}
		else
		{
			tmp = _T("(unnamed)");
		}
		desc.AppendFormat(_T(" - %s"), tmp);
		PC += CELL_SZ;
	}

	else if (op == PUSH)
	{
		CELL arg = GetAt(PC);
		DisRange(line, PC, CELL_SZ);
		desc.Format(_T("PUSH %0*x"), CELL_WD, arg);
		PC += CELL_SZ;
	}

	else if (op == CPUSH)
	{
		BYTE arg = the_memory[PC++];
		line.AppendFormat(_T(" %02x"), arg);
		desc.Format(_T("CPUSH %02x"), arg);
	}

	else
	{
		OPCODE_T *op_p = FindOpcode(op);
		if (op_p)
		{
			desc = op_p->asm_instr;
		}

	}

	DisOut(fp, line, desc, line_len);
	return PC;
}

CELL CCFCompiler::DisDict(CELL PC, FILE *fp)
{
	CString line, desc;
	int CELL_WD = CELL_SZ * 2;

	DICT_T *dp = (DICT_T *)&the_memory[PC];
	if (dp->next == 0)
	{
		line.Format(_T("\n%0*x"), CELL_WD, PC, dp->next);
		DisRange(line, PC, CELL_SZ);
		desc.Format(_T("End"));
		DisOut(fp, line, desc, 33);
		return 0;
	}

	line.Format(_T("\n%0*x"), CELL_WD, PC);
	DisRange(line, PC, CELL_SZ);
	desc.Format(_T("Next=%0*x, %S"), CELL_SZ, dp->next, dp->name);
	DisOut(fp, line, desc, 33);

	PC += CELL_SZ;
	line.Format(_T("%0*x"), CELL_WD, PC);
	DisRange(line, PC, 5);
	desc.Format(_T("XT=%0*x, flags=%02x"), CELL_WD, dp->XT, dp->flags);
	DisOut(fp, line, desc, 32);

	PC += CELL_SZ + 1;
	line.Format(_T("%0*x"), CELL_WD, PC);
	DisRange(line, PC, dp->len + 2);
	desc.Empty();
	DisOut(fp, line, desc, 32);

	return dp->next;
}

